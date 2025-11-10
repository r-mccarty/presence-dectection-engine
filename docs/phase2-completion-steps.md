# Phase 2 Completion Steps

**Implementation Status**: Phase 2 COMPLETE ✅
**Deployment Status**: DEPLOYED TO HARDWARE ✅
**Testing Status**: Physical Integration Tests PENDING ⏳
**Date**: 2025-11-07
**Implementation**: 4-State Machine with Temporal Filtering

---

## Deployment Summary

✅ **Completed 2025-11-07**:
- Firmware compiled successfully (1.0 MB, 54.8% flash usage)
- Deployed to M5Stack @ 192.168.0.180
- Connected to Home Assistant @ 192.168.0.148
- All 7 entities verified in Home Assistant
- State machine operational (DEBOUNCING_ON → PRESENT observed)

⏳ **Pending**:
- Physical integration testing (4 test scenarios in Step 3 below)
- Real-world tuning based on usage patterns

---

## Overview

Phase 2 transforms the Phase 1 immediate z-score detection into a robust state machine with debouncing and temporal filtering. This eliminates the "twitchy" behavior of Phase 1 while maintaining the core statistical significance approach.

## What Changed in Phase 2

### Core Features Added

1. **4-State Machine**
   - `IDLE`: No presence detected (binary sensor: OFF)
   - `DEBOUNCING_ON`: High signal detected, timer running (binary sensor: OFF)
   - `PRESENT`: Confirmed presence (binary sensor: ON)
   - `DEBOUNCING_OFF`: Low signal detected, timer running (binary sensor: ON)

2. **Temporal Filtering**
   - **On Debounce**: Must see z_still >= k_on for 3+ seconds before turning ON
   - **Off Debounce**: Must see z_still < k_off for 5+ seconds before turning OFF
   - **Absolute Clear Delay**: Must wait 30 seconds since last high-confidence signal before starting off-debounce

3. **Runtime Configuration**
   - All 3 debounce timers exposed as Home Assistant number entities
   - Persistent across reboots
   - No firmware reflash needed to adjust timing

### Technical Changes

**C++ Implementation** (`esphome/custom_components/bed_presence_engine/`)
- `bed_presence.h`: Added state machine enum, timer variables, setter methods
- `bed_presence.cpp`: Replaced if/else with switch-based state machine
- Semantic fix: Renamed `mu_move_`/`sigma_move_` → `mu_still_`/`sigma_still_`

**Configuration** (`esphome/packages/presence_engine.yaml`)
- Added 3 new `number` entities: on_debounce_ms, off_debounce_ms, abs_clear_delay_ms
- Default values: 3000ms, 5000ms, 30000ms

**Dashboard** (`homeassistant/dashboards/bed_presence_dashboard.yaml`)
- Added "Debounce Timers" card to Configuration view

**Unit Tests** (`esphome/test/test_presence_engine.cpp`)
- Complete rewrite with time mocking
- 14 tests covering state machine, debouncing, absolute clear delay

---

## Deployment Steps

### Prerequisites

- ✅ Phase 2 code committed to git repository
- ✅ Ubuntu-node has SSH access configured
- ✅ Firmware compiles successfully
- ✅ Unit tests pass (14/14)

### Step 1: Sync and Flash Firmware ✅ COMPLETE

**On Ubuntu-node** (via SSH):

```bash
ssh ubuntu-node
cd ~/bed-presence-sensor
git pull origin main  # Sync Phase 2 changes
cd esphome
source ~/esphome-venv/bin/activate
esphome run bed-presence-detector.yaml --device /dev/ttyACM0
```

**Or use helper script:**

```bash
ssh ubuntu-node
~/sync-and-flash.sh
```

**Expected output:**
- Compilation succeeds
- Firmware uploads to M5Stack
- Device reboots
- Logs show: "Setting up Bed Presence Engine (Phase 2)..."
- Logs show debounce timer values: "Debounce timers: on=3000ms, off=5000ms, abs_clear=30000ms"

### Step 2: Verify Home Assistant Integration ✅ COMPLETE

**Deployment completed 2025-11-07**. All entities verified present and functional.

**Check new entities exist:**

1. Settings → Devices & Services → ESPHome → Bed Presence Detector
2. Verify 3 new entities:
   - `number.bed_presence_detector_on_debounce_ms` ✅ (value: 3000)
   - `number.bed_presence_detector_off_debounce_ms` ✅ (value: 5000)
   - `number.bed_presence_detector_absolute_clear_delay_ms` ✅ (value: 30000)
3. Verify existing entities still work:
   - `binary_sensor.bed_presence_detector_bed_occupied` ✅ (state: on)
   - `sensor.bed_presence_detector_presence_state_reason` ✅ (shows z-score and debounce info)
   - `number.bed_presence_detector_k_on_on_threshold_multiplier` ✅
   - `number.bed_presence_detector_k_off_off_threshold_multiplier` ✅
   - `sensor.bed_presence_detector_presence_change_reason`

