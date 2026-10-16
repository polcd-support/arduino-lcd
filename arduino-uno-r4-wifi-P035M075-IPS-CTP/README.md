# arduino-uno-r4-wifi

Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3 + P035M075-IPS-CTP
（ILI9488 320x480 IPS + FT6336U 电容触摸）的点屏与触摸演示。

当前固件已恢复为 STM32 `lcd_demo` 演示流程：

```
LOGO -> 文字 -> 红/绿/蓝/白/黑 -> 8 色彩条 -> 24 级灰阶 -> 触摸手写
```

背光已在板上飞线强制拉高，固件不配置也不写 D6。LCD 使用 SPI0 24 MHz 实物测试高速档，像素数据由 RA4M1 DTC 硬件搬运。
演示播放期间轻触任意位置可直接进入手写页，右下角的 `Clear` 按钮可清屏。
屏幕代码仍隔离在 `src/screen/`，主机端虚拟面板测试见 `test/`。

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

U1/U2 的 OE 固定接 3.3 V，转换器始终使能。固件按
**LCD 控制脚置为确定电平（复位拉低、CS 拉高）→ `SPI.begin()`/`Wire.begin()`
→ 面板复位与初始化 → 触摸复位与初始化 → 绘制首屏** 的顺序启动；D6 全程不参与。

## 编译与上传

Arduino IDE 选择 **Arduino UNO R4 WiFi**，打开 `arduino-uno-r4-wifi-P035M075-IPS-CTP.ino` 直接上传。
命令行等价写法：

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi .
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi -p <串口> .
```

当前演示程序实测：Flash 96 696 字节（36%），静态 RAM 12 940 字节（39%）。

串口 115200 会打印一行自检信息：

```
  chip=0x64
screen: P035M075-IPS-CTP  panel=P035M075-IPS-CTP (ILI9488 320x480)  320x480  rotation=0  spi=20MHz  bpp=3  touch=FT6336U @0x38
```

`chip` 读不到或不是 0x64/0x11 一类 FT6xxx 值，说明触摸 I2C 没通（检查排线方向与 I2C 波形）。
触摸时串口每 100 ms 输出一次换算后的坐标，例如 `touch: x=120 y=240`。

## 目录结构

```
arduino-uno-r4-wifi-P035M075-IPS-CTP.ino 只做 setup/loop 转发（文件名与草图目录一致）
  src/app/demo_app.{h,cpp}       LOGO/文字/颜色/灰阶/触摸手写状态机
src/screen/                    屏幕模块（换屏只动这里）
  screen_config.h              选板卡、选屏幕、默认方向、缓冲区大小
  screen.{h,cpp}               对外接口：初始化时序、SPI 像素流水线、旋转、触摸换算
  screen_device.h              数据模型：面板 / 触摸驱动 / 设备
  screen_devices.h             设备注册表（新增屏幕登记处）
  boards/uno_r4_wifi.h         板卡引脚与总线映射
  panels/panel_p035m075_ips_ctp.{h,cpp}   ILI9488 面板描述与初始化表
  touch/touch.h                触摸驱动接口
  touch/touch_ft6336u.{h,cpp}  FT6336U/FT6236（本模组，默认）
  touch/touch_cst816.{h,cpp}   CST816S/T（同款 FPC 排线的其它模组）
  gfx/gfx.{h,cpp}              绘图 API（LCD_*），移植自参考工程 lcd.c
  gfx/fonts.{h,cpp}            ASCII 12/16/24/32 与中文点阵（来自参考工程 lcdfont.h）
  gfx/image.h                  RLE 图像结构
  assets/logo.h                LOGO 资产（RLE，14 148 字节）
