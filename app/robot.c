#include "robot.h"
#include "timebase.h"
#include "motor.h"
static LineEstimator line;
static uint16_t last_raw[LINE_COUNT];
static ControlParameters parameters;
static ControlState controller;
static volatile uint8_t run_requested, capture_fault;
static volatile ControlFault emergency_fault;
static volatile uint32_t last_frame_ms, ui_alive_ms;
static uint32_t frames, max_execution_us, timing_errors, previous_cycle;
static int8_t steering_sign = 1;
static RobotTrace last_run;
static RobotDiagnostics diagnostics;
static void ResetDiagnostics(void)
{
    static const RobotDiagnostics empty={0};
    diagnostics=empty;
}
static void CountEvent(uint16_t *count) { if(*count<65535U) ++*count; }
static void CaptureLastRun(void)
{
    last_run.mode=controller.mode; last_run.error=line.error; last_run.mask=line.mask;
    last_run.left=controller.left; last_run.right=controller.right;
    last_run.timestamp_ms=last_frame_ms; last_run.saturation_ms=controller.saturation_ms;
    last_run.brake_count=controller.brake_count;
    last_run.common_percent=(uint16_t)controller.speed;
    last_run.target_percent=(uint16_t)controller.target;
    last_run.speed_reason=controller.speed_reason;
    last_run.valid=1;
}
void Robot_Init(void)
{
    Line_Init(&line); Control_Defaults(&parameters); Control_Stop(&controller, FAULT_NONE);
    run_requested = 0; capture_fault = 0; emergency_fault = FAULT_NONE;
    last_frame_ms = ui_alive_ms = frames = max_execution_us = timing_errors = previous_cycle = 0;
    steering_sign = 1; last_run.valid=0;
    ResetDiagnostics();
}
void Robot_Stop(ControlFault fault)
{
    uint32_t mask = Time_SaveIRQ();
    if(run_requested && controller.mode!=CONTROL_STOPPED && controller.mode!=CONTROL_FAULT) CaptureLastRun();
    run_requested = 0; emergency_fault = fault;
    Control_Stop(&controller, fault); Motor_StopBrake();
    Time_RestoreIRQ(mask);
}
void Robot_SetParameters(const ControlParameters *p)
{
    uint32_t mask = Time_SaveIRQ(); parameters = *p; parameters.advance_ms = 0; Time_RestoreIRQ(mask);
}
RobotStartReason Robot_GetStartReason(void)
{
    RobotStartReason reason = START_READY;
    uint32_t mask = Time_SaveIRQ();
    if (capture_fault || !frames || (uint32_t)(Time_Millis() - last_frame_ms) > 10U) reason = START_ADC;
    else if (line.calibrating) reason = START_CALIBRATING;
    else if (!line.calibrated) reason = START_NEED_CALIB;
    else if (!line.white_reference_valid) reason = START_NEED_WHITE;
    else if (!line.valid) reason = START_NO_LINE;
    else if (!parameters.base_percent || !parameters.max_percent) reason = START_ZERO_DUTY;
    Time_RestoreIRQ(mask);
    return reason;
}
uint8_t Robot_Start(void)
{
    uint32_t mask = Time_SaveIRQ();
    uint32_t now = Time_Millis();
    uint8_t ready = Robot_GetStartReason() == START_READY;
    if (ready) {
        ResetDiagnostics(); diagnostics.valid=1;
        Control_Start(&controller); emergency_fault = FAULT_NONE;
        ui_alive_ms = now; run_requested = 1;
    }
    Time_RestoreIRQ(mask);
    return ready;
}
uint8_t Robot_SetWhiteReference(void)
{
    uint32_t mask = Time_SaveIRQ();
    uint8_t ok = 0;
    if (!run_requested && !capture_fault && frames && (uint32_t)(Time_Millis() - last_frame_ms) <= 10U)
        ok = Line_SetWhiteReference(&line, last_raw);
    Time_RestoreIRQ(mask);
    return ok;
}
void Robot_OnFrame(const LineFrame *frame)
{
    uint32_t start = Time_Cycles(), interval = start - previous_cycle;
    uint32_t elapsed_us;
    previous_cycle = start;
    last_frame_ms = frame->timestamp_ms;
    ++frames;
    if (frame->dma_errors || frame->missed_frames) {
        capture_fault = 1; Robot_Stop(FAULT_ADC); return;
    }
    {
        uint8_t i;
        for (i = 0; i < LINE_COUNT; ++i) last_raw[i] = frame->raw[i];
    }
    Line_Update(&line, frame->raw);
    if (!run_requested) return;
    {
        uint32_t interval_us=interval/(SystemCoreClock/1000000U);
        if(!diagnostics.min_interval_us || interval_us<diagnostics.min_interval_us) diagnostics.min_interval_us=interval_us;
        if(interval_us>diagnostics.max_interval_us) diagnostics.max_interval_us=interval_us;
    }
    if (emergency_fault != FAULT_NONE) { Robot_Stop(emergency_fault); return; }
    if (interval > (SystemCoreClock / 1000000U) * 4000U ||
        interval < (SystemCoreClock / 1000000U) * 1000U) {
        ++timing_errors; Robot_Stop(FAULT_TIMING); return;
    }
    {
        uint8_t canonical_mask = line.mask;
        ControlMode before_mode=controller.mode;
        uint32_t before_corner=controller.corner_pending_ms;
        int8_t before_side=controller.corner_pending_dir;
        if (steering_sign < 0) {
            uint8_t bit, reversed = 0;
            for (bit = 0; bit < LINE_COUNT	; ++bit)
                if (canonical_mask & (1U << bit)) reversed |= (uint8_t)(1U << (7U - bit));
            canonical_mask = reversed;
        }
        if(line.valid) diagnostics.last_valid_mask=canonical_mask;
        else if(diagnostics.previous_valid) {
            diagnostics.loss_mask=canonical_mask; diagnostics.loss_from=controller.mode;
            diagnostics.loss_side=controller.corner_pending_ms?controller.corner_pending_dir:
                controller.last_side_ms?controller.last_side:controller.post_turn_dir;
        }
        diagnostics.previous_valid=line.valid;
        Control_Step(&controller, &parameters, (int16_t)(line.error * steering_sign), line.valid,
                     canonical_mask, BOARD_CONTROL_PERIOD_MS);
        if(controller.corner_pending_ms && (!before_corner || before_side!=controller.corner_pending_dir)) CountEvent(&diagnostics.corner_events);
        if(controller.mode==CONTROL_GAP && before_mode!=CONTROL_GAP) CountEvent(&diagnostics.gap_entries);
        if((controller.mode==CONTROL_TURN || controller.mode==CONTROL_BRAKE) &&
           before_mode!=CONTROL_TURN && before_mode!=CONTROL_BRAKE) CountEvent(&diagnostics.turn_entries);
    }
    elapsed_us = (Time_Cycles() - start) / (SystemCoreClock / 1000000U);
    if (elapsed_us > max_execution_us) max_execution_us = elapsed_us;
    if (elapsed_us > diagnostics.max_execution_us) diagnostics.max_execution_us = elapsed_us;
    if (elapsed_us > 1000U) { ++timing_errors; Robot_Stop(FAULT_TIMING); return; }
    /* SysTick may have latched a fault while this lower-priority ISR was computing. */
    {
        uint32_t mask = Time_SaveIRQ();
        if (emergency_fault != FAULT_NONE) { Robot_Stop(emergency_fault); }
        else if (controller.mode == CONTROL_FAULT || controller.mode == CONTROL_STOPPED) {
            run_requested = 0; Motor_StopBrake();
        } else {
            Motor_SetDuty(controller.left, controller.right);
            CaptureLastRun();
        }
        Time_RestoreIRQ(mask);
    }
    elapsed_us = (Time_Cycles() - start) / (SystemCoreClock / 1000000U);
    if (elapsed_us > max_execution_us) max_execution_us = elapsed_us;
    if (elapsed_us > diagnostics.max_execution_us) diagnostics.max_execution_us = elapsed_us;
    if (elapsed_us > 1000U) { ++timing_errors; Robot_Stop(FAULT_TIMING); }
}
void Robot_Watchdog(uint32_t now)
{
    if (!run_requested) return;
    if ((uint32_t)(now - last_frame_ms) > 10U) emergency_fault = FAULT_ADC;
    else if ((uint32_t)(now - ui_alive_ms) > 100U) emergency_fault = FAULT_UI;
    if (emergency_fault != FAULT_NONE) Motor_StopBrake();
}
void Robot_UIAlive(uint32_t now) { ui_alive_ms = now; }
void Robot_GetLastRun(RobotTrace *out)
{
    uint32_t mask=Time_SaveIRQ(); *out=last_run; Time_RestoreIRQ(mask);
}
void Robot_GetDiagnostics(RobotDiagnostics *out)
{
    uint32_t mask=Time_SaveIRQ(); *out=diagnostics; Time_RestoreIRQ(mask);
}
void Robot_GetLine(LineEstimator *out)
{
    uint32_t mask = Time_SaveIRQ(); *out = line; Time_RestoreIRQ(mask);
}
void Robot_GetStatus(RobotStatus *out)
{
    uint32_t mask = Time_SaveIRQ();
    out->mode = emergency_fault == FAULT_NONE ? controller.mode : CONTROL_FAULT;
    out->fault = emergency_fault == FAULT_NONE ? controller.fault : emergency_fault;
    out->left = emergency_fault == FAULT_NONE ? controller.left : 0;
    out->right = emergency_fault == FAULT_NONE ? controller.right : 0;
    out->frames = frames; out->max_execution_us = max_execution_us;
    out->timing_errors = timing_errors; out->steering_sign = steering_sign;
    out->line_stop_reason = controller.line_stop_reason;
    out->brake_count = controller.brake_count;
    out->common_percent = (uint16_t)controller.speed;
    out->target_percent = (uint16_t)controller.target;
    out->speed_reason = controller.speed_reason;
    Time_RestoreIRQ(mask);
}
void Robot_BeginCalibration(void)
{
    uint32_t mask = Time_SaveIRQ();
    Robot_Stop(FAULT_NONE); Line_BeginCalibration(&line); Time_RestoreIRQ(mask);
}
uint8_t Robot_EndCalibration(void)
{
    uint32_t mask = Time_SaveIRQ(); uint8_t ok = Line_EndCalibration(&line);
    Time_RestoreIRQ(mask); return ok;
}
void Robot_CancelCalibration(void)
{
    uint32_t mask = Time_SaveIRQ();
    if (line.calibrating) { line.calibrating = 0; line.calibrated = 0; }
    Time_RestoreIRQ(mask);
}
void Robot_TogglePolarity(void)
{
    uint32_t mask = Time_SaveIRQ();
    Robot_Stop(FAULT_NONE); line.raw_high_mask ^= 0xFFU;
    line.line_is_raw_high = line.raw_high_mask == 0xFFU;
    {
        uint8_t i;
        for (i = 0; i < LINE_COUNT; ++i)
            line.white[i] = (line.raw_high_mask & (1U << i)) ? line.low[i] : line.high[i];
    }
    /* An explicit manual inversion is an override, not automatic white detection. */
    line.white_reference_valid = line.calibrated;
    line.valid = 0; line.mask = 0;
    Time_RestoreIRQ(mask);
}
void Robot_ToggleSteering(void)
{
    uint32_t mask = Time_SaveIRQ();
    Robot_Stop(FAULT_NONE); steering_sign = -steering_sign;
    Time_RestoreIRQ(mask);
}
