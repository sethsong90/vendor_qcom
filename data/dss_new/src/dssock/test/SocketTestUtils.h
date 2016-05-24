#ifndef SOCKETTESTUTILS_H
#define SOCKETTESTUTILS_H

/*===========================================================================
  @file SocketTestUtils.h

  This file is a helper wrapping the Socket object and providing
   functions for creating, destroying and writing to the socket.
       
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/       
        test/SocketTestUtils.h $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "PSIfaceHelper.h"
#include "SocketFactory.h"

#include "CommonUtils.h"

#include "qtf.h"
#include "qtf_tmc.h"
#include "qtf_msgr_msg.h"

extern "C" {
#include "ps_acl.h"
#include "ps_iface.h"
#include "ps_aclrules.h"
#include "dss_netpolicy.h"
#include "ds_flow_control.h"
#include "semaphore.h"
#include "pthread.h"
}

#include "DS_Net_Platform.h"
#include "DS_Net_CreateInstance.h"
#include "DS_Utils_SignalCBFactory.bid"
#include "DSNetPolicy.bid"
#include "AEEISignalCBFactory.h"
#include "DSNetNetworkFactory.bid"
#include "DSSockSocketFactory.bid"
#include "IDSNetworkFactory.h"
#include "IDSNetwork.h"

#if 0
#define PHYSLINK_INDEX  1
#define NUM_PHYS_LINKS  2
#define NUM_FLOW_HELPER 1
#endif

#define NUM_FLOW_HELPER 10
namespace PS
{
  namespace QTF
  {
    class SocketTestUtils
    {
      public:
        /*===================================================================

                      PUBLIC DATA DECLARATIONS

        ===================================================================*/

#if 0
        /**
           @brief Getter function for PhysLinkHelper and therefore 
                  PhysLinks

           Getter function for PhysLinkHelper and therefore 
                  PhysLinks for the passed in index

           @param index Index of the PhysLinkHelper instance to return 
        
           @retval PhysLinkHelper *  returns the current physLinkHelper 
                   pointer for the passed index
        */
        PhysLinkHelper * GetPhysLinkHelper
        (
            int index
        );
#endif
        /**
          @brief Getter function for PSIfaceHelper and therefore Iface 

          Getter function for PSIfaceHelper and therefore Iface

          @param None 
        
          @retval PSIfaceHelper * Returns the current PSIfaceHelper 
                   pointer
        */
        PSIfaceHelper * GetPSIfaceHelper
        (
          void
        );
#if 0
        /**
           @brief Getter function for PSFlowHelper and therefore flow 

           Getter function for PSFlowHelper and therefore flow. PS Flows
           are created in IfaceIoctlCB function. Calling GetFlowHelper 
           without any index results in the most recently created flow 
           being returned. 

           @param[in] index Index of the flowHelper to retreive 
            
           @see PSFlowHelper::PSIfaceIoctlCallBack 
        
           @retval PSFlowHelper * returns the current PSIfaceHelper 
                   pointer
        */
        static PSFlowHelper * GetFlowHelper
        (
          int index = -1
        );
#endif        
        /**
           @brief Getter function for obtaining secondary QOS 
            
           Getter function for obtaining secondary QOS. Required for 
           testing functions like suspend and modify. Calling GetSecFlow 
           without an index returns the current secQos 
        
           @param[in] index Index of the secQos to return
        
           @retval IDSNetQoSSecondary* pointer to the created secQos
        */
        IDSNetQoSSecondary * GetSecQos
        (
          int index = -1
        );

#if 0
        /**
           @brief Setter function for the PSFlowHelperArr array
            
           Setter function for the PSFlowHelperArr array 
            
           @param[in] pPSFlowHelper PSFlowHelper pointer to set in the 
           array
            
           @param[in] index index in which to set. If the pointer is to 
           be asigned as the last element, the index should not be set. 
           The index should be set only to update already existing 
           PSFlowHelpers 
        */
        static void SetFlowHelper
        (
          PSFlowHelper * pPSFlowHelper,
          int index = -1
        );

        /**
           @brief Getter function for IfacePtr 
            
           Getter function for IfacePtr

           @param none 
              
           @retval ps_iface_type * ps_iface pointer
        */
        ps_iface_type * GetPSIfacePtr
        (
          void
        );

        /**
           @brief Getter function for elements in PhysLinkArr

           Getter function for elements in PhysLinkArr based 
           on index 
        
           @param[in] idx Index of PhysLinkHelper whose 
           physLink pointer to return 
            
           @retval ps_phys_link_type * PhysLink element
        */
        ps_phys_link_type * GetPhysLinkPtr
        (
          int idx
        );

        /**
           @brief Getter function for SocketFactory  

           Getter function for SocketFactory  

           @param None 
        
           @retval SocketFactory * returns the current SocketFactory 
                   pointer
        */
        DS::Sock::SocketFactory * GetSocketFactoryPtr
        (
          void
        );
