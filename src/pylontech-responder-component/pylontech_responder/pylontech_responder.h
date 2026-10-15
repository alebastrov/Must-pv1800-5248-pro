#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/modbus/modbus.h"
#include "esphome/core/log.h"
#include <vector>
#include <string>

namespace esphome {
namespace pylontech {

class PylontechResponder : public Component {
 public:
  PylontechResponder() = default;

  void set_modbus_parent(modbus::Modbus *parent) { this->parent_hub_ = parent; }
  void set_uart_parent(uart::UARTComponent *parent) { this->uart_bus_ = parent; }

  void setup() override {
    ESP_LOGI("custom_modbus", "Pylontech Custom Modbus Controller Engine active.");
  }

  void loop() override {
    if (this->uart_bus_ == nullptr) return;

    // Read bytes directly from the injected hardware UART bus instance
    while (this->uart_bus_->available()) {
      uint8_t c;
      this->uart_bus_->read_byte(&c);
      buffer_.push_back(c);
      last_byte_time_ = esphome::millis();
    }

    // Process only when we hit a complete Modbus RTU request footprint (8 bytes)
    // AND the line goes silent for a short inter-frame pause
    if (buffer_.size() >= 8 && (esphome::millis() - last_byte_time_ > 15)) {
      if (buffer_[0] == 0x00) {
        process_pylontech_frame();
      }
      buffer_.clear();
    }

    // Clean loose line noise fragments to prevent buffer leaks
    if (!buffer_.empty() && (esphome::millis() - last_byte_time_ > 150)) {
      buffer_.clear();
    }
  }

 private:
  modbus::Modbus *parent_hub_{nullptr};
  uart::UARTComponent *uart_bus_{nullptr};
  std::vector<uint8_t> buffer_;
  uint32_t last_byte_time_ = 0;

  void process_pylontech_frame() {
    uint8_t function_code = buffer_[1];

    // Match Function Code 0x03 (Read Holding Registers)
    if (function_code == 0x03) {
      uint16_t start_register = (buffer_[2] << 8) | buffer_[3];
      uint16_t num_registers = (buffer_[4] << 8) | buffer_[5];

      std::string rx_hex = "";
      for (uint8_t byte : buffer_) {
        char buf[4];
        sprintf(buf, "%02X ", byte);
        rx_hex += buf;
      }
      ESP_LOGI("custom_modbus", "<< Intercepted 0x00 Modbus Read: %s", rx_hex.c_str());
      ESP_LOGI("custom_modbus", "Request Start Reg: 0x%04X, Count: %d", start_register, num_registers);

      const uint16_t BASE_REG = 0x1000;
      const uint16_t TOTAL_MAPPED_REGS = 6;

      // Handle Exception Out of Bounds Frame
      if (start_register < BASE_REG || (start_register + num_registers) > (BASE_REG + TOTAL_MAPPED_REGS)) {
        ESP_LOGE("custom_modbus", "Requested registers out of bounds: 0x%04X", start_register);
        std::vector<uint8_t> exception_frame = { 0x00, 0x83, 0x02 };
        uint16_t crc = crc16(exception_frame.data(), exception_frame.size());
        exception_frame.push_back(crc & 0xFF);
        exception_frame.push_back((crc >> 8) & 0xFF);
        this->uart_bus_->write_array(exception_frame.data(), exception_frame.size());
        this->uart_bus_->flush();
        return;
      }

      // Live Battery Metrics (Option A Map)
      uint16_t reg_voltage     = 5260; // 52.6V
      int16_t  reg_current     = 57;   // 5.7A
      uint16_t reg_soc         = 85;   // 85%
      uint16_t reg_soh         = 99;   // 99%
      int16_t  reg_temperature = 335;  // 33.5 °C
      uint16_t reg_error       = 0;

      uint16_t bms_registers[TOTAL_MAPPED_REGS] = {
        reg_voltage, (uint16_t)reg_current, reg_soc, reg_soh, (uint16_t)reg_temperature, reg_error
      };

      // Build Response array
      std::vector<uint8_t> response = {
        0x00,                         // Address
        0x03,                         // Function
        (uint8_t)(num_registers * 2)  // Data length
      };

      uint16_t internal_offset = start_register - BASE_REG;
      for (uint16_t i = 0; i < num_registers; i++) {
        uint16_t reg_value = bms_registers[internal_offset + i];
        response.push_back((reg_value >> 8) & 0xFF);
        response.push_back(reg_value & 0xFF);
      }

      // Append CRC
      uint16_t crc = crc16(response.data(), response.size());
      response.push_back(crc & 0xFF);
      response.push_back((crc >> 8) & 0xFF);

      // Send directly over the raw physical UART stream
      this->uart_bus_->write_array(response.data(), response.size());
      this->uart_bus_->flush();

      std::string tx_hex = "";
      for (uint8_t byte : response) {
        char buf[4];
        sprintf(buf, "%02X ", byte);
        tx_hex += buf;
      }
      ESP_LOGI("custom_modbus", ">> Responded to 0x00 with bytes: %s", tx_hex.c_str());
    }
  }

  uint16_t crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; pos++) {
      crc ^= (uint16_t)data[pos];
      for (int i = 8; i != 0; i--) {
        if ((crc & 0x0001) != 0) {
          crc >>= 1;
          crc ^= 0xA001;
        } else {
          crc >>= 1;
        }
      }
    }
    return crc;
  }
};

}  // namespace pylontech
}  // namespace esphome
