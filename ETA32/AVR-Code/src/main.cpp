#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include "common_macros.h"
#include "../lib/lcd/lcd.h"


// ============================================================
// Keypad Rows: PC5, PC4, PC3, PC2
#define KEYPAD_ROW_PORT PORTC
#define KEYPAD_ROW_DIR  DDRC
const uint8_t row_pins[4] = {5, 4, 3, 2}; 

// Keypad Cols: PD7, PD6, PD5, PD3 (Notice the jump to PD3!)
#define KEYPAD_COL_PORT PORTD
#define KEYPAD_COL_DIR  DDRD
#define KEYPAD_COL_PIN  PIND
const uint8_t col_pins[4] = {7, 6, 5, 3}; 

// Lock Mechanism: Moved to Relay 1 (PC7) to avoid LCD pin conflict
#define LOCK_PORT PORTC
#define LOCK_DIR  DDRC
#define LOCK_PIN  7
// ============================================================

class HardwareUART {
public:
    void begin(unsigned long baud) {
        uint16_t ubrr_value = (F_CPU / 16UL / baud) - 1;
        UBRRH = (unsigned char)(ubrr_value >> 8);
        UBRRL = (unsigned char)(ubrr_value);
        UCSRA = 0;                                          
        UCSRB = (1 << RXEN) | (1 << TXEN);                
        UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); 
    }
    
    void write(char data) { while (!(UCSRA & (1 << UDRE))); UDR = data; }
    void print(const char* str) { while (*str) write(*str++); }
    bool available() { return (UCSRA & (1 << RXC)); }
    char read() { while (!available()); return UDR; }

    uint8_t readStringUntil_timeout(char* buf, uint8_t maxLen, uint16_t timeout_ms) {
        uint8_t idx = 0;
        uint32_t elapsed = 0;          
        const uint32_t limit = (uint32_t)timeout_ms * 10; 

        while (elapsed < limit && idx < (maxLen - 1)) {
            if (available()) {
                char c = read();
                elapsed = 0;           
                if (c == '\n') break;  
                if (c != '\r') buf[idx++] = c;
            } else {
                _delay_us(100);
                elapsed++;
            }
        }
        buf[idx] = '\0';
        return idx;                    
    }
};

HardwareUART Serial;

const char keypad[4][4] = {
    {'7', '8', '9', '/'},
    {'4', '5', '6', '*'},
    {'1', '2', '3', '-'},
    {'C', '0', '=', '+'}
};

void keypad_init() {
    // Set columns as inputs and enable pull-ups targeting specific pins
    for(uint8_t i = 0; i < 4; i++) {
        CLEAR_BIT(KEYPAD_COL_DIR, col_pins[i]); 
        SET_BIT(KEYPAD_COL_PORT, col_pins[i]);  
    }

    // Set rows as outputs and idle HIGH targeting specific pins
    for(uint8_t i = 0; i < 4; i++) {
        SET_BIT(KEYPAD_ROW_DIR, row_pins[i]);
        SET_BIT(KEYPAD_ROW_PORT, row_pins[i]);
    }
}

char keypad_getkey() {
    for (uint8_t row = 0; row < 4; row++) {
        CLEAR_BIT(KEYPAD_ROW_PORT, row_pins[row]); // Pull specific row LOW
        _delay_us(10);                  

        for (uint8_t col = 0; col < 4; col++) {
            if (READ_BIT(KEYPAD_COL_PIN, col_pins[col]) == 0) {  // Key pressed?
                _delay_ms(20); // Debounce
                if (READ_BIT(KEYPAD_COL_PIN, col_pins[col]) == 0) {
                    while (READ_BIT(KEYPAD_COL_PIN, col_pins[col]) == 0); // Wait for release
                    SET_BIT(KEYPAD_ROW_PORT, row_pins[row]); // Restore row
                    return keypad[row][col];
                }
            }
        }
        SET_BIT(KEYPAD_ROW_PORT, row_pins[row]); // Restore row HIGH if no press
    }
    return '\0';
}

inline void lock_init()   { SET_BIT(LOCK_DIR,  LOCK_PIN); }
inline void lock_open()   { SET_BIT(LOCK_PORT, LOCK_PIN); }
inline void lock_close()  { CLEAR_BIT(LOCK_PORT, LOCK_PIN); }

