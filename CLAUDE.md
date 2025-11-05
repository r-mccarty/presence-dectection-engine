# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a bed presence detection system for Home Assistant using an ESP32 microcontroller and LD2410 mmWave radar sensor. The project implements a **3-phase development roadmap** defined in `docs/presence-engine-spec.md`.

**Current Status: Phase 1 Implementation (Z-Score Based Detection)**

The repository contains:
- **Phase 1 presence engine** - Simple z-score based statistical detection (IMPLEMENTED)
- **C++ unit tests** - Comprehensive test suite for Phase 1 logic (FULLY IMPLEMENTED)
- **Home Assistant integration** - Dashboard and blueprints (PARTIALLY COMPLETE - see Known Issues)
- **Development infrastructure** - CI/CD, Codespaces environment, documentation (COMPLETE)

## Implementation Roadmap

The project follows a 3-phase development plan documented in `docs/presence-engine-spec.md`:

### Phase 1: Z-Score Based Detection ✅ CURRENT IMPLEMENTATION
- Simple statistical presence detection using z-scores
- Immediate state transitions (no debouncing - intentionally "twitchy")
- Hysteresis via separate ON/OFF threshold multipliers (`k_on` > `k_off`)
- Runtime tunable thresholds via Home Assistant
- **Status**: Core logic fully implemented and tested

### Phase 2: State Machine + Debouncing ⏳ PLANNED
- 4-state machine (IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF)
- Temporal filtering with configurable debounce timers
- Eliminates false positives/negatives from Phase 1
- **Status**: Not yet implemented

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
│   │   └── bed_presence_engine/  # Phase 1 C++ implementation
│   │       ├── __init__.py       # ESPHome component registration
│   │       ├── binary_sensor.py  # ESPHome config schema
│   │       ├── bed_presence.h    # C++ header (z-score logic)
│   │       └── bed_presence.cpp  # C++ implementation (96 lines)
│   ├── packages/                 # Modular YAML configuration
│   │   ├── hardware_m5stack_ld2410.yaml    # UART, GPIO, LD2410 sensor
│   │   ├── presence_engine.yaml            # k_on/k_off entities
│   │   ├── services_calibration.yaml       # Placeholder services
│   │   └── diagnostics.yaml                # Device health sensors
│   ├── test/
│   │   └── test_presence_engine.cpp        # 14 comprehensive unit tests (219 lines)
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

## Phase 1 Implementation Details

### Core Algorithm (bed_presence.cpp:47-77)

The Phase 1 presence engine implements simple z-score based detection:

1. **Input**: `ld2410_still_energy` sensor value
2. **Z-Score Calculation**: `z = (energy - μ) / σ`
   - Hardcoded baseline statistics: `μ_move_ = 100.0`, `σ_move_ = 20.0` (placeholders in code)
   - **IMPORTANT**: These values need manual calibration per `docs/phase1-hardware-setup.md`
3. **Threshold Comparison with Hysteresis**:
   - Turn ON when `z > k_on` (default: 4.0)
   - Turn OFF when `z < k_off` (default: 2.0)
   - Hysteresis zone: `k_off < z < k_on` (state remains unchanged)
4. **Immediate State Changes**: No temporal debouncing (this is Phase 1 behavior)

### Class Structure (bed_presence.h:20-62)

```cpp
class BedPresenceEngine : public Component, public binary_sensor::BinarySensor {
  // Configuration (set from ESPHome YAML)
  float k_on_{4.0f};   // ON threshold multiplier
  float k_off_{2.0f};  // OFF threshold multiplier

  // Hardcoded baseline (user must update these after baseline collection)
  float mu_move_{100.0f};    // Mean moving energy
  float sigma_move_{20.0f};  // Std dev moving energy

  // Simple boolean state (NO state machine in Phase 1)
  bool is_occupied_{false};

  // Runtime updaters called from Home Assistant
  void update_k_on(float k);
  void update_k_off(float k);
};
```

