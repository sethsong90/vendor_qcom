/*===========================================================================
Diag Legacy Service Mapping Layer Implementation for Debug Message
(F3 Message) Service, and Optimized Debug Message Service
(Also known as QSHRINK Message)

GENERAL DESCRIPTION
   API definitons for Debug Message Service Mapping Layer.

EXTERNALIZED FUNCTIONS
Note: These functions should not be used directly, use the MSG_* macros instead.
   msg_send
   msg_send_1
   msg_send_2
   msg_send_3
   msg_send_var
   msg_sprintf
Note: These functions or the relevant macros (QSR_MSG*) should not be called directly.
	  	MSG* macros are converted to QSR_MSG* with a text replacement before build.
	  	qsr_msg_send
	  	qsr_msg_send_1
	  	qsr_msg_send_2
	  	qsr_msg_send_3
	  	qsr_msg_send_var


INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2007-2011, 2013 Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential
===========================================================================*/

/*===========================================================================
                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header:

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/01/08   SJ	   Changes for CBSP 2.0
05/01/08   JV      Added support to update the copy of run-time masks in the
                   msg_mask_tbl in this process during initialization and also
				   on mask change
11/29/07   mad     Created File
===========================================================================*/

#define MSG_TBL_GEN

#if defined (FEATURE_WINCE)
#include <windows.h>
#endif /* FEATURE_WINCE */
#include "diagsvc_malloc.h"
#include "diag_lsm_msg_i.h"
#include "msg.h"
#include "msg_qsr.h"
#include "msgcfg.h"
#include "diag_lsmi.h"
#include "diag_lsm.h"
#include "diagcmd.h"
#include "diag_shared_i.h" /* for definition of diag_data struct, and diag datatypes. */
#include "msgi.h"
#include "msg_pkt_defs.h"
#include "msg_arrays_i.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include "errno.h"
#include "stdio.h"
#include "stringl.h"
#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>
#include "ts_linux.h"
#include <string.h>
#include <stdint.h>


/* internal datatypes and defines */
typedef struct{
  uint32 args[10];
}msg_large_args;
#define MSG_LARGE_ARGS(X) (((msg_large_args *)X)->args)
#define MSG_WCDMA_TIME_FORMAT 2
#define MSG_CDMA_TIME_FORMAT  3

/* Define the symbol that tells this module which time format to use. */
#if defined(FEATURE_WCDMA)
#define MSG_TIME_FORMAT MSG_WCDMA_TIME_FORMAT
#else
#define MSG_TIME_FORMAT MSG_CDMA_TIME_FORMAT
#endif

#if (MSG_TIME_FORMAT == MSG_WCDMA_TIME_FORMAT)
#ifndef FEATURE_WINCE
#include "seq.h"
extern boolean l1m_is_connected (void);
#endif

/* Number of bits in the frame number that is inserted into the timestamp,
and the mask to extract these bits. */
#define MSG_FN_NUM_BITS 10
#define MSG_FN_MASK 0x3ff
#endif

typedef enum
{
  MSG_MASK_BT,          /* Build-time mask */
  MSG_MASK_RT           /* Run-time mask */
} msg_mask_type;

/* TODO: These statistics needs to be shared across processes */
/* Statistics */
//static uint32 msg_total;    /* total number of messages logged */
static uint32 msg_dropped;  /* number of dropped messages */
static uint32 msg_drop_delta;   /* number of dropped messages */

/* Message operating mode.  If a legacy message request packet is used, the message service will build legacy packet formats for MSG_SSID_LEGACY and ignore all other SSIDs. */

/*static enum
{
  MSG_MODE_STD,         // Standard operating mode
  MSG_MODE_TERSE,       // Terse mode - send the store packet as is
                   //(let the tool expand the pointers).
  MSG_MODE_LEGACY       // Legacy packet mode
} msg_mode;
*/
#ifndef MSG_FMT_STR_ARG_SIZE
#define MSG_FMT_STR_ARG_SIZE 280 /* 280 is guess, close enough to accomodate QCRIL messages upto 252 bytes long */
#endif

/* Internal function declarations */
#ifndef MSM5000_IRAM_FWD
byte *msg_send_prep
 (const msg_const_type *const_blk, unsigned int num_args,
   unsigned int *pLength, uint64 timestamp, boolean ts_valid);
//static void msg_get_time(qword * ts);
static byte * msg_sprintf_prep (const msg_const_type * const_blk, unsigned int num_args, unsigned int* pLength);
static byte* qsr_msg_send_prep(const msg_qsr_const_type* const_blk, unsigned int num_args, unsigned int* pLength);
#endif
boolean msg_get_ssid_rt_mask(int ssid, uint32* mask);
//static boolean msg_get_ssid_masks (msg_mask_type mask_type, uint16 ssid_start,
  //                          uint16 ssid_end, uint32 * mask);
static const char *msg_format_filename2 (const char *filename);

/* this keeps track of number of failures to IDiagPkt_Send().
This will currently be used only internally. */
static unsigned int gMsg_commit_to_cs_fail = 0;
/* this keeps track of number of failures to WriteFile().
This will currently be used only internally. */
static unsigned int gMsg_commit_fail;

/* use this global buffer to read message mask in, from the kernel-mode
driver.Avoids a malloc everytime the mask needs updation. */
byte* gMsg_Mask_Read_Buf = NULL;
uint32 gMsg_Mask_Size = 0; /* size of gMsg_Mask_Read_Buf */
int gnDiag_LSM_Msg_Initialized = 0;
/*===========================================================================
FUNCTION msg_update_mask

DESCRIPTION
   This function sends updates the data structure for msg masks.

DEPENDENCIES
  None
===========================================================================*/
void msg_update_mask(unsigned char* ptr, int num_bytes_read)
{
	int k = 0;
	int bytes_to_read = num_bytes_read;
	if (num_bytes_read > MSG_MASK_SIZE)
		bytes_to_read = MSG_MASK_SIZE;

	for (k = 0; k < bytes_to_read; k++) {
		read_mask[k] = *ptr;
		ptr++;
	}
}
/*---------------------------------------------------------------------------------------------------
                                    Externalised functions
(Do not call any of these functions directly, use the Macros defined in msg.h instead.)
---------------------------------------------------------------------------------------------------*/


