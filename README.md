<div align="center">

# 🌡️ TempMon

### Lightweight System Temperature Monitor for Arch Linux

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux-green.svg)](https://www.kernel.org/)
[![AUR: Arch Linux](https://img.shields.io/badge/AUR-Arch%20Linux-1793D1.svg)](https://archlinux.org/)
[![Build: Make](https://img.shields.io/badge/Build-Make-orange.svg)](https://www.gnu.org/software/make/)

**A native, lightweight system tray application for monitoring CPU/GPU temperatures, power consumption, and fan speeds.**

*Similar to CoreTemp or iStat Menus, but built specifically for Linux with minimal resource usage.*

[Features](#-features) • [Installation](#-installation) • [Usage](#-usage) • [Project Structure](#-project-structure) • [GitHub Setup](#-github-setup) • [Troubleshooting](#-troubleshooting)

---

</div>

## ✨ Features

<table>
<tr>
<td width="50%">

### 📊 Monitoring Capabilities
- 🌡️ **Temperature Monitoring**
  - CPU core temperatures
  - GPU temperatures
  - Motherboard sensors
  - Disk/NVMe sensors (when exposed via hwmon)
  - Other hardware sensors
- ⚡ **Power Consumption**
  - CPU package power (watts)
  - GPU power draw
- 🌀 **Fan Speed Monitoring**
  - All system fans (RPM)
- 📈 **System Stats**
  - CPU frequency (MHz)
  - Network throughput (Rx/Tx)

</td>
<td width="50%">

### 🚀 Performance & Features
- 🪶 **Lightweight**
  - ~5MB RAM usage
  - Minimal CPU overhead
- 🎯 **Native Implementation**
  - Pure C++17
  - GTK3 + libappindicator
- 📊 **Auto-Discovery**
  - Automatically finds all sensors
  - No manual configuration needed
- 🔄 **Real-time Updates**
  - 2-second refresh rate
  - Live system tray display
- ⚙️ **CoreTemp-like Tray Icon**
  - Multiple icon styles (compact, rounded, square)
  - Optional temperature delay for the tray icon
  - Settings window for quick customization
- 🚨 **Alerts & History**
  - Configurable temperature alerts
  - Built-in history graph window

</td>
</tr>
</table>

---

## 📸 Screenshots

<div align="center">

| System Tray | Dropdown Menu |
|:-----------:|:-------------:|
| ![Tray Icon showing 52°C](https://via.placeholder.com/200x40/2e3440/88c0d0?text=52%C2%B0C) | ![Menu with sensors](https://via.placeholder.com/300x200/2e3440/88c0d0?text=CPU%3A+52%C2%B0C%0AGPU%3A+45%C2%B0C%0APower%3A+15W) |

*The app displays current CPU temperature in the tray and all sensors in a dropdown menu*

</div>

---

## 📋 Requirements

### 📦 System Requirements

| Component | Requirement |
|-----------|-------------|
| **OS** | Arch Linux (or Arch-based distro) |
| **Compiler** | GCC with C++17 support |
| **Libraries** | GTK3, libappindicator-gtk3 |
| **Build Tools** | make, pkg-config |
| **Kernel** | Linux kernel with hwmon support |

### 🔧 Dependencies
```bash
sudo pacman -S base-devel gtk3 libappindicator-gtk3
```

<details>
<summary><b>Optional: GPU-specific drivers</b></summary>

#### For NVIDIA GPUs
```bash
sudo pacman -S nvidia-utils
```

#### For AMD GPUs
The `amdgpu` kernel driver should already expose temperature sensors.

#### For Better Sensor Detection
```bash
sudo pacman -S lm_sensors
sudo sensors-detect
```

</details>

---

## 🚀 Installation

### Method 1: Quick Install Script (Recommended)
```bash
git clone https://github.com/yourusername/tempmon.git
cd tempmon
chmod +x install.sh
./install.sh
```

The script will:
- ✅ Check and install dependencies
- ✅ Compile the application
- ✅ Optionally install to `/usr/local/bin`
- ✅ Optionally add to autostart
- ✅ Check for sensor kernel modules

### Method 2: Manual Installation
```bash
sudo pacman -S base-devel gtk3 libappindicator-gtk3
make
sudo make install
tempmon
```

### Method 3: Build Arch Package
```bash
makepkg -si
```

---

## 🎮 Usage

### Starting TempMon
```bash
tempmon
```

The application will:
1. 🔍 Auto-discover all available hardware sensors
2. 📊 Display the highest CPU temperature in the system tray
3. 📋 Show all sensors in a dropdown menu when clicked
4. 🔄 Update readings every 2 seconds
5. ⚙️ Provide a Settings window for icon style, delay, sensor selection, and alerts
6. 📈 Display CPU frequency + network throughput in the menu
7. 🧾 Offer a temperature history graph window

### Stopping TempMon

- Click the tray icon and select **Quit**
- Or use: `killall tempmon`

---

## ⚙️ Autostart Configuration

### systemd User Service (Recommended)
```bash
# Install systemd user unit and enable autostart
sudo make install
systemctl --user daemon-reload
systemctl --user enable --now tempmon.service
```

Disable it later with:
```bash
systemctl --user disable --now tempmon.service
```

### For Desktop Environments (GNOME, KDE, Xfce, MATE)
```bash
mkdir -p ~/.config/autostart
cp tempmon.desktop ~/.config/autostart/
```

Or create the file manually:
```bash
cat > ~/.config/autostart/tempmon.desktop << 'EOF'
[Desktop Entry]
Type=Application
Name=TempMon
Comment=Temperature Monitor
Exec=tempmon
Terminal=false
X-GNOME-Autostart-enabled=true
EOF
```

### For Window Managers (i3, Sway, bspwm)

#### i3 WM
Add to `~/.config/i3/config`:
```bash
exec --no-startup-id tempmon
```

#### Sway
Add to `~/.config/sway/config`:
```bash
exec tempmon
```

#### bspwm
Add to `~/.config/bspwm/bspwmrc`:
```bash
tempmon &
```

---

## 🔧 How It Works

<div align="center">
```
┌─────────────────────────────────────────────────────────────┐
│          Linux Kernel (hwmon interface)                     │
│                /sys/class/hwmon/hwmon*                       │
└──────────────────────────┬──────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
   temp*_input        power*_input       fan*_input
        │                  │                  │
   Temperature (°C)    Power (Watts)    Fan Speed (RPM)
        │                  │                  │
        └──────────────────┼──────────────────┘
                           │
                           ▼
        ┌──────────────────────────────────────────┐
        │       TempMon Application                │
        │  • Auto-discovers sensors                │
        │  • Reads values every 2 seconds          │
        │  • Updates GTK tray indicator            │
        └──────────────────────────────────────────┘
```

</div>

### 📊 Supported Sensor Types

| Sensor Type | Interface | Example Hardware |
|-------------|-----------|------------------|
| 🌡️ **Temperature** | `hwmon/temp*_input` | CPU cores, GPU, chipset |
| ⚡ **Power** | `hwmon/power*_input` | CPU package, GPU power |
| 🌀 **Fan Speed** | `hwmon/fan*_input` | Case fans, CPU cooler |

### 🖥️ Compatible Hardware

<table>
<tr>
<td width="50%">

#### Intel Systems
- **CPU**: `coretemp` driver
  - Core temperatures
  - Package power
- **GPU**: `i915` driver (integrated)

</td>
<td width="50%">

#### AMD Systems
- **CPU**: `k10temp` driver
  - Tctl/Tdie temperatures
  - Package power
- **GPU**: `amdgpu` driver
  - Junction/edge temps
  - GPU power

</td>
</tr>
<tr>
<td colspan="2">

#### NVIDIA GPUs
- **GPU**: `nvidia` driver
  - GPU temperature
  - Power draw
  - Fan speed

</td>
</tr>
</table>

---

## 📁 Project Structure
```
tempmon/
├── 📄 .gitignore             # Git ignore rules
├── 📄 LICENSE                # MIT License
├── 📄 README.md              # This file
├──src/                       
   └── 💻 tempmon.cpp         # Main C++ source (~300 lines)
├── 🔨 Makefile               # Build configuration
├── 🚀 install.sh             # Automated installer
├── 🖥️ tempmon.desktop        # Desktop entry for autostart
├── 🧰 tempmon.service        # systemd user service (autostart)
├── 📦 PKGBUILD               # Arch package build script
├── 📚 ENHANCEMENTS.md        # Customization guide
└── 🖼️ images/                # Screenshots (optional)
    ├── how-it-works.png
    ├── tray-icon.png
    └── dropdown-menu.png
```

### What Files Do I Need?

**Quick Start (Just Use It):**
```
tempmon.cpp + Makefile + install.sh
```

**Manual Installation:**
```
tempmon.cpp + Makefile
```

**Build Arch Package:**
```
tempmon.cpp + Makefile + tempmon.desktop + PKGBUILD
```

**Total repository size: ~50-300 KB** (extremely lightweight!)

---

## 🐙 GitHub Setup

### Initial Repository Setup
```bash
cd tempmon
git init
git add .
git commit -m "Initial commit: TempMon system monitor"
git remote add origin https://github.com/yourusername/tempmon.git
git branch -M main
git push -u origin main
```

### Repository Configuration

**Topics (Tags):**
```
arch-linux, temperature-monitor, system-tray, gtk3, cpp, linux, hardware-monitoring, sensors, lightweight
```

**Description:**
```
Lightweight system tray monitor for CPU/GPU temps, power, and fans
```

### Creating First Release
```bash
git tag -a v1.0.0 -m "Initial release of TempMon"
git push origin v1.0.0
```

On GitHub: **Releases** → **Create a new release** → Tag: `v1.0.0`

### .gitignore File
```gitignore
# Compiled binary
tempmon

# Build artifacts
*.o
*.so
*.a
*.out

# Editor files
*.swp
*.swo
*~
.vscode/
.idea/
*.sublime-*

# OS files
.DS_Store
Thumbs.db
desktop.ini

# Backup files
*.bak
*.backup

# Package files
*.tar.gz
*.tar.xz
*.pkg.tar.zst
pkg/
src/

# Debug files
*.log
core

# IDE specific
.clangd/
compile_commands.json
```

### Optional: GitHub Actions CI/CD

Create `.github/workflows/build.yml`:
```yaml
name: Build

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential libgtk-3-dev libappindicator3-dev
    
    - name: Build
      run: make
    
    - name: Upload binary
      uses: actions/upload-artifact@v3
      with:
        name: tempmon
        path: tempmon
```

---

## 🎨 Customization

### Changing Update Interval

Edit `tempmon.cpp` line ~270:
```cpp
// Change from 2000ms (2 seconds) to desired value
timer_id = g_timeout_add(2000, updateCallback, this);
```

### Settings Window (Recommended)
Open **Settings** from the tray menu to configure:
- Temperature source (auto or a specific sensor)
- Update interval and icon display delay
- Icon style (compact, rounded, square)
- Theme (dark, light, accent)
- Decimal precision and degree symbol
- Temperature alerts (threshold + cooldown)

Settings are stored in `~/.config/tempmon/config.ini`.

### Changing Tray Display

Edit the `computeAutoTemperature()` function to show GPU temp instead:
```cpp
// Search for GPU temperature instead of CPU
if (sensor.type == "temp" && sensor.name.find("GPU") != std::string::npos) {
    // Show GPU temp in tray
}
```

### Adding Temperature Alerts

See [ENHANCEMENTS.md](ENHANCEMENTS.md) for examples of adding:
- 🔔 Desktop notifications for high temps
- 🎨 Color-coded temperature warnings
- 📈 Temperature history and graphs
- ⚙️ Configuration file support

---

## 🐛 Troubleshooting

### ❌ No sensors detected

**Solution 1: Load kernel modules**
```bash
# For Intel CPUs
sudo modprobe coretemp

# For AMD CPUs  
sudo modprobe k10temp

# For AMD GPUs
sudo modprobe amdgpu

# Make permanent
echo "coretemp" | sudo tee -a /etc/modules-load.d/sensors.conf
```

**Solution 2: Run sensors-detect**
```bash
sudo pacman -S lm_sensors
sudo sensors-detect
# Answer YES to save configuration
sudo reboot
```

**Solution 3: Verify sensors exist**
```bash
# List available hardware monitors
ls -la /sys/class/hwmon/

# Check what sensors are detected
sensors

# Check specific device
cat /sys/class/hwmon/hwmon0/name
cat /sys/class/hwmon/hwmon0/temp1_input
```

### 🚫 App doesn't appear in system tray

**GNOME:**
```bash
sudo pacman -S gnome-shell-extension-appindicator
gnome-extensions enable appindicatorsupport@rgcjonas.gmail.com
```

**KDE Plasma:** Should work out of the box. Check System Settings → System Tray.

**i3/Sway:** Make sure you're running a status bar that supports tray icons (i3bar, polybar, waybar).

**Xfce:** Panel should support indicators by default. Right-click panel → Add Items → Notification Area.

### ⚠️ Build errors

**Error: `gtk/gtk.h: No such file or directory`**
```bash
sudo pacman -S gtk3 libappindicator-gtk3 pkg-config
```

**Error: `undefined reference to 'app_indicator_new'`**
```bash
sudo pacman -S libappindicator-gtk3
```

**Check pkg-config:**
```bash
pkg-config --modversion gtk+-3.0
pkg-config --modversion appindicator3-0.1
```

### 🔥 Temperatures seem wrong

**Compare with other tools:**
```bash
sudo pacman -S lm_sensors
sensors
watch -n 1 sensors

# For NVIDIA
nvidia-smi
```

**Note about AMD Tctl vs Tdie:**

AMD reports two temperatures:
- **Tctl**: Control temperature (with offset)
- **Tdie**: Die temperature (actual)

TempMon shows both if available.

---

## 🗑️ Uninstallation
```bash
# Remove binary
sudo make uninstall

# Remove autostart entry
rm -f ~/.config/autostart/tempmon.desktop

# Remove source directory
cd .. && rm -rf tempmon
```

---

## 🤝 Contributing

Contributions are welcome! Here are some ideas:

<table>
<tr>
<td>

### 🎯 Feature Ideas
- [ ] Configuration file support
- [ ] Temperature alert notifications  
- [ ] Historical graphs
- [ ] Disk temperature monitoring
- [ ] Network statistics
- [ ] CPU frequency display

</td>
<td>

### 🐛 Improvements
- [ ] Multi-language support
- [ ] GUI preferences window
- [ ] Custom color themes
- [ ] Export data to CSV
- [ ] Wayland native support
- [ ] Plugin system

</td>
</tr>
</table>

### How to Contribute

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 📊 Comparison with Other Tools

| Feature | TempMon | conky | psensor | i3status | htop |
|---------|---------|-------|---------|----------|------|
| **System Tray** | ✅ | ❌ | ✅ | ❌ | ❌ |
| **Lightweight** | ✅ | ⚠️ | ⚠️ | ✅ | ✅ |
| **Auto-discovery** | ✅ | ❌ | ✅ | ❌ | N/A |
| **Power Monitoring** | ✅ | ❌ | ❌ | ❌ | ❌ |
| **No Config Needed** | ✅ | ❌ | ✅ | ❌ | ✅ |
| **RAM Usage** | ~5MB | ~20MB | ~15MB | ~2MB | ~10MB |

### Why TempMon?

- ✅ **vs CoreTemp**: Native Linux, monitors all hardware types
- ✅ **vs iStat Menus**: Open source, free, customizable
- ✅ **vs conky**: System tray integration, zero configuration
- ✅ **vs psensor**: Lighter weight, cleaner interface, includes power monitoring

---

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

Copyright (c) 2025 [Your Name]

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.

---

## 📞 Support

- 🐛 **Bug Reports**: [Open an issue](https://github.com/yourusername/tempmon/issues)
- 💡 **Feature Requests**: [Start a discussion](https://github.com/yourusername/tempmon/discussions)  
- 📖 **Documentation**: Check [ENHANCEMENTS.md](ENHANCEMENTS.md) for customization examples

---

## 🙏 Acknowledgments

- **GTK Project** - UI toolkit
- **libappindicator** - System tray support
- **Linux kernel** - hwmon interface
- **Arch Linux** - The inspiration for lightweight tools

---

<div align="center">

**[⬆ Back to Top](#-tempmon)**

Made with ❤️ for the Arch Linux community

![Arch Linux](https://img.shields.io/badge/Arch%20Linux-1793D1?style=for-the-badge&logo=arch-linux&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)

</div>
