"""Binary Sensor Platform for Bed Presence Engine"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, text_sensor
from esphome.const import CONF_ID, DEVICE_CLASS_OCCUPANCY

from . import bed_presence_engine_ns, BedPresenceEngine

# Configuration keys
CONF_ENERGY_SENSOR = "energy_sensor"
CONF_DISTANCE_SENSOR = "distance_sensor"
CONF_K_ON = "k_on"
CONF_K_OFF = "k_off"
CONF_ON_DEBOUNCE_MS = "on_debounce_ms"
CONF_OFF_DEBOUNCE_MS = "off_debounce_ms"
CONF_ABS_CLEAR_DELAY_MS = "abs_clear_delay_ms"
CONF_DISTANCE_MIN = "distance_min_cm"
CONF_DISTANCE_MAX = "distance_max_cm"
CONF_STATE_REASON = "state_reason"
CONF_LAST_CHANGE_REASON = "last_change_reason"

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(
    BedPresenceEngine,
    device_class=DEVICE_CLASS_OCCUPANCY
).extend(
    {
        cv.GenerateID(): cv.declare_id(BedPresenceEngine),
        cv.Required(CONF_ENERGY_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_K_ON, default=9.0): cv.float_range(min=0.0, max=15.0),
        cv.Optional(CONF_K_OFF, default=4.0): cv.float_range(min=0.0, max=15.0),
        cv.Optional(CONF_ON_DEBOUNCE_MS, default=3000): cv.positive_int,
        cv.Optional(CONF_OFF_DEBOUNCE_MS, default=5000): cv.positive_int,
        cv.Optional(CONF_ABS_CLEAR_DELAY_MS, default=30000): cv.positive_int,
        cv.Optional(CONF_STATE_REASON): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_LAST_CHANGE_REASON): text_sensor.text_sensor_schema(),
        cv.Optional(CONF_DISTANCE_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_DISTANCE_MIN, default=0.0): cv.float_range(min=0.0, max=1000.0),
        cv.Optional(CONF_DISTANCE_MAX, default=600.0): cv.float_range(min=0.0, max=1000.0),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await binary_sensor.register_binary_sensor(var, config)

    energy_sensor = await cg.get_variable(config[CONF_ENERGY_SENSOR])
    cg.add(var.set_energy_sensor(energy_sensor))

    if CONF_DISTANCE_SENSOR in config:
        distance_sensor = await cg.get_variable(config[CONF_DISTANCE_SENSOR])
        cg.add(var.set_distance_sensor(distance_sensor))

    cg.add(var.set_d_min_cm(config[CONF_DISTANCE_MIN]))
    cg.add(var.set_d_max_cm(config[CONF_DISTANCE_MAX]))

    cg.add(var.set_k_on(config[CONF_K_ON]))
    cg.add(var.set_k_off(config[CONF_K_OFF]))

    # Phase 2: Debounce timers
    cg.add(var.set_on_debounce_ms(config[CONF_ON_DEBOUNCE_MS]))
    cg.add(var.set_off_debounce_ms(config[CONF_OFF_DEBOUNCE_MS]))
    cg.add(var.set_abs_clear_delay_ms(config[CONF_ABS_CLEAR_DELAY_MS]))

    if CONF_STATE_REASON in config:
        reason_sensor = await text_sensor.new_text_sensor(config[CONF_STATE_REASON])
        cg.add(var.set_state_reason_sensor(reason_sensor))

    if CONF_LAST_CHANGE_REASON in config:
        change_reason_sensor = await text_sensor.new_text_sensor(config[CONF_LAST_CHANGE_REASON])
        cg.add(var.set_last_change_reason_sensor(change_reason_sensor))
