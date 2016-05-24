/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file omx_mp3_adec.cpp
  This module contains the implementation of the OpenMAX core & component.

Copyright (c) 2006-2013 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

/*============================================================================
                              Edit History

$Header: //linux/pkgs/proprietary/mm-audio/main/source/7k/adec-omxmp3/src/omx_mp3_adec.cpp#1 $
when       who     what, where, why
--------   ---     -------------------------------------------------------

============================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////
#include<string.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<sys/mman.h>
#include<omx_mp3_adec.h>
#include <errno.h>
#include <semaphore.h>

omx_mp3_adec* mp3Inst= NULL;
OMX_U32 mp3_frequency_index[3][4] = {
    { 11025,0,22050,44100},
    {12000,0,24000,48000},
    {8000,0,16000,32000}
};


using namespace std;

// omx_cmd_queue destructor
omx_mp3_adec::omx_cmd_queue::~omx_cmd_queue()
{
    // Nothing to do
}

// omx cmd queue constructor
omx_mp3_adec::omx_cmd_queue::omx_cmd_queue(): m_read(0),m_write(0),m_size(0)
{
    memset(m_q,      0,sizeof(omx_event)*OMX_CORE_CONTROL_CMDQ_SIZE);
}

// omx cmd queue insert
bool omx_mp3_adec::omx_cmd_queue::insert_entry(unsigned p1, unsigned p2,
                                               unsigned id)
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
        DEBUG_PRINT_ERROR("ERROR!!! Command Queue Full");
    }
    return ret;
}

// omx cmd queue delete
bool omx_mp3_adec::omx_cmd_queue::pop_entry(unsigned *p1, unsigned *p2,
                                            unsigned *id)
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
        DEBUG_PRINT_ERROR("ERROR Delete!!! Command Queue Empty");
    }
    return ret;
}

// factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
    return (new omx_mp3_adec);
}
bool omx_mp3_adec::omx_cmd_queue::get_msg_id(unsigned *id)
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

/** ======================================================================
 @brief static member function for handling all events from kernel

  Kernel events are processed through this routine.
  @param client_data pointer to decoder component
  @param id command identifier
  @return none
 */

void omx_mp3_adec::process_event_cb(void *client_data, unsigned char id)
{
    (void)id;
    DEBUG_PRINT("PE:Waiting for event's...");
    omx_mp3_adec  *pThis = (omx_mp3_adec *) client_data;
    pThis->process_events(pThis);
}

void omx_mp3_adec::process_events(omx_mp3_adec *client_data)
{
    OMX_BUFFERHEADERTYPE   *bufHdr = NULL;
    struct mmap_info *ion_buf = NULL;
    OMX_STATETYPE state;
    struct msm_audio_event tcxo_event;
    struct msm_audio_ion_info audio_ion_buf;
    int rc = 0;
    META_OUT *pmeta_out = NULL;
    bool breakflag = false;
    (void)client_data;
    // This loop waits indefintely till an EVENT is recieved.
    while(1)
    {
        DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
        DEBUG_PRINT("\n PE::Calling AUDIO_GET_EVENT ...\n");
        rc = ioctl(m_drv_fd,AUDIO_GET_EVENT,&tcxo_event);
        DEBUG_PRINT("PE:Event Thread %d errno=%d",rc,errno);
        if((rc == -1) )
        {
            DEBUG_PRINT("PE:Event Thread exiting %d",rc);
            return;
        }
        DEBUG_PRINT("PE:state[%d] suspensionpolicy[%d] event[%d]",\
                                          state, suspensionPolicy,
                                          tcxo_event.event_type);

        switch(tcxo_event.event_type)
        {
            case AUDIO_EVENT_STREAM_INFO:
            {
                if(!pcm_feedback)
                {
                    DEBUG_PRINT("PE:Rxed StreamInfo for tunnel mode\n");
                    break;
                }
                DEBUG_PRINT("PE: Recieved AUDIO_EVENT_STREAM_INFO");
                struct msm_audio_bitstream_info stream_info;
                ioctl(m_drv_fd, AUDIO_GET_STREAM_INFO,&stream_info);

              /*  struct msm_audio_bitstream_info *pStrm =
                    (struct msm_audio_bitstream_info*)malloc(
                                    sizeof(struct msm_audio_bitstream_info));
                memcpy(pStrm,&stream_info,
                                    sizeof(struct msm_audio_bitstream_info));

                // command thread needs to free the memory allocated here.
                post_command(0,(unsigned int)pStrm,OMX_COMPONENT_STREAM_INFO);*/
            if ((pSamplerate != stream_info.sample_rate) ||
                (pChannels != stream_info.chan_info))
            {
                m_adec_param.nSampleRate = stream_info.sample_rate;
                m_adec_param.nChannels = stream_info.chan_info;
                DEBUG_PRINT("OMX_COMPONENT_PORTSETTINGS_CHANGED\n");
                DEBUG_PRINT("Changed frequency %lu channel %lu\n",m_adec_param.nSampleRate,m_adec_param.nChannels);
                post_command(0,0,OMX_COMPONENT_PORTSETTINGS_CHANGED);
            }
            else
            {
                m_out_bEnabled = OMX_TRUE;
                bOutputPortReEnabled = 1;
                DEBUG_PRINT("OMX COMPONENT PORTSETTINGS NOT CHANGED\n");
                ioctl( m_drv_fd, AUDIO_OUTPORT_FLUSH, 0);
                pthread_mutex_lock(&m_out_th_lock_1);
                if ( is_out_th_sleep )
                {
                    is_out_th_sleep = false;
                    DEBUG_DETAIL("ETBP:WAKING UP OUT THREADS\n");
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                    DEBUG_PRINT("ETBP:WAKING UP IN THREADS\n");
                    in_th_wakeup();
                    is_in_th_sleep = false;
                }
                pthread_mutex_unlock(&m_in_th_lock_1);
            }
                pSamplerate = stream_info.sample_rate;
                pChannels = stream_info.chan_info;
                break;
          }
          case AUDIO_EVENT_BITSTREAM_ERROR_INFO:
          {
              DEBUG_PRINT("PE: Rxed AUDIO_ERROR_INFO...\n");
              struct msm_audio_bitstream_error_info error_info;

              if(!pcm_feedback)
              {
                    DEBUG_PRINT("PE:Rxed StreamInfo for tunnel mode\n");
                    break;
              }
              ioctl(m_drv_fd,AUDIO_GET_BITSTREAM_ERROR_INFO,&error_info);
              DEBUG_PRINT("DEC ID = %d\n",
                               error_info.dec_id);
              DEBUG_PRINT("ERR INDICATOR = %d\n",
                               error_info.err_msg_indicator);
              DEBUG_PRINT("DEC TYPE = %d\n",
                             error_info.err_type);

              if(!bOutputPortReEnabled)
              {
                  bOutputPortReEnabled = true;
                  pthread_mutex_lock(&m_out_th_lock_1);
                  if(is_out_th_sleep)
                  {
                      DEBUG_DETAIL("PE:WAKING UP OUT THREADS\n");
                      out_th_wakeup();
                      is_out_th_sleep = false;
                  }
                  pthread_mutex_unlock(&m_out_th_lock_1);

              }
              break;

          }
        case AUDIO_EVENT_SUSPEND:
            {
                get_state(&m_cmp, &state);
                if(pcm_feedback&&
                  (suspensionPolicy == OMX_SuspensionEnabled) &&
                  (state == OMX_StatePause) )
                {
                    DEBUG_PRINT("PE: Recieved AUDIO_EVENT_SUSPEND");
                    pthread_mutex_lock(&m_suspendresume_lock);
                    if ( bSuspendEventRxed )
                    {
                        DEBUG_PRINT("suspend event allready in process\n");
                        pthread_mutex_unlock(&m_suspendresume_lock);
                        break;
                    }
                    pthread_mutex_unlock(&m_suspendresume_lock);
                    m_timer->stopTimer();
                    post_command(0,0,OMX_COMPONENT_SUSPEND);
                }
                else
                {
                    DEBUG_PRINT("Suspend not processed Mode[%d] state[%d] Suspend[%d]",
                                pcm_feedback,state,bSuspendEventRxed);
                }
            }
            break;

        case AUDIO_EVENT_RESUME:
            {
                get_state(&m_cmp, &state);
                if(pcm_feedback&&
                  (suspensionPolicy == OMX_SuspensionEnabled) &&
                  (state == OMX_StatePause) )
                {
                    pthread_mutex_lock(&m_suspendresume_lock);
                    if (getSuspendFlg() && !getResumeFlg())
                    {
                        pthread_mutex_unlock(&m_suspendresume_lock);
                        DEBUG_PRINT("PE: Ignoring AUDIO_EVENT_RESUME");
                        // signal the output thread that RESUME has happened
                    }
                    pthread_mutex_unlock(&m_suspendresume_lock);
                }
                else
                {
                    DEBUG_PRINT("Resume not processed Mode[%d] state[%d] Resume[%d]",
                                pcm_feedback,state,bResumeEventRxed);
                }
            }
            break;

        case AUDIO_EVENT_WRITE_DONE:
            {
                if(pcm_feedback)
                {
                    tcxo_event.event_payload.aio_buf.data_len -= sizeof(META_IN);
                }

                /* Detect & free the Fake EOS buffer in Suspended State */
                if(bSuspendEventRxed)
                {
                    if(fake_in_eos_sent &&
                        tcxo_event.event_payload.aio_buf.data_len == 0)
                    {
                        ion_buf = (struct mmap_info*)tcxo_event.event_payload.
                            aio_buf.private_data;

                        if(ion_buf)
                        {
                            DEBUG_PRINT("\nWR_DONE FAKE EOS 0x%x,len=0\n",
                                (unsigned int)(ion_buf->pBuffer));

                            audio_ion_buf.fd = ion_buf->ion_fd;
                            audio_ion_buf.vaddr = ion_buf->pBuffer;
                            if(0 > ioctl(m_drv_fd, AUDIO_DEREGISTER_ION,
                                &audio_ion_buf))
                            {
                                DEBUG_PRINT("Error in ioctl AUDIO_DEREGISTER_ION\n");
                            }

                            free_ion_buffer((void**)&ion_buf);
                        }
                        else
                        {
                            DEBUG_PRINT_ERROR("ion_buf[NULL] in WRITE_DONE on SUSPEND\n");
                        }

                        if(fake_eos_recieved && bSuspendinprogress)
                        {
                                DEBUG_PRINT("Calling Audio_stop\n");
                                if(0 > ioctl(m_drv_fd, AUDIO_STOP, 0))
                                {
                                        DEBUG_PRINT("\n Error in ioctl AUDIO_STOP\n");
                                }
                                pthread_mutex_lock(&m_suspendresume_lock);
                                bSuspendinprogress = false;
                                pthread_mutex_unlock(&m_suspendresume_lock);

                                if(getWaitForSuspendCmplFlg())
                                {
                                        DEBUG_PRINT_ERROR("Release P-->Executing context to IL client.\n");
                                        release_wait_for_suspend();
                                }

                        }
                        fake_in_eos_ack_received = true;
                        break;
                    }
                }

                /* Normal handling of i/p buffers consumed by driver */
                bufHdr = (OMX_BUFFERHEADERTYPE*)tcxo_event.event_payload.
                    aio_buf.private_data;

                if(bufHdr)
                {

                    bufHdr->nFilledLen = tcxo_event.event_payload.
                        aio_buf.data_len;

                    DEBUG_PRINT("\n$$--ASYNC WRITE sent 0x%x with len = %u\n",
                        (unsigned)(bufHdr->pBuffer), (unsigned)(bufHdr->nFilledLen));

                    post_input((unsigned) &m_cmp, (unsigned int)bufHdr,
                        OMX_COMPONENT_GENERATE_BUFFER_DONE);
                    pthread_mutex_lock(&in_buf_count_lock);
                    drv_inp_buf_cnt--;
                    pthread_mutex_unlock(&in_buf_count_lock);
                }
                else
                {
                    DEBUG_PRINT("Invalid bufHdr[%p] in WRITE_DONE\n", bufHdr);
                }
            }
            break;

        case AUDIO_EVENT_READ_DONE:
            {
                tcxo_event.event_payload.aio_buf.data_len -= sizeof(META_OUT);

                /* Check whether i/p Fake EOS sent in Suspended State */
                if(bSuspendEventRxed)
                {
                    DEBUG_PRINT("\nREAD_DONE: In bSuspendEventRxed Loop\n");

                    if(!fake_eos_recieved)
                    {
                        /* EOS Buffer in Suspended State */
                        if(tcxo_event.event_payload.aio_buf.data_len == 0)
                        {
                            if(fake_in_eos_sent)
                            {
                                pthread_mutex_lock(&out_buf_count_lock);
                                if(!drv_out_buf_cnt && m_suspend_out_buf_cnt)
                                {
                                    pthread_mutex_unlock(&out_buf_count_lock);
                                    DEBUG_PRINT("\nRD_DONE: FakeEOS in SuspendBuf\n");
                                    DEBUG_PRINT("\nm_suspend_out_buf_cnt = %d\n", m_suspend_out_buf_cnt);
                                    ion_buf = (struct mmap_info*)tcxo_event.event_payload.aio_buf.
                                        private_data;
                                    if(ion_buf)
                                    {
                                        if( ion_buf == m_suspend_out_buf_list
                                            [ m_suspend_out_buf_cnt - 1] )
                                        {
                                            m_suspend_out_buf_list[m_suspend_out_buf_cnt - 1] = NULL;
                                            m_suspend_out_buf_cnt--;
                                        }
                                        else
                                        {
                                            DEBUG_PRINT("ion_buf mismatch with Suspend BufList\n");
                                        }

                                        DEBUG_PRINT("\n***ASYNC READ FAKE EOS 0x%x, len = 0\n",
                                           (unsigned)(ion_buf->pBuffer));

                                        audio_ion_buf.fd = ion_buf->ion_fd;
                                        audio_ion_buf.vaddr = ion_buf->pBuffer;
                                        if(0 > ioctl(m_drv_fd, AUDIO_DEREGISTER_ION,
                                            &audio_ion_buf))
                                        {
                                            DEBUG_PRINT("Error in ioctl AUDIO_DEREGISTER_ION\n");
                                        }

                                        free_ion_buffer((void**)&ion_buf);
                                    }
                                    else
                                    {
                                        DEBUG_PRINT_ERROR("ion_buf[NULL] in READ_DONE on SUSPEND\n");
                                    }
                                    breakflag = true;
                                }
                                else
                                {
                                    pthread_mutex_unlock(&out_buf_count_lock);
                                    DEBUG_PRINT("\nRD_DONE: FakeEOS in NormalBuf\n");
                                    DEBUG_PRINT("\n Fake o/p EOS rxd in normal buffer\n");

                                    breakflag = true;
                                    pthread_mutex_lock(&out_buf_count_lock);
                                    if(drv_out_buf_cnt > 0)
                                    {
                                        if(!m_suspend_out_drv_buf)
                                        {
                                            m_suspend_drv_buf_cnt = drv_out_buf_cnt;

                                            m_suspend_out_drv_buf = (struct msm_audio_aio_buf*)
                                                calloc(sizeof(struct msm_audio_aio_buf), drv_out_buf_cnt);

                                            if(!m_suspend_out_drv_buf)
                                            {
                                                DEBUG_PRINT_ERROR("\n calloc failed for m_suspend_out_drv_buf\n");
                                                this->m_cb.EventHandler(&this->m_cmp,
                                                        this->m_app_data,
                                                        OMX_EventError,
                                                        OMX_ErrorDynamicResourcesUnavailable,
                                                        0,
                                                        NULL);
                                                 m_suspend_drv_buf_cnt = 0;
                                                 breakflag = false;
                                            }
                                        }
                                        if(m_suspend_out_drv_buf)
                                        {
                                            drv_out_buf_cnt--;
                                            m_suspend_out_drv_buf[drv_out_buf_cnt] =
                                            tcxo_event.event_payload.aio_buf;
                                        }
                                    }
                                    pthread_mutex_unlock(&out_buf_count_lock);
                                }
                            }
                            else
                            {
                                pthread_mutex_lock(&out_buf_count_lock);
                                if(!drv_out_buf_cnt && m_suspend_out_buf_cnt)
                                {
                                    pthread_mutex_unlock(&out_buf_count_lock);
                                    DEBUG_PRINT("\nRD_DONE: NormalEOS in SuspendBuf\n");
                                    ion_buf = (struct mmap_info*)tcxo_event.event_payload.aio_buf.
                                        private_data;
                                    if(ion_buf)
                                    {
                                        ion_buf->filled_len = 0;
                                    }
                                    else
                                    {
                                        DEBUG_PRINT_ERROR("ion_buf[NULL] in READ_DONE on SUSPEND\n");
                                    }
                                    breakflag = true;
                                }
                                else
                                {
                                    pthread_mutex_unlock(&out_buf_count_lock);
                                    DEBUG_PRINT("\nRD_DONE: NormalEOS in NormalBuf\n");
                                    DEBUG_PRINT("\n Actual o/p EOS rxd in normal buffer\n");
                                }
                            }

                            DEBUG_PRINT("\n @@@@@@@@@@@@@@@@ FAKE o/p EOS @@@@@@@@@@@@@@@@\n");
                            fake_eos_recieved = true;


                            pthread_mutex_lock(&out_buf_count_lock);
                            if((drv_out_buf_cnt > 0 && fake_in_eos_sent) ||
                                ((drv_out_buf_cnt > 1 && !fake_in_eos_sent)))
                            {
                                DEBUG_PRINT("\n ~~~Pending o/p Buffers !!!!!\n");
                                DEBUG_PRINT("\n drv_out_buf_cnt = %d\n", drv_out_buf_cnt);
                                pthread_mutex_unlock(&out_buf_count_lock);
                                if(0 > ioctl(m_drv_fd, AUDIO_FLUSH, 0))
                                {
                                    DEBUG_PRINT_ERROR("\n Error in ioctl AUDIO_FLUSH\n");
                                }
                            }
                            else
                            {
                                pthread_mutex_unlock(&out_buf_count_lock);

                                if((fake_in_eos_sent && fake_in_eos_ack_received) ||
                                   (!fake_in_eos_sent))
                                {
                                        DEBUG_PRINT("o/p Fake EOS Rxd!! Calling Audio_stop\n");
                                        if(0 > ioctl(m_drv_fd, AUDIO_STOP, 0))
                                        {
                                                DEBUG_PRINT("\n Error in ioctl AUDIO_STOP\n");
                                        }
                                        pthread_mutex_lock(&m_suspendresume_lock);
                                        bSuspendinprogress = false;
                                        pthread_mutex_unlock(&m_suspendresume_lock);

                                        if(getWaitForSuspendCmplFlg())
                                        {
                                                DEBUG_PRINT_ERROR("Release P-->Executing context to IL client.\n");
                                                release_wait_for_suspend();
                                        }
                                }
                            }
                            pthread_mutex_lock(&m_out_th_lock_1);
                            if(is_out_th_sleep)
                            {
                                DEBUG_DETAIL("PE:WAKING UP OUT THREADS\n");
                                out_th_wakeup();
                                is_out_th_sleep = false;
                            }
                            pthread_mutex_unlock(&m_out_th_lock_1);
                        }
                        /* Finite length Buffer in Suspended State */
                        else
                        {
                            pthread_mutex_lock(&out_buf_count_lock);
                            if(drv_out_buf_cnt == 1)
                            {
                                pthread_mutex_unlock(&out_buf_count_lock);
                                if(!m_suspend_out_buf_cnt)
                                {
                                    DEBUG_PRINT("\n RD_DONE: SuspendBuf Alloc call on last buffer done\n");
                                    DEBUG_PRINT("\n RD_DONE: drv_out is 1 & suspend_cnt is 0\n");
                                    alloc_fill_suspend_out_buf();
                                }
                                else
                                {
                                    DEBUG_PRINT_ERROR("\n Error!! suspend_cnt = %d when drv_out is 1\n",
                                        m_suspend_out_buf_cnt);
                                    breakflag = true;
                                }
                            }
                            else if(!drv_out_buf_cnt)
                            {
                                pthread_mutex_unlock(&out_buf_count_lock);
                                if(m_suspend_out_buf_cnt)
                                {
                                    DEBUG_PRINT("\n Suspend buffer mode without Fake EOS\n");
                                    ion_buf = (struct mmap_info*)tcxo_event.event_payload.aio_buf.
                                        private_data;
                                    if(ion_buf)
                                    {
                                        ion_buf->filled_len = tcxo_event.event_payload.
                                            aio_buf.data_len;
                                    }
                                    else
                                    {
                                        DEBUG_PRINT("ion_buf[NULL] in READ_DONE on SUSPEND\n");
                                    }
                                    alloc_fill_suspend_out_buf();
                                }
                                else
                                {
                                    DEBUG_PRINT_ERROR("\n Error!! suspend_cnt is 0 when drv_out is 0\n");
                                }
                                breakflag = true;
                            }
                            else
                            {
                                DEBUG_PRINT("\nNormal flow, drv_out = %d\n", drv_out_buf_cnt);
                                pthread_mutex_unlock(&out_buf_count_lock);
                            }
                        }
                    }
                    else
                    {
                        /* Handling buffers flushed after Fake EOS rxd */
                        breakflag = true;
                        pthread_mutex_lock(&out_buf_count_lock);
                        if(drv_out_buf_cnt)
                        {
                            DEBUG_PRINT("\nAfter Fake EOS, buffers getting flushed = %d\n",
                                drv_out_buf_cnt);

                            if(!m_suspend_out_drv_buf)
                            {
                                DEBUG_PRINT("\n Allocate Suspend_List first time\n");
                                m_suspend_drv_buf_cnt = drv_out_buf_cnt;
                                m_suspend_out_drv_buf = (struct msm_audio_aio_buf*)
                                        calloc(sizeof(struct msm_audio_aio_buf), drv_out_buf_cnt);

                                if(!m_suspend_out_drv_buf)
                                {
                                        DEBUG_PRINT_ERROR("\n calloc failed for m_suspend_out_drv_buf\n");
                                        this->m_cb.EventHandler(&this->m_cmp,
                                                this->m_app_data,
                                                OMX_EventError,
                                                OMX_ErrorDynamicResourcesUnavailable,
                                                0,
                                                NULL);
                                        m_suspend_drv_buf_cnt = 0;
                                        breakflag = false;
                                }
                            }

                            if(m_suspend_out_drv_buf)
                            {
                                drv_out_buf_cnt--;
                                m_suspend_out_drv_buf[drv_out_buf_cnt] =
                                       tcxo_event.event_payload.aio_buf;
                            }
                            if(!drv_out_buf_cnt || !breakflag)
                            {
                                DEBUG_PRINT("Suspend After output EOS no more buffers left\n");
                                if((fake_in_eos_sent && fake_in_eos_ack_received) ||
                                   (!fake_in_eos_sent))
                                {
                                        DEBUG_PRINT("Calling Audio_stop\n");
                                        if(0 > ioctl(m_drv_fd, AUDIO_STOP, 0))
                                        {
                                                DEBUG_PRINT("\n Error in ioctl AUDIO_STOP\n");
                                        }

                                        pthread_mutex_lock(&m_suspendresume_lock);
                                        bSuspendinprogress = false;
                                        pthread_mutex_unlock(&m_suspendresume_lock);

                                        if(getWaitForSuspendCmplFlg())
                                        {
                                                DEBUG_PRINT_ERROR("Release P-->Executing context to IL client.\n");
                                                release_wait_for_suspend();
                                        }
                                }
                            }
                        }
                       pthread_mutex_unlock(&out_buf_count_lock);
                    }

                    if(breakflag)
                    {
                        DEBUG_PRINT("\nbreakflag is set\n");
                        breakflag = false;
                        break;
                    }
                }

                bufHdr = (OMX_BUFFERHEADERTYPE*)tcxo_event.event_payload.
                    aio_buf.private_data;

                if(bufHdr)
                {
                    bufHdr->nFilledLen = tcxo_event.event_payload.
                        aio_buf.data_len;
                    pmeta_out = (META_OUT*) (bufHdr->pBuffer - sizeof(META_OUT));
                    if(!pmeta_out)
                    {
                        DEBUG_PRINT_ERROR("\n Invalid pmeta_out(NULL)\n");
                        return;
                    }

                    if ((bufHdr->nFilledLen == 0) && (pmeta_out->nFlags & OMX_BUFFERFLAG_EOS))
                    {
                        DEBUG_PRINT("EOS Flag set in o/p Buffer");
                        bufHdr->nFlags |= pmeta_out->nFlags;
                    }
                    else
                    {
                        if(pmeta_out->nFlags & OMX_BUFFERFLAG_EOS)
                        {
                            DEBUG_PRINT("Ignore PCM+EOS...");
                        }

                        bufHdr->nFlags &= ~OMX_BUFFERFLAG_EOS;
                    }
                    DEBUG_PRINT("\n@@--ASYNC Read gives 0x%x with len = %u\n",
                       (unsigned) (bufHdr->pBuffer),(unsigned) (bufHdr->nFilledLen));

                    pthread_mutex_lock(&out_buf_count_lock);
                    drv_out_buf_cnt--;
                    pthread_mutex_unlock(&out_buf_count_lock);

                    post_output((unsigned) &m_cmp, (unsigned int)bufHdr,
                        OMX_COMPONENT_GENERATE_FRAME_DONE);

                }
                else
                {
                    DEBUG_PRINT("Invalid bufHdr[%p] in READ_DONE\n", bufHdr);
                }
            }
            break;

        default:
            DEBUG_PRINT("PE: Recieved Invalid Event");
            break;
        }
    }
    return;
}
void omx_mp3_adec::wait_for_event()
{
	sem_wait(&sem_th_state);
}

void omx_mp3_adec::event_complete()
{
	sem_post(&sem_th_state);
}
void omx_mp3_adec::in_th_goto_sleep()
{
    pthread_mutex_lock(&m_in_th_lock);
    while (m_is_in_th_sleep == 0)
    {
        pthread_cond_wait(&in_cond, &m_in_th_lock);
    }
    m_is_in_th_sleep = 0;
    pthread_mutex_unlock(&m_in_th_lock);
}

void omx_mp3_adec::in_th_wakeup()
{
    pthread_mutex_lock(&m_in_th_lock);
    if (m_is_in_th_sleep == 0) {
        m_is_in_th_sleep = 1;
        pthread_cond_signal(&in_cond);
    }
    pthread_mutex_unlock(&m_in_th_lock);
}

void omx_mp3_adec::out_th_goto_sleep()
{
    pthread_mutex_lock(&m_out_th_lock);
    while (m_is_out_th_sleep == 0)
    {
        pthread_cond_wait(&out_cond, &m_out_th_lock);
    }
    m_is_out_th_sleep = 0;
    pthread_mutex_unlock(&m_out_th_lock);
}

void omx_mp3_adec::out_th_wakeup()
{
    pthread_mutex_lock(&m_out_th_lock);
    if (m_is_out_th_sleep == 0) {
        m_is_out_th_sleep = 1;
        pthread_cond_signal(&out_cond);
    }
    pthread_mutex_unlock(&m_out_th_lock);
}

void omx_mp3_adec::wait_for_flush_event()
{
    pthread_mutex_lock(&m_flush_cmpl_lock);
    m_flush_cmpl_event = 1;
    pthread_mutex_unlock(&m_flush_cmpl_lock);
    sem_wait(&sem_flush_cmpl_state);
}

void omx_mp3_adec::event_flush_complete()
{
    pthread_mutex_lock(&m_flush_cmpl_lock);
    if(m_flush_cmpl_event == 1){
	sem_post(&sem_flush_cmpl_state);
        m_flush_cmpl_event = 0;
    }
    pthread_mutex_unlock(&m_flush_cmpl_lock);
}

