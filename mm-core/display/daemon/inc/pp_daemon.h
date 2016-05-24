/*
 * Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef _PP_DAEMON_H
#define _PP_DAEMON_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cutils/properties.h>
#include <cutils/log.h>

#include "common_log.h"
#include <cutils/sockets.h>
#include <linux/msm_mdp.h>
#include <linux/fb.h>

#include "lib-postproc.h"
#include "abl_oem.h"
#include "abl_driver.h"
#include "aba_core_api.h"
#include "aba_cabl.h"
#include "parser.h"
#include "mdp_version.h"
#include "als.h"
#include "DummySensor.h"
#include "Calib.h"
#include "NativeLightSensor.h"

#ifdef ALS_ENABLE
#include "LightSensor.h"
#endif

#ifndef LOGE_IF
#define LOGE_IF(cond, ...) \
    ( (CONDITION(cond)) ? ((void)LOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) : (void)0 )
#endif
#ifndef LOGD_IF
#define LOGD_IF(a...) while(0) {}
#endif

#define MAX_FB_NAME_LEN 128
#define TOTAL_FB_NUM 3
#define PRIMARY_PANEL_TYPE_CNT 5
#define EXTERNAL_PANEL_TYPE_CNT 1
#define WRITEBACK_PANEL_TYPE_CNT 1
#define HDMI_PANEL "hdmi panel"
#define LVDS_PANEL "lvds panel"
#define DTV_PANEL "dtv panel"
#define MIPI_DSI_VIDEO_PANEL "mipi dsi video panel"
#define MIPI_DSI_CMD_PANEL "mipi dsi cmd panel"
#define WB_PANEL "writeback panel"
#define EDP_PANEL "edp panel"
#define AD_FILE_LEN 2

const char* SYS_BRIGHTNESS = "/sys/class/leds/lcd-backlight/brightness";
const char* SYS_BRIGHTNESS_ALT = "/sys/devices/virtual/graphics/fb0/msmfb_bl0/brightness";

#ifdef _ANDROID_
const char * FRAMEBUFFER_NODE = "/dev/graphics/fb";
#else
const char * FRAMEBUFFER_NODE = "/dev/fb";
#endif

#define MAX_BACKLIGHT_LEN 12
#define LEVEL_CHANGE_SLEEP_DURATION 5
#define MAX_DBG_MSG_LEN 6000

#define DAEMON_SOCKET "pps"
#define CONN_TIMEOUT 3600 /* (60*60) secs = 1 hour */

#define MAX_CMD_LEN 4096
#define SHORT_CMD_LEN 64

#define CMD_DEBUG_PREFIX "debug:"
#define CMD_DEBUG_CABL_ON "debug:cabl:on"
#define CMD_DEBUG_CABL_OFF "debug:cabl:off"
#define CMD_DEBUG_AD_ON "debug:ad:on"
#define CMD_DEBUG_AD_OFF "debug:ad:off"
#define CMD_DEBUG_PP_ON "debug:pp:on"
#define CMD_DEBUG_PP_OFF "debug:pp:off"
#define CMD_DEBUG_DAEMON_ON "debug:daemon:on"
#define CMD_DEBUG_DAEMON_OFF "debug:daemon:off"

#define CMD_CABL_PREFIX "cabl:"
#define CMD_CABL_ON "cabl:on"
#define CMD_CABL_OFF "cabl:off"
#define CMD_CABL_SET "cabl:set"
#define CMD_CABL_STATUS "cabl:status"
#define YUV_INPUT_STATE_PROP "hw.cabl.yuv"
#define AD_DEFAULT_MODE_PROP "ro.qcom.ad.default.mode"
#define AD_CALIB_DATA_PATH_PROP "ro.qcom.ad.calib.data"
#define AD_SENSOR_PROP "ro.qcom.ad.sensortype"

