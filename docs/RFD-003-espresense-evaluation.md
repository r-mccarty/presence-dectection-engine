# RFD-003 Discussion: ESPresense Integration Analysis

## Metadata

- **Date:** 2025-11-14
- **Author:** AI Research Assistant
- **Related:** RFD-003 Bluetooth Beacon Distance Calibration
- **Purpose:** Evaluate ESPresense as a potential solution for BLE presence detection needs

---

## Executive Summary

**ESPresense** is a mature, open-source ESP32-based BLE presence detection system that directly addresses many of the use cases outlined in RFD-003. This document evaluates whether we should:

1. **Adopt ESPresense** as a complementary system to our LD2410 presence detector
2. **Build custom** BLE tracking as proposed in RFD-003 Phase 4.0
3. **Hybrid approach** - use ESPresense for multi-zone, LD2410 for high-accuracy bed presence

**Recommendation:** Evaluate ESPresense as a **complementary system** rather than attempting to integrate BLE into the existing LD2410 firmware. This preserves the simplicity and reliability of the bed presence detector while leveraging a proven solution for whole-home tracking.

---

## What is ESPresense?

### Overview

**ESPresense** is an open-source project specifically designed for room-level BLE presence detection using ESP32 hardware. It's the spiritual successor to projects like room-assistant and monitor, optimized for low-cost ESP32 boards.

**GitHub:** https://github.com/ESPresense/ESPresense
**Documentation:** https://espresense.com/
**License:** AGPL-3.0

### Core Features

1. **Multi-Node Room Detection**
   - Deploy multiple ESP32 nodes throughout the home
   - Each node reports RSSI for detected BLE devices
   - System determines which room a device is in based on strongest signal
   - MQTT integration with Home Assistant's `mqtt_room` component

2. **Advanced Device Tracking**
   - Fingerprint-based identification (not just MAC addresses)
   - IRK (Identity Resolving Key) support for Apple devices with randomized MACs
   - Tracks phones, smartwatches, BLE beacons, fitness trackers, etc.
   - Works with devices in motion (follows people room-to-room)

3. **Distance Estimation**
   - RSSI-to-distance conversion with calibration per device
   - Three-value median filter + Kalman filter for noise reduction
   - Configurable `rssi@1m` calibration values
   - Distance reported via MQTT (meters)

4. **ESPresense Companion (Optional)**
   - Multilateration engine for precise positioning
   - Uses floor plans to calculate actual device location
   - Aggregates data from 3+ nodes for triangulation
   - Automatic optimization of node settings

5. **Hardware Support**
   - Standard ESP32 modules (~$3 on AliExpress)
   - M5Atom (Lite, Matrix)
   - M5StickC Plus
   - Web-based installation and OTA updates

6. **Home Assistant Integration**
   - MQTT Discovery for automatic sensor creation
   - Device trackers for each tracked device
   - Sensors for distance, RSSI, battery level (if available)
   - Compatible with existing HA automations

---

## How ESPresense Addresses RFD-003 Use Cases

### Use Case 1: Person Identification

**RFD-003 Scenario:** Detect *who* is in bed, not just *if* bed is occupied.

**ESPresense Solution:**
- Track each person's phone/smartwatch as they move throughout the home
- ESPresense reports `sensor.person_a_location = "bedroom"` when in bedroom
- Can be combined with LD2410 bed occupancy in Home Assistant automation:

```yaml
# Home Assistant Template Sensor
template:
  - sensor:
      - name: "Bed Occupant"
        state: >
          {% set bed_occupied = is_state('binary_sensor.bed_presence_detector_bed_occupied', 'on') %}
          {% set person_a_location = states('sensor.person_a_location') %}
          {% set person_b_location = states('sensor.person_b_location') %}

          {% if bed_occupied %}
            {% set occupants = [] %}
            {% if person_a_location == 'bedroom' %}
              {% set occupants = occupants + ['person_a'] %}
            {% endif %}
            {% if person_b_location == 'bedroom' %}
              {% set occupants = occupants + ['person_b'] %}
            {% endif %}
            {{ occupants | join(',') if occupants else 'unknown' }}
          {% else %}
            vacant
          {% endif %}
```

