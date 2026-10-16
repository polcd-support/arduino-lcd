# arduino-uno-r4-wifi

Arduino UNO R4 WiFi + “Arduino-UNO-R4-WIFI-SPI 通用转接板” Rev 1.3 + P128H002-IPS-CTP
（GC9A01 240x240 IPS + CST816 电容触摸）的点屏与触摸演示。

效果与 STM32 参考工程 `stm32g0b1rc-P128H002-IPS-CTP` 的 `LCD_DEMO()` 一致：

```
LOGO 3s -> 多字号文字 2s -> 全屏纯色 5 色 x 1s -> 彩条 3s -> 灰阶 3s -> 手写画线
```

屏幕相关代码全部隔离在 `src/screen/`，按“板卡 / 面板 / 触摸 / 绘图”分层，换屏或加屏只碰配置和
新增文件。以上流程已用主机端虚拟面板跑通并逐帧核对（见 `test/`）。背光已在板上飞线强制拉高，
固件不配置也不写 D6；LCD 使用 SPI0 24 MHz 实物测试高速档，像素数据由 RA4M1 DTC 硬件搬运。

## 硬件与接线

转接板已把信号固定好，无需外接飞线。依据转接板原理图与 README Rev 1.3：

| 信号 | UNO R4 WiFi | 说明 |
|---|---|---|
| LCD SDA/MOSI | D11 | 经 U1 TXB0108 |
| LCD SCK | D13 | 经 U1 |
| LCD CS | D10 | 经 U1 |
| LCD DC/RS | D9 | 经 U1 |
| LCD RST | D8 | 经 U1 |
| 背光 | 硬件强制拉高 | 已飞线常亮；D6 保持高阻，固件不配置、不输出 |
| D7 | 未连接 | 固件不使用 |
| 触摸 SDA / SCL | A4 / A5 | 走硬件 `Wire`，经 U2 TXS0104E |
| 触摸 RST / INT | D5 / D4 | INT 本驱动轮询，未使用 |

关键点：U1/U2 的 OE 在转接板上固定接 3.3 V，电平转换器始终使能，D7 不参与控制。固件按
**配置所有 UNO 输出脚电平 → `SPI.begin()`/`Wire.begin()` → 面板复位与初始化
→ 触摸复位与初始化 → 绘制首屏** 的顺序启动；D6 全程不参与。

面板参数（与参考工程 `lcd_init.c`/`spi.c` 核对）：

- 控制器/分辨率：GC9A01，240x240；RGB565（`0x3A = 0x05`，2 字节/像素）。
- `0x36` 四方向：`0x08` / `0xC8`（竖屏 0/180）、`0x68` / `0xA8`（横屏 0/180）。
- GRAM 无行列偏移，四个方向 `offset_x/offset_y` 全为 0。
- SPI：参考工程 `SPI1` 为 `CLKPolarity = HIGH` + `CLKPhase = 2EDGE` → **SPI mode 3**；
  本工程固定保留该模式。RA4M1 PCLKA=48 MHz，SPI0 高速档为 24/12 MHz，板级
  `SCREEN_SPI_HZ = 24 MHz`，像素数据由 SPI0 + DTC 硬件搬运，请求 20 MHz 反而会被向下分频到 12 MHz。

## 编译与上传

Arduino IDE 选择 **Arduino UNO R4 WiFi**，打开 `arduino-uno-r4-wifi-P128H002-IPS-CTP.ino` 直接上传。
命令行等价写法：

```sh
arduino-cli compile --fqbn arduino:renesas_uno:unor4wifi .
arduino-cli upload  --fqbn arduino:renesas_uno:unor4wifi -p <串口> .
```

串口 115200 会打印一行自检信息：

```
screen: P128H002-IPS-CTP  panel=P128H002-IPS-CTP (GC9A01 240x240)  240x240  rotation=0  spi=24MHz  bpp=2  tx=DTC actual=24.0MHz  touch=CST816 @0x15
  chip=0xB5
```

`chip` 读不到或不是 0xB5 一类 CST816S/T 值，说明触摸 I2C 没通（检查排线方向与 A4/A5 上拉）。

## 目录结构

