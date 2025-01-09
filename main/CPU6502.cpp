//
// Created by Philippe Hilger on 2025-01-05.
//

#include "CPU6502.h"
#include <cstdio>

enum {
  N = 1 << 7,
  V = 1 << 6,
  B = 1 << 4,
  D = 1 << 3,
  I = 1 << 2,
  Z = 1 << 1,
  C = 1 << 0
};
static const char* TAG = "6502";

void CPU6502::reset() {
  a = x = y = 0;
  sr = 0;
  sp = 0x00;
  pc = getWord(0xfffc);
  ESP_LOGD(TAG, "Processor reset.\n");
  run = true;
}

void CPU6502::emulate() {
  memory->printMap();
  ESP_LOGD(TAG, "Emulation started.\n");

  ESP_LOGI(TAG, "Start timer, auto-reload at alarm event");

  gptimer_alarm_config_t alarm_config = {
      1000000,  // alarm count: period = 1µs
      0,        // reload_count
      true,
  };

  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
  ESP_ERROR_CHECK(gptimer_start(gptimer));
  int counter = 1000;
  uint16_t spc = 0;
  int val, srC;
  uint16_t ind;

  while (run) {
    while (!xQueueReceive(sTimerQueue, &sQueueElement, pdMS_TO_TICKS(100))) {
      vTaskDelay(1);
    }
    // ESP_LOGI(TAG, "Timer reloaded, count=%llu", sQueueElement.microtime);

    spc = pc;
    cmd = "???";
    arg[0] = 0;
    uint8_t inst = nextByte();
    switch (inst) {
        // -0 DONE

      case 0x00:  // BRK
        cmd = "BRK";
        sprintf(arg, "%02X", nextByte());
        break;
      case 0x10:  // BPL
        branch("BPL", N, false);
        break;
      case 0x20:  // JSR abs
        pushWord(pc + 2);
        pc = nextWord();
        cmd = "JSR";
        sprintf(arg, "$%04X", pc);
        break;
      case 0x30:  // BMI
        branch("BMI", N, true);
        break;
      case 0x40:  // RTI impl
        cmd = "RTI";
        break;
      case 0x50:  // BVC
        branch("BVC", V, false);
        break;
      case 0x60:  // RTS impl
        pc = pullWord();
        cmd = "RTS";
        break;
      case 0x70:  // BVS
        branch("BVS", V, true);
        break;
      case 0x90:  // BCC
        branch("BCC", C, false);
        break;
      case 0xA0:  // LDY #
        cmd = "LDY";
        y = nextByte();
        sprintf(arg, "#$%02X", uint8_t(y));
        bit(N, y < 0);
        bit(Z, y == 0);
        break;
      case 0xB0:  // BCS
        branch("BCS", C, true);
        break;
      case 0xC0:  // CPY #
        cmd = "CPY";
        val = int8_t(nextByte());
        sprintf(arg, "#$%02X", uint8_t(val));
        bit(N, y < val);
        bit(Z, y == val);
        break;
      case 0xD0:  // BNE
        branch("BNE", Z, false);
        break;
      case 0xE0:  // CPX #
        cmd = "CPX";
        val = int8_t(nextByte());
        sprintf(arg, "#$%02X", uint8_t(val));
        bit(N, x < val);
        bit(Z, x == val);
        break;
      case 0xF0:  // BEQ
        branch("BEQ", Z, true);
        break;

      case 0x01:  // ORA X,ind
        break;
      case 0x11:  // ORA ind,Y
        break;

        // -1

        // -2

      case 0xA2:
        cmd = "LDX";
        x = nextByte();
        sprintf(arg, "#$%02X", uint8_t(x));
        bit(N, x < 0);
        bit(Z, x == 0);
        break;

        // -3

        // -4

        // -5

      case 0x05:  // ORA zpg
        break;
      case 0x15:  // ORA zpg,X
        break;

        // -6

      case 0x06:  // ASL zpg
        break;
      case 0x16:  // ASL zpg,X
        break;

        // -7

        // -8

      case 0x08:  // PHP impl
        break;
      case 0x18:  // CLC impl
        break;

        // -9

      case 0x09:  // ORA #
        cmd = "ORA";
        val = nextByte();
        sprintf(arg, "#$%02X", uint8_t(val));
        a |= val;
        bit(N, a < 0);
        bit(Z, a == 0);
        break;

      case 0x29:  // AND #
        cmd = "AND";
        val = nextByte();
        sprintf(arg, "#$%02X", uint8_t(val));
        a &= val;
        bit(N, a < 0);
        bit(Z, a == 0);
        break;

      case 0x49:  // EOR #
        cmd = "EOR";
        val = nextByte();
        sprintf(arg, "#$%02X", uint8_t(val));
        a ^= val;
        bit(N, a < 0);
        bit(Z, a == 0);
        break;

      case 0x69:  // ADC #
        break;

      case 0xA9:  // LDA #
        cmd = "LDA";
        a = nextByte();
        sprintf(arg, "#$%02X", uint8_t(a));
        bit(N, a < 0);
        bit(Z, a == 0);
        break;

      case 0xC9:  // CMP #
        cmd = "CMP";
        val = int8_t(nextByte());
        sprintf(arg, "#$%02X", uint8_t(val));
        bit(N, a < val);
        bit(Z, a == val);
        break;

      case 0xE9:  // SBC #
        break;

      // -A
      case 0x0A:  // ASL A
        cmd = "ASL A";
        bit(C, a < 0);
        a <<= 1;
        bit(N, a < 0);
        bit(Z, y == 0);
        break;

      case 0x2A:  // ROL A
        cmd = "ROL A";
        srC = (sr & C) ? 1 : 0;
        bit(C, a < 0);
        a = (a << 1) | srC;
        bit(N, a < 0);
        bit(Z, y == 0);
        break;

      case 0x4A:  // LSR A
        cmd = "LSR A";
        bit(C, a < 0);
        a >>= 1;
        bit(N, a < 0);
        bit(Z, y == 0);
        break;

      case 0x6A:  // ROR A
        srC = (sr & C) ? 0x80 : 0;
        bit(C, a & 0x01);
        a = (a >> 1) | srC;
        bit(N, a < 0);
        bit(Z, y == 0);
        break;

        // -B

        // -C

      case 0x4C:
        pc = nextWord();
        cmd = "JMP";
        sprintf(arg, "$%04X", pc);
        break;

      case 0x6C:
        ind = nextWord();
        pc = getWord(ind);
        cmd = "JMP";
        sprintf(arg, "($%04X)", ind);
        break;

        // -D

      case 0x0D:  // ORA abs
        break;

        // -E

      case 0x0E:  // ASL abs
        break;

        // -F

      default:  // illegal
        cmd = "???";
        break;
    }
    ESP_LOGD(
        TAG,
        "%04X: %s %-20s [a=%02x x=%02x, y=%02x, sr=%02x, sp=%02x, pc=%04x]",
        spc, cmd, arg, uint8_t(a), uint8_t(x), uint8_t(y), sr, sp, pc);
    counter--;
    if (counter <= 0) {
      run = false;
    }
  }
  ESP_LOGI(TAG, "Stop timer");
  ESP_ERROR_CHECK(gptimer_stop(gptimer));

  ESP_LOGI(TAG, "Disable timer");
  ESP_ERROR_CHECK(gptimer_disable(gptimer));
}

