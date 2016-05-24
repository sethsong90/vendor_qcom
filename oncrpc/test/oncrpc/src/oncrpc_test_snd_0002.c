/******************************************************************************
  @file  oncrpc_test_snd_0002
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

when         who      what, where, why
--------     ---      -------------------------------------------------------
04/15/2009   rr       Initial version, based on oncrpc_test


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
#define SND_FIRST_TDB_TONE 1000
enum tdb_tones
{

   SND_FIRST_TONE = SND_FIRST_TDB_TONE,/* Use for range checking 1st tone  */
   SND_0,              /* DTMF for 0 key                                   */
   SND_1,              /* DTMF for 1 key                                   */
   SND_2,              /* DTMF for 2 key                                   */
   SND_3,              /* DTMF for 3 key                                   */
   SND_4,              /* DTMF for 4 key                                   */
   SND_5,              /* DTMF for 5 key                                   */
   SND_6,              /* DTMF for 6 key                                   */
   SND_7,              /* DTMF for 7 key                                   */
   SND_8,              /* DTMF for 8 key                                   */
   SND_9,              /* DTMF for 9 key                                   */
   SND_A,              /* DTMF for A key                                   */
   SND_B,              /* DTMF for B key                                   */
   SND_C,              /* DTMF for C key                                   */
   SND_D,              /* DTMF for D key                                   */
   SND_POUND,          /* DTMF for # key                                   */
   SND_STAR,           /* DTMF for * key                                   */
   SND_CTRL,           /* Tone for a control key                           */
   SND_2ND,            /* Tone for secondary function on a key             */
   SND_WARN,           /* Warning tone (e.g. overwriting user phone# slot) */
   SND_ERR,            /* Tone to indicate an error                        */
   SND_TIME,           /* Time marker tone                                 */
   SND_RING_A,         /* 1st Ringer tone                                  */
   SND_RING_B,         /* 2nd Ringer tone                                  */
   SND_RING_C,         /* 3rd Ringer tone                                  */
   SND_RING_D,         /* 4th Ringer tone                                  */
   SND_RING_A4,        /*  440.0 Hz  -Piano Notes-                         */
   SND_RING_AS4,       /*  466.1 Hz                                        */
   SND_RING_B4,        /*  493.8 Hz                                        */
   SND_RING_C4,        /*  523.2 Hz                                        */
   SND_RING_CS4,       /*  554.3 Hz                                        */
   SND_RING_D4,        /*  587.3 Hz                                        */
   SND_RING_DS4,       /*  622.2 Hz                                        */
   SND_RING_E4,        /*  659.2 Hz                                        */
   SND_RING_F4,        /*  698.5 Hz                                        */
   SND_RING_FS4,       /*  739.9 Hz                                        */
   SND_RING_G4,        /*  784.0 Hz                                        */
   SND_RING_GS4,       /*  830.6 Hz                                        */
   SND_RING_A5,        /*  880.0 Hz                                        */
   SND_RING_AS5,       /*  932.2 Hz                                        */
   SND_RING_B5,        /*  987.7 Hz                                        */
   SND_RING_C5,        /* 1046.5 Hz                                        */
   SND_RING_CS5,       /* 1108.7 Hz                                        */
   SND_RING_D5,        /* 1174.6 Hz                                        */
   SND_RING_DS5,       /* 1244.3 Hz                                        */
   SND_RING_E5,        /* 1318.5 Hz                                        */
   SND_RING_F5,        /* 1397.0 Hz                                        */
   SND_RING_FS5,       /* 1479.9 Hz                                        */
   SND_RING_G5,        /* 1568.0 Hz                                        */
   SND_RING_GS5,       /* 1661.2 Hz                                        */
   SND_RING_A6,        /* 1760.0 Hz                                        */
   SND_RING_AS6,       /* 1864.7 Hz                                        */
   SND_RING_B6,        /* 1975.5 Hz                                        */
   SND_RING_C6,        /* 2093.1 Hz                                        */
   SND_RING_CS6,       /* 2217.4 Hz                                        */
   SND_RING_D6,        /* 2349.3 Hz                                        */
   SND_RING_DS6,       /* 2489.1 Hz                                        */
   SND_RING_E6,        /* 2637.0 Hz                                        */
   SND_RING_F6,        /* 2793.7 Hz                                        */
   SND_RING_FS6,       /* 2959.9 Hz                                        */
   SND_RING_G6,        /* 3135.9 Hz                                        */
   SND_RING_GS6,       /* 3322.4 Hz                                        */
   SND_RING_A7,        /* 3520.0 Hz                                        */
   SND_RBACK,          /* Ring back (audible ring)                         */
   SND_BUSY,           /* Busy tone                                        */
   SND_INTERCEPT_A,    /* First tone of an intercept                       */
   SND_INTERCEPT_B,    /* Second tone of an intercept                      */
   SND_REORDER_TONE,   /* Reorder                                          */
   SND_PWRUP,          /* Power-up tone                                    */
   SND_OFF_HOOK_TONE,  /* Off-hook tone, IS-95 (CAI 7.7.5.5)               */
   SND_CALL_WT_TONE,   /* Call-waiting tone                                */
   SND_DIAL_TONE_TONE, /* Dial tone                                        */
   SND_ANSWER_TONE,    /* Answer tone                                      */
   SND_HIGH_PITCH_A,   /* 1st High pitch for IS-54B alerting               */
   SND_HIGH_PITCH_B,   /* 2nd High pitch for IS-54B alerting               */
   SND_MED_PITCH_A,    /* 1st Medium pitch for IS-54B alerting             */
   SND_MED_PITCH_B,    /* 2nd Medium pitch for IS-54B alerting             */
   SND_LOW_PITCH_A,    /* 1st Low pitch for IS-54B alerting                */
   SND_LOW_PITCH_B,    /* 2nd Low pitch for IS-54B alerting                */
   SND_TEST_ON,        /* Test tone on                                     */
   SND_MSG_WAITING,    /* Message Waiting Tone                             */
   SND_PIP_TONE_TONE,  /* Used for Pip-Pip-Pip-Pip (Vocoder) Tone          */
   SND_SPC_DT_INDIA,   /* Used for India's Special Dial Tone               */
   SND_SIGNAL_INDIA,   /* Used in Various India Signalling Tones           */
   SND_DT_TONE_INDIA,  /* Used for India's Normal Dial Tone (and others)   */
   SND_DT_TONE_BRAZIL, /* Used for Brazil's Dial Tone                      */
   SND_DT_DTACO_TONE,  /* Used for DTACO's single tone (350Hz, 350Hz)      */
   SND_HFK_TONE1,      /* These two tones used for Voice Activation and    */
   SND_HFK_TONE2,      /* Incoming Call Answer in phone VR-HFK             */
   SND_LAST_TONE       /* Use for range checking last tone                 */

};