/*===========================================================================
FUNCTION MSG_SEND

DESCRIPTION
   This function sends out a debug message with no arguments across DiagPkt CS interface.
   Do not call directly; use macro MSG_* defined in msg.h

DEPENDENCIES
   DiagPkt handle should be initialised.
===========================================================================*/
void
msg_send (const msg_const_type * const_blk)
{
//    if(gpiDiagPkt)
	//removed WM specific code
	#ifdef FEATURE_WINMOB
    if(ghWinDiag)
	#else
	if(fd != -1)
	#endif
    {
       //msg_ext_type *pMsg = NULL;
       byte* pMsg = NULL;
       const unsigned int num_args = 0;  /* # of message arguments */
       unsigned int nLength = 0;
       pMsg = msg_send_prep(const_blk, num_args, &nLength, 0, FALSE);
       if (pMsg)
       {
		   //removed WM specific code
		   #ifdef FEATURE_WINMOB
           DWORD NumberOfBytesWritten = 0;
           if(!WriteFile(ghWinDiag, (LPCVOID) pMsg, nLength, &NumberOfBytesWritten, NULL)) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
		   #else
	   int NumberOfBytesWritten = 0;
	   if((NumberOfBytesWritten = write(fd, (const void*) pMsg, nLength)) != 0) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
		   #endif
	   {
			DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
				   __func__, NumberOfBytesWritten, errno);
			gMsg_commit_to_cs_fail++;
	  }
	  DiagSvc_Free(pMsg, GEN_SVC_ID);
       }
    }
} /* msg_send() */

/*===========================================================================
FUNCTION MSG_SEND_TS

DESCRIPTION
   This function sends out a debug message with no arguments, and uses
    timestamp passed in by client. Do not call directly; use macro MSG_*
	 defined in msg.h

DEPENDENCIES
   windiag driver handle should be initialised.
===========================================================================*/
void
msg_send_ts(const msg_const_type *const_blk, uint64 timestamp)
{
	if (fd != -1) {
		byte *pMsg = NULL;
		const unsigned int num_args = 0;  /* # of message arguments */
		unsigned int nLength = 0;
		int ret;
		pMsg = msg_send_prep(const_blk, num_args, &nLength, timestamp,
				      TRUE);
		if (pMsg) {
			if ((ret = write(fd, (const void *) pMsg, nLength)) != 0) {
				DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
					__func__, ret, errno);
				gMsg_commit_fail++;
			}
			DiagSvc_Free(pMsg, GEN_SVC_ID);
		}
	}
} /* msg_send() */

/*===========================================================================
FUNCTION MSG_SEND_1

DESCRIPTION
   This function sends out a debug message with 1 argument across DiagPkt CS interface.
   Do not call directly; use macro MSG_* defined in msg.h

DEPENDENCIES
   DiagPkt handle should be initialised.
===========================================================================*/
void
msg_send_1 (const msg_const_type *pconst_blk, uint32 xx_arg1)
{
//    if(gpiDiagPkt)

	//removed WM specific code
	#ifdef FEATURE_WINMOB
   if(ghWinDiag)
  #else
	if(fd != -1)
	#endif
   {
      //msg_ext_type *pMsg = NULL;
      byte *pMsg = NULL;
	  //int i;
      unsigned int nLength = 0;
      const unsigned int num_args = 1;  /* # of message arguments */

       pMsg = msg_send_prep(pconst_blk, num_args, &nLength, 0, FALSE);
       if (pMsg)
       {
          msg_ext_type *pTemp = (msg_ext_type*)(pMsg+DIAG_REST_OF_DATA_POS);
//          DWORD NumberOfBytesWritten = 0;
          /* Store the arguments in the buffer. */
          //args = pMsg->args;
        uint32 *args = pTemp->args;
          args[0] = xx_arg1;
//removed WM specific code
		  #ifdef FEATURE_WINMOB
          if(!WriteFile(ghWinDiag, (LPCVOID) pMsg, nLength, &NumberOfBytesWritten, NULL)) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
		  #else
	  int NumberOfBytesWritten;
	  if((NumberOfBytesWritten = write(fd, (const void*) pMsg, nLength)) != 0) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
		  #endif
	  {
		DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
				  __func__, NumberOfBytesWritten, errno);
		gMsg_commit_to_cs_fail++;
          }
          DiagSvc_Free(pMsg, GEN_SVC_ID);
       }
    }
} /* void msg_send_1() */

/*===========================================================================
FUNCTION MSG_SEND_2

DESCRIPTION
   This function sends out a debug message with 2 arguments across DiagPkt CS interface.
   Do not call directly; use macro MSG_* defined in msg.h

DEPENDENCIES
   DiagPkt handle should be initialised.
===========================================================================*/
void
msg_send_2 (const msg_const_type *pconst_blk, uint32 xx_arg1, uint32 xx_arg2)
{
    //if(gpiDiagPkt)
	//removed WM specific code
	#ifdef FEATURE_WINMOB
    if(ghWinDiag)
	#else
	if(fd != -1)
	#endif
    {
       //msg_ext_type *pMsg = NULL;
      byte *pMsg = NULL;

       unsigned int nLength = 0;
       const unsigned int num_args = 2;  /* # of message arguments */

       pMsg = msg_send_prep(pconst_blk, num_args, &nLength, 0, FALSE);

       if (pMsg)
       {
          msg_ext_type *pTemp = (msg_ext_type*)(pMsg+DIAG_REST_OF_DATA_POS);
          /* Store the arguments in the buffer. */
          uint32 *args = MSG_LARGE_ARGS(pTemp->args);
          args[0] = xx_arg1;
          args[1] = xx_arg2;
		  //removed WM specific code
		  #ifdef FEATURE_WINMOB
		  DWORD NumberOfBytesWritten = 0;
          if(!WriteFile(ghWinDiag, (LPCVOID) pMsg, nLength, &NumberOfBytesWritten, NULL)) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
		  #else
	  int NumberOfBytesWritten = 0;
	  if((NumberOfBytesWritten = write(fd, (const void*) pMsg, nLength)) != 0) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
		  #endif
	  {
		DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
				  __func__, NumberOfBytesWritten, errno);
		gMsg_commit_to_cs_fail++;
          }
          DiagSvc_Free(pMsg, GEN_SVC_ID);
       }
    }
}               /* msg_send_2() */

