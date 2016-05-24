/*
 * ----------------------------------------------------------------------------
 *  Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 *  Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 *
 *  9x15_helper.c : Functions used by mdm_helper to  monitor and interact with 9x15
 *  module
 *
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
#include <linux/ioctl.h>
#include <cutils/properties.h>
#include <termios.h>
#include <pthread.h>
#include "9x15_helper.h"

struct private_data_9x15 private_data_9x15_hsic = {
	.flashless_boot_device = "/dev/ks_hsic_bridge",
	.efs_sync_device = "/dev/efs_hsic_bridge",
	.file_prefix = "",
	.efs_ks_pid = 0,
	.partition_list = {
		{
			{
				{16, TEMP_FILE_DIR "efs1.tmp"},
				"efs1.mbn",
				TEMP_FILE_DIR "efs1bin.tmp"
			},
			EFS_FILE_DIR "m9kefs1",
			3072,
		},
		{
			{
				{17, TEMP_FILE_DIR "efs2.tmp"},
				"efs2.mbn",
				TEMP_FILE_DIR "efs2bin.tmp"
			},
			EFS_FILE_DIR "m9kefs2",
			3072,
		},
		{
			{
				{20, TEMP_FILE_DIR "efs3.tmp"},
				"efs3.mbn",
				TEMP_FILE_DIR "efs3bin.tmp"
			},
			EFS_FILE_DIR "m9kefs3",
			3072,
		},
	},
	.other_prepend_images = {
		{
			{29, TEMP_FILE_DIR "acdb.tmp"},
			"acdb.mbn",
			"mdm_acdb.img",
		},
	},
	.image_list = {
		{2, "amss.mbn"},
		{6, "apps.mbn"},
		{8, "dsp1.mbn"},
		{11, "osbl.mbn"},
		{12, "dsp2.mbn"},
		{21, "sbl1.mbn"},
		{22, "sbl2.mbn"},
		{23, "rpm.mbn"},
		{28, "dsp3.mbn"},
		{0, NULL},
	},
};

struct private_data_9x15 private_data_9x15_hsic_dsda = {
	.flashless_boot_device = "/dev/ks_hsic_bridge",
	.efs_sync_device = "/dev/efs_hsic_bridge",
	.file_prefix = "mdm1",
	.efs_ks_pid = 0,
	.partition_list = {
		{
			{
				{16, TEMP_FILE_DIR "m9k0efs1.tmp"},
				"efs1.mbn",
				TEMP_FILE_DIR "m9k0efs1bin.tmp"
			},
			EFS_FILE_DIR "mdm1m9kefs1",
			3072,
		},
		{
			{
				{17, TEMP_FILE_DIR "m9k0efs2.tmp"},
				"efs2.mbn",
				TEMP_FILE_DIR "m9k0efs2bin.tmp"
			},
			EFS_FILE_DIR "mdm1m9kefs2",
			3072,
		},
		{
			{
				{20, TEMP_FILE_DIR "m9k0efs3.tmp"},
				"efs3.mbn",
				TEMP_FILE_DIR "m9k0efs3bin.tmp"
			},
			EFS_FILE_DIR "mdm1m9kefs3",
			3072,
		},
	},
	.other_prepend_images = {
		{
			{29, TEMP_FILE_DIR "m9k0acdb.tmp"},
			"acdb.mbn",
			"mdm_acdb.img",
		},
	},
	.image_list = {
		{2, "amss.mbn"},
		{6, "apps.mbn"},
		{8, "dsp1.mbn"},
		{11, "osbl.mbn"},
		{12, "dsp2.mbn"},
		{21, "sbl1.mbn"},
		{22, "sbl2.mbn"},
		{23, "rpm.mbn"},
		{28, "dsp3.mbn"},
		{0, NULL},
	},
};

struct private_data_9x15 private_data_9x15_usb_dsda = {
	.flashless_boot_device = "/dev/ks_usb_bridge",
	.efs_sync_device = "/dev/efs_usb_bridge",
	.file_prefix = "mdm2",
	.efs_ks_pid = 0,
	.partition_list =
	{
		{
			{
				{16, TEMP_FILE_DIR "m9k1efs1.tmp"},
				"efs1.mbn",
				TEMP_FILE_DIR "m9k1efs1bin.tmp"
			},
			EFS_FILE_DIR "mdm2m9kefs1",
			3072,
		},
		{
			{
				{17, TEMP_FILE_DIR "m9k1efs2.tmp"},
				"efs2.mbn",
				TEMP_FILE_DIR "m9k1efs2bin.tmp"
			},
			EFS_FILE_DIR "mdm2m9kefs2",
			3072,
		},
		{
			{
				{20, TEMP_FILE_DIR "m9k1efs3.tmp"},
				"efs3.mbn",
				TEMP_FILE_DIR "m9k1efs3bin.tmp"},
			EFS_FILE_DIR "mdm2m9kefs3",
			3072,
		},
	},
	.other_prepend_images =
	{
		{
			{29, TEMP_FILE_DIR "m9k1acdb.tmp"},
			"acdb.mbn",
			"mdm_acdb.img",
		},
	},
	.image_list = {
		{2, "amss.mbn"},
		{6, "apps.mbn"},
		{8, "dsp1.mbn"},
		{11, "osbl.mbn"},
		{12, "dsp2.mbn"},
		{21, "sbl1.mbn"},
		{22, "sbl2.mbn"},
		{23, "rpm.mbn"},
		{28, "dsp3.mbn"},
		{0, NULL},
	},
};

static int Bring9x15OutOfReset(int fd);
static int PrependHeaders(struct mdm_device *dev);
static int LoadSahara(struct mdm_device *dev, char* options);
static int find_file(const char *filename, char* temp_string,
		size_t temp_string_size);

static int WaitForCOMport(char *DevNode, int attempt_read);
#define DELAY_BETWEEN_RETRIES_MS    (500)
#define NUM_RETRIES                 (50)

static char* search_paths[] = {"/system/etc/firmware/", "/firmware/image/",
	TEMP_FILE_DIR, EFS_FILE_DIR};

struct partitions_to_file_dump partition_list[] = {
    {EFS_FILE_DIR "m9kefs1", TEMP_FILE_DIR "efs1bin.tmp", 3072},
    {EFS_FILE_DIR "m9kefs2", TEMP_FILE_DIR "efs2bin.tmp", 3072},
    {EFS_FILE_DIR "m9kefs3", TEMP_FILE_DIR "efs3bin.tmp", 3072},
};
struct headers_to_prepend header_list[] = {
    {"efs1.mbn", TEMP_FILE_DIR "efs1bin.tmp", TEMP_FILE_DIR "efs1.tmp"},
    {"efs2.mbn", TEMP_FILE_DIR "efs2bin.tmp", TEMP_FILE_DIR "efs2.tmp"},
    {"efs3.mbn", TEMP_FILE_DIR "efs3bin.tmp", TEMP_FILE_DIR "efs3.tmp"},
    {"acdb.mbn", "mdm_acdb.img", TEMP_FILE_DIR "acdb.tmp"},
};

/*The below functions will eventually be moved to a seperate
 * file(9x15_helper.c)*/
