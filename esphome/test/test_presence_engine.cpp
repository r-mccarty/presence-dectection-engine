/**
 * Unit Tests for Bed Presence Engine - Phase 2
 *
 * These tests document and verify Phase 2 state machine logic with debouncing.
 * Since the actual implementation requires ESPHome dependencies,
 * these tests demonstrate the expected behavior using a simplified model.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

/**
 * Simplified Phase 2 Presence Engine for Testing
 *
 * This models the core Phase 2 logic without ESPHome dependencies:
 * - Z-score calculation
 * - 4-state machine (IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF)
 * - Debounce timers with time mocking
 * - Absolute clear delay
 */
class SimplePresenceEngine {
public:
    // State machine states
    enum State { IDLE, DEBOUNCING_ON, PRESENT, DEBOUNCING_OFF };

    // Configuration (matching actual implementation defaults)
    float mu_still_ = 100.0f;
    float sigma_still_ = 20.0f;
    float k_on_ = 4.0f;
    float k_off_ = 2.0f;
    unsigned long on_debounce_ms_ = 3000;
    unsigned long off_debounce_ms_ = 5000;
    unsigned long abs_clear_delay_ms_ = 30000;
    float d_min_cm_ = 0.0f;
    float d_max_cm_ = 600.0f;

    // State
    State current_state_ = IDLE;
    bool binary_output_ = false;  // Simulates binary sensor output
    std::string last_reason_ = "";

    // Time tracking (mock time for testing)
    unsigned long mock_time_ = 0;
    unsigned long debounce_start_time_ = 0;
    unsigned long last_high_confidence_time_ = 0;
    bool calibrating_ = false;
    unsigned long calibration_end_time_ = 0;
    std::vector<float> calibration_samples_;

    // Z-score calculation: z = (x - μ) / σ
    float calculate_z_score(float energy) {
        if (sigma_still_ <= 0.001f) {
            return 0.0f;  // Prevent division by zero
        }
        return (energy - mu_still_) / sigma_still_;
    }

