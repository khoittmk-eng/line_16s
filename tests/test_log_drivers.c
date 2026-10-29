/* Actual log_storage.c/log_serial.c with fake SPL Flash operations and a fake
 * DWT clock. This validates addresses/framing, not physical erase or baud. */
#include "board.h"
#include "stm32f10x_flash.h"
#include <string.h>
extern volatile unsigned test_count, failure_line;
void test_done(void);
void DriverLogSerial_Init(void);
void DriverLogSerial_Write(const char *text);
uint8_t DriverLogStorage_Begin(void);
uint8_t DriverLogStorage_Erase(unsigned slot);
uint8_t DriverLogStorage_Write(unsigned slot,unsigned offset,const void *data,unsigned bytes);
void DriverLogStorage_End(void);
static void Check(int ok,unsigned line)
{
    ++test_count;
    if(!ok) { failure_line=60000U+line; test_done(); }
}
#define CHECK(x) Check((x),__LINE__)
static uint8_t irq_disabled, flash_unlocked, fail_flash;
static uint32_t cycles, bit_time[40];
static uint8_t bits[40];
static unsigned bit_count, bytes_sent, pages_erased, programmed;
uint32_t SerialTest_SaveIRQ(void)
{
    CHECK(!irq_disabled); irq_disabled=1; ++bytes_sent; return 0;
}
void SerialTest_RestoreIRQ(uint32_t previous)
{
    CHECK(irq_disabled && previous==0); irq_disabled=0;
}
uint32_t SerialTest_Cycles(void)
{
    if(GPIOB->BSRR || GPIOB->BRR) {
        CHECK(irq_disabled && bit_count<40);
        CHECK((GPIOB->BSRR==GPIO_Pin_12 && !GPIOB->BRR) || (GPIOB->BRR==GPIO_Pin_12 && !GPIOB->BSRR));
        bit_time[bit_count]=cycles; bits[bit_count++]=GPIOB->BSRR!=0;
        GPIOB->BSRR=GPIOB->BRR=0;
    }
    cycles+=75; return cycles;
}
void FLASH_Unlock(void) { CHECK(!flash_unlocked); flash_unlocked=1; }
void FLASH_Lock(void) { flash_unlocked=0; }
void FLASH_ClearFlag(uint32_t flag) { (void)flag; }
FLASH_Status FLASH_ErasePage(uint32_t address)
{
    CHECK(flash_unlocked && address>=0x60008000UL && address<0x6000C000UL && !(address&1023U));
    ++pages_erased;
    if(fail_flash) return FLASH_ERROR_PG;
    memset((void *)address,255,1024); return FLASH_COMPLETE;
}
FLASH_Status FLASH_ProgramHalfWord(uint32_t address,uint16_t value)
{
    CHECK(flash_unlocked && address>=0x60008000UL && address<0x6000C000UL && !(address&1U));
    ++programmed;
    if(fail_flash) return FLASH_ERROR_PG;
    *(uint16_t *)address&=value; return FLASH_COMPLETE;
}
void RunLogDriverTests(void)
{
    unsigned i,j,before;
    const uint8_t values[4]={0x01,0x55,0xAA,0xFF};
    uint16_t value=0x1234;
    DriverLogSerial_Init(); CHECK(GPIOB->ODR&GPIO_Pin_12);
    GPIOB->BSRR=GPIOB->BRR=0; cycles=0xFFFFF000UL;
    DriverLogSerial_Write("\x01\x55\xAA\xFF");
    CHECK(bytes_sent==4 && bit_count==40 && !irq_disabled);
    for(i=0;i<4;++i) {
        CHECK(bits[i*10]==0 && bits[i*10+9]==1);
        for(j=0;j<8;++j) CHECK(bits[i*10+j+1]==((values[i]>>j)&1));
        for(j=1;j<10;++j) CHECK((uint32_t)(bit_time[i*10+j]-bit_time[i*10+j-1])==7500U);
    }
    CHECK(!DriverLogStorage_Erase(0) && !DriverLogStorage_Write(0,0,&value,2));
    CHECK(DriverLogStorage_Begin()); CHECK(!DriverLogStorage_Begin());
    CHECK(!DriverLogStorage_Erase(2) && !pages_erased);
    CHECK(DriverLogStorage_Erase(1) && pages_erased==8);
    CHECK(DriverLogStorage_Write(1,8190,&value,2));
    CHECK(*(uint16_t *)0x6000BFFEUL==value);
    before=programmed;
    CHECK(!DriverLogStorage_Write(2,0,&value,2));
    CHECK(!DriverLogStorage_Write(1,8192,&value,2));
    CHECK(!DriverLogStorage_Write(1,1,&value,2));
    CHECK(!DriverLogStorage_Write(1,0,&value,3));
    CHECK(!DriverLogStorage_Write(1,0xFFFFFFFEU,&value,4));
    CHECK(programmed==before);
    fail_flash=1; CHECK(!DriverLogStorage_Erase(0));
    CHECK(!DriverLogStorage_Write(1,0,&value,2));
    DriverLogStorage_End(); CHECK(!flash_unlocked);
    fail_flash=0;
}
