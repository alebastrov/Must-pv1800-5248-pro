#pragma once
#include "esphome.h"
#include <string>
#include <sstream>

#ifdef USE_ARDUINO
#include <Arduino.h>
#endif

inline std::string _process_version_numeric(long long value, const std::string& raw_display) {
    if (value < 0 || value > 9990000) {
        return "Invalid version number: " + raw_display;
    }

    int major = value / 10000;
    int minor = (value / 100) % 100;
    int patch = value % 100;

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d.%02d.%02d", major, minor, patch);
    return std::string(buffer);
}

// Robust text sensor handler that extracts only digits
inline std::string format_version(std::string raw_state) {
    if (raw_state.empty()) {
        return "Unknown version";
    }

    // Strip out all dots, 'v', spaces, or other punctuation characters
    std::string sanitized = "";
    for (char c : raw_state) {
        if (std::isdigit(c)) {
            sanitized += c;
        }
    }

    // If no numbers were found at all, return the original state text
    if (sanitized.empty()) {
        return "Unknown version";
    }

    long long value = 0;
    std::stringstream ss(sanitized);
    ss >> value;

    return _process_version_numeric(value, raw_state);
}

// Overload for direct integer values (like 10103)
inline std::string format_version(int value) {
    return _process_version_numeric(value, std::to_string(value));
}

// Overload for raw string literals (like "10103" or "1.01.03") passed in lambdas
inline std::string format_version(const char* raw_str) {
    return format_version(std::string(raw_str));
}

// ==========================================
// LED BLINK CONTROLLER
// ==========================================
class LedBlinkController : public esphome::Component {
 private:
    int pin_;
    uint32_t on_time_ = 0;
    uint32_t off_time_ = 0;
    uint32_t last_toggle_ = 0;
    bool led_state_ = false;
    bool is_blinking_ = false;

 public:
    LedBlinkController(int pin) : pin_(pin) {}

    void setup() override {
#ifdef USE_ARDUINO
        pinMode(pin_, OUTPUT);
        digitalWrite(pin_, LOW);
#endif
    }

    void loop() override {
        if (!is_blinking_) return;

        uint32_t now = millis();
        uint32_t target_delay = led_state_ ? on_time_ : off_time_;

        if (now - last_toggle_ >= target_delay) {
            led_state_ = !led_state_;
#ifdef USE_ARDUINO
            digitalWrite(pin_, led_state_ ? HIGH : LOW);
#endif
            last_toggle_ = now;
        }
    }

    void set_pattern(uint32_t on_ms, uint32_t off_ms) {
        on_time_ = on_ms;
        off_time_ = off_ms;
        is_blinking_ = true;
        led_state_ = true;
#ifdef USE_ARDUINO
        digitalWrite(pin_, HIGH);
#endif
        last_toggle_ = millis();
    }

    void stop() {
        is_blinking_ = false;
#ifdef USE_ARDUINO
        digitalWrite(pin_, LOW);
#endif
    }
};

// Global pointer to access the controller safely from YAML lambdas
inline LedBlinkController* my_led = nullptr;

inline void set_led_under_protection() { if(my_led) my_led->set_pattern(100, 900); }
inline void set_led_limiting_current() { if(my_led) my_led->set_pattern(350, 150); }
inline void set_led_no_protection()    { if(my_led) my_led->set_pattern(100, 1900); }
inline void stop_led_blinking()        { if(my_led) my_led->stop(); }
