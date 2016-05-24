/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

              Legacy Service Mapping layer implementation for Packet request/response

GENERAL DESCRIPTION
  Contains main implementation of Legacy Service Mapping layer for Diagnostic Packet Req/Res Services.

EXTERNALIZED FUNCTIONS
  diagpkt_alloc
  diagpkt_subsys_alloc
  diagpkt_shorten
  diagpkt_commit
  diagpkt_get_cmd_code
  diagpkt_set_cmd_code
  diagpkt_subsys_get_id
  diagpkt_subsys_get_cmd_code
  diagpkt_err_rsp
  diagpkt_subsys_alloc_v2
  diagpkt_subsys_alloc_v2_delay
  diagpkt_delay_commit
  diagpkt_subsys_get_status
  diagpkt_subsys_set_status
  diagpkt_subsys_get_delayed_rsp_id
  diagpkt_subsys_reset_delayed_rsp_id
  diagpkt_subsys_set_rsp_cnt

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2007-2011, 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header:

when       who    what, where, why
--------   ---    ----------------------------------------------------------
10/01/08   SJ    Changes for CBSP2.0
04/14/08   JV    Added support to pass a pointer to a locally created object as
                 an argument to IDiagPkt_BindPkt()
04/14/08   JV    Replaced KxMutex lock and unlock with ICritSect enter and leave
02/27/08   JV    Created

===========================================================================*/


/* ==========================================================================
   Include Files
========================================================================== */
#if defined (FEATURE_WINCE)
#include <windows.h>
#endif /* FEATURE_WINCE */
#include "stdio.h"
#include "comdef.h"
#include "diagpkt.h"
#include "diag_lsmi.h"
#include "diagsvc_malloc.h"
#include "diagdiag.h"
#include "diagcmd.h"
#include "diag.h" /* For definition of diag_cmd_rsp */
#include "diag_lsm_pkt_i.h"
#include "diagi.h"
#include "diagpkti.h"
//#include "winbase.h" /* For Windows native Critical sections, DeviceIOControl() etc */
//#include "diagtune.h" /* to be able to include diag_shared_i.h */
#include "diag_shared_i.h" /* data structures for registration */

#include <pthread.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <assert.h>
#include "errno.h"
#include "../include/diag_lsm.h"

#define std_strlprintf     snprintf

/* Used to maintain identity of processor, for a particaulr packet registration.
 This is defined in kernel code too */
#define APPS_PROC 1
/*Local Function declarations*/
static void diagpkt_user_tbl_init (void);
uint32 diagpkt_with_delay (diagpkt_subsys_id_type id,
                           diagpkt_subsys_cmd_code_type code);

PACK(void *) diagpkt_delay_alloc(
  diagpkt_cmd_code_type code,
  unsigned int length);

void diagpkt_request_response_handler(const unsigned char* packet,
        uint16 packetLen, unsigned char* response, int responseLen, int* responseLenReq);

/*this keeps track of number of failures to IDiagPkt_Send().
This will currently be used only internally.*/
static unsigned int gPkt_commit_fail = 0;

typedef struct
{
  uint8 command_code;
}
diagpkt_hdr_type;

typedef struct
{
  uint8 command_code;
  uint8 subsys_id;
  uint16 subsys_cmd_code;
}
diagpkt_subsys_hdr_type;

typedef struct
{
  uint8 command_code;
  uint8 subsys_id;
  uint16 subsys_cmd_code;
  uint32 status;
  uint16 delayed_rsp_id;
  uint16 rsp_cnt; /* 0, means one response and 1, means two responses */
}
diagpkt_subsys_hdr_type_v2;

struct diagpkt_delay_params{
		void* pRsp;
		int size;
		int* num_bytes_ptr;
};

#define DIAGPKT_HDR_PATTERN (0xDEADD00DU)
#define DIAGPKT_OVERRUN_PATTERN (0xDEADU)
#define DIAGPKT_USER_TBL_SIZE 128

/* This is the user dispatch table. */
static diagpkt_user_table_type *diagpkt_user_table[DIAGPKT_USER_TBL_SIZE];

#define DIAGPKT_PKT2LSMITEM(p) \
  ((diagpkt_lsm_rsp_type *) (((byte *) p) - FPOS (diagpkt_lsm_rsp_type, rsp.pkt)))

/* critical section for the user table */
#ifdef FEATURE_WINMOB
CRITICAL_SECTION gUserTblCritSect;
#else
pthread_mutex_t gUserTbl_mutex;
#endif

/* delayed_rsp_id 0 represents no delay in the response. Any other number
   means that the diag packet has a delayed response. */
uint16 delayed_rsp_id = 1;

#define DIAGPKT_MAX_DELAYED_RSP 0xFFFF

/*=============================================================================
FUNCTION DIAGPKT_GET_DELAYED_RSP_ID_LSM

DESCRIPTION
Internal function.
IOCTL into the diag driver, and get the next delayed response id in the system

DEPENDENCIES
Windiag driver should be initialized.
=============================================================================*/

