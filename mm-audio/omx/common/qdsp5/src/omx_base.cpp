/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_base.cpp
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

#include "omx_base.h"

using namespace std;

void omx_cmd_msg(void *client_data, unsigned char id)
{
    COmxBase *pThis = (COmxBase*)client_data;
    DEBUG_DETAIL("%s id=%d pThis=%p\n",__FUNCTION__,id,pThis);
    pThis->process_command_msg(id);
    return ;
}

void omx_in_msg(void *client_data, unsigned char id)
{
    COmxBaseIn *pThis = (COmxBaseIn*)client_data;
    DEBUG_DETAIL("%s id=%d pThis=%p\n",__FUNCTION__,id,pThis);
    pThis->process_in_port_msg(id);
    return ;

}

void omx_out_msg(void *client_data, unsigned char id)
{
    COmxBaseOut *pThis = (COmxBaseOut*)client_data;
    DEBUG_DETAIL("%s id=%d pThis=%p\n",__FUNCTION__,id,pThis);
    pThis->process_out_port_msg(id);
    return ;
}

void omx_event_msg(void *client_data, unsigned char id)
{
    COmxBaseEventHandler *pThis = (COmxBaseEventHandler*)client_data;
    DEBUG_DETAIL("%s id=%d pThis=%p\n",__FUNCTION__,id,pThis);
    pThis->processEvents();
    return ;
}

omx_cmd_queue::omx_cmd_queue(): m_read(0),m_write(0),m_size(0)
{
    memset(m_q,      0,sizeof(omx_event)*OMX_CORE_CONTROL_CMDQ_SIZE);
}

bool omx_cmd_queue::insert_entry(unsigned p1, unsigned p2,
unsigned char id)
{
    bool ret = true;
    if(m_size < OMX_CORE_CONTROL_CMDQ_SIZE)
    {
        m_q[m_write].id       = id;
        m_q[m_write].param1   = p1;
        m_q[m_write].param2   = p2;
        m_write++;
        m_size ++;
        if(m_write >= OMX_CORE_CONTROL_CMDQ_SIZE)
        {
            m_write = 0;
        }
    }
    else
    {
        ret = false;
        DEBUG_PRINT_ERROR("Command Queue Full\n");
    }
    return ret;
}

// omx cmd queue delete
bool omx_cmd_queue::pop_entry(unsigned *p1, unsigned *p2,
unsigned char *id)
{
    bool ret = true;
    if (m_size > 0)
    {
        *id = m_q[m_read].id;
        *p1 = m_q[m_read].param1;
        *p2 = m_q[m_read].param2;
        // Move the read pointer ahead
        ++m_read;
        --m_size;
        if(m_read >= OMX_CORE_CONTROL_CMDQ_SIZE)
        {
            m_read = 0;

        }
    }
    else
    {
        ret = false;
        DEBUG_PRINT_ERROR("Command Queue Empty");
    }
    return ret;
}

bool omx_cmd_queue::get_msg_id(unsigned char *id)
{
    if(m_size > 0)
    {
        *id = m_q[m_read].id;
        DEBUG_PRINT("get_msg_id=%d\n",*id);
    }
    else{
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// COmxBase
//////////////////////////////////////////////////////////////////////////////

void COmxBase::wait_for_event()
{
    pthread_mutex_lock(&m_event_lock);
    while(m_is_event_done == 0)
    {
        pthread_cond_wait(&cond, &m_event_lock);
    }
    m_is_event_done = 0;
    pthread_mutex_unlock(&m_event_lock);
}

void COmxBase::event_complete()
{
    pthread_mutex_lock(&m_event_lock);
    if(m_is_event_done == 0) {
        m_is_event_done = 1;
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&m_event_lock);
}

void COmxBase::in_th_sleep()
{
    DEBUG_DETAIL("COmxBase::%s\n",__FUNCTION__);
    pthread_mutex_lock(&m_in_th_lock_1);
    is_in_th_sleep = true;
    pthread_mutex_unlock(&m_in_th_lock_1);
    in_sleep();
}

void COmxBase::in_th_timedsleep()
{
    int rc = 0;
    DEBUG_DETAIL("COmxBase::%s\n",__FUNCTION__);
    pthread_mutex_lock(&m_in_th_timedlock_1);
    is_in_th_timedsleep = true;
    pthread_mutex_unlock(&m_in_th_timedlock_1);
    in_timedsleep();
}



void COmxBase::in_th_wakeup()
{
    pthread_mutex_lock(&m_in_th_lock_1);
    if(is_in_th_sleep)
    {
        DEBUG_DETAIL("COmxBase::%s\n",__FUNCTION__);
        is_in_th_sleep = false;
        in_wakeup();
    }
    pthread_mutex_unlock(&m_in_th_lock_1);
}

void COmxBase::in_th_timedwakeup()
{
    pthread_mutex_lock(&m_in_th_timedlock_1);
    if(is_in_th_timedsleep)
    {
        DEBUG_DETAIL("COmxBase::%s\n",__FUNCTION__);
        is_in_th_timedsleep = false;
        in_timedwakeup();
    }
    pthread_mutex_unlock(&m_in_th_timedlock_1);
}

void COmxBase::out_th_sleep()
{
    DEBUG_DETAIL("COmxBase::%s\n",__FUNCTION__);
    pthread_mutex_lock(&m_out_th_lock_1);
    is_out_th_sleep = true;
    pthread_mutex_unlock(&m_out_th_lock_1);
    out_sleep();
}
void COmxBase::out_th_wakeup()
{
    pthread_mutex_lock(&m_out_th_lock_1);
    if(is_out_th_sleep)
    {
        DEBUG_DETAIL("COmxBase::%s\n",__FUNCTION__);
        is_out_th_sleep = false;
        out_wakeup();
    }
    pthread_mutex_unlock(&m_out_th_lock_1);
}

void COmxBase::in_sleep()
{
    pthread_mutex_lock(&m_in_th_lock);
    while (m_is_in_th_sleep == 0)
    {
        pthread_cond_wait(&in_cond, &m_in_th_lock);
    }
    m_is_in_th_sleep = 0;
    pthread_mutex_unlock(&m_in_th_lock);
}
void COmxBase::in_timedsleep()
{
    struct timespec   ts;
    DEBUG_DETAIL("COmxBase::%s\n",__FUNCTION__);
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    pthread_mutex_lock(&m_in_th_timedlock);
    if(m_is_in_th_timedsleep == 0)
    {
        pthread_cond_timedwait(&in_timedcond, &m_in_th_timedlock,&ts);
    }
    m_is_in_th_timedsleep = 0;
    pthread_mutex_unlock(&m_in_th_timedlock);
}

void COmxBase::in_wakeup()
{
    pthread_mutex_lock(&m_in_th_lock);
    if (m_is_in_th_sleep == 0) {
        m_is_in_th_sleep = 1;
        pthread_cond_signal(&in_cond);
    }
    pthread_mutex_unlock(&m_in_th_lock);
}

void COmxBase::in_timedwakeup()
{
    pthread_mutex_lock(&m_in_th_timedlock);
    if (m_is_in_th_timedsleep == 0) {
        m_is_in_th_timedsleep = 1;
        pthread_cond_signal(&in_timedcond);
    }
    pthread_mutex_unlock(&m_in_th_timedlock);
}

void COmxBase::out_sleep()
{
    pthread_mutex_lock(&m_out_th_lock);
    while (m_is_out_th_sleep == 0)
    {
        pthread_cond_wait(&out_cond, &m_out_th_lock);
    }
    m_is_out_th_sleep = 0;
    pthread_mutex_unlock(&m_out_th_lock);
}

void COmxBase::out_wakeup()
{
    pthread_mutex_lock(&m_out_th_lock);
    if (m_is_out_th_sleep == 0) {
        m_is_out_th_sleep = 1;
        pthread_cond_signal(&out_cond);
    }
    pthread_mutex_unlock(&m_out_th_lock);
}


COmxBase::COmxBase(const char *devName,OMX_U32 sf, OMX_U8 ch):
m_timer(NULL),
m_flush_cnt(255),
m_ch_cfg(ch),
pcm_feedback(OMX_FALSE),
m_trigger_eos(OMX_FALSE),
is_in_th_sleep(false),
is_in_th_timedsleep(false),
is_out_th_sleep(false),
m_flush_in_prog(false),
m_pause_to_exe(false),
bSuspendEventRxed(false),
bResumeEventRxed(false),
bOutputPortReEnabled(false),
m_drv_fd(-1),
m_sample_rate(sf),
m_flags(0),
output_buffer_size(0),
m_input_buffer_size(0),
m_inp_act_buf_count(OMX_CORE_NUM_INPUT_BUFFERS),
m_out_act_buf_count (OMX_CORE_NUM_OUTPUT_BUFFERS),
m_inp_current_buf_count(0),
m_out_current_buf_count(0),
nTimeStamp(0),
m_eos_bm(0),
m_is_event_done(0),
m_is_in_th_sleep(0),
m_is_in_th_timedsleep(0),
m_is_out_th_sleep(0),
m_inp_bEnabled(OMX_TRUE),
m_out_bEnabled(OMX_TRUE),
m_inp_bPopulated(OMX_FALSE),
m_out_bPopulated(OMX_FALSE),
m_app_data(0),
m_volume(OMX_ADEC_DEFAULT_VOL),
m_state(OMX_StateLoaded),
m_ipc_to_cmd_th(NULL),
m_session_id(0),
waitForSuspendCmplFlg(false),
m_dec_state(false),
m_filledSpace(0)

{
    //TBD::output_buffer_size and m_input_buffer_size not INIT
    DEBUG_PRINT("COmxBase::%s\n",__FUNCTION__);
    int cond_ret = 0;
    memset(&m_cb,        0,      sizeof(m_cb));
    memset(&m_cmp,       0,     sizeof(m_cmp));

    strncpy(m_devName,devName,strlen(devName));
    m_devName[strlen(devName)] = '\0';
    DEBUG_PRINT("m_devName[%s],devName[%s]\n",m_devName,devName);
    pthread_mutexattr_init(&m_state_attr);
    pthread_mutex_init(&m_state_lock, &m_state_attr);

    pthread_mutexattr_init(&m_commandlock_attr);
    pthread_mutex_init(&m_commandlock, &m_commandlock_attr);

    pthread_mutexattr_init(&m_event_attr);
    pthread_mutex_init(&m_event_lock, &m_event_attr);

    pthread_mutexattr_init(&m_flush_attr);
    pthread_mutex_init(&m_flush_lock, &m_flush_attr);

    pthread_mutexattr_init(&m_in_th_attr);
    pthread_mutex_init(&m_in_th_lock, &m_in_th_attr);

    pthread_mutexattr_init(&m_in_th_timedattr);
    pthread_mutex_init(&m_in_th_timedlock, &m_in_th_timedattr);

    pthread_mutexattr_init(&m_out_th_attr);
    pthread_mutex_init(&m_out_th_lock, &m_out_th_attr);

    pthread_mutexattr_init(&m_in_th_attr_1);
    pthread_mutex_init(&m_in_th_lock_1, &m_in_th_attr_1);

    pthread_mutexattr_init(&m_in_th_timedattr_1);
    pthread_mutex_init(&m_in_th_timedlock_1, &m_in_th_timedattr_1);

    pthread_mutexattr_init(&m_out_th_attr_1);
    pthread_mutex_init(&m_out_th_lock_1, &m_out_th_attr_1);

    pthread_mutexattr_init(&m_suspendresume_lock_attr);
    pthread_mutex_init(&m_suspendresume_lock, &m_suspendresume_lock_attr);

    pthread_mutexattr_init(&m_tcxo_lock_attr);
    pthread_mutex_init(&m_tcxo_lock, &m_tcxo_lock_attr);

    pthread_mutexattr_init(&m_WaitForSuspendCmpl_lock_attr);
    pthread_mutex_init(&m_WaitForSuspendCmpl_lock, &m_WaitForSuspendCmpl_lock_attr);

    if ((cond_ret = pthread_cond_init (&cond, NULL)) != 0)
    {
        DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for cond\n");
        if (cond_ret == EAGAIN)
        DEBUG_PRINT_ERROR("system lacked necessary resources(other than mem)\n");
        else if (cond_ret == ENOMEM)
        DEBUG_PRINT_ERROR("Insufficient memory to initcondition variable\n");
    }
    if ((cond_ret = pthread_cond_init (&in_cond, NULL))!= 0)
    {
        DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for in_cond\n");
        if (cond_ret == EAGAIN)
        DEBUG_PRINT_ERROR("system lacked necessary resources(other than mem)\n");
        else if (cond_ret == ENOMEM)
        DEBUG_PRINT_ERROR("Insufficient memory to init condition variable\n");
    }
    if ((cond_ret = pthread_cond_init (&out_cond, NULL))!= 0)
    {
        DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for out_cond\n");
        if (cond_ret == EAGAIN)
        DEBUG_PRINT_ERROR("system lacked necessary resources(other than mem)\n");
        else if (cond_ret == ENOMEM)
        DEBUG_PRINT_ERROR("Insufficient memory to init condition variable\n");
    }
    if ((cond_ret = pthread_cond_init (&in_timedcond, NULL))!= 0)
    {
        DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for in_timedcond\n");
        if (cond_ret == EAGAIN)
        DEBUG_PRINT_ERROR("system lacked necessary resources(other than mem)\n");
        else if (cond_ret == ENOMEM)
        DEBUG_PRINT_ERROR("Insufficient memory to init condition variable\n");
    }

    sem_init(&sem_read_msg,0, 0);
    sem_init(&sem_write_msg,0, 0);
    sem_init(&sem_States,0, 0);
    sem_init(&sem_WaitForSuspendCmpl_states,0, 0);
}

COmxBase::~COmxBase()
{
    DEBUG_DETAIL("COmxBase::%s\n",__FUNCTION__);
    if ( !m_comp_deinit && (m_drv_fd != -1) )
    {
        deinit();
    }
    pthread_mutexattr_destroy(&m_suspendresume_lock_attr);
    pthread_mutexattr_destroy(&m_commandlock_attr);
    pthread_mutexattr_destroy(&m_state_attr);
    pthread_mutexattr_destroy(&m_flush_attr);
    pthread_mutexattr_destroy(&m_in_th_attr_1);
    pthread_mutexattr_destroy(&m_in_th_timedattr_1);
    pthread_mutexattr_destroy(&m_out_th_attr_1);
    pthread_mutexattr_destroy(&m_event_attr);
    pthread_mutexattr_destroy(&m_in_th_attr);
    pthread_mutexattr_destroy(&m_in_th_timedattr);
    pthread_mutexattr_destroy(&m_out_th_attr);
    pthread_mutexattr_destroy(&m_WaitForSuspendCmpl_lock_attr);
    pthread_mutexattr_destroy(&m_tcxo_lock_attr);

    pthread_mutex_destroy(&m_WaitForSuspendCmpl_lock);
    pthread_mutex_destroy(&m_tcxo_lock);
    pthread_mutex_destroy(&m_state_lock);
    pthread_mutex_destroy(&m_commandlock);
    pthread_mutex_destroy(&m_flush_lock);
    pthread_mutex_destroy(&m_event_lock);
    pthread_mutex_destroy(&m_in_th_lock);
    pthread_mutex_destroy(&m_in_th_timedlock);
    pthread_mutex_destroy(&m_out_th_lock);
    pthread_mutex_destroy(&m_in_th_lock_1);
    pthread_mutex_destroy(&m_in_th_timedlock_1);
    pthread_mutex_destroy(&m_out_th_lock_1);
    pthread_mutex_destroy(&m_suspendresume_lock);

    pthread_cond_destroy(&cond);
    pthread_cond_destroy(&in_cond);
    pthread_cond_destroy(&in_timedcond);
    pthread_cond_destroy(&out_cond);

    sem_destroy (&sem_read_msg);
    sem_destroy (&sem_write_msg);
    sem_destroy (&sem_States);
    sem_destroy (&sem_WaitForSuspendCmpl_states);
    DEBUG_PRINT_ERROR("OMX AAC component destroyed\n");
    return;
}

OMX_ERRORTYPE COmxBase::component_init(OMX_STRING role)
{
    DEBUG_PRINT("COmxBase::%s role[%s] devName[%s]\n",__FUNCTION__,role,m_devName);
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    m_state            = OMX_StateLoaded;

    m_volume = OMX_ADEC_DEFAULT_VOL; /* Close to unity gain */
    suspensionPolicy= OMX_SuspensionDisabled;

    m_ipc_to_cmd_th = NULL;
    m_is_out_th_sleep = 0;
    m_is_in_th_sleep = 0;
    is_out_th_sleep= false;
    is_in_th_sleep=false;
    bOutputPortReEnabled = false;

    m_pcm_param.nPortIndex = OMX_CORE_OUTPUT_PORT_INDEX;
    m_pcm_param.nChannels = m_ch_cfg;
    m_pcm_param.eNumData = OMX_NumericalDataSigned;
    m_pcm_param.bInterleaved = OMX_TRUE;
    m_pcm_param.nBitPerSample = 16;
    m_pcm_param.nSamplingRate = m_sample_rate;
    m_pcm_param.ePCMMode = OMX_AUDIO_PCMModeLinear;
    m_pcm_param.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
    m_pcm_param.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

    memset(&m_priority_mgm, 0, sizeof(m_priority_mgm));
    m_priority_mgm.nGroupID =0;
    m_priority_mgm.nGroupPriority=0;
    memset(&m_buffer_supplier, 0, sizeof(m_buffer_supplier));
    m_buffer_supplier.nPortIndex=OMX_BufferSupplyUnspecified;


    if(!strcmp(role,"OMX.qcom.audio.decoder.aac"))
    {   m_drv_fd = open(m_devName,O_RDWR);
        pcm_feedback = OMX_TRUE;
        DEBUG_PRINT("\nCOmxBase::component_init: role[%s]fd[%d]dev[%s]\n",role,m_drv_fd,m_devName);
    }
    else if(!strcmp(role,"OMX.qcom.audio.decoder.tunneled.aac"))
    {
        m_drv_fd = open(m_devName,O_WRONLY);
        pcm_feedback = OMX_FALSE;
        DEBUG_PRINT("\nCOmxBase::component_init: role[%s]fd[%d]dev[%s]\n",role,m_drv_fd,m_devName);
    }
    else if(!strcmp(role,"OMX.qcom.audio.decoder.ac3") || !strcmp(role,"OMX.qcom.audio.decoder.eac3"))
    {
        m_drv_fd = open(m_devName,O_RDWR);
        pcm_feedback = OMX_TRUE;
        DEBUG_PRINT("\nCOmxBase::component_init: role[%s]fd[%d]dev[%s]\n",role,m_drv_fd,m_devName);
    }
    else
    {
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED is invalid\n", role);
    }
    if (m_drv_fd < 0)
    {
        DEBUG_PRINT_ERROR("Dev Open Failed[%d] errno[%d]",\
        m_drv_fd,errno);
        return OMX_ErrorInsufficientResources;
    }

    if(ioctl(m_drv_fd, AUDIO_GET_SESSION_ID,&m_session_id) == -1)
    {
        DEBUG_PRINT("AUDIO_GET_SESSION_ID FAILED\n");
    }

    return eRet;
}

OMX_ERRORTYPE COmxBase::component_deinit(
OMX_HANDLETYPE hComp)
{
    DEBUG_PRINT("COmxBase::%s \n",__FUNCTION__);
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(m_drv_fd != -1)
        eRet = deinit();
    DEBUG_PRINT_ERROR("%s deinit completed\n",__FUNCTION__);
    return eRet;
}

OMX_ERRORTYPE COmxBase::deinit()
{
    DEBUG_DETAIL("COmxBase::%s \n",__FUNCTION__);
    if(get_state() != OMX_StateLoaded || ((get_state() == OMX_StateLoaded) &&
                            BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)))
    {
        DEBUG_PRINT_ERROR("%s,Deinit called in state[%d]\n",__FUNCTION__,\
	                                                             get_state());
        if(get_state() != OMX_StateInvalid)
            set_state(OMX_StateLoaded);
	// Get back any buffers from driver
        if(!BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
	    execute_omx_flush(-1,false);
	// force state change to loaded so that all threads can be exited
	DEBUG_PRINT_ERROR("Freeing Buf:inp_current_buf_count[%d][%d]\n",\
	                                                 m_inp_current_buf_count,
	                                                 m_input_buf_hdrs.size());
	m_input_buf_hdrs.eraseall();
	DEBUG_PRINT_ERROR("Freeing Buf:out_current_buf_count[%d][%d]\n",\
	                                                 m_out_current_buf_count,
	                                                 m_output_buf_hdrs.size());
	m_output_buf_hdrs.eraseall();
    }

    DEBUG_DETAIL("WAKE UP IN TH\n");
    in_th_wakeup();
    DEBUG_DETAIL("WAKE UP OUT TH\n");
    out_th_wakeup();

    if(m_timer)
    {
        delete m_timer;
        m_timer = NULL;
    }
    if(m_pIn)
    {
    delete m_pIn;
        m_pIn = NULL;
    }
    if(m_pOut)
    {
    delete m_pOut;
        m_pOut = NULL;
    }
    if(m_pEvent)
    {
    delete m_pEvent;
        m_pEvent = NULL;
    }


    bOutputPortReEnabled = false;

    if ( ioctl(m_drv_fd, AUDIO_STOP, 0) == -1 )
    {
        DEBUG_PRINT("deinit:AUDIO_STOP failed %d\n", errno);
    }
    DEBUG_DETAIL("deinit calling m_ipc_to_cmd_th\n");
    if (m_ipc_to_cmd_th != NULL) {
        omx_thread_stop(m_ipc_to_cmd_th);
        m_ipc_to_cmd_th = NULL;
    }

    DEBUG_DETAIL("deinit calling omx_aac_event_stop\n");

    m_filledSpace = 0;
    m_out_act_buf_count = 0;
    m_inp_act_buf_count = 0;
    m_flush_in_prog = false;
    resetSuspendFlg();
    resetResumeFlg();

    m_eos_bm=false;
    m_pause_to_exe=false;

    m_sample_rate = 0;
    m_ch_cfg = 0;

    if (m_drv_fd >= 0) {
        close(m_drv_fd);
        m_drv_fd = -1;
    }
    else
    {
        DEBUG_PRINT(" aac device already closed \n");
    }
    m_comp_deinit=1;
    m_is_out_th_sleep = 1;
    m_is_in_th_sleep = 1;

    DEBUG_PRINT("************************************\n");
    DEBUG_PRINT_ERROR(" DEINIT COMPLETED");
    DEBUG_PRINT("************************************\n");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE  COmxBase::allocate_buffer(
OMX_IN OMX_HANDLETYPE            hComp,
OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
OMX_IN OMX_U32                   port,
OMX_IN OMX_PTR                   appData,
OMX_IN OMX_U32                   bytes)
{
    DEBUG_DETAIL("COmxBase::%s \n",__FUNCTION__);
    OMX_ERRORTYPE eRet = OMX_ErrorNone; // OMX return type

    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Allocate Buf in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if(hComp == NULL)
    {
        port = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }

    if(port == OMX_CORE_INPUT_PORT_INDEX)
    {
        eRet = allocate_input_buffer(hComp,bufferHdr,port,appData,bytes);
    }
    else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    {
        eRet = allocate_output_buffer(hComp,bufferHdr,port,appData,bytes);
    }
    else
    {
        DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",
        (int)port);
        eRet = OMX_ErrorBadPortIndex;
    }
    if((eRet == OMX_ErrorNone))
    {
        if(allocate_done())
        {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
            {
                DEBUG_DETAIL("AB-->Send IDLE transition complete...\n");
                // Send the callback now
                BITMASK_CLEAR(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                post_command(OMX_CommandStateSet,OMX_StateIdle,
                OMX_COMPONENT_GENERATE_EVENT);
            }
        }

        if((port == OMX_CORE_INPUT_PORT_INDEX) && m_inp_bPopulated)
        {
            if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING))
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
                DEBUG_DETAIL("AB-->Send Input Port Enable complete complete.\n");

                post_command(OMX_CommandPortEnable, OMX_CORE_INPUT_PORT_INDEX,
                OMX_COMPONENT_GENERATE_EVENT);
            }
        }
        if((port == OMX_CORE_OUTPUT_PORT_INDEX) && m_out_bPopulated)
        {
            if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING))
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                DEBUG_DETAIL("AB-->Send Output Port Enable complete complete\n");
                m_out_bEnabled = OMX_TRUE;
                bOutputPortReEnabled = 1;

                DEBUG_DETAIL("AllocBuf-->in_th_sleep=%d out_th_sleep=%d\n",\
                is_in_th_sleep,is_out_th_sleep);
                out_th_wakeup();
                in_th_wakeup();
                post_command(OMX_CommandPortEnable,OMX_CORE_OUTPUT_PORT_INDEX,
                OMX_COMPONENT_GENERATE_EVENT);
            }
        }
    }
    DEBUG_PRINT("Allocate Buffer exit with ret Code[%d] port[%u]\n", eRet,\
                                                         (unsigned int)port);
    return eRet;
}

