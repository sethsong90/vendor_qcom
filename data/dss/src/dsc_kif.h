/******************************************************************************

                        D S C _ K I F . H

******************************************************************************/

/******************************************************************************

  @file    dsc_kif.h
  @brief   DSC's Kernel Interface Module header file

  DESCRIPTION
  Header file for Kernel Interface module.

  ---------------------------------------------------------------------------
  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_kif.h#2 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSC_KIF_H__
#define __DSC_KIF_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include "dsci.h"
#include "dsc_dcmi.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant representing integer value used to skip module load during init
---------------------------------------------------------------------------*/
#define DSC_KIF_SKIP 0

/*--------------------------------------------------------------------------- 
   Type representing callback function registered by upper layer to receive
   notification of interface up event
---------------------------------------------------------------------------*/
typedef void (* dsc_kif_opened_f) 
(
    int link, 
    dsc_op_status_t status, 
    void * clnt_hdl
);

/*--------------------------------------------------------------------------- 
   Type representing callback function registered by upper layer to receive
   notification of interface down event
---------------------------------------------------------------------------*/
typedef void (* dsc_kif_closed_f) 
(
    int link, 
    dsc_op_status_t status,
    void * clnt_hdl
);

/*--------------------------------------------------------------------------- 
   Type representing callback function registered by upper layer to receive
   notification of interface reconfigured event
---------------------------------------------------------------------------*/
typedef void (* dsc_kif_reconfigured_f) 
(
    int link, 
    dsc_op_status_t status,
    void * clnt_hdl
);

/*--------------------------------------------------------------------------- 
   Type representing collection of callback functions registered by upper 
   layer to receive event notifications/confirmations
---------------------------------------------------------------------------*/
typedef struct {
    dsc_kif_opened_f opened_cb;
    dsc_kif_closed_f closed_cb;
    dsc_kif_reconfigured_f reconfigured_cb;
} dsc_kif_clntcb_t;

/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_kif_reconfigure
===========================================================================*/
/*!
@brief
  API to reconfigure virtual Ethernet interface for the specified link.

@return
  int - 0 if successful
       -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - DHCP discover is done
*/
/*=========================================================================*/
int dsc_kif_reconfigure (int link);

/*===========================================================================
  FUNCTION  dsc_kif_open
===========================================================================*/
/*!
@brief
  API to bring up virtual Ethernet interface for the specified link. Once
  interface is up, the associated client callback is called. 

@return
  int - 0 if command to bring up interface is successfully issued, 
        -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
int dsc_kif_open (int link, const dsc_kif_clntcb_t * clntcb, void * clnt_hdl);

/*===========================================================================
  FUNCTION  dsc_kif_close
===========================================================================*/
/*!
@brief
  API to bring down virtual Ethernet interface for the specified link. Once
  interface is down, the associated client callback is called. 

@return
  int - 0 if command to bring down interface is successfully issued, 
        -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
int dsc_kif_close (int link);

/*===========================================================================
  FUNCTION  dsc_kif_ioctl
===========================================================================*/
/*!
@brief
  Generic IOCTL handler of the KIF module. 

@return
  int - 0 if IOCTL is successfully processed, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
int dsc_kif_ioctl (int link, dsc_dcm_iface_ioctl_t * ioctl);

/*===========================================================================
  FUNCTION  dsc_kif_init
===========================================================================*/
/*!
@brief
  Initialization routine for the KIF module. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
void 
dsc_kif_init
(

    int nint,
    int links[],
    char * iname, 
    int skip, 
    char * dirpath, 
    char * modscript,
    char * dhcpscript
);

#endif /* __DSC_KIF_H__ */
