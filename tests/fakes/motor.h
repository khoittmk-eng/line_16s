#ifndef FAKE_MOTOR_H
#define FAKE_MOTOR_H
#include <stdint.h>
void Motor_StopBrake(void);
void Motor_SetDuty(int16_t left, int16_t right);
#endif
