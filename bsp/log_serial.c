#include "log_serial.h"
#include "board.h"
#include "timebase.h"
void LogSerial_Init(void)
{
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    GPIO_SetBits(GPIOB,GPIO_Pin_12);
    gpio.GPIO_Pin=GPIO_Pin_12; gpio.GPIO_Mode=GPIO_Mode_Out_PP; gpio.GPIO_Speed=GPIO_Speed_2MHz;
    GPIO_Init(GPIOB,&gpio);
}
static void Byte(uint8_t value)
{
    uint32_t bit_cycles=SystemCoreClock/9600U;
    uint32_t start, mask=Time_SaveIRQ();
    unsigned bit;
    uint16_t frame=((uint16_t)value<<1)|0x200U; /* start=0, data LSB first, stop=1 */
    start=Time_Cycles();
    for(bit=0;bit<10;++bit) {
        if(frame&(1U<<bit)) GPIOB->BSRR=GPIO_Pin_12;
        else GPIOB->BRR=GPIO_Pin_12;
        while((uint32_t)(Time_Cycles()-start)<(bit+1U)*bit_cycles) {}
    }
    Time_RestoreIRQ(mask);
}
void LogSerial_Write(const char *text)
{
    while(*text) Byte((uint8_t)*text++);
}
