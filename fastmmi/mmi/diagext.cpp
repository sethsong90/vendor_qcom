/*
 * Copyright (c) 2013-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include "msg.h"
#include "diag_lsm.h"
#include "stdio.h"
#include "diagpkt.h"
#include "string.h"
#include <unistd.h>
#include "diag.h"
#include "diagcmd.h"
#include <dirent.h>
#include <sys/stat.h>
#include <cutils/properties.h>
#include <mmi_module_manage.h>
#include <pthread.h>
#include <semaphore.h>
#include "diagext.h"
#include "mmi.h"
#include "mmi_utils.h"

static const char *TAG = "mmi_diag";

static const char baseDir[255] = BASE_DIR;
static sem_t g_diag_work;
static char g_cur_testcase[256] = { 0 };

static int g_icmd = 0;

/*===========================================================================*/

/* Local Function declarations */

/*===========================================================================*/
PACKED void *ftm_app_status(PACK(void *)req_pkt, uint16 pkt_len);
PACKED void *ftm_select_sequence(PACK(void *)req_pkt, uint16 pkt_len);
PACKED void *ftm_clear_results(PACK(void *)req_pkt, uint16 pkt_len);
PACKED void *ftm_execute_single_test(PACK(void *)req_pkt, uint16 pkt_len);
PACKED void *ftm_execute_all_tests(PACK(void *)req_pkt, uint16 pkt_len);
PACKED void *ftm_read_file(PACK(void *)req_pkt, uint16 pkt_len);
PACKED void *ftm_write_file(PACK(void *)req_pkt, uint16 pkt_len);
PACKED void *ftm_test_list_to_file(PACK(void *)req_pkt, uint16 pkt_len);
PACKED void *ftm_erase_all_files(PACK(void *)req_pkt, uint16 pkt_len);
PACKED void *ftm_dir_to_file(PACK(void *)req_pkt, uint16 pkt_len);
PACKED void ftm_ap_send_log_msg(int res, const char *sTestCase);

PACK(void *) get_usb_serialno(PACK(void *)req_pkt, uint16 pkt_len);
PACK(void *) ftm_ap_dispatch(PACK(void *)req_pkt, uint16 pkt_len);



/*  FFBM related function */
bool is_file(const char *path) {
    struct stat st;

    if(lstat(path, &st) == 0)
        return S_ISREG(st.st_mode) != 0;
    return false;
}

/*Erase all files under FTM_AP */
static int erase_all_files() {

    struct dirent *de;
    DIR *dir;
    char filepath[PATH_MAX];

    dir = opendir(baseDir);
    if(dir != 0) {
        while((de = readdir(dir))) {
            if((strncmp(de->d_name, ".", 2) == 0)
               || (strncmp(de->d_name, "..", 3) == 0))
                continue;
            strlcpy(filepath, baseDir, sizeof(filepath));
            if(filepath[strlen(baseDir) - 1] != '/')
                strlcat(filepath, "/", sizeof(filepath));
            strlcat(filepath, de->d_name, sizeof(filepath));
            if(is_file(filepath))
                remove(filepath);
        }
        closedir(dir);
    }
    return FTM_SUCCESS;
}

/*List the FTM_AP directory contents to a file directory.Txt,
in the FTM_AP directory(see 3.3.1.1).  The directory.Txt file
can later be retrieved to desktop with FTM_AP_READ_FILE command.*/
static int dir_list_filename_to_file(const char *filename) {
    struct dirent *de;
    DIR *dir;
    FILE *fp = NULL;
    char filepath[PATH_MAX];
    char buf[255];

    memset(buf, 0, 255);

    strlcpy(filepath, baseDir, sizeof(filepath));
    if(filepath[strlen(baseDir) - 1] != '/')
        strlcat(filepath, "/", sizeof(filepath));
    strlcat(filepath, filename, sizeof(filepath));

    fp = fopen(filepath, "w");
    if(!fp)
        return FTM_FAIL;

    dir = opendir(baseDir);
    if(dir != 0) {
        while((de = readdir(dir))) {
            if((strncmp(de->d_name, ".", 1) == 0 && strlen(de->d_name) == 1)
               || (strncmp(de->d_name, "..", 2) == 0 && strlen(de->d_name) == 2))
                continue;

            ALOGE("%s \n", de->d_name);
            snprintf(buf, sizeof(buf), "%s\n", de->d_name);
            fwrite(buf, strlen(buf), 1, fp);
        }
        closedir(dir);
    }
    fclose(fp);
    return FTM_SUCCESS;
}

