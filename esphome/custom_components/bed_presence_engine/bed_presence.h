#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <string>
#include <vector>

namespace esphome {
namespace bed_presence_engine {

// Phase 2: State machine states
enum State {
  IDLE,           // No presence detected (binary sensor: OFF)
  DEBOUNCING_ON,  // High signal detected, timer running (binary sensor: OFF)
  PRESENT,        // Confirmed presence (binary sensor: ON)
  DEBOUNCING_OFF  // Low signal detected, timer running (binary sensor: ON)
};

/**
 * BedPresenceEngine Component - Phase 2 Implementation
 *
 * Implements z-score based presence detection with temporal filtering:
 * - Calculates z-score: z = (energy - μ) / σ
 * - Compares against threshold multipliers k_on and k_off
 * - 4-state machine with debouncing (IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF)
 * - Eliminates "twitchiness" through sustained condition requirements
 * - Absolute clear delay prevents premature clearing after recent high signals
 */
class BedPresenceEngine : public Component, public binary_sensor::BinarySensor {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Configuration setters
  void set_energy_sensor(sensor::Sensor *sensor) { energy_sensor_ = sensor; }
  void set_k_on(float k) { k_on_ = k; }
  void set_k_off(float k) { k_off_ = k; }
  void set_on_debounce_ms(unsigned long ms) { on_debounce_ms_ = ms; }
  void set_off_debounce_ms(unsigned long ms) { off_debounce_ms_ = ms; }
  void set_abs_clear_delay_ms(unsigned long ms) { abs_clear_delay_ms_ = ms; }
  void set_state_reason_sensor(text_sensor::TextSensor *sensor) { state_reason_sensor_ = sensor; }
  void set_last_change_reason_sensor(text_sensor::TextSensor *sensor) { last_change_reason_sensor_ = sensor; }
  void set_distance_sensor(sensor::Sensor *sensor) { distance_sensor_ = sensor; }
  void set_d_min_cm(float value) { d_min_cm_ = value; }
  void set_d_max_cm(float value) { d_max_cm_ = value; }

  // Public methods for runtime updates from HA
  void update_k_on(float k);
  void update_k_off(float k);
  void update_on_debounce_ms(unsigned long ms);
  void update_off_debounce_ms(unsigned long ms);
  void update_abs_clear_delay_ms(unsigned long ms);
  void update_d_min_cm(float value);
  void update_d_max_cm(float value);

  // Calibration + reset services
  void start_baseline_calibration(uint32_t duration_s);
  void stop_baseline_calibration();
  void reset_to_defaults();

 protected:
  // Input sensor
  sensor::Sensor *energy_sensor_{nullptr};
  sensor::Sensor *distance_sensor_{nullptr};

  // Baseline calibration collected on 2025-11-06 18:39:42
  // Location: New sensor position looking at bed
  // Conditions: Empty bed, door closed, minimal movement
  // Statistics: mean=6.67%, stdev=3.51%, n=30 samples over 60 seconds
  // Phase 2: Renamed from mu_move_/sigma_move_ for semantic correctness (measures still_energy)
  float mu_still_{6.7f};    // Mean still energy (empty bed)
  float sigma_still_{3.5f}; // Std dev still energy (empty bed)
  float mu_stat_{6.7f};     // Reserved for Phase 3 (moving energy fusion)
  float sigma_stat_{3.5f};  // Reserved for Phase 3 (moving energy fusion)

  // Threshold multipliers (k_on > k_off for hysteresis)
  float k_on_{9.0f};   // Turn ON when z > k_on (default: 9 std deviations)
  float k_off_{4.0f};  // Turn OFF when z < k_off (default: 4 std deviations)

  // Phase 3: Distance window (cm)
  float d_min_cm_{0.0f};
  float d_max_cm_{600.0f};

  // Phase 2: State machine (replaces simple boolean)
  State current_state_{IDLE};

  // Phase 2: Debounce timers
  unsigned long debounce_start_time_{0};      // Timestamp when current debounce started
  unsigned long last_high_confidence_time_{0}; // Last time z_still > k_on (while in PRESENT)
  unsigned long on_debounce_ms_{3000};         // Default: 3 seconds
  unsigned long off_debounce_ms_{5000};        // Default: 5 seconds
  unsigned long abs_clear_delay_ms_{30000};    // Default: 30 seconds

  // Output sensors
  text_sensor::TextSensor *state_reason_sensor_{nullptr};
  text_sensor::TextSensor *last_change_reason_sensor_{nullptr};

  // Internal methods
  float calculate_z_score(float energy, float mu, float sigma);
  void process_energy_reading(float energy);
  void publish_reason(const std::string &reason);
  void publish_change_reason(const std::string &reason);

  // Calibration helpers
  void handle_calibration_sample(float energy);
  void finalize_calibration();

  bool calibrating_{false};
  unsigned long calibration_end_time_{0};
  std::vector<float> calibration_samples_;
  static constexpr size_t MAX_CALIBRATION_SAMPLES = 4096;
};

}  // namespace bed_presence_engine
}  // namespace esphome
