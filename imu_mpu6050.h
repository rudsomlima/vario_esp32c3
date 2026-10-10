// imu_mpu6050.h - Driver minimo MPU-6050 (GY-521).
//
// Configuracao: amostragem 50 Hz, DLPF ~21 Hz (anti-aliasing p/ 50 Hz),
// acel +-4 g, giro +-500 dps. O pino INT sobe a cada amostra nova e fica alto
// ate a proxima leitura (latch) - na fase 2 ele acorda o ESP32 do light sleep.
#pragma once
#include <Arduino.h>
#include <Wire.h>

class ImuMPU6050 {
public:
  float ax, ay, az;   // g (com correcao de escala)
  float gx, gy, gz;   // rad/s (ja sem bias)
  float accelScale = 1.0f;   // ajustado no boot para |a| = 1 g em repouso

  // GY-521: AD0 em GND -> 0x68 (padrao), AD0 em VCC -> 0x69
  bool begin(TwoWire &w, uint8_t addr = 0x68) {
    wire_ = &w; addr_ = addr;
    uint8_t who = 0;
    if (!readRegs(0x75, &who, 1)) return false;
    // original = 0x68; clones devolvem 0x70, 0x72, 0x98... aceita, so rejeita barramento morto
    if (who == 0x00 || who == 0xFF) return false;
    whoAmI = who;
    writeReg(0x6B, 0x80);   // PWR_MGMT_1: reset
    delay(100);
    writeReg(0x6B, 0x01);   // acorda, clock = PLL do giro X (mais estavel)
    delay(10);
    writeReg(0x1A, 0x04);   // CONFIG: DLPF acel 21 Hz / giro 20 Hz
    writeReg(0x19, 19);     // SMPLRT_DIV: 1 kHz / (1+19) = 50 Hz
    writeReg(0x1B, 0x08);   // GYRO_CONFIG: +-500 dps (65.5 LSB/dps)
    writeReg(0x1C, 0x08);   // ACCEL_CONFIG: +-4 g (8192 LSB/g)
    writeReg(0x37, 0x30);   // INT_PIN_CFG: ativo alto, push-pull, latch, limpa em qualquer leitura
    writeReg(0x38, 0x01);   // INT_ENABLE: DATA_RDY
    delay(50);
    return true;
  }

  // Le acel+temp+giro (14 bytes a partir de ACCEL_XOUT_H). Tambem limpa o INT.
  bool read() {
    uint8_t b[14];
    if (!readRegs(0x3B, b, 14)) return false;
    const float AS = 1.0f / 8192.0f;
    const float GS = (1.0f / 65.5f) * DEG_TO_RAD;
    ax = (int16_t)(b[0] << 8 | b[1]) * AS * accelScale;
    ay = (int16_t)(b[2] << 8 | b[3]) * AS * accelScale;
    az = (int16_t)(b[4] << 8 | b[5]) * AS * accelScale;
    gx = (int16_t)(b[8] << 8 | b[9]) * GS - bgx_;
    gy = (int16_t)(b[10] << 8 | b[11]) * GS - bgy_;
    gz = (int16_t)(b[12] << 8 | b[13]) * GS - bgz_;
    return true;
  }

  // Bias do giro + escala do acelerometro, com o aparelho PARADO.
  // Retorna false se detectou movimento (nesse caso nada e alterado).
  bool calibrate(int n = 100) {
    float sx = 0, sy = 0, sz = 0, s2 = 0, sa = 0;
    float obx = bgx_, oby = bgy_, obz = bgz_, osc = accelScale;
    bgx_ = bgy_ = bgz_ = 0; accelScale = 1.0f;
    for (int i = 0; i < n; i++) {
      delay(20);
      read();
      sx += gx; sy += gy; sz += gz;
      s2 += gx * gx + gy * gy + gz * gz;
      sa += sqrtf(ax * ax + ay * ay + az * az);
    }
    float mx = sx / n, my = sy / n, mz = sz / n;
    float var = s2 / n - (mx * mx + my * my + mz * mz);
    if (var > 0.0005f) {               // ~1.3 deg/s rms -> mexeu
      bgx_ = obx; bgy_ = oby; bgz_ = obz; accelScale = osc;
      return false;
    }
    bgx_ = mx; bgy_ = my; bgz_ = mz;
    accelScale = 1.0f / (sa / n);
    return true;
  }

  void sleep() { if (wire_) writeReg(0x6B, 0x40); }   // ~5 uA (ignora se nao iniciado)

  uint8_t whoAmI = 0;

private:
  void writeReg(uint8_t r, uint8_t v) {
    wire_->beginTransmission(addr_);
    wire_->write(r); wire_->write(v);
    wire_->endTransmission();
  }
  bool readRegs(uint8_t r, uint8_t *buf, uint8_t n) {
    wire_->beginTransmission(addr_);
    wire_->write(r);
    if (wire_->endTransmission(false) != 0) return false;
    if (wire_->requestFrom(addr_, n) != n) return false;
    for (uint8_t i = 0; i < n; i++) buf[i] = wire_->read();
    return true;
  }
  TwoWire *wire_ = nullptr;
  uint8_t addr_ = 0x68;
  float bgx_ = 0, bgy_ = 0, bgz_ = 0;
};
