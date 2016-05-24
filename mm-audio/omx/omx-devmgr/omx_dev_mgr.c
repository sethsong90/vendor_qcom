/*
 * Copyright (c) 2010-2011 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential. 
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "control.h"

#define DEVMGR_MAX_PLAYBACK_SESSION 4
#define DEVMGR_MAX_RECORDING_SESSION 2
#define DEVMGR_DEFAULT_SID 65523

#define DIR_RX 1
#define DIR_TX 2

static int devmgr_devid_rx;
static int devmgr_devid_tx;
static int devmgr_dev_count_tx;
static int devmgr_dev_count_rx;
unsigned short devmgr_sid_rx_array[DEVMGR_MAX_PLAYBACK_SESSION];
unsigned short devmgr_sid_tx_array[DEVMGR_MAX_PLAYBACK_SESSION];
static int devmgr_sid_count_rx = 0;
static int devmgr_sid_count_tx = 0;

const char *devctl_help_text =
"Usage:  ./mm-omx-devmgr -cmd=dev_switch_rx -dev_id=x					\n\
	./mm-omx-devmgr -cmd=dev_switch_tx -dev_id=x					\n\
	where x = any of the supported device IDs listed below.			\n\
	To exit mm-omx-devmgr: 											\n\
	./mm-omx-devmgr -exit											\n\
Note:                                                               	\n\
(i)   Handset RX/TX is set as default device for all playbacks/recordings \n\
(ii)  After a device switch, audio will be routed to the last set   	\n\
      device                                                        	\n\
(iii) Device List and their corresponding IDs can be got using:			\n\
	./mm-omx-devmgr -help						\n";

char cmdstr[255];

void print_help_menu(void)
{
	int i, dev_cnt, device_id, alsa_ctl;
	const char **device_names;
	printf("%s\n", devctl_help_text);
	alsa_ctl = msm_mixer_open("/dev/snd/controlC0", 0);
	if (alsa_ctl < 0)
		perror("Fail to open ALSA MIXER\n");
	else{
		printf("\nDevice List:\n");
		dev_cnt = msm_get_device_count();
		device_names = msm_get_device_list();
		for (i = 0; i < dev_cnt;) {
			device_id = msm_get_device(device_names[i]);
			if (device_id >= 0)
				printf("device name %s:dev_id: %d\n",
					device_names[i], device_id);
			i++;
		}
	}
	alsa_ctl = msm_mixer_close();
	if (alsa_ctl < 0)
		perror("Fail to close ALSA MIXER\n");
}

int devmgr_disable_device(int dev_id, unsigned short route_dir)
{
	if (route_dir == DIR_RX){
		devmgr_dev_count_rx--;
		if (devmgr_dev_count_rx == 0) {
			if (msm_en_device(dev_id, 0) < 0)
				return -1;
		}
	}
	else if (route_dir == DIR_TX){
		devmgr_dev_count_tx--;
		if (devmgr_dev_count_tx == 0) {
			if (msm_en_device(dev_id, 0) < 0)
				return -1;
		}
	}
	else
		perror("devmgr_disable_device: Invalid route direction\n");
	return 0;
}

int devmgr_enable_device(int dev_id, unsigned short route_dir)
{

	if (msm_en_device(dev_id, 1) < 0)
		return -1;
	if (route_dir == DIR_RX)
		devmgr_dev_count_rx++;
	else if (route_dir == DIR_TX)
		devmgr_dev_count_tx++;
	else
		perror("devmgr_enable_device: Invalid route direction\n");
	return 0;
}

int devmgr_register_session(unsigned short session_id, unsigned short route_dir)
{

	printf("devmgr_register_session: Registering Session ID = %d\n",
								session_id);
	if (route_dir == DIR_RX){
		if ((devmgr_sid_count_rx < DEVMGR_MAX_PLAYBACK_SESSION) &&
		(devmgr_sid_rx_array[devmgr_sid_count_rx] == DEVMGR_DEFAULT_SID))
			devmgr_sid_rx_array[devmgr_sid_count_rx++] = session_id;

		if (devmgr_enable_device(devmgr_devid_rx, DIR_RX) < 0){
			perror("could not enable RX device\n");
			return -1;
		}
		if (msm_route_stream(DIR_RX, session_id, devmgr_devid_rx, 1) < 0) {
			perror("could not route stream to Device\n");
			if (devmgr_disable_device(devmgr_devid_rx, DIR_RX) < 0)
				perror("could not disable device\n");
			return -1;
		}
	}
	else if (route_dir == DIR_TX){
		if ((devmgr_sid_count_tx < DEVMGR_MAX_RECORDING_SESSION) &&
		(devmgr_sid_tx_array[devmgr_sid_count_tx] == DEVMGR_DEFAULT_SID))
			devmgr_sid_tx_array[devmgr_sid_count_tx++] = session_id;
		if (devmgr_enable_device(devmgr_devid_tx, DIR_TX) < 0){
			perror("could not enable TX device\n");
			return -1;
		}

		if (msm_route_stream(DIR_TX, session_id, devmgr_devid_tx, 1) < 0) {
			perror("could not route stream to Device\n");
			if (devmgr_disable_device(devmgr_devid_tx, DIR_TX) < 0)
				perror("could not disable device\n");
			return -1;
		}
	}
	else
		perror("devmgr_register_session: Invalid route direction.\n");
	return 0;
}

int devmgr_unregister_session(unsigned short session_id, unsigned short route_dir)
{

	int index = 0;
	printf("devmgr_unregister_session: Unregistering Session ID = %d\n",
	session_id);
	if (route_dir == DIR_RX){
		while (index < devmgr_sid_count_rx) {
			if (session_id == devmgr_sid_rx_array[index])
			break;
		index++;
	}
		while (index < (devmgr_sid_count_rx-1)) {
			devmgr_sid_rx_array[index]  =  devmgr_sid_rx_array[index+1];
		index++;
	}
	/* Reset the last entry */
		devmgr_sid_rx_array[index]         = DEVMGR_DEFAULT_SID;
		devmgr_sid_count_rx--;

		if (msm_route_stream(DIR_RX, session_id, devmgr_devid_rx, 0) < 0)
			perror("could not de-route stream to Device\n");

		if (devmgr_disable_device(devmgr_devid_rx, DIR_RX) < 0){
			perror("could not disable RX device\n");
		}
	}else if(route_dir == DIR_TX){
		while (index < devmgr_sid_count_tx) {
			if (session_id == devmgr_sid_tx_array[index])
				break;
			index++;
		}
		while (index < (devmgr_sid_count_tx-1)) {
			devmgr_sid_tx_array[index]  =  devmgr_sid_tx_array[index+1];
			index++;
		}
		/* Reset the last entry */
		devmgr_sid_tx_array[index]         = DEVMGR_DEFAULT_SID;
		devmgr_sid_count_tx--;

		if (msm_route_stream(DIR_TX, session_id, devmgr_devid_tx, 0) < 0)
			perror("could not de-route stream to Device\n");

		if (devmgr_disable_device(devmgr_devid_tx, DIR_TX) < 0){
			perror("could not disable TX device\n");
		}
	}
	else
		perror("devmgr_unregister_session: Invalid route direction\n");
	return 0;
}

