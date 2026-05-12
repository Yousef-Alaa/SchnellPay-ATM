#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include "common_macros.h"
#include "../lib/lcd/lcd.h"

// ============================================================
//  UART CLASS
//  Baud rate calculated automatically from F_CPU.
//  At 16 MHz + 9600 baud  →  UBRR = 103  (correct)
// ============================================================
class HardwareUART {
public:
    void begin(unsigned long baud) {
        uint16_t ubrr_value = (F_CPU / 16UL / baud) - 1;
        UBRRH = (unsigned char)(ubrr_value >> 8);
        UBRRL = (unsigned char)(ubrr_value);
        UCSRA = 0;                                          // make sure U2X is off
        UCSRB = (1 << RXEN) | (1 << TXEN);                 // enable RX + TX
        UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); // 8-N-1
    }

    // --- TX helpers ---
    void write(char data) {
        while (!(UCSRA & (1 << UDRE)));
        UDR = data;
    }

    void print(const char* str) {
        while (*str) write(*str++);
    }

    void println(const char* str) {
        print(str);
        write('\r');
        write('\n');
    }

    // --- RX helpers ---
    bool available() {
        return (UCSRA & (1 << RXC));
    }

    char read() {
        while (!available());
        return UDR;
    }

    /*
     * readStringUntil_timeout()
     * Reads characters into buf until:
     *   - a '\n' terminator is found, OR
     *   - timeout_ms milliseconds pass with no new byte, OR
     *   - buf is full (maxLen-1 chars)
     *
     * Returns the number of characters stored (0 = timeout / empty).
     */
    uint8_t readStringUntil_timeout(char* buf, uint8_t maxLen,
                                    uint16_t timeout_ms) {
        uint8_t  idx     = 0;
        uint32_t elapsed = 0;          // counts in ~100 µs steps
        const uint32_t limit = (uint32_t)timeout_ms * 10; // steps per ms × ms

        while (elapsed < limit && idx < (maxLen - 1)) {
            if (available()) {
                char c = read();
                elapsed = 0;           // reset watchdog on any incoming byte
                if (c == '\n') break;  // terminator received → done
                if (c != '\r') {
                    buf[idx++] = c;
                }
            } else {
                _delay_us(100);
                elapsed++;
            }
        }

        buf[idx] = '\0';
        return idx;                    // 0 means timeout with nothing received
    }
};

HardwareUART Serial;

// ============================================================
//  KEYPAD
//  Rows  → PB4..PB7  (output, active-LOW drive)
//  Cols  → PD2..PD5  (input,  pull-up enabled)
// ============================================================
const char keypad[4][4] = {
    {'7', '8', '9', '/'},
    {'4', '5', '6', '*'},
    {'1', '2', '3', '-'},
    {'C', '0', '=', '+'}
};

void keypad_init() {
    DDRD  &= ~0x3C;   // PD2-PD5 as inputs
    PORTD |=  0x3C;   // enable pull-ups on PD2-PD5
    DDRB  |=  0xF0;   // PB4-PB7 as outputs
    PORTB |=  0xF0;   // rows idle HIGH
}

char keypad_getkey() {
    for (uint8_t row = 4; row < 8; row++) {
        CLEAR_BIT(PORTB, row);          // pull row LOW
        _delay_us(10);                  // let lines settle

        for (uint8_t col = 2; col < 6; col++) {
            if (READ_BIT(PIND, col) == 0) {  // key pressed?
                _delay_ms(20);               // debounce
                if (READ_BIT(PIND, col) == 0) {
                    while (READ_BIT(PIND, col) == 0); // wait for release
                    SET_BIT(PORTB, row);
                    return keypad[row - 4][col - 2];
                }
            }
        }
        SET_BIT(PORTB, row);            // restore row HIGH
    }
    return '\0';
}

// ============================================================
//  LOCK / LED pin
//  PB0 — HIGH = unlocked / LED on
// ============================================================
#define LOCK_DDR   DDRB
#define LOCK_PORT  PORTB
#define LOCK_PIN   DDB0

inline void lock_init()   { SET_BIT(LOCK_DDR,  LOCK_PIN); }
inline void lock_open()   { SET_BIT(LOCK_PORT, LOCK_PIN); }
inline void lock_close()  { CLEAR_BIT(LOCK_PORT, LOCK_PIN); }

enum SystemState {
    STATE_ENTER_PHONE,
    STATE_ENTER_OTP,
    STATE_MAIN_MENU,
    STATE_ENTER_DEPOSIT,
    STATE_ENTER_WITHDRAW
};

