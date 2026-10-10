#pragma once
#include <stdint.h>
#include <string>
#define NIMBLE_CPP_VERSION_MAJOR 2
namespace NIMBLE_PROPERTY { enum : uint32_t { READ = 2, WRITE_NR = 4, WRITE = 8, NOTIFY = 16 }; }
struct NimBLEConnInfo { uint16_t getConnHandle() const { return 0; } };
static std::string g_bleOut; static int g_bleNotifies = 0; static uint8_t g_bleConnected = 0; static uint16_t g_bleMtu = 23;
struct NimBLECharacteristic { void setValue(const uint8_t* d, size_t n) { g_bleOut.append((const char*)d, n); } bool notify() { g_bleNotifies++; return true; } };
struct NimBLEService { NimBLECharacteristic* createCharacteristic(const char*, uint32_t) { static NimBLECharacteristic c; return &c; } bool start() { return true; } };
struct NimBLEAdvertising { bool addServiceUUID(const char*) { return true; } bool setName(const std::string&) { return true; }
  void setMinInterval(uint16_t) {} void setMaxInterval(uint16_t) {} bool start(uint32_t = 0) { return true; } };
struct NimBLEServer { void advertiseOnDisconnect(bool) {} NimBLEService* createService(const char*) { static NimBLEService s; return &s; }
  NimBLEAdvertising* getAdvertising() const { static NimBLEAdvertising a; return &a; } uint8_t getConnectedCount() const { return g_bleConnected; }
  NimBLEConnInfo getPeerInfo(uint8_t) const { return {}; } uint16_t getPeerMTU(uint16_t) const { return g_bleMtu; }
  void updateConnParams(uint16_t, uint16_t, uint16_t, uint16_t, uint16_t) const {} };
struct NimBLEDevice { static bool init(const std::string&) { return true; } static bool deinit(bool = false) { return true; }
  static bool setPower(int8_t) { return true; } static bool setMTU(uint16_t) { return true; }
  static NimBLEServer* createServer() { static NimBLEServer s; return &s; } };
