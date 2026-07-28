/*
 * SPDX-License-Identifier: MIT
 */

#ifndef DS5_BRIDGE_SWITCH_RUMBLE_ADAPTER_H
#define DS5_BRIDGE_SWITCH_RUMBLE_ADAPTER_H

#include "switch_rumble.h"

void switch_rumble_adapter_apply(const SwitchRumbleState &rumble);
void switch_rumble_adapter_stop();
void switch_rumble_adapter_task();

#endif // DS5_BRIDGE_SWITCH_RUMBLE_ADAPTER_H
