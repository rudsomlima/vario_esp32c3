// vario_esp32c3.ino - Vario audio com fusao acelerometro + barometro
// FASE 4: baixo consumo (light sleep) + BLE opcional ($LK8EX1 p/ XCTrack)
//         + portal de configuracao por Wi-Fi (so sob demanda, em solo).
//
// Hardware: ESP32-C3 SuperMini + GY-521 (MPU-6050) + GY-63 (MS5611)
//           + piezo passivo entre GPIO0 e GPIO1 + botao GPIO3->GND
//           + LiPo via TP4056 (com protecao) -> diodo Schottky -> pino 5V
//             (o diodo impede o USB da placa de jogar 5 V na bateria; ver docs/montagem.md)
// Core: Arduino-ESP32 3.x  |  Placa: "ESP32C3 Dev Module", USB CDC On Boot = Enabled
// Biblioteca: NimBLE-Arduino 2.x (h2zero)
// Particao: "Minimal SPIFFS (1.9MB APP with OTA)" (Wi-Fi + BLE nao cabem na padrao)
//
// Pinagem
//   GPIO6 SDA, GPIO7 SCL -> MS5611 (0x77) + MPU-6050 (0x68), 3V3 e GND
//   GPIO5 <- INT do MPU-6050 (OBRIGATORIO: e ele que acorda a CPU)
//   GPIO3 <- botao para GND (+ pull-up externo 100k)
//   GPIO4 <- divisor da bateria: BAT+ -- 1M -- GPIO4 -- 1M -- GND (+100nF em GPIO4)
//   GPIO0/GPIO1 -> piezo
//
// Botao: 1 clique  = volume (mudo/baixo/medio/alto)
//        2 cliques = liga/desliga BLE
//        segurar 2 s = desliga | ligar = segurar ~1 s
//        ligar e CONTINUAR segurando ate ~4 s (3 bipes agudos) = portal de configuracao
//
// Ciclo (50x por segundo):
//   [light sleep] --INT do MPU--> le IMU -> Mahony -> Kalman predict
//   -> (a cada 2) le MS5611 + dispara proxima conversao -> Kalman update
//   -> som -> BLE -> botao -> [light sleep]
// Com o BLE ativo nao ha light sleep (o radio precisa do clock); por isso o
// BLE se desliga sozinho se ninguem conectar em bleTimeoutMin minutos.

#include <Arduino.h>
#include <Wire.h>
#include <esp_sleep.h>
#include "driver/gpio.h"
#include "settings.h"
#include "ms5611.h"
#include "imu_mpu6050.h"
#include "mahony_gravity.h"
#include "kalman_vario.h"
#include "vario_audio.h"
#include "piezo_idf.h"
#include "ble_lk8ex1.h"
#include "portal.h"

// ---------------- configuracao de compilacao ----------------
#define LOW_POWER            1  // 1 = light sleep entre amostras
#define PIEZO_KEEP_IN_SLEEP  1  // 1 = PWM no RC_FAST, dorme ate apitando. Se o som
                                //     falhar/engasgar, ponha 0 (fica acordado so ao apitar)
#define DEBUG_PLOT           0  // 1 = Serial Plotter. DESLIGA o sleep (o USB cai no light sleep)

const char    FW_VERSION[] = "0.4.0";
const char    BLE_NAME[] = "Vario-C3";
const uint8_t PIN_SDA = 6, PIN_SCL = 7;
const uint8_t PIN_MPU_INT = 5;
const uint8_t PIN_BUTTON = 3;
const uint8_t PIN_VBAT = 4;
const uint8_t PIN_PIEZO_A = 0, PIN_PIEZO_B = 1;

const uint8_t  BARO_EVERY_TICKS = 2;    // 25 Hz de pressao (tick = amostra do MPU, 50 Hz)
const uint8_t  TEMP_EVERY_CONV  = 25;   // 1 temperatura por segundo
const float    AUTO_OFF_CLIMB   = 0.3f; // m/s
const float    AUTO_OFF_GYRO    = 0.5f; // rad/s (soma |gx|+|gy|+|gz|)
const uint32_t CLICK_GAP_MS     = 400;  // janela para o 2o clique

// As demais configuracoes (som, filtros, auto-off, BLE) ficam em settings.h / NVS.

