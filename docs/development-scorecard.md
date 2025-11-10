# Development Scorecard

This scorecard consolidates the historical Phase 1/Phase 2 completion notes and the current Phase 3
deployment status into a single reference. Use it to see what has been delivered, what validation
evidence exists, and what follow-up work remains.

## Phase Timeline At A Glance

| Phase | Date | Status | Highlights | Outstanding Items |
|-------|------|--------|------------|-------------------|
| Phase 1 – Z-Score Foundation | 2025-11-06 | ✅ Complete | Hardcoded μ/σ baseline, live HA connectivity, scripts + tests in place | Documented follow-up functional soak tests |
| Phase 2 – Debounced State Machine | 2025-11-07 | ✅ Deployed | 4-state engine with tunable debounce timers, dashboard controls, expanded unit suite | Physical multi-scenario validation + tuning |
| Phase 3 – Automated Calibration & Hardening | 2025-11-08 | ✅ Deployed | MAD-based calibration service, distance windowing, change reasons, reset helpers | Calibration history/persistence, optional moving-energy fusion |

For detailed historical logs, see `docs/phase1-completion-steps.md` and `docs/phase2-completion-steps.md`.

## Phase 1 — Z-Score Foundation (Complete)

### Delivered Scope
- Established end-to-end firmware flashing + HA connectivity workflow.
- Implemented simple z-score detector with runtime `k_on`/`k_off` threshold numbers.
- Captured first vacant-bed baseline (`μ=6.30%`, `σ=2.56%`, 30 samples) and hardcoded the values.
- Added baseline collection script (`scripts/collect_baseline.py`) and C++ unit tests validating z-score math.

### Validation Evidence
- ✅ PlatformIO unit suite covering z-score logic (14/14 pass).
- ✅ `tests/e2e` pipeline validated HA entity availability and update cadence (token workflow documented).
- ✅ Device reflashed 2025-11-06 and verified against occupied/empty transitions.

### Outstanding / Historical Follow-Ups
- Long-haul soak test of binary sensor in empty vs occupied state (tracked but optional once Phase 2 shipped).
- Documenting any environment-specific quirks discovered after Phase 2/3 upgrades (reference troubleshooting guide).

## Phase 2 — Debounced State Machine (Deployed)

### Delivered Scope
- Refactored C++ component into `IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF` machine.
- Added runtime-configurable debounce knobs (`on`, `off`, `absolute_clear`) surfaced in HA dashboard.
- Rebuilt unit tests with deterministic timer control and updated architecture docs.
- Synced deployment via ubuntu-node helper scripts; device online @ 192.168.0.180 with all 7 entities.

### Validation Evidence
- ✅ Firmware compile + flash logs (2025-11-07) show expected timer defaults and state transitions.
- ✅ Home Assistant dashboard verification: new number entities plus change-reason text sensor.
- ✅ Regression unit suite (14 tests) rewritten to cover debounce paths and reset behaviors.

### Remaining Items
- ⏳ Physical integration: four in-bed scenarios still queued for human-in-the-loop testing.
- 🎯 Real-world tuning once long-term occupancy data is captured (will inform future defaults).

## Phase 3 — Automated Calibration & Hardening (Deployed)

### Delivered Scope
- Added MAD-based calibration flow exposed via `esphome.bed_presence_detector_calibrate_start_baseline`.
- Implemented distance windowing with persistent `d_min_cm` / `d_max_cm` numbers to ignore fan zones.
- Added change-reason text sensor plus reset service returning all knobs to known-good defaults.
- Documented workflow in `docs/presence-engine-spec.md`, `docs/calibration.md`, and troubleshooting guide.

### Operational State
- Firmware v2025.11.08 running in production; defaults: `k_on=9.0`, `k_off=4.0`, `on=3s`, `off=5s`,
  `absolute_clear=30s`, `d_min=0cm`, `d_max=600cm`, baseline μ=6.7%, σ=3.5%.
- Automated calibration preferred over legacy scripts; ubuntu-node remains source of truth for secrets.
- Presence change reasons now surfaced in HA for debugging (e.g., `on:threshold_exceeded`).

### Remaining Capacity / Wishlist
- Persist calibration history snapshots for auditing + quick rollback.
- Explore optional moving-energy fusion / restlessness metrics once Phase 3 telemetry is stable.

## Forward Work (Phase 3.1+ Opportunities)
1. **Data persistence:** Store baseline snapshots and wizard outcomes (local flash + HA sensor history).
2. **Advanced analytics:** Investigate moving-energy fusion, restlessness scoring, or multi-target filtering.
3. **Operational monitoring:** Add alerts for calibration drift, sensor offline events, or excessive abs_clear hits.

Refer back to `docs/presence-engine-spec.md` for formal requirements and to `docs/ARCHITECTURE.md` for the
authoritative design details backing each phase.
