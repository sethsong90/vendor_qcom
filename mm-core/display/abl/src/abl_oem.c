
/*=============================================================================

  A D A P T I V E   B A C K L I G H T   C O N T R O L   H E A D E R   F I L E

  DESCRIPTION
  This file contains interface specifications of the adaptive backlight control
  feature for display enhancements on QUALCOMM MSM display drivers. It reduces
  display power consumption by applying only the minimum required backlight for
  any given full video frame. The change in backlight levels is coupled with
  correponding gamma correction to compensate for changing brightness levels.

  Copyright (c) 2011-2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ===========================================================================*/
/*============================================================================
  INCLUDE FILES FOR MODULE
  ============================================================================*/
#include <cutils/properties.h>
#include "abl_oem.h"

/*============================================================================
  OEM Tuning Parameters
  ============================================================================*/

/* Display Panel Characterization - Gamma curve normalized to 0-1024
 * The tables below contain the default Gamma curve (r=2.2)
 */
static uint32_t oem_gamma_grayscale[] = {
       0,   32,   64,   96,  129,  161,  193,  225,
     257,  289,  321,  353,  386,  418,  450,  482,
     514,  546,  578,  610,  643,  675,  707,  739,
     771,  803,  835,  867,  900,  932,  964,  996,
     1024};

static uint32_t oem_gamma_luminance[] = {
       0,    1,    2,    6,   11,   17,   26,   36,
      49,   63,   79,   98,  118,  141,  166,  193,
     223,  255,  289,  325,  364,  405,  449,  495,
     544,  595,  649,  705,  763,  825,  888,  955,
     1024};

static uint32_t oem_length_gamma_lut = 33;

/* BL minimum level         range: 0-1024 */
static uint32_t oem_bl_level_threshold    = 124;

/* OEM table parameters                                        range: 0-1024
 *                                                     QL_LOW QL_MED QL_HIGH
 */
static uint32_t oem_bl_min_ratio[ABL_QUALITY_MAX]    = {  461,  614,  768};
static uint32_t oem_bl_max_ratio[ABL_QUALITY_MAX]    = {1024, 1024, 1024};
static uint32_t oem_pixel_distortion[ABL_QUALITY_MAX]= { 150,  100,   100};
static uint32_t oem_bl_change_speed[ABL_QUALITY_MAX] = {   8,   12,   16};

/*============================================================================
  Other Parameters
  ============================================================================*/

/*    Display Panel Characterization - BL response curve   */
static uint32_t oem_blresponse_bl[]         = {0,100, 200, 300, 400, 500,
                                                600, 700, 800, 900,1024};
static uint32_t oem_blresponse_luminance[]  = {0,100, 200, 300, 400, 500,
                                                600, 700, 800, 900,1024};
static uint32_t oem_length_bl_luminance_lut = 11;

/*============================================================================
  API Info Initialization
  ============================================================================*/
