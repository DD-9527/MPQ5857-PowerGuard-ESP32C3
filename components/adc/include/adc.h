#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC reading structure
 */
typedef struct {
    int raw_vsns;    /**< Raw voltage sense ADC value */
    int raw_isns;    /**< Raw current sense ADC value */
    float voltage;   /**< Calibrated voltage in volts */
    float current;   /**< Calibrated current in amps */
} adc_reading_t;

/**
 * @brief Initialize ADC channels and calibration
 *
 * This function initializes the ADC peripheral, configures the voltage and
 * current sense channels, and sets up calibration if available.
 */
void adc_init(void);

/**
 * @brief Get single ADC reading with calibration and conversion
 *
 * This function reads both voltage and current sense channels, applies
 * calibration if available, and converts the readings to physical units.
 *
 * @return adc_reading_t Structure containing raw and calibrated readings
 */
adc_reading_t adc_read_once(void);

#ifdef __cplusplus
}
#endif

#endif /* ADC_H */