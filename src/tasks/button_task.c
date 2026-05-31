// button_task.c
#include "button_task.h"
#include "FreeRTOS.h"
#include "button_event.h"
#include "logger/logger.h"
#include "main.h"
#include "shared_state.h"
#include "task.h"

#define DEBOUNCE_MS 20

extern QueueHandle_t button_event_queue;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    const char *name;
    uint8_t last_stable_state; // last confirmed state after debounce
} Button;

static Button buttons[] = {
    {BTN1_GPIO_Port, BTN1_Pin, "BTN1", GPIO_PIN_SET},
    {BTN2_GPIO_Port, BTN2_Pin, "BTN2", GPIO_PIN_SET},
};

#define BUTTON_COUNT (sizeof(buttons) / sizeof(buttons[0]))

void ButtonTask(void *argument)
{
    for (;;)
    {
        for (uint8_t i = 0; i < BUTTON_COUNT; i++)
        {
            Button *btn = &buttons[i];

            GPIO_PinState current = HAL_GPIO_ReadPin(btn->port, btn->pin);

            // Only act if state changed from last stable reading
            if (current != btn->last_stable_state)
            {
                // Wait for bounce to settle
                vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));

                // Re-read after debounce delay
                GPIO_PinState confirmed = HAL_GPIO_ReadPin(btn->port, btn->pin);

                // Only accept if still changed
                if (confirmed != btn->last_stable_state)
                {
                    btn->last_stable_state = confirmed;

                    ButtonEvent event = {
                        .button_id = i,
                        .name      = btn->name,
                        .type      = (confirmed == GPIO_PIN_RESET) ? BUTTON_EVENT_PRESSED : BUTTON_EVENT_RELEASED,
                    };

                    // Send to queue — 0 timeout, don't block if queue is full
                    if (xQueueSend(button_event_queue, &event, 0) != pdTRUE)
                    {
                        log_info("button queue full — event dropped");
                    }

                    // Still log immediately for debug
                    if (event.type == BUTTON_EVENT_PRESSED)
                        log_info("%s pressed", btn->name);
                    else
                        log_info("%s released", btn->name);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // poll at 100Hz
    }
}

void ButtonTask_Create(void)
{
    xTaskCreate(ButtonTask, "BTN", 256, NULL, 2, NULL);
}