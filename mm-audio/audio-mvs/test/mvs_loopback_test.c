/*
 * Copyright (c) 2008, 2010-2012 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifdef USE_MVS_DRIVER

#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/msm_audio_mvs.h>
#include "control.h"
#include <getopt.h>

#define TEST_LOOP_COUNTER 500
#define SND_DEVICE_HANDSET 0
#define MAX_STRING 10
static void Display_Help(void);
int devidtable[2] = {0, 6};

/* Usage: mm-audio-mvs-test <mvs_mode> <rate_type>
 * Refer msm_audio_mvs.h for supported modes and rates. */
int main(int argc, char **argv) {
	int mvsDvrFd = NULL;
	int ret = -1;
	int opt = 0;
	int option_index = 0;
	int mvs_mode = 0;
	int rate_type = -1;
	int dtx_mode = -1;
	int dev_id = 0; /*handset is the default device*/

	struct option long_options[] = {
		{"vocoder-type",     required_argument,    0, 'v'},
		{"vocoder-config",   required_argument,    0, 'c'},
		{"dtx-mode",       required_argument,    0, 'd'},
		{"deviceid",       required_argument,    0, 'i'},
		{"help",            no_argument,          0, 'h'},
		{0, 0, 0, 0}
	};

	if (argc == 1) {
		printf("Vocoder-Type needed, please use --help for usage\n\n");
		goto EXIT;
	}

	while ((opt = getopt_long(argc, argv, "-v:c:d:i:h", long_options, &option_index)) != -1)
	{
		switch (opt) {
		case 'v':
			mvs_mode = atoi(optarg);
			if ((mvs_mode < MVS_MODE_IS733) ||
				(mvs_mode > MVS_MODE_PCM_WB)) {
				printf("\nUnsupported Vocoder %d please use --help for info\n\n", mvs_mode);
				goto EXIT;
			}

			break;
		case 'c':
			rate_type = atoi(optarg);
			break;
		case 'p':
			dtx_mode = atoi(optarg);
			break;
		case 'i':
			dev_id = atoi(optarg);
			if (dev_id != 0 && dev_id != 1) {
				printf("Unsupported dev id %d please use --help for info\n\n", dev_id);
				goto EXIT;
			}
			break;
		case 'h':
			Display_Help();
			goto EXIT;
		}
	}

	switch (mvs_mode) {
	case MVS_MODE_G711A:
		if (-1 == rate_type) {
			rate_type = MVS_G711A_MODE_MULAW;
			printf("\n%d:Mode not set, Using default value\n", __LINE__);
		} else if ((rate_type < MVS_G711A_MODE_MULAW)
			&& (rate_type > MVS_G711A_MODE_ALAW)) {
			rate_type = MVS_G711A_MODE_MULAW;
			printf("\n%d:In correct Config ,Using default value\n", __LINE__);
		}
		if (-1 == dtx_mode)
			dtx_mode = 1;
		break;

	case MVS_MODE_G711:
		if (-1 == rate_type) {
			rate_type = MVS_G711_MODE_MULAW;
			printf("\n%d:Mode not set, Using default value\n", __LINE__);
		} else if ((rate_type < MVS_G711_MODE_MULAW)
			&& (rate_type > MVS_G711_MODE_ALAW)) {
			rate_type = MVS_G711_MODE_MULAW;
			printf("\n%d:In correct config ,Using default value\n", __LINE__);
		}
		if (-1 == dtx_mode)
			dtx_mode = 1;
		break;

	case MVS_MODE_IS733:
	case MVS_MODE_IS127:
	case MVS_MODE_4GV_NB:
	case MVS_MODE_4GV_WB:
		if (-1 == rate_type) {
			rate_type = MVS_VOC_1_RATE;
			printf("\n%d:Mode not set, Using default value\n", __LINE__);
		} else if ((rate_type < MVS_VOC_0_RATE)
			|| (rate_type >= MVS_VOC_RATE_MAX)) {
			rate_type = MVS_VOC_1_RATE;
			printf("\n%d:In correct config ,Using default value\n", __LINE__);
		}
		break;
	case MVS_MODE_AMR:
		if (-1 == rate_type) {
			rate_type = MVS_AMR_MODE_0590;
			printf("\n%d:Mode not set, Using default value\n", __LINE__);
		} else if ((rate_type < MVS_AMR_MODE_0475)
			|| (rate_type > MVS_AMR_MODE_1220)) {
			rate_type = MVS_AMR_MODE_1220;
			printf("\n%d:In correct config ,Using default value\n", __LINE__);
		}
		break;
	case MVS_MODE_AMR_WB:
		if (-1 == rate_type) {
			rate_type = MVS_AMR_MODE_1825;
			printf("\n%d:Mode not set, Using default value\n", __LINE__);
		} else if ((rate_type < MVS_AMR_MODE_0660)
			|| (rate_type > MVS_AMR_MODE_2385)) {
			rate_type = MVS_AMR_MODE_1825;
			printf("\n%d:In correct config ,Using default value\n", __LINE__);
		}
		break;
	}

	printf("\nmvs_mode %d\n", mvs_mode);
	printf("rate_type %d\n", rate_type);
	printf("dtx_mode %d\n\n", dtx_mode);

	/* Setup device and route voice to device. */

#ifdef AUDIOV2
	ret = msm_mixer_open("/dev/snd/controlC0", 0);
	if (ret < 0)
		printf("Error %d opening mixer \n", ret);

	int device_id_rx = msm_get_device("handset_rx");

	int device_id_tx = msm_get_device("handset_tx");

	ret = msm_en_device(device_id_rx, 1);
	if (ret < 0)
		printf("Error %d enabling handset_rx \n", ret);

	ret = msm_en_device(device_id_tx, 1);
	if (ret < 0)
		printf("Error %d enabling handset_tx \n", ret);

	ret = msm_route_voice(device_id_rx, device_id_tx, 1);
	if (ret < 0)
		printf("Error %d routing voice to handset \n", ret);

	ret = msm_start_voice();
	if (ret < 0)
		printf("Error %d starting voice \n", ret);

	ret = msm_set_voice_rx_vol(100);
	if (ret < 0)
		printf("Error %d setting volume \n", ret);

	ret = msm_set_voice_tx_mute(0);
	if (ret < 0)
		printf("Error %d un-muting \n", ret);
#else /*AUDIOV2*/
	struct FILE *fp=fopen("/sys/class/misc/msm_snd/device","wb");
	if(!fp)
		printf("unable to set the device\n");
	else {
		char DeviceSettingString[MAX_STRING];
		int snddevid = devidtable[dev_id];
		snprintf(DeviceSettingString,MAX_STRING,"%d %d %d"
			, snddevid, SND_MUTE_UNMUTED, SND_MUTE_UNMUTED);
		fwrite(DeviceSettingString,strlen(DeviceSettingString),1,fp);
		fclose(fp);
		printf("Device set(%s)\n", DeviceSettingString);
	}
#endif /*AUDIOV2*/

	/* Open MVS driver. */
	mvsDvrFd = open("/dev/msm_mvs", O_RDWR);
	if (mvsDvrFd < 0){
		printf("MVS drvr open failed %d \n", mvsDvrFd);
		goto error;
	}

	struct msm_audio_mvs_config mvs_config;
	mvs_config.mvs_mode = mvs_mode;
	mvs_config.rate_type = rate_type;
	mvs_config.dtx_mode = dtx_mode;

	printf("Configuring MVS for %d Mode\n",mvs_mode);
	ret = ioctl(mvsDvrFd, AUDIO_SET_MVS_CONFIG, &mvs_config);
	printf("MVS ioctl returned %d\n", ret);

	ret = ioctl(mvsDvrFd, AUDIO_START, NULL);
	printf("MVS start returned %d\n", ret);

#ifdef AUDIOV2
	struct q5v2_msm_audio_mvs_frame test_frame;
#else
	struct msm_audio_mvs_frame test_frame;
#endif

	int i = 0;
	while (i < TEST_LOOP_COUNTER) {
		ret = read(mvsDvrFd, &test_frame, sizeof(test_frame));
		usleep(30000); /* should improve the loop back perception */
		if (ret > 0) {
			ret = write(mvsDvrFd, &test_frame, sizeof(test_frame));
			if (ret < 0)
				printf("MVS write returned %d \n", ret);
		} else {
			printf("MVS read returned %d \n", ret);
		}

		i++;
	}

	ret = ioctl(mvsDvrFd, AUDIO_STOP, NULL);
	printf("MVS stop returned %d \n", ret);

	ret = close(mvsDvrFd);
	printf("MVS close returned %d \n", ret);

#ifdef AUDIOV2
	/* Mute and disable the device. */
	ret = msm_set_voice_rx_vol(0);
	if (ret <0)
		printf("Error %d setting volume\n", ret);

	ret = msm_set_voice_tx_mute(1);
	if (ret < 0)
		printf("Error %d setting mute\n", ret);

	ret = msm_end_voice();
	if (ret < 0)
		printf("Error %d ending voice\n", ret);

	ret = msm_en_device(device_id_rx, 0);
	if (ret < 0)
		printf("Error %d disabling handset_rx\n", ret);

	ret = msm_en_device(device_id_tx, 0);
	if (ret < 0)
		printf("Error %d disabling handset_tx\n", ret);

	ret = msm_mixer_close();
	if (ret < 0)
		printf("Error %d closing mixer\n", ret);
#endif /*AUDIOV2*/

error:

if(i == TEST_LOOP_COUNTER)
	printf("*****TEST PASSED*****\n");
else
	printf("*****TEST FAILED*****\n");

EXIT:
	return 0;
}

