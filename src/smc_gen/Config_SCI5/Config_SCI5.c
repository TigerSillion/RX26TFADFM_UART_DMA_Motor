/*
* Copyright (c) 2016 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/***********************************************************************************************************************
* File Name        : Config_SCI5.c
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
* Function Name: R_Config_SCI5_Create
* Description  : This function initializes the SCI5 channel
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

void R_Config_SCI5_Create(void)
{
    /* Cancel SCI stop state */
    MSTP(SCI5) = 0U;

    /* Set interrupt priority */
    IPR(SCI5, RXI5) = _0A_SCI_PRIORITY_LEVEL10;
    IPR(SCI5, TXI5) = _0A_SCI_PRIORITY_LEVEL10;

    /* Clear the control register */
    SCI5.SCR.BYTE = 0x00U;

    /* Set TXD5 pin */
    MPC.PB5PFS.BYTE = 0x0AU;
    PORTB.PODR.BYTE |= 0x20U;
    PORTB.PDR.BYTE |= 0x20U;

    /* Set RXD5 pin */
    MPC.PB6PFS.BYTE = 0x0AU;
    PORTB.PMR.BYTE |= 0x40U;

    /* Set clock enable */
    SCI5.SCR.BYTE = _00_SCI_INTERNAL_SCK_UNUSED;

    /* Clear the SIMR1.IICM, SPMR.CKPH, and CKPOL bit, and set SPMR */
    SCI5.SIMR1.BIT.IICM = 0U;
    SCI5.SPMR.BYTE = _00_SCI_RTS | _00_SCI_CLOCK_NOT_INVERTED | _00_SCI_CLOCK_NOT_DELAYED;

    /* Set control registers */
    SCI5.SMR.BYTE = _00_SCI_CLOCK_PCLK | _00_SCI_MULTI_PROCESSOR_DISABLE | _00_SCI_STOP_1 | _00_SCI_PARITY_DISABLE | 
                    _00_SCI_DATA_LENGTH_8 | _00_SCI_ASYNCHRONOUS_OR_I2C_MODE;
    SCI5.SCMR.BYTE = _00_SCI_SERIAL_MODE | _00_SCI_DATA_INVERT_NONE | _00_SCI_DATA_LSB_FIRST | 
                     _10_SCI_DATA_LENGTH_8_OR_7 | _62_SCI_SCMR_DEFAULT;
    SCI5.SEMR.BYTE = _00_SCI_INSTANT_TRANSMIT_DISABLE | _04_SCI_BIT_MODULATION_ENABLE | _00_SCI_DEPEND_BGDM_ABCS | 
                     _00_SCI_16_BASE_CLOCK | _00_SCI_NOISE_FILTER_DISABLE | _40_SCI_BAUDRATE_DOUBLE | 
                     _80_SCI_FALLING_EDGE_START_BIT;
    SCI5.SPTR.BYTE = _00_SCI_IN_SIGNAL_NOT_INVERT | _00_SCI_OUT_SIGNAL_NOT_INVERT | _00_SCI_RECEIVE_TIME_NOT_ADJUST | 
                     _00_SCI_TRANSMIT_TIME_NOT_ADJUST | _03_SCI_SPTR_DEFAULT;

    /* Set bit rate */
    SCI5.BRR = 0x00U;
    SCI5.MDDR = 0x89U;

    /* Set RXD5 signal input select */
    SYSTEM.PRDFR0.BIT.SCI5RXD = 0U;

    R_Config_SCI5_Create_UserInit();
}

/***********************************************************************************************************************
* Function Name: R_Config_SCI5_Start
* Description  : This function starts the SCI5 channel
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

void R_Config_SCI5_Start(void)
{
    /* Clear interrupt flag */
    IR(SCI5, TXI5) = 0U;
    IR(SCI5, RXI5) = 0U;

    /* Enable SCI interrupt */
    IEN(SCI5, TXI5) = 1U;
    EN(SCI5, TEI5) = 1U;
    IEN(SCI5, RXI5) = 1U;
    EN(SCI5, ERI5) = 1U;
}

/***********************************************************************************************************************
* Function Name: R_Config_SCI5_Stop
* Description  : This function stop the SCI5 channel
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/

void R_Config_SCI5_Stop(void)
{
    /* Set TXD5 pin */
    PORTB.PMR.BYTE &= 0xDFU;

    /* Disable serial transmit */
    SCI5.SCR.BIT.TE = 0U;

    /* Disable serial receive */
    SCI5.SCR.BIT.RE = 0U;

    /* Disable SCI interrupt */
    SCI5.SCR.BIT.TIE = 0U;
    SCI5.SCR.BIT.RIE = 0U;
    IEN(SCI5, TXI5) = 0U;
    EN(SCI5, TEI5) = 0U;
    IR(SCI5, TXI5) = 0U;
    IEN(SCI5, RXI5) = 0U;
    EN(SCI5, ERI5) = 0U;
    IR(SCI5, RXI5) = 0U;
}

/***********************************************************************************************************************
* Function Name: R_Config_SCI5_Serial_Receive
* Description  : This function receive SCI5 data
* Arguments    : rx_buf -
*                    receive buffer pointer (Not used when receive data handled by DMAC/DTC)
*                rx_num -
*                    buffer size (Not used when receive data handled by DMAC/DTC)
* Return Value : status -
*                    MD_OK or MD_ARGERROR
***********************************************************************************************************************/

MD_STATUS R_Config_SCI5_Serial_Receive(uint8_t * const rx_buf, uint16_t rx_num)
{
    SCI5.SCR.BYTE |= 0x50U;

    return MD_OK;
}

/***********************************************************************************************************************
* Function Name: R_Config_SCI5_Serial_Send
* Description  : This function transmits SCI5 data
* Arguments    : tx_buf -
*                    transfer buffer pointer (Not used when transmit data handled by DMAC/DTC)
*                tx_num -
*                    buffer size (Not used when transmit data handled by DMAC/DTC)
* Return Value : status -
*                    MD_OK or MD_ARGERROR or MD_ERROR
***********************************************************************************************************************/

MD_STATUS R_Config_SCI5_Serial_Send(uint8_t * const tx_buf, uint16_t tx_num)
{
    SCI5.SCR.BYTE |= 0xA0U;
    /* Set TXD5 pin */
    PORTB.PMR.BYTE |= 0x20U;

    return MD_OK;
}

/* Start user code for adding. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
