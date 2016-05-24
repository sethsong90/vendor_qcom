#ifndef FLOWHELPER_H
#define FLOWHELPER_H
/*===========================================================================
  @file PSFlowHelper.h

  This file is a helper wrapping the ps_flow object to be used in
  for QTF testcases.

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/test/       
        PSFlowHelper.h#1 $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
extern "C" {
#include "ps_iface_flow.h"
#include "ps_flow.h"
}

#include "PosixTask.h"
#include "DSNetQosTypes.h"
#include "IDSNetQoSManager.h"
#include "IDSNetIPFilter.h"
#include "IDSNetQoSSecondary.h"
#include "ps_ipfltr_defs.h"
#include "DSNetIPFilterSpec.bid"
#include "DSNetQoSFlowSpec.bid"
#include "DS_Net_CreateInstance.h"

namespace PS
{
  namespace QTF
  {

    class PSFlowHelper
    {
      public:
        /*===========================================================================

                      PUBLIC DATA DECLARATIONS

        ===========================================================================*/

        /**
           @brief Getter method for the flow ponter 
            
           Getter method for the flow ponter  
            
           @param None 
            
           @retval ps_flow_type* return the current flowPtr
        */
        ps_flow_type* GetPSFlowPtr
        (
          void
        );
        
        /**
          @brief Function to assign callback functions 
           
          Assign the supplied callback functions to this Flow's 
          callback 
        
          @param[in] activateCmdFuncPtr Flow activate callback 
          function. 
          @param[in] configureCmdFuncPtr Flow Configure callback 
          function. 
          @param[in] suspendCmdFuncPtr Flow Suspend callback function
          @param[in] goNullCmdFuncPtr Flow Null callback function
          @param[in] ioctlCmdFuncPtr Flow IOCTL callback function 
           
          @retval : None 
        */
        void PSFlowHelper::SetCallBacks
        (
          ps_flow_cmd_f_ptr_type    activateCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type    configureCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type    suspendCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type    goNullCmdFuncPtr = NULL,
          ps_flow_ioctl_f_ptr_type  ioctlCmdFuncPtr = NULL 
        );

        /**
           @brief Static callback function for Flow Activate command 
           posting activate indication.    
            
           Static callback function for Flow Activate command. Posts the 
           semaphore to PosixTask to call Activate indication. This is 
           the default command callback for activate. This has the 
           effect of activating all sec flows created automatically. 
        
           @param[in] flowPtr       Pointer to the flow that was activated 
           @param[in] clientDataPtr Ignored in this case
        
           @retval int 0 : if no errors 
        */
        static int PSActivateCmdCB
        (
          ps_flow_type  * flowPtr,
          void          * clientDataPtr
        );
        
        /**
           @brief Static callback function for Flow Activate command 
            
           Static callback function for Flow Activate command. Dummy 
           callback to prevent newly created secondary flows from 
           becoming activated 
        
           @param[in] flowPtr       Pointer to the flow that was activated 
           @param[in] clientDataPtr Ignored in this case
        
           @retval int 0 : if no errors 
        */
        static int PSActivateCmdCBDoNothing
        (
          ps_flow_type  * flowPtr,
          void          * clientDataPtr
        );

        /**
           @brief Static callback function from Flow Configure command 
            
           Static callback function from Flow Configure command 
        
           @param[in] flowPtr       Pointer to the flow that was configured
           @param[in] clientDataPtr Ignored in this case
        
           @retval int 0 : if no errors 
        */
        static int PSConfigureCmdCB
        (
          ps_flow_type  * flowPtr,
          void          * clientDataPtr
        );

        /**
           @brief Static callback function from Flow Suspend command. 
                  Calls the suspend inddication.
            
           Static callback function from Flow Suspend command. Default 
           callback for Suspend command. Posts the semaphore to call the 
           Suspend indication. 
        
           @param[in] flowPtr       Pointer to the flow that was 
                 suspended
           @param[in] clientDataPtr Ignored in this case
        
           @retval int 0 if no errors 
        */
        static int PSSuspendCmdCB
        (
          ps_flow_type  * flowPtr,
          void          * clientDataPtr
        );

        /**
           @brief Static callback function from Flow Suspend command 
           Does Nothing.
            
           Static callback function from Flow Suspend command. Does 
           Nothing. 

           @param[in] flowPtr       Pointer to the flow that was 
                 suspended
           @param[in] clientDataPtr Ignored in this case

           @retval int 0 if no errors 
        */
        static int PSSuspendCmdCBDoNothing
        (
          ps_flow_type  * flowPtr,
          void          * clientDataPtr
        );

        
        /**
           @brief Static callback function from Flow Null command 
            
           Static callback function from Flow Null command 
        
           @param[in] flowPtr       Pointer to the flow that went NULL
           @param[in] clientDataPtr Ignored in this case
        
           @retval int 0 if no errors 
        */
        static int PSGoNullCmdCB
        (
          ps_flow_type * flowPtr,
          void         * clientDataPtr
        );
        
        /**
           @brief Static callback function from Flow Ioctl command 
            
           Static callback function from Flow Ioctl command 
        
           @param[in] flowPtr   Pointer to the flow on which Ioctl 
           operation is being applied 
           @param[in] ioctlName Name of the IOCTL command
           @param[in] argvalPtr Argument. Ignored in this case
           @param[in] psErrno   error value if any
        
           @retval int 0 if no error
        */
        static int PSIoctlCmdCB
        (
          ps_flow_type         * flowPtr,
          ps_flow_ioctl_type     ioctlName,
          void                 * argvalPtr,
          int16                * psErrno
        );
          
        /**
          @brief Static function to create PSFlowHelper Object. 
           
          Static function to create PSFlowHelper Object. Creates 
          the object, creates ps_flow and sets the callback functions 
        
          @param[in] iface               PS Iface to associate with flow
          @param[in] physLink            PhysLink to 
          associate with flow 
          @param[in] ioctlInfo           contains the qos spec and ip 
          ftlr subset Id 
          @param[in] activateCmdFuncPtr  Flow activate callback 
          function. 
          @param[in] configureCmdFuncPtr Flow Configure callback 
          function. 
          @param[in] suspendCmdFuncPtr   Flow Suspend callback function
          @param[in] goNullCmdFuncPtr    Flow Null callback function
          @param[in] ioctlCmdFuncPtr     Flow IOCTL callback function 
        
          @retval PSFlowHelper* 
        */
        static PSFlowHelper * PSFlowHelper::CreateInstance
        (
          ps_iface_type                   * iface,
          ps_phys_link_type               * physLink,
          ps_iface_ioctl_qos_request_type * ioctlInfo,
          ps_flow_cmd_f_ptr_type            activateCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type            configureCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type            suspendCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type            goNullCmdFuncPtr = NULL,
          ps_flow_ioctl_f_ptr_type          ioctlCmdFuncPtr = NULL 
        );

        /**
           @brief Function that calls the ps_iface_create_flow
            
           This function gets called from iface_ioctl_callback 
           in response to RequestSecFlow called from the 
           SocketTsetUtils. This function creates the flow and 
           associates the passde in iface and physLink 
            
           @param[in] iface    PS Iface to associate with flow 
           @param[in] physLink contains the physLink to 
           associate with flow 
           @param[in] ioctlInfo contains the qos spec and ip ftlr 
            subset Id to create the flow
            
           @see CreateFlow
            
           @retval int Non-zero if no errors
        */

        int CreatePSFlowInCallBack
        (
            ps_iface_type                   * iface,
            ps_phys_link_type               * physLink,
            ps_iface_ioctl_qos_request_type * ioctlInfo
        );
        
        /**
           @brief Helper function to call activate on flow
        
           Helper function to call activate on flow 
            
           @param None 
            
           @retVal None 
        */
        void Activate
        (
          void
        );

       /** 
        @brief Method for constructing PSFlowHelper if flow pointer 
        is available. 
         
        Method for constructing PSFlowHelper if flow pointer 
        is available. 
          
        @param[in] flowPtr             Flow Pointer to be 
          associated with PSFlowHelper
        @param[in] activateCmdFuncPtr  Flow activate callback 
        function. 
        @param[in] configureCmdFuncPtr Flow Configure callback 
        function. 
        @param[in] suspendCmdFuncPtr   Flow Suspend callback function
        @param[in] goNullCmdFuncPtr    Flow Null callback function
        @param[in] ioctlCmdFuncPtr     Flow IOCTL callback function 

        @retval PSFlowHelper* 
        */
        PSFlowHelper * PSFlowHelper::CreateInstance
        (
          ps_flow_type                    * flowPtr,
          ps_flow_cmd_f_ptr_type            activateCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type            configureCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type            suspendCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type            goNullCmdFuncPtr = NULL,
          ps_flow_ioctl_f_ptr_type          ioctlCmdFuncPtr = NULL
        );

        /**
          @brief Private constructor for PSFlowHelper when PSLow is 
          available. 

          Private constructor for PSFlowHelper. The object is 
          constructed by calling the static CreateInstance method. 

          @param[in] None 

          @see PSFlowHelper::CreateInstance

        */
        PSFlowHelper
        (
          ps_flow_type           * pFlowPtr
        );
        
        /**
           @brief Helper function for posting flow related messages 
            
           Helper function for posting flow related messages 
        
           @param[in] eventType The message type to be posted
            
           @retval None 
        */
        void PostFlowMessage
        (
          PS::QTF::EventType eventType
        );

        /*===========================================================================

                      PRIVATE DATA DECLARATIONS

        ===========================================================================*/        
      private:

        /**
          @brief Private constructor for PSFlowHelper 
            
          Private constructor for PSFlowHelper. The object is 
          constructed by calling the static CreateInstance method. 
        
          @param[in] None 
            
          @retVal PSFlowHelper::CreateInstance
        
        */
        PSFlowHelper
        (
          void
        );
        

        /**
           @brief The encapsulated flow pointer used by the 
           helper 
            
        */
        ps_flow_type            * flowPtr;

    }; /* class */
  } /* QTF */
}/* PS */
#endif /* FLOWHELPER_H */             
