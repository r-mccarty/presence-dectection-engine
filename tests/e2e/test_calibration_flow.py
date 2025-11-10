"""
End-to-End Integration Tests for Bed Presence Detector

These tests verify the full integration between Home Assistant and the ESPHome device,
including the calibration wizard flow and threshold updates.

Environment Variables Required:
- HA_URL: WebSocket URL for Home Assistant (e.g., ws://homeassistant.local:8123/api/websocket)
- HA_TOKEN: Long-lived access token for Home Assistant
"""

import os
import pytest
import asyncio
from hass_ws import HomeAssistantClient


@pytest.fixture
async def ha_client():
    """Fixture to create and authenticate Home Assistant WebSocket client"""
    url = os.getenv("HA_URL")
    token = os.getenv("HA_TOKEN")

    if not url or not token:
        pytest.skip("HA_URL and HA_TOKEN environment variables must be set")

    client = HomeAssistantClient(url, token)
    await client.connect()
    yield client
    await client.disconnect()


@pytest.mark.asyncio
async def test_device_is_connected(ha_client):
    """Test that the bed presence detector device is connected to Home Assistant"""
    # Get device information
    devices = await ha_client.get_devices()

    # Find our device
    bed_detector = next(
        (d for d in devices if "bed-presence-detector" in d.get("name", "").lower()),
        None
    )

    assert bed_detector is not None, "Bed presence detector device not found"
    assert bed_detector.get("disabled_by") is None, "Device is disabled"


@pytest.mark.asyncio
async def test_presence_sensor_exists(ha_client):
    """Test that the bed occupied binary sensor exists"""
    state = await ha_client.get_state("binary_sensor.bed_presence_detector_bed_occupied")
    assert state is not None, "binary_sensor.bed_presence_detector_bed_occupied not found"
    assert state["state"] in ["on", "off"], "Invalid state for presence sensor"


@pytest.mark.asyncio
async def test_threshold_entities_exist(ha_client):
    """Test that threshold configuration entities exist"""
    k_on_threshold = await ha_client.get_state("number.bed_presence_detector_k_on_on_threshold_multiplier")
    k_off_threshold = await ha_client.get_state("number.bed_presence_detector_k_off_off_threshold_multiplier")

    assert k_on_threshold is not None, "number.bed_presence_detector_k_on_on_threshold_multiplier not found"
    assert k_off_threshold is not None, "number.bed_presence_detector_k_off_off_threshold_multiplier not found"

    # Verify thresholds are sensible (k_on should be higher than k_off for hysteresis)
    k_on_val = float(k_on_threshold["state"])
    k_off_val = float(k_off_threshold["state"])
    assert k_on_val > k_off_val, "k_on threshold should be higher than k_off for hysteresis"


@pytest.mark.asyncio
async def test_update_threshold_via_service(ha_client):
    """Test that we can update thresholds via Home Assistant service call"""
    # Set a new k_on threshold value (Phase 1: z-score multiplier, typical range 0-10)
    await ha_client.call_service(
        "number",
        "set_value",
        entity_id="number.bed_presence_detector_k_on_on_threshold_multiplier",
        value=5.0
    )

    # Wait for update to propagate
    await asyncio.sleep(1)

    # Verify the update
    state = await ha_client.get_state("number.bed_presence_detector_k_on_on_threshold_multiplier")
    assert float(state["state"]) == 5.0, "k_on threshold was not updated"


@pytest.mark.asyncio
async def test_calibration_service_exists(ha_client):
    """Test that the calibration ESPHome services are available"""
    services = await ha_client.get_services()

    # Check that our custom ESPHome services exist
    esphome_services = services.get("esphome", {})

    required = [
        "bed_presence_detector_start_calibration",
        "bed_presence_detector_calibrate_start_baseline",
        "bed_presence_detector_stop_calibration",
        "bed_presence_detector_calibrate_stop",
        "bed_presence_detector_reset_to_defaults",
        "bed_presence_detector_calibrate_reset_all",
    ]

    for service_name in required:
        assert service_name in esphome_services, f"{service_name} service not found"


