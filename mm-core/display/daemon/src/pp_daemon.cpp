/*
 * DESCRIPTION
 * This file runs the daemon for postprocessing features.
 * It listens to the socket for client connections and controls the features
 * based on commands it received from the clients.
 *
 * Copyright (c) 2012-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 */

#include "pp_daemon.h"
#include <QServiceUtils.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <poll.h>
#include <unistd.h>
#include "Calib.h"

#define LOG_TAG "PPDaemon"
#define LOG_NDDEBUG 0
#define SUCCESS 0
#define FAILED -1
#define HDMI_PRIMARY_NODE  "/sys/class/graphics/fb0/hdmi_primary"
#define BL_SCALE_MAX (1024)

volatile int32_t sigflag = 0;
adp_global featureList;
int FBFd = -1;
int32_t BLFd = -1;

static display_hw_info gHwInfo;
static bool gMDP5 = false;
static int gMDPVersion;

void free_cmap(struct fb_cmap *cmap);

static void inspectHW();
static void free_hist(struct mdp_histogram_data *hist);
static void print_hist(struct mdp_histogram_data *hist);
static void conv3channel(struct fb_cmap *cmap, uint32_t *outmap);
static void initialize_cmap(struct fb_cmap *cmap);
static int read_hist(int32_t fb, struct mdp_histogram_data *hist);
static int32_t startHistogram();
static int32_t stopHistogram();
static int32_t copyHistogram(struct mdp_histogram_data *hist, uint32_t *outhist);
static int32_t get_backlight_level();
static int32_t set_backlight_level(int32_t);
static int32_t outCmap(int32_t fb, uint32_t *outcmap, fb_cmap mColorMap);
static int set_backlight_scale(int32_t bl_scale, uint32_t bl_min_level);
static int ql_string2int(char* c_lvl);


static void inspectHW(){
    gHwInfo.nPriPanelMode =  qdutils::MDPVersion::getInstance().getPanelType();
    gMDPVersion = qdutils::MDPVersion::getInstance().getMDPVersion();
    switch(gMDPVersion) {
        case qdutils::MDP_V3_0:
        case qdutils::MDP_V3_0_3:
        case qdutils::MDP_V3_0_4:
        case qdutils::MDP_V3_1:
        case qdutils::MDP_V4_0:
        case qdutils::MDP_V4_1:
            gHwInfo.nPriDisplayHistBins = 32;
            gHwInfo.nPriDisplayHistComp = 3;
            gHwInfo.nPriDisplayHistBlock = MDP_BLOCK_DMA_P;
            break;
        case qdutils::MDP_V4_2:
        case qdutils::MDP_V4_3:
        case qdutils::MDP_V4_4:
            gHwInfo.nPriDisplayHistBins = 128;
            gHwInfo.nPriDisplayHistComp = 3;
            gHwInfo.nPriDisplayHistBlock = MDP_BLOCK_DMA_P;
            break;
        case qdutils::MDSS_V5:
            gHwInfo.nPriDisplayHistBins = 256;
            gHwInfo.nPriDisplayHistComp = 1;
            gHwInfo.nPriDisplayHistBlock = MDP_LOGICAL_BLOCK_DISP_0;
            gMDP5 = true;
            break;
        case qdutils::MDP_V_UNKNOWN:
        case qdutils::MDP_V2_2:
        default:
            gHwInfo.nPriDisplayHistBins = 0;
            gHwInfo.nPriDisplayHistComp = 0;
            gHwInfo.nPriDisplayHistBlock = 0;
            break;
        }
}

static int32_t set_backlight_level(int32_t backlight) {
    int32_t bytes, ret = -1;
    char buffer[MAX_BACKLIGHT_LEN];
    memset(buffer, 0, MAX_BACKLIGHT_LEN);
    bytes = snprintf(buffer, MAX_BACKLIGHT_LEN, "%d\n", backlight);
    ret = write(BLFd, buffer, bytes);
    lseek(BLFd, 0, SEEK_SET);
    if (ret <= 0) {
        LOGE("Failed to write to BLFd.");
        return ret;
    }
    return 0;
}

static int32_t get_backlight_level(void) {
    int32_t level = -1;
    ssize_t bytes;
    char buffer[MAX_BACKLIGHT_LEN];
    memset(buffer, 0, MAX_BACKLIGHT_LEN);
    bytes = pread(BLFd, buffer, sizeof (char) * MAX_BACKLIGHT_LEN, 0);
    if (bytes > 0) {
        level = atoi(&buffer[0]);
    } else
        LOGE("BL FD read failure: bytes = %d error = %s ", bytes,
            strerror(errno));
    return level;
}

static void free_hist(struct mdp_histogram_data *hist)
{

    if(hist == NULL){
        LOGE("%s: Histogram data NULL", __FUNCTION__);
        return;
    }

    if (hist->c0)
        free(hist->c0);
    if (hist->c1)
        free(hist->c1);
    if (hist->c2)
        free(hist->c2);
    if (hist->extra_info)
        free(hist->extra_info);
}

static void print_hist(struct mdp_histogram_data *hist)
{
    uint32_t *r, *b, *g, i;

    char strbuf[MAX_DBG_MSG_LEN]="";
    size_t strbuf_len = 0;

    if(hist == NULL){
        LOGE("%s: Histogram data NULL", __FUNCTION__);
        return;
    }

    r = hist->c0;
    g = hist->c1;
    b = hist->c2;

    snprintf(strbuf, MAX_DBG_MSG_LEN * sizeof(char), "%s", "hist all (R,G,B)\n");
    for (i = 0; i < hist->bin_cnt; i++) {
        strbuf_len = strlen(strbuf);
        snprintf(strbuf+strbuf_len,(MAX_DBG_MSG_LEN - strbuf_len) * sizeof(char),
                          "%d %d %d,", *r, *g, *b);
        r++;
        g++;
        b++;
    }  //end for loop

}

void free_cmap(struct fb_cmap *cmap)
{

    if(cmap == NULL){
        LOGE("%s: Colormap struct NULL", __FUNCTION__);
        return;
    }

    if (cmap->red)
        free(cmap->red);
    if (cmap->green)
        free(cmap->green);
    if (cmap->blue)
        free(cmap->blue);
}

static void conv3channel(struct fb_cmap *cmap, uint32_t *outmap)
{
    int32_t i;
    __u16 *r, *g, *b;

    if ((cmap == NULL) || (cmap == NULL)) {
        LOGE("%s: Invalid parameters passed", __FUNCTION__);
        return;
    }

    /* map LUT */
    r = cmap->red;
    g = cmap->green;
    b = cmap->blue;
    for (i = 0; i < BL_LUT_SIZE; i++) {
        r[i] = (outmap[i] & 0xFF);
        g[i] = (outmap[i] & 0xFF);
        b[i] = (outmap[i] & 0xFF);
    }
    return;

}

static void initialize_cmap(struct fb_cmap *cmap)
{

    if(cmap == NULL){
        LOGE("%s: Colormap struct NULL", __FUNCTION__);
        return;
    }

    cmap->red = 0;
    cmap->green = 0;
    cmap->blue = 0;
    cmap->transp = 0;

    cmap->start = 0;
    cmap->len = BL_LUT_SIZE;
    cmap->red = (__u16 *)malloc(cmap->len * sizeof(__u16));
    if (!cmap->red) {
        LOGE("%s: can't malloc cmap red!", __FUNCTION__);
        goto fail_rest1;
    }

    cmap->green = (__u16 *)malloc(cmap->len * sizeof(__u16));
    if (!cmap->green) {
        LOGE("%s: can't malloc cmap green!", __FUNCTION__);
        goto fail_rest2;
    }

    cmap->blue = (__u16 *)malloc(cmap->len * sizeof(__u16));
    if (!cmap->blue) {
        LOGE("%s: can't malloc cmap blue!", __FUNCTION__);
        goto fail_rest3;
    }

    return;

fail_rest3:
    free(cmap->blue);
fail_rest2:
    free(cmap->green);
fail_rest1:
    free(cmap->red);

    free_cmap(cmap);
}

static int read_hist(int32_t fb, struct mdp_histogram_data *hist)
{
    int32_t ret;

    if(hist == NULL){
        LOGE("%s: Histogram data is null!", __FUNCTION__);
        return -1;
    }

    ret = ioctl(fb, MSMFB_HISTOGRAM, hist);
    if ((ret != 0) && (errno != ENODATA) && (errno != ETIMEDOUT)
        && (errno != EPERM))
        /* ENODATA or ETIMEDOUT indicates a valid histogram failure */
        LOGE("%s: MSMFB_HISTOGRAM failed: %s", __FUNCTION__, strerror(errno));

    if (ret == 0)
        return 0;
    else
        return -errno;
}

static int32_t stopHistogram() {
    int32_t ret;
    uint32_t block = gHwInfo.nPriDisplayHistBlock;
    ret = ioctl(FBFd, MSMFB_HISTOGRAM_STOP, &block);
    if (ret < 0) {
        LOGE("MSMFB_HISTOGRAM_STOP failed!");
    }
    return ret;
}

static int32_t startHistogram() {
    int32_t ret;
    struct mdp_histogram_start_req req;

    req.block = gHwInfo.nPriDisplayHistBlock;
    req.frame_cnt = 1;
    req.bit_mask = 0x0;
    req.num_bins = gHwInfo.nPriDisplayHistBins;

    ret = ioctl(FBFd, MSMFB_HISTOGRAM_START, &req);
    if (ret < 0) {
        LOGE("MSMFB_HISTOGRAM_START failed!");
    }
    return ret;
}

static int32_t copyHistogram(struct mdp_histogram_data *hist, uint32_t *outhist)
{

    uint32_t offset, size;
    int32_t ret = 0;

    offset = hist->bin_cnt;
    size = hist->bin_cnt * 4;
    switch (gHwInfo.nPriDisplayHistComp) {
    case 3:
        memcpy(outhist + offset * 2, hist->c2, size);
    case 2:
        memcpy(outhist + offset, hist->c1, size);
    case 1:
        memcpy(outhist, hist->c0, size);
        break;
    default:
        ret = -1;
    }
    return ret;
}

static int32_t outCmap(int32_t fb, uint32_t *outcmap, fb_cmap mColorMap)
{
    struct msmfb_mdp_pp lut;
    struct fb_cmap cmap;
    uint32_t *cmap_data;
    int32_t i, ret = -1;
    cmap = mColorMap;
    cmap.start = 0;
    cmap.len = BL_LUT_SIZE;
    char strbuf[MAX_DBG_MSG_LEN]="";
    size_t strbuf_len = 0;

    conv3channel(&cmap,outcmap);

    switch (gHwInfo.nPriDisplayHistBins) {
    default:
        ret = ioctl(fb, MSMFB_SET_LUT, &cmap);
        break;
    case 256:
        cmap_data = (uint32_t *)malloc(cmap.len * sizeof(uint32_t));
        if (!cmap_data)
            goto err_mem;

        snprintf(strbuf, cmap.len * sizeof(char), "%s", "LUT\n");
        for (i = 0; (uint32_t)i < cmap.len; i++) {
            strbuf_len = strlen(strbuf);
            cmap_data[i] = cmap.red[i] & 0xFF;
            snprintf(strbuf+strbuf_len,(cmap.len - strbuf_len) * sizeof(char),
                              "%d ", cmap_data[i]);
        }

        lut.op = mdp_op_lut_cfg;
        lut.data.lut_cfg_data.lut_type = mdp_lut_hist;
        lut.data.lut_cfg_data.data.hist_lut_data.block = gHwInfo.nPriDisplayHistBlock;
        lut.data.lut_cfg_data.data.hist_lut_data.ops = MDP_PP_OPS_WRITE |
                                                        MDP_PP_OPS_ENABLE;
        lut.data.lut_cfg_data.data.hist_lut_data.len = cmap.len;
        lut.data.lut_cfg_data.data.hist_lut_data.data = cmap_data;

        ret = ioctl(fb, MSMFB_MDP_PP, &lut);
err_mem:
        free(cmap_data);
        break;
    }
    return ret;
}

static int set_backlight_scale(int32_t bl_scale, uint32_t bl_min_level) {
    int32_t ret;
    struct msmfb_mdp_pp backlight;

    backlight.op = mdp_bl_scale_cfg;
    backlight.data.bl_scale_data.min_lvl = bl_min_level;
    if (bl_scale > BL_SCALE_MAX)
        bl_scale = BL_SCALE_MAX;
    backlight.data.bl_scale_data.scale = bl_scale;
    ret = ioctl(FBFd, MSMFB_MDP_PP, &backlight);
    if (ret)
        LOGE("FAILED TO SET BACKLIGHT SCALE, %s", strerror(errno));

    return ret;
}

static int ql_string2int(char* c_lvl){
    int ret = -1;
    if (!strcmp(c_lvl, CABL_LVL_LOW)) {
        ret = ABL_QUALITY_LOW;
    } else if (!strcmp(c_lvl, CABL_LVL_MEDIUM)) {
        ret = ABL_QUALITY_NORMAL;
    } else if (!strcmp(c_lvl, CABL_LVL_HIGH)) {
        ret = ABL_QUALITY_HIGH;
    } else if (!strcmp(c_lvl, CABL_LVL_AUTO)) {
        ret = ABL_QUALITY_AUTO;
    }
    return ret;
}

void ScreenRefresher::ProcessRefreshWork() {
    int32_t ret =-1;
    uint32_t count = 0;
    struct timespec wait_time;

    LOGE_IF(mDebug, "%s() Entering, mState = %d", __func__, mState);
    pthread_mutex_lock(&mLock);
    mState |= REFRESHER_STATE_ENABLE;
    pthread_mutex_unlock(&mLock);
    sp<IServiceManager> sm = defaultServiceManager();
    if (sm == NULL) {
        LOGE("NULL servicemanger object");
        return ;
    }

    pthread_mutex_lock(&mLock);

    while (mState & REFRESHER_STATE_ENABLE) {
        LOGE_IF(mDebug, "Wait for mWaitCond!");
        if (!(mState & REFRESHER_STATE_CONFIG)) {
            pthread_cond_wait(&mWaitCond, &mLock); //Wait for Refresh()
        }

        count = 0;

        LOGE_IF(mDebug, "mState = %d, mFrames = %d", mState, mFrames);
        while (mFrames && count < mFrames ) {
            LOGE_IF(mDebug, "mFrames = %d, count = %d", mFrames, count);
            pthread_mutex_unlock(&mLock);
            ret = screenRefresh();
            if(ret < 0) {
                LOGE("%s: Failed to signal HWC", strerror(errno));
            }
            count++;

            clock_gettime(CLOCK_REALTIME, &wait_time);
            wait_time.tv_nsec += mMS*1000000;

            pthread_mutex_lock(&mLock);
            ret = pthread_cond_timedwait(&mWaitCond, &mLock, &wait_time);
            if (ret == 0) {
                count = 0;
            } else if (ret == ETIMEDOUT) {
                continue;
            } else {
                LOGE("%s: pthread_cond_timedwait failed. err: %s", __func__, strerror(errno));
            }
        }
        mState &= ~REFRESHER_STATE_CONFIG;
    }
    pthread_mutex_unlock(&mLock);
    pthread_exit(NULL);
    LOGE_IF(mDebug, "%s() Exiting!", __func__);
    return;
}

int ScreenRefresher::Control(bool bEnable) {
    int ret = 0;

    LOGE_IF(mDebug, "%s() Entering!", __func__);
    if( bEnable ) {
        pthread_mutex_lock(&mLock);
        mRefCount = mRefCount + 1;
        if( mState & REFRESHER_STATE_ENABLE ) {
            ret = -1;
            pthread_mutex_unlock(&mLock);
            return ret;
        }
        /*start the incalidate thread here*/
        LOGE_IF(mDebug, "Starting the refresh thread!");
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        ret = pthread_create(&mThread, &attr, refresher_thrd_func, this);
        if (ret) {
            LOGE("Failed to create screen refresher thread, error = %s", strerror(ret));
        }
        pthread_attr_destroy(&attr);
        pthread_mutex_unlock(&mLock);
    } else {
        pthread_mutex_lock(&mLock);
        mRefCount = mRefCount - 1;
        if (mRefCount == 0) {
            LOGE_IF(mDebug, "Stopping the refresh thread!");
            mState &= ~REFRESHER_STATE_ENABLE;
        }
        pthread_mutex_unlock(&mLock);
    }

    LOGE_IF(mDebug, "%s() Exiting, ret = %d!", __func__, ret);
    return ret;
}

int ScreenRefresher::Refresh(uint32_t nFrames, uint32_t nMS) {
    int ret = -1;

    LOGE_IF(mDebug, "%s() Entering!", __func__);
    pthread_mutex_lock(&mLock);
    if (mState & REFRESHER_STATE_ENABLE) {
        mFrames = nFrames;
        mMS = nMS;
        mState |= REFRESHER_STATE_CONFIG;
        pthread_cond_signal(&mWaitCond);
        LOGE_IF(mDebug, "Sending mWaitCond signal!");
        ret = 0;
    }
    pthread_mutex_unlock(&mLock);
    LOGE_IF(mDebug, "%s() Exiting, ret = %d!", __func__, ret);
    return ret;
}

int ScreenRefresher::Notify(int notification_type) {
    LOGE_IF(mDebug, "%s() Entering!", __func__);

    LOGE_IF(mDebug, "%s() Exiting, ret = -1!", __func__);
    return -1;
}

void AbaContext::FilteredSignal(){
    pthread_mutex_lock(&mAbaLock);
    if(mSignalToWorker == WAIT_SIGNAL) {
        if (pRefresher){
            LOGE_IF(mDebug, "%s: Calling ScreenRefresher->Refresh", __func__);
            pRefresher->Refresh(1,16);
        }
        pthread_cond_signal(&mAbaCond);
    }
    mSignalToWorker = NO_WAIT_SIGNAL;
    pthread_mutex_unlock(&mAbaLock);
}

void CABL::initHW() {
    /* Hardware capacity determination*/
    mHistInfo.hist_block = gHwInfo.nPriDisplayHistBlock;
    mHistInfo.hist_bins = gHwInfo.nPriDisplayHistBins;
    mHistInfo.hist_components = gHwInfo.nPriDisplayHistComp;
}

int is_backlight_modified(CABL *cabl, int *old_lvl)
{
    int ret, temp_lvl = get_backlight_level();
    if (temp_lvl < 0) {
        LOGE("Invalid backlight level: %d !", temp_lvl);
        return 0;
    }
    struct fb_cmap cmap;
    if (temp_lvl != *old_lvl) {
        ret = 1;
        LOGE_IF(cabl->mDebug, "The BL level changed,");
        *old_lvl = temp_lvl;
        LOGE_IF(cabl->mDebug, "The BL level changed to %d", temp_lvl);
        /* Reset the orig level only if > than the min level */
        if (temp_lvl >= (long) cabl->mOEMParams.bl_min_level) {
            cabl->mOEMParams.orig_level = temp_lvl;
            abl_change_orig_bl_level(&cabl->mOEMParams);
            //set the new restore level
            cabl->mUserBLLevel = temp_lvl;
        }
    } else {
        ret = 0;
    }
    return ret;
}

