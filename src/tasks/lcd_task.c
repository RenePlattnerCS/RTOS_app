// lcd_task.c
#include "lcd_task.h"
#include "FreeRTOS.h"
#include "button_event.h"
#include "lcd.h"
#include "main.h"
#include "shared_state.h"
#include "task.h"
#include <stm32f4xx_hal_tim.h>

extern uint32_t debug_adc;      // from servo_task.c
extern TIM_HandleTypeDef htim3; // for microsecond delay

char last_event_str[16] = "waiting..."; // shown on row 1

void LcdTask(void *argument)
{
    vTaskDelay(pdMS_TO_TICKS(200));
    lcd_init();

    for (;;)
    {
        // snapshot shared state safely

        JoystickState local;
        xSemaphoreTake(joystick_mutex, portMAX_DELAY);
        local = joystick_state;
        xSemaphoreGive(joystick_mutex);

        // row 0: axis value
        lcd_set_cursor(0, 0);
        lcd_print("Axis X:         "); // trailing spaces clear old chars

        lcd_set_cursor(0, 8);
        lcd_print_int(local.x);

        // row 1: raw ADC
        lcd_set_cursor(1, 0);
        lcd_print("ADC:            ");
        lcd_set_cursor(1, 5);
        lcd_print_int((int) debug_adc);

        ButtonEvent event;
        if (xQueueReceive(button_event_queue, &event, 0) == pdTRUE)
        {
            // Format event string for LCD row 1
            snprintf(
                last_event_str,
                sizeof(last_event_str),
                "%s %s",
                event.name,
                event.type == BUTTON_EVENT_PRESSED ? "DOWN" : "UP  ");
        }
        // Row 1 — last button event
        lcd_set_cursor(1, 0);
        lcd_print(last_event_str);

        vTaskDelay(pdMS_TO_TICKS(100)); // 10Hz refresh — LCD cant show faster
    }
}

// Function to create the task
void LcdTask_Create(void)
{
    xTaskCreate(LcdTask, "LCD", 256, NULL, 1, NULL); // low priority — display only
}