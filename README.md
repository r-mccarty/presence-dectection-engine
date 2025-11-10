# Home Assistant Bed Presence Detector

[![Compile Firmware](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/compile_firmware.yml/badge.svg?branch=main&event=push)](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/compile_firmware.yml)
[![Lint YAML](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/lint_yaml.yml/badge.svg?branch=main&event=push)](https://github.com/r-mccarty/bed-presence-sensor/actions/workflows/lint_yaml.yml)

A high-reliability, tunable, and transparent bed-presence detection solution for Home Assistant using an ESP32 microcontroller and an LD2410 mmWave radar sensor.

This project implements a **state machine with temporal filtering** and **statistical z-score analysis** to provide robust presence detection that is resilient to environmental changes and false positives.

## 🎯 Development Status

*   **Phase 1: Z-Score Detection** - Core statistical engine with hysteresis ✅ **COMPLETE**
*   **Phase 2: State Machine + Debouncing** - Temporal filtering for reliability ✅ **DEPLOYED**
*   **Phase 3: Automated Calibration + Distance Windowing** - MAD statistics + HA services ✅ **DEPLOYED**

See the [Development Scorecard](docs/development-scorecard.md) for a consolidated view of what shipped in each phase, validation evidence, and upcoming work.

For a complete technical breakdown, see the [**Project Architecture**](docs/ARCHITECTURE.md).

## ✨ Key Features

*   **On-Device Statistical Analysis** - All z-score calculations run on ESP32 for maximum speed
*   **4-State Machine with Debouncing** - Eliminates false positives/negatives through temporal filtering
*   **Distance Windowing** - Ignore noise sources outside the configured min/max range
*   **Automated Calibration** - MAD-based baseline service (`calibrate_start_baseline`) tunes μ/σ in-place
*   **Guided Calibration Wizard** - Home Assistant helpers/scripts walk non-technical users through safe recalibration with status tracking
*   **Runtime Tunable** - Adjust thresholds, timers, and distance window via Home Assistant without reflashing
*   **Transparent Dashboard** - Live visualization of energy levels, z-scores, and state transitions
*   **Fully Tested** - 16 C++ unit tests plus Python E2E integration tests

## 📚 Documentation

| Guide | Description |
|-------|-------------|
| [**Development Scorecard**](docs/development-scorecard.md) | Snapshot of Phase 1–3 accomplishments, validation status, and forward-looking tasks |
| [**Architecture**](docs/ARCHITECTURE.md) | Technical deep-dive: algorithms, state machine logic, and testing strategy |
| [**Hardware Setup**](docs/HARDWARE_SETUP.md) | Wiring, sensor calibration, and hardware configuration |
| [**Development Workflow**](docs/DEVELOPMENT_WORKFLOW.md) | Codespace ↔ ubuntu-node workflow, network access, and common tasks |
| [**Contributing**](CONTRIBUTING.md) | Environment setup, secrets management, and development commands |
| [**Calibration Helpers**](homeassistant/configuration_helpers.yaml) | HA helper entities + scripts powering the guided wizard |
| [**Troubleshooting**](docs/troubleshooting.md) | Common issues and solutions |
| [**FAQ**](docs/faq.md) | Frequently asked questions |
| [**Quick Start**](docs/quickstart.md) | User-facing quick start guide |

## 🚀 Quick Start

### 1. Hardware

Assemble the required hardware by following the [**Hardware Setup Guide**](docs/HARDWARE_SETUP.md).

**Required Components:**
- M5Stack Basic (ESP32) or compatible ESP32 board
- LD2410 mmWave radar sensor
- USB cable for programming
- Jumper wires for UART connection

### 2. Development Environment

This repository is pre-configured for **GitHub Codespaces**:

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/r-mccarty/bed-presence-sensor)

All required tools (ESPHome, PlatformIO, Python) are automatically installed.

For local development setup, see the [**Contributing Guide**](CONTRIBUTING.md).

### 3. Compile and Flash

Follow the [**Development Workflow**](docs/DEVELOPMENT_WORKFLOW.md) to compile and flash the firmware:

```bash
cd esphome
esphome compile bed-presence-detector.yaml  # Compile firmware
platformio test -e native                   # Run C++ unit tests (14 tests)
esphome run bed-presence-detector.yaml      # Flash to device
```

### 4. Home Assistant Integration

Once the device connects to Home Assistant:

1. **Deploy Dashboard**: Copy `homeassistant/dashboards/bed_presence_dashboard.yaml` to Home Assistant
2. **Tune Parameters**: Adjust thresholds and debounce timers via the Configuration view
3. **Monitor**: Use `text_sensor.presence_state_reason` + `text_sensor.presence_change_reason` for real-time debugging

See the [**Quick Start Guide**](docs/quickstart.md) for detailed instructions.

## 🔧 Repository Structure

```
.
├── docs/                      # Documentation
│   ├── ARCHITECTURE.md        # Technical deep-dive
│   ├── DEVELOPMENT_WORKFLOW.md # Development workflow guide
│   ├── HARDWARE_SETUP.md      # Hardware setup and calibration
│   └── ...
├── esphome/                   # ESP32 firmware
│   ├── custom_components/     # C++ presence engine
│   ├── packages/              # Modular YAML configuration
│   ├── test/                  # C++ unit tests (14 tests)
│   └── bed-presence-detector.yaml # Main ESPHome config
├── homeassistant/             # Home Assistant configuration
│   ├── blueprints/            # Automation blueprints
│   └── dashboards/            # Lovelace dashboard
├── tests/e2e/                 # Python E2E integration tests
├── CONTRIBUTING.md            # Developer setup guide
└── CLAUDE.md                  # AI agent context map
```

## 🤝 Contributing

Contributions are welcome! Please see the [**Contributing Guide**](CONTRIBUTING.md) for:

- Environment setup (Codespaces, local development)
- Secrets management (WiFi credentials, Home Assistant tokens)
- Development commands (compile, test, flash)
- CI/CD workflows

**Areas for contribution:**
- Calibration history persistence + analytics
- Real-world tuning and optimization
- Hardware assets (3D printable mounts, wiring diagrams)
- Documentation improvements

## 📄 License

This project is licensed under the Apache 2.0 License. See the [LICENSE](LICENSE) file for details.

## 🔗 Links

- [Project Architecture](docs/ARCHITECTURE.md) - Understand how it works
- [Hardware Setup](docs/HARDWARE_SETUP.md) - Build the hardware
- [Development Workflow](docs/DEVELOPMENT_WORKFLOW.md) - Learn the workflow
- [Troubleshooting](docs/troubleshooting.md) - Solve common issues
- [GitHub Issues](https://github.com/r-mccarty/bed-presence-sensor/issues) - Report bugs or request features
