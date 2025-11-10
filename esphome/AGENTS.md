# AGENTS.md – ESPHome Firmware

Guidance for agents editing firmware, custom components, and PlatformIO tests under `esphome/`. **Always read `../CLAUDE.md` first**, then consult:
- `docs/ARCHITECTURE.md` for algorithm details and state machine design
- `docs/DEVELOPMENT_WORKFLOW.md` for the **critical two-machine workflow**
- `docs/presence-engine-spec.md` for engineering requirements

## Current Firmware Status (Phase 3 DEPLOYED)
- **Status**: Phase 3 is **DEPLOYED** and fully operational on hardware
- **Engine**: `custom_components/bed_presence_engine/` implements 4-state debounced presence engine with distance windowing & calibration
- **Algorithm**: Z-score statistical analysis of `ld2410_still_energy`
  - Baseline: μ = 6.7%, σ = 3.5% (calibrated 2025-11-06, empty bed)
  - Thresholds: k_on = 9.0, k_off = 4.0
  - ON threshold: 6.7 + (9.0 × 3.5) = 38.2% still energy
  - OFF threshold: 6.7 + (4.0 × 3.5) = 20.7% still energy
- **State machine**: `IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF`
  - Debounce timers: on = 3000ms, off = 5000ms, absolute_clear = 30000ms
  - Distance window: d_min = 0cm, d_max = 600cm (runtime tunable)
  - All parameters are runtime-tunable from Home Assistant
- **Calibration**: MAD-based baseline collection via ESPHome services (`calibrate_start_baseline`, `calibrate_reset_all`)
- **Last-change telemetry**: `Presence Change Reason` text sensor emits reason codes for each transition
- **Runtime tuning**: Template `number` entities in `packages/presence_engine.yaml` call `update_*` methods
- **Tests**: 16 comprehensive C++ unit tests, all passing (`platformio test -e native`)

## ⚠️ CRITICAL: Two-Machine Workflow

**This project requires TWO development environments** (see `docs/DEVELOPMENT_WORKFLOW.md`):

### Codespaces / Local Machine
- ✅ Code editing, documentation
- ✅ Git operations (commit, push)
- ✅ Unit tests: `platformio test -e native`
- ✅ YAML validation: `yamllint .` or `esphome config bed-presence-detector.yaml`
- ❌ **CANNOT** compile firmware (dependency issues)
- ❌ **CANNOT** flash to device (no physical USB access)
- ❌ **CANNOT** access Home Assistant (network isolation)

### ubuntu-node (Physical Hardware Machine)
- ✅ Pull git changes from Codespaces
- ✅ Compile firmware: `esphome compile bed-presence-detector.yaml`
- ✅ Flash to device: `esphome run bed-presence-detector.yaml` (or `~/sync-and-flash.sh`)
- ✅ Stream logs: `esphome logs bed-presence-detector.yaml`
- ✅ Run E2E tests: `cd tests/e2e && pytest`
- ✅ Access Home Assistant API (192.168.0.148)
- ✅ Physical USB access to M5Stack device

### Standard Workflow
1. **Edit in Codespaces**: Make code changes, run unit tests
2. **Commit & Push**: `git add . && git commit -m "..." && git push`
3. **SSH to ubuntu-node**: `ssh user@ubuntu-node`
4. **Pull & Flash**: `cd ~/bed-presence-sensor && git pull && ~/sync-and-flash.sh`
5. **Verify**: Check ESPHome logs and Home Assistant state

## Key Files
- `bed-presence-detector.yaml` – main entry point; includes packages for hardware, presence engine, calibration services, diagnostics.
- `packages/presence_engine.yaml` – binary sensor definition, threshold + debounce + distance template numbers, text sensors.
- `custom_components/bed_presence_engine/bed_presence.h/.cpp` – C++ engine. Read header before editing implementation.
- `custom_components/bed_presence_engine/binary_sensor.py` – Schema definitions; update when adding options (e.g., new timers).
- `test/test_presence_engine.cpp` – PlatformIO unit tests; extend when modifying state machine or thresholds.

## Coding Guidance
- Use `ESP_LOGI/W/D` inside the engine to explain state transitions (`publish_reason()` already formats z-scores + timers).
- Maintain trailing underscores for private fields (`current_state_`, `debounce_start_time_`).
- Keep the component non-blocking: no `delay()`, no heavy heap allocations inside `loop()`.
- When adding new runtime parameters, expose setters + template numbers, update HA dashboard + docs.
- For new tests, use the existing `AdvanceTime` helper to simulate debounce timers.

## Common Tasks
| Goal | Touch These Files | Notes |
| --- | --- | --- |
| Adjust defaults | `packages/presence_engine.yaml`, `bed_presence.cpp` constants | Update docs + dashboard defaults too. |
| Add telemetry | `bed_presence.cpp`, `packages/diagnostics.yaml` | Ensure sensors exist in HA dashboard. |
| Tune calibration defaults | `bed_presence.cpp/.h`, `packages/services_calibration.yaml`, `packages/presence_engine.yaml` | Update docs + HA dashboard if defaults move. |

## Troubleshooting

### Firmware Behavior Issues
- **State never leaves DEBOUNCING_ON**: Check `on_debounce_ms` timer duration, inspect logs for z-score output, verify baseline calibration.
- **Binary sensor flaps/oscillates**: Increase debounce timers or widen hysteresis (gap between k_on/k_off). Confirm HA template numbers persisted.
- **Stuck in OFF state**: Verify baseline calibration, check z-scores exceed k_on threshold, try lowering k_on temporarily.

### Build/Compilation Issues
- **"Cannot compile in Codespaces"**: This is expected! Compilation MUST be done on ubuntu-node (see two-machine workflow above).
- **Compile errors referencing missing methods**: Update both header (.h) and implementation (.cpp), plus schema in `binary_sensor.py`.
- **PlatformIO missing libs**: Delete `.pio` folder and rerun; dependencies auto-install on first build.
- **Unit tests fail**: Check that mocked time advances correctly, verify state transitions match expected sequence.

### Deployment Issues
- **"Cannot flash firmware"**: Must be on ubuntu-node with physical USB connection to M5Stack device.
- **Device not found**: Check USB cable, verify device shows in `esphome run` device list, try different USB port.
- **OTA fails**: Check WiFi connectivity, verify `secrets.yaml` credentials, use USB fallback.

### Secrets Management
- **"WiFi credentials not working"**: Check `secrets.yaml` file (WiFi credentials embedded in firmware), ubuntu-node is source of truth.
- **"Cannot access HA API"**: Check `.env.local` file (Python scripts), must run on ubuntu-node or use SSH port forwarding.

For detailed troubleshooting, see `../docs/troubleshooting.md`. For repo-wide context, see `../AGENTS.md`.
