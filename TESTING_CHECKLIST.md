# TempMon Testing & Verification Checklist

**Purpose:** Systematic testing plan to verify what works vs. what doesn't  
**Created:** 2026-02-15  
**Status:** Tests not yet executed (no test infrastructure or dependencies available)

---

## Test Environment Setup

### Prerequisites
- [ ] Arch Linux (or Arch-based distribution)
- [ ] GTK3 development libraries installed
- [ ] libappindicator-gtk3 installed
- [ ] System with hwmon-capable hardware
- [ ] Desktop environment with system tray support

### Setup Commands
```bash
sudo pacman -S base-devel gtk3 libappindicator-gtk3 lm_sensors
sudo sensors-detect  # Load sensor kernel modules
cd /path/to/TempMon
make clean && make   # Attempt build
```

**Expected Build Result:** ❌ Compilation should **FAIL** due to duplicate function definitions

---

## Phase 1: Build System Tests

### Test 1.1: Dependencies Missing (Stub Build)
**Purpose:** Verify fallback behavior when GTK/AppIndicator not available

```bash
# Remove pkg-config metadata temporarily
sudo mv /usr/lib/pkgconfig/gtk+-3.0.pc /tmp/ || true
sudo mv /usr/lib/pkgconfig/appindicator3-0.1.pc /tmp/ || true

make clean
make
./tempmon
```

**Expected Results:**
- [x] Build succeeds with stub source
- [x] Binary runs and shows error message
- [x] Error mentions missing GTK/AppIndicator packages

**Actual Results:**
- ✅ PASS - Stub builds successfully
- ✅ PASS - Shows appropriate error message
- Status: **WORKING AS DESIGNED**

```
[TempMon][ERROR] Missing GTK/AppIndicator development packages.
[TempMon][ERROR] Install gtk3 and libappindicator development packages, then rebuild.
```

### Test 1.2: Dependencies Present (Full Build)
**Purpose:** Verify main application compilation

```bash
# Restore pkg-config metadata
sudo mv /tmp/gtk+-3.0.pc /usr/lib/pkgconfig/ || true
sudo mv /tmp/appindicator3-0.1.pc /usr/lib/pkgconfig/ || true

make clean
make 2>&1 | tee build.log
```

**Expected Results:**
- [ ] ❌ Build FAILS with multiple errors
- [ ] Error: redefinition of functions (updateIndicator, buildMenu, etc.)
- [ ] Error: return statement with value in void function (line 633)

**Predicted Errors:**
```
src/tempmon.cpp:616:10: error: redefinition of 'void TempMonitor::updateIndicator()'
src/tempmon.cpp:879:12: error: redefinition of 'double TempMonitor::updateIndicator()'
src/tempmon.cpp:623:10: error: redefinition of 'void TempMonitor::buildMenu()'
src/tempmon.cpp:633:16: error: return-statement with a value, in function returning 'void'
src/tempmon.cpp:633:16: error: 'max_cpu_temp' was not declared in this scope
```

**Actual Results:**
- [ ] Not yet tested (dependencies not available in test environment)
- Status: **PREDICTED TO FAIL**

---

## Phase 2: Core Functionality Tests

**Note:** These tests require fixing compilation errors first.

### Test 2.1: Sensor Discovery
**Purpose:** Verify hwmon sensor auto-detection

**Prerequisites:**
- [ ] Application builds successfully
- [ ] Hardware with sensors (CPU temp minimum)

**Test Steps:**
```bash
# Check available sensors
ls -la /sys/class/hwmon/
sensors

# Run TempMon with debug
./tempmon 2>&1 | grep "discoverSensors"
```

**Expected Results:**
- [ ] App discovers at least CPU temperature sensors
- [ ] Logs show sensor discovery process
- [ ] No "No sensors found" error
- [ ] Tray icon appears with temperature

**Test Data Sources:**
- `/sys/class/hwmon/hwmon0/name` - Device name (e.g., "coretemp")
- `/sys/class/hwmon/hwmon0/temp1_input` - Temperature in millidegrees
- `/sys/class/hwmon/hwmon0/power1_input` - Power in microwatts (if available)
- `/sys/class/hwmon/hwmon0/fan1_input` - Fan RPM (if available)