**Pros:**
- ✅ Proven solution with active community
- ✅ Handles MAC randomization (iOS/Android)
- ✅ No custom firmware development needed
- ✅ LD2410 remains primary bed presence sensor

**Cons:**
- ❌ Requires separate ESP32 node(s) for BLE scanning
- ❌ Fusion logic lives in Home Assistant, not on-device
- ❌ Slightly higher latency than on-device fusion

---

### Use Case 2: Multi-Zone Tracking

**RFD-003 Scenario:** Track presence across bedroom, bathroom, closet.

**ESPresense Solution:**
This is ESPresense's primary use case. Deploy one ESP32 node in each zone:
- Bedroom node (could share ESP32 with LD2410, or separate)
- Bathroom node
- Closet node

ESPresense automatically determines which zone has the strongest signal and publishes:
- `sensor.person_a_location = "bathroom"`
- `sensor.person_a_distance = 1.2` (meters)

**Pros:**
- ✅ Purpose-built for this exact scenario
- ✅ Mature filtering algorithms (median + Kalman)
- ✅ Room-level accuracy tested across thousands of deployments
- ✅ Cheaper than deploying LD2410 in every room ($3 ESP32 vs $30 LD2410)

**Cons:**
- ❌ Requires person to carry phone/beacon
- ❌ WiFi infrastructure needed in all zones
- ❌ Multiple ESP32s to manage (firmware updates, etc.)

---

### Use Case 3: Hybrid Sensor Fusion

**RFD-003 Scenario:** Combine LD2410 high-accuracy bed presence + BLE identity/multi-zone.

**ESPresense Solution:**
Deploy as complementary systems:
- **Bed zone:** LD2410 for high-accuracy occupancy detection
- **Whole home:** ESPresense for person identification and room tracking
- **Fusion:** Home Assistant templates combine both data sources

**Architecture:**
```
┌──────────────────┐       ┌──────────────────┐       ┌──────────────────┐
│ ESP32 + LD2410   │       │ ESP32 (ESPresense│       │ ESP32 (ESPresense│
│ Bedroom          │       │ Bathroom)        │       │ Living Room)     │
│ - Bed occupancy  │       │ - BLE scanning   │       │ - BLE scanning   │
└────────┬─────────┘       └────────┬─────────┘       └────────┬─────────┘
         │                          │                          │
         └──────────────────────────┴──────────────────────────┘
                                    │
                           ┌────────▼────────┐
                           │ Home Assistant  │
                           │ - Fusion logic  │
                           │ - Automations   │
                           └─────────────────┘
```

**State Machine Example:**
```yaml
# Advanced presence state combining LD2410 + ESPresense
template:
  - sensor:
      - name: "Person A Presence State"
        state: >
          {% set bed_occupied = is_state('binary_sensor.bed_presence_detector_bed_occupied', 'on') %}
          {% set location = states('sensor.person_a_location') %}
          {% set distance_bedroom = states('sensor.bedroom_person_a_distance') | float(999) %}

          {% if bed_occupied and location == 'bedroom' and distance_bedroom < 2.0 %}
            in_bed
          {% elif location == 'bedroom' and not bed_occupied %}
            in_bedroom_not_in_bed
          {% elif location == 'bathroom' %}
            in_bathroom
          {% elif location == 'not_home' %}
            away
          {% else %}
            home_other
          {% endif %}
```

**Pros:**
- ✅ Best of both worlds: LD2410 reliability + ESPresense flexibility
- ✅ Each system does what it's best at
- ✅ LD2410 firmware remains simple and stable
- ✅ ESPresense is maintained by active community

**Cons:**
- ❌ Two separate systems to deploy and manage
- ❌ Fusion logic in Home Assistant (not edge-computed)
- ❌ More hardware (additional ESP32 nodes)

---

## ESPresense vs. Custom BLE Implementation (RFD-003 Phase 4.0)

