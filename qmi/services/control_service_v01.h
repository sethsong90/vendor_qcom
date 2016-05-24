#ifndef CTL_SERVICE_H
#define CTL_SERVICE_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        C O N T R O L _ S E R V I C E _ V 0 1  . H

GENERAL DESCRIPTION
  This is the public header file which defines the ctl service Data structures.

 Copyright (c) 2010 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.

 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====* 
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY 
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*
 * This header file defines the types and structures that were defined in 
 * cat. It contains the constant values defined, enums, structures,
 * messages, and service message IDs (in that order) Structures that were 
 * defined in the IDL as messages contain mandatory elements, optional 
 * elements, a combination of mandatory and optional elements (mandatory 
 * always come before optionals in the structure), or nothing (null message)
 *  
 * An optional element in a message is preceded by a uint8_t value that must be
 * set to true if the element is going to be included. When decoding a received
 * message, the uint8_t values will be set to true or false by the decode
 * routine, and should be checked before accessing the values that they
 * correspond to. 
 *  
 * Variable sized arrays are defined as static sized arrays with an unsigned
 * integer (32 bit) preceding it that must be set to the number of elements
 * in the array that are valid. For Example:
 *  
 * uint32_t test_opaque_len;
 * uint8_t test_opaque[16];
 *  
 * If only 4 elements are added to test_opaque[] then test_opaque_len must be
 * set to 4 before sending the message.  When decoding, the _len value is set 
 * by the decode routine and should be checked so that the correct number of 
 * elements in the array will be accessed. 
 */

/* This file was generated with Tool version 02.01 
   It was generated on: Wed Nov 10 2010
   From IDL File: control_service_v01.idl */

#include <stdint.h>
#include "qmi_idl_lib.h"
#include "common_v01.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Version Number of the IDL used to generate this file */
#define CTL_V01_IDL_MAJOR_VERS 01
#define CTL_V01_IDL_MINOR_VERS 01
#define CTL_V01_IDL_TOOL_VERS 02

/* Const Definitions */

#define CTL_QMUX_MAX_VERSIONS_V01 255
#define CTL_ADDENDUM_MAX_LABEL_V01 255

typedef struct {

  /* Mandatory */
  /*  Host-unique QMI instance for this device driver */
  uint8_t host_driver_instance;
}ctl_set_instance_id_req_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  uint16_t qmi_link_id;  /*  Unique QMI link ID assigned to the 
                                             link over which the message is 
                                             exchanged; the upper byte is 
                                             assigned by the QMI_CTL service 
                                             and the lower byte is assigned by 
                                             the host (the value passed in the 
                                             request)  */

  /* Mandatory */
  qmi_response_type_v01 resp;  /*  Standard response type.  */
}ctl_set_instance_id_resp_msg_v01;  /* Message */

typedef struct {

  uint8_t qmi_svc_type;  /*  QMI service type, as defined in [Q1] */

  uint16_t major_ver;  /*  Major version number */

  uint16_t minor_ver;  /*  Minor version number */
}qmi_version_type_v01;  /* Type */

typedef struct {

  /*  Label describing the addendum */
  char addendum_label[CTL_ADDENDUM_MAX_LABEL_V01 + 1];

  /*  Addendum versions */
  uint32_t qmi_addendum_version_len;	/* Must be set to # of elements in qmi_addendum_version */
  qmi_version_type_v01 qmi_addendum_version[CTL_QMUX_MAX_VERSIONS_V01];
}qmi_service_version_list_type_v01;  /* Type */

/*
 * ctl_get_version_info_req_msg is empty
 * typedef struct {
 * }ctl_get_version_info_req_msg_v01;
 */

typedef struct {

  /* Mandatory */
  /*  Service versions */
  uint32_t qmi_service_version_len;	/* Must be set to # of elements in qmi_service_version */
  qmi_version_type_v01 qmi_service_version[CTL_QMUX_MAX_VERSIONS_V01];

  /* Mandatory */
  /*  Standard response type */
  qmi_response_type_v01 resp;

  /* Optional */
  /*  Service addendum versions                                     */
  uint8_t addendum_version_list_valid;	/* Must be set to true if addendum_version_list is being passed */
  qmi_version_type_v01 addendum_version_list;
}ctl_get_version_info_resp_msg_v01;  /* Message */

