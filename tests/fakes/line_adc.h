#ifndef FAKE_LINE_ADC_H
#define FAKE_LINE_ADC_H
#include <stdint.h>
typedef struct {
    uint16_t raw[8];
    uint32_t sequence, timestamp_ms, dma_errors, missed_frames;
} LineFrame;
void LineADC_PauseForLog(void);
void LineADC_ResumeAfterLog(void);
#endif
