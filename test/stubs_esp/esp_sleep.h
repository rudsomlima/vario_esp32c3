#pragma once
#include <stdint.h>
typedef enum { ESP_SLEEP_WAKEUP_UNDEFINED, ESP_SLEEP_WAKEUP_ALL, ESP_SLEEP_WAKEUP_GPIO = 7 } esp_sleep_source_t;
typedef esp_sleep_source_t esp_sleep_wakeup_cause_t;
typedef enum { ESP_GPIO_WAKEUP_GPIO_LOW = 0, ESP_GPIO_WAKEUP_GPIO_HIGH = 1 } esp_deepsleep_gpio_wake_up_mode_t;
typedef enum { ESP_PD_DOMAIN_RC_FAST } esp_sleep_pd_domain_t;
typedef enum { ESP_PD_OPTION_OFF, ESP_PD_OPTION_ON, ESP_PD_OPTION_AUTO } esp_sleep_pd_option_t;
typedef int esp_err_t;
inline esp_err_t esp_deep_sleep_enable_gpio_wakeup(uint64_t, esp_deepsleep_gpio_wake_up_mode_t) { return 0; }
inline void esp_deep_sleep_start() {}
inline esp_err_t esp_light_sleep_start() { return 0; }
inline esp_err_t esp_sleep_enable_gpio_wakeup() { return 0; }
inline esp_err_t esp_sleep_enable_timer_wakeup(uint64_t) { return 0; }
inline esp_err_t esp_sleep_disable_wakeup_source(esp_sleep_source_t) { return 0; }
inline esp_err_t esp_sleep_pd_config(esp_sleep_pd_domain_t, esp_sleep_pd_option_t) { return 0; }
inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() { return ESP_SLEEP_WAKEUP_UNDEFINED; }
