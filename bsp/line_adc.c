#include "line_adc.h"
#include "timebase.h"
static volatile uint16_t dma_samples[BOARD_ADC_COUNT * 2U];
static volatile LineFrame published;
static LineFrameCallback frame_callback;
static uint8_t log_resume;
void LineADC_SetCallback(LineFrameCallback callback) { frame_callback = callback; }
static void NotifyFrame(void)
{
    LineFrame frame;
    if (frame_callback) { LineADC_GetFrame(&frame); frame_callback(&frame); }
}
static uint8_t WaitCalibration(uint8_t reset)
{
    uint32_t start = Time_Cycles();
    while (reset ? ADC_GetResetCalibrationStatus(ADC1) : ADC_GetCalibrationStatus(ADC1)) {
        if ((uint32_t)(Time_Cycles() - start) > SystemCoreClock / 100U) return 0;
    }
    return 1;
}
uint8_t LineADC_Init(void)
{
    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;
    DMA_InitTypeDef dma;
    TIM_TimeBaseInitTypeDef timer;
    RCC_ClocksTypeDef clocks;
    uint32_t timer_hz;
    uint8_t i;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = 0x00FF;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);
    DMA_DeInit(DMA1_Channel1);
    DMA_StructInit(&dma);
    dma.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    dma.DMA_MemoryBaseAddr = (uint32_t)dma_samples;
    dma.DMA_BufferSize = BOARD_ADC_COUNT * 2U;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma.DMA_Mode = DMA_Mode_Circular;
    dma.DMA_Priority = DMA_Priority_High;
    DMA_Init(DMA1_Channel1, &dma);
    DMA_ClearFlag(DMA1_FLAG_GL1);
    DMA_ITConfig(DMA1_Channel1, DMA_IT_HT | DMA_IT_TC | DMA_IT_TE, ENABLE);
    NVIC_SetPriority(DMA1_Channel1_IRQn, 1U);
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    ADC_StructInit(&adc);
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = ENABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
    adc.ADC_NbrOfChannel = BOARD_ADC_COUNT;
    ADC_Init(ADC1, &adc);
    for (i = 0; i < BOARD_ADC_COUNT; ++i)
        ADC_RegularChannelConfig(ADC1, i, i + 1U, ADC_SampleTime_55Cycles5);
    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    /* ADC stabilization before calibration; clock-based bounded startup wait. */
    {
        uint32_t start = Time_Cycles();
        while ((uint32_t)(Time_Cycles() - start) < SystemCoreClock / 100000U) {}
    }
    ADC_ResetCalibration(ADC1);
    if (!WaitCalibration(1)) return 0;
    ADC_StartCalibration(ADC1);
    if (!WaitCalibration(0)) return 0;
    RCC_GetClocksFreq(&clocks);
    timer_hz = clocks.PCLK1_Frequency * ((RCC->CFGR & RCC_CFGR_PPRE1) ? 2U : 1U);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = (uint16_t)(timer_hz / 1000000U - 1U);
    timer.TIM_Period = BOARD_CONTROL_PERIOD_MS * 1000U - 1U;
    TIM_TimeBaseInit(TIM3, &timer);
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
    DMA_Cmd(DMA1_Channel1, ENABLE);
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
    return 1;
}
void DMA1_Channel1_IRQHandler(void)
{
    uint32_t flags = DMA1->ISR;
    uint8_t offset, i;
    if (flags & DMA1_FLAG_TE1) {
        DMA_ClearITPendingBit(DMA1_IT_GL1);
        TIM_Cmd(TIM3, DISABLE);
        DMA_Cmd(DMA1_Channel1, DISABLE);
        ++published.dma_errors;
        NotifyFrame();
        return;
    }
    if (!(flags & (DMA1_FLAG_HT1 | DMA1_FLAG_TC1))) return;
    if ((flags & (DMA1_FLAG_HT1 | DMA1_FLAG_TC1)) == (DMA1_FLAG_HT1 | DMA1_FLAG_TC1)) {
        /* Timing contract violated: stop capture rather than publish uncertain data. */
        TIM_Cmd(TIM3, DISABLE);
        DMA_Cmd(DMA1_Channel1, DISABLE);
        DMA_ClearITPendingBit(DMA1_IT_GL1);
        ++published.missed_frames;
        NotifyFrame();
        return;
    }
    offset = (flags & DMA1_FLAG_TC1) ? BOARD_ADC_COUNT : 0U;
    DMA_ClearITPendingBit((flags & DMA1_FLAG_TC1) ? DMA1_IT_TC1 : DMA1_IT_HT1);
    for (i = 0; i < BOARD_ADC_COUNT; ++i) published.raw[i] = dma_samples[offset + i];
    published.timestamp_ms = Time_Millis();
    ++published.sequence;
    NotifyFrame();
	}
void LineADC_GetFrame(LineFrame *frame)
{
    uint32_t mask = Time_SaveIRQ();
    uint8_t i;
    for (i = 0; i < BOARD_ADC_COUNT; ++i) frame->raw[i] = published.raw[i];
    frame->sequence = published.sequence;
    frame->timestamp_ms = published.timestamp_ms;
    frame->dma_errors = published.dma_errors;
    frame->missed_frames = published.missed_frames;
    Time_RestoreIRQ(mask);
}
void LineADC_PauseForLog(void)
{
    uint32_t irq=Time_SaveIRQ(), flags;
    log_resume=(TIM3->CR1&TIM_CR1_CEN) && !published.dma_errors && !published.missed_frames;
    TIM_Cmd(TIM3,DISABLE);
    ADC_ExternalTrigConvCmd(ADC1,DISABLE);
    ADC_Cmd(ADC1,DISABLE);
    DMA_Cmd(DMA1_Channel1,DISABLE);
    flags=DMA1->ISR;
    /* Maintenance must not silently clear an already pending capture failure. */
    if(flags&DMA1_FLAG_TE1) { ++published.dma_errors; log_resume=0; }
    if((flags&(DMA1_FLAG_HT1|DMA1_FLAG_TC1))==(DMA1_FLAG_HT1|DMA1_FLAG_TC1)) {
        ++published.missed_frames; log_resume=0;
    }
    DMA_ClearITPendingBit(DMA1_IT_GL1);
    NVIC_ClearPendingIRQ(DMA1_Channel1_IRQn);
    if(published.dma_errors || published.missed_frames) NotifyFrame();
    Time_RestoreIRQ(irq);
}
void LineADC_ResumeAfterLog(void)
{
    uint32_t start,irq=Time_SaveIRQ();
    if(!log_resume) { Time_RestoreIRQ(irq); return; }
    log_resume=0;
    DMA_SetCurrDataCounter(DMA1_Channel1,BOARD_ADC_COUNT*2U);
    DMA_ClearITPendingBit(DMA1_IT_GL1);
    NVIC_ClearPendingIRQ(DMA1_Channel1_IRQn);
    ADC_ClearFlag(ADC1,ADC_FLAG_EOC|ADC_FLAG_STRT);
    ADC_Cmd(ADC1,ENABLE);
    start=Time_Cycles();
    while((uint32_t)(Time_Cycles()-start)<SystemCoreClock/100000U) {}
    TIM_SetCounter(TIM3,0);
    TIM_ClearFlag(TIM3,TIM_FLAG_Update);
    DMA_Cmd(DMA1_Channel1,ENABLE);
    ADC_ExternalTrigConvCmd(ADC1,ENABLE);
    TIM_Cmd(TIM3,ENABLE);
    Time_RestoreIRQ(irq);
}
