/*
 * Neutral controller state shared by USB output profiles.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef DS5_BRIDGE_CONTROLLER_STATE_H
#define DS5_BRIDGE_CONTROLLER_STATE_H

#include <cstdint>

enum ControllerButton : uint32_t {
    CONTROLLER_BUTTON_SQUARE   = 1u << 0,
    CONTROLLER_BUTTON_CROSS    = 1u << 1,
    CONTROLLER_BUTTON_CIRCLE   = 1u << 2,
    CONTROLLER_BUTTON_TRIANGLE = 1u << 3,
    CONTROLLER_BUTTON_L1       = 1u << 4,
    CONTROLLER_BUTTON_R1       = 1u << 5,
    CONTROLLER_BUTTON_L2       = 1u << 6,
    CONTROLLER_BUTTON_R2       = 1u << 7,
    CONTROLLER_BUTTON_CREATE   = 1u << 8,
    CONTROLLER_BUTTON_OPTIONS  = 1u << 9,
    CONTROLLER_BUTTON_L3       = 1u << 10,
    CONTROLLER_BUTTON_R3       = 1u << 11,
    CONTROLLER_BUTTON_HOME     = 1u << 12,
    CONTROLLER_BUTTON_PAD      = 1u << 13,
    CONTROLLER_BUTTON_MUTE     = 1u << 14,
};

enum ControllerDpad : uint8_t {
    CONTROLLER_DPAD_UP = 0,
    CONTROLLER_DPAD_UP_RIGHT,
    CONTROLLER_DPAD_RIGHT,
    CONTROLLER_DPAD_DOWN_RIGHT,
    CONTROLLER_DPAD_DOWN,
    CONTROLLER_DPAD_DOWN_LEFT,
    CONTROLLER_DPAD_LEFT,
    CONTROLLER_DPAD_UP_LEFT,
    CONTROLLER_DPAD_NEUTRAL,
};

struct ControllerState {
    uint32_t buttons = 0;
    uint8_t dpad = CONTROLLER_DPAD_NEUTRAL;
    uint16_t lx = 0x8000;
    uint16_t ly = 0x8000;
    uint16_t rx = 0x8000;
    uint16_t ry = 0x8000;
    uint8_t l2 = 0;
    uint8_t r2 = 0;
    int16_t gyro[3]{};
    int16_t accel[3]{};
    uint8_t battery = 0;
    bool charging = false;
};

#endif // DS5_BRIDGE_CONTROLLER_STATE_H
