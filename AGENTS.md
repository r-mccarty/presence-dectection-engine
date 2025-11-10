# AGENTS.md – Repository-Wide Agent Guidance

**Note:** This file provides directory-specific guidance for AI agents. For comprehensive project context, **always read `CLAUDE.md` first** – it serves as your primary context map and documents the complete project structure, workflow, and current state.

## Project Snapshot
- **Domain**: ESP32-based bed presence detection system using LD2410 mmWave sensor and ESPHome firmware.
- **Current phase**: **Phase 3 is DEPLOYED** and fully operational. Phase 1 and Phase 2 are complete.
- **Core behavior**: Z-score statistical analysis of `ld2410_still_energy` with a 4-state debounced state machine (`IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF`). All thresholds and debounce timers are runtime-tunable from Home Assistant.
- **Baselines**: μ_still = 6.7%, σ_still = 3.5% (calibrated 2025-11-06, empty bed). Defaults: k_on = 9.0, k_off = 4.0, on_debounce = 3s, off_debounce = 5s, absolute_clear = 30s, d_min = 0 cm, d_max = 600 cm.
- **Development Environment**: Two-machine workflow: GitHub Codespaces for code editing, ubuntu-node for firmware flashing and hardware access.
- **Languages**: C++ (custom component), YAML (ESPHome + HA), Python (tests + scripts), Markdown (docs).

## High-Level Workflow

**⚠️ CRITICAL:** This project uses a **two-machine workflow** (see `docs/DEVELOPMENT_WORKFLOW.md`):
- **Codespaces**: Code editing, git operations, documentation
- **ubuntu-node**: Firmware compilation, flashing (physical USB access), Home Assistant API access

### Standard Development Flow
1. **Read Context**: Start with `CLAUDE.md` for project overview, then consult relevant docs:
   - Algorithm/design → `docs/ARCHITECTURE.md`
   - Compilation/flashing → `docs/DEVELOPMENT_WORKFLOW.md`
   - Hardware/calibration → `docs/HARDWARE_SETUP.md`
   - Troubleshooting → `docs/troubleshooting.md`
2. **Edit Code**: Make changes in Codespaces (or local environment).
3. **Test Locally** (unit tests only in Codespaces):
   - `cd esphome && platformio test -e native` (C++ unit tests)
   - `yamllint esphome/ homeassistant/` (YAML validation)
4. **Commit & Push**: Standard git workflow.
5. **Deploy to Hardware** (must be done on ubuntu-node):
   - SSH to ubuntu-node: `ssh user@ubuntu-node`
   - Pull changes: `cd ~/bed-presence-sensor && git pull`
   - Flash firmware: `~/sync-and-flash.sh` or `~/flash-firmware.sh`
6. **Integration Testing** (on ubuntu-node):
   - `cd tests/e2e && pytest` (requires Home Assistant access)

## Repository Map

**📁 Each subdirectory has its own AGENTS.md with specific guidance** – always consult the local AGENTS.md when working in that directory.

```
/                    ─ CLAUDE.md (primary AI context), README.md (human docs), AGENTS.md (this file)
├── docs/            ─ AGENTS.md + comprehensive documentation
│   ├── ARCHITECTURE.md              ─ Technical design, algorithms, state machine
│   ├── DEVELOPMENT_WORKFLOW.md      ─ Two-machine workflow (Codespace ↔ ubuntu-node)
│   ├── HARDWARE_SETUP.md            ─ Hardware specs, wiring, calibration
│   ├── troubleshooting.md           ─ Common issues and solutions
│   ├── presence-engine-spec.md      ─ Source of truth for 3-phase engineering spec
│   └── [other docs]                 ─ Quickstart, FAQ, RFDs, deployment guides
├── esphome/         ─ AGENTS.md + firmware (custom component, YAML packages, PlatformIO tests)
│   ├── custom_components/bed_presence_engine/  ─ C++ implementation
│   ├── packages/                    ─ Modular YAML configuration
│   ├── test/                        ─ C++ unit tests (14 scenarios, all passing)
│   └── bed-presence-detector.yaml   ─ Main ESPHome config
├── homeassistant/   ─ AGENTS.md + Lovelace dashboard, automation blueprint
├── tests/e2e/       ─ AGENTS.md + Python integration tests (require live Home Assistant)
├── hardware/        ─ CAD placeholders for mounts/enclosures (0-byte placeholders)
└── scripts/         ─ Utility scripts (collect_baseline.py, etc.)
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
- **Binary sensor stuck OFF**: Verify baseline calibration, check z-scores in logs, adjust `k_on` threshold, review `docs/troubleshooting.md`.
- **Sensor oscillates/flaps**: Increase debounce timers, widen hysteresis (gap between `k_on`/`k_off`), check state machine logs.
- **Cannot compile/flash**: Must be on ubuntu-node with physical USB access (see `docs/DEVELOPMENT_WORKFLOW.md`).
- **PlatformIO fails on first run**: Allow dependency download or delete `.pio` and retry.
- **E2E pytest cannot connect**: Run on ubuntu-node with Home Assistant access, verify `HA_URL` + `HA_TOKEN` in `.env.local`.
- **Secrets file confusion**: `.env.local` = HA API access (Python scripts), `secrets.yaml` = WiFi credentials (firmware). Ubuntu-node is source of truth for both.

## Essential Reading Order

**Start here every time:**
1. **`CLAUDE.md`** – Primary AI context map, project overview, current status, documentation roadmap
2. **Task-specific docs**:
   - Algorithm questions → `docs/ARCHITECTURE.md`
   - Code changes/flashing → `docs/DEVELOPMENT_WORKFLOW.md`
   - Hardware/calibration → `docs/HARDWARE_SETUP.md`
   - Debugging → `docs/troubleshooting.md`
   - Environment setup → `CONTRIBUTING.md`
3. **Subdirectory `AGENTS.md`** – Directory-specific guidance (you **must** follow when editing those areas)
4. **`docs/presence-engine-spec.md`** – Engineering roadmap and detailed requirements for all 3 phases

**For users:** `README.md` provides human-friendly project overview and deployment instructions.
