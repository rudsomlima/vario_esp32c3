#pragma once
#include "Arduino.h"
struct IPAddress { String toString() const { return String("192.168.4.1"); } };
enum { WIFI_OFF = 0, WIFI_AP = 2 };
enum wifi_power_t { WIFI_POWER_8_5dBm = 34 };
struct WiFiClass {
  void macAddress(uint8_t *m) { for (int i = 0; i < 6; i++) m[i] = i; }
  bool mode(int) { return true; }
  bool softAP(const char *, const char *) { return true; }
  bool setTxPower(wifi_power_t) { return true; }
  IPAddress softAPIP() { return {}; }
  bool softAPdisconnect(bool) { return true; }
};
static WiFiClass WiFi;
