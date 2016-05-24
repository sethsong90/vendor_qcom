#ifndef QMI_SVC_UTILS_H
#define QMI_SVC_UTILS_H
/*===========================================================================

                      Q M I _ S V C _ U T I L S . H

DESCRIPTION

  The QMI Services external interface utils file.
 
EXTERNALIZED UTIL FUNCTIONS 
  qmi_svc_put_param_tlv() 
    Construct a message option TLV from given type, length, and value in 
    the provided DSM item.
 
  qmi_svc_put_result_tlv()
    Construct a result option TLV from given result and error code in 
    the provided DSM item.
 
  qmi_svc_get_tl()
    Extract the type and length from the message.
 
Copyright (c) 2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
===========================================================================*/
/*===========================================================================

                            EDIT HISTORY FOR FILE

when        who    what, where, why
--------    ---    ----------------------------------------------------------
01/20/11    rk      Created Module 
===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "dsm.h"
#include "qmi_svc_defs.h"

/*===========================================================================

                         EXTERNAL QMI SVC UTIL MACRO DECLARATIONS

===========================================================================*/
/*===========================================================================
MACRO QMI_SVC_HDLR()

DESCRIPTION
  Returns a qmi_svc_handler initializer given command type and handler
  function.

PARAMETERS
  cmdval : The command type value of the message
  hdlr   : The name of the handler function

RETURNS
  An initializer of qmi_svc_handler type.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
#define QMI_SVC_HDLR(cmdval,hdlr)  { cmdval, #hdlr, hdlr }

/*===========================================================================
MACRO QMI_SVC_PKT_PUSH()

DESCRIPTION
  To push down the given value, val of length len, onto pkt.

PARAMETERS
  pkt : packet
  val : value
  len : length

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
#define QMI_SVC_PKT_PUSH(pkt,val,len)  ( len == dsm_pushdown_packed(pkt,\
                                                  val,\
                                                  len,\
                                                  DSM_DS_SMALL_ITEM_POOL ) )

/*===========================================================================

                            EXTERNAL QMI SVC UTIL FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION QMI_SVC_PUT_PARAM_TLV()

DESCRIPTION
  Construct a TLV using the input type, length and value

PARAMETERS
  pkt   : message to append the TLV to
  type  : value to be put in type field
  len   : value to be put in length field
  value : contents of the value field of TLV

RETURN VALUE
  TRUE  - Input tlv construction is success.
  FALSE - Input tlv construction is failed.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
boolean  qmi_svc_put_param_tlv
(
  dsm_item_type **  pkt,
  byte              type,
  uint16            len,
  void *            value
);

/*===========================================================================
FUNCTION QMI_SVC_PUT_RESULT_TLV()

DESCRIPTION
  Construct a Result Code TLV using the input result and error code

PARAMETERS
  response : response message to append the TLV to
  result   : result code
  error    : error code
  

RETURN VALUE
  TRUE - Result code tlv construction is success.
  FALSE - Result code tlv construction is failed.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
boolean  qmi_svc_put_result_tlv
(
  dsm_item_type **   response,
  qmi_result_e_type  result,
  qmi_error_e_type   error
);

/*===========================================================================
FUNCTION QMI_SVC_GET_TL()

DESCRIPTION
  Extract the type and length from the message

PARAMETERS
  pkt  : message
  type : value in type field
  len  : value in length field

RETURN VALUE
  TRUE   - extracted success.
  FALSE  - extraction failed.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
boolean  qmi_svc_get_tl
(
  dsm_item_type **  pkt,
  uint8 *           type,
  uint16 *          len
);

#endif /* QMI_SVC_UTIL_H */
