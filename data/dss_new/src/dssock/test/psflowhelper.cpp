/*===========================================================================
  FILE: PSFlowHelper.cpp

   OVERVIEW: This file is a helper wrapping the ps_flow object to be used 
   for QTF testcases.

  DEPENDENCIES: None

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/test/
  PSFlowHelper.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- -------- ------------------------------------------------------------
  2008-05-14 gnamasiv Created module

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "PSFlowHelper.h"

using namespace PS::QTF;

/*===========================================================================

                      PUBLIC DECLARATIONS

===========================================================================*/

void PSFlowHelper::Activate
(
  void
)
{
  int16 psErrno = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/


  ps_flow_activate_cmd( flowPtr, &psErrno, NULL );
}

PSFlowHelper::PSFlowHelper
(
  void
)
{
}

PSFlowHelper * PSFlowHelper::CreateInstance
(
  ps_iface_type                   * iface,
  ps_phys_link_type               * physLink,
  ps_iface_ioctl_qos_request_type * ioctlInfo,
  ps_flow_cmd_f_ptr_type            activateCmdFuncPtr,
  ps_flow_cmd_f_ptr_type            configureCmdFuncPtr,
  ps_flow_cmd_f_ptr_type            suspendCmdFuncPtr,
  ps_flow_cmd_f_ptr_type            goNullCmdFuncPtr,
  ps_flow_ioctl_f_ptr_type          ioctlCmdFuncPtr 
)
{
  int            psErrno = 0;
  PSFlowHelper * flowHelper = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  flowHelper = new PSFlowHelper();
  
  psErrno = flowHelper->CreatePSFlowInCallBack
  (
      iface,
      physLink,
      ioctlInfo
  );

  if( psErrno != 0 )
  {
    MSG_ERROR( "Error creating PSFlow %d", psErrno, 0, 0 );
    return NULL;
  }

  flowHelper->SetCallBacks(
               activateCmdFuncPtr,
               configureCmdFuncPtr,
               suspendCmdFuncPtr,
               goNullCmdFuncPtr,
               ioctlCmdFuncPtr);


  return flowHelper;
}

PSFlowHelper * PSFlowHelper::CreateInstance
(
  ps_flow_type                    * flowPtr,
  ps_flow_cmd_f_ptr_type            activateCmdFuncPtr,
  ps_flow_cmd_f_ptr_type            configureCmdFuncPtr,
  ps_flow_cmd_f_ptr_type            suspendCmdFuncPtr,
  ps_flow_cmd_f_ptr_type            goNullCmdFuncPtr,
  ps_flow_ioctl_f_ptr_type          ioctlCmdFuncPtr 
)
{
  PSFlowHelper * flowHelper = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  flowHelper = new PSFlowHelper ( flowPtr );
  
    flowHelper->SetCallBacks(
               activateCmdFuncPtr,
               configureCmdFuncPtr,
               suspendCmdFuncPtr,
               goNullCmdFuncPtr,
               ioctlCmdFuncPtr );


  return flowHelper;
}
        
PSFlowHelper::PSFlowHelper 
(
  ps_flow_type  * pFlowPtr
)
{
  flowPtr = pFlowPtr;
}

void PSFlowHelper::PostFlowMessage
(
  PS::QTF::EventType eventType
)
{
  PosixTask::PostMessage( eventType, flowPtr );
}

