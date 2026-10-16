# arduino-uno-r4-wifi

Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3 + P0147B0108
（ST7789P3 BOE 1.47 寸 172x320 IPS，无触摸）的点屏演示。

效果与 STM32 参考工程 `stm32g0b1rc-P0147B0108` 的 `LCD_Display_RunTest()` 一致：

```
彩条 3s -> 灰阶 3s -> 彩条 3s -> ...
```

起始画面为彩条。本模组无触摸，参考工程也不绘制 Clear 按钮，因此没有手写/清屏画面。
以上流程已用主机端虚拟面板跑通并逐帧核对（见 `test/`）。

## 硬件与接线

本模组只有 SPI 点屏，无触摸排线，无需外接飞线。依据转接板原理图与 README Rev 1.3：

| 信号 | UNO R4 WiFi | 说明 |
|---|---|---|
| LCD SDA/MOSI | D11 | 经 U1 TXB0108 |
| LCD SCK | D13 | 经 U1 |
| LCD CS | D10 | 经 U1 |
| LCD DC/RS | D9 | 经 U1 |
| LCD RST | D8 | 经 U1 |
| 背光 | 硬件强制拉高 | D6 保持高阻，固件不配置、不输出（已飞线强制开启） |
| D7 | 未连接 | 固件不使用 |
| 触摸 SDA / SCL / RST / INT | — | 本模组无触摸，`boards/uno_r4_wifi.h` 把 RST/INT 置 -1，A4/A5 不使用 |

关键点：U1/U2 的 OE 在转接板上固定接 3.3 V，电平转换器始终使能，D7 不参与控制；
背光已用飞线强制拉高，D6 全程保持高阻。固件顺序是
**LCD 控制脚置为确定电平（复位拉低、CS 拉高）→ `SPI.begin()`/`Wire.begin()`
→ 面板复位与初始化 → 绘制首屏**，`src/screen/screen.cpp` 严格按这个顺序执行，测试会校验。

## 编译与上传

Arduino IDE 选择 **Arduino UNO R4 WiFi**，打开 `arduino-uno-r4-wifi-P0147B0108.ino` 直接上传。
命令行等价写法：

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi .
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi -p <串口> .
```

串口 115200 会打印一行自检信息（本工程无触摸，`touch=none`）：

```
screen: P0147B0108  panel=P0147B0108 (ST7789P3 172x320)  172x320  rotation=0  spi=24MHz  bpp=2  tx=DTC  actual=24.0MHz  touch=none
```

`tx=DTC actual=24.0MHz` 表示像素由 SPI0 + RA4M1 DTC 硬件搬运，实际时钟 24 MHz
（主机端桩件没有 DTC，会显示 `tx=byte`）。Flash / 静态 RAM 占用待实物编译实测。

## 目录结构

```
arduino-uno-r4-wifi-P0147B0108.ino 只做 setup/loop 转发（文件名与草图目录一致）
src/app/demo_app.{h,cpp}       LCD_Display_RunTest() 的状态机与各画面
src/screen/                    屏幕模块（换屏只动这里）
  screen_config.h              选板卡、选屏幕、默认方向、缓冲区大小
  screen.{h,cpp}               对外接口：初始化时序、SPI 像素流水线、旋转、触摸换算
  screen_device.h              数据模型：面板 / 触摸驱动 / 设备
  screen_devices.h             设备注册表（新增屏幕登记处）
  boards/uno_r4_wifi.h         板卡引脚与总线映射
  bus/spi_dma_ra4m1.{h,cpp}    RA4M1 SPI0 + DTC 硬件搬运后端（逐字节 SPI 的回退见 screen.cpp）
  panels/panel_p0147b0108.{h,cpp}   ST7789P3 面板描述与初始化表
  gfx/gfx.{h,cpp}              绘图 API（LCD_*），移植自参考工程 lcd.c
  gfx/fonts.{h,cpp}            ASCII 12/16/24/32 与中文点阵（来自参考工程 lcdfont.h）
  gfx/image.h                  RLE 图像结构（本工程未放 LOGO 资产）
test/host/                     主机端验证（Arduino 桩件 + 虚拟 ST7789P3）
test/run.sh                    编译并运行主机端验证，结果图输出到 test/out/
tools/pic2rle.py               把 Image2Lcd 风格的 C 数组转成 RLE 资产（工具链能力，本工程未用）
```

## 分层约定

- `app` 只调用 `gfx`（`LCD_*`）和 `screen`（`Screen_*`），不碰寄存器。
- `gfx` 颜色统一 RGB565；`screen` 负责换算成面板真正需要的格式（本面板即 RGB565）。
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
2. 触摸驱动：本工程无触摸，`src/screen/touch/` 已移除。需要触摸时从样板
   `P035M075-IPS-CTP/src/screen/touch/` 拷回，选中的驱动用
   `#if SCREEN_TOUCH_DRIVER == ...` 包起来；无触摸的屏在设备里把 `touch` 置 `NULL`。
3. 在 `src/screen/screen_devices.h` 的表里加一行。
4. 在 `src/screen/screen_config.h` 里把 `SCREEN_DEVICE`（和需要时的 `SCREEN_TOUCH_DRIVER`）指过去。
5. 板卡引脚不同就加 `src/screen/boards/xxx.h`，改 `screen_config.h` 里包含的那一个。

面板把颜色按 RGB565 还是 RGB666 发由 `ScreenPanel::pixel_format` 决定，上层代码不用改；
写像素、填充、文字、图像都走同一条 `Screen_WritePixels565()` 流水线。

## 无触摸