int power_up_9x15(struct mdm_device *dev)
{
	int rcode = 0;
	static int power_on_count;

	LOGI("Starting up %s\n",dev->mdm_name);
    /* Is this the first powerup.If so set up descriptor
	 * for mdm driver,send,wake charm ioctl,etc
	*/
	if (power_on_count == 0) {
		LOGI("%s: Opening mdm port", dev->mdm_name);
		dev->device_descriptor = open(dev->mdm_port,
				O_RDONLY | O_NONBLOCK);
		if (dev->device_descriptor < 0) {
			LOGE("%s: Failed to open mdm dev node", dev->mdm_name);
			return RET_FAILED;
		}
	}
    rcode = Bring9x15OutOfReset(dev->device_descriptor);
    if (rcode != RET_SUCCESS) {
        LOGE("%s: Failed to bring modem out of reset", dev->mdm_name);
        return RET_FAILED;
    }
    usleep(2000*1000);
    LOGI("%s is out of reset", dev->mdm_name);

    rcode = PrependHeaders(dev);
    if (rcode != RET_SUCCESS) {
        LOGE("%s: Failed to prepend headers", dev->mdm_name);
        return RET_FAILED;
    }

    if (power_on_count == 0) {
        rcode = LoadSahara(dev, "");
    }
    else {
        rcode = LoadSahara(dev, "-i");
    }
    if (rcode != RET_SUCCESS) {
        LOGE("%s: Failed to transfer images", dev->mdm_name);
        return RET_FAILED;
    }

    LOGI("%s: Regular boot.", dev->mdm_name);
    if (ioctl(dev->device_descriptor,
					NORMAL_BOOT_DONE, &rcode) < 0) {
        LOGE("%s: NORMAL_BOOT_DONE failed, rcode: %d", dev->mdm_name, errno);
        return RET_FAILED;
    }
    power_on_count++;

    return RET_SUCCESS;
}

