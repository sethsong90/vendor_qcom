/* csd_client.c
 *
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
//#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0

#ifdef _ANDROID_
#include <utils/Log.h>
#include <cutils/properties.h>
#include "common_log.h"
#else
#define LOGI(...)      fprintf(stdout,__VA_ARGS__)
#define LOGE(...)      fprintf(stderr,__VA_ARGS__)
#define LOGV(...)      fprintf(stderr,__VA_ARGS__)
#define LOGD(...)      fprintf(stderr,__VA_ARGS__)
#endif /* _ANDROID_ */

#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "qmi_client.h"
#include "acdb-loader.h"
#include "qmi_client_instance_defs.h"
#include "acdb-id-mapper.h"

#include "core_sound_driver_v01.h"
#include "csd_client.h"

#define MAX_SERVICE_INFO 1

#define TIMEOUT_MS 3000
#define SAMPLE_RATE_8KHZ 8000
#define SAMPLE_RATE_16KHZ 16000
#define SAMPLE_RATE_48KHZ 48000
#define BITS_PER_SAMPLE 16

#define MODULE_ID_VOICE_MODULE_FENS 0x00010EEB
#define MODULE_ID_VOICE_MODULE_ST 0x00010EE3
#define VOICE_PARAM_MOD_ENABLE 0x00010E00

#define MAX_SESSION_NAME_LEN 50
#define CSD_VOICE_SESSION_NAME  "default modem voice"
#define CSD_VOLTE_SESSION_NAME  "default volte voice"
#define CSD_VOICE2_SESSION_NAME "default modem voice2"

/* CSD server version ID - Below CSD version supports the
 * following voice call setup latency optimization features
 *
 * 1. Enable multiple devices in one device enable command
 * 2. Call vocproc attach and detach at vocproc open time
 * 3. Parallelize MDM call sequence with APQ codec setup
 * 4. Call QMI async messages and return without wait for response
 */
//#define CSD_VER_ID_WITH_OPTIMIZATION 0x01090001
#define CSD_VER_ID_WITH_OPTIMIZATION 0x11111111

/* from AudioHardwareALSA.h */
#define ANC_FLAG 0x00000001
#define TTY_OFF 0x00000010
#define TTY_FULL 0x00000020
#define TTY_VCO 0x00000040
#define TTY_HCO 0x00000080

#define AANC_ENABLE 1
#define AANC_DISABLE 0

/* External EC reference devices are only used when APQ is paired with external
 * modem. These device IDs are specific to the external modem, since CSD-QMI is
 * the only module that uses them the definitions are moved here.
 */
#define DEVICE_EXTERNAL_EC_REFERENCE_MONO_ACDB_ID 43
#define DEVICE_EXTERNAL_EC_REFERENCE_STEREO_ACDB_ID 44
#define DEVICE_EXTERNAL_EC_REFERENCE2_MONO_ACDB_ID 48
#define DEVICE_EXTERNAL_EC_REFERENCE2_STEREO_ACDB_ID 49

/* MDM Device ACDB IDs */
#define DEVICE_INCALL_VOICE_RECORD_STEREO_ACDB_ID       45
#define DEVICE_INCALL_MUSIC_DELIVERY_MONO_ACDB_ID       46
#define DEVICE_INCALL_VOICE_RECORD_MONO_ACDB_ID         47

#define DEVICE_TTY_HEADSET_MONO_RX_ACDB_ID 17// TTY_HEADSET_SPKR
#define DEVICE_TTY_HEADSET_MONO_TX_ACDB_ID 16// TTY_HEADSET_MIC

#define MAX_ATTEMPTS 60
#define QMI_RIL_SPEECH_CODEC_INFO_MESSAGE 0x53

#define MAX_VOL_INDEX 5
#define MIN_VOL_INDEX 0

/* .5 for rounding before casting to non-decmal value.
 * Should not be used if you need decmal values or are expecting negitive
 * indexes.
 */
#define percent_to_index(val, min, max) \
        ((val) * ((max) - (min)) * 0.01 + (min) + .5)

enum {
    AUDIO_SLIMSLAVE_UNVOTE,
    AUDIO_SLIMSLAVE_VOTE
};

enum {
    STATUS_WAIT,
    STATUS_SUCCESS,
    STATUS_FAIL
};

enum thread_command {
    CSD_NONE,
    CSD_DEINIT,
    CSD_REINIT,
};

enum session_state {
    SESSION_INVALID,
    SESSION_CREATED,
    SESSION_STOPPED,
    SESSION_STARTED,
    SESSION_STANDBY,
};

enum device_status {
    DEVICE_INVALID,
    DEVICE_DISABLED,
    DEVICE_ENABLED,
};

struct csd_resp_msg_v01 {
    qmi_response_type_v01 resp;
    uint8_t handle_valid;
    uint32_t handle;
    uint8_t cmd_token_valid;
    uint32_t cmd_token;
    uint8_t qmi_csd_status_code_valid;
    qmi_csd_status_v01 qmi_csd_status_code;
};

enum {
    INCALL_REC_NONE = -1,
    INCALL_REC_UPLINK,
    INCALL_REC_DOWNLINK,
    INCALL_REC_UPLINK_AND_DOWNLINK,
};

enum {
    NETWORK_NONE,
    GSM,
    WCDMA,
    CDMA,
    LTE,
    TD_SCDMA,
};

enum {
    CODEC_NONE,
    QCELP_13K,
    EVRC,
    EVRC_B,
    EVRC_WB,
    EVRC_NW,
    AMR_NB,
    AMR_WB,
    GSM_EFR,
    GSM_FR,
    GSM_HR,
};

struct incall_rec_info {
    uint32_t rec_enable;
    uint32_t rec_mode;
    uint32_t recording;
};

struct incall_play_info {
    uint32_t play_enable;
    uint32_t playing;
};

enum session_id {
    INVALID_SESSION_ID = -1,
    VOICE_SESSION_ID,
    VOLTE_SESSION_ID,
    BASE_INDEX_CSD_INSTANCE2,
    VOICE2_SESSION_ID = BASE_INDEX_CSD_INSTANCE2,
    ALL_SESSION_ID
};

enum csd_instances {
    CSD_SERVICE_INSTANCE1,
    CSD_SERVICE_INSTANCE2,
    MAX_CSD_INSTANCES
};

/* CSD session structure holds informtion per session data.
 * Each session will have Stream, Context and Manager handle.
 *
 * All the properties suchas Wide Voice, FENS, ST, volume, mute
 * are session based. In call music delivery and In call recording
 * is also session based.
 */
struct csd_session {
    char name[MAX_SESSION_NAME_LEN];
    uint32_t state;
    uint32_t async_status;
    uint32_t stream_handle;
    uint32_t context_handle;
    uint32_t manager_handle;
    uint8_t wide_voice;
    uint8_t fens;
    uint8_t slow_talk;
    int volume;
    int mute;
    struct incall_rec_info rec_info;
    struct incall_play_info play_info;
    pthread_mutex_t csd_async_cond_mutex;
    pthread_cond_t csd_async_wait_condition;

};

/* CSD service structure holds information per each CSD service
 * connection with MDM. Each CSD service will spawn a new thread
 * to connect to the CSD QMI service.
 *
 * Device handle exposed by CSD service is singleton per service.
 * Where a CSD service can have multiple sessions such as
 * Voice, Voice2 and VoLTE are supported.
 */

struct csd_service {
    int thread_command;
    qmi_client_type notifier;
    qmi_client_type csd_handle;
    qmi_cci_os_signal_type os_params;
    qmi_service_instance qmi_instance_id;
    uint32_t device_handle;
    int device_state;
    pthread_cond_t csd_thread_condition;
    pthread_mutex_t csd_thread_mutex;
    pthread_mutex_t csd_service_mutex;
    pthread_t csd_thread;
    struct csd_session sessions[ALL_SESSION_ID];
    uint32_t csd_version;
};

/* CSD Client is global data which will contain device ids, tty mode,
 * aanc mode as these are specific to device information on APQ.
 * slim_fd is a file descriptor used to vote/unvote slimbus
 * before enable/disable devices cmd send to MDM
 *
 * This structure will also contain one or more csd service
 * handle per MDM which exposes CSD service.
 */
struct csd_client {
    int rx_dev_id;
    int tx_dev_id;
    int ec_dev_id;
    uint32_t tty_mode;
    uint32_t aanc_mode;
    struct csd_service csd_services[MAX_CSD_INSTANCES];
    int slim_fd;
    pthread_mutex_t csd_client_mutex;
};

struct csd_iterator {
    int cur_index;
    int session_id;
};

struct qmi_ril_speech_codec_info_msg {
  /* Unique message identifier */
  uint8_t  msg_id;
  /* Length of the message payload */
  uint16_t msg_payload_len;
  uint8_t  network_mode;
  uint8_t  speech_codec;
};

/* CSD response structure contains service, session structure pointers
 * and a void pointer to hold response message buffer address
 *
 * This is needed to get hold of the addresses of dynamically allocated
 * structures as some of async messages are returned immediately without
 * waiting for response and these are freeed in call back function.
 */
struct csd_response_msg {
    struct csd_service *service;
    struct csd_session *session;
    void *resp;
};

struct csd_client csd_client;

bool is_i2s_ext_modem;
static int socket_connection_state;
static int sockid;
static int i2s_sample_rate = SAMPLE_RATE_8KHZ;


static void* csd_thread_main(void *data);

static bool fusion_dual_mdm()
{
    return false;
}

static bool fusion_single_mdm()
{
#ifdef _ANDROID_
    char platform[128], baseband[128];

    property_get("ro.board.platform", platform, "");
    property_get("ro.baseband", baseband, "");

    if (!((!(strcmp("msm8960", platform) && strcmp("apq8084", platform)))
          && !strcmp("mdm", baseband)))
        return false;
    else
#endif
        return true;
}

static bool fusion_sglte()
{
#ifdef _ANDROID_
    char platform[128], baseband[128];

    property_get("ro.board.platform", platform, "");
    property_get("ro.baseband", baseband, "");

    if (!strcmp("msm8960", platform) && !strcmp("sglte2", baseband))
        return true;
    else
#endif
        return false;
}

static int platform_is_i2s_ext_modem(void)
{
    char platform_ver[128];
    int ver_num = 0;
    FILE *fp;
    bool is_i2s_ext_modem = false;

    fp = fopen("/sys/devices/soc0/platform_version","r");
    if(fp == NULL) {
        LOGE("Cannot open /sys/devices/soc0/platform_version file");
    } else {
        if (fgets(platform_ver, sizeof(platform_ver), fp) != NULL) {
            ver_num = atoi(platform_ver);
            LOGV("platform_ver %s, ver_num %x\n", platform_ver, ver_num);

            if (ver_num == 0x10001) {
                is_i2s_ext_modem = true;
            }
        }
        fclose(fp);
    }
    return is_i2s_ext_modem;
}

static int get_sessionid_by_vsid(uint32_t vsid)
{
    int sessionid = INVALID_SESSION_ID;
    switch(vsid)
    {
    case VOICE_SESSION_VSID:
        sessionid = VOICE_SESSION_ID;
        break;

    case VOICE2_SESSION_VSID:
        sessionid = VOICE2_SESSION_ID;
        break;

    case VOLTE_SESSION_VSID:
        sessionid = VOLTE_SESSION_ID;
        break;

    case ALL_SESSION_VSID:
        sessionid = ALL_SESSION_ID;
        break;

    default:
        sessionid = INVALID_SESSION_ID;
        break;
    }
    return sessionid;

}

struct csd_service *get_service_by_sessionid(int sessionid)
{
    struct csd_service *service = NULL;

    switch (sessionid) {
    case INVALID_SESSION_ID:
    case ALL_SESSION_ID:
        service = NULL;
        break;

    case VOICE2_SESSION_ID:
        if (fusion_dual_mdm()) {
            /* When there are two MDMs, VOICE2_SESSION_ID corresponds to
             * voice session in second instance of the CSD service.
             */
            service = &csd_client.csd_services[CSD_SERVICE_INSTANCE2];
        }
        else {
            service = &csd_client.csd_services[CSD_SERVICE_INSTANCE1];
        }

    case VOICE_SESSION_ID:
    case VOLTE_SESSION_ID:
    default:
        service = &csd_client.csd_services[CSD_SERVICE_INSTANCE1];
    }
    return service;

}

static struct csd_session *get_session_by_sessionid(int sessionid)
{
    struct csd_session *session = NULL;
    switch (sessionid) {
    case VOICE_SESSION_ID:
    case VOLTE_SESSION_ID:
        session =
        &csd_client.csd_services[CSD_SERVICE_INSTANCE1].sessions[sessionid];
        break;

    case VOICE2_SESSION_ID:
        if (fusion_dual_mdm()) {
            /* When there are two MDMs, VOICE2_SESSION_ID corresponds to
             * voice session in second instance of the CSD service.
             */
            session = &csd_client.csd_services[CSD_SERVICE_INSTANCE2].sessions[sessionid - BASE_INDEX_CSD_INSTANCE2];
        }
        else {
            session =
            &csd_client.csd_services[CSD_SERVICE_INSTANCE1].sessions[sessionid];
        }
        break;
    case INVALID_SESSION_ID:
    case ALL_SESSION_ID:
    default:
        session = NULL;
    }

    return session;

}

static void iterator_init(struct csd_iterator *it, int session_id)
{
    if (session_id == ALL_SESSION_ID) {
        it->cur_index = 0;
        it->session_id = ALL_SESSION_ID - 1;
    } else{
        it->cur_index = session_id;
        it->session_id = session_id;
    }
}

static bool iterator_has_next(struct csd_iterator *it)
{
    if (it->cur_index <= it->session_id) {
        return true;
    } else {
        return false;
    }
}

static void iterator_next(struct csd_iterator *it, struct csd_service **service,
                          struct csd_session **session)
{

    if (it->cur_index <= it->session_id) {
        *service = get_service_by_sessionid(it->cur_index);
        *session = get_session_by_sessionid(it->cur_index);
        it->cur_index++;
    } else {
        *service = NULL;
        *session = NULL;
    }

}

void get_dev_ids(int *rx_dev_id, int *tx_dev_id, int *ec_dev_id){
    pthread_mutex_lock(&csd_client.csd_client_mutex);
    *rx_dev_id = csd_client.rx_dev_id;
    *tx_dev_id = csd_client.tx_dev_id;
    *ec_dev_id = csd_client.ec_dev_id;
    pthread_mutex_unlock(&csd_client.csd_client_mutex);
}

void get_ec_dev_id(int *ec_dev_id){
    pthread_mutex_lock(&csd_client.csd_client_mutex);
    *ec_dev_id = csd_client.ec_dev_id;
    pthread_mutex_unlock(&csd_client.csd_client_mutex);
}

void set_dev_ids(int rx_dev_id, int tx_dev_id, int ec_dev_id){
    pthread_mutex_lock(&csd_client.csd_client_mutex);
    csd_client.rx_dev_id = rx_dev_id;
    csd_client.tx_dev_id = tx_dev_id;
    csd_client.ec_dev_id = ec_dev_id;
    pthread_mutex_unlock(&csd_client.csd_client_mutex);
}

void set_tty_mode(uint32_t ttymode)
{
    pthread_mutex_lock(&csd_client.csd_client_mutex);
    csd_client.tty_mode = ttymode;
    pthread_mutex_unlock(&csd_client.csd_client_mutex);
}

void get_tty_mode(uint32_t *ttymode)
{
    pthread_mutex_lock(&csd_client.csd_client_mutex);
    *ttymode = csd_client.tty_mode;
    pthread_mutex_unlock(&csd_client.csd_client_mutex);
}

void set_aanc_mode(uint32_t aanc_mode)
{
    pthread_mutex_lock(&csd_client.csd_client_mutex);
    csd_client.aanc_mode = aanc_mode;
    pthread_mutex_unlock(&csd_client.csd_client_mutex);
}

void get_aanc_mode(uint32_t *aanc_mode)
{
    pthread_mutex_lock(&csd_client.csd_client_mutex);
    *aanc_mode = csd_client.aanc_mode;
    pthread_mutex_unlock(&csd_client.csd_client_mutex);
}

int set_csd_response_msg(struct csd_response_msg **csd_resp_msg,
                         struct csd_service *service,
                         struct csd_session *session,
                         void *resp)
{
    int rc = 0;
    struct csd_response_msg *csd_resp;

    csd_resp = (struct csd_response_msg *)
                malloc(sizeof(struct csd_response_msg));

    if (csd_resp != NULL) {
        csd_resp->service = service;
        csd_resp->session = session;
        csd_resp->resp = resp;
        *csd_resp_msg = csd_resp;
    } else {
        LOGE("%s: Failed to allocate memory for CSD response mesg \n",
              __func__);

        rc = -1;
    }

    return rc;
}

