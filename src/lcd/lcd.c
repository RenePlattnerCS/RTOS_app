// lcd.c
#include "lcd/lcd.h"
#include "main.h" // for GPIO pin definitions
#include "stdio.h"
#include "string.h"

extern TIM_HandleTypeDef htim3; // for microsecond delay

static void delay_us(uint32_t us)
{
    // Elapsed-time approach: no counter reset, so FreeRTOS context switches
    // only EXTEND the delay (fine — LCD requires minimum times, not maximum).
    // uint16_t subtraction wraps correctly for TIM3's 16-bit counter.
    // Safe for us <= 32767; all LCD delays are <= 5000 us.
    uint16_t start = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
    while ((uint16_t)((uint16_t)__HAL_TIM_GET_COUNTER(&htim3) - start) < (uint16_t)us)
        ;
}

static void lcd_send_nibble(uint8_t nibble)
{
    HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (nibble >> 0) & 1);
    HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (nibble >> 1) & 1);
    HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (nibble >> 2) & 1);
    HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (nibble >> 3) & 1);

    // Pulse enable
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, GPIO_PIN_RESET);
    delay_us(100);
}

static void lcd_send_byte(uint8_t byte, uint8_t is_data)
{
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, is_data ? GPIO_PIN_SET : GPIO_PIN_RESET);

    lcd_send_nibble(byte >> 4);   // high nibble first
    lcd_send_nibble(byte & 0x0F); // low nibble second
}

void lcd_init(void)
{
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);

    lcd_send_nibble(0x03);
    delay_us(5000);
    lcd_send_nibble(0x03);
    delay_us(200);
    lcd_send_nibble(0x03);
    delay_us(200);
    lcd_send_nibble(0x02);
    delay_us(200);

    lcd_send_byte(0x28, 0);
    delay_us(50);
    lcd_send_byte(0x0C, 0);
    delay_us(50);
    lcd_send_byte(0x06, 0);
    delay_us(50);
    lcd_send_byte(0x01, 0);
    delay_us(2000);
}

void lcd_clear(void)
{
    lcd_send_byte(0x01, 0);
    delay_us(2000);
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t row_offsets[] = {0x00, 0x40}; // row 0 and row 1 DDRAM addresses
    lcd_send_byte(0x80 | (row_offsets[row] + col), 0);
}

void lcd_print(const char *str)
{
    while (*str)
        lcd_send_byte(*str++, 1);
}

void lcd_print_int(int value)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    lcd_print(buf);
}