boolean diagpkt_get_delayed_rsp_id_lsm(uint16* pRsp_ID)
{
   boolean success = FALSE;
   struct diagpkt_delay_params obj;

#ifdef FEATURE_WINMOB
		if(ghWinDiag && pRsp_ID)
	#else
		if((fd != -1) && pRsp_ID)
	#endif
		{
		#ifdef FEATURE_WINMOB
        DWORD NumberOfBytes = 0;
		if(
         (DeviceIoControl(ghWinDiag, DIAG_IOCTL_GET_DELAYED_RSP_ID, NULL, 0, (LPVOID)pRsp_ID, FSIZ(diagpkt_subsys_hdr_type_v2,delayed_rsp_id), &NumberOfBytes, NULL))
         && (NumberOfBytes == FSIZ(diagpkt_subsys_hdr_type_v2,delayed_rsp_id))
         )
		#else
		int NumberOfBytes = 0;
        obj.num_bytes_ptr = &NumberOfBytes;
		obj.size = FSIZ(diagpkt_subsys_hdr_type_v2,delayed_rsp_id);
		obj.pRsp = pRsp_ID;
        if(!ioctl(fd, DIAG_IOCTL_GET_DELAYED_RSP_ID, (void *)&obj, sizeof(obj), NULL, 0, NULL, NULL )&& (NumberOfBytes == FSIZ(diagpkt_subsys_hdr_type_v2,delayed_rsp_id)))
		#endif
	   {
         success = TRUE;
      }
      else
      {
		 #ifdef FEATURE_WINMOB
         WCE_MSG(1, (L"Diag_LSM: diagpkt_get_delayed_rsp_id_lsm: DeviceIOControl failed."));
		 #else
		 DIAG_LOGE(" diagpkt_get_delayed_rsp_id_lsm: "
				 "DeviceIOControl failed.");
		 #endif
      }
   }
   return success;
}
/*===========================================================================

FUNCTION DIAGPKT_WITH_DELAY

DESCRIPTION
  This procedure checks if the diagnostic packet has been registered with or
  without delay.

DEPENDENCIES
  None.

RETURN VALUE
  Return value is 0 if Diag packet has no delayed response and 1 if Diag
      packet has delayed response

SIDE EFFECTS
  None



===========================================================================*/
uint32 diagpkt_with_delay (diagpkt_subsys_id_type id,
                           diagpkt_subsys_cmd_code_type code)
{
	uint16 packet_id = code;      /* Command code for std or subsystem */
    uint8 subsys_id = id;
    const diagpkt_user_table_type *user_tbl_entry = NULL;
    const diagpkt_user_table_entry_type *tbl_entry = NULL;
    int tbl_entry_count = 0;
    int i, j;
    boolean found = FALSE;
    uint32 delay_flag = 0;

  /* Search the dispatch table for a matching subsystem ID.  If the
     subsystem ID matches, search that table for an entry for the given
     command code. */
    for (i = 0; !found && i < DIAGPKT_USER_TBL_SIZE; i++)
    {
		user_tbl_entry = diagpkt_user_table[i];
		if (user_tbl_entry != NULL && user_tbl_entry->subsysid == subsys_id)
        {
			tbl_entry = user_tbl_entry->user_table;
			delay_flag = user_tbl_entry->delay_flag;
			tbl_entry_count = (tbl_entry) ? user_tbl_entry->count : 0;

            for (j = 0; (tbl_entry!=NULL) && !found && j < tbl_entry_count; j++)
            {
				if (packet_id >= tbl_entry->cmd_code_lo &&
                       packet_id <= tbl_entry->cmd_code_hi)
                {
                /* If the entry has no func, ignore it. */
                    found = TRUE;
                }
                tbl_entry++;
            }
        } /* endif if (user_tbl_entry != NULL && user_tbl_entry->subsysid == subsys_id) */
    }
	return delay_flag;
}               /* diagpkt_with_delay */


/*===========================================================================

FUNCTION DIAGPKT_DELAY_ALLOC

DESCRIPTION
  This function allocates the specified amount of space in a pre-malloced buffer.

DEPENDENCIES
  None.

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
PACK(void *) diagpkt_delay_alloc(
  diagpkt_cmd_code_type code,
  unsigned int length)
{
  PACK(void *)ptr = NULL;
  unsigned int size = 0;
   diagpkt_subsys_hdr_type_v2 *pkt = NULL;

  size = DIAG_DEL_RESP_REST_OF_DATA_POS + length;


  /* We allocate from the memory pool used for events, logs and messages
  because it is a delayed response. */
  ptr = DiagSvc_Malloc (size, GEN_SVC_ID);
  if (NULL != ptr)
   {
     diag_data_delayed_response* pdiag_data = (diag_data_delayed_response*) ptr;
     pdiag_data->length = length;
     pdiag_data->diagdata.diag_data_type = DIAG_DATA_TYPE_DELAYED_RESPONSE;
     pkt = (diagpkt_subsys_hdr_type_v2*) ((byte*)(pdiag_data)+DIAG_DEL_RESP_REST_OF_DATA_POS);
     pkt->command_code = code;
  }
  else
  {
	  /* Alloc not successful.  Return NULL. DiagSvc_Malloc() allocates memory
	  from client's heap using a malloc call if the pre-malloced buffers are not available.
	  So if this fails, it means that the client is out of heap. */

	DIAG_LOGE(" diagpkt_delay_alloc: DiagSvc_Malloc Failed");
  }
  return pkt;
} /* diagpkt_delay_alloc */


