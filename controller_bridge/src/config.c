#include "controller_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN 512

/* Helper function to trim whitespace */
static char *trim_whitespace(char *str) {
    char *end;
    
    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    return str;
}

/* Helper to parse button name */
static uint16_t parse_button_name(const char *name) {
    if (strcmp(name, "A") == 0) return BTN_A;
    if (strcmp(name, "B") == 0) return BTN_B;
    if (strcmp(name, "X") == 0) return BTN_X;
    if (strcmp(name, "Y") == 0) return BTN_Y;
    if (strcmp(name, "L") == 0) return BTN_L;
    if (strcmp(name, "R") == 0) return BTN_R;
    if (strcmp(name, "ZL") == 0) return BTN_ZL;
    if (strcmp(name, "ZR") == 0) return BTN_ZR;
    if (strcmp(name, "MINUS") == 0) return BTN_MINUS;
    if (strcmp(name, "PLUS") == 0) return BTN_PLUS;
    if (strcmp(name, "LSTICK") == 0) return BTN_LSTICK;
    if (strcmp(name, "RSTICK") == 0) return BTN_RSTICK;
    if (strcmp(name, "HOME") == 0) return BTN_HOME;
    if (strcmp(name, "CAPTURE") == 0) return BTN_CAPTURE;
    if (strcmp(name, "GL") == 0) return BTN_GL;
    if (strcmp(name, "GR") == 0) return BTN_GR;
    return 0;
}

/* Helper to parse direction */
static input_direction_t parse_direction(const char *dir) {
    if (strcmp(dir, "up") == 0) return DIR_UP;
    if (strcmp(dir, "down") == 0) return DIR_DOWN;
    if (strcmp(dir, "left") == 0) return DIR_LEFT;
    if (strcmp(dir, "right") == 0) return DIR_RIGHT;
    return DIR_NONE;
}

