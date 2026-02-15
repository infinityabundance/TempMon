# TempMon Development Roadmap & Decision Guide

**Created:** 2026-02-15  
**Purpose:** Visual guide for prioritizing and completing TempMon development  
**Current Stage:** Late Alpha (Core works, needs cleanup & integration)

---

## Current State Visualization

```
┌─────────────────────────────────────────────────────────────────┐
│                        TEMPMON v1.0                              │
│                    Current Completion: 69%                       │
└─────────────────────────────────────────────────────────────────┘

┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│  CORE FEATURES   │  │  UI FEATURES     │  │ ADVANCED         │
│     ✅ 75%       │  │     ✅ 90%       │  │    ⚠️ 25%        │
├──────────────────┤  ├──────────────────┤  ├──────────────────┤
│ ✅ Temp sensors  │  │ ✅ Tray icon     │  │ ⚠️ Alerts (60%)  │
│ ✅ Power sensors │  │ ✅ Menu system   │  │ 🟡 CPU freq      │
│ ✅ Fan sensors   │  │ ✅ Settings UI   │  │ 🟡 Network stats │
│ ✅ Auto-discover │  │ ✅ Themes (3)    │  │ ❌ Graphing      │
│ 🟡 CPU frequency │  │ ✅ Icon styles   │  │ ❌ Color alerts  │
│ 🟡 Network stats │  │ ✅ Config save   │  │                  │
└──────────────────┘  └──────────────────┘  └──────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                    CRITICAL BLOCKERS 🔴                          │
├─────────────────────────────────────────────────────────────────┤
│ ❌ Won't compile with GTK3/libappindicator3 dependencies       │
│ ❌ 7 duplicate function definitions                             │
│ ❌ buildMenu() returns undefined variable in void function      │
├─────────────────────────────────────────────────────────────────┤
│ Impact: BLOCKS ALL FEATURE DEVELOPMENT & TESTING                │
│ Must fix before anything else can proceed                       │
└─────────────────────────────────────────────────────────────────┘
```

---

## Development Decision Tree

```
START: Fix TempMon
    |
    ├─> Q1: Can you compile with full dependencies?
    |   │
    |   NO ──> 🔴 STOP - Fix compilation first
    |   │      ├─ Remove duplicate functions (lines 471-620)
    |   │      ├─ Fix buildMenu stub (line 633)
    |   │      └─ Verify: make clean && make
    |   │
    |   YES ──> Continue to Q2
    |
    ├─> Q2: What's your priority goal?
    |   │
    |   ├─> [A] Get to working beta quickly (Recommended)
    |   │   └─> Follow "Quick Beta Path" below
    |   │
    |   ├─> [B] Complete all documented features
    |   │   └─> Follow "Full Feature Path" below
    |   │
    |   ├─> [C] Simplify and clean up
    |   │   └─> Follow "Simplification Path" below
    |   │
    |   └─> [D] Add new features
    |       └─> Fix compilation first, then decide
    |
    └─> Q3: What about unused code (CPU freq, network)?
        │
        ├─> [Keep] Integrate into application
        │   ├─ Effort: ~3 hours
        │   └─ Adds value to users
        │
        └─> [Remove] Delete unused functions
            ├─ Effort: ~30 minutes
            └─ Simplifies codebase
```

---

## Path A: Quick Beta (Recommended)

**Goal:** Minimal changes to reach stable beta  
**Time:** 3-4 hours  
**Complexity:** Low

```
Step 1: Fix Critical Errors (1 hour)
    └─> Remove duplicate functions
    └─> Fix broken stub
    └─> Test compilation

Step 2: Remove/Hide Incomplete Features (30 min)
    └─> Comment out maybeShowAlert() function
    └─> Remove alert references from docs
    └─> Update README feature list

Step 3: Clean Up Docs (30 min)
    └─> Create minimal ENHANCEMENTS.md or remove refs
    └─> Fix PKGBUILD source paths
    └─> Update README to show CPU/Network as "Planned"

Step 4: Test & Verify (1-2 hours)
    └─> Build with dependencies
    └─> Run manual tests (Phases 2-4)
    └─> Verify core features work
    └─> Take real screenshots

RESULT: Working, stable application with accurate docs
```

