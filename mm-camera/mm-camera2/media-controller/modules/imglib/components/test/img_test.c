/**********************************************************************
 * Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved. *
 * Qualcomm Technologies Proprietary and Confidential.                 *
 **********************************************************************/

#include "img_test.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Function: img_test_init
 *
 * Description: initializes the image test object
 *
 * Input parameters:
 *   p_test - test object
 *
 * Return values:
 *   IMG_SUCCESS
 *   IMG_ERR_NO_MEMORY
 *   IMG_ERR_INVALID_INPUT
 *
 * Notes: none
 **/
int img_test_init(imglib_test_t *p_test)
{
  char filename[MAX_FILENAME_LEN];
  char path[MAX_FILENAME_LEN];
  int i = 0;
  char *ext = "%d.yuv";
  int rc = 0;

  IDBG_HIGH("%s:%d] cnt %d", __func__, __LINE__, p_test->in_count);
  for (i = 0; i < p_test->in_count; i++) {
    strlcpy(filename, p_test->input_fn, strlen(p_test->input_fn) + 1);
    strncat(filename, ext, strlen(ext) + 1);
    snprintf(path, sizeof(path), filename, i);
    IDBG_HIGH("%s:%d] filename %s", __func__, __LINE__, path);

    rc = img_test_fill_buffer(p_test, i, 0);
    if (rc < 0)
      return rc;

    rc = img_test_read(p_test, path, i);
    if (rc < 0)
      return rc;

  }

  return IMG_SUCCESS;
}

/**
 * Function: img_test_fill_buffer
 *
 * Description: fill the buffer structures
 *
 * Input parameters:
 *   p_test - test object
 *   index - index of the buffer
 *   analysis - set if its analysis frame
 *
 * Return values:
 *   IMG_SUCCESS
 *   IMG_ERR_NO_MEMORY
 *   IMG_ERR_INVALID_INPUT
 *
 * Notes: none
 **/
int img_test_fill_buffer(imglib_test_t *p_test, int index, int analysis)
{
  int lrc = 0;
  int i = 0;
  img_frame_t *pframe = NULL;
  int size = 0;
  float h_f, w_f;
  float chroma_f;
  int offset = 0;

  if (index >= MAX_TEST_FRAMES) {
    IDBG_ERROR("%s:%d] ", __func__, __LINE__);
    return IMG_ERR_INVALID_INPUT;
  }

  lrc = img_get_subsampling_factor(p_test->ss, &w_f, &h_f);
  if (lrc != IMG_SUCCESS) {
    IDBG_ERROR("%s:%d] ", __func__, __LINE__);
    return IMG_ERR_INVALID_INPUT;
  }
  chroma_f = h_f * w_f * 2;

  pframe = &p_test->frame[index];
  pframe->frame_cnt = 1;
  pframe->idx = index;
  pframe->info.analysis = analysis;
  pframe->info.width = p_test->width;
  pframe->info.height = p_test->height;
  IDBG_HIGH("%s:%d] dim %dx%d", __func__, __LINE__, p_test->width,
    p_test->height);
  size = IMG_LENGTH(pframe->info) * (1 + chroma_f);
  pframe->frame[0].plane_cnt = 2;

  p_test->addr[p_test->mem_cnt] = (uint8_t *)malloc(size);
  if (NULL == p_test->addr[p_test->mem_cnt]) {
    IDBG_ERROR("%s:%d] ", __func__, __LINE__);
    return IMG_ERR_NO_MEMORY;
  }

  for (i = 0; i < pframe->frame[0].plane_cnt; i++) {
    pframe->frame[0].plane[i].fd = -1;
    pframe->frame[0].plane[i].addr = p_test->addr[p_test->mem_cnt] + offset;
    pframe->frame[0].plane[i].offset = 0;
    if (i == 0) { /* Y plane */
      pframe->frame[0].plane[i].width = pframe->info.width;
      pframe->frame[0].plane[i].height = pframe->info.height;
      pframe->frame[0].plane[i].stride = pframe->info.width;
      pframe->frame[0].plane[i].length = IMG_LENGTH(pframe->frame[0].plane[i]);
    } else { /* Chroma plane */
      pframe->frame[0].plane[i].width = pframe->info.width * w_f;
      pframe->frame[0].plane[i].height = pframe->info.height * h_f;
      pframe->frame[0].plane[i].stride = pframe->info.width * w_f * 2;
      pframe->frame[0].plane[i].length = IMG_LENGTH(pframe->frame[0].plane[i])
        * 2;
    }
    offset = pframe->frame[0].plane[i].length;
  }
  p_test->mem_cnt++;
  IDBG_MED("%s:%d] ", __func__, __LINE__);
  return IMG_SUCCESS;
}

/**
 * Function: img_test_read
 *
 * Description: read the data from the file and fill the buffers
 *
 * Input parameters:
 *   none
 *
 * Return values:
 *   IMG_SUCCESS
 *   IMG_ERR_NO_MEMORY
 *   IMG_ERR_INVALID_INPUT
 *
 * Notes: none
 **/