void omx_deinit_devmgr(void)
{
	int alsa_ctl;
	alsa_ctl = msm_mixer_close();
	if (alsa_ctl < 0)
		perror("Fail to close ALSA MIXER\n");
	else{
		printf("%s: Closed ALSA MIXER\n", __func__);
	}
}

void omx_init_devmgr(void)
{

	int i, alsa_ctl, dev_cnt, device_id;
	const char **device_names;
	const char *def_device_rx = "handset_rx";
	const char *def_device_tx = "handset_tx";

	alsa_ctl = msm_mixer_open("/dev/snd/controlC0", 0);
	if (alsa_ctl < 0)
		perror("Fail to open ALSA MIXER\n");
	else{
		printf("Device Manager: List of Devices supported: \n");
		dev_cnt = msm_get_device_count();
		device_names = msm_get_device_list();
		for (i = 0; i < dev_cnt;) {
			device_id = msm_get_device(device_names[i]);
			if (device_id >= 0)
				printf("device name %s:dev_id: %d\n", device_names[i],
			device_id);
				i++;
			}
			devmgr_devid_rx = msm_get_device(def_device_rx);
			printf("Setting default RX =  %d\n", devmgr_devid_rx);
			devmgr_devid_tx = msm_get_device(def_device_tx);
			printf("Setting default TX =  %d\n", devmgr_devid_tx);
			for (i = 0; i < DEVMGR_MAX_PLAYBACK_SESSION; i++){
				devmgr_sid_rx_array[i] = DEVMGR_DEFAULT_SID;
				devmgr_sid_tx_array[i] = DEVMGR_DEFAULT_SID;
			}
		}
    return;
}

