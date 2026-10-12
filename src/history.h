#pragma once
#include "esphome.h"
#include <array>
#include <string>

namespace ProtectionHistory {
    constexpr size_t MAX_ENTRIES = 10;

    // Fixed-size array avoids runtime heap expansion
    inline std::array<std::string, MAX_ENTRIES> history_array;
    inline size_t head_index = 0; // Tracks the newest entry position
    inline size_t current_size = 0;
}

void add_to_history(esphome::time::RealTimeClock *time_source, esphome::template_::TemplateTextSensor *sensor, std::string event_text) {
    auto time_now = time_source->now();
    char time_buf[32]; // Fixed: syntax typo resolved from your previous layout

    if (time_now.is_valid()) {
        struct tm c_tm = time_now.to_c_tm();
        strftime(time_buf, sizeof(time_buf), "[%H:%M:%S]", &c_tm);
    } else {
        snprintf(time_buf, sizeof(time_buf), "[00:00:00]");
    }

    if (event_text.length() > 80) {
        event_text = event_text.substr(0, 77) + "...";
    }

    // Reference variables to clean up logic
    auto& storage = ProtectionHistory::history_array;
    auto& head = ProtectionHistory::head_index;
    auto& count = ProtectionHistory::current_size;

    // Advance head index circularly to point to the new slot (overwrites oldest entry)
    if (count > 0) {
        head = (head + 1) % ProtectionHistory::MAX_ENTRIES;
    }

    // Assign directly. This overwrites the old string memory slot without moving other array items!
    storage[head] = std::string(time_buf) + " " + event_text;

    if (count < ProtectionHistory::MAX_ENTRIES) {
        count++;
    }

    // Prefit full_history capacity to completely prevent mid-loop heap allocations
    std::string full_history;
    full_history.reserve(count * 95); // 80 chars max + 11 time chars + newline

    // Traverse backward from the newest (head) to the oldest to preserve chronological display
    size_t current_inspect = head;
    for (size_t i = 0; i < count; ++i) {
        if (!full_history.empty()) {
            full_history += "\n";
        }
        full_history += storage[current_inspect];

        // Wrap around backward safely
        current_inspect = (current_inspect == 0) ? (ProtectionHistory::MAX_ENTRIES - 1) : (current_inspect - 1);
    }

    sensor->publish_state(full_history);
}
