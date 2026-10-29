#include "robot.h"
#include "timebase.h"
#include "control_tuning.h"
extern volatile unsigned test_count, failure_line;
void test_done(void);
uint32_t SystemCoreClock = 72000000;
static uint32_t now_ms;
static int16_t motor_left, motor_right;
uint32_t Time_Millis(void) { return now_ms; }
uint32_t Time_Cycles(void) { return now_ms * 72000U; }
uint32_t Time_SaveIRQ(void) { return 0; }
void Time_RestoreIRQ(uint32_t mask) { (void)mask; }
void Motor_StopBrake(void) { motor_left = motor_right = 0; }
void Motor_SetDuty(int16_t left, int16_t right) { motor_left = left; motor_right = right; }
static void Check(int ok, unsigned line)
{
    ++test_count;
    if (!ok) { failure_line = 10000U + line; test_done(); }
}
#define CHECK(x) Check((x), __LINE__)
void TestRobot_Emit(uint8_t calibration_sample, uint8_t adc_error)
{
    LineFrame frame;
    unsigned i;
    now_ms += 2U;
    for (i = 0; i < 8; ++i) frame.raw[i] = calibration_sample == 2 ?
        ((i == 3 || i == 4) ? 3800 : 200) : calibration_sample ? 3800 : 200;
    frame.sequence = now_ms / 2;
    frame.timestamp_ms = now_ms;
    frame.dma_errors = adc_error;
    frame.missed_frames = 0;
    Robot_OnFrame(&frame);
}
void TestRobot_EmitMask(uint8_t mask)
{
    LineFrame frame;
    unsigned i;
    now_ms += 2U;
    for (i = 0; i < 8; ++i) frame.raw[i] = (mask & (1U << i)) ? 3800 : 200;
    frame.sequence = now_ms / 2U; frame.timestamp_ms = now_ms;
    frame.dma_errors = frame.missed_frames = 0;
    Robot_OnFrame(&frame);
}
void TestRobot_Prepare(void)
{
    unsigned i;
    now_ms = 0; motor_left = motor_right = 0;
    Robot_Init();
    CHECK(!Robot_Start());
    Robot_BeginCalibration();
    for (i = 0; i < 60; ++i) TestRobot_Emit(i & 1, 0);
    CHECK(Robot_EndCalibration());
    CHECK(Robot_GetStartReason() == START_NEED_WHITE);
    CHECK(!Robot_Start());
    TestRobot_Emit(0, 0);
    CHECK(Robot_SetWhiteReference());
    TestRobot_Emit(2, 0);
    Robot_UIAlive(now_ms);
    CHECK(Robot_Start());
}
void TestRobot_LoseMask(uint8_t mask)
{
    LineFrame frame;
    unsigned i;
    now_ms+=2;
    for(i=0;i<8;++i) frame.raw[i]=(mask&(1U<<i))?3800:200;
    frame.sequence=now_ms/2; frame.timestamp_ms=now_ms; frame.dma_errors=frame.missed_frames=0;
    Robot_OnFrame(&frame);
}
void RunRobotTests(void)
{
    RobotStatus status;
    unsigned i;
    /* Per-wheel zero requests are separate from full entry braking. */
    {
        RobotDiagnostics d,frozen;
        TestRobot_Prepare();
        for(i=0;i<150;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0x80); }
        Robot_GetDiagnostics(&d);
        CHECK(d.inner_zero_events==1 && d.inner_zero_ms>0 && !d.entry_brake_events);
        Robot_Stop(FAULT_NONE); Robot_GetDiagnostics(&frozen);
        TestRobot_EmitMask(0x18); Robot_GetDiagnostics(&d);
        CHECK(d.inner_zero_events==frozen.inner_zero_events && d.inner_zero_ms==frozen.inner_zero_ms);
        CHECK(Robot_Start()); Robot_GetDiagnostics(&d);
        CHECK(!d.inner_zero_events && !d.inner_zero_ms && !d.entry_brake_events);
        for(i=0;i<600;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0x18); }
        for(i=0;i<3;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0xF8); }
        Robot_GetStatus(&status);
        CHECK(status.mode==CONTROL_TRACK && motor_left>0 && motor_right>0);
        now_ms+=11; Robot_Watchdog(now_ms); Robot_GetStatus(&status);
        CHECK(status.mode==CONTROL_FAULT && status.fault==FAULT_ADC && !motor_left && !motor_right);
        TestRobot_EmitMask(0x18); Robot_GetStatus(&status);
        CHECK(status.mode==CONTROL_FAULT && !motor_left && !motor_right);
    }
    /* Actual estimator masks -> shape memory -> early recovery, with frozen diagnostics. */
    {
        RobotDiagnostics d, frozen;
        TestRobot_Prepare();
        for(i=0;i<600;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0x18); }
        for(i=0;i<3;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0xF8); }
        Robot_GetStatus(&status);
        CHECK(status.mode==CONTROL_TRACK && status.speed_reason==SPEED_CORNER && status.target_percent==45);
        CHECK(motor_left>motor_right && motor_right>0 && status.brake_count==0);
        for(i=0;i<6;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0x18); }
        for(i=0;i<3;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0); }
        Robot_GetStatus(&status); Robot_GetDiagnostics(&d);
        CHECK(status.mode==CONTROL_TURN && status.brake_count==0);
        CHECK(d.valid && d.corner_events==1 && d.gap_entries==0 && d.turn_entries==1);
        CHECK(d.loss_from==CONTROL_TRACK && d.loss_mask==0 && d.last_valid_mask==0x18 && d.loss_side==1);
        CHECK(d.min_interval_us==2000 && d.max_interval_us==2000);
        CHECK(d.run_ms==1224 && d.mode_ms[CONTROL_TRACK]==1218 && d.mode_ms[CONTROL_PROBE]==0);
        CHECK(d.entry_brake_events==0 && d.mode_ms[CONTROL_ENTRY_BRAKE]==0);
        CHECK(d.mode_ms[CONTROL_BRAKE]==0 && d.mode_ms[CONTROL_TURN]==6);
        for(i=0;i<650;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0); }
        Robot_GetStatus(&status); Robot_GetDiagnostics(&frozen);
        CHECK(status.fault==FAULT_LINE && status.line_stop_reason==LINE_STOP_TURN_TIMEOUT);
        TestRobot_EmitMask(0x80); Robot_GetDiagnostics(&d);
        CHECK(d.last_valid_mask==frozen.last_valid_mask && d.turn_entries==frozen.turn_entries && d.loss_side==frozen.loss_side);
        CHECK(d.run_ms==frozen.run_ms && d.mode_ms[CONTROL_TURN]==frozen.mode_ms[CONTROL_TURN]);
        CHECK(d.entry_brake_events==frozen.entry_brake_events && d.inner_zero_ms==frozen.inner_zero_ms);
        TestRobot_EmitMask(0x18); CHECK(Robot_Start()); Robot_GetDiagnostics(&d);
        CHECK(d.valid && !d.corner_events && !d.turn_entries && !d.max_interval_us);
        CHECK(!d.run_ms && !d.curve_ms && !d.mode_ms[CONTROL_TRACK]);
        CHECK(!d.entry_brake_events && !d.inner_zero_ms && !d.inner_zero_events);
        now_ms+=5; TestRobot_EmitMask(0x18);
        Robot_GetStatus(&status); Robot_GetDiagnostics(&d);
        CHECK(status.fault==FAULT_TIMING && d.max_interval_us==7000);
    }
    TestRobot_Prepare();
    for (i = 0; i < 100; ++i) { Robot_UIAlive(now_ms); TestRobot_Emit(2, 0); }
    CHECK(motor_left > 0 && motor_right > 0);
    Robot_Stop(FAULT_NONE);
    TestRobot_Emit(2, 0);
    CHECK(!motor_left && !motor_right);
    {
        RobotTrace trace;
        Robot_GetLastRun(&trace);
        CHECK(trace.valid && trace.left>0 && trace.right>0);
    }
    Robot_GetStatus(&status);
    CHECK(status.mode == CONTROL_STOPPED);
    /* Estimator -> controller -> PWM: a central eye tracks at fixed Base, not Max. */
    TestRobot_Prepare();
    for(i=0;i<800;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0x10); }
    Robot_GetStatus(&status);
    CHECK(status.mode==CONTROL_TRACK && status.speed_reason==SPEED_BASE);
    CHECK(status.common_percent==45 && status.target_percent==45 && !status.brake_count);
    Robot_Stop(FAULT_NONE);
    {
        RobotTrace trace;
        Robot_GetLastRun(&trace);
        CHECK(trace.valid && trace.speed_reason==SPEED_BASE && trace.target_percent==45 && trace.common_percent==45);
    }
    TestRobot_Prepare();
    TestRobot_Emit(2, 0);
    now_ms += 11;
    Robot_Watchdog(now_ms);
    CHECK(!motor_left && !motor_right);
    Robot_GetStatus(&status);
    CHECK(status.mode == CONTROL_FAULT && status.fault == FAULT_ADC);
    TestRobot_Emit(2, 0);
    CHECK(!motor_left && !motor_right); /* Watchdog fault cannot be overwritten by a later callback. */
    TestRobot_Prepare();
    for (i = 0; i < 60; ++i) { TestRobot_Emit(2, 0); Robot_Watchdog(now_ms); }
    Robot_GetStatus(&status);
    CHECK(!motor_left && !motor_right && status.fault == FAULT_UI);
    TestRobot_Prepare();
    TestRobot_Emit(2, 1);
    TestRobot_Emit(2, 0);
    CHECK(!Robot_Start()); /* A stopped DMA acquisition needs reset, not a rapid retry. */
    CHECK(!motor_left && !motor_right);
    TestRobot_Prepare();
    now_ms += 5;
    TestRobot_Emit(2, 0);
    Robot_GetStatus(&status);
    CHECK(status.mode == CONTROL_FAULT && status.fault == FAULT_TIMING);
    CHECK(!motor_left && !motor_right);
    TestRobot_Prepare();
    Robot_Stop(FAULT_NONE); Robot_ToggleSteering();
    CHECK(Robot_Start());
    for (i = 0; i < 300; ++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0x18); }
    for (i = 0; i < 300; ++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0xF8); }
    Robot_GetStatus(&status);
    CHECK(status.mode == CONTROL_TRACK && status.steering_sign == -1);
    CHECK(motor_left>=0 && motor_right>=0);
    for(i=0;i<3;++i) { Robot_UIAlive(now_ms); TestRobot_LoseMask(0); }
    for(i=0;i<100;++i) { Robot_UIAlive(now_ms); TestRobot_LoseMask(0); }
    Robot_GetStatus(&status);
    CHECK(status.mode==CONTROL_TURN && motor_left<0 && motor_right>0);
    CHECK(motor_left==-500 && motor_right==500);
    Robot_Stop(FAULT_NONE);
    /* A direct high-duty turn is active RUN: preserve trace and allow explicit Stop. */
    {
        ControlParameters p;
        RobotTrace trace;
        TestRobot_Prepare(); Control_Defaults(&p); p.max_percent=90; Robot_SetParameters(&p);
        for(i=0;i<1500;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0x18); }
        Robot_UIAlive(now_ms); TestRobot_EmitMask(0x60);
        for(i=0;i<3;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0); }
        Robot_GetStatus(&status);
        CHECK(status.mode==CONTROL_TURN && !status.brake_count && motor_left>motor_right);
        Robot_Stop(FAULT_NONE); Robot_GetLastRun(&trace);
        CHECK(trace.valid && trace.mode==CONTROL_TURN && !trace.brake_count);
        CHECK(trace.speed_reason==SPEED_CORNER && trace.target_percent>=35 && trace.target_percent<=45);
        TestRobot_EmitMask(0x18); Robot_GetStatus(&status);
        CHECK(status.mode==CONTROL_STOPPED && !motor_left && !motor_right);
    }
    /* Sensor endpoint -> white -> black: motor requests remain active without a new Start. */
    TestRobot_Prepare();
    for(i=0;i<500;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0x18); }
    Robot_UIAlive(now_ms); TestRobot_EmitMask(0x10);
    {
        int16_t left=motor_left,right=motor_right;
        for(i=0;i<120;++i) {
            Robot_UIAlive(now_ms); TestRobot_EmitMask(0);
            if(i==0) { left=motor_left; right=motor_right; } /* Freeze filtered GAP pair. */
            CHECK(motor_left==left && motor_right==right);
            CHECK(motor_left>0 && motor_right>0);
        }
        Robot_GetStatus(&status);
        CHECK(status.mode==CONTROL_GAP && status.fault==FAULT_NONE && !status.brake_count);
        Robot_UIAlive(now_ms); TestRobot_EmitMask(0x18); Robot_GetStatus(&status);
        CHECK(status.mode==CONTROL_TRACK && motor_left>0 && motor_right>0 && status.target_percent==45);
    }
    /* Watchdogs still stop a held command during a gap, and the fault stays latched. */
    Robot_UIAlive(now_ms); TestRobot_EmitMask(0);
    now_ms+=11; Robot_Watchdog(now_ms); Robot_GetStatus(&status);
    CHECK(status.fault==FAULT_ADC && !motor_left && !motor_right);
    TestRobot_EmitMask(0x18);
    CHECK(!motor_left && !motor_right);
    TestRobot_Prepare();
    for(i=0;i<500;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0x18); }
    for(i=0;i<CONTROL_GAP_MAX_MS/2U+1U;++i) { Robot_UIAlive(now_ms); TestRobot_EmitMask(0); }
    Robot_GetStatus(&status);
    CHECK(status.fault==FAULT_LINE && status.line_stop_reason==LINE_STOP_GAP_TIMEOUT && !motor_left && !motor_right);
    Robot_UIAlive(now_ms); TestRobot_EmitMask(0x18); Robot_GetStatus(&status);
    CHECK(status.mode==CONTROL_FAULT && !motor_left && !motor_right);
}
