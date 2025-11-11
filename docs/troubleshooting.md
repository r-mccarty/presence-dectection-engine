# Troubleshooting Guide

**Current Status**: Phase 3 (Automated Calibration + Hardening) - Deployed and operational; Phase 3.1+ persistence/analytics work is now in planning.

This guide covers common issues and solutions for the bed presence detection system. The system uses a **3-phase architecture** with Phase 3 currently live in production.

## System Architecture Overview

The presence detection engine uses:
- **Z-score statistical analysis** for signal normalization
- **4-state machine** (IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF)
- **Temporal filtering** with configurable debounce timers
- **Hysteresis** to prevent rapid oscillation
- **Distance window filtering** so only targets within `d_min`/`d_max` affect decisions
- **Change-reason telemetry** so every transition/calibration advertises why it happened

**Current Configuration**:
- Baseline: μ=6.7%, σ=3.5% (empty bed)
- ON threshold: k_on=9.0 (z-score > 9.0 triggers DEBOUNCING_ON)
- OFF threshold: k_off=4.0 (z-score < 4.0 triggers DEBOUNCING_OFF)
- ON debounce: 3 seconds (sustained high signal required)
- OFF debounce: 5 seconds (sustained low signal required)
- Absolute clear delay: 30 seconds (prevents premature clearing)
- Distance window: d_min=0 cm, d_max=600 cm (frames outside this range are ignored)

## Phase 3.1+ Objectives (WIP)
- Persist calibration snapshots (μ/σ results + metadata) for auditability and rollback
- Layer in optional moving-energy fusion / restlessness analytics once telemetry stabilizes
- Add operational monitoring for calibration drift, offline sensors, or repeated abs-clear delays

## Common Issues

### Device Not Connecting to Wi-Fi

**Symptoms**: Device doesn't appear in Home Assistant, fallback AP is active