static int get_external_ec_device_id(int rx_dev_id)
{
    int ec_dev_id = DEVICE_EXTERNAL_EC_REFERENCE_MONO_ACDB_ID;

    switch (rx_dev_id) {
    case DEVICE_HANDSET_RX_ACDB_ID:
    case DEVICE_SPEAKER_MONO_RX_ACDB_ID:
    case DEVICE_HEADSET_MONO_RX_ACDB_ID:
        ec_dev_id = DEVICE_EXTERNAL_EC_REFERENCE_MONO_ACDB_ID;
        break;

    case DEVICE_SPEAKER_RX_ACDB_ID:
    case DEVICE_HEADSET_RX_ACDB_ID:
    /* case DEVICE_SPEAKER_HEADSET_RX_ACDB_ID: */
    case DEVICE_ANC_HEADSET_STEREO_RX_ACDB_ID:
        ec_dev_id = DEVICE_EXTERNAL_EC_REFERENCE_STEREO_ACDB_ID;
        break;

    case DEVICE_BT_SCO_RX_ACDB_ID:
    case DEVICE_BT_SCO_RX_WB_ACDB_ID:
        ec_dev_id = DEVICE_EXTERNAL_EC_REFERENCE2_MONO_ACDB_ID;
        break;

    case DEVICE_USB_RX_ACDB_ID:
        ec_dev_id = DEVICE_EXTERNAL_EC_REFERENCE2_STEREO_ACDB_ID;
        break;

    /* case DEVICE_PROXY_RX_ACDB_ID: */
    default:
        ec_dev_id = DEVICE_EXTERNAL_EC_REFERENCE_MONO_ACDB_ID;
    }

    return ec_dev_id;
}

static int initialize_session_data(struct csd_session *session)
{
    session->state = SESSION_INVALID;
    session->volume = 3;
    pthread_cond_init(&session->csd_async_wait_condition, NULL);
    pthread_mutex_init(&session->csd_async_cond_mutex, NULL);

    return 0;
}

static int initialize_service_data(struct csd_service *service)
{
    int i = 0;
    pthread_mutex_init(&service->csd_service_mutex, NULL);

    pthread_mutex_lock(&service->csd_service_mutex);
    memset(&service->notifier, 0, sizeof(service->notifier));
    memset(&service->csd_handle, 0, sizeof(service->csd_handle));
    memset(&service->os_params, 0, sizeof(service->os_params));
    service->device_handle = 0;
    service->device_state = DEVICE_INVALID;
    service->csd_version = 0;
    pthread_cond_init(&service->csd_thread_condition, NULL);
    pthread_mutex_init(&service->csd_thread_mutex, NULL);
    memset(&service->csd_thread, 0, sizeof(service->csd_thread));

    for (i = 0; i < ALL_SESSION_ID; i++) {
        initialize_session_data(&service->sessions[i]);
    }

    strlcpy(service->sessions[VOICE_SESSION_ID].name,
           CSD_VOICE_SESSION_NAME, strlen(CSD_VOICE_SESSION_NAME)+1);
    strlcpy(service->sessions[VOLTE_SESSION_ID].name,
           CSD_VOLTE_SESSION_NAME, strlen(CSD_VOLTE_SESSION_NAME)+1);
    strlcpy(service->sessions[VOICE2_SESSION_ID].name,
           CSD_VOICE2_SESSION_NAME, strlen(CSD_VOICE2_SESSION_NAME)+1);

    /* For Fusion DSDA with 2 MDMS use QMI_CLIENT_QMUX_RMNET_USB_INSTANCE2_0
     * for second instance.
     */

    /* Setup the instance id for each service*/
    service->qmi_instance_id = QMI_CLIENT_QMUX_RMNET_USB_INSTANCE_0;

    pthread_mutex_unlock(&service->csd_service_mutex);

    return 0;
}

static int initialize_client_data()
{
    int i = 0;
    struct csd_service *service = NULL;

    pthread_mutex_init(&csd_client.csd_client_mutex, NULL);

    pthread_mutex_lock(&csd_client.csd_client_mutex);

    csd_client.tty_mode = TTY_OFF;

    /* Set up default devices */
    csd_client.rx_dev_id = 0x00000007;
    csd_client.ec_dev_id = get_external_ec_device_id(csd_client.rx_dev_id);
    csd_client.tx_dev_id = 0x00000004;

    if (is_i2s_ext_modem) {
       LOGV("%s, this is I2S based external modem\n", __func__);
       csd_client.rx_dev_id = 1;
       csd_client.tx_dev_id = 0;
    }

    pthread_mutex_unlock(&csd_client.csd_client_mutex);

    initialize_service_data(&csd_client.csd_services[CSD_SERVICE_INSTANCE1]);

    return 0;
}

static int csd_join_csd_thread(int csd_service_index){

    int rc = 0;
    struct csd_service *csd_service = &csd_client.csd_services[csd_service_index];

    pthread_mutex_lock(&csd_service->csd_service_mutex);
    csd_service->thread_command = CSD_DEINIT;
    pthread_mutex_unlock(&csd_service->csd_service_mutex);

    pthread_mutex_lock(&csd_service->csd_thread_mutex);
    pthread_cond_signal(&csd_service->csd_thread_condition);
    pthread_mutex_unlock(&csd_service->csd_thread_mutex);

    rc = pthread_join(csd_client.csd_services[csd_service_index].csd_thread,
                       NULL);
    if (rc)
        LOGE("%s: csd_thread[%d] pthread_join err %d\n",
             __func__, csd_service_index, rc);

    return rc;
}

static int csd_create_thread(int csd_service_index) {
    struct csd_service *csd_service = NULL;
    int rc = 0;

    csd_service = &csd_client.csd_services[csd_service_index];
    pthread_mutex_lock(&csd_service->csd_service_mutex);
    rc = pthread_create(&csd_service->csd_thread, (const pthread_attr_t *) NULL,
                        csd_thread_main, csd_service);

    if (rc)
        LOGE("%s: csdthread[%d] create failed %d\n",
             __func__, rc, csd_service_index);
    else
        LOGD("%s: csdthread[%d] create successfull %d\n",
             __func__, rc, csd_service_index);

    pthread_mutex_unlock(&csd_service->csd_service_mutex);

    return rc;
}

static int csd_async_wait(struct csd_session *csd_session,
                          struct csd_service *service,
                          bool wait_for_response)
{
    int rc = 0;
    struct timespec ts;

    if((service->csd_version < CSD_VER_ID_WITH_OPTIMIZATION) ||
       ((service->csd_version >= CSD_VER_ID_WITH_OPTIMIZATION) &&
        (wait_for_response == true))) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += TIMEOUT_MS/1000;

        pthread_mutex_lock(&csd_session->csd_async_cond_mutex);
        csd_session->async_status = STATUS_WAIT;

        while ((csd_session->async_status == STATUS_WAIT) && (rc == 0)) {
            rc = pthread_cond_timedwait(&csd_session->csd_async_wait_condition,
                                        &csd_session->csd_async_cond_mutex, &ts);
        }

        if ((csd_session->async_status == STATUS_FAIL) ||
            (csd_session->async_status == STATUS_WAIT)) {
            LOGE("%s: CSD async_status =%d\n",
                 __func__, csd_session->async_status);

            rc = -1;
        }

        pthread_mutex_unlock(&csd_session->csd_async_cond_mutex);
    }

    return rc;
}

static uint8_t csd_success(qmi_response_type_v01 resp,
                           uint8_t valid,
                           qmi_csd_status_v01 status)
{
    uint8_t rc = 0;

    if (resp.result == QMI_RESULT_SUCCESS && resp.error == QMI_ERR_NONE) {
        if (valid) {
            if (status == QMI_CSD_EOK_V01) {
                rc = 1;
            }
        } else {
            rc = 1;
        }
    }

    return rc;
}

static void csd_client_ind_cb(qmi_client_type user_handle,
                              unsigned int msg_id,
                              void *ind_buf,
                              unsigned int ind_buf_len,
                              void *ind_cb_data)
{
    LOGD("%s: msg_id %d\n", __func__, msg_id);
}

static void csd_client_error_cb(qmi_client_type user_handle,
                                qmi_client_error_type error,
                                void *err_cb_data)
{
    struct csd_service *service = err_cb_data;
    int i = 0;

    LOGD("%s: error %d cb_data 0x%x\n",
         __func__, error, (uint32_t) err_cb_data);

    if (service == NULL) {
        LOGE("%s: service is NULL\n", __func__);
        return;
    }

    pthread_mutex_lock(&service->csd_service_mutex);
    /* Set session state to invalid so that csd_client_xxx() session apis fail,
     * incase the thread is not scheduled yet to run CSD_REINT command.
     */
    for (i = 0; i< ALL_SESSION_ID; i++) {
        (&service->sessions[i])->state = SESSION_INVALID;
    }
    /* Set device state to invalid so that csd_client_xxx_() device apis fail,
     * incase the thread is not scheduled yet to run CSD_REINT command.
     */
    service->device_state = DEVICE_INVALID;

    if (error == QMI_SERVICE_ERR) {
        LOGD("%s: MDM reset\n", __func__);

        service->thread_command = CSD_REINIT;
    }
    pthread_mutex_unlock(&service->csd_service_mutex);

    pthread_mutex_lock(&service->csd_thread_mutex);
    pthread_cond_signal(&service->csd_thread_condition);
    pthread_mutex_unlock(&service->csd_thread_mutex);
}

static void csd_client_async_cb(qmi_client_type handle,
                                unsigned int msg_id,
                                void *resp,
                                unsigned int len,
                                void *cb_data,
                                qmi_client_error_type err)
{
    struct csd_resp_msg_v01 *resp_msg = resp;
    struct csd_response_msg *csd_resp;
    struct csd_session *session;

    if (cb_data == NULL) {
        LOGE("%s: Error, csd_resp is null\n", __func__);

        return;
    }

    if (resp_msg != NULL ) {
        LOGD("%s: msg_id 0x%x result %d error %d\n", __func__,
             msg_id, resp_msg->resp.result, resp_msg->resp.error);
    }

    switch (msg_id) {
    case QMI_CSD_IOCTL_VS_CMD_SET_MUTE_RESP_V01:
    case QMI_CSD_IOCTL_VS_CMD_SET_UI_PROPERTY_RESP_V01:
    case QMI_CSD_IOCTL_VC_CMD_SET_DEVICE_CONFIG_RESP_V01:
    case QMI_CSD_IOCTL_VC_CMD_ENABLE_RESP_V01:
    case QMI_CSD_IOCTL_VC_CMD_DISABLE_RESP_V01:
    case QMI_CSD_IOCTL_VC_CMD_SET_RX_VOLUME_INDEX_RESP_V01:
    case QMI_CSD_IOCTL_VM_CMD_SET_TTY_MODE_RESP_V01:
    case QMI_CSD_IOCTL_VM_CMD_SET_WIDEVOICE_RESP_V01: {
        csd_resp = cb_data;

        if (resp_msg == NULL ) {
            LOGE("%s: Error, resp_msg is null\n", __func__);
        } else {
            if ((csd_resp->service->csd_version >= CSD_VER_ID_WITH_OPTIMIZATION)) {
                if (!csd_success(resp_msg->resp,
                                 resp_msg->qmi_csd_status_code_valid,
                                 resp_msg->qmi_csd_status_code)) {

                    LOGE("%s: msg_id 0x%x CSD status fail\n",
                         __func__, msg_id);
                }
            } else {
                session = csd_resp->session;

                pthread_mutex_lock(&session->csd_async_cond_mutex);

                if (csd_success(resp_msg->resp,
                                resp_msg->qmi_csd_status_code_valid,
                                resp_msg->qmi_csd_status_code)) {
                    session->async_status = STATUS_SUCCESS;
                } else {
                    session->async_status = STATUS_FAIL;
                }

                pthread_cond_signal(&session->csd_async_wait_condition);
                pthread_mutex_unlock(&session->csd_async_cond_mutex);
            }
        }

        free(csd_resp->resp);
        free(csd_resp);

        break;
    }

    case QMI_CSD_IOCTL_VC_CMD_SET_MUTE_RESP_V01:
    case QMI_CSD_IOCTL_VM_CMD_ATTACH_STREAM_RESP_V01:
    case QMI_CSD_IOCTL_VM_CMD_DETACH_STREAM_RESP_V01:
    case QMI_CSD_IOCTL_VM_CMD_ATTACH_CONTEXT_RESP_V01:
    case QMI_CSD_IOCTL_VM_CMD_DETACH_CONTEXT_RESP_V01:
    case QMI_CSD_IOCTL_VM_CMD_START_VOICE_RESP_V01:
    case QMI_CSD_IOCTL_VM_CMD_STANDBY_VOICE_RESP_V01:
    case QMI_CSD_IOCTL_VM_CMD_STOP_VOICE_RESP_V01:
    case QMI_CSD_IOCTL_VS_CMD_START_RECORD_RESP_V01:
    case QMI_CSD_IOCTL_VS_CMD_STOP_RECORD_RESP_V01:
    case QMI_CSD_IOCTL_VS_CMD_START_PLAYBACK_RESP_V01:
    case QMI_CSD_IOCTL_VS_CMD_STOP_PLAYBACK_RESP_V01: {
        session = cb_data;

        if (resp_msg == NULL ) {
            LOGE("%s: Error, resp_msg is null\n", __func__);
        } else {
            pthread_mutex_lock(&session->csd_async_cond_mutex);

            if (csd_success(resp_msg->resp, resp_msg->qmi_csd_status_code_valid,
                            resp_msg->qmi_csd_status_code)) {
                session->async_status = STATUS_SUCCESS;
            } else {
                session->async_status = STATUS_FAIL;
            }

            pthread_cond_signal(&session->csd_async_wait_condition);
            pthread_mutex_unlock(&session->csd_async_cond_mutex);
        }

        break;
    }

    default: {
        session = cb_data;

        if (resp_msg == NULL ) {
            LOGE("%s: Error, resp_msg is null\n", __func__);
        } else {
            LOGE("%s: Unknown msg %d\n", __func__, msg_id);

            pthread_mutex_lock(&session->csd_async_cond_mutex);
            session->async_status = STATUS_FAIL;
            pthread_cond_signal(&session->csd_async_wait_condition);
            pthread_mutex_unlock(&session->csd_async_cond_mutex);
        }
    }
    }
}

static uint32_t get_sample_rate(int dev_id)
{
    uint32_t rate = SAMPLE_RATE_48KHZ;

    if (is_i2s_ext_modem) {
        LOGD("sample rate %d\n", i2s_sample_rate);

        return i2s_sample_rate;
    }

    switch (dev_id) {
    case DEVICE_BT_SCO_RX_ACDB_ID:
    case DEVICE_BT_SCO_TX_ACDB_ID:
        rate = SAMPLE_RATE_8KHZ;
        break;

    case DEVICE_BT_SCO_RX_WB_ACDB_ID:
    case DEVICE_BT_SCO_TX_WB_ACDB_ID:
        rate = SAMPLE_RATE_16KHZ;
        break;

    case DEVICE_USB_RX_ACDB_ID:
    case DEVICE_USB_TX_ACDB_ID:
        rate = SAMPLE_RATE_48KHZ;
        break;

    default:
        rate = SAMPLE_RATE_48KHZ;
    }

    LOGD("%s: Rate %d\n",__func__, rate);

    return rate;
}

static int get_csd_tty_mode(uint32_t flag)
{
    int mode = QMI_CSD_TTY_DISABLE_V01;

    switch(flag) {
    case TTY_FULL:
        mode = QMI_CSD_TTY_FULL_V01;
        break;

    case TTY_VCO:
        mode = QMI_CSD_TTY_VCO_V01;
        break;

    case TTY_HCO:
        mode = QMI_CSD_TTY_HCO_V01;
        break;

    default:
        mode = QMI_CSD_TTY_DISABLE_V01;
    }

    LOGD("%s: Mode %d\n", __func__, mode);

    return mode;
}

static uint8_t set_external_ec(int rx_dev_id)
{
    uint8_t set = 0;

    /* External EC is not supported on I2S based modem */
    if (is_i2s_ext_modem)
        return 0;

    switch (rx_dev_id) {
    case DEVICE_HANDSET_RX_ACDB_ID:
    case DEVICE_SPEAKER_MONO_RX_ACDB_ID:
    case DEVICE_SPEAKER_RX_ACDB_ID:
    case DEVICE_HEADSET_MONO_RX_ACDB_ID:
    case DEVICE_HEADSET_RX_ACDB_ID:
    /* case DEVICE_SPEAKER_HEADSET_RX_ACDB_ID: */
    case DEVICE_ANC_HEADSET_STEREO_RX_ACDB_ID:
    case DEVICE_BT_SCO_RX_ACDB_ID:
    case DEVICE_BT_SCO_RX_WB_ACDB_ID:
    case DEVICE_USB_RX_ACDB_ID:
    /* case DEVICE_PROXY_RX_ACDB_ID: */
        set = 1;
        break;

    default:
        set = 0;
    }

    return set;
}

