# arduino-uno-r4-wifi

Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3 + P024B044P
（ST7789 240x320 IPS + FT6236 电容触摸）的点屏与触摸演示。

效果与 STM32 参考工程 `stm32g0b1rc-P024B044P` 的 `LCD_DEMO()` 一致：

```
LOGO 3s -> 多字号文字 2s -> 全屏纯色 5 色 x 1s -> 彩条 3s -> 灰阶 3s
-> 手写画线（右下角 Clear 清屏，左下角 Dial 进入拨号画面）
```

屏幕相关代码全部隔离在 `src/screen/`，按“板卡 / 面板 / 触摸 / 绘图”分层，换屏或加屏只碰配置和
新增文件。以上流程已用主机端虚拟面板跑通并逐帧核对（见 `test/`）。

## 硬件与接线

转接板已把信号固定好，无需外接飞线。依据转接板原理图与 README Rev 1.3：

| 信号 | UNO R4 WiFi | 说明 |
|---|---|---|
| LCD SDA/MOSI | D11 | 经 U1 TXB0108 |
| LCD SCK | D13 | 经 U1 |
| LCD CS | D10 | 经 U1 |
| LCD DC/RS | D9 | 经 U1 |
| LCD RST | D8 | 经 U1 |
| 背光 | 硬件强制拉高 | D6 保持高阻，固件不配置、不输出 |
| D7 | 未连接 | 固件不使用 |
| 触摸 SDA / SCL | A4 / A5 | 走硬件 `Wire`，经 U2 TXS0104E |
| 触摸 RST / INT | D5 / D4 | INT 本驱动轮询，未使用 |

U1/U2 的 OE 固定接 3.3 V，转换器始终使能，D7 不参与控制。固件按
**LCD 控制脚置为确定电平（复位拉低、CS 拉高）→ `SPI.begin()`/`Wire.begin()`
→ 面板复位与初始化 → 触摸复位与初始化 → 绘制首屏** 的顺序启动；背光已由板上飞线强制拉高，
固件全程不碰 D6，`Screen_Backlight*()` 为空操作。

## 编译与上传

Arduino IDE 选择 **Arduino UNO R4 WiFi**，打开 `arduino-uno-r4-wifi-P024B044P.ino` 直接上传。
命令行等价写法：

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi .
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi -p <串口> .
```

串口 115200 会打印一行自检信息：

```
screen: P024B044P  panel=P024B044P (ST7789 240x320)  240x320  rotation=0  spi=24MHz  bpp=2  tx=DTC  actual=24.0MHz  touch=FT6336U @0x38
  chip=0x64
```

`chip` 读不到或不是 0x64/0x11 一类 FT6xxx 值，说明触摸 I2C 没通（检查排线方向与 I2C 波形）。
注意：`chip` 非零只说明触摸 I2C 通了，触摸功能仍需实物手测确认，不能凭此判定已硬件验证。

## 目录结构

```
arduino-uno-r4-wifi-P024B044P.ino 只做 setup/loop 转发（文件名与草图目录一致）
src/app/demo_app.{h,cpp}       LCD_DEMO() 的状态机与各画面（含拨号画面）
src/screen/                    屏幕模块（换屏只动这里）
  screen_config.h              选板卡、选屏幕、默认方向、缓冲区大小
  screen.{h,cpp}               对外接口：初始化时序、SPI 像素流水线、旋转、触摸换算
  screen_device.h              数据模型：面板 / 触摸驱动 / 设备
  screen_devices.h             设备注册表（新增屏幕登记处）
  boards/uno_r4_wifi.h         板卡引脚与总线映射
  panels/panel_p024b044p.{h,cpp}   ST7789 面板描述与初始化表
  touch/touch.h                触摸驱动接口
  touch/touch_ft6336u.{h,cpp}  FT6236/FT6336U（FT6xxx 寄存器，本模组，默认）
  touch/touch_cst816.{h,cpp}   CST816S/T（同款 FPC 排线的其它模组）
  gfx/gfx.{h,cpp}              绘图 API（LCD_*），移植自参考工程 lcd.c
  gfx/fonts.{h,cpp}            ASCII 12/16/24/32 与中文点阵（来自参考工程 lcdfont.h）
  gfx/image.h                  RLE 图像结构
  assets/logo.h                LOGO 资产（RLE，14 088 字节）
