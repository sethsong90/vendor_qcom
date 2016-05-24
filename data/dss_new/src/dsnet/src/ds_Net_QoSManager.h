#ifndef DS_NET_QOS_MANAGER_H
#define DS_NET_QOS_MANAGER_H

/*===========================================================================
  @file QoSManager.h

  This file defines the class that implements the IQoSManager interface.

  The QoSManager class (ds::Net::QoSManager) implements the following interfaces:
  IQI
  IEventManager
  IQoSManager

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSManager.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-12-22 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Utils_Factory.h"
#include "ds_Net_IQoSManager.h"
#include "ds_Net_IQoSManagerPriv.h"
#include "ds_Net_Handle.h"
#include "ds_Net_QoSDefault.h"
#include "ds_Net_Utils.h"
#include "ds_Net_MemManager.h"
#include "ds_Net_QoSManagerJson.h"
#include "ds_Net_INetworkFactory.h"

/*===========================================================================
                     FORWARD DECLERATION
===========================================================================*/
struct IPrivSet;

/*===========================================================================

                     PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace ds
{
namespace Net
{
/*lint -esym(1510, IQoSManager) */
/*lint -esym(1510, IQI) */
class QoSManager : public IQoSManager,
                   public IQoSManagerPriv, 
                   public Handle
{
private:
  QoSDefault*             mpQoSDefault;
  QoSManagerJson          mQoSManagerJason;
  ISignalBus *            mpSigBusProfilesChanged;
  IPrivSet *              mpPrivSet;
  NetworkModeType         mNetworkMode;

public:
  QoSManager
  (
    int32           ifaceHandle,
    IPrivSet      * pPrivSet,
    NetworkModeType networkMode
  );

  /*! 
  @brief
  Destructor of QoSManager object. 
  */
  virtual void Destructor
  (
    void 
  )
  throw();

  /*!
  @brief
  Dummy destructor. Only used to free memory. 
  
  @details
  For objects that support weak references, destruction of the object and
  freeing up of memory can happen at different times. Hence Destructor()
  is the one which performs actual destruction of the object, where as 
  ~Object() would perform memory cleanup. 
  @params     None.
  @return     None.
  */
  virtual ~QoSManager() throw();

  virtual boolean Process
  (
    void* userDataPtr
  );

  /*-------------------------------------------------------------------------
    Inherited functions from IQoSManager and IQoSManagerPriv.
  -------------------------------------------------------------------------*/
  virtual ds::ErrorType CDECL RequestSecondary
  (
    const QoSSpecType*             requestedQoSSpec,
    IQoSSecondary**                    session
  );

  virtual ds::ErrorType CDECL RequestBundle 
  (
    const QoSSpecType* specs, 
    int specsLen, 
    QoSRequestOpCodeType opCode,
    IQoSSecondariesOutput** sessions
  );

  virtual ds::ErrorType CDECL GetQosDefault
  (
    IQoSDefault**                      qoSDefaultObj
  );

  virtual ds::ErrorType CDECL Request
  (
    const char* specs, 
    QoSRequestOpCodeType opCode,
    IQoSSecondariesOutput** qosSessions,
    char* errSpec,
    int errSpecLen, 
    int* errSpecLenReq
  );

  virtual ds::ErrorType CDECL Close
  (
    IQoSSecondariesInput* qosSessions
  );

  virtual ds::ErrorType CDECL Resume
  (
    IQoSSecondariesInput* qosSessions
  );

  virtual ds::ErrorType CDECL Suspend
  (
    IQoSSecondariesInput* qosSessions
  );

  virtual ds::ErrorType CDECL GetSupportedProfiles
  (
    QoSProfileIdType                        *profiles,
    int                                     profilesLen,
    int*                                    profilesLenReq
  );
  
  virtual ds::ErrorType CDECL CreateQoSSecondariesInput 
  (
    IQoSSecondariesInput** newQoSSecondariesInput
  );

  /*-------------------------------------------------------------------------
    Forwarders for Handle.
  -------------------------------------------------------------------------*/
  virtual ds::ErrorType CDECL OnStateChange
  (
    ISignal*                                signalObj,
    ds::Net::EventType                      eventID,
    IQI**                                   regObj
  );

  virtual ds::ErrorType GetSignalBus
  (
    ds::Net::EventType  eventID,
    ISignalBus **       ppISigBus
  );

  /*-------------------------------------------------------------------------
    Overload new/delete operators.
  -------------------------------------------------------------------------*/
  DSNET_OVERLOAD_OPERATORS(PS_MEM_DS_NET_QOS_MANAGER)

  /*-------------------------------------------------------------------------
    Macro to implement IWeakRef/IQI methods
  -------------------------------------------------------------------------*/
    DS_UTILS_IWEAKREF_IMPL_DEFAULTS_NO_QI()
    
    virtual int CDECL QueryInterface (AEEIID iid, void **ppo);                                                                      

};/* class QoSManager */
} /* namespace Net */
} /* namespace ds */

#endif /* DS_NET_QOS_MANAGER_H */

