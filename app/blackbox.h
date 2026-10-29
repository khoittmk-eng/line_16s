#ifndef BLACKBOX_H
#define BLACKBOX_H
#include "control.h"
#define BLACKBOX_CAPACITY 160U
#define BLACKBOX_PRE_SAMPLES 80U
#define BLACKBOX_POST_SAMPLES 80U
#define BLACKBOX_SAMPLE_MS 4U
typedef enum { LOG_ARMED, LOG_RECORDING, LOG_POST, LOG_READY } BlackboxState;
typedef enum { LOG_TRIGGER_LOSS, LOG_TRIGGER_CORNER } BlackboxTrigger;
typedef struct {
    uint32_t ms;
    uint16_t raw[8];
    int16_t error, left, right, common_x10, target_x10, correction_x10;
    int16_t derivative_x10, planner_x10;
    uint16_t lost_ms, recovery_ms;
    uint8_t mask, valid, mode, reason, fault, stop_reason;
    int8_t side;
    uint8_t flags; /* bit0: shape, bit1: post-turn, bit2: terminal, bit3: trigger, bit4: B23 bridge */
} BlackboxSample;
typedef struct {
    BlackboxState state;
    BlackboxTrigger trigger;
    uint16_t count;
    uint8_t saved, ram_saved;
    uint32_t sequence;
} BlackboxStatus;
void Blackbox_Init(void);
void Blackbox_Arm(BlackboxTrigger trigger);
void Blackbox_Start(uint32_t now, const ControlParameters *p, int8_t steering_sign,
                    const uint16_t *low, const uint16_t *high, const uint16_t *white, uint8_t polarity);
void Blackbox_Record(uint32_t now, const uint16_t *raw, int16_t error, uint8_t mask,
                     uint8_t valid, const ControlState *s);
void Blackbox_Stop(uint32_t now, ControlFault fault);
void Blackbox_GetStatus(BlackboxStatus *out);
uint8_t Blackbox_Save(void);
uint8_t Blackbox_Export(void);
/* Logical oldest-first RAM samples; caller must keep the recorder stopped. */
uint8_t Blackbox_GetSample(unsigned index, BlackboxSample *out);
#endif