/* ======================================================================
FUNCTION
omx_aac_adec::AllocateInputBuffer

DESCRIPTION
Helper function for allocate buffer in the input pin

PARAMETERS
None.

RETURN VALUE
true/false

========================================================================== */
OMX_ERRORTYPE  COmxBase::allocate_input_buffer(
OMX_IN OMX_HANDLETYPE                hComp,
OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
OMX_IN OMX_U32                        port,
OMX_IN OMX_PTR                     appData,
OMX_IN OMX_U32                       bytes)
{
    DEBUG_PRINT("COmxBase::%s\n",__FUNCTION__);
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE            *bufHdr;
    unsigned                   nBufSize = MAX(bytes, m_input_buffer_size);
    char                       *buf_ptr;
    // allocate buffer header and assign pBuffer in COmxBaseDec
    //offset 4 for pseudo raw
    if(m_inp_current_buf_count < m_inp_act_buf_count)
    {
        bufHdr = *bufferHdr;
        DEBUG_PRINT("::%s bufHdr[%p]pBuffer[%p]m_inp_buf_cnt[%u]bytes[%u]",
        __FUNCTION__,bufHdr, bufHdr->pBuffer,
        (unsigned int)m_inp_current_buf_count,
        (unsigned int)bytes);
        bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
        bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
        bufHdr->nAllocLen         = nBufSize;
        bufHdr->pAppPrivate       = appData;
        bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
        m_input_buf_hdrs.insert(bufHdr, NULL);
        m_inp_current_buf_count++;
    }
    else
    {
        DEBUG_PRINT("Input buffer memory allocation failed 2\n");
        eRet =  OMX_ErrorInsufficientResources;
    }

    return eRet;
}

OMX_ERRORTYPE  COmxBase::allocate_output_buffer(
OMX_IN OMX_HANDLETYPE                hComp,
OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
OMX_IN OMX_U32                        port,
OMX_IN OMX_PTR                     appData,
OMX_IN OMX_U32                       bytes)
{
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;

    OMX_BUFFERHEADERTYPE            *bufHdr;
    unsigned                   nBufSize = MAX(bytes,output_buffer_size);
    char                       *buf_ptr;
    DEBUG_PRINT("COmxBase::%s\n",__FUNCTION__);
    if(m_out_current_buf_count < m_out_act_buf_count)
    {
        bufHdr = *bufferHdr;

        DEBUG_PRINT("::%s bufHdr[%p]pBuffer[%p]m_out_buf_cnt[%u]bytes[%u]",
        __FUNCTION__, bufHdr,bufHdr->pBuffer,
        (unsigned int)m_out_current_buf_count,
        (unsigned int) bytes);
        bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
        bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
        bufHdr->nAllocLen         = nBufSize;
        bufHdr->pAppPrivate       = appData;
        bufHdr->nOutputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX;
        m_output_buf_hdrs.insert(bufHdr, NULL);
        m_out_current_buf_count++;
    }
    else
    {
        DEBUG_PRINT("Allocate_output:buffer memory allocation failed\n");
        eRet =  OMX_ErrorInsufficientResources;
    }
    return eRet;
}

