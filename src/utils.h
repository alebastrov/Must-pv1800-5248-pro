#pragma once
#include "esphome.h"
#include <string>
#include <cstdlib>

#ifdef USE_ARDUINO
#include <Arduino.h>
#endif

// ==========================================
// VERSION FORMATTING LOGIC
// ==========================================
inline std::string format_version(int value) {
    int major = value / 10000;
    int minor = (value / 100) % 100;
    int patch = value % 100;

    std::string minor_str = std::to_string(minor);
    if (minor < 10) minor_str = "0" + minor_str;

    std::string patch_str = std::to_string(patch);
    if (patch < 10) patch_str = "0" + patch_str;

    return std::to_string(major) + "." + minor_str + "." + patch_str;
}

// ==========================================
// SEPARATED LED BLINK CONTROLLER
// ==========================================
class LedBlinkController : public esphome::Component {
 private:
    esphome::output::FloatOutput *pwm_out_;
    uint32_t on_time_ = 0;
    uint32_t off_time_ = 0;
    bool is_blinking_ = false;
    std::string active_id_ = "";

 public:
    LedBlinkController(esphome::output::FloatOutput *pwm_out) : pwm_out_(pwm_out) {}

    void set_pattern(const std::string& pattern_name, uint32_t on_ms, uint32_t off_ms) {
        this->active_id_ = pattern_name;
        this->on_time_ = on_ms;
        this->off_time_ = off_ms;
        this->is_blinking_ = true;
        this->execute_cycle(pattern_name);
    }

    void stop() {
        this->is_blinking_ = false;
        this->active_id_ = "";
        this->cancel_timeout("pwm_blink");
        if (this->pwm_out_) this->pwm_out_->set_level(0.0f); // Turn off PWM
    }

 private:
    void execute_cycle(std::string pattern_id) {
        // If pattern changed or blinking stopped, terminate this specific loop branch
        if (!this->is_blinking_ || this->active_id_ != pattern_id) return;

        // Turn LED ON (1.0f means 100% duty cycle full brightness)
        if (this->pwm_out_) this->pwm_out_->set_level(1.0f);

        // Schedule to drop low after ON duration completes
        this->set_timeout("pwm_blink", this->on_time_, [this, pattern_id]() {
            if (!this->is_blinking_ || this->active_id_ != pattern_id) return;

            // Turn LED OFF (0.0f means 0% duty cycle)
            if (this->pwm_out_) this->pwm_out_->set_level(0.0f);

            // Schedule next repeat cycle after OFF duration completes
            this->set_timeout("pwm_blink", this->off_time_, [this, pattern_id]() {
                this->execute_cycle(pattern_id);
            });
        });
    }
};

// Global accessor reference variable
inline LedBlinkController* my_led = nullptr;

inline void set_led_under_protection() { if(my_led) my_led->set_pattern("prot", 100, 900); }
inline void set_led_limiting_current() { if(my_led) my_led->set_pattern("lim", 350, 150); }
inline void set_led_no_protection()    { if(my_led) my_led->set_pattern("none", 100, 1900); }
inline void stop_led_blinking()        { if(my_led) my_led->stop(); }