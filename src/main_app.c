/* Includes ------------------------------------------------------------------*/
#include "main_app.h"
#include "main.h"

void main_app(void)
{
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, orange_LED_Pin);
        HAL_Delay(1500);
    }
}