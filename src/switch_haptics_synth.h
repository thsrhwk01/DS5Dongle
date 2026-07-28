/*
 * Switch HD Rumble to stereo PCM synthesizer shared by firmware and host tests.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef DS5_BRIDGE_SWITCH_HAPTICS_SYNTH_H
#define DS5_BRIDGE_SWITCH_HAPTICS_SYNTH_H

#include <cstddef>
#include <cstdint>

#include "switch_rumble.h"

constexpr uint32_t SWITCH_HAPTICS_SAMPLE_RATE = 3000;
constexpr size_t SWITCH_HAPTICS_PACKET_FRAMES = 64;

class SwitchHapticsSynth {
public:
    void reset();
    void set_state(const SwitchRumbleState &state);

    // Render signed 8-bit interleaved stereo PCM. gain_q8 is 256 for unity.
    void render(int8_t *samples, size_t frames, uint16_t gain_q8 = 256);
    bool silent() const;

private:
    struct Oscillator {
        uint32_t phase = 0;
        uint32_t increment = 0;
        int32_t amplitude_q8 = 0;
        int32_t target_amplitude_q8 = 0;
        uint8_t ramp_remaining = 0;
    };

    static void set_band(Oscillator &oscillator, const SwitchRumbleBand &band);
    static int32_t render_oscillator(Oscillator &oscillator);
    static int8_t mix_actuator(Oscillator &low, Oscillator &high, uint16_t gain_q8);

    Oscillator left_low_{};
    Oscillator left_high_{};
    Oscillator right_low_{};
    Oscillator right_high_{};
};

#endif // DS5_BRIDGE_SWITCH_HAPTICS_SYNTH_H
