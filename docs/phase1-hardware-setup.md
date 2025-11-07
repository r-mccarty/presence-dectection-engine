# Phase 1 Hardware Setup Checklist

This document guides you through setting up and testing the Phase 1 firmware with actual hardware.

## Prerequisites

- ✅ M5Stack Basic v2.7
- ⏳ LD2410C mmWave radar sensor (arriving later today)
- ✅ ESPHome firmware compiled successfully
- ✅ Home Assistant running and accessible

## Step 1: Verify Current ESPHome Configuration

Before connecting the LD2410 sensor, verify your existing M5Stack firmware:

1. **Check M5Stack Connection**
   - Go to Home Assistant → Settings → Devices & Services → ESPHome
   - Verify `bed-presence-detector` is online
   - Note: It won't have presence detection yet (waiting for LD2410)

2. **Update secrets.yaml**
   - File: `esphome/secrets.yaml`
   - Replace placeholder values:
     - `wifi_ssid`: Your actual Wi-Fi network name
     - `wifi_password`: Your Wi-Fi password
     - `ota_password`: Choose a secure OTA update password
   - Keep the `api_encryption_key` as-is (or generate a new one)

## Step 2: Connect LD2410 to M5Stack

**Wiring configuration** (per `esphome/packages/hardware_m5stack_ld2410.yaml`):

| LD2410 Pin | M5Stack Pin | Purpose           |
|------------|-------------|-------------------|
| TX         | GPIO16 (RX) | UART receive      |
| RX         | GPIO17 (TX) | UART transmit     |
| VCC        | 5V          | Power supply      |
| GND        | GND         | Ground            |

**Important notes:**
- ⚠️ TX on LD2410 connects to RX on M5Stack (cross-over)
- ⚠️ RX on LD2410 connects to TX on M5Stack (cross-over)
- The LD2410 operates at 256000 baud (configured in YAML)
- Phase 1 and Phase 2 both use **still energy only** per [RFD-001](RFD-001-still-vs-moving-energy.md)

## Step 3: Collect Baseline Data

Before updating hardcoded μ and σ values, you need to collect baseline energy readings from an **empty bed**.

### 3A. Flash Current Firmware (Temporary)

```bash
cd esphome
esphome run bed-presence-detector.yaml
```

Choose your M5Stack device and flash.

### 3B. Observe Still Energy Values

