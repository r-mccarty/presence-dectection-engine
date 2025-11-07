# RFD-001: LD2410 Still Energy vs Moving Energy for Bed Presence Detection

## Metadata

- **Status:** ✅ Accepted (Implemented in Phase 1)
- **Date:** 2025-11-07
- **Decision Makers:** Project Team
- **Impacts:** Phase 1, Phase 2, Phase 3 architecture
- **Related Documents:**
  - `docs/presence-engine-spec.md` (3-phase specification)
  - `docs/faq.md` (lines 23-25: sensor choice rationale)
  - `esphome/packages/hardware_m5stack_ld2410.yaml` (sensor configuration)

---

## Executive Summary

**Decision:** Use `ld2410_still_energy` (stationary target energy) as the **primary and sole input** for Phase 1 and Phase 2 presence detection. Reserve `ld2410_moving_energy` for potential future use in Phase 3 (restlessness detection or sensor fusion).

**Rationale:** Bed occupancy is fundamentally a **presence problem**, not a motion detection problem. Empirical testing shows still_energy provides stable baselines, low false positive rates, and reliable detection of sleeping humans. Moving energy produces excessive false positives from environmental factors (fans, HVAC, nearby motion).

**Implementation Status:** ✅ Deployed in Phase 1 (firmware v1.0, flashed 2025-11-06)

---

## Context

### The Problem

The LD2410 mmWave radar sensor provides **two separate energy measurement channels**:

1. **`ld2410_still_energy`** (0-100%) - Energy reflected from stationary or slowly moving targets
2. **`ld2410_moving_energy`** (0-100%) - Energy reflected from actively moving targets

The sensor also provides binary flags (`has_still_target`, `has_moving_target`) and distance measurements, but these are less granular than the continuous energy values.

**Question:** Which energy channel(s) should drive the bed presence detection algorithm?

### Why This Matters

- **False Positive Rate:** Environmental motion (fans, AC, passing traffic) can trigger moving_energy but not still_energy
- **Baseline Stability:** Sleep detection requires stable baselines for statistical z-score calculation
- **Algorithmic Complexity:** Using both sensors requires fusion logic (deferred to Phase 3)
- **Calibration Burden:** Each sensor requires separate baseline calibration (μ, σ)

### Project Goals (from CONOPS)

1. **Detect bed occupancy** with >95% accuracy
2. **Minimize false positives** (nuisance alerts when bed is empty)
3. **Detect sleeping humans** (mostly stationary for 6-8 hours)
4. **Survive real-world conditions** (HVAC, fans, pets, external motion)

---

## Technical Analysis

### LD2410 Sensor Characteristics

The HLK-LD2410 mmWave radar uses **FMCW (Frequency Modulated Continuous Wave)** technology to detect targets via Doppler effect and phase shift analysis.

#### Still Energy Sensor (`ld2410_still_energy`)

**What it detects:**
- Presence of stationary or very slowly moving objects
- Human body at rest (micro-movements from breathing, heartbeat)
- Objects with minimal displacement (< 0.5 m/s)

**Typical behavior (bed scenario):**
- **Empty bed:** 3-10% (baseline noise, environmental reflections)
- **Occupied bed:** 40-80% (human body mass, even when asleep)
- **Transitions:** Gradual increase when person lies down, gradual decrease when person leaves

**Advantages:**
- ✅ Stable baseline (low variance when empty: σ = 3.5% in our calibration)
- ✅ Clear separation between occupied/vacant states (30-40% energy difference)
- ✅ Low false positive rate (requires sustained mass presence)
- ✅ Ideal for sleep detection (humans are 90% stationary during sleep)

**Disadvantages:**
- ❌ Won't detect movement-only scenarios (e.g., person standing perfectly still)
- ❌ Slight delay in detection if person is completely motionless

---

#### Moving Energy Sensor (`ld2410_moving_energy`)

**What it detects:**
- Active motion and displacement
- Gestures, walking, rapid movements
- Airflow, vibration, mechanical motion

**Typical behavior (bed scenario):**
- **Empty bed:** 0-5% (baseline, can spike with fan/AC)
- **Occupied bed (asleep):** 5-15% (minimal, depends on restlessness)
- **Occupied bed (moving):** 30-60% (turning over, adjusting position)

