// lcd.h
#ifndef LCD_H
#define LCD_H

#include "stm32f4xx_hal.h" // adjust for your STM32 family

void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print(const char *str);
void lcd_print_int(int value);

#endif