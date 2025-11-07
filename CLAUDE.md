# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a bed presence detection system for Home Assistant using an ESP32 microcontroller and LD2410 mmWave radar sensor. The project implements a **3-phase development roadmap** defined in `docs/presence-engine-spec.md`.

**Current Status: Phase 2 DEPLOYED** ✅

The repository contains:
- **Phase 2 presence engine** - State machine with temporal filtering ✅ **DEPLOYED**
- **Hardware deployment** - M5Stack + LD2410 connected to Home Assistant ✅ **OPERATIONAL**
- **Baseline calibration** - Real sensor statistics collected and flashed ✅ **CALIBRATED**
- **C++ unit tests** - Comprehensive test suite for Phase 2 logic ✅ **14 TESTS PASSING**
- **Home Assistant integration** - Dashboard and blueprints ✅ **FULLY FUNCTIONAL**
- **Development infrastructure** - Codespace ↔ Ubuntu-node workflow ✅ **DOCUMENTED & CONFIGURED**

## Implementation Roadmap

The project follows a 3-phase development plan documented in `docs/presence-engine-spec.md`:

### Phase 1: Z-Score Based Detection ✅ COMPLETE
- Simple statistical presence detection using z-scores
- Immediate state transitions (no debouncing - intentionally "twitchy")
- Hysteresis via separate ON/OFF threshold multipliers (`k_on` > `k_off`)
- Runtime tunable thresholds via Home Assistant
- **Status**: Completed 2025-11-06

### Phase 2: State Machine + Debouncing ✅ DEPLOYED
- 4-state machine (IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF)
- Temporal filtering with configurable debounce timers (3s on, 5s off, 30s absolute clear delay)
- Eliminates false positives/negatives from Phase 1
- Runtime tunable debounce timers via Home Assistant
- **Status**: Deployed to hardware 2025-11-07, fully operational

### Phase 3: Calibration + Environmental Hardening ⏳ PLANNED
- Automated baseline calibration via Home Assistant services
- Distance windowing to ignore specific zones
- MAD (Median Absolute Deviation) statistical analysis
- **Status**: Service stubs exist but no implementation

## Monorepo Structure

```
/workspaces/bed-presence-sensor/
├── esphome/                      # ESP32 firmware
│   ├── custom_components/
│   │   └── bed_presence_engine/  # Phase 2 C++ implementation
│   │       ├── __init__.py       # ESPHome component registration
│   │       ├── binary_sensor.py  # ESPHome config schema (with debounce params)
│   │       ├── bed_presence.h    # C++ header (state machine + debouncing)
│   │       └── bed_presence.cpp  # C++ implementation (162 lines)
│   ├── packages/                 # Modular YAML configuration
│   │   ├── hardware_m5stack_ld2410.yaml    # UART, GPIO, LD2410 sensor
│   │   ├── presence_engine.yaml            # k_on/k_off + debounce timer entities
│   │   ├── services_calibration.yaml       # Placeholder services
│   │   └── diagnostics.yaml                # Device health sensors
│   ├── test/
│   │   └── test_presence_engine.cpp        # 14 comprehensive unit tests (355 lines, Phase 2)
│   ├── bed-presence-detector.yaml          # Main ESPHome entry point
│   ├── platformio.ini                      # PlatformIO test configuration
│   └── secrets.yaml.example                # Template for WiFi credentials
├── homeassistant/                # Home Assistant configuration
│   ├── blueprints/automation/
│   │   └── bed_presence_automation.yaml    # Automation blueprint
│   ├── dashboards/
│   │   └── bed_presence_dashboard.yaml     # 4-view Lovelace dashboard
│   └── configuration_helpers.yaml.example  # Required helper entities
├── tests/e2e/                    # Python E2E tests
│   ├── test_calibration_flow.py            # 9 test functions (INCOMPLETE)
│   └── requirements.txt                    # MISSING hass_ws dependency
├── docs/                         # Documentation
│   ├── presence-engine-spec.md             # **SOURCE OF TRUTH** - 3-phase spec
│   ├── phase1-hardware-setup.md            # Hardware setup guide for Phase 1
│   ├── phase2-completion-steps.md          # Phase 2 deployment and testing guide
│   ├── quickstart.md, calibration.md, faq.md, troubleshooting.md
│   └── assets/                             # EMPTY placeholder files (0 bytes)
├── hardware/mounts/              # 3D printable parts
│   └── m5stack_side_mount.stl              # EMPTY placeholder (0 bytes)
├── .github/workflows/            # CI/CD
│   ├── compile_firmware.yml                # ESPHome compile + PlatformIO tests
│   └── lint_yaml.yml                       # YAML validation
└── .devcontainer/                # GitHub Codespaces setup
    ├── devcontainer.json                   # Auto-installs tools
    └── Dockerfile                          # Python 3.11 + Claude CLI
```

## Development Commands

### ESPHome Firmware (run from `esphome/` directory)

