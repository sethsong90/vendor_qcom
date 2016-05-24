/*
 * Copyright (c) 2012-2014, Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <mmi_window.h>
#include <mmi_module_manage.h>
#include <dlfcn.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <hash_map>

#include <cam_types.h>
#include <cam_intf.h>
#include <mm_qcamera_app.h>
#include <mm_jpeg_interface.h>

#include "mmi_config.h"
#include "mmi_utils.h"

#define LOG_TAG   "MMI_CAMERA"
#include <cutils/log.h>

#define PAD_TO_SIZE(size, padding) ((size + padding - 1) & ~(padding - 1))
#define DEFAULT_CAMERA_DEVICE "/dev/video1"

#define PREVIEW_WIDTH 640
#define PREVIEW_HEIGHT 480
#define SNAPSHOT_WIDTH 640
#define SNAPSHOT_HEIGHT 480
#define CAMERA_TIMEOUT 5 // seconds to wait for camera ready

#define PREVIEW_BUF_SIZE (PREVIEW_WIDTH * PREVIEW_HEIGHT)
#define SNAPSHOT_BUF_SIZE (SNAPSHOT_WIDTH * SNAPSHOT_HEIGHT)

uint8_t *g_pPreviewYUV420;
uint8_t *g_pPreview_Y;
uint8_t *g_pPreview_UV;
uint8_t *g_pPreviewRGB8888;
uint16_t *g_pPreviewRGB565;
uint8_t *g_pRotate90_tmp;

static int gRetVal = 0;
static int previewOrSnap = 0;   //0:preview, 1:snapshot
case_run_mode_t mode = TEST_MODE_NONE;

pthread_t frame_thread;
sem_t g_sem;

int mp0len, mp1len;
uint32_t g_sensor_mount_angle;

static hash_map < string, string > paras;

struct input_params {
    char device[128];
};

static struct input_params input;

/** DUMP_TO_FILE:
 *  @filename: file name
 *  @p_addr: address of the buffer
 *  @len: buffer length
 *
 *  dump the image to the file
 **/
#define DUMP_TO_FILE(filename, p_addr, len) ({ \
  int rc = 0; \
  FILE *fp = fopen(filename, "w+"); \
  if (fp) { \
    rc = fwrite(p_addr, 1, len, fp); \
    fclose(fp); \
  } else { \
    ALOGE("%s:%d] cannot dump image", __func__, __LINE__); \
  } \
})

static int g_count = 1, g_i;

typedef struct {
    char *filename;
    int width;
    int height;
    char *out_filename;
} jpeg_test_input_t;

typedef struct {
    struct ion_fd_data ion_info_fd;
    struct ion_allocation_data alloc;
    int p_pmem_fd;
    long size;
    int ion_fd;
    uint8_t *addr;
} buffer_test_t;

typedef struct {
    char *filename;
    int width;
    int height;
    char *out_filename;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    buffer_test_t input;
    buffer_test_t output;
    int use_ion;
    uint32_t handle;
    mm_jpeg_ops_t ops;
    uint32_t job_id[5];
    mm_jpeg_encode_params_t params;
    mm_jpeg_job_t job;
    uint32_t session_id;
} mm_jpeg_intf_test_t;

mm_camera_test_obj_t camera_obj;

static int encode_mmi(jpeg_test_input_t * p_input, void *data);

////------------- dll need ----------------
typedef int (*mm_app_open_lib_t) (mm_camera_app_t * cam_app, uint8_t cam_id, mm_camera_test_obj_t * test_obj);
typedef int (*mm_app_close_lib_t) (mm_camera_test_obj_t * test_obj);
typedef int (*mm_app_stop_preview_lib_t) (mm_camera_test_obj_t * test_obj);
typedef mm_camera_channel_t *(*mm_app_add_channel_lib_t) (mm_camera_test_obj_t * test_obj,
                                                          mm_camera_channel_type_t ch_type,
                                                          mm_camera_channel_attr_t * attr,
                                                          mm_camera_buf_notify_t channel_cb, void *userdata);
typedef mm_camera_stream_t *(*mm_app_add_stream_lib_t) (mm_camera_test_obj_t * test_obj, mm_camera_channel_t * channel);
typedef int (*mm_app_config_stream_lib_t) (mm_camera_test_obj_t * test_obj,
                                           mm_camera_channel_t * channel,
                                           mm_camera_stream_t * stream, mm_camera_stream_config_t * config);
typedef int (*mm_app_del_stream_lib_t) (mm_camera_test_obj_t * test_obj,
                                        mm_camera_channel_t * channel, mm_camera_stream_t * stream);
typedef int (*mm_app_del_channel_lib_t) (mm_camera_test_obj_t * test_obj, mm_camera_channel_t * channel);
typedef int (*mm_app_start_channel_lib_t) (mm_camera_test_obj_t * test_obj, mm_camera_channel_t * channel);
typedef int (*mm_app_stream_initbuf_lib_t) (cam_frame_len_offset_t * frame_offset_info,
                                            uint8_t * num_bufs,
                                            uint8_t ** initial_reg_flag,
                                            mm_camera_buf_def_t ** bufs,
                                            mm_camera_map_unmap_ops_tbl_t * ops_tbl, void *user_data);