**Update Dashboard:**

1. Settings → Dashboards → Bed Presence Detector
2. Edit Dashboard → Configuration view
3. Verify "Debounce Timers" card appears with 3 controls

If dashboard doesn't auto-update:
1. Edit Dashboard → Raw Configuration Editor
2. Copy content from `homeassistant/dashboards/bed_presence_dashboard.yaml`
3. Paste and save

### Step 3: Test Phase 2 Behavior ⏳ PENDING

**Status**: Physical integration testing not yet performed. Firmware is deployed and entities are verified, but real-world debouncing behavior needs validation.

**Test 1: Quick Motion (Should NOT Trigger)**

1. Watch Home Assistant dashboard
2. Quickly wave hand over sensor for 1-2 seconds
3. **Expected**: `binary_sensor.bed_occupied` remains OFF
4. **Reason**: Debounce timer not satisfied (need 3+ seconds)

**Test 2: Sustained Presence (Should Trigger)**

1. Place hand over sensor and hold steady for 5+ seconds
2. **Expected**: After 3 seconds, `binary_sensor.bed_occupied` turns ON
3. **Expected**: `text_sensor.presence_state_reason` shows "ON: z=X.XX, debounced 3000ms"
4. Check ESPHome logs:
   ```
   [D][bed_presence_engine:064] IDLE → DEBOUNCING_ON (z=4.25 >= k_on=9.0)
   [I][bed_presence_engine:080] DEBOUNCING_ON → PRESENT: ON: z=4.25, debounced 3000ms
   ```

**Test 3: Getting Into Bed (Absolute Clear Delay)**

1. Trigger presence by getting into bed
2. Wait for binary sensor to turn ON
3. Lie very still (simulate sleep)
4. **Expected**: Sensor remains ON for at least 30 seconds even if z-score drops below k_off
5. **Reason**: Absolute clear delay prevents premature clearing after recent movement

**Test 4: Getting Out of Bed (Off Debounce)**

1. While sensor is ON, get out of bed
2. Wait 30+ seconds for absolute clear delay
3. Observe logs: "PRESENT → DEBOUNCING_OFF"
4. Wait 5 more seconds
5. **Expected**: After total 35+ seconds, sensor turns OFF
6. **Expected**: Reason shows "OFF: z=X.XX, debounced 5000ms"

### Step 4: Tune Debounce Timers (Optional)

Phase 2 allows runtime tuning without reflashing:

**If sensor is still too twitchy:**
- Increase `on_debounce_ms` to 5000 (5 seconds)
- Increase `abs_clear_delay_ms` to 60000 (60 seconds)

**If sensor is too slow to respond:**
- Decrease `on_debounce_ms` to 2000 (2 seconds)
- Decrease `abs_clear_delay_ms` to 15000 (15 seconds)

**To adjust:**
1. Settings → Devices → Bed Presence Detector
2. Find number entity (e.g., "On Debounce Timer (ms)")
3. Enter new value
4. Changes take effect immediately (no reboot needed)

---

## Validation Checklist

Use this checklist to confirm Phase 2 is working correctly:

- [ ] **Firmware compiles** without errors
- [ ] **Unit tests pass** (14/14 tests)
- [ ] **Device connects** to Home Assistant
- [ ] **3 new entities** appear in Home Assistant
- [ ] **Quick motion** does NOT trigger sensor (< 3 seconds)
- [ ] **Sustained presence** triggers sensor after 3 seconds
- [ ] **State transitions** appear in logs (IDLE → DEBOUNCING_ON → PRESENT)
- [ ] **Binary sensor** only changes on confirmed transitions (not during debouncing)
- [ ] **Getting out of bed** takes 35+ seconds to clear (30s abs_clear + 5s off_debounce)
- [ ] **Dashboard** shows debounce timer controls
- [ ] **Runtime tuning** works (changing timer value affects behavior immediately)

---

## Troubleshooting

### Issue: Sensor still twitchy after Phase 2 upgrade

**Symptoms**: Binary sensor oscillates rapidly ON/OFF

**Diagnosis**: Debounce timers may be too short, or thresholds (k_on/k_off) may need adjustment

**Solution**:
1. Check current debounce timer values (should be 3000, 5000, 30000)
2. Increase on_debounce_ms to 5000 (5 seconds)
3. Increase abs_clear_delay_ms to 60000 (60 seconds)
4. If still twitchy, adjust k_on/k_off thresholds (increase k_on to 10.0 or higher)

### Issue: Sensor too slow to respond

**Symptoms**: Takes too long to detect presence when getting into bed

**Diagnosis**: On debounce timer too long

**Solution**:
1. Decrease on_debounce_ms to 2000 (2 seconds)
2. Test with sustained motion (get into bed, lie down)
3. If false positives occur, increase back to 3000

### Issue: Sensor clears too quickly when lying still

**Symptoms**: Binary sensor turns OFF while person is still in bed but not moving

**Diagnosis**: Absolute clear delay too short, or z-score dropping below k_off during stillness

