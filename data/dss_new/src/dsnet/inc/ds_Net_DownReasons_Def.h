#ifndef DS_NET_DOWNREASONS_DEF_H
#define DS_NET_DOWNREASONS_DEF_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"

/**
  * ds Net Network Down Reason types and constants 
  */
typedef int ds_Net_NetDownReasonType;

/** @memberof ds_Net
  * 
  * Type 1: MIP Network Down Reasons.
  */
#define ds_Net_DOWN_REASON_TYPE1 0x10000

/** @memberof ds_Net
  * 
  * Type 2: Internal Network Down Reasons.
  */
#define ds_Net_DOWN_REASON_TYPE2 0x20000

/** @memberof ds_Net
  * 
  * Type 3: Call Manager Network Down Reasons
  */
#define ds_Net_DOWN_REASON_TYPE3 0x30000

/** @memberof ds_Net
  * 
  * Type 4: EAP Network Down Reasons.
  */
#define ds_Net_DOWN_REASON_TYPE4 0x40000

/** @memberof ds_Net
  * 
  * Type 5: IPSEC Network Down Reasons.
  */
#define ds_Net_DOWN_REASON_TYPE5 0x50000

/** @memberof ds_Net
  * 
  * Type 6: Network Down Reasons mapped to corresponding 
  * network down reasons from 3GPP TS 24.008 version 3.5.0 Release 1999
  */
#define ds_Net_DOWN_REASON_TYPE6 0x60000

/** @memberof ds_Net_NetDownReason
  * 
  * The network was closed for an unspecified reason.
  */
#define ds_Net_NetDownReason_QDS_NOT_SPECIFIED 0
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_REASON_UNSPECIFIED 0x10040
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_ADMINISTRATIVELY_PROHIBITED 0x10041
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_INSUFFICIENT_RESOURCES 0x10042
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE 0x10043
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_HA_AUTHENTICATION_FAILURE 0x10044
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_REQUESTED_LIFETIME_TOO_LONG 0x10045
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_MALFORMED_REQUEST 0x10046
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_MALFORMED_REPLY 0x10047
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_ENCAPSULATION_UNAVAILABLE 0x10048
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_VJHC_UNAVAILABLE 0x10049
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_REVERSE_TUNNEL_UNAVAILABLE 0x1004a
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET 0x1004b
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_DELIVERY_STYLE_NOT_SUPPORTED 0x1004f
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_MISSING_NAI 0x10061
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_MISSING_HA 0x10062
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_MISSING_HOME_ADDR 0x10063
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_UNKNOWN_CHALLENGE 0x10068
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_MISSING_CHALLENGE 0x10069
#define ds_Net_NetDownReason_QDS_MIP_FA_ERR_STALE_CHALLENGE 0x1006a
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_REASON_UNSPECIFIED 0x10080
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_ADMINISTRATIVELY_PROHIBITED 0x10081
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_INSUFFICIENT_RESOURCES 0x10082
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE 0x10083
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_FA_AUTHENTICATION_FAILURE 0x10084
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_REGISTRATION_ID_MISMATCH 0x10085
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_MALFORMED_REQUEST 0x10086
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_UNKNOWN_HA_ADDR 0x10088
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_REVERSE_TUNNEL_UNAVAILABLE 0x10089
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET 0x1008a
#define ds_Net_NetDownReason_QDS_MIP_HA_ERR_ENCAPSULATION_UNAVAILABLE 0x1008b
#define ds_Net_NetDownReason_QDS_MIP_ERR_REASON_UNKNOWN 0x1ffff

/** @memberof ds_Net_NetDownReason
  * 
  * Call is terminated by the mobile device because of some internal error. 
  */
#define ds_Net_NetDownReason_QDS_INTERNAL_ERROR 0x200c9

/** @memberof ds_Net_NetDownReason
  * 
  * Call is terminated locally by the mobile. 
  */
#define ds_Net_NetDownReason_QDS_INTERNAL_CALL_ENDED 0x200ca

/** @memberof ds_Net_NetDownReason
  * 
  * Call is terminated locally by the mobile but the cause is not mapped to a network down reason yet. 
  */
#define ds_Net_NetDownReason_QDS_INTERNAL_UNKNOWN_CAUSE_CODE 0x200cb

/** @memberof ds_Net_NetDownReason
  * 
  * Reason why the call ended cannot be provided or is not mapped to a network down reason yet. 
  */
#define ds_Net_NetDownReason_QDS_UNKNOWN_CAUSE_CODE 0x200cc

/** @memberof ds_Net_NetDownReason
  * 
  * The network connection request was rejected as the previous connection of the same network is
  * currently being closed.
  */
#define ds_Net_NetDownReason_QDS_CLOSE_IN_PROGRESS 0x200cd

/** @memberof ds_Net_NetDownReason
  * 
  * This indicates that the session was terminated by the network.
  */