    // Advance mock time
    void advance_time(unsigned long ms) {
        mock_time_ += ms;
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

    void finalize_calibration() {
        calibrating_ = false;
        if (calibration_samples_.empty()) {
            return;
        }

        auto samples = calibration_samples_;
        calibration_samples_.clear();

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

        mu_still_ = median;
        sigma_still_ = sigma;
    }

    void start_calibration(uint32_t duration_s) {
        calibrating_ = true;
        calibration_samples_.clear();
        calibration_end_time_ = mock_time_ + duration_s * 1000UL;
    }

    void maybe_collect_calibration(float energy) {
        if (!calibrating_) {
            return;
        }
        calibration_samples_.push_back(energy);
        if (mock_time_ >= calibration_end_time_) {
            finalize_calibration();
        }
    }

    // Process energy reading (Phase 3 logic: distance window + calibration + state machine)
    void process_energy(float energy, bool distance_allowed = true) {
        if (calibrating_ && mock_time_ >= calibration_end_time_) {
            finalize_calibration();
        }

        if (!distance_allowed) {
            return;
        }

        float z_still = calculate_z_score(energy);
        unsigned long now = mock_time_;

        maybe_collect_calibration(energy);

        switch (current_state_) {
            case IDLE:
                if (z_still >= k_on_) {
                    debounce_start_time_ = now;
                    current_state_ = DEBOUNCING_ON;
                }
                break;

            case DEBOUNCING_ON:
                if (z_still >= k_on_) {
                    // Condition still holds, check timer
                    if ((now - debounce_start_time_) >= on_debounce_ms_) {
                        current_state_ = PRESENT;
                        last_high_confidence_time_ = now;
                        binary_output_ = true;

                        char buf[64];
                        snprintf(buf, sizeof(buf), "ON: z=%.2f, debounced %lums", z_still, on_debounce_ms_);
                        last_reason_ = buf;
                    }
                } else {
                    // Condition lost, abort debounce
                    current_state_ = IDLE;
                }
                break;

            case PRESENT:
                // Update high confidence timestamp whenever strong signal detected
                if (z_still > k_on_) {
                    last_high_confidence_time_ = now;
                }

                // Check for transition to DEBOUNCING_OFF
                if (z_still < k_off_) {
                    // Low signal detected, check absolute clear delay
                    if ((now - last_high_confidence_time_) >= abs_clear_delay_ms_) {
                        debounce_start_time_ = now;
                        current_state_ = DEBOUNCING_OFF;
                    }
                }
                break;

            case DEBOUNCING_OFF:
                if (z_still < k_off_) {
                    // Condition still holds, check timer
                    if ((now - debounce_start_time_) >= off_debounce_ms_) {
                        current_state_ = IDLE;
                        binary_output_ = false;

                        char buf[64];
                        snprintf(buf, sizeof(buf), "OFF: z=%.2f, debounced %lums", z_still, off_debounce_ms_);
                        last_reason_ = buf;
                    }
                } else if (z_still >= k_on_) {
                    // High signal returned, abort debounce
                    current_state_ = PRESENT;
                    last_high_confidence_time_ = now;
                }
                break;
        }
    }
};

class PresenceEngineTest : public ::testing::Test {
protected:
    SimplePresenceEngine engine_;
};

TEST_F(PresenceEngineTest, ZScoreCalculation) {
    // With μ=100, σ=20:
    EXPECT_FLOAT_EQ(engine_.calculate_z_score(100.0f), 0.0f);   // (100-100)/20 = 0
    EXPECT_FLOAT_EQ(engine_.calculate_z_score(120.0f), 1.0f);   // (120-100)/20 = 1
    EXPECT_FLOAT_EQ(engine_.calculate_z_score(140.0f), 2.0f);   // (140-100)/20 = 2
    EXPECT_FLOAT_EQ(engine_.calculate_z_score(180.0f), 4.0f);   // (180-100)/20 = 4
    EXPECT_FLOAT_EQ(engine_.calculate_z_score(80.0f), -1.0f);   // (80-100)/20 = -1
}

TEST_F(PresenceEngineTest, InitialStateIsIdle) {
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::IDLE);
    EXPECT_FALSE(engine_.binary_output_);
}

TEST_F(PresenceEngineTest, TransitionsToOccupiedWithDebouncing) {
    // k_on=4.0, so need z>=4
    // z=4 means energy = 100 + 4*20 = 180

    // High signal detected, should enter DEBOUNCING_ON
    engine_.process_energy(185.0f);  // z=4.25
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::DEBOUNCING_ON);
    EXPECT_FALSE(engine_.binary_output_);  // Binary sensor still OFF during debounce

    // Advance time but not enough to complete debounce
    engine_.advance_time(2000);  // 2 seconds (need 3)
    engine_.process_energy(185.0f);  // Still high
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::DEBOUNCING_ON);
    EXPECT_FALSE(engine_.binary_output_);  // Still OFF

    // Advance time to complete debounce
    engine_.advance_time(1000);  // Total 3 seconds
    engine_.process_energy(185.0f);  // Still high
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);
    EXPECT_TRUE(engine_.binary_output_);  // Now ON
}

TEST_F(PresenceEngineTest, DebouncingOnAborts) {
    // Start debouncing
    engine_.process_energy(185.0f);  // z=4.25
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::DEBOUNCING_ON);

    // Advance time partway
    engine_.advance_time(2000);

    // Signal drops below threshold before debounce completes
    engine_.process_energy(135.0f);  // z=1.75 < k_on
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::IDLE);
    EXPECT_FALSE(engine_.binary_output_);  // Should remain OFF
}

