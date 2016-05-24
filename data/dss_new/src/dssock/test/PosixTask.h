#ifndef POSIX_TASK_H
#define POSIX_TASK_H

/*===========================================================================
  @file PosixTask.h

  This file is a helper wrapping a Posix Task thread. It contain
  functions to listen to messages from event call backs or test cases and
  call the corresponding indications. It also has helper synchronization
  methods.
       
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/test     
      /PosixTask.h $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
extern "C" {
#include "pthread.h"
#include "semaphore.h"
#include "ps_iface.h"
#include "ps_phys_link.h"
#include "ds_flow_control.h"
}

#include "CommonUtils.h"
#include "msg.h"
#include "qtf.h"
#include "qtf_tmc.h"
#include "qtf_msgr_msg.h"

/*===================================================================

         PUBLIC DATA DECLARATIONS

===================================================================*/
      
namespace PS
{
  namespace QTF
  {

    typedef int EventType;

    namespace Event {

       const ::PS::QTF::EventType READ = 0x1;

       const ::PS::QTF::EventType WRITE = 0x2;

       const ::PS::QTF::EventType IFACE_UP = 0x3;

       const ::PS::QTF::EventType IFACE_DOWN = 0x4;

       const ::PS::QTF::EventType IFACE_CONFIGURE = 0x5;

       const ::PS::QTF::EventType FLOW_ACTIVATE = 0x6;

       const ::PS::QTF::EventType FLOW_SUSPEND = 0x7;
       
       const ::PS::QTF::EventType END_POSIX_TASK = 0x8;

       const ::PS::QTF::EventType PHYS_LINK_UP = 0x9;

       const ::PS::QTF::EventType PHYS_LINK_DOWN = 0xA;

       const ::PS::QTF::EventType PHYS_LINK_GONE = 0xB;

       const ::PS::QTF::EventType IFACE_ROUTABLE_IND = 0XC;

       const ::PS::QTF::EventType CHANGE_IP = 0XD;

       const ::PS::QTF::EventType CHANGE_FLTR = 0XE;
       
       const ::PS::QTF::EventType FLOW_DISABLED = 0XF;

       const ::PS::QTF::EventType FLOW_ENABLED = 0X10;

       const ::PS::QTF::EventType FLOW_TX_DISABLED = 0X11;

       const ::PS::QTF::EventType FLOW_TX_ENABLED = 0X12;

       const ::PS::QTF::EventType PHYSLINK_FLOW_ENABLED = 0X13;

       const ::PS::QTF::EventType PHYSLINK_FLOW_DISABLED = 0X14;

       const ::PS::QTF::EventType SOCKET_EVENT_MAX = 0x15;
    }

    class PosixTask
    {
      public:
      
      /**
         @brief Routine to create a new thread 
          
         Function that executes the passed in start_routine as an 
         independent thread passing it the argument arg. 
      
         @param[in] startRoutine Function to be executed as a new 
          thread 
         @param[in] arg argument to be passed to the start_routine 
          
         @retval pthread_t* threadId of the newly created thread. 
      */
      pthread_t * CreatePosixThread
      (
        void            * ( * startRoutine )( void * ),
        void            *     arg
      );

      /**
         @brief Initializes the threadTask 
          
         Initializes the semaphore array and starts 
         the new thread. 
          
         @param None 
      
         @retval int 0 Success. Non-zero otherwise
      */
      int Init
      (
        void
      );
      
      /**
         @brief Body of the PosixTask 
          
         The main function of posixtask which waits in a loop for 
         messages from MessageRouter and calls the appropriate 
         callback function. 
      
         @param arg An optional argument that could be passed to the 
         task. 
          
         @retval None 
      */
      static void * PosixTaskStart
      (
        void * arg = NULL
      );
      
      /**
         @brief Wait till the PosixThread completes 
          
         Causes the main thread to wait in the join system call 
         till the posixthread completes. This function is required 
         as QTF checks if all threads are already completed as 
         part of tear down. 
          
         @param[in] None 
          
         @retval void* Return value from the running thread
      */
      void * PosixTask::WaitForThreadCompletion
      (
        void
      );

      /**
         @brief Helper function to post message to the task
          
         Helper function to post message to the task 
          
         @param[in] messageType Type of the message to be posted
         @param[in] userData    void pointer to be passed to 
            the callback function 
          
         @see PS::QTF::Event
          
         @retval int 
      */
      static int PostMessage
      (
        int      messageType, 
        void   * userData
      );

      /**
        @brief Function that waits till the write callback is called

        Function that waits for the type of message passed in event 
        using semaphores 
         
        @param[in] event Event to wait for

        @retval : None
      */ 
      static void WaitForCallBack
      (
        PS::QTF::EventType event
      );


      /**
         @brief Function registered as callback for the write event 
    
         When this function is called to signal the corresponding 
         semaphore indicating the write event 

         @param[in] arg Not used
   
         @retval None 

      */
      static void WriteCallBack
      (
        void * arg
      );

      /**
         @brief Function registered as callback for the flow configure
                event
    
         When this function is called post the corresponding semaphore
         indicating the flow configure event 

         @param[in] arg Not used
   
         @retval None 

      */
      static void IfaceConfigureCallBack
      (
        void* arg
      );
      
      /**
         @brief Function registered as callback for the event passed 
         as argument 
          
         Function registered as callback for the event passed 
         as argument. When this function is called post the 
         corresponding semaphore indicating the event 

         @param[in] this_iface_ptr pointer to the iface on which call 
         back is called. 
         @param[in] event          event to trigger the callback 
         @param[in] event_info     event_info of event 
         @param[in] user_data_ptr  pointer to user passed in info 

         @retval None 

     */
      static void PosixTask::EventCallBack
      (
        ps_iface_type             * this_iface_ptr,
        ps_iface_event_enum_type    event,
        ps_iface_event_info_u_type  event_info,
        void                      * user_data_ptr
      );

      /**
         @brief Function registered as callback for the flow activate
                event
    
         When this function is called post the corresponding semaphore
         indicating the flow activate event 

         @param[in] arg Not used
   
         @retval None 

      */
      static void PosixTask::FlowActivateCallBack
      (
        void * arg
      );
      
      /**
         @brief Function registered as callback for the iface down
         event
    
         When this function is called it send a message router message 
         indicating the iface down event 

         @param[in] arg Not used
   
         @retval None 
      */
      static void IfaceDownCallBack
      (
        void * arg
      );

      /**
        @brief Function registered as callback for the iface down
               event
    
        When this function is called it send a message router message 
        indicating the iface down event 

        @param[in] arg :Not used
   
        @retval : None 

      */
      static void IfaceUpCallBack
      (
        void * arg
      );
      
      /**
         @brief Signal the semaphore corresponding to the passed index
          
         Signal the semaphore corresponding to the passed index 
          
         @param[in] index Index of the semaphore to be signalled
      */
      static void PosixTask::CallBack
      (
        int index
      );

    private:
      /*===================================================================

                    PRIVATE DATA DECLARATIONS

      ===================================================================*/
      /**
         @brief Stores the threadId of the PosixTask thread.
      
         Stores the threadId of the PosixTask thread.
      */
      pthread_t        threadId;

      /**
         @brief Array of semaphores, one for each type of event
      
         Array of semaphores, one for each type of event. 
      */ 
      static sem_t     callBackSemaphore[ PS::QTF::Event::SOCKET_EVENT_MAX ];

    }; /* class PosixTask */
  } /* QTF */
} /* PS */
#endif /* POSIX_TASK_H */
