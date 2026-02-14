#include "uart_dma_port.h"

#include "r_smc_entry.h"

volatile uint8_t g_sci5_rx_byte;
volatile uint8_t g_sci5_rx_flag;
volatile uint8_t g_sci5_tx_done;

static uart_dma_diag_t g_diag_cache;

void uart_dma_port_init(void)
{
    g_sci5_rx_byte = 0U;
    g_sci5_rx_flag = 0U;
    g_sci5_tx_done = 0U;

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

        while (g_sci5_tx_done == 0U)
        {
            /* wait for TEI5 callback */
        }
    }

    return MD_OK;
}

bool uart_dma_port_try_get_rx_byte(uint8_t *out)
{
    if ((out == NULL) || (g_sci5_rx_flag == 0U))
    {
        return false;
    }

    *out = g_sci5_rx_byte;
    g_sci5_rx_flag = 0U;
    return true;
}

const uart_dma_diag_t *uart_dma_port_get_diag(void)
{
    g_diag_cache.rx_byte = g_sci5_rx_byte;
    g_diag_cache.rx_flag = g_sci5_rx_flag;
    g_diag_cache.tx_done = g_sci5_tx_done;
    return &g_diag_cache;
}

void uart_dma_port_on_tx_end_isr(void)
{
    g_sci5_tx_done = 1U;
}

void uart_dma_port_on_rx_end_isr(void)
{
    g_sci5_rx_flag = 1U;

    DMAC0.DMCRA = _00000001_DMAC0_DMCRA_COUNT;
    R_Config_DMAC0_Start();
}

void uart_dma_port_on_rx_error_isr(void)
{
    g_sci5_rx_flag = 0U;
}

void uart_dma_port_bind_dmac_addresses(void)
{
    DMAC0.DMSAR = (void *)&SCI5.RDR;
    DMAC0.DMDAR = (void *)&g_sci5_rx_byte;

    DMAC1.DMDAR = (void *)&SCI5.TDR;
}