/*===========================================================================

FUNCTION DIAGPKT_LSM_PROCESS_REQUEST

DESCRIPTION
  This procedure formats a response packet in response to a request
  packet received from the Diagnostic Monitor.  Calls packet processing
  functions through a table, counts input and output types

DEPENDENCIES
  None.

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/

void
diagpkt_LSM_process_request (void *req_pkt, uint16 pkt_len,
							 diag_cmd_rsp rsp_func, void *rsp_func_param)
{
	uint16 packet_id;     /* Command code for std or subsystem */
    uint8 subsys_id = DIAGPKT_NO_SUBSYS_ID;
    const diagpkt_user_table_type *user_tbl_entry = NULL;
    const diagpkt_user_table_entry_type *tbl_entry = NULL;
    int tbl_entry_count = 0;
    int i,j;
    void *rsp_pkt = NULL;
    boolean found = FALSE;
    uint16 cmd_code = 0xFF;
	#ifdef PKT_RESPONSE_DEBUG
	DIAG_LOGE(" print received packet \n");
	for (i=0;i<pkt_len;i++) {
			DIAG_LOGE("\t %x \t",
					*((unsigned char*)(req_pkt + i)));
	}
	#endif
    packet_id = diagpkt_get_cmd_code (req_pkt);

    if ( packet_id == DIAG_SUBSYS_CMD_VER_2_F )
    {
		cmd_code = packet_id;
    }

	if ((packet_id == DIAG_SUBSYS_CMD_F) || ( packet_id == DIAG_SUBSYS_CMD_VER_2_F ))
    {
		subsys_id = diagpkt_subsys_get_id (req_pkt);
        packet_id = diagpkt_subsys_get_cmd_code (req_pkt);
    }

 /* Search the dispatch table for a matching subsystem ID.  If the
     subsystem ID matches, search that table for an entry for the given
     command code. */

    for (i = 0; !found && i < DIAGPKT_USER_TBL_SIZE; i++)
    {
        user_tbl_entry = diagpkt_user_table[i];

        if (user_tbl_entry != NULL && user_tbl_entry->subsysid == subsys_id && user_tbl_entry->cmd_code == cmd_code)
        {
			tbl_entry = user_tbl_entry->user_table;

            tbl_entry_count = (tbl_entry) ? user_tbl_entry->count : 0;

            for (j = 0; (tbl_entry!=NULL) && !found && j < tbl_entry_count; j++)
            {
				if (packet_id >= tbl_entry->cmd_code_lo &&
                    packet_id <= tbl_entry->cmd_code_hi)
                {
                 /* If the entry has no func, ignore it. */
                     if (tbl_entry->func_ptr)
                     {
						 found = TRUE;
                         rsp_pkt = (void *) (*tbl_entry->func_ptr) (req_pkt, pkt_len);
						 if (rsp_pkt)
                         {
							/* The most common case: response is returned.  Go ahead
							 and commit it here. */
							 diagpkt_commit (rsp_pkt);

                         } /* endif if (rsp_pkt) */
	               } /* endif if (tbl_entry->func_ptr) */
                } /* endif if (packet_id >= tbl_entry->cmd_code_lo && packet_id <= tbl_entry->cmd_code_hi)*/
				tbl_entry++;
            } /* for (j = 0; (tbl_entry!=NULL) && !found && j < tbl_entry_count; j++) */
        } /* endif if (user_tbl_entry != NULL && user_tbl_entry->subsysid == subsys_id
             && user_tbl_entry->cmd_code == cmd_code)*/
     } /*  for (i = 0; !found && i < DIAGPKT_USER_TBL_SIZE; i++) */

  /* Assume that rsp and rsp_pkt are NULL if !found */

  if (!found)
  {
//      ERR_FATAL("Diag_LSM: diagpkt_LSM_process_request: Did not find match in user table",0,0,0);
	       DIAG_LOGE(" diagpkt_LSM_process_request: Did not find "
				" match in user table \n");
  }
  return;
}               /* diagpkt_LSM_process_request */

/* ==========================================================================
FUNCTION
DIAGPKT_USER_TBL_INIT

DESCRIPTION
  Registers the table given to the diagpkt user table

DEPENDENCIES
  None.

RETURN VALUE
  None

SIDE EFFECTS
  None
============================================================================= */
static void
diagpkt_user_tbl_init (void)
{
	int i = 0;
    static boolean initialized = FALSE;

    if (!initialized)
	{
     /* This can apparently throw a STATUS_NO_MEMORY exception,
     we might've to handle that. TODO. */
	//REMOVED WM SPECIFIC CODE
		#ifdef FEATURE_WINMOB
     InitializeCriticalSection(&gUserTblCritSect);
		#else
	 pthread_mutex_init( &gUserTbl_mutex, NULL );
	 #endif
     for (i = 0; ((i < DIAGPKT_USER_TBL_SIZE) && (TRUE == initialized)); i++)
     {
      diagpkt_user_table[i] = NULL;
     }
	}/* if (!initialized) */
}
/*===========================================================================
FUNCTION   Diag_LSM_Pkt_Init

DESCRIPTION
  Initializes the Packet Req/Res service.

DEPENDENCIES
  None.

RETURN VALUE
  FALSE = failure, else TRUE

SIDE EFFECTS
  None

===========================================================================*/

boolean Diag_LSM_Pkt_Init(void)
{
   return TRUE;
} /* Diag_LSM_Pkt_Init */

/*===========================================================================

FUNCTION    Diag_LSM_Pkt_DeInit

DESCRIPTION
  De-Initialize the Diag Packet Req/Res service.

DEPENDENCIES
  None.

RETURN VALUE
  boolean: returns TRUE; currently does nothing.

SIDE EFFECTS
  None

===========================================================================*/
boolean Diag_LSM_Pkt_DeInit(void)
{
	int i = 0;
	#ifdef FEATURE_WINMOB
   DWORD client_id = gdwClientID;
   #else
   int client_id = gdwClientID;
   #endif
   boolean ret = TRUE;

   /* De-register from DCM. */
   //REMOVED WM SPECIFIC CODE
   #ifdef FEATURE_WINMOB
   if(!DeviceIoControl(ghWinDiag, DIAG_IOCTL_COMMAND_DEREG, (LPVOID)&client_id, sizeof(client_id), NULL, 0, NULL, NULL))
   #else
   if(!ioctl(fd, DIAG_IOCTL_COMMAND_DEREG,(void*)&client_id, sizeof(client_id), NULL, 0, NULL, NULL))
   #endif
 	{
 	   DIAG_LOGE(" Diag_LSM_Pkt_DeInit: DeviceIOControl failed, "
			"Error = %d\n.",errno);
      ret = FALSE;
 	}

   /* free the entries in user table */
    for (i = 0; i < DIAGPKT_USER_TBL_SIZE; i++)
    {
		if (diagpkt_user_table[i] != NULL)
		{
			free(diagpkt_user_table[i]);
			diagpkt_user_table[i] = NULL;
		}
        else
            break;
    }

    /* TODO: When this function is called, DCM cleans up it's tables */
    return ret;
} /* Diag_LSM_Pkt_DeInit */