| Criterion | ESPresense | Custom (RFD-003 Phase 4.0) |
|-----------|------------|----------------------------|
| **Development Effort** | ✅ Zero (use existing) | ❌ High (custom firmware, testing) |
| **Time to Deploy** | ✅ Days (flash & configure) | ⚠️ Weeks/months (development + validation) |
| **Stability** | ✅ Battle-tested (1000s of users) | ⚠️ Unknown (needs extensive testing) |
| **Feature Set** | ✅ Mature (filters, calibration, IRK) | ⚠️ Basic (Phase 4.0 scope limited) |
| **Multi-Zone** | ✅ Native support | ❌ Requires multiple ESPs or future phase |
| **Maintenance** | ✅ Community-maintained | ❌ Our responsibility |
| **ESP32 Stability Risk** | ✅ No risk (separate device) | ⚠️ High (WiFi + BLE + LD2410 contention) |
| **Privacy** | ⚠️ Same concerns (BLE tracking) | ⚠️ Same concerns |
| **Cost** | $ ~$3 per zone (ESP32 only) | $ Same hardware cost |
| **Integration Complexity** | ⚠️ MQTT + HA templates | ✅ Native ESPHome sensors |
| **Customization** | ⚠️ Limited (fork if needed) | ✅ Full control |
| **Multilateration** | ✅ Via ESPresense Companion | ❌ Not in Phase 4.0 scope |
| **Device Fingerprinting** | ✅ Built-in | ❌ Not in Phase 4.0 scope |
| **IRK Support (Apple)** | ✅ Native | ❌ Not in Phase 4.0 scope |

---

## Key Insights from ESPresense Design

If we decide to build custom BLE tracking, ESPresense provides valuable lessons:

### 1. Signal Filtering is Critical

ESPresense uses **median filter + Kalman filter** for RSSI smoothing. Raw RSSI is too noisy for reliable distance estimation.

**RFD-003 Parallel:** Similar to how our LD2410 presence engine uses z-score normalization and debouncing for stability. BLE needs even more aggressive filtering due to multipath interference.

### 2. Per-Device Calibration Required

ESPresense allows calibration of `rssi@1m` for each tracked device because:
- Different phones transmit at different power levels
- Beacon orientation affects signal strength
- Phone in pocket vs. hand changes RSSI by 5-10 dBm

**RFD-003 Implication:** The simple calibration in Phase 4.0 (single `TxPower` + `n` values) won't be accurate. Need per-device calibration profiles.

### 3. MAC Randomization is a Major Challenge

Modern iOS/Android devices randomize MAC addresses for privacy. ESPresense solves this with:
- **Fingerprinting:** Track based on device characteristics (service UUIDs, manufacturer data)
- **IRK (Identity Resolving Key):** Apple's pairing mechanism for stable device IDs

**RFD-003 Limitation:** Phase 4.0 proposal uses raw MAC addresses, which won't work reliably with modern phones. Would need to add fingerprinting or require dedicated beacons (not phones).

### 4. Multi-Node Deployment is Complex

ESPresense handles:
- Node auto-discovery via MQTT
- Aggregating data from multiple nodes
- Resolving conflicts (device seen by 2+ nodes)
- Node calibration and optimization

**RFD-003 Implication:** Phase 4.1 (multi-zone) is significantly more complex than Phase 4.0 scope suggests. ESPresense took years to mature this functionality.

### 5. Home Assistant is the Right Place for Fusion

ESPresense deliberately offloads fusion logic to Home Assistant (MQTT + templates) rather than doing it on-device. This:
- Keeps ESP32 firmware simple
- Allows users to customize fusion logic
- Enables integration with other sensors (door sensors, motion, etc.)

**RFD-003 Alignment:** This supports the "Option B" approach in RFD-003 (Home Assistant-based fusion) over "Option A" (on-device fusion).

---

## Comparison: ESPresense vs. room-assistant vs. monitor

For context, here's how ESPresense compares to alternative BLE presence solutions:

