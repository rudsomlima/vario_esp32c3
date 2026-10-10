#pragma once
#include <stdint.h>
#include <stddef.h>
class TwoWire {
public:
  bool begin(int, int, uint32_t) { return true; }
  void beginTransmission(uint8_t) {}
  size_t write(uint8_t) { return 1; }
  uint8_t endTransmission(bool = true) { return 0; }
  uint8_t requestFrom(uint8_t, uint8_t n) { return n; }
  int read() { return 0; }
};
static TwoWire Wire;