/* ==========================================================================*/
/* Externalized functions */

/* Do not call this function directly. Use the macros defined in diagpkt.h. */
/* ==========================================================================
FUNCTION DIAGPKT_TBL_REG

DESCRIPTION
   Registers the table given to the diagpkt user table

DEPENDENCIES
  None.

RETURN VALUE
  None

SIDE EFFECTS
  None
=============================================================================*/
void
diagpkt_tbl_reg (const diagpkt_user_table_type * tbl_ptr)
{
#ifdef FEATURE_WINMOB
     if(NULL != ghWinDiag)
#else
	if(-1 != fd)
#endif
	{
 	  int i = 0; //,j=0;
 	  word num_entries = tbl_ptr->count;
 	  bindpkt_params *bind_req = (bindpkt_params*)
 	                        malloc(sizeof(bindpkt_params) * num_entries);
     bindpkt_params_per_process bind_req_send;
 	  if(NULL != bind_req)
 	  {
 		  /* Make sure this is initialized */
           diagpkt_user_tbl_init ();

           for (i = 0; i < DIAGPKT_USER_TBL_SIZE; i++)
           {
    		     if (diagpkt_user_table[i] == NULL)
    		     {
				   // REMOVED WM SPECIFIC CODE
					 #ifdef FEATURE_WINMOB
                 EnterCriticalSection(&gUserTblCritSect);
					 #else
                 pthread_mutex_lock( &gUserTbl_mutex );
	   			     #endif
                 diagpkt_user_table[i] = (diagpkt_user_table_type *)
    			                        malloc(sizeof(diagpkt_user_table_type));
       			  if (NULL == diagpkt_user_table[i])
       			  {
       			    DIAG_LOGE(" diagpkt_tbl_reg: malloc failed.");
       				  free (bind_req);
					  // REMOVED WM SPECIFIC CODE
					  #ifdef FEATURE_WINMOB
                    LeaveCriticalSection(&gUserTblCritSect); /*this is a bug in WM6 code, need to leave critical section before returning from function. */
						#else
                      pthread_mutex_unlock( &gUserTbl_mutex );
					  #endif
       				  return;
       			  }
                 memcpy(diagpkt_user_table[i], tbl_ptr, sizeof(diagpkt_user_table_type));
                 break;
				 #ifdef FEATURE_WINMOB
                 LeaveCriticalSection(&gUserTblCritSect);
				 #else
				 pthread_mutex_unlock( &gUserTbl_mutex );
				 #endif
    		     }
           }
          bind_req_send.count = num_entries;
		  #ifdef FEATURE_WINMOB
          StringCbPrintf(bind_req_send.sync_obj_name, sizeof(bind_req_send.sync_obj_name), _T("%s%d"), DIAG_LSM_PKT_EVENT_PREFIX, gdwClientID);
		  #else
		  (void)std_strlprintf(bind_req_send.sync_obj_name, MAX_SYNC_OBJ_NAME_SIZE,
								"%s%d", DIAG_LSM_PKT_EVENT_PREFIX, gdwClientID);
		  #endif

			for (i = 0; i < num_entries; i++) {
				bind_req[i].cmd_code = tbl_ptr->cmd_code;
				bind_req[i].subsys_id = tbl_ptr->subsysid;
				bind_req[i].cmd_code_lo = tbl_ptr->user_table[i].cmd_code_lo;
				bind_req[i].cmd_code_hi = tbl_ptr->user_table[i].cmd_code_hi;
				bind_req[i].event_id = 0;
				bind_req[i].log_code = 0;
				bind_req[i].client_id = gdwClientID;
				bind_req[i].proc_id = APPS_PROC;
			 #ifdef PKT_RESPONSE_DEBUG
 DIAG_LOGE(" params are %d \t%d \t%d \t%d \t%d \t %d \t \n",
		 bind_req[i].cmd_code, bind_req[i].subsys_id,
		bind_req[i].cmd_code_lo, bind_req[i].cmd_code_hi,
		bind_req[i].proc_id, gdwClientID  );
			 #endif
	      }
          bind_req_send.params = bind_req;
		  #ifdef FEATURE_WINMOB
		  if(!DeviceIoControl(ghWinDiag, DIAG_IOCTL_COMMAND_REG, &bind_req_send, sizeof(bind_req_send), NULL, 0, NULL, NULL))
		  #else
		  if(!ioctl(fd, DIAG_IOCTL_COMMAND_REG, &bind_req_send, sizeof(bind_req_send), NULL, 0, NULL, NULL))
		  #endif
		  {
			DIAG_LOGE("diagpkt_tbl_reg: DeviceIOControl failed, error: %d\n", errno);
 	      }
          free (bind_req);

 	  } /* if(NULL != bind_req) */

   } /* if(NULL != ghWinDiag)// && (NULL != gpiDiagPktRsp)) */
   else
   {
   DIAG_LOGE(" diagpkt_tbl_reg: Registration failed.");
   }

}

