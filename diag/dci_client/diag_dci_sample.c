
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

		Sample Application for Diag Consumer Interface

GENERAL DESCRIPTION
This is a sample application to demonstrate using Diag Consumer Interface APIs.

EXTERNALIZED FUNCTIONS
  None

INITIALIZATION AND SEQUENCING REQUIREMENTS


Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.

Author: Ravi Aravamudhan
Source: Jamila Murabbi
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================*/

#include "event.h"
#include "msg.h"
#include "log.h"
#include "diag_lsm.h"
#include "diag_lsmi.h"
#include "diag_lsm_dci.h"
#include "diag_shared_i.h"
#include "stdio.h"
#include "string.h"
#include "malloc.h"
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>

#define TOTAL_LOG_CODES 12
#define DIAG_SAMPLE_SIGNAL	SIGRTMIN + 15

/* Structure for Log packet parsing*/
struct log_code_name_type{
	int log_code;
	char *name;
};

/* Set a sample log codes */
struct log_code_name_type log_codes[TOTAL_LOG_CODES] = {
	{0x5072, "GSM L1 Serving Cell Info"},{0x12E8, "Transceiver Resource Manager"},{0x119B, "Srch TNG 1x Searcher"},
	{0x11AF, "Srch Finger Driver Dump"},{0x115F, "DIAG TEST LOG"},{0x14C8,"CPU MPNITOR MODEM"},{0x1375,"CGPS IPC DATA"},
	{0x158C,"XOADC READING LOG"},{0x14D8,"TEMP_MONITOR_LOG"},{0x117b, "WMS Set Routes"},{0x11EB, "DPL Log Messages"},
	{0x1160, "WMS Read"}
};

/* Signal handler that handles the change in DCI channel */
void notify_handler(int signal, siginfo_t *info, void *unused)
{
	if (info) {
		int err;
		diag_dci_peripherals list = 0;

		DIAG_LOGE("diag: In %s, signal %d received from kernel, data is: %x\n",
			__func__, signal, info->si_int);

		if (info->si_int & DIAG_STATUS_OPEN) {
			if (info->si_int & DIAG_CON_MPSS) {
				DIAG_LOGE("diag: DIAG_STATUS_OPEN on DIAG_CON_MPSS\n");
			} else {
				DIAG_LOGE("diag: DIAG_STATUS_OPEN on unknown peripheral\n");
			}
		} else if (info->si_int & DIAG_STATUS_CLOSED) {
			if (info->si_int & DIAG_CON_MPSS) {
				DIAG_LOGE("diag: DIAG_STATUS_CLOSED on DIAG_CON_MPSS\n");
			} else {
				DIAG_LOGE("diag: DIAG_STATUS_CLOSED on unknown peripheral\n");
			}
		}
		err = diag_get_dci_support_list(&list);
		if(err != DIAG_DCI_NO_ERROR) {
			DIAG_LOGE("diag: could not get support list, err: %d\n", err);
		}
		/* This will print out all peripherals supporting DCI */
		if (list & DIAG_CON_MPSS)
			DIAG_LOGE("diag: Modem supports DCI\n");
		if (list & DIAG_CON_LPASS)
			DIAG_LOGE("diag: LPASS supports DCI\n");
		if (list & DIAG_CON_WCNSS)
			DIAG_LOGE("diag: RIVA supports DCI\n");
		if (list & DIAG_CON_APSS)
			DIAG_LOGE("diag: APSS supports DCI\n");
		if (!list)
			DIAG_LOGE("diag: No current dci support\n")
	} else {
		DIAG_LOGE("diag: In %s, signal %d received from kernel, but no info value, info: 0x%x\n",
			__func__, signal, (unsigned int)info);
	}
}

/* Singal Handler that will be fired when we receive DCI data */
void dci_data_handler(int signal)
{
	/* Do something here when you receive DCI data. */

	/* This is usually for holding wakelocks when the
	   clients are running in Diag Non Real Time mode
	   or when they know the Apps processor is in deep
	   sleep but they still need to process DCI data.

	   Please Note: Wakelocks must be released
	   after processing the data in the respective
	   response/log/event handler. Failure to do so
	   will affect the power consumption of the Apps
	   processor. */
}

void process_dci_log_stream(unsigned char *ptr, int len)
{
	int i;
	for (i = 0; i < TOTAL_LOG_CODES; i++) {
		if (*(uint16*)(ptr+2) == log_codes[i].log_code) {
			DIAG_LOGE("  Received a Log of type %s, length = %d\n", log_codes[i].name, len);
			break;
		}
	}
	/* Uncomment to Print the logs */
	/*
	i = 0;
	while (i < len) {
		if(i % 8 == 0)
			printf("\n");
		DIAG_LOGE("  %02x  ", ptr[i]);
		i++;
	}
	printf("\n");
	*/
}

