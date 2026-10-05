#pragma once
#include "esphome.h"
#include <vector>
#include <string>

enum HttpRequestStatus {
  HTTP_REQUEST_STATUS_IDLE = 0,
  HTTP_REQUEST_STATUS_ONGOING = 1,
  HTTP_REQUEST_STATUS_ERROR = 2,
  HTTP_REQUEST_STATUS_SUCCESS = 3
};

void log_and_send_tg(const char* tag, const std::string& message, const std::string& bot_token, esphome::http_request::HttpRequestComponent *http_client) {
  ESP_LOGW(tag, "%s", message.c_str());
  //here might be a telegram bot message sender, note: bot_token is fetching from secrets.yaml
}