void AbaContext::initHW() {
    uint32_t i,j;
    mAbaHwInfo.uHistogramBins = gHwInfo.nPriDisplayHistBins;
    mAbaHwInfo.uHistogramComponents = gHwInfo.nPriDisplayHistComp;
    mAbaHwInfo.uBlock = gHwInfo.nPriDisplayHistBlock;
    mAbaHwInfo.uLUTSize = BL_LUT_SIZE;
    mAbaHwInfo.uFactor = HIST_PROC_FACTOR;

    i = mAbaHwInfo.uHistogramBins;
    j = 0;
    while (i > 0) {
        j += (i&0x1)?1:0;
        i = i >> 1;
    }

    if ((mAbaHwInfo.uHistogramBins <= 256) && (1 == j)) {
        i = mAbaHwInfo.uHistogramBins;
        do {
            i = i >> 1;
            mAbaHwInfo.uFactor--;
        } while (i > 1);
        mAbaHwInfo.uHalfBin = (mAbaHwInfo.uFactor)?(1<<(mAbaHwInfo.uFactor-1)):0;
    }

    mInputHistogram = (uint32_t *)malloc(mAbaHwInfo.uHistogramBins *
        mAbaHwInfo.uHistogramComponents * sizeof(uint32_t));
    if (!mInputHistogram) {
        LOGE("%s: can't malloc mInputHistogram!", __FUNCTION__);
        goto fail_hist;
    }

    mOutputHistogram = (uint32_t *)malloc(mAbaHwInfo.uHistogramBins *
        mAbaHwInfo.uHistogramComponents * sizeof(uint32_t));
    if (!mOutputHistogram) {
        LOGE("%s: can't malloc mOutputHistogram!", __FUNCTION__);
        goto fail_hist;
    }

    minLUT = (uint32_t *) malloc((mAbaHwInfo.uLUTSize) * sizeof(uint32_t));
    if (!minLUT) {
        LOGE("%s: can't malloc minLUT!", __FUNCTION__);
        goto fail_hist;
    }

    hist.c0 = 0;
    hist.c1 = 0;
    hist.c2 = 0;
    hist.block = mAbaHwInfo.uBlock;
    hist.bin_cnt = mAbaHwInfo.uHistogramBins;
    hist.c0 = (uint32_t *)malloc(hist.bin_cnt * sizeof(uint32_t));
    if (!hist.c0) {
        LOGE("%s: can't malloc red cmap!", __FUNCTION__);
        goto fail_hist;
    }

    hist.c1 = (uint32_t *)malloc(hist.bin_cnt * sizeof(uint32_t));
    if (!hist.c1) {
        LOGE("%s: can't malloc green cmap!", __FUNCTION__);
        goto fail_hist;
    }

    hist.c2 = (uint32_t *)malloc(hist.bin_cnt * sizeof(uint32_t));
    if (!hist.c2) {
        LOGE("%s: can't malloc blue cmap!", __FUNCTION__);
        goto fail_hist;
    }

    hist.extra_info = (uint32_t *)malloc(2 * sizeof(uint32_t));
    if (!hist.extra_info) {
        LOGE("%s: can't malloc extra info!", __FUNCTION__);
        goto fail_hist;
    }
    return;

fail_hist:
    free_hist(&hist);

    if (mInputHistogram)
        free(mInputHistogram);
    if (mOutputHistogram)
        free(mOutputHistogram);
    if (minLUT)
        free(minLUT);

    return;
}

int AbaContext::is_backlight_modified(int *old_lvl)
{
    int ret, temp_lvl = get_backlight_level();
    if (temp_lvl < 0) {
        LOGE("Invalid backlight level: %d !", temp_lvl);
        return 0;
    }
    if (temp_lvl != *old_lvl) {
        ret = 1;
        LOGE_IF(mCablDebug, "%s: The BL level changed,", __FUNCTION__);
        *old_lvl = temp_lvl;
        LOGE_IF(mCablDebug, "%s: The BL level changed to %d", __FUNCTION__, temp_lvl);

        /* Reset the orig level only if > than the min level */
        if (temp_lvl >= (long)mCablOEMParams.bl_min_level) {
            orig_level = temp_lvl;
            ret = AbaSetOriginalBacklightLevel(pHandle, orig_level);
            //set the new restore level
            mUserBLLevel = temp_lvl;
        }
    } else {
        ret = 0;
    }
    return ret;
}

int32_t CABL::auto_adjust_quality_lvl(){
    int32_t result = 0;
    char lvl[MAX_CMD_LEN];
    char property[PROPERTY_VALUE_MAX];
    if (property_get(YUV_INPUT_STATE_PROP, property, NULL) > 0) {
        if ((atoi(property) == 1) && (mPowerSaveLevel != mOEMParams.video_quality_lvl)) {
            mPowerSaveLevel = mOEMParams.video_quality_lvl;
            LOGE_IF(mDebug, "Power saving level: %d", mPowerSaveLevel);
            pthread_mutex_lock(&mCABLLock);
            result = abl_change_quality_level(&mOEMParams, mPowerSaveLevel);
            pthread_mutex_unlock(&mCABLLock);
        }else if ((atoi(property) == 0) && (mPowerSaveLevel != mOEMParams.ui_quality_lvl)) {
            mPowerSaveLevel = mOEMParams.ui_quality_lvl;
            LOGE_IF(mDebug, "Power saving level: %d", mPowerSaveLevel);
            pthread_mutex_lock(&mCABLLock);
            result = abl_change_quality_level(&mOEMParams, mPowerSaveLevel);
            pthread_mutex_unlock(&mCABLLock);
        }
    }
    return result;
}

int32_t AbaContext::auto_adjust_quality_lvl(){
    int32_t result = 0;
    char lvl[MAX_CMD_LEN];
    char property[PROPERTY_VALUE_MAX];
    if (property_get(YUV_INPUT_STATE_PROP, property, NULL) > 0) {
        if ((atoi(property) == 1) && (mQualityLevel != mCablOEMParams.video_quality_lvl)) {
            mQualityLevel = mCablOEMParams.video_quality_lvl;
            LOGE_IF(mCablDebug, "%s: Power saving level: %d", __FUNCTION__, mQualityLevel);
            pthread_mutex_lock(&mAbaLock);
            result = AbaSetQualityLevel(pHandle, (AbaQualityLevelType) mQualityLevel);
            if( result != ABA_STATUS_SUCCESS)
                LOGE("AbaSetQualityLevel failed with status = %d", result);
            pthread_mutex_unlock(&mAbaLock);
        }else if ((atoi(property) == 0) && (mQualityLevel != mCablOEMParams.ui_quality_lvl)) {
            mQualityLevel = mCablOEMParams.ui_quality_lvl;
            LOGE_IF(mCablDebug, "%s: Power saving level: %d", __FUNCTION__, mQualityLevel);
            pthread_mutex_lock(&mAbaLock);
            result = AbaSetQualityLevel(pHandle, (AbaQualityLevelType) mQualityLevel);
            if( result != ABA_STATUS_SUCCESS)
                LOGE("AbaSetQualityLevel failed with status = %d", result);
            pthread_mutex_unlock(&mAbaLock);
        }
    }
    return result;
}

int calculateDiffALS(int currALS, int prevALS){
    int ret = 0;
    float calcVal;
    if (!prevALS)
        return (currALS > 0) ? 1 : 0;
    calcVal = fabsf((prevALS - currALS) / prevALS);
    if(calcVal > SVI_ALS_RATIO_THR)
        return 1;
    if (fabsf(prevALS - currALS) > SVI_ALS_LIN_THR)
        ret = 1;
    return ret;
}

/* Single ABA Worker for CABL and SVI */
void AbaContext::ProcessAbaWorker() {

    AbaStatusType retval;
    AbaStateType CablState;
    AbaStateType SVIState;

    int32_t old_level, bl_scale_ratio = 0;
    int32_t set_ratio;
    int32_t ret;
    uint32_t tmp_ALS = 1, out_ALS = 0;
    void *term;
    bool32 IsConverged = 0;

    set_ratio = mCablOEMParams.bl_min_level;
    LOGE_IF(mDebug, "%s(): Entering ", __func__);

    pthread_mutex_lock(&mAbaLock);
    if(!mWorkerRunning)
        mWorkerRunning = true;
    else{
        pthread_mutex_unlock(&mAbaLock);
        goto exit_cleanup;
    }
    pthread_mutex_unlock(&mAbaLock);

    if (IsABAStatusON(ABA_FEATURE_SVI)) {
        if((!mALSUThreadRunning) && (mSVIEnable))
            StartALSUpdaterThread();
    }

    pRefresher = new ScreenRefresher;
    if (pRefresher){
        LOGE_IF(mDebug, "%s: Calling ScreenRefresher->Control(true)", __func__);
        pRefresher->Control(1);
    }

    old_level = mUserBLLevel;
    set_ratio = BL_SCALE_MAX;

    LOGE_IF(mDebug, "%s: Starting worker thread", __FUNCTION__);

    pthread_mutex_lock(&mAbaLock);
    while (IsABAStatusON(ABA_FEATURE_CABL | ABA_FEATURE_SVI)){
        LOGE_IF(mDebug, "%s: In outer", __FUNCTION__);

        if (mHistStatus == 0) {
            mSignalToWorker = WAIT_SIGNAL;
            LOGE_IF(mDebug, "%s: Waiting for signal", __FUNCTION__);
            pthread_cond_wait(&mAbaCond, &mAbaLock);
            LOGE_IF(mDebug, "%s: AbaWorker is signalled", __FUNCTION__);
        }
        pthread_mutex_unlock(&mAbaLock);

        AbaGetState((pHandle), &CablState,ABA_FEATURE_CABL);
        if((CablState== ABA_STATE_ACTIVE) &&
            (IsABAStatusON(ABA_FEATURE_CABL))){
            if(AUTO_QL_MODE == mCablOEMParams.default_ql_mode) {
                    ret = auto_adjust_quality_lvl();
                    if (ret)
                        LOGE("%s: adjust_quality_level failed", __FUNCTION__);
            }
        }

        pthread_mutex_lock(&mAbaLock);
        while (IsABAStatusON(ABA_FEATURE_CABL | ABA_FEATURE_SVI)){

            if (mHistStatus == 0) {
                pthread_mutex_unlock(&mAbaLock);
                break;
            }
            pthread_mutex_unlock(&mAbaLock);

            AbaGetState((pHandle), &CablState,ABA_FEATURE_CABL);
            AbaGetState((pHandle), &SVIState,ABA_FEATURE_SVI);
            if ((CablState == SVIState) && (SVIState == ABA_STATE_ACTIVE))
                LOGE_IF(mDebug, "Both SVI and CABL active");
            else if (CablState == ABA_STATE_ACTIVE)
                LOGE_IF(mDebug, "CABL is only active");
            else if (SVIState == ABA_STATE_ACTIVE)
                LOGE_IF(mDebug, "SVI is only active");

            if (!IsABAStatusON(ABA_FEATURE_CABL | ABA_FEATURE_SVI)){
                pthread_mutex_lock(&mAbaLock);
                break;
            }

            if (IsABAStatusON(ABA_FEATURE_SVI)) {
                if((!mALSUThreadRunning) && (mSVIEnable))
                    StartALSUpdaterThread();
            }
            pthread_mutex_lock(&mALSUpdateLock);
            tmp_ALS = mCurrALSValue;
            mLastALSValue = mCurrALSValue;
            pthread_mutex_unlock(&mALSUpdateLock);

            retval = AbaSetAmbientLightLevel((pHandle), tmp_ALS, &out_ALS);
            if(retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM
                || retval == ABA_STATUS_NOT_SUPPORTED) {
                LOGE("%s: AbaSetAmbientLightLevel() failed with ret = %d",
                    __FUNCTION__, retval);
                pthread_mutex_lock(&mAbaLock);
                continue;
            }
            LOGE_IF(mDebug, "%s: Input ALS = %u, Output ALS = %u", __func__,
                tmp_ALS, out_ALS);

            pthread_mutex_lock(&mAbaLock);
            if (mHistStatus == 1){
                pthread_mutex_unlock(&mAbaLock);
                is_backlight_modified(&old_level);
                ret = read_hist(FBFd, &hist);
                if(ret != 0) {
                    LOGE_IF(mDebug, "%s: Do Histogram read failed - ret = %d",
                        __FUNCTION__, ret);
                    pthread_mutex_lock(&mAbaLock);
                    continue;
                }

                //Preprocess Histogram only if not MDP5 & CABL is ON
                AbaGetState((pHandle),&CablState,ABA_FEATURE_CABL);
                if((!gMDP5) && (CablState==ABA_STATE_ACTIVE) &&
                    (IsABAStatusON(ABA_FEATURE_CABL))) {
                    if(!copyHistogram(&hist, mInputHistogram)){
                        retval = AbaPreprocessHistogram((pHandle),
                        mInputHistogram, mOutputHistogram);

                        if(retval == ABA_STATUS_FAIL ||
                            retval == ABA_STATUS_BAD_PARAM ||
                                        retval == ABA_STATUS_NOT_SUPPORTED) {
                            LOGE("%s: PreProcess failed retval = %d",
                                __FUNCTION__,retval);
                            pthread_mutex_lock(&mAbaLock);
                            continue;
                        }
                    }
                }
                LOGE_IF(mDebug,"%s: Backlight value= %d", __FUNCTION__,mBl_lvl);
            } else {
                pthread_mutex_unlock(&mAbaLock);
            }

            retval = AbaProcess(pHandle, mInputHistogram, minLUT, &mBl_lvl);
            if (retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM ||
                        retval == ABA_STATUS_NOT_SUPPORTED) {
                LOGE("%s: AbaProcess failed ret = %d", __FUNCTION__, retval);
                pthread_mutex_lock(&mAbaLock);
                continue;
            }

            if (mBl_lvl != (uint32_t) set_ratio) {
                LOGE_IF(mCablDebug, "User level = %03d Set Ratio = %03d \
                    Scale Ratio = %04d", mUserBLLevel, (int32_t) set_ratio,
                    (int32_t) bl_scale_ratio);

                AbaGetState((pHandle), &CablState, ABA_FEATURE_CABL);
                if((CablState == ABA_STATE_ACTIVE) &&
                    (IsABAStatusON(ABA_FEATURE_CABL))){
                    if (is_backlight_modified(&old_level)) {
                        pthread_mutex_lock(&mAbaLock);
                        continue;
                    }
                }

                set_backlight_scale(mBl_lvl, mCablOEMParams.bl_min_level);
                set_ratio = mBl_lvl;

            }

            retval = AbaIsConverged(pHandle, &IsConverged);
            if(retval != ABA_STATUS_SUCCESS){
                    IsConverged = 0;
            }

            if(!IsConverged) {
                LOGE_IF(mDebug, "Updating LUT and calling Refresh!");
                ret = outCmap(FBFd, minLUT, mColorMap);
                if (ret != 0) {
                    LOGE("%s: outCmap() failed", __FUNCTION__);
                    pthread_mutex_lock(&mAbaLock);
                    continue;
                }
                if (pRefresher)
                    pRefresher->Refresh(1,16);
            }

            pthread_mutex_lock(&mAbaLock);
        }
        pthread_mutex_unlock(&mAbaLock);
        /*
         * Immediate unlock and lock to provide any thread waiting for lock chance to acquire
        */
        pthread_mutex_lock(&mAbaLock);
    }
    pthread_mutex_unlock(&mAbaLock);

    if (pRefresher){
        LOGE_IF(mDebug, "%s: Calling ScreenRefresher->Control(0)", __func__);
        pRefresher->Control(0);
    }

    /* Cleanup  */
    pthread_mutex_lock(&mAbaLock);
    mWorkerRunning = false;
    pthread_mutex_unlock(&mAbaLock);

    free_cmap(&mColorMap);

    pthread_mutex_lock(&mAbaLock);
    mHistStatus = 0;
    pthread_mutex_unlock(&mAbaLock);

    set_backlight_scale(BL_SCALE_MAX, mCablOEMParams.bl_min_level);

    //Stop the running ALS Updater Thread
    if((mSVIEnable) && (mALSUThreadRunning))
        StopALSUpdaterThread();

    if(pRefresher)
        delete pRefresher;

    LOGE_IF(mDebug, "%s(): Exiting ", __func__);

    pthread_join(pthread_self(),&term);
    pthread_exit(NULL);
    return;

exit_cleanup:
    pthread_mutex_lock(&mAbaLock);
    if(mWorkerRunning)
        mWorkerRunning = false;
    pthread_mutex_unlock(&mAbaLock);

    //Stop the running ALS Updater Thread
    if((mSVIEnable) && (mALSUThreadRunning))
        StopALSUpdaterThread();

    if(pRefresher)
        delete pRefresher;

    LOGE_IF(mDebug, "%s(): Exiting with error", __func__);

    pthread_join(pthread_self(),&term);
    pthread_exit(NULL);
}

int AbaContext::Notify(int notification_type) {
    int32_t ret = 0;
    int32_t level = 0;
    LOGE_IF(mDebug, "Starting %s ", __func__);
    pthread_mutex_lock(&mAbaLock);
    if (notification_type == NOTIFY_TYPE_UPDATE && mHistStatus == 0) {
        pthread_mutex_unlock(&mAbaLock);
        level = get_backlight_level();
        if (level <= (int32_t)mCablOEMParams.bl_min_level) {
            LOGE_IF(mCablDebug, "New BL level %d lower than min level %d,"
                    " Skip this update for calc",
                    level, mCablOEMParams.bl_min_level);
            return -1;
        }
        if (!IsABAStatusON(ABA_FEATURE_CABL | ABA_FEATURE_SVI))
            return ret;
        LOGE_IF(mDebug, "Start notification received, start histogram");
        ret = startHistogram();
        if (0 == ret) {
            pthread_mutex_lock(&mAbaLock);
            mHistStatus = 1;
            pthread_mutex_unlock(&mAbaLock);
            LOGE_IF(mDebug, "%s: Filtered signal called from Notify Start", __FUNCTION__);
            FilteredSignal();
        }
    } else if (mHistStatus == 1 &&
        (notification_type == NOTIFY_TYPE_SUSPEND ||
        notification_type == NOTIFY_TYPE_NO_UPDATE)) {
        LOGE_IF(mDebug, "Stop notification received, stop histogram");
        if(!stopHistogram()){
            mHistStatus = 0;
        }
        pthread_mutex_unlock(&mAbaLock);
        if((notification_type == NOTIFY_TYPE_SUSPEND) && (mSVIEnable)){
            StopALSUpdaterThread();
        }

    } else {
        pthread_mutex_unlock(&mAbaLock);
    }
    return ret;
}

/* SetLightSensor() should be called only when mSVIEnable is true*/
void AbaContext::SetLightSensor() {
    int ret;
    char property[PROPERTY_VALUE_MAX];

    if (property_get(SVI_SENSOR_PROP, property, 0) > 0) {
        int type = atoi(property);
        if (type == 0) {
            mLightSensor = new ALS();
        } else if (type == 1) {
            mLightSensor = new DummySensor();
        } else if (type == 2) {
            mLightSensor = new NativeLightSensor();
#ifdef ALS_ENABLE
        } else if (type == 3) {
            mLightSensor = new LightSensor();
#endif
        } else {
            LOGE("Invalid choice for sensor type, initializing the default sensor class!");
            mLightSensor = new ALS();
        }
    } else {
#ifdef ALS_ENABLE
        mLightSensor  = new LightSensor();
#else
        mLightSensor = new NativeLightSensor();
#endif
    }

    // Registering the ALS
    ret = mLightSensor->ALSRegister();
    if( ret ) {
        LOGE("%s(): ALSRegister() Failed : Return Value = %d", __func__, ret);
    }
}

/* CleanupLightSensor() should be called only when mSVIEnable is true*/
void AbaContext::CleanupLightSensor() {
    int ret;
    //CleanUp ALS
    ret = mLightSensor->ALSCleanup();
    if( ret ) {
        LOGE("%s(): ALSCleanup() Failed : Return Value = %d", __func__, ret);
    }
    delete mLightSensor;
}

/* StopALSUpdaterThread should be called only when mSVIEnable is true*/
void AbaContext::StopALSUpdaterThread(){

    LOGE_IF(mDebug, "%s(): Entering ", __func__);
    int ret;

    //Disabling ALS
    ret = mLightSensor->ALSDeRegister();
    if (ret) {
        LOGE("%s(): ALSDeRegister() Failed : Return Value = %d",__func__,ret);
    } else {
        pthread_mutex_lock(&mALSUpdateLock);
        bALSRunEnabled = false;
        pthread_mutex_unlock(&mALSUpdateLock);
    }
    //Stopping the ALS Updater thread
    pthread_mutex_lock(&mALSUpdateLock);
    mALSUpdateThreadStatus = ALS_UPDATE_OFF;
    //Setting mCurrALSValue to 1 for moving algo to CABL side
    mCurrALSValue = 1;
    pthread_mutex_unlock(&mALSUpdateLock);

    LOGE_IF(mDebug, "%s(): Exiting ", __func__);
}

/* StartALSUpdaterThread should be called only when mSVIEnable is true*/
void AbaContext::StartALSUpdaterThread(){

    LOGE_IF(mDebug, "%s(): Entering ", __func__);
    int ret_val;
    int32_t err;

    //Enabling ALS
    ret_val = mLightSensor->ALSRun(false);
    if(ret_val) {
        LOGE("%s(): ALSRun() Failed : Return Value = %d",__func__, ret_val);
    } else{
        bALSRunEnabled = true;
    }

    //Checkiung if the ALS Updation thread is running or not
    pthread_mutex_lock(&mALSUpdateLock);
    if (mALSUThreadRunning){
        pthread_mutex_unlock(&mALSUpdateLock);
        goto exit_func;
    }
    pthread_mutex_unlock(&mALSUpdateLock);

    //Creating a separate thread for ALS Updation
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    /* update the Thread status */
    pthread_mutex_lock(&mALSUpdateLock);
    mALSUThreadRunning = true;
    mALSUpdateThreadStatus = ALS_UPDATE_ON;
    pthread_mutex_unlock(&mALSUpdateLock);

    LOGE_IF(mDebug, "%s: Trying to create a new thread", __func__);
    err = pthread_create(&mALSUpdateThread, &attr, als_update_func, this);
    if (err) {
        LOGE("%s: ALSUpdate: Failed to start the updater thread", __FUNCTION__);
        mALSUThreadRunning = false;
        mALSUpdateThreadStatus = ALS_UPDATE_OFF;
        pthread_attr_destroy(&attr);
    }
    pthread_attr_destroy(&attr);

exit_func:

    LOGE_IF(mDebug, "%s(): Exiting ", __func__);
}

