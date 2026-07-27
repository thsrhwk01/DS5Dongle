#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "dualsense_parser.h"
#include "switch_pro_protocol.h"
#include "switch_rumble.h"

namespace {

uint16_t unpack_x(const uint8_t *stick) {
    return static_cast<uint16_t>(stick[0]) | ((stick[1] & 0x0f) << 8);
}

uint16_t unpack_y(const uint8_t *stick) {
    return static_cast<uint16_t>(stick[1] >> 4) | (static_cast<uint16_t>(stick[2]) << 4);
}

int16_t read_s16(const uint8_t *data) {
    return static_cast<int16_t>(static_cast<uint16_t>(data[0]) |
                                (static_cast<uint16_t>(data[1]) << 8));
}

void test_dualsense_parser() {
    uint8_t report[63]{};
    report[0] = 0x00;
    report[1] = 0x80;
    report[2] = 0xff;
    report[3] = 0x40;
    report[4] = 0x22;
    report[5] = 0x33;
    report[7] = 0x08 | 0x10 | 0x40; // neutral, Square + Circle
    report[8] = 0x01 | 0x08 | 0x10 | 0x80;
    report[9] = 0x01 | 0x02;
    report[15] = 0x34;
    report[16] = 0x12;
    report[17] = 0x78;
    report[18] = 0x56;
    report[19] = 0xbc;
    report[20] = 0x9a;
    report[21] = 0x11;
    report[22] = 0x11;
    report[23] = 0x22;
    report[24] = 0x22;
    report[25] = 0x33;
    report[26] = 0x33;
    report[52] = 0x15; // charging, battery step 5

    ControllerState state{};
    assert(dualsense_parse_input(report, sizeof(report), state));
    assert(state.lx == 0x0000);
    assert(state.ly == 0x8080);
    assert(state.rx == 0xffff);
    assert(state.ry == 0x4040);
    assert(state.l2 == 0x22 && state.r2 == 0x33);
    assert(state.dpad == CONTROLLER_DPAD_NEUTRAL);
    assert(state.buttons & CONTROLLER_BUTTON_SQUARE);
    assert(state.buttons & CONTROLLER_BUTTON_CIRCLE);
    assert(state.buttons & CONTROLLER_BUTTON_L1);
    assert(state.buttons & CONTROLLER_BUTTON_R2);
    assert(state.buttons & CONTROLLER_BUTTON_CREATE);
    assert(state.buttons & CONTROLLER_BUTTON_R3);
    assert(state.buttons & CONTROLLER_BUTTON_HOME);
    assert(state.buttons & CONTROLLER_BUTTON_PAD);
    assert(state.gyro[0] == 0x1234);
    assert(state.gyro[1] == 0x5678);
    assert(state.gyro[2] == static_cast<int16_t>(0x9abc));
    assert(state.accel[0] == 0x1111);
    assert(state.accel[1] == 0x2222);
    assert(state.accel[2] == 0x3333);
    assert(state.battery == 5 && state.charging);
    assert(!dualsense_parse_input(report, 62, state));
}

void ready_protocol(SwitchProProtocol &protocol) {
    uint8_t out[64]{};
    assert(protocol.next_report(0, out));
    assert(out[0] == 0x81 && out[1] == 0x01 && out[3] == 0x03);

    // Exercise control SET_REPORT normalization: report ID is separate.
    const uint8_t handshake[] = {0x02};
    protocol.handle_output_report(0x80, handshake, sizeof(handshake));
    assert(protocol.next_report(1, out));
    assert(out[0] == 0x81 && out[1] == 0x02);

    const uint8_t disable_timeout[] = {0x80, 0x04};
    protocol.handle_output_report(0, disable_timeout, sizeof(disable_timeout));
    assert(protocol.next_report(2, out));
    assert(out[0] == 0x30 && out[1] == 0x04);
    assert(protocol.ready());
}

void test_switch_input_mapping() {
    SwitchProProtocol protocol;
    ready_protocol(protocol);

    ControllerState state{};
    state.buttons = CONTROLLER_BUTTON_SQUARE | CONTROLLER_BUTTON_CROSS |
                    CONTROLLER_BUTTON_CIRCLE | CONTROLLER_BUTTON_TRIANGLE |
                    CONTROLLER_BUTTON_L1 | CONTROLLER_BUTTON_R1 |
                    CONTROLLER_BUTTON_HOME | CONTROLLER_BUTTON_PAD |
                    CONTROLLER_BUTTON_CREATE | CONTROLLER_BUTTON_OPTIONS;
    state.dpad = CONTROLLER_DPAD_UP_RIGHT;
    state.lx = 0x0000;
    state.ly = 0x0000;
    state.rx = 0xffff;
    state.ry = 0xffff;
    state.l2 = 1;
    state.r2 = 1;
    state.battery = 10;
    protocol.update_controller(state);

    uint8_t out[64]{};
    assert(protocol.next_report(10, out));
    assert(out[0] == 0x30);
    assert(out[2] == 0x80);            // full battery
    assert((out[3] & 0xcf) == 0xcf);   // Y/X/B/A/R/ZR
    assert((out[4] & 0xb3) == 0xb3);   // Minus/Plus/Home/Capture/wired
    assert((out[5] & 0xc6) == 0xc6);   // Up/Right/L/ZL
    assert(unpack_x(out + 6) == 0x15c);
    assert(unpack_y(out + 6) == 0xea4);
    assert(unpack_x(out + 9) == 0xea4);
    assert(unpack_y(out + 9) == 0x15c);
}

void test_subcommands_and_spi() {
    SwitchProProtocol protocol;
    ready_protocol(protocol);
    uint8_t packet[64]{};
    uint8_t out[64]{};

    packet[0] = 0x01;
    packet[10] = 0x02;
    protocol.handle_output_report(0, packet, sizeof(packet));
    assert(protocol.next_report(10, out));
    assert(out[0] == 0x21 && out[13] == 0x82 && out[14] == 0x02);
    assert(out[17] == 0x03); // Pro Controller type

    std::memset(packet, 0, sizeof(packet));
    packet[0] = 0x01;
    packet[10] = 0x10;
    packet[11] = 0x3d;
    packet[12] = 0x60;
    packet[15] = 9;
    protocol.handle_output_report(0, packet, sizeof(packet));
    assert(protocol.next_report(11, out));
    assert(out[13] == 0x90 && out[14] == 0x10);
    assert(out[20] == 0xa4 && out[21] == 0x46 && out[22] == 0x6a);

    // The advertised IMU calibration uses neutral offsets, not offsets copied
    // from an unrelated physical controller.
    std::memset(packet, 0, sizeof(packet));
    packet[0] = 0x01;
    packet[10] = 0x10;
    packet[11] = 0x20;
    packet[12] = 0x60;
    packet[15] = 24;
    protocol.handle_output_report(0, packet, sizeof(packet));
    assert(protocol.next_report(12, out));
    for (size_t i = 0; i < 6; ++i) assert(out[20 + i] == 0);
    for (size_t i = 12; i < 18; ++i) assert(out[20 + i] == 0);
}

void test_switch_imu_conversion() {
    SwitchProProtocol protocol;
    ready_protocol(protocol);
    uint8_t packet[64]{};
    uint8_t out[64]{};

    packet[0] = 0x01;
    packet[10] = 0x40; // enable IMU
    packet[11] = 0x01;
    protocol.handle_output_report(0, packet, sizeof(packet));
    assert(protocol.next_report(3, out)); // subcommand ACK

    ControllerState first{};
    first.accel[0] = 200;
    first.gyro[0] = 160;
    protocol.update_controller(first);

    ControllerState second{};
    second.accel[0] = 400;
    second.gyro[0] = 320;
    protocol.update_controller(second);

    ControllerState newest{};
    newest.accel[0] = 600;
    newest.accel[1] = -400;
    newest.accel[2] = 1000;
    newest.gyro[0] = 480;
    newest.gyro[1] = -320;
    newest.gyro[2] = 160;
    protocol.update_controller(newest);

    assert(protocol.next_report(10, out));
    assert(out[0] == 0x30);

    // Sample 0 is newest. Switch axes are the inverse of SDL's conversion to
    // PlayStation coordinates, and the units are down-scaled to Switch ranges.
    assert(read_s16(out + 13) == -500); // accel X = -DS Z / 2
    assert(read_s16(out + 15) == -300); // accel Y = -DS X / 2
    assert(read_s16(out + 17) == -200); // accel Z =  DS Y / 2
    assert(read_s16(out + 19) == -143); // gyro X = -DS Z * 14.284/16
    assert(read_s16(out + 21) == -429); // gyro Y = -DS X * 14.284/16
    assert(read_s16(out + 23) == -286); // gyro Z =  DS Y * 14.284/16

    // The remaining packet samples retain recent reports, newest to oldest.
    assert(read_s16(out + 27) == -200);
    assert(read_s16(out + 33) == -286);
    assert(read_s16(out + 39) == -100);
    assert(read_s16(out + 45) == -143);
}

void test_switch_rumble_conversion() {
    const uint8_t maximum_left[8] = {
        0x00, 0xc8, 0x40, 0x72,
        0x00, 0x01, 0x40, 0x40,
    };
    const SwitchRumbleState maximum = switch_decode_hd_rumble(maximum_left);
    assert(maximum.heavy >= 250);
    assert(maximum.light >= 250);

    const uint8_t split_bands[8] = {
        0x00, 0x64, 0x40, 0x4a, // high code 50, low code 20
        0x00, 0x3c, 0x40, 0x63, // high code 30, low code 70
    };
    const SwitchRumbleState split = switch_decode_hd_rumble(split_bands);
    assert(split.heavy > split.light);
    assert(split.light > 0);

    SwitchProProtocol protocol;
    ready_protocol(protocol);
    uint8_t packet[64]{};
    uint8_t out[64]{};
    SwitchRumbleState pending{};

    packet[0] = 0x01;
    packet[10] = 0x48; // enable vibration
    packet[11] = 0x01;
    protocol.handle_output_report(0, packet, sizeof(packet));
    assert(protocol.next_report(3, out));
    assert(protocol.take_rumble(pending));
    assert(pending == SwitchRumbleState{});

    std::memset(packet, 0, sizeof(packet));
    packet[0] = 0x10;
    std::memcpy(packet + 2, maximum_left, sizeof(maximum_left));
    protocol.handle_output_report(0, packet, sizeof(packet));
    assert(protocol.take_rumble(pending));
    assert(pending == maximum);
}

} // namespace

int main() {
    test_dualsense_parser();
    test_switch_input_mapping();
    test_subcommands_and_spi();
    test_switch_imu_conversion();
    test_switch_rumble_conversion();
    std::cout << "switch_pro host tests passed\n";
    return 0;
}
