/*
 * Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3
 * + P020B025P-IPS-V2（ST7789P3 240x320 IPS，无触摸）
 *
 * 演示效果与 STM32 参考工程 stm32g0b1rc-P020B025P-IPS-V2 的
 * LCD_Display_RunTest() 一致：全屏黑 -> 开背光 -> 彩条 3s -> 灰阶 3s -> 循环。
 *
 * 屏幕相关代码全部隔离在 src/screen/ 下（板卡引脚、面板、绘图各一层），
 * 换屏或加屏只改 src/screen/screen_config.h 与 screen_devices.h，详见 README.md。
 *
 * 接线（转接板已固定，无需外接）：
 *   D8 LCD_RST   D9 LCD_DC   D10 LCD_CS   D11 LCD_MOSI   D13 LCD_SCK
 *   背光由硬件飞线强制拉高，D6/D7 均不使用（D7 未接，U1/U2 OE 板上固定使能）
 *
 * 本模组不带触摸，转接板的触摸脚（D5/D4/A4/A5）本工程不使用。
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