/*===========================================================================
FUNCTION MSG_SEND_3

DESCRIPTION
This function sends out a debug message with 2 arguments across DiagPkt CS interface.
Do not call directly; use macro MSG_* defined in msg.h

DEPENDENCIES
DiagPkt handle should be initialised.
===========================================================================*/
#ifndef MSM5000_IRAM_FWD
void
msg_send_3 (const msg_const_type * pconst_blk, uint32 xx_arg1,
        uint32 xx_arg2, uint32 xx_arg3)
{
    //if(gpiDiagPkt)
	//removed WM specific code
	#ifdef FEATURE_WINMOB
   if(ghWinDiag)
	#else
	if(fd != -1)
	#endif
    {
       //msg_ext_type *pMsg = NULL;
      byte *pMsg = NULL;
       unsigned int nLength = 0;
       const unsigned int num_args = 3;  /* # of message arguments */

       pMsg = msg_send_prep(pconst_blk, num_args, &nLength, 0, FALSE);
       if (pMsg)
       {
          uint32 *args = NULL;

          msg_ext_type *pTemp = (msg_ext_type*)(pMsg+DIAG_REST_OF_DATA_POS);
           /* Store the arguments in the buffer. */
          args = MSG_LARGE_ARGS(pTemp->args);
          args[0] = xx_arg1;
          args[1] = xx_arg2;
          args[2] = xx_arg3;

		  //removed WM specific code
	#ifdef FEATURE_WINMOB
	  DWORD NumberOfBytesWritten = 0;
	  if(!WriteFile(ghWinDiag, (LPCVOID) pMsg, nLength, &NumberOfBytesWritten, NULL)) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
	#else
	  int NumberOfBytesWritten = 0;
	  if((NumberOfBytesWritten = write(fd, (const void*) pMsg, nLength)) != 0) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
	#endif
          {
		DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
			  __func__, NumberOfBytesWritten, errno);
		gMsg_commit_to_cs_fail++;
          }
          DiagSvc_Free(pMsg, GEN_SVC_ID);
       }
    }
} /* msg_send_3() */
#endif /* MSM5000_IRAM_FWD */

/*===========================================================================
FUNCTION MSG_SEND_VAR

DESCRIPTION
   This function sends out a debug message with variable number of arguments
   across DiagPkt CS interface.
   Do not call directly; use macro MSG_* defined in msg.h

DEPENDENCIES
   DiagPkt handle should be initialised.

===========================================================================*/
void
msg_send_var (const msg_const_type *pconst_blk, uint32 num_args, ...)
{
    //if(gpiDiagPkt)
	//removed WM specific code
	#ifdef FEATURE_WINMOB
	if(ghWinDiag)
	#else
	if(fd != -1)
	#endif
    {
       byte *pMsg = NULL;
       unsigned int nLength = 0;


       pMsg = msg_send_prep(pconst_blk, num_args, &nLength, 0, FALSE);
       if (pMsg)
       {
  //        DWORD NumberOfBytesWritten = 0;
          va_list arg_list;     /* ptr to the variable argument list */
          unsigned int i;
          msg_ext_type *pTemp = (msg_ext_type*)(pMsg+DIAG_REST_OF_DATA_POS);
          /* Store the arguments in the buffer. */
          uint32 *args = MSG_LARGE_ARGS(pTemp->args);
          /* Initialize variable arguments */
          va_start (arg_list, num_args);
          /* Store arguments from variable list. */
          for (i = 0; i < num_args; i++)
          {
             args[i] = va_arg (arg_list, uint32);
          }
          /* Reset variable arguments */
          va_end (arg_list);
		  //removed WM specific code
	#ifdef FEATURE_WINMOB
          if(!WriteFile(ghWinDiag, (LPCVOID) pMsg, nLength, &NumberOfBytesWritten, NULL)) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
	#else
	  int NumberOfBytesWritten;
	  if((NumberOfBytesWritten = write(fd, (const void*) pMsg, nLength)) != 0) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
	#endif
	  {
		DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
				  __func__, NumberOfBytesWritten, errno);
		gMsg_commit_to_cs_fail++;
          }
          DiagSvc_Free(pMsg, GEN_SVC_ID);
       }
    }
} /* msg_send_var() */

