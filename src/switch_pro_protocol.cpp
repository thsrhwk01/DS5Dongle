/*
 * Nintendo Switch Pro USB protocol state machine.
 *
 * Portions are based on GP2040-CE's Switch Pro driver.
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "switch_pro_protocol.h"

#include <algorithm>
#include <cstring>

namespace {

enum : uint8_t {
    REPORT_FEATURE = 0x01,
    REPORT_SUBCOMMAND_REPLY = 0x21,
    REPORT_FULL_INPUT = 0x30,
    REPORT_CONFIGURATION = 0x80,
    REPORT_USB_REPLY = 0x81,
};

enum : uint8_t {
    USB_IDENTIFY = 0x01,
    USB_HANDSHAKE = 0x02,
    USB_BAUD_RATE = 0x03,
    USB_DISABLE_TIMEOUT = 0x04,
    USB_ENABLE_TIMEOUT = 0x05,
};

enum : uint8_t {
    CMD_GET_CONTROLLER_STATE = 0x00,
    CMD_PAIR = 0x01,
    CMD_DEVICE_INFO = 0x02,
    CMD_SET_MODE = 0x03,
    CMD_TRIGGER_BUTTONS = 0x04,
    CMD_SET_SHIPMENT = 0x08,
    CMD_SPI_READ = 0x10,
    CMD_SET_NFC_IR_CONFIG = 0x21,
    CMD_SET_NFC_IR_STATE = 0x22,
    CMD_SET_PLAYER_LIGHTS = 0x30,
    CMD_GET_PLAYER_LIGHTS = 0x31,
    CMD_UNKNOWN_33 = 0x33,
    CMD_SET_HOME_LIGHT = 0x38,
    CMD_TOGGLE_IMU = 0x40,
    CMD_IMU_SENSITIVITY = 0x41,
    CMD_READ_IMU = 0x43,
    CMD_ENABLE_VIBRATION = 0x48,
    CMD_GET_VOLTAGE = 0x50,
};

constexpr uint8_t PRO_CONTROLLER_TYPE = 0x03;
constexpr uint32_t INPUT_INTERVAL_MS = 8;
constexpr uint16_t STICK_MIN = 0x015c;
constexpr uint16_t STICK_MAX = 0x0ea4;
constexpr int32_t DUALSENSE_ACCEL_COUNTS_PER_G = 8192;
constexpr int32_t SWITCH_ACCEL_COUNTS_PER_G = 4096;
// DualSense raw gyro is nominally 16 counts/(degree/s). SDL's current Switch
// driver uses 14.2842 counts/(degree/s), consistent with the emulated factory
// calibration coefficient of 13371 below.
constexpr int32_t DUALSENSE_GYRO_COUNTS_PER_1000_DPS = 16000;
constexpr int32_t SWITCH_GYRO_COUNTS_PER_1000_DPS = 14284;

// Prefix of the emulated factory SPI bank. It contains device identity, motion
// calibration, both stick calibrations and controller colors. Unlisted bytes
// read as erased flash (0xff).
constexpr uint8_t FACTORY_SPI_PREFIX[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, PRO_CONTROLLER_TYPE, 0xa0,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x02,
    0xff, 0xff, 0xff, 0xff,
    // Neutral offsets and nominal Switch Pro motion sensitivity. Publishing
    // offsets captured from an unrelated physical controller would inject a
    // constant bias into the converted DualSense sensor stream.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
    0x00, 0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x3b, 0x34, 0x3b, 0x34, 0x3b, 0x34,
    0xff, 0xff, 0xff, 0xff, 0xff,
    // Left stick: max-above-center, center, min-below-center.
    0xa4, 0x46, 0x6a, 0x00, 0x08, 0x80, 0xa4, 0x46, 0x6a,
    // Right stick: center, min-below-center, max-above-center.
    0x00, 0x08, 0x80, 0xa4, 0x46, 0x6a, 0xa4, 0x46, 0x6a,
    0xff,
    0x1b, 0x1b, 0x1d, 0xff, 0xff, 0xff,
    0xec, 0x00, 0x8c, 0xec, 0x00, 0x8c, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x50, 0xfd, 0x00, 0x00, 0xc6,
    0x0f, 0x0f, 0x30, 0x61, 0xae, 0x90, 0xd9, 0xd4,
    0x14, 0x54, 0x41, 0x15, 0x54, 0xc7, 0x79, 0x9c,
    0x33, 0x36, 0x63, 0x0f, 0x30, 0x61, 0xae, 0x90,
    0xd9, 0xd4, 0x14, 0x54, 0x41, 0x15, 0x54, 0xc7,
    0x79, 0x9c, 0x33, 0x36, 0x63,
};

constexpr uint8_t USER_SPI_PREFIX[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xb2, 0xa1, 0xa4, 0x46, 0x6a, 0x00, 0x08, 0x80,
    0xa4, 0x46, 0x6a,
    0xb2, 0xa1, 0x00, 0x08, 0x80, 0xa4, 0x46, 0x6a,
    0xa4, 0x46, 0x6a,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

uint16_t axis_to_12(uint16_t value) {
    return std::clamp<uint16_t>(value >> 4, STICK_MIN, STICK_MAX);
}

void pack_stick(uint8_t out[3], uint16_t x, uint16_t y) {
    out[0] = x & 0xff;
    out[1] = static_cast<uint8_t>(((x >> 8) & 0x0f) | ((y & 0x0f) << 4));
    out[2] = static_cast<uint8_t>((y >> 4) & 0xff);
}

uint8_t switch_battery_level(uint8_t dualsense_level) {
    if (dualsense_level <= 1) return 0;
    if (dualsense_level <= 3) return 2;
    if (dualsense_level <= 5) return 4;
    if (dualsense_level <= 8) return 6;
    return 8;
}

int16_t scale_signed(int32_t value, int32_t numerator, int32_t denominator) {
    int32_t scaled = value * numerator;
    scaled += scaled >= 0 ? denominator / 2 : -denominator / 2;
    scaled /= denominator;
    return static_cast<int16_t>(std::clamp<int32_t>(scaled, INT16_MIN, INT16_MAX));
}

void write_s16_le(uint8_t *out, int16_t value) {
    const uint16_t bits = static_cast<uint16_t>(value);
    out[0] = static_cast<uint8_t>(bits);
    out[1] = static_cast<uint8_t>(bits >> 8);
}

} // namespace

SwitchProProtocol::SwitchProProtocol() {
    reset();
}

void SwitchProProtocol::reset() {
    state_ = ControllerState{};
    for (auto &sample : imu_history_) sample = ControllerState{};
    imu_history_count_ = 0;
    rumble_ = {};
    std::memset(response_, 0, sizeof(response_));
    packet_counter_ = 0;
    player_lights_ = 0;
    input_mode_ = REPORT_FULL_INPUT;
    last_input_ms_ = 0;
    initialized_ = false;
    ready_ = false;
    response_queued_ = false;
    input_dirty_ = true;
    imu_enabled_ = false;
    vibration_enabled_ = false;
    rumble_pending_ = false;
}

void SwitchProProtocol::set_mac_address(const uint8_t mac[6]) {
    if (mac != nullptr) std::memcpy(mac_, mac, sizeof(mac_));
}

void SwitchProProtocol::update_controller(const ControllerState &state) {
    state_ = state;
    imu_history_[2] = imu_history_[1];
    imu_history_[1] = imu_history_[0];
    imu_history_[0] = state;
    imu_history_count_ = std::min<size_t>(imu_history_count_ + 1, 3);
    input_dirty_ = true;
}

void SwitchProProtocol::fill_input(uint8_t out[11]) const {
    std::memset(out, 0, 11);
    out[0] = static_cast<uint8_t>(switch_battery_level(state_.battery) << 4);

    // Nintendo's physical face-button positions match Square/Cross/Circle/Triangle
    // to Y/B/A/X respectively.
    if (state_.buttons & CONTROLLER_BUTTON_SQUARE) out[1] |= 1u << 0;   // Y
    if (state_.buttons & CONTROLLER_BUTTON_TRIANGLE) out[1] |= 1u << 1; // X
    if (state_.buttons & CONTROLLER_BUTTON_CROSS) out[1] |= 1u << 2;    // B
    if (state_.buttons & CONTROLLER_BUTTON_CIRCLE) out[1] |= 1u << 3;   // A
    if (state_.buttons & CONTROLLER_BUTTON_R1) out[1] |= 1u << 6;
    if ((state_.buttons & CONTROLLER_BUTTON_R2) || state_.r2 != 0) out[1] |= 1u << 7;

    if (state_.buttons & CONTROLLER_BUTTON_CREATE) out[2] |= 1u << 0;  // Minus
    if (state_.buttons & CONTROLLER_BUTTON_OPTIONS) out[2] |= 1u << 1; // Plus
    if (state_.buttons & CONTROLLER_BUTTON_R3) out[2] |= 1u << 2;
    if (state_.buttons & CONTROLLER_BUTTON_L3) out[2] |= 1u << 3;
    if (state_.buttons & CONTROLLER_BUTTON_HOME) out[2] |= 1u << 4;
    if (state_.buttons & CONTROLLER_BUTTON_PAD) out[2] |= 1u << 5;      // Capture
    out[2] |= 1u << 7; // charging grip / wired connection

    switch (state_.dpad) {
        case CONTROLLER_DPAD_UP: out[3] |= 1u << 1; break;
        case CONTROLLER_DPAD_UP_RIGHT: out[3] |= (1u << 1) | (1u << 2); break;
        case CONTROLLER_DPAD_RIGHT: out[3] |= 1u << 2; break;
        case CONTROLLER_DPAD_DOWN_RIGHT: out[3] |= (1u << 0) | (1u << 2); break;
        case CONTROLLER_DPAD_DOWN: out[3] |= 1u << 0; break;
        case CONTROLLER_DPAD_DOWN_LEFT: out[3] |= (1u << 0) | (1u << 3); break;
        case CONTROLLER_DPAD_LEFT: out[3] |= 1u << 3; break;
        case CONTROLLER_DPAD_UP_LEFT: out[3] |= (1u << 1) | (1u << 3); break;
        default: break;
    }
    if (state_.buttons & CONTROLLER_BUTTON_L1) out[3] |= 1u << 6;
    if ((state_.buttons & CONTROLLER_BUTTON_L2) || state_.l2 != 0) out[3] |= 1u << 7;

    const uint16_t lx = axis_to_12(state_.lx);
    const uint16_t ly = (0x1000 - axis_to_12(state_.ly)) & 0x0fff;
    const uint16_t rx = axis_to_12(state_.rx);
    const uint16_t ry = (0x1000 - axis_to_12(state_.ry)) & 0x0fff;
    pack_stick(out + 4, lx, ly);
    pack_stick(out + 7, rx, ry);
}

void SwitchProProtocol::fill_imu_sample(uint8_t out[12], const ControllerState &state) {
    // SDL's Switch driver converts raw Switch axes to PlayStation's de-facto
    // standard as PS = {-Switch Y, Switch Z, -Switch X}. Apply the inverse
    // here, along with the native sensor-resolution conversion.
    const int16_t accel_x = scale_signed(-static_cast<int32_t>(state.accel[2]),
                                         SWITCH_ACCEL_COUNTS_PER_G,
                                         DUALSENSE_ACCEL_COUNTS_PER_G);
    const int16_t accel_y = scale_signed(-static_cast<int32_t>(state.accel[0]),
                                         SWITCH_ACCEL_COUNTS_PER_G,
                                         DUALSENSE_ACCEL_COUNTS_PER_G);
    const int16_t accel_z = scale_signed(state.accel[1],
                                         SWITCH_ACCEL_COUNTS_PER_G,
                                         DUALSENSE_ACCEL_COUNTS_PER_G);
    const int16_t gyro_x = scale_signed(-static_cast<int32_t>(state.gyro[2]),
                                        SWITCH_GYRO_COUNTS_PER_1000_DPS,
                                        DUALSENSE_GYRO_COUNTS_PER_1000_DPS);
    const int16_t gyro_y = scale_signed(-static_cast<int32_t>(state.gyro[0]),
                                        SWITCH_GYRO_COUNTS_PER_1000_DPS,
                                        DUALSENSE_GYRO_COUNTS_PER_1000_DPS);
    const int16_t gyro_z = scale_signed(state.gyro[1],
                                        SWITCH_GYRO_COUNTS_PER_1000_DPS,
                                        DUALSENSE_GYRO_COUNTS_PER_1000_DPS);

    write_s16_le(out + 0, accel_x);
    write_s16_le(out + 2, accel_y);
    write_s16_le(out + 4, accel_z);
    write_s16_le(out + 6, gyro_x);
    write_s16_le(out + 8, gyro_y);
    write_s16_le(out + 10, gyro_z);
}

void SwitchProProtocol::build_identify(uint8_t out[SWITCH_PRO_REPORT_SIZE]) {
    std::memset(out, 0, SWITCH_PRO_REPORT_SIZE);
    out[0] = REPORT_USB_REPLY;
    out[1] = USB_IDENTIFY;
    out[3] = PRO_CONTROLLER_TYPE;
    for (size_t i = 0; i < sizeof(mac_); ++i) out[4 + i] = mac_[5 - i];
}

void SwitchProProtocol::build_input(uint8_t out[SWITCH_PRO_REPORT_SIZE]) {
    std::memset(out, 0, SWITCH_PRO_REPORT_SIZE);
    out[0] = REPORT_FULL_INPUT;
    out[1] = packet_counter_++;
    fill_input(out + 2);
    out[12] = 0x09;
    if (imu_enabled_ && imu_history_count_ != 0) {
        // Switch reports carry newest-to-oldest samples; consumers such as SDL
        // reverse them when producing chronological sensor events. Repeat the
        // oldest available sample until the three-entry history is primed.
        for (size_t i = 0; i < 3; ++i) {
            const size_t history_index = std::min(i, imu_history_count_ - 1);
            fill_imu_sample(out + 13 + i * 12, imu_history_[history_index]);
        }
    }
}

bool SwitchProProtocol::take_rumble(SwitchRumbleState &out) {
    if (!rumble_pending_) return false;
    out = rumble_;
    rumble_pending_ = false;
    return true;
}

bool SwitchProProtocol::next_report(uint32_t now_ms, uint8_t out[SWITCH_PRO_REPORT_SIZE]) {
    if (out == nullptr) return false;
    if (response_queued_) {
        std::memcpy(out, response_, SWITCH_PRO_REPORT_SIZE);
        response_queued_ = false;
        return true;
    }
    if (!initialized_) {
        build_identify(out);
        initialized_ = true;
        return true;
    }
    if (!ready_) return false;
    if (!input_dirty_ && now_ms - last_input_ms_ < INPUT_INTERVAL_MS) return false;
    build_input(out);
    input_dirty_ = false;
    last_input_ms_ = now_ms;
    return true;
}

void SwitchProProtocol::handle_output_report(uint8_t report_id, const uint8_t *buffer, size_t len) {
    if (buffer == nullptr) return;
    uint8_t packet[SWITCH_PRO_REPORT_SIZE]{};
    size_t packet_len = 0;
    if (report_id != 0) {
        packet[0] = report_id;
        packet_len = std::min(len, SWITCH_PRO_REPORT_SIZE - 1);
        std::memcpy(packet + 1, buffer, packet_len);
        ++packet_len;
    } else {
        packet_len = std::min(len, SWITCH_PRO_REPORT_SIZE);
        std::memcpy(packet, buffer, packet_len);
    }
    if (packet_len < 2) return;

    if (packet[0] == REPORT_CONFIGURATION) {
        handle_configuration(packet, packet_len);
    } else if (packet[0] == REPORT_FEATURE) {
        handle_subcommand(packet, packet_len);
    }

    // Both the rumble-only report and every subcommand report carry two
    // four-byte linear-actuator commands after the packet counter.
    if ((packet[0] == REPORT_FEATURE || packet[0] == 0x10) && packet_len >= 10) {
        rumble_ = vibration_enabled_ ? switch_decode_hd_rumble(packet + 2)
                                     : SwitchRumbleState{};
        rumble_pending_ = true;
    }
}

void SwitchProProtocol::handle_configuration(const uint8_t *packet, size_t len) {
    if (len < 2) return;
    const uint8_t subtype = packet[1];
    std::memset(response_, 0, sizeof(response_));
    switch (subtype) {
        case USB_IDENTIFY:
            build_identify(response_);
            break;
        case USB_HANDSHAKE:
        case USB_BAUD_RATE:
            response_[0] = REPORT_USB_REPLY;
            response_[1] = subtype;
            break;
        case USB_DISABLE_TIMEOUT:
            response_[0] = REPORT_FULL_INPUT;
            response_[1] = subtype;
            ready_ = true;
            break;
        case USB_ENABLE_TIMEOUT:
        default:
            response_[0] = REPORT_FULL_INPUT;
            response_[1] = subtype;
            break;
    }
    response_queued_ = true;
}

void SwitchProProtocol::handle_subcommand(const uint8_t *packet, size_t len) {
    if (len < 11) return;
    const uint8_t command = packet[10];
    std::memset(response_, 0, sizeof(response_));
    response_[0] = REPORT_SUBCOMMAND_REPLY;
    response_[1] = packet_counter_++;
    fill_input(response_ + 2);
    response_[13] = 0x80;
    response_[14] = command;

    switch (command) {
        case CMD_GET_CONTROLLER_STATE:
            response_[15] = 0x03;
            break;
        case CMD_PAIR:
            response_[13] = 0x81;
            response_[15] = 0x03;
            break;
        case CMD_DEVICE_INFO:
            response_[13] = 0x82;
            response_[15] = 0x04; // firmware major
            response_[16] = 0x91; // firmware minor
            response_[17] = PRO_CONTROLLER_TYPE;
            response_[18] = 0x02;
            std::memcpy(response_ + 19, mac_, sizeof(mac_));
            response_[25] = 0x01;
            response_[26] = 0x02;
            break;
        case CMD_SET_MODE:
            if (len >= 12) input_mode_ = packet[11];
            response_[15] = input_mode_;
            break;
        case CMD_TRIGGER_BUTTONS:
            response_[13] = 0x83;
            break;
        case CMD_SET_SHIPMENT:
        case CMD_SET_NFC_IR_CONFIG:
        case CMD_SET_NFC_IR_STATE:
        case CMD_UNKNOWN_33:
        case CMD_SET_HOME_LIGHT:
        case CMD_IMU_SENSITIVITY:
            break;
        case CMD_SPI_READ:
            if (len >= 16) {
                const uint32_t address = static_cast<uint32_t>(packet[11]) |
                                         (static_cast<uint32_t>(packet[12]) << 8) |
                                         (static_cast<uint32_t>(packet[13]) << 16) |
                                         (static_cast<uint32_t>(packet[14]) << 24);
                const uint8_t size = std::min<uint8_t>(packet[15], 44);
                response_[13] = 0x90;
                std::memcpy(response_ + 15, packet + 11, 5);
                read_spi(response_ + 20, address, size);
            }
            break;
        case CMD_SET_PLAYER_LIGHTS:
            if (len >= 12) player_lights_ = packet[11];
            break;
        case CMD_GET_PLAYER_LIGHTS:
            response_[13] = 0xb0;
            response_[15] = player_lights_;
            break;
        case CMD_TOGGLE_IMU:
            if (len >= 12) imu_enabled_ = packet[11] != 0;
            break;
        case CMD_ENABLE_VIBRATION:
            if (len >= 12) vibration_enabled_ = packet[11] != 0;
            break;
        case CMD_READ_IMU:
            response_[13] = 0xc0;
            if (len >= 13) {
                response_[15] = packet[11];
                response_[16] = packet[12];
            }
            break;
        case CMD_GET_VOLTAGE:
            response_[13] = 0xd0;
            response_[15] = 0x83;
            response_[16] = 0x06;
            break;
        default:
            response_[15] = 0x03;
            break;
    }
    response_queued_ = true;
}

void SwitchProProtocol::read_spi(uint8_t *dest, uint32_t address, uint8_t size) {
    std::memset(dest, 0xff, size);
    const uint8_t *source = nullptr;
    size_t source_size = 0;
    uint32_t base = 0;
    if (address >= 0x6000 && address < 0x7000) {
        source = FACTORY_SPI_PREFIX;
        source_size = sizeof(FACTORY_SPI_PREFIX);
        base = 0x6000;
    } else if (address >= 0x8000 && address < 0x8100) {
        source = USER_SPI_PREFIX;
        source_size = sizeof(USER_SPI_PREFIX);
        base = 0x8000;
    }
    if (source == nullptr) return;

    const size_t offset = address - base;
    if (offset >= source_size) return;
    const size_t copy_len = std::min<size_t>(size, source_size - offset);
    std::memcpy(dest, source + offset, copy_len);
}