/* Get CSD version */
static int get_csd_version(struct csd_service *service)
{
    int rc = 0;

    qmi_csd_query_driver_version_resp_msg_v01 version_resp;
    memset(&version_resp, 0, sizeof(version_resp));

    rc = qmi_client_send_msg_sync(service->csd_handle,
                                  QMI_CSD_QUERY_DRIVER_VERSION_RESP_V01,
                                  NULL, 0,
                                  &version_resp, sizeof(version_resp),
                                  TIMEOUT_MS);

    if (rc == QMI_NO_ERR) {
        LOGD("%s: QMI - CSD version: %x\n",
             __func__, version_resp.csd_version);

        service->csd_version = version_resp.csd_version;
        return 0;
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
             __func__,
             version_resp.resp.result, version_resp.resp.error,
             version_resp.csd_version_valid,
             version_resp.csd_version, rc);

        return -1;
    }
}

/* CSD init */
static int init(struct csd_service *service)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_init_resp_msg_v01 init_resp;
    memset(&init_resp, 0, sizeof(init_resp));

    rc = qmi_client_send_msg_sync(service->csd_handle, QMI_CSD_INIT_REQ_V01,
                                  NULL, 0, &init_resp, sizeof(init_resp),
                                  TIMEOUT_MS);
    if (rc == QMI_NO_ERR &&
        csd_success(init_resp.resp, init_resp.qmi_csd_status_code_valid,
                    init_resp.qmi_csd_status_code)) {

        return 0;
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
             __func__,
             init_resp.resp.result, init_resp.resp.error,
             init_resp.qmi_csd_status_code_valid,
             init_resp.qmi_csd_status_code, rc);

        return -1;
    }
}

static int deinit(struct csd_service *service)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_deinit_resp_msg_v01 deinit_resp;
    memset(&deinit_resp, 0, sizeof(deinit_resp));

    rc = qmi_client_send_msg_sync(service->csd_handle,
                                  QMI_CSD_DEINIT_REQ_V01, NULL, 0,
                                  &deinit_resp, sizeof(deinit_resp),
                                  TIMEOUT_MS);
    if (rc == QMI_NO_ERR &&
        csd_success(deinit_resp.resp, deinit_resp.qmi_csd_status_code_valid,
                    deinit_resp.qmi_csd_status_code)) {

        return 0;
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
             __func__,
             deinit_resp.resp.result, deinit_resp.resp.error,
             deinit_resp.qmi_csd_status_code_valid,
             deinit_resp.qmi_csd_status_code, rc);

        return -1;
    }
}

/* Device control */
static int open_device_control(struct csd_service *service)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_open_device_control_resp_msg_v01 open_resp;
    memset(&open_resp, 0, sizeof(open_resp));

    rc = qmi_client_send_msg_sync(service->csd_handle,
                                  QMI_CSD_OPEN_DEVICE_CONTROL_REQ_V01, NULL,
                                  0, &open_resp, sizeof(open_resp),
                                  TIMEOUT_MS);
    if (rc == QMI_NO_ERR &&
        csd_success(open_resp.resp, open_resp.open_status_valid,
                    open_resp.open_status)) {
        if (open_resp.qmi_csd_device_handle_valid) {
            service->device_handle = open_resp.qmi_csd_device_handle;

            return 0;
        } else {
            LOGE("%s: Not valid, %d handle %d\n",
                 __func__,
                 open_resp.qmi_csd_device_handle_valid,
                 open_resp.qmi_csd_device_handle);

            return -1;
        }
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
             __func__,
             open_resp.resp.result, open_resp.resp.error,
             open_resp.open_status_valid, open_resp.open_status, rc);

        return -1;
    }
}

static int close_device_control(struct csd_service *service)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_close_req_msg_v01 close_req;
    qmi_csd_close_resp_msg_v01 close_resp;
    memset(&close_resp, 0, sizeof(close_resp));

    close_req.handle = service->device_handle;

    rc = qmi_client_send_msg_sync(service->csd_handle, QMI_CSD_CLOSE_REQ_V01,
                                  &close_req, sizeof(close_req),
                                  &close_resp, sizeof(close_resp),
                                  TIMEOUT_MS);
    if (rc == QMI_NO_ERR &&
        csd_success(close_resp.resp, close_resp.qmi_csd_status_code_valid,
                    close_resp.qmi_csd_status_code)) {
        service->device_handle = 0;

        return 0;
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
             __func__,
             close_resp.resp.result, close_resp.resp.error,
             close_resp.qmi_csd_status_code_valid,
             close_resp.qmi_csd_status_code, rc);

        return -1;
    }
}

/* This function is to vote/unvote ioctl for slimbus master to be woken up
 * before enable/disable devices cmd send to MDM and allow to sleep after
 * enable/disable devices cmd.
 */
static int vote_unvote_for_slimbus(int vote)
{
    int rc = 0;

    switch (vote) {
        case AUDIO_SLIMSLAVE_VOTE:
            if((ioctl(csd_client.slim_fd, AUDIO_SLIMSLAVE_VOTE))) {
                LOGE("%s: ioctl failed", __func__ );

                rc = -errno;
            }
            break;
        case AUDIO_SLIMSLAVE_UNVOTE:
            if((ioctl(csd_client.slim_fd, AUDIO_SLIMSLAVE_UNVOTE))) {
                LOGE("%s: ioctl failed", __func__);

                rc = -errno;
            }
            break;
        default:
            LOGE("%s: ioctl not valid", __func__);

            rc = -EINVAL;
            break;
    }

    return rc;
}

static int enable_device(struct csd_service *service, int rx_dev_id,
                         int tx_dev_id)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_dev_cmd_enable_req_msg_v01 enable_req;
    qmi_csd_ioctl_dev_cmd_enable_resp_msg_v01 enable_resp;
    qmi_csd_dev_enable_v01 enable_req_payload;
    int remote_rx_dev_id = acdb_loader_get_remote_acdb_id(rx_dev_id);
    int remote_tx_dev_id = acdb_loader_get_remote_acdb_id(tx_dev_id);
    int ec_dev_id;

    /* Use the remote device ID if it exists else use the APQ device ID */
    if (remote_rx_dev_id <= 0)
        remote_rx_dev_id = rx_dev_id;
    if (remote_tx_dev_id <= 0)
        remote_tx_dev_id = tx_dev_id;

    LOGD("%s: Remote rx %d, tx %d\n",
         __func__, remote_rx_dev_id, remote_tx_dev_id);

    memset(&enable_resp, 0, sizeof(enable_resp));
    memset(&enable_req, 0, sizeof(enable_req));
    memset(&enable_req_payload, 0, sizeof(enable_req_payload));

    enable_req.handle = service->device_handle;

    if ((rx_dev_id != -1) && (tx_dev_id != -1)) {
        enable_req_payload.devs_len = 2; // Rx + Tx
        enable_req_payload.devs[0].dev_id = remote_rx_dev_id;
        enable_req_payload.devs[0].dev_attrib.sample_rate =
                                                     get_sample_rate(rx_dev_id);
        enable_req_payload.devs[0].dev_attrib.bits_per_sample = BITS_PER_SAMPLE;
        enable_req_payload.devs[1].dev_id = remote_tx_dev_id;
        enable_req_payload.devs[1].dev_attrib.sample_rate =
                                                     get_sample_rate(tx_dev_id);
        enable_req_payload.devs[1].dev_attrib.bits_per_sample = BITS_PER_SAMPLE;

        if (set_external_ec(rx_dev_id) &&
            service->csd_version >= CSD_VER_ID_WITH_OPTIMIZATION) {
            get_ec_dev_id(&ec_dev_id);
            enable_req.extn_devices_valid = 1;
            enable_req.extn_devices_len = 1;
            enable_req.extn_devices[0].dev_id = ec_dev_id;
            enable_req.extn_devices[0].dev_attrib.sample_rate =
                                                    get_sample_rate(ec_dev_id);
            enable_req.extn_devices[0].dev_attrib.bits_per_sample =
                                                    BITS_PER_SAMPLE;
        }
    } else if ((rx_dev_id != -1) && (tx_dev_id == -1)) {
        enable_req_payload.devs_len = 1; // Rx
        enable_req_payload.devs[0].dev_id = rx_dev_id;
        enable_req_payload.devs[0].dev_attrib.sample_rate =
                                                     get_sample_rate(rx_dev_id);
        enable_req_payload.devs[0].dev_attrib.bits_per_sample= BITS_PER_SAMPLE;
    } else if ((rx_dev_id == -1) && (tx_dev_id != -1)) {
        enable_req_payload.devs_len = 1; // Tx
        enable_req_payload.devs[0].dev_id = tx_dev_id;
        enable_req_payload.devs[0].dev_attrib.sample_rate =
                                                     get_sample_rate(tx_dev_id);
        enable_req_payload.devs[0].dev_attrib.bits_per_sample = BITS_PER_SAMPLE;
    }
    memcpy(&enable_req.qmi_csd_dev_enable_cmd_payload, &enable_req_payload,
           sizeof(enable_req_payload));

    vote_unvote_for_slimbus(AUDIO_SLIMSLAVE_VOTE);
    rc = qmi_client_send_msg_sync(service->csd_handle,
                                  QMI_CSD_IOCTL_DEV_CMD_ENABLE_REQ_V01,
                                  &enable_req, sizeof(enable_req),
                                  &enable_resp, sizeof(enable_resp),
                                  TIMEOUT_MS);
    vote_unvote_for_slimbus(AUDIO_SLIMSLAVE_UNVOTE);
    if (rc == QMI_NO_ERR &&
        csd_success(enable_resp.resp, enable_resp.qmi_csd_status_code_valid,
                    enable_resp.qmi_csd_status_code)) {
        LOGD("%s: dev_id1=%d dev_id=%d successfull\n",
             __func__,rx_dev_id, tx_dev_id);
        return 0;
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
             __func__,
             enable_resp.resp.result, enable_resp.resp.error,
             enable_resp.qmi_csd_status_code_valid,
             enable_resp.qmi_csd_status_code, rc);

        return -1;
    }
}

static int disable_device(struct csd_service *service, int rx_dev_id,
                          int tx_dev_id)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_dev_cmd_disable_req_msg_v01 disable_req;
    qmi_csd_ioctl_dev_cmd_disable_resp_msg_v01 disable_resp;
    int remote_rx_dev_id = acdb_loader_get_remote_acdb_id(rx_dev_id);
    int remote_tx_dev_id = acdb_loader_get_remote_acdb_id(tx_dev_id);
    int ec_dev_id;

    /* Use the remote device ID if it exists else use the APQ device ID */
    if (remote_rx_dev_id <= 0)
        remote_rx_dev_id = rx_dev_id;
    if (remote_tx_dev_id <= 0)
        remote_tx_dev_id = tx_dev_id;

    memset(&disable_resp, 0, sizeof(disable_resp));
    memset(&disable_req, 0, sizeof(disable_req));

    disable_req.handle = service->device_handle;

    if ((rx_dev_id != -1) && (tx_dev_id != -1)) {
        disable_req.qmi_csd_dev_disable_cmd_payload.dev_ids_len = 2; // RX + TX
        disable_req.qmi_csd_dev_disable_cmd_payload.dev_ids[0] =
                                                               remote_rx_dev_id;
        disable_req.qmi_csd_dev_disable_cmd_payload.dev_ids[1] =
                                                               remote_tx_dev_id;
        if (set_external_ec(rx_dev_id) &&
            service->csd_version >= CSD_VER_ID_WITH_OPTIMIZATION) {
            get_ec_dev_id(&ec_dev_id);
            disable_req.extn_devices_valid = 1;
            disable_req.extn_devices_len = 1;
            disable_req.extn_devices[0] = ec_dev_id;
        }
    } else if ((rx_dev_id != -1) && (tx_dev_id == -1)) {
        disable_req.qmi_csd_dev_disable_cmd_payload.dev_ids_len = 1; // Rx
        disable_req.qmi_csd_dev_disable_cmd_payload.dev_ids[0] = rx_dev_id;
    } else if ((rx_dev_id == -1) && (tx_dev_id != -1)) {
        disable_req.qmi_csd_dev_disable_cmd_payload.dev_ids_len = 1; // Tx
        disable_req.qmi_csd_dev_disable_cmd_payload.dev_ids[0] = tx_dev_id;
    }

    vote_unvote_for_slimbus(AUDIO_SLIMSLAVE_VOTE);
    rc = qmi_client_send_msg_sync(service->csd_handle,
                                  QMI_CSD_IOCTL_DEV_CMD_DISABLE_REQ_V01,
                                  &disable_req, sizeof(disable_req),
                                  &disable_resp, sizeof(disable_resp),
                                  TIMEOUT_MS);
    vote_unvote_for_slimbus(AUDIO_SLIMSLAVE_UNVOTE);
    if (rc == QMI_NO_ERR &&
        csd_success(disable_resp.resp, disable_resp.qmi_csd_status_code_valid,
                    disable_resp.qmi_csd_status_code)) {
        return 0;
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
             __func__,
             disable_resp.resp.result, disable_resp.resp.error,
             disable_resp.qmi_csd_status_code_valid,
             disable_resp.qmi_csd_status_code, rc);

        return -1;
    }
}

static int set_aanc_config(struct csd_service *service, int rx_dev_id,
                           int tx_dev_id, int ec_dev_id, int enableFlag)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_dev_cmd_aanc_control_req_msg_v01 req;
    qmi_csd_ioctl_dev_cmd_aanc_control_resp_msg_v01 resp;
    qmi_txn_handle txn = 0;
    int remote_rx_dev_id = acdb_loader_get_remote_acdb_id(rx_dev_id);
    int remote_tx_dev_id = acdb_loader_get_remote_acdb_id(tx_dev_id);

    if (remote_rx_dev_id <= 0)
        remote_rx_dev_id = rx_dev_id;
    if (remote_tx_dev_id <= 0)
        remote_tx_dev_id = tx_dev_id;

    LOGD("%s: rx: %d, tx: %d, ec: %d, enable: %d\n",
         __func__, rx_dev_id, tx_dev_id, ec_dev_id, enableFlag);

    memset(&resp, 0, sizeof(resp));
    memset(&req, 0, sizeof(req));

    req.handle = service->device_handle;
    req.aanc_ctrl =
            enableFlag ?
                    QMI_CSD_DEV_AFE_AANC_ENABLE_V01 :
                    QMI_CSD_DEV_AFE_AANC_DISABLE_V01;

    req.rx_dev_id = remote_rx_dev_id;
    req.tx_dev_id = remote_tx_dev_id;
    req.ref_dev_id = ec_dev_id;

    rc = qmi_client_send_msg_sync(service->csd_handle,
                                  QMI_CSD_IOCTL_DEV_CMD_AANC_CONTROL_REQ_V01,
                                  &req, sizeof(req), &resp, sizeof(resp),
                                  TIMEOUT_MS);
    if (rc == QMI_NO_ERR &&
        csd_success(resp.resp, resp.qmi_csd_status_code_valid,
                    resp.qmi_csd_status_code)) {
        return 0;
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %d, rc %d\n",
             __func__,
             resp.resp.result, resp.resp.error,
             resp.qmi_csd_status_code_valid,
             resp.qmi_csd_status_code, rc);

        return -1;
    }
}

static int enable_aanc_config(struct csd_service *service, int rx_dev_id,
                              int tx_dev_id, int ec_dev_id)
{
    return set_aanc_config(service, rx_dev_id, tx_dev_id,
                           ec_dev_id, AANC_ENABLE);
}

static int disable_aanc_config(struct csd_service *service, int rx_dev_id,
                              int tx_dev_id, int ec_dev_id)
{
    return set_aanc_config(service, rx_dev_id, tx_dev_id,
                           ec_dev_id, AANC_DISABLE);
}

/* Voice stream */
static int open_voice_stream_passive_control(struct csd_service *service,
                                             struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_open_passive_control_voice_stream_req_msg_v01 open_req;
    qmi_csd_open_passive_control_voice_stream_resp_msg_v01 open_resp;
    memset(&open_resp, 0, sizeof(open_resp));
    memset(&open_req, 0, sizeof(open_req));

    strlcpy(open_req.session_name, session->name,
            sizeof(open_req.session_name));

    rc = qmi_client_send_msg_sync(service->csd_handle,
                             QMI_CSD_OPEN_PASSIVE_CONTROL_VOICE_STREAM_REQ_V01,
                             &open_req, sizeof(open_req), &open_resp,
                             sizeof(open_resp), TIMEOUT_MS);
    if (rc == QMI_NO_ERR &&
        csd_success(open_resp.resp, open_resp.open_status_valid,
                    open_resp.open_status)) {
        if (open_resp.qmi_csd_vs_passive_control_handle_valid) {
            session->stream_handle =
                open_resp.qmi_csd_vs_passive_control_handle;

            return 0;
        } else {
            LOGE("%s: Not valid, %d handle %d\n",
                 __func__,
                 open_resp.qmi_csd_vs_passive_control_handle_valid,
                 open_resp.qmi_csd_vs_passive_control_handle);

            return -1;
        }
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
             __func__,
             open_resp.resp.result, open_resp.resp.error,
             open_resp.open_status_valid, open_resp.open_status, rc);

        return -1;
    }
}

