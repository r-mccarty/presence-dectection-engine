# Gemini Code Assistant Context

This document provides a comprehensive overview of the Bed Presence Sensor project for the Gemini Code Assistant. It covers the project's purpose, architecture, key technologies, and development conventions.

## Project Overview

The Bed Presence Sensor is a high-reliability, tunable, and transparent bed-presence detection solution for Home Assistant. It uses an ESP32 microcontroller and an LD2410 mmWave radar sensor to detect presence.

The core of the project is a C++ statistical presence detection engine running on the ESP32. This engine uses z-score normalization to convert raw mmWave radar readings into statistical significance, combined with a 4-state machine for temporal filtering. This approach makes the detection resilient to environmental changes and false positives.

The project is deeply integrated with Home Assistant, allowing for real-time monitoring, tuning of parameters, and visualization of sensor data.

### Key Technologies

*   **Hardware:** ESP32 (M5Stack Basic), LD2410 mmWave radar sensor
*   **Firmware:** ESPHome, C++ for the custom presence engine
*   **Home Assistant:** YAML for configuration and dashboards
*   **Testing:** PlatformIO for C++ unit tests, Python with `pytest` for end-to-end tests
*   **CI/CD:** GitHub Actions for compiling firmware and linting YAML files

### Architecture

The project follows a 3-phase development roadmap:

*   **Phase 1 (Complete):** Z-score based detection with hysteresis.
*   **Phase 2 (Deployed):** State machine with debouncing for temporal filtering.
*   **Phase 3 (Deployed):** Automated calibration + distance windowing using MAD statistics.

The C++ presence engine implements a 4-state machine: `IDLE`, `DEBOUNCING_ON`, `PRESENT`, and `DEBOUNCING_OFF`. State transitions are determined by z-score values and debounce timers.

The ESPHome configuration is modular, with separate YAML files for hardware, the presence engine, services, and diagnostics.

## Building and Running

The recommended development environment is GitHub Codespaces, which comes pre-configured with all the necessary tools.

### Key Commands

All commands should be run from the project's root directory.

**Compile Firmware:**
```bash
cd esphome
esphome compile bed-presence-detector.yaml
```

**Run C++ Unit Tests:**
```bash
cd esphome
platformio test -e native
```

**Flash Firmware:**
```bash
cd esphome
esphome run bed-presence-detector.yaml
```
*(Note: Flashing requires physical USB access to the device.)*

**Run End-to-End Tests:**
```bash
cd tests/e2e
pip install -r requirements.txt
export HA_URL="ws://<your-ha-instance>:8123/api/websocket"
export HA_TOKEN="<your-long-lived-access-token>"
pytest
```

**Linting:**
```bash
yamllint esphome/ homeassistant/
black tests/e2e/ scripts/
```

## Development Conventions

*   **Secrets Management:** The project uses two separate secrets files:
    *   `.env.local` at the project root for Home Assistant API access for Python scripts.
    *   `esphome/secrets.yaml` for WiFi credentials to be embedded in the firmware.
*   **C++ Style:** Follow ESPHome coding conventions. Update unit tests for any algorithm changes.
*   **Python Style:** Use Black for formatting. Follow PEP 8 guidelines.
*   **YAML Style:** Use 2 spaces for indentation. Validate with `yamllint`.
*   **Testing:** All C++ changes must be accompanied by unit tests. All Python changes should have corresponding end-to-end tests.
*   **Pull Requests:** PRs should include a clear description of changes, link to related issues, and test results.
