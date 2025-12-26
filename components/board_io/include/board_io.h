#ifndef BOARD_IO_H
#define BOARD_IO_H

#ifdef __cplusplus
extern "C" {
#endif

extern int flag;

/**
 * @brief Configure all GPIO pins for the board
 *
 * This function initializes and configures all GPIO pins used by the board:
 * - Output pins: EN1, EN2, BUZZER (push-pull outputs with default levels)
 * - Input pins: FAULT, ISNS, VSNS (digital inputs)
 * - I2C pins: SCL, SDA (reset state, to be configured by I2C driver)
 *
 * The function sets safe default levels for all outputs and configures
 * inputs without pull-up/pull-down resistors.
 */
void configure_pins(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_IO_H */