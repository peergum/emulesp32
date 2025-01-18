//
// Created by Philippe Hilger on 2025-01-06.
//

#ifndef EMULESP32_APPLE2PLUS_H
#define EMULESP32_APPLE2PLUS_H

#include "CPU6502.h"
#include "Memory.h"

#ifdef __cplusplus
extern "C" {
#endif

    void initEmulation();

#ifdef __cplusplus
}

class Apple2plus: public CPU6502 {
public:
    Apple2plus(Memory *memory);
    bool isRunning();
    uint8_t *text1();
};

#endif

#endif //EMULESP32_APPLE2PLUS_H
