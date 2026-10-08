#pragma once
#include "esphome.h"

// format string value 10101 into 1.01.01
std::string format_version(int value) {
    if (value < 0 || value > 9990000) {
        return "Invalid version " + std::to_string(value);
    }
    int major = value / 10000;
    int minor = (value / 100) % 100;
    int patch = value % 100;

    char buffer[16];
    sprintf(buffer, "%d.%02d.%02d", major, minor, patch);
    return std::string(buffer);
}