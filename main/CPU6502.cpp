//
// Created by Philippe Hilger on 2025-01-05.
//

#include "CPU6502.h"
#include <cstdio>
#include <cstring>

#define gen_arg(arg, fmt, ...) \
  {}

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
  running = true;
}

void CPU6502::emulate() {
  memory->printMap();
  ESP_LOGD(TAG, "Emulation started.\n");

  ESP_LOGI(TAG, "Start timer, auto-reload at alarm event");

  gptimer_alarm_config_t alarm_config = {
      1,  // alarm count: period = 1µs
      0,     // reload_count
      true,
  };

  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
  ESP_ERROR_CHECK(gptimer_start(gptimer));
  int counter = 100000000;
  uint16_t spc = 0;
  uint8_t val, srC;
  uint16_t ind;

  int cnt = 0;
  while (running) {
    // xQueueReceive(sTimerQueue, &sQueueElement, pdMS_TO_TICKS(1));
    // vTaskDelay(0);
    cnt++;
    if (cnt % 100000 == 0) {
      ESP_LOGI(TAG, "cnt=%d", cnt);
      // vTaskDelay(0);
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
        gen_arg(arg, "%02X", nextByte());
        break;
      case 0x10:  // BPL
        branch("BPL", N, false);
        break;
      case 0x20:  // JSR abs
        pushWord(pc + 2);
        pc = nextWord();
        cmd = "JSR";
        gen_arg(arg, "$%04X", pc);
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
        gen_arg(arg, "#$%02X", y);
        bit(N, y & 0x80);
        bit(Z, y == 0);
        break;
      case 0xB0:  // BCS
        branch("BCS", C, true);
        break;
      case 0xC0:  // CPY #
        cmd = "CPY";
        val = nextByte();
        gen_arg(arg, "#$%02X", val);
        bit(N, y < val);
        bit(Z, y == val);
        break;
      case 0xD0:  // BNE
        branch("BNE", Z, false);
        break;
      case 0xE0:  // CPX #
        cmd = "CPX";
        val = nextByte();
        gen_arg(arg, "#$%02X", val);
        bit(N, x < val);
        bit(Z, x == val);
        break;
      case 0xF0:  // BEQ
        branch("BEQ", Z, true);
        break;

        // -1
      case 0x01:  // ORA X,ind
        cmd = "ORA";
        ind = nextByte();
        val = getByte(getWord(ind + x));
        a |= val;
        gen_arg(arg, "($%02X, X)", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x11:  // ORA ind,Y
        cmd = "ORA";
        ind = nextByte();
        val = getByte(getWord(ind) + y);
        a |= val;
        gen_arg(arg, "($%02X), Y", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x21:  // AND X,ind
        cmd = "AND";
        ind = nextByte();
        val = getByte(getWord(ind + x));
        a &= val;
        gen_arg(arg, "($%02X, X)", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x31:  // ORA ind,Y
        cmd = "AND";
        ind = nextByte();
        val = getByte(getWord(ind) + y);
        a &= val;
        gen_arg(arg, "($%02X), Y", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x41:  // EOR X,ind
        cmd = "EOR";
        ind = nextByte();
        val = getByte(getWord(ind + x));
        a ^= val;
        gen_arg(arg, "($%02X, X)", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x51:  // EOR ind,Y
        cmd = "EOR";
        ind = nextByte();
        val = getByte(getWord(ind) + y);
        a ^= val;
        gen_arg(arg, "($%02X), Y", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x61:  // ADC X,ind
        cmd = "ADC";
        ind = nextByte();
        val = getByte(getWord(ind + x));
        gen_arg(arg, "($%02X, X)", ind);
        adc(val);
        break;
      case 0x71:  // ADC ind,Y
        cmd = "ADC";
        ind = nextByte();
        val = getByte(getWord(ind) + y);
        gen_arg(arg, "($%02X), Y", ind);
        adc(val);
        break;
      case 0x81:  // STA X,ind
        cmd = "STA";
        ind = nextByte();
        setByte(getWord(ind + x), a);
        gen_arg(arg, "($%02X, X)", ind);
        break;
      case 0x91:  // STA ind,Y
        cmd = "STA";
        ind = nextByte();
        setByte(getWord(ind) + y, a);
        gen_arg(arg, "($%02X), Y", ind);
        break;
      case 0xA1:  // LDA X,ind
        cmd = "LDA";
        ind = nextByte();
        val = getByte(ind + x);
        a = val;
        gen_arg(arg, "($%02X, X)", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0xB1:  // LDA ind,Y
        cmd = "LDA";
        ind = nextByte();
        val = getByte(ind) + y;
        a = val;
        gen_arg(arg, "($%02X), Y", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0xC1:  // CMP X,ind
        cmd = "CMP";
        ind = nextByte();
        val = getByte(ind + x);
        gen_arg(arg, "($%02X, X)", ind);
        bit(N, a < val);
        bit(Z, a == val);
        break;
      case 0xD1:  // CMP ind,Y
        cmd = "CMP";
        ind = nextByte();
        val = getByte(ind) + y;
        gen_arg(arg, "($%02X), Y", ind);
        bit(N, a < val);
        bit(Z, a == val);
        break;
      case 0xE1:  // SBC X,ind
        cmd = "SBC";
        ind = nextByte();
        val = getByte(ind + x);
        gen_arg(arg, "($%02X, X)", ind);
        sbc(val);
        break;
      case 0xF1:  // SBC ind,Y
        cmd = "SBC";
        ind = nextByte();
        val = getByte(ind) + y;
        gen_arg(arg, "($%02X), Y", ind);
        sbc(val);
        break;

        // -2

      case 0xA2:
        cmd = "LDX";
        x = nextByte();
        gen_arg(arg, "#$%02X", x);
        bit(N, x & 0x80);
        bit(Z, x == 0);
        break;

        // -3

        // -4

      case 0x24:  // BIT zpg
        cmd = "BIT";
        ind = nextByte();
        val = getByte(ind);
        gen_arg(arg, "$%02X", ind);
        bit(N, val & N);
        bit(V, val & V);
        bit(Z, (a & val) == 0);
        break;

      case 0x84:  // STY zpg
        cmd = "STY";
        ind = nextByte();
        setByte(ind, y);
        gen_arg(arg, "$%02X", ind);
        break;

      case 0x94:  // STY zpg,X
        cmd = "STY";
        ind = nextByte();
        setByte(ind + x, y);
        gen_arg(arg, "$%02X, X", ind);
        break;

      case 0xA4:  // LDY zpg
        cmd = "LDY";
        ind = nextByte();
        y = getByte(ind);
        gen_arg(arg, "$%02X", ind);
        bit(N, y & 0x80);
        bit(Z, y == 0);
        break;

      case 0xB4:  // LDY zpg,X
        cmd = "LDY";
        ind = nextByte();
        y = getByte(ind + x);
        gen_arg(arg, "$%02X, X", ind);
        bit(N, y & 0x80);
        bit(Z, y == 0);
        break;

      case 0xC4:  // CPY zpg
        cmd = "CPY";
        ind = nextByte();
        val = getByte(ind);
        gen_arg(arg, "$%02X", ind);
        bit(N, y < val);
        bit(Z, y == val);
        break;

      case 0xE4:  // CPX zpg
        cmd = "CPX";
        ind = nextByte();
        val = getByte(ind);
        gen_arg(arg, "$%02X", ind);
        bit(N, x < val);
        bit(Z, x == val);
        break;

        // -5

      case 0x05:  // ORA zpg
        cmd = "ORA";
        ind = nextByte();
        val = getByte(ind);
        a |= val;
        gen_arg(arg, "$%02X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x15:  // ORA zpg,X
        cmd = "ORA";
        ind = nextByte();
        val = getByte(ind + x);
        a |= val;
        gen_arg(arg, "$%02X, X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x25:  // AND zpg
        cmd = "AND";
        ind = nextByte();
        val = getByte(ind);
        a &= val;
        gen_arg(arg, "$%02X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x35:  // AND zpg,X
        cmd = "AND";
        ind = nextByte();
        val = getByte(ind + x);
        a &= val;
        gen_arg(arg, "$%02X, X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x45:  // EOR zpg
        cmd = "EOR";
        ind = nextByte();
        val = getByte(ind);
        a ^= val;
        gen_arg(arg, "$%02X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x55:  // EOR zpg,X
        cmd = "EOR";
        ind = nextByte();
        val = getByte(ind + x);
        a ^= val;
        gen_arg(arg, "$%02X, X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x65:  // ADC zpg
        cmd = "ADC";
        ind = nextByte();
        val = getByte(ind);
        gen_arg(arg, "$%02X", ind);
        adc(val);
        break;
      case 0x75:  // ADC zpg,X
        cmd = "ADC";
        ind = nextByte();
        val = getByte(ind + x);
        gen_arg(arg, "$%02X, X", ind);
        adc(val);
        break;
      case 0x85:  // STA zpg
        cmd = "STA";
        ind = nextByte();
        setByte(ind, a);
        gen_arg(arg, "$%02X", ind);
        break;
      case 0x95:  // STA zpg,X
        cmd = "STA";
        ind = nextByte();
        setByte(ind + x, a);
        gen_arg(arg, "$%02X, X", ind);
        break;
      case 0xA5:  // LDA zpg
        cmd = "LDA";
        ind = nextByte();
        val = getByte(ind);
        a = val;
        gen_arg(arg, "$%02X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0xB5:  // LDA zpg,X
        cmd = "LDA";
        ind = nextByte();
        val = getByte(ind + x);
        a = val;
        gen_arg(arg, "$%02X, X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0xC5:  // CMP zpg
        cmd = "CMP";
        ind = nextByte();
        val = getByte(ind);
        gen_arg(arg, "$%02X", ind);
        bit(N, a < val);
        bit(Z, a == val);
        break;
      case 0xD5:  // CMP zpg,X
        cmd = "CMP";
        ind = nextByte();
        val = getByte(ind + x);
        gen_arg(arg, "$%02X, X", ind);
        bit(N, a < val);
        bit(Z, a == val);
        break;
      case 0xE5:  // SBC zpg
        cmd = "SBC";
        ind = nextByte();
        val = getByte(ind);
        gen_arg(arg, "$%02X", ind);
        sbc(val);
        break;
      case 0xF5:  // SBC zpg,X
        cmd = "SBC";
        ind = nextByte();
        val = getByte(ind + x);
        gen_arg(arg, "$%02X, X", ind);
        sbc(val);
        break;

        // -6

      case 0x06:  // ASL zpg
        cmd = "ASL";
        ind = nextByte();
        val = getByte(ind);
        bit(C, val & 0x80);
        val <<= 1;
        setByte(ind, val);
        gen_arg(arg, "$%02X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x16:  // ASL zpg,X
        cmd = "ASL";
        ind = nextByte();
        val = getByte(ind + x);
        bit(C, val & 0x80);
        val <<= 1;
        setByte(ind + x, val);
        gen_arg(arg, "$%02X, X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x26:  // ROL zpg
        cmd = "ROL";
        ind = nextByte();
        val = getByte(ind);
        srC = (sr & C) ? 1 : 0;
        bit(C, val & 0x80);
        val = (val << 1) | srC;
        gen_arg(arg, "$%02X", ind);
        setByte(ind, val);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x36:  // ROL zpg,X
        cmd = "ROL";
        ind = nextByte();
        val = getByte(ind + x);
        srC = (sr & C) ? 1 : 0;
        bit(C, val & 0x80);
        val = (val << 1) | srC;
        gen_arg(arg, "$%02X, X", ind);
        setByte(ind + x, val);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x46:  // LSR zpg
        cmd = "LSR";
        ind = nextByte();
        val = getByte(ind);
        bit(C, val & 0x01);
        val >>= 1;
        setByte(ind, val);
        gen_arg(arg, "$%02X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x56:  // LSR zpg,X
        cmd = "LSR";
        ind = nextByte();
        val = getByte(ind + x);
        bit(C, val & 0x01);
        val >>= 1;
        setByte(ind + x, val);
        gen_arg(arg, "$%02X, X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x66:  // ROR zpg
        cmd = "ROR";
        ind = nextByte();
        val = getByte(ind);
        srC = (sr & C) ? 0x80 : 0;
        bit(C, val & 0x01);
        val = (val >> 1) | srC;
        setByte(ind, val);
        gen_arg(arg, "$%02X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x76:  // ROR zpg,X
        cmd = "ROR";
        ind = nextByte();
        val = getByte(ind + x);
        srC = (sr & C) ? 0x80 : 0;
        bit(C, val & 0x01);
        val = (val >> 1) | srC;
        setByte(ind + x, val);
        gen_arg(arg, "$%02X, X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x86:  // STX zpg
        cmd = "STX";
        ind = nextByte();
        setByte(ind, x);
        gen_arg(arg, "$%02X", ind);
        break;
      case 0x96:  // STX zpg,Y
        cmd = "STX";
        ind = nextByte();
        setByte(ind + y, x);
        gen_arg(arg, "$%02X, Y", ind);
        break;
      case 0xA6:  // LDX zpg
        cmd = "LDX";
        ind = nextByte();
        x = getByte(ind);
        gen_arg(arg, "$%02X", ind);
        bit(N, x & 0x80);
        bit(Z, x == 0);
        break;
      case 0xB6:  // LDX zpg,Y
        cmd = "LDX";
        ind = nextByte();
        x = getByte(ind + y);
        gen_arg(arg, "$%02X, Y", ind);
        bit(N, x & 0x80);
        bit(Z, x == 0);
        break;
      case 0xC6:  // DEC zpg
        cmd = "DEC";
        ind = nextByte();
        val = getByte(ind);
        val--;
        gen_arg(arg, "$%02X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0xD6:  // DEC zpg,X
        cmd = "DEC";
        ind = nextByte();
        val = getByte(ind + x);
        val--;
        gen_arg(arg, "$%02X, X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0xE6:  // INC zpg
        cmd = "INC";
        ind = nextByte();
        val = getByte(ind);
        val++;
        gen_arg(arg, "$%02X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0xF6:  // INC zpg,X
        cmd = "INC";
        ind = nextByte();
        val = getByte(ind + x);
        val++;
        gen_arg(arg, "$%02X, X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;

        // -7

        // nothing here

        // -8

      case 0x08:  // PHP impl
        val = sr | 0x30;
        pushByte(val);
        cmd = "PHP";
        break;
      case 0x18:  // CLC impl
        sr &= C;
        cmd = "CLC";
        break;
      case 0x28:  // PLP impl
        sr = pullByte() & 0xcf;
        cmd = "PLP";
        break;
      case 0x38:  // SEC impl
        sr |= C;
        cmd = "SEC";
        break;
      case 0x48:  // PHA impl
        pushByte(a);
        cmd = "PHA";
        break;
      case 0x58:  // CLI impl
        sr &= I;
        cmd = "CLI";
        break;
      case 0x68:  // PLA impl
        a = pullByte();
        cmd = "PLA";
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x78:  // SEI impl
        sr |= I;
        cmd = "SEI";
        break;
      case 0x88:  // DEY impl
        y--;
        cmd = "DEY";
        bit(N, y & 0x80);
        bit(Z, y == 0);
        break;
      case 0x98:  // TYA impl
        a = y;
        cmd = "TYA";
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0xa8:  // TAY impl
        y = a;
        cmd = "TAY";
        bit(N, y & 0x80);
        bit(Z, y == 0);
        break;
      case 0xb8:  // CLV impl
        sr &= V;
        cmd = "CLV";
        break;
      case 0xc8:  // INY impl
        y++;
        cmd = "INY";
        bit(N, y & 0x80);
        bit(Z, y == 0);
        break;
      case 0xd8:  // CLD impl
        sr &= D;
        cmd = "CLD";
        break;
      case 0xe8:  // INX impl
        x++;
        cmd = "INX";
        bit(N, x & 0x80);
        bit(Z, x == 0);
        break;
      case 0xf8:  // SED impl
        sr |= D;
        cmd = "SED";
        break;

        // -9

      case 0x09:  // ORA #
        cmd = "ORA";
        a |= nextByte();
        gen_arg(arg, "#$%02X, X", a);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x19:  // ORA abs,Y
        cmd = "ORA";
        ind = nextWord();
        val = getByte(ind + y);
        a |= val;
        gen_arg(arg, "$%04X, Y", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x29:  // AND #
        cmd = "AND";
        val = nextByte();
        a &= val;
        gen_arg(arg, "#$%02X", val);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x39:  // AND abs,Y
        cmd = "AND";
        ind = nextWord();
        val = getByte(ind + y);
        a &= val;
        gen_arg(arg, "$%04X, Y", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x49:  // EOR #
        cmd = "EOR";
        val = nextByte();
        a ^= val;
        gen_arg(arg, "#$%02X", val);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x59:  // EOR abs,Y
        cmd = "EOR";
        ind = nextWord();
        val = getByte(ind + y);
        a ^= val;
        gen_arg(arg, "$%02X, Y", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x69:  // ADC #
        cmd = "ADC";
        val = nextByte();
        gen_arg(arg, "#$%02X", val);
        adc(val);
        break;
      case 0x79:  // ADC abs,Y
        cmd = "ADC";
        ind = nextWord();
        val = getByte(ind + y);
        gen_arg(arg, "$%02X, Y", ind);
        adc(val);
        break;
      // case 0x89 is illegal
      case 0x99:  // STA abs,Y
        cmd = "STA";
        ind = nextWord();
        setByte(ind + y, a);
        gen_arg(arg, "$%02X, Y", ind);
        break;
      case 0xA9:  // LDA #
        cmd = "LDA";
        val = nextByte();
        a = val;
        gen_arg(arg, "#$%02X", val);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0xB9:  // LDA abs,Y
        cmd = "LDA";
        ind = nextWord();
        val = getByte(ind + y);
        a = val;
        gen_arg(arg, "$%02X, Y", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0xC9:  // CMP #
        cmd = "CMP";
        val = nextByte();
        gen_arg(arg, "#$%02X", val);
        bit(N, a < val);
        bit(Z, a == val);
        break;
      case 0xD9:  // CMP abs,Y
        cmd = "CMP";
        ind = nextWord();
        val = getByte(ind + y);
        gen_arg(arg, "$%02X, Y", ind);
        bit(N, a < val);
        bit(Z, a == val);
        break;
      case 0xE9:  // SBC #
        cmd = "SBC";
        val = nextByte();
        gen_arg(arg, "#$%02X", val);
        sbc(val);
        break;
      case 0xF9:  // SBC abs,Y
        cmd = "SBC";
        ind = nextWord();
        val = getByte(ind + y);
        gen_arg(arg, "$%02X, y", ind);
        sbc(val);
        break;

      // -A
      case 0x0A:  // ASL A
        cmd = "ASL";
        strcpy(arg, "A");
        bit(C, a & 0x80);
        a <<= 1;
        bit(N, a & 0x80);
        bit(Z, y == 0);
        break;

      case 0x2A:  // ROL A
        cmd = "ROL";
        strcpy(arg, "A");
        srC = (sr & C) ? 1 : 0;
        bit(C, a & 0x80);
        a = (a << 1) | srC;
        bit(N, a & 0x80);
        bit(Z, y == 0);
        break;

      case 0x4A:  // LSR A
        cmd = "LSR";
        strcpy(arg, "A");
        bit(C, a & 0x01);
        a >>= 1;
        bit(N, a & 0x80);
        bit(Z, y == 0);
        break;

      case 0x6A:  // ROR A
        cmd = "ROR";
        strcpy(arg, "A");
        srC = (sr & C) ? 0x80 : 0;
        bit(C, a & 0x01);
        a = (a >> 1) | srC;
        bit(N, a & 0x80);
        bit(Z, y == 0);
        break;

      case 0x8A:  // TXA impl
        cmd = "TXA";
        a = x;
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;

      case 0x9A:  // TXS impl
        cmd = "TXS";
        sp = x;
        break;

      case 0xAA:  // TAX impl
        cmd = "TAX";
        x = a;
        bit(N, x & 0x80);
        bit(Z, x == 0);
        break;

      case 0xBA:  // TSX impl
        cmd = "TSX";
        x = sp;
        bit(N, x & 0x80);
        bit(Z, x == 0);
        break;

      case 0xCA:  // DEX impl
        cmd = "DEX";
        x--;
        bit(N, x & 0x80);
        bit(Z, x == 0);
        break;

      case 0XEA:  // NOP impl

        break;
        // -B

        // -C

      case 0x2C:  // BIT abs
        cmd = "BIT";
        ind = nextWord();
        val = getByte(ind);
        gen_arg(arg, "$%04X", ind);
        bit(N, val & N);
        bit(V, val & V);
        bit(Z, (a & val) == 0);
        break;

      case 0x4C:  // JMP abs
        pc = nextWord();
        cmd = "JMP";
        gen_arg(arg, "$%04X", pc);
        break;

      case 0x6C:  // jmp ind
        ind = nextWord();
        pc = getWord(ind);
        cmd = "JMP";
        gen_arg(arg, "($%04X)", ind);
        break;

      case 0x8C:  // STY abs
        cmd = "STY";
        ind = nextWord();
        setByte(ind, y);
        gen_arg(arg, "$%04X", ind);
        break;

      case 0xAC:  // LDY abs
        cmd = "LDY";
        ind = nextWord();
        y = getByte(ind);
        gen_arg(arg, "$%04X", ind);
        bit(N, y & 0x80);
        bit(Z, y == 0);
        break;

      case 0xBC:  // LDY abs,X
        cmd = "LDY";
        ind = nextByte();
        y = getByte(ind + x);
        gen_arg(arg, "$%02X, X", ind);
        bit(N, y & 0x80);
        bit(Z, y == 0);
        break;

      case 0xCC:  // CPY abs
        cmd = "CPY";
        ind = nextByte();
        val = getByte(ind);
        gen_arg(arg, "$%02X", ind);
        bit(N, y < val);
        bit(Z, y == val);
        break;

      case 0xEC:  // CPX abs
        cmd = "CPX";
        ind = nextByte();
        val = getByte(ind);
        gen_arg(arg, "$%02X", ind);
        bit(N, x < val);
        bit(Z, x == val);
        break;

        // -D

      case 0x0D:  // ORA abs
        cmd = "ORA";
        ind = nextWord();
        val = getByte(ind);
        a |= val;
        gen_arg(arg, "$%04X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x1D:  // ORA abs,X
        cmd = "ORA";
        ind = nextWord();
        val = getByte(ind + x);
        a |= val;
        gen_arg(arg, "$%04X, X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x2D:  // AND abs
        cmd = "AND";
        ind = nextWord();
        val = getByte(ind);
        a &= val;
        gen_arg(arg, "$%04X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x3D:  // AND abs,X
        cmd = "AND";
        ind = nextWord();
        val = getByte(ind + x);
        a &= val;
        gen_arg(arg, "$%04X, X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x4D:  // EOR abs
        cmd = "EOR";
        ind = nextWord();
        val = getByte(ind);
        a ^= val;
        gen_arg(arg, "$%04X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x5D:  // EOR abs,X
        cmd = "EOR";
        ind = nextWord();
        val = getByte(ind + x);
        a ^= val;
        gen_arg(arg, "$%04X, X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0x6D:  // ADC abs
        cmd = "ADC";
        ind = nextWord();
        val = getByte(ind);
        gen_arg(arg, "$%04X", ind);
        adc(val);
        break;
      case 0x7D:  // ADC abs,X
        cmd = "ADC";
        ind = nextWord();
        val = getByte(ind + x);
        gen_arg(arg, "$%04X, X", ind);
        adc(val);
        break;
      case 0x8D:  // STA abs
        cmd = "STA";
        ind = nextWord();
        setByte(ind, a);
        gen_arg(arg, "$%04X", ind);
        break;
      case 0x9D:  // STA abs,X
        cmd = "STA";
        ind = nextWord();
        setByte(ind + x, a);
        gen_arg(arg, "$%04X, X", ind);
        break;
      case 0xAD:  // LDA abs
        cmd = "LDA";
        ind = nextWord();
        val = getByte(ind);
        a = val;
        gen_arg(arg, "$%04X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0xBD:  // LDA abs,X
        cmd = "LDA";
        ind = nextWord();
        val = getByte(ind + x);
        a = val;
        gen_arg(arg, "$%04X, X", ind);
        bit(N, a & 0x80);
        bit(Z, a == 0);
        break;
      case 0xCD:  // CMP abs
        cmd = "CMP";
        ind = nextWord();
        val = getByte(ind);
        gen_arg(arg, "$%04X", ind);
        bit(N, a < val);
        bit(Z, a == val);
        break;
      case 0xDD:  // CMP abs,X
        cmd = "CMP";
        ind = nextWord();
        val = getByte(ind + x);
        gen_arg(arg, "$%04X, X", ind);
        bit(N, a < val);
        bit(Z, a == val);
        break;
      case 0xED:  // SBC abs
        cmd = "SBC";
        ind = nextWord();
        val = getByte(ind);
        gen_arg(arg, "$%04X", ind);
        sbc(val);
        break;
      case 0xFD:  // SBC abs,X
        cmd = "SBC";
        ind = nextWord();
        val = getByte(ind + x);
        gen_arg(arg, "$%04X, X", ind);
        sbc(val);
        break;

        // -E

      case 0x0E:  // ASL abs
        cmd = "ASL";
        ind = nextWord();
        val = getByte(ind);
        bit(C, val & 0x80);
        val <<= 1;
        setByte(ind, val);
        gen_arg(arg, "$%04X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x1E:  // ASL abs,X
        cmd = "ASL";
        ind = nextWord();
        val = getByte(ind + x);
        bit(C, val & 0x80);
        val <<= 1;
        setByte(ind + x, val);
        gen_arg(arg, "$%04X, X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x2E:  // ROL abs
        cmd = "ROL";
        ind = nextWord();
        val = getByte(ind);
        srC = (sr & C) ? 1 : 0;
        bit(C, val & 0x80);
        val = (val << 1) | srC;
        gen_arg(arg, "$%04X", ind);
        setByte(ind, val);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x3E:  // ROL abs,X
        cmd = "ROL";
        ind = nextWord();
        val = getByte(ind + x);
        srC = (sr & C) ? 1 : 0;
        bit(C, val & 0x80);
        val = (val << 1) | srC;
        gen_arg(arg, "$%04X, X", ind);
        setByte(ind + x, val);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x4E:  // LSR abs
        cmd = "LSR";
        ind = nextWord();
        val = getByte(ind);
        bit(C, val & 0x01);
        val >>= 1;
        setByte(ind, val);
        gen_arg(arg, "$%04X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x5E:  // LSR abs,X
        cmd = "LSR";
        ind = nextWord();
        val = getByte(ind + x);
        bit(C, val & 0x01);
        val >>= 1;
        setByte(ind + x, val);
        gen_arg(arg, "$%04X, X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x6E:  // ROR abs
        cmd = "ROR";
        ind = nextWord();
        val = getByte(ind);
        srC = (sr & C) ? 0x80 : 0;
        bit(C, val & 0x01);
        val = (val >> 1) | srC;
        setByte(ind, val);
        gen_arg(arg, "$%04X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x7E:  // ROR abs,X
        cmd = "ROR";
        ind = nextWord();
        val = getByte(ind + x);
        srC = (sr & C) ? 0x80 : 0;
        bit(C, val & 0x01);
        val = (val >> 1) | srC;
        setByte(ind + x, val);
        gen_arg(arg, "$%04X, X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0x8E:  // STX abs
        cmd = "STX";
        ind = nextWord();
        setByte(ind, x);
        gen_arg(arg, "$%04X", ind);
        break;
      // case 0x9E: // illegal
      case 0xAE:  // LDX abs
        cmd = "LDX";
        ind = nextWord();
        x = getByte(ind);
        gen_arg(arg, "$%04X", ind);
        bit(N, x & 0x80);
        bit(Z, x == 0);
        break;
      case 0xBE:  // LDX abs,Y
        cmd = "LDX";
        ind = nextWord();
        x = getByte(ind + y);
        gen_arg(arg, "$%04X, Y", ind);
        bit(N, x & 0x80);
        bit(Z, x == 0);
        break;
      case 0xCE:  // DEC abs
        cmd = "DEC";
        ind = nextWord();
        val = getByte(ind);
        val--;
        gen_arg(arg, "$%04X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0xDE:  // DEC abs,X
        cmd = "DEC";
        ind = nextWord();
        val = getByte(ind + x);
        val--;
        gen_arg(arg, "$%04X, X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0xEE:  // INC abs
        cmd = "INC";
        ind = nextWord();
        val = getByte(ind);
        val++;
        gen_arg(arg, "$%04X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;
      case 0xFE:  // INC abs,X
        cmd = "INC";
        ind = nextWord();
        val = getByte(ind + x);
        val++;
        gen_arg(arg, "$%04X, X", ind);
        bit(N, val & 0x80);
        bit(Z, val == 0);
        break;

        // -F

      default:  // illegal
        cmd = "???";
        break;
    }
    // ESP_LOGD(
    //     TAG,
    //     "%04X: %s %-20s [a=%02x x=%02x, y=%02x, sr=%02x, sp=%02x, pc=%04x]",
    //     spc, cmd, arg, uint8_t(a), x, y, sr, sp, pc);
    counter--;
    if (counter <= 0) {
      running = false;
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

uint8_t CPU6502::carry() { return sr & C ? 1 : 0; }

void CPU6502::branch(const char* inst, uint8_t cond, bool set) {
  int16_t rel;
  rel = int16_t(int8_t(nextByte()));
  // ESP_LOGD(TAG, "%02x", rel);
  cmd = inst;
  if (((sr & cond) != 0) == set) {
    gen_arg(arg, "%d -> jump", rel);
    pc = uint16_t(int16_t(pc) + rel);
  } else {
    gen_arg(arg, "%d -> skip", rel);
  }
}

void CPU6502::halt() {
  running = false;
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

void CPU6502::setByte(uint16_t address, uint8_t byte) {
  memory->mem[address] = byte;
}

uint16_t CPU6502::getWord(uint16_t address) {
  return ((uint16_t(memory->mem[address])) +
          (uint16_t(memory->mem[address + 1]) << 8)) &
         0xffff;
}

void CPU6502::pushByte(uint8_t byte) { memory->mem[sp++] = byte; }

uint8_t CPU6502::pullByte() {
  sp--;
  uint8_t byte = memory->mem[sp];
  return byte;
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

void CPU6502::adc(uint8_t val) {
  // TODO: BCD addition
  uint16_t res;
  res = a + val + carry();
  bit(C, res >= 0x100);
  res &= 0x7f;
  bit(V, (a < 0x80 && val < 0x80 && res >= 0x80) ||
             (a >= 0x80 && val >= 0x80 && res < 0x80));
  a = res;
  bit(N, a & 0x80);
  bit(Z, a == 0);
}

void CPU6502::sbc(uint8_t val) {
  // TODO: BCD subtraction
  uint16_t res;
  res = a - val - carry();
  bit(C, res >= 0x100);
  res &= 0x7f;
  bit(V, (a < 0x80 && val < 0x80 && res >= 0x80) ||
             (a >= 0x80 && val >= 0x80 && res < 0x80));
  a = res;
  bit(N, a & 0x80);
  bit(Z, a == 0);
}