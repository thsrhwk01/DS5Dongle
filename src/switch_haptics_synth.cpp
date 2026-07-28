/*
 * Switch HD Rumble to stereo PCM synthesizer.
 *
 * SPDX-License-Identifier: MIT
 */

#include "switch_haptics_synth.h"

#include <algorithm>

namespace {

constexpr uint8_t AMPLITUDE_RAMP_SAMPLES = 16;

int32_t sine_q15(uint32_t phase) {
    // A corrected parabolic sine approximation avoids four soft-float sin()
    // calls per sample on the RP2350. Input phase wraps over all 32 bits and
    // output is Q15. The correction reduces the parabola's peak-region error.
    int32_t x = static_cast<int32_t>(phase >> 16);
    if (x >= 32768) x -= 65536;
    const int32_t magnitude = x < 0 ? -x : x;
    int32_t y = static_cast<int32_t>((4ll * x * (32768 - magnitude)) >> 15);
    const int32_t y_magnitude = y < 0 ? -y : y;
    const int32_t correction = static_cast<int32_t>(
        (7373ll * (((static_cast<int64_t>(y) * y_magnitude) >> 15) - y)) >> 15);
    y += correction;
    return std::clamp<int32_t>(y, -32767, 32767);
}

uint32_t frequency_to_increment(uint16_t frequency_hz) {
    if (frequency_hz == 0) return 0;
    return static_cast<uint32_t>((static_cast<uint64_t>(frequency_hz) << 32) /
                                 SWITCH_HAPTICS_SAMPLE_RATE);
}

} // namespace

void SwitchHapticsSynth::reset() {
    left_low_ = {};
    left_high_ = {};
    right_low_ = {};
    right_high_ = {};
}

void SwitchHapticsSynth::set_band(Oscillator &oscillator, const SwitchRumbleBand &band) {
    oscillator.increment = frequency_to_increment(band.frequency_hz);
    const int32_t target = static_cast<int32_t>(band.amplitude) << 8;
    if (target != oscillator.target_amplitude_q8) {
        oscillator.target_amplitude_q8 = target;
        oscillator.ramp_remaining = AMPLITUDE_RAMP_SAMPLES;
    }
}

void SwitchHapticsSynth::set_state(const SwitchRumbleState &state) {
    set_band(left_low_, state.left.low);
    set_band(left_high_, state.left.high);
    set_band(right_low_, state.right.low);
    set_band(right_high_, state.right.high);
}

int32_t SwitchHapticsSynth::render_oscillator(Oscillator &oscillator) {
    if (oscillator.ramp_remaining != 0) {
        oscillator.amplitude_q8 +=
            (oscillator.target_amplitude_q8 - oscillator.amplitude_q8) /
            oscillator.ramp_remaining;
        --oscillator.ramp_remaining;
    }
    const int32_t sample = sine_q15(oscillator.phase);
    oscillator.phase += oscillator.increment;
    return sample * oscillator.amplitude_q8;
}

int8_t SwitchHapticsSynth::mix_actuator(Oscillator &low, Oscillator &high,
                                        uint16_t gain_q8) {
    // Shift each product back from Q8 before summing so all per-sample math
    // fits in 32 bits. Normalizing by the combined amplitude keeps two strong
    // simultaneous bands from hard-clipping each other.
    const int32_t mixed = (render_oscillator(low) >> 8) +
                          (render_oscillator(high) >> 8);
    const int32_t total_amplitude =
        (low.amplitude_q8 + high.amplitude_q8 + 128) >> 8;
    const int32_t normalizer = std::max<int32_t>(255, total_amplitude);
    int32_t sample_q15 = mixed / normalizer;
    sample_q15 = (sample_q15 * gain_q8) >> 8;
    const int32_t sample_u8 = (sample_q15 * 127) / 32767;
    return static_cast<int8_t>(std::clamp<int32_t>(sample_u8, -127, 127));
}

void SwitchHapticsSynth::render(int8_t *samples, size_t frames, uint16_t gain_q8) {
    if (samples == nullptr) return;
    for (size_t frame = 0; frame < frames; ++frame) {
        samples[frame * 2] = mix_actuator(left_low_, left_high_, gain_q8);
        samples[frame * 2 + 1] = mix_actuator(right_low_, right_high_, gain_q8);
    }
}

bool SwitchHapticsSynth::silent() const {
    const auto oscillator_silent = [](const Oscillator &oscillator) {
        return oscillator.amplitude_q8 == 0 && oscillator.target_amplitude_q8 == 0;
    };
    return oscillator_silent(left_low_) && oscillator_silent(left_high_) &&
           oscillator_silent(right_low_) && oscillator_silent(right_high_);
}
