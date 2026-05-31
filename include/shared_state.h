#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"

typedef struct
{
    int8_t  x;
    int8_t  y;
    uint8_t buttons;
} JoystickState;

extern JoystickState      joystick_state;
extern SemaphoreHandle_t  joystick_mutex;

void shared_state_init(void);

#endif /* SHARED_STATE_H */