**Pros:**
- ✅ Fastest path to stable release
- ✅ No broken features in codebase
- ✅ Accurate documentation
- ✅ Can be released as beta

**Cons:**
- ⚠️ Fewer features than documented
- ⚠️ Alert system removed temporarily

---

## Path B: Full Feature Completion

**Goal:** Implement all documented features  
**Time:** 8-12 hours  
**Complexity:** Medium-High

```
Step 1: Fix Critical Errors (1 hour)
    └─> Same as Path A, Step 1

Step 2: Complete Alert System (3-4 hours)
    ├─> Add missing AppSettings fields
    ├─> Add missing TempMonitor members
    ├─> Implement onAlertDismissed callback
    ├─> Add alert controls to Settings UI
    ├─> Update loadSettings/saveSettings
    ├─> Call maybeShowAlert from update loop
    └─> Test alert functionality

Step 3: Integrate CPU Frequency (1 hour)
    ├─> Call readCpuFrequencyMHz() in update
    ├─> Add "CPU Freq: X MHz" to menu
    └─> Test on various CPU types

Step 4: Complete Network Monitoring (2-3 hours)
    ├─> Define NetStats struct
    ├─> Store previous sample for rate calc
    ├─> Add network update to timer loop
    ├─> Add "Network: Rx/Tx" items to menu
    └─> Test with various network loads

Step 5: Documentation & Testing (1-2 hours)
    ├─> Create ENHANCEMENTS.md
    ├─> Fix PKGBUILD
    ├─> Update README with complete feature list
    ├─> Run full test suite
    └─> Take screenshots

RESULT: Feature-complete application matching documentation
```

**Pros:**
- ✅ All documented features implemented
- ✅ More value for users
- ✅ Documentation is accurate

**Cons:**
- ⚠️ More time investment
- ⚠️ More potential bugs to fix
- ⚠️ Alert system needs careful testing

---

## Path C: Simplification & Quality

**Goal:** Clean, maintainable codebase with core features  
**Time:** 5-6 hours  
**Complexity:** Medium

```
Step 1: Fix Critical Errors (1 hour)
    └─> Same as Path A, Step 1

Step 2: Remove Incomplete Features (1 hour)
    ├─> Delete maybeShowAlert() function (lines 734-767)
    ├─> Delete readCpuFrequencyMHz() (lines 672-685)
    ├─> Delete readNetworkTotals() (lines 687-719)
    ├─> Delete formatNetworkRate() (lines 721-732)
    └─> Update README to mark as "Planned Features"

Step 3: Code Quality Improvements (2-3 hours)
    ├─> Verify no more duplicate code exists
    ├─> Add code comments where needed
    ├─> Improve error messages
    ├─> Consider adding logging levels
    └─> Run static analysis (cppcheck, clang-tidy)

Step 4: Documentation & Testing (1-2 hours)
    ├─> Create accurate ENHANCEMENTS.md with future ideas
    ├─> Fix all README inaccuracies
    ├─> Update Contributing section with architecture notes
    ├─> Add inline code documentation
    └─> Test core features thoroughly

RESULT: Clean, well-documented app with solid core features
```

**Pros:**
- ✅ Clean, maintainable codebase
- ✅ No technical debt
- ✅ Easy for contributors to understand
- ✅ Accurate, honest documentation

**Cons:**
- ⚠️ Fewer features than currently documented
- ⚠️ Users may want alerts/CPU/network features

---

## Compilation Fix Guide (Required for All Paths)

### Issue 1: Duplicate Functions

**Problem:** Functions defined twice with different signatures

**Affected Functions:**
```
updateIndicator()       Lines 616 (void) and 879 (double)
buildMenu()            Lines 623 (stub) and 887 (complete)
formatIconText()       Lines 507-524 and 770-787
iconColors()           Lines 527-546 and 790-809
drawRoundedRect()      Lines 549-557 and 812-820
renderIcon()           Lines 560-613 and 823-876
computeDisplayTemp()   Lines 471-504 and 637-670
```

**Solution:** Delete first occurrence (keep second)

