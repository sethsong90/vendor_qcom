/********************************************************************
---------------------------------------------------------------------
 Copyright (c) 2013-2014 Qualcomm Technologies, Inc.
 All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
----------------------------------------------------------------------
SecureUI Sample/Test Client app.
*********************************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/msm_ion.h>
#include <utils/Log.h>
#include "QSEEComAPI.h"
#include "common_log.h"
#include <getopt.h>
#include <sys/mman.h>
#include "SecureUI.h"
#include "secure_ui_defs.h"
#include <signal.h>
#include <SecureUILib.h>
#include <sys/eventfd.h>

//#define DISPLAY_TEST


/** adb log */
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "SECURE_UI_SAMPLE_CLIENT: "
#ifdef LOG_NDDEBUG
#undef LOG_NDDEBUG
#endif
#define LOG_NDDEBUG 0 //Define to enable LOGD
#ifdef LOG_NDEBUG
#undef LOG_NDEBUG
#endif
#define LOG_NDEBUG  0 //Define to enable LOGV

#define LOGD_PRINT(...) do { LOGD(__VA_ARGS__); printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGE_PRINT(...) do { LOGE(__VA_ARGS__); printf(__VA_ARGS__); printf("\n"); } while(0)

/* commands */
#define SEC_UI_SAMPLE_CMD0_START_SEC_UI     0
#define SEC_UI_SAMPLE_CMD1_SHOW_IMAGE       1
#define SEC_UI_SAMPLE_CMD2_STOP_DISP        2
#define SEC_UI_SAMPLE_CMD3_SHOW_IMGAGE_PTR  3

#define MAX_FILENAME_LEN (256)
#define SAMPLE_IMAGE1_PATH "/data/local/tmp/sec_img1__194_259_RGBA8888"
#define SAMPLE_IMAGE2_PATH "/data/local/tmp/sec_img2__196_244_RGBA8888"
#define SAMPLE_IMAGE1_PATH_LEN (strlen(SAMPLE_IMAGE1_PATH)+1)
#define SAMPLE_IMAGE2_PATH_LEN (strlen(SAMPLE_IMAGE2_PATH)+1)
#define SAMPLE_IMAGE1_HEIGHT 194
#define SAMPLE_IMAGE1_WIDTH 259
#define SAMPLE_IMAGE2_HEIGHT 196
#define SAMPLE_IMAGE2_WIDTH 244
#define SAMPLE_IMAGE_X 300
#define SAMPLE_IMAGE_Y 400

#define TOUCH_EVENT_TIMEOUT     10000   /* in mili seconds */
#define SLEEP_TIME_BEFORE_ABORT  30    /* in seconds */
#define TEST_ITERATION 1
#define FAILURE (-1)

struct send_cmd{
	uint32_t cmd_id;
	uint32_t height;
	uint32_t width;
	uint32_t x;
	uint32_t y;
	uint32_t timeout;
	char img_path[MAX_FILENAME_LEN];
 };

struct send_cmd_rsp{
	int32_t status;
 };

static int g_efd = -1;
static volatile int g_run = 1;

int32_t qsc_start_app(struct QSEECom_handle **l_QSEEComHandle,
                        const char *appname, int32_t buf_size)
{
	int32_t ret = 0;

	/* start the application */
	ret = QSEECom_start_app(l_QSEEComHandle, "/system/etc/firmware",
				appname, buf_size);
	if (ret) {
		LOGE_PRINT("Loading app -%s failed",appname);
	} else {
		LOGD("Loading app -%s succeded",appname);
		QSEECom_set_bandwidth(*l_QSEEComHandle, true);
	}

	return ret;
}

/**@brief:  Implement simple shutdown app
 * @param[in]	handle.
 * @return	zero on success or error count on failure.
 */
