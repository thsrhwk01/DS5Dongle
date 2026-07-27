/*
 * SPDX-License-Identifier: MIT
 */

#ifndef DS5_BRIDGE_SWITCH_PRO_USB_H
#define DS5_BRIDGE_SWITCH_PRO_USB_H

#include <cstdint>

#include "controller_state.h"
#include "tusb.h"

void switch_pro_usb_init();
void switch_pro_usb_reset();
void switch_pro_usb_update(const ControllerState &state);
void switch_pro_usb_task();
uint16_t switch_pro_usb_get_report(uint8_t report_id, hid_report_type_t report_type,
                                   uint8_t *buffer, uint16_t reqlen);
void switch_pro_usb_set_report(uint8_t report_id, hid_report_type_t report_type,
                               const uint8_t *buffer, uint16_t bufsize);

const uint8_t *switch_pro_device_descriptor();
const uint8_t *switch_pro_configuration_descriptor();
const uint8_t *switch_pro_report_descriptor();

#endif // DS5_BRIDGE_SWITCH_PRO_USB_H