#define ds_Net_NetDownReason_QDS_NW_INITIATED_TERMINATION 0x200ce
#define ds_Net_NetDownReason_QDS_APP_PREEMPTED 0x200cf
#define ds_Net_NetDownReason_QDS_CDMA_LOCK 0x301f4
#define ds_Net_NetDownReason_QDS_INTERCEPT 0x301f5
#define ds_Net_NetDownReason_QDS_REORDER 0x301f6
#define ds_Net_NetDownReason_QDS_REL_SO_REJ 0x301f7
#define ds_Net_NetDownReason_QDS_INCOM_CALL 0x301f8
#define ds_Net_NetDownReason_QDS_ALERT_STOP 0x301f9
#define ds_Net_NetDownReason_QDS_ACTIVATION 0x301fa
#define ds_Net_NetDownReason_QDS_MAX_ACCESS_PROBE 0x301fb
#define ds_Net_NetDownReason_QDS_CCS_NOT_SUPPORTED_BY_BS 0x301fc
#define ds_Net_NetDownReason_QDS_NO_RESPONSE_FROM_BS 0x301fd
#define ds_Net_NetDownReason_QDS_REJECTED_BY_BS 0x301fe
#define ds_Net_NetDownReason_QDS_INCOMPATIBLE 0x301ff
#define ds_Net_NetDownReason_QDS_ALREADY_IN_TC 0x30200
#define ds_Net_NetDownReason_QDS_USER_CALL_ORIG_DURING_GPS 0x30201
#define ds_Net_NetDownReason_QDS_USER_CALL_ORIG_DURING_SMS 0x30202
#define ds_Net_NetDownReason_QDS_NO_CDMA_SRV 0x30203
#define ds_Net_NetDownReason_QDS_MC_ABORT 0x30204
#define ds_Net_NetDownReason_QDS_PSIST_NG 0x30205
#define ds_Net_NetDownReason_QDS_UIM_NOT_PRESENT 0x30206
#define ds_Net_NetDownReason_QDS_RETRY_ORDER 0x30207
#define ds_Net_NetDownReason_QDS_ACCESS_BLOCK 0x30208
#define ds_Net_NetDownReason_QDS_ACCESS_BLOCK_ALL 0x30209
#define ds_Net_NetDownReason_QDS_IS707B_MAX_ACC 0x3020a
#define ds_Net_NetDownReason_QDS_CONF_FAILED 0x303e8
#define ds_Net_NetDownReason_QDS_INCOM_REJ 0x303e9
#define ds_Net_NetDownReason_QDS_NO_GW_SRV 0x303ea
#define ds_Net_NetDownReason_QDS_NO_GPRS_CONTEXT 0x303eb
#define ds_Net_NetDownReason_QDS_ILLEGAL_MS 0x303ec
#define ds_Net_NetDownReason_QDS_ILLEGAL_ME 0x303ed
#define ds_Net_NetDownReason_QDS_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED 0x303ee
#define ds_Net_NetDownReason_QDS_GPRS_SERVICES_NOT_ALLOWED 0x303ef
#define ds_Net_NetDownReason_QDS_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK 0x303f0
#define ds_Net_NetDownReason_QDS_IMPLICITLY_DETACHED 0x303f1
#define ds_Net_NetDownReason_QDS_PLMN_NOT_ALLOWED 0x303f2
#define ds_Net_NetDownReason_QDS_LA_NOT_ALLOWED 0x303f3
#define ds_Net_NetDownReason_QDS_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN 0x303f4
#define ds_Net_NetDownReason_QDS_PDP_DUPLICATE 0x303f5
#define ds_Net_NetDownReason_QDS_UE_RAT_CHANGE 0x303f6
#define ds_Net_NetDownReason_QDS_CONGESTION 0x303f7
#define ds_Net_NetDownReason_QDS_NO_PDP_CONTEXT_ACTIVATED 0x303f8
#define ds_Net_NetDownReason_QDS_ACCESS_CLASS_DSAC_REJECTION 0x303f9
#define ds_Net_NetDownReason_QDS_CD_GEN_OR_BUSY 0x305dc
#define ds_Net_NetDownReason_QDS_CD_BILL_OR_AUTH 0x305dd
#define ds_Net_NetDownReason_QDS_CHG_HDR 0x305de
#define ds_Net_NetDownReason_QDS_EXIT_HDR 0x305df
#define ds_Net_NetDownReason_QDS_HDR_NO_SESSION 0x305e0
#define ds_Net_NetDownReason_QDS_HDR_ORIG_DURING_GPS_FIX 0x305e1
#define ds_Net_NetDownReason_QDS_HDR_CS_TIMEOUT 0x305e2
#define ds_Net_NetDownReason_QDS_HDR_RELEASED_BY_CM 0x305e3
#define ds_Net_NetDownReason_QDS_COLLOC_ACQ_FAIL 0x305e4
#define ds_Net_NetDownReason_QDS_OTASP_COMMIT_IN_PROG 0x305e5
#define ds_Net_NetDownReason_QDS_NO_HYBR_HDR_SRV 0x305e6
#define ds_Net_NetDownReason_QDS_HDR_NO_LOCK_GRANTED 0x305e7
#define ds_Net_NetDownReason_QDS_HOLD_OTHER_IN_PROG 0x305e8
#define ds_Net_NetDownReason_QDS_HDR_FADE 0x305e9
#define ds_Net_NetDownReason_QDS_HDR_ACC_FAIL 0x305ea
#define ds_Net_NetDownReason_QDS_CLIENT_END 0x307d0
#define ds_Net_NetDownReason_QDS_NO_SRV 0x307d1
#define ds_Net_NetDownReason_QDS_FADE 0x307d2
#define ds_Net_NetDownReason_QDS_REL_NORMAL 0x307d3
#define ds_Net_NetDownReason_QDS_ACC_IN_PROG 0x307d4
#define ds_Net_NetDownReason_QDS_ACC_FAIL 0x307d5
#define ds_Net_NetDownReason_QDS_REDIR_OR_HANDOFF 0x307d6
#define ds_Net_NetDownReason_QDS_OFFLINE 0x309c4
#define ds_Net_NetDownReason_QDS_EMERGENCY_MODE 0x309c5
#define ds_Net_NetDownReason_QDS_PHONE_IN_USE 0x309c6
#define ds_Net_NetDownReason_QDS_INVALID_MODE 0x309c7
#define ds_Net_NetDownReason_QDS_INVALID_SIM_STATE 0x309c8
#define ds_Net_NetDownReason_QDS_NO_COLLOC_HDR 0x309c9
#define ds_Net_NetDownReason_QDS_CALL_CONTROL_REJECTED 0x309ca

