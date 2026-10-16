/*
 * Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3
 * + P028X101（ST7789 240x320 TFT + FT6236 电容触摸）
 *
 * 演示效果与 STM32 参考工程 stm32g0b1rc-P028X101 的 LCD_DEMO() 一致：
 *   LOGO 3s -> 多字号文字 2s -> 全屏纯色 5 色 x 1s -> 彩条 3s -> 灰阶 3s
 *   -> 手写画线（右下角 Clear 清屏）
 *
 * 屏幕相关代码全部隔离在 src/screen/ 下（板卡引脚、面板、触摸、绘图各一层），
 * 换屏或加屏只改 src/screen/screen_config.h 与 screen_devices.h，详见 README.md。
 *
 * 接线（转接板已固定，无需外接）：
 *   D8 LCD_RST   D9 LCD_DC   D10 LCD_CS   D11 LCD_MOSI   D13 LCD_SCK
 *   背光由硬件飞线强制拉高，D6/D7 均不使用（D7 未接，U1/U2 OE 板上固定使能）
 *   D5 触摸复位  D4 触摸中断   A4 触摸 SDA   A5 触摸 SCL
 */

#include "src/app/demo_app.h"

void setup()
{
    DemoApp_Setup();
}

void loop()
{
    DemoApp_Loop();
}