/* ======================================================================
FUNCTION
  omx_mp3_adec::omx_mp3_adec

DESCRIPTION
  Constructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_mp3_adec::omx_mp3_adec(): m_state(OMX_StateInvalid),
                              m_app_data(NULL),
		   	      m_timer(NULL),
                              m_first_mp3_header(0),
                              m_suspend_out_buf_list(NULL),
                              m_suspend_out_drv_buf(NULL),
                              m_suspend_drv_buf_cnt(0),
                              m_suspend_out_buf_cnt(0),
                              m_resume_out_buf_cnt(0),
                              drv_inp_buf_cnt(0),
                              drv_out_buf_cnt(0),
                              m_is_alloc_buf(0),
                              m_drv_fd(-1),
                              fake_in_eos_ack_received(false),
                              fake_in_eos_sent(false),
                              is_in_th_sleep(false),
                              is_out_th_sleep(false),
			      m_to_idle(false),
                              m_pause_to_exe(false),
                              waitForSuspendCmplFlg(false),
                              m_inp_act_buf_count (OMX_CORE_NUM_INPUT_BUFFERS),
                              m_out_act_buf_count (OMX_CORE_NUM_OUTPUT_BUFFERS),
                              m_inp_bEnabled(OMX_TRUE),
                              m_out_bEnabled(OMX_TRUE),
                              m_inp_bPopulated(OMX_FALSE),
                              m_out_bPopulated(OMX_FALSE),
                              m_inp_current_buf_count(0),
                              m_out_current_buf_count(0),
                              m_comp_deinit(0),
                              m_flags(0),
                              nTimestamp(0),
                              nLastTimeStamp(0),
                              output_buffer_size (OMX_MP3_OUTPUT_BUFFER_SIZE),
                              input_buffer_size (OMX_CORE_INPUT_BUFFER_SIZE),
                              m_odd_byte(0),
                              m_odd_byte_set(false),
			      m_flush_cmpl_event(0),
                              Omx_fillbufcnt(0),
                              set_pcm_config(0),
			      m_in_use_buf_case(false),
                              m_out_use_buf_case(false),
                              m_input_eos_rxd(false),
                              m_output_eos_rxd(false),
			      bGenerateEOSPending(false),
			      m_ipc_to_in_th(NULL),
                              m_ipc_to_out_th(NULL),
                              m_ipc_to_cmd_th(NULL),
			      m_ipc_to_event_th(NULL),
                              mime_type(NULL),
                              ion_fd(0)
{
    int cond_ret = 0;
    memset(&m_cmp,       0,     sizeof(m_cmp));
    memset(&m_cb,        0,      sizeof(m_cb));

    pthread_mutexattr_init(&m_inputlock_attr);
    pthread_mutex_init(&m_inputlock, &m_inputlock_attr);

    pthread_mutexattr_init(&m_state_lock_attr);
    pthread_mutex_init(&m_state_lock, &m_state_lock_attr);

    pthread_mutexattr_init(&m_commandlock_attr);
    pthread_mutex_init(&m_commandlock, &m_commandlock_attr);
    pthread_mutexattr_init(&m_outputlock_attr);
    pthread_mutex_init(&m_outputlock, &m_outputlock_attr);

    pthread_mutexattr_init(&m_seq_attr);
    pthread_mutex_init(&m_seq_lock, &m_seq_attr);

    pthread_mutexattr_init(&m_flush_attr);
    pthread_mutex_init(&m_flush_lock, &m_flush_attr);

    pthread_mutexattr_init(&m_in_th_attr);
    pthread_mutex_init(&m_in_th_lock, &m_in_th_attr);

    pthread_mutexattr_init(&m_out_th_attr);
    pthread_mutex_init(&m_out_th_lock, &m_out_th_attr);

    pthread_mutexattr_init(&m_in_th_attr_1);
    pthread_mutex_init(&m_in_th_lock_1, &m_in_th_attr_1);

    pthread_mutexattr_init(&m_out_th_attr_1);
    pthread_mutex_init(&m_out_th_lock_1, &m_out_th_attr_1);

    pthread_mutexattr_init(&out_buf_count_lock_attr);
    pthread_mutex_init(&out_buf_count_lock, &out_buf_count_lock_attr);

    pthread_mutexattr_init(&m_flush_cmpl_attr);
    pthread_mutex_init(&m_flush_cmpl_lock, &m_flush_cmpl_attr);

    pthread_mutexattr_init(&in_buf_count_lock_attr);
    pthread_mutex_init(&in_buf_count_lock, &in_buf_count_lock_attr);
    pthread_mutexattr_init(&m_suspendresume_lock_attr);
    pthread_mutex_init(&m_suspendresume_lock, &m_suspendresume_lock_attr);
    pthread_mutexattr_init(&m_WaitForSuspendCmpl_lock_attr);
    pthread_mutex_init(&m_WaitForSuspendCmpl_lock, &m_WaitForSuspendCmpl_lock_attr);

    if ((cond_ret = pthread_cond_init (&in_cond, NULL)) != 0)
    {
        DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for in_cond\n");
        if (cond_ret == EAGAIN)
            DEBUG_PRINT_ERROR("The system lacked necessary resources(other than mem)\n");
        else if (cond_ret == ENOMEM)
            DEBUG_PRINT_ERROR("Insufficient memory to initialise condition variable\n");
    }

    if ((cond_ret = pthread_cond_init (&out_cond, NULL)) != 0)
    {
        DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for out_cond\n");
        if (cond_ret == EAGAIN)
            DEBUG_PRINT_ERROR("The system lacked necessary resources(other than mem)\n");
        else if (cond_ret == ENOMEM)
            DEBUG_PRINT_ERROR("Insufficient memory to initialise condition variable\n");
    }

    sem_init(&sem_read_msg,0, 0);
    sem_init(&sem_write_msg,0, 0);
    sem_init(&sem_States,0, 0);
    sem_init(&sem_th_state, 0, 0);
    sem_init(&sem_flush_cmpl_state, 0, 0);
    sem_init(&sem_WaitForSuspendCmpl_states,0, 0);
    m_comp_deinit = 0;
    m_timer = new timer(this);
  if(m_timer == NULL)
  {
      DEBUG_PRINT_ERROR("Not able to allocate memory for timer obj\n");
  }
  DEBUG_PRINT("%s %p\n",__FUNCTION__,this);

  ion_fd = open("/dev/ion", O_RDONLY);
  if (ion_fd < 0) {
      DEBUG_PRINT_ERROR("/dev/ion open failed \n");
  }
    return;
}


/* ======================================================================
FUNCTION
  omx_mp3_adec::~omx_mp3_adec

DESCRIPTION
  Destructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_mp3_adec::~omx_mp3_adec()
{
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    /* If component deinit is not happend, do it now before destroying */
    if ( !m_comp_deinit )
    {
        deinit_decoder();
    }
    if (mime_type)
    {
        free(mime_type);
        mime_type = NULL;
    }
    pthread_mutex_destroy(&m_inputlock);

    pthread_mutex_destroy(&m_outputlock);
    pthread_mutexattr_destroy(&m_suspendresume_lock_attr);

    pthread_mutex_destroy(&m_suspendresume_lock);

    pthread_mutex_destroy(&m_state_lock);

    pthread_mutex_destroy(&m_commandlock);

    pthread_mutex_destroy(&m_in_th_lock);

    pthread_mutex_destroy(&m_out_th_lock);

    pthread_mutex_destroy(&m_in_th_lock_1);

    pthread_mutex_destroy(&m_out_th_lock_1);

    pthread_mutex_destroy(&out_buf_count_lock);

    pthread_mutex_destroy(&m_flush_cmpl_lock);

    pthread_mutexattr_destroy(&m_seq_attr);
    pthread_mutex_destroy(&m_seq_lock);

    pthread_mutex_destroy(&in_buf_count_lock);
    pthread_mutex_destroy(&m_WaitForSuspendCmpl_lock);
    pthread_mutexattr_destroy(&m_WaitForSuspendCmpl_lock_attr);
    pthread_mutexattr_destroy(&out_buf_count_lock_attr);
    pthread_mutexattr_destroy(&in_buf_count_lock_attr);
    pthread_cond_destroy(&in_cond);
    pthread_cond_destroy(&out_cond);

    sem_destroy (&sem_read_msg);
    sem_destroy (&sem_write_msg);
    sem_destroy (&sem_States);
    sem_destroy (&sem_th_state);
    sem_destroy (&sem_flush_cmpl_state);
    sem_destroy (&sem_WaitForSuspendCmpl_states);

    if (ion_fd >= 0) {
        close(ion_fd);
        ion_fd = -1;
    }

    DEBUG_PRINT_ERROR("OMX MP3 component destroyed\n");
    return;
}

/**
  @brief memory function for sending EmptyBufferDone event
   back to IL client

  @param bufHdr OMX buffer header to be passed back to IL client
  @return none
 */
void omx_mp3_adec::buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{
    OMX_BUFFERHEADERTYPE *buffer = bufHdr;
    META_IN *pmeta_in = NULL;
    msm_audio_aio_buf audio_aio_buf;
    bool is_finite_eos_buffer = false;

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    DEBUG_PRINT("EBD::Callback drvin[%d] flush[%d]drvout[%d] out[%d] in[%d]",
						drv_inp_buf_cnt, bFlushinprogress, drv_out_buf_cnt,
						nNumOutputBuf, nNumInputBuf);

    if(m_in_use_buf_case && (m_flush_inbuf == false))
    {
        buffer = m_loc_in_use_buf_hdrs.find(bufHdr);

        if(!buffer)
        {
            DEBUG_PRINT("BDC::UseBufhdr for buffer[%p] is NULL", bufHdr);
            return;
        }

        buffer->nFlags = bufHdr->nFlags;
        buffer->nFilledLen = bufHdr->nFilledLen;
    }

    if((bufHdr->nFlags & OMX_BUFFERFLAG_EOS) && (bufHdr->nFilledLen))
    {
        DEBUG_PRINT("\nBDC: Finite EOS Buffer emptied\n");
        is_finite_eos_buffer = true;
        if(m_odd_byte_set)
        {
            /* For odd byte in last buffer, set the Filled Length to 2 */
            bufHdr->nFilledLen = 2;
            *(bufHdr->pBuffer) = m_odd_byte;
            *(bufHdr->pBuffer+1) = 0;
            m_odd_byte_set = false;
        }
        else
        {
            /* Set the Filled Length to 0 & EOS flag for the Buffer Header */
            bufHdr->nFilledLen = 0;
            m_input_eos_rxd = true;
        }

        if(!pcm_feedback && bufHdr->nFilledLen == 0)
        {
            DEBUG_PRINT("\nBDC: EOS with 0 length in Tunneled Mode\n");
            is_finite_eos_buffer = false;
        }
        else
        {
            if (pcm_feedback)//Non-Tunnelled mode
            {
                pmeta_in = (META_IN*) (bufHdr->pBuffer - sizeof(META_IN));
                if(pmeta_in)
                {
                    // copy the metadata info from the BufHdr and insert to payload
                    pmeta_in->offsetVal  = sizeof(META_IN);
                    pmeta_in->nTimeStamp = (((OMX_BUFFERHEADERTYPE*)bufHdr)->nTimeStamp);

                    if(bufHdr->nFilledLen)
                    {
                        pmeta_in->nFlags &= ~OMX_BUFFERFLAG_EOS;
                    }
                    else
                    {
                        pmeta_in->nFlags = bufHdr->nFlags;
                    }
                }
                else
                {
                    DEBUG_PRINT_ERROR("\n Invalid pmeta_in(NULL)\n");
                    return;
                }
            }
            /* Asynchronous write call to the MP3 driver */
            audio_aio_buf.buf_len = bufHdr->nAllocLen;
            audio_aio_buf.private_data = bufHdr;

            if(pcm_feedback)
            {
                audio_aio_buf.buf_addr = (OMX_U8*)pmeta_in;
                audio_aio_buf.data_len = bufHdr->nFilledLen + sizeof(META_IN);
                audio_aio_buf.mfield_sz = sizeof(META_IN);
            }
            else
            {
                audio_aio_buf.data_len = bufHdr->nFilledLen;
                audio_aio_buf.buf_addr = bufHdr->pBuffer;
            }

            pthread_mutex_lock(&in_buf_count_lock);
            drv_inp_buf_cnt++;
            DEBUG_PRINT("\nBDC:: i/p Buffers with MP3 drv = %d\n", drv_inp_buf_cnt);
            pthread_mutex_unlock(&in_buf_count_lock);

            DEBUG_PRINT ("\nBDC:: Calling MP3 ASYNC_WRITE ...");
            if(0 > ioctl(m_drv_fd, AUDIO_ASYNC_WRITE, &audio_aio_buf))
            {
                DEBUG_PRINT("\nBDC: Error in ASYNC WRITE call\n");
                pthread_mutex_lock(&in_buf_count_lock);
                drv_inp_buf_cnt--;
                pthread_mutex_unlock(&in_buf_count_lock);
                return;
            }
            DEBUG_PRINT("\n AUDIO_ASYNC_WRITE issued for EOS buf len = %u\n", (unsigned)(bufHdr->nFilledLen));
        }
    }

    if(is_finite_eos_buffer == false)
    {
        if(m_cb.EmptyBufferDone)
        {
            pthread_mutex_lock(&in_buf_count_lock);
            nNumInputBuf--;
            DEBUG_PRINT("BDC : nNumInputBuf = %d nflags =%u \n", nNumInputBuf,
               (unsigned)(bufHdr->nFlags));
            pthread_mutex_unlock(&in_buf_count_lock);


            buffer->nFilledLen = 0; /* consumed all input samples */
            m_cb.EmptyBufferDone(&m_cmp, m_app_data, buffer);

            if(!pcm_feedback && (m_input_eos_rxd == true) && nNumInputBuf == 0)
            {
                DEBUG_PRINT("\n Calling fsync() in Tunneled Mode ...\n");
                fsync(m_drv_fd);
                post_input((unsigned) & m_cmp,(unsigned) buffer,
                    OMX_COMPONENT_GENERATE_EOS);
            }
        }
        PrintFrameHdr(OMX_COMPONENT_GENERATE_BUFFER_DONE, buffer);
    }
}

void omx_mp3_adec::flush_ack()
{
    int lcnt = 0;
    pthread_mutex_lock(&m_flush_lock);
    --m_flush_cnt;
    lcnt = m_flush_cnt;
    DEBUG_PRINT("%s[%p] flushcnt[%d]\n",__FUNCTION__,this, lcnt);
    pthread_mutex_unlock(&m_flush_lock);
    if( lcnt == 0 )
    {
        event_complete();
	event_flush_complete();
    }
}
void omx_mp3_adec::frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{
    OMX_BUFFERHEADERTYPE *buffer = bufHdr;
    OMX_BUFFERHEADERTYPE *tmpbuffer = bufHdr;
    META_OUT *pmeta_out = NULL;
    OMX_STATETYPE state;

    DEBUG_PRINT("%s[%p] drvin[%d] flush[%d]drvout[%d] out[%d] in[%d]",
                                                 __FUNCTION__, this, 
						drv_inp_buf_cnt, bFlushinprogress, drv_out_buf_cnt,
						nNumOutputBuf, nNumInputBuf);

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(m_out_use_buf_case && (m_flush_outbuf == true))
    {
        tmpbuffer = m_loc_out_use_buf_hdrs.find(bufHdr);
	pmeta_out = (META_OUT*) (tmpbuffer->pBuffer - sizeof(META_OUT));
    } else
	pmeta_out = (META_OUT*) (bufHdr->pBuffer - sizeof(META_OUT));
    if(!pmeta_out)
    {
        DEBUG_PRINT_ERROR("\n Invalid pmeta_out(NULL)\n");
        return;
    }

    bufHdr->nTimeStamp = (pmeta_out->nTimeStamp);
    nTimestamp = bufHdr->nTimeStamp;
    // copy the pcm frame to the client buffer
    if(bufHdr->nFilledLen > bufHdr->nAllocLen)
    {
        DEBUG_PRINT_ERROR("FDC: Invalid Data! FilledLen[%u] > AllocLen[%u]",
            (unsigned)(bufHdr->nFilledLen), (unsigned)(bufHdr->nAllocLen));
        bufHdr->nFilledLen=0;
    }

    if(m_out_use_buf_case && (m_flush_outbuf == false))
    {
        buffer = m_loc_out_use_buf_hdrs.find(bufHdr);

        if(!buffer)
        {
            DEBUG_PRINT("FDC: UseBufhdr for buffer[%p] is NULL", bufHdr);
            return;
        }

        buffer->nFilledLen = bufHdr->nFilledLen;
        buffer->nFlags = bufHdr->nFlags;
        buffer->nTimeStamp = bufHdr->nTimeStamp;
        if(m_fbd_cnt == 0)
        {
            buffer->nFilledLen = bufHdr->nFilledLen - MP3_DECODER_DELAY * m_adec_param.nChannels;
            buffer->nOffset = MP3_DECODER_DELAY * m_adec_param.nChannels;
            DEBUG_DETAIL("FDC: First buffer offset is %d\n", buffer->nOffset);
        }
        memcpy(buffer->pBuffer, bufHdr->pBuffer, bufHdr->nFilledLen);
    }

    if (!m_output_eos_rxd)
    {
        if(bufHdr->nFlags & OMX_BUFFERFLAG_EOS)
        {
            get_state(&m_cmp, &state);
            if((state == OMX_StatePause) &&
               (suspensionPolicy == OMX_SuspensionEnabled)
                && (bSuspendEventRxed == true))
            {
                unsigned int p1 = 0; /* Parameter - 1 */
                unsigned int p2 = 0; /* Parameter - 2 */
                unsigned int id = 0;
                bool ret = false;
                fake_eos_recieved = true;

                /* Faked EOS recieved, no processing of this mesg is reqd */
                ret = m_output_ctrl_q.get_msg_id(&id);
                if(ret && (id == OMX_COMPONENT_SUSPEND))
                {
                    m_output_ctrl_q.pop_entry(&p1, &p2, &id);
                }

                pthread_mutex_lock(&out_buf_count_lock);
                if(drv_out_buf_cnt)
                {
                    DEBUG_PRINT("\nFDC: FakeEOS! Drv Flush called, drv_out = %d\n", drv_out_buf_cnt);
                    pthread_mutex_unlock(&out_buf_count_lock);
                    if(0 > ioctl(m_drv_fd, AUDIO_FLUSH, 0))
                    {
                        DEBUG_PRINT_ERROR("\n Error in ioctl AUDIO_FLUSH\n");
                    }
                }
                else
                {
                    pthread_mutex_unlock(&out_buf_count_lock);
                    pthread_mutex_lock(&m_suspendresume_lock);
                    if(bSuspendinprogress)
                    {
                         pthread_mutex_unlock(&m_suspendresume_lock);
                         DEBUG_PRINT("\nFDC: FakeEOS! Drv Stop called\n");
                         if(0 > ioctl(m_drv_fd, AUDIO_STOP, 0))
                         {
                                 DEBUG_PRINT("\n Error in ioctl AUDIO_STOP\n");
                                 return;
                         }

                         pthread_mutex_lock(&m_suspendresume_lock);
                         bSuspendinprogress = false;
                         pthread_mutex_unlock(&m_suspendresume_lock);

                         if (getWaitForSuspendCmplFlg())
                         {
                                 DEBUG_PRINT_ERROR("Release P-->Executing context to IL client.\n");
                                 release_wait_for_suspend();
                         }
                    }
                    else
                    {
                       DEBUG_PRINT("\nFDC: FakeEOS! Drv Stop Already done\n");
                       pthread_mutex_unlock(&m_suspendresume_lock);
                    }
                }

                DEBUG_PRINT("FDC: Actual EOS reached, ctrlq=%d fake_eos = %d",
                    m_output_ctrl_q.m_size, fake_eos_recieved);
            }

            DEBUG_PRINT("FDC: END OF STREAM \n");
            m_output_eos_rxd = true;
	    if (!bFlushinprogress)
		    post_output(0, (unsigned)buffer, OMX_COMPONENT_GENERATE_EOS);
	    else
		    bGenerateEOSPending = true;
        }
        else
        {
            DEBUG_PRINT("FDC:VALID DATA LENGTH datalen=%u nflags=%u ts[%u] \
                        tot_time[%u] bufferhdr [%x]\n",(unsigned)( bufHdr->nFilledLen),
                       (unsigned)( bufHdr->nFlags),(unsigned) nTimestamp, (unsigned)ntotal_playtime,(unsigned) bufHdr);
        }
    }

    if(m_output_eos_rxd)
    {
        buffer->nFilledLen = MP3_DECODER_DELAY * m_adec_param.nChannels;
        buffer->nOffset = 0;
        memset(buffer->pBuffer, 0, buffer->nFilledLen);
    }
    if(m_cb.FillBufferDone)
    {
        pthread_mutex_lock(&out_buf_count_lock);
        m_fbd_cnt++;
        --nNumOutputBuf;
        DEBUG_PRINT("FDC:Cnt[%d] NumOutBuf[%d] \n",
            m_fbd_cnt, nNumOutputBuf);
        pthread_mutex_unlock(&out_buf_count_lock);

        m_cb.FillBufferDone(&m_cmp, m_app_data, buffer);
        PrintFrameHdr(OMX_COMPONENT_GENERATE_FRAME_DONE, buffer);
    }

    return;
}

/** ======================================================================
 @brief static member function for handling all commands from IL client

  IL client commands are processed and callbacks are generated
  through this routine. Audio Command Server provides the thread context
  for this routine.

  @param client_data pointer to decoder component
  @param id command identifier
  @return none
 */

