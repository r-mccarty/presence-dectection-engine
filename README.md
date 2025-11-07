# Home Assistant Bed Presence Detector

[![Compile Firmware](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/compile_firmware.yml/badge.svg)](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/compile_firmware.yml)
[![Lint YAML](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/lint_yaml.yml/badge.svg)](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/lint_yaml.yml)

A high-reliability, tunable, and transparent bed-presence detection solution for Home Assistant. This project uses an ESP32 microcontroller and an LD2410 mmWave radar sensor to provide statistical presence detection using **z-score analysis**.

The core of this project is a presence engine that runs on-device on the ESP32, with runtime-tunable thresholds and an operator dashboard inside Home Assistant. This repository contains all the code, configuration, and comprehensive testing infrastructure necessary to build, deploy, and test the solution.

## How It Works: Z-Score Statistical Detection

The presence engine analyzes the LD2410 mmWave radar's "still energy" readings using **z-score normalization** to determine statistical significance:

1. **Z-Score Calculation**: `z = (current_energy - baseline_mean) / baseline_std_dev`
2. **Threshold Comparison with Hysteresis**:
   - Transition to **OCCUPIED** when `z > k_on` (default: 9.0 standard deviations)
   - Transition to **VACANT** when `z < k_off` (default: 4.0 standard deviations)
   - When `k_off < z < k_on`, the state remains unchanged (hysteresis prevents oscillation)

This approach normalizes sensor readings to a statistical scale, making the detection robust across different environments and sensor placements. The `k_on` and `k_off` thresholds are **runtime tunable** via Home Assistant without reflashing the device.

## Development Status

✅ **Phase 1 COMPLETE** | ✅ **Phase 2 DEPLOYED** | ⏳ **Phase 3 Planned**

This project follows a **3-phase development roadmap** (see `docs/presence-engine-spec.md` for details):

### Phase 1: Z-Score Based Detection ✅ COMPLETE
- ✅ **C++ Presence Engine**: Statistical z-score based detection with hysteresis
- ✅ **Runtime Tunable Thresholds**: `k_on` and `k_off` adjustable via Home Assistant
- ✅ **Hardware Deployed**: M5Stack + LD2410 tested and operational
- ✅ **Baseline Calibrated**: Real sensor statistics collected and validated (μ=6.7%, σ=3.5%)
- ✅ **Presence Detection Validated**: Correctly detects occupied (64% energy, z=16.37) and empty bed (3% energy, z=-1.06)
- ✅ **Status**: Completed 2025-11-06

### Phase 2: State Machine + Debouncing ✅ DEPLOYED
- ✅ **4-State Machine**: IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF eliminates "twitchiness"
- ✅ **Temporal Filtering**: 3-second on-debounce, 5-second off-debounce, 30-second absolute clear delay
- ✅ **Runtime Tunable Timers**: Debounce parameters adjustable via Home Assistant without reflashing
- ✅ **Comprehensive C++ Unit Tests**: 14 tests with time mocking (355 lines, all passing)
- ✅ **Dashboard Updated**: Debounce timer controls added to Configuration view
- ✅ **Semantic Fixes**: Variable naming corrected (mu_move_ → mu_still_ for accuracy)
- ✅ **Hardware Deployed**: Firmware flashed and operational on M5Stack device
- ✅ **Status**: Deployed to hardware 2025-11-07, fully operational in production

**Phase 2 Characteristics:**
- Debounced state transitions prevent false positives/negatives
- Quick motion (< 3 seconds) won't trigger sensor
- Sustained presence required to turn ON, sustained absence required to turn OFF
- Absolute clear delay prevents premature clearing after recent movement
- Binary sensor only changes on confirmed transitions (not during debouncing)

### Phase 3: Automated Calibration ⏳ PLANNED
- Automated baseline calibration via Home Assistant services
- MAD (Median Absolute Deviation) statistical analysis
- Distance windowing to ignore specific zones
- Calibration wizard UI in Home Assistant

**Ready to Contribute?**
- **Phase 2 tuning**: Real-world testing and optimization of debounce parameters
- **Phase 3 implementation**: Automated calibration algorithm using MAD statistics
- **Hardware assets**: 3D printable mounts and wiring diagrams
- **Documentation**: Additional tuning guides and real-world deployment experiences

## Key Features (Phase 2)

*   **On-Device Statistical Analysis:** All z-score calculations and state machine run on the ESP32 for maximum speed and reliability. Not dependent on Wi-Fi or Home Assistant for decision-making.
*   **4-State Machine with Debouncing:** Temporal filtering eliminates false positives/negatives through sustained condition requirements (IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF).
*   **Z-Score Normalization:** Converts raw sensor readings to statistical significance, making detection robust across different environments.
*   **Hysteresis Thresholds:** Separate `k_on` and `k_off` thresholds prevent rapid state oscillation.
*   **Runtime Tunable:** Adjust thresholds AND debounce timers via Home Assistant entities without reflashing firmware - perfect for experimentation.
*   **Absolute Clear Delay:** Prevents premature clearing within 30 seconds of last high-confidence signal (configurable).
*   **Transparent Operator Dashboard:** Live Lovelace dashboard visualizes energy levels, thresholds, debounce timers, and z-score values in real-time.
*   **State Reason Tracking:** Text sensor shows z-score values and debounce timing for each state change for debugging.
*   **Fully Tested:** 14 C++ unit tests with time mocking validate all state machine logic, plus Python E2E integration tests.

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

Run the 14 comprehensive unit tests that validate z-score calculation, state machine transitions, debouncing logic, and edge cases:

```bash
platformio test -e native
```

All tests should pass. These tests verify the Phase 2 presence engine logic in isolation without needing hardware.

**Step 3: Flash to Device**

Flash the firmware to a physically connected M5Stack/ESP32:

```bash
esphome run bed-presence-detector.yaml
```

**Step 4: Manual Baseline Calibration**

⚠️ **Important**: Phase 2 requires manual baseline calibration. See `docs/phase1-hardware-setup.md` for the procedure to:
1. Collect 30-60 seconds of sensor readings with an empty bed
2. Calculate mean (μ) and standard deviation (σ)
3. Update values in `esphome/custom_components/bed_presence_engine/bed_presence.h` (lines 60-61: mu_still_, sigma_still_)
4. Recompile and reflash

**Note**: Current baseline is μ=6.7%, σ=3.5% (collected 2025-11-06, suitable for new sensor position).

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

3.  **Tune Thresholds & Debounce Timers:**
    - Adjust `k_on` (ON threshold) and `k_off` (OFF threshold) via the Configuration view
    - Defaults are k_on=9.0 and k_off=4.0 (tuned to reduce false positives)
    - **Phase 2**: Adjust debounce timers (on_debounce_ms=3000, off_debounce_ms=5000, abs_clear_delay_ms=30000)
    - Changes take effect immediately without reflashing
    - See `docs/phase2-completion-steps.md` for tuning guidance

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
