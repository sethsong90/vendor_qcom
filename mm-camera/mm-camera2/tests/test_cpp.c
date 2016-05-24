/*============================================================================

  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#include <sys/mman.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <time.h>
//#include "camera.h"
#include <linux/ion.h>
#include <linux/msm_ion.h>
#include <media/msmb_camera.h>
#include <media/msmb_pproc.h>
//#include "cam_mmap.h"
#include "cam_list.h"
#include "camera_dbg.h"
#include "pproc_interface.h"
#include "cpp.h"
#include "module_pproc_common.h"

#include "mct_module.h"
#include "mct_stream.h"
#include "mct_pipeline.h"
#include "module_cpp.h"
#include "camera_dbg.h"

#undef CDBG
#define CDBG ALOGE
#undef CDBG_ERROR
#define CDBG_ERROR ALOGE

extern int test_isp_init_modules();

struct v4l2_frame_buffer {
  struct v4l2_buffer buffer;
  unsigned long addr[VIDEO_MAX_PLANES];
  uint32_t size;
  struct ion_allocation_data ion_alloc[VIDEO_MAX_PLANES];
  struct ion_fd_data fd_data[VIDEO_MAX_PLANES];
};

struct cpp_plane_info_t frame_640_480_r0_y = {
  .src_width = 320,
  .src_height = 240,
  .src_stride = 320,
  .dst_width = 320,
  .dst_height = 240,
  .dst_stride = 320,
  .rotate = 0,
  .mirror = 0,
  .prescale_padding = 0,
  .postscale_padding = 0,
  .h_scale_ratio = 1,
  .v_scale_ratio = 1,
  .h_scale_initial_phase = 0,
  .v_scale_initial_phase = 0,
  .maximum_dst_stripe_height = 240,
  .input_plane_fmt = PLANE_Y,
  .source_address = 0,
  .destination_address = 0,
};

struct cpp_plane_info_t frame_640_480_r0_cbcr = {
  .src_width = 160,
  .src_height = 120,
  .src_stride = 320,
  .dst_width = 160,
  .dst_height = 120,
  .dst_stride = 320,
  .rotate = 0,
  .mirror = 0,
  .prescale_padding = 0,
  .postscale_padding = 0,
  .h_scale_ratio = 1,
  .v_scale_ratio = 1,
  .h_scale_initial_phase = 0,
  .v_scale_initial_phase = 0,
  .maximum_dst_stripe_height = 120,
  .input_plane_fmt = PLANE_CBCR,
  .source_address = 0x12C00,
  .destination_address = 0x12C00,
};

pthread_cond_t frame_done_cond;
pthread_mutex_t mutex;

typedef struct {
  void *ptr;
  CPP_STATUS (*cpp_get_instance)(uint32_t *cpp_client_inst_id);
  CPP_STATUS (*cpp_free_instance)(uint32_t cpp_client_inst_id);
  CPP_STATUS (*cpp_process_frame)(cpp_process_queue_t *new_frame);
  CPP_STATUS (*cpp_client_frame_finish)(uint32_t client_id, uint32_t frame_id);
  void (*cpp_prepare_frame_info)(struct cpp_plane_info_t *in_info,
                                 struct msm_cpp_frame_info_t *out_info);
} cpp_lib_t;

typedef struct {
  uint32_t input_width;
  uint32_t input_height;
  uint32_t process_window_first_pixel;
  uint32_t process_window_first_line;
  uint32_t process_window_width;
  uint32_t process_window_height;
  uint16_t rotation;
  uint16_t mirror;
  double h_scale_ratio;
  double v_scale_ratio;
  char input_filename[256];
  char output_filename[256];
  double noise_profile[4];
  double luma_weight;
  double chroma_weight;
  double denoise_ratio;
  cpp_asf_mode asf_mode;
  double sharpness_ratio;
  int run;
} cpp_testcase_input_t;

uint8_t *do_mmap_ion(int ion_fd, struct ion_allocation_data *alloc,
  struct ion_fd_data *ion_info_fd, int *mapFd)
{
  void *ret; /* returned virtual address */
  int rc = 0;
  struct ion_handle_data handle_data;

  /* to make it page size aligned */
  alloc->len = (alloc->len + 4095) & (~4095);
  rc = ioctl(ion_fd, ION_IOC_ALLOC, alloc);
  if (rc < 0) {
    CDBG_ERROR("ION allocation failed\n");
    goto ION_ALLOC_FAILED;
  }

  ion_info_fd->handle = alloc->handle;
  rc = ioctl(ion_fd, ION_IOC_SHARE, ion_info_fd);
  if (rc < 0) {
    CDBG_ERROR("ION map failed %s\n", strerror(errno));
    goto ION_MAP_FAILED;
  }
  *mapFd = ion_info_fd->fd;
  ret = mmap(NULL,
    alloc->len,
    PROT_READ  | PROT_WRITE,
    MAP_SHARED,
    *mapFd,
    0);

  if (ret == MAP_FAILED) {
    CDBG_ERROR("ION_MMAP_FAILED: %s (%d)\n", strerror(errno), errno);
    goto ION_MAP_FAILED;
  }

  return ret;

