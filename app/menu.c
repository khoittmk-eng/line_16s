#include "menu.h"
#include "oled.h"
#include "motor.h"
#include "robot.h"
#include <stdio.h>
#define TEST_DUTY 200
#define TEST_TIMEOUT_MS 2000U
MenuState currentMenu;
MenuParameters menu_parameters;
static uint8_t mainIndex, pidIndex, isEditingPID, calibState, motorTestIdx, motorTestActive;
static uint32_t motor_started_ms;
static uint8_t notice, suppress_run_release, isEditingCorner, show_last;
static uint32_t last_service_ms;
static const char *SpeedLabel(SpeedReason reason)
{
    static const char *labels[]={"OFF","BASE","BST","CURV","WAIT","GAP","REC","BRK","PRB","CRN"};
    return (unsigned)reason<sizeof(labels)/sizeof(labels[0])?labels[reason]:"?";
}
static void ResetParameters(void)
{
    Control_Defaults(&menu_parameters);
    Robot_SetParameters(&menu_parameters);
}
void Menu_Init(void)
{
    currentMenu = MENU_MAIN;
    mainIndex = pidIndex = isEditingPID = calibState = motorTestIdx = motorTestActive = notice = 0;
    isEditingCorner = suppress_run_release = show_last = 0;
    ResetParameters();
    Motor_StopBrake();
}
static void StopTest(void) { Motor_StopBrake(); motorTestActive = 0; }
static void ChangeParameter(int direction)
{
    float *value = pidIndex == 0 ? &menu_parameters.kp :
                   pidIndex == 1 ? &menu_parameters.ki : &menu_parameters.kd;
    if (pidIndex <= 2) {
        float step = pidIndex == 0 ? 0.5f : 0.01f;
        float maximum = pidIndex == 0 ? 100.0f : 5.0f;
        *value += direction * step;
        if (*value < 0.0f) *value = 0.0f;
        if (*value > maximum) *value = maximum;
    } else {
        uint16_t *speed = pidIndex == 3 ? &menu_parameters.base_percent : &menu_parameters.max_percent;
        int next = (int)*speed + direction * 5;
        if (next < 0) next = 0;
        if (next > 100) next = 100;
        *speed = (uint16_t)next;
        if (menu_parameters.base_percent > menu_parameters.max_percent) {
            if (pidIndex == 3) menu_parameters.max_percent = menu_parameters.base_percent;
            else menu_parameters.base_percent = menu_parameters.max_percent;
        }
    }
}
void Menu_Process(const ButtonEvents *events, LineEstimator *line, uint8_t stale,
                  uint8_t oled_ok, uint32_t now)
{
    RobotStatus run;
    Robot_GetStatus(&run);
    Robot_SetParameters(&menu_parameters);
    if (motorTestActive && (stale || !oled_ok || (uint32_t)(now - motor_started_ms) >= TEST_TIMEOUT_MS ||
                            (uint32_t)(now - last_service_ms) > 20U)) StopTest();
    last_service_ms = now;
    if (events->long_press & 4U) suppress_run_release = 0;
    if ((!oled_ok || stale) && run.mode != CONTROL_STOPPED && run.mode != CONTROL_FAULT)
        Robot_Stop(!oled_ok ? FAULT_OLED : FAULT_ADC);
    if (currentMenu == MENU_RUN && (events->pressed & 4U) &&
        run.mode != CONTROL_STOPPED && run.mode != CONTROL_FAULT) {
        Robot_Stop(FAULT_NONE); suppress_run_release = 1;
    }
    if (events->long_press & 2U) {
        Robot_Stop(FAULT_NONE);
        StopTest();
        Robot_CancelCalibration();
        calibState = isEditingPID = isEditingCorner = notice = suppress_run_release = 0;
        currentMenu = MENU_MAIN;
        return;
    }
    switch (currentMenu) {
    case MENU_MAIN:
        if (events->short_press & 1U) mainIndex = (mainIndex + 1U) % 6U;
        if (events->short_press & 4U) mainIndex = mainIndex ? mainIndex - 1U : 5U;
        if (events->short_press & 2U) {
            currentMenu = (MenuState)(mainIndex + 1U);
            notice = 0;
            if (currentMenu == MENU_MOTOR) { motorTestIdx = 0; StopTest(); }
            if (currentMenu == MENU_CALIB) calibState = 0;
        }
        break;
    case MENU_RUN:
        if((events->short_press&1U) && (run.mode==CONTROL_STOPPED || run.mode==CONTROL_FAULT)) show_last=(show_last+1U)%3U;
        if (events->long_press & 1U) {
            if (run.mode == CONTROL_STOPPED || run.mode == CONTROL_FAULT) Robot_ToggleSteering();
        }
        if (events->short_press & 4U) {
            show_last=0;
            if (suppress_run_release) suppress_run_release = 0;
            else if (run.mode != CONTROL_STOPPED && run.mode != CONTROL_FAULT) Robot_Stop(FAULT_NONE);
            else {
                Robot_SetParameters(&menu_parameters);
                notice = stale || !oled_ok || !Robot_Start();
            }
        }
        break;
    case MENU_PID:
        if (events->short_press & 1U) {
            if (isEditingPID) ChangeParameter(1); else pidIndex = (pidIndex + 1U) % 5U;
        }
        if (events->short_press & 4U) {
            if (isEditingPID) ChangeParameter(-1); else pidIndex = pidIndex ? pidIndex - 1U : 4U;
        }
        if (events->short_press & 2U) isEditingPID ^= 1U;
        break;
    case MENU_CALIB:
        if ((events->short_press & 2U) && calibState == 3) {
            notice = Robot_SetWhiteReference() ? 0 : 7;
        }
        if ((events->short_press & 1U) && calibState == 0) notice = 2; /* Auto spin not yet available. */
        if (events->short_press & 4U) {
            notice = 0;
            if (calibState == 2) { Robot_EndCalibration(); calibState = 3; }
            else if (calibState == 0 && !stale) { Robot_BeginCalibration(); calibState = 2; }
        }
        if (calibState == 2 && stale) { Robot_CancelCalibration(); calibState = 3; }
        break;
    case MENU_SENSOR:
        /* Extra shortcut preserves polarity selection without changing old short-press mapping. */
        if (events->long_press & 1U) Robot_TogglePolarity();
        break;
    case MENU_MOTOR:
        if (events->short_press & 1U) { StopTest(); motorTestIdx = (motorTestIdx + 1U) % 5U; }
        if (events->short_press & 4U) { StopTest(); motorTestIdx = motorTestIdx ? motorTestIdx - 1U : 4U; }
        if (events->short_press & 2U) {
            if (motorTestActive || motorTestIdx == 0) StopTest();
            else if (!stale && oled_ok) {
                motor_started_ms = now; motorTestActive = 1;
                switch (motorTestIdx) {
                case 1: Motor_SetDuty(TEST_DUTY, 0); break;
                case 2: Motor_SetDuty(0, TEST_DUTY); break;
                case 3: Motor_SetDuty(TEST_DUTY, TEST_DUTY); break;
                case 4: Motor_SetDuty(-TEST_DUTY, -TEST_DUTY); break;
                default: StopTest(); break;
                }
            }
        }
        break;
    case MENU_SYSTEM:
        if (events->short_press & 1U) ResetParameters();
        if (events->short_press & 4U) { currentMenu = MENU_CORNER; isEditingCorner = 0; }
        break;
    case MENU_CORNER:
        if(isEditingCorner && (events->short_press&(1U|4U))) {
            int value=(int)menu_parameters.align_ms+((events->short_press&1U)?2:-2);
            if(value<4) value=4;
            if(value>40) value=40;
            menu_parameters.align_ms=(uint16_t)value;
        }
        if(events->short_press&2U) isEditingCorner^=1U;
        break;
    default: Robot_Stop(FAULT_NONE); StopTest(); currentMenu = MENU_MAIN; break;
    }
    if (motorTestActive) {
        switch (motorTestIdx) {
        case 1: Motor_SetDuty(TEST_DUTY, 0); break;
        case 2: Motor_SetDuty(0, TEST_DUTY); break;
        case 3: Motor_SetDuty(TEST_DUTY, TEST_DUTY); break;
        case 4: Motor_SetDuty(-TEST_DUTY, -TEST_DUTY); break;
        default: StopTest(); break;
        }
    }
}
static void DrawMain(void)
{
    static const char *items[] = {"1. Run", "2. Sensor Monitor", "3. PID Setup", "4. Calibration", "5. Motor Test", "6. System"};
    uint8_t start = mainIndex >= 4U ? mainIndex - 3U : 0U, i;
    OLED_DrawString(1, 2, BOARD_FIRMWARE_TAG, 1);
    OLED_DrawString(20, 2, "MAIN MENU", 1);
    OLED_DrawString(88, 2, "N2:OK", 1);
    OLED_DrawRect(0, 0, 128, 12, 1);
    for (i = 0; i < 4 && start + i < 6; ++i) {
        uint8_t y = 16U + i * 12U;
        if (start + i == mainIndex) OLED_DrawString(2, y, ">", 1);
        OLED_DrawString(12, y, items[start + i], 1);
    }
}
static void DrawPID(void)
{
    uint8_t i;
    char text[24];
    OLED_DrawString(30, 0, "PID SETUP", 1);
    OLED_DrawString(104, 0, "RAM", 1);
    /* All five parameters visible: Base/Max must not be hidden by scrolling. */
    for (i = 0; i < 5U; ++i) {
        uint8_t item = i, y = 12U + i * 8U;
        if (item == pidIndex) OLED_DrawString(2, y, isEditingPID ? "*" : ">", 1);
        switch (item) {
        case 0: snprintf(text, sizeof(text), "Kp  : %.1f", menu_parameters.kp); break;
        case 1: snprintf(text, sizeof(text), "Ki  : %.2f", menu_parameters.ki); break;
        case 2: snprintf(text, sizeof(text), "Kd  : %.2f", menu_parameters.kd); break;
        case 3: snprintf(text, sizeof(text), "Base: %u%%", menu_parameters.base_percent); break;
        default: snprintf(text, sizeof(text), "Max : %u%%", menu_parameters.max_percent); break;
        }
        OLED_DrawString(15, y, text, 1);
    }
    OLED_DrawString(0, 56, isEditingPID ? "N1:+  N3:-  N2:OK" : "N2:Edit [Hold:Back]", 1);
}
static void DrawMotor(void)
{
    static const char *items[] = {"0. Stop All", "1. Left Fwd", "2. Right Fwd", "3. Both Fwd", "4. Both Rev"};
    uint8_t i, start = motorTestIdx >= 4U ? 1U : 0U;
    OLED_DrawString(25, 0, "MOTOR TEST", 1);
    for (i = 0; i < 4U; ++i) {
        uint8_t item = start + i, y = 14U + i * 10U;
        if (item == motorTestIdx) OLED_DrawString(2, y, ">", 1);
        OLED_DrawString(12, y, items[item], 1);
    }
    OLED_DrawString(90, 44, motorTestActive ? "[RUN]" : "[OFF]", 1);
    OLED_DrawString(0, 56, "[Hold N2 to Back]", 1);
}
void Menu_Draw(const LineEstimator *line, const LineFrame *frame, uint8_t stale)
{
    char text[32];
    uint8_t i;
    OLED_Clear();
    switch (currentMenu) {
    case MENU_MAIN: DrawMain(); break;
    case MENU_PID: DrawPID(); break;
    case MENU_MOTOR: DrawMotor(); break;
    case MENU_RUN: {
        RobotStatus run;
        static const char *modes[] = {"READY", "TRACK", "GAP", "SEARCH", "FAULT", "TURN", "PROBE", "UNUSED", "RECENTER", "BRAKE"};
        static const char *short_modes[] = {"RDY","TRK","GAP","SRC","FLT","TRN","PRB","---","REC","BRK"};
        RobotStartReason reason = Robot_GetStartReason();
        static const char *blocked[] = {"READY N3 START", "ADC ERROR: RESET", "CALIB IN PROGRESS",
            "CALIBRATE FIRST", "SET WHITE REF FIRST", "PLACE ON BLACK LINE", "BASE/MAX IS ZERO"};
        Robot_GetStatus(&run);
        if(show_last==2U && (run.mode==CONTROL_STOPPED || run.mode==CONTROL_FAULT)) {
            RobotDiagnostics d;
            Robot_GetDiagnostics(&d);
            OLED_DrawString(12,0,"DIAG " BOARD_FIRMWARE_TAG,1);
            if(d.valid) {
                snprintf(text,sizeof(text),"C:%u G:%u T:%u",d.corner_events>999?999:d.corner_events,d.gap_entries>999?999:d.gap_entries,d.turn_entries>999?999:d.turn_entries);
                OLED_DrawString(0,12,text,1);
                snprintf(text,sizeof(text),"LAST:%02X LOSS:%02X",d.last_valid_mask,d.loss_mask);
                OLED_DrawString(0,24,text,1);
                snprintf(text,sizeof(text),"FROM:%s DIR:%+d",short_modes[d.loss_from],d.loss_side);
                OLED_DrawString(0,36,text,1);
                snprintf(text,sizeof(text),"dt%lu ex%lu us",(unsigned long)(d.max_interval_us>99999?99999:d.max_interval_us),(unsigned long)(d.max_execution_us>99999?99999:d.max_execution_us));
                OLED_DrawString(0,48,text,1);
            } else OLED_DrawString(0,24,"NO RUN DATA",1);
            snprintf(text,sizeof(text),"F:%u R:%u N1:LIVE",(unsigned)run.fault,(unsigned)run.line_stop_reason);
            OLED_DrawString(0,57,text,1);
            break;
        }
        if(show_last && (run.mode==CONTROL_STOPPED || run.mode==CONTROL_FAULT)) {
            RobotTrace trace;
            Robot_GetLastRun(&trace);
            OLED_DrawString(12,0,"LAST RUN " BOARD_FIRMWARE_TAG,1);
            if(trace.valid) {
                snprintf(text,sizeof(text),"%s %s V%u T%u",short_modes[trace.mode],SpeedLabel(trace.speed_reason),trace.common_percent,trace.target_percent);
                OLED_DrawString(0,12,text,1);
                snprintf(text,sizeof(text),"E:%+d MASK:%02X",trace.error,trace.mask);
                OLED_DrawString(0,25,text,1);
                snprintf(text,sizeof(text),"L:%d%% R:%d%%",trace.left/10,trace.right/10);
                OLED_DrawString(0,38,text,1);
                snprintf(text,sizeof(text),"B:%u SAT:%lu F:%u",trace.brake_count,(unsigned long)trace.saturation_ms,(unsigned)run.fault);
                OLED_DrawString(0,49,text,1);
            } else OLED_DrawString(0,25,"NO RUN DATA",1);
            OLED_DrawString(0,57,"N1:DIAG N3:START",1);
            break;
        }
        OLED_DrawString(30, 0, "RUN MODE", 1);
        OLED_DrawString(100, 0, BOARD_FIRMWARE_TAG, 1);
        OLED_DrawRect(0, 0, 128, 10, 1);
        if (run.mode == CONTROL_FAULT) {
            static const char *faults[] = {"FAULT", "ADC FAULT", "LINE LOST", "TIMING FAULT", "UI FAULT", "OLED FAULT"};
            OLED_DrawString(0,12,faults[run.fault],1);
        } else if (run.mode == CONTROL_STOPPED)
            OLED_DrawString(0,12,blocked[reason],1);
        else OLED_DrawString(20, 12, modes[run.mode], 1);
        if(run.mode!=CONTROL_STOPPED && run.mode!=CONTROL_FAULT) {
            snprintf(text,sizeof(text),"B:%u",run.brake_count);
            OLED_DrawString(96,12,text,1);
        }
        snprintf(text, sizeof(text), "E:%+d F:%u DIR:%c", line->error, (unsigned)run.fault,
                 run.steering_sign > 0 ? '+' : '-');
        OLED_DrawString(0, 25, text, 1);
        snprintf(text, sizeof(text), "PWM L:%d%% R:%d%%", run.left / 10, run.right / 10);
        OLED_DrawString(0, 38, text, 1);
        snprintf(text,sizeof(text),"V%u T%u %s",run.common_percent,run.target_percent,SpeedLabel(run.speed_reason));
        OLED_DrawString(0,50,text,1);
        snprintf(text, sizeof(text), "R:%u", (unsigned)run.line_stop_reason);
        OLED_DrawString(104, 50, text, 1);
        OLED_DrawString(0, 57, "N3:Run Hold N2:Back", 1);
        break;
    }
    case MENU_SENSOR:
        OLED_DrawString(18, 0, "SENSOR MONITOR", 1);
        OLED_DrawRect(0, 0, 128, 10, 1);
        for (i = 0; i < LINE_COUNT; ++i) {
            uint8_t x = 8U + i * 14U, y, h;
            uint16_t strength = line->calibrated ? line->strength[i] : (uint16_t)(((uint32_t)frame->raw[i] * 1000U) / 4095U);
            h = (uint8_t)((strength * 25U) / 1000U);
            OLED_DrawRect(x, 13, 10, 27, 1);
            for (y = 0; y < h; ++y) {
                uint8_t col;
                for (col = 1; col < 9U; ++col) OLED_DrawPixel(x + col, 39U - y, 1);
            }
        }
        snprintf(text, sizeof(text), "D:%02X E:%+d V:%u", line->mask, line->error, line->valid);
        OLED_DrawString(0, 43, text, 1);
        snprintf(text, sizeof(text), "CAL:%u W:%u ADC:%s", line->calibrated,
                 line->white_reference_valid, stale ? "ERR" : "OK");
        OLED_DrawString(0, 54, text, 1);
        break;
    case MENU_CALIB:
        OLED_DrawString(22, 0, "CALIBRATION", 1);
        OLED_DrawRect(0, 0, 128, 10, 1);
        if (calibState == 0) {
            OLED_DrawString(0, 15, "N1: Auto Calib (Spin)", 1);
            OLED_DrawString(0, 28, "N3: Manual Calib", 1);
            OLED_DrawString(0, 42, notice == 2 ? "AUTO NOT READY" : "Status: READY", 1);
        } else if (calibState == 2) {
            OLED_DrawString(10, 20, "MANUAL CALIB...", 1);
            OLED_DrawString(0, 35, "Sweep car over line", 1);
            OLED_DrawString(0, 48, "Press N3 to Finish", 1);
        } else {
            if (!line->calibrated) {
                OLED_DrawString(20, 25, "CALIB FAILED", 1);
                OLED_DrawString(0, 42, "Hold N2 to retry", 1);
            } else if (!line->white_reference_valid) {
                OLED_DrawString(0, 19, "PLACE ALL ON WHITE", 1);
                OLED_DrawString(0, 30, "N2: SET WHITE REF", 1);
                OLED_DrawString(0, 42, notice == 7 ? "SWEEP/WHITE RETRY" : "MOTORS STOPPED", 1);
            } else {
                OLED_DrawString(20, 25, "CALIB DONE!", 1);
                OLED_DrawString(0, 42, "WHITE REF OK", 1);
            }
        }
        OLED_DrawString(0, 56, "[Hold N2 to Back]", 1);
        break;
    case MENU_SYSTEM:
        OLED_DrawString(0, 0, "SYSTEM CONFIG " BOARD_FIRMWARE_TAG, 1);
        OLED_DrawRect(0, 0, 128, 10, 1);
        OLED_DrawString(0, 14, "MCU: STM32F103C8T6", 1);
        OLED_DrawString(0, 26, "PWM: TIM4 - 20kHz", 1);
        OLED_DrawString(0, 38, "ADC:DMA N3:Corner", 1);
        OLED_DrawString(0, 50, "N1: Reset PID Default", 1);
        break;
    case MENU_CORNER:
        OLED_DrawString(12,0,"RECOVERY " BOARD_FIRMWARE_TAG,1);
        OLED_DrawString(0,14,"Advance: DISABLED",1);
        OLED_DrawString(2,28,isEditingCorner?"*":">",1);
        snprintf(text,sizeof(text),"Reacq: %ums",menu_parameters.align_ms);
        OLED_DrawString(15,28,text,1);
        OLED_DrawString(0,42,"SENSOR FEEDBACK",1);
        OLED_DrawString(0,56,isEditingCorner?"N1:+ N3:- N2:OK":"N2:Edit Hold:Back",1);
        break;
    default: break;
    }
}