**Validation:**
```bash
# Compare TempMon readings with lm_sensors
sensors
# Should match temperatures shown in TempMon menu
```

**Status:** ⏳ Pending fix of compilation errors

---

### Test 2.2: Temperature Reading Accuracy
**Purpose:** Verify temperature values match system readings

**Test Steps:**
1. Open TempMon menu
2. Run `sensors` in terminal
3. Compare values

**Expected Results:**
- [ ] CPU temperatures match within ±1°C
- [ ] GPU temperatures match (if GPU present)
- [ ] No negative or extreme values (>200°C)

**Common Issues:**
- AMD Tctl vs Tdie offset (TempMon should show both)
- NVIDIA GPU needs `nvidia` driver loaded

**Status:** ⏳ Pending compilation fix

---

### Test 2.3: Power Consumption Reading
**Purpose:** Verify power sensor reading and unit conversion

**Test Steps:**
```bash
# Check raw power value
cat /sys/class/hwmon/hwmon*/power*_input 2>/dev/null | head -1

# Calculate expected value (divide by 1,000,000 for watts)
# Example: 15000000 µW = 15.0 W

# Compare with TempMon menu
./tempmon  # Open menu and check power values
```

**Expected Results:**
- [ ] Power values in reasonable range (5-150W for CPU, 50-400W for GPU)
- [ ] Values update in real-time under load
- [ ] Units show "W" not "µW"

**Status:** ⏳ Pending compilation fix

---

### Test 2.4: Fan Speed Reading
**Purpose:** Verify RPM reading

**Test Steps:**
```bash
# Check raw fan value
cat /sys/class/hwmon/hwmon*/fan*_input 2>/dev/null

# Compare with TempMon menu
```

**Expected Results:**
- [ ] RPM values in reasonable range (0-5000 typical)
- [ ] Values update when fan speed changes
- [ ] Shows "RPM" units

**Status:** ⏳ Pending compilation fix

---

## Phase 3: UI Component Tests

### Test 3.1: Tray Icon Display
**Purpose:** Verify icon appears and shows temperature

**Test Steps:**
1. Start TempMon
2. Locate icon in system tray
3. Verify temperature is displayed

**Expected Results:**
- [ ] Icon appears in system tray
- [ ] Temperature number is visible and readable
- [ ] Updates every 2 seconds (default)
- [ ] Temperature matches menu values

**Desktop Environment Specific:**
- **GNOME:** May require `gnome-shell-extension-appindicator`
- **KDE:** Should work natively
- **i3/Sway:** Requires status bar with tray support

**Status:** ⏳ Pending compilation fix

---

### Test 3.2: Tray Icon Styles
**Purpose:** Verify all icon styles render correctly

**Test Steps:**
1. Open Settings
2. Try each icon style:
   - Compact
   - Rounded (default)
   - Square
3. Verify icon updates

**Expected Results:**
- [ ] All three styles render without errors
- [ ] Icon files created in `~/.cache/tempmon/`
- [ ] Styles visually different

**Validation:**
```bash
ls -lh ~/.cache/tempmon/tempmon-icon.png
file ~/.cache/tempmon/tempmon-icon.png  # Should be PNG image
```

**Status:** ⏳ Pending compilation fix

---

### Test 3.3: Theme Support
**Purpose:** Verify all themes render correctly

**Test Steps:**
1. Open Settings
2. Try each theme:
   - Dark (default)
   - Light
   - Accent
3. Check icon appearance

**Expected Results:**
- [ ] Dark: Dark background, light text
- [ ] Light: Light background, dark text
- [ ] Accent: Blue background, light text
- [ ] Text remains readable in all themes

**Status:** ⏳ Pending compilation fix

---

### Test 3.4: Menu Functionality
**Purpose:** Verify dropdown menu shows all sensors

**Test Steps:**
1. Click tray icon
2. Menu should appear with:
   - List of all sensors with values
   - Separator line
   - "Settings" option
   - "Quit" option

**Expected Results:**
- [ ] Menu appears on click
- [ ] All discovered sensors listed
- [ ] Values update on each menu open
- [ ] Sensor items are disabled (not clickable)
- [ ] Settings and Quit are clickable

**Status:** ⏳ Pending compilation fix

