#include <stdint.h>
#include <stdbool.h>

#include "board_io.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "board_io";

/* GPIO pin masks for different pin groups */
#define OUTPUT_PP_PIN_MASK  ((1ULL << CONFIG_GPIO_EN1) | \
                             (1ULL << CONFIG_GPIO_EN2) | \
                             (1ULL << CONFIG_GPIO_BUZZER))

#define INPUT_PIN_MASK      ((1ULL << CONFIG_GPIO_FAULT) | \
                             (1ULL << CONFIG_GPIO_ISNS)  | \
                             (1ULL << CONFIG_GPIO_VSNS))

/* Default output levels */
#define DEFAULT_EN1_LEVEL   1
#define DEFAULT_EN2_LEVEL   0
#define DEFAULT_BUZZER_LEVEL 0

/**
 * @brief Configure all GPIO pins for the board
 *
 * This function initializes and configures all GPIO pins used by the board:
 * - Output pins: EN1, EN2, BUZZER (push-pull outputs)
 * - Input pins: FAULT, ISNS, VSNS (digital inputs)
 * - I2C pins: SCL, SDA (will be configured by I2C driver)
 */
void configure_pins(void)
{
    ESP_LOGI(TAG, "Starting GPIO configuration");

    /* Reset all pins to default state */
    gpio_reset_pin(CONFIG_GPIO_EN1);
    gpio_reset_pin(CONFIG_GPIO_EN2);
    gpio_reset_pin(CONFIG_GPIO_BUZZER);
    gpio_reset_pin(CONFIG_GPIO_FAULT);
    gpio_reset_pin(CONFIG_GPIO_ISNS);
    gpio_reset_pin(CONFIG_GPIO_VSNS);
    gpio_reset_pin(CONFIG_GPIO_I2C_SCL);
    gpio_reset_pin(CONFIG_GPIO_I2C_SDA);

    /* Configure push-pull output pins */
    gpio_config_t pp_conf = {
        .pin_bit_mask = OUTPUT_PP_PIN_MASK,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&pp_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure output pins: %s", esp_err_to_name(ret));
        return;
    }

    /* Set default output levels */
    gpio_set_level(CONFIG_GPIO_EN1, DEFAULT_EN1_LEVEL);
    gpio_set_level(CONFIG_GPIO_EN2, DEFAULT_EN2_LEVEL);
    gpio_set_level(CONFIG_GPIO_BUZZER, DEFAULT_BUZZER_LEVEL);

    ESP_LOGI(TAG, "Output pins configured: EN1=%d, EN2=%d, BUZZER=%d",
             DEFAULT_EN1_LEVEL, DEFAULT_EN2_LEVEL, DEFAULT_BUZZER_LEVEL);

    /* Configure input pins */
    gpio_config_t in_conf = {
        .pin_bit_mask = INPUT_PIN_MASK,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    ret = gpio_config(&in_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure input pins: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Input pins configured: FAULT, ISNS, VSNS");
    ESP_LOGI(TAG, "GPIO configuration completed successfully");
}