/* ======================================================================
FUNCTION
omx_aac_adec::UseBuffer

DESCRIPTION
OMX Use Buffer method implementation.

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None , if everything successful.

========================================================================== */
OMX_ERRORTYPE  COmxBase::use_buffer(
OMX_IN OMX_HANDLETYPE            hComp,
OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
OMX_IN OMX_U32                   port,
OMX_IN OMX_PTR                   appData,
OMX_IN OMX_U32                   bytes,
OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    DEBUG_PRINT("ComxBase::%s\n",__FUNCTION__);
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Allocate Buf in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if(hComp == NULL)
    {
        port = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if(port == OMX_CORE_INPUT_PORT_INDEX)
    {
        eRet = use_input_buffer(hComp,bufferHdr,port,appData,bytes,buffer);
    }
    else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    {
        eRet = use_output_buffer(hComp,bufferHdr,port,appData,bytes,buffer);
    }
    else
    {
        DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",(int)port);
        eRet = OMX_ErrorBadPortIndex;
    }

    if((eRet == OMX_ErrorNone))
    {
        if(allocate_done())
        {
            if (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
            {
                BITMASK_CLEAR(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                post_command(OMX_CommandStateSet,OMX_StateIdle,
                OMX_COMPONENT_GENERATE_EVENT);
            }
        }

        if((port == OMX_CORE_INPUT_PORT_INDEX) && m_inp_bPopulated)
        {
            if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING))
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);

                post_command(OMX_CommandPortEnable, OMX_CORE_INPUT_PORT_INDEX,
                OMX_COMPONENT_GENERATE_EVENT);

            }
        }
        if((port == OMX_CORE_OUTPUT_PORT_INDEX) && m_out_bPopulated)
        {
            if(BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING))
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);

                post_command(OMX_CommandPortEnable, OMX_CORE_OUTPUT_PORT_INDEX,
                OMX_COMPONENT_GENERATE_EVENT);
                bOutputPortReEnabled = 1;
                DEBUG_DETAIL("WAKEUP BOTH IN AND OUT TH IF SLEEPING...%d %d\n",\
                is_in_th_sleep,
                is_out_th_sleep);
                out_th_wakeup();
                in_th_wakeup();
            }
        }
    }
    DEBUG_PRINT("::%s port[%u]eRet[%u]\n",__FUNCTION__,(unsigned int)port,eRet);

    return eRet;
}
/* ======================================================================
FUNCTION
omx_aac_adec::UseInputBuffer

DESCRIPTION
Helper function for Use buffer in the input pin

PARAMETERS
None.

RETURN VALUE
true/false

========================================================================== */
OMX_ERRORTYPE  COmxBase::use_input_buffer(
OMX_IN OMX_HANDLETYPE            hComp,
OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
OMX_IN OMX_U32                   port,
OMX_IN OMX_PTR                   appData,
OMX_IN OMX_U32                   bytes,
OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE            *bufHdr;
    unsigned                   nBufSize = MAX(bytes, m_input_buffer_size);
    char                       *buf_ptr;

    if(hComp == NULL)
    {
        port = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    DEBUG_PRINT("::%s\n",__FUNCTION__);

    if(bytes < m_input_buffer_size)
    {
        /* return if i\p buffer size provided by client
    is less than min i\p buffer size supported by omx component*/
        return OMX_ErrorInsufficientResources;
    }
    if(m_inp_current_buf_count < m_inp_act_buf_count)
    {

        buf_ptr = (char *) calloc(sizeof(OMX_BUFFERHEADERTYPE), 1);

        if (buf_ptr != NULL)
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));
            bufHdr->pBuffer           = (OMX_U8 *)(buffer);
            DEBUG_PRINT("::%s bufHdr[%p]pBuffer[%p]bytes[%u]",\
            __FUNCTION__,
            bufHdr, bufHdr->pBuffer,(unsigned int)bytes);

            bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            m_input_buffer_size         = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
            bufHdr->nOffset           = 0;
            m_input_buf_hdrs.insert(bufHdr, NULL);
            m_inp_current_buf_count++;
        }
        else
        {
            DEBUG_PRINT("use_input_buffer:buffer memory allocation failed\n");
            eRet =  OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG_PRINT("Input buffer memory allocation failed 2\n");
        eRet =  OMX_ErrorInsufficientResources;
    }
    return eRet;
}

/* ======================================================================
FUNCTION
omx_aac_adec::UseOutputBuffer

DESCRIPTION
Helper function for Use buffer in the input pin

PARAMETERS
None.

RETURN VALUE
true/false

========================================================================== */
OMX_ERRORTYPE  COmxBase::use_output_buffer(
OMX_IN OMX_HANDLETYPE            hComp,
OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
OMX_IN OMX_U32                   port,
OMX_IN OMX_PTR                   appData,
OMX_IN OMX_U32                   bytes,
OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE            *bufHdr;
    unsigned                   nBufSize = MAX(bytes,output_buffer_size);
    char                       *buf_ptr;

    DEBUG_PRINT("COmxBase::%s\n",__FUNCTION__);
    if(hComp == NULL)
    {
        port = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }

    if(bytes < output_buffer_size)
    {
        /* return if o\p buffer size provided by client
    is less than min o\p buffer size supported by omx component*/
        return OMX_ErrorInsufficientResources;
    }
    if(m_out_current_buf_count < m_out_act_buf_count)
    {
        buf_ptr = (char *) calloc(sizeof(OMX_BUFFERHEADERTYPE), 1);

        if (buf_ptr != NULL)
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            DEBUG_PRINT("BufHdr=%p buffer=%p\n",bufHdr,buffer);
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

            bufHdr->pBuffer           = (OMX_U8 *)(buffer);
            DEBUG_PRINT("::%s bufHdr[%p]pBuffer[%p]len[%u]",
            __FUNCTION__,
            bufHdr, bufHdr->pBuffer,(unsigned int)bytes);
            bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            output_buffer_size        = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nOutputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX;
            bufHdr->nOffset           = 0;
            m_output_buf_hdrs.insert(bufHdr, NULL);
            m_out_current_buf_count++;
        }
        else
        {
            DEBUG_PRINT("use_output_buffer:buf memory allocation failed\n");
            eRet =  OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG_PRINT("Output buffer memory allocation failed 2\n");
        eRet =  OMX_ErrorInsufficientResources;
    }
    return eRet;
}


bool COmxBase::search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{
    bool eRet = false;
    OMX_BUFFERHEADERTYPE *temp = NULL;

    temp = m_input_buf_hdrs.find_ele(buffer);
    if(buffer && temp)
    {
        eRet = true;
    }
    return eRet;
}


bool COmxBase::search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{
    bool eRet = false;
    OMX_BUFFERHEADERTYPE *temp = NULL;

    temp = m_output_buf_hdrs.find_ele(buffer);
    if(buffer && temp)
    {
        eRet = true;
    }
    return eRet;
}

