# Phase 1 Completion Steps

> **Note:** This historical checklist remains for deep detail. For a consolidated Phase 1–3 scorecard, see
> `docs/development-scorecard.md`.

This document outlines the remaining tasks to complete Phase 1 implementation of the bed presence detection system.

## Phase 1 Changelog

- Established firmware build, flashing, and Home Assistant connectivity, including unit and E2E coverage for the
  z-score algorithm.
- Captured a vacant-bed baseline (μ = 6.30%, σ = 2.56%) with `scripts/collect_baseline.py` and updated
  `bed_presence.h` with the measured constants.
- Reflashed the device on 2025-11-06 and validated live detection against an occupied/empty bed scenario.
- Documented the known no-debounce limitation and queued Phase 2 work to add the state machine and timers.
- **Current status:** ✅ Phase 1 is complete and validated; active development has shifted to Phase 2.

## Current Status

**Completed:**
- ✅ Phase 1 firmware compiled and flashed to M5Stack
- ✅ Device connected to Home Assistant via WiFi
- ✅ All Phase 1 entities visible in Home Assistant (with `bed_presence_detector_` prefix)
- ✅ LD2410 sensor operational and reading values
- ✅ Unit tests passing (14 tests covering z-score logic)
- ✅ SSH access configured via Cloudflare Tunnel
- ✅ Development environment set up on Ubuntu node
- ✅ E2E integration test framework set up (entity names corrected)
- ✅ Baseline data collection script created (`scripts/collect_baseline.py`)
- ✅ Baseline statistics collected (2025-11-05 22:36:52):
  - **Mean (μ):** 6.30%
  - **Std Dev (σ):** 2.56%
  - **Samples:** 30 over 60 seconds
  - **Environment:** Queen bed, right nightstand, empty bed, door closed, user 25-30ft away
- ✅ C++ code updated with calibrated baseline values (bed_presence.h lines 40-47)
- ✅ Firmware recompiled with new baseline
- ✅ Firmware flashed via USB (2025-11-06)

**Remaining Tasks:** None. Phase 1 is complete and the deliverables below are retained for historical reference.

---

## Step 1: E2E Integration Testing

**Objective:** Verify Home Assistant integration and entity functionality

### Prerequisites

1. **Get Home Assistant Long-Lived Access Token**
   - Navigate to: Profile → Security → Long-Lived Access Tokens
   - Click "Create Token"
   - Name it: "Bed Presence E2E Testing"
   - Copy the token (it will only be displayed once)
   - Store securely (password manager recommended)

2. **Set up environment on Ubuntu node**
   ```bash
   ssh ubuntu-node
   cd ~/bed-presence-sensor/tests/e2e

   # Install test dependencies (if not already done)
   pip install -r requirements.txt
   ```

3. **Configure environment variables**
   ```bash
   # Local network access (recommended - faster)
   export HA_URL="ws://YOUR_NODE_IP:8123/api/websocket"

   # OR via Cloudflare Tunnel (if testing remotely)
   export HA_URL="ws://your-domain.com:8123/api/websocket"

   # Set the access token
   export HA_TOKEN="your-long-lived-access-token"
   ```

### Run Tests

```bash
cd ~/bed-presence-sensor/tests/e2e
pytest -v
```

### Expected Results

**Should PASS:**
- ✅ `test_ha_connection()` - WebSocket connection to Home Assistant
- ✅ `test_entities_exist()` - All Phase 1 entities are registered
- ✅ `test_sensor_values()` - LD2410 sensors report numeric values
- ✅ `test_threshold_entities_exist()` - k_on and k_off number entities exist
- ✅ `test_update_threshold_via_service()` - Can update thresholds dynamically
- ✅ `test_reset_to_defaults()` - Reset thresholds to default values (4.0, 2.0)
- ✅ `test_presence_binary_sensor()` - bed_occupied binary sensor exists

**Should SKIP:**
- ⏭️ `test_calibration_helpers_exist()` - Phase 3 feature (input_number helpers)
- ⏭️ `test_full_calibration_flow()` - Phase 3 automated calibration

### Troubleshooting

**Connection errors:**
- Verify HA_URL is correct (check IP address or domain)
- Test WebSocket connection: `wscat -c $HA_URL`
- Verify token is valid: Check in HA Profile → Security

