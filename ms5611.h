// ms5611.h - Driver MS5611 nao-bloqueante (I2C).
//
// Uso: startPressure()/startTemperature() inicia a conversao (OSR 4096 = 9.04 ms
// max, ~1.4 mA DURANTE a conversao, depois standby < 0.15 uA). Depois de >= 10 ms
// chame readResult(). O MCU dorme durante a conversao - nada de delay() ativo.
//
// A compensacao e feita em double: o codigo inteiro do datasheet nao tem
// resolucao suficiente para vario (observacao tambem feita no driver do
// har-in-air).
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

class MS5611 {
public:
  enum Conv : uint8_t { NONE, PRESSURE, TEMPERATURE };

  // GY-63: CSB em GND -> 0x77 (padrao). CSB em VCC -> 0x76.
  bool begin(TwoWire &w, uint8_t addr = 0x77) {
    wire_ = &w; addr_ = addr;
    if (!cmd(0x1E)) return false;      // reset
    delay(3);                          // 2.8 ms de reload da PROM
    for (int i = 0; i < 8; i++) {
      wire_->beginTransmission(addr_);
      wire_->write(0xA0 + 2 * i);
      if (wire_->endTransmission() != 0) return false;
      if (wire_->requestFrom(addr_, (uint8_t)2) != 2) return false;
      prom_[i] = ((uint16_t)wire_->read() << 8) | wire_->read();
    }
    return crcOk();
  }

  bool startPressure()    { pending_ = PRESSURE;    return cmd(0x48); } // D1 OSR4096
  bool startTemperature() { pending_ = TEMPERATURE; return cmd(0x58); } // D2 OSR4096
  Conv pending() const { return pending_; }

  // Le o ADC. Retorna true quando uma nova PRESSAO compensada esta disponivel.
  bool readResult() {
    if (pending_ == NONE) return false;
    Conv c = pending_;
    pending_ = NONE;
    if (!cmd(0x00)) return false;
    if (wire_->requestFrom(addr_, (uint8_t)3) != 3) return false;
    uint32_t raw = ((uint32_t)wire_->read() << 16) | ((uint32_t)wire_->read() << 8) | wire_->read();
    if (raw == 0) return false;        // leitura antes da conversao terminar
    if (c == TEMPERATURE) { d2_ = raw; return false; }
    d1_ = raw;
    if (d2_ == 0) return false;        // precisa de ao menos uma temperatura
    compensate(prom_, d1_, d2_, pressurePa_, tempC_);
    return true;
  }

  float pressurePa() const { return pressurePa_; }
  float temperatureC() const { return tempC_; }

  // Formula do datasheet com compensacao de 2a ordem. Publica para teste.
  static void compensate(const uint16_t C[8], uint32_t D1, uint32_t D2,
                         float &pPa, float &tC) {
    double dT = (double)D2 - (double)C[5] * 256.0;
    double TEMP = 2000.0 + dT * (double)C[6] / 8388608.0;
    double OFF = (double)C[2] * 65536.0 + (double)C[4] * dT / 128.0;
    double SENS = (double)C[1] * 32768.0 + (double)C[3] * dT / 256.0;
    if (TEMP < 2000.0) {
      double T2 = dT * dT / 2147483648.0;
      double t = TEMP - 2000.0;
      double OFF2 = 5.0 * t * t / 2.0;
      double SENS2 = 5.0 * t * t / 4.0;
      if (TEMP < -1500.0) {
        double u = TEMP + 1500.0;
        OFF2 += 7.0 * u * u;
        SENS2 += 11.0 * u * u / 2.0;
      }
      TEMP -= T2; OFF -= OFF2; SENS -= SENS2;
    }
    pPa = (float)(((double)D1 * SENS / 2097152.0 - OFF) / 32768.0); // 0.01 mbar = 1 Pa
    tC = (float)(TEMP / 100.0);
  }

  // Altitude padrao ISA (m). Para vario so importa a variacao.
  static float altitudeM(float pPa, float p0 = 101325.0f) {
    return 44330.77f * (1.0f - powf(pPa / p0, 0.190263f));
  }

private:
  bool cmd(uint8_t c) {
    wire_->beginTransmission(addr_);
    wire_->write(c);
    return wire_->endTransmission() == 0;
  }

  // CRC-4 da PROM (AN520)
  bool crcOk() {
    uint16_t p[8];
    for (int i = 0; i < 8; i++) p[i] = prom_[i];
    uint16_t crcRead = p[7] & 0x000F;
    p[7] &= 0xFF00;
    uint16_t rem = 0;
    for (int cnt = 0; cnt < 16; cnt++) {
      rem ^= (cnt & 1) ? (p[cnt >> 1] & 0x00FF) : (p[cnt >> 1] >> 8);
      for (int b = 8; b > 0; b--)
        rem = (rem & 0x8000) ? (rem << 1) ^ 0x3000 : (rem << 1);
    }
    rem = (rem >> 12) & 0x000F;
    return rem == crcRead;
  }

  TwoWire *wire_ = nullptr;
  uint8_t addr_ = 0x77;
  uint16_t prom_[8] = {0};
  uint32_t d1_ = 0, d2_ = 0;
  Conv pending_ = NONE;
  float pressurePa_ = 101325.0f, tempC_ = 20.0f;
};