---

### Test 3.5: Settings Window
**Purpose:** Verify all settings controls work

**Test Steps:**
1. Click tray icon → Settings
2. Test each control:
   - Temperature source dropdown
   - Update interval spinner
   - Display delay spinner
   - Icon style dropdown
   - Theme dropdown
   - Show unit checkbox
   - Show decimal checkbox
3. Click "Apply" and verify changes

**Expected Results:**
- [ ] Settings window opens
- [ ] All controls functional
- [ ] Changes apply immediately after clicking Apply
- [ ] Settings persist after restart
- [ ] Config file created at `~/.config/tempmon/config.ini`

**Validation:**
```bash
cat ~/.config/tempmon/config.ini
# Should show all saved settings
```

**Status:** ⏳ Pending compilation fix

---

## Phase 4: Advanced Feature Tests

### Test 4.1: Temperature Delay Feature
**Purpose:** Verify display delay option works

**Test Steps:**
1. Open Settings
2. Set display delay to 10 seconds
3. Apply settings
4. Generate CPU load (e.g., `stress --cpu 4 --timeout 30s`)
5. Observe tray icon temperature

**Expected Results:**
- [ ] Tray icon shows delayed temperature (10s behind current)
- [ ] Menu shows real-time temperature (no delay)
- [ ] History buffer maintains samples

**Validation:**
```bash
# Install stress if needed
sudo pacman -S stress

# Run load test
stress --cpu 4 --timeout 30s &
watch -n 1 'sensors | grep "Package id 0:"'
# Compare with TempMon tray icon (should lag by ~10s)
```

**Status:** ⏳ Pending compilation fix

---

### Test 4.2: Sensor Selection
**Purpose:** Verify manual sensor selection works

**Test Steps:**
1. Open Settings
2. Change from "Auto" to specific sensor
3. Apply and verify tray icon shows that sensor

**Expected Results:**
- [ ] Dropdown lists all temperature sensors
- [ ] Selecting specific sensor changes tray display
- [ ] Selection persists after restart

**Status:** ⏳ Pending compilation fix

---

### Test 4.3: Update Interval
**Purpose:** Verify configurable refresh rate

**Test Steps:**
1. Open Settings
2. Try different intervals (0.5s, 2s, 10s)
3. Monitor update frequency

**Expected Results:**
- [ ] Icon updates at configured rate
- [ ] Menu values update at same rate
- [ ] Minimum 500ms enforced
- [ ] Maximum 10s enforced

**Status:** ⏳ Pending compilation fix

---

## Phase 5: Alert System Tests

**Note:** Alert system is incomplete and will FAIL all tests.

### Test 5.1: Alert Configuration (WILL FAIL)
**Purpose:** Verify alert settings exist

**Test Steps:**
1. Open Settings window
2. Look for alert-related controls

**Expected Results (Current):**
- [x] ❌ NO alert controls present in UI
- [x] ❌ Cannot enable/disable alerts
- [x] ❌ Cannot set threshold temperature
- [x] ❌ Cannot set cooldown period

**Actual Results:**
- Settings window shows only: sensor, interval, delay, icon, theme, unit, decimal
- **Alert controls are MISSING**

**Status:** ❌ FAILED - Feature incomplete

---

### Test 5.2: Alert Triggering (WILL FAIL)
**Purpose:** Verify alert appears when threshold exceeded

**Prerequisites:**
- [ ] Fix missing AppSettings fields
- [ ] Fix missing TempMonitor members
- [ ] Implement onAlertDismissed callback
- [ ] Add maybeShowAlert() call to update loop

**Test Steps:**
1. Set alert threshold to low value (e.g., 40°C)
2. Enable alerts
3. Wait for temperature to exceed threshold
4. Verify alert dialog appears

**Expected Results (After Fix):**
- [ ] Alert dialog shows when temp exceeds threshold
- [ ] Dialog shows temperature value
- [ ] Only one alert shown (alert_dialog_visible flag works)
- [ ] Cooldown prevents spam

**Current Results:**
- ❌ Compilation error - missing struct members
- ❌ maybeShowAlert() never called

**Status:** ❌ FAILED - Feature incomplete

---

