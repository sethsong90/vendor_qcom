/*===========================================================================
  FILE: PSIfaceHelper.cpp

  OVERVIEW: This file is a helper wrapping the Iface and utility functions
  to be used in for QTF testcases.

  DEPENDENCIES: None

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/
  test/PSIfaceHelper.h $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "PSIfaceHelper.h"

/*-------------------------------------------------------------------------
  Macro for creating a simple ACL to be used for Iface creation
-------------------------------------------------------------------------*/
ACL_DEF( sta_acl )
ACL_START
PASS( ACL_IFNAME_CLASS, REQUIRED_IFACE_TYPE_IS( STA_IFACE ) );
PASS( ACL_DEFAULT_CLASS, ALL );  
ACL_END

using namespace PS::QTF;


PSIfaceHelper     *  PSIfaceHelper::ifaceHelperArr[];
int                  PSIfaceHelper::ifaceHelperCount;

/*===========================================================================

                     PUBLIC MEMBER FUNCTIONS

===========================================================================*/
PSIfaceHelper::PSIfaceHelper
(
  ps_iface_tx_cmd_f_ptr_type  pIfaceTxCBFnPtr,
  ps_iface_cmd_f_ptr_type     pBringUpIfaceCmdFnPtr,
  ps_iface_cmd_f_ptr_type     pTearDownIfaceCmdFnPtr,
  ps_iface_ioctl_f_ptr_type   pIoctlIfaceFnPtr,
  ps_flow_ioctl_f_ptr_type    pPsFlowIoctlFnPtr
)
{

  ifaceTxCBFnPtr = ( pIfaceTxCBFnPtr == NULL ) ? 
                   STAPSIfaceTxCmdCbFlowForwardingDefault : pIfaceTxCBFnPtr;
  bringUpIfaceCmdFnPtr = (pBringUpIfaceCmdFnPtr == NULL) ? 
                         PSIfaceBringUpCmdCB : pBringUpIfaceCmdFnPtr;
  tearDownIfaceCmdFnPtr = (pTearDownIfaceCmdFnPtr == NULL) ? 
                          PSIfaceTearDownCmdCB : pTearDownIfaceCmdFnPtr;
  ioctlIfaceFnPtr = (pIoctlIfaceFnPtr == NULL) ? 
                    PSIfaceIoctlHdlr : pIoctlIfaceFnPtr;
}
   
int PSIfaceHelper::PSIfaceBringUpCmdCB
(
  ps_iface_type * ifacePtr,
  void          * clientData
)
{ 
  PSIfaceHelper * psIfaceHelper;
  int             ifaceIndex;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*--------------------------------------------------------------
  1. Bring up the default physLink 
  2. Activate the default flow 
  3. Bring up the iface 
  --------------------------------------------------------------*/
  ifaceIndex = PSIfaceHelper::LookUpPSIfaceHelper( ifacePtr );
  psIfaceHelper = PSIfaceHelper::GetPSIfaceHelper( ifaceIndex );

  PosixTask::PostMessage( 
    PS::QTF::Event::PHYS_LINK_UP, 
    psIfaceHelper->GetPhysLinkHelperPtr( 0 )->GetPhysLinkPtr() 
    );

  PosixTask::PostMessage( 
    PS::QTF::Event::FLOW_ACTIVATE, 
    psIfaceHelper->GetPSFlowHelperPtr( 0 )->GetPSFlowPtr() );

  PosixTask::PostMessage( PS::QTF::Event::IFACE_UP, ifacePtr );

  return 0;
}

int PSIfaceHelper::PSIfaceTearDownCmdCB
(
  ps_iface_type * ifacePtr,
  void          * clientData
)
{
  PosixTask::PostMessage( PS::QTF::Event::IFACE_DOWN, ifacePtr );
  return 0;
}