// --------------------------------------------
SettingsStore cfg;
MS5611        baro;
ImuMPU6050    imu;
MahonyGravity ahrs;
KalmanVario   kf;
VarioAudio    audio;
PiezoIDF      piezo;
BleLK8EX1     ble;

static uint32_t tickCount = 0, convCount = 0;
static uint32_t lastActivityMs = 0;
static uint32_t lastMicros = 0;
static uint32_t bleLastConnMs = 0;
static float    batteryV = 0, lastAVert = 0;
static bool     intOk = false;   // INT do MPU funcionando -> pode dormir

// ---------- utilidades ----------
static float readBatteryV() {
  uint32_t mv = 0;
  for (int i = 0; i < 8; i++) mv += analogReadMilliVolts(PIN_VBAT);
  return (mv / 8.0f) * 2.0f / 1000.0f;   // divisor 1M/1M
}

static int batteryBars(float v) {        // 1..4 bipes
  if (v < 1.0f) return 0;                // divisor nao ligado
  if (v > 4.0f) return 4;
  if (v > 3.85f) return 3;
  if (v > 3.7f) return 2;
  return 1;
}

static void applySettings() {
  const Settings &s = cfg.s;
  audio.cfg.liftOn     = s.liftOn;
  audio.cfg.liftOff    = s.liftOn - 0.05f;
  audio.cfg.sinkOn     = s.sinkOn;
  audio.cfg.sinkOff    = s.sinkOn + 0.2f;
  audio.cfg.fBase      = s.fBase;
  audio.cfg.fPerMs     = s.fPerMs;
  audio.cfg.periodZero = (uint16_t)s.periodZero;
  ahrs.airspeed        = s.airspeed;
}

// Sinais sonoros curtos (sempre no volume alto)
static void chirp(uint16_t f1, uint16_t f2) {
  auto v = piezo.volume();
  piezo.setVolume(PiezoIDF::VOL_HIGH);
  piezo.beep(f1, 70); delay(40); piezo.beep(f2, 90);
  piezo.setVolume(v);
}
static void chirpBleOn()  { chirp(1500, 2000); }
static void chirpBleOff() { chirp(2000, 1500); }

static void goToSleep() {
  ble.end();
  // desarma o timer/GPIO do light sleep, senao o deep sleep acorda em 50 ms
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_BUTTON, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}

static void powerOff() {
  piezo.setVolume(PiezoIDF::VOL_HIGH);
  piezo.beep(1200, 120); piezo.beep(800, 120); piezo.beep(500, 250);
  imu.sleep();                           // MS5611 ja fica em standby sozinho
  while (digitalRead(PIN_BUTTON) == LOW) delay(10);   // espera soltar
  delay(50);
  goToSleep();
}

static void fatal(int beeps) {
  for (int i = 0; i < beeps; i++) { piezo.beep(300, 250); delay(200); }
  delay(1000);
  goToSleep();
}

static void bleStart() {
  if (ble.begin(BLE_NAME)) { bleLastConnMs = millis(); chirpBleOn(); }
  else { piezo.beep(300, 400); }
}

static bool sleepAllowed() {
#if LOW_POWER && !DEBUG_PLOT
  return intOk && !ble.active() && (!piezo.playing() || piezo.keepsRunningInSleep());
#else
  return false;
#endif
}

// Espera a proxima amostra do MPU (INT alto). Dorme se puder.
static void waitForImuData() {
  uint32_t t0 = millis();
  while (digitalRead(PIN_MPU_INT) == LOW) {
    if (millis() - t0 > 30) return;      // INT nao veio (fio solto?): segue assim mesmo
    if (sleepAllowed()) esp_light_sleep_start();   // acorda pelo INT ou pelo timer de 50 ms
    else delay(1);                                  // idle do FreeRTOS
  }
}