**Entity not found errors:**
- Go to HA Developer Tools → States
- Search for entities starting with `sensor.ld2410_` and `number.k_`
- Verify M5Stack device is online: Settings → Devices → M5Stack Basic

**Import errors:**
- Reinstall dependencies: `pip install -r requirements.txt`
- Verify `homeassistant-api>=4.0.0` is installed

---

## Step 2: Baseline Data Collection

**Objective:** Collect statistical baseline of LD2410 still_energy readings with empty bed

### Why This Matters

Phase 1 uses hardcoded placeholder values for baseline statistics:
```cpp
float mu_move_{100.0f};   // Mean moving energy (PLACEHOLDER)
float sigma_move_{20.0f}; // Std dev moving energy (PLACEHOLDER)
```

These values are used for z-score calculation:
```cpp
z = (current_energy - mu_move_) / sigma_move_
```

**Without proper calibration, presence detection won't work correctly.**

### Collection Procedure

1. **Prepare the environment**
   - Ensure bed is completely empty (no people, pets, or objects)
   - Close bedroom door to minimize external movement
   - Let environment settle for 1-2 minutes

2. **Monitor sensor readings**

   **Method A: Home Assistant UI**
   - Go to Developer Tools → States
   - Find `sensor.ld2410_still_energy`
   - Watch the value for 60 seconds
   - Record values every 2-3 seconds (aim for 20-30 samples)

   **Method B: Via API** (recommended for automation)
   ```bash
   # Set environment variables first
   export HA_URL="ws://YOUR_NODE_IP:8123/api/websocket"
   export HA_TOKEN="your-token"

   # Create a simple collection script
   cd ~/bed-presence-sensor
   python3 << 'EOF'
   import requests
   import time
   import json
   from statistics import mean, stdev

   HA_IP = "YOUR_NODE_IP"  # Update this
   TOKEN = "your-token"     # Update this

   headers = {
       "Authorization": f"Bearer {TOKEN}",
       "Content-Type": "application/json"
   }

   samples = []
   print("Collecting baseline samples (60 seconds)...")
   print("Make sure the bed is empty!")
   time.sleep(3)  # Give user time to step away

   for i in range(30):  # 30 samples over 60 seconds
       try:
           response = requests.get(
               f"http://{HA_IP}:8123/api/states/sensor.ld2410_still_energy",
               headers=headers
           )
           if response.status_code == 200:
               value = float(response.json()['state'])
               samples.append(value)
               print(f"Sample {i+1}/30: {value}")
           time.sleep(2)
       except Exception as e:
           print(f"Error: {e}")

   if len(samples) >= 10:
       mu = mean(samples)
       sigma = stdev(samples)
       print(f"\n{'='*50}")
       print(f"BASELINE STATISTICS:")
       print(f"  Mean (μ):              {mu:.2f}")
       print(f"  Std Dev (σ):           {sigma:.2f}")
       print(f"  Samples collected:     {len(samples)}")
       print(f"  Min value:             {min(samples):.2f}")
       print(f"  Max value:             {max(samples):.2f}")
       print(f"{'='*50}")
       print(f"\nUpdate bed_presence.h with:")
       print(f"  float mu_move_{{{mu:.2f}f}};")
       print(f"  float sigma_move_{{{sigma:.2f}f}};")
   else:
       print(f"\nNot enough samples collected ({len(samples)}). Need at least 10.")
   EOF
   ```

3. **Record the statistics**
   - Note the mean (μ) and standard deviation (σ)
   - Save these values - you'll need them for the next step

### Expected Values

Typical LD2410 still_energy readings with empty bed:
- **Mean (μ):** 0-50 (depends on room layout and sensor position)
- **Std Dev (σ):** 5-20 (natural variation in sensor readings)

**Note:** Your values will differ based on:
- Sensor placement and angle
- Room size and furniture
- Background electromagnetic noise
- Mattress type and bedding

---

## Step 3: Baseline Calibration and Reflash

**Objective:** Update firmware with actual baseline statistics

### Update C++ Code

1. **SSH into the node**
   ```bash
   ssh ubuntu-node
   cd ~/bed-presence-sensor
   ```

2. **Edit the header file**
   ```bash
   nano esphome/custom_components/bed_presence_engine/bed_presence.h
   ```

