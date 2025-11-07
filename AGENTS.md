# AGENTS.md

Guidance for AI coding agents working in this repository. For the full narrative history and detailed requirements, read `CLAUDE.md` (source of truth) and the user-facing `README.md` + `/docs` collection before you change anything.

## Project Snapshot
- **Domain**: ESPHome firmware + Home Assistant content for an ESP32 + LD2410 bed presence detector.
- **Current phase**: **Phase 2** is implemented and ready for deployment. Phase 1 is complete and Phase 3 (automated calibration) is still planned.
- **Core behavior**: Z-score analysis of `ld2410_still_energy` with a 4-state debounced state machine (`IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF`). Thresholds (`k_on`, `k_off`) and debounce timers (on/off/absolute-clear) are runtime-tunable from Home Assistant.
- **Baselines**: `mu_still = 6.7`, `sigma_still = 3.5` collected 2025-11-06. Defaults: `k_on = 9.0`, `k_off = 4.0`, `on_debounce = 3s`, `off_debounce = 5s`, `absolute_clear = 30s`.
- **Languages**: C++ (custom component), YAML (ESPHome + HA), Python (tests), Markdown (docs).

## High-Level Workflow
1. Review design + requirements (`CLAUDE.md`, `docs/presence-engine-spec.md`).
2. Make firmware changes inside `esphome/` (see subdirectory guidance) and keep Home Assistant + docs in sync when behavior changes.
3. Run tests:
   - `cd esphome && esphome compile bed-presence-detector.yaml`
   - `cd esphome && platformio test -e native`
   - `cd tests/e2e && pytest` (requires live Home Assistant; Phase 3 tests are skipped)
   - `yamllint esphome/ homeassistant/`
4. Update docs whenever defaults, workflows, or operator UX changes.

## Repository Map
```
/                    ─ Repo-wide docs (this file, CLAUDE.md, README.md)
├── esphome/         ─ Firmware (custom component, YAML packages, PlatformIO tests)
├── homeassistant/   ─ Lovelace dashboard, automation blueprint, helper templates
├── docs/            ─ Engineering spec, hardware setup, quickstart, troubleshooting
├── tests/e2e/       ─ Python integration tests that talk to Home Assistant
├── hardware/        ─ CAD placeholders for mounts/enclosures
└── scripts/         ─ Tooling helpers (see script headers before use)
```

## Coding Standards
- **C++**: Follow ESPHome style (CamelCase classes, snake_case functions/vars, trailing underscore on private members). Do not add exceptions/RTTI. Log with `ESP_LOG*` macros. All new logic must have matching unit tests in `esphome/test/`.
- **YAML**: 2-space indent, descriptive comments, keep entity IDs consistent with docs/UI. Update Lovelace dashboard + blueprints when entities change.
- **Python**: PEP 8, `black` formatting, prefer pytest idioms. Use async fixtures for HA WebSocket interactions when needed.
- **Markdown**: Present tense, ≤120 char lines, clearly call out phase-specific behavior.

## Commit & PR Expectations
- Conventional commits: `<type>: <subject>` (`feat`, `fix`, `docs`, `test`, `refactor`, `style`, `chore`, `ci`). Imperative subject ≤72 chars, no trailing period.
- Every behavior change **must** update the relevant documentation (README, docs, dashboards, blueprints) and tests.
- Include test commands you actually ran in the PR description.

## Troubleshooting Reminders
- **Binary sensor stuck OFF**: Verify baseline calibration, lower `k_on`, inspect ESPHome logs for z-score/state machine messages.
- **Sensor oscillates**: Increase debounce timers or widen hysteresis (gap between `k_on`/`k_off`).
- **PlatformIO fails on first run**: Allow dependency download or delete `.pio` and retry.
- **E2E pytest cannot connect**: Confirm `HA_URL` + `HA_TOKEN`, HA instance must expose WebSocket API.

## Read First / Quick Links
- `README.md` – high-level overview + deployment instructions.
- `CLAUDE.md` – authoritative technical reference and change log.
- `docs/presence-engine-spec.md` – engineering roadmap for Phases 1-3.
- Subdirectory `AGENTS.md` files – scoped guidance you **must** follow when editing those areas.
