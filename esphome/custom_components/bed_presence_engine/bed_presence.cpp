#include "bed_presence.h"
#include "esphome/core/log.h"
#include <algorithm>
#include <cmath>

namespace esphome {
namespace bed_presence_engine {

static const char *const TAG = "bed_presence_engine";

void BedPresenceEngine::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Bed Presence Engine (Phase 3)...");
  ESP_LOGCONFIG(TAG, "  Baseline (still): μ=%.2f, σ=%.2f", this->mu_still_, this->sigma_still_);
  ESP_LOGCONFIG(TAG, "  Baseline (stat): μ=%.2f, σ=%.2f", this->mu_stat_, this->sigma_stat_);
  ESP_LOGCONFIG(TAG, "  Threshold multipliers: k_on=%.2f, k_off=%.2f", this->k_on_, this->k_off_);
  ESP_LOGCONFIG(TAG, "  Debounce timers: on=%lums, off=%lums, abs_clear=%lums",
                this->on_debounce_ms_, this->off_debounce_ms_, this->abs_clear_delay_ms_);
  ESP_LOGCONFIG(TAG, "  Distance window: [%.1fcm, %.1fcm]", this->d_min_cm_, this->d_max_cm_);
  ESP_LOGCONFIG(TAG, "  Phase 3: Distance windowing + MAD calibration enabled");

  // Initialize to IDLE state
  this->current_state_ = IDLE;
  this->publish_state(false);