int monitor_9x15(struct mdm_device *dev)
{
	int boot_status = 0;
    char command_string[2048];
    struct private_data_9x15 *data_9x15 =
	    (struct private_data_9x15 *) dev->private_data;
    char *newargv[] = {
        KS_PATH,
        "-m",
        "-p", data_9x15->efs_sync_device,
        "-w", EFS_FILE_DIR,
        "-t", "-1",
        "-l",
        "-g", data_9x15->file_prefix,
        NULL };
    char *newenviron[] = { NULL };
    int rcode;

    if(WaitForCOMport(data_9x15->efs_sync_device, 0) == RET_FAILED) {
        LOGE("%s: Could not find EFS sync port", dev->mdm_name);
        return RET_FAILED;
    }
    LOGI("%s: Launching EFS sync process",dev->mdm_name);

    data_9x15->efs_ks_pid = fork();
    if (data_9x15->efs_ks_pid < 0) {
        LOGE("%s: Forking new process for EFS sync failed", dev->mdm_name);
        return RET_FAILED;
    }
    else if (data_9x15->efs_ks_pid == 0) {
        if (execve(KS_PATH, newargv, newenviron) == -1) {
            LOGE("%s: Spawning EFS KS process using execve failed",
			    dev->mdm_name);
            _exit(127);
        }
    }

	LOGI("%s: monitoring 9x15 for errors", dev->mdm_name);
	if (ioctl(dev->device_descriptor, WAIT_FOR_RESTART, &boot_status) < 0) {
		LOGE("%s: WAIT_FOR_RESTART ioctl fail\n", dev->mdm_name);
		return RET_FAILED;
	}
	if (boot_status == CHARM_NORMAL_BOOT) {
		LOGI("%s: Ramdumps not needed. Normal reboot requested",
				dev->mdm_name);
		dev->required_action = MDM_REQUIRED_ACTION_NORMAL_BOOT;
	} else if (boot_status == CHARM_RAM_DUMPS) {
		LOGI("%s: Ramdumps requested\n",dev->mdm_name);
		dev->required_action = MDM_REQUIRED_ACTION_RAMDUMPS;
	} else {
		LOGE("%s: Unknown boot_status returned", dev->mdm_name);
		return RET_FAILED;
	}

    if (data_9x15->efs_ks_pid > 0) {
        LOGE("%s: Issuing kill(%i) for EFS Sync process\n",
			dev->mdm_name, data_9x15->efs_ks_pid);
        kill(data_9x15->efs_ks_pid, SIGTERM);
        wait(&rcode);
        LOGE("%s: EFS Sync process should be dead", dev->mdm_name);
        data_9x15->efs_ks_pid = 0;
    }
	return RET_SUCCESS;
}

int reboot_9x15(struct mdm_device *dev)
{
    dev->required_action = MDM_REQUIRED_ACTION_NONE;
    return power_up_9x15(dev);
}

int prepare_9x15_for_ramdumps(struct mdm_device *dev)
{
    dev->required_action = MDM_REQUIRED_ACTION_RAMDUMPS;
	return RET_SUCCESS;
}

int collect_ramdumps_from_9x15(struct mdm_device *dev)
{
	int rcode;
	rcode = LoadSahara(dev, "-m");
	if (rcode != RET_SUCCESS) {
		LOGE("%s: Failed to collect RAM dumps", dev->mdm_name);
	}
	LOGI("%s: sending ram_dump_done ioctl\n", dev->mdm_name);
	if (ioctl(dev->device_descriptor, RAM_DUMP_DONE, &rcode) < 0) {
		LOGE("%s: Failed to send RAM_DUMP_DONE ioctl\n", dev->mdm_name);
		return RET_FAILED;
	}
	return RET_SUCCESS;
}

