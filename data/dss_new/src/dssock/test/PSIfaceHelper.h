/*===========================================================================
  @file PSIfaceHelper.h

  This file is a helper wrapping the Iface and utility functions
  to be used in for QTF testcases.

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/       
        test/PSIfaceHelper.h $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

#ifndef PS_UTILS_H
#define PS_UTILS_H
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "SocketFactory.h"
#include "qtf.h"
#include "qtf_tmc.h"
#include "qtf_msgr_msg.h"
#include "CommonUtils.h"

extern "C" {
#include "ps_acl.h"
#include "ps_iface.h"
#include "ps_aclrules.h"
#include "dss_netpolicy.h"
#include "ds_flow_control.h"
}
#include "DS_Net_Platform.h"
#include "DS_Net_CreateInstance.h"
#include "DS_Utils_SignalCBFactory.bid"
#include "DSNetPolicy.bid"
#include "AEEISignalCBFactory.h"
#include "PhysLinkHelper.h"
#include "PosixTask.h"
#include "PSFlowHelper.h"

#define MAX_IFACE_NUM 10
namespace PS
{
  namespace QTF
  {
    class PSIfaceHelper
    {
      public:

        /*===========================================================================

                      PUBLIC DATA DECLARATIONS

        ===========================================================================*/

        /**
           @brief Getter function for the local iface pointer 
           
           Getter function for the local iface pointer 
            
           @param : None 
            
           @retval ps_iface_type* : the pointer to the local iface 
                   pointer.
        */
        ps_iface_type* GetPSIfacePtr
        (
         void
        );

        /**
           @brief Initializer for PSIface
           
           Function to create iface, physLinks and initialize all call 
           back functions and bring up the physLinks
        
           @param[in] count           number of physLinks in the array 
           @param[in] aclFPtr         ACL function pointer 
           @param[in] aclPostProcFPtr ACL post process function pointer
           @param[in] ifaceName       name of the Iface 
           @retval int: 0 if no error
        */
        int InitIface 
        (
            int                         count,
            acl_fptr_type               aclFPtr = NULL,
            acl_post_process_fptr_type  aclPostProcFPtr = NULL,
            ps_iface_name_enum_type     ifaceName = STA_IFACE
        );

        /*-------------------------------------------------------------------------
          Default handlers for different Iface events
        -------------------------------------------------------------------------*/
        
        /**
           @brief Callback function for IfaceUpCmd
           
           Default IfaceUP callback function. Posts semaphore for
           ps_iface_up_ind. 
        
           @param[in] ifacePtr Pointer to the iface being brought up 
           @param[in] clientData Ignored

           @retval int always 0
        */
        static int PSIfaceBringUpCmdCB
        (
          ps_iface_type * ifacePtr,
          void          * clientData
        );

        /**
           @brief Callback function for IfaceDownCmd
           
           Default IfaceDown callback function. Posts semaphore for 
                  ps_iface_down_ind.
        
           @param[in] ifacePtr Pointer to the iface being brought down
           @param[in] clientData Ignored

           @retval int always 0
        */
        static int PSIfaceTearDownCmdCB
        (
          ps_iface_type * ifacePtr,
          void          * clientData
        );
        

        /**
           @brief Callback function for Ioctl cmd
          
           Default IOCTL callback function. Does nothing.
        
           @param[in] psIfacePtr Pointer to the iface being issued ioclt
           @param[in] ioctlName  Name of the Ioctl operation to perform 
           @param[in] argvalPtr  Contains the ioctl_info 
           @param[out] psErrno   returns errors if any

           @retval int always 0
        */
        static int PSIfaceIoctlHdlr
        (
          ps_iface_type        * psIfacePtr,
          ps_iface_ioctl_type    ioctlName,
          void                 * argvalPtr,
          int16                * psErrno
        );

        /**
           @brief Callback function for Ioctl cmd
          
           Default IOCTL callback function. Does nothing.
        
           @param[in] psFlowPtr  Pointer to the Flow being issued ioclt
           @param[in] ioctlName  Name of the Ioctl operation to perform 
           @param[in] argvalPtr  Contains the ioctl_info 
           @param[out] psErrno   returns errors if any

           @retval int always 0
        */
        static int PSIfaceFlowIoctlHdlr
        (
          ps_flow_type        * psFlowPtr,
          ps_flow_ioctl_type    ioctlName,
          void                * argvalPtr,
          int16               * psErrno
        );


        /**
           @brief Default Implementation of tx callback function. 
            
           Default Implementation of tx callback function. Does nothing. 
           Returns 0 
            
           @param[in] ifacePtr iface on which the tx event has occured
           @param[in] pktChainPtr pointer to dsm_item_s 
           @param[in] metaInfoPtr Routing and pkt meta info.
           @param[in] txCmdInfo additional info on tx_cmd
        
           @retval int 0 always returns 0
        */
        static int STAPSIfaceTxCmdCbFlowForwardingDefault
        (
          ps_iface_type        *  ifacePtr,
          dsm_item_type        ** pktChainPtr,
          ps_tx_meta_info_type *  metaInfoPtr,
          void                 *  txCmdInfo
        );

        /**
           @brief Default Implementation of tx callback function. 
            
           To check if flowforwarding is enabled.
            
           @param[in] ifacePtr iface on which the tx event has occured
           @param[in] pktChainPtr pointer to dsm_item_s 
           @param[in] metaInfoPtr Routing and pkt meta info.
           @param[in] txCmdInfo additional info on tx_cmd 
            
           @retval int 0 if flow forwarsing is enabled. 
                   int -1 if flow forwarding is disabled 
        */
        static int STAPSIfaceTxCmdCbFlowForwardingEnabled
        (
          ps_iface_type         *   ifacePtr,
          dsm_item_type         **  pktChainPtr,
          ps_tx_meta_info_type  *   metaInfoPtr,
          void                  *   txCmdInfo
        );

        /**
           @brief Default Implementation of tx callback function. 
           
           Implementation of tx callback function to determine if 
           flowForwarding  is disabled on the current Iface. 
            
           @param[in] ifacePtr iface on which the tx event has occured
           @param[in] pktChainPtr pointer to dsm_item_s 
           @param[in] metaInfoPtr Routing and pkt meta info.
           @param[in] txCmdInfo additional info on tx_cmd
        
           @retval int 0 if flowForwarding is disabled 
           @retval int -1 if flowForwarding is enabled  
        */
        static int STAPSIfaceTxCmdCbFlowForwardingDisabled
        (
          ps_iface_type         *   ifacePtr,
          dsm_item_type         **  pktChainPtr,
          ps_tx_meta_info_type  *   metaInfoPtr,
          void                  *   txCmdInfo
        );
        
        /**
           @brief Initializes the Iface
           
           Initializes the Iface object with callbacks and 
           initializes then physLink with it.
        
           @param[in] aclFPtr                 ACL function pointer 
           @param[in] aclPostProcFPtr         ACL post process function
                 pointer
           @param[in] pIfaceTxCBFnPtr         Tx callback function pointer. 
                 Default implementation used to determine if
                 flowforwarding is enabled.
           @param[in] pBringUpIfaceCmdFnPtr   Callback function for 
                 IfaceUP Cmd
           @param[in] pTearDownIfaceCmdFnPtr  Callback function pointer 
                 for IfaceDown Cmd
           @param[in] pIoctlIfaceFnPtr        Callback function

           @retval PSIfaceHelper *            Pointer to the initialized
           Iface 
        */
        static PSIfaceHelper * CreateIfaceInstance
        (
          int                             count = 2,
          acl_fptr_type                   aclFPtr = NULL,
          acl_post_process_fptr_type      aclPostProcFPtr = NULL,
          ps_iface_tx_cmd_f_ptr_type      ifaceTxCBFnPtr = NULL,
          ps_iface_cmd_f_ptr_type         bringUpIfaceCmdFnPtr = NULL,
          ps_iface_cmd_f_ptr_type         tearDownIfaceCmdFnPtr = NULL,
          ps_iface_ioctl_f_ptr_type       ioctlIfaceFnPtr = NULL
        );

        /**
           @brief Function to set the TX callback function 
            
           This is required as Tx also gets called as a 
           side-effect of either Initialization or test 
           case setup when no checking should be done. 
           However, when the test function is applied, 
           the function passed to Init should be applied. 
           Can also be used to test different forwarding 
           behavior in the same testcase. 
        
           @param[in] tx_cmd Function pointer to the callback 
            function to be called on tx. If NULL, the member
            variable pIfaceTxCBFnPtr is used
           @param[in] tx_cmd_info data to be passed to the 
           callback function 
            
           @retval None 
        */

        void setTxFunction
        (
          ps_iface_tx_cmd_f_ptr_type  txCmd = NULL,
          void                      * txCmdInfo = NULL
        );

        /**
           @brief Used to set the callbacks to the flow after the flow
           is created
            
           Used to set the callbacks to the flow after the flow
           is created. Just stores the creation state which could be 
           used for subsequent creation. 
            
           @param[in] physLinkHelper      physLinkHelper holding the 
                 physLink to be associated with the flow
           @param[in] activateCmdFuncPtr  Activate command callback
           @param[in] configureCmdFuncPtr Configure command callback
           @param[in] suspendCmdFuncPtr   Suspend command callback
           @param[in] goNullCmdFuncPtr    Going NULL command callback
           @param[in] ioctlCmdFuncPtr     IOCTL command callback
            
           @retVal -1 if error. 0 if no errors
         */
        int SetFlowCreationState
        (
          PhysLinkHelper                  * physLinkHelper,
          ps_flow_cmd_f_ptr_type            activateCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type            configureCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type            suspendCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type            goNullCmdFuncPtr = NULL,
          ps_flow_ioctl_f_ptr_type          ioctlCmdFuncPtr = NULL
        );
        
        /**
           @brief Get the physLinkHelper ptr corresponding to the 
           index
        
           @param[in] index index of the physLinkHelper to return
            
           @retval PhysLinkHelper* physLinkHelper at the specified 
           index
        */
        PhysLinkHelper * GetPhysLinkHelperPtr
        (
          int index = -1
        );

        /**
           @brief Get the flowHelper ptr corresponding to the index
        
           @param[in] index index of the flowHelper to return
            
           @retval PSFlowHelper * flowHelper at the specified 
           index
        */
        PSFlowHelper * GetPSFlowHelperPtr
        (
          int index = -1
        );
        
        /**
           @brief Get the flowHelper ptr corresponding to the index
        
           @param[in] PSFlowHelper * flowHelper at the specified 
           @param[in] index index of the flowHelper to return 
            
           @retVal None 
        */
        void SetPSFlowHelperPtr
        (
          PSFlowHelper *  psFlowHelper,
          int             index = -1
        );

        /**
           @brief Get the PSIfaceHelper at the specified position
            
           Get the PSIfaceHelper at the specified position. Call 
           with -1 or no index argument to obtain the last created 
           iface. 
            
           @param[in] index Index to look for PSIfaceHelper
            
           @retval PSIfaceHelper* PSIface helper if present. NULL 
           if index is invalid 
         */
        static PSIfaceHelper * GetPSIfaceHelper
        (
          int index = -1
        );
        
        /**
           @brief Returns the index of the passed in ps_iface pointer 
            
           Returns the index of the passed in ps_iface pointer  
        
           @param[in] psIface  pointer to the psIface type to look for
            
           @retval int index of the ps_iface object. -1 is ps_iface is 
                   not found.
        */
        static int LookUpPSIfaceHelper
        (
          ps_iface_type * psIface
        );

        /**
           @brief Creates a FlowHelper object and populates the 
           FlowHelperArr 
            
           Creates a FlowHelper object and populates the 
           FlowHelperArr 
            
           @param[in] qosRequestType qos request type for flow 
           creation 
            
           @retval int 0 ifno errors. -1 if error 
        */
        int PSIfaceHelper::CreateFlow
        (
          ps_iface_ioctl_qos_request_type  * qosRequestType
        );
        
        /**
           @brief Helper function for posting Iface related messages 
            
           Helper function for posting Iface related messages 
        
           @param[in] eventType The message type to be posted
            
           @retval None         
         */
        void PostIfaceMessage
        (
          PS::QTF::EventType eventType
        );
