/*
 * 屏幕模块编译期配置。
 *
 * 换屏只改这里：选板子、选屏幕（面板 + 触摸）、选默认方向。
 * 新增屏幕的步骤见 README。
 */
#ifndef SCREEN_CONFIG_H
#define SCREEN_CONFIG_H

/* ---------------- 板卡 ---------------- */
#define SCREEN_BOARD_UNO_R4_WIFI 1
#define SCREEN_BOARD SCREEN_BOARD_UNO_R4_WIFI

/* ---------------- 屏幕 ---------------- */
/* 设备 = 面板 + 触摸组合，注册表见 screen_devices.h。 */
#define SCREEN_DEVICE_P0147B0108 0
#define SCREEN_DEVICE SCREEN_DEVICE_P0147B0108

/* ---------------- 触摸驱动 ---------------- */
/* 本模组无触摸（参考工程 HARDWARE/CST816 是空目录，demo 只用 SPI 点屏），选 NONE：
 * 设备表里 touch 为 NULL，没有任何触摸驱动参与编译，src/screen/touch/ 已移除。
 * 换带触摸的模组时从样板 P035M075-IPS-CTP 拷回 touch/ 并在此加 SCREEN_TOUCH_DRIVER_XXX。 */
#define SCREEN_TOUCH_DRIVER_NONE 0
#define SCREEN_TOUCH_DRIVER SCREEN_TOUCH_DRIVER_NONE

/* ---------------- 运行参数 ---------------- */
/* 上电方向：0/1 竖屏，2/3 横屏（相对面板原生方向）。 */
#define SCREEN_DEFAULT_ROTATION 0

/* 发送缓冲区字节数，必须同时被 2（RGB565）和 3（RGB666）整除：
 * 5760 = 2880 像素 x 2 字节 = 1920 像素 x 3 字节，减少 DTC 启动并保留充足堆栈。 */
#define SCREEN_TX_BYTES 5760

/* 1 = 初始化时通过串口 115200 打印面板名、触摸 ID 和结果。 */
#define SCREEN_DEBUG 1

/* ---------------- 板卡引脚映射 ---------------- */
#include "boards/uno_r4_wifi.h"

#endif