/*===========================================================================

FUNCTION DIAGPKT_GET_CMD_CODE

DESCRIPTION
  This function returns the command code in the specified diag packet.

DEPENDENCIES
  None.

RETURN VALUE
  cmd_code

SIDE EFFECTS
  None

===========================================================================*/
diagpkt_cmd_code_type
diagpkt_get_cmd_code (PACK(void *)ptr)
{
	diagpkt_cmd_code_type cmd_code = 0;
	if(ptr)
	{
		/* Diag command codes are the first byte */
        return *((diagpkt_cmd_code_type *) ptr);
	}
	return cmd_code;
}               /* diag_get_cmd_code */


/*===========================================================================

FUNCTION DIAGPKT_SET_CMD_CODE

DESCRIPTION
  This function sets the command code in the specified diag packet.

DEPENDENCIES
  None.

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void
diagpkt_set_cmd_code (PACK(void *)ptr, diagpkt_cmd_code_type cmd_code)
{
	if(ptr)
	{
		*((diagpkt_cmd_code_type *) ptr) = cmd_code;
	}
}               /* diagpkt_set_cmd_code */



/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_GET_ID

DESCRIPTION
  This function returns the subsystem ID in the specified diag packet.


DEPENDENCIES
  None.

RETURN VALUE
  subsys_id. If the packet is not a DIAG_SUBSYS_CMD_F or DIAG_SUBSYS_CMD_VER_2_F packet,
  0xFF is returned.

SIDE EFFECTS
  None

===========================================================================*/
diagpkt_subsys_id_type
diagpkt_subsys_get_id (PACK(void *)ptr)
{
	diagpkt_subsys_id_type id = 0;
	if (ptr)
	{
		diagpkt_subsys_hdr_type *pkt_ptr = (void *) ptr;

        if ((pkt_ptr->command_code == DIAG_SUBSYS_CMD_F) || (pkt_ptr->command_code
                      == DIAG_SUBSYS_CMD_VER_2_F))
        {
		    id = (diagpkt_subsys_id_type) pkt_ptr->subsys_id;
        }
        else
        {
		    id = 0xFF;
        }
	}
    return id;
}               /* diagpkt_subsys_get_id */

/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_GET_CMD_CODE

DESCRIPTION
  This function returns the subsystem command code in the specified
  diag packet.

DEPENDENCIES
  None.

RETURN VALUE
  subsys_cmd_code. If the packet is not a DIAG_SUBSYS_CMD_F or DIAG_SUBSYS_CMD_VER_2_F packet,
  0xFFFF is returned.

SIDE EFFECTS
  None
===========================================================================*/
diagpkt_subsys_cmd_code_type
diagpkt_subsys_get_cmd_code (PACK(void *)ptr)
{
	diagpkt_subsys_cmd_code_type code = 0;
	if(ptr)
	{
		diagpkt_subsys_hdr_type *pkt_ptr = (void *) ptr;

        if ((pkt_ptr->command_code == DIAG_SUBSYS_CMD_F) || (pkt_ptr->command_code
            == DIAG_SUBSYS_CMD_VER_2_F))
        {
		    code = pkt_ptr->subsys_cmd_code;
        }
        else
        {
            code = 0xFFFF;
		}
	}
	return code;
}               /* diagpkt_subsys_get_cmd_code */


/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_GET_STATUS

DESCRIPTION
  This function gets the status field in the DIAG_SUBSYS_CMD_VER_2_F packet

DEPENDENCIES
  This function's first argument (ptr) should always be DIAG_SUBSYS_CMD_VER_2_F
  packet.

RETURN VALUE
  status

SIDE EFFECTS
  None

===========================================================================*/
diagpkt_subsys_status_type
diagpkt_subsys_get_status (PACK(void *)ptr)
{
  diagpkt_subsys_hdr_type_v2 *pkt_ptr = (void *) ptr;

  assert (pkt_ptr != NULL);
  assert (pkt_ptr->command_code == DIAG_SUBSYS_CMD_VER_2_F);

  return pkt_ptr->status;
}               /* diagpkt_subsys_get_status */


/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_SET_STATUS

DESCRIPTION
  This function sets the status field in the DIAG_SUBSYS_CMD_VER_2_F packet.

DEPENDENCIES
  This function's first argument (ptr) should always be DIAG_SUBSYS_CMD_VER_2_F
  packet.

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void
diagpkt_subsys_set_status (PACK(void *)ptr, diagpkt_subsys_status_type status)
{
  diagpkt_subsys_hdr_type_v2 *pkt_ptr = (void *) ptr;

  assert (pkt_ptr != NULL);
  assert (pkt_ptr->command_code == DIAG_SUBSYS_CMD_VER_2_F);

  pkt_ptr->status = status;
}               /* diagpkt_subsys_set_status */


/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_GET_DELAYED_RSP_ID

DESCRIPTION
  This function gets the delayed response ID field in the
  DIAG_SUBSYS_CMD_VER_2_F packet.

DEPENDENCIES
  This function's first argument (ptr) should always be DIAG_SUBSYS_CMD_VER_2_F
  packet.

RETURN VALUE
  delayed response ID

SIDE EFFECTS
  None
===========================================================================*/
diagpkt_subsys_delayed_rsp_id_type
diagpkt_subsys_get_delayed_rsp_id (PACK(void *)ptr)
{
  diagpkt_subsys_hdr_type_v2 *pkt_ptr = (void *) ptr;

  assert (pkt_ptr != NULL);
  assert (pkt_ptr->command_code == DIAG_SUBSYS_CMD_VER_2_F);

  return (pkt_ptr) ? pkt_ptr->delayed_rsp_id : 0;
}               /* diagpkt_subsys_get_delayed_rsp_id */


/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_RESET_DELAYED_RSP_ID

DESCRIPTION
  This function sets the delayed response ID to zero in the
  DIAG_SUBSYS_CMD_VER_2_F packet.