**Solutions**:
1. Verify Wi-Fi credentials in `esphome/secrets.yaml`
2. Check that your Wi-Fi network is 2.4GHz (ESP32 doesn't support 5GHz)
3. Check Wi-Fi signal strength at the device location
4. Review logs via USB serial connection: `esphome logs bed-presence-detector.yaml`

**Debug**: Check ESPHome logs for WiFi connection errors. The device should show its IP address on successful connection.

### False Positives (Detects presence when bed is empty)

**Symptoms**: Sensor reports occupied when bed is vacant

**Root Causes**:
- Baseline statistics (μ, σ) don't match actual empty bed conditions
- ON threshold (k_on) is too low
- Environmental interference (nearby movement, fans, HVAC)
- ON debounce timer too short
- Distance window is too wide, allowing hallways/fans into the detection zone

**Solutions**:
1. **Recalibrate baseline**: Collect new baseline data with empty bed (see `docs/calibration.md`)
2. **Increase k_on threshold**: Adjust `number.k_on_on_threshold_multiplier` in Home Assistant (try 10.0 or 11.0)
3. **Increase ON debounce timer**: Adjust `number.on_debounce_ms` to 5000 (5 seconds) to require longer sustained signal
4. **Check environment**: Ensure no pets, fans, or movement in sensor range during calibration
5. **Verify sensor position**: Sensor should point directly at bed, not at doorway or hallway
6. **Tighten the distance window**: Raise `number.distance_min_cm` or lower `number.distance_max_cm` until only the mattress area is in range

**Debug**: Monitor `text_sensor.presence_state_reason` to see real-time z-scores and state transitions. Z-scores should be near 0 when bed is empty.

### False Negatives (Doesn't detect presence when occupied)

**Symptoms**: Sensor reports vacant when someone is in bed

**Root Causes**:
- Baseline statistics don't match actual sensor readings
- ON threshold (k_on) is too high
- Sensor position doesn't have clear line of sight to occupant
- Person lying very still (edge case)
- Distance window excludes the occupant (d_min too high or d_max too low)

**Solutions**:
1. **Recalibrate baseline**: Collect new baseline data (current baseline may be incorrect)
2. **Decrease k_on threshold**: Adjust `number.k_on_on_threshold_multiplier` to 8.0 or 7.0
3. **Verify sensor mounting**: Sensor should have unobstructed view of bed surface
4. **Check raw sensor values**: Monitor `sensor.ld2410_still_energy` - should be significantly higher (>30%) when occupied
5. **Adjust absolute clear delay**: If clearing too quickly when still, increase `number.abs_clear_delay_ms` to 60000 (60 seconds)
6. **Widen the distance window**: Lower `number.distance_min_cm` or raise `number.distance_max_cm` until occupied readings show up reliably

**Debug**:
- Check `sensor.ld2410_still_energy` raw values (should be 30-70% when occupied, 3-10% when vacant)
- Calculate expected z-score: `(energy - 6.7) / 3.5` should be > 9.0 when occupied
- Monitor state transitions in `text_sensor.presence_state_reason`

### Sensor Responds Too Slowly

**Symptoms**: Takes several seconds to detect presence after getting into bed

**Root Causes**:
- ON debounce timer too long (default: 3 seconds)
- Person not moving enough to generate strong signal initially

**Solutions**:
1. **Decrease ON debounce timer**: Adjust `number.on_debounce_ms` to 2000 (2 seconds) or even 1000 (1 second)
2. **Decrease k_on threshold slightly**: Lower threshold = more sensitive detection
3. **Check sensor position**: Ensure sensor captures initial movement when getting into bed

**Trade-off**: Shorter debounce timers increase false positive risk from transient movement near bed.

### Sensor Clears Too Quickly When Lying Still

**Symptoms**: Sensor reports vacant while person is still in bed but not moving

**Root Causes**:
- Absolute clear delay too short (default: 30 seconds)
- OFF threshold (k_off) too high
- Person lying very still generates low signal

**Solutions**:
1. **Increase absolute clear delay**: Adjust `number.abs_clear_delay_ms` to 60000 (60 seconds) or higher
2. **Decrease k_off threshold**: Adjust `number.k_off_off_threshold_multiplier` to 3.0 or lower
3. **Monitor z-scores during stillness**: Check `text_sensor.presence_state_reason` to see actual z-scores when lying still

**How it works**: The absolute clear delay requires BOTH low z-score AND 30+ seconds since last high confidence reading before clearing.

### Sensor Still "Twitchy" After Phase 3 Upgrade

**Symptoms**: Rapid state changes despite debounce timers

**Root Causes**:
- Debounce timers too short for your environment
- Thresholds too close together (insufficient hysteresis)
- Environmental interference causing fluctuating signals

**Solutions**:
1. **Increase all debounce timers**:
   - `on_debounce_ms` → 5000 (5 seconds)
   - `off_debounce_ms` → 10000 (10 seconds)
   - `abs_clear_delay_ms` → 60000 (60 seconds)
2. **Widen hysteresis gap**: Increase k_on to 10.0, decrease k_off to 3.0 (gap = 7.0 std deviations)
3. **Recalibrate baseline in stable conditions**: Minimize environmental interference during calibration

**Debug**: Check logs for state transitions - should see DEBOUNCING_ON/DEBOUNCING_OFF states lasting full timer duration before transitioning.

### Dashboard Not Showing Data

**Symptoms**: Dashboard loads but shows "unknown" or missing data

**Solutions**:
1. **Verify ESPHome device is online**: Settings → Devices & Services → ESPHome → "Bed Presence Detector"
2. **Check entity names**: Phase 3 entities should include:
   - `binary_sensor.bed_occupied` (main presence sensor)
   - `text_sensor.presence_state_reason` (debug info with z-scores and state)
   - `text_sensor.presence_change_reason` (why the last transition/calibration happened)
   - `sensor.ld2410_still_energy` (raw sensor reading)
   - `sensor.ld2410_still_distance` (raw still distance, used for the window)
   - `number.k_on_on_threshold_multiplier` (ON threshold tuning)
   - `number.k_off_off_threshold_multiplier` (OFF threshold tuning)
   - `number.on_debounce_ms` (ON debounce timer)
   - `number.off_debounce_ms` (OFF debounce timer)
   - `number.abs_clear_delay_ms` (absolute clear delay timer)
   - `number.distance_min_cm` (lower bound for valid targets)
   - `number.distance_max_cm` (upper bound for valid targets)
3. **Check Home Assistant logs**: Configuration → Logs → Filter for "bed_presence" or "bed_occupied"
4. **Verify dashboard entity IDs match**: Edit dashboard YAML and ensure entity names match your device

### Codespaces build fails with `unable to find user vscode`

**Symptoms**: When launching a GitHub Codespace that uses a custom devcontainer image, the build fails and drops into a recovery container with the log message `unable to find user vscode: no matching entries in passwd file`.

**Solutions**:
1. Update your `.devcontainer/Dockerfile` to create a non-root user named `vscode` (UID/GID `1000` by default) and install `sudo`. Example for Debian-based images:
   ```dockerfile
   FROM debian:bullseye-slim
   ENV DEBIAN_FRONTEND=noninteractive
   RUN apt-get update \
       && apt-get -y install --no-install-recommends sudo git curl \
       && rm -rf /var/lib/apt/lists/*
   ARG USERNAME=vscode
   ARG USER_UID=1000
   ARG USER_GID=$USER_UID
   RUN groupadd --gid $USER_GID $USERNAME \
       && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
       && echo "$USERNAME ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME \
       && chmod 0440 /etc/sudoers.d/$USERNAME
   USER $USERNAME
   ```
2. Alternatively, if you are using a pre-built image that already contains a non-root user (e.g., `node`), set `"remoteUser"` in `.devcontainer/devcontainer.json` to that user.
3. After applying either change, run **Codespaces: Rebuild Container** so the new user configuration is applied.

## Diagnostic Tools

### Presence State Reason Text Sensor (Best Debug Tool)

The `text_sensor.presence_state_reason` entity provides real-time debugging information:

**Example output**:
```
PRESENT (z=12.34, high_conf_age=5000ms, abs_clear_ready=no)
```

**Fields**:
- **State**: Current state machine state (IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF)
- **z**: Current z-score (statistical significance of signal)
- **high_conf_age**: Milliseconds since last high confidence reading (used for absolute clear delay)
- **abs_clear_ready**: Whether absolute clear delay has elapsed (yes/no)

**How to use**:
1. Open Home Assistant → Developer Tools → States
2. Search for `text_sensor.presence_state_reason`
3. Monitor in real-time as you test the sensor

**What to look for**:
- Empty bed: z should be near 0 (typically -2.0 to 2.0)
- Occupied bed: z should be > 9.0 (k_on threshold)
- State transitions: Should see DEBOUNCING_ON → PRESENT after 3 seconds
- Clearing: Should see DEBOUNCING_OFF → IDLE after 5 seconds + 30 second absolute delay

For a one-line summary of why the last transition occurred, also watch
`text_sensor.bed_presence_detector_presence_change_reason` (values like `on:threshold_exceeded`,
`off:abs_clear_delay`, `calibration:completed`).

### ESPHome Logs

View real-time logs from the device:
```bash
cd esphome
esphome logs bed-presence-detector.yaml
```

**What to look for**:
- WiFi connection status and IP address
- Sensor initialization messages
- State transitions (if enabled in code)
- Error messages or warnings

### Home Assistant Developer Tools

**States Tool** (Developer Tools → States):
- Monitor all entity values in real-time
- Check `sensor.ld2410_still_energy` raw readings
- Verify number entity values (k_on, k_off, debounce timers)

**Services Tool** (Developer Tools → Services):
- Test adjusting thresholds without dashboard
- Trigger calibration/reset services (`...calibrate_start_baseline`, `...calibrate_stop`, `...calibrate_reset_all`)

### Web Server

The device exposes a web server at `http://<device-ip>` for quick diagnostics:
- View current sensor readings
- Check device uptime and WiFi status
- No historical data (use Home Assistant for that)

### Manual Z-Score Calculation

To verify threshold behavior:

1. Note current `sensor.ld2410_still_energy` value (e.g., 45.0%)
2. Calculate z-score: `(energy - μ) / σ`
3. Example: `(45.0 - 6.7) / 3.5 = 10.94`
4. Compare to thresholds:
   - z > 9.0 → Should trigger DEBOUNCING_ON
   - z < 4.0 → Should trigger DEBOUNCING_OFF

## Phase-Specific Troubleshooting

### Phase 2 Recap: State Machine + Debouncing

Even though Phase 3 is deployed, these checks help when the core state machine misbehaves.

**Symptoms unique to Phase 2**:
- States stuck in DEBOUNCING_ON or DEBOUNCING_OFF
- Timers not elapsing properly
- Absolute clear delay preventing expected clearing

**Debug checklist**:
1. Verify all 5 number entities exist and have correct values
2. Check `text_sensor.presence_state_reason` for state and timing info
3. Confirm signal is sustained above/below threshold for full debounce duration
4. For clearing issues: Check `high_conf_age` exceeds `abs_clear_delay_ms` (30000ms default)

### Phase 3 (Current): Automated Calibration + Distance Windowing

**Features live in production firmware:**
- `esphome.bed_presence_detector_calibrate_start_baseline` collects still-energy samples (duration_s > 0) within the configured distance window.
- MAD statistics derive μ/σ; results are applied immediately and annotated via `Presence State Reason`.
- `Presence Change Reason` reports `calibration:started`, `calibration:completed`, or `calibration:insufficient_samples`.
- Distance windowing is runtime-tunable via `number.distance_min_cm` / `number.distance_max_cm`, filtering any frames outside `[d_min, d_max]`.
- `esphome.bed_presence_detector_calibrate_reset_all` restores known-good defaults across all number entities.

**Troubleshooting tips:**
1. **"Insufficient samples"** – Increase `duration_s` or widen the distance window temporarily so the sensor sees the bed area.
2. **"Duration must be > 0" error** – Developer Tools → Services requires an integer > 0. Recommended: 60–120 seconds.
3. **Unexpected μ/σ values** – Ensure the bed is empty and there is no major movement within the detection zone during sampling.
4. **Distance window confusion** – If every frame is filtered out, temporarily set `distance_min_cm=0` and `distance_max_cm=600`, calibrate, then tighten the window again.

## Known Issues and Limitations

### Calibration History Persistence (Pending)

**Status:** The guided Home Assistant wizard is live, but baseline snapshots are still volatile (lost on reboot unless recorded manually). (Tracked as a Phase 3.1+ objective.)

**Issue:** μ/σ values derived during calibration only live in RAM. Operators who forget to note the `Last Calibration` timestamp or resulting statistics may lose traceability after a power cycle.

**Workaround:**
1. Use the wizard's status card to copy the completion timestamp into release notes or HA logbook.
2. Store μ/σ readings in Home Assistant helpers (input_text/input_number) or automation traces after each calibration run.
3. Keep the distance window tight; rerun the wizard any time the environment changes significantly.

**Planned fix:** Persist baseline snapshots in flash (device side) and optionally surface history sensors in Home Assistant.

---

### Empty Hardware Assets

**Issue:** The following files exist but are 0-byte placeholders:

**Files:**
- `hardware/mounts/m5stack_side_mount.stl` - 3D printable mount
- `docs/assets/wiring_diagram.png` - Wiring diagram image
- `docs/assets/demo.gif` - Demo animation

**Impact:** Users cannot access 3D printable mounts or visual wiring diagrams.

**Workaround:**
- Wiring instructions are available in text format in `docs/HARDWARE_SETUP.md`
- Mount design can be created based on M5Stack dimensions

**Planned fix:** Community contributions welcome for hardware assets.

---

### Network Access Limitations from Codespaces

**Issue:** GitHub Codespaces cannot directly access local Home Assistant instance at 192.168.0.148:8123.

**Reason:** Codespaces runs on GitHub's cloud infrastructure, separate from your local network.

**Impact:**
- Cannot browse Home Assistant web UI directly from Codespaces
- Python scripts cannot connect to HA API without tunneling

**Workaround:**

**For web UI access:**
```bash
# In Codespace terminal
ssh -L 8123:192.168.0.148:8123 ubuntu-node
# Then browse to http://localhost:8123
```

**For Python scripts:**
```bash
# Option 1: Run directly on ubuntu-node (recommended)
ssh ubuntu-node "cd ~/bed-presence-sensor && python3 scripts/collect_baseline.py"

# Option 2: Use SSH tunnel from Codespace
# Terminal 1: ssh -L 8123:192.168.0.148:8123 ubuntu-node
# Terminal 2: python3 scripts/collect_baseline.py
```

**Detailed information:** See [DEVELOPMENT_WORKFLOW.md](DEVELOPMENT_WORKFLOW.md) for complete network access guide.

---

### E2E Tests Incomplete

**Issue:** Python E2E integration tests framework exists but some tests are incomplete or skipped.

**Files:** `tests/e2e/test_calibration_flow.py`

**Status:**
- ✅ Test framework functional
- ✅ Phase 1/2/3 entity + service tests working
- ⚠️ Calibration wizard helper tests skipped (UI exists but is not yet covered)

**Impact:** Automated coverage does not exercise the live HA wizard, so regressions in the helper/dashboard flow still require manual verification. Device-side calibration remains fully tested.

**Workaround:** Keep using the Home Assistant wizard for user workflows and rely on Developer Tools → Services or scripts for regression testing until the UI tests are implemented.

---

### Variable Naming Inconsistency (Resolved)

**Historical issue:** In early Phase 1 code, variables were named `mu_move_` and `sigma_move_` despite using still energy.

**Resolution:** Fixed in Phase 2 with semantic correction:
- `mu_move_` → `mu_still_` (lines 60 in bed_presence.h)
- `sigma_move_` → `sigma_still_` (lines 61 in bed_presence.h)

**Current status:** ✅ Resolved, variable names now match algorithm (still energy only).

---

## Getting Help

If you continue to experience issues:

1. **Gather diagnostic info**:
   - Current value of `text_sensor.presence_state_reason`
   - Raw `sensor.ld2410_still_energy` readings (both empty and occupied)
   - Current threshold values (k_on, k_off)
   - Current debounce timer values
2. **Check documentation**:
   - FAQ: [faq.md](faq.md)
   - Calibration guide: [calibration.md](calibration.md)
   - Phase 2 completion guide: [phase2-completion-steps.md](phase2-completion-steps.md)
   - Architecture: [ARCHITECTURE.md](ARCHITECTURE.md)
   - Development workflow: [DEVELOPMENT_WORKFLOW.md](DEVELOPMENT_WORKFLOW.md)
3. **Review GitHub Issues**: Check for similar reported issues
4. **Report issues** with:
   - Full diagnostic info above
   - ESPHome logs (anonymize WiFi credentials)
   - Steps to reproduce the issue