#define CMD_AD_PREFIX "ad:"
#define CMD_AD_ON "ad:on"
#define CMD_AD_OFF "ad:off"
#define CMD_AD_STATUS "ad:query:status"
#define CMD_AD_CALIB_ON "ad:calib:on"
#define CMD_AD_CALIB_OFF "ad:calib:off"
#define CMD_AD_INIT "ad:init"
#define CMD_AD_CFG "ad:config"
#define CMD_AD_INPUT "ad:input"
#define CMD_AD_ASSERTIVENESS "ad:assertiveness"
#define CMD_AD_STRLIMIT "ad:strlim"
#define CMD_BL_SET "bl:set"

#define CALIB_INIT 0x1
#define CALIB_CFG 0x2
#define AD_PARAM_SEPARATOR ";"
#define AD_DATA_SEPARATOR ","
#define AD_INIT_PARAM_CNT 19
#define AD_CFG_PARAM_CNT 11
#define AD_MAX_DATA_CNT 256
#define BL_LIN_LUT_SIZE 256
#define BL_ATT_ALPHA_BASE 1024
#define ASYM_COL_AL_LUT_SIZE 33
#define START_ALS_VALUE 1000
#define AD_ENABLE_PRI 1
#define AD_ENABLE_WB 2
#define AD_REFRESH_CNT 256
#define AD_REFRESH_INTERVAL 16
#define AD_STRLIMT_MIN 0
#define AD_STRLIMT_MAX 255

#define SVI_ALS_RATIO_THR 0.05
#define SVI_ALS_LIN_THR 35.0

#define CALIB_READY(x) (x == (CALIB_INIT | CALIB_CFG))

#define CMD_SVI_PREFIX "svi:"
#define CMD_SVI_ON "svi:on"
#define CMD_SVI_OFF "svi:off"
#define SVI_SENSOR_PROP "ro.qcom.svi.sensortype"

#define BL_LUT_SIZE  256
#define HIST_PROC_FACTOR 8

#define CMD_PP_ON "pp:on"
#define CMD_PP_OFF "pp:off"
#define CMD_PP_SET_HSIC "pp:set:hsic"
#define CMD_POSTPROC_STATUS "pp:query:status:postproc"
#define PP_CFG_FILE_PROP "hw.pp.cfg"
#define PP_CFG_FILE_PATH "/data/pp_data.cfg"

#define CMD_OEM_PREFIX "oem:"
#define CMD_OEM_GET_PROFILES "oem:get:profile"
#define CMD_OEM_SET_PROFILE "oem:set:profile"

#define CONTROL_DISABLE 0x0
#define CONTROL_ENABLE 0x1
#define CONTROL_PAUSE 0x2
#define CONTROL_RESUME 0x3

#define OP_INDEX 0
#define DISPLAY_INDEX 1
#define FEATURE_INDEX 2
#define FLAG_INDEX 3
#define DATA_INDEX 4

#define CMD_GET "get"
#define CMD_SET "set"

#define PRIMARY_DISPLAY 0
#define SECONDARY_DISPLAY 1
#define WIFI_DISPLAY 2

#define FEATURE_PCC "pcc"
#define MIN_PCC_PARAMS_REQUIRED 37
#define TOKEN_PARAMS_DELIM ":;"

#define CMD_DCM_ON "dcm:on"
#define CMD_DCM_OFF "dcm:off"

#define DEFAULT_HUE 0
#define DEFAULT_SAT 0
#define DEFAULT_INT 0
#define DEFAULT_CON 0

#define REFRESHER_STATE_ENABLE 0x1
#define REFRESHER_STATE_CONFIG 0x2

#define HUE_RANGE 180
#define BRIGHTNESS_RANGE 255
#define CON_SAT_RANGE 1.0f
#define CAP_RANGE(value,max,min) do { if (value - min < -0.0001)\
    {value = min;}\
    else if(value - max > 0.0001)\
    {value = max;}\
} while(0);

int ad_init_data_cnt[AD_INIT_PARAM_CNT] = {33, 33, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 256, 256};
int ad_cfg_data_cnt[AD_CFG_PARAM_CNT] = {1, 33, 1, 1, 1, 1, 2, 4, 1, 1, 1};

enum ad_status {
    AD_OFF,
    AD_ON,
    AD_CALIB_OFF,
    AD_CALIB_ON
};

enum ad_mode {
    ad_mode_auto_bl,
    ad_mode_auto_str,
    ad_mode_calib
};