void AbaContext::ProcessAbaALSUpdater(){

    int ret_val, tmp_ALS = 1, prev_ALS = 1, diff_ALS = 0, worker_ALS = 0;
    void *term;

    LOGE_IF(mDebug, "%s(): Entering ", __func__);

    pthread_mutex_lock(&mALSUpdateLock);
    while(mALSUpdateThreadStatus == ALS_UPDATE_ON){
        if(bALSRunEnabled){
            worker_ALS = mLastALSValue;
            pthread_mutex_unlock(&mALSUpdateLock);

            tmp_ALS = mLightSensor->ALSReadSensor();
            tmp_ALS = (tmp_ALS > 0) ? tmp_ALS : 1;
            LOGE_IF(mDebug, "%s: ALS value read = %d", __func__, tmp_ALS);
            diff_ALS = calculateDiffALS(tmp_ALS, worker_ALS);
            if (diff_ALS)
                FilteredSignal();

            if (calculateDiffALS(tmp_ALS, prev_ALS)) {
                prev_ALS = tmp_ALS;
                pthread_mutex_lock(&mALSUpdateLock);
                mCurrALSValue = tmp_ALS;
                pthread_mutex_unlock(&mALSUpdateLock);
            }

        } else {
            pthread_mutex_unlock(&mALSUpdateLock);
            ret_val = mLightSensor->ALSRun(false);
            if(ret_val) {
                LOGE("%s(): ALSRun() Failed : Return Value = %d", __func__,
                    ret_val);
                pthread_mutex_lock(&mALSUpdateLock);
                continue;
            }
            bALSRunEnabled = true;
            pthread_mutex_unlock(&mALSUpdateLock);
        }
        pthread_mutex_lock(&mALSUpdateLock);
    }
    pthread_mutex_unlock(&mALSUpdateLock);

    pthread_mutex_lock(&mALSUpdateLock);
    mALSUThreadRunning =false;
    pthread_mutex_unlock(&mALSUpdateLock);

    LOGE_IF(mDebug, "%s(): Exiting ", __func__);

    pthread_join(pthread_self(),&term);
    pthread_exit(NULL);
}

int32_t AbaContext::CABLControl(bool bEnable) {
    AbaStatusType retval;
    LOGD_IF(mCablDebug, "Start_CABL E");
    pthread_mutex_lock(&mCABLCtrlLock);
    if (mAbaHwInfo.uHistogramBins== 0) {
        LOGE("%s: CABL not supported on this HW!", __FUNCTION__);
        pthread_mutex_unlock(&mCABLCtrlLock);
        return -1;
    }

    if (!mCablEnable) {
        LOGE("%s: CABL not enabled!", __FUNCTION__);
        pthread_mutex_unlock(&mCABLCtrlLock);
        return -1;
    }

    if(bEnable){ //CABL ON
        if (IsABAStatusON(ABA_FEATURE_CABL)) {
            LOGE("%s: CABL is already on!", __FUNCTION__);
            pthread_mutex_unlock(&mCABLCtrlLock);
            return 0;
        }
        retval = AbaGetDefaultParams(eCABLConfig, &mCablOEMParams,
            sizeof(mCablOEMParams));
        if(retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM ||
            retval == ABA_STATUS_NOT_SUPPORTED) {
            LOGE("%s: GetDefaultParams Failed retval = %d", __FUNCTION__, retval);
            pthread_mutex_unlock(&mCABLCtrlLock);
            return retval;
        }

        /* oem specific initialization */
        Aba_apiInfoInit_oem(&mCablOEMParams);

        /* driver initialization */
        retval = AbaInit(eCABLConfig, &mCablOEMParams, sizeof(mCablOEMParams),
            &mAbaHwInfo, pHandle);
        if(retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM ||
            retval == ABA_STATUS_NOT_SUPPORTED) {
            LOGE("%s: AbaInit() Failed retval = %d", __FUNCTION__, retval);
            pthread_mutex_unlock(&mCABLCtrlLock);
            return retval;
        }

        /* Get the backlight level and initialize the new level to it */
        mUserBLLevel = get_backlight_level();
        if (mUserBLLevel < 0) {
            LOGE("Invalid backlight level: %d !", mUserBLLevel);
        }
        /* Following api is called only for CABL*/
        retval = AbaSetOriginalBacklightLevel(pHandle, mUserBLLevel);
        if(retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM ||
            retval == ABA_STATUS_NOT_SUPPORTED) {
            LOGE("%s: AbaSetOriginalBacklightLevel Failed retval = %d",
                __FUNCTION__, retval);
            pthread_mutex_unlock(&mCABLCtrlLock);
            return retval;
        }

        /* Get the user defined power saving level */
        char property[PROPERTY_VALUE_MAX];
        if (property_get("hw.cabl.level", property, NULL) > 0) {
            CABLQualityLevelType newLvl = (CABLQualityLevelType) ql_string2int(property);
            if ((newLvl < 0) || (newLvl > ABL_QUALITY_MAX)) {
                mQualityLevel = (CABLQualityLevelType) ABA_QUALITY_HIGH;
                LOGE("%s: Invalid power saving level, setting hw.cabl.level to High",
                    __FUNCTION__);
            } else if (newLvl < ABL_QUALITY_MAX) {
                mQualityLevel = (CABLQualityLevelType) newLvl;
                LOGE_IF(mCablDebug, "Quality level: %d", mQualityLevel);
            }
        }

        retval = AbaSetQualityLevel(pHandle,(AbaQualityLevelType) mQualityLevel);
        if(retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM ||
            retval == ABA_STATUS_NOT_SUPPORTED) {
            LOGE("%s: AbaSetQualityLevel(CABL) failed retval = %d", __FUNCTION__,
                retval);
            pthread_mutex_unlock(&mCABLCtrlLock);
            return retval;
        }

        retval = AbaActivate(pHandle,ABA_FEATURE_CABL);
        if(retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM ||
            retval == ABA_STATUS_NOT_SUPPORTED) {
            LOGE("%s: AbaActivate() failed retval = %d", __FUNCTION__, retval);
            pthread_mutex_unlock(&mCABLCtrlLock);
            return retval;
        }

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        /* update the CABL status so threads can run */
        SetABAStatusON(ABA_FEATURE_CABL);
        LOGE("Start_CABL sets CABL STATUS TO CABL_ON");
        /* Check if worker thread exists else start*/
        if(!mWorkerRunning){
            stopHistogram();
            initialize_cmap(&mColorMap);
            int32_t err = pthread_create(&mWorkerThread, &attr, aba_worker_func, this);
            if (err) {
                LOGE("%s: CABL: Failed to start the control thread", __FUNCTION__);
                SetABAStatusOFF(ABA_FEATURE_CABL);
                free_cmap(&mColorMap);
                pthread_attr_destroy(&attr);
                pthread_mutex_unlock(&mCABLCtrlLock);
                return -1;
            }
        }
        pthread_attr_destroy(&attr);
        LOGE_IF(mCablDebug, "Start_CABL X");
        pthread_mutex_unlock(&mCABLCtrlLock);
        return 0;

    } else { //CABL Stop
        LOGD_IF(mCablDebug, "Stop_CABL E");
        if (!IsABAStatusON(ABA_FEATURE_CABL)) {
            LOGE("%s: CABL is already off!", __FUNCTION__);
            pthread_mutex_unlock(&mCABLCtrlLock);
            return -1;
        }
        SetABAStatusOFF(ABA_FEATURE_CABL);
        AbaDisable(pHandle,ABA_FEATURE_CABL);
        LOGE_IF(mCablDebug, "Stopped CABL X");
    }
    pthread_mutex_unlock(&mCABLCtrlLock);
    return 0;
}

int AbaContext::SVIControl(bool bEnable) {
    int ret = -1;
    AbaStatusType retval;
    pthread_mutex_lock(&mSVICtrlLock);

    if(mSVISupported) {
        if (bEnable) {
            if (IsABAStatusON(ABA_FEATURE_SVI)) {
                LOGE(" %s %d SVI is already on", __FUNCTION__, __LINE__);
                pthread_mutex_unlock(&mSVICtrlLock);
                return ret;
            }

            if (mSVIEnable) {
                pthread_mutex_lock(&mAbaLock);
                SetABAStatusON(ABA_FEATURE_SVI);
                pthread_mutex_unlock(&mAbaLock);

                retval = AbaGetDefaultParams(eSVIConfig, &mSVIOEMParams, sizeof(mSVIOEMParams));
                if(retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM || retval == ABA_STATUS_NOT_SUPPORTED) {
                    LOGE("%s: GetDefaultParams Failed retval = %d", __FUNCTION__, retval);
                    pthread_mutex_unlock(&mSVICtrlLock);
                    return retval;
                }

                /* driver initialization */
                retval = AbaInit(eSVIConfig, &mSVIOEMParams, sizeof(mSVIOEMParams), &mAbaHwInfo, pHandle);
                if(retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM || retval == ABA_STATUS_NOT_SUPPORTED) {
                    LOGE("%s: AbaInit() Failed retval = %d", __FUNCTION__, retval);
                    pthread_mutex_unlock(&mSVICtrlLock);
                    return retval;
                }

                retval = AbaActivate(pHandle,ABA_FEATURE_SVI);
                if(retval == ABA_STATUS_FAIL || retval == ABA_STATUS_BAD_PARAM || retval == ABA_STATUS_NOT_SUPPORTED) {
                    LOGE("%s: AbaActivate(SVI) failed retval = %d", __FUNCTION__, retval);
                    pthread_mutex_unlock(&mSVICtrlLock);
                    return retval;
                } else {
                    LOGE("%s: Activated SVI feature", __FUNCTION__);
                }

                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
                /* Check if worker thread is running else start */
                if(!mWorkerRunning){
                    stopHistogram();
                    initialize_cmap(&mColorMap);
                    ret = pthread_create(&mWorkerThread, &attr, aba_worker_func, this);
                    if (ret) {
                        SetABAStatusOFF(ABA_FEATURE_SVI);
                        free_cmap(&mColorMap);
                        LOGE("%s:  Failed to start the ad_thread thread", __FUNCTION__);
                        pthread_attr_destroy(&attr);
                        pthread_mutex_unlock(&mSVICtrlLock);
                        return ret;
                    }
                }
                pthread_attr_destroy(&attr);
                StartALSUpdaterThread();
            } else {
                LOGE("%s: SVI is not enabled!", __FUNCTION__);
            }
        } else {
            if (!IsABAStatusON(ABA_FEATURE_SVI)) {
                LOGE("%s: SVI is already off!", __FUNCTION__);
                pthread_mutex_unlock(&mSVICtrlLock);
                return ret;
            }
            StopALSUpdaterThread();
            if (IsABAStatusON(ABA_FEATURE_SVI)) {
                SetABAStatusOFF(ABA_FEATURE_SVI);
                AbaDisable(pHandle,ABA_FEATURE_SVI);
            }
        }
    }
    pthread_mutex_unlock(&mSVICtrlLock);
    return 0;
}

/* Worker thread */
void *worker(void *data) {
    CABL *cabl = (CABL *) data;
    int32_t old_level, set_ratio, bl_scale_ratio = 0;
    int32_t ret;

    if (NULL == cabl)
    {
        LOGE("cabl object passed to worker thread is null");
        return NULL;
    }

    old_level = cabl->mUserBLLevel;
    set_ratio = BL_SCALE_MAX;
    if (cabl->mRefresher) {
        LOGE_IF(cabl->mDebug, "Calling mRefresher->control(true)");
        cabl->mRefresher->Control(1);
    }

    LOGE_IF(cabl->mDebug, "Starting worker thread");
    while (true) {
        pthread_mutex_lock(&cabl->mCABLLock);
        if (cabl->eStatus != CABL_ON) {
            pthread_mutex_unlock(&cabl->mCABLLock);
            break;
        }
        if (cabl->mPowerLevelChange) {
            ret = abl_change_quality_level(&cabl->mOEMParams,
                cabl->mPowerSaveLevel);
            cabl->mPowerLevelChange = false;
            if (ret)
                LOGE("%s: CABL Quality change failed to set %d",__func__,
                    cabl->mPowerSaveLevel);
        }
        pthread_mutex_unlock(&cabl->mCABLLock);

        if (AUTO_QL_MODE == cabl->mOEMParams.default_ql_mode) {
            /* Set the power saving level according to the input format*/
            ret = cabl->auto_adjust_quality_lvl();
            if (ret) {
                LOGE("adjust_quality_level failed");
            }
        }

        pthread_mutex_lock(&cabl->mCABLLock);
        while ((cabl->mHistStatus == 0) && (cabl->eStatus == CABL_ON)) {
            pthread_cond_wait(&cabl->mCABLCond, &cabl->mCABLLock);
        }
        if (cabl->eStatus == CABL_OFF) {
            pthread_mutex_unlock(&cabl->mCABLLock);
            break;
        }
        /* Set LUT */
        if (cabl->mHistStatus == 1) {
            pthread_mutex_unlock(&cabl->mCABLLock);
            is_backlight_modified(cabl, &old_level);
            ret = abl_calc_lut(&bl_scale_ratio, FBFd, &cabl->mColorMap);
            if (ret != 0) {
                if ((ret != -ENODATA) && (ret != -ETIMEDOUT) && (ret != -EPERM))
                    LOGE("abl_calc_lut() failed with ret = %d", ret);
                /* If caused by stopped histogram sleep to yield to control */
                if (ret == -EPERM || ret == -ENODATA)
                    usleep(16666);
                continue;
            }

            LOGE_IF(cabl->mDebug,
                    "User level = %03d Set Ratio = %03d Scale Ratio = %04d",
                    cabl->mUserBLLevel, (int32_t) set_ratio,
                    (int32_t) bl_scale_ratio);
            /*Check to see if BL modified during calculation*/
            if (is_backlight_modified(cabl, &old_level)) {
                /*Revert ABL calculations as not applying LUT*/
                abl_revert_calc();
                continue;
            } else {
                if (bl_scale_ratio != set_ratio) {
                    ret = abl_set_lut(FBFd, &cabl->mColorMap);
                    if (ret != 0) {
                        LOGE("abl_set_lut() failed");
                        abl_revert_calc();
                        continue;
                    }
                    set_backlight_scale(bl_scale_ratio, cabl->mOEMParams.bl_min_level);
                    set_ratio = bl_scale_ratio;
                    if (cabl->mRefresher) {
                        LOGE_IF(cabl->mDebug, "Calling screenRefresh");
                        cabl->mRefresher->Refresh(1, 16);
                    }
                }
                abl_update_history();
            }

        } else {
            pthread_mutex_unlock(&cabl->mCABLLock);
        }
    }
    if (cabl->mRefresher) {
        LOGE_IF(cabl->mDebug, "Calling mRefresher->control(false)");
        cabl->mRefresher->Control(0);
    }
    pthread_exit(NULL);

    return NULL;
}

int CABL::Notify(int notification_type) {
    int32_t ret = 0;
    LOGE_IF(mDebug, "Starting %s ", __func__);
    pthread_mutex_lock(&mCABLLock);
    if (notification_type == NOTIFY_TYPE_UPDATE && mHistStatus == 0) {
        int32_t level = get_backlight_level();
        if (level <= (int32_t)mOEMParams.bl_min_level) {
            LOGE_IF(mDebug, "New BL level %d lower than min level %d,"
                    " Skip this update for calc",
                    level, mOEMParams.bl_min_level);
            pthread_mutex_unlock(&mCABLLock);
            return ret;
        }
        if (eStatus == CABL_OFF) {
            pthread_mutex_unlock(&mCABLLock);
            return ret;
        }
        LOGE_IF(mDebug, "Start notification received, start histogram");
        ret = startHistogram();
        if (0 == ret) {
            mHistStatus = 1;
            pthread_cond_signal(&mCABLCond);
        }
    } else if (mHistStatus == 1 &&
        (notification_type == NOTIFY_TYPE_SUSPEND ||
        notification_type == NOTIFY_TYPE_NO_UPDATE)) {
        LOGE_IF(mDebug, "Stop notification received, stop histogram");
        if(!stopHistogram())
            mHistStatus = 0;

    }
    pthread_mutex_unlock(&mCABLLock);
    return ret;
}

int AD::Notify(int notification_type) {

    pthread_mutex_lock(&mADLock);
    if (notification_type == NOTIFY_TYPE_UPDATE) {
        mADUpdateType = NOTIFY_TYPE_UPDATE;
        pthread_cond_signal(&mADCond);
    }
    else if (notification_type == NOTIFY_TYPE_SUSPEND) {
        mADUpdateType = NOTIFY_TYPE_SUSPEND;
    }
    pthread_mutex_unlock(&mADLock);

    if (notification_type == NOTIFY_TYPE_SUSPEND) {
        pthread_mutex_lock(&mLightSensor->mALSLock);
        pthread_cond_signal(&mLightSensor->mALSCond);
        pthread_mutex_unlock(&mLightSensor->mALSLock);
    }
    return 0;
}

int32_t CABL::start_cabl() {
    LOGE_IF(mDebug, "Start_CABL E");
    pthread_mutex_lock(&mCABLCtrlLock);
    if (gHwInfo.nPriDisplayHistBins == 0) {
        LOGE("CABL not supported on this HW!");
        pthread_mutex_unlock(&mCABLCtrlLock);
        return -1;
    }

    if (!mEnable) {
        LOGE("CABL not enabled!");
        pthread_mutex_unlock(&mCABLCtrlLock);
        return -1;
    }
    pthread_mutex_lock(&mCABLLock);
    if (eStatus == CABL_ON) {
        LOGE_IF(mDebug, "CABL is already on!");
        pthread_mutex_unlock(&mCABLLock);
        pthread_mutex_unlock(&mCABLCtrlLock);
        return 0;
    }
    pthread_mutex_unlock(&mCABLLock);
    /* Get the user defined power saving level */
    char property[PROPERTY_VALUE_MAX];
    if (property_get("hw.cabl.level", property, NULL) > 0) {
        int newLvl = ql_string2int(property);
        if ((newLvl < 0) || (newLvl > ABL_QUALITY_AUTO)) {
            mPowerSaveLevel = ABL_QUALITY_HIGH;
            LOGE("Invalid power saving level, setting hw.cabl.level to High");
        } else if (newLvl < ABL_QUALITY_MAX) {
            mPowerSaveLevel = newLvl;
            LOGE_IF(mDebug, "Power saving level: %d", mPowerSaveLevel);
        } else if (newLvl == ABL_QUALITY_AUTO) {
            mPowerSaveLevel = ABL_QUALITY_HIGH;
        }
    }

    /* Get the backlight level and initialize the new level to it */
    mUserBLLevel = get_backlight_level();
    if (mUserBLLevel < 0) {
        LOGE("Invalid backlight level: %d !", mUserBLLevel);
    }

    /* oem initialization */
    apiInfoInit_oem(&mOEMParams, mPowerSaveLevel, mUserBLLevel);

    // Check if initial level is more than the minimum allowed level
    // otherwise, set it to the min level
    long init_level = (mUserBLLevel >= (long) mOEMParams.bl_min_level) ?
            mUserBLLevel : (long) mOEMParams.bl_min_level;
    LOGE_IF(mDebug, "Original backlight level %03d "
            "Initializing with: %03ld Min level: %03ld",
            mUserBLLevel, init_level, (long) mOEMParams.bl_min_level);

    /* driver initialization */
    int32_t ret = abl_init(FBFd, &mColorMap, &mOEMParams, &mHistInfo,
                                                mPowerSaveLevel, init_level);
    if (ret) {
        LOGE("abl_init() failed!");
        free_cmap(&mColorMap);
        pthread_mutex_unlock(&mCABLCtrlLock);
        return ret;
    }

    mRefresher = new ScreenRefresher();

    /* ABL Init is done, let the control
     * thread start the histogram */
    stopHistogram();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    /* update the CABL status so threads can run */
    eStatus = CABL_ON;
    /* Start the worker thread */
    int32_t err = pthread_create(&mWorkerThreadID, &attr, worker, this);
    if (err) {
        LOGE("CABL: Failed to start the control thread");
        eStatus = CABL_OFF;
        free_cmap(&mColorMap);
        pthread_attr_destroy(&attr);
        pthread_mutex_unlock(&mCABLCtrlLock);
        return -1;
    }
    pthread_attr_destroy(&attr);
    LOGE_IF(mDebug, "Start_CABL X");
    pthread_mutex_unlock(&mCABLCtrlLock);
    return 0;
}