int mdm_9x15_post_ramdump_cleanup(struct mdm_device *dev)
{
	int boot_status = 0;
	LOGE("%s : 9x15_post_ramdump_collect *Nothing to do*",
			dev->mdm_name);
	dev->required_action = MDM_REQUIRED_ACTION_NONE;

	if (ioctl(dev->device_descriptor, WAIT_FOR_RESTART, &boot_status) < 0) {
		LOGE("%s: WAIT_FOR_RESTART ioctl fail\n", dev->mdm_name);
		return RET_FAILED;
	}
	if (boot_status != CHARM_NORMAL_BOOT) {
		LOGE("%s: Unexpected action(%d) recieved from driver",
		dev->mdm_name,
		boot_status);
		return RET_FAILED;

	}
	return RET_SUCCESS;
}

static int Bring9x15OutOfReset(int fd)
{
	int mdm_status = 0;
	LOGI("Bringing up modem");
	if (ioctl(fd, WAKE_CHARM) < 0) {
		LOGE("%Failed to issue ioctl to bring up modem");
		return RET_FAILED;
	}
	return RET_SUCCESS;
}

static int PrependHeaders(struct mdm_device *dev)
{

    char header_file_string[512];
    char binary_file_string[512];
    char command_string[2048];
    int i;
    struct private_data_9x15 *data_9x15 =
	    (struct private_data_9x15 *) dev->private_data;

    // First dump any partitions to files
    for (i = 0; i < NUM_EFS_PARTITIONS; i++) {
        if (snprintf(command_string,
                     sizeof(command_string),
                     "dd if=%s of=%s bs=1024 count=%d",
                     data_9x15->partition_list[i].partition,
                     data_9x15->partition_list[i].header_info.binary_file,
                     data_9x15->partition_list[i].kb) >
			sizeof(command_string)) {
            LOGE("%s: String was truncated.",dev->mdm_name);
            return RET_FAILED;
        }
        LOGE("%s: Running %s", dev->mdm_name, command_string);
        system(command_string);
    }

    // Concatenate headers for the partitions dumped out
    for (i = 0; i < NUM_EFS_PARTITIONS; i++) {
        if (snprintf(command_string,
                     sizeof(command_string),
                     "cat %s%s %s >%s",
                     dev->images_path,
                     data_9x15->partition_list[i].header_info.header_file,
                     data_9x15->partition_list[i].header_info.binary_file,
                     data_9x15->partition_list[i].header_info.file_details.\
		     filename) > sizeof(command_string)) {
            LOGE("%s: String was truncated.", dev->mdm_name);
            return RET_FAILED;
        }
        LOGE("%s: Running %s", dev->mdm_name, command_string);
        system(command_string);
    }

    for (i = 0; i < NUM_OTHER_HEADER_PREPEND_FILES; i++) {
        if (snprintf(command_string,
                     sizeof(command_string),
                     "cat %s%s %s%s >%s",
                     dev->images_path,
                     data_9x15->other_prepend_images[i].header_file,
                     dev->images_path,
                     data_9x15->other_prepend_images[i].binary_file,
                     data_9x15->other_prepend_images[i].file_details.filename) >
			sizeof(command_string)) {
            LOGE("%s: String was truncated.", dev->mdm_name);
            return RET_FAILED;
        }
        LOGE("%s: Running %s", dev->mdm_name, command_string);
        system(command_string);
    }
    return RET_SUCCESS;
}