static int close_voice_stream_passive_control(struct csd_service *service,
                                              struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_close_req_msg_v01 close_req;
    qmi_csd_close_resp_msg_v01 close_resp;
    memset(&close_resp, 0, sizeof(close_resp));
    memset(&close_req, 0, sizeof(close_req));
    int ret = 0;

    if (session->stream_handle) {
        close_req.handle = session->stream_handle;

        rc = qmi_client_send_msg_sync(service->csd_handle,
                                      QMI_CSD_CLOSE_REQ_V01,
                                      &close_req, sizeof(close_req),
                                      &close_resp, sizeof(close_resp),
                                      TIMEOUT_MS);
        if (rc == QMI_NO_ERR &&
            csd_success(close_resp.resp, close_resp.qmi_csd_status_code_valid,
                        close_resp.qmi_csd_status_code)) {
            session->stream_handle = 0;
            ret = 0;
        } else {
            LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
                 __func__,
                 close_resp.resp.result, close_resp.resp.error,
                 close_resp.qmi_csd_status_code_valid,
                 close_resp.qmi_csd_status_code, rc);
            ret = -1;
        }
    }

    return ret;
}

static int set_stream_mute(struct csd_service *service,
                           struct csd_session *session,
                           int mute)
{
    struct csd_response_msg *csd_resp_msg;
    int ret = 0;
    uint32_t mute_resp_size;
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vs_cmd_set_mute_req_msg_v01 mute_req;
    qmi_csd_ioctl_vs_cmd_set_mute_resp_msg_v01 *mute_resp;
    memset(&mute_req, 0, sizeof(mute_req));
    qmi_txn_handle txn = 0;

    mute_req.handle = session->stream_handle;
    mute_req.qmi_csd_vs_ioctl_set_mute_payload.cmd_token = 0;
    mute_req.qmi_csd_vs_ioctl_set_mute_payload.direction = 0; // TX
    mute_req.qmi_csd_vs_ioctl_set_mute_payload.mute_flag = mute;

    mute_resp_size = sizeof(qmi_csd_ioctl_vs_cmd_set_mute_resp_msg_v01);
    mute_resp = (qmi_csd_ioctl_vs_cmd_set_mute_resp_msg_v01 *)
                                                        malloc(mute_resp_size);
    if (mute_resp != NULL) {
        memset(mute_resp, 0, mute_resp_size);
    } else {
        LOGE("%s: Failed to allocate memory for mute_resp \n", __func__);

        ret = -1;
        goto done;
    }

    ret = set_csd_response_msg(&csd_resp_msg, service, session, mute_resp);
    if (ret < 0)
        goto resp_err;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VS_CMD_SET_MUTE_REQ_V01,
                                   &mute_req, sizeof(mute_req),
                                   mute_resp, mute_resp_size,
                                   csd_client_async_cb, csd_resp_msg, &txn);

    /* If no error dynamically allocated structures will be freed in call back
     * function(csd_client_async_cb) otherwise free here
     */
    if (rc == QMI_NO_ERR) {
        ret = csd_async_wait(session, service, false);
        goto done;
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        ret = -1;
        goto qmi_err;
    }

qmi_err:
    free(csd_resp_msg);

resp_err:
    free(mute_resp);

done:
    return ret;
}

static int set_ui_property(struct csd_service *service,
                           struct csd_session *session,
                           uint32_t module_id,
                           uint8_t flag)
{
    struct csd_response_msg *csd_resp_msg;
    int ret = 0;
    uint32_t property_resp_size;
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vs_cmd_set_ui_property_req_msg_v01 property_req;
    qmi_csd_ioctl_vs_cmd_set_ui_property_resp_msg_v01 *property_resp;
    memset(&property_req, 0, sizeof(property_req));
    qmi_txn_handle txn = 0;

    property_req.handle = session->stream_handle;
    property_req.qmi_csd_vs_ioctl_set_ui_property_payload.cmd_token = 0;
    property_req.qmi_csd_vs_ioctl_set_ui_property_payload.module_id = module_id;
    property_req.qmi_csd_vs_ioctl_set_ui_property_payload.param_id =
        VOICE_PARAM_MOD_ENABLE;
    property_req.qmi_csd_vs_ioctl_set_ui_property_payload.param_data_len = 1;
    property_req.qmi_csd_vs_ioctl_set_ui_property_payload.param_data [0] = flag;

    property_resp_size = sizeof(qmi_csd_ioctl_vs_cmd_set_ui_property_resp_msg_v01);
    property_resp = (qmi_csd_ioctl_vs_cmd_set_ui_property_resp_msg_v01 *)
                                                    malloc(property_resp_size);
    if (property_resp != NULL) {
        memset(property_resp, 0, property_resp_size);
    } else {
        LOGE("%s: Failed to allocate memory for resp_buffer \n", __func__);

        ret = -1;
        goto done;
    }

    ret = set_csd_response_msg(&csd_resp_msg, service, session, property_resp);
    if (ret < 0)
        goto resp_err;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VS_CMD_SET_UI_PROPERTY_REQ_V01,
                                   &property_req, sizeof(property_req),
                                   property_resp, property_resp_size,
                                   csd_client_async_cb, csd_resp_msg, &txn);

    /* If no error dynamically allocated structures will be freed in call back
     * function(csd_client_async_cb) otherwise free here
     */
    if (rc == QMI_NO_ERR) {
        ret = csd_async_wait(session, service, false);
        goto done;
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        ret = -1;
        goto qmi_err;
    }

qmi_err:
    free(csd_resp_msg);

resp_err:
    free(property_resp);

done:
    return ret;
}

static int start_record(struct csd_service *service, struct csd_session *session, int rec_mode)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vs_cmd_start_record_req_msg_v01 start_record_req;
    qmi_csd_ioctl_vs_cmd_start_record_resp_msg_v01 start_record_resp;
    memset(&start_record_req, 0, sizeof(start_record_req));
    memset(&start_record_resp, 0, sizeof(start_record_resp));
    qmi_txn_handle txn = 0;

    if (!session->rec_info.recording) {
        start_record_req.handle = session->stream_handle;
        start_record_req.cmd_token = 0;
        start_record_req.dev_id_valid = 1;
        start_record_req.mode_valid = 1;
        start_record_req.mode = QMI_CSD_VS_RECORD_MODE_TX_RX_MIXING_V01;
        if (rec_mode == INCALL_REC_UPLINK) {
            start_record_req.tx_tap_point = QMI_CSD_VS_TAP_POINT_STREAM_END_V01;
            start_record_req.rx_tap_point = QMI_CSD_VS_TAP_POINT_NONE_V01;
            start_record_req.dev_id = DEVICE_INCALL_VOICE_RECORD_MONO_ACDB_ID;
        } else if (rec_mode == INCALL_REC_DOWNLINK) {
            start_record_req.tx_tap_point = QMI_CSD_VS_TAP_POINT_NONE_V01;
            start_record_req.rx_tap_point = QMI_CSD_VS_TAP_POINT_STREAM_END_V01;
            start_record_req.dev_id = DEVICE_INCALL_VOICE_RECORD_MONO_ACDB_ID;
        } else if (rec_mode == INCALL_REC_UPLINK_AND_DOWNLINK) {
           start_record_req.tx_tap_point = QMI_CSD_VS_TAP_POINT_STREAM_END_V01;
           start_record_req.rx_tap_point = QMI_CSD_VS_TAP_POINT_STREAM_END_V01;
           start_record_req.dev_id = DEVICE_INCALL_VOICE_RECORD_STEREO_ACDB_ID;
        }

        rc = qmi_client_send_msg_async(service->csd_handle,
                                  QMI_CSD_IOCTL_VS_CMD_START_RECORD_REQ_V01,
                                  &start_record_req, sizeof(start_record_req),
                                  &start_record_resp, sizeof(start_record_resp),
                                  csd_client_async_cb, session, &txn);
        if (rc == QMI_NO_ERR) {
            if (!csd_async_wait(session, service, true)) {
                session->rec_info.recording = 1;
                return 0;
            } else {
                return -1;
            }
        } else {
            LOGE("%s: QMI - result %d\n", __func__, rc);
            return -1;
        }
    } else {
        LOGV("%s: incall record already started\n", __func__);
    }
    return 0;
}

static int stop_record(struct csd_service *service, struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vs_cmd_stop_record_req_msg_v01 stop_record_req;
    qmi_csd_ioctl_vs_cmd_stop_record_resp_msg_v01 stop_record_resp;
    memset(&stop_record_req, 0, sizeof(stop_record_req));
    memset(&stop_record_resp, 0, sizeof(stop_record_resp));
    qmi_txn_handle txn = 0;

    if (session->rec_info.recording) {
        stop_record_req.handle = session->stream_handle;
        stop_record_req.cmd_token = 0;

        rc = qmi_client_send_msg_async(service->csd_handle,
                                    QMI_CSD_IOCTL_VS_CMD_STOP_RECORD_REQ_V01,
                                    &stop_record_req, sizeof(stop_record_req),
                                    &stop_record_resp, sizeof(stop_record_resp),
                                    csd_client_async_cb, session, &txn);
        if (rc == QMI_NO_ERR) {
            if (!csd_async_wait(session, service, true)) {
                session->rec_info.recording = 0;
                return 0;
            } else {
                return -1;
            }
         } else {
            LOGE("%s: QMI - result %d\n", __func__, rc);
            return -1;
         }
    } else {
         LOGV("%s: incall record has not started\n", __func__);
    }
    return 0;
}

static int start_playback(struct csd_service *service,
                          struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vs_cmd_start_playback_req_msg_v01 start_playback_req;
    qmi_csd_ioctl_vs_cmd_start_playback_resp_msg_v01 start_playback_resp;
    memset(&start_playback_req, 0, sizeof(start_playback_req));
    memset(&start_playback_resp, 0, sizeof(start_playback_resp));
    qmi_txn_handle txn = 0;

    if (!session->play_info.playing) {
        start_playback_req.handle = session->stream_handle;
        start_playback_req.cmd_token = 0;
        start_playback_req.dev_id = DEVICE_INCALL_MUSIC_DELIVERY_MONO_ACDB_ID;

        rc = qmi_client_send_msg_async(service->csd_handle,
                                    QMI_CSD_IOCTL_VS_CMD_START_PLAYBACK_REQ_V01,
                                    &start_playback_req,
                                                sizeof(start_playback_req),
                                    &start_playback_resp,
                                                sizeof(start_playback_resp),
                                    csd_client_async_cb, session, &txn);
        if (rc == QMI_NO_ERR) {
            if (!csd_async_wait(session, service, true)) {
                session->play_info.playing = 1;
                return 0;
            } else {
                return -1;
            }
        } else {
            LOGE("%s: QMI - result %d\n", __func__, rc);
            return -1;
        }
    } else {
        LOGV("%s: incall playback already started\n", __func__);
    }
    return 0;
}

static int stop_playback(struct csd_service *service,
                         struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vs_cmd_stop_playback_req_msg_v01 stop_playback_req;
    qmi_csd_ioctl_vs_cmd_stop_playback_resp_msg_v01 stop_playback_resp;
    memset(&stop_playback_req, 0, sizeof(stop_playback_req));
    memset(&stop_playback_resp, 0, sizeof(stop_playback_resp));
    qmi_txn_handle txn = 0;

    if (session->play_info.playing) {
        stop_playback_req.handle = session->stream_handle;
        stop_playback_req.cmd_token = 0;

        rc = qmi_client_send_msg_async(service->csd_handle,
                                     QMI_CSD_IOCTL_VS_CMD_STOP_PLAYBACK_REQ_V01,
                                     &stop_playback_req,
                                               sizeof(stop_playback_req),
                                     &stop_playback_resp,
                                               sizeof(stop_playback_resp),
                                     csd_client_async_cb, session, &txn);
        if (rc == QMI_NO_ERR) {
            if (!csd_async_wait(session, service, true)) {
                session->play_info.playing = 0;
                return 0;
            } else {
                return -1;
            }
        } else {
            LOGE("%s: QMI - result %d\n", __func__, rc);
            return -1;
        }
    } else {
        LOGV("%s: incall playback has not started\n", __func__);
    }
    return 0;
}

/* Voice context */
static int open_voice_context_full_control(struct csd_service *service,
                                           struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_open_voice_context_req_msg_v01 open_req;
    qmi_csd_open_voice_context_resp_msg_v01 open_resp;
    memset(&open_resp, 0, sizeof(open_resp));
    memset(&open_req, 0, sizeof(open_req));

    strlcpy(open_req.qmi_csd_vc_open_payload.session_name,
            session->name,
            sizeof(open_req.qmi_csd_vc_open_payload.session_name));
    open_req.qmi_csd_vc_open_payload.direction =
    QMI_CSD_VC_DIRECTION_TX_AND_RX_V01;
    open_req.qmi_csd_vc_open_payload.network_id =
    QMI_CSD_NETWORK_ID_DEFAULT_V01;

    rc = qmi_client_send_msg_sync(service->csd_handle,
                                  QMI_CSD_OPEN_VOICE_CONTEXT_REQ_V01,
                                  &open_req, sizeof(open_req), &open_resp,
                                  sizeof(open_resp), TIMEOUT_MS);
    if (rc == QMI_NO_ERR &&
        csd_success(open_resp.resp, open_resp.open_status_valid,
                    open_resp.open_status)) {
        if (open_resp.qmi_csd_vc_handle_valid) {
            session->context_handle = open_resp.qmi_csd_vc_handle;

            return 0;
        } else {
            LOGE("%s: Not valid, %d handle %d\n",
                 __func__,
                 open_resp.qmi_csd_vc_handle_valid,
                 open_resp.qmi_csd_vc_handle);

            return -1;
        }
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
             __func__,
             open_resp.resp.result, open_resp.resp.error,
             open_resp.open_status_valid, open_resp.open_status, rc);

        return -1;
    }
}

static int close_voice_context_full_control(struct csd_service *service,
                                            struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_close_req_msg_v01 close_req;
    qmi_csd_close_resp_msg_v01 close_resp;
    int ret  = 0;

    memset(&close_resp, 0, sizeof(close_resp));
    memset(&close_req, 0, sizeof(close_req));

    if (session->context_handle) {
        close_req.handle = session->context_handle;

        rc = qmi_client_send_msg_sync(service->csd_handle, QMI_CSD_CLOSE_REQ_V01,
                                      &close_req, sizeof(close_req),
                                      &close_resp, sizeof(close_resp),
                                      TIMEOUT_MS);
        if (rc == QMI_NO_ERR &&
            csd_success(close_resp.resp, close_resp.qmi_csd_status_code_valid,
                        close_resp.qmi_csd_status_code)) {
            session->context_handle = 0;
            ret = 0;
        } else {
            LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
                 __func__,
                 close_resp.resp.result, close_resp.resp.error,
                 close_resp.qmi_csd_status_code_valid,
                 close_resp.qmi_csd_status_code, rc);
            ret = -1;
        }
    }
    return ret;
}