**Compile firmware:**
```bash
cd esphome
esphome compile bed-presence-detector.yaml
```

**Run C++ unit tests (fully implemented):**
```bash
cd esphome
platformio test -e native
```
14 tests covering z-score calculation, state transitions, hysteresis, edge cases.

**Flash to device:**
```bash
cd esphome
esphome run bed-presence-detector.yaml
```

### End-to-End Tests (INCOMPLETE - see Known Issues)

```bash
cd tests/e2e
pip install -r requirements.txt  # Missing hass_ws dependency
export HA_URL="ws://your-ha-instance:8123/api/websocket"
export HA_TOKEN="your-long-lived-access-token"
pytest
```

## Phase 2 Implementation Details

### Core Algorithm (bed_presence.cpp:49-128)

The Phase 2 presence engine implements state machine with temporal filtering:

1. **Input**: `ld2410_still_energy` sensor value
2. **Z-Score Calculation**: `z_still = (energy - μ_still) / σ_still`
   - ✅ **CALIBRATED** baseline statistics: `μ_still = 6.7`, `σ_still = 3.5`
   - Baseline collected: 2025-11-06 with empty bed (30 samples over 60 seconds)
   - Location: New sensor position looking at bed
3. **State Machine Processing**:
   - **IDLE**: Detects `z_still >= k_on` → enters DEBOUNCING_ON
   - **DEBOUNCING_ON**: Waits 3 seconds with sustained high signal → PRESENT (or aborts to IDLE)
   - **PRESENT**: Tracks high confidence signals, waits for absolute clear delay + low signal → DEBOUNCING_OFF
   - **DEBOUNCING_OFF**: Waits 5 seconds with sustained low signal → IDLE (or aborts to PRESENT)
4. **Temporal Filtering**: Debounce timers prevent rapid oscillation

### Class Structure (bed_presence.h:29-80)

```cpp
enum State {
  IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF
};

class BedPresenceEngine : public Component, public binary_sensor::BinarySensor {
  // Configuration (set from ESPHome YAML)
  float k_on_{9.0f};   // ON threshold multiplier
  float k_off_{4.0f};  // OFF threshold multiplier

  // ✅ CALIBRATED baseline (collected 2025-11-06, empty bed)
  float mu_still_{6.7f};    // Mean still energy (semantic fix from mu_move_)
  float sigma_still_{3.5f}; // Std dev still energy (semantic fix from sigma_move_)

  // Phase 2: State machine
  State current_state_{IDLE};
  unsigned long debounce_start_time_{0};
  unsigned long last_high_confidence_time_{0};
  unsigned long on_debounce_ms_{3000};
  unsigned long off_debounce_ms_{5000};
  unsigned long abs_clear_delay_ms_{30000};

  // Runtime updaters called from Home Assistant
  void update_k_on(float k);
  void update_k_off(float k);
  void update_on_debounce_ms(unsigned long ms);
  void update_off_debounce_ms(unsigned long ms);
  void update_abs_clear_delay_ms(unsigned long ms);
};
```

**Phase 2 Characteristics:**
- ✅ Z-score normalization for statistical significance
- ✅ Hysteresis to prevent rapid oscillation
- ✅ Runtime tunable thresholds AND debounce timers
- ✅ State reason tracking with debounce timing info
- ✅ 4-state machine enum (IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF)
- ✅ Debounce timers (3s on, 5s off, 30s absolute clear delay)
- ✅ Temporal filtering prevents false positives/negatives

### ESPHome Configuration (presence_engine.yaml)

**Entities exposed to Home Assistant:**

```yaml
binary_sensor:
  - platform: bed_presence_engine
    name: "Bed Occupied"                    # binary_sensor.bed_occupied
    id: bed_occupied
    k_on: 9.0
    k_off: 4.0
    on_debounce_ms: 3000       # Phase 2: 3 seconds
    off_debounce_ms: 5000      # Phase 2: 5 seconds
    abs_clear_delay_ms: 30000  # Phase 2: 30 seconds
    state_reason:
      name: "Presence State Reason"         # text_sensor.presence_state_reason
      id: presence_state_reason             # Shows z-score and debounce timing

number:
  - platform: template
    name: "k_on (ON Threshold Multiplier)"  # number.k_on_on_threshold_multiplier
    id: k_on_input
    min_value: 0.0
    max_value: 15.0
    step: 0.1
    initial_value: 9.0
    restore_value: true                      # Persists across reboots

  - platform: template
    name: "k_off (OFF Threshold Multiplier)" # number.k_off_off_threshold_multiplier
    id: k_off_input
    min_value: 0.0
    max_value: 15.0
    step: 0.1
    initial_value: 4.0
    restore_value: true
```

**Note**: Phase 2 adds 3 new number entities for debounce timer configuration: `on_debounce_ms`, `off_debounce_ms`, `abs_clear_delay_ms`.