int devmgr_devctl_handler()
{

	char *token;
	char *saveptr = NULL;
	int ret_val = 0, sid, dev_source, dev_dest, index;

	token = strtok_r(cmdstr, " ", saveptr);

	if (token != NULL) {
		if (!memcmp(token, "-cmd=", (sizeof("-cmd=") - 1))) {
			token = &token[sizeof("-cmd=") - 1];
			if (!strcmp(token, "dev_switch_rx")) {
				token = strtok_r(NULL, " ", saveptr);
                                if(token == NULL)
                                    return -1;
				if (!memcmp(token, "-dev_id=",
						(sizeof("-dev_id=") - 1))) {
					dev_dest = atoi(&token
						[sizeof("-dev_id=") - 1]);
					dev_source = devmgr_devid_rx;
					if (dev_source != dev_dest) {
						printf("%s: Device Switch from = %d to = %d\n",
							__func__, dev_source, dev_dest);
						if (devmgr_sid_count_rx == 0){
							devmgr_devid_rx = dev_dest;
							printf("%s: Device Switch Success\n",
						__func__);
						}

						for (index = 0;
                         index < devmgr_sid_count_rx;
							 index++) {
							msm_route_stream
							(DIR_RX, devmgr_sid_rx_array[index],
							dev_dest, 1);
							if (
								(devmgr_disable_device
							(dev_source, DIR_RX)) == 0) {
								msm_route_stream(DIR_RX, 
							devmgr_sid_rx_array[index], dev_source, 0);
								if ((devmgr_enable_device
							(dev_dest, DIR_RX))== 0) {
									printf("%s: Device Switch Success\n",
								__func__);
									devmgr_devid_rx = dev_dest;
								}
						    }
						}
					} else {
						printf("%s(): Device has not changed as current device is:%d\n",
					__func__, dev_dest);
					}
				}
			} else if (!strcmp(token, "dev_switch_tx")) {
				token = strtok_r(NULL, " ", saveptr);
                                if(token == NULL)
                                    return -1;
				if (!memcmp(token, "-dev_id=",
						(sizeof("-dev_id=") - 1))) {
					dev_dest = atoi(&token
						[sizeof("-dev_id=") - 1]);
					dev_source = devmgr_devid_tx;
					if (dev_source != dev_dest) {
						printf("%s: Device Switch from = %d to = %d\n",
							__func__, dev_source, dev_dest);
						if (devmgr_sid_count_tx == 0){
							devmgr_devid_tx = dev_dest;
							printf("%s: Device Switch Success\n",
						__func__);
						}

						for (index = 0;
                         index < devmgr_sid_count_tx;
							 index++) {
							msm_route_stream
							(DIR_TX, devmgr_sid_tx_array[index],
							dev_dest, 1);
							if (
								(devmgr_disable_device
							(dev_source, DIR_TX)) == 0) {
								msm_route_stream(DIR_TX,
							devmgr_sid_tx_array[index], dev_source, 0);
								if ((devmgr_enable_device
							(dev_dest, DIR_TX))== 0) {
									printf("%s: Device Switch Success\n",
								__func__);
									devmgr_devid_tx = dev_dest;
								}
						    }
						}
					} else {
						printf("%s(): Device has not changed as current device is:%d\n",
					__func__, dev_dest);
					}
				}
			} else if (!strcmp(token, "register_session_rx")) {
				token = strtok_r(NULL, " ", saveptr);
                                if(token == NULL)
                                    return -1;
				if (!memcmp(token, "-sid=", (sizeof
					("-sid=") - 1))) {
					sid = atoi(&token[sizeof("-sid=")
									- 1]);
					devmgr_register_session(sid, DIR_RX);
				}
			} else if (!strcmp(token, "unregister_session_rx")) {
				token = strtok_r(NULL, " ", saveptr);
                                if(token == NULL)
                                    return -1;
				if (!memcmp(token, "-sid=", (sizeof
					("-sid=") - 1))) {
					sid = atoi(&token[sizeof("-sid=")
									- 1]);
					devmgr_unregister_session(sid, DIR_RX);
				}
			} else if (!strcmp(token, "register_session_tx")) {
				token = strtok_r(NULL, " ", saveptr);
                                if(token == NULL)
                                    return -1;
				if (!memcmp(token, "-sid=", (sizeof
					("-sid=") - 1))) {
					sid = atoi(&token[sizeof("-sid=")
									- 1]);
					devmgr_register_session(sid, DIR_TX);
				}
			} else if (!strcmp(token, "unregister_session_tx")) {
				token = strtok_r(NULL, " ", saveptr);
                                if(token == NULL)
                                    return -1;
				if (!memcmp(token, "-sid=", (sizeof
					("-sid=") - 1))) {
					sid = atoi(&token[sizeof("-sid=")
									- 1]);
					devmgr_unregister_session(sid, DIR_TX);
				}
			} else {
				ret_val = -1;
			}
		} else {
			ret_val = -1;
		}
	}

	return ret_val;
}