DEPENDENCIES
  This function's first argument (ptr) should always be DIAG_SUBSYS_CMD_VER_2_F
  packet.

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void
diagpkt_subsys_reset_delayed_rsp_id (PACK(void *)ptr)
{
  diagpkt_subsys_hdr_type_v2 *pkt_ptr = (void *) ptr;

  assert (pkt_ptr != NULL);
  assert (pkt_ptr->command_code == DIAG_SUBSYS_CMD_VER_2_F);

  pkt_ptr->delayed_rsp_id = 0;
}               /* diagpkt_subsys_reset_delayed_rsp_id */


/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_SET_RSP_CNT

DESCRIPTION
  This function sets the response count in the DIAG_SUBSYS_CMD_VER_2_F packet.

DEPENDENCIES
  This function's first argument (ptr) should always be DIAG_SUBSYS_CMD_VER_2_F
  packet.

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void
diagpkt_subsys_set_rsp_cnt (PACK(void *)ptr, diagpkt_subsys_rsp_cnt rsp_cnt)
{
  diagpkt_subsys_hdr_type_v2 *pkt_ptr = (void *) ptr;

  assert (pkt_ptr != NULL);
  assert (pkt_ptr->command_code == DIAG_SUBSYS_CMD_VER_2_F);

  pkt_ptr->rsp_cnt = rsp_cnt;
}               /* diagpkt_subsys_set_rsp_cnt */


/*============================================================================
FUNCTION DIAGPKT_ALLOC

DESCRIPTION
  This function allocates the specified amount of space from a pre-malloced buffer.
  If space is unavailable in the pre-malloced buffer, then a malloc is done.

DEPENDENCIES
  diagpkt_commit() must be called to commit the response packet to be sent.
  Not calling diagpkt_commit() will result in a memory leak.

RETURN VALUE
  pointer to the allocated memory

SIDE EFFECTS
  None

============================================================================*/
PACK(void *)
diagpkt_alloc (diagpkt_cmd_code_type code, unsigned int length)
{
    diagpkt_lsm_rsp_type *item = NULL;
    diagpkt_hdr_type *pkt = NULL;
    PACK(uint16 *)pattern = NULL;    /* Overrun pattern. */
    unsigned char *p;
    diag_data* pdiag_data = NULL;
     unsigned int size = 0;

	#ifdef FEATURE_WINMOB
     if(NULL == ghWinDiag)
    #else
	 if(-1 == fd)
	#endif
      {
         return NULL;
      }
     size = DIAG_REST_OF_DATA_POS + FPOS (diagpkt_lsm_rsp_type, rsp.pkt) + length + sizeof (uint16);

    /*-----------------------------------------------
      Try to allocate a buffer.  Size of buffer must
      include space for overhead and CRC at the end.
    -----------------------------------------------*/
      pdiag_data = (diag_data*)DiagSvc_Malloc (size, PKT_SVC_ID);
      if(NULL == pdiag_data)
      {
         /* Alloc not successful.  Return NULL. DiagSvc_Malloc() allocates memory
	  from client's heap using a malloc call if the pre-malloced buffers are not available.
	  So if this fails, it means that the client is out of heap. */
         return NULL;
      }
      /* Fill in the fact that this is a response */
      pdiag_data->diag_data_type = DIAG_DATA_TYPE_RESPONSE;
      // WM7 prototyping: advance the pointer now
      item = (diagpkt_lsm_rsp_type*)((byte*)(pdiag_data)+DIAG_REST_OF_DATA_POS);

    /* This pattern is written to verify pointers elsewhere in this
       service  are valid. */
    item->rsp.pattern = DIAGPKT_HDR_PATTERN;    /* Sanity check pattern */

    /* length ==  size unless packet is resized later */
    item->rsp.size = length;
    item->rsp.length = length;

    pattern = (PACK(uint16 *)) & item->rsp.pkt[length];

    /* We need this to meet alignment requirements - MATS */
    p = (unsigned char *) pattern;
    p[0] = (DIAGPKT_OVERRUN_PATTERN >> 8) & 0xff;
    p[1] = (DIAGPKT_OVERRUN_PATTERN >> 0) & 0xff;

    pkt = (diagpkt_hdr_type *) & item->rsp.pkt;

    if (pkt)
    {
        pkt->command_code = code;
    }
    return (PACK(void *)) pkt;

}               /* diagpkt_alloc */

/*===========================================================================

FUNCTION DIAGPKT_DELAY_COMMIT

DESCRIPTION
  This function commits the response.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void diagpkt_delay_commit (PACK(void *) pkt)
{
	if (pkt)
    {
       unsigned int pkt_len = 0;
       diag_data_delayed_response* pdiag_del_rsp_data = NULL;
      diag_data* pdiag_data = NULL;

       pdiag_del_rsp_data = (diag_data_delayed_response*)((byte*)(pkt) - DIAG_DEL_RESP_REST_OF_DATA_POS);
       pkt_len = pdiag_del_rsp_data->length;

       /* We don't need to Write the "length" field in pdiag_del_rsp_data to DCM,
       so strip that out to get the diag_data from diag_data_delayed_response.
       */
       pdiag_data = (diag_data*)((byte*)pdiag_del_rsp_data + sizeof(pdiag_del_rsp_data->length));

#ifdef FEATURE_WINMOB
      if (pkt_len > 0 && NULL != ghWinDiag)
#else
	  if (pkt_len > 0 && -1 != fd)
