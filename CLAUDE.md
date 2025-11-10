# AI Agent Context Map: Bed Presence Sensor

**Note to AI:** This is your primary context file. It provides a high-level summary and serves as a map to the detailed documentation. Use this to orient yourself before accessing other files for specific tasks.

---

## Project Overview & Status

This is an ESP32-based bed presence detection system using an LD2410 mmWave sensor. It uses **z-score statistical analysis** and a **4-state machine** (IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF) for reliable presence detection.

*   **Current Status:** Phase 3 is **DEPLOYED** and fully operational. The system now includes automated MAD-based calibration, distance windowing, and detailed change reasons on top of the Phase 2 temporal filtering.
*   **Hardware:** M5Stack Basic (ESP32) + LD2410 mmWave sensor, connected to Home Assistant at IP 192.168.0.148
*   **Baseline:** μ=6.7%, σ=3.5% (calibrated 2025-11-06, empty bed)
*   **Source of Truth for Logic:** The engineering specification is in `docs/presence-engine-spec.md`.

---

## Key Documents and Context

This repository's information has been refactored into focused documents. **Always refer to these documents for detailed information:**

### For Understanding Technical Design and Algorithms

**📖 Go to:** [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)

**Contains:**
- 3-phase development roadmap (Phase 1, 2, 3)
- Detailed Phase 2 state machine logic and implementation
- C++ class structure (`BedPresenceEngine`)
- Z-score calculation and threshold logic
- Testing strategy (unit tests, integration tests)
- ESPHome configuration and entity structure

**Use when:** You need to understand how the presence detection algorithm works, state machine transitions, or testing approach.

---

### For Code Changes, Compilation, and Flashing

**📖 Go to:** [`docs/DEVELOPMENT_WORKFLOW.md`](docs/DEVELOPMENT_WORKFLOW.md)

**Contains:**
- **Critical:** The two-machine workflow (Codespace ↔ ubuntu-node)
- SSH port forwarding for network access from Codespaces
- Standard workflow for code changes (edit → commit → sync → flash)
- Helper scripts on ubuntu-node (`~/sync-and-flash.sh`, `~/flash-firmware.sh`)
- Common development tasks (updating thresholds, recalibrating baseline)
- Quick iteration patterns

**Use when:** Making code changes, compiling firmware, flashing to device, or running scripts that need Home Assistant API access.

**⚠️ CRITICAL:** All firmware flashing MUST be done on ubuntu-node (has physical USB access). Codespaces is for code editing only.

---

### For Environment Setup and Secrets Management

**📖 Go to:** [`CONTRIBUTING.md`](CONTRIBUTING.md)

**Contains:**
- GitHub Codespaces setup (automatic)
- Local development environment setup
- **Critical:** Detailed explanation of TWO secrets files:
  - `.env.local` - Home Assistant API access for Python scripts
  - `secrets.yaml` - WiFi credentials embedded in firmware
- Development commands (compile, test, flash)
- CI/CD workflows

**Use when:** Setting up development environment, managing credentials, or understanding which secrets file to use.

**⚠️ CRITICAL:** Ubuntu-node is the source of truth for both secrets files. Always copy FROM ubuntu-node TO Codespace, never reverse.

---

### For Hardware, Wiring, and Calibration

**📖 Go to:** [`docs/HARDWARE_SETUP.md`](docs/HARDWARE_SETUP.md)

**Contains:**
- Hardware specifications (M5Stack, LD2410 sensor)
- Wiring diagram and UART configuration
- Physical device details (MAC address, IP, firmware version)
- Manual baseline calibration procedure
- Baseline statistics (μ, σ) collection process
- Hardware validation and testing

**Use when:** Setting up hardware, collecting baseline data, or understanding physical device configuration.

---

### For Diagnosing Errors and Known Issues

**📖 Go to:** [`docs/troubleshooting.md`](docs/troubleshooting.md)

**Contains:**
- Common issues and solutions (false positives, false negatives, connectivity)
- Phase-specific troubleshooting (Phase 2 state machine issues)
- Diagnostic tools (`text_sensor.presence_state_reason`, ESPHome logs)
- Known issues (placeholder calibration services, empty hardware assets)
- Manual z-score calculation for debugging

**Use when:** Diagnosing errors, sensor not behaving as expected, or user reports issues.

---

## Core Code Files

**C++ Implementation:**
- `esphome/custom_components/bed_presence_engine/bed_presence.cpp` - Core presence engine logic (162 lines)
- `esphome/custom_components/bed_presence_engine/bed_presence.h` - Class definition with state machine (80 lines)
- `esphome/custom_components/bed_presence_engine/__init__.py` - ESPHome component registration
- `esphome/custom_components/bed_presence_engine/binary_sensor.py` - ESPHome config schema

**Testing:**
- `esphome/test/test_presence_engine.cpp` - 14 comprehensive C++ unit tests (355 lines, all passing)
- `tests/e2e/test_calibration_flow.py` - Python E2E integration tests

**Configuration:**
- `esphome/bed-presence-detector.yaml` - Main ESPHome entry point
- `esphome/packages/presence_engine.yaml` - Presence engine entities and configuration
- `esphome/packages/hardware_m5stack_ld2410.yaml` - Hardware-specific UART and GPIO configuration

---

## Quick Reference: Phase 2 Parameters

**Current Baseline (Calibrated 2025-11-06):**
- μ_still = 6.7% (mean still energy, empty bed)
- σ_still = 3.5% (standard deviation)

