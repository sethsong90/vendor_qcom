/******************************************************************************
  @file:  gsiff_loc_api_glue.h
  @brief: GNSS / Sensor Interface Framework Support

  DESCRIPTION
    This file defines the module where gsiff interacts with Loc Api.

  INITIALIZATION AND SEQUENCING REQUIREMENTS

  -----------------------------------------------------------------------------
  Copyright (c) 2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------

******************************************************************************/

/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
05/01/11   jb       Initial version
08/01/11   jb       1. Add a way to send messages synchronously over LocAPI

======================================================================*/
#ifndef __GSIFF_LOC_API_GLUE_H__
#define __GSIFF_LOC_API_GLUE_H__

#include <stdbool.h>
#include "loc_api_v02_client.h"

/*===========================================================================
FUNCTION    gsiff_loc_api_open

DESCRIPTION
   Initializes internal structures for loc api module and opens a loc api client.

   msg_q_id: Message Queue id to place time sync data in using gsiff_msg structures.

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool gsiff_loc_api_open(
   int msg_q_id,
   uint64_t event_reg_mask
   );

/*===========================================================================
FUNCTION    gsiff_loc_api_close

DESCRIPTION
   Destroys internal structures for dealing with Loc Api and closes Loc Api Client

DEPENDENCIES
   N/A

RETURN VALUE
   0/FALSE : Failure
   1/TRUE  : Successful

SIDE EFFECTS
   N/A

===========================================================================*/
bool gsiff_loc_api_close();

/*===========================================================================
FUNCTION    gsiff_loc_api_send

DESCRIPTION
   Sends a message over Loc Api. Must be opened before using.

   reqId:      Identifier for the request type in the payload union
   reqPayload: Payload to send over Loc Api.

DEPENDENCIES
   N/A

RETURN VALUE
   locClientStatusEnumType: Look at different possible return codes.

SIDE EFFECTS
   N/A

===========================================================================*/
locClientStatusEnumType gsiff_loc_api_send(
   uint32_t                  reqId,
   locClientReqUnionType*    reqPayload
   );

/*===========================================================================
FUNCTION    gsiff_loc_api_send_sync

DESCRIPTION
   Sends a message over Loc Api. Must be opened before using. Sends
   synchronously for messages that cannot get dropped.

   reqId:           Identifier for the request type in the payload union
   reqPayload:      Payload to send over Loc Api.
   timeout_msec:    Timeout in milliseconds
   ind_id:          ind ID to block for, usually the same as req_id
   ind_payload_ptr: Ind payload received.

DEPENDENCIES
   N/A

RETURN VALUE
   locClientStatusEnumType: Look at different possible return codes.

SIDE EFFECTS
   N/A

===========================================================================*/
locClientStatusEnumType gsiff_loc_api_send_sync(
   uint32_t                  reqId,
   locClientReqUnionType*    reqPayload,
   uint32_t                  timeout_msec,
   uint32_t                  ind_id,          /* ind ID to block for, usually the same as req_id */
   void                      *ind_payload_ptr /* can be NULL*/
   );

/*===========================================================================
FUNCTION    gsiff_loc_api_get_handle

DESCRIPTION
   Retrieves the current handle used. Useful for sending modem start up messages.

DEPENDENCIES
   N/A

RETURN VALUE
   Current loc_api client handle

SIDE EFFECTS
   N/A

===========================================================================*/
locClientHandleType gsiff_loc_api_get_handle();

#endif /* __GSIFF_LOC_API_GLUE_H__*/