bool config_load(config_t *config, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return false;
    }
    
    memset(config, 0, sizeof(config_t));
    
    /* Set defaults */
    strcpy(config->serial_port, "COM3");
    config->baud_rate = 115200;
    config->enable_keyboard = true;
    config->enable_controller = true;
    config->update_rate_hz = 1000;
    config->controller_deadzone = 10;
    config->bindings = NULL;
    config->binding_count = 0;
    config->controller_bindings = NULL;
    config->controller_binding_count = 0;
    config->use_custom_controller_bindings = false;
    
    /* Initialize stick calibration */
    memset(&config->left_stick_cal, 0, sizeof(stick_calibration_t));
    memset(&config->right_stick_cal, 0, sizeof(stick_calibration_t));
    
    /* Allocate initial bindings array */
    int bindings_capacity = 64;
    config->bindings = malloc(bindings_capacity * sizeof(key_binding_t));
    if (!config->bindings) {
        fclose(file);
        return false;
    }
    
    char line[MAX_LINE_LEN];
    char section[64] = "";
    
    while (fgets(line, sizeof(line), file)) {
        char *trimmed = trim_whitespace(line);
        
        /* Skip empty lines and comments */
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }
        
        /* Section header */
        if (trimmed[0] == '[') {
            char *end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                strncpy(section, trimmed + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';
            }
            continue;
        }
        
        /* Key = Value */
        char *equals = strchr(trimmed, '=');
        if (!equals) continue;
        
        *equals = '\0';
        char *key = trim_whitespace(trimmed);
        char *value = trim_whitespace(equals + 1);
        
        /* Parse based on section */
        if (strcmp(section, "Serial") == 0) {
            if (strcmp(key, "port") == 0) {
                strncpy(config->serial_port, value, MAX_PATH_LEN - 1);
                config->serial_port[MAX_PATH_LEN - 1] = '\0';
            } else if (strcmp(key, "baud_rate") == 0) {
                config->baud_rate = atoi(value);
            }
        } else if (strcmp(section, "General") == 0) {
            if (strcmp(key, "enable_keyboard") == 0) {
                config->enable_keyboard = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "enable_controller") == 0) {
                config->enable_controller = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "update_rate_hz") == 0) {
                config->update_rate_hz = atoi(value);
            } else if (strcmp(key, "controller_deadzone") == 0) {
                config->controller_deadzone = atoi(value);
            }
        } else if (strcmp(section, "KeyBindings") == 0) {
            /* Parse binding: type:value */
            char *colon = strchr(value, ':');
            if (!colon) continue;
            
            *colon = '\0';
            char *type_str = trim_whitespace(value);
            char *value_str = trim_whitespace(colon + 1);
            
            /* Expand bindings array if needed */
            if (config->binding_count >= bindings_capacity) {
                bindings_capacity *= 2;
                key_binding_t *new_bindings = realloc(config->bindings, 
                                                       bindings_capacity * sizeof(key_binding_t));
                if (!new_bindings) {
                    fclose(file);
                    config_free(config);
                    return false;
                }
                config->bindings = new_bindings;
            }
            
            key_binding_t *binding = &config->bindings[config->binding_count];
            strncpy(binding->key_name, key, MAX_KEY_NAME - 1);
            binding->key_name[MAX_KEY_NAME - 1] = '\0';
            
            /* Parse type and value */
            if (strcmp(type_str, "button") == 0) {
                binding->type = INPUT_TYPE_BUTTON;
                binding->value.button_mask = parse_button_name(value_str);
                if (binding->value.button_mask != 0) {
                    config->binding_count++;
                }
            } else if (strcmp(type_str, "dpad") == 0) {
                binding->type = INPUT_TYPE_DPAD;
                binding->value.direction = parse_direction(value_str);
                if (binding->value.direction != DIR_NONE) {
                    config->binding_count++;
                }
            } else if (strcmp(type_str, "lstick") == 0) {
                binding->type = INPUT_TYPE_LSTICK;
                binding->value.direction = parse_direction(value_str);
                if (binding->value.direction != DIR_NONE) {
                    config->binding_count++;
                }
            } else if (strcmp(type_str, "rstick") == 0) {
                binding->type = INPUT_TYPE_RSTICK;
                binding->value.direction = parse_direction(value_str);
                if (binding->value.direction != DIR_NONE) {
                    config->binding_count++;
                }
            }
        } else if (strcmp(section, "ControllerBindings") == 0) {
            /* Parse controller button binding: button_index = switch_button_name */
            int button_index = atoi(key);
            uint16_t switch_button = parse_button_name(value);
            
            if (button_index >= 0 && switch_button != 0) {
                /* Allocate controller bindings if not already */
                if (!config->controller_bindings) {
                    config->controller_bindings = malloc(32 * sizeof(controller_button_binding_t));
                    if (!config->controller_bindings) {
                        fclose(file);
                        config_free(config);
                        return false;
                    }
                }
                
                /* Expand if needed */
                if (config->controller_binding_count >= 32) {
                    controller_button_binding_t *new_bindings = realloc(
                        config->controller_bindings,
                        (config->controller_binding_count + 16) * sizeof(controller_button_binding_t)
                    );
                    if (!new_bindings) {
                        fclose(file);
                        config_free(config);
                        return false;
                    }
                    config->controller_bindings = new_bindings;
                }
                
                config->controller_bindings[config->controller_binding_count].controller_button_index = button_index;
                config->controller_bindings[config->controller_binding_count].switch_button_mask = switch_button;
                config->controller_binding_count++;
                config->use_custom_controller_bindings = true;
            }
        } else if (strcmp(section, "StickCalibration") == 0) {
            /* Parse stick calibration data */
            if (strcmp(key, "left_center_x") == 0) {
                config->left_stick_cal.center_x = atoi(value);
            } else if (strcmp(key, "left_center_y") == 0) {
                config->left_stick_cal.center_y = atoi(value);
            } else if (strcmp(key, "left_min_x") == 0) {
                config->left_stick_cal.min_x = atoi(value);
            } else if (strcmp(key, "left_max_x") == 0) {
                config->left_stick_cal.max_x = atoi(value);
            } else if (strcmp(key, "left_min_y") == 0) {
                config->left_stick_cal.min_y = atoi(value);
            } else if (strcmp(key, "left_max_y") == 0) {
                config->left_stick_cal.max_y = atoi(value);
                config->left_stick_cal.is_calibrated = true;
            } else if (strcmp(key, "right_center_x") == 0) {
                config->right_stick_cal.center_x = atoi(value);
            } else if (strcmp(key, "right_center_y") == 0) {
                config->right_stick_cal.center_y = atoi(value);
            } else if (strcmp(key, "right_min_x") == 0) {
                config->right_stick_cal.min_x = atoi(value);
            } else if (strcmp(key, "right_max_x") == 0) {
                config->right_stick_cal.max_x = atoi(value);
            } else if (strcmp(key, "right_min_y") == 0) {
                config->right_stick_cal.min_y = atoi(value);
            } else if (strcmp(key, "right_max_y") == 0) {
                config->right_stick_cal.max_y = atoi(value);
                config->right_stick_cal.is_calibrated = true;
            }
        }
    }
    
    fclose(file);
    return true;
}