void CABL::stop_cabl() {
    LOGE_IF(mDebug, "Stop_CABL E");
    void *ret;
    pthread_mutex_lock(&mCABLCtrlLock);
    pthread_mutex_lock(&mCABLLock);
    if (eStatus == CABL_OFF) {
        LOGE("CABL is already off!");
        pthread_mutex_unlock(&mCABLLock);
        pthread_mutex_unlock(&mCABLCtrlLock);
        return;
    }
    /* stop the threads */
    eStatus = CABL_OFF;
    pthread_mutex_unlock(&mCABLLock);
    pthread_cond_signal(&mCABLCond);
    /* signal the worker for exit */
    pthread_join(mWorkerThreadID, &ret);
    LOGE_IF(mDebug, "CABL Threads teminated, cleaning up resources");
    pthread_mutex_lock(&mCABLLock);
    mHistStatus = 0;
    pthread_mutex_unlock(&mCABLLock);
    set_backlight_scale(BL_SCALE_MAX, mOEMParams.bl_min_level); //Restore BL to 100%
    if (mRefresher) {
        delete mRefresher;
        mRefresher = NULL;
    }
    LOGE_IF(mDebug, "Stop_CABL X");
    pthread_mutex_unlock(&mCABLCtrlLock);
}

int PostProc::write_hsic_values() {
    int ret = FAILED;

    struct compute_params op_params;
    struct mdp_overlay_pp_params overlay_pp_params;
    struct msmfb_mdp_pp pp;

    op_params.operation = PP_OP_PA;
    op_params.params.pa_params = mNewCfg.pa_cfg;
    ret = display_pp_compute_params(&op_params, &overlay_pp_params);

    pp.op = mdp_op_pa_cfg;
    pp.data.pa_cfg_data.block = mBlockType;
    pp.data.pa_cfg_data.pa_data = overlay_pp_params.pa_cfg;

    ret = ioctl(FBFd, MSMFB_MDP_PP, &pp);
    if (ret) {
        LOGE("%s calling ioctl failed", __func__);
    }
    return ret;
}

void PostProc::init_cc_matrix()
{
    /* Identity matrix for the CC calculation */
    mNewCfg.conv_cfg.cc_matrix[0][0]=1;
    mNewCfg.conv_cfg.cc_matrix[0][1]=0;
    mNewCfg.conv_cfg.cc_matrix[0][2]=0;
    mNewCfg.conv_cfg.cc_matrix[0][3]=0;

    mNewCfg.conv_cfg.cc_matrix[1][0]=0;
    mNewCfg.conv_cfg.cc_matrix[1][1]=1;
    mNewCfg.conv_cfg.cc_matrix[1][2]=0;
    mNewCfg.conv_cfg.cc_matrix[1][3]=0;

    mNewCfg.conv_cfg.cc_matrix[2][0]=0;
    mNewCfg.conv_cfg.cc_matrix[2][1]=0;
    mNewCfg.conv_cfg.cc_matrix[2][2]=1;
    mNewCfg.conv_cfg.cc_matrix[2][3]=0;

    mNewCfg.conv_cfg.cc_matrix[3][0]=0;
    mNewCfg.conv_cfg.cc_matrix[3][1]=0;
    mNewCfg.conv_cfg.cc_matrix[3][2]=0;
    mNewCfg.conv_cfg.cc_matrix[3][3]=1;

}

int PostProc::get_saved_hsic_config() {
    char property[PROPERTY_VALUE_MAX];
    int ret = FAILED;
    int32_t hue = 0, intensity = 0;
    float sat = 0, contrast = 0;

    ret = parse_pa_data(&hue, &sat, &intensity, &contrast);
    if (ret)
        return ret;

    if (gMDP5) {
        mNewCfg.pa_cfg.hue = hue;
        mNewCfg.pa_cfg.sat = sat;
        mNewCfg.pa_cfg.intensity = intensity;
        mNewCfg.pa_cfg.contrast = contrast;

        mNewCfg.pa_cfg.ops = MDP_PP_OPS_ENABLE | MDP_PP_OPS_WRITE;
    } else {
        mNewCfg.conv_cfg.hue = hue;
        mNewCfg.conv_cfg.sat = sat;
        mNewCfg.conv_cfg.intensity = intensity;
        mNewCfg.conv_cfg.contrast = contrast;

        mNewCfg.conv_cfg.ops = 1;
    }

    return 0;
}

void PostProc::copy_config() {
    memcpy(&mCurCfg, &mNewCfg, sizeof(display_pp_cfg));
}

int32_t PostProc::compare_config() {
    return memcmp(&mNewCfg, &mCurCfg, sizeof(display_pp_cfg));
}

void PostProc::print_values(const char *str, display_pp_cfg *tmp) {
    if (!tmp || !str) {
        LOGE("NULL pointer");
        return;
    }
    LOGD("----- %s -----", str);
    if (gMDP5) {
        LOGD("\t Hue: %d", tmp->pa_cfg.hue);
        LOGD("\t Saturation: %f", tmp->pa_cfg.sat);
        LOGD("\t Intensity: %d", tmp->pa_cfg.intensity);
        LOGD("\t Contrast: %f", tmp->pa_cfg.contrast);
        LOGD("\t Options: 0x%08x", tmp->pa_cfg.ops);
    } else {
        LOGD("\t Hue: %d", tmp->conv_cfg.hue);
        LOGD("\t Saturation: %f", tmp->conv_cfg.sat);
        LOGD("\t Intensity: %d", tmp->conv_cfg.intensity);
        LOGD("\t Contrast: %f", tmp->conv_cfg.contrast);
        LOGD("\t Options: 0x%08x", tmp->conv_cfg.ops);
    }
}

void *pp_poll_thread(void* data) {
    int ret = -1;
    PostProc *pp = (PostProc *)data;
    if (!pp) {
        LOGE("poll thread: NULL object of pp");
        return NULL;
    }
    /* detach the thread */
    pthread_detach(pthread_self());
    while(pp->mStatus == PP_ON) {
        usleep(500000);
        LOGE_IF(pp->mDebug, "Reading the HSIC properties");
        ret = pp->get_saved_hsic_config();
        if (ret) {
            LOGE("Failed to get the saved hsic config");
            break;
        }

        if(pp->compare_config()) {
            pp->copy_config();
            pp->print_values("HSIC params are changed, \
                                    new HSIC Params are:", &pp->mNewCfg);
            if (gMDP5) {
                if(pp->write_hsic_values())
                    LOGE("Failed to set the new hsic params");
            } else {
                if(display_pp_conv_set_cfg(pp->mBlockType, &(pp->mNewCfg.conv_cfg)))
                    LOGE("Failed to set the new hsic params");
            }
        }
    }
    return NULL;
}

int32_t PostProc::start_pp() {
    int ret = FAILED;

    ret = display_pp_init();
    if(ret != SUCCESS){
        LOGE("Failed to initialize fb");
        return ret;
    }

    if (!gMDP5) {
        init_cc_matrix();
        ret = display_pp_conv_init(mBlockType, NULL);
        if (ret) {
            LOGE("Failed to initialize pp");
            return ret;
        }
    }
    /* set status to on */
    mStatus = PP_ON;
    if (mPoll) {
        pthread_t thread_id;
        ret = pthread_create(&thread_id, NULL, pp_poll_thread, this);
        if (ret) {
            LOGE("Failed to start the pp poll thread");
            return ret;
        }
    }

    return 0;
}

int32_t PostProc::set_hsic(int32_t hue, float sat,
                                  int32_t intensity, float contrast) {
    if (gMDP5) {
        mNewCfg.pa_cfg.hue = hue;
        mNewCfg.pa_cfg.sat = sat;
        mNewCfg.pa_cfg.intensity = intensity;
        mNewCfg.pa_cfg.contrast = contrast;

        mNewCfg.pa_cfg.ops = MDP_PP_OPS_ENABLE | MDP_PP_OPS_WRITE;
    } else {
        mNewCfg.conv_cfg.hue = hue;
        mNewCfg.conv_cfg.sat = sat;
        mNewCfg.conv_cfg.intensity = intensity;
        mNewCfg.conv_cfg.contrast = contrast;
    }

    LOGE_IF(mDebug, "New HSIC Values: %d %.3f %d %.3f", hue,
                            sat, intensity, contrast);
    if (compare_config()) {
        LOGE_IF(mDebug, "HSIC Values have changed, applying new values");
        copy_config();
        int32_t err;
        if (gMDP5)
            err = write_hsic_values();
        else
            err = display_pp_conv_set_cfg(mBlockType, &(mNewCfg.conv_cfg));

        if(err) {
            LOGE("Failed to set hsic values");
            return err;
        }
    }
    return 0;
}

int32_t PostProc::stop_pp() {
    int32_t err = FAILED;
    mStatus = PP_OFF;

    err = display_pp_exit();
    if (err) {
        LOGE("Failed to close fb");
        return err;
    }
    return 0;
}
void *handler_thread(void *data) {
    DaemonContext *context = (DaemonContext *)data;
    if (!context) {
        LOGE("Context object passed to thread is null");
        return NULL;
    }
    /* Make this thread detached */
    pthread_detach(pthread_self());
    if (context->mAcceptFd < 0) {
        LOGE("No valid accepted connection fd!");
    }
    int32_t acceptFd = context->mAcceptFd;
    context->mAcceptFd = -1;
    LOGE_IF(context->mDebug, "Started the handler for acceptFd %d", acceptFd);

    while(!sigflag) {
        fd_set read_fd;
        FD_ZERO(&read_fd);
        FD_SET(acceptFd, &read_fd);
        /* selecting the time out value */
        struct timeval to;
        to.tv_sec = CONN_TIMEOUT;
        to.tv_usec = 0;
        /* wait for the connection */
        /* select() takes (max of all fds being passed) + 1 as first arg,
         * we only have one fd so passing fd + 1 */
        int32_t len = select(acceptFd+1, &read_fd, NULL, NULL, &to);
        if (len < 0) {
            LOGE("select failed: %s", strerror(errno));
            break;
        }
        if (FD_ISSET(acceptFd, &read_fd)){
            char buf[MAX_CMD_LEN];
            memset(buf, 0, sizeof(buf));
            int32_t ret = read (acceptFd, buf, sizeof(buf));
            if (ret == 0) {
                break;
            } else if (ret < 0) {
                LOGE("Unable to read the command %s", strerror(errno));
                break;
            }

            if ((context->mAD.isADEnabled() == AD_ENABLE_WB) &&
                (!strncmp(buf, CMD_AD_PREFIX, strlen(CMD_AD_PREFIX)))) {
                LOGE_IF(context->mDebug, "AD on WB FB, disabling the AD on primary FB.");
                break;
            }

            if(context->ProcessCommand(buf, len, acceptFd)) {
                LOGE("Failed to process the command!");
            }
        } else {
            LOGE("acceptFd not set!");
            break;
        }
    }
    close(acceptFd);
    return NULL;
}

void *listener_thread(void* data) {
    DaemonContext *context = (DaemonContext *)data;
    if (NULL == context) {
        LOGE("Context object passed to thread is null");
        return NULL;
    }
    LOGE_IF(context->mDebug, "Starting the listening thread");
    pthread_detach(pthread_self());
    /* wait for the connection */
    while (!sigflag) {
        struct sockaddr addr;
        socklen_t alen;
        alen = sizeof (addr);
        /* check for a new connection */
        int32_t acceptFd = accept(context->mListenFd,
                                            (struct sockaddr *) &addr, &alen);
        if (acceptFd > 0) {
            LOGE_IF(context->mDebug, "Accepted connection fd %d, \
                                                creating handler", acceptFd);
            /* creating the handler thread for this connection */
            pthread_t handler;
            while (context->mAcceptFd != -1) {
                LOGE("handler has not copied the data yet... wait some time");
                sleep(1);
            }
            context->mAcceptFd = acceptFd;
            if (pthread_create(&handler, NULL, handler_thread, context)) {
                LOGE("Failed to create the handler thread for Fd %d", acceptFd);
                break;
            }
            sched_yield();
        }
        else
            LOGE("%s: Failed to accept socket connection", strerror(errno));
    }
    LOGE("Closing the listening socket at fd %d", context->mListenFd);
    close(context->mListenFd);
    return NULL;
}

int32_t DaemonContext::reply(bool result, const int32_t& fd) {
    char buf[32];
    memset(buf, 0x00, sizeof(buf));

    if (fd < 0)
        return 0;

    if (result)
        snprintf(buf, sizeof(buf), "Success\n");
    else
        snprintf(buf, sizeof(buf), "Failure\n");
    int32_t ret = write(fd, buf, strlen(buf));
    if(ret == -1) {
        LOGE("Failed to reply back: %s", strerror(errno));
    }
    return ret;
}

int32_t DaemonContext::reply(const char *reply, const int32_t& fd) {
    if (NULL == reply) {
        LOGE("Reply string is NULL!");
        return -1;
    }

    if (fd < 0)
        return 0;

    int32_t ret = write(fd, reply, strlen(reply));
    if(ret == -1) {
        LOGE("Failed to reply back: %s", strerror(errno));
    }
    return ret;
}

int DaemonContext::ProcessCABLMsg(char* buf) {
    int32_t result = 1;

    LOGE_IF(mDebug, "PROCESS_CABL_MSG(): Command : %s", buf);
    if(!strncmp(buf, CMD_CABL_ON, strlen(CMD_CABL_ON))) {
        pthread_mutex_lock(&mCABLOpLock);
        if(bAbaEnabled){
            mABA->SetCABLFeature();
            result = mABA->CABLControl(true);
        } else {
            result = mCABL->start_cabl();
        }
        pthread_mutex_lock(&mCtrlLock);
        if (!result)
            mCtrlStatus |= cabl_bit;
        pthread_mutex_unlock(&mCtrlLock);
        pthread_mutex_unlock(&mCABLOpLock);
    } else if (!strncmp(buf, CMD_CABL_OFF, strlen(CMD_CABL_OFF))) {
        pthread_mutex_lock(&mCABLOpLock);
        if(bAbaEnabled){
            result = mABA->CABLControl(false);
        } else {
            mCABL->stop_cabl();
            result = 0;
        }
        pthread_mutex_lock(&mCtrlLock);
        mCtrlStatus &= ~cabl_bit;
        pthread_mutex_unlock(&mCtrlLock);
        pthread_mutex_unlock(&mCABLOpLock);
    } else if (!strncmp(buf, CMD_CABL_SET, strlen(CMD_CABL_SET))) {
        char *c_lvl = NULL, *tmp = NULL, *bufr;
        tmp = strtok_r(buf, " ", &bufr);
        while (tmp != NULL) {
            c_lvl = tmp;
            tmp = strtok_r(NULL, " ", &bufr);
        }
        if (c_lvl == NULL) {
            LOGE("Invalid quality level!");
            return -1;
        }
        if (!strcmp(c_lvl, CABL_LVL_AUTO)) {
            pthread_mutex_lock(&mCABLOpLock);
            if(bAbaEnabled){
                mABA->SetQualityLevel(ABL_QUALITY_HIGH);
                result = AbaSetQualityLevel(mABA->GetHandle(),
                (AbaQualityLevelType) ABL_QUALITY_HIGH);
            } else {
                pthread_mutex_lock(&mCABL->mCABLLock);
                mCABL->mPowerSaveLevel = ABL_QUALITY_HIGH;
                mCABL->mPowerLevelChange = true;
                result = 0;
                pthread_mutex_unlock(&mCABL->mCABLLock);
            }
            if (!result) {
                property_set("hw.cabl.level", c_lvl);
                if(bAbaEnabled){
                    mABA->SetDefaultQualityMode(AUTO_QL_MODE);
                } else {
                    pthread_mutex_lock(&mCABL->mCABLLock);
                    mCABL->mOEMParams.default_ql_mode = AUTO_QL_MODE;
                    pthread_mutex_unlock(&mCABL->mCABLLock);
                }
            }
            pthread_mutex_unlock(&mCABLOpLock);
        } else {
            if(bAbaEnabled){
                CABLQualityLevelType i_lvl = (CABLQualityLevelType) ql_string2int(c_lvl);
                if((int)i_lvl < 0){
                    LOGE("Invalid quality level!");
                    result = -1;
                } else {
                    if (i_lvl == mABA->GetQualityLevel()) {
                        LOGE_IF(mDebug, "ABA CABL Power level has not changed!");
                        pthread_mutex_lock(&mCABLOpLock);
                        mABA->SetDefaultQualityMode(USER_QL_MODE);
                        pthread_mutex_unlock(&mCABLOpLock);
                        result = 0;
                    } else {
                        pthread_mutex_lock(&mCABLOpLock);
                        result = AbaSetQualityLevel(mABA->GetHandle(), (AbaQualityLevelType) i_lvl);
                        if (!result) {
                            property_set("hw.cabl.level", c_lvl);
                            mABA->SetQualityLevel((CABLQualityLevelType) i_lvl);
                            mABA->SetDefaultQualityMode(USER_QL_MODE);
                        }
                        pthread_mutex_unlock(&mCABLOpLock);
                    }
                }
            } else {
                int i_lvl = ql_string2int(c_lvl);
                if(i_lvl < 0){
                    LOGE("Invalid quality level!");
                    result = -1;
                } else {
                    pthread_mutex_lock(&mCABLOpLock);
                    pthread_mutex_lock(&mCABL->mCABLLock);
                    if (i_lvl == mCABL->mPowerSaveLevel) {
                        LOGE_IF(mDebug, "CABL Power level has not changed!");
                        mCABL->mOEMParams.default_ql_mode = USER_QL_MODE;
                        result = 0;
                    } else {
                        property_set("hw.cabl.level", c_lvl);
                        mCABL->mPowerSaveLevel = i_lvl;
                        mCABL->mOEMParams.default_ql_mode = USER_QL_MODE;
                        mCABL->mPowerLevelChange = true;
                        result = 0;
                    }
                    pthread_mutex_unlock(&mCABL->mCABLLock);
                    pthread_mutex_unlock(&mCABLOpLock);
                }
            }
        }
    }
    return result;
}