本模组（参考工程 `HARDWARE/CST816` 是空目录）没有触摸：
- `Screen_Device_P0147B0108.touch = NULL`，`Screen_HasTouch()` 返回 `false`，`Screen_TouchRead()` 恒返回 0；
- `boards/uno_r4_wifi.h` 中 `SCREEN_PIN_TOUCH_RST` / `SCREEN_PIN_TOUCH_INT` 为 `-1`，
  `screen.cpp` 与触摸驱动会自动跳过；
- `screen_config.h` 中 `SCREEN_TOUCH_DRIVER = SCREEN_TOUCH_DRIVER_NONE`；本工程不带
  `src/screen/touch/`，换带触摸的模组时从样板拷回驱动并选中即可。

## 与 STM32 参考工程的差异

| 项 | 参考工程 | 本工程 | 原因 |
|---|---|---|---|
| 像素格式 | 16-bit（0x3A=0x05，RGB565） | 相同 | ST7789P3 的 4 线 SPI 直接发 RGB565 |
| 循环重绘 | 每个循环都重画当前画面 | 进入状态时画一次 | 参考每个循环重绘，观感相同且更省 SPI |
| 初始化序列末尾 | `LCD_Init()` 只有 `0x29 + delay_ms(20)`，没有 0x2A/0x2B/0x2C 窗口设置 | 照抄：同样只有 `0x29 + 20 ms` | 逐条转写参考 `lcd_init.c`，未增删命令 |
| 清屏 | `LCD_Display_Init()` 先清屏再开背光 | 相同（`DemoApp_Setup`） | 画面一致 |
| 文字 / LOGO | `LCD_Display_RunTest()` 不显示任何文字或图片 | 未移植 | 参考 demo 只画彩条与灰阶 |
| 触摸 | `lcd_demo.c` 不碰触摸 | 无触摸 | 参考工程 CST816 目录为空 |
| 中文/字库 | `HARDWARE/LCD/lcdfont.h` | 同数据保留在 `gfx/fonts.*` | 绘图库能力保留，本 demo 未用 |

## 验证

```sh
sh test/run.sh
```

`test/` 用桩件替换 Arduino API，把 SPI 字节流喂给虚拟 ST7789P3，检查：

- 初始化命令序列（20 条，含初始化后按默认方向重写的 0x36）、各命令数据长度、
  `0x3A=0x05`、`0x11` 后 120 ms 延时、`0x29` 后 20 ms 延时；
- 背光由外部硬件保持开启（255），初始化后与整个演示期间测试都确认固件从未配置或写入 D6；
- 竖屏 GRAM 列偏移：可见第 0 列写到 GRAM 第 34 列（0x2A = 34..205），GRAM 0..33 列留空；
- setup 清屏写满 172x320 = 55 040 像素；
- 彩条 6 条颜色与 6 个分界 x（0..27 / 28..56 / 57..85 / 86..113 / 114..142 / 143..171）；
- 灰阶 24 级首格黑、末格白、逐格递增、余数 4 像素补在最后一格；
- 起始状态为彩条，3 s 后切灰阶、6 s 后切回彩条；
- `Screen_HasTouch() == false`。

结果图 `test/out/*.png` 可人工核对：`1-colorbars.png`（彩条）、`2-grayscale.png`（灰阶）、
`3-colorbars-again.png`（切回彩条）。

尚未验证（需要实物）：24 MHz SPI 高速档的波形裕量、面板显色与 Gamma、GRAM 偏移的实际观感。
若实物出现随机色点、错行或偶发花屏，把 `SCREEN_SPI_HZ` 改回 `12000000UL`（12 MHz 回退档）再测。

## 已知限制

- 当前不使用 Arduino Renesas core 会切换到 32 位帧的缓冲区 `SPI.transfer()`，而是用
  SPI0 + DTC 保持 8 位数据帧和原始字节顺序。初始化失败会自动回退到逐字节路径。
- RA4M1 48 MHz 外设时钟下，请求 20 MHz 会被 FSP 向下分频到 12 MHz；当前改用 24 MHz 高速档
  （`SCREEN_SPI_HZ`，面板侧上限写在 `Panel_P0147B0108.spi_hz` 同取 24 MHz）。若实物出现随机
  色点、错行或偶发花屏，将 `SCREEN_SPI_HZ` 改为 `12000000UL`。
- 发送缓冲增大到 5760 字节（2880 像素 x 2 字节），窗口参数改为批量发送；
  单色刷屏只生成一次模板，以减少 DTC 启动和 CPU 填充开销。
- 整屏填充 172x320 需要发 110 080 字节，24 MHz 的纯线路传输下限约 37 ms。
- 背光由硬件飞线强制开启；`SCREEN_PIN_BL=-1`，`Screen_Backlight*()` 为空操作，D6 保持高阻。
- 本 demo 每 3 s 切换一次画面，切换时整屏重绘。
- 本屏无触摸，无法实现手写/清屏。
- 字符宽度按 `sizey/2`，超出屏幕的字模整字丢弃（不会写出窗口外）。

## 资产来源

- `src/screen/gfx/fonts.cpp`、`fonts.h`：参考工程 `HARDWARE/LCD/lcdfont.h`，只把中文索引从 GBK
  改成 UTF-8 三字节，点阵数据未动（本 demo 未使用，绘图库能力保留）。
- 本工程 demo 不显示 LOGO，因此没有 `src/screen/assets/logo.h`。
- `tools/pic2rle.py` 保留：如需给本屏加图像资产，可用它把 Image2Lcd 的 C 数组转成 RLE。
  本族 `pic.h` 的 `gImage_logo` 为 RGB565（数组长度 = 8 + W*H*2），当前工具只支持 RGB888
  3 字节/像素；若要用，需要先给工具加 `--bpp 2` 分支。