```
arduino-uno-r4-wifi-P128H002-IPS-CTP.ino 只做 setup/loop 转发
src/app/demo_app.{h,cpp}       LCD_DEMO() 的状态机与各画面
src/screen/                    屏幕模块（换屏只动这里）
  screen_config.h              选板卡、选屏幕、默认方向、缓冲区大小
  screen.{h,cpp}               对外接口：初始化时序、SPI 像素流水线、旋转、触摸换算
  screen_device.h              数据模型：面板 / 触摸驱动 / 设备
  screen_devices.h             设备注册表（新增屏幕登记处）
  boards/uno_r4_wifi.h         板卡引脚与总线映射
  bus/spi_dma_ra4m1.{h,cpp}    SPI0 + DTC 底层搬运（仅 RA4M1 实物编译，主机测试不参与）
  panels/panel_p128h002_ips_ctp.{h,cpp}   GC9A01 面板描述与初始化表
  touch/touch.h                触摸驱动接口
  touch/touch_cst816.{h,cpp}   CST816S/T（本模组，默认）
  touch/touch_ft6336u.{h,cpp}  FT6336U/FT6236（本屏未选用，留给同款 FPC 排线的其它模组）
  gfx/gfx.{h,cpp}              绘图 API（LCD_*），移植自参考工程 lcd.c
  gfx/fonts.{h,cpp}            ASCII 12/16/24/32 与中文点阵（来自参考工程 lcdfont.h）
  gfx/image.h                  RLE 图像结构
  assets/logo.h                LOGO 资产（RLE，9 716 字节 / 2 429 段）
test/host/                     主机端验证（Arduino 桩件 + 虚拟 GC9A01）
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
2. 触摸若不是 FT6336U/CST816，在 `src/screen/touch/` 加一个实现 `ScreenTouchDriver` 的文件，
   同样用 `#if SCREEN_TOUCH_DRIVER == ...` 包起来。
3. 在 `src/screen/screen_devices.h` 的表里加一行。
4. 在 `src/screen/screen_config.h` 里把 `SCREEN_DEVICE`（和需要时的 `SCREEN_TOUCH_DRIVER`）指过去。
5. 板卡引脚不同就加 `src/screen/boards/xxx.h`，改 `screen_config.h` 里包含的那一个。

面板把颜色按 RGB666 发还是按 RGB565 发，由 `ScreenPanel::pixel_format` 决定，上层代码不用改；
写像素、填充、文字、图像都走同一条 `Screen_WritePixels565()` 流水线。

## 触摸标定

CST816 @ `0x15`（寄存器 `0x01` 手势、`0x02` 触点数、`0x03..0x06` 坐标、`0xA7` 芯片 ID），
驱动只做 I2C 轮询：初始化显式把 `IrqCtl(0xFA)` 写 `0x00` 关闭片内触摸/变化中断输出（四个参考
工程都不写该寄存器），并写 `0xFE = 0x01` 禁止自动休眠；每次采样从 `0x01` 起读 6 字节，
读完即清掉片内事件状态，`0x02` 读到 `0x00`/`0xFF`（睡眠）都算无触摸。不使用转接板 D4/INT。

默认与参考工程一致：原生范围 240x240、不交换、不翻转、无偏移（方向 0 下原生坐标即显示坐标，
参考工程 `TOUCH_OFFSET_Y = 0`，`#if REVERSE` 分支为死代码）。实测方向不对时改
`panels/panel_p128h002_ips_ctp.cpp` 里 `p128h002_touch` 的
`swap_xy`、`invert_x`、`invert_y`、`offset_x`、`offset_y`。
`Screen_TouchReadRaw()` 返回未换算的原生坐标，便于对照标定。
方向 2/3（横屏）的触摸换算是按 MADCTL 的 `MV` 位推出来的，实物核对前请先只在方向 0 下用。

## 与 STM32 参考工程的差异

| 项 | 参考工程 | 本工程 | 原因 |
|---|---|---|---|
| 像素格式 | 16-bit（0x3A=0x05，RGB565） | 相同 | GC9A01 的 4 线 SPI 支持 RGB565 |
| 重复整屏重绘 | 每个循环都重画当前画面 | 进入状态时画一次 | 否则 RLE LOGO/灰阶每几十毫秒重画会卡住触摸，观感相同 |
| LOGO 数据 | `gImage_logo[]` 含 8 字节图像头，`LCD_ShowPicture()` 把它当 4 个像素发出，整图右移 4 像素、末尾 4 像素回卷到左上 | 从第 8 字节起按 200x183 完整显示在 (29,29) | 修正参考工程的偏移，画面内容一致 |
| 文字内容 | `STM32 Display` / `Multi-Size Text` / `浦洋液晶` | `UNO R4 WiFi` / `Multi-Size Text` / `浦洋液晶` | 字号、位置、颜色不变，只换掉板名 |
| 手写画面 | `STATE_GRAYSCALE -> STATE_HANDWRITING` 不调用 `DrawClearButton()`，按钮命中检测被注释掉 | 只清屏画黑底，不画按钮、不做按钮命中 | 参考工程里按钮相关代码本身就是被注释掉的死代码 |
| 未用状态 | `STATE_IMAGE`、`STATE_COUNTDOWN` 不可达 | 未移植 | 参考代码里同样进不去 |
| 纯色整屏填充尺寸 | `LCD_FillRect_FastStatic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ...)` 越界一格（241x241 窗口） | `LCD_Fill()` 只填 0..239 | 修正参考工程的越界写窗口 |
| 手写线段保护 | 跨度超过 50 像素的线段丢弃、贴边点不画 | 相同（`LCD_MAX_SEGMENT`） | 保持与参考工程一致的行为 |
| 中文字库索引 | GBK 两字节 | UTF-8 三字节 | 源码用 UTF-8 直接写中文即可 |