## C++ Unit Tests Status

**Location**: `esphome/test/test_presence_engine.cpp`

**Status**: ✅ FULLY IMPLEMENTED (355 lines, 14 test cases, Phase 2)

The tests create a `SimplePresenceEngine` class that replicates Phase 2 state machine logic without ESPHome dependencies. All tests pass and validate:

- ✅ Z-score calculation accuracy
- ✅ Initial state (IDLE with binary sensor OFF)
- ✅ Debounced transitions (3s on, 5s off)
- ✅ Debounce abort conditions (signal lost during debounce)
- ✅ Absolute clear delay (prevents premature clearing)
- ✅ High confidence timestamp tracking
- ✅ State machine behavior (4 states: IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF)
- ✅ Dynamic threshold and timer updates
- ✅ State reason tracking with debounce timing
- ✅ Edge cases (zero sigma, negative energy, very large values)

**Run tests**: `cd esphome && platformio test -e native`

## Known Issues & Discrepancies

### 1. Calibration Services Are Placeholders

**File**: `esphome/packages/services_calibration.yaml`

**Status**: Services are defined and callable, but implementation just logs messages.

```yaml
esphome:
  on_boot:
    - lambda: |-
        // TODO Phase 3: Implement actual baseline data collection
        // TODO Phase 3: Implement MAD-based threshold calculation
```

**Available services**:
- `esphome.bed_presence_detector_start_calibration` (placeholder)
- `esphome.bed_presence_detector_stop_calibration` (placeholder)
- `esphome.bed_presence_detector_reset_to_defaults` (functional - resets k_on/k_off to 4.0/2.0)

### 2. Hardware Assets Are Empty Placeholders

**Files** (all 0 bytes):
- `hardware/mounts/m5stack_side_mount.stl`
- `docs/assets/wiring_diagram.png`
- `docs/assets/demo.gif`

**Hardware Configuration**:
- **Device**: M5Stack Basic (ESP32-D0WDQ6-V3, 16MB Flash, MAC: 08:b6:1f:a5:6e:68)
- **Sensor**: LD2410 mmWave radar (UART GPIO16/17, 256000 baud, firmware 2.44.25070917)
- **Network**: WiFi connected to TP-Link_BECC at IP 192.168.0.180
- **Home Assistant**: API connected to 192.168.0.148, all entities available
- **Location**: New sensor position looking at bed

**Baseline Calibration** (recalibrated 2025-11-06 18:39:42):
- **Mean (μ):** 6.67% still energy (empty bed, rounded to 6.7%)
- **Std Dev (σ):** 3.51% (rounded to 3.5%)
- **Samples:** 30 over 60 seconds
- **Conditions:** Empty bed, door closed, minimal movement
- **ON Threshold:** 6.7 + (9.0 × 3.5) = 38.2% (z > 9.0)
- **OFF Threshold:** 6.7 + (4.0 × 3.5) = 20.7% (z < 4.0)

**Phase 1 Validation Results** (tested 2025-11-06):
- ✅ Empty bed test: 3.0% still energy, z=-1.06, state=OFF
- ✅ Occupied bed test: 64.0% still energy, z=16.37, state=ON
- ✅ Thresholds tuned to k_on=9.0, k_off=4.0 for reduced false positives
- ✅ Wider hysteresis gap (17.5%) reduces oscillation
- ✅ Phase 1 **COMPLETE AND VALIDATED**

**Known Phase 1 Limitation:**
- ⚠️ Sensor remains "twitchy" with rapid state changes due to no debouncing (Phase 2 feature)
- Tuned thresholds reduce but don't eliminate oscillation during movement in bed

## Network Access from Codespaces

**⚠️ IMPORTANT**: GitHub Codespaces runs on GitHub's cloud infrastructure and **cannot directly access** your local Home Assistant instance at `192.168.0.148:8123`.

### Accessing Home Assistant Web UI from Codespaces

To view the Home Assistant web interface from your Codespace browser, use **SSH port forwarding**:

```bash
# In Codespace terminal
ssh -L 8123:192.168.0.148:8123 ubuntu-node
```