void CPU6502::bit(uint8_t flag, bool conf) {
  sr |= conf ? flag : 0;
  sr &= conf ? 0xff : ~flag;
}

void CPU6502::branch(const char* inst, uint8_t cond, bool set) {
  int16_t rel;
  rel = int16_t(int8_t(nextByte()));
  // ESP_LOGD(TAG, "%02x", rel);
  cmd = inst;
  if (((sr & cond) != 0) == set) {
    sprintf(arg, "%d -> jump", rel);
    pc = uint16_t(int16_t(pc) + rel);
  } else {
    sprintf(arg, "%d -> skip", rel);
  }
}

void CPU6502::halt() {
  run = false;
  ESP_LOGD(TAG, "Processor halted,.\n");
}

uint8_t CPU6502::nextByte() {
  uint8_t b = getByte(pc++) & 0xff;
  return b;
}

uint16_t CPU6502::nextWord() {
  uint16_t w = getWord(pc) & 0xffff;
  pc += 2;
  return w;
}

uint8_t CPU6502::getByte(uint16_t address) { return memory->mem[address]; }

uint16_t CPU6502::getWord(uint16_t address) {
  return ((uint16_t(memory->mem[address])) +
          (uint16_t(memory->mem[address + 1]) << 8)) &
         0xffff;
}

void CPU6502::pushWord(uint16_t word) {
  memory->mem[sp++] = uint8_t(word & 0xff);
  memory->mem[sp++] = uint8_t(word >> 8);
}

uint16_t CPU6502::pullWord() {
  sp -= 2;
  uint16_t word =
      (uint16_t(memory->mem[sp])) + (uint16_t(memory->mem[sp + 1]) << 8);
  return word & 0xffff;
}