int DaemonContext::ProcessADMsg(const char* buf){
    int32_t result = 1;

    LOGE_IF(mDebug, "PROCESS_AD_MSG(): Command : %s", buf);

    if (!strncmp(buf, CMD_AD_CALIB_ON, strlen(CMD_AD_CALIB_ON))) {
        pthread_mutex_lock(&mADOpLock);
        if (mAD.ADStatus() == AD_ON) {
            /* stop ad first before turning on ad_calib mode */
            result = mAD.ADControl(this, false);
            if (result) {
                pthread_mutex_unlock(&mADOpLock);
                return result;
            }
        }
        result = mAD.ADControl(this, true, ad_mode_calib, MDP_LOGICAL_BLOCK_DISP_0);
        pthread_mutex_unlock(&mADOpLock);
    } else if (!strncmp(buf, CMD_AD_CALIB_OFF, strlen(CMD_AD_CALIB_OFF))) {
        pthread_mutex_lock(&mADOpLock);
        if ( mAD.ADStatus() == AD_CALIB_ON ) {
            result = mAD.ADControl(this, false);
        }
        pthread_mutex_unlock(&mADOpLock);
    } else if (!strncmp(buf, CMD_AD_ON, strlen(CMD_AD_ON))) {
        ad_mode mode;
        int display_id = MDP_LOGICAL_BLOCK_DISP_0;
        uint32_t flag = 0;
        char c_mode[32];
        sscanf(buf, CMD_AD_ON ";" "%d" ";" "%d" ";" "%d", &mode, &display_id, &flag);
        if(!mSplitDisplay && flag != 0) {
           LOGE("Invalid input, target doesn't support AD split mode");
           return result;
        }
        pthread_mutex_lock(&mADOpLock);
        if (mAD.IsADInputValid(mode, display_id)) {
            if (mAD.ADStatus() == AD_CALIB_ON) {
            /* stop ad_calib first before turning on ad mode */
                result = mAD.ADControl(this, false);
                if (result) {
                    pthread_mutex_unlock(&mADOpLock);
                    return result;
                }
            }
            if (mAD.ADStatus() == AD_ON && (mode != mAD.ADMode() || mAD.mFlags != flag)) {
                mAD.mFlags = (MDSS_PP_SPLIT_MASK & flag);
                result = mAD.ADControl(this, false);
                if (result) {
                    pthread_mutex_unlock(&mADOpLock);
                    return result;
                }
            }
            result = mAD.ADControl(this, true, mode, display_id);
            pthread_mutex_lock(&mCtrlLock);
            if (!result) {
                snprintf(c_mode, (size_t) sizeof(c_mode), "%d", mode);
                property_set("hw.ad.mode",c_mode);
                mCtrlStatus |= ad_bit;
            }
            pthread_mutex_unlock(&mCtrlLock);
        }
        pthread_mutex_unlock(&mADOpLock);
    } else if (!strncmp(buf, CMD_AD_OFF, strlen(CMD_AD_OFF))) {
        pthread_mutex_lock(&mADOpLock);
        if (mAD.ADStatus() == AD_ON) {
            result = mAD.ADControl(this, false);
            pthread_mutex_lock(&mCtrlLock);
            if (!result)
                mCtrlStatus &= ~ad_bit;
            pthread_mutex_unlock(&mCtrlLock);
        }
        pthread_mutex_unlock(&mADOpLock);
    } else if (!strncmp(buf, CMD_AD_INIT, strlen(CMD_AD_INIT))) {
        char *params = NULL;
        params = strchr(buf, ';');
        if (params == NULL) {
            LOGE("Invalid format for input command");
        } else {
            params = params + 1;
            if (mAD.ADStatus() == AD_CALIB_ON) {
                pthread_mutex_lock(&mADOpLock);
                result = mAD.ADInit(params);

                if (mAD.mRefresher) {
                    LOGE_IF(mAD.mRefresher->mDebug, "Calling refresh()");
                    mAD.mRefresher->Refresh(1, AD_REFRESH_INTERVAL);
                }

                if (mAD.ADLastSentALSValue() >= 0)
                    mAD.ADUpdateAL(mAD.ADLastSentALSValue(), AD_REFRESH_CNT);
                pthread_mutex_unlock(&mADOpLock);
            } else {
                LOGE("AD calibration mode is not ON!!");
            }
        }
    } else if (!strncmp(buf, CMD_AD_CFG, strlen(CMD_AD_CFG))) {
        char *params = NULL;
        params = strchr(buf, ';');
        if (params == NULL) {
            LOGE("Invalid format for input command");
        } else {
            params = params + 1;
            if (mAD.ADStatus() == AD_CALIB_ON) {
                pthread_mutex_lock(&mADOpLock);
                result = mAD.ADConfig(params);

                if (mAD.mRefresher) {
                    LOGE_IF(mAD.mRefresher->mDebug, "Calling refresh()");
                    mAD.mRefresher->Refresh(1, AD_REFRESH_INTERVAL);
                }

                if (mAD.ADLastSentALSValue() >= 0)
                    mAD.ADUpdateAL(mAD.ADLastSentALSValue(), AD_REFRESH_CNT);
                pthread_mutex_unlock(&mADOpLock);
            } else {
                LOGE("AD calibration mode is not ON!!");
            }
        }
    } else if (!strncmp(buf, CMD_AD_INPUT, strlen(CMD_AD_INPUT))) {
        int32_t lux_value;
        uint32_t enableManualInput = 0;
        sscanf(buf, CMD_AD_INPUT ";" "%d" ";" "%d", &lux_value, &enableManualInput);
        if ((mAD.ADStatus() == AD_CALIB_ON) || (mAD.ADStatus() == AD_ON)) {
            pthread_mutex_lock(&mADOpLock);
            if (enableManualInput == 1) {
                mAD.ADControl(this, CONTROL_PAUSE);
                mAD.mLastManualInput = lux_value;
            } else {
                mAD.ADControl(this, CONTROL_RESUME);
            }

            result = mAD.ADUpdateAL(lux_value, AD_REFRESH_CNT);

            pthread_mutex_unlock(&mADOpLock);
        } else {
            LOGE("AD is not ON!!");
        }
    } else if (!strncmp(buf, CMD_AD_ASSERTIVENESS, strlen(CMD_AD_ASSERTIVENESS)) ||
            !strncmp(buf, CMD_AD_STRLIMIT, strlen(CMD_AD_STRLIMIT))) {
        pthread_mutex_lock(&mADOpLock);
        if (mAD.ADStatus() == AD_ON) {
            int32_t value;
            if (!strncmp(buf, CMD_AD_ASSERTIVENESS, strlen(CMD_AD_ASSERTIVENESS))) {
                sscanf(buf, CMD_AD_ASSERTIVENESS";" "%d", &value);
                if (mAD.mAssertivenessSliderValue == value) {
                    LOGE_IF(mDebug, "Input assertiveness is the same as current one!");
                    pthread_mutex_unlock(&mADOpLock);
                    return 0;
                }
                mAD.mAssertivenessSliderValue = value;
            } else if (!strncmp(buf, CMD_AD_STRLIMIT, strlen(CMD_AD_STRLIMIT))) {
                sscanf(buf, CMD_AD_STRLIMIT";" "%d", &value);
                if ((value >= AD_STRLIMT_MIN) && (value <= AD_STRLIMT_MAX)) {
                    if (mAD.ADGetStrLimit() == value) {
                        LOGE_IF(mDebug, "Strenght limit is the same as current one!");
                        pthread_mutex_unlock(&mADOpLock);
                        return 0;
                    }
                    mAD.ADSetStrLimit(value);
                } else {
                    LOGE("AD strength limit out of range!");
                    pthread_mutex_unlock(&mADOpLock);
                    return -1;
                }
            }

            result = mAD.ADSetupMode();
            if (result)
                LOGE("Failed to set the assertiveness!");

            if (mAD.mRefresher) {
                LOGE_IF(mAD.mRefresher->mDebug, "Calling refresh()");
                mAD.mRefresher->Refresh(1, AD_REFRESH_INTERVAL);
            }

            result = mAD.ADUpdateAL(mAD.ADCurrALSValue(), AD_REFRESH_CNT);

        } else
            LOGE("AD is not ON, cannot set assertiveness or strength limit");
        pthread_mutex_unlock(&mADOpLock);
    } else {
        LOGE("Unsupported AD message.");
    }

    return result;
}

int DaemonContext::ProcessSVIMsg(char* buf){
    int32_t result = 1;

    LOGE_IF(mDebug,"PROCESS_SVI_MSG(): Command : %s", buf);
    if (!strncmp(buf, CMD_SVI_ON, strlen(CMD_SVI_ON))) {
        LOGE("%s:  Command received %s", __FUNCTION__, buf);
        mABA->SetSVIFeature();

        pthread_mutex_lock(&mSVIOpLock);
        result = mABA->SVIControl(true);
        pthread_mutex_lock(&mCtrlLock);

        if (!result) {
            mCtrlStatus |= svi_bit;
        }

        pthread_mutex_unlock(&mCtrlLock);
        pthread_mutex_unlock(&mSVIOpLock);
    } else if (!strncmp(buf, CMD_SVI_OFF, strlen(CMD_SVI_OFF))) {
        pthread_mutex_lock(&mSVIOpLock);
        if (mABA->IsABAStatusON(ABA_FEATURE_SVI)) {
            LOGE("%s:  Command received %s", __FUNCTION__, buf);
            result = mABA->SVIControl(false);
            pthread_mutex_lock(&mCtrlLock);
            /*Take care of return value below*/
            AbaDisable(mABA->GetHandle(),ABA_FEATURE_SVI);
            if (!result)
                mCtrlStatus &= ~svi_bit;
            pthread_mutex_unlock(&mCtrlLock);
        }
        pthread_mutex_unlock(&mSVIOpLock);
    }
    return result;
}

void DaemonContext::ProcessControlWork() {
    LOGE_IF(mDebug, "Starting control loop");
    int ret = 0, tmp_ctrl_status = 0;
    int screenOn = screenStatus;

    pthread_mutex_lock(&mCtrlLock);
    /* assume we are holding lock when loop evaluates */
    while ((mCtrlStatus & cabl_bit) || (mCtrlStatus & svi_bit) || (mCtrlStatus & ad_bit)) {
        if (mCtrlStatus != tmp_ctrl_status) {
            if (tmp_ctrl_status & cabl_bit && !(mCtrlStatus & cabl_bit)){
                if (bAbaEnabled){
                    mABA->Notify(NOTIFY_TYPE_SUSPEND);
                } else {
                    mCABL->Notify(NOTIFY_TYPE_SUSPEND);
                }
            }
            else if (!(tmp_ctrl_status & cabl_bit) && (mCtrlStatus & cabl_bit)
                                                        && screenStatus == 1) {
                if (bAbaEnabled){
                    mABA->Notify(NOTIFY_TYPE_UPDATE);
                } else {
                    mCABL->Notify(NOTIFY_TYPE_UPDATE);
                }
            }
            if (tmp_ctrl_status & svi_bit && !(mCtrlStatus & svi_bit)){
                if(bAbaEnabled)
                    mABA->Notify(NOTIFY_TYPE_SUSPEND);
            }
            else if (!(tmp_ctrl_status & svi_bit) && (mCtrlStatus & svi_bit)
                                                        && screenStatus == 1) {
                if (bAbaEnabled)
                    mABA->Notify(NOTIFY_TYPE_UPDATE);
            }
            if (tmp_ctrl_status & ad_bit && !(mCtrlStatus & ad_bit))
                mAD.Notify(NOTIFY_TYPE_SUSPEND);
            else if (!(tmp_ctrl_status & ad_bit) && (mCtrlStatus & ad_bit)
                                                        && screenOn == 1) {
                mAD.Notify(NOTIFY_TYPE_UPDATE);
            }
            tmp_ctrl_status = mCtrlStatus;
        }

        pthread_mutex_unlock(&mCtrlLock);
        uint32_t update_notify;
        if (screenStatus == 0) {
            update_notify = NOTIFY_UPDATE_START;
            ret =  ioctl(FBFd, MSMFB_NOTIFY_UPDATE, &update_notify);
            int tmp_notify = update_notify;
            if (ret != 0) {
                if (errno != ETIMEDOUT)
                    LOGE("MSMFB_NOTIFY_UPDATE start ioctl failed");
                pthread_mutex_lock(&mCtrlLock);
                continue;
            }
            if ((tmp_ctrl_status & cabl_bit) || (tmp_ctrl_status & svi_bit)){
                if(bAbaEnabled){
                    if(mABA->Notify(tmp_notify) == -1){
                        pthread_mutex_lock(&mCtrlLock);
                        continue;
                    }
                } else {
                   mCABL->Notify(tmp_notify);
                }
            }
            if (tmp_notify == NOTIFY_TYPE_UPDATE) {
                screenStatus = 1;
                screenOn = 1;
            }
            if (tmp_ctrl_status & ad_bit)
                mAD.Notify(tmp_notify);
        }
        else if (screenStatus == 1) {
            update_notify = NOTIFY_UPDATE_STOP;
            ret =  ioctl(FBFd, MSMFB_NOTIFY_UPDATE, &update_notify);
            int tmp_notify = update_notify;
            if (ret != 0) {
                if (errno != ETIMEDOUT)
                    LOGE("MSMFB_NOTIFY_UPDATE stop ioctl failed");
                pthread_mutex_lock(&mCtrlLock);
                continue;
            }
            screenStatus = 0;
            if (tmp_notify == NOTIFY_TYPE_SUSPEND){
                screenOn = 0;
            }
            if (tmp_ctrl_status & cabl_bit){
                if(bAbaEnabled){
                    mABA->Notify(tmp_notify);
                } else {
                    mCABL->Notify(tmp_notify);
                }
            }
            else if (tmp_ctrl_status & svi_bit) {
                if(bAbaEnabled)
                    mABA->Notify(tmp_notify);
            }
            if (tmp_ctrl_status & ad_bit)
                mAD.Notify(tmp_notify);
        }
        pthread_mutex_lock(&mCtrlLock);
    }
    if (mCtrlStatus != tmp_ctrl_status) {
        if (tmp_ctrl_status & cabl_bit && !(mCtrlStatus & cabl_bit)){
            if (bAbaEnabled){
                mABA->Notify(NOTIFY_TYPE_SUSPEND);
            } else {
                mCABL->Notify(NOTIFY_TYPE_SUSPEND);
            }
        }
        if (tmp_ctrl_status & ad_bit && !(mCtrlStatus & ad_bit))
            mAD.Notify(NOTIFY_TYPE_SUSPEND);
        tmp_ctrl_status = mCtrlStatus;
    }
    pthread_mutex_unlock(&mCtrlLock);
    return;
}

int DaemonContext::ProcessDebugMsg(char* buf){
    LOGE_IF(mDebug, "ProcessDebugMsg(): Command : %s", buf);
    LOGE("ProcessDebugMsg(): Command : %s", buf);

    if (!strncmp(buf, CMD_DEBUG_DAEMON_ON, strlen(CMD_DEBUG_DAEMON_ON))) {
        mDebug = true;
    } else if (!strncmp(buf, CMD_DEBUG_DAEMON_OFF, strlen(CMD_DEBUG_DAEMON_OFF))) {
        mDebug = false;
    } else if (!strncmp(buf, CMD_DEBUG_CABL_ON, strlen(CMD_DEBUG_CABL_ON))) {
        if(bAbaEnabled){
            mABA->SetDebugLevel(true);
        } else {
            mCABL->mDebug = true;
        }
    } else if (!strncmp(buf, CMD_DEBUG_CABL_OFF, strlen(CMD_DEBUG_CABL_OFF))) {
        if(bAbaEnabled){
            mABA->SetDebugLevel(false);
        } else {
            mCABL->mDebug = false;
        }

    } else if (!strncmp(buf, CMD_DEBUG_AD_ON, strlen(CMD_DEBUG_AD_ON))) {
        mAD.mDebug = true;
    } else if (!strncmp(buf, CMD_DEBUG_AD_OFF, strlen(CMD_DEBUG_AD_OFF))) {
        mAD.mDebug = false;
    } else if (!strncmp(buf, CMD_DEBUG_PP_ON, strlen(CMD_DEBUG_PP_ON))) {
        mPostProc.mDebug = true;
    } else if (!strncmp(buf, CMD_DEBUG_PP_OFF, strlen(CMD_DEBUG_PP_OFF))) {
        mPostProc.mDebug = false;
    } else {
        LOGE("Unsupported debug message.");
        return -1;
    }
    return 0;
}

void DaemonContext::ProcessADPollWork() {
    int32_t ret = -1, index = -1;
    char buffer[AD_FILE_LEN + 1] = {0};
    char ADFilePath[MAX_FB_NAME_LEN];
    char ad_cmd[SHORT_CMD_LEN];
    bool ad_enabled = false;
    struct pollfd ad_poll_fd;
    LOGE_IF(mDebug, "%s() Entering", __func__);

    ret = SelectFB(MDP_LOGICAL_BLOCK_DISP_2, &index);
    if (ret == 0 && index >= 0) {
        memset(buffer, 0, sizeof(buffer));
        memset(ADFilePath, 0, sizeof(ADFilePath));
        snprintf(ADFilePath, sizeof(ADFilePath), "/sys/class/graphics/fb%d/ad", index);
        LOGE_IF(mDebug, "Polling on %s", ADFilePath);
        ad_fd = open(ADFilePath, O_RDONLY);
        if (ad_fd < 0) {
            LOGE("Unable to open fd%d/ad node  err:  %s", index, strerror(errno));
            return;
        }
    }

    ad_poll_fd.fd = ad_fd; //file descriptor
    ad_poll_fd.events = POLLPRI | POLLERR; //requested events
    while (true) {
        if ((ret = poll(&ad_poll_fd, 1, -1)) < 0) {//infinite timeout
            LOGE("Error in polling AD node: %s.", strerror(errno));
            break;
        } else {
            if (ad_poll_fd.revents & POLLPRI) {
                memset(buffer, 0, AD_FILE_LEN);
                ret = pread(ad_fd, buffer, AD_FILE_LEN, 0);
                if (ret > 0) {
                    buffer[AD_FILE_LEN] = '\0';
                    ad_enabled = atoi(&buffer[0]) == 1;
                    LOGE_IF(mDebug, "Requested AD Status from AD node: %d", ad_enabled);
                    //turn on/off ad according to the value of ad_enabled
                    memset(ad_cmd, 0, sizeof(ad_cmd));
                    if (ad_enabled)
                        snprintf(ad_cmd, SHORT_CMD_LEN, "%s;%d;%d",
                                CMD_AD_ON, ad_mode_auto_str, MDP_LOGICAL_BLOCK_DISP_2);
                    else
                        snprintf(ad_cmd, SHORT_CMD_LEN, "%s", CMD_AD_OFF);
                    ret = ProcessCommand(ad_cmd, SHORT_CMD_LEN, -1);
                    if (ret)
                        LOGE("Unable to process command for AD, ret = %d", ret);

                } else if (ret == 0) {
                    LOGE_IF(mDebug, "No data to read from AD node.");
                } else {
                    LOGE("Unable to read AD node, %s", strerror(errno));
               }
            }
        }
    }

    LOGE_IF(mDebug, "Closing the ad node.");
    close(ad_fd);

    LOGE_IF(mDebug, "%s() Exiting", __func__);
    return;
}