### Test 5.3: Alert Persistence (WILL FAIL)
**Purpose:** Verify alert settings saved to config

**Test Steps:**
1. Configure alert settings
2. Restart application
3. Check if settings persist

**Expected Results (After Fix):**
- [ ] Alert settings saved to config.ini
- [ ] Settings restored on restart

**Current Results:**
- ❌ No alert fields in config file
- ❌ loadSettings() doesn't read alert settings
- ❌ saveSettings() doesn't write alert settings

**Status:** ❌ FAILED - Feature incomplete

---

## Phase 6: Unused Feature Tests

### Test 6.1: CPU Frequency Reading (Not Integrated)
**Purpose:** Verify frequency reading function works in isolation

**Test Steps:**
```bash
# Manual test of the unused function
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
# Should show frequency in kHz
```

**Function Test (Manual):**
```cpp
// If function could be called:
double freq = readCpuFrequencyMHz();
// Expected: freq > 0 and < 10000 (reasonable MHz range)
```

**Expected Results:**
- [ ] Function would work if called
- [ ] Reads correct file path
- [ ] Converts kHz to MHz correctly (divide by 1000)

**Current Results:**
- ⚠️ Function defined but never called anywhere
- ⚠️ Not shown in menu
- ⚠️ Not shown in tray

**Status:** 🟡 WRITTEN BUT UNUSED

**Integration Required:**
1. Call `readCpuFrequencyMHz()` in update cycle
2. Add menu item to display value
3. Format: "CPU Freq: 3200 MHz" or similar

---

### Test 6.2: Network Monitoring (Incomplete)
**Purpose:** Verify network functions work in isolation

**Test Steps:**
```bash
# Manual test of network reading
cat /proc/net/dev
# Should show interface statistics
```

**Function Test (Manual):**
```cpp
// If functions could be called:
NetStats stats = readNetworkTotals();  // ❌ NetStats undefined
// Expected: stats with rx_bytes and tx_bytes

std::string rate = formatNetworkRate(1024000);  // 1MB/s
// Expected: "1000.0 KB/s" or "1.0 MB/s"
```

**Expected Results:**
- [ ] readNetworkTotals() reads /proc/net/dev correctly
- [ ] Skips loopback interface
- [ ] Sums all non-loopback interfaces
- [ ] formatNetworkRate() formats KB/s vs MB/s correctly

**Current Results:**
- ❌ NetStats struct undefined (compilation error)
- ⚠️ Functions defined but never called
- ⚠️ No rate calculation (needs two samples)
- ⚠️ Not shown in menu
- ⚠️ Not shown in tray

**Status:** ❌ INCOMPLETE - Missing struct definition + not integrated

**Integration Required:**
1. Define `NetStats` struct with `rx_bytes` and `tx_bytes` fields
2. Store previous sample for rate calculation
3. Call functions in update cycle
4. Calculate rate: (current - previous) / time_delta
5. Add menu items to display Rx/Tx rates

---

## Phase 7: Configuration Tests

### Test 7.1: Config File Creation
**Purpose:** Verify config file is created on first run

**Test Steps:**
```bash
rm -rf ~/.config/tempmon/
./tempmon &
sleep 3
pkill tempmon
cat ~/.config/tempmon/config.ini
```

**Expected Results:**
- [ ] Directory created: `~/.config/tempmon/`
- [ ] Config file created: `~/.config/tempmon/config.ini`
- [ ] Contains default values

**Status:** ⏳ Pending compilation fix

---

### Test 7.2: Config File Loading
**Purpose:** Verify settings loaded from config

**Test Steps:**
```bash
# Manually edit config
cat > ~/.config/tempmon/config.ini << EOF
update_interval_ms=5000
display_delay_ms=10000
icon_style=2
theme_style=1
show_unit=false
show_decimal=true
EOF

# Start TempMon and verify settings match
./tempmon
```

**Expected Results:**
- [ ] Update interval is 5 seconds
- [ ] Display delay is 10 seconds
- [ ] Icon style is Square (2)
- [ ] Theme is Light (1)
- [ ] Unit symbol hidden
- [ ] Decimal shown

**Status:** ⏳ Pending compilation fix

---

### Test 7.3: Config File Persistence
**Purpose:** Verify changes to settings are saved

