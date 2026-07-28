# Pico DualSense Switch Bridge
[English](./README.md)
> 使用 Pico 2 W 将蓝牙 DualSense 转换为 PC DualSense 或 Switch 2 Pro Controller USB 模式。

> 此翻译来自上游项目，尚未涵盖本分支的全部功能。英文 README 是当前的权威文档。

# 功能特点
 - 支持HD震动

# 使用方法
1. 按住 Pico 上的BOOTSEL进入刷机
2. 将 .uf2 文件拖入进去
3. 将 DS5 手柄进入蓝牙配对模式
4. Enjoy it

***你可能需要在控制器处于匹配模式时重新插拔 pico***

- 手柄连接到pico以后，系统才会显示设备

# Pico 配置调整
你可以通过网页调整Pico的内部设置

- 用于正式固件: https://ds5.awalol.eu.org
- 用于测试固件: https://ds5-dev.awalol.eu.org

### Pico W 版本

Pico W 由于性能问题，只能支持震动，不支持扬声器。
你可以通过开启 `-DPICO_W_BUILD=ON` 编译项去开启 Pico W 固件编译，或者在 Github Action 下载预编译的固件

### USB 唤醒支持
这是一项实验性的功能。如果你需要该功能，请前往 feat/usb-wake 分支进行编译，或者使用该分支对应的 Github Action 预编译固件。`pico-dualsense-switch-bridge-wake.uf2` 为该功能的固件

极为建议在使用该功能前阅读  #60 和 #61

### 社区分支
https://github.com/MarcelineVPQ/DS5Dongle-OLED-Edition
https://github.com/zurce/DS5Dongle-OLED

# 当前问题:
- 声音可能有点小卡顿
- 由于编码需要，需要对pico进行超频，当前的参数是1.2V 320MHz。
- 若您的pico使用该超频参数无法启动，请自行增加电压或者降低频率

# 未来计划
请查看[DS5Dongle plan](https://github.com/users/awalol/projects/5)

# 编译
需要将pico sdk里面的tinyusb版本升级到最新

# 致谢
 - [rafaelvaloto/Pico_W-Dualsense](https://github.com/rafaelvaloto/Pico_W-Dualsense) - 灵感来源
 - [egormanga/SAxense](https://github.com/egormanga/SAxense) - 震动报文
 - [https://controllers.fandom.com/wiki/Sony_DualSense](https://controllers.fandom.com/wiki/Sony_DualSense) - 数据报文结构
 - [Paliverse/DualSenseX](https://github.com/Paliverse/DualSenseX) - 扬声器数据包报文
