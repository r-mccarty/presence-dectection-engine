# AGENTS.md – Home Assistant Content

Guidance for agents editing the Home Assistant assets in this directory (`homeassistant/`). Check `../README.md`, `../CLAUDE.md`, and the docs (especially `docs/quickstart.md` and `docs/presence-engine-spec.md`) so UI and automations always reflect current firmware behavior.

## What Lives Here
```
homeassistant/
├── blueprints/automation/bed_presence_automation.yaml
├── dashboards/bed_presence_dashboard.yaml
└── configuration_helpers.yaml.example  # Phase 3 helpers (not active yet)
```

## Phase Alignment
- **Phase 2 firmware is live**. The dashboard and blueprint must expose:
  - Debounced binary sensor `binary_sensor.bed_occupied`
  - Threshold sliders (`number.k_on_on_threshold_multiplier`, `number.k_off_off_threshold_multiplier`)
  - Debounce timer controls (`number.on_debounce_timer_ms`, `number.off_debounce_timer_ms`, `number.absolute_clear_delay_ms`)
  - Diagnostic sensors (`sensor.ld2410_still_energy`, Wi-Fi metrics, uptime, ESPHome version)
- **Phase 3** calibration helpers remain planned: keep the commented calibration wizard section in the dashboard and helper template file in sync with firmware progress.

## Editing Guidelines
- YAML indentation = 2 spaces. Use descriptive `name`/`label` strings—these render directly in HA.
- Match entity IDs with ESPHome defaults. If you rename anything in firmware, update dashboard, blueprint, docs, and tests.
- Keep dashboard sections grouped by operator workflow: Status, Configuration, Debounce Timers, Diagnostics. When adding cards, confirm they render in HA raw editor preview.
- For blueprints, document inputs thoroughly (`input:` block comments) so end users understand how to customize automations.

## Blueprint Expectations
`blueprints/automation/bed_presence_automation.yaml`:
- Triggers on `state` changes of `binary_sensor.bed_occupied`.
- Offers separate action sequences for transitions to `on` and `off`.
- Optional time window + person presence filters.
- When you add new firmware features that need automations (e.g., calibration services), expose them as additional optional actions and update docs + README examples.

## Dashboard Highlights
`dashboards/bed_presence_dashboard.yaml` currently contains:
1. **Status View** – shows presence state, z-score reason text sensor, recent history graphs.
2. **Configuration View** – threshold sliders and descriptive markdown explaining z-score behavior.
3. **Debounce Timers View** – Phase 2 addition with number inputs for on/off debounce and absolute clear delay.
4. **Diagnostics View** – Wi-Fi signal, uptime, ESPHome version, IP address.
5. **Calibration Wizard (commented)** – Leave commented until Phase 3 services + helpers are implemented.

When editing, ensure card titles and markdown match actual defaults (k_on 9.0, k_off 4.0, debounce 3s/5s/30s). Update screenshots/docs if the layout changes materially.

## Testing Changes
- Validate YAML in HA raw editor or run `yamllint homeassistant/` from repo root.
- After updating dashboards/blueprints, copy into a local HA instance to confirm UI renders and blueprint import succeeds.
- If firmware entity IDs changed, rerun `tests/e2e` (requires live HA) to catch regressions.

## Troubleshooting Reminders
- Dashboard cards missing entities? Confirm ESPHome device is online and entity IDs match the YAML.
- Blueprint not triggering? Make sure the automation created from it has the correct device/entity mapping and that HA automations are enabled.
- Debounce controls not applying? Verify template numbers exist and firmware exposes matching `update_*` methods.

Need repository-wide expectations? See `../AGENTS.md`.