| Feature | ESPresense | room-assistant | monitor |
|---------|-----------|----------------|---------|
| **Hardware** | ESP32 (~$3) | Raspberry Pi Zero W (~$15+) | Raspberry Pi |
| **Deployment** | Multiple cheap nodes | Multiple Pis (expensive) | Single Pi per area |
| **Room Detection** | Automatic (strongest RSSI) | Automatic | Manual pairing required |
| **Setup Difficulty** | ⚠️ Moderate (MQTT, ESP32 flash) | ⚠️ Moderate (Pi setup per room) | ❌ High (manual pairing) |
| **MAC Randomization** | ✅ Fingerprinting + IRK | ✅ Supported | ⚠️ Limited |
| **Distance Estimation** | ✅ Yes (RSSI + calibration) | ✅ Yes | ✅ Yes |
| **Multilateration** | ✅ Via Companion | ✅ Built-in | ❌ No |
| **Power** | Low (ESP32) | Medium (Pi Zero) | Medium (Pi) |
| **Cost (5 rooms)** | ~$15 (5x ESP32) | ~$75+ (5x Pi Zero W) | ~$50+ |
| **OTA Updates** | ✅ Web-based | ⚠️ Manual per Pi | ⚠️ Manual |
| **Active Development** | ✅ Very active (2025+) | ⚠️ Moderate | ⚠️ Minimal |

**Verdict:** ESPresense is the most cost-effective and actively maintained solution for ESP32-based BLE presence detection.

---

## Privacy & Security: ESPresense vs. RFD-003 Analysis

Both ESPresense and the RFD-003 custom implementation share the same fundamental privacy concerns:

### Shared Concerns
- ⚠️ BLE devices broadcast identifiable signals
- ⚠️ Tracking risk (anyone with BLE scanner can detect)
- ⚠️ Movement profiling possible

### ESPresense Advantages
- ✅ Mature privacy documentation and community awareness
- ✅ Supports randomized MAC addresses (fingerprinting)
- ✅ Local-only operation (no cloud, MQTT stays on LAN)
- ✅ IRK support reduces need for static MAC addresses

### ESPresense Limitations
- ⚠️ MQTT traffic not encrypted by default (can enable TLS)
- ⚠️ No built-in beacon spoofing protection
- ⚠️ Fingerprinting can be bypassed by sophisticated attackers

**Recommendation:** For both ESPresense and custom implementations:
- Use for convenience features only (lighting, notifications)
- Do NOT use for security (locks, alarms) without additional authentication
- Enable MQTT TLS encryption
- Document privacy implications for users

---

## Proposed Integration Architectures

### Architecture 1: Separate Systems (Recommended)

**Design:** Keep LD2410 bed presence detector independent, add ESPresense as a separate whole-home presence system.

```
Bed Presence Detection:
  ESP32 + LD2410 → bed_occupied sensor → Home Assistant

Whole-Home Presence:
  3-5x ESP32 (ESPresense) → person_location sensors → Home Assistant

Fusion:
  Home Assistant templates combine both systems
```

**Implementation Steps:**
1. Deploy ESPresense nodes in key zones (bedroom, bathroom, living room, etc.)
2. Configure ESPresense to track family members' phones/beacons
3. Create HA template sensors that combine LD2410 `bed_occupied` + ESPresense `person_location`
4. Build automations using combined state

**Pros:**
- ✅ Zero risk to existing LD2410 system
- ✅ Each system optimized for its purpose
- ✅ Can deploy ESPresense incrementally (one room at a time)
- ✅ Leverage community-maintained ESPresense updates

**Cons:**
- ❌ More hardware to manage
- ❌ Fusion logic in Home Assistant (not edge)
- ❌ Requires MQTT setup

---

### Architecture 2: Shared ESP32 with BLE Proxy (Moderate Risk)

**Design:** Run LD2410 presence engine on the bedroom ESP32, add ESPHome BLE Proxy for Home Assistant's native Bluetooth integration.

```yaml
# esphome/bed-presence-detector.yaml
esphome:
  name: bed-presence-detector

# Existing LD2410 presence engine (unchanged)
packages:
  presence_engine: !include packages/presence_engine.yaml

# Add ESPHome Bluetooth Proxy
esp32_ble_tracker:
  scan_parameters:
    interval: 1100ms
    window: 1100ms
    active: false

bluetooth_proxy:
  active: true
```