typedef int32_t(*mm_app_stream_deinitbuf_lib_t) (mm_camera_map_unmap_ops_tbl_t * ops_tbl, void *user_data);
typedef int (*mm_app_cache_ops_lib_t) (mm_camera_app_meminfo_t * mem_info, unsigned int cmd);
typedef int32_t(*mm_app_stream_clean_invalidate_buf_lib_t) (int index, void *user_data);
typedef int32_t(*mm_app_stream_invalidate_buf_lib_t) (int index, void *user_data);
typedef int (*mm_app_start_capture_lib_t) (mm_camera_test_obj_t * test_obj, uint8_t num_snapshots);
typedef int (*mm_camera_app_wait_lib_t) ();
typedef int (*mm_app_stop_capture_lib_t) (mm_camera_test_obj_t * test_obj);
typedef int (*mm_app_set_params_t) (mm_camera_test_obj_t * test_obj, cam_intf_parm_type_t param_type, int32_t value);

void *libqcamera = NULL;
mm_app_open_lib_t mm_app_open_lib;
mm_app_close_lib_t mm_app_close_lib;
mm_app_stop_preview_lib_t mm_app_stop_preview_lib;
mm_app_add_channel_lib_t mm_app_add_channel_lib;
mm_app_add_stream_lib_t mm_app_add_stream_lib;
mm_app_config_stream_lib_t mm_app_config_stream_lib;
mm_app_del_stream_lib_t mm_app_del_stream_lib;
mm_app_del_channel_lib_t mm_app_del_channel_lib;
mm_app_start_channel_lib_t mm_app_start_channel_lib;
mm_app_stream_initbuf_lib_t mm_app_stream_initbuf_lib;
mm_app_stream_deinitbuf_lib_t mm_app_stream_deinitbuf_lib;
mm_app_cache_ops_lib_t mm_app_cache_ops_lib;
mm_app_stream_clean_invalidate_buf_lib_t mm_app_stream_clean_invalidate_buf_lib;
mm_app_stream_invalidate_buf_lib_t mm_app_stream_invalidate_buf_lib;
mm_app_start_capture_lib_t mm_app_start_capture_lib;
mm_camera_app_wait_lib_t mm_camera_app_wait_lib;
mm_app_stop_capture_lib_t mm_app_stop_capture_lib;
mm_app_set_params_t mm_app_set_params_lib;

//------------------------------------

mmi_window *window;

int module_main(mmi_module * mod);
extern "C" void __attribute__ ((constructor)) register_module(void);

mmi_module *g_module;
void register_module(void) {
    g_module = mmi_module::register_module(LOG_TAG, module_main);
}

/** btn_cb_pass:
 *  @btn: button pointer
 *
 *  handle pass button on-click event
 **/
void btn_cb_pass(void *btn) {
    gRetVal = 0;
    sem_post(&g_sem);
    return;
}

/** btn_cb_fail:
 *  @btn: button pointer
 *
 *  handle fail button on-click event
 **/
void btn_cb_fail(void *btn) {
    gRetVal = -1;
    sem_post(&g_sem);
    return;
}

/** btn_cb_takepicture:
 *  @btn: button pointer
 *
 *  handle take-picture button on-click event
 **/
void btn_cb_takepicture(void *btn) {
    previewOrSnap = 1;
    sem_post(&g_sem);
    return;
}

/** allocBuffers:
 *
 *  allocate all buffers needed for preview,JPEG enc, etc...
 **/
void allocBuffers() {
    int stride, scanline;

    stride = PAD_TO_SIZE(PREVIEW_WIDTH, CAM_PAD_TO_16);
    scanline = PAD_TO_SIZE(PREVIEW_HEIGHT, CAM_PAD_TO_2);
    mp0len = stride * scanline;
    stride = PAD_TO_SIZE(PREVIEW_WIDTH, CAM_PAD_TO_16);
    scanline = PAD_TO_SIZE(PREVIEW_HEIGHT / 2, CAM_PAD_TO_2);
    mp1len = stride * scanline;

    g_pPreviewYUV420 = new uint8_t[mp0len + mp1len];
    g_pPreview_Y = new uint8_t[PREVIEW_BUF_SIZE];
    g_pPreview_UV = new uint8_t[PREVIEW_BUF_SIZE / 2];
    g_pPreviewRGB8888 = new uint8_t[PREVIEW_BUF_SIZE * 4];
    g_pPreviewRGB565 = new uint16_t[PREVIEW_BUF_SIZE];
    g_pRotate90_tmp = new uint8_t[PREVIEW_BUF_SIZE * 4];
}

void deallocBuffers() {
    delete[]g_pPreviewYUV420;
    delete[]g_pPreview_Y;
    delete[]g_pPreview_UV;

    delete[]g_pPreviewRGB8888;
    delete[]g_pPreviewRGB565;
    delete[]g_pRotate90_tmp;
}

/** subMenu:
 *
 *  create mmi UI and buffers needed
 **/
void subMenu() {
    class mmi_button *btn;

    window = new mmi_window();
    int width = window->get_width();
    int height = window->get_height();

    g_module->add_window(window);

    mmi_rect_t rect;

    rect.x = 0;
    rect.y = height - height / 8;
    rect.w = width / 3;
    rect.h = height / 8;
    btn = new mmi_button(rect, "pass", btn_cb_pass);
    btn->set_color(0, 125, 125, 255);
    g_module->add_btn(btn);

    rect.x = width / 3;
    btn = new mmi_button(rect, "fail", btn_cb_fail);
    btn->set_color(125, 125, 0, 255);
    g_module->add_btn(btn);

    rect.x = width * 2 / 3;
    btn = new mmi_button(rect, "snapshot", btn_cb_takepicture);
    btn->set_color(0, 0, 255, 255);
    g_module->add_btn(btn);
    //allocate needed buffers
    allocBuffers();
}

/** destroyMenu:
 *
 *  destroy resources
 **/
void destroyMenu() {
    deallocBuffers();
    g_module->clean_source();
}