static int set_device_config(struct csd_service *service,
                             struct csd_session *session,
                             int rx_dev_id, int tx_dev_id)
{
    struct csd_response_msg *csd_resp_msg;
    int ret = 0;
    uint32_t config_resp_size;
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vc_cmd_set_device_config_req_msg_v01 config_req;
    qmi_csd_ioctl_vc_cmd_set_device_config_resp_msg_v01 *config_resp;
    int remote_rx_dev_id = acdb_loader_get_remote_acdb_id(rx_dev_id);
    int remote_tx_dev_id = acdb_loader_get_remote_acdb_id(tx_dev_id);

    /* Use the remote device ID if it exists else use the APQ device ID */
    if (remote_rx_dev_id <= 0)
        remote_rx_dev_id = rx_dev_id;
    if (remote_tx_dev_id <= 0)
        remote_tx_dev_id = tx_dev_id;

    memset(&config_req, 0, sizeof(config_req));
    qmi_txn_handle txn = 0;

    config_req.handle = session->context_handle;
    config_req.qmi_csd_vc_ioctl_set_device_config_payload.cmd_token = 0;
    config_req.qmi_csd_vc_ioctl_set_device_config_payload.tx_dev_num =
                                                               remote_tx_dev_id;
    config_req.qmi_csd_vc_ioctl_set_device_config_payload.rx_dev_num =
                                                               remote_rx_dev_id;
    config_req.qmi_csd_vc_ioctl_set_device_config_payload.tx_dev_sr =
                                                     get_sample_rate(tx_dev_id);
    config_req.qmi_csd_vc_ioctl_set_device_config_payload.rx_dev_sr =
                                                     get_sample_rate(rx_dev_id);
    config_req.ec_ref_dev_num_valid = set_external_ec(rx_dev_id);
    config_req.ec_ref_dev_num = csd_client.ec_dev_id;

    config_resp_size = sizeof(qmi_csd_ioctl_vc_cmd_set_device_config_resp_msg_v01);
    config_resp = (qmi_csd_ioctl_vc_cmd_set_device_config_resp_msg_v01 *)
                                                          malloc(config_resp_size);
    if (config_resp != NULL) {
        memset(config_resp, 0, config_resp_size);
    } else {
        LOGE("%s: Failed to allocate memory for resp_buffer \n", __func__);

        ret = -1;
        goto done;
    }

    ret = set_csd_response_msg(&csd_resp_msg, service, session, config_resp);
    if (ret < 0)
        goto resp_err;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VC_CMD_SET_DEVICE_CONFIG_REQ_V01,
                                   &config_req, sizeof(config_req),
                                   config_resp, config_resp_size,
                                   csd_client_async_cb, csd_resp_msg, &txn);

    /* If no error dynamically allocated structures will be freed in call back
     * function(csd_client_async_cb) otherwise free here
     */
    if (rc == QMI_NO_ERR) {
        ret = csd_async_wait(session, service, false);
        goto done;
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        ret = -1;
        goto qmi_err;
    }

qmi_err:
    free(csd_resp_msg);

resp_err:
    free(config_resp);

done:
    return ret;
}

static int enable_voice_context(struct csd_service *service,
                                struct csd_session *session)
{
    struct csd_response_msg *csd_resp_msg;
    int ret = 0;
    uint32_t enable_resp_size;
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vc_cmd_enable_req_msg_v01 enable_req;
    qmi_csd_ioctl_vc_cmd_enable_resp_msg_v01 *enable_resp;
    memset(&enable_req, 0, sizeof(enable_req));
    qmi_txn_handle txn = 0;

    enable_req.handle = session->context_handle;
    enable_req.cmd_token = 0;

    enable_resp_size = sizeof(qmi_csd_ioctl_vc_cmd_enable_resp_msg_v01);
    enable_resp = (qmi_csd_ioctl_vc_cmd_enable_resp_msg_v01 *)
                                                malloc(enable_resp_size);

    if (enable_resp != NULL) {
        memset(enable_resp, 0, enable_resp_size);
    } else {
        LOGE("%s: Failed to allocate memory for resp_buffer \n", __func__);

        ret = -1;
        goto done;
    }

    ret = set_csd_response_msg(&csd_resp_msg, service, session, enable_resp);
    if (ret < 0)
        goto resp_err;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VC_CMD_ENABLE_REQ_V01,
                                   &enable_req, sizeof(enable_req),
                                   enable_resp, enable_resp_size,
                                   csd_client_async_cb, csd_resp_msg, &txn);

    /* If no error dynamically allocated structures will be freed in call back
     * function(csd_client_async_cb) otherwise free here
     */
    if (rc == QMI_NO_ERR) {
        ret = csd_async_wait(session, service, false);
        goto done;
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        ret = -1;
        goto qmi_err;
    }

qmi_err:
    free(csd_resp_msg);

resp_err:
    free(enable_resp);

done:
    return ret;
}

static int disable_voice_context(struct csd_service *service,
                                 struct csd_session *session)
{
    struct csd_response_msg *csd_resp_msg;
    int ret = 0;
    uint32_t disable_resp_size;
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vc_cmd_disable_req_msg_v01 disable_req;
    qmi_csd_ioctl_vc_cmd_disable_resp_msg_v01 *disable_resp;
    memset(&disable_req, 0, sizeof(disable_req));
    qmi_txn_handle txn = 0;

    disable_req.handle = session->context_handle;
    disable_req.cmd_token = 0;

    disable_resp_size = sizeof(qmi_csd_ioctl_vc_cmd_disable_resp_msg_v01);
    disable_resp = (qmi_csd_ioctl_vc_cmd_disable_resp_msg_v01 *)
                                                malloc(disable_resp_size);

    if (disable_resp != NULL) {
        memset(disable_resp, 0, disable_resp_size);
    } else {
        LOGE("%s: Failed to allocate memory for resp_buffer \n", __func__);

        ret = -1;
        goto done;
    }

    ret = set_csd_response_msg(&csd_resp_msg, service, session, disable_resp);
    if (ret < 0)
        goto resp_err;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VC_CMD_DISABLE_REQ_V01,
                                   &disable_req, sizeof(disable_req),
                                   disable_resp, disable_resp_size,
                                   csd_client_async_cb, csd_resp_msg, &txn);

    /* If no error dynamically allocated structures will be freed in call back
     * function(csd_client_async_cb) otherwise free here
     */
    if (rc == QMI_NO_ERR) {
        ret = csd_async_wait(session, service, false);
        goto done;
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        ret = -1;
        goto qmi_err;
    }

qmi_err:
    free(csd_resp_msg);

resp_err:
    free(disable_resp);

done:
    return ret;
}

static int set_rx_volume_index(struct csd_service *service,
                               struct csd_session *session,
                               int volume)
{
    struct csd_response_msg *csd_resp_msg;
    int ret = 0;
    uint32_t vol_resp_size;
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vc_cmd_set_rx_volume_index_req_msg_v01 vol_req;
    qmi_csd_ioctl_vc_cmd_set_rx_volume_index_resp_msg_v01 *vol_resp;
    memset(&vol_req, 0, sizeof(vol_req));
    qmi_txn_handle txn = 0;

    vol_req.handle = session->context_handle;
    vol_req.qmi_csd_vc_ioctl_set_rx_volume_index_payload.cmd_token = 0;
    vol_req.qmi_csd_vc_ioctl_set_rx_volume_index_payload.vol_index = volume;

    vol_resp_size = sizeof(qmi_csd_ioctl_vc_cmd_set_rx_volume_index_resp_msg_v01);
    vol_resp = (qmi_csd_ioctl_vc_cmd_set_rx_volume_index_resp_msg_v01 *)
                   malloc(vol_resp_size);

    if (vol_resp != NULL) {
        memset(vol_resp, 0, vol_resp_size);
    } else {
        LOGE("%s: Failed to allocate memory for resp_buffer \n", __func__);

        ret = -1;
        goto done;
    }

    ret = set_csd_response_msg(&csd_resp_msg, service, session, vol_resp);
    if (ret < 0)
        goto resp_err;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VC_CMD_SET_RX_VOLUME_INDEX_REQ_V01,
                                   &vol_req, sizeof(vol_req),
                                   vol_resp, vol_resp_size,
                                   csd_client_async_cb, csd_resp_msg, &txn);

    /* If no error dynamically allocated structures will be freed in call back
     * function(csd_client_async_cb) otherwise free here
     */
    if (rc == QMI_NO_ERR) {
        ret = csd_async_wait(session, service, false);
        goto done;
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        ret = -1;
        goto qmi_err;
    }

qmi_err:
    free(csd_resp_msg);

resp_err:
    free(vol_resp);

done:
    return ret;
}

static int set_rx_device_mute(struct csd_service *service,
                              struct csd_session *session, int mute)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vc_cmd_set_mute_req_msg_v01 mute_req;
    qmi_csd_ioctl_vc_cmd_set_mute_resp_msg_v01 mute_resp;
    memset(&mute_resp, 0, sizeof(mute_resp));
    memset(&mute_req, 0, sizeof(mute_req));
    qmi_txn_handle txn = 0;

    mute_req.handle = session->context_handle;
    mute_req.qmi_csd_vc_ioctl_set_mute_payload.cmd_token = 0;
    mute_req.qmi_csd_vc_ioctl_set_mute_payload.direction = 1; // RX
    mute_req.qmi_csd_vc_ioctl_set_mute_payload.mute_flag = mute;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VC_CMD_SET_MUTE_REQ_V01,
                                   &mute_req, sizeof(mute_req),
                                   &mute_resp, sizeof(mute_resp),
                                   csd_client_async_cb, session, &txn);
    if (rc == QMI_NO_ERR) {
        if (!csd_async_wait(session, service, true)) {
            return 0;
        } else {
            return -1;
        }
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        return -1;
    }
}

/* Voice manager */
static int open_voice_manager_passive_control(struct csd_service *service,
                                               struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_open_voice_manager_req_msg_v01 open_req;
    qmi_csd_open_voice_manager_resp_msg_v01 open_resp;
    memset(&open_resp, 0, sizeof(open_resp));
    memset(&open_req, 0, sizeof(open_req));

    strlcpy(open_req.qmi_csd_vm_open_payload.session_name,
            session->name,
            sizeof(open_req.qmi_csd_vm_open_payload.session_name));

    rc = qmi_client_send_msg_sync(service->csd_handle,
                                  QMI_CSD_OPEN_VOICE_MANAGER_REQ_V01,
                                  &open_req, sizeof(open_req),
                                  &open_resp, sizeof(open_resp),
                                  TIMEOUT_MS);
    if (rc == QMI_NO_ERR &&
        csd_success(open_resp.resp, open_resp.open_status_valid,
                    open_resp.open_status)) {
        if (open_resp.qmi_csd_vm_handle_valid) {
            session->manager_handle = open_resp.qmi_csd_vm_handle;

            return 0;
        } else {
            LOGE("%s: Not valid %d, handle %d\n",
                 __func__,
                 open_resp.qmi_csd_vm_handle_valid,
                 open_resp.qmi_csd_vm_handle);

            return -1;
        }
    } else {
        LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
             __func__,
             open_resp.resp.result, open_resp.resp.error,
             open_resp.open_status_valid, open_resp.open_status, rc);

        return -1;
    }
}

static int close_voice_manager_passive_control(struct csd_service *service,
                                               struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_close_req_msg_v01 close_req;
    qmi_csd_close_resp_msg_v01 close_resp;
    int ret = 0;

    memset(&close_resp, 0, sizeof(close_resp));
    memset(&close_req, 0, sizeof(close_req));

    if (session->manager_handle) {
        close_req.handle = session->manager_handle;

        rc = qmi_client_send_msg_sync(service->csd_handle,
                                      QMI_CSD_CLOSE_REQ_V01,
                                      &close_req, sizeof(close_req),
                                      &close_resp, sizeof(close_resp),
                                      TIMEOUT_MS);

        if (rc == QMI_NO_ERR &&
            csd_success(close_resp.resp, close_resp.qmi_csd_status_code_valid,
                        close_resp.qmi_csd_status_code)) {
            session->manager_handle = 0;
            ret = 0;
        } else {
            LOGE("%s: QMI - result %d, error %d, CSD - valid %d, status %x, rc %d\n",
                 __func__,
                 close_resp.resp.result, close_resp.resp.error,
                 close_resp.qmi_csd_status_code_valid,
                 close_resp.qmi_csd_status_code, rc);
            ret = -1;
        }
    }
    return ret;
}

static int attach_voice_context(struct csd_service *service,
                                struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vm_cmd_attach_context_req_msg_v01 attach_req;
    qmi_csd_ioctl_vm_cmd_attach_context_resp_msg_v01 attach_resp;
    memset(&attach_resp, 0, sizeof(attach_resp));
    memset(&attach_req, 0, sizeof(attach_req));
    qmi_txn_handle txn = 0;

    attach_req.handle = session->manager_handle;
    attach_req.qmi_csd_vm_ioctl_attach_context_payload.cmd_token = 0;
    attach_req.qmi_csd_vm_ioctl_attach_context_payload.context_handle =
                                                        session->context_handle;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VM_CMD_ATTACH_CONTEXT_REQ_V01,
                                   &attach_req, sizeof(attach_req),
                                   &attach_resp, sizeof(attach_resp),
                                   csd_client_async_cb, session, &txn);
    if (rc == QMI_NO_ERR) {
        if (!csd_async_wait(session, service, true)) {
            return 0;
        } else {
            return -1;
        }
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        return -1;
    }
}

static int detach_voice_context(struct csd_service *service,
                                struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vm_cmd_detach_context_req_msg_v01 detach_req;
    qmi_csd_ioctl_vm_cmd_detach_context_resp_msg_v01 detach_resp;
    memset(&detach_resp, 0, sizeof(detach_resp));
    memset(&detach_req, 0, sizeof(detach_req));
    qmi_txn_handle txn = 0;

    detach_req.handle = session->manager_handle;
    detach_req.qmi_csd_vm_ioctl_detach_context_payload.cmd_token = 0;
    detach_req.qmi_csd_vm_ioctl_detach_context_payload.context_handle =
                                                        session->context_handle;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VM_CMD_DETACH_CONTEXT_REQ_V01,
                                   &detach_req, sizeof(detach_req),
                                   &detach_resp, sizeof(detach_resp),
                                   csd_client_async_cb, session, &txn);
    if (rc == QMI_NO_ERR) {
        if (!csd_async_wait(session, service, true)) {
            return 0;
        } else {
            return -1;
        }
    } else {
        LOGE("detach_context: QMI - result %d\n", rc);

        return -1;
    }
}

static int start_voice_manager(struct csd_service *service,
                               struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vm_cmd_start_voice_req_msg_v01 start_req;
    qmi_csd_ioctl_vm_cmd_start_voice_resp_msg_v01 start_resp;
    memset(&start_resp, 0, sizeof(start_resp));
    memset(&start_req, 0, sizeof(start_req));
    qmi_txn_handle txn = 0;

    start_req.handle = session->manager_handle;
    start_req.cmd_token = 0;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VM_CMD_START_VOICE_REQ_V01,
                                   &start_req, sizeof(start_req),
                                   &start_resp, sizeof(start_resp),
                                   csd_client_async_cb, session, &txn);
    if (rc == QMI_NO_ERR) {
        if (!csd_async_wait(session, service, true)) {
            return 0;
        } else {
            return -1;
        }
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        return -1;
    }
}

static int standby_voice_manager( struct csd_service *service,
                                  struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vm_cmd_standby_voice_req_msg_v01 standby_req;
    qmi_csd_ioctl_vm_cmd_standby_voice_resp_msg_v01 standby_resp;
    qmi_txn_handle txn = 0;
    memset(&standby_resp, 0, sizeof(standby_resp));
    memset(&standby_req, 0, sizeof(standby_req));

    standby_req.handle = session->manager_handle;
    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VM_CMD_STANDBY_VOICE_REQ_V01,
                                   &standby_req, sizeof(standby_req),
                                   &standby_resp, sizeof(standby_resp),
                                   csd_client_async_cb,
                                   session, &txn);
    if (rc == QMI_NO_ERR) {
        if (!csd_async_wait(session, service, true)) {
            return 0;
        } else {
            return -1;
        }
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        return -1;
    }
}

static int stop_voice_manager(struct csd_service *service,
                              struct csd_session *session)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vm_cmd_stop_voice_req_msg_v01 stop_req;
    qmi_csd_ioctl_vm_cmd_stop_voice_resp_msg_v01 stop_resp;
    memset(&stop_resp, 0, sizeof(stop_resp));
    memset(&stop_req, 0, sizeof(stop_req));
    qmi_txn_handle txn = 0;

    stop_req.handle = session->manager_handle;
    stop_req.cmd_token = 0;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VM_CMD_STOP_VOICE_REQ_V01,
                                   &stop_req, sizeof(stop_req),
                                   &stop_resp, sizeof(stop_resp),
                                   csd_client_async_cb, session, &txn);
    if (rc == QMI_NO_ERR) {
        if (!csd_async_wait(session, service, true)) {
            return 0;
        } else {
            return -1;
        }
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        return -1;
    }
}

