#ifndef PS_EAP_SIM_AKA_H
#define PS_EAP_SIM_AKA_H


/*===========================================================================

       E A P   S U B S C R I B E R   I D E N T I T Y   M O D U L E
        A U T H E N T I C A T I O N   K E Y   A G R E E M E N T
                            
                
                   
DESCRIPTION
  This file contains common EAP-SIM/AKA processing functions.
     
    
Copyright (c) 2006-2009 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_eap_sim_aka.h#1 $ 
  $DateTime: 2011/01/10 09:44:56 $ 
  $Author: maldinge $

===========================================================================*/



/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ps_eap.h"

/*===========================================================================

               EAP SIM DATA DECLARATIONS AND DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  Task service function type   
---------------------------------------------------------------------------*/
typedef void (* eap_sim_aka_task_srv_req_cb_type )( void * user_data);

typedef boolean (* eap_sim_aka_task_srv_req_type)
                 ( eap_sim_aka_task_srv_req_cb_type  req_cb,
                                            void*             user_data);

/*---------------------------------------------------------------------------
  EAP AKA Message Notification receive function type
---------------------------------------------------------------------------*/
typedef void (* eap_sim_aka_notification_rcv_cb_type )( void * user_data);

/* Permanent ID Len - arbitrary */
#define EAP_SIM_AKA_MAX_ID_LEN            255

/*---------------------------------------------------------------------------
  EAP SIM-AKA specific method meta info
---------------------------------------------------------------------------*/
typedef struct
{
  struct 
  {
    /*preset identity lenght*/
    uint8                          provided_id_len;

    /*preset identity*/
    uint8                          provided_id[EAP_SIM_AKA_MAX_ID_LEN];
  } id;

  struct
  {
    /* imsi_mcc */
    uint16 mcc;  /* imsi_mcc */
    uint8  mnc;  /* imsi_mnc */
    uint32 s1;   /* imsi_s1 / imsi_min1 */
    uint16 s2;   /* imsi_s2 / imsi_min2 */
  } imsi;

  /*task srv function*/
  eap_sim_aka_task_srv_req_type  task_srv_fct;
  /*Notification receive callback function*/
  eap_sim_aka_notification_rcv_cb_type notification_rcv_cb_fct;

  /*Sequence Number details*/
  uint64 aka_algo_seqnum_arr[EAP_AKA_SEQ_NUM_ARRAY_ELEMENTS];
  uint8  aka_algo_seqnum_arr_num_elements;

  /*EAP Shared Secret details*/
  char   eap_shared_secret[EAP_SHARED_SECRET_LEN];

 /* boolean flag to differentiate new and old client */
  boolean new_client;

  /*-------------------------------------------------------------------------
    Algorithm to be used for EAP_AKA/SIM
  -------------------------------------------------------------------------*/
  eap_sim_aka_algo_enum_type sim_aka_algo_type;

  /*-------------------------------------------------------------------------
    tells whether to do EAP-AKA in software or using USIM card
  -------------------------------------------------------------------------*/
  boolean eap_aka_in_sw;

  /*-------------------------------------------------------------------------
    Attributes required to store data to be passed to the AKALAGO
    module which implements the MILENAGE algorithm
  -------------------------------------------------------------------------*/
  uint8  aka_algo_milenage_op_data[EAP_AKA_ALGO_MILENAGE_OP_LEN];
  uint32 aka_algo_milenage_op_data_len;
}eap_sim_aka_meta_info_type;

extern eap_identity_type eap_reauth_id;
extern eap_identity_type eap_pseudonym_id;

/*---------------------------------------------------------------------------
  AT_NOTIFICATION_CODE
---------------------------------------------------------------------------*/
/* Enumeration of AKA notification codes according to RFC 4187 */
typedef enum
{
   EAP_AKA_NOTIFICATION_GENERAL_FAILURE_AFTER_AUTH   = 0,
   EAP_AKA_NOTIFICATION_USER_DENIED_TEMP             = 1026,
   EAP_AKA_NOTIFICATION_USER_NOT_SUBSCRIBED          = 1031,
   EAP_AKA_NOTIFICATION_GENERAL_FAILURE              = 16384,
   EAP_AKA_NOTIFICATION_REALM_UNAVAILABLE            = 16385, 
   EAP_AKA_NOTIFICATION_USER_NAME_UNAVAILABLE        = 16386, 
   EAP_AKA_NOTIFICATION_CALL_BARRED                  = 16387,
   EAP_AKA_NOTIFICATION_SUCCESS                      = 32768
}eap_sim_aka_notification_enum_type;

#endif /* PS_EAP_SIM_AKA_H */
