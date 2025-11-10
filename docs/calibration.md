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

### 1. Prepare the environment
1. Position the LD2410 sensor exactly where it will live.
2. Empty the bed completely (no people, pets, or large objects).
3. Set `number.bed_presence_detector_distance_min_cm` / `distance_max_cm` so the intended bed zone is inside the
   window and nearby noise sources (doorway, fan) are excluded.
4. Open Home Assistant → Developer Tools → Services or use the ESPHome web UI for the device.

### 2. Start baseline collection
Call either service (both map to the same device-side implementation):

```
service: esphome.bed_presence_detector_calibrate_start_baseline
data:
  duration_s: 60
```

Guidelines:
- `duration_s` must be > 0. 60 seconds is the default; 90–120 seconds is helpful in noisy rooms.
- Samples are captured only when frames fall inside the configured distance window.
- You can also call the legacy alias `esphome.bed_presence_detector_start_calibration`.

### 3. Monitor progress
- **ESPHome logs** (`esphome logs bed-presence-detector.yaml`) show sample counts and MAD summary.
- **Text sensors**:
  - `sensor.bed_presence_detector_presence_state_reason` reports verbose status (`Calibration complete: μ=...`).
  - `sensor.bed_presence_detector_presence_change_reason` transitions to `calibration:started`, then
    `calibration:completed` or `calibration:insufficient_samples`.
- The device auto-finalizes when the timer expires or when you call `esphome.bed_presence_detector_calibrate_stop`.

### 4. Validate the new baseline
1. Check Home Assistant graphs—`ld2410_still_energy` z-scores should sit near 0 with an empty bed.
2. Lie in the bed; the binary sensor should enter PRESENT after the on-debounce interval using the new μ/σ.
3. If anything looks off, rerun calibration or adjust `k_on`/`k_off` until satisfied.

### 5. Reset if needed
If you ever need to roll back to the known-good defaults:

```
service: esphome.bed_presence_detector_calibrate_reset_all
```

This resets μ/σ, thresholds, debounce timers, and distance window, and republishes the default values to the
Home Assistant number entities. The legacy alias `esphome.bed_presence_detector_reset_to_defaults` remains
available.

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