#endif
      {
			  #ifdef FEATURE_WINMOB
          DWORD NumberOfBytesWritten = 0;
           if(
              !WriteFile(ghWinDiag, (LPCVOID) pdiag_data, pkt_len + DIAG_REST_OF_DATA_POS, &NumberOfBytesWritten, NULL) ||
               NumberOfBytesWritten != pkt_len
              )
				   #else
	  int NumberOfBytesWritten = write(fd,(const void*)pdiag_data, pkt_len + DIAG_REST_OF_DATA_POS);

          if(NumberOfBytesWritten != 0) // || NumberOfBytesWritten != pkt_len )
				   #endif
	  {
		DIAG_LOGE("Diag_LSM_Pkt: Write failed in %s, bytes written: %d, error: %d\n",
					  __func__, NumberOfBytesWritten, errno);
		gPkt_commit_fail++;
          }
       }
       DiagSvc_Free(pdiag_del_rsp_data, GEN_SVC_ID);
   } /* end if (pkt)*/
    return;
}               /* diagpkt_delay_commit */

/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_ALLOC

DESCRIPTION
  This function returns the command code in the specified diag packet.

DEPENDENCIES
  None

RETURN VALUE
  Pointer to allocated memory

SIDE EFFECTS
  None

===========================================================================*/
PACK(void *)
diagpkt_subsys_alloc (diagpkt_subsys_id_type id,
              diagpkt_subsys_cmd_code_type code, unsigned int length)
{
  diagpkt_subsys_hdr_type *hdr = NULL;
  if(-1 == fd)
  {
     return NULL;
  }

  hdr = (diagpkt_subsys_hdr_type *) diagpkt_alloc (DIAG_SUBSYS_CMD_F, length);

  if( hdr != NULL )
  {
      hdr->subsys_id = id;
      hdr->subsys_cmd_code = code;

  }

  return (PACK(void *)) hdr;

}               /* diagpkt_subsys_alloc */


/*===========================================================================
FUNCTION DIAGPKT_SUBSYS_ALLOC_V2

DESCRIPTION
  This function allocates the specified amount of space from a pre-malloced buffer.
  If space is unavailable in the pre-malloced buffer, then a malloc is done.

DEPENDENCIES
  diagpkt_commit() must be called to commit the response packet to be sent.
  Not calling diagpkt_commit() will result in a memory leak.

RETURN VALUE
  pointer to the allocated memory

SIDE EFFECTS
  None

============================================================================*/

PACK(void *)
diagpkt_subsys_alloc_v2 (diagpkt_subsys_id_type id,
              diagpkt_subsys_cmd_code_type code, unsigned int length)
{
  diagpkt_subsys_hdr_type_v2 *hdr = NULL;
   if(-1 == fd)
   {
       return NULL;
   }

  hdr = (diagpkt_subsys_hdr_type_v2 *) diagpkt_alloc (DIAG_SUBSYS_CMD_VER_2_F, length);

  if (hdr != NULL)
  {
      hdr->subsys_id = id;
      hdr->subsys_cmd_code = code;
      hdr->status = 0;

      if( diagpkt_with_delay (id,code) )
      {
          /* IOCTL into the diag driver, to get the next delayed response id in the system */
         if(!diagpkt_get_delayed_rsp_id_lsm(&(hdr->delayed_rsp_id)))
         {
            /* IOCTL failed, free the allocated memory and return. */
            diagpkt_lsm_rsp_type *item = DIAGPKT_PKT2LSMITEM (hdr);
            diag_data* pdiag_data = (diag_data*)((byte*)(item) - DIAG_REST_OF_DATA_POS);
            DiagSvc_Free(pdiag_data,PKT_SVC_ID);
            return NULL;
         }
      }
	  else
      {
          hdr->delayed_rsp_id = 0;
      }
      hdr->rsp_cnt = 0;
  }
  return (PACK(void *)) hdr;

}               /* diagpkt_subsys_alloc_v2 */


/*===========================================================================

FUNCTION DIAGPKT_SUBSYS_ALLOC_V2_DELAY

DESCRIPTION
  This function allocates the specified amount of space from a pre-malloced buffer.
  If space is unavailable in the pre-malloced buffer, then a malloc is done.This
  function is used to send a delayed response.This response has same priority as
  F3 messages and logs.

DEPENDENCIES
  diagpkt_delay_commit() must be called to commit the response packet to be
  sent. Not calling diagpkt_delay_commit() will result in a memory leak and
  response packet will not be sent.

  Note:User is required to provide delayed response id as an argument.
       This helps tools to match the delayed response with the original
       request response pair.

RETURN VALUE
  pointer to the allocated memory

SIDE EFFECTS
  None

===========================================================================*/
PACK(void *)
diagpkt_subsys_alloc_v2_delay (diagpkt_subsys_id_type id,
              diagpkt_subsys_cmd_code_type code,
              diagpkt_subsys_delayed_rsp_id_type delayed_rsp_id_arg,
              unsigned int length)
{
  diagpkt_subsys_hdr_type_v2 *hdr = NULL;
   if(-1 == fd)
   {
       return NULL;
   }

  hdr = (diagpkt_subsys_hdr_type_v2 *) diagpkt_delay_alloc(
                 DIAG_SUBSYS_CMD_VER_2_F,
                 length);

  if(hdr != NULL)
  {
      hdr->subsys_id = id;
      hdr->subsys_cmd_code = code;
      hdr->status = 0;
      hdr->delayed_rsp_id = delayed_rsp_id_arg;
      hdr->rsp_cnt = 1;
  }
  return (PACK(void *)) hdr;
}               /* diagpkt_subsys_alloc_v2_delay */


