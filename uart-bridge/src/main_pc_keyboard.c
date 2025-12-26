// UART Bridge for Nintendo Switch Controller - PC Keyboard Version
// This Pico receives binary controller packets from PC via USB serial
// and forwards them to the Switch Pico via UART
//
// Connect: This Pico GP0 (TX) -> Switch Pico GP1 (RX)
//          This Pico GP1 (RX) -> Switch Pico GP0 (TX)
//          Common GND
//          This Pico USB -> PC (for Python script)

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <stdio.h>
#include <string.h>

// UART Configuration
#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_BAUD_RATE 115200

// Controller state packet (matches Python script)
typedef struct __attribute__((packed)) {
    uint16_t buttons;    // Button bitmask
    uint8_t  hat;        // D-Pad/HAT value
    uint8_t  lx;         // Left stick X (0-255)
    uint8_t  ly;         // Left stick Y (0-255)
    uint8_t  rx;         // Right stick X (0-255)
    uint8_t  ry;         // Right stick Y (0-255)
    uint8_t  vendor;     // Vendor byte (unused)
} controller_state_t;

#define PACKET_SIZE sizeof(controller_state_t)

int main(void)
{
    // Initialize stdio for USB serial
    stdio_init_all();
    
    // Initialize LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    
    // Initialize UART to Switch Pico
    uart_init(UART_ID, UART_BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);
    
    sleep_ms(2000);  // Wait for USB serial to be ready
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  Nintendo Switch UART Bridge - PC Keyboard Mode\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Hardware Connections:\n");
    printf("  This Pico GP0 (TX) -> simple-s2rc Pico GP1 (RX)\n");
    printf("  This Pico GP1 (RX) -> simple-s2rc Pico GP0 (TX)\n");
    printf("  GND -> GND\n");
    printf("  This Pico USB -> PC\n");
    printf("\n");
    printf("UART initialized @ %d baud\n", UART_BAUD_RATE);
    printf("\n");
    printf("Ready to receive binary packets from PC!\n");
    printf("Run: python keyboard_to_serial.py COM<X>\n");
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("\n");
    
    uint8_t packet_buffer[PACKET_SIZE];
    uint8_t packet_index = 0;
    uint32_t packets_received = 0;
    uint32_t packets_forwarded = 0;
    uint32_t last_stats_time = 0;
    bool led_state = false;
    uint32_t led_toggle_time = 0;
    
    while (true) {
        // Read bytes from USB serial
        int c = getchar_timeout_us(0);
        
        if (c != PICO_ERROR_TIMEOUT) {
            packet_buffer[packet_index++] = (uint8_t)c;
            
            // When we have a complete packet
            if (packet_index >= PACKET_SIZE) {
                // Forward the packet to Switch Pico via UART
                uart_write_blocking(UART_ID, packet_buffer, PACKET_SIZE);
                
                packets_received++;
                packets_forwarded++;
                
                // Blink LED on activity
                gpio_put(PICO_DEFAULT_LED_PIN, 1);
                led_toggle_time = to_ms_since_boot(get_absolute_time()) + 50;
                
                // Parse and display state for debugging
                controller_state_t *state = (controller_state_t*)packet_buffer;
                printf("[RX] Buttons=0x%04X HAT=%d LX=%d LY=%d RX=%d RY=%d\n",
                       state->buttons, state->hat, state->lx, state->ly, 
                       state->rx, state->ry);
                
                // Reset for next packet
                packet_index = 0;
            }
        }
        
        // Turn off LED after activity
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (led_toggle_time > 0 && now >= led_toggle_time) {
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            led_toggle_time = 0;
        }
        
        // Print stats every 10 seconds
        if (now - last_stats_time >= 10000) {
            if (packets_received > 0) {
                printf("\n[STATS] Packets: RX=%lu, FWD=%lu\n\n", 
                       packets_received, packets_forwarded);
            }
            last_stats_time = now;
        }
        
        tight_loop_contents();
    }
}
