#ifndef LINE_ESTIMATOR_H
#define LINE_ESTIMATOR_H
#include <stdint.h>
#define LINE_COUNT 8U
typedef struct {
    uint16_t low[LINE_COUNT], high[LINE_COUNT], strength[LINE_COUNT];
    uint16_t white[LINE_COUNT];
    uint32_t samples;
    int16_t error; /* [-3500,3500], 1000 units per channel spacing. */
    uint8_t calibrating, calibrated, valid, mask;
    uint8_t line_is_raw_high;
    uint8_t raw_high_mask; /* Per-channel raw HIGH means black. */
    uint8_t white_reference_valid;
} LineEstimator;
void Line_Init(LineEstimator *line);
void Line_BeginCalibration(LineEstimator *line);
uint8_t Line_EndCalibration(LineEstimator *line);
uint8_t Line_SetWhiteReference(LineEstimator *line, const uint16_t raw[LINE_COUNT]);
void Line_Update(LineEstimator *line, const uint16_t raw[LINE_COUNT]);
#endif