/** YUV2_Y_UV:
 *  @raw_buf: yuv420sp raw data
 *
 *  split yuv420sp data to Y and CbCr components
 **/
void YUV2_Y_UV(uint8_t * raw_buf) {
    int32_t stride, scanline;
    uint8_t *ptr;

    stride = PAD_TO_SIZE(PREVIEW_WIDTH, CAM_PAD_TO_16);
    scanline = PAD_TO_SIZE(PREVIEW_HEIGHT, CAM_PAD_TO_2);

    ptr = raw_buf;
    for(int h = 0; h < PREVIEW_HEIGHT; h++) {
        memcpy(&g_pPreview_Y[h * PREVIEW_WIDTH], ptr, PREVIEW_WIDTH);
        ptr += stride;
    }

    ptr = raw_buf + stride * scanline;
    stride = PAD_TO_SIZE(PREVIEW_WIDTH, CAM_PAD_TO_16);
    scanline = PAD_TO_SIZE(PREVIEW_HEIGHT / 2, CAM_PAD_TO_2);

    for(int h = 0; h < PREVIEW_HEIGHT / 2; h++) {
        memcpy(&g_pPreview_UV[h * PREVIEW_WIDTH], ptr, PREVIEW_WIDTH);
        ptr += stride;
    }
}

/** yuvtorgb8888:
 *  @width: frame width
 *  @height: frame height
 *  @src_y: pointer to Y componet datas
 *  @src_yuv: pointer to cbcr component datas
 *  @dest_rgb8888: pointer to dest buffer
 *
 *  translate yuv data to rgb8888
 **/
void yuvtorgb8888(int width, int height, unsigned char *src_y, unsigned char *src_uv, unsigned char *dest_rgb8888) {
    uint32_t i, j;
    int r, g, b;
    uint32_t YPOS, UPOS, VPOS;
    uint32_t num = height * width - 1;

    for(i = 0; i < height; i++) {
        for(j = 0; j < width; j++) {
            YPOS = i * width + j;
            VPOS = (i / 2) * width + (j & 0xFFFE);
            UPOS = (i / 2) * width + (j | 0x0001);
            r = src_y[YPOS] + (1.370705 * (src_uv[VPOS] - 128));
            g = src_y[YPOS] - (0.698001 * (src_uv[VPOS] - 128)) - (0.337633 * (src_uv[UPOS] - 128));
            b = src_y[YPOS] + (1.732446 * (src_uv[UPOS] - 128));

            if(r > 255)
                r = 255;
            if(r < 0)
                r = 0;

            if(g > 255)
                g = 255;
            if(g < 0)
                g = 0;

            if(b > 255)
                b = 255;
            if(b < 0)
                b = 0;

            dest_rgb8888[num * 4] = r;
            dest_rgb8888[num * 4 + 1] = g;
            dest_rgb8888[num * 4 + 2] = b;
            dest_rgb8888[num * 4 + 3] = 0xFF;

            num--;
        }
    }
    num++;
}

/** rgb8888to565:
 *  @p8888: pointer to rgb8888 data
 *  @p565: pointer to rgb565 data
 *
 *  translate rgb8888 data to rgb565
 **/
void rgb8888to565(uint8_t * p8888, uint16_t * p565) {
    uint32_t i, j;
    uint32_t r, g, b;

    memset(p565, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT * 2);
    for(i = 0; i < PREVIEW_WIDTH; i++) {
        for(j = 0; j < PREVIEW_HEIGHT; j++) {
            r = p8888[(i * PREVIEW_HEIGHT + j) * 4];
            g = p8888[(i * PREVIEW_HEIGHT + j) * 4 + 1];
            b = p8888[(i * PREVIEW_HEIGHT + j) * 4 + 2];

            r = r * 32.0 / 256;
            g = g * 64.0 / 256;
            b = b * 32.0 / 256;

            p565[i * PREVIEW_HEIGHT + j] |= (r << 11) & 0xf800;
            p565[i * PREVIEW_HEIGHT + j] |= (g << 5) & 0x07e0;
            p565[i * PREVIEW_HEIGHT + j] |= b & 0x001f;
        }
    }
}


/** rotate_screen:
 *  @data: pointer to buffer to be rotated
 *  @w: frame width
 *  @h: frame height
 *  @n: bytes per pixel
 *
 *  rotate the frame by 90or270 degree
 **/
void rotate_screen(unsigned char *data, int w, int h, int n) {
    int nw = h, nh = w;
    int i, j;
    int ni, nj;

    for(i = 0; i < h; i++) {
        for(j = 0; j < w; j++) {
            if (g_sensor_mount_angle == 270)
                ni = j;
            else
                ni = nh - j - 1;
            nj = i;
            unsigned char *src = data + (i * w + j) * n;
            unsigned char *dst = g_pRotate90_tmp + (ni * nw + nj) * n;

            memcpy(dst, src, n);
        }
    }
    memcpy(data, g_pRotate90_tmp, w * h * n);
}