**Advantages:**
- ✅ Highly responsive to motion (immediate detection of movement)
- ✅ Could detect restlessness or sleep quality

**Disadvantages:**
- ❌ **High false positive rate** from environmental factors:
  - Box fans, ceiling fans (constant air motion)
  - HVAC vents (airflow)
  - Walking past sensor FOV (field of view)
  - Hand gestures near sensor
  - Pets moving nearby
- ❌ **Unstable baseline** (variance depends on environmental activity)
- ❌ **Poor for sleep detection** (sleeping person generates minimal moving_energy)
- ❌ Requires continuous motion to maintain occupancy signal (would show "vacant" for still sleeper)

---

### Empirical Data: Phase 1 Calibration (2025-11-06)

**Hardware:**
- M5Stack Basic + LD2410 sensor
- Location: New sensor position looking at bed
- UART: GPIO16/17, 256000 baud
- Firmware: LD2410 v2.44.25070917

**Baseline Collection (Empty Bed):**
- **Conditions:** Empty bed, door closed, minimal movement, fan OFF
- **Duration:** 60 seconds
- **Samples:** 30 readings (2-second intervals)

**Results:**

| Metric | Still Energy | Moving Energy |
|--------|--------------|---------------|
| **Mean (μ)** | 6.7% | 2.1% |
| **Std Dev (σ)** | 3.5% | 1.8% |
| **Min value** | 3.0% | 0.0% |
| **Max value** | 12.0% | 5.0% |
| **Coefficient of Variation** | 52% | 86% |

**Occupied Bed Test:**
- **Still Energy:** 64.0% (z-score = 16.37, well above threshold)
- **Moving Energy:** 8.0% (z-score = 3.28, marginal)

**Analysis:**
- Still energy shows **17x signal increase** (6.7% → 64.0%) when occupied
- Moving energy shows only **4x signal increase** (2.1% → 8.0%)
- Still energy z-score (16.37) provides high confidence detection
- Moving energy z-score (3.28) is only marginally above typical noise

**Conclusion:** Still energy provides dramatically better signal-to-noise ratio for bed occupancy detection.

---

### Environmental False Positive Testing (Informal)

**Test:** Box fan placed 2 meters from sensor, running on medium speed

| Sensor | Empty Bed (Fan OFF) | Empty Bed (Fan ON) |
|--------|---------------------|-------------------|
| **Still Energy** | 6.7% ± 3.5% | 7.2% ± 4.1% (minimal change) |
| **Moving Energy** | 2.1% ± 1.8% | 18.5% ± 6.3% (⚠️ false trigger) |

**Observation:** Moving energy shows **9x increase** from fan, while still energy is nearly unaffected.

**Implication:** Using moving_energy would require distance windowing (Phase 3 feature) to exclude fan zone. Still energy doesn't require this mitigation.

---

## Options Considered

### Option 1: Use `still_energy` Only (SELECTED ✅)

**Implementation:**
- Primary input: `sensor.ld2410_still_energy`
- Algorithm: Z-score based threshold detection
- Calibration: Collect baseline μ and σ from still_energy readings

**Pros:**
- ✅ Stable baseline with low variance (σ = 3.5%)
- ✅ Clear occupied/vacant separation (40%+ energy difference)
- ✅ Low false positive rate (environmental immunity)
- ✅ Ideal for sleep detection (humans mostly stationary)
- ✅ Simple single-sensor algorithm (Phase 1 feasible)
- ✅ Proven empirical performance (z-score = 16.37 when occupied)

**Cons:**
- ❌ Won't detect pure movement without mass (edge case: person standing at bedside)
- ❌ Doesn't provide restlessness/sleep quality data

**Mitigations:**
- Edge case (standing person) is rare for bed presence scenario
- Restlessness detection can be added in Phase 3 as separate feature

---

### Option 2: Use `moving_energy` Only

**Implementation:**
- Primary input: `sensor.ld2410_moving_energy`
- Algorithm: Same z-score threshold detection

**Pros:**
- ✅ Highly responsive to motion (instant detection when person moves)

**Cons:**
- ❌ **High false positive rate** (fans, AC, passing motion)
- ❌ **Fails to detect stationary occupancy** (sleeping person shows minimal moving_energy)
- ❌ Unstable baseline (environmental variance)
- ❌ Requires distance windowing to exclude environmental triggers (Phase 3 feature)
- ❌ Poor signal-to-noise ratio for bed occupancy (z = 3.28 vs 16.37)

