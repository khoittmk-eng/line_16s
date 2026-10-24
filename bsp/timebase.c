#include "timebase.h"
#include "robot.h"
#include "motor.h"
static volatile uint32_t uptime_ms;
void Time_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    if (SysTick_Config(SystemCoreClock / 1000U) != 0U) { while (1) {} }
    NVIC_SetPriority(SysTick_IRQn, 0U);
}
void SysTick_Handler(void) { ++uptime_ms; Robot_Watchdog(uptime_ms); Motor_SafetyTick(uptime_ms); }
uint32_t Time_Millis(void) { return uptime_ms; }
uint32_t Time_Cycles(void) { return DWT->CYCCNT; }
uint32_t Time_SaveIRQ(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    __DMB();
    return mask;
}
void Time_RestoreIRQ(uint32_t mask) { __DMB(); __set_PRIMASK(mask); }
