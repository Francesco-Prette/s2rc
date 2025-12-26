#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"

// Button definitions (16 buttons total for Switch Pro Controller)
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

#define REPORT_INTERVAL_MS 8  // Send reports every 8ms (125Hz polling rate)

// Test mode: Uncomment to enable button test loop (cycles through all buttons)
// #define TEST_MODE_ENABLED

// UART Configuration
#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_BAUD_RATE 115200

// UART Protocol: 8 bytes matching HID report structure
// Byte 0-1: Button state (uint16_t, little endian)
// Byte 2: Hat switch (D-pad)
// Byte 3: Left stick X (0-255)
// Byte 4: Left stick Y (0-255)
// Byte 5: Right stick X (0-255)
// Byte 6: Right stick Y (0-255)
// Byte 7: Vendor byte (unused, set to 0)

typedef struct __attribute__((packed)) {
    uint16_t buttons;     // 2 bytes: 14 buttons + 2 bits padding
    uint8_t  hat;         // 1 byte: Hat switch (upper 4 bits) + padding (lower 4 bits)
    uint8_t  lx;          // Left stick X
    uint8_t  ly;          // Left stick Y
    uint8_t  rx;          // Right stick X (Z axis)
    uint8_t  ry;          // Right stick Y (Rz axis)
    uint8_t  vendor;      // Vendor specific byte
} hid_report_t;  // Total: 8 bytes

