/*
 *qsc_helper: QSC related functions used by mdm_helper
 *
 *Copyright (C) 2012 Qualcomm Technologies, Inc. All rights reserved.
 *                   Qualcomm Technologies Proprietary/GTDR
 *
 *All data and information contained in or disclosed by this document is
 *confidential and proprietary information of Qualcomm Technologies, Inc. and all
 *rights therein are expressly reserved.  By accepting this material the
 *recipient agrees that this material and the information contained therein
 *is held in confidence and in trust and will not be used, copied, reproduced
 *in whole or in part, nor its contents revealed in any manner to others
 *without the express written permission of Qualcomm Technologies, Inc.
 *
 *qsc_helper.c : Functions used by mdm_helper to  monitor and interact with QSC
 *module
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/termios.h>
#include <linux/msm_charm.h>
#include <linux/un.h>
#include <linux/ioctl.h>
#include <cutils/properties.h>
#include <termios.h>
#include <pthread.h>
#include "qsc_helper.h"
#include "mdmfiletransfer/mdm_file_transfer_interface.h"

static int BringQSCOutOfReset(int fd, int force_reboot);
static int CheckForDloadMode(int query_mode);
static int SwitchUARTMode(int mode);
static int PrepareForImageUpgrade(struct mdm_device *drv);
static void* QSCHelperCommandThread(void *);
static void SetupQSCHelperFlags();
static int enable_phase_one_behavior = 0;
static int previous_image_upgrade_failed = 0;
static int ram_dumps_enabled = 0;
static int uart_fd = -1;
static int mdm_helper_restarted;
char uart_port[30] = QSC_UART_NODE;
pthread_t qsc_helper_thread;

/*power on sequence*/
int power_up_qsc(struct mdm_device *drv)
{
	int rcode = 0;
	static int power_on_count;
	/* Is this the first powerup.If so set up descriptor
	 * for mdm driver,send,wake charm ioctl,etc
	*/
	if (power_on_count == 0) {
		SetupQSCHelperFlags();
		LOGI("Opening mdm port");
		drv->device_descriptor = open(drv->mdm_port,
				O_RDONLY | O_NONBLOCK);
		if (drv->device_descriptor < 0) {
			LOGE("Failed to open mdm dev node");
			return RET_FAILED;
		}
		/*Thread to monitor for image upgrade requests*/
		rcode = pthread_create(&qsc_helper_thread, NULL,
				&QSCHelperCommandThread,(void*)drv);
		if (rcode) {
			LOGE("Failed to create upgrade thread");
			return RET_FAILED;
		}
		if (previous_image_upgrade_failed) {
			BringQSCOutOfReset(drv->device_descriptor, 1);
			do {
				LOGE("QSC image invalid. Reupgrade image");
				usleep(30000 * 1000);
			} while(1);
		}
		if (SwitchUARTMode(UART_MODE_TTY) != RET_SUCCESS) {
			LOGE("Failed to put uart into TTY mode");
			return RET_FAILED;
		}
		/*If QSC did a hard reset last time around we would be in
		 * dload mode.We do not want to send the wake charm in that
		 * case so we test this early on */
		if (CheckForDloadMode(DLOAD_QUERY_SINGLE) == RET_SUCCESS) {
			if (!ram_dumps_enabled) {
				BringQSCOutOfReset(drv->device_descriptor, 1);
				goto regular_boot;
			}
			drv->required_action = MDM_REQUIRED_ACTION_RAMDUMPS;
			enable_phase_one_behavior = 1;
			power_on_count++;
			return RET_SUCCESS;
		}
		if (!mdm_helper_restarted) {
			rcode = BringQSCOutOfReset(drv->device_descriptor, 1);
			if (rcode != RET_SUCCESS) {
				LOGE("Failed to bring modem out of reset");
				return RET_FAILED;
			}
			LOGI("QSC is out of reset");
		}
		/*In case we were spinning in ERR_FATAL before this boot*/
		if (CheckForDloadMode(DLOAD_QUERY_SINGLE) == RET_SUCCESS) {
			if (!ram_dumps_enabled) {
				BringQSCOutOfReset(drv->device_descriptor, 1);
				goto regular_boot;
			}
			enable_phase_one_behavior = 1;
			LOGI("Phase 1 behavior enabled");
			drv->required_action = MDM_REQUIRED_ACTION_RAMDUMPS;
			power_on_count++;
			return RET_SUCCESS;
		}
regular_boot:
		LOGI("Regular boot.");
		if (ioctl(drv->device_descriptor,
					NORMAL_BOOT_DONE, &rcode) < 0) {
			LOGE("NORMAL_BOOT_DONE failed, rcode: %d", errno);
			return RET_FAILED;
		}
		power_on_count++;

	} else {
		power_on_count++;
		if (enable_phase_one_behavior) {
			enable_phase_one_behavior = 0;
			LOGI("Phase one boot done");
			BringQSCOutOfReset(drv->device_descriptor, 1);
			if (CheckForDloadMode(DLOAD_QUERY_SINGLE) ==
					RET_SUCCESS) {
				LOGE("Retrying to reset QSC to NORMAL mode");
				BringQSCOutOfReset(drv->device_descriptor, 1);

				if (CheckForDloadMode(DLOAD_QUERY_SINGLE) ==
						RET_SUCCESS) {
					LOGE("Unable to reset QSC");
					return RET_FAILED;
				}
			}
		}
		if (ioctl(drv->device_descriptor,
					NORMAL_BOOT_DONE, &rcode) < 0) {
			LOGE("NORMAL_BOOT_DONE ioctl failed");
			return RET_FAILED;
		}
		LOGI("Power_on_count = %d", power_on_count);
	}
	if (SwitchUARTMode(UART_MODE_SMUX) != RET_SUCCESS) {
		LOGE("Unable to switch UART to SMUX mode");
		return RET_FAILED;
	}
	return RET_SUCCESS;
}