@pytest.mark.asyncio
async def test_reset_to_defaults(ha_client):
    """Test the reset to defaults service"""
    # Call reset service
    await ha_client.call_service(
        "esphome",
        "bed_presence_detector_reset_to_defaults"
    )

    # Wait for reset to complete
    await asyncio.sleep(2)

    # Verify defaults are restored (Phase 3 defaults)
    k_on_state = await ha_client.get_state("number.bed_presence_detector_k_on_on_threshold_multiplier")
    k_off_state = await ha_client.get_state("number.bed_presence_detector_k_off_off_threshold_multiplier")
    on_debounce = await ha_client.get_state("number.bed_presence_detector_on_debounce_ms")
    off_debounce = await ha_client.get_state("number.bed_presence_detector_off_debounce_ms")
    abs_clear = await ha_client.get_state("number.bed_presence_detector_absolute_clear_delay_ms")
    d_min = await ha_client.get_state("number.bed_presence_detector_distance_min_cm")
    d_max = await ha_client.get_state("number.bed_presence_detector_distance_max_cm")

    assert float(k_on_state["state"]) == 9.0, "k_on threshold not reset to default (9.0)"
    assert float(k_off_state["state"]) == 4.0, "k_off threshold not reset to default (4.0)"
    assert float(on_debounce["state"]) == 3000, "on_debounce_ms not reset"
    assert float(off_debounce["state"]) == 5000, "off_debounce_ms not reset"
    assert float(abs_clear["state"]) == 30000, "abs_clear_delay_ms not reset"
    assert float(d_min["state"]) == 0.0, "distance_min not reset"
    assert float(d_max["state"]) == 600.0, "distance_max not reset"


@pytest.mark.asyncio
async def test_state_reason_sensor(ha_client):
    """Test that the state reason text sensor is available and updating"""
    state = await ha_client.get_state("sensor.bed_presence_detector_presence_state_reason")
    assert state is not None, "State reason sensor not found"
    assert len(state["state"]) > 0, "State reason is empty"


@pytest.mark.asyncio
async def test_change_reason_sensor(ha_client):
    """Test that the change reason text sensor is available and updating"""
    state = await ha_client.get_state("sensor.bed_presence_detector_presence_change_reason")
    assert state is not None, "Change reason sensor not found"
    assert len(state["state"]) > 0, "Change reason is empty"


@pytest.mark.asyncio
async def test_calibration_helpers_exist(ha_client):
    """Test that the calibration helper entities exist in Home Assistant"""
    helper_entities = [
        "input_select.bed_presence_calibration_step",
        "input_boolean.bed_presence_calibration_in_progress",
        "input_boolean.bed_presence_calibration_confirm_empty_bed",
        "input_number.bed_presence_calibration_duration_seconds",
        "input_datetime.bed_presence_last_calibration",
        "script.bed_presence_start_baseline_calibration",
        "script.bed_presence_cancel_baseline_calibration",
        "script.bed_presence_reset_calibration_defaults",
    ]

    missing = []
    for entity_id in helper_entities:
        state = await ha_client.get_state(entity_id)
        if state is None:
            missing.append(entity_id)

    assert not missing, f"Missing calibration helper entities: {missing}"


@pytest.mark.skip(reason="Requires physical calibration cycle with empty bed")
@pytest.mark.asyncio
async def test_full_calibration_flow(ha_client):
    """
    Test the full calibration workflow:
    1. Start vacant calibration
    2. Wait for completion
    3. Start occupied calibration
    4. Wait for completion
    5. Verify thresholds were calculated
    """
    # Reset to known state
    await ha_client.call_service(
        "esphome",
        "bed_presence_detector_reset_to_defaults"
    )
    await asyncio.sleep(1)

    # Start vacant calibration script
    await ha_client.call_service("script", "bed_presence_start_baseline_calibration")

    # Wait for calibration to complete (script has 30s delay)
    await asyncio.sleep(35)

    # Check that calibration step was updated
    step_state = await ha_client.get_state("input_select.bed_presence_calibration_step")
    assert step_state["state"] in ["Finalizing", "Completed"], \
        f"Unexpected calibration step: {step_state['state']}"

    # Note: Full flow would continue with occupied calibration,
    # but we keep this test short for CI/CD purposes


@pytest.mark.asyncio
async def test_phase3_configuration_entities_exist(ha_client):
    """Test that Phase 3 configuration entities exist"""
    on_debounce = await ha_client.get_state("number.bed_presence_detector_on_debounce_ms")
    off_debounce = await ha_client.get_state("number.bed_presence_detector_off_debounce_ms")
    abs_clear = await ha_client.get_state("number.bed_presence_detector_absolute_clear_delay_ms")
    d_min = await ha_client.get_state("number.bed_presence_detector_distance_min_cm")
    d_max = await ha_client.get_state("number.bed_presence_detector_distance_max_cm")

    assert on_debounce is not None, "number.bed_presence_detector_on_debounce_ms not found"
    assert off_debounce is not None, "number.bed_presence_detector_off_debounce_ms not found"
    assert abs_clear is not None, "number.bed_presence_detector_absolute_clear_delay_ms not found"
    assert d_min is not None, "number.bed_presence_detector_distance_min_cm not found"
    assert d_max is not None, "number.bed_presence_detector_distance_max_cm not found"

    # Verify values are reasonable
    assert float(on_debounce["state"]) >= 0, "on_debounce_ms should be non-negative"
    assert float(off_debounce["state"]) >= 0, "off_debounce_ms should be non-negative"
    assert float(abs_clear["state"]) >= 0, "abs_clear_delay_ms should be non-negative"
    assert 0.0 <= float(d_min["state"]) <= 600.0, "distance_min_cm should be within 0-600"
    assert 0.0 <= float(d_max["state"]) <= 600.0, "distance_max_cm should be within 0-600"