**Steps:**
```bash
# Open editor
vim src/tempmon.cpp

# Delete these line ranges (in reverse order to preserve line numbers):
# - Lines 637-670 (computeDisplayTemperature - first)
# - Lines 560-620 (renderIcon + updateIndicator - first)
# - Lines 549-557 (drawRoundedRect - first)
# - Lines 527-546 (iconColors - first)
# - Lines 507-524 (formatIconText - first)
# - Lines 471-504 (computeDisplayTemperature - keep looking)
# - Lines 623-634 (broken buildMenu - first)

# Or use sed to delete ranges:
sed -i '623,634d' src/tempmon.cpp  # Fix broken buildMenu first
sed -i '560,620d' src/tempmon.cpp  # Remove first renderIcon + updateIndicator
sed -i '549,557d' src/tempmon.cpp  # Remove first drawRoundedRect
sed -i '527,546d' src/tempmon.cpp  # Remove first iconColors
sed -i '507,524d' src/tempmon.cpp  # Remove first formatIconText
sed -i '471,504d' src/tempmon.cpp  # Remove first computeDisplayTemperature

# Verify compilation
make clean
make 2>&1 | tee build.log

# Should succeed now
```

**Warning:** Line numbers will shift after each deletion. Consider using an editor that handles multi-cursor or block deletion.

### Issue 2: Broken buildMenu() Stub

**Problem:** Line 633 returns value in void function

**Current Code:**
```cpp
void buildMenu() {
    if (!menu) return;
    GList* items = gtk_container_get_children(GTK_CONTAINER(menu));
    for (GList* iter = items; iter != nullptr; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    return max_cpu_temp;  // ❌ ERROR
}
```

**Solution:** Delete entire stub (lines 623-634)

Keep the complete version at lines 887-921 instead.

---

## Feature-Specific Guides

### Alert System Completion Guide

**Missing Components:**

1. **AppSettings fields (add to line ~172):**
```cpp
struct AppSettings {
    // ... existing fields ...
    bool alert_enabled = false;
    double alert_threshold_c = 80.0;
    int alert_cooldown_seconds = 300;
};
```

2. **TempMonitor members (add to line ~223):**
```cpp
private:
    // ... existing members ...
    Clock::time_point last_alert_time = Clock::now();
    bool alert_dialog_visible = false;
```

3. **onAlertDismissed callback (add around line ~1100):**
```cpp
static void onAlertDismissed(GtkDialog*, gint, gpointer data) {
    auto* self = static_cast<TempMonitor*>(data);
    self->alert_dialog_visible = false;
    gtk_widget_destroy(GTK_WIDGET(data));
}
```

4. **Settings UI (add to showSettingsWindow around line ~950):**
```cpp
// Add after theme controls:
GtkWidget* alert_check = gtk_check_button_new_with_label("Enable temperature alerts");
gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(alert_check), settings.alert_enabled);
gtk_box_pack_start(GTK_BOX(content_box), alert_check, FALSE, FALSE, 5);

GtkWidget* threshold_label = gtk_label_new("Alert threshold (°C):");
GtkWidget* threshold_spin = gtk_spin_button_new_with_range(50, 100, 1);
gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold_spin), settings.alert_threshold_c);
// ... pack into box ...

GtkWidget* cooldown_label = gtk_label_new("Cooldown (seconds):");
GtkWidget* cooldown_spin = gtk_spin_button_new_with_range(60, 600, 30);
gtk_spin_button_set_value(GTK_SPIN_BUTTON(cooldown_spin), settings.alert_cooldown_seconds);
// ... pack into box ...

// Store widget references
g_object_set_data(G_OBJECT(window), "alert_check", alert_check);
g_object_set_data(G_OBJECT(window), "threshold_spin", threshold_spin);
g_object_set_data(G_OBJECT(window), "cooldown_spin", cooldown_spin);
```

5. **applySettingsFromWindow (add around line ~1070):**
```cpp
auto* alert_check = GTK_TOGGLE_BUTTON(g_object_get_data(..., "alert_check"));
auto* threshold_spin = GTK_SPIN_BUTTON(g_object_get_data(..., "threshold_spin"));
auto* cooldown_spin = GTK_SPIN_BUTTON(g_object_get_data(..., "cooldown_spin"));

settings.alert_enabled = gtk_toggle_button_get_active(alert_check);
settings.alert_threshold_c = gtk_spin_button_get_value(threshold_spin);
settings.alert_cooldown_seconds = gtk_spin_button_get_value(cooldown_spin);
```

