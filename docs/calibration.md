# Calibration Guide

Reliable calibration keeps the z-score based detector stable across different rooms and beds. Phase 3 introduced
fully automated, on-device calibration so you no longer need to edit firmware or run external scripts—everything
happens through ESPHome services and Home Assistant.

## Phase Overview

- **Phase 1 & 2** – Manual script + firmware edit path (still available for debugging or data export)
- **Phase 3** – Automated MAD calibration, distance windowing, and reset services (current production workflow)

> **Recommendation:** Use the Phase 3 automated workflow for all new calibrations. The legacy script is only
> needed when you want raw CSV data or offline analysis.

---

## Phase 3 Automated Workflow (Recommended)

### 1. Enable the Calibration Wizard
1. Copy `homeassistant/configuration_helpers.yaml` into your HA configuration (or include it via `!include`).
2. Reload helpers/scripts and ensure the new entities (input_select/input_boolean/input_number/input_datetime + scripts)
   exist in Developer Tools → States.
3. Open the **Bed Presence Detector** dashboard → **Calibration Wizard** tab.

### 2. Prepare the environment
1. Position the LD2410 sensor where it will remain.
2. Empty the bed completely (no people, pets, or large objects).
3. Set `number.bed_presence_detector_distance_min_cm` / `distance_max_cm` to cover only the bed zone.

### 3. Run the wizard
1. Toggle **I confirm the bed is empty**.
2. Adjust the **Calibration Duration** slider (60 seconds default; 90–120 seconds helps in noisy rooms).
3. Press **Start Baseline**. The wizard:
   - Calls `esphome.bed_presence_detector_calibrate_start_baseline` with the selected duration
   - Tracks progress via `input_select.bed_presence_calibration_step`
   - Automatically calls `esphome.bed_presence_detector_calibrate_stop` when the timer completes
4. Watch the `Wizard Status` card or `sensor.bed_presence_detector_presence_change_reason` for
   `calibration:completed` (success) or `calibration:insufficient_samples` (retry needed).

### 4. Review & Validate
1. The automation updates `input_datetime.bed_presence_last_calibration` whenever a calibration completes.
2. Check `sensor.bed_presence_detector_ld2410_still_energy`—empty-bed readings should hover around 0 z-score.
3. Lie in the bed to confirm PRESENT → DEBOUNCING_OFF transitions feel correct. If not, rerun the wizard or fine-tune `k_on`/`k_off`.

### 5. Manual fallback & resets
- Need to intervene? Use the dashboard's **Cancel** button (`script.bed_presence_cancel_baseline_calibration`).
- Roll back to defaults with **Reset Defaults** (`script.bed_presence_reset_calibration_defaults`, which wraps
  `esphome.bed_presence_detector_calibrate_reset_all`).
- Prefer Developer Tools → Services? Call `esphome.bed_presence_detector_calibrate_start_baseline` /
  `..._calibrate_stop` directly—the wizard is a friendly wrapper over those services.

---

## Legacy Script Workflow (Optional)

The original Phase 1/2 process is still included for operators who want raw sample files or need to run entirely
offline. Workflow summary:

1. SSH to ubuntu-node and run `python3 scripts/collect_baseline.py` (requires `HA_TOKEN`).
2. The script samples 30 readings over 60 seconds, prints μ/σ, and writes `baseline_results.txt`.
3. Manually copy the `mu_*` / `sigma_*` constants into `bed_presence.h`, recompile, and flash firmware.

Use this path only when you must capture data outside of ESPHome (e.g., to compare against external analytics).
Otherwise, rely on the automated services to keep firmware and Home Assistant completely in sync.

---

## Maintenance Cadence

- Re-run the calibration service whenever you relocate the sensor, change bedding significantly, or notice drift.
- Keep a note of the `calibration:completed` timestamp (visible in ESPHome logs and the change-reason sensor) for
  troubleshooting.
- Seasonal HVAC changes: rerun the automated calibration and optionally snapshot the resulting μ/σ in your release
  notes for traceability.
