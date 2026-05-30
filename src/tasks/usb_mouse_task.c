#include "usb_mouse_task.h"
#include "usb/usbd_framework.h"
#include "logger/logger.h"
#include "FreeRTOS.h"
#include "task.h"

static TaskHandle_t usb_mouse_task_handle = NULL;

static void UsbMouseTask(void *argument)
{
    log_info("USB mouse task started");
    for (;;)
    {
        usbd_poll();
        taskYIELD();
    }
}

void UsbMouseTask_Create(void)
{
    xTaskCreate(
        UsbMouseTask,
        "UsbMouse",
        256,
        NULL,
        3,
        &usb_mouse_task_handle);
}
