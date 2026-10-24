#ifndef MOTOR_H
#define MOTOR_H
#include "stm32f10x.h"
/* Signed permille [-1000, 1000], NOT measured RPM. */
void Motor_Init(void);
void Motor_SafetyTick(uint32_t now);
void Motor_SetDuty(int16_t left, int16_t right);
void Motor_StopBrake(void);
void Motor_StopCoast(void);
#endif
