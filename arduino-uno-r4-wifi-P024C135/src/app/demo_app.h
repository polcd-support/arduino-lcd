/*
 * 演示程序：与参考工程 Core/Src/lcd_demo.c 的 LCD_DEMO() 效果一致。
 *
 * 状态顺序（参考工程里可到达的流程，起始状态为文字页）：
 *   文字 2s -> 全屏纯色 5 色 x 1s -> 彩条 3s -> 灰阶 3s -> 手写 + 清屏按钮
 *
 * 与参考工程的两处实现差异（视觉结果相同）：
 *  - 参考工程在每个循环里重复整屏重绘；这里改为进入状态时画一次，
 *    否则灰阶每 10 ms 重画一次会让触摸失去响应。
 *  - 参考工程里 STATE_LOGO / STATE_IMAGE / STATE_COUNTDOWN 不可达（上电初始状态就是
 *    文字页，且没有任何转移指向它们），这里没有移植。
 */
#ifndef DEMO_APP_H
#define DEMO_APP_H

/* 初始化屏幕、清屏、开背光，并进入文字画面。 */
void DemoApp_Setup(void);

/* 在 loop() 里反复调用，每次推进一小步。 */
void DemoApp_Loop(void);

#endif
