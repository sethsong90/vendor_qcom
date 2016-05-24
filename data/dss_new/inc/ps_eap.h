
#ifndef PS_EAP_H
#define PS_EAP_H


/*===========================================================================


   E X T E N S I B L E   A U T H E N T I C A T I O N   P R O T O C O L (EAP)
                       A P I   H E A D E R   F I L E
                   
DESCRIPTION


 The EAP API header file.


Copyright (c) 2005-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_eap.h#2 $ $DateTime: 2011/07/11 12:29:11 $ $Author: dandrus $


when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/10/10    jee    To support re-transmission functionality for EAP server
07/14/10  mga/jee  Modifications to support EAP server functionality
08/31/09    mga    Merged from eHRPD branch
12/19/08    pp     Common Modem Interface: Public/Private API split.
10/21/08    scb    Added function to obtain the reauth id from SFS/EFS
07/18/08    rnp    Fixed Compiler warnings
11/28/07    lti    Added changes for supporting EAP Ext
05/14/07    lti    Added meta info for upper EAP methods
08/25/06    lti    Added function for setting the resp required flag
08/25/06    lti    Added result cback registration
04/14/06    lti    Brought eap_get_identity and eap_get_identifier from 
                   ps_eapi.h
                   Brought eap_code_enum_type to ps_eapi.h
                   Brought eap_types_enum_type from ps_eap_peer.c
                   Formerly known as eap_peer_type_enume_type
                   Brought eap_frm_header_type from ps_eap_eap.c
12/05/05    hba    Updated for support for addition authenticatin schemes
                   including PEAP, TTLS, TLV and MsCHAPv2.
11/23/05    lyr    Added close cback registration
03/22/05    lyr    Updated APIs to reflect design changes
03/21/05    sv     Created module.

===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "dsm.h"



/*===========================================================================

                          PUBLIC DATA DECLARATIONS

===========================================================================*/


#define EAP_IDENTITY_MAX_CHAR 255

/*---------------------------------------------------------------------------
 RFC specifies minimum EAP MTU of 1020 - any client using EAP services must
 support at least 1020 byte MTU on the link.
---------------------------------------------------------------------------*/
#define EAP_DEFAULT_MTU_SIZE 1020

/*---------------------------------------------------------------------------
  Handle to the EAP instance created
---------------------------------------------------------------------------*/
typedef sint15 eap_handle_type;

/*---------------------------------------------------------------------------
  Invalid EAP handle value
---------------------------------------------------------------------------*/
#define EAP_INVALID_HANDLE -1

/*---------------------------------------------------------------------------
  Number of elements in EAP AKA sequence number array.
---------------------------------------------------------------------------*/
#define EAP_AKA_SEQ_NUM_ARRAY_ELEMENTS 32

/*---------------------------------------------------------------------------
  EAP shared secret length
---------------------------------------------------------------------------*/
#define EAP_SHARED_SECRET_LEN 16

/*---------------------------------------------------------------------------
  EAP AKA MILENAGE OP parameter length
---------------------------------------------------------------------------*/
#define EAP_AKA_ALGO_MILENAGE_OP_LEN 16

/*Arbitrary value */
#define EAP_EXT_MAX_VENDOR_ID_PER_INSTANCE  10

/*Per EAP RFC 3748 - page 38*/
#define EAP_EXT_VENDOR_ID_LEN 3

/*---------------------------------------------------------------------------
  Enumeration to indicate the type of AKA/SIM Algorithms to be used.
  This is set by the technology specific mode handler
---------------------------------------------------------------------------*/
typedef enum
{
  EAP_AKA_ALGO_NONE      = 0x0000,
  EAP_AKA_ALGO_SHA1      = 0x0001,
  EAP_AKA_ALGO_MILENAGE  = 0x0002,
  EAP_AKA_ALGO_CAVE      = 0x0003,
  EAP_SIM_ALGO_GSM       = 0x0004,
  EAP_SIM_ALGO_USIM_GSM  = 0x0005,
  EAP_SIM_AKA_ALGO_MAX
} eap_sim_aka_algo_enum_type;