/*Wait till mdm_driver tells us something has
 * gone wrong with the QSC*/
int monitor_qsc(struct mdm_device *drv)
{
	int boot_status = 0;
	LOGI("monitoring QSC for errors");
	if (ioctl(drv->device_descriptor, WAIT_FOR_RESTART, &boot_status) < 0) {
		LOGE("WAIT_FOR_RESTART ioctl fail\n");
		return RET_FAILED;
	}
	if (boot_status == CHARM_NORMAL_BOOT) {
		LOGI("Ramdumps not needed. Normal reboot requested");
		drv->required_action = MDM_REQUIRED_ACTION_NORMAL_BOOT;
	} else if (boot_status == CHARM_RAM_DUMPS) {
		LOGI("Ramdumps requested\n");
		drv->required_action = MDM_REQUIRED_ACTION_RAMDUMPS;
	} else {
		LOGE("Unknown boot_status returned");
		return RET_FAILED;
	}
	return RET_SUCCESS;
}

int reboot_qsc(struct mdm_device *drv)
{
	int rcode = 0;
	LOGE("rebooting QSC");
	rcode = BringQSCOutOfReset(drv->device_descriptor, 1);
	if (rcode != RET_SUCCESS) {
		LOGE("failed to reboot QSC");
		return RET_FAILED;
	}
	if (ioctl(drv->device_descriptor, NORMAL_BOOT_DONE, &rcode) < 0) {
		LOGE("NORMAL_BOOT_DONE failed, rcode: %d", errno);
		return RET_FAILED;
	}
	return RET_SUCCESS;
}

int prepare_qsc_for_ramdumps(struct mdm_device *drv)
{
	int rcode;
	LOGI("Set UART to TTY mode");
	if (SwitchUARTMode(UART_MODE_TTY) != RET_SUCCESS) {
		LOGE("Unable to swtich uart to TTY mode");
		return RET_FAILED;
	}
	rcode = CheckForDloadMode(DLOAD_QUERY_TILL_RETRY_LIMIT);
	if (rcode != RET_SUCCESS) {
		LOGE("QSC Failed to enter dload mode");
		/*Tell the mdm-driver we failed to collect dumps*/
		if (ioctl(drv->device_descriptor, RAM_DUMP_DONE, &rcode) < 0)
			LOGE("Failed to send RAM_DUMP_DONE ioctl");
		return RET_FAILED;
	}
	return RET_SUCCESS;
}

