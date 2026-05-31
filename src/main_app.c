#include "main_app.h"
#include "blink_led.h"
#include "logger/logger.h"
#include "main.h"
#include "servo_task.h"
#include "usb_mouse_task.h"

#ifdef SYSVIEW_ENABLE
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_REC.h"

extern UART_HandleTypeDef huart2;

static void sysview_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    SYSVIEW_UART_Config();
    SEGGER_SYSVIEW_Conf();
    SEGGER_SYSVIEW_Start();
}
#endif

void app_init(void)
{
#ifdef SYSVIEW_ENABLE
    sysview_init();
#endif
    log_info("Initializing Tasks...");
    BlinkLEDTask_Create();
    ServoTask_Create();
    UsbMouseTask_Create();
}
