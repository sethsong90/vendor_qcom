#ifndef DIAG_LSM_DCI_H
#define DIAG_LSM_DCI_H

#ifdef __cplusplus
extern "C"
{
#endif
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
# Copyright (c) 2012 by Qualcomm Technologies, Inc.  All Rights Reserved.
# Qualcomm Technologies Proprietary and Confidential.

	  Diag Consumer Interface (DCI)

GENERAL DESCRIPTION

Headers specific to DCI.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

	    EDIT HISTORY FOR MODULE

$Header:

when       who    what, where, why
--------   ---    ----------------------------------------------------------
10/09/12   RA     Added Interface for DCI I/O
03/20/12   SJ     Created
===========================================================================*/

/*strlcpy is from OpenBSD and not supported by Meego.
GNU has an equivalent g_strlcpy implementation into glib.
Featurized with compile time USE_GLIB flag for Meego builds.*/
#ifdef USE_GLIB
#define strlcpy g_strlcpy
#define strlcat g_strlcat
#endif

/* ---------------------------------------------------------------------------
		Internal functions
 --------------------------------------------------------------------------- */

/* Internal function used for DCI related initializations */
int diag_lsm_dci_init(void);

/* Internal function used by LSM to find transaction related information */
void lookup_pkt_rsp_transaction(unsigned char *ptr);

/*
 * Structure to maintain information about every request sent through DCI
 *
 * @uid: unique identifier for the request
 * @func_ptr: callback function to be called on receiving a response
 * @rsp_ptr: pointer to hold the response
 * @rsp_len: length of the response
 * @data_ptr: Additional data specified by the client
 */
typedef struct {
	int uid;
	void (*func_ptr)(unsigned char *, int len, void *data_ptr);
	unsigned char *rsp_ptr;
	int rsp_len;
	void *data_ptr;
} diag_pkt_tracking_info;

/*
 * Structure to maintain a list of all requests sent by the client for bookkeeping
 * purposed. The table is keyed by the UID.
 *
 * @info: pointer to the current request entry info struct
 * @prev: pointer to the previous element in the list
 * @next: pointer to the next element in the list
 */
typedef struct diag_pkt_rsp_tracking_tbl {
	diag_pkt_tracking_info *info;
	struct diag_pkt_rsp_tracking_tbl *prev;
	struct diag_pkt_rsp_tracking_tbl *next;
} diag_pkt_rsp_tracking_tbl;

/* This is a bit mask used for peripheral list. */
typedef uint16 diag_dci_peripherals;
#define DIAG_CON_APSS (0x0001)   /* Bit mask for APSS */
#define DIAG_CON_MPSS (0x0002)   /* Bit mask for MPSS */
#define DIAG_CON_LPASS (0x0004) /* Bit mask for LPASS */
#define DIAG_CON_WCNSS (0x0008) /* Bit mask for WCNSS */

/*
 * The status bit masks when received in a signal handler are to be
 * used in conjunction with the peripheral list bit mask to determine the
 * status for a peripheral. For instance, 0x00010002 would denote an open
 * status on the MPSS
 */
#define DIAG_STATUS_OPEN (0x00010000)	/* Bit mask for DCI channel open status   */
#define DIAG_STATUS_CLOSED (0x00020000)	/* Bit mask for DCI channel closed status */

#define ENABLE			1
#define DISABLE			0
#define IN_BUF_SIZE		16384
#define DCI_PKT_RSP_TYPE	0
#define DCI_LOG_TYPE		-1
#define DCI_EVENT_TYPE		-2

#define DIAG_INVALID_SIGNAL	0

/* This is used for querying DCI Log
   or Event Mask */
struct diag_log_event_stats {
	uint16 code;
	int is_set;
};

/*
 * Structure to hold DCI client information
 *
 * @client_info: basic information used for registration
 * @func_ptr_logs: callback function for streaming logs
 * @func_ptr_events: callback function for streaming events
 * @data_signal_flag: flag for signaling client whenever there is incoming data
 * @data_signal_type: singal to be fired when there in incoming data
 * @req_tbl_head: list of all the active command requests
 * @req_tbl_mutex: mutex to protect request table list
 */
struct diag_dci_client_tbl {
	struct task_struct *client;
	int client_id;
	diag_dci_peripherals list;
	int signal_type;
	unsigned char dci_data[IN_BUF_SIZE];
	int data_len;
	/* Callback functions for logs & events. The input parameters will be
	   buffer pointer, length */
	void (*func_ptr_logs)(unsigned char *, int len);
	void (*func_ptr_events)(unsigned char *, int len);
	int data_signal_flag;
	int data_signal_type;
	struct diag_pkt_rsp_tracking_tbl req_tbl_head;
	pthread_mutex_t req_tbl_mutex;
} *dci_client_tbl;

/* This is used for health stats */
struct diag_dci_health_stats {
	int client_id;
	int dropped_logs;
	int dropped_events;
	int received_logs;
	int received_events;
	int reset_status;
};

/* List of possible error codes returned during DCI transaction */
enum {
	DIAG_DCI_NO_ERROR = 1001,	/* No error */
	DIAG_DCI_NO_REG,		/* Could not register */
	DIAG_DCI_NO_MEM,		/* Failed memory allocation */
	DIAG_DCI_NOT_SUPPORTED,		/* This particular client is not supported */
	DIAG_DCI_HUGE_PACKET,		/* Request/Response Packet too huge */
	DIAG_DCI_SEND_DATA_FAIL,	/* writing to kernel or remote peripheral fails */
	DIAG_DCI_ERR_DEREG,		/* Error while de-registering */
	DIAG_DCI_PARAM_FAIL		/* Incorrect Parameter */
} diag_dci_error_type;

/* Internal function used to parse each and every line in the DCI Input file */
int dci_parse_line(char *str, int *client_id, diag_dci_peripherals *list);

/* Internal function - handler for response; prints the response packet to the file */
void process_response(unsigned char *ptr, int len, void *data_ptr);

/* Internal function to convert a string to an Interger - Takes care of
Hexadecimal numbers too */
int to_integer(char *str);

/* Internal function - handler for logs; prints the log packet to the file */
void process_dci_log_stream(unsigned char *ptr, int len);

/* Internal function - handler for events; prints the event packet to the file */
void process_dci_event_stream(unsigned char *ptr, int len);

/* ---------------------------------------------------------------------------
			External functions
 --------------------------------------------------------------------------- */

/* Initialization function required for DCI functions. Input parameters are:
   a) pointer to an int which holds client id
   b) pointer to a bit mask which holds peripheral information,
   c) an int to specify which diag instance to use (to be used for fusion targets, not implemented as of now),
   d) void* for future needs (not implemented as of now) */
int diag_register_dci_client(int *, diag_dci_peripherals *, int, void *);

/* This provides information on which peripherals are supporting DCI
   User needs to pass a pointer to the bit mask described above */
int diag_get_dci_support_list(diag_dci_peripherals *);

/* Main command to send the DCI request. Input parameters are:
   a) client ID generate earlier
   b) request buffer
   c) request buffer length
   d) response buffer
   e) response buffer length
   f) call back function pointer
   g) data pointer */
int diag_send_dci_async_req(int client_id, unsigned char buf[], int bytes, unsigned char *rsp_ptr, int rsp_len,
			    void (*func_ptr)(unsigned char *ptr, int len, void *data_ptr), void *data_ptr);

/* Closes DCI connection for this client. The client needs to pass a pointer
   to the client id generated earlier */
int diag_release_dci_client(int *);

/* Used to set up log streaming to the client. This will send an array of log codes, which are desired
   by client. Input parameters are:
   1. Client ID
   2. Boolean value telling to set or disable logs specified later
   3. Array of log codes
   4. Number of log codes specified in argument 3
   */
int diag_log_stream_config(int client_id, int set_mask, uint16 log_codes_array[], int num_codes);

/* Initialization function required for DCI streaming. Input parameters are:
   call back function pointer */
int diag_register_dci_stream(void (*func_ptr_logs)(unsigned char *ptr, int len),
			     void (*func_ptr_events)(unsigned char *ptr, int len));

/* Used to set up event streaming to the client. This will send an array of event ids, which are desired
   by client. Input parameters are:
   1. Client ID
   2. Boolean value telling to set or disable event specified later
   3. Array of event id
   4. Number of event ids specified in argument 3
   */
int diag_event_stream_config(int client_id, int set_mask, int event_id_array[], int num_codes);

/* This will get current DCI statistics for logs & events */
int diag_get_health_stats(struct diag_dci_health_stats *dci_health);

/* Queries a given Log Code to check if it is enabled or not. Input parameters are:
   1. Client ID
   2. Log Code that needs to be checked
   3. Pointer to boolean to store the result */
int diag_get_log_status(int client_id, uint16 log_code, boolean *value);

/* Queries a given Event ID to check if it is enabled or not. Input parameters are:
   1. Client ID
   2. Event ID that needs to be checked
   3. Pointer to boolean to store the result */
int diag_get_event_status(int client_id, uint16 event_id, boolean *value);

/* Disables all the Log Codes for a given client. The Input parameters are:
   1. Client ID */
int diag_disable_all_logs(int client_id);

/* Disables all the Event ID for a given client. The Input parameters are:
   1. Client ID */
int diag_disable_all_events(int client_id);

/* Votes for real time or non real time mode. The Input paramters are:
   1. Client ID
   2. The desired mode - MODE_REALTIME or MODE_NONREALTIME */
int diag_dci_vote_real_time(int client_id, int real_time);

/* Gets the current mode (Real time or Non Real Time ) Diag is in.
   The Input parameters are:
   1. A pointer to an integer that will hold the result */
int diag_dci_get_real_time_status(int *real_time);

/* Registers a signal to be fired on receiving DCI data from
   the peripherals. The input parameters are:
   1. Client ID
   2. Signal Type */
int diag_register_dci_signal_data(int client_id, int signal_type);

/* Disables the signal that fires on receiving DCI data from
   the peripherals. The input parameters are:
   1. Client ID
   2. Signal Type */
int diag_deregister_dci_signal_data(int client_id);

#ifdef __cplusplus
}
#endif

#endif /* DIAG_LSM_DCI_H */