6. **loadSettings (add around line ~270):**
```cpp
else if (key == "alert_enabled") {
    settings.alert_enabled = (value == "true");
}
else if (key == "alert_threshold_c") {
    settings.alert_threshold_c = std::stod(value);
}
else if (key == "alert_cooldown_seconds") {
    settings.alert_cooldown_seconds = std::stoi(value);
}
```

7. **saveSettings (add around line ~292):**
```cpp
file << "alert_enabled=" << (settings.alert_enabled ? "true" : "false") << "\n";
file << "alert_threshold_c=" << settings.alert_threshold_c << "\n";
file << "alert_cooldown_seconds=" << settings.alert_cooldown_seconds << "\n";
```

8. **Call from update (modify updateCallback around line ~1114):**
```cpp
static gboolean updateCallback(gpointer data) {
    auto* monitor = static_cast<TempMonitor*>(data);
    monitor->updateSensorValues();
    double temp = monitor->updateIndicator();  // Now returns temperature
    monitor->buildMenu();
    monitor->maybeShowAlert(temp);  // ADD THIS LINE
    return TRUE;
}
```

**Estimated Time:** 3-4 hours  
**Complexity:** Medium  
**Benefit:** Complete advertised feature

---

### CPU Frequency Integration Guide

**Steps:**

1. **Add to TempMonitor class (around line ~400):**
```cpp
double cpu_frequency_mhz = 0.0;  // Store as member variable
```

2. **Call in updateSensorValues (around line ~408):**
```cpp
void updateSensorValues() {
    // ... existing sensor updates ...
    cpu_frequency_mhz = readCpuFrequencyMHz();
}
```

3. **Add to menu in buildMenu (around line ~906):**
```cpp
// After sensor loop, before separator:
if (cpu_frequency_mhz > 0.0) {
    std::ostringstream oss;
    oss << "CPU Freq: " << std::fixed << std::setprecision(0) 
        << cpu_frequency_mhz << " MHz";
    GtkWidget* item = gtk_menu_item_new_with_label(oss.str().c_str());
    gtk_widget_set_sensitive(item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
}
```

**Estimated Time:** 30 minutes  
**Complexity:** Low  
**Benefit:** Useful system information

---

### Network Monitoring Integration Guide

**Steps:**

1. **Define NetStats struct (around line ~143):**
```cpp
struct NetStats {
    double rx_bytes = 0.0;
    double tx_bytes = 0.0;
};
```

2. **Add to TempMonitor members (around line ~223):**
```cpp
NetStats prev_network_stats;
NetStats current_network_stats;
Clock::time_point last_network_time = Clock::now();
```

3. **Update in updateSensorValues (around line ~408):**
```cpp
void updateSensorValues() {
    // ... existing updates ...
    
    auto now = Clock::now();
    prev_network_stats = current_network_stats;
    current_network_stats = readNetworkTotals();
    last_network_time = now;
}
```

4. **Add rate calculation method:**
```cpp
std::pair<double, double> getNetworkRates() const {
    // Returns (rx_rate, tx_rate) in bytes/sec
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        Clock::now() - last_network_time).count();
    if (elapsed == 0) return {0.0, 0.0};
    
    double rx_rate = (current_network_stats.rx_bytes - prev_network_stats.rx_bytes) / elapsed;
    double tx_rate = (current_network_stats.tx_bytes - prev_network_stats.tx_bytes) / elapsed;
    return {rx_rate, tx_rate};
}
```

5. **Add to menu in buildMenu:**
```cpp
auto [rx_rate, tx_rate] = getNetworkRates();
if (rx_rate > 0 || tx_rate > 0) {
    std::string display = "Network: ↓" + formatNetworkRate(rx_rate) + 
                         " ↑" + formatNetworkRate(tx_rate);
    GtkWidget* item = gtk_menu_item_new_with_label(display.c_str());
    gtk_widget_set_sensitive(item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
}
```

**Estimated Time:** 2 hours  
**Complexity:** Medium  
**Benefit:** Useful for monitoring network activity

---

## Testing Recommendations by Path

