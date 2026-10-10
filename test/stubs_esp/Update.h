#pragma once
#include <stdint.h>
#include <stddef.h>
#define UPDATE_SIZE_UNKNOWN 0xFFFFFFFF
struct UpdateClass { bool begin(size_t) { return true; } size_t write(uint8_t *, size_t n) { return n; }
  bool end(bool) { return true; } void abort() {} bool hasError() { return false; } };
static UpdateClass Update;
