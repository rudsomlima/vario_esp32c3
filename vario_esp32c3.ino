// vario_esp32c3.ino - Vario audio com fusao acelerometro + barometro
// FASE 2: baixo consumo - light sleep entre amostras, acordado pelo INT do MPU.
//
// Hardware: ESP32-C3 SuperMini + GY-521 (MPU-6050) + GY-63 (MS5611)
//           + piezo passivo entre GPIO0 e GPIO1 + botao GPIO3->GND
//           + LiPo via TP4056 (com protecao) no pino 5V
// Core: Arduino-ESP32 3.x  |  Placa: "ESP32C3 Dev Module", USB CDC On Boot = Enabled
//
// Pinagem
//   GPIO6 SDA, GPIO7 SCL -> MS5611 (0x77) + MPU-6050 (0x68), 3V3 e GND
//   GPIO5 <- INT do MPU-6050 (OBRIGATORIO na fase 2: e ele que acorda a CPU)
//   GPIO3 <- botao para GND (+ pull-up externo 100k)
//   GPIO4 <- divisor da bateria: BAT+ -- 1M -- GPIO4 -- 1M -- GND (+100nF em GPIO4)
//   GPIO0/GPIO1 -> piezo
//
// Botao: curto = volume (mudo/baixo/medio/alto, fica salvo) | longo 2 s = desliga
//        ligar = segurar ~1 s
//
// Ciclo (50x por segundo):
//   [light sleep] --INT do MPU--> le IMU -> Mahony -> Kalman predict
//   -> (a cada 2) le MS5611 + dispara proxima conversao -> Kalman update
//   -> som -> botao -> [light sleep]
// O MS5611 converte e o PWM do piezo toca enquanto a CPU dorme.

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include "driver/gpio.h"
#include "ms5611.h"
#include "imu_mpu6050.h"
#include "mahony_gravity.h"
#include "kalman_vario.h"
#include "vario_audio.h"
#include "piezo_idf.h"

// ---------------- configuracao ----------------
#define LOW_POWER            1  // 1 = light sleep entre amostras (fase 2)
#define PIEZO_KEEP_IN_SLEEP  1  // 1 = PWM no RC_FAST, dorme ate apitando. Se o som
                                //     falhar/engasgar, ponha 0 (fica acordado so ao apitar)
#define DEBUG_PLOT           0  // 1 = Serial Plotter. DESLIGA o sleep (o USB cai no light sleep)

const uint8_t PIN_SDA = 6, PIN_SCL = 7;
const uint8_t PIN_MPU_INT = 5;
const uint8_t PIN_BUTTON = 3;
const uint8_t PIN_VBAT = 4;
const uint8_t PIN_PIEZO_A = 0, PIN_PIEZO_B = 1;

const uint8_t  BARO_EVERY_TICKS = 2;    // 25 Hz de pressao (tick = amostra do MPU, 50 Hz)
const uint8_t  TEMP_EVERY_CONV  = 25;   // 1 temperatura por segundo
const uint32_t AUTO_OFF_MS      = 20UL * 60UL * 1000UL;  // 20 min sem atividade
const float    AUTO_OFF_CLIMB   = 0.3f;  // m/s
const float    AUTO_OFF_GYRO    = 0.5f;  // rad/s (soma |gx|+|gy|+|gz|)

// Compensacao centrifuga: velocidade de voo em termica (m/s). ~9 m/s = 32 km/h.
// O eixo X do GY-521 deve apontar para a FRENTE (se apontar para tras: -9.0).
// Errar para MENOS e mais seguro que para mais (ver test/sim_flight.cpp). 0 = desliga.
const float AIRSPEED_MS = 9.0f;

// Kalman: ajuste fino depois de voar (ver README)
const float KF_SIGMA_ACC  = 0.35f;  // m/s^2
const float KF_SIGMA_BIAS = 0.02f;  // m/s^2/sqrt(s)
const float KF_SIGMA_BARO = 0.25f;  // m

// --------------------------------------------
MS5611        baro;
ImuMPU6050    imu;
MahonyGravity ahrs;
KalmanVario   kf;
VarioAudio    audio;
PiezoIDF      piezo;
Preferences   prefs;