struct ad_default_params {
    struct mdss_ad_init init;
    struct mdss_ad_cfg cfg;
};

enum ctrl_status {
    cabl_bit = 0x01,
    ad_bit = 0x02,
    ctrl_bit = 0x04,
    svi_bit = 0x08,
};

enum cabl_status {
    CABL_OFF = false,
    CABL_ON  = true
};

enum pp_status {
    PP_OFF = false,
    PP_ON  = true
};

enum alsupdate_status {
    ALS_UPDATE_OFF = false,
    ALS_UPDATE_ON = true
};

enum require_signal {
    WAIT_SIGNAL,
    NO_WAIT_SIGNAL
};

enum panel_mode {
    MIPI_DSI_VIDEO = '8',
    MIPI_DSI_CMD = '9',
};

union display_pp_cfg {
    struct display_pp_pa_cfg pa_cfg;
    struct display_pp_conv_cfg conv_cfg;
};

class ScreenRefresher {
    uint32_t mState;
    uint32_t mFrames;
    uint32_t mMS;
    uint32_t mRefCount;
    pthread_t mThread;
    pthread_mutex_t mLock;
    pthread_cond_t  mWaitCond;
    void ProcessRefreshWork();
    static void *refresher_thrd_func(void *obj) {
        reinterpret_cast<ScreenRefresher *>(obj)->ProcessRefreshWork();
        return NULL;
    }
public:
    bool mDebug;
    ScreenRefresher(): mState(0), mFrames(0), mMS(0), mRefCount(0), mDebug(false){
        pthread_mutex_init(&mLock, NULL);
        pthread_cond_init(&mWaitCond, NULL);
        /* set the flags based on property */
        char property[PROPERTY_VALUE_MAX];
        if (property_get("debug.refresh.logs", property, 0) > 0 && (atoi(property) > 0 )) {
            mDebug = true;
        }
    }
    ~ScreenRefresher() {
        pthread_mutex_destroy(&mLock);
        pthread_cond_destroy(&mWaitCond);
    }

    int Control(bool bEnable);
    int Refresh(uint32_t nFrames, uint32_t nMS);
    int Notify(int notification_type);
};

class CABL {
public:
    bool mEnable;
    bool mDebug;
    volatile cabl_status eStatus;
    bool mHistStatus;
    bool mPowerLevelChange;
    int32_t mPowerSaveLevel;
    int32_t mUserBLLevel;
    struct fb_cmap mColorMap;
    struct hw_info_hist mHistInfo;
    bl_oem_api mOEMParams;
    pthread_t mControlThreadID;
    pthread_t mWorkerThreadID;
    pthread_mutex_t mCABLCtrlLock;
    pthread_mutex_t mCABLLock;
    pthread_cond_t  mCABLCond;
    char mPanelMode;
    ScreenRefresher *mRefresher;
    int32_t auto_adjust_quality_lvl();
    void initHW();
    CABL() : mEnable(false), mDebug(false), eStatus(CABL_OFF), mHistStatus(0),
            mPowerLevelChange(false), mPowerSaveLevel(ABL_QUALITY_HIGH),
            mRefresher(NULL) {
        /* initialize the cabl thread sync variables */
        pthread_mutex_init(&mCABLCtrlLock, NULL);
        pthread_mutex_init(&mCABLLock, NULL);
        pthread_cond_init(&mCABLCond, NULL);
        /* set the flags based on property */
        char property[PROPERTY_VALUE_MAX];
        if (property_get("ro.qualcomm.cabl", property, NULL) > 0 && (atoi(property) == 1)) {
            mEnable = true;
        }
        if (property_get("debug.cabl.logs", property, 0) > 0 && (atoi(property) > 0 )) {
            mDebug = true;
        }
    }
    ~CABL() {
        pthread_mutex_destroy(&mCABLCtrlLock);
        pthread_mutex_destroy(&mCABLLock);
        pthread_cond_destroy(&mCABLCond);
    }
    int32_t start_cabl();
    void stop_cabl();
    int Notify(int notification_type);
};