**Thresholds:**
- k_on = 9.0 (z-score > 9.0 triggers DEBOUNCING_ON)
- k_off = 4.0 (z-score < 4.0 triggers DEBOUNCING_OFF)
- ON threshold: 6.7 + (9.0 × 3.5) = 38.2% still energy
- OFF threshold: 6.7 + (4.0 × 3.5) = 20.7% still energy

**Debounce Timers:**
- on_debounce_ms = 3000 (3 seconds sustained high signal required)
- off_debounce_ms = 5000 (5 seconds sustained low signal required)
- abs_clear_delay_ms = 30000 (30 seconds since last high confidence reading)

**All parameters are runtime tunable via Home Assistant number entities.**

**Distance Window Defaults:**
- d_min_cm = 0 (allow all readings)
- d_max_cm = 600 (full LD2410 range)
- Frames outside the window are ignored by the presence logic and calibration sampler.

**Calibration + Reset Services (ESPHome):**
- `esphome.bed_presence_detector_calibrate_start_baseline` (preferred) or `..._start_calibration` legacy alias
- `esphome.bed_presence_detector_calibrate_stop` / `..._stop_calibration`
- `esphome.bed_presence_detector_calibrate_reset_all` / `..._reset_to_defaults`
- Services collect still-energy samples, compute μ/σ via MAD, and push defaults back to HA number entities.

**Telemetry:**
- `sensor.bed_presence_detector_presence_state_reason` → Verbose z-score + timer context
- `sensor.bed_presence_detector_presence_change_reason` → Short reason codes (`on:threshold_exceeded`, `calibration:completed`, etc.)

---

## Monorepo Structure (Abbreviated)

```
/workspaces/bed-presence-sensor/
├── CLAUDE.md                         # This file - AI agent context map
├── README.md                         # Human-friendly entry point
├── CONTRIBUTING.md                   # Developer setup and secrets management
├── docs/
│   ├── ARCHITECTURE.md               # Technical deep-dive (algorithms, state machine)
│   ├── DEVELOPMENT_WORKFLOW.md       # Codespace ↔ ubuntu-node workflow
│   ├── HARDWARE_SETUP.md             # Hardware setup and calibration
│   ├── troubleshooting.md            # Common issues and solutions
│   └── presence-engine-spec.md       # Source of truth for 3-phase spec
├── esphome/
│   ├── custom_components/bed_presence_engine/  # C++ implementation
│   ├── packages/                     # Modular YAML configuration
│   ├── test/test_presence_engine.cpp # 14 C++ unit tests
│   └── bed-presence-detector.yaml    # Main ESPHome config
├── homeassistant/
│   ├── blueprints/automation/        # Automation blueprint
│   └── dashboards/                   # Lovelace dashboard
└── tests/e2e/                        # Python E2E integration tests
```

---

## Critical Workflow Reminders

1. **Code Changes:** Edit in Codespace → Commit → Push → SSH to ubuntu-node → Pull → Flash
2. **Flashing Firmware:** MUST be done on ubuntu-node (physical USB access), use `~/sync-and-flash.sh`
3. **Secrets Management:** Ubuntu-node is source of truth, copy FROM ubuntu-node TO Codespace
4. **Network Access:** Codespaces cannot directly access Home Assistant (192.168.0.148), use SSH port forwarding or run scripts on ubuntu-node
5. **Baseline Calibration:** Manual process, run `collect_baseline.py` on ubuntu-node, update `bed_presence.h`, recompile and reflash

---

## Development Status Summary

**✅ Phase 1: Z-Score Detection** - COMPLETE (2025-11-06)
- Z-score calculation with hysteresis
- Runtime tunable thresholds (k_on, k_off)
- Immediate state transitions (intentionally "twitchy")

**✅ Phase 2: State Machine + Debouncing** - DEPLOYED (2025-11-07)
- 4-state machine (IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF)
- Temporal filtering eliminates false positives/negatives
- Runtime tunable debounce timers
- 16 C++ unit tests, all passing
- Fully operational on hardware

**✅ Phase 3: Environmental Hardening + Calibration** - DEPLOYED (2025-11-08)
- Distance windowing driven by `distance_min/max` runtime knobs
- MAD-based baseline calibration via ESPHome services (start/stop/reset)
- New text sensor exposes last change reason codes (e.g., `on:threshold_exceeded`)
- Runtime reset service restores all knobs/baselines to known-good defaults
- Home Assistant wizard helpers remain TODO, but device-side automation is complete

---

## Known Issues and Limitations

**Calibration Wizard UI:** Device-side MAD calibration + reset services are live, but the Home Assistant dashboard wizard remains in progress. Trigger calibration via Developer Tools → Services until the UI helpers ship.

**Empty Hardware Assets:** The following files are 0-byte placeholders:
- `hardware/mounts/m5stack_side_mount.stl`
- `docs/assets/wiring_diagram.png`
- `docs/assets/demo.gif`

**Network Limitations:** GitHub Codespaces cannot directly access local Home Assistant instance. Use SSH port forwarding or run scripts on ubuntu-node.

---

## When Working on Tasks

1. **Always check the relevant documentation first** before making assumptions
2. **For code changes:** Read DEVELOPMENT_WORKFLOW.md for the two-machine workflow
3. **For algorithm questions:** Read ARCHITECTURE.md for technical details
4. **For hardware issues:** Read HARDWARE_SETUP.md and troubleshooting.md
5. **For secrets:** Read CONTRIBUTING.md to understand which file to use

**Remember:** This is a two-machine workflow project. Codespaces = code editing, ubuntu-node = firmware flashing and hardware access.
