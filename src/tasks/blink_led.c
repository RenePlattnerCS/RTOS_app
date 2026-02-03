#include "blink_led.h"
#include "main.h"
#include "stm32f4xx_hal.h" // For HAL_GPIO_TogglePin, etc.

// Task handle (optional, if you need to control the task later)
static TaskHandle_t blinkTaskHandle = NULL;

// Task function
static void BlinkLEDTask(void *argument)
{
    for (;;)
    {
        HAL_GPIO_TogglePin(GPIOD, LED_Pin); // toggle LED on PD12
        vTaskDelay(pdMS_TO_TICKS(2000));    // delay 500ms
        // HAL_Delay(1000);
    }
}

// Function to create the task
void BlinkLEDTask_Create(void)
{
    xTaskCreate(
        BlinkLEDTask,      // Task function
        "BlinkLED",        // Name (for debug)
        128,               // Stack size (in words, not bytes)
        NULL,              // Parameters
        2,                 // Priority
        &blinkTaskHandle); // Handle
}
