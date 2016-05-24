#ifndef REX_UTILS_H
#define REX_UTILS_H
/*===========================================================================
  @file RexUtils.h

  This file is a helper containing functions to start ps_task,
  dcc_task.     
       
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datacommon/dssock/main/latest/     
      test/RexUtils.h $
  $DateTime: 2009/09/28 14:45:31 $ $Author: dmudgal $

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
extern "C" {
#include "task.h"
#include "ps.h"
#include "dcc_task.h"
}              

#include "qtf.h"
#include "qtf_tmc.h"
#include "qtf_msgr_msg.h"

namespace PS
{
  namespace QTF
  {

  class RexUtils
  {
    public:  
    /*===========================================================================

                      PUBLIC DATA DECLARATIONS

    ===========================================================================*/
      /**
      @brief create PS Task
      
      Function to spawn the PS Task and wait till PS Task 
          has started.

      @param[in] name  name of the task

      @retval None
      */ 
      void SpawnPSTask
      (
        char * name
      ); 

      /**
        @brief Create DCC Task
        
        Completes the DCC initialization and starts the task
     
        @param[in] name  Name of the DCC task

        @retval  None
      */
      void RexUtils::SpawnDCCTask
      (
        char * name
      );
  }; /* class RexUtils */

} /* namespace QTF */
} /* namespace PS */

#endif /* #ifndef REX_UTILS_H */
