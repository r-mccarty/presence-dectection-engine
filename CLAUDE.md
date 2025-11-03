# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a bed presence detection system for Home Assistant using an ESP32 microcontroller and LD2410 mmWave radar sensor. The project features:

- **On-device presence engine** running on ESP32 (C++ custom component)
- **Stateful filtering** with temporal debouncing and hysteresis
- **Home Assistant integration** with calibration wizard and operator dashboard
- **End-to-end testing** via C++ unit tests and Python E2E tests

## Monorepo Structure

The repository is organized into distinct subsystems:

- `esphome/` - ESP32 firmware (ESPHome YAML + C++ custom components)
  - `custom_components/` - Core C++ presence engine
  - `packages/` - Reusable ESPHome YAML modules
  - `test/` - C++ unit tests using PlatformIO
- `homeassistant/` - Home Assistant configuration
  - `blueprints/` - Automation blueprints
  - `dashboards/` - Lovelace dashboard YAML
- `tests/e2e/` - Python-based integration tests
- `hardware/` - CAD files for 3D printable parts
- `docs/` - User-facing documentation

## Development Commands

### ESPHome Firmware (run from `esphome/` directory)

**Compile firmware:**
```bash
cd esphome
esphome compile bed-presence-detector.yaml
```

**Run C++ unit tests:**
```bash
cd esphome
platformio test -e native
```

**Flash to device:**
```bash
cd esphome
esphome run bed-presence-detector.yaml
```

### End-to-End Tests (run from `tests/e2e/` directory)

**Install dependencies:**
```bash
cd tests/e2e
pip install -r requirements.txt
```

**Run tests:**
```bash
export HA_URL="ws://your-ha-instance:8123/api/websocket"
export HA_TOKEN="your-long-lived-access-token"
pytest
```

## Development Workflow

1. **Phase 1: Firmware Development**
   - Work in `esphome/` directory
   - Compile firmware with `esphome compile` to validate syntax
   - Run `platformio test -e native` for fast C++ unit test feedback
   - Only flash to device after compilation and tests pass

2. **Phase 2: Home Assistant Configuration**
   - Deploy dashboard from `homeassistant/dashboards/`
   - Deploy blueprints from `homeassistant/blueprints/`
   - Create required helpers per `homeassistant/configuration_helpers.yaml.example`

3. **Phase 3: Integration Testing**
   - Run E2E tests from `tests/e2e/` with live Home Assistant instance
   - Requires configured device and HA credentials

## Key Architecture Concepts

### Presence Engine Design

The core presence detection logic runs entirely on the ESP32 as a C++ custom component. This ensures:
- Fast response times (no Wi-Fi dependency)
- High reliability (continues functioning if Home Assistant is down)
- Sophisticated state machine with debouncing to prevent flapping

The engine processes mmWave radar data through:
1. Threshold comparison (configurable via Home Assistant)
2. Temporal debouncing (configurable delay timers)
3. Hysteresis to prevent oscillation
4. State machine transitions with reason tracking

### Home Assistant Integration

The Home Assistant side provides:
- **Calibration Wizard**: Guided UI flow using Home Assistant helpers and scripts
- **Operator Dashboard**: Live visualization of sensor values, thresholds, and state reasons
- **Configuration Interface**: Exposes tunable parameters (thresholds, debounce timers) as entities

## Testing Strategy

- **C++ Unit Tests**: Test presence engine state machine logic in isolation (fast, no hardware required)
- **ESPHome Compilation**: Validates YAML configuration and C++ compilation (catches syntax errors)
- **E2E Integration Tests**: Verify full system integration with live Home Assistant and device

## Environment Setup

This repository is designed for GitHub Codespaces with automatic environment setup via `.devcontainer/devcontainer.json`:
- ESPHome CLI
- PlatformIO (for C++ testing)
- Python 3 with pytest, yamllint, black

## Important Implementation Details

### ESPHome Custom Component Structure

The custom component is located in `esphome/custom_components/bed_presence_engine/`:
- `__init__.py` - ESPHome component schema and configuration validation
- `bed_presence.h` - C++ header defining the BedPresenceEngine class and state machine
- `bed_presence.cpp` - Implementation of the presence detection logic

The component inherits from both `Component` and `BinarySensor` to integrate with ESPHome's lifecycle.

### State Machine States

The presence engine uses a 4-state machine:
1. `VACANT` - No presence detected
2. `DEBOUNCING_OCCUPIED` - Presence signal detected, waiting for confirmation
3. `OCCUPIED` - Confirmed presence
4. `DEBOUNCING_VACANT` - Absence signal detected, waiting for confirmation

Hysteresis is implemented via separate thresholds: `occupied_threshold` > `vacant_threshold`.

### ESPHome Package System

Configuration is modularized using ESPHome packages:
- `hardware_m5stack_ld2410.yaml` - Hardware-specific config (UART, GPIO pins, LD2410 sensor)
- `presence_engine.yaml` - Presence engine configuration and tuning entities
- `services_calibration.yaml` - ESPHome services for calibration workflow
- `diagnostics.yaml` - Device health monitoring sensors

The main entry point `bed-presence-detector.yaml` includes all packages.

### Secrets Management

Create `esphome/secrets.yaml` from `esphome/secrets.yaml.example` before compiling. Never commit the actual secrets file.

### CI/CD Workflows

- `compile_firmware.yml` - Validates ESPHome compilation and runs C++ unit tests on every push to esphome/
- `lint_yaml.yml` - Validates YAML syntax across the repository

## Current Implementation Status

### ✅ Completed Components

