// UART Bridge for Nintendo Switch Controller with USB Keyboard Support
// This Pico receives input from:
//   1. PC via USB serial (text commands)
//   2. USB Keyboard via USB Host (key presses)
// and forwards to Switch Pico via UART
//
// Connect: This Pico GP0 (TX) -> Switch Pico GP1 (RX)
//          This Pico GP1 (RX) -> Switch Pico GP0 (TX)
//          Common GND
//
// USB Keyboard: Connect via USB OTG adapter to Pico's USB port

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Button definitions matching the Switch Pico
// Standard Nintendo Switch HID button order: B, A, Y, X, L, R, ZL, ZR, -, +, LS, RS, Home, Capture
#define BTN_B       (1 << 0)
#define BTN_A       (1 << 1)
#define BTN_Y       (1 << 2)
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
#define BTN_GL      (1 << 14)  // Grip Left / Back Left
#define BTN_GR      (1 << 15)  // Grip Right / Back Right

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

// Keyboard state
static bool keyboard_mounted = false;
static uint8_t keyboard_dev_addr = 0;
static uint8_t keyboard_instance = 0;
static controller_state_t kbd_state = {0};
static controller_state_t last_sent_state = {0};
static uint32_t last_send_time = 0;

// Key mapping types
#define MAP_BUTTON 0
#define MAP_DPAD   1
#define MAP_LSTICK 2
#define MAP_RSTICK 3

// Stick direction flags
#define STICK_UP    (1 << 0)
#define STICK_DOWN  (1 << 1)
#define STICK_LEFT  (1 << 2)
#define STICK_RIGHT (1 << 3)

// Configurable key mappings (HID keycodes)
typedef struct {
    uint8_t key;
    uint16_t button;
    uint8_t hat;
    uint8_t stick_dir;  // For analog stick directions
    uint8_t map_type;   // MAP_BUTTON, MAP_DPAD, MAP_LSTICK, MAP_RSTICK
} key_mapping_t;

// Default keyboard mappings (WASD + IJKL layout + analog stick controls)
static key_mapping_t key_mappings[] = {
    // D-Pad: WASD
    {HID_KEY_W, 0, DPAD_UP, 0, MAP_DPAD},
    {HID_KEY_S, 0, DPAD_DOWN, 0, MAP_DPAD},
    {HID_KEY_A, 0, DPAD_LEFT, 0, MAP_DPAD},
    {HID_KEY_D, 0, DPAD_RIGHT, 0, MAP_DPAD},
    
    // Face buttons: IJKL
    {HID_KEY_I, BTN_X, 0, 0, MAP_BUTTON},     // I -> X (top)
    {HID_KEY_K, BTN_B, 0, 0, MAP_BUTTON},     // K -> B (right)
    {HID_KEY_J, BTN_Y, 0, 0, MAP_BUTTON},     // J -> Y (left)
    {HID_KEY_L, BTN_A, 0, 0, MAP_BUTTON},     // L -> A (bottom)
    
    // Shoulders: Q E R F (and G T as alternatives)
    {HID_KEY_Q, BTN_L, 0, 0, MAP_BUTTON},
    {HID_KEY_E, BTN_R, 0, 0, MAP_BUTTON},
    {HID_KEY_R, BTN_ZL, 0, 0, MAP_BUTTON},
    {HID_KEY_F, BTN_ZR, 0, 0, MAP_BUTTON},
    {HID_KEY_G, BTN_L, 0, 0, MAP_BUTTON},     // G -> L (alternative)
    {HID_KEY_T, BTN_R, 0, 0, MAP_BUTTON},     // T -> R (alternative)
    
    // System buttons
    {HID_KEY_1, BTN_MINUS, 0, 0, MAP_BUTTON},
    {HID_KEY_2, BTN_PLUS, 0, 0, MAP_BUTTON},
    {HID_KEY_3, BTN_LSTICK, 0, 0, MAP_BUTTON},
    {HID_KEY_4, BTN_RSTICK, 0, 0, MAP_BUTTON},
    {HID_KEY_H, BTN_HOME, 0, 0, MAP_BUTTON},
    {HID_KEY_C, BTN_CAPTURE, 0, 0, MAP_BUTTON},
    
    // Arrow keys as alternative dpad
    {HID_KEY_ARROW_UP, 0, DPAD_UP, 0, MAP_DPAD},
    {HID_KEY_ARROW_DOWN, 0, DPAD_DOWN, 0, MAP_DPAD},
    {HID_KEY_ARROW_LEFT, 0, DPAD_LEFT, 0, MAP_DPAD},
    {HID_KEY_ARROW_RIGHT, 0, DPAD_RIGHT, 0, MAP_DPAD},
    
    // Left Analog Stick: Numpad 8456
    {HID_KEY_KEYPAD_8, 0, 0, STICK_UP, MAP_LSTICK},      // Numpad 8 -> Left stick up
    {HID_KEY_KEYPAD_5, 0, 0, STICK_DOWN, MAP_LSTICK},    // Numpad 5 -> Left stick down
    {HID_KEY_KEYPAD_4, 0, 0, STICK_LEFT, MAP_LSTICK},    // Numpad 4 -> Left stick left
    {HID_KEY_KEYPAD_6, 0, 0, STICK_RIGHT, MAP_LSTICK},   // Numpad 6 -> Left stick right
    
    // Right Analog Stick: UHJK  
    {HID_KEY_U, 0, 0, STICK_UP, MAP_RSTICK},     // U -> Right stick up
    {HID_KEY_M, 0, 0, STICK_DOWN, MAP_RSTICK},   // M -> Right stick down
    {HID_KEY_N, 0, 0, STICK_LEFT, MAP_RSTICK},   // N -> Right stick left
    {HID_KEY_COMMA, 0, 0, STICK_RIGHT, MAP_RSTICK}, // , -> Right stick right
};