int PSIfaceHelper::PSIfaceIoctlHdlr
(
  ps_iface_type        * psIfacePtr,
  ps_iface_ioctl_type    ioctlName,
  void                 * argvalPtr,
  int16                * psErrno
)
{
  int ifaceIndex;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  switch ( ioctlName )
  {

  case PS_IFACE_IOCTL_QOS_REQUEST:

    if( ( ifaceIndex = LookUpPSIfaceHelper( psIfacePtr )) == -1 )
    {
      MSG_ERROR( "Invalid psIfacePtr ", 0, 0, 0);
      return -1;
    }
    GetPSIfaceHelper( ifaceIndex )->CreateFlow( 
      ( ps_iface_ioctl_qos_request_type  * ) argvalPtr );
/*
      PSFlowHelper::CreateInstance(
                    ifaceHelper,
                    physLinkHelperArr [ physLinkIndex ],
                    ( ps_iface_ioctl_qos_request_type  * ) argval ,
                    activateCmdFuncPtr,
                    configureCmdFuncPtr,
                    suspendCmdFuncPtr,
                    goNullCmdFuncPtr,
                    ioctlCmdFuncPtr ) 
                    );
*/
      break;

    default:
      MSG_ERROR("Operation not supported", 0, 0, 0);

      break;
  }

  return 0;
}

int PSIfaceHelper::CreateFlow
(
  ps_iface_ioctl_qos_request_type  * qosRequestType
)
{
  PSFlowHelper * psFlowHelper = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  psFlowHelper = 
    PSFlowHelper::CreateInstance(
    GetPSIfacePtr(), 
    GetPhysLinkHelperPtr( physLinkIndexForFlowCreat )->GetPhysLinkPtr(),
    qosRequestType,
    flowActivateCmdFuncPtr,
    flowConfigureCmdFuncPtr,
    flowSuspendCmdFuncPtr,
    flowGoNullCmdFuncPtr,
    flowIoctlCmdFuncPtr
    );
  
  if ( psFlowHelper )
  {
    SetPSFlowHelperPtr(
      psFlowHelper
    );

    return 0;
  }
  else
  {
    MSG_ERROR( "Error creating flow ", 0, 0, 0 );
    return -1;
  }
}

PSIfaceHelper * PSIfaceHelper::GetPSIfaceHelper
(
  int index
)
{
  if ( index < -1 || index >= ifaceHelperCount )
  {
    MSG_ERROR( "PSIface of invalid index %d requested", 
               index, 0, 0);
    return 0;
  }
  else if ( index == -1 )
  {
    return ifaceHelperArr [ ifaceHelperCount - 1 ];
  }
  else
  {
    return ifaceHelperArr [ index ];
  }
}

int PSIfaceHelper::LookUpPSIfaceHelper
(
  ps_iface_type * psIface
)
{
  int index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  for ( index = 0; index < ifaceHelperCount; index ++ )
  {
    if ( ifaceHelperArr [ index ]->GetPSIfacePtr() == psIface )
    {
      return  index;
    }
  }

  return -1;
}


/* 
int PSIfaceHelper::PSIfaceFlowIoctlHdlr
(
  ps_flow_type        * psFlowPtr,
  ps_flow_ioctl_type    ioctlName,
  void                * argvalPtr,
  int16               * psErrno
)
{
  return 0;
}
*/

int PSIfaceHelper::STAPSIfaceTxCmdCbFlowForwardingDefault
(
  ps_iface_type         * ifacePtr,
  dsm_item_type        ** pktChainPtr,
  ps_tx_meta_info_type  * metaInfoPtr,
  void                  * txCmdInfo
)
{
  return 0;
}


void PSIfaceHelper::setTxFunction
(
  ps_iface_tx_cmd_f_ptr_type  txCmd,
  void                      * txCmdInfo
)
{
  if( txCmd == NULL )
  {
    txCmd = ifaceTxCBFnPtr;
  }
  ps_iface_set_tx_function(&qtfIface,
                           txCmd,
                           txCmdInfo);
}

