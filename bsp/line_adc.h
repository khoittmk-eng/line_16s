#ifndef LINE_ADC_H
#define LINE_ADC_H
#include "board.h"
typedef struct {
    uint16_t raw[BOARD_ADC_COUNT];
    uint32_t sequence;
    uint32_t timestamp_ms;
    uint32_t dma_errors;
    uint32_t missed_frames;
} LineFrame;
typedef void (*LineFrameCallback)(const LineFrame *frame);
void LineADC_SetCallback(LineFrameCallback callback);
uint8_t LineADC_Init(void);
void LineADC_GetFrame(LineFrame *frame);
#endif
