#pragma once
#include "esphome.h"
#include <vector>
#include <string>

static std::vector<std::string> internal_protection_storage;

void add_to_history(esphome::time::RealTimeClock *time_source,
                    esphome::template_::TemplateTextSensor *sensor,
                    std::string event_text) {

    auto time_now = time_source->now();
    char time_buf[32]; // Fixed missing array size bound indicator bug

    if (time_now.is_valid()) {
        // Fix: Save rvalue to a local variable before using the address operator '&'
        struct tm c_tm = time_now.to_c_tm();
        strftime(time_buf, sizeof(time_buf), "[%H:%M:%S]", &c_tm);
    } else {
        snprintf(time_buf, sizeof(time_buf), "[00:00:00]");
    }

    std::string new_entry = std::string(time_buf) + " " + event_text;

    internal_protection_storage.push_back(new_entry);
    if (internal_protection_storage.size() > 10) {
        internal_protection_storage.erase(internal_protection_storage.begin());
    }

    std::string full_history = "";
    for (const auto& entry : internal_protection_storage) {
        if (!full_history.empty()) {
            full_history += "\n";
        }
        full_history += entry;
    }

    sensor->publish_state(full_history);
}