**How it works:**
- ESP32 BLE Proxy forwards BLE advertisements to Home Assistant
- Home Assistant's native Bluetooth integration tracks devices
- No distance estimation, just presence/absence
- LD2410 continues to run independently

**Pros:**
- ✅ No additional ESP32 hardware needed for bedroom
- ✅ Uses native Home Assistant Bluetooth (no MQTT)
- ✅ Simple ESPHome config addition
- ✅ LD2410 code unchanged

**Cons:**
- ⚠️ ESP32 WiFi + BLE contention risk (RFD-003 concern)
- ⚠️ No per-room distance estimation
- ⚠️ Requires additional ESP32s in other rooms for multi-zone

---

### Architecture 3: Custom BLE Integration (High Effort)

**Design:** Implement RFD-003 Phase 4.0 as proposed - add BLE scanning to existing LD2410 firmware.

**Use this only if:**
- ESPresense doesn't meet requirements (unlikely)
- Need on-device fusion (edge computing requirement)
- Cannot run MQTT or additional ESP32 nodes
- Willing to invest in development and long-term maintenance

**Recommendation:** **Not recommended** unless there's a specific requirement ESPresense cannot fulfill. Development effort is high, and WiFi+BLE stability risk is significant.

---

## Recommended Path Forward

### Short-Term (Next 1-2 Months)

**1. Proof of Concept: ESPresense Evaluation**

Deploy a minimal ESPresense setup to validate it works in your environment:

- Purchase 2-3 cheap ESP32 boards (~$10 total)
- Flash ESPresense firmware using Web Tools
- Configure MQTT integration with Home Assistant
- Track 1-2 phones for 7 days
- Measure accuracy, latency, battery impact on phones

**Success Criteria:**
- [ ] Room-level detection accuracy >80%
- [ ] Latency <5 seconds for room transitions
- [ ] No phone battery complaints
- [ ] No ESP32 WiFi stability issues

**2. Create Fusion Templates**

Build Home Assistant templates that combine:
- LD2410 `bed_occupied` (high-accuracy zone detection)
- ESPresense `person_location` (identity and multi-room)

**Example:**
```yaml
template:
  - sensor:
      - name: "Master Bedroom Presence"
        state: >
          {% set bed_occupied = is_state('binary_sensor.bed_presence_detector_bed_occupied', 'on') %}
          {% set person_a_loc = states('sensor.person_a_location') %}
          {% set person_b_loc = states('sensor.person_b_location') %}

          {% if bed_occupied %}
            {% if person_a_loc == 'bedroom' and person_b_loc == 'bedroom' %}
              both_in_bed
            {% elif person_a_loc == 'bedroom' %}
              person_a_in_bed
            {% elif person_b_loc == 'bedroom' %}
              person_b_in_bed
            {% else %}
              unknown_in_bed
            {% endif %}
          {% elif person_a_loc == 'bedroom' or person_b_loc == 'bedroom' %}
            in_room_not_in_bed
          {% else %}
            vacant
          {% endif %}
```

**3. Document as Optional Enhancement**

Add ESPresense integration guide to project docs:
- `docs/espresense-integration.md`
- Position as **optional enhancement** for users who want person ID
- Emphasize that LD2410 bed presence works independently
- Provide example automations for different use cases

---

### Long-Term (6+ Months)

**4. Expand ESPresense Deployment (If PoC Successful)**

If proof of concept validates ESPresense works well:
- Deploy nodes in additional zones (bathroom, closet, living room, etc.)
- Refine calibration and templates
- Share findings with community (blog post, Home Assistant forum)

**5. Monitor Community Development**

Keep an eye on:
- **ESPresense:** New features, stability improvements, Home Assistant integration enhancements
- **Home Assistant:** Native BLE improvements (Bermuda BLE Trilateration, etc.)
- **ESPHome:** New BLE components and capabilities

**6. Revisit Custom Implementation Only If:**
- ESPresense project becomes unmaintained
- Critical feature gap identified that cannot be addressed via ESPresense
- Edge computing requirement emerges (cannot use Home Assistant for fusion)

---

## Decision Matrix: When to Choose Each Approach