/** @memberof ds_Net_NetDownReason
  * 
  * A general error code indicating failure 
  */
#define ds_Net_NetDownReason_QDS_EAP_CLIENT_ERR_UNABLE_TO_PROCESS 0x41389

/** @memberof ds_Net_NetDownReason
  * 
  * The peer does not support any of the versions listed in AT_VERSION_LIST 
  */
#define ds_Net_NetDownReason_QDS_EAP_CLIENT_ERR_UNSUPPORTED_VERS 0x4138a

/** @memberof ds_Net_NetDownReason
  * 
  * The peer's policy requires more triplets than the server included in AT_RAND 
  */
#define ds_Net_NetDownReason_QDS_EAP_CLIENT_ERR_INSUFFICIENT_CHALLANGES 0x4138b

/** @memberof ds_Net_NetDownReason
  * 
  * The peer believes that the RAND challenges included in AT_RAND were not fresh 
  */
#define ds_Net_NetDownReason_QDS_EAP_CLIENT_ERR_RAND_NOT_FRESH 0x4138c

/** @memberof ds_Net_NetDownReason
  * 
  * Notification code indicating general failure after authentication 
  */
#define ds_Net_NetDownReason_QDS_EAP_NOTIFICATION_GENERAL_FAILURE_AFTER_AUTH 0x4138d

/** @memberof ds_Net_NetDownReason
  * 
  * Notification code indicating general failure before authentication 
  */
#define ds_Net_NetDownReason_QDS_EAP_NOTIFICATION_GENERAL_FAILURE_BEFORE_AUTH 0x4138e

/** @memberof ds_Net_NetDownReason
  * 
  * User has been temporarily denied access to the requested service.
  * (Implies failure, used after successful authentication.)
  */
#define ds_Net_NetDownReason_QDS_EAP_NOTIFICATION_TEMP_DENIED_ACCESS 0x4138f

/** @memberof ds_Net_NetDownReason
  * 
  * User has not subscribed to the requested service.
  * (Implies failure, used after successful authentication.)
  */
#define ds_Net_NetDownReason_QDS_EAP_NOTIFICATION_USER_NOT_SUBSCRIBED 0x41390

/** @memberof ds_Net_NetDownReason
  * 
  * Success.  User has been successfully authenticated 
  */
#define ds_Net_NetDownReason_QDS_EAP_SUCCESS 0x41391

/** @memberof ds_Net_NetDownReason
  * 
  * Notification error code while parsing an EAP identity indicating that 
  * NAI realm portion is unavailable in environments where a realm is required.
  */
#define ds_Net_NetDownReason_QDS_EAP_NOTIFICATION_REALM_UNAVAILABLE 0x41392

/** @memberof ds_Net_NetDownReason
  * 
  * Notification error code while parsing an EAP identity indicating 
  * that username is unavailable.
  */
#define ds_Net_NetDownReason_QDS_EAP_NOTIFICATION_USER_NAME_UNAVAILABLE 0x41393

/** @memberof ds_Net_NetDownReason
  * 
  * Notification error code indicating that the call has been barred. 
  */
#define ds_Net_NetDownReason_QDS_EAP_NOTIFICATION_CALL_BARRED 0x41394

/** @memberof ds_Net_NetDownReason
  * 
  * The call failed because the PDIF is unreachable. 
  */
#define ds_Net_NetDownReason_QDS_IPSEC_GW_UNREACHABLE 0x513ed

/** @memberof ds_Net_NetDownReason
  * 
  * The call failed because of authentication failure during IPSEC tunnel establishment. 
  */
#define ds_Net_NetDownReason_QDS_IPSEC_AUTH_FAILED 0x513ee

/** @memberof ds_Net_NetDownReason
  * 
  * The call failed because the certificate is invalid. 
  */
#define ds_Net_NetDownReason_QDS_IPSEC_CERT_INVALID 0x513ef

/** @memberof ds_Net_NetDownReason
  * 
  * The call failed because of internal error in IPSEC. 
  */
#define ds_Net_NetDownReason_QDS_IPSEC_INTERNAL_ERROR 0x513f0

/** @memberof ds_Net_NetDownReason
  * 
  * This reason indicates that the operator determined barring.
  */
#define ds_Net_NetDownReason_QDS_OPERATOR_DETERMINED_BARRING 0x60008
#define ds_Net_NetDownReason_QDS_LLC_SNDCP_FAILURE 0x60019

/** @memberof ds_Net_NetDownReason
  * 
  * PDP context activation request, secondary PDP context activation request or PDP context 
  * modification request cannot be accepted due to insufficient resources.
  */
#define ds_Net_NetDownReason_QDS_INSUFFICIENT_RESOURCES 0x6001a

/** @memberof ds_Net_NetDownReason
  * 
  * Requested service was rejected by the external packet data network 
  * because the access point name was not included although required 
  * or if the access point name could not be resolved.
  */
#define ds_Net_NetDownReason_QDS_UNKNOWN_APN 0x6001b

/** @memberof ds_Net_NetDownReason
  * 
  * Requested service was rejected by the external packet data network 
  * because the PDP address or type could not be recognized.
  */
#define ds_Net_NetDownReason_QDS_UNKNOWN_PDP 0x6001c

/** @memberof ds_Net_NetDownReason
  * 
  * Requested service was rejected by the external packet data network 
  * due to a failed user authentication.
  */
#define ds_Net_NetDownReason_QDS_AUTH_FAILED 0x6001d