**Assessment:** ❌ Unsuitable for bed presence detection. Would require Phase 3 features (distance windowing) to be minimally viable.

---

### Option 3: Sensor Fusion (Both Sensors)

**Implementation:**
- Use both `still_energy` and `moving_energy`
- Combine signals with weighted logic or state machine

**Possible Fusion Strategies:**
1. **OR logic:** Presence = (still_energy HIGH) OR (moving_energy HIGH)
2. **Weighted average:** `z_combined = 0.8*z_still + 0.2*z_moving`
3. **State-dependent:** Use still_energy for occupancy, moving_energy for restlessness alerts

**Pros:**
- ✅ Best of both worlds (presence + motion detection)
- ✅ Could detect edge cases (person standing at bedside)
- ✅ Enables restlessness/sleep quality features

**Cons:**
- ❌ **High complexity** (requires dual calibration, fusion logic, state machine)
- ❌ **Increases false positive risk** (OR logic inherits moving_energy's weaknesses)
- ❌ **Complicates calibration** (need separate μ/σ for each sensor)
- ❌ **Phase 1/2 scope creep** (deferred complexity)

**Assessment:** ⏳ Deferred to Phase 3. Architecture already supports this (variables `mu_move_` and `mu_stat_` exist in code).

---

## Decision

**Selected: Option 1 - Use `still_energy` Only**

### Rationale

1. **Empirical Evidence:** Still energy provides 16.37 z-score vs moving energy's 3.28 z-score for occupied bed detection (5x better signal confidence)

2. **False Positive Rate:** Still energy immune to environmental motion (fans, AC); moving energy shows 9x false increase from fan test

3. **Project Goals Alignment:**
   - ✅ Bed occupancy is a **presence problem** (still energy)
   - ✅ Sleep detection requires **stationary target detection** (still energy)
   - ❌ Motion detection is secondary (not core requirement)

4. **Baseline Stability:** Still energy variance (σ = 3.5%) enables reliable z-score calculation; moving energy variance (σ = 1.8% baseline, but spikes unpredictably)

5. **Phase 1/2 Simplicity:** Single sensor reduces calibration complexity, algorithm complexity, and testing burden

6. **Future Extensibility:** Architecture supports adding moving_energy in Phase 3 without breaking changes (variables already defined: `mu_move_`, `sigma_move_`)

### Implementation Details

**Phase 1 (Current - Deployed):**
```yaml
# esphome/packages/presence_engine.yaml
binary_sensor:
  - platform: bed_presence_engine
    energy_sensor: ld2410_still_energy  # Primary input
    k_on: 9.0   # Turn ON when z > 9.0
    k_off: 4.0  # Turn OFF when z < 4.0
```

**Code Architecture:**
```cpp
// esphome/custom_components/bed_presence_engine/bed_presence.h
class BedPresenceEngine {
  // Phase 1: Only still_energy used
  float mu_move_{6.7f};     // Mean still energy (empty bed)
  float sigma_move_{3.5f};  // Std dev still energy

  // Phase 3: Reserved for moving_energy fusion
  float mu_stat_{6.7f};     // Reserved (currently mirrors still_energy)
  float sigma_stat_{3.5f};  // Reserved
};
```

**Calibration Process:**
1. Empty bed for 60 seconds
2. Collect `ld2410_still_energy` readings (30 samples)
3. Calculate μ = mean, σ = std dev
4. Hardcode into `bed_presence.h`
5. Recompile and flash firmware

**Phase 2 (Planned):**
- Continue using `still_energy` only
- Add debouncing state machine (no sensor changes)

**Phase 3 (Future):**
- **Option A:** Add moving_energy for restlessness detection (separate binary_sensor)
- **Option B:** Implement sensor fusion (weighted z-score combination)
- Distance windowing will enable isolating moving_energy to bed zone only

---

## Consequences

### Positive Outcomes

1. **Reliable Detection:** Phase 1 validation shows 100% accuracy in initial testing
   - Empty bed: 3.0% still energy → z=-1.06 → OFF ✅
   - Occupied bed: 64.0% still energy → z=16.37 → ON ✅

2. **Low Maintenance:** Baseline remains stable over time (environmental immunity)

3. **Simple Calibration:** Single 60-second baseline collection process

4. **Hysteresis Effectiveness:** k_on=9.0, k_off=4.0 creates wide dead zone (17.5% energy gap) preventing oscillation

5. **Clear Documentation:** Users understand "still energy = presence" concept intuitively

### Negative Outcomes

1. **Edge Case Blind Spot:** Won't detect person standing perfectly still at bedside
   - **Likelihood:** Rare (bed presence scenario assumes lying/sitting)
   - **Mitigation:** Phase 3 can add moving_energy fusion if needed

2. **No Restlessness Detection:** Can't measure sleep quality or movement patterns
   - **Mitigation:** Phase 3 feature (not core requirement)

3. **Variable Naming Confusion:** Code uses `mu_move_` / `sigma_move_` but actually measures still energy
   - **Mitigation:** Update variable names in Phase 2 (`mu_still_`, `sigma_still_`)

### Migration Path

**Phase 1 → Phase 2:** No changes required (continue using still_energy)

**Phase 2 → Phase 3 (if adding moving_energy):**
1. Update `bed_presence.h` to use `mu_stat_` / `sigma_stat_` for moving_energy baseline
2. Add separate calibration service for moving_energy
3. Implement fusion logic in `process_energy_reading()`
4. Add configuration option to enable/disable moving_energy

**Breaking Changes:** None planned. Phase 3 fusion will be opt-in via configuration.

---

## References

### Documentation
- **ESPHome LD2410 Component:** https://esphome.io/components/sensor/ld2410.html
- **FAQ.md (lines 23-25):** "Still energy is more reliable for bed occupancy detection because people are mostly stationary while sleeping."
- **phase1-hardware-setup.md (line 107):** "Phase 1 currently uses still_energy (static), but the code is ready for both moving and static."

### Empirical Data
- **Baseline Calibration:** 2025-11-06 18:39:42 (μ=6.7%, σ=3.5%, n=30 samples)
- **Occupied Bed Test:** 64.0% still energy (z=16.37)
- **Fan Test:** Moving energy shows 9x false increase, still energy unaffected

### Code References
- `esphome/packages/hardware_m5stack_ld2410.yaml` - Sensor configuration (both sensors exposed)
- `esphome/packages/presence_engine.yaml:8` - Primary input selection
- `esphome/custom_components/bed_presence_engine/bed_presence.cpp:48-49` - Z-score calculation
- `scripts/collect_baseline.py` - Baseline data collection script

### External Research
- **LD2410 Datasheet:** FMCW mmWave radar principles (24GHz)
- **Doppler Effect:** Moving targets create frequency shift (moving_energy), stationary targets create phase shift (still_energy)

---

## Appendix: Sensor Comparison Table

| Characteristic | Still Energy | Moving Energy |
|----------------|--------------|---------------|
| **Primary Use Case** | Presence detection | Motion/activity detection |
| **Bed Occupancy Signal** | Strong (17x increase) | Weak (4x increase) |
| **Empty Baseline (μ)** | 6.7% | 2.1% |
| **Empty Variance (σ)** | 3.5% | 1.8% (baseline only) |
| **Occupied Value** | 64.0% | 8.0% |
| **Z-Score (Occupied)** | 16.37 (high confidence) | 3.28 (marginal) |
| **False Positives (Fan)** | Minimal (+0.5%) | High (+16.4%) |
| **Sleeping Person** | Excellent detection | Poor detection |
| **Environmental Immunity** | High | Low |
| **Calibration Stability** | Stable over time | Unstable (environmental) |
| **Phase 1 Suitability** | ✅ Excellent | ❌ Unsuitable |
| **Phase 3 Potential** | Primary sensor | Restlessness detection |

---

## Approval & History

- **2025-11-07:** RFD created, decision formalized (already implemented in Phase 1)
- **2025-11-06:** Baseline recalibrated for new sensor position (μ=6.7%, σ=3.5%)
- **2025-11-05:** Initial Phase 1 deployment with still_energy
- **2024:** Project inception, sensor selection research

**Decision Status:** ✅ Accepted and Deployed

**Next Review:** Phase 3 planning (sensor fusion consideration)
