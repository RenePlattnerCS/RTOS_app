// button_event.h
#ifndef BUTTON_EVENT_H
#define BUTTON_EVENT_H

#include <stdint.h>

typedef enum
{
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED,
} ButtonEventType;

typedef struct
{
    uint8_t button_id; // 0 = BTN1, 1 = BTN2
    ButtonEventType type;
    const char *name;
} ButtonEvent;

#endif