class PostProc {
public:
    bool mEnable;
    bool mDebug;
    bool mStatus;
    bool mPoll;
    uint32_t mBlockType;
    struct display_hw_info mHWInfo;
    display_pp_cfg mCurCfg;
    display_pp_cfg mNewCfg;
    PostProc() : mEnable(false), mDebug(false), mStatus(PP_OFF), mPoll(false) {
        char property[PROPERTY_VALUE_MAX];
        if (property_get("debug.pp.logs", property, 0) > 0 && (atoi(property) == 1)) {
            mDebug = true;
        }
        if (property_get("debug.csc.poll", property, 0) > 0 && (atoi(property) == 1)) {
            mPoll = true;
        }
    }
    int get_saved_hsic_config();
    void print_values(const char *, display_pp_cfg *);
    void copy_config();
    void init_cc_matrix();
    int32_t compare_config();
    int32_t start_pp();
    int32_t stop_pp();
    int32_t set_hsic(int, float, int, float);
    int write_hsic_values();
    int parse_pa_data(int* hue, float* sat, int* intensity, float* contrast);
    int save_pa_data(int hue, float sat, int intensity, float contrast);
    int processPPDataFile();
};

class DaemonContext;

class AD {
    int mDisplayFd;
    int mEnable;
    int mADUpdateType;
    int mStatus;
    int mMode;
    bool mADSupported;
    int mALSValue;
    int mLastSentALSValue;
    int mPrevALSValue;
    int mPrimaryPanelMode;
    bool bIsFirstRun;
    ALS *mLightSensor;
    pthread_t mADThread;
    pthread_mutex_t mADLock;
    pthread_cond_t  mADCond;

    uint32_t mBLLinLUT[BL_LIN_LUT_SIZE];
    uint32_t mBLLinInvLUT[BL_LIN_LUT_SIZE];
    uint32_t mBLAttLUT[AD_BL_ATT_LUT_LEN];
    struct ad_default_params mDefADConfig;
    uint32_t mADCalibStatus;

    int ADParseCalibData(struct ad_default_params* params);
    int ADPrintCalibData(struct ad_default_params* params);
    void ProcessADWork();
    static void *ad_thrd_func(void *obj) {
        reinterpret_cast<AD *>(obj)->ProcessADWork();
        return NULL;
    }

    int ADRun(ad_mode mode);
    int ADCleanup();
public:
    ScreenRefresher *mRefresher;
    bool mDebug;
    int mAssertivenessSliderValue; // value between 0 - 255
    uint32_t mFlags;
    uint32_t mLastManualInput;
    int mPrevBL;
    uint16_t mAD_calib_a;
    uint16_t mAD_calib_c;
    uint16_t mAD_calib_d;
    int isADEnabled(){
        return mEnable;
    }

    int ADStatus(){
        return mStatus;
    }

    int ADMode(){
        return mMode;
    }

    int ADCurrALSValue(){
        return mALSValue;
    }

    int ADLastSentALSValue(){
        return mLastSentALSValue;
    }

    uint8_t ADGetStrLimit(){
        return mDefADConfig.cfg.strength_limit;
    }

