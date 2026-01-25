#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <cairo.h>
#include <glib.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

namespace {
// --- UI/behavior constants ----------------------------------------------------
// These keep the UI responsive and avoid spamming the kernel sensor files.
constexpr int kIconCanvasSize = 64; // High-DPI friendly size; the system tray will scale it.
constexpr int kMinUpdateMs = 500;   // Avoid reading sensors too frequently.
constexpr int kMaxUpdateMs = 10000; // Keep the tray feeling responsive.
constexpr int kMaxDelaySeconds = 30; // Maximum delay for icon display.

struct Color {
    double r;
    double g;
    double b;
    double a;
};

// Timestamp formatting so every log entry answers *when* something happened.
std::string nowTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buffer;
    localtime_r(&now_time, &tm_buffer);
    std::ostringstream oss;
    oss << std::put_time(&tm_buffer, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Human-friendly error logs with explicit *where*, *why*, and *what file/path*.
void logError(const std::string& where, const std::string& why, const std::string& path = "") {
    std::cerr << "[TempMon][ERROR] " << nowTimestamp() << " | " << where << " | " << why;
    if (!path.empty()) {
        std::cerr << " | Path: " << path;
    }
    std::cerr << std::endl;
}

// Informational logging for configuration and lifecycle events.
void logInfo(const std::string& where, const std::string& message) {
    std::cerr << "[TempMon][INFO] " << nowTimestamp() << " | " << where << " | " << message << std::endl;
}

// Trim helpers to keep config and sensor values predictable.
std::string trim(const std::string& str) {
    const size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "";
    }
    const size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

// Read a single line from a sysfs/config file with explicit error reporting.
std::string readFileLine(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        logError("readFileLine", "Failed to open sensor file.", path);
        return "";
    }

    std::string content;
    if (!std::getline(file, content)) {
        logError("readFileLine", "Failed to read sensor file contents.", path);
        return "";
    }

    return trim(content);
}

// Persist a Cairo surface to disk so AppIndicator can load it as a tray icon.
bool writePng(cairo_surface_t* surface, const std::string& path) {
    const cairo_status_t status = cairo_surface_write_to_png(surface, path.c_str());
    if (status != CAIRO_STATUS_SUCCESS) {
        logError("writePng", std::string("Failed to write icon PNG: ") + cairo_status_to_string(status), path);
        return false;
    }
    return true;
}

// Resolve the per-user config directory in a resilient way.
std::string getConfigDir() {
    const char* config_dir = g_get_user_config_dir();
    if (!config_dir || std::string(config_dir).empty()) {
        logError("getConfigDir", "g_get_user_config_dir returned empty. Falling back to ~/.config.");
        const char* home = g_get_home_dir();
        if (!home || std::string(home).empty()) {
            logError("getConfigDir", "g_get_home_dir returned empty. Using /tmp for config.");
            return "/tmp/tempmon";
        }
        return std::string(home) + "/.config/tempmon";
    }
    return std::string(config_dir) + "/tempmon";
}

// Resolve the per-user cache directory for generated icons.
std::string getCacheDir() {
    const char* cache_dir = g_get_user_cache_dir();
    if (!cache_dir || std::string(cache_dir).empty()) {
        logError("getCacheDir", "g_get_user_cache_dir returned empty. Using /tmp/tempmon.");
        return "/tmp/tempmon";
    }
    return std::string(cache_dir) + "/tempmon";
}

// Create required directories while logging failures in detail.
void ensureDirectory(const std::string& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        if (!fs::create_directories(path, ec)) {
            logError("ensureDirectory", "Failed to create directory.", path);
        }
    }
}
} // namespace

// Metadata for each discovered sensor device.
struct SensorInfo {
    std::string name;
    std::string path;
    std::string type; // "temp", "power", "fan"
    double value = 0.0;
    double max = 0.0;
};

// Holds timestamped temperature readings to support delay previews.
struct TempSample {
    Clock::time_point timestamp;
    double value = 0.0;
};

// Different icon shapes to match user preferences (CoreTemp-like variety).
enum class IconStyle {
    Compact = 0,
    Rounded,
    Square,
};

// Basic theme palettes for contrast and readability.
enum class ThemeStyle {
    Dark = 0,
    Light,
    Accent,
};