test/host/                     主机端验证（Arduino 桩件 + 虚拟 ILI9488）
test/run.sh                    编译并运行主机端验证，结果图输出到 test/out/
tools/pic2rle.py               把 Image2Lcd 风格的 C 数组转成 RLE 资产
```

## 分层约定

- `app` 只调用 `gfx`（`LCD_*`）和 `screen`（`Screen_*`），不碰寄存器。
- `gfx` 颜色统一 RGB565；`screen` 负责换算成面板真正需要的格式（本面板是 RGB666）。
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

默认与参考工程一致：320x480、不交换、不翻转、无偏移（方向 0 下原生坐标即显示坐标）。
FT6336U 使用 100 kHz I²C 轮询模式，阈值为 `SCREEN_TOUCH_THRESHOLD=40`；数值越低越灵敏。
实测方向不对时改 `panels/panel_p035m075_ips_ctp.cpp` 里 `p035m075_touch` 的
`swap_xy`、`invert_x`、`invert_y`、`offset_x`、`offset_y`。
`Screen_TouchReadRaw()` 返回未换算的原生坐标，便于对照标定。
方向 2/3（横屏）的触摸换算是按 MADCTL 的 `MV` 位推出来的，实物核对前请先只在方向 0 下用。

## 与 STM32 参考工程的差异

| 项 | 参考工程 | 本工程 | 原因 |
|---|---|---|---|
| 像素格式 | 18-bit（0x3A=0x66） | 相同 | ILI9488 的 4 线 SPI 只有 18-bit，不支持 RGB565 |
| 重复整屏重绘 | 每个循环都重画当前画面 | 进入状态时画一次 | 否则 RLE LOGO/灰阶每 10 ms 重画会卡住触摸，观感相同 |
| LOGO 数据 | `gImage_logo[]` 含 8 字节图像头，`LCD_ShowPicture()` 把它当像素发出，底部少约 5 行 | 从第 8 字节起按 260x238 完整显示 | 修正参考工程的偏移，画面内容一致 |
| 文字内容 | `STM32 Display` / `Multi-Size Text` / `浦洋液晶` | `UNO R4 WiFi` / `Multi-Size Text` / `浦洋液晶` | 字号、位置、颜色不变，只换掉板名 |
| 未用状态 | `STATE_IMAGE`、`STATE_COUNTDOWN` 不可达 | 未移植 | 参考代码里同样进不去 |
| 手写线段保护 | 跨度超过 50 像素的线段丢弃、贴边点不画 | 相同（`LCD_MAX_SEGMENT`） | 保持与参考工程一致的行为 |
| 中文字库索引 | GBK 两字节 | UTF-8 三字节 | 源码用 UTF-8 直接写中文即可 |

## 验证

```sh
sh test/run.sh
```

`test/` 用桩件替换 Arduino API，把 SPI 字节流喂给虚拟 ILI9488，检查：

- 初始化命令序列、各命令数据长度、`0x3A=0x66`、`0x11` 后 120 ms 延时；
- 背光由外部硬件保持开启，测试确认固件从未配置或写入 D6；
- LCD SPI 请求时钟为 24 MHz，FT6336U 配置为阈值 40、轮询模式；
- 260x238 LOGO、英文/中文文字和红绿蓝白黑整屏画面；
- 8 色彩条、24 级灰阶、触摸笔画以及 `Clear` 清屏；
- 静态画面只在进入状态时绘制一次，显示计时从画面绘制完成后开始。

各阶段结果图输出为 `test/out/1-logo.png` 到 `test/out/5-handwriting.png`。

尚未验证（需要实物）：24 MHz SPI 高速档的波形裕量、触摸方向标定、ILI9488 实际显色与 Gamma。

## 已知限制

- 当前不使用 Arduino Renesas core 会切换到 32 位帧的缓冲区 `SPI.transfer()`，而是用
  SPI0 + DTC 保持 8 位数据帧和 RGB666 原始字节顺序。初始化失败会自动回退到逐字节路径。
- RA4M1 48 MHz 外设时钟下，请求 20 MHz 会被 FSP 向下分频到 12 MHz；当前改用 24 MHz 高速档。
  若实物出现随机色点、错行或偶发花屏，将 `SCREEN_SPI_HZ` 改为 `12000000UL`。
- 发送缓冲增大到 5760 字节（6 行），Logo RLE 暂存与发送块对齐，窗口参数改为批量发送；
  单色刷屏只生成一次模板，以减少 DTC 启动和 CPU 填充开销。
- 整屏填充 320x480 需要发送 460 800 字节，24 MHz 的纯线路传输下限约 154 ms。
- 背光由硬件飞线强制开启；`SCREEN_PIN_BL=-1`，`Screen_Backlight*()` 为空操作，D6 保持高阻。
- 字符宽度按 `sizey/2`，超出屏幕的字模整字丢弃（不会写出窗口外）。

## 资产来源

- `src/screen/gfx/fonts.cpp`、`fonts.h`：参考工程 `HARDWARE/LCD/lcdfont.h`，只把中文索引从 GBK
  改成 UTF-8 三字节，点阵数据未动。
- `src/screen/assets/logo.h`：参考工程 `HARDWARE/LCD/pic.h` 的 `gImage_logo`，
  用 `tools/pic2rle.py` 转成 RGB565 RLE：

```sh
python tools/pic2rle.py <参考工程>/HARDWARE/LCD/pic.h 260 238 src/screen/assets/logo.h --skip 8
```
