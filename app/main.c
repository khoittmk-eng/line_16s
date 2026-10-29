#include "board.h"
#include "board_init.h"
#include "timebase.h"
#include "motor.h"
#include "line_adc.h"
#include "buttons.h"
#include "oled.h"
#include "line_estimator.h"
#include "menu.h"
#include "robot.h"
#include <stdio.h>
/* Debugger-visible snapshots; estimator and control are owned by the DMA callback. */
LineFrame debug_frame;
LineEstimator debug_line;
volatile uint32_t button_short_count[3], button_long_count[3];
volatile uint32_t adc_frame_count;
volatile uint8_t adc_ready, oled_ready, adc_stale;
int main(void)
{
    uint32_t last_blink = 0, last_page = 0, last_sequence = 0;
    uint8_t page = 0;
    ButtonEvents events;
    SystemCoreClockUpdate();
    Board_Init();
    Time_Init();
    Motor_Init();
    Robot_Init();
    LineADC_SetCallback(Robot_OnFrame);
    Buttons_Init();
    Menu_Init();
    adc_ready = LineADC_Init();
    /* Foreground startup delay for OLED power stabilization; interrupts continue. */
    {
        uint32_t start = Time_Millis();
        while ((uint32_t)(Time_Millis() - start) < 100U) { }
    }
    oled_ready = OLED_Init();
    while (1) {
        uint32_t now;
        uint8_t i;
        LineADC_GetFrame(&debug_frame);
        now = Time_Millis();
        adc_stale = !adc_ready || !debug_frame.sequence || debug_frame.dma_errors ||
                    debug_frame.missed_frames || (uint32_t)(now - debug_frame.timestamp_ms) > 10U;
        if (debug_frame.sequence != last_sequence) {
            last_sequence = debug_frame.sequence;
            adc_frame_count = last_sequence;

        }
        Robot_GetLine(&debug_line);
        Robot_UIAlive(now);
        Buttons_Process(now, &events);
        for (i = 0; i < 3; ++i) {
            if (events.short_press & (1U << i)) ++button_short_count[i];
            if (events.long_press & (1U << i)) ++button_long_count[i];
        }
        Menu_Process(&events, &debug_line, adc_stale, oled_ready, now);
        if ((uint32_t)(now - last_blink) >= 500U) {
            last_blink = now;
            GPIO_WriteBit(BOARD_LED_PORT, BOARD_LED_PIN,
                GPIO_ReadOutputDataBit(BOARD_LED_PORT, BOARD_LED_PIN) ? Bit_RESET : Bit_SET);
        }
        if (oled_ready && (uint32_t)(now - last_page) >= 10U) {
            last_page = now;
            if (page == 0U) Menu_Draw(&debug_line, &debug_frame, adc_stale);
            oled_ready = OLED_FlushPage(page);
            page = (page + 1U) & 7U;
            if (!oled_ready) {
                ButtonEvents no_events = {0, 0, 0, 0};
                Menu_Process(&no_events, &debug_line, adc_stale, 0, Time_Millis());
            }
        }
        /* CPU giu chay lien tuc: khong vao WFI de giu kenh SWD luon san sang cho ST-Link/Keil */
    }
}
