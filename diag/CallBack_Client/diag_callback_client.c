/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

		Test Application for Diag Callback Client

GENERAL DESCRIPTION
  Contains main implementation of Diagnostic Services Application for UART.

EXTERNALIZED FUNCTIONS
  None

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2012-2013 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

		EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "malloc.h"
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include "msg.h"
#include "diag_lsm.h"
#include "stdio.h"
#include "diagpkt.h"
#include "diag_lsmi.h"
#include "diag_shared_i.h"

FILE *op_file = NULL; /* File to drain the callback client output */

/* ---------------------------------------------------------------------------
	Callback functions
 --------------------------------------------------------------------------- */
/* Callback for the primary processor */
int process_diag_data(unsigned char *ptr, int len, void *context_data)
{
	int i;
	fprintf(op_file, "From the primary proc %d\n", *(int *)context_data);
	DIAG_LOGE("\n From primary proc %d\n", *(int *)context_data);
	for (i = 0; i < len; i++) {
		DIAG_LOGE("%02x\t", ptr[i]);
		fprintf(op_file, "%02x\t", ptr[i]);
	}
	DIAG_LOGE("\n\n");
	fprintf(op_file, "\n\n");
	return 0;
}

/* Callback for the remote processor */
int process_remote_diag_data(unsigned char *ptr, int len, void *context_data)
{
	int i;
	DIAG_LOGE("\n From remote proc %d\n", *(int *)context_data);
	fprintf(op_file, "From the remote proc %d\n", *(int *)context_data);
	for (i = 0; i < len; i++) {
		DIAG_LOGE("%02x\t", ptr[i]);
		fprintf(op_file, "%02x\t", ptr[i]);
	}
	DIAG_LOGE("\n\n");
	fprintf(op_file, "\n\n");
	return 0;
}

int main(int argc, char *argv[])
{
	boolean bInit_Success = FALSE;
	int data_primary = 100;
	int data_remote = 200;
	int i = 0;

	bInit_Success = Diag_LSM_Init(NULL);
	if (!bInit_Success) {
		DIAG_LOGE("Diag LSM Init failed, Exiting... err:%d", errno);
		exit(0);
	}

	/* Wake lock to keep the Application processor alive - disabling power collapse */
	system("echo 'diag_callback' > /sys/power/wake_lock");

	op_file = fopen("/system/bin/call_op.txt", "w");
	if(!op_file) {
		DIAG_LOGE("diag: Cannot open file, err: %d\n", errno);
		exit(0);
	}
	/* Setting the output file buffer to flush whenever there is a new line
	   or the max size is reached 4096 */
	setvbuf(op_file , NULL , _IOLBF , 4096);
	setvbuf(stdout , NULL , _IOLBF , 4096);

	/* Register the callback for the primary processor */
	diag_register_callback(&process_diag_data, &data_primary);

	/* Register the callback for the remote processor - MDM in this case */
	diag_register_remote_callback(&process_remote_diag_data, MDM, &data_remote);

	/* This is required to get the data through this app */
	DIAG_LOGE("diag: register callback\n");
	diag_switch_logging(CALLBACK_MODE, NULL);

	/* This is meant for MSMs */
	DIAG_LOGE(" Sending Diag Stress Test command for 100K Iterations\n");
	unsigned char buf[24] = { 75, 18, 0, 0, 1, 0, 0, 0, 16, 1, 255, 255, 0xA0, 0x86, 1, 0, 100, 0, 0, 0, 200, 0, 0, 0 };
	diag_callback_send_data(MSM, buf, 24);

	/* This is meant for MDMs and will result in an error for MSM only config */
	DIAG_LOGE(" Sending NV Write Command - Writing value 13 NV Item 10\n");
	unsigned char nv_write[134];
	nv_write[0] = 0x27;
	nv_write[1] = 0x0A;
	nv_write[2] = 0;
	nv_write[3] = 0;
	nv_write[4] = 0x0D;
	for (i = 5; i < 134; i++)
		nv_write[i] = 0;
	diag_callback_send_data(MDM, nv_write, 134);

	/* This is meant for MDMs and will result in an error for MSM only config */
	DIAG_LOGE(" Sending NV Read Command - Reading NV Item 10\n");
	unsigned char nv_read[134];
	nv_read[0] = 0x26;
	nv_read[1] = 0x0A;
	for (i = 2; i < 134; i++)
		nv_read[i] = 0;
	diag_callback_send_data(MDM, nv_read, 134);

	sleep(2);

	/* Enable Independent SSR - This will return an error on MSMs */
	DIAG_LOGE(" Enabling Independent SSR\n");
	system("echo 3 > /sys/module/subsystem_restart/parameters/restart_level");

	/* This is meant for MDMs and will result in an error for MSM only config */
	DIAG_LOGE(" Sending Software Error Fatal SSR command\n");
	unsigned char ssr1[4] = {75, 37, 3, 0};
	diag_callback_send_data(MDM, ssr1, 4);
	sleep(20);

	/* This is meant for MDMs and will result in an error for MSM only config */
	DIAG_LOGE(" Sending Software Exception SSR command\n");
	unsigned char ssr2[5] = {75, 37, 3, 0, 3};
	diag_callback_send_data(MDM, ssr2, 5);
	sleep(20);

	/* This is meant for MDMs and will result in an error for MSM only config */
	DIAG_LOGE(" Sending Software Exception NULL ptr SSR command\n");
	unsigned char ssr3[5] = {75, 37, 3, 0, 2};
	diag_callback_send_data(MDM, ssr3, 5);
	sleep(20);

	/* This is meant for MDMs and will result in an error for MSM only config */
	DIAG_LOGE(" Sending Watchdog Bite SSR command\n");
	unsigned char ssr4[5] = {75, 37, 3, 0, 1};
	diag_callback_send_data(MDM, ssr4, 5);
	sleep(20);

	/* This is meant for MSMs and may result in an error for MSM+MDM config */
	DIAG_LOGE(" Reading NV Item 10 after the restart\n");
	diag_callback_send_data(MSM, nv_read, 134);

	while (1) {
		/* Infinte loop to keep the app awake
		   to receive data */
	}

	/* Release the handle to Diag*/
	Diag_LSM_DeInit();
	fclose(op_file);

	/* Releasing the wakelock */
	system("echo 'diag_callback' > /sys/power/wake_unlock");
	return 0;
}
