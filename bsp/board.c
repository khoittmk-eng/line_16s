#include "board.h"
#include "board_init.h"
void Board_RemapOLEDAndKeepSWD(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    /* RM0008: SWJ_CFG is write-only; do not propagate its readback.
     * Write a defined SWD-enabled configuration together with I2C1 remap.
     * Bit 27 is reserved on F103C8 and must remain at its reset value. */
    AFIO->MAPR = (AFIO->MAPR & ~(AFIO_MAPR_SWJ_CFG | (1UL<<27))) |
                 AFIO_MAPR_SWJ_CFG_JTAGDISABLE | AFIO_MAPR_I2C1_REMAP;
}
void Board_Init(void)
{
    GPIO_InitTypeDef gpio;
    Board_RemapOLEDAndKeepSWD();
    /* Giu SWD clock luon hoat dong trong cac che do debug/sleep */
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP | DBGMCU_CR_DBG_STOP | DBGMCU_CR_DBG_STANDBY;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                          RCC_APB2Periph_GPIOC, ENABLE);
    /* Hold existing motor direction and PWM pins low. No motor run command. */
    GPIO_ResetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11);
    GPIO_ResetBits(GPIOB, GPIO_Pin_6 | GPIO_Pin_7);
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOB, &gpio);
    GPIO_SetBits(BOARD_LED_PORT, BOARD_LED_PIN);
    gpio.GPIO_Pin = BOARD_LED_PIN;
    GPIO_Init(BOARD_LED_PORT, &gpio);
}