static int LoadSahara(struct mdm_device *dev, char* options)
{
    char command_string[2048];
    char temp_string[256];
    struct private_data_9x15 *data_9x15 =
	    (struct private_data_9x15 *) dev->private_data;
    int i;
    int rcode;

    LOGE("%s: Loading Sahara images", dev->mdm_name);
    if(WaitForCOMport(data_9x15->flashless_boot_device, 0) == RET_FAILED) {
        LOGE("%s: Could not find flashless boot port", dev->mdm_name);
        return RET_FAILED;
    }

    if (snprintf(command_string,
             sizeof(command_string),
             "%s %s -w %s -p %s -r %d -g %s",
             KS_PATH,
             options,
             dev->ram_dump_path,
             data_9x15->flashless_boot_device,
             RAM_DUMP_IMAGE,
             data_9x15->file_prefix) > sizeof(command_string)) {
        LOGE("%s: String was truncated.", dev->mdm_name);
        return RET_FAILED;
    }

    for (i = 0; data_9x15->image_list[i].filename != NULL; i++) {
        if (snprintf(temp_string,
                     sizeof(temp_string),
                     " -s %d:%s%s",
                     data_9x15->image_list[i].image_id,
                     dev->images_path,
                     data_9x15->image_list[i].filename) > sizeof(temp_string)) {
            LOGE("%s: String was truncated.", dev->mdm_name);
            return RET_FAILED;
        }
        strlcat(command_string, temp_string, sizeof(command_string));
    }

    for (i = 0; i < NUM_EFS_PARTITIONS; i++) {
        if (snprintf(temp_string,
                     sizeof(temp_string),
                     " -s %d:%s",
                     data_9x15->partition_list[i].header_info.\
		     file_details.image_id,
                     data_9x15->partition_list[i].header_info.\
		     file_details.filename) > sizeof(temp_string)) {
            LOGE("%s: String was truncated.", dev->mdm_name);
            return RET_FAILED;
        }
        strlcat(command_string, temp_string, sizeof(command_string));
    }

    for (i = 0; i < NUM_OTHER_HEADER_PREPEND_FILES; i++) {
        if (snprintf(temp_string,
                     sizeof(temp_string),
                     " -s %d:%s",
                     data_9x15->other_prepend_images[i].file_details.image_id,
                     data_9x15->other_prepend_images[i].file_details.filename) >
			sizeof(temp_string)) {
            LOGE("%s: String was truncated.", dev->mdm_name);
            return RET_FAILED;
        }
        strlcat(command_string, temp_string, sizeof(command_string));
    }
    LOGE("%s: Running '%s'", dev->mdm_name, command_string);

    rcode  = system(command_string);
    LOGI("%s: RetCode = %u", dev->mdm_name, rcode);

    if(rcode != 0) {
        if (rcode == 1280) {
            LOGE("%s: ERROR: RAM dumps were forced unexpectedly",
			    dev->mdm_name);
        }
        else {
            LOGE("%s: ERROR: ks return code was %d, something failed",
			    dev->mdm_name, rcode);
        }
        return RET_FAILED;
    }
    LOGE("%s: Sahara transfer completed successfully", dev->mdm_name);
    return RET_SUCCESS;
}

static int find_file(const char *filename, char* temp_string, 
		size_t temp_string_size) {
    int i;
    struct stat stat_buf;

    if (stat(filename, &stat_buf) >= 0)
        return RET_SUCCESS;

    for (i = 0; i < sizeof(search_paths)/sizeof(char *); i++) {
        if (snprintf(temp_string,
                     temp_string_size,
                     "%s%s%s",
                     search_paths[i],
                     (search_paths[i][strlen(search_paths[i]) - 1] == '/')\
		     ? "" : "/",
                     filename) >= temp_string_size) {
            LOGE("String was truncated.");
            return RET_FAILED;
        }
        if (stat(temp_string, &stat_buf) < 0) {
            LOGE("File '%s' was not found", temp_string);
        }
        else {
            return RET_SUCCESS;
        }
    }
    LOGE("File '%s' was not found", filename);
    return RET_FAILED;
}

int WaitForCOMport(char *DevNode, int attempt_read)
{
    struct stat status_buf;
    int i;

    LOGI("Testing if port \"%s\" exists", DevNode);
    for (i = 0; i < NUM_RETRIES && stat(DevNode, &status_buf) < 0; i++) {
        LOGE("Couldn't find \"%s\", %i of %i", DevNode, i+1, NUM_RETRIES);
        usleep(DELAY_BETWEEN_RETRIES_MS * 1000);
    }
    if (i == NUM_RETRIES) {
        LOGE("'%s' was not found", DevNode);
        return RET_FAILED;
    }
    if (attempt_read) {
        FILE *fd;
        LOGI("Attempting to open port \"%s\" for reading", DevNode);
        for (i=0; i<NUM_RETRIES && (fd = fopen(DevNode,"r"))==NULL; i++) {
            LOGE("Couldn't read \"%s\", %i of %i", DevNode, i+1, NUM_RETRIES);
            usleep(DELAY_BETWEEN_RETRIES_MS*1000);
        }
        if (i == NUM_RETRIES) {
            LOGE("'%s' could not be opened for reading", DevNode);
            return RET_FAILED;
        }
        fclose(fd);
    }
    return RET_SUCCESS;
}