int PSIfaceHelper::InitIface
(
  int                         count,
  acl_fptr_type               aclFPtr,
  acl_post_process_fptr_type  aclPostProcFPtr,
  ps_iface_name_enum_type     ifaceName
)
{
  int                       ps_errno = 0;
  int                       index = 0;
  ps_phys_link_type       * physLinkArr;

  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  acl.acl_fptr       = ( aclFPtr == NULL ) ? 
                       sta_acl : aclFPtr;
  acl.post_proc_fptr = aclPostProcFPtr;

  /*--------------------------------------------------------------
  Create PhysLinkHelperArray.
  --------------------------------------------------------------*/
  physLinkHelperCount = count;
  physLinkArr = new ps_phys_link_type [ count ];
  physLinkHelperArr = new PhysLinkHelper * [ count ];

  for ( index = 0 ; index < count; index++ )
  {
    physLinkHelperArr [ index ] = new PhysLinkHelper
      (
      &(physLinkArr [ index ])
      );
  }

  ps_errno = ps_iface_create(&qtfIface,
                             ifaceName,
                             &acl,
                             NULL,
                             physLinkArr,
                             count); 
  if (ps_errno < 0)
  {
    MSG_HIGH("Unable to create iface. Error :%d", ps_errno,0,0); 
    return -1;
  }

  qtfIface.bring_up_cmd_f_ptr   = bringUpIfaceCmdFnPtr; 
  qtfIface.tear_down_cmd_f_ptr  = tearDownIfaceCmdFnPtr; 
  qtfIface.iface_ioctl_f_ptr    = ioctlIfaceFnPtr;

  /*--------------------------------------------------------------
  Initialize to the default TX function. Set the correct Tx
  callback function after initialization to prevent errors during
  initialization
  --------------------------------------------------------------*/

  ps_iface_set_tx_function(&qtfIface,
                           STAPSIfaceTxCmdCbFlowForwardingDefault,
                           NULL);

  for ( index = 0; index < count; index++ )
  {
    PS_PHYS_LINK_SET_CAPABILITY( &( physLinkArr[ index ] ),
                                PS_PHYS_LINK_CAPABILITY_FLOW_DECOUPLED);
    //ps_phys_link_up_ind( &( physLinkArr[ idx ] ) );
  }

  flowHelperArr = new PSFlowHelper * [ 1 ];
  flowHelperCount = 1;
  flowHelperArr [ 0 ] = new PSFlowHelper( PS_IFACE_GET_DEFAULT_FLOW(&qtfIface) );
/*
  PS_FLOW_SET_CB_F_PTR
  (
    PS_IFACE_GET_DEFAULT_FLOW(&qtfIface),
    NULL,
    NULL,
    NULL,
    NULL,
    psFlowIoctlFnPtr
  );
*/

  ps_iface_enable_ind(&qtfIface);
  return 0;

}        

void PSIfaceHelper::PostIfaceMessage
(
  PS::QTF::EventType eventType
)
{
  PosixTask::PostMessage( eventType, &qtfIface );
}

int PSIfaceHelper::SetFlowCreationState
(
  PhysLinkHelper                  * physLinkHelper,
  ps_flow_cmd_f_ptr_type            activateCmdFuncPtr,
  ps_flow_cmd_f_ptr_type            configureCmdFuncPtr,
  ps_flow_cmd_f_ptr_type            suspendCmdFuncPtr,
  ps_flow_cmd_f_ptr_type            goNullCmdFuncPtr,
  ps_flow_ioctl_f_ptr_type          ioctlCmdFuncPtr
)
{
  int index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  physLinkIndexForFlowCreat = -1;

  flowActivateCmdFuncPtr = activateCmdFuncPtr;
  flowConfigureCmdFuncPtr = configureCmdFuncPtr;
  flowSuspendCmdFuncPtr = suspendCmdFuncPtr;
  flowGoNullCmdFuncPtr = goNullCmdFuncPtr;
  flowIoctlCmdFuncPtr = ioctlCmdFuncPtr;
  
  for ( index = 0; index < physLinkHelperCount; index ++ )
  {
    if ( physLinkHelperArr [ index ] == physLinkHelper )
    {
      physLinkIndexForFlowCreat = index;
      break;
    }
  }

  if ( physLinkIndexForFlowCreat == -1 )
  {
    MSG_ERROR( "PhysLink passed for flow creation does "
               "not belong to the iface", 0, 0, 0);
    return -1;
  }

  return 0;
}

PSIfaceHelper * PSIfaceHelper::CreateIfaceInstance
(
  int                           count,
  acl_fptr_type                 aclFPtr,
  acl_post_process_fptr_type    aclPostProcFPtr,
  ps_iface_tx_cmd_f_ptr_type    pIfaceTxCBFnPtr,
  ps_iface_cmd_f_ptr_type       pBringUpIfaceCmdFnPtr,
  ps_iface_cmd_f_ptr_type       pTearDownIfaceCmdFnPtr,
  ps_iface_ioctl_f_ptr_type     pIoctlIfaceFnPtr
)
{

  PSIfaceHelper * psIfaceHelper = new PSIfaceHelper(
    pIfaceTxCBFnPtr,
    pBringUpIfaceCmdFnPtr,
    pTearDownIfaceCmdFnPtr,
    pIoctlIfaceFnPtr
    );

  if( psIfaceHelper -> InitIface
      ( 
      count,
      aclFPtr,
      aclPostProcFPtr
      ) != 0 
    )
  {

    delete( psIfaceHelper );
    return NULL;
  }
  
  ifaceHelperArr [ ifaceHelperCount++ ] = psIfaceHelper;
  return psIfaceHelper;
}