ION_MAP_FAILED:
  handle_data.handle = ion_info_fd->handle;
  ioctl(ion_fd, ION_IOC_FREE, &handle_data);
ION_ALLOC_FAILED:
  return NULL;
}

int do_munmap_ion (int ion_fd, struct ion_fd_data *ion_info_fd,
                   void *addr, size_t size)
{
  int rc = 0;
  rc = munmap(addr, size);
  close(ion_info_fd->fd);

  struct ion_handle_data handle_data;
  handle_data.handle = ion_info_fd->handle;
  ioctl(ion_fd, ION_IOC_FREE, &handle_data);
  return rc;
}

void dump_test_case_params(cpp_testcase_input_t *test_case)
{
  int i;
  CDBG("input_width: %d\n", test_case->input_width);
  CDBG("input_height: %d\n", test_case->input_height);
  CDBG("process_window_first_pixel: %d\n", test_case->process_window_first_pixel);
  CDBG("process_window_first_line: %d\n", test_case->process_window_first_line);
  CDBG("process_window_width: %d\n", test_case->process_window_width);
  CDBG("process_window_height: %d\n", test_case->process_window_height);
  CDBG("rotation: %d\n", test_case->rotation);
  CDBG("mirror: %d\n", test_case->mirror);
  CDBG("h_scale_ratio: %f\n", test_case->h_scale_ratio);
  CDBG("v_scale_ratio: %f\n", test_case->v_scale_ratio);
  CDBG("input_filename: %s\n", test_case->input_filename);
  CDBG("output_filename: %s\n", test_case->output_filename);
  for (i = 0; i < 4; i++) {
    CDBG("noise_profile[%d]: %f\n", i, test_case->noise_profile[i]);
  }
  CDBG("luma_weight: %f\n", test_case->luma_weight);
  CDBG("chroma_weight: %f\n", test_case->chroma_weight);
  CDBG("denoise_ratio: %f\n", test_case->denoise_ratio);
  CDBG("asf_mode: %d\n", test_case->asf_mode);
  CDBG("sharpness_ratio: %f\n", test_case->sharpness_ratio);
  CDBG("run: %d\n", test_case->run);
}