void process_dci_event_stream(unsigned char *ptr, int len)
{
	DIAG_LOGE(" Received an event of size %d bytes\n", len);
	/* Uncomment to Print the Events */
	/*
	i = 0;
	while (i < len) {
		if(i % 8 == 0)
			printf("\n");
		DIAG_LOGE("  %02x  ", ptr[i]);
		i++;
	}
	printf("\n");
	*/
}

void process_response(unsigned char *ptr, int len, void *data_ptr)
{
	int i = 0;
	DIAG_LOGE(" Received Response of size %d bytes.\n", len);
	while (i < len) {
		if(i % 8 == 0)
			printf("\n");
		DIAG_LOGE("  %02x  ", ptr[i]);
		i++;
	}
	printf("\n");

	/* Parsing Logic for the response - Based on the request in this sample */
	if (ptr[0] == 75)
		DIAG_LOGE("  Command Code:   Diag\n");
	if (ptr[1] == 18)
		DIAG_LOGE("  Subsystem ID:   Diag\n");
	if (ptr[8] <= 9) {
		DIAG_LOGE("  Test for:       F3s\n");
	} else if (ptr[8] == 16) {
		DIAG_LOGE("  Test for:       Logs\n");
	} else if (ptr[8] == 17) {
		DIAG_LOGE("  Test for:       Events without payload\n");
	} else if (ptr[8] == 18) {
		DIAG_LOGE("  Test for:       Events with payload\n");
	}
	printf("\n");
}

