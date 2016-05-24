#ifndef DS_NET_QOS_H
#define DS_NET_QOS_H
/*===========================================================================
  @file QoS.h

  This file defines the class that implements the IQoS interface.

  TODO: Detailed explaination about the class here.

  Copyright (c) 2008 - 2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoS.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-03-25 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Net_IQoS.h"
#include "ds_Net_IQoS1x.h"
#include "ds_Net_IQoSPriv.h"
#include "ds_Net_Handle.h"
#include "ds_Net_PhysLink.h"
#include "ds_Net_Platform.h"
#include "ds_Net_IQoSFlowPriv.h"
#include "ds_Net_QoSJson.h"

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

/*lint -esym(1510, IQoS) */
/*lint -esym(1510, IQI) */
class QoS : public IQoS,
            public IQoSPriv,
            public Handle
{
private:
  int32                      mIfaceHandle;
  PhysLink *                 mpPhysLink;
  IQoS1x *                   mpQoS1x;
  QoSJson                    mQoSJson;

protected:

  IPrivSet *                 mpPrivSet;
  NetworkModeType            mNetworkMode;

  ds::ErrorType GetGrantedFlowSpecInternal 
  (
    NetPlatform::FlowIoctlEnumType  ioctlKind,
    IQoSFlowPriv**               rxFlowObj,
    IQoSFlowPriv**               txFlowObj
  );
  
  ds::ErrorType GetGrantedFlowSpecInternal 
  (
    NetPlatform::FlowIoctlEnumType  ioctlKind,
    char* rxFlow, 
    int rxFlowLen, 
    int* rxFlowLenReq, 
    char* txFlow, 
    int txFlowLen, 
    int* txFlowLenReq  
  );

  ds::ErrorType GetQoS1x
  (
    IQoS1x* *pQoS1x
  );

  /*!
  @brief
  Protected destructor.

  @details
  The destructor of this object is private. Use Release() method of the
  IQI interface to free the object.

  @param      None.
  @return     None.
  @see        IQI::Release()
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
  virtual ~QoS
  (
    void
  )
  throw();

public:
  QoS
  (
    int32 ifaceHandle,
    int32 flowHandle,
    IPrivSet * pPrivSet,
    NetworkModeType networkMode
  );

  /*---------------------------------------------------------------------
    Inherited functions from IQoS.
  ---------------------------------------------------------------------*/
  virtual ds::ErrorType CDECL GetGrantedFlowSpec 
  (
    char* rxFlow, 
    int rxFlowLen, 
    int* rxFlowLenReq, 
    char* txFlow, 
    int txFlowLen, 
    int* txFlowLenReq
  ) = 0;

  virtual ds::ErrorType CDECL GetGrantedFlowSpecPriv 
  (
    IQoSFlowPriv** rxFlow, 
    IQoSFlowPriv** txFlow
  ) = 0;

  virtual ds::ErrorType CDECL GetTXPhysLink 
  (
    IPhysLink** txPhysLinkObj
  );

  virtual ds::ErrorType CDECL GetRXPhysLink 
  (
    IPhysLink** rxPhysLinkObj
  );

  virtual ds::ErrorType CDECL GetModifyResult 
  (
    QoSResultType* result
  );

  virtual ds::ErrorType CDECL GetUpdatedInfoCode 
  (
    QoSInfoCodeType* infoCode
  );

  virtual ds::ErrorType CDECL GetTXQueueLevel 
  (
    QoSTXQueueLevelType* TXQueueLevel
  );

  virtual ds::ErrorType CDECL GetTechObject 
  (
    AEEIID iid,
    void** ppo
  );


}; /* class QoS */
} /* Namespace Net */
} /* Namespace ds */

#endif /* DS_NET_QOS_H */
