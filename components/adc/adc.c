#include <stdint.h>
#include <stdbool.h>

#include "adc.h"
#include "mpq5857.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "adc";

/* ADC Channel Configuration */
#define ADC_CHAN_VSNS ADC_CHANNEL_1    /**< Voltage sense channel */
#define ADC_CHAN_ISNS ADC_CHANNEL_0    /**< Current sense channel */

#define ADC_ATTEN_VSNS ADC_ATTEN_DB_6  /**< Voltage sense attenuation */
#define ADC_ATTEN_ISNS ADC_ATTEN_DB_2_5  /**< Current sense attenuation */

/* Voltage calculation constants */
#define VOLTAGE_SCALE_FACTOR 23.9f     /**< Voltage scaling factor */
#define VOLTAGE_OFFSET -0.07f         /**< Voltage offset in volts */

/* Current calculation constants */
#define CURRENT_SCALE_FACTOR (10.0 * 24.0)     /**< Current scaling factor */
#define CURRENT_OFFSET -0.0f         /**< Current offset in amps */

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t cali_handle_vsns = NULL;
static adc_cali_handle_t cali_handle_isns = NULL;

/**
 * @brief Initialize ADC calibration for specified channel
 * @param unit ADC unit
 * @param channel ADC channel
 * @param atten Attenuation setting
 * @param out_handle Output calibration handle
 * @return true if calibration successful, false otherwise
 */
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel,
                                adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    if (!out_handle) {
        ESP_LOGE(TAG, "Invalid output handle");
        return false;
    }

    adc_cali_handle_t handle = NULL;
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NOT_SUPPORTED) {
            ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
        } else {
            ESP_LOGE(TAG, "Calibration failed: %s", esp_err_to_name(ret));
        }
        return false;
    }

    *out_handle = handle;
    ESP_LOGI(TAG, "ADC calibration initialized for channel %d", channel);
    return true;
}

void adc_init(void)
{
    /* Initialize ADC unit */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC unit: %s", esp_err_to_name(ret));
        return;
    }

    /* Configure voltage sense channel */
    adc_oneshot_chan_cfg_t cfg_vsns = {
        .atten = ADC_ATTEN_VSNS,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    
    ret = adc_oneshot_config_channel(adc1_handle, ADC_CHAN_VSNS, &cfg_vsns);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure VSNS channel: %s", esp_err_to_name(ret));
        return;
    }

    /* Configure current sense channel */
    adc_oneshot_chan_cfg_t cfg_isns = {
        .atten = ADC_ATTEN_ISNS,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    
    ret = adc_oneshot_config_channel(adc1_handle, ADC_CHAN_ISNS, &cfg_isns);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ISNS channel: %s", esp_err_to_name(ret));
        return;
    }

    /* Initialize calibration */
    bool vsns_calibrated = adc_calibration_init(ADC_UNIT_1, ADC_CHAN_VSNS,
                                               ADC_ATTEN_VSNS, &cali_handle_vsns);
    bool isns_calibrated = adc_calibration_init(ADC_UNIT_1, ADC_CHAN_ISNS,
                                               ADC_ATTEN_ISNS, &cali_handle_isns);

    if (vsns_calibrated && isns_calibrated) {
        ESP_LOGI(TAG, "ADC initialized successfully with calibration");
    } else {
        ESP_LOGW(TAG, "ADC initialized with partial calibration");
    }
}

adc_reading_t adc_read_once(void)
{
    adc_reading_t result = {0};
    
    if (!adc1_handle) {
        ESP_LOGE(TAG, "ADC not initialized");
        return result;
    }

    int temp_voltage = 0;
    int temp_current = 0;

    /* Read voltage sense */
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHAN_VSNS, &result.raw_vsns);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read VSNS: %s", esp_err_to_name(ret));
        result.raw_vsns = 0;
    }

    if (cali_handle_vsns) {
        ret = adc_cali_raw_to_voltage(cali_handle_vsns, result.raw_vsns, &temp_voltage);
        if (ret == ESP_OK) {
            result.voltage = (temp_voltage * VOLTAGE_SCALE_FACTOR / 1000.0f) + VOLTAGE_OFFSET;
        } else {
            ESP_LOGE(TAG, "Failed to calibrate VSNS: %s", esp_err_to_name(ret));
            result.voltage = 0.0f;
        }
    } else {
        /* No calibration available, use raw value */
        result.voltage = (float)result.raw_vsns;
    }

    /* Read current sense */
    ret = adc_oneshot_read(adc1_handle, ADC_CHAN_ISNS, &result.raw_isns);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ISNS: %s", esp_err_to_name(ret));
        result.raw_isns = 0;
    }
    
    if (cali_handle_isns) {
        ret = adc_cali_raw_to_voltage(cali_handle_isns, result.raw_isns, &temp_current);
        if (ret == ESP_OK) {
            result.current = ((temp_current / CURRENT_SCALE_FACTOR) + CURRENT_OFFSET) * (mpq5857_get_OCPLVL() + 1);//与mpq5857耦合，需要解耦
        } else {
            ESP_LOGE(TAG, "Failed to calibrate ISNS: %s", esp_err_to_name(ret));
            result.current = 0.0f;
        }
    } else {
        /* No calibration available, use raw value */
        result.current = (float)result.raw_isns;
    }

    return result;
}