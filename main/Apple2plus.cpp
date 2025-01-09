//
// Created by Philippe Hilger on 2025-01-06.
//

#include "Apple2plus.h"
#include <cstring>
#include <cstdio>

const int NUM_BLOCKS = 2;

static Block ram = {0x0000, 0xc000, RW};
static Block rom = {0xc000, 0x4000, RO};
static Block blocks[NUM_BLOCKS] = {ram, rom};

extern const uint8_t apple2plus_rom_start[] asm(
    "_binary_apple2plus_rom_start");
extern const uint8_t apple2plus_rom_end[] asm(
    "_binary_apple2plus_rom_end");

void initEmulation() {
    Apple2plus *apple2 = new Apple2plus(new Memory(65536, blocks, 2));
    apple2->power(true);
}

Apple2plus::Apple2plus(Memory *memory): CPU6502(memory) {
    printf("+---------------------+\n"
           "| Apple ][+ Emulation |\n"
           "+---------------------+\n");
    load(0xd000, apple2plus_rom_start,
         apple2plus_rom_end - apple2plus_rom_start);
}



