// Stub das APIs Arduino-ESP32 usadas, so para checar sintaxe/tipos no PC
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#define PROGMEM
#define F(s) (s)
#define FPSTR(p) (p)
#define constrain(x, lo, hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))
class String {
public:
  std::string s;
  String() {}
  String(const char *c) : s(c) {}
  String(float v, unsigned int d) { char b[32]; snprintf(b, sizeof b, "%.*f", (int)d, v); s = b; }
  String &operator+=(const String &o) { s += o.s; return *this; }
  String &operator+=(const char *c) { s += c; return *this; }
  String &operator+=(char c) { s += c; return *this; }
  String &operator+=(int v) { s += std::to_string(v); return *this; }
  friend String operator+(const String &a, const String &b) { String r = a; r += b; return r; }
  friend String operator+(const String &a, const char *b) { String r = a; r += b; return r; }
  float toFloat() const { return (float)atof(s.c_str()); }
  long toInt() const { return atol(s.c_str()); }
  unsigned int length() const { return (unsigned)s.size(); }
  const char *c_str() const { return s.c_str(); }
};
struct EspClass { void restart() {} };
static EspClass ESP;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define DEG_TO_RAD 0.017453292519943295f
#define INPUT 0x01
#define OUTPUT 0x03
#define INPUT_PULLUP 0x05
#define LOW 0
#define HIGH 1
inline void delay(unsigned long) {}
inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t) { return 1; }
inline uint32_t analogReadMilliVolts(uint8_t) { return 0; }
inline bool setCpuFrequencyMhz(uint32_t) { return true; }
inline bool ledcAttachChannel(uint8_t, uint32_t, uint8_t, int8_t) { return true; }
inline bool ledcWrite(uint8_t, uint32_t) { return true; }
inline uint32_t ledcChangeFrequency(uint8_t, uint32_t, uint8_t) { return 0; }
inline bool ledcOutputInvert(uint8_t, bool) { return true; }
inline bool ledcDetach(uint8_t) { return true; }
struct HWCDC { void begin(unsigned long) {} int printf(const char*, ...) { return 0; } };
static HWCDC Serial;
void setup(); void loop();