int32_t DaemonContext::ProcessCommand(char *buf, const int32_t len, const int32_t& fd) {
    int result = 0;
    int ret = 0;

    if (NULL == buf) {
        LOGE("Command string is NULL!");
        result = -1;
        return result;
    }

    LOGE_IF(mDebug, "Command received: %s ", buf);
    if (!strncmp(buf, CMD_SET, strlen(CMD_SET))) {
        result = ProcessSetMsg(&buf[0]);
        reply(!result, fd);
    } else if (!strncmp(buf, CMD_DCM_ON, strlen(CMD_DCM_ON))) {
        result = -1;
        if(!mDCM)
            mDCM = new DCM();
        if (mDCM) {
            result = mDCM->DCMControl(true);
        }
        reply(!result, fd);
    } else if (!strncmp(buf, CMD_DCM_OFF, strlen(CMD_DCM_OFF))) {
        if (mDCM && (mDCM->mStatus == DCM_ON)) {
            result = mDCM->DCMControl(false);
            delete mDCM;
            mDCM = NULL;
        } else if (!mDCM) {
            LOGE("DCM is already off !");
            result = 0;
        } else {
            result = -1;
        }

        reply(!result, fd);
    } else if (!strncmp(buf, CMD_PP_ON, strlen(CMD_PP_ON))) {
        pthread_mutex_lock(&mPostProcOpLock);
        result = mPostProc.start_pp();
        pthread_mutex_unlock(&mPostProcOpLock);
        reply(!result, fd);
    } else if (!strncmp(buf, CMD_PP_OFF, strlen(CMD_PP_OFF))) {
        pthread_mutex_lock(&mPostProcOpLock);
        mPostProc.stop_pp();
        pthread_mutex_unlock(&mPostProcOpLock);
        reply(true, fd);
    } else if (!strncmp(buf, CMD_PP_SET_HSIC, strlen(CMD_PP_SET_HSIC))) {
        int32_t hue, intensity;
        float sat, contrast;
        /* start the postproc module */
        sscanf(buf, CMD_PP_SET_HSIC "%d %f %d %f", &hue,
                &sat, &intensity, &contrast);
        pthread_mutex_lock(&mPostProcOpLock);
        mPostProc.start_pp();
        result = mPostProc.set_hsic(hue, sat, intensity, contrast);
        if (result){
            LOGE("Failed to set PA data");
            pthread_mutex_unlock(&mPostProcOpLock);
            reply(!result, fd);
            return result;
        }

        result = mPostProc.save_pa_data(hue, sat, intensity, contrast);
        if (result){
            LOGE("Failed to save PA data");
        }

        pthread_mutex_unlock(&mPostProcOpLock);
        reply(!result, fd);
    } else if (!strncmp(buf, CMD_POSTPROC_STATUS, strlen(CMD_POSTPROC_STATUS))) {
        char buf[32];
        memset(buf, 0x00, sizeof(buf));
        if (mPostProc.mStatus == PP_ON)
            snprintf(buf, sizeof(buf), "Running\n");
        else
            snprintf(buf, sizeof(buf), "Stopped\n");
        reply(buf, fd);
    } else if (!strncmp(buf, CMD_CABL_STATUS, strlen(CMD_CABL_STATUS))) {
        char status[32];
        memset(status, 0x00, sizeof(status));
        if (bAbaEnabled){
            if (mABA && mABA->IsABAStatusON(ABA_FEATURE_CABL))
                snprintf(status, sizeof(status), "running\n");
            else
                snprintf(status, sizeof(status), "stopped\n");
        } else {
            if (mCABL && mCABL->eStatus == CABL_ON)
                snprintf(status, sizeof(status), "running\n");
            else
                snprintf(status, sizeof(status), "stopped\n");
        }
        reply(status, fd);
    } else if (!strncmp(buf, CMD_CABL_PREFIX, strlen(CMD_CABL_PREFIX))) {
        StartAlgorithmObjects();
        result = ProcessCABLMsg(&buf[0]);
        reply(!result, fd);
    } else if (!strncmp(buf, CMD_SVI_PREFIX, strlen(CMD_SVI_PREFIX))) {
        StartAlgorithmObjects();
        if(bAbaEnabled)
            result = ProcessSVIMsg(&buf[0]);
        else
            result = -1;
        reply(!result, fd);
    } else if (!strncmp(buf, CMD_AD_STATUS, strlen(CMD_AD_STATUS))) {
        char ad_status[32];
        memset(ad_status, 0x00, sizeof(ad_status));
        int def_status = 0, def_mode = -1;
        pthread_mutex_lock(&mADOpLock);
        def_status = mAD.ADStatus();
        if (def_status == AD_ON || def_status == AD_CALIB_ON)
            def_mode = mAD.ADMode();
        pthread_mutex_unlock(&mADOpLock);
        snprintf(ad_status, sizeof(ad_status), "%d;%d", def_status, def_mode);
        reply(ad_status, fd);
    } else if (!strncmp(buf, CMD_AD_PREFIX, strlen(CMD_AD_PREFIX))) {
        result = ProcessADMsg(&buf[0]);
        reply(!result, fd);
    } else if (!strncmp(buf, CMD_OEM_PREFIX, strlen(CMD_OEM_PREFIX))) {
        if (!strncmp(buf, CMD_OEM_GET_PROFILES, strlen(CMD_OEM_GET_PROFILES))) {
            int32_t numProfiles = featureList.getDCCNumberOfProfiles();
            if (numProfiles <= 0) {
                LOGE("Failed to get profiles / No profiles!");
                reply(false, fd);
            }
            uint32_t listSize = ((numProfiles + 1) * LABEL_SIZE) + 1;
            char *list = (char *)calloc(listSize, 1);
            if(!featureList.getDCCProfiles(list, listSize))
                reply(list, fd);
            else {
                LOGE("Failed to get the profiles!");
                reply(false, fd);
            }
            free(list);
        }
        else if (!strncmp(buf, CMD_OEM_SET_PROFILE,
                    strlen(CMD_OEM_SET_PROFILE))) {
            const char *profileName = buf + strlen(CMD_OEM_SET_PROFILE) + 1;
            pthread_mutex_lock(&mPostProcOpLock);
            mPostProc.start_pp();
            result = featureList.setDCCProfile(profileName);
            pthread_mutex_unlock(&mPostProcOpLock);
            reply(!result, fd);
        }
        else {
            /* pass the command to the OEM module */
            pp_oem_message_handler(buf, len, fd);
        }
    } else if (!strncmp(buf, CMD_BL_SET, strlen(CMD_BL_SET))) {
        int32_t backlight;
        sscanf(buf, CMD_BL_SET ";" "%d", &backlight);
        if (mAD.ADStatus() == AD_CALIB_ON) {
            pthread_mutex_lock(&mADOpLock);
            result = mAD.ADSetCalibBL(backlight);
            pthread_mutex_unlock(&mADOpLock);
        } else {
            LOGE("AD calib is not on, start AD calib first!");
        }
        reply(!result, fd);
    } else if (!strncmp(buf, CMD_DEBUG_PREFIX, strlen(CMD_DEBUG_PREFIX))) {
        result = ProcessDebugMsg(&buf[0]);
        reply(!result, fd);
    } else {
        LOGE("Unknown command for pp daemon: %s", buf);
        result = -1;
    }
    ret = result;
    pthread_mutex_lock(&mCtrlLock);
    if (!result && !(mCtrlStatus & ctrl_bit) &&
        ((mCtrlStatus & cabl_bit) || (mCtrlStatus & svi_bit) || (mCtrlStatus & ad_bit))) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        ret = pthread_create(&mControlThrdId, &attr, &DaemonContext::control_thrd_func, this);
        if (ret)
            LOGE("Failed to start the control thread, status = %d", ret);
        else
            mCtrlStatus |= ctrl_bit;
        pthread_attr_destroy(&attr);
    }
    if (!result && (mCtrlStatus & ctrl_bit) &&
        !(mCtrlStatus & cabl_bit) && !(mCtrlStatus & svi_bit) && !(mCtrlStatus & ad_bit)) {
            void *term;
            pthread_mutex_unlock(&mCtrlLock);
            pthread_join(mControlThrdId, &term);
            pthread_mutex_lock(&mCtrlLock);
            LOGE_IF(mDebug, "control thread terminated");
            mCtrlStatus &= ~ctrl_bit;
    }
    pthread_mutex_unlock(&mCtrlLock);
    return ret;
}

int32_t DaemonContext::getListenFd() {
    /* trying to open a socket */
    mListenFd = android_get_control_socket(DAEMON_SOCKET);
    if (mListenFd < 0) {
        LOGE("Obtaining listener socket %s failed", DAEMON_SOCKET);
        return -1;
    }
    LOGE_IF(mDebug, "Acquired the %s socket", DAEMON_SOCKET);
    /* listen to the opened socket */
    if (listen(mListenFd, mNumConnect) < 0) {
        LOGE("Unable to listen on fd '%d' for socket %s",
                                  mListenFd, DAEMON_SOCKET);
        return -1;
    }
    return 0;
}

void DaemonContext::StartAlgorithmObjects() {

    if(mABA || mCABL)
        return;

    if (2 == display_pp_cabl_supported()){
        mABA = new AbaContext();
        mABA->initHW();
    } else if ((1 == display_pp_cabl_supported()) || (mBootStartCABL)){
        mCABL = new CABL();
        mCABL->initHW();
    }
}

void DaemonContext::StopAlgorithmObjects() {
    if ((bAbaEnabled) && (mABA)) {
        delete mABA;
    } else if (mCABL){
        delete mCABL;
    }
}

int DaemonContext::SelectFB(int display_id, int *fb_idx) {
    int ret = -1, index = -1, i = 0, j = 0;
    int fd_type[TOTAL_FB_NUM] = {-1, -1, -1};
    char fb_type[MAX_FB_NAME_LEN];
    char msmFbTypePath[MAX_FB_NAME_LEN];
    const char* logical_display_0[PRIMARY_PANEL_TYPE_CNT] = {HDMI_PANEL,
        LVDS_PANEL, MIPI_DSI_VIDEO_PANEL, MIPI_DSI_CMD_PANEL, EDP_PANEL};
    const char* logical_display_1[EXTERNAL_PANEL_TYPE_CNT] = {DTV_PANEL};
    const char* logical_display_2[WRITEBACK_PANEL_TYPE_CNT] = {WB_PANEL};

    for (i = 0; i < TOTAL_FB_NUM; i++) {
        memset(fb_type, 0, sizeof(fb_type));
        snprintf(msmFbTypePath, sizeof(msmFbTypePath),
        "/sys/class/graphics/fb%d/msm_fb_type", i);
        fd_type[i] = open(msmFbTypePath, O_RDONLY);
        if (fd_type[i] >= 0 ) {
            ret = read(fd_type[i], fb_type, sizeof(fb_type));
            if (ret == 0) {
                continue;
            } else if (ret < 0) {
                ret = errno;
                LOGE("Unable to read fb type file at fd_type[%d], err:  %s", i, strerror(errno));
                break;
            }

            if (display_id == MDP_LOGICAL_BLOCK_DISP_0) {
                for(j = 0; j < PRIMARY_PANEL_TYPE_CNT; j++) {
                    if(!strncmp(fb_type, logical_display_0[j] , strlen(logical_display_0[j]))) {
                        index = i;
                        ret = 0;
                        goto exit;
                    }
                }
            } else if (display_id == MDP_LOGICAL_BLOCK_DISP_1) {
                for(j = 0; j < EXTERNAL_PANEL_TYPE_CNT; j++) {
                    if(!strncmp(fb_type, logical_display_1[j], strlen(logical_display_1[j]))) {
                        index = i;
                        ret = 0;
                        goto exit;
                    }
                }
            } else if (display_id == MDP_LOGICAL_BLOCK_DISP_2) {
                for(j = 0; j < WRITEBACK_PANEL_TYPE_CNT; j++) {
                    if(!strncmp(fb_type, logical_display_2[j], strlen(logical_display_2[j]))) {
                        index = i;
                        ret = 0;
                        goto exit;
                    }
                }
            } else {
                LOGE("Unsupported display_id %d", display_id);
                ret = -1;
                break;
            }
        } else {
            LOGE("Unable to open fb type file  err:  %s", strerror(errno));
        }
    }

exit:
    for (i = 0; i < TOTAL_FB_NUM; i++) {
        if (fd_type[i] >= 0) {
            close(fd_type[i]);
        }
    }

    *fb_idx = index;
    return ret;
}

bool DaemonContext::IsSplitDisplay(int fb_fd) {
    struct fb_var_screeninfo info;
    struct msmfb_mdp_pp pp;

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &info) == -1) {
        LOGE("Error getting FB Info : %s", strerror(errno));
        return false;
    }

    if (info.xres > qdutils::MAX_DISPLAY_DIM && (qdutils::MDPVersion::getInstance().is8084()))
        mSplitDisplay = true;
    return mSplitDisplay;
}

int32_t DaemonContext::start() {
    /* start listening to the socket for connections*/
    int32_t err = getListenFd();
    if (err) {
        LOGE("Failed to listen to the socket");
        return err;
    }
    pthread_t thread_id;
    /* start the listener thread to handle connection requests */
    err = pthread_create(&thread_id, NULL, listener_thread, this);
    if (err) {
        LOGE("Failed to start the listener thread");
        return err;
    }

    /* start pp polling thread at boot up if polling property is set */
    if (mPostProc.mPoll)
        mPostProc.start_pp();

    /* start AD polling thread, which checks the content of /ad file */
    if (mAD.isADEnabled() == AD_ENABLE_WB) {
        pthread_attr_t ad_attr;
        pthread_attr_init(&ad_attr);
        LOGE_IF(mDebug, "Creating ad poll thread");
        err = pthread_create(&mADPollThrdId, &ad_attr, &ad_poll_thrd_func, this);
        if (err)
            LOGE("Failed to start the AD poll thread, err = %d", err);
    } else {
        LOGE_IF(mDebug, "AD on WB FB is not supported");
    }

    return 0;
}

void AD::ProcessADWork() {
    int ret = -1, tmp_ALS = 0;

    if(mADSupported ) {
        ret = ADSetupMode();
        if(ret) {
            LOGE("%s(): ADSetupMode() Failed : Return Value = %d",__func__, ret);
            goto exit_ad_off;
        } else  {
            LOGE_IF(mDebug, "ADSetup successful");
        }

        ret = mLightSensor->ALSRegister();
        bIsFirstRun = true;
        if( ret ) {
            LOGE("%s(): ALSRegister() Failed : Return Value = %d",__func__, ret);
            goto exit_ad_off;
        } else {
            LOGE_IF(mDebug, "ALSRegister successful");
        }

        pthread_mutex_lock(&mADLock);
        mStatus = AD_ON;
        pthread_mutex_unlock(&mADLock);

        pthread_mutex_lock(&mADLock);
        while(AD_ON == mStatus) {

            if (mADUpdateType != NOTIFY_TYPE_UPDATE) {
                bIsFirstRun = true;
                pthread_cond_wait(&mADCond, &mADLock);
            }
            pthread_mutex_unlock(&mADLock);

            if (mRefresher) {
                LOGE_IF(mDebug, "Calling mRefresher->control(true)");
                mRefresher->Control(1);
            }

            ret = mLightSensor->ALSRun(bIsFirstRun);
            if(ret) {
                LOGE("%s(): ALSRun() Failed : Return Value = %d",__func__, ret);
                goto exit_cleanup;
            } else {
                LOGE_IF(mDebug, "ALSRun successful");
            }

            if (bIsFirstRun)
                if (mLightSensor->mALSPaused)
                    ADUpdateAL(mLastManualInput, AD_REFRESH_CNT);
                else
                    ADUpdateAL(START_ALS_VALUE, AD_REFRESH_CNT);

            pthread_mutex_lock(&mADLock);
            while(AD_ON == mStatus) {
                if ( mADUpdateType != NOTIFY_TYPE_UPDATE){
                    break;
                }
                pthread_mutex_unlock(&mADLock);
                tmp_ALS = mLightSensor->ALSReadSensor();
                mALSValue = (tmp_ALS > 0) ? tmp_ALS : 1;

                if(fabsf(log(mALSValue) - log(mPrevALSValue)) > AMBIENT_LIGHT_THR || bIsFirstRun){
                    mPrevALSValue = mALSValue;
                    ADUpdateAL(mALSValue, AD_REFRESH_CNT);
                }

                pthread_mutex_lock(&mADLock);
                if (bIsFirstRun) {
                    bIsFirstRun = false;
                    break;
                }
            }
            pthread_mutex_unlock(&mADLock);

            if (mRefresher) {
                LOGE_IF(mDebug, "Calling control(false)");
                mRefresher->Control(0);
            }

            ret = mLightSensor->ALSDeRegister();
            if (ret) {
                LOGE("%s(): ALSDeRegister() Failed : Return Value = %d",__func__, ret);
                goto exit_cleanup;
            }

            pthread_mutex_lock(&mADLock);
        }
        pthread_mutex_unlock(&mADLock);

        ret = mLightSensor->ALSCleanup();
        if (ret) {
            LOGE("%s(): ALSCleanup() Failed : Return Value = %d",__func__, ret);
            return;
        }

        ret = ADCleanup();
        if (ret) {
            LOGE("%s(): ADCleanup() Failed : Return Value = %d",__func__, ret);
            return;
        }

    }
    return;
exit_dereg:
    mLightSensor->ALSDeRegister();
exit_cleanup:
    mLightSensor->ALSCleanup();
exit_ad_off:
    if (mDisplayFd > 0) {
        close(mDisplayFd);
        mDisplayFd = -1;
    }
    mStatus = AD_OFF;
    return;

}

int AD::ADSetupMode() {
    int ret = -1;
    struct msmfb_mdp_pp pp;
    struct fb_var_screeninfo info;
    struct mdss_ad_init *init = &pp.data.ad_init_cfg.params.init;

    if(mADSupported) {
        mADCalibStatus = 0;

        pp.op = mdp_op_ad_cfg;
        pp.data.ad_init_cfg.ops = MDP_PP_OPS_ENABLE | MDP_PP_AD_INIT | mFlags;
        pp.data.ad_init_cfg.params.init = mDefADConfig.init;

        if (ioctl(mDisplayFd, FBIOGET_VSCREENINFO, &info) == -1) {
            LOGE("Error getting FB Info : %s", strerror(errno));
            ret = errno;
            return ret;
        }
        init->frame_w = info.xres;
        init->frame_h = info.yres;

        LOGE_IF(mDebug, "Calling ioctl for AD init!");
        if (ioctl(mDisplayFd, MSMFB_MDP_PP, &pp)) {
            LOGE("ad init ioctl failed, err: %s", strerror(errno));
            ret = errno;
        } else {
            mADCalibStatus |= CALIB_INIT;
            ret = 0;
        }

        pp.op = mdp_op_ad_cfg;
        pp.data.ad_init_cfg.ops = MDP_PP_OPS_ENABLE | MDP_PP_AD_CFG | mFlags;
        pp.data.ad_init_cfg.params.cfg = mDefADConfig.cfg;
        pp.data.ad_init_cfg.params.cfg.mode = mMode;

        LOGE_IF(mDebug, "Calling ioctl for AD config!");

        //set the ad_cfg_calib if assertiveness is valid
        ret = ADCalcCalib();
        if (ret == SUCCESS) {
            pp.data.ad_init_cfg.params.cfg.calib[0] = mAD_calib_a;
            pp.data.ad_init_cfg.params.cfg.calib[2] = mAD_calib_c;
            pp.data.ad_init_cfg.params.cfg.calib[3] = mAD_calib_d;
        }

        if (ioctl(mDisplayFd, MSMFB_MDP_PP, &pp)) {
            LOGE("ad config ioctl failed, err: %s", strerror(errno));
            ret = errno;
        } else {
            mADCalibStatus |= CALIB_CFG;
            ret = 0;
        }

        LOGE_IF(mDebug, "%s: Calibration status = %d",__func__, mADCalibStatus);
    }
    return ret;
}

int AD::ADCleanup() {
    int ret = -1;
    struct msmfb_mdp_pp pp;

    if(mADSupported) {
        pp.op = mdp_op_ad_cfg;
        pp.data.ad_init_cfg.ops = MDP_PP_OPS_DISABLE | MDP_PP_AD_CFG;
        pp.data.ad_init_cfg.params.cfg = mDefADConfig.cfg;
        pp.data.ad_init_cfg.params.cfg.bl_ctrl_mode = MDSS_AD_BL_CTRL_MODE_DIS;

        LOGE_IF(mDebug, "%s:Calling ioctl for AD config!",__func__);
        if (ioctl(mDisplayFd, MSMFB_MDP_PP, &pp)) {
            LOGE("ad config ioctl failed, err: %s", strerror(errno));
            ret = errno;
        } else {
            mADCalibStatus = 0;
            ret = 0;
        }

        LOGE_IF(mDebug, "%s: Cleanup status = %d",__func__, ret);
    }
    return ret;
}

int AD::ADSetCalibMode(int mode) {
    int ret = 0;
    struct msmfb_mdp_pp pp;
    struct mdss_calib_cfg *calib = &pp.data.mdss_calib_cfg;

    if(mADSupported) {
        pp.op = mdp_op_calib_mode;
        calib->ops = MDP_PP_OPS_ENABLE;
        calib->calib_mask = mode;

        LOGE_IF(mDebug, "Calling mdss_calib_cfg ioctl!");
        if (ioctl(mDisplayFd, MSMFB_MDP_PP, &pp)) {
            LOGE("mdss_calib_cfg ioctl failed, err: %s", strerror(errno));
            ret = errno;
        }
    }
    return ret;
}

int AD::ADSetCalibBL(int bl) {
    int ret = 0;
    struct msmfb_mdp_pp pp;
    struct mdss_ad_input *input = &pp.data.ad_input;

    if(mADSupported) {
        pp.op = mdp_op_ad_input;
        input->mode = MDSS_AD_MODE_CALIB;
        input->in.calib_bl = bl;

        LOGE_IF(mDebug, "Calling ad_set_calib_bl ioctl!");
        if (ioctl(mDisplayFd, MSMFB_MDP_PP, &pp)) {
            LOGE("ad_set_calib_bl ioctl failed, err: %s", strerror(errno));
            ret = errno;
        }
    }
    return ret;
}


