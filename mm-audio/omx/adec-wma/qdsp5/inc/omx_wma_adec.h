#ifndef  _OMX_WMA_DEC_H_
#define  _OMX_WMA_DEC_H_
/*============================================================================
                            O p e n M A X   Component
                                Video Decoder

*//** @file comx_wma_adec.h
  This module contains the class definition for openMAX decoder component.

Copyright (c) 2006-2008 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

/*============================================================================
                              Edit History

$Header: //depot/asic/sandbox/users/ronaldk/videodec/OMXVdecCommon/omx_vdec.h#1 $
when       who     what, where, why
--------   ---     -------------------------------------------------------
08/13/2008  pl     Initial version

============================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

/* Uncomment out below line #define LOG_NDEBUG 0 if we want to see all DEBUG_PRINT or LOGV messaging */
#include<stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include "OMX_Core.h"
#include "OMX_Audio.h"
#include "adec_svr.h"
#include "qc_omx_component.h"
#include "Map.h"
#include <semaphore.h>
#include "QOMX_AudioExtensions.h"
#include "QOMX_AudioIndexExtensions.h"

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
//////////////////////////////////////////////////////////////////////////////
//
#define PrintFrameHdr(bufHdr) \
    DEBUG_PRINT("bufHdr[%x]buf[%x]size[%d]TS[%d]\n",\
    (unsigned) bufHdr,\
    (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->pBuffer,\
    (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nFilledLen,\
    (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nTimeStamp)

// BitMask Management logic
#define BITS_PER_BYTE 8
#define BITMASK_SIZE(mIndex)            (((mIndex) + BITS_PER_BYTE - 1)/BITS_PER_BYTE)
#define BITMASK_OFFSET(mIndex)          ((mIndex)/BITS_PER_BYTE)
#define BITMASK_FLAG(mIndex)            (1 << ((mIndex) % BITS_PER_BYTE))
#define BITMASK_CLEAR(mArray,mIndex)    (mArray)[BITMASK_OFFSET(mIndex)] &=  ~(BITMASK_FLAG(mIndex))
#define BITMASK_SET(mArray,mIndex)      (mArray)[BITMASK_OFFSET(mIndex)] |=  BITMASK_FLAG(mIndex)
#define BITMASK_PRESENT(mArray,mIndex)  ((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex)  )
#define BITMASK_ABSENT(mArray,mIndex)   (((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex)  ) == 0x0)

#define OMX_CORE_NUM_INPUT_BUFFERS      2
#define OMX_CORE_INPUT_BUFFER_SIZE      16000
#define OMX_CORE_NUM_OUTPUT_BUFFERS     2
#define OMX_CORE_OUTPUT_BUFFER_SIZE     8192
#define OMX_CORE_CONTROL_CMDQ_SIZE      100 /*5000;100;*/
#define OMX_ADEC_VOLUME_STEP            0x147 /* 0x7FFF / 0x64 */
#define OMX_ADEC_MIN                    0
#define OMX_ADEC_MAX                    100
#define NON_TUNNEL                      1
#define TUNNEL                          0

/*
   TCXO buffer size calculated based on below formula:
  -worst case wma packet size of 40 bytes(8k mono)
  -During suspend max 410 packets can be with driver(two bitstream buffers
   each of 8k)
  -Number of samples per packet = 1024 bytes
  -Total TCXO buffer Size = 410 * 1024 = 410k
*/
#define OMX_TCXO_BUFFER                 (410 * 1024)
#define TRUE 1
#define FALSE 0
#define WMA_DATA_THR_MAX                2380
#define WMA_DATA_THR_OFFSET             3
#define WMA_DATA_REQUEST_THRESHOLD      0x20
#define WMA_TOTAL_DUMMY_BYTES_TO_SEND   8192
#define WMA_DUMMY_BYTE 0xff
#define DRIVER_PACKET_SIZE              6
#define PADDING_SIZE                    1
#define PACKET_NUM                      10
#define DEFAULT_CHANNEL_MODE            2
#define MONO 1
#define STEREO 2
#define DEFAULT_SAMPLING_RATE           44100
#define SEEKABLE_BIT  0x08
#define SPLICE_BIT    0x04
#define IP_PORT_BITMASK                 0x02
#define OP_PORT_BITMASK                 0x01
#define IP_OP_PORT_BITMASK              0x03
#define WMA_DSP_MIN_BUF_SIZE            8192
#define WMA_DSP_MIN_CHANNELS            1
#define WMA_DSP_MAX_CHANNELS            2

#define CxSpeakerOrig     0
#define CxSpeaker2        1 // LEFT | RIGHT
#define ENCOPT4_PLUSVER   0xe000
#define ENCOPT4_PLUSV1    0xc000
#define ENCOPT4_PLUSV1ALT 0x8000
#define ENCOPT4_PLUSV2    0x2000
#define ENCOPT4_PLUSV3    0x4000
#define ENCOPT4_PLUSV2_CHEX        0x0004 // 1b
#define ENCOPT4_PLUSV2_CXLCCHMASK 0x00000007 // 3b
#define ENCOPT4_PLUSV2_CXLCCHMASK_BITSOFFSET 0

class omx_wma_adec;
class timer;
class omxBufMgr;
typedef struct timerinfo
{
    pthread_t    thr;
    timer        *pTimer;
    omx_wma_adec *base;
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
    omx_wma_adec        *m_base;
    TIMERINFO           *m_timerinfo;
public:
    timer(omx_wma_adec* base);
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
    inline omx_wma_adec* get_comp()
    {
        return m_base;
    }
};

// OMX WMA audio decoder class
class omx_wma_adec: public qc_omx_component
{
public:
    omx_wma_adec();  // constructor
    virtual ~omx_wma_adec();  // destructor

    OMX_ERRORTYPE allocate_buffer(OMX_HANDLETYPE             hComp,
        OMX_BUFFERHEADERTYPE **bufferHdr,
        OMX_U32                     port,
        OMX_PTR                  appData,
        OMX_U32                    bytes);


    OMX_ERRORTYPE component_deinit(OMX_HANDLETYPE hComp);

    OMX_ERRORTYPE component_init(OMX_STRING role);

    OMX_ERRORTYPE component_role_enum(OMX_HANDLETYPE hComp,
        OMX_U8         *role,
        OMX_U32        index);

    OMX_ERRORTYPE component_tunnel_request(OMX_HANDLETYPE             hComp,
        OMX_U32                     port,
        OMX_HANDLETYPE     peerComponent,
        OMX_U32                 peerPort,
        OMX_TUNNELSETUPTYPE *tunnelSetup);

    OMX_ERRORTYPE empty_this_buffer(OMX_HANDLETYPE         hComp,
        OMX_BUFFERHEADERTYPE *buffer);


    OMX_ERRORTYPE empty_this_buffer_proxy(OMX_HANDLETYPE         hComp,
        OMX_BUFFERHEADERTYPE *buffer);


    OMX_ERRORTYPE fill_this_buffer(OMX_HANDLETYPE         hComp,
        OMX_BUFFERHEADERTYPE *buffer);


    OMX_ERRORTYPE free_buffer(OMX_HANDLETYPE         hComp,
        OMX_U32                 port,
        OMX_BUFFERHEADERTYPE *buffer);

    OMX_ERRORTYPE get_component_version(OMX_HANDLETYPE              hComp,
        OMX_STRING          componentName,
        OMX_VERSIONTYPE *componentVersion,
        OMX_VERSIONTYPE *     specVersion,
        OMX_UUIDTYPE       *componentUUID);

    OMX_ERRORTYPE get_config(OMX_HANDLETYPE      hComp,
        OMX_INDEXTYPE configIndex,
        OMX_PTR        configData);

    OMX_ERRORTYPE get_extension_index(OMX_HANDLETYPE     hComp,
        OMX_STRING     paramName,
        OMX_INDEXTYPE *indexType);

    OMX_ERRORTYPE get_parameter(OMX_HANDLETYPE hComp,
        OMX_INDEXTYPE paramIndex,
        OMX_PTR paramData);

    OMX_ERRORTYPE get_state(OMX_HANDLETYPE hComp,
        OMX_STATETYPE *state);

    static void process_in_port_msg(void          *client_data,
        unsigned char id);

    static void process_out_port_msg(void          *client_data,
        unsigned char id);

    static void process_command_msg(void          *client_data,
        unsigned char id);

    static void process_event_cb(void          *client_data,
                                   unsigned char id);
    OMX_ERRORTYPE set_callbacks(OMX_HANDLETYPE hComp,
        OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData);

    OMX_ERRORTYPE set_config(OMX_HANDLETYPE hComp,
        OMX_INDEXTYPE configIndex,
        OMX_PTR configData);

    OMX_ERRORTYPE set_parameter(OMX_HANDLETYPE hComp,
        OMX_INDEXTYPE paramIndex,
        OMX_PTR paramData);

    OMX_ERRORTYPE use_buffer(OMX_HANDLETYPE             hComp,
        OMX_BUFFERHEADERTYPE **bufferHdr,
        OMX_U32                     port,
        OMX_PTR                  appData,
        OMX_U32                    bytes,
        OMX_U8                  *buffer);

    OMX_ERRORTYPE use_EGL_image(OMX_HANDLETYPE             hComp,
        OMX_BUFFERHEADERTYPE **bufferHdr,
        OMX_U32                     port,
        OMX_PTR                  appData,
        void *                  eglImage);

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

    // Bit Positions
    enum flags_bit_positions
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


    typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*>
        input_buffer_map;

    typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*>
        output_buffer_map;

    enum port_indexes
    {
        OMX_CORE_INPUT_PORT_INDEX        =0,
        OMX_CORE_OUTPUT_PORT_INDEX       =1
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
        bool get_msg_with_id(unsigned *p1,unsigned *p2, unsigned id);
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

    typedef struct metadata_output_suspend
    {
        int       offsetVal;
        OMX_TICKS nTimeStamp;
        OMX_U32   nFlags;
    }__attribute__((packed))META_OUT_SUS;

    ///////////////////////////////////////////////////////////
    // Member variables
    ///////////////////////////////////////////////////////////
    timer                          *m_timer;
    OMX_U8                         *m_tmp_in_meta_buf;
    OMX_U8                         *m_tmp_out_meta_buf;
    OMX_U8                         m_flush_cnt ;
    OMX_U8                         m_eos_bm;
    OMX_U8                       m_first;
    OMX_U8                       bTsupdate;

    omxBufMgr                    *m_bufMgr;
    int                            nNumInputBuf;
    int                            nNumOutputBuf;
    int                            m_drv_fd;   // Kernel device node file handle
    bool                           bFlushinprogress;
    bool                           bSuspendEventRxed;
    bool                           bResumeEventRxed;
    bool                           bEOSSent;
    bool                           cmdwaiting;
    bool                           m_event_thread_kill;
    bool                           fake_eos_recieved;
    bool                           is_in_th_sleep;
    bool                           is_out_th_sleep;
    bool                           bOutputPortReEnabled;
    bool                           bSendWMAPacketpending;
    bool                           m_pause_to_exe;
    bool                           bBufferAfterFlush;
    bool                           waitForSuspendCmplFlg;
    unsigned                       m_msg_cnt; // message count
    unsigned                       m_cmd_cnt; // command count
    unsigned                       m_etb_cnt; // Empty This Buffer count
    unsigned                       m_ebd_cnt; // Empty Buffer Done Count
    unsigned int                   frameDuration;//duration of each frame
    unsigned int                   m_inp_act_buf_count;// Num of Input Buffers
    unsigned int                   m_out_act_buf_count;// Numb of Output Buffers
    unsigned int                   m_inp_current_buf_count;// Num of Input Buffers
    unsigned int                   m_out_current_buf_count;// Numb of Output Buffers

    unsigned int                   m_trans_buf_cnt;//Num of transcoding Buffers
    unsigned int                   m_flags;//encapsulate the waiting states.
    unsigned int                   fbd_cnt;
    OMX_TICKS                      nTimestamp;
    OMX_TICKS                      wma_packet_timestamp;
    unsigned int                   pcm_feedback;//tunnel or non-tunnel
    unsigned int                   wma10Pro;
    unsigned int                   ntotal_playtime;
    unsigned int                   output_buffer_size;
    unsigned int                   input_buffer_size;
    int                            fwrite_count;
    int                            set_pcm_config;
    unsigned int                   packet_num;
    unsigned short                 m_session_id;

    OMX_PRIORITYMGMTTYPE           m_priority_mgm ;
    OMX_PARAM_BUFFERSUPPLIERTYPE   m_buffer_supplier;
    // store I/P PORT state
    OMX_BOOL                       m_inp_bEnabled;
    // store O/P PORT state
    OMX_BOOL                       m_out_bEnabled;
    //Input port Populated
    OMX_BOOL                       m_inp_bPopulated;
    //Output port Populated
    OMX_BOOL                       m_out_bPopulated;
    OMX_BOOL                     m_is_use_in_buffer;
    sem_t                          sem_States;
    sem_t                          sem_read_msg;
    sem_t                          sem_WaitForSuspendCmpl_states;

    sem_t                          sem_write_msg;
    volatile int                   m_is_event_done;
    volatile int                   m_is_in_th_sleep;
    volatile int                   m_is_out_th_sleep;
    input_buffer_map               m_input_buf_hdrs;
    output_buffer_map              m_output_buf_hdrs;
    omx_cmd_queue                  m_input_q;
    omx_cmd_queue                  m_input_ctrl_cmd_q;
    omx_cmd_queue                  m_input_ctrl_ebd_q;
    omx_cmd_queue                  m_command_q;
    omx_cmd_queue                  m_output_q;
    omx_cmd_queue                  m_output_ctrl_cmd_q;
    omx_cmd_queue                  m_output_ctrl_fbd_q;
    pthread_mutexattr_t            m_suspendresume_lock_attr;
    pthread_mutexattr_t            m_outputlock_attr;
    pthread_mutexattr_t            m_commandlock_attr;
    pthread_mutexattr_t            m_lock_attr;
    pthread_mutexattr_t            m_state_attr;
    pthread_mutexattr_t            m_flush_attr;
    pthread_mutexattr_t            m_in_th_attr_1;
    pthread_mutexattr_t            m_out_th_attr_1;
    pthread_mutexattr_t            m_event_attr;
    pthread_mutexattr_t            m_in_th_attr;
    pthread_mutexattr_t            m_out_th_attr;
    pthread_mutexattr_t            out_buf_count_lock_attr;
    pthread_mutexattr_t            in_buf_count_lock_attr;
    pthread_cond_t                 cond;
    pthread_cond_t                 in_cond;
    pthread_cond_t                 out_cond;
    pthread_mutexattr_t            m_WaitForSuspendCmpl_lock_attr;
    pthread_mutex_t                m_WaitForSuspendCmpl_lock;
    pthread_mutex_t                m_suspendresume_lock;
    pthread_mutex_t                m_lock;
    pthread_mutex_t                m_commandlock;
    pthread_mutex_t                m_outputlock;
    // Mutexes for state change
    pthread_mutex_t                m_state_lock;
    // Mutexes for  flush acks from input and output threads
    pthread_mutex_t                m_flush_lock;
    pthread_mutex_t                m_event_lock;
    pthread_mutex_t                m_in_th_lock;
    pthread_mutex_t                m_out_th_lock;
    pthread_mutex_t                m_in_th_lock_1;
    pthread_mutex_t                m_out_th_lock_1;
    pthread_mutex_t                out_buf_count_lock;
    pthread_mutex_t                in_buf_count_lock;
    OMX_PTR                        m_app_data;// Application data
    OMX_S32                        m_volume;//Unit to be determined
    OMX_STATETYPE                  m_state;// OMX State
    OMX_STATETYPE                  nState;
    OMX_CALLBACKTYPE               m_cb;// Application callbacks
    struct wma_ipc_info            *m_ipc_to_in_th;// for input thread
    struct wma_ipc_info            *m_ipc_to_out_th;// for output thread
    struct wma_ipc_info            *m_ipc_to_cmd_th;// for command thread
    struct wma_ipc_info            *m_ipc_to_event_th;    //for txco event thread
    OMX_AUDIO_PARAM_WMATYPE        m_adec_param; // Cache WMA decoder parameter
    QOMX_AUDIO_PARAM_WMA10PROTYPE   m_adec_param_wma10Pro;
    OMX_AUDIO_PARAM_PCMMODETYPE    m_pcm_param; // Cache pcm  parameter
    OMX_SUSPENSIONPOLICYTYPE       suspensionPolicy;
    OMX_PARAM_COMPONENTROLETYPE    component_Role;
    OMX_U8                         m_comp_deinit;

    ///////////////////////////////////////////////////////////
    // Private methods
    ///////////////////////////////////////////////////////////
    OMX_ERRORTYPE allocate_output_buffer(OMX_HANDLETYPE       hComp,
        OMX_BUFFERHEADERTYPE **bufferHdr,
        OMX_U32 port,OMX_PTR appData,
        OMX_U32              bytes);

    OMX_ERRORTYPE allocate_input_buffer(OMX_HANDLETYPE       hComp,
        OMX_BUFFERHEADERTYPE **bufferHdr,
        OMX_U32              port,
        OMX_PTR              appData,
        OMX_U32              bytes);

    OMX_ERRORTYPE use_input_buffer(OMX_IN OMX_HANDLETYPE          hComp,
        OMX_INOUT OMX_BUFFERHEADERTYPE **bufHdr,
        OMX_IN OMX_U32                 port,
        OMX_IN OMX_PTR                 appData,
        OMX_IN OMX_U32                 bytes,
        OMX_IN OMX_U8*                 buffer);

    OMX_ERRORTYPE use_output_buffer(OMX_IN OMX_HANDLETYPE          hComp,
        OMX_INOUT OMX_BUFFERHEADERTYPE **bufHdr,
        OMX_IN OMX_U32                 port,
        OMX_IN OMX_PTR                 appData,
        OMX_IN OMX_U32                 bytes,
        OMX_IN OMX_U8*                 buffer);

    OMX_ERRORTYPE fill_this_buffer_proxy(OMX_HANDLETYPE       hComp,
        OMX_BUFFERHEADERTYPE *buffer);

    OMX_ERRORTYPE send_command_proxy(OMX_HANDLETYPE  hComp,
        OMX_COMMANDTYPE cmd,
        OMX_U32         param1,
        OMX_PTR         cmdData);

    OMX_ERRORTYPE send_command(OMX_HANDLETYPE hComp,
        OMX_COMMANDTYPE  cmd,
        OMX_U32       param1,
        OMX_PTR      cmdData);

    bool allocate_done(void);

    bool release_done(OMX_U32         param1);

    bool execute_omx_flush(OMX_IN OMX_U32 param1, bool cmd_cmpl=true);
    bool execute_input_omx_flush(void);
    bool execute_output_omx_flush(void);
    bool search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer);
    bool search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer);
    bool post_input(unsigned int p1, unsigned int p2,
        unsigned int id);

    bool post_output(unsigned int p1, unsigned int p2,
        unsigned int id);

    void process_events(omx_wma_adec *client_data);
    bool omx_wma_fake_eos(void);
    void append_data_to_temp_buf(void);

    void buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr);
    void frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr);
    void wait_for_event();
    void event_complete();
    void in_th_goto_sleep();
    void in_th_wakeup();
    void out_th_goto_sleep();
    void out_th_wakeup();
    void flush_ack();
    unsigned int          fcount;
    int logarithm(int);

     void deinit_decoder();
    OMX_U8 *m_trans_buffer;
    OMX_U8 *m_trans_buffer_start;
    OMX_U32 m_trans_buffer_length;
    OMX_U32 m_dwSpillOverBitsLength;
    bool m_notfirstblock;

    void GetByteFromBitStream(OMX_U8 *pByte, OMX_U8 *pSrc, int nFromBit, int nBits);
    unsigned long WmAudioGetSamplesPerFrame(unsigned long SamplesPerSec, unsigned int nVersion);
    OMX_ERRORTYPE transcode_wma(OMX_HANDLETYPE hComp, OMX_U8 *pSrc,OMX_BUFFERHEADERTYPE* buffer);
    OMX_ERRORTYPE transcode_wma10Pro(OMX_HANDLETYPE hComp, OMX_U8 *pSrc,OMX_BUFFERHEADERTYPE* buffer);
    void SendWMAPacket(OMX_BUFFERHEADERTYPE* buffer);
    void SendDummyPacket(OMX_BUFFERHEADERTYPE* buffer);

};