static int set_wide_voice(struct csd_service *service,
                          struct csd_session *session,
                          uint8_t wide_voice)
{
    struct csd_response_msg *csd_resp_msg;
    int ret = 0;
    uint32_t wide_voice_resp_size;
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vm_cmd_set_widevoice_req_msg_v01 wide_voice_req;
    qmi_csd_ioctl_vm_cmd_set_widevoice_resp_msg_v01 *wide_voice_resp;
    memset(&wide_voice_req, 0, sizeof(wide_voice_req));
    qmi_txn_handle txn = 0;

    wide_voice_req.handle = session->manager_handle;
    wide_voice_req.qmi_csd_vm_ioctl_set_widevoice_payload.cmd_token = 0;
    wide_voice_req.qmi_csd_vm_ioctl_set_widevoice_payload.enable =
        wide_voice;

    wide_voice_resp_size = sizeof(qmi_csd_ioctl_vm_cmd_set_widevoice_resp_msg_v01);
    wide_voice_resp = (qmi_csd_ioctl_vm_cmd_set_widevoice_resp_msg_v01 *)
                   malloc(wide_voice_resp_size);

    if (wide_voice_resp != NULL) {
        memset(wide_voice_resp, 0, wide_voice_resp_size);
    } else {
        LOGE("%s: Failed to allocate memory for resp_buffer \n", __func__);

        ret = -1;
        goto done;
    }

    ret = set_csd_response_msg(&csd_resp_msg, service, session, wide_voice_resp);
    if (ret < 0)
        goto resp_err;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VM_CMD_SET_WIDEVOICE_REQ_V01,
                                   &wide_voice_req, sizeof(wide_voice_req),
                                   wide_voice_resp, wide_voice_resp_size,
                                   csd_client_async_cb, csd_resp_msg, &txn);

    /* If no error dynamically allocated structures will be freed in call back
     * function(csd_client_async_cb) otherwise free here
     */
    if (rc == QMI_NO_ERR) {
        ret = csd_async_wait(session, service, false);
        goto done;
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        ret = -1;
        goto qmi_err;
    }

qmi_err:
    free(csd_resp_msg);

resp_err:
    free(wide_voice_resp);

done:
    return ret;
}

static int set_csd_tty_mode(struct csd_client *client,
                            struct csd_service *service,
                            struct csd_session *session)
{
    struct csd_response_msg *csd_resp_msg;
    int ret = 0;
    uint32_t tty_resp_size;
    qmi_client_error_type rc = QMI_NO_ERR;
    qmi_csd_ioctl_vm_cmd_set_tty_mode_req_msg_v01 tty_req;
    qmi_csd_ioctl_vm_cmd_set_tty_mode_resp_msg_v01 *tty_resp;
    memset(&tty_req, 0, sizeof(tty_req));
    qmi_txn_handle txn = 0;

    if ((client->rx_dev_id != DEVICE_TTY_HEADSET_MONO_RX_ACDB_ID) &&
        (client->tx_dev_id != DEVICE_TTY_HEADSET_MONO_TX_ACDB_ID)) {
        return 0;
    }

    tty_req.handle = session->manager_handle;
    tty_req.qmi_csd_vm_ioctl_set_tty_mode_payload.cmd_token = 0;
    tty_req.qmi_csd_vm_ioctl_set_tty_mode_payload.mode =
               get_csd_tty_mode(client->tty_mode);

    tty_resp_size = sizeof(qmi_csd_ioctl_vm_cmd_set_tty_mode_resp_msg_v01);
    tty_resp = (qmi_csd_ioctl_vm_cmd_set_tty_mode_resp_msg_v01 *)
                   malloc(tty_resp_size);

    if (tty_resp != NULL) {
        memset(tty_resp, 0, tty_resp_size);
    } else {
        LOGE("%s: Failed to allocate memory for resp_buffer \n", __func__);

        ret = -1;
        goto done;
    }

    ret = set_csd_response_msg(&csd_resp_msg, service, session, tty_resp);
    if (ret < 0)
        goto resp_err;

    rc = qmi_client_send_msg_async(service->csd_handle,
                                   QMI_CSD_IOCTL_VM_CMD_SET_TTY_MODE_REQ_V01,
                                   &tty_req, sizeof(tty_req),
                                   tty_resp, tty_resp_size,
                                   csd_client_async_cb, csd_resp_msg, &txn);

    /* If no error dynamically allocated structures will be freed in call back
     * function(csd_client_async_cb) otherwise free here
     */
    if (rc == QMI_NO_ERR) {
        ret = csd_async_wait(session, service, false);
        goto done;
    } else {
        LOGE("%s: QMI - result %d\n", __func__, rc);

        ret = -1;
        goto qmi_err;
    }

qmi_err:
    free(csd_resp_msg);

resp_err:
    free(tty_resp);

done:
    return ret;
}

static int csd_disable_devices(struct csd_service *service)
{
    int rc = 0;
    int rx_dev_id = 0;
    int tx_dev_id = 0;
    int ec_dev_id = 0;
    uint32_t aanc_mode = 0;

    if (service->device_state == DEVICE_ENABLED) {
        get_dev_ids(&rx_dev_id, &tx_dev_id, &ec_dev_id);
        get_aanc_mode(&aanc_mode);

        if (aanc_mode) {
            rc = disable_aanc_config(service, rx_dev_id, tx_dev_id, ec_dev_id);
            if (rc < 0) {
                LOGE("%s: Error %d disabling aanc config\n", __func__, rc);

                goto done;
            }
        }

        if (set_external_ec(rx_dev_id) &&
            service->csd_version < CSD_VER_ID_WITH_OPTIMIZATION) {
            /* Enable external EC reference device. */
            rc = disable_device(service, -1, ec_dev_id);
            if (rc < 0) {
                LOGE("%s: Error %d disabling external EC device\n",
                     __func__, rc);

                goto done;
            }
        }

        rc = disable_device(service, rx_dev_id, tx_dev_id);
        if (rc < 0) {
            LOGE("%s: Error %d disabling device\n", __func__, rc);

            goto done;
        }

        service->device_state = DEVICE_DISABLED;
        LOGD("%s: devices disabled rx=%d, tx=%d, ec=%d\n",
             __func__, rx_dev_id, tx_dev_id, ec_dev_id);

        goto done;
    } else {
        LOGE("%s: Invalid device state %d\n", __func__, rc);

        if (service->device_state == DEVICE_INVALID)
            rc = -1;

        goto done;
    }

done:
    return rc;
}

static int csd_enable_devices(struct csd_service *service)
{
    int rc = 0;
    int rx_dev_id = 0;
    int tx_dev_id = 0;
    int ec_dev_id = 0;
    uint32_t aanc_mode = 0;

    if (service->device_state == DEVICE_DISABLED) {
        get_dev_ids(&rx_dev_id, &tx_dev_id, &ec_dev_id);
        rc = enable_device(service, rx_dev_id, tx_dev_id);
        if (rc < 0) {
            LOGE("%s: Error %d enabling device\n", __func__, rc);

            goto done;
        }

        if (set_external_ec(rx_dev_id) &&
            service->csd_version < CSD_VER_ID_WITH_OPTIMIZATION) {
            /* Enable external EC reference device. */
            rc = enable_device(service, -1, ec_dev_id);
            if (rc < 0) {
                LOGE("%s: Error %d enabling external EC device\n",
                     __func__, rc);

                goto disable_dev;
            }
        }

        get_aanc_mode(&aanc_mode);
        if (aanc_mode) {
            rc = enable_aanc_config(service, rx_dev_id, tx_dev_id, ec_dev_id);
            if (rc < 0) {
                LOGE("%s: Error %d enabling aanc config\n", __func__, rc);

                goto disable_ec_dev;
            }
        }

        service->device_state = DEVICE_ENABLED;

        LOGD("%s: devices enabled rx=%d, tx=%d, ec=%d aanc_mode=%d\n",
             __func__, rx_dev_id, tx_dev_id, ec_dev_id, aanc_mode);
    } else {
        LOGE("%s: Invalid device state %d\n", __func__, rc);

        if (service->device_state == DEVICE_INVALID)
            rc = -1;

        goto done;
    }

    goto done;

disable_ec_dev:
        if (set_external_ec(rx_dev_id) &&
            service->csd_version < CSD_VER_ID_WITH_OPTIMIZATION) {
            disable_device(service, -1, ec_dev_id);
        }

disable_dev:
    disable_device(service, rx_dev_id, tx_dev_id);

done:
    return rc;
}

static int csd_service_init(struct csd_service *service)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    uint32_t num_services = 0, num_entries = 0;
    qmi_idl_service_object_type csd_service_obj;
    qmi_service_info info[MAX_SERVICE_INFO];

    /* Get the service object for CSD */
    csd_service_obj = csd_get_service_object_v01();

    /* Wait till the service is up */
    rc = qmi_client_notifier_init(csd_service_obj,
                                  &service->os_params,
                                  &service->notifier);
    if (rc != QMI_NO_ERR) {
        LOGE("%s: Error %d for qmi_client_notifier_init\n", __func__, rc);

        rc = -1;
        goto done;
    }
    LOGD("%s: qmi_client_notifier_init() successful\n", __func__);

    while (1) {
        rc = qmi_client_get_service_instance(csd_service_obj,
                                             service->qmi_instance_id,
                                             &info[0]);
        if (rc == QMI_NO_ERR)
            break;

        QMI_CCI_OS_SIGNAL_WAIT(&service->os_params, 0);
    }

    /* Connect to the CSD service */
    rc = qmi_client_init(&info[0], csd_service_obj, csd_client_ind_cb,
                         &service, NULL, &service->csd_handle);
    if (rc != QMI_NO_ERR) {
        LOGE("%s: Error %d for qmi_client_init\n", __func__, rc);

        rc = -1;
        goto rel_notifier;
    }

    /* Register error callback for error notifications */
    rc = qmi_client_register_error_cb(service->csd_handle,
                                      csd_client_error_cb, service);
    if (rc != QMI_NO_ERR) {
        LOGE("%s: Error %d for qmi_client_register_error_cb\n",__func__, rc);

        rc = -1;
        goto rel_csd;
    }

    rc = get_csd_version(service);
    if (rc < 0) {
        LOGE("%s: Error %d while getting CSD version\n", __func__, rc);
    }

    rc = init(service);
    if (rc < 0) {
        LOGE("%s: Error %d initing CSD\n", __func__, rc);

        rc = -1;
        goto rel_csd;
    }

    goto done;

deinit_csd:
    deinit(service);

rel_csd:
    qmi_client_release(service->csd_handle);
    service->csd_handle = NULL;

rel_notifier:
    qmi_client_release(service->notifier);
    service->notifier = NULL;
    memset(&service->os_params, 0, sizeof(service->os_params));

done:
    return rc;
}

static int csd_close_session_handles(struct csd_service *service,
                                     int session_id)
{

    int rc = 0;

    rc = close_voice_stream_passive_control(service,
                                            &service->sessions[session_id]);
    if (rc < 0)
        LOGE("%s: Error %d opening voice stream\n", __func__, rc);

    rc = close_voice_context_full_control(service,
                                          &service->sessions[session_id]);
    if (rc < 0)
        LOGE("%s: Error %d opening voice context\n", __func__, rc);

    rc = close_voice_manager_passive_control(service,
                                             &service->sessions[session_id]);
    if (rc < 0)
        LOGE("%s: Error %d opening voice manager\n", __func__, rc);

    service->sessions[session_id].state = SESSION_INVALID;

    return rc;
}


static int csd_open_session_handles(struct csd_service *service, int session_id)
{
    int rc = 0;

    rc = open_voice_stream_passive_control(service,
                                           &service->sessions[session_id]);
    if (rc < 0) {
        LOGE("%s: Error %d opening voice stream\n", __func__, rc);

        goto done;
    }

    rc = open_voice_context_full_control(service,
                                         &service->sessions[session_id]);
    if (rc < 0) {
        LOGE("%s: Error %d opening voice context\n", __func__, rc);

        goto done;
    }

    rc = open_voice_manager_passive_control(service,
                                            &service->sessions[session_id]);
    if (rc < 0) {
        LOGE("%s: Error %d opening voice manager\n", __func__, rc);

        goto done;
    }

    if (service->csd_version >= CSD_VER_ID_WITH_OPTIMIZATION) {
        rc = attach_voice_context(service, &service->sessions[session_id]);
        if (rc < 0) {
            LOGE("%s: Error %d attaching voice context\n", __func__, rc);

            goto done;
        }
    }

    service->sessions[session_id].state = SESSION_CREATED;

done:

    return rc;
}

static int csd_init(struct csd_service *service)
{
    int rc = 0;
    int i;

    rc = csd_service_init(service);
    if (rc < 0) {
        LOGE("%s: Error %d initializing csd service \n", __func__, rc);

        goto done;
    }

    rc = open_device_control(service);
    if (rc < 0) {
        LOGE("%s: Error %d opening device control\n", __func__, rc);

        goto done;
    }

    service->device_state = DEVICE_DISABLED;

    if (fusion_dual_mdm() || fusion_single_mdm()) {

        rc = csd_open_session_handles(service, VOICE_SESSION_ID);
        if (rc < 0) {
            LOGE("%s: Error %d opening session handles for VOICE session\n",
                  __func__, rc);
            goto done;
        }

        rc = csd_open_session_handles(service, VOLTE_SESSION_ID);
        if (rc < 0) {
            LOGE("%s: Error %d opening session handles for VOLTE session\n",
                  __func__, rc);

            csd_close_session_handles(service, VOICE_SESSION_ID);
            goto done;
        }
    } else if (fusion_sglte()) {
        rc = csd_open_session_handles(service, VOICE_SESSION_ID);
        if (rc < 0) {
            LOGE("%s: Error %d opening session handles for VOICE session\n",
                  __func__, rc);

            goto done;
        }

        rc = csd_open_session_handles(service, VOICE2_SESSION_ID);
        if (rc < 0) {
            LOGE("%s: Error %d pening session handles for VOICE2 session\n",
                  __func__, rc);

            csd_close_session_handles(service, VOICE_SESSION_ID);
            goto done;
        }
    }

done:
    if (rc < 0) {
        LOGE("%s: Error %d closing device control\n", __func__, rc);
    }
    return rc;
}


static int csd_service_deinit(struct csd_service *service)
{
    qmi_client_error_type rc = QMI_NO_ERR;
    int ret = 0;

    ret = deinit(service);
    if (ret < 0) {
        LOGE("%s: Error %d deiniting CSD\n", __func__, rc);
    }

    rc = qmi_client_release(service->csd_handle);
    if (rc != QMI_NO_ERR) {
        LOGE("%s: Error %d for release of csd_handle\n", __func__, rc);

        ret = -1;
        goto done;
    }

    rc = qmi_client_release(service->notifier);
    LOGE("%s: notifier handle release rc:%d", __func__, rc);
    if (rc != QMI_NO_ERR) {
        LOGE("%s: Error %d for release of notifier\n", __func__, rc);

        ret = -1;
        goto done;
    }

    service->csd_handle = NULL;
    service->notifier = NULL;
    memset(&service->os_params, 0, sizeof(service->os_params));

done:
    return ret;
}

static int csd_deinit(struct csd_service *service)
{
    int i = 0;
    struct csd_session *session = NULL;

    for (i = 0; i< ALL_SESSION_ID; i++) {
        session = &service->sessions[i];

        close_voice_context_full_control(service, session);
        close_voice_stream_passive_control(service, session);
        close_voice_manager_passive_control(service, session);
        if (service->csd_version >= CSD_VER_ID_WITH_OPTIMIZATION)
            detach_voice_context(service, session);

        session->state = SESSION_INVALID;
    }

    close_device_control(service);
    service->device_state = DEVICE_INVALID;

    csd_service_deinit(service);

    return 0;
}

static void* csd_thread_main(void *data)
{
    int rc = 0;
    struct csd_service * service = (struct csd_service *)data;

    pthread_mutex_lock(&service->csd_service_mutex);
    rc = csd_init(service);
    if (rc < 0)
        LOGE("%s: Error %d csd_client_service_init\n",
             __func__, rc);

    pthread_mutex_unlock(&service->csd_service_mutex);

    while (true) {
        pthread_mutex_lock(&service->csd_thread_mutex);
        pthread_cond_wait(&service->csd_thread_condition,
                          &service->csd_thread_mutex);
        pthread_mutex_unlock(&service->csd_thread_mutex);

        pthread_mutex_lock(&service->csd_service_mutex);
        switch (service->thread_command) {

        case CSD_REINIT:
            rc = csd_deinit(service);
            if (rc < 0) {
                LOGE("%s: Error %d deiniting csd_client\n", __func__, rc);
            } else {
                rc = csd_init(service);
                if (rc < 0)
                    LOGE("%s: Error %d initing csd_client\n",__func__, rc);
            }
            break;

         case CSD_DEINIT:
            rc = csd_deinit(service);
            if (rc < 0) {
                LOGE("%s: Error %d deiniting csd_client\n",__func__, rc);
            }

             pthread_mutex_unlock(&service->csd_service_mutex);
             goto exitthread;

         default:
            LOGE("%s: Wrong command", __func__);
            break;
        }
        service->thread_command = CSD_NONE;
         pthread_mutex_unlock(&service->csd_service_mutex);
    }

exitthread:
    return NULL;
}