static void Display_Help(void)
{
	printf(" \n Command \n");
	printf(" \n adb shell mm-audio-mvs-test <Vocoder type> \n");
	printf(" \n Options\n");
	printf(" -v --vocoder-type    - Specifies the Vocoder or MVS mode to be used.\n\n");
	printf("                       1- MVS_MODE_IS733 (QCELP 13K)\n");
	printf("                       2- MVS_MODE_IS127 (EVRC-8k)\n");
	printf("                       3- MVS_MODE_4GV_NB (EVRC-B)\n");
	printf("                       4- MVS_MODE_4GV_WB (EVRC-WB)\n");
	printf("                       5- MVS_MODE_AMR\n");
	printf("                       6- MVS_MODE_EFR\n");
	printf("                       7- MVS_MODE_FR\n");
	printf("                       8- MVS_MODE_HR\n");
	printf("                       9- MVS_MODE_LINEAR_PCM\n");
	printf("                       10- MVS_MODE_G711\n");
	printf("                       12- MVS_MODE_PCM\n");
	printf("                       13- MVS_MODE_AMR_WB\n");
	printf("                       14- MVS_MODE_G729A\n");
	printf("                       15- MVS_MODE_G711A\n");
	printf("                       16- MVS_MODE_G722\n");
	printf("                       18- MVS_MODE_PCM_WB\n\n");
	printf(" -c --vocoder-config  - For different vocoders this field means differnet things\n\n");
	printf("                      - For vocoder-type 1 to 4 it takes max bit rates and below are the possible values\n");
	printf("                       0- MVS_VOC_0_RATE\n");
	printf("                       1- MVS_VOC_8_RATE(default)\n");
	printf("                       2- MVS_VOC_4_RATE\n");
	printf("                       3- MVS_VOC_2_RATE\n");
	printf("                       4- MVS_VOC_1_RATE\n");
	printf("                       5- MVS_VOC_ERASURE\n\n");
	printf("                      - For vocoder-type MVS_MODE_G711 it indicates the mode\n");
	printf("                       0- MVS_G711_MODE_MULAW\n");
	printf("                       1- MVS_G711_MODE_ALAW\n\n");
	printf("                      - For vocoder-type MVS_MODE_G711A it indicates the mode\n");
	printf("                       0- MVS_G711A_MODE_MULAW\n");
	printf("                       1- MVS_G711A_MODE_ALAW\n\n");
	printf("                      - For vocoder-type MVS_MODE_AMR it takes bit rate value and below are the possible values\n");
	printf("                       0- MVS_AMR_MODE_0475\n");
	printf("                       1- MVS_AMR_MODE_0515\n");
	printf("                       2- MVS_AMR_MODE_0590\n");
	printf("                       3- MVS_AMR_MODE_0670\n");
	printf("                       4- MVS_AMR_MODE_0740\n");
	printf("                       5- MVS_AMR_MODE_0795\n");
	printf("                       6- MVS_AMR_MODE_1020\n");
	printf("                       7- MVS_AMR_MODE_1220\n\n");
	printf("                      - For vocoder-type MVS_MODE_AMR_WB  it takes bit rate value and below are the possible values\n");
	printf("                       8- MVS_AMR_MODE_0660\n");
	printf("                       9- MVS_AMR_MODE_0885\n");
	printf("                       10- MVS_AMR_MODE_1265\n");
	printf("                       11- MVS_AMR_MODE_1425\n");
	printf("                       12- MVS_AMR_MODE_1585\n");
	printf("                       13- MVS_AMR_MODE_1825\n");
	printf("                       14- MVS_AMR_MODE_1985\n");
	printf("                       15- MVS_AMR_MODE_2305\n\n");
	printf("                      - For the below vocoder-types this value is not used\n");
	printf("                       MVS_MODE_EFR\n");
	printf("                       MVS_MODE_FR\n");
	printf("                       MVS_MODE_HR\n");
	printf("                       MVS_MODE_G729A\n");
	printf("                       MVS_MODE_G722\n");
	printf("                       MVS_MODE_LINEAR_PCM\n");
	printf("                       MVS_MODE_PCM\n");
	printf("                       MVS_MODE_PCM_WB\n\n");
	printf(" -d --dtx-mode        -Set Dtx Mode, Valid for below vocoder types\n");
	printf("                       MVS_MODE_G711A\n\n");
	printf(" -i --deviceid        -Device(0-handset, 1-speaker)\n\n");
	printf(" \nExamples\n");
	printf(" 1.adb shell mm-audio-mvs-test -v 2 -i 1  - Uses MVS_MODE_IS127(QCELP 13K)\n");
	printf("                                            plays the output on speaker\n");
	printf(" 2.adb shell mm-audio-mvs-test --vocoder-type=5 --vocoder-config=7 --deviceid=0  \n");
	printf("                                          - Uses MVS_MODE_AMR\n");
	printf("                                            at bit rate of MVS_AMR_MODE_1220(12.2kbps)\n");
	printf("                                            plays the output on handset device\n");
	printf(" 1.adb shell mm-audio-mvs-test -v 3 -c 4  - Uses MVS_MODE_4GV_NB(QCELP 13K)\n");
	printf("                                            at MVS_VOC_1_RATE bit rate(full rate)\n");
	printf("                                            plays the output on default device\n");
}

