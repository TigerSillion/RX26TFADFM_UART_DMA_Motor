#ifndef APP_TYPES_H
#define APP_TYPES_H

#include "r_cg_macrodriver.h"

typedef MD_STATUS md_status_t;

typedef enum
{
    APP_OK = 0,
    APP_ERR_ARG,
    APP_ERR_BUSY,
    APP_ERR_HW
} app_status_t;

typedef struct
{
    volatile uint8_t rx_byte;
    volatile uint8_t rx_flag;
    volatile uint8_t tx_done;
    volatile uint32_t rx_count;
    volatile uint32_t rx_overrun_count;
    volatile uint32_t tx_count;
    volatile uint32_t tx_timeout_count;
    volatile uint32_t rx_error_count;
} uart_dma_diag_t;

#endif
