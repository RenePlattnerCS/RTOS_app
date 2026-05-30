/* Includes ------------------------------------------------------------------*/
#include "main_app.h"
#include "blink_led.h"
#include "logger/logger.h"
#include "main.h"
#include "servo_task.h"
#include "usb_mouse_task.h"

void app_init(void)
{
    log_info("Initializing Tasks...");
    BlinkLEDTask_Create();
    ServoTask_Create();
    UsbMouseTask_Create();
}