#else /* USE_MVS_DRIVER*/

#include <stdio.h>
#include <getopt.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "comdef.h"

#define SYS_USER_PPLMN_LIST_MAX_LENGTH 85
#define SYS_PLMN_LIST_MAX_LENGTH 40

#include "mvs.h"
#include "mvs_rpc.h"
#include "snd.h"
#include "snd_rpc.h"


#define MVS_TEST_MVS_FRAME_Q_SIZE 100   /* 2 sec buffer */

mvs_frame_type mvs_test_mvs_frame_buf[MVS_TEST_MVS_FRAME_Q_SIZE];
int mvs_test_mvs_delay = 0;
uint8 doNotStop = FALSE;
int interrupt_count = 0;
typedef struct {
	mvs_frame_type *frame_q;
	int q_size;
	int frame_cnt;
	int curr_frame;
	int miss_cnt;
	int lost_cnt;
} mvs_frame_q_type;

mvs_frame_q_type mvs_test_mvs_frame_q;


/*===========================================================================

FUNCTION mvssup_frame_q_init

DESCRIPTION
  This function will initialize MVS voice frame queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void mvssup_frame_q_init(
			mvs_frame_q_type *frame_q_ptr,
			mvs_frame_type *frames_buf,
			int size
			);

/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_q_empty

DESCRIPTION
  This function will emptify MVS voice frame queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void mvssup_frame_q_empty(
			 mvs_frame_q_type *frame_q_ptr
			 );

/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_check_q_full

DESCRIPTION
  This function will check if MVS voice frame queue is full or not.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
boolean mvssup_frame_check_q_full(
				 mvs_frame_q_type *frame_q_ptr
				 );

/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_get_q_size

DESCRIPTION
  This function will return the size of the queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int mvssup_frame_get_q_size(
			   mvs_frame_q_type *frame_q_ptr
			   );

/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_get_q_frame_cnt

DESCRIPTION
  This function will return the no of frame in the queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int mvssup_frame_get_q_frame_cnt(
				mvs_frame_q_type *frame_q_ptr
				);

/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_check_q_empty

DESCRIPTION
  This function will check if MVS voice frame queue is empty or not.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
boolean mvssup_frame_check_q_empty(
				  mvs_frame_q_type *frame_q_ptr
				  );

/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_q_put

DESCRIPTION
  This function will put a frame into the queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
mvs_pkt_status_type mvssup_frame_q_put(
				      mvs_frame_q_type *frame_q_ptr,
				      uint8 *frame_data,
				      mvs_frame_info_type *frame_info,
				      uint16 frame_len
				      );

/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_q_get

DESCRIPTION
  This function will get a frame from the queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
mvs_pkt_status_type mvssup_frame_q_get(
				      mvs_frame_q_type *frame_q_ptr,
				      uint8 *frame_data,
				      mvs_frame_info_type *frame_info,
				      uint16 *frame_len
				      );

/*===========================================================================

FUNCTION mvssup_frame_q_init

DESCRIPTION
  This function will initialize MVS voice frame queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void mvssup_frame_q_init(
			mvs_frame_q_type *frame_q_ptr,
			mvs_frame_type *frames_buf,
			int size
			) {
	if ((frame_q_ptr==NULL) || (frames_buf==NULL) || (size<=0)) {
    //printf("MVS frame q init failed %d", size, 0, 0);
		return;
	}


	frame_q_ptr->frame_q = frames_buf;
	frame_q_ptr->q_size = size;
	frame_q_ptr->frame_cnt = 0;
	frame_q_ptr->curr_frame = 0;
	frame_q_ptr->miss_cnt = 0;
	frame_q_ptr->lost_cnt = 0;
}


/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_q_empty

DESCRIPTION
  This function will emptify MVS voice frame queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void mvssup_frame_q_empty(
			 mvs_frame_q_type *frame_q_ptr
			 ) {
	if (frame_q_ptr!=NULL) {
		frame_q_ptr->frame_cnt = 0;
		frame_q_ptr->curr_frame = 0;
	}
}


/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_check_q_full

DESCRIPTION
  This function will check if MVS voice frame queue is full or not.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
boolean mvssup_frame_check_q_full(
				 mvs_frame_q_type *frame_q_ptr
				 ) {
	if ((frame_q_ptr==NULL) || (frame_q_ptr->frame_q==NULL)
	    || (frame_q_ptr->q_size <= 0)) {
		return TRUE;
	}

	if (frame_q_ptr->frame_cnt >= frame_q_ptr->q_size) {
		return TRUE;
	}

	return FALSE;
}


/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_get_q_size

DESCRIPTION
  This function will return the size of the queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int mvssup_frame_get_q_size(
			   mvs_frame_q_type *frame_q_ptr
			   ) {
	if ((frame_q_ptr==NULL) || (frame_q_ptr->frame_q==NULL)
	    || (frame_q_ptr->q_size <= 0)) {
		return 0;
	}

	return frame_q_ptr->q_size;

}

/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_get_q_frame_cnt

DESCRIPTION
  This function will return the no of frame in the queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
int mvssup_frame_get_q_frame_cnt(
				mvs_frame_q_type *frame_q_ptr
				) {
	if ((frame_q_ptr==NULL) || (frame_q_ptr->frame_q==NULL)
	    || (frame_q_ptr->q_size <= 0)) {
		return 0;
	}

	return frame_q_ptr->frame_cnt;

}

/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_check_q_empty

DESCRIPTION
  This function will check if MVS voice frame queue is empty or not.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
boolean mvssup_frame_check_q_empty(
				  mvs_frame_q_type *frame_q_ptr
				  ) {
	if ((frame_q_ptr==NULL) || (frame_q_ptr->frame_q==NULL)
	    || (frame_q_ptr->q_size <= 0)) {
		return TRUE;
	}

	if (frame_q_ptr->frame_cnt <= 0) {
		return TRUE;
	}

	return FALSE;
}


/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_q_put

DESCRIPTION
  This function will put a frame into the queue.

DEPENDENCIES
  mvssup_frame_q_init must be called first to initialize the queue.

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
mvs_pkt_status_type mvssup_frame_q_put(
				      mvs_frame_q_type *frame_q_ptr,
				      uint8 *frame_data,
				      mvs_frame_info_type *frame_info,
				      uint16 frame_len
				      ) {
	mvs_pkt_status_type status = MVS_PKT_STATUS_NORMAL;
	int tail;


	if (frame_len > MVS_PCM_PKT_SIZE) {
		frame_len = MVS_PCM_PKT_SIZE;
	}


	if ((frame_q_ptr==NULL) || (frame_data==NULL) || (frame_info==NULL)) {
		return MVS_PKT_STATUS_SLOW;    /* slow status indicates program error */
	}

	if ((frame_q_ptr->frame_q == NULL) || (frame_q_ptr->q_size <= 0)) {
    //printf("MVS frame queue needs to be initialised.", 0, 0, 0);
		return MVS_PKT_STATUS_SLOW;    /* slow status indicates program error */
	}

	if (frame_q_ptr->frame_cnt > frame_q_ptr->q_size) {
   /* printf("Unexpected MVS frame queue error %d %d !!", \
	      frame_q_ptr->frame_cnt, frame_q_ptr->q_size, 0);*/
		return MVS_PKT_STATUS_SLOW;
	}


	if (frame_q_ptr->frame_cnt == frame_q_ptr->q_size) {
		frame_q_ptr->frame_cnt--;

		if (frame_q_ptr->frame_cnt == 0) {
			frame_q_ptr->curr_frame = 0;
		} else {
			frame_q_ptr->curr_frame = (frame_q_ptr->curr_frame + 1)
						  % (frame_q_ptr->q_size);
		}

		frame_q_ptr->lost_cnt++;

   /* printf("MVS frame queue overflow %x %d %d", \
	     (uint32)(frame_q_ptr->frame_q), frame_q_ptr->frame_cnt, \
	     frame_q_ptr->lost_cnt);*/

		status = MVS_PKT_STATUS_FAST;
	}


	tail = (frame_q_ptr->curr_frame + frame_q_ptr->frame_cnt)
	       % (frame_q_ptr->q_size);

	memcpy((frame_q_ptr->frame_q + tail)->frame_data, frame_data, frame_len);
	(frame_q_ptr->frame_q + tail)->frame_info = *frame_info;
	(frame_q_ptr->frame_q + tail)->frame_len = frame_len;

	frame_q_ptr->frame_cnt++;

	return status;
}


