# HA Bed Presence Detector

[![Compile Firmware](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/compile_firmware.yml/badge.svg)](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/compile_firmware.yml)
[![Lint YAML](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/lint_yaml.yml/badge.svg)](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/lint_yaml.yml)

A high-reliability, tunable, and transparent bed-presence detection solution for Home Assistant. This project uses an ESP32 microcontroller and an LD2410 mmWave radar sensor to provide rock-solid presence data.

The core of this project is a sophisticated on-device presence engine that runs on the ESP32, with a user-friendly Calibration Wizard and Operator Dashboard inside Home Assistant. This repository contains all the code, configuration, and documentation necessary to build, deploy, and test the solution.

## Development Status

⚠️ **Current Status:** Initial Framework - Not Yet Hardware Tested

This repository contains a complete development framework with:
- ✅ **ESPHome Custom Component**: C++ presence engine with 4-state machine (vacant/debouncing/occupied)
- ✅ **Modular Configuration**: ESPHome packages for hardware, presence engine, calibration, and diagnostics
- ✅ **Home Assistant Integration**: Dashboard, automation blueprints, and calibration wizard helpers
- ✅ **CI/CD Workflows**: Automated firmware compilation and YAML validation
- ✅ **Testing Infrastructure**: C++ unit test structure and E2E test framework
- ✅ **Documentation**: Quickstart, calibration guide, troubleshooting, and FAQ
- ✅ **Dev Environment**: GitHub Codespaces with all dependencies pre-configured

**Important Notes:**
- The C++ unit tests are structural placeholders and need full implementation with ESPHome framework mocking
- Calibration services need algorithm implementation for threshold calculation
- E2E tests require a Home Assistant WebSocket client library (see `tests/e2e/requirements.txt`)
- Hardware files (STL mounts, wiring diagrams) are placeholders
- **This code has not been tested with actual hardware yet**

**Ready to Contribute?**
- The architecture and interfaces are defined and ready for implementation
- Start with C++ unit tests or calibration algorithm
- Hardware testers needed for M5Stack + LD2410 validation

## Key Features

*   **On-Device Presence Engine:** All core logic runs on the ESP32 for maximum speed and reliability. Not dependent on Wi-Fi or Home Assistant for its decision-making.
*   **Stateful Filtering:** Uses a state machine with temporal debouncing and hysteresis to prevent false positives/negatives.
*   **HA Calibration Wizard:** A guided, step-by-step UI in Home Assistant to calibrate the sensor for any room.
*   **Transparent Operator Dashboard:** A live Lovelace dashboard that visualizes sensor data, thresholds, and the reason for the last state change.
*   **End-to-End Testable:** Includes C++ unit tests for firmware logic and Python E2E tests for the full system integration.

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

## Core Development Workflow

The development process is broken into three distinct phases. Follow them in order.

### Phase 1: Firmware Development (ESPHome)

All commands in this phase should be run from the `esphome/` directory.

```bash
cd esphome
```

1.  **Compile & Validate Firmware:**
    This is the most important command. It will compile the entire ESPHome configuration, including the C++ `bed_presence_engine`. If this command passes, the firmware is syntactically correct.

    ```bash
    esphome compile bed-presence-detector.yaml
    ```

2.  **Run C++ Unit Tests:**
    This command runs the unit tests for the presence engine logic. This tests the C++ code in isolation, without needing any hardware. It is the fastest way to verify the state machine, debounce timers, etc.

    ```bash
    platformio test -e native
    ```

3.  **Flash to Device:**
    Once compilation and tests pass, flash the firmware to a physically connected M5Stack/ESP32. (This will require connecting a device to the Codespace environment via USB forwarding).

    ```bash
    esphome run bed-presence-detector.yaml
    ```

### Phase 2: Home Assistant Configuration

These files need to be placed into your Home Assistant instance's configuration directory.

1.  **Dashboard:** Copy `homeassistant/dashboards/bed_presence_dashboard.yaml` into your `config/lovelace/dashboards/` directory.
2.  **Automation Blueprint:** Copy `homeassistant/blueprints/automation/bed_presence_automation.yaml` into your `config/blueprints/automation/` directory.
3.  **Required Helpers:** Ensure the helpers defined in `homeassistant/configuration_helpers.yaml.example` are created in your Home Assistant instance. This is required for the Calibration Wizard to function.

### Phase 3: End-to-End (E2E) Integration Testing

These tests verify that the Home Assistant scripts and the ESPHome device's services are all working together correctly.

This requires a running Home Assistant instance with a configured `bed-presence-detector` device connected.

1.  **Navigate to the test directory:**
    ```bash
    cd tests/e2e
    ```

2.  **Install dependencies:**
    ```bash
    pip install -r requirements.txt
    ```

3.  **Run the tests:**
    You will need to configure environment variables with your Home Assistant URL and a Long-Lived Access Token.

    ```bash
    export HA_URL="ws://your-ha-instance:8123/api/websocket"
    export HA_TOKEN="your-long-lived-access-token"

    pytest
    ```

## License

This project is licensed under the Apache 2.0 License. See the [LICENSE](LICENSE) file for details.
