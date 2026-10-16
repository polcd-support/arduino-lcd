# arduino-uno-r4-wifi

Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3 + P024C135
（ILI9341 240x320 + FT6236 电容触摸）的点屏与触摸演示。

效果与 STM32 参考工程 `stm32g0b1rc-P024C135` 的 `LCD_DEMO()` 一致：

```
多字号文字 2s -> 全屏纯色 5 色 x 1s -> 彩条 3s -> 灰阶 3s -> 手写画线
```

参考工程的上电起始状态就是文字页（`AppState g_state = STATE_TEXT;`），LOGO/图片/倒计时
三个状态没有任何转移指向它们，因此本工程也不移植，直接进入文字页（见“与参考工程的差异”）。

背光已在板上飞线强制拉高，固件不配置也不写 D6。LCD 像素数据由 RA4M1 SPI0 + DTC 硬件搬运；
本 ILI9341 面板请求 10 MHz，PCLKA=48 MHz 分频后实际约 8 MHz。
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

U1/U2 的 OE 固定接 3.3 V，转换器始终使能。固件按
**LCD 控制脚置为确定电平（复位拉低、CS 拉高）→ `SPI.begin()`/`Wire.begin()`
→ 面板复位与初始化 → 触摸复位与初始化 → 绘制首屏** 的顺序启动；D6 全程不参与。

## 编译与上传

Arduino IDE 选择 **Arduino UNO R4 WiFi**，打开 `arduino-uno-r4-wifi-P024C135.ino` 直接上传。
命令行等价写法：

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi .
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi -p <串口> .
```

串口 115200 会打印一行自检信息：

```
screen: P024C135  panel=P024C135 (ILI9341 240x320)  240x320  rotation=0  spi=10MHz  bpp=2  tx=DTC actual=8.0MHz  touch=FT6336U @0x38
  chip=0x64
```

`spi=10MHz` 是固件请求值（受 `Panel_P024C135.spi_hz` 限制），`actual=8.0MHz` 是 FSP 在
PCLKA=48 MHz 下分频后的真实时钟。`chip` 读不到或不是 0x64/0x11 一类 FT6xxx 值，
说明触摸 I2C 没通（检查排线方向与 I2C 波形）。
本模组触摸为 FT6236，与 FT6336U 共用 FT6xxx 寄存器，驱动符号沿用样板命名 `FT6336U`。

## 目录结构

```
arduino-uno-r4-wifi-P024C135.ino 只做 setup/loop 转发（文件名与草图目录一致）
src/app/demo_app.{h,cpp}       LCD_DEMO() 的状态机与各画面
src/screen/                    屏幕模块（换屏只动这里）
  screen_config.h              选板卡、选屏幕、默认方向、缓冲区大小
  screen.{h,cpp}               对外接口：初始化时序、SPI 像素流水线、旋转、触摸换算
  screen_device.h              数据模型：面板 / 触摸驱动 / 设备
  screen_devices.h             设备注册表（新增屏幕登记处）
  boards/uno_r4_wifi.h         板卡引脚与总线映射
  bus/spi_dma_ra4m1.{h,cpp}    RA4M1 SPI0 + DTC 发送后端（仅实物路径编译）
  panels/panel_p024c135.{h,cpp}   ILI9341 面板描述与初始化表
  touch/touch.h                触摸驱动接口
  touch/touch_ft6336u.{h,cpp}  FT6336U（FT6xxx 寄存器，兼容 FT6236，本模组默认）
  touch/touch_cst816.{h,cpp}   CST816S/T（同款 FPC 排线的其它模组）
  gfx/gfx.{h,cpp}              绘图 API（LCD_*），移植自参考工程 lcd.c
  gfx/fonts.{h,cpp}            ASCII 12/16/24/32 与中文点阵（来自参考工程 lcdfont.h）
  gfx/image.h                  RLE 图像结构（本工程未使用，作为绘图库能力保留）