/* <EJECT> */
/*===========================================================================

FUNCTION mvssup_frame_q_get

DESCRIPTION
  This function will get a frame from the queue.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
mvs_pkt_status_type mvssup_frame_q_get(
				      mvs_frame_q_type *frame_q_ptr,
				      uint8 *frame_data,
				      mvs_frame_info_type *frame_info,
				      uint16 *frame_len
				      ) {
	if ((frame_q_ptr==NULL) || (frame_data==NULL) || (frame_info==NULL)) {
		return MVS_PKT_STATUS_FAST;   /* fast status indicates program error */
	}

	if ((frame_q_ptr->frame_q == NULL) || (frame_q_ptr->q_size <= 0)) {
   /* printf("MVS frame queue needs to be initialised.", 0, 0, 0);*/
		return MVS_PKT_STATUS_FAST;   /* fast status indicates program error */
	}

	if (frame_q_ptr->frame_cnt < 0) {
   /* printf("Unexpected MVS frame queue error %d %d !!", \
	      frame_q_ptr->frame_cnt, frame_q_ptr->q_size, 0);*/
		return MVS_PKT_STATUS_FAST;
	}


	if (frame_q_ptr->frame_cnt == 0) {
		frame_q_ptr->miss_cnt++;

   /* printf("MVS frame queue underflow %x %d %d", \
	     (uint32)(frame_q_ptr->frame_q), frame_q_ptr->frame_cnt, \
	     frame_q_ptr->miss_cnt);*/

		return MVS_PKT_STATUS_SLOW;
	}


	memcpy(frame_data,
	       (frame_q_ptr->frame_q + frame_q_ptr->curr_frame)->frame_data,
	       (frame_q_ptr->frame_q + frame_q_ptr->curr_frame)->frame_len);
	*frame_info = (frame_q_ptr->frame_q + frame_q_ptr->curr_frame)->frame_info;
	if (frame_len != NULL) {
		*frame_len = (frame_q_ptr->frame_q + frame_q_ptr->curr_frame)->frame_len;
	}

	frame_q_ptr->frame_cnt--;

	if (frame_q_ptr->frame_cnt == 0) {
		frame_q_ptr->curr_frame = 0;
	} else {
		frame_q_ptr->curr_frame = (frame_q_ptr->curr_frame + 1)
					  % (frame_q_ptr->q_size);
	}

	return MVS_PKT_STATUS_NORMAL;
}

