"""
Bed Presence Engine Custom Component for ESPHome

Implements the full three-phase presence engine:
- Z-score calculation with runtime thresholds
- 4-state debounced state machine
- Phase 3 distance windowing + MAD-based calibration services
"""
import esphome.codegen as cg
from esphome.components import binary_sensor

DEPENDENCIES = []
AUTO_LOAD = ["binary_sensor"]

# Define the namespace and class
bed_presence_engine_ns = cg.esphome_ns.namespace("bed_presence_engine")
BedPresenceEngine = bed_presence_engine_ns.class_(
    "BedPresenceEngine",
    cg.Component,
    binary_sensor.BinarySensor
)
