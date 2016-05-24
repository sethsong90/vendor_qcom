#ifndef _OMX_BASE_H
#define _OMX_BASE_H_

/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_base.h
This module contains the class definition for openMAX decoder component.

Copyright (c) 2006-2008, 2012 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*=====================================================================*/
/*=========================================================================
                            Edit History

$Header:
when       who     what, where, why
--------   ---     -------------------------------------------------------
=========================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "omx_utils.h"
#include "omx_base_utils.h"
#include <linux/msm_audio.h>

#include "OMX_Core.h"
#include "OMX_Audio.h"
#include "qc_omx_component.h"
#include "QOMX_AudioIndexExtensions.h"
#include "QOMX_AudioExtensions.h"

#include "Map.h"

extern "C" {
    void * get_omx_component_factory_fn();
    void omx_cmd_msg(void *client_data, unsigned char id);
    void omx_in_msg(void *client_data, unsigned char id);
    void omx_out_msg(void *client_data, unsigned char id);
    void omx_event_msg(void *client_data, unsigned char id);
}

#ifdef _ANDROID_
#undef LOG_TAG
#define LOG_TAG "QC_BASE"
#endif
//////////////////////////////////////////////////////////////////////////////
//               Macros
////////////////////////////////////////////////////////////////////////////
#define OMX_SPEC_VERSION  0x00000101
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (x >= y?x:y)
#define DSP_MIN_BUF_SIZE 1550

///////////////////////////////////////////////////////////////////////////////
// Class declarations
///////////////////////////////////////////////////////////////////////////////

class COmxTimer
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
    COmxBase            *m_base;
    TIMERINFO           *m_timerinfo;
public:
    COmxTimer(COmxBase* base);
    ~COmxTimer();

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
    inline COmxBase* get_comp()
    {
        return m_base;
    }
};

class COmxBaseIn
{
public:
    COmxBaseIn(COmxBase * base, int fd,OMX_CALLBACKTYPE cb,
    OMX_PTR appdata);

    virtual ~COmxBaseIn();

    virtual bool post_input(unsigned int p1,unsigned int p2,unsigned int id);

    virtual void process_in_port_msg( unsigned char id)=0;

    virtual void buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr);

    bool execute_input_omx_flush();

    inline void setCtxt(struct ipc_info* pCtxt)
    {
        m_ipc_to_in_th = pCtxt;
    }

    inline void set_cb(OMX_CALLBACKTYPE cb, OMX_PTR app_data)
    {
        m_cb = cb;
        m_app_data = app_data;
    }
    inline OMX_CALLBACKTYPE get_cb()
    {
        return m_cb ;
    }
    inline OMX_PTR get_app_data()
    {
        return m_app_data ;
    }

    inline void set_state(OMX_STATETYPE state)
    {
        pthread_mutex_lock(&m_state_lock);

        // If old state is executing
        // and new state is idle,
        // we reset operation values.
        if ((m_state == OMX_StateExecuting) && (state == OMX_StateIdle))
        {
            doReset();
        }

        m_state = state;
        pthread_mutex_unlock(&m_state_lock);
    }
    inline OMX_STATETYPE get_state()
    {
        pthread_mutex_lock(&m_state_lock);
        OMX_STATETYPE state = m_state;
        pthread_mutex_unlock(&m_state_lock);
        return state;
    }
    inline COmxBase* get_comp()
    {
        return m_comp;
    }
    inline void set_cnt_in_buf(unsigned int numBuf)
    {
        pthread_mutex_lock(&m_in_buf_count_lock);
        nNumInputBuf = numBuf;
        pthread_mutex_unlock(&m_in_buf_count_lock);
        return ;
    }
    inline unsigned int get_cnt_in_buf()
    {
        unsigned int numBuf=0;
        pthread_mutex_lock(&m_in_buf_count_lock);
        numBuf = nNumInputBuf;
        pthread_mutex_unlock(&m_in_buf_count_lock);
        return numBuf;
    }
    inline void inc_cnt_in_buf()
    {
        pthread_mutex_lock(&m_in_buf_count_lock);
        ++nNumInputBuf;
        pthread_mutex_unlock(&m_in_buf_count_lock);
        return;
    }
    inline void dec_cnt_in_buf()
    {
        pthread_mutex_lock(&m_in_buf_count_lock);
        --nNumInputBuf ;
        pthread_mutex_unlock(&m_in_buf_count_lock);
        return;
    }
    inline omx_cmd_queue * get_cmd_q()
    {
        return &m_input_ctrl_cmd_q;
    }
    inline omx_cmd_queue * get_bd_q()
    {
        return &m_input_ctrl_ebd_q;
    }
    inline omx_cmd_queue * get_data_q()
    {
        return &m_input_q;
    }
    inline unsigned int get_q_size(omx_cmd_queue *q)
    {
        unsigned int q_s=0;
        pthread_mutex_lock(&m_lock);
        q_s = q->m_size;
        pthread_mutex_unlock(&m_lock);
        return q_s;
    }
    inline bool get_msg(omx_cmd_queue *q,unsigned *p1, unsigned *p2,
    unsigned char *id)
    {
        bool flg = false;
        pthread_mutex_lock(&m_lock);
        flg = q->pop_entry(p1,p2,id);
        pthread_mutex_unlock(&m_lock);
        return flg;
    }
    inline int get_drv_fd()
    {
        return m_drv_fd;
    }
    inline void set_drv_fd(int fd)
    {
        m_drv_fd = fd;
    }
    inline void reset_drv_fd()
    {
        m_drv_fd = -1;
    }
    inline void set_inth_ctxt(struct ipc_info* ctxt)
    {
        m_ipc_to_in_th = ctxt;
    }
    inline struct ipc_info* get_inth_ctxt()
    {
        return m_ipc_to_in_th ;
    }
    inline unsigned int get_sf()
    {
        return m_sample_rate;
    }
    inline void set_sf(unsigned int sf)
    {
        m_sample_rate = sf;
    }
    inline OMX_U8 get_ch()
    {
        return m_ch_cfg;
    }
    inline void set_ch(OMX_U8 ch)
    {
        m_ch_cfg = ch;
    }
    typedef struct
    {
        OMX_U32 tot_in_buf_len;
        OMX_U32 etb_cnt;
        OMX_U32 ebd_cnt;
    }IN_PB_STATS;

    IN_PB_STATS         m_in_pb_stat;
    OMX_BOOL            m_input_port_flushed;

protected:
    // Virtual function designed to be implemented by the derived classes
    // so that they can reset any variables requiring reset when
    // changing state from Executing to Idle.
    // Providing an empty implementation, since there's nothing to reset 
    // at this level, and don't really want to deal with the design change
    // in all derived classes or other scenarios.
    // Not public. Only accesible by derived classes.
    virtual void doReset() 
    {
    };

private:
    int                 m_drv_fd;
    OMX_U8              m_ch_cfg;
    COmxBase            *m_comp;
    struct ipc_info     *m_ipc_to_in_th;
    OMX_CALLBACKTYPE    m_cb;

    omx_cmd_queue       m_input_q;
    omx_cmd_queue       m_input_ctrl_cmd_q;
    omx_cmd_queue       m_input_ctrl_ebd_q;

    OMX_PTR             m_app_data;
    OMX_BOOL            m_inp_bPopulated;
    OMX_STATETYPE       m_state;

    unsigned int        m_inp_act_buf_count;
    unsigned int        m_input_buffer_size;
    unsigned int        nNumInputBuf;
    unsigned int        m_sample_rate;

    pthread_mutex_t     m_lock;
    pthread_mutex_t     m_state_lock;
    pthread_mutex_t     m_in_buf_count_lock;
    pthread_mutexattr_t m_lock_attr;
    pthread_mutexattr_t m_state_attr;
    pthread_mutexattr_t m_in_buf_count_lock_attr;
};

class COmxBaseOut
{
public:
    COmxBaseOut(COmxBase * base, int fd,OMX_CALLBACKTYPE cb,
    OMX_PTR appdata);

    virtual ~COmxBaseOut();

    bool post_output(unsigned int p1, unsigned int p2, unsigned int id);

    virtual void process_out_port_msg(unsigned char id)=0;

    virtual void frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr);

    inline void set_outh_ctxt(struct ipc_info* pCtxt)
    {
        m_ipc_to_out_th = pCtxt;
    }
    inline struct ipc_info* get_outh_ctxt()
    {
        return m_ipc_to_out_th ;
    }
    inline void set_cb(OMX_CALLBACKTYPE cb, OMX_PTR app_data)
    {
        m_cb = cb;
        m_app_data = app_data;
    }
    inline OMX_CALLBACKTYPE get_cb()
    {
        return m_cb ;
    }
    inline OMX_PTR get_app_data()
    {
        return m_app_data ;
    }
    inline COmxBase* get_comp()
    {
        return m_comp;
    }
    inline void set_state(OMX_STATETYPE state)
    {
        pthread_mutex_lock(&m_state_lock);
        m_state = state;
        pthread_mutex_unlock(&m_state_lock);
    }
    inline OMX_STATETYPE get_state()
    {
        OMX_STATETYPE state;
        pthread_mutex_lock(&m_state_lock);
        state =m_state;
        pthread_mutex_unlock(&m_state_lock);
        return state;
    }
    inline void set_out_buf_cnt(unsigned int numBuf)
    {
        pthread_mutex_lock(&out_buf_count_lock);
        nNumOutputBuf = numBuf;
        pthread_mutex_unlock(&out_buf_count_lock);
        return;
    }
    inline void inc_cnt_out_buf()
    {
        pthread_mutex_lock(&out_buf_count_lock);
        ++nNumOutputBuf;
        pthread_mutex_unlock(&out_buf_count_lock);
        return;
    }
    inline void dec_cnt_out_buf()
    {
        pthread_mutex_lock(&out_buf_count_lock);
        --nNumOutputBuf ;
        pthread_mutex_unlock(&out_buf_count_lock);
        return;
    }
    inline unsigned int get_cnt_out_buf()
    {
        unsigned int numBuf=0;
        pthread_mutex_lock(&out_buf_count_lock);
        numBuf = nNumOutputBuf;
        pthread_mutex_unlock(&out_buf_count_lock);
        return numBuf;
    }
    bool execute_output_omx_flush();

    inline int get_drv_fd()
    {
        return m_drv_fd;
    }
    inline void set_drv_fd(int fd)
    {
        m_drv_fd = fd;
    }
    inline void reset_drv_fd()
    {
        m_drv_fd = -1;
    }
    inline omx_cmd_queue * get_cmd_q()
    {
        return &m_output_ctrl_cmd_q;
    }
    inline omx_cmd_queue * get_bd_q()
    {
        return &m_output_ctrl_fbd_q;
    }
    inline omx_cmd_queue * get_data_q()
    {
        return &m_output_q;
    }
    inline unsigned int get_q_size(omx_cmd_queue * q)
    {
        unsigned int q_s=0;
        pthread_mutex_lock(&m_outputlock);
        q_s = q->m_size;
        pthread_mutex_unlock(&m_outputlock);
        return q_s;
    }
    inline bool get_msg_qid(omx_cmd_queue *q,unsigned char *ident)
    {
        return (q->get_msg_id(ident));
    }
    inline bool get_msg(omx_cmd_queue *q,unsigned *p1, unsigned *p2,
    unsigned char *id)
    {
        bool flg = false;
        pthread_mutex_lock(&m_outputlock);
        flg = q->pop_entry(p1,p2,id);
        pthread_mutex_unlock(&m_outputlock);
        return flg;
    }
private:
    typedef struct
    {
        OMX_U32 tot_out_buf_len;
        OMX_U32 tot_pb_time;
        OMX_U32 fbd_cnt;
        OMX_U32 ftb_cnt;
    }OUT_PB_STATS;

    int                  m_drv_fd;
    COmxBase             *m_comp;
    struct ipc_info      *m_ipc_to_out_th;

    unsigned int         output_buffer_size;
    unsigned int         nNumOutputBuf;

    omx_cmd_queue        m_output_q;
    omx_cmd_queue        m_output_ctrl_cmd_q;
    omx_cmd_queue        m_output_ctrl_fbd_q;

    pthread_mutexattr_t  m_outputlock_attr;
    pthread_mutexattr_t  out_buf_count_lock_attr;
    pthread_mutexattr_t  m_state_attr;

    pthread_mutex_t      m_outputlock;
    pthread_mutex_t      m_state_lock;
    pthread_mutex_t      out_buf_count_lock;

    OMX_PTR              m_app_data;
    OMX_CALLBACKTYPE     m_cb;
    OMX_STATETYPE        m_state;
    OUT_PB_STATS         m_out_pb_stat;
};

///////////////////////////////////////////////////////////////////////////////
// Openmax component base class
///////////////////////////////////////////////////////////////////////////////
class COmxBase:public qc_omx_component
{
public:
    COmxBase(const char *devName,OMX_U32 sf, OMX_U8 ch);

    virtual ~COmxBase();

    virtual OMX_ERRORTYPE component_init(
    OMX_STRING role);

    virtual OMX_ERRORTYPE component_deinit(
    OMX_HANDLETYPE hComp);

    virtual OMX_ERRORTYPE component_role_enum(
    OMX_HANDLETYPE hComp,
    OMX_U8         *role,
    OMX_U32        index)=0;

    virtual OMX_ERRORTYPE get_state(
    OMX_HANDLETYPE hComp,
    OMX_STATETYPE  *state);

    virtual OMX_ERRORTYPE get_config(
    OMX_HANDLETYPE hComp,
    OMX_INDEXTYPE  configIndex,
    OMX_PTR        configData);

    virtual OMX_ERRORTYPE get_parameter(
    OMX_HANDLETYPE hComp,
    OMX_INDEXTYPE  paramIndex,
    OMX_PTR        paramData);

    virtual OMX_ERRORTYPE get_extension_index(
    OMX_HANDLETYPE hComp,
    OMX_STRING     paramName,
    OMX_INDEXTYPE  *indexType);

    virtual OMX_ERRORTYPE get_component_version(
    OMX_HANDLETYPE  hComp,
    OMX_STRING      componentName,
    OMX_VERSIONTYPE *componentVersion,
    OMX_VERSIONTYPE *specVersion,
    OMX_UUIDTYPE    *componentUUID);

    virtual OMX_ERRORTYPE set_parameter(
    OMX_HANDLETYPE hComp,
    OMX_INDEXTYPE  paramIndex,
    OMX_PTR        paramData);

    virtual OMX_ERRORTYPE set_callbacks(
    OMX_HANDLETYPE   hComp,
    OMX_CALLBACKTYPE *callbacks,
    OMX_PTR          appData);

    virtual OMX_ERRORTYPE set_config(
    OMX_HANDLETYPE hComp,
    OMX_INDEXTYPE configIndex,
    OMX_PTR configData);

    virtual OMX_ERRORTYPE send_command(OMX_HANDLETYPE hComp,
    OMX_COMMANDTYPE  cmd,
    OMX_U32       param1,
    OMX_PTR      cmdData);

    virtual OMX_ERRORTYPE empty_this_buffer(
    OMX_HANDLETYPE       hComp,
    OMX_BUFFERHEADERTYPE *buffer);

    virtual OMX_ERRORTYPE fill_this_buffer(
    OMX_HANDLETYPE       hComp,
    OMX_BUFFERHEADERTYPE *buffer);

    virtual OMX_ERRORTYPE free_buffer(OMX_HANDLETYPE       hComp,
    OMX_U32              port,
    OMX_BUFFERHEADERTYPE *buffer);

    virtual OMX_ERRORTYPE allocate_buffer(
    OMX_HANDLETYPE       hComp,
    OMX_BUFFERHEADERTYPE **bufferHdr,
    OMX_U32              port,
    OMX_PTR              appData,
    OMX_U32              bytes);

    virtual OMX_ERRORTYPE use_buffer(
    OMX_HANDLETYPE       hComp,
    OMX_BUFFERHEADERTYPE **bufferHdr,
    OMX_U32              port,
    OMX_PTR              appData,
    OMX_U32              bytes,
    OMX_U8               *buffer);

    virtual OMX_ERRORTYPE component_tunnel_request(
    OMX_HANDLETYPE      hComp,
    OMX_U32             port,
    OMX_HANDLETYPE      peerComponent,
    OMX_U32             peerPort,
    OMX_TUNNELSETUPTYPE *tunnelSetup);

    virtual OMX_ERRORTYPE use_EGL_image(
    OMX_HANDLETYPE      hComp,
    OMX_BUFFERHEADERTYPE **bufferHdr,
    OMX_U32              port,
    OMX_PTR              appData,
    void *               eglImage);

    virtual void process_command_msg( unsigned char id);

    bool post_command(unsigned int p1, unsigned int p2,
    unsigned int id);

    virtual OMX_ERRORTYPE process_cmd(OMX_HANDLETYPE  hComp,
    OMX_COMMANDTYPE cmd,
    OMX_U32         param1,
    OMX_PTR         cmdData);

    inline void set_state(OMX_STATETYPE state)
    {
        pthread_mutex_lock(&m_state_lock);
        m_state = state;
        pthread_mutex_unlock(&m_state_lock);
    }
    inline OMX_STATETYPE get_state()
    {
        OMX_STATETYPE state;
        pthread_mutex_lock(&m_state_lock);
        state =m_state;
        pthread_mutex_unlock(&m_state_lock);
        return state;
    }
    void setTS(unsigned int TS);

    bool search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer);

    bool search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer);

    OMX_ERRORTYPE deinit();

    void flush_ack();

    void wait_for_event();

    void event_complete();

    void in_th_sleep();

    void in_th_timedsleep();

    void out_th_sleep();

    void in_th_wakeup();

    void in_th_timedwakeup();

    void out_th_wakeup();

    inline void set_inp_bEnabled(OMX_BOOL flg){m_inp_bEnabled = flg;}
    inline void set_out_bEnabled(OMX_BOOL flg){m_out_bEnabled = flg;}
    inline void set_inp_bPopulated(OMX_BOOL flg){m_inp_bPopulated = flg;}
    inline void set_out_bPopulated(OMX_BOOL flg){m_out_bPopulated = flg;}
    inline OMX_BOOL get_inp_bEnabled(){ return m_inp_bEnabled;}
    inline OMX_BOOL get_out_bEnabled(){ return m_out_bEnabled;}
    inline OMX_BOOL get_inp_bPopulated(){ return m_inp_bPopulated;}
    inline OMX_BOOL get_out_bPopulated(){ return m_out_bPopulated;}

    inline unsigned int get_inp_act_buf_cnt(){return m_inp_act_buf_count;}
    inline unsigned int get_out_act_buf_cnt(){return m_out_act_buf_count;}
    inline void set_in_act_buf_cnt(unsigned int cnt){m_inp_act_buf_count = cnt;}
    inline void set_out_act_buf_cnt(unsigned int cnt){m_out_act_buf_count=cnt;}

    inline void trigger_eos(OMX_BOOL flg)
    {
        m_trigger_eos = flg;
    }
    inline OMX_BOOL is_trigger_eos()
    {
        return m_trigger_eos;
    }
    inline unsigned int getTS()
    {
        return nTimeStamp;
    }
    inline bool getSuspendFlg()
    {
        return bSuspendEventRxed;
    }
    inline void setSuspendFlg()
    {
        pthread_mutex_lock(&m_suspendresume_lock);
        bSuspendEventRxed = true;
        pthread_mutex_unlock(&m_suspendresume_lock);
    }
    inline void resetSuspendFlg()
    {
        pthread_mutex_lock(&m_suspendresume_lock);
        bSuspendEventRxed = false;
        pthread_mutex_unlock(&m_suspendresume_lock);
    }
    inline bool getResumeFlg()
    {
        return bResumeEventRxed;
    }
    inline void setResumeFlg()
    {
        pthread_mutex_lock(&m_suspendresume_lock);
        bResumeEventRxed = true;
        pthread_mutex_unlock(&m_suspendresume_lock);
    }
    inline void resetResumeFlg()
    {
        pthread_mutex_lock(&m_suspendresume_lock);
        bResumeEventRxed = false;
        pthread_mutex_unlock(&m_suspendresume_lock);
    }
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
    inline OMX_U8 get_eos_bm()
    {
        return m_eos_bm;
    }
    inline void set_eos_bm(OMX_U8 eos)
    {
        m_eos_bm = eos;
    }
    inline bool get_flush_stat()
    {
        return m_flush_in_prog;
    }
    inline void set_is_pause_to_exe(bool flg)
    {
        m_pause_to_exe = flg;
    }
    inline bool is_pause_to_exe()
    {
        return m_pause_to_exe;
    }
    inline int get_drv_fd()
    {
        return m_drv_fd;
    }
    inline void set_drv_fd(int fd)
    {
        m_drv_fd = fd;
    }
    inline void reset_drv_fd()
    {
        m_drv_fd = -1;
    }
    inline unsigned int get_sf()
    {
        return m_sample_rate;
    }
    inline void set_sf(unsigned int sf)
    {
        m_sample_rate = sf;
    }
    inline OMX_U8 get_ch()
    {
        return m_ch_cfg;
    }
    inline void set_ch(OMX_U8 ch)
    {
        m_ch_cfg = ch;
    }
    inline void set_port_recfg(bool flg)
    {
        bOutputPortReEnabled = flg;
    }
    inline bool get_port_recfg()
    {
        return bOutputPortReEnabled;
    }
    inline unsigned int get_in_buf_size()
    {
        return m_input_buffer_size;
    }
    inline void set_in_buf_size(unsigned int bufsize)
    {
        m_input_buffer_size = bufsize;
    }
    inline unsigned int get_out_buf_size()
    {
        return output_buffer_size;
    }
    inline void set_out_buf_size(unsigned int bufsize)
    {
        output_buffer_size = bufsize;
    }
    inline OMX_BOOL get_pcm_feedback()
    {
        return pcm_feedback;
    }
    inline void set_pcm_feedback(OMX_BOOL flg)
    {
        pcm_feedback = flg;
    }
    inline void set_dec_state(bool dec_state)
    {
        m_dec_state = dec_state;
    }
    inline bool get_dec_state()
    {
        return m_dec_state;
    }
    inline OMX_CALLBACKTYPE get_cb()
    {
        return m_cb;
    }
    inline OMX_PTR get_app_data()
    {
        return m_app_data;
    }
    inline void set_cmdth_ctxt(struct ipc_info* ctxt)
    {
        m_ipc_to_cmd_th = ctxt;
    }
    inline struct ipc_info* get_cmdth_ctxt()
    {
        return m_ipc_to_cmd_th;
    }

    inline OMX_AUDIO_PARAM_PCMMODETYPE & get_pcm_param()
    {
        return m_pcm_param;
    }
    inline unsigned int& mflags()
    {
        return m_flags;
    }
    COmxBaseIn           *m_pIn;
    COmxBaseOut          *m_pOut;
    COmxBaseEventHandler *m_pEvent;
    COmxTimer            *m_timer;

    inline void setFirstBufSentToDSPFlg(bool flg)
    {
        m_data_written_to_dsp = flg;
    }
    inline bool getFirstBufSentToDSPFlg()
    {
        return m_data_written_to_dsp;
    }
    inline COmxTimer* getTimerInst()
    {
        if(m_timer)
        return m_timer;
        else
        return NULL;
    }
    inline bool getTimerExpiry()
    {
        return (m_timer->getTimerExpiry());
    }
    inline void setTimerExpiry()
    {
        m_timer->setTimerExpiry() ;
    }
    inline void resetTimerExpiry()
    {
        (m_timer->resetTimerExpiry());
    }
    inline OMX_U8 getTimeOut()
    {
        return (m_timer->getTimeOut());
    }
    inline omx_cmd_queue * get_cmd_q()
    {
        return &m_command_q;
    }
    inline unsigned int get_q_size(omx_cmd_queue *q)
    {
        unsigned int q_s=0;
        pthread_mutex_lock(&m_commandlock);
        q_s = q->m_size;
        pthread_mutex_unlock(&m_commandlock);
        return q_s;
    }
    inline bool get_msg(omx_cmd_queue *q,unsigned *p1, unsigned *p2,
    unsigned char *id)
    {
        bool flg = false;
        pthread_mutex_lock(&m_commandlock);
        flg = q->pop_entry(p1,p2,id);
        pthread_mutex_unlock(&m_commandlock);
        return flg;
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
    void setFilledSpaceInTcxoBuf(OMX_U32 len)
    {
        pthread_mutex_lock(&m_tcxo_lock);
        m_filledSpace = len;
        pthread_mutex_unlock(&m_tcxo_lock);
    }
    OMX_U32 getFilledSpaceInTcxoBuf()
    {
        OMX_U32 len = 0;
        pthread_mutex_lock(&m_tcxo_lock);
        len = m_filledSpace;
        pthread_mutex_unlock(&m_tcxo_lock);
        return len;
    }
private:
    //////////////////////////////////////////////////////////////////
    // Type definitions
    //////////////////////////////////////////////////////////////////
    typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*>
    input_buffer_map;
    typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*>
    output_buffer_map;
    //////////////////////////////////////////////////////////////////
    // Private definitions
    //////////////////////////////////////////////////////////////////

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

    bool execute_omx_flush(OMX_IN OMX_U32 param1, bool cmd_cmpl=true);

    bool allocate_done(void);

    bool release_done(OMX_U32 param1);

    void in_sleep();
    void in_timedsleep();
    void in_wakeup();
    void in_timedwakeup();
    void out_sleep();
    void out_wakeup();

    //////////////////////////////////////////////////////////////////
    // Member variables
    //////////////////////////////////////////////////////////////////
    OMX_U8                         m_flush_cnt ;
    OMX_U8                         m_ch_cfg;
    OMX_BOOL                       pcm_feedback;
    OMX_BOOL                       m_trigger_eos;

    bool                           is_in_th_sleep;
    bool                           is_in_th_timedsleep;
    bool                           is_out_th_sleep;
    bool                           m_flush_in_prog;
    bool                           m_pause_to_exe;
    bool                           bSuspendEventRxed;
    bool                           bResumeEventRxed;
    bool                           bOutputPortReEnabled;
    bool                           waitForSuspendCmplFlg;
    bool                           m_dec_state;
    int                            m_drv_fd;

    unsigned int                   m_sample_rate;
    unsigned int                   m_flags;
    unsigned int                   output_buffer_size;
    unsigned int                   m_input_buffer_size;
    // Num of configured buffers
    unsigned int                   m_inp_act_buf_count;
    unsigned int                   m_out_act_buf_count;
    // Num of current buffers
    unsigned int                   m_inp_current_buf_count;
    unsigned int                   m_out_current_buf_count;
    unsigned int                   nTimeStamp;
    OMX_U8                         m_eos_bm;

    volatile int                   m_is_event_done;
    volatile int                   m_is_in_th_sleep;
    volatile int                   m_is_in_th_timedsleep;
    volatile int                   m_is_out_th_sleep;

    OMX_BOOL                       m_inp_bEnabled;
    OMX_BOOL                       m_out_bEnabled;
    OMX_BOOL                       m_inp_bPopulated;
    OMX_BOOL                       m_out_bPopulated;

    input_buffer_map               m_input_buf_hdrs;
    output_buffer_map              m_output_buf_hdrs;

    omx_cmd_queue                  m_command_q;

    sem_t                          sem_read_msg;
    sem_t                          sem_write_msg;
    sem_t                          sem_States;
    sem_t                          sem_WaitForSuspendCmpl_states;
    pthread_cond_t                 cond;
    pthread_cond_t                 in_cond;
    pthread_cond_t                 in_timedcond;
    pthread_cond_t                 out_cond;

    pthread_mutexattr_t            m_state_attr;
    pthread_mutexattr_t            m_flush_attr;
    pthread_mutexattr_t            m_commandlock_attr;
    pthread_mutexattr_t            m_event_attr;
    pthread_mutexattr_t            m_in_th_attr;
    pthread_mutexattr_t            m_in_th_timedattr;
    pthread_mutexattr_t            m_out_th_attr;
    pthread_mutexattr_t            m_in_th_attr_1;
    pthread_mutexattr_t            m_in_th_timedattr_1;
    pthread_mutexattr_t            m_out_th_attr_1;
    pthread_mutexattr_t            m_suspendresume_lock_attr;
    pthread_mutexattr_t            m_tcxo_lock_attr;
    pthread_mutexattr_t            m_WaitForSuspendCmpl_lock_attr;

    pthread_mutex_t                m_state_lock;
    pthread_mutex_t                m_WaitForSuspendCmpl_lock;
    pthread_mutex_t                m_commandlock;
    pthread_mutex_t                m_flush_lock;
    pthread_mutex_t                m_event_lock;
    pthread_mutex_t                m_in_th_lock;
    pthread_mutex_t                m_in_th_timedlock;
    pthread_mutex_t                m_out_th_lock;
    pthread_mutex_t                m_in_th_lock_1;
    pthread_mutex_t                m_in_th_timedlock_1;
    pthread_mutex_t                m_out_th_lock_1;
    pthread_mutex_t                m_suspendresume_lock;
    pthread_mutex_t                m_tcxo_lock;

    OMX_PTR                        m_app_data;
    OMX_S32                        m_volume;
    OMX_STATETYPE                  m_state;
    OMX_CALLBACKTYPE               m_cb;

    struct ipc_info                *m_ipc_to_cmd_th;

    OMX_PRIORITYMGMTTYPE           m_priority_mgm ;
    OMX_PARAM_BUFFERSUPPLIERTYPE   m_buffer_supplier;
    OMX_PARAM_COMPONENTROLETYPE    component_Role;
    OMX_SUSPENSIONPOLICYTYPE       suspensionPolicy;
    OMX_AUDIO_PARAM_PCMMODETYPE    m_pcm_param;

    OMX_U8                         m_comp_deinit;
    bool                           m_data_written_to_dsp;
    unsigned short                 m_session_id;
    char                           m_devName[512];
    OMX_U32                        m_filledSpace;
};

class COmxBaseEventHandler
{
public:
    COmxBaseEventHandler(COmxBase * base, int drv_fd,
    OMX_BOOL suspensionPolicy,
    OMX_STATETYPE state);

    virtual ~COmxBaseEventHandler();

    inline void set_default_param(unsigned int drv_fd,
    OMX_BOOL suspensionPolicy,
    OMX_STATETYPE state)
    {
        m_drv_fd = drv_fd;
        m_suspensionPolicy = suspensionPolicy;
        m_state = state;
        return;
    }
    inline int get_drv_fd()
    {
        return m_drv_fd;
    }
    inline void set_state(OMX_STATETYPE state)
    {
        pthread_mutex_lock(&m_state_lock);
        m_state = state;
        pthread_mutex_unlock(&m_state_lock);
    }
    inline OMX_STATETYPE get_state()
    {
        OMX_STATETYPE state;
        pthread_mutex_lock(&m_state_lock);
        state = m_state;
        pthread_mutex_unlock(&m_state_lock);
        return state;
    }
    inline OMX_BOOL  getSuspensionPolicy()
    {
        return m_suspensionPolicy;
    }
    virtual OMX_ERRORTYPE processEvents();

    inline void set_eventh_ctxt(struct ipc_info* ctxt)
    {
        m_ipc_to_event_th = ctxt;
    }
    inline struct ipc_info* get_eventh_ctxt()
    {
        return m_ipc_to_event_th;
    }
    inline COmxBase* get_comp()
    {
        return m_comp;
    }
private:
    int                  m_drv_fd;
    OMX_BOOL             m_suspensionPolicy;

    OMX_STATETYPE        m_state;

    COmxBase             *m_comp;
    COmxBaseEventHandler *m_this;

    struct ipc_info      *m_ipc_to_event_th;
    pthread_mutex_t      m_state_lock;
    pthread_mutexattr_t  m_state_attr;
};

#endif // _OMX_BASE_H_