The [presence engine spec](presence-engine-spec.md#phase-1-foundational-logic---from-energy-to-binary-state)
defines the z-score using still energy, so confirm that signal first.

1. Go to Home Assistant → Developer Tools → States.
2. Find `sensor.ld2410_still_energy`.
3. **Leave the bed completely empty** for 60 seconds.
4. **Record the values** you observe:
   - Typical empty-bed baseline should hover around **6-8%**.
   - Expect minor noise with standard deviation near **3-4%**.
   - Large spikes (>20%) usually indicate motion in the room—remove the cause and repeat.

### 3C. Calculate Baseline Statistics (Still Energy Only)

Phase 1 relies exclusively on still energy (see [RFD-001](RFD-001-still-vs-moving-energy.md)), so focus on estimating:

- **μ_still** – the mean still energy when the bed is empty.
- **σ_still** – the standard deviation of that still energy baseline.

> **Reference:** Our production baseline (collected 2025-11-06) measured μ_still = **6.7** and σ_still = **3.5**.
> Use your own measurements, but these values are a helpful sanity check.

**Example observations:**
```
Still energy readings (empty bed): 5.1, 6.4, 6.8, 7.5, 5.9, 6.2

Estimates:
μ_still ≈ 6.7
σ_still ≈ 3.4
```

## Step 4: Update Hardcoded Values

Edit `esphome/custom_components/bed_presence_engine/bed_presence.h`:

```cpp
// Replace with your observed still-energy baseline
float mu_still_{6.7f};    // Mean still energy (empty bed)
float sigma_still_{3.5f}; // Std dev still energy (empty bed)
```

**Note:** Even in Phase 1 we rely solely on `ld2410_still_energy`. Moving-energy fusion is deferred to Phase 3.

## Step 5: Recompile and Flash Phase 1 Firmware

```bash
cd esphome
esphome compile bed-presence-detector.yaml
esphome run bed-presence-detector.yaml
```

Wait for the device to reboot and reconnect to Home Assistant.

## Step 6: Verify Phase 1 Operation

### 6A. Check Entities in Home Assistant

You should now see:

**Binary Sensor:**
- `binary_sensor.bed_occupied` - Main presence detection (OFF initially)

**Number Inputs (tunable via UI):**
- `number.k_on_on_threshold_multiplier` - Default: 4.0
- `number.k_off_off_threshold_multiplier` - Default: 2.0

**Text Sensor:**
- `text_sensor.presence_state_reason` - Shows why state last changed

**Raw Sensors (for debugging):**
- `sensor.ld2410_still_energy` (primary Phase 1 signal)
- `sensor.ld2410_moving_energy` (available but ignored by the engine)
- `sensor.ld2410_moving_distance`
- etc.

### 6B. Test Detection

**Test 1: Immediate ON (no debouncing)**
1. With empty bed, note that `bed_occupied` is OFF
2. Wave your hand vigorously in front of the sensor
3. **Expected:** `bed_occupied` turns ON **immediately**
4. Check `presence_state_reason` - should show: `ON: z=X.XX > k_on=4.00`

**Test 2: Immediate OFF (twitchy behavior)**
1. While `bed_occupied` is ON
2. Remain perfectly still or move away
3. **Expected:** `bed_occupied` turns OFF **immediately** (within 1-2 seconds)
4. Check `presence_state_reason` - should show: `OFF: z=X.XX < k_off=2.00`

**Test 3: Hysteresis**
1. Turn ON the sensor
2. Move slowly and gently (moderate energy)
3. **Expected:** Sensor should stay ON even if energy drops between k_off and k_on
4. Only turns OFF when energy drops significantly below baseline

### 6C. Validate "Twitchy" Behavior

**This is expected for Phase 1!**

- Rapid ON/OFF transitions
- Responds to small movements
- No smoothing or filtering
- Sensitive to fan noise, AC vents, etc.

This validates the core z-score concept. Phase 2 will add temporal filtering.

## Step 7: Tune Thresholds (Optional)

If the sensor is too sensitive or not sensitive enough:

### Make it LESS sensitive (harder to trigger):
- Increase `k_on` from 4.0 → 5.0 or 6.0
- This requires a stronger signal (higher z-score) to turn ON

### Make it MORE sensitive (easier to trigger):
- Decrease `k_on` from 4.0 → 3.0 or 2.5
- This triggers ON with weaker signals

### Adjust hysteresis (prevent flapping):
- Keep `k_on` > `k_off` by at least 1.0-2.0
- Larger gap = more stable (but less responsive)
- Smaller gap = more responsive (but may flap)

**Adjust via Home Assistant:**
- Go to Settings → Devices & Services → ESPHome → bed-presence-detector
- Find the `number` entities and adjust sliders
- Changes take effect immediately (no reboot needed)

## Step 8: Monitor Logs (Debugging)

If something isn't working:

```bash
cd esphome
esphome logs bed-presence-detector.yaml
```

Look for:
- `[bed_presence_engine] Setting up Bed Presence Engine (Phase 1)...`
- `[bed_presence_engine] Baseline (still): μ=X.XX, σ=X.XX`
- `[bed_presence_engine] Presence detected: ON: z=X.XX > k_on=X.XX`
- `[bed_presence_engine] Presence cleared: OFF: z=X.XX < k_off=X.XX`

## Troubleshooting

### Sensor always OFF
- LD2410 may not be wired correctly (check TX/RX crossover)
- Baseline values (μ, σ) may be incorrect - recollect data
- k_on threshold may be too high - try lowering to 3.0

### Sensor always ON
- Baseline μ may be too high
- k_off threshold may be too high
- Check for nearby fans, AC vents causing constant motion

### Sensor not appearing in HA
- Check Wi-Fi credentials in `secrets.yaml`
- Verify M5Stack is on same network as Home Assistant
- Check ESPHome logs for connection errors

### Energy values are 0 or not updating
- LD2410 not powered correctly
- UART wiring reversed or incorrect
- Baud rate mismatch (should be 256000)

## Phase 1 Success Criteria

✅ **Phase 1 is complete when:**
1. `bed_occupied` sensor appears in Home Assistant
2. Sensor turns ON immediately when waving hand
3. Sensor turns OFF immediately when still
4. State is "twitchy" (no debouncing, rapid changes)
5. `presence_state_reason` shows z-scores in logs
6. Can tune `k_on` and `k_off` from HA UI

## Next Steps

Once Phase 1 is validated:
- Document what k_on/k_off values work best for your bed
- Note any false positives (fan, AC, etc.)
- Prepare for **Phase 2**: Temporal filtering and state machine
  - Will add debounce timers
  - Will eliminate "twitchy" behavior
  - Will require configuration of on/off debounce delays

## Quick Reference

**Default Phase 1 Parameters:**
- μ_move = 100.0 (placeholder - UPDATE THIS)
- σ_move = 20.0 (placeholder - UPDATE THIS)
- k_on = 4.0 (4 standard deviations to turn ON)
- k_off = 2.0 (2 standard deviations to turn OFF)

**Phase 1 Logic:**
```
z_score = (energy - μ) / σ
if (not occupied and z_score > k_on): turn ON
if (occupied and z_score < k_off): turn OFF
```

**Key Files:**
- Firmware config: `esphome/bed-presence-detector.yaml`
- Hardware config: `esphome/packages/hardware_m5stack_ld2410.yaml`
- C++ engine: `esphome/custom_components/bed_presence_engine/bed_presence.cpp`
- Baseline values: `bed_presence.h` lines 43-46