3. **Find and update these lines** (around line 43-46):
   ```cpp
   // Hardcoded baseline statistics (update after baseline collection)
   float mu_move_{100.0f};   // Replace with your calculated mean
   float sigma_move_{20.0f}; // Replace with your calculated std dev
   ```

   **Example** (with collected values):
   ```cpp
   // Baseline collected 2025-11-05 with empty bed
   float mu_move_{25.3f};    // Mean still energy (empty bed)
   float sigma_move_{8.7f};  // Std dev still energy (empty bed)
   ```

4. **Save and exit** (Ctrl+X, then Y, then Enter in nano)

### Recompile and Flash

```bash
cd ~/bed-presence-sensor/esphome
source ~/esphome-venv/bin/activate

# Compile and flash
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

### Verify Update

Monitor the logs to ensure successful flash:
```bash
esphome logs bed-presence-detector.yaml --device /dev/ttyACM0
```

Look for:
- Device boots successfully
- Connects to WiFi
- Connects to Home Assistant API
- LD2410 sensor reporting values

---

## Step 4: Live Presence Detection Testing

**Objective:** Validate Phase 1 presence detection with actual occupancy

### Test Procedure

1. **Monitor the entities in Home Assistant**
   - Go to Developer Tools → States
   - Watch these entities:
     - `sensor.ld2410_still_energy` - Raw sensor input
     - `text_sensor.presence_state_reason` - Z-score and decision logic
     - `binary_sensor.bed_occupied` - Presence detection output
     - `number.k_on_on_threshold_multiplier` - Current ON threshold
     - `number.k_off_off_threshold_multiplier` - Current OFF threshold

2. **Test Case 1: Empty Bed**
   - Ensure bed is empty
   - Wait 30 seconds
   - **Expected:** `binary_sensor.bed_occupied` = OFF
   - **Check:** `presence_state_reason` should show z-scores < k_off

3. **Test Case 2: Bed Occupied**
   - Get into bed and remain still
   - Wait 10-15 seconds
   - **Expected:** `binary_sensor.bed_occupied` = ON
   - **Check:** `presence_state_reason` should show z-scores > k_on

4. **Test Case 3: Exit Bed**
   - Exit the bed
   - Wait 10-15 seconds
   - **Expected:** `binary_sensor.bed_occupied` = OFF
   - **Check:** `presence_state_reason` should show z-scores < k_off

5. **Test Case 4: Threshold Tuning**
   - If detection is too sensitive (false positives):
     - Increase `k_on` (e.g., from 4.0 to 5.0)
     - Increase `k_off` (e.g., from 2.0 to 3.0)
   - If detection is not sensitive enough (false negatives):
     - Decrease `k_on` (e.g., from 4.0 to 3.0)
     - Decrease `k_off` (e.g., from 2.0 to 1.5)

### Expected Behavior (Phase 1)

**Normal operation:**
- ✅ Detects presence when someone is in bed
- ✅ Clears presence when bed is empty
- ✅ Thresholds are runtime-tunable via Home Assistant
- ✅ State reason shows z-score values for debugging

**Known Phase 1 limitations (expected):**
- ⚠️ "Twitchy" behavior - rapid state changes (no debouncing)
- ⚠️ False positives from movement near bed
- ⚠️ False negatives if person is completely still
- ⚠️ Sensitive to external vibrations

**These limitations are intentional in Phase 1 and will be addressed in Phase 2 (state machine + debouncing).**

### Troubleshooting

**Problem: Always shows vacant**
- Verify baseline statistics are correct
- Check that k_on is not too high (try lowering to 3.0)
- Look at `sensor.ld2410_still_energy` - is it changing when occupied?
- Check `text_sensor.presence_state_reason` - what z-scores are being calculated?

**Problem: Always shows occupied**
- Baseline statistics may be incorrect
- Try raising k_off threshold
- Check if sensor is picking up external movement (pets, fans, etc.)

**Problem: Random flipping between states**
- Expected in Phase 1 (no debouncing)
- Can partially mitigate by widening hysteresis gap: increase k_on, decrease k_off
- Full fix requires Phase 2 implementation

---

## Step 5: Documentation and Commit

**Objective:** Document calibration results and commit changes

### Update CLAUDE.md

Edit `/workspaces/bed-presence-sensor/CLAUDE.md` to reflect actual baseline values:

Find the section "Hardware Not Yet Tested" and update:
```markdown
## Hardware Testing Status

**Status**: ✅ **TESTED AND CALIBRATED**

