#ifndef DIAG_LSMI_H
#define DIAG_LSMI_H

/*===========================================================================

                   Diag Mapping Layer DLL , internal declarations

DESCRIPTION
  Internal declarations for Diag Service Mapping Layer.


Copyright (c) 2007-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential
===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header:

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/01/08   sj      Added featurization for WM specific code & CBSP2.0
02/04/08   mad     Created File
===========================================================================*/

//#include <windows.h>

#ifdef FEATURE_WINMOB
extern HANDLE ghWinDiag; /* this process's global HANDLE to windiag stream driver */
extern DWORD gdwClientID;
extern HANDLE ghReq_Sync_Event;
extern HANDLE ghMask_Sync_Event;
#else
#define NUM_PROC 10
extern int fd;
extern int fd_md[NUM_PROC];
extern int gdwClientID;
void log_to_device(unsigned char *ptr, int logging_mode, int size, int type);
void lookup_pkt_rsp_transaction(unsigned char *ptr);
void send_mask_modem(unsigned char mask_buf[], int count_mask_bytes);
int diag_has_remote_device(uint16 *remote_mask);
int diag_register_socket_cb(int (*callback_ptr)(void *data_ptr, int socket_id), void *data_ptr);
int diag_set_socket_fd(int socket_id, int socket_fd);
int diag_send_socket_data(int id, unsigned char buf[], int num_bytes);
int diag_get_max_channels(void);
int diag_read_mask_file_list(char *mask_list_file);

/* === Functions dealing with diag wakelocks === */

/* Returns 1 if a wakelock is initialized for this process,
   0 otherwise. */
int diag_is_wakelock_init(void);

/* Opens the wakelock files and initializes the wakelock for
   the current process. It doesn't hold any wakelock. To hold
   a wakelock, call diag_wakelock_acquire. */
void diag_wakelock_init(char *wakelock_name);

/* Closes the wakelock files. It doesn't release the wakelock
   for the current process if held. */
void diag_wakelock_destroy(void);

/* Acquires a wakelock for the current process under the name
   given by diag_wakelock_init. */
void diag_wakelock_acquire(void);

/* Releases the wakelock held by the current process. */
void diag_wakelock_release(void);

#endif
extern boolean gbRemote;
//removed WM specific code
#ifdef FEATURE_WINMOB
	#define DIAG_LSM_PKT_EVENT_PREFIX _T("DIAG_SYNC_EVENT_PKT_")
	#define DIAG_LSM_MASK_EVENT_PREFIX _T("DIAG_SYNC_EVENT_MASK_")
#else
	#define DIAG_LSM_PKT_EVENT_PREFIX "DIAG_SYNC_EVENT_PKT_"
	#define DIAG_LSM_MASK_EVENT_PREFIX "DIAG_SYNC_EVENT_MASK_"
#endif
#endif /* DIAG_LSMI_H */

