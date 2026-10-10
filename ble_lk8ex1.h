// ble_lk8ex1.h - Envia $LK8EX1 por BLE (Nordic UART Service) para XCTrack / XCSoar.
//
// Biblioteca: NimBLE-Arduino 2.x (h2zero) - instalar pelo Gerenciador de Bibliotecas.
// Mesmo servico usado pelo vario do har-in-air, validado com o XCTrack.
//
// No XCTrack: Preferencias > Sensores > Bluetooth LE > escolher "Vario-C3".
//
// $LK8EX1,pressao(Pa),altitude(99999 = use a pressao),vario(cm/s),temp(C),bateria(V),*CS
#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>

class BleLK8EX1 {
public:
  bool begin(const char *name) {
    if (active_) return true;
    if (!NimBLEDevice::init(name)) return false;
#if defined(NIMBLE_CPP_VERSION_MAJOR) && NIMBLE_CPP_VERSION_MAJOR >= 2
    NimBLEDevice::setPower(0);           // 0 dBm: o celular esta a menos de 1 m
#endif
    NimBLEDevice::setMTU(64);            // a sentenca cabe numa notificacao so
    server_ = NimBLEDevice::createServer();
    server_->advertiseOnDisconnect(true);
    NimBLEService *svc = server_->createService(NUS_SERVICE);
    tx_ = svc->createCharacteristic(NUS_TX, NIMBLE_PROPERTY::NOTIFY);
    svc->createCharacteristic(NUS_RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    svc->start();
    NimBLEAdvertising *adv = server_->getAdvertising();
    adv->addServiceUUID(NUS_SERVICE);
    adv->setName(name);
    adv->setMinInterval(320);            // 200 ms (unidades de 0.625 ms)
    adv->setMaxInterval(800);            // 500 ms: menos consumo anunciando
    adv->start();
    active_ = true;
    connParamsSet_ = false;
    startedMs_ = millis();
    everConnected_ = false;
    return true;
  }

  void end() {
    if (!active_) return;
    NimBLEDevice::deinit(true);
    active_ = false;
    server_ = nullptr; tx_ = nullptr;
  }

  bool active() const { return active_; }
  bool connected() const { return active_ && server_->getConnectedCount() > 0; }
  bool everConnected() const { return everConnected_; }
  uint32_t activeForMs() const { return active_ ? millis() - startedMs_ : 0; }

  // Chamar periodicamente; envia so se houver alguem conectado.
  void send(float pressurePa, float climbMs, float tempC, float batteryV) {
    if (!connected()) { connParamsSet_ = false; return; }
    everConnected_ = true;
    uint16_t h = server_->getPeerInfo(0).getConnHandle();
    if (!connParamsSet_) {
      // intervalo 50-100 ms (unidades de 1.25 ms), sem latencia, timeout 4 s
      server_->updateConnParams(h, 40, 80, 0, 400);
      connParamsSet_ = true;
    }

    char buf[72], bat[8];
    if (batteryV > 1.0f) snprintf(bat, sizeof(bat), "%.2f", batteryV);
    else snprintf(bat, sizeof(bat), "999");                   // 999 = nao disponivel
    int n = snprintf(buf, sizeof(buf), "$LK8EX1,%ld,99999,%d,%d,%s,*",
                     (long)lroundf(pressurePa), (int)lroundf(climbMs * 100.0f),
                     (int)lroundf(tempC), bat);
    uint8_t cs = 0;
    for (int i = 1; i < n - 1; i++) cs ^= (uint8_t)buf[i];   // entre '$' e '*'
    n += snprintf(buf + n, sizeof(buf) - n, "%02X\r\n", cs);

    // Se o MTU negociado for pequeno, quebra em pedacos (o NUS e um fluxo de bytes)
    uint16_t mtu = server_->getPeerMTU(h);
    int chunk = (mtu > 3) ? mtu - 3 : 20;
    for (int i = 0; i < n; i += chunk) {
      int len = (n - i < chunk) ? n - i : chunk;
      tx_->setValue((const uint8_t *)buf + i, len);
      tx_->notify();
    }
  }

private:
  static constexpr const char *NUS_SERVICE = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
  static constexpr const char *NUS_RX      = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
  static constexpr const char *NUS_TX      = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

  NimBLEServer *server_ = nullptr;
  NimBLECharacteristic *tx_ = nullptr;
  bool active_ = false, connParamsSet_ = false, everConnected_ = false;
  uint32_t startedMs_ = 0;
};
