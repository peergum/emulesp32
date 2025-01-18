#include <stdio.h>
#include "esp_log.h"
#include "CPU6502.h"

#include "Apple2plus.h"
#include "main.h"

static const char *TAG = "Main";

int app_main(void) {
  esp_log_level_set("6502", ESP_LOG_INFO);
  ESP_LOGD(TAG, "Starting");
  initEmulation();
  while (true) {
    vTaskDelay(1000);
  }
}
