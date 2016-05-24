//=============================================================================
//
// COPYRIGHT 2012-2013 Qualcomm Technologies, Inc.
// All rights reserved. Qualcomm Technologies proprietary and confidential.
//
// $Header: //depot/asic/msmshared/users/crd_dash/QTA/inc/QTATypes.h#2 $
// $DateTime: 2013/09/06 13:24:17 $
// $Change: 4394368 $
//=============================================================================

#ifndef QTA_TYPES_H
#define QTA_TYPES_H

#include "HTTPStackInterface.h" //defines the HTTP related types

namespace QTA
{

//Return codes used within QTA to indicate Http failures
typedef  video::HTTPReturnCode HttpReturnCode;

//Http Method type like GET, POST etc.
typedef video::HTTPMethodType HttpMethodType;

//Http Socket options lile Blocking/NonBlocking
typedef video::HTTPSocketType HttpSocketType;

//Http Stack option
typedef video::HTTPStackOption HttpStackOption;

//Http Stack Notify Code
typedef video::HTTPStackNotifyCode HttpStackNotifyCode;

//The struct for message returned in cal
typedef video::HTTPStackNotificationCbData HttpStackNotificationCbData;

//Defines the enum for specifying the priority for processing
//http requests to the QTA.
typedef enum {
   REQ_PRIORITY_LOW  = 0,
   REQ_PRIORITY_MED  = 1,
   REQ_PRIORITY_HIGH = 2,
   REQ_PRIORITY_MAX  = 3
} HttpRequestPriority;

//Defines the return status code for the algorithms
typedef enum {
   STATUS_SUCCESS  = 0,
   STATUS_FAILURE  = 1
} ReturnStatusCode;

//Enums for unknowns
enum {
   BW_ESTIMATE_UNKNOWN = MAX_UINT32,
   RTT_ESTIMATE_UNKNOWN = MAX_UINT32
};


} //end of namespace QTA

#endif //QTA_TYPES_H