void devmgr_cmd_svr(void) {
	const char *exit_str = "-exit";
	int fd;
	ssize_t read_count;

	if (mknod("/data/omx_devmgr", S_IFIFO | 0666, 0) == 0) {
		fd = open("/data/omx_devmgr", O_RDONLY);

	omx_init_devmgr();
	while (1) {
		cmdstr[0] = '\0';
		read_count = read(fd, cmdstr, 255);
		if (read_count == 0) {
			// end of stream
			sleep(2);
		} else if (read_count < 0) {
			fprintf(stderr, "omx_devmgr: error reading cmd\n");
			break;
		} else {
			cmdstr[read_count] = '\0';
			if (!strcmp(cmdstr, exit_str)) {
				break;
			}
			cmdstr[read_count-1] = ' ';
			if (devmgr_devctl_handler() < 0) {
				printf("mm-omx-devmgr: Invalid command\n");
				print_help_menu();
			}
		}
	}
	printf("omx_devmgr: exit server mode\n");
	close(fd);
    remove("/data/omx_devmgr");
	omx_deinit_devmgr();
	} else {
		fprintf(stderr, "omx_devmgr: Failed to create server\n");
	}
}

void write_devctlcmd(int fd, char *buf1, char *buf2){
	int nbytes, nbytesWritten;
	char cmdstr[128];
	snprintf(cmdstr, 128, "%s %s\n", buf1, buf2);
	nbytes = strlen(cmdstr);
	nbytesWritten = write(fd, cmdstr, nbytes);

	if(nbytes != nbytesWritten)
		printf("Failed to write string \"%s\" to omx_devmgr\n",cmdstr);
}


int main(int argc, char **argv)
{
	argc--;
	argv++;
	int fd;
	if (argc > 0) {
		fd = open("/data/omx_devmgr", O_WRONLY);
		if (!strcmp(argv[0], "-help")) {
			print_help_menu();
		}
		else if(!strcmp(argv[0], "-exit") && (argc == 1)){
			if (fd >= 0)
				write(fd, argv[0], strlen(argv[0]));
			else
				printf("Unable to open /data/omx_devmgr\n");
		}
		else if((!strcmp(argv[0], "-cmd=dev_switch_rx") ||
	!strcmp(argv[0], "-cmd=dev_switch_tx")) && (argc == 2)){
			if (fd >= 0)
				write_devctlcmd(fd, argv[0], argv[1]);
			else
				printf("Unable to open /data/omx_devmgr\n");
		}
		else
			printf("mm-omx-devmgr: Invalid command\n");
		close(fd);
	}
	else {
		devmgr_cmd_svr();
	}

	return 0;
}