#endif

        /**  
          @brief Getter function for Socket Pointer

          Getter function for Socket Pointer

          @param None 
        
          @retval Socket * returns the current Socket pointer 
        */
        DS::Sock::Socket * GetSocketPtr
        (
          void
        );

#if 0
        /**
           @brief Function to create a UDP socket with a default policy.

           Function to create a UDP socket with a default policy.
        
           @param[in] family      Socket family
           @param[in] socketType  Socket type
           @param[in] protocol    Socket protocol
        
           @retval ISocket * UDP socket set with a default policy.
        */
        DS::Sock::ISocket * SocketTestUtils::CreateSocket
        (
          DS::Sock::FamilyType      family,
          DS::Sock::SocketType      socketType,
          DS::Sock::ProtocolType    protocol
        );
#endif

        /**
          @brief Helper function for registering callbacks
          
          Function to register functions as a callback for the
          specified event.

          @param[in] event The event for which the call back function
            is being registered.      
          @param[in] callBackFn The callback function to be registered
          @param[in] arg The argument to b passed to the callback

          @retval int 0 if no errors were encountered
        */
        int RegisterCallBack
        (
          int event,
          void (*callBackFn)(void *arg),
          void* arg
        );

        /**
          @brief CleanUp function for clearing all the static variables 
                 at the end
          
          CleanUp function for clearing all the static variables at the 
          end or on error. 

          @param None 
         
          @retval int 0 if no errors were encountered 
        */                                              
        void CleanUp
        (
          void
        );

        /**
          @brief Function to test write.
          
          Function to test writing to the UDP socket and returns
          the error value.

          @param length Length of data to be written

          @retval DS::ErrorType Errors encountered in write
        */
        DS::ErrorType WriteData
        (
          int length = 10
        );
        
        /**
          @brief Function to test write.
          
          Function to test writing to the UDP socket and returns
          the error value.

          @param[in] sockPtr pointer of the socket to write to  
          @param[in] length  Length of data to be written

          @retval DS::ErrorType Errors encountered in write
        */
        DS::ErrorType SocketTestUtils::WriteData
        (
          ISocket * sockPtr,
          int       length = 10
        );

        /**
          @brief Function to return a dummy address to be used by the UDP socket to
          
          Function to return a dummy address to be used by the UDP socket to
          write to. 
         
          @param[out] remoteAddr dummy remote address to be return 

          @retval SockAddrStorageType * dummy address that was create 
        */
        DS::Sock::SockAddrStorageType * GetDummyAddress
        (
          DS::Sock::SockAddrStorageType * remoteAddr
        );

        /**
          @brief Method to make the current socket a system socket 
           
          Method to make the current socket a system socket 

          @param None 

          @retval None 
        */
        int SetSystemSocket
        (
          void
        );

        
        /**
           @brief Destructor for SocketUtils 
            
           Releases all the resources initialized by Init function. 
           Also calls the destructor of ifaceHelper, physLinkHelper 
           and flowHelper. Apart from this, calls release on 
           network object, network factory object, socket factory 
           object.
        
           @param None 
            
           @retval None 
        */
        ~SocketTestUtils
        (
          void
        );
        
        
        /**
          @brief Create a secondary Qos based on the passed QosManager 
          object 
            
          Create a secondary Qos based on the passed QosManager object. 
          It uses a default filter or the passed-in filter. 
            
          @param[in] dsQosManager QosManager used to create the QOS 
          @param[in] ipFilter     The sec QOS created is based on this 
          @param[in] flowCount    Number of flowSpecs to create 
          @param[in] filterCount  Number of filterSpecs to create  
          @retval int 0 if no error, non-zero otherwise
        */
        IDSNetQoSSecondary *  CreateFlow 
        (
          IDSNetQoSManager    * dsQosManager,            
          ip_filter_spec_type * ipFilter = NULL,
          int                   flowCount = 1,
          int                   filterCount = 1
        );