#define NUM_KEY_MAPPINGS (sizeof(key_mappings) / sizeof(key_mappings[0]))

void send_controller_state(controller_state_t *state) {
    uart_write_blocking(UART_ID, (uint8_t*)state, sizeof(controller_state_t));
}

void print_help() {
    printf("\n=== Nintendo Switch Controller Bridge ===\n");
    printf("Two input modes:\n\n");
    printf("1. SERIAL COMMANDS (type in terminal):\n");
    printf("   Buttons: Y B A X L R ZL ZR - + LS RS H C GL GR\n");
    printf("   D-Pad: U D L R (or UL, DR, etc.)\n");
    printf("   Analog: LX:128 LY:128 RX:128 RY:128 (0-255)\n");
    printf("   Examples: A, A+B, U, LX:255, A+LX:255, GL+GR\n\n");
    printf("2. USB KEYBOARD (plug in USB keyboard):\n");
    printf("   D-Pad: WASD or Arrow Keys\n");
    printf("   Buttons: I=X, K=B, J=Y, L=A\n");
    printf("   Shoulders: Q/G=L, E/T=R, R=ZL, F=ZR\n");
    printf("   System: 1=-, 2=+, 3=LS, 4=RS, H=Home, C=Capture\n");
    printf("   Left Stick: Numpad 8456 (Up/Down/Left/Right)\n");
    printf("   Right Stick: U M N , (Up/Down/Left/Right)\n");
    printf("   ** Hold keys to keep buttons/sticks pressed! **\n");
    printf("\nType 'help' to see this message again\n");
    printf("=========================================\n\n");
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
    if (strcmp(token, "GL") == 0) { *buttons |= BTN_GL; return true; }
    if (strcmp(token, "GR") == 0) { *buttons |= BTN_GR; return true; }
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

// Process keyboard report and update controller state
void process_keyboard_report(hid_keyboard_report_t const *report) {
    // Reset state
    kbd_state.buttons = 0;
    kbd_state.hat = DPAD_NEUTRAL;
    
    // Reset analog sticks to center
    kbd_state.lx = 128;
    kbd_state.ly = 128;
    kbd_state.rx = 128;
    kbd_state.ry = 128;
    
    uint8_t dpad_up = 0, dpad_down = 0, dpad_left = 0, dpad_right = 0;
    uint8_t lstick_dirs = 0;  // Bit flags for left stick directions
    uint8_t rstick_dirs = 0;  // Bit flags for right stick directions
    
    // Check each pressed key (up to 6 simultaneous keys)
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t keycode = report->keycode[i];
        if (keycode == 0) continue;
        
        // Look up key in mappings
        for (uint8_t j = 0; j < NUM_KEY_MAPPINGS; j++) {
            if (key_mappings[j].key == keycode) {
                switch (key_mappings[j].map_type) {
                    case MAP_BUTTON:
                        kbd_state.buttons |= key_mappings[j].button;
                        break;
                        
                    case MAP_DPAD:
                        // D-Pad handling - track individual directions
                        switch (key_mappings[j].hat) {
                            case DPAD_UP: dpad_up = 1; break;
                            case DPAD_DOWN: dpad_down = 1; break;
                            case DPAD_LEFT: dpad_left = 1; break;
                            case DPAD_RIGHT: dpad_right = 1; break;
                        }
                        break;
                        
                    case MAP_LSTICK:
                        lstick_dirs |= key_mappings[j].stick_dir;
                        break;
                        
                    case MAP_RSTICK:
                        rstick_dirs |= key_mappings[j].stick_dir;
                        break;
                }
                break;
            }
        }
    }
    
    // Calculate combined D-Pad state from individual directions
    if (dpad_up && dpad_right) {
        kbd_state.hat = DPAD_UP_RIGHT;
    } else if (dpad_up && dpad_left) {
        kbd_state.hat = DPAD_UP_LEFT;
    } else if (dpad_down && dpad_right) {
        kbd_state.hat = DPAD_DN_RIGHT;
    } else if (dpad_down && dpad_left) {
        kbd_state.hat = DPAD_DN_LEFT;
    } else if (dpad_up) {
        kbd_state.hat = DPAD_UP;
    } else if (dpad_down) {
        kbd_state.hat = DPAD_DOWN;
    } else if (dpad_left) {
        kbd_state.hat = DPAD_LEFT;
    } else if (dpad_right) {
        kbd_state.hat = DPAD_RIGHT;
    } else {
        kbd_state.hat = DPAD_NEUTRAL;
    }
    
    // Calculate left stick position from direction flags
    if (lstick_dirs & STICK_UP) {
        kbd_state.ly = 0;  // Y-axis inverted: 0 = up
    } else if (lstick_dirs & STICK_DOWN) {
        kbd_state.ly = 255;  // Y-axis inverted: 255 = down
    }
    
    if (lstick_dirs & STICK_LEFT) {
        kbd_state.lx = 0;  // 0 = left
    } else if (lstick_dirs & STICK_RIGHT) {
        kbd_state.lx = 255;  // 255 = right
    }
    
    // Calculate right stick position from direction flags
    if (rstick_dirs & STICK_UP) {
        kbd_state.ry = 0;  // Y-axis inverted: 0 = up
    } else if (rstick_dirs & STICK_DOWN) {
        kbd_state.ry = 255;  // Y-axis inverted: 255 = down
    }
    
    if (rstick_dirs & STICK_LEFT) {
        kbd_state.rx = 0;  // 0 = left
    } else if (rstick_dirs & STICK_RIGHT) {
        kbd_state.rx = 255;  // 255 = right
    }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    
    if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        printf("\n[USB] Keyboard mounted on address %d, instance %d\n", dev_addr, instance);
        keyboard_mounted = true;
        keyboard_dev_addr = dev_addr;
        keyboard_instance = instance;
        
        // Request to receive reports
        if (!tuh_hid_receive_report(dev_addr, instance)) {
            printf("[USB] Error: cannot request report\n");
        }
    }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    if (keyboard_mounted && keyboard_dev_addr == dev_addr && keyboard_instance == instance) {
        printf("\n[USB] Keyboard unmounted\n");
        keyboard_mounted = false;
        keyboard_dev_addr = 0;
        
        // Reset keyboard state
        kbd_state.buttons = 0;
        kbd_state.hat = DPAD_NEUTRAL;
    }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
    if (keyboard_mounted && keyboard_dev_addr == dev_addr && keyboard_instance == instance) {
        hid_keyboard_report_t const *kbd_report = (hid_keyboard_report_t const *)report;
        process_keyboard_report(kbd_report);
        
        // Request next report
        tuh_hid_receive_report(dev_addr, instance);
    }
}