int parse_test_case_file(char **argv, cpp_testcase_input_t *test_case)
{
  char *filename = argv[1];
  char type[256], value[256];
  FILE *fp;

  printf("file name: %s\n", filename);
  test_case->run = 1;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Cannot open test case file!\n");
    return -1;
  }
  while (!feof(fp)) {
    if (fscanf(fp, "%s %s", type, value) != 2)
      break;

    if (!strncmp(type, "input_width", 256)) {
      test_case->input_width = atoi(value);
    } else if (!strncmp(type, "input_height", 256)) {
      test_case->input_height = atoi(value);
    } else if (!strncmp(type, "process_window_first_pixel", 256)) {
      test_case->process_window_first_pixel = atoi(value);
    } else if (!strncmp(type, "process_window_first_line", 256)) {
      test_case->process_window_first_line = atoi(value);
    } else if (!strncmp(type, "process_window_width", 256)) {
      test_case->process_window_width = atoi(value);
    } else if (!strncmp(type, "process_window_height", 256)) {
      test_case->process_window_height = atoi(value);
    } else if (!strncmp(type, "rotation", 256)) {
      test_case->rotation = atoi(value);
    } else if (!strncmp(type, "mirror", 256)) {
      test_case->mirror = atoi(value);
    } else if (!strncmp(type, "h_scale_ratio", 256)) {
      test_case->h_scale_ratio = atof(value);
    } else if (!strncmp(type, "v_scale_ratio", 256)) {
      test_case->v_scale_ratio = atof(value);
    } else if (!strncmp(type, "input_filename", 256)) {
      strncpy(test_case->input_filename, value, 256);
    } else if (!strncmp(type, "output_filename", 256)) {
      strncpy(test_case->output_filename, value, 256);
    } else if (!strncmp(type, "noise_profile0", 256)) {
      test_case->noise_profile[0] = atof(value);
    } else if (!strncmp(type, "noise_profile1", 256)) {
      test_case->noise_profile[1] = atof(value);
    } else if (!strncmp(type, "noise_profile2", 256)) {
      test_case->noise_profile[2] = atof(value);
    } else if (!strncmp(type, "noise_profile3", 256)) {
      test_case->noise_profile[3] = atof(value);
    } else if (!strncmp(type, "luma_weight", 256)) {
      test_case->luma_weight = atof(value);
    } else if (!strncmp(type, "chroma_weight", 256)) {
      test_case->chroma_weight = atof(value);
    } else if (!strncmp(type, "denoise_ratio", 256)) {
      test_case->denoise_ratio = atof(value);
    } else if (!strncmp(type, "asf_mode", 256)) {
      if (!strncmp(value, "ASF_OFF", 256)) {
        test_case->asf_mode = ASF_OFF;
      } else if (!strncmp(value, "ASF_DUAL_FILTER", 256)) {
        test_case->asf_mode = ASF_DUAL_FILTER;
      } else if (!strncmp(value, "ASF_EMBOSS", 256)) {
        test_case->asf_mode = ASF_EMBOSS;
      } else if (!strncmp(value, "ASF_SKETCH", 256)) {
        test_case->asf_mode = ASF_SKETCH;
      } else if (!strncmp(value, "ASF_NEON", 256)) {
        test_case->asf_mode = ASF_NEON;
      }
    } else if (!strncmp(type, "run", 256)) {
      test_case->run = atoi(value);
    } else if (!strncmp(type, "sharpness_ratio", 256)) {
      test_case->sharpness_ratio = atof(value);
    }
  }
  dump_test_case_params(test_case);
  return 0;
}

void cpp_debug_input_info(struct cpp_plane_info_t *frame)
{
  CDBG("CPP: src_width %d\n", frame->src_width);
  CDBG("CPP: src_height %d\n", frame->src_height);
  CDBG("CPP: src_stride %d\n", frame->src_stride);
  CDBG("CPP: dst_width %d\n", frame->dst_width);
  CDBG("CPP: dst_height %d\n", frame->dst_height);
  CDBG("CPP: dst_stride %d\n", frame->dst_stride);
  CDBG("CPP: rotate %d\n", frame->rotate);
  CDBG("CPP: mirror %d\n", frame->mirror);
  CDBG("CPP: prescale_padding %d\n", frame->prescale_padding);
  CDBG("CPP: postscale_padding %d\n", frame->postscale_padding);
  CDBG("CPP: horizontal_scale_ratio %f\n", frame->h_scale_ratio);
  CDBG("CPP: vertical_scale_ratio %f\n", frame->v_scale_ratio);
  CDBG("CPP: horizontal_scale_initial_phase %lld\n",
       frame->horizontal_scale_initial_phase);
  CDBG("CPP: vertical_scale_initial_phase %lld\n",
       frame->vertical_scale_initial_phase);
  CDBG("CPP: maximum_dst_stripe_height %d\n", frame->maximum_dst_stripe_height);
  CDBG("CPP: input_plane_fmt %d\n", frame->input_plane_fmt);
  CDBG("CPP: source_address 0x%x\n", frame->source_address);
  CDBG("CPP: destination_address 0x%x\n", frame->destination_address);
}