void apiInfoInit_oem(bl_oem_api *api_para, uint32_t init_data_quality,
                        uint32_t init_data_level)
{
    // Reserved parameters
    static double   reserved_param_SS[ABL_QUALITY_MAX]    = {0.32, 0.28, 0.25};
    static int32_t  reserved_param_LT[ABL_QUALITY_MAX]    = {   3,    3,    3};
    static uint32_t reserved_param_WST[ABL_QUALITY_MAX]   = {   8,    6,    4};
    static uint32_t reserved_param_FCT[ABL_QUALITY_MAX]   = { 820,  409,  409};
    static uint32_t reserved_param_BDF[ABL_QUALITY_MAX]   = {   4,    2,    4};
    static uint32_t reserved_param_BSTHC[ABL_QUALITY_MAX] = {   8,    8,    8};
    static uint32_t reserved_param_SCT[ABL_QUALITY_MAX]   = {1020, 1024, 1024};
    static uint32_t reserved_param_SCD[ABL_QUALITY_MAX]   = { 800,  800,  800};

    int count, maxBLFd = -1, bytes = -1;
    char buffer[MAX_BACKLIGHT_LEN];

    char property[PROPERTY_VALUE_MAX];
    (void)memset(api_para, 0, sizeof(bl_oem_api));

    /* DEBUG option, 0:no printf, 1:coefficients; 2: final gamma; 3: with histogram */
    if (property_get("debug.cabl.logs", property, 0) > 0) {
        api_para->bl_debug = atoi(property);
        api_para->bl_debug = ((api_para->bl_debug < 0) ? 0
                : ((api_para->bl_debug > 3) ? 3 : api_para->bl_debug));
    }

    for (count = ABL_QUALITY_LOW; count < ABL_QUALITY_MAX; count++) {
      api_para->cabl_quality_params[count].bl_min_ratio = oem_bl_min_ratio[count];
      api_para->cabl_quality_params[count].bl_max_ratio = oem_bl_max_ratio[count];
      api_para->cabl_quality_params[count].pixel_distortion = oem_pixel_distortion[count];
      api_para->cabl_quality_params[count].bl_filter_stepsize = oem_bl_change_speed[count];
      api_para->cabl_quality_params[count].reserved_param_SS = reserved_param_SS[count];
      api_para->cabl_quality_params[count].reserved_param_LT = reserved_param_LT[count];
      api_para->cabl_quality_params[count].reserved_param_WST = reserved_param_WST[count];
      api_para->cabl_quality_params[count].reserved_param_FCT = reserved_param_FCT[count];
      api_para->cabl_quality_params[count].reserved_param_BDF = reserved_param_BDF[count];
      api_para->cabl_quality_params[count].reserved_param_BSTHC = reserved_param_BSTHC[count];
      api_para->cabl_quality_params[count].reserved_param_SCT = reserved_param_SCT[count];
      api_para->cabl_quality_params[count].reserved_param_SCD = reserved_param_SCD[count];
    }

    if (property_get("hw.cabl.level", property, 0) > 0) {
        if (!strcmp(property, CABL_LVL_AUTO))
            api_para->default_ql_mode = AUTO_QL_MODE;
        else
            api_para->default_ql_mode = USER_QL_MODE;
    } else {
        LOGE("Property hw.cabl.level is never set!");
        api_para->default_ql_mode = AUTO_QL_MODE;
    }

    /* Open the max backlight sysfs node */
    api_para->bl_max_level = 255;
    maxBLFd = open(SYS_MAX_BRIGHTNESS, O_RDONLY);
    if (maxBLFd < 0) {
        LOGE("Cannot open backlight");
    } else {
        memset(buffer, 0, MAX_BACKLIGHT_LEN);
        bytes = read(maxBLFd, buffer, sizeof (char) * MAX_BACKLIGHT_LEN);
        if (bytes > 0) {
            lseek(maxBLFd, 0, SEEK_SET);
            api_para->bl_max_level = atoi(&buffer[0]);
        }
        close(maxBLFd);
    }

    api_para->ui_quality_lvl    = ABL_QUALITY_HIGH;
    api_para->video_quality_lvl = ABL_QUALITY_LOW;
    api_para->SetLevel        = init_data_quality;
    api_para->bl_level_len[0] = oem_length_gamma_lut;
    api_para->bl_level_len[1] = oem_length_bl_luminance_lut;
    api_para->pY_shade        = oem_gamma_grayscale;
    api_para->pY_gamma        = oem_gamma_luminance;
    api_para->pY_lvl          = oem_blresponse_bl;
    api_para->pbl_lvl         = oem_blresponse_luminance;

    api_para->bl_level_threshold = oem_bl_level_threshold;
    api_para->bl_min_level = interpolate(oem_length_bl_luminance_lut,
        oem_blresponse_bl, oem_blresponse_luminance, oem_bl_level_threshold) *
        api_para->bl_max_level / 1024;
    api_para->orig_level = (init_data_level > api_para->bl_min_level) ?
        init_data_level: api_para->bl_min_level;
}