static const diagpkt_user_table_entry_type ftm_fastmmi_diag_func_table[] = {    /* susbsys_cmd_code lo = 0 , susbsys_cmd_code hi = 0, call back function */
    {FTM_AP_C, FTM_AP_C, ftm_ap_dispatch},
};

static const diagpkt_user_table_entry_type ftm_usb_func_table[] = { /* subsys cmd low, subsys cmd code high, call back function */
    {FTM_DEVICE_INFO, FTM_DEVICE_INFO, get_usb_serialno},
};

int diag_deinit(void) {
    sem_close(&g_diag_work);
    return Diag_LSM_DeInit();
}

int thread_execute_diag_request(int icmd, char *params) {

    if(icmd == FTM_AP_EXECUTE_SINGLE_TEST) {
        mmi_module *module = get_module_by_domain(params);

        /* Can not find the test. Return error. */
        if(module == NULL)
            return FTM_ERROR_TEST_CASE_NOT_FOUND;

        if(get_mmi_state() == MMI_IDLE) {

            strlcpy(g_cur_testcase, params, sizeof(g_cur_testcase));
            sem_post(&g_diag_work);
            g_icmd = icmd;

            return FTM_NO_ERROR;
        } else if(get_mmi_state() == MMI_BUSY) {
            return FTM_ERROR_MMI_IN_EXECUTION;
        }
    } else if(icmd == FTM_AP_EXECUTE_ALL_TESTS) {
        if(get_mmi_state() == MMI_IDLE) {
            ALOGE("run all tests\n");
            sem_post(&g_diag_work);
            g_icmd = icmd;
            return FTM_NO_ERROR;
        } else
            return FTM_ERROR_MMI_IN_EXECUTION;
    }
    return FTM_ERROR_UNKNOWN;
}


void *diag_thread(void *) {
    int ret;

    while(1) {
        sem_wait(&g_diag_work);
        ALOGE("icmd: %d\n", g_icmd);
        if(g_icmd == FTM_AP_EXECUTE_ALL_TESTS) {
            ret = excute_all_test();
            ftm_ap_send_log_msg(ret, "ALL TEST");
        } else if(g_icmd == FTM_AP_EXECUTE_SINGLE_TEST) {
            ret = excute_single_test(g_cur_testcase);
            ftm_ap_send_log_msg(ret, g_cur_testcase);
        }
    }
    return NULL;
}


/*===========================================================================*/

/* Main Function. This initializes Diag_LSM, calls the tested APIs and exits. */

/*===========================================================================*/
int diag_init(void) {
    boolean bInit_Success = FALSE;
    int res;
    pthread_t ptid;

    ALOGE("\n\t\t=====================");
    ALOGE("\n\t\tStarting fastmmi Test App");
    ALOGE("\n\t\t=====================");

    ALOGE("\n Calling LSM init \n");

    /* Calling LSM init  */
    bInit_Success = Diag_LSM_Init(NULL);

    if(!bInit_Success) {
        ALOGE("fastmmi Test App: Diag_LSM_Init() failed.");
        return -1;
    }

    ALOGE("fastmmi Test App: Diag_LSM_Init succeeded. \n");

    /* Registering diag packet with no subsystem id. This is so
     * that an empty request to the app. gets a response back
     * and we can ensure that the diag is working as well as the app. is
     * responding subsys id = 11, table = test_tbl_2,
     * To execute on QXDM :: "send_data 75 11 0 0 0 0 0 0"
     OR
     * To execute on QXDM :: "send_data 75 11 3 0 0 0 0 0"
     */
    DIAGPKT_DISPATCH_TABLE_REGISTER(DIAG_SUBSYS_FTM, ftm_fastmmi_diag_func_table);
    DIAGPKT_DISPATCH_TABLE_REGISTER(DIAG_SUBSYS_FTM, ftm_usb_func_table);

    /*start a diag thread to deal with diag command */
    sem_init(&g_diag_work, 0, 0);
    res = pthread_create(&ptid, NULL, diag_thread, NULL);
    if(res < 0) {
        perror("creat diag thread fail \n");
        return -1;
    }
    return 0;
}


