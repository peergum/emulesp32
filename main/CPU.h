//
// Created by Philippe Hilger on 2025-01-05.
//

#ifndef EMULESP32_CPU_H
#define EMULESP32_CPU_H

#include "Memory.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gptimer.h"
#include "esp_log.h"

#define TIMER_DIVIDER (16)  //  Hardware timer clock divider
#define TIMER_SCALE \
  (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

typedef struct {
  uint64_t microtime;
} queue_element_t;

#ifdef __cplusplus

class CPU {
 public:
  CPU(const char *processor, Memory *memory, uint32_t frequency);

  virtual void reset() = 0;
  virtual void emulate() = 0;

  void load(uint16_t start, const uint8_t *data, const uint16_t len) {
    memory->load(start, data, len);
  }

  void boot() {
    reset();
    running = true;
    emulate();
  }

  void power(bool on) {
    if (on) {
      printf("Powering on.\n");
      boot();
    } else {
      printf("Powering off.\n");
      halt();
      running = false;
    }
  }

  virtual void halt() = 0;

 protected:
  char *processor;
  Memory *memory;
  bool running = false;
  uint32_t frequency;
  QueueHandle_t sTimerQueue;
  queue_element_t sQueueElement;
  gptimer_handle_t gptimer;
};

#endif

#endif  // EMULESP32_CPU_H