#if 0
        /**
           @brief Callback function for Ioctl cmd. Creates the PS Flow
          
           Creates the PS Flow and associates it with the passed 
           ps_iface_ptr and the corresponding physLinkPtr 
        
           @param[in] psIfacePtr Pointer to the iface being issued ioclt
           @param[in] ioctlName  Name of the Ioctl operation to perform 
           @param[in] argvalPtr  Contains the ioctl_info 
           @param[out] psErrno   returns errors if any
            
           @see PSFlowHelper::CreateInstance
            
           @retval int always 0
        */
        static int PSIfaceIoctlCallBack
        (
          ps_iface_type        * ps_iface_ptr,
          ps_iface_ioctl_type    ioctl_name,
          void                 * argval_ptr,
          int16                * ps_errno
        );
#endif
        
        /**
           @brief Helper function to create a test Flow Spec 
            
           Helper function to create a test Flow Spec 
        
           @param[in] flowCount Number of flowSpecs to be created 
            
           @retval IDSNetQoSFlow** Newly created FlowSpec Array
        */
        static IDSNetQoSFlow ** CreateTestQosFlowSpecs
        (
          int flowCount = 1
        );
        
        /**
           @brief Helper function to create a test Filter Spec 
            
           Helper function to create a test Filter Spec 
        
           @param[in] filterCount Number of filter Spec to be 
           created. 
           
           @retval IDSNetIPFilter** Newly created Filter array
        */
        static IDSNetIPFilter ** CreateTestFilterSpecs
        (
          int filterCount = 1
        );
         
#if 0
        /**
           @brief Function to set iface Callbacks 
            
           Function to set iface Callbacks
        
           @param[in] pIfaceTxCBFnPtr         Tx callback function 
                 pointer. Default implementation used to determine if
                 flowforwarding is enabled.
           @param[in] pBringUpIfaceCmdFnPtr   Callback function for 
                 IfaceUP Cmd
           @param[in] pTearDownIfaceCmdFnPtr  Callback function pointer 
                 for IfaceDown Cmd
           @param[in] pIfaceIoctlFnPtr        Callback function for PS 
                 Iface IOCTL Cmd
           @param[in] pPsFlowIoctlFnPtr       Callback function for PS 
                 Flow IOCTL Cmd
            
           @retVal None  
        */
        static void SetIfaceCallBacks
        (
          ps_iface_tx_cmd_f_ptr_type    pIfaceTxCBFnPtr = NULL,
          ps_iface_cmd_f_ptr_type       pBringUpIfaceCmdFnPtr = NULL,
          ps_iface_cmd_f_ptr_type       pTearDownIfaceCmdFnPtr = NULL,
          ps_iface_ioctl_f_ptr_type     pIfaceIoctlFnPtr = NULL,
          ps_flow_ioctl_f_ptr_type      pPsFlowIoctlFnPtr = NULL
        );
        
        /**
           @brief Function for setting physLink related callback 
                  functions
            
           Function for setting physLink related callback 
                  functions 
 
           @param[in] pPhysLinkUpFnPtr   Callback function for 
                 physLinkUp event
           @param[in] pPhysLinkDownFnPtr Callback function for 
                 physLinkDown event
           @param[in] pPhysLinkNullFnPtr Callback function for 
                 physLinkNull event
            
           @retVal None 

        */
        static void SetPhysLinkCallBacks
        (
          ps_phys_link_cmd_f_ptr_type   pPhysLinkUpFnPtr = NULL,
          ps_phys_link_cmd_f_ptr_type   pPhysLinkDownFnPtr = NULL,
          ps_phys_link_cmd_f_ptr_type   pPhysLinkNullFnPtr = NULL  
        );

        /**
          @brief Function for setting Flow related callback functions 
           
          Function for setting Flow related callback functions 
        
          @param[in] activateCmdFuncPtr  Flow activate callback 
          function. 
          @param[in] configureCmdFuncPtr Flow Configure callback 
          function. 
          @param[in] suspendCmdFuncPtr   Flow Suspend callback function
          @param[in] goNullCmdFuncPtr    Flow Null callback function
          @param[in] ioctlCmdFuncPtr     Flow IOCTL callback function 
           
          @retVal None 
        */
        static void SetFlowCallBacks
        (
          ps_flow_cmd_f_ptr_type        pActivateCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type        pConfigureCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type        pSuspendCmdFuncPtr = NULL,
          ps_flow_cmd_f_ptr_type        pGoNullCmdFuncPtr = NULL,
          ps_flow_ioctl_f_ptr_type      pIoctlCmdFuncPtr = NULL 
        );
