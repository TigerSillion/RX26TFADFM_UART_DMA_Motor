/*
* Copyright (c) 2016 - 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/***********************************************************************************************************************
* File Name        : Config_DMAC1.h
* Component Version: 1.8.0
* Device(s)        : R5F526TFCxFM
* Description      : This file implements device driver for Config_DMAC1.
***********************************************************************************************************************/

#ifndef CFG_Config_DMAC1_H
#define CFG_Config_DMAC1_H

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "r_cg_dmac.h"

/***********************************************************************************************************************
Macro definitions (Register bit)
***********************************************************************************************************************/

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/
#define _00000000_DMAC1_SRC_ADDR                                (0x00000000UL) /* Source address */
#define _00000000_DMAC1_DST_ADDR                                (0x00000000UL) /* Destination address */
#define _00000001_DMAC1_DMCRA_COUNT                             (0x00000001UL) /* Transfer count */
#define _0000_DMAC1_SRC_EXT_RPT_AREA                            (0x0000U) /* Source address extended repeat area */
#define _0000_DMAC1_DST_EXT_RPT_AREA                            (0x0000U) /* Destination address extended repeat area */

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Global functions
***********************************************************************************************************************/
void R_Config_DMAC1_Create(void);
void R_Config_DMAC1_Create_UserInit(void);
void R_Config_DMAC1_Start(void);
void R_Config_DMAC1_Stop(void);
void R_Config_DMAC1_Set_SoftwareTrigger(void);
void R_Config_DMAC1_Clear_SoftwareTrigger(void);
/* Start user code for function. Do not edit comment generated here */
/* End user code. Do not edit comment generated here */
#endif
