#if defined(__APPLE__)

#include "controller_bridge.h"
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <Carbon/Carbon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static IOHIDManagerRef hid_manager = NULL;

/* Key code mappings for macOS */
typedef struct {
    const char *name;
    int key_code;
} key_map_t;

static const key_map_t key_mappings[] = {
    {"a", kVK_ANSI_A}, {"b", kVK_ANSI_B}, {"c", kVK_ANSI_C}, {"d", kVK_ANSI_D},
    {"e", kVK_ANSI_E}, {"f", kVK_ANSI_F}, {"g", kVK_ANSI_G}, {"h", kVK_ANSI_H},
    {"i", kVK_ANSI_I}, {"j", kVK_ANSI_J}, {"k", kVK_ANSI_K}, {"l", kVK_ANSI_L},
    {"m", kVK_ANSI_M}, {"n", kVK_ANSI_N}, {"o", kVK_ANSI_O}, {"p", kVK_ANSI_P},
    {"q", kVK_ANSI_Q}, {"r", kVK_ANSI_R}, {"s", kVK_ANSI_S}, {"t", kVK_ANSI_T},
    {"u", kVK_ANSI_U}, {"v", kVK_ANSI_V}, {"w", kVK_ANSI_W}, {"x", kVK_ANSI_X},
    {"y", kVK_ANSI_Y}, {"z", kVK_ANSI_Z},
    {"0", kVK_ANSI_0}, {"1", kVK_ANSI_1}, {"2", kVK_ANSI_2}, {"3", kVK_ANSI_3},
    {"4", kVK_ANSI_4}, {"5", kVK_ANSI_5}, {"6", kVK_ANSI_6}, {"7", kVK_ANSI_7},
    {"8", kVK_ANSI_8}, {"9", kVK_ANSI_9},
    {"space", kVK_Space}, {"enter", kVK_Return}, {"escape", kVK_Escape},
    {"tab", kVK_Tab}, {"backspace", kVK_Delete}, {"delete", kVK_ForwardDelete},
    {"up", kVK_UpArrow}, {"down", kVK_DownArrow},
    {"left", kVK_LeftArrow}, {"right", kVK_RightArrow},
    {"f1", kVK_F1}, {"f2", kVK_F2}, {"f3", kVK_F3}, {"f4", kVK_F4},
    {"f5", kVK_F5}, {"f6", kVK_F6}, {"f7", kVK_F7}, {"f8", kVK_F8},
    {"f9", kVK_F9}, {"f10", kVK_F10}, {"f11", kVK_F11}, {"f12", kVK_F12},
    {"shift", kVK_Shift}, {"ctrl", kVK_Control}, {"alt", kVK_Option},
    {"num0", kVK_ANSI_Keypad0}, {"num1", kVK_ANSI_Keypad1},
    {"num2", kVK_ANSI_Keypad2}, {"num3", kVK_ANSI_Keypad3},
    {"num4", kVK_ANSI_Keypad4}, {"num5", kVK_ANSI_Keypad5},
    {"num6", kVK_ANSI_Keypad6}, {"num7", kVK_ANSI_Keypad7},
    {"num8", kVK_ANSI_Keypad8}, {"num9", kVK_ANSI_Keypad9},
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

bool platform_input_init(void) {
    /* Create HID manager */
    hid_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!hid_manager) {
        fprintf(stderr, "Error: Could not create HID manager\n");
        return false;
    }
    
    /* Set up device matching - looking for game controllers */
    CFMutableDictionaryRef match_dict = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    
    if (match_dict) {
        /* Match game controllers */
        int usage_page = kHIDPage_GenericDesktop;
        int usage = kHIDUsage_GD_GamePad;
        
        CFNumberRef page_ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page);
        CFNumberRef usage_ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
        
        CFDictionarySetValue(match_dict, CFSTR(kIOHIDDeviceUsagePageKey), page_ref);
        CFDictionarySetValue(match_dict, CFSTR(kIOHIDDeviceUsageKey), usage_ref);
        
        IOHIDManagerSetDeviceMatching(hid_manager, match_dict);
        
        CFRelease(page_ref);
        CFRelease(usage_ref);
        CFRelease(match_dict);
    }
    
    /* Open the HID manager */
    IOReturn result = IOHIDManagerOpen(hid_manager, kIOHIDOptionsTypeNone);
    if (result != kIOReturnSuccess) {
        fprintf(stderr, "Warning: Could not open HID manager (error %d)\n", result);
        fprintf(stderr, "Controller input may not work\n");
    }
    
    return true;
}

void platform_input_cleanup(void) {
    if (hid_manager) {
        IOHIDManagerClose(hid_manager, kIOHIDOptionsTypeNone);
        CFRelease(hid_manager);
        hid_manager = NULL;
    }
}

void platform_input_poll(controller_state_t *state, config_t *config) {
    /* Poll keyboard using Carbon Event Manager */
    if (config->enable_keyboard) {
        for (int i = 0; i < config->binding_count; i++) {
            key_binding_t *binding = &config->bindings[i];
            int key_code = get_key_code(binding->key_name);
            
            if (key_code == 0) continue;
            
            /* Check if key is pressed using CGEventSourceKeyState */
            bool is_pressed = CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, key_code);
            
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
    
    /* Poll HID devices (controllers) */
    if (config->enable_controller && hid_manager) {
        CFSetRef device_set = IOHIDManagerCopyDevices(hid_manager);
        if (device_set) {
            CFIndex device_count = CFSetGetCount(device_set);
            if (device_count > 0) {
                IOHIDDeviceRef *devices = malloc(device_count * sizeof(IOHIDDeviceRef));
                CFSetGetValues(device_set, (const void **)devices);
                
                /* Poll first device only */
                if (device_count > 0) {
                    IOHIDDeviceRef device = devices[0];
                    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);
                    
                    if (elements) {
                        CFIndex element_count = CFArrayGetCount(elements);
                        
                        for (CFIndex i = 0; i < element_count; i++) {
                            IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
                            IOHIDElementType type = IOHIDElementGetType(element);
                            
                            if (type == kIOHIDElementTypeInput_Button ||
                                type == kIOHIDElementTypeInput_Axis) {
                                
                                IOHIDValueRef value;
                                if (IOHIDDeviceGetValue(device, element, &value) == kIOReturnSuccess) {
                                    CFIndex int_value = IOHIDValueGetIntegerValue(value);
                                    uint32_t usage = IOHIDElementGetUsage(element);
                                    
                                    /* Button handling */
                                    if (type == kIOHIDElementTypeInput_Button) {
                                        bool pressed = (int_value != 0);
                                        
                                        /* Map common button usages */
                                        switch (usage) {
                                            case 0x01: /* Button 1 (A) */
                                                if (pressed) state->buttons |= BTN_B;
                                                else state->buttons &= ~BTN_B;
                                                break;
                                            case 0x02: /* Button 2 (B) */
                                                if (pressed) state->buttons |= BTN_A;
                                                else state->buttons &= ~BTN_A;
                                                break;
                                        }
                                    }
                                }
                            }
                        }
                        
                        CFRelease(elements);
                    }
                }
                
                free(devices);
            }
            CFRelease(device_set);
        }
    }
}

#endif /* __APPLE__ */