/** @memberof ds_Net_NetDownReason
  * 
  * GGSN has rejected the activation request.
  */
#define ds_Net_NetDownReason_QDS_GGSN_REJECT 0x6001e

/** @memberof ds_Net_NetDownReason
  * 
  * Activation request rejected with unspecified reason.
  */
#define ds_Net_NetDownReason_QDS_ACTIVATION_REJECT 0x6001f

/** @memberof ds_Net_NetDownReason
  * 
  * The requested service option is not supported by the PLMN.
  */
#define ds_Net_NetDownReason_QDS_OPTION_NOT_SUPPORTED 0x60020

/** @memberof ds_Net_NetDownReason
  * 
  * The requested service option is not subscribed for.
  */
#define ds_Net_NetDownReason_QDS_OPTION_UNSUBSCRIBED 0x60021

/** @memberof ds_Net_NetDownReason
  * 
  * MSC cannot service the request because of temporary 
  * outage of one or more functions required for supporting the service. 
  */
#define ds_Net_NetDownReason_QDS_OPTION_TEMP_OOO 0x60022

/** @memberof ds_Net_NetDownReason
  * 
  * MNSAPI requested by the MS in the PDP context activation 
  * request is already used by another active PDP context of this MS.
  */
#define ds_Net_NetDownReason_QDS_NSAPI_ALREADY_USED 0x60023

/** @memberof ds_Net_NetDownReason
  * 
  * Regular MS or network initiated PDP context deactivation.
  */
#define ds_Net_NetDownReason_QDS_REGULAR_DEACTIVATION 0x60024

/** @memberof ds_Net_NetDownReason
  * 
  * The new QoS cannot be accepted by the UE that were 
  * indicated by the network in the PDP Context Modification procedure.
  */
#define ds_Net_NetDownReason_QDS_QOS_NOT_ACCEPTED 0x60025

/** @memberof ds_Net_NetDownReason
  * 
  * PDP context deactivation is caused by an error situation in the network.
  */
#define ds_Net_NetDownReason_QDS_NETWORK_FAILURE 0x60026

/** @memberof ds_Net_NetDownReason
  * 
  * This cause code is used by the network to request a PDP context 
  * reactivation after a GGSN restart. It is up to the application to reattach. 
  * The specification does not mandate it.
  */
#define ds_Net_NetDownReason_QDS_UMTS_REATTACH_REQ 0x60027
#define ds_Net_NetDownReason_QDS_FEATURE_NOT_SUPPORTED 0x60028

/** @memberof ds_Net_NetDownReason
  * 
  * There is a semantic error in the TFT operation included 
  * in a secondary PDP context activation request or an MS-initiated PDP context modification.
  */
#define ds_Net_NetDownReason_QDS_TFT_SEMANTIC_ERROR 0x60029

/** @memberof ds_Net_NetDownReason
  * 
  * There is a syntactical error in the TFT operation included 
  * in a secondary PDP context activation request or an MS-initiated PDP context modification.
  */
#define ds_Net_NetDownReason_QDS_TFT_SYNTAX_ERROR 0x6002a

/** @memberof ds_Net_NetDownReason
  * 
  * PDP context identified by the Linked TI IE the secondary PDP 
  * context activation request is not active.
  */
#define ds_Net_NetDownReason_QDS_UNKNOWN_PDP_CONTEXT 0x6002b

/** @memberof ds_Net_NetDownReason
  * 
  * There is one or more semantic errors in packet filter(s) 
  * of the TFT included in a secondary PDP context activation 
  * request or an MS-initiated PDP context modification.
  */
#define ds_Net_NetDownReason_QDS_FILTER_SEMANTIC_ERROR 0x6002c

/** @memberof ds_Net_NetDownReason
  * 
  * There is one or more syntactical errors in packet filter(s) 
  * of the TFT included in a secondary PDP context activation request 
  * or an MS-initiated PDP context modification.
  */
#define ds_Net_NetDownReason_QDS_FILTER_SYNTAX_ERROR 0x6002d

/** @memberof ds_Net_NetDownReason
  * 
  * The network has already activated a PDP context without TFT.
  */
#define ds_Net_NetDownReason_QDS_PDP_WITHOUT_ACTIVE_TFT 0x6002e

/** @memberof ds_Net_NetDownReason
  * 
  * The equipment sending this cause has received a message 
  * with a transaction identifier which is not currently in use on the MS-network interface.
  */
#define ds_Net_NetDownReason_QDS_INVALID_TRANSACTION_ID 0x60051

/** @memberof ds_Net_NetDownReason
  * 
  * Message is semantically incorrect.
  */
#define ds_Net_NetDownReason_QDS_MESSAGE_INCORRECT_SEMANTIC 0x6005f

/** @memberof ds_Net_NetDownReason
  * 
  * Mandatory information is invalid.
  */
#define ds_Net_NetDownReason_QDS_INVALID_MANDATORY_INFO 0x60060

/** @memberof ds_Net_NetDownReason
  * 
  * Message type is non-existent or is not supported.
  */
#define ds_Net_NetDownReason_QDS_MESSAGE_TYPE_UNSUPPORTED 0x60061

/** @memberof ds_Net_NetDownReason
  * 
  * Message type is not compatible with the protocol state.
  */
#define ds_Net_NetDownReason_QDS_MSG_TYPE_NONCOMPATIBLE_STATE 0x60062

/** @memberof ds_Net_NetDownReason
  * 
  * Information element is non-existent or is not implemented.
  */
#define ds_Net_NetDownReason_QDS_UNKNOWN_INFO_ELEMENT 0x60063

/** @memberof ds_Net_NetDownReason
  * 
  * Conditional IE Error 
  */
