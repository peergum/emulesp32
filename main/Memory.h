//
// Created by Philippe Hilger on 2025-01-06.
//

#ifndef EMULESP32_MEMORY_H
#define EMULESP32_MEMORY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus

typedef enum Type {
    RO = 0,
    RW = 1,
} Type;

typedef struct Block {
    uint16_t start;
    uint16_t len;
    Type type;
} Block;

class Memory {
public:
    Memory(size_t mapSize, Block *blocks, const int numBlocks);
    void load(uint16_t start, const uint8_t *data, const uint16_t len);
    void printMap();

public:
    Block *blocks;
    int numBlocks;
    uint8_t *mem;
    size_t mapSize;
};

#endif

#endif //EMULESP32_MEMORY_H
