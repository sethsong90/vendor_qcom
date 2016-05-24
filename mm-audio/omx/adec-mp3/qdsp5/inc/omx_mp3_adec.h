#ifndef _OMX_MP3_DEC_H_
#define _OMX_MP3_DEC_H_
/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder

*//** @file omx_mp3_adec.h
  This module contains the class definition for openMAX decoder component.

Copyright (c) 2006-2013 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

/*============================================================================
                              Edit History

$Header: ///linux/pkgs/proprietary/mm-audio/main/source/7k/adec-omxmp3/omx_mp3_adec.h $
when       who     what, where, why
--------   ---     -------------------------------------------------------

============================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

#include<stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <semaphore.h>
#include <linux/msm_audio.h>
#include <linux/msm_ion.h>
#include "QOMX_AudioExtensions.h"
#include "QOMX_AudioIndexExtensions.h"
#include "OMX_Core.h"
#include "OMX_Audio.h"
#include "adec_svr.h"
#include "qc_omx_component.h"
#include "Map.h"

extern "C" {
    void * get_omx_component_factory_fn(void);
}

//////////////////////////////////////////////////////////////////////////////
//                       Module specific globals
//////////////////////////////////////////////////////////////////////////////
#define OMX_SPEC_VERSION  0x00000101
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (x >= y?x:y)

//////////////////////////////////////////////////////////////////////////////
//               Macros
////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_PRINT
#undef DEBUG_PRINT
#define DEBUG_PRINT
#endif

#define PrintFrameHdr(i,bufHdr) \
                   DEBUG_PRINT("i=%d Frame Header bufHdr[%x]buf[%x]size[%d]TS[%d] nFlags[0x%x]\n",i,\
                           (unsigned) bufHdr,                                     \
                           (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->pBuffer,   \
                           (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nFilledLen,\
                           (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nTimeStamp,\
                           (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nFlags)

// BitMask Management logic

#define OMX_CORE_NUM_INPUT_BUFFERS      2
#define OMX_CORE_NUM_OUTPUT_BUFFERS     2

#define OMX_CORE_INPUT_BUFFER_SIZE    8192
#define OMX_MP3_OUTPUT_BUFFER_SIZE    32768
#define OMX_CORE_CONTROL_CMDQ_SIZE      100
#define BITS_PER_BYTE                   8
#define OMX_ADEC_MIN                    0
#define OMX_ADEC_MAX                    100
#define NON_TUNNEL                      1
#define TUNNEL                          0
#define MAX_SUSPEND_OUT_BUFFERS         10
#define DEFAULT_SAMPLING_RATE  44100
#define DEFAULT_CHANNEL_MODE   2
#define MP3_DECODER_DELAY   1058  // MP3 decoder delay of 529 samples (529 * 2 bytes)

#define BITMASK_SIZE(mIndex)            (((mIndex) + BITS_PER_BYTE - 1)/BITS_PER_BYTE)
#define BITMASK_OFFSET(mIndex)          ((mIndex)/BITS_PER_BYTE)
#define BITMASK_FLAG(mIndex)            (1 << ((mIndex) % BITS_PER_BYTE))
#define BITMASK_CLEAR(mArray,mIndex)    (mArray)[BITMASK_OFFSET(mIndex)] &=  ~(BITMASK_FLAG(mIndex))
#define BITMASK_SET(mArray,mIndex)      (mArray)[BITMASK_OFFSET(mIndex)] |=  BITMASK_FLAG(mIndex)
#define BITMASK_PRESENT(mArray,mIndex)  ((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex)  )
#define BITMASK_ABSENT(mArray,mIndex)   (((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex)  ) == 0x0)

class omx_mp3_adec;
class timer;

typedef struct timerinfo
{
    pthread_t    thr;
    timer        *pTimer;
    omx_mp3_adec *base;
}TIMERINFO;

class timer
{
private:
    bool                m_timerExpiryFlg;
    OMX_U8              m_timeout;
    OMX_BOOL            m_deleteTimer;
    sem_t               m_sem_state;
    volatile int        m_timer_cnt;
    pthread_cond_t      m_timer_cond;
    pthread_mutexattr_t m_timer_mutex_attr;
    pthread_mutex_t     m_timer_mutex;

    pthread_cond_t      m_tcond;
    pthread_mutexattr_t m_tmutex_attr;
    pthread_mutex_t     m_tmutex;
    omx_mp3_adec        *m_base;
    TIMERINFO           *m_timerinfo;
public:
    timer(omx_mp3_adec* base);
    ~timer();

    void        startTimer();
    void        stopTimer();
    int         timer_run();
    void        wait_for_timer_event();

    inline void releaseTimer()
    {
        pthread_mutex_lock(&m_tmutex);
        m_deleteTimer = OMX_TRUE;
        pthread_mutex_unlock(&m_tmutex);

        sem_post(&m_sem_state);
    }
    inline OMX_BOOL getReleaseTimerStat()
    {
        OMX_BOOL flg;
        pthread_mutex_lock(&m_tmutex);
        flg = m_deleteTimer;
        pthread_mutex_unlock(&m_tmutex);
        return flg;
    }
    inline bool getTimerExpiry()
    {
        return m_timerExpiryFlg;
    }
    inline void setTimerExpiry()
    {
        m_timerExpiryFlg = true;
    }
    inline void resetTimerExpiry()
    {
        m_timerExpiryFlg = false;
    }
    inline OMX_U8 getTimeOut()
    {
        return m_timeout;
    }
    inline omx_mp3_adec* get_comp()
    {
        return m_base;
    }
};

// OMX mp3 audio decoder class
class omx_mp3_adec: public qc_omx_component
{
public:
    omx_mp3_adec();  // constructor
    virtual ~omx_mp3_adec();  // destructor

    OMX_ERRORTYPE component_init(OMX_STRING role);

    OMX_ERRORTYPE component_deinit(OMX_HANDLETYPE hComp);

    static void process_in_port_msg(void          *client_data,
                                 unsigned char id);

    static void process_out_port_msg(void          *client_data,
                                  unsigned char id);

    static void process_command_msg(void          *client_data,
                                   unsigned char id);

    static void process_event_cb(void          *client_data,
                                   unsigned char id);

    OMX_ERRORTYPE free_buffer(OMX_HANDLETYPE       hComp,
                              OMX_U32              port,
                              OMX_BUFFERHEADERTYPE *buffer);

    OMX_ERRORTYPE empty_this_buffer(OMX_HANDLETYPE       hComp,
                                    OMX_BUFFERHEADERTYPE *buffer);

    OMX_ERRORTYPE fill_this_buffer(OMX_HANDLETYPE       hComp,
                                   OMX_BUFFERHEADERTYPE *buffer);

    OMX_ERRORTYPE allocate_buffer(OMX_HANDLETYPE       hComp,
                                  OMX_BUFFERHEADERTYPE **bufferHdr,
                                  OMX_U32              port,
                                  OMX_PTR              appData,
                                  OMX_U32              bytes);

    OMX_ERRORTYPE use_buffer(OMX_HANDLETYPE       hComp,
                             OMX_BUFFERHEADERTYPE **bufferHdr,
                             OMX_U32              port,
                             OMX_PTR              appData,
                             OMX_U32              bytes,
                             OMX_U8               *buffer);

    OMX_ERRORTYPE component_role_enum(OMX_HANDLETYPE hComp,
                                      OMX_U8         *role,
                                      OMX_U32        index);

    OMX_ERRORTYPE get_config(OMX_HANDLETYPE hComp,
                             OMX_INDEXTYPE  configIndex,
                             OMX_PTR        configData);


    OMX_ERRORTYPE get_parameter(OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE  paramIndex,
                                OMX_PTR        paramData);

    OMX_ERRORTYPE get_state(OMX_HANDLETYPE hComp,
                            OMX_STATETYPE  *state);


    OMX_ERRORTYPE get_extension_index(OMX_HANDLETYPE hComp,
                                      OMX_STRING     paramName,
                                      OMX_INDEXTYPE  *indexType);

    OMX_ERRORTYPE get_component_version(OMX_HANDLETYPE  hComp,
                                        OMX_STRING      componentName,
                                        OMX_VERSIONTYPE *componentVersion,
                                        OMX_VERSIONTYPE *specVersion,
                                        OMX_UUIDTYPE    *componentUUID);

    OMX_ERRORTYPE set_callbacks(OMX_HANDLETYPE   hComp,
                                OMX_CALLBACKTYPE *callbacks,
                                OMX_PTR          appData);

    OMX_ERRORTYPE set_config(OMX_HANDLETYPE hComp,
                             OMX_INDEXTYPE  configIndex,
                             OMX_PTR        configData);

    OMX_ERRORTYPE set_parameter(OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE  paramIndex,
                                OMX_PTR        paramData);

    OMX_ERRORTYPE send_command(OMX_HANDLETYPE  hComp,
                               OMX_COMMANDTYPE cmd,
                               OMX_U32         param1,
                               OMX_PTR         cmdData);

    OMX_ERRORTYPE use_EGL_image(OMX_HANDLETYPE       hComp,
                                OMX_BUFFERHEADERTYPE **bufferHdr,
                                OMX_U32              port,
                                OMX_PTR              appData,
                                void *               eglImage);

    OMX_ERRORTYPE component_tunnel_request(OMX_HANDLETYPE      hComp,
                                           OMX_U32             port,
                                           OMX_HANDLETYPE      peerComponent,
                                           OMX_U32             peerPort,
                                           OMX_TUNNELSETUPTYPE *tunnelSetup);

    bool post_command(unsigned int p1, unsigned int p2,
        unsigned int id);

    inline timer* getTimerInst()
    {
        if(m_timer)
            return m_timer;
        else
            return NULL;
    }
    inline bool getSuspendFlg()
    {
        return bSuspendEventRxed;
    }
    inline void setSuspendFlg()
    {
        bSuspendEventRxed = true;
    }
    inline void resetSuspendFlg()
    {
        bSuspendEventRxed = false;
    }
    inline bool getResumeFlg()
    {
        return bResumeEventRxed;
    }
    inline void setResumeFlg()
    {
        bResumeEventRxed = true;
    }
    inline void resetResumeFlg()
    {
        bResumeEventRxed = false;
    }

    // Deferred callback identifiers
    enum
    {
        OMX_COMPONENT_GENERATE_EVENT       = 0x1,
        OMX_COMPONENT_GENERATE_BUFFER_DONE = 0x2,
        OMX_COMPONENT_GENERATE_ETB         = 0x3,
        OMX_COMPONENT_GENERATE_COMMAND     = 0x4,
        OMX_COMPONENT_GENERATE_FRAME_DONE  = 0x5,
        OMX_COMPONENT_GENERATE_FTB         = 0x6,
        OMX_COMPONENT_GENERATE_EOS         = 0x7,
        OMX_COMPONENT_PORTSETTINGS_CHANGED = 0x8,
        OMX_COMPONENT_SUSPEND              = 0x09,
        OMX_COMPONENT_RESUME               = 0x0a
    };
    inline bool getWaitForSuspendCmplFlg()
    {
        bool t;
        pthread_mutex_lock(&m_WaitForSuspendCmpl_lock);
        t = waitForSuspendCmplFlg;
        pthread_mutex_unlock(&m_WaitForSuspendCmpl_lock);
        return t;
    }
    inline void setWaitForSuspendCmplFlg()
    {
        pthread_mutex_lock(&m_WaitForSuspendCmpl_lock);
        waitForSuspendCmplFlg = true;
        pthread_mutex_unlock(&m_WaitForSuspendCmpl_lock);
    }
    inline void resetWaitForSuspendCmplFlg()
    {
        pthread_mutex_lock(&m_WaitForSuspendCmpl_lock);
        waitForSuspendCmplFlg = false;
        pthread_mutex_unlock(&m_WaitForSuspendCmpl_lock);
    }
    void wait_for_suspend_cmpl()
    {
        setWaitForSuspendCmplFlg();
        sem_wait(&sem_WaitForSuspendCmpl_states);
    }
    void release_wait_for_suspend()
    {
        resetWaitForSuspendCmplFlg();
        sem_post(&sem_WaitForSuspendCmpl_states);
    }

private:

    enum port_indexes
    {
        OMX_CORE_INPUT_PORT_INDEX   = 0,
        OMX_CORE_OUTPUT_PORT_INDEX  = 1

    };
    // Bit Positions
    enum flags_bit_positions
    {
        OMX_COMPONENT_IDLE_PENDING            =0x1,
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
        ~omx_cmd_queue();
        bool insert_entry(unsigned p1, unsigned p2, unsigned id);
        bool pop_entry(unsigned *p1,unsigned *p2, unsigned *id);
        bool get_msg_id(unsigned *id);
    };
    struct mp3_header
    {
        OMX_U8 sync;
        OMX_U8 version;
        uint8_t Layer;
        OMX_U8 protection;
        OMX_U32  bitrate;
        OMX_U32 sampling_rate;
        OMX_U8 padding;
        OMX_U8 private_bit;
        OMX_U8 channel_mode;
    };

    struct mmap_info
    {
        int ion_fd;
        ion_user_handle_t handle;
        void* pBuffer;
        unsigned map_buf_size;
        unsigned filled_len;
    };

    typedef struct metadata_input
    {
        OMX_U16   offsetVal;
        OMX_TICKS nTimeStamp;
        OMX_U32   nFlags;
    }__attribute__((packed)) META_IN;

    typedef struct metadata_output
    {
        OMX_U16   offsetVal;
        OMX_TICKS nTimeStamp;
        OMX_U32   nFlags;
        OMX_U16   error;
        OMX_U16   samplingFreq;
        OMX_U16   chCfg;
        OMX_U32   tickCount;
    }__attribute__((packed))META_OUT;

    typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*>
                                                   input_buffer_map;

    typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*>
                                                   output_buffer_map;

    OMX_STATETYPE                m_state;    // OMX State
    OMX_PTR                      m_app_data;    // Application data
    timer                        *m_timer;

    OMX_TICKS                    nLastTimeStamp;
    OMX_U8                       m_flush_cnt;
    OMX_U8                       m_first_mp3_header;
    struct mmap_info             **m_suspend_out_buf_list;
    struct msm_audio_aio_buf     *m_suspend_out_drv_buf;
    unsigned int                 m_suspend_drv_buf_cnt;
    unsigned int                 m_suspend_out_buf_cnt;
    unsigned int                 m_resume_out_buf_cnt;
    int                          drv_inp_buf_cnt;
    int                          drv_out_buf_cnt;

    OMX_U8                       m_is_alloc_buf;
    int                          nNumInputBuf;
    int                          nNumOutputBuf;
    int                          m_drv_fd;   // Kernel device node file handle
    bool                         bOutputPortReEnabled;
    bool                         bFlushcompleted;
    bool                         bFlushinprogress;
    bool                         bSuspendEventRxed;
    bool                         bResumeEventRxed;
    bool                         fake_in_eos_ack_received;
    bool                         fake_in_eos_sent;
    bool                         fake_eos_recieved;
    bool                         is_in_th_sleep;
    bool                         is_out_th_sleep;
    bool                         m_flush_inbuf;
    bool                         m_flush_outbuf;
    bool                         m_to_idle;
    bool                         m_pause_to_exe;
    bool                         waitForSuspendCmplFlg;
    unsigned                     m_msg_cnt; // message count
    bool                         bSuspendinprogress;
    unsigned                     m_cmd_cnt; // command count
    unsigned                     m_etb_cnt; // Empty This Buffer count
    unsigned                     m_ebd_cnt; // Empty Buffer Done Count

    unsigned int                 m_inp_act_buf_count;// Num of Input Buffers
    unsigned int                 m_out_act_buf_count;// Numb of Output Buffers
    unsigned short               m_session_id;
    OMX_PRIORITYMGMTTYPE         m_priority_mgm ;
    OMX_PARAM_BUFFERSUPPLIERTYPE m_buffer_supplier;
    // store I/P PORT state
    OMX_BOOL                     m_inp_bEnabled;
    // store O/P PORT state
    OMX_BOOL                     m_out_bEnabled;
    //Input port Populated
    OMX_BOOL                     m_inp_bPopulated;
    //Output port Populated
    OMX_BOOL                     m_out_bPopulated;
    unsigned int                 m_inp_current_buf_count;// Num of Input Buffers
    unsigned int                 m_out_current_buf_count;// Numb of Output Buffers
    unsigned int                 m_comp_deinit;
    unsigned int                 m_flags;//encapsulate the waiting states.
    unsigned int                 m_fbd_cnt;
    unsigned int                 nTimestamp;
    // Tunnel or Non-tunnel mode
    unsigned int                 pcm_feedback;
    unsigned int                 ntotal_playtime;
    unsigned int                 output_buffer_size;
    unsigned int                 input_buffer_size;
    unsigned int                 pSamplerate;
    unsigned int                 pChannels;
    unsigned int                 pBitrate ;
    unsigned int                 op_settings_changed ;

    /* Odd byte in Input buffer */
    unsigned char                m_odd_byte;
    bool                         m_odd_byte_set;

    //holds the duration of each frame
    unsigned int             frameDuration;

    volatile int                 m_is_in_th_sleep;
    volatile int                 m_is_out_th_sleep;
    volatile int                 m_flush_cmpl_event;

    omx_cmd_queue                m_input_q;
    omx_cmd_queue                m_input_ctrl_q;
    omx_cmd_queue                m_command_q;
    omx_cmd_queue                m_output_q;
    omx_cmd_queue                m_output_ctrl_q;
    omx_cmd_queue                m_input_ctrl_cmd_q;
    omx_cmd_queue                m_input_ctrl_ebd_q;
    omx_cmd_queue                m_output_ctrl_cmd_q;
    omx_cmd_queue                m_output_ctrl_fbd_q;

    pthread_cond_t               in_cond;
    pthread_cond_t               out_cond;
    pthread_mutexattr_t          m_WaitForSuspendCmpl_lock_attr;
    pthread_mutex_t              m_WaitForSuspendCmpl_lock;
    pthread_mutex_t              m_suspendresume_lock;
    pthread_mutex_t              m_inputlock;
    pthread_mutex_t              m_commandlock;
    pthread_mutex_t              m_outputlock;
    pthread_mutex_t              m_seq_lock;
    pthread_mutex_t              m_flush_lock;
    pthread_mutex_t              m_in_th_lock;
    pthread_mutex_t              m_state_lock;
    pthread_mutex_t              m_in_th_lock_1;
    pthread_mutex_t              m_out_th_lock;
    pthread_mutex_t              m_out_th_lock_1;
    pthread_mutex_t              out_buf_count_lock;
    pthread_mutex_t              in_buf_count_lock;
    pthread_mutex_t              m_flush_cmpl_lock;
    pthread_mutexattr_t          m_suspendresume_lock_attr;
    pthread_mutexattr_t          m_state_lock_attr;
    pthread_mutexattr_t          m_seq_attr;
    pthread_mutexattr_t          m_flush_attr;
    pthread_mutexattr_t          m_outputlock_attr;
    pthread_mutexattr_t          m_commandlock_attr;
    pthread_mutexattr_t          m_inputlock_attr;
    pthread_mutexattr_t          m_in_th_attr_1;
    pthread_mutexattr_t          m_in_th_attr;
    pthread_mutexattr_t          m_out_th_attr_1;
    pthread_mutexattr_t          m_out_th_attr;
    pthread_mutexattr_t          out_buf_count_lock_attr;
    pthread_mutexattr_t          in_buf_count_lock_attr;
    pthread_mutexattr_t          m_flush_cmpl_attr;

    unsigned int Omx_fillbufcnt;
    int waiting_for_resume;

    sem_t sem_read_msg;
    sem_t sem_write_msg;
    sem_t sem_States;
    sem_t sem_th_state;
    sem_t sem_flush_cmpl_state;
    sem_t sem_WaitForSuspendCmpl_states;
    int set_pcm_config;

    OMX_S32                      m_volume;   //Unit to be determined

    OMX_STATETYPE                nState;

    input_buffer_map             m_input_buf_hdrs;
    output_buffer_map            m_output_buf_hdrs;

    input_buffer_map             m_loc_in_use_buf_hdrs;
    output_buffer_map            m_loc_out_use_buf_hdrs;

    bool                         m_in_use_buf_case;
    bool                         m_out_use_buf_case;
    bool                         m_input_eos_rxd;
    bool                         m_output_eos_rxd;
    bool                         bGenerateEOSPending;

    OMX_CALLBACKTYPE             m_cb;       // Application callbacks

    // for posting and recieving mesgs thru pipes
    struct mp3_ipc_info          *m_ipc_to_in_th;
    struct mp3_ipc_info          *m_ipc_to_out_th;
    struct mp3_ipc_info          *m_ipc_to_cmd_th;
    struct mp3_ipc_info          *m_ipc_to_event_th;

    OMX_AUDIO_PARAM_MP3TYPE      m_adec_param; // Cache mp3 decoder parameter
    OMX_AUDIO_PARAM_PCMMODETYPE  m_pcm_param; // Cache pcm  parameter
    OMX_SUSPENSIONPOLICYTYPE     suspensionPolicy;
    OMX_PARAM_COMPONENTROLETYPE  component_Role;
    OMX_STRING                   mime_type;
    int32_t ion_fd;
    ////////////////////////////////////////////////////////////////////
    // Private methods
    ////////////////////////////////////////////////////////////////////
    void buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr);

    void frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr);

    OMX_ERRORTYPE allocate_input_buffer(OMX_HANDLETYPE       hComp,
                                        OMX_BUFFERHEADERTYPE **bufferHdr,
                                        OMX_U32              port,
                                        OMX_PTR              appData,
                                        OMX_U32              bytes);
    OMX_ERRORTYPE  allocate_output_buffer(
                       OMX_IN OMX_HANDLETYPE            hComp,
                       OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                               OMX_IN OMX_U32                   port,
                               OMX_IN OMX_PTR                   appData,
                               OMX_IN OMX_U32                   bytes);

    OMX_ERRORTYPE  use_input_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes,
                         OMX_IN OMX_U8*                   buffer);

    OMX_ERRORTYPE  use_output_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes,
                         OMX_IN OMX_U8*                   buffer);

    OMX_ERRORTYPE  mp3_frameheader_parser(OMX_BUFFERHEADERTYPE* buffer,
                                  struct mp3_header *header);

    OMX_ERRORTYPE empty_this_buffer_proxy(OMX_HANDLETYPE       hComp,
                                          OMX_BUFFERHEADERTYPE *buffer);


    OMX_ERRORTYPE fill_this_buffer_proxy(OMX_HANDLETYPE       hComp,
                                         OMX_BUFFERHEADERTYPE *buffer);

    OMX_ERRORTYPE send_command_proxy(OMX_HANDLETYPE hComp,
                            OMX_COMMANDTYPE  cmd,
                            OMX_U32       param1,
                            OMX_PTR      cmdData);

    bool post_input(unsigned int p1, unsigned int p2,
                  unsigned int id);

    bool post_output(unsigned int p1, unsigned int p2,
                  unsigned int id);

    void process_events(omx_mp3_adec *client_data);

    bool allocate_done(void);

    bool release_done(OMX_U32 param1);

    bool execute_omx_flush(OMX_IN OMX_U32 param1, bool cmd_cmpl=true);

    bool execute_input_omx_flush(void);

    bool execute_output_omx_flush(void);

    bool search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer);

    bool search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer);

    bool omx_mp3_fake_eos(void);

    bool alloc_fill_suspend_out_buf(void);
    void wait_for_event();

    void event_complete();

    void wait_for_flush_event();

    void event_flush_complete();

    void* alloc_ion_buffer(unsigned int bufsize);

    void free_ion_buffer(void** pmem_buffer);

    void in_th_goto_sleep();

    void in_th_wakeup();

    void out_th_goto_sleep();

    void out_th_wakeup();
    void flush_ack();
    void deinit_decoder();
};
#endif // _OMX_MP3_DEC_H_
