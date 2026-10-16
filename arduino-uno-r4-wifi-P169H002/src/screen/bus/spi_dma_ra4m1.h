/*
 * Arduino UNO R4 (RA4M1) SPI0 + DTC 发送后端。
 *
 * DTC 是 RA4M1 的硬件数据传输控制器：SPI0 TXI 每次需要新字节时，
 * DTC 自动把内存中的下一个字节写入 SPDR，CPU 不再逐字节轮询。
 */
#ifndef SCREEN_SPI_DMA_RA4M1_H
#define SCREEN_SPI_DMA_RA4M1_H

#include <Arduino.h>

#if defined(ARDUINO_ARCH_RENESAS_UNO)

/* 先由 Arduino SPI 完成 D11/D12/D13 的引脚复用，再切换到 FSP SPI0+DTC。 */
bool ScreenSpiDma_Begin(uint32_t hz, uint8_t mode);

/* 同步发送：返回前数据已经全部移出 SPI，因而调用者可安全拉高 CS。 */
bool ScreenSpiDma_Write(const uint8_t *data, uint32_t length);

/* 最近一次硬件后端的错误码；0 表示成功。 */
int32_t ScreenSpiDma_LastError(void);

/* FSP 分频后的真实 SPI 时钟，可能低于请求值。 */
uint32_t ScreenSpiDma_ActualHz(void);

#endif

#endif
