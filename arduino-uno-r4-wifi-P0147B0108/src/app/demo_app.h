/*
 * 演示程序：与参考工程 Core/Src/lcd_demo.c 的 LCD_Display_RunTest() 效果一致。
 *
 * 状态顺序（参考工程唯一的循环）：
 *   彩条 3s -> 灰阶 3s -> 彩条 3s -> ...
 * 起始状态为彩条。本模组无触摸，参考工程也未绘制 Clear 按钮，故不移植手写/清屏。
 *
 * 与参考工程的一处实现差异（视觉结果相同）：
 *  - 参考工程每个循环重复整屏重绘；这里改为进入状态时画一次，
 *    否则 172x320 灰阶每轮重画会拖慢刷新。
 *  - 参考工程 LCD_Display_Init() 先清屏再开背光，这里的 DemoApp_Setup 照做。
 */
#ifndef DEMO_APP_H
#define DEMO_APP_H

/* 初始化屏幕、清屏、开背光，并进入彩条画面。 */
void DemoApp_Setup(void);

/* 在 loop() 里反复调用，每次推进一小步。 */
void DemoApp_Loop(void);

#endif