    void ADSetStrLimit(uint8_t strLimit){
        mDefADConfig.cfg.strength_limit = strLimit;
    }
    AD() : mDisplayFd(-1), mEnable(false), mADUpdateType(0), mStatus(false), mMode(-1),
        mADSupported(false), mALSValue(0), mLastSentALSValue(-1), mPrevALSValue(0),
        mPrimaryPanelMode(MIPI_DSI_VIDEO), bIsFirstRun(true), mLightSensor(NULL),
        mRefresher(NULL), mDebug(false), mFlags(0) {
        mAssertivenessSliderValue = -1;
        mPrevBL = -1;
        mAD_calib_a = 0;
        mAD_calib_c = 0;
        mAD_calib_d = 0;
        int mdp_version = qdutils::MDPVersion::getInstance().getMDPVersion();
        mEnable = display_pp_ad_supported();
        if (mEnable && mdp_version == qdutils::MDSS_V5) {
            mADSupported = true;
        } else
            mADSupported = false;

        if (mADSupported) {
            char property[PROPERTY_VALUE_MAX];
            if (property_get(AD_SENSOR_PROP, property, 0) > 0) {
                int type = atoi(property);
                if (type == 0) {
                    mLightSensor  = new ALS();
                } else if (type == 1) {
                    mLightSensor = new DummySensor();
                } else if (type == 2) {
                    mLightSensor = new NativeLightSensor();
#ifdef ALS_ENABLE
                } else if (type == 3) {
                    mLightSensor  = new LightSensor();
#endif
                } else {
                    LOGE("Invalid choice for sensor type, initializing the default sensor class!");
                    mLightSensor  = new ALS();
                }
            } else {
#ifdef ALS_ENABLE
                mLightSensor  = new LightSensor();
#else
                mLightSensor = new NativeLightSensor();
#endif
            }

            mPrimaryPanelMode =  qdutils::MDPVersion::getInstance().getPanelType();
            if(mEnable == AD_ENABLE_PRI) {
                mRefresher = new ScreenRefresher();
            }

            if (property_get("debug.ad.logs", property, 0) > 0 && (atoi(property) == 1)) {
                mDebug = true;
            }

            mDefADConfig.init.bl_lin_len = BL_LIN_LUT_SIZE;
            mDefADConfig.init.bl_att_len = AD_BL_ATT_LUT_LEN;
            mDefADConfig.init.alpha_base = BL_ATT_ALPHA_BASE;
            mDefADConfig.init.bl_lin = &mBLLinLUT[0];
            mDefADConfig.init.bl_lin_inv = &mBLLinInvLUT[0];
            mDefADConfig.init.bl_att_lut = &mBLAttLUT[0];

        }

        pthread_mutex_init(&mADLock, NULL);
        pthread_cond_init(&mADCond, NULL);
    }

    ~AD() {
        if (mADSupported) {
            delete mLightSensor;
        }

        if (mRefresher) {
            delete mRefresher;
            mRefresher = NULL;
        }
        pthread_mutex_destroy(&mADLock);
        pthread_cond_destroy(&mADCond);
    }

    int ADControl(DaemonContext *ctx, int enableBit, ad_mode mode = ad_mode_calib, int display_id = MDP_BLOCK_MAX);
    int ADInit(char* initial);
    int ADConfig(char* config);
    int ADInput(int amlight);
    int ADUpdateAL(int amlight, int refresh_cnt);
    int ADSetCalibMode(int mode);
    int ADSetCalibBL(int bl);
    int ADSetupMode();
    int ADCalcCalib();
    int Notify(int notification_type);
    bool IsADInputValid(ad_mode mode, int display_id) {
        if (mode != ad_mode_auto_str && mode != ad_mode_auto_bl) {
            LOGE("Invalid AD mode!");
            return false;
        }
        if (display_id < MDP_LOGICAL_BLOCK_DISP_0 || display_id > MDP_LOGICAL_BLOCK_DISP_2) {
            LOGE("Invalid AD display option!");
            return false;
        }
        return true;
    }
};

class AbaContext {
    // Aba parameters
    AbaHardwareInfoType mAbaHwInfo;
    bool mDebug;
    bool mWorkerRunning;
    int aba_status;
    pthread_cond_t  mAbaCond;
    pthread_mutex_t mAbaLock;
    pthread_t mWorkerThread;
    require_signal mSignalToWorker;
    ScreenRefresher *pRefresher;
    static void *aba_worker_func(void *obj) {
        reinterpret_cast<AbaContext *>(obj)->ProcessAbaWorker();
        return NULL;
    }
    void FilteredSignal();
    void ProcessAbaWorker();
    void *pHandle;

    // Backlight Params & Functions
    // minimum bl level converted from bl_level_threshold range:0-255
    int is_backlight_modified(int *);
    int32_t  mUserBLLevel;
    uint32_t mBl_lvl;
    uint32_t orig_level;

    // Histogram , LUT & Color-map
    bool mHistStatus;
    fb_cmap mColorMap;
    mdp_histogram_data hist;
    uint32_t *mInputHistogram;
    uint32_t *mOutputHistogram;
    uint32_t *minLUT;

