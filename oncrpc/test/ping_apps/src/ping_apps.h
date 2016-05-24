#ifndef PING_APPS_H
#define PING_APPS_H
/******************************************************************************
  @file  ping_apps.h
  @brief Ping mdm rpc program interface

  DESCRIPTION
    Interface definition for Ping modem program.

  INITIALIZATION AND SEQUENCING REQUIREMENTS


  -----------------------------------------------------------------------------
  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/

/*===========================================================================

                             EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/core/api/rapi/mproc/rel/04.07/inc/ping_apps.h#1 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/10/08     rr    Initial version.
===========================================================================*/

/*============================================================================

             TYPE DEFINITIONS

============================================================================*/
#include "comdef.h"

typedef boolean (ping_apps_data_cb_f_type) (
                               uint32 *data,
                               uint32 size_words, /*~ PARAM data VARRAY LENGTH size_words */
                               uint32 sum
                               );
/*~ CALLBACK ping_apps_data_cb_f_type
    ONERROR return FALSE */

typedef void (ping_apps_cb_f_type) (int val);

unsigned long ping_apps_register(ping_apps_cb_f_type ping_apps_cb_func, int num);
/*~ FUNCTION ping_apps_register RELEASE_FUNC ping_apps_un_register(ping_apps_cb_func) */

unsigned long ping_apps_un_register(ping_apps_cb_f_type ping_apps_cb_func);
/*~ FUNCTION ping_apps_un_register */


uint32 ping_apps_data(
  uint32 *data,
  uint32 size_words /*~ PARAM data VARRAY LENGTH size_words */
  );
/*~ FUNCTION ping_apps_data ONERROR return 0 */



unsigned long ping_apps_un_register_data_cb(ping_apps_data_cb_f_type ping_apps_data_cb_func);
/*~ FUNCTION ping_apps_un_register_data_cb */

unsigned long ping_apps_register_data_cb(ping_apps_data_cb_f_type ping_apps_data_cb_func,
                             uint32 num_callbacks,
                             uint32 size_words,
                             uint32 interval_ms,
                             uint32 num_tasks);
/*~ FUNCTION  ping_apps_register_data_cb RELEASE_FUNC ping_apps_un_register_data_cb(ping_apps_data_cb_func) */




#endif /* PING_APPS_H */

