//
// Created by Philippe Hilger on 2025-01-05.
//

#include "CPU.h"
#include <cstring>

static const char *TAG = "CPU";

static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer,
                                        const gptimer_alarm_event_data_t *edata,
                                        void *user_data) {
  BaseType_t high_task_awoken = pdFALSE;
  QueueHandle_t queue = (QueueHandle_t)user_data;

  // Retrieve count value and send to queue
  queue_element_t element = {.microtime = edata->count_value};

  xQueueSendFromISR(queue, &element, &high_task_awoken);
  // return whether we need to yield at the end of ISR
  return (high_task_awoken == pdTRUE);
}

CPU::CPU(const char *processor, Memory *memory, uint32_t frequency) {
  this->processor = (char *)malloc(strlen(processor) + 1);
  strcpy(this->processor, processor);
  this->memory = memory;
  this->frequency = frequency;
  sTimerQueue = xQueueCreate(1000, sizeof(queue_element_t));
  if (!sTimerQueue) {
    printf("Can't start processor clock\n");
    return;
  }

  ESP_LOGI(TAG, "Create timer handle");
  gptimer = NULL;
  gptimer_config_t timer_config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_DOWN,
      .resolution_hz = frequency,  // 1MHz, 1 tick=1us
      .intr_priority = 1,
      .flags = {.intr_shared = false, .allow_pd = 0, .backup_before_sleep = 0}};
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

  gptimer_event_callbacks_t cbs = {
      .on_alarm = timer_on_alarm_cb,
  };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, sTimerQueue));

  ESP_LOGI(TAG, "Enable timer");
  ESP_ERROR_CHECK(gptimer_enable(gptimer));
}
