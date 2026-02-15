#include "uart_dma_port.h"

#include "r_smc_entry.h"

#define UART_DMA_TX_WAIT_LIMIT (2000000UL)
#define UART_DMA_MIN_BAUD      (9600UL)
#define UART_DMA_MAX_BAUD      (4000000UL)
#define UART_DMA_RX_FIFO_SIZE  (256U)
#define UART_DMA_BAUD_ERR_MAX_PERMILLE (25UL)

volatile uint8_t g_sci5_rx_byte;
volatile uint8_t g_sci5_rx_flag;
volatile uint8_t g_sci5_tx_done;

static uart_dma_diag_t g_diag_cache;
static uint32_t g_uart_baud = 115200UL;
static volatile uint8_t g_rx_fifo[UART_DMA_RX_FIFO_SIZE];
static volatile uint16_t g_rx_fifo_head;
static volatile uint16_t g_rx_fifo_tail;

typedef struct
{
    uint8_t cks_bits;
    uint8_t semr_bits;
    uint32_t base_div;
} uart_baud_mode_t;

static const uart_baud_mode_t g_baud_modes[] =
{
    { _00_SCI_CLOCK_PCLK,    _40_SCI_BAUDRATE_DOUBLE | _00_SCI_16_BASE_CLOCK | _00_SCI_DEPEND_BGDM_ABCS, 32UL },
    { _00_SCI_CLOCK_PCLK,    _40_SCI_BAUDRATE_DOUBLE | _10_SCI_8_BASE_CLOCK  | _00_SCI_DEPEND_BGDM_ABCS, 16UL },
    { _00_SCI_CLOCK_PCLK,    _00_SCI_BAUDRATE_SINGLE | _00_SCI_16_BASE_CLOCK | _00_SCI_DEPEND_BGDM_ABCS, 64UL },
    { _00_SCI_CLOCK_PCLK,    _00_SCI_BAUDRATE_SINGLE | _10_SCI_8_BASE_CLOCK  | _00_SCI_DEPEND_BGDM_ABCS, 32UL },
    /* ABCSE=1 uses 6-base clock mode; effective divisor is 12 at CKS=PCLK. */
    { _00_SCI_CLOCK_PCLK,    _00_SCI_BAUDRATE_SINGLE | _00_SCI_16_BASE_CLOCK | _08_SCI_6_BASE_CLOCK,      12UL },
    { _01_SCI_CLOCK_PCLK_4,  _00_SCI_BAUDRATE_SINGLE | _00_SCI_16_BASE_CLOCK | _08_SCI_6_BASE_CLOCK,      48UL },
    { _02_SCI_CLOCK_PCLK_16, _00_SCI_BAUDRATE_SINGLE | _00_SCI_16_BASE_CLOCK | _08_SCI_6_BASE_CLOCK,      192UL },
    { _03_SCI_CLOCK_PCLK_64, _00_SCI_BAUDRATE_SINGLE | _00_SCI_16_BASE_CLOCK | _08_SCI_6_BASE_CLOCK,      768UL }
};

void uart_dma_port_init(void)
{
    g_sci5_rx_byte = 0U;
    g_sci5_rx_flag = 0U;
    g_sci5_tx_done = 0U;
    g_diag_cache.rx_count = 0U;
    g_diag_cache.rx_overrun_count = 0U;
    g_diag_cache.tx_count = 0U;
    g_diag_cache.tx_timeout_count = 0U;
    g_diag_cache.rx_error_count = 0U;
    g_uart_baud = 115200UL;
    g_rx_fifo_head = 0U;
    g_rx_fifo_tail = 0U;

    uart_dma_port_bind_dmac_addresses();
}

void uart_dma_port_start(void)
{
    R_Config_SCI5_Serial_Receive(NULL, 0U);
    R_Config_DMAC0_Start();
    R_Config_SCI5_Start();
}

void uart_dma_port_stop(void)
{
    R_Config_DMAC0_Stop();
    R_Config_DMAC1_Stop();
    R_Config_SCI5_Stop();
}

md_status_t uart_dma_port_tx_async(const uint8_t *buf, uint16_t len)
{
    uint16_t i;
    uint32_t wait_count;

    if ((buf == NULL) || (len == 0U))
    {
        return MD_ARGERROR;
    }

    for (i = 0U; i < len; i++)
    {
        g_sci5_tx_done = 0U;

        DMAC1.DMCRA = 1U;
        DMAC1.DMSAR = (void *)&buf[i];
        R_Config_DMAC1_Start();
        R_Config_DMAC1_Set_SoftwareTrigger();

        (void)R_Config_SCI5_Serial_Send(NULL, 1U);

        wait_count = 0U;
        while (g_sci5_tx_done == 0U)
        {
            if (wait_count >= UART_DMA_TX_WAIT_LIMIT)
            {
                g_diag_cache.tx_timeout_count++;
                return MD_ERROR;
            }
            wait_count++;
        }
        g_diag_cache.tx_count++;
    }

    return MD_OK;
}

bool uart_dma_port_try_get_rx_byte(uint8_t *out)
{
    uint16_t tail;

    if (out == NULL)
    {
        return false;
    }

    if (g_rx_fifo_head == g_rx_fifo_tail)
    {
        return false;
    }

    tail = g_rx_fifo_tail;
    *out = g_rx_fifo[tail];
    tail++;
    if (tail >= UART_DMA_RX_FIFO_SIZE)
    {
        tail = 0U;
    }
    g_rx_fifo_tail = tail;
    g_sci5_rx_flag = (g_rx_fifo_head != g_rx_fifo_tail) ? 1U : 0U;
    g_diag_cache.rx_count++;
    return true;
}

