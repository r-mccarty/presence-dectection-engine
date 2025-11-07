# Frequently Asked Questions

**Current Status**: Phase 2 (State Machine + Debouncing) - Deployed and operational

## General Questions

### What is the detection range of the LD2410 sensor?

The LD2410 can detect presence up to 6 meters, but for bed detection we configure it for 1-2 meters for best accuracy. The sensor should be mounted with a clear line of sight to the bed surface.

### Can I use this with a different ESP32 board?

Yes! The code is portable to any ESP32 board. You'll need to:
1. Update pin assignments in `esphome/packages/hardware_m5stack_ld2410.yaml` to match your board
2. Verify your board has 2 available GPIO pins for UART communication
3. Ensure adequate power supply for the LD2410 sensor (5V)

**Common ESP32 boards**: ESP32-DevKitC, NodeMCU-32S, WROOM-32, WROVER, etc.

### Does this work with adjustable beds?

Yes, the sensor detects presence regardless of bed position. However, significant position changes may affect the sensor's view angle and signal strength. You may need to:
- Recalibrate baseline if bed adjustment is permanent
- Increase `abs_clear_delay_ms` if bed movement causes transient false negatives

### Will this detect pets on the bed?

**Yes**, the sensor detects any presence (human or pet). The mmWave radar responds to motion and presence within its field of view, regardless of species.

**Tuning for pets**:
- **Small pets** (cats, small dogs): May not trigger detection if too small or still
- **Large pets** (big dogs): Will likely trigger detection similar to humans
- **Solution**: Increase `k_on` threshold to reduce sensitivity if pets cause false positives

### Does this use a camera? What about privacy?

**No camera** - the LD2410 is a mmWave radar sensor that detects presence through radio waves, not visual imaging. It cannot:
- Capture images or video
- Identify individuals
- "see" through clothing or materials

**Privacy preserved**: Only presence/absence state is reported to Home Assistant.

### How accurate is the detection?

**Phase 2 accuracy** (current deployment):
- **False positive rate**: Low (<1% with proper calibration and debounce timers)
- **False negative rate**: Very low when moving, moderate when lying very still (tunable with `abs_clear_delay_ms`)
- **Response time**: 3-5 seconds (configurable debounce timers)
- **Detection confidence**: High when person is moving, moderate when completely still

**Factors affecting accuracy**:
- Sensor positioning and mounting
- Baseline calibration quality
- Environmental interference (fans, HVAC, pets)
- Threshold and timer tuning

## Technical Questions

### What are "z-scores" and why use them?

**Z-scores** measure statistical significance - how many standard deviations a sensor reading is from the baseline mean.

**Formula**: `z = (current_reading - baseline_mean) / baseline_std_dev`

**Example**:
- Baseline: μ=6.7%, σ=3.5% (empty bed)
- Current reading: 45.0% (occupied bed)
- Z-score: `(45.0 - 6.7) / 3.5 = 10.94`

**Why z-scores?**
- Normalizes sensor readings across different hardware/environments
- Provides threshold values that work regardless of absolute sensor readings
- Statistically meaningful (z > 9.0 means signal is 9 standard deviations above baseline)

### Why use still energy instead of moving energy?

**Still energy** is more reliable for bed occupancy detection because:
- People are mostly stationary while sleeping (even during minor movements)
- Moving energy spikes dramatically for brief movements, causing false positives
- Still energy provides a more stable signal for temporal filtering

**When to use moving energy**:
- Detecting restlessness or sleep quality (future feature)
- Distinguishing between sleeping and active states
- Detecting movement patterns (not implemented in Phase 1/2)

### What's the purpose of the debounce timers?

**Debounce timers** prevent rapid state changes from transient signals. Phase 2 implements 3 types:

1. **ON debounce** (default: 3 seconds):
   - Signal must stay above `k_on` threshold for full duration
   - Prevents false positives from walking past bed
   - State: IDLE → DEBOUNCING_ON → PRESENT

2. **OFF debounce** (default: 5 seconds):
   - Signal must stay below `k_off` threshold for full duration
   - Prevents false negatives from brief stillness
   - State: PRESENT → DEBOUNCING_OFF → IDLE

3. **Absolute clear delay** (default: 30 seconds):
   - Prevents clearing immediately after last high confidence reading
   - Handles lying very still (low signal but still present)
   - Requires BOTH low signal AND 30+ seconds since last high reading

**Trade-offs**:
- Longer timers = more stable, slower response
- Shorter timers = faster response, more false positives/negatives

### What is the state machine and how does it work?

Phase 2 implements a **4-state finite state machine**:

```
IDLE (vacant, waiting for presence)
  ↓ (z > k_on)
DEBOUNCING_ON (potential presence, verifying)
  ↓ (sustained 3s) or ↑ (signal lost - abort)
PRESENT (occupied, high confidence)
  ↓ (z < k_off AND abs clear delay elapsed)
DEBOUNCING_OFF (potential vacancy, verifying)
  ↓ (sustained 5s) or ↑ (signal returns - abort)
IDLE (vacant, confirmed)
```

**Key features**:
- **Abort conditions**: Debounce can abort back to previous state if signal changes
- **Temporal filtering**: All transitions require sustained signals
- **State reason tracking**: `text_sensor.presence_state_reason` shows current state and timing info

### Can I use multiple sensors for a king-size bed?

Yes, but you'll need to deploy multiple ESPHome devices (one per sensor) and combine their states in Home Assistant.

**Option 1: Template Binary Sensor** (OR logic):
```yaml
binary_sensor:
  - platform: template
    name: "Bed Occupied (Combined)"
    state: >
      {{ is_state('binary_sensor.bed_left_occupied', 'on') or
         is_state('binary_sensor.bed_right_occupied', 'on') }}
```