int create_frame_info(cpp_testcase_input_t *test_case,
  struct cpp_frame_info_t *frame_info)
{
  int i = 0;
  struct cpp_plane_info_t *plane_info = frame_info->plane_info;
  for(i = 0; i < 2; i++) {
    memset(&plane_info[i], 0, sizeof(struct cpp_plane_info_t));
    plane_info[i].rotate = test_case->rotation / 90;
    plane_info[i].mirror = test_case->mirror;
    plane_info[i].h_scale_ratio = 1/test_case->h_scale_ratio;
    plane_info[i].v_scale_ratio = 1/test_case->v_scale_ratio;
    plane_info[i].h_scale_initial_phase = test_case->process_window_first_pixel;
    plane_info[i].h_scale_initial_phase = test_case->process_window_first_line;
    plane_info[i].src_width = test_case->input_width;
    plane_info[i].src_height = test_case->input_height;
    plane_info[i].src_stride = plane_info[i].src_width;
    plane_info[i].dst_width =
      test_case->process_window_width * test_case->h_scale_ratio;
    plane_info[i].dst_height =
      test_case->process_window_height * test_case->v_scale_ratio;
    plane_info[i].prescale_padding = 22;
    plane_info[i].postscale_padding = 4;
    if (plane_info[i].rotate == 0 || plane_info[i].rotate == 2) {
        plane_info[i].dst_stride = plane_info[i].dst_width;
        plane_info[i].maximum_dst_stripe_height = PAD_TO_2(plane_info[i].dst_height);
    } else {
      plane_info[i].dst_stride = PAD_TO_32(plane_info[i].dst_height/2) * 2;
      plane_info[i].maximum_dst_stripe_height = plane_info[i].dst_width;
    }
  }

  plane_info[0].input_plane_fmt = PLANE_Y;
  plane_info[1].src_width /= 2;
  plane_info[1].src_height /= 2;
  plane_info[1].dst_width /= 2;
  plane_info[1].dst_height /= 2;
  plane_info[1].h_scale_initial_phase /= 2;
  plane_info[1].v_scale_initial_phase /= 2;
  plane_info[1].maximum_dst_stripe_height =
    PAD_TO_2(plane_info[1].maximum_dst_stripe_height / 2);
  plane_info[1].postscale_padding = 0;
  plane_info[1].input_plane_fmt = PLANE_CBCR;
  plane_info[1].source_address =
    plane_info[0].src_width * plane_info[0].src_height;
  plane_info[1].destination_address =
    plane_info[0].dst_stride * plane_info[0].maximum_dst_stripe_height;

  for (i = 0; i < 2; i++) {
    cpp_debug_input_info(&plane_info[i]);
  }

  for (i = 0; i < 4; i++) {
    frame_info->noise_profile[i] = test_case->noise_profile[i];
  }
  frame_info->luma_weight = test_case->luma_weight;
  frame_info->chroma_weight = test_case->chroma_weight;
  frame_info->denoise_ratio = test_case->denoise_ratio;
  frame_info->asf_mode = test_case->asf_mode;
  frame_info->sharpness_ratio = test_case->sharpness_ratio;
  frame_info->num_planes = 2;
  if (frame_info->asf_mode == ASF_SKETCH ||
    frame_info->asf_mode == ASF_EMBOSS) {
    frame_info->num_planes = 1;
  }

  return 0;
}

void frame_done_event(void)
{
  pthread_mutex_lock(&mutex);
  ALOGE("%s: Signal\n", __func__);
  pthread_cond_signal(&frame_done_cond);
  ALOGE("%s: Signal done\n", __func__);
  pthread_mutex_unlock(&mutex);
}

