/*
* Copyright (c) 2016 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/***********************************************************************************************************************
* File Name        : Config_SCI5_user.c
* Component Version: 1.12.0
* Device(s)        : R5F526TFCxFM
* Description      : This file implements device driver for Config_SCI5.
***********************************************************************************************************************/

/***********************************************************************************************************************
Pragma directive
***********************************************************************************************************************/
/* Start user code for pragma. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "r_cg_macrodriver.h"
#include "Config_SCI5.h"
/* Start user code for include. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
#include "r_cg_userdefine.h"

/***********************************************************************************************************************
Global variables and functions
***********************************************************************************************************************/
/* Start user code for global. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
* Function Name: R_Config_SCI5_Create_UserInit
* Description  : This function adds user code after initializing the SCI5 channel
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

void R_Config_SCI5_Create_UserInit(void)
{
    /* Start user code for user init. Do not edit comment generated here */
    /* End user code. Do not edit comment generated here */
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI5_transmit_interrupt
* Description  : This function is TXI5 interrupt service routine
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

#if FAST_INTERRUPT_VECTOR == VECT_SCI5_TXI5
#pragma interrupt r_Config_SCI5_transmit_interrupt(vect=VECT(SCI5,TXI5),fint)
#else
#pragma interrupt r_Config_SCI5_transmit_interrupt(vect=VECT(SCI5,TXI5))
#endif
static void r_Config_SCI5_transmit_interrupt(void)
{
    /*Set bit PSW.I = 1 to allow multiple interrupts*/
    R_BSP_SETPSW_I();

    r_Config_SCI5_callback_transmitend();
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI5_transmitend_interrupt
* Description  : This function is TEI5 interrupt service routine
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

void r_Config_SCI5_transmitend_interrupt(void)
{
    /*Set bit PSW.I = 1 to allow multiple interrupts*/
    R_BSP_SETPSW_I();

    r_Config_SCI5_callback_transmitend();
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI5_receive_interrupt
* Description  : This function is RXI5 interrupt service routine
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

#if FAST_INTERRUPT_VECTOR == VECT_SCI5_RXI5
#pragma interrupt r_Config_SCI5_receive_interrupt(vect=VECT(SCI5,RXI5),fint)
#else
#pragma interrupt r_Config_SCI5_receive_interrupt(vect=VECT(SCI5,RXI5))
#endif
static void r_Config_SCI5_receive_interrupt(void)
{
    /*Set bit PSW.I = 1 to allow multiple interrupts*/
    R_BSP_SETPSW_I();

    r_Config_SCI5_callback_receiveend();
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI5_receiveerror_interrupt
* Description  : This function is ERI5 interrupt service routine
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

void r_Config_SCI5_receiveerror_interrupt(void)
{
    /*Set bit PSW.I = 1 to allow multiple interrupts*/
    R_BSP_SETPSW_I();

    uint8_t err_type;

    r_Config_SCI5_callback_receiveerror();

    /* Clear overrun, framing and parity error flags */
    err_type = SCI5.SSR.BYTE;
    err_type &= 0xC7U;
    err_type |= 0xC0U;
    SCI5.SSR.BYTE = err_type;
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI5_callback_transmitend
* Description  : This function is a callback function when SCI5 finishes transmission
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

static void r_Config_SCI5_callback_transmitend(void)
{
    /* Start user code for r_Config_SCI5_callback_transmitend. Do not edit comment generated here */
    /* USER CODE SAFE: TX completion hook for app layer */
    uart_dma_port_on_tx_end_isr();
    /* End user code. Do not edit comment generated here */
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI5_callback_receiveend
* Description  : This function is a callback function when SCI5 finishes reception
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

static void r_Config_SCI5_callback_receiveend(void)
{
    /* Start user code for r_Config_SCI5_callback_receiveend. Do not edit comment generated here */
    /* USER CODE SAFE: DMAC0 callback owns RX completion handling */
    /* End user code. Do not edit comment generated here */
}

/***********************************************************************************************************************
* Function Name: r_Config_SCI5_callback_receiveerror
* Description  : This function is a callback function when SCI5 reception encounters error
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

static void r_Config_SCI5_callback_receiveerror(void)
{
    /* Start user code for r_Config_SCI5_callback_receiveerror. Do not edit comment generated here */
    /* USER CODE SAFE: RX error hook */
    uart_dma_port_on_rx_error_isr();
    /* End user code. Do not edit comment generated here */
}

/* Start user code for adding. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