## 验证

```sh
sh test/run.sh
```

`test/` 用桩件替换 Arduino API，把 SPI 字节流喂给虚拟 GC9A01，检查：

- 初始化命令序列共 51 条（参考工程 50 条 + 默认方向 MADCTL 一条），逐条命令码与数据长度；
- `0x3A = 0x05`、`0x11` 后 120 ms 延时、`0x35` 不带数据；
- 背光由外部硬件保持 255，初始化期间与整个演示期间固件从未配置或写入 D6；
- RLE LOGO 展开 36 600 像素（= 200x183），起点 (29,29) 与终点 (228,211) 均为白色，外圈仍为黑；
- 4 个纯色、6 条彩条（x=20/60/100/140/180/220）、24 级灰阶的每个像素值；
- 手写笔画、抬手复位后重按不连线、右下角没有 Clear 按钮；
- 触摸坐标在方向 0/1/2/3 下的换算与抬手判定，CST816 芯片 ID 读回。

结果图 `test/out/*.png` 可人工核对：`1-logo.png`（LOGO）、`2-text.png`（文字）、
`4-colorbar.png`（彩条）、`5-grayscale.png`（灰阶）、`6-handwriting.png`（手写，右下角无按钮）。

尚未验证（需要实物）：SPI/I2C 实际波形与最高可用时钟、背光电流与亮度、触摸方向标定、
GC9A01 实际显色与 Gamma。24 MHz 高速档的波形裕量需上用示波器确认；若面板出现随机色点、
错行或偶发花屏，把 `SCREEN_SPI_HZ` 改回 `12000000UL` 作为回退。

## 已知限制

- GC9A01 是**圆形**面板：GRAM 为 240x240 方阵，但可见区域是直径 240 的圆，
  四角物理上不可见。彩条/灰阶/纯色的四角、以及 (29,29) 的 LOGO 边角会被圆形边框裁掉，
  这只影响观感，不影响显存与测试断言。
- `SCREEN_SPI_HZ` 为 24 MHz（`src/screen/boards/uno_r4_wifi.h`），走 SPI0 + DTC 硬件搬运；
  若面板出现随机色点/错行，改回 `12000000UL`。RA4M1 PCLKA=48 MHz，请求 20 MHz 会被
  向下分频到 12 MHz，因此 24 MHz 才是真实可用高速档。
- 整屏填充 240x240 需要发 115 200 字节，24 MHz 下约 0.038 s。
- 背光为 5 V 经 27 Ω 电阻限流的方案，不是恒流驱动，亮度不代表规格书数值。
- 字符宽度按 `sizey/2`，超出屏幕的字模整字丢弃（不会写出窗口外）。

## 资产来源

- `src/screen/gfx/fonts.cpp`、`fonts.h`：参考工程 `HARDWARE/LCD/lcdfont.h`，只把中文索引从 GBK
  改成 UTF-8 三字节，点阵数据未动。
- `src/screen/assets/logo.h`：参考工程 `HARDWARE/LCD/pic.h` 的 `gImage_logo`
  （73208 字节 = 8 字节图像头 + 200x183x2 字节 RGB565），用 `tools/pic2rle.py`
  的 `--bpp 2` 转成 RLE：

```sh
python tools/pic2rle.py <参考工程>/HARDWARE/LCD/pic.h 200 183 src/screen/assets/logo.h --skip 8 --bpp 2
```

`--bpp` 默认 3（RGB888，给只支持 18-bit 的面板用），`--bpp 2` 按大端 RGB565 直读。
