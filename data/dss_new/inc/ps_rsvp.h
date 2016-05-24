#ifndef PS_RSVP_H
#define PS_RSVP_H
/*===========================================================================
   R E S O U R C E   R E S E R V A T I O N   P R O T O C O L (RSVP)
                       A P I   H E A D E R   F I L E

DESCRIPTION
 The RSVP API header file.


Copyright (c) 2005-2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_rsvp.h#1 $ $DateTime: 2011/01/10 09:44:56 $ $Author: maldinge $


when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/14/08    pp     Common Modem Interface: Public/Private API split.
07/18/08    dm     Fixed compiler warnings
04/03.07    ks     Support REG_FILTER and DEREG_FILTER options.
11/29/05    sv     Created module.

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "dsm.h"
#include "dssocket_defs.h"
#include "dss_netpolicy.h"


/*===========================================================================

                          PUBLIC DATA DECLARATIONS

===========================================================================*/
#define PS_RSVP_LOCAL_MSGID 0
#define PS_RSVP_MAX_FILTERS 1

/*---------------------------------------------------------------------------
  RSVP Message type
---------------------------------------------------------------------------*/
typedef enum
{
  PS_RSVP_MIN_MSG       = 0,
  PS_RSVP_PATH_MSG      = 1,
  PS_RSVP_RESV_MSG      = 2,
  PS_RSVP_PATH_ERR_MSG  = 3,
  PS_RSVP_RESV_ERR_MSG  = 4,
  PS_RSVP_PATH_TEAR_MSG = 5,
  PS_RSVP_RESV_TEAR_MSG = 6,
  PS_RSVP_RESV_CONF_MSG = 7,
  PS_RSVP_MAX_MSG

} ps_rsvp_msg_enum_type;

/*---------------------------------------------------------------------------
  RSVP object enum type
---------------------------------------------------------------------------*/
typedef enum
{
  PS_RSVP_TIME_VALUE_OBJ      = 0x1,
  PS_RSVP_SENDER_TEMPLATE_OBJ = 0x2,
  PS_RSVP_SENDER_TSPEC_OBJ    = 0x4,
  PS_RSVP_ADSPEC_OBJ          = 0x8,
  PS_RSVP_RESV_CONF_OBJ       = 0x10,
  PS_RSVP_POLICY_OBJ          = 0x20,
  PS_RSVP_STYLE_OBJ           = 0x40,
  PS_RSVP_FLOW_DESCRIPTOR_OBJ = 0x80,
  PS_RSVP_SCOPE_OBJ           = 0x100,
  PS_RSVP_ERROR_SPEC_OBJ      = 0x200,
  PS_RSVP_OPAQUE_OBJ          = 0x400,
  PS_RSVP_SESSION_OBJ         = 0x800
} ps_rsvp_obj_enum_type;

/*---------------------------------------------------------------------------
  RSVP object data types
---------------------------------------------------------------------------*/
typedef  uint32    ps_rsvp_time_value_obj_type;       /* Time value object */

typedef  boolean ps_rsvp_resv_conf_obj_type;      /* RESV_CONF object type */

typedef struct
{
  char    * policy_data;
  int32     len;
} ps_rsvp_policy_obj_type;                       /* Opaque policy type */

typedef enum
{
  PS_RSVP_STYLE_WF  = 0x11,
  PS_RSVP_STYLE_FF  = 0xA,
  PS_RSVP_STYLE_SE  = 0x12,
  PS_RSVP_STYLE_MAX = 0x1F
} ps_rsvp_style_obj_enum_type;                        /* Style Object type */

typedef struct
{
  struct ps_in6_addr  error_node;
  uint8               flags;
  uint8               error_code;
  uint16              error_value;
} ps_rsvp_error_spec_obj_type;

typedef struct
{
  char    * buffer;
  uint16    buf_len;
} ps_rsvp_opaque_obj_type; /* Opaque objects to be appended to the msgs */

/*---------------------------------------------------------------------------
  RSVP session options.
---------------------------------------------------------------------------*/
typedef  enum
{
  PS_RSVP_TTL_OPT,
  PS_RSVP_PROTOCOL_OPT,
  PS_RSVP_RFC_COMPLIANT,
  PS_RSVP_REG_FILTER,
  PS_RSVP_DEREG_FILTER
} ps_rsvp_option_enum_type;

/*---------------------------------------------------------------------------
  RSVP option type
---------------------------------------------------------------------------*/
typedef struct
{
  int32                     protocol;
  uint16                    port;
  dss_net_policy_info_type  policy_info;
} ps_rsvp_protocol_opt_type;

typedef struct
{
  struct ps_sockaddr  * src_addr[PS_RSVP_MAX_FILTERS];
  uint8                 num_filters;
} ps_rsvp_reg_filter_opt_type;

/*---------------------------------------------------------------------------
  RSVP event enumeration
---------------------------------------------------------------------------*/
typedef enum
{
  PS_RSVP_PATH_EVENT,
  PS_RSVP_RESV_EVENT,
  PS_RSVP_PATH_ERR_EVENT,
  PS_RSVP_RESV_CONF_EVENT,
  PS_RSVP_RESV_ERR_EVENT
} ps_rsvp_event_enum_type;

