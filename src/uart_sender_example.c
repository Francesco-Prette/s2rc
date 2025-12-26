// Example UART sender for second Pico connected to PC
// This Pico receives input from PC via USB serial and forwards to Switch Pico via UART
// Connect: This Pico GP0 (TX) -> Switch Pico GP1 (RX)
//          This Pico GP1 (RX) -> Switch Pico GP0 (TX)
//          Common GND

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <stdio.h>
#include <string.h>

// Button definitions matching the Switch Pico
#define BTN_Y       (1 << 0)
#define BTN_B       (1 << 1)
#define BTN_A       (1 << 2)
#define BTN_X       (1 << 3)
#define BTN_L       (1 << 4)
#define BTN_R       (1 << 5)
#define BTN_ZL      (1 << 6)
#define BTN_ZR      (1 << 7)
#define BTN_MINUS   (1 << 8)
#define BTN_PLUS    (1 << 9)
#define BTN_LSTICK  (1 << 10)
#define BTN_RSTICK  (1 << 11)
#define BTN_HOME    (1 << 12)
#define BTN_CAPTURE (1 << 13)

// D-Pad definitions (HAT values)
#define DPAD_UP        0x00
#define DPAD_UP_RIGHT  0x01
#define DPAD_RIGHT     0x02
#define DPAD_DN_RIGHT  0x03
#define DPAD_DOWN      0x04
#define DPAD_DN_LEFT   0x05
#define DPAD_LEFT      0x06
#define DPAD_UP_LEFT   0x07
#define DPAD_NEUTRAL   0x08

// UART Configuration
#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_BAUD_RATE 115200

typedef struct __attribute__((packed)) {
    uint16_t buttons;
    uint8_t  hat;
    uint8_t  lx;
    uint8_t  ly;
    uint8_t  rx;
    uint8_t  ry;
    uint8_t  vendor;
} controller_state_t;

void send_controller_state(controller_state_t *state) {
    uart_write_blocking(UART_ID, (uint8_t*)state, sizeof(controller_state_t));
}

void print_help() {
    printf("\n=== Nintendo Switch Controller Commands ===\n");
    printf("Buttons: Y B A X L R ZL ZR - + LS RS H C\n");
    printf("D-Pad: U D L R (or combinations like UL, DR)\n");
    printf("Analog: LX:128 LY:128 RX:128 RY:128 (0-255)\n");
    printf("Examples:\n");
    printf("  A           - Press A button\n");
    printf("  A+B         - Press A and B together\n");
    printf("  U           - Press D-Pad Up\n");
    printf("  LX:255      - Move left stick full right\n");
    printf("  A+LX:255    - Press A while moving stick\n");
    printf("Type 'help' to see this message again\n");
    printf("==========================================\n\n");
}

bool parse_button(char *token, uint16_t *buttons) {
    if (strcmp(token, "Y") == 0) { *buttons |= BTN_Y; return true; }
    if (strcmp(token, "B") == 0) { *buttons |= BTN_B; return true; }
    if (strcmp(token, "A") == 0) { *buttons |= BTN_A; return true; }
    if (strcmp(token, "X") == 0) { *buttons |= BTN_X; return true; }
    if (strcmp(token, "L") == 0) { *buttons |= BTN_L; return true; }
    if (strcmp(token, "R") == 0) { *buttons |= BTN_R; return true; }
    if (strcmp(token, "ZL") == 0) { *buttons |= BTN_ZL; return true; }
    if (strcmp(token, "ZR") == 0) { *buttons |= BTN_ZR; return true; }
    if (strcmp(token, "-") == 0) { *buttons |= BTN_MINUS; return true; }
    if (strcmp(token, "+") == 0) { *buttons |= BTN_PLUS; return true; }
    if (strcmp(token, "LS") == 0) { *buttons |= BTN_LSTICK; return true; }
    if (strcmp(token, "RS") == 0) { *buttons |= BTN_RSTICK; return true; }
    if (strcmp(token, "H") == 0) { *buttons |= BTN_HOME; return true; }
    if (strcmp(token, "C") == 0) { *buttons |= BTN_CAPTURE; return true; }
    return false;
}

