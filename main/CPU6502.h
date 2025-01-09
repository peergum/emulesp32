//
// Created by Philippe Hilger on 2025-01-05.
//

#ifndef EMULESP32_CPU6502_H
#define EMULESP32_CPU6502_H

#include "CPU.h"
#include "Memory.h"
#include <stdint.h>

#ifdef __cplusplus

class CPU6502 : public CPU  {
public:
    CPU6502(Memory *memory): CPU("6502", memory, 1000000) {}
    void reset();
    void emulate();
    void halt();
    uint8_t nextByte();
    uint8_t getByte(uint16_t address);
    uint16_t nextWord();
    uint16_t getWord(uint16_t address);
    void branch(const char *inst, uint8_t cond, bool set);
    void pushWord(uint16_t word);
    uint16_t pullWord();
    void bit(uint8_t flag, bool conf);

   private:
    int8_t a, x, y = 0;
    uint8_t sp = 0x00;
    uint8_t sr = 0;
    uint16_t pc = 0xFFFD;
    const char *cmd;
    char arg[32];
};

#endif

#endif //EMULESP32_CPU6502_H