const uart_dma_diag_t *uart_dma_port_get_diag(void)
{
    g_diag_cache.rx_byte = g_sci5_rx_byte;
    g_diag_cache.rx_flag = g_sci5_rx_flag;
    g_diag_cache.tx_done = g_sci5_tx_done;
    return &g_diag_cache;
}

void uart_dma_port_reset_diag(void)
{
    g_diag_cache.rx_count = 0U;
    g_diag_cache.rx_overrun_count = 0U;
    g_diag_cache.tx_count = 0U;
    g_diag_cache.tx_timeout_count = 0U;
    g_diag_cache.rx_error_count = 0U;
}

void uart_dma_port_on_tx_end_isr(void)
{
    g_sci5_tx_done = 1U;
}

void uart_dma_port_on_rx_end_isr(void)
{
    uint16_t head;
    uint16_t next_head;

    head = g_rx_fifo_head;
    next_head = (uint16_t)(head + 1U);
    if (next_head >= UART_DMA_RX_FIFO_SIZE)
    {
        next_head = 0U;
    }

    if (next_head == g_rx_fifo_tail)
    {
        g_diag_cache.rx_overrun_count++;
    }
    else
    {
        g_rx_fifo[head] = g_sci5_rx_byte;
        g_rx_fifo_head = next_head;
        g_sci5_rx_flag = 1U;
    }

    DMAC0.DMCRA = _00000001_DMAC0_DMCRA_COUNT;
    R_Config_DMAC0_Start();
}

void uart_dma_port_on_rx_error_isr(void)
{
    g_sci5_rx_flag = 0U;
    g_diag_cache.rx_error_count++;
}

void uart_dma_port_bind_dmac_addresses(void)
{
    DMAC0.DMSAR = (void *)&SCI5.RDR;
    DMAC0.DMDAR = (void *)&g_sci5_rx_byte;

    DMAC1.DMDAR = (void *)&SCI5.TDR;
}

uart_dma_baud_status_t uart_dma_port_set_baud(uint32_t baud)
{
    uint32_t pclk_hz;
    uint32_t i;
    uint32_t brr;
    uint32_t baud_actual;
    uint32_t err_abs;
    uint32_t err_per_mille;
    uint32_t best_err;
    uint32_t best_brr;
    uint8_t best_semr_bits;
    uint8_t best_cks_bits;
    uint8_t semr_base;
    uint8_t smr_base;
    uint8_t found;

    if ((baud < UART_DMA_MIN_BAUD) || (baud > UART_DMA_MAX_BAUD))
    {
        return UART_DMA_BAUD_ARG;
    }

    pclk_hz = (uint32_t)BSP_PCLKA_HZ;
    found = 0U;
    best_err = 0xFFFFFFFFUL;
    best_brr = 0U;
    best_semr_bits = 0U;
    best_cks_bits = 0U;

    for (i = 0U; i < (uint32_t)(sizeof(g_baud_modes) / sizeof(g_baud_modes[0])); i++)
    {
        const uart_baud_mode_t *mode = &g_baud_modes[i];
        if (pclk_hz < (mode->base_div * baud))
        {
            continue;
        }

        brr = (pclk_hz + ((mode->base_div * baud) / 2UL)) / (mode->base_div * baud);
        if (brr == 0UL)
        {
            brr = 1UL;
        }
        brr -= 1UL;
        if (brr > 255UL)
        {
            continue;
        }

        baud_actual = pclk_hz / (mode->base_div * (brr + 1UL));
        if (baud_actual > baud)
        {
            err_abs = baud_actual - baud;
        }
        else
        {
            err_abs = baud - baud_actual;
        }
        err_per_mille = (err_abs * 1000UL) / baud;
        if (err_per_mille < best_err)
        {
            best_err = err_per_mille;
            best_brr = brr;
            best_semr_bits = mode->semr_bits;
            best_cks_bits = mode->cks_bits;
            found = 1U;
        }
    }

    if ((found == 0U) || (best_err > UART_DMA_BAUD_ERR_MAX_PERMILLE))
    {
        return UART_DMA_BAUD_UNSUPPORTED;
    }

    uart_dma_port_stop();

    semr_base = (uint8_t)(SCI5.SEMR.BYTE & (uint8_t)~(_04_SCI_BIT_MODULATION_ENABLE |
                                                     _08_SCI_6_BASE_CLOCK |
                                                     _10_SCI_8_BASE_CLOCK |
                                                     _40_SCI_BAUDRATE_DOUBLE));
    SCI5.SEMR.BYTE = (uint8_t)(semr_base | best_semr_bits | _00_SCI_BIT_MODULATION_DISABLE);

    smr_base = (uint8_t)(SCI5.SMR.BYTE & (uint8_t)~_03_SCI_CLOCK_PCLK_64);
    SCI5.SMR.BYTE = (uint8_t)(smr_base | best_cks_bits);

    SCI5.BRR = (uint8_t)best_brr;
    g_uart_baud = baud;

    uart_dma_port_start();
    return UART_DMA_BAUD_OK;
}

uint32_t uart_dma_port_get_baud(void)
{
    return g_uart_baud;
}
