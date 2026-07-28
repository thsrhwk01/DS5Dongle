/*
 * Nintendo Switch HD Rumble decoder shared by firmware and host tests.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef DS5_BRIDGE_SWITCH_RUMBLE_H
#define DS5_BRIDGE_SWITCH_RUMBLE_H

#include <cstdint>

struct SwitchRumbleBand {
    uint16_t frequency_hz = 0;
    uint8_t amplitude = 0;

    bool operator==(const SwitchRumbleBand &) const = default;
};

struct SwitchRumbleActuator {
    SwitchRumbleBand low{};
    SwitchRumbleBand high{};

    bool operator==(const SwitchRumbleActuator &) const = default;
};

struct SwitchRumbleState {
    SwitchRumbleActuator left{};
    SwitchRumbleActuator right{};

    bool operator==(const SwitchRumbleState &) const = default;

    bool silent() const {
        return left.low.amplitude == 0 && left.high.amplitude == 0 &&
               right.low.amplitude == 0 && right.high.amplitude == 0;
    }
};

// Decode the two four-byte Switch linear-actuator commands. Each actuator can
// request a low- and high-frequency component at independent amplitudes.
SwitchRumbleState switch_decode_hd_rumble(const uint8_t rumble_data[8]);

#endif // DS5_BRIDGE_SWITCH_RUMBLE_H
