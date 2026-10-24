#ifndef FAKE_TIMEBASE_H
#define FAKE_TIMEBASE_H
#include <stdint.h>
#define BOARD_CONTROL_PERIOD_MS 2U
extern uint32_t SystemCoreClock;
uint32_t Time_Millis(void);
uint32_t Time_Cycles(void);
uint32_t Time_SaveIRQ(void);
void Time_RestoreIRQ(uint32_t mask);
#endif