/*---------------------------------------------------------------------------
  RSVP event call back function type.
---------------------------------------------------------------------------*/
typedef void  (*ps_rsvp_cback_fcn_type)(int32                      sid,
                                        void                     * user_data,
                                        ps_rsvp_event_enum_type    event,
                                        uint32                     obj_mask,
                                        uint32                     msgid);


/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_RSVP_CREATE_SESSION

DESCRIPTION
  Creates a RSVP session

DEPENDENCIES
  None.

RETURN VALUE
  None

SIDE EFFECTS
  Allocates and initializes RSVP control block.
===========================================================================*/
int32 ps_rsvp_create_session
(
  struct ps_sockaddr     * dest,
  int32                    prot_id,
  ps_rsvp_cback_fcn_type   rsvp_cb_fptr,
  void                   * user_data,
  int16                  * ps_errno_ptr
);

/*===========================================================================
FUNCTION PS_RSVP_SET_MSG_OBJECT

DESCRIPTION
  Set an RSVP object.

DEPENDENCIES
  None.

RETURN VALUE
  Returns SUCCESS if it is successful else returns ERROR.

SIDE EFFECTS
  Creates a RSVP object and stores it in session object.
===========================================================================*/
int32 ps_rsvp_set_msg_object
(
  int32                    sid,
  ps_rsvp_msg_enum_type    msg_type,
  ps_rsvp_obj_enum_type    obj,
  void                   * data,
  uint16                 * data_len,
  int16                  * ps_errno_ptr
);

/*===========================================================================
FUNCTION PS_RSVP_GET_MSG_OBJECT

DESCRIPTION
  Read the requested RSVP object from the message specified. Message ID 0
  corressponds to the messages sent by this RSVP session.Message ID is assigned
  and returned to the application in the event call back for all the received
  messages.

DEPENDENCIES
  None.

RETURN VALUE
  Returns the Object in the memory specified by the

SIDE EFFECTS
  None.
===========================================================================*/
int32 ps_rsvp_get_msg_object
(
  int32                    sid,
  ps_rsvp_msg_enum_type    msg_type,
  uint32                   msgid,
  ps_rsvp_obj_enum_type    op,
  void                   * data,
  uint16                 * data_len,
  int16                  * ps_errno_ptr
);

/*===========================================================================
FUNCTION PS_RSVP_SET_OPTION

DESCRIPTION
  Set the specified RSVP option.

DEPENDENCIES
  None.

RETURN VALUE
  returns SUCCESS if it is successful else returns ERROR.

SIDE EFFECTS
  Stores the RSVP option in the RSVP control block.
===========================================================================*/
int32 ps_rsvp_set_option
(
  int32                       sid,
  ps_rsvp_option_enum_type    op,
  void                      * data,
  int16                     * ps_errno_ptr
);

/*===========================================================================
FUNCTION PS_RSVP_GET_OPTION

DESCRIPTION
  Get the RSVP option value.

DEPENDENCIES
  None.

RETURN VALUE
  returns the RSVP option value in the data pointer.

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_rsvp_get_option
(
  int32                       sid,
  ps_rsvp_option_enum_type    op,
  void                      * data,
  int16                     * ps_errno_ptr
);

/*===========================================================================
FUNCTION PS_RSVP_SENDER

DESCRIPTION
 Sends the RSVP PATH message

DEPENDENCIES
  None.

RETURN VALUE
  None

SIDE EFFECTS
  Sends the PATH message to the receivers
===========================================================================*/
int32 ps_rsvp_sender
(
  int32                sid,
  struct ps_sockaddr * dest_addr,
  int16              * ps_errno_ptr
);

/*===========================================================================
FUNCTION PS_RSVP_RESERVE

DESCRIPTION
  Sends the RSVP reserve message.

DEPENDENCIES
  None.

RETURN VALUE
  None

SIDE EFFECTS
  Sends the RESV message onto the network.
===========================================================================*/
int32 ps_rsvp_reserve
(
  int32                sid,
  struct ps_sockaddr * dest_addr,
  int16              * ps_errno_ptr
);

/*===========================================================================
FUNCTION PS_RSVP_RELEASE

DESCRIPTION
  Free the RSVP ocntrol block.

DEPENDENCIES
  None.

RETURN VALUE
  Returns SUCCESS if it is successful else return ERROR.

SIDE EFFECTS
  Frees the RSVP control block.
===========================================================================*/
int32 ps_rsvp_release
(
  int32                sid,
  struct ps_sockaddr * dest_addr,
  int16              * ps_errno_ptr
);

/*===========================================================================
FUNCTION PS_RSVP_FREE_MSG

DESCRIPTION
  Free the received RSVP message.

DEPENDENCIES
  None.

RETURN VALUE
  returns SUCCESS if freeing is successful else returns ERROR.

SIDE EFFECTS
  Frees the specified message from the RSVP control block.
===========================================================================*/
int32 ps_rsvp_free_msg
(
  int32    sid,
  uint32   msgid,
  int16  * ps_errno_ptr
);

/*===========================================================================
FUNCTION PS_RSVP_DELETE_SESSION

DESCRIPTION
  Free the RSVP ocntrol block.

DEPENDENCIES
  None.

RETURN VALUE
  Returns SUCCESS if it is successful else return ERROR.

SIDE EFFECTS
  Frees the RSVP control block.
===========================================================================*/
int32 ps_rsvp_delete_session
(
  int32   sid,
  int16 * ps_errno_ptr
);

#endif  /* PS_RSVP_H */