**Solution**:
1. Increase abs_clear_delay_ms to 60000 (60 seconds) or higher
2. Check z-score values in `text_sensor.presence_state_reason` during stillness
3. If z-score is consistently low during stillness, may need to recalibrate baseline (see Phase 1 calibration)

### Issue: New entities don't appear in Home Assistant

**Symptoms**: Only see old Phase 1 entities (k_on, k_off) but not debounce timers

**Diagnosis**: ESPHome device not fully re-registered with Home Assistant

**Solution**:
1. Check ESPHome device logs for errors
2. Settings → Devices → Bed Presence Detector → Delete Device
3. Re-flash firmware: `ssh ubuntu-node "~/sync-and-flash.sh"`
4. Device should auto-discover with all Phase 2 entities

### Issue: Logs show "Phase 1" instead of "Phase 2"

**Symptoms**: Setup logs show "Setting up Bed Presence Engine (Phase 1)..."

**Diagnosis**: Old firmware still on device (git sync or flash failed)

**Solution**:
1. Verify Phase 2 code is in repository: `git log -1 --oneline`
2. Re-sync on ubuntu-node: `ssh ubuntu-node "cd ~/bed-presence-sensor && git pull"`
3. Re-flash: `ssh ubuntu-node "~/sync-and-flash.sh"`
4. Check logs again after reboot

---

## State Machine Diagram

```
┌──────────────────────────────────────────────────────────────────────┐
│                    Phase 2: 4-State Machine                          │
└──────────────────────────────────────────────────────────────────────┘

                     z_still >= k_on
                ┌─────────────────────┐
                │                     │
                ▼                     │
    ┌───────────────────┐             │
    │       IDLE        │◄────────────┤
    │  (Binary: OFF)    │             │
    └───────────────────┘             │
                │                     │
                │ z_still >= k_on     │
                ▼                     │
    ┌───────────────────┐             │
    │  DEBOUNCING_ON    │             │
    │  (Binary: OFF)    │             │
    │  Timer running    │             │
    └───────────────────┘             │
                │                     │
     Timer >= on_debounce_ms          │
      & z_still >= k_on               │
                │                     │
                ▼                     │
    ┌───────────────────┐             │
    │     PRESENT       │             │
    │  (Binary: ON)     │             │
    │  Update high_conf │             │
    └───────────────────┘             │
                │                     │
     z_still < k_off &                │
     time since high_conf             │
       >= abs_clear_delay             │
                │                     │
                ▼                     │
    ┌───────────────────┐             │
    │  DEBOUNCING_OFF   │             │
    │  (Binary: ON)     │             │
    │  Timer running    │             │
    └───────────────────┘             │
                │                     │
     Timer >= off_debounce_ms         │
      & z_still < k_off               │
                │                     │
                └─────────────────────┘

Reset Conditions (abort debounce):
- DEBOUNCING_ON → IDLE:    if z_still < k_on (lost signal)
- DEBOUNCING_OFF → PRESENT: if z_still >= k_on (signal returned)
```

---

## Phase 2 vs Phase 1 Comparison

| Feature | Phase 1 | Phase 2 |
|---------|---------|---------|
| **State Management** | Boolean flag | 4-state machine |
| **Response Time** | Immediate (< 1 second) | 3-5 seconds (debounced) |
| **False Positives** | High (twitchy) | Low (filtered) |
| **False Negatives** | Low | Very low (absolute clear delay) |
| **Temporal Filtering** | None | Debouncing + absolute clear delay |
| **Binary Sensor Stability** | Changes immediately | Changes only on confirmed transitions |
| **Configurability** | Thresholds only (k_on, k_off) | Thresholds + 3 debounce timers |
| **Use Case** | Testing, proof of concept | Production deployment |

---

## Next Steps: Phase 3 (Now Deployed)

Phase 2 established the production-ready foundation. Phase 3 has since delivered:

1. **Automated Calibration** – ESPHome services (`calibrate_start_baseline`, `calibrate_reset_all`) collect samples, apply MAD-derived μ/σ, and broadcast change reasons.
2. **Distance Windowing** – Runtime `distance_min_cm` / `distance_max_cm` numbers filter frames outside the bed zone.
3. **MAD Statistics** – Outlier-resistant σ calculation replaces manual spreadsheet workflows.

Remaining wishlist items (Phase 3.1+):
- Home Assistant calibration wizard + helper entities
- Optional moving-energy fusion / restlessness metrics
- Flash persistence for calibration history

See `docs/presence-engine-spec.md` for the deployed specification and upcoming enhancements.

---

## References

- **Phase 2 Specification**: `docs/presence-engine-spec.md` (lines 62-215)
- **Phase 1 Completion**: `docs/phase1-completion-steps.md`
- **Hardware Setup**: `docs/phase1-hardware-setup.md`
- **Troubleshooting Guide**: `docs/troubleshooting.md`
- **Development Workflow**: `docs/ubuntu-node-setup.md`
