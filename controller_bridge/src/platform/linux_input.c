#if defined(__linux__)

#include "controller_bridge.h"
#include <linux/input.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_DEVICES 8

static int keyboard_fd = -1;
static int joystick_fd = -1;

/* Key code mappings for Linux */
typedef struct {
    const char *name;
    int key_code;
} key_map_t;

static const key_map_t key_mappings[] = {
    {"a", KEY_A}, {"b", KEY_B}, {"c", KEY_C}, {"d", KEY_D}, {"e", KEY_E},
    {"f", KEY_F}, {"g", KEY_G}, {"h", KEY_H}, {"i", KEY_I}, {"j", KEY_J},
    {"k", KEY_K}, {"l", KEY_L}, {"m", KEY_M}, {"n", KEY_N}, {"o", KEY_O},
    {"p", KEY_P}, {"q", KEY_Q}, {"r", KEY_R}, {"s", KEY_S}, {"t", KEY_T},
    {"u", KEY_U}, {"v", KEY_V}, {"w", KEY_W}, {"x", KEY_X}, {"y", KEY_Y},
    {"z", KEY_Z},
    {"0", KEY_0}, {"1", KEY_1}, {"2", KEY_2}, {"3", KEY_3}, {"4", KEY_4},
    {"5", KEY_5}, {"6", KEY_6}, {"7", KEY_7}, {"8", KEY_8}, {"9", KEY_9},
    {"space", KEY_SPACE}, {"enter", KEY_ENTER}, {"escape", KEY_ESC},
    {"tab", KEY_TAB}, {"backspace", KEY_BACKSPACE}, {"delete", KEY_DELETE},
    {"up", KEY_UP}, {"down", KEY_DOWN}, {"left", KEY_LEFT}, {"right", KEY_RIGHT},
    {"f1", KEY_F1}, {"f2", KEY_F2}, {"f3", KEY_F3}, {"f4", KEY_F4},
    {"f5", KEY_F5}, {"f6", KEY_F6}, {"f7", KEY_F7}, {"f8", KEY_F8},
    {"f9", KEY_F9}, {"f10", KEY_F10}, {"f11", KEY_F11}, {"f12", KEY_F12},
    {"shift", KEY_LEFTSHIFT}, {"ctrl", KEY_LEFTCTRL}, {"alt", KEY_LEFTALT},
    {"num0", KEY_KP0}, {"num1", KEY_KP1}, {"num2", KEY_KP2},
    {"num3", KEY_KP3}, {"num4", KEY_KP4}, {"num5", KEY_KP5},
    {"num6", KEY_KP6}, {"num7", KEY_KP7}, {"num8", KEY_KP8}, {"num9", KEY_KP9},
    {NULL, 0}
};

static int get_key_code(const char *key_name) {
    for (int i = 0; key_mappings[i].name != NULL; i++) {
        if (strcmp(key_mappings[i].name, key_name) == 0) {
            return key_mappings[i].key_code;
        }
    }
    return 0;
}

static int open_keyboard_device(void) {
    /* Try to open keyboard event device */
    for (int i = 0; i < 32; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        
        /* Check if it's a keyboard */
        unsigned long evbit = 0;
        ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);
        
        if (evbit & (1 << EV_KEY)) {
            /* This device supports key events */
            printf("Found keyboard at %s\n", path);
            return fd;
        }
        
        close(fd);
    }
    
    return -1;
}

static int open_joystick_device(void) {
    /* Try to open joystick device */
    for (int i = 0; i < 8; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/js%d", i);
        
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            printf("Found joystick at %s\n", path);
            return fd;
        }
    }
    
    return -1;
}

bool platform_input_init(void) {
    keyboard_fd = open_keyboard_device();
    joystick_fd = open_joystick_device();
    
    if (keyboard_fd < 0 && joystick_fd < 0) {
        fprintf(stderr, "Warning: No input devices found\n");
        fprintf(stderr, "Note: You may need to run with sudo or add yourself to the 'input' group\n");
    }
    
    return true;
}

void platform_input_cleanup(void) {
    if (keyboard_fd >= 0) {
        close(keyboard_fd);
        keyboard_fd = -1;
    }
    if (joystick_fd >= 0) {
        close(joystick_fd);
        joystick_fd = -1;
    }
}