/*---------------------------------------------------------------------------
  Result code for authentication
---------------------------------------------------------------------------*/
typedef enum
{
  EAP_RESULT_SUCCESS = 0,
  EAP_RESULT_FAILURE = 1
  
} eap_result_enum_type;


/*---------------------------------------------------------------------------
  This information is sent in response to a Request/Identity from 
  authenticator
---------------------------------------------------------------------------*/
typedef struct
{ 
  uint16  len;                         /* length of identity               */
  uint8  name[EAP_IDENTITY_MAX_CHAR];  /* User Id                          */
} eap_identity_type;


/*---------------------------------------------------------------------------
  Enumeration of all EAP codes
  
  See RFC 3748
---------------------------------------------------------------------------*/
typedef enum
{
  EAP_MIN_CODE      = 0,

  EAP_REQUEST_CODE  = 1,
  EAP_RESPONSE_CODE = 2,
  EAP_SUCCESS_CODE  = 3,
  EAP_FAILURE_CODE  = 4,

  EAP_MAX_CODE

} eap_code_enum_type;

/*---------------------------------------------------------------------------
  EAP frame header structure
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |     Code      |   Identifier  |               Length          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
---------------------------------------------------------------------------*/
typedef PACKED struct PACKED_POST
{
  uint8  code;
  uint8  identifier;
  uint16 length;
} eap_frm_hdr_type;

/*-------------------------------------------------------------------------
  EAP EXT Meta Info - map of vendor id to vendor handler callback
---------------------------------------------------------------------------*/
typedef void (*eap_ext_vendor_id_input_handler_cb_type)
                                            (eap_handle_type    eap_handle,
                                             dsm_item_type    **msg_ptr);

typedef void (*eap_ext_vendor_id_result_handler_cb_type)
                                            ( eap_handle_type       eap_handle,
                                              eap_result_enum_type  result);

typedef void (*eap_ext_vendor_id_close_handler_cb_type)
                                            (  eap_handle_type  eap_handle);

typedef struct
{
  uint8                                      vendor_id[EAP_EXT_VENDOR_ID_LEN]; 
  eap_ext_vendor_id_input_handler_cb_type    vendor_id_input_handler_ptr;
  eap_ext_vendor_id_result_handler_cb_type   vendor_id_result_handler_ptr;  
  eap_ext_vendor_id_close_handler_cb_type    vendor_id_close_handler_ptr;  
}eap_ext_vendor_id_to_handler_type;

typedef struct
{
    uint16                               vendor_id_count;
    eap_ext_vendor_id_to_handler_type    vendor_handler_tbl
                                         [EAP_EXT_MAX_VENDOR_ID_PER_INSTANCE];
}eap_ext_vendor_id_meta_info_type;


/*---------------------------------------------------------------------------
  Prototype for the transmit function that the lower layer registers to 
  allow EAP to transmit a packet.
---------------------------------------------------------------------------*/
typedef void (*eap_trp_tx_cback_type)
(
  void           *userdata, 
  dsm_item_type **pdu
);


/*---------------------------------------------------------------------------
  Prototype for the authentication complete callback that the lower layer
  registers to allow EAP to convey the authetication result
  
  NOTE: The pre-master key has to be copied by client
---------------------------------------------------------------------------*/
typedef void (*eap_result_ind_cback_type)
( 
  eap_handle_type       handle, 
  void                 *result_ind_user_data,
  eap_result_enum_type  result,
  uint8                *pre_master_key,
  uint16                pre_master_key_len
);


/*---------------------------------------------------------------------------
  Prototype for the method-specific input callback function, which each 
  method has to register at power-up
---------------------------------------------------------------------------*/
typedef void (*eap_method_input_cback_type)
(
 eap_handle_type   handle,
 dsm_item_type   **sdu
);