### Baseline Statistics (Collected YYYY-MM-DD)

- **Mean (μ):** [your value]
- **Std Dev (σ):** [your value]
- **Samples:** [number of samples]
- **Environment:** [brief description - e.g., "queen bed, 10ft from window"]

### Phase 1 Validation Results

- ✅ Presence detection functional with calibrated baseline
- ✅ Threshold tuning verified (k_on: X.X, k_off: X.X)
- ✅ E2E tests passing
- ⚠️ Known limitations: [list any specific issues observed]

### Recommended Settings

Based on testing:
- **k_on:** [recommended value]
- **k_off:** [recommended value]
- **Notes:** [any specific observations or recommendations]
```

### Commit Changes

```bash
cd ~/bed-presence-sensor

# Commit updated baseline statistics
git add esphome/custom_components/bed_presence_engine/bed_presence.h
git commit -m "Update baseline statistics from calibration

Collected baseline data with empty bed:
- Mean (μ): [value]
- Std Dev (σ): [value]
- Samples: [count]
- Date: [YYYY-MM-DD]

Presence detection now properly calibrated for environment.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"

# Commit documentation updates
git add CLAUDE.md
git commit -m "Document Phase 1 calibration and testing results

- Add baseline statistics
- Document validation test results
- Note known Phase 1 limitations
- Provide recommended threshold settings

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"
```

---

## Phase 1 Completion Checklist

Use this checklist to track progress:

- [x] E2E integration tests run successfully
- [x] Baseline data collected (minimum 20 samples) - **30 samples collected**
- [x] Baseline statistics calculated (μ and σ) - **μ=6.30%, σ=2.56%**
- [x] C++ code updated with actual baseline values - **bed_presence.h updated**
- [x] Firmware recompiled and flashed - **Completed 2025-11-06**
- [ ] Presence detection tested with empty bed - **IN PROGRESS**
- [ ] Presence detection tested with occupied bed
- [ ] Threshold tuning validated
- [ ] Known limitations documented
- [ ] CLAUDE.md updated with results
- [ ] Changes committed to git
- [ ] Final verification: Device remains stable for 24 hours

---

## Next Steps After Phase 1

Once Phase 1 is complete and validated:

### Phase 2: State Machine + Debouncing
- Implement 4-state machine (IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF)
- Add configurable debounce timers (default: 3s ON, 5s OFF)
- Add number entities for debounce timer configuration
- Update unit tests for state machine logic
- Update dashboard with debounce controls

### Phase 3: Advanced Calibration
- ✅ Automated baseline collection service (`calibrate_start_baseline`)
- ✅ MAD (Median Absolute Deviation) statistical analysis
- ✅ Distance windowing (ignore specific zones)
- 🔜 Calibration history snapshots + persistence

---

## Support and Troubleshooting

### Getting Help

If you encounter issues:

1. **Check the logs**
   ```bash
   # Device logs
   esphome logs bed-presence-detector.yaml --device /dev/ttyACM0

   # Or in Home Assistant
   Settings → Devices → Bed Presence Detector → Logs
   ```

2. **Review documentation**
   - `docs/phase1-hardware-setup.md` - Hardware setup guide
   - `docs/troubleshooting.md` - Common issues and solutions
   - `docs/presence-engine-spec.md` - Technical specification

3. **Run unit tests**
   ```bash
   cd ~/bed-presence-sensor/esphome
   platformio test -e native
   ```

4. **Check sensor health**
   - Go to Developer Tools → States
   - Verify `sensor.ld2410_still_energy` is updating (not stale)
   - Check device diagnostics in Home Assistant

### Common Issues

**Sensor readings are 0 or not updating:**
- Check UART connections (GPIO16/17)
- Verify LD2410 has power (3.3V)
- Check ESPHome logs for LD2410 initialization errors

**WiFi connection drops:**
- Check WiFi signal strength (use 2.4GHz, not 5GHz)
- Verify credentials in `secrets.yaml`
- Consider adding a WiFi extender near the device

**Home Assistant connection lost:**
- Verify API encryption key matches
- Check if device is reachable: `ping YOUR_DEVICE_IP`
- Restart Home Assistant or ESPHome device

---

**Document Version:** 1.0
**Last Updated:** 2025-11-05
**Phase:** 1 (Z-Score Based Detection)
**Status:** Ready for execution
