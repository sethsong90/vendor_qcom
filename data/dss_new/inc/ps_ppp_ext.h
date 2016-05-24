#ifndef PS_PPP_EXT_H
#define PS_PPP_EXT_H
/*===========================================================================

                          P S _ P P P _ E X T . H

DESCRIPTION
  This file definitions that are used for PPP externally (at Modem level).


Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                            EDIT HISTORY FOR FILE

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_ppp_defs.h_v   1.7   03 Feb 2003 15:01:04   jeffd  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_ppp_ext.h#1 $
  $Author: maldinge $ $DateTime: 2011/01/10 09:44:56 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/31/09   mga     Merged from eHRPD branch
12/14/08    pp     Created module as part of Common Modem Interface:
                   Public/Private API split.
===========================================================================*/

/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "comdef.h"
#include "ps_eap.h"

/*---------------------------------------------------------------------------
TYPEDEF PPP_DEV_ENUM_TYPE

DESCRIPTION
  Enum defining the PPP "devices" that are available
---------------------------------------------------------------------------*/
typedef enum
{
  PPP_MIN_DEV                   = 0,   /* the lower bound value            */
  PPP_UM_SN_DEV                 = 0,   /* Um Service Network instance      */
  PPP_RM_DEV                    = 1,   /* the RM instance                  */
  PPP_UM_AN_DEV                 = 2,   /* The Um AN instance               */
  PPP_UW_FMC_DEV                = 3,   /* The UW FMC instance              */
  PPP_MAX_DEV,                         /* max value - array sizing         */
  PPP_INVALID_DEV               = 255  /* invalid device                   */
} ppp_dev_enum_type;

/*---------------------------------------------------------------------------
  Maximum length permissable for user id. Note that these should be equal to
  or greater than NV_MAX_PPP_USER_ID_LENGTH. The greater than case, may be
  necessary to store the TE2 user-id for network model packet calls.

  This must correspond to NV_MAX_PPP_USER_ID_LENGTH in nv.h
---------------------------------------------------------------------------*/
#define PPP_MAX_USER_ID_LEN 127

/*---------------------------------------------------------------------------
  Maximum length permissable for password used during authentication, i.e.
  PAP or CHAP. Note that this should be equal to or greater than
  NV_MAX_PAP_PASSWORD_LENGTH.

  Note that in network model, the Password used during resync for CHAP is
  different than that used initially.

  This must correspond to NV_MAX_PPP_PASSWORD_LENGTH in nv.h
---------------------------------------------------------------------------*/
#define PPP_MAX_PASSWD_LEN  127


/*---------------------------------------------------------------------------
  The maximum challenge length we support is 16 bytes as is the maximum
  length of the challenge name length.
---------------------------------------------------------------------------*/
#define PPP_CHAP_CHAL_LEN      16
#define PPP_CHAP_CHAL_NAME_LEN 16

/*---------------------------------------------------------------------------
  EAP user ID maximum length and Password length 
---------------------------------------------------------------------------*/
#define PPP_EAP_MAX_USER_ID_LEN   255
#define PPP_EAP_SHARED_SECRET_LEN 16
 
/*---------------------------------------------------------------------------
  Maximum length for APN. NULL APN (apn length zero) is supported.
---------------------------------------------------------------------------*/
#define VSNCP_3GPP2_APN_MAX_VAL_LEN  100

typedef enum 
{
  PS_PPP_EAP_RESULT_SUCCESS = 0,
  PS_PPP_EAP_RESULT_FAILURE = 1 
}ps_ppp_eap_result_enum_type;

/*---------------------------------------------------------------------------
    Mode Handler Function type that is registerd with PS.
    This wll be called by PS when EAP returns result.
---------------------------------------------------------------------------*/

typedef void (* ppp_eap_config_data_after_auth )
(
  uint8 *master_session_key,
  uint16 master_session_key_len,
  ps_ppp_eap_result_enum_type ps_eap_result,
  void *eap_user_data,
  uint64 *aka_algo_seqnum_arr,
  uint8 aka_algo_seqnum_arr_num_elements
);

/*---------------------------------------------------------------------------
TYPEDEF PPP_AUTH_INFO_TYPE

DESCRIPTION
  This contains the authentication information (i.e. username & password)

  How this is to be used:
  All the lengths should indicate the  amount of data in the corresponding
  buffers. If nothing is stored they should be set to 0.
  Client Mode:
    user_id_info and passwd_info MUST be filled (i.e. are input parameters)
    in if was accept authentication. challenge_info and challenge_name are
    NOT used either as input or output.
  Server Mode:
    challenge_name SHOULD be filled in if doing CHAP - whatever is passed
    here will be in the challange name field of the CHAP challenge.  The
    other three fields are filled in when the client sends its information.
---------------------------------------------------------------------------*/
typedef struct
{
  char  user_id_info[PPP_MAX_USER_ID_LEN];
  char  passwd_info[PPP_MAX_PASSWD_LEN];
  char  challenge_info[PPP_CHAP_CHAL_LEN];
  char  challenge_name[PPP_CHAP_CHAL_NAME_LEN];
  uint8 user_id_len;
  uint8 passwd_len;
  uint8 challenge_len;
  uint8 challenge_name_len;
  uint8 nai_entry_index;
  char    eap_user_id[PPP_EAP_MAX_USER_ID_LEN];
  uint8   eap_user_id_len;
  char    eap_shared_secret[PPP_EAP_SHARED_SECRET_LEN];
  void    *eap_user_data;
  ppp_eap_config_data_after_auth  get_config_data_after_auth_ptr;
  uint64 aka_algo_seqnum_arr[EAP_AKA_SEQ_NUM_ARRAY_ELEMENTS];
  uint8 aka_algo_seqnum_arr_num_elements;
  /*-------------------------------------------------------------------------
    Algorithm to be used for EAP_AKA (i.e either SHA1 or MILENAGE 
    supported currently)
  -------------------------------------------------------------------------*/
  eap_sim_aka_algo_enum_type aka_algo_type;

  /*-------------------------------------------------------------------------
    To decide whether to do EAP-AKA in software or USIM card
  -------------------------------------------------------------------------*/
  boolean ehrpd_usim_present;

  /*-------------------------------------------------------------------------
    Attributes required to store data to be passed to the AKALAGO
    module which implements the MILENAGE algorithm
  -------------------------------------------------------------------------*/
  uint8  aka_algo_milenage_op_data[EAP_AKA_ALGO_MILENAGE_OP_LEN];
  uint32 aka_algo_milenage_op_data_len;
} ppp_auth_info_type;

#endif /* PS_PPP_EXT_H */