/* Main Function. This demonstrates using the DCI APIs defined in diag_lsm_dci.h*/
int main(int argc, char *argv[])
{
	int err, client_id;
	int signal_type = SIGCONT, channel = 0;
	boolean bInit_success = FALSE;
	diag_dci_peripherals list = DIAG_CON_MPSS | DIAG_CON_APSS;
	struct diag_dci_health_stats *dci_health_stats;	/* To collect DCI Health Statistics */
	unsigned char *dci_rsp_pkt = NULL;

	/* Signal handling to handle SSR */
	struct sigaction notify_action;
	sigemptyset(&notify_action.sa_mask);
	notify_action.sa_sigaction = notify_handler;
	/* Use SA_SIGINFO to denote we are expecting data with the signal */
	notify_action.sa_flags = SA_SIGINFO;
	sigaction(signal_type, &notify_action, NULL);

	/* Signal handling for DCI Data */
	struct sigaction dci_data_action;
	sigemptyset(&dci_data_action.sa_mask);
	dci_data_action.sa_handler = dci_data_handler;
	sigaction(DIAG_SAMPLE_SIGNAL, &dci_data_action, NULL);

	/* Registering with Diag which gives the client a handle to the Diag driver */
	bInit_success = Diag_LSM_Init(NULL);
	if (!bInit_success) {
		DIAG_LOGE(" Couldn't register with Diag LSM, errno: %d\n", errno);
		return -1;
	}

	dci_rsp_pkt = (unsigned char *)malloc(4096);
	if (!dci_rsp_pkt) {
		DIAG_LOGE("  Unable to allocate memory for DCI rsp pkt, errno: %d", errno);
		return -1;
	}

	/* Registering with DCI - This assigns a client ID */
	err = diag_register_dci_client(&client_id, &list, channel, &signal_type);
	if (err != DIAG_DCI_NO_ERROR) {
		DIAG_LOGE(" Could not register with DCI, err: %d, errno: %d\n", err, errno);
		free(dci_rsp_pkt);
		return -1;
	} else
		DIAG_LOGE(" Successfully registered with DCI, client ID = %d\n", client_id);

	err = diag_register_dci_signal_data(client_id, DIAG_SAMPLE_SIGNAL);
	if (err != DIAG_DCI_NO_ERROR) {
		DIAG_LOGE(" Could not register signal for DCI Data, err: %d, errno: %d\n", err, errno);
		free(dci_rsp_pkt);
		return -1;
	}

	/* Getting supported Peripherals list*/
	DIAG_LOGE(" DCI Status on Processors:\n");
	err = diag_get_dci_support_list(&list);
	if (err != DIAG_DCI_NO_ERROR) {
		printf(" Could not get support list, err: %d, errno: %d\n", err, errno);
		free(dci_rsp_pkt);
		return -1;
	}
	DIAG_LOGE("   MPSS:\t ");
	DIAG_LOGE((list & DIAG_CON_MPSS) ? "UP\n" : "DOWN\n");
	DIAG_LOGE("   LPASS:\t ");
	DIAG_LOGE((list & DIAG_CON_LPASS) ? "UP\n" : "DOWN\n");
	DIAG_LOGE("   WCNSS:\t ");
	DIAG_LOGE((list & DIAG_CON_WCNSS) ? "UP\n" : "DOWN\n");
	DIAG_LOGE("   APSS:\t ");
	DIAG_LOGE((list & DIAG_CON_APSS) ? "UP\n" : "DOWN\n");


	/* Initializing Log and Event streaming by registering
	   listeners - This is required to receive Logs and Events */
	DIAG_LOGE(" Opening Data Stream, registering listeners\n");
	err = diag_register_dci_stream(process_dci_log_stream, process_dci_event_stream);
	if (err != DIAG_DCI_NO_ERROR)
		DIAG_LOGE(" Unable to register DCI stream, err: %d, errno: %d\n", err, errno);

	printf("\n");

	DIAG_LOGE(" Sending Asynchronous Command \n");
	/* Diag Log stress test command - one Log packet comes out every second */
	unsigned char buf[24] = {75, 18, 0, 0, 1, 0, 0, 0, 16, 1, 1, 0, 0, 1, 0, 0, 232, 3, 0, 0, 1, 0, 0, 0};
	err = diag_send_dci_async_req(client_id, buf, 24, dci_rsp_pkt, 100, &process_response, NULL);
	if (err != DIAG_DCI_NO_ERROR)
		DIAG_LOGE(" Error sending SEND_REQUEST_ASYNC to peripheral %d\n", err);

	sleep(1);
	printf("\n");

	/* Setting Log masks ----------------------------
	   0x5072 - GSM L1 Serving Cell Info
	   0x115F - DIAG TEST LOG
	   0x12E8 - Transceiver Resource Manager
	   0x119B - Srch TNG 1x Searcher
	   0x11AF - Srch Finger Driver Dump
	   0x14C8 - CPU MPNITOR MODEM
	   0x1375 - CGPS IPC DATA
	   -------------------------------------------*/
	DIAG_LOGE(" Setting Log masks\n");
	DIAG_LOGE("   [0x5072] GSM L1 Serving Cell Info\n");
	DIAG_LOGE("   [0x115F] DIAG TEST LOG\n");
	DIAG_LOGE("   [0x12E8] Transceiver Resource Manager\n");
	DIAG_LOGE("   [0x119B] Srch TNG 1x Searcher\n");
	DIAG_LOGE("   [0x11AF] Srch Finger Driver Dump\n");
	DIAG_LOGE("   [0x14C8] CPU MPNITOR MODEM\n");
	DIAG_LOGE("   [0x1375] CGPS IPC DATA\n");
	uint16	log_codes_array[7] = {0x5072, 0x115F, 0x12E8, 0x119B, 0x11AF, 0x14C8, 0x1375};
	err = diag_log_stream_config(client_id, ENABLE, log_codes_array, 7);
	if (err != DIAG_DCI_NO_ERROR)
		DIAG_LOGE(" Error sending SET_LOG_MASK to peripheral, err: %d, errno: %d\n", err, errno);

	/* Putting the thread to sleep for 30 seconds. The client must sleep for the time it
	   wants to receive logs/events/command responses */
	sleep(30);
	printf("\n");

	/* Display Health Statistics */
	DIAG_LOGE(" Stream Health statistics on Applications processor:\n");
	dci_health_stats = malloc(sizeof(struct diag_dci_health_stats));
	if (!dci_health_stats) {
		DIAG_LOGE("  Unable to allocate memory for DCI health stats, errno: %d", errno);
		free(dci_rsp_pkt);
		return -1;
	}

	err = diag_get_health_stats(dci_health_stats);
	if (err == DIAG_DCI_NO_ERROR) {
		DIAG_LOGE("   Log Drop Count:\t%d\n", dci_health_stats->dropped_logs);
		DIAG_LOGE("   Log Total Count:\t%d\n", dci_health_stats->received_logs);
		DIAG_LOGE("   Event Drop Count:\t%d\n", dci_health_stats->dropped_events);
		DIAG_LOGE("   Event Total Count:\t%d\n", dci_health_stats->received_events);
	} else
		DIAG_LOGE("   Error in collecting statistics, err: %d, errno: %d\n", err, errno);

	sleep(1);
	printf("\n");

	/* Clearing these log masks ------------------------
	   0x115F - DIAG TEST LOG: This packet is supposed to come out every second if its mask is set!
	   0x12E8 - Transceiver Resource Manager
	   0x1375 - CGPS IPC DATA
	   ---------------------------------------------------- */
	DIAG_LOGE(" Clearing log masks\n");
	DIAG_LOGE("   [0x115F] DIAG TEST LOG\n");
	DIAG_LOGE("   [0x12E8] Transceiver Resource Manager\n");
	DIAG_LOGE("   [0x1375] CGPS IPC DATA\n");
	uint16	log_codes_array2[3] = {0x115F, 0x1375};
	err = diag_log_stream_config(client_id, DISABLE, log_codes_array2, 3);
	if (err != DIAG_DCI_NO_ERROR)
		DIAG_LOGE(" Error sending CLEAR_LOG_MASK to peripheral, err: %d, errno: %d\n", err, errno);

	sleep(5);
	printf("\n");
	// ----------------

	/* Setting Log masks ----------------------------
	   0x117B - WMS Set Routes
	   0x11EB - DPL Log Messages
	   0x1160 - WMS Read
	   -------------------------------------------*/
	DIAG_LOGE(" *** To generate the following logs on APSS, please run the test_diag app\n");
	DIAG_LOGE(" Setting Log masks\n");
	DIAG_LOGE("   [0x117B] WMS Set Routes\n");
	DIAG_LOGE("   [0x11EB] DPL Log Messages\n");
	DIAG_LOGE("   [0x1160] WMS Read\n");

	uint16	log_codes_array_apps[3] = {0x117B, 0x11EB, 0x1160};
	err = diag_log_stream_config(client_id, ENABLE, log_codes_array_apps, 3);
	if (err != DIAG_DCI_NO_ERROR)
		DIAG_LOGE(" Error sending SET_LOG_MASK to peripheral, err: %d, errno: %d\n", err, errno);

	/* Putting the thread to sleep for 30 seconds. The client must sleep for the time it
	   wants to receive logs/events/command responses */
	sleep(30);
	printf("\n");

	/* Display Health Statistics */
	DIAG_LOGE(" Stream Health statistics on Applications processor:\n");
	if (!dci_health_stats)
		dci_health_stats = malloc(sizeof(struct diag_dci_health_stats));
	if (!dci_health_stats) {
		DIAG_LOGE("  Unable to allocate memory for DCI health stats, errno: %d", errno);
		free(dci_rsp_pkt);
		return -1;
	}

	err = diag_get_health_stats(dci_health_stats);
	if (err == DIAG_DCI_NO_ERROR) {
		DIAG_LOGE("   Log Drop Count:\t%d\n", dci_health_stats->dropped_logs);
		DIAG_LOGE("   Log Total Count:\t%d\n", dci_health_stats->received_logs);
		DIAG_LOGE("   Event Drop Count:\t%d\n", dci_health_stats->dropped_events);
		DIAG_LOGE("   Event Total Count:\t%d\n", dci_health_stats->received_events);
	} else
		DIAG_LOGE("   Error in collecting statistics, err: %d, errno: %d\n", err, errno);

	sleep(1);
	printf("\n");

	/* Clearing these log masks ------------------------
	   0x115F - DIAG TEST LOG: This packet is supposed to come out every second if its mask is set!
	   0x12E8 - Transceiver Resource Manager
	   0x1375 - CGPS IPC DATA
	   ---------------------------------------------------- */
	DIAG_LOGE(" Clearing log masks\n");
	DIAG_LOGE("   [0x117B] WMS Set Routes\n");
	DIAG_LOGE("   [0x11EB] DPL Log Messages\n");
	DIAG_LOGE("   [0x1160] WMS Read\n");
	uint16	log_codes_array2_apps[3] = {0x117B, 0x11EB, 0x1160};
	err = diag_log_stream_config(client_id, DISABLE, log_codes_array2_apps, 3);
	if (err != DIAG_DCI_NO_ERROR)
		DIAG_LOGE(" Error sending CLEAR_LOG_MASK to peripheral, err: %d, errno: %d\n", err, errno);

	sleep(5);
	printf("\n");

	/* Releasing DCI connection */
	DIAG_LOGE(" Releasing DCI connection \n");
	err = diag_release_dci_client(&client_id);
	if (err != DIAG_DCI_NO_ERROR)
		DIAG_LOGE(" Error releasing DCI connection, err: %d, errno: %d\n", err, errno);

	Diag_LSM_DeInit();
	printf("\n");
	free(dci_health_stats);
	free(dci_rsp_pkt);
	return 0;
}
