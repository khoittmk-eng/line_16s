#include "control.h"
#include "line_estimator.h"
volatile unsigned test_count, failure_line;
__attribute__((noinline)) void test_done(void) { while (1) {} }
static void Check(int condition, unsigned line)
{
    ++test_count;
    if (!condition) { failure_line = line; test_done(); }
}
#define CHECK(x) Check((x), __LINE__)
static void Calibrate(LineEstimator *line)
{
    unsigned i,j; uint16_t raw[8];
    Line_Init(line); Line_BeginCalibration(line);
    for (j = 0; j < 60; ++j) {
        for (i = 0; i < 8; ++i) raw[i] = (j & 1) ? 3800 : 200;
        Line_Update(line, raw);
    }
    CHECK(Line_EndCalibration(line));
    for (i = 0; i < 8; ++i) raw[i] = 200;
    CHECK(Line_SetWhiteReference(line, raw));
}
void RunRobotTests(void);
void RunMenuTests(void);
void RunRecoveryTests(void);
void RunSpeedTests(void);
void RunMotorTests(void);
void RunBlackboxTests(void);
void RunLogDriverTests(void);
void RunAmbiguousTests(void);
void RunDirectTurnTests(void);
void RunFixedBaseTests(void);
void RunCornerFlowTests(void);
void RunArcTests(void);
void RunScheduledTests(void);
void RunCornerReturnTests(void);
void RunBoardTests(void);
void test_entry(void)
{
    LineEstimator line;
    ControlParameters p;
    ControlState state;
    uint16_t raw[8];
    unsigned i,j;
    int error;
    Line_Init(&line);
    CHECK(line.line_is_raw_high == 0);
    Line_BeginCalibration(&line);
    for (i = 0; i < 8; ++i) raw[i] = 200;
    for (i = 0; i < 60; ++i) Line_Update(&line, raw);
    CHECK(!Line_EndCalibration(&line)); /* Reject no contrast. */
    Calibrate(&line);
    Line_Update(&line, raw);
    CHECK(line.mask == 0 && !line.valid);
    raw[3] = raw[4] = 3800;
    Line_Update(&line, raw);
    CHECK(line.valid && line.mask == 0x18 && line.error == 0);
    CHECK(line.strength[3] == 1000 && line.strength[0] == 0);
    raw[3] = raw[4] = 200; raw[0] = 3800;
    Line_Update(&line, raw);
    CHECK(line.valid && line.error == -3500);
    raw[7] = 3800;
    Line_Update(&line, raw);
    CHECK(!line.valid); /* Two disconnected branches must not appear centered. */
    for (i = 0; i < 8; ++i) raw[i] = 3800;
    Line_Update(&line, raw);
    CHECK(line.mask == 0xFF && !line.valid);
    /* Actual reported orientation: raw HIGH white, raw LOW black. */
    CHECK(Line_SetWhiteReference(&line, raw));
    Line_Update(&line, raw);
    CHECK(line.mask == 0 && !line.valid && line.strength[0] == 0);
    raw[3] = raw[4] = 200;
    Line_Update(&line, raw);
    CHECK(line.valid && line.mask == 0x18 && line.error == 0);
    CHECK(line.strength[3] == 1000 && line.strength[0] == 0);
    /* Reference white need not exactly match the sweep endpoint. */
    Calibrate(&line);
    for (i = 0; i < 8; ++i) raw[i] = 3400;
    CHECK(Line_SetWhiteReference(&line, raw));
    Line_Update(&line, raw);
    CHECK(line.mask == 0 && line.strength[0] == 0);
    raw[3] = raw[4] = 200;
    Line_Update(&line, raw);
    CHECK(line.valid && line.strength[3] == 1000 && line.strength[0] == 0);
    /* White-reference validation refuses ambiguous mid-range samples. */
    Calibrate(&line);
    for (i = 0; i < 8; ++i) raw[i] = 2000;
    CHECK(!Line_SetWhiteReference(&line, raw));
    Control_Defaults(&p);
    CHECK(p.base_percent==45 && p.max_percent==85);
    CHECK(p.straight_percent==45);
    CHECK(p.kp==12.0f && p.ki==0.0f && p.kd==0.12f);
    /* Keep the original low-duty regression sequence independent of new trial defaults. */
    p.base_percent=20; p.straight_percent=20; p.max_percent=30;
    Control_Start(&state);
    Control_Step(&state, &p, 0, 1, 0x18, 2);
    CHECK(state.left == state.right && state.left <= 1);
    for (i = 0; i < 1500; ++i) Control_Step(&state, &p, 0, 1, 0x18, 2);
    CHECK(state.left == 200 && state.right == 200); /* Base reached; Max is a ceiling only. */
    Control_Step(&state, &p, 1000, 1, 0x10, 2);
    CHECK(state.left > state.right && state.left <= 300 && state.right >= 0);
    Control_Start(&state);
    for (i = 0; i < 500; ++i) Control_Step(&state, &p, 0, 1, 0x18, 2);
    Control_Step(&state, &p, 0, 0, 0, 2);
    CHECK(state.mode == CONTROL_GAP && state.left > 0 && state.right > 0);
    Control_Step(&state, &p, 0, 1, 0x18, 2);
    CHECK(state.mode == CONTROL_TRACK && state.derivative == 0);
    Control_Start(&state);
    for (i = 0; i < 200; ++i) Control_Step(&state, &p, 2000, 1, 0x40, 2);
    Control_Step(&state, &p, 2000, 0, 0, 2);
    CHECK(state.mode == CONTROL_PROBE);
    for (i = 0; i < 50; ++i) Control_Step(&state, &p, 2000, 0, 0, 2);
    CHECK(state.mode == CONTROL_TURN && state.left > 0 && state.right < 0);
    for (i = 0; i < 700; ++i) Control_Step(&state, &p, 2000, 0, 0, 2);
    CHECK(state.mode == CONTROL_FAULT && !state.left && !state.right);
    Control_Start(&state);
    Control_Step(&state, &p, 0, 1, 0x18, 2);
    Control_Step(&state, &p, 0, 0, 0xFF, 2);
    CHECK(state.mode == CONTROL_PROBE);
    for (i = 0; i < 40; ++i) Control_Step(&state, &p, 0, 0, 0xFF, 2);
    CHECK(state.mode == CONTROL_FAULT && state.line_stop_reason == LINE_STOP_AMBIGUOUS);
    Control_Start(&state);
    Control_Step(&state, &p, 0, 1, 0x18, 10);
    CHECK(state.mode == CONTROL_FAULT && state.fault == FAULT_TIMING);
    for (error = -3500; error <= 3500; error += 250) {
        Control_Start(&state);
        for (j = 0; j < 400; ++j) {
            Control_Step(&state, &p, error, 1, 1, 2);
            CHECK(state.left >= -300 && state.right >= -300 && state.left <= 300 && state.right <= 300);
            if (state.mode == CONTROL_TRACK) CHECK(state.left >= 0 && state.right >= 0);
        }
    }
    p.ki = 1.0f;
    Control_Start(&state);
    for (i = 0; i < 2000; ++i) Control_Step(&state, &p, 1500, 1, 0x30, 2);
    CHECK(state.integral <= 10.0f && state.integral >= -10.0f);
    Control_Stop(&state, FAULT_NONE);
    CHECK(state.mode == CONTROL_STOPPED && !state.left && !state.right);
    RunCornerReturnTests();
    RunScheduledTests();
    RunArcTests();
    RunCornerFlowTests();
    RunFixedBaseTests();
    RunDirectTurnTests();
    RunAmbiguousTests();
    RunRecoveryTests();
    RunSpeedTests();
    RunRobotTests();
    RunMenuTests();
    RunBlackboxTests();
    RunMotorTests();
    RunLogDriverTests();
    RunBoardTests();
    test_done();
}