int csd_client_start_voice(uint32_t vsid)
{
    int rc = 0;
    int incall_dev = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    int rx_dev_id = 0;
    int tx_dev_id = 0;
    int ec_dev_id = 0;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID - 1)) {
        rc = -1;
        LOGE("%s: Invalid vsid id %d sessionid id =%d error=%d\n",
             __func__, vsid, sessionid, rc);

        goto done;
    }

    service = get_service_by_sessionid(sessionid);
    session = get_session_by_sessionid(sessionid);

    pthread_mutex_lock(&service->csd_service_mutex);

    LOGD("%s: device_state=%d, session_state=%d\n",
         __func__, service->device_state, session->state);

    /* Check for valid service and session state */
    if (session->state == SESSION_CREATED ||
        session->state == SESSION_STOPPED) {

        rc = csd_enable_devices(service);
        if (rc < 0) {
            LOGE("%s: Error %d enabling devices\n", __func__, rc);

            goto done;
        }

        get_dev_ids(&rx_dev_id, &tx_dev_id, &ec_dev_id);
        rc = set_device_config(service, session, rx_dev_id, tx_dev_id);
        if (rc < 0) {
            LOGE("%s: Error %d setting device config\n", __func__, rc);

            goto disable_dev;
        }

        rc = enable_voice_context(service, session);
        if (rc < 0) {
            LOGE("%s: Error %d enabling voice context\n", __func__, rc);

            goto disable_dev;
        }

        if (service->csd_version < CSD_VER_ID_WITH_OPTIMIZATION) {
            rc = attach_voice_context(service, session);
            if (rc < 0) {
                LOGE("%s: Error %d attaching voice context\n", __func__, rc);

                goto disable_vc;
            }
        }

        if (session->wide_voice) {
            rc = set_wide_voice(service, session, session->wide_voice);
            if (rc < 0) {
                LOGE("%s: Error %d setting wide voice\n",
                     __func__, rc);
                /* Not a critical error allow voice call to continue */
            }
        }

        if (session->slow_talk) {
            rc = set_ui_property(service, session,
                                 MODULE_ID_VOICE_MODULE_ST,
                                 session->slow_talk);
            if (rc < 0) {
                LOGE("%s: Error %d setting slow talk\n",
                     __func__, rc);
                /* Not a critical error allow voice call to continue. */
            }
        }

        if (session->fens) {
            rc = set_ui_property(service, session,
                                 MODULE_ID_VOICE_MODULE_FENS,
                                 session->fens);
            if (rc < 0) {
                LOGE("%s: Error %d setting FENS\n", __func__, rc);
                /* Not a critical error allow voice call to continue */
            }
        }

        if (csd_client.tty_mode != TTY_OFF) {
            rc = set_csd_tty_mode(&csd_client, service, session);
            if (rc < 0) {
                LOGE("%s: Error %d setting tty mode\n", __func__, rc);
                /* Not a critical error allow voice call to continue */
            }
        }

        rc = start_voice_manager(service, session);
        if (rc < 0) {
            LOGE("%s: Error %d starting voice manager \n",__func__, rc);
            goto detach_vc;
        }

        session->state = SESSION_STARTED;

        rc = set_rx_volume_index(service, session, session->volume);
        if (rc < 0) {
            LOGE("%s: Error %d setting volume %d\n",
                 __func__, rc, session->volume);
            /* Not a critical error allow voice call to continue */
        }

        rc = set_stream_mute(service, session, session->mute);
        if (rc < 0) {
            LOGE("%s: Error %d setting mute %d\n",
                 __func__, rc, session->mute);
            /* Not a critical error allow voice call to continue */
        }

        /* Start record if enabled */
        if (session->rec_info.rec_enable) {
            if (session->rec_info.rec_mode == INCALL_REC_UPLINK_AND_DOWNLINK) {
                incall_dev = DEVICE_INCALL_VOICE_RECORD_STEREO_ACDB_ID;
            } else {
                incall_dev = DEVICE_INCALL_VOICE_RECORD_MONO_ACDB_ID;
            }

            rc = enable_device(service, incall_dev, -1);
            if (rc < 0) {
                LOGE("%s: Error %d enabling device\n", __func__, rc);
            } else {
                /* Start record */
                rc = start_record(service, session, session->rec_info.rec_mode);
                if (rc < 0) {
                    LOGE("%s: Error %d start record\n", __func__, rc);
                    disable_device(service, incall_dev, -1);
                }
            }
        }

        /* Start playback if enabled */
        if (session->play_info.play_enable) {
            /* Enable device */
            rc = enable_device(service,
                               DEVICE_INCALL_MUSIC_DELIVERY_MONO_ACDB_ID, -1);
            if (rc < 0) {
                LOGE("%s: Error %d enabling device\n", __func__, rc);
            } else {
                /* Start playback */
                rc = start_playback(service, session);
                if (rc < 0) {
                    LOGE("%s: Error %d start playback\n", __func__, rc);
                    disable_device(service,
                                   DEVICE_INCALL_MUSIC_DELIVERY_MONO_ACDB_ID, -1);
                }
            }
        }
        goto done;
    } else {
        LOGE("%s: Invalid session state %d", __func__, session->state);

        rc = -1;
        goto done;
    }

detach_vc:
    if (service->csd_version < CSD_VER_ID_WITH_OPTIMIZATION)
        detach_voice_context(service, session);

disable_vc:
    disable_voice_context(service, session);

disable_dev:
    csd_disable_devices(service);

done:
    pthread_mutex_unlock(&service->csd_service_mutex);
    return rc;
}

int csd_client_stop_voice(uint32_t vsid)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID - 1)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    service = get_service_by_sessionid(sessionid);
    session = get_session_by_sessionid(sessionid);

    pthread_mutex_lock(&service->csd_service_mutex);

    LOGD("%s: device_state=%d, session_state=%d\n",
         __func__, service->device_state, session->state);

    if (session->state == SESSION_STARTED ||
        session->state == SESSION_STANDBY) {

        rc = stop_voice_manager(service, session);
        if (rc < 0) {
            LOGE("%s: Error %d stopping voice manager\n", __func__, rc);

            goto done;
        }

        if (service->csd_version < CSD_VER_ID_WITH_OPTIMIZATION) {
            rc = detach_voice_context(service, session);
            if (rc < 0) {
                LOGE("%s: Error %d detaching voice context\n", __func__, rc);

                goto done;
            }
        }

        rc = disable_voice_context(service, session);
        if (rc < 0) {
            LOGE("%s: Error %d disabling voice context\n", __func__, rc);

            goto done;
        }

        if (session->rec_info.recording) {
            /* Stop record */
            rc = stop_record(service, session);
            if (rc < 0) {
                LOGE("%s: Error %d stop record\n", __func__, rc);
            }

            /* Disable device */
            if (session->rec_info.rec_mode == INCALL_REC_UPLINK_AND_DOWNLINK) {
                rc = disable_device(service,
                                    DEVICE_INCALL_VOICE_RECORD_STEREO_ACDB_ID, -1);
                if (rc < 0) {
                    LOGE("%s: Error %d disable device\n", __func__, rc);
                }
            } else {
                rc = disable_device(service,
                                    DEVICE_INCALL_VOICE_RECORD_MONO_ACDB_ID, -1);
                if (rc < 0) {
                    LOGE("%s: Error %d disable device\n", __func__, rc);
                }
            }
        }

        if (session->play_info.playing) {
            /* Stop record */
            rc = stop_playback(service, session);
            if (rc < 0) {
                LOGE("%s: Error %d stop record\n", __func__, rc);
            }

            /* Disable device */
            rc = disable_device(service,
                                DEVICE_INCALL_MUSIC_DELIVERY_MONO_ACDB_ID, -1);

            if (rc < 0) {
                LOGE("%s: Error %d disable device\n", __func__, rc);
            }
        }

        rc = csd_disable_devices(service);
        if (rc < 0) {
            LOGE("%s: Error %d disabling devices \n", __func__, rc);

            goto done;
        }

        session->state = SESSION_STOPPED;
    } else {
        LOGE("%s: Invalid session state %d", __func__, session->state);

        rc = -1;
        goto done;
    }

done:
    pthread_mutex_unlock(&service->csd_service_mutex);
    return rc;
}

int csd_client_standby_voice(uint32_t vsid)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID - 1)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    service = get_service_by_sessionid(sessionid);
    session = get_session_by_sessionid(sessionid);

    pthread_mutex_lock(&service->csd_service_mutex);

    /* Check for valid session state */
    if (session->state == SESSION_STARTED) {
        rc = standby_voice_manager(service, session);
        if (rc < 0) {
            LOGE("%s: Error %d standby voice manager control\n", __func__, rc);

            goto done;
        }

        rc = disable_voice_context(service, session);
        if (rc < 0) {
            LOGE("%s: Error %d disabling voice context\n", __func__, rc);

            goto done;
        }

        session->state = SESSION_STANDBY;
    } else {
        LOGE("%s: Invalid session state %d", __func__, session->state);

        rc = -1;
        goto done;
    }

done:
    pthread_mutex_unlock(&service->csd_service_mutex);
    return rc;
}

int csd_client_resume_voice(uint32_t vsid)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    int rx_dev_id = 0;
    int tx_dev_id = 0;
    int ec_dev_id = 0;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID - 1)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }
    service = get_service_by_sessionid(sessionid);
    session = get_session_by_sessionid(sessionid);

    pthread_mutex_lock(&service->csd_service_mutex);

    if (session->state != SESSION_STANDBY) {
        LOGE("%s: Resume received in invalid state %d\n",
             __func__, session->state);

        rc = -1;
        goto done;
    }

    if (session->state == SESSION_STANDBY) {

        get_dev_ids(&rx_dev_id, &tx_dev_id, &ec_dev_id);
        rc = set_device_config(service, session, rx_dev_id, tx_dev_id);
        if (rc < 0) {
            LOGE("%s Error %d setting device config\n", __func__, rc);

            goto done;
        }

        rc = enable_voice_context(service, session);
        if (rc < 0) {
            LOGE("%s Error %d enabling voice context\n", __func__, rc);

            goto done;
        }

        rc = start_voice_manager(service, session);
        if (rc < 0) {
            LOGE("%s Error %d  start voice manager\n", __func__, rc);

            goto disable_vc;
        }

	session->state = SESSION_STARTED;
        goto done;

    } else {
        LOGE("%s: Invalid session state %d", __func__, session->state);

        rc = -1;
        goto done;
    }

disable_vc:
    disable_voice_context(service, session);

done:
    pthread_mutex_unlock(&service->csd_service_mutex);

    return rc;
}

int csd_client_enable_device_config(int rx_dev_id, int tx_dev_id)
{
    int rc = 0;
    int i, j;
    int remote_rx_dev_id, remote_tx_dev_id, ec_dev_id;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;

    service = &csd_client.csd_services[CSD_SERVICE_INSTANCE1];
    pthread_mutex_lock(&service->csd_service_mutex);

    if (service->csd_version >= CSD_VER_ID_WITH_OPTIMIZATION) {
        remote_rx_dev_id = acdb_loader_get_remote_acdb_id(rx_dev_id);
        remote_tx_dev_id = acdb_loader_get_remote_acdb_id(tx_dev_id);
        /* Use the remote device ID if it exists else use the APQ device ID */
        if (remote_rx_dev_id <= 0)
            remote_rx_dev_id = rx_dev_id;
        if (remote_tx_dev_id <= 0)
            remote_tx_dev_id = tx_dev_id;

        /* External EC reference device type is based on APQ Rx device type */
        ec_dev_id = get_external_ec_device_id(rx_dev_id);
        set_dev_ids(remote_rx_dev_id, remote_tx_dev_id, ec_dev_id);

        LOGD("%s: rx_dev:%d tx_dev:%d \n",
             __func__, rx_dev_id, tx_dev_id );

        for (j = 0; j < ALL_SESSION_ID; j++) {
            session = &service->sessions[j];

            if (session->state == SESSION_STARTED) {

                rc = set_device_config(service, session,
                                       remote_rx_dev_id, remote_tx_dev_id);
                if (rc < 0) {
                    LOGE("%s: Error %d setting device config\n",
                         __func__, rc);

                    continue;
                }

                rc = enable_voice_context(service, session);
                if (rc < 0) {
                    LOGE("%s: Error %d enabling context\n", __func__, rc);

                    continue;
                }
            } else {
                LOGD("%s: no action needed session_state = %d\n",
                     __func__, session->state);
            }
        }
    }
    pthread_mutex_unlock(&service->csd_service_mutex);

    return rc;
}

int csd_client_enable_device(int rx_dev_id, int tx_dev_id, uint32_t flags)
{
    int rc = 0;
    int i, j;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    int ec_dev_id = 0;

    LOGD("%s: rx_dev:%d tx_dev:%d flags %x \n",
         __func__, rx_dev_id, tx_dev_id, flags);

    if (is_i2s_ext_modem) {
       /* for I2S based external modem , only supported 0 and 1 device IDs */
       rx_dev_id = 1;
       tx_dev_id = 0;

       LOGD("%s: I2S based modem, rx %d, tx %d\n",__func__,
            rx_dev_id, tx_dev_id);
    }

    /* External EC reference device type is based on APQ Rx device type */
    ec_dev_id = get_external_ec_device_id(rx_dev_id);
    set_dev_ids(rx_dev_id, tx_dev_id, ec_dev_id);
    set_tty_mode(flags & 0x000000F0);
    set_aanc_mode(flags & ANC_FLAG);

    service = &csd_client.csd_services[CSD_SERVICE_INSTANCE1];
    pthread_mutex_lock(&service->csd_service_mutex);

    for (j = 0; j < ALL_SESSION_ID; j++) {
         session = &service->sessions[j];

         if (session->state == SESSION_STARTED ||
             session->state == SESSION_STANDBY) {
             /* Enable the device only if Voice session starts */
             rc = csd_enable_devices(service);
             if (rc < 0) {
                 LOGE("%s: Error %d enabling devices\n",
                       __func__,rc);
                 rc = -1;
                 goto done;
             }
         }

         if (service->csd_version < CSD_VER_ID_WITH_OPTIMIZATION) {
             if (session->state == SESSION_STARTED) {
                 rc = set_device_config(service, session,
                                        rx_dev_id, tx_dev_id);
                 if (rc < 0) {
                     LOGE("%s: Error %d setting device config\n",
                          __func__, rc);

                     continue;
                 }

                 rc = enable_voice_context(service, session);
                 if (rc < 0) {
                     LOGE("%s: Error %d enabling context\n", __func__, rc);

                     continue;
                 }

                 rc = attach_voice_context(service, session);
                 if (rc < 0) {
                     LOGE("%s: Error %d enabling context\n", __func__, rc);

                     disable_voice_context(service, session);
                     continue;
                 }
             } else {
                 LOGD("%s: no action needed session_state = %d\n",
                      __func__, session->state);
             }
        }
    }

done:
    pthread_mutex_unlock(&service->csd_service_mutex);
    return rc;
}

int csd_client_disable_device(void)
{
    int rc = 0;
    int i, j;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    int rx_dev_id = 0;
    int tx_dev_id = 0;
    int ec_dev_id = 0;

    service = &csd_client.csd_services[CSD_SERVICE_INSTANCE1];
    pthread_mutex_lock(&service->csd_service_mutex);

    if (service->csd_version >= CSD_VER_ID_WITH_OPTIMIZATION) {
        rc = csd_disable_devices(service);
        if (rc < 0) {
            LOGE("%s: Error %d disabling devices \n", __func__, rc);
        }

        /* Disable VC for all sessions */
        for (j = 0; j < ALL_SESSION_ID; j++) {
            session = &service->sessions[j];
            if (session->state == SESSION_STARTED) {
                rc = disable_voice_context(service, session);
                if (rc < 0)
                    LOGE("%s: Error %d disable context\n", __func__, rc);
            }
        }
    } else {
        /* Detach and disable VC for all sessions */
        for (j = 0; j < ALL_SESSION_ID; j++) {
            session = &service->sessions[j];

            if (session->state == SESSION_STARTED) {
                rc = detach_voice_context(service, session);
                if (rc < 0)
                    LOGE("%s: Error %d detach context\n", __func__, rc);

                rc = disable_voice_context(service, session);
                if (rc < 0)
                    LOGE("%s: Error %d disable context\n", __func__, rc);
            }
        }

        rc = csd_disable_devices(service);
        if (rc < 0) {
            LOGE("%s: Error %d disabling devices \n", __func__, rc);
        }
    }
    pthread_mutex_unlock(&service->csd_service_mutex);

    return rc;
}

