// piezo_idf.h - Tom no piezo pelo driver LEDC do ESP-IDF (incluso no Arduino-ESP32 3.x).
//
// Por que IDF e nao ledcWrite() do Arduino: aqui da para escolher o clock do
// LEDC. Com RC_FAST (~17.5 MHz, oscilador interno) o PWM CONTINUA tocando
// durante o light sleep -> a CPU dorme ate enquanto o vario apita.
// keepInSleep=false usa o clock padrao (APB): o loop principal entao nao
// dorme enquanto houver som (fallback seguro se o RC_FAST der problema).
//
// VOL_HIGH: canal 0 no pino A, canal 1 no pino B com saida invertida, mesmo
//           timer -> ponte H (6.6 Vpp no piezo a partir de 3.3 V).
// VOL_MED : so pino A (single-ended). VOL_LOW: single-ended, pulso estreito.
#pragma once
#include <Arduino.h>
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

class PiezoIDF {
public:
  enum Volume : uint8_t { VOL_MUTE, VOL_LOW, VOL_MED, VOL_HIGH };

  bool begin(uint8_t pinA, uint8_t pinB, bool keepInSleep) {
    pa_ = pinA; pb_ = pinB; keepInSleep_ = keepInSleep;

    ledc_timer_config_t t = {};
    t.speed_mode = MODE;
    t.duty_resolution = LEDC_TIMER_10_BIT;
    t.timer_num = TIMER;
    t.freq_hz = 1000;
    t.clk_cfg = keepInSleep ? LEDC_USE_RC_FAST_CLK : LEDC_AUTO_CLK;
    if (ledc_timer_config(&t) != ESP_OK) return false;

    if (!configChannel(CH_A, pa_, false)) return false;
    if (!configChannel(CH_B, pb_, true)) return false;   // invertido
    ledc_stop(MODE, CH_A, 0);
    ledc_stop(MODE, CH_B, B_IDLE);

    if (keepInSleep) {
      // mantem o RC_FAST ligado e os pinos com a config normal durante o sono
      esp_sleep_pd_config(ESP_PD_DOMAIN_RC_FAST, ESP_PD_OPTION_ON);
      gpio_sleep_sel_dis((gpio_num_t)pa_);
      gpio_sleep_sel_dis((gpio_num_t)pb_);
    }
    return true;
  }

  void setVolume(Volume v) {
    vol_ = v;
    uint16_t f = freq_; freq_ = 0xFFFF;   // forca reaplicar
    play(f);
  }
  Volume volume() const { return vol_; }
  bool playing() const { return freq_ != 0 && freq_ != 0xFFFF; }
  bool keepsRunningInSleep() const { return keepInSleep_; }

  // f = 0 -> silencio
  void play(uint16_t f) {
    if (vol_ == VOL_MUTE) f = 0;
    if (f == freq_) return;
    freq_ = f;
    if (f == 0) {
      ledc_stop(MODE, CH_A, 0);
      ledc_stop(MODE, CH_B, B_IDLE);
      return;
    }
    ledc_set_freq(MODE, TIMER, f);
    const uint32_t half = 1u << 9;              // 50% de 10 bits
    uint32_t dA = (vol_ == VOL_LOW) ? half / 8 : half;
    ledc_set_duty(MODE, CH_A, dA);
    ledc_update_duty(MODE, CH_A);
    if (vol_ == VOL_HIGH) {
      ledc_set_duty(MODE, CH_B, half);          // invertido -> complementar ao A
      ledc_update_duty(MODE, CH_B);
    } else {
      ledc_stop(MODE, CH_B, B_IDLE);            // B parado em LOW
    }
  }

  // Bipe bloqueante curto (feedback de botao / inicializacao)
  void beep(uint16_t f, uint16_t ms) { play(f); delay(ms); play(0); }

private:
  static constexpr ledc_mode_t MODE = LEDC_LOW_SPEED_MODE;
  static constexpr ledc_timer_t TIMER = LEDC_TIMER_0;
  static constexpr ledc_channel_t CH_A = LEDC_CHANNEL_0;
  static constexpr ledc_channel_t CH_B = LEDC_CHANNEL_1;
  // a inversao do canal B e feita na matriz de GPIO, DEPOIS do LEDC:
  // nivel ocioso 1 no LEDC = pino em LOW
  static constexpr uint32_t B_IDLE = 1;

  bool configChannel(ledc_channel_t ch, uint8_t pin, bool invert) {
    ledc_channel_config_t c = {};
    c.gpio_num = pin;
    c.speed_mode = MODE;
    c.channel = ch;
    c.intr_type = LEDC_INTR_DISABLE;
    c.timer_sel = TIMER;
    c.duty = 0;
    c.hpoint = 0;
    c.flags.output_invert = invert ? 1 : 0;
    return ledc_channel_config(&c) == ESP_OK;
  }

  uint8_t pa_ = 0, pb_ = 1;
  bool keepInSleep_ = false;
  uint16_t freq_ = 0;
  Volume vol_ = VOL_HIGH;
};