/*===========================================================================*/

/* dummy registered functions */

/*===========================================================================*/

PACK(void *) ftm_ap_dispatch(PACK(void *)req_pkt, uint16 pkt_len) {
    PACK(void *) rsp = NULL;

    ftm_header *pheader = (ftm_header *) req_pkt;
    unsigned short iCmd = pheader->ftm_cmd_id;

    ALOGE("\n  start to process FTM_CMD_ID iCmd=%d\n", iCmd);
    switch (iCmd) {
    case FTM_AP_TEST_APP_STATUS:
        rsp = ftm_app_status(req_pkt, pkt_len);
        break;
    case FTM_AP_SELECT_SEQUENCE:
        rsp = ftm_select_sequence(req_pkt, pkt_len);
        break;
    case FTM_AP_CLEAR_RESULTS:
        rsp = ftm_clear_results(req_pkt, pkt_len);
        break;
    case FTM_AP_EXECUTE_SINGLE_TEST:
        rsp = ftm_execute_single_test(req_pkt, pkt_len);
        break;
    case FTM_AP_EXECUTE_ALL_TESTS:
        rsp = ftm_execute_all_tests(req_pkt, pkt_len);
        break;
    case FTM_AP_READ_FILE:
        rsp = ftm_read_file(req_pkt, pkt_len);
        break;
    case FTM_AP_WRITE_FILE:
        rsp = ftm_write_file(req_pkt, pkt_len);
        break;
    case FTM_AP_TEST_LIST_TO_FILE:
        rsp = ftm_test_list_to_file(req_pkt, pkt_len);
        break;
    case FTM_AP_ERASE_ALL_FILES:
        rsp = ftm_erase_all_files(req_pkt, pkt_len);
        break;
    case FTM_AP_DIR_TO_FILE:
        rsp = ftm_dir_to_file(req_pkt, pkt_len);
        break;
    default:
        break;
    }

    return rsp;
}

PACKED void *ftm_app_status(PACK(void *)req_pkt, uint16 pkt_len) {
    ftm_ap_status_response *rsp = NULL;

    /* Allocate the same length as the request. */
    rsp = (ftm_ap_status_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_AP_C, sizeof(ftm_ap_status_response));

    if(rsp != NULL) {
        ALOGE("ftm_app_status: diagpkt_subsys_alloc succeeded");
    } else {
        ALOGE("ftm_app_status: diagpkt_subsys_alloc failed");
        return NULL;
    }


    rsp->state = get_mmi_state();
    rsp->fail_count = get_mmi_fail_count();

    rsp->ftm_error_code = FTM_SUCCESS;  /*success */

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_AP_C;
    rsp->sftm_header.ftm_cmd_id = FTM_AP_TEST_APP_STATUS;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;

    return rsp;
}

PACKED void *ftm_select_sequence(PACK(void *)req_pkt, uint16 pkt_len) {

    ftm_select_sequence_response *rsp = NULL;
    ftm_select_seq_req *req = NULL;
    char filepath[PATH_MAX];
    int iNumTestCase = 0;

    req = (ftm_select_seq_req *) req_pkt;

    /* Allocate the same length as the request. */
    rsp = (ftm_select_sequence_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_AP_C, sizeof(ftm_common_response));
    ALOGE("ftm_select_sequence: start \n");
    if(rsp != NULL) {
        ALOGE("ftm_select_sequence: diagpkt_subsys_alloc succeeded \n");
    } else {
        ALOGE("ftm_select_sequence: diagpkt_subsys_alloc failed \n");
        return NULL;
    }

    strlcpy(filepath, baseDir, sizeof(filepath));
    if(filepath[strlen(baseDir) - 1] != '/')
        strlcat(filepath, "/", sizeof(filepath));
    strlcat(filepath, req->filename, sizeof(filepath));
    rsp->ftm_error_code = reconfig_mmi(filepath, &iNumTestCase);    /*success */

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_AP_C;
    rsp->sftm_header.ftm_cmd_id = FTM_AP_SELECT_SEQUENCE;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->iNumTests = iNumTestCase;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;
    ALOGE("ftm_select_sequence: end \n");

    return rsp;
}