void Aba_apiInfoInit_oem(CablInitialConfigType *api_para)
{
    /* Reserved parameters */
    static double   SoftClippingSlope[ABL_QUALITY_MAX]                 = { 0.32, 0.28, 0.25};
    static int32_t  uLutType[ABL_QUALITY_MAX]                          = {    3,    3,    3};
    static uint32_t uWindowSizeThreshold[ABL_QUALITY_MAX]              = {    8,    6,    4};
    static uint32_t uFilterCoefficientThreshold[ABL_QUALITY_MAX]       = {  820,  750,  600};
    static uint32_t uBacklightReductionFactor[ABL_QUALITY_MAX]         = {    4,    2,    1};
    static uint32_t uBacklightStepSizeHighCorrelation[ABL_QUALITY_MAX] = {   20,   20,   20};
    static uint32_t uSceneCorrelationThreshold[ABL_QUALITY_MAX]        = { 1024, 1024, 1024};
    static uint32_t uSceneChangeThreshold[ABL_QUALITY_MAX]             = {  800,  800,  800};

    int count;

    char property[PROPERTY_VALUE_MAX];
    (void)memset(api_para, 0, sizeof(CablInitialConfigType));

    /* DEBUG option, 0:no printf, 1:coefficients; 2: final gamma; 3: with histogram */
    if (property_get("debug.cabl.logs", property, 0) > 0) {
        api_para->uDebugLevel = atoi(property);
        api_para->uDebugLevel = (api_para->uDebugLevel > 3) ? 3
                                : api_para->uDebugLevel;
    }

    for (count = ABL_QUALITY_LOW; count < ABL_QUALITY_MAX; count++) {
      api_para->aCablQualityParameters[count].uBacklightScaleRatioLowerLimit =
                                        oem_bl_min_ratio[count];
      api_para->aCablQualityParameters[count].uBacklightScaleRatioUpperLimit =
                                        oem_bl_max_ratio[count];
      api_para->aCablQualityParameters[count].uPixelDistortionRate =
                                        oem_pixel_distortion[count];
      api_para->aCablQualityParameters[count].uFilterStepSize =
                                        oem_bl_change_speed[count];
      api_para->aCablQualityParameters[count].SoftClippingSlope =
                                        SoftClippingSlope[count];
      api_para->aCablQualityParameters[count].uLutType = uLutType[count];
      api_para->aCablQualityParameters[count].uWindowSizeThreshold =
                                        uWindowSizeThreshold[count];
      api_para->aCablQualityParameters[count].uFilterCoefficientThreshold =
                                        uFilterCoefficientThreshold[count];
      api_para->aCablQualityParameters[count].uBacklightReductionFactor =
                                        uBacklightReductionFactor[count];
      api_para->aCablQualityParameters[count].uBacklightStepSizeHighCorrelation =
                                        uBacklightStepSizeHighCorrelation[count];
      api_para->aCablQualityParameters[count].uSceneCorrelationThreshold =
                                        uSceneCorrelationThreshold[count];
      api_para->aCablQualityParameters[count].uSceneChangeThreshold =
                                        uSceneChangeThreshold[count];
    }

    api_para->uGammaResponseTableLength = oem_length_gamma_lut;
    api_para->uBacklightResponseTableLength = oem_length_bl_luminance_lut;
    api_para->pGammaResponseX        = oem_gamma_grayscale;
    api_para->pGammaResponseY        = oem_gamma_luminance;
    api_para->pBacklightResponseX          = oem_blresponse_bl;
    api_para->pBacklightResponseY         = oem_blresponse_luminance;

    api_para->uBacklightThresholdLevel = oem_bl_level_threshold;

    api_para->ui_quality_lvl    = ABL_QUALITY_HIGH;
    api_para->video_quality_lvl = ABL_QUALITY_LOW;
    api_para->bl_min_level = interpolate(oem_length_bl_luminance_lut,
            oem_blresponse_bl, oem_blresponse_luminance,
            oem_bl_level_threshold) * 255 / 1024;
}

/*=============================================================================
  FUNCTION interpolate()  - Interpolate function
  =============================================================================*/
static uint32_t interpolate(uint32_t len, uint32_t x[], uint32_t y[], uint32_t target)
{
    uint32_t result = 0;
    uint32_t i, j, ui_m, ui_c;
    double dm, dc;
    if ((x == NULL) || (y == NULL))
        return result;
    if( !x || !y)
        DISP_OSAL_LOG3("%s: begining interpolation: X = %u Y = %u", __func__, x, y);

    if (target >= x[len-1]) {
        result = y[len-1];
    } else if (target == x[0]) {
        result = y[0];
    } else {
        for(i = 1; target > x[i]; i++);

        j = i - 1;
        if (target == x[j]) {
            result  = y[j];
        } else {
            ui_m = ((y[i] - y[j])<<14) / (x[i] - x[j]);
            ui_c = y[i] - ((ui_m * x[i] + 8192)>>14);
            result = ((ui_m * target + 8192)>>14)+ ui_c;
        }
    }
    return result;
}

void pp_oem_message_handler(const char *cmd, const int32_t len, const int32_t fd) {
    /* =============================================
     *    OEMs can put the handler code here
     * =============================================*/
}

