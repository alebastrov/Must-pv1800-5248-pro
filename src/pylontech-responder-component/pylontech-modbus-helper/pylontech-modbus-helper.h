#include "esphome.h"

namespace pylontech {
    class PylontechResponder : public Component, public UARTDevice {
     public:
      PylontechResponder(UARTComponent *parent) : UARTDevice(parent) {}

      void setup() override {
        // Initialization code if needed
      }

      void loop() override {
        // Read bytes from the UART bus if available
        while (available()) {
          uint8_t c;
          read_byte(&c);
          buffer_.push_back(c);
          last_byte_time_ = millis();
        }

        // Process the frame if the bus is silent (inter-frame timeout ~5-10ms)
        if (!buffer_.empty() && (millis() - last_byte_time_ > 10)) {
          process_frame();
          buffer_.clear();
        }
      }

     private:
      std::vector<uint8_t> buffer_;
      uint32_t last_byte_time_ = 0;

      void process_frame() {
        // 1. Minimum Modbus RTU read request size is 8 bytes
        if (buffer_.size() < 8) {
            ESP_LOGE("custom_modbus", "Inverter requested less than 8 bytes: %d", buffer_.size());
            return;
        }
        ESP_LOGI("custom_modbus", "< Received frame of length %d", buffer_.size());
        std::string rx_hex = "";
        for (uint8_t byte : buffer_) {
          char buf[4];
          sprintf(buf, "%02X ", byte);
          rx_hex += buf;
        }
        ESP_LOGI("custom_modbus", "< RX bytes: %s", rx_hex.c_str());
        // 2. Verify the frame belongs to address 0x00
        if (buffer_[0] != 0x00) {
          ESP_LOGI("custom_modbus", "...Ignored: Frame not addressed to 0x00 (Target was: 0x%02X)", buffer_.empty() ? 0xFF : buffer_[0]);
          return;
        }

        // 3. Match Function Code 0x03 (Read Holding Registers)
        if (buffer_[1] == 0x03) {
            ESP_LOGI("custom_modbus", "Matched Function Code 0x03 on Address 0x00. Generating response...");
            // Extract the requested Starting Register Address (Combine High and Low bytes)
            uint16_t start_register = (buffer_[2] << 8) | buffer_[3];
            // Extract how many registers the inverter wants to read
            uint16_t num_registers = (buffer_[4] << 8) | buffer_[5];
            ESP_LOGI("custom_modbus", "Inverter requested Address: 0x%04X, Count: %d", start_register, num_registers);
            // Define your base/starting block address (Change 0x1000 if your inverter looks elsewhere)
            const uint16_t BASE_REG = 0x1000;
            const uint16_t TOTAL_MAPPED_REGS = 6;

            // Ensure the request falls within our handled virtual memory range
            if (start_register < BASE_REG || (start_register + num_registers) > (BASE_REG + TOTAL_MAPPED_REGS)) {
              ESP_LOGE("custom_modbus", "Requested registers out of handled range (0x%04X - 0x%04X)", start_register, start_register + num_registers);
              return;
            }

            // 4. Set up your live values (e.g., hardcoded placeholders or pulled from variables)
            uint16_t reg_voltage     = 5260; // 52.6V (scaled x100) or 52600 (scaled x1000)
            int16_t  reg_current     = 57;   // 5.7A (scaled x10)
            int16_t  reg_temperature = 335;  // 33.5 °C (scaled x10)
            uint16_t reg_error       = 0;    // 0 = Normal / No Faults
            uint16_t reg_soc         = 85;   // 85% State of Charge
            uint16_t reg_soh         = 99;   // 100% State of Health

            // Map these variables sequentially into an array
            uint16_t bms_registers[TOTAL_MAPPED_REGS] = {
              reg_voltage,
              (uint16_t)reg_current,
              (uint16_t)reg_temperature,
              reg_error,
              reg_soc,
              reg_soh
            };

            // 5. Build response frame header
            std::vector<uint8_t> response = {
              0x00,                         // Slave Address
              0x03,                         // Function Code
              (uint8_t)(num_registers * 2)  // Byte count (2 bytes per register)
            };

            // 6. Slice out and append exactly the registers the inverter requested
            uint16_t internal_offset = start_register - BASE_REG;
            for (uint16_t i = 0; i < num_registers; i++) {
              uint16_t reg_value = bms_registers[internal_offset + i];
              response.push_back((reg_value >> 8) & 0xFF); // High Byte
              response.push_back(reg_value & 0xFF);        // Low Byte
            }

            // 7. Calculate and append Modbus CRC
            uint16_t crc = crc16(response.data(), response.size());
            response.push_back(crc & 0xFF);
            response.push_back((crc >> 8) & 0xFF);

            // Log out response packet for validation
            std::string tx_hex = "";
            for (uint8_t byte : response) {
              char buf[4];
              sprintf(buf, "%02X ", byte);
              tx_hex += buf;
            }
            ESP_LOGI("custom_modbus", "< Responding with Hex bytes: %s", tx_hex.c_str());

            // 8. Send response packet back to inverter
            write_array(response.data(), response.size());
            flush();
        }
      }

      // Standard Modbus / RTU CRC16 calculation helper
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
}