# arduino-uno-r4-wifi

Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3 + P183B001-CTP
（ST7789 240x284 IPS + CST816S/T 电容触摸）的点屏与触摸演示。

效果与 STM32 参考工程 `stm32g0b1rc-P183B001-CTP` 的 `LCD_DEMO()` 一致：

```
LOGO 3s -> 多字号文字 2s -> 全屏纯色 5 色 x 1s -> 彩条 3s -> 灰阶 3s -> 手写画线
```

屏幕相关代码全部隔离在 `src/screen/`，按“板卡 / 面板 / 触摸 / 绘图”分层，换屏或加屏只碰配置和
新增文件。以上流程已用主机端虚拟面板跑通并逐帧核对（见 `test/`）。

背光已在板上飞线强制拉高，固件不配置也不写 D6。LCD 使用 SPI0 24 MHz 实物测试高速档，像素数据由
RA4M1 DTC 硬件搬运。

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

关键点：U1/U2 的 OE 固定接 3.3 V，转换器始终使能。固件按
**LCD 控制脚置为确定电平（复位拉低、CS 拉高）→ `SPI.begin()`/`Wire.begin()`
→ 面板复位与初始化 → 触摸复位与初始化 → 绘制首屏** 的顺序启动；D6 全程不参与。

## 编译与上传

Arduino IDE 选择 **Arduino UNO R4 WiFi**，打开 `arduino-uno-r4-wifi-P183B001-CTP.ino` 直接上传。
命令行等价写法：

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi .
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi -p <串口> .
```

串口 115200 会打印一行自检信息：

```
screen: P183B001-CTP  panel=P183B001-CTP (ST7789 240x284)  240x284  rotation=0  spi=24MHz  bpp=2  touch=CST816 @0x15  tx=DTC actual=24.0MHz
  chip=0xB5
```

`tx=DTC actual=24.0MHz` 表示像素数据已由 RA4M1 DTC 按 8 位帧搬运，实际分频结果就是 24 MHz。
`chip` 读不到或不是 0xB5/0xB6 一类 CST816 值，说明触摸 I2C 没通（检查排线方向与 I2C 波形）。
`SCREEN_DEBUG` 打开（默认）时，手写状态下每次触摸采样还会按参考工程的格式打印 `X,Y`。

## 目录结构

```
arduino-uno-r4-wifi-P183B001-CTP.ino 只做 setup/loop 转发（文件名与草图目录一致）
src/app/demo_app.{h,cpp}       LCD_DEMO() 的状态机与各画面
src/screen/                    屏幕模块（换屏只动这里）
  screen_config.h              选板卡、选屏幕、默认方向、缓冲区大小
  screen.{h,cpp}               对外接口：初始化时序、SPI 像素流水线、旋转、触摸换算
  screen_device.h              数据模型：面板 / 触摸驱动 / 设备
  screen_devices.h             设备注册表（新增屏幕登记处）
  boards/uno_r4_wifi.h         板卡引脚与总线映射
  bus/spi_dma_ra4m1.{h,cpp}    RA4M1 SPI0 + DTC 搬运后端
  panels/panel_p183b001_ctp.{h,cpp}       ST7789 面板描述与初始化表
  touch/touch.h                触摸驱动接口
  touch/touch_cst816.{h,cpp}   CST816S/T（本模组，默认）
  touch/touch_ft6336u.{h,cpp}  兼容 FT6xxx 的其它模组，本屏未选中
  gfx/gfx.{h,cpp}              绘图 API（LCD_*），移植自参考工程 lcd.c
  gfx/fonts.{h,cpp}            ASCII 12/16/24/32 与中文点阵（来自参考工程 lcdfont.h）
  gfx/image.h                  RLE 图像结构
  assets/logo.h                LOGO 资产（RLE，14 088 字节）