static uint32_t tickCount = 0, convCount = 0;
static uint32_t lastActivityMs = 0;
static uint32_t lastMicros = 0;
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

static void goToSleep() {
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

static bool sleepAllowed() {
#if LOW_POWER && !DEBUG_PLOT
  return intOk && (!piezo.playing() || piezo.keepsRunningInSleep());
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
  setCpuFrequencyMhz(80);                // suficiente; 160 MHz so gasta mais
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_MPU_INT, INPUT);

  bool keep = PIEZO_KEEP_IN_SLEEP;
  if (!piezo.begin(PIN_PIEZO_A, PIN_PIEZO_B, keep)) {
    piezo.begin(PIN_PIEZO_A, PIN_PIEZO_B, false);   // RC_FAST nao aceito: fallback
  }

  // Acordou pelo botao: so liga se segurar ~1 s (evita ligar na mochila)
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) {
    uint32_t t0 = millis();
    while (digitalRead(PIN_BUTTON) == LOW && millis() - t0 < 1000) delay(10);
    if (millis() - t0 < 1000) goToSleep();
  }

#if DEBUG_PLOT
  Serial.begin(115200);
#endif

  prefs.begin("vario", false);
  uint8_t vol = prefs.getUChar("vol", PiezoIDF::VOL_HIGH);
  if (vol > PiezoIDF::VOL_HIGH) vol = PiezoIDF::VOL_HIGH;

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
  ahrs.airspeed = AIRSPEED_MS;
  ahrs.alignToAccel(sx, sy, sz);

  // Altitude inicial: media de algumas leituras
  baro.startTemperature(); delay(10); baro.readResult();
  float hSum = 0; int hN = 0;
  for (int i = 0; i < 8; i++) {
    baro.startPressure(); delay(10);
    if (baro.readResult()) { hSum += MS5611::altitudeM(baro.pressurePa()); hN++; }
  }
  if (hN == 0) fatal(2);
  kf.begin(hSum / hN, KF_SIGMA_ACC, KF_SIGMA_BIAS, KF_SIGMA_BARO);

  // "pronto": tom subindo + 1..4 bipes de bateria (sempre audiveis)
  batteryV = readBatteryV();
  piezo.setVolume(PiezoIDF::VOL_HIGH);
  piezo.beep(600, 80); piezo.beep(900, 80); piezo.beep(1300, 120);
  delay(300);
  for (int i = 0; i < batteryBars(batteryV); i++) { piezo.beep(1000, 60); delay(120); }
  piezo.setVolume((PiezoIDF::Volume)vol);

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
static void handleButton(uint32_t now) {
  static uint32_t pressedAt = 0;
  static bool wasPressed = false;
  bool p = digitalRead(PIN_BUTTON) == LOW;
  if (p && !wasPressed) pressedAt = now;
  if (p && now - pressedAt > 2000) powerOff();
  if (!p && wasPressed && now - pressedAt > 30) {          // clique curto: volume
    auto v = (PiezoIDF::Volume)((piezo.volume() + 1) % 4);
    piezo.play(0);
    if (v == PiezoIDF::VOL_MUTE) {
      piezo.setVolume(PiezoIDF::VOL_LOW);
      piezo.beep(400, 150);                                // "mudo"
    } else {
      piezo.setVolume(v);
      for (int i = 0; i < (int)v; i++) { piezo.beep(1000, 50); delay(80); }
    }
    piezo.setVolume(v);
    prefs.putUChar("vol", (uint8_t)v);                     // lembra no proximo voo
    lastActivityMs = now;
  }
  wasPressed = p;
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

  // 4) Bateria a cada 10 s, botao, auto-off
  if (tickCount % 500 == 0) batteryV = readBatteryV();
  handleButton(now);
  if (now - lastActivityMs > AUTO_OFF_MS) powerOff();

#if DEBUG_PLOT
  if (tickCount % 5 == 0) {   // 10 Hz, formato do Serial Plotter
    Serial.printf("alt:%.2f climb:%.2f bias:%.3f aVert:%.2f\n",
                  kf.altitude(), climb, kf.bias(), lastAVert);
  }
#endif

  tickCount++;
}
