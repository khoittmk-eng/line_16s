#include "log_storage.h"
#include "stm32f10x_flash.h"
#include "line_adc.h"
#include "timebase.h"
static uint32_t irq_mask;
static uint8_t opened;
const void *LogStorage_Slot(unsigned slot)
{
    return slot<LOG_SLOT_COUNT?(const void *)(LOG_FLASH_BASE+slot*LOG_SLOT_BYTES):0;
}
uint8_t LogStorage_Begin(void)
{
    if(opened) return 0;
    irq_mask=Time_SaveIRQ();
    /* Single-bank Flash stalls instruction fetch during erase/program. Never
     * leave ADC/DMA running: both half-frame flags would become pending. */
    LineADC_PauseForLog();
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
    opened=1; return 1;
}
uint8_t LogStorage_Erase(unsigned slot)
{
    unsigned offset;
    if(!opened || slot>=LOG_SLOT_COUNT) return 0;
    for(offset=0;offset<LOG_SLOT_BYTES;offset+=1024U)
        if(FLASH_ErasePage(LOG_FLASH_BASE+slot*LOG_SLOT_BYTES+offset)!=FLASH_COMPLETE) return 0;
    return 1;
}
uint8_t LogStorage_Write(unsigned slot, unsigned offset, const void *data, unsigned bytes)
{
    const uint16_t *p=(const uint16_t *)data;
    unsigned i;
    uint32_t address;
    if(!opened || slot>=LOG_SLOT_COUNT || (offset&1U) || (bytes&1U) ||
       offset>LOG_SLOT_BYTES || bytes>LOG_SLOT_BYTES-offset) return 0;
    address=LOG_FLASH_BASE+slot*LOG_SLOT_BYTES+offset;
    for(i=0;i<bytes;i+=2U) {
        if(FLASH_ProgramHalfWord(address+i,p[i/2U])!=FLASH_COMPLETE ||
           *(const volatile uint16_t *)(address+i)!=p[i/2U]) return 0;
    }
    return 1;
}
void LogStorage_End(void)
{
    if(!opened) return;
    FLASH_Lock(); LineADC_ResumeAfterLog(); opened=0; Time_RestoreIRQ(irq_mask);
}