    // CABL specific parameters
    AbaConfigInfoType eCABLConfig;
    bool mCablDebug;
    bool mCablEnable;
    CablInitialConfigType mCablOEMParams;
    CABLQualityLevelType mQualityLevel;
    int32_t auto_adjust_quality_lvl();
    pthread_mutex_t mCABLCtrlLock;

    // SVI specific parameters
    AbaConfigInfoType eSVIConfig;
    bool mSVIDebug;
    bool mSVIEnable;
    bool mSVISupported;
    SVIConfigParametersType mSVIOEMParams;
    void ProcessSVIWork();

    // ALS specific parameters
    alsupdate_status mALSUpdateThreadStatus;
    ALS *mLightSensor;
    bool bALSRunEnabled;
    bool mALSUThreadRunning;
    int mCurrALSValue;
    int mLastALSValue;
    pthread_cond_t  mALSCond;
    pthread_mutex_t mALSUpdateLock;
    pthread_mutex_t mSVICtrlLock;
    pthread_t mALSUpdateThread;
    static void *als_update_func(void *obj) {
        reinterpret_cast<AbaContext *>(obj)->ProcessAbaALSUpdater();
        return NULL;
    }
    void ProcessAbaALSUpdater();
    void SetLightSensor();
    void StartALSUpdaterThread();

public:
    //Aba Parameters
    int Notify(int notification_type);
    void initHW();
    inline void SetDebugLevel(bool t){
        mDebug = t;
    }
    inline void* GetHandle(){
        return pHandle;
    }
    inline void SetABAStatusON(int x){
        aba_status = aba_status | x;
    }
    inline void SetABAStatusOFF(int x){
        aba_status = aba_status & ~x;
    }
    inline bool IsABAStatusON(int x){
        return ( aba_status & x);
    }

    //Cabl Parameters
    int32_t CABLControl(bool);
    inline void SetDefaultQualityMode(uint32_t mode){
        mCablOEMParams.default_ql_mode = mode;
    }
    inline void SetQualityLevel(CABLQualityLevelType q){
        mQualityLevel = q;
    }
    inline CABLQualityLevelType GetQualityLevel(){
        return mQualityLevel;
    }
    inline void SetCABLFeature(){
        eCABLConfig.eFeature = ABA_FEATURE_CABL;
        eCABLConfig.ePanelType = LCD_PANEL;
    }

    //SVI Parameters
    int SVIControl(bool);
    inline void SetSVIFeature(){
        eSVIConfig.eFeature = ABA_FEATURE_SVI;
    }

    //ALS Parameters
    void StopALSUpdaterThread();
    void CleanupLightSensor();

    // ABA constructor
    AbaContext() :  mWorkerRunning(false), aba_status(0), mSignalToWorker(NO_WAIT_SIGNAL),
                pRefresher(NULL), mHistStatus(0), mCablDebug(false),
                mCablEnable(false), mSVIDebug(false), mSVIEnable(false),
                mLightSensor(NULL), bALSRunEnabled(false), mALSUThreadRunning(false),
                mCurrALSValue(0), mLastALSValue(0) {

        char property[PROPERTY_VALUE_MAX];
        int mdp_version = qdutils::MDPVersion::getInstance().getMDPVersion();
        pthread_mutex_init(&mAbaLock, NULL);
        pthread_mutex_init(&mALSUpdateLock, NULL);
        pthread_mutex_init(&mCABLCtrlLock, NULL);
        pthread_mutex_init(&mSVICtrlLock, NULL);
        pthread_cond_init(&mAbaCond, NULL);
        pthread_cond_init(&mALSCond, NULL);

        // SVI Initializations
        mSVISupported = true;
        if (1 == display_pp_svi_supported()) {
            mSVIEnable = true;
            SetLightSensor();
        }
        if (property_get("debug.svi.logs", property, 0) > 0 && (atoi(property) == 1)) {
            LOGE("SVI Debug Enabled");
            mSVIDebug = true;
        }

        //  CABL Initializations
        if (2 == display_pp_cabl_supported()) {
            mCablEnable = true;
        }
        if (property_get("debug.cabl.logs", property, 0) > 0 && (atoi(property) > 0 )) {
            mCablDebug = true;
        }

        mDebug = mSVIDebug || mCablDebug;

        AbaStatusType retval = AbaCreateSession(&pHandle);
        if(retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM || retval == ABA_STATUS_NOT_SUPPORTED) {
            LOGE("Create session failed retval = %d", retval);
        } else {
            LOGE_IF(mDebug, "ABA Create Session Successful");
        }
    }