int collect_ramdumps_from_qsc(struct mdm_device *drv)
{
	int rcode;
	if (!ram_dumps_enabled) {
		LOGI("QSC dump collection is disabled");
		return RET_SUCCESS;
	}
	LOGI("Collecting dumps");
	rcode = save_ram_dumps(uart_port, drv->ram_dump_path);
	if (rcode != RET_SUCCESS) {
		LOGE("Failed to collect ramdumps");
		/*Tell the mdm-driver we failed to collect dumps*/
		if (ioctl(drv->device_descriptor, RAM_DUMP_DONE, &rcode) < 0) {
			LOGE("Failed to send RAM_DUMP_DONE ioctl");
		}
		return RET_FAILED;
	}
	return RET_SUCCESS;
}

int qsc_helper_post_ramdump_cleanup(struct mdm_device *drv)
{
	LOGI("sending ram_dump_done ioctl\n");
	if (ioctl(drv->device_descriptor, RAM_DUMP_DONE) < 0) {
		LOGE("Failed to send RAM_DUMP_DONE ioctl\n");
		return RET_FAILED;
	}
	if (!enable_phase_one_behavior) {
		LOGI("Waiting for normal boot notification");
		if (monitor_qsc(drv) != RET_SUCCESS) {
			LOGE("Error returned by wait function");
			return RET_FAILED;
		}
		if (drv->required_action != MDM_REQUIRED_ACTION_NORMAL_BOOT) {
			LOGE("Unexpected action  %d returned by wait function",
					drv->required_action);
			return RET_FAILED;
		}
	}
	return RET_SUCCESS;
}

int qsc_helper_failure_cleanup(struct mdm_device * drv)
{
	LOGI("QSC helper cleanup");
	if (drv->device_descriptor >= 0)
		close(drv->device_descriptor);
	if (uart_fd >= 0)
		close(uart_fd);
	return RET_SUCCESS;
}

static int BringQSCOutOfReset(int fd, int force_reboot)
{
	int mdm_status = 0;
	LOGI("Bringing up modem");
	if (ioctl(fd, CHECK_FOR_BOOT, &mdm_status) < 0) {
		LOGE("failed to get mdm status");
		return RET_FAILED;
	}
	if (!force_reboot && !mdm_status) {
		LOGI("Modem is already up");
		return RET_SUCCESS;
	}
	if (ioctl(fd, WAKE_CHARM) < 0) {
		LOGE("Failed to issue ioctl to bring up modem");
		return RET_FAILED;
	}
	return RET_SUCCESS;
}

