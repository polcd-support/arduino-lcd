#include "spi_dma_ra4m1.h"

#if defined(ARDUINO_ARCH_RENESAS_UNO)

#include <SPI.h>
#include <IRQManager.h>
#include "r_dtc.h"
#include "r_spi.h"

/* SPI0 发送使用一个 DTC 描述符；接收端在 TX-only 模式下不会产生数据。 */
static spi_instance_ctrl_t s_spi_ctrl;
static spi_cfg_t s_spi_cfg;
static spi_extended_cfg_t s_spi_ext;

static dtc_instance_ctrl_t s_dtc_ctrl;
static transfer_info_t s_dtc_info;
static dtc_extended_cfg_t s_dtc_ext;
static transfer_cfg_t s_dtc_cfg;
static transfer_instance_t s_dtc_instance;

static volatile bool s_done = false;
static volatile spi_event_t s_event = SPI_EVENT_TRANSFER_ABORTED;
static bool s_open = false;
static int32_t s_last_error = 0;
static uint32_t s_actual_hz = 0;

static void spi_dma_callback(spi_callback_args_t *args)
{
    if (!args) return;
    s_event = args->event;
    s_done = true;
}

static void configure_transfer(void)
{
    memset(&s_dtc_ctrl, 0, sizeof(s_dtc_ctrl));
    memset(&s_dtc_info, 0, sizeof(s_dtc_info));
    memset(&s_dtc_ext, 0, sizeof(s_dtc_ext));
    memset(&s_dtc_cfg, 0, sizeof(s_dtc_cfg));
    memset(&s_dtc_instance, 0, sizeof(s_dtc_instance));

    s_dtc_info.transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED;
    s_dtc_info.transfer_settings_word_b.repeat_area = TRANSFER_REPEAT_AREA_SOURCE;
    s_dtc_info.transfer_settings_word_b.irq = TRANSFER_IRQ_END;
    s_dtc_info.transfer_settings_word_b.chain_mode = TRANSFER_CHAIN_MODE_DISABLED;
    s_dtc_info.transfer_settings_word_b.src_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
    s_dtc_info.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE;
    s_dtc_info.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL;
    s_dtc_info.p_src = nullptr;
    s_dtc_info.p_dest = nullptr;       /* R_SPI_Open() 会改成 SPI0 SPDR。 */
    s_dtc_info.num_blocks = 0;
    s_dtc_info.length = 0;

    s_dtc_cfg.p_info = &s_dtc_info;
    s_dtc_cfg.p_extend = &s_dtc_ext;

    s_dtc_instance.p_ctrl = &s_dtc_ctrl;
    s_dtc_instance.p_cfg = &s_dtc_cfg;
    s_dtc_instance.p_api = &g_transfer_on_dtc;
}