#if 0
        /**
           @brief Adds the paased in PSIfaceHelper the static 
           PSIfaceHelper array 
        
           @param[in] psIfaceHelper pointer to the PSIfaceHelper to be 
                 added
            
           @retval int 0 if not an error. -1 if error
        */
        static int AddToPSIfaceArray
        (
          PSIfaceHelper * psIfaceHelper
        );
#endif
      private:

        
        /*===========================================================================

                      PUBLIC DATA DECLARATIONS

        ===========================================================================*/
        /**
           @brief Constructor for PSIfaceHelper class. 

           Constructor for PSIfaceHelper class. Populates the 
                  interface type and call back function pointers. This
                  private construcotr is called from
                  PSIfaceHelper::CreateIfaceInstance
          
           @param[in] pIfaceTxCBFnPtr         Tx callback function pointer. 
                 Default implementation used to determine if
                 flowforwarding is enabled.
           @param[in] pBringUpIfaceCmdFnPtr   Callback function for 
                 IfaceUP Cmd
           @param[in] pTearDownIfaceCmdFnPtr  Callback function pointer 
                 for IfaceDown Cmd
           @param[in] pIoctlIfaceFnPtr        Callback function

           @see PSIfaceHelper::CreateIfaceInstance 
           @retval None 
        */
        PSIfaceHelper
        (
          ps_iface_tx_cmd_f_ptr_type      ifaceTxCBFnPtr = NULL,
          ps_iface_cmd_f_ptr_type         bringUpIfaceCmdFnPtr = NULL,
          ps_iface_cmd_f_ptr_type         tearDownIfaceCmdFnPtr = NULL,
          ps_iface_ioctl_f_ptr_type       ioctlIfaceFnPtr = NULL,
          ps_flow_ioctl_f_ptr_type        pPsFlowIoctlFnPtr = NULL
        );



        /**
           @brief The encapsulated iface 
            
           Encapsulated Iface type in PSIfaceHelper. The Iface is initialized 
           as a STA Iface 
        */
        ps_iface_type               qtfIface;
        
        /**
           @brief Acl Used for creation of Iface
        
           Acl Used for creation of Iface
        */
        acl_type                    acl;
        
        /**
           @brief Callback functions for Iface Tx event 
            
           Callback functions for Iface Tx event
       */
        ps_iface_tx_cmd_f_ptr_type  ifaceTxCBFnPtr;

        /**
           @brief Callback functions for Iface BringUp event 
            
           Callback functions for Iface BringUp event
       */
        ps_iface_cmd_f_ptr_type     bringUpIfaceCmdFnPtr;

        /**
           @brief Callback functions for Iface TearDown event 
            
           Callback functions for Iface TearDown event
       */
        ps_iface_cmd_f_ptr_type     tearDownIfaceCmdFnPtr;

        /**
           @brief Callback functions for Iface IOCTL event 
            
           Callback functions for Iface IOCTL event
       */
        ps_iface_ioctl_f_ptr_type   ioctlIfaceFnPtr;

        /**
           @brief Array of FlowHelper objects associated with 
           this Iface 
            
           Array of FlowHelper objects associated with 
           this Iface  
        */
        PSFlowHelper **             flowHelperArr;

        /**
           @brief Count of FlowHelper objects associated with 
           this Iface 
            
           Count of FlowHelper objects associated with this Iface 
        */
        int                         flowHelperCount;

        /**
           @brief Array of PhysLinkHelper objects associated with 
           this Iface 
            
           Array of PhysLinkHelper objects associated with this Iface 
        */ 
        PhysLinkHelper           ** physLinkHelperArr;
        
        /**
           @brief Count of PhysLinkHelper objects associated with 
           this Iface 
            
           Count of PhysLinkHelper objects associated with this Iface 
        */ 
        int                         physLinkHelperCount;
        
        /**
           @brief Static array of ifacehelper objects
        
           Static array of ifacehelper objects
        */
        static PSIfaceHelper      * ifaceHelperArr [ MAX_IFACE_NUM ];
        
        /**
           @brief Count of the number of IfaceHelper objects
        
           Count of the number of IfaceHelper objects 
        */
        static int                  ifaceHelperCount;

        /*--------------------------------------------------------------
        Set of command callback function pointers to be set as callbacks 
        for the flow to be created. 
        --------------------------------------------------------------*/

        /**
           @brief Flow activate command callback function pointer 
            
           Flow activate command callback function pointer     
        */
        ps_flow_cmd_f_ptr_type      flowActivateCmdFuncPtr;

        /**
           @brief Flow configure command callback function pointer 
            
           Flow configure command callback function pointer     
        */
        ps_flow_cmd_f_ptr_type      flowConfigureCmdFuncPtr;

        /**
           @brief Flow suspend command callback function pointer 
            
           Flow suspend command callback function pointer     
        */
        ps_flow_cmd_f_ptr_type      flowSuspendCmdFuncPtr;

        /**
           @brief Flow going NULL command callback function pointer 
            
           Flow going NULL command callback function pointer     
        */
        ps_flow_cmd_f_ptr_type      flowGoNullCmdFuncPtr;

        /**
           @brief Flow IOCTL command callback function pointer 
            
           Flow IOCLTL command callback function pointer     
        */
        ps_flow_ioctl_f_ptr_type    flowIoctlCmdFuncPtr; 

        /**
           @brief Index of the physLinkHelper to be used with 
           flow creation 
        */
        int                         physLinkIndexForFlowCreat;

    }; /* PSIfaceHelper */


  } /* QTF */
} /* PS */
#endif /* PS_UTILS_H */