int img_test_read(imglib_test_t *p_test, char *filename, int index)
{
  int lrc = 0;
  int i = 0;
  img_sub_frame_t *pframe = NULL;
  FILE *fp;

  if (index >= MAX_TEST_FRAMES) {
    IDBG_ERROR("%s:%d] ", __func__, __LINE__);
    return IMG_ERR_INVALID_INPUT;
  }

  pframe = &p_test->frame[index].frame[0];

  IDBG_MED("%s:%d] filename %s", __func__, __LINE__, filename);
  fp = fopen(filename, "rb+");
  if (fp) {
    for (i = 0; i < pframe->plane_cnt; i++) {
      lrc = fread(pframe->plane[i].addr, 1, pframe->plane[i].length, fp);
      IDBG_HIGH("%s:%d] bytes_read %d idx %d", __func__, __LINE__, lrc, i);
    }
    fclose(fp);
  } else {
    IDBG_ERROR("%s:%d] failed", __func__, __LINE__);
    return IMG_ERR_NO_MEMORY;
  }
  IDBG_MED("%s:%d] ", __func__, __LINE__);
  return IMG_SUCCESS;
}

/**
 * Function: img_test_write
 *
 * Description: write the data to the file
 *
 * Input parameters:
 *   none
 *
 * Return values:
 *   IMG_SUCCESS
 *   IMG_ERR_NO_MEMORY
 *   IMG_ERR_INVALID_INPUT
 *
 * Notes: none
 **/
int img_test_write(imglib_test_t *p_test, char *filename, int index)
{
  int lrc = 0;
  int i = 0;
  img_sub_frame_t *pframe = NULL;
  FILE *fp;

  if (index >= MAX_TEST_FRAMES) {
    IDBG_ERROR("%s:%d] ", __func__, __LINE__);
    return IMG_ERR_INVALID_INPUT;
  }

  pframe = &p_test->frame[index].frame[0];

  IDBG_MED("%s:%d] frame %p", __func__, __LINE__, pframe);

  fp = fopen(filename, "wb+");
  if (fp) {
    for (i = 0; i < pframe->plane_cnt; i++) {
      lrc = fwrite(pframe->plane[i].addr, 1, pframe->plane[i].length, fp);
      IDBG_HIGH("%s:%d] bytes_written %d idx %d", __func__, __LINE__, lrc, i);
    }
    fclose(fp);
  } else {
    IDBG_ERROR("%s:%d] failed", __func__, __LINE__);
    return IMG_ERR_NO_MEMORY;
  }
  IDBG_MED("%s:%d] ", __func__, __LINE__);
  return IMG_SUCCESS;
}

/**
 * Function: role_enum_to_str
 *
 * Description: translate role enum to string
 *
 * Input parameters:
 *   none
 *
 * Return values:
 *   role string
 *
 * Notes: none
 **/
char* role_enum_to_str(img_comp_role_t role)
{
  switch (role) {
  case IMG_COMP_DENOISE:
    return "DENOISE";
  case IMG_COMP_HDR:
    return "HDR";
  case IMG_COMP_FACE_PROC:
    return "FACE_PROC";
  case IMG_COMP_CAC:
    return "CAC";
  default:
    return NULL;
  }
}

/**
 * Function: wdmode_enum_to_str
 *
 * Description: translate role enum to string
 *
 * Input parameters:
 *   none
 *
 * Return values:
 *   role string
 *
 * Notes: none
 **/
char* wdmode_enum_to_str(wd_mode_t mode)
{
  switch (mode) {
  case WD_MODE_CBCR_ONLY:
    return "WD_MODE_CBCR_ONLY";
  case WD_MODE_STREAMLINE_YCBCR:
    return "WD_MODE_STREAMLINE_YCBCR";
  case WD_MODE_STREAMLINED_CBCR:
    return "WD_MODE_STREAMLINED_CBCR";
  case WD_MODE_YCBCR_PLANE:
    return "WD_MODE_YCBCR_PLANE";
  default:
    return NULL;
  }
}

/**
 * Function: print_usage
 *
 * Description: print the usage of the test application
 *
 * Input parameters:
 *   none
 *
 * Return values:
 *   none
 *
 * Notes: none
 **/
void print_usage()
{
  fprintf(stderr, "Usage: program_name [options] [-I <input file>]"
    " [-O <output file] [-W <width>] [-H <height>] [-R <role>]\n");
  fprintf(stderr, "Mandatory options:\n");
  fprintf(stderr, "  -I FILE\t\tPath to the input file.\n");
  fprintf(stderr, "  -O FILE\t\tPath for the output file.\n");
  fprintf(stderr, "  -W WIDTH\t\tInput image width.\n");
  fprintf(stderr, "  -H HEIGHT\t\tInput image height.\n");
  fprintf(stderr, "  -R ROLE\t\tImaging LIB role. 0.Denoise 1.HDR "
    "2.Face detect 3.CAC\n");
  fprintf(stderr, "  -N FILE\t\tNumber of input images.\n");
  fprintf(stderr, "  -M FILE\t\tWavelet denoise mode 0.YCbCr normal"
    "1.CbCr normal 2.YCbCr streamlined 3.CbCr streamlined\n");
  fprintf(stderr, "\n");
}

