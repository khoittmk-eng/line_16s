#ifndef ROBOT_H
#define ROBOT_H
#include "control.h"
#include "line_estimator.h"
#include "line_adc.h"
typedef struct {
    ControlMode mode; ControlFault fault;
    int16_t left, right;
    uint32_t frames, max_execution_us, timing_errors;
    int8_t steering_sign;
    LineStopReason line_stop_reason;
    uint16_t brake_count;
    uint16_t common_percent, target_percent;
    SpeedReason speed_reason;
} RobotStatus;
typedef struct {
    ControlMode mode;
    int16_t error, left, right;
    uint8_t mask, valid;
    uint32_t timestamp_ms, saturation_ms;
    uint16_t brake_count;
    uint16_t common_percent, target_percent;
    SpeedReason speed_reason;
} RobotTrace;
void Robot_GetLastRun(RobotTrace *trace);
typedef struct {
    uint32_t min_interval_us, max_interval_us, max_execution_us;
    uint32_t mode_ms[CONTROL_ENTRY_BRAKE+1], curve_ms, run_ms, inner_zero_ms;
    uint16_t corner_events, gap_entries, turn_entries;
    uint16_t inner_zero_events, entry_brake_events;
    uint8_t previous_inner_zero;
    uint8_t valid, last_valid_mask, loss_mask, previous_valid;
    ControlMode loss_from;
    int8_t loss_side;
} RobotDiagnostics;
void Robot_GetDiagnostics(RobotDiagnostics *out);
typedef enum { START_READY, START_ADC, START_CALIBRATING, START_NEED_CALIB,
               START_NEED_WHITE, START_NO_LINE, START_ZERO_DUTY } RobotStartReason;
RobotStartReason Robot_GetStartReason(void);
uint8_t Robot_SetWhiteReference(void);
void Robot_Init(void);
void Robot_OnFrame(const LineFrame *frame);
void Robot_GetLine(LineEstimator *line);
void Robot_GetStatus(RobotStatus *status);
void Robot_SetParameters(const ControlParameters *parameters);
uint8_t Robot_Start(void);
void Robot_Stop(ControlFault fault);
void Robot_Watchdog(uint32_t now);
void Robot_UIAlive(uint32_t now);
void Robot_BeginCalibration(void);
uint8_t Robot_EndCalibration(void);
void Robot_CancelCalibration(void);
void Robot_TogglePolarity(void);
void Robot_ToggleSteering(void);
#endif
