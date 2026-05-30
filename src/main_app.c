/* Includes ------------------------------------------------------------------*/
#include "main_app.h"
#include "blink_led.h"
#include "logger/logger.h"
#include "main.h"
#include "servo_task.h"

void app_init(void)
{
    log_info("System initialized with ITM enabled");
    BlinkLEDTask_Create();
    ServoTask_Create();
}

void app_run(void)
{
    while (1)
    {
        // HAL_GPIO_TogglePin(LED_GPIO_Port, orange_LED_Pin);
        // HAL_Delay(1500);
    }
}