int AD::ADControl(DaemonContext *ctx, int enableBit, ad_mode mode, int display_id) {
    int ret = -1;
    void *term;
    int index = -1;

    if(mADSupported) {
        LOGE_IF(mDebug, "%s: Entering", __func__);

        if (enableBit == CONTROL_PAUSE) {
            ret = mLightSensor->ALSPauseControl(true);
        } else if (enableBit == CONTROL_RESUME) {
            ret = mLightSensor->ALSPauseControl(false);
        } else if (enableBit == CONTROL_ENABLE) {
            if (mStatus == AD_ON ) {
                LOGE_IF(mDebug, "AD is already on !");
                return ret;
            } else if (mStatus == AD_CALIB_ON) {
                LOGE_IF(mDebug, "AD calibration is in progress!");
                return ret;
            }

            if (mEnable) {
                if (mDisplayFd >= 0) {
                    LOGE("Unexpected state, mDisplayFd should have been closed, mDisplayFd = %d", mDisplayFd);
                    return ret;
                }

                ret = ctx->SelectFB(display_id, &index);
                if (ret || index < 0) {
                    LOGE("Cannot identify framebuffer for display = %d, index = %d", display_id, index);
                    return ret;
                } else {
                    char FbPath[MAX_FB_NAME_LEN];
                    snprintf(FbPath, sizeof(FbPath), "%s%d", FRAMEBUFFER_NODE, index);
                    mDisplayFd = open(FbPath, O_RDWR);
                    if (mDisplayFd < 0) {
                        LOGE("Cannot open framebuffer, err: %s", strerror(errno));
                        ret = errno;
                        return ret;
                    }
                }

                switch (mode) {
                    case ad_mode_auto_bl:
                    case ad_mode_auto_str:
                        //Parse calibrationd data file
                        ret = ADParseCalibData(&mDefADConfig);
                        if (ret) {
                            LOGE("%s(): parsing ad calib file failed with ret = %d", __func__, ret);
                            break;
                        }

                        if (mode != ad_mode_auto_bl)
                            mDefADConfig.cfg.bl_ctrl_mode = MDSS_AD_BL_CTRL_MODE_DIS;
                        else {
                            mDefADConfig.cfg.bl_ctrl_mode = MDSS_AD_BL_CTRL_MODE_EN;
                            mPrevBL = get_backlight_level();
                            LOGE_IF(mDebug, "Backlight before entering AD auto bl mode: %d.", mPrevBL);
                        }

                        if (mDebug)
                            ret = ADPrintCalibData(&mDefADConfig);

                        ret = ADRun(mode);
                        if (ret) {
                            LOGE("%s: ADRun() failed, ret = %d", __func__, ret);
                        }
                        break;
                    case ad_mode_calib:
                        ret = ADSetCalibMode(1);
                        mStatus = AD_CALIB_ON;
                        mADCalibStatus = 0;
                        break;

                    default:
                        break;
                }

                if (ret) {
                    LOGE("%s: Could not enable AD mode = %d, ret = %d, closing display Fd!!", __func__, mode, ret);
                    close(mDisplayFd);
                    mDisplayFd = -1;
                } else if (mRefresher) {
                    LOGE_IF(mRefresher->mDebug, "Calling mRefresher->control(true)");
                    mRefresher->Control(1);
                }
            } else {
                LOGE("Assertive display is not enabled!");
            }
        } else if (enableBit == CONTROL_DISABLE) {
            if (mStatus == AD_OFF) {
                LOGE("AD is already off!");
                return ret;
            } else if (mStatus == AD_CALIB_OFF) {
                LOGE_IF(mDebug, "AD calibration is already off!");
                return ret;
            }

            if (mStatus == AD_ON) {
                mStatus = AD_OFF;

                pthread_mutex_lock(&mADLock);
                pthread_cond_signal(&mADCond);
                pthread_mutex_unlock(&mADLock);

                pthread_mutex_lock(&mLightSensor->mALSLock);
                pthread_cond_signal(&mLightSensor->mALSCond);
                pthread_mutex_unlock(&mLightSensor->mALSLock);

                ret = pthread_join(mADThread, &term);
                if (mMode == ad_mode_auto_bl) {
                    /* reset the backlight scale when exiting mode 0*/
                    LOGE_IF(mDebug, "Reset backlight to %d.", mPrevBL);
                    ret = set_backlight_level(mPrevBL);
                    if (ret)
                        LOGE("%s(): Failed to reset the backlight", __func__);
                }

            } else if (mStatus == AD_CALIB_ON) {
                ret = ADSetCalibMode(0);
                ret = ADCleanup();
                if( ret ) {
                    LOGE("%s(): Failed : Return Value = %d",__func__, ret);
                    return ret;
                }
                mStatus = AD_CALIB_OFF;
                ret = 0;
            } else {
                    LOGE("Unexpected state, mStatus = %d, mDisplayFd = %d", mStatus, mDisplayFd);
            }

            if (mRefresher && (mStatus == AD_CALIB_OFF || mStatus == AD_OFF)) {
                    LOGE_IF(mRefresher->mDebug, "Calling mRefresher->control(true)");
                    mRefresher->Control(0);
            }

            if (mDisplayFd < 0) {
                LOGE("Unexpected state, invalid mDisplayFd, mDisplayFd = %d", mDisplayFd);
            } else {
                close(mDisplayFd);
            }

            mDisplayFd = -1;

        }
    }
    return ret;
}

int AD::ADRun(ad_mode mode) {
    int ret = -1;

    if(mADSupported) {
        mMode = mode;
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        ret = pthread_create(&mADThread, &attr, &AD::ad_thrd_func, this);
        if (ret) {
            LOGE("AD: Failed to start the ad_thread thread");
            pthread_attr_destroy(&attr);
        }
    }
    return ret;
}

int tokenize_params(char *inputParams, const char *delim, const int minTokenReq,
                                        char* tokenStr[], int *idx){
    char *tmp_token = NULL, *inputParams_r;
    int ret = 0, index = 0;
    if (!inputParams) {
        ret = -1;
        goto err;
    }

    tmp_token = strtok_r(inputParams, delim, &inputParams_r);
    while (tmp_token != NULL) {
        tokenStr[index++] = tmp_token;
        if (index < minTokenReq) {
            tmp_token = strtok_r(NULL, delim, &inputParams_r);
        }else{
            break;
        }
    }
    *idx = index;
err:
    return ret;
}

int DaemonContext::ProcessSetMsg(char* buf)
{
    int ret = -1;

    if (strstr(buf, FEATURE_PCC)) {
         ret = ProcessPCCMsg(&buf[0]);
        if (ret){
            LOGE("Failed to set PCC data");
        }
    } else {
        LOGE("This message is not supported currently!");
    }
    return ret;
}

int DaemonContext::ProcessPCCMsg(char* buf)
{

    char* tokens[MIN_PCC_PARAMS_REQUIRED];
    char *temp_token = NULL;
    int ret = -1, index = 0;
    int mdp_block = 0, offset = 0;
    struct display_pp_pcc_cfg pcc_cfg;
    struct display_pcc_coeff *coeff_ptr = NULL;

    memset(tokens, 0, sizeof(tokens));
    ret = tokenize_params(buf, TOKEN_PARAMS_DELIM, MIN_PCC_PARAMS_REQUIRED, tokens, &index);
    if(ret){
        LOGE("tokenize_params failed! (Line %d)", __LINE__);
        goto err;
    }

    if (index != MIN_PCC_PARAMS_REQUIRED){
        LOGE("invalid number of params reqiuired = %d != given = %d",
                MIN_PCC_PARAMS_REQUIRED, index);
        goto err;
    }

    LOGE_IF(mDebug, "tokenize_params successful with index = %d", index);

    if (tokens[DISPLAY_INDEX]) {
        switch(atoi(tokens[DISPLAY_INDEX])) {
            case PRIMARY_DISPLAY:
                mdp_block = MDP_LOGICAL_BLOCK_DISP_0;
                break;
            case SECONDARY_DISPLAY:
                mdp_block = MDP_LOGICAL_BLOCK_DISP_1;
                break;
            case WIFI_DISPLAY:
                mdp_block = MDP_LOGICAL_BLOCK_DISP_2;
                break;
            default:
                LOGE("Display option is invalid");
                goto err;
        }
    } else {
        LOGE("Display option is not provided!");
        goto err;
    }

    pcc_cfg.ops = MDP_PP_OPS_ENABLE | MDP_PP_OPS_WRITE;
    coeff_ptr = &pcc_cfg.r;
    while(offset != (MIN_PCC_PARAMS_REQUIRED - DATA_INDEX)) {
        for(index = 0; index < PCC_COEFF_PER_COLOR; index++) {
            coeff_ptr->pcc_coeff[index] = atof(tokens[index + DATA_INDEX + offset]);
        }
        offset += PCC_COEFF_PER_COLOR;
        switch(offset){
            case PCC_COEFF_PER_COLOR:
                coeff_ptr = &pcc_cfg.g;
                break;
            case (PCC_COEFF_PER_COLOR * 2):
                coeff_ptr = &pcc_cfg.b;
                break;
            default:
                break;
        }
    }

    LOGE_IF(mDebug, "Calling user space library for PCC!!");
    ret = display_pp_pcc_set_cfg(mdp_block, &pcc_cfg);
err:
    return ret;
}

int AD::ADInit(char* initial) {
    int ret = -1;
    char* params[AD_INIT_PARAM_CNT];
    char* data[AD_MAX_DATA_CNT];
    int i = 0, j = 0, index = 0;
    uint32_t* bl_lm;
    uint32_t* bl_lm_inv;
    uint32_t* bl_att;
    struct fb_var_screeninfo info;

    if(mADSupported) {
        struct msmfb_mdp_pp pp;
        struct mdss_ad_init *init = &pp.data.ad_init_cfg.params.init;

        pp.op = mdp_op_ad_cfg;
        pp.data.ad_init_cfg.ops = MDP_PP_OPS_ENABLE | MDP_PP_AD_INIT;
        init->bl_lin_len = BL_LIN_LUT_SIZE;
        init->bl_att_len = AD_BL_ATT_LUT_LEN;
        init->alpha_base = BL_ATT_ALPHA_BASE;

        bl_lm = (uint32_t*) malloc(BL_LIN_LUT_SIZE * sizeof(uint32_t));
        if (bl_lm == NULL)
            return ret;

        bl_lm_inv = (uint32_t*) malloc(BL_LIN_LUT_SIZE * sizeof(uint32_t));
        if (bl_lm_inv == NULL) {
            free(bl_lm);
            return ret;
        }

        bl_att = (uint32_t*) malloc(AD_BL_ATT_LUT_LEN * sizeof(uint32_t));
        if (bl_att == NULL) {
            free(bl_lm);
            free(bl_lm_inv);
            return ret;
        }

        /* Initialization parsing*/
        memset(params, 0, sizeof(params));
        ret = tokenize_params(initial, AD_PARAM_SEPARATOR, AD_INIT_PARAM_CNT, params, &index);
        if(ret){
            LOGE("tokenize_params failed!");
            goto err;
        }

        if (index != AD_INIT_PARAM_CNT){
            LOGE("invalid number of params reqiuired = %d != given = %d",
                    AD_INIT_PARAM_CNT, index);
            goto err;
        }

        for (i = 0; i < AD_INIT_PARAM_CNT; i++) {
            memset(data, 0, sizeof(data));
            index = 0;
            ret = tokenize_params(params[i], AD_DATA_SEPARATOR, ad_init_data_cnt[i], data, &index);
            if(ret){
                LOGE("tokenize_params failed!");
                goto err;
            }
            if (index != ad_init_data_cnt[i]){
                LOGE("invalid number of params reqiuired = %d != given = %d",
                        ad_init_data_cnt[i], index);
                goto err;
            }

            switch (i) {
            case 0:
                for (j = 0; j < ad_init_data_cnt[i]; j++)
                    init->asym_lut[j] = atoi(data[j]);
                break;
            case 1:
                for (j = 0; j < ad_init_data_cnt[i]; j++)
                    init->color_corr_lut[j] = atoi(data[j]);
                break;
            case 2:
                for (j = 0; j < ad_init_data_cnt[i]; j++)
                    init->i_control[j] = (uint8_t) atoi(data[j]);
                break;
            case 3:
                init->black_lvl = (uint8_t) atoi(data[0]);
                break;
            case 4:
                init->white_lvl = (uint16_t) atoi(data[0]);
                break;
            case 5:
                init->var = (uint8_t) atoi(data[0]);
                break;
            case 6:
                init->limit_ampl = (uint8_t) atoi(data[0]);
                break;
            case 7:
                init->i_dither = (uint8_t) atoi(data[0]);
                break;
            case 8:
                init->slope_max = (uint8_t) atoi(data[0]);
                break;
            case 9:
                init->slope_min = (uint8_t) atoi(data[0]);
                break;
            case 10:
                init->dither_ctl = (uint8_t) atoi(data[0]);
                break;
            case 11:
                init->format = (uint8_t) atoi(data[0]);
                break;
            case 12:
                init->auto_size = (uint8_t) atoi(data[0]);
                break;
            case 13:
                init->frame_w = (uint16_t) atoi(data[0]);
                break;
            case 14:
                init->frame_h = (uint16_t) atoi(data[0]);
                break;
            case 15:
                init->logo_v = (uint8_t) atoi(data[0]);
                break;
            case 16:
                init->logo_h = (uint8_t) atoi(data[0]);
                break;
            case 17:
                for (j = 0; j < ad_init_data_cnt[i]; j++)
                    bl_lm[j] = (uint32_t) atoi(data[j]);
                init->bl_lin = bl_lm;
                break;
            case 18:
                for (j = 0; j < ad_init_data_cnt[i]; j++)
                    bl_lm_inv[j] = (uint32_t) atoi(data[j]);
                init->bl_lin_inv = bl_lm_inv;
                break;
            default:
                break;
            }

        }

        if (ioctl(mDisplayFd, FBIOGET_VSCREENINFO, &info) == -1) {
            LOGE("Error getting FB Info : %s", strerror(errno));
            ret = errno;
            goto err;
        }
        init->frame_w = info.xres;
        init->frame_h = info.yres;

        LOGE_IF(mDebug, "Calling ioctl for AD init!");
        if (ioctl(mDisplayFd, MSMFB_MDP_PP, &pp)) {
            LOGE("ad_init ioctl failed, err: %s", strerror(errno));
            ret = errno;
        } else {
            mADCalibStatus |= CALIB_INIT;
        }

err:
        free(bl_lm);
        free(bl_lm_inv);
        free(bl_att);

    }
    return ret;
}

int AD::ADConfig(char* config) {
    int ret = -1;
    char* params[AD_CFG_PARAM_CNT];
    char* data[AD_MAX_DATA_CNT];
    int i = 0, j = 0, index = 0;

    if(mADSupported) {
        struct msmfb_mdp_pp pp;
        struct mdss_ad_cfg *cfg = &pp.data.ad_init_cfg.params.cfg;

        pp.op = mdp_op_ad_cfg;
        pp.data.ad_init_cfg.ops = MDP_PP_AD_CFG;

        /* Calibration parsing*/
        memset(params, 0, sizeof(params));
        ret = tokenize_params(config, AD_PARAM_SEPARATOR, AD_CFG_PARAM_CNT, params, &index);
        if(ret){
            LOGE("tokenize_params failed!");
            goto err;
        }

        if (index != AD_CFG_PARAM_CNT){
            LOGE("invalid number of params reqiuired = %d != given = %d",
                    AD_CFG_PARAM_CNT, index);
            goto err;
        }

        for (i = 0; i < AD_CFG_PARAM_CNT; i++) {
            memset(data, 0, sizeof(data));
            index = 0;
            ret = tokenize_params(params[i], AD_DATA_SEPARATOR, ad_cfg_data_cnt[i], data, &index);
            if(ret){
                LOGE("tokenize_params failed!");
                goto err;
            }
            if (index != ad_cfg_data_cnt[i]){
                LOGE("invalid number of params reqiuired = %d != given = %d",
                        ad_cfg_data_cnt[i], index);
                goto err;
            }

            switch (i) {
            case 0:
                mMode = atoi(data[0]);
                cfg->mode = mMode;
                break;
            case 1:
                for (j = 0; j < ad_cfg_data_cnt[i]; j++)
                    cfg->al_calib_lut[j] = atoi(data[j]);
                break;
            case 2:
                cfg->backlight_min = (uint16_t) atoi(data[0]);
                break;
            case 3:
                cfg->backlight_max = (uint16_t) atoi(data[0]);
                break;
            case 4:
                cfg->backlight_scale = (uint16_t) atoi(data[0]);
                break;
            case 5:
                cfg->amb_light_min = (uint16_t) atoi(data[0]);
                break;
            case 6:
                for (j = 0; j < ad_cfg_data_cnt[i]; j++)
                    cfg->filter[j] = (uint16_t) atoi(data[j]);
                break;
            case 7:
                for (j = 0; j < ad_cfg_data_cnt[i]; j++)
                    cfg->calib[j] = (uint16_t) atoi(data[j]);
                break;
            case 8:
                cfg->strength_limit = (uint8_t) atoi(data[0]);
                break;
            case 9:
                cfg->t_filter_recursion = (uint8_t) atoi(data[0]);
                break;
            case 10:
                cfg->stab_itr = (uint16_t) atoi(data[0]);
                break;
            default:
                break;
            }

        }

        LOGE_IF(mDebug, "Calling ioctl for AD cfg!");
        if (ioctl(mDisplayFd, MSMFB_MDP_PP, &pp)) {
            LOGE("ad_config ioctl failed, err: %s", strerror(errno));
            ret = errno;
        } else {
            mADCalibStatus |= CALIB_CFG;
        }

    }
err:
    return ret;
}

int AD::ADInput(int aml) {
    int ret = -1;
    struct msmfb_mdp_pp in;
    struct mdss_ad_input *input = &in.data.ad_input;

    if(mADSupported) {
        if(!CALIB_READY(mADCalibStatus)) {
            LOGE("%s:Init or Config not yet done, calib status = %d", __func__, mADCalibStatus);
            return -1;
        }
        /*ambient light tolenrance check */
        if (aml < AMBIENT_LIGHT_MIN || aml > AMBIENT_LIGHT_MAX) {
            LOGE("Invalid albient light input = %d", aml);
            return -1;
        }

        mALSValue = aml;

        in.op = mdp_op_ad_input;
        in.data.ad_input.mode = mMode;
        in.data.ad_input.in.amb_light = mALSValue;
        ret = ioctl(mDisplayFd, MSMFB_MDP_PP, &in);
        if (ret) {
            if (errno != EHOSTDOWN) {
                LOGE("%s: ad_input ioctl failed, ret = %d, err: %s",__func__, ret, strerror(errno));
                ret = errno;
            } else
                ret = 0;
        } else {
            LOGE_IF(mDebug, "%s: ad_input ioctl successful!!, ALS value = %d",__func__, in.data.ad_input.in.amb_light);
            ret = 0;
        }
    }
    return ret;
}

int AD::ADUpdateAL(int aml, int refresh_cnt) {
    int ret = -1;

    if (mRefresher) {
        LOGE_IF(mRefresher->mDebug, "Calling refresh()");
        mRefresher->Refresh(refresh_cnt, AD_REFRESH_INTERVAL);
    }

    ret = ADInput(aml);
    if (ret) {
        if (ret == ETIMEDOUT)
            LOGE_IF(mDebug, "%s(): ADInput() Failed : ETIMEDOUT", __func__);
        else
            LOGE("%s(): ADInput() Failed : Return Value = %d", __func__, ret);
        if (mRefresher) {
            LOGE_IF(mRefresher->mDebug, "Cancelling refresh");
            mRefresher->Refresh(0, AD_REFRESH_INTERVAL);
        }
    } else {
        LOGE_IF(mDebug, "%s(): ADUpdateAL() successfull, ALS = %d",
                __func__, mALSValue);
        mLastSentALSValue = aml;
    }

    return ret;
}