PACKED void *ftm_clear_results(PACK(void *)req_pkt, uint16 pkt_len) {
    ftm_common_response *rsp = NULL;

    /* Allocate the same length as the request. */
    rsp = (ftm_common_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_AP_C, sizeof(ftm_common_response));

    if(rsp != NULL) {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc succeeded");
    } else {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc failed");
        return NULL;
    }

    rsp->ftm_error_code = clear_cur_test_result();  /*success */

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_AP_C;
    rsp->sftm_header.ftm_cmd_id = FTM_AP_CLEAR_RESULTS;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;
    return rsp;
}

PACKED void *ftm_execute_single_test(PACK(void *)req_pkt, uint16 pkt_len) {
    ftm_common_response *rsp = NULL;
    ftm_single_test_case_req *req = NULL;

    req = (ftm_single_test_case_req *) req_pkt;
    /* Allocate the same length as the request. */
    rsp = (ftm_common_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_AP_C, sizeof(ftm_common_response));

    if(rsp != NULL) {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc succeeded");
    } else {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc failed");
        return NULL;
    }
    rsp->ftm_error_code = thread_execute_diag_request(FTM_AP_EXECUTE_SINGLE_TEST, req->test_case);  /*success */

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_AP_C;
    rsp->sftm_header.ftm_cmd_id = FTM_AP_EXECUTE_SINGLE_TEST;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;

    return rsp;
}


PACKED void *ftm_execute_all_tests(PACK(void *)req_pkt, uint16 pkt_len) {
    ftm_common_response *rsp = NULL;

    /* Allocate the same length as the request. */
    rsp = (ftm_common_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_AP_C, sizeof(ftm_common_response));

    if(rsp != NULL) {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc succeeded");
    } else {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc failed");
        return NULL;
    }

    rsp->ftm_error_code = thread_execute_diag_request(FTM_AP_EXECUTE_ALL_TESTS, NULL);  /*success */

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_AP_C;
    rsp->sftm_header.ftm_cmd_id = FTM_AP_EXECUTE_ALL_TESTS;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;

    return rsp;
}


PACKED void *ftm_read_file(PACK(void *)req_pkt, uint16 pkt_len) {

    ftm_read_file_response *rsp = NULL;
    ftm_read_file_req *req = NULL;
    FILE *fp = NULL;
    char filepath[255] = { 0 };
    unsigned short iSize = 0;
    unsigned char Data[PACK_SIZE];

    req = (ftm_read_file_req *) req_pkt;

    /*parse parameter */
    char *pFileName = req->filename;
    unsigned int offset = req->offset;
    unsigned short max_size = req->max_size;

    ALOGE("ftm_read_file: pFileName=%s,%d\n", pFileName, offset);
    /* Allocate the same length as the request. */
    rsp = (ftm_read_file_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_AP_C, sizeof(ftm_read_file_response));

    if(rsp != NULL) {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc succeeded");
    } else {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc failed");
        return NULL;
    }

    /*get dir */
    memset(Data, 0, sizeof(Data));
    strlcat(filepath, baseDir, sizeof(filepath));
    if(filepath[strlen(baseDir) - 1] != '/')
        strlcat(filepath, "/", sizeof(filepath));
    strlcat(filepath, pFileName, sizeof(filepath));


    fp = fopen(filepath, "rb");
    if(fp) {
        fseek(fp, 0, SEEK_END); /*non-portable */
        rsp->file_size = ftell(fp);

        ALOGE("ftm_read_file: offset=%d,filesize=%ld \n", offset, rsp->file_size);

        if(offset < rsp->file_size) {
            fseek(fp, offset, SEEK_SET);
            iSize = fread(Data, 1, max_size, fp);
            rsp->ftm_error_code = READ_SUCCESS;
        } else
            rsp->ftm_error_code = READ_BAD_OFFSET_ERR;

        fclose(fp);
        fp = NULL;
    } else {
        rsp->ftm_error_code = READ_FILE_NOT_EXIST_ERR;
    }

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_AP_C;
    rsp->sftm_header.ftm_cmd_id = FTM_AP_READ_FILE;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;

    rsp->offset = offset;
    rsp->size = iSize;
    ALOGE("ftm_read_file: iSize=%d", iSize);
    memcpy(rsp->Data, Data, sizeof(rsp->Data));

    return rsp;
}