/*==========================================================================

FUNCTION MVS_TEST_MVS_CB

DESCRIPTION
  Function handling MVS event callback.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void mvs_test_mvs_cb(mvs_event_type *mvsEvent) {
	switch (mvsEvent->cmd.cmd_status) {
	case MVS_CMD_SUCCESS:
		printf("MVS_CMD_SUCCESS\n");
		break;
	case MVS_CMD_BUSY:
		printf("MVS_CMD_BUSY\n");
		break;
	case MVS_CMD_FAILURE:
		printf("MVS_CMD_FAILURE\n");
		break;
	default:
		printf("UnKnown CMD Status\n");
		break;
	}

}
/*==========================================================================

FUNCTION MVS_TEST_MVS_UL_CB

DESCRIPTION
  MVS uplink packet processing

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void mvs_test_mvs_ul_cb(
		       uint8 *vocoder_packet,
		       mvs_frame_info_type *frame_info,
		       uint16 packet_length,
		       mvs_pkt_status_type *status
		       ) {
	if ((vocoder_packet==NULL) || (frame_info==NULL)) {
    /*printf("packet pointer error", 0, 0, 0);*/
		if (status!=NULL) {
			*status = MVS_PKT_STATUS_SLOW;
		}

		return;
	}


	*status = mvssup_frame_q_put(&mvs_test_mvs_frame_q, vocoder_packet,
				     frame_info, packet_length);

	if (mvssup_frame_check_q_full(&mvs_test_mvs_frame_q) != TRUE) {
		*status = MVS_PKT_STATUS_SLOW;	 /* ask for more packets */
	}
}

