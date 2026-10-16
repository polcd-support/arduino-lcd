# arduino-uno-r4-wifi

Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3 + P154H007
（ST7789P3 240x240 IPS，无触摸）的点屏演示。

效果与 STM32 参考工程 `stm32g0b1rc-P154H007` 的 `LCD_DEMO()` 一致，整段序列无限循环：

```
LOGO 3s -> 多字号文字 2s -> 全屏纯色 5 色 x 1s -> 彩条 3s -> 灰阶 3s -> 回到 LOGO
```

背光已在板上飞线强制拉高，固件不配置也不写 D6。LCD 使用 SPI0 24 MHz 实物测试高速档，像素数据由 RA4M1 DTC 硬件搬运。

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

U1/U2 的 OE 在转接板上固定接使能，转换器始终工作；D7 未连接也不参与控制。固件按
**LCD 控制脚置为确定电平（复位拉低、CS 拉高）→ `SPI.begin()`
→ 面板复位与初始化 → 绘制首屏** 的顺序启动；D6 全程不参与。

P154H007 模组**无触摸**：转接板上的 TP_SDA/TP_CLK/TP_INT/TP_RST（A4/A5/D4/D5）在本工程不配置，
I2C 总线也不再使用。参考工程 `HARDWARE/CST816`、`HARDWARE/FT6236` 目录虽然存在，但 demo
从不调用，本工程同样不使用（触摸驱动代码保留给使用同款转接板的其它模组，见下）。

## 编译与上传

Arduino IDE 选择 **Arduino UNO R4 WiFi**，打开 `arduino-uno-r4-wifi-P154H007.ino` 直接上传。
命令行等价写法：

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi .
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi -p <串口> .
```

串口 115200 会打印一行自检信息：

```
screen: P154H007  panel=P154H007 (ST7789P3 240x240)  240x240  rotation=0  spi=24MHz  bpp=2  tx=DTC  actual=24.0MHz  touch=none
```

`touch=none` 表示该设备未配置触摸（本模组正常）。

## 目录结构

```
arduino-uno-r4-wifi-P154H007.ino 只做 setup/loop 转发（文件名与草图目录一致）
src/app/demo_app.{h,cpp}       LCD_DEMO() 的循环状态机与各画面
src/screen/                    屏幕模块（换屏只动这里）
  screen_config.h              选板卡、选屏幕、触摸驱动、默认方向、缓冲区大小
  screen.{h,cpp}               对外接口：初始化时序、SPI 像素流水线、旋转、触摸换算
  screen_device.h              数据模型：面板 / 触摸驱动 / 设备
  screen_devices.h             设备注册表（新增屏幕登记处）
  bus/spi_dma_ra4m1.{h,cpp}    RA4M1 SPI0 + DTC 硬件搬运后端
  boards/uno_r4_wifi.h         板卡引脚与总线映射
  panels/panel_p154h007.{h,cpp}   ST7789P3 面板描述与初始化表
  gfx/gfx.{h,cpp}              绘图 API（LCD_*），移植自参考工程 lcd.c
  gfx/fonts.{h,cpp}            ASCII 12/16/24/32 与中文点阵（来自参考工程 lcdfont.h）
  gfx/image.h                  RLE 图像结构
  assets/logo.h                LOGO 资产（240x220 RGB565 RLE，14 088 字节）
test/host/                     主机端验证（Arduino 桩件 + 虚拟 ST7789P3）
test/run.sh                    编译并运行主机端验证，结果图输出到 test/out/
tools/pic2rle.py               把 Image2Lcd 风格的 C 数组转成 RLE 资产
```

## 分层约定

- `app` 只调用 `gfx`（`LCD_*`）和 `screen`（`Screen_*`），不碰寄存器。
- `gfx` 颜色统一 RGB565；`screen` 负责换算成面板真正需要的格式（本面板就是 RGB565，直通）。
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
2. 若该屏带触摸，新建 `src/screen/touch/touch_xxx.{h,cpp}`，实现一个 `ScreenTouchDriver`
   （`init` / `read`），用 `#if SCREEN_TOUCH_DRIVER == SCREEN_TOUCH_DRIVER_XXX` 包起来，
   并在 `screen_config.h` 里登记该驱动宏；无触摸的屏把 `ScreenDevice::touch` 设为 `NULL`，不需要该目录。
3. 在 `src/screen/screen_devices.h` 的表里加一行。
4. 在 `src/screen/screen_config.h` 里把 `SCREEN_DEVICE`（和需要时的 `SCREEN_TOUCH_DRIVER`）指过去。
5. 板卡引脚不同就加 `src/screen/boards/xxx.h`，改 `screen_config.h` 里包含的那一个。

面板把颜色按 RGB565 发还是按 RGB666 发，由 `ScreenPanel::pixel_format` 决定，上层代码不用改；
写像素、填充、文字、图像都走同一条 `Screen_WritePixels565()` 流水线。

## 触摸

本模组（P154H007）**无触摸**：`ScreenDevice::touch` 为 `NULL`，`screen_config.h` 选中
`SCREEN_TOUCH_DRIVER_NONE`，没有 `src/screen/touch/` 目录，触摸代码与引脚都不参与编译。因此：

- `Screen_HasTouch()` 返回 `false`，`Screen_TouchRead()` / `Screen_TouchReadRaw()` 恒返回 0；
- `boards/uno_r4_wifi.h` 里 `SCREEN_PIN_TOUCH_RST` / `SCREEN_PIN_TOUCH_INT` 为 `-1`，引脚不配置；
- 测试断言以上行为（不做坐标换算）。

