# Quickstart Guide

**Current Status**: Phase 2 (State Machine + Debouncing) - Deployed and operational

This guide will help you get your bed presence detector up and running quickly. The system uses statistical z-score analysis with a 4-state machine for reliable presence detection.

## What This System Does

The bed presence detector:
- ✅ **Detects presence** in bed using mmWave radar (no camera, preserves privacy)
- ✅ **Filters noise** with temporal debouncing (3s on, 5s off, 30s clear delay)
- ✅ **Self-tunes** via runtime adjustable thresholds and timers in Home Assistant
- ✅ **Prevents false positives** from transient movement near bed
- ✅ **Prevents false negatives** when lying very still

## Prerequisites

- **Hardware**:
  - M5Stack ESP32 device (or compatible ESP32 board)
  - LD2410 mmWave radar sensor
  - USB cable for flashing
  - 4 wires for UART connection
- **Software**:
  - Home Assistant instance (version 2023.1 or later)
  - ESPHome installed (`pip install esphome`)
  - Python 3.9+ (for optional calibration scripts)
- **Network**:
  - 2.4GHz WiFi network (ESP32 doesn't support 5GHz)
  - Home Assistant and ESP32 on same network

## Hardware Assembly

1. **Connect the LD2410 sensor to your M5Stack device**:
   - TX (LD2410) → GPIO16 (M5Stack RX)
   - RX (LD2410) → GPIO17 (M5Stack TX)
   - VCC → 5V
   - GND → GND

2. **Mount the sensor** pointing at the bed surface (approximately 1-2 meters distance)

3. **Verify wiring**: Double-check connections - incorrect wiring can damage components

**Note**: Wiring diagram placeholder exists at `docs/assets/wiring_diagram.png` but is currently empty (0 bytes).

## Firmware Installation

### Step 1: Clone Repository and Set Up Secrets

```bash
# Clone the repository
git clone https://github.com/your-repo/bed-presence-sensor.git
cd bed-presence-sensor

# Create secrets file from template
cd esphome
cp secrets.yaml.example secrets.yaml
nano secrets.yaml  # Fill in your WiFi credentials
```

**Required secrets**:
```yaml
wifi_ssid: "YourWiFiNetwork"
wifi_password: "YourWiFiPassword"
api_encryption_key: "base64-key-here"  # ESPHome will generate if blank
ota_password: "your-ota-password"
```

### Step 2: Compile and Flash Firmware

**First-time flash (USB required)**:
```bash
cd esphome
esphome run bed-presence-detector.yaml
# Select USB serial port when prompted
```

**Subsequent updates (OTA)**:
```bash
cd esphome
esphome run bed-presence-detector.yaml
# Select OTA option when prompted
```

### Step 3: Verify Device is Online

After flashing:
1. Check ESPHome logs: `esphome logs bed-presence-detector.yaml`
2. Look for WiFi connection message with IP address
3. Device should appear in Home Assistant → Settings → Devices & Services → ESPHome

## Home Assistant Configuration

### Step 1: Add ESPHome Device

The device should **auto-discover** in Home Assistant:
1. Go to Settings → Devices & Services
2. Look for "Bed Presence Detector" in discovered devices
3. Click "Configure" and enter API encryption key (from `secrets.yaml`)

**Entities created** (8 total):
- `binary_sensor.bed_occupied` - Main presence sensor
- `text_sensor.presence_state_reason` - Debug info (state, z-score, timers)
- `sensor.bed_presence_detector_ld2410_still_energy` - Raw sensor reading (%)
- `number.k_on_on_threshold_multiplier` - ON threshold (default: 9.0)
- `number.k_off_off_threshold_multiplier` - OFF threshold (default: 4.0)
- `number.on_debounce_ms` - ON debounce timer (default: 3000ms)
- `number.off_debounce_ms` - OFF debounce timer (default: 5000ms)
- `number.abs_clear_delay_ms` - Absolute clear delay (default: 30000ms)

### Step 2: Deploy Dashboard (Optional)

```bash
# Copy dashboard YAML
cat homeassistant/dashboards/bed_presence_dashboard.yaml
```

1. Home Assistant → Settings → Dashboards → Add Dashboard
2. Create new dashboard: "Bed Presence"
3. Edit Dashboard → Raw Configuration Editor
4. Paste YAML content
5. Save

### Step 3: Deploy Automation Blueprint (Optional)

```bash
# Copy blueprint to Home Assistant config directory
cp homeassistant/blueprints/automation/bed_presence_automation.yaml \
   /config/blueprints/automation/
```

Then: Settings → Automations & Scenes → Create Automation → Use Blueprint

## Calibration

**Important**: The system ships with default baseline values (μ=6.7%, σ=3.5%) from the developer's hardware. **You should recalibrate** for your specific sensor and environment.

### Quick Calibration (Manual Adjustment)

1. **Empty bed test**:
   - Ensure bed is completely vacant
   - Monitor `sensor.bed_presence_detector_ld2410_still_energy` in Home Assistant (Developer Tools → States)
   - Typical empty bed reading: 3-10%

2. **Occupied bed test**:
   - Get into bed and lie still
   - Monitor `sensor.bed_presence_detector_ld2410_still_energy`
   - Typical occupied bed reading: 30-70%

3. **Adjust thresholds if needed**:
   - If false positives (detects when empty): Increase `k_on` to 10.0 or 11.0
   - If false negatives (doesn't detect when occupied): Decrease `k_on` to 8.0 or 7.0
   - For slow clearing when still: Increase `abs_clear_delay_ms` to 60000 (60s)

### Full Calibration (Guided Wizard – Recommended)

1. Include `homeassistant/configuration_helpers.yaml` in your Home Assistant configuration and reload helpers.
2. Open the **Bed Presence Detector** dashboard → **Calibration Wizard** tab.
3. Toggle **I confirm the bed is empty**, adjust the duration slider if needed, then press **Start Baseline**.
4. Stay out of bed until the wizard step changes to **Finalizing**, then wait for the change-reason sensor to report `calibration:completed`.
5. Review the updated baseline/time stamps in the status card. Use the **Reset Defaults** button if you need to roll back.

### Manual Service Invocation (Advanced)

If you prefer using Developer Tools → Services, call:

```
service: esphome.bed_presence_detector_calibrate_start_baseline
data:
  duration_s: 60
```

Stop with `esphome.bed_presence_detector_calibrate_stop` or reset via `esphome.bed_presence_detector_calibrate_reset_all`.

> **Need raw CSV data?** `scripts/collect_baseline.py` still works, but the wizard keeps firmware and Home Assistant
> perfectly aligned without manual edits.

See [calibration.md](calibration.md) for the detailed workflow plus the legacy script instructions.

## Testing Your Setup

### Test 1: Verify Sensor Readings

1. Developer Tools → States → Search "ld2410"
2. View `sensor.bed_presence_detector_ld2410_still_energy`:
   - Empty bed: Should be low (3-10%)
   - Occupied bed: Should be high (30-70%)

### Test 2: Verify Presence Detection

1. Developer Tools → States → Search "bed_occupied"
2. View `binary_sensor.bed_presence_detector_bed_occupied`:
   - Empty bed: Should be "off" (IDLE state)
   - Get into bed: Should transition to "on" after ~3 seconds (DEBOUNCING_ON → PRESENT)
   - Get out of bed: Should transition to "off" after ~5 seconds + 30 second delay (DEBOUNCING_OFF → IDLE)

### Test 3: Monitor State Transitions

1. Developer Tools → States → Search "presence_state_reason"
2. View `sensor.bed_presence_detector_presence_state_reason`:
   - Example: `PRESENT (z=12.34, high_conf_age=5000ms, abs_clear_ready=no)`
   - Watch state machine transitions in real-time

### Test 4: Inspect Change Reasons

1. Developer Tools → States → Search "presence_change_reason"
2. View `sensor.bed_presence_detector_presence_change_reason`:
   - Example values: `on:threshold_exceeded`, `off:abs_clear_delay`, `calibration:completed`
   - Confirms why the last state flip (or calibration event) occurred

## Tuning for Your Environment

### Common Adjustments

**Sensor too sensitive (false positives)**:
- Increase `k_on` to 10.0 or higher
- Increase `on_debounce_ms` to 5000 (5 seconds)

**Sensor not sensitive enough (false negatives)**:
- Decrease `k_on` to 8.0 or lower
- Decrease `on_debounce_ms` to 2000 (2 seconds)

**Clears too quickly when lying still**:
- Increase `abs_clear_delay_ms` to 60000 (60 seconds) or higher

**Sensor responds too slowly**:
- Decrease `on_debounce_ms` to 2000 or 1000

All adjustments can be made in Home Assistant without reflashing firmware.

## Phase Roadmap

**Phase 1** ✅ Complete:
- Z-score based detection
- Hysteresis (k_on > k_off)
- Runtime tunable thresholds

**Phase 2** ✅ Deployed (Current):
- 4-state machine (IDLE → DEBOUNCING_ON → PRESENT → DEBOUNCING_OFF)
- Temporal filtering with configurable debounce timers
- Absolute clear delay to prevent premature clearing

**Phase 3** ✅ Deployed:
- Automated baseline calibration via ESPHome services (MAD statistics)
- Distance windowing to ignore specific zones/noise sources
- Presence change reason telemetry + reset services, plus guided HA calibration wizard

## Next Steps

- **Fine-tune your system**: Adjust thresholds and timers based on real-world usage
- **Set up automations**: Use blueprint to trigger actions when bed state changes
- **Monitor performance**: Check dashboard for statistics and diagnostics
- **Troubleshooting**: See [troubleshooting.md](troubleshooting.md) for common issues
- **Advanced calibration**: See [calibration.md](calibration.md) for detailed procedures
