/*
 * Nintendo Switch HD Rumble decoder shared by firmware and host tests.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef DS5_BRIDGE_SWITCH_RUMBLE_H
#define DS5_BRIDGE_SWITCH_RUMBLE_H

#include <cstdint>

struct SwitchRumbleState {
    // DualSense compatible-rumble motor values. The heavy motor carries the
    // Switch low-frequency component and the light motor carries the
    // high-frequency component.
    uint8_t heavy = 0;
    uint8_t light = 0;

    bool operator==(const SwitchRumbleState &) const = default;
};

// Decode the two four-byte Switch linear-actuator commands into the two
// frequency bands available through DualSense compatible rumble.
SwitchRumbleState switch_decode_hd_rumble(const uint8_t rumble_data[8]);

#endif // DS5_BRIDGE_SWITCH_RUMBLE_H