/*===========================================================================
FUNCTION MSG_SPRINTF

DESCRIPTION

   This function sends out a debug message with variable number of arguments
   across DiagPkt CS interface.
   This will build a message sprintf diagnostic Message with var #
   of parameters.
   Do not call directly; use macro MSG_SPRINTF_* defined in msg.h

DEPENDENCIES
   DiagPkt handle should be initialised.
===========================================================================*/
void
msg_sprintf (const msg_const_type *const_blk, ...)
{

#ifdef FEATURE_LOG_STDOUT
   // cheap replacemnet - log to stdout
   va_list arg_list;
   va_start(arg_list, const_blk);
   char str[256];
   (void) vsnprintf(str, sizeof(str), const_blk->fmt, arg_list);
   va_end(arg_list);

   int last_char_idx = strlen(str) - 1;
   if (last_char_idx >= 0) {
      // remove extra \n if present
      if ('\n' == str[last_char_idx] ) {
         str[last_char_idx] = '\0';
      }
   }
   printf( "%s [%d]: %s\n", const_blk->fname, const_blk->desc.line, str);
   fflush(stdout); // don't buffer output

#else
	//removed WM specific code
	#ifdef FEATURE_WINMOB
	 if(const_blk && ghWinDiag)
    #else
	if(const_blk && (fd != -1))
	#endif
	{
 		//if(gpiDiagPkt)
 	   //{
 		   /* Declared union for future use */
 		  typedef union
 		  {
 			msg_ext_type ext;
 		  } msg_sprintf_desc_type;
 		  //msg_sprintf_desc_type *pMsg = NULL;
        byte *pMsg = NULL;
 		  unsigned int int_cnt = 0;           /* Calculate the # args, to allocate buffer */
 		  unsigned int fname_length = 0;      /* Stores the file name along with '\0'     */
 		  unsigned int fmt_length = 0;        /* Stores the fmt length,'\0' and arg size  */
 		  unsigned int total_allocated = 0;   /* Total buffer allocated                   */
 		  const char* abb_filename = NULL;

 		  /* Point to note: In AMSS, two files cannot have the same name,
 		  so the whole file path is not displayed in QXDM. In WM, that need not be the case?
 		   For now, get the file name only, as done in legacy diag. */
 		  abb_filename = msg_format_filename2(const_blk->fname);
 		  fname_length = strlen (abb_filename) + 1;

 		  fmt_length = strlen (const_blk->fmt) + 1 + MSG_FMT_STR_ARG_SIZE;
 		  /* Calculate # of arguments to ensure enough space is allocated. */
 		  int_cnt = sizeof (msg_desc_type) - FSIZ (msg_ext_store_type, const_data_ptr) +
 					fmt_length + fname_length;
 		  /* Calculates number of uint32s required */
 		  int_cnt = (int_cnt + sizeof (uint32) - 1) / sizeof (uint32);
 		  /*  Allocates the buffer required, fills in the header  */
 		  pMsg = msg_sprintf_prep (const_blk, int_cnt, &total_allocated);
 		  if (pMsg)
 		  {
           msg_sprintf_desc_type *pTemp = (msg_sprintf_desc_type*)(pMsg+DIAG_REST_OF_DATA_POS);
 			 char *str = NULL; /* Used to copy the file name and fmt string to the msg */
 			 va_list arg_list;   /* ptr to the variable argument list */
 			 unsigned int fmt_len_available = 0; /* Remaining buffer for format string       */

 			/* Queue a debug message in Extended Message Format. */
 			pTemp->ext.hdr.cmd_code = DIAG_EXT_MSG_F;
 		   /* This function embedds the argument in the string itself.
 			** Hence the num_args is assigned 0 */
 			pTemp->ext.hdr.num_args = 0;
 			/* Copy 'desc'. */
 			pTemp->ext.desc = const_blk->desc;
 		   /* Copy the format string where the argument list would start.
 			   Since there are no arguments, the format string starts in the 'args'
 			   field. */
 			str = (char *) pTemp->ext.args;
 			/* Calculate the buffer left to copy the format string */
 			fmt_len_available = total_allocated - (FPOS (msg_ext_type, args) + fname_length);
 			if( fmt_len_available < fmt_length)
 			{
 			   fmt_length = fmt_len_available;
 			}
 			/* Initialize variable argument list */
 			va_start(arg_list, const_blk);
 			/* Copy the format string with arguments */
		#ifdef FEATURE_WINMOB
         (void) _vsnprintf_s(str, fmt_length, _TRUNCATE, const_blk->fmt, arg_list);
			#else
		 (void) vsnprintf(str, fmt_length, const_blk->fmt, arg_list);
		 #endif
         //
 			str[fmt_length-1] = '\0';
 			/* Reset variable arguments */
 			va_end(arg_list);
 			/* Move the str pass the format string, strlen excludes the terminal
 			** NULL hence 1 is added to include NULL. */
 			str += strlen((const char *)str) + 1;
 			/* Copy the filename */
			#ifdef FEATURE_WINMOB
			std_strlprintf(str, fname_length, "%s", abb_filename);
			#else
			snprintf(str, fname_length, "%s", abb_filename);
			#endif
 			/* Move the str pass the filename, strlen excludes the terminal NULL
 			** hence 1 is added to include NULL. */
 			str += strlen((const char *)str) + 1;
 			/* str is now pointing to the byte after the last valid byte. str - msg
 			 gives the total length required. */
 			//removed WM specific code
			#ifdef FEATURE_WINMOB
			DWORD NumberOfBytesWritten = 0;
			if(!WriteFile(ghWinDiag, (LPCVOID) pMsg, (uint32)(str - (char *)pMsg), &NumberOfBytesWritten, NULL)) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
			#else
			int NumberOfBytesWritten = 0;
			if((NumberOfBytesWritten = write(fd, (const void*) pMsg, (uint32)(str - (char *)pMsg))) != 0) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
			#endif
 			{
				DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
					__func__, NumberOfBytesWritten, errno);
 				gMsg_commit_to_cs_fail++;
 			}
 			DiagSvc_Free(pMsg, GEN_SVC_ID);
 		 }/* if (pMsg) */
 	   //}/* if(gpiDiagPkt) */
	} /* if(const_blk && ghWinDiag ) */
#endif //FEATURE_LOG_STDOUT
    return;
 }  /* msg_sprintf */
/*===========================================================================
FUNCTION qsr_msg_send

DESCRIPTION
   This function sends out a debug message with no arguments.
   This function, or the macro QSR_MSG should not be called directly,
   MSG macros are converted to QSR_MSG macro by text-replacement.

DEPENDENCIES
   windiag driver should be initialized with a Diag_LSM_Init() call.
===========================================================================*/
void qsr_msg_send ( const msg_qsr_const_type * const_blk )
{
	#ifdef FEATURE_WINMOB
	 if(ghWinDiag)
    #else
	if(fd != -1)
	#endif
    {
       byte* pMsg = NULL;
       const unsigned int num_args = 0;  /* # of message arguments */
       unsigned int nLength = 0;
       pMsg = qsr_msg_send_prep (const_blk, num_args, &nLength);
       if (pMsg)
       {
		#ifdef FEATURE_WINMOB
          DWORD NumberOfBytesWritten = 0;
		#else
		  int NumberOfBytesWritten = 0;
		#endif

        #ifdef FEATURE_WINMOB
          if(
             !WriteFile(ghWinDiag, (LPCVOID) pMsg, nLength, &NumberOfBytesWritten, NULL) ||
             (NumberOfBytesWritten != nLength - DIAG_REST_OF_DATA_POS)
            )
		#else
	  if((NumberOfBytesWritten = write(fd, (const void*) pMsg, nLength)) != 0)
		#endif
          {
		DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
			  __func__, NumberOfBytesWritten, errno);
		gMsg_commit_fail++;
          }
          DiagSvc_Free(pMsg, GEN_SVC_ID);
       }
    }
}/* qsr_msg_send */

/*===========================================================================
FUNCTION qsr_msg_send_1

DESCRIPTION
   This function sends out a debug message with no arguments.
   This function, or the macro QSR_MSG_1 should not be called directly,
   MSG* macros are converted to QSR_MSG* macros by text-replacement.

DEPENDENCIES
   windiag driver should be initialized with a Diag_LSM_Init() call.
===========================================================================*/
void qsr_msg_send_1 (const msg_qsr_const_type * const_blk, uint32 xx_arg1)
{
   	#ifdef FEATURE_WINMOB
	 if(ghWinDiag)
    #else
	if(fd != -1)
	#endif
    {
       byte* pMsg = NULL;
       const unsigned int num_args = 1;  /* # of message arguments */
       unsigned int nLength = 0;
       pMsg = qsr_msg_send_prep (const_blk, num_args, &nLength);
       if (pMsg)
       {
		#ifdef FEATURE_WINMOB
          DWORD NumberOfBytesWritten = 0;
		#else
		  int NumberOfBytesWritten = 0;
		#endif
          msg_qsr_type *pTemp = (msg_qsr_type*)(pMsg+DIAG_REST_OF_DATA_POS);
          /* Store the arguments in the buffer. */
          pTemp->args[0] = xx_arg1;

        #ifdef FEATURE_WINMOB
		  if(
			 !WriteFile(ghWinDiag, (LPCVOID) pMsg, nLength, &NumberOfBytesWritten, NULL) ||
			 (NumberOfBytesWritten != nLength - DIAG_REST_OF_DATA_POS)
			)
		#else
	  if((NumberOfBytesWritten = write(fd, (const void*) pMsg, nLength)) != 0)
		#endif
          {
		DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
			  __func__, NumberOfBytesWritten, errno);
		gMsg_commit_fail++;
          }
          DiagSvc_Free(pMsg, GEN_SVC_ID);
       }
    }
}/* qsr_msg_send_1 */

