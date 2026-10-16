# P028X101

Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3 + P028X101
（ST7789 240x320 TFT + FT6236 电容触摸）的点屏与触摸演示。

效果与 STM32 参考工程 `stm32g0b1rc-P028X101` 的 `LCD_DEMO()` 一致：

```
LOGO 3s -> 多字号文字 2s -> 全屏纯色 5 色 x 1s -> 彩条 3s -> 灰阶 3s -> 手写画线
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
| 背光 | 硬件强制拉高 | 转接板已飞线把背光常开；D6 保持高阻输入，固件不配置也不驱动 |
| U1/U2 使能 OE | 固定接 3.3 V | 电平转换器始终使能；D7 未连接，固件不使用 |
| 触摸 SDA / SCL | A4 / A5 | 走硬件 `Wire`，经 U2 TXS0104E |
| 触摸 RST / INT | D5 / D4 | INT 本驱动轮询，未使用 |

关键点：U1/U2 的 OE 在转接板上固定接 3.3 V，转换器始终使能。固件顺序为
**配置 LCD 控制脚电平 → `SPI.begin()`/`Wire.begin()` → 面板复位与初始化 → 触摸复位与初始化**；
背光由硬件常开，固件从不配置也从不驱动 D6。`src/screen/screen.cpp` 按这个顺序执行，测试会校验
固件在整个演示期间未操作 D6。

RA4M1 上 LCD 走 **SPI0 + DTC** 硬件搬运（`src/screen/bus/spi_dma_ra4m1.*`）：DTC 是 RA4M1 的
硬件数据传输控制器，SPI0 TXI 每次需要新字节时由它自动写入发送寄存器。PCLKA=48 MHz，SPI0 高速档
为 24/12 MHz，`SCREEN_SPI_HZ` 取 24 MHz（面板侧上限同为 24 MHz）。

## 编译与上传

Arduino IDE 选择 **Arduino UNO R4 WiFi**，打开 `arduino-uno-r4-wifi-P028X101.ino` 直接上传。
命令行等价写法：

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi .
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi -p <串口> .
```

串口 115200 会打印一行自检信息：

```
screen: P028X101  panel=P028X101 (ST7789 240x320)  240x320  rotation=0  spi=24MHz  bpp=2  tx=DTC  actual=24.0MHz  touch=FT6336U @0x38
  chip=0x64
```

`chip` 读不到或不是非零的 FT6xxx 值，说明触摸 I2C 没通（检查排线方向与上拉）。
FT6336U 只有读回非零芯片 ID 且能返回真实坐标时才算实物验证通过，软件仿真不能替代。

## 目录结构

