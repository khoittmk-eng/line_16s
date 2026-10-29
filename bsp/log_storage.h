#ifndef LOG_STORAGE_H
#define LOG_STORAGE_H
#include <stdint.h>
#ifndef LOG_FLASH_BASE
#define LOG_FLASH_BASE 0x0800C000UL
#endif
#define LOG_SLOT_BYTES 8192U
#define LOG_SLOT_COUNT 2U
/* Only the dedicated final 16 KiB may be erased or programmed. */
const void *LogStorage_Slot(unsigned slot);
uint8_t LogStorage_Begin(void);
uint8_t LogStorage_Erase(unsigned slot);
uint8_t LogStorage_Write(unsigned slot, unsigned offset, const void *data, unsigned bytes);
void LogStorage_End(void);
#endif