int PSIfaceHelper::STAPSIfaceTxCmdCbFlowForwardingEnabled
(
  ps_iface_type         * ifacePtr,
  dsm_item_type        ** pktChainPtr,
  ps_tx_meta_info_type  * metaInfoPtr,
  void                  * txCmdInfo
)
{
  int32  defaultPSFlowHandle;
  int32  currFltrResult;
  ps_rt_meta_info_type * rtMetaInfoPtr;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  
  if (! metaInfoPtr )
  {
    return -1;
  }

  rtMetaInfoPtr = metaInfoPtr->rt_meta_info_ptr;
  if (!rtMetaInfoPtr)
  {
    return -1;
  }

  defaultPSFlowHandle =
  NetPlatform::GetDefaultPSFlowFromRtMetaInfo( rtMetaInfoPtr );
  currFltrResult = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr );

  if (defaultPSFlowHandle == currFltrResult)
  {
    return 0;
  }

  else
  {
    return -1;
  }
}

int PSIfaceHelper::STAPSIfaceTxCmdCbFlowForwardingDisabled
(
  ps_iface_type         * ifacePtr,
  dsm_item_type        ** pktChainPtr,
  ps_tx_meta_info_type  * metaInfoPtr,
  void                  * txCmdInfo
)
{
  int32                  defaultPSFlowHandle;
  int32                  currFltrResult;
  ps_rt_meta_info_type * rtMetaInfoPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  if ( ! metaInfoPtr )
  {
    return -1;
  }

  rtMetaInfoPtr = metaInfoPtr->rt_meta_info_ptr;
  
  if ( ! rtMetaInfoPtr )
  {
    return -1;
  }
  
  defaultPSFlowHandle =
  NetPlatform::GetDefaultPSFlowFromRtMetaInfo( rtMetaInfoPtr );
  currFltrResult = NetPlatform::GetPSFlowFromRtMetaInfo( rtMetaInfoPtr );
  
  if ( defaultPSFlowHandle == currFltrResult )
  {
    return -1;
  }
  else
  {
    return 0;
  }

}

ps_iface_type * PSIfaceHelper::GetPSIfacePtr
(
  void
)
{
  return &qtfIface;
}

PhysLinkHelper * PSIfaceHelper::GetPhysLinkHelperPtr
(
  int index
)
{
  if ( index < -1 || index >= physLinkHelperCount )
  {
    MSG_ERROR( " PhysLink of invalid index %d requested", 
               index, 0, 0);
    return 0;
  }
  else if ( index == -1 )
  {
    return physLinkHelperArr[ physLinkHelperCount - 1 ];
  }
  else
  {
    return physLinkHelperArr [ index ];
  }
}

void PSIfaceHelper::SetPSFlowHelperPtr
(
  PSFlowHelper *  psFlowHelper,
  int             flowIndex
)
{
  PSFlowHelper ** tempFlowHelperArr;
  int             index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  if ( flowIndex < -1 || flowIndex >= flowHelperCount )
  {
    MSG_ERROR( " Flow of invalid index %d requested", 
               flowIndex, 0, 0);
  }
  else if ( flowIndex == -1 )
  {
    tempFlowHelperArr = new PSFlowHelper * [ flowHelperCount + 1 ];

    for ( index = 0; index < flowHelperCount; index ++ )
    {
      tempFlowHelperArr [ index ] = flowHelperArr [ index ];
    }
    tempFlowHelperArr [ index ] = psFlowHelper;
    flowHelperCount ++;

    delete [] ( flowHelperArr );

    flowHelperArr = tempFlowHelperArr;
  }
  else
  {
    delete ( flowHelperArr [ flowIndex ] );
    flowHelperArr [ flowIndex ] = psFlowHelper;
  }
}

PSFlowHelper * PSIfaceHelper::GetPSFlowHelperPtr
(
  int index
)
{
  if (index < -1 || index >= flowHelperCount )
  {
    MSG_ERROR( " Flow of invalid index %d requested", 
               index, 0, 0);
    return 0;
  }
  else if ( index == -1 )
  {
    return flowHelperArr [ flowHelperCount - 1 ];
  }
  else
  {
    return flowHelperArr [ index ];
  }
}

