#include "controller_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#include <termios.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

static volatile bool g_running = true;

/* Global raw stick values for calibration (populated by platform code) */
int g_raw_lx = 128;
int g_raw_ly = 128;
int g_raw_rx = 128;
int g_raw_ry = 128;

void signal_handler(int signum) {
    (void)signum;
    g_running = false;
}

/* Cross-platform key capture */
#ifdef _WIN32
int wait_for_key(void) {
    printf("Press any key... ");
    fflush(stdout);
    int key = _getch();
    printf("\n");
    return key;
}
#else
int wait_for_key(void) {
    struct termios old_tio, new_tio;
    int key;
    
    printf("Press any key... ");
    fflush(stdout);
    
    /* Get terminal settings */
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    
    /* Disable canonical mode and echo */
    new_tio.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    
    /* Read one character */
    key = getchar();
    
    /* Restore terminal settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    
    printf("\n");
    return key;
}
#endif

/* Convert key code to string representation */
const char* key_to_string(int key) {
    static char buf[32];
    
    if (key >= 'a' && key <= 'z') {
        snprintf(buf, sizeof(buf), "%c", key);
        return buf;
    } else if (key >= 'A' && key <= 'Z') {
        snprintf(buf, sizeof(buf), "%c", tolower(key));
        return buf;
    } else if (key >= '0' && key <= '9') {
        snprintf(buf, sizeof(buf), "%c", key);
        return buf;
    }
    
    /* Special keys */
    switch (key) {
        case ' ': return "SPACE";
        case '\t': return "TAB";
        case '\r':
        case '\n': return "ENTER";
        case 27: return "ESC";
#ifdef _WIN32
        case 224: /* Extended key prefix on Windows */
            key = _getch();
            switch (key) {
                case 72: return "UP";
                case 80: return "DOWN";
                case 75: return "LEFT";
                case 77: return "RIGHT";
                default: snprintf(buf, sizeof(buf), "EXT_%d", key); return buf;
            }
#endif
        default:
            snprintf(buf, sizeof(buf), "KEY_%d", key);
            return buf;
    }
}

/* Detect which controller button was pressed */
int detect_controller_button_press(int timeout_ms) {
    if (!platform_input_init()) {
        return -1;
    }
    
    controller_state_t prev_state, curr_state;
    config_t temp_config;
    memset(&temp_config, 0, sizeof(config_t));
    temp_config.enable_controller = true;
    temp_config.controller_deadzone = 10;
    
    controller_state_init(&prev_state);
    platform_input_poll(&prev_state, &temp_config);
    
    int detected_button = -1;
    int elapsed = 0;
    
    /* Phase 1: Wait for button press */
    while (elapsed < timeout_ms && detected_button == -1) {
        SLEEP_MS(50);
        elapsed += 50;
        
        controller_state_init(&curr_state);
        platform_input_poll(&curr_state, &temp_config);
        
        /* Check for button press (new buttons that weren't pressed before) */
        uint16_t new_buttons = curr_state.buttons & ~prev_state.buttons;
        
        if (new_buttons != 0) {
            /* Found a button press - get the index */
            for (int i = 0; i < 16; i++) {
                if (new_buttons & (1 << i)) {
                    detected_button = i;
                    break;
                }
            }
        }
        
        prev_state = curr_state;
    }
    
    /* If no button was pressed, timeout */
    if (detected_button == -1) {
        platform_input_cleanup();
        return -1;
    }
    
    /* Phase 2: Wait for button release */
    uint16_t button_mask = (1 << detected_button);
    while (elapsed < timeout_ms) {
        SLEEP_MS(50);
        elapsed += 50;
        
        controller_state_init(&curr_state);
        platform_input_poll(&curr_state, &temp_config);
        
        /* Check if the button has been released */
        if ((curr_state.buttons & button_mask) == 0) {
            /* Button released - we're done! */
            platform_input_cleanup();
            return detected_button;
        }
    }
    
    /* Timeout while waiting for release (user is still holding the button) */
    platform_input_cleanup();
    return detected_button;  /* Return anyway, but this is less ideal */
}

/* Get button name from Switch button mask */
const char* get_button_name(uint16_t mask) {
    if (mask == BTN_A) return "A";
    if (mask == BTN_B) return "B";
    if (mask == BTN_X) return "X";
    if (mask == BTN_Y) return "Y";
    if (mask == BTN_L) return "L";
    if (mask == BTN_R) return "R";
    if (mask == BTN_ZL) return "ZL";
    if (mask == BTN_ZR) return "ZR";
    if (mask == BTN_MINUS) return "MINUS";
    if (mask == BTN_PLUS) return "PLUS";
    if (mask == BTN_LSTICK) return "LSTICK";
    if (mask == BTN_RSTICK) return "RSTICK";
    if (mask == BTN_HOME) return "HOME";
    if (mask == BTN_CAPTURE) return "CAPTURE";
    return "UNKNOWN";
}

/* Calibrate analog stick */
bool calibrate_analog_stick(const char *stick_name, stick_calibration_t *cal, 
                            int *raw_x_ptr, int *raw_y_ptr) {
    if (!platform_input_init()) {
        fprintf(stderr, "Error: Could not initialize input for calibration\n");
        return false;
    }
    
    config_t temp_config;
    memset(&temp_config, 0, sizeof(config_t));
    temp_config.enable_controller = true;
    temp_config.controller_deadzone = 0;  /* No deadzone during calibration */
    
    controller_state_t state;
    
    printf("\n");
    printf("===============================================================\n");
    printf("         CALIBRATING %s STICK\n", stick_name);
    printf("===============================================================\n");
    printf("\n");
    
    /* Step 1: Capture center position */
    printf("Step 1: Center Position\n");
    printf("  Release the %s stick and let it return to center.\n", stick_name);
    printf("  Press ENTER when ready...");
    getchar();
    
    /* Sample center position */
    int center_samples = 20;
    long sum_x = 0, sum_y = 0;
    for (int i = 0; i < center_samples; i++) {
        controller_state_init(&state);
        platform_input_poll(&state, &temp_config);
        sum_x += *raw_x_ptr;
        sum_y += *raw_y_ptr;
        SLEEP_MS(10);
    }
    cal->center_x = (int)(sum_x / center_samples);
    cal->center_y = (int)(sum_y / center_samples);
    printf("  Center captured: X=%d, Y=%d\n\n", cal->center_x, cal->center_y);
    
    /* Step 2: Capture range by rotating stick */
    printf("Step 2: Range Calibration\n");
    printf("  Slowly rotate the %s stick in a full circle\n", stick_name);
    printf("  2-3 times to capture the full range.\n");
    printf("  Press ENTER to start (you have 10 seconds)...");
    getchar();
    
    cal->min_x = cal->center_x;
    cal->max_x = cal->center_x;
    cal->min_y = cal->center_y;
    cal->max_y = cal->center_y;
    
    printf("  Calibrating");
    fflush(stdout);
    
    /* Collect samples for 10 seconds */
    int duration_ms = 10000;
    int samples = 0;
    for (int elapsed = 0; elapsed < duration_ms; elapsed += 50) {
        controller_state_init(&state);
        platform_input_poll(&state, &temp_config);
        
        int x = *raw_x_ptr;
        int y = *raw_y_ptr;
        
        if (x < cal->min_x) cal->min_x = x;
        if (x > cal->max_x) cal->max_x = x;
        if (y < cal->min_y) cal->min_y = y;
        if (y > cal->max_y) cal->max_y = y;
        
        samples++;
        if (samples % 20 == 0) {
            printf(".");
            fflush(stdout);
        }
        
        SLEEP_MS(50);
    }
    
    printf(" Done!\n");
    printf("  Range captured:\n");
    printf("    X: %d to %d (center: %d)\n", cal->min_x, cal->max_x, cal->center_x);
    printf("    Y: %d to %d (center: %d)\n", cal->min_y, cal->max_y, cal->center_y);
    printf("\n");
    
    cal->is_calibrated = true;
    platform_input_cleanup();
    return true;
}

/* Interactive configuration wizard */
bool run_configuration_wizard(const char *output_filename) {
    printf("\n");
    printf("===============================================================\n");
    printf("              CONFIGURATION WIZARD\n");
    printf("===============================================================\n");
    printf("\n");
    printf("Choose configuration type:\n");
    printf("  1) Keyboard bindings (press keys to map actions)\n");
    printf("  2) Controller bindings (press controller buttons to remap)\n");
    printf("\n");
    printf("Enter choice (1 or 2): ");
    fflush(stdout);
    
    char choice[10];
    if (!fgets(choice, sizeof(choice), stdin)) {
        return false;
    }
    
    bool use_controller = (choice[0] == '2');
    
    printf("\n");
    if (use_controller) {
        printf("CONTROLLER BINDING WIZARD\n");
        printf("=========================\n");
        printf("This wizard will detect your controller button presses and let you\n");
        printf("remap them to Switch buttons.\n");
        printf("\nMake sure your controller is connected!\n");
    } else {
        printf("KEYBOARD BINDING WIZARD\n");
        printf("=======================\n");
        printf("This wizard will help you set up custom key bindings.\n");
        printf("For each button, press the key you want to use.\n");
    }
    printf("\n");
    
    /* Ask for serial port configuration */
    char serial_port[64] = "COM1";  /* Default */
    int baud_rate = 115200;
    
    printf("Serial Port Configuration\n");
    printf("=========================\n");
#ifdef _WIN32
    printf("Enter COM port (e.g., COM10) [default: COM1]: ");
#else
    printf("Enter serial port (e.g., /dev/ttyACM0) [default: /dev/ttyACM0]: ");
    strncpy(serial_port, "/dev/ttyACM0", sizeof(serial_port) - 1);
#endif
    fflush(stdout);
    
    char port_input[64];
    if (fgets(port_input, sizeof(port_input), stdin)) {
        /* Remove newline */
        port_input[strcspn(port_input, "\r\n")] = '\0';
        
        /* If user entered a port, use it */
        if (strlen(port_input) > 0) {
            strncpy(serial_port, port_input, sizeof(serial_port) - 1);
            serial_port[sizeof(serial_port) - 1] = '\0';
        }
    }
    
    printf("Enter baud rate [default: 115200]: ");
    fflush(stdout);
    
    char baud_input[32];
    if (fgets(baud_input, sizeof(baud_input), stdin)) {
        /* Remove newline */
        baud_input[strcspn(baud_input, "\r\n")] = '\0';
        
        /* If user entered a baud rate, use it */
        if (strlen(baud_input) > 0) {
            int temp_baud = atoi(baud_input);
            if (temp_baud > 0) {
                baud_rate = temp_baud;
            }
        }
    }
    
    printf("\nUsing serial port: %s @ %d baud\n", serial_port, baud_rate);
    printf("\nPress ENTER to continue...");
    getchar();
    printf("\n");
    
    typedef struct {
        const char *button_name;
        const char *config_key;
        const char *default_key;
    } button_config_t;
    
    button_config_t buttons[] = {
        {"D-Pad UP", "dpad_up", "UP"},
        {"D-Pad DOWN", "dpad_down", "DOWN"},
        {"D-Pad LEFT", "dpad_left", "LEFT"},
        {"D-Pad RIGHT", "dpad_right", "RIGHT"},
        {"Button A", "button_a", "k"},
        {"Button B", "button_b", "i"},
        {"Button X", "button_x", "u"},
        {"Button Y", "button_y", "j"},
        {"Button L", "button_l", "l"},
        {"Button R", "button_r", "f"},
        {"Button ZL", "button_zl", "t"},
        {"Button ZR", "button_zr", "s"},
        {"Button MINUS", "button_minus", "1"},
        {"Button PLUS", "button_plus", "2"},
        {"Button HOME", "button_home", "h"},
        {"Button CAPTURE", "button_capture", "c"},
        {"Left Stick UP", "lstick_up", "w"},
        {"Left Stick DOWN", "lstick_down", "s"},
        {"Left Stick LEFT", "lstick_left", "a"},
        {"Left Stick RIGHT", "lstick_right", "d"},
        {"Right Stick UP", "rstick_up", "g"},
        {"Right Stick DOWN", "rstick_down", "v"},
        {"Right Stick LEFT", "rstick_left", "a"},
        {"Right Stick RIGHT", "rstick_right", "y"}
    };
    
    char bindings[24][64];
    int num_buttons = sizeof(buttons) / sizeof(buttons[0]);
    
    /* Declare calibration variables in outer scope for file saving */
    stick_calibration_t left_cal, right_cal;
    memset(&left_cal, 0, sizeof(left_cal));
    memset(&right_cal, 0, sizeof(right_cal));
    
    if (use_controller) {
        /* Controller binding mode */
        typedef struct {
            const char *switch_button_name;
            uint16_t switch_button_mask;
        } switch_button_t;
        
        switch_button_t switch_buttons[] = {
            {"Button A", BTN_A},
            {"Button B", BTN_B},
            {"Button X", BTN_X},
            {"Button Y", BTN_Y},
            {"Button L", BTN_L},
            {"Button R", BTN_R},
            {"Button ZL", BTN_ZL},
            {"Button ZR", BTN_ZR},
            {"Button MINUS", BTN_MINUS},
            {"Button PLUS", BTN_PLUS},
            {"Button LSTICK", BTN_LSTICK},
            {"Button RSTICK", BTN_RSTICK},
            {"Button HOME", BTN_HOME},
            {"Button CAPTURE", BTN_CAPTURE}
        };
        
        int num_switch_buttons = sizeof(switch_buttons) / sizeof(switch_buttons[0]);
        int binding_count = 0;
        
        printf("Press each controller button to map it to a Switch button.\n");
        printf("Press ENTER to skip a button.\n\n");
        
        for (int i = 0; i < num_switch_buttons; i++) {
            printf("[%2d/%2d] Press controller button for Switch %s\n", 
                   i + 1, num_switch_buttons, switch_buttons[i].switch_button_name);
            printf("        Waiting for button press (10 seconds)...");
            fflush(stdout);
            
            int button_idx = detect_controller_button_press(10000);
            
            if (button_idx >= 0) {
                printf(" Detected button %d\n", button_idx);
                snprintf(bindings[binding_count], sizeof(bindings[binding_count]), 
                         "%d = %s", button_idx, get_button_name(switch_buttons[i].switch_button_mask));
                binding_count++;
            } else {
                printf(" Skipped\n");
            }
            printf("\n");
        }
        
        num_buttons = binding_count;
        
        /* After button bindings, offer stick calibration */
        printf("\n");
        printf("Would you like to calibrate the analog sticks?\n");
        printf("This will improve stick accuracy and precision.\n");
        printf("  1) Yes, calibrate sticks\n");
        printf("  2) No, skip calibration\n");
        printf("\nEnter choice (1 or 2): ");
        fflush(stdout);
        
        char cal_choice[10];
        if (fgets(cal_choice, sizeof(cal_choice), stdin) && cal_choice[0] == '1') {
            /* Calibrate left stick */
            if (!calibrate_analog_stick("LEFT", &left_cal, &g_raw_lx, &g_raw_ly)) {
                fprintf(stderr, "Warning: Left stick calibration failed\n");
            }
            
            /* Calibrate right stick */
            if (!calibrate_analog_stick("RIGHT", &right_cal, &g_raw_rx, &g_raw_ry)) {
                fprintf(stderr, "Warning: Right stick calibration failed\n");
            }
        }
        
    } else {
        /* Keyboard binding mode */
        /* Map config keys to proper binding format */
        typedef struct {
            const char *config_key;
            const char *binding_format;  /* e.g., "button:A", "dpad:up", "lstick:left" */
        } binding_format_t;
        
        binding_format_t formats[] = {
            {"dpad_up", "dpad:up"},
            {"dpad_down", "dpad:down"},
            {"dpad_left", "dpad:left"},
            {"dpad_right", "dpad:right"},
            {"button_a", "button:A"},
            {"button_b", "button:B"},
            {"button_x", "button:X"},
            {"button_y", "button:Y"},
            {"button_l", "button:L"},
            {"button_r", "button:R"},
            {"button_zl", "button:ZL"},
            {"button_zr", "button:ZR"},
            {"button_minus", "button:MINUS"},
            {"button_plus", "button:PLUS"},
            {"button_home", "button:HOME"},
            {"button_capture", "button:CAPTURE"},
            {"lstick_up", "lstick:up"},
            {"lstick_down", "lstick:down"},
            {"lstick_left", "lstick:left"},
            {"lstick_right", "lstick:right"},
            {"rstick_up", "rstick:up"},
            {"rstick_down", "rstick:down"},
            {"rstick_left", "rstick:left"},
            {"rstick_right", "rstick:right"}
        };
        
        for (int i = 0; i < num_buttons; i++) {
            printf("[%2d/%2d] %s (default: %s)\n", i + 1, num_buttons, 
                   buttons[i].button_name, buttons[i].default_key);
            
            int key = wait_for_key();
            const char *key_str = key_to_string(key);
            
            /* Find the binding format for this config key */
            const char *format = NULL;
            for (int j = 0; j < (int)(sizeof(formats) / sizeof(formats[0])); j++) {
                if (strcmp(buttons[i].config_key, formats[j].config_key) == 0) {
                    format = formats[j].binding_format;
                    break;
                }
            }
            
            /* Generate binding in format: key = type:value */
            if (format) {
                snprintf(bindings[i], sizeof(bindings[i]), "%s = %s", key_str, format);
            } else {
                snprintf(bindings[i], sizeof(bindings[i]), "%s = unknown", key_str);
            }
            
            printf("        Mapped to: %s\n\n", key_str);
        }
    }
    
    /* Create config file */
    FILE *f = fopen(output_filename, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not create config file: %s\n", output_filename);
        return false;
    }
    
    fprintf(f, "# Controller Bridge Configuration\n");
    fprintf(f, "# Generated by configuration wizard\n\n");
    fprintf(f, "[Serial]\n");
    fprintf(f, "port = %s\n", serial_port);
    fprintf(f, "baud_rate = %d\n\n", baud_rate);
    fprintf(f, "[General]\n");
    fprintf(f, "enable_keyboard = %s\n", use_controller ? "false" : "true");
    fprintf(f, "enable_controller = %s\n", use_controller ? "true" : "false");
    fprintf(f, "update_rate_hz = 1000\n");
    fprintf(f, "controller_deadzone = 10\n\n");
    fprintf(f, "controller_index = 0\n\n");
    
    if (use_controller) {
        fprintf(f, "[ControllerBindings]\n");
        fprintf(f, "# Controller button index = Switch button name\n");
        for (int i = 0; i < num_buttons; i++) {
            fprintf(f, "%s\n", bindings[i]);
        }
        
        /* Save stick calibration if performed */
        if (left_cal.is_calibrated || right_cal.is_calibrated) {
            fprintf(f, "\n[StickCalibration]\n");
            fprintf(f, "# Analog stick calibration data\n");
            
            if (left_cal.is_calibrated) {
                fprintf(f, "left_center_x = %d\n", left_cal.center_x);
                fprintf(f, "left_center_y = %d\n", left_cal.center_y);
                fprintf(f, "left_min_x = %d\n", left_cal.min_x);
                fprintf(f, "left_max_x = %d\n", left_cal.max_x);
                fprintf(f, "left_min_y = %d\n", left_cal.min_y);
                fprintf(f, "left_max_y = %d\n", left_cal.max_y);
            }
            
            if (right_cal.is_calibrated) {
                fprintf(f, "right_center_x = %d\n", right_cal.center_x);
                fprintf(f, "right_center_y = %d\n", right_cal.center_y);
                fprintf(f, "right_min_x = %d\n", right_cal.min_x);
                fprintf(f, "right_max_x = %d\n", right_cal.max_x);
                fprintf(f, "right_min_y = %d\n", right_cal.min_y);
                fprintf(f, "right_max_y = %d\n", right_cal.max_y);
            }
        }
    } else {
        fprintf(f, "[KeyBindings]\n");
        for (int i = 0; i < num_buttons; i++) {
            fprintf(f, "%s\n", bindings[i]);
        }
    }
    
    fclose(f);
    
    printf("\n");
    printf("===============================================================\n");
    printf("Configuration saved to: %s\n", output_filename);
    printf("===============================================================\n");
    printf("\n");
    
    return true;
}

void print_banner(void) {
    printf("\n");
    printf("===============================================================\n");
    printf("         CONTROLLER BRIDGE - Nintendo Switch Remote\n");
    printf("===============================================================\n");
    printf("\n");
}

void print_config_info(const config_t *config) {
    printf("Configuration:\n");
    printf("  Serial Port:      %s @ %d baud\n", config->serial_port, config->baud_rate);
    printf("  Keyboard Input:   %s\n", config->enable_keyboard ? "Enabled" : "Disabled");
    printf("  Controller Input: %s\n", config->enable_controller ? "Enabled" : "Disabled");
    printf("  Controller Index: %d\n", config->controller_index);
    printf("  Update Rate:      %d Hz\n", config->update_rate_hz);
    printf("  Loaded Bindings:  %d key mappings\n", config->binding_count);
    printf("\n");
}

void print_controls(void) {
    printf("Default Controls:\n");
    printf("  D-Pad:        Arrow Keys\n");
    printf("  Left Stick:   WASD\n");
    printf("  Face Buttons: U=X, I=B, J=Y, K=A\n");
    printf("  Shoulders:    L=L, F=R, T=ZL, S=ZR\n");
    printf("  System:       H=Home, C=Capture, 1/2=Minus/Plus\n");
    printf("\n");
    printf("Press Ctrl+C to quit\n");
    printf("---------------------------------------------------------------\n\n");
}

int main(int argc, char *argv[]) {
    config_t config;
    char config_filename[256] = "controller_bridge.ini";
    bool run_wizard = false;
    
    print_banner();
    
    /* Parse command line arguments */
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("Usage: %s [options] [config_file]\n\n", argv[0]);
            printf("Options:\n");
            printf("  --help, -h          Show this help message\n");
            printf("  --setup             Run interactive setup wizard\n");
            printf("  [config_file]       Use specified config file (default: controller_bridge.ini)\n");
            printf("\n");
            printf("Examples:\n");
            printf("  %s                              # Interactive prompt\n", argv[0]);
            printf("  %s --setup                      # Run setup wizard\n", argv[0]);
            printf("  %s custom_config.ini            # Use custom config\n", argv[0]);
            printf("\n");
            return 0;
        } else if (strcmp(argv[1], "--setup") == 0) {
            run_wizard = true;
        } else {
            strncpy(config_filename, argv[1], sizeof(config_filename) - 1);
            config_filename[sizeof(config_filename) - 1] = '\0';
        }
    } else {
        /* Interactive prompt: use existing config or run setup wizard? */
        printf("Would you like to:\n");
        printf("  1) Use existing configuration file\n");
        printf("  2) Run setup wizard (configure key bindings)\n");
        printf("\n");
        printf("Enter choice (1 or 2): ");
        fflush(stdout);
        
        char choice[10];
        if (fgets(choice, sizeof(choice), stdin)) {
            if (choice[0] == '2') {
                run_wizard = true;
            } else {
                /* Option 1: Ask for config filename */
                printf("\nEnter configuration filename (press ENTER for default 'controller_bridge.ini'): ");
                fflush(stdout);
                
                char input[256];
                if (fgets(input, sizeof(input), stdin)) {
                    /* Remove newline */
                    input[strcspn(input, "\r\n")] = '\0';
                    
                    /* If user entered a filename, use it */
                    if (strlen(input) > 0) {
                        strncpy(config_filename, input, sizeof(config_filename) - 1);
                        config_filename[sizeof(config_filename) - 1] = '\0';
                        
                        /* Add .ini extension if not present */
                        if (strlen(config_filename) < 4 || 
                            strcmp(config_filename + strlen(config_filename) - 4, ".ini") != 0) {
                            strncat(config_filename, ".ini", sizeof(config_filename) - strlen(config_filename) - 1);
                        }
                    }
                }
            }
        }
        printf("\n");
    }
    
    /* Run setup wizard if requested */
    if (run_wizard) {
        printf("Enter filename for new configuration (e.g., my_config.ini): ");
        fflush(stdout);
        
        if (fgets(config_filename, sizeof(config_filename), stdin)) {
            /* Remove newline */
            config_filename[strcspn(config_filename, "\r\n")] = '\0';
            
            /* Add .ini extension if not present */
            if (strlen(config_filename) < 4 || 
                strcmp(config_filename + strlen(config_filename) - 4, ".ini") != 0) {
                strncat(config_filename, ".ini", sizeof(config_filename) - strlen(config_filename) - 1);
            }
        }
        
        if (!run_configuration_wizard(config_filename)) {
            fprintf(stderr, "Setup wizard failed!\n");
            return 1;
        }
        
        printf("Configuration wizard completed!\n");
        printf("Starting with new configuration...\n\n");
    }
    
    /* Load configuration */
    printf("Loading configuration from: %s\n", config_filename);
    if (!config_load(&config, config_filename)) {
        fprintf(stderr, "Warning: Could not load config file '%s'\n", config_filename);
        fprintf(stderr, "Creating default configuration...\n");
        if (!config_create_default("controller_bridge.ini")) {
            fprintf(stderr, "Error: Could not create default config\n");
            return 1;
        }
        if (!config_load(&config, "controller_bridge.ini")) {
            fprintf(stderr, "Error: Could not load newly created config\n");
            return 1;
        }
    }
    
    print_config_info(&config);
    
    /* Open serial port */
    printf("Opening serial port %s...\n", config.serial_port);
    serial_port_t serial = serial_open(config.serial_port, config.baud_rate);
    if (!serial || !serial_is_open(serial)) {
        fprintf(stderr, "\nError: Could not open serial port %s\n", config.serial_port);
        fprintf(stderr, "\nTroubleshooting:\n");
#ifdef _WIN32
        fprintf(stderr, "  - Check Device Manager for correct COM port\n");
        fprintf(stderr, "  - Ensure no other program is using the port\n");
        fprintf(stderr, "  - Try a different COM port number in config file\n");
#else
        fprintf(stderr, "  - Check available ports with: ls /dev/tty* | grep -E '(USB|ACM)'\n");
        fprintf(stderr, "  - You may need permissions: sudo usermod -a -G dialout $USER\n");
        fprintf(stderr, "  - Or run with: sudo %s\n", argv[0]);
#endif
        config_free(&config);
        return 1;
    }
    printf("Serial port opened successfully!\n\n");
    
    /* Initialize controller state */
    controller_state_t state;
    controller_state_init(&state);
    
    /* Initialize input handler */
    input_handler_t *input = input_handler_create(&state, &config);
    if (!input || !input_handler_start(input)) {
        fprintf(stderr, "Error: Could not initialize input handler\n");
        serial_close(serial);
        config_free(&config);
        return 1;
    }
    
    print_controls();
    
    /* Set up signal handler */
    signal(SIGINT, signal_handler);
