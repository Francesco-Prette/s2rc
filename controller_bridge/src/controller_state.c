#include "controller_bridge.h"
#include <string.h>

void controller_state_init(controller_state_t *state) {
    memset(state, 0, sizeof(controller_state_t));
    state->lx = STICK_CENTER;
    state->ly = STICK_CENTER;
    state->rx = STICK_CENTER;
    state->ry = STICK_CENTER;
}

void controller_state_update_sticks(controller_state_t *state) {
    /* Update left stick */
    if (state->lstick_up) {
        state->ly = STICK_MIN;  /* Y-axis inverted: 0 = up */
    } else if (state->lstick_down) {
        state->ly = STICK_MAX;  /* Y-axis inverted: 255 = down */
    } else {
        state->ly = STICK_CENTER;
    }

    if (state->lstick_left) {
        state->lx = STICK_MIN;  /* 0 = left */
    } else if (state->lstick_right) {
        state->lx = STICK_MAX;  /* 255 = right */
    } else {
        state->lx = STICK_CENTER;
    }

    /* Update right stick */
    if (state->rstick_up) {
        state->ry = STICK_MIN;  /* Y-axis inverted: 0 = up */
    } else if (state->rstick_down) {
        state->ry = STICK_MAX;  /* Y-axis inverted: 255 = down */
    } else {
        state->ry = STICK_CENTER;
    }

    if (state->rstick_left) {
        state->rx = STICK_MIN;  /* 0 = left */
    } else if (state->rstick_right) {
        state->rx = STICK_MAX;  /* 255 = right */
    } else {
        state->rx = STICK_CENTER;
    }
}

uint8_t controller_state_get_hat(const controller_state_t *state) {
    /* Calculate HAT value from D-pad directions */
    if (state->dpad_up && state->dpad_right) {
        return DPAD_UP_RIGHT;
    } else if (state->dpad_up && state->dpad_left) {
        return DPAD_UP_LEFT;
    } else if (state->dpad_down && state->dpad_right) {
        return DPAD_DN_RIGHT;
    } else if (state->dpad_down && state->dpad_left) {
        return DPAD_DN_LEFT;
    } else if (state->dpad_up) {
        return DPAD_UP;
    } else if (state->dpad_down) {
        return DPAD_DOWN;
    } else if (state->dpad_left) {
        return DPAD_LEFT;
    } else if (state->dpad_right) {
        return DPAD_RIGHT;
    } else {
        return DPAD_NEUTRAL;
    }
}

uint8_t apply_stick_calibration(int raw_value, const stick_calibration_t *cal, bool is_y_axis) {
    /* If calibration not available, return raw value */
    if (!cal || !cal->is_calibrated) {
        return (uint8_t)raw_value;
    }
    
    int center = is_y_axis ? cal->center_y : cal->center_x;
    int min = is_y_axis ? cal->min_y : cal->min_x;
    int max = is_y_axis ? cal->max_y : cal->max_x;
    
    /* Apply calibration mapping */
    int calibrated;
    if (raw_value < center) {
        /* Map min..center to 0..128 */
        int range = center - min;
        if (range <= 0) range = 1;  /* Avoid division by zero */
        calibrated = (raw_value - min) * 128 / range;
    } else {
        /* Map center..max to 128..255 */
        int range = max - center;
        if (range <= 0) range = 1;
        calibrated = 128 + ((raw_value - center) * 127 / range);
    }
    
    /* Clamp to valid range */
    if (calibrated < 0) calibrated = 0;
    if (calibrated > 255) calibrated = 255;
    
    return (uint8_t)calibrated;
}

void controller_state_to_packet(const controller_state_t *state, uint8_t *packet) {
    /* Build 10-byte packet: 0xAA 0x55 header + 8 data bytes */
    /* This matches what the s2rc receiver expects */
    
    /* Header bytes for packet synchronization */
    packet[0] = 0xAA;
    packet[1] = 0x55;
    
    /* Bytes 2-3: Buttons (little endian uint16_t) */
    packet[2] = (uint8_t)(state->buttons & 0xFF);
    packet[3] = (uint8_t)((state->buttons >> 8) & 0xFF);
    
    /* Byte 4: D-Pad HAT */
    packet[4] = controller_state_get_hat(state);
    
    /* Byte 5: Left Stick X */
    packet[5] = state->lx;
    
    /* Byte 6: Left Stick Y */
    packet[6] = state->ly;
    
    /* Byte 7: Right Stick X */
    packet[7] = state->rx;
    
    /* Byte 8: Right Stick Y */
    packet[8] = state->ry;
    
    /* Byte 9: Vendor byte (always 0) */
    packet[9] = 0x00;
}
