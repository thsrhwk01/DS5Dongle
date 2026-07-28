/*
 * Nintendo Switch HD Rumble decoder.
 *
 * The packet layout and logarithmic amplitude ranges are documented by the
 * Nintendo Switch reverse-engineering project and are also used by upstream
 * Linux hid-nintendo. This implementation uses an integer approximation so it
 * remains small and deterministic on RP2350.
 *
 * SPDX-License-Identifier: MIT
 */

#include "switch_rumble.h"

#include <algorithm>

namespace {

uint8_t decode_high_amplitude_code(const uint8_t data[4]) {
    // Bit zero belongs to the high-frequency code. The remaining even value
    // is twice the logarithmic amplitude index.
    return static_cast<uint8_t>((data[1] & 0xfeu) >> 1);
}

uint8_t decode_low_amplitude_code(const uint8_t data[4]) {
    // The low amplitude index is split between byte 2 bit 7 and byte 3.
    // Canonical zero amplitude is 0x0040.
    if (data[3] < 0x40) return 0;
    const unsigned code = 2u * (data[3] - 0x40u) + ((data[2] >> 7) & 1u);
    return static_cast<uint8_t>(std::min(code, 100u));
}

uint8_t amplitude_code_to_u8(uint8_t code) {
    if (code == 0) return 0;
    code = std::min<uint8_t>(code, 100);

    // Reconstruct physical amplitude (x1000) from the three logarithmic
    // ranges. Constants are fixed-point approximations of 2^(1/4), 2^(1/16)
    // and 2^(1/32). Code 100 is the highest actuator-safe table entry.
    uint32_t amplitude_milli;
    uint8_t start;
    uint32_t multiplier_q16;
    if (code == 1) {
        amplitude_milli = 8;
        start = 1;
        multiplier_q16 = 1u << 16;
    } else if (code < 16) {
        amplitude_milli = 12;
        start = 2;
        multiplier_q16 = 77936; // 2^(1/4)
    } else if (code < 32) {
        amplitude_milli = 117;
        start = 16;
        multiplier_q16 = 68438; // 2^(1/16)
    } else {
        amplitude_milli = 230;
        start = 32;
        multiplier_q16 = 66971; // 2^(1/32)
    }

    for (uint8_t i = start; i < code; ++i) {
        amplitude_milli = (amplitude_milli * multiplier_q16 + 0x8000u) >> 16;
    }
    amplitude_milli = std::min<uint32_t>(amplitude_milli, 1003);
    return static_cast<uint8_t>((amplitude_milli * 255u + 501u) / 1003u);
}

uint16_t frequency_code_to_hz(const uint8_t code) {
    // Nintendo's table is 10 * 2^(code / 32). Start at code 0x40 (40 Hz)
    // and step with a Q16 approximation of 2^(1/32). The complete encoded
    // range tops out near 1.25 kHz, below the 1.5 kHz Nyquist limit of the
    // DualSense 3 kHz haptics stream used by this firmware.
    uint32_t frequency_millihz = 40000;
    for (uint8_t i = 0x40; i < code; ++i) {
        frequency_millihz = static_cast<uint32_t>(
            (static_cast<uint64_t>(frequency_millihz) * 66971u + 0x8000u) >> 16);
    }
    return static_cast<uint16_t>((frequency_millihz + 500u) / 1000u);
}

uint16_t decode_high_frequency(const uint8_t data[4]) {
    const uint16_t encoded = static_cast<uint16_t>(data[0]) |
                             (static_cast<uint16_t>(data[1] & 1u) << 8);
    return frequency_code_to_hz(static_cast<uint8_t>(0x60u + (encoded >> 2)));
}

uint16_t decode_low_frequency(const uint8_t data[4]) {
    return frequency_code_to_hz(static_cast<uint8_t>(0x40u + (data[2] & 0x7fu)));
}

SwitchRumbleActuator decode_actuator(const uint8_t data[4]) {
    return {
        .low = {
            .frequency_hz = decode_low_frequency(data),
            .amplitude = amplitude_code_to_u8(decode_low_amplitude_code(data)),
        },
        .high = {
            .frequency_hz = decode_high_frequency(data),
            .amplitude = amplitude_code_to_u8(decode_high_amplitude_code(data)),
        },
    };
}

} // namespace

SwitchRumbleState switch_decode_hd_rumble(const uint8_t rumble_data[8]) {
    if (rumble_data == nullptr) return {};

    return {
        .left = decode_actuator(rumble_data),
        .right = decode_actuator(rumble_data + 4),
    };
}
