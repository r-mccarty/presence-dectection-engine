# Frequently Asked Questions

**Current Status**: Phase 3 (Automated Calibration & Hardening) - Deployed and operational | Phase 3.1+ (Advanced Features) - In planning

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

**Phase 3 accuracy** (current deployment):
- **False positive rate**: Very low (<0.5% with automated MAD calibration and distance windowing)
- **False negative rate**: Very low when moving, moderate when lying very still (tunable with `abs_clear_delay_ms`)
- **Response time**: 3-5 seconds (configurable debounce timers)
- **Detection confidence**: High when person is moving, moderate when completely still
- **Environmental robustness**: Excellent with distance windowing to exclude interference zones

**Phase 3 improvements over Phase 2**:
- Automated MAD-based calibration eliminates outlier sensitivity
- Distance windowing (d_min/d_max) filters out fan zones and other interference sources
- Change reason telemetry helps identify and resolve edge cases
- Runtime reset service provides quick recovery from misconfigurations

**Factors affecting accuracy**:
- Sensor positioning and mounting
- Baseline calibration quality (automated in Phase 3)
- Environmental interference (mitigated via distance windowing)
- Threshold and timer tuning (all runtime configurable)

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
- Detecting restlessness or sleep quality (**Phase 3.1+ opportunity**)
- Distinguishing between sleeping and active states
- Detecting movement patterns (not implemented in Phases 1-3)

**Phase 3.1+ exploration**: Optional moving-energy fusion could provide:
- Restlessness metrics for sleep quality tracking
- Enhanced confidence during state transitions
- Multi-modal presence detection combining both energy types

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

### What's the difference between Phase 1, 2, 3, and 3.1+?

**Phase 1** ✅ Complete:
- Basic z-score detection
- Immediate state transitions (no debouncing - intentionally "twitchy")
- Hysteresis via k_on/k_off thresholds
- Runtime tunable thresholds
- **Status**: Completed 2025-11-06

**Phase 2** ✅ Deployed:
- 4-state machine (IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF)
- Temporal filtering with 3 debounce timers
- Absolute clear delay for handling stillness
- Runtime tunable thresholds AND timers
- **Status**: Deployed to hardware 2025-11-07

**Phase 3** ✅ Deployed (Current Production):
- Automated baseline calibration via ESPHome services (MAD statistics)
- Distance windowing (d_min/d_max) to ignore interference zones
- Presence change reason telemetry for debugging
- Runtime reset service for quick recovery from misconfigurations
- Guided Home Assistant calibration wizard with helper entities
- **Status**: Deployed 2025-11-08, wizard shipped 2025-11-10, E2E tests (16) passing 2025-11-11

**Phase 3.1+** 🎯 Future Opportunities:
- **Data persistence**: Store calibration history snapshots for auditing and rollback
- **Advanced analytics**: Moving-energy fusion for restlessness metrics and sleep quality
- **Operational monitoring**: Alerts for calibration drift, sensor offline events
- **Multi-target filtering**: Enhanced detection for shared beds
- **Status**: Planning phase, prioritization based on user feedback

## Calibration Questions

### What is "baseline calibration"?

**Baseline calibration** measures the sensor's readings when the bed is empty to establish statistical baselines (μ and σ).

**Phase 3 Automated Process** (Recommended):
1. Ensure bed is completely vacant
2. Call `esphome.bed_presence_detector_calibrate_start_baseline` (duration_s ≥ 60 recommended)
3. **MAD-based statistics** calculate robust μ (median) and σ (MAD × 1.4826) - resistant to outliers
4. Results are applied immediately and persist across reboots
5. Optional `calibrate_reset_all` service restores all parameters to known-good defaults

**Why needed**:
- Different sensors have different characteristics
- Environmental conditions vary (room size, materials, interference)
- Sensor position affects readings
- Temperature and humidity can cause sensor drift over time

**Calibration Methods**:
- **Primary**: ESPHome service (`calibrate_start_baseline`) - automated, MAD-robust, no manual code changes
- **UI Wizard**: Home Assistant Calibration Wizard (guided workflow with confirmations and status tracking)
- **Legacy**: Python script (`scripts/collect_baseline.py`) for raw data exports and manual analysis

**Phase 3 Advantages**:
- No firmware recompilation required
- MAD statistics ignore transient spikes during calibration
- Immediate feedback via `sensor.presence_change_reason`
- One-click reset to factory defaults if needed

### How often should I recalibrate?

**Recalibrate when**:
- First setup (always calibrate for your specific hardware/environment)
- Moving sensor to new position or room
- Changing bed/mattress
- Persistent false positives or negatives
- After firmware updates that change baseline values
- Seasonal environmental changes (temperature, humidity shifts)

**Not needed**:
- Normal daily operation (baseline persists across reboots)
- Adjusting threshold or timer values (runtime tunable)
- Minor environmental changes (system adapts via thresholds)
- After adjusting distance windowing (d_min/d_max)

**Typical frequency**: Once during initial setup, then rarely (every 3-6 months or when issues arise)

**Phase 3 makes recalibration easy**: No code changes or firmware reflashing required - just call the service or use the wizard!

### Can I calibrate automatically from Home Assistant?

**Yes.** Use `esphome.bed_presence_detector_calibrate_start_baseline` with a positive `duration_s`. The device collects
samples, computes μ/σ via MAD, and updates runtime values on the fly. `Presence Change Reason` will report
`calibration:completed` when finished.