#define ds_Net_NetDownReason_QDS_CONDITIONAL_IE_ERROR 0x60064

/** @memberof ds_Net_NetDownReason
  * 
  * Message is not compatible with the current protocol state 
  */
#define ds_Net_NetDownReason_QDS_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE 0x60065

/** @memberof ds_Net_NetDownReason
  * 
  * Used to report a protocol error event only when no other 
  * cause in the protocol error class applies. 
  */
#define ds_Net_NetDownReason_QDS_PROTOCOL_ERROR 0x6006f

/** @memberof ds_Net_NetDownReason
  * 
  * This reason indicates that there was an access point name type conflict. 
  */
#define ds_Net_NetDownReason_QDS_APN_TYPE_CONFLICT 0x60070
#else /* C++ */
#include "AEEStdDef.h"

/**
  * ds Net Network Down Reason types and constants 
  */
namespace ds
{
   namespace Net
   {
      typedef int NetDownReasonType;
      
      /**
        * Type 1: MIP Network Down Reasons.
        */
      const NetDownReasonType DOWN_REASON_TYPE1 = 0x10000;
      
      /**
        * Type 2: Internal Network Down Reasons.
        */
      const NetDownReasonType DOWN_REASON_TYPE2 = 0x20000;
      
      /**
        * Type 3: Call Manager Network Down Reasons
        */
      const NetDownReasonType DOWN_REASON_TYPE3 = 0x30000;
      
      /**
        * Type 4: EAP Network Down Reasons.
        */
      const NetDownReasonType DOWN_REASON_TYPE4 = 0x40000;
      
      /**
        * Type 5: IPSEC Network Down Reasons.
        */
      const NetDownReasonType DOWN_REASON_TYPE5 = 0x50000;
      
      /**
        * Type 6: Network Down Reasons mapped to corresponding 
        * network down reasons from 3GPP TS 24.008 version 3.5.0 Release 1999
        */
      const NetDownReasonType DOWN_REASON_TYPE6 = 0x60000;
      namespace NetDownReason
      {
         
         /**
           * The network was closed for an unspecified reason.
           */
         const ::ds::Net::NetDownReasonType QDS_NOT_SPECIFIED = 0;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_REASON_UNSPECIFIED = 0x10040;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_ADMINISTRATIVELY_PROHIBITED = 0x10041;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_INSUFFICIENT_RESOURCES = 0x10042;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE = 0x10043;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_HA_AUTHENTICATION_FAILURE = 0x10044;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_REQUESTED_LIFETIME_TOO_LONG = 0x10045;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_MALFORMED_REQUEST = 0x10046;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_MALFORMED_REPLY = 0x10047;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_ENCAPSULATION_UNAVAILABLE = 0x10048;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_VJHC_UNAVAILABLE = 0x10049;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_REVERSE_TUNNEL_UNAVAILABLE = 0x1004a;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET = 0x1004b;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_DELIVERY_STYLE_NOT_SUPPORTED = 0x1004f;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_MISSING_NAI = 0x10061;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_MISSING_HA = 0x10062;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_MISSING_HOME_ADDR = 0x10063;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_UNKNOWN_CHALLENGE = 0x10068;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_MISSING_CHALLENGE = 0x10069;
         const ::ds::Net::NetDownReasonType QDS_MIP_FA_ERR_STALE_CHALLENGE = 0x1006a;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_REASON_UNSPECIFIED = 0x10080;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_ADMINISTRATIVELY_PROHIBITED = 0x10081;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_INSUFFICIENT_RESOURCES = 0x10082;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE = 0x10083;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_FA_AUTHENTICATION_FAILURE = 0x10084;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_REGISTRATION_ID_MISMATCH = 0x10085;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_MALFORMED_REQUEST = 0x10086;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_UNKNOWN_HA_ADDR = 0x10088;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_REVERSE_TUNNEL_UNAVAILABLE = 0x10089;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET = 0x1008a;
         const ::ds::Net::NetDownReasonType QDS_MIP_HA_ERR_ENCAPSULATION_UNAVAILABLE = 0x1008b;
         const ::ds::Net::NetDownReasonType QDS_MIP_ERR_REASON_UNKNOWN = 0x1ffff;
         
         /**
           * Call is terminated by the mobile device because of some internal error. 
           */
         const ::ds::Net::NetDownReasonType QDS_INTERNAL_ERROR = 0x200c9;
         
         /**
           * Call is terminated locally by the mobile. 
           */
         const ::ds::Net::NetDownReasonType QDS_INTERNAL_CALL_ENDED = 0x200ca;
         
         /**
           * Call is terminated locally by the mobile but the cause is not mapped to a network down reason yet. 
           */
         const ::ds::Net::NetDownReasonType QDS_INTERNAL_UNKNOWN_CAUSE_CODE = 0x200cb;
         
         /**
           * Reason why the call ended cannot be provided or is not mapped to a network down reason yet. 
           */
         const ::ds::Net::NetDownReasonType QDS_UNKNOWN_CAUSE_CODE = 0x200cc;
         
         /**
           * The network connection request was rejected as the previous connection of the same network is
           * currently being closed.
           */
         const ::ds::Net::NetDownReasonType QDS_CLOSE_IN_PROGRESS = 0x200cd;
         
