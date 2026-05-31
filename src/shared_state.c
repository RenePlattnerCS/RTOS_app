#include "shared_state.h"

JoystickState joystick_state     = {0};
SemaphoreHandle_t joystick_mutex = NULL;
QueueHandle_t button_event_queue = 0;
QueueHandle_t button_usb_queue   = 0;

void shared_state_init(void)
{
    joystick_mutex     = xSemaphoreCreateMutex();
    button_event_queue = xQueueCreate(BUTTON_QUEUE_LENGTH, sizeof(ButtonEvent));
    button_usb_queue   = xQueueCreate(8, sizeof(ButtonEvent));

    configASSERT(joystick_mutex != 0);
    configASSERT(button_event_queue != 0);
    configASSERT(button_usb_queue != 0);
}
