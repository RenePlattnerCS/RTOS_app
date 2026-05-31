// lcd.c
#include "lcd/lcd.h"
#include "FreeRTOS.h"
#include "main.h"
#include "stdio.h"
#include "string.h"
#include "task.h"   // for vTaskDelay in lcd_init

extern TIM_HandleTypeDef htim3;

// ---------------------------------------------------------------------------
// Delay
// Elapsed-time approach: no counter reset, so FreeRTOS context switches
// only EXTEND the delay (fine — LCD requires minimum times, not maximum).
// uint16_t subtraction wraps correctly for TIM3's 16-bit counter.
// Safe for us <= 32767; all LCD delays are <= 5000 us.
// ---------------------------------------------------------------------------
static void delay_us(uint32_t us)
{
    uint16_t start = (uint16_t) __HAL_TIM_GET_COUNTER(&htim3);
    while ((uint16_t) ((uint16_t) __HAL_TIM_GET_COUNTER(&htim3) - start) < (uint16_t) us)
        ;
}

// ---------------------------------------------------------------------------
// Nibble / byte primitives
// ---------------------------------------------------------------------------

// Send one nibble (4 bits) and pulse E.
// Must be called inside taskENTER_CRITICAL to prevent inter-nibble preemption.
static void lcd_send_nibble(uint8_t nibble)
{
    HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (nibble >> 0) & 1);
    HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (nibble >> 1) & 1);
    HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (nibble >> 2) & 1);
    HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (nibble >> 3) & 1);

    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);
    delay_us(100);
}

// Send one full byte as two nibbles.
// Critical section protects the two nibbles from being split by preemption.
// Critical section is ~220us — acceptable for all tasks in this project.
static void lcd_send_byte(uint8_t byte, uint8_t is_data)
{
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, is_data ? GPIO_PIN_SET : GPIO_PIN_RESET);
    lcd_send_nibble(byte >> 4);
    lcd_send_nibble(byte & 0x0F);
    // No critical section: HD44780 has no maximum inter-nibble timeout.
    // taskENTER_CRITICAL here was masking USB interrupts for 220us per byte
    // (3.5ms per lcd_print call), causing USB frames to be missed.
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// lcd_init must be called from a FreeRTOS task context (not from main before
// vTaskStartScheduler) because it uses vTaskDelay for the power-on wait.
void lcd_init(void)
{
    // Power-on wait — yields to other tasks, must not be in critical section
    vTaskDelay(pdMS_TO_TICKS(200));

    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);

    // 4-bit init sequence per HD44780 datasheet.
    // No critical section here — these delays are minimums, preemption only
    // extends them which is safe. Critical sections are per-byte in
    // lcd_send_byte once we switch to 4-bit mode.
    lcd_send_nibble(0x03);
    delay_us(4500); // must be > 4.1ms
    lcd_send_nibble(0x03);
    delay_us(200); // must be > 100us
    lcd_send_nibble(0x03);
    delay_us(200);
    lcd_send_nibble(0x02);
    delay_us(200); // switch to 4-bit

    lcd_send_byte(0x28, 0);
    delay_us(50); // 4-bit, 2 lines, 5x8 font
    lcd_send_byte(0x0C, 0);
    delay_us(50); // display on, cursor off
    lcd_send_byte(0x06, 0);
    delay_us(50); // entry mode: increment, no shift
    lcd_send_byte(0x01, 0);
    delay_us(2000); // clear display, needs 2ms
}

void lcd_clear(void)
{
    lcd_send_byte(0x01, 0);
    delay_us(2000); // clear command needs 2ms minimum
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t row_offsets[] = {0x00, 0x40}; // DDRAM addresses for row 0 and 1
    lcd_send_byte(0x80 | (row_offsets[row] + col), 0);
}

void lcd_print(const char *str)
{
    while (*str)
        lcd_send_byte(*str++, 1); // each byte has its own critical section
}

void lcd_print_int(int value)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    lcd_print(buf);
}