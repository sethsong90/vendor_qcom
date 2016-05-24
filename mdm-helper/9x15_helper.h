/*
 * ----------------------------------------------------------------------------
 *  Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 *  Qualcomm Technologies Proprietary and Confidential.
 *  ---------------------------------------------------------------------------
 *
 *  9x15_helper.h : Functions used by mdm_helper to  monitor and interact with 9x15
 *  module
 *
 */
#include "mdm_helper.h"

#define TEMP_FILE_DIR "/data/misc/mdmhelperdata/"
#define EFS_FILE_DIR "/dev/block/platform/msm_sdcc.1/by-name/"
#define KS_PATH "/system/bin/ks"
#define RAM_DUMP_IMAGE 21
#define NUM_EFS_PARTITIONS 3
#define NUM_OTHER_HEADER_PREPEND_FILES 1

struct image_id_mapping {
    int image_id;
    char* filename;
};

struct headers_to_prepend {
    struct image_id_mapping file_details;
    char* header_file;
    char* binary_file;
};
struct partitions_to_file_dump {
    struct headers_to_prepend header_info;
    char *partition;
    size_t kb;
};

struct private_data_9x15 {
    char* flashless_boot_device;
    char* efs_sync_device;
    char* file_prefix;
    pid_t efs_ks_pid;
    struct partitions_to_file_dump partition_list[NUM_EFS_PARTITIONS];
    struct headers_to_prepend other_prepend_images[NUM_OTHER_HEADER_PREPEND_FILES];
    struct image_id_mapping image_list[];
};

/*The below function declararions/definitions will eventually
 * be moved into a seperate file (9x15_helper.c/.h)*/
int power_up_9x15(struct mdm_device *);
int monitor_9x15(struct mdm_device *);
int reboot_9x15(struct mdm_device *);
int prepare_9x15_for_ramdumps(struct mdm_device *);
int collect_ramdumps_from_9x15(struct mdm_device *);
int mdm_9x15_post_ramdump_cleanup(struct mdm_device *);

extern struct private_data_9x15 private_data_9x15_hsic;
extern struct private_data_9x15 private_data_9x15_hsic_dsda;
extern struct private_data_9x15 private_data_9x15_usb_dsda;

