/* Includes ------------------------------------------------------------------*/
#include "main_app.h"
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_REC.h"
#include "blink_led.h"
#include "logger/logger.h"
#include "main.h"
#include "servo_task.h"
#include "usb_mouse_task.h"

extern UART_HandleTypeDef huart2;

static void sysview_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    SYSVIEW_UART_Config();
    SEGGER_SYSVIEW_Conf();
    SEGGER_SYSVIEW_Start();
    // The FreeRTOS trace hooks poll the RTT down-buffer on every task switch.
    // SystemView sends START when you click Record, the target responds with
    // a fresh HELLO, and recording begins cleanly with no buffer-overflow race.
}

void app_init(void)
{
    // char test[] = "UART OK\r\n";
    // HAL_UART_Transmit(&huart2, (uint8_t *) test, sizeof(test), 100);
    sysview_init();
    log_info("Initializing Tasks...");
    BlinkLEDTask_Create();
    ServoTask_Create();
    UsbMouseTask_Create();
}
