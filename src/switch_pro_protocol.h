/*
 * Nintendo Switch Pro USB protocol state machine.
 *
 * Portions are based on GP2040-CE's Switch Pro driver.
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef DS5_BRIDGE_SWITCH_PRO_PROTOCOL_H
#define DS5_BRIDGE_SWITCH_PRO_PROTOCOL_H

#include <cstddef>
#include <cstdint>

#include "controller_state.h"
#include "switch_rumble.h"

constexpr size_t SWITCH_PRO_REPORT_SIZE = 64;

class SwitchProProtocol {
public:
    SwitchProProtocol();

    void reset();
    void set_mac_address(const uint8_t mac[6]);
    void update_controller(const ControllerState &state);

    // report_id is non-zero for a control SET_REPORT, while TinyUSB supplies
    // report_id == 0 and keeps the ID in buffer[0] for interrupt OUT reports.
    void handle_output_report(uint8_t report_id, const uint8_t *buffer, size_t len);

    // Produces a complete 64-byte packet, including its report ID in byte 0.
    bool next_report(uint32_t now_ms, uint8_t out[SWITCH_PRO_REPORT_SIZE]);
    bool take_rumble(SwitchRumbleState &out);
    bool ready() const { return ready_; }

private:
    void build_identify(uint8_t out[SWITCH_PRO_REPORT_SIZE]);
    void build_input(uint8_t out[SWITCH_PRO_REPORT_SIZE]);
    void fill_input(uint8_t out[11]) const;
    static void fill_imu_sample(uint8_t out[12], const ControllerState &state);
    void handle_configuration(const uint8_t *packet, size_t len);
    void handle_subcommand(const uint8_t *packet, size_t len);
    static void read_spi(uint8_t *dest, uint32_t address, uint8_t size);

    ControllerState state_{};
    ControllerState imu_history_[3]{};
    size_t imu_history_count_ = 0;
    SwitchRumbleState rumble_{};
    uint8_t mac_[6]{0x02, 0x50, 0x69, 0x63, 0x6f, 0x01};
    uint8_t response_[SWITCH_PRO_REPORT_SIZE]{};
    uint8_t packet_counter_ = 0;
    uint8_t player_lights_ = 0;
    uint8_t input_mode_ = 0x30;
    uint32_t last_input_ms_ = 0;
    bool initialized_ = false;
    bool ready_ = false;
    bool response_queued_ = false;
    bool input_dirty_ = true;
    bool imu_enabled_ = false;
    bool vibration_enabled_ = false;
    bool rumble_pending_ = false;
};

#endif // DS5_BRIDGE_SWITCH_PRO_PROTOCOL_H
