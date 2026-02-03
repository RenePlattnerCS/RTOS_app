#ifndef BLINK_LED_H
#define BLINK_LED_H

#include "FreeRTOS.h"
#include "task.h"

// Function to create the task (called from main or init)
void BlinkLEDTask_Create(void);

#endif
