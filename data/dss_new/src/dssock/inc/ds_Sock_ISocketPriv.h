#ifndef DS_SOCK_ISOCKETPRIV_H
#define DS_SOCK_ISOCKETPRIV_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"

/**
  * This is a privileged external interface for ds_Socket.
  *
  * This interface exposes extra socket functionality for use by BrewMP.
  */
#define ds_Sock_AEEIID_ISocketPriv 0x109dcf2
#define INHERIT_ds_Sock_ISocketPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*SetCloseOnNullBearerTimeout)(iname* _pif, int timeoutSec); \
   AEEResult (*StartNetwork)(iname* _pif)
AEEINTERFACE_DEFINE(ds_Sock_ISocketPriv);

/** @memberof ds_Sock_ISocketPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_ISocketPriv_AddRef(ds_Sock_ISocketPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketPriv)->AddRef(_pif);
}

/** @memberof ds_Sock_ISocketPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_ISocketPriv_Release(ds_Sock_ISocketPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketPriv)->Release(_pif);
}

/** @memberof ds_Sock_ISocketPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Sock_ISocketPriv_QueryInterface(ds_Sock_ISocketPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Sock_ISocketPriv
  * 
  * This function enables the feature to close the socket after
  			 * a transition to Null bearer was detected and the bearer has 
  			 * remained Null during the given timeout period.
  			 *
  			 * This internal feature is used by BrewMP Out-of-Coverage feature.
  			 *
  * @param _pif Pointer to interface
  * @param timeoutSec Timeout, in seconds. An input value of 0 means
  			 *        the feature will remain disabled.
  			 * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  		    */
static __inline AEEResult ds_Sock_ISocketPriv_SetCloseOnNullBearerTimeout(ds_Sock_ISocketPriv* _pif, int timeoutSec)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketPriv)->SetCloseOnNullBearerTimeout(_pif, timeoutSec);
}

/** @memberof ds_Sock_ISocketPriv
  * 
  * This function brings up the underlying Network of Socket if 
  			 * applicable.
  *
  			 * This is required for BREW Networking backward compatibility 
  			 * for socket operations that require to bring UP the network 
  			 * before Bind is applied on the socket (e.g. Realize). 
  			 * Note: The network is brought up automatically when Bind is 
  			 * applied on the socket.
  			 *
  * @param _pif Pointer to interface
  * @param None
  * @retval AEE_SUCCESS The operation completed successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocketPriv_StartNetwork(ds_Sock_ISocketPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketPriv)->StartNetwork(_pif);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"

/**
  * This is a privileged external interface for ds::Socket.
  *
  * This interface exposes extra socket functionality for use by BrewMP.
  */
namespace ds
{
   namespace Sock
   {
      const ::AEEIID AEEIID_ISocketPriv = 0x109dcf2;
      struct ISocketPriv : public ::IQI
      {
         
         /**
           * This function enables the feature to close the socket after
           			 * a transition to Null bearer was detected and the bearer has 
           			 * remained Null during the given timeout period.
           			 *
           			 * This internal feature is used by BrewMP Out-of-Coverage feature.
           			 *
           * @param timeoutSec Timeout, in seconds. An input value of 0 means
           			 *        the feature will remain disabled.
           			 * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           		    */
         virtual ::AEEResult AEEINTERFACE_CDECL SetCloseOnNullBearerTimeout(int timeoutSec) = 0;
         
         /**
           * This function brings up the underlying Network of Socket if 
           			 * applicable.
           *
           			 * This is required for BREW Networking backward compatibility 
           			 * for socket operations that require to bring UP the network 
           			 * before Bind is applied on the socket (e.g. Realize). 
           			 * Note: The network is brought up automatically when Bind is 
           			 * applied on the socket.
           			 *
           * @param None
           * @retval AEE_SUCCESS The operation completed successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL StartNetwork() = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_SOCK_ISOCKETPRIV_H