// ---------- setup ----------
void setup() {
  setCpuFrequencyMhz(80);                // suficiente (e o minimo com radio ligado)
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_MPU_INT, INPUT);

  if (!piezo.begin(PIN_PIEZO_A, PIN_PIEZO_B, PIEZO_KEEP_IN_SLEEP)) {
    piezo.begin(PIN_PIEZO_A, PIN_PIEZO_B, false);   // RC_FAST nao aceito: fallback
  }

  // Acordou pelo botao: so liga se segurar ~1 s (evita ligar na mochila)
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) {
    uint32_t t0 = millis();
    while (digitalRead(PIN_BUTTON) == LOW && millis() - t0 < 1000) delay(10);
    if (millis() - t0 < 1000) goToSleep();
  }

  cfg.load();
  applySettings();

  // Portal: continuar segurando o botao por mais ~3 s
  if (digitalRead(PIN_BUTTON) == LOW) {
    uint32_t t1 = millis();
    while (digitalRead(PIN_BUTTON) == LOW && millis() - t1 < 3000) delay(10);
    if (millis() - t1 >= 3000) {
      piezo.setVolume(PiezoIDF::VOL_HIGH);
      piezo.beep(1800, 80); delay(60); piezo.beep(2100, 80); delay(60); piezo.beep(2400, 150);
      while (digitalRead(PIN_BUTTON) == LOW) delay(10);   // espera soltar
      portal::run(cfg, piezo, FW_VERSION, readBatteryV, powerOff, PIN_BUTTON);  // nao retorna
    }
  }

#if DEBUG_PLOT
  Serial.begin(115200);
#endif

  Wire.begin(PIN_SDA, PIN_SCL, 400000);
  if (!baro.begin(Wire)) fatal(2);       // 2 bipes graves = MS5611 nao encontrado / CRC
  if (!imu.begin(Wire))  fatal(3);       // 3 bipes graves = MPU-6050 nao encontrado

  // Apoiar parado ~2 s: bias do giro + escala do acelerometro.
  if (!imu.calibrate()) piezo.beep(400, 300);

  // Confere se o INT do MPU esta ligado (sem ele nao da para dormir)
  imu.read();                            // limpa o latch
  uint32_t t0 = millis();
  while (millis() - t0 < 60 && digitalRead(PIN_MPU_INT) == LOW) delay(1);
  intOk = digitalRead(PIN_MPU_INT) == HIGH;
  if (!intOk) { for (int i = 0; i < 4; i++) { piezo.beep(350, 80); delay(80); } }  // 4 bipes curtos graves

  // Atitude inicial pela gravidade media
  float sx = 0, sy = 0, sz = 0;
  for (int i = 0; i < 20; i++) { delay(20); imu.read(); sx += imu.ax; sy += imu.ay; sz += imu.az; }
  ahrs.begin();
  applySettings();                       // ahrs.begin() nao mexe no airspeed, mas garante
  ahrs.alignToAccel(sx, sy, sz);

  // Altitude inicial: media de algumas leituras
  baro.startTemperature(); delay(10); baro.readResult();
  float hSum = 0; int hN = 0;
  for (int i = 0; i < 8; i++) {
    baro.startPressure(); delay(10);
    if (baro.readResult()) { hSum += MS5611::altitudeM(baro.pressurePa()); hN++; }
  }
  if (hN == 0) fatal(2);
  kf.begin(hSum / hN, cfg.s.kfAcc, 0.02f, cfg.s.kfBaro);

  // "pronto": tom subindo + 1..4 bipes de bateria (sempre audiveis)
  batteryV = readBatteryV();
  piezo.setVolume(PiezoIDF::VOL_HIGH);
  piezo.beep(600, 80); piezo.beep(900, 80); piezo.beep(1300, 120);
  delay(300);
  for (int i = 0; i < batteryBars(batteryV); i++) { piezo.beep(1000, 60); delay(120); }
  piezo.setVolume((PiezoIDF::Volume)cfg.s.volume);

  if (cfg.s.bleEnabled) { delay(200); bleStart(); }

  // Fontes de despertar do light sleep: INT do MPU (nivel alto) + timer de seguranca
  gpio_wakeup_enable((gpio_num_t)PIN_MPU_INT, GPIO_INTR_HIGH_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  esp_sleep_enable_timer_wakeup(50 * 1000);   // us

  baro.startPressure();
  imu.read();
  lastActivityMs = millis();
  lastMicros = micros();
}

