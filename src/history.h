#pragma once
#include "esphome.h"
#include <vector>
#include <string>

static std::vector<std::string> internal_protection_storage;

void add_to_history(esphome::time::RealTimeClock *time_source,
                    esphome::template_::TemplateTextSensor *sensor,
                    std::string event_text) {

    auto time_now = time_source->now();

    // Explicit array buffer allocation (Safe sizing)
    char time_buf[32];

    if (time_now.is_valid()) {
        struct tm c_tm = time_now.to_c_tm();
        strftime(time_buf, sizeof(time_buf), "[%H:%M:%S]", &c_tm);
    } else {
        snprintf(time_buf, sizeof(time_buf), "[00:00:00]");
    }

    // Truncate message text if it's too long to prevent JSON overflow crashes
    if (event_text.length() > 80) {
        event_text = event_text.substr(0, 77) + "...";
    }

    std::string new_entry = std::string(time_buf) + " " + event_text;

    // Push newest entries to the top
    internal_protection_storage.insert(internal_protection_storage.begin(), new_entry);

    // Pop off old items from the back of the queue
    if (internal_protection_storage.size() > 10) {
        internal_protection_storage.pop_back();
    }

    // Build unified clean history string
    std::string full_history = "";
    for (const auto& entry : internal_protection_storage) {
        if (!full_history.empty()) {
            full_history += "\n";
        }
        full_history += entry;
    }

    // Update sensor state safely
    sensor->publish_state(full_history);
}
