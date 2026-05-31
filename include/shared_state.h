#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include "FreeRTOS.h"
#include "button_event.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>

typedef struct
{
    int8_t x;
    int8_t y;
    uint8_t buttons;
} JoystickState;

#define BUTTON_QUEUE_LENGTH 8 // holds up to 8 unprocessed events

extern QueueHandle_t button_event_queue;

extern JoystickState joystick_state;
extern SemaphoreHandle_t joystick_mutex;

void shared_state_init(void);

#endif /* SHARED_STATE_H */