#endif

        /**
          @brief Function to initialize Socket
          
          The function creates the socket(creates the physLinks 
          and iface if required), initializes the created physLinks
          and issues a write to build the route_meta_info 

          @param[in] family        Socket family
          @param[in] socketType    Socket type
          @param[in] protocol      Socket protocol 
          @param[in] numPhysLinks  Number of physLinks to be created    
          @param[in] numSecFlows   Number of secondary flow to be 
                created.
           
          @retval int              0 if no erros are encountered.
        */

        int SocketTestUtils::Init
        (
          FamilyType                      family,
          SocketType                      socketType,
          ProtocolType                    protocol,
          int                             numPhysLinks = 1,
          int                             numSecFlows = 0
        );

        /**
           @brief  Create the Iface and the default flow. Set the 
                   callbacks for the iface and default flow.
            
           Create the Iface and the default flow. Set the 
                   callbacks for the iface and default flow.
            
           @param[in] numPhysLinks            Number of physLinks to be created 
           @param[in] pIfaceTxCBFnPtr         Tx callback function 
                 pointer. Default implementation used to determine if
                 flowforwarding is enabled.
           @param[in] pBringUpIfaceCmdFnPtr   Callback function for 
                 IfaceUP Cmd
           @param[in] pTearDownIfaceCmdFnPtr  Callback function pointer 
                 for IfaceDown Cmd
           @param[in] pIfaceIoctlFnPtr        Callback function for PS 
                 Iface IOCTL Cmd
           @param[in] pPsFlowIoctlFnPtr       Callback function for PS 
                 Flow IOCTL Cmd
        
           @retval PSIfaceHelper* pointer to PSIfaceHelper
        */
        virtual PSIfaceHelper * CreateIfaceAndDefaultFlow
        (
          int                           numPhysLinks = 2,
          ps_iface_tx_cmd_f_ptr_type    pIfaceTxCBFnPtr = NULL,
          ps_iface_cmd_f_ptr_type       pBringUpIfaceCmdFnPtr = NULL,
          ps_iface_cmd_f_ptr_type       pTearDownIfaceCmdFnPtr = NULL,
          ps_iface_ioctl_f_ptr_type     pIfaceIoctlFnPtr = NULL,
          ps_flow_ioctl_f_ptr_type      pPsFlowIoctlFnPtr = NULL
        );

        /**
           @brief Start the posix task
            
           Start the PosixTask and so it starts listening for message 
           router messages 
            
           @param[in] None 
                   
           @retval PosixTask* 
        */
        virtual PosixTask * StartPosixTask
        (
          void
        );

        /**
           @brief Create a IDSNetpolicy
        
           Create a default policy 
            
           @param None 
            
           @retval IDSNetPolicy* pointer to policy
        */
        virtual IDSNetPolicy * CreateNetPolicy
        (
          void
        );

        /**
           @brief Method to set the Callback functions for 
           physLinks 
            
           Method to set the Callback functions for 
           physLinks. PhysLinks can be obtained from IfaceHelper 
           objects. 
        
           @param[in] ifaceHelper Pointer to ifaceHelper object
            
           @retVal None 
        */
        virtual void SetPhysLinkCallBacks
        ( 
          PSIfaceHelper * ifaceHelper
        );
        
        /**
           @brief Helper method to create and bringup a network object 
            
           Helper method to create and bringup a network object 
        
           @param[in] netPolicyPtr  pointer to network policy based on 
                 which network is to be created
            
           @retval IDSNetwork* pointer to netwoek object
        */
        virtual IDSNetwork * SocketTestUtils::CreateNetwork
        (
          IDSNetPolicy      * netPolicyPtr
        );

        /**
           @brief Helper function to create QosManager 
            
           Helper function to create QosManager 
        
           @param[in] network pointer to the network object
        
           @retval IDSNetQoSManager* pointer to the newly created 
           QosManager 
        */
        virtual IDSNetQoSManager * CreateQosManager
        (
          IDSNetwork * network
        );

        /**
           @brief Helper function to create secondary flow 
            
           Helper function to create secondary flow  
        
           @param[in] psIface pointer to the IfaceHelper for 
           which the flow has to be created 
           @param[in] flowIndex 
       */
        virtual void SocketTestUtils::ConfigureSecFlow
        (
          PSIfaceHelper * psIface,
          int             Index
        );
        
        /**
           @brief Helper method that can be overriden to decide if 
           flows must be activated or not. 
        
           @param[in] psIface   Pointer to PSIfaceHelper containing 
           the flow to be activated 
           @param[in] flowIndex Index of the flow to be activated
            
           @retval None 
        */
        void SocketTestUtils::ActivateSecFlows
        (
          PSIfaceHelper * psIface,
          int             flowIndex
        );
        
        /**
           @brief Overridable helper function to Create Socket based 
           on the network object 
            
           Overridable helper function to Create Socket based 
           on the network object 
            
           @param[in] family      Socket family
           @param[in] socketType  Socket type
           @param[in] protocol    Socket protocol
           @param[in] network     network to create socket based on

        @retval ISocket * UDP socket set with a default policy.
        */

        virtual ISocket * SocketTestUtils::CreateSocket
        (
          FamilyType   family,
          SocketType   socketType,
          ProtocolType protocol,
          IDSNetwork * network
        );

        /**
           @brief Callback method to wait on specific events 
           if required 
            
           The  method to wait on specific events 
           if required . This method waits for IfaceUp, 
           physLinkUp for all physLinks and FlowActivate 
           for all flows including the default flow
        
           @param[in] ifaceHelper pointer to ifaceHelper to obtain 
           physLinkHelpers and or FlowHelpers 
           @param[in] numPhysLinks Number of created physLinks to 
           determine how many times wait should be issued. 
           @param[in] numPhysLinks Number of created flows to 
           determine how many times wait should be issued 

           @retVal None 
        */
        virtual void WaitForEvents
        ( 
          PSIfaceHelper * ifaceHelper,
          int             numPhysLinks,
          int             numSecFlows
        );
        
        /**
           @brief Default implementation of method to bringup 
           physlinks 
            
           Default implementation of virtual method to bring up all the 
           phys links. Should be overridden if any other behavior is 
           desired. 
        
           @param[in] ifaceHelper pointer to ifaceHelper containing 
           the physlinks 
            
           @retval None 
        */

        virtual void SocketTestUtils::BringUpPhysLinks
        ( 
          PSIfaceHelper * ifaceHelper 
        );

      private:
        /*===================================================================

                      PRIVATE DATA DECLARATIONS

        ===================================================================*/
        /**
           @brief The pointer to the ifaceHelper encapsulating the 
           ifacePtr used to bring upp the network
            
           The pointer to the ifaceHelper encapsulating the ifacePtr 
           used to bring up the network 
        */                                            
        static PSIfaceHelper     * ifaceHelper;

