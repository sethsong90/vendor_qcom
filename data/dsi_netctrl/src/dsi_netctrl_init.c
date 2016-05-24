/*!
  @file
  dsi_netctrl_init.c

  @brief
  implements dsi_netctrl initialization

*/

/*===========================================================================

  Copyright (c) 2010-2013 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary and Confidential.

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header:  $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
04/19/10   js      created

===========================================================================*/
#include "dsi_netctrli.h"
#include "dsi_netctrl_mni.h"
#include "dsi_netctrl_platform.h"
#include "dsi_netctrl.h"
#include "dsi_netctrl_cb_thrd.h"
#include "netmgr.h"

#define DSI_INIT_WAIT_TIME_BEFORE_RETRY 500000 /* usec
                                                  time interval between each
                                                  NETLINK req event sent from
                                                  dsi_netctrl
                                                */
#define DSI_INIT_MAX_RETRY_COUNT  120 /* max number of retrying ping msg
                                         to netmgr for its readiness */

/*===========================================================================
  FUNCTION:  dsi_init_netmgr_general
===========================================================================*/
/*!
    @brief
    Initializes netmgr client

    @return
    DSI_SUCCESS
    DSI_ERROR
*/
/*=========================================================================*/
static int dsi_init_netmgr_general()
{
  if (NETMGR_SUCCESS !=
      netmgr_client_register(dsi_netmgr_cb, NULL, &netmgr_hndl))
  {
    return DSI_ERROR;
  }

  return DSI_SUCCESS;
}

/*===========================================================================
  FUNCTION:  dsi_init_query_netmgr
===========================================================================*/
/*!
    @brief
    seperate thread to query netmgr for readiness status. It updates
    dsi_inited accordingly.
    Repeat query periodically if dsi_inited is not TRUE

    @return
    None
*/
/*=========================================================================*/
static void dsi_init_query_netmgr()
{
  unsigned int i = 0;

  /* dsi_inited will be updated once resp nl msg is received */
  while(dsi_inited != DSI_TRUE &&     /* check if dsi_inited has been updated */
        i < DSI_INIT_MAX_RETRY_COUNT )/* stop retrying after reaching max trial number */
  {
    i++;
    DSI_LOG_ERROR("dsi_init_query_netmgr %d time", i);
    if( NETMGR_SUCCESS == netmgr_client_send_ping_msg())
    {
      netmgr_ready_queried = DSI_TRUE;
    }

    usleep(DSI_INIT_WAIT_TIME_BEFORE_RETRY);
  }
}

/*===========================================================================
  FUNCTION:  dsi_ping_thread
===========================================================================*/
/*!
    @brief
    used to start thread for query netmgr readiness status.

    @return
    DSI_ERROR
    DSI_SUCCESS
*/
/*=========================================================================*/
static int dsi_init_ping_thread()
{
  pthread_t inited_thread;

  if(pthread_create(&inited_thread, NULL, (void *)&dsi_init_query_netmgr, NULL))
  {
    DSI_LOG_ERROR("%s", "failed to create dsi_ping_thread\n");
    return DSI_ERROR;
  }
  return DSI_SUCCESS;
}

/* once test mode is supported, populate this vtbl
   with test functions */
dsi_mni_vtable_t dsi_netctrl_mni_vtbl;

/*===========================================================================
  FUNCTION:  dsi_init
===========================================================================*/
/*!
    @brief
    used to initialize dsi netctrl module.

    @return
    DSI_ERROR
    DSI_SUCCESS
*/
/*=========================================================================*/
int dsi_init(int mode)
{
  if(dsi_inited == DSI_TRUE)
  {
    DSI_LOG_ERROR("%s","dsi_init: dsi already inited");
    return DSI_ERROR;
  }

  int ret = DSI_ERROR;
  int reti = DSI_SUCCESS;

  DSI_LOG_DEBUG( "%s", "dsi_netctrl_init: ENTRY" );
  DSI_LOG_DEBUG( "dsi_netctrl_init: RIL instance %d", dsi_get_ril_instance() );

    /* this do..while loop decides the overall return value
     set ret to ERROR at the beginning. set ret to SUCCESS
     at the end. If there was an error in the middle, we break out*/
  do
  {
    ret = DSI_ERROR;

    reti = DSI_SUCCESS;
    switch(mode)
    {
    case DSI_MODE_TEST:
      DSI_LOG_ERROR("%s","not supported test mode");
      break;
    case DSI_MODE_GENERAL:
      DSI_LOG_DEBUG("%s","initializing dsi_netctrl in general mode");
      /* if no vtbl is provided, it's inited in general mode
         by default */
      if (DSI_SUCCESS != dsi_init_internal(NULL))
      {
        reti = DSI_ERROR;
        DSI_LOG_ERROR("%s","dsi_init_internal failed");
      }
      if (DSI_SUCCESS != dsi_init_netmgr_general())
      {
        reti = DSI_ERROR;
        DSI_LOG_ERROR("%s","dsi_init_netmgr_general failed");
      }
      break;
    default:
      reti = DSI_ERROR;
      DSI_LOG_ERROR("%s","not supported default mode");
      break;
    }
    if (reti == DSI_ERROR)
    {
      break;
    }

    ret = DSI_SUCCESS;
  } while (0);

  /* start the thread for send/recv netmgr ready state */
  ret = dsi_init_ping_thread();

  if (ret == DSI_SUCCESS)
  {
    DSI_LOG_DEBUG( "%s", "dsi_netctrl_init: EXIT with suc" );
  }
  else
  {
    DSI_LOG_DEBUG( "%s", "dsi_netctrl_init: EXIT with err" );
  }

  return ret;
}

/*===========================================================================
  FUNCTION:  dsi_init_ex
===========================================================================*/
/*!
  @brief
  used to initialize dsi netctrl module and executing the callback, given as an argument.

  param [in] init mode
  param [in] dsi_init_cb_func
  param [in] dsi_init_cb_data

  @return
  DSI_ERROR
  DSI_SUCCESS
*/
/*=========================================================================*/
int dsi_init_ex
(
  int mode,
  void (* dsi_init_cb_func)( void * ),
  void *dsi_init_cb_data
)
{
  dsi_init_cb_info.cb_func = dsi_init_cb_func;
  dsi_init_cb_info.cb_data = dsi_init_cb_data;

  /* Calling dsi_init with mode argument*/
  return dsi_init(mode);
}