test/host/                     主机端验证（Arduino 桩件 + 虚拟 ST7789）
test/run.sh                    编译并运行主机端验证，结果图输出到 test/out/
tools/pic2rle.py               把 Image2Lcd 风格的 C 数组转成 RLE 资产
```

## 分层约定

- `app` 只调用 `gfx`（`LCD_*`）和 `screen`（`Screen_*`），不碰寄存器。
- `gfx` 颜色统一 RGB565；`screen` 负责换算成面板真正需要的格式（本面板就是 RGB565）。
- 面板只描述“寄存器怎么写”，触摸只描述“寄存器怎么读”，方向修正（翻转/交换/偏移）写在设备里。
- 一次绘制的正确姿势：

```cpp
Screen_SetWindow(x, y, x + w - 1, y + h - 1);   // 设置窗口，CS 保持有效
Screen_WritePixels565(pixels, w * h);            // 由它释放 CS
```

## 新增一块屏

1. 在 `src/screen/panels/` 加 `panel_xxx.{h,cpp}`，填写 `ScreenPanel`：分辨率、初始化序列、
   4 个方向的 `madctl`/GRAM 偏移、像素格式（RGB565 或 RGB666）、建议 SPI 时钟与模式。
   `.cpp` 用 `#if SCREEN_DEVICE == SCREEN_DEVICE_XXX` 包起来，未选中的屏不参与编译。
2. 触摸若不是 FT6336U/CST816，在 `src/screen/touch/` 加一个实现 `ScreenTouchDriver` 的文件，
   同样用 `#if SCREEN_TOUCH_DRIVER == ...` 包起来。
3. 在 `src/screen/screen_devices.h` 的表里加一行。
4. 在 `src/screen/screen_config.h` 里把 `SCREEN_DEVICE`（和需要时的 `SCREEN_TOUCH_DRIVER`）指过去。
5. 板卡引脚不同就加 `src/screen/boards/xxx.h`，改 `screen_config.h` 里包含的那一个。

面板把颜色按 RGB666 发还是按 RGB565 发，由 `ScreenPanel::pixel_format` 决定，上层代码不用改；
写像素、填充、文字、图像都走同一条 `Screen_WritePixels565()` 流水线。

## 触摸标定

默认与参考工程一致：240x320、不交换、不翻转、无偏移（方向 0 下原生坐标即显示坐标）。
FT6336U 使用 100 kHz I²C 轮询模式，阈值为 `SCREEN_TOUCH_THRESHOLD=40`；数值越低越灵敏。
实测方向不对时改 `panels/panel_p024b044p.cpp` 里 `p024b044p_touch` 的
`swap_xy`、`invert_x`、`invert_y`、`offset_x`、`offset_y`。
`Screen_TouchReadRaw()` 返回未换算的原生坐标，便于对照标定。
方向 2/3（横屏）的触摸换算是按 MADCTL 的 `MV` 位推出来的，实物核对前请先只在方向 0 下用。

## 与 STM32 参考工程的差异

| 项 | 参考工程 | 本工程 | 原因 |
|---|---|---|---|
| 0x36 (MADCTL) | 只写死 0x00，仅支持方向 0 | 方向 0/1/2/3 = 0x00/0xC0/0x70/0xA0 | 横屏值按 ST7789 标准补全，不改方向 0 的行为 |
| 初始化末尾 | 读回 LCD ID 后 `if(id==0x42C2A9) 0x20; else 0x21;` | 硬编码 `0x21`（Display Inversion On） | UNO R4 + 转接板没有 MISO 回读线，无法复刻该运行时分支；若实物显色反相，把面板表里这条改成 0x20 |
| 重复整屏重绘 | 每个循环都重画当前画面 | 进入状态时画一次 | 否则 RLE LOGO/灰阶每 10 ms 重画会卡住触摸，观感相同；拨号画面本身是事件驱动重绘，不受影响 |
| LOGO 数据 | `gImage_logo[]` 含 8 字节图像头，`LCD_ShowPicture()` 把它当像素发出（整图右移 4 像素、末尾少 4 像素） | 从第 8 字节起按 240x220 完整显示在 (0,29) | 修正参考工程的偏移，画面内容一致 |
| 文字内容 | `STM32 Display` / `Multi-Size Text` / `浦洋液晶` | `UNO R4 WiFi` / `Multi-Size Text` / `浦洋液晶` | 字号、位置、颜色不变，只换掉板名 |
| 未用状态 | `STATE_IMAGE`、`STATE_COUNTDOWN` 不可达 | 未移植 | 参考代码里同样进不去 |
| 手写线段保护 | 跨度超过 50 像素的线段丢弃、贴边点不画 | 相同（`LCD_MAX_SEGMENT`） | 保持与参考工程一致的行为 |
| 中文字库索引 | GBK 两字节 | UTF-8 三字节 | 源码用 UTF-8 直接写中文即可 |