#if 0
        /**
           @brief An array of PhysLinkHelpers encapsulating physLinks to 
           be associated with the PSIface. 
            
           An array of PhysLinkHelpers encapsulating physLinks to 
           be associated with the PSIface.  
        */
        static PhysLinkHelper   ** physLinkHelperArr;

        /**
           @brief Index of the physLink associated with the flow. 
            
             Index of the physLink associated with the flow. Since
             ps_create_flow is called automatically from a callback,
             this variable is set before requesting for seconndary QOS
        */
        static int                 physLinkIndex;

        /**
           @brief Network Factory to create the Network object. 
            
           Network Factory to create the Network object. 
        */
        IDSNetworkFactory        * netFactory;
#endif
        /**
           @brief Pointer to network object to create and bring 
           up network 
        
           Pointer to network object to create and bring 
           up network 
        */
        IDSNetwork               * network;
#if 0
        /**
           @brief Pointer to Qos Manager to request secondary 
           QOS. 
        
           Pointer to Qos Manager to request secondary 
           QOS 
        */
        IDSNetQoSManager         * qosManager; 
         

        /**
           @brief Pointer to SocketFactory to create Socket
        
           Pointer to SocketFactory to create Socket 
        */
        DS::Sock::SocketFactory  * socketFactoryPtr;
