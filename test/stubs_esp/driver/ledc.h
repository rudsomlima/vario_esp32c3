#pragma once
#include <stdint.h>
#define ESP_OK 0
typedef enum { LEDC_LOW_SPEED_MODE } ledc_mode_t;
typedef enum { LEDC_TIMER_0 } ledc_timer_t;
typedef enum { LEDC_CHANNEL_0, LEDC_CHANNEL_1 } ledc_channel_t;
typedef enum { LEDC_TIMER_10_BIT = 10 } ledc_timer_bit_t;
typedef enum { LEDC_AUTO_CLK, LEDC_USE_RC_FAST_CLK } ledc_clk_cfg_t;
typedef enum { LEDC_INTR_DISABLE } ledc_intr_type_t;
typedef struct { ledc_mode_t speed_mode; ledc_timer_bit_t duty_resolution; ledc_timer_t timer_num; uint32_t freq_hz; ledc_clk_cfg_t clk_cfg; bool deconfigure; } ledc_timer_config_t;
typedef struct { int gpio_num; ledc_mode_t speed_mode; ledc_channel_t channel; ledc_intr_type_t intr_type; ledc_timer_t timer_sel; uint32_t duty; int hpoint; struct { unsigned int output_invert: 1; } flags; } ledc_channel_config_t;
inline int ledc_timer_config(const ledc_timer_config_t*) { return 0; }
inline int ledc_channel_config(const ledc_channel_config_t*) { return 0; }
inline int ledc_stop(ledc_mode_t, ledc_channel_t, uint32_t) { return 0; }
inline int ledc_set_freq(ledc_mode_t, ledc_timer_t, uint32_t) { return 0; }
inline int ledc_set_duty(ledc_mode_t, ledc_channel_t, uint32_t) { return 0; }
inline int ledc_update_duty(ledc_mode_t, ledc_channel_t) { return 0; }
