/**********************************************************************
* Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved. *
* Qualcomm Technologies Proprietary and Confidential.                 *
**********************************************************************/

#ifndef __IMG_DBG_H__
#define __IMG_DBG_H__
#include <stdio.h>

#ifndef IDBG_LOG_LEVEL
#define IDBG_LOG_LEVEL 2
#endif

#ifndef IDBG_LOG_TAG
#define IDBG_LOG_TAG "mm-camera-img"
#endif

#undef IDBG
#if (IDBG_LOG_LEVEL > 0)
    #ifdef _ANDROID_
        #undef LOG_NIDEBUG
        #undef LOG_TAG
        #define LOG_NIDEBUG 0
        #define LOG_TAG IDBG_LOG_TAG
        #include <utils/Log.h>
        #define IDBG(fmt, args...) ALOGE(fmt, ##args)
        #define INDBG(fmt, args...) do{}while(0)
    #else
        #define IDBG(fmt, args...) fprintf(stderr, fmt, ##args)
    #endif
#else
    #define IDBG(fmt, args...) do{}while(0)
    #define INDBG(fmt, args...) do{}while(0)
#endif

#if (IDBG_LOG_LEVEL >= 4)
  #define IDBG_ERROR(...)  IDBG(__VA_ARGS__)
  #define IDBG_HIGH(...)   IDBG(__VA_ARGS__)
  #define IDBG_MED(...)    IDBG(__VA_ARGS__)
  #define IDBG_LOW(...)    IDBG(__VA_ARGS__)
#elif (IDBG_LOG_LEVEL == 3)
  #define IDBG_ERROR(...)  IDBG(__VA_ARGS__)
  #define IDBG_HIGH(...)   IDBG(__VA_ARGS__)
  #define IDBG_MED(...)    IDBG(__VA_ARGS__)
  #define IDBG_LOW(...)    INDBG(__VA_ARGS__)
#elif (IDBG_LOG_LEVEL == 2)
  #define IDBG_ERROR(...)  IDBG(__VA_ARGS__)
  #define IDBG_HIGH(...)   IDBG(__VA_ARGS__)
  #define IDBG_MED(...)    INDBG(__VA_ARGS__)
  #define IDBG_LOW(...)    INDBG(__VA_ARGS__)
#elif (IDBG_LOG_LEVEL == 1)
  #define IDBG_ERROR(...)  IDBG(__VA_ARGS__)
  #define IDBG_HIGH(...)   INDBG(__VA_ARGS__)
  #define IDBG_MED(...)    INDBG(__VA_ARGS__)
  #define IDBG_LOW(...)    INDBG(__VA_ARGS__)
#else
  #define IDBG_ERROR(...)  INDBG(__VA_ARGS__)
  #define IDBG_HIGH(...)   INDBG(__VA_ARGS__)
  #define IDBG_MED(...)    INDBG(__VA_ARGS__)
  #define IDBG_LOW(...)    INDBG(__VA_ARGS__)
#endif

/** IDBG_HERE:
 *
 * Prints current function and line number
 **/
#define IDBG_HERE IDBG_ERROR("%s()[%d]", __func__, __LINE__);

/** IDBG_STOP:
 *
 * Intentional STOP, in order to check backtrace
 **/
#define IDBG_STOP {volatile int*idbg_p=0;\
  IDBG_ERROR("%s()[%d] ================ Intentional STOP ================\n",\
    __func__, __LINE__);*idbg_p=0;};

/** IDBG_DUMP_D:
 *    @val: value to be dumped
 *
 * Dumps integer value
 **/
#define IDBG_DUMP_D(val) {IDBG_ERROR("%s()[%d] %s value is %d \n",\
  __func__, __LINE__, #val, val);};

/** IDBG_DUMP_H:
 *    @val: value to be dumped
 *
 * Dumps HEX value
 **/
#define IDBG_DUMP_H(val) {IDBG_ERROR("%s()[%d] %s value is 0x%x \n",\
  __func__, __LINE__, #val, val);};

/** IDBG_DUMP_S:
 *    @val: value to be dumped
 *
 * Dumps string value
 **/
#define IDBG_DUMP_S(val) {IDBG_ERROR("%s()[%d] %s value is %s \n",\
  __func__, __LINE__, #val, val);};

#endif /* __IMG_DBG_H__ */

