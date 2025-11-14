# ESPresence: A Mature Alternative to Custom BLE Implementation

## TL;DR

**ESPresense** is an actively-maintained, open-source ESP32-based BLE presence detection system that directly addresses the use cases outlined in RFD-003. Rather than building custom BLE tracking into our LD2410 firmware (Phase 4.0), we should evaluate using ESPresense as a **complementary system**.

**Full Analysis:** See `docs/RFD-003-espresense-evaluation.md`

---

## What is ESPresense?

ESPresense is a purpose-built solution for room-level BLE presence detection:

- **Hardware:** Low-cost ESP32 boards (~$3 each)
- **Tracking:** Phones, smartwatches, BLE beacons - tracks people room-to-room
- **Integration:** MQTT + Home Assistant's `mqtt_room` component
- **Features:** MAC randomization support (iOS/Android), distance estimation, multilateration, Kalman filtering
- **Active Development:** Mature project with large community (2021-2025+)

**Project Links:**
- GitHub: https://github.com/ESPresense/ESPresense
- Docs: https://espresense.com/
- License: AGPL-3.0

---

## How ESPresense Addresses RFD-003 Use Cases

### ✅ Use Case 1: Person Identification
Combine LD2410 bed occupancy with ESPresense location tracking:
- LD2410: "Bed is occupied" (high accuracy, >95%)
- ESPresense: "Person A is in bedroom" (room-level detection)
- Home Assistant: Fuses both → "Person A is in bed"

### ✅ Use Case 2: Multi-Zone Tracking
Deploy multiple ESP32 nodes (bedroom, bathroom, living room, etc.):
- Automatic room detection based on strongest RSSI
- Tracks person movement throughout home
- Far cheaper than deploying LD2410 in every room ($3 vs $30)

### ✅ Use Case 3: Hybrid Sensor Fusion
Best of both worlds:
- **LD2410:** High-accuracy bed presence (remains primary sensor)
- **ESPresense:** Person identity + multi-room tracking
- **Home Assistant:** Templates combine both data sources

---

## ESPresense vs. Custom BLE (RFD-003 Phase 4.0)

| Criterion | ESPresense | Custom (Phase 4.0) |
|-----------|------------|-------------------|
| Development Effort | ✅ Zero | ❌ High (weeks/months) |
| Stability | ✅ Battle-tested (1000s of users) | ⚠️ Unknown (needs validation) |
| Feature Set | ✅ Mature (filters, calibration, IRK, multilateration) | ⚠️ Basic (limited scope) |
| Multi-Zone | ✅ Native support | ❌ Future phase |
| ESP32 Stability Risk | ✅ No risk (separate device) | ⚠️ High (WiFi+BLE+LD2410 contention) |
| Maintenance | ✅ Community-maintained | ❌ Our responsibility |
| MAC Randomization | ✅ Fingerprinting + IRK | ❌ Not in Phase 4.0 scope |
| Customization | ⚠️ Limited (fork if needed) | ✅ Full control |

---

## Recommended Architecture: Separate Complementary Systems

**Keep it simple:**
```
Bed Presence:
  ESP32 + LD2410 → bed_occupied sensor → Home Assistant
  (Unchanged, reliable, focused)

Whole-Home Presence:
  3-5x ESP32 (ESPresense) → person_location sensors → Home Assistant
  (Purpose-built, community-maintained)

Fusion:
  Home Assistant templates combine both systems
  (Flexible, user-customizable)
```

**Why this approach:**
- ✅ Zero risk to existing LD2410 system
- ✅ Each system optimized for its purpose
- ✅ Leverage community-maintained ESPresense
- ✅ Can deploy incrementally (PoC → full deployment)
- ✅ No WiFi+BLE contention on bedroom ESP32

---

## Key Lessons from ESPresense Design

If we decide to build custom BLE, ESPresense teaches us:

1. **Filtering is critical:** Median + Kalman filters needed for stable RSSI (RFD-003 underestimates this)
2. **Per-device calibration required:** Different phones/beacons need different `rssi@1m` values
3. **MAC randomization is complex:** Modern phones randomize MACs - need fingerprinting or IRK support
4. **Multi-node is hard:** Aggregation, conflict resolution, calibration took ESPresense years to mature
5. **Home Assistant fusion makes sense:** Offloading fusion logic keeps ESP32 firmware simple

---

## Recommended Path Forward

### Short-Term (1-2 Months): Proof of Concept

1. **Test ESPresense:**
   - Purchase 2-3 ESP32 boards (~$10 total)
   - Flash ESPresense, configure MQTT
   - Track 1-2 phones for 7 days
   - Measure accuracy, latency, stability

2. **Build Fusion Templates:**
   - Create HA templates combining LD2410 + ESPresense
   - Test person identification accuracy
   - Validate with different use cases

3. **Document as Optional Enhancement:**
   - Add `docs/espresense-integration.md`
   - Position as opt-in for users who want person ID
   - Emphasize LD2410 works independently

### Long-Term (6+ Months): Expand or Revisit

- **If PoC succeeds:** Deploy additional nodes, refine templates, share with community
- **If PoC fails:** Investigate issues, consider alternatives
- **Monitor developments:** ESPresense updates, HA Bluetooth improvements, UWB maturity

**Only build custom BLE if:**
- ESPresense proves inadequate (unlikely)
- Critical feature gap identified
- Edge computing requirement (cannot use HA for fusion)

---

## Discussion Questions

1. **Use Cases:** How many users need person identification vs. just occupancy detection?
2. **Complexity:** Is MQTT + ESPresense too complex vs. native ESPHome integration?
3. **Privacy:** Should we recommend dedicated beacons or phone tracking?
4. **Resources:** Should we invest in custom BLE or focus on LD2410 improvements + ESPresense integration docs?
5. **Alternatives:** Wait for UWB maturity? Explore WiFi presence detection?

---

## Conclusion

**ESPresense is a mature, proven solution** that addresses RFD-003 use cases without custom development.

**Recommendation:**
- ✅ Evaluate ESPresense as complementary system (Architecture 1)
- ✅ Keep LD2410 focused on high-accuracy bed presence
- ✅ Combine both in Home Assistant for advanced state machines
- ⚠️ Defer custom BLE unless ESPresense inadequate

**The question isn't "should we build BLE tracking?" but "is ESPresense + LD2410 fusion sufficient?"**

---

## Next Steps

1. ✅ **Shared this analysis** with project community
2. ⏳ **Gather feedback** on ESPresense vs. custom approach
3. ⏳ **If interest exists:** Start ESPresense proof of concept
4. ⏳ **Update RFD-003** status based on community decision

**Full technical analysis:** `docs/RFD-003-espresense-evaluation.md`

---

**Related Documents:**
- `docs/RFD-003-bluetooth-beacon-distance-calibration.md` - Original proposal
- `docs/RFD-003-espresense-evaluation.md` - Full ESPresense analysis (this summary)
- `docs/ARCHITECTURE.md` - Current LD2410 presence engine
