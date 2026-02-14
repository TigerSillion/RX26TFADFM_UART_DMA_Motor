#include "uart_dma_demo.h"

#include "uart_dma_port.h"

void uart_dma_demo_run_forever(void)
{
    uint8_t rx_byte;

    uart_dma_port_init();
    uart_dma_port_start();

    while (1)
    {
        if (uart_dma_port_try_get_rx_byte(&rx_byte))
        {
            (void)uart_dma_port_tx_async(&rx_byte, 1U);
        }
    }
}