static int SwitchUARTMode(int mode)
{
	static const int ldisc = N_SMUX;
	int current_mode = -1;
	struct termios options;
	LOGI("UART fd is %d ", uart_fd);
	switch (mode) {
	case UART_MODE_TTY:
		if (uart_fd >= 0) {
			if (ioctl(uart_fd, TIOCGETD, &current_mode) < 0) {
				LOGE("Failed to get current UART mode");
				return RET_FAILED;
			}
			if (current_mode == N_TTY) {
				LOGI("UART is already in tty mode");
				return RET_SUCCESS;
			}
			LOGI("Closing uart_fd");
			close(uart_fd);
			LOGI("UART_fd closed");
		}
		/*Open UART port.This will put it in tty mode*/
		LOGI("Opening port");
		uart_fd = open(QSC_UART_NODE, O_RDWR | O_NOCTTY | O_NDELAY);
		if (uart_fd < 0) {
			LOGE("Error opening UART port in TTY mode");
			goto error;
		}
		current_mode = -1;
		if (ioctl(uart_fd, TIOCGETD, &current_mode) < 0) {
			LOGE("Failed to get UART mode");
			return RET_FAILED;
		}
		if (current_mode != N_TTY) {
			LOGE("Failed to put UART into TTY mode");
			return RET_FAILED;
		}
		LOGI("UART is in tty mode");
		return RET_SUCCESS;
	case UART_MODE_SMUX:
		LOGI("Switching UART to SMUX mode");
		if (uart_fd >= 0) {
			if (ioctl(uart_fd, TIOCGETD, &current_mode) < 0) {
				LOGE("failed to get current uart mode");
				goto error;
			}
			if (current_mode == N_SMUX) {
				LOGI("UART is already in SMUX mode");
				return RET_SUCCESS;
			}
			LOGI("Closing uart port");
			close(uart_fd);
			LOGI("Done closing uart port");
		}
		LOGI("Opening UART port");
		uart_fd = open(QSC_UART_NODE, O_RDWR | O_NOCTTY | O_NDELAY);
		LOGI("Done opening UART port");
		if (uart_fd < 0) {
			LOGE("failed to open uart device node");
			goto error;
		}
		LOGI("Configuring UART");
		fcntl(uart_fd, F_SETFL, 0);
		if (ioctl(uart_fd, TCGETS, &options) < 0) {
			LOGE("TCGETS ioctl failed");
			goto error;
		}
		options.c_cc[VTIME] = 0;/*inter-charecter timer unused*/
		options.c_cc[VMIN] = 0; /*blocking read until 1 chars recieved*/
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= (CS8 | CLOCAL | CREAD | CRTSCTS);
		options.c_iflag = IGNPAR;
		options.c_oflag = 0;
		options.c_lflag = 0;
		LOGI("Baud Rate = 3.2Mbps");
		cfsetospeed(&options, BAUD_RATE_3200);
		cfsetispeed(&options, BAUD_RATE_3200);
		if (ioctl(uart_fd, TCSETS, &options) < 0) {
			LOGE("TCSETS ioctl fail");
			goto error;
		}
		LOGI("Setting SMUX as line discipline");
		if (ioctl(uart_fd, TIOCSETD, &ldisc) < 0) {
			LOGE("Failed to set SMUX line discipline");
			goto error;
		}
		break;
	default:
		break;
	}
	return RET_SUCCESS;
error:
	LOGE("failed to configure UART");
	return RET_FAILED;
}

static int CheckForDloadMode(int query_mode)
{
	int i;
	for(i = 0; i < NUM_DLOAD_DETECT_RETRIES; i++) {
		if (get_dload_status(uart_port)) {
			LOGI("QSC is in dload mode\n");
			return RET_SUCCESS;
		}
		if (query_mode == DLOAD_QUERY_SINGLE)
			break;
		usleep(MDM_POLL_DELAY*1000);
	}
	LOGI("QSC is not in dload mode");
	return RET_FAILED;
}

static void SetupQSCHelperFlags()
{
	char dump_enabled_property_value[PROPERTY_VALUE_MAX];
	char restart_property_value[PROPERTY_VALUE_MAX];
	struct stat upgrade_file_stat;
	property_get("ro.service.mdm_helper_restarted",
			restart_property_value, NULL);
	if (strncmp(restart_property_value, "true",  PROPERTY_VALUE_MAX) == 0) {
		LOGI("mdm helper has been restarted");
		mdm_helper_restarted = 1;
	} else
		LOGI("First start of mdm helper");
	property_get("persist.sys.dump_enabled",
			dump_enabled_property_value,
			"true");
	if (strncmp(dump_enabled_property_value,
				"true", PROPERTY_VALUE_MAX) == 0) {
		LOGI("Ramdump collection enabled");
		ram_dumps_enabled = 1;
	}
	else
		LOGI("Ramdump collection is disabled");
	if (!stat(IMAGE_UPGRADE_STATUS_FILE, &upgrade_file_stat)) {
		/*Last image upgrade did not go through comrrectly.
		 * QSC image is probably not good at this point and
		 * should be reupgraded*/
		LOGE("The last image failed to complete.");
		previous_image_upgrade_failed = 1;
	}
}