/*===========================================================================
FUNCTION qsr_msg_send_2

DESCRIPTION
   This function sends out a debug message with no arguments.
   This function, or the macro QSR_MSG_2 should not be called directly,
   MSG* macros are converted to QSR_MSG* macros by text-replacement.

DEPENDENCIES
   windiag driver should be initialized with a Diag_LSM_Init() call.
===========================================================================*/

void qsr_msg_send_2 (const msg_qsr_const_type * const_blk, uint32 xx_arg1, uint32 xx_arg2)
{
   	#ifdef FEATURE_WINMOB
	 if(ghWinDiag)
    #else
	if(fd != -1)
	#endif
    {
       byte* pMsg = NULL;
       const unsigned int num_args = 2;  /* # of message arguments */
       unsigned int nLength = 0;
       pMsg = qsr_msg_send_prep (const_blk, num_args, &nLength);
       if (pMsg)
       {
		  #ifdef FEATURE_WINMOB
			   DWORD NumberOfBytesWritten = 0;
		  #else
			   int NumberOfBytesWritten = 0;
		  #endif
          msg_qsr_type *pTemp = (msg_qsr_type*)(pMsg+DIAG_REST_OF_DATA_POS);
          /* Store the arguments in the buffer. */
          pTemp->args[0] = xx_arg1;
          pTemp->args[1] = xx_arg2;

		  #ifdef FEATURE_WINMOB
          if(
             !WriteFile(ghWinDiag, (LPCVOID) pMsg, nLength, &NumberOfBytesWritten, NULL) ||
             (NumberOfBytesWritten != nLength - DIAG_REST_OF_DATA_POS)
            )
		  #else
	  if((NumberOfBytesWritten = write(fd, (const void*) pMsg, nLength)) != 0)
		  #endif
          {
		DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
			  __func__, NumberOfBytesWritten, errno);
		gMsg_commit_fail++;
          }
          DiagSvc_Free(pMsg, GEN_SVC_ID);
       }
    }
}/* qsr_msg_send_2 */

/*===========================================================================
FUNCTION qsr_msg_send_3

DESCRIPTION
   This function sends out a debug message with no arguments.
   This function, or the macro QSR_MSG_3 should not be called directly,
   MSG* macros are converted to QSR_MSG* macros by text-replacement.

DEPENDENCIES
   windiag driver should be initialized with a Diag_LSM_Init() call.
===========================================================================*/

void qsr_msg_send_3 (const msg_qsr_const_type * const_blk, uint32 xx_arg1, uint32 xx_arg2, uint32 xx_arg3)
{
   	#ifdef FEATURE_WINMOB
	 if(ghWinDiag)
    #else
	if(fd != -1)
	#endif
    {
       byte* pMsg = NULL;
       const unsigned int num_args = 3;  /* # of message arguments */
       unsigned int nLength = 0;
       pMsg = qsr_msg_send_prep (const_blk, num_args, &nLength);
       if (pMsg)
       {
        #ifdef FEATURE_WINMOB
          DWORD NumberOfBytesWritten = 0;
		#else
		  int NumberOfBytesWritten = 0;
		#endif

          msg_qsr_type *pTemp = (msg_qsr_type*)(pMsg+DIAG_REST_OF_DATA_POS);
          /* Store the arguments in the buffer. */
          pTemp->args[0] = xx_arg1;
          pTemp->args[1] = xx_arg2;
          pTemp->args[2] = xx_arg3;

		  #ifdef FEATURE_WINMOB
          if(
             !WriteFile(ghWinDiag, (LPCVOID) pMsg, nLength, &NumberOfBytesWritten, NULL) ||
             (NumberOfBytesWritten != nLength - DIAG_REST_OF_DATA_POS)
            )
		  #else
	  if((NumberOfBytesWritten = write(fd, (const void*) pMsg, nLength)) != 0)
		  #endif
          {
		DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
			  __func__, NumberOfBytesWritten, errno);
		gMsg_commit_fail++;
          }
          DiagSvc_Free(pMsg, GEN_SVC_ID);
       }
    }
}/* qsr_msg_send_3 */

/*===========================================================================
FUNCTION qsr_msg_send_var

DESCRIPTION
   This function sends out a debug message with no arguments.
   This function, or the macro QSR_MSG_* should not be called directly,
   MSG* macros are converted to QSR_MSG* macros by text-replacement.

DEPENDENCIES
   windiag driver should be initialized with a Diag_LSM_Init() call.
===========================================================================*/
void qsr_msg_send_var ( const msg_qsr_const_type * const_blk, uint32 num_args, ...)
{
   	#ifdef FEATURE_WINMOB
	 if(ghWinDiag)
    #else
	if(fd != -1)
	#endif
    {
       byte* pMsg = NULL;
       unsigned int nLength = 0;
       pMsg = qsr_msg_send_prep (const_blk, num_args, &nLength);
       if (pMsg)
       {
        #ifdef FEATURE_WINMOB
          DWORD NumberOfBytesWritten = 0;
		#else
		  int NumberOfBytesWritten = 0;
		#endif

          msg_qsr_type *pTemp = (msg_qsr_type*)(pMsg+DIAG_REST_OF_DATA_POS);
          va_list arg_list;     /* ptr to the variable argument list */
          unsigned int i;
          uint32 *args = MSG_LARGE_ARGS(pTemp->args);

          /* Initialize variable arguments */
          va_start (arg_list, num_args);
          /* Store arguments from variable list. */
          for (i = 0; i < num_args; i++)
          {
             args[i] = va_arg (arg_list, uint32);
          }
          /* Reset variable arguments */
          va_end (arg_list);

		  #ifdef FEATURE_WINMOB
          if(
             !WriteFile(ghWinDiag, (LPCVOID) pMsg, nLength, &NumberOfBytesWritten, NULL) ||
             (NumberOfBytesWritten != nLength - DIAG_REST_OF_DATA_POS)
            )
		  #else
	  if((NumberOfBytesWritten = write(fd, (const void*) pMsg, nLength)) != 0)
		  #endif
          {
		DIAG_LOGE("Diag_LSM_Msg: Write failed in %s, bytes written: %d, error: %d\n",
			  __func__, NumberOfBytesWritten, errno);
		gMsg_commit_fail++;
          }
          DiagSvc_Free(pMsg, GEN_SVC_ID);
       }
    }
} /* qsr_msg_send_var */

