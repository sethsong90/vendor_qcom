/*===========================================================================
  @file PhysLinkHelper.h

  This file is a helper wrapping a PhysLink pointer and providing helper       
  functions to be used for QTF testcases.     
       
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/       
        test/PhysLinkHelper.h $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/
#ifndef PHYSLINKHELPER_H
#define PHYSLINKHELPER_H

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "PosixTask.h"

extern "C" {
#include "ps_phys_link.h"
#include "ps_phys_linki_event.h"
}

namespace PS
{
  namespace QTF
  {

    class PhysLinkHelper
    {

      public:
        /*===========================================================================

                      PUBLIC DATA DECLARATIONS

        ===========================================================================*/

        /**
           @brief Returns the wrapped physLink pointer
           
           Returns the wrapped physLink pointer
        
           @param[in] None
           
           @retval ps_phys_link_type * Pointer to the wrapped 
           physLink. 
        */
        ps_phys_link_type* PhysLinkHelper::GetPhysLinkPtr
        (
          void
        );


        /**
           @brief Default call back function for physLinkUpCmd. 
           
           Default call back function for physLinkUpCmd. Posts message 
           to the PosixTask for ps_phys_link_up_ind. 
        
           @param[in] phys_link_ptr pointer to physLink coming up
           @param[in] client_data   Ignored
        
           @retval int 0 If no errors
        */
        static int PSIfacePhysLinkUpCmdCB
        (
          ps_phys_link_type * physLinkPtr,
          void              * clientData
        );
        
        /**
           @brief Default call back function for physLinkUpCmd. 
           
           Default call back function for physLinkUpCmd. Posts message 
           to the PosixTask for ps_phys_link_up_ind. 
        
           @param[in] phys_link_ptr pointer to physLink coming up
           @param[in] client_data   Ignored
        
           @retval int 0 If no errors
        */
        static int IfacePhysLinkUpCmdCB
        (
          ps_phys_link_type * physLinkPtr,
          void              * clientData
        );
        /**
           @brief Call back function for physLinkUpCmd. 
           
           Call back function for physLinkUpCmd. Does Nothing
        
           @param[in] phys_link_ptr pointer to physLink coming up
           @param[in] client_data   Ignored
        
           @retval int 0 If no errors
        */
        static int PSIfacePhysLinkUpCmdDoNothingCB
        (
          ps_phys_link_type * physLinkPtr,
          void              * clientData
        );

        /**
           @brief Default Call back function for physLinkDownCmd. 
           
           Default Call back function for physLinkDownCmd. Posts message 
           to the PosixTask for ps_phys_link_down_ind 
        
           @param[in] phys_link_ptr pointer of physlink going down
           @param[in] client_data   Ignored
        
           @retval int 0 If no errors
        */
        static int
        PSIfacePhysLinkDownCmdCB
        (
          ps_phys_link_type * physLinkPtr,
          void              * clientData
        );

        /**
           @brief Default Callback function for physLinkNullCmd.

           Default Callback function for physLinkNullCmd. Posts message 
           to the PosixTask for ps_phys_link_gone_ind 
        
           @param[in] phys_link_ptr pointer of physLink going Null
           @param[in] client_data   Ignored 
        
           @retval int 0 If no error.
        */
        static int
        STAPSIfacePhysLinkGoNullCmdCB
        (
          ps_phys_link_type * physLinkPtr,
          void              * clientData
        );

        /**
           @brief Public constructor for PhysLinkHelper
           
           The function initializes the physLink in the given 
           index by assigning it callback functions and bringing it up. 
            
           @param[in] pPhysLink         Pointer to phys_link being 
                 wrapped
           @param[in] physLinkUpFnPtr   Callback function for physLinkUp
                 event
           @param[in] physLinkDownFnPtr Callback function for 
                 physLinkDown event
           @param[in] PhysLinkNullFnPtr Callback function for 
                 physLinkNull event
            
           @returnval None
        */
        PhysLinkHelper
        (
          ps_phys_link_type          * pPhysLink,
          ps_phys_link_cmd_f_ptr_type  physLinkUpFnPtr = NULL,
          ps_phys_link_cmd_f_ptr_type  physLinkDownFnPtr = NULL,
          ps_phys_link_cmd_f_ptr_type  PhysLinkNullFnPtr = NULL
        );

        /**
           @brief Helper function for posting physLink related messages 
            
           Helper function for posting physLink related messages 
        
           @param[in] eventType The message type to be posted
            
           @retval None         
        */
        void PostPhysLinkMessage
        (
          PS::QTF::EventType eventType
        );

      private:
        /*===========================================================================

                      PRIVATE DATA DECLARATIONS

        ===========================================================================*/
        
        /**
           @brief phys_link pointer being wrapped around by this class.
        */

        ps_phys_link_type * physLink;

    }; /* class PhysLinkHelper */
  } /* namespace QTF */
} /* namespace PS */
#endif /* PHYSLINKHELPER_H */