/*Monitors for external commands from the user space.
 * Currently this is limited to image upgrade.*/
static void* QSCHelperCommandThread(void *arg)
{
	char buffer[QSC_HELPER_CMD_BUF_SIZE + 1];
	int socket_fd;
	int client_fd;
	int rcode;
	int bytes_read;
	int offset;
	char *command = NULL;
	struct stat socket_stat;
	const char * const socket_name = QSC_HELPER_SOCKET;
	struct sockaddr_un serv_addr;
	struct mdm_device *drv = (struct mdm_device *)arg;
	socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		LOGE("Server failed to create socket");
		goto error;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sun_family = AF_LOCAL;
	snprintf(serv_addr.sun_path, UNIX_PATH_MAX, "%s", socket_name);
	if (!stat(QSC_HELPER_SOCKET, &socket_stat)) {
		unlink(QSC_HELPER_SOCKET);
	}
	if (bind(socket_fd, (struct sockaddr*) &serv_addr, UNIX_PATH_MAX) < 0) {
		LOGE("Failed to bind to socket: %s",strerror(errno));
		goto error;
	}
	if (chmod(QSC_HELPER_SOCKET, S_IRUSR | S_IWUSR) == -1) {
		LOGE("Failed to set socket permissions: %s",strerror(errno));
		goto error;
	}
	if (listen(socket_fd, 1) < 0) {
		LOGE("Listen failed : %s", strerror(errno));
		goto error;
	}
	LOGI("Upgrade thread created");
	do {
		LOGI("Waiting for command");
		client_fd = accept(socket_fd, NULL,0);
		if (client_fd < 0) {
			LOGE("Failed to accept connection: %s",strerror(errno));
			goto cleanup;
		}
		offset = 0;
		do {
			bytes_read = read(client_fd, buffer + offset,
					QSC_HELPER_CMD_BUF_SIZE - offset);
			if (bytes_read < 0) {
				LOGE("Failed to read from socket: %s",
						strerror(errno));
				goto cleanup;
			}
			offset += bytes_read;
		} while (bytes_read > 0 && offset < QSC_HELPER_CMD_BUF_SIZE);
		LOGI("Command is :%s:",buffer);
		if (strncmp(buffer,QSC_HELPER_COMMAND_UPGRADE_START,
					QSC_HELPER_COMMAND_LENGTH) == 0) {
			LOGI("Image upgrade start command recieved");
			rcode = 0;
			if (!previous_image_upgrade_failed) {
				if (ioctl(drv->device_descriptor,
							CHECK_FOR_BOOT,
							&rcode) < 0) {
					LOGE("CHECK_FOR_BOOT ioctl failed.");
					rcode = RET_FAILED;
				} else if (rcode) {
					LOGE("QSC is not yet up");
					rcode = RET_FAILED;
				}
				if (rcode == RET_FAILED) {
					write(client_fd, &rcode, sizeof(rcode));
					close (client_fd);
					goto cleanup;
				}
			}
			rcode = PrepareForImageUpgrade(drv);
			write(client_fd, &rcode, sizeof(rcode));
			close(client_fd);
		} else if (strncmp(buffer,QSC_HELPER_COMMAND_UPGRADE_DONE,
					QSC_HELPER_COMMAND_LENGTH) == 0) {
			LOGI("Image upgrade done command recieved");
			rcode = unlink(IMAGE_UPGRADE_STATUS_FILE);
			if (rcode == -1) {
					LOGE("Failed to delete status file: %s",
						strerror(errno));
					rcode = RET_FAILED;
			}
			write(client_fd,&rcode, sizeof(rcode));
			close (client_fd);
			goto cleanup;
		} else if (strncmp(buffer,
					QSC_HELPER_COMMAND_SWITCH_USB_CONTROL,
					QSC_HELPER_COMMAND_LENGTH) == 0) {
			LOGI("switching control to QSC");
			rcode = MDM_CONTROLLED_UPGRADE;
			if (ioctl(drv->device_descriptor,
						IMAGE_UPGRADE, &rcode) < 0) {
				LOGE("Failed to send image_upgrade ioctl");
			}
			rcode = RET_FAILED;
			write(client_fd, &rcode, sizeof(rcode));
			close(client_fd);
			goto cleanup;
		} else {
			LOGE("Unrecognised command %s",command);
			rcode = RET_FAILED;
			write(client_fd, &rcode, sizeof(rcode));
			close(client_fd);
			goto cleanup;
		}

cleanup:
		memset(buffer, '\0', sizeof(buffer));
	} while(1);
error:
	do {
		LOGE("Upgrade thread has failed to init");
		usleep(30000 * 1000);
	} while(1);
	exit(1);
}

