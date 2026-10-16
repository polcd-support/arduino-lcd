# arduino-uno-r4-wifi

Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3 + P020B025P-IPS-V2
（ST7789P3 240x320 IPS，无触摸）的点屏演示。

效果与 STM32 参考工程 `stm32g0b1rc-P020B025P-IPS-V2` 的 `LCD_Display_RunTest()` 一致：

```
彩条 3s -> 灰阶 3s -> 循环
```

（背光由硬件常开，固件不再有“开背光”这一步。）

本屏没有触摸，所以参考工程里的手写/文字/图片演示本来就不存在（参考 `lcd_demo.c` 只做彩条与灰阶）。
屏幕相关代码全部隔离在 `src/screen/`，按“板卡 / 面板 / 绘图”分层，换屏或加屏只碰配置和新增文件。
以上流程已用主机端虚拟面板跑通并逐帧核对（见 `test/`）。

## 硬件与接线

转接板已把信号固定好，无需外接飞线。依据转接板原理图与 README Rev 1.3：

| 信号 | UNO R4 WiFi | 说明 |
|---|---|---|
| LCD SDA/MOSI | D11 | 经 U1 TXB0108 |
| LCD SCK | D13 | 经 U1 |
| LCD CS | D10 | 经 U1 |
| LCD DC/RS | D9 | 经 U1 |
| LCD RST | D8 | 经 U1 |
| 背光 | 硬件强制拉高 | 实物已飞线把背光常开；D6 保持高阻，固件不配置也不驱动 |
| U1/U2 使能 OE | 固定使能 | 转接板上 OE 固定接 3.3 V，电平转换器始终打开；D7 未连接、固件不使用 |

本模组**不带触摸**，转接板的触摸脚（D5 触摸复位 / D4 触摸中断 / A4·A5 触摸 I2C）本工程不驱动，
`src/screen/boards/uno_r4_wifi.h` 里 `SCREEN_PIN_TOUCH_RST` / `SCREEN_PIN_TOUCH_INT` 置 `-1`。

关键点：电平转换器 OE 固定使能，无需固件干预；背光已由硬件强制开启，D6 与 D7 都不由固件操作。
固件只需配置 LCD 控制脚、启动 SPI0（24 MHz）后复位并初始化面板，初始化与整个演示期间都不会碰 D6。

## 编译与上传

Arduino IDE 选择 **Arduino UNO R4 WiFi**，打开 `arduino-uno-r4-wifi-P020B025P-IPS-V2.ino` 直接上传。
命令行等价写法：

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi .
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi -p <串口> .
```

串口 115200 会打印一行自检信息：

```
screen: P020B025P-IPS-V2  panel=P020B025P-IPS-V2 (ST7789P3 240x320)  240x320  rotation=0  spi=24MHz  bpp=2  tx=DTC  actual=24.0MHz  touch=none
```

## 目录结构

```
arduino-uno-r4-wifi-P020B025P-IPS-V2.ino 只做 setup/loop 转发
src/app/demo_app.{h,cpp}       LCD_Display_RunTest() 的状态机（彩条 <-> 灰阶）
src/screen/                    屏幕模块（换屏只动这里）
  screen_config.h              选板卡、选屏幕、默认方向、缓冲区大小
  screen.{h,cpp}               对外接口：初始化时序、SPI 像素流水线、旋转
  screen_device.h              数据模型：面板 / 触摸驱动 / 设备
  screen_devices.h             设备注册表（新增屏幕登记处）
  boards/uno_r4_wifi.h         板卡引脚与总线映射
  bus/spi_dma_ra4m1.{h,cpp}    RA4M1 SPI0 + DTC 硬件搬运后端（与屏幕无关）
  panels/panel_p020b025p_ips_v2.{h,cpp}   ST7789P3 面板描述与初始化表
  gfx/gfx.{h,cpp}              绘图 API（LCD_*），移植自参考工程 lcd.c
  gfx/fonts.{h,cpp}            ASCII 12/16/24/32 与中文点阵（来自参考工程 lcdfont.h）
  gfx/image.h                  RLE 图像结构（本工程 demo 未用到图片，保留以备用）
test/host/                     主机端验证（Arduino 桩件 + 虚拟 ST7789P3）
test/run.sh                    编译并运行主机端验证，结果图输出到 test/out/
tools/pic2rle.py               把 Image2Lcd 风格的 C 数组转成 RLE 资产（--bpp 2/3）
```

## 分层约定

- `app` 只调用 `gfx`（`LCD_*`）和 `screen`（`Screen_*`），不碰寄存器。
- `gfx` 颜色统一 RGB565；`screen` 负责换算成面板真正需要的格式（本面板本来就是 RGB565，直接发）。
- 面板只描述“寄存器怎么写”，方向修正（翻转/交换/偏移）写在设备里。
- 一次绘制的正确姿势：

```cpp
Screen_SetWindow(x, y, x + w - 1, y + h - 1);   // 设置窗口，CS 保持有效
Screen_WritePixels565(pixels, w * h);            // 由它释放 CS
```

## 新增一块屏

1. 在 `src/screen/panels/` 加 `panel_xxx.{h,cpp}`，填写 `ScreenPanel`：分辨率、初始化序列、
   4 个方向的 `madctl`/GRAM 偏移、像素格式（RGB565 或 RGB666）、建议 SPI 时钟与模式。
   `.cpp` 用 `#if SCREEN_DEVICE == SCREEN_DEVICE_XXX` 包起来，未选中的屏不参与编译。