bool parse_dpad(char *token, uint8_t *hat) {
    if (strcmp(token, "U") == 0) { *hat = DPAD_UP; return true; }
    if (strcmp(token, "D") == 0) { *hat = DPAD_DOWN; return true; }
    if (strcmp(token, "L") == 0) { *hat = DPAD_LEFT; return true; }
    if (strcmp(token, "R") == 0) { *hat = DPAD_RIGHT; return true; }
    if (strcmp(token, "UL") == 0) { *hat = DPAD_UP_LEFT; return true; }
    if (strcmp(token, "UR") == 0) { *hat = DPAD_UP_RIGHT; return true; }
    if (strcmp(token, "DL") == 0) { *hat = DPAD_DN_LEFT; return true; }
    if (strcmp(token, "DR") == 0) { *hat = DPAD_DN_RIGHT; return true; }
    return false;
}

bool parse_analog(char *token, controller_state_t *state) {
    if (strncmp(token, "LX:", 3) == 0) {
        state->lx = atoi(token + 3);
        return true;
    }
    if (strncmp(token, "LY:", 3) == 0) {
        state->ly = atoi(token + 3);
        return true;
    }
    if (strncmp(token, "RX:", 3) == 0) {
        state->rx = atoi(token + 3);
        return true;
    }
    if (strncmp(token, "RY:", 3) == 0) {
        state->ry = atoi(token + 3);
        return true;
    }
    return false;
}

int main(void)
{
    stdio_init_all();
    
    // Initialize LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    
    // Initialize UART
    uart_init(UART_ID, UART_BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);
    
    sleep_ms(2000);
    
    printf("\n=== Nintendo Switch UART Controller Bridge ===\n");
    printf("Pico initialized. UART on GP0/GP1 @ 115200 baud\n");
    printf("Connect: GP0 (TX) -> Switch Pico GP1 (RX)\n");
    printf("         GP1 (RX) -> Switch Pico GP0 (TX)\n");
    printf("         GND -> GND\n");
    
    print_help();
    
    char input_buffer[128];
    uint8_t input_index = 0;
    
    controller_state_t state = {0};
    state.hat = DPAD_NEUTRAL;
    state.lx = 128;
    state.ly = 128;
    state.rx = 128;
    state.ry = 128;
    
    while (true) {
        // Read from USB serial
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\n' || c == '\r') {
                input_buffer[input_index] = '\0';
                
                if (input_index > 0) {
                    // Reset state
                    state.buttons = 0;
                    state.hat = DPAD_NEUTRAL;
                    state.lx = 128;
                    state.ly = 128;
                    state.rx = 128;
                    state.ry = 128;
                    
                    if (strcmp(input_buffer, "help") == 0) {
                        print_help();
                    } else {
                        // Parse the command
                        char *token = strtok(input_buffer, "+");
                        bool valid = false;
                        
                        while (token != NULL) {
                            // Trim spaces
                            while (*token == ' ') token++;
                            
                            if (parse_button(token, &state.buttons)) {
                                valid = true;
                            } else if (parse_dpad(token, &state.hat)) {
                                valid = true;
                            } else if (parse_analog(token, &state)) {
                                valid = true;
                            }
                            
                            token = strtok(NULL, "+");
                        }
                        
                        if (valid) {
                            send_controller_state(&state);
                            gpio_put(PICO_DEFAULT_LED_PIN, 1);
                            printf("Sent: Buttons=0x%04X Hat=%d LX=%d LY=%d RX=%d RY=%d\n", 
                                   state.buttons, state.hat, state.lx, state.ly, state.rx, state.ry);
                            sleep_ms(50);
                            gpio_put(PICO_DEFAULT_LED_PIN, 0);
                        } else {
                            printf("Invalid command. Type 'help' for usage.\n");
                        }
                    }
                }
                
                input_index = 0;
                printf("> ");
            } else if (input_index < sizeof(input_buffer) - 1) {
                input_buffer[input_index++] = c;
                putchar(c);  // Echo
            }
        }
        
        tight_loop_contents();
    }
}