static int PrepareForImageUpgrade(struct mdm_device *drv)
{
	int fd;
	int mdm_driver_cmd = APQ_CONTROLLED_UPGRADE;
	fd = open(IMAGE_UPGRADE_STATUS_FILE, O_CREAT,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if (fd < 0) {
		LOGE("Failed to create upgrade status file: %s",
				strerror(errno));
		return RET_FAILED;
	}
	close(fd);
	if (ioctl(drv->device_descriptor,
				IMAGE_UPGRADE, &mdm_driver_cmd) < 0) {
		LOGE("Failed to send ioctl to mdm-driver");
		return RET_FAILED;
	}
	if (SwitchUARTMode(UART_MODE_TTY) != RET_SUCCESS) {
		LOGE("Failed to switch UART to TTY mode");
		return RET_FAILED;
	}
	if (CheckForDloadMode(DLOAD_QUERY_SINGLE) != RET_SUCCESS) {
		LOGE("QSC did not go into dload mode");
		return RET_FAILED;
	}
	return RET_SUCCESS;
}

int standalone_task(struct mdm_device *drv, char *argv[])
{
	const char * const sock_name = QSC_HELPER_SOCKET;
	int sock_fd;
	int bytes_written;
	int bytes_read;
	int offset;
	int status;
	struct sockaddr_un serv_addr;
	LOGI("Standalone task for %s", drv->mdm_name);
	if ((strncmp(argv[1], QSC_HELPER_COMMAND_UPGRADE_START,
				QSC_HELPER_COMMAND_LENGTH) != 0) &&
		(strncmp(argv[1], QSC_HELPER_COMMAND_UPGRADE_DONE,
			 QSC_HELPER_COMMAND_LENGTH) != 0) &&
		(strncmp(argv[1],QSC_HELPER_COMMAND_SWITCH_USB_CONTROL,
			 QSC_HELPER_COMMAND_LENGTH) != 0)) {
		LOGE("Invalid command");
		return RET_FAILED;
	}
	sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		LOGE("Failed to create socket : %s", strerror(errno));
		return RET_FAILED;
	}
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sun_family = AF_LOCAL;
	snprintf(serv_addr.sun_path, UNIX_PATH_MAX, "%s", sock_name);
	if (connect(sock_fd, (struct sockaddr*) &serv_addr,
				sizeof(sa_family_t) + strlen(sock_name)) != 0) {
	LOGE("Failed to connect to mdm-helper socket: %s", strerror(errno));
	close(sock_fd);
	return RET_FAILED;
	}
	bytes_written = write(sock_fd, argv[1], strlen(argv[1]));
	if (bytes_written < 0) {
		LOGE("Write to socket failed :%s:",strerror(errno));
		close(sock_fd);
		return RET_FAILED;
	}
	shutdown(sock_fd, SHUT_WR);
	offset = 0;
	do {
		bytes_read = read(sock_fd, &status,
				sizeof(status) - offset);
		if (bytes_read < 0) {
			LOGE("Failed to read from socket: %s",
					strerror(errno));
			close(sock_fd);
			return RET_FAILED;
		}
		offset += bytes_read;
	} while (bytes_read > 0 && offset < sizeof(int));
	LOGI("Return code is %d",status);
	close(sock_fd);
	return status;
}