```
arduino-uno-r4-wifi-P028X101.ino 只做 setup/loop 转发
src/app/demo_app.{h,cpp}       LCD_DEMO() 的状态机与各画面
src/screen/                    屏幕模块（换屏只动这里）
  screen_config.h              选板卡、选屏幕、默认方向、缓冲区大小
  screen.{h,cpp}               对外接口：初始化时序、SPI 像素流水线、旋转、触摸换算
  bus/spi_dma_ra4m1.{h,cpp}    UNO R4（RA4M1）专用 SPI0 + DTC 发送后端
  screen_device.h              数据模型：面板 / 触摸驱动 / 设备
  screen_devices.h             设备注册表（新增屏幕登记处）
  boards/uno_r4_wifi.h         板卡引脚与总线映射
  panels/panel_p028x101.{h,cpp}   ST7789 面板描述与初始化表
  touch/touch.h                触摸驱动接口
  touch/touch_ft6336u.{h,cpp}  FT6336U/FT6236（本模组，默认）
  touch/touch_cst816.{h,cpp}   CST816S/T（同款 FPC 排线的其它模组）
  gfx/gfx.{h,cpp}              绘图 API（LCD_*），移植自参考工程 lcd.c
  gfx/fonts.{h,cpp}            ASCII 12/16/24/32 与中文点阵（来自参考工程 lcdfont.h）
  gfx/image.h                  RLE 图像结构
  assets/logo.h                LOGO 资产（RLE，14 088 字节）
test/host/                     主机端验证（Arduino 桩件 + 虚拟 ST7789）
test/run.sh                    编译并运行主机端验证，结果图输出到 test/out/
tools/pic2rle.py               把 Image2Lcd 风格的 C 数组转成 RLE 资产（支持 RGB888/RGB565）
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

面板把颜色按 RGB565 发还是按 RGB666 发，由 `ScreenPanel::pixel_format` 决定，上层代码不用改；
写像素、填充、文字、图像都走同一条 `Screen_WritePixels565()` 流水线。

## 触摸标定

默认与参考工程 FT6236 分支一致：240x320、不交换、不翻转、无偏移（方向 0 下原生坐标即显示坐标）。
实测方向不对时改 `panels/panel_p028x101.cpp` 里 `p028x101_touch` 的
`swap_xy`、`invert_x`、`invert_y`、`offset_x`、`offset_y`。
`Screen_TouchReadRaw()` 返回未换算的原生坐标，便于对照标定。
方向 2/3（横屏）的触摸换算是按 MADCTL 的 `MV` 位推出来的，实物核对前请先只在方向 0 下用。

## 与 STM32 参考工程的差异

| 项 | 参考工程 | 本工程 | 原因 |
|---|---|---|---|
| 像素格式 | 0x3A=0x05（16-bit RGB565） | 相同 | ST7789 SPI 下每像素 2 字节 |
| 初始化末尾的 LCD ID 分支 | `read_lcd_id()` 读到 `0x42C2A9` 写 `0x20`，否则写 `0x21` | 固定写 `0x21` | 转接板未接 MISO，UNO R4 读不到 ID；实况反相就改成 `0x20` |
| MADCTL 四方向 | `0x36` 只硬编码 `0x00`，源码看不出另外三个方向 | 补全为标准 ST7789 值 `0x00/0xC0/0x70/0xA0` | 屏幕要支持旋转；实物核对前请先只在方向 0 下用 |
| 重复整屏重绘 | 每个循环都重画当前画面 | 进入状态时画一次 | 否则 RLE LOGO/灰阶每 10 ms 重画会卡住触摸，观感相同 |
| LOGO 数据 | `LCD_ShowPicture(0,29,239,219,gImage_logo)` 把 8 字节图像头当像素发出，且只发 `239*219` 个像素（窗口是 240x220） | 从第 8 字节起按 240x220 在 (0,29) 完整显示 | 修正参考工程的偏移与计数，画面内容一致 |
| 文字内容 | `STM32 Display` / `Multi-Size Text` / `浦洋液晶` | `UNO R4 WiFi` / `Multi-Size Text` / `浦洋液晶` | 字号、位置、颜色不变，只换掉板名 |
| 全屏纯色填充范围 | `LCD_FillRect_FastStatic(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,...)`（右下各多 1 像素，被裁剪） | `LCD_FillRect(0,0,W-1,H-1,...)` | 结果相同，去掉越界写法 |
| 未用状态 | `STATE_IMAGE`、`STATE_COUNTDOWN` 不可达 | 未移植 | 参考代码里同样进不去 |
| 手写线段保护 | 跨度超过 50 像素的线段丢弃、贴边点不画 | 相同（`LCD_MAX_SEGMENT`） | 保持与参考工程一致的行为 |
| 中文字库索引 | GBK 两字节 | UTF-8 三字节 | 源码用 UTF-8 直接写中文即可 |

## 验证

```sh
sh test/run.sh
```

`test/` 用桩件替换 Arduino API，把 SPI 字节流喂给虚拟 ST7789，检查：

- 初始化命令序列（16 条，含旋转补写的 1 条 0x36）、各命令数据长度、`0x3A=0x05`、`0x11` 后 120 ms 延时、
  `0x21` 在 `0x29` 之前；
- 固件在整个演示期间从未操作 D6（背光由硬件常开，`Sim_BacklightPinTouchCount() == 0`）；
- RLE LOGO 展开 52 800 像素（= 240x220）且位于 (0,29)，5 个纯色、6 条彩条、24 级灰阶的每个像素值；
- 手写笔画、Clear 按钮清屏；
- 触摸坐标在方向 0/1/2/3 下的换算与抬手判定。

结果图 `test/out/*.png` 可人工核对：`1-logo.png`（LOGO）、`2-text.png`（文字）、
`4-colorbar.png`（彩条）、`5-grayscale.png`（灰阶）、`6-handwriting.png`（手写）。

尚未验证（需要实物）：SPI/I2C 实际波形与最高可用时钟、背光电流与亮度、触摸方向标定、
ST7789 实际显色与 Gamma。首次上电若面板出现随机色点/错行，把 `SCREEN_SPI_HZ` 改回 `12000000UL`
再试。FT6336U 只有读回非零芯片 ID 且能返回真实坐标时才算实物验证通过。

## 已知限制

- `SCREEN_SPI_HZ` 为 24 MHz（`src/screen/boards/uno_r4_wifi.h`）。RA4M1 PCLKA=48 MHz，SPI0 高速档
  为 24/12 MHz；面板出现随机色点/错行时改回 `12000000UL`。面板侧上限写在 `Panel_P028X101.spi_hz`
  （24 MHz），实际取两者较小值。
- 整屏填充 240x320 需要发 153 600 字节，24 MHz 下约 0.05 s，纯色/灰阶画面基本看不到推进。
- 背光为 5 V 经限流电阻的方案，不是恒流驱动，亮度不代表规格书标称值。
- 字符宽度按 `sizey/2`，超出屏幕的字模整字丢弃（不会写出窗口外）。

## 资产来源

- `src/screen/gfx/fonts.cpp`、`fonts.h`：参考工程 `HARDWARE/LCD/lcdfont.h`，只把中文索引从 GBK
  改成 UTF-8 三字节，点阵数据未动。
- `src/screen/assets/logo.h`：参考工程 `HARDWARE/LCD/pic.h` 的 `gImage_logo`（本屏的 pic.h 里还有
  `gImage_img1/img2/img3`，参考工程里未显示，本工程未用），用 `tools/pic2rle.py` 转成 RGB565 RLE。
  本屏图像本体是 RGB565（每像素 2 字节），因此给工具加了 `--bpp 2`（默认 3 = RGB888，保持向后兼容）：

```sh
python tools/pic2rle.py <参考工程>/HARDWARE/LCD/pic.h 240 220 src/screen/assets/logo.h --skip 8 --bpp 2
```
