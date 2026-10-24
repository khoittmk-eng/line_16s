#ifndef TIMEBASE_H
#define TIMEBASE_H
#include "stm32f10x.h"
void Time_Init(void);
uint32_t Time_Millis(void);
uint32_t Time_Cycles(void);
uint32_t Time_SaveIRQ(void);
void Time_RestoreIRQ(uint32_t mask);
#endif