/*---------------------------------------------------------------------------
  Prototype for the method-specific close callback function, which each 
  method has to register at power-up
---------------------------------------------------------------------------*/
typedef void (*eap_method_close_cback_type)
(
 eap_handle_type   handle
);

/*---------------------------------------------------------------------------
  Prototype for the method-specific result callback function, which each 
  method has to register at power-up
---------------------------------------------------------------------------*/
typedef void (*eap_method_result_cback_type)
(
 eap_handle_type   handle,
 eap_result_enum_type  result
);

/*---------------------------------------------------------------------------
  Enumeration of all the external methods supported by the EAP implementation
---------------------------------------------------------------------------*/
typedef enum
{
  /* NOTE: 0-2 are reserved values                                         */

  EAP_MIN_EXT_METHOD       = 3,

  EAP_MD5_METHOD           = EAP_MIN_EXT_METHOD,
  EAP_TLS_METHOD           = 4,
  EAP_PEAP_METHOD          = 5,
  EAP_TTLS_METHOD          = 6,
  EAP_TLV_METHOD           = 7,
  EAP_MSCHAP_V2_METHOD     = 8,
  EAP_SIM_METHOD           = 9,
  EAP_AKA_METHOD           = 10,
/*---------------------------------------------------------------------------
  Added a new method to support AKA_PRIME
---------------------------------------------------------------------------*/
  EAP_AKA_PRIME_METHOD     = 11,
  EAP_EXT_METHOD           = 12,
  EAP_MAX_METHOD

} eap_method_enum_type;

/*---------------------------------------------------------------------------
  Enumeration of all the external methods supported by the EAP implementation
---------------------------------------------------------------------------*/
typedef enum
{
  /* NOTE: 0-2 are reserved values                                         */

  EAP_MIN_EXT_METHOD_MASK       = 1 << EAP_MIN_EXT_METHOD,

  EAP_MD5_METHOD_MASK           = EAP_MIN_EXT_METHOD_MASK,
  EAP_TLS_METHOD_MASK           = 1 << EAP_TLS_METHOD,
  EAP_PEAP_METHOD_MASK          = 1 << EAP_PEAP_METHOD,
  EAP_TTLS_METHOD_MASK          = 1 << EAP_TTLS_METHOD,
  EAP_TLV_METHOD_MASK           = 1 << EAP_TLV_METHOD,
  EAP_MSCHAP_V2_METHOD_MASK     = 1 << EAP_MSCHAP_V2_METHOD,
  EAP_SIM_METHOD_MASK           = 1 << EAP_SIM_METHOD,
  EAP_AKA_METHOD_MASK           = 1 << EAP_AKA_METHOD,
/*---------------------------------------------------------------------------
  Added a new method to support AKA_PRIME
---------------------------------------------------------------------------*/
  EAP_AKA_PRIME_METHOD_MASK     = 1 << EAP_AKA_PRIME_METHOD,
  EAP_EXT_METHOD_MASK           = 1 << EAP_EXT_METHOD,
  EAP_MAX_METHOD_MASK           = 1 << EAP_MAX_METHOD

} eap_method_mask_enum_type;

/*---------------------------------------------------------------------------
  Role supported : peer, authenticator or both
---------------------------------------------------------------------------*/
typedef enum
{
  EAP_INVALID_ROLE         = -1,
  EAP_PEER_ROLE            = 0,
  EAP_AUTHENTICATOR_ROLE   = 1,
  EAP_ANY_ROLE             = 2
} eap_role_enum_type;


/*---------------------------------------------------------------------------
  Callback type for client start indication callback . 
  This is used only for server role functionality.
---------------------------------------------------------------------------*/
typedef void (*eap_client_ind_cb_type)
(
  eap_handle_type       handle,
  void                 *client_user_data
);

