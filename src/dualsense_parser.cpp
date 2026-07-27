/*
 * SPDX-License-Identifier: MIT
 */

#include "dualsense_parser.h"

namespace {

uint16_t expand_axis(uint8_t value) {
    // Expand the complete 8-bit range to 16 bits while retaining both ends.
    return static_cast<uint16_t>(value) * 0x0101u;
}

int16_t read_s16_le(const uint8_t *data) {
    return static_cast<int16_t>(static_cast<uint16_t>(data[0]) |
                                (static_cast<uint16_t>(data[1]) << 8));
}

} // namespace

bool dualsense_parse_input(const uint8_t *report, size_t len, ControllerState &state) {
    if (report == nullptr || len < 63) return false;

    ControllerState next{};
    next.lx = expand_axis(report[0]);
    next.ly = expand_axis(report[1]);
    next.rx = expand_axis(report[2]);
    next.ry = expand_axis(report[3]);
    next.l2 = report[4];
    next.r2 = report[5];

    const uint8_t face = report[7];
    next.dpad = (face & 0x0f) <= CONTROLLER_DPAD_NEUTRAL
                    ? (face & 0x0f)
                    : CONTROLLER_DPAD_NEUTRAL;
    if (face & 0x10) next.buttons |= CONTROLLER_BUTTON_SQUARE;
    if (face & 0x20) next.buttons |= CONTROLLER_BUTTON_CROSS;
    if (face & 0x40) next.buttons |= CONTROLLER_BUTTON_CIRCLE;
    if (face & 0x80) next.buttons |= CONTROLLER_BUTTON_TRIANGLE;

    const uint8_t shoulders = report[8];
    if (shoulders & 0x01) next.buttons |= CONTROLLER_BUTTON_L1;
    if (shoulders & 0x02) next.buttons |= CONTROLLER_BUTTON_R1;
    if (shoulders & 0x04) next.buttons |= CONTROLLER_BUTTON_L2;
    if (shoulders & 0x08) next.buttons |= CONTROLLER_BUTTON_R2;
    if (shoulders & 0x10) next.buttons |= CONTROLLER_BUTTON_CREATE;
    if (shoulders & 0x20) next.buttons |= CONTROLLER_BUTTON_OPTIONS;
    if (shoulders & 0x40) next.buttons |= CONTROLLER_BUTTON_L3;
    if (shoulders & 0x80) next.buttons |= CONTROLLER_BUTTON_R3;

    const uint8_t system = report[9];
    if (system & 0x01) next.buttons |= CONTROLLER_BUTTON_HOME;
    if (system & 0x02) next.buttons |= CONTROLLER_BUTTON_PAD;
    if (system & 0x04) next.buttons |= CONTROLLER_BUTTON_MUTE;

    // DualSense enhanced input reports store the three sensor axes in X/Y/Z
    // order. Keep that native PlayStation coordinate system here; individual
    // USB output profiles are responsible for their own axis transform.
    next.gyro[0] = read_s16_le(report + 15);
    next.gyro[1] = read_s16_le(report + 17);
    next.gyro[2] = read_s16_le(report + 19);
    next.accel[0] = read_s16_le(report + 21);
    next.accel[1] = read_s16_le(report + 23);
    next.accel[2] = read_s16_le(report + 25);

    next.battery = report[52] & 0x0f;
    const uint8_t power_state = report[52] >> 4;
    next.charging = power_state == 0x01 || power_state == 0x02;

    state = next;
    return true;
}
