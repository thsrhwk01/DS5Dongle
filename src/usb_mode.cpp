/*
 * SPDX-License-Identifier: MIT
 */

#include "usb_mode.h"

#include <cstdio>

#include "config.h"
#include "pico/time.h"
#include "switch_pro_usb.h"
#include "tusb.h"
#include "wake.h"

namespace {

bool save_pending = false;
uint32_t save_after_ms = 0;
UsbOutputMode active_mode = UsbOutputMode::DualSense;

void select_mode(UsbOutputMode mode) {
    active_mode = mode;
    get_config().usb_output_mode = static_cast<uint8_t>(mode);
}

} // namespace

bool usb_mode_is_switch() {
    return active_mode == UsbOutputMode::SwitchPro;
}

const char *usb_mode_name() {
    return usb_mode_is_switch() ? "Switch Pro" : "DualSense";
}

void usb_mode_init() {
    active_mode = get_config().usb_output_mode == static_cast<uint8_t>(UsbOutputMode::SwitchPro)
                      ? UsbOutputMode::SwitchPro
                      : UsbOutputMode::DualSense;
    switch_pro_usb_init();
    printf("[USB] Output mode: %s\n", usb_mode_name());
}

void usb_mode_apply_config() {
    active_mode = get_config().usb_output_mode == static_cast<uint8_t>(UsbOutputMode::SwitchPro)
                      ? UsbOutputMode::SwitchPro
                      : UsbOutputMode::DualSense;
    switch_pro_usb_reset();
}

void usb_mode_task() {
    if (!save_pending) return;
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (static_cast<int32_t>(now - save_after_ms) < 0) return;
    if (config_save()) save_pending = false;
    else save_after_ms = now + 2000;
}

void usb_mode_toggle_and_reconnect() {
    const UsbOutputMode next = usb_mode_is_switch()
                                   ? UsbOutputMode::DualSense
                                   : UsbOutputMode::SwitchPro;
    wake_note_usb_reconnect();
    tud_disconnect();
    select_mode(next);
    switch_pro_usb_reset();
    save_pending = !config_save();
    if (save_pending) save_after_ms = to_ms_since_boot(get_absolute_time()) + 2000;

    printf("[USB] Switching output mode to %s\n", usb_mode_name());
    sleep_ms(150);
    tud_connect();
}
