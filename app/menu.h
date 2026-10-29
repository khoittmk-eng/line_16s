#ifndef MENU_H
#define MENU_H
#include "buttons.h"
#include "line_adc.h"
#include "line_estimator.h"
#include "control.h"
typedef enum { MENU_MAIN, MENU_RUN, MENU_SENSOR, MENU_PID, MENU_CALIB, MENU_MOTOR, MENU_SYSTEM, MENU_CORNER, MENU_LOG } MenuState;
typedef ControlParameters MenuParameters;
extern MenuState currentMenu;
extern MenuParameters menu_parameters;
void Menu_Init(void);
void Menu_Process(const ButtonEvents *events, LineEstimator *line, uint8_t adc_stale,
                  uint8_t oled_ok, uint32_t now);
void Menu_Draw(const LineEstimator *line, const LineFrame *frame, uint8_t adc_stale);
#endif