| Scenario | Recommended Approach |
|----------|---------------------|
| **Want to identify who is in bed** | ESPresense (Architecture 1) |
| **Need whole-home room tracking** | ESPresense (Architecture 1) |
| **Single bedroom, no multi-zone** | LD2410 only (no BLE needed) |
| **Have MQTT infrastructure** | ESPresense (Architecture 1) |
| **No MQTT, want simple setup** | ESPHome BLE Proxy (Architecture 2) |
| **Edge computing requirement** | Custom BLE (Architecture 3) - last resort |
| **Limited budget** | ESPresense (~$3 per room) |
| **Don't want to manage hardware** | LD2410 only (keep it simple) |
| **Experimenting with BLE** | ESPresense PoC (low risk) |
| **Production-critical system** | LD2410 only + wait for BLE maturity |

---

## Open Questions for Community Discussion

1. **Use Case Priority:**
   - How many users actually need person identification vs. just occupancy?
   - Is multi-room tracking a common need, or niche feature?

2. **ESPresense Adoption Barriers:**
   - Does MQTT add too much complexity for average users?
   - Would native ESPHome integration be preferred? (There's an ESPHome component attempt in the community)

3. **Privacy Preferences:**
   - Are users comfortable with BLE tracking phones?
   - Should we recommend dedicated beacons instead of phones?

4. **Development Resources:**
   - Should we invest time in custom BLE implementation?
   - Or focus on improving LD2410 presence engine and documenting ESPresense integration?

5. **Alternative Technologies:**
   - Should we wait for UWB (Ultra-Wideband) maturity? (iPhone U1 chip, Android support coming)
   - What about WiFi presence detection? (router-based, less accurate but no beacons needed)

---

## Conclusion

**ESPresense is a mature, purpose-built solution** that addresses the BLE presence detection use cases outlined in RFD-003 without requiring us to build and maintain custom firmware.

**Recommended approach:**
1. **Keep LD2410 presence detector focused** on what it does best: high-accuracy bed occupancy detection
2. **Evaluate ESPresense** as a complementary system for person identification and multi-room tracking
3. **Combine both systems** in Home Assistant for advanced presence state machines
4. **Defer custom BLE development** unless ESPresense proves inadequate

This approach:
- ✅ Minimizes risk to the stable LD2410 system
- ✅ Leverages proven, community-maintained software
- ✅ Allows incremental deployment and experimentation
- ✅ Preserves option to build custom if needed

**The question for the community is not "should we build BLE tracking?" but rather "is ESPresense + LD2410 fusion sufficient for our needs?"**

---

## References

### ESPresense Project
- **GitHub:** https://github.com/ESPresense/ESPresense
- **Documentation:** https://espresense.com/
- **Community Forum Discussion:** https://community.home-assistant.io/t/esp-32-ble-scanner-a-room-presence-detection-solution/315205

### Related Blog Posts
- **Better Presence Detection with ESPresense:** https://www.jamesridgway.co.uk/better-presence-detection-with-home-assistant-and-espresence/
- **ESPresense Easy Room Detection:** https://blog.briancmoses.com/2022/03/espresense-easy-room-detection-for-home-assistant.html
- **Is ESPresense the Successor to ESP32-MQTT-Room?** https://home-assistant-guide.com/news/2021/09/09/is-espresense-the-successor-to-esp32-mqtt-room-weve-been-waiting-for/

### Alternative Projects (For Comparison)
- **room-assistant:** https://www.room-assistant.io/
- **monitor:** https://github.com/andrewjfreyer/monitor
- **Bermuda BLE Trilateration (HA Component):** https://www.homeautomationguy.io/blog/room-location-detection-with-bermuda-and-home-assistant-8f94b

### Our Project Docs
- `docs/RFD-003-bluetooth-beacon-distance-calibration.md` - Original proposal
- `docs/ARCHITECTURE.md` - Current LD2410 presence engine
- `docs/presence-engine-spec.md` - Phase 3 specification

---

**Next Steps:**
1. Share this analysis with the project community
2. Gather feedback on ESPresense vs. custom implementation
3. If interest exists, start ESPresense proof of concept
4. Update RFD-003 status based on community decision

---

**End of ESPresense Evaluation Document**