int AD::ADPrintCalibData(struct ad_default_params* params) {
    int ret = 0;

    if(mADSupported) {
        LOGE_IF(mDebug, "=======================Printing mdss_ad_init====================");
        int i = 0, j = 0;
        for (i = 0; i < AD_INIT_PARAM_CNT; i++) {
            switch (i) {
            case 0:
                for (j = 0; j < ad_init_data_cnt[i]; j++)
                    LOGE_IF(mDebug, "init.asym_lut[%d]: %d", j, params->init.asym_lut[j]);
                break;
            case 1:
                for (j = 0; j < ad_init_data_cnt[i]; j++)
                    LOGE_IF(mDebug, "init.color_corr_lut[%d]: %d", j, params->init.color_corr_lut[j]);
                break;
            case 2:
                for (j = 0; j < ad_init_data_cnt[i]; j++)
                    LOGE_IF(mDebug, "init.i_control[%d]: %d", j, params->init.i_control[j]);
                break;
            case 3:
                LOGE_IF(mDebug, "init.black_lvl: %d", params->init.black_lvl);
                break;
            case 4:
                LOGE_IF(mDebug, "init.white_lvl: %d", params->init.white_lvl);
                break;
            case 5:
                LOGE_IF(mDebug, "init.var: %d", params->init.var);
                break;
            case 6:
                LOGE_IF(mDebug, "init.limit_ampl: %d", params->init.limit_ampl);
                break;
            case 7:
                LOGE_IF(mDebug, "init.i_dither: %d", params->init.i_dither);
                break;
            case 8:
                LOGE_IF(mDebug, "init.slope_max: %d", params->init.slope_max);
                break;
            case 9:
                LOGE_IF(mDebug, "init.slope_min: %d", params->init.slope_min);
                break;
            case 10:
                LOGE_IF(mDebug, "init.dither_ctl: %d", params->init.dither_ctl);
                break;
            case 11:
                LOGE_IF(mDebug, "init.format: %d", params->init.format);
                break;
            case 12:
                LOGE_IF(mDebug, "init.auto_size: %d", params->init.auto_size);
                break;
            case 13:
                LOGE_IF(mDebug, "init.frame_w: %d", params->init.frame_w);
                break;
            case 14:
                LOGE_IF(mDebug, "init.frame_h: %d", params->init.frame_h);
                break;
            case 15:
                LOGE_IF(mDebug, "init.logo_v: %d", params->init.logo_v);
                break;
            case 16:
                LOGE_IF(mDebug, "init.logo_h: %d", params->init.logo_h);
                break;
            case 17:
                for (j = 0; j < ad_init_data_cnt[i]; j++)
                    LOGE_IF(mDebug, "init.bl_lin[%d]: %d", j, params->init.bl_lin[j]);
                break;
            case 18:
                for (j = 0; j < ad_init_data_cnt[i]; j++)
                    LOGE_IF(mDebug, "init.bl_lin_inv[%d]: %d", j, params->init.bl_lin_inv[j]);
                break;
            default:
                break;
            }

        }

        LOGE_IF(mDebug, "===================Printing mdss_ad_cfg====================");

        for (i = 0; i < AD_CFG_PARAM_CNT; i++) {
            switch (i) {
            case 0:
                LOGE_IF(mDebug, "cfg.mode: %d", params->cfg.mode);
                break;
            case 1:
                for (j = 0; j < ad_cfg_data_cnt[i]; j++)
                    LOGE_IF(mDebug, "cfg.al_calib_lut[%d]: %d", j, params->cfg.al_calib_lut[j]);
                break;
            case 2:
                LOGE_IF(mDebug, "cfg.backlight_min: %d", params->cfg.backlight_min);
                break;
            case 3:
                LOGE_IF(mDebug, "cfg.backlight_max: %d", params->cfg.backlight_max);
                break;
            case 4:
                LOGE_IF(mDebug, "cfg.backlight_scale: %d", params->cfg.backlight_scale);
                break;
            case 5:
                LOGE_IF(mDebug, "cfg.amb_light_min: %d", params->cfg.amb_light_min);
                break;
            case 6:
                for (j = 0; j < ad_cfg_data_cnt[i]; j++)
                    LOGE_IF(mDebug, "cfg.filter[%d]: %d", j, params->cfg.filter[j]);
                break;
            case 7:
                for (j = 0; j < ad_cfg_data_cnt[i]; j++)
                    LOGE_IF(mDebug, "cfg.calib[%d]: %d", j, params->cfg.calib[j]);
                break;
            case 8:
                LOGE_IF(mDebug, "cfg.strength_limit: %d", params->cfg.strength_limit);
                break;
            case 9:
                LOGE_IF(mDebug, "cfg.t_filter_recursion: %d", params->cfg.t_filter_recursion);
                break;
            case 10:
                LOGE_IF(mDebug, "cfg.stab_itr: %d", params->cfg.stab_itr);
                break;
            default:
                break;
            }

        }
    }
    return ret;
}

int AD::ADCalcCalib() {
    int ret = FAILED;
    if(mAssertivenessSliderValue < 0 || mAssertivenessSliderValue > 255) {
        LOGE_IF(mDebug, "assertivenessSliderValue is not in valid range");
        return ret;
    }

    double assertiveness =
        pow(2.0, (8.0 * (mAssertivenessSliderValue - 128.0) / 255.0));

    mAD_calib_a = round(mDefADConfig.cfg.calib[0] * assertiveness);
    mAD_calib_c = round(mDefADConfig.cfg.calib[2] * assertiveness);
    mAD_calib_d= round(mDefADConfig.cfg.calib[3] * assertiveness);

    return SUCCESS;
}

int AD::ADParseCalibData(struct ad_default_params* params) {
    int ret = FAILED;
    FILE *fp = NULL;
    char *line = NULL;
    char *temp_token = NULL;
    uint32_t temp;
    int i = 0, j = 0;
    int items;

    if(mADSupported) {
        /* check whehter the input is valid or not */
        if (params == NULL) {
            LOGE("Invalid input!");
        }

        char property[PROPERTY_VALUE_MAX];
        char ad_calib_file[256] = {'\0'};
        if (property_get(AD_CALIB_DATA_PATH_PROP, ad_calib_file, NULL) > 0) {
            if ( -1 == access(&ad_calib_file[0], R_OK|F_OK)) {
                LOGE("%s: No permission to access calibration data file or file does not exists!",__func__);
                return -1;
            }
        }

        LOGE_IF(mDebug, "======================Reading calib_data E=========================");
        /* open file for parsing*/
        fp = fopen(ad_calib_file, "r");
        if (!fp)
            return -1;

        line = (char *)malloc(MAX_CMD_LEN * sizeof(char));
        if (!line) {
            LOGE("Cant allocate memory");
            goto err;
        }

        while(fgets(line, MAX_CMD_LEN * sizeof(char), fp)) {
            if (line[0] == '\r' || line[0] == '\n' || line[0] == '#')
                continue;
            if (!strncmp(line, "=version", strlen("=version")))
                goto interp_version;
            if (!strncmp(line, "=init", strlen("=init")))
                goto interp_init;
            if (!strncmp(line, "=config", strlen("=config")))
                goto interp_config;

interp_version:
            while(fgets(line, MAX_CMD_LEN * sizeof(char), fp) && i < 1) {
                if (line[0] == '\r' || line[0] == '\n' || line[0] == '#')
                    goto interp_version;
                LOGE_IF(mDebug, "version = %s", line);
                ++i;
            }
            goto exit_version;
interp_init:
            while(fgets(line, MAX_CMD_LEN * sizeof(char), fp) &&
                    i < AD_INIT_PARAM_CNT) {
                char *line_r;

                if (line[0] == '\r' || line[0] == '\n' || line[0] == '#')
                    goto interp_init;

                items = ad_init_data_cnt[i];
                temp_token = strtok_r(line, " ", &line_r);
                if (temp_token != NULL) {
                    j = 0;
                    do {
                        temp = atoi(temp_token);
                        LOGE_IF(mDebug, "%d ", temp);
                        switch (i) {
                        case 0:
                            mDefADConfig.init.asym_lut[j] = temp;
                            break;
                        case 1:
                            mDefADConfig.init.color_corr_lut[j] = temp;
                            break;
                        case 2:
                            mDefADConfig.init.i_control[j] = (uint8_t) temp;
                            break;
                        case 3:
                            mDefADConfig.init.black_lvl = (uint8_t) temp;
                            break;
                        case 4:
                            mDefADConfig.init.white_lvl = (uint16_t) temp;
                            break;
                        case 5:
                            mDefADConfig.init.var = (uint8_t) temp;
                            break;
                        case 6:
                            mDefADConfig.init.limit_ampl = (uint8_t) temp;
                            break;
                        case 7:
                            mDefADConfig.init.i_dither = (uint8_t) temp;
                            break;
                        case 8:
                            mDefADConfig.init.slope_max = (uint8_t) temp;
                            break;
                        case 9:
                            mDefADConfig.init.slope_min = (uint8_t) temp;
                            break;
                        case 10:
                            mDefADConfig.init.dither_ctl = (uint8_t) temp;
                            break;
                        case 11:
                            mDefADConfig.init.format = (uint8_t) temp;
                            break;
                        case 12:
                            mDefADConfig.init.auto_size = (uint8_t) temp;
                            break;
                        case 13:
                            mDefADConfig.init.frame_w = (uint16_t) temp;
                            break;
                        case 14:
                            mDefADConfig.init.frame_h = (uint16_t) temp;
                            break;
                        case 15:
                            mDefADConfig.init.logo_v = (uint8_t) temp;
                            break;
                        case 16:
                            mDefADConfig.init.logo_h = (uint8_t) temp;
                            break;
                        case 17:
                            mDefADConfig.init.bl_lin[j] = (uint32_t) temp;
                            break;
                        case 18:
                            mDefADConfig.init.bl_lin_inv[j] = (uint32_t) temp;
                            break;
                        default:
                            break;
                        }
                        j++;
                        temp_token = strtok_r(NULL, " ", &line_r);
                    } while (temp_token != NULL && j < items);
                    if (j != items)
                        LOGE("not enough items (%d/%d) on input line %d", j, items, i);
                }
                i++;
            }
            goto exit_init;
interp_config:
            while(fgets(line, MAX_CMD_LEN * sizeof(char), fp) &&
                    i < AD_CFG_PARAM_CNT) {
                char *line_r;
                if (line[0] == '\r' || line[0] == '\n' || line[0] == '#')
                    goto interp_config;

                items = ad_cfg_data_cnt[i];
                temp_token = strtok_r(line, " ", &line_r);
                if (temp_token != NULL) {
                    j = 0;
                    do {
                        temp = atoi(temp_token);
                        LOGE_IF(mDebug, "%d ", temp);
                        switch (i) {
                        case 0:
                            mDefADConfig.cfg.mode = temp;
                            break;
                        case 1:
                            mDefADConfig.cfg.al_calib_lut[j] = temp;
                            break;
                        case 2:
                            mDefADConfig.cfg.backlight_min = (uint16_t) temp;
                            break;
                        case 3:
                            mDefADConfig.cfg.backlight_max = (uint16_t) temp;
                            break;
                        case 4:
                            mDefADConfig.cfg.backlight_scale = (uint16_t) temp;
                            break;
                        case 5:
                            mDefADConfig.cfg.amb_light_min = (uint16_t) temp;
                            break;
                        case 6:
                            mDefADConfig.cfg.filter[j] = (uint16_t) temp;
                            break;
                        case 7:
                            mDefADConfig.cfg.calib[j] = (uint16_t) temp;
                            break;
                        case 8:
                            mDefADConfig.cfg.strength_limit = (uint8_t) temp;
                            break;
                        case 9:
                            mDefADConfig.cfg.t_filter_recursion = (uint8_t) temp;
                            break;
                        case 10:
                            mDefADConfig.cfg.stab_itr = (uint16_t) temp;
                            break;
                        default:
                            break;
                        }
                        j++;
                        temp_token = strtok_r(NULL, " ", &line_r);
                    } while (temp_token != NULL && j < items);
                    if (j != items)
                        LOGE("not enough items (%d/%d) on input line %d", j, items, i);
                }
                i++;
            }
            goto exit_config;
exit_version:
            LOGE_IF(mDebug, "Finish parsing version session");
            i = 0;
            continue;
exit_init:
            LOGE_IF(mDebug, "Finish parsing init session");
            i = 0;
            continue;
exit_config:
            LOGE_IF(mDebug, "Finish parsing cfg session");
        }


        LOGE_IF(mDebug, "=================Reading calib_data X, ret %d================", ret);

        free(line);
err:
        ret = fclose(fp);
    }
    return ret;
}

static void signal_handler(int32_t sig) {
    /* Dummy signal handler */
    ;
}

static bool isHDMIPrimary (void) {

    char isPrimaryHDMI = '0';
    /* read HDMI sysfs nodes */
    FILE *fp = fopen(HDMI_PRIMARY_NODE, "r");

    if (fp) {
        fread(&isPrimaryHDMI, 1, 1, fp);
        if (isPrimaryHDMI == '1'){
            /* HDMI is primary */
            LOGD("%s: HDMI is primary display", __FUNCTION__);
            fclose(fp);
            return true;
        } else {
            /* Should never happen */
            LOGE("%s: HDMI_PRIMARY_NODE is: %c", __FUNCTION__, isPrimaryHDMI);
            fclose(fp);
            return false;
        }
    } else {
        /* HDMI_PRIMARY_NODE not present */
        LOGD("%s: HDMI is not primary display", __FUNCTION__);
        return false;
    }
}

int PostProc::parse_pa_data(int* hue, float* sat, int* intensity, float* contrast)
{
    int ret = FAILED, cnt = 0;
    char *line = NULL;
    FILE* fp;
    char pp_cfg_file[256] = {'\0'};
    if (property_get(PP_CFG_FILE_PROP, pp_cfg_file, NULL) > 0) {
        if ( -1 == access(&pp_cfg_file[0], R_OK|F_OK)) {
            LOGE("No permission to access postproc data file \
            or file does not exists! Using default settings!!");
            return 0;
        }
    } else {
        if (property_set(PP_CFG_FILE_PROP, PP_CFG_FILE_PATH)) {
            LOGE("Failed to set the pp cfg file property");
            return -1;
        }
        ret = strlcpy(pp_cfg_file, PP_CFG_FILE_PATH, sizeof(pp_cfg_file));
        if (ret >= sizeof(pp_cfg_file)) {
            LOGE("PP file path & name too long to fit in.");
            return -1;
        }
    }

    fp = fopen(pp_cfg_file, "r");
    if (!fp) {
        LOGE("Failed to open file %s",pp_cfg_file);
        return ret;
    }

    line = (char *)malloc(MAX_CMD_LEN * sizeof(char));
    if (!line) {
        LOGE("Cannot allocate memory");
        goto err;
    }

    while(fgets(line, MAX_CMD_LEN * sizeof(char), fp)) {
        if (line[0] == '\r' || line[0] == '\n' || line[0] == '#')
            continue;
        if (!strncmp(line, "=PA", strlen("=PA"))) {
            while(cnt < 1 && fgets(line, MAX_CMD_LEN * sizeof(char), fp)) {
                if (line[0] == '\r' || line[0] == '\n' || line[0] == '#')
                    continue;
                sscanf(line, "%d %f %d %f", hue,
                        sat, intensity, contrast);

                LOGD_IF(mDebug, "HSIC = %d %f %d %f", *hue, *sat, *intensity, *contrast);
                ++cnt;
            }
            LOGD_IF(mDebug, "Finish parsing PA data");
        }
    }
    ret = 0;
    free(line);
err:
    fclose(fp);
    return ret;
}

int PostProc::save_pa_data(int hue, float sat, int intensity, float contrast)
{

    int ret = FAILED, i = 0;
    FILE *fp = NULL;
    char pa_string[128] = {'\0'};
    char pp_cfg_file[256] = {'\0'};
    if (property_get(PP_CFG_FILE_PROP, pp_cfg_file, NULL) <= 0) {
        LOGE("Posproc file property is not set");
        return ret;
    }

    fp = fopen(pp_cfg_file, "w+");
    if (!fp) {
        LOGE("Postproc data file open failed");
        return ret;
    }
    fputs("=PA\n", fp);
    snprintf(pa_string, sizeof(pa_string), "%d %f %d %f\n", hue, sat, intensity, contrast);
    fputs(pa_string, fp);
    fclose(fp);
    return 0;
}

int PostProc::processPPDataFile()
{
    int ret = FAILED;
    ret = get_saved_hsic_config();
    if (ret) {
        LOGE("Failed to get the saved hsic config");
        return ret;
    }

    ret = start_pp();
    if (ret) {
        LOGE("Failed to start pp");
        return ret;
    }

    if (gMDP5)
        ret = set_hsic(mNewCfg.pa_cfg.hue, mNewCfg.pa_cfg.sat,
                    mNewCfg.pa_cfg.intensity, mNewCfg.pa_cfg.contrast);
    else
        ret = set_hsic(mNewCfg.conv_cfg.hue, mNewCfg.conv_cfg.sat,
                    mNewCfg.conv_cfg.intensity, mNewCfg.conv_cfg.contrast);

    if (ret) {
        LOGE("Failed to set the hsic values");
        return ret;
    }

    ret = stop_pp();
    if (ret) {
        LOGE("Failed to stop pp");
        return ret;
    }
    return 0;
}

int32_t main(int32_t argc, char** argv) {
    DaemonContext context;
    sigset_t signal_mask;
    int32_t sig;
    int ret;
    /*read the cabl version and set the property */
    char cabl_version[VERS_MAX_LEN] = {0};
    cabl_get_version(cabl_version);
    LOGD("CABL version %s", cabl_version);
    if (property_set("hw.cabl.version", cabl_version))
        LOGE("Failed to set the cabl version property");

    if (property_set(YUV_INPUT_STATE_PROP, "0"))
        LOGE("Failed to set the cabl auto level adjust property");

    // on bootup read and populate the OEM profiles
    char oem_param_file[256] = {'\0'};
    property_get("hw.oem.configfile", oem_param_file, "/data/oem_params.cfg");
    FILE *fp = fopen(oem_param_file, "rb");
    if (fp) {
        featureList.init(fp);
        if (context.mDebug)
            featureList.dump();
        featureList.verifyChecksum(fp);
        fclose(fp);
    } else {
        LOGE("Failed to open the config file!");
    }
    inspectHW();

    bool is_hdmi_primary = isHDMIPrimary();
    if (!is_hdmi_primary) {
        if (gMDP5)
            context.mPostProc.mBlockType = MDP_LOGICAL_BLOCK_DISP_0;
        else
            context.mPostProc.mBlockType = MDP_BLOCK_DMA_P;
    } else
        context.mPostProc.mBlockType = MDP_BLOCK_OVERLAY_1;

    /* Open the primary framebuffer */
    int fb_idx = -1;
    ret = context.SelectFB(MDP_LOGICAL_BLOCK_DISP_0, &fb_idx);
    if (fb_idx < 0) {
        LOGE("Cannot locate the primary framebuffer");
        exit(ret);
    } else {
        char FbPath[MAX_FB_NAME_LEN];
        snprintf(FbPath, sizeof(FbPath), "%s%d", FRAMEBUFFER_NODE, fb_idx);
        FBFd = open(FbPath, O_RDWR);
        if (FBFd < 0) {
            LOGE("Cannot open framebuffer");
            exit(FBFd);
        }
    }
    /* Open the backlight sysfs node */
    BLFd = open(SYS_BRIGHTNESS, O_RDWR);
    if (BLFd < 0) {
        BLFd = open(SYS_BRIGHTNESS_ALT, O_RDWR);
        if (BLFd < 0) {
            LOGE("Cannot open backlight");
            exit(BLFd);
        }
    }
    /*Check whether target is using split display or not*/
    context.IsSplitDisplay(FBFd);

    if(context.start()) {
        LOGE("Failure to start the listener thread");
        close(FBFd);
        exit(-1);
    }


    if(!context.mDCM)
        context.mDCM = new DCM();
    if (context.mDCM)
        context.mDCM->DCMControl(true);


    char buf[SHORT_CMD_LEN];
    int32_t acceptFd = -1;
    int opt;
    static struct option long_options[] = {
        {"enable-cabl", no_argument, 0, 'c'},
        {"enable-ad", no_argument, 0, 'a'},
        {0, 0, 0, 0},
    };

    int32_t update_notify = NOTIFY_UPDATE_START;
    ret = ioctl(FBFd, MSMFB_NOTIFY_UPDATE, &update_notify);
    while(!ioctl(FBFd, MSMFB_NOTIFY_UPDATE, &update_notify))
        continue;
    while ((opt = getopt_long(argc, argv, "ac", long_options, NULL)) != -1) {
        switch (opt) {
            case 'a':
                snprintf(buf, SHORT_CMD_LEN, "%s;%d;%d", CMD_AD_ON, ad_mode_auto_str, MDP_LOGICAL_BLOCK_DISP_0);
                context.ProcessCommand(buf, SHORT_CMD_LEN, acceptFd);
                break;
            case 'c':
                snprintf(buf, SHORT_CMD_LEN, "%s", CMD_CABL_ON);
                context.mBootStartCABL = true;
                context.ProcessCommand(buf, SHORT_CMD_LEN, acceptFd);
                break;
            default:
                LOGE("Un-recognized option");
                break;
        }
    }

    // on bootup read and populate the pp data file if any
    context.mPostProc.processPPDataFile();

    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);

    /* Register the signal Handler */
    sigfillset(&signal_mask);
    signal(SIGTERM, &signal_handler);
    signal(SIGPIPE, SIG_IGN);
    while (1) {
        sigwait(&signal_mask, &sig);
        switch (sig) {
            case SIGTERM:
                LOGD("SIGTERM received, stopping daemon");
                sigflag = 1;
                if(2 == display_pp_cabl_supported()){
                    context.getABA()->CABLControl(false);
                    context.getABA()->SVIControl(false);
                    if(1 == display_pp_svi_supported()){
                        context.getABA()->StopALSUpdaterThread();
                        context.getABA()->CleanupLightSensor();
                    }
                    context.StopAlgorithmObjects();
                } else {
                    context.mCABL->stop_cabl();
                }
                context.mPostProc.stop_pp();
                context.mAD.ADControl(&context, false);

                if (context.mDCM) {
                    context.mDCM->DCMControl(false);
                    delete context.mDCM;
                    context.mDCM = NULL;
                }
                if (context.mAD.isADEnabled() == AD_ENABLE_WB) {
                    void *term;
                    pthread_join(context.mADPollThrdId, &term);
                    LOGE_IF(context.mDebug, "AD Poll thread terminated");
                }
                close(BLFd);
                close(FBFd);
                exit(EXIT_SUCCESS);
        }
    }
    return 0;
}
