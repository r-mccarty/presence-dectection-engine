# Development Workflow Guide

This guide explains the **two-machine development workflow** used for this project and provides detailed instructions for common development tasks.

## Table of Contents

- [Workflow Overview](#workflow-overview)
- [Network Access from Codespaces](#network-access-from-codespaces)
- [Standard Development Workflow](#standard-development-workflow)
- [Ubuntu-node Helper Scripts](#ubuntu-node-helper-scripts)
- [Common Development Tasks](#common-development-tasks)
- [Quick Iteration Patterns](#quick-iteration-patterns)

---

## Workflow Overview

**⚠️ CRITICAL:** This project uses a **two-location workflow**. Understanding this architecture is essential for effective development.

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                       DEVELOPMENT LOCATIONS                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌──────────────────────┐              ┌──────────────────────┐   │
│  │   GitHub Codespace   │              │    Ubuntu Node       │   │
│  │  ─────────────────   │              │  ────────────────    │   │
│  │  • Code editing      │◄────git─────►│  • Firmware flash   │   │
│  │  • Git operations    │              │  • Device testing   │   │
│  │  • Documentation     │              │  • USB connection   │   │
│  │  • NO device access  │              │  • HA integration   │   │
│  └──────────────────────┘              └──────────────────────┘   │
│         ▲                                         │                 │
│         │                                         │                 │
│         └───────── git push/pull ─────────────────┘                │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘

                              ▼
                    ┌──────────────────┐
                    │   M5Stack Device  │
                    │  ───────────────  │
                    │  USB connected to │
                    │  Ubuntu Node only │
                    └──────────────────┘
```

### Key Principles

1. **Source of Truth**: Git repository (GitHub)
2. **Code Development**: GitHub Codespace (edit, test compilation, commit, push)
3. **Firmware Flashing**: Ubuntu-node (git pull, then flash to device via USB)
4. **Secrets Management**: Ubuntu-node is source of truth for `secrets.yaml` and `.env.local` (gitignored)

**Why Two Machines?**
- Codespaces runs on GitHub's cloud infrastructure
- ESP32 device is physically connected to ubuntu-node via USB
- Codespaces cannot directly access USB devices or local network (192.168.0.148)
- Git synchronization bridges the two environments

---

## Network Access from Codespaces

**⚠️ IMPORTANT**: GitHub Codespaces runs on GitHub's cloud infrastructure and **cannot directly access** your local Home Assistant instance at `192.168.0.148:8123`.

### Accessing Home Assistant Web UI from Codespaces

To view the Home Assistant web interface from your Codespace browser, use **SSH port forwarding**:

```bash
# In Codespace terminal
ssh -L 8123:192.168.0.148:8123 ubuntu-node
```

**What this does:**
- Creates a tunnel: `localhost:8123` (Codespace) → `192.168.0.148:8123` (HA on ubuntu-node's network)
- Forwards local port 8123 to Home Assistant

**Then access Home Assistant at:** `http://localhost:8123` in your Codespace browser

**Keep the SSH session open** while browsing - closing it will break the tunnel.

**Advanced tunneling options:**

```bash
# Run in background (returns terminal to you)
ssh -fN -L 8123:192.168.0.148:8123 ubuntu-node

# With keep-alive (prevents timeout)
ssh -o ServerAliveInterval=60 -L 8123:192.168.0.148:8123 ubuntu-node

# Stop background tunnel
ps aux | grep "ssh.*8123"  # Find process ID
kill <process_id>           # Kill the tunnel
```

---

### Running Python Scripts That Need Home Assistant API

Scripts like `collect_baseline.py` need to connect to the Home Assistant API. You have **two options**:

**Option 1: Run Directly on Ubuntu-node** (RECOMMENDED)

```bash
ssh ubuntu-node "cd ~/bed-presence-sensor && python3 scripts/collect_baseline.py"
```

**Advantages:**
- Script runs on ubuntu-node where HA is localhost
- No network issues, fastest performance
- Uses `.env.local` on ubuntu-node
- Simpler and more reliable

**Option 2: Run in Codespace with SSH Tunnel**

```bash
# Terminal 1: Create SSH tunnel
ssh -L 8123:192.168.0.148:8123 ubuntu-node

# Terminal 2: Run script with localhost URL
cd /workspaces/bed-presence-sensor
python3 scripts/collect_baseline.py
# Script will use HA_URL=http://localhost:8123 from .env.local (if configured)
```

**Note:** Most development workflows run scripts directly on ubuntu-node to avoid networking complexity.

---

## Standard Development Workflow

This is the recommended workflow for making code changes and deploying them to the device.

### Step-by-Step: Code Changes

**1. In Codespace (code editing and testing):**

```bash
# Edit code (example: update presence engine logic)
nano esphome/custom_components/bed_presence_engine/bed_presence.h

# Test compilation (optional but recommended)
cd esphome
esphome compile bed-presence-detector.yaml

# Run C++ unit tests (optional but recommended)
platformio test -e native

# If tests pass, commit and push
cd /workspaces/bed-presence-sensor
git add esphome/custom_components/bed_presence_engine/bed_presence.h
git commit -m "feat: update presence detection threshold logic"
git push origin main
```

**2. On ubuntu-node (flash to device):**

```bash
# Option A: Use helper script (RECOMMENDED)
ssh ubuntu-node
~/sync-and-flash.sh

# Option B: Manual steps
ssh ubuntu-node
cd ~/bed-presence-sensor
git pull origin main              # Sync from GitHub
cd esphome
source ~/esphome-venv/bin/activate
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

**3. Verify in Home Assistant:**

- Open Home Assistant at `http://192.168.0.148:8123`
- Go to Settings → Devices & Services → ESPHome → "Bed Presence Detector"
- Check device logs for successful connection
- Test presence detection functionality

---

## Ubuntu-node Helper Scripts

Two automation scripts are available on ubuntu-node to simplify the workflow:

### Script 1: `~/sync-and-flash.sh` (RECOMMENDED)

**Purpose:** Pull latest code from GitHub and flash to device in one command.

**What it does:**
1. Pulls latest code from GitHub (`git pull origin main`)
2. Verifies WiFi credentials exist in `secrets.yaml`
3. Verifies baseline values in `bed_presence.h`
4. Flashes firmware to device via USB
5. Shows device logs

**Usage:**
```bash
ssh ubuntu-node
~/sync-and-flash.sh
```

**When to use:**
- After making changes in Codespace and pushing to GitHub
- When you want to deploy the latest code to hardware
- Standard workflow for most development tasks

---

### Script 2: `~/flash-firmware.sh`

**Purpose:** Flash existing code without pulling from GitHub.

**What it does:**
1. Runs pre-flight checklist (secrets, baseline, git status)
2. Flashes firmware from current local code
3. Shows device logs

**Usage:**
```bash
ssh ubuntu-node
~/flash-firmware.sh
```

**When to use:**
- Reflashing without code updates (e.g., after device reset)
- Testing existing code on a different device
- When you know the local code is already up to date

---

### Quick Reference: `cat ~/WORKFLOW-README.md`

Ubuntu-node has a quick reference file with workflow reminders:

```bash
ssh ubuntu-node "cat ~/WORKFLOW-README.md"
```

---

## Common Development Tasks

### Task 1: Modifying Presence Detection Thresholds

**Default values** (`bed_presence.h:49-50`):
```cpp
float k_on_{9.0f};   // Turn ON when z > 9.0 (9 std deviations)
float k_off_{4.0f};  // Turn OFF when z < 4.0 (4 std deviations)
```

**To change defaults (hardcoded in firmware):**

1. **In Codespace:**
   ```bash
   nano esphome/custom_components/bed_presence_engine/bed_presence.h
   # Update k_on_ and/or k_off_ values

   # Also update initial_value in presence_engine.yaml
   nano esphome/packages/presence_engine.yaml
   # Update lines 24 and 40 (initial_value for k_on and k_off)

   git add .
   git commit -m "feat: adjust default thresholds to k_on=10.0, k_off=3.5"
   git push origin main
   ```

2. **On ubuntu-node:**
   ```bash
   ssh ubuntu-node "~/sync-and-flash.sh"
   ```

**To change at runtime (no reflash needed):**
- Open Home Assistant → Settings → Devices → Bed Presence Detector
- Adjust `k_on (ON Threshold Multiplier)` and `k_off (OFF Threshold Multiplier)` sliders
- Changes take effect immediately
- Values persist across reboots (`restore_value: true`)

---

### Task 2: Updating Baseline Statistics (Recalibration)

**Current values** (`bed_presence.h:60-61`) ✅ **CALIBRATED** (2025-11-06):
```cpp
// Baseline calibration collected on 2025-11-06 18:39:42
// Location: New sensor position looking at bed
// Conditions: Empty bed, door closed, minimal movement
// Statistics: mean=6.67%, stdev=3.51%, n=30 samples over 60 seconds
float mu_still_{6.7f};     // Mean still energy (empty bed)
float sigma_still_{3.5f};  // Std dev still energy (empty bed)
```

**When to recalibrate:**
- Sensor position changed
- Frequent false positives or false negatives
- Environmental conditions changed significantly

**Recalibration process:**

1. **Collect baseline data (on ubuntu-node):**
   ```bash
   ssh ubuntu-node
   cd ~/bed-presence-sensor

   # Ensure bed is empty, room is quiet
   python3 scripts/collect_baseline.py

   # Script will output:
   # Mean (μ): 6.7%
   # Std Dev (σ): 3.5%
   ```

2. **Update firmware (in Codespace):**
   ```bash
   nano esphome/custom_components/bed_presence_engine/bed_presence.h

   # Update lines 60-61 with new values:
   float mu_still_{6.7f};     # Replace with your μ
   float sigma_still_{3.5f};  # Replace with your σ

   git add .
   git commit -m "calibration: update baseline to μ=6.7, σ=3.5"
   git push origin main
   ```

3. **Flash updated firmware (on ubuntu-node):**
   ```bash
   ssh ubuntu-node "~/sync-and-flash.sh"
   ```

4. **Verify calibration:**
   - Open Home Assistant → Developer Tools → States
   - Check `text_sensor.presence_state_reason`
   - With empty bed, z-score should be near 0 (typically -2.0 to 2.0)
   - With occupied bed, z-score should be > 9.0

**Note:** Phase 3 automation is live—prefer `esphome.bed_presence_detector_calibrate_start_baseline` unless you
need offline/raw data. Manual edits remain documented here for legacy workflows.

---

### Task 3: Adjusting Debounce Timers (Phase 2)

**Default values** (`bed_presence.h:70-72`):
```cpp
unsigned long on_debounce_ms_{3000};      // 3 seconds
unsigned long off_debounce_ms_{5000};     // 5 seconds
unsigned long abs_clear_delay_ms_{30000}; // 30 seconds
```

**To change defaults (hardcoded in firmware):**

1. **In Codespace:**
   ```bash
   nano esphome/custom_components/bed_presence_engine/bed_presence.h
   # Update timer values

   # Also update initial_value in presence_engine.yaml
   nano esphome/packages/presence_engine.yaml
   # Update initial_value for on_debounce_ms, off_debounce_ms, abs_clear_delay_ms

   git add .
   git commit -m "feat: increase debounce timers for more stability"
   git push origin main
   ```

2. **On ubuntu-node:**
   ```bash
   ssh ubuntu-node "~/sync-and-flash.sh"
   ```

**To change at runtime (no reflash needed):**
- Open Home Assistant → Settings → Devices → Bed Presence Detector
- Adjust timer number entities:
  - `on_debounce_ms` (milliseconds)
  - `off_debounce_ms` (milliseconds)
  - `abs_clear_delay_ms` (milliseconds)
- Changes take effect immediately
- Values persist across reboots

**Tuning guidelines:**
- **Sensor too twitchy**: Increase all timers (e.g., 5000, 10000, 60000)
- **Sensor too slow**: Decrease on_debounce_ms (e.g., 2000 or 1000)
- **Clears too quickly when still**: Increase abs_clear_delay_ms (e.g., 60000 or 120000)

---

### Task 4: Adding New Features to C++ Code

**Example: Adding a new configuration parameter**

1. **In Codespace (edit code):**
   ```bash
   # Add member variable to header
   nano esphome/custom_components/bed_presence_engine/bed_presence.h
   # Add: float new_parameter_{5.0f};

   # Add setter and updater methods
   # void set_new_parameter(float p) { new_parameter_ = p; }
   # void update_new_parameter(float p);

   # Implement in cpp file
   nano esphome/custom_components/bed_presence_engine/bed_presence.cpp

   # Add to binary_sensor.py schema
   nano esphome/custom_components/bed_presence_engine/binary_sensor.py

   # Add number entity in YAML
   nano esphome/packages/presence_engine.yaml

   # Update unit tests
   nano esphome/test/test_presence_engine.cpp
   ```

2. **Test compilation and tests:**
   ```bash
   cd esphome
   esphome compile bed-presence-detector.yaml  # Verify compilation
   platformio test -e native                   # Run unit tests
   ```

3. **Commit and flash:**
   ```bash
   git add .
   git commit -m "feat: add new parameter for X functionality"
   git push origin main
   ssh ubuntu-node "~/sync-and-flash.sh"
   ```

---

### Task 5: Updating Home Assistant Dashboard

**Dashboard location:** `homeassistant/dashboards/bed_presence_dashboard.yaml`

**Workflow:**

1. **Edit in Codespace:**
   ```bash
   nano homeassistant/dashboards/bed_presence_dashboard.yaml
   # Make changes to dashboard YAML

   git add .
   git commit -m "dashboard: add new card for debounce timers"
   git push origin main
   ```

2. **Update in Home Assistant:**
   - Option A: Copy entire YAML and paste in HA dashboard editor
   - Option B: If using file-based dashboard:
     ```bash
     ssh ubuntu-node
     cd ~/bed-presence-sensor
     git pull origin main
     # Copy dashboard YAML to HA config directory
     ```

**No device reflashing required** - dashboard is Home Assistant configuration only.

---

## Quick Iteration Patterns

### Pattern 1: Rapid C++ Testing (No Hardware)

**Use case:** Testing algorithm changes without flashing to device.

```bash
# In Codespace
cd esphome

# Make changes to bed_presence.cpp or bed_presence.h
nano custom_components/bed_presence_engine/bed_presence.cpp

# Run unit tests (fast, no hardware needed)
platformio test -e native

# Iterate until tests pass
# Then commit and flash to device when ready
```

**Advantages:**
- Fast feedback loop (tests run in <5 seconds)
- No hardware required
- Validates logic before flashing

---

### Pattern 2: Quick Threshold Tuning (No Code Changes)

**Use case:** Finding optimal threshold values through experimentation.

```bash
# No Codespace needed - just use Home Assistant UI

# 1. Open HA → Settings → Devices → Bed Presence Detector
# 2. Adjust k_on and k_off sliders
# 3. Test sensor behavior
# 4. Monitor text_sensor.presence_state_reason for z-scores
# 5. Iterate until behavior is optimal
# 6. Document final values in notes

# When optimal values found, update defaults in code:
# (In Codespace) nano esphome/custom_components/bed_presence_engine/bed_presence.h
# Update k_on_ and k_off_ default values
# Commit and flash
```

---

### Pattern 3: Multi-Step Baseline Calibration

**Use case:** Collecting new baseline statistics after sensor repositioning.

```bash
# Step 1: Collect data (on ubuntu-node)
ssh ubuntu-node "cd ~/bed-presence-sensor && python3 scripts/collect_baseline.py"
# Record μ and σ values from output

# Step 2: Update code (in Codespace)
# nano esphome/custom_components/bed_presence_engine/bed_presence.h
# Update mu_still_ and sigma_still_ values

# Step 3: Flash (on ubuntu-node)
ssh ubuntu-node "~/sync-and-flash.sh"

# Step 4: Validate (in Home Assistant)
# Check text_sensor.presence_state_reason
# Verify z-score near 0 for empty bed
```

---

## Pre-Flight Checklist

Before flashing firmware, verify:

- [ ] **Secrets copied from ubuntu-node** (if working in Codespace)
  ```bash
  ssh ubuntu-node "cat ~/bed-presence-sensor/esphome/secrets.yaml" > esphome/secrets.yaml
  ```

- [ ] **Code compiles successfully**
  ```bash
  cd esphome && esphome compile bed-presence-detector.yaml
  ```

- [ ] **Unit tests pass**
  ```bash
  cd esphome && platformio test -e native
  ```

- [ ] **Git changes committed**
  ```bash
  git status  # Should be clean or committed
  ```

- [ ] **Baseline values are correct** in `bed_presence.h`

- [ ] **Device connected to ubuntu-node via USB**
  ```bash
  ssh ubuntu-node "ls /dev/ttyACM*"  # Should show device
  ```

---

## Common Issues and Solutions

### Issue: "Device not found" when flashing

**Cause:** USB device not connected or incorrect device path.

**Solution:**
```bash
ssh ubuntu-node
ls /dev/ttyACM*   # Check device path
ls /dev/ttyUSB*   # Alternative device path

# Use correct path in flash command
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

---

### Issue: "secrets.yaml not found" in Codespace

**Cause:** Secrets file not copied from ubuntu-node.

**Solution:**
```bash
# Copy secrets FROM ubuntu-node TO Codespace
ssh ubuntu-node "cat ~/bed-presence-sensor/esphome/secrets.yaml" > esphome/secrets.yaml
```

---

### Issue: Git conflicts when pulling on ubuntu-node

**Cause:** Local changes on ubuntu-node conflicting with pushed changes.

**Solution:**
```bash
ssh ubuntu-node
cd ~/bed-presence-sensor

# Option A: Stash local changes
git stash
git pull origin main
git stash pop  # Re-apply local changes

# Option B: Discard local changes (if not needed)
git reset --hard
git pull origin main
```

---

## Summary: Two-Machine Workflow Checklist

1. **Code editing**: Always in Codespace
2. **Compilation testing**: Can be done in Codespace
3. **Unit testing**: Can be done in Codespace
4. **Committing**: Always in Codespace
5. **Flashing**: Always on ubuntu-node (physical USB access)
6. **Baseline collection**: Always on ubuntu-node (network access to HA)
7. **Secrets management**: Ubuntu-node is source of truth

**Remember:** Codespaces = code editing, ubuntu-node = hardware interaction.

---

## Additional Resources

- [Architecture Documentation](ARCHITECTURE.md) - Technical details
- [Hardware Setup Guide](HARDWARE_SETUP.md) - Physical device configuration
- [Troubleshooting Guide](troubleshooting.md) - Common issues and solutions
- [Ubuntu Node Setup](ubuntu-node-setup.md) - SSH and network configuration