         /**
           * This indicates that the session was terminated by the network.
           */
         const ::ds::Net::NetDownReasonType QDS_NW_INITIATED_TERMINATION = 0x200ce;
         const ::ds::Net::NetDownReasonType QDS_APP_PREEMPTED = 0x200cf;
         const ::ds::Net::NetDownReasonType QDS_CDMA_LOCK = 0x301f4;
         const ::ds::Net::NetDownReasonType QDS_INTERCEPT = 0x301f5;
         const ::ds::Net::NetDownReasonType QDS_REORDER = 0x301f6;
         const ::ds::Net::NetDownReasonType QDS_REL_SO_REJ = 0x301f7;
         const ::ds::Net::NetDownReasonType QDS_INCOM_CALL = 0x301f8;
         const ::ds::Net::NetDownReasonType QDS_ALERT_STOP = 0x301f9;
         const ::ds::Net::NetDownReasonType QDS_ACTIVATION = 0x301fa;
         const ::ds::Net::NetDownReasonType QDS_MAX_ACCESS_PROBE = 0x301fb;
         const ::ds::Net::NetDownReasonType QDS_CCS_NOT_SUPPORTED_BY_BS = 0x301fc;
         const ::ds::Net::NetDownReasonType QDS_NO_RESPONSE_FROM_BS = 0x301fd;
         const ::ds::Net::NetDownReasonType QDS_REJECTED_BY_BS = 0x301fe;
         const ::ds::Net::NetDownReasonType QDS_INCOMPATIBLE = 0x301ff;
         const ::ds::Net::NetDownReasonType QDS_ALREADY_IN_TC = 0x30200;
         const ::ds::Net::NetDownReasonType QDS_USER_CALL_ORIG_DURING_GPS = 0x30201;
         const ::ds::Net::NetDownReasonType QDS_USER_CALL_ORIG_DURING_SMS = 0x30202;
         const ::ds::Net::NetDownReasonType QDS_NO_CDMA_SRV = 0x30203;
         const ::ds::Net::NetDownReasonType QDS_MC_ABORT = 0x30204;
         const ::ds::Net::NetDownReasonType QDS_PSIST_NG = 0x30205;
         const ::ds::Net::NetDownReasonType QDS_UIM_NOT_PRESENT = 0x30206;
         const ::ds::Net::NetDownReasonType QDS_RETRY_ORDER = 0x30207;
         const ::ds::Net::NetDownReasonType QDS_ACCESS_BLOCK = 0x30208;
         const ::ds::Net::NetDownReasonType QDS_ACCESS_BLOCK_ALL = 0x30209;
         const ::ds::Net::NetDownReasonType QDS_IS707B_MAX_ACC = 0x3020a;
         const ::ds::Net::NetDownReasonType QDS_CONF_FAILED = 0x303e8;
         const ::ds::Net::NetDownReasonType QDS_INCOM_REJ = 0x303e9;
         const ::ds::Net::NetDownReasonType QDS_NO_GW_SRV = 0x303ea;
         const ::ds::Net::NetDownReasonType QDS_NO_GPRS_CONTEXT = 0x303eb;
         const ::ds::Net::NetDownReasonType QDS_ILLEGAL_MS = 0x303ec;
         const ::ds::Net::NetDownReasonType QDS_ILLEGAL_ME = 0x303ed;
         const ::ds::Net::NetDownReasonType QDS_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED = 0x303ee;
         const ::ds::Net::NetDownReasonType QDS_GPRS_SERVICES_NOT_ALLOWED = 0x303ef;
         const ::ds::Net::NetDownReasonType QDS_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK = 0x303f0;
         const ::ds::Net::NetDownReasonType QDS_IMPLICITLY_DETACHED = 0x303f1;
         const ::ds::Net::NetDownReasonType QDS_PLMN_NOT_ALLOWED = 0x303f2;
         const ::ds::Net::NetDownReasonType QDS_LA_NOT_ALLOWED = 0x303f3;
         const ::ds::Net::NetDownReasonType QDS_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN = 0x303f4;
         const ::ds::Net::NetDownReasonType QDS_PDP_DUPLICATE = 0x303f5;
         const ::ds::Net::NetDownReasonType QDS_UE_RAT_CHANGE = 0x303f6;
         const ::ds::Net::NetDownReasonType QDS_CONGESTION = 0x303f7;
         const ::ds::Net::NetDownReasonType QDS_NO_PDP_CONTEXT_ACTIVATED = 0x303f8;
         const ::ds::Net::NetDownReasonType QDS_ACCESS_CLASS_DSAC_REJECTION = 0x303f9;
         const ::ds::Net::NetDownReasonType QDS_CD_GEN_OR_BUSY = 0x305dc;
         const ::ds::Net::NetDownReasonType QDS_CD_BILL_OR_AUTH = 0x305dd;
         const ::ds::Net::NetDownReasonType QDS_CHG_HDR = 0x305de;
         const ::ds::Net::NetDownReasonType QDS_EXIT_HDR = 0x305df;
         const ::ds::Net::NetDownReasonType QDS_HDR_NO_SESSION = 0x305e0;
         const ::ds::Net::NetDownReasonType QDS_HDR_ORIG_DURING_GPS_FIX = 0x305e1;
         const ::ds::Net::NetDownReasonType QDS_HDR_CS_TIMEOUT = 0x305e2;
         const ::ds::Net::NetDownReasonType QDS_HDR_RELEASED_BY_CM = 0x305e3;
         const ::ds::Net::NetDownReasonType QDS_COLLOC_ACQ_FAIL = 0x305e4;
         const ::ds::Net::NetDownReasonType QDS_OTASP_COMMIT_IN_PROG = 0x305e5;
         const ::ds::Net::NetDownReasonType QDS_NO_HYBR_HDR_SRV = 0x305e6;
         const ::ds::Net::NetDownReasonType QDS_HDR_NO_LOCK_GRANTED = 0x305e7;
         const ::ds::Net::NetDownReasonType QDS_HOLD_OTHER_IN_PROG = 0x305e8;
         const ::ds::Net::NetDownReasonType QDS_HDR_FADE = 0x305e9;
         const ::ds::Net::NetDownReasonType QDS_HDR_ACC_FAIL = 0x305ea;
         const ::ds::Net::NetDownReasonType QDS_CLIENT_END = 0x307d0;
         const ::ds::Net::NetDownReasonType QDS_NO_SRV = 0x307d1;
         const ::ds::Net::NetDownReasonType QDS_FADE = 0x307d2;
         const ::ds::Net::NetDownReasonType QDS_REL_NORMAL = 0x307d3;
         const ::ds::Net::NetDownReasonType QDS_ACC_IN_PROG = 0x307d4;
         const ::ds::Net::NetDownReasonType QDS_ACC_FAIL = 0x307d5;
         const ::ds::Net::NetDownReasonType QDS_REDIR_OR_HANDOFF = 0x307d6;
         const ::ds::Net::NetDownReasonType QDS_OFFLINE = 0x309c4;
         const ::ds::Net::NetDownReasonType QDS_EMERGENCY_MODE = 0x309c5;
         const ::ds::Net::NetDownReasonType QDS_PHONE_IN_USE = 0x309c6;
         const ::ds::Net::NetDownReasonType QDS_INVALID_MODE = 0x309c7;
         const ::ds::Net::NetDownReasonType QDS_INVALID_SIM_STATE = 0x309c8;
         const ::ds::Net::NetDownReasonType QDS_NO_COLLOC_HDR = 0x309c9;
         const ::ds::Net::NetDownReasonType QDS_CALL_CONTROL_REJECTED = 0x309ca;
         
