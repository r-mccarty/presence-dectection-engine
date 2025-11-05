# Home Assistant Bed Presence Detector

[![Compile Firmware](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/compile_firmware.yml/badge.svg)](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/compile_firmware.yml)
[![Lint YAML](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/lint_yaml.yml/badge.svg)](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/lint_yaml.yml)

A high-reliability, tunable, and transparent bed-presence detection solution for Home Assistant. This project uses an ESP32 microcontroller and an LD2410 mmWave radar sensor to provide statistical presence detection using **z-score analysis**.

The core of this project is a presence engine that runs on-device on the ESP32, with runtime-tunable thresholds and an operator dashboard inside Home Assistant. This repository contains all the code, configuration, and comprehensive testing infrastructure necessary to build, deploy, and test the solution.

## How It Works: Z-Score Statistical Detection

The presence engine analyzes the LD2410 mmWave radar's "still energy" readings using **z-score normalization** to determine statistical significance:

1. **Z-Score Calculation**: `z = (current_energy - baseline_mean) / baseline_std_dev`
2. **Threshold Comparison with Hysteresis**:
   - Transition to **OCCUPIED** when `z > k_on` (default: 4.0 standard deviations)
   - Transition to **VACANT** when `z < k_off` (default: 2.0 standard deviations)
   - When `k_off < z < k_on`, the state remains unchanged (hysteresis prevents oscillation)

This approach normalizes sensor readings to a statistical scale, making the detection robust across different environments and sensor placements. The `k_on` and `k_off` thresholds are **runtime tunable** via Home Assistant without reflashing the device.

## Development Status

✅ **Phase 1 Complete** | ⏳ **Phase 2 Planned** | ⏳ **Phase 3 Planned**

This project follows a **3-phase development roadmap** (see `docs/presence-engine-spec.md` for details):

### Phase 1: Z-Score Based Detection ✅ IMPLEMENTED
- ✅ **C++ Presence Engine**: Statistical z-score based detection with hysteresis
- ✅ **Runtime Tunable Thresholds**: `k_on` and `k_off` adjustable via Home Assistant
- ✅ **Comprehensive C++ Unit Tests**: 14 tests covering all logic paths (219 lines, all passing)
- ✅ **Home Assistant Dashboard**: Live visualization of energy levels and thresholds
- ✅ **E2E Test Framework**: Python integration tests with proper dependencies
- ✅ **CI/CD Workflows**: Automated compilation and testing
- ✅ **Modular ESPHome Configuration**: Hardware, presence engine, services, diagnostics
- ✅ **Complete Documentation**: Quickstart, hardware setup, troubleshooting, FAQ

**Phase 1 Characteristics:**
- Immediate state transitions (no temporal debouncing - intentionally "twitchy")
- Hysteresis via separate ON/OFF thresholds prevents rapid oscillation
- Manual baseline calibration (hardcoded μ and σ values in code)
- State reason tracking shows z-score values for debugging

### Phase 2: State Machine + Debouncing ⏳ PLANNED
- 4-state machine (IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF)
- Configurable debounce timers for temporal filtering
- Eliminates false positives/negatives from rapid state changes

### Phase 3: Automated Calibration ⏳ PLANNED
- Automated baseline calibration via Home Assistant services
- MAD (Median Absolute Deviation) statistical analysis
- Distance windowing to ignore specific zones
- Calibration wizard UI in Home Assistant

**Current Limitations:**
- ⚠️ **Not yet tested with actual hardware** - firmware compiles successfully but needs hardware validation
- ⚠️ No temporal debouncing in Phase 1 (sensor may be "twitchy" - adjust thresholds or wait for Phase 2)
- ⚠️ Baseline statistics (μ, σ) must be manually calibrated by editing code (see `docs/phase1-hardware-setup.md`)
- ⚠️ Hardware assets (STL mounts, wiring diagrams) are placeholders

**Ready to Contribute?**
- **Hardware testers needed**: M5Stack + LD2410 validation and baseline calibration
- **Phase 2 implementation**: State machine and debounce timer logic
- **Phase 3 implementation**: Automated calibration algorithm using MAD statistics
- **Hardware assets**: 3D printable mounts and wiring diagrams

## Key Features (Phase 1)

