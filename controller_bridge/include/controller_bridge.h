#ifndef CONTROLLER_BRIDGE_H
#define CONTROLLER_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Nintendo Switch button definitions */
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
#define BTN_GL      (1 << 14)
#define BTN_GR      (1 << 15)

/* D-Pad HAT values */
#define DPAD_UP        0x00
#define DPAD_UP_RIGHT  0x01
#define DPAD_RIGHT     0x02
#define DPAD_DN_RIGHT  0x03
#define DPAD_DOWN      0x04
#define DPAD_DN_LEFT   0x05
#define DPAD_LEFT      0x06
#define DPAD_UP_LEFT   0x07
#define DPAD_NEUTRAL   0x08

/* Constants */
#define MAX_PATH_LEN 512
#define MAX_KEY_NAME 32
#define STICK_CENTER 128
#define STICK_MIN 0
#define STICK_MAX 255

/* Controller state structure */
typedef struct {
    uint16_t buttons;
    bool dpad_up;
    bool dpad_down;
    bool dpad_left;
    bool dpad_right;
    bool lstick_up;
    bool lstick_down;
    bool lstick_left;
    bool lstick_right;
    bool rstick_up;
    bool rstick_down;
    bool rstick_left;
    bool rstick_right;
    uint8_t lx;
    uint8_t ly;
    uint8_t rx;
    uint8_t ry;
} controller_state_t;

/* Input type enumeration */
typedef enum {
    INPUT_TYPE_BUTTON,
    INPUT_TYPE_DPAD,
    INPUT_TYPE_LSTICK,
    INPUT_TYPE_RSTICK
} input_type_t;

/* Input direction enumeration */
typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_NONE
} input_direction_t;

/* Key binding structure */
typedef struct {
    char key_name[MAX_KEY_NAME];
    input_type_t type;
    union {
        uint16_t button_mask;
        input_direction_t direction;
    } value;
} key_binding_t;

/* Controller button binding structure */
typedef struct {
    int controller_button_index;  /* Physical controller button index */
    uint16_t switch_button_mask;  /* Which Switch button it maps to */
} controller_button_binding_t;

/* Analog stick calibration data */
typedef struct {
    int center_x;
    int center_y;
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    bool is_calibrated;
} stick_calibration_t;

/* Configuration structure */
typedef struct {
    char serial_port[MAX_PATH_LEN];
    int baud_rate;
    bool enable_keyboard;
    bool enable_controller;
    int update_rate_hz;
    int controller_deadzone;
    key_binding_t *bindings;
    int binding_count;
    controller_button_binding_t *controller_bindings;
    int controller_binding_count;
    bool use_custom_controller_bindings;
    stick_calibration_t left_stick_cal;
    stick_calibration_t right_stick_cal;
} config_t;

/* Function declarations */

/* Controller state management */
void controller_state_init(controller_state_t *state);
void controller_state_update_sticks(controller_state_t *state);
uint8_t controller_state_get_hat(const controller_state_t *state);
void controller_state_to_packet(const controller_state_t *state, uint8_t *packet);

/* Configuration */
bool config_load(config_t *config, const char *filename);
bool config_create_default(const char *filename);
void config_free(config_t *config);
key_binding_t *config_find_binding(config_t *config, const char *key_name);

/* Serial port */
typedef void* serial_port_t;
serial_port_t serial_open(const char *port_name, int baud_rate);
void serial_close(serial_port_t port);
bool serial_write(serial_port_t port, const uint8_t *data, size_t len);
bool serial_is_open(serial_port_t port);

/* Input handler */
typedef struct input_handler input_handler_t;
input_handler_t *input_handler_create(controller_state_t *state, config_t *config);
void input_handler_destroy(input_handler_t *handler);
bool input_handler_start(input_handler_t *handler);
void input_handler_stop(input_handler_t *handler);
bool input_handler_is_running(input_handler_t *handler);

/* Platform-specific input initialization */
bool platform_input_init(void);
void platform_input_cleanup(void);
void platform_input_poll(controller_state_t *state, config_t *config);

/* Global raw stick values for calibration (set by platform code) */
extern int g_raw_lx, g_raw_ly, g_raw_rx, g_raw_ry;

/* Calibration helper function */
uint8_t apply_stick_calibration(int raw_value, const stick_calibration_t *cal, bool is_y_axis);

#endif /* CONTROLLER_BRIDGE_H */