int csd_client_start_record(uint32_t vsid, int rec_mode)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    int incall_dev = 0;
    struct csd_iterator it;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    iterator_init(&it, sessionid);
    while (iterator_has_next(&it)) {
        iterator_next(&it, &service, &session);

        if (service && session) {
            pthread_mutex_lock(&service->csd_service_mutex);
            session->rec_info.rec_enable = 1;
            session->rec_info.rec_mode = rec_mode;

            if (session->state == SESSION_STARTED &&
                !session->rec_info.recording) {

                /* Assumption is here that only one session will be active
                 * amongst all csd services.
                 */

                /* Enable device */
                if (rec_mode == INCALL_REC_UPLINK_AND_DOWNLINK) {
                    incall_dev = DEVICE_INCALL_VOICE_RECORD_STEREO_ACDB_ID;
                } else {
                    incall_dev = DEVICE_INCALL_VOICE_RECORD_MONO_ACDB_ID;
                }

                rc = enable_device(service, incall_dev, -1);
                if (rc < 0) {
                    LOGE("%s: Error %d enabling device = %d\n",
                         __func__, rc, incall_dev);

                    pthread_mutex_unlock(&service->csd_service_mutex);
                    continue;
                }

                 /* Start record */
                 rc = start_record(service, session, rec_mode);
                 if (rc < 0) {
                     LOGE("%s: Error %d start record\n", __func__, rc);

                     disable_device(service, incall_dev, -1);
                 }
            }
            pthread_mutex_unlock(&service->csd_service_mutex);
        } else {
            LOGE("%s: service = %p or session = %p is Null\n",
                 __func__, service, session);

            rc = -1;
            goto done;
        }
    }

done:
    return rc;
}

int csd_client_stop_record(uint32_t vsid)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    int incall_dev = 0;
    struct csd_iterator it;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    iterator_init(&it, sessionid);
    while (iterator_has_next(&it)) {
        iterator_next(&it, &service, &session);

        if (service && session) {
            pthread_mutex_lock(&service->csd_service_mutex);

            if (session->state == SESSION_STARTED &&
                session->rec_info.recording) {
                /* Stop record */
                 rc = stop_record(service, session);
                 if (rc < 0) {
                     LOGE("%s: Error %d stop record\n", __func__, rc);
                 }

                 /* Disable device */
                 if (session->rec_info.rec_mode == INCALL_REC_UPLINK_AND_DOWNLINK) {
                     incall_dev = DEVICE_INCALL_VOICE_RECORD_STEREO_ACDB_ID;
                 } else {
                     incall_dev = DEVICE_INCALL_VOICE_RECORD_MONO_ACDB_ID;
                 }

                 rc = disable_device(service, incall_dev, -1);
                 if (rc < 0)
                     LOGE("%s: Error %d disable device\n", __func__, rc);

            }
            session->rec_info.rec_enable = 0;
            session->rec_info.rec_mode = INCALL_REC_NONE;
            pthread_mutex_unlock(&service->csd_service_mutex);
        } else {
            LOGE("%s: service = %p or session = %p is Null\n",
                 __func__, service, session);

            rc = -1;
            goto done;
        }
    }

done:
    return rc;
}

int csd_client_start_playback(uint32_t vsid)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    int incall_dev = 0;
    struct csd_iterator it;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    iterator_init(&it, sessionid);
    while (iterator_has_next(&it)) {
        iterator_next(&it, &service, &session);

        if (service && session) {
            pthread_mutex_lock(&service->csd_service_mutex);
            session->play_info.play_enable = 1;
            if ((session->state == SESSION_STARTED) &&
                (!session->play_info.playing)) {

                /* Assumption is here that only one session will be active
                 * amongst all csd services.
                 */

                /* Enable device */
                rc = enable_device(service,
                                   DEVICE_INCALL_MUSIC_DELIVERY_MONO_ACDB_ID,
                                   -1);
                if (rc < 0) {
                    LOGE("%s: Error %d enabling device\n", __func__, rc);

                    pthread_mutex_unlock(&service->csd_service_mutex);
                    continue;
                }

                /* Start playback */
                rc = start_playback(service, session);
                if (rc < 0) {
                    LOGE("%s: Error %d start playback\n", __func__, rc);

                    disable_device(service,
                                   DEVICE_INCALL_MUSIC_DELIVERY_MONO_ACDB_ID,
                                   -1);
                }
            }
            pthread_mutex_unlock(&service->csd_service_mutex);
        } else {
            LOGE("%s: service = %p or session = %p is Null\n",
                 __func__, service, session);

            rc = -1;
            goto done;
        }
    }

done:
    return rc;
}

int csd_client_stop_playback(uint32_t vsid)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    int incall_dev = 0;
    struct csd_iterator it;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    iterator_init(&it, sessionid);
    while (iterator_has_next(&it)) {
        iterator_next(&it, &service, &session);

        if (service && session) {
            pthread_mutex_lock(&service->csd_service_mutex);
            session->play_info.play_enable = 0;
            if ((session->state == SESSION_STARTED) &&
                (session->play_info.playing)) {

                /* Stop playback */
                rc = stop_playback(service, session);
                if (rc < 0)
                    LOGE("%s: Error %d stop record\n", __func__, rc);

                /* Disable device */
                rc = disable_device(service,
                                    DEVICE_INCALL_MUSIC_DELIVERY_MONO_ACDB_ID,
                                    -1);
                if (rc < 0)
                    LOGE("%s: Error %d disable device\n", __func__, rc);
            }
            pthread_mutex_unlock(&service->csd_service_mutex);
        } else {
            LOGE("%s: service = %p or session = %p is Null\n",
                 __func__, service, session);

            rc = -1;
            goto done;
        }
    }

done:
    return rc;
}

int csd_client_volume(uint32_t vsid, int volume)
{
    int rc = 0;
    int vol = 100 - volume;
    int volume_index = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    struct csd_iterator it;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    /* Convert the volume percentage to index
     *  100 -> 5, 80-> 4, 60 -> 3, 40 -> 2, 20 -> 1, 0 -> 0
     */
    volume_index = (int)percent_to_index(vol, MIN_VOL_INDEX,
                                              MAX_VOL_INDEX);
    LOGV("%s: volume in percentage:%d,volume in index =%d\n",
         __func__, volume, volume_index);

    iterator_init(&it, sessionid);
    while (iterator_has_next(&it)) {
        iterator_next(&it, &service, &session);

        if (service && session) {
            pthread_mutex_lock(&service->csd_service_mutex);
            session->volume = volume_index;
            if (session->state == SESSION_STARTED ||
                session->state == SESSION_STANDBY) {
                rc = set_rx_volume_index(service, session, volume_index);
                if (rc < 0) {
                    LOGE("%s: Error %d setting volume on session %s\n",
                         __func__, rc, session->name);
                }

                LOGD("%s: volume %d, state %d, rc %d\n",
                     __func__, volume_index, session->state, rc);
            } else {
                LOGD("%s: Invalid session state %d\n",
                     __func__, session->state);
            }
            pthread_mutex_unlock(&service->csd_service_mutex);
        } else {
            LOGE("%s: service = %p or session = %p is Null\n",
                 __func__, service, session);

            rc = -1;
            goto done;
        }
    }

done:
    return rc;

}

int csd_client_mic_mute(uint32_t vsid, int mute)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    struct csd_iterator it;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    iterator_init(&it, sessionid);
    while (iterator_has_next(&it)) {
        iterator_next(&it, &service, &session);

        if (service && session) {
            pthread_mutex_lock(&service->csd_service_mutex);
            session->mute = mute;
            if (session->state == SESSION_STARTED ||
                session->state == SESSION_STANDBY) {
                    rc = set_stream_mute(service, session, mute);
                    if (rc < 0) {
                        LOGE("%s: Error %d setting mic mute on session %s\n",
                             __func__, rc, session->name);
                    }

                    LOGD("%s: mute %d, state %d, rc %d\n",
                         __func__, mute, session->state, rc);
            } else {
                LOGD("%s: Invalid session state %d\n",
                     __func__, session->state);
            }
            pthread_mutex_unlock(&service->csd_service_mutex);
        } else {
            LOGE("%s: service = %p or session = %p is Null\n",
                 __func__, service, session);

            rc = -1;
            goto done;
        }
    }

done:
    return rc;
}

int csd_client_set_rx_mute(uint32_t vsid, int mute)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    struct csd_iterator it;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    iterator_init(&it, sessionid);
    while (iterator_has_next(&it)) {
        iterator_next(&it, &service, &session);

        if (service && session) {
            pthread_mutex_lock(&service->csd_service_mutex);
            if (session->state == SESSION_STARTED ||
                session->state == SESSION_STANDBY) {
                    rc = set_rx_device_mute(service, session, mute);
                    if (rc < 0) {
                        LOGE("%s: Error %d setting rx mute on session %s\n",
                             __func__, rc, session->name);
                    }
                    LOGD("%s: mute %d, state %d, rc %d\n",
                         __func__, mute, session->state, rc);
            } else {
                LOGD("%s: Invalid session state %d\n",
                     __func__, session->state);
            }
            pthread_mutex_unlock(&service->csd_service_mutex);
        } else {
            LOGE("%s: service = %p or session = %p is Null\n",
                 __func__, service, session);

            rc = -1;
            goto done;
        }
    }

done:
    return rc;
}

int csd_client_wide_voice(uint32_t vsid, uint8_t flag)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    struct csd_iterator it;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    iterator_init(&it, sessionid);
    while (iterator_has_next(&it)) {
        iterator_next(&it, &service, &session);

        if (service && session) {
            pthread_mutex_lock(&service->csd_service_mutex);
            if (session->state == SESSION_CREATED ||
                session->state == SESSION_STOPPED) {
                    session->wide_voice = flag;

                    LOGD("%s: flag %d, state %d, rc %d\n",
                         __func__, session->wide_voice, session->state, rc);
            } else {
                LOGD("%s: widevoice cannot be set in state %d, rc %d\n",
                __func__, session->state, rc);
            }
            pthread_mutex_unlock(&service->csd_service_mutex);
        } else {
            LOGE("%s: service = %p or session = %p is Null\n",
                 __func__, service, session);

            rc = -1;
            goto done;
        }
    }

done:
    return rc;
}

int csd_client_slow_talk(uint32_t vsid, uint8_t flag)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    struct csd_iterator it;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    iterator_init(&it, sessionid);
    while (iterator_has_next(&it)) {
        iterator_next(&it, &service, &session);

        if (service && session) {
            pthread_mutex_lock(&service->csd_service_mutex);
            session->slow_talk = flag;
            /* ST can be set when voice call is in progress. */
            if (session->state == SESSION_STARTED ||
                 session->state == SESSION_STANDBY) {
                rc = set_ui_property(service, session,
                                     MODULE_ID_VOICE_MODULE_ST, flag);
                if (rc < 0) {
                    LOGE("%s: Error %d setting FENS on session %s\n",
                         __func__, rc, session->name);
                }
            }

            LOGD("%s: flag %d, state %d, rc %d\n",
                 __func__, session->fens, session->state, rc);
            pthread_mutex_unlock(&service->csd_service_mutex);
        } else {
            LOGE("%s: service = %p or session = %p is Null\n",
                 __func__, service, session);

            rc = -1;
            goto done;
        }
    }

done:
    return rc;
}

int csd_client_fens(uint32_t vsid, uint8_t flag)
{
    int rc = 0;
    struct csd_service *service = NULL;
    struct csd_session *session = NULL;
    struct csd_iterator it;
    int sessionid = get_sessionid_by_vsid(vsid);

    if((sessionid < VOICE_SESSION_ID) || (sessionid > ALL_SESSION_ID)) {
        LOGE("%s: Invalid vsid id %d sessionid id =%d\n",
             __func__, vsid, sessionid);

        rc = -1;
        goto done;
    }

    iterator_init(&it, sessionid);
    while (iterator_has_next(&it)) {
        iterator_next(&it, &service, &session);

        if (service && session) {
            pthread_mutex_lock(&service->csd_service_mutex);
            session->fens = flag;
            /* FENS can be set when voice call is in progress. */
            if (session->state == SESSION_STARTED ||
                session->state == SESSION_STANDBY) {
                rc = set_ui_property(service, session,
                                     MODULE_ID_VOICE_MODULE_FENS, flag);
                if (rc < 0) {
                    LOGE("%s: Error %d setting FENS on session %s\n",
                         __func__, rc, session->name);
                }
            }

            LOGD("%s: flag %d, state %d, rc %d\n",
                 __func__, session->fens, session->state, rc);
            pthread_mutex_unlock(&service->csd_service_mutex);
        } else {
            LOGE("%s: service = %p or session = %p is Null\n",
                 __func__, service, session);

            rc = -1;
            goto done;
        }
    }

done:
    return rc;
}

int csd_client_get_sample_rate(int *sample_rate)
{
    int rc = 0;
    int size = 0, n, rv;
    struct addrinfo hints, *serv_info;
    char send_payload;
    char recv_buffer[64];
    int recv_bytes = 0;
    int retries = 0;
    struct qmi_ril_speech_codec_info_msg speech_info;

    LOGV("%s: connection_state %d\n", __func__, socket_connection_state);

    if (!socket_connection_state) {
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        rc = getaddrinfo(NULL, "5001", &hints, &serv_info);
        if (rc != 0) {
            LOGE("%s: getaddrinfo fail, rc %d\n", __func__, rc);
            return -1;
        }

        sockid = socket(serv_info->ai_family, serv_info->ai_socktype,
                        serv_info->ai_protocol);
        if (sockid == -1) {
            LOGE("%s: socket fail, sockid %d\n", __func__, sockid);

            freeaddrinfo(serv_info);
            return -1;
        }

        while ((connect(sockid, serv_info->ai_addr, serv_info->ai_addrlen) == -1)) {
            retries++;
            LOGV("%s: retries %d\n", __func__, retries);

            usleep(10000);
            if(retries == MAX_ATTEMPTS) {
                freeaddrinfo(serv_info);
                close(sockid);
                LOGE("%s: connect failed till max tries reached\n", __func__);

                return -1;
            }
        }
        freeaddrinfo(serv_info);
        socket_connection_state = 1;
    }

    memset(recv_buffer, 0, sizeof(recv_buffer));
    send_payload = QMI_RIL_SPEECH_CODEC_INFO_MESSAGE;
    rc = send(sockid, &send_payload, sizeof(send_payload), 0);
    if (rc == -1) {
        LOGE("%s: send: server has restarted, rc %d\n", __func__, rc);

        close(sockid);
        socket_connection_state = 0;
        return -1;
    }
    LOGV("%s: send request(%x) to RIL\n", __func__, send_payload);

    recv_bytes = recv(sockid, recv_buffer, sizeof(recv_buffer) , 0);
    if (recv_bytes <= 0) {
        LOGV("%s: recv: server has restarted, recv_bytes %d\n",
             __func__, recv_bytes);

        close(sockid);
        socket_connection_state = 0;
        return -1;
    }
    LOGD("%s: recv_bytes %d\n", __func__, recv_bytes);

    if (recv_bytes != 0) {
        speech_info.msg_id = recv_buffer[0];
        memcpy((char*)&speech_info.msg_payload_len, &recv_buffer[1], 2);
        speech_info.network_mode = recv_buffer[3];
        speech_info.speech_codec = recv_buffer[4];
    }

    LOGD("speech_info: payload_len %d, msg_id %x\n",
         speech_info.msg_payload_len, speech_info.msg_id);
    LOGD("speech_info: network_mode %d, speech_codec %d\n",
         speech_info.network_mode, speech_info.speech_codec);

    if ((speech_info.msg_id == QMI_RIL_SPEECH_CODEC_INFO_MESSAGE)
         && (!speech_info.msg_payload_len)) {
         if ((speech_info.speech_codec == EVRC_WB) ||
             (speech_info.speech_codec == EVRC_NW) ||
             (speech_info.speech_codec == AMR_WB)) {
                *sample_rate = SAMPLE_RATE_16KHZ;
         } else
                *sample_rate = SAMPLE_RATE_8KHZ;

         LOGD("%s: sample_rate %d\n", __func__, *sample_rate);
    }
    i2s_sample_rate = *sample_rate;

    return 0;
}

int csd_client_init(void)
{
    int rc = 0;

    memset(&csd_client, 0, sizeof(struct csd_client));

    is_i2s_ext_modem = platform_is_i2s_ext_modem();
    initialize_client_data();

    rc = csd_create_thread(CSD_SERVICE_INSTANCE1);
    if (rc < 0)
        LOGE("%s: Error %d creating thread of CSD_SERVICE_INSTANCE1",
             __func__, rc);

    csd_client.slim_fd = open("/dev/audio_slimslave", O_RDWR);
    if(csd_client.slim_fd < 0) {
        LOGE("%s: Failed to open /dev/audio_slimslave device", __func__);

        rc = csd_client.slim_fd;
    }

    return rc;
}

int csd_client_deinit(void)
{
    int rc = 0;

    rc = csd_join_csd_thread(CSD_SERVICE_INSTANCE1);
    if (rc < 0)
        LOGE("%s: Error %d closing thread of CSD_SERVICE_INSTANCE1",
             __func__, rc);

    if (csd_client.slim_fd >= 0) {
        if(close(csd_client.slim_fd) < 0)
            LOGE("%s: Failed to close /dev/audio_slimslave device", __func__);

        csd_client.slim_fd = -1;
        rc = -1;
    }

    return rc;
}