TEST_F(PresenceEngineTest, TransitionsToVacantWithDebouncing) {
    // First get to PRESENT state
    engine_.process_energy(185.0f);
    engine_.advance_time(3000);
    engine_.process_energy(185.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);

    // Wait for absolute clear delay (30 seconds default)
    engine_.advance_time(30000);

    // Now low signal detected, should enter DEBOUNCING_OFF
    engine_.process_energy(135.0f);  // z=1.75 < k_off
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::DEBOUNCING_OFF);
    EXPECT_TRUE(engine_.binary_output_);  // Still ON during debounce

    // Advance time to complete off debounce (5 seconds)
    engine_.advance_time(5000);
    engine_.process_energy(135.0f);  // Still low
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::IDLE);
    EXPECT_FALSE(engine_.binary_output_);  // Now OFF
}

TEST_F(PresenceEngineTest, DebouncingOffAborts) {
    // Get to PRESENT state
    engine_.process_energy(185.0f);
    engine_.advance_time(3000);
    engine_.process_energy(185.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);

    // Wait for absolute clear delay and enter DEBOUNCING_OFF
    engine_.advance_time(30000);
    engine_.process_energy(135.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::DEBOUNCING_OFF);

    // Advance time partway through debounce
    engine_.advance_time(3000);

    // High signal returns, should abort debounce
    engine_.process_energy(185.0f);  // z=4.25 >= k_on
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);
    EXPECT_TRUE(engine_.binary_output_);  // Should remain ON
}

TEST_F(PresenceEngineTest, AbsoluteClearDelayBlocksTransition) {
    // Get to PRESENT state
    engine_.process_energy(185.0f);
    engine_.advance_time(3000);
    engine_.process_energy(185.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);

    // Low signal detected but abs_clear_delay not yet elapsed
    engine_.advance_time(10000);  // Only 10 seconds (need 30)
    engine_.process_energy(135.0f);  // z < k_off
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);  // Should remain PRESENT
    EXPECT_TRUE(engine_.binary_output_);  // Should remain ON
}

TEST_F(PresenceEngineTest, HighConfidenceTimestampTracking) {
    // Get to PRESENT state
    engine_.process_energy(185.0f);
    engine_.advance_time(3000);
    engine_.process_energy(185.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);
    unsigned long first_hc_time = engine_.last_high_confidence_time_;

    // Advance time and provide another high signal
    engine_.advance_time(10000);
    engine_.process_energy(185.0f);  // z > k_on
    EXPECT_GT(engine_.last_high_confidence_time_, first_hc_time);  // Should update

    // Now need to wait 30 seconds from latest high confidence signal before clearing
    engine_.advance_time(29000);  // Almost 30 seconds from second signal
    engine_.process_energy(135.0f);  // Low signal
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);  // Still blocking
}

TEST_F(PresenceEngineTest, UpdateKOnDynamically) {
    engine_.k_on_ = 5.0f;  // Increase threshold

    // Now need z>=5, so energy >= 100 + 5*20 = 200
    engine_.process_energy(185.0f);  // z=4.25 < k_on
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::IDLE);

    engine_.process_energy(205.0f);  // z=5.25 >= k_on
    engine_.advance_time(3000);
    engine_.process_energy(205.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);
    EXPECT_TRUE(engine_.binary_output_);
}

TEST_F(PresenceEngineTest, UpdateKOffDynamically) {
    // Get to PRESENT state
    engine_.process_energy(185.0f);
    engine_.advance_time(3000);
    engine_.process_energy(185.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);

    // Update k_off to 3.0
    engine_.k_off_ = 3.0f;

    // Now need z<3 to enter DEBOUNCING_OFF, so energy < 100 + 3*20 = 160
    engine_.advance_time(30000);  // Wait for abs_clear_delay
    engine_.process_energy(165.0f);  // z=3.25 > k_off
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);  // Should remain PRESENT

    engine_.process_energy(155.0f);  // z=2.75 < k_off
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::DEBOUNCING_OFF);
}