若换用同款转接板上的触摸模组，在 `src/screen/touch/` 加一个实现 `ScreenTouchDriver` 的驱动、
在 `screen_config.h` 里新增并选中对应的 `SCREEN_TOUCH_DRIVER_XXX`，再在
`panels/panel_p154h007.cpp` 里把 `ScreenDevice::touch` 指向该设备的 `ScreenTouchConfig`
（标定方式与参考工程一致：`swap_xy`/`invert_x`/`invert_y`/`offset_x`/`offset_y`）。

## 与 STM32 参考工程的差异

| 项 | 参考工程 | 本工程 | 原因 |
|---|---|---|---|
| 像素格式 | 0x3A=0x05（RGB565） | 相同 | 逐条转写参考初始化序列 |
| MADCTL 四方向 | `0x36` 硬编码 0x00（仅竖屏正向） | 0x00/0xC0/0x70/0xA0 | 参考只用一个方向；其余取 ST7789 标准值，给 `Screen_SetRotation` 用 |
| 初始化序列末尾 | 写 0x2A/0x2B（0..239）与 0x2C | 相同 | 逐条转写，不做增删 |
| 演示结构 | `while(1)` 里跑完整段序列，每个循环重绘当前画面 | 同一段序列做成循环状态机，进入状态时画一次 | 保持样板写法，屏幕观感相同，避免阻塞式重绘 |
| LOGO 数据 | `LCD_ShowPicture(0,10,240,220,gImage_logo+8)`，参考工程此处已跳过 8 字节图像头 | 从第 8 字节起按 240x220 完整显示（(0,10) 起） | 与参考画面一致；RLE 存放节省 Flash |
| 触摸 | demo 为纯显示循环，不响应触摸 | 相同（无触摸设备，无触摸驱动代码） | 参考工程 `HARDWARE/CST816`、`FT6236` 存在但 demo 从不调用 |
| 清屏按钮 | 参考 `lcd.h` 未声明 `DrawClearButton()` | 未移植 | 参考工程没有该函数 |
| 文字内容 | `"STM32 Display"` | `"UNO R4 WiFi"` | 字号、位置、颜色不变，只换掉板名 |
| 中文字库索引 | GBK 两字节 | UTF-8 三字节 | 源码用 UTF-8 直接写中文即可 |

## 验证

```sh
sh test/run.sh
```

`test/` 用桩件替换 Arduino API，把 SPI 字节流喂给虚拟 ST7789P3，检查：

- 初始化命令序列（25 条：24 条面板表 + 1 条方向 0 的 MADCTL）、各命令数据长度、`0x3A=0x05`、`0x11` 后 120 ms 延时、首条 `0xD0` 后（第二条第 `0xD0` 之前）10 ms 延时；
- 背光由外部硬件保持 255，测试确认固件从未配置或写入 D6（初始化后与整个演示结束后都校验）；
- RLE LOGO 展开 52 800 像素（= 240x220），画在 (0,10)，上下各留 10 行黑边；
- 文字页各字符串的笔画像素与位置、4 个纯色、6 条彩条、24 级灰阶递增；
- 序列循环一整圈（16 s）后回到 LOGO 画面；
- `Screen_HasTouch() == false`。

结果图 `test/out/*.png` 可人工核对：`1-logo.png`（LOGO）、`2-text.png`（文字）、
`3-red.png`（纯色）、`4-colorbar.png`（彩条）、`5-grayscale.png`（灰阶）、`6-logo-loop.png`（第二圈 LOGO）。

尚未验证（需要实物）：24 MHz SPI 高速档的波形裕量、背光电流与亮度、ST7789P3 实际显色与 Gamma。
若实物出现随机色点、错行或偶发花屏，把 `SCREEN_SPI_HZ` 改为 `12000000UL`（见下）。

## 已知限制

- `SCREEN_SPI_HZ` 取 24 MHz 高速档（`src/screen/boards/uno_r4_wifi.h`）：RA4M1 PCLKA=48 MHz，
  20 MHz 请求会被 FSP 向下分频到 12 MHz，因此直接开 24 MHz 档，实测由 DTC 搬运保证时序。
  若实物出现随机色点、错行或偶发花屏，将 `SCREEN_SPI_HZ` 改为 `12000000UL` 回退。
- 当前不使用 Arduino Renesas core 会切换到 32 位帧的缓冲区 `SPI.transfer()`，而是用
  SPI0 + DTC 保持 8 位数据帧和 RGB565 原始字节顺序。初始化失败会自动回退到逐字节路径。
- 发送缓冲增大到 5760 字节（6 行 RGB565），图像 RLE 暂存与发送块对齐，窗口参数改为批量发送。
- 整屏填充 240x240 需要发 115 200 字节，24 MHz 下纯线路传输下限约 38 ms。
- 背光由硬件飞线强制开启；`SCREEN_PIN_BL=-1`，`Screen_Backlight*()` 为空操作，D6 保持高阻。
- 字符宽度按 `sizey/2`，超出屏幕的字模整字丢弃（不会写出窗口外）。

## 资产来源

- `src/screen/gfx/fonts.cpp`、`fonts.h`：参考工程 `HARDWARE/LCD/lcdfont.h`，只把中文索引从 GBK
  改成 UTF-8 三字节，点阵数据未动。
- `src/screen/assets/logo.h`：参考工程 `HARDWARE/LCD/pic.h` 的 `gImage_logo`（240x220 RGB565，
  前 8 字节图像头），用 `tools/pic2rle.py` 转成 RGB565 RLE：

```sh
python tools/pic2rle.py <参考工程>/HARDWARE/LCD/pic.h 240 220 src/screen/assets/logo.h --skip 8 --bpp 2
```

`pic.h` 里有 4 个图像数组，工具会按“数据长度 == 宽*高*bpp”自动选中 `gImage_logo`；
`--bpp 2` 表示每像素 2 字节（RGB565，默认 3 为 RGB888，保持向后兼容）。
