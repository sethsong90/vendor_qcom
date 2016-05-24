//=============================================================================
//
// COPYRIGHT 2012-2013 Qualcomm Technologies, Inc.
// All rights reserved. Qualcomm Technologies proprietary and confidential.
//
// $Header: //depot/asic/msmshared/users/crd_dash/QTA/inc/HttpHandlerFactory.h#3 $
// $DateTime: 2013/09/06 16:09:31 $
// $Change: 4395644 $
//=============================================================================

#ifndef HttpHandlerFactory_H
#define HttpHandlerFactory_H

#include "QTATypes.h"            //Defines all the QTA types
#include "IHttpHandler.h"        //Defines the IHttpHandler interface
#include "IHttpStatusNotificationHandler.h"        //Defines the IHttpStatusNotificationHandler interface
#include "HTTPCookieStore.h"    //Defines the CookieManager
#include "IStreamManager.h"     //Defines the IStreamManager interface

namespace QTA
{
///
/// @brief Factory to create QTA or regular HttpStackInterface objects.
///        This is NOT threadsafe.
///
class HttpHandlerFactory
{
public:

   //Defines the enum for specifying the type of object (RA or regular
   //HTTPStackInterface) to instantiate.
   typedef enum {
      HTTP_STACK_QTA = 0,       //Instantiate QTA
      HTTP_STACK_DEFAULT = 1   //Instantiate Regular HTTPStackInterface wrapper
   } HttpStackTypeEnum;

public :

   ///
   /// @brief This method should be used to create the objects implementing IHttpHandler interface.
   ///        This can be either the QTA or the HTTPStackInterface wrapper.
   /// @param ppIface: [InOut] Pointer to a pointer to the IHttpHandler implementation.
   ///        The Source would pass it and this function would create
   ///        the instance and return it
   /// @param pRaToSource: [In] Pointer to an implementation that implements the
   ///        QTAToSource interface
   /// @param cookieMgr: [In] HttpCookie manager
   /// @param pStreamManager: [In] Pointer to the IStreamManger to push notifications. Set to
   ///        NULL if IStreamManager is not available.
   /// @param stackType: [In] Used to select the implementation for IHttpHandler interface.
   ///
   /// @return
   /// HTTP_SUCCESS: Successfully create the specified implementation for IHttpHandler interface
   /// HTTP_FAILURE: Failed to create the specified implementation for IHttpHandler interface
   ///
   static HttpReturnCode CreateInstance( IHttpHandler ** ppIface,
                                         IHttpStatusNotificationHandler * pRaToSource,
                                         HTTPCookieMgr& cookieMgr,
                                         IStreamManager * pStreamManager = NULL,
                                         HttpStackTypeEnum stackType = HTTP_STACK_DEFAULT);

   ///
   /// @brief Should be used to delete the implementation objects for IHttpHandler interface
   ///
   static void DeleteInstance(IHttpHandler* pIface);

private:
   ///
   /// @brief Default Ctor
   HttpHandlerFactory(){}

   ///
   /// @brief Default Dtor
   ///
   ~HttpHandlerFactory(){}

private:
   /// Pointer to singleton instance of QTA
   static IHttpHandler* mIHttpHandlerPtr;

   /// Refence count users using the singleton
   static uint32 mRefCount;

};
}

#endif