class omxBufMgr
{
public:
    omxBufMgr():
             m_remaining_space(OMX_TCXO_BUFFER-1),
             m_rejected_bytes(0),
             m_tot_rejected_bytes(0),
             m_start(NULL),
             m_read(NULL),
             m_write(NULL),
             m_end(NULL)
    {
        m_start = (unsigned char*)
                     malloc(OMX_TCXO_BUFFER*sizeof(unsigned char));
        memset(m_start,0,OMX_TCXO_BUFFER);

        m_end = m_start + OMX_TCXO_BUFFER;
        m_write = m_start;
        m_read = m_start;
        DEBUG_PRINT("%s m_start[%p] m_end[%p]\n",__FUNCTION__,m_start,m_end);
    }

    ~omxBufMgr()
    {
       m_read = NULL;
       m_write= NULL;
       m_end= NULL;
       m_rejected_bytes = 0;
       m_tot_rejected_bytes = 0;
       if(m_start)
           free(m_start);
    }
    inline void reset()
    {
        m_read = m_start;
        m_write = m_start;
        m_remaining_space = OMX_TCXO_BUFFER-1;
        m_rejected_bytes = 0;
    }
    inline bool isEmpty()
    {
        if(m_remaining_space == (OMX_TCXO_BUFFER -1))
            return true;
        else
            return false;
    }
    inline OMX_U32 getBufFilledSpace()
    {
        return (OMX_TCXO_BUFFER -1 - m_remaining_space);
    }

    inline OMX_U32 getBufFreeSpace()
    {
        return (m_remaining_space);
    }
    OMX_U32  appendToBuf(OMX_U8 *srcBuf, OMX_U32 srcLen);
    OMX_U32  emptyToBuf(OMX_U8 *destBuf, OMX_U32 len );

    void print();
private:
    OMX_U32  m_remaining_space;
    OMX_U32  m_rejected_bytes;
    OMX_U32  m_tot_rejected_bytes;
    OMX_U8   *m_start;
    OMX_U8   *m_read;
    OMX_U8   *m_write;
    OMX_U8   *m_end;
};

#endif //_OMX_WMA_DEC_H_