/*==========================================================================

FUNCTION MVS_TEST_MVS_DL_CB

DESCRIPTION
  MVS downlink packet processing

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
void mvs_test_mvs_dl_cb(
		       uint8 *vocoder_packet,
		       mvs_frame_info_type *frame_info,
		       mvs_pkt_status_type *status
		       ) {
	uint16 mvs_test_frame_len;
	interrupt_count++;
	if (mvs_test_mvs_delay > 0) {
		mvs_test_mvs_delay--;
		*status = MVS_PKT_STATUS_SLOW;
		return;
	}

	if (mvssup_frame_check_q_empty(&mvs_test_mvs_frame_q) == TRUE) {
		*status = MVS_PKT_STATUS_SLOW;
	} else {
		*status = mvssup_frame_q_get(&mvs_test_mvs_frame_q, vocoder_packet,
					     frame_info, &mvs_test_frame_len);
	}
}


int main(int argc, char **argv) {

	oncrpc_init();

	oncrpc_task_start();

	printf("oncrpc_task_start\n");

	sndcb_app_init();

	printf("sndcb_app_init\n");

	mvscb_app_init();

	printf("mvscb_app_init\n");


	snd_set_device((snd_device_type)SND_DEVICE_HANDSET,
		       (snd_mute_control_type)SND_MUTE_UNMUTED,
		       (snd_mute_control_type)SND_MUTE_UNMUTED,
		       NULL,
		       NULL);
	printf("snd_set_device...unmute\n");

	snd_set_volume((snd_device_type)SND_DEVICE_HANDSET,
		       (snd_method_type)SND_METHOD_VOICE,
		       4,
		       NULL,
		       NULL);
	printf("snd_set_volume\n");

	mvssup_frame_q_init(&mvs_test_mvs_frame_q, mvs_test_mvs_frame_buf,
			    MVS_TEST_MVS_FRAME_Q_SIZE);

	mvs_acquire(MVS_CLIENT_TEST, mvs_test_mvs_cb);
	sleep(1);

	printf("mvs_acquire\n");

	mvs_test_mvs_delay = MVS_TEST_MVS_FRAME_Q_SIZE;
	mvs_enable(MVS_CLIENT_TEST, MVS_MODE_AMR, mvs_test_mvs_ul_cb,mvs_test_mvs_dl_cb,
		   MVS_PKT_CONTEXT_ISR);
	sleep(1);

	printf("mvs_enable\n");

	mvs_amr_set_amr_mode(MVS_AMR_MODE_1220);

	printf("mvs_amr_set_amr_mode_1220\n");

	sleep(30);

	mvs_release(MVS_CLIENT_TEST);

	printf("mvs_release\n");

	snd_set_device((snd_device_type)SND_DEVICE_HANDSET,
		       (snd_mute_control_type)SND_MUTE_MUTED,
		       (snd_mute_control_type)SND_MUTE_MUTED,
		       NULL,
		       NULL);
	printf("snd_set_device...mute\n");

   /*For a minute, check if ARM11 received more than 3000 request then return 0 else return failure.*/

	if (interrupt_count > 1500) {
		printf("TEST PASS...interrupt_count = %d\n",interrupt_count);
		return 0;
	} else {
		printf("TEST FAIL...interrupt_count = %d\n",interrupt_count);
		return -1;
	}

}
#endif /* USE_MVS_DRIVER*/