**Phase 1 Characteristics:**
- ✅ Z-score normalization for statistical significance
- ✅ Hysteresis to prevent rapid oscillation
- ✅ Runtime tunable thresholds
- ✅ State reason tracking
- ❌ NO state machine enum (just boolean `is_occupied_`)
- ❌ NO debounce timers (changes are immediate)
- ❌ NO temporal filtering

### ESPHome Configuration (presence_engine.yaml)

**Entities exposed to Home Assistant:**

```yaml
binary_sensor:
  - platform: bed_presence_engine
    name: "Bed Occupied"                    # binary_sensor.bed_occupied
    id: bed_occupied
    k_on: 4.0
    k_off: 2.0
    state_reason:
      name: "Presence State Reason"         # text_sensor.presence_state_reason
      id: presence_state_reason             # Shows z-score values

number:
  - platform: template
    name: "k_on (ON Threshold Multiplier)"  # number.k_on_on_threshold_multiplier
    id: k_on_input
    min_value: 0.0
    max_value: 10.0
    step: 0.1
    initial_value: 4.0
    restore_value: true                      # Persists across reboots

  - platform: template
    name: "k_off (OFF Threshold Multiplier)" # number.k_off_off_threshold_multiplier
    id: k_off_input
    min_value: 0.0
    max_value: 10.0
    step: 0.1
    initial_value: 2.0
    restore_value: true
```

**Note**: Entity names in Phase 1 are `k_on` and `k_off`, NOT `occupied_threshold` or `vacant_threshold`.

## C++ Unit Tests Status

**Location**: `esphome/test/test_presence_engine.cpp`

**Status**: ✅ FULLY IMPLEMENTED (219 lines, 14 test cases)

The tests create a `SimplePresenceEngine` class that replicates Phase 1 logic without ESPHome dependencies. All tests pass and validate:

- ✅ Z-score calculation accuracy
- ✅ Initial state (vacant)
- ✅ Transitions to occupied when `z > k_on`
- ✅ Transitions to vacant when `z < k_off`
- ✅ Hysteresis behavior (state remains in hysteresis zone)
- ✅ No debouncing (immediate transitions)
- ✅ Dynamic threshold updates (`update_k_on()`, `update_k_off()`)
- ✅ State reason tracking
- ✅ Edge cases (zero sigma, negative energy, very large values)

**Run tests**: `cd esphome && platformio test -e native`

**CORRECTION**: The current CLAUDE.md incorrectly states these tests are "structural placeholders". They are fully functional.

## Known Issues & Discrepancies

### 1. Home Assistant Dashboard Entity Mismatches

**File**: `homeassistant/dashboards/bed_presence_dashboard.yaml`

**Problem**: Dashboard references entities that don't exist or have wrong names.

**Configuration View (lines 60-73)** references:
- ❌ `number.occupied_threshold` (does not exist)
- ❌ `number.vacant_threshold` (does not exist)
- ❌ `number.debounce_occupied_ms` (Phase 2 feature, not implemented)
- ❌ `number.debounce_vacant_ms` (Phase 2 feature, not implemented)

**Should reference**:
- ✅ `number.k_on_on_threshold_multiplier`
- ✅ `number.k_off_off_threshold_multiplier`

**Threshold Visualization (lines 42-51)** references:
- ❌ `number.occupied_threshold`
- ❌ `number.vacant_threshold`

**Should reference**:
- ✅ `number.k_on_on_threshold_multiplier`
- ✅ `number.k_off_off_threshold_multiplier`

### 2. E2E Tests Missing Dependency

**File**: `tests/e2e/test_calibration_flow.py`

**Problem**: Imports `hass_ws.HomeAssistantClient` but `hass_ws` is not in `requirements.txt`.

**Line 15**: `from hass_ws import HomeAssistantClient`

**requirements.txt** only has:
```
pytest>=7.0.0
pytest-asyncio>=0.21.0
# TODO: Add the correct Home Assistant API library
```