int PSFlowHelper::CreatePSFlowInCallBack
(
  ps_iface_type                   * iface,
  ps_phys_link_type               * physLink,
  ps_iface_ioctl_qos_request_type * ioctlInfo
)
{
  ps_flow_create_param_type          reqQosParam;
  int                                retVal;
  int16                              psErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  reqQosParam.qos_spec            = ioctlInfo->qos_ptr;
  reqQosParam.flow_validate_f_ptr = NULL;
  reqQosParam.fltr_validate_f_ptr = NULL;
  reqQosParam.fltr_priority       = PS_IFACE_IPFLTR_PRIORITY_FCFS;
  reqQosParam.subset_id           = ioctlInfo->subset_id;
  reqQosParam.enable_filtering    = TRUE;

  retVal =  ps_iface_create_flow( iface,
                                  physLink,
                                  &reqQosParam,
                                  &flowPtr,
                                  &psErrno );

  if (retVal != 0)
  {
    MSG_ERROR("Error returned from ps_iface_create_flow %d", psErrno, 0, 0);
    flowPtr = NULL;
    return retVal;
  }

  ioctlInfo->flow_ptr = flowPtr;

  
  return 0;
}

void PSFlowHelper::SetCallBacks
(
  ps_flow_cmd_f_ptr_type   activateCmdFuncPtr,
  ps_flow_cmd_f_ptr_type   configureCmdFuncPtr,
  ps_flow_cmd_f_ptr_type   suspendCmdFuncPtr,
  ps_flow_cmd_f_ptr_type   goNullCmdFuncPtr,
  ps_flow_ioctl_f_ptr_type ioctlCmdFuncPtr 
)
{
  
  PS_FLOW_SET_CB_F_PTR
  (
    flowPtr,
    activateCmdFuncPtr == NULL ? PSActivateCmdCB 
    : activateCmdFuncPtr,
    configureCmdFuncPtr == NULL ? PSConfigureCmdCB
    : configureCmdFuncPtr,
    suspendCmdFuncPtr == NULL ? PSSuspendCmdCB
    : suspendCmdFuncPtr,
    goNullCmdFuncPtr == NULL ? PSGoNullCmdCB
    : goNullCmdFuncPtr,
    ioctlCmdFuncPtr == NULL ? PSIoctlCmdCB
    : ioctlCmdFuncPtr
  );
}

int PSFlowHelper::PSActivateCmdCBDoNothing
(
  ps_flow_type  * flowPtr,
  void          * clientDataPtr
)
{
  return 0;
}

int PSFlowHelper::PSActivateCmdCB
(
  ps_flow_type  * flowPtr,
  void          * clientDataPtr
)
{

  PosixTask::PostMessage
  (
    PS::QTF::Event::FLOW_ACTIVATE, 
    flowPtr
  );

  return 0;
}

int PSFlowHelper::PSConfigureCmdCB
(
  ps_flow_type  * flowPtr,
  void          * clientDataPtr
)
{
  return 0;
}

int PSFlowHelper::PSSuspendCmdCBDoNothing
(
  ps_flow_type  * flowPtr,
  void          * clientDataPtr
)
{
  return 0;
}

int PSFlowHelper::PSSuspendCmdCB
(
  ps_flow_type  * flowPtr,
  void          * clientDataPtr
)
{

  PosixTask::PostMessage
  (
    PS::QTF::Event::FLOW_SUSPEND, 
    flowPtr
  );
 
  return 0;
}

int PSFlowHelper::PSGoNullCmdCB
(
  ps_flow_type * flowPtr,
  void         * clientDataPtr
)
{
  
  return 0;
}

int PSFlowHelper::PSIoctlCmdCB
(
  ps_flow_type         * flowPtr,
  ps_flow_ioctl_type     ioctlName,
  void                 * argvalPtr,
  int16                * psErrno
)
{
  switch ( ioctlName )
  {
    case PS_FLOW_IOCTL_QOS_MODIFY:

      PosixTask::PostMessage
      (
        PS::QTF::Event::CHANGE_FLTR, 
        PS_FLOW_GET_IFACE(flowPtr)
      );
      break;

    case PS_FLOW_IOCTL_QOS_SUSPEND:

      PosixTask::PostMessage
      (
        PS::QTF::Event::FLOW_SUSPEND, 
        flowPtr
      );
      break;
  }

  return 0;
}

ps_flow_type* PSFlowHelper::GetPSFlowPtr
(
  void
)
{
  
  return flowPtr;
}