static void configure_spi(uint32_t hz, uint8_t mode)
{
    memset(&s_spi_ctrl, 0, sizeof(s_spi_ctrl));
    memset(&s_spi_cfg, 0, sizeof(s_spi_cfg));
    memset(&s_spi_ext, 0, sizeof(s_spi_ext));

    s_spi_cfg.channel = 0;
    s_spi_cfg.operating_mode = SPI_MODE_MASTER;
    s_spi_cfg.clk_phase = (mode == SPI_MODE1 || mode == SPI_MODE3)
                            ? SPI_CLK_PHASE_EDGE_EVEN : SPI_CLK_PHASE_EDGE_ODD;
    s_spi_cfg.clk_polarity = (mode == SPI_MODE2 || mode == SPI_MODE3)
                               ? SPI_CLK_POLARITY_HIGH : SPI_CLK_POLARITY_LOW;
    s_spi_cfg.mode_fault = SPI_MODE_FAULT_ERROR_DISABLE;
    s_spi_cfg.bit_order = SPI_BIT_ORDER_MSB_FIRST;
    s_spi_cfg.p_transfer_tx = &s_dtc_instance;
    s_spi_cfg.p_transfer_rx = nullptr;
    s_spi_cfg.p_callback = spi_dma_callback;
    s_spi_cfg.p_context = nullptr;
    s_spi_cfg.p_extend = &s_spi_ext;
    s_spi_cfg.rxi_irq = FSP_INVALID_VECTOR;
    s_spi_cfg.txi_irq = FSP_INVALID_VECTOR;
    s_spi_cfg.tei_irq = FSP_INVALID_VECTOR;
    s_spi_cfg.eri_irq = FSP_INVALID_VECTOR;
    s_spi_cfg.rxi_ipl = 12;
    s_spi_cfg.txi_ipl = 12;
    s_spi_cfg.tei_ipl = 12;
    s_spi_cfg.eri_ipl = 12;

    s_spi_ext.spi_clksyn = SPI_SSL_MODE_CLK_SYN;
    s_spi_ext.spi_comm = SPI_COMMUNICATION_TRANSMIT_ONLY;
    s_spi_ext.ssl_polarity = SPI_SSLP_LOW;
    s_spi_ext.ssl_select = SPI_SSL_SELECT_SSL0;
    s_spi_ext.mosi_idle = SPI_MOSI_IDLE_VALUE_FIXING_DISABLE;
    s_spi_ext.parity = SPI_PARITY_MODE_DISABLE;
    s_spi_ext.byte_swap = SPI_BYTE_SWAP_DISABLE;
    s_spi_ext.spck_delay = SPI_DELAY_COUNT_1;
    s_spi_ext.ssl_negation_delay = SPI_DELAY_COUNT_1;
    s_spi_ext.next_access_delay = SPI_DELAY_COUNT_1;

    /* 由 FSP 根据当前 PCLKA 计算最接近且不超过请求值的分频。 */
    s_spi_ext.spck_div.spbr = 0;
    s_spi_ext.spck_div.brdv = 0;
    R_SPI_CalculateBitrate(hz, &s_spi_ext.spck_div);

    const uint32_t peripheral_hz = R_FSP_SystemClockHzGet(BSP_FEATURE_SPI_CLK);
    const uint32_t divider = 2UL * ((uint32_t)s_spi_ext.spck_div.spbr + 1UL) *
                             (1UL << s_spi_ext.spck_div.brdv);
    s_actual_hz = divider ? peripheral_hz / divider : 0;
}

bool ScreenSpiDma_Begin(uint32_t hz, uint8_t mode)
{
    if (s_open) return true;

    /* 复用 Arduino core 已验证的引脚选择逻辑，随后释放 SPI0 给 FSP。 */
    SPI.begin();
    SPI.beginTransaction(SPISettings(hz, MSBFIRST, mode));
    SPI.endTransaction();
    SPI.end();

    configure_transfer();
    configure_spi(hz, mode);

    SpiMasterIrqReq_t irq_req = {
        .ctrl = &s_spi_ctrl,
        .cfg = &s_spi_cfg,
        .hw_channel = 0,
    };
    if (!IRQManager::getInstance().addPeripheral(IRQ_SPI_MASTER, &irq_req)) {
        s_last_error = -1;
        return false;
    }

    /* DTC 的触发源必须是 IRQManager 刚为 SPI0 分配的 TXI 向量。 */
    s_dtc_ext.activation_source = s_spi_cfg.txi_irq;

    const fsp_err_t err = R_SPI_Open(&s_spi_ctrl, &s_spi_cfg);
    if (err != FSP_SUCCESS) {
        s_last_error = (int32_t)err;
        return false;
    }

    s_last_error = 0;
    s_open = true;
    return true;
}

bool ScreenSpiDma_Write(const uint8_t *data, uint32_t length)
{
    if (!s_open || !data) return false;
    if (length == 0) return true;

    while (length > 0) {
        /* DTC normal mode单次最多 65536 字节；本项目通常每块 960 字节。 */
        const uint32_t take = length > 65535UL ? 65535UL : length;
        s_done = false;
        s_event = SPI_EVENT_TRANSFER_ABORTED;

        const fsp_err_t err = R_SPI_Write(&s_spi_ctrl, data, take, SPI_BIT_WIDTH_8_BITS);
        if (err != FSP_SUCCESS) {
            s_last_error = (int32_t)err;
            return false;
        }

        /* 20 MHz 下 65535 字节约 26 ms；超时仅用于硬件异常保护。 */
        const uint32_t start = millis();
        while (!s_done) {
            if ((uint32_t)(millis() - start) > 1000UL) {
                s_last_error = -2;
                return false;
            }
            __WFI();
        }
        if (s_event != SPI_EVENT_TRANSFER_COMPLETE) {
            s_last_error = -(100 + (int32_t)s_event);
            return false;
        }

        data += take;
        length -= take;
    }

    s_last_error = 0;
    return true;
}

int32_t ScreenSpiDma_LastError(void)
{
    return s_last_error;
}

uint32_t ScreenSpiDma_ActualHz(void)
{
    return s_actual_hz;
}

#endif