int main(void) {
    Serial.begin(9600);
    lcd_init();
    keypad_init();
    lock_init();
    lock_close();

    SystemState currentState = STATE_ENTER_PHONE;
    
    // Memory to hold user session data
    char phone_num[15] = {0};
    char otp_code[6] = {0};
    
    char input_buffer[15];
    uint8_t char_count = 0;

    lcd_clear();
    lcd_print("Enter Phone:");

    while (1) {
        char key = keypad_getkey();
        if (key == '\0') continue;

        // --- HANDLE 'C' (CLEAR) ---
        if (key == 'C') {
            char_count = 0;
            input_buffer[0] = '\0';
            lcd_clear();
            if (currentState == STATE_ENTER_PHONE) lcd_print("Enter Phone:");
            else if (currentState == STATE_ENTER_OTP) lcd_print("Enter OTP:");
            else if (currentState == STATE_ENTER_DEPOSIT) lcd_print("Dep Amount:");
            else if (currentState == STATE_ENTER_WITHDRAW) lcd_print("Wtd Amount:");
            else if (currentState == STATE_MAIN_MENU) lcd_print("1:Dep  2:Wtd");
        }

        // --- HANDLE MAIN MENU SELECTION ---
        else if (currentState == STATE_MAIN_MENU) {
            if (key == '1') {
                currentState = STATE_ENTER_DEPOSIT;
                char_count = 0;
                lcd_clear();
                lcd_print("Dep Amount:");
            } else if (key == '2') {
                currentState = STATE_ENTER_WITHDRAW;
                char_count = 0;
                lcd_clear();
                lcd_print("Wtd Amount:");
            }
        }

        // --- HANDLE '=' (SUBMIT) ---
        else if (key == '=') {
            input_buffer[char_count] = '\0';
            lcd_clear();
            lcd_print("Processing...");

            // 1. GENERATE PIN FLOW
            if (currentState == STATE_ENTER_PHONE) {
                strcpy(phone_num, input_buffer); // Save phone to memory
                
                Serial.print("G,");
                Serial.print(phone_num);
                Serial.write('\n');

                char response[20];
                uint8_t rx = Serial.readStringUntil_timeout(response, sizeof(response), 20000);
                
                if (rx > 0 && strcmp(response, "PASS") == 0) {
                    currentState = STATE_ENTER_OTP;
                    lcd_clear();
                    lcd_print("Enter OTP:");
                } else {
                    lcd_clear();
                    // lcd_print("Phone Failed");
                    // _delay_ms(2000);
                    // lcd_clear();
                    lcd_print(response);
                    _delay_ms(3000);
                    lcd_print("Enter Phone:");
                    lcd_clear();

                }
                char_count = 0;
            }

            // 2. VERIFY FLOW
            else if (currentState == STATE_ENTER_OTP) {
                strcpy(otp_code, input_buffer); // Save OTP to memory
                
                Serial.print("V,");
                Serial.print(phone_num);
                Serial.print(",");
                Serial.print(otp_code);
                Serial.write('\n');

                char response[10];
                uint8_t rx = Serial.readStringUntil_timeout(response, sizeof(response), 10000);
                
                if (rx > 0 && strcmp(response, "PASS") == 0) {
                    currentState = STATE_MAIN_MENU;
                    lcd_clear();
                    lcd_print("1:Dep  2:Wtd");
                } else {
                    lcd_clear();
                    lcd_print("Invalid OTP");
                    _delay_ms(2000);
                    lcd_clear();
                    lcd_print("Enter OTP:");
                }
                char_count = 0;
            }

            // 3. DEPOSIT OR WITHDRAW FLOW
            else if (currentState == STATE_ENTER_DEPOSIT || currentState == STATE_ENTER_WITHDRAW) {
                if (currentState == STATE_ENTER_DEPOSIT) Serial.print("D,");
                else Serial.print("W,");
                
                Serial.print(phone_num);
                Serial.print(",");
                Serial.print(otp_code);
                Serial.print(",");
                Serial.print(input_buffer); // The amount
                Serial.write('\n');

                char response[10];
                uint8_t rx = Serial.readStringUntil_timeout(response, sizeof(response), 15000);
                
                lcd_clear();
                if (rx > 0 && strcmp(response, "PASS") == 0) {
                    lcd_print("Success!");
                } else {
                    lcd_print("Transaction Fail");
                }
                
                _delay_ms(3000);
                
                // Return to Main Menu after transaction
                currentState = STATE_MAIN_MENU;
                char_count = 0;
                lcd_clear();
                lcd_print("1:Dep  2:Wtd");
            }
        }

        // --- HANDLE TYPING NUMBERS ---
        else if (char_count < 14 && currentState != STATE_MAIN_MENU) {
            input_buffer[char_count] = key;
            lcd_set_cursor(1, char_count);
            lcd_data(key); // Show the numbers being typed
            char_count++;
        }
    }
    return 0;
}