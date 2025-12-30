#ifdef _WIN32

#define DIRECTINPUT_VERSION 0x0800
#include "controller_bridge.h"
#include <windows.h>
#include <xinput.h>
#include <dinput.h>
#include <stdio.h>
#include <string.h>

/* Virtual key code mappings */
typedef struct {
    const char *name;
    int vk_code;
} key_map_t;

static const key_map_t key_mappings[] = {
    {"a", 'A'}, {"b", 'B'}, {"c", 'C'}, {"d", 'D'}, {"e", 'E'},
    {"f", 'F'}, {"g", 'G'}, {"h", 'H'}, {"i", 'I'}, {"j", 'J'},
    {"k", 'K'}, {"l", 'L'}, {"m", 'M'}, {"n", 'N'}, {"o", 'O'},
    {"p", 'P'}, {"q", 'Q'}, {"r", 'R'}, {"s", 'S'}, {"t", 'T'},
    {"u", 'U'}, {"v", 'V'}, {"w", 'W'}, {"x", 'X'}, {"y", 'Y'},
    {"z", 'Z'},
    {"0", '0'}, {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'},
    {"5", '5'}, {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'},
    {"space", VK_SPACE}, {"enter", VK_RETURN}, {"escape", VK_ESCAPE},
    {"tab", VK_TAB}, {"backspace", VK_BACK}, {"delete", VK_DELETE},
    {"up", VK_UP}, {"down", VK_DOWN}, {"left", VK_LEFT}, {"right", VK_RIGHT},
    {"f1", VK_F1}, {"f2", VK_F2}, {"f3", VK_F3}, {"f4", VK_F4},
    {"f5", VK_F5}, {"f6", VK_F6}, {"f7", VK_F7}, {"f8", VK_F8},
    {"f9", VK_F9}, {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12},
    {"shift", VK_SHIFT}, {"ctrl", VK_CONTROL}, {"alt", VK_MENU},
    {"num0", VK_NUMPAD0}, {"num1", VK_NUMPAD1}, {"num2", VK_NUMPAD2},
    {"num3", VK_NUMPAD3}, {"num4", VK_NUMPAD4}, {"num5", VK_NUMPAD5},
    {"num6", VK_NUMPAD6}, {"num7", VK_NUMPAD7}, {"num8", VK_NUMPAD8},
    {"num9", VK_NUMPAD9},
    {NULL, 0}
};

/* DirectInput globals */
static LPDIRECTINPUT8 g_dinput = NULL;
static LPDIRECTINPUTDEVICE8 g_gamepad = NULL;
static bool g_dinput_initialized = false;

static int get_vk_code(const char *key_name) {
    for (int i = 0; key_mappings[i].name != NULL; i++) {
        if (strcmp(key_mappings[i].name, key_name) == 0) {
            return key_mappings[i].vk_code;
        }
    }
    return 0;
}

/* DirectInput device enumeration callback */
static BOOL CALLBACK enum_joysticks_callback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext) {
    HRESULT hr;
    (void)pContext;
    
    /* Skip XInput devices (we handle those separately) */
    /* This is a simplified check - proper implementation would use device VID/PID */
    
    /* Try to create device interface */
    hr = IDirectInput8_CreateDevice(g_dinput, &pdidInstance->guidInstance, &g_gamepad, NULL);
    if (FAILED(hr)) {
        return DIENUM_CONTINUE;
    }
    
    /* Set data format */
    hr = IDirectInputDevice8_SetDataFormat(g_gamepad, &c_dfDIJoystick2);
    if (FAILED(hr)) {
        IDirectInputDevice8_Release(g_gamepad);
        g_gamepad = NULL;
        return DIENUM_CONTINUE;
    }
    
    /* Set cooperative level */
    hr = IDirectInputDevice8_SetCooperativeLevel(g_gamepad, GetConsoleWindow(), 
                                                   DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
    if (FAILED(hr)) {
        IDirectInputDevice8_Release(g_gamepad);
        g_gamepad = NULL;
        return DIENUM_CONTINUE;
    }
    
    /* Acquire the device */
    IDirectInputDevice8_Acquire(g_gamepad);
    
    printf("DirectInput gamepad found: %s\n", pdidInstance->tszProductName);
    
    /* Stop enumeration after finding first gamepad */
    return DIENUM_STOP;
}

bool platform_input_init(void) {
    HRESULT hr;
    
    /* Initialize DirectInput */
    hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
                           &IID_IDirectInput8, (VOID**)&g_dinput, NULL);
    if (FAILED(hr)) {
        fprintf(stderr, "Warning: DirectInput initialization failed (error 0x%08lx)\n", hr);
        fprintf(stderr, "DirectInput controllers (PS4/PS5) will not be detected.\n");
        return true;  /* Continue anyway, XInput might still work */
    }
    
    /* Enumerate game controllers */
    hr = IDirectInput8_EnumDevices(g_dinput, DI8DEVCLASS_GAMECTRL, 
                                   enum_joysticks_callback, NULL, DIEDFL_ATTACHEDONLY);
    if (FAILED(hr)) {
        fprintf(stderr, "Warning: Failed to enumerate DirectInput devices\n");
    }
    
    g_dinput_initialized = true;
    return true;
}

void platform_input_cleanup(void) {
    if (g_gamepad) {
        IDirectInputDevice8_Unacquire(g_gamepad);
        IDirectInputDevice8_Release(g_gamepad);
        g_gamepad = NULL;
    }
    
    if (g_dinput) {
        IDirectInput8_Release(g_dinput);
        g_dinput = NULL;
    }
    
    g_dinput_initialized = false;
}

void platform_input_poll(controller_state_t *state, config_t *config) {
    /* Poll keyboard */
    if (config->enable_keyboard) {
        for (int i = 0; i < config->binding_count; i++) {
            key_binding_t *binding = &config->bindings[i];
            int vk = get_vk_code(binding->key_name);
            
            if (vk == 0) continue;
            
            bool is_pressed = (GetAsyncKeyState(vk) & 0x8000) != 0;
            
            if (is_pressed) {
                switch (binding->type) {
                    case INPUT_TYPE_BUTTON:
                        state->buttons |= binding->value.button_mask;
                        break;
                    
                    case INPUT_TYPE_DPAD:
                        switch (binding->value.direction) {
                            case DIR_UP:    state->dpad_up = true; break;
                            case DIR_DOWN:  state->dpad_down = true; break;
                            case DIR_LEFT:  state->dpad_left = true; break;
                            case DIR_RIGHT: state->dpad_right = true; break;
                            default: break;
                        }
                        break;
                    
                    case INPUT_TYPE_LSTICK:
                        switch (binding->value.direction) {
                            case DIR_UP:    state->lstick_up = true; break;
                            case DIR_DOWN:  state->lstick_down = true; break;
                            case DIR_LEFT:  state->lstick_left = true; break;
                            case DIR_RIGHT: state->lstick_right = true; break;
                            default: break;
                        }
                        break;
                    
                    case INPUT_TYPE_RSTICK:
                        switch (binding->value.direction) {
                            case DIR_UP:    state->rstick_up = true; break;
                            case DIR_DOWN:  state->rstick_down = true; break;
                            case DIR_LEFT:  state->rstick_left = true; break;
                            case DIR_RIGHT: state->rstick_right = true; break;
                            default: break;
                        }
                        break;
                }
            }
        }
    }
    
    /* Poll controllers */
    if (config->enable_controller) {
        bool controller_found = false;
        
        /* Try XInput first (Xbox controllers) */
        XINPUT_STATE xinput_state;
        DWORD result = XInputGetState(0, &xinput_state);
        
        if (result == ERROR_SUCCESS) {
            controller_found = true;
            XINPUT_GAMEPAD *pad = &xinput_state.Gamepad;
            
            /* Map Xbox buttons to Switch layout */
            if (pad->wButtons & XINPUT_GAMEPAD_A) state->buttons |= BTN_B;      /* Xbox A -> Switch B */
            if (pad->wButtons & XINPUT_GAMEPAD_B) state->buttons |= BTN_A;      /* Xbox B -> Switch A */
            if (pad->wButtons & XINPUT_GAMEPAD_X) state->buttons |= BTN_Y;      /* Xbox X -> Switch Y */
            if (pad->wButtons & XINPUT_GAMEPAD_Y) state->buttons |= BTN_X;      /* Xbox Y -> Switch X */
            if (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) state->buttons |= BTN_L;
            if (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) state->buttons |= BTN_R;
            if (pad->bLeftTrigger > 128) state->buttons |= BTN_ZL;
            if (pad->bRightTrigger > 128) state->buttons |= BTN_ZR;
            if (pad->wButtons & XINPUT_GAMEPAD_BACK) state->buttons |= BTN_MINUS;
            if (pad->wButtons & XINPUT_GAMEPAD_START) state->buttons |= BTN_PLUS;
            if (pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) state->buttons |= BTN_LSTICK;
            if (pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) state->buttons |= BTN_RSTICK;
            
            /* D-Pad */
            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) state->dpad_up = true;
            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) state->dpad_down = true;
            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) state->dpad_left = true;
            if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) state->dpad_right = true;
            
            /* Analog sticks with deadzone */
            int deadzone = (int)(config->controller_deadzone / 100.0f * 32767.0f);
            
            if (abs(pad->sThumbLX) > deadzone || abs(pad->sThumbLY) > deadzone) {
                state->lx = (uint8_t)((pad->sThumbLX + 32768) >> 8);
                state->ly = (uint8_t)(255 - ((pad->sThumbLY + 32768) >> 8));
            } else {
                /* Stick within deadzone - return to center */
                state->lx = 128;
                state->ly = 128;
            }
            
            if (abs(pad->sThumbRX) > deadzone || abs(pad->sThumbRY) > deadzone) {
                state->rx = (uint8_t)((pad->sThumbRX + 32768) >> 8);
                state->ry = (uint8_t)(255 - ((pad->sThumbRY + 32768) >> 8));
            } else {
                /* Stick within deadzone - return to center */
                state->rx = 128;
                state->ry = 128;
            }
        }
        
        /* If no XInput controller, try DirectInput (PS4/PS5/other controllers) */
        if (!controller_found && g_gamepad) {
            DIJOYSTATE2 js;
            HRESULT hr;
            
            /* Poll the device */
            hr = IDirectInputDevice8_Poll(g_gamepad);
            if (FAILED(hr)) {
                /* Device may have been unplugged, try to reacquire */
                hr = IDirectInputDevice8_Acquire(g_gamepad);
                if (SUCCEEDED(hr)) {
                    hr = IDirectInputDevice8_Poll(g_gamepad);
                }
            }
            
            if (SUCCEEDED(hr)) {
                /* Get device state */
                hr = IDirectInputDevice8_GetDeviceState(g_gamepad, sizeof(DIJOYSTATE2), &js);
                if (SUCCEEDED(hr)) {
                    controller_found = true;
                    
                    /* Check if custom controller bindings are configured */
                    if (config->use_custom_controller_bindings && config->controller_bindings) {
                        /* Use custom button mappings */
                        for (int i = 0; i < config->controller_binding_count; i++) {
                            int btn_idx = config->controller_bindings[i].controller_button_index;
                            if (btn_idx >= 0 && btn_idx < 128 && (js.rgbButtons[btn_idx] & 0x80)) {
                                state->buttons |= config->controller_bindings[i].switch_button_mask;
                            }
                        }
                    } else {
                        /* Use default PS5/DirectInput button mapping */
                        /* Button mapping for PS5 controller: */
                        /* Button 0 = Square, 1 = X, 2 = Circle, 3 = Triangle */
                        if (js.rgbButtons[1] & 0x80) state->buttons |= BTN_B;      /* PS X -> Switch B */
                        if (js.rgbButtons[2] & 0x80) state->buttons |= BTN_A;      /* PS Circle -> Switch A */
                        if (js.rgbButtons[0] & 0x80) state->buttons |= BTN_Y;      /* PS Square -> Switch Y */
                        if (js.rgbButtons[3] & 0x80) state->buttons |= BTN_X;      /* PS Triangle -> Switch X */
                        if (js.rgbButtons[4] & 0x80) state->buttons |= BTN_L;      /* PS L1 */
                        if (js.rgbButtons[5] & 0x80) state->buttons |= BTN_R;      /* PS R1 */
                        if (js.rgbButtons[6] & 0x80) state->buttons |= BTN_ZL;     /* PS L2 */
                        if (js.rgbButtons[7] & 0x80) state->buttons |= BTN_ZR;     /* PS R2 */
                        if (js.rgbButtons[8] & 0x80) state->buttons |= BTN_MINUS;  /* PS Share/Create */
                        if (js.rgbButtons[9] & 0x80) state->buttons |= BTN_PLUS;   /* PS Options */
                        if (js.rgbButtons[10] & 0x80) state->buttons |= BTN_LSTICK; /* PS L3 */
                        if (js.rgbButtons[11] & 0x80) state->buttons |= BTN_RSTICK; /* PS R3 */
                        if (js.rgbButtons[12] & 0x80) state->buttons |= BTN_HOME;   /* PS Home */
                        if (js.rgbButtons[13] & 0x80) state->buttons |= BTN_CAPTURE; /* PS Touchpad */
                    }
                    
                    /* D-Pad from POV hat */
                    if (js.rgdwPOV[0] != (DWORD)-1) {
                        DWORD pov = js.rgdwPOV[0];
                        /* POV is in hundredths of degrees (0 = up, 9000 = right, 18000 = down, 27000 = left) */
                        if (pov >= 31500 || pov <= 4500) state->dpad_up = true;
                        if (pov >= 4500 && pov <= 13500) state->dpad_right = true;
                        if (pov >= 13500 && pov <= 22500) state->dpad_down = true;
                        if (pov >= 22500 && pov <= 31500) state->dpad_left = true;
                    }
                    
                    /* Store raw stick values for calibration */
                    g_raw_lx = js.lX >> 8;  /* Convert to 0-255 range */
                    g_raw_ly = js.lY >> 8;
                    g_raw_rx = js.lZ >> 8;
                    g_raw_ry = js.lRz >> 8;
                    
                    
                    /* Analog sticks - DirectInput range is 0-65535 */
                    /* Use calibrated center if available (scale from 0-255 to 0-65535), otherwise assume 32767 */
                    int left_center_x = config->left_stick_cal.is_calibrated ? (config->left_stick_cal.center_x * 257) : 32767;
                    int left_center_y = config->left_stick_cal.is_calibrated ? (config->left_stick_cal.center_y * 257) : 32767;
                    int right_center_x = config->right_stick_cal.is_calibrated ? (config->right_stick_cal.center_x * 257) : 32767;
                    int right_center_y = config->right_stick_cal.is_calibrated ? (config->right_stick_cal.center_y * 257) : 32767;
                    
                    int deadzone = (int)(config->controller_deadzone / 100.0f * 32767.0f);
                    int lx_centered = (int)js.lX - left_center_x;
                    int ly_centered = (int)js.lY - left_center_y;
                    int rx_centered = (int)js.lZ - right_center_x;
                    int ry_centered = (int)js.lRz - right_center_y;
                    
                    /* Apply deadzone and calibration */
                    if (abs(lx_centered) > deadzone || abs(ly_centered) > deadzone) {
                        /* Stick is outside deadzone - apply calibration if available */
                        if (config->left_stick_cal.is_calibrated) {
                            state->lx = apply_stick_calibration(g_raw_lx, &config->left_stick_cal, false);
                            state->ly = apply_stick_calibration(g_raw_ly, &config->left_stick_cal, true);
                        } else {
                            /* Convert from DirectInput range to Switch range (0-255, center 128) */
                            /* Map values relative to actual center point */
                            if (js.lX < left_center_x) {
                                /* Below center: map [0, center] → [0, 128] */
                                state->lx = (uint8_t)((js.lX * 128) / left_center_x);
                            } else {
                                /* Above center: map [center, 65535] → [128, 255] */
                                state->lx = (uint8_t)(128 + ((js.lX - left_center_x) * 127) / (65535 - left_center_x));
                            }
                            
                            if (js.lY < left_center_y) {
                                state->ly = (uint8_t)((js.lY * 128) / left_center_y);
                            } else {
                                state->ly = (uint8_t)(128 + ((js.lY - left_center_y) * 127) / (65535 - left_center_y));
                            }
                        }
                    } else {
                        /* Stick is within deadzone - return to center */
                        state->lx = 128;
                        state->ly = 128;
                    }
                    
                    if (abs(rx_centered) > deadzone || abs(ry_centered) > deadzone) {
                        /* Stick is outside deadzone - apply calibration if available */
                        if (config->right_stick_cal.is_calibrated) {
                            state->rx = apply_stick_calibration(g_raw_rx, &config->right_stick_cal, false);
                            state->ry = apply_stick_calibration(g_raw_ry, &config->right_stick_cal, true);
                        } else {
                            /* Convert from DirectInput range to Switch range (0-255, center 128) */
                            /* Map values relative to actual center point */
                            if (js.lZ < right_center_x) {
                                /* Below center: map [0, center] → [0, 128] */
                                state->rx = (uint8_t)((js.lZ * 128) / right_center_x);
                            } else {
                                /* Above center: map [center, 65535] → [128, 255] */
                                state->rx = (uint8_t)(128 + ((js.lZ - right_center_x) * 127) / (65535 - right_center_x));
                            }
                            
                            if (js.lRz < right_center_y) {
                                state->ry = (uint8_t)((js.lRz * 128) / right_center_y);
                            } else {
                                state->ry = (uint8_t)(128 + ((js.lRz - right_center_y) * 127) / (65535 - right_center_y));
                            }
                        }
                    } else {
                        /* Stick is within deadzone - return to center */
                        state->rx = 128;
                        state->ry = 128;
                    }
                }
            }
        }
    }
}

#endif /* _WIN32 */