## 验证

```sh
sh test/run.sh
```

`test/` 用桩件替换 Arduino API，把 SPI 字节流喂给虚拟 ST7789，检查：

- 初始化命令序列（16 条）、各命令数据长度、`0x3A=0x05`、`0x21` 在 `0x29` 之前、`0x11` 后 120 ms 延时；
- SPI 请求时钟为 24 MHz，像素数据由 SPI0 + DTC 搬运（主机测试走逐字节回退路径）；
- 背光由外部硬件保持 255，测试确认固件在初始化时和整个演示期间从未配置或写入 D6；
- RLE LOGO 展开 52 800 像素（= 240x220）且位于 (0,29)；
- 文字页、5 个纯色、6 条彩条、24 级灰阶的每个像素值；
- 手写笔画、Clear 按钮清屏、Dial 按钮进入拨号画面并点键显示号码；
- 触摸坐标在方向 0/1/2/3 下的换算与抬手判定。

结果图 `test/out/*.png` 可人工核对：`1-logo.png`（LOGO）、`2-text.png`（文字）、
`3-red.png`（纯色）、`4-colorbar.png`（彩条）、`5-grayscale.png`（灰阶）、
`6-handwriting.png`（手写）、`7-cleared.png`（清屏）、`8-dialer.png`（拨号）、
`9-rotation2.png`（横屏触摸换算）。

尚未验证（需要实物）：24 MHz SPI 高速档的波形裕量与最高可用时钟、背光电流与亮度、
触摸方向标定、ST7789 实际显色与 Gamma。首次上电若屏幕出现随机色点或错行，
把 `SCREEN_SPI_HZ` 改回 `12000000UL`（12 MHz 档）。

## 已知限制

- LCD 使用 SPI0 + RA4M1 DTC 硬件搬运像素，保持 8 位数据帧和原始字节顺序；DTC/FSP 初始化
  失败时自动回退到逐字节 `SPI.transfer()` 路径。
- `SCREEN_SPI_HZ` 为 24 MHz 实物测试高速档（`src/screen/boards/uno_r4_wifi.h`），面板侧上限
  写在 `Panel_P024B044P.spi_hz`（24 MHz），实际取两者较小值。RA4M1 外设时钟 PCLKA=48 MHz，
  请求 20 MHz 会被向下分频到 12 MHz，所以直接用 24 MHz 档；出现随机色点/错行时改回
  `12000000UL`。
- 发送缓冲增大到 5760 字节（同时被 RGB565 的 2 字节和 RGB666 的 3 字节整除），单色刷屏只生成
  一次模板，再由 DTC 反复发送。
- 整屏填充 240x320 需要发 153 600 字节，24 MHz 下约 0.05 s。
- 背光为 5 V 经 27 Ω 电阻限流的方案，不是恒流驱动，亮度不代表规格书数值。
- 背光由硬件飞线强制开启；`SCREEN_PIN_BL=-1`，`Screen_Backlight*()` 为空操作，D6 保持高阻。
- ST7789 面板没有回读线，固件无法读取 LCD ID，初始化末尾的显示反相位只能硬编码（当前 0x21）。
- 字符宽度按 `sizey/2`，超出屏幕的字模整字丢弃（不会写出窗口外）。

## 资产来源

- `src/screen/gfx/fonts.cpp`、`fonts.h`：参考工程 `HARDWARE/LCD/lcdfont.h`，只把中文索引从 GBK
  改成 UTF-8 三字节，点阵数据未动。
- `src/screen/assets/logo.h`：参考工程 `HARDWARE/LCD/pic.h` 的 `gImage_logo`（RGB565，数组长度
  105 608 = 8 + 240x220x2），用 `tools/pic2rle.py` 转成 RGB565 RLE。该文件里有
  `gImage_img1/img2/img3/logo` 多个数组，工具按“去掉图像头后长度 == 宽 x 高 x bpp”自动挑选：

```sh
python tools/pic2rle.py <参考工程>/HARDWARE/LCD/pic.h 240 220 src/screen/assets/logo.h --skip 8 --bpp 2
```

  `--bpp 2` 表示输入是 2 字节/像素的 RGB565（默认 3 = RGB888，保持向后兼容），
  `--skip 8` 跳过 Image2Lcd 的 8 字节图像头。
