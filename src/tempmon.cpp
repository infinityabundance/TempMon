#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cmath>

namespace fs = std::filesystem;

struct SensorInfo {
    std::string name;
    std::string path;
    std::string type; // "temp", "power", "fan"
    double value;
    double max;
};

class TempMonitor {
private:
    AppIndicator* indicator;
    GtkWidget* menu;
    std::vector<SensorInfo> sensors;
    guint timer_id;

    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::string content;
        std::getline(file, content);
        return content;
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, last - first + 1);
    }

    void discoverSensors() {
        sensors.clear();
        
        // Scan /sys/class/hwmon for hardware sensors
        std::string hwmon_path = "/sys/class/hwmon";
        
        if (!fs::exists(hwmon_path)) return;

        for (const auto& entry : fs::directory_iterator(hwmon_path)) {
            std::string hwmon_dir = entry.path().string();
            std::string device_name = readFile(hwmon_dir + "/name");
            device_name = trim(device_name);

            // Scan for temperature sensors
            for (int i = 1; i <= 20; i++) {
                std::string temp_input = hwmon_dir + "/temp" + std::to_string(i) + "_input";
                std::string temp_label = hwmon_dir + "/temp" + std::to_string(i) + "_label";
                std::string temp_max = hwmon_dir + "/temp" + std::to_string(i) + "_max";
                
                if (fs::exists(temp_input)) {
                    SensorInfo sensor;
                    sensor.path = temp_input;
                    sensor.type = "temp";
                    
                    std::string label = readFile(temp_label);
                    if (!label.empty()) {
                        sensor.name = device_name + " - " + trim(label);
                    } else {
                        sensor.name = device_name + " - Temp" + std::to_string(i);
                    }
                    
                    std::string max_str = readFile(temp_max);
                    sensor.max = max_str.empty() ? 100.0 : std::stod(max_str) / 1000.0;
                    
                    sensors.push_back(sensor);
                }
            }

            // Scan for power sensors (wattage)
            for (int i = 1; i <= 10; i++) {
                std::string power_input = hwmon_dir + "/power" + std::to_string(i) + "_input";
                std::string power_label = hwmon_dir + "/power" + std::to_string(i) + "_label";
                
                if (fs::exists(power_input)) {
                    SensorInfo sensor;
                    sensor.path = power_input;
                    sensor.type = "power";
                    
                    std::string label = readFile(power_label);
                    if (!label.empty()) {
                        sensor.name = device_name + " - " + trim(label);
                    } else {
                        sensor.name = device_name + " - Power" + std::to_string(i);
                    }
                    
                    sensor.max = 0;
                    sensors.push_back(sensor);
                }
            }

            // Scan for fan sensors
            for (int i = 1; i <= 10; i++) {
                std::string fan_input = hwmon_dir + "/fan" + std::to_string(i) + "_input";
                std::string fan_label = hwmon_dir + "/fan" + std::to_string(i) + "_label";
                
                if (fs::exists(fan_input)) {
                    SensorInfo sensor;
                    sensor.path = fan_input;
                    sensor.type = "fan";
                    
                    std::string label = readFile(fan_label);
                    if (!label.empty()) {
                        sensor.name = device_name + " - " + trim(label);
                    } else {
                        sensor.name = device_name + " - Fan" + std::to_string(i);
                    }
                    
                    sensor.max = 0;
                    sensors.push_back(sensor);
                }
            }
        }
    }

    void updateSensorValues() {
        for (auto& sensor : sensors) {
            std::string value_str = readFile(sensor.path);
            if (!value_str.empty()) {
                double raw_value = std::stod(value_str);
                
                if (sensor.type == "temp") {
                    sensor.value = raw_value / 1000.0; // Convert from millidegrees
                } else if (sensor.type == "power") {
                    sensor.value = raw_value / 1000000.0; // Convert from microwatts to watts
                } else if (sensor.type == "fan") {
                    sensor.value = raw_value; // RPM
                }
            }
        }
    }

    std::string formatSensorValue(const SensorInfo& sensor) {
        std::ostringstream oss;
        oss.precision(1);
        oss << std::fixed;
        
        if (sensor.type == "temp") {
            oss << sensor.value << "°C";
        } else if (sensor.type == "power") {
            oss << sensor.value << "W";
        } else if (sensor.type == "fan") {
            oss << static_cast<int>(sensor.value) << " RPM";
        }
        
        return oss.str();
    }

    void updateMenu() {
        // Clear existing menu items (except quit)
        GList* items = gtk_container_get_children(GTK_CONTAINER(menu));
        for (GList* iter = items; iter != NULL; iter = g_list_next(iter)) {
            GtkWidget* widget = GTK_WIDGET(iter->data);
            const gchar* label = gtk_menu_item_get_label(GTK_MENU_ITEM(widget));
            if (label && std::string(label) != "Quit") {
                gtk_widget_destroy(widget);
            }
        }
        g_list_free(items);

        updateSensorValues();

        // Add sensor readings
        for (const auto& sensor : sensors) {
            std::string display = sensor.name + ": " + formatSensorValue(sensor);
            GtkWidget* item = gtk_menu_item_new_with_label(display.c_str());
            gtk_widget_set_sensitive(item, FALSE); // Make it non-clickable
            gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
            gtk_widget_show(item);
        }

        // Add separator
        GtkWidget* separator = gtk_separator_menu_item_new();
        gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), separator);
        gtk_widget_show(separator);

        // Update indicator label with highest CPU temp
        updateIndicatorLabel();
    }

    void updateIndicatorLabel() {
        double max_cpu_temp = 0.0;
        
        for (const auto& sensor : sensors) {
            if (sensor.type == "temp" && 
                (sensor.name.find("CPU") != std::string::npos || 
                 sensor.name.find("Core") != std::string::npos ||
                 sensor.name.find("Tctl") != std::string::npos)) {
                max_cpu_temp = std::max(max_cpu_temp, sensor.value);
            }
        }

        if (max_cpu_temp > 0) {
            std::ostringstream oss;
            oss.precision(0);
            oss << std::fixed << max_cpu_temp << "°C";
            app_indicator_set_label(indicator, oss.str().c_str(), "100°C");
        }
    }

    static gboolean updateCallback(gpointer data) {
        TempMonitor* monitor = static_cast<TempMonitor*>(data);
        monitor->updateMenu();
        return TRUE; // Continue timer
    }

    static void onQuit(GtkMenuItem* item, gpointer data) {
        gtk_main_quit();
    }

public:
    TempMonitor() {
        // Create indicator
        indicator = app_indicator_new(
            "tempmon",
            "temperature-symbolic",
            APP_INDICATOR_CATEGORY_HARDWARE
        );
        
        app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
        app_indicator_set_label(indicator, "--", "100°C");

        // Create menu
        menu = gtk_menu_new();

        // Initial sensor discovery
        discoverSensors();
        updateMenu();

        // Add quit menu item
        GtkWidget* quit_item = gtk_menu_item_new_with_label("Quit");
        g_signal_connect(quit_item, "activate", G_CALLBACK(onQuit), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);
        gtk_widget_show(quit_item);

        app_indicator_set_menu(indicator, GTK_MENU(menu));

        // Set up timer to update every 2 seconds
        timer_id = g_timeout_add(2000, updateCallback, this);
    }

    ~TempMonitor() {
        if (timer_id > 0) {
            g_source_remove(timer_id);
        }
    }

    void run() {
        gtk_main();
    }
};

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    TempMonitor monitor;
    monitor.run();

    return 0;
}