**Need a UI?** Use the **Calibration Wizard** tab on the Bed Presence dashboard after including
`homeassistant/configuration_helpers.yaml`. It wraps the same ESPHome services with confirmations, timers, and
status tracking.

### What is "distance windowing" and when should I use it?

**Distance windowing** (Phase 3 feature) filters sensor readings by distance, ignoring readings outside your configured range.

**Use cases**:
- **Ceiling fans**: Set `d_max_cm = 150` to ignore fan motion at 200cm
- **Nearby hallways**: Set `d_min_cm = 50` to ignore people walking past the door
- **Shared walls**: Limit detection to your bed's physical location
- **Large rooms**: Focus detection on bed area, ignore far corners

**How it works**:
- Every sensor frame reports distance to detected target
- Frames outside `[d_min_cm, d_max_cm]` are completely ignored
- Only in-window frames affect presence logic and calibration
- Default: `d_min = 0cm`, `d_max = 600cm` (full LD2410 range)

**Configuration**:
1. Observe `sensor.bed_presence_detector_still_distance` in Home Assistant
2. Note distance ranges when bed is occupied vs. interference sources
3. Adjust `number.bed_presence_detector_distance_min/max` to exclude interference zones
4. Changes take effect immediately (no reboot required)

**Pro tip**: Use distance windowing BEFORE calibration for cleanest baseline. Calibration only samples frames within the configured window.

### What are "change reasons" and how do I use them?

**Change reasons** (Phase 3 feature) provide debugging context for every presence state change.

**Available via**: `sensor.bed_presence_detector_presence_change_reason`

**Common reason codes**:
- `on:threshold_exceeded` - Z-score crossed k_on threshold, debounce started
- `on:debounce_confirmed` - Sustained signal for full debounce period, now PRESENT
- `off:threshold_dropped` - Z-score fell below k_off, off-debounce started
- `off:debounce_confirmed` - Sustained low signal, now IDLE
- `calibration:started` - Calibration service called, presence detection paused
- `calibration:completed` - New baseline applied, detection resumed
- `reset:defaults_restored` - Reset service called, all parameters restored

**Use for troubleshooting**:
- If stuck in PRESENT: Check if `off:debounce_confirmed` ever occurs
- If false positives: Look for frequent `on:threshold_exceeded` without confirmation
- If calibration issues: Verify `calibration:completed` appears after service calls

**Phase 3.1+ opportunity**: Change reason history logging and analytics dashboard

## Development Questions

### How do I run the C++ unit tests locally?

```bash
cd esphome
platformio test -e native
```

**Test coverage** (16 C++ unit tests, 390+ lines):
- ✅ Z-score calculation accuracy
- ✅ State machine transitions (all 4 states)
- ✅ Debounce timer behavior
- ✅ Abort conditions during debouncing
- ✅ Absolute clear delay logic
- ✅ Distance windowing (Phase 3)
- ✅ Edge cases (zero sigma, negative energy)

**Additional E2E tests** (16 Python integration tests):
- ✅ Calibration service flow (start/stop/reset)
- ✅ Home Assistant entity availability and updates
- ✅ WebSocket communication with ESPHome device
- ✅ Wizard helper entities and automation flows

**Runs in ~5 seconds** (C++ unit tests) + ~30 seconds (E2E tests on ubuntu-node)

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

3. **E2E Tests** (requires hardware + Home Assistant, run on ubuntu-node):
   ```bash
   # SSH to ubuntu-node first
   cd /home/user/bed-presence-sensor/tests/e2e
   source ~/.venv-e2e/bin/activate
   pytest
   ```
   Note: E2E tests must run on ubuntu-node to access Home Assistant at 192.168.0.148

**Recommendation**: Run unit tests frequently during development in Codespaces, E2E tests before deployment on ubuntu-node.

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

**Phase 3.1+ Priority Areas**:
1. **Calibration persistence**: Flash storage for baseline history, rollback capability, audit logging
2. **Advanced analytics**: Moving-energy fusion, restlessness scoring, sleep quality metrics
3. **Operational monitoring**: Alerts for drift, offline sensors, or configuration issues
4. **Documentation**: Wiring diagrams (docs/assets/wiring_diagram.png is placeholder), tutorials, video guides
5. **Hardware**: 3D printable mounts (hardware/mounts/m5stack_side_mount.stl is placeholder)
6. **Multi-target detection**: Enhanced algorithms for shared beds or multiple occupants

**Other Contribution Areas**:
1. **Testing**: Hardware compatibility testing with different ESP32 boards and LD2410 variants
2. **Integration**: Additional Home Assistant blueprints, dashboards, automations
3. **Translations**: Internationalization of UI elements and documentation
4. **Bug fixes**: Check GitHub Issues for reported issues

**Process**:
1. Check GitHub Issues for open tasks or create proposal for new features
2. Fork repository and create feature branch
3. Follow coding standards in existing code
4. Write unit tests for new features (C++ and/or Python E2E)
5. Update documentation (FAQ, ARCHITECTURE.md, etc.)
6. Test on hardware if possible (or coordinate with maintainers)
7. Submit pull request with clear description and testing evidence

**See**: CONTRIBUTING.md and docs/development-scorecard.md for Phase 3.1+ roadmap details.
