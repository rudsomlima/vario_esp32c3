// settings.h - Configuracoes do vario salvas na NVS (flash), com padroes seguros.
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <string.h>

struct Settings {
  // som
  uint8_t volume     = 3;       // 0 mudo, 1 baixo, 2 medio, 3 alto
  float   liftOn     = 0.10f;   // m/s - comeca a bipar
  float   sinkOn     = -2.5f;   // m/s - alarme de afundamento
  float   fBase      = 700.0f;  // Hz em 0 m/s
  float   fPerMs     = 120.0f;  // Hz por m/s
  float   periodZero = 600.0f;  // ms do ciclo de bipe perto de 0 m/s
  // filtros
  float   airspeed   = 9.0f;    // m/s - compensacao centrifuga (0 = desliga)
  float   kfAcc      = 0.35f;   // m/s^2
  float   kfBaro     = 0.25f;   // m
  // energia
  float   autoOffMin = 20.0f;   // min sem atividade -> desliga (0 = nunca)
  // BLE ($LK8EX1 para XCTrack / XCSoar)
  bool    bleEnabled    = false;
  float   bleHz         = 10.0f;  // sentencas por segundo
  float   bleTimeoutMin = 5.0f;   // desliga o BLE se ninguem conectar (0 = nunca)
  // portal de configuracao (Wi-Fi)
  char    apPass[33]    = "vario1234";  // 8 a 32 caracteres (WPA2)
};

class SettingsStore {
public:
  Settings s;

  void load() {
    Preferences p;
    p.begin(NS, true);
    Settings d;
    s.volume        = p.getUChar("vol", d.volume);
    s.liftOn        = p.getFloat("liftOn", d.liftOn);
    s.sinkOn        = p.getFloat("sinkOn", d.sinkOn);
    s.fBase         = p.getFloat("fBase", d.fBase);
    s.fPerMs        = p.getFloat("fPerMs", d.fPerMs);
    s.periodZero    = p.getFloat("period0", d.periodZero);
    s.airspeed      = p.getFloat("airspeed", d.airspeed);
    s.kfAcc         = p.getFloat("kfAcc", d.kfAcc);
    s.kfBaro        = p.getFloat("kfBaro", d.kfBaro);
    s.autoOffMin    = p.getFloat("autoOff", d.autoOffMin);
    s.bleEnabled    = p.getBool("ble", d.bleEnabled);
    s.bleHz         = p.getFloat("bleHz", d.bleHz);
    s.bleTimeoutMin = p.getFloat("bleTmo", d.bleTimeoutMin);
    if (p.getString("apPass", s.apPass, sizeof(s.apPass)) == 0) strcpy(s.apPass, d.apPass);
    p.end();
    sanitize();
  }

  void save() {
    sanitize();
    Preferences p;
    p.begin(NS, false);
    p.putUChar("vol", s.volume);
    p.putFloat("liftOn", s.liftOn);
    p.putFloat("sinkOn", s.sinkOn);
    p.putFloat("fBase", s.fBase);
    p.putFloat("fPerMs", s.fPerMs);
    p.putFloat("period0", s.periodZero);
    p.putFloat("airspeed", s.airspeed);
    p.putFloat("kfAcc", s.kfAcc);
    p.putFloat("kfBaro", s.kfBaro);
    p.putFloat("autoOff", s.autoOffMin);
    p.putBool("ble", s.bleEnabled);
    p.putFloat("bleHz", s.bleHz);
    p.putFloat("bleTmo", s.bleTimeoutMin);
    p.putString("apPass", s.apPass);
    p.end();
  }

  // Salva so um campo (botao), sem regravar tudo
  void saveVolume()     { Preferences p; p.begin(NS, false); p.putUChar("vol", s.volume); p.end(); }
  void saveBleEnabled() { Preferences p; p.begin(NS, false); p.putBool("ble", s.bleEnabled); p.end(); }

  void resetDefaults() { s = Settings(); save(); }

private:
  static constexpr const char *NS = "vario";

  static float clampf(float v, float lo, float hi, float def) {
    if (isnan(v)) return def;
    return v < lo ? lo : (v > hi ? hi : v);
  }

  void sanitize() {
    Settings d;
    if (s.volume > 3) s.volume = d.volume;
    s.liftOn        = clampf(s.liftOn, -0.5f, 1.0f, d.liftOn);
    s.sinkOn        = clampf(s.sinkOn, -10.0f, -0.5f, d.sinkOn);
    s.fBase         = clampf(s.fBase, 200.0f, 2000.0f, d.fBase);
    s.fPerMs        = clampf(s.fPerMs, 10.0f, 400.0f, d.fPerMs);
    s.periodZero    = clampf(s.periodZero, 150.0f, 1500.0f, d.periodZero);
    s.airspeed      = clampf(s.airspeed, -15.0f, 15.0f, d.airspeed);
    s.kfAcc         = clampf(s.kfAcc, 0.05f, 3.0f, d.kfAcc);
    s.kfBaro        = clampf(s.kfBaro, 0.05f, 3.0f, d.kfBaro);
    s.autoOffMin    = clampf(s.autoOffMin, 0.0f, 240.0f, d.autoOffMin);
    s.bleHz         = clampf(s.bleHz, 1.0f, 10.0f, d.bleHz);
    s.bleTimeoutMin = clampf(s.bleTimeoutMin, 0.0f, 120.0f, d.bleTimeoutMin);
    s.apPass[sizeof(s.apPass) - 1] = 0;
    size_t n = strlen(s.apPass);
    if (n < 8) strcpy(s.apPass, d.apPass);   // WPA2 exige >= 8
  }
};
