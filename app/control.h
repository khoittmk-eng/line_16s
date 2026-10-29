#ifndef CONTROL_H
#define CONTROL_H
#include <stdint.h>
/* advance_ms is ignored. straight_percent is legacy CSV metadata, not a speed target. */
typedef struct { float kp, ki, kd; uint16_t base_percent, straight_percent, max_percent; uint16_t advance_ms, align_ms; } ControlParameters;
/* ADVANCE and ENTRY_BRAKE are reserved legacy values; ALIGN now means sensor-based RECENTER. */
typedef enum { CONTROL_STOPPED, CONTROL_TRACK, CONTROL_GAP, CONTROL_SEARCH, CONTROL_FAULT, CONTROL_TURN, CONTROL_PROBE, CONTROL_ADVANCE, CONTROL_ALIGN, CONTROL_BRAKE, CONTROL_ENTRY_BRAKE } ControlMode;
typedef enum { FAULT_NONE, FAULT_ADC, FAULT_LINE, FAULT_TIMING, FAULT_UI, FAULT_OLED } ControlFault;
typedef enum { LINE_STOP_NONE, LINE_STOP_GAP_TIMEOUT, LINE_STOP_TURN_TIMEOUT, LINE_STOP_AMBIGUOUS, LINE_STOP_NO_HISTORY, LINE_STOP_SATURATED } LineStopReason;
typedef enum { SPEED_IDLE, SPEED_BASE, SPEED_BOOST, SPEED_CURVE, SPEED_WAIT, SPEED_GAP, SPEED_RECOVERY, SPEED_BRAKE, SPEED_PROBE, SPEED_CORNER } SpeedReason;
typedef struct {
    ControlMode mode;
    ControlFault fault;
    float speed, integral, derivative, last_error, last_correction;
    float target, planner_rate, speed_bonus; /* speed_bonus/cruise_ms reserved, zero in B25. */
    uint32_t cruise_ms, speed_guard_ms;
    uint32_t curve_age_ms;
    int16_t curve_left, curve_right;
    uint8_t curve_valid;
    SpeedReason speed_reason;
    float saturation_error;
    uint32_t stable_ms, lost_ms, recovery_ms, reacquire_ms, ambiguous_ms;
    uint32_t corner_hint_ms, saturation_ms, split_cue_ms;
    uint32_t brake_ms, guide_age_ms, last_side_ms, post_turn_ms;
    uint32_t shape_ms, corner_pending_ms;
    uint16_t brake_count;
    int8_t corner_dir, turn_dir, split_cue_dir, last_side, post_turn_dir;
    int8_t shape_dir, corner_pending_dir;
    LineStopReason line_stop_reason;
    int16_t left, right;
    int16_t guide_left, guide_right;
    uint8_t history, lost, may_cross_gap, guide_valid, recovery_seen_line;
    uint8_t probe_continue;
    float recovery_arc_speed;
    float gap_correction;
    uint8_t recovery_arc;
    float corner_gain;
    uint8_t search_near_line;
    uint16_t corner_inward_ms, corner_outward_ms;
    uint8_t corner_releasing;
} ControlState;
void Control_Defaults(ControlParameters *p);
void Control_Stop(ControlState *s, ControlFault fault);
void Control_Start(ControlState *s);
void Control_Step(ControlState *s, const ControlParameters *p, int16_t error, uint8_t valid, uint8_t mask, uint32_t dt_ms);
#endif