  if (this->state_reason_sensor_ != nullptr) {
    this->state_reason_sensor_->publish_state("Initial state: IDLE");
  }
  if (this->last_change_reason_sensor_ != nullptr) {
    this->last_change_reason_sensor_->publish_state("idle:init");
  }
}

void BedPresenceEngine::loop() {
  if (this->calibrating_ && millis() >= this->calibration_end_time_) {
    this->finalize_calibration();
  }

  // Check if we have a valid energy reading
  if (this->energy_sensor_ == nullptr || !this->energy_sensor_->has_state()) {
    return;
  }

  if (this->distance_sensor_ != nullptr && this->distance_sensor_->has_state()) {
    float distance = this->distance_sensor_->state;
    if (distance < this->d_min_cm_ || distance > this->d_max_cm_) {
      ESP_LOGVV(TAG, "Ignoring frame, distance %.2fcm outside window [%.1fcm, %.1fcm]", distance, this->d_min_cm_,
                this->d_max_cm_);
      return;
    }
  }

  float energy = this->energy_sensor_->state;
  this->handle_calibration_sample(energy);
  this->process_energy_reading(energy);
}

float BedPresenceEngine::calculate_z_score(float energy, float mu, float sigma) {
  // Prevent division by zero
  if (sigma <= 0.001f) {
    ESP_LOGW(TAG, "Invalid sigma (%.2f), returning z=0", sigma);
    return 0.0f;
  }

  // z = (x - μ) / σ
  return (energy - mu) / sigma;
}

void BedPresenceEngine::process_energy_reading(float energy) {
  // Calculate z-score for still energy (Phase 2 uses still_energy)
  float z_still = this->calculate_z_score(energy, this->mu_still_, this->sigma_still_);

  // Log the z-score for debugging
  ESP_LOGVV(TAG, "Energy=%.2f, z_still=%.2f, state=%d", energy, z_still, this->current_state_);

  unsigned long now = millis();

  // Phase 2 Logic: 4-state machine with debouncing
  switch (this->current_state_) {
    case IDLE:
      if (z_still >= this->k_on_) {
        this->debounce_start_time_ = now;
        this->current_state_ = DEBOUNCING_ON;
        ESP_LOGD(TAG, "IDLE → DEBOUNCING_ON (z=%.2f >= k_on=%.2f)", z_still, this->k_on_);
      }
      break;

    case DEBOUNCING_ON:
      if (z_still >= this->k_on_) {
        // Condition still holds, check timer
        if ((now - this->debounce_start_time_) >= this->on_debounce_ms_) {
          this->current_state_ = PRESENT;
          this->last_high_confidence_time_ = now;
          this->publish_state(true);

          char reason[64];
          snprintf(reason, sizeof(reason), "ON: z=%.2f, debounced %lums", z_still, this->on_debounce_ms_);
          this->publish_reason(reason);
          this->publish_change_reason("on:threshold_exceeded");

          ESP_LOGI(TAG, "DEBOUNCING_ON → PRESENT: %s", reason);
        }
      } else {
        // Condition lost, abort debounce
        this->current_state_ = IDLE;
        ESP_LOGD(TAG, "DEBOUNCING_ON → IDLE (z=%.2f < k_on, abort)", z_still);
      }
      break;

    case PRESENT:
      // Update high confidence timestamp whenever strong signal detected
      if (z_still > this->k_on_) {
        this->last_high_confidence_time_ = now;
      }

      // Check for transition to DEBOUNCING_OFF
      if (z_still < this->k_off_) {
        // Low signal detected, check absolute clear delay
        if ((now - this->last_high_confidence_time_) >= this->abs_clear_delay_ms_) {
          this->debounce_start_time_ = now;
          this->current_state_ = DEBOUNCING_OFF;
          ESP_LOGD(TAG, "PRESENT → DEBOUNCING_OFF (z=%.2f < k_off, abs_clear=%lums ago)",
                   z_still, (now - this->last_high_confidence_time_));
        }
      }
      break;

    case DEBOUNCING_OFF:
      if (z_still < this->k_off_) {
        // Condition still holds, check timer
        if ((now - this->debounce_start_time_) >= this->off_debounce_ms_) {
          this->current_state_ = IDLE;
          this->publish_state(false);

          char reason[64];
          snprintf(reason, sizeof(reason), "OFF: z=%.2f, debounced %lums", z_still, this->off_debounce_ms_);
          this->publish_reason(reason);
          this->publish_change_reason("off:abs_clear_delay");

          ESP_LOGI(TAG, "DEBOUNCING_OFF → IDLE: %s", reason);
        }
      } else if (z_still >= this->k_on_) {
        // High signal returned, abort debounce
        this->current_state_ = PRESENT;
        this->last_high_confidence_time_ = now;
        ESP_LOGD(TAG, "DEBOUNCING_OFF → PRESENT (z=%.2f >= k_on, signal returned)", z_still);
      }
      break;
  }
}

void BedPresenceEngine::publish_reason(const std::string &reason) {
  if (this->state_reason_sensor_ != nullptr) {
    this->state_reason_sensor_->publish_state(reason.c_str());
  }
}

void BedPresenceEngine::publish_change_reason(const std::string &reason) {
  if (this->last_change_reason_sensor_ != nullptr) {
    this->last_change_reason_sensor_->publish_state(reason.c_str());
  }
}

void BedPresenceEngine::update_k_on(float k) {
  ESP_LOGI(TAG, "Updating k_on: %.2f -> %.2f", this->k_on_, k);
  this->k_on_ = k;
}

void BedPresenceEngine::update_k_off(float k) {
  ESP_LOGI(TAG, "Updating k_off: %.2f -> %.2f", this->k_off_, k);
  this->k_off_ = k;
}

void BedPresenceEngine::update_on_debounce_ms(unsigned long ms) {
  ESP_LOGI(TAG, "Updating on_debounce_ms: %lu -> %lu", this->on_debounce_ms_, ms);
  this->on_debounce_ms_ = ms;
}

void BedPresenceEngine::update_off_debounce_ms(unsigned long ms) {
  ESP_LOGI(TAG, "Updating off_debounce_ms: %lu -> %lu", this->off_debounce_ms_, ms);
  this->off_debounce_ms_ = ms;
}

void BedPresenceEngine::update_abs_clear_delay_ms(unsigned long ms) {
  ESP_LOGI(TAG, "Updating abs_clear_delay_ms: %lu -> %lu", this->abs_clear_delay_ms_, ms);
  this->abs_clear_delay_ms_ = ms;
}

void BedPresenceEngine::update_d_min_cm(float value) {
  ESP_LOGI(TAG, "Updating d_min_cm: %.1f -> %.1f", this->d_min_cm_, value);
  this->d_min_cm_ = value;
}

void BedPresenceEngine::update_d_max_cm(float value) {
  ESP_LOGI(TAG, "Updating d_max_cm: %.1f -> %.1f", this->d_max_cm_, value);
  this->d_max_cm_ = value;
}

void BedPresenceEngine::start_baseline_calibration(uint32_t duration_s) {
  if (duration_s == 0) {
    ESP_LOGW(TAG, "Ignoring calibration request with 0s duration");
    return;
  }

  uint32_t clamped = std::min<uint32_t>(duration_s, 600);  // Hard cap at 10 minutes
  this->calibrating_ = true;
  this->calibration_samples_.clear();
  size_t expected_samples = static_cast<size_t>(clamped) * 50;  // assume up to 50 frames/sec
  this->calibration_samples_.reserve(std::min(expected_samples, MAX_CALIBRATION_SAMPLES));
  this->calibration_end_time_ = millis() + clamped * 1000UL;

  ESP_LOGI(TAG, "Starting baseline calibration for %us (collecting samples within distance window)", clamped);
  this->publish_reason("Calibration started");
  this->publish_change_reason("calibration:started");
}

void BedPresenceEngine::stop_baseline_calibration() {
  if (!this->calibrating_) {
    ESP_LOGW(TAG, "Calibration stop requested, but no calibration in progress");
    return;
  }
  this->finalize_calibration();
}

void BedPresenceEngine::reset_to_defaults() {
  ESP_LOGI(TAG, "Resetting engine parameters to known-good defaults");
  this->mu_still_ = 6.7f;
  this->sigma_still_ = 3.5f;
  this->k_on_ = 9.0f;
  this->k_off_ = 4.0f;
  this->on_debounce_ms_ = 3000;
  this->off_debounce_ms_ = 5000;
  this->abs_clear_delay_ms_ = 30000;
  this->d_min_cm_ = 0.0f;
  this->d_max_cm_ = 600.0f;

  this->calibrating_ = false;
  this->calibration_samples_.clear();

  this->current_state_ = IDLE;
  this->publish_state(false);
  this->publish_reason("Reset to defaults");
  this->publish_change_reason("off:reset_to_defaults");
}

void BedPresenceEngine::handle_calibration_sample(float energy) {
  if (!this->calibrating_) {
    return;
  }

  if (this->calibration_samples_.size() >= MAX_CALIBRATION_SAMPLES) {
    ESP_LOGW(TAG, "Calibration sample buffer full (%u samples), finalizing early",
             static_cast<unsigned>(this->calibration_samples_.size()));
    this->finalize_calibration();
    return;
  }

  this->calibration_samples_.push_back(energy);

  if (millis() >= this->calibration_end_time_) {
    this->finalize_calibration();
  }
}

static float compute_median(std::vector<float> values) {
  if (values.empty()) {
    return 0.0f;
  }

  size_t mid = values.size() / 2;
  std::nth_element(values.begin(), values.begin() + mid, values.end());
  float median = values[mid];

  if (values.size() % 2 == 0) {
    std::nth_element(values.begin(), values.begin() + mid - 1, values.end());
    median = (median + values[mid - 1]) / 2.0f;
  }
  return median;
}

void BedPresenceEngine::finalize_calibration() {
  if (!this->calibrating_) {
    return;
  }

  this->calibrating_ = false;

  if (this->calibration_samples_.empty()) {
    ESP_LOGW(TAG, "Calibration finished with no samples collected");
    this->publish_reason("Calibration failed: no samples");
    this->publish_change_reason("calibration:insufficient_samples");
    return;
  }

  auto samples = this->calibration_samples_;
  this->calibration_samples_.clear();

  float median = compute_median(samples);
  std::vector<float> deviations(samples.size());
  for (size_t i = 0; i < samples.size(); ++i) {
    deviations[i] = std::fabs(samples[i] - median);
  }
  float mad = compute_median(deviations);
  float sigma = mad * 1.4826f;
  if (sigma < 0.05f) {
    sigma = 0.05f;
  }

  this->mu_still_ = median;
  this->sigma_still_ = sigma;

  ESP_LOGI(TAG, "Calibration complete: mu=%.2f, sigma=%.2f (samples=%u)", median, sigma,
           static_cast<unsigned>(samples.size()));

  char summary[96];
  snprintf(summary, sizeof(summary), "Calibration complete: μ=%.2f, σ=%.2f, n=%u", median, sigma,
           static_cast<unsigned>(samples.size()));
  this->publish_reason(summary);
  this->publish_change_reason("calibration:completed");
}


}  // namespace bed_presence_engine
}  // namespace esphome
