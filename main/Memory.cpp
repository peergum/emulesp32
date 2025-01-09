//
// Created by Philippe Hilger on 2025-01-06.
//

#include <malloc.h>
#include "Memory.h"
#include <cstdio>
#include <cstring>
#include "esp_log.h"

static const char *TAG = "Memory";

Memory::Memory(size_t mapSize, Block *blocks, const int numBlocks): blocks(blocks), numBlocks(numBlocks), mapSize(mapSize) {
  this->mem = (uint8_t *)malloc(mapSize);
  memset(this->mem, 0, mapSize);

  // this->blocks = (Block *)malloc(numBlocks * sizeof(Block));
  // for (int i = 0; i < numBlocks; i++) {
  //   this->blocks[i] = *(blocks + i);
  // }
  // this->numBlocks = numBlocks;
}

void Memory::printMap() {
  ESP_LOGI(TAG, "Memory Map");
  for (int i = 0; i < numBlocks; i++) {
    ESP_LOGI(TAG, "- %04X -> %04X, type %s", blocks[i].start,
             blocks[i].start + blocks[i].len - 1,
             blocks[i].type == RO ? "ROM" : "RAM");
    }
}

void Memory::load(uint16_t start, const uint8_t *data, const uint16_t len) {
    if (start+len > mapSize) {
      ESP_LOGE(TAG, "Load Overflow.");
      return;
    }
    ESP_LOGI(TAG, "Loading %d bytes data at %04X", len, start);
    memcpy(mem + start, data, len);
}