/*---------------------------------------------------------------------------
  Callback function used for task switching after the re-transmission timer 
  time out. Used only for server functionality.
---------------------------------------------------------------------------*/
typedef void (* eap_server_timer_cback_type ) (void *user_data_eap_server);
typedef boolean (* eap_client_timer_cback_type) (
                                    eap_server_timer_cback_type  timer_cback,
                                    void *eap_server_user_data,
                                    void *client_user_data);

/*---------------------------------------------------------------------------
  Structure to hold all relevant information to be passed from lower layer
  during EAP initialization
---------------------------------------------------------------------------*/
typedef struct
{
  eap_identity_type          user_id;             /* User Id               */
  eap_trp_tx_cback_type      trp_tx_f_ptr;        /* Transport tx fn ptr   */
  void                      *trp_tx_user_data;    /* User data for tx fn   */
  eap_result_ind_cback_type  result_ind_f_ptr;    /* auth completed cback  */
  void                      *result_ind_user_data;/* auth cback user data  */
  
  /* The following authentication protocol mask is a bitmask derived using
   * the EAP method enumeration above
   */
  uint32                     eap_auth_prot_mask;  /* auth protocol mask    */
  boolean                    raw_mode;            /* Passthrough or not    */

  /* The following boolean indicates whether the EAP state machine should
   * wait for a SUCCESS from the EAP Server before deeming successful the
   * EAP converstation.
   */ 
  boolean                    authenticator_rsp_required;

  boolean                    support_eap_ext; /*TRUE if support for EAP 
                                                Extended Method is needed*/

  eap_client_ind_cb_type     eap_client_start_ind_cb; /* Identity succ ind */
  eap_role_enum_type         eap_role;                /*Peer or Server role*/
  eap_client_timer_cback_type timer_f_ptr;     /*timer context switch cback*/
  void                       *timer_user_data;  /*user data for timer cback*/
  void                       *start_ind_user_data;    /* start_ind cback user 
                                                         data  */
} eap_info_type;


/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================

FUNCTION EAP_METHOD_INPUT_CBACK_REG

DESCRIPTION
  Registers the method-specific input function with EAP services

DEPENDENCIES
  This must be done once at powerup as all EAP instances will use the same
  input function for a given method.

RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
void eap_method_input_cback_reg
(
  eap_method_input_cback_type cback,
  eap_method_enum_type        method,
  eap_role_enum_type          role
);


/*===========================================================================

FUNCTION EAP_METHOD_CLOSE_CBACK_REG

DESCRIPTION
  Registers the method-specific close function with EAP services

DEPENDENCIES
  This must be done once at powerup as all EAP instances will use the same
  close function for a given method.

RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
void eap_method_close_cback_reg
(
  eap_method_close_cback_type cback,
  eap_method_enum_type        method,
  eap_role_enum_type          role
);

/*===========================================================================

FUNCTION EAP_METHOD_RESULT_CBACK_REG

DESCRIPTION
  Registers the method-specific result function with EAP services

DEPENDENCIES
  This must be done once at powerup as all EAP instances will use the same
  result function for a given method.

RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
void eap_method_result_cback_reg
(
  eap_method_result_cback_type cback,
  eap_method_enum_type        method,
  eap_role_enum_type          role
);

/*===========================================================================

FUNCTION EAP_CREATE_INSTANCE

DESCRIPTION
  Initializes EAP control block and returns handle to EAP instance.

DEPENDENCIES
  EAP services must have been initialized

RETURN VALUE
  Handle to EAP control block.
  
  -1 indicates a failure

SIDE EFFECTS
  Allocates an EAP instance to the specified client.

===========================================================================*/
eap_handle_type eap_create_instance
(  
  eap_info_type *eap_info
);