/** mmi_app_load_hal:
*  @my_cam_app : camer handler
*
*  load hal interface
*  Return value: 0 -- success
*				 -1 -- failure
**/
int mmi_app_load_hal(mm_camera_app_t * my_cam_app) {
    memset(&my_cam_app->hal_lib, 0, sizeof(hal_interface_lib_t));
    my_cam_app->hal_lib.ptr = dlopen("libmmcamera_interface.so", RTLD_NOW);
    my_cam_app->hal_lib.ptr_jpeg = dlopen("libmmjpeg_interface.so", RTLD_NOW);
    if(!my_cam_app->hal_lib.ptr || !my_cam_app->hal_lib.ptr_jpeg) {
        ALOGE("%s Error opening HAL library %s\n", __func__, dlerror());
        return -MM_CAMERA_E_GENERAL;
    }

    *(void **) &(my_cam_app->hal_lib.get_num_of_cameras) = dlsym(my_cam_app->hal_lib.ptr, "get_num_of_cameras");
    *(void **) &(my_cam_app->hal_lib.mm_camera_open) = dlsym(my_cam_app->hal_lib.ptr, "camera_open");
    *(void **) &(my_cam_app->hal_lib.jpeg_open) = dlsym(my_cam_app->hal_lib.ptr_jpeg, "jpeg_open");

    if(my_cam_app->hal_lib.get_num_of_cameras == NULL ||
       my_cam_app->hal_lib.mm_camera_open == NULL || my_cam_app->hal_lib.jpeg_open == NULL) {
        ALOGE("%s Error loading HAL sym %s\n", __func__, dlerror());
        return -MM_CAMERA_E_GENERAL;
    }

    my_cam_app->num_cameras = my_cam_app->hal_lib.get_num_of_cameras();
    ALOGI("%s: num_cameras = %d\n", __func__, my_cam_app->num_cameras);

    return MM_CAMERA_OK;
}

/** mmi_app_add_preview_stream:
*  @channel : pointer to preview channel
*  @stream_cb: preview data callback
*  @userdata: userdata
*  @num_bufs: number of buffers in this stream
*
*  add a stream for preview channel
*  Return value: the added stream
**/
mm_camera_stream_t *mmi_app_add_preview_stream(mm_camera_channel_t * channel,
                                               mm_camera_buf_notify_t stream_cb, void *userdata, uint8_t num_bufs) {
    int rc = MM_CAMERA_OK;
    mm_camera_stream_t *stream = NULL;
    cam_capability_t *cam_cap = (cam_capability_t *) (camera_obj.cap_buf.buf.buffer);

    //get the sensor's mount angle
    g_sensor_mount_angle = cam_cap->sensor_mount_angle;

    stream = mm_app_add_stream_lib(&camera_obj, channel);
    if(NULL == stream) {
        ALOGE("%s: add stream failed\n", __func__);
        return NULL;
    }
    stream->s_config.mem_vtbl.get_bufs = mm_app_stream_initbuf_lib;
    stream->s_config.mem_vtbl.put_bufs = mm_app_stream_deinitbuf_lib;
    stream->s_config.mem_vtbl.clean_invalidate_buf = mm_app_stream_clean_invalidate_buf_lib;
    stream->s_config.mem_vtbl.invalidate_buf = mm_app_stream_invalidate_buf_lib;
    stream->s_config.mem_vtbl.user_data = (void *) stream;
    stream->s_config.stream_cb = stream_cb;
    stream->s_config.userdata = userdata;
    stream->num_of_bufs = num_bufs;
    stream->s_config.stream_info = (cam_stream_info_t *) stream->s_info_buf.buf.buffer;
    memset(stream->s_config.stream_info, 0, sizeof(cam_stream_info_t));
    stream->s_config.stream_info->stream_type = CAM_STREAM_TYPE_PREVIEW;
    stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_CONTINUOUS;
    stream->s_config.stream_info->fmt = DEFAULT_PREVIEW_FORMAT;
    stream->s_config.stream_info->dim.width = PREVIEW_WIDTH;
    stream->s_config.stream_info->dim.height = PREVIEW_HEIGHT;
    stream->s_config.padding_info = cam_cap->padding_info;
    rc = mm_app_config_stream_lib(&camera_obj, channel, stream, &stream->s_config);
    if(MM_CAMERA_OK != rc) {
        ALOGE("%s:config preview stream err=%d\n", __func__, rc);
        return NULL;
    }
    return stream;
}

/** mmi_app_display:
*  @frame : frame info
*  @frame_idx: frame index
*
*  show the frame data to the display
**/
void mmi_app_display(mm_camera_buf_def_t * frame, int frame_idx) {
    if(frame != NULL) {
        memcpy(g_pPreviewYUV420, (uint8_t *) frame->buffer, mp0len + mp1len);

        //split YUV to Y,UV component
        YUV2_Y_UV(g_pPreviewYUV420);

        //convert Y,UV to rgb8888
        yuvtorgb8888(PREVIEW_WIDTH, PREVIEW_HEIGHT, g_pPreview_Y, g_pPreview_UV, g_pPreviewRGB8888);

        //rotate the image
        rotate_screen(g_pPreviewRGB8888, PREVIEW_WIDTH, PREVIEW_HEIGHT, 4);

        //convert from rgb8888 to rgb565 for display
        rgb8888to565(g_pPreviewRGB8888, g_pPreviewRGB565);

        GGLSurface *mmi_gglsf = window->get_gr_memory_surface();

        if (mode != TEST_MODE_PCBA) {
            window->window_lock();

            if (mmi_gglsf->format == (GGLubyte) GGL_PIXEL_FORMAT_RGBX_8888) {
                for (uint32_t i = 0; i < PREVIEW_WIDTH; i++) {
                    memcpy(mmi_gglsf->data + i * mmi_gglsf->stride * 4,
                           (uint8_t *) g_pPreviewRGB8888 + i * PREVIEW_HEIGHT * 4,
                           PREVIEW_HEIGHT * 4);
                }
            } else {
                for (uint32_t i = 0; i < PREVIEW_WIDTH; i++) {
                    memcpy(mmi_gglsf->data + i * mmi_gglsf->stride * 2,
                           (uint8_t *) g_pPreviewRGB565 + i * PREVIEW_HEIGHT * 2,
                           PREVIEW_HEIGHT * 2);
                }
            }
            window->window_unlock();
        } else {
            // In PCBA mode we probably do not have a display. Ping main
            // thread when preview frame is ready.
            sem_post(&g_sem);
        }
    }
}

