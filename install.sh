#!/bin/bash

set -e

echo "======================================"
echo "TempMon Installation Script"
echo "======================================"
echo

# Check if running on Arch Linux
if [ ! -f /etc/arch-release ]; then
    echo "Warning: This script is designed for Arch Linux."
    echo "You may need to adjust package names for other distributions."
    echo
fi

# Check for required dependencies
echo "Checking dependencies..."

MISSING_DEPS=()

if ! pacman -Qi gtk3 &> /dev/null; then
    MISSING_DEPS+=("gtk3")
fi

if ! pacman -Qi libappindicator-gtk3 &> /dev/null; then
    MISSING_DEPS+=("libappindicator-gtk3")
fi

if ! pacman -Qi base-devel &> /dev/null; then
    MISSING_DEPS+=("base-devel")
fi

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo "Missing dependencies: ${MISSING_DEPS[*]}"
    echo
    read -p "Install missing dependencies? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        sudo pacman -S --needed "${MISSING_DEPS[@]}"
    else
        echo "Cannot proceed without dependencies."
        exit 1
    fi
fi

echo "✓ All dependencies satisfied"
echo

# Compile
echo "Compiling TempMon..."
make clean 2>/dev/null || true
make

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful"
else
    echo "✗ Compilation failed"
    exit 1
fi

echo

# Ask about installation
read -p "Install to /usr/local/bin? (requires sudo) (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo make install
    echo "✓ Installed to /usr/local/bin/tempmon"
    INSTALLED_PATH="/usr/local/bin/tempmon"
else
    INSTALLED_PATH="./tempmon"
    echo "Skipping system installation. You can run ./tempmon from this directory."
fi

echo

# Ask about systemd user service
if [ "$INSTALLED_PATH" = "/usr/local/bin/tempmon" ]; then
    read -p "Enable systemd user service for autostart? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        systemctl --user daemon-reload
        systemctl --user enable --now tempmon.service
        echo "✓ Enabled systemd user service (tempmon.service)"
    fi
else
    echo "Note: systemd user service requires an installed binary in /usr/local/bin."
    echo "      You can re-run the installer and choose to install the binary."
fi

echo

# Ask about autostart
read -p "Add to autostart? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    mkdir -p ~/.config/autostart
    
    if [ "$INSTALLED_PATH" = "/usr/local/bin/tempmon" ]; then
        cp tempmon.desktop ~/.config/autostart/
    else
        # Modify desktop file with full path
        sed "s|Exec=tempmon|Exec=$PWD/tempmon|" tempmon.desktop > ~/.config/autostart/tempmon.desktop
    fi
    
    echo "✓ Added to autostart (~/.config/autostart/tempmon.desktop)"
fi

echo

# Check for sensor modules
echo "Checking for sensor kernel modules..."

MODULES_LOADED=0

if lsmod | grep -q coretemp; then
    echo "✓ coretemp module loaded (Intel CPU)"
    MODULES_LOADED=1
fi

if lsmod | grep -q k10temp; then
    echo "✓ k10temp module loaded (AMD CPU)"
    MODULES_LOADED=1
fi

if lsmod | grep -q amdgpu; then
    echo "✓ amdgpu module loaded (AMD GPU)"
    MODULES_LOADED=1
fi

if lsmod | grep -q nvidia; then
    echo "✓ nvidia module loaded (NVIDIA GPU)"
    MODULES_LOADED=1
fi

if [ $MODULES_LOADED -eq 0 ]; then
    echo "⚠ No common sensor modules detected."
    echo "  You may need to run 'sudo sensors-detect' and reboot."
    echo "  Or manually load modules with 'sudo modprobe <module>'"
fi

echo
echo "======================================"
echo "Installation complete!"
echo "======================================"
echo
echo "To run TempMon:"
if [ "$INSTALLED_PATH" = "/usr/local/bin/tempmon" ]; then
    echo "  tempmon"
else
    echo "  ./tempmon"
fi
echo
echo "To test sensors manually:"
echo "  sensors"
echo
echo "If you don't see sensors, try:"
echo "  sudo pacman -S lm_sensors"
echo "  sudo sensors-detect"
echo
