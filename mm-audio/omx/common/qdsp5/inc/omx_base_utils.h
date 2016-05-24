#ifndef _OMX_BASE_UTILS_H_
#define _OMX_BASE_UTILS_H_
/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_base_utils.h
This module contains the class definition for openMAX decoder component.

Copyright (c) 2006-2008 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*=====================================================================*/
/*=========================================================================
                            Edit History

$Header:
when       who     what, where, why
--------   ---     -------------------------------------------------------
=========================================================================*/
#include "OMX_Types.h"

#define PrintFrameHdr(i,bufHdr) \
    DEBUG_PRINT("i=%d OMX bufHdr[%p]buf[%p]size[%d]TS[%d]nFlags[0x%x]\n",\
    i,\
    (void*) bufHdr,\
    (void*)((OMX_BUFFERHEADERTYPE *)bufHdr)->pBuffer,\
    (unsigned int)((OMX_BUFFERHEADERTYPE *)bufHdr)->nFilledLen,\
    (unsigned int)((OMX_BUFFERHEADERTYPE *)bufHdr)->nTimeStamp, \
    (unsigned int)((OMX_BUFFERHEADERTYPE *)bufHdr)->nFlags)

// BitMask Management logic

#define BITMASK_SIZE(mIndex) \
    (((mIndex) + BITS_PER_BYTE - 1)/BITS_PER_BYTE)
#define BITMASK_OFFSET(mIndex)\
    ((mIndex)/BITS_PER_BYTE)
#define BITMASK_FLAG(mIndex) \
    (1 << ((mIndex) % BITS_PER_BYTE))
#define BITMASK_CLEAR(mArray,mIndex)\
    (mArray)[BITMASK_OFFSET(mIndex)] &=  ~(BITMASK_FLAG(mIndex))
#define BITMASK_SET(mArray,mIndex)\
    (mArray)[BITMASK_OFFSET(mIndex)] |=  BITMASK_FLAG(mIndex)
#define BITMASK_PRESENT(mArray,mIndex)\
    ((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex))
#define BITMASK_ABSENT(mArray,mIndex)\
    (((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex)) == 0x0)

#define OMX_CORE_NUM_INPUT_BUFFERS   2
#define OMX_CORE_NUM_OUTPUT_BUFFERS  2
#define OMX_CORE_CONTROL_CMDQ_SIZE   100
#define BITS_PER_BYTE                8
#define OMX_ADEC_DEFAULT_VOL         25
#define OMX_ADEC_VOLUME_STEP         0x147
#define OMX_ADEC_MIN                 0
#define OMX_ADEC_MAX                 10

///////////////////////////////////////////////
// Module level declarations
/////////////////////////////////////////////////

struct omx_event
{
    unsigned param1;
    unsigned param2;
    unsigned id;
};

struct omx_cmd_queue
{
    omx_event m_q[OMX_CORE_CONTROL_CMDQ_SIZE];
    unsigned m_read;
    unsigned m_write;
    unsigned m_size;

    omx_cmd_queue();
    ~omx_cmd_queue(){}
    bool insert_entry(unsigned p1, unsigned p2, unsigned char id);
    bool pop_entry(unsigned *p1,unsigned *p2, unsigned char *id);
    bool get_msg_with_id(unsigned *p1,unsigned *p2, unsigned char id);
    bool get_msg_id(unsigned char *id);
};

typedef struct psuedo_raw
{
    OMX_U16 sync_word;
    OMX_U16 fr_len;
}__attribute__((packed))PSEUDO_RAW;

typedef struct
{
    OMX_U16   offsetVal;
    OMX_TICKS nTimeStamp;
    OMX_U32   nFlags;
    OMX_U16   error;
    OMX_U16   samplingFreq;
    OMX_U16   chCfg;
    OMX_U32   tickCount;
}__attribute__((packed))META_OUT;

typedef struct metadata_output_suspend
{
   int       offsetVal;
   OMX_TICKS nTimeStamp;
   OMX_U32   nFlags;
}__attribute__((packed))META_OUT_SUS;
// Deferred callback identifiers
enum
{
    //Event Callbacks from the component thread context
    OMX_COMPONENT_GENERATE_EVENT       = 0x1,
    //Buffer Done callbacks from component thread context
    OMX_COMPONENT_GENERATE_BUFFER_DONE = 0x2,
    OMX_COMPONENT_GENERATE_ETB         = 0x3,
    //Command
    OMX_COMPONENT_GENERATE_COMMAND     = 0x4,
    OMX_COMPONENT_GENERATE_FRAME_DONE  = 0x05,
    OMX_COMPONENT_GENERATE_FTB         = 0x06,
    OMX_COMPONENT_GENERATE_EOS         = 0x07,
    OMX_COMPONENT_PORTSETTINGS_CHANGED = 0x08,
    OMX_COMPONENT_SUSPEND              = 0x09,
    OMX_COMPONENT_RESUME               = 0x0a,
    OMX_COMPONENT_STREAM_INFO          = 0x0b,
    OMX_COMPONENT_ERROR_INFO           = 0x0c
};
enum port_indexes
{
    OMX_CORE_INPUT_PORT_INDEX        =0,
    OMX_CORE_OUTPUT_PORT_INDEX       =1
};

enum //flags_bit_positions
{
    // Defer transition to IDLE
    OMX_COMPONENT_IDLE_PENDING            =0x1,
    // Defer transition to LOADING
    OMX_COMPONENT_LOADING_PENDING         =0x2,
    OMX_COMPONENT_MUTED                   =0x3,
    // Defer transition to Enable
    OMX_COMPONENT_INPUT_ENABLE_PENDING    =0x4,
    // Defer transition to Enable
    OMX_COMPONENT_OUTPUT_ENABLE_PENDING   =0x5,
    // Defer transition to Disable
    OMX_COMPONENT_INPUT_DISABLE_PENDING   =0x6,
    // Defer transition to Disable
    OMX_COMPONENT_OUTPUT_DISABLE_PENDING  =0x7
};
typedef struct metadata_input
{
    OMX_U16   offsetVal;
    OMX_TICKS nTimeStamp;
    OMX_U32   nFlags;
}__attribute__((packed)) META_IN;


void* omx_comp_timer_handler(void *);

class COmxBase;
class COmxBaseEventHandler;
class COmxTimer;

typedef struct timerinfo
{
    pthread_t    thr;
    COmxTimer    *timer;
    COmxBase     *base;
}TIMERINFO;

#endif // _OMX_BASE_UTILS_H_