#endif
        /**
           @brief Pointer to the created Socket
        
           Pointer to the created Socket 
        */
        ISocket                  * sockPtr;

        /**
           @brief Pointer to Network policy
        
           Pointer to network policy to create a network suitable for 
           the given policy 
        */
        IDSNetPolicy             * netPolicyPtr;

#if 0
        /**
           @brief An array of FlowHelper objects to store the flows 
           created in the callback 
        
           An array of FlowHelper objects to store the flows 
           created in the callback
        */
        static PSFlowHelper      * flowHelperArr [ NUM_FLOW_HELPER ];

        /**
           @brief Index to maintain the highest PS Flow index seen so 
                  far.
        
           Index to maintain the highest PS Flow index seen so 
                  far
        */ 
         
#endif
        static int                 currentFlowCount;
        /**
           @brief Pointer to PosixTask to post messages and wait for 
           semaphore being signalled 
            
           Pointer to PosixTask to post messages and wait for 
           semaphore being signalled 
        */  
        PosixTask                * posixTask;

        /**
           @brief Pointer to Sec QOS being requested
        
           Pointer to Sec QOS being requested 
        */
        IDSNetQoSSecondary       * secQosArr [ NUM_FLOW_HELPER ];

#if 0
        /**
           @brief Spec Type used to request QOS
        
           Spec Type used to request QOS 
        */
        DS::NetQoS::SpecType     * specType; 
         
        /**
           @brief Tx callback function pointer. Default implementation 
                  used to determine if flowforwarding is enabled.
            
           Tx callback function pointer. Default implementation used to 
           determine if flowforwarding is enabled. 
        */
        static ps_iface_tx_cmd_f_ptr_type    ifaceTxCBFnPtr;

        /**
           @brief Callback function for IfaceUP Cmd 
            
           Callback function for IfaceUP Cmd
        */
        static ps_iface_cmd_f_ptr_type       bringUpIfaceCmdFnPtr;

        /**
           @brief Callback function pointer for IfaceDown Cmd 
            
           Callback function pointer for IfaceDown Cmd
        */
        static ps_iface_cmd_f_ptr_type       tearDownIfaceCmdFnPtr;

        /**
           @brief Callback function for PS Iface IOCTL Cmd
            
           Callback function for PS Iface IOCTL Cmd 
        */
        static ps_iface_ioctl_f_ptr_type     ifaceIoctlFnPtr;

        /**
           @brief Callback function for PS Flow IOCTL Cmd
            
           Callback function for PS Flow IOCTL Cmd 
        */ 
        static ps_flow_ioctl_f_ptr_type      psFlowIoctlFnPtr;

        /**
           @brief Callback function for physLinkUp event
            
           Callback function for physLinkUp event 
        */
        static ps_phys_link_cmd_f_ptr_type   physLinkUpFnPtr;

        /**
           @brief Callback function for physLinkDown event
        
           Callback function for physLinkDown event 
        */
        static ps_phys_link_cmd_f_ptr_type   physLinkDownFnPtr;

        /**
           @brief Callback function for physLinkNull event 
            
           Callback function for physLinkNull event 
        */
        static ps_phys_link_cmd_f_ptr_type   physLinkNullFnPtr;
        
        /**
           @brief Flow activate callback function. 
            
           Flow activate callback function. 
        */ 
        static ps_flow_cmd_f_ptr_type        activateCmdFuncPtr;

        /**
           @brief Flow Configure callback function. 
            
           Flow Configure callback function. 
        */ 
        static ps_flow_cmd_f_ptr_type        configureCmdFuncPtr;

        /**
           @brief Flow Suspend callback function 
            
           Flow Suspend callback function 
        */ 
        static ps_flow_cmd_f_ptr_type        suspendCmdFuncPtr;

        /**
           @brief Flow Null callback function 
            
           Flow Null callback function 
        */
        static ps_flow_cmd_f_ptr_type        goNullCmdFuncPtr;

        /**
           @brief Flow IOCTL callback function
        
           Flow IOCTL callback function
        */
        static ps_flow_ioctl_f_ptr_type      ioctlCmdFuncPtr;
#endif
    }; /* class SocketTestUtils */
  } /* namespace QTF */
}/* namespace PS */
#endif
