#ifndef BOARD_H
#define BOARD_H
#include "stm32f10x.h"
/* Baseline: STM32F103C8T6, 8 MHz HSE, 64 KB Flash, 20 KB RAM. */
/* Existing robot: sensors PA0..PA7; motor DIR PA8..PA11;
 * PWM PB6/PB7; OLED PB8/PB9; buttons PB0/PB1/PB10.
 * TB6612 STBY wiring is not yet confirmed. */
#define BOARD_CONTROL_PERIOD_MS 2U
#define BOARD_LED_PORT GPIOC
#define BOARD_LED_PIN GPIO_Pin_13
/* PC13 active-low LED is an assumption; adapt to your board. */
#endif