         /**
           * A general error code indicating failure 
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_CLIENT_ERR_UNABLE_TO_PROCESS = 0x41389;
         
         /**
           * The peer does not support any of the versions listed in AT_VERSION_LIST 
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_CLIENT_ERR_UNSUPPORTED_VERS = 0x4138a;
         
         /**
           * The peer's policy requires more triplets than the server included in AT_RAND 
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_CLIENT_ERR_INSUFFICIENT_CHALLANGES = 0x4138b;
         
         /**
           * The peer believes that the RAND challenges included in AT_RAND were not fresh 
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_CLIENT_ERR_RAND_NOT_FRESH = 0x4138c;
         
         /**
           * Notification code indicating general failure after authentication 
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_NOTIFICATION_GENERAL_FAILURE_AFTER_AUTH = 0x4138d;
         
         /**
           * Notification code indicating general failure before authentication 
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_NOTIFICATION_GENERAL_FAILURE_BEFORE_AUTH = 0x4138e;
         
         /**
           * User has been temporarily denied access to the requested service.
           * (Implies failure, used after successful authentication.)
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_NOTIFICATION_TEMP_DENIED_ACCESS = 0x4138f;
         
         /**
           * User has not subscribed to the requested service.
           * (Implies failure, used after successful authentication.)
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_NOTIFICATION_USER_NOT_SUBSCRIBED = 0x41390;
         
         /**
           * Success.  User has been successfully authenticated 
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_SUCCESS = 0x41391;
         
         /**
           * Notification error code while parsing an EAP identity indicating that 
           * NAI realm portion is unavailable in environments where a realm is required.
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_NOTIFICATION_REALM_UNAVAILABLE = 0x41392;
         
         /**
           * Notification error code while parsing an EAP identity indicating 
           * that username is unavailable.
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_NOTIFICATION_USER_NAME_UNAVAILABLE = 0x41393;
         
         /**
           * Notification error code indicating that the call has been barred. 
           */
         const ::ds::Net::NetDownReasonType QDS_EAP_NOTIFICATION_CALL_BARRED = 0x41394;
         
         /**
           * The call failed because the PDIF is unreachable. 
           */
         const ::ds::Net::NetDownReasonType QDS_IPSEC_GW_UNREACHABLE = 0x513ed;
         
         /**
           * The call failed because of authentication failure during IPSEC tunnel establishment. 
           */
         const ::ds::Net::NetDownReasonType QDS_IPSEC_AUTH_FAILED = 0x513ee;
         
         /**
           * The call failed because the certificate is invalid. 
           */
         const ::ds::Net::NetDownReasonType QDS_IPSEC_CERT_INVALID = 0x513ef;
         
         /**
           * The call failed because of internal error in IPSEC. 
           */
         const ::ds::Net::NetDownReasonType QDS_IPSEC_INTERNAL_ERROR = 0x513f0;
         
         /**
           * This reason indicates that the operator determined barring.
           */
         const ::ds::Net::NetDownReasonType QDS_OPERATOR_DETERMINED_BARRING = 0x60008;
         const ::ds::Net::NetDownReasonType QDS_LLC_SNDCP_FAILURE = 0x60019;
         
         /**
           * PDP context activation request, secondary PDP context activation request or PDP context 
           * modification request cannot be accepted due to insufficient resources.
           */
         const ::ds::Net::NetDownReasonType QDS_INSUFFICIENT_RESOURCES = 0x6001a;
         
         /**
           * Requested service was rejected by the external packet data network 
           * because the access point name was not included although required 
           * or if the access point name could not be resolved.
           */
         const ::ds::Net::NetDownReasonType QDS_UNKNOWN_APN = 0x6001b;
         
         /**
           * Requested service was rejected by the external packet data network 
           * because the PDP address or type could not be recognized.
           */
         const ::ds::Net::NetDownReasonType QDS_UNKNOWN_PDP = 0x6001c;
         
         /**
           * Requested service was rejected by the external packet data network 
           * due to a failed user authentication.
           */
         const ::ds::Net::NetDownReasonType QDS_AUTH_FAILED = 0x6001d;
         