/*----------------------------------------------------------------------------
                                          Internal functions
-----------------------------------------------------------------------------*/
static uint32 Diag_LSM_Msg_ComputeMaskSize(void)
{
   uint32 msg_mask_size = 0;
   int i = 0;
   for(i = 0; i < MSG_MASK_TBL_CNT; i++)
   {
      msg_mask_size += sizeof(uint16) + sizeof(uint16);
      msg_mask_size += (msg_mask_tbl[i].ssid_last - msg_mask_tbl[i].ssid_first + 1)*sizeof(uint32);
   }
   return msg_mask_size;
}

/*===========================================================================
FUNCTION Diag_LSM_Msg_Init

DESCRIPTION
Initializes the Diag Message service mapping layer.

DEPENDENCIES
None

===========================================================================*/
boolean Diag_LSM_Msg_Init (void)
{
   boolean status = TRUE;
   if(!gnDiag_LSM_Msg_Initialized)
   {
      gMsg_Mask_Size = Diag_LSM_Msg_ComputeMaskSize();
      if(!gMsg_Mask_Read_Buf)
      {
         gMsg_Mask_Read_Buf = malloc(gMsg_Mask_Size);
         if(!gMsg_Mask_Read_Buf)
         {
            status = FALSE;
         }
      }
      gnDiag_LSM_Msg_Initialized = 1;
   }
   return status;
} /* Diag_LSM_Msg_Init() */

/*===========================================================================
FUNCTION Diag_LSM_Msg_DeInit

DESCRIPTION
Prepares mapping layer exit for Diag message service.
Currently does nothing, just returns TRUE.
This is an internal function, to be used only by Diag_LSM module.

DEPENDENCIES
None.


===========================================================================*/
boolean Diag_LSM_Msg_DeInit(void)
{
   boolean success = TRUE;
//    if(msg_mask)
//    {
//       success = UnmapViewOfFile((LPCVOID)msg_mask);
//    }
//    /* WM7: Close the HANDLE returned by CreateFileMapping(), if this is running in kernel. */
//    /* For any user-space app that loads/unloads dynamically, this handle shouldn't be closed,
//    because kernel might still be using the handle. */
//    if(ghMsg_Shared_Mask)
//    {
//       success = CloseHandle(ghMsg_Shared_Mask);
//    }
   if(gMsg_Mask_Read_Buf)
   {
      free(gMsg_Mask_Read_Buf);
      gMsg_Mask_Read_Buf = NULL;
   }
    return success;
} /* boolean Diag_LSM_Msg_DeInit() */

boolean msg_get_ssid_rt_mask(int ssid, uint32* mask)
{
		boolean success = FALSE;
		int first;
		int last;
		unsigned char *ptr = read_mask;

		while (*(uint32_t *)(ptr + 4))
		{
			first = *(uint32_t *)ptr;
			ptr +=8;
			last = *(uint32_t *)ptr;
			ptr+=4;

			if(first <= ssid && ssid <= last)
			{
					success = TRUE;
					break;
			}
			else
			{
				//advance pointer by the number of bytes
				ptr = ptr + MAX_SSID_PER_RANGE * 4;
			}
		}
		if(success == TRUE)
		{
			//printf("\n MASK : %x\n", *((uint32_t *)(ptr+(ssid-first)*4)));
			*mask = *((uint32_t *)(ptr+(ssid-first)*4));
		}
		return success;
}

/*===========================================================================
FUNCTION msg_format_filename2

DESCRIPTION
retrieves the position of filename from full file path.

DEPENDENCIES
None.
===========================================================================*/
static const char *
msg_format_filename2 (const char *filename)
{
  const char *p_front = filename;
  const char *p_end = filename + strlen (filename);

  while (p_end != p_front)
  {
    if ((*p_end == '\\') || (*p_end == ':') || (*p_end == '/'))
    {
      p_end++;
      break;
    }
    p_end--;
  }
  return p_end;
} /*const char *msg_format_filename2() */

#ifndef MSM5000_IRAM_FWD
/*===========================================================================

FUNCTION MSG_SEND_PREP

DESCRIPTION
   Prepares the buffer needed by msg_send*().
   Allocates, fills in all data except arguments, and returns a pointer
   to the allocated message buffer.  It also handles message statisitics.

RETURN VALUE
   Returns the allocated buffer, and the length of the buffer

DEPENDENCIES
   None
===========================================================================*/

