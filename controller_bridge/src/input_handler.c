#include "controller_bridge.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

struct input_handler {
    controller_state_t *state;
    config_t *config;
    bool running;
};

input_handler_t *input_handler_create(controller_state_t *state, config_t *config) {
    input_handler_t *handler = malloc(sizeof(input_handler_t));
    if (!handler) {
        return NULL;
    }
    
    handler->state = state;
    handler->config = config;
    handler->running = false;
    
    return handler;
}

void input_handler_destroy(input_handler_t *handler) {
    if (handler) {
        input_handler_stop(handler);
        free(handler);
    }
}

bool input_handler_start(input_handler_t *handler) {
    if (!handler || handler->running) {
        return false;
    }
    
    if (!platform_input_init()) {
        fprintf(stderr, "Error: Failed to initialize input system\n");
        return false;
    }
    
    handler->running = true;
    return true;
}

void input_handler_stop(input_handler_t *handler) {
    if (handler && handler->running) {
        handler->running = false;
        platform_input_cleanup();
    }
}

bool input_handler_is_running(input_handler_t *handler) {
    return handler && handler->running;
}