/** mmi_app_preview_notify_cb:
*  @bufs : frame info
*  @user_data: userdata
*
*  preview notify callback
**/
static void mmi_app_preview_notify_cb(mm_camera_super_buf_t * bufs, void *user_data) {
    mm_camera_buf_def_t *frame = bufs->bufs[0];
    mm_camera_test_obj_t *pme = (mm_camera_test_obj_t *) user_data;

    mmi_app_display(frame, frame->frame_idx);

    if(MM_CAMERA_OK != pme->cam->ops->qbuf(bufs->camera_handle, bufs->ch_id, frame)) {
        ALOGE("%s: Failed in Preview Qbuf\n", __func__);
    }
    mm_app_cache_ops_lib((mm_camera_app_meminfo_t *) frame->mem_info, ION_IOC_INV_CACHES);

    ALOGE("%s: END\n", __func__);
}

/** mmi_app_add_preview_channel:
*  add preview channel
*
*  Return value: the added channel
**/
mm_camera_channel_t *mmi_app_add_preview_channel() {
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *stream = NULL;

    channel = mm_app_add_channel_lib(&camera_obj, MM_CHANNEL_TYPE_PREVIEW, NULL, NULL, NULL);
    if(NULL == channel) {
        ALOGE("%s: add channel failed", __func__);
        return NULL;
    }
    stream = mmi_app_add_preview_stream(channel, mmi_app_preview_notify_cb, (void *) &camera_obj, PREVIEW_BUF_NUM);

    if(NULL == stream) {
        ALOGE("%s: add stream failed\n", __func__);
        mm_app_del_channel_lib(&camera_obj, channel);
        return NULL;
    }
    return channel;
}

/** mmi_app_add_preview_channel:
*  start preview
*
*  Return value: 0--success
                       -1--failure
**/
int mmi_start_preview() {
    int rc = MM_CAMERA_OK;
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *stream = NULL;
    uint8_t i;

    channel = mmi_app_add_preview_channel();
    if(NULL == channel) {
        ALOGE("%s: add channel failed", __func__);
        return -MM_CAMERA_E_GENERAL;
    }

    rc = mm_app_start_channel_lib(&camera_obj, channel);
    if(MM_CAMERA_OK != rc) {
        ALOGE("%s:start preview failed rc=%d\n", __func__, rc);
        for(i = 0; i < channel->num_streams; i++) {
            stream = &channel->streams[i];
            mm_app_del_stream_lib(&camera_obj, channel, stream);
        }
        mm_app_del_channel_lib(&camera_obj, channel);
        return rc;
    }
    return rc;
}

/** get_input:
*  get the input parameters from mmi.cfg file
*
**/
void get_input() {
    char temp[256] = { 0 };
    parse_parameter(mmi_config::query_config_value(g_module->get_domain(), "parameter"), paras);
    get_para_value(paras, "device", temp, sizeof(temp), NULL);
    if(temp != NULL)
        strlcpy(input.device, temp, sizeof(input.device));
    else
        strlcpy(input.device, DEFAULT_CAMERA_DEVICE, sizeof(input.device));
}