1. **ESPHome Custom Component**: Full C++ implementation in `esphome/custom_components/bed_presence_engine/`
   - State machine logic with 4 states
   - Hysteresis-based threshold comparison
   - Temporal debouncing with configurable timers
   - State transition reason tracking
   - Dynamic threshold/debounce updates via lambdas

2. **ESPHome Configuration**: Complete modular YAML configuration
   - Main entry point: `bed-presence-detector.yaml`
   - Four packages: hardware, presence engine, calibration services, diagnostics
   - Number entities for runtime tuning
   - Basic calibration service placeholders

3. **Home Assistant Integration**: Full UI and automation support
   - Lovelace dashboard with 4 views (Status, Configuration, Calibration Wizard, Diagnostics)
   - Automation blueprint for presence events
   - Helper configuration for calibration workflow

4. **Development Infrastructure**
   - GitHub Codespaces devcontainer with auto-setup
   - CI/CD workflows for firmware compilation and YAML linting
   - Comprehensive documentation

### ⚠️ Needs Implementation

1. **C++ Unit Tests** (`esphome/test/test_presence_engine.cpp`)
   - Currently structural placeholders
   - Need to mock ESPHome framework (Component, BinarySensor, Sensor, TextSensor, millis())
   - Should test all state transitions, debounce logic, and hysteresis behavior

2. **Calibration Algorithm** (`esphome/packages/services_calibration.yaml`)
   - Services are defined but logic is placeholder
   - Need to implement energy value sampling during calibration periods
   - Calculate optimal thresholds with safety margins from collected data

3. **E2E Tests** (`tests/e2e/test_calibration_flow.py`)
   - Framework is in place but missing Home Assistant WebSocket client library
   - Add dependency: Consider `homeassistant-api`, `python-homeassistant`, or raw `websockets`
   - Update `tests/e2e/requirements.txt` with chosen library

4. **Hardware Assets**
   - `hardware/mounts/m5stack_side_mount.stl` - placeholder, needs actual 3D model
   - `docs/assets/wiring_diagram.png` - placeholder, needs actual diagram
   - `docs/assets/demo.gif` - placeholder, needs demo recording

### 🔧 Hardware Not Tested

**This codebase has NOT been tested with actual hardware.** Before deploying:
1. Verify LD2410 sensor UART configuration (TX/RX pins, baud rate)
2. Test M5Stack GPIO pin assignments
3. Validate energy value ranges from LD2410 (thresholds may need adjustment)
4. Confirm state transitions work reliably in real-world conditions

## Key Files Reference

### Critical Files for Development

- `esphome/custom_components/bed_presence_engine/bed_presence.cpp` - Core state machine logic
- `esphome/packages/presence_engine.yaml` - Threshold/debounce entities and update lambdas
- `esphome/packages/services_calibration.yaml` - Calibration service definitions
- `homeassistant/dashboards/bed_presence_dashboard.yaml` - User interface
- `homeassistant/configuration_helpers.yaml.example` - Required HA helpers

### Files Safe to Modify

- Threshold defaults in `esphome/packages/presence_engine.yaml`
- Debounce timer defaults in `esphome/packages/presence_engine.yaml`
- GPIO pins in `esphome/packages/hardware_m5stack_ld2410.yaml` (for different boards)
- Dashboard layout in `homeassistant/dashboards/bed_presence_dashboard.yaml`

### Files Requiring Careful Changes

- `esphome/custom_components/bed_presence_engine/__init__.py` - ESPHome component schema
- `esphome/custom_components/bed_presence_engine/bed_presence.h` - Public API and state enum
- State machine logic in `bed_presence.cpp` - Changes affect reliability

## Common Development Tasks

### Adding a New Configurable Parameter

1. Add to `__init__.py` CONFIG_SCHEMA with validation
2. Add setter method in `bed_presence.h`
3. Store as member variable in `bed_presence.h`
4. Use in logic in `bed_presence.cpp`
5. Add number entity in `presence_engine.yaml` with on_value lambda

### Implementing Calibration Logic

1. Add member variables to store calibration samples in `bed_presence.h`
2. Implement sampling logic in `start_calibration` service
3. Calculate thresholds in `stop_calibration` service
4. Consider: min/max tracking, outlier filtering, safety margins
5. Update number entities via `id(entity_name).publish_state(value)`

### Testing State Transitions Locally

Without hardware, you can:
1. Mock the energy sensor in C++ unit tests
2. Advance mock time with `advance_time()` function
3. Verify state transitions and published states
4. Test edge cases (energy drops during debounce, rapid changes, etc.)

## Troubleshooting

### ESPHome Compilation Errors

**Error**: "ld2410 platform not found"
- **Fix**: ESPHome may not have LD2410 support in your version. Check ESPHome version or implement custom UART communication.

**Error**: "bed_presence_engine not found"
- **Fix**: Ensure `custom_components/bed_presence_engine/` is in same directory as YAML file or use `external_components`

### PlatformIO Test Errors

**Error**: "esphome/core/component.h: No such file"
- **Fix**: Unit tests need ESPHome framework mocking. Current tests are structural only.

**Error**: "undefined reference to millis()"
- **Fix**: Mock `millis()` in test file (already defined in `test_presence_engine.cpp`)

### Home Assistant Integration Issues

**Error**: Device not auto-discovered
- **Fix**: Check Wi-Fi credentials in `secrets.yaml`, verify HA is on same network, check ESPHome logs

**Error**: Number entities not showing
- **Fix**: Verify device is connected, check entity IDs match dashboard YAML

**Error**: Services not available
- **Fix**: Ensure device firmware includes `services_calibration.yaml` package
