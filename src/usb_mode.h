/*
 * SPDX-License-Identifier: MIT
 */

#ifndef DS5_BRIDGE_USB_MODE_H
#define DS5_BRIDGE_USB_MODE_H

enum class UsbOutputMode : unsigned char {
    DualSense = 0,
    SwitchPro = 1,
};

// Must be called after config_load() and before the controller causes USB to
// reconnect. The saved mode is restored; runtime BOOTSEL double-click toggles it.
void usb_mode_init();
void usb_mode_task();
bool usb_mode_is_switch();
void usb_mode_apply_config();
void usb_mode_toggle_and_reconnect();
const char *usb_mode_name();

#endif // DS5_BRIDGE_USB_MODE_H
