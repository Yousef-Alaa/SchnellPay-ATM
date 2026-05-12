#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "lcd.h"

static void lcd_trigger() {
    // EN is on the Control Port (PORTA)
    set_bit(LCD_CTRL_PORT, EN);
    _delay_ms(1);
    clear_bit(LCD_CTRL_PORT, EN);
    _delay_ms(1);
}

static void send_nibble(uint8_t nibble) {
    // Clear the specific pins used for D4-D7 on the Data Port (PORTB)
    clear_bit(LCD_DATA_PORT, D4);
    clear_bit(LCD_DATA_PORT, D5);
    clear_bit(LCD_DATA_PORT, D6);
    clear_bit(LCD_DATA_PORT, D7);

    // Set the bits according to the nibble value on the Data Port
    if (read_bit(nibble, 0)) set_bit(LCD_DATA_PORT, D4);
    if (read_bit(nibble, 1)) set_bit(LCD_DATA_PORT, D5);
    if (read_bit(nibble, 2)) set_bit(LCD_DATA_PORT, D6);
    if (read_bit(nibble, 3)) set_bit(LCD_DATA_PORT, D7);
    
    lcd_trigger();
}

void lcd_cmd(unsigned char cmd) {
    // RS = 0 on Control Port
    clear_bit(LCD_CTRL_PORT, RS);
    
    // Send high nibble
    send_nibble((cmd & 0xF0) >> 4);
    
    // Send low nibble
    send_nibble(cmd & 0x0F);
    
    _delay_ms(2);
}

void lcd_data(unsigned char data) {
    // RS = 1 on Control Port
    set_bit(LCD_CTRL_PORT, RS);
    
    // Send high nibble
    send_nibble((data & 0xF0) >> 4);
    
    // Send low nibble
    send_nibble(data & 0x0F);
    
    _delay_ms(2);
}

void lcd_init() {
    // Set Control pins as output
    set_bit(LCD_CTRL_DDR, RS);
    set_bit(LCD_CTRL_DDR, EN);
    
    // Set Data pins as output
    set_bit(LCD_DATA_DDR, D4);
    set_bit(LCD_DATA_DDR, D5);
    set_bit(LCD_DATA_DDR, D6);
    set_bit(LCD_DATA_DDR, D7);
    
    _delay_ms(40);

    // Clear RS initially
    clear_bit(LCD_CTRL_PORT, RS);
    
    send_nibble(0x02);
    lcd_cmd(set_4bit_2line);
    lcd_cmd(cursor_off);
    lcd_cmd(clear_display);
    lcd_cmd(entry_mode_set);
}

void lcd_print(const char *str) {
    // strings ends with '\0' in C
    for (int i = 0; str[i] != '\0'; i++) {
        lcd_data(str[i]);
    }
}

void lcd_print(int num) { 
    if(num > 9999) {
        lcd_print("Err"); 
        return;
    }
    char buffer[5];
    snprintf(buffer, sizeof(buffer), "%d", num);
    lcd_print(buffer); 
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? 0x00 : 0x40;
    address += col;
    lcd_cmd(set_ddrma_addr | address);
}

void lcd_clear() {
    lcd_cmd(clear_display);
}

void lcd_shift_left() {
    lcd_cmd(shift_display_left);
}

void lcd_shift_right() {
    lcd_cmd(shift_display_right);
}

void lcd_cursor_off() {
    lcd_cmd(cursor_off);
}

void lcd_cursor_on() {
    lcd_cmd(cursor_on);
}

void lcd_cursor_blink() {
    lcd_cmd(cursor_blink);
}

void lcd_display_off() {
    lcd_cmd(display_off);
}