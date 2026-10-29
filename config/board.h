#ifndef BOARD_H
#define BOARD_H
#include "stm32f10x.h"
/* Wiring from the owner; electrical orientation still needs bench verification. */
#define BOARD_FIRMWARE_TAG "B29"
#define BOARD_CONTROL_PERIOD_MS 2U
#define BOARD_LED_PORT GPIOC
#define BOARD_LED_PIN GPIO_Pin_13
#define BOARD_ADC_COUNT 8U
#define BOARD_PWM_HZ 20000U
/* Provisional button polarity inherited from the previous working project. */
#define BOARD_BUTTON_ACTIVE_LOW 1U
#define BOARD_BUTTON_DEBOUNCE_MS 25U
#define BOARD_BUTTON_LONG_MS 600U
/* Forward polarities inherited from previous firmware, not yet bench-verified. */
#define BOARD_LEFT_FORWARD_IN1_HIGH 0U
#define BOARD_RIGHT_FORWARD_IN1_HIGH 0U
/* No encoder and no GPIO for STBY (permanently tied to 3V3). */
#define BOARD_OLED_ADDRESS_7BIT 0x3CU
#define BOARD_OLED_I2C_HZ 400000U
#define BOARD_OLED_WIDTH 128U
#define BOARD_OLED_HEIGHT 64U
/* Keep the previous working initialization profile, despite its SH1106 name. */
#define BOARD_OLED_LEGACY_INIT 1U
#define BOARD_OLED_COLUMN_OFFSET 0U
/* Motor Test verified by owner. RUN uses calibrated black-line feedback at 2 ms. */
#endif