// Send keyboard state periodically
void update_keyboard_state() {
    if (!keyboard_mounted) return;
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    // Send updates at ~125Hz (8ms) if state changed, or every 100ms to maintain connection
    bool state_changed = (kbd_state.buttons != last_sent_state.buttons) || 
                         (kbd_state.hat != last_sent_state.hat) ||
                         (kbd_state.lx != last_sent_state.lx) ||
                         (kbd_state.ly != last_sent_state.ly) ||
                         (kbd_state.rx != last_sent_state.rx) ||
                         (kbd_state.ry != last_sent_state.ry);
    
    if (state_changed || (now - last_send_time >= 100)) {
        send_controller_state(&kbd_state);
        last_sent_state = kbd_state;
        last_send_time = now;
        
        if (state_changed) {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            printf("[KBD] Buttons=0x%04X Hat=%d LX=%d LY=%d RX=%d RY=%d\n", 
                   kbd_state.buttons, kbd_state.hat,
                   kbd_state.lx, kbd_state.ly, kbd_state.rx, kbd_state.ry);
        }
    }
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
    
    // Initialize keyboard state
    kbd_state.hat = DPAD_NEUTRAL;
    kbd_state.lx = 128;
    kbd_state.ly = 128;
    kbd_state.rx = 128;
    kbd_state.ry = 128;
    
    last_sent_state = kbd_state;
    
    sleep_ms(2000);
    
    printf("\n=== Nintendo Switch UART Controller Bridge ===\n");
    printf("Pico initialized. UART on GP0/GP1 @ 115200 baud\n");
    printf("Connect: GP0 (TX) -> Switch Pico GP1 (RX)\n");
    printf("         GP1 (RX) -> Switch Pico GP0 (TX)\n");
    printf("         GND -> GND\n");
    printf("\nInitializing USB Host for keyboard...\n");
    
    // Initialize TinyUSB host stack
    tusb_init();
    
    print_help();
    
    char input_buffer[128];
    uint8_t input_index = 0;
    
    controller_state_t serial_state = {0};
    serial_state.hat = DPAD_NEUTRAL;
    serial_state.lx = 128;
    serial_state.ly = 128;
    serial_state.rx = 128;
    serial_state.ry = 128;
    
    printf("> ");
    
    uint32_t led_off_time = 0;
    
    while (true) {
        // Process USB host events
        tuh_task();
        
        // Update keyboard state and send to UART
        update_keyboard_state();
        
        // Turn off LED after brief blink
        if (led_off_time > 0 && to_ms_since_boot(get_absolute_time()) >= led_off_time) {
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            led_off_time = 0;
        }
        
        // Read from USB serial (for text commands)
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\n' || c == '\r') {
                input_buffer[input_index] = '\0';
                
                if (input_index > 0) {
                    // Reset state
                    serial_state.buttons = 0;
                    serial_state.hat = DPAD_NEUTRAL;
                    serial_state.lx = 128;
                    serial_state.ly = 128;
                    serial_state.rx = 128;
                    serial_state.ry = 128;
                    
                    if (strcmp(input_buffer, "help") == 0) {
                        print_help();
                    } else {
                        // Parse the command
                        char *token = strtok(input_buffer, "+");
                        bool valid = false;
                        uint16_t buttons = 0;
                        uint8_t hat = DPAD_NEUTRAL;
                        
                        while (token != NULL) {
                            // Trim spaces
                            while (*token == ' ') token++;
                            
                            if (parse_button(token, &buttons)) {
                                valid = true;
                            } else if (parse_dpad(token, &hat)) {
                                valid = true;
                            } else if (parse_analog(token, &serial_state)) {
                                valid = true;
                            }
                            
                            token = strtok(NULL, "+");
                        }
                        
                        // Copy parsed values to state
                        serial_state.buttons = buttons;
                        serial_state.hat = hat;
                        
                        if (valid) {
                            send_controller_state(&serial_state);
                            gpio_put(PICO_DEFAULT_LED_PIN, 1);
                            printf("Sent: Buttons=0x%04X Hat=%d LX=%d LY=%d RX=%d RY=%d\n", 
                                   serial_state.buttons, serial_state.hat, 
                                   serial_state.lx, serial_state.ly, 
                                   serial_state.rx, serial_state.ry);
                            led_off_time = to_ms_since_boot(get_absolute_time()) + 50;
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