test/host/                     主机端验证（Arduino 桩件 + 虚拟 ST7789）
test/run.sh                    编译并运行主机端验证，结果图输出到 test/out/
tools/pic2rle.py               把 Image2Lcd 风格的 C 数组转成 RLE 资产（支持 --bpp 2/3）
```

## 分层约定

- `app` 只调用 `gfx`（`LCD_*`）和 `screen`（`Screen_*`），不碰寄存器。
- `gfx` 颜色统一 RGB565；`screen` 负责换算成面板真正需要的格式（本面板是 RGB565，1:1 直通）。
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
2. 触摸若不是 CST816/FT6xxx，在 `src/screen/touch/` 加一个实现 `ScreenTouchDriver` 的文件，
   同样用 `#if SCREEN_TOUCH_DRIVER == ...` 包起来。
3. 在 `src/screen/screen_devices.h` 的表里加一行。
4. 在 `src/screen/screen_config.h` 里把 `SCREEN_DEVICE`（和需要时的 `SCREEN_TOUCH_DRIVER`）指过去。
5. 板卡引脚不同就加 `src/screen/boards/xxx.h`，改 `screen_config.h` 里包含的那一个。

面板把颜色按 RGB666 发还是按 RGB565 发，由 `ScreenPanel::pixel_format` 决定，上层代码不用改；
写像素、填充、文字、图像都走同一条 `Screen_WritePixels565()` 流水线。

## 触摸标定

CST816 @ `0x15` 只做 I2C 轮询：初始化显式把 `IrqCtl(0xFA)` 写 `0x00` 关闭片内触摸/变化中断输出
（四个参考工程都不写该寄存器），不接也不使用转接板 D4/INT；每次采样从 `0x01` 起读 6 字节
（手势/触点数/坐标），读完即清片内事件状态，`0x02` 为 `0x00`/`0xFF`（睡眠）都算无触摸。

本屏触摸面板原生坐标范围与显示一致（240x284），不交换 X/Y，实物确认 Y 轴需要翻转，
翻转后再叠加参考工程 `CST816.c` 里的 `TOUCH_OFFSET_Y = -10`。负偏移叠加后由
`Screen_TouchRead()` 夹到 `[0, 面板尺寸-1]`，因此原始 Y 靠近下限时显示坐标会被钳到 0。
实测方向不对时改 `panels/panel_p183b001_ctp.cpp` 里 `p183b001_touch` 的
`swap_xy`、`invert_x`、`invert_y`、`offset_x`、`offset_y`。
`Screen_TouchReadRaw()` 返回未换算的原生坐标，便于对照标定。
方向 2/3（横屏）的触摸换算是按 MADCTL 的 `MV` 位推出来的，实物核对前请先只在方向 0 下用。

## 与 STM32 参考工程的差异

| 项 | 参考工程 | 本工程 | 原因 |
|---|---|---|---|
| 像素格式 | 16-bit（0x3A=0x05，RGB565） | 相同 | ST7789 SPI 直接支持 RGB565 |
| 重复整屏重绘 | 每个循环都重画当前画面 | 进入状态时画一次 | 否则 RLE LOGO/灰阶每 10 ms 重画会卡住触摸，观感相同 |
| LOGO 数据 | `gImage_logo[]` 含 8 字节图像头，`LCD_ShowPicture(0,29,239,219,…)` 把图像头当像素发出，且只发 239x219 个像素 | 从第 8 字节起按 240x220 完整显示 @ (0,29) | 修正参考工程的像素数/偏移，画面内容一致 |
| 文字内容 | `STM32 Display` / `Multi-Size Text` / `浦洋液晶` | `UNO R4 WiFi` / `Multi-Size Text` / `浦洋液晶` | 字号、位置、颜色不变，只换掉板名 |
| 清屏按钮 | `DrawClearButton()` 画 80x50 灰底 24 号黑字，命中检测被注释掉 | 相同（只画不响应） | 保持与参考工程一致的行为 |
| 触摸坐标打印 | 每次采样 `printf("%d,%d\n", X, Y)` | 相同，但放在 `#if SCREEN_DEBUG` 内（默认开） | 串口日志可控 |
| 触摸 `REVERSE` 分支 | `#if REVERSE`（值为 0）内含 `Y = 283 - Y` | 根据 P183 实物方向启用等价的 `invert_y` | 修正上下触摸方向 |
| 未用状态 | `STATE_IMAGE`、`STATE_COUNTDOWN` 不可达 | 未移植 | 参考代码里同样进不去 |
| 手写线段保护 | 跨度超过 50 像素的线段丢弃、贴边点不画 | 相同（`LCD_MAX_SEGMENT`） | 保持与参考工程一致的行为 |
| 中文字库索引 | GBK 两字节 | UTF-8 三字节 | 源码用 UTF-8 直接写中文即可 |