/* write file request struct
CMD_CODE	          Unsigned / 1 byte	            Command ID - Set CMD_CODE to 75
SUB_SYS_ID	          Unsigned / 1 byte	            Subsystem ID - FTM ID is 11
SUBSYS_CMD_CODE	          Unsigned / 2 bytes	            FTM Mode ID - FTM_AP_C (52)
FTM_CMD_ID	          Unsigned / 2 bytes	            6 - FTM_AP_WRITE_FILE
FTM_DATA_LEN	          Unsigned / 2 bytes	            Unused, set to 0
FTM_RSP_PKT_SIZE	  Unsigned / 2 bytes	            Unused, set to 0
FILENAME	          Variable length ASCII             Null terminated  The file to be read.
                                                            The filename cannot contain any path "/"
MORE_DATA	          Unsigned / 1 bytes	            0 = no more data   1 = more data
SIZE	                  Unsigned / 2 bytes	            The actual number of bytes transfer in DATA portion
DATA	                  Variable length binary            The data stream
*/
PACKED void *ftm_write_file(PACK(void *)req_pkt, uint16 pkt_len) {
    ftm_common_response *rsp = NULL;

    char filepath[255] = { 0 };
    static FILE *fp = NULL;
    unsigned char *pReq = (unsigned char *) req_pkt;
    char *filename = (char *) (pReq + WRITE_FILENAME_OFFSET);

    unsigned char append_data = *(unsigned char *) (pReq + WRITE_APPEND_DATA_OFFSET(filename));
    unsigned short i_size = *(unsigned short *) (pReq + WRITE_ISIZE_OFFSET(filename));
    unsigned char *pData = (unsigned char *) &pReq[WRITE_DATA_OFFSET(filename)];


    /* Allocate the same length as the request. */
    rsp = (ftm_common_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_AP_C, sizeof(ftm_common_response));

    if(rsp != NULL) {
        ALOGE("ftm_write_file: diagpkt_subsys_alloc succeeded");
    } else {
        ALOGE("ftm_write_file: diagpkt_subsys_alloc failed");
        return NULL;
    }

    strlcat(filepath, baseDir, sizeof(filepath));
    if(filepath[strlen(baseDir) - 1] != '/')
        strlcat(filepath, "/", sizeof(filepath));
    strlcat(filepath, filename, sizeof(filepath));

    if(!append_data)
        fp = fopen(filepath, "w");
    else
        fp = fopen(filepath, "a+");

    if(fp) {
        if(i_size > 0)
            fwrite(pData, 1, i_size, fp);

        ALOGE("write file: i_size=%d", i_size);
        rsp->ftm_error_code = WRITE_SUCCESS;
        fclose(fp);
        fp = NULL;
    } else {
        rsp->ftm_error_code = WRITE_OPEN_ERR;
    }

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_AP_C;
    rsp->sftm_header.ftm_cmd_id = FTM_AP_WRITE_FILE;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;

    return rsp;
}


PACKED void *ftm_test_list_to_file(PACK(void *)req_pkt, uint16 pkt_len) {
    ftm_common_response *rsp = NULL;
    char filepath[PATH_MAX];

    strlcpy(filepath, baseDir, sizeof(filepath));
    if(filepath[strlen(baseDir) - 1] != '/')
        strlcat(filepath, "/", sizeof(filepath));
    strlcat(filepath, TESTLIST_FILENAME, sizeof(filepath));
    /* Allocate the same length as the request. */
    rsp = (ftm_common_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_AP_C, sizeof(ftm_common_response));

    if(rsp != NULL) {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc succeeded");
    } else {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc failed");
        return NULL;
    }

    rsp->ftm_error_code = test_list_to_file(filepath);  /*success */

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_AP_C;
    rsp->sftm_header.ftm_cmd_id = FTM_AP_TEST_LIST_TO_FILE;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;
    return rsp;
}


