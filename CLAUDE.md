# CLAUDE.md

Guidance for autonomous coding agents working in this repository.

## Project Snapshot
- **Domain**: ESPHome firmware + Home Assistant content for an ESP32 + LD2410 bed presence detector.
- **Roadmap status**: Phase 1 ✅ complete, Phase 2 ✅ live implementation, Phase 3 ⏳ planned.
- **Baselines**: `mu_still = 6.7`, `sigma_still = 3.5` (collected 2025-11-06).
- **Defaults**: `k_on = 9.0`, `k_off = 4.0`, `on_debounce = 3s`, `off_debounce = 5s`, `absolute_clear = 30s`.

## Agent Checklist
1. Read `README.md` for deployment context and `docs/presence-engine-spec.md` for requirements.
2. Modify firmware under `esphome/` and keep Home Assistant configs (`homeassistant/`) + docs in sync.
3. Run relevant tests before committing:
   - `cd esphome && esphome compile bed-presence-detector.yaml`
   - `cd esphome && platformio test -e native`
   - `cd tests/e2e && pytest` (requires live Home Assistant; Phase 3 cases skip today)
   - `yamllint esphome/ homeassistant/`
4. Follow conventional commits (`<type>: <subject>`) and include executed test commands in PR summaries.
5. Update documentation whenever defaults, workflows, or UX change.

## Repository Tour
```
/                    ─ Repo-wide docs (this file, README.md)
├── esphome/         ─ Firmware + PlatformIO tests
├── homeassistant/   ─ Dashboards, blueprints, helper templates
├── docs/            ─ Specs, quickstart, troubleshooting
├── tests/e2e/       ─ Python integration tests (partial)
├── hardware/        ─ CAD placeholders
└── scripts/         ─ Tooling helpers
```

### Firmware essentials
- Custom component: `esphome/custom_components/bed_presence_engine/`
- Unit tests: `esphome/test/test_presence_engine.cpp`
- Main YAML entry point: `esphome/bed-presence-detector.yaml`

### Home Assistant essentials
- Automation blueprint: `homeassistant/blueprints/automation/bed_presence_automation.yaml`
- Lovelace dashboard: `homeassistant/dashboards/bed_presence_dashboard.yaml`
- Helper templates: `homeassistant/configuration_helpers.yaml.example`

## Workflow Notes
- GitHub Codespaces is for editing + git; ubuntu-node (SSH) is for flashing hardware.
- Secrets live on ubuntu-node (`secrets.yaml` is gitignored). Never push secrets from hardware host back to repo.
- Run helper script `ssh ubuntu-node "~/sync-and-flash.sh"` after merging changes you want on-device.

## Calibration Quick Reference
- Baseline values above live in `bed_presence.h` and were captured with empty bed on 2025-11-06.
- Recalibrate via `ssh ubuntu-node "cd ~/bed-presence-sensor && python3 scripts/collect_baseline.py"`, update firmware constants, commit, then flash.

## Change Log & Deep Dives
For the full narrative history, roadmap details, and hardware deployment notes consult:
- `docs/presence-engine-spec.md`
- `docs/phase2-completion-steps.md`
- `docs/calibration.md`

These documents serve as the authoritative changelog and background reference.