// Persistent configuration model (saved in ~/.config/tempmon/config.ini).
struct AppSettings {
    int update_interval_ms = 2000;
    int display_delay_ms = 0;
    IconStyle icon_style = IconStyle::Rounded;
    ThemeStyle theme_style = ThemeStyle::Dark;
    bool show_unit = true;
    bool show_decimal = false;
    std::string selected_sensor_path; // Empty => auto (highest CPU temp)
};

class TempMonitor {
public:
    // Construct and fully initialize the tray indicator, menu, and settings.
    TempMonitor() {
        ensureDirectory(getConfigDir());
        ensureDirectory(getCacheDir());
        loadSettings();

        indicator = app_indicator_new(
            "tempmon",
            "temperature-symbolic",
            APP_INDICATOR_CATEGORY_HARDWARE);

        app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
        app_indicator_set_label(indicator, "", "100°C");

        menu = gtk_menu_new();

        // Discover hardware sensors once at startup; values are refreshed regularly.
        discoverSensors();
        updateSensorValues();
        buildMenu();
        updateIndicator();

        app_indicator_set_menu(indicator, GTK_MENU(menu));
        // Start periodic updates based on the configured interval.
        restartTimer();
    }

    ~TempMonitor() {
        // Ensure the GLib timer is removed cleanly.
        if (timer_id > 0) {
            g_source_remove(timer_id);
        }
    }

    void run() {
        gtk_main();
    }

private:
    AppIndicator* indicator = nullptr;
    GtkWidget* menu = nullptr;
    GtkWidget* settings_window = nullptr;
    guint timer_id = 0;

    std::vector<SensorInfo> sensors;
    AppSettings settings;
    std::deque<TempSample> temp_history;

    std::string configPath() const {
        return getConfigDir() + "/config.ini";
    }

    std::string iconPath() const {
        return getCacheDir() + "/tempmon-icon.png";
    }

