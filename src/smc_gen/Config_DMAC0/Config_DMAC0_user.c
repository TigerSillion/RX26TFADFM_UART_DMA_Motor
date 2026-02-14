/*
* Copyright (c) 2016 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/***********************************************************************************************************************
* File Name        : Config_DMAC0_user.c
* Component Version: 1.8.0
* Device(s)        : R5F526TFCxFM
* Description      : This file implements device driver for Config_DMAC0.
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
#include "Config_DMAC0.h"
/* Start user code for include. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
#include "r_cg_userdefine.h"

/***********************************************************************************************************************
Global variables and functions
***********************************************************************************************************************/
/* Start user code for global. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */

/***********************************************************************************************************************
* Function Name: R_Config_DMAC0_Create_UserInit
* Description  : This function adds user code after initializing the DMAC0 channel
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

void R_Config_DMAC0_Create_UserInit(void)
{
    /* Start user code for user init. Do not edit comment generated here */
    /* USER CODE SAFE: bind SCI5 RX register and app RX byte buffer */
    DMAC0.DMSAR = (void *)&SCI5.RDR;
    DMAC0.DMDAR = (void *)&g_sci5_rx_byte;
    g_sci5_rx_flag = 0U;
    /* End user code. Do not edit comment generated here */
}

/***********************************************************************************************************************
* Function Name: r_Config_DMAC0_dmac0i_interrupt
* Description  : This function is dmac0i interrupt service routine
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

#if FAST_INTERRUPT_VECTOR == VECT_DMAC_DMAC0I
#pragma interrupt r_Config_DMAC0_dmac0i_interrupt(vect=VECT(DMAC,DMAC0I),fint)
#else
#pragma interrupt r_Config_DMAC0_dmac0i_interrupt(vect=VECT(DMAC,DMAC0I))
#endif
static void r_Config_DMAC0_dmac0i_interrupt(void)
{
    if (DMAC0.DMSTS.BIT.DTIF == 1U)
    {
        DMAC0.DMSTS.BIT.DTIF = 0U;
        r_dmac0_callback_transfer_end();
    }
}

/***********************************************************************************************************************
* Function Name: r_dmac0_callback_transfer_end
* Description  : This function is dmac0 transfer end callback function
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

static void r_dmac0_callback_transfer_end(void)
{
    /* Start user code for r_dmac0_callback_transfer_end. Do not edit comment generated here */
    /* USER CODE SAFE: notify app and re-arm one-byte RX transfer */
    uart_dma_port_on_rx_end_isr();
    /* End user code. Do not edit comment generated here */
}

/* Start user code for adding. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