enum SystemState { STATE_ENTER_PHONE, STATE_ENTER_OTP, STATE_MAIN_MENU, STATE_ENTER_DEPOSIT, STATE_ENTER_WITHDRAW };

int main(void) {
    Serial.begin(9600);
    lcd_init();
    keypad_init();
    lock_init();
    lock_close();

    SystemState currentState = STATE_ENTER_PHONE;
    char phone_num[15] = {0};
    char otp_code[6] = {0};
    char input_buffer[15];
    uint8_t char_count = 0;

    lcd_clear();
    lcd_print("Enter Phone:");

    while (1) {
        char key = keypad_getkey();
        if (key == '\0') continue;

        if (key == 'C') {
            char_count = 0; input_buffer[0] = '\0'; lcd_clear();
            if (currentState == STATE_ENTER_PHONE) lcd_print("Enter Phone:");
            else if (currentState == STATE_ENTER_OTP) lcd_print("Enter OTP:");
            else if (currentState == STATE_ENTER_DEPOSIT) lcd_print("Dep Amount:");
            else if (currentState == STATE_ENTER_WITHDRAW) lcd_print("Wtd Amount:");
            else if (currentState == STATE_MAIN_MENU) lcd_print("1:Dep  2:Wtd");
        }
        else if (currentState == STATE_MAIN_MENU) {
            if (key == '1') { currentState = STATE_ENTER_DEPOSIT; char_count = 0; lcd_clear(); lcd_print("Dep Amount:"); } 
            else if (key == '2') { currentState = STATE_ENTER_WITHDRAW; char_count = 0; lcd_clear(); lcd_print("Wtd Amount:"); }
        }
        else if (key == '=') {
            input_buffer[char_count] = '\0';
            lcd_clear(); lcd_print("Processing...");

            if (currentState == STATE_ENTER_PHONE) {
                strcpy(phone_num, input_buffer); 
                Serial.print("G,"); Serial.print(phone_num); Serial.write('\n');

                char response[20];
                uint8_t rx = Serial.readStringUntil_timeout(response, sizeof(response), 20000); 
                
                if (rx > 0 && strcmp(response, "PASS") == 0) { currentState = STATE_ENTER_OTP; lcd_clear(); lcd_print("Enter OTP:"); } 
                else { lcd_clear(); lcd_print(response); _delay_ms(3000); lcd_print("Enter Phone:"); lcd_clear(); }
                char_count = 0;
            }
            else if (currentState == STATE_ENTER_OTP) {
                strcpy(otp_code, input_buffer); 
                Serial.print("V,"); Serial.print(phone_num); Serial.print(","); Serial.print(otp_code); Serial.write('\n');

                char response[10];
                uint8_t rx = Serial.readStringUntil_timeout(response, sizeof(response), 20000); 
                
                if (rx > 0 && strcmp(response, "PASS") == 0) { currentState = STATE_MAIN_MENU; lcd_clear(); lcd_print("1:Dep  2:Wtd"); } 
                else { lcd_clear(); lcd_print("Invalid OTP"); _delay_ms(2000); lcd_clear(); lcd_print("Enter OTP:"); }
                char_count = 0;
            }
            else if (currentState == STATE_ENTER_DEPOSIT || currentState == STATE_ENTER_WITHDRAW) {
                if (currentState == STATE_ENTER_DEPOSIT) Serial.print("D,"); else Serial.print("W,");
                Serial.print(phone_num); Serial.print(","); Serial.print(otp_code); Serial.print(","); Serial.print(input_buffer); Serial.write('\n');

                char response[10];
                uint8_t rx = Serial.readStringUntil_timeout(response, sizeof(response), 20000); 
                
                lcd_clear();
                if (rx > 0 && strcmp(response, "PASS") == 0) {
                    lcd_print("Success!");
                    lock_open();     // CLICKS RELAY 1 OPEN!
                    _delay_ms(2000); 
                    lock_close();    // CLOSES RELAY 1
                } else { 
                    lcd_print("Transaction Fail"); 
                }
                
                _delay_ms(3000);
                currentState = STATE_MAIN_MENU; char_count = 0; lcd_clear(); lcd_print("1:Dep  2:Wtd");
            }
        }
        else if (char_count < 14 && currentState != STATE_MAIN_MENU) {
            input_buffer[char_count] = key;
            lcd_set_cursor(1, char_count);
            lcd_data(key); 
            char_count++;
        }
    }
    return 0;
}