int init_camera() {
    int cam_idx;

    cam_idx = input.device[10] - '0';

    //load .so and functions
    libqcamera = dlopen("libmm-qcamera.so", RTLD_NOW);
    if(!libqcamera) {
        ALOGE("FATAL ERROR: could not dlopen liboemcamera.so: %s", dlerror());
        return CASE_FAIL;
    }

    mm_app_open_lib = (mm_app_open_lib_t) dlsym(libqcamera, "mm_app_open");
    if(!mm_app_open_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_open_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_close_lib = (mm_app_close_lib_t) dlsym(libqcamera, "mm_app_close");
    if(!mm_app_close_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_close_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_stop_preview_lib = (mm_app_stop_preview_lib_t) dlsym(libqcamera, "mm_app_stop_preview");
    if(!mm_app_stop_preview_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_stop_preview_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_add_channel_lib = (mm_app_add_channel_lib_t) dlsym(libqcamera, "mm_app_add_channel");
    if(!mm_app_add_channel_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_add_channel_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_add_stream_lib = (mm_app_add_stream_lib_t) dlsym(libqcamera, "mm_app_add_stream");
    if(!mm_app_add_stream_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_add_stream_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_config_stream_lib = (mm_app_config_stream_lib_t) dlsym(libqcamera, "mm_app_config_stream");
    if(!mm_app_config_stream_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_config_stream_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_del_stream_lib = (mm_app_del_stream_lib_t) dlsym(libqcamera, "mm_app_del_stream");
    if(!mm_app_del_stream_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_del_stream_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_del_channel_lib = (mm_app_del_channel_lib_t) dlsym(libqcamera, "mm_app_del_channel");
    if(!mm_app_del_channel_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_del_channel_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_start_channel_lib = (mm_app_start_channel_lib_t) dlsym(libqcamera, "mm_app_start_channel");
    if(!mm_app_start_channel_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_start_channel_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_stream_initbuf_lib = (mm_app_stream_initbuf_lib_t) dlsym(libqcamera, "mm_app_stream_initbuf");
    if(!mm_app_stream_initbuf_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_stream_initbuf_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_stream_deinitbuf_lib = (mm_app_stream_deinitbuf_lib_t) dlsym(libqcamera, "mm_app_stream_deinitbuf");
    if(!mm_app_stream_deinitbuf_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_stream_deinitbuf_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_cache_ops_lib = (mm_app_cache_ops_lib_t) dlsym(libqcamera, "mm_app_cache_ops");
    if(!mm_app_cache_ops_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_cache_ops_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_stream_clean_invalidate_buf_lib =
        (mm_app_stream_clean_invalidate_buf_lib_t) dlsym(libqcamera, "mm_app_stream_clean_invalidate_buf");
    if(!mm_app_stream_clean_invalidate_buf_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_stream_clean_invalidate_buf_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_stream_invalidate_buf_lib =
        (mm_app_stream_invalidate_buf_lib_t) dlsym(libqcamera, "mm_app_stream_invalidate_buf");
    if(!mm_app_stream_invalidate_buf_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_stream_invalidate_buf_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_start_capture_lib = (mm_app_start_capture_lib_t) dlsym(libqcamera, "mm_app_start_capture");
    if(!mm_app_start_capture_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_start_capture_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_camera_app_wait_lib = (mm_camera_app_wait_lib_t) dlsym(libqcamera, "mm_camera_app_wait");
    if(!mm_camera_app_wait_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_camera_app_wait_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_stop_capture_lib = (mm_app_stop_capture_lib_t) dlsym(libqcamera, "mm_app_stop_capture");
    if(!mm_app_stop_capture_lib) {
        ALOGE("FATAL ERROR: could not dlsym mm_app_stop_capture_lib\n");
        gRetVal = CASE_FAIL;
        goto error;
    }

    mm_app_set_params_lib = (mm_app_set_params_t) dlsym(libqcamera, "mm_app_set_params");
    if(!mm_app_set_params_lib) {
        ALOGE("WARNING: could not dlsym mm_app_set_params\n");
    }
    /*load hal interface*/
    mm_camera_app_t my_cam_app;

    memset(&my_cam_app, 0, sizeof(mm_camera_app_t));
    if((mmi_app_load_hal(&my_cam_app) != MM_CAMERA_OK)) {
        ALOGE("%s:mm_app_init err\n", __func__);
        goto error;
    }
    /*open camera*/
    gRetVal = mm_app_open_lib(&my_cam_app, cam_idx - 1, &camera_obj);
    if(gRetVal != MM_CAMERA_OK) {
        goto error;
    }
    /*set EV*/
    if (mm_app_set_params_lib != NULL)
      mm_app_set_params_lib(&camera_obj, CAM_INTF_PARM_AEC_ALGO_TYPE, 0);

    error:
      dlclose(libqcamera);

      return gRetVal;
}

void do_capture() {
    jpeg_test_input_t jpeg_input[1];
    char jpeg_name[256];

    snprintf(jpeg_name, sizeof(jpeg_name), "/data/FTM_AP/%s.JPG", g_module->get_domain());
    jpeg_input[0].width = SNAPSHOT_WIDTH;
    jpeg_input[0].height = SNAPSHOT_HEIGHT;
    jpeg_input[0].out_filename = jpeg_name;
    encode_mmi(&jpeg_input[0], g_pPreviewYUV420);
}

int manual_test() {

    if (init_camera()) {
        return CASE_FAIL;
    }
    sem_init(&g_sem, 0, 0);
    subMenu();

    STAR_PREVIEW:
    /*start preview*/
    mmi_start_preview();
    /*wait*/
    sem_wait(&g_sem);

    /*stop preview*/
    mm_app_stop_preview_lib(&camera_obj);

    if (previewOrSnap == 1) {
        do_capture();
        previewOrSnap = 0;
        goto STAR_PREVIEW;
    }

    destroyMenu();
    mm_app_close_lib(&camera_obj);

    return gRetVal;
}

int auto_test() {
    struct timespec ts;

    if (init_camera()) {
        return PCBA_FAIL_TO_OEPN_CAMERA;
    }
    allocBuffers();
    if (mmi_start_preview()) {
        gRetVal = PCBA_FAIL_TO_OEPN_CAMERA;
        goto cleanup;
    }
    sem_init(&g_sem, 0, 0);

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        goto cleanup;
    ts.tv_sec += CAMERA_TIMEOUT;
    if (sem_timedwait(&g_sem, &ts)) {
        gRetVal = PCBA_FAIL_TO_OEPN_CAMERA;
        goto cleanup;
    }

    do_capture();

cleanup:
    mm_app_stop_preview_lib(&camera_obj);
    mm_app_close_lib(&camera_obj);
    deallocBuffers();

    return gRetVal;
}


/** camera_main:
*  camera module entry
*
*  Return Value: 0---pass
                       -1---fail
**/
int module_main(mmi_module * mod) {
    if(mod == NULL)
        return CASE_FAIL;

    g_module = mod;
    mode = g_module->get_run_mode();

    get_input();
    ALOGI("DEVICE:%s\n", input.device);

    if (mode == TEST_MODE_PCBA) {
        return auto_test();
    } else {
        return manual_test();
    }

    return gRetVal;
}
/** buffer_allocate:
 *
 *     @p_buffer: ION buffer
 *
 *  Return:
 *     buffer address
 *
 *  Description:
 *      allocates ION buffer
 *
 **/
void *buffer_allocate(buffer_test_t * p_buffer) {
    void *l_buffer = NULL;

    int lrc = 0;
    struct ion_handle_data lhandle_data;

    p_buffer->alloc.len = p_buffer->size;
    p_buffer->alloc.align = 4096;
    p_buffer->alloc.flags = 0;
    p_buffer->alloc.heap_mask = 0x1 << ION_IOMMU_HEAP_ID;

    p_buffer->ion_fd = open("/dev/ion", O_RDONLY);
    if(p_buffer->ion_fd < 0) {
        ALOGE("%s :Ion open failed", __func__);
        goto ION_ALLOC_FAILED;
    }

    /* Make it page size aligned */
    p_buffer->alloc.len = (p_buffer->alloc.len + 4095) & (~4095);
    lrc = ioctl(p_buffer->ion_fd, ION_IOC_ALLOC, &p_buffer->alloc);
    if(lrc < 0) {
        ALOGE("%s :ION allocation failed len %d", __func__, p_buffer->alloc.len);
        goto ION_ALLOC_FAILED;
    }

    p_buffer->ion_info_fd.handle = p_buffer->alloc.handle;
    lrc = ioctl(p_buffer->ion_fd, ION_IOC_SHARE, &p_buffer->ion_info_fd);
    if(lrc < 0) {
        ALOGE("%s :ION map failed %s", __func__, strerror(errno));
        goto ION_MAP_FAILED;
    }

    p_buffer->p_pmem_fd = p_buffer->ion_info_fd.fd;

    l_buffer = mmap(NULL, p_buffer->alloc.len, PROT_READ | PROT_WRITE, MAP_SHARED, p_buffer->p_pmem_fd, 0);

    if(l_buffer == MAP_FAILED) {
        ALOGE("%s :ION_MMAP_FAILED: %s (%d)", __func__, strerror(errno), errno);
        goto ION_MAP_FAILED;
    }

    return l_buffer;

  ION_MAP_FAILED:
    lhandle_data.handle = p_buffer->ion_info_fd.handle;
    ioctl(p_buffer->ion_fd, ION_IOC_FREE, &lhandle_data);
    return NULL;
  ION_ALLOC_FAILED:
    return NULL;

}

/** buffer_deallocate:
 *
 *     @p_buffer: ION buffer
 *
 *  Return:
 *     buffer address
 *
 *  Description:
 *      deallocates ION buffer
 *
 **/
int buffer_deallocate(buffer_test_t * p_buffer) {
    int lrc = 0;
    int lsize = (p_buffer->size + 4095) & (~4095);

    struct ion_handle_data lhandle_data;

    lrc = munmap(p_buffer->addr, lsize);

    close(p_buffer->ion_info_fd.fd);

    lhandle_data.handle = p_buffer->ion_info_fd.handle;
    ioctl(p_buffer->ion_fd, ION_IOC_FREE, &lhandle_data);

    close(p_buffer->ion_fd);
    return lrc;
}

static void mm_jpeg_encode_callback(jpeg_job_status_t status,
                                    uint32_t client_hdl, uint32_t jobId, mm_jpeg_output_t * p_output, void *userData) {
    mm_jpeg_intf_test_t *p_obj = (mm_jpeg_intf_test_t *) userData;

    if(status == JPEG_JOB_STATUS_ERROR) {
        ALOGE("%s:%d] Encode error", __func__, __LINE__);
    } else {
        ALOGE("%s:%d] Encode success file%s addr %p len %d",
              __func__, __LINE__, p_obj->out_filename, p_output->buf_vaddr, p_output->buf_filled_len);
        DUMP_TO_FILE(p_obj->out_filename, p_output->buf_vaddr, p_output->buf_filled_len);
    }
    g_i++;
    if(g_i >= g_count) {
        ALOGE("%s:%d] Signal the thread", __func__, __LINE__);
        pthread_cond_signal(&p_obj->cond);
    }
}

int mm_jpeg_test_alloc(buffer_test_t * p_buffer, int use_pmem) {
    int ret = 0;

    /*Allocate buffers */
    if(use_pmem) {
        p_buffer->addr = (uint8_t *) buffer_allocate(p_buffer);
        if(NULL == p_buffer->addr) {
            ALOGE("%s:%d] Error", __func__, __LINE__);
            return -1;
        }
    } else {
        /* Allocate heap memory */
        p_buffer->addr = (uint8_t *) malloc(p_buffer->size);
        if(NULL == p_buffer->addr) {
            ALOGE("%s:%d] Error", __func__, __LINE__);
            return -1;
        }
    }
    return ret;
}

void mm_jpeg_test_free(buffer_test_t * p_buffer) {
    if(p_buffer->addr == NULL)
        return;

    if(p_buffer->p_pmem_fd > 0)
        buffer_deallocate(p_buffer);
    else
        free(p_buffer->addr);

    memset(p_buffer, 0x0, sizeof(buffer_test_t));
}

int mm_jpeg_test_read(mm_jpeg_intf_test_t * p_obj, void *data) {
    memcpy(p_obj->input.addr, data, p_obj->input.size);
    return 0;
}

static int encode_init(jpeg_test_input_t * p_input, mm_jpeg_intf_test_t * p_obj, void *data) {
    int rc = -1;
    int size = p_input->width * p_input->height;
    mm_jpeg_encode_params_t *p_params = &p_obj->params;
    mm_jpeg_encode_job_t *p_job_params = &p_obj->job.encode_job;

    p_obj->filename = p_input->filename;
    p_obj->width = p_input->width;
    p_obj->height = p_input->height;
    p_obj->out_filename = p_input->out_filename;
    p_obj->use_ion = 1;

    pthread_mutex_init(&p_obj->lock, NULL);
    pthread_cond_init(&p_obj->cond, NULL);

    /* allocate buffers */
    p_obj->input.size = size * 3 / 2;
    rc = mm_jpeg_test_alloc(&p_obj->input, p_obj->use_ion);
    if(rc) {
        ALOGE("%s:%d] Error", __func__, __LINE__);
        return -1;
    }

    p_obj->output.size = size * 3 / 2;
    rc = mm_jpeg_test_alloc(&p_obj->output, 0);
    if(rc) {
        ALOGE("%s:%d] Error", __func__, __LINE__);
        return -1;
    }

    rc = mm_jpeg_test_read(p_obj, data);
    if(rc) {
        ALOGE("%s:%d] Error", __func__, __LINE__);
        return -1;
    }

    /* set encode parameters */
    p_params->jpeg_cb = mm_jpeg_encode_callback;
    p_params->userdata = p_obj;
    p_params->color_format = MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V2;

    /* dest buffer config */
    p_params->dest_buf[0].buf_size = p_obj->output.size;
    p_params->dest_buf[0].buf_vaddr = p_obj->output.addr;
    p_params->dest_buf[0].fd = p_obj->output.p_pmem_fd;
    p_params->dest_buf[0].index = 0;
    p_params->num_dst_bufs = 1;

    /* src buffer config */
    p_params->src_main_buf[0].buf_size = p_obj->input.size;
    p_params->src_main_buf[0].buf_vaddr = p_obj->input.addr;
    p_params->src_main_buf[0].fd = p_obj->input.p_pmem_fd;
    p_params->src_main_buf[0].index = 0;
    p_params->src_main_buf[0].format = MM_JPEG_FMT_YUV;
    p_params->src_main_buf[0].offset.mp[0].len = size;
    p_params->src_main_buf[0].offset.mp[1].len = size >> 1;
    p_params->num_src_bufs = 1;

    p_params->encode_thumbnail = 0;
    p_params->quality = 80;

    p_job_params->dst_index = 0;
    p_job_params->src_index = 0;
    p_job_params->rotation = 0;

    p_params->main_dim.src_dim.width = p_obj->width;
    p_params->main_dim.src_dim.height = p_obj->height;
    p_params->main_dim.dst_dim.width = p_obj->width;
    p_params->main_dim.dst_dim.height = p_obj->height;

    /* main dimension */
    p_job_params->main_dim.src_dim.width = p_obj->width;
    p_job_params->main_dim.src_dim.height = p_obj->height;
    p_job_params->main_dim.dst_dim.width = p_obj->width;
    p_job_params->main_dim.dst_dim.height = p_obj->height;
    p_job_params->main_dim.crop.top = 0;
    p_job_params->main_dim.crop.left = 0;
    p_job_params->main_dim.crop.width = p_obj->width;
    p_job_params->main_dim.crop.height = p_obj->height;

    /* thumb dimension */
    p_job_params->thumb_dim.src_dim.width = p_obj->width;
    p_job_params->thumb_dim.src_dim.height = p_obj->height;
    p_job_params->thumb_dim.dst_dim.width = 512;
    p_job_params->thumb_dim.dst_dim.height = 384;
    p_job_params->thumb_dim.crop.top = 0;
    p_job_params->thumb_dim.crop.left = 0;
    p_job_params->thumb_dim.crop.width = p_obj->width;
    p_job_params->thumb_dim.crop.height = p_obj->height;
    return 0;
}

typedef uint32_t(*jpeg_open_t) (mm_jpeg_ops_t * ops);

static int encode_mmi(jpeg_test_input_t * p_input, void *data) {
    int rc = 0;
    mm_jpeg_intf_test_t jpeg_obj;
    int i = 0;

    memset(&jpeg_obj, 0x0, sizeof(jpeg_obj));
    rc = encode_init(p_input, &jpeg_obj, data);
    if(rc) {
        ALOGE("%s:%d] Error", __func__, __LINE__);
        return -1;
    }
    /*open mm-jpeg-interface.so*/

    jpeg_open_t jpeg_open_func;
    void *libqcamera = dlopen("libmmjpeg_interface.so", RTLD_NOW);

    if(!libqcamera) {
        ALOGE("FATAL ERROR: could not dlopen: %s\n", dlerror());
        return -1;
    } else {
        ALOGE("open libmmjpeg_interface.so suc.\n");
    }
    jpeg_open_func = (jpeg_open_t) dlsym(libqcamera, "jpeg_open");
    if(!jpeg_open_func) {
        ALOGE("FATAL ERROR: could not dlsym \n");
        return -1;
    } else {
        ALOGE("dlsym jpeg_open suc.\n");
    }

    jpeg_obj.handle = jpeg_open_func(&jpeg_obj.ops);

    if(jpeg_obj.handle == 0) {
        ALOGE("%s:%d] Error", __func__, __LINE__);
        goto end;
    }

    rc = jpeg_obj.ops.create_session(jpeg_obj.handle, &jpeg_obj.params, &jpeg_obj.job.encode_job.session_id);
    if(jpeg_obj.job.encode_job.session_id == 0) {
        ALOGE("%s:%d] Error", __func__, __LINE__);
        goto end;
    }

    for(i = 0; i < g_count; i++) {
        jpeg_obj.job.job_type = JPEG_JOB_TYPE_ENCODE;
        rc = jpeg_obj.ops.start_job(&jpeg_obj.job, &jpeg_obj.job_id[i]);
        if(rc) {
            ALOGE("%s:%d] Error", __func__, __LINE__);
            goto end;
        }
    }

    ALOGI("waiting...\n");
    pthread_mutex_lock(&jpeg_obj.lock);
    pthread_cond_wait(&jpeg_obj.cond, &jpeg_obj.lock);
    pthread_mutex_unlock(&jpeg_obj.lock);

    jpeg_obj.ops.destroy_session(jpeg_obj.job.encode_job.session_id);

    jpeg_obj.ops.close(jpeg_obj.handle);


  end:
    mm_jpeg_test_free(&jpeg_obj.input);
    mm_jpeg_test_free(&jpeg_obj.output);
    return 0;
}
