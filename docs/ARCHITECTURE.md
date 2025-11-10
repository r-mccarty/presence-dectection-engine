# Project Architecture: Bed Presence Sensor

This document provides a comprehensive technical deep-dive into the bed presence detection system's architecture, algorithms, and implementation details.

## Table of Contents

- [Overview](#overview)
- [3-Phase Development Roadmap](#3-phase-development-roadmap)
- [Phase 2 Implementation Details](#phase-2-implementation-details)
- [C++ Class Structure](#c-class-structure)
- [Z-Score Calculation and Threshold Logic](#z-score-calculation-and-threshold-logic)
- [ESPHome Configuration Structure](#esphome-configuration-structure)
- [Testing Strategy](#testing-strategy)

---

## Overview

This system implements a **statistical presence detection engine** running on an ESP32 microcontroller. The core algorithm uses **z-score normalization** to convert raw mmWave radar readings into statistical significance, combined with a **4-state machine** for temporal filtering.

**Key Technical Characteristics:**
- **On-device processing**: All calculations run on ESP32, no cloud dependency
- **Real-time operation**: Sub-second latency from sensor reading to state change
- **Statistical robustness**: Z-score normalization handles environmental variations
- **Temporal filtering**: State machine with debouncing eliminates false positives/negatives
- **Runtime tunability**: All parameters adjustable via Home Assistant without reflashing

**Hardware:**
- **Microcontroller**: M5Stack Basic (ESP32-D0WDQ6-V3, 16MB Flash)
- **Sensor**: LD2410 mmWave radar (UART, 256000 baud)
- **Communication**: WiFi + ESPHome native API

---

## 3-Phase Development Roadmap

The project follows a structured 3-phase approach, with each phase building on the previous one. See `docs/presence-engine-spec.md` for the complete engineering specification.

### Phase 1: Z-Score Based Detection ✅ COMPLETE

**Objective:** Establish core statistical detection with hysteresis.

**Key Features:**
- Z-score calculation: `z = (current_energy - μ) / σ`
- Threshold-based state transitions: ON when `z > k_on`, OFF when `z < k_off`
- Hysteresis: Separate ON/OFF thresholds prevent oscillation (k_on > k_off)
- Runtime tunable thresholds via Home Assistant
- Immediate state transitions (intentionally "twitchy" for validation)

**Implementation:**
- Custom ESPHome C++ component (`bed_presence_engine`)
- Binary sensor publishes ON/OFF state
- Number entities for threshold tuning (k_on, k_off)
- Text sensor for state reason tracking

**Status:** Completed 2025-11-06, validated on hardware

---

### Phase 2: State Machine + Debouncing ✅ DEPLOYED

**Objective:** Eliminate "twitchiness" through temporal filtering and state management.

**Key Features:**
- **4-state machine**: IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF
- **On-debounce**: Sustained high signal (3s) required to turn ON
- **Off-debounce**: Sustained low signal (5s) required to turn OFF
- **Absolute clear delay**: Prevents premature clearing within 30s of last high confidence
- **Runtime tunable timers**: All debounce parameters adjustable via Home Assistant
- **State reason tracking**: Debug info includes z-score, timing, and state

**State Machine Diagram:**
```
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

**Implementation:**
- Enhanced C++ class with state machine enum
- Debounce timers with millisecond precision
- High confidence timestamp tracking
- State reason with timing information
- 5 additional number entities for timer configuration

**Status:** Deployed 2025-11-07, fully operational on hardware

---

### Phase 3: Automated Calibration ✅ DEPLOYED

**Objective:** Enable self-calibration and environmental adaptation without reflashing firmware.

**Implemented Features:**
- **Automated baseline calibration**: `esphome.bed_presence_detector_calibrate_start_baseline` collects still-energy data for N seconds, computes μ/σ via MAD, and updates runtime variables immediately.
- **MAD (Median Absolute Deviation)**: Resistant to outliers (e.g., a fan gust) when deriving σ. Minimum σ clamp prevents divide-by-zero.
- **Distance windowing**: Frames whose still-distance fall outside `[distance_min_cm, distance_max_cm]` are ignored before state machine + calibration logic.
- **Change-reason telemetry**: `text_sensor.presence_change_reason` publishes concise reason codes (`on:threshold_exceeded`, `off:abs_clear_delay`, `calibration:completed`).
- **Reset services**: `calibrate_reset_all` / `reset_to_defaults` restore μ/σ, thresholds, debounce timers, and distance window to known-good defaults while republishing HA numbers.

**Implementation Notes:**
- ESPHome services call new C++ helpers (`start_baseline_calibration`, `stop_baseline_calibration`, `reset_to_defaults`).
- Sample collection uses bounded vectors (max 4096 samples) and finalizes automatically when the duration expires, even if no new samples arrive.
- Distance window defaults to `[0cm, 600cm]` so existing deployments behave identically until tuned.
- Flash persistence remains a future enhancement; values survive until reboot thanks to runtime storage.

**Status:** Deployed 2025-11-08 alongside 16 C++ unit tests + new e2e coverage. Home Assistant calibration wizard + helpers (`homeassistant/configuration_helpers.yaml`) now wrap these services, leaving flash persistence as the remaining backlog item.

---

## Phase 2 Implementation Details

### Core Algorithm (bed_presence.cpp:49-128)

The Phase 2 presence engine implements a state machine with temporal filtering:

**1. Input Processing**
- Read `ld2410_still_energy` sensor value (0-100%)
- **Note**: Phase 1 and 2 use ONLY still energy (see RFD-001 for rationale)

**2. Z-Score Calculation**
```cpp
float z_still = (energy - mu_still_) / sigma_still_;
```

Where:
- `energy` = current sensor reading (e.g., 45.0%)
- `mu_still_` = baseline mean (e.g., 6.7%)
- `sigma_still_` = baseline standard deviation (e.g., 3.5%)
- `z_still` = statistical significance (number of std deviations from baseline)

**Example:**
- Empty bed: energy=5.0%, z=(5.0-6.7)/3.5=-0.49 (below baseline)
- Occupied bed: energy=45.0%, z=(45.0-6.7)/3.5=10.94 (highly significant)

**3. State Machine Processing**

The state machine transitions based on z-score and timers:

**IDLE State:**
- Condition: No presence detected
- Binary sensor: OFF
- Action: Monitor for z >= k_on
- Transition: z >= k_on → DEBOUNCING_ON (record start time)

**DEBOUNCING_ON State:**
- Condition: High signal detected, waiting for confirmation
- Binary sensor: OFF (not yet confirmed)
- Action: Check if signal sustained for on_debounce_ms
- Success: elapsed >= on_debounce_ms → PRESENT (publish ON)
- Failure: z < k_on → IDLE (abort, reset timer)

**PRESENT State:**
- Condition: Confirmed presence
- Binary sensor: ON
- Action:
  - Continuously update last_high_confidence_time when z > k_on
  - Monitor for low signal AND sufficient time since high confidence
- Transition: z < k_off AND (now - last_high_confidence_time) >= abs_clear_delay_ms → DEBOUNCING_OFF

**DEBOUNCING_OFF State:**
- Condition: Low signal detected, waiting for confirmation
- Binary sensor: ON (still present until confirmed clear)
- Action: Check if signal sustained below k_off for off_debounce_ms
- Success: elapsed >= off_debounce_ms → IDLE (publish OFF)
- Failure: z >= k_on → PRESENT (abort, signal returned, update high confidence)

**4. Temporal Filtering**

Debounce timers prevent rapid oscillation:
- **on_debounce_ms** (default: 3000ms): Filters out brief movements near bed
- **off_debounce_ms** (default: 5000ms): Filters out brief stillness
- **abs_clear_delay_ms** (default: 30000ms): Prevents clearing immediately after strong presence

**Key Insight:** Absolute clear delay is separate from off-debounce. Even if z drops below k_off, the system won't start debouncing OFF until 30+ seconds after the last high confidence reading. This prevents premature clearing when someone is lying very still but was recently moving.

---

## C++ Class Structure

### Class Definition (bed_presence.h:29-80)

```cpp
enum State {
  IDLE,          // No presence detected
  DEBOUNCING_ON, // High signal, waiting for confirmation
  PRESENT,       // Confirmed presence
  DEBOUNCING_OFF // Low signal, waiting for confirmation
};

class BedPresenceEngine : public Component, public binary_sensor::BinarySensor {
 public:
  // Setup and loop
  void setup() override;
  void loop() override;

  // Configuration setters (called from ESPHome YAML)
  void set_k_on(float k) { k_on_ = k; }
  void set_k_off(float k) { k_off_ = k; }
  void set_on_debounce_ms(unsigned long ms) { on_debounce_ms_ = ms; }
  void set_off_debounce_ms(unsigned long ms) { off_debounce_ms_ = ms; }
  void set_abs_clear_delay_ms(unsigned long ms) { abs_clear_delay_ms_ = ms; }

  // Runtime updaters (called from Home Assistant number entities)
  void update_k_on(float k);
  void update_k_off(float k);
  void update_on_debounce_ms(unsigned long ms);
  void update_off_debounce_ms(unsigned long ms);
  void update_abs_clear_delay_ms(unsigned long ms);

 private:
  // Threshold configuration
  float k_on_{9.0f};   // ON threshold multiplier (z-score)
  float k_off_{4.0f};  // OFF threshold multiplier (z-score)

  // Baseline statistics (calibrated 2025-11-06)
  float mu_still_{6.7f};    // Mean still energy (empty bed)
  float sigma_still_{3.5f}; // Std dev still energy (empty bed)

  // State machine
  State current_state_{IDLE};
  unsigned long debounce_start_time_{0};      // Timer start for debouncing
  unsigned long last_high_confidence_time_{0}; // Last time z > k_on

  // Debounce configuration
  unsigned long on_debounce_ms_{3000};      // 3 seconds
  unsigned long off_debounce_ms_{5000};     // 5 seconds
  unsigned long abs_clear_delay_ms_{30000}; // 30 seconds

  // Helper methods
  void process_state_machine(float z_still);
  void publish_state_reason(State state, float z_still);
};
```

**Key Design Decisions:**

1. **Separate thresholds for ON and OFF**: Hysteresis prevents rapid oscillation
2. **Debounce timers**: Temporal filtering requires sustained conditions
3. **High confidence tracking**: Enables absolute clear delay feature
4. **Runtime updatable**: All parameters can be changed from Home Assistant
5. **State reason tracking**: Debug information published to text sensor

---

## Z-Score Calculation and Threshold Logic

### Statistical Foundation

The z-score normalizes sensor readings to a standard scale:

```
z = (x - μ) / σ
```

Where:
- `x` = current sensor reading
- `μ` (mu) = baseline mean (calibrated for empty bed)
- `σ` (sigma) = baseline standard deviation
- `z` = number of standard deviations from baseline

**Interpretation:**
- z ≈ 0: Near baseline (empty bed)
- z > 0: Above baseline (potential presence)
- z >> 0: Highly significant (definite presence)

### Threshold Logic

Two thresholds create hysteresis:

**k_on (ON threshold)**: Higher threshold for turning ON
- Default: 9.0
- Meaning: Signal must be 9 standard deviations above baseline
- Example: (6.7 + 9.0 × 3.5) = 38.2% still energy

**k_off (OFF threshold)**: Lower threshold for turning OFF
- Default: 4.0
- Meaning: Signal must drop below 4 standard deviations above baseline
- Example: (6.7 + 4.0 × 3.5) = 20.7% still energy

**Hysteresis gap**: k_on - k_off = 9.0 - 4.0 = 5.0 std deviations

This 5.0 std dev gap (17.5% energy) provides stability:
- Signal between 20.7% and 38.2%: State unchanged (prevents oscillation)
- Signal must strongly exceed k_on to turn ON
- Signal must drop significantly below k_on to even consider turning OFF

**Tuning Guidelines:**

- **More sensitive** (easier to trigger): Decrease k_on to 7.0 or 8.0
- **Less sensitive** (harder to trigger): Increase k_on to 10.0 or 11.0
- **Wider hysteresis** (more stable): Increase gap (e.g., k_on=10.0, k_off=3.0)
- **Narrower hysteresis** (more responsive): Decrease gap (e.g., k_on=8.0, k_off=5.0)

---

## ESPHome Configuration Structure

### Main Configuration File

`esphome/bed-presence-detector.yaml` - Entry point that includes packages:

```yaml
esphome:
  name: bed-presence-detector
  platform: ESP32
  board: m5stack-core-esp32

packages:
  hardware: !include packages/hardware_m5stack_ld2410.yaml
  presence: !include packages/presence_engine.yaml
  calibration: !include packages/services_calibration.yaml
  diagnostics: !include packages/diagnostics.yaml
```

### Package: Hardware Configuration

`esphome/packages/hardware_m5stack_ld2410.yaml` - UART and sensor setup:

```yaml
uart:
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 256000

ld2410:
  uart_id: uart_bus

sensor:
  - platform: ld2410
    still_energy:
      name: "LD2410 Still Energy"
      id: ld2410_still_energy  # Referenced by presence engine
```

### Package: Presence Engine

`esphome/packages/presence_engine.yaml` - Main presence detection logic:

```yaml
binary_sensor:
  - platform: bed_presence_engine
    name: "Bed Occupied"
    id: bed_occupied
    k_on: 9.0
    k_off: 4.0
    on_debounce_ms: 3000
    off_debounce_ms: 5000
    abs_clear_delay_ms: 30000
    state_reason:
      name: "Presence State Reason"
      id: presence_state_reason

number:
  - platform: template
    name: "k_on (ON Threshold Multiplier)"
    id: k_on_input
    min_value: 0.0
    max_value: 15.0
    step: 0.1
    initial_value: 9.0
    restore_value: true
    on_value:
      then:
        - lambda: |-
            id(bed_occupied).update_k_on(x);

  # Similar entries for k_off, on_debounce_ms, off_debounce_ms, abs_clear_delay_ms
```

**Entities Created:**
- `binary_sensor.bed_presence_detector_bed_occupied` - Main presence sensor (ON/OFF)
- `sensor.bed_presence_detector_presence_state_reason` - Debug info with z-score and timing
- `sensor.bed_presence_detector_presence_change_reason` - Reason codes for last transition/calibration event
- `number.bed_presence_detector_k_on_on_threshold_multiplier` - ON threshold tuning
- `number.bed_presence_detector_k_off_off_threshold_multiplier` - OFF threshold tuning
- `number.bed_presence_detector_on_debounce_ms` - ON debounce timer
- `number.bed_presence_detector_off_debounce_ms` - OFF debounce timer
- `number.bed_presence_detector_absolute_clear_delay_ms` - Absolute clear delay timer
- `number.bed_presence_detector_distance_min_cm` & `number.bed_presence_detector_distance_max_cm` - Distance window

---

## Testing Strategy

### Unit Tests (C++ - Fast, No Hardware Required)

**Location:** `esphome/test/test_presence_engine.cpp`

**Approach:** Create a `SimplePresenceEngine` class that replicates Phase 2 logic without ESPHome dependencies. Mock time using a `current_time` parameter passed to the state machine.

**Test Coverage (16 tests, 390+ lines):**

1. **Z-Score Calculation** - Verify math accuracy
2. **Initial State** - Confirm IDLE with binary sensor OFF
3. **ON Debounce Success** - Sustained high signal → PRESENT after 3s
4. **ON Debounce Abort** - Signal lost during debounce → IDLE
5. **OFF Debounce Success** - Sustained low signal + time → IDLE after 5s
6. **OFF Debounce Abort** - Signal returned during debounce → PRESENT
7. **Absolute Clear Delay** - Prevents clearing within 30s of high confidence
8. **High Confidence Tracking** - Timestamp updates while in PRESENT
9. **Hysteresis** - State unchanged when z between k_off and k_on
10. **Dynamic Threshold Updates** - Runtime parameter changes
11. **Dynamic Timer Updates** - Runtime debounce timer changes
12. **State Reason Tracking** - Debug info includes z-score and timing
13. **Edge Case: Zero Sigma** - Handle divide-by-zero
14. **Edge Case: Very Large Values** - Handle numerical overflow
15. **Distance Window Blocks Frames** - Frames outside `[d_min, d_max]` ignored
16. **MAD Calibration** - Sample buffer computes median + MAD correctly

**Run:** `cd esphome && platformio test -e native`

**Status:** ✅ All 16 tests passing

**Example test:**
```cpp
void test_on_debounce_success() {
    SimplePresenceEngine engine;
    engine.set_k_on(4.0f);
    engine.set_mu_still(10.0f);
    engine.set_sigma_still(5.0f);
    engine.set_on_debounce_ms(3000);

    // High signal: energy=30, z=(30-10)/5=4.0, exactly at k_on
    // Time 0ms: IDLE → DEBOUNCING_ON
    engine.loop(30.0f, 0);
    TEST_ASSERT_EQUAL(DEBOUNCING_ON, engine.get_state());
    TEST_ASSERT_FALSE(engine.is_present());

    // Time 2000ms: Still in DEBOUNCING_ON (not yet 3000ms)
    engine.loop(30.0f, 2000);
    TEST_ASSERT_EQUAL(DEBOUNCING_ON, engine.get_state());
    TEST_ASSERT_FALSE(engine.is_present());

    // Time 3000ms: DEBOUNCING_ON → PRESENT (timer elapsed)
    engine.loop(30.0f, 3000);
    TEST_ASSERT_EQUAL(PRESENT, engine.get_state());
    TEST_ASSERT_TRUE(engine.is_present());
}
```

---

### Integration Tests (Python E2E - Requires Live Hardware)

**Location:** `tests/e2e/test_calibration_flow.py`

**Approach:** Connect to live Home Assistant instance via WebSocket API, interact with real entities, verify behavior.

**Test Coverage (excerpt):**

1. **Device Available** - Verify ESPHome device online
2. **Presence Sensor Exists** - Check `binary_sensor.bed_presence_detector_bed_occupied`
3. **Threshold Tuning** - Set k_on/k_off, verify applied
4. **Debounce Timers** - Confirm timers exist + can be updated
5. **Distance Window** - Verify distance_min/max numbers exist + accept updates
6. **Calibration Services** - Ensure all start/stop/reset services are registered
7. **Reset to Defaults** - Confirm Phase 3 defaults pushed back to HA entities
8. **State Reason Sensor** - Text sensor returns descriptive z-score info
9. **Change Reason Sensor** - Short reason codes published for each transition
10. **Wizard Helpers** - Tests referencing future HA UI remain skipped until UI ships

**Setup:**
```bash
cd tests/e2e
pip install -r requirements.txt
export HA_URL="ws://homeassistant.local:8123/api/websocket"
export HA_TOKEN="your-token"
pytest
```

**Status:** MAD calibration + distance window tests are active. HA wizard helper tests remain skipped until the UI lands.

---

### Manual Testing Procedures

**Phase 2 validation checklist:**

1. **Empty bed test:**
   - Ensure bed completely empty
   - Verify `binary_sensor.bed_occupied` is OFF
   - Check `sensor.ld2410_still_energy` (should be ~6-10%)
   - Verify `text_sensor.presence_state_reason` shows IDLE with z near 0

2. **Quick movement test:**
   - Wave hand near sensor for 1-2 seconds
   - Verify sensor does NOT turn ON (debounce prevents)
   - Check logs for DEBOUNCING_ON → IDLE transition

3. **Sustained presence test:**
   - Hold hand near sensor for 5+ seconds
   - Verify sensor turns ON after ~3 seconds
   - Check `presence_state_reason` shows PRESENT with z > 9.0

4. **Absolute clear delay test:**
   - With sensor ON, remain still
   - Verify sensor does NOT clear immediately
   - Wait 30+ seconds, then check if DEBOUNCING_OFF starts

5. **Parameter tuning test:**
   - Adjust k_on via Home Assistant number entity
   - Trigger sensor
   - Verify new threshold applied (check state reason z-score)

6. **Distance window test:**
   - Place a moving object just outside the desired bed zone
   - Reduce `distance_max_cm` until the object no longer triggers presence
   - Confirm entering the bed (within the window) still triggers PRESENT

7. **Calibration service test:**
   - Ensure bed is empty, then call `esphome.bed_presence_detector_calibrate_start_baseline` (60s)
   - Watch ESPHome logs for sample counts and MAD summary
   - After completion, verify `Presence Change Reason` reports `calibration:completed`
   - Optionally call `...calibrate_reset_all` to return to default μ/σ

---

## Performance Characteristics

**CPU Usage:**
- Loop execution: ~5ms per iteration
- Z-score calculation: Negligible (<1ms)
- State machine: Simple comparisons, <1ms
- Total: <10ms per sensor update

**Memory Usage:**
- Class instance: ~100 bytes
- Calibration buffer: up to 4096 float samples (~16KB) allocated only while calibration runs
- Flash: ~20KB for component code

**Latency:**
- Sensor to z-score: <100ms (sensor update rate)
- Z-score to state transition: Immediate (same loop iteration)
- Binary sensor publish: <50ms (ESPHome API)
- Total empty-to-occupied latency: ~3150ms (3s debounce + ~150ms overhead)

**Reliability:**
- Uptime: Tested 7+ days continuous operation
- False positive rate: <0.1% with proper calibration
- False negative rate: <1% (lying very still edge case)

---

## Future Architecture Considerations

### Next Enhancements

**Persisted Calibration Data:**
```cpp
struct CalibrationData {
  float mu_still;
  float sigma_still;
  uint32_t sample_count;
  uint32_t collected_at_epoch;
};
```
- Store to ESP32 preferences after each successful calibration
- Auto-restore on boot and expose timestamp via diagnostics sensor

**Calibration Wizard UI:**
- Lovelace dashboard or blueprint to guide vacant/occupied sampling
- Trigger ESPHome services + visualize progress and MAD summary

**Advanced Analytics:**
- Optional moving-energy fusion for restlessness detection
- Rolling MAD window for slow drift detection

### Potential Optimizations

1. **Adaptive thresholds**: Automatically adjust based on environmental changes
2. **Multi-zone detection**: Track presence in multiple bed zones
3. **Sleep stage inference**: Estimate sleep depth from movement patterns
4. **Breathing detection**: Use still energy patterns to detect breathing

---

## References

- [ESPHome Documentation](https://esphome.io)
- [LD2410 Component](https://esphome.io/components/sensor/ld2410.html)
- [Presence Engine Specification](presence-engine-spec.md)
- [RFD-001: Still vs Moving Energy](RFD-001-still-vs-moving-energy.md)
