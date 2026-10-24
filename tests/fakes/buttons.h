#ifndef FAKE_BUTTONS_H
#define FAKE_BUTTONS_H
#include <stdint.h>
typedef struct { uint8_t pressed, released, short_press, long_press; } ButtonEvents;
#endif