*   **On-Device Statistical Analysis:** All z-score calculations run on the ESP32 for maximum speed and reliability. Not dependent on Wi-Fi or Home Assistant for decision-making.
*   **Z-Score Normalization:** Converts raw sensor readings to statistical significance, making detection robust across different environments.
*   **Hysteresis Thresholds:** Separate `k_on` and `k_off` thresholds prevent rapid state oscillation without complex state machines.
*   **Runtime Tunable:** Adjust thresholds via Home Assistant entities without reflashing firmware - perfect for experimentation.
*   **Transparent Operator Dashboard:** Live Lovelace dashboard visualizes energy levels, thresholds, and z-score values in real-time.
*   **State Reason Tracking:** Text sensor shows the exact z-score value that triggered each state change for debugging.
*   **Fully Tested:** 14 C++ unit tests validate all presence logic, plus Python E2E integration tests.

## Repository Structure

This is a monorepo containing all artifacts for the project.

```
.
├── .devcontainer/       # Configuration for the GitHub Codespaces environment.
├── .github/             # CI/CD workflows for automated testing and compilation.
├── docs/                # User-facing documentation (Quickstart, Calibration, etc.).
├── esphome/             # All firmware for the ESP32 device.
│   ├── custom_components/ # The core C++ presence engine.
│   ├── packages/          # Reusable modules of ESPHome YAML configuration.
│   └── test/              # C++ unit tests for the presence engine.
├── hardware/            # CAD files for 3D printable mounts and enclosures.
├── homeassistant/       # All configuration files for Home Assistant.
│   ├── blueprints/        # Automation blueprints.
│   └── dashboards/        # The Lovelace dashboard/wizard file.
└── tests/
    └── e2e/               # Python-based End-to-End integration tests.
```

## Development Environment Setup

This repository is pre-configured for **GitHub Codespaces**.

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/r-mccarty/bed-presence-sensor)

When you launch the Codespace, the `.devcontainer/devcontainer.json` configuration will automatically install and set up all required tools:
*   **ESPHome** (Command Line Interface)
*   **PlatformIO** (For C++ unit testing)
*   **Python 3** and `pip`
*   Required Python libraries (`pytest`, `yamllint`, `black`)

There are no manual setup steps required.

## Quick Start

### 1. Firmware Development (ESPHome)

All commands in this section should be run from the `esphome/` directory.

```bash
cd esphome
```

**Step 1: Compile & Validate Firmware**

Compile the entire ESPHome configuration including the C++ `bed_presence_engine`:

```bash
esphome compile bed-presence-detector.yaml
```

**Step 2: Run C++ Unit Tests** ✅

Run the 14 comprehensive unit tests that validate z-score calculation, state transitions, and hysteresis logic:

```bash
platformio test -e native
```

All tests should pass. These tests verify the Phase 1 presence engine logic in isolation without needing hardware.

**Step 3: Flash to Device**

Flash the firmware to a physically connected M5Stack/ESP32:

```bash
esphome run bed-presence-detector.yaml
```

**Step 4: Manual Baseline Calibration**

⚠️ **Important**: Phase 1 requires manual baseline calibration. See `docs/phase1-hardware-setup.md` for the procedure to:
1. Collect 30-60 seconds of sensor readings with an empty bed
2. Calculate mean (μ) and standard deviation (σ)
3. Update values in `esphome/custom_components/bed_presence_engine/bed_presence.h` (lines 43-46)
4. Recompile and reflash

### 2. Home Assistant Configuration

Once the device is connected to Home Assistant via ESPHome integration:

1.  **Deploy Dashboard:**
    - Copy `homeassistant/dashboards/bed_presence_dashboard.yaml` content
    - In Home Assistant: Settings → Dashboards → Add Dashboard → Create new
    - Edit → Raw Configuration Editor → Paste YAML
    - Dashboard includes Status, Configuration, and Diagnostics views

2.  **Deploy Automation Blueprint:**
    - Copy `homeassistant/blueprints/automation/bed_presence_automation.yaml` to `config/blueprints/automation/`
    - Create automations via Settings → Automations & Scenes → Create Automation → Use Blueprint

3.  **Tune Thresholds:**
    - Adjust `k_on` (ON threshold) and `k_off` (OFF threshold) via the Configuration view
    - Start with defaults (4.0 and 2.0) and adjust based on sensor behavior
    - Changes take effect immediately without reflashing

### 3. End-to-End (E2E) Integration Testing (Optional)

Verify full system integration with a live Home Assistant instance:

```bash
cd tests/e2e
pip install -r requirements.txt  # Installs pytest, pytest-asyncio, homeassistant-api
export HA_URL="ws://your-ha-instance:8123/api/websocket"
export HA_TOKEN="your-long-lived-access-token"
pytest  # Phase 3 tests will be automatically skipped
```

## License

This project is licensed under the Apache 2.0 License. See the [LICENSE](LICENSE) file for details.