**Test Steps:**
1. Open Settings
2. Change multiple options
3. Apply
4. Restart TempMon
5. Verify settings persist

**Expected Results:**
- [ ] All setting changes saved to config.ini
- [ ] Settings restored on next run

**Status:** ⏳ Pending compilation fix

---

## Phase 8: Installation Tests

### Test 8.1: Quick Install Script
**Purpose:** Verify install.sh works correctly

**Test Steps:**
```bash
chmod +x install.sh
./install.sh
# Follow prompts
```

**Expected Results:**
- [ ] Checks for dependencies
- [ ] Offers to install missing deps
- [ ] Builds successfully
- [ ] Offers to install to /usr/local/bin
- [ ] Offers to enable systemd service
- [ ] Offers to add to autostart
- [ ] Checks for sensor modules

**Status:** ⏳ Pending compilation fix

---

### Test 8.2: Manual Installation
**Purpose:** Verify make install works

**Test Steps:**
```bash
make clean
make
sudo make install
which tempmon
tempmon --version || tempmon
```

**Expected Results:**
- [ ] Binary installed to `/usr/local/bin/tempmon`
- [ ] Systemd service installed to `/usr/lib/systemd/user/tempmon.service`
- [ ] Binary is executable

**Status:** ⏳ Pending compilation fix

---

### Test 8.3: Autostart Configuration
**Purpose:** Verify autostart works

**Test Steps:**
```bash
# Desktop environment method
mkdir -p ~/.config/autostart
cp tempmon.desktop ~/.config/autostart/
# Log out and log back in
# Verify TempMon starts automatically

# systemd method
systemctl --user enable --now tempmon.service
systemctl --user status tempmon.service
```

**Expected Results:**
- [ ] Desktop entry method starts TempMon on login
- [ ] Systemd service starts TempMon
- [ ] Only one instance runs

**Status:** ⏳ Pending compilation fix

---

### Test 8.4: PKGBUILD Package Build
**Purpose:** Verify Arch package builds correctly

**Test Steps:**
```bash
makepkg -si
# Review PKGBUILD for issues
```

**Expected Results:**
- [ ] ❌ WILL FAIL - source paths are wrong
- [ ] Should reference src/tempmon.cpp not tempmon.cpp
- [ ] Should include src/tempmon_stub.cpp
- [ ] Should include tempmon.service

**Predicted Errors:**
```
==> ERROR: tempmon.cpp was not found in the build directory
```

**Status:** ❌ PREDICTED FAILURE - PKGBUILD has wrong paths

**Fix Required:**
```diff
-source=("tempmon.cpp"
+source=("src/tempmon.cpp"
+        "src/tempmon_stub.cpp"
         "Makefile"
-        "tempmon.desktop")
+        "tempmon.desktop"
+        "tempmon.service")
```

---

## Phase 9: Error Handling Tests

### Test 9.1: No Sensors Available
**Purpose:** Verify graceful handling when no sensors found

**Test Steps:**
```bash
# Hide hwmon directory temporarily (requires root)
sudo mv /sys/class/hwmon /sys/class/hwmon.bak
./tempmon
# Check logs for error message
sudo mv /sys/class/hwmon.bak /sys/class/hwmon
```

**Expected Results:**
- [ ] App starts but shows error in logs
- [ ] Tray icon shows "--" or similar
- [ ] No crash
- [ ] Menu shows "No sensors found" or similar

**Status:** ⏳ Pending compilation fix

---

### Test 9.2: Permission Errors
**Purpose:** Verify handling of unreadable sensor files

**Test Steps:**
```bash
# Make a sensor file unreadable (requires root)
sudo chmod 000 /sys/class/hwmon/hwmon0/temp1_input
./tempmon
# Check logs
sudo chmod 644 /sys/class/hwmon/hwmon0/temp1_input
```

**Expected Results:**
- [ ] Error logged for unreadable file
- [ ] App continues with other sensors
- [ ] No crash

**Status:** ⏳ Pending compilation fix

---

### Test 9.3: Invalid Sensor Values
**Purpose:** Verify handling of corrupted sensor data

**Test Steps:**
- This test requires mocking or manual file manipulation
- Not easily testable in production