byte *msg_send_prep(const msg_const_type *const_blk, unsigned int num_args,
					  unsigned int *pLength,
		      uint64 timestamp, boolean ts_valid)
{
  uint32 rt_mask;
  boolean valid_ssid = FALSE;
  byte *pMsg = NULL;

  if (pLength)
	  *pLength = 0;

  /* Check the runtime mask */
 valid_ssid = msg_get_ssid_rt_mask( (int)const_blk->desc.ss_id, &rt_mask);

 if (valid_ssid && (const_blk->desc.ss_mask & rt_mask)) {
    const char* abb_filename = NULL;
    unsigned int alloc_len = 0;
    abb_filename = msg_format_filename2(const_blk->fname);

    /* total number of bytes to be allocated, including dereferenced FileName and Format strings */
    alloc_len =            DIAG_REST_OF_DATA_POS + /* WM7: For windiag driver to recognize that this is an F3 Msg. */
                           FPOS (msg_ext_type, args) +
                           num_args * FSIZ (msg_ext_type,args[0]) +
                           strlen(abb_filename) + 1 +
                           strlen(const_blk->fmt) + 1;

    /* Get a pointer to a buffer.  If it's a NULL pointer, there is no memory in the client heap. */
    //pMsg = (msg_ext_type *) DiagSvc_Malloc(alloc_len, GEN_SVC_ID);

    pMsg = (byte *) DiagSvc_Malloc(alloc_len, GEN_SVC_ID);

    if (pMsg)
    {
       /* position of format string in the returned buffer. */
       //uint32 fmt_pos = FPOS (msg_ext_type, args) + num_args * FSIZ (msg_ext_type,args[0]);
       msg_ext_type* pTemp = (msg_ext_type*)((byte*)pMsg + DIAG_REST_OF_DATA_POS);
       uint32 fmt_pos = DIAG_REST_OF_DATA_POS +
                        FPOS (msg_ext_type, args) +
                        num_args * FSIZ (msg_ext_type,args[0]);
       diag_data* pdiag_data = (diag_data*) pMsg;
       //Prototyping Diag 1.5 WM7:Fill in the fact that this is an F3 Message.
       pdiag_data->diag_data_type = DIAG_DATA_TYPE_F3;
       if(pLength)
       {
          *pLength = alloc_len; /* return the number of bytes allocated. */
       }
	    /* client timestamp is valid, copy that into the header */
	if (ts_valid) {
		timestamp = timestamp*4;
		timestamp = timestamp/5;
		timestamp = timestamp << 16;
		memcpy((char *) (&(pTemp->hdr.ts_lo)),
		       (char *) &(timestamp), 4);
		memcpy((char *) (&(pTemp->hdr.ts_hi)),
		       ((char *) &(timestamp)) + 4, 4);
	} else {
		ts_get_lohi(&(pTemp->hdr.ts_lo),
				&(pTemp->hdr.ts_hi));
	}

#if (MSG_TIME_FORMAT == MSG_WCDMA_TIME_FORMAT)
      pTemp->hdr.ts_type = MSG_TS_TYPE_GW;
#else
      pTemp->hdr.ts_type = MSG_TS_TYPE_CDMA_FULL;
#endif
      pTemp->hdr.cmd_code = DIAG_EXT_MSG_F;
      pTemp->hdr.num_args = (uint8)num_args;
      pTemp->hdr.drop_cnt = (unsigned char) ((msg_drop_delta > 255) ? 255 : msg_drop_delta);
      msg_drop_delta = 0;   /* Reset delta drop count */

    /* expand it now, copy over the filename and format strings.
     The order is: hdr,desc,args,format string, filename. args are copied in the msg_send_1 etc... functions */
     memcpy((void *)((char*)(pMsg) + DIAG_REST_OF_DATA_POS + sizeof(msg_hdr_type)), (void *)&(const_blk->desc), sizeof (msg_desc_type));
     memcpy((void *)((char *)(pMsg) + fmt_pos), (void *)(const_blk->fmt), strlen(const_blk->fmt) +1);
     memcpy((void *)((char *)(pMsg) + fmt_pos + strlen(const_blk->fmt) + 1), (void *)(abb_filename), strlen(abb_filename) +1  );
    } /* if (pMsg) */
    else
    {
       msg_drop_delta++;
       msg_dropped++;        /* count up one more ignored message */
    } /* if (pMsg) */
  }/* if (valid_ssid && (const_blk->desc.ss_mask & rt_mask)) */

  else
  {
    //printf("msg_send_prep: mask check returned FALSE \n");
  }

  return pMsg;
}               /* msg_send_prep() */
#endif /* MSM5000_IRAM_FWD */

/*===========================================================================
FUNCTION qsr_msg_send_prep

DESCRIPTION
   Internal function.
   Prepares the buffer that is sent to diag driver by the qsr_msg_send* functions.
   The const block is expanded in the context of the caller.
===========================================================================*/
static byte* qsr_msg_send_prep(const msg_qsr_const_type* const_blk,
                               unsigned int num_args, unsigned int* pLength)
{
  uint32 rt_mask;
  boolean valid_ssid = FALSE;
  byte *pMsg = NULL;

  if(pLength)
  {
     *pLength = 0;
  }
  /* Check the runtime mask */
  valid_ssid = msg_get_ssid_rt_mask ( const_blk->desc.ss_id, &rt_mask);
  if (valid_ssid && (const_blk->desc.ss_mask & rt_mask))
  {
    unsigned int alloc_len = 0;

    /* total number of bytes to be allocated, including space for the hash value */
    alloc_len =            DIAG_REST_OF_DATA_POS + /* For windiag driver to recognize that this is an F3 Msg. */
                           FPOS (msg_qsr_type, args) + /* includes header,desc and hash size */
                           num_args * FSIZ (msg_qsr_type,args[0]);

    /* Get a pointer to a buffer.  If it's a NULL pointer, there is no memory in the client heap. */
    pMsg = (byte *) DiagSvc_Malloc(alloc_len, GEN_SVC_ID);

    if (pMsg)
    {
      /* Find the position to copy in the header, const expanded values etc */
      msg_qsr_type* pTemp = (msg_qsr_type*)((byte*)pMsg + DIAG_REST_OF_DATA_POS);

      /* For windiag driver to recognize that this is an F3 Msg. */
      diag_data* pdiag_data = (diag_data*) pMsg;
      pdiag_data->diag_data_type = DIAG_DATA_TYPE_F3;

      if(pLength)
      {
         *pLength = alloc_len; /* return the number of bytes allocated. */
      }

      //msg_get_time(&(pTemp->hdr.ts));
	  ts_get_lohi(&(pTemp->hdr.ts_lo), &(pTemp->hdr.ts_hi));
#if (MSG_TIME_FORMAT == MSG_WCDMA_TIME_FORMAT)
      pTemp->hdr.ts_type = MSG_TS_TYPE_GW;
#else
      pTemp->hdr.ts_type = MSG_TS_TYPE_CDMA_FULL;
#endif
      pTemp->hdr.cmd_code = DIAG_QSR_EXT_MSG_TERSE_F; /* cmd_code = 146 for QSR messages */
      pTemp->hdr.num_args = (uint8)num_args;
      pTemp->hdr.drop_cnt = (unsigned char) ((msg_drop_delta > 255) ? 255 : msg_drop_delta);
      msg_drop_delta = 0;   /* Reset delta drop count */

     /* expand it now.
      The order is: hdr (already done),desc,hash,args. args are copied in the qsr_msg_send* functions */
      pTemp->desc.line = const_blk->desc.line;
      pTemp->desc.ss_id = const_blk->desc.ss_id;
      pTemp->desc.ss_mask = const_blk->desc.ss_mask;
      pTemp->msg_hash = const_blk->msg_hash;

    } /* if (pMsg) */
    else
    {
      msg_drop_delta++;
      msg_dropped++;        /* count up one more ignored message */
    } /* if (pMsg) */
  }/* if (valid_ssid && (const_blk->desc.ss_mask & rt_mask)) */

  return pMsg;

}/* qsr_msg_send_prep */

/*===========================================================================

FUNCTION MSG_GET_SSID_MASKS

DESCRIPTION
  This function will get an SSID as input and return its corresponding
  real time mask by accessing the appropriate data
  structure. It returns True/False based on whether the SSID is valid or not.

DEPENDENCIES

RETURN VALUE
  True/False

SIDE EFFECTS
  None.
===========================================================================*/
//static boolean
//msg_get_ssid_rt_masks (uint16 ssid_start,
  //          uint16 ssid_end, uint32 * mask)