TEST_F(PresenceEngineTest, StateReasonIsUpdated) {
    // Turn ON (with debouncing)
    engine_.process_energy(185.0f);
    engine_.advance_time(3000);
    engine_.process_energy(185.0f);
    EXPECT_NE(engine_.last_reason_.find("ON:"), std::string::npos);
    EXPECT_NE(engine_.last_reason_.find("z="), std::string::npos);
    EXPECT_NE(engine_.last_reason_.find("debounced"), std::string::npos);
    std::string reason_on = engine_.last_reason_;

    // Turn OFF (with debouncing)
    engine_.advance_time(30000);
    engine_.process_energy(135.0f);
    engine_.advance_time(5000);
    engine_.process_energy(135.0f);
    EXPECT_NE(engine_.last_reason_.find("OFF:"), std::string::npos);
    EXPECT_NE(engine_.last_reason_.find("z="), std::string::npos);
    EXPECT_NE(engine_.last_reason_.find("debounced"), std::string::npos);

    // Reasons should be different
    EXPECT_NE(reason_on, engine_.last_reason_);
}

TEST_F(PresenceEngineTest, HandlesZeroSigmaGracefully) {
    engine_.sigma_still_ = 0.0f;

    // Should return z=0 without crashing
    EXPECT_FLOAT_EQ(engine_.calculate_z_score(100.0f), 0.0f);
    EXPECT_FLOAT_EQ(engine_.calculate_z_score(1000.0f), 0.0f);

    // Should not change state (z=0 is between k_off and k_on)
    engine_.process_energy(1000.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::IDLE);
}

TEST_F(PresenceEngineTest, HandlesNegativeEnergyValues) {
    // Negative energy should work (could happen with sensor noise)
    engine_.process_energy(-40.0f);  // z = (-40-100)/20 = -7
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::IDLE);

    // Should still be able to turn ON with high values (with debouncing)
    engine_.process_energy(185.0f);
    engine_.advance_time(3000);
    engine_.process_energy(185.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);
}

TEST_F(PresenceEngineTest, HandlesVeryLargeEnergyValues) {
    // Very large energy should turn ON (with debouncing)
    engine_.process_energy(10000.0f);  // z = (10000-100)/20 = 495
    engine_.advance_time(3000);
    engine_.process_energy(10000.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);

    // And back OFF with low values (with debouncing)
    engine_.advance_time(30000);  // abs_clear_delay
    engine_.process_energy(0.0f);  // z = (0-100)/20 = -5
    engine_.advance_time(5000);  // off_debounce
    engine_.process_energy(0.0f);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::IDLE);
}

TEST_F(PresenceEngineTest, DistanceWindowBlocksFrames) {
    engine_.d_min_cm_ = 50.0f;
    engine_.d_max_cm_ = 200.0f;

    // High energy but frame rejected -> remains IDLE
    engine_.process_energy(185.0f, false);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::IDLE);

    // Allow frame -> should debounce as normal
    engine_.process_energy(185.0f, true);
    engine_.advance_time(3000);
    engine_.process_energy(185.0f, true);
    EXPECT_EQ(engine_.current_state_, SimplePresenceEngine::PRESENT);
}

TEST_F(PresenceEngineTest, CalibrationComputesMedianAndMad) {
    engine_.start_calibration(2);  // 2 seconds

    engine_.process_energy(120.0f);  // Sample 1
    engine_.process_energy(110.0f);  // Sample 2
    engine_.advance_time(1000);
    engine_.process_energy(130.0f);  // Sample 3
    engine_.process_energy(800.0f);  // Outlier

    // Advance time to finish calibration
    engine_.advance_time(2000);
    engine_.process_energy(100.0f);  // Trigger finalize

    // Median of [120,110,130,800] = (120+130)/2 = 125
    EXPECT_FLOAT_EQ(engine_.mu_still_, 125.0f);
    // MAD: values -> [5,15,5,675] median = (5+15)/2 = 10 -> sigma ≈ 10 * 1.4826
    EXPECT_NEAR(engine_.sigma_still_, 14.826f, 0.01f);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
