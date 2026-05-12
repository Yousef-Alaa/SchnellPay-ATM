#ifndef LCD_H
#define LCD_H

#include <avr/io.h>

#include "../bitmasking.h"

#define LCD_PORT PORTA
#define LCD_DDR DDRA

#define RS PA1
#define EN PA2

#define D4 PA3
#define D5 PA4
#define D6 PA5
#define D7 PA6 

#define clear_display 0x01
#define return_home 0x02
#define entry_mode_set 0x06
#define cursor_off 0x0C
#define display_off 0x08
#define cursor_on 0x0E
#define cursor_blink 0x0F
#define set_4bit_2line 0x28
#define set_ddrma_addr 0x80

#define shift_display_left 0x18
#define shift_display_right 0x1C

void lcd_init(); 
void lcd_cmd(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_print(const char *str);
void lcd_print(int num);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_clear();
void lcd_shift_left();
void lcd_shift_right();
void lcd_cursor_off();
void lcd_cursor_on();
void lcd_cursor_blink();
void lcd_display_off();

#endif