## 验证

```sh
sh test/run.sh
```

`test/` 用桩件替换 Arduino API，把 SPI 字节流喂给虚拟 ST7789，检查：

- 初始化命令序列（15 条 + 方向设置 1 条）、各命令数据长度、`0x3A=0x05`、`0x11` 后 120 ms 延时；
- 背光由外部硬件保持 255，测试确认固件从未配置或写入 D6；
- RLE LOGO 展开 52 800 像素（= 240x220）并落在 (0,29)；
- 文字笔画、5 个纯色、6 条彩条（每条 40 px）、24 级灰阶（余数 0）的每个像素值；
- 手写笔画、80x50 的 Clear 按钮已画出，且点按钮不清屏（按钮不响应）；
- 触摸坐标在方向 0/1/2/3 下的换算、Y 轴翻转、`offset_y=-10` 的钳位与抬手判定。

结果图 `test/out/*.png` 可人工核对：`1-logo.png`（LOGO）、`2-text.png`（文字）、
`4-colorbar.png`（彩条）、`5-grayscale.png`（灰阶）、`6-handwriting.png`（手写）、
`7-button.png`（点按钮后画面未被清除）。

尚未验证（需要实物）：24 MHz SPI 高速档的波形裕量、触摸方向标定、ST7789 实际显色与 Gamma。
CST816S/T 目前只以串口读回非零芯片 ID 与真实坐标作为通过依据，未做整机触摸验证。
若实物出现随机色点、错行或偶发花屏，将 `SCREEN_SPI_HZ` 改为 `12000000UL`。

## 已知限制

- 当前不使用 Arduino Renesas core 会切换到 32 位帧的缓冲区 `SPI.transfer()`，而是用
  SPI0 + DTC 保持 8 位数据帧和原始字节顺序。初始化失败会自动回退到逐字节路径。
- RA4M1 PCLKA=48 MHz 下请求 20 MHz 会被 FSP 向下分频到 12 MHz；当前改用 24 MHz 高速档。
  若实物出现随机色点、错行或偶发花屏，将 `SCREEN_SPI_HZ` 改为 `12000000UL`。
- 发送缓冲增大到 5760 字节（按 RGB565 为 2880 像素）；单色刷屏只生成一次模板，以减少
  DTC 启动和 CPU 填充开销。
- 整屏填充 240x284 需要发 136 320 字节，24 MHz 的纯线路传输下限约 45 ms。
- 背光由硬件飞线强制开启；`SCREEN_PIN_BL=-1`，`Screen_Backlight*()` 为空操作，D6 保持高阻。
- 字符宽度按 `sizey/2`，超出屏幕的字模整字丢弃（不会写出窗口外）。
- 参考工程触摸上边缘会被算成负坐标，本工程改为钳到 0，边缘 10 像素内的采样会落在 y=0。

## 资产来源

- `src/screen/gfx/fonts.cpp`、`fonts.h`：参考工程 `HARDWARE/LCD/lcdfont.h`，只把中文索引从 GBK
  改成 UTF-8 三字节，点阵数据未动。
- `src/screen/assets/logo.h`：参考工程 `HARDWARE/LCD/pic.h` 的 `gImage_logo`（8 + 240*220*2 字节，
  数据本身就是 RGB565），用 `tools/pic2rle.py` 转成 RLE：

```sh
python tools/pic2rle.py <参考工程>/HARDWARE/LCD/pic.h 240 220 src/screen/assets/logo.h --skip 8 --bpp 2
```

`tools/pic2rle.py` 的 `--bpp` 默认 3（RGB888，兼容旧用法），`--bpp 2` 时按大端 RGB565 直读。
该参考 `pic.h` 含 4 个数组，工具会按“长度 = 宽*高*bpp + skip”挑选正确的数组。
