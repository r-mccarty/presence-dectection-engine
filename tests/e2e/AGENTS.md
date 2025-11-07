# AGENTS.md – End-to-End Integration Tests

Guidance for agents working on the async pytest suite under `tests/e2e/`. These tests exercise the deployed ESPHome device through the Home Assistant WebSocket API.

## Current Scope
- Verifies device connectivity, binary sensor availability, runtime threshold + debounce controls, ESPHome services, and state-reason telemetry.
- Uses `hass_ws.HomeAssistantClient` (see `requirements.txt`) to talk to HA over WebSocket.
- Calibration helper tests remain skipped until Phase 3 features land.

## Running the Suite
```bash
cd tests/e2e
pip install -r requirements.txt
export HA_URL="ws://homeassistant.local:8123/api/websocket"
export HA_TOKEN="<long-lived-access-token>"
pytest            # full run (skips Phase 3 tests automatically)
pytest -k debounce  # example: focus on debounce-related tests when added
```

### Environment Requirements
- Live Home Assistant instance ≥ 2023.8 with the ESPHome device provisioned.
- Entities created by ESPHome Phase 2 firmware must exist with the expected entity IDs:
  - `binary_sensor.bed_presence_detector_bed_occupied`
  - `number.bed_presence_detector_k_on_on_threshold_multiplier`
  - `number.bed_presence_detector_k_off_off_threshold_multiplier`
  - `number.bed_presence_detector_on_debounce_timer_ms`
  - `number.bed_presence_detector_off_debounce_timer_ms`
  - `number.bed_presence_detector_absolute_clear_delay_ms`
  - `sensor.bed_presence_detector_presence_state_reason`
- Custom ESPHome services exposed: `start_calibration`, `stop_calibration`, `reset_to_defaults` (Phase 3 still planned for full automation).

## Editing Guidelines
- Keep tests async-friendly. Use `await asyncio.sleep()` sparingly; prefer polling loops with timeouts if waiting for HA state changes.
- When asserting defaults, mirror firmware truth (Phase 2 defaults: `k_on=9.0`, `k_off=4.0`, `on_debounce=3000`, `off_debounce=5000`, `absolute_clear=30000`). Update README/docs when defaults change.
- Tag future-phase tests with `@pytest.mark.skip(reason="Phase X feature")` until implemented.
- Add fixtures for repeated setup/teardown (e.g., resetting thresholds, clearing timers).

## Troubleshooting
- **Connection refused**: Verify HA URL/token, ensure HA WebSocket API accessible from test machine.
- **Entity not found**: Confirm ESPHome device is online and using expected entity IDs; adjust tests + docs if IDs change.
- **Service call failures**: Check ESPHome logs; ensure service names match `packages/services_calibration.yaml` definitions.

Need broader repository context? See `../../AGENTS.md`.
