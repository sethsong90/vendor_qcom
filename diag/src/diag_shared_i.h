#ifndef DIAG_SHARED_I_H

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                GENERAL DESCRIPTION

  Diag-internal declarations  and definitions common to API layer
  (Diag_LSM) and diag driver (DCM).

Copyright (c) 2007-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================
                        EDIT HISTORY FOR FILE
$Header:  $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/03/08   mad     Created file.
===========================================================================*/


#include "diagi.h"
/*Prototyping Diag 1.5 WM7: Adding a uint32 to the beginning of the diag packet,
 so the windiag driver can identify what this is. This will be stripped out in the
 WDG_Write() function, and only the rest of the data will
 be copied to diagbuf. */
typedef struct
{
  uint32 diag_data_type; /* This will be used to identify whether the data passed to DCM is an event, log, F3 or response.*/
  uint8 rest_of_data;
}
diag_data;
#define DIAG_REST_OF_DATA_POS (FPOS(diag_data, rest_of_data))

/* Reasoning for an extra structure:
diagpkt_delay_alloc allocates a certain length, to send out a delayed response.
On WM, diagpkt_delay_commit needs to know the length of the response, to call WriteFile(),
and the header doesn't have any length field.

(On AMSS, diagpkt_delay_alloc directly allocates to diagbuf, and length is stored in the
diagbuf header. When diagpkt_delay_commit() (that results in diagbuf_commit()) is called,
we know how much to commit.)

*/
typedef struct
{
   uint32 length; /* length of delayed response pkt */
   diag_data diagdata;
}diag_data_delayed_response;
#define DIAG_DEL_RESP_REST_OF_DATA_POS (FPOS(diag_data_delayed_response,diagdata.rest_of_data))

/* different values that go in for diag_data_type */
#define DIAG_DATA_TYPE_EVENT         0
#define DIAG_DATA_TYPE_F3            1
#define DIAG_DATA_TYPE_LOG           2
#define DIAG_DATA_TYPE_RESPONSE      3
#define DIAG_DATA_TYPE_DELAYED_RESPONSE   4
#define DIAG_DATA_TYPE_DCI_LOG		0x00000100
#define DIAG_DATA_TYPE_DCI_EVENT	0x00000200

/* The various IOCTLs */
#define DIAG_IOCTL_COMMAND_REG       0 /* IOCTL for packet registration
                                 Clients can use this to register to respond to packets from host tool */
#define DIAG_IOCTL_COMMAND_DEREG     1 /* IOCTL for de-registration */
/* Client process uses this to de-register itself, while unloading gracefully. */
#define DIAG_IOCTL_MASK_REG          2 /* IOCTL for registration for mask-change */
#define DIAG_IOCTL_MASK_DEREG        3
#define DIAG_IOCTL_GETEVENTMASK      4 /* For Client process to get event mask from DCM */
#define DIAG_IOCTL_GETLOGMASK        5
#define DIAG_IOCTL_GETMSGMASK        6
#define DIAG_IOCTL_GET_DELAYED_RSP_ID  8 /* Diag_LSM uses this IOCTL to get the next delayed response id
                                          in the system. */
#define DIAG_IOCTL_LSM_DEINIT		9
#define DIAG_IOCTL_SWITCH_LOGGING	7
#define DIAG_IOCTL_DCI_INIT		20
#define DIAG_IOCTL_DCI_DEINIT		21
#define DIAG_IOCTL_DCI_SUPPORT		22
#define DIAG_IOCTL_DCI_REG		23
#define DIAG_IOCTL_DCI_STREAM_INIT	24
#define DIAG_IOCTL_DCI_HEALTH_STATS	25
#define DIAG_IOCTL_DCI_LOG_STATUS	26
#define DIAG_IOCTL_DCI_EVENT_STATUS	27
#define DIAG_IOCTL_DCI_CLEAR_LOGS	28
#define DIAG_IOCTL_DCI_CLEAR_EVENTS	29
#define DIAG_IOCTL_REMOTE_DEV		32
#define DIAG_IOCTL_VOTE_REAL_TIME	33
#define DIAG_IOCTL_GET_REAL_TIME	34

#define DIAG_EVENTSVC_MASK_CHANGE   0
#define DIAG_LOGSVC_MASK_CHANGE     1
#define DIAG_MSGSVC_MASK_CHANGE     2

typedef struct
{
   uint16 cmd_code;
   uint16 subsys_id;
   uint16 cmd_code_lo;
   uint16 cmd_code_hi;
   uint16 proc_id;
   uint32 event_id;
   uint32 log_code;
   uint32 client_id;
}
bindpkt_params;

#define MAX_SYNC_OBJ_NAME_SIZE 32
typedef struct
{
   char sync_obj_name[MAX_SYNC_OBJ_NAME_SIZE]; /* Name of the synchronization object associated with this process */
   uint32 count; /* Number of entries in this bind */
   bindpkt_params *params; /* first bind params */
}
bindpkt_params_per_process;

struct real_time_vote_t {
   uint16 proc;
   uint8 real_time_vote;
};

/* functions used by windiag driver to write events into diag buffers */
void* event_q_alloc (void *in, unsigned int length);
void event_q_pending (void *in);

void diagpkt_tbl_reg_dcm (void* LSM_obj, const byte * tbl_ptr, unsigned int count);
boolean diagpkt_tbl_dereg_dcm(uint32 client_id);
boolean event_process_LSM_mask_req (unsigned char* mask, int maskLen, int * maskLenReq);
boolean log_process_LSM_mask_req (unsigned char* mask, int maskLen, int * maskLenReq);
boolean msg_process_LSM_mask_req (unsigned char* mask, int maskLen, int * maskLenReq);
boolean diagpkt_mask_tbl_reg (uint32 client_id, uint32 LSM_sync_obj);
#endif
