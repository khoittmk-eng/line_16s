#include "menu.h"
#include "robot.h"
#include "timebase.h"
#include "blackbox.h"
#include <stddef.h>
extern volatile unsigned test_count, failure_line;
void test_done(void);
void TestRobot_Prepare(void);
void TestRobot_Emit(uint8_t sample, uint8_t error);
/* The bare emulation entry bypasses ARM C runtime locale initialization.
 * Test build renames snprintf to TestSnprintf; formatting itself is not under test. */
int snprintf(char *out, size_t size, const char *format, ...)
{
    size_t i = 0;
    while (format[i]) {
        if (i + 1U < size) out[i] = format[i];
        ++i;
    }
    if (size) out[i < size ? i : size - 1U] = 0;
    return (int)i;
}
static uint8_t visible_speeds, visible_last, visible_planner, visible_fault, visible_place, visible_diag, visible_time, visible_steer;
static int StartsWith(const char *s, const char *prefix)
{
    while (*prefix) if (*s++ != *prefix++) return 0;
    return 1;
}
void OLED_Clear(void) {}
uint8_t OLED_FlushPage(uint8_t page) { (void)page; return 1; }
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t c) { (void)x; (void)y; (void)c; }
void OLED_DrawString(uint8_t x, uint8_t y, const char *s, uint8_t c)
{
    (void)x; (void)c;
    if(StartsWith(s,"LINE LOST")) visible_fault=1;
    if(StartsWith(s,"PLACE ON BLACK LINE")) visible_place=1;
    if(StartsWith(s,"LAST RUN")) visible_last=1;
    if(StartsWith(s,"DIAG ")) visible_diag=1;
    if(StartsWith(s,"TIME ms ")) visible_time=1;
    if(StartsWith(s,"STEER ")) visible_steer=1;
    if(StartsWith(s,"V%u T%u") && y==50U) visible_planner=1;
    if (StartsWith(s, "Base:") && y == 31U) visible_speeds |= 1U;
    if (StartsWith(s, "Max :") && y == 38U) visible_speeds |= 2U;
    if (StartsWith(s, "Straight:")) visible_speeds |= 4U;
}
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t c) { (void)x; (void)y; (void)w; (void)h; (void)c; }
static void Check(int ok, unsigned line)
{
    ++test_count;
    if (!ok) { failure_line = 20000U + line; test_done(); }
}
#define CHECK(x) Check((x), __LINE__)
static void Send(uint8_t short_press, uint8_t pressed, uint8_t long_press)
{
    ButtonEvents e;
    LineEstimator line;
    e.pressed = pressed; e.short_press = short_press; e.long_press = long_press; e.released = 0;
    Robot_GetLine(&line);
    Robot_UIAlive(Time_Millis());
    Menu_Process(&e, &line, 0, 1, Time_Millis());
}
static ControlMode Mode(void) { RobotStatus status; Robot_GetStatus(&status); return status.mode; }
void RunMenuTests(void)
{
    unsigned i;
    LineEstimator line;
    TestRobot_Prepare();
    Robot_Stop(FAULT_NONE);
    Menu_Init();
    Send(1, 0, 0); /* N1 is navigation, not Enter. */
    Send(2, 0, 0);
    CHECK(currentMenu == MENU_SENSOR && Mode() == CONTROL_STOPPED);
    Menu_Init();
    Send(2, 0, 0); /* N2 enters default item 1 Run. */
    CHECK(currentMenu == MENU_RUN);
    Send(4, 0, 0);
    CHECK(Mode() == CONTROL_TRACK);
    {
        LineFrame frame={{0},0,0,0,0};
        Robot_GetLine(&line); visible_planner=0; Menu_Draw(&line,&frame,0);
        CHECK(visible_planner);
    }
    Send(0, 4, 0);
    CHECK(Mode() == CONTROL_STOPPED);
    Send(4, 0, 0); /* Release after Stop must not restart. */
    CHECK(Mode() == CONTROL_STOPPED);
    {
        LineFrame frame={{0},0,0,0,0};
        Send(1,0,0); Robot_GetLine(&line); visible_last=0;
        Menu_Draw(&line,&frame,0);
        CHECK(visible_last && Mode()==CONTROL_STOPPED);
        Send(1,0,0);
        visible_diag=0; Menu_Draw(&line,&frame,0);
        CHECK(visible_diag && Mode()==CONTROL_STOPPED);
        Send(1,0,0);
        visible_time=0; Menu_Draw(&line,&frame,0);
        CHECK(visible_time && Mode()==CONTROL_STOPPED);
        Send(1,0,0);
        visible_steer=0; Menu_Draw(&line,&frame,0);
        CHECK(visible_steer && Mode()==CONTROL_STOPPED);
        Send(1,0,0);
        visible_planner=0; Menu_Draw(&line,&frame,0);
        CHECK(visible_planner && Mode()==CONTROL_STOPPED);
    }
    Send(4, 0, 0);
    CHECK(Mode() == CONTROL_TRACK);
    Send(0, 4, 0);
    Send(0, 0, 4); /* Holding Stop must not swallow the next new Start. */
    Send(4, 0, 0);
    CHECK(Mode() == CONTROL_TRACK);
    Send(0, 0, 2);
    CHECK(Mode() == CONTROL_STOPPED && currentMenu == MENU_MAIN);
    /* Full UI calibration -> min/max -> white reference flow. */
    Menu_Init();
    for (i = 0; i < 3; ++i) Send(1, 0, 0);
    Send(2, 0, 0);
    CHECK(currentMenu == MENU_CALIB);
    Send(4, 0, 0);
    for (i = 0; i < 60; ++i) TestRobot_Emit(i & 1, 0);
    Send(4, 0, 0);
    CHECK(Robot_GetStartReason() == START_NEED_WHITE);
    TestRobot_Emit(0, 0);
    Send(2, 0, 0);
    Robot_GetLine(&line);
    CHECK(line.calibrated && line.white_reference_valid);
    TestRobot_Emit(2, 0);
    CHECK(Robot_GetStartReason() == START_READY);
    Menu_Init(); Send(2, 0, 0); Send(4, 0, 0);
    CHECK(Mode() == CONTROL_TRACK);
    Send(0, 0, 2);
    /* User can edit Base/Max on OLED; all fields remain visible without scrolling. */
    TestRobot_Prepare(); Robot_Stop(FAULT_NONE); Menu_Init();
    Send(1,0,0); Send(1,0,0); Send(2,0,0);
    CHECK(currentMenu == MENU_PID);
    Send(1,0,0); Send(1,0,0); Send(1,0,0); /* Base row. */
    Send(2,0,0); Send(1,0,0); Send(2,0,0);
    CHECK(menu_parameters.base_percent == 50 && menu_parameters.straight_percent == 50);
    Send(1,0,0); Send(2,0,0); Send(4,0,0); Send(2,0,0); /* Next row is Max. */
    CHECK(menu_parameters.max_percent == 80);
    /* Lower Max beneath Base: UI must lower Base too, preserving the invariant. */
    Send(2,0,0);
    for(i=0;i<9;++i) Send(4,0,0);
    Send(2,0,0);
    CHECK(menu_parameters.base_percent==35 && menu_parameters.max_percent==35);
    CHECK(menu_parameters.straight_percent==35);
    /* Five-row wrap in both directions: Max -> Kp -> Max. */
    Send(1,0,0); Send(2,0,0); Send(1,0,0); Send(2,0,0);
    CHECK(menu_parameters.kp==12.5f && menu_parameters.max_percent==35);
    Send(4,0,0); Send(2,0,0); Send(4,0,0);
    CHECK(menu_parameters.max_percent==30 && menu_parameters.base_percent==30 && menu_parameters.straight_percent==30);
    Send(1,0,0); Send(2,0,0);
    /* Raising Base above Max raises the ceiling; the compatibility field follows. */
    Send(4,0,0); Send(2,0,0); Send(1,0,0); Send(1,0,0);
    CHECK(menu_parameters.base_percent==40 && menu_parameters.max_percent==40 && menu_parameters.straight_percent==40);
    Send(4,0,0); Send(2,0,0); Send(1,0,0); Send(2,0,0); Send(4,0,0); Send(2,0,0);
    CHECK(menu_parameters.base_percent==35 && menu_parameters.max_percent==35 && menu_parameters.straight_percent==35);
    {
        LineFrame frame = {{0},0,0,0,0};
        Robot_GetLine(&line); visible_speeds = 0;
        Menu_Draw(&line,&frame,0);
        CHECK(visible_speeds == 3U); /* No obsolete Straight row. */
    }
    Send(0,0,2); Send(4,0,0); Send(4,0,0); Send(2,0,0); Send(4,0,0);
    CHECK(currentMenu == MENU_RUN && Mode() == CONTROL_TRACK);
    for (i=0;i<800;++i) { Robot_UIAlive(Time_Millis()); TestRobot_Emit(2,0); }
    {
        RobotStatus status;
        Robot_GetStatus(&status);
        CHECK(status.left == 350 && status.right == 350); /* Edited Base/Max 35%. */
    }
    Send(0,0,2);
    /* Keep six old main items; corner timing is entered through System N3. */
    Menu_Init();
    for(i=0;i<5;++i) Send(1,0,0);
    Send(2,0,0); CHECK(currentMenu == MENU_SYSTEM);
    Send(4,0,0); CHECK(currentMenu == MENU_CORNER);
    Send(2,0,0); Send(1,0,0); Send(2,0,0);
    CHECK(menu_parameters.advance_ms==0 && menu_parameters.align_ms==14);
    Send(2,0,0); Send(4,0,0); Send(2,0,0);
    CHECK(menu_parameters.align_ms==12);
    Send(0,0,2); CHECK(currentMenu == MENU_MAIN);
    TestRobot_Prepare(); Menu_Init(); Send(2,0,0);
    TestRobot_Emit(0,0); Robot_Stop(FAULT_LINE);
    {
        LineFrame frame={{0},0,0,0,0};
        Robot_GetLine(&line); visible_fault=visible_place=0;
        Menu_Draw(&line,&frame,0);
        CHECK(visible_fault && !visible_place && Mode()==CONTROL_FAULT);
    }
    Robot_Init(); Menu_Init(); Send(2, 0, 0); Send(4, 0, 0);
    CHECK(Mode() == CONTROL_STOPPED && Robot_GetStartReason() == START_ADC);
    /* Existing six main items remain; System N2 owns stopped-only logging. */
    TestRobot_Prepare();
    for(i=0;i<20;++i) { Robot_UIAlive(Time_Millis()); TestRobot_Emit(2,0); }
    Robot_Stop(FAULT_NONE); Menu_Init();
    for(i=0;i<5;++i) Send(1,0,0);
    Send(2,0,0); Send(2,0,0);
    CHECK(currentMenu==MENU_LOG && Mode()==CONTROL_STOPPED);
    Send(1,0,0); Send(1,0,0); Send(2,0,0); /* Save. */
    {
        BlackboxStatus log;
        Blackbox_GetStatus(&log); CHECK(log.saved && log.ram_saved);
        Send(1,0,0); Send(2,0,0); /* Export never invokes Start. */
        CHECK(Mode()==CONTROL_STOPPED);
        Send(1,0,0); Send(1,0,0); Send(2,0,0); /* Toggle/arm CORNER. */
        Blackbox_GetStatus(&log);
        CHECK(log.trigger==LOG_TRIGGER_CORNER && log.state==LOG_ARMED && log.saved);
        Send(0,0,2); CHECK(currentMenu==MENU_MAIN && Mode()==CONTROL_STOPPED);
    }
}