int32_t qsc_shutdown_app(struct QSEECom_handle **l_QSEEComHandle)
{
	int32_t ret = 0;

	LOGD("qsc_shutdown_app: start");
	QSEECom_set_bandwidth(*l_QSEEComHandle, false);
	/* shutdown the application */
	if (*l_QSEEComHandle != NULL) {
		ret = QSEECom_shutdown_app(l_QSEEComHandle);
		if (ret) {
			LOGE_PRINT("Shutdown app failed with ret = %d", ret);
		} else {
			LOGD("shutdown app: pass");
		}
	} else {
		LOGE_PRINT("cannot shutdown as the handle is NULL");
	}
	return ret;
}

int32_t issue_send_cmd(struct QSEECom_handle *l_QSEEComHandle,
                                         struct send_cmd *send_cmd)
{
	int32_t ret = 0;
	int32_t req_len = 0;
	int32_t rsp_len = 0;
	struct send_cmd_rsp *msgrsp;	/* response data sent from QSEE */

	/* populate the data in shared buffer */

	memcpy(l_QSEEComHandle->ion_sbuffer,send_cmd, sizeof(struct send_cmd));

	req_len = sizeof(struct send_cmd);
	rsp_len = sizeof(struct send_cmd_rsp);

	if (req_len & QSEECOM_ALIGN_MASK)
		req_len = QSEECOM_ALIGN(req_len);

	if (rsp_len & QSEECOM_ALIGN_MASK)
		rsp_len = QSEECOM_ALIGN(rsp_len);

	msgrsp=(struct send_cmd_rsp *)(l_QSEEComHandle->ion_sbuffer + req_len);
	/* send request from HLOS to QSEApp */
	ret = QSEECom_send_cmd(l_QSEEComHandle,
			l_QSEEComHandle->ion_sbuffer,
				req_len,
				msgrsp,
				rsp_len);
	if (ret) {
		LOGE_PRINT("send command %d failed with ret = %d\n", send_cmd->cmd_id,ret);
		return ret;
	}
	return msgrsp->status;
}

static void * abort_ui(void* arg){
	uint32_t * max_sleep_time = (uint32_t *) arg;
	int32_t sleep_time = rand() % (*max_sleep_time) +1;
	uint64_t c = 1;

	LOGD_PRINT("started abort ui thread, aborting in %d seconds", sleep_time);

	while ((sleep_time-- > 0) && (g_run))
		sleep(1);
	write(g_efd,&c,sizeof(c));
	abort_secure_ui();

	LOGD_PRINT("abort ui thread finished");
	return NULL;
}

