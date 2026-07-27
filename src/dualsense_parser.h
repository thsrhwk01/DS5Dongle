/*
 * SPDX-License-Identifier: MIT
 */

#ifndef DS5_BRIDGE_DUALSENSE_PARSER_H
#define DS5_BRIDGE_DUALSENSE_PARSER_H

#include <cstddef>
#include <cstdint>

#include "controller_state.h"

// Parse the 63-byte payload of a DualSense USB-style input report. Bluetooth
// report 0x31 carries this payload after its three-byte HID/sequence header.
bool dualsense_parse_input(const uint8_t *report, size_t len, ControllerState &state);

#endif // DS5_BRIDGE_DUALSENSE_PARSER_H
