/*
 * Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3
 * + P035M075-IPS-CTP（ILI9488 320x480 IPS + FT6336U 电容触摸）
 *
 * LCD 演示：LOGO、文字、纯色、彩条、灰阶和触摸手写。
 * 背光已由硬件飞线强制拉高；固件不配置也不写 D6。
 *
 * 屏幕相关代码全部隔离在 src/screen/ 下（板卡引脚、面板、触摸、绘图各一层），
 * 换屏或加屏只改 src/screen/screen_config.h 与 screen_devices.h，详见 README.md。
 *
 * 接线（转接板已固定，无需外接）：
 *   D8 LCD_RST   D9 LCD_DC   D10 LCD_CS   D11 LCD_MOSI   D13 LCD_SCK
 *   背光由硬件强制拉高，D6/D7 均不使用
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
