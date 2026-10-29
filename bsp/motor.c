#include "board.h"
#include "motor.h"
#include "timebase.h"
static uint16_t period;
static volatile int16_t previous[2];
static volatile uint32_t last_command_ms;
static void SetWheel(uint8_t left, int16_t duty)
{
    uint16_t in1 = left ? GPIO_Pin_9 : GPIO_Pin_10;
    uint16_t in2 = left ? GPIO_Pin_8 : GPIO_Pin_11;
    uint8_t polarity = left ? BOARD_LEFT_FORWARD_IN1_HIGH : BOARD_RIGHT_FORWARD_IN1_HIGH;
    uint16_t magnitude;
    if (duty > 1000) duty = 1000;
    if (duty < -1000) duty = -1000;
    if (left) TIM_SetCompare2(TIM4, 0); else TIM_SetCompare1(TIM4, 0);
    if (duty == 0) {
        /* Explicit datasheet short brake: IN1=IN2=H, PWM=L, STBY=H. */
        GPIO_SetBits(GPIOA, in1 | in2);
    } else {
        uint8_t high = (duty > 0) ? polarity : !polarity;
        GPIO_ResetBits(GPIOA, in1 | in2);
        GPIO_SetBits(GPIOA, high ? in1 : in2);
        magnitude = (uint16_t)(((uint32_t)(duty < 0 ? -duty : duty) * period) / 1000U);
        if (left) TIM_SetCompare2(TIM4, magnitude); else TIM_SetCompare1(TIM4, magnitude);
    }
}
void Motor_Init(void)
{
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef timer;
    TIM_OCInitTypeDef oc;
    RCC_ClocksTypeDef clocks;
    uint32_t timer_hz;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_GetClocksFreq(&clocks);
    timer_hz = clocks.PCLK1_Frequency * ((RCC->CFGR & RCC_CFGR_PPRE1) ? 2U : 1U);
    period = (uint16_t)(timer_hz / BOARD_PWM_HZ);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Period = period - 1U;
    TIM_TimeBaseInit(TIM4, &timer);
    TIM_OCStructInit(&oc);
    oc.TIM_OCMode = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OC1Init(TIM4, &oc);
    TIM_OC2Init(TIM4, &oc);
    /* Immediate zeroing before changing direction; no preload delay. */
    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Disable);
    TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Disable);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_ResetBits(GPIOA, gpio.GPIO_Pin);
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOB, &gpio);
    TIM_Cmd(TIM4, ENABLE);
    Motor_StopBrake();
}
void Motor_SetDuty(int16_t left, int16_t right)
{
    uint32_t mask = Time_SaveIRQ();
    uint8_t reverse_left=(left>0 && previous[0]<0) || (left<0 && previous[0]>0);
    uint8_t reverse_right=(right>0 && previous[1]<0) || (right<0 && previous[1]>0);
    /* Guard each reversing wheel separately. An inner-wheel reversal must not
     * interrupt the outer wheel's forward arc command. */
    if(reverse_left && reverse_right && ((left>0 && right<0) || (left<0 && right>0))) {
        /* Changing pivot direction must not insert a both-wheel zero command.
         * Guard the new forward wheel first and briefly retain the old forward
         * wheel, capped by the new forward request. Next update guards the
         * remaining wheel. Explicit Stop and straight reversal bypass this. */
        if(left>0) { right=previous[1]<left?previous[1]:left; left=0; }
        else { left=previous[0]<right?previous[0]:right; right=0; }
    } else {
        if(reverse_left) left=0;
        if(reverse_right) right=0;
    }
    SetWheel(1, left); SetWheel(0, right);
    previous[0] = left; previous[1] = right;
    last_command_ms = Time_Millis();
    Time_RestoreIRQ(mask);
}
void Motor_StopBrake(void) { Motor_SetDuty(0, 0); }
void Motor_StopCoast(void)
{
    uint32_t mask = Time_SaveIRQ();
    GPIO_ResetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11);
    /* Both direction inputs low AND PWM high: high-impedance stop. */
    TIM_SetCompare1(TIM4, period); TIM_SetCompare2(TIM4, period);
    previous[0] = 0; previous[1] = 0;
    Time_RestoreIRQ(mask);
}

void Motor_SafetyTick(uint32_t now)
{
    if ((previous[0] || previous[1]) && (uint32_t)(now - last_command_ms) > 20U)
        Motor_StopBrake();
}
