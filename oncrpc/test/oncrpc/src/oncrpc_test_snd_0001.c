/******************************************************************************
  @file  oncrpc_test
  @brief Linux user-space snd register test

  DESCRIPTION
  Oncrpc test program for Linux user-space .

  -----------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
06/11/08   rr       Initial version, based on oncrpc_test
02/18/09   rr       Add missing sndcb_app_init();

======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <unistd.h>
#include "oncrpc.h"
#define SYS_USER_PPLMN_LIST_MAX_LENGTH 85
#define SYS_PLMN_LIST_MAX_LENGTH 40
#include "snd.h"
#include "snd_rpc.h"

/*=====================================================================
     External declarations
======================================================================*/


/*=====================================================================
      Constants and Macros
======================================================================*/
static char *VersionStr="1.2";


void snd_callback( const void      *client_data, /* pointer to Client data   */
        snd_status_type status )     /* Status returned by Sound */
{
   printf("Snd Callback data_ptr:0x%08x  status:%d\n",(unsigned int) client_data, (int)status);
}

/*===========================================================================
  FUNCTION  main
===========================================================================*/
/*!
@brief
  entry to test

@return
  0

@note
  - Dependencies
    - ONCRPC
  - Side Effects
*/
/*=========================================================================*/
int main(int argc, char *argv[])
{
   int cnt=0;
   int nunTestsToRun = 1;

   printf("%s, version %s\n",argv[0],VersionStr);
   printf("Testing SND_NULL and SND_SET_DEVICE \n");
   printf("Usage: oncrpc_test <num iterations>, default = 1 \n\n");

   oncrpc_init();
   oncrpc_task_start();
   sndcb_app_init();
   if(argc == 1)
   {
      nunTestsToRun = 1;
   } else
   {
      nunTestsToRun = atoi(argv[1]);
   }
   printf("\n\nRunning for %d iterations ...\n",nunTestsToRun);
   printf("ONCRPC TEST STARTED...\n");

   if(nunTestsToRun == 0)
   {
      while(1)
      {
         printf("Launch RPC call, test#%d\n",cnt);
         cnt++;
         snd_null();
         {
            snd_device_type       device = SND_DEVICE_CURRENT;

            /* The device chosen for this command   */
            snd_mute_control_type ear_mute = SND_MUTE_UNMUTED;
            /* Mute, unmute output                  */
            snd_mute_control_type mic_mute = SND_MUTE_UNMUTED;
            /* Mute, unmute microphone              */

            /* Call back function                   */
            snd_set_device(device, ear_mute, mic_mute, snd_callback, 0);
         }
      }
   } else
   {
      for(cnt=0;cnt < nunTestsToRun;cnt++)
      {
         printf("Launch RPC call, test#%d\n",cnt);
         snd_null();
         {
            snd_device_type       device = SND_DEVICE_CURRENT;

            /* The device chosen for this command   */
            snd_mute_control_type ear_mute = SND_MUTE_UNMUTED;
            /* Mute, unmute output                  */
            snd_mute_control_type mic_mute = SND_MUTE_UNMUTED;
            /* Mute, unmute microphone              */

            /* Call back function                   */


            snd_set_device(device, ear_mute, mic_mute, snd_callback, 0);

         }
      }
   }

   printf("%s COMPLETE...\n",argv[0]);
   printf("PASS\n");
   return(0);
}


