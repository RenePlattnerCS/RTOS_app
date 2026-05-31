#include "usb_joystick_task.h"
#include "FreeRTOS.h"
#include "logger/logger.h"
#include "shared_state.h"
#include "task.h"
#include "usb/usbd_framework.h"

static TaskHandle_t usb_joystick_task_handle = NULL;

static void UsbJoystickTask(void *argument)
{
    log_info("USB joystick task started");

    // Persists across loop iterations — only button events change it.
    // If this were reset each loop from joystick_state.buttons (always 0),
    // a held button would disappear after the first 4ms window.
    uint8_t current_buttons = 0;

    for (;;)
    {
        JoystickState local;

        xSemaphoreTake(joystick_mutex, portMAX_DELAY);
        local = joystick_state;
        xSemaphoreGive(joystick_mutex);

        // Drain button queue and update persistent state.
        ButtonEvent event;
        while (xQueueReceive(button_usb_queue, &event, 0) == pdTRUE)
        {
            if (event.type == BUTTON_EVENT_PRESSED)
                current_buttons |= (uint8_t)(1u << event.button_id);
            else
                current_buttons &= (uint8_t)~(1u << event.button_id);
        }
        local.buttons = current_buttons;

        usbd_update_joystick(&local);
        usbd_poll();
        vTaskDelay(pdMS_TO_TICKS(4));
    }
}

void UsbJoystickTask_Create(void)
{
    xTaskCreate(UsbJoystickTask, "UsbJoystick", 256, NULL, 3, &usb_joystick_task_handle);
}