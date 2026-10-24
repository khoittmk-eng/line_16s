#include "board.h"
#include "board_init.h"
static volatile uint32_t uptime_ms;
/* Exposed in debugger; measures tick delivery, not control execution. */
volatile uint32_t control_tick_count;
void SysTick_Handler(void)
{
    ++uptime_ms;
    if ((uptime_ms % BOARD_CONTROL_PERIOD_MS) == 0U) {
        ++control_tick_count;
        /* Future fixed-period control hook: no OLED or blocking I/O here. */
    }
}
int main(void)
{
    uint32_t last_blink = 0U;
    SystemCoreClockUpdate();
    Board_Init();
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U) {
        while (1) { /* Invalid tick configuration. */ }
    }
    while (1) {
        uint32_t now = uptime_ms;
        if ((uint32_t)(now - last_blink) >= 500U) {
            last_blink = now;
            GPIO_WriteBit(BOARD_LED_PORT, BOARD_LED_PIN,
                GPIO_ReadOutputDataBit(BOARD_LED_PORT, BOARD_LED_PIN) ? Bit_RESET : Bit_SET);
        }
        __WFI();
    }
}