**Option 2: Group** (any entity):
```yaml
binary_sensor:
  - platform: group
    name: "Bed Occupied (Combined)"
    device_class: occupancy
    entities:
      - binary_sensor.bed_left_occupied
      - binary_sensor.bed_right_occupied
```

**Use case**: King-size beds where one person's presence should maintain "occupied" state even if the other side is vacant.

### How does hysteresis prevent flapping?

**Hysteresis** uses different thresholds for ON vs OFF transitions, creating a "dead zone" that prevents oscillation.

**Phase 2 hysteresis**:
- **ON threshold**: k_on = 9.0 (z > 9.0 required)
- **OFF threshold**: k_off = 4.0 (z < 4.0 required)
- **Gap**: 5.0 standard deviations (hysteresis band)

**Example**:
- Current z-score = 6.5 (between thresholds)
- If state is PRESENT: Stays present (z not below 4.0)
- If state is IDLE: Stays idle (z not above 9.0)
- No oscillation occurs in the "dead zone" between 4.0 and 9.0

**Wider gap** (higher k_on - lower k_off) = more stable but requires stronger signal differentiation.

### What's the difference between Phase 1, 2, and 3?

**Phase 1** ✅ Complete:
- Basic z-score detection
- Immediate state transitions (no debouncing - intentionally "twitchy")
- Hysteresis via k_on/k_off thresholds
- Runtime tunable thresholds
- **Status**: Completed 2025-11-06

**Phase 2** ✅ Deployed (Current):
- 4-state machine (IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF)
- Temporal filtering with 3 debounce timers
- Absolute clear delay for handling stillness
- Runtime tunable thresholds AND timers
- **Status**: Deployed to hardware 2025-11-07

**Phase 3** ⏳ Planned:
- Automated baseline calibration via Home Assistant services
- Distance windowing to ignore zones
- MAD (Median Absolute Deviation) robust statistics
- **Status**: Service stubs exist but no implementation

## Calibration Questions

### What is "baseline calibration"?

**Baseline calibration** measures the sensor's readings when the bed is empty to establish statistical baselines (μ and σ).

**Process**:
1. Ensure bed is completely vacant
2. Collect 30+ samples over 60 seconds
3. Calculate mean (μ) and standard deviation (σ)
4. Update firmware or use service (Phase 3) to apply new baseline

**Why needed**:
- Different sensors have different characteristics
- Environmental conditions vary (room size, materials, interference)
- Sensor position affects readings

**Current method**: Manual using Python script (`scripts/collect_baseline.py`)
**Phase 3**: Automated via Home Assistant services

### How often should I recalibrate?

**Recalibrate when**:
- First setup (always calibrate for your specific hardware/environment)
- Moving sensor to new position or room
- Changing bed/mattress
- Persistent false positives or negatives
- After firmware updates that change baseline values

**Not needed**:
- Normal daily operation (baseline persists across reboots)
- Adjusting threshold or timer values (runtime tunable)
- Minor environmental changes (system adapts via thresholds)

**Typical frequency**: Once during initial setup, then rarely (every few months or when issues arise).

### Can I calibrate automatically from Home Assistant?

**Not yet** - this is a **Phase 3 feature** (planned).

**Current Phase 2**: Calibration services exist (`esphome.bed_presence_detector_start_calibration`) but are placeholders that only log messages.

**Workaround**: Use Python script `scripts/collect_baseline.py` and manually update firmware.

## Development Questions

### How do I run the C++ unit tests locally?

```bash
cd esphome
platformio test -e native
```

**Test coverage** (14 tests, 355 lines):
- ✅ Z-score calculation accuracy
- ✅ State machine transitions (all 4 states)
- ✅ Debounce timer behavior
- ✅ Abort conditions during debouncing
- ✅ Absolute clear delay logic
- ✅ Edge cases (zero sigma, negative energy)

**Runs in ~5 seconds** without hardware (native tests on development machine).

### Can I test without hardware?

**Yes** - multiple testing levels:

1. **C++ Unit Tests** (no hardware):
   ```bash
   cd esphome
   platformio test -e native
   ```

2. **ESPHome Compilation** (syntax check, no hardware):
   ```bash
   cd esphome
   esphome compile bed-presence-detector.yaml
   ```

3. **E2E Tests** (requires hardware + Home Assistant):
   ```bash
   cd tests/e2e
   pytest
   ```

**Recommendation**: Run unit tests frequently during development, hardware tests before deployment.

### What IDE/editor should I use?

**Recommended**:
- **VS Code** with ESPHome extension (syntax highlighting, auto-complete)
- **GitHub Codespaces** (pre-configured environment, documented in CLAUDE.md)
- **PlatformIO IDE** for C++ development (integrated testing, debugging)

**Works with**:
- vim/neovim with LSP
- Sublime Text
- Any text editor (ESPHome is just YAML + C++)

### How do I contribute to this project?

**Contribution areas**:
1. **Phase 3 implementation**: Automated calibration services, MAD statistics
2. **Documentation**: Tutorials, wiring diagrams, troubleshooting tips
3. **Testing**: Hardware compatibility, edge case testing
4. **Hardware**: 3D printable mounts (current STL is placeholder)
5. **Integration**: Additional Home Assistant blueprints, dashboards

**Process**:
1. Check GitHub Issues for open tasks
2. Fork repository and create feature branch
3. Follow coding standards in existing code
4. Write unit tests for new features
5. Update documentation
6. Submit pull request with clear description

**See**: Project README and CONTRIBUTING.md (if exists) for detailed guidelines.