PACKED void *ftm_erase_all_files(PACK(void *)req_pkt, uint16 pkt_len) {
    ftm_common_response *rsp = NULL;

    /* Allocate the same length as the request. */
    rsp = (ftm_common_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_AP_C, sizeof(ftm_common_response));

    if(rsp != NULL) {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc succeeded");
    } else {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc failed");
        return NULL;
    }

    rsp->ftm_error_code = erase_all_files();    /*success */

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_AP_C;
    rsp->sftm_header.ftm_cmd_id = FTM_AP_ERASE_ALL_FILES;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;
    return rsp;
}

PACKED void *ftm_dir_to_file(PACK(void *)req_pkt, uint16 pkt_len) {
    ftm_common_response *rsp = NULL;
    const char *filename = DIRECTORY_FILENAME;

    /* Allocate the same length as the request. */
    rsp = (ftm_common_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_AP_C, sizeof(ftm_common_response));

    if(rsp != NULL) {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc succeeded");
    } else {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc failed");
        return NULL;
    }

    rsp->ftm_error_code = dir_list_filename_to_file(filename);  /*success */

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_AP_C;
    rsp->sftm_header.ftm_cmd_id = FTM_AP_DIR_TO_FILE;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;
    return rsp;
}

PACK(void *) get_usb_serialno(PACK(void *)req_pkt, uint16 pkt_len) {
    char value[PROPERTY_VALUE_MAX];
    ftm_usb_serialnum_response *rsp = NULL;

    property_get("ro.serialno", value, "");

    /* Allocate the same length as the request. */
    rsp =
        (ftm_usb_serialnum_response *) diagpkt_subsys_alloc(DIAG_SUBSYS_FTM, FTM_DEVICE_INFO,
                                                            sizeof(ftm_usb_serialnum_response) + strlen(value) + 1);

    if(rsp == NULL) {
        ALOGE("ftm_read_file: diagpkt_subsys_alloc failed");
        return NULL;
    }

    if(strlen(value) > 0) {
        strlcpy(rsp->serialnum, value, sizeof(rsp->serialnum));
        rsp->ftm_error_code = FTM_SUCCESS;  /*success */
    } else {
        rsp->ftm_error_code = FTM_FAIL; /*fail */
    }

    rsp->sftm_header.cmd_code = 75;
    rsp->sftm_header.sub_sys_id = DIAG_SUBSYS_FTM;
    rsp->sftm_header.sub_sys_cmd_code = FTM_DEVICE_INFO;
    rsp->sftm_header.ftm_cmd_id = FTM_USB_SERIALNUM;
    rsp->sftm_header.ftm_data_len = 0;
    rsp->sftm_header.ftm_rsp_pkt_size = 0;

    return rsp;
}

PACKED void ftm_ap_send_log_msg(int res, const char *sTestCase) {
    ftm_ap_log_pkt_type *ftm_ap_log_pkt_ptr = NULL;
    int result = log_status(LOG_FTM_VER_2_C);
    char testcase[SIZE_OF_TEST_CASE_NAME] = { 0 };
    strlcpy(testcase, sTestCase, sizeof(testcase));
    ALOGE("ftm_ap_send_log_msg: log_status result=%d, sTestCase=%s\n", result, testcase);
    ftm_ap_log_pkt_ptr = (ftm_ap_log_pkt_type *) log_alloc(LOG_FTM_VER_2_C, FTM_AP_LOG_HEADER_SIZE + strlen(testcase));

    if(result == 1) {
        if(ftm_ap_log_pkt_ptr != NULL) {
            ftm_ap_log_pkt_ptr->ftm_log_id = FTM_AP_LOG_PKT_ID;
            ftm_ap_log_pkt_ptr->test_result = res;
            /*copy the case name to log data */
            strlcpy((char *) ftm_ap_log_pkt_ptr->data, testcase, sizeof(ftm_ap_log_pkt_ptr->data));
            log_commit(ftm_ap_log_pkt_ptr);
        }
    }
}
