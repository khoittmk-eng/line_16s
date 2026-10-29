/* Actual bsp/motor.c, with SPL register writes modeled in RAM-backed MMIO.
 * This checks electrical requests, not motor dynamics or the STM32 PWM waveform. */
#include "board.h"
#include "timebase.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
void DriverMotor_Init(void);
void DriverMotor_SetDuty(int16_t left, int16_t right);
void DriverMotor_StopBrake(void);
void DriverMotor_StopCoast(void);
void DriverMotor_SafetyTick(uint32_t now);
static void Check(int ok, unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=50000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
void RCC_APB1PeriphClockCmd(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void RCC_APB2PeriphClockCmd(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void RCC_GetClocksFreq(RCC_ClocksTypeDef *c) { c->PCLK1_Frequency=36000000; }
void GPIO_StructInit(GPIO_InitTypeDef *g) { (void)g; }
void GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *g) { (void)p; (void)g; }
void GPIO_SetBits(GPIO_TypeDef *p, uint16_t pins) { p->ODR|=pins; }
void GPIO_ResetBits(GPIO_TypeDef *p, uint16_t pins) { p->ODR&=~pins; }
void TIM_TimeBaseStructInit(TIM_TimeBaseInitTypeDef *t) { (void)t; }
void TIM_TimeBaseInit(TIM_TypeDef *p, TIM_TimeBaseInitTypeDef *t) { p->ARR=t->TIM_Period; }
void TIM_OCStructInit(TIM_OCInitTypeDef *t) { (void)t; }
void TIM_OC1Init(TIM_TypeDef *p, TIM_OCInitTypeDef *t) { (void)p; (void)t; }
void TIM_OC2Init(TIM_TypeDef *p, TIM_OCInitTypeDef *t) { (void)p; (void)t; }
void TIM_OC1PreloadConfig(TIM_TypeDef *p, uint16_t t) { (void)p; (void)t; }
void TIM_OC2PreloadConfig(TIM_TypeDef *p, uint16_t t) { (void)p; (void)t; }
void TIM_Cmd(TIM_TypeDef *p, FunctionalState s) { (void)p; (void)s; }
void TIM_SetCompare1(TIM_TypeDef *p, uint16_t n) { p->CCR1=n; }
void TIM_SetCompare2(TIM_TypeDef *p, uint16_t n) { p->CCR2=n; }
void RunMotorTests(void)
{
    const uint16_t pins=GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;
    RCC->CFGR=RCC_CFGR_PPRE1_DIV2;
    DriverMotor_Init();
    CHECK(TIM4->ARR==3599 && TIM4->CCR1==0 && TIM4->CCR2==0);
    CHECK((GPIOA->ODR&pins)==pins); /* HH on each channel explicitly requests short brake. */
    DriverMotor_SetDuty(600,300);
    CHECK(TIM4->CCR2==2160 && TIM4->CCR1==1080);
    CHECK((GPIOA->ODR&pins)==(GPIO_Pin_8|GPIO_Pin_11));
    /* B33: a zero inner-wheel command must brake without interrupting the
     * driven outer wheel. Both directions and both channels are exercised. */
    DriverMotor_SetDuty(850,0);
    CHECK(TIM4->CCR2==3060 && TIM4->CCR1==0);
    CHECK((GPIOA->ODR&pins)==(GPIO_Pin_8|GPIO_Pin_10|GPIO_Pin_11));
    DriverMotor_SetDuty(0,850);
    CHECK(TIM4->CCR2==0 && TIM4->CCR1==3060);
    CHECK((GPIOA->ODR&pins)==(GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_11));
    DriverMotor_SetDuty(0,0);
    CHECK(!TIM4->CCR1 && !TIM4->CCR2 && (GPIOA->ODR&pins)==pins);
    DriverMotor_SetDuty(-850,0);
    CHECK(TIM4->CCR2==3060 && !TIM4->CCR1 && (GPIOA->ODR&pins)==(GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11));
    DriverMotor_SetDuty(0,-850);
    CHECK(!TIM4->CCR2 && TIM4->CCR1==3060 && (GPIOA->ODR&pins)==(GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10));
    DriverMotor_StopBrake();
    CHECK(!TIM4->CCR1 && !TIM4->CCR2 && (GPIOA->ODR&pins)==pins);
    DriverMotor_StopCoast();
    CHECK(TIM4->CCR1==3600 && TIM4->CCR2==3600 && !(GPIOA->ODR&pins));
    DriverMotor_SetDuty(600,600);
    DriverMotor_SetDuty(-200,200); /* Only the reversing left wheel is zeroed. */
    CHECK(TIM4->CCR1==720 && !TIM4->CCR2 && (GPIOA->ODR&pins)==(GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_11));
    DriverMotor_SetDuty(-200,200);
    CHECK(TIM4->CCR1==720 && TIM4->CCR2==720);
    CHECK((GPIOA->ODR&pins)==(GPIO_Pin_9|GPIO_Pin_11));
    DriverMotor_SetDuty(200,-200); /* Pivot retarget: stagger both reversal guards. */
    CHECK(TIM4->CCR1==720 && !TIM4->CCR2 && (GPIOA->ODR&pins)==(GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_11));
    DriverMotor_SetDuty(200,-200);
    CHECK(!TIM4->CCR1 && TIM4->CCR2==720 && (GPIOA->ODR&pins)==(GPIO_Pin_8|GPIO_Pin_10|GPIO_Pin_11));
    DriverMotor_SetDuty(200,-200);
    CHECK(TIM4->CCR1==720 && TIM4->CCR2==720);
    /* A lower requested ceiling also bounds the briefly retained forward wheel. */
    DriverMotor_SetDuty(-100,100);
    CHECK(!TIM4->CCR1 && TIM4->CCR2==360 && (GPIOA->ODR&pins)==(GPIO_Pin_8|GPIO_Pin_10|GPIO_Pin_11));
    DriverMotor_StopBrake();
    CHECK(!TIM4->CCR1 && !TIM4->CCR2 && (GPIOA->ODR&pins)==pins);
    /* Ordinary straight forward-to-reverse still zeroes both for its guard. */
    DriverMotor_SetDuty(400,400); DriverMotor_SetDuty(-200,-200);
    CHECK(!TIM4->CCR1 && !TIM4->CCR2 && (GPIOA->ODR&pins)==pins);
    DriverMotor_SetDuty(-200,-200);
    CHECK(TIM4->CCR1==720 && TIM4->CCR2==720);
    DriverMotor_SafetyTick(Time_Millis()+21);
    CHECK(!TIM4->CCR1 && !TIM4->CCR2 && (GPIOA->ODR&pins)==pins);
    /* The B32 lease fix remains valid for zero drive commands; zero is now
     * already braking before the lease expires. Explicit coast stays separate. */
    DriverMotor_SetDuty(0,0);
    DriverMotor_SafetyTick(Time_Millis()+20);
    CHECK(!TIM4->CCR1 && !TIM4->CCR2 && (GPIOA->ODR&pins)==pins);
    DriverMotor_SafetyTick(Time_Millis()+21);
    CHECK(!TIM4->CCR1 && !TIM4->CCR2 && (GPIOA->ODR&pins)==pins);
    DriverMotor_StopCoast(); /* Intentional standalone coast has no drive lease. */
    DriverMotor_SafetyTick(Time_Millis()+21);
    CHECK(TIM4->CCR1==3600 && TIM4->CCR2==3600 && !(GPIOA->ODR&pins));
    DriverMotor_StopBrake();
    DriverMotor_SafetyTick(Time_Millis()+21);
    CHECK(!TIM4->CCR1 && !TIM4->CCR2 && (GPIOA->ODR&pins)==pins);
}