test/host/                     主机端验证（Arduino 桩件 + 虚拟 ILI9341）
test/run.sh                    编译并运行主机端验证，结果图输出到 test/out/
tools/pic2rle.py               把 Image2Lcd 风格的 C 数组转成 RLE 资产
```

## 分层约定

- `app` 只调用 `gfx`（`LCD_*`）和 `screen`（`Screen_*`），不碰寄存器。
- `gfx` 颜色统一 RGB565；`screen` 负责换算成面板真正需要的格式（本面板即 RGB565，2 字节/像素）。
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
实测方向不对时改 `panels/panel_p024c135.cpp` 里 `p024c135_touch` 的
`swap_xy`、`invert_x`、`invert_y`、`offset_x`、`offset_y`。
`Screen_TouchReadRaw()` 返回未换算的原生坐标，便于对照标定。
方向 2/3（横屏）的触摸换算是按 MADCTL 的 `MV` 位推出来的，实物核对前请先只在方向 0 下用。

## 与 STM32 参考工程的差异

| 项 | 参考工程 | 本工程 | 原因 |
|---|---|---|---|
| 像素格式 | 16-bit（0x3A=0x55，RGB565） | 相同 | ILI9341 4 线 SPI 支持 RGB565，2 字节/像素，无需转换 |
| 重复整屏重绘 | 每个循环都重画当前画面 | 进入状态时画一次 | 否则灰阶每 10 ms 重画会卡住触摸，观感相同 |
| 起始状态与未用状态 | 上电即 `STATE_TEXT`；`STATE_LOGO`/`STATE_IMAGE`/`STATE_COUNTDOWN` 不可达 | 未移植这三个状态 | 参考代码里同样进不去（无转移指向），也没有可显示的图片资产 |
| 文字内容 | `STM32 Display` / `Multi-Size Text` / `浦洋液晶` | `UNO R4 WiFi` / `Multi-Size Text` / `浦洋液晶` | 字号、位置、颜色不变，只换掉板名 |
| 手写线段保护 | 跨度超过 50 像素的线段丢弃、贴边点不画 | 相同（`LCD_MAX_SEGMENT`） | 保持与参考工程一致的行为 |
| 手写粗线实现 | `LCD_DrawThickLine()` 里旧的“平行线”版本被注释掉，实际生效的是 Bresenham + `gui_fill_circle(uRow, uCol, size, color)`：每一步画一个半径 `size` 的实心圆（`size=2` → 约 5 像素宽） | 相同（Bresenham 每步画半径 `size` 的实心圆）；参考的 `MAX_ALLOWED_DISTANCE=50` 收进同值的 `LCD_MAX_SEGMENT` | 落笔效果与参考工程一致，只把魔数变成具名常量 |
| 中文字库索引 | GBK 两字节 | UTF-8 三字节 | 源码用 UTF-8 直接写中文即可 |

## 验证

```sh
sh test/run.sh
```

`test/` 用桩件替换 Arduino API，把 SPI 字节流喂给虚拟 ILI9341，检查：

- 初始化命令序列（21 条）、各命令数据长度、`0x3A=0x55`、`0x11` 后 120 ms 延时；
- 背光由外部硬件保持开启，测试确认固件从未配置或写入 D6；
- LCD SPI 请求时钟为 10 MHz（面板上限），FT6336U 配置为阈值 40、轮询模式；
- 上电直接进入文字页（写入 76800 + 文字笔画 = 85984 像素，没有先画 LOGO）；
- 5 个纯色、6 条彩条（条宽 40）、24 级灰阶的每个像素值；
- 手写笔画、Clear 按钮清屏；
- 触摸坐标在方向 0/1/2/3 下的换算（240x320 / 320x240）与抬手判定。

结果图 `test/out/*.png` 可人工核对：`1-text.png`（文字）、`2-red.png`（纯色）、
`3-colorbar.png`（彩条）、`4-grayscale.png`（灰阶）、`5-handwriting.png`（手写）。

尚未验证（需要实物）：SPI/I2C 实际波形与时钟裕量、背光电流与亮度、触摸方向标定、
ILI9341 实际显色与 Gamma。若无实物佐证，不能声称 FT6336U 已通过硬件验证——
只有串口读到非零芯片 ID 并返回真实坐标才算数。

## 已知限制

- 本 ILI9341 面板请求 10 MHz（`Panel_P024C135.spi_hz`），RA4M1 PCLKA=48 MHz 下 FSP 分频后
  实际约 8 MHz。板级 `SCREEN_SPI_HZ` 为 24 MHz 高速档，但被面板上限限制；若实物出现随机色点、
  错行或偶发花屏，将 `SCREEN_SPI_HZ` 改为 `12000000UL` 回退。
- 主机端测试走逐字节回退路径（`tx=byte`）；实物路径用 SPI0 + DTC 保持 8 位帧和原始字节顺序，
  初始化失败会自动回退到逐字节。
- 发送缓冲增大到 5760 字节，静态画面只在进入状态时绘制一次，减少 DTC 启动和 CPU 填充开销。
- 整屏填充 240x320 需要发 153 600 字节，约 8 MHz 下纯线路传输下限约 0.15 s，
  纯色/灰阶画面上能看到自上而下的推进。
- 背光由硬件飞线强制开启；`SCREEN_PIN_BL=-1`，`Screen_Backlight*()` 为空操作，D6 保持高阻。
  背光为 5 V 经 27 Ω 电阻限流的方案（约 67~81 mA），不是恒流驱动，亮度不代表规格书 400 nit。
- 字符宽度按 `sizey/2`，超出屏幕的字模整字丢弃（不会写出窗口外）。

## 资产来源

- `src/screen/gfx/fonts.cpp`、`fonts.h`：参考工程 `HARDWARE/LCD/lcdfont.h`，只把中文索引从 GBK
  改成 UTF-8 三字节，点阵数据未动。
- 本工程 demo 不显示 LOGO，**没有** `src/screen/assets/logo.h`：参考工程的上电起始状态就是文字页，
  LOGO 状态不可达，所以不移植图片资产。`gfx/image.h`、`LCD_ShowImage()`、`tools/pic2rle.py`
  作为绘图库与工具链能力保留。
- 若将来需要显示 LOGO：本族参考工程 `HARDWARE/LCD/pic.h` 的 `gImage_logo` 是 **RGB565**
  （数组长度 = 8 + W*H*2），用 `tools/pic2rle.py --bpp 2 --skip 8` 生成，例如：

```sh
python tools/pic2rle.py <参考工程>/HARDWARE/LCD/pic.h <W> <H> src/screen/assets/logo.h --skip 8 --bpp 2
```