**Fix options**:
1. Add a Home Assistant WebSocket library (e.g., `python-homeassistant-ws`, `homeassistant-api`)
2. Update test code to match chosen library's API
3. Tests also reference wrong entity names (see issue #1)

### 3. Calibration Services Are Placeholders

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

### 4. Hardware Assets Are Empty Placeholders

**Files** (all 0 bytes):
- `hardware/mounts/m5stack_side_mount.stl`
- `docs/assets/wiring_diagram.png`
- `docs/assets/demo.gif`

### 5. Hardware Not Yet Tested

**The firmware compiles successfully but has NOT been tested with actual hardware.**

Before deploying:
1. Update hardcoded baseline statistics in `bed_presence.h` (lines 43-46) after baseline collection
2. Follow `docs/phase1-hardware-setup.md` for baseline data collection procedure
3. Verify LD2410 sensor UART configuration (GPIO16/17, 256000 baud)
4. Test that energy values from LD2410 are in expected range

## Development Workflow

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
4. **IMPORTANT**: Fix entity names first (see Known Issues #1)

**Deploy automation blueprint**:
1. Copy `homeassistant/blueprints/automation/bed_presence_automation.yaml`
2. Place in `<config>/blueprints/automation/` directory
3. Settings → Automations & Scenes → Create Automation → Use Blueprint

### 3. Integration Testing

**Prerequisites**:
- Fix E2E test dependencies (see Known Issues #2)
- Live Home Assistant instance with device connected
- Long-lived access token

```bash
cd tests/e2e
export HA_URL="ws://homeassistant.local:8123/api/websocket"
export HA_TOKEN="your-token"
pytest
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

**Current values** (bed_presence.h:43-46):
```cpp
float mu_move_{100.0f};   // Mean moving energy (placeholder)
float sigma_move_{20.0f}; // Std dev moving energy (placeholder)
```

**To calibrate**:
1. Follow `docs/phase1-hardware-setup.md` section "Baseline Data Collection"
2. Record 30-60 seconds of `ld2410_still_energy` values with empty bed
3. Calculate mean (μ) and standard deviation (σ)
4. Update `mu_move_` and `sigma_move_` in `bed_presence.h`
5. Recompile and flash

**Note**: Phase 3 will automate this process via calibration services.

### Implementing Phase 2 (State Machine + Debouncing)

**Required changes**:

1. **bed_presence.h**: Add state machine enum and timers
   ```cpp
   enum State { IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF };
   State current_state_{IDLE};
   unsigned long debounce_on_ms_{3000};
   unsigned long debounce_off_ms_{5000};
   unsigned long state_change_time_{0};
   ```

2. **bed_presence.cpp**: Replace immediate transitions with debounce logic
   - `process_energy_reading()` becomes a state machine
   - Track time in each debouncing state
   - Only transition after timer expires

3. **presence_engine.yaml**: Add debounce timer entities
   ```yaml
   number:
     - platform: template
       name: "Debounce Occupied (ms)"
       # ... similar to k_on/k_off entities
   ```

4. **Update dashboard**: Add debounce timer controls (currently references non-existent entities)

5. **Update unit tests**: Add time mocking and test debounce behavior

**Reference**: See `docs/presence-engine-spec.md` Phase 2 requirements.

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

**What they test**: Phase 1 z-score logic in isolation

**Run**: `cd esphome && platformio test -e native`

**When to use**:
- Verifying algorithm changes
- Testing edge cases
- Regression testing during refactoring
- Fast feedback loop (runs in <5 seconds)

**Coverage**: 14 tests validate all Phase 1 behavior

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

**File**: `esphome/secrets.yaml` (gitignored)

**Create from template**:
```bash
cd esphome
cp secrets.yaml.example secrets.yaml
```

**Edit** `secrets.yaml`:
```yaml
wifi_ssid: "YourWiFiNetwork"
wifi_password: "YourWiFiPassword"
api_encryption_key: "generate-with-esphome-wizard"
ota_password: "generate-with-esphome-wizard"
```

**Never commit** `secrets.yaml` to git.

## Key Files Reference

### Critical Files for Phase 1 Development

- `docs/presence-engine-spec.md` - **SOURCE OF TRUTH** for 3-phase roadmap
- `esphome/custom_components/bed_presence_engine/bed_presence.cpp` - Core z-score logic (96 lines)
- `esphome/custom_components/bed_presence_engine/bed_presence.h` - Class definition (66 lines)
- `esphome/packages/presence_engine.yaml` - k_on/k_off entities and lambdas
- `esphome/test/test_presence_engine.cpp` - Unit tests (219 lines, 14 tests)

### Files Needing Updates (Known Issues)

- `homeassistant/dashboards/bed_presence_dashboard.yaml` - Fix entity names (lines 42-51, 60-73)
- `tests/e2e/requirements.txt` - Add Home Assistant WebSocket library
- `tests/e2e/test_calibration_flow.py` - Fix entity names and library import

### Placeholder Files (Empty - 0 bytes)

- `hardware/mounts/m5stack_side_mount.stl`
- `docs/assets/wiring_diagram.png`
- `docs/assets/demo.gif`

### Files Safe to Modify

- Threshold defaults: `bed_presence.h` (lines 49-50), `presence_engine.yaml` (lines 24, 40)
- GPIO pins: `packages/hardware_m5stack_ld2410.yaml` (for different boards)
- Baseline statistics: `bed_presence.h` (lines 43-46) - update after calibration
- Dashboard layout: `bed_presence_dashboard.yaml` (after fixing entity names)

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

**Error**: `Entity not found: number.occupied_threshold`
- **Cause**: Dashboard uses wrong entity names (see Known Issues #1)
- **Fix**: Replace with `number.k_on_on_threshold_multiplier`

**Error**: Device not auto-discovered
- **Cause**: WiFi/API configuration issue
- **Fix**: Check `secrets.yaml` credentials, verify same network as HA
- **Debug**: View ESPHome device logs in HA: Settings → Devices → Bed Presence Detector → Logs

**Error**: Service not available
- **Cause**: Calibration services are placeholders in Phase 1
- **Expected**: Services exist but only log messages (Phase 3 will implement)

### Phase 1 Presence Detection Issues

**Problem**: Sensor always reports vacant
- **Cause**: Baseline statistics (μ, σ) don't match actual sensor readings
- **Fix**: Follow `docs/phase1-hardware-setup.md` to collect baseline data
- **Debug**: Check `sensor.ld2410_still_energy` values in HA
- **Verify**: Calculate z-score manually: `(energy - 100) / 20` should be >4 when occupied

**Problem**: Sensor is "twitchy" (rapid state changes)
- **Cause**: This is expected Phase 1 behavior (no debouncing)
- **Fix**: Adjust `k_on`/`k_off` thresholds for wider hysteresis, or implement Phase 2

**Problem**: Sensor doesn't detect presence
- **Cause**: k_on threshold too high, or baseline incorrect
- **Fix**: Lower `k_on` via HA UI (try 3.0 or 2.5)
- **Debug**: Check `text_sensor.presence_state_reason` to see z-score values

## Additional Resources

### Documentation

- `docs/presence-engine-spec.md` - Complete 3-phase engineering specification
- `docs/phase1-hardware-setup.md` - Hardware wiring and baseline calibration guide
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

**What Works**:
- ✅ Phase 1 z-score presence detection (fully implemented)
- ✅ C++ unit tests (14 tests, all passing)
- ✅ ESPHome firmware compilation
- ✅ Runtime threshold tuning via Home Assistant
- ✅ CI/CD workflows (compile + test)
- ✅ GitHub Codespaces development environment
- ✅ Comprehensive documentation

**What Needs Work**:
- ⚠️ Home Assistant dashboard entity names (wrong references)
- ⚠️ E2E tests missing dependency (hass_ws library)
- ⚠️ Calibration services are placeholders (Phase 3 feature)
- ⚠️ Hardware assets are empty placeholders (STL, diagrams, demo)
- ⚠️ Firmware not tested with actual hardware
- ⚠️ Phase 2 (debouncing) and Phase 3 (auto-calibration) not implemented

**Next Steps**:
1. Fix dashboard entity names to match actual Phase 1 entities
2. Test firmware with actual M5Stack + LD2410 hardware
3. Collect baseline data and update hardcoded statistics
4. Add Home Assistant WebSocket library to E2E tests
5. Implement Phase 2 state machine and debouncing
6. Implement Phase 3 automated calibration
