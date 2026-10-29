#ifndef LOG_SERIAL_H
#define LOG_SERIAL_H
/* Foreground, stopped-only TX. PB12 -> CH340 RXD, common GND, 9600 8N1. */
void LogSerial_Init(void);
void LogSerial_Write(const char *text);
#endif
