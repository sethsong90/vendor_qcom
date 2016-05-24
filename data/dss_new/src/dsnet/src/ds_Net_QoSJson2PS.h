#ifndef DS_NET_QOS_JSON_2_PS_H
#define DS_NET_QOS_JSON_2_PS_H

/*===========================================================================
  @file ds_Net_QoSJSon2PS.h

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Net_Utils.h"
#include "AEEStdErr.h"
#include "ds_Net_MemManager.h"
#include "ds_Net_Platform.h"
#include "AEEIEnv.h"
#include "AEEenv.h"
#include "AEEIJSONTree.h"
#include "AEEJSONParser.h"
#include "AEEJSONGen.h"
#include "ds_Net_PSQoSSpecWrapper.h"

/*===========================================================================
                     FORWARD DECLERATION
===========================================================================*/


/*===========================================================================

                     PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace ds
{
namespace Net
{
/*lint -esym(1510, IQoSJSon2PS) */
/*lint -esym(1510, IQI) */
class QoSJSon2PS
{
public:

  /*! 
  @brief
  Constructor of QoSJSon2PS object. 
  */
  QoSJSon2PS();

  /*! 
  @brief
  Destructor of QoSJSon2PS object. 
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
  virtual ~QoSJSon2PS() throw();


  /*-------------------------------------------------------------------------
    Inherited functions from IQoSJSon2PS.
  -------------------------------------------------------------------------*/
  virtual ::AEEResult CDECL ConvertJSon2PSQoSSpec 
  (
    const char* sQoSSpec, 
    NetPlatform::PSQoSSpecType** pPSQoSFlowSpecArray,
    int* piSpecArrayLen
  );

  virtual ::AEEResult CDECL GenerateJSonErrFromPSSpec 
  (
    NetPlatform::PSQoSSpecType* pPSQoSFlowSpecArray,
    int piSpecArrayLen,
    char* sQoSSpecErr
  );
  
  virtual ::AEEResult CDECL GenerateJSonFromQoSFlow
  (
    NetPlatform::PSFlowSpecType* pPSQoSFlowSpec,
    char* sQoSFlow, 
    int nQoSFlowLen,
    int* nQoSFlowLenReq
  );

  /*-------------------------------------------------------------------------
    Overload new/delete operators.
  -------------------------------------------------------------------------*/
  DSNET_OVERLOAD_OPERATORS(PS_MEM_DS_NET_QOS_JSON_2_PS)

  /*-------------------------------------------------------------------------
  Macro to implement IWeakRef/IQI methods
  -------------------------------------------------------------------------*/
  DS_UTILS_IWEAKREF_IMPL_DEFAULTS_NO_QI();

  /*-------------------------------------------------------------------------
  Client provided privileges
  -------------------------------------------------------------------------*/

  /*-------------------------------------------------------------------------
  Constant definitions
  -------------------------------------------------------------------------*/

  typedef struct Str2CodeTable 
  {
    char *str;
    int code;
  } Str2CodeTable;

  enum
  {
    CONVERSATIONAL = 0,  
    STREAMING      = 1,  
    INTERACTIVE    = 2,  
    BACKGROUND     = 3   
  };

  enum
  {
    MIN_MAX      = 0,  
    TOKEN_BUCKET = 1,  
  };

  enum
  {
    BEST_EFFORT      = 0,  
    BACKGROUND_P     = 1,  
    RESERVED         = 2,  
    EXCELLENT_EFFORT = 3,  
    CONTROLLED_LOAD  = 4,  
    VIDEO            = 5,  
    VOICE            = 6,  
    NETWORK_CONTROL  = 7,  
  };



private:
  /*-------------------------------------------------------------------------
  Constant definitions
  -------------------------------------------------------------------------*/
  typedef enum
  {
    RX_FLOW,
    RX_Min_FLOW,
    TX_FLOW,
    TX_MIN_FLOW,
    RX_FILTER,
    TX_FILTER
  }JSonQoSObjectType;

  /*-------------------------------------------------------------------------
  Private methods
  -------------------------------------------------------------------------*/
  ::AEEResult ConvertJSonObj2PS 
  (
    JSValue* pJSonValueRoot,
    char* sObjectName,
    JSonQoSObjectType objType, 
    NetPlatform::PSQoSSpecType* pPSQoSSpec
  );

  AEEResult ConvertJSonFlow2PS 
  (
    JSValue* pJSonValueFlow,
    ip_flow_type* pFlow
  );

  ::AEEResult ConvertJSonFilter2PS 
  (
    JSValue* pJSonValueFilter,
    ip_filter_type* pFilter
  );

  ::AEEResult ConvertPSErrors2JSon
  (
    JSValue* pJSonValueErrorArray,
    uint32 errMask,
    int qoSSpecIndex,
    char* context,
    int index,
    int errType,
    JSonQoSObjectType objectType
  );

  ::AEEResult ConvertPSError2JSon
  (
    JSValue* pJSonValueErrorArray,
    int qoSSpecIndex,
    char* context,
    int index,
    char* flowOrFilter,
    char* errorOption,
    int errType
  );
  AEEResult Str2Code
  (
    const Str2CodeTable *list,
    const char *str,
    int *code
  );
  /*-------------------------------------------------------------------------
  Private members
  -------------------------------------------------------------------------*/
  IEnv* mpIEnv;
  IJSONTree* mpiTree;
  PSQoSSpecWrapper* mPSQoSSpecWrapper;

};/* class QoSJSon2PS */
} /* namespace Net */
} /* namespace ds */

#endif /* DS_NET_QOS_JSON_2_PS_H */

