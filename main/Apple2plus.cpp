//
// Created by Philippe Hilger on 2025-01-06.
//

#include "Apple2plus.h"
#include <cstring>
#include <cstdio>

#include "display.h"

const int NUM_BLOCKS = 2;

static Block ram = {0x0000, 0xc000, RW};
static Block rom = {0xc000, 0x4000, RO};
static Block blocks[NUM_BLOCKS] = {ram, rom};

static const char *TAG = "Apple2";

extern const uint8_t apple2plus_rom_start[] asm("_binary_apple2plus_rom_start");
extern const uint8_t apple2plus_rom_end[] asm("_binary_apple2plus_rom_end");

static uint8_t ucParameterToPass;
TaskHandle_t xEmulationHandle = NULL;
TaskHandle_t xVideoHandle = NULL;
volatile static bool ready = false;

static Apple2plus *apple2;

void emulate(void *pvParameters) {
  apple2 = new Apple2plus(new Memory(65536, blocks, 2));
  apple2->power(true);
  apple2->power(false);
  vTaskDelete(xEmulationHandle);
}

void video(void *pvParameters) {
  int cnt = 0;
  bool on = false;
  uint8_t ch = 0;
  for (;; ch++) {
    // for (int i = 0; i < 0x400; i++ ){
    //   *(apple2->text1() + i) = ch;
    // }
    drawText(apple2->text1(), 40, 24, on);
    vTaskDelay(10);
    cnt++;
    if (cnt%10 == 0) {
      on = !on;
      // ESP_LOGI(TAG, "on=%d", on);
    }
  }
  vTaskDelete(xEmulationHandle);
}

void initEmulation() {
  xTaskCreate(emulate, "Emulator", 8192, &ucParameterToPass, tskIDLE_PRIORITY,
              &xEmulationHandle);
  configASSERT(xEmulationHandle);

  xTaskCreate(video, "Video", 8192, &ucParameterToPass, tskIDLE_PRIORITY,
              &xVideoHandle);
  configASSERT(xVideoHandle);

  // for (;;) {
  //   vTaskDelay(1000);
  // }
  // Use the handle to delete the task.
  // if (xEmulationHandle != NULL) {
  //   vTaskDelete(xEmulationHandle);
  // }
}

Apple2plus::Apple2plus(Memory *memory) : CPU6502(memory, 5000000) {
  ESP_LOGD(TAG,
           "+---------------------+\n"
           "| Apple ][+ Emulation |\n"
           "+---------------------+\n");
  load(0xd000, apple2plus_rom_start, apple2plus_rom_end - apple2plus_rom_start);
  ready = true;
}

bool Apple2plus::isRunning() { return running; }

uint8_t *Apple2plus::text1() { return memory->mem + 0x400; }