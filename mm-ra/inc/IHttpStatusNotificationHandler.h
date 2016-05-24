//=============================================================================
//
// COPYRIGHT 2012-2013 Qualcomm Technologies, Inc.
// All rights reserved. Qualcomm Technologies proprietary and confidential.
//
// $Header: //depot/asic/msmshared/users/crd_dash/QTA/inc/IHttpStatusNotificationHandler.h#3 $
// $DateTime: 2013/09/06 16:09:31 $
// $Change: 4395644 $
//=============================================================================

#ifndef IHttpStatusNotificationHandler_H
#define IHttpStatusNotificationHandler_H

#include "QTATypes.h" //defines HttpStackNotifyCode

namespace QTA
{
///
/// @brief Defines the interface between QTA and Source. The source should
///        implement this interface to receive notifications from QTA.
///
class IHttpStatusNotificationHandler {
public :

   ///
   /// @brief Default Ctor
   ///
   IHttpStatusNotificationHandler(){}

   ///
   /// @brief Default Dtor
   ///
   virtual ~IHttpStatusNotificationHandler(){}

   ////    Operations    ////

   ///
   /// @brief Processes notification from the QTA
   /// @param RaNotifyCode: [In] Code identifying the notification
   /// @param pCbData: [In] Blob of data that should be interpreted based on the RaNotifyCode
   ///
   virtual bool Notify(HttpStackNotifyCode raHttpNotifyCode,
                       void *pCbData) = 0;


};
}

#endif
