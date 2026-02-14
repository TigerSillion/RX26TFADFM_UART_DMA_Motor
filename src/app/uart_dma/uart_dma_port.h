#ifndef UART_DMA_PORT_H
#define UART_DMA_PORT_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/app_types.h"

void uart_dma_port_init(void);
void uart_dma_port_start(void);
void uart_dma_port_stop(void);
md_status_t uart_dma_port_tx_async(const uint8_t *buf, uint16_t len);
bool uart_dma_port_try_get_rx_byte(uint8_t *out);
const uart_dma_diag_t *uart_dma_port_get_diag(void);

void uart_dma_port_on_tx_end_isr(void);
void uart_dma_port_on_rx_end_isr(void);
void uart_dma_port_on_rx_error_isr(void);
void uart_dma_port_bind_dmac_addresses(void);

#endif
