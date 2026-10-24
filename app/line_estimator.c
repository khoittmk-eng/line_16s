#include "line_estimator.h"
#define MIN_CONTRAST 200U
void Line_Init(LineEstimator *line)
{
    uint8_t i;
    line->calibrating = line->calibrated = line->valid = line->mask = 0;
    line->samples = 0;
    line->error = 0;
    /* Provisional raw LOW = black. White reference establishes actual polarity. */
    line->line_is_raw_high = 0;
    line->raw_high_mask = 0;
    line->white_reference_valid = 0;
    for (i = 0; i < LINE_COUNT; ++i) {
        line->low[i] = 4095; line->high[i] = line->strength[i] = line->white[i] = 0;
    }
}
void Line_BeginCalibration(LineEstimator *line)
{
    uint8_t polarity = line->line_is_raw_high;
    uint8_t raw_high_mask = line->raw_high_mask;
    Line_Init(line);
    line->line_is_raw_high = polarity;
    line->raw_high_mask = raw_high_mask;
    line->calibrating = 1;
}
uint8_t Line_EndCalibration(LineEstimator *line)
{
    uint8_t i;
    line->calibrating = 0;
    line->calibrated = 0;
    if (line->samples < 50U) return 0;
    for (i = 0; i < LINE_COUNT; ++i)
        if (line->high[i] <= line->low[i] || line->high[i] - line->low[i] < MIN_CONTRAST) return 0;
    line->calibrated = 1;
    return 1;
}
uint8_t Line_SetWhiteReference(LineEstimator *line, const uint16_t raw[LINE_COUNT])
{
    uint8_t i, high_mask = 0;
    if (!line->calibrated || line->calibrating) return 0;
    for (i = 0; i < LINE_COUNT; ++i) {
        uint16_t range = line->high[i] - line->low[i];
        uint16_t white = raw[i];
        if (range < MIN_CONTRAST) return 0;
        /* White must be close to a measured endpoint, not over a line edge. */
        if (white <= line->low[i] + range / 4U) high_mask |= (uint8_t)(1U << i);
        else if (white < line->high[i] - range / 4U) return 0;
    }
    for (i = 0; i < LINE_COUNT; ++i) {
        uint16_t white = raw[i];
        if (white < line->low[i]) white = line->low[i];
        if (white > line->high[i]) white = line->high[i];
        line->white[i] = white;
    }
    line->raw_high_mask = high_mask;
    line->line_is_raw_high = high_mask == 0xFFU;
    line->white_reference_valid = 1;
    line->valid = line->mask = 0;
    return 1;
}
void Line_Update(LineEstimator *line, const uint16_t raw[LINE_COUNT])
{
    uint8_t i, groups = 0, previous = 0;
    uint32_t sum = 0, weighted = 0;
    line->valid = line->mask = 0;
    if (line->calibrating) {
        for (i = 0; i < LINE_COUNT; ++i) {
            if (raw[i] < line->low[i]) line->low[i] = raw[i];
            if (raw[i] > line->high[i]) line->high[i] = raw[i];
        }
        ++line->samples;
        return;
    }
    if (!line->calibrated) return;
    for (i = 0; i < LINE_COUNT; ++i) {
        uint16_t value = raw[i];
        uint16_t strength;
        uint8_t active;
        if (value < line->low[i]) value = line->low[i];
        if (value > line->high[i]) value = line->high[i];
        if (line->white_reference_valid) {
            /* The measured white sample is exactly zero, even with ambient drift. */
            if (line->raw_high_mask & (1U << i)) {
                strength = value <= line->white[i] ? 0 :
                    (uint16_t)(((uint32_t)(value - line->white[i]) * 1000U) /
                               (line->high[i] - line->white[i]));
            } else {
                strength = value >= line->white[i] ? 0 :
                    (uint16_t)(((uint32_t)(line->white[i] - value) * 1000U) /
                               (line->white[i] - line->low[i]));
            }
        } else {
            strength = (uint16_t)(((uint32_t)(value - line->low[i]) * 1000U) /
                                 (line->high[i] - line->low[i]));
            if (!(line->raw_high_mask & (1U << i))) strength = 1000U - strength;
        }
        if (strength < 150U) strength = 0; /* Background noise deadband, including OLED. */
        line->strength[i] = strength;
        active = strength >= 500U;
        if (active) line->mask |= (uint8_t)(1U << i);
        if (active && !previous) ++groups;
        previous = active;
        /* Suppress weak background before computing a centroid. */
        if (strength < 150U) strength = 0;
        sum += strength;
        weighted += (uint32_t)strength * i * 1000U;
    }
    /* Multiple separated groups or full-width line need supervisor handling. */
    if (line->white_reference_valid && sum && line->mask && line->mask != 0xFFU && groups == 1U) {
        line->error = (int16_t)((int32_t)(weighted / sum) - 3500);
        line->valid = 1;
    }
}