int main(int argc, char *argv[])
{
	struct QSEECom_handle *l_QSEEComHandle = NULL;
	struct send_cmd cmd = {0};
	int input, i;
	int32_t ret;
	uint32_t rv;
	uint32_t sleeping_time = SLEEP_TIME_BEFORE_ABORT;
	pthread_t abort_ui_thread;
	uint8_t failed = 0;

	g_efd = eventfd(0, 0);
	if (g_efd == -1) {
		LOGE_PRINT("Failed to create eventfd: %s", strerror(errno));
		return errno;
	}

	ret = qsc_start_app(&l_QSEEComHandle,"secure_ui_sample", 1024);
	if (ret) {
		LOGE_PRINT("Start app: fail");
		return ret;
	} else {
		LOGD_PRINT("Start app: pass");
	}


	for(i = 0; i< TEST_ITERATION ; i++){
		g_run = 1;
		ret = pthread_create( &abort_ui_thread, NULL, abort_ui, &sleeping_time);
		if ( ret )
		{
			LOGE("Error: Creating abort_ui_thread thread failed!");
			failed = 1;
			break;
		}

		/* The first stage starts secure ui and shows image 1
		 * if the secure ui doen't start, we quit
		 * if showing the image fails, we abort the secure ui and quit */

		cmd.cmd_id = SEC_UI_SAMPLE_CMD0_START_SEC_UI;
		ret = issue_send_cmd(l_QSEEComHandle,&cmd);
		if(ret != SEC_UI_SUCCESS){
			LOGE_PRINT("   Failed SEC_UI_SAMPLE_CMD0_START_SEC_UI: %d",ret);
			failed = 1;
			break;
		}
		LOGD_PRINT("   Succeeded SEC_UI_SAMPLE_CMD0_START_SEC_UI (%d)",ret);

#ifdef DISPLAY_TEST
		cmd.cmd_id = SEC_UI_SAMPLE_CMD1_SHOW_IMAGE;
		cmd.height = SAMPLE_IMAGE1_HEIGHT;
		cmd.width = SAMPLE_IMAGE1_WIDTH;
		cmd.x = SAMPLE_IMAGE_X;
		cmd.y = SAMPLE_IMAGE_Y;
		strlcpy(cmd.img_path, SAMPLE_IMAGE1_PATH,SAMPLE_IMAGE1_PATH_LEN);
		ret = issue_send_cmd(l_QSEEComHandle,&cmd);
		if(ret < 0){
			LOGE_PRINT("   Failed SEC_UI_SAMPLE_CMD1_SHOW_IMAGE: 0 %d",ret);
			abort_secure_ui();
			break;
		}
		printf("   Succeeded SEC_UI_SAMPLE_CMD1_SHOW_IMAGE for the first image\n"
				"press enter to continue ... \n");
		getchar();
		cmd.cmd_id = SEC_UI_SAMPLE_CMD3_SHOW_IMGAGE_PTR;
		cmd.height = SAMPLE_IMAGE2_HEIGHT;
		cmd.width = SAMPLE_IMAGE2_WIDTH;
		cmd.x = SAMPLE_IMAGE_X;
		cmd.y = SAMPLE_IMAGE_Y;
		strlcpy(cmd.img_path, SAMPLE_IMAGE2_PATH,SAMPLE_IMAGE2_PATH_LEN);
		ret = issue_send_cmd(l_QSEEComHandle,&cmd);
		if(ret < 0){
			LOGE_PRINT("   Failed SEC_UI_SAMPLE_CMD1_SHOW_IMAGE_PTR: 0 %d",ret);
			abort_secure_ui();
			break;
		}
		printf("   Succeeded SEC_UI_SAMPLE_CMD1_SHOW_IMAGE_PTR for the second image\n"
				"press enter to continue ... \n");
		getchar();
		/* The second stage aborts the secure ui and tries to show image 2
		 * showing the image should fail, if not we try to send stop display command and quit */

		abort_secure_ui();
		printf("   Secure UI aborted! \n");

		cmd.cmd_id = SEC_UI_SAMPLE_CMD1_SHOW_IMAGE;
		cmd.height = SAMPLE_IMAGE2_HEIGHT;
		cmd.width = SAMPLE_IMAGE2_WIDTH;
		cmd.x = SAMPLE_IMAGE_X;
		cmd.y = SAMPLE_IMAGE_Y;
		strlcpy(cmd.img_path, SAMPLE_IMAGE2_PATH,SAMPLE_IMAGE2_PATH_LEN);
		ret = issue_send_cmd(l_QSEEComHandle,&cmd);
		if(ret != SEC_UI_NON_SECURE_BUFFER){
			LOGE_PRINT("    SEC_UI_SAMPLE_CMD1_SHOW_IMAGE didn't fail with SEC_UI_NON_SECURE_BUFFER ret = %d", ret);
			cmd.cmd_id = SEC_UI_SAMPLE_CMD2_STOP_DISP;
			ret = issue_send_cmd(l_QSEEComHandle,&cmd);
			if(ret < 0){
				LOGE_PRINT("   Failed SEC_UI_SAMPLE_CMD2_STOP_DISP: %d",ret);
				break;
			}
			printf("   Succeeded SEC_UI_SAMPLE_CMD2_STOP_DISP\n");
			break;
		}
		printf("   SEC_UI_SAMPLE_CMD1_SHOW_IMAGE failed as required \n"
				"press enter to continue ...\n");
		getchar();

		/*The last part of the test is the following sequence:
		 * 1. start the secure ui again
		 * 2. show image 2
		 * 3. switch to image 1
		 * 4. stop secure display with request
		 *
		 * at any stage if failed, abort the secure ui and quit  */

		//step 1
		cmd.cmd_id = SEC_UI_SAMPLE_CMD0_START_SEC_UI;
		ret = issue_send_cmd(l_QSEEComHandle,&cmd);
		if(ret < 0){
			LOGE_PRINT("   Failed SEC_UI_SAMPLE_CMD0_START_SEC_UI: %d",ret);
			break;
		}
		printf("   Succeeded SEC_UI_SAMPLE_CMD0_START_SEC_UI\n");

		//step 2
		cmd.cmd_id = SEC_UI_SAMPLE_CMD1_SHOW_IMAGE;
		cmd.height = SAMPLE_IMAGE2_HEIGHT;
		cmd.width = SAMPLE_IMAGE2_WIDTH;
		cmd.x = SAMPLE_IMAGE_X;
		cmd.y = SAMPLE_IMAGE_Y;
		strlcpy(cmd.img_path, SAMPLE_IMAGE2_PATH,SAMPLE_IMAGE2_PATH_LEN);
		ret = issue_send_cmd(l_QSEEComHandle,&cmd);
		if(ret < 0){
			LOGE_PRINT("   Failed SEC_UI_SAMPLE_CMD1_SHOW_IMAGE: 0 %d",ret);
			abort_secure_ui();
			break;
		}
		printf("   Succeeded SEC_UI_SAMPLE_CMD1_SHOW_IMAGE for the second image\n"
				"press enter to show image 1 ... \n");
		getchar();

		//step 3
		cmd.cmd_id = SEC_UI_SAMPLE_CMD1_SHOW_IMAGE;
		cmd.height = SAMPLE_IMAGE1_HEIGHT;
		cmd.width = SAMPLE_IMAGE1_WIDTH;
		cmd.x = SAMPLE_IMAGE_X;
		cmd.y = SAMPLE_IMAGE_Y;
		strlcpy(cmd.img_path, SAMPLE_IMAGE1_PATH,SAMPLE_IMAGE1_PATH_LEN);
		ret = issue_send_cmd(l_QSEEComHandle,&cmd);
		if(ret < 0){
			LOGE_PRINT("   Failed SEC_UI_SAMPLE_CMD1_SHOW_IMAGE: 0 %d",ret);
			abort_secure_ui();
			break;
		}
		printf("   Succeeded SEC_UI_SAMPLE_CMD1_SHOW_IMAGE for the first image\n"
				"press enter to continue ... \n");
		getchar();

#endif

		LOGD_PRINT("Starting secure touch test");
		rv = UseSecureTouch(g_efd, l_QSEEComHandle);

		if(0 == rv){
			LOGD_PRINT("touch test succeeded!");
		} else {
			LOGE_PRINT("touch test aborted!, (%d)",rv);
			abort_secure_ui();
			break;
		}
		//step 4
		cmd.cmd_id = SEC_UI_SAMPLE_CMD2_STOP_DISP;
		ret = issue_send_cmd(l_QSEEComHandle,&cmd);
		if(ret < 0){
			LOGE_PRINT("   Failed SEC_UI_SAMPLE_CMD2_STOP_DISP: %d",ret);
		} else {
			LOGD_PRINT("   Succeeded SEC_UI_SAMPLE_CMD2_STOP_DISP");
		}

		g_run = 0;
		if  (pthread_join(abort_ui_thread, NULL) == FAILURE){
			LOGE_PRINT("Error: joining abort ui thread failed!");
			failed = 1;
			break;
		} else {
			LOGD_PRINT("Joined abort ui thread!, finished iteration %d",i);
		}
	}

	if (g_efd != -1) close(g_efd);

	ret = qsc_shutdown_app(&l_QSEEComHandle);
	if (ret) {
		LOGE_PRINT("   Failed to shutdown app: %d",ret);
		failed = 1;
	}
	LOGD_PRINT("shutdown: pass\n");

	if  (pthread_join(abort_ui_thread, NULL) == FAILURE){
		LOGE_PRINT("Error: joining abort ui thread failed!");
	} else {
		LOGD_PRINT("Joined abort ui thread!");
	}
	if(!failed){
		LOGD_PRINT("Succeeded %d touch test iterations",TEST_ITERATION);
	} else {
		LOGE_PRINT("failed touch test after %d iterations",i);
	}
	return 0;

}