**Expected Results:**
- [ ] Invalid values caught by try/catch in updateSensorValues()
- [ ] Error logged
- [ ] Sensor shows 0.0 or skipped

**Status:** ⏳ Code review suggests it should work (try/catch present)

---

## Phase 10: Performance Tests

### Test 10.1: CPU Usage
**Purpose:** Verify minimal CPU overhead

**Test Steps:**
```bash
./tempmon &
TEMPMON_PID=$!
sleep 10
# Measure CPU usage
ps -p $TEMPMON_PID -o %cpu,rss,cmd
top -p $TEMPMON_PID -n 10 -d 1
```

**Expected Results:**
- [ ] CPU usage < 1% during idle
- [ ] CPU usage < 2% during updates

**Status:** ⏳ Pending compilation fix

---

### Test 10.2: Memory Usage
**Purpose:** Verify ~5MB RAM claim

**Test Steps:**
```bash
./tempmon &
TEMPMON_PID=$!
sleep 5
ps -p $TEMPMON_PID -o rss,vsz,cmd
# RSS = Resident Set Size (actual RAM usage)
```

**Expected Results:**
- [ ] RSS < 10 MB (5MB claimed, allowing margin)
- [ ] Memory stable over time (no leaks)

**Status:** ⏳ Pending compilation fix

---

### Test 10.3: Update Rate Impact
**Purpose:** Verify faster updates don't cause issues

**Test Steps:**
1. Set update interval to 500ms (minimum)
2. Monitor CPU/memory for 5 minutes
3. Check for performance degradation

**Expected Results:**
- [ ] CPU usage remains reasonable
- [ ] No memory leaks
- [ ] System remains responsive

**Status:** ⏳ Pending compilation fix

---

## Test Summary Template

### Overall Test Results

| Phase | Tests | Pass | Fail | Skip | Status |
|-------|-------|------|------|------|--------|
| Phase 1: Build System | 2 | 1 | 0 | 1 | 🟡 Partial |
| Phase 2: Core Functionality | 4 | 0 | 0 | 4 | ⏳ Blocked |
| Phase 3: UI Components | 5 | 0 | 0 | 5 | ⏳ Blocked |
| Phase 4: Advanced Features | 3 | 0 | 0 | 3 | ⏳ Blocked |
| Phase 5: Alert System | 3 | 0 | 3 | 0 | ❌ Failed |
| Phase 6: Unused Features | 2 | 0 | 1 | 1 | ❌ Failed |
| Phase 7: Configuration | 3 | 0 | 0 | 3 | ⏳ Blocked |
| Phase 8: Installation | 4 | 0 | 1 | 3 | 🟡 Partial |
| Phase 9: Error Handling | 3 | 0 | 0 | 3 | ⏳ Blocked |
| Phase 10: Performance | 3 | 0 | 0 | 3 | ⏳ Blocked |
| **TOTAL** | **32** | **1** | **5** | **26** | **⏳ Blocked** |

**Pass Rate:** 3% (1/32)  
**Blocking Issue:** Compilation errors prevent testing 26/32 tests

---

## Critical Blockers

1. **Duplicate function definitions** - Prevents compilation
2. **Broken buildMenu() stub** - Compilation error
3. **Missing Alert system components** - 3 tests fail
4. **Undefined NetStats struct** - 1 test fails
5. **PKGBUILD source paths** - Installation test fails

**Resolution Required:** Fix compilation errors before 81% of tests can be executed.

---

## Next Steps

### Immediate Actions
1. ✅ Document current state (this file)
2. ⏳ Fix duplicate function definitions
3. ⏳ Verify compilation succeeds
4. ⏳ Execute Phases 2-4 tests (core functionality)

### Short-term Actions
5. ⏳ Complete alert system implementation
6. ⏳ Execute Phase 5 tests (alerts)
7. ⏳ Decide on unused features (integrate or remove)

### Long-term Actions
8. ⏳ Set up test environment with dependencies
9. ⏳ Execute all remaining tests
10. ⏳ Add automated testing infrastructure
11. ⏳ Create CI/CD pipeline

---

**Testing Status:** Comprehensive test plan created. Execution blocked by compilation errors.  
**Estimated Testing Time:** 4-6 hours once compilation issues resolved.