         /**
           * GGSN has rejected the activation request.
           */
         const ::ds::Net::NetDownReasonType QDS_GGSN_REJECT = 0x6001e;
         
         /**
           * Activation request rejected with unspecified reason.
           */
         const ::ds::Net::NetDownReasonType QDS_ACTIVATION_REJECT = 0x6001f;
         
         /**
           * The requested service option is not supported by the PLMN.
           */
         const ::ds::Net::NetDownReasonType QDS_OPTION_NOT_SUPPORTED = 0x60020;
         
         /**
           * The requested service option is not subscribed for.
           */
         const ::ds::Net::NetDownReasonType QDS_OPTION_UNSUBSCRIBED = 0x60021;
         
         /**
           * MSC cannot service the request because of temporary 
           * outage of one or more functions required for supporting the service. 
           */
         const ::ds::Net::NetDownReasonType QDS_OPTION_TEMP_OOO = 0x60022;
         
         /**
           * MNSAPI requested by the MS in the PDP context activation 
           * request is already used by another active PDP context of this MS.
           */
         const ::ds::Net::NetDownReasonType QDS_NSAPI_ALREADY_USED = 0x60023;
         
         /**
           * Regular MS or network initiated PDP context deactivation.
           */
         const ::ds::Net::NetDownReasonType QDS_REGULAR_DEACTIVATION = 0x60024;
         
         /**
           * The new QoS cannot be accepted by the UE that were 
           * indicated by the network in the PDP Context Modification procedure.
           */
         const ::ds::Net::NetDownReasonType QDS_QOS_NOT_ACCEPTED = 0x60025;
         
         /**
           * PDP context deactivation is caused by an error situation in the network.
           */
         const ::ds::Net::NetDownReasonType QDS_NETWORK_FAILURE = 0x60026;
         
         /**
           * This cause code is used by the network to request a PDP context 
           * reactivation after a GGSN restart. It is up to the application to reattach. 
           * The specification does not mandate it.
           */
         const ::ds::Net::NetDownReasonType QDS_UMTS_REATTACH_REQ = 0x60027;
         const ::ds::Net::NetDownReasonType QDS_FEATURE_NOT_SUPPORTED = 0x60028;
         
         /**
           * There is a semantic error in the TFT operation included 
           * in a secondary PDP context activation request or an MS-initiated PDP context modification.
           */
         const ::ds::Net::NetDownReasonType QDS_TFT_SEMANTIC_ERROR = 0x60029;
         
         /**
           * There is a syntactical error in the TFT operation included 
           * in a secondary PDP context activation request or an MS-initiated PDP context modification.
           */
         const ::ds::Net::NetDownReasonType QDS_TFT_SYNTAX_ERROR = 0x6002a;
         
         /**
           * PDP context identified by the Linked TI IE the secondary PDP 
           * context activation request is not active.
           */
         const ::ds::Net::NetDownReasonType QDS_UNKNOWN_PDP_CONTEXT = 0x6002b;
         
         /**
           * There is one or more semantic errors in packet filter(s) 
           * of the TFT included in a secondary PDP context activation 
           * request or an MS-initiated PDP context modification.
           */
         const ::ds::Net::NetDownReasonType QDS_FILTER_SEMANTIC_ERROR = 0x6002c;
         
         /**
           * There is one or more syntactical errors in packet filter(s) 
           * of the TFT included in a secondary PDP context activation request 
           * or an MS-initiated PDP context modification.
           */
         const ::ds::Net::NetDownReasonType QDS_FILTER_SYNTAX_ERROR = 0x6002d;
         
         /**
           * The network has already activated a PDP context without TFT.
           */
         const ::ds::Net::NetDownReasonType QDS_PDP_WITHOUT_ACTIVE_TFT = 0x6002e;
         
         /**
           * The equipment sending this cause has received a message 
           * with a transaction identifier which is not currently in use on the MS-network interface.
           */
         const ::ds::Net::NetDownReasonType QDS_INVALID_TRANSACTION_ID = 0x60051;
         
         /**
           * Message is semantically incorrect.
           */
         const ::ds::Net::NetDownReasonType QDS_MESSAGE_INCORRECT_SEMANTIC = 0x6005f;
         
         /**
           * Mandatory information is invalid.
           */
         const ::ds::Net::NetDownReasonType QDS_INVALID_MANDATORY_INFO = 0x60060;
         
         /**
           * Message type is non-existent or is not supported.
           */
         const ::ds::Net::NetDownReasonType QDS_MESSAGE_TYPE_UNSUPPORTED = 0x60061;
         
         /**
           * Message type is not compatible with the protocol state.
           */
         const ::ds::Net::NetDownReasonType QDS_MSG_TYPE_NONCOMPATIBLE_STATE = 0x60062;
         
         /**
           * Information element is non-existent or is not implemented.
           */
         const ::ds::Net::NetDownReasonType QDS_UNKNOWN_INFO_ELEMENT = 0x60063;
         
         /**
           * Conditional IE Error 
           */
         const ::ds::Net::NetDownReasonType QDS_CONDITIONAL_IE_ERROR = 0x60064;
         
         /**
           * Message is not compatible with the current protocol state 
           */
         const ::ds::Net::NetDownReasonType QDS_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE = 0x60065;
         
         /**
           * Used to report a protocol error event only when no other 
           * cause in the protocol error class applies. 
           */
         const ::ds::Net::NetDownReasonType QDS_PROTOCOL_ERROR = 0x6006f;
         
         /**
           * This reason indicates that there was an access point name type conflict. 
           */
         const ::ds::Net::NetDownReasonType QDS_APN_TYPE_CONFLICT = 0x60070;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_DOWNREASONS_DEF_H