//{
  // return TRUE;
//   unsigned int i;
//   boolean success = FALSE;
//
//   msg_mask_share_type *mask_begin = (msg_mask_share_type*) msg_mask;
//   byte* mask_end = msg_mask + gMsg_Mask_Size; //msg_mask + &msg_mask_tbl[MSG_MASK_TBL_CNT - 1];
//   uint32 *mask_array = NULL;  /* First pass forces a search */
//
// /*-------------------------------------------------------------------------*/
//   RETAILMSG(1,(TEXT("msg_get_ssid_rt_masks: ssid_start = %d, ssid_end = %d \n"),ssid_start, ssid_end));
//   RETAILMSG(1,(TEXT("msg_get_ssid_rt_masks: mask_begin = %x, mask_end = %x \n"),mask_begin, mask_end));
//
//   if (mask && ssid_start <= ssid_end)
//   {
//     /* Initialize as successful.  If invalid entry found, set to FALSE. */
//     success = TRUE;
//
//     for (i = ssid_start; i <= ssid_end; i++)
//     {
//       /* If not currently matched to a correct table, reset table search. */
//       if ((byte*) mask_begin > mask_end || i < mask_begin->ssid_first || i > mask_begin->ssid_last)
//       {
//         mask_begin = (msg_mask_share_type*) msg_mask;//msg_mask_tbl;
//         mask_array = NULL;
//       }
//
//       /* Table search: if mask_array is not set */
//       while (!mask_array && (byte*)mask_begin <= mask_end)
//       {
//         if (i >= mask_begin->ssid_first && i <= mask_begin->ssid_last)
//         {
//          // if (mask_type == MSG_MASK_BT) /* WM7: we're not concerned with BT mask here, in LSM. BT mask is needed only in DCM version of this function .*/
//           //{
//             //mask_array = tbl->bt_mask_array;
//           //}
//           //else
//           //{
//             mask_array = mask_begin->rt_mask_array;
//           //}
//         }
//         else
//         {
//           //tbl++;        /* Look at next table entry */
//            mask_begin = (msg_mask_share_type*) ((byte*)mask_begin + (mask_begin->ssid_last - mask_begin->ssid_first +1) * sizeof(uint32));
//            RETAILMSG(1,(TEXT("msg_get_ssid_rt_masks: mask_begin, in while loop= %x \n"),mask_begin));
//         }
//       }
//
//       if (mask_array)
//       {
//         /* Valid SSID found.  Write mask to caller's mask. */
//         mask[i - ssid_start] = mask_array[i - mask_begin->ssid_first];
//       }
//       else
//       {
//         /* Invalid SSID.  Indicate failure and continue. */
//         mask[i - ssid_start] = 0;
//         success = FALSE;    /* At least 1 entry is invalid */
//       }
//     }
//   }
//
//   return (success);
//}               /* msg_get_ssid_rt_masks() */



/*===========================================================================
FUNCTION MSG_SPRINTF_PREP

DESCRIPTION
   Prepares the buffer needed by msg_sprintf().
   Allocates, fills in all data except arguments, and returns a pointer
   to the allocated message buffer.  It also handles message statisitics.

RETURN VALUE
   Returns the allocated buffer, and the length of the buffer

DEPENDENCIES
   None
===========================================================================*/
#ifndef MSM5000_IRAM_FWD
static byte *
msg_sprintf_prep (const msg_const_type *pconst_blk, unsigned int num_args, unsigned int* pLength)
{
  uint32 rt_mask;
  boolean valid_ssid = FALSE;
  byte *pMsg = NULL;
  unsigned int alloc_len = DIAG_REST_OF_DATA_POS +
                           FPOS (msg_ext_store_type, args) +
                           num_args * FSIZ (msg_ext_store_type,args[0]);

  /* Check the runtime mask */
  valid_ssid = msg_get_ssid_rt_mask ( (int)pconst_blk->desc.ss_id, &rt_mask);
 if (valid_ssid && (pconst_blk->desc.ss_mask & rt_mask))
 {
    /* Get a pointer to a buffer.  If it's a NULL pointer, there is no memory in the client heap. */
    pMsg = (byte *) DiagSvc_Malloc (alloc_len, GEN_SVC_ID);
    if (pMsg)
    {
       msg_ext_store_type* pTemp = (msg_ext_store_type*)((byte*)pMsg + DIAG_REST_OF_DATA_POS);
       diag_data* pdiag_data = (diag_data*) pMsg;
       //Prototyping Diag 1.5 WM7:Fill in the fact that this is an F3 Message.
       pdiag_data->diag_data_type = DIAG_DATA_TYPE_F3;

       if(pLength)
       {
          *pLength = alloc_len; /* return value, of how much memory is allocated */
       }

	   ts_get_lohi(&(pTemp->hdr.ts_lo), &(pTemp->hdr.ts_hi));
#if (MSG_TIME_FORMAT == MSG_WCDMA_TIME_FORMAT)
      pTemp->hdr.ts_type = MSG_TS_TYPE_GW;
#else
      pTemp->hdr.ts_type = MSG_TS_TYPE_CDMA_FULL;
#endif
      pTemp->hdr.num_args = (uint8)num_args;
      pTemp->hdr.drop_cnt = (unsigned char) ((msg_drop_delta > 255) ? 255 : msg_drop_delta);
      msg_drop_delta = 0;   /* Reset delta drop count */
     /* Set the pointer to the constant blk, to be expanded by msg_sprintf */
      pTemp->const_data_ptr = pconst_blk;
    }
    else
    {
		#ifdef FEATURE_WINMOB
       RETAILMSG(1,(TEXT("msg_sprintf_prep: mask check returned FALSE \n")));
	   #else
	   //printf("msg_sprintf_prep: mask check returned FALSE \n");
	   #endif
      msg_drop_delta++;
      msg_dropped++;        // count up one more ignored message
    }
	// REMOVED MASKING FOR PHASE I
 }
 else
 {
	  #ifdef FEATURE_WINMOB
	  RETAILMSG(1,(TEXT("msg_sprintf_prep: mask check returned FALSE \n")));
	  #else
	  //printf("msg_sprintf_prep: mask check returned FALSE \n");
	  #endif
  }

  return pMsg;
}               /* msg_sprintf_prep() */
#endif /* MSM5000_IRAM_FWD */

