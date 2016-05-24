/*
 * Copyright (c) 2013, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef __SYSTEM_CORE_MMI_DIAG__
#define __SYSTEM_CORE_MMI_DIAG__

#include <msg.h>
#include <log.h>

/* Subsystem command codes for the test app  */
#define FTM_AP_C  52

#define SIZE_OF_TEST_CASE_NAME 256

#define WRITE_FILENAME_OFFSET 10
#define WRITE_APPEND_DATA_OFFSET(filename) (WRITE_FILENAME_OFFSET + strlen(filename) + 1)
#define WRITE_ISIZE_OFFSET(filename) (WRITE_APPEND_DATA_OFFSET(filename) + 1)
#define WRITE_DATA_OFFSET(filename) (WRITE_ISIZE_OFFSET(filename) + 2)


#define FTM_AP_LOG_PKT_ID 0x1125    /* FTM Sublog code under FTM_LOG_V2 (0x117C) */

enum {
    FTM_AP_TEST_APP_STATUS = 0,
    FTM_AP_SELECT_SEQUENCE = 1,
    FTM_AP_CLEAR_RESULTS = 2,
    FTM_AP_EXECUTE_SINGLE_TEST = 3,
    FTM_AP_EXECUTE_ALL_TESTS = 4,
    FTM_AP_READ_FILE = 5,
    FTM_AP_WRITE_FILE = 6,
    FTM_AP_ERASE_ALL_FILES = 7,
    FTM_AP_TEST_LIST_TO_FILE = 8,
    FTM_AP_DIR_TO_FILE = 9
};

/* Subsystem command codes for the test app  */
#define FTM_DEVICE_INFO  54
enum {
    FTM_USB_SERIALNUM = 0,
};


#define DIRECTORY_FILENAME  "directory.txt"
#define TESTLIST_FILENAME  "testlist.txt"
#define PACK_SIZE 1024
typedef enum {
    APP_IDLE = 0,
    APP_EXECUTING
} app_status;

enum {
    FTM_SUCCESS = 0,
    FTM_FAIL = 1,
};

/* FTM Error code for FTM_AP_EXECUTE_SINGLE_TEST and FTM_AP_EXECUTE_ALL_TESTS*/
enum {
    FTM_NO_ERROR = 0,
    FTM_ERROR_TEST_CASE_NOT_FOUND,
    FTM_ERROR_MMI_IN_EXECUTION,
    FTM_ERROR_UNKNOWN,
};

/*  Read command
 0 = Success
 1 = Error
 2 = Error: Offset is invalid
 3 = Error: File does not exist
 4 = Error: File exists, but cannot be opened.
 */

enum {
    READ_SUCCESS = 0,
    READ_ERR = 1,
    READ_BAD_OFFSET_ERR = 2,
    READ_FILE_NOT_EXIST_ERR = 3,
    READ_OPEN_ERR = 4,
};

/*	Write command
0 = Success
1 = Error
2 = Error: File cannot be opened
3 = Warning: The previous file has not been closed.
*/
enum {
    WRITE_SUCCESS = 0,
    WRITE_ERR = 1,
    WRITE_OPEN_ERR = 2,
    WRITE_FILE_CLOSED_ERR = 3,
};

typedef enum {
    CFG_SUCCESS = 0,
    CFG_NOT_FOUND,
    CFG_FORMAT_ERROR,
    CFG_CASE_ERROR,
    CFG_DEFUAL_NOT_FOUND,
} seq_result;

typedef struct {
    unsigned char cmd_code;
    unsigned char sub_sys_id;
    unsigned short sub_sys_cmd_code;
    unsigned short ftm_cmd_id;
    unsigned short ftm_data_len;
    unsigned short ftm_rsp_pkt_size;
} __attribute__((packed)) ftm_header;

