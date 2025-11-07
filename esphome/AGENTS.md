# AGENTS.md – ESPHome Firmware

Guidance for agents editing firmware, custom components, and PlatformIO tests under `esphome/`. Always cross-check `../CLAUDE.md`, `../README.md`, and the docs set (especially `docs/presence-engine-spec.md` and `docs/phase1-hardware-setup.md`) before making changes.

## Current Firmware Status (Phase 2)
- **Engine**: `custom_components/bed_presence_engine/` implements the 4-state debounced presence engine.
- **Algorithm**: Z-score of `ld2410_still_energy` using baseline `μ = 6.7`, `σ = 3.5`. Thresholds: `k_on = 9.0`, `k_off = 4.0`.
- **State machine**: `IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF` with timers (`on = 3000 ms`, `off = 5000 ms`, `absolute_clear = 30000 ms`).
- **Runtime tuning**: Template `number` entities in `packages/presence_engine.yaml` call `update_*` methods on the component so HA adjustments apply instantly.
- **Tests**: `platformio test -e native` covers 14 scenarios with mocked time to validate state machine behavior. Keep coverage high when adding features.

## Essential Commands (run from `esphome/`)
```bash
# Validate YAML + C++ build
esphome compile bed-presence-detector.yaml

# Flash to connected device
esphome run bed-presence-detector.yaml

# Stream device logs
esphome logs bed-presence-detector.yaml

# Run unit tests
platformio test -e native

# Lint YAML quickly
esphome config bed-presence-detector.yaml
```

## Key Files
- `bed-presence-detector.yaml` – main entry point; includes packages for hardware, presence engine, calibration services placeholder, diagnostics.
- `packages/presence_engine.yaml` – binary sensor definition, threshold + debounce template numbers, text sensor output.
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
| Implement Phase 3 calibration | `bed_presence.cpp/.h`, `packages/services_calibration.yaml`, HA helpers | Follow spec in `docs/presence-engine-spec.md`. |

## Troubleshooting
- **State never leaves DEBOUNCING_ON**: Check that `on_debounce_ms` is achievable; inspect logs for z-score output.
- **Binary sensor flaps**: Increase debounce timers or widen hysteresis. Confirm HA template numbers persisted.
- **Compile errors referencing missing methods**: Update both header and cpp, plus bindings in `binary_sensor.py`.
- **PlatformIO missing libs**: Delete `.pio` folder and rerun tests; dependencies auto-install.

Need broader context? Return to `../AGENTS.md` for repo-wide expectations.