@pytest.mark.asyncio
async def test_phase2_update_debounce_timers(ha_client):
    """Test that we can update debounce timers via Home Assistant service call"""
    # Set a new on_debounce value (Phase 2: milliseconds, typical range 0-10000)
    await ha_client.call_service(
        "number",
        "set_value",
        entity_id="number.bed_presence_detector_on_debounce_ms",
        value=5000
    )

    # Wait for update to propagate
    await asyncio.sleep(1)

    # Verify the update
    state = await ha_client.get_state("number.bed_presence_detector_on_debounce_ms")
    assert float(state["state"]) == 5000, "on_debounce_ms was not updated"


@pytest.mark.asyncio
async def test_phase3_update_distance_window(ha_client):
    """Test that we can update the distance window entities"""
    await ha_client.call_service(
        "number",
        "set_value",
        entity_id="number.bed_presence_detector_distance_min_cm",
        value=100
    )
    await asyncio.sleep(1)
    await ha_client.call_service(
        "number",
        "set_value",
        entity_id="number.bed_presence_detector_distance_max_cm",
        value=300
    )
    await asyncio.sleep(1)

    d_min = await ha_client.get_state("number.bed_presence_detector_distance_min_cm")
    d_max = await ha_client.get_state("number.bed_presence_detector_distance_max_cm")
    assert float(d_min["state"]) == 100.0, "distance_min_cm was not updated"
    assert float(d_max["state"]) == 300.0, "distance_max_cm was not updated"


@pytest.mark.asyncio
async def test_phase2_state_machine_monitoring(ha_client):
    """Test Phase 2 state machine by monitoring state changes over time"""
    # Collect 10 samples over 10 seconds to observe state machine behavior
    samples = []

    for _ in range(10):
        state = await ha_client.get_state("binary_sensor.bed_presence_detector_bed_occupied")
        reason = await ha_client.get_state("sensor.bed_presence_detector_presence_state_reason")
        energy = await ha_client.get_state("sensor.bed_presence_detector_ld2410_still_energy")

        samples.append({
            "presence": state["state"],
            "reason": reason["state"],
            "energy": float(energy["state"])
        })

        await asyncio.sleep(1)

    # Verify we got valid data from all samples
    assert len(samples) == 10, "Did not collect all samples"
    assert all(s["presence"] in ["on", "off"] for s in samples), "Invalid presence states"
    assert all(len(s["reason"]) > 0 for s in samples), "Empty state reasons"
    assert all(s["energy"] >= 0 for s in samples), "Invalid energy readings"


@pytest.mark.asyncio
async def test_phase2_z_score_calculation(ha_client):
    """Test that z-score calculations are reflected in state reason"""
    # Get current state reason which should contain z-score information
    reason = await ha_client.get_state("sensor.bed_presence_detector_presence_state_reason")

    assert reason is not None, "State reason sensor not found"
    reason_text = reason["state"]

    # Phase 2 state reason should contain z-score information
    # Format example: "z=5.23, state=PRESENT" or "z=-1.45, state=IDLE"
    assert "z=" in reason_text.lower() or "z-score" in reason_text.lower(), \
        f"State reason does not contain z-score information: {reason_text}"


@pytest.mark.asyncio
async def test_phase2_hysteresis_validation(ha_client):
    """Test that hysteresis gap is maintained (k_on > k_off)"""
    k_on = await ha_client.get_state("number.bed_presence_detector_k_on_on_threshold_multiplier")
    k_off = await ha_client.get_state("number.bed_presence_detector_k_off_off_threshold_multiplier")

    k_on_val = float(k_on["state"])
    k_off_val = float(k_off["state"])

    # Hysteresis requires k_on > k_off
    assert k_on_val > k_off_val, \
        f"Hysteresis violation: k_on ({k_on_val}) should be greater than k_off ({k_off_val})"

    # Reasonable gap (at least 1.0 sigma for good hysteresis)
    gap = k_on_val - k_off_val
    assert gap >= 1.0, \
        f"Hysteresis gap too small: {gap:.2f}σ (recommend >= 1.0σ)"


@pytest.mark.asyncio
async def test_phase2_sensor_raw_data_available(ha_client):
    """Test that raw LD2410 sensor data is available"""
    still_energy = await ha_client.get_state("sensor.bed_presence_detector_ld2410_still_energy")

    assert still_energy is not None, "LD2410 still energy sensor not found"
    assert still_energy["state"] not in ["unavailable", "unknown"], \
        "LD2410 sensor is unavailable"

    # Energy should be a percentage (0-100)
    energy_val = float(still_energy["state"])
    assert 0 <= energy_val <= 100, \
        f"LD2410 energy out of range: {energy_val}% (expected 0-100%)"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
