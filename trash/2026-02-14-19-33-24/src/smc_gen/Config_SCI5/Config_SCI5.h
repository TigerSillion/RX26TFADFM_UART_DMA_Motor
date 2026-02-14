/*
* Copyright (c) 2016 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/***********************************************************************************************************************
* File Name        : Config_SCI5.h
* Component Version: 1.12.0
* Device(s)        : R5F526TFCxFM
* Description      : This file implements device driver for Config_SCI5.
***********************************************************************************************************************/

#ifndef CFG_Config_SCI5_H
#define CFG_Config_SCI5_H

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "r_cg_sci.h"

/***********************************************************************************************************************
Macro definitions (Register bit)
***********************************************************************************************************************/

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Global functions
***********************************************************************************************************************/
void R_Config_SCI5_Create(void);
void R_Config_SCI5_Create_UserInit(void);
void R_Config_SCI5_Start(void);
void R_Config_SCI5_Stop(void);
MD_STATUS R_Config_SCI5_Serial_Send(uint8_t * const tx_buf, uint16_t tx_num);
MD_STATUS R_Config_SCI5_Serial_Receive(uint8_t * const rx_buf, uint16_t rx_num);
void r_Config_SCI5_transmitend_interrupt(void);
static void r_Config_SCI5_callback_transmitend(void);
static void r_Config_SCI5_callback_receiveend(void);
void r_Config_SCI5_receiveerror_interrupt(void);
static void r_Config_SCI5_callback_receiveerror(void);
/* Start user code for function. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
#endif
