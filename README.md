# Pico2W DualSense / Switch Pro Bridge

[中文](./README.CN.md)

> Bridge a Bluetooth DualSense controller to a USB DualSense or Nintendo Switch Pro Controller profile.

***This repository only implements the core functionality of DS5Dongle — making a wireless controller appear as a wired
connection. For additional features, please refer to [Community Fork](#Community-Fork)***

## Overview

This project enables the Raspberry Pi Pico2W (or another compatible board, e.g. the Waveshare RP2350B-Plus-W) to bridge a Bluetooth DualSense or DualSense Edge controller to either a USB DualSense device or a USB Nintendo Switch Pro Controller profile.

## Features

- 🎮 Full DualSense connectivity via Pico2W (or other compatible board)
- 🔊 Supports HD haptics (advanced vibration feedback)
- 🎧 Headset audio output — controller speaker and 3.5 mm jack
- 🎤 Headset microphone input — the controller mic is exposed as a USB audio input device
- 📡 Wireless Bluetooth bridging
- 🎮 Runtime-selectable DualSense and Nintendo Switch Pro USB profiles
- 🔘 BOOTSEL-button management — pair, change USB profile, enter BOOTSEL for flashing, or forget pairings without unplugging
- ⚡ Runs at the stock 150 MHz clock — no overclock required

## Getting Started

### Get the firmware

You have two options:

- **Download a pre-built `.uf2`** — grab the newest
  [Releases](../../releases) build (`ds5-bridge-<version>.uf2`; other board
  builds are bundled in `other board.zip`; `config_tool.py` is attached there
  too). No tools needed.
- **Build it yourself** — see [Build Instructions](#build-instructions)
  below (Windows users get a one-command script).

### Flashing Firmware

1. Hold the BOOTSEL button on the Pico2W
2. Connect the Pico2W to your computer via USB
3. The device will mount as a USB storage device
4. Drag and drop the .uf2 firmware file onto the device

> The firmware also supports a **reboot-to-BOOTSEL** command: the **Reboot to Bootloader** button in the
> [web config](#configuration) reboots the dongle into BOOTSEL mode without holding the physical button.

### Pairing the Controller

1. Put the DualSense controller into Bluetooth pairing mode
2. Wait for the Pico2W to detect and connect
3. Once connected, the device will appear on the host system

***You may need to replug the Pico when the controller is in pairing mode.***

### USB output modes

The default profile is **DualSense** for Windows. While the firmware is running,
double-click BOOTSEL to switch to **Nintendo Switch Pro Controller** output; double-click
again to return to DualSense. The selected profile is saved in flash and restored on
the next boot. USB disconnects and reconnects automatically when the profile changes.

Do not hold BOOTSEL while plugging the Pico in to select a profile: the RP2350 boot ROM
uses that gesture for UF2 flashing, so the application firmware does not run. Profile
selection is deliberately a runtime double click.

For Switch 2, enable **Nintendo Switch Pro Controller Wired Communication** in the
console settings. The Switch profile supports buttons, D-pad, both sticks, digital
ZL/ZR, Plus/Minus, Home, Capture, pairing/reconnection, USB initialization, motion
sensors, and rumble. DualSense gyro/accelerometer samples are transformed into the
Switch Pro coordinate system and native units, with the three most recent samples sent
in each full input report. Switch HD Rumble low/high frequency and amplitude commands
are decoded per actuator and synthesized as a phase-continuous stereo PCM stream for
the DualSense haptic actuators. Short amplitude ramps suppress clicks while preserving
left/right spatial effects. The two controllers have different actuator mechanics, so
the result is a frequency-faithful translation rather than physically identical output.
A 500 ms command watchdog ramps to silence if the USB host disappears while rumble is
active. NFC/amiibo is out of scope.

### BOOTSEL button: pair, change USB mode, or clear controllers

While the firmware is running, the Pico's **BOOTSEL button** doubles as a
controller and USB-profile control — no unplugging or re-flashing needed:

- **Short press (click):**
  - If a controller is connected, the current one is disconnected (its pairing is
    kept, so it can reconnect later). Use this to free the dongle for a different
    already-paired controller.
  - If nothing is connected, a 30-second scan starts to pair a new controller.
    Put the DualSense into pairing mode (hold **PS + Create/Share** until the
    light bar flashes) while the scan runs.
- **Double click:** switch between **DualSense** and **Switch Pro** USB output,
  persist the selection, and reconnect USB. (Clicks register after a brief pause,
  to allow for a second/third click.)
- **Triple click:** **Reboot into BOOTSEL** — the dongle re-enumerates as a USB
  mass-storage drive so you can drag on a new `.uf2`, without holding BOOTSEL while
  plugging in.
- **Long press (~1.5 s):** Disconnect and **forget every paired controller** — all
  stored pairings are deleted and blacklisted so they won't silently auto-reconnect,
  even across a power cycle. The onboard LED flashes six times to confirm. To use a
  forgotten controller again, put it back into **PS + Create/Share** pairing mode.

> Triple click is a software path into the bootloader; you can also still enter it
> the hardware way by holding BOOTSEL **while plugging in** the Pico (see
> [Flashing Firmware](#flashing-firmware) above). All of these act on
> click / double / triple / long-press **while the firmware is already running**.

## Configuration

You can modify the Pico settings via the web config.

- For release: https://ds5.awalol.eu.org
- For development: https://ds5-dev.awalol.eu.org

## Community Fork

### Audio Auto Haptics fork [loteran/DS5Dongle](https://github.com/loteran/DS5Dongle)

> Adds real-time haptic feedback generated from game audio.
> The Pico listens to the sound stream and converts bass and impact sounds into DualSense rumble — no game-side haptic
> support needed.

### DS5_Bridge [SundayMoments/DS5_Bridge](https://github.com/SundayMoments/DS5_Bridge)

> More customization features, such as adjusting audio, haptics, trigger strength, lighting, button remapping, and
> shortcuts.

### OLED Edition [MarcelineVPQ/DS5Dongle-OLED-Edition](https://github.com/MarcelineVPQ/DS5Dongle-OLED-Edition)

> OLED Edition is a fork of awalol/DS5Dongle (upstream) that adds an optional Pico-OLED-1.3 128×64 display add-on with
> 11 screens (status, 4-slot multi-controller pairing, lightbar color picker with favorites and effect presets, trigger
> test, gyro tilt, touchpad, diagnostics, CPU/clock, BT signal strength, audio VU meters, and a persistent settings menu),
> plus a DS5 button-combo soft-reboot.

### [artzox/DS5Dongle](https://github.com/artzox/DS5Dongle)

> Building on Awalol's and Loteran's work to expand simulated haptics and add new features like 
> native haptics anti-aliasing, audio-leak, trigger vibrations, trigger resistance, gyro-aiming, automated profile loading.
> [Detail Introduction](https://github.com/awalol/DS5Dongle/issues/221)

### DS4Dongle [snipem/DS4Dongle](https://github.com/snipem/DS4Dongle)

> Firmware for the Raspberry Pi Pico 2 W that works for DualShock 4. Inspired by DS5Dongle with headset and microphone support and configuration abilities.

### [zurce/DS5Dongle-OLED](https://github.com/zurce/DS5Dongle-OLED)

## Notes

The Pico device will only be visible to the system after the controller is connected

Some behaviors depend on reconnection cycles to take effect

### Microphone

The controller microphone is exposed as a USB audio input — "Headset Microphone"
on Windows. After selecting it as your recording device, raise its input/capture
level in your OS: Windows in particular often defaults it to 0 (or very low),
which makes the mic seem dead even though it is working.

### Low-battery LED indicator

When the connected DualSense reports its battery at or below 10% (and it is not charging), the Pico onboard LED switches
from solid-on to a 1 Hz blink so you can see the warning at a glance. The LED returns to solid-on as soon as the
controller is plugged in or its reported level rises again. The blink also fires when `disable_pico_led` is set — the
warning is treated as critical and overrides the LED-off preference; the LED returns to its disabled (off) state once
the battery recovers or the controller starts charging.

To opt out at build time, configure with `-DENABLE_BATT_LED=OFF`. Default is ON.

### Pico W Version

Pico W only has haptics support, no speaker. You can enable Pico W firmware compilation with `-DPICO_W_BUILD=ON`, or
download precompiled firmware from GitHub Actions.

### Waveshare RP2350B-Plus-W

The [Waveshare RP2350B-Plus-W](https://www.waveshare.com/wiki/RP2350B-Plus-W) is an RP2350B-based board with the RM2 wireless module (same CYW43 silicon as the Pico 2 W), 16 MB QSPI flash, and a USB-C connector. Build with:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DPICO_SDK_PATH=<sdk> -DWAVESHARE_RP2350B_PLUS_W_BUILD=ON
cmake --build build --target ds5-bridge
```

Or download precompiled firmware from GitHub Actions.

### USB Wake Feature

Wake-on-PS is now built into the standard firmware — there is no separate `feat/usb-wake` branch or `ds5-bridge-wake.uf2`
build. It is **disabled by default**; turn it on with the **Wake PC from sleep on PS button** toggle in the
[web config](#configuration). When enabled, the dongle presents a HID keyboard interface and advertises USB remote
wakeup so a controller button can wake the PC; when disabled, that interface is not enumerated. See
[Wake-on-PS](#wake-on-ps-optional) for setup.

It is recommended to read #60 and #61 before using this feature.

## Known Issues

- ⚠️ Audio may experience slight stuttering

## Performance

The audio path — libopus encode/decode, the resampler, and the Bluetooth/USB
packet handling on the hot path — executes from **RAM** instead of flash. This
removes flash-fetch (XIP cache miss) stalls from the time-critical audio loop,
which previously forced the RP2350 to be overclocked just to keep up with audio
encoding.

As a result, the firmware runs the **full audio path (haptics, speaker, 3.5 mm
output, and microphone) at the stock 150 MHz clock — no overclock and no
core-voltage bump.**

> Earlier releases required 320 MHz @ 1.2 V; overclocking is no longer needed.
> If you build for a different board and it fails to boot, reduce the CPU
> frequency (and/or raise the voltage) in `CMakeLists.txt`.

## Build Instructions

### Windows 11 (one command, no WSL)

You don't even need to clone this repo. Download just
[`tools/build-windows.ps1`](tools/build-windows.ps1) to any folder and run
it in **PowerShell**:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
```

(If you already have a checkout, run `tools\build-windows.ps1` from the
repo root instead — it detects and uses your local checkout.)

The script installs every prerequisite (CMake, Ninja, Python, Git and the
ARM GNU toolchain — via `winget`, falling back to portable downloads if
`winget` is unavailable), clones the project (if not run from a checkout)
plus the pinned Pico SDK + TinyUSB into `%USERPROFILE%\.ds5-build`, builds
the firmware, and drops `ds5-bridge.uf2` next to the script and on your
Desktop. It is safe to re-run; already-installed tools are skipped.

Build a fork or a specific ref with `-Repo <url>` / `-Ref <branch|tag>`.

Build a variant with `-Variant debug`.

### Other platforms

To build from source manually:

1. Install the Pico SDK 2.3.0 and switch its TinyUSB submodule to tag 0.21.0
   i.e. ***Update TinyUSB in the Pico SDK to the latest version***
2. Initialise this repo's submodules: `git submodule update --init --recursive`
3. Configure and build with the standard Pico SDK toolchain:
   `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPICO_SDK_PATH=<sdk>`
   then `cmake --build build --target ds5-bridge`

1. ***Update TinyUSB in the Pico SDK to the latest version***
2. Compile using standard Pico SDK toolchain

On macOS, `tools/build-macos.sh` can prepare a repo-local Pico SDK checkout, prompt to install missing Homebrew build
tools, initialize submodules, pin TinyUSB, and build the firmware:

```sh
tools/build-macos.sh
```

Use `tools/build-macos.sh --clean` to rebuild from scratch, or
`--sdk-dir <path>` to use an existing SDK checkout. When using `--sdk-dir`, the script asks before checking that SDK out
to the required Pico SDK and TinyUSB versions. If Homebrew's `arm-none-eabi-gcc` formula is installed without standard C
headers, the script asks to install the complete `gcc-arm-embedded` cask and points CMake at that toolchain.

## Xbox Game Bar (optional)

The **PS button = Xbox Game Bar** toggle in the [web config](#configuration) maps the controller's PS button to
keyboard shortcuts, sent over the same HID keyboard interface used by [Wake-on-PS](#wake-on-ps-optional):

- **Short press** (tap and release) → `Win`+`G`, which opens the **Xbox Game Bar** overlay.
- **Long press** (hold ≥ 750 ms) → `Win`+`Tab`, which opens **Task View**.

The toggle is off by default, and the keyboard interface is only enumerated while it (or wake) is enabled. 
> If the Game Bar overlay opens but does not respond to controller inputs, Windows may be missing the modern input stack. Installing or updating **Microsoft GameInput** will resolve this and restore controller navigation. You can install the service directly by opening an elevated command prompt and running `winget install Microsoft.GameInput`, or read the [official documentation](https://learn.microsoft.com/en-us/gaming/gdk/docs/features/common/input/overviews/input-overview) for more details.

## Wake-on-PS (optional)

Enabling the **Wake PC from sleep on PS button** toggle in the [web config](#configuration) makes the dongle present a
second HID interface (a boot keyboard) and advertise USB remote wakeup. A controller button press while the host is
suspended then injects an **F15** keypress, waking the PC from **S3 sleep**. F15 was chosen because it has no default
Windows or app binding — a stray fire never inserts characters or triggers shortcuts. The toggle is off by default, and
the keyboard interface is only enumerated while it (or the Xbox Game Bar shortcut) is enabled.

Scope: **S3 only.** Modern Standby (S0ix) is not supported. To check your machine, run `powercfg /a` — you need
"Standby (S3)" listed under available sleep states.

After enabling the toggle (then **Reconnect USB** so the interface re-enumerates):

1. Open Device Manager → the new **HID Keyboard Device** (and its parent **USB Composite Device**) → Properties → Power
   Management → tick **"Allow this device to wake the computer."**
2. Verify with `powercfg /devicequery wake_armed`.
3. Sleep the PC; press any button on the controller; the PC should wake within ~1 s.
4. After a wake, `powercfg /lastwake` should attribute the wake to the HID Keyboard Device.

> Wake also needs `SelectiveSuspendEnabled = 1` (a `REG_DWORD`) on the controller's audio interface (`MI_00`). Windows
> only writes it at first install, so a runtime toggle may need it set manually. It lives under each per-instance
> `Device Parameters` key:
>
> ```
> HKLM\SYSTEM\CurrentControlSet\Enum\USB\VID_054C&PID_0CE6&MI_00\<instance>\Device Parameters
>     SelectiveSuspendEnabled    (REG_DWORD) = 1
> ```
>
> `PID_0CE6` is the DualSense (`PID_0DF2` for the Edge), and `<instance>` is device/port-specific (e.g.
> `6&212078ea&1&0000`), so there can be more than one node — set it on every one. An elevated PowerShell one-liner that
> covers all present instances:
>
> ```powershell
> Get-ChildItem 'HKLM:\SYSTEM\CurrentControlSet\Enum\USB\VID_054C&PID_0CE6&MI_00' | ForEach-Object {
>   New-ItemProperty "$($_.PSPath)\Device Parameters" SelectiveSuspendEnabled -Value 1 -PropertyType DWord -Force }
> ```
>
> Then Reconnect USB or reboot. (Re-installing the device — clearing its Windows device cache and replugging — also
> makes Windows write the value itself.)

## Roadmap

- Please check out [DS5Dongle plan](https://github.com/users/awalol/projects/5)

## Community

- Join the Discord server: [Discord Server](https://discord.gg/hM4ntchGCa)
- If you have a bug, please open an issue instead.

## References

- [rafaelvaloto/Pico_W-Dualsense](https://github.com/rafaelvaloto/Pico_W-Dualsense) — Project inspiration
- [egormanga/SAxense](https://apps.sdore.me/SAxense) — Bluetooth Haptics POC
- [https://controllers.fandom.com/wiki/Sony_DualSense](https://controllers.fandom.com/wiki/Sony_DualSense) - DualSense
  data report structure documentation
- [Paliverse/DualSenseX](https://github.com/Paliverse/DualSenseX) — Speaker report packet
- [Nielk1’s research report and packet samples](https://github.com/egormanga/SAxense/issues/1)