This creates a tunnel that forwards:
- Local port 8123 (in Codespace) → 192.168.0.148:8123 (Home Assistant on ubuntu-node's network)

**Then access Home Assistant at**: `http://localhost:8123` in your Codespace browser

**Keep the SSH session open** while browsing - closing it will break the tunnel.

### Running Python Scripts That Need Home Assistant API

Scripts like `collect_baseline.py` need to connect to the Home Assistant API. You have **two options**:

**Option 1: Run Directly on Ubuntu-node** (RECOMMENDED)
```bash
ssh ubuntu-node "cd ~/bed-presence-sensor && python3 scripts/collect_baseline.py"
```
- Script runs on ubuntu-node where HA is on localhost
- No network issues, fastest performance
- Uses `.env.local` on ubuntu-node

**Option 2: Run in Codespace with SSH Tunnel**
```bash
# Terminal 1: Create SSH tunnel
ssh -L 8123:192.168.0.148:8123 ubuntu-node

# Terminal 2: Run script with localhost URL
cd /workspaces/bed-presence-sensor
python3 scripts/collect_baseline.py
# Script will use HA_URL=http://localhost:8123 from .env.local (if configured)
```

**Note**: Most development workflows run scripts directly on ubuntu-node to avoid networking complexity.

## Development Workflow

**⚠️ CRITICAL**: This project uses a **two-location workflow**. Code changes are made in **GitHub Codespace**, synced via **git**, and firmware is flashed from **ubuntu-node** (which has physical USB access to the M5Stack device).

### Codespace ↔ Ubuntu-node Workflow Architecture

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

### Workflow Key Principles

1. **Source of Truth**: Git repository (GitHub)
2. **Code Development**: GitHub Codespace (edit, test compilation, commit, push)
3. **Firmware Flashing**: Ubuntu-node (git pull, then flash to device via USB)
4. **Secrets Management**: Ubuntu-node is source of truth for `secrets.yaml` (gitignored)

**See `docs/ubuntu-node-setup.md` for comprehensive workflow documentation including:**
- Standard code change workflow (edit → commit → sync → flash)
- Quick iteration patterns
- Baseline calibration multi-step process
- Secrets management (copy secrets FROM ubuntu-node TO Codespace, never reverse)
- Pre-flight checklist to prevent common mistakes
- Common sync issues and fixes
- DO/DON'T best practices

### Standard Workflow: Code Changes

**In Codespace** (you are here):
```bash
# 1. Edit code
nano esphome/custom_components/bed_presence_engine/bed_presence.h

# 2. Test compilation (optional but recommended)
cd esphome
esphome compile bed-presence-detector.yaml
platformio test -e native  # Run C++ unit tests

# 3. Commit and push
git add esphome/custom_components/bed_presence_engine/bed_presence.h
git commit -m "Description of changes"
git push origin main
```

**On ubuntu-node** (via SSH):
```bash
# 4. Sync and flash using helper script (RECOMMENDED)
ssh ubuntu-node
~/sync-and-flash.sh
```

**Or manually**:
```bash
ssh ubuntu-node
cd ~/bed-presence-sensor
git pull origin main  # Sync from GitHub
cd esphome
source ~/esphome-venv/bin/activate
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

### Ubuntu-node Helper Scripts

Two automation scripts are available on ubuntu-node:

**`~/sync-and-flash.sh`** (RECOMMENDED)
- Pulls latest code from GitHub
- Verifies WiFi credentials and baseline values
- Flashes firmware
- **Use after making changes in Codespace**

**`~/flash-firmware.sh`**
- Runs pre-flight checklist
- Flashes existing code without git pull
- **Use when reflashing without code updates**

**Quick reference**: `cat ~/WORKFLOW-README.md` on ubuntu-node

### 1. Firmware Development (Fastest Iteration)

```bash
cd esphome
esphome compile bed-presence-detector.yaml  # Validate syntax
platformio test -e native                   # Run unit tests (fast, no hardware)
```

**Only after tests pass:**
```bash
esphome run bed-presence-detector.yaml      # Flash to device
```

### 2. Home Assistant Configuration

**Prerequisites**:
- ESPHome device successfully connected to Home Assistant
- Device appears in Settings → Devices & Services → ESPHome

**Deploy dashboard**:
1. Copy `homeassistant/dashboards/bed_presence_dashboard.yaml` content
2. Settings → Dashboards → Add Dashboard → Create new dashboard
3. Edit Dashboard → Raw Configuration Editor → Paste YAML
4. ✅ Entity names are now correct for Phase 1 (no manual fixes needed)

**Deploy automation blueprint**:
1. Copy `homeassistant/blueprints/automation/bed_presence_automation.yaml`
2. Place in `<config>/blueprints/automation/` directory
3. Settings → Automations & Scenes → Create Automation → Use Blueprint

### 3. Integration Testing

**Prerequisites**:
- ✅ E2E test dependencies now included in `requirements.txt`
- Live Home Assistant instance with device connected
- Long-lived access token

```bash
cd tests/e2e
pip install -r requirements.txt  # Install dependencies including homeassistant-api
export HA_URL="ws://homeassistant.local:8123/api/websocket"
export HA_TOKEN="your-token"
pytest  # Phase 3 tests will be skipped automatically
```

## Common Development Tasks

### Modifying Phase 1 Thresholds

**Default values** (bed_presence.h:49-50):
```cpp
float k_on_{4.0f};   // Turn ON when z > 4.0 (4 std deviations)
float k_off_{2.0f};  // Turn OFF when z < 2.0 (2 std deviations)
```

**To change defaults**:
1. Edit `esphome/custom_components/bed_presence_engine/bed_presence.h`
2. Update `k_on_` and/or `k_off_` values
3. Also update `initial_value` in `esphome/packages/presence_engine.yaml` (lines 24, 40)
4. Recompile and flash

**At runtime** (no reflash needed):
- Adjust via Home Assistant UI: Settings → Devices → Bed Presence Detector
- Changes persist across reboots (`restore_value: true`)

### Updating Baseline Statistics (Phase 1 Manual Calibration)

**Current values** (bed_presence.h:40-47) ✅ **CALIBRATED** (2025-11-06):
```cpp
// Baseline calibration collected on 2025-11-06 18:39:42
// Location: New sensor position looking at bed
// Conditions: Empty bed, door closed, minimal movement
// Statistics: mean=6.67%, stdev=3.51%, n=30 samples over 60 seconds
float mu_move_{6.7f};     // Mean still energy (empty bed)
float sigma_move_{3.5f};  // Std dev still energy (empty bed)
float mu_stat_{6.7f};     // Same as mu_move_ for Phase 1
float sigma_stat_{3.5f};  // Same as sigma_move_ for Phase 1
```

**To re-calibrate** (if sensor position changes or false positives/negatives occur):
1. Follow `docs/phase1-completion-steps.md` Step 2 "Baseline Data Collection"
2. Use the collection script: `ssh ubuntu-node "cd ~/bed-presence-sensor && python3 scripts/collect_baseline.py"`
3. Record mean (μ) and std dev (σ) from script output
4. **In Codespace**: Edit `bed_presence.h` with new values
5. **In Codespace**: Commit and push: `git add . && git commit -m "Recalibrate baseline" && git push origin main`
6. **On ubuntu-node**: Sync and flash: `ssh ubuntu-node "~/sync-and-flash.sh"`

**Note**: Phase 3 will automate this process via calibration services.

### Implementing Phase 3 (Automated Calibration)

**Required changes**:

1. **bed_presence.h**: Add calibration state
   ```cpp
   bool is_calibrating_{false};
   std::vector<float> calibration_samples_;
   unsigned long calibration_start_time_{0};
   ```

2. **services_calibration.yaml**: Replace placeholder lambdas with:
   - `start_calibration`: Set `is_calibrating_ = true`, start timer
   - In `loop()`: Collect energy samples while calibrating
   - `stop_calibration`: Calculate μ and σ using MAD method, update thresholds

3. **Calculate statistics**: Implement MAD (Median Absolute Deviation) for robust outlier handling
   ```cpp
   float median = calculate_median(samples);
   float mad = calculate_mad(samples, median);
   float sigma = 1.4826 * mad;  // MAD to std dev conversion
   ```

4. **Update number entities**: Publish new k_on/k_off values via `id(k_on_input).publish_state(value)`

**Reference**: See `docs/presence-engine-spec.md` Phase 3 requirements.

## Testing Strategy

### Unit Tests (C++ - Fast, No Hardware)

**What they test**: Phase 2 z-score logic and state machine in isolation

**Run**: `cd esphome && platformio test -e native`

**When to use**:
- Verifying algorithm changes
- Testing edge cases
- Regression testing during refactoring
- Fast feedback loop (runs in <5 seconds)

**Coverage**: 14 tests validate all Phase 2 behavior (z-score, state machine, debouncing)

### ESPHome Compilation (Syntax Check)

**What it tests**: YAML validity, C++ compilation, ESPHome integration

**Run**: `cd esphome && esphome compile bed-presence-detector.yaml`

**When to use**:
- After modifying YAML configuration
- After changing C++ component code
- Before flashing to device
- CI/CD validation

### E2E Integration Tests (Python - Requires Hardware)

**What they test**: Full system integration with live Home Assistant

**Run**: `cd tests/e2e && pytest` (after fixing dependencies)

**When to use**:
- Verifying Home Assistant integration
- Testing calibration workflows
- Validating entity names and services
- Acceptance testing

**Status**: Framework exists but incomplete (see Known Issues #2)

### Manual Testing with Hardware

**Required for**:
- Validating actual LD2410 sensor readings
- Testing presence detection accuracy
- Calibrating baseline statistics
- Real-world reliability testing

**Follow**: `docs/phase1-hardware-setup.md` for testing procedures

## CI/CD Workflows

### compile_firmware.yml (Functional)

**Triggers**: Push/PR to main affecting `esphome/**`

**Jobs**:
1. **compile**: Installs ESPHome, compiles `bed-presence-detector.yaml`
2. **test**: Installs PlatformIO, runs `platformio test -e native`

**Status**: ✅ Both jobs functional and passing

### lint_yaml.yml (Functional)

**Triggers**: Push/PR to main

**Jobs**: Runs `yamllint` on `esphome/` and `homeassistant/` directories

**Status**: ✅ Functional

## Environment Setup

### GitHub Codespaces (Recommended)

**Setup**: Automatic via `.devcontainer/devcontainer.json`

**Includes**:
- Python 3.11
- ESPHome CLI
- PlatformIO
- pytest, pytest-asyncio
- yamllint, black
- Claude CLI

**Launch**: Click "Code" → "Codespaces" → "Create codespace on main"

### Local Development

**Requirements**:
- Python 3.11+
- pip

**Install tools**:
```bash
pip install esphome platformio pytest pytest-asyncio yamllint black
```

## Secrets Management

**⚠️ IMPORTANT**: This project uses **TWO different secrets files** for different purposes. Understanding the distinction is critical.

### Quick Reference: Which Secrets File Do I Need?

| File | Location | Purpose | Contains | Used By | Source of Truth |
|------|----------|---------|----------|---------|-----------------|
| **`.env.local`** | Project root | Python scripts access HA API | `HA_URL`, `HA_WS_URL`, `HA_TOKEN` | `collect_baseline.py`, E2E tests, monitoring scripts | **Ubuntu-node** |
| **`secrets.yaml`** | `esphome/` | WiFi credentials embedded in firmware | `wifi_ssid`, `wifi_password`, `api_encryption_key`, `ota_password` | ESPHome compiler | **Ubuntu-node** |

**Key Points:**
- These files are **completely unrelated** and serve different purposes
- Both are gitignored (contain sensitive credentials)
- **Ubuntu-node is the source of truth** for both files
- When working in Codespace, copy FROM ubuntu-node TO Codespace (never reverse)

### File 1: `.env.local` - Home Assistant API Access

**Purpose**: Python scripts need to connect to the Home Assistant REST/WebSocket API to read sensor values or control entities.

**Location**: `/workspaces/bed-presence-sensor/.env.local` (project root, gitignored)

**Contains**:
```bash
# Home Assistant Connection Configuration
HA_URL=http://192.168.0.148:8123
HA_WS_URL=ws://192.168.0.148:8123/api/websocket
HA_TOKEN=your-long-lived-access-token-here
```

**Used by**:
- `scripts/collect_baseline.py` - Collects LD2410 sensor readings from HA
- `tests/e2e/test_calibration_flow.py` - Integration tests
- Any script that needs to read/write HA entities

**How to get a Long-Lived Access Token**:
1. Open Home Assistant web UI → Profile (bottom left)
2. Scroll to "Long-Lived Access Tokens"
3. Click "Create Token"
4. Name: "Bed Presence Development"
5. Copy the token (shown only once!)

**On Ubuntu-node** (source of truth):
```bash
# Already exists at ~/bed-presence-sensor/.env.local
cat ~/bed-presence-sensor/.env.local
```

**On Codespace** (copy from ubuntu-node):
```bash
# Copy .env.local FROM ubuntu-node TO Codespace
ssh ubuntu-node "cat ~/bed-presence-sensor/.env.local" > /workspaces/bed-presence-sensor/.env.local

# Verify
cat /workspaces/bed-presence-sensor/.env.local
```

**Never commit** `.env.local` to git (contains sensitive HA_TOKEN).

### File 2: `esphome/secrets.yaml` - WiFi Credentials for Firmware

**Purpose**: ESP32 device needs WiFi credentials to connect to your local network. These are embedded into the firmware binary during compilation.

**Location**: `/workspaces/bed-presence-sensor/esphome/secrets.yaml` (gitignored)

**Contains**:
```yaml
wifi_ssid: "TP-Link_BECC"
wifi_password: "your_wifi_password"
api_encryption_key: "base64-encoded-key-here"
ota_password: "your_ota_password"
```

**Used by**:
- ESPHome compiler (`esphome compile bed-presence-detector.yaml`)
- Credentials are burned into firmware during compilation
- ESP32 device uses these to connect to WiFi and Home Assistant

**On Ubuntu-node** (source of truth):
```bash
# Already exists at ~/bed-presence-sensor/esphome/secrets.yaml
cat ~/bed-presence-sensor/esphome/secrets.yaml
```

**On Codespace** (copy from ubuntu-node):
```bash
# Copy secrets.yaml FROM ubuntu-node TO Codespace
ssh ubuntu-node "cat ~/bed-presence-sensor/esphome/secrets.yaml" > /workspaces/bed-presence-sensor/esphome/secrets.yaml

# Verify
cat /workspaces/bed-presence-sensor/esphome/secrets.yaml
```

**Never commit** `secrets.yaml` to git (contains WiFi password).

### Why Two Files?

**Context 1: Python Scripts Running on a Computer**
- Scripts run on ubuntu-node or Codespace (regular computers with network access)
- Need to connect TO Home Assistant API over HTTP/WebSocket
- Use `.env.local` with `HA_URL` and `HA_TOKEN`

**Context 2: ESP32 Firmware Running on Embedded Device**
- Firmware runs on M5Stack ESP32 (embedded microcontroller)
- Needs WiFi credentials to join network and reach Home Assistant
- Uses `secrets.yaml` credentials embedded during compilation

**These are completely separate contexts with different tools and different needs.**

### Troubleshooting

**"collect_baseline.py fails with connection error"**
- Check that `.env.local` exists and has correct `HA_URL` and `HA_TOKEN`
- From Codespace: Use SSH tunnel (see "Network Access from Codespaces") OR run on ubuntu-node
- Verify token is valid in HA → Profile → Long-Lived Access Tokens

**"ESPHome compile fails - secrets.yaml not found"**
- Copy `secrets.yaml` from ubuntu-node: `ssh ubuntu-node "cat ~/bed-presence-sensor/esphome/secrets.yaml" > esphome/secrets.yaml`
- Or create from template: `cp esphome/secrets.yaml.example esphome/secrets.yaml` and fill in values

**"Which file do I edit?"**
- Changing WiFi network → Edit `esphome/secrets.yaml` (then recompile firmware)
- Changing HA access for scripts → Edit `.env.local` (no recompile needed)

## Key Files Reference

### Critical Files for Phase 1 Development

- `docs/presence-engine-spec.md` - **SOURCE OF TRUTH** for 3-phase roadmap
- `esphome/custom_components/bed_presence_engine/bed_presence.cpp` - Core z-score logic (96 lines)
- `esphome/custom_components/bed_presence_engine/bed_presence.h` - Class definition (66 lines)
- `esphome/packages/presence_engine.yaml` - k_on/k_off entities and lambdas
- `esphome/test/test_presence_engine.cpp` - Unit tests (219 lines, 14 tests)

### ~~Files Needing Updates (Known Issues)~~ ✅ ALL FIXED

- ✅ `homeassistant/dashboards/bed_presence_dashboard.yaml` - Entity names corrected, Phase 2 features removed
- ✅ `tests/e2e/requirements.txt` - Home Assistant WebSocket library added
- ✅ `tests/e2e/test_calibration_flow.py` - Entity names and default values corrected, Phase 3 tests skipped

### Placeholder Files (Empty - 0 bytes)

- `hardware/mounts/m5stack_side_mount.stl`
- `docs/assets/wiring_diagram.png`
- `docs/assets/demo.gif`

### Files Safe to Modify

- Threshold defaults: `bed_presence.h` (lines 49-50), `presence_engine.yaml` (lines 24, 40)
- GPIO pins: `packages/hardware_m5stack_ld2410.yaml` (for different boards)
- Baseline statistics: `bed_presence.h` (lines 43-46) - update after calibration
- Dashboard layout: `bed_presence_dashboard.yaml` (entity names now correct for Phase 1)

### Files Requiring Careful Changes

- `bed_presence.cpp` - Core presence detection logic
- `binary_sensor.py` - ESPHome component schema (breaking changes affect YAML config)
- `__init__.py` - Component registration (only change for Phase 2/3 features)

## Troubleshooting

### ESPHome Compilation Errors

**Error**: `Platform 'bed_presence_engine' not found`
- **Cause**: ESPHome can't find custom component
- **Fix**: Ensure `custom_components/bed_presence_engine/` is in `esphome/` directory
- **Verify**: `ls esphome/custom_components/bed_presence_engine/` shows 4 files

**Error**: `'ld2410_still_energy' not found`
- **Cause**: LD2410 sensor not properly configured
- **Fix**: Check `packages/hardware_m5stack_ld2410.yaml` has `sensor:` section with `ld2410_still_energy`

### PlatformIO Test Errors

**Error**: `No tests found`
- **Cause**: Test file not detected
- **Fix**: Ensure `test/test_presence_engine.cpp` exists
- **Verify**: `platformio test -e native --list-tests` shows tests

**Error**: `Assertion failed`
- **Cause**: Unit test logic failure (indicates code bug)
- **Fix**: Read test output to identify which test failed and why
- **Debug**: Add printf statements in `SimplePresenceEngine` class

### Home Assistant Integration Issues

**Error**: `Entity not found: number.k_on_on_threshold_multiplier`
- **Cause**: ESPHome device not yet connected to Home Assistant or entity names don't match
- **Fix**: Ensure device is connected and entity names match Phase 1 configuration

**Error**: Device not auto-discovered
- **Cause**: WiFi/API configuration issue
- **Fix**: Check `secrets.yaml` credentials, verify same network as HA
- **Debug**: View ESPHome device logs in HA: Settings → Devices → Bed Presence Detector → Logs

**Error**: Service not available
- **Cause**: Calibration services are placeholders in Phase 1
- **Expected**: Services exist but only log messages (Phase 3 will implement)

### Phase 2 Presence Detection Issues

**Problem**: Sensor always reports vacant
- **Cause**: Baseline statistics (μ, σ) don't match actual sensor readings
- **Fix**: Follow `docs/phase1-hardware-setup.md` to collect baseline data
- **Debug**: Check `sensor.ld2410_still_energy` values in HA
- **Verify**: Calculate z-score manually: `(energy - mu_still) / sigma_still` should be >k_on when occupied

**Problem**: Sensor still twitchy after Phase 2 upgrade
- **Cause**: Debounce timers too short
- **Fix**: Increase `on_debounce_ms` to 5000 (5 seconds), `abs_clear_delay_ms` to 60000 (60 seconds)
- **Alternative**: Adjust `k_on`/`k_off` thresholds for wider hysteresis

**Problem**: Sensor too slow to respond
- **Cause**: On debounce timer too long
- **Fix**: Decrease `on_debounce_ms` to 2000 (2 seconds) via HA UI
- **Debug**: Check logs for state transitions (IDLE → DEBOUNCING_ON → PRESENT)

**Problem**: Sensor clears too quickly when lying still
- **Cause**: Absolute clear delay too short
- **Fix**: Increase `abs_clear_delay_ms` to 60000 (60 seconds) or higher
- **Debug**: Check z-score values in `text_sensor.presence_state_reason` during stillness

## Additional Resources

### Documentation

- `docs/presence-engine-spec.md` - Complete 3-phase engineering specification
- `docs/phase1-hardware-setup.md` - Hardware wiring and baseline calibration guide
- `docs/phase2-completion-steps.md` - Phase 2 deployment, testing, and troubleshooting guide
- `docs/quickstart.md` - User-facing quickstart guide
- `docs/calibration.md` - Calibration procedures (describes Phase 2/3 features)
- `docs/troubleshooting.md` - User troubleshooting guide
- `docs/faq.md` - Frequently asked questions

### External References

- ESPHome Documentation: https://esphome.io
- LD2410 Component: https://esphome.io/components/sensor/ld2410.html
- Home Assistant Blueprints: https://www.home-assistant.io/docs/automation/using_blueprints/
- PlatformIO Testing: https://docs.platformio.org/en/latest/advanced/unit-testing/index.html

## Summary of Current State

**✅ Phase 2 DEPLOYED AND OPERATIONAL**:
- ✅ Phase 2 state machine with temporal filtering (**deployed to hardware**)
- ✅ M5Stack + LD2410 hardware (**operational**, connected to Home Assistant)
- ✅ Baseline calibration (**completed**: μ=6.7%, σ=3.5%, collected 2025-11-06)
- ✅ C++ unit tests (14 Phase 2 tests, all passing)
- ✅ ESPHome firmware compilation (successful, no errors)
- ✅ Runtime tunable thresholds AND debounce timers via Home Assistant
- ✅ CI/CD workflows (compile + test)
- ✅ **Codespace ↔ Ubuntu-node workflow** (documented + configured)
- ✅ GitHub Codespaces development environment
- ✅ Comprehensive documentation including Phase 2 deployment guide
- ✅ Home Assistant dashboard with debounce timer controls
- ✅ E2E tests with proper dependencies and entity references
- ✅ **Ubuntu-node helper scripts** (`~/sync-and-flash.sh`, `~/flash-firmware.sh`)

**What Needs Work** (Future Phases):
- ⏳ **Phase 3**: Automated calibration services (planned)
- ⚠️ Hardware assets are empty placeholders (STL, diagrams, demo)
- ⚠️ Calibration services exist but are placeholders (Phase 3 implementation)

**Phase 2 Deployment Status** (Completed 2025-11-07):
1. ✅ **DEPLOYED**: 4-state machine running on hardware (IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF)
2. ✅ **DEPLOYED**: Debounce timers active (3s on, 5s off, 30s absolute clear delay)
3. ✅ **DEPLOYED**: All 7 entities available in Home Assistant (bed_occupied, state_reason, 5 tuning parameters)
4. ✅ **VERIFIED**: State machine transitions observed (DEBOUNCING_ON → PRESENT after 3s)
5. ✅ **VERIFIED**: Temporal filtering eliminating false positives/negatives
6. ✅ **VERIFIED**: Runtime tuning functional via Home Assistant UI
7. ✅ **PHASE 2 COMPLETE** - System is production-ready

**Next Steps**:
- **OPTIONAL**: Tune debounce parameters based on real-world usage patterns
- **FUTURE**: Implement Phase 3 automated calibration

**Hardware Configuration**:
- **Device**: M5Stack Basic (ESP32-D0WDQ6-V3, 16MB Flash)
- **MAC Address**: 08:b6:1f:a5:6e:68
- **WiFi**: TP-Link_BECC @ 192.168.0.180
- **Home Assistant**: Connected @ 192.168.0.148
- **Sensor**: LD2410 (firmware 2.44.25070917, UART GPIO16/17)
- **Location**: New sensor position looking at bed
- **Baseline**: μ=6.7%, σ=3.5% (empty bed, 30 samples, recalibrated 2025-11-06)
- **Thresholds**: ON=38.2% (z>9.0), OFF=20.7% (z<4.0)