static char *VersionStr="1.3 >>> SND_TONE_START";


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
   int nunTestsToRun = 5000;

   printf("%s, version %s\n",argv[0],VersionStr);
   printf("Testing SND_TONE_START CR169900 \n");
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

   snd_device_type       device = SND_DEVICE_HANDSET;

   /* The device chosen for this command   */
   snd_mute_control_type ear_mute = SND_MUTE_UNMUTED;
   /* Mute, unmute output                  */
   snd_mute_control_type mic_mute = SND_MUTE_UNMUTED;
   /* Mute, unmute microphone              */

   /* Call back function                   */
   snd_set_device(device, ear_mute, mic_mute, snd_callback, 0);
   sleep(1);
   if(nunTestsToRun == 0)
   {
      while(1)
      {
         printf("SND Tone Start  %d \n",cnt);
         cnt++;
         snd_tone_start (SND_DEVICE_HANDSET, SND_METHOD_KEY_BEEP, 0,5, SND_APATH_LOCAL,NULL,NULL);

      }
   } else
   {
      for(cnt=0;cnt < nunTestsToRun;cnt++)
      {
         printf("SND Tone Start  %d \n",cnt);
         snd_tone_start (SND_DEVICE_HANDSET, SND_METHOD_KEY_BEEP, 0,5, SND_APATH_LOCAL,NULL,NULL);
      }
   }

   printf("%s COMPLETE...\n",argv[0]);
   printf("PASS\n");
   return(0);
}