/*===========================================================================

FUNCTION EAP_DELETE_INSTANCE

DESCRIPTION
   Shuts down the EAP instance.

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  None.

SIDE EFFECTS
  This will cause any subsequent packets sent from an EAP Method to be 
  discarded by the EAP layer.

===========================================================================*/
void eap_delete_instance
(  
  eap_handle_type eap_handle
);


/*===========================================================================

FUNCTION EAP_INPUT

DESCRIPTION
  Rx function for EAP protocol. Parse the EAP header and calls the upper
  Layer's input function if necessary.

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  None

SIDE EFFECTS
  Calls auth protocols RX function.

===========================================================================*/
void eap_input
( 
  eap_handle_type   eap_handle,
  dsm_item_type   **sdu
);


/*===========================================================================
FUNCTION EAP_XMIT_PKT

DESCRIPTION
  Tx function for the EAP protocol. Adds EAP header and send the packet on
  the associated link layer.

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  None.

SIDE EFFECTS
  Sends the packet on associated link.

===========================================================================*/
void eap_xmit_pkt
(
   eap_handle_type   eap_handle,
   dsm_item_type   **sdu
); 


/*===========================================================================

FUNCTION EAP_AUTH_COMPLETE_IND

DESCRIPTION
  This function is called by the authentication protocol to notify EAP of the
  result of the authentication process.

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  None.

SIDE EFFECTS
  Calls client's authentication completed callback.

===========================================================================*/
void eap_auth_complete_ind 
(
  eap_handle_type          handle,
  eap_result_enum_type     result,
  uint8                   *pre_master_key,
  uint16                   pre_master_key_len
);

/*===========================================================================

FUNCTION EAP_AUTH_TRANSACTION_CLOSING

DESCRIPTION
  This function is called by the authentication protocol to notify 
  EAP's peer layer to perform appropriate state transitions, once
  indicating that the EAP auth transaction is closing.

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void eap_auth_transaction_closing 
(
  eap_handle_type          eap_handle
);

/*===========================================================================
FUNCTION EAP_SET_AUTHENTICATOR_RSP_REQUIRED

DESCRIPTION
  Offers support for dynamically setting the authenticator_rsp_required flag

DEPENDENCIES

  
RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
void 
eap_set_authenticator_rsp_required
(
  eap_handle_type          eap_handle,
  boolean                  authenticator_rsp_required 
);

/*===========================================================================

FUNCTION EAP_HANDLE_IS_VALID

DESCRIPTION
  This function is called to verify the validity of the EAP instance

DEPENDENCIES
  EAP instance should have been created

RETURN VALUE
  True:  Handle is valid
  False: Handle is invalid

SIDE EFFECTS
  None.

===========================================================================*/
boolean eap_handle_is_valid
(
  eap_handle_type eap_handle
);



/*===========================================================================

FUNCTION EAP_SET_AUTH_MGR

DESCRIPTION
  This function set the authenticator manager for a particular upper method.
  
  Each EAP instance can have a separate auth manager irrespective of the
  method in use.
  
  EAP is just a conduit for this auth mgr information - an auth mgr is stored
  per EAP instance and not associated with any method. Caller is expected to
  store any such information in the auth_mgr entity.

DEPENDENCIES
  EAP instance should have been created

RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
void eap_set_auth_mgr
(
  eap_handle_type       eap_handle,
  void                 *auth_mgr
);


/*===========================================================================

FUNCTION EAP_SET_AUTH_MGR

DESCRIPTION
  This function gets the authenticator manager for a particular upper method.

DEPENDENCIES
  EAP instance should have been created

RETURN VALUE
  A pointer to the authenticator manager. Note here that the returned pointer
  is abstract and it is up to the caller to cast to the concrete manager.

SIDE EFFECTS
  None.

===========================================================================*/
void* eap_get_auth_mgr
(
  eap_handle_type   eap_handle
);

