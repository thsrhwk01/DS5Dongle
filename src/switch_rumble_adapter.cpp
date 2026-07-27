/*
 * Switch HD Rumble to DualSense compatible-rumble output adapter.
 *
 * SPDX-License-Identifier: MIT
 */

#include "switch_rumble_adapter.h"

#include "bt.h"
#include "config.h"
#include "pico/time.h"
#include "utils.h"

namespace {

constexpr uint16_t DUALSENSE_VIBRATION_V2_MIN_VERSION = 0x0215;
constexpr uint32_t RUMBLE_REFRESH_MS = 1000;

SwitchRumbleState last_rumble{};
uint32_t last_sent_ms = 0;
bool last_rumble_valid = false;

bool dualsense_supports_vibration_v2() {
    if (is_dse) return true;
    const auto firmware = get_feature_data(0x20, 64);
    if (firmware.size() <= 45) return false;
    const uint16_t update_version = static_cast<uint16_t>(firmware[44]) |
                                    (static_cast<uint16_t>(firmware[45]) << 8);
    return update_version >= DUALSENSE_VIBRATION_V2_MIN_VERSION;
}

} // namespace

void switch_rumble_adapter_apply(const SwitchRumbleState &rumble) {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (last_rumble_valid && rumble == last_rumble && now - last_sent_ms < RUMBLE_REFRESH_MS) {
        return;
    }

    SetStateData state{};
    state.UseRumbleNotHaptics = 1;
    if (dualsense_supports_vibration_v2()) {
        state.EnableImprovedRumbleEmulation = 1;
    } else {
        state.EnableRumbleEmulation = 1;
    }
    state.RumbleEmulationLeft = rumble.heavy;
    state.RumbleEmulationRight = rumble.light;
    update_state(state);

    last_rumble = rumble;
    last_sent_ms = now;
    last_rumble_valid = true;
}

void switch_rumble_adapter_stop() {
    // Force a stop packet even when the cached state was already zero. This
    // also covers USB profile changes while the controller is still connected.
    last_rumble_valid = false;
    switch_rumble_adapter_apply({});
}
