#pragma once
#include <stdint.h>
#include <stddef.h>
class String;
class Preferences { public:
  bool begin(const char*, bool) { return true; }
  void end() {}
  uint8_t getUChar(const char*, uint8_t d) { return d; }
  size_t putUChar(const char*, uint8_t) { return 1; }
  float getFloat(const char*, float d) { return d; }
  size_t putFloat(const char*, float) { return 4; }
  bool getBool(const char*, bool d) { return d; }
  size_t putBool(const char*, bool) { return 1; }
  size_t getString(const char*, char* buf, size_t) { if (buf) buf[0] = 0; return 0; }
  size_t putString(const char*, const char*) { return 1; }
};