OMX_ERRORTYPE  COmxBase::free_buffer(OMX_IN OMX_HANDLETYPE         hComp,
OMX_IN OMX_U32                 port,
OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT("COmxBase::%s Free_Buffer port[%u]buffer[%p] \n",\
    __FUNCTION__,(unsigned int)port,buffer);

    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (m_state == OMX_StateIdle &&
            (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
    {
        DEBUG_PRINT(" free buffer while Component in Loading pending\n");
    }
    else if(((m_inp_bEnabled == OMX_FALSE) &&
                                      (port == OMX_CORE_INPUT_PORT_INDEX))||
            ((m_out_bEnabled == OMX_FALSE) &&
                                    (port == OMX_CORE_OUTPUT_PORT_INDEX)))
    {
        DEBUG_PRINT("Free Buffer while port[%d] disabled\n",(unsigned int)port);
    }
    else if((m_state == OMX_StateExecuting) || (m_state == OMX_StatePause))
    {
        DEBUG_PRINT("Invalid state to free buffer,ports need to be disabled:\
                    OMX_ErrorPortUnpopulated\n");
        m_cb.EventHandler(&m_cmp,
        m_app_data,
        OMX_EventError,
        OMX_ErrorPortUnpopulated,
        NULL,
        NULL );

        return eRet;
    }
    else
    {
        DEBUG_PRINT("free_buffer: Invalid state to free buffer,ports need to be\
                    disabled:OMX_ErrorPortUnpopulated\n");
        m_cb.EventHandler(&m_cmp,
        m_app_data,
        OMX_EventError,
        OMX_ErrorPortUnpopulated,
        NULL,
        NULL );
    }
    if(port == OMX_CORE_INPUT_PORT_INDEX)
    {
        if(m_inp_current_buf_count != 0)
        {
            m_inp_bPopulated = OMX_FALSE;
            if(search_input_bufhdr(buffer) == true)
            {
                DEBUG_DETAIL("Free_Buf:in_buffer[%p]\n",buffer);
                m_input_buf_hdrs.erase(buffer);
                free(buffer);
                m_inp_current_buf_count--;
            }
            else
            {
                DEBUG_PRINT_ERROR("Error: free_buffer invalid Input buf hdr\n");
                eRet = OMX_ErrorBadParameter;
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("Error: free_buffer,Port Index calculation \
                            came out Invalid\n");
            eRet = OMX_ErrorBadPortIndex;
        }

        if(BITMASK_PRESENT((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING)
                && release_done(0))
        {
            DEBUG_DETAIL("INPUT PORT MOVING TO DISABLED STATE \n");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING);
            post_command(OMX_CommandPortDisable,
            OMX_CORE_INPUT_PORT_INDEX,
            OMX_COMPONENT_GENERATE_EVENT);
        }

    }
    else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    {
        if(m_out_current_buf_count != 0)
        {
            m_out_bPopulated = OMX_FALSE;
            if(search_output_bufhdr(buffer) == true)
            {
                DEBUG_DETAIL("Free_Buf:out_buffer[%p]\n",buffer);
                m_output_buf_hdrs.erase(buffer);
                free(buffer);
                m_out_current_buf_count--;
            }
            else
            {
                DEBUG_PRINT_ERROR("Error: free_buffer invalid Output bufhdr\n");
                eRet = OMX_ErrorBadParameter;
            }
        }
        else
        {
            eRet = OMX_ErrorBadPortIndex;
        }
        if(BITMASK_PRESENT((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING)
                && release_done(1))
        {
            DEBUG_DETAIL("OUTPUT PORT MOVING TO DISABLED STATE \n");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
            post_command(OMX_CommandPortDisable,
            OMX_CORE_OUTPUT_PORT_INDEX,
            OMX_COMPONENT_GENERATE_EVENT);
        }
    }
    else
    {
        eRet = OMX_ErrorBadPortIndex;
    }

    if((eRet == OMX_ErrorNone) &&
            (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
    {
        if(release_done(-1))
        {
            if(ioctl(m_drv_fd, AUDIO_STOP, 0) == -1)
            {
                DEBUG_PRINT_ERROR("FreeBufs:Audio stop failed %d\n",errno);
            }
            DEBUG_DETAIL("RELEASE DONE SUCCESS\n");
            // Send the callback now
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_LOADING_PENDING);
            post_command(OMX_CommandStateSet,
            OMX_StateLoaded,OMX_COMPONENT_GENERATE_EVENT);
        }
    }

    return eRet;
}

/* ======================================================================
FUNCTION
COmxBase::GetState

DESCRIPTION
Returns the state information back to the caller.<TBD>

PARAMETERS
<TBD>.

RETURN VALUE
Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  COmxBase::get_state(OMX_IN OMX_HANDLETYPE  hComp,
OMX_OUT OMX_STATETYPE* state)
{
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    *state = m_state;
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
COmxBase::GetConfig

DESCRIPTION
OMX Get Config Method implementation.

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if successful.

========================================================================== */
OMX_ERRORTYPE  COmxBase::get_config(OMX_IN OMX_HANDLETYPE hComp,
OMX_IN OMX_INDEXTYPE configIndex,
OMX_INOUT OMX_PTR    configData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Config in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    DEBUG_PRINT("COmxBase::%s configIndex[%d]\n",__FUNCTION__,configIndex);
    switch(configIndex)
    {
    case OMX_IndexConfigAudioVolume:
        {
            OMX_AUDIO_CONFIG_VOLUMETYPE *volume =
            (OMX_AUDIO_CONFIG_VOLUMETYPE*) configData;

            if (volume->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
            {
                volume->nSize = sizeof(volume);
                volume->nVersion.nVersion = OMX_SPEC_VERSION;
                volume->bLinear = OMX_TRUE;
                volume->sVolume.nValue = m_volume;
                volume->sVolume.nMax   = OMX_ADEC_MAX;
                volume->sVolume.nMin   = OMX_ADEC_MIN;
            }
            else
            {
                eRet = OMX_ErrorBadPortIndex;
            }
        }
        break;

    case OMX_IndexConfigAudioMute:
        {
            OMX_AUDIO_CONFIG_MUTETYPE *mute =
            (OMX_AUDIO_CONFIG_MUTETYPE*) configData;
            if (mute->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
            {
                mute->nSize = sizeof(mute);
                mute->nVersion.nVersion = OMX_SPEC_VERSION;
                mute->bMute = (BITMASK_PRESENT(&m_flags,
                OMX_COMPONENT_MUTED)?OMX_TRUE:OMX_FALSE);
            }
            else {
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }
    default:
        eRet = OMX_ErrorUnsupportedIndex;
        break;
    }
    return eRet;
}

/**

@brief member function to retrieve version of component
@param hComp handle to this component instance
@param componentName name of component
@param componentVersion  pointer to memory space which stores the
    version number
@param specVersion pointer to memory sapce which stores version of
        openMax specification
@param componentUUID
@return Error status
*/
OMX_ERRORTYPE COmxBase::get_component_version(
OMX_IN OMX_HANDLETYPE    hComp,
OMX_OUT OMX_STRING       componentName,
OMX_OUT OMX_VERSIONTYPE* componentVersion,
OMX_OUT OMX_VERSIONTYPE* specVersion,
OMX_OUT OMX_UUIDTYPE*    componentUUID)
{
    if((hComp == NULL) || (componentName == NULL) ||
            (specVersion == NULL) || (componentUUID == NULL))
    {
        componentVersion = NULL;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Comp Version in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    /* TBD -- Return the proper version */
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
COmxBase::ComponentRoleEnum

DESCRIPTION
OMX Component Role Enum method implementation.

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  COmxBase::component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
OMX_OUT OMX_U8*       role,
OMX_IN OMX_U32        index)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    // Base class, no definition here
    DEBUG_PRINT("COmxBase::%s, No definition\n",__FUNCTION__);
    *role = '\0';
    return eRet;
}

/* ======================================================================
FUNCTION
COmxBase::SetCallbacks

DESCRIPTION
Set the callbacks.

PARAMETERS
None.

RETURN VALUE
OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE COmxBase::set_callbacks(OMX_IN OMX_HANDLETYPE      hComp,
OMX_IN OMX_CALLBACKTYPE* callbacks,
OMX_IN OMX_PTR           appData)
{
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    m_cb       = *callbacks;
    m_app_data =    appData;

    DEBUG_PRINT("COmxBase::%s\n",__FUNCTION__);

    if(m_pIn)  m_pIn->set_cb(m_cb,m_app_data);
    if(m_pOut) m_pOut->set_cb(m_cb,m_app_data);
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
COmxBase::ComponentTunnelRequest

DESCRIPTION
OMX Component Tunnel Request method implementation. <TBD>

PARAMETERS
None.

RETURN VALUE
OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  COmxBase::component_tunnel_request(
OMX_IN OMX_HANDLETYPE          hComp,
OMX_IN OMX_U32                 port,
OMX_IN OMX_HANDLETYPE          peerComponent,
OMX_IN OMX_U32                 peerPort,
OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup)
{
    if((hComp == NULL) || (peerComponent == NULL) || (tunnelSetup == NULL))
    {
        port = 0;
        peerPort = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    DEBUG_PRINT_ERROR("Error: component_tunnel_request Not Implemented\n");
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE COmxBase::use_EGL_image(
OMX_IN OMX_HANDLETYPE            hComp,
OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
OMX_IN OMX_U32                   port,
OMX_IN OMX_PTR                   appData,
OMX_IN void*                     eglImage)
{
    if((hComp == NULL) || (appData == NULL) || (eglImage == NULL))
    {
        bufferHdr = NULL;
        port = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    DEBUG_PRINT_ERROR("Error : use_EGL_image:  Not Implemented \n");
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
COmxBase::SetConfig

DESCRIPTION
OMX Set Config method implementation

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if successful.
========================================================================== */
OMX_ERRORTYPE  COmxBase::set_config(OMX_IN OMX_HANDLETYPE hComp,
OMX_IN OMX_INDEXTYPE configIndex,
OMX_IN OMX_PTR       configData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if((m_state == OMX_StateInvalid) ||(m_state == OMX_StateExecuting))
    {
        DEBUG_PRINT_ERROR("Error,Set Config in %d State\n",m_state);
        return OMX_ErrorInvalidState;
    }
    DEBUG_PRINT("COmxBase::%s configIndex[%d]\n",__FUNCTION__,configIndex);

    switch(configIndex)
    {
    case OMX_IndexConfigAudioVolume:
        {
            OMX_AUDIO_CONFIG_VOLUMETYPE *vol =
            (OMX_AUDIO_CONFIG_VOLUMETYPE*)configData;
            if (vol->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
            {
                if ((vol->sVolume.nValue <= OMX_ADEC_MAX) &&
                        (vol->sVolume.nValue >= OMX_ADEC_MIN))
                {
                    m_volume = vol->sVolume.nValue;
                    if (BITMASK_ABSENT(&m_flags, OMX_COMPONENT_MUTED))
                    {
                        /* ioctl(m_drv_fd, AUDIO_VOLUME,
                        * m_volume * OMX_ADEC_VOLUME_STEP); */
                    }
                }
                else
                {
                    eRet = OMX_ErrorBadParameter;
                }
            }
            else
            {
                eRet = OMX_ErrorBadPortIndex;
            }
        }
        break;

    case OMX_IndexConfigAudioMute:
        {
            OMX_AUDIO_CONFIG_MUTETYPE *mute =
            (OMX_AUDIO_CONFIG_MUTETYPE*)configData;
            if (mute->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
            {
                if (mute->bMute == OMX_TRUE)
                {
                    BITMASK_SET(&m_flags, OMX_COMPONENT_MUTED);
                    /* ioctl(m_drv_fd, AUDIO_VOLUME, 0); */
                }
                else
                {
                    BITMASK_CLEAR(&m_flags, OMX_COMPONENT_MUTED);
                    /* ioctl(m_drv_fd, AUDIO_VOLUME,
                    * m_volume * OMX_ADEC_VOLUME_STEP); */
                }
            }
            else
            {
                eRet = OMX_ErrorBadPortIndex;
            }
        }
        break;

    default:
        eRet = OMX_ErrorUnsupportedIndex;
        break;
    }
    return eRet;
}

/* ======================================================================
FUNCTION
COmxBase::GetExtensionIndex

DESCRIPTION
OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  COmxBase::get_extension_index(
OMX_IN OMX_HANDLETYPE  hComp,
OMX_IN OMX_STRING      paramName,
OMX_OUT OMX_INDEXTYPE* indexType)
{
    if((hComp == NULL) || (paramName == NULL) || (indexType == NULL))
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if(get_state() == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Extension Index in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if(strncmp(paramName,"OMX.Qualcomm.index.audio.sessionId",
               strlen("OMX.Qualcomm.index.audio.sessionId")) == 0)
    {
        *indexType =(OMX_INDEXTYPE)QOMX_IndexParamAudioSessionId;
        DEBUG_PRINT("Extension index type - %d\n", *indexType);
    }
    else
    {
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;

}

/* ======================================================================
FUNCTION
COmxBase::GetParam

DESCRIPTION
OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  COmxBase::get_parameter(OMX_HANDLETYPE hComp,
OMX_INDEXTYPE  paramIndex,
OMX_PTR        paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    DEBUG_PRINT("COmxBase::%s paramIndex[%d]\n",__FUNCTION__,paramIndex);

    switch(paramIndex)
    {
    case OMX_IndexParamAudioInit:
        {
            OMX_PORT_PARAM_TYPE *portParamType =
            (OMX_PORT_PARAM_TYPE *) paramData;
            DEBUG_PRINT("OMX_IndexParamAudioInit\n");

            portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
            portParamType->nSize = sizeof(portParamType);
            portParamType->nPorts           = 2;
            portParamType->nStartPortNumber = 0;
            break;
        }

    case OMX_IndexParamComponentSuspended:
        {
            OMX_PARAM_SUSPENSIONTYPE *suspend=
            (OMX_PARAM_SUSPENSIONTYPE *) paramData;
            if(getSuspendFlg())
            {
                suspend->eType = OMX_Suspended;
            }
            else
            {
                suspend->eType = OMX_NotSuspended;
            }
            DEBUG_PRINT("get_parameter: suspend type %d", suspend->eType);

            break;
        }
    case OMX_IndexParamVideoInit:
        {
            OMX_PORT_PARAM_TYPE *portParamType =
            (OMX_PORT_PARAM_TYPE *) paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamVideoInit\n");
            portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
            portParamType->nSize = sizeof(portParamType);
            portParamType->nPorts           = 0;
            portParamType->nStartPortNumber = 0;
            break;
        }
    case OMX_IndexParamPriorityMgmt:
        {
            OMX_PRIORITYMGMTTYPE *priorityMgmtType =
            (OMX_PRIORITYMGMTTYPE*)paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamPriorityMgmt\n");
            priorityMgmtType->nSize = sizeof(priorityMgmtType);
            priorityMgmtType->nVersion.nVersion = OMX_SPEC_VERSION;
            priorityMgmtType->nGroupID = m_priority_mgm.nGroupID;
            priorityMgmtType->nGroupPriority = m_priority_mgm.nGroupPriority;
            break;
        }
    case OMX_IndexParamImageInit:
        {
            OMX_PORT_PARAM_TYPE *portParamType =
            (OMX_PORT_PARAM_TYPE *) paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamImageInit\n");
            portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
            portParamType->nSize = sizeof(portParamType);
            portParamType->nPorts           = 0;
            portParamType->nStartPortNumber = 0;
            break;
        }

    case OMX_IndexParamCompBufferSupplier:
        {
            DEBUG_PRINT("get_parameter: OMX_IndexParamCompBufferSupplier\n");
            OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType
            = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamCompBufferSupplier\n");

            bufferSupplierType->nSize = sizeof(bufferSupplierType);
            bufferSupplierType->nVersion.nVersion = OMX_SPEC_VERSION;
            if(OMX_CORE_INPUT_PORT_INDEX   == bufferSupplierType->nPortIndex)
            {
                bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
            }
            else if (OMX_CORE_OUTPUT_PORT_INDEX ==
                                               bufferSupplierType->nPortIndex)
            {
                bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
            }
            else
            {
                eRet = OMX_ErrorBadPortIndex;
            }
            DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamCompBufferSupplier \
                        eRet %08x\n", eRet);
            break;
        }

    case OMX_IndexParamOtherInit:
        {
            OMX_PORT_PARAM_TYPE *portParamType =
                                             (OMX_PORT_PARAM_TYPE *) paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamOtherInit\n");
            portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
            portParamType->nSize = sizeof(portParamType);
            portParamType->nPorts           = 0;
            portParamType->nStartPortNumber = 0;
            break;
        }

    case QOMX_IndexParamAudioSessionId:
    {
       QOMX_AUDIO_STREAM_INFO_DATA *streaminfoparam =
               (QOMX_AUDIO_STREAM_INFO_DATA *) paramData;
       streaminfoparam->sessionId = m_session_id;
       DEBUG_PRINT("QOMX_IndexParamAudioSessionId--> session_id=%d\n",m_session_id);
       break;
    }

    default:
        {
            DEBUG_PRINT_ERROR("unknown param %08x\n", paramIndex);
            eRet = OMX_ErrorUnsupportedIndex;
        }
    }
    return eRet;
}

/* ======================================================================
FUNCTION
COmxBase::SetParam

DESCRIPTION
OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  COmxBase::set_parameter(OMX_HANDLETYPE hComp,
OMX_INDEXTYPE  paramIndex,
OMX_PTR        paramData)
{
    DEBUG_PRINT("COmxBase::%s paramIndex[%d]\n",__FUNCTION__,paramIndex);
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    switch(paramIndex)
    {
    case OMX_IndexParamPriorityMgmt:
        {
            DEBUG_PRINT("set_parameter: OMX_IndexParamPriorityMgmt\n");
            if(m_state != OMX_StateLoaded)
            {
                DEBUG_PRINT_ERROR("Set Parameter called in Invalid State\n");
                return OMX_ErrorIncorrectStateOperation;
            }
            OMX_PRIORITYMGMTTYPE *priorityMgmtype
            = (OMX_PRIORITYMGMTTYPE*) paramData;
            DEBUG_PRINT("set_parameter: OMX_IndexParamPriorityMgmt %u\n",
            (unsigned int)priorityMgmtype->nGroupID);
            DEBUG_PRINT("set_parameter: priorityMgmtype %d\n",
            (unsigned int)priorityMgmtype->nGroupPriority);
            m_priority_mgm.nGroupID = priorityMgmtype->nGroupID;
            m_priority_mgm.nGroupPriority = priorityMgmtype->nGroupPriority;
            break;
        }
    case OMX_IndexParamCompBufferSupplier:
        {
            DEBUG_PRINT("set_parameter: OMX_IndexParamCompBufferSupplier\n");
            OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType
            = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
            DEBUG_PRINT("set_parameter: OMX_IndexParamCompBufferSupplier %d\n",
            bufferSupplierType->eBufferSupplier);

            if(bufferSupplierType->nPortIndex == OMX_CORE_INPUT_PORT_INDEX ||
               bufferSupplierType->nPortIndex == OMX_CORE_OUTPUT_PORT_INDEX)
            {
                DEBUG_PRINT("set_para: IndexParamCompBufferSupplier In/Out \n");
                m_buffer_supplier.eBufferSupplier =
                                           bufferSupplierType->eBufferSupplier;
            }
            else
            {
                eRet = OMX_ErrorBadPortIndex;
            }

            DEBUG_PRINT_ERROR("set_parameter:OMX_IndexParamCompBufferSupplier: \
                                            eRet  %08x\n", eRet);
            break;
        }

    case OMX_IndexParamAudioPcm:
        {
            DEBUG_PRINT("set_parameter: OMX_IndexParamAudioPcm\n");
            OMX_AUDIO_PARAM_PCMMODETYPE *pcmparam
            = (OMX_AUDIO_PARAM_PCMMODETYPE *) paramData;

            if (OMX_CORE_OUTPUT_PORT_INDEX== pcmparam->nPortIndex)
            {
                COmxBase::m_pcm_param.nChannels =  pcmparam->nChannels;
                COmxBase::m_pcm_param.eNumData = pcmparam->eNumData;
                COmxBase::m_pcm_param.bInterleaved = pcmparam->bInterleaved;
                COmxBase::m_pcm_param.nBitPerSample =   pcmparam->nBitPerSample;
                COmxBase::m_pcm_param.nSamplingRate =   pcmparam->nSamplingRate;
                COmxBase::m_pcm_param.ePCMMode =  pcmparam->ePCMMode;
                COmxBase::m_pcm_param.eChannelMapping[0] =
                                                   pcmparam->eChannelMapping[0];
                COmxBase::m_pcm_param.eChannelMapping[1] =
                                                   pcmparam->eChannelMapping[1];

                DEBUG_PRINT("set_parameter: Sampling rate[%u]",\
                                         (unsigned int)pcmparam->nSamplingRate);
                DEBUG_PRINT("set_parameter: channels[%u]", \
                                             (unsigned int)pcmparam->nChannels);
            }
            else
            {
                DEBUG_PRINT_ERROR("set_parameter:OMX_IndexParamAudioPcm \
                        OMX_ErrorBadPortIndex %d\n", (int)pcmparam->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case OMX_IndexParamSuspensionPolicy:
        {
            OMX_PARAM_SUSPENSIONPOLICYTYPE *suspend_policy;
            suspend_policy = (OMX_PARAM_SUSPENSIONPOLICYTYPE*)paramData;
            suspensionPolicy= suspend_policy->ePolicy;
            DEBUG_PRINT("SET_PARAMETER: Set SUSPENSION POLICY %d EventObj=%p \n",
            suspensionPolicy,m_pEvent);
            break;
        }
    case OMX_IndexParamStandardComponentRole:
        {
            OMX_PARAM_COMPONENTROLETYPE *componentRole;
            componentRole = (OMX_PARAM_COMPONENTROLETYPE*)paramData;
            component_Role.nSize = componentRole->nSize;
            component_Role.nVersion = componentRole->nVersion;
            strcpy((char *)component_Role.cRole,(const char*)componentRole->cRole);
            break;
        }
    default:
        {
            DEBUG_PRINT_ERROR("unknown param %d\n", paramIndex);
            eRet = OMX_ErrorUnsupportedIndex;
        }
    }
    return eRet;
}

OMX_ERRORTYPE  COmxBase::send_command(
OMX_IN OMX_HANDLETYPE hComp,
OMX_IN OMX_COMMANDTYPE  cmd,
OMX_IN OMX_U32       param1,
OMX_IN OMX_PTR      cmdData)
{
    DEBUG_PRINT("COmxBase::%s cmd[%d]param1[%u]\n",__FUNCTION__,cmd,
    (unsigned int)param1);
    int portIndex = (int)param1;
    if(hComp == NULL)
    {
        cmdData = NULL;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if(OMX_StateInvalid == m_state)
    {
        return OMX_ErrorInvalidState;
    }
    DEBUG_PRINT(" inside  send_command is_out_th_sleep=%d\n",is_out_th_sleep);
    if ( (cmd == OMX_CommandFlush) && (portIndex > 1) )
    {
        return OMX_ErrorBadPortIndex;
    }
    post_command((unsigned)cmd,(unsigned)param1,OMX_COMPONENT_GENERATE_COMMAND);
    DEBUG_PRINT("send_command : semwait= %u\n",(unsigned int)param1);
    sem_wait (&sem_States);
    DEBUG_PRINT("send_command : semwait released\n");
    return OMX_ErrorNone;
}


OMX_ERRORTYPE  COmxBase::empty_this_buffer(
OMX_IN OMX_HANDLETYPE         hComp,
OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    OMX_STATETYPE state;

    DEBUG_PRINT("COmxBase::%s bufHdr[%p]buffer[%p]Len[%u]\n",\
    __FUNCTION__,buffer, buffer->pBuffer,
    (unsigned int)buffer->nFilledLen);
    pthread_mutex_lock(&m_state_lock);
    get_state(&m_cmp, &state);
    pthread_mutex_unlock(&m_state_lock);

    if ( state == OMX_StateInvalid )
    {
        DEBUG_PRINT("Empty this buffer in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if ( (buffer != NULL) &&
            (buffer->nInputPortIndex == 0) &&
            (buffer->nSize == sizeof (OMX_BUFFERHEADERTYPE) &&
                (buffer->nVersion.nVersion == OMX_SPEC_VERSION)) &&
            (m_inp_bEnabled ==OMX_TRUE)&&
            (search_input_bufhdr(buffer) == true))
    {
        m_pIn->inc_cnt_in_buf();
        DEBUG_PRINT("ETB:m_numInBuf is %d", m_pIn->get_cnt_in_buf());
        m_pIn->post_input((unsigned)hComp,
        (unsigned) buffer,OMX_COMPONENT_GENERATE_ETB);
        setTS(buffer->nTimeStamp);
    }
    else
    {
        DEBUG_PRINT_ERROR("Bad header %p \n", buffer);
        eRet = OMX_ErrorBadParameter;
        if ( m_inp_bEnabled ==OMX_FALSE )
        {
            DEBUG_PRINT("ETB ErrorIncorrectStateOperation Port Status %d \n",\
            m_inp_bEnabled);
            eRet =  OMX_ErrorIncorrectStateOperation;
        }
        else if (buffer->nVersion.nVersion != OMX_SPEC_VERSION)
        {
            eRet = OMX_ErrorVersionMismatch;
        }
        else if (buffer->nInputPortIndex != 0)
        {
            eRet = OMX_ErrorBadPortIndex;
        }
    }
    return eRet;
}


/* ======================================================================
FUNCTION
COmxBase::FillThisBuffer

DESCRIPTION
IL client uses this method to release the frame buffer
after displaying them.

PARAMETERS
None.

RETURN VALUE
true/false

========================================================================== */
OMX_ERRORTYPE  COmxBase::fill_this_buffer(
OMX_IN OMX_HANDLETYPE         hComp,
OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_STATETYPE state;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT("COmxBase::%s bufHdr[%p]\n",__FUNCTION__,buffer);

    pthread_mutex_lock(&m_state_lock);
    get_state(&m_cmp, &state);
    pthread_mutex_unlock(&m_state_lock);

    if(  buffer != NULL &&
            (buffer->nOutputPortIndex == 1) &&
            ((buffer->nSize == sizeof (OMX_BUFFERHEADERTYPE)) &&
                (buffer->nVersion.nVersion == OMX_SPEC_VERSION)) &&
            (m_out_bEnabled == OMX_TRUE)&&
            (search_output_bufhdr(buffer) == true)
            )
    {
        m_pOut->inc_cnt_out_buf();
        DEBUG_PRINT("FTB:m_numOutBuf is %d", m_pOut->get_cnt_out_buf());
        m_pOut->post_output((unsigned)hComp,
        (unsigned) buffer,OMX_COMPONENT_GENERATE_FTB);
    }
    else
    {
        eRet = OMX_ErrorBadParameter;
        if ( m_out_bEnabled == OMX_FALSE )
        {
            eRet = OMX_ErrorIncorrectStateOperation;
        }
        else if (buffer->nVersion.nVersion != OMX_SPEC_VERSION)
        {
            eRet = OMX_ErrorVersionMismatch;
        }
        else if (buffer->nOutputPortIndex != 1)
        {
            eRet = OMX_ErrorBadPortIndex;
        }
    }
    return eRet;
}

bool COmxBase::post_command(unsigned int p1,
unsigned int p2,
unsigned int id)
{
    bool bRet  = false;

    pthread_mutex_lock(&m_commandlock);

    m_command_q.insert_entry(p1,p2,id);

    if(m_ipc_to_cmd_th)
    {
        bRet = true;
        omx_post_msg(m_ipc_to_cmd_th, id);
    }
    DEBUG_DETAIL("COmxBase::%s-->state[%d]id[%d]cmdq[%d]flags[%x]\n",\
    __FUNCTION__,
    m_state,
    id,
    m_command_q.m_size,
    m_flags >> 3);
    pthread_mutex_unlock(&m_commandlock);

    return bRet;
}

void COmxBase::flush_ack()
{
    // Decrement the FLUSH ACK count and notify the waiting recepients
    pthread_mutex_lock(&m_flush_lock);
    --m_flush_cnt;
    if( m_flush_cnt == 0 )
    {
        event_complete();
    }
    DEBUG_PRINT("COmxBase::%s cnt=%d \n",__FUNCTION__,m_flush_cnt);
    pthread_mutex_unlock(&m_flush_lock);
}

OMX_ERRORTYPE  COmxBase::process_cmd(OMX_IN OMX_HANDLETYPE hComp,
OMX_IN OMX_COMMANDTYPE  cmd,
OMX_IN OMX_U32       param1,
OMX_IN OMX_PTR      cmdData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE* buffer;

    OMX_STATETYPE eState = (OMX_STATETYPE) param1;
    int rc = 0;
    int bFlag = 1;

    DEBUG_PRINT("COmxBase::%s m_state[%d] eState[%d]\n",__FUNCTION__,
    m_state,eState);
    if(hComp == NULL)
    {
        cmdData = NULL;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if (OMX_CommandStateSet == cmd)
    {
        /***************************/
        /* Current State is Loaded */
        /***************************/
        if(m_state == OMX_StateLoaded)
        {
            if(eState == OMX_StateIdle)
            {
                if (allocate_done() ||(m_inp_bEnabled == OMX_FALSE
                            && m_out_bEnabled == OMX_FALSE))

                {
                    DEBUG_PRINT("SCP-->Allocate Done Complete\n");
                }
                else
                {
                    DEBUG_PRINT("SCP-->Loaded to Idle-Pending\n");
                    BITMASK_SET(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                    bFlag = 0;
                }
            }
            else if(eState == OMX_StateLoaded)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Loaded\n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorSameState,
                0, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if(eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->WaitForResources\n");
                eRet = OMX_ErrorNone;
            }
            else if(eState == OMX_StateExecuting)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Executing\n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorIncorrectStateTransition,
                0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StatePause)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Pause\n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorIncorrectStateTransition,
                0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Invalid\n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorInvalidState,
                0, NULL );
                m_state = OMX_StateInvalid;
                if(m_pIn)  m_pIn->set_state(m_state);
                if(m_pOut) m_pOut->set_state(m_state);
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP-->Loaded to Invalid(%d))\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /***************************/
        /* Current State is IDLE */
        /***************************/
        else if(m_state == OMX_StateIdle)
        {
            if(eState == OMX_StateLoaded)
            {
                if(release_done(-1))
                {
                    if(ioctl(m_drv_fd, AUDIO_STOP, 0) == -1)
                    {
                        DEBUG_PRINT("SCP:Idle->Loaded,ioctl stop failed %d\n",\
                        errno);
                    }
                    /*TBD::m_first_aac_header=0;
                    //SilenceInsertionEnabled = 0;
                    nTimestamp=0;*/
                    //m_data_written_to_dsp = false;
                    DEBUG_PRINT("SCP-->Idle to Loaded\n");
                }
                else
                {
                    DEBUG_PRINT("SCP--> Idle to Loaded-Pending\n");
                    BITMASK_SET(&m_flags, OMX_COMPONENT_LOADING_PENDING);
                    // Skip the event notification
                    bFlag = 0;
                }
            }
            else if(eState == OMX_StateExecuting)
            {
                struct msm_audio_pcm_config  pcm_config;
                DEBUG_PRINT("SCP-->Tunnel/NT[%d] bufcnt=%d out_buf_size=%d\n",\
                pcm_feedback,m_out_act_buf_count,
                output_buffer_size);
                if(ioctl(m_drv_fd, AUDIO_GET_PCM_CONFIG, &pcm_config) == -1)
                DEBUG_PRINT_ERROR("SCP:Idle->Exe,ioctl get-pcm fail "
                                  "m_drv_fd=%d errno=%d\n",m_drv_fd,errno);

                pcm_config.pcm_feedback = pcm_feedback;
                pcm_config.buffer_count = m_out_act_buf_count;
                pcm_config.buffer_size  = output_buffer_size;

                if(ioctl(m_drv_fd, AUDIO_SET_PCM_CONFIG, &pcm_config) == -1)
                {
                    DEBUG_PRINT("SCP:ioctl set-pcm-config failed "
                                "m_drv_fd=%d errno=%d\n",m_drv_fd,errno);
                }
                DEBUG_PRINT("SCP-->Idle to Executing\n");
            }
            else if(eState == OMX_StateIdle)
            {
                DEBUG_PRINT("OMXCORE-SM: Idle-->Idle\n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                OMX_EventError, OMX_ErrorSameState,
                OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorSameState;
            }

            else if(eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("OMXCORE-SM: Idle-->WaitForResources\n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                OMX_EventError, OMX_ErrorIncorrectStateTransition,
                OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StatePause)
            {
                DEBUG_PRINT("SCP: Idle-->Pause\n");
            }
            else if(eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("SCP: Idle-->Invalid\n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorInvalidState,
                0, NULL );
                m_state = OMX_StateInvalid;
                if(m_pIn)  m_pIn->set_state(m_state);
                if(m_pOut) m_pOut->set_state(m_state);
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP--> Idle to %d Not Handled\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /******************************/
        /* Current State is Executing */
        /******************************/
        else if(m_state == OMX_StateExecuting)
        {
            if(eState == OMX_StateIdle)
            {
                DEBUG_PRINT("SCP-->Executing to Idle \n");

                if(pcm_feedback)
                {
                    pthread_mutex_lock(&m_flush_lock);
                    m_flush_cnt = 2;
                    pthread_mutex_unlock(&m_flush_lock);
                    execute_omx_flush(-1,false);
                }
                else
                {
                    pthread_mutex_lock(&m_flush_lock);
                    m_flush_cnt = 1;
                    pthread_mutex_unlock(&m_flush_lock);

                    execute_omx_flush(0,false);
                }
            }
            else if(eState == OMX_StatePause)
            {
                DEBUG_DETAIL("*************************\n");
                DEBUG_PRINT("SCP-->RXED PAUSE STATE\n");
                DEBUG_DETAIL("*************************\n");
                DEBUG_PRINT("SCP:E-->P, start timer\n");
                getTimerInst()->startTimer();
            }
            else if(eState == OMX_StateLoaded)
            {
                DEBUG_PRINT("\n SCP:Executing --> Loaded \n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorIncorrectStateTransition,
                0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("\nSCP:Executing --> WaitForResources \n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorIncorrectStateTransition,
                0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StateExecuting)
            {
                DEBUG_PRINT("\n SCP: Executing --> Executing \n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorSameState,
                0, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if(eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("\n SCP: Executing --> Invalid \n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorInvalidState,
                0, NULL );
                m_state = OMX_StateInvalid;
                if(m_pIn)  m_pIn->set_state(m_state);
                if(m_pOut) m_pOut->set_state(m_state);
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP--> Executing to %d Not Handled\n",\
                eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /***************************/
        /* Current State is Pause  */
        /***************************/
        else if(m_state == OMX_StatePause)
        {
            if(!getTimerInst()->getTimerExpiry())
            {
                getTimerInst()->stopTimer();
            }
            getTimerInst()->resetTimerExpiry();
            if(eState == OMX_StateExecuting)
            {
                m_pause_to_exe=true;
            }
            if( (eState == OMX_StateExecuting || eState == OMX_StateIdle) )
            {
                DEBUG_DETAIL("PE: WAKING UP OUT THREAD\n");
                out_th_wakeup();
            }
            if(eState == OMX_StateExecuting)
            {
                if(bSuspendEventRxed)
                    bResumeEventRxed = true;
                DEBUG_PRINT("SCP: Paused --> Executing %d %d\n",\
                bSuspendEventRxed,bResumeEventRxed);
                if(bSuspendEventRxed && bResumeEventRxed)
                {
                    m_pOut->post_output(0,0,OMX_COMPONENT_RESUME);
                    // start the audio driver that was suspended before
                    rc = ioctl(m_drv_fd, AUDIO_START, 0);
                    if(rc <0)
                    {
                        DEBUG_PRINT_ERROR("AUDIO_START FAILED fd=%d errno=%d\n",
                                                                m_drv_fd,errno);
                        post_command((unsigned)OMX_CommandStateSet,
                        (unsigned)OMX_StateInvalid,
                        OMX_COMPONENT_GENERATE_COMMAND);
                        execute_omx_flush(-1,false);
                        return OMX_ErrorInvalidState;
                    }
                    bSuspendEventRxed = false;
                    bResumeEventRxed = false;
                }
            }
            else if(eState == OMX_StateIdle)
            {
                DEBUG_PRINT("SCP-->Paused to Idle \n");

                if(pcm_feedback)
                {
                    pthread_mutex_lock(&m_flush_lock);
                    m_flush_cnt = 2;
                    pthread_mutex_unlock(&m_flush_lock);
                    execute_omx_flush(-1,false);
                }
                else
                {
                    pthread_mutex_lock(&m_flush_lock);
                    m_flush_cnt = 1;
                    pthread_mutex_unlock(&m_flush_lock);

                    execute_omx_flush(0,false);
                }
            }
            else if(eState == OMX_StateLoaded)
            {
                DEBUG_PRINT("\n SCP:Pause --> loaded \n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorIncorrectStateTransition,
                0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("\n SCP: Pause --> WaitForResources \n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorIncorrectStateTransition,
                0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StatePause)
            {
                DEBUG_PRINT("\n SCP:Pause --> Pause \n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorSameState,
                0, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if(eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("\n SCP:Pause --> Invalid \n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorInvalidState,
                0, NULL );
                m_state = OMX_StateInvalid;
                if(m_pIn)  m_pIn->set_state(m_state);
                if(m_pOut) m_pOut->set_state(m_state);
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT("SCP-->Paused to %d Not Handled\n",eState);
                eRet = OMX_ErrorBadParameter;

            }
        }
        /**************************************/
        /* Current State is WaitForResources  */
        /**************************************/
        else if(m_state == OMX_StateWaitForResources)
        {
            if(eState == OMX_StateLoaded)
            {
                DEBUG_PRINT("SCP: WaitForResources-->Loaded\n");
            }
            else if(eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("SCP: WaitForResources-->WaitForResources\n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorSameState,
                0, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if(eState == OMX_StateExecuting)
            {
                DEBUG_PRINT("SCP: WaitForResources-->Executing\n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorIncorrectStateTransition,
                0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StatePause)
            {
                DEBUG_PRINT("SCP: WaitForResources-->Pause\n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorIncorrectStateTransition,
                0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("SCP: WaitForResources-->Invalid\n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorInvalidState,
                0, NULL );
                m_state = OMX_StateInvalid;
                if(m_pIn)  m_pIn->set_state(m_state);
                if(m_pOut) m_pOut->set_state(m_state);
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP--> %d to %d(Not Handled)\n",m_state,
                                                                       eState);
                eRet = OMX_ErrorBadParameter;
            }
        }

        /****************************/
        /* Current State is Invalid */
        /****************************/
        else if(m_state == OMX_StateInvalid)
        {
            if((OMX_StateLoaded == eState) ||
                    (OMX_StateWaitForResources == eState) ||
                    (OMX_StateIdle == eState) ||
                    (OMX_StateExecuting == eState)||
                    (OMX_StatePause == eState) ||
                    (OMX_StateInvalid == eState))
            {
                DEBUG_PRINT("SCP: Invalid-->State transition\n");
                m_cb.EventHandler(&this->m_cmp,
                this->m_app_data,
                OMX_EventError,
                OMX_ErrorInvalidState,
                0, NULL );
                m_state = OMX_StateInvalid;
                if(m_pIn)  m_pIn->set_state(m_state);
                if(m_pOut) m_pOut->set_state(m_state);
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT("SCP: Paused --> %d Not Handled\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        DEBUG_PRINT("OMX_CommandStateSet : posting m_sem_state\n");
    }
    else if (cmd == OMX_CommandFlush)
    {
        DEBUG_DETAIL("*************************\n");
        DEBUG_PRINT("SCP-->RXED FLUSH COMMAND port=%u\n",\
        (unsigned int)param1);
        DEBUG_DETAIL("*************************\n");

        bFlag = 0;
        if((param1 == OMX_CORE_INPUT_PORT_INDEX) ||
                (param1 == OMX_CORE_OUTPUT_PORT_INDEX) ||
                ((int)param1 == -1))
        {
            execute_omx_flush(param1);
        }
        else
        {
            eRet = OMX_ErrorBadPortIndex;
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventError,
            OMX_CommandFlush, OMX_ErrorBadPortIndex, NULL );
        }
    }
    else if (cmd == OMX_CommandPortDisable)
    {
	// Skip the event notification
	bFlag = 0;
        if(param1 == OMX_CORE_INPUT_PORT_INDEX || param1 == OMX_ALL)
        {
            DEBUG_PRINT("SCP: Disabling Input port Indx\n");
            m_inp_bEnabled = OMX_FALSE;
            if(((m_state == OMX_StateLoaded) || (m_state == OMX_StateIdle))
                    && release_done(0))
            {
                DEBUG_PRINT("process_cmd:OMX_CommandPortDisable:\
                            OMX_CORE_INPUT_PORT_INDEX:release_done \n");
                DEBUG_PRINT("OMX_CommandPortDisable:%u",m_inp_bEnabled);

                post_command(OMX_CommandPortDisable,
                OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT);
            }
            else
            {
                if((m_state == OMX_StatePause) ||
                        (m_state == OMX_StateExecuting))
                {
                    DEBUG_PRINT("SCP: execute_omx_flush in Disable in \
                                        param1=%d m_state=%d \n",\
                    (unsigned int)param1, m_state);
                    execute_omx_flush(param1,false );
                }
                DEBUG_PRINT("process_cmd:OMX_CommandPortDisable:\
                                OMX_CORE_INPUT_PORT_INDEX \n");
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
            }
        }
        if ((param1 == OMX_CORE_OUTPUT_PORT_INDEX) || (param1 == OMX_ALL))
        {
            DEBUG_PRINT("SCP: Disabling Output port Indx\n");
            m_out_bEnabled = OMX_FALSE;
            bOutputPortReEnabled = false;
            if(((m_state == OMX_StateLoaded) || (m_state == OMX_StateIdle))
                    && release_done(1))
            {
                DEBUG_PRINT("process_cmd:OMX_CommandPortDisable:\
                            OMX_CORE_OUTPUT_PORT_INDEX:release_done \n");
                DEBUG_PRINT("OMX_CommandPortDisable:%u",m_inp_bEnabled);

                post_command(OMX_CommandPortDisable,
                OMX_CORE_OUTPUT_PORT_INDEX,
                OMX_COMPONENT_GENERATE_EVENT
                );
            }
            else
            {
                if((m_state == OMX_StatePause) || (m_state == OMX_StateExecuting))
                {
                    DEBUG_PRINT("SCP: execute_omx_flush in Disable out \
                                param1=%d m_state=%d \n",(unsigned int)param1,
                    m_state);
                    execute_omx_flush(param1, false);
                }
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("SCP-->Disabling invalid port ID[%d]",\
            (unsigned int)param1);
        }

    }
    else if (cmd == OMX_CommandPortEnable)
    {
	// Skip the event notification
	bFlag = 0;
        if (param1 == OMX_CORE_INPUT_PORT_INDEX  || param1 == OMX_ALL)
        {
            m_inp_bEnabled = OMX_TRUE;
            DEBUG_PRINT("SCP: Enabling Input port Indx\n");
            if(((m_state == OMX_StateLoaded) &&
                 !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)) ||
                 (m_state == OMX_StateWaitForResources) ||
                 (m_inp_bPopulated == OMX_TRUE))
            {
                post_command(OMX_CommandPortEnable,
                OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT);
            }
            else
            {
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING);
            }
        }
        if ((param1 == OMX_CORE_OUTPUT_PORT_INDEX) || (param1 == OMX_ALL))
        {
            bOutputPortReEnabled = 1;
            DEBUG_PRINT("SCP: Enabling Output port Indx\n");
            m_out_bEnabled = OMX_TRUE;
            if(((m_state == OMX_StateLoaded) &&
                        !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)) ||
                    (m_state == OMX_StateWaitForResources) ||
                    (m_out_bPopulated == OMX_TRUE))
            {
                post_command(OMX_CommandPortEnable,
                OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT);
            }
            else
            {
                DEBUG_PRINT("process_cmd:OMX_CommandPortEnable:\
                            OMX_CORE_OUTPUT_PORT_INDEX:release_done \n");
                bOutputPortReEnabled = 0;
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
            }
            DEBUG_PRINT("...WAKE UP IN & OUT TH %d %d\n",is_in_th_sleep,
                                                         is_out_th_sleep);
            out_th_wakeup();
            in_th_wakeup();
            if(get_eos_bm())
            {
                // If o/p port reconfiguration happens when EOS flags is set
                // trigger o/p EOS immediately as part of FBD
                trigger_eos(OMX_TRUE);
            }
        } else
        {
            DEBUG_PRINT_ERROR("SCP-->Enabling invalid port ID[%d]",\
            (unsigned int)param1);
        }

    }
    else
    {
        DEBUG_PRINT_ERROR("SCP-->ERROR: Invali Command [%d]\n",cmd);
        eRet = OMX_ErrorNotImplemented;
    }
    DEBUG_PRINT("posting m_sem_state\n");
    sem_post (&sem_States);
    if(eRet == OMX_ErrorNone && bFlag)
    {
        post_command(cmd,eState,OMX_COMPONENT_GENERATE_EVENT);
    }
    return eRet;
}


bool COmxBase::execute_omx_flush(OMX_IN OMX_U32 param1, bool cmd_cmpl)
{
    bool bRet = true;
    OMX_BUFFERHEADERTYPE *omx_buf;
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned      ident;
    bool          bqueStatus = 0;

    DEBUG_PRINT("COmxBase::%s Port[%d]",__FUNCTION__,(unsigned int)param1);

    struct timespec abs_timeout;
    abs_timeout.tv_sec = 1;
    abs_timeout.tv_nsec = 0; //333333;
    if ((int)param1 == -1)
    {
        m_flush_in_prog = true;
        DEBUG_PRINT("Execute flush for both port m_pIn[%p] m_pOut[%p]\n",
                                                            m_pIn,m_pOut);
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 2;
        pthread_mutex_unlock(&m_flush_lock);

        if(m_pIn)
        {
            // Send Flush commands to input and output threads
            m_pIn->post_input(OMX_CommandFlush,
            OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        }
        if(m_pOut){
            m_pOut->post_output(OMX_CommandFlush,
            OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        }
        // Send Flush to the kernel so that the in and out buffers are released
        if(ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1)
        DEBUG_PRINT_ERROR("FLush:ioctl flush failed errno=%d\n",errno);
        else
        DEBUG_DETAIL("COmxBase:: FLUSH SUCCESS...\n");

        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
        is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");
        out_th_wakeup();
        in_th_wakeup();

        while (1 )
        {
            if(!m_pOut && !m_pIn)
            {
                m_flush_in_prog = false;
                DEBUG_PRINT_ERROR("Flush,pOut and pIn are deleted already\n");
                return bRet;
            }
            DEBUG_DETAIL("Flush:nNumOutputBuf = %d nNumInputBuf=%d\n",\
            m_pOut->get_cnt_out_buf(),
            m_pIn->get_cnt_in_buf() );
            if((m_pOut->get_cnt_out_buf() > 0) || (m_pIn->get_cnt_in_buf() > 0))
            {
                out_th_wakeup();
                in_th_wakeup();
                DEBUG_DETAIL(" READ FLUSH PENDING HENCE WAIT\n");
                DEBUG_DETAIL("BEFORE READ ioctl_flush\n");
                usleep (10000);
                if(ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1)
                DEBUG_PRINT_ERROR("Flush: ioctl flush failed %d\n", errno);
                DEBUG_DETAIL("AFTER READ ioctl_flush\n");
                sem_timedwait(&sem_read_msg,&abs_timeout);
                DEBUG_DETAIL("AFTER READ done ioctl_flush\n");
            }
            else
            {
                break;
            }
        }
        DEBUG_PRINT("RECIEVED BOTH FLUSH ACK's param1=%d cmd_cmpl=%d",\
        (unsigned int)param1,cmd_cmpl);

        // sleep till the FLUSH ACK are done by both the input and
        // output threads
        wait_for_event();

        // If not going to idle state, Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if(cmd_cmpl)
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
            OMX_CommandFlush, OMX_CORE_INPUT_PORT_INDEX, NULL );
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
            OMX_CommandFlush, OMX_CORE_OUTPUT_PORT_INDEX, NULL );
            DEBUG_PRINT("Inside FLUSH.. sending FLUSH CMPL\n");
        }
        m_flush_in_prog = false;
    }
    else if (param1 == OMX_CORE_INPUT_PORT_INDEX)
    {
        DEBUG_PRINT("Execute FLUSH for I/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 1;
        pthread_mutex_unlock(&m_flush_lock);
        m_pIn->post_input(OMX_CommandFlush,
        OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);

        if(ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1)
		DEBUG_PRINT_ERROR("FLush:ioctl flush failed errno=%d\n",errno);
        else
		DEBUG_DETAIL("COmxBase:: FLUSH SUCCESS...\n");

        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
        is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");
        in_th_wakeup();
        out_th_wakeup();

        // Send Flush to the kernel so that the in and out buffers are released
        // sleep till the FLUSH ACK are done by both the input and output thrds
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%d",
        (unsigned int)param1);
        wait_for_event();
        DEBUG_DETAIL(" RECIEVED FLUSH ACK FOR I/P PORT param1=%d",\
        (unsigned int)param1);

        // Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if(cmd_cmpl)
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
            OMX_CommandFlush, OMX_CORE_INPUT_PORT_INDEX, NULL );
        }
    }
    else if (param1 == OMX_CORE_OUTPUT_PORT_INDEX)
    {
        DEBUG_PRINT("Executing FLUSH for O/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 1;
        pthread_mutex_unlock(&m_flush_lock);
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%d",(unsigned int)param1);
        m_pOut->post_output(OMX_CommandFlush,
        OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        if(ioctl( m_drv_fd, AUDIO_OUTPORT_FLUSH, 0) == -1)
        DEBUG_PRINT_ERROR("FLush:ioctl AUDIO_OUTPORT_FLUSH flush failed errno=%d\n",errno);
        else
        DEBUG_DETAIL("COmxBase:: FLUSH SUCCESS...\n");

        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
        is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");

        out_th_wakeup();

        // sleep till the FLUSH ACK are done by both the input and output thrds
        wait_for_event();
        // Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if(cmd_cmpl){
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
            OMX_CommandFlush, OMX_CORE_OUTPUT_PORT_INDEX, NULL );
        }
        DEBUG_DETAIL("RECIEVED FLUSH ACK FOR O/P PORT param1=%d ",\
        (unsigned int)param1);
    }
    else
    {
        DEBUG_PRINT("Invalid Port ID[%d]",(unsigned int)param1);

    }
    return bRet;
}

void COmxBase::process_command_msg( unsigned char id)
{
    unsigned     p1;         // Parameter - 1
    unsigned     p2;         // Parameter - 2
    unsigned     qsize  = 0;

    omx_cmd_queue *cmd_q  = get_cmd_q();
    qsize = get_q_size(cmd_q);

    DEBUG_PRINT("COmxBase::%s qsize[%d] state[%d]",__FUNCTION__,
                                               qsize, get_state());
    if(!qsize )
    {
        DEBUG_PRINT("CMD-->BREAKING FROM LOOP\n");
        return;
    }
    else
    {
        get_msg(cmd_q,&p1,&p2,&id);
    }

    DEBUG_PRINT("::%s->state[%d]id[%d]cmdq[%d] \n",__FUNCTION__,
    get_state(),id, qsize);
    switch(id)
    {
    case OMX_COMPONENT_GENERATE_EVENT:
        {
            if (get_cb().EventHandler)
            {
                if (p1 == OMX_CommandStateSet)
                {
                    if(m_pIn)  m_pIn->set_state((OMX_STATETYPE)p2);
                    if(m_pOut) m_pOut->set_state((OMX_STATETYPE)p2);
                    if(m_pEvent) m_pEvent->set_state((OMX_STATETYPE)p2);

                    set_state((OMX_STATETYPE)p2);
                    DEBUG_PRINT("CMD:Process->state set to %d \n", \
                    get_state());

                    if((get_state() == OMX_StateExecuting) ||
                       (get_state() == OMX_StateLoaded))
                    {

                        in_th_wakeup();
                        out_th_wakeup();

                        if((get_state() == OMX_StateExecuting))
                            set_is_pause_to_exe(false);

                    }
                }
                if (m_state == OMX_StateInvalid)
                {
                    get_cb().EventHandler(&m_cmp,
                    m_app_data,
                    OMX_EventError,
                    OMX_ErrorInvalidState,
                    0, NULL );
                }
                else
                {
                    get_cb().EventHandler(&m_cmp,
                    m_app_data,
                    OMX_EventCmdComplete,
                    p1, p2, NULL );
                }
            }
            else
            {
                DEBUG_PRINT_ERROR("ERROR:CMD-->EventHandler NULL \n");
            }
            break;
        }
    case OMX_COMPONENT_GENERATE_COMMAND:
        {
            DEBUG_PRINT("BEFORE process_cmd\n");
            process_cmd(&m_cmp,
            (OMX_COMMANDTYPE)p1,
            (OMX_U32)p2,(OMX_PTR)NULL);
            DEBUG_PRINT("AFTER process_cmd\n");
            break;
        }
    case  OMX_COMPONENT_PORTSETTINGS_CHANGED:
        {
            DEBUG_DETAIL("CMD-->RXED PORTSETTINGS_CHANGED");
            get_cb().EventHandler(&m_cmp,
            m_app_data,
            OMX_EventPortSettingsChanged,
            1, 1, NULL );
            break;
        }

    case OMX_COMPONENT_SUSPEND:
        {
            if(!get_dec_state())
            {
                  DEBUG_PRINT("DONT PROCESS SUSPEND EVENT, PLAYBACK NOT STARTED\n");
                  break;
            }

            setSuspendFlg();

            DEBUG_DETAIL("CMD-->Suspend event rxed suspendflag=%d \n",\
            getSuspendFlg());

            // signal the output thread to process suspend
            m_pOut->post_output(0,0,OMX_COMPONENT_SUSPEND);
            //signal the input thread to process suspend
            m_pIn->post_input(0,0,OMX_COMPONENT_SUSPEND);
            in_th_wakeup();
            out_th_wakeup();
            break;
        }
    case OMX_COMPONENT_RESUME:
        {
            // signal the output thread that RESUME has happened
            setResumeFlg();
            m_pOut->post_output(0,0,OMX_COMPONENT_RESUME);
            out_th_wakeup();
            if(getWaitForSuspendCmplFlg())
            {
                DEBUG_PRINT("Release P-->Executing context to IL client.\n");
                release_wait_for_suspend();
            }

            break;
        }
    default:
        {
            DEBUG_PRINT_ERROR("CMD->state[%d]id[%d]\n",m_state,id);
        }
    }
    return;
}


/* ======================================================================
FUNCTION
COmxBaseDec::AllocateDone

DESCRIPTION
Checks if entire buffer pool is allocated by IL Client or not.
Need this to move to IDLE state.

PARAMETERS
None.

RETURN VALUE
true/false.

========================================================================== */
bool COmxBase::allocate_done(void)
{
    OMX_BOOL bRet = OMX_FALSE;

    if(pcm_feedback== OMX_TRUE)
    {
        if ((m_inp_act_buf_count == m_inp_current_buf_count)
                &&(m_out_act_buf_count == m_out_current_buf_count))
        {
            bRet=OMX_TRUE;
        }
        if((m_inp_act_buf_count == m_inp_current_buf_count) && m_inp_bEnabled )
        m_inp_bPopulated = OMX_TRUE;

        if((m_out_act_buf_count == m_out_current_buf_count) && m_out_bEnabled )
        m_out_bPopulated = OMX_TRUE;
    }
    else if(pcm_feedback== OMX_FALSE)
    {
        if(m_inp_act_buf_count == m_inp_current_buf_count)
        {
            bRet=OMX_TRUE;
        }
        if((m_inp_act_buf_count == m_inp_current_buf_count) && m_inp_bEnabled )
        m_inp_bPopulated = OMX_TRUE;
    }
    DEBUG_PRINT("COmxBase::%s bRet=%d\n",__FUNCTION__,bRet);
    return bRet;
}

/* ======================================================================
FUNCTION
COmxBaseDec::ReleaseDone

DESCRIPTION
Checks if IL client has released all the buffers.

PARAMETERS
None.

RETURN VALUE
true/false

========================================================================== */
bool COmxBase::release_done(OMX_U32 param1)
{
    OMX_BOOL bRet = OMX_FALSE;
    DEBUG_PRINT("COmxBase::%s\n",__FUNCTION__);
    if((int)param1 ==-1)
    {
        if ((0 == m_inp_current_buf_count)&&(0 == m_out_current_buf_count))
        {
            bRet=OMX_TRUE;
        }
    }
    else if(param1 == 0 )
    {
        if ((0 == m_inp_current_buf_count))
        {
            bRet=OMX_TRUE;
        }
    }
    else if(param1 ==1)
    {
        if ((0 == m_out_current_buf_count))
        {
            bRet=OMX_TRUE;
        }
    }
    return bRet;
}

void COmxBase::setTS(unsigned int TS)
{
    nTimeStamp = TS;
}


/////////////////////////////////////////////////////////////////////////
/////////////////////BASE IN ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
COmxBaseIn::COmxBaseIn(COmxBase * base,int fd,OMX_CALLBACKTYPE cb,
OMX_PTR appdata):
m_drv_fd(fd),
m_ch_cfg(0),
m_comp(base),
m_ipc_to_in_th(NULL),
m_cb(cb),
m_app_data(appdata),
m_inp_bPopulated(OMX_FALSE),
m_state(OMX_StateLoaded),
m_inp_act_buf_count(0),
m_input_buffer_size(0),
nNumInputBuf(0),
m_sample_rate(0)
{
    DEBUG_PRINT("COmxBaseIn::%s\n",__FUNCTION__);

    memset(&m_in_pb_stat,0,sizeof(IN_PB_STATS));
    memset(&m_cb,0,sizeof(OMX_CALLBACKTYPE));

    pthread_mutexattr_init(&m_lock_attr);
    pthread_mutex_init(&m_lock, &m_lock_attr);

    pthread_mutexattr_init(&m_state_attr);
    pthread_mutex_init(&m_state_lock, &m_state_attr);

    pthread_mutexattr_init(&m_in_buf_count_lock_attr);
    pthread_mutex_init(&m_in_buf_count_lock, &m_in_buf_count_lock_attr);
}

COmxBaseIn::~COmxBaseIn()
{
    DEBUG_PRINT("COmxBaseIn::%s\n",__FUNCTION__);
    DEBUG_PRINT("STATS: in-buf-len[%d]etb_cnt[%d] ebd_cnt[%d]",\
    (unsigned int)m_in_pb_stat.tot_in_buf_len,
    (unsigned int)m_in_pb_stat.etb_cnt,
    (unsigned int)m_in_pb_stat.ebd_cnt);
    get_comp()->in_th_wakeup();
    memset(&m_in_pb_stat,0,sizeof(IN_PB_STATS));
    memset(&m_cb,0,sizeof(OMX_CALLBACKTYPE));

    if (m_ipc_to_in_th != NULL) {
        omx_thread_stop(m_ipc_to_in_th);
        m_ipc_to_in_th = NULL;
    }
    memset(&m_cb,0,sizeof(OMX_CALLBACKTYPE));
    m_state = OMX_StateLoaded;
    m_sample_rate = 0;
    m_ch_cfg = 0;
    m_inp_act_buf_count = 0;
    m_inp_bPopulated = OMX_FALSE;
    m_input_buffer_size = 0;
    m_drv_fd = -1;
    nNumInputBuf = 0;
    m_comp = NULL;


    pthread_mutex_destroy(&m_lock);
    pthread_mutexattr_destroy(&m_lock_attr);

    pthread_mutex_destroy(&m_state_lock);
    pthread_mutexattr_destroy(&m_state_attr);

    pthread_mutexattr_destroy(&m_in_buf_count_lock_attr);
    pthread_mutex_destroy(&m_in_buf_count_lock);
    DEBUG_PRINT("Destructor, COmxBaseIn \n");
}

bool COmxBaseIn::post_input(unsigned int p1,
unsigned int p2,
unsigned int id)
{
    bool bRet = false;
    pthread_mutex_lock(&m_lock);

    if((OMX_COMPONENT_GENERATE_COMMAND == id) || (id == OMX_COMPONENT_SUSPEND))
    {
        m_input_ctrl_cmd_q.insert_entry(p1,p2,id);
    }
    else if((OMX_COMPONENT_GENERATE_BUFFER_DONE == id))
    {
        m_input_ctrl_ebd_q.insert_entry(p1,p2,id);
    }
    else
    {
        m_input_q.insert_entry(p1,p2,id);
    }
    DEBUG_PRINT("m_ipc_to_in_th[%p] id[%d]\n",m_ipc_to_in_th,id);

    if(m_ipc_to_in_th)
    {
        bRet = true;
        omx_post_msg(m_ipc_to_in_th, id);
    }

    DEBUG_DETAIL("PostInput-->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d] \n",\
    m_state,
    id,
    m_input_ctrl_cmd_q.m_size,
    m_input_ctrl_ebd_q.m_size,
    m_input_q.m_size);
    pthread_mutex_unlock(&m_lock);
    return bRet;
}

void COmxBaseIn::buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{
    DEBUG_DETAIL("COmxBaseIn::%s bufHdr[%p]\n",__FUNCTION__,bufHdr);
    if(m_cb.EmptyBufferDone)
    {
        m_in_pb_stat.tot_in_buf_len += bufHdr->nFilledLen;
        m_in_pb_stat.ebd_cnt++;
        dec_cnt_in_buf();
        bufHdr->nFilledLen = 0;
        PrintFrameHdr(OMX_COMPONENT_GENERATE_BUFFER_DONE,bufHdr);
        m_cb.EmptyBufferDone(&(get_comp()->m_cmp), m_app_data, bufHdr);
        DEBUG_DETAIL("EBD CB:: in_buf_len=%u nNumInputBuf=%u\n",\
        (unsigned int)m_in_pb_stat.tot_in_buf_len,
        (unsigned int)nNumInputBuf);
    }
    return;
}


bool COmxBaseIn::execute_input_omx_flush()
{
    DEBUG_PRINT("COmxBaseIn::%s\n",__FUNCTION__);
    OMX_BUFFERHEADERTYPE *omx_buf;
    unsigned      p1;
    unsigned      p2;
    unsigned char ident;
    unsigned      qsize=0;
    unsigned      tot_qsize=0;

    pthread_mutex_lock(&m_lock);
    do
    {
        qsize = m_input_q.m_size;
        tot_qsize = qsize;
        tot_qsize += m_input_ctrl_ebd_q.m_size;

        DEBUG_DETAIL("Input FLUSH-->flushq[%d] ebd[%d]dataq[%d]",\
        m_input_ctrl_cmd_q.m_size,
        m_input_ctrl_ebd_q.m_size,qsize);
        if(!tot_qsize)
        {
            DEBUG_DETAIL("Input-->BREAKING FROM execute_input_flush LOOP");
            break;
        }
        if (qsize)
        {
            m_input_q.pop_entry(&p1, &p2, &ident);
            if ((ident == OMX_COMPONENT_GENERATE_ETB) ||
                    (ident == OMX_COMPONENT_GENERATE_BUFFER_DONE))
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                omx_buf->nFilledLen = 0;
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
            }
        }
        else if((m_input_ctrl_ebd_q.m_size))
        {
            m_input_ctrl_ebd_q.pop_entry(&p1, &p2, &ident);
            if(ident == OMX_COMPONENT_GENERATE_BUFFER_DONE)
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                omx_buf->nFilledLen = 0;
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
            }
        }
        else{}
    }while(tot_qsize>0);
    DEBUG_DETAIL("*************************\n");
    DEBUG_DETAIL("IN-->FLUSHING DONE\n");
    DEBUG_DETAIL("*************************\n");
    m_comp->flush_ack();
    m_input_port_flushed = OMX_TRUE;
    pthread_mutex_unlock(&m_lock);
    return true;
}
/////////////////////////////////////////////////////////////////////////
/////////////////////BASE OUT ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
COmxBaseOut::COmxBaseOut(COmxBase * base, int fd,OMX_CALLBACKTYPE cb,
OMX_PTR appdata):
m_drv_fd(fd),
m_comp(base),
m_ipc_to_out_th(NULL),
output_buffer_size(0),
nNumOutputBuf(0),
m_app_data(appdata),
m_cb(cb),
m_state(OMX_StateInvalid)
{
    DEBUG_PRINT("COmxBaseOut::%s \n",__FUNCTION__);
    memset(&m_out_pb_stat,0,sizeof(OUT_PB_STATS));
    memset(&m_cb,0,sizeof(OMX_CALLBACKTYPE));

    pthread_mutexattr_init(&out_buf_count_lock_attr);
    pthread_mutex_init(&out_buf_count_lock, &out_buf_count_lock_attr);

    pthread_mutexattr_init(&m_outputlock_attr);
    pthread_mutex_init(&m_outputlock, &m_outputlock_attr);

    pthread_mutexattr_init(&m_state_attr);
    pthread_mutex_init(&m_state_lock, &m_state_attr);
}

COmxBaseOut::~COmxBaseOut()
{
    DEBUG_PRINT("COmxBaseOut::%s \n",__FUNCTION__);
    DEBUG_PRINT("STATS: in-buf-len[%u]pbtime[%d]fbd_cnt[%d]ftb_cnt[%d]",\
    (unsigned int)m_out_pb_stat.tot_out_buf_len,
    (unsigned int)m_out_pb_stat.tot_pb_time,
    (unsigned int)m_out_pb_stat.fbd_cnt,
    (unsigned int)m_out_pb_stat.ftb_cnt);

    get_comp()->out_th_wakeup();

    memset(&m_out_pb_stat,0,sizeof(OUT_PB_STATS));
    memset(&m_cb,0,sizeof(OMX_CALLBACKTYPE));

    if (m_ipc_to_out_th != NULL) {
        omx_thread_stop(m_ipc_to_out_th);
        m_ipc_to_out_th = NULL;
    }
    m_app_data = NULL;
    memset(&m_cb,0,sizeof(OMX_CALLBACKTYPE));
    m_state = OMX_StateLoaded;
    m_drv_fd = -1;
    output_buffer_size = 0;
    nNumOutputBuf = 0;
    m_state = OMX_StateInvalid;
    m_comp = NULL;
    m_app_data = NULL;

    pthread_mutexattr_destroy(&m_outputlock_attr);
    pthread_mutexattr_destroy(&m_state_attr);
    pthread_mutexattr_destroy(&out_buf_count_lock_attr);

    pthread_mutex_destroy(&out_buf_count_lock);
    pthread_mutex_destroy(&m_outputlock);
    pthread_mutex_destroy(&m_state_lock);
    DEBUG_DETAIL("Destructor, COmxBaseOut,Out\n");
}

bool COmxBaseOut::post_output(unsigned int p1,
unsigned int p2,
unsigned int id)
{
    bool bRet = false;
    pthread_mutex_lock(&m_outputlock);
    if((OMX_COMPONENT_GENERATE_COMMAND == id) ||
            (id == OMX_COMPONENT_SUSPEND) ||
            (id == OMX_COMPONENT_RESUME))
    {
        // insert flush message and fbd
        m_output_ctrl_cmd_q.insert_entry(p1,p2,id);
    }
    else if((OMX_COMPONENT_GENERATE_FRAME_DONE == id) )
    {
        // insert flush message and fbd
        m_output_ctrl_fbd_q.insert_entry(p1,p2,id);
    }
    else
    {
        m_output_q.insert_entry(p1,p2,id);
    }
    if(m_ipc_to_out_th)
    {
        bRet = true;
        omx_post_msg(m_ipc_to_out_th, id);
    }
    DEBUG_DETAIL("PostOutput-->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d]\n",\
    m_state,
    id,
    m_output_ctrl_cmd_q.m_size,
    m_output_ctrl_fbd_q.m_size,
    m_output_q.m_size);
    pthread_mutex_unlock(&m_outputlock);
    return bRet;
}


void COmxBaseOut::frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{
    DEBUG_DETAIL("COmxBaseOut::%s bufHdr[%p]\n",__FUNCTION__,bufHdr);
    if(m_cb.FillBufferDone)
    {
        PrintFrameHdr(OMX_COMPONENT_GENERATE_FRAME_DONE,bufHdr);
        m_out_pb_stat.fbd_cnt++;
        dec_cnt_out_buf();
        DEBUG_DETAIL("FBD CB:: nNumOutputBuf=%u out_buf_len=%u\n",\
        (unsigned int)nNumOutputBuf,
        (unsigned int)m_out_pb_stat.tot_out_buf_len);
        m_out_pb_stat.tot_pb_time     = bufHdr->nTimeStamp;
        m_cb.FillBufferDone(&(get_comp()->m_cmp), m_app_data, bufHdr);
    }
    return;
}


bool COmxBaseOut::execute_output_omx_flush()
{
    OMX_BUFFERHEADERTYPE *omx_buf;
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned char ident;
    unsigned       qsize=0; // qsize
    unsigned       tot_qsize=0; // qsize

    DEBUG_PRINT("COmxBaseOut::%s\n",__FUNCTION__);

    pthread_mutex_lock(&m_outputlock);
    do
    {
        qsize = m_output_q.m_size;
        DEBUG_DETAIL("OUT FLUSH-->flushq[%d] fbd[%d]dataq[%d]",\
        m_output_ctrl_cmd_q.m_size,
        m_output_ctrl_fbd_q.m_size,qsize);
        tot_qsize = qsize;
        tot_qsize += m_output_ctrl_fbd_q.m_size;
        if(!tot_qsize)
        {
            DEBUG_DETAIL("OUT-->BREAKING FROM execute_input_flush LOOP");
            break;
        }
        if (qsize)
        {
            m_output_q.pop_entry(&p1,&p2,&ident);
            if ( (ident == OMX_COMPONENT_GENERATE_FTB) ||
                    (ident == OMX_COMPONENT_GENERATE_FRAME_DONE))
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                omx_buf->nTimeStamp = m_comp->getTS();
                omx_buf->nFilledLen = 0;
                frame_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                DEBUG_DETAIL("CALLING FBD FROM FLUSH");
            }
        }
        else if((qsize = m_output_ctrl_fbd_q.m_size))
        {
            m_output_ctrl_fbd_q.pop_entry(&p1, &p2, &ident);
            if(ident == OMX_COMPONENT_GENERATE_FRAME_DONE)
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                omx_buf->nTimeStamp = m_comp->getTS();
                omx_buf->nFilledLen = 0;
                frame_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                DEBUG_DETAIL("CALLING FROM CTRL-FBDQ FROM FLUSH");
            }
        }
    }while(qsize>0);
    DEBUG_DETAIL("*************************\n");
    DEBUG_DETAIL("OUT-->FLUSHING DONE\n");
    DEBUG_DETAIL("*************************\n");
    m_comp->flush_ack();
    pthread_mutex_unlock(&m_outputlock);

    return true;
}


COmxBaseEventHandler::COmxBaseEventHandler(COmxBase *base,
int drv_fd,
OMX_BOOL suspensionPolicy,
OMX_STATETYPE state):
m_drv_fd(drv_fd),
m_suspensionPolicy(suspensionPolicy),
m_state(state),
m_comp(base),
m_ipc_to_event_th(NULL)
{
    DEBUG_PRINT("COmxBaseEventHandler::%s \n",__FUNCTION__);

    pthread_mutexattr_init(&m_state_attr);
    pthread_mutex_init(&m_state_lock, &m_state_attr);
}

COmxBaseEventHandler::~COmxBaseEventHandler()
{
    DEBUG_PRINT("COmxBaseEventHandler::%s \n",__FUNCTION__);
    DEBUG_PRINT("Destructor, COmxBaseEventHandler..............\n");
    if( ioctl(m_drv_fd,AUDIO_ABORT_GET_EVENT,NULL) < 0)
    DEBUG_PRINT_ERROR("EVENT ABORT Failed fd=%d errno=%d\n",m_drv_fd,errno);

    DEBUG_PRINT("Destructor, COmxBaseEventHandler.ABORT EVENT..success..\n");
    if (m_ipc_to_event_th != NULL )
    {
        omx_thread_stop(m_ipc_to_event_th);
        m_ipc_to_event_th = NULL;
    }
    m_drv_fd = -1;
    m_state = OMX_StateLoaded;

    pthread_mutexattr_destroy(&m_state_attr);
    pthread_mutex_destroy(&m_state_lock);

    DEBUG_PRINT("Destructor, COmxBaseEventHandler.complete.............\n");
}

OMX_ERRORTYPE COmxBaseEventHandler::processEvents()
{
    OMX_STATETYPE                state;
    struct msm_audio_event       dsp_event;
    struct msm_audio_bitstream_info stream_info;
    int rc = 0;

    DEBUG_PRINT("COmxBaseEventHandler::%s \n",__FUNCTION__);
    while(1)
    {
        DEBUG_DETAIL("COmxBaseEventHandler::%s:Waiting for AUDIO_GET_EVENT\n",\
                                                                  __FUNCTION__);
        rc = ioctl(m_drv_fd,AUDIO_GET_EVENT,&dsp_event);
        DEBUG_PRINT("PE:rc%d errno=%d, dsp_event %d",rc,errno,
                                                     dsp_event.event_type);
        if ((rc == -1))
        {
            DEBUG_PRINT_ERROR("PE:Event Thread exiting %d",rc);
            return OMX_ErrorUndefined;
        }

        OMX_STATETYPE state = get_state();
        switch ( dsp_event.event_type )
        {
        case AUDIO_EVENT_STREAM_INFO:
            {
                // do nothing, this is specific to decoder
            }

        case AUDIO_EVENT_SUSPEND:
            {
                if(get_comp()->getSuspendFlg() && get_comp()->getResumeFlg())
                {
                    DEBUG_PRINT("PE:Ignoring Event Already in "
                                "Suspended state[%d] suspension_policy[%d]"
                                "event[%d] suspendflg[%d] resumeflg[%d]\n",
                                state, m_suspensionPolicy,
                                dsp_event.event_type,get_comp()->getSuspendFlg(),
                                get_comp()->getResumeFlg());
                    // Ignore events if not Pause state;
                    continue;
                }

                if((state != OMX_StatePause) ||
                        (m_suspensionPolicy != OMX_TRUE))
                {
                    DEBUG_PRINT("PE:Ignoring Suspend Event state[%d] "
                                "suspension_policy[%d] event[%d] ",\
                                state, m_suspensionPolicy,dsp_event.event_type);
                    // Ignore events if not Pause state;
                    continue;
                }
                else
                {
                    DEBUG_PRINT("PE: Recieved AUDIO_EVENT_SUSPEND");
                    get_comp()->getTimerInst()->stopTimer();

                    if(get_comp()->getSuspendFlg())
                    {
                        DEBUG_PRINT("PE:Suspend event already in process\n");
                        break;
                    }
                    get_comp()->setSuspendFlg();
                    m_comp->post_command(0,0,OMX_COMPONENT_SUSPEND);
                }

            }
            break;

        case AUDIO_EVENT_RESUME:
            {
                if(get_comp()->getSuspendFlg() && get_comp()->getResumeFlg())
                {
                    DEBUG_PRINT("PE:Ignoring Event,Comp Already Suspended [%d] "
                    "suspension_policy[%d]event[%d]suspendflg[%d]"
                    "resumeflg[%d]\n",\
                    state, m_suspensionPolicy,
                    dsp_event.event_type,get_comp()->getSuspendFlg(),
                    get_comp()->getResumeFlg());
                    // Ignore events if not Pause state;
                    continue;
                }
                if((state != OMX_StatePause) ||
                        (m_suspensionPolicy != OMX_TRUE))
                {
                    DEBUG_PRINT("PE:Ignoring Resume Event state[%d] "
                                "suspension_policy[%d] event[%d] ",\
                                state, m_suspensionPolicy,dsp_event.event_type);
                    // Ignore events if not Pause state;
                    continue;
                }
                else if ( (get_comp()->getSuspendFlg()) &&
                          !(get_comp()->getResumeFlg()) )
                {
                    DEBUG_PRINT("PE: Recieved AUDIO_EVENT_RESUME");
                    // signal the output thread that RESUME has happened
                    get_comp()->setResumeFlg();
                    m_comp->post_command(0,0,OMX_COMPONENT_RESUME);
                }

            }
            break;

        default:

            DEBUG_PRINT("Invalid event rxed type=%d\n",\
            dsp_event.event_type);
            break;
        }
    }
    return OMX_ErrorNone;
}

COmxTimer::COmxTimer(COmxBase* base):
    m_timerExpiryFlg(false),
    m_timeout(30),
    m_deleteTimer(OMX_FALSE),
    m_timer_cnt(0),
    m_base(base),
    m_timerinfo(NULL)
{
    sem_init(&m_sem_state,0, 0);

    pthread_cond_init (&m_timer_cond, NULL);
    pthread_mutexattr_init(&m_timer_mutex_attr);
    pthread_mutex_init(&m_timer_mutex, &m_timer_mutex_attr);

    pthread_cond_init (&m_tcond, NULL);
    pthread_mutexattr_init(&m_tmutex_attr);
    pthread_mutex_init(&m_tmutex, &m_tmutex_attr);

    m_timerinfo = (TIMERINFO*)malloc(sizeof(TIMERINFO));
    m_timerinfo->timer = this;
    m_timerinfo->base  = m_base;
    int rc = pthread_create(&m_timerinfo->thr ,0, omx_comp_timer_handler,
                             (void*)m_timerinfo );
    if(rc < 0)
{
        DEBUG_PRINT_ERROR("Fail to create timer thread rc=%d errno=%d\n",rc,
                                                                     errno);
        free(m_timerinfo);
        m_timerinfo = NULL;
        }
        else
        DEBUG_PRINT("Created thread for timer object...\n");
}

COmxTimer::~COmxTimer()
{

    releaseTimer();
    stopTimer();
    int rc = pthread_join(m_timerinfo->thr,NULL);
    DEBUG_PRINT("******************************\n");
    DEBUG_PRINT("CLOSING TIMER THREAD...%d\n",rc,m_timerinfo->thr);
    DEBUG_PRINT("******************************\n");
    if(m_timerinfo)
        {
        m_timerinfo->timer = NULL;
        m_timerinfo->base = NULL;
        free(m_timerinfo);
        m_timerinfo = NULL;
        }

    sem_destroy(&m_sem_state);
    pthread_mutexattr_destroy(&m_timer_mutex_attr);
    pthread_mutex_destroy(&m_timer_mutex);
    pthread_cond_destroy(&m_timer_cond);

    pthread_mutexattr_destroy(&m_tmutex_attr);
    pthread_mutex_destroy(&m_tmutex);
    pthread_cond_destroy(&m_tcond);

    m_timerExpiryFlg=false;
    m_deleteTimer = OMX_FALSE;
    m_timer_cnt = 1;
    m_base = NULL;
    }

void COmxTimer::wait_for_timer_event()
{
    sem_wait(&m_sem_state);
}

void COmxTimer::startTimer()
{
    sem_post(&m_sem_state);
}

int COmxTimer::timer_run()
{
    int rc =0;

    struct timespec   ts;
    struct timeval    tp;

    clock_gettime(CLOCK_REALTIME, &ts);
    DEBUG_PRINT("%s: Starting timer at %u %u %d\n",
                            __FUNCTION__,
                            ts.tv_sec,ts.tv_nsec,m_timer_cnt);
    clock_gettime(CLOCK_REALTIME, &ts);
            /* Convert from timeval to timespec */
    ts.tv_sec += m_timeout;
    pthread_mutex_lock(&m_timer_mutex);
    while (m_timer_cnt == 0)
    {
        if(getReleaseTimerStat()== OMX_TRUE)
        {
            DEBUG_PRINT_ERROR("Killing timer thread...\n");
            pthread_mutex_unlock(&m_timer_mutex);
            return 0;
        }
        rc = pthread_cond_timedwait(&m_timer_cond,
                                    &m_timer_mutex,
                                    &ts);
        DEBUG_PRINT("Timed wait rc=%d\n",rc);
        break;
    }
    m_timer_cnt = 0;
    pthread_mutex_unlock(&m_timer_mutex);
    clock_gettime(CLOCK_REALTIME, &ts);
    DEBUG_PRINT("%s: Elapsed Timer: %u %u\n",
                            __FUNCTION__,
                            ts.tv_sec,ts.tv_nsec);
    return rc;
    }

void COmxTimer::stopTimer()
{
    pthread_mutex_lock(&m_timer_mutex);
    if(m_timer_cnt == 0)
    {
        m_timer_cnt = 1;
        pthread_cond_signal(&m_timer_cond);
    }
    m_timer_cnt=0;
    pthread_mutex_unlock(&m_timer_mutex);
    DEBUG_PRINT("STOP TIMER...\n");
    return;
}

void* omx_comp_timer_handler(void *pT)
{
    int count = 0;
    TIMERINFO *pTime = (TIMERINFO*)pT;
    COmxTimer *pt = pTime->timer;
    COmxBase  *pb = pTime->base;
    int               rc = 0;

    while(1)
    {
        pt->wait_for_timer_event();
        if(pt->getReleaseTimerStat()== OMX_TRUE)
        {
            DEBUG_PRINT_ERROR("Killing timer thread...\n");
            goto exit_th;
        }
        rc = pt->timer_run();
        if(rc != ETIMEDOUT)
        {
            if(pt->getReleaseTimerStat()== OMX_TRUE)
            {
                DEBUG_PRINT_ERROR("Now, Kill timer thread...\n");
                goto exit_th;
            }
            else
            {
                DEBUG_PRINT("Timer, go and wait again...\n");
                continue;
            }
}
        // now post event to command thread;
        DEBUG_DETAIL("SH:state=%d suspendstat=%d\n",pb->get_state(),\
                                                    pb->getSuspendFlg());
        if( (pb->get_state()== OMX_StatePause) && !(pb->getSuspendFlg()))
        {
            // post suspend message to command thread;
            pb->post_command(0,0,OMX_COMPONENT_SUSPEND);
            pt->setTimerExpiry();
        }
        else
        {
            DEBUG_PRINT("SH: Ignore Timer expiry state=%d",pb->get_state());
        }
    }

exit_th:
    DEBUG_PRINT_ERROR("Timer thread exited\n");
    return NULL;
}