    // Read the INI-style config file into settings, with safe fallbacks.
    void loadSettings() {
        std::ifstream file(configPath());
        if (!file.is_open()) {
            logInfo("loadSettings", "No config file found. Using defaults.");
            return;
        }

        std::unordered_map<std::string, std::string> values;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }
            const auto eq_pos = line.find('=');
            if (eq_pos == std::string::npos) {
                continue;
            }
            const std::string key = trim(line.substr(0, eq_pos));
            const std::string value = trim(line.substr(eq_pos + 1));
            values[key] = value;
        }

        if (values.count("update_interval_ms")) {
            settings.update_interval_ms = std::clamp(std::stoi(values["update_interval_ms"]), kMinUpdateMs, kMaxUpdateMs);
        }
        if (values.count("display_delay_ms")) {
            settings.display_delay_ms = std::clamp(std::stoi(values["display_delay_ms"]), 0, kMaxDelaySeconds * 1000);
        }
        if (values.count("icon_style")) {
            settings.icon_style = static_cast<IconStyle>(std::stoi(values["icon_style"]));
        }
        if (values.count("theme_style")) {
            settings.theme_style = static_cast<ThemeStyle>(std::stoi(values["theme_style"]));
        }
        if (values.count("show_unit")) {
            settings.show_unit = values["show_unit"] == "true";
        }
        if (values.count("show_decimal")) {
            settings.show_decimal = values["show_decimal"] == "true";
        }
        if (values.count("selected_sensor_path")) {
            settings.selected_sensor_path = values["selected_sensor_path"];
        }
    }

    // Persist the settings with comments for clarity.
    void saveSettings() {
        std::ofstream file(configPath());
        if (!file.is_open()) {
            logError("saveSettings", "Unable to open config file for writing.", configPath());
            return;
        }

        file << "# TempMon settings - autogenerated\n";
        file << "update_interval_ms=" << settings.update_interval_ms << "\n";
        file << "display_delay_ms=" << settings.display_delay_ms << "\n";
        file << "icon_style=" << static_cast<int>(settings.icon_style) << "\n";
        file << "theme_style=" << static_cast<int>(settings.theme_style) << "\n";
        file << "show_unit=" << (settings.show_unit ? "true" : "false") << "\n";
        file << "show_decimal=" << (settings.show_decimal ? "true" : "false") << "\n";
        file << "selected_sensor_path=" << settings.selected_sensor_path << "\n";
    }

    void discoverSensors() {
        sensors.clear();
        const std::string hwmon_path = "/sys/class/hwmon";

        if (!fs::exists(hwmon_path)) {
            logError("discoverSensors", "hwmon path does not exist; sensors cannot be discovered.", hwmon_path);
            return;
        }

        for (const auto& entry : fs::directory_iterator(hwmon_path)) {
            const std::string hwmon_dir = entry.path().string();
            std::string device_name = readFileLine(hwmon_dir + "/name");
            if (device_name.empty()) {
                device_name = "UnknownDevice";
            }

            // Temperature sensors are most common, so scan a wider range.
            for (int i = 1; i <= 20; i++) {
                const std::string temp_input = hwmon_dir + "/temp" + std::to_string(i) + "_input";
                const std::string temp_label = hwmon_dir + "/temp" + std::to_string(i) + "_label";
                const std::string temp_max = hwmon_dir + "/temp" + std::to_string(i) + "_max";

                if (fs::exists(temp_input)) {
                    SensorInfo sensor;
                    sensor.path = temp_input;
                    sensor.type = "temp";

                    const std::string label = fs::exists(temp_label) ? readFileLine(temp_label) : "";
                    if (!label.empty()) {
                        sensor.name = device_name + " - " + label;
                    } else {
                        sensor.name = device_name + " - Temp" + std::to_string(i);
                    }

                    const std::string max_str = fs::exists(temp_max) ? readFileLine(temp_max) : "";
                    sensor.max = max_str.empty() ? 100.0 : std::stod(max_str) / 1000.0;

                    sensors.push_back(sensor);
                }
            }

            // Power sensors are less common but useful for laptops/GPUs.
            for (int i = 1; i <= 10; i++) {
                const std::string power_input = hwmon_dir + "/power" + std::to_string(i) + "_input";
                const std::string power_label = hwmon_dir + "/power" + std::to_string(i) + "_label";

                if (fs::exists(power_input)) {
                    SensorInfo sensor;
                    sensor.path = power_input;
                    sensor.type = "power";

                    const std::string label = fs::exists(power_label) ? readFileLine(power_label) : "";
                    if (!label.empty()) {
                        sensor.name = device_name + " - " + label;
                    } else {
                        sensor.name = device_name + " - Power" + std::to_string(i);
                    }

                    sensor.max = 0;
                    sensors.push_back(sensor);
                }
            }

            // Fan sensors tend to be limited; keep the scan smaller.
            for (int i = 1; i <= 10; i++) {
                const std::string fan_input = hwmon_dir + "/fan" + std::to_string(i) + "_input";
                const std::string fan_label = hwmon_dir + "/fan" + std::to_string(i) + "_label";

                if (fs::exists(fan_input)) {
                    SensorInfo sensor;
                    sensor.path = fan_input;
                    sensor.type = "fan";

                    const std::string label = fs::exists(fan_label) ? readFileLine(fan_label) : "";
                    if (!label.empty()) {
                        sensor.name = device_name + " - " + label;
                    } else {
                        sensor.name = device_name + " - Fan" + std::to_string(i);
                    }

                    sensor.max = 0;
                    sensors.push_back(sensor);
                }
            }
        }

        if (sensors.empty()) {
            logError("discoverSensors", "No sensors found under /sys/class/hwmon.");
        }
    }

    // Read current values from sysfs into memory.
    void updateSensorValues() {
        for (auto& sensor : sensors) {
            const std::string value_str = readFileLine(sensor.path);
            if (value_str.empty()) {
                continue;
            }

            try {
                const double raw_value = std::stod(value_str);
                if (sensor.type == "temp") {
                    sensor.value = raw_value / 1000.0;
                } else if (sensor.type == "power") {
                    sensor.value = raw_value / 1000000.0;
                } else if (sensor.type == "fan") {
                    sensor.value = raw_value;
                }
            } catch (const std::exception& ex) {
                logError("updateSensorValues", std::string("Failed to parse sensor value: ") + ex.what(), sensor.path);
            }
        }
    }

    // Format sensor values for the dropdown menu list.
    std::string formatSensorValue(const SensorInfo& sensor) const {
        std::ostringstream oss;
        if (sensor.type == "temp") {
            if (settings.show_decimal) {
                oss << std::fixed << std::setprecision(1) << sensor.value;
            } else {
                oss << std::fixed << std::setprecision(0) << sensor.value;
            }
            oss << "°C";
        } else if (sensor.type == "power") {
            oss << std::fixed << std::setprecision(1) << sensor.value << "W";
        } else if (sensor.type == "fan") {
            oss << static_cast<int>(sensor.value) << " RPM";
        }
        return oss.str();
    }

    // Resolve user-selected sensor, if configured.
    const SensorInfo* findSelectedTempSensor() const {
        if (settings.selected_sensor_path.empty()) {
            return nullptr;
        }
        for (const auto& sensor : sensors) {
            if (sensor.path == settings.selected_sensor_path && sensor.type == "temp") {
                return &sensor;
            }
        }
        return nullptr;
    }

    // Auto mode: pick the hottest CPU-related sensor, or fallback to max temp.
    double computeAutoTemperature() const {
        double max_cpu_temp = 0.0;
        for (const auto& sensor : sensors) {
            if (sensor.type != "temp") {
                continue;
            }

            const bool is_cpu = sensor.name.find("CPU") != std::string::npos ||
                                sensor.name.find("Core") != std::string::npos ||
                                sensor.name.find("Tctl") != std::string::npos ||
                                sensor.name.find("Package") != std::string::npos;
            if (is_cpu) {
                max_cpu_temp = std::max(max_cpu_temp, sensor.value);
            }
        }

        if (max_cpu_temp <= 0.0) {
            for (const auto& sensor : sensors) {
                if (sensor.type == "temp") {
                    max_cpu_temp = std::max(max_cpu_temp, sensor.value);
                }
            }
        }

        return max_cpu_temp;
    }

    // Compute real-time or delayed temperature for the tray icon.
    double computeDisplayTemperature() {
        const SensorInfo* selected = findSelectedTempSensor();
        double current = selected ? selected->value : computeAutoTemperature();
        if (current <= 0.0) {
            return current;
        }

        const auto now = Clock::now();
        temp_history.push_back({now, current});
        const auto delay_ms = settings.display_delay_ms;

        if (delay_ms <= 0) {
            while (temp_history.size() > 5) {
                temp_history.pop_front();
            }
            return current;
        }

        const auto target_time = now - std::chrono::milliseconds(delay_ms);
        double delayed_value = current;
        for (const auto& sample : temp_history) {
            if (sample.timestamp <= target_time) {
                delayed_value = sample.value;
            } else {
                break;
            }
        }

        while (!temp_history.empty() && temp_history.front().timestamp < target_time - std::chrono::seconds(5)) {
            temp_history.pop_front();
        }

        return delayed_value;
    }

    // Convert the temperature to the compact text displayed in the icon.
    std::string formatIconText(double temperature) const {
        if (temperature <= 0.0) {
            return "--";
        }

        std::ostringstream oss;
        if (settings.show_decimal) {
            oss << std::fixed << std::setprecision(1) << temperature;
        } else {
            oss << std::fixed << std::setprecision(0) << temperature;
        }

        if (settings.show_unit) {
            oss << "°";
        }

        return oss.str();
    }

    // Decide colors based on theme selection.
    void iconColors(Color& background, Color& text, Color& border) const {
        switch (settings.theme_style) {
            case ThemeStyle::Light:
                background = {0.95, 0.95, 0.95, 1.0};
                text = {0.10, 0.10, 0.10, 1.0};
                border = {0.75, 0.75, 0.75, 1.0};
                break;
            case ThemeStyle::Accent:
                background = {0.20, 0.30, 0.55, 1.0};
                text = {0.95, 0.95, 0.95, 1.0};
                border = {0.10, 0.15, 0.30, 1.0};
                break;
            case ThemeStyle::Dark:
            default:
                background = {0.20, 0.20, 0.20, 1.0};
                text = {0.93, 0.93, 0.93, 1.0};
                border = {0.05, 0.05, 0.05, 1.0};
                break;
        }
    }

    // Rounded rectangle helper for the tray icon.
    void drawRoundedRect(cairo_t* cr, double x, double y, double width, double height, double radius) {
        const double degrees = M_PI / 180.0;
        cairo_new_sub_path(cr);
        cairo_arc(cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
        cairo_arc(cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
        cairo_arc(cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
        cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
        cairo_close_path(cr);
    }

    // Render a full tray icon (background + text) and load it into AppIndicator.
    void renderIcon(const std::string& text) {
        cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, kIconCanvasSize, kIconCanvasSize);
        cairo_t* cr = cairo_create(surface);

        Color background;
        Color text_color;
        Color border;
        iconColors(background, text_color, border);

        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);

        const double padding = 6.0;
        const double width = kIconCanvasSize - padding * 2.0;
        const double height = kIconCanvasSize - padding * 2.0;

        // Compact and square styles intentionally share the same geometry.
        if (settings.icon_style == IconStyle::Compact) {
            cairo_rectangle(cr, padding, padding, width, height);
        } else if (settings.icon_style == IconStyle::Square) {
            cairo_rectangle(cr, padding, padding, width, height);
        } else {
            drawRoundedRect(cr, padding, padding, width, height, 10.0);
        }

        cairo_set_source_rgba(cr, background.r, background.g, background.b, background.a);
        cairo_fill_preserve(cr);
        cairo_set_line_width(cr, 2.0);
        cairo_set_source_rgba(cr, border.r, border.g, border.b, border.a);
        cairo_stroke(cr);

        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, settings.show_decimal ? 20.0 : 24.0);
        cairo_text_extents_t extents;
        cairo_text_extents(cr, text.c_str(), &extents);

        const double text_x = (kIconCanvasSize - extents.width) / 2.0 - extents.x_bearing;
        const double text_y = (kIconCanvasSize - extents.height) / 2.0 - extents.y_bearing;

        cairo_set_source_rgba(cr, text_color.r, text_color.g, text_color.b, text_color.a);
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, text.c_str());

        cairo_destroy(cr);

        if (writePng(surface, iconPath())) {
            app_indicator_set_icon_full(indicator, iconPath().c_str(), "TempMon Temperature");
        }

        cairo_surface_destroy(surface);
    }

    // Re-render the tray icon with the latest temperature.
    void updateIndicator() {
        const double temperature = computeDisplayTemperature();
        const std::string text = formatIconText(temperature);
        renderIcon(text);
    }

    // Build the dropdown menu each tick so values stay current.
    void buildMenu() {
        if (!menu) {
            return;
        }

        GList* items = gtk_container_get_children(GTK_CONTAINER(menu));
        for (GList* iter = items; iter != nullptr; iter = g_list_next(iter)) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(items);

        updateSensorValues();

        for (const auto& sensor : sensors) {
            std::string display = sensor.name + ": " + formatSensorValue(sensor);
            GtkWidget* item = gtk_menu_item_new_with_label(display.c_str());
            gtk_widget_set_sensitive(item, FALSE);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            gtk_widget_show(item);
        }

        GtkWidget* separator = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);
        gtk_widget_show(separator);

        GtkWidget* settings_item = gtk_menu_item_new_with_label("Settings");
        g_signal_connect(settings_item, "activate", G_CALLBACK(onSettings), this);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), settings_item);
        gtk_widget_show(settings_item);

        GtkWidget* quit_item = gtk_menu_item_new_with_label("Quit");
        g_signal_connect(quit_item, "activate", G_CALLBACK(onQuit), nullptr);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);
        gtk_widget_show(quit_item);
    }

    // Restart the update timer when settings change.
    void restartTimer() {
        if (timer_id > 0) {
            g_source_remove(timer_id);
        }
        timer_id = g_timeout_add(settings.update_interval_ms, updateCallback, this);
    }

    // Create the settings window with CoreTemp-like configuration knobs.
    void showSettingsWindow() {
        if (settings_window) {
            gtk_window_present(GTK_WINDOW(settings_window));
            return;
        }

        settings_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(settings_window), "TempMon Settings");
        gtk_window_set_default_size(GTK_WINDOW(settings_window), 420, 380);
        gtk_window_set_resizable(GTK_WINDOW(settings_window), FALSE);
        gtk_container_set_border_width(GTK_CONTAINER(settings_window), 12);

        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_add(GTK_CONTAINER(settings_window), vbox);

        GtkWidget* sensor_label = gtk_label_new("Temperature source:");
        gtk_label_set_xalign(GTK_LABEL(sensor_label), 0.0);
        gtk_box_pack_start(GTK_BOX(vbox), sensor_label, FALSE, FALSE, 0);

        GtkWidget* sensor_combo = gtk_combo_box_text_new();
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sensor_combo), "auto", "Auto (highest CPU temp)");
        for (const auto& sensor : sensors) {
            if (sensor.type == "temp") {
                gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(sensor_combo), sensor.path.c_str(), sensor.name.c_str());
            }
        }
        const std::string current_sensor = settings.selected_sensor_path.empty() ? "auto" : settings.selected_sensor_path;
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(sensor_combo), current_sensor.c_str());
        gtk_box_pack_start(GTK_BOX(vbox), sensor_combo, FALSE, FALSE, 0);

        GtkWidget* update_label = gtk_label_new("Update interval (seconds):");
        gtk_label_set_xalign(GTK_LABEL(update_label), 0.0);
        gtk_box_pack_start(GTK_BOX(vbox), update_label, FALSE, FALSE, 0);

        GtkWidget* update_spin = gtk_spin_button_new_with_range(0.5, 10.0, 0.5);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(update_spin), settings.update_interval_ms / 1000.0);
        gtk_box_pack_start(GTK_BOX(vbox), update_spin, FALSE, FALSE, 0);

        GtkWidget* delay_label = gtk_label_new("Display delay inside tray icon (seconds):");
        gtk_label_set_xalign(GTK_LABEL(delay_label), 0.0);
        gtk_box_pack_start(GTK_BOX(vbox), delay_label, FALSE, FALSE, 0);

        GtkWidget* delay_spin = gtk_spin_button_new_with_range(0.0, kMaxDelaySeconds, 1.0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(delay_spin), settings.display_delay_ms / 1000.0);
        gtk_box_pack_start(GTK_BOX(vbox), delay_spin, FALSE, FALSE, 0);

        GtkWidget* icon_label = gtk_label_new("Icon style:");
        gtk_label_set_xalign(GTK_LABEL(icon_label), 0.0);
        gtk_box_pack_start(GTK_BOX(vbox), icon_label, FALSE, FALSE, 0);

        GtkWidget* icon_combo = gtk_combo_box_text_new();
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(icon_combo), "0", "Compact");
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(icon_combo), "1", "Rounded");
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(icon_combo), "2", "Square");
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(icon_combo), std::to_string(static_cast<int>(settings.icon_style)).c_str());
        gtk_box_pack_start(GTK_BOX(vbox), icon_combo, FALSE, FALSE, 0);

        GtkWidget* theme_label = gtk_label_new("Color theme:");
        gtk_label_set_xalign(GTK_LABEL(theme_label), 0.0);
        gtk_box_pack_start(GTK_BOX(vbox), theme_label, FALSE, FALSE, 0);

        GtkWidget* theme_combo = gtk_combo_box_text_new();
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), "0", "Dark");
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), "1", "Light");
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), "2", "Accent");
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(theme_combo), std::to_string(static_cast<int>(settings.theme_style)).c_str());
        gtk_box_pack_start(GTK_BOX(vbox), theme_combo, FALSE, FALSE, 0);

        GtkWidget* unit_check = gtk_check_button_new_with_label("Show ° unit in tray icon");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(unit_check), settings.show_unit);
        gtk_box_pack_start(GTK_BOX(vbox), unit_check, FALSE, FALSE, 0);

        GtkWidget* decimal_check = gtk_check_button_new_with_label("Show decimal precision in tray icon");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(decimal_check), settings.show_decimal);
        gtk_box_pack_start(GTK_BOX(vbox), decimal_check, FALSE, FALSE, 0);

        GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_pack_end(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

        GtkWidget* save_button = gtk_button_new_with_label("Save");
        GtkWidget* cancel_button = gtk_button_new_with_label("Cancel");
        gtk_box_pack_end(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(button_box), save_button, FALSE, FALSE, 0);

        // Save callback preserves settings, restarts timer, and updates icon.
        auto on_save = [](GtkButton*, gpointer data) {
            auto* self = static_cast<TempMonitor*>(data);
            self->applySettingsFromWindow();
        };

        g_signal_connect(save_button, "clicked", G_CALLBACK(on_save), this);
        g_signal_connect(cancel_button, "clicked", G_CALLBACK(onCloseSettings), this);
        g_signal_connect(settings_window, "delete-event", G_CALLBACK(onDeleteSettings), this);

        g_object_set_data(G_OBJECT(settings_window), "sensor_combo", sensor_combo);
        g_object_set_data(G_OBJECT(settings_window), "update_spin", update_spin);
        g_object_set_data(G_OBJECT(settings_window), "delay_spin", delay_spin);
        g_object_set_data(G_OBJECT(settings_window), "icon_combo", icon_combo);
        g_object_set_data(G_OBJECT(settings_window), "theme_combo", theme_combo);
        g_object_set_data(G_OBJECT(settings_window), "unit_check", unit_check);
        g_object_set_data(G_OBJECT(settings_window), "decimal_check", decimal_check);

        gtk_widget_show_all(settings_window);
    }

    // Apply settings from widgets to config, and refresh visuals.
    void applySettingsFromWindow() {
        if (!settings_window) {
            return;
        }

        auto* sensor_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(settings_window), "sensor_combo"));
        auto* update_spin = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(settings_window), "update_spin"));
        auto* delay_spin = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(settings_window), "delay_spin"));
        auto* icon_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(settings_window), "icon_combo"));
        auto* theme_combo = GTK_COMBO_BOX(g_object_get_data(G_OBJECT(settings_window), "theme_combo"));
        auto* unit_check = GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(settings_window), "unit_check"));
        auto* decimal_check = GTK_TOGGLE_BUTTON(g_object_get_data(G_OBJECT(settings_window), "decimal_check"));

        const gchar* sensor_id = gtk_combo_box_get_active_id(sensor_combo);
        if (!sensor_id || std::string(sensor_id) == "auto") {
            settings.selected_sensor_path.clear();
        } else {
            settings.selected_sensor_path = sensor_id;
        }

        settings.update_interval_ms = std::clamp(static_cast<int>(gtk_spin_button_get_value(update_spin) * 1000), kMinUpdateMs, kMaxUpdateMs);
        settings.display_delay_ms = std::clamp(static_cast<int>(gtk_spin_button_get_value(delay_spin) * 1000), 0, kMaxDelaySeconds * 1000);
        const gchar* icon_id = gtk_combo_box_get_active_id(icon_combo);
        if (!icon_id) {
            logError("applySettingsFromWindow", "Icon style selection missing. Keeping previous value.");
        } else {
            settings.icon_style = static_cast<IconStyle>(std::stoi(icon_id));
        }

        const gchar* theme_id = gtk_combo_box_get_active_id(theme_combo);
        if (!theme_id) {
            logError("applySettingsFromWindow", "Theme selection missing. Keeping previous value.");
        } else {
            settings.theme_style = static_cast<ThemeStyle>(std::stoi(theme_id));
        }
        settings.show_unit = gtk_toggle_button_get_active(unit_check);
        settings.show_decimal = gtk_toggle_button_get_active(decimal_check);

        saveSettings();
        restartTimer();
        updateSensorValues();
        updateIndicator();

        gtk_widget_destroy(settings_window);
        settings_window = nullptr;
    }

    static void onSettings(GtkMenuItem*, gpointer data) {
        auto* self = static_cast<TempMonitor*>(data);
        self->showSettingsWindow();
    }

    static void onQuit(GtkMenuItem*, gpointer) {
        gtk_main_quit();
    }

    static void onCloseSettings(GtkButton*, gpointer data) {
        auto* self = static_cast<TempMonitor*>(data);
        if (self->settings_window) {
            gtk_widget_destroy(self->settings_window);
            self->settings_window = nullptr;
        }
    }

    static gboolean onDeleteSettings(GtkWidget*, GdkEvent*, gpointer data) {
        auto* self = static_cast<TempMonitor*>(data);
        if (self->settings_window) {
            gtk_widget_destroy(self->settings_window);
            self->settings_window = nullptr;
        }
        return TRUE;
    }

    static gboolean updateCallback(gpointer data) {
        auto* monitor = static_cast<TempMonitor*>(data);
        monitor->updateSensorValues();
        monitor->updateIndicator();
        monitor->buildMenu();
        return TRUE;
    }
};

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    TempMonitor monitor;
    monitor.run();

    return 0;
}