typedef struct {

  uint8_t qmi_svc_type;  /*  QMI service type, as defined in [Q1] */

  uint8_t client_id;  /*  Client ID assigned by the service */
}qmi_client_id_type_v01;  /* Type */

typedef struct {

  /* Mandatory */
  uint8_t qmi_svc_type;  /*  QMI service type, as defined in [Q1] */
}ctl_get_client_id_req_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  qmi_client_id_type_v01 qmi_svc_type;

  /* Mandatory */
  qmi_response_type_v01 resp;  /*  Standard response type. */
}ctl_get_client_id_resp_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  qmi_client_id_type_v01 qmi_svc_type;  /*  Client ID to release            */
}ctl_release_client_id_req_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  /*  Client ID released by the service */
  qmi_client_id_type_v01 qmi_svc_type;

  /* Mandatory */
  qmi_response_type_v01 resp;  /*  Standard response type */
}ctl_release_client_id_resp_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  qmi_client_id_type_v01 qmi_svc_type;  /*  Revoked client ID           */
}ctl_revoke_client_id_ind_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  qmi_client_id_type_v01 qmi_svc_type;  /*  Revoked client ID           */
}ctl_invalid_client_id_ind_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  uint16_t data_format;  /*  Data format used by the driver
                                             - 0 - No QoS flow header
                                             - 1 - QoS flow header present
                                         */

  /* Optional */
  uint8_t link_prot_valid;	/* Must be set to true if link_prot is being passed */
  uint16_t link_prot;  /*  Bitmask of link protocols 
                                             supported by the driver. (If
                                             multiple protocols are supported, 
                                             they are OR'ed together as a 
                                             mask.)
                                             - 0x1 - 802.3
                                             - 0x2 - IP   */
}ctl_set_data_format_req_msg_v01;  /* Message */

typedef struct {

  /* Mandatory */
  qmi_response_type_v01 resp;  /*  Standard response type */

  /* Optional */
  uint8_t link_prot_valid;	/* Must be set to true if link_prot is being passed */
  uint16_t link_prot;  /*  The link protocol used by the 
                                                 driver. Only one protocol
                                                 in the response indicates the 
                                                 mode that should be used.
                                                 - 0x1 - 802.3 
                                                 - 0x2 - IP  
                                              */
}ctl_set_data_format_resp_msg_v01;  /* Message */

/*Service Message Definition*/
#define QMI_CTL_SET_INSTANCE_ID_REQ_V01 0x0001
#define QMI_CTL_SET_INSTANCE_ID_RESP_V01 0x0001
#define QMI_CTL_GET_VERSION_INFO_REQ_V01 0x0002
#define QMI_CTL_GET_VERSION_INFO_RESP_V01 0x0002
#define QMI_CTL_GET_CLIENT_ID_REQ_V01 0x0003
#define QMI_CTL_GET_CLIENT_ID_RESP_V01 0x0003
#define QMI_CTL_RELEASE_CLIENT_ID_REQ_V01 0x0004
#define QMI_CTL_RELEASE_CLIENT_ID_RESP_V01 0x0004
#define QMI_CTL_REVOKE_CLIENT_ID_IND_V01 0x0005
#define QMI_CTL_INVALID_CLIENT_ID_IND_V01 0x0006
#define QMI_CTL_SET_DATA_FORMAT_REQ_V01 0x0007
#define QMI_CTL_SET_DATA_FORMAT_RESP_V01 0x0007

/* Service Object Accessor */
qmi_idl_service_object_type ctl_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version );
 
#define ctl_get_service_object_v01( ) \
          ctl_get_service_object_internal_v01( \
            CTL_V01_IDL_MAJOR_VERS, CTL_V01_IDL_MINOR_VERS, \
            CTL_V01_IDL_TOOL_VERS )


#ifdef __cplusplus
}
#endif
#endif