int main(void)
{
    stdio_init_all();
    
    // Initialize LED pin
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    
    // Initialize UART on GP0 (TX) and GP1 (RX)
    uart_init(UART_ID, UART_BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Enable UART FIFO
    uart_set_fifo_enabled(UART_ID, true);
    
    sleep_ms(2000);

    tusb_init();

    // Initialize report with neutral state
    hid_report_t current_report = {0};
    current_report.hat = 0x08;  // Neutral D-pad (GP2040-CE uses 0x08 for SWITCH_HAT_NOTHING)
    current_report.lx = 128;    // Center
    current_report.ly = 128;
    current_report.rx = 128;
    current_report.ry = 128;
    current_report.vendor = 0;

    absolute_time_t last_report = get_absolute_time();
    uint8_t uart_buffer[8];
    uint8_t buffer_index = 0;
    enum { WAIT_HEADER1, WAIT_HEADER2, READ_DATA } uart_state = WAIT_HEADER1;

    // LED blink to indicate ready
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(500);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

#ifdef TEST_MODE_ENABLED
    printf("\n=== BUTTON TEST MODE ENABLED ===\n");
    printf("Cycling through all buttons (except HOME and CAPTURE)\n");
    printf("Each button will be pressed for 1 second\n\n");
    
    // Test button array (excluding HOME and CAPTURE)
    typedef struct {
        uint16_t button;
        const char* name;
    } button_test_t;
    
    button_test_t test_buttons[] = {
        {BTN_B, "B"},
        {BTN_A, "A"},
        {BTN_Y, "Y"},
        {BTN_X, "X"},
        {BTN_L, "L"},
        {BTN_R, "R"},
        {BTN_ZL, "ZL"},
        {BTN_ZR, "ZR"},
        {BTN_MINUS, "MINUS (-)"},
        {BTN_PLUS, "PLUS (+)"},
        {BTN_LSTICK, "L-STICK"},
        {BTN_RSTICK, "R-STICK"},
        {BTN_GL, "GL (Grip Left)"},
        {BTN_GR, "GR (Grip Right)"}
    };
    
    // D-Pad test values
    typedef struct {
        uint8_t hat;
        const char* name;
    } dpad_test_t;
    
    dpad_test_t test_dpads[] = {
        {0x00, "D-PAD UP"},
        {0x01, "D-PAD UP-RIGHT"},
        {0x02, "D-PAD RIGHT"},
        {0x03, "D-PAD DOWN-RIGHT"},
        {0x04, "D-PAD DOWN"},
        {0x05, "D-PAD DOWN-LEFT"},
        {0x06, "D-PAD LEFT"},
        {0x07, "D-PAD UP-LEFT"}
    };
    
    uint32_t test_index = 0;
    uint32_t test_stage = 0; // 0=buttons, 1=dpad
    absolute_time_t test_timer = get_absolute_time();
    
    while (true) {
        tud_task();
        
        // Change button every 1 second
        if (absolute_time_diff_us(test_timer, get_absolute_time()) >= 1000000) {
            // Reset to neutral
            current_report.buttons = 0;
            current_report.hat = 0x08;  // Neutral (matching GP2040-CE)
            
            if (test_stage == 0) {
                // Test buttons
                if (test_index < sizeof(test_buttons) / sizeof(test_buttons[0])) {
                    current_report.buttons = test_buttons[test_index].button;
                    printf("Testing: %s (bit %d)\n", test_buttons[test_index].name, test_buttons[test_index].button);
                    test_index++;
                } else {
                    // Move to D-Pad tests
                    test_stage = 1;
                    test_index = 0;
                }
            } else {
                // Test D-Pad
                if (test_index < sizeof(test_dpads) / sizeof(test_dpads[0])) {
                    current_report.hat = test_dpads[test_index].hat;  // HAT value in lower 4 bits
                    printf("Testing: %s (value 0x%02X)\n", test_dpads[test_index].name, test_dpads[test_index].hat);
                    test_index++;
                } else {
                    // Loop back to buttons
                    printf("\n=== Restarting test cycle ===\n\n");
                    test_stage = 0;
                    test_index = 0;
                }
            }
            
            test_timer = get_absolute_time();
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
        }
        
        // Send HID reports
        if (absolute_time_diff_us(last_report, get_absolute_time()) >= (REPORT_INTERVAL_MS * 1000)) {
            if (tud_hid_ready()) {
                tud_hid_report(0, &current_report, sizeof(current_report));
                last_report = get_absolute_time();
                gpio_put(PICO_DEFAULT_LED_PIN, 0);
            }
        }
    }
#else
    while (true) {
        tud_task(); // TinyUSB must run constantly

        // Check for UART data with packet synchronization
        while (uart_is_readable(UART_ID)) {
            uint8_t byte = uart_getc(UART_ID);
            
            switch (uart_state) {
                case WAIT_HEADER1:
                    if (byte == 0xAA) {
                        uart_state = WAIT_HEADER2;
                    }
                    break;
                    
                case WAIT_HEADER2:
                    if (byte == 0x55) {
                        uart_state = READ_DATA;
                        buffer_index = 0;
                    } else {
                        uart_state = WAIT_HEADER1;
                    }
                    break;
                    
                case READ_DATA:
                    uart_buffer[buffer_index++] = byte;
                    
                    // When we have a complete 8-byte packet
                    if (buffer_index >= 8) {
                        // Parse the data into the HID report
                        // Mask buttons to 16 bits
                        current_report.buttons = uart_buffer[0] | (uart_buffer[1] << 8);
                        // HAT switch is in lower 4 bits (descriptor: HAT first, then padding)
                        current_report.hat = uart_buffer[2];
                        current_report.lx = uart_buffer[3];
                        current_report.ly = uart_buffer[4];
                        current_report.rx = uart_buffer[5];
                        current_report.ry = uart_buffer[6];
                        current_report.vendor = uart_buffer[7];
                        
                        // Reset state for next packet
                        uart_state = WAIT_HEADER1;
                        buffer_index = 0;
                        
                        // Blink LED to indicate data received
                        gpio_put(PICO_DEFAULT_LED_PIN, 1);
                    }
                    break;
            }
        }

        // Send HID reports at consistent 8ms intervals
        if (absolute_time_diff_us(last_report, get_absolute_time()) >= (REPORT_INTERVAL_MS * 1000)) {
            if (tud_hid_ready()) {
                tud_hid_report(0, &current_report, sizeof(current_report));
                last_report = get_absolute_time();
                
                // Turn off LED after sending
                gpio_put(PICO_DEFAULT_LED_PIN, 0);
            }
        }
    }
#endif
}
