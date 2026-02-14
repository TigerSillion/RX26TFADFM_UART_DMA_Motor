/*
* Copyright (c) 2016 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/***********************************************************************************************************************
* File Name        : r_cg_userdefine.h
* Version          : 1.0.41
* Device(s)        : R5F526TFCxFM
* Description      : User header file for code generation.
***********************************************************************************************************************/

#ifndef CG_USER_DEF_H
#define CG_USER_DEF_H

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
/* Start user code for include. Do not edit comment generated here */
/* USER CODE SAFE: app-layer UART DMA hook declarations */
#include "../../app/uart_dma/uart_dma_port.h"
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
Macro definitions (Register bit)
***********************************************************************************************************************/
/* Start user code for register. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/
/* Start user code for macro define. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/
/* Start user code for type define. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
Global functions
***********************************************************************************************************************/
/* Start user code for function. Do not edit comment generated here */
/* USER CODE SAFE: shared UART DMA status flags */
extern volatile uint8_t g_sci5_rx_byte;
extern volatile uint8_t g_sci5_rx_flag;
extern volatile uint8_t g_sci5_tx_done;
/* End user code. Do not edit comment generated here */
#endif

