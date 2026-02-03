/* Includes ------------------------------------------------------------------*/
#include "main_app.h"
#include "blink_led.h"
#include "main.h"

void app_init(void)
{
    BlinkLEDTask_Create();
}

void app_run(void)
{
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, orange_LED_Pin);
        HAL_Delay(1500);
    }
}