void omx_mp3_adec::process_out_port_msg(void *client_data, unsigned char id)
{
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned      ident;
    unsigned     qsize  = 0; // qsize
    unsigned      tot_qsize = 0;
    omx_mp3_adec  *pThis = (omx_mp3_adec *) client_data;
    OMX_STATETYPE state;

DEBUG_PRINT("%s %p\n",__FUNCTION__,pThis);
loopback_out:
    pthread_mutex_lock(&pThis->m_seq_lock);
    pthread_mutex_lock(&pThis->m_state_lock);
    pThis->get_state(&pThis->m_cmp, &state);
    pthread_mutex_unlock(&pThis->m_state_lock);

    if(state == OMX_StateLoaded)
    {
        pthread_mutex_unlock(&pThis->m_seq_lock);
        DEBUG_PRINT(" OUT: IN LOADED STATE RETURN\n");
        return;
    }

    pthread_mutex_lock(&pThis->m_outputlock);

    qsize = pThis->m_output_ctrl_cmd_q.m_size;
    tot_qsize = pThis->m_output_ctrl_cmd_q.m_size;
    tot_qsize += pThis->m_output_ctrl_fbd_q.m_size;
    tot_qsize += pThis->m_output_q.m_size;
    tot_qsize += pThis->m_output_ctrl_q.m_size;

    DEBUG_DETAIL("OUT-->QSIZE-flush=%d,cmdq[%d] fbd=%d QSIZE=%d state=%d flush=%d to_idle=%d\n",\
        pThis->m_output_ctrl_cmd_q.m_size,
        pThis->m_output_ctrl_q.m_size,
        pThis->m_output_ctrl_fbd_q.m_size,
        pThis->m_output_q.m_size,state, pThis->bFlushinprogress,pThis->m_to_idle);

    if(tot_qsize ==0) {
        pthread_mutex_unlock(&pThis->m_outputlock);
        pthread_mutex_unlock(&pThis->m_seq_lock);
        DEBUG_DETAIL("OUT-->BREAK FROM LOOP...%d\n",tot_qsize);
        return;
    }

    if((state == OMX_StatePause) && !(pThis->m_output_ctrl_cmd_q.m_size)&& 
       (!pThis->m_output_ctrl_q.m_size) && !pThis->bFlushinprogress && !pThis->m_to_idle )
    {
        pthread_mutex_unlock(&pThis->m_outputlock);

        DEBUG_PRINT("OUT:1.SLEEPING OUT THREAD\n");
        pthread_mutex_lock(&pThis->m_out_th_lock_1);
        pThis->is_out_th_sleep = true;
        pthread_mutex_unlock(&pThis->m_out_th_lock_1);
        pthread_mutex_unlock(&pThis->m_seq_lock);
        pThis->out_th_goto_sleep();
        goto loopback_out;
    }
    else if((!pThis->bOutputPortReEnabled) && (!pThis->m_output_ctrl_cmd_q.m_size))
    {
        // case where no port reconfig and nothing in the flush q
        DEBUG_DETAIL("No flush/port reconfig qsize=%d tot_qsize=%d",\
            qsize,tot_qsize);
        pthread_mutex_unlock(&pThis->m_outputlock);
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
        if(state == OMX_StateLoaded)
        {
            pthread_mutex_unlock(&pThis->m_seq_lock);
            return;
	}

        if(pThis->m_output_ctrl_cmd_q.m_size || !(pThis->bFlushinprogress))
        {
            //race condition, hence check again
            pthread_mutex_lock(&pThis->m_out_th_lock_1);
            pThis->is_out_th_sleep = true;
            pthread_mutex_unlock(&pThis->m_out_th_lock_1);
            DEBUG_PRINT("OUT:2. SLEEPING OUT THREAD \n");
            pthread_mutex_unlock(&pThis->m_seq_lock);
            pThis->out_th_goto_sleep();
        }
        else
        {
            pthread_mutex_unlock(&pThis->m_seq_lock);
        }
        goto loopback_out;
    }
    else if(state == OMX_StatePause)
    {
        DEBUG_PRINT ("\n OUT Thread in the pause state");
        if(pThis->m_output_ctrl_q.m_size)
        {
            unsigned int indx; bool ret = false;
            qsize=pThis->m_output_ctrl_q.m_size;
            if(qsize)
            {
                ret = pThis->m_output_ctrl_q.get_msg_id(&indx);
                DEBUG_PRINT("OUT: suspend/resume id=%x suspendevent=%d fake_eos=%d",indx,
                    pThis->bSuspendEventRxed,pThis->fake_eos_recieved);
                if(ret && (indx == OMX_COMPONENT_SUSPEND))
                {
                    DEBUG_PRINT("NEED TO PROCESS SUSPEND EVENT");
                }
                else if((indx == OMX_COMPONENT_RESUME) && ret)
                {
                   if(pThis->bSuspendEventRxed && pThis->fake_eos_recieved)
                   {
                        DEBUG_PRINT("NEED TO PROCESS RESUME EVENT");
                   }
                   else
                   {
                       DEBUG_PRINT_ERROR("\n Early Resume ! Fake EOS not Rxd\n");
                       pthread_mutex_lock(&pThis->m_out_th_lock_1);
                       DEBUG_DETAIL("OUT: SLEEPING OUT THREAD\n");
                       pThis->is_out_th_sleep = true;
                       pthread_mutex_unlock(&pThis->m_out_th_lock_1);
                       pthread_mutex_unlock(&pThis->m_outputlock);
                       pthread_mutex_unlock(&pThis->m_seq_lock);
                       pThis->out_th_goto_sleep();
                       pthread_mutex_lock(&pThis->m_seq_lock);
                       pthread_mutex_lock(&pThis->m_outputlock);
                   }
                }
            }
	}
	else
        {
            DEBUG_PRINT("OUT: pause state =%d \n", state);
            if (!pThis->bFlushinprogress)
            {
                if(pThis->bSuspendEventRxed && !pThis->bResumeEventRxed)
                {
                    DEBUG_DETAIL("OUT: SLEEPING OUT THREAD\n");
                    pthread_mutex_lock(&pThis->m_out_th_lock_1);
                    pThis->is_out_th_sleep = true;
                    pthread_mutex_unlock(&pThis->m_out_th_lock_1);
                    pthread_mutex_unlock(&pThis->m_outputlock);
                    pthread_mutex_unlock(&pThis->m_seq_lock);
                    pThis->out_th_goto_sleep();
                    pthread_mutex_lock(&pThis->m_seq_lock);
                    pthread_mutex_lock(&pThis->m_outputlock);
                }
            }
        }
    }

    qsize = pThis->m_output_ctrl_cmd_q.m_size;
    if (qsize)
    {
        // process FLUSH message
        pThis->m_output_ctrl_cmd_q.pop_entry(&p1,&p2,&ident);
    }
    else if((qsize = pThis->m_output_ctrl_fbd_q.m_size) &&
        (pThis->bOutputPortReEnabled) && ((state == OMX_StateExecuting)|| pThis->m_to_idle))
    {
        // then process EBD's
        pThis->m_output_ctrl_fbd_q.pop_entry(&p1,&p2,&ident);
    }
    else if((qsize = pThis->m_output_q.m_size) && !pThis->bFlushinprogress &&
        (pThis->bOutputPortReEnabled) && (state == OMX_StateExecuting))
    {
        // if no FLUSH and FBD's then process FTB's
        pThis->m_output_q.pop_entry(&p1,&p2,&ident);
    }
    else if ((qsize = pThis->m_output_ctrl_q.m_size) && 
             ((state == OMX_StatePause) || (state == OMX_StateExecuting)))
    {
       pThis->m_output_ctrl_q.pop_entry(&p1,&p2,&ident);
    }
    else if(state == OMX_StateLoaded)
    {
        pthread_mutex_unlock(&pThis->m_outputlock);
        pthread_mutex_unlock(&pThis->m_seq_lock);
        DEBUG_PRINT("IN: ***in OMX_StateLoaded so exiting\n");
        return ;
    }
    else
    {
        qsize = 0;
        pthread_mutex_unlock(&pThis->m_outputlock);
        pthread_mutex_unlock(&pThis->m_seq_lock);
        goto loopback_out;
    }

    pthread_mutex_unlock(&pThis->m_outputlock);
    pthread_mutex_unlock(&pThis->m_seq_lock);

    if(qsize > 0)
    {
        id = ident;
        DEBUG_DETAIL("OUT->state[%d]ident[%d]flushq[%d]ctrlq[%d] fbd[%d]dataq[%d]\n",\
            pThis->m_state,
            ident,
            pThis->m_output_ctrl_cmd_q.m_size,
            pThis->m_output_ctrl_q.m_size,
            pThis->m_output_ctrl_fbd_q.m_size,
            pThis->m_output_q.m_size);

        if(id == OMX_COMPONENT_GENERATE_FRAME_DONE)
        {
            pThis->frame_done_cb((OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_FTB)
        {
            pThis->fill_this_buffer_proxy((OMX_HANDLETYPE)p1,
                (OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_EOS)
        {
            pThis->m_cb.EventHandler(&pThis->m_cmp,
                pThis->m_app_data,
                OMX_EventBufferFlag,
                1, 1, NULL );
        }
        else if(id == OMX_COMPONENT_PORTSETTINGS_CHANGED)
        {
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                OMX_EventPortSettingsChanged, 1, 0, NULL );
        }
        else if(id == OMX_COMPONENT_SUSPEND)
        {
            if(!pThis->m_output_eos_rxd && !pThis->fake_eos_recieved)
            {
                DEBUG_PRINT("\n OUT: In Suspend Request, Actual Out EOS not rxd\n");
                pthread_mutex_lock(&pThis->out_buf_count_lock);
                if(!pThis->drv_out_buf_cnt)
                {
                    pthread_mutex_unlock(&pThis->out_buf_count_lock);
                    pThis->alloc_fill_suspend_out_buf();
                }
                else
                {
                    DEBUG_PRINT("\n OUT: %d o/p buffers still pending ..\n",
                        pThis->drv_out_buf_cnt);
                    pthread_mutex_unlock(&pThis->out_buf_count_lock);
                }
            }
            else
            {
                DEBUG_PRINT("\n OUT: Actual EOS Rxd on Suspend Request\n");
                pThis->fake_eos_recieved = true;
            }
        }
        else if(id == OMX_COMPONENT_RESUME)
        {
            if(pThis->getSuspendFlg() )
            {
                // start the audio driver that was suspended before
                if(ioctl(pThis->m_drv_fd, AUDIO_START, 0)< 0)
                {
                    DEBUG_PRINT_ERROR("AUDIO_START failed...\n");
                    pThis->m_first_mp3_header= 0;
                    pThis->post_command((unsigned)OMX_CommandStateSet,
                             (unsigned)OMX_StateInvalid,
                              OMX_COMPONENT_GENERATE_COMMAND);
                    pThis->post_command(OMX_CommandFlush,-1,OMX_COMPONENT_GENERATE_COMMAND);
                }
                pthread_mutex_lock(&pThis->m_suspendresume_lock);
                pThis->bSuspendEventRxed = false;
                pThis->bResumeEventRxed = false;
                pThis->bSuspendinprogress = false;
                pthread_mutex_unlock(&pThis->m_suspendresume_lock);
            }

            // Inform the client to start processing data now
            // that the component has succesfully rxed the faked EOS too.
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                OMX_EventComponentResumed,0,0,NULL);
            DEBUG_PRINT("RESUMED...\n");

            if(pThis->fake_eos_recieved)
            {
                DEBUG_PRINT("\nOUT: Fake EOS rxd!! m_suspend_out_buf_cnt = %d, \
                            m_suspend_drv_buf_cnt = %d\n", pThis->m_suspend_out_buf_cnt,
                            pThis->m_suspend_drv_buf_cnt);

                if(pThis->m_suspend_drv_buf_cnt)
                {
                    unsigned int i = 0;
                    int ret = 0;
                    for(i = pThis->m_suspend_drv_buf_cnt; i; i--)
                    {
                        ret = ioctl(pThis->m_drv_fd, AUDIO_ASYNC_READ, &pThis->m_suspend_out_drv_buf[i-1]);
                        if(ret < 0)
                        {
                            DEBUG_PRINT_ERROR("\n Error in ASYNC READ call, ret = %d\n", ret);
                        }
                        else
                        {
                            pthread_mutex_lock(&pThis->out_buf_count_lock);
                            pThis->drv_out_buf_cnt++;
                            pthread_mutex_unlock(&pThis->out_buf_count_lock);
                        }
                    }
                    free(pThis->m_suspend_out_drv_buf);
                    pThis->m_suspend_out_drv_buf = NULL;
                    pThis->m_suspend_drv_buf_cnt = 0;
                }

                if(!pThis->m_suspend_out_buf_cnt)
                {
                    DEBUG_PRINT("\nOUT: Fake EOS rxd!! No SuspendBuf\n");
                    pThis->fake_eos_recieved=false;
                    pThis->fake_in_eos_sent = false;
                }

            }
            else
            {
                DEBUG_PRINT_ERROR("\nOUT: Error in Suspend handling, Fake Out EOS not rxd\n");
            }
        }
        else if(id == OMX_COMPONENT_GENERATE_COMMAND)
        {
            // Execute FLUSH command
            if(p1 == OMX_CommandFlush)
            {
                DEBUG_DETAIL("Executing FLUSH command on Output port\n");
                pThis->execute_output_omx_flush();

            }
            else
            {
                DEBUG_DETAIL("Invalid command[%d]\n",p1);
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("ERROR:OUT-->Invalid Id[%d]\n",id);
        }
    }
    else
    {
        DEBUG_DETAIL("ERROR: OUT--> Empty OUTPUTQ\n");
    }

    return;
}

void omx_mp3_adec::process_command_msg(void *client_data, unsigned char id)
{
    unsigned               p1; // Parameter - 1
    unsigned               p2; // Parameter - 2
    unsigned               ident;
    unsigned               qsize=0; // qsize
    OMX_STATETYPE          state;
    omx_mp3_adec  *pThis = (omx_mp3_adec *) client_data;

    DEBUG_PRINT("%s %p\n",__FUNCTION__,pThis);
    pthread_mutex_lock(&pThis->m_commandlock);
    qsize = pThis->m_command_q.m_size;
    DEBUG_PRINT("CMD-->QSIZE=%d state=%d\n",pThis->m_command_q.m_size,
        pThis->m_state);

    if(!qsize )
    {
        DEBUG_PRINT("CMD-->BREAKING FROM LOOP\n");
        pthread_mutex_unlock(&pThis->m_commandlock);
        return;
    }
    else
    {
        pThis->m_command_q.pop_entry(&p1,&p2,&ident);
    }
    pthread_mutex_unlock(&pThis->m_commandlock);

    id = ident;
    DEBUG_PRINT("CMD->state[%d]id[%d]cmdq[%d] \n",\
        pThis->m_state,ident, \
        pThis->m_command_q.m_size);

    if(id == OMX_COMPONENT_GENERATE_EVENT)
    {
        if (pThis->m_cb.EventHandler)
        {
            if (p1 == OMX_CommandStateSet)
            {
                pthread_mutex_lock(&pThis->m_seq_lock);
                pThis->m_state = (OMX_STATETYPE) p2;
                DEBUG_PRINT("CMD:Process->state set to %d \n", \
                    pThis->m_state);

                if((pThis->m_state == OMX_StateExecuting) ||
                    (pThis->m_state == OMX_StateLoaded))
                {
                    pthread_mutex_lock(&pThis->m_in_th_lock_1);
                    if(pThis->is_in_th_sleep)
                    {
                        pThis->is_in_th_sleep = false;
                        DEBUG_DETAIL("CMD:WAKING UP IN THREADS\n");
                        pThis->in_th_wakeup();
                    }
                    pthread_mutex_unlock(&pThis->m_in_th_lock_1);

                    pthread_mutex_lock(&pThis->m_out_th_lock_1);
                    if(pThis->is_out_th_sleep)
                    {
                        DEBUG_DETAIL("CMD:WAKING UP OUT THREADS\n");
                        pThis->is_out_th_sleep = false;
                        pThis->out_th_wakeup();
                    }
                    pthread_mutex_unlock(&pThis->m_out_th_lock_1);
                    if((pThis->m_state == OMX_StateExecuting))
                      pThis->m_pause_to_exe = false;
                }
                else if(pThis->m_state == OMX_StateIdle)
                {
                    pThis->m_to_idle=false;
                }
                pthread_mutex_unlock(&pThis->m_seq_lock);
            }
            if (pThis->m_state == OMX_StateInvalid)
            {
                pThis->m_cb.EventHandler(&pThis->m_cmp,
                    pThis->m_app_data,
                    OMX_EventError,
                    OMX_ErrorInvalidState,
                    0, NULL );
            }
            else
            {
                pThis->m_cb.EventHandler(&pThis->m_cmp,
                    pThis->m_app_data,
                    OMX_EventCmdComplete,
                    p1, p2, NULL );

            }
        }
        else
        {
            DEBUG_PRINT_ERROR("ERROR:CMD-->EventHandler NULL \n");
        }
    }
    else if(id == OMX_COMPONENT_GENERATE_COMMAND)
    {

        pThis->send_command_proxy(&pThis->m_cmp,
            (OMX_COMMANDTYPE)p1,
            (OMX_U32)p2,(OMX_PTR)NULL);

    }
    else if(id == OMX_COMPONENT_PORTSETTINGS_CHANGED)
    {
        DEBUG_DETAIL("CMD OMX_COMPONENT_PORTSETTINGS_CHANGED");
        pThis->m_cb.EventHandler(&pThis->m_cmp,
            pThis->m_app_data,
            OMX_EventPortSettingsChanged,
            1, 0, NULL );
    }
    else if(OMX_COMPONENT_SUSPEND == id)
    {
        if (!pThis->m_first_mp3_header)
        {
            DEBUG_PRINT("DONT PROCESS SUSPEND EVENT, PLAYBACK NOT STARTED\n");
            return;
        }

        pthread_mutex_lock(&pThis->m_suspendresume_lock);
        if (pThis->getSuspendFlg())
        {
            pthread_mutex_unlock(&pThis->m_suspendresume_lock);
            DEBUG_PRINT("DO NOT PROCESS SUSPEND EVENT, ALREADY IN SUSPEND MODE\n");
            return;
        }
        else
        {
           pthread_mutex_unlock(&pThis->m_suspendresume_lock);
        }

        if (!pThis->bOutputPortReEnabled)
        {
            DEBUG_PRINT("DONT PROCESS SUSPEND EVENT, NO PORT RECONFIG\n");
            return;
        }


        pthread_mutex_lock(&pThis->m_seq_lock);
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
        if (state != OMX_StatePause) {
              pthread_mutex_unlock(&pThis->m_seq_lock);
              DEBUG_PRINT("DONT PROCESS SUSPEND EVENT, COMPONENT NOT IN PAUSED STATE\n");
              return;
        }
        pthread_mutex_unlock(&pThis->m_seq_lock);
        pthread_mutex_lock(&pThis->m_suspendresume_lock);
        if(pThis->m_pause_to_exe)
        {
              DEBUG_PRINT("DONT PROCESS SUSPEND EVENT, COMPONENT MOVING FROM PAUSED TO EXE STATE\n");
              pthread_mutex_unlock(&pThis->m_suspendresume_lock);
              return;
        }
        pThis->bSuspendEventRxed = true;
        pThis->bSuspendinprogress = true;

        DEBUG_PRINT("CMD-->Suspend event rxed suspendflag=%d \n",\
                            pThis->bSuspendEventRxed);
        pthread_mutex_unlock(&pThis->m_suspendresume_lock);

        if(!pThis->m_output_eos_rxd && !pThis->fake_eos_recieved)
        {
            // signal the output thread to process suspend
            pThis->post_output(0,0,OMX_COMPONENT_SUSPEND);
            //signal the input thread to process suspend
            pThis->post_input(0,0,OMX_COMPONENT_SUSPEND);
        }
        else
        {
            DEBUG_PRINT("DONT FORWARD SUSPEND?RESUME.\n");
            pThis->fake_eos_recieved = true;
            pthread_mutex_lock(&pThis->out_buf_count_lock);
            if(pThis->drv_out_buf_cnt > 0)
            {
                pthread_mutex_unlock(&pThis->out_buf_count_lock);
                DEBUG_PRINT("Flushing pending out buffers, drv out is %d \n",pThis->drv_out_buf_cnt);
                if(0 > ioctl(pThis->m_drv_fd, AUDIO_FLUSH, 0))
                {
                    DEBUG_PRINT_ERROR("\n Error in ioctl AUDIO_FLUSH\n");
                }
            }
            else
            {
                pthread_mutex_unlock(&pThis->out_buf_count_lock);
                DEBUG_PRINT("...AUDIO STOP...\n");
                ioctl(pThis->m_drv_fd, AUDIO_STOP, 0);
                pthread_mutex_lock(&pThis->m_suspendresume_lock);
                pThis->bSuspendinprogress = false;
                pthread_mutex_unlock(&pThis->m_suspendresume_lock);
                if (pThis->getWaitForSuspendCmplFlg())
                {
                    DEBUG_PRINT_ERROR("Release P-->Executing context to IL client.\n");
                    pThis->release_wait_for_suspend();
                }
            }

        }
        pthread_mutex_lock(&pThis->m_in_th_lock_1);
        if(pThis->is_in_th_sleep)
        {
                DEBUG_DETAIL("CMD:WAKING UP IN THREADS\n");
                pThis->in_th_wakeup();
                pThis->is_in_th_sleep = false;
        }
        pthread_mutex_unlock(&pThis->m_in_th_lock_1);
        pthread_mutex_lock(&pThis->m_out_th_lock_1);
        if(pThis->is_out_th_sleep)
        {
                DEBUG_DETAIL("CMD:WAKING UP OUT THREADS\n");
                pThis->out_th_wakeup();
                pThis->is_out_th_sleep = false;
        }
        pthread_mutex_unlock(&pThis->m_out_th_lock_1);

    }
    else if(id == OMX_COMPONENT_RESUME)
    {
        pthread_mutex_lock(&pThis->m_suspendresume_lock);
        if (pThis->getSuspendFlg() && pThis->bSuspendinprogress)
        {
            pthread_mutex_unlock(&pThis->m_suspendresume_lock);
            DEBUG_PRINT("DO NOT PROCESS DRIVER RESUME EVENT\n");
            return;
        }
        // signal the output thread that RESUME has happened
        pThis->bResumeEventRxed = true;
        pthread_mutex_unlock(&pThis->m_suspendresume_lock);
        pThis->post_output(0,0,OMX_COMPONENT_RESUME);

        pthread_mutex_lock(&pThis->m_out_th_lock_1);
        if(pThis->is_out_th_sleep)
        {
            DEBUG_DETAIL("CMD:WAKING UP OUT THREADS\n");
            pThis->out_th_wakeup();
            pThis->is_out_th_sleep = false;
        }
        pthread_mutex_unlock(&pThis->m_out_th_lock_1);
    }
    else if(id == OMX_COMPONENT_GENERATE_EOS)
    {
        pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
            OMX_EventBufferFlag, 1, 1, NULL );
    }
    else
    {
        DEBUG_PRINT_ERROR("CMD:Error--> incorrect event posted\n");
    }
    return;
}

void omx_mp3_adec::process_in_port_msg(void *client_data, unsigned char id)
{
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned      ident;
    unsigned      qsize=0; // qsize
    unsigned      tot_qsize = 0;
    omx_mp3_adec  *pThis = (omx_mp3_adec *) client_data;
    OMX_STATETYPE state;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,pThis);
    if(!pThis)
    {
        DEBUG_PRINT_ERROR("IN:ERROR : Context is incorrect; bailing out\n");
        return;
    }
loopback_in:
    pthread_mutex_lock(&pThis->m_seq_lock);
    pthread_mutex_lock(&pThis->m_state_lock);
    pThis->get_state(&pThis->m_cmp, &state);
    pthread_mutex_unlock(&pThis->m_state_lock);
    if(state == OMX_StateLoaded)
    {
        pthread_mutex_unlock(&pThis->m_seq_lock);
        DEBUG_PRINT(" IN: IN LOADED STATE RETURN\n");
        return;
    }
    // Protect the shared queue data structure
    pthread_mutex_lock(&pThis->m_inputlock);

    qsize = pThis->m_input_ctrl_cmd_q.m_size;
    tot_qsize = qsize;
    tot_qsize += pThis->m_input_ctrl_ebd_q.m_size;
    tot_qsize += pThis->m_input_q.m_size;
    tot_qsize += pThis->m_input_ctrl_q.m_size;
    DEBUG_DETAIL("Input-->QSIZE-flush=%d,cmdq[%d] ebd=%d QSIZE=%d state=%d Flush=%d\n",\
        pThis->m_input_ctrl_cmd_q.m_size,
        pThis->m_input_ctrl_q.m_size,
        pThis->m_input_ctrl_ebd_q.m_size,
        pThis->m_input_q.m_size, state, pThis->bFlushinprogress);

    if(tot_qsize ==0) {
        DEBUG_DETAIL("IN-->BREAKING FROM IN LOOP");
        pthread_mutex_unlock(&pThis->m_inputlock);
        pthread_mutex_unlock(&pThis->m_seq_lock);
        return;
    }
    if ( (state == OMX_StatePause) && !(pThis->m_input_ctrl_cmd_q.m_size) &&
         !(pThis->m_input_ctrl_q.m_size) && !pThis->bFlushinprogress && !pThis->m_to_idle)
    {
        pthread_mutex_unlock(&pThis->m_inputlock);
        DEBUG_PRINT("SLEEPING IN THREAD\n");
        pthread_mutex_lock(&pThis->m_in_th_lock_1);
        pThis->is_in_th_sleep = true;
        pthread_mutex_unlock(&pThis->m_in_th_lock_1);
        pthread_mutex_unlock(&pThis->m_seq_lock);
        pThis->in_th_goto_sleep();
        goto loopback_in;
    }
    else if ((state == OMX_StatePause))
    {
        if(!( pThis->m_input_ctrl_q.m_size) && !(pThis->m_input_ctrl_cmd_q.m_size) )
        {
            pthread_mutex_unlock(&pThis->m_inputlock);

            DEBUG_DETAIL("IN: SLEEPING IN THREAD\n");
            pthread_mutex_lock(&pThis->m_in_th_lock_1);
            pThis->is_in_th_sleep = true;
            pthread_mutex_unlock(&pThis->m_in_th_lock_1);
            pthread_mutex_unlock(&pThis->m_seq_lock);
            pThis->in_th_goto_sleep();
            goto loopback_in;
        }
    }

    if(qsize)
    {
        // process FLUSH message
        pThis->m_input_ctrl_cmd_q.pop_entry(&p1,&p2,&ident);
    }
    else if((qsize = pThis->m_input_ctrl_ebd_q.m_size) &&
        ((state == OMX_StateExecuting) || pThis->m_to_idle))
    {
        // then process EBD's
        pThis->m_input_ctrl_ebd_q.pop_entry(&p1,&p2,&ident);
    }
    else if((qsize = pThis->m_input_q.m_size) && !pThis->bFlushinprogress &&
        (state == OMX_StateExecuting) && (pThis->m_resume_out_buf_cnt == pThis->m_suspend_out_buf_cnt))
    {
        // if no FLUSH and EBD's then process ETB's
        pThis->m_input_q.pop_entry(&p1, &p2, &ident);
    }
    else if((qsize = pThis->m_input_ctrl_q.m_size) && (pThis->bSuspendEventRxed) && (state == OMX_StatePause))
    {
        pThis->m_input_ctrl_q.pop_entry(&p1,&p2,&ident);
    }
    else if(state == OMX_StateLoaded)
    {
        pthread_mutex_unlock(&pThis->m_inputlock);
        pthread_mutex_unlock(&pThis->m_seq_lock);
        DEBUG_PRINT("IN: ***in OMX_StateLoaded so exiting\n");
        return ;
    }
    else
    {
        qsize = 0;
        pthread_mutex_unlock(&pThis->m_inputlock);
        if(state == OMX_StatePause || (pThis->m_resume_out_buf_cnt != pThis->m_suspend_out_buf_cnt))
        {
            DEBUG_PRINT("SLEEPING IN THREAD\n");
            pthread_mutex_lock(&pThis->m_in_th_lock_1);
            pThis->is_in_th_sleep = true;
            pthread_mutex_unlock(&pThis->m_in_th_lock_1);
            pthread_mutex_unlock(&pThis->m_seq_lock);
            pThis->in_th_goto_sleep();
        }
        else
        {
            pthread_mutex_unlock(&pThis->m_seq_lock);
        }
        goto loopback_in;
    }
    pthread_mutex_unlock(&pThis->m_inputlock);
    pthread_mutex_unlock(&pThis->m_seq_lock);
    if(qsize > 0)
    {
        id = ident;
        DEBUG_DETAIL("Input->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d]\n",\
            pThis->m_state,
            ident,
            pThis->m_input_ctrl_cmd_q.m_size,
            pThis->m_input_ctrl_ebd_q.m_size,
            pThis->m_input_q.m_size);

        if(id == OMX_COMPONENT_GENERATE_BUFFER_DONE)
        {
            pThis->buffer_done_cb((OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_EOS)
        {
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                OMX_EventBufferFlag, 0, 1, NULL );
        }
        else if(id == OMX_COMPONENT_GENERATE_ETB)
        {
            pThis->empty_this_buffer_proxy((OMX_HANDLETYPE)p1,
                (OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_COMMAND)
        {
            // Execute FLUSH command
            if(p1 == OMX_CommandFlush)
            {
                DEBUG_DETAIL(" Executing FLUSH command on Input port\n");
                pThis->execute_input_omx_flush();
            }
            else
            {
                DEBUG_DETAIL("Invalid command[%d]\n",p1);
            }
        }
        else if(id == OMX_COMPONENT_SUSPEND)
        {
            /* Send Fake EOS when actual EOS buffer is not with MP3 driver */
            if(!pThis->m_input_eos_rxd && !pThis->fake_in_eos_sent)
            {
                DEBUG_PRINT("\n IN: In Suspend Request, Actual In EOS not rxd\n");
                DEBUG_PRINT("IN:FAKING EOS TO KERNEL");
                pThis->omx_mp3_fake_eos();
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("IN:Error--> ProcessMsgCb Ignored due to Invalid Identifier\n");
        }
    }
    else
    {
        DEBUG_DETAIL("ERROR:IN-->Empty INPUT Q\n");
    }
    return ;
}

/**
 @brief member function for performing component initialization

 @param role C string mandating role of this component
 @return Error status
 */
OMX_ERRORTYPE omx_mp3_adec::component_init(OMX_STRING role)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    m_state            = OMX_StateLoaded;
    m_is_in_th_sleep=0;
    m_is_out_th_sleep=0;
    m_pause_to_exe = false;
    m_flush_cmpl_event=0;
    m_timer->resetTimerExpiry();

    /* DSP does not give information about the bitstream
    randomly assign the value right now. Query will result in
    incorrect param */
    memset(&m_adec_param, 0, sizeof(m_adec_param));
    m_adec_param.nSize = sizeof(m_adec_param);
    m_adec_param.nSampleRate = DEFAULT_SAMPLING_RATE;// default value
    m_volume = 25; /* Close to unity gain */
    m_adec_param.nChannels = DEFAULT_CHANNEL_MODE;
    pSamplerate = DEFAULT_SAMPLING_RATE;
    pChannels = DEFAULT_CHANNEL_MODE;
    m_adec_param.eChannelMode = OMX_AUDIO_ChannelModeStereo;

    m_pcm_param.nChannels = DEFAULT_CHANNEL_MODE;
    m_pcm_param.eNumData = OMX_NumericalDataSigned;
    m_pcm_param.bInterleaved = OMX_TRUE;
    m_pcm_param.nBitPerSample = 16;
    m_pcm_param.nSamplingRate = DEFAULT_SAMPLING_RATE;
    m_pcm_param.ePCMMode = OMX_AUDIO_PCMModeLinear;
    m_pcm_param.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
    m_pcm_param.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

    /* default calculation of frame duration */
    frameDuration = (((OMX_MP3_OUTPUT_BUFFER_SIZE)* 1000) / (DEFAULT_SAMPLING_RATE * DEFAULT_CHANNEL_MODE * 2));
    m_fbd_cnt = 0;
    nTimestamp = 0;
    DEBUG_PRINT(" Enabling Non-Tunnel mode \n");
    pcm_feedback = 1;    /* by default enable non-tunnel mode */
    ntotal_playtime = 0;
    nState = OMX_StateLoaded;
    bFlushinprogress = 0;
    nNumInputBuf = 0;
    nNumOutputBuf = 0;
    m_ipc_to_in_th = NULL;  // Command server instance
    m_ipc_to_out_th = NULL;  // Client server instance
    m_ipc_to_cmd_th = NULL;  // command instance
    m_ipc_to_event_th = NULL;

    bFlushcompleted = 0;
    bSuspendinprogress = 0;

    m_first_mp3_header = 0;
    suspensionPolicy= OMX_SuspensionDisabled;
    resetSuspendFlg();
    resetResumeFlg();;
    fake_eos_recieved = false;
    fake_in_eos_ack_received = false;
    fake_in_eos_sent = false;
    m_to_idle =false;
    mp3Inst = this;

    bOutputPortReEnabled = 0;
    memset(&m_priority_mgm, 0, sizeof(m_priority_mgm));
    m_priority_mgm.nGroupID =0;
    m_priority_mgm.nGroupPriority=0;

    memset(&m_buffer_supplier, 0, sizeof(m_buffer_supplier));
    m_buffer_supplier.nPortIndex=OMX_BufferSupplyUnspecified;
    m_suspend_out_buf_list = NULL;
    m_suspend_out_drv_buf = NULL;
    m_resume_out_buf_cnt = 0;
    m_suspend_out_buf_cnt = 0;
    m_suspend_drv_buf_cnt = 0;
    drv_inp_buf_cnt = 0;
    drv_out_buf_cnt = 0;
    m_flush_inbuf = false;
    m_flush_outbuf = false;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);

    mime_type = (OMX_STRING) malloc(sizeof("audio/mpeg"));
    if (mime_type)
    {
        strcpy(mime_type, "audio/mpeg");
        DEBUG_PRINT("MIME type: %s\n", mime_type);
    } else {
        DEBUG_PRINT_ERROR("error allocating mime type string\n");
    }
    if(!strcmp(role,"OMX.qcom.audio.decoder.mp3"))
    {
        pcm_feedback = 1;
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
    }
    else if(!strcmp(role,"OMX.qcom.audio.decoder.tunneled.mp3"))
    {
        pcm_feedback = 0;
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
    }
    else
    {
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED is invalid\n", role);
    }
    /* Open the MP3 device in asynchronous mode */
    if(0 == pcm_feedback)
    {
        m_drv_fd = open("/dev/msm_mp3",O_WRONLY | O_NONBLOCK);
    }
    else
    {
        m_drv_fd = open("/dev/msm_mp3",O_RDWR | O_NONBLOCK);
    }
    if (m_drv_fd < 0)
    {
        DEBUG_PRINT_ERROR("component_init: device open fail Loaded -->Invalid\n");
        return OMX_ErrorInsufficientResources;
    }
    if(ioctl(m_drv_fd, AUDIO_GET_SESSION_ID,&m_session_id) == -1)
    {
        DEBUG_PRINT("AUDIO_GET_SESSION_ID FAILED\n");
    }
    if(!m_ipc_to_in_th)
    {
        m_ipc_to_in_th = omx_mp3_thread_create(process_in_port_msg, this,
           (char *) "INPUT_THREAD");
        if(!m_ipc_to_in_th)
        {
            DEBUG_PRINT_ERROR("ERROR!!!INPUT THREAD failed to get created\n");
            return OMX_ErrorHardware;
        }
    }
    if(!m_ipc_to_cmd_th)
    {
        m_ipc_to_cmd_th = omx_mp3_thread_create(process_command_msg, this,
           (char *) "COMMAND_THREAD");
        if(!m_ipc_to_cmd_th)
        {
            DEBUG_PRINT_ERROR("ERROR!!! COMMAND THREAD failed to get created\n");
            return OMX_ErrorHardware;
        }
    }
    if(pcm_feedback)
    {
        if(!m_ipc_to_out_th)
        {
            m_ipc_to_out_th = omx_mp3_thread_create(process_out_port_msg, this,
               (char *) "OUTPUT_THREAD");
            if(!m_ipc_to_out_th)
            {
                DEBUG_PRINT_ERROR("ERROR!!! OUTPUT THREAD failed to get created\n");
                return OMX_ErrorHardware;
            }
        }
    }
    DEBUG_PRINT_ERROR("%s[%p]component init: role = %s\n", __FUNCTION__, 
                                                            this, role);
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
OMX_ERRORTYPE  omx_mp3_adec::get_component_version(OMX_IN OMX_HANDLETYPE               hComp,
                                                  OMX_OUT OMX_STRING          componentName,
                                                  OMX_OUT OMX_VERSIONTYPE* componentVersion,
                                                  OMX_OUT OMX_VERSIONTYPE*      specVersion,
                                                  OMX_OUT OMX_UUIDTYPE*       componentUUID)
{
    (void)hComp;
    (void)componentName;
    (void)componentVersion;
    (void)specVersion;
    (void)componentUUID; 
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Comp Version in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    /* TBD -- Return the proper version */
    return OMX_ErrorNone;
}
/**
  @brief member function handles command from IL client

  This function simply queue up commands from IL client.
  Commands will be processed in command server thread context later

  @param hComp handle to component instance
  @param cmd type of command
  @param param1 parameters associated with the command type
  @param cmdData
  @return Error status
*/
OMX_ERRORTYPE  omx_mp3_adec::send_command(OMX_IN OMX_HANDLETYPE hComp,
                                          OMX_IN OMX_COMMANDTYPE  cmd,
                                          OMX_IN OMX_U32       param1,
                                          OMX_IN OMX_PTR      cmdData)
{
    int portIndex = (int)param1;
    (void)hComp;
    (void)cmdData;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(OMX_StateInvalid == m_state)
    {
        return OMX_ErrorInvalidState;
    }

    DEBUG_PRINT(" inside  send_command is_out_th_sleep=%d\n",is_out_th_sleep);
    if ( (cmd == OMX_CommandFlush) && (portIndex > 1) )
    {
        return OMX_ErrorBadPortIndex;
    }
#if 0
    if(bFlushinprogress)
	{
    DEBUG_PRINT_ERROR("send_command : FLUSH INP... wait= %d param1 = %d flush[%d]\n",cmd,(signed)param1, bFlushinprogress);
            sem_wait(&sem_States);
    DEBUG_PRINT_ERROR("send_command : FLUSH INP RELEASED... wait= %d param1 = %dflush[%d]\n",cmd,(signed)param1, bFlushinprogress);
	}
#endif
    if((m_state == OMX_StatePause) )
    {
        DEBUG_PRINT("Send Command-->State=%d cmd=%d param1=%lu sus=%d res=%d\n",
                             m_state,cmd,param1,getSuspendFlg(),getResumeFlg());
        if((cmd == OMX_CommandStateSet) )
        {
            pthread_mutex_lock(&m_suspendresume_lock);
            if((OMX_STATETYPE)param1 == OMX_StateExecuting)
            {
                m_pause_to_exe = true;
            }
            if(getSuspendFlg() && bSuspendinprogress)
            {
                pthread_mutex_unlock(&m_suspendresume_lock);
                DEBUG_PRINT_ERROR("Send Command, waiting for suspend/resume procedure to complete\n");
                wait_for_suspend_cmpl();
            }
            else
               pthread_mutex_unlock(&m_suspendresume_lock);
        }
    }

    post_command((unsigned)cmd,(unsigned)param1,OMX_COMPONENT_GENERATE_COMMAND);
    DEBUG_PRINT("send_command : received cmd = %d param1 = %d\n",cmd,(signed)param1);

    DEBUG_PRINT("send_command : received state before semwait= %d\n",(signed)param1);
    sem_wait (&sem_States);
    DEBUG_PRINT("send_command : received state after semwait\n");

    return OMX_ErrorNone;
}

/**
 @brief member function performs actual processing of commands excluding
  empty buffer call

 @param hComp handle to component
 @param cmd command type
 @param param1 parameter associated with the command
 @param cmdData

 @return error status
*/
OMX_ERRORTYPE  omx_mp3_adec::send_command_proxy(OMX_IN OMX_HANDLETYPE hComp,
                                          OMX_IN OMX_COMMANDTYPE  cmd,
                                          OMX_IN OMX_U32       param1,
                                          OMX_IN OMX_PTR      cmdData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
   //   Handle only IDLE and executing
    OMX_STATETYPE eState = (OMX_STATETYPE) param1;
    int rc = 0;
    int bFlag = 1;

    nState = eState;
    (void)hComp;
    (void)cmdData;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(cmd == OMX_CommandStateSet)
    {
        /***************************/
        /* Current State is Loaded */
        /***************************/
        if(m_state == OMX_StateLoaded)
        {
            if(eState == OMX_StateIdle)
            {

                if (allocate_done()||(m_inp_bEnabled == OMX_FALSE && m_out_bEnabled == OMX_FALSE))
                {
                    DEBUG_PRINT("SCP: Loaded->Idle\n");
                }
                else
                {
                    DEBUG_PRINT("SCP: Loaded-->Idle-Pending\n");
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
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
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
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StatePause)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Pause\n");
                m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorIncorrectStateTransition,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Invalid\n");
                m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorInvalidState,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                m_state = OMX_StateInvalid;
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP: Loaded-->Invalid(%d Not Handled)\n",eState);
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
                if(release_done(OMX_ALL))
                {
                    if(0 > ioctl(m_drv_fd, AUDIO_ABORT_GET_EVENT, NULL))
                    {
                        DEBUG_PRINT("\n Error in ioctl AUDIO_ABORT_GET_EVENT\n");
                        return OMX_ErrorHardware;
                    }
                    if (m_ipc_to_event_th != NULL)
                    {
                        omx_mp3_thread_stop(m_ipc_to_event_th);
                        m_ipc_to_event_th = NULL;
                    }
                    if(ioctl(m_drv_fd, AUDIO_STOP, 0) == -1)
                    {
                        DEBUG_PRINT("SCP:Idle->Loaded,ioctl stop failed %d\n",\
                            errno);
                    }
                    DEBUG_PRINT("Close device in Send Command Proxy\n");
                    m_first_mp3_header= 0;
                    DEBUG_PRINT("SCP: Idle-->Loaded\n");
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
                if(suspensionPolicy == OMX_SuspensionEnabled)
                {
                    m_suspend_out_buf_list = (struct mmap_info**)calloc(1,
                        sizeof(struct mmap_info*) * MAX_SUSPEND_OUT_BUFFERS);

                    if(m_suspend_out_buf_list == NULL)
                    {
                        DEBUG_PRINT("NOT ABLE TO ALLOCATE m_suspend_out_buf_list");
                        return OMX_ErrorHardware;
                    }
                }

                if(!m_ipc_to_event_th)
                {
                    DEBUG_PRINT("CREATING EVENT THREAD -->GNG TO EXE STATE");
                    m_ipc_to_event_th = omx_mp3_event_thread_create(
                        process_event_cb, this,
                        (char *)"EVENT_THREAD");
                    if(!m_ipc_to_event_th)
                    {
                        DEBUG_PRINT_ERROR("ERROR!!! EVENT THREAD failed to get created\n");
                        sem_post (&sem_States);
                        return OMX_ErrorHardware;
                    }
                }

                struct msm_audio_pcm_config  pcm_config;
                DEBUG_PRINT("SCP:configure driver mode as %d \n",pcm_feedback);
                if(ioctl(m_drv_fd, AUDIO_GET_PCM_CONFIG, &pcm_config) == -1)
                    DEBUG_PRINT("SCP:Idle->Exe,ioctl get-pcm fail %d\n",errno);
                pcm_config.pcm_feedback = pcm_feedback;
                pcm_config.buffer_count = m_out_act_buf_count;
                pcm_config.buffer_size  = output_buffer_size + sizeof(META_OUT);

                if(ioctl(m_drv_fd, AUDIO_SET_PCM_CONFIG, &pcm_config) == -1)
                {
                    DEBUG_PRINT("SCP:ioctl set-pcm-config failed=%d\n",errno);
                }


                DEBUG_PRINT("SCP: Idle-->Executing\n");
                nState = eState;
            }
            else if(eState == OMX_StateIdle)
            {
                DEBUG_PRINT("SCP: Idle-->Idle\n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data, OMX_EventError, OMX_ErrorSameState, OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if(eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("SCP: Idle-->WaitForResources\n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data, OMX_EventError, OMX_ErrorIncorrectStateTransition, OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StatePause)
            {
                DEBUG_PRINT("SCP: Idle-->Pause\n");
            }
            else if(eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("SCP: Idle-->Invalid\n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data, OMX_EventError, OMX_ErrorInvalidState, OMX_COMPONENT_GENERATE_EVENT, NULL );
                m_state = OMX_StateInvalid;
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP: Idle --> %d Not Handled\n",eState);
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
                DEBUG_PRINT("SCP: Executing to Idle \n");
                bFlushinprogress = 1;
                m_to_idle = true;
                if(pcm_feedback)
                {
                    pthread_mutex_lock(&m_flush_lock);
                    m_flush_cnt = 2;
                    pthread_mutex_unlock(&m_flush_lock);
                    execute_omx_flush(OMX_ALL, false); // Flush all ports
                }
                else
                {
                    pthread_mutex_lock(&m_flush_lock);
                    m_flush_cnt = 1;
                    pthread_mutex_unlock(&m_flush_lock);
                    execute_omx_flush(0,false);
                }
                bFlushinprogress = 0;
                m_output_eos_rxd = false;
                m_input_eos_rxd = false;
            }
            else if(eState == OMX_StatePause)
            {
                nState = eState;
                /* Issue pause to driver to pause tunnel mode playback */
                if(!pcm_feedback)
                {
                    ioctl(m_drv_fd,AUDIO_PAUSE, 1);
                }
                DEBUG_DETAIL("*************************\n");
                DEBUG_PRINT("SCP-->RXED PAUSE STATE\n");
                DEBUG_DETAIL("*************************\n");
                DEBUG_PRINT("SCP:E-->P, start timer\n");
                getTimerInst()->startTimer();
            }
            else if(eState == OMX_StateLoaded)
            {
                DEBUG_PRINT("\n SCP:Executing --> Loaded \n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorIncorrectStateTransition,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("\nSCP:Executing --> WaitForResources \n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorIncorrectStateTransition,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StateExecuting)
            {
                DEBUG_PRINT("\n SCP: Executing --> Executing \n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorSameState,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if(eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("\n SCP: Executing --> Invalid \n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorInvalidState,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                m_state = OMX_StateInvalid;
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP: Executing --> %d Not Handled\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /***************************/
        /* Current State is Pause  */
        /***************************/
        else if(m_state == OMX_StatePause)
        {
            DEBUG_PRINT("SCP: aused --> Executing %d %d in=%d out%d\n",getSuspendFlg(),getResumeFlg(),
                                    is_in_th_sleep,is_out_th_sleep);
            if(!getTimerInst()->getTimerExpiry())
            {
                getTimerInst()->stopTimer();
            }
            getTimerInst()->resetTimerExpiry();
            if( (eState == OMX_StateExecuting || eState == OMX_StateIdle) )
            {
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                    is_in_th_sleep = false;
                    DEBUG_DETAIL("SCP: WAKING UP IN THREAD\n");
                    in_th_wakeup();
                }
                pthread_mutex_unlock(&m_in_th_lock_1);
                pthread_mutex_lock(&m_out_th_lock_1);
                if(is_out_th_sleep)
                {
                    DEBUG_DETAIL("SCP: WAKING UP OUT THREAD\n");
                    out_th_wakeup();
                    is_out_th_sleep = false;
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
            }

            if(eState == OMX_StateExecuting)
            {
                nState = eState;
                DEBUG_PRINT("SCP: Paused --> Executing %d %d \n",getSuspendFlg(),getResumeFlg());
                if( getSuspendFlg())
                    setResumeFlg();
                if(getSuspendFlg() && getResumeFlg())
                {
                    resetSuspendFlg();
                    resetResumeFlg();
                    post_output(0,0,OMX_COMPONENT_RESUME);
                    // start the audio driver that was suspended before
                    rc = ioctl(m_drv_fd, AUDIO_START, 0);
                    if(rc <0)
                    {
                        DEBUG_PRINT_ERROR("AUDIO_START FAILED\n");
                        m_first_mp3_header= 0;
                        post_command((unsigned)OMX_CommandStateSet,
                             (unsigned)OMX_StateInvalid,
                              OMX_COMPONENT_GENERATE_COMMAND);
			bFlushinprogress = 1;
                        execute_omx_flush(-1,false);
			bFlushinprogress = 0;
                        return OMX_ErrorInvalidState;
                    }
                }
            }
            else if(eState == OMX_StateIdle)
            {
                m_to_idle = true;
                DEBUG_PRINT("SCP: Paused to Idle \n");
                if( getSuspendFlg())
                    setResumeFlg();
                DEBUG_PRINT ("\n Internal flush issued");
                bFlushinprogress = 1;
                if(pcm_feedback)
                {
                    pthread_mutex_lock(&m_flush_lock);
                    m_flush_cnt = 2;
                    pthread_mutex_unlock(&m_flush_lock);
                    execute_omx_flush(OMX_ALL, false); // Flush all ports
                }
                else
                {
                    pthread_mutex_lock(&m_flush_lock);
                    m_flush_cnt = 1;
                    pthread_mutex_unlock(&m_flush_lock);
                    execute_omx_flush(0,false);
                }
                bFlushinprogress = 0;
                m_output_eos_rxd = false;
                m_input_eos_rxd = false;
            }
            else if(eState == OMX_StateLoaded)
            {
                DEBUG_PRINT("\n SCP:Pause --> loaded \n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorIncorrectStateTransition,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StateWaitForResources)
            {
                DEBUG_PRINT("\n SCP: Pause --> WaitForResources \n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorIncorrectStateTransition,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StatePause)
            {
                DEBUG_PRINT("\n SCP:Pause --> Pause \n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorSameState,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if(eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("\n SCP:Pause --> Invalid \n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorInvalidState,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                m_state = OMX_StateInvalid;
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT("SCP: Paused to %d Not Handled\n",eState);
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
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorSameState,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if(eState == OMX_StateExecuting)
            {
                DEBUG_PRINT("SCP: WaitForResources-->Executing\n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorIncorrectStateTransition,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StatePause)
            {
                DEBUG_PRINT("SCP: WaitForResources-->Pause\n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorIncorrectStateTransition,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if(eState == OMX_StateInvalid)
            {
                DEBUG_PRINT("SCP: WaitForResources-->Invalid\n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorInvalidState,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                m_state = OMX_StateInvalid;
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP--> %d to %d(Not Handled)\n",m_state,eState);
                eRet = OMX_ErrorBadParameter;
            }
        }

        /****************************/
        /* Current State is Invalid */
        /****************************/
        else if(m_state == OMX_StateInvalid)
        {
            if(OMX_StateLoaded == eState || OMX_StateWaitForResources == eState ||
                OMX_StateIdle == eState || OMX_StateExecuting == eState          ||
                OMX_StatePause == eState || OMX_StateInvalid == eState)
            {
                DEBUG_PRINT("SCP: Invalid-->Loaded/Idle/Executing/Pause/Invalid/WaitForResources\n");
                this->m_cb.EventHandler(&this->m_cmp,
                    this->m_app_data,
                    OMX_EventError,
                    OMX_ErrorInvalidState,
                    OMX_COMPONENT_GENERATE_EVENT, NULL );
                m_state = OMX_StateInvalid;
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT("SCP: Paused --> %d Not Handled\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }

    }
    else if (cmd == OMX_CommandFlush)
    {
        DEBUG_DETAIL("*************************\n");
        DEBUG_PRINT("SCP-->RXED FLUSH COMMAND port=%d\n",(signed)param1);
        DEBUG_DETAIL("*************************\n");

        bFlushinprogress = 1;


        bFlag = 0;
        if((param1 == OMX_CORE_INPUT_PORT_INDEX) ||
            (param1 == OMX_CORE_OUTPUT_PORT_INDEX) ||
            (param1 == OMX_ALL))
        {
            execute_omx_flush(param1);
        }
        else
        {
            eRet = OMX_ErrorBadPortIndex;
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventError,
                OMX_CommandFlush, OMX_ErrorBadPortIndex, NULL );
        }

        if(((m_input_eos_rxd) && (param1 == OMX_CORE_INPUT_PORT_INDEX)) ||
           ((m_input_eos_rxd) && (param1 == OMX_ALL)))
        {
            m_input_eos_rxd = false;
        }

        if (m_output_eos_rxd)
        {

            m_input_eos_rxd = false;
            m_output_eos_rxd = false;
        }
        bFlushinprogress = 0;
        bFlushcompleted = 1;
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
                bOutputPortReEnabled = 0;
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortDisable:\
                            OMX_CORE_INPUT_PORT_INDEX:release_done \n");
                DEBUG_PRINT("************* OMX_CommandPortDisable:\
                            m_inp_bEnabled si %d\n",m_inp_bEnabled);

                post_command(OMX_CommandPortDisable,
                    OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT);
            }
            else
            {
                if((m_state == OMX_StatePause) || (m_state == OMX_StateExecuting))
                {
                    DEBUG_PRINT("SCP: execute_omx_flush in Disable in \
                                param1=%d m_state=%d \n",(signed)param1, m_state);
		    bFlushinprogress = 1;
                    execute_omx_flush(param1,false );
		    bFlushinprogress = 0;
                }
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortDisable:\
                            OMX_CORE_INPUT_PORT_INDEX \n");
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
            }

        }
        if ((param1 == OMX_CORE_OUTPUT_PORT_INDEX) || (param1 == OMX_ALL))
        {
            DEBUG_PRINT("SCP: Disabling Output port Indx\n");
            m_out_bEnabled = OMX_FALSE;
            if(((m_state == OMX_StateLoaded) || (m_state == OMX_StateIdle))
                && release_done(1))
            {
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortDisable:\
                            OMX_CORE_OUTPUT_PORT_INDEX:release_done \n");
                DEBUG_PRINT("************* OMX_CommandPortDisable:\
                            m_out_bEnabled is %d\n",m_inp_bEnabled);

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
                                param1=%d m_state=%d \n",(signed)param1, m_state);
		    bFlushinprogress = 1;
                    execute_omx_flush(param1, false);
		    bFlushinprogress = 0;
                }
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("SCP-->Disabling invalid port ID[%d]",(signed)param1);
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
            if(((m_state == OMX_StateLoaded)
                && !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                || (m_state == OMX_StateWaitForResources)
                || (m_inp_bPopulated == OMX_TRUE))
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
            if(((m_state == OMX_StateLoaded)
                && !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                || (m_state == OMX_StateWaitForResources)
                || (m_out_bPopulated == OMX_TRUE))
            {
                post_command(OMX_CommandPortEnable,
                    OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT);
            }
            else
            {
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortEnable:\
                            OMX_CORE_OUTPUT_PORT_INDEX:release_done \n");
                bOutputPortReEnabled = 0;
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
            }
            pthread_mutex_lock(&m_in_th_lock_1);
            if(is_in_th_sleep)
            {
                is_in_th_sleep = false;
                DEBUG_DETAIL("SCP:WAKING UP IN THREADS\n");
                in_th_wakeup();
            }
            pthread_mutex_unlock(&m_in_th_lock_1);
            pthread_mutex_lock(&m_out_th_lock_1);
            if(is_out_th_sleep)
            {
                is_out_th_sleep = false;
                DEBUG_PRINT("SCP:WAKING OUT THR, OMX_CommandPortEnable\n");
                out_th_wakeup();
            }
            pthread_mutex_unlock(&m_out_th_lock_1);


        } else
        {
            DEBUG_PRINT_ERROR("SCP-->Enabling invalid port ID[%d]",(signed)param1);
        }

    }
    else
    {
        DEBUG_PRINT_ERROR("SCP-->ERROR: Invali Command [%d]\n",cmd);
        eRet = OMX_ErrorNotImplemented;
    }

    DEBUG_PRINT("posting sem_States\n");
    sem_post (&sem_States);

    if(eRet == OMX_ErrorNone && bFlag)
    {
        post_command(cmd,eState,OMX_COMPONENT_GENERATE_EVENT);
    }
    return eRet;
}

/**
@brief member function that flushes buffers that are pending to be written
to driver

@param none
@return bool value indicating whether flushing is carried out successfully
*/
bool omx_mp3_adec::execute_omx_flush(OMX_IN OMX_U32 param1, bool cmd_cmpl)
{
    bool bRet = true;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    DEBUG_PRINT("Execute_omx_flush Port[%d]",(signed) param1);
    struct timespec abs_timeout;
    abs_timeout.tv_sec = 1;
    abs_timeout.tv_nsec = 0; //333333;
    if (param1 == OMX_ALL)
    {
        DEBUG_PRINT_ERROR("Execute flush for both I/p O/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 2;
        pthread_mutex_unlock(&m_flush_lock);

        // Send Flush commands to input and output threads
        post_input(OMX_CommandFlush,
            OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);

        post_output(OMX_CommandFlush,
            OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        // Send Flush to the kernel so that the in and out buffers are released
        if(ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1)
            DEBUG_PRINT("FLush:ioctl flush failed errno=%d\n",errno);

            DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
                    is_in_th_sleep,is_out_th_sleep);
            DEBUG_DETAIL("****************************************");
            pthread_mutex_lock(&m_in_th_lock_1);
            if(is_in_th_sleep)
            {
                is_in_th_sleep = false;
                DEBUG_DETAIL("For FLUSH-->WAKING UP IN THREADS\n");
                in_th_wakeup();
            }
            pthread_mutex_unlock(&m_in_th_lock_1);

            pthread_mutex_lock(&m_out_th_lock_1);
            if(is_out_th_sleep)
            {
                is_out_th_sleep = false;
                DEBUG_DETAIL("For FLUSH-->WAKING UP OUT THREADS\n");
                out_th_wakeup();
            }
            pthread_mutex_unlock(&m_out_th_lock_1);

        while (1 )
        {
            pthread_mutex_lock(&out_buf_count_lock);
            pthread_mutex_lock(&in_buf_count_lock);
            DEBUG_PRINT("this=%p Flush:nNumOutputBuf = %d  %d nNumInputBuf=%d %d\n", this,\
                nNumOutputBuf,drv_out_buf_cnt, nNumInputBuf,drv_inp_buf_cnt);
            
            if(nNumInputBuf > 0 || nNumOutputBuf > 0)
            {
                pthread_mutex_unlock(&in_buf_count_lock);
                pthread_mutex_unlock(&out_buf_count_lock);
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                    is_in_th_sleep = false;
                    DEBUG_DETAIL("FLUSH-->WAKING UP IN THREAD\n");
                    in_th_wakeup();
                }
                pthread_mutex_unlock(&m_in_th_lock_1);

                pthread_mutex_lock(&m_out_th_lock_1);
                if ( is_out_th_sleep )
                {
                    is_out_th_sleep = false;
                    DEBUG_DETAIL("FLUSH-->WAKING UP OUT THREAD\n");
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
                DEBUG_PRINT("this=%p BEFORE READ ioctl_flush\n",this);

                if(ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1)
                    DEBUG_PRINT("Flush: ioctl flush failed %d\n",\
                    errno);
                DEBUG_PRINT("%p AFTER READ ioctl_flush\n", this);
                usleep (10000);
                DEBUG_PRINT("AFTER READ SEM_TIMEWAIT\n");
            }
            else
            {
                pthread_mutex_unlock(&in_buf_count_lock);
                pthread_mutex_unlock(&out_buf_count_lock);
                break;
            }
        }

        // sleep till the FLUSH ACK are done by both the input and
        // output threads
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%d",(signed)param1);
        wait_for_event();

        DEBUG_PRINT("RECIEVED BOTH FLUSH ACK's param1=%d cmd_cmpl=%d",\
            (signed)param1,cmd_cmpl);

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
    }
    else if (param1 == OMX_CORE_INPUT_PORT_INDEX)
    {
        DEBUG_PRINT("Execute FLUSH for I/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 1;
        pthread_mutex_unlock(&m_flush_lock);
        post_input(OMX_CommandFlush,
            OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        if(ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1)
        {
            DEBUG_PRINT("FLush:ioctl flush failed errno=%d\n",errno);
        }
        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
            is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");
        if(is_in_th_sleep)
        {
            pthread_mutex_lock(&m_in_th_lock_1);
            is_in_th_sleep = false;
            pthread_mutex_unlock(&m_in_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP IN THREADS\n");
            in_th_wakeup();
        }

        if(is_out_th_sleep)
        {
            pthread_mutex_lock(&m_out_th_lock_1);
            is_out_th_sleep = false;
            pthread_mutex_unlock(&m_out_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP OUT THREADS\n");
            out_th_wakeup();
        }
        // Send Flush to the kernel so that the in and out buffers are released
        // sleep till the FLUSH ACK are done by both the input and output thrds
        DEBUG_DETAIL("Executing FLUSH for I/p port\n");
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%d",(signed)param1);
        wait_for_event();
        DEBUG_DETAIL(" RECIEVED FLUSH ACK FOR I/P PORT param1=%d",(signed)param1);

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
        DEBUG_DETAIL("Executing FLUSH for O/p port\n");
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%d",(signed)param1);
        post_output(OMX_CommandFlush,
            OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        if(ioctl( m_drv_fd, AUDIO_OUTPORT_FLUSH, 0) == -1)
        {
            DEBUG_PRINT("AUDIO_OUTPORT flush ioctl failed errno=%d\n",errno);
            DEBUG_PRINT("m_drv_fd =%d\n",m_drv_fd);
        }
        pthread_mutex_lock(&m_out_th_lock_1);
        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
            is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");

        if(is_out_th_sleep)
        {
            //pthread_mutex_lock(&m_out_th_lock_1);
            is_out_th_sleep = false;
            pthread_mutex_unlock(&m_out_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP OUT THREADS\n");
            out_th_wakeup();
        }
        else
        {
            pthread_mutex_unlock(&m_out_th_lock_1);
        }

        // sleep till the FLUSH ACK are done by both the input and output thrds
        wait_for_event();
        // Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if(cmd_cmpl){
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                OMX_CommandFlush, OMX_CORE_OUTPUT_PORT_INDEX, NULL );
        }
        DEBUG_DETAIL("RECIEVED FLUSH ACK FOR O/P PORT param1=%d ",(signed)param1);
    }
    else
    {
        DEBUG_PRINT("Invalid Port ID[%d]",(signed)param1);

    }
    return bRet;
}

/**
 @brief member function that flushes buffers that are pending to be written
  to driver

 @param none
 @return bool value indicating whether flushing is carried out successfully
*/
bool omx_mp3_adec::execute_input_omx_flush()
{
    OMX_BUFFERHEADERTYPE *omx_buf;
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned      ident;
    unsigned       qsize=0; // qsize
    unsigned       tot_qsize=0; // qsize

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    DEBUG_PRINT("Execute_omx_flush on input port");

    do
    {
        pthread_mutex_lock(&m_inputlock);
        qsize = m_input_q.m_size;
        tot_qsize = qsize;
        tot_qsize += m_input_ctrl_ebd_q.m_size;
        pthread_mutex_unlock(&m_inputlock);

        pthread_mutex_lock(&in_buf_count_lock);
        tot_qsize += nNumInputBuf;
        pthread_mutex_unlock(&in_buf_count_lock);

        DEBUG_PRINT("Input FLUSH-->flushq[%d] ebd[%d]dataq[%d]drvbuf[%d]numin[%d]flush[%d]",\
            m_input_ctrl_cmd_q.m_size,
            m_input_ctrl_ebd_q.m_size,qsize,drv_inp_buf_cnt,nNumInputBuf,bFlushinprogress);
        if(!tot_qsize)
        {
            DEBUG_DETAIL("Input-->BREAKING FROM execute_input_flush LOOP");
            break;
        }
        if (qsize)
        {
            pthread_mutex_lock(&m_inputlock);
            m_input_q.pop_entry(&p1, &p2, &ident);
            pthread_mutex_unlock(&m_inputlock);
            if ((ident == OMX_COMPONENT_GENERATE_ETB))
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Flush:Input dataq=0x%x \n", (unsigned)omx_buf);
                omx_buf->nFilledLen = 0;
                m_flush_inbuf = true;
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                m_flush_inbuf = false;
            }
        }
        else if((m_input_ctrl_ebd_q.m_size))
        {
            pthread_mutex_lock(&m_inputlock);
            m_input_ctrl_ebd_q.pop_entry(&p1, &p2, &ident);
            pthread_mutex_unlock(&m_inputlock);
            if(ident == OMX_COMPONENT_GENERATE_BUFFER_DONE)
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                omx_buf->nFilledLen = 0;
                DEBUG_DETAIL("Flush:ctrl dataq=0x%x \n",(unsigned) omx_buf);
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
            }
        }
        else
        {
        DEBUG_PRINT("inside Input FLUSH-->flushq[%d] ebd[%d]dataq[%d]drvbuf[%d]numin[%d]flush[%d]",\
            m_input_ctrl_cmd_q.m_size,
            m_input_ctrl_ebd_q.m_size,qsize,drv_inp_buf_cnt,nNumInputBuf,bFlushinprogress);

        }
	if(((m_state != OMX_StateExecuting) && (m_state != OMX_StatePause) &&
				(m_state != OMX_StateInvalid)) || (m_comp_deinit))
	{
		DEBUG_PRINT_ERROR("IN EXITING FLUSH state=%d %d\n",m_state,m_comp_deinit);
		break;
	}
	tot_qsize = m_input_q.m_size;
        tot_qsize += m_input_ctrl_ebd_q.m_size;
        pthread_mutex_lock(&in_buf_count_lock);
        tot_qsize += nNumInputBuf;
        pthread_mutex_unlock(&in_buf_count_lock);

    }while((tot_qsize>0) && bFlushinprogress);
    DEBUG_DETAIL("*************************\n");
    DEBUG_PRINT("IN-->%p FLUSHING DONE\n", this);
    DEBUG_DETAIL("*************************\n");
    flush_ack();
    return true;
}
/**
 @brief member function that flushes buffers that are pending to be written
  to driver

 @param none
 @return bool value indicating whether flushing is carried out successfully
*/
bool omx_mp3_adec::execute_output_omx_flush()
{
    OMX_BUFFERHEADERTYPE *omx_buf;
    struct msm_audio_aio_buf  *aio_buf;
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned      ident;
    unsigned       qsize=0; // qsize
    unsigned       tot_qsize=0; // qsize
    DEBUG_PRINT("Execute_omx_flush on output port ");

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    DEBUG_PRINT("OUT FLUSH-->flushq[%d] fbd[%d]dataq[%d]drbuf[%d]state[%d]flush[%d] this[%p]numOut[%d] numin[%d] drvin[%d]",\
            m_output_ctrl_cmd_q.m_size,
            m_output_ctrl_fbd_q.m_size,qsize,drv_out_buf_cnt,m_state, bFlushinprogress,this,nNumOutputBuf, nNumInputBuf, drv_inp_buf_cnt);
    do
    {
        pthread_mutex_lock(&m_outputlock);
        qsize = m_output_q.m_size;
        tot_qsize = qsize;
        tot_qsize += m_output_ctrl_fbd_q.m_size;
        pthread_mutex_unlock(&m_outputlock);

        pthread_mutex_lock(&out_buf_count_lock);
        tot_qsize += nNumOutputBuf;
        pthread_mutex_unlock(&out_buf_count_lock);

        if(!tot_qsize)
        {
            DEBUG_DETAIL("OUT-->BREAKING FROM execute_input_flush LOOP");
            break;
        }
        if (qsize)
        {
            pthread_mutex_lock(&m_outputlock);
            m_output_q.pop_entry(&p1,&p2,&ident);
            pthread_mutex_unlock(&m_outputlock);
            if ( (ident == OMX_COMPONENT_GENERATE_FTB))
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Ouput Buf_Addr=%x TS[0x%x] \n",\
                    (unsigned)omx_buf,nTimestamp);
                omx_buf->nTimeStamp = nTimestamp;
                omx_buf->nFilledLen = 0;
                m_flush_outbuf = true;
                frame_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                m_flush_outbuf = false;
                DEBUG_DETAIL("CALLING FBD FROM FLUSH");
            }
        }
        else if((qsize = m_output_ctrl_fbd_q.m_size))
        {
            pthread_mutex_lock(&m_outputlock);
            m_output_ctrl_fbd_q.pop_entry(&p1, &p2, &ident);
            pthread_mutex_unlock(&m_outputlock);
            if(ident == OMX_COMPONENT_GENERATE_FRAME_DONE)
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Ouput Buf_Addr=%x TS[0x%x] \n", \
                   (unsigned) omx_buf,nTimestamp);
                omx_buf->nTimeStamp = nTimestamp;
                omx_buf->nFilledLen = 0;
                frame_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                DEBUG_DETAIL("CALLING FROM CTRL-FBDQ FROM FLUSH");
            }
        }
        else if((nNumOutputBuf > 0) && (m_suspend_drv_buf_cnt > 0) &&
                (tot_qsize == (OMX_U32)nNumOutputBuf) &&
                (tot_qsize == m_suspend_drv_buf_cnt))
        {
            DEBUG_PRINT_ERROR("inside OUT Flush Suspend buffer processing pending m_suspend_drv_buf_cnt %d \n",m_suspend_drv_buf_cnt);

            aio_buf = &m_suspend_out_drv_buf[m_suspend_drv_buf_cnt-1];
            omx_buf = (OMX_BUFFERHEADERTYPE *)aio_buf->private_data;
            omx_buf->nFilledLen = 0;
            omx_buf->nFlags = 0;
            omx_buf->nTimeStamp= nTimestamp;
            m_suspend_drv_buf_cnt--;
            if(!m_suspend_drv_buf_cnt)
            {
                    free(m_suspend_out_drv_buf);
                    m_suspend_out_drv_buf = NULL;
            }
            post_output((unsigned) &m_cmp, (unsigned int)omx_buf,
                            OMX_COMPONENT_GENERATE_FRAME_DONE);
        }
        else
        {
            DEBUG_PRINT("inside OUT FLUSH-->flushq[%d] fbd[%d]dataq[%d]drbuf[%d]state[%d]flush[%d] this[%p]numOut[%d] numin[%d] drvin[%d]",\
            m_output_ctrl_cmd_q.m_size,
            m_output_ctrl_fbd_q.m_size,qsize,drv_out_buf_cnt,m_state, bFlushinprogress,this,nNumOutputBuf, nNumInputBuf, drv_inp_buf_cnt);
        }

        if(((m_state != OMX_StateExecuting) && (m_state != OMX_StatePause) &&
                                (m_state != OMX_StateInvalid)) || (m_comp_deinit))
        {
           DEBUG_PRINT_ERROR("OUT EXITING FLUSH state=%d %d\n",m_state, m_comp_deinit);
           break;
        }
        tot_qsize = m_output_q.m_size;
        tot_qsize += m_output_ctrl_fbd_q.m_size;
        pthread_mutex_lock(&out_buf_count_lock);
        tot_qsize += nNumOutputBuf;
        pthread_mutex_unlock(&out_buf_count_lock);

    }while((tot_qsize>0) && bFlushinprogress );
    if (bGenerateEOSPending == true && m_output_eos_rxd == true)
	    post_output(0, 0, OMX_COMPONENT_GENERATE_EOS);
    DEBUG_DETAIL("*************************\n");
    DEBUG_PRINT("OUT-->FLUSHING DONE\n");
    DEBUG_DETAIL("*************************\n");
    flush_ack();

    return true;
}

/**
  @brief member function that posts command
  in the command queue

  @param p1 first paramter for the command
  @param p2 second parameter for the command
  @param id command ID
  @param lock self-locking mode
  @return bool indicating command being queued
 */
bool omx_mp3_adec::post_input(unsigned int p1,
                              unsigned int p2,
                              unsigned int id)
{
    bool bRet = false;
    pthread_mutex_lock(&m_inputlock);

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(id == OMX_COMPONENT_SUSPEND)
    {
        m_input_ctrl_q.insert_entry(p1,p2,id);
    }
    else if((OMX_COMPONENT_GENERATE_COMMAND == id))
    {
        // insert flush message and ebd
        m_input_ctrl_cmd_q.insert_entry(p1,p2,id);
    }
    else if((OMX_COMPONENT_GENERATE_BUFFER_DONE == id))
    {
        // insert ebd
        m_input_ctrl_ebd_q.insert_entry(p1,p2,id);
    }
    else
    {
        m_input_q.insert_entry(p1,p2,id);
    }
    if(m_ipc_to_in_th)
    {
        bRet = true;
        omx_mp3_post_msg(m_ipc_to_in_th, id);
    }

    DEBUG_DETAIL("PostInput-->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d] \n",\
        m_state,
        id,
        m_input_ctrl_cmd_q.m_size,
        m_input_ctrl_ebd_q.m_size,
        m_input_q.m_size);

    pthread_mutex_unlock(&m_inputlock);

    return bRet;
}

bool omx_mp3_adec::post_command(unsigned int p1,
                              unsigned int p2,
                              unsigned int id)
{
    bool bRet = false;

    pthread_mutex_lock(&m_commandlock);

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    m_command_q.insert_entry(p1,p2,id);
    if(m_ipc_to_cmd_th)
    {
        bRet = true;
        omx_mp3_post_msg(m_ipc_to_cmd_th, id);
    }

    DEBUG_DETAIL("PostCmd-->state[%d]id[%d]cmdq[%d]flags[%x]\n",\
        m_state,
        id,
        m_command_q.m_size,
        m_flags >> 3);

    pthread_mutex_unlock(&m_commandlock);

    return bRet;
}


bool omx_mp3_adec::post_output(unsigned int p1,
                              unsigned int p2,
                              unsigned int id)
{
    bool bRet = false;
    pthread_mutex_lock(&m_outputlock);

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if((id == OMX_COMPONENT_SUSPEND) || (id == OMX_COMPONENT_RESUME))
    {
        m_output_ctrl_q.insert_entry(p1,p2,id);
    }
    else if((OMX_COMPONENT_GENERATE_COMMAND == id) )
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
        omx_mp3_post_msg(m_ipc_to_out_th, id);
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
/**
  @brief member function that return parameters to IL client

  @param hComp handle to component instance
  @param paramIndex Parameter type
  @param paramData pointer to memory space which would hold the
        paramter
  @return error status
*/
OMX_ERRORTYPE  omx_mp3_adec::get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                           OMX_IN OMX_INDEXTYPE paramIndex,
                                           OMX_INOUT OMX_PTR     paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    (void)hComp;
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if(paramData == NULL)
    {
        DEBUG_PRINT("get_parameter: paramData is NULL\n");
        return OMX_ErrorBadParameter;
    }
    switch(paramIndex)
    {
    case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
            portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

            DEBUG_PRINT("OMX_IndexParamPortDefinition portDefn->nPortIndex = %u\n",\
               (unsigned) (portDefn->nPortIndex));

            portDefn->nVersion.nVersion = OMX_SPEC_VERSION;
            portDefn->nSize = sizeof(portDefn);
            portDefn->eDomain    = OMX_PortDomainAudio;

            if (0 == portDefn->nPortIndex)
            {
                portDefn->eDir =  OMX_DirInput;
                portDefn->bEnabled   = m_inp_bEnabled;
                portDefn->bPopulated = m_inp_bPopulated;
                portDefn->nBufferCountActual = m_inp_act_buf_count; //2; /* What if the component does not restrict how many buffer to take */
                portDefn->nBufferCountMin    = OMX_CORE_NUM_INPUT_BUFFERS; //2;
                portDefn->nBufferSize        = input_buffer_size;

                portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
                portDefn->format.audio.eEncoding = OMX_AUDIO_CodingMP3;
                portDefn->format.audio.pNativeRender = 0;


            }
            else if (1 == portDefn->nPortIndex)
            {
                portDefn->eDir =  OMX_DirOutput;
                portDefn->bEnabled   = m_out_bEnabled;
                portDefn->bPopulated = m_out_bPopulated;
                portDefn->nBufferCountActual = m_out_act_buf_count; /* What if the component does not restrict how many buffer to take */
                portDefn->nBufferCountMin    = OMX_CORE_NUM_OUTPUT_BUFFERS;
                portDefn->nBufferSize        = output_buffer_size;

                portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
                portDefn->format.audio.cMIMEType = mime_type;
                portDefn->format.audio.eEncoding = OMX_AUDIO_CodingPCM;
                portDefn->format.audio.pNativeRender = 0;

            }
            else
            {
                portDefn->eDir =  OMX_DirMax;
                DEBUG_PRINT_ERROR("Bad Port idx %d\n", (int)portDefn->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }

            break;
        }

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

    case OMX_IndexParamAudioPortFormat:
        {
            OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
                (OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
            DEBUG_PRINT("OMX_IndexParamAudioPortFormat\n");
            portFormatType->nVersion.nVersion = OMX_SPEC_VERSION;
            portFormatType->nSize = sizeof(portFormatType);

            if (OMX_CORE_INPUT_PORT_INDEX == portFormatType->nPortIndex)
            {
                portFormatType->eEncoding = OMX_AUDIO_CodingMP3;
            }
            else if(OMX_CORE_OUTPUT_PORT_INDEX== portFormatType->nPortIndex)
            {
                DEBUG_PRINT("get_parameter: OMX_IndexParamAudioFormat: %u\n",
                    (unsigned)(portFormatType->nIndex));
                portFormatType->eEncoding = OMX_AUDIO_CodingPCM;
            }
            else
            {
                DEBUG_PRINT_ERROR("get_parameter: Bad port index %d\n",
                    (int)portFormatType->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }

    case OMX_IndexParamAudioMp3:
        {
            OMX_AUDIO_PARAM_MP3TYPE *mp3Param = (OMX_AUDIO_PARAM_MP3TYPE *) paramData;
            DEBUG_PRINT("OMX_IndexParamAudioMp3\n");

            if (OMX_CORE_INPUT_PORT_INDEX== mp3Param->nPortIndex)
            {
                *mp3Param = m_adec_param;
            }
            else
            {
                DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioMp3 OMX_ErrorBadPortIndex %d\n", (int)mp3Param->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case QOMX_IndexParamAudioSessionId:
    {
       QOMX_AUDIO_STREAM_INFO_DATA *streaminfoparam =
               (QOMX_AUDIO_STREAM_INFO_DATA *) paramData;
       streaminfoparam->sessionId = m_session_id;
       break;
    }

    case OMX_IndexParamAudioPcm:
        {
            OMX_AUDIO_PARAM_PCMMODETYPE *pcmparam = (OMX_AUDIO_PARAM_PCMMODETYPE *) paramData;

            if (OMX_CORE_OUTPUT_PORT_INDEX== pcmparam->nPortIndex)
            {
                pcmparam->eNumData  =    m_pcm_param.eNumData;
                pcmparam->bInterleaved  = m_pcm_param.bInterleaved;
                pcmparam->nBitPerSample = m_pcm_param.nBitPerSample;
                pcmparam->ePCMMode = m_pcm_param.ePCMMode;
                pcmparam->eChannelMapping[0] = m_pcm_param.eChannelMapping[0];
                pcmparam->eChannelMapping[1] = m_pcm_param.eChannelMapping[1] ;

                pcmparam->nChannels = m_adec_param.nChannels;
                pcmparam->nSamplingRate = m_adec_param.nSampleRate;
                DEBUG_PRINT("get_parameter: Sampling rate %u",(unsigned) pcmparam->nSamplingRate);
                DEBUG_PRINT("get_parameter: Number of channels %u", (unsigned)pcmparam->nChannels);
            }
            else
            {
                DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioPcm OMX_ErrorBadPortIndex %u\n",(unsigned) pcmparam->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case OMX_IndexParamComponentSuspended:
        {
            OMX_PARAM_SUSPENSIONTYPE *suspend= (OMX_PARAM_SUSPENSIONTYPE *) paramData;
            if(bSuspendEventRxed)
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
            OMX_PORT_PARAM_TYPE *portParamType = (OMX_PORT_PARAM_TYPE *) paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamVideoInit\n");
            portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
            portParamType->nSize = sizeof(portParamType);
            portParamType->nPorts           = 0;
            portParamType->nStartPortNumber = 0;
            break;
        }
    case OMX_IndexParamPriorityMgmt:
        {
            OMX_PRIORITYMGMTTYPE *priorityMgmtType = (OMX_PRIORITYMGMTTYPE*)paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamPriorityMgmt\n");
            priorityMgmtType->nSize = sizeof(priorityMgmtType);
            priorityMgmtType->nVersion.nVersion = OMX_SPEC_VERSION;
            priorityMgmtType->nGroupID = m_priority_mgm.nGroupID;
            priorityMgmtType->nGroupPriority = m_priority_mgm.nGroupPriority;
            break;
        }
    case OMX_IndexParamImageInit:
        {
            OMX_PORT_PARAM_TYPE *portParamType = (OMX_PORT_PARAM_TYPE *) paramData;
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
            else if (OMX_CORE_OUTPUT_PORT_INDEX == bufferSupplierType->nPortIndex)
            {
                bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
            }
            else
            {
                eRet = OMX_ErrorBadPortIndex;
            }
            DEBUG_PRINT("get_parameter:OMX_IndexParamCompBufferSupplier \
                        eRet %08x\n", eRet);
            break;
        }

        /*Component should support this port definition*/
    case OMX_IndexParamOtherInit:
        {
            OMX_PORT_PARAM_TYPE *portParamType = (OMX_PORT_PARAM_TYPE *) paramData;
            DEBUG_PRINT("get_parameter: OMX_IndexParamOtherInit %08x\n", paramIndex);
            portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
            portParamType->nSize = sizeof(portParamType);
            portParamType->nPorts           = 0;
            portParamType->nStartPortNumber = 0;
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

/**
 @brief member function that set paramter from IL client

 @param hComp handle to component instance
 @param paramIndex parameter type
 @param paramData pointer to memory space which holds the paramter
 @return error status
 */
OMX_ERRORTYPE  omx_mp3_adec::set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                           OMX_IN OMX_INDEXTYPE paramIndex,
                                           OMX_IN OMX_PTR        paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    (void)hComp;
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Set Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if(paramData == NULL)
    {
        DEBUG_PRINT("param data is NULL");
        return OMX_ErrorBadParameter;
    }
    switch(paramIndex)
    {

    case OMX_IndexParamAudioMp3:
        {
            DEBUG_PRINT("SET-PARAM::OMX_IndexParamAudioMP3");
            m_adec_param = *((OMX_AUDIO_PARAM_MP3TYPE *) paramData);

            pSamplerate = m_adec_param.nSampleRate;
            pChannels = m_adec_param.nChannels;
            pBitrate = m_adec_param.nBitRate;

            if (OMX_CORE_INPUT_PORT_INDEX== m_adec_param.nPortIndex)
            {
                if(m_adec_param.nChannels == 1)
                {
                    frameDuration = (((OMX_MP3_OUTPUT_BUFFER_SIZE)* 1000) /(m_adec_param.nSampleRate * 2));
                    DEBUG_PRINT("frame duration of mono config = %d sampling rate = %u \n",frameDuration,
                    (unsigned)(m_adec_param.nSampleRate));
                }
                else if(m_adec_param.nChannels == 2)
                {
                    frameDuration = (((OMX_MP3_OUTPUT_BUFFER_SIZE)* 1000) /(m_adec_param.nSampleRate * 4));
                    DEBUG_PRINT("frame duration of stero config = %d sampling rate = %u \n",frameDuration,
                    (unsigned)(m_adec_param.nSampleRate));
                }
            }
            else
            {
                DEBUG_PRINT_ERROR("set_parameter: OMX_IndexParamAudioMP3 Bad port index %d\n", (int)m_adec_param.nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
            portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

            if(((m_state == OMX_StateLoaded)&&
                !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                || (m_state == OMX_StateWaitForResources &&
                ((OMX_DirInput == portDefn->eDir && m_inp_bEnabled == true)||
                (OMX_DirInput == portDefn->eDir && m_out_bEnabled == true)))
                ||(((OMX_DirInput == portDefn->eDir && m_inp_bEnabled == false)||
                (OMX_DirInput == portDefn->eDir && m_out_bEnabled == false)) &&
                (m_state != OMX_StateWaitForResources)))
            {
                DEBUG_PRINT("Set Parameter called in valid state\n");
            }
            else
            {
                DEBUG_PRINT_ERROR("Set Parameter called in Invalid State\n");
                return OMX_ErrorIncorrectStateOperation;
            }
            DEBUG_PRINT("OMX_IndexParamPortDefinition portDefn->nPortIndex = %u\n",\
               (unsigned) (portDefn->nPortIndex));
            if (OMX_CORE_INPUT_PORT_INDEX == portDefn->nPortIndex)
            {
                DEBUG_PRINT("SET_PARAMETER:OMX_IndexParamPortDefinition port[%u]"
                    "bufsize[%u] buf_cnt[%u]\n",(unsigned)(portDefn->nPortIndex),
                   (unsigned)( portDefn->nBufferSize),(unsigned)( portDefn->nBufferCountActual));

                if(portDefn->nBufferCountActual > OMX_CORE_NUM_INPUT_BUFFERS)
                {
                    m_inp_act_buf_count = portDefn->nBufferCountActual;
                }
                else
                {
                    m_inp_act_buf_count = OMX_CORE_NUM_INPUT_BUFFERS;
                }

                if(portDefn->nBufferSize > input_buffer_size)
                {
                    input_buffer_size = portDefn->nBufferSize;
                }
                else
                {
                    input_buffer_size = OMX_CORE_INPUT_BUFFER_SIZE;
                }

            }
            else if (OMX_CORE_OUTPUT_PORT_INDEX == portDefn->nPortIndex)
            {
                DEBUG_PRINT("SET_PARAMETER:OMX_IndexParamPortDefinition port[%u]"
                    "bufsize[%u] buf_cnt[%u]\n",(unsigned)(portDefn->nPortIndex),
                    (unsigned)(portDefn->nBufferSize),(unsigned) (portDefn->nBufferCountActual));

                if(portDefn->nBufferCountActual > OMX_CORE_NUM_OUTPUT_BUFFERS)
                {
                    m_out_act_buf_count = portDefn->nBufferCountActual;
                }
                else
                {
                    m_out_act_buf_count = OMX_CORE_NUM_OUTPUT_BUFFERS;
                }

                if(portDefn->nBufferSize > output_buffer_size)
                {
                    output_buffer_size  = portDefn->nBufferSize;
                }
                else
                {
                    output_buffer_size = OMX_MP3_OUTPUT_BUFFER_SIZE;
                }

            }
            else
            {
                DEBUG_PRINT_ERROR(" set_parameter: Bad Port idx %d",
                    (int)portDefn->nPortIndex);

                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }
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
               (unsigned)( priorityMgmtype->nGroupID));
            DEBUG_PRINT("set_parameter: priorityMgmtype %u\n",
               (unsigned) (priorityMgmtype->nGroupPriority));
            m_priority_mgm.nGroupID = priorityMgmtype->nGroupID;
            m_priority_mgm.nGroupPriority = priorityMgmtype->nGroupPriority;
            break;
        }
    case  OMX_IndexParamAudioPortFormat:
        {
            OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
                (OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
            DEBUG_PRINT("set_parameter: OMX_IndexParamAudioPortFormat\n");

            if (OMX_CORE_INPUT_PORT_INDEX== portFormatType->nPortIndex)
            {
                portFormatType->eEncoding = OMX_AUDIO_CodingMP3;
            }
            else if(OMX_CORE_OUTPUT_PORT_INDEX == portFormatType->nPortIndex)
            {
                DEBUG_PRINT("set_parameter: OMX_IndexParamAudioFormat: %u\n",
                    (unsigned)(portFormatType->nIndex));
                portFormatType->eEncoding = OMX_AUDIO_CodingPCM;
            }
            else
            {
                DEBUG_PRINT_ERROR("set_parameter: Bad port index %d\n",
                    (int)portFormatType->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }


    case OMX_IndexParamCompBufferSupplier:
        {
            DEBUG_PRINT("set_parameter: OMX_IndexParamCompBufferSupplier\n");
            OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType
                = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
            DEBUG_PRINT("set_parameter: OMX_IndexParamCompBufferSupplier %d\n",
                bufferSupplierType->eBufferSupplier);

            if(bufferSupplierType->nPortIndex == OMX_CORE_INPUT_PORT_INDEX
                || bufferSupplierType->nPortIndex == OMX_CORE_OUTPUT_PORT_INDEX)
            {
                DEBUG_PRINT("set_parameter: OMX_IndexParamCompBufferSupplier In/Out put \n");
                m_buffer_supplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;
            }
            else
            {
                eRet = OMX_ErrorBadPortIndex;
            }

            DEBUG_PRINT("set_parameter:OMX_IndexParamCompBufferSupplier: \
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
                m_pcm_param.nChannels =  pcmparam->nChannels;
                m_pcm_param.eNumData = pcmparam->eNumData;
                m_pcm_param.bInterleaved = pcmparam->bInterleaved;
                m_pcm_param.nBitPerSample =   pcmparam->nBitPerSample;
                m_pcm_param.nSamplingRate =   pcmparam->nSamplingRate;
                m_pcm_param.ePCMMode =  pcmparam->ePCMMode;
                m_pcm_param.eChannelMapping[0] =  pcmparam->eChannelMapping[0];
                m_pcm_param.eChannelMapping[1] =  pcmparam->eChannelMapping[1];
                DEBUG_PRINT("set_parameter: Sampling rate %u",(unsigned)( pcmparam->nSamplingRate));
                DEBUG_PRINT("set_parameter: Number of channels %d",(unsigned) (pcmparam->nChannels));
            }
            else
            {
                DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioPcm \
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
            DEBUG_PRINT("SET_PARAMETER: Set SUSPENSION POLICY %d  m_ipc_to_event_th=%p\n",
                suspensionPolicy,m_ipc_to_event_th);
            break;
        }
    case OMX_IndexParamStandardComponentRole:
        {
            OMX_PARAM_COMPONENTROLETYPE *componentRole;
            componentRole = (OMX_PARAM_COMPONENTROLETYPE*)paramData;
            component_Role.nSize = componentRole->nSize;
            component_Role.nVersion = componentRole->nVersion;
            strcpy((char *)component_Role.cRole,(const char*)componentRole->cRole);
            DEBUG_PRINT("SET_PARAMETER: role = %s\n",  component_Role.cRole);
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

/* ======================================================================
FUNCTION
  omx_mp3_adec::GetConfig

DESCRIPTION
  OMX Get Config Method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::get_config(OMX_IN OMX_HANDLETYPE      hComp,
                                        OMX_IN OMX_INDEXTYPE configIndex,
                                        OMX_INOUT OMX_PTR     configData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
   (void)hComp;
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Config in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
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
            } else {
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
            } else {
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
  omx_mp3_adec::SetConfig

DESCRIPTION
  OMX Set Config method implementation

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.
========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::set_config(OMX_IN OMX_HANDLETYPE      hComp,
                                        OMX_IN OMX_INDEXTYPE configIndex,
                                        OMX_IN OMX_PTR        configData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
   (void)hComp;
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Set Config in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if( m_state == OMX_StateExecuting)
    {
        DEBUG_PRINT_ERROR("set_config:Ignore in Exe state\n");
        return OMX_ErrorInvalidState;
    }
    switch(configIndex)
    {
    case OMX_IndexConfigAudioVolume:
        {
            OMX_AUDIO_CONFIG_VOLUMETYPE *vol = (OMX_AUDIO_CONFIG_VOLUMETYPE*)
                configData;
            if (vol->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
            {
                if ((vol->sVolume.nValue <= OMX_ADEC_MAX) &&
                    (vol->sVolume.nValue >= OMX_ADEC_MIN)) {
                        m_volume = vol->sVolume.nValue;
                        if (BITMASK_ABSENT(&m_flags, OMX_COMPONENT_MUTED))
                        {
                            /* ioctl(m_drv_fd, AUDIO_VOLUME, m_volume * OMX_ADEC_VOLUME_STEP); */
                        }

                } else {
                    eRet = OMX_ErrorBadParameter;
                }
            } else {
                eRet = OMX_ErrorBadPortIndex;
            }
        }
        break;

    case OMX_IndexConfigAudioMute:
        {
            OMX_AUDIO_CONFIG_MUTETYPE *mute = (OMX_AUDIO_CONFIG_MUTETYPE*)
                configData;
            if (mute->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
            {
                if (mute->bMute == OMX_TRUE) {
                    BITMASK_SET(&m_flags, OMX_COMPONENT_MUTED);
                    /* ioctl(m_drv_fd, AUDIO_VOLUME, 0); */
                } else {
                    BITMASK_CLEAR(&m_flags, OMX_COMPONENT_MUTED);
                    /* ioctl(m_drv_fd, AUDIO_VOLUME, m_volume * OMX_ADEC_VOLUME_STEP); */
                }
            } else {
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
  omx_mp3_adec::GetExtensionIndex

DESCRIPTION
  OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::get_extension_index(OMX_IN OMX_HANDLETYPE      hComp,
                                                OMX_IN OMX_STRING      paramName,
                                                OMX_OUT OMX_INDEXTYPE* indexType)
{
  DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
  if((hComp == NULL) || (paramName == NULL) || (indexType == NULL))
  {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
  }
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Extension Index in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
  if(strncmp(paramName,"OMX.Qualcomm.index.audio.sessionId",strlen("OMX.Qualcomm.index.audio.sessionId")) == 0)
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
  omx_mp3_adec::GetState

DESCRIPTION
  Returns the state information back to the caller.<TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::get_state(OMX_IN OMX_HANDLETYPE  hComp,
                                       OMX_OUT OMX_STATETYPE* state)
{
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
   (void)hComp;
    *state = m_state;
	//DEBUG_PRINT("Returning the state %d\n",*state);
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_mp3_adec::ComponentTunnelRequest

DESCRIPTION
  OMX Component Tunnel Request method implementation. <TBD>

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::component_tunnel_request(OMX_IN OMX_HANDLETYPE                hComp,
                                                     OMX_IN OMX_U32                        port,
                                                     OMX_IN OMX_HANDLETYPE        peerComponent,
                                                     OMX_IN OMX_U32                    peerPort,
                                                     OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup)
{
    (void)hComp;
    (void)port;
    (void)peerComponent;
    (void)peerPort;
    (void)tunnelSetup;
    DEBUG_PRINT_ERROR("Error: component_tunnel_request Not Implemented\n");
    return OMX_ErrorNotImplemented;
}

#if 0 //may use latter
/* Round the buffer sizes given by the OMX Client to nearest power of 2 */
static unsigned round_to_powerof2(unsigned int x)
{
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >>16);
    return x + 1;
}
#endif
void* omx_mp3_adec::alloc_ion_buffer(unsigned int bufsize)
{
    struct mmap_info *ion_data = NULL;
    struct ion_allocation_data alloc_data;
    struct ion_fd_data fd_data;

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    ion_data = (struct mmap_info*) calloc(sizeof(struct mmap_info), 1);

    if(!ion_data)
    {
        DEBUG_PRINT("\n alloc_ion_buffer: ion_data allocation failed\n");
        return NULL;
    }

    if (ion_fd< 0)
    {
        DEBUG_PRINT ("\n ion driver not yet opened");
        free(ion_data);
        return NULL;
    }

    /* Align the size wrt the page boundary size of 4k */
    ion_data->map_buf_size = (bufsize + 4095) & (~4095);
    /* Round the size to the nearest power of 2 */
    //ion_data->map_buf_size = round_to_powerof2(ion_data->map_buf_size);

    alloc_data.len = ion_data->map_buf_size;
    alloc_data.align = 0x1000;
#ifdef QCOM_AUDIO_USE_SYSTEM_HEAP_ID
    alloc_data.heap_mask = ION_HEAP(ION_SYSTEM_HEAP_ID);
#else
    alloc_data.heap_mask = ION_HEAP(ION_AUDIO_HEAP_ID);
#endif
    alloc_data.flags = 0;

    int rc = ioctl(ion_fd, ION_IOC_ALLOC, &alloc_data);
    if (rc) {
        DEBUG_PRINT("ION_IOC_ALLOC ioctl failed\n");
        free(ion_data);
        return NULL;
    }

    fd_data.handle = alloc_data.handle;
    rc = ioctl(ion_fd, ION_IOC_SHARE, &fd_data);
    if (rc) {
        DEBUG_PRINT("ION_IOC_SHARE ioctl failed\n");
        rc = ioctl(ion_fd, ION_IOC_FREE, &(alloc_data.handle));
        if (rc) {
            DEBUG_PRINT("ION_IOC_FREE ioctl failed\n");
        }
        free(ion_data);
        return NULL;
    }

    /* Map the ion file descriptor into current process address space */
    ion_data->pBuffer = (OMX_U8*) mmap( NULL,
        ion_data->map_buf_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd_data.fd,
        0
        );

    if(MAP_FAILED == ion_data->pBuffer)
    {
        DEBUG_PRINT ("\n mmap() failed");
        close(fd_data.fd);
        int rc = ioctl(ion_fd, ION_IOC_FREE, &(alloc_data.handle));
        if (rc) {
            DEBUG_PRINT_ERROR("ION_IOC_FREE ioctl failed\n");
        }
        free(ion_data);
        return NULL;
    }

    ion_data->ion_fd = fd_data.fd;
    return ion_data;
}

void omx_mp3_adec::free_ion_buffer(void** mem_buffer)
{
    struct mmap_info** ion_data = (struct mmap_info**) mem_buffer;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(ion_data && (*ion_data))
    {
        if ((*ion_data)->pBuffer &&
            (EINVAL == munmap ((*ion_data)->pBuffer, (*ion_data)->map_buf_size)))
        {
            DEBUG_PRINT ("\n Error in Unmapping the buffer %p\n",
                (*ion_data)->pBuffer);
        }
        (*ion_data)->pBuffer = NULL;

        close((*ion_data)->ion_fd);
        ion_user_handle_t handle = (*ion_data)->handle;
        int rc = ioctl(ion_fd, ION_IOC_FREE, &handle);
        if (rc) {
            DEBUG_PRINT_ERROR("ION_IOC_FREE ioctl failed\n");
        }
        (*ion_data)->ion_fd = -1;

        free(*ion_data);
        *ion_data = NULL;
    }
    else
    {
        DEBUG_PRINT ("\n free_ion_buffer: Invalid input parameter\n");
    }
}

/* ======================================================================
FUNCTION
  omx_mp3_adec::AllocateInputBuffer

DESCRIPTION
  Helper function for allocate buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::allocate_input_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                                  OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                                  OMX_IN OMX_U32                        port,
                                                  OMX_IN OMX_PTR                     appData,
                                                  OMX_IN OMX_U32                       bytes)
{
    struct mmap_info *mmap_data = NULL;
    OMX_U8 *map_buffer = NULL;
    struct msm_audio_ion_info audio_ion_buf ;
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE            *bufHdr;
    unsigned                   nBufSize = bytes;
    char                       *buf_ptr;
    unsigned int nMapBufSize = 0;
    struct ion_allocation_data alloc_data;
    struct ion_fd_data fd_data;

    (void)hComp;
    (void)port;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    DEBUG_PRINT("Inside omx_mp3_adec::allocate_input_buffer");
    if(bytes < input_buffer_size)
    {
        /* return if i\p buffer size provided by client is less
        than min i\p buffer size supported by omx component */
        DEBUG_PRINT("\nError: bytes[%u] < input_buffer_size[%u]\n",(unsigned) bytes,
            input_buffer_size);
        return OMX_ErrorInsufficientResources;
    }

    if(m_inp_current_buf_count < m_inp_act_buf_count)
    {
        buf_ptr = (char *) calloc( sizeof(OMX_BUFFERHEADERTYPE) , 1);

        if (buf_ptr != NULL)
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;

            if (ion_fd < 0)
            {
                free(buf_ptr);
                DEBUG_PRINT ("\n ion driver not yet opened");
                return OMX_ErrorInsufficientResources;
            }

            if(pcm_feedback)
            {
                /* Add META_IN size + 2 internal bytes for odd address & odd byte handling */
                nMapBufSize = nBufSize + sizeof(META_IN) + 2;
            }
            else
            {
                /* Add 2 internal bytes for odd address & odd byte handling */
                nMapBufSize = nBufSize + 2;
            }

            /* Align the size wrt the page boundary size of 4k */
            nMapBufSize = (nMapBufSize + 4095) & (~4095);
            /* Round the size to the nearest power of 2 */
            //nMapBufSize = round_to_powerof2(nMapBufSize);

            alloc_data.len =   nMapBufSize;
            alloc_data.align = 0x1000;
#ifdef QCOM_AUDIO_USE_SYSTEM_HEAP_ID
            alloc_data.heap_mask = ION_HEAP(ION_SYSTEM_HEAP_ID);
#else
            alloc_data.heap_mask = ION_HEAP(ION_AUDIO_HEAP_ID);
#endif
            alloc_data.flags = 0;

            int rc = ioctl(ion_fd, ION_IOC_ALLOC, &alloc_data);
            if (rc) {
                DEBUG_PRINT("ION_IOC_ALLOC ioctl failed\n");
                map_buffer = NULL;
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }

            fd_data.handle = alloc_data.handle;
            rc = ioctl(ion_fd, ION_IOC_SHARE, &fd_data);
            if (rc) {
                DEBUG_PRINT("ION_IOC_SHARE ioctl failed\n");
                rc = ioctl(ion_fd, ION_IOC_FREE, &(alloc_data.handle));
                if (rc) {
                    DEBUG_PRINT("ION_IOC_FREE ioctl failed\n");
                }
                map_buffer = NULL;
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }

            /* Map the ion file descriptor into current process address space */
            map_buffer = (OMX_U8*) mmap( NULL,
                nMapBufSize,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd_data.fd,
                0
                );

            if(MAP_FAILED == map_buffer)
            {
                DEBUG_PRINT ("\n mmap() failed");
                map_buffer = NULL;
                close(fd_data.fd);
                rc = ioctl(ion_fd, ION_IOC_FREE, &(alloc_data.handle));
                if (rc) {
                    DEBUG_PRINT("ION_IOC_FREE ioctl failed\n");
                }
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }

            if(pcm_feedback)
            {
                bufHdr->pBuffer = map_buffer + sizeof(META_IN);
            }
            else
            {
                bufHdr->pBuffer = map_buffer;
            }

            mmap_data = (struct mmap_info*) calloc(sizeof(struct mmap_info), 1);

            if(mmap_data == NULL)
            {
                DEBUG_PRINT ("\n mmap_data allocation FAILED\n");
                munmap(bufHdr->pBuffer, nMapBufSize);
                close(fd_data.fd);
                int rc = ioctl(ion_fd, ION_IOC_FREE, &fd_data.handle);
                if (rc) {
                    DEBUG_PRINT_ERROR("ION_IOC_FREE ioctl failed\n");
                }
                map_buffer = NULL;
                bufHdr->pBuffer = NULL;
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }

            mmap_data->pBuffer = (void*)map_buffer;
            mmap_data->ion_fd = fd_data.fd;
            mmap_data->handle = alloc_data.handle;
            mmap_data->map_buf_size = nMapBufSize;

            /* Register the mapped ion buffer with the MP3 driver */
            audio_ion_buf.fd = fd_data.fd;
            audio_ion_buf.vaddr = map_buffer;
            if(0 > ioctl(m_drv_fd, AUDIO_REGISTER_ION, &audio_ion_buf))
            {
                DEBUG_PRINT("\n Error in ioctl AUDIO_REGISTER_ION\n");
                return OMX_ErrorHardware;
            }

            *bufferHdr = bufHdr;

            DEBUG_PRINT("Allocate_input:bufHdr %x bufHdr->pBuffer %x",(unsigned) bufHdr,
              (unsigned)  bufHdr->pBuffer);

            bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
            m_input_buf_hdrs.insert(bufHdr, (OMX_BUFFERHEADERTYPE*)mmap_data);
            m_inp_current_buf_count++;
        }
        else
        {
            DEBUG_PRINT("Allocate_input:I/P buffer memory allocation failed\n");
            eRet = OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG_PRINT("\nCould not allocate more buffers than ActualBufCnt\n");
        eRet = OMX_ErrorInsufficientResources;
    }

    return eRet;
}

OMX_ERRORTYPE  omx_mp3_adec::allocate_output_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                                  OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                                  OMX_IN OMX_U32                        port,
                                                  OMX_IN OMX_PTR                     appData,
                                                  OMX_IN OMX_U32                       bytes)
{
    struct mmap_info *mmap_data = NULL;
    OMX_U8 *map_buffer = NULL;
    struct msm_audio_ion_info audio_ion_buf ;
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE            *bufHdr;
    unsigned                   nBufSize = bytes;
    char                       *buf_ptr;
    unsigned int nMapBufSize = 0;
    struct ion_allocation_data alloc_data;
    struct ion_fd_data fd_data;

    (void)hComp;
   (void)port;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    DEBUG_PRINT("Inside omx_mp3_adec::allocate_output_buffer");
    if(bytes < output_buffer_size)
    {
        /* return if o\p buffer size provided by client is less
        than min o\p buffer size supported by omx component */
        DEBUG_PRINT("\nError: bytes[%u] < output_buffer_size[%u]\n", (unsigned)bytes,
            output_buffer_size);
        return OMX_ErrorInsufficientResources;
    }

    if(m_out_current_buf_count < m_out_act_buf_count)
    {
        buf_ptr = (char *) calloc( sizeof(OMX_BUFFERHEADERTYPE) , 1);

        if (buf_ptr != NULL)
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;

            if (ion_fd < 0)
            {
                free(buf_ptr);
                DEBUG_PRINT ("\n ion driver not yet opened");
                return OMX_ErrorInsufficientResources;
            }

            nMapBufSize = nBufSize + sizeof(META_OUT);

            /* Align the size wrt the page boundary size of 4k */
            nMapBufSize = (nMapBufSize + 4095) & (~4095);
            /* Round the size to the nearest power of 2 */
            //nMapBufSize = round_to_powerof2(nMapBufSize);

            alloc_data.len =   nMapBufSize;
            alloc_data.align = 0x1000;
#ifdef QCOM_AUDIO_USE_SYSTEM_HEAP_ID
            alloc_data.heap_mask = ION_HEAP(ION_SYSTEM_HEAP_ID);
#else
            alloc_data.heap_mask = ION_HEAP(ION_AUDIO_HEAP_ID);
#endif
            alloc_data.flags = 0;

            int rc = ioctl(ion_fd, ION_IOC_ALLOC, &alloc_data);
            if (rc) {
                DEBUG_PRINT("ION_IOC_ALLOC ioctl failed\n");
                map_buffer = NULL;
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }

            fd_data.handle = alloc_data.handle;
            rc = ioctl(ion_fd, ION_IOC_SHARE, &fd_data);
            if (rc) {
                DEBUG_PRINT("ION_IOC_SHARE ioctl failed\n");
                rc = ioctl(ion_fd, ION_IOC_FREE, &(alloc_data.handle));
                if (rc) {
                    DEBUG_PRINT("ION_IOC_FREE ioctl failed\n");
                }
                map_buffer = NULL;
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }

            /* Map the ion file descriptor into current process address space */
            map_buffer = (OMX_U8*) mmap( NULL,
                nMapBufSize,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd_data.fd,
                0
                );

            if(MAP_FAILED == map_buffer)
            {
                DEBUG_PRINT ("\n mmap() failed");
                map_buffer = NULL;
                close(fd_data.fd);
                rc = ioctl(ion_fd, ION_IOC_FREE, &(alloc_data.handle));
                if (rc) {
                    DEBUG_PRINT("ION_IOC_FREE ioctl failed\n");
                }
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }

            bufHdr->pBuffer = map_buffer + sizeof(META_OUT);

            mmap_data = (struct mmap_info*) calloc(sizeof(struct mmap_info), 1);

            if(mmap_data == NULL)
            {
                DEBUG_PRINT ("\n mmap_data allocation FAILED\n");
                munmap(bufHdr->pBuffer, nMapBufSize);
                close(fd_data.fd);
                int rc = ioctl(ion_fd, ION_IOC_FREE, &fd_data.handle);
                if (rc) {
                    DEBUG_PRINT_ERROR("ION_IOC_FREE ioctl failed\n");
                }
                map_buffer = NULL;
                bufHdr->pBuffer = NULL;
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }

            mmap_data->pBuffer = (void*)map_buffer;
            mmap_data->ion_fd = fd_data.fd;
            mmap_data->handle = alloc_data.handle;
            mmap_data->map_buf_size = nMapBufSize;

            /* Register the mapped ion buffer with the MP3 driver */
            audio_ion_buf.fd = fd_data.fd;
            audio_ion_buf.vaddr = map_buffer;
            if(0 > ioctl(m_drv_fd, AUDIO_REGISTER_ION, &audio_ion_buf))
            {
                DEBUG_PRINT("\n Error in ioctl AUDIO_REGISTER_ION\n");
                return OMX_ErrorHardware;
            }

            *bufferHdr = bufHdr;

            DEBUG_PRINT("Allocate_output:bufHdr %x bufHdr->pBuffer %x",(unsigned) bufHdr,
               (unsigned)( bufHdr->pBuffer));

            bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nOutputPortIndex  = OMX_CORE_OUTPUT_PORT_INDEX;
            bufHdr->pOutputPortPrivate = (void*)ion_fd;
            m_output_buf_hdrs.insert(bufHdr, (OMX_BUFFERHEADERTYPE*)mmap_data);
            m_out_current_buf_count++;
        }
        else
        {
            DEBUG_PRINT("Allocate_output:O/P buffer memory allocation failed\n");
            eRet = OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG_PRINT("\nCould not allocate more buffers than ActualBufCnt\n");
        eRet = OMX_ErrorInsufficientResources;
    }
    return eRet;
}


// AllocateBuffer  -- API Call
/* ======================================================================
FUNCTION
  omx_mp3_adec::AllocateBuffer

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::allocate_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                     OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                     OMX_IN OMX_U32                        port,
                                     OMX_IN OMX_PTR                     appData,
                                     OMX_IN OMX_U32                       bytes)
{

    OMX_ERRORTYPE eRet = OMX_ErrorNone; // OMX return type
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(m_state == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Allocate Buf in Invalid State\n");
        return OMX_ErrorInvalidState;
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
        DEBUG_PRINT_ERROR("allocate_buffer:Error--> Invalid Port Index received %d\n",
            (int)port);
        eRet = OMX_ErrorBadPortIndex;
    }

    if((eRet == OMX_ErrorNone))
    {
        DEBUG_PRINT("Checking for Output Allocate buffer Done");
        if(allocate_done())
        {
            m_is_alloc_buf++;
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
                m_out_bEnabled = OMX_TRUE;
                bOutputPortReEnabled = 1;
                //m_out_bPopulated = OMX_TRUE;
                DEBUG_PRINT("AllocBuf-->is_out_th_sleep=%d\n",is_out_th_sleep);
                pthread_mutex_lock(&m_out_th_lock_1);
                if(is_out_th_sleep)
                {
                    is_out_th_sleep = false;
                    DEBUG_DETAIL("AllocBuf:WAKING UP OUT THREADS\n");
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                    is_in_th_sleep = false;
                    DEBUG_DETAIL("AB:WAKING UP IN THREADS\n");
                    in_th_wakeup();
                }
                pthread_mutex_unlock(&m_in_th_lock_1);
                post_command(OMX_CommandPortEnable, OMX_CORE_OUTPUT_PORT_INDEX,
                    OMX_COMPONENT_GENERATE_EVENT);
            }
        }
    }
    DEBUG_PRINT("Allocate Buffer exit with ret Code[%d] port[%u]\n", eRet,(unsigned)port);
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_mp3_adec::UseBuffer

DESCRIPTION
  OMX Use Buffer method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None , if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::use_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes,
                         OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
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
                m_out_bPopulated = OMX_TRUE;
                bOutputPortReEnabled = 1;
                DEBUG_PRINT("UseBuf-->is_out_th_sleep=%d\n",is_out_th_sleep);
                pthread_mutex_lock(&m_out_th_lock_1);
                if(is_out_th_sleep)
                {
                    is_out_th_sleep = false;
                    DEBUG_DETAIL("UseBuf:WAKING UP OUT THREADS\n");
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                    is_in_th_sleep = false;
                    DEBUG_DETAIL("UB:WAKING UP IN THREADS\n");
                    in_th_wakeup();
                }
                pthread_mutex_unlock(&m_in_th_lock_1);
            }
        }
    }
    DEBUG_PRINT("Use Buffer for port[%u] eRet[%d]\n", (unsigned)port,eRet);
    return eRet;
}
/* ======================================================================
FUNCTION
  omx_mp3_adec::UseInputBuffer

DESCRIPTION
  Helper function for Use buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::use_input_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes,
                         OMX_IN OMX_U8*                   buffer)
{
    struct mmap_info *mmap_data = NULL;
    struct msm_audio_ion_info audio_ion_buf ;
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE            *bufHdr = NULL, *loc_bufHdr = NULL;
    OMX_U8 *tmp_bufptr = NULL;
    unsigned                   nBufSize = bytes;
    char                       *buf_ptr = NULL, *loc_buf_ptr = NULL;
    unsigned int nMapBufSize = 0;
    (void)hComp;
    (void)port;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    DEBUG_PRINT("Inside omx_mp3_adec::use_input_buffer");
    if(bytes < input_buffer_size)
    {
        /* return if i\p buffer size provided by client is less
        than min i\p buffer size supported by omx component */
        DEBUG_PRINT("\nError: bytes[%u] < input_buffer_size[%u]\n",(unsigned) bytes,
            input_buffer_size);
        return OMX_ErrorInsufficientResources;
    }

    if(m_inp_current_buf_count < m_inp_act_buf_count)
    {
        buf_ptr = (char *) calloc( sizeof(OMX_BUFFERHEADERTYPE) , 1);
        loc_buf_ptr = (char *) calloc( sizeof(OMX_BUFFERHEADERTYPE) , 1);

        if (buf_ptr != NULL && loc_buf_ptr != NULL)
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            loc_bufHdr = (OMX_BUFFERHEADERTYPE *) loc_buf_ptr;

            if (ion_fd < 0)
            {
                free(loc_buf_ptr);
                free(buf_ptr);
                DEBUG_PRINT ("\n ion driver not yet opened");
                return OMX_ErrorInsufficientResources;
            }

            if(pcm_feedback)
            {
                /* Add META_IN size + 2 internal bytes for odd address & odd byte handling */
                nMapBufSize = nBufSize + sizeof(META_IN) + 2;
            }
            else
            {
                /* Add 2 internal bytes for odd address & odd byte handling */
                nMapBufSize = nBufSize + 2;
            }

            /* Align the size wrt the page boundary size of 4k */
            nMapBufSize = (nMapBufSize + 4095) & (~4095);
            /* Round the size to the nearest power of 2 */
            //nMapBufSize = round_to_powerof2(nMapBufSize);

            struct ion_allocation_data alloc_data;
            struct ion_fd_data fd_data;

            alloc_data.len =   nMapBufSize;
            alloc_data.align = 0x1000;
#ifdef QCOM_AUDIO_USE_SYSTEM_HEAP_ID
            alloc_data.heap_mask = ION_HEAP(ION_SYSTEM_HEAP_ID);
#else
            alloc_data.heap_mask = ION_HEAP(ION_AUDIO_HEAP_ID);
#endif
            alloc_data.flags = 0;

            int rc = ioctl(ion_fd, ION_IOC_ALLOC, &alloc_data);
            if (rc) {
                DEBUG_PRINT("ION_IOC_ALLOC ioctl failed\n");
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }
            fd_data.handle = alloc_data.handle;

            rc = ioctl(ion_fd, ION_IOC_SHARE, &fd_data);
            if (rc) {
                DEBUG_PRINT("ION_IOC_SHARE ioctl failed\n");
                rc = ioctl(ion_fd, ION_IOC_FREE, &(alloc_data.handle));
                if (rc) {
                    DEBUG_PRINT("ION_IOC_FREE ioctl failed\n");
                }
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }

            /* Map the ion file descriptor into current process address space */
            tmp_bufptr = (OMX_U8*) mmap( NULL,
                nMapBufSize,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd_data.fd,
                0
                );

            if(MAP_FAILED == tmp_bufptr)
            {
                DEBUG_PRINT ("\n mmap() failed");
                free(loc_buf_ptr);
                free(buf_ptr);
                close(fd_data.fd);
                rc = ioctl(ion_fd, ION_IOC_FREE, &(alloc_data.handle));
                if (rc) {
                    DEBUG_PRINT("ION_IOC_FREE ioctl failed\n");
                }
                return OMX_ErrorInsufficientResources;
            }

            mmap_data = (struct mmap_info*) calloc(sizeof(struct mmap_info), 1);

            if(mmap_data == NULL)
            {
                DEBUG_PRINT ("\n mmap_data allocation FAILED\n");
                munmap(tmp_bufptr, nMapBufSize);
                close(fd_data.fd);
                int rc = ioctl(ion_fd, ION_IOC_FREE, &fd_data.handle);
                if (rc) {
                    DEBUG_PRINT_ERROR("ION_IOC_FREE ioctl failed\n");
                }
                free(loc_buf_ptr);
                free(buf_ptr);
                return OMX_ErrorInsufficientResources;
            }

            mmap_data->pBuffer = (void*)tmp_bufptr;
            mmap_data->ion_fd = fd_data.fd;
            mmap_data->handle = alloc_data.handle;
            mmap_data->map_buf_size = nMapBufSize;

            /* Register the mapped ION buffer with the MP3 driver */
            audio_ion_buf.fd = fd_data.fd;
            audio_ion_buf.vaddr = tmp_bufptr;
            if(0 > ioctl(m_drv_fd, AUDIO_REGISTER_ION, &audio_ion_buf))
            {
                DEBUG_PRINT("\n Error in ioctl AUDIO_REGISTER_ION\n");
                return OMX_ErrorHardware;
            }

            *bufferHdr = bufHdr;

            bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
            bufHdr->nOffset           = 0;

            memcpy(loc_bufHdr, bufHdr, sizeof(OMX_BUFFERHEADERTYPE));
            bufHdr->pBuffer = (OMX_U8 *)(buffer);

            if(pcm_feedback)
            {
                loc_bufHdr->pBuffer = tmp_bufptr + sizeof(META_IN);
            }
            else
            {
                loc_bufHdr->pBuffer = tmp_bufptr;
            }

            DEBUG_PRINT("\nUse_input:bufHdr %x bufHdr->pBuffer %x", (unsigned)bufHdr,
               (unsigned)( bufHdr->pBuffer));
            DEBUG_PRINT("\nUse_input:locbufHdr %x locbufHdr->pBuffer %x",
               (unsigned) loc_bufHdr, (unsigned)(loc_bufHdr->pBuffer));

            m_input_buf_hdrs.insert(bufHdr, (OMX_BUFFERHEADERTYPE*)mmap_data);
            m_loc_in_use_buf_hdrs.insert(bufHdr, loc_bufHdr);
            m_loc_in_use_buf_hdrs.insert(loc_bufHdr, bufHdr);

            m_inp_current_buf_count++;

            if(m_inp_current_buf_count == m_inp_act_buf_count)
            {
                m_in_use_buf_case = true;
            }
        }
        else
        {
            DEBUG_PRINT("Useinput:I/P buffer header memory allocation failed\n");
            if(buf_ptr)
            {
                free(buf_ptr);
                buf_ptr = NULL;
            }

            if(loc_buf_ptr)
            {
                free(loc_buf_ptr);
                loc_buf_ptr = NULL;
            }

            eRet = OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG_PRINT("\nCould not use more buffers than ActualBufCnt\n");
        eRet = OMX_ErrorInsufficientResources;
    }

    return eRet;

}

/* ======================================================================
FUNCTION
  omx_mp3_adec::UseOutputBuffer

DESCRIPTION
  Helper function for Use buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::use_output_buffer(
                         OMX_IN OMX_HANDLETYPE            hComp,
                         OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                         OMX_IN OMX_U32                   port,
                         OMX_IN OMX_PTR                   appData,
                         OMX_IN OMX_U32                   bytes,
                         OMX_IN OMX_U8*                   buffer)
{
    struct mmap_info *mmap_data = NULL;
    struct msm_audio_ion_info audio_ion_buf ;
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE            *bufHdr = NULL, *loc_bufHdr = NULL;
    OMX_U8 *tmp_bufptr = NULL;
    unsigned                   nBufSize = bytes;
    char                       *buf_ptr = NULL, *loc_buf_ptr = NULL;
    unsigned int nMapBufSize = 0;
    struct ion_allocation_data alloc_data;
    struct ion_fd_data fd_data;

    (void)hComp;
    (void)port;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    DEBUG_PRINT("Inside omx_mp3_adec::use_output_buffer");
    if(bytes < output_buffer_size)
    {
        /* return if o\p buffer size provided by client is less
        than min o\p buffer size supported by omx component */
        DEBUG_PRINT("\nError: bytes[%u] < output_buffer_size[%u]\n",(unsigned) bytes,
            output_buffer_size);
        return OMX_ErrorInsufficientResources;
    }


    if(m_out_current_buf_count < m_out_act_buf_count)
    {
        buf_ptr = (char *) calloc( sizeof(OMX_BUFFERHEADERTYPE) , 1);
        loc_buf_ptr = (char *) calloc( sizeof(OMX_BUFFERHEADERTYPE) , 1);

        if (buf_ptr != NULL && loc_buf_ptr != NULL)
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            loc_bufHdr = (OMX_BUFFERHEADERTYPE *) loc_buf_ptr;

            if (ion_fd < 0)
            {
                free(loc_buf_ptr);
                free(buf_ptr);
                DEBUG_PRINT ("\n ion driver not yet opened");
                return OMX_ErrorInsufficientResources;
            }

            nMapBufSize = nBufSize + sizeof(META_OUT);

            /* Align the size wrt the page boundary size of 4k */
            nMapBufSize = (nMapBufSize + 4095) & (~4095);
            /* Round the size to the nearest power of 2 */
            //nMapBufSize = round_to_powerof2(nMapBufSize);

            DEBUG_PRINT ("\n Size of Mp3 output buffer %d",nBufSize);
            /* Map the ION file descriptor into current process address space */

            alloc_data.len =   nMapBufSize;
            alloc_data.align = 0x1000;
#ifdef QCOM_AUDIO_USE_SYSTEM_HEAP_ID
            alloc_data.heap_mask = ION_HEAP(ION_SYSTEM_HEAP_ID);
#else
            alloc_data.heap_mask = ION_HEAP(ION_AUDIO_HEAP_ID);
#endif
            alloc_data.flags = 0;

            int rc = ioctl(ion_fd, ION_IOC_ALLOC, &alloc_data);
            if (rc) {
                DEBUG_PRINT("ION_IOC_ALLOC ioctl failed\n");
                free(buf_ptr);
                close(ion_fd);
                return OMX_ErrorInsufficientResources;
            }

            fd_data.handle = alloc_data.handle;
            rc = ioctl(ion_fd, ION_IOC_SHARE, &fd_data);
            if (rc) {
                DEBUG_PRINT("ION_IOC_SHARE ioctl failed\n");
                rc = ioctl(ion_fd, ION_IOC_FREE, &(alloc_data.handle));
                if (rc) {
                    DEBUG_PRINT("ION_IOC_FREE ioctl failed\n");
                }
                free(buf_ptr);
                close(ion_fd);
                return OMX_ErrorInsufficientResources;
            }

            /* Map the ion file descriptor into current process address space */
            tmp_bufptr = (OMX_U8*) mmap( NULL,
                nMapBufSize,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd_data.fd,
                0
                );

            if(MAP_FAILED == tmp_bufptr)
            {
                DEBUG_PRINT ("\n mmap() failed");
                free(loc_buf_ptr);
                free(buf_ptr);
                close(fd_data.fd);
                rc = ioctl(ion_fd, ION_IOC_FREE, &(alloc_data.handle));
                if (rc) {
                    DEBUG_PRINT("ION_IOC_FREE ioctl failed\n");
                }
                return OMX_ErrorInsufficientResources;
            }

            mmap_data = (struct mmap_info*) calloc(sizeof(struct mmap_info), 1);

            if(mmap_data == NULL)
            {
                DEBUG_PRINT ("\n mmap_data allocation FAILED\n");
                munmap(tmp_bufptr, nMapBufSize);
                close(fd_data.fd);
                int rc = ioctl(ion_fd, ION_IOC_FREE, &fd_data.handle);
                if (rc) {
                    DEBUG_PRINT_ERROR("ION_IOC_FREE ioctl failed\n");
                }
                free(loc_buf_ptr);
                free(buf_ptr);
                close(ion_fd);
                return OMX_ErrorInsufficientResources;
            }

            mmap_data->pBuffer = (void*)tmp_bufptr;
            mmap_data->ion_fd = fd_data.fd;
            mmap_data->handle = alloc_data.handle;
            mmap_data->map_buf_size = nMapBufSize;

            /* Register the mapped ION buffer with the MP3 driver */
            audio_ion_buf.fd = fd_data.fd;
            audio_ion_buf.vaddr = tmp_bufptr;
            if(0 > ioctl(m_drv_fd, AUDIO_REGISTER_ION, &audio_ion_buf))
            {
                DEBUG_PRINT("\n Error in ioctl AUDIO_REGISTER_ION\n");
                return OMX_ErrorHardware;
            }

            *bufferHdr = bufHdr;

            bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nOutputPortIndex  = OMX_CORE_OUTPUT_PORT_INDEX;
            bufHdr->nOffset           = 0;

            memcpy(loc_bufHdr, bufHdr, sizeof(OMX_BUFFERHEADERTYPE));
            bufHdr->pBuffer = (OMX_U8 *)(buffer);
            loc_bufHdr->pBuffer = tmp_bufptr + sizeof(META_OUT);

            DEBUG_PRINT("Use_Output:bufHdr %x bufHdr->pBuffer %x size %d ",
                (unsigned)bufHdr,(unsigned) bufHdr->pBuffer,nBufSize);

            m_output_buf_hdrs.insert(bufHdr, (OMX_BUFFERHEADERTYPE*)mmap_data);
            m_loc_out_use_buf_hdrs.insert(bufHdr, loc_bufHdr);
            m_loc_out_use_buf_hdrs.insert(loc_bufHdr, bufHdr);

            m_out_current_buf_count++;

            if(m_out_current_buf_count == m_out_act_buf_count)
            {
                m_out_use_buf_case = true;
            }
        }
        else
        {
            DEBUG_PRINT("Useoutput:O/P buffer hdr memory allocation failed\n");
            if(buf_ptr)
            {
                free(buf_ptr);
                buf_ptr = NULL;
            }

            if(loc_buf_ptr)
            {
                free(loc_buf_ptr);
                loc_buf_ptr = NULL;
            }

            eRet = OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG_PRINT("\nCould not use more buffers than ActualBufCnt\n");
        eRet = OMX_ErrorInsufficientResources;
    }

    return eRet;

}

/**
 @brief member function that searches for caller buffer

 @param buffer pointer to buffer header
 @return bool value indicating whether buffer is found
 */
bool omx_mp3_adec::search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{
    bool eRet = false;
    OMX_BUFFERHEADERTYPE *temp = NULL;

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    //access only in IL client context
    temp = m_input_buf_hdrs.find_ele(buffer);
    if(buffer && temp)
    {
        DEBUG_DETAIL("search_input_bufhdr %x \n",(unsigned) buffer);
        eRet = true;
    }
    return eRet;
}

/**
 @brief member function that searches for caller buffer

 @param buffer pointer to buffer header
 @return bool value indicating whether buffer is found
 */
bool omx_mp3_adec::search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{
    bool eRet = false;
    OMX_BUFFERHEADERTYPE *temp = NULL;

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);

    //access only in IL client context
    temp = m_output_buf_hdrs.find_ele(buffer);
    if(buffer && temp)
    {
        DEBUG_DETAIL("search_output_bufhdr %x \n", (unsigned)buffer);
        eRet = true;
    }

    return eRet;
}

// Free Buffer - API call
/**
  @brief member function that handles free buffer command from IL client

  This function is a block-call function that handles IL client request to
  freeing the buffer

  @param hComp handle to component instance
  @param port id of port which holds the buffer
  @param buffer buffer header
  @return Error status
*/
OMX_ERRORTYPE  omx_mp3_adec::free_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                      OMX_IN OMX_U32                 port,
                                      OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_BUFFERHEADERTYPE *bufHdr = buffer;
    struct mmap_info *mmap_data = NULL;
    struct msm_audio_ion_info audio_ion_buf;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
   (void)hComp;

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    DEBUG_PRINT("Free_Buffer buf %x port=%u\n", (unsigned)buffer,(unsigned)port);

    if(m_state == OMX_StateIdle &&
        (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
    {
        DEBUG_PRINT(" free buffer while Component in Loading pending\n");
    }
    else if((m_inp_bEnabled == OMX_FALSE && port == OMX_CORE_INPUT_PORT_INDEX)||
        (m_out_bEnabled == OMX_FALSE && port == OMX_CORE_OUTPUT_PORT_INDEX))
    {
        DEBUG_PRINT("Free Buffer while port %u disabled\n", (unsigned)port);
    }
    else if(m_state == OMX_StateExecuting || m_state == OMX_StatePause)
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
                /* Buffer exist */
                //access only in IL client context
                DEBUG_PRINT("Free_Buf:in_buffer[%p]\n",buffer);
                mmap_data = (struct mmap_info*) m_input_buf_hdrs.find(buffer);

                if(m_in_use_buf_case)
                {
                    bufHdr = m_loc_in_use_buf_hdrs.find(buffer);
                }

                if(mmap_data)
                {
                    audio_ion_buf.fd = mmap_data->ion_fd;
                    audio_ion_buf.vaddr = mmap_data->pBuffer;
                    if(0 > ioctl(m_drv_fd, AUDIO_DEREGISTER_ION, &audio_ion_buf))
                    {
                        DEBUG_PRINT("\n Error in ioctl AUDIO_DEREGISTER_ION\n");
                    }

                    if (mmap_data->pBuffer &&
                        (EINVAL == munmap (mmap_data->pBuffer, mmap_data->map_buf_size)))
                    {
                        DEBUG_PRINT ("\n Error in Unmapping the buffer %p", bufHdr);
                    }
                    mmap_data->pBuffer = NULL;

                    close(mmap_data->ion_fd);
                    ion_user_handle_t handle= mmap_data->handle;
                    int rc = ioctl(ion_fd, ION_IOC_FREE, &handle);
                    if (rc) {
                        DEBUG_PRINT_ERROR("ION_IOC_FREE ioctl failed\n");
                    }
                    mmap_data->ion_fd = -1;
                    free(mmap_data);
                    mmap_data = NULL;
                }

                if(m_in_use_buf_case && bufHdr)
                {
                    m_loc_in_use_buf_hdrs.erase(buffer);
                    m_loc_in_use_buf_hdrs.erase(bufHdr);
                    bufHdr->pBuffer = NULL;
                    free(bufHdr);
                    bufHdr = NULL;
                }

                m_input_buf_hdrs.erase(buffer);
                if(buffer)
                {
                    free(buffer);
                    buffer = NULL;
                }

                m_inp_current_buf_count--;

                if(!m_inp_current_buf_count)
                {
                    m_in_use_buf_case = false;
                }

                DEBUG_PRINT("Free_Buf:in_buffer[%p] input buffer count = %d\n", buffer,
                    m_inp_current_buf_count);
            }
            else
            {
                DEBUG_PRINT_ERROR("Free_Buf:Error-->free_buffer, \
                                  Invalid Input buffer header\n");
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
            bOutputPortReEnabled = 0;
            DEBUG_PRINT("INPUT PORT MOVING TO DISABLED STATE \n");
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
                /* Buffer exist */
                //access only in IL client context
                mmap_data = (struct mmap_info*) m_output_buf_hdrs.find(buffer);

                if(m_out_use_buf_case)
                {
                    bufHdr = m_loc_out_use_buf_hdrs.find(buffer);
                }

                if(mmap_data)
                {
                    audio_ion_buf.fd = mmap_data->ion_fd;
                    audio_ion_buf.vaddr = mmap_data->pBuffer;
                    if(0 > ioctl(m_drv_fd, AUDIO_DEREGISTER_ION, &audio_ion_buf))
                    {
                        DEBUG_PRINT("\n Error in ioctl AUDIO_DEREGISTER_ION\n");
                    }

                    if (mmap_data->pBuffer &&
                        (EINVAL == munmap (mmap_data->pBuffer, mmap_data->map_buf_size)))
                    {
                        DEBUG_PRINT ("\n Error in Unmapping the buffer %p", bufHdr);
                    }
                    mmap_data->pBuffer = NULL;

                    close(mmap_data->ion_fd);
                    ion_user_handle_t handle= mmap_data->handle;
                    int rc = ioctl(ion_fd, ION_IOC_FREE, &handle);
                    if (rc) {
                        DEBUG_PRINT_ERROR("ION_IOC_FREE ioctl failed\n");
                    }
                    mmap_data->ion_fd = -1;
                    free(mmap_data);
                    mmap_data = NULL;
                }

                if(m_out_use_buf_case && bufHdr)
                {
                    m_loc_out_use_buf_hdrs.erase(buffer);
                    m_loc_out_use_buf_hdrs.erase(bufHdr);
                    bufHdr->pBuffer = NULL;
                    free(bufHdr);
                    bufHdr = NULL;
                }

                m_output_buf_hdrs.erase(buffer);
                if(buffer)
                {
                    free(buffer);
                    buffer = NULL;
                }

                m_out_current_buf_count--;

                if(!m_out_current_buf_count)
                {
                    m_out_use_buf_case = false;
                }

                DEBUG_PRINT("Free_Buf:out_buffer[%p] output buffer count = %d\n",buffer,m_out_current_buf_count);
            }
            else
            {
                DEBUG_PRINT("Free_Buf:Error-->free_buffer , \
                            Invalid Output buffer header\n");
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
            bOutputPortReEnabled = 0;
            DEBUG_PRINT("OUTPUT PORT MOVING TO DISABLED STATE \n");
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
        if(release_done(OMX_ALL))
        {
            if(ioctl(m_drv_fd,AUDIO_ABORT_GET_EVENT,NULL) < 0)
                DEBUG_PRINT("AUDIO ABORT_GET_EVENT in free buffer failed\n");
            else
                DEBUG_PRINT("AUDIO ABORT_GET_EVENT in free buffer passed\n");

            if (m_ipc_to_event_th != NULL)
            {
                omx_mp3_thread_stop(m_ipc_to_event_th);
                m_ipc_to_event_th = NULL;
            }
            if(ioctl(m_drv_fd, AUDIO_STOP, 0) < 0)
                DEBUG_PRINT("AUDIO STOP in free buffer failed\n");
            else
                DEBUG_PRINT("AUDIO STOP in free buffer passed\n");
            m_first_mp3_header= 0;
            m_is_alloc_buf = 0;

            DEBUG_PRINT("Free_Buf: Free buffer\n");

            // Send the callback now
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_LOADING_PENDING);
            DEBUG_PRINT("Before OMX_StateLoaded OMX_COMPONENT_GENERATE_EVENT\n");
            post_command(OMX_CommandStateSet,
                OMX_StateLoaded,OMX_COMPONENT_GENERATE_EVENT);
            DEBUG_PRINT("After OMX_StateLoaded OMX_COMPONENT_GENERATE_EVENT\n");
        }
    }

    return eRet;
}


/**
 @brief member function that that handles empty this buffer command

 This function meremly queue up the command and data would be consumed
 in command server thread context

 @param hComp handle to component instance
 @param buffer pointer to buffer header
 @return error status
 */
OMX_ERRORTYPE  omx_mp3_adec::empty_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                              OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    OMX_STATETYPE state;
    pthread_mutex_lock(&m_state_lock);
    get_state(&m_cmp, &state);
    pthread_mutex_unlock(&m_state_lock);
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if ( state == OMX_StateInvalid )
    {
        DEBUG_PRINT("Empty this buffer in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if(buffer->nFilledLen > buffer->nAllocLen)
    {
        DEBUG_PRINT("ETB: buffer->nFilledLen[%u] > buffer->nAllocLen[%u]",
           (unsigned) buffer->nFilledLen,(unsigned) buffer->nAllocLen);
        return OMX_ErrorBadParameter;
    }

    if ( (buffer != NULL) &&
        (buffer->nInputPortIndex == 0) &&
        (buffer->nSize == sizeof (OMX_BUFFERHEADERTYPE) &&
        (buffer->nVersion.nVersion == OMX_SPEC_VERSION)) &&
        (m_inp_bEnabled ==OMX_TRUE)&&
        (search_input_bufhdr(buffer) == true))
    {
        pthread_mutex_lock(&in_buf_count_lock);
        nNumInputBuf++;
        pthread_mutex_unlock(&in_buf_count_lock);
        PrintFrameHdr(OMX_COMPONENT_GENERATE_ETB,buffer);

        post_input((unsigned)hComp,
            (unsigned) buffer,OMX_COMPONENT_GENERATE_ETB);
    }
    else
    {
        DEBUG_PRINT_ERROR("Bad header %x \n", (int)buffer);
        eRet = OMX_ErrorBadParameter;
        if ( m_inp_bEnabled ==OMX_FALSE )
        {
            DEBUG_PRINT("ETB OMX_ErrorIncorrectStateOperation Port Status %d \n",\
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

/**
  @brief member function that writes data to kernel driver

  @param hComp handle to component instance
  @param buffer pointer to buffer header
  @return error status
 */
OMX_ERRORTYPE  omx_mp3_adec::empty_this_buffer_proxy(OMX_IN OMX_HANDLETYPE         hComp,
                                                     OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_BUFFERHEADERTYPE *bufHdr = buffer;
    OMX_U8 *tmpbuffer = NULL;
    struct msm_audio_aio_buf audio_aio_buf;
    META_IN *pmeta_in = NULL;
    struct mp3_header mp3_header_info;
    int res = 0, rc = 0;
    struct msm_audio_config drv_config;
    unsigned int new_length = buffer->nFilledLen;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(m_in_use_buf_case)
    {
        DEBUG_PRINT("\nUseBuffer flag SET\n");
        bufHdr = m_loc_in_use_buf_hdrs.find(buffer);

        if(!bufHdr)
        {
            DEBUG_PRINT("UseBufhdr for buffer[%p] is NULL", buffer);
            return OMX_ErrorBadParameter;
        }

        if(new_length)
        {
            if(m_odd_byte_set)
            {
                memcpy(bufHdr->pBuffer+1, buffer->pBuffer, new_length);
            }
            else
            {
                memcpy(bufHdr->pBuffer, buffer->pBuffer, new_length);
            }
        }

        tmpbuffer = bufHdr->pBuffer;
        memcpy(bufHdr, buffer, sizeof(OMX_BUFFERHEADERTYPE));
        bufHdr->pBuffer = tmpbuffer;
    }

    if(m_input_eos_rxd == true)
    {
        DEBUG_PRINT("\n ETBP: Input EOS Reached!! Invalid ETB call\n");
        bufHdr->nFilledLen = 0;
        bufHdr->nFlags &= ~OMX_BUFFERFLAG_EOS;
        post_input((unsigned) &m_cmp,(unsigned) bufHdr,
            OMX_COMPONENT_GENERATE_BUFFER_DONE);
        return OMX_ErrorNone;
    }

    if(m_odd_byte_set)
    {
        if(new_length && !m_in_use_buf_case)
        {
            memmove(bufHdr->pBuffer+1, bufHdr->pBuffer, new_length);
        }

        *bufHdr->pBuffer = m_odd_byte;
        new_length++;
    }

    if(new_length == 1)
    {
        new_length++;
        *(bufHdr->pBuffer+1) = 0;
    }

    /* Odd length Buffer handling */
    if(new_length%2 == 1)
    {
        m_odd_byte_set = true;
        m_odd_byte = *(bufHdr->pBuffer + new_length - 1);
        new_length--;
    }
    else
    {
        m_odd_byte_set = false;
    }


    if(m_first_mp3_header == 0)
    {
        m_first_mp3_header = 1; /* No more parsing after first frame*/
        DEBUG_PRINT("ETBP: Callin mp3 frame header parsing\n");
        res = mp3_frameheader_parser( (OMX_BUFFERHEADERTYPE*)bufHdr,
            &mp3_header_info);
        if(mp3_header_info.Layer != 0x01)
        {
            DEBUG_PRINT_ERROR("Unsupported MP3 Layer format: %d\n",mp3_header_info.Layer);
            m_first_mp3_header= 0;
            post_command((unsigned)OMX_CommandStateSet,
                         (unsigned)OMX_StateInvalid,
                         OMX_COMPONENT_GENERATE_COMMAND);
            post_command(OMX_CommandFlush,-1,
                         OMX_COMPONENT_GENERATE_COMMAND);

            buffer_done_cb((OMX_BUFFERHEADERTYPE *)bufHdr);
            return OMX_ErrorInvalidState;
        }

        if(res ==0)
        {
            ioctl(m_drv_fd, AUDIO_GET_CONFIG, &drv_config);
            drv_config.sample_rate = mp3_header_info.sampling_rate;
            drv_config.channel_count = mp3_header_info.channel_mode;
            DEBUG_PRINT("sample_rate %d channel_count %d\n",drv_config.sample_rate,drv_config.channel_count);
            drv_config.type = 2; // mp3 decoding

            if(pcm_feedback)
            {

                drv_config.meta_field = 1;
                OMX_U32 th_val = 1;
                printf("th_val = %lu\n",th_val);
                if(ioctl(m_drv_fd, AUDIO_SET_ERR_THRESHOLD_VALUE, &th_val) == -1)
                DEBUG_PRINT("set-AUDIO_SET_ERR_THRESHOLD_VALUE ioctl failed %d\n",errno);
                else
                DEBUG_PRINT("set-AUDIO_SET_ERR_THRESHOLD_VALUE  success\n");

            }
            else
            {
                drv_config.meta_field = 0;
            }
            DEBUG_PRINT("\n DRIVER CONFIG BUFFER SIZE: %d\n",drv_config.buffer_size);

            if(ioctl(m_drv_fd, AUDIO_SET_CONFIG, &drv_config) < 0)
                DEBUG_PRINT_ERROR("AUDIO_SETCONFIG FAILED\n");

            rc = ioctl(m_drv_fd, AUDIO_START, 0);
            if(rc <0)
            {
                DEBUG_PRINT_ERROR("AUDIO_START FAILED for Handle %p with error no.:%d \n",hComp,errno);
                m_first_mp3_header= 0;
                post_command((unsigned)OMX_CommandStateSet,
                             (unsigned)OMX_StateInvalid,
                              OMX_COMPONENT_GENERATE_COMMAND);
                post_command(OMX_CommandFlush,-1,
                             OMX_COMPONENT_GENERATE_COMMAND);

                 pthread_mutex_lock(&in_buf_count_lock);
                 pthread_mutex_unlock(&in_buf_count_lock);
                 buffer_done_cb((OMX_BUFFERHEADERTYPE *)bufHdr);
                 return OMX_ErrorInvalidState;
            }
            else
                DEBUG_PRINT("AUDIO_START SUCCESS for Handle: %p\n",hComp);
            if ((pcm_feedback)&&
               (( m_adec_param.nSampleRate != mp3_header_info.sampling_rate) ||
                ( m_adec_param.nChannels != mp3_header_info.channel_mode) ))
            {
                m_adec_param.nSampleRate = mp3_header_info.sampling_rate;
                m_adec_param.nChannels = mp3_header_info.channel_mode;
                op_settings_changed = 1;
                DEBUG_PRINT("OMX_COMPONENT_PORTSETTINGS_CHANGED\n");
                bOutputPortReEnabled=0;
                {

                    DEBUG_PRINT("ETBP:: SLEEPING IN THREAD\n");
                    pthread_mutex_lock(&m_in_th_lock_1);
                    is_in_th_sleep = true;
                    pthread_mutex_unlock(&m_in_th_lock_1);
                    post_command((unsigned) & hComp,(unsigned) bufHdr,OMX_COMPONENT_PORTSETTINGS_CHANGED);
                    in_th_goto_sleep();
                    DEBUG_PRINT("ETBP: -->IN thread woken up by somebody\n");
                }
            }
            else
            {
                m_out_bEnabled = OMX_TRUE;
                bOutputPortReEnabled = 1;
                DEBUG_PRINT("OMX COMPONENT PORTSETTINGS NOT CHANGED\n");
                pthread_mutex_lock(&m_out_th_lock_1);
                if ( is_out_th_sleep )
                {
                    is_out_th_sleep = false;
                    DEBUG_DETAIL("ETBP:WAKING UP OUT THREADS\n");
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                    DEBUG_PRINT("ETBP:WAKING UP IN THREADS\n");
                    in_th_wakeup();
                    is_in_th_sleep = false;
                }
                pthread_mutex_unlock(&m_in_th_lock_1);
            }
        }
        else
        {
            DEBUG_PRINT("ETBP:configure Driver for MP3 sample rate = %u \n",(unsigned) (m_adec_param.nSampleRate));

            ioctl(m_drv_fd, AUDIO_GET_CONFIG, &drv_config);
            drv_config.sample_rate = m_adec_param.nSampleRate;
            drv_config.channel_count = m_adec_param.nChannels;
            drv_config.type = 2; // mp3 decoding
            // drv_config.buffer_size = input_buffer_size;
            if(pcm_feedback)
            {

                drv_config.meta_field = 1;
            }
            else
            {
                drv_config.meta_field = 0;
            }
            DEBUG_PRINT("\n DRIVER CONFIG BUFFER SIZE 2 : %d\n",drv_config.buffer_size);
            if(ioctl(m_drv_fd, AUDIO_SET_CONFIG, &drv_config) < 0)
                DEBUG_PRINT_ERROR("AUDIO_SETCONFIG FAILED\n");

            rc = ioctl(m_drv_fd, AUDIO_START, 0);

            if(rc <0)
            {
                DEBUG_PRINT_ERROR("2 AUDIO_START FAILED for Handle:%p with error %d\n",hComp,errno);
                m_first_mp3_header= 0;
                 pthread_mutex_lock(&in_buf_count_lock);
                 pthread_mutex_unlock(&in_buf_count_lock);
                      post_command((unsigned)OMX_CommandStateSet,
                             (unsigned)OMX_StateInvalid,
                              OMX_COMPONENT_GENERATE_COMMAND);
                post_command(OMX_CommandFlush,-1,
                             OMX_COMPONENT_GENERATE_COMMAND);
                 buffer_done_cb((OMX_BUFFERHEADERTYPE *)bufHdr);
                        return OMX_ErrorInvalidState;
            }
            else
                DEBUG_PRINT("2 AUDIO_START SUCCESS for Handle: %p\n",hComp);
            m_out_bEnabled = OMX_TRUE;
            bOutputPortReEnabled = 1;
            DEBUG_PRINT("OMX COMPONENT PORTSETTINGS NOT CHANGED\n");
            pthread_mutex_lock(&m_out_th_lock_1);
            if ( is_out_th_sleep )
            {
                is_out_th_sleep = false;
                DEBUG_DETAIL("ETBP:WAKING UP OUT THREADS\n");
                out_th_wakeup();
            }
            pthread_mutex_unlock(&m_out_th_lock_1);
            pthread_mutex_lock(&m_in_th_lock_1);
            if(is_in_th_sleep)
            {
                DEBUG_PRINT("ETBP:WAKING UP IN THREADS\n");
                in_th_wakeup();
                is_in_th_sleep = false;
            }
            pthread_mutex_unlock(&m_in_th_lock_1);
        }
    }

    if (pcm_feedback)//Non-Tunnelled mode
    {
        pmeta_in = (META_IN*) (bufHdr->pBuffer - sizeof(META_IN));
        if(pmeta_in)
        {
            // copy the metadata info from the BufHdr and insert to payload
            pmeta_in->offsetVal  = sizeof(META_IN);
            pmeta_in->nTimeStamp = (((OMX_BUFFERHEADERTYPE*)bufHdr)->nTimeStamp);
            if(!(bufHdr->nFlags & OMX_BUFFERFLAG_EOS))
                nLastTimeStamp = (((OMX_BUFFERHEADERTYPE*)bufHdr)->nTimeStamp);
            pmeta_in->nFlags     = bufHdr->nFlags;
        }
        else
        {
            DEBUG_PRINT_ERROR("\n Invalid pmeta_in(NULL)\n");
            return OMX_ErrorUndefined;
        }
    }
    DEBUG_PRINT ("\n Before Write");
    // EOS support with dependancy on Kernel and DSP
    if( (bufHdr->nFlags & OMX_BUFFERFLAG_EOS))
    {
        ntotal_playtime = bufHdr->nTimeStamp;
        DEBUG_PRINT("ETBP:EOS OCCURED, Total playtime %d \n", ntotal_playtime);

        if(new_length == 0)
        {
            m_input_eos_rxd = true;
        }
        else
        {
            if(pcm_feedback)
            {
                pmeta_in->nTimeStamp = nLastTimeStamp;
                pmeta_in->nFlags &= ~OMX_BUFFERFLAG_EOS;
            }

        }
    }

    /* For tunneled case, EOS buffer with 0 length not pushed */
    if(!pcm_feedback && (bufHdr->nFlags & OMX_BUFFERFLAG_EOS) && new_length == 0)
    {
        pthread_mutex_lock(&in_buf_count_lock);
        nNumInputBuf--;
        DEBUG_PRINT("ETBP: nNumInputBuf %d\n",nNumInputBuf);
        pthread_mutex_unlock(&in_buf_count_lock);

        DEBUG_PRINT("\nETBP: EOS with 0 length in Tunneled Mode\n");
    }
    else
    {
        DEBUG_PRINT("ETBP: Before write nFlags[%u] len[%d]\n",(unsigned) bufHdr->nFlags,
            new_length);

        /* Asynchronous write call to the MP3 driver */
        audio_aio_buf.buf_len = bufHdr->nAllocLen;
        audio_aio_buf.private_data = bufHdr;

        if(pcm_feedback)
        {
            audio_aio_buf.buf_addr = (OMX_U8*)pmeta_in;
            audio_aio_buf.data_len = new_length + sizeof(META_IN);
            audio_aio_buf.mfield_sz = sizeof(META_IN);
        }
        else
        {
            audio_aio_buf.data_len = new_length;
            audio_aio_buf.buf_addr = bufHdr->pBuffer;
        }

        DEBUG_PRINT ("\nETBP: Sending Data buffer[%x], Filled length = %d\n",
           (unsigned)( audio_aio_buf.buf_addr), audio_aio_buf.data_len);

        if(m_to_idle || bFlushinprogress)
        {
            pthread_mutex_lock(&in_buf_count_lock);
            DEBUG_PRINT("ETBP: nNumInputBuf %d Return Bufs\n",nNumInputBuf);
            pthread_mutex_unlock(&in_buf_count_lock);
            buffer_done_cb((OMX_BUFFERHEADERTYPE *)bufHdr);
            return OMX_ErrorNone;
        }
        pthread_mutex_lock(&in_buf_count_lock);
        drv_inp_buf_cnt++;
        pthread_mutex_unlock(&in_buf_count_lock);
        DEBUG_PRINT("ASYNC WRITE = %d flush=%d\n", drv_inp_buf_cnt, bFlushinprogress);

        if(0 > (rc = ioctl(m_drv_fd, AUDIO_ASYNC_WRITE, &audio_aio_buf)))
        {
            DEBUG_PRINT("\n Error in ASYNC WRITE call, rc = %d\n", rc);

            pthread_mutex_lock(&in_buf_count_lock);
            nNumInputBuf--;
            drv_inp_buf_cnt--;
            DEBUG_PRINT("ETBP: nNumInputBuf %d, drv_inp_buf_cnt %d\n", nNumInputBuf, drv_inp_buf_cnt);
            pthread_mutex_unlock(&in_buf_count_lock);

            return OMX_ErrorUndefined;
        }

        DEBUG_PRINT("\n AUDIO_ASYNC_WRITE issued for %x\n",(unsigned) bufHdr->pBuffer);
    }

    if(bFlushcompleted)
    {
        nTimestamp = buffer->nTimeStamp;
        bFlushcompleted = 0;
        DEBUG_PRINT("ETBP:bFlushcompleted\n");
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE  omx_mp3_adec::mp3_frameheader_parser(OMX_BUFFERHEADERTYPE* buffer,struct mp3_header *header)
{

    OMX_U8* temp_pBuf1 = NULL;
    unsigned int i = 0;
    OMX_U8 temp;

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);

    if (buffer->nFilledLen == 0)
    {
        DEBUG_PRINT ("\n Length is zero hence no point in processing \n");
        return OMX_ErrorMax;
    }

    temp_pBuf1 = buffer->pBuffer;
    i = 0;
    while(*temp_pBuf1 != 0xFF)
    {
        i++;
        temp_pBuf1++;
        if(i==buffer->nFilledLen)
            return OMX_ErrorMax;
    }
    temp = temp_pBuf1[0];
    header->sync = temp & 0xFF;
    if(header->sync == 0xFF)
    {
        temp = temp_pBuf1[1];
        header->sync = temp & 0xC0;
        if (header->sync != 0xC0)
        {
            DEBUG_PRINT("failure");
            return OMX_ErrorMax;
        }
    }
    else
    {
        DEBUG_PRINT("failure");
        return OMX_ErrorMax;
    }
    temp = temp_pBuf1[1];
    header->version = (temp & 0x18)>>3;
    header->Layer = (temp & 0x06)>>1;
    temp = temp_pBuf1[2];
    header->sampling_rate = (temp & 0x0C)>>2;
    temp = temp_pBuf1[3];
    header->channel_mode = (temp & 0xC0)>>6;

    DEBUG_PRINT("Channel Mode: %u, Sampling rate: %u and header version: %d from the header\n",(unsigned) (header->channel_mode),(unsigned) (header->sampling_rate), header->version);
    if((header->channel_mode == 0)||(header->channel_mode == 1)||(header->channel_mode == 2)) // Stereo, Joint Stereo,Dual Mono)
    {
        header->channel_mode = 2;  // stereo
    }
    else if (header->channel_mode == 3)
    {
        header->channel_mode = 1; // for all other cases configuring as mono TBD
    }
    else
    {
        header->channel_mode = 2; // if the channel is not recog. making the channel by default to Stereo.
        DEBUG_PRINT("Defauting the channel mode to Stereo");
    }
    header->sampling_rate = mp3_frequency_index[header->sampling_rate][header->version];
    DEBUG_PRINT(" frequency = %u, channels = %u\n",(unsigned)(header->sampling_rate), (unsigned)(header->channel_mode));
    return OMX_ErrorNone;

}

OMX_ERRORTYPE  omx_mp3_adec::fill_this_buffer_proxy (OMX_IN OMX_HANDLETYPE         hComp,
                                                     OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_BUFFERHEADERTYPE *bufHdr = buffer;
    struct mmap_info *ion_data = NULL;
    META_OUT *pmeta_out = NULL;
    msm_audio_aio_buf audio_aio_buf;
    msm_audio_ion_info audio_ion_buf;
    OMX_STATETYPE state;
    int ret = 0;
   (void)hComp;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(m_out_use_buf_case)
    {
        bufHdr = m_loc_out_use_buf_hdrs.find(buffer);

        if(!bufHdr)
        {
            DEBUG_PRINT("UseBufhdr for buffer[%p] is NULL", buffer);
            return OMX_ErrorBadParameter;
        }
    }

    if(m_output_eos_rxd == true)
    {
        DEBUG_PRINT("\n FTBP: Output EOS!! Invalid FTB call\n");
        bufHdr->nFilledLen = 0;
        bufHdr->nFlags &= ~OMX_BUFFERFLAG_EOS;
        post_output((unsigned) &m_cmp, (unsigned int)bufHdr,
            OMX_COMPONENT_GENERATE_FRAME_DONE);
        return OMX_ErrorNone;
    }
    if(bFlushinprogress)
    {
        DEBUG_PRINT("FTBP: flush in progress, return buf\n");
        bufHdr->nFilledLen = 0;
        bufHdr->nFlags = 0;
        bufHdr->nTimeStamp= nTimestamp;
        post_output((unsigned) &m_cmp, (unsigned int)bufHdr,
            OMX_COMPONENT_GENERATE_FRAME_DONE);
        return OMX_ErrorNone;
    }


    get_state(&m_cmp, &state);
    if(fake_eos_recieved)
    {
        if((state == OMX_StateExecuting) && m_suspend_out_buf_cnt)
        {
            DEBUG_PRINT("FTBP:Send frames stored in Suspend List\n");
            if(m_resume_out_buf_cnt < m_suspend_out_buf_cnt)
            {
                if(!m_suspend_out_buf_list || !m_suspend_out_buf_list
                    [m_resume_out_buf_cnt])
                {
                    DEBUG_PRINT("\nInvalid m_suspend_out_buf_list & contents\n");
                    return OMX_ErrorUndefined;
                }

                ion_data = m_suspend_out_buf_list[m_resume_out_buf_cnt];

                if(!ion_data->pBuffer)
                {
                    DEBUG_PRINT("\nInvalid buffer extracted in suspend list\n");
                    return OMX_ErrorUndefined;
                }

                bufHdr->nFilledLen = ion_data->filled_len;
                if(bufHdr->nFilledLen)
                {
                    memcpy(bufHdr->pBuffer, (void*)((char*)(ion_data->pBuffer)+sizeof(META_OUT)), bufHdr->nFilledLen);
                }
                else
                {
                    bufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
                }
                bufHdr->nTimeStamp = nTimestamp;
                post_output(0, (unsigned int)bufHdr,
                    OMX_COMPONENT_GENERATE_FRAME_DONE);

                audio_ion_buf.fd = ion_data->ion_fd;
                audio_ion_buf.vaddr = ion_data->pBuffer;
                if(0 > ioctl(m_drv_fd, AUDIO_DEREGISTER_ION,
                    &audio_ion_buf))
                {
                    DEBUG_PRINT("Error in ioctl AUDIO_DEREGISTER_ION\n");
                }

                free_ion_buffer((void**)&ion_data);

                pthread_mutex_lock(&m_seq_lock);
                m_resume_out_buf_cnt++;

                if(m_resume_out_buf_cnt == m_suspend_out_buf_cnt)
                {
                    m_resume_out_buf_cnt = m_suspend_out_buf_cnt = 0;
                    fake_eos_recieved = false;
                    fake_in_eos_sent = false;
                    pthread_mutex_lock(&m_in_th_lock_1);
                    if(is_in_th_sleep)
                    {
                        DEBUG_DETAIL("ftbp:WAKING UP IN THREADS\n");
                        in_th_wakeup();
                        is_in_th_sleep = false;
                    }
                    pthread_mutex_unlock(&m_in_th_lock_1);
                }
                pthread_mutex_unlock(&m_seq_lock);

                return OMX_ErrorNone;
            }
        }
    }


    if((search_output_bufhdr(buffer) == true))
    {
        pmeta_out = (META_OUT*) (bufHdr->pBuffer - sizeof(META_OUT));
        if(!pmeta_out)
        {
            DEBUG_PRINT_ERROR("\n Invalid pmeta_out(NULL)\n");
            return OMX_ErrorUndefined;
        }

        DEBUG_PRINT("\n Calling ASYNC_READ for %u bytes \n",(unsigned)(bufHdr->nAllocLen));
        audio_aio_buf.buf_len = bufHdr->nAllocLen;
        audio_aio_buf.private_data = bufHdr;
        audio_aio_buf.buf_addr = (OMX_U8*)pmeta_out;
        audio_aio_buf.data_len = 0;
        audio_aio_buf.mfield_sz = sizeof(META_OUT);

        pthread_mutex_lock(&out_buf_count_lock);
        drv_out_buf_cnt++;
        DEBUG_PRINT("this=%p ASYNC_READ = %d flush=%d\n", this, drv_out_buf_cnt,bFlushinprogress);
        pthread_mutex_unlock(&out_buf_count_lock);

        ret = ioctl(m_drv_fd, AUDIO_ASYNC_READ, &audio_aio_buf);
        if(ret < 0)
        {
            DEBUG_PRINT_ERROR("Error in ASYNC READ call, ret = %d\n", ret);
            pthread_mutex_lock(&out_buf_count_lock);
            drv_out_buf_cnt--;
            pthread_mutex_unlock(&out_buf_count_lock);
	    buffer->nTimeStamp = nTimestamp;
            buffer->nFilledLen = 0;
	    frame_done_cb((OMX_BUFFERHEADERTYPE *)bufHdr);
            DEBUG_PRINT_ERROR("FTBP: nNumOutputBuf %d, drv_out_buf_cnt %d\n", nNumOutputBuf, drv_out_buf_cnt);
            return OMX_ErrorUndefined;
        }
        DEBUG_PRINT("\n AUDIO_ASYNC_READ issued for %x\n", (unsigned)(audio_aio_buf.buf_addr));
    }
    else
    {
        DEBUG_PRINT_ERROR("FTBP:Invalid buffer in FTB \n");
	buffer->nTimeStamp = nTimestamp;
        buffer->nFilledLen = 0;
	frame_done_cb((OMX_BUFFERHEADERTYPE *)bufHdr);
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;

}


/* ======================================================================
FUNCTION
  omx_mp3_adec::FillThisBuffer

DESCRIPTION
  IL client uses this method to release the frame buffer
  after displaying them.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::fill_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                                  OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if( ( buffer != NULL )&&
        (buffer->nOutputPortIndex == 1) &&
        (buffer->nSize == sizeof (OMX_BUFFERHEADERTYPE)) &&
        (buffer->nVersion.nVersion == OMX_SPEC_VERSION) &&
        (search_output_bufhdr(buffer) == true)&&(m_out_bEnabled==OMX_TRUE)
        )
    {

        pthread_mutex_lock(&out_buf_count_lock);
        Omx_fillbufcnt++;
        nNumOutputBuf++;
        DEBUG_PRINT("FTB: Ouput Buffer Number nNumOutputBuf %d Omx_fillbufcnt %d bufferhdr %u", nNumOutputBuf,Omx_fillbufcnt,(unsigned)buffer);
        pthread_mutex_unlock(&out_buf_count_lock);
    PrintFrameHdr(OMX_COMPONENT_GENERATE_FTB,buffer);
        post_output((unsigned)hComp,
            (unsigned) buffer,OMX_COMPONENT_GENERATE_FTB);
    }
    else
    {

        eRet = OMX_ErrorBadParameter;

        if (m_out_bEnabled == OMX_FALSE)
        {
            eRet=OMX_ErrorIncorrectStateOperation;
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

/* ======================================================================
FUNCTION
  omx_mp3_adec::SetCallbacks

DESCRIPTION
  Set the callbacks.

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                                           OMX_IN OMX_CALLBACKTYPE* callbacks,
                                           OMX_IN OMX_PTR             appData)
{
    m_cb       = *callbacks;
    m_app_data =    appData;
   (void)hComp;
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_mp3_adec::ComponentDeInit

DESCRIPTION
  Destroys the component and release memory allocated to the heap.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
//    m_is_in_th_sleep=0;
//    m_is_out_th_sleep=0;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    (void)hComp;
    if (OMX_StateLoaded != m_state)
    {
        DEBUG_PRINT_ERROR("Warning: Received DeInit when not in LOADED state, cur_state %d\n",
            m_state);
	DEBUG_PRINT("Wait for flush to complete if flush is in progress\n");
	if (bFlushinprogress)
		wait_for_flush_event();
	DEBUG_PRINT("Flush completed\n");
    }
    if (mime_type)
    {
         free(mime_type);
         mime_type = NULL;
    }
    deinit_decoder();
    DEBUG_PRINT_ERROR("COMPONENT DEINIT...\n");
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_mp3_adec::deinit_decoder

DESCRIPTION
  Closes all the threads and release memory allocated to the heap.

PARAMETERS
  None.

RETURN VALUE
  None.

========================================================================== */
void  omx_mp3_adec::deinit_decoder()
{
    DEBUG_PRINT("Component-deinit STARTED\n");

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    pthread_mutex_lock(&m_in_th_lock_1);
    if(is_in_th_sleep)
    {
        DEBUG_DETAIL("Deinit:WAKING UP IN THREADS\n");
        in_th_wakeup();
        is_in_th_sleep = false;
    }
    pthread_mutex_unlock(&m_in_th_lock_1);

    pthread_mutex_lock(&m_out_th_lock_1);
    if(is_out_th_sleep)
    {
        DEBUG_DETAIL("Deinit:WAKING UP OUT THREADS\n");
        out_th_wakeup();
        is_out_th_sleep = false;
    }
    pthread_mutex_unlock(&m_out_th_lock_1);

    DEBUG_PRINT("Component-deinit calling omx_mp3_thread_stop(m_ipc_to_in_th)\n");
    if (m_ipc_to_in_th != NULL) {
        omx_mp3_thread_stop(m_ipc_to_in_th);
        m_ipc_to_in_th = NULL;
    }

    DEBUG_PRINT("Component-deinit calling omx_mp3_thread_stop(m_ipc_to_cmd_th)\n");

    if (m_ipc_to_cmd_th != NULL) {
        omx_mp3_thread_stop(m_ipc_to_cmd_th);
        m_ipc_to_cmd_th = NULL;
    }

    DEBUG_PRINT("Component-deinit being processed m_drv_fd %d %d %d\n",m_drv_fd,m_suspend_out_buf_cnt,m_resume_out_buf_cnt);

    while(m_suspend_out_buf_cnt > m_resume_out_buf_cnt)
    {
        struct mmap_info *ion_data = NULL;
        msm_audio_ion_info audio_ion_buf;

        ion_data = m_suspend_out_buf_list[m_resume_out_buf_cnt];

        DEBUG_PRINT("Deinit:Freeing Buf fd=%d\n",ion_data->ion_fd);
        audio_ion_buf.fd = ion_data->ion_fd;
        audio_ion_buf.vaddr = ion_data->pBuffer;
        if(0 > ioctl(m_drv_fd, AUDIO_DEREGISTER_ION,
                    &audio_ion_buf))
        {
            DEBUG_PRINT("Error in ioctl AUDIO_DEREGISTER_ION\n");
        }
        free_ion_buffer((void**)&ion_data);
        m_resume_out_buf_cnt++;
    }
    DEBUG_PRINT("Component-deinit m_suspend_out_buf_list\n");
    if(m_suspend_out_buf_list)
    {
        free(m_suspend_out_buf_list);
        m_suspend_out_buf_list = NULL;
    }
    if(m_suspend_out_drv_buf)
    {
        free(m_suspend_out_drv_buf);
        m_suspend_out_drv_buf = NULL;
    }

    drv_inp_buf_cnt = 0;
    drv_out_buf_cnt = 0;
    m_resume_out_buf_cnt = 0;
    m_suspend_out_buf_cnt = 0;
    m_suspend_drv_buf_cnt = 0;
    m_flush_inbuf = false;
    m_flush_outbuf = false;
    m_to_idle = false;
    DEBUG_PRINT("Component-deinit success ..\n");
    m_is_alloc_buf = 0;
    m_pause_to_exe = false;

    if (m_drv_fd >= 0)
    {
        if(ioctl(m_drv_fd, AUDIO_STOP, 0) <0)
            DEBUG_PRINT("De-init: AUDIO_STOP FAILED\n");
        if(ioctl(m_drv_fd,AUDIO_ABORT_GET_EVENT,NULL) < 0)
        {
            DEBUG_PRINT("De-init: AUDIO_ABORT_GET_EVENT FAILED\n");
        }
        DEBUG_PRINT("De-init: Free buffer\n");
        if(close(m_drv_fd) < 0)
            DEBUG_PRINT("De-init: Driver Close Failed \n");
        m_drv_fd = -1;
    }
    else
    {
        DEBUG_PRINT_ERROR(" mp3 device already closed\n");
    }


    DEBUG_PRINT("Component-deinit calling omx_mp3_thread_stop(m_ipc_to_event_th)\n");
    if (m_ipc_to_event_th != NULL) {
        omx_mp3_thread_stop(m_ipc_to_event_th);
        m_ipc_to_event_th = NULL;
    }

    DEBUG_PRINT("Component-deinit pcm_feedback = %d\n",pcm_feedback);

    if(pcm_feedback ==1)
    {
        DEBUG_PRINT("Component-deinit calling omx_mp3_thread_stop(m_ipc_to_out_th)\n");
        if (m_ipc_to_out_th != NULL) {
            omx_mp3_thread_stop(m_ipc_to_out_th);
            m_ipc_to_out_th = NULL;
        }
    }

    m_to_idle =false;
    bOutputPortReEnabled = 0;
    nState = OMX_StateLoaded;
    bFlushinprogress = 0;
    nNumInputBuf = 0;
    nNumOutputBuf = 0;
    suspensionPolicy= OMX_SuspensionDisabled;
    resetSuspendFlg();
    resetResumeFlg();
    fake_eos_recieved = false;
    fake_in_eos_sent = false;
    is_in_th_sleep = false;
    is_out_th_sleep = false;
    m_first_mp3_header = 0;
    m_inp_current_buf_count=0;
    m_out_current_buf_count=0;
    m_inp_bEnabled = OMX_FALSE;
    m_out_bEnabled = OMX_FALSE;
    m_inp_bPopulated = OMX_FALSE;
    m_out_bPopulated = OMX_FALSE;
    input_buffer_size = 0;
    output_buffer_size = 0;
    m_odd_byte = 0;
    m_odd_byte_set = false;
    m_input_eos_rxd = false;
    m_output_eos_rxd = false;
    bGenerateEOSPending = false;
    m_flush_cmpl_event = 0;
    delete m_timer;
    mp3Inst = NULL;
    m_comp_deinit = 1;
    DEBUG_PRINT_ERROR("Component-deinit m_drv_fd = %d\n",m_drv_fd);
    DEBUG_PRINT("Component-deinit end \n");
}

/* ======================================================================
FUNCTION
  omx_mp3_adec::UseEGLImage

DESCRIPTION
  OMX Use EGL Image method implementation <TBD>.

PARAMETERS
  <TBD>.

RETURN VALUE
  Not Implemented error.

========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::use_EGL_image(OMX_IN OMX_HANDLETYPE                hComp,
                                          OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                          OMX_IN OMX_U32                        port,
                                          OMX_IN OMX_PTR                     appData,
                                          OMX_IN void*                      eglImage)
{
    (void)hComp;
    (void)bufferHdr;
    (void)port;
    (void)appData;
    (void)eglImage;
    DEBUG_PRINT_ERROR("Error : use_EGL_image:  Not Implemented \n");
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_mp3_adec::ComponentRoleEnum

DESCRIPTION
  OMX Component Role Enum method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_mp3_adec::component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
                                                OMX_OUT OMX_U8*        role,
                                                OMX_IN OMX_U32        index)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    const char *cmp_role = "audio_decoder.mp3";
   (void)hComp;

    if(index == 0 && role)
    {
        memcpy(role, cmp_role, sizeof(cmp_role));
        *(((char *) role) + sizeof(cmp_role)) = '\0';
    }
    else
    {
        eRet = OMX_ErrorNoMore;
    }
    return eRet;
}


/* ======================================================================
FUNCTION
  omx_mp3_adec::AllocateDone

DESCRIPTION
  Checks if entire buffer pool is allocated by IL Client or not.
  Need this to move to IDLE state.

PARAMETERS
  None.

RETURN VALUE
  true/false.

========================================================================== */
bool omx_mp3_adec::allocate_done(void)
{
    OMX_BOOL bRet = OMX_FALSE;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(pcm_feedback==1)
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
    else if(pcm_feedback==0)
    {
        if(m_inp_act_buf_count == m_inp_current_buf_count)
        {
            bRet=OMX_TRUE;
        }
        if((m_inp_act_buf_count == m_inp_current_buf_count) && m_inp_bEnabled )
            m_inp_bPopulated = OMX_TRUE;
    }
    return bRet;
}


/* ======================================================================
FUNCTION
  omx_mp3_adec::ReleaseDone

DESCRIPTION
  Checks if IL client has released all the buffers.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_mp3_adec::release_done(OMX_U32 param1)
{
    OMX_BOOL bRet = OMX_FALSE;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(param1 == OMX_ALL)
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

bool omx_mp3_adec::omx_mp3_fake_eos()
{
    msm_audio_aio_buf audio_aio_buf ;
    struct msm_audio_ion_info audio_ion_buf;
    struct mmap_info *ion_buf = NULL;
    META_IN *pmeta_in = NULL;

    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    ion_buf = (struct mmap_info *)alloc_ion_buffer(sizeof(META_IN));

    if(ion_buf == NULL)
    {
        DEBUG_PRINT("\nError in allocating ION buffer\n");
        return false;
    }

    DEBUG_PRINT("\nFake i/p EOS buffer %u, ion_fd = %d \n",
      (unsigned)  (ion_buf->pBuffer), ion_buf->ion_fd);

    /* Register the mapped ION buffer with the MP3 driver */
    audio_ion_buf.fd = ion_buf->ion_fd;
    audio_ion_buf.vaddr = ion_buf->pBuffer;
    if(0 > ioctl(m_drv_fd, AUDIO_REGISTER_ION, &audio_ion_buf))
    {
        DEBUG_PRINT("\n Error in ioctl AUDIO_REGISTER_ION\n");
        free_ion_buffer((void**)&ion_buf);
        return false;
    }

    pmeta_in = (META_IN*)ion_buf->pBuffer;
    // copy the metadata info from the BufHdr and insert to payload
    pmeta_in->offsetVal  = sizeof(META_IN);
    pmeta_in->nTimeStamp = nTimestamp;
    pmeta_in->nFlags     = OMX_BUFFERFLAG_EOS;

    audio_aio_buf.buf_addr = ion_buf->pBuffer;
    audio_aio_buf.buf_len = ion_buf->map_buf_size;
    audio_aio_buf.data_len = sizeof(META_IN);
    audio_aio_buf.private_data = (void*)ion_buf;
    audio_aio_buf.mfield_sz = sizeof(META_IN);

    DEBUG_PRINT ("\nSending FAKE EOS at i/p on Suspend state\n");
    fake_in_eos_ack_received = false;
    fake_in_eos_sent = true;
    if(0 > ioctl(m_drv_fd, AUDIO_ASYNC_WRITE, &audio_aio_buf))
    {
        DEBUG_PRINT("\nError in ASYNC WRITE for Fake EOS\n");
        if(0 > ioctl(m_drv_fd, AUDIO_DEREGISTER_ION, &audio_ion_buf))
        {
            DEBUG_PRINT("\n Error in ioctl AUDIO_REGISTER_ION\n");
        }

        free_ion_buffer((void**)&ion_buf);
        fake_in_eos_ack_received = false;
        fake_in_eos_sent = false;
        return false;
    }
    DEBUG_PRINT("\n AUDIO_ASYNC_WRITE issued for Fake EOS buf len = 0\n");
    return true;
}

bool omx_mp3_adec::alloc_fill_suspend_out_buf()
{
    msm_audio_aio_buf audio_aio_buf;
    struct msm_audio_ion_info audio_ion_buf;
    struct mmap_info *ion_buf = NULL;
    DEBUG_PRINT("%s %p\n",__FUNCTION__,this);
    if(!m_suspend_out_buf_list)
    {
        DEBUG_PRINT_ERROR("\n Error: m_suspend_out_buf_list is NULL");
        return false;
    }

    ion_buf = (struct mmap_info*)alloc_ion_buffer(output_buffer_size
        + sizeof(META_OUT));

    if(ion_buf == NULL)
    {
        DEBUG_PRINT("\nError in allocating ION buffer\n");
        return false;
    }
    DEBUG_PRINT("\nSuspend o/p ION buffer %u, ion_fd = %d \n",
       (unsigned) ion_buf->pBuffer, ion_buf->ion_fd);

    /* Register the mapped ION buffer with the MP3 driver */
    audio_ion_buf.fd = ion_buf->ion_fd;
    audio_ion_buf.vaddr = ion_buf->pBuffer;
    if(0 > ioctl(m_drv_fd, AUDIO_REGISTER_ION, &audio_ion_buf))
    {
        DEBUG_PRINT("\n Error in ioctl AUDIO_REGISTER_ION\n");
        free_ion_buffer((void**)&ion_buf);
        return false;
    }

    audio_aio_buf.buf_addr = ion_buf->pBuffer;
    audio_aio_buf.buf_len = output_buffer_size;
    audio_aio_buf.data_len = 0;
    audio_aio_buf.private_data = (void*)ion_buf;
    audio_aio_buf.mfield_sz = sizeof(META_OUT);

    pthread_mutex_lock(&out_buf_count_lock);
    DEBUG_PRINT("\n ASYNC_READ issued for Suspend o/p ION buf[%d]\n",
        m_suspend_out_buf_cnt);
    m_suspend_out_buf_list[m_suspend_out_buf_cnt] = ion_buf;
    m_suspend_out_buf_cnt++;
    pthread_mutex_unlock(&out_buf_count_lock);

    DEBUG_PRINT ("\nSending ION buffer at o/p on Suspend state\n");
    if(0 > ioctl(m_drv_fd, AUDIO_ASYNC_READ, &audio_aio_buf))
    {
        DEBUG_PRINT("\nError in ASYNC READ for SuspendBuf\n");
        if(0 > ioctl(m_drv_fd, AUDIO_DEREGISTER_ION, &audio_ion_buf))
        {
            DEBUG_PRINT("\n Error in ioctl AUDIO_REGISTER_ION\n");
        }

        pthread_mutex_lock(&out_buf_count_lock);
        m_suspend_out_buf_cnt--;
        m_suspend_out_buf_list[m_suspend_out_buf_cnt] = NULL;
        pthread_mutex_unlock(&out_buf_count_lock);
        free_ion_buffer((void**)&ion_buf);
        return false;
    }

   return true;
}


timer::timer(omx_mp3_adec* base):
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
    m_timerinfo->pTimer = this;
    m_timerinfo->base  = m_base;
    int rc = pthread_create(&m_timerinfo->thr ,0, omx_mp3_comp_timer_handler,
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

timer::~timer()
        {

    releaseTimer();
    stopTimer();
    int rc = pthread_join(m_timerinfo->thr,NULL);
    DEBUG_PRINT("******************************\n");
    DEBUG_PRINT("CLOSING TIMER THREAD...%d%lu\n",rc,m_timerinfo->thr);
    DEBUG_PRINT("******************************\n");
    if(m_timerinfo)
        {
        m_timerinfo->pTimer = NULL;
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

void timer::wait_for_timer_event()
{
    sem_wait(&m_sem_state);
}

void timer::startTimer()
{
    sem_post(&m_sem_state);
}

int timer::timer_run()
{
    int rc =0;
    struct timespec   ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    DEBUG_PRINT("%s: Starting timer at %lu %ld %d\n",
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
    DEBUG_PRINT("%s: Elapsed Timer: %lu %ld\n",
                            __FUNCTION__,
                            ts.tv_sec,ts.tv_nsec);
    return rc;
    }

void timer::stopTimer()
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

void* omx_mp3_comp_timer_handler(void *pT)
{
    TIMERINFO *pTime = (TIMERINFO*)pT;
    timer *pt = pTime->pTimer;
    omx_mp3_adec *pb = pTime->base;
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
        OMX_STATETYPE state;
        DEBUG_DETAIL("SH:state=%d suspendstat=%d\n",pb->get_state((OMX_HANDLETYPE)pb,&state),\
                                                    pb->getSuspendFlg());
        pb->get_state((OMX_HANDLETYPE)pb,&state);
        if( ((state== OMX_StatePause)) && !(pb->getSuspendFlg()))
{
            // post suspend message to command thread;
            pb->post_command(0,0,omx_mp3_adec::OMX_COMPONENT_SUSPEND);
            pt->setTimerExpiry();
}
        else
{
            DEBUG_PRINT("SH: Ignore Timer expiry state=%d",state);
        }
}

exit_th:
    DEBUG_PRINT_ERROR("Timer thread exited\n");
    return NULL;
}
