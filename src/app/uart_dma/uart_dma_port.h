#ifndef UART_DMA_PORT_H
#define UART_DMA_PORT_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/app_types.h"

typedef enum
{
    UART_DMA_BAUD_OK = 0,
    UART_DMA_BAUD_ARG,
    UART_DMA_BAUD_UNSUPPORTED
} uart_dma_baud_status_t;

void uart_dma_port_init(void);
void uart_dma_port_start(void);
void uart_dma_port_stop(void);
md_status_t uart_dma_port_tx_async(const uint8_t *buf, uint16_t len);
bool uart_dma_port_try_get_rx_byte(uint8_t *out);
const uart_dma_diag_t *uart_dma_port_get_diag(void);
void uart_dma_port_reset_diag(void);
uart_dma_baud_status_t uart_dma_port_set_baud(uint32_t baud);
uint32_t uart_dma_port_get_baud(void);

void uart_dma_port_on_tx_end_isr(void);
void uart_dma_port_on_rx_end_isr(void);
void uart_dma_port_on_rx_error_isr(void);
void uart_dma_port_bind_dmac_addresses(void);

#endif
