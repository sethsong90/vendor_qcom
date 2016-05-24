/***************************************************************************
* Copyright (c) 2010-2013 Qualcomm Technologies, Inc.  All Rights Reserved.*
* Qualcomm Technologies Proprietary and Confidential.                      *
***************************************************************************/
#ifndef __EZTUNE_H__
#define __EZTUNE_H__
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <mm_qcamera_app.h>
typedef int boolean;
#include "chromatix.h"
#include "chromatix_common.h"
#include "af_tuning.h"
#include "eztune_process.h"
#include "eztune_diagnostics.h"
#include "eztune_preview.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define CAM_EZ_DEBUG 0
#if (CAM_EZ_DEBUG)
#ifdef _ANDROID_
#include <utils/Log.h>
#endif
  #undef LOG_NIDEBUG
  #undef LOG_TAG
  #define LOG_NIDEBUG 0
  #define LOG_TAG "mm_camera_EZTune"
  #define CDBG_EZ(fmt, args...) \
    ALOGE("%s:%d "fmt"\n", __func__, __LINE__, ##args)
#else
  #define CDBG_EZ(fmt, args...) do{}while(0)
#endif

#ifdef _ANDROID_
  #include <utils/Log.h>
  #define CDBG_EZ_HIGH(fmt, args...)  ALOGE(fmt, ##args)
  #define CDBG_EZ_ERROR(fmt, args...)  ALOGE(fmt, ##args)
#else
  #define CDBG_EZ_HIGH(fmt, args...) fprintf(stderr, fmt, ##args)
  #define CDBG_EZ_ERROR(fmt, args...) fprintf(stderr, fmt, ##args)
#endif

#define EZTUNE_MAX_RECV  2048
#define EZTUNE_MAX_SEND  2048

#define EZTUNE_FORMAT_MAX 20
#define EZTUNE_FORMAT_STR 50

#define POISITION1  ((void *) 0x00100100)
#define POISITION2  ((void *) 0x00200200)

typedef enum {
  EZTUNE_MISC_GET_VERSION,
  EZTUNE_MISC_APPLY_CHANGES,
  EZTUNE_MISC_WRITE_INI,
  EZTUNE_MISC_READ_INI,
  EZTUNE_MISC_LIST_INI,
} eztune_misc_message_t;

typedef struct {
  int item_num;
  int table_index;
  char * value_string;
} eztune_set_val_t;

struct ezlist {
  struct ezlist  *next, *prev;
};

typedef struct my_list {
  struct ezlist list;
  eztune_set_val_t data;
}my_list_t;

static inline void init_list (struct ezlist *ptr)
{
  ptr->next = ptr ;
  ptr->prev = ptr ;
}

static inline void add_node ( struct ezlist *tmp , struct ezlist *head)
{
  struct ezlist *prev =  head->prev;

  head->prev = tmp;
  tmp->next = head;
  tmp->prev = prev;
  prev->next = tmp;
}

static inline void del_node (struct ezlist *ptr)
{
  struct ezlist *prev = ptr->prev ;
  struct ezlist *next = ptr->next ;

  next->prev = ptr->prev;
  prev->next = ptr->next;
  ptr->next = POISITION1;
  ptr->prev = POISITION2;
}

typedef struct {
  int8_t(*eztune_status) (void);
} eztune_function_table_t;

typedef struct {
  uint32_t clientsocket_id;
  mm_camera_lib_handle *lib_handle;
  uint32_t status;
  eztune_function_table_t fn_table;
  tune_get_data_t get_data;
  chromatix_parms_type *chromatixptr;
  chromatix_parms_type *snapchromatixptr;
  chromatix_VFE_common_type *common_chromatixptr;
  af_tune_parms_t *af_tune_ptr;
  vfe_diagnostics_t *diagnostics_ptr;
  ez_3a_params_t diagnostics_3a;
 /* ez_pp_params_t diagnostics_pp;
  ez_sensor_params_t diagnostics_sens;
  ez_config_params_t diagnostics_conf;*/
  ez_af_tuning_params_t af_tuning;
} eztune_t;

#endif /* __EZTUNE_H__ */
