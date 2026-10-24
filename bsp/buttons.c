#include "board.h"
#include "buttons.h"
#include "timebase.h"
typedef struct {
    volatile uint32_t edge_ms;
    volatile uint32_t edge_sequence;
    uint32_t processed_sequence;
    uint32_t pressed_ms;
    uint8_t stable_pressed;
    uint8_t long_sent;
} ButtonState;
static ButtonState state[3];
static const uint16_t pins[3] = {GPIO_Pin_0, GPIO_Pin_1, GPIO_Pin_10};
static uint8_t IsPressed(uint8_t i)
{
    uint8_t high = GPIO_ReadInputDataBit(GPIOB, pins[i]);
    return BOARD_BUTTON_ACTIVE_LOW ? !high : high;
}
void Buttons_Init(void)
{
    GPIO_InitTypeDef gpio;
    EXTI_InitTypeDef exti;
    uint8_t i;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
    gpio.GPIO_Mode = BOARD_BUTTON_ACTIVE_LOW ? GPIO_Mode_IPU : GPIO_Mode_IPD;
    GPIO_Init(GPIOB, &gpio);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource10);
    EXTI_StructInit(&exti);
    exti.EXTI_Line = EXTI_Line0 | EXTI_Line1 | EXTI_Line10;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    exti.EXTI_LineCmd = ENABLE;
    EXTI_ClearITPendingBit(exti.EXTI_Line);
    EXTI_Init(&exti);
    /* A held-at-boot button must be released before generating a new short press. */
    for (i = 0; i < 3; ++i) {
        state[i].stable_pressed = IsPressed(i);
        state[i].long_sent = state[i].stable_pressed;
    }
    NVIC_SetPriority(EXTI0_IRQn, 3U);
    NVIC_SetPriority(EXTI1_IRQn, 3U);
    NVIC_SetPriority(EXTI15_10_IRQn, 3U);
    NVIC_EnableIRQ(EXTI0_IRQn); NVIC_EnableIRQ(EXTI1_IRQn); NVIC_EnableIRQ(EXTI15_10_IRQn);
}
static void RecordEdge(uint8_t i, uint32_t line)
{
    if (EXTI_GetITStatus(line) != RESET) {
        EXTI_ClearITPendingBit(line);
        state[i].edge_ms = Time_Millis();
        ++state[i].edge_sequence;
    }
}
void EXTI0_IRQHandler(void) { RecordEdge(0, EXTI_Line0); }
void EXTI1_IRQHandler(void) { RecordEdge(1, EXTI_Line1); }
void EXTI15_10_IRQHandler(void) { RecordEdge(2, EXTI_Line10); }
void Buttons_Process(uint32_t now, ButtonEvents *events)
{
    uint8_t i;
    events->pressed = events->released = events->short_press = events->long_press = 0;
    for (i = 0; i < 3; ++i) {
        uint32_t mask = Time_SaveIRQ();
        uint32_t sequence = state[i].edge_sequence;
        uint32_t edge_ms = state[i].edge_ms;
        uint8_t bit = (uint8_t)(1U << i);
        now = Time_Millis();
        /* GPIO is read only after an EXTI edge has settled, not periodically polled. */
        if (sequence != state[i].processed_sequence &&
            (uint32_t)(now - edge_ms) >= BOARD_BUTTON_DEBOUNCE_MS) {
            uint8_t pressed = IsPressed(i);
            state[i].processed_sequence = sequence;
            if (pressed != state[i].stable_pressed) {
                state[i].stable_pressed = pressed;
                if (pressed) {
                    state[i].pressed_ms = edge_ms;
                    state[i].long_sent = 0;
                    events->pressed |= bit;
                } else {
                    events->released |= bit;
                    if (!state[i].long_sent) {
                        if ((uint32_t)(edge_ms - state[i].pressed_ms) >= BOARD_BUTTON_LONG_MS)
                            events->long_press |= bit;
                        else events->short_press |= bit;
                    }
                }
            }
        }
        if (state[i].stable_pressed && !state[i].long_sent &&
            state[i].edge_sequence == state[i].processed_sequence &&
            (uint32_t)(now - state[i].pressed_ms) >= BOARD_BUTTON_LONG_MS) {
            state[i].long_sent = 1;
            events->long_press |= bit;
        }
        Time_RestoreIRQ(mask);
    }
}
