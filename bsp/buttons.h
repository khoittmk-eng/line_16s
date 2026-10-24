#ifndef BUTTONS_H
#define BUTTONS_H
#include "stm32f10x.h"
typedef struct { uint8_t pressed, released, short_press, long_press; } ButtonEvents;
void Buttons_Init(void);
void Buttons_Process(uint32_t now, ButtonEvents *events);
#endif