int main(int argc, char * argv[])
{
  int rc = 0, dev_fd = 0;
  int i = 0;
  int ionfd = 0;
  int read_len = 0;
  uint32_t client_id = 0;
  cpp_process_queue_t *new_frame;
  struct msm_cpp_frame_info_t *frame;
  int in_frame_fd, out_frame_fd;
  int in_file_fd, out_file_fd;
  struct v4l2_frame_buffer in_frame, out_frame;
  cpp_testcase_input_t test_case;
  struct cpp_frame_info_t frame_info;

  mct_module_t *cpp_module = NULL;
  module_pproc_common_ctrl_t *module_ctrl = NULL;
  struct cpp_library_params_t *cpp_lib_ctrl = NULL;
  mct_pipeline_cap_t query_buf;
  mct_event_t event;
  mct_stream_info_t stream_info;
  mct_port_t *s_port = NULL;
  uint32_t identity = 0;
  CDBG("%s:%d\n", __func__, __LINE__);

  if (argc > 1) {
    rc = parse_test_case_file(argv, &test_case);
  if (rc < 0)
    return rc;
  create_frame_info(&test_case, &frame_info);
  } else {
    printf("Usage: cpp-test-app <test case file>\n");
    return 0;
  }


  in_file_fd = open(test_case.input_filename, O_RDWR | O_CREAT, 0777);
  if (in_file_fd < 0) {
	  ALOGE("Cannot open file\n");
  }

  ALOGE("%s: Condition wait init\n", __func__);
  pthread_cond_init(&frame_done_cond, NULL);

  ionfd = open("/dev/ion", O_RDONLY | O_SYNC);
  if (ionfd < 0) {
	CDBG_ERROR("Ion device open failed\n");
  }

  in_frame.ion_alloc[0].len =
	  frame_info.plane_info[0].src_width * frame_info.plane_info[0].src_height * 1.5;
  in_frame.ion_alloc[0].len = in_frame.ion_alloc[0].len;
  in_frame.ion_alloc[0].heap_mask = 0x1 << ION_IOMMU_HEAP_ID;
  in_frame.ion_alloc[0].align = 4096;
  in_frame.addr[0] = (unsigned long) do_mmap_ion(ionfd,
    &(in_frame.ion_alloc[0]), &(in_frame.fd_data[0]), &in_frame_fd);

  ALOGE("In Frame FD: %d\n", in_frame_fd);
  read_len = read(in_file_fd, (void *)in_frame.addr[0], in_frame.ion_alloc[0].len);
  if( read_len != (int) in_frame.ion_alloc[0].len)
  {
	  ALOGE("Copy input image failed\n");
  }

  ALOGE("Read len: %d frame size: %d\n", read_len, (int) in_frame.ion_alloc[0].len);

  out_frame.ion_alloc[0].len =
    frame_info.plane_info[0].dst_stride *
    frame_info.plane_info[0].maximum_dst_stripe_height +
    frame_info.plane_info[1].dst_stride *
    frame_info.plane_info[1].maximum_dst_stripe_height;
  out_frame.ion_alloc[0].len = out_frame.ion_alloc[0].len;
  out_frame.ion_alloc[0].heap_mask = 0x1 << ION_IOMMU_HEAP_ID;
  out_frame.ion_alloc[0].align = 4096;
  out_frame.addr[0] = (unsigned long) do_mmap_ion(ionfd,
	&(out_frame.ion_alloc[0]), &(out_frame.fd_data[0]), &out_frame_fd);

  memset((void *) out_frame.addr[0], 128, out_frame.ion_alloc[0].len);
  ALOGE("Out Frame FD: %d\n", out_frame_fd);

  /* Call sensor mct module init */
  cpp_module = module_cpp_init("cpp");
  CDBG("%s:%d\n", __func__, __LINE__);

  CDBG("%s: module_cpp_init = %p", __func__, cpp_module);
  if (cpp_module== NULL) {
    CDBG_ERROR("%s: cpp_module = NULL\n",  __func__);
    exit(1);
  }
  rc = cpp_module->start_session(cpp_module, 1);
  if (rc == FALSE) {
    CDBG_ERROR("%s: TEST_ISP: ispif_module start error = %d\n",  __func__, rc);
    exit(1);
  }
  /* Call query capabilities */
  cpp_module->query_mod(cpp_module, &query_buf, 1);
  /*CDBG("%s:%d caps: mode %d position %d mount angle %d\n", __func__, __LINE__,
    query_buf.pp_cap.modes_supported, query_buf.sensor_cap.position, query_buf.sensor_cap.sensor_mount_angle);
  CDBG("%s:%d caps: focal length %f hor view angle %f ver view angle %f\n",
    __func__, __LINE__, query_buf.sensor_cap.focal_length,
    query_buf.sensor_cap.hor_view_angle, query_buf.sensor_cap.ver_view_angle);
  CDBG("%s:%d\n", __func__, __LINE__);
*/
  identity = pack_identity(1, 0);
  /* Call set mod */
  cpp_module->set_mod(cpp_module, MCT_MODULE_FLAG_SINK, identity);

  CDBG("%s:%d process_event %p\n", __func__, __LINE__, cpp_module->process_event);

  CDBG("%s:%d\n", __func__, __LINE__);
  s_port = MCT_PORT_CAST(MCT_MODULE_SINKPORTS(cpp_module)->data);
  CDBG("%s:%d s_port %p\n", __func__, __LINE__, s_port);
  s_port->ext_link(identity, s_port, NULL);
  CDBG("%s:test cpp %d\n", __func__, __LINE__);

  module_ctrl = (module_pproc_common_ctrl_t *) cpp_module->module_private;

  CDBG("%s:%d pproc_iface %p lib_params %p \n", __func__, __LINE__,
	   module_ctrl->pproc_iface, module_ctrl->pproc_iface->lib_params);

  cpp_lib_ctrl = (struct cpp_library_params_t *)
    module_ctrl->pproc_iface->lib_params->lib_private_data;
  CDBG("%s:%d pproc_iface %p lib_params %p clientid %d\n", __func__, __LINE__,
	   module_ctrl->pproc_iface, module_ctrl->pproc_iface->lib_params,
	   cpp_lib_ctrl->client_id);

  char out_fname[256];
  new_frame = (cpp_process_queue_t *)malloc(sizeof(cpp_process_queue_t));
  new_frame->client_id = cpp_lib_ctrl->client_id;
  new_frame->frame_info = frame_info;
  new_frame->frame_info.frame_id = 0;
  new_frame->frame_info.frame_type = MSM_CPP_REALTIME_FRAME;
  new_frame->frame_info.plane_info[0].src_fd = in_frame_fd;
  new_frame->frame_info.plane_info[0].dst_fd = out_frame_fd;
  new_frame->frame_info.plane_info[1].src_fd = in_frame_fd;
  new_frame->frame_info.plane_info[1].dst_fd = out_frame_fd;
  ALOGE("%s: Enqueue frame\n", __func__);

  event.identity = identity;
  event.direction = MCT_EVENT_DOWNSTREAM;
  event.type = MCT_EVENT_MODULE_EVENT;
  event.u.module_event.type = MCT_EVENT_MODULE_FRAME_IND;
  event.u.module_event.module_event_data = (void *)new_frame;
  s_port->event_func(s_port, &event);

  pthread_mutex_lock(&mutex);
  ALOGE("%s: Condition wait\n", __func__);
  pthread_cond_wait(&frame_done_cond, &mutex);
  ALOGE("%s: Condition wait done\n", __func__);
  pthread_mutex_unlock(&mutex);

  event.identity = identity;
  event.direction = MCT_EVENT_DOWNSTREAM;
  event.type = MCT_EVENT_MODULE_EVENT;
  event.u.module_event.type = MCT_EVENT_MODULE_FRAME_DONE;
  event.u.module_event.module_event_data = (void *)
    (new_frame->client_id << 16 | 0);
  s_port->event_func(s_port, &event);

  CDBG("%s:%d\n", __func__, __LINE__);
  event.identity = identity;
  event.direction = MCT_EVENT_DOWNSTREAM;
  event.type = MCT_EVENT_CONTROL_CMD;
  event.u.ctrl_event.type = MCT_EVENT_CONTROL_STREAMOFF;
  s_port->event_func(s_port, &event);
  CDBG("%s:%d\n", __func__, __LINE__);

  ALOGE("%s: frame %d finish\n", __func__, i);
  sprintf(out_fname, "%s_%d.yuv", test_case.output_filename, 0);
  out_file_fd = open(out_fname, O_RDWR | O_CREAT, 0777);
  if (out_file_fd < 0) {
	ALOGE("Cannot open file\n");
  }
  write(out_file_fd,
	(const void *)out_frame.addr[0], out_frame.ion_alloc[0].len);
  close(out_file_fd);
  free(new_frame);

  s_port->un_link(identity, s_port, NULL);
  CDBG("%s:%d\n", __func__, __LINE__);
  /* Free cpp mct module */
  module_cpp_deinit(cpp_module);

  CDBG("%s:%d\n", __func__, __LINE__);
  return 0;
}