bool config_create_default(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        return false;
    }
    
    fprintf(file, "# Controller Bridge Configuration File\n");
    fprintf(file, "# Edit this file to customize your key bindings and settings\n\n");
    
    fprintf(file, "[Serial]\n");
    fprintf(file, "port = COM3\n");
    fprintf(file, "baud_rate = 115200\n\n");
    
    fprintf(file, "[General]\n");
    fprintf(file, "enable_keyboard = true\n");
    fprintf(file, "enable_controller = true\n");
    fprintf(file, "update_rate_hz = 1000\n");
    fprintf(file, "controller_deadzone = 10\n\n");
    
    fprintf(file, "[KeyBindings]\n");
    fprintf(file, "# Face buttons\n");
    fprintf(file, "u = button:X\n");
    fprintf(file, "j = button:Y\n");
    fprintf(file, "k = button:A\n");
    fprintf(file, "i = button:B\n\n");
    
    fprintf(file, "# Shoulders\n");
    fprintf(file, "l = button:L\n");
    fprintf(file, "f = button:R\n");
    fprintf(file, "t = button:ZL\n");
    fprintf(file, "s = button:ZR\n\n");
    
    fprintf(file, "# System\n");
    fprintf(file, "1 = button:MINUS\n");
    fprintf(file, "2 = button:PLUS\n");
    fprintf(file, "h = button:HOME\n");
    fprintf(file, "c = button:CAPTURE\n\n");
    
    fprintf(file, "# D-Pad\n");
    fprintf(file, "up = dpad:up\n");
    fprintf(file, "down = dpad:down\n");
    fprintf(file, "left = dpad:left\n");
    fprintf(file, "right = dpad:right\n\n");
    
    fprintf(file, "# Left Stick\n");
    fprintf(file, "w = lstick:up\n");
    fprintf(file, "a = lstick:left\n");
    fprintf(file, "s = lstick:down\n");
    fprintf(file, "d = lstick:right\n\n");
    
    fclose(file);
    return true;
}

void config_free(config_t *config) {
    if (config->bindings) {
        free(config->bindings);
        config->bindings = NULL;
    }
    config->binding_count = 0;
    
    if (config->controller_bindings) {
        free(config->controller_bindings);
        config->controller_bindings = NULL;
    }
    config->controller_binding_count = 0;
}

key_binding_t *config_find_binding(config_t *config, const char *key_name) {
    for (int i = 0; i < config->binding_count; i++) {
        if (strcmp(config->bindings[i].key_name, key_name) == 0) {
            return &config->bindings[i];
        }
    }
    return NULL;
}