2. 模组若带触摸，补一个实现 `ScreenTouchDriver` 的驱动文件（寄存器布局按具体 IC 写），
   也用 `#if SCREEN_TOUCH_DRIVER == ...` 包起来；无触摸时把设备表里的 `touch` 写成 `NULL`
   即可（本工程即如此）。
3. 在 `src/screen/screen_devices.h` 的表里加一行。
4. 在 `src/screen/screen_config.h` 里把 `SCREEN_DEVICE` 指过去。
5. 板卡引脚不同就加 `src/screen/boards/xxx.h`，改 `screen_config.h` 里包含的那一个。

面板把颜色按 RGB666 发还是按 RGB565 发，由 `ScreenPanel::pixel_format` 决定，上层代码不用改；
写像素、填充、文字、图像都走同一条 `Screen_WritePixels565()` 流水线。

## 触摸

本模组无触摸：`ScreenDevice::touch == NULL`，`Screen_HasTouch()` 返回 `false`，
`Screen_TouchRead()` 恒返回 0 个触点。触摸相关代码在 `screen.cpp` 中保留（判空后直接返回），
以后换带触摸的模组时补一个触摸驱动即可，无需改动上层。

## 与 STM32 参考工程的差异

| 项 | 参考工程 | 本工程 | 原因 |
|---|---|---|---|
| 像素格式 | 16-bit（0x3A=0x05，RGB565） | 相同 | ST7789P3 的 4 线 SPI 用 RGB565，每像素 2 字节 |
| 面板复位保持时间 | RES 低 100 ms → 高 120 ms | RES 低 100 ms → 高 100 ms | 样板 `screen.cpp` 固定 100/100，本屏沿用；120→100 ms 对本面板无影响 |
| 重复整屏重绘 | 每轮循环都重画当前画面 | 进入状态时画一次 | 观感相同，省 SPI 带宽 |
| 初始化末尾 0x2C | `LCD_Init()` 末尾写 0x2C 并延时 20 ms | 照抄（面板表最后一行） | 保持一致；本工程每次 `Screen_SetWindow` 都会重设窗口，不依赖这一条 |
| LOGO / 文字 / 手写 | `lcd_demo.c` 里本就没有 | 未移植 | 参考的 `LCD_Display_RunTest()` 只有彩条与灰阶 |
| 触摸 | 参考工程完全不碰触摸 | 本工程同样无触摸 | 模组不带触摸 IC |
| 中文字库索引 | GBK 两字节 | UTF-8 三字节 | 源码用 UTF-8 直接写中文即可（本 demo 未用到中文） |

## 验证

```sh
sh test/run.sh
```

`test/` 用桩件替换 Arduino API，把 SPI 字节流喂给虚拟 ST7789P3，检查：

- 面板初始化 19 条命令的序列与各命令数据长度（外加 `Screen_Init` 结尾应用方向的一条 `0x36`）、
  `0x3A=0x05`、`0x11` 后 120 ms 延时、末尾 `0x2C` 后 20 ms 延时；
- 背光由外部硬件保持 255（初始化与整个演示期间都是），且固件从未配置或写入 D6
  （`Sim_BacklightPinTouchCount() == 0`，演示全程再查一次）；
- setup 清屏写满 240x320 像素；
- 彩条 6 条颜色与分界（x=20/60/100/140/180/220 取中点核对）、占满整屏高度；
- 灰阶 24 级首格黑、末格白、逐格单调递增、占满整屏高度；
- 3 s / 6 s 两个切换点后回到对应画面；
- `Screen_HasTouch() == false`（无触摸）。

结果图 `test/out/*.png` 可人工核对：`1-colorbar.png`（彩条）、`2-grayscale.png`（灰阶）。

尚未验证（需要实物）：SPI 实际波形与最高可用时钟、背光电流与亮度、ST7789P3 实际显色与 Gamma。
低速保守起见，若 24 MHz 下出现随机色点或错行，把 `SCREEN_SPI_HZ` 改回 `12000000UL`。

## 已知限制

- SPI 后端用 RA4M1 的 SPI0 + DTC 硬件搬运，`SCREEN_SPI_HZ` 取 24 MHz（面板侧上限与板卡档一致），
  RA4M1 PCLKA=48 MHz，24 MHz 分频可精确得到；请求 20 MHz 时实际会向下分频到 12 MHz，所以直接用
  24 MHz 档。出现随机色点/错行就改回 `12000000UL`。
- 整屏填充 240x320 需要发 153 600 字节，24 MHz 下约 0.05 s；DTC 搬运时 CPU 不逐字节参与。
- 字符宽度按 `sizey/2`，超出屏幕的字模整字丢弃（不会写出窗口外）。

## 资产来源

- `src/screen/gfx/fonts.cpp`、`fonts.h`：参考工程 `HARDWARE/LCD/lcdfont.h`，只把中文索引从 GBK
  改成 UTF-8 三字节，点阵数据未动。
- 本工程 demo **不含图片资产**，因此没有 `src/screen/assets/logo.h`；`gfx/image.h`、`LCD_ShowImage()`
  与 `tools/pic2rle.py` 保留，作为以后加图的绘图能力。
- `tools/pic2rle.py` 新增 `--bpp`（默认 3，向后兼容；2 = 源数据已是 RGB565，每像素 2 字节、高字节在前）。
  参考工程 `HARDWARE/LCD/pic.h` 的 `gImage_img1/gImage_logo` 图像本体是 RGB565
  （数组长度 = 8 + W*H*2），以后要用它们加图时命令形如：

```sh
python tools/pic2rle.py <参考工程>/HARDWARE/LCD/pic.h <宽> <高> src/screen/assets/xxx.h --skip 8 --bpp 2
```
