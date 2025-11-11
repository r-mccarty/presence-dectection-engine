# AI Agent Context Guide

This is the canonical orientation document for every AI agent working in this
repository. `CLAUDE.md`, `GEMINI.md`, and the root `AGENTS.md` all point to this
same content—consult any one of them before you start a task.

## 1. Current Snapshot
- **Product**: ESP32 + LD2410 bed-presence detector with ESPHome firmware and Home
  Assistant integrations.
- **Phase**: Phase 3 (automated calibration, distance windowing, change-reason
  telemetry) is fully deployed. Phase 3.1+ efforts focus on calibration
  persistence, analytics, and operational monitoring. For historical details and
  remaining wishlist items, see `docs/development-scorecard.md`.
- **Baseline defaults**: μ=6.7%, σ=3.5%, `k_on=9.0`, `k_off=4.0`,
  `on_debounce=3s`, `off_debounce=5s`, `abs_clear=30s`, distance window
  `[0cm, 600cm]`. Confirm any future changes in `docs/ARCHITECTURE.md` and the
  ESPHome packages.

## 2. Orientation Checklist
1. **Understand the feature area**  
   Start with the document that matches your task:
   - `docs/ARCHITECTURE.md` – Presence-engine design, state machine, tests.
   - `docs/presence-engine-spec.md` – Formal multi-phase requirements.
   - `docs/development-scorecard.md` – Timeline + validation evidence.
   - `docs/DEVELOPMENT_WORKFLOW.md` – Two-machine (Codespaces ↔ ubuntu-node)
     workflow, flashing, network tunneling.
   - `docs/HARDWARE_SETUP.md` – Wiring, calibration environment.
   - `docs/troubleshooting.md` – Operational runbooks and diagnostics.
   - `docs/quickstart.md`, `docs/faq.md`, `docs/calibration.md` – Operator-facing
     onboarding and workflows.

2. **Check directory-specific guidance**  
   Every major subdirectory (`docs/`, `esphome/`, `homeassistant/`, `tests/e2e/`,
   etc.) has its own `AGENTS.md`. Follow those instructions before editing files
   inside the directory.

3. **Sync with current phase goals**  
   If your work touches lifecycle items (calibration history, analytics,
   monitoring), reference the Phase 3.1+ objectives in the scorecard and the
   corresponding sections in `docs/presence-engine-spec.md`.

## 3. Workflow Highlights
- **Two-machine rule**: Code editing and docs happen in Codespaces; firmware
  builds, flashing, and anything that needs Home Assistant access must run on
  `ubuntu-node`. Details live in `docs/DEVELOPMENT_WORKFLOW.md`.
- **Secrets**: `.env.local` (HA API) and `esphome/secrets.yaml` (WiFi/API/OTA)
  are sourced from ubuntu-node. Never push secrets; instructions reside in
  `CONTRIBUTING.md`.
- **Testing**:  
  - `cd esphome && platformio test -e native` for C++ unit tests.  
  - `yamllint esphome/ homeassistant/` for YAML validation.  
  - `tests/e2e/` suites (run on ubuntu-node) cover HA interactions; wizard UI
    flow still requires manual verification per `docs/troubleshooting.md`.
- **Coding standards**: ESPHome C++ style (trailing `_` for members), YAML
  2-space indent, Python formatted with Black/PEP 8. Add or update tests for any
  behavior changes and document them in the relevant guide.

## 4. When Updating Documentation
- Prefer targeted docs instead of bloating this file. Historical context → scorecard,
  workflow steps → `docs/DEVELOPMENT_WORKFLOW.md`, hardware specifics →
  `docs/HARDWARE_SETUP.md`, troubleshooting tips → `docs/troubleshooting.md`.
- Keep Markdown lines ≤120 chars, use fenced code blocks with language hints, and
  follow the warning pattern `> **Warning**: ...` as described in `docs/AGENTS.md`.

## 5. Getting Help
If you encounter conflicting instructions or unexpected repository state:
1. Re-read the directory `AGENTS.md`.
2. Check open issues/notes in `docs/development-scorecard.md`.
3. Ask the user for clarification before making assumptions that could desync
   firmware, documentation, and Home Assistant configuration.