/*===========================================================================

FUNCTION DIAGPKT_SHORTEN

DESCRIPTION
  In legacy diag, this function was used to shorten a previously allocated
  response buffer. Now, since we use pre-malloced buffers, this function will
  not serve the purpose of freeing any memory. It just updates the length
  field with the new length.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/

void
diagpkt_shorten (PACK(void *)pkt, unsigned int new_length)
{

  diagpkt_lsm_rsp_type *item = NULL;
  PACK(uint16 *)pattern = NULL;

  if (pkt)
  {
    /* Do pointer arithmetic in bytes, then case to q_type; */
      item = DIAGPKT_PKT2LSMITEM (pkt);

      if (new_length < item->rsp.size)
      {
		  unsigned char *p;
          item->rsp.length = new_length;

      /* Write the new buffer overrun detection pattern */
          pattern = (PACK(uint16 *)) & item->rsp.pkt[new_length];

      /* We need this to meet alignment requirements - MATS */
          p = (unsigned char *) pattern;
          p[0] = (DIAGPKT_OVERRUN_PATTERN >> 8) & 0xff;
          p[1] = (DIAGPKT_OVERRUN_PATTERN >> 0) & 0xff;
      }
      else
      {
		  DIAG_LOGE(" diagpkt_shorten: diagpkt_shorten Failed");
		  return;
      }
  }
  return;
}               /* diagpkt_shorten */
/*===========================================================================

FUNCTION DIAGPKT_COMMIT

DESCRIPTION
  This function commits previously allocated space in the diagnostics output
  buffer.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/

void diagpkt_commit (PACK(void *)pkt)
{
	if (pkt)
	{
		unsigned int length = 0;
		unsigned char *temp = NULL;
		int type = DIAG_DATA_TYPE_RESPONSE;
		int ret;

		diagpkt_lsm_rsp_type *item = DIAGPKT_PKT2LSMITEM (pkt);
		item->rsp_func = NULL;
		item->rsp_func_param = NULL;
		/* end mobile-view */
#ifdef PKT_RESPONSE_DEBUG
		DIAG_LOGE(" printing buffer at top \n");
		int i;
		for(i=0;i<item->rsp.length;i++)
			DIAG_LOGE(" %x ", ((unsigned char*)(pkt))[i]);
#endif
		length = DIAG_REST_OF_DATA_POS + FPOS(diagpkt_lsm_rsp_type, rsp.pkt) + item->rsp.length + sizeof(uint16);
		if (item->rsp.length > 0) {
			if(-1 != fd) {
				temp =  (unsigned char*) DiagSvc_Malloc((int)DIAG_REST_OF_DATA_POS + (int)(item->rsp.length), PKT_SVC_ID);
				if (temp) {
					memcpy(temp, (unsigned char*)&type, DIAG_REST_OF_DATA_POS);
					memcpy(temp+4, pkt, item->rsp.length);
#ifdef PKT_RESPONSE_DEBUG
					int j;
					DIAG_LOGE(" printing buffer %d \n",
						(int)(item->rsp.length +
						DIAG_REST_OF_DATA_POS));
					for(j=0; j < (int)(item->rsp.length + DIAG_REST_OF_DATA_POS) ;j++)
						DIAG_LOGE(" %x ",
						((unsigned char*)(temp))[j]);
					DIAG_LOGE("\n");
#endif
					if((ret = write(fd, (const void*) temp, item->rsp.length + DIAG_REST_OF_DATA_POS)) != 0) /*TODO: Check the Numberofbyteswritten against number of bytes we wanted to write?*/
					{
						DIAG_LOGE("Diag_LSM_Pkt: Write failed in %s, bytes written: %d, error: %d\n",
							__func__, ret, errno);
						gPkt_commit_fail++;
					}
					DiagSvc_Free(temp, PKT_SVC_ID);
				} else
					DIAG_LOGE(" diagpkt_commit: Could "
						"not allocate memory\n");
			}
		}
		diagpkt_free(pkt);
	} /* end if (pkt)*/
	return;
}      /* diagpkt_commit */


/*===========================================================================

FUNCTION DIAGPKT_ERR_RSP

DESCRIPTION
  This function generates an error response packet.

DEPENDENCIES
  None

RETURN VALUE
  pointer to the error response

SIDE EFFECTS
  None

===========================================================================*/

PACK(void *)
diagpkt_err_rsp (diagpkt_cmd_code_type code,
         PACK(void *)req_pkt, uint16 req_len)
{
  DIAG_BAD_CMD_F_rsp_type *rsp;
  const unsigned int rsp_len = MIN (sizeof (DIAG_BAD_CMD_F_rsp_type),
               req_len + FPOS (DIAG_BAD_CMD_F_rsp_type, pkt));
  rsp = (DIAG_BAD_CMD_F_rsp_type *) diagpkt_alloc (code, rsp_len);

  if(req_pkt)
  {
    memcpy ((void *) rsp->pkt,
            (void *) req_pkt,
            rsp_len - FPOS (DIAG_BAD_CMD_F_rsp_type, pkt));
  }
  else if (req_len != 0)
  {
      //MSG_HIGH("Non-0 request length (%d) and NULL request pointer!",req_len,0,0);
     DIAG_LOGE("Non-0 request length (%d) and NULL request pointer!",
								req_len);
  }

  return ((void *) rsp);
}               /* diagkt_err_rsp */

/*=========================================================================
FUNCTION DIAGPKT_FREE

DESCRIPTION

  This function free the packet allocated by diagpkt_alloc(), which doesn't

  need to 'commit' for sending as a response if it is merely a temporary

  processing packet.

===========================================================================*/

void

diagpkt_free(PACK(void *)pkt)

{
  if (pkt)
  {
    byte *item = (byte*)DIAGPKT_PKT2LSMITEM(pkt);
    item -= DIAG_REST_OF_DATA_POS;
    DiagSvc_Free ((void *)item,PKT_SVC_ID);
  }
 return;
}
























