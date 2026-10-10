// vario_audio.h - Logica do som do vario (independente de hardware).
//
// Subida  >= LIFT_ON   : bipes; frequencia e cadencia aumentam com a taxa.
// Descida <= SINK_ON   : tom continuo grave (alarme de afundamento).
// Entre os dois        : silencio.
// Histerese nos limiares para nao "gaguejar" perto do zero.
//
// tick() devolve a frequencia desejada (Hz) ou 0 para silencio.
#pragma once
#include <stdint.h>

struct VarioAudioCfg {
  float liftOn = 0.10f, liftOff = 0.05f;    // m/s
  float sinkOn = -2.5f, sinkOff = -2.3f;    // m/s
  float fBase = 700.0f, fPerMs = 120.0f;    // Hz, Hz por m/s (subida)
  float fMax = 2200.0f;
  float sinkBase = 450.0f, sinkPerMs = 40.0f, sinkMin = 200.0f;
  uint16_t periodZero = 600;                // ms de ciclo perto de 0 m/s
  uint16_t periodMin = 130;                 // ms de ciclo em subida forte
  float dutyOn = 0.5f;                      // fracao do ciclo com som
};

class VarioAudio {
public:
  VarioAudioCfg cfg;

  uint16_t tick(float climb, uint32_t nowMs) {
    // histerese
    if (lift_) { if (climb < cfg.liftOff) lift_ = false; }
    else if (climb >= cfg.liftOn) { lift_ = true; cycleStart_ = nowMs; period_ = 0; }
    if (sink_) { if (climb > cfg.sinkOff) sink_ = false; }
    else if (climb <= cfg.sinkOn) sink_ = true;

    if (sink_) {
      float f = cfg.sinkBase + cfg.sinkPerMs * climb;   // climb < 0
      if (f < cfg.sinkMin) f = cfg.sinkMin;
      return (uint16_t)f;
    }
    if (!lift_) return 0;

    // novo ciclo: trava o periodo pela taxa atual (soa mais natural)
    if (period_ == 0 || nowMs - cycleStart_ >= period_) {
      cycleStart_ = nowMs;
      float p = cfg.periodZero / (1.0f + 0.6f * climb);
      if (p < cfg.periodMin) p = cfg.periodMin;
      period_ = (uint16_t)p;
    }
    if (nowMs - cycleStart_ > (uint32_t)(period_ * cfg.dutyOn)) return 0;

    // frequencia acompanha a taxa durante o bipe
    float f = cfg.fBase + cfg.fPerMs * climb;
    if (f > cfg.fMax) f = cfg.fMax;
    return (uint16_t)f;
  }

private:
  bool lift_ = false, sink_ = false;
  uint32_t cycleStart_ = 0;
  uint16_t period_ = 0;
};