/*read command request
CMD_CODE	                        Unsigned / 1 byte	    Command ID  - Set CMD_CODE to 75
SUB_SYS_ID	                        Unsigned / 1 byte	    Subsystem ID - FTM ID is 11
SUBSYS_CMD_CODE	               Unsigned / 2 bytes	    FTM Mode ID - FTM_AP_C (52)
FTM_CMD_ID	                        Unsigned / 2 bytes	    5 - FTM_AP_READ_FILE
FTM_DATA_LEN	                   Unsigned / 2 bytes	    Unused, set to 0
FTM_RSP_PKT_SIZE	              Unsigned / 2 bytes	    Unused, set to 0
OFFSET	                            Unsigned / 4 bytes	    The offset of the file location
MAX_SIZE	                            Unsigned / 2 bytes	     The maximum number of bytes to transfer.  The size is subject to the diag buffer limitation
FILENAME	                            Variable ASCII             Null terminated	The file to be read. The filename cannot contain any path "/"
*/
typedef struct {
    ftm_header sftm_header;
    unsigned int offset;
    unsigned short max_size;
    char filename[256];
} __attribute__((packed)) ftm_read_file_req;

/*single test command request
CMD_CODE	                                     Unsigned / 1 byte	      Command ID - Set CMD_CODE to 75
SUB_SYS_ID	                                     Unsigned / 1 byte	      Subsystem ID - FTM ID is 11
SUBSYS_CMD_CODE	                            Unsigned / 2 bytes	      FTM Mode ID - FTM_AP_C (52)
FTM_CMD_ID	                                     Unsigned / 2 bytes	      3  - FTM_ AP_EXECUTE_SINGLE_TEST
FTM_DATA_LEN	                                 Unsigned / 2 bytes	      Unused, set to 0
FTM_RSP_PKT_SIZE	                            Unsigned / 2 bytes	      Unused, set to 0
TEST_CASE	                                      Variable ASCII              Null terminated	String name of the test case defined in sequence file (.seq)
*/
typedef struct {
    ftm_header sftm_header;
    char test_case[256];
} __attribute__((packed)) ftm_single_test_case_req;

/*select sequence command request
CMD_CODE	                                     Unsigned / 1 byte	      Command ID - Set CMD_CODE to 75
SUB_SYS_ID	                                     Unsigned / 1 byte	      Subsystem ID - FTM ID is 11
SUBSYS_CMD_CODE	                            Unsigned / 2 bytes	      FTM Mode ID - FTM_AP_C (52)
FTM_CMD_ID	                                     Unsigned / 2 bytes	      3  - FTM_ AP_EXECUTE_SINGLE_TEST
FTM_DATA_LEN	                                 Unsigned / 2 bytes	      Unused, set to 0
FTM_RSP_PKT_SIZE	                            Unsigned / 2 bytes	      Unused, set to 0
FILENAME	                                          Variable ASCII              Null terminated	String name of the test case defined in sequence file (.seq)
*/
typedef struct {
    ftm_header sftm_header;
    char filename[256];
} __attribute__((packed)) ftm_select_seq_req;

typedef struct {
    ftm_header sftm_header;
    unsigned short ftm_error_code;
    unsigned long file_size;
    unsigned long offset;
    unsigned short size;
    unsigned char Data[PACK_SIZE];
}__attribute__((packed)) ftm_read_file_response;

typedef struct {
    ftm_header sftm_header;
    unsigned short ftm_error_code;
    unsigned char state;        //   "   0 = Idle   "   1 = Executing
    unsigned long fail_count;   //Returns the number of test cases that have failed since the status was resetted by FTM_MMIFTM_AP_CLEAR_ALL_STATUS.
}__attribute__((packed)) ftm_ap_status_response;

typedef struct {
    ftm_header sftm_header;
    unsigned short ftm_error_code;
}__attribute__((packed))  ftm_common_response;

typedef struct {
    ftm_header sftm_header;
    unsigned short ftm_error_code;
    unsigned short iNumTests;   // num of test from the sequence
}__attribute__((packed)) ftm_select_sequence_response;

typedef struct {
    ftm_header sftm_header;
    unsigned short ftm_error_code;
    char serialnum[0];
}__attribute__((packed)) ftm_usb_serialnum_response;

typedef PACKED struct {
    log_hdr_type hdr;
    word ftm_log_id;            /* FTM log id */
    int test_result;            /* result (0==PASS), else Error Code */
    byte data[SIZE_OF_TEST_CASE_NAME];  /* Variable length payload, test_case nam look at FTM log id for contents */
}__attribute__((packed)) ftm_ap_log_pkt_type;

#define FTM_AP_LOG_HEADER_SIZE (sizeof(ftm_ap_log_pkt_type) - SIZE_OF_TEST_CASE_NAME)


int diag_deinit(void);
int diag_init(void);

#endif