void platform_input_poll(controller_state_t *state, config_t *config) {
    /* Poll keyboard events */
    if (config->enable_keyboard && keyboard_fd >= 0) {
        struct input_event ev;
        
        while (read(keyboard_fd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type != EV_KEY) continue;
            
            bool is_pressed = (ev.value != 0);
            
            /* Find binding for this key */
            for (int i = 0; i < config->binding_count; i++) {
                key_binding_t *binding = &config->bindings[i];
                int key_code = get_key_code(binding->key_name);
                
                if (key_code == 0 || ev.code != key_code) continue;
                
                switch (binding->type) {
                    case INPUT_TYPE_BUTTON:
                        if (is_pressed) {
                            state->buttons |= binding->value.button_mask;
                        } else {
                            state->buttons &= ~binding->value.button_mask;
                        }
                        break;
                    
                    case INPUT_TYPE_DPAD:
                        switch (binding->value.direction) {
                            case DIR_UP:    state->dpad_up = is_pressed; break;
                            case DIR_DOWN:  state->dpad_down = is_pressed; break;
                            case DIR_LEFT:  state->dpad_left = is_pressed; break;
                            case DIR_RIGHT: state->dpad_right = is_pressed; break;
                            default: break;
                        }
                        break;
                    
                    case INPUT_TYPE_LSTICK:
                        switch (binding->value.direction) {
                            case DIR_UP:    state->lstick_up = is_pressed; break;
                            case DIR_DOWN:  state->lstick_down = is_pressed; break;
                            case DIR_LEFT:  state->lstick_left = is_pressed; break;
                            case DIR_RIGHT: state->lstick_right = is_pressed; break;
                            default: break;
                        }
                        break;
                    
                    case INPUT_TYPE_RSTICK:
                        switch (binding->value.direction) {
                            case DIR_UP:    state->rstick_up = is_pressed; break;
                            case DIR_DOWN:  state->rstick_down = is_pressed; break;
                            case DIR_LEFT:  state->rstick_left = is_pressed; break;
                            case DIR_RIGHT: state->rstick_right = is_pressed; break;
                            default: break;
                        }
                        break;
                }
            }
        }
    }
    
    /* Poll joystick events */
    if (config->enable_controller && joystick_fd >= 0) {
        struct js_event js;
        
        while (read(joystick_fd, &js, sizeof(js)) == sizeof(js)) {
            if (js.type & JS_EVENT_BUTTON) {
                bool pressed = (js.value != 0);
                
                /* Standard button mapping */
                switch (js.number) {
                    case 0: /* A */
                        if (pressed) state->buttons |= BTN_B;
                        else state->buttons &= ~BTN_B;
                        break;
                    case 1: /* B */
                        if (pressed) state->buttons |= BTN_A;
                        else state->buttons &= ~BTN_A;
                        break;
                    case 2: /* X */
                        if (pressed) state->buttons |= BTN_Y;
                        else state->buttons &= ~BTN_Y;
                        break;
                    case 3: /* Y */
                        if (pressed) state->buttons |= BTN_X;
                        else state->buttons &= ~BTN_X;
                        break;
                    case 4: /* LB */
                        if (pressed) state->buttons |= BTN_L;
                        else state->buttons &= ~BTN_L;
                        break;
                    case 5: /* RB */
                        if (pressed) state->buttons |= BTN_R;
                        else state->buttons &= ~BTN_R;
                        break;
                    case 6: /* Back */
                        if (pressed) state->buttons |= BTN_MINUS;
                        else state->buttons &= ~BTN_MINUS;
                        break;
                    case 7: /* Start */
                        if (pressed) state->buttons |= BTN_PLUS;
                        else state->buttons &= ~BTN_PLUS;
                        break;
                    case 9: /* Left stick */
                        if (pressed) state->buttons |= BTN_LSTICK;
                        else state->buttons &= ~BTN_LSTICK;
                        break;
                    case 10: /* Right stick */
                        if (pressed) state->buttons |= BTN_RSTICK;
                        else state->buttons &= ~BTN_RSTICK;
                        break;
                }
            } else if (js.type & JS_EVENT_AXIS) {
                int deadzone = (int)(config->controller_deadzone / 100.0f * 32767.0f);
                
                if (abs(js.value) < deadzone) {
                    js.value = 0;
                }
                
                switch (js.number) {
                    case 0: /* Left X */
                        state->lx = (uint8_t)((js.value + 32768) >> 8);
                        break;
                    case 1: /* Left Y */
                        state->ly = (uint8_t)(255 - ((js.value + 32768) >> 8));
                        break;
                    case 2: /* Right X */
                        state->rx = (uint8_t)((js.value + 32768) >> 8);
                        break;
                    case 3: /* Right Y */
                        state->ry = (uint8_t)(255 - ((js.value + 32768) >> 8));
                        break;
                    case 6: /* D-pad X */
                        if (js.value < -16384) state->dpad_left = true;
                        else if (js.value > 16384) state->dpad_right = true;
                        else { state->dpad_left = false; state->dpad_right = false; }
                        break;
                    case 7: /* D-pad Y */
                        if (js.value < -16384) state->dpad_up = true;
                        else if (js.value > 16384) state->dpad_down = true;
                        else { state->dpad_up = false; state->dpad_down = false; }
                        break;
                }
            }
        }
    }
}

#endif /* __linux__ */
