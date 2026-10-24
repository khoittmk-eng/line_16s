#ifndef OLED_H
#define OLED_H
#include "stm32f10x.h"
uint8_t OLED_Init(void);
void OLED_Clear(void);
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void OLED_DrawString(uint8_t x, uint8_t y, const char *text, uint8_t color);
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void OLED_Text(uint8_t row, const char *text);
/* Send one 128-byte page per call, foreground only. Returns 0 on bus failure. */
uint8_t OLED_FlushPage(uint8_t page);
#endif