    ~AbaContext() {
        pthread_mutex_destroy(&mAbaLock);
        pthread_mutex_destroy(&mALSUpdateLock);
        pthread_mutex_destroy(&mCABLCtrlLock);
        pthread_mutex_destroy(&mSVICtrlLock);
        pthread_cond_destroy(&mAbaCond);
        pthread_cond_destroy(&mALSCond);
    }
};

class DaemonContext {
    AbaContext *mABA;
    bool bAbaEnabled;
    pthread_t mControlThrdId;
    int screenStatus;
    int mCtrlStatus;

    int ProcessADMsg (const char* buf);
    int ProcessCABLMsg(char* buf);
    int ProcessSVIMsg(char* buf);
    int ProcessDebugMsg(char* buf);
    int ProcessSetMsg(char* buf);
    int ProcessPCCMsg(char* buf);
    static void *control_thrd_func(void *obj) {
        reinterpret_cast<DaemonContext *>(obj)->ProcessControlWork();
        return NULL;
    }
    void ProcessControlWork();

    static void *ad_poll_thrd_func(void *obj) {
        reinterpret_cast<DaemonContext *>(obj)->ProcessADPollWork();
        return NULL;
    }
    void ProcessADPollWork();
    void StartAlgorithmObjects();

public:
    CABL *mCABL;
    PostProc mPostProc;
    AD mAD;
    DCM *mDCM;
    bool mDebug;
    int32_t mListenFd;
    int32_t mAcceptFd;
    int32_t mNumConnect;
    int32_t nPriPanelType;
    pthread_mutex_t mCtrlLock;
    pthread_mutex_t mCABLOpLock;
    pthread_mutex_t mPostProcOpLock;
    pthread_mutex_t mADOpLock;
    pthread_mutex_t mSVIOpLock;
    pthread_t mADPollThrdId;
    int ad_fd;
    bool mSplitDisplay;
    bool mBootStartCABL;
    int32_t start();
    int32_t getListenFd();
    int32_t ProcessCommand(char *, const int32_t, const int32_t&);
    int32_t reply(bool, const int32_t&);
    int32_t reply(const char *, const int32_t&);
    int SelectFB(int display_id, int* idx);
    bool IsSplitDisplay(int);
    inline AbaContext* getABA(){
        return mABA;
    }
    void StopAlgorithmObjects();
    DaemonContext() : mABA(NULL), bAbaEnabled(false), screenStatus(0),
        mCtrlStatus(0), mCABL(NULL), mDCM(NULL), mDebug(false),
        mListenFd(-1), mAcceptFd(-1), mNumConnect(4), mSplitDisplay(0) {
        char property[PROPERTY_VALUE_MAX];
        if (property_get("debug.listener.logs", property, 0) > 0 && (atoi(property) == 1)) {
            mDebug = true;
        }
        if(2 == display_pp_cabl_supported()){
            bAbaEnabled = true;
        }
        mBootStartCABL = false;
        pthread_mutex_init(&mCtrlLock, NULL);
        pthread_mutex_init(&mCABLOpLock, NULL);
        pthread_mutex_init(&mPostProcOpLock, NULL);
        pthread_mutex_init(&mADOpLock, NULL);
        pthread_mutex_init(&mSVIOpLock, NULL);
        ad_fd = -1;
    }
    ~DaemonContext() {
        pthread_mutex_destroy(&mCtrlLock);
        pthread_mutex_destroy(&mCABLOpLock);
        pthread_mutex_destroy(&mPostProcOpLock);
        pthread_mutex_destroy(&mADOpLock);
        pthread_mutex_destroy(&mSVIOpLock);
    }
};

int tokenize_params(char *inputParams, const char *delim, const int minTokenReq,
                                        char* tokenStr[], int *idx);

#endif /* _PP_DAEMON_H */

