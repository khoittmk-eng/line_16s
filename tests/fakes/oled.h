#ifndef FAKE_OLED_H
#define FAKE_OLED_H
#include <stdint.h>
#define BOARD_FIRMWARE_TAG "B09"
void OLED_Clear(void);
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void OLED_DrawString(uint8_t x, uint8_t y, const char *text, uint8_t color);
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
#endif
