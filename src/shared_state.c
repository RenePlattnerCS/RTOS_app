#include "shared_state.h"

JoystickState     joystick_state = {0};
SemaphoreHandle_t joystick_mutex = NULL;

void shared_state_init(void)
{
    joystick_mutex = xSemaphoreCreateMutex();
}