/*===========================================================================

FUNCTION EAP_SET_META_INFO

DESCRIPTION
  This function set the meta info for a particular upper method.
  
  Each EAP instance can have a separate meta info irrespective of the
  method in use.
  
DEPENDENCIES
  EAP instance should have been created

RETURN VALUE
  None

SIDE EFFECTS
  None.

===========================================================================*/
void eap_set_meta_info
(
  eap_handle_type       eap_handle,
  void                 *meta_info_ptr
);

/*===========================================================================

FUNCTION EAP_GET_META_INFO

DESCRIPTION
  This function gets the meta info for a particular upper method.

DEPENDENCIES
  EAP instance should have been created

RETURN VALUE
  A pointer to the meta info. Note here that the returned pointer
  is abstract and it is up to the caller to cast to the concrete manager.

SIDE EFFECTS
  None.

===========================================================================*/
void* eap_get_meta_info
(
  eap_handle_type   eap_handle
);

/*===========================================================================

FUNCTION EAP_GET_IDENTITY

DESCRIPTION
  This function is called by a method to request the identity of the EAP
  instance

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  EAP identity.

SIDE EFFECTS
  None.

===========================================================================*/
eap_identity_type eap_get_identity
(
  eap_handle_type eap_handle
);

/*===========================================================================

FUNCTION EAP_GET_IDENTIFIER

DESCRIPTION
  This function is called by a method to request the identifier of the last EAP
  packet

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  Last EAP pkt identifier.

SIDE EFFECTS
  None.

===========================================================================*/
uint8 eap_get_identifier
(
  eap_handle_type eap_handle
);

/*===========================================================================

FUNCTION EAP_GET_REAUTH_ID

DESCRIPTION
  This function is called by a method to request the re-authentication 
  identifier of the last EAP packet

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  Last EAP reauth identifier if present -1 otherwise.

SIDE EFFECTS
  None.

===========================================================================*/
int eap_get_reauth_id
(
  eap_handle_type                        eap_handle,
  uint8*                                 data,
  uint16*                                max_data_len
);

/*===========================================================================

FUNCTION EAP_GET_PSEUDONYM_ID

DESCRIPTION
  This function is called by a method to request the pseudonym
  identifier of the last EAP packet

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  Last EAP pseudonym identifier if present -1 otherwise.

SIDE EFFECTS
  None.

===========================================================================*/
int eap_get_pseudonym_id
(
  eap_handle_type                        eap_handle,
  uint8*                                 data,
  uint16*                                max_data_len
);

/*===========================================================================

FUNCTION EAP_START_IND

DESCRIPTION
  This function is called when the client is ready and the AT can start EAP.
  This is used only by EAP server functionality.
 
DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  Returns -1 if no handle or error scenario.
  Returns 0 in case of success.

SIDE EFFECTS
  None.

===========================================================================*/
int eap_start_ind 
( 
  eap_handle_type eap_handle
);

/*===========================================================================
FUNCTION EAP_EXT_XMIT_PKT

DESCRIPTION
  Tx function for the EAP protocol. Adds EAP Ext header and send the packet 
  to the lower EAP layer.

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  None.

SIDE EFFECTS
  Sends the packet on associated link.

===========================================================================*/
void eap_ext_xmit_pkt
(
   eap_handle_type   eap_handle,
   dsm_item_type   **sdu,
   uint8*             vendor_id
); 

/*===========================================================================
FUNCTION EAP_EXT_AUTH_COMPLETE_IND

DESCRIPTION
  This function is called by the authentication protocol to notify EAP of the
  result of the authentication process. The extended method will act as 
  pass-through and send the result to the lower EAP layer.

DEPENDENCIES
  EAP instance must have been created

RETURN VALUE
  None.

SIDE EFFECTS

===========================================================================*/
void eap_ext_auth_complete_ind 
(
  eap_handle_type          handle,
  eap_result_enum_type     result,
  uint8                   *pre_master_key,
  uint16                   pre_master_key_len
);

#endif  /* PS_EAP_H */
