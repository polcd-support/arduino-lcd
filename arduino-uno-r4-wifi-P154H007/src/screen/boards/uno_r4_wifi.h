/*
 * Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3 引脚映射。
 *
 * 依据：转接板原理图 Arduino-UNO-R4-WIFI-SPI通用转接板.kicad_sch 与同目录 README.md。
 *
 *   信号            转接板网络      UNO R4 WiFi   说明
 *   LCD SDA(MOSI)   LCD_SDA        D11           U1 TXB0108 通道 A2/B2
 *   LCD SCK         LCD_CLK        D13           U1 通道 A1/B1
 *   LCD CS          LCD_CS         D10           U1 通道 A3/B3
 *   LCD DC/RS       LCD_DC         D9            U1 通道 A4/B4
 *   LCD RESET       LCD_RST        D8            U1 通道 A5/B5
 *   背光控制        LCD_PWR        硬件强制拉高   D6 不再由固件驱动
 *   D7              -              D7            转接板未连接
 *
 * 注意：
 * - U1/U2 的 OE 在转接板上固定接使能，电平转换器始终工作，D7 未连接也不参与控制。
 * - 当前实物已飞线把背光强制拉高；D6 保持高阻输入，固件不配置也不写该引脚。
 * - P154H007 模组无触摸：转接板上的 TP_SDA/TP_CLK/TP_INT/TP_RST（A4/A5/D4/D5）
 *   在本工程不配置，因此 I2C 总线也不再使用（SCREEN_I2C_HZ 仅作占位）。
 */
#ifndef SCREEN_BOARD_UNO_R4_WIFI_H
#define SCREEN_BOARD_UNO_R4_WIFI_H

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#define SCREEN_SPI_BUS SPI
#define SCREEN_I2C_BUS Wire

#define SCREEN_PIN_CS 10
#define SCREEN_PIN_DC 9
#define SCREEN_PIN_RST 8
#define SCREEN_PIN_BL (-1)

/* P154H007 无触摸：-1 = 不配置该引脚（screen.cpp 与触摸驱动都用 #if >= 0 守卫）。 */
#define SCREEN_PIN_TOUCH_RST (-1)
#define SCREEN_PIN_TOUCH_INT (-1)

/* 背光已由硬件强制开启；该配置仅用于兼容通用接口，固件不做 PWM 调光。 */
#define SCREEN_BL_PWM 0

/* RA4M1 PCLKA=48 MHz，SPI0 可用高速档为 24/12 MHz。
 * 20 MHz 请求实际会向下分频到 12 MHz；这里开启 24 MHz 实物测试档。
 * 如果屏幕出现随机色点/错行，改回 12000000UL。 */
#define SCREEN_SPI_HZ 24000000UL

/* RA4M1 使用 SPI0 + DTC 硬件搬运，仍保持 8 位帧和原始字节顺序。 */
#define SCREEN_SPI_DMA 1

/* DTC/FSP 初始化失败时的保守回退路径。 */
#define SCREEN_SPI_BYTE_TRANSFER 1

/* 本模组无触摸，I2C 不使用；保留宏以免 screen.cpp 引用缺失。 */
#define SCREEN_I2C_HZ 100000UL

#endif
