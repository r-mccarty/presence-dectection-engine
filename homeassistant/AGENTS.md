# AGENTS.md – Home Assistant Content

Guidance for agents editing Home Assistant assets in this directory (`homeassistant/`). **Always read `../CLAUDE.md` first**, then consult:
- `docs/ARCHITECTURE.md` for entity structure and configuration
- `docs/quickstart.md` for user-facing setup instructions
- `docs/presence-engine-spec.md` for engineering requirements

## What Lives Here
```
homeassistant/
├── blueprints/automation/bed_presence_automation.yaml   # Automation blueprint for presence events
├── dashboards/bed_presence_dashboard.yaml               # Lovelace dashboard configuration
└── configuration_helpers.yaml                           # HA helper + script definitions for the calibration wizard
```

## Current Status: Phase 3 DEPLOYED

**Phase 3 firmware is fully operational.** The dashboard and blueprint expose:

### Primary Entities
- **Binary Sensor**: `binary_sensor.bed_presence_detector_bed_occupied` (debounced presence state)
- **State Reason**: `text_sensor.bed_presence_detector_presence_state_reason` (z-score + timer diagnostics)
- **Change Reason**: `text_sensor.bed_presence_detector_presence_change_reason` (reason codes for latest transition)

### Configuration Controls (Runtime Tunable)
- **Thresholds**:
  - `number.bed_presence_detector_k_on_on_threshold_multiplier` (default: 9.0)
  - `number.bed_presence_detector_k_off_off_threshold_multiplier` (default: 4.0)
- **Debounce Timers**:
  - `number.bed_presence_detector_on_debounce_ms` (default: 3000ms)
  - `number.bed_presence_detector_off_debounce_ms` (default: 5000ms)
  - `number.bed_presence_detector_absolute_clear_delay_ms` (default: 30000ms)
- **Distance Window**:
  - `number.bed_presence_detector_distance_min_cm` (default: 0cm)
  - `number.bed_presence_detector_distance_max_cm` (default: 600cm)

### Diagnostic Sensors
- `sensor.bed_presence_detector_ld2410_still_energy` (raw sensor data, percent)
- `sensor.bed_presence_detector_wifi_signal_percent` (connectivity)
- `sensor.bed_presence_detector_uptime` (device uptime)
- `text_sensor.bed_presence_detector_esphome_version` (firmware version)
- `text_sensor.bed_presence_detector_presence_change_reason` (recent state-change cause)

### Phase 3 (Firmware + Wizard Deployed)
- Calibration wizard view is live in `dashboards/bed_presence_dashboard.yaml` (see **Calibration Wizard** tab).
- `configuration_helpers.yaml` defines input helpers, scripts, and automations that wrap ESPHome services.
- Buttons + scripts call the same `calibrate_start_baseline`/`calibrate_stop` services that firmware exposes.

## Editing Guidelines
- **YAML style**: 2-space indentation, descriptive `name`/`label` strings (these render directly in HA UI)
- **Entity ID consistency**: Match ESPHome defaults. If renaming entities in firmware:
  1. Update `esphome/packages/presence_engine.yaml`
  2. Update this dashboard and blueprint
  3. Update `docs/ARCHITECTURE.md` and `docs/quickstart.md`
  4. Update `tests/e2e/test_calibration_flow.py`
- **Dashboard organization**: Group by workflow: Status → Configuration → Debounce Timers → Diagnostics
- **Blueprint documentation**: Document all inputs thoroughly in `input:` blocks
- **Testing changes**: Validate YAML in HA raw editor before committing

## Blueprint Expectations

`blueprints/automation/bed_presence_automation.yaml`:
- **Trigger**: State changes of `binary_sensor.bed_presence_detector_bed_occupied`
- **Actions**: Separate sequences for presence detected (→ on) and presence cleared (→ off)
- **Filters**: Optional time windows, person presence conditions
- **Wizard integration**: Reuse `script.bed_presence_start_baseline_calibration` or related helpers when automating calibration from blueprints

When modifying:
- Test blueprint import in Home Assistant UI
- Update example automations in `docs/quickstart.md`
- Document all inputs with clear descriptions
- Maintain backward compatibility when possible

## Dashboard Highlights

`dashboards/bed_presence_dashboard.yaml` structure:

1. **Status View**
   - Binary sensor state (bed occupied/clear)
   - Text sensor with z-score reason (e.g., "PRESENT: z=12.3, debounce satisfied")
   - History graph (24-hour presence timeline)

2. **Configuration View**
   - Threshold sliders (k_on, k_off)
   - Markdown explanation of z-score calculation
   - Current baseline values (μ = 6.7%, σ = 3.5%)

3. **Debounce Timers View** (Phase 2 addition)
   - On debounce timer (milliseconds)
   - Off debounce timer (milliseconds)
   - Absolute clear delay (milliseconds)
   - Markdown explanation of temporal filtering

4. **Diagnostics View**
   - Still energy sensor (raw percentage)
   - WiFi signal strength
   - Device uptime
   - ESPHome version
   - IP address

5. **Calibration Wizard** – Guided workflow driven by helper entities (confirmation toggle, duration slider, start/cancel/reset buttons, status + timestamp card)

**Critical**: Ensure all default values match firmware (k_on=9.0, k_off=4.0, timers=3s/5s/30s). Update `docs/quickstart.md` screenshots if layout changes.

## Testing Changes

### YAML Validation
```bash
# From repository root
yamllint homeassistant/

# Quick syntax check
yq eval homeassistant/dashboards/bed_presence_dashboard.yaml
```

### Live Testing (Requires ubuntu-node)
1. Copy updated YAML to Home Assistant:
   - Dashboard: HA UI → Settings → Dashboards → Raw Configuration Editor
   - Blueprint: HA UI → Settings → Automations & Scenes → Blueprints → Import
2. Verify UI rendering and entity availability
3. Test blueprint automation creation
4. Check that number controls update firmware (watch ESPHome logs)

### Integration Tests
```bash
# Run on ubuntu-node (requires Home Assistant access)
cd tests/e2e
pytest -v
```

## Troubleshooting

### Entity Issues
- **Dashboard cards missing entities**: Confirm ESPHome device online, check entity IDs match `esphome/packages/presence_engine.yaml`
- **Entity names changed**: Update dashboard YAML, blueprint YAML, docs, and E2E tests
- **Controls not persisting**: Check Home Assistant recorder/history configuration

### Blueprint Issues
- **Blueprint not triggering**: Verify automation enabled, check entity mapping, review automation traces in HA UI
- **Wrong entity IDs in automation**: Re-import blueprint after entity changes, recreate automation from blueprint

### Configuration Issues
- **Debounce controls not applying**: Verify ESPHome firmware has `update_*` methods, check HA logs for service call errors
- **Thresholds reset on reboot**: Ensure ESPHome template numbers have `restore_value: true`
- **Calibration services return errors**: Confirm `duration_s > 0`, check ESPHome logs for `calibration:insufficient_samples`, widen the distance window temporarily if needed

For detailed troubleshooting, see `../docs/troubleshooting.md`. For repo-wide context, see `../AGENTS.md`.
