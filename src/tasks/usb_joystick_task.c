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
    for (;;)
    {
        JoystickState local;

        xSemaphoreTake(joystick_mutex, portMAX_DELAY);
        local = joystick_state; // copy under mutex
        xSemaphoreGive(joystick_mutex);

        usbd_update_joystick(&local); // hand it to the USB layer
        usbd_poll();
        vTaskDelay(pdMS_TO_TICKS(4));
    }
}

void UsbJoystickTask_Create(void)
{
    xTaskCreate(UsbJoystickTask, "UsbJoystick", 256, NULL, 3, &usb_joystick_task_handle);
}