/**
 * Function: main
 *
 * Description: main encoder test app routine
 *
 * Input parameters:
 *   argc - argument count
 *   argv - argument strings
 *
 * Return values:
 *   none
 *
 * Notes: none
 **/
int main(int argc, char* argv[])
{
  int rc, c, i, val = 0;
  int lrc = IMG_SUCCESS;
  imglib_test_t img_test;
  hdr_test_t hdr_test;
  denoise_test_t denoise_test;
  faceproc_test_t faceproc_test;
  cac_test_t cac_test;
  img_comp_role_t role = IMG_COMP_DENOISE;

  /* Initialize the structures */
  memset(&img_test, 0x0, sizeof(img_test));
  memset(&hdr_test, 0x0, sizeof(hdr_test));
  memset(&denoise_test, 0x0, sizeof(denoise_test));
  memset(&faceproc_test, 0x0, sizeof(faceproc_test));
  memset(&cac_test, 0x0, sizeof(cac_test));
  pthread_mutex_init(&img_test.mutex, NULL);
  pthread_cond_init(&img_test.cond, NULL);

  fprintf(stderr, "=======================================================\n");
  fprintf(stderr, " Qualcomm ImagingLIB test\n");
  fprintf(stderr, "=======================================================\n");
  opterr = 1;

  while ((c = getopt(argc, argv, "I:O:W:H:N:R:M:PZ")) != -1) {
    switch (c) {
    case 'O':
      img_test.out_fn = optarg;
      fprintf(stderr, "%-25s%s\n", "Output image path", img_test.out_fn);
      break;
    case 'I':
      img_test.input_fn = optarg;
      fprintf(stderr, "%-25s%s\n", "Input image path", img_test.input_fn);
      break;
    case 'W':
      img_test.width = atoi(optarg);
      fprintf(stderr, "%-25s%d\n", "Input image width", img_test.width);
      break;
    case 'H':
      img_test.height = atoi(optarg);
      fprintf(stderr, "%-25s%d\n", "Input image height", img_test.height);
      break;
    case 'N':
      img_test.in_count = atoi(optarg);
      fprintf(stderr, "%-25s%d\n", "Input image count", img_test.in_count);
      break;
    case 'M':
      denoise_test.mode = atoi(optarg);
      if (wdmode_enum_to_str(denoise_test.mode) == NULL) {
        print_usage();
        val = -1;
        goto exit;
      }
      fprintf(stderr, "%-25s%s\n", "Wavelet mode role",
        wdmode_enum_to_str(denoise_test.mode));
      break;
    case 'R':
      role = atoi(optarg);
      if (role_enum_to_str(role) == NULL) {
        print_usage();
        val = -1;
        goto exit;
      }
      fprintf(stderr, "%-25s%s\n", "Imaging LIB role", role_enum_to_str(role));
      break;
    default:
      print_usage();
      val = 0;
      goto exit;
    }
  }

  if ((img_test.width == 0) || (img_test.height == 0)
    || (img_test.out_fn == NULL) || (img_test.input_fn == NULL)
    || (img_test.in_count <= 0)) {
    print_usage();
    val = 0;
    goto exit;
  }
  rc = img_test_init(&img_test);
  if (rc != IMG_SUCCESS) {
    IDBG_ERROR("%s:%d] rc %d", __func__, __LINE__, rc);
    val = rc;
    goto exit;
  }

  switch (role) {
  case IMG_COMP_HDR:
    hdr_test.base = &img_test;
    rc = hdr_test_execute(&hdr_test);
    break;
  case IMG_COMP_DENOISE:
    denoise_test.base = &img_test;
    rc = denoise_test_execute(&denoise_test);
    break;
  case IMG_COMP_FACE_PROC:
    faceproc_test.base = &img_test;
    rc = faceproc_test_execute(&faceproc_test);
    break;
  case IMG_COMP_CAC:
    cac_test.base = &img_test;
    rc = cac_test_execute(&cac_test);
    break;
  default:
    fprintf(stderr, "Error invalid mode\n");
    goto error;
  }

  if (rc != IMG_SUCCESS) {
    fprintf(stderr, "Error rc %d", rc);
    goto error;
  }

  goto exit;

  error: for (i = 0; i < img_test.mem_cnt; i++) {
    if (img_test.addr[i] != NULL) {
      free(img_test.addr[i]);
      img_test.addr[i] = NULL;
    }
  }

exit:
  pthread_mutex_destroy(&img_test.mutex);
  pthread_cond_destroy(&img_test.cond);

  return val;
}