#ifndef _WIN32
    signal(SIGTERM, signal_handler);
#endif
    
    /* Calculate sleep time for update rate */
    int sleep_ms = 1000 / config.update_rate_hz;
    if (sleep_ms < 1) sleep_ms = 1;
    
    /* Main loop */
    uint8_t packet[10];  /* 10-byte packet: 0xAA 0x55 header + 8 data bytes */
    unsigned long packet_count = 0;
    
    printf("Controller bridge active! Waiting for input...\n\n");
    
    while (g_running && input_handler_is_running(input)) {
        /* Reset state for this frame */
        controller_state_init(&state);
        
        /* Poll all input sources */
        platform_input_poll(&state, &config);
        
        /* Update analog stick positions from directional flags (keyboard input) */
        /* Only update if controller didn't already set analog values */
        if (state.lx == STICK_CENTER && state.ly == STICK_CENTER) {
            /* Left stick at center - apply keyboard directions if any */
            if (state.lstick_up || state.lstick_down || state.lstick_left || state.lstick_right) {
                if (state.lstick_up) state.ly = STICK_MIN;
                else if (state.lstick_down) state.ly = STICK_MAX;
                
                if (state.lstick_left) state.lx = STICK_MIN;
                else if (state.lstick_right) state.lx = STICK_MAX;
            }
        }
        
        if (state.rx == STICK_CENTER && state.ry == STICK_CENTER) {
            /* Right stick at center - apply keyboard directions if any */
            if (state.rstick_up || state.rstick_down || state.rstick_left || state.rstick_right) {
                if (state.rstick_up) state.ry = STICK_MIN;
                else if (state.rstick_down) state.ry = STICK_MAX;
                
                if (state.rstick_left) state.rx = STICK_MIN;
                else if (state.rstick_right) state.rx = STICK_MAX;
            }
        }
        
        /* Convert state to packet */
        controller_state_to_packet(&state, packet);
        
        /* Send packet every cycle (matching Python behavior - 1000Hz continuous sending) */
        if (serial_write(serial, packet, sizeof(packet))) {
            packet_count++;
            
            /* Print status on button press (not on every packet) */
            if (state.buttons != 0 && (packet_count % 100 == 0)) {
                printf("\r[Packets: %lu] Buttons: 0x%04X  ", packet_count, state.buttons);
                fflush(stdout);
            }
        } else {
            fprintf(stderr, "\nWarning: Failed to write to serial port\n");
            SLEEP_MS(100);
        }
        
        /* Sleep to maintain update rate */
        SLEEP_MS(sleep_ms);
    }
    
    printf("\n\nShutting down...\n");
    
    /* Send neutral state before exit */
    controller_state_init(&state);
    controller_state_to_packet(&state, packet);
    serial_write(serial, packet, sizeof(packet));
    
    /* Cleanup */
    input_handler_destroy(input);
    serial_close(serial);
    config_free(&config);
    
    printf("Goodbye!\n");
    return 0;
}