// ---------- botao ----------
static void onClicks(int n, uint32_t now) {
  if (n == 1) {                                           // volume
    auto v = (PiezoIDF::Volume)((piezo.volume() + 1) % 4);
    piezo.play(0);
    if (v == PiezoIDF::VOL_MUTE) {
      piezo.setVolume(PiezoIDF::VOL_LOW);
      piezo.beep(400, 150);                               // "mudo"
    } else {
      piezo.setVolume(v);
      for (int i = 0; i < (int)v; i++) { piezo.beep(1000, 50); delay(80); }
    }
    piezo.setVolume(v);
    cfg.s.volume = (uint8_t)v;
    cfg.saveVolume();                                     // lembra no proximo voo
  } else if (n == 2) {                                    // BLE liga/desliga
    piezo.play(0);
    cfg.s.bleEnabled = !ble.active();
    cfg.saveBleEnabled();
    if (cfg.s.bleEnabled) bleStart();
    else { ble.end(); chirpBleOff(); }
  }
  lastActivityMs = now;
}

static void handleButton(uint32_t now) {
  static uint32_t pressedAt = 0, releasedAt = 0;
  static bool wasPressed = false;
  static int clicks = 0;
  bool p = digitalRead(PIN_BUTTON) == LOW;
  if (p && !wasPressed) pressedAt = now;
  if (p && now - pressedAt > 2000) powerOff();
  if (!p && wasPressed && now - pressedAt > 30) { clicks++; releasedAt = now; }
  if (!p && clicks > 0 && now - releasedAt > CLICK_GAP_MS) { onClicks(clicks, now); clicks = 0; }
  wasPressed = p;
}

// ---------- BLE ----------
static void serviceBle(uint32_t now, float climb) {
  if (!ble.active()) return;
  uint32_t every = (uint32_t)lroundf(50.0f / cfg.s.bleHz);
  if (every < 1) every = 1;
  if (tickCount % every == 0)
    ble.send(baro.pressurePa(), climb, baro.temperatureC(), batteryV);
  if (ble.connected()) { bleLastConnMs = now; lastActivityMs = now; return; }
  // ninguem conectado ha bleTimeoutMin: desliga o BLE para voltar ao light sleep
  if (cfg.s.bleTimeoutMin > 0 && now - bleLastConnMs > (uint32_t)(cfg.s.bleTimeoutMin * 60000.0f)) {
    ble.end();
    chirpBleOff();   // nao muda cfg.s.bleEnabled: no proximo boot tenta de novo
  }
}

// ---------- loop ----------
void loop() {
  waitForImuData();

  uint32_t now = millis();
  uint32_t us = micros();
  float dt = (us - lastMicros) * 1e-6f;
  lastMicros = us;
  if (dt <= 0 || dt > 0.1f) dt = 0.02f;

  // 1) IMU -> atitude -> aceleracao vertical -> predicao do Kalman
  if (imu.read()) {                      // a leitura tambem abaixa o INT
    lastAVert = ahrs.update(imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz, dt);
    kf.predict(lastAVert, dt);
    if (fabsf(imu.gx) + fabsf(imu.gy) + fabsf(imu.gz) > AUTO_OFF_GYRO) lastActivityMs = now;
  }

  // 2) Barometro: le a conversao disparada 2 ticks atras e dispara a proxima
  if (tickCount % BARO_EVERY_TICKS == 0) {
    if (baro.readResult()) kf.update(MS5611::altitudeM(baro.pressurePa()));
    if (++convCount % TEMP_EVERY_CONV == 0) baro.startTemperature();
    else baro.startPressure();
  }

  float climb = kf.climb();
  if (fabsf(climb) > AUTO_OFF_CLIMB) lastActivityMs = now;

  // 3) Som
  piezo.play(audio.tick(climb, now));

  // 4) BLE
  serviceBle(now, climb);

  // 5) Bateria a cada 10 s, botao, auto-off
  if (tickCount % 500 == 0) batteryV = readBatteryV();
  handleButton(now);
  if (cfg.s.autoOffMin > 0 && now - lastActivityMs > (uint32_t)(cfg.s.autoOffMin * 60000.0f)) powerOff();

#if DEBUG_PLOT
  if (tickCount % 5 == 0) {   // 10 Hz, formato do Serial Plotter
    Serial.printf("alt:%.2f climb:%.2f bias:%.3f aVert:%.2f\n",
                  kf.altitude(), climb, kf.bias(), lastAVert);
  }
#endif

  tickCount++;
}
