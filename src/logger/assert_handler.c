#include "logger/logger.h"
#include "main.h"
#include "stm32f4xx.h"

void assert_failed_handler(char const *file, int line, char const *expr)
{
    __disable_irq();
    log_error("ASSERT: (%s) failed at %s:%d", expr, file, line);
    __BKPT(0);
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, orange_LED_Pin);
        HAL_Delay(1500);
    }
}