### Path A (Quick Beta)
**Test Focus:** Core features only
- ✅ Sensor discovery
- ✅ Tray icon display
- ✅ Menu functionality
- ✅ Settings window
- ✅ Configuration persistence
- ✅ Update intervals

**Skip:**
- ❌ Alert system (removed)
- ❌ CPU frequency (not integrated)
- ❌ Network stats (not integrated)

---

### Path B (Full Feature)
**Test Focus:** Everything
- ✅ All Path A tests
- ✅ Alert triggering and cooldown
- ✅ Alert configuration persistence
- ✅ CPU frequency display
- ✅ Network rate calculation
- ✅ Network display in menu

**Critical Tests:**
1. Alert doesn't spam (cooldown works)
2. Alert threshold configurable
3. CPU frequency updates correctly
4. Network rates calculate properly

---

### Path C (Simplification)
**Test Focus:** Core + quality
- ✅ All Path A tests
- ✅ Error handling edge cases
- ✅ Memory leak checks
- ✅ CPU usage monitoring
- ✅ Invalid sensor data handling

**Quality Checks:**
1. No memory leaks (valgrind)
2. Low CPU usage (<1%)
3. Graceful error handling
4. Clear, helpful log messages

---

## Recommendation Matrix

### If you have...

**2-4 hours available:**
→ Choose **Path A** (Quick Beta)
- Gets you to stable release fastest
- Can always add features later
- Removes technical debt

**8-12 hours available:**
→ Choose **Path B** (Full Feature)
- Implements all documented features
- Best user experience
- Documentation matches reality

**Want clean codebase:**
→ Choose **Path C** (Simplification)
- Technical debt eliminated
- Easy for contributors
- Maintainability priority

**Urgent release needed:**
→ Choose **Path A** (Quick Beta)
- Minimal risk
- Fast to validate
- Can iterate later

**Long-term project:**
→ Choose **Path B** (Full Feature)
- Best feature set
- Complete implementation
- Worth the investment

---

## Success Criteria by Path

### Path A Success:
- [x] Compiles without errors
- [x] All core features work
- [x] Documentation is accurate
- [x] No broken features present
- [x] Can be released as v1.0-beta

### Path B Success:
- [x] All Path A criteria
- [x] Alert system fully functional
- [x] CPU frequency displayed
- [x] Network monitoring works
- [x] All documented features implemented
- [x] Can be released as v1.0

### Path C Success:
- [x] All Path A criteria
- [x] No code duplication
- [x] Excellent error handling
- [x] Well-documented code
- [x] Easy to maintain
- [x] Can be released as v1.0-stable

---

## Next Steps Checklist

**Immediate (Required):**
- [ ] Choose a development path (A, B, or C)
- [ ] Fix compilation errors (1-2 hours)
- [ ] Test that binary runs successfully
- [ ] Verify core features work

**Short-term (This week):**
- [ ] Complete chosen path requirements
- [ ] Run relevant test suite
- [ ] Update documentation
- [ ] Take real screenshots

**Medium-term (This month):**
- [ ] Add test infrastructure
- [ ] Set up GitHub Actions CI
- [ ] Create proper release (tag + binaries)
- [ ] Submit to AUR if not done

**Long-term (Optional):**
- [ ] Add features not in current plan
- [ ] Implement graphing support
- [ ] Add color-coded temperature zones
- [ ] Multi-language support

---

## Final Recommendations

**For Maintainer:**

1. **Start with Path A** to get stable beta
2. **Then incrementally add features** from Path B
3. **Keep code quality high** using lessons from Path C

**For Contributors:**

1. **Wait for compilation fix** before contributing
2. **Read CODE_ANALYSIS.md** to understand current state
3. **Check FEATURE_STATUS.md** to avoid duplicating work
4. **Follow TESTING_CHECKLIST.md** to verify changes

**For Users:**

1. **Wait for beta release** (after compilation fix)
2. **Report bugs with sensor detection** (most common issue)
3. **Request features** via GitHub issues
4. **Consider contributing** if you have C++/GTK experience

---

**Document Status:** Complete roadmap ready for decision-making  
**Last Updated:** 2026-02-15  
**Next Review:** After Path A/B/C completion
