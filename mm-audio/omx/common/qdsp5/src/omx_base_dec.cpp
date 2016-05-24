/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_base_dec.cpp
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
#include "omx_base_dec.h"

using namespace std;

//////////////////////////////////////////////////////////////
// COMXBaseDecIn
//////////////////////////////////////////////////////////////
COmxBaseDecIn::COmxBaseDecIn(
COmxBase* base, int fd,
OMX_CALLBACKTYPE cb,
OMX_U32 buf_size,
OMX_U32 sf,
OMX_U8 ch,
OMX_BOOL pcm_feedback,OMX_PTR appdata):
COmxBaseIn(base,fd,cb,appdata),
m_sample_rate(sf),
m_ch_cfg(ch),
m_tmp_meta_buf(NULL),
m_app_data(appdata),
m_default_bufsize(buf_size),
m_mode(pcm_feedback)
{
    DEBUG_PRINT("COmxBaseDecIn::%s\n",__FUNCTION__);
    DEBUG_DETAIL("fd[%d] buf_size[%u]sf[%u]ch[%u]pcm[%u]appdata[%p]\n",\
    fd,(unsigned int)buf_size,(unsigned int)sf,(unsigned int)ch,
    (unsigned int)pcm_feedback,appdata);
    if ( m_tmp_meta_buf == NULL )
    {   // size of input buffer+meta in+ pseudo raw header
        m_tmp_meta_buf = (OMX_U8*)malloc(sizeof(OMX_U8)*
        (buf_size+ sizeof(META_IN)+4));
        if ( m_tmp_meta_buf == NULL )
        {
            DEBUG_PRINT_ERROR("UseBuf: Mem alloc failed for meta buf\n");
        }
        else
        DEBUG_PRINT("UseBuf: Mem alloc success for meta buf %p\n", m_tmp_meta_buf);
    }
}

COmxBaseDecIn::~COmxBaseDecIn()
{
    DEBUG_PRINT("COmxBaseDecIn::%s\n",__FUNCTION__);
    if(m_tmp_meta_buf)
    {
        free(m_tmp_meta_buf);
        m_tmp_meta_buf = NULL;
    }
    m_app_data = NULL;
    m_sample_rate = 0;
    m_ch_cfg = 0;
    m_default_bufsize = 0;
}


void COmxBaseDecIn::process_in_port_msg(unsigned char id)
{
    DEBUG_PRINT("COmxBaseDecIn::%s id=%d, No Definition\n",__FUNCTION__,id);
    return;
}

bool COmxBaseDecIn::omx_fake_eos()
{
    DEBUG_PRINT("COmxBaseDecIn::%s\n",__FUNCTION__);
    int ret;
    META_IN meta_in;

    // FAKE an EOS
    // copy the metadata info from the BufHdr and insert to payload
    meta_in.offsetVal  = sizeof(META_IN);
    meta_in.nTimeStamp = get_comp()->getTS();
    meta_in.nFlags     = OMX_BUFFERFLAG_EOS;
    ret = write(get_drv_fd(), &meta_in, sizeof(META_IN));
    DEBUG_PRINT("omx_aac_fake_eos :ret=%d", ret);
    return true;
}

//////////////////////////////////////////////////////////////
// COMXBaseDecOut
//////////////////////////////////////////////////////////////
COmxBaseDecOut::COmxBaseDecOut(COmxBase * base, int fd, OMX_CALLBACKTYPE cb,
OMX_BOOL pcm_feedback, OMX_PTR appdata, int buf_size):
COmxBaseOut(base,fd,cb,appdata),
m_drv_fd(fd),
m_appdata(appdata),
m_mode(pcm_feedback),
fake_eos_recieved(OMX_FALSE),
m_tmp_out_meta_buf(NULL),
m_bufsize(buf_size),
m_bufMgr(NULL)

{
    DEBUG_PRINT("COmxBaseDecOut::%s\n",__FUNCTION__);
    if(!m_tmp_out_meta_buf)
    {
        m_tmp_out_meta_buf = (OMX_U8*)malloc(sizeof(OMX_U8)*
        (m_bufsize+ sizeof(META_OUT)));
        if ( m_tmp_out_meta_buf == NULL )
        {
            DEBUG_PRINT_ERROR("Mem alloc failed for out meta buf\n");
        }
        else
        {
            DEBUG_DETAIL(" OUT TMP BUF SUCCESS %p\n",m_tmp_out_meta_buf);
        }
    }
    m_bufMgr = new omxBufMgr;
    if(m_bufMgr == NULL)
    {
        DEBUG_PRINT_ERROR("Not able to allocate memory for Buffer Manager\n");
    }
}

COmxBaseDecOut::~COmxBaseDecOut()
{
    DEBUG_PRINT("COmxBaseDecOut::%s\n",__FUNCTION__);
    if(m_tmp_out_meta_buf)
    {
        free(m_tmp_out_meta_buf);
    }
    if(m_bufMgr)
    {
        delete m_bufMgr;
    }
    fake_eos_recieved = OMX_FALSE;

    m_drv_fd = -1;
    m_bufsize = 0;
    m_appdata = NULL;
    m_bufsize = 0;
}

bool COmxBaseDecOut::process_ftb(
OMX_IN OMX_HANDLETYPE hComp,
OMX_BUFFERHEADERTYPE  *buffer)
{
    DEBUG_PRINT("COmxBaseDecOut::%s buffer[%p]\n",__FUNCTION__,buffer);
    int nDatalen = 0;
    OMX_STATETYPE state;
    META_OUT_SUS      meta_out_sus;
    int byte_count = 0;
    int meta_count = 0;

    DEBUG_DETAIL("FTB-->bufHdr=%p buffer=%p alloclen=%u",\
    buffer, buffer->pBuffer,
    (unsigned int)buffer->nAllocLen);
    state = get_state();

    if(m_bufMgr)
    {
        get_comp()->setFilledSpaceInTcxoBuf(m_bufMgr->getBufFilledSpace());
    }
    if(fake_eos_recieved)
    {
        DEBUG_PRINT("Residual still left[%d]state[%d]\n",
                                               m_bufMgr->getBufFilledSpace(),state);
        if(state == OMX_StateExecuting)
        {
            meta_count = m_bufMgr->emptyToBuf((OMX_U8 *)&meta_out_sus,sizeof(META_OUT_SUS));
            DEBUG_PRINT("Num of bytes meta copied[%d]Residual[%lu]\n",meta_out_sus.offsetVal,
                                                         m_bufMgr->getBufFilledSpace());
            if(meta_count > 0)
            {
                byte_count = m_bufMgr->emptyToBuf(buffer->pBuffer,meta_out_sus.offsetVal);
                DEBUG_PRINT("Num of bytes copied[%d]Residual[%lu]\n",byte_count,
                                                          m_bufMgr->getBufFilledSpace());
            }

            if(byte_count == 0){
                fake_eos_recieved = OMX_FALSE;
                byte_count = 0;

                // input EOS sent to DSP
                DEBUG_PRINT("FTB: EOSFLAG=%d\n",get_comp()->get_eos_bm());
                if((get_comp()->get_eos_bm()& IP_OP_PORT_BITMASK)
                                                    == IP_OP_PORT_BITMASK)
                {
                    DEBUG_PRINT("FTB:ENABLING SUSPEND FLAG\n");
                    buffer->nFlags= 0x01;
                    get_comp()->set_eos_bm(0);
                }
                DEBUG_PRINT("FTB: Wake up in thread\n");
                get_comp()->in_th_wakeup();
            }
            buffer->nTimeStamp = meta_out_sus.nTimeStamp;
            buffer->nFilledLen = byte_count;
            frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
            return OMX_ErrorNone;
        }
    }
    if((get_comp()->is_trigger_eos() == OMX_TRUE))
    {
        // HACK--> as we cant issue O/p flush now.
        buffer->nFlags= 0x01;
        buffer->nFilledLen = 0;
        buffer->nTimeStamp= get_comp()->getTS();
        get_comp()->trigger_eos(OMX_FALSE);
        post_output((unsigned) & hComp,(unsigned) buffer,OMX_COMPONENT_GENERATE_FRAME_DONE);

        return OMX_ErrorNone;
    }
    /* Assume fill this buffer function has already checked
    validity of buffer */
    {
        //int ncount = buffer->nAllocLen/OMX_AAC_OUTPUT_BUFFER_SIZE;
        int ncount = buffer->nAllocLen/m_bufsize;
        int nRead = 0;
        int nReadbytes = 0;
        OMX_IN OMX_U8  *pBuf = NULL;
        META_OUT       meta_out;
        pBuf = buffer->pBuffer;

        for (nRead = 0; nRead < 1; nRead++)
        {
            //if(m_output_ctrl_cmd_q.m_size)
            if(get_q_size(get_cmd_q()))
            {
                DEBUG_DETAIL("FTB-->: FLUSH CMD IN Q, Exit from Loop");
                //nDatalen += nReadbytes;
                break;
            }
            DEBUG_DETAIL("FTB:BEFORE READ fd=%d tmpBuf=%p\n",m_drv_fd,
                                                             m_tmp_out_meta_buf);
            nReadbytes = read(m_drv_fd, m_tmp_out_meta_buf,(m_bufsize+ sizeof(META_OUT)));
            if (( nReadbytes <= 0))
            {
                DEBUG_PRINT("FTB: nReadbytes is -1 break read() nRead=%d %d",\
                nRead,nReadbytes);
                buffer->nFilledLen = 0;
                buffer->nTimeStamp= get_comp()->getTS();
                post_output((unsigned) & hComp,(unsigned) buffer,OMX_COMPONENT_GENERATE_FRAME_DONE);

                return OMX_ErrorNone;
            }
            // extract the metadata contents
            memcpy(&meta_out, m_tmp_out_meta_buf,sizeof(META_OUT));
            DEBUG_PRINT("FTB->Meta:loop[%d]bytesread[%d]meta-len[0x%x]TS[0x%x]"\
            "nFlags[0x%x]\n",\
            nRead,nReadbytes, meta_out.offsetVal,
            (unsigned int)meta_out.nTimeStamp,
            (unsigned int)meta_out.nFlags);
            if(nRead == 0)
            {
                buffer->nTimeStamp = (meta_out.nTimeStamp)/1000;
                DEBUG_PRINT("FTB:Meta loop=%d TS[0x%x] TS[0x%x]\n",\
                nRead,
                (unsigned int)buffer->nTimeStamp,
                (unsigned int)meta_out.nTimeStamp);

            }
            // copy the pcm frame to the client buffer
            buffer->nFlags |= meta_out.nFlags;
            memcpy(pBuf, &m_tmp_out_meta_buf[sizeof(META_OUT)],
            (nReadbytes - sizeof(META_OUT)));

            if(((buffer->nFlags) & 0x0001) == OMX_BUFFERFLAG_EOS)
            {
                DEBUG_PRINT("FTB: EOS reached Output port");
                if((get_comp()->get_eos_bm())){
                    DEBUG_PRINT("FTB: END OF STREAM \n");
                    unsigned eos_bm=0 ; //output EOS recieved;
                    eos_bm = get_comp()->get_eos_bm();
                    get_comp()->set_eos_bm((eos_bm| OP_PORT_BITMASK));
                }
                DEBUG_PRINT("FTB: END OF STREAM m_eos_bm=%d\n",\
                get_comp()->get_eos_bm());
                break;
            }
            // just reading msize, should it be guarded by mutex ?
            //else if(m_output_ctrl_cmd_q.m_size)
            else if(get_q_size(get_cmd_q()))
            {
                DEBUG_DETAIL("FTB-->: Flush Cmd found in Q, STOP READING");
                nDatalen = nDatalen + (nReadbytes - sizeof(META_OUT));
                break;
            }
            pBuf += (nReadbytes - sizeof(META_OUT));
            nDatalen = nDatalen + (nReadbytes - sizeof(META_OUT));
        }

        buffer->nFilledLen = nDatalen;

        if((nDatalen <= 0)&& !(get_comp()->getSuspendFlg()) )
        {
            buffer->nFilledLen = 0;
            DEBUG_PRINT("FTB-->: Data length read is 0\n");
            frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
            if((buffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS )
            {
                DEBUG_PRINT("FTB: Now, Send EOS flag to Client \n");
                get_cb().EventHandler(&(get_comp()->m_cmp),
                get_app_data(),
                OMX_EventBufferFlag,
                1, 1, NULL );
                if(get_comp()->get_eos_bm()){
                    DEBUG_PRINT("FTB: END OF STREAM \n");
                    unsigned eos_bm=0;;
                    eos_bm = get_comp()->get_eos_bm();
                    get_comp()->set_eos_bm((eos_bm| OP_PORT_BITMASK));
                }
                DEBUG_PRINT("FTB: END OF STREAM m_eos_bm=%d\n",\
                get_comp()->get_eos_bm());
            }
        }
        else
        {

            if((buffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS )
            {
                if(get_comp()->getSuspendFlg())
                {
                    unsigned      p1; // Parameter - 1
                    unsigned      p2; // Parameter - 2
                    unsigned char id;
                    bool          ret = false;
                    fake_eos_recieved = OMX_TRUE;

                    // Faked EOS recieved, no processing of this mesg is reqd.
                    //ret = m_output_ctrl_cmd_q.get_msgq_id(&id);
                    ret = get_msg_qid(get_cmd_q(),&id);
                    if(ret && (id == OMX_COMPONENT_SUSPEND))
                    {
                        //m_output_ctrl_cmd_q.pop_entry(&p1,&p2,&id);
                        get_msg(get_cmd_q(),&p1,&p2,&id);
                    }
                    ioctl(m_drv_fd, AUDIO_STOP, 0);
                    if(!get_comp()->get_eos_bm())
                    {
                        // reset the flag as input eos was not set when this scenario happened
                        // o/p thread blocked on read() before faked_eos sent to dsp,
                        // now i/p thread fakes eos, o/p thread recieves the faked eos,
                        // since suspend flag is true, reset the nflags
                        buffer->nFlags = 0;
                    }
                    // now inform the client that the comp is SUSPENDED
                    DEBUG_PRINT_ERROR("FTB:EOS reached Send EH ctrlq=%d fake_eos=%d",\
                    get_q_size(get_cmd_q()),fake_eos_recieved);
                }
                else
                {
                    DEBUG_PRINT("FTBP: Now, Send EOS flag to Client \n");
                    frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
                    get_cb().EventHandler(&(get_comp()->m_cmp),
                    get_app_data(),
                    OMX_EventBufferFlag,
                    1, 1, NULL );
                    return OMX_ErrorNone;
                }
            }
            state = get_state();

            if (state == OMX_StatePause)
            {
                DEBUG_PRINT("FTBP:Post the FBD to event thread currstate=%d\n",\
                state);
                post_output((unsigned) & hComp,(unsigned) buffer,
                OMX_COMPONENT_GENERATE_FRAME_DONE);
            }
            else
            {
                frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
            }
        }
    }
    return OMX_ErrorNone;
}

void COmxBaseDecOut::process_out_port_msg(unsigned char id)
{
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned char ident;
    unsigned       qsize     = 0; // qsize
    unsigned      tot_qsize = 0;
    OMX_STATETYPE state;
    bool bOutputPortReEnabled = false;

    DEBUG_PRINT("COmxBaseDecOut::%s id[%d]\n",__FUNCTION__,id);

    loopback_out:
    bOutputPortReEnabled = get_comp()->get_port_recfg();

    state = get_state();
    if(state == OMX_StateLoaded)
    {
        DEBUG_PRINT(" OUT: IN LOADED STATE RETURN\n");
        return;
    }
    omx_cmd_queue *cmd_q  = get_cmd_q();
    omx_cmd_queue *bd_q  = get_bd_q();
    omx_cmd_queue *data_q = get_data_q();

    qsize = get_q_size(cmd_q);
    tot_qsize = qsize;
    tot_qsize += get_q_size(bd_q);
    tot_qsize += get_q_size(data_q);

    if ( 0 == tot_qsize )
    {
        DEBUG_DETAIL("OUT-->BREAK FROM LOOP...%d\n",tot_qsize);
        return;
    }
    if((state != OMX_StateExecuting) && !qsize)
    {
        state = get_state();
        if(state == OMX_StateLoaded)
        {    return;}
        DEBUG_DETAIL("OUT:1.SLEEPING OUT THREAD\n");
        get_comp()->out_th_sleep();
        goto loopback_out;
    }
    if((!bOutputPortReEnabled) && !(get_q_size(get_cmd_q())))
    {
        // case where no port reconfig and nothing in the flush q
        DEBUG_DETAIL("No flush/port reconfig qsize=%d tot_qsize=%d",\
        get_q_size(get_cmd_q()),tot_qsize);
        state = get_state();
        if(state == OMX_StateLoaded)
        {return;}

        if((get_q_size(get_cmd_q())) || !(get_comp()->get_flush_stat()))
        {
            DEBUG_PRINT("OUT THREAD SLEEPING...\n");
            get_comp()->out_th_sleep();
            goto loopback_out;
        }
    }
    else if(state == OMX_StatePause)
    {
        DEBUG_PRINT ("\n OUT Thread in the pause state");
        if(!(get_q_size(get_cmd_q())))
        {
            // to take care of some race condition, where the output
            // thread is working on the old state;
            state = get_state();
            DEBUG_PRINT("OUT: pause state =%d m_pause_to_exe=%d\n",\
            state,
            get_comp()->is_pause_to_exe());
            if ((state == OMX_StatePause) &&
                    !(get_comp()->is_pause_to_exe()) )
            {
                DEBUG_DETAIL("OUT: SLEEPING OUT THREAD\n");
                get_comp()->out_th_sleep();
                goto loopback_out;
            }
            else
            DEBUG_PRINT("OUT--> In pause if() check, but now state changed\n");
        }
    }

    qsize = get_q_size(cmd_q);
    tot_qsize = qsize;
    tot_qsize += get_q_size(bd_q);
    tot_qsize += get_q_size(data_q);

    state = get_state();

    DEBUG_DETAIL("OUT-->QSIZE-flush=%d,fbd=%d QSIZE=%d state=%d\n",\
    get_q_size(cmd_q),
    get_q_size(bd_q),
    get_q_size(data_q),state);
    if (qsize)
    {
        // process FLUSH message
        get_msg(cmd_q,&p1,&p2,&ident);
    }
    else if((qsize = get_q_size(bd_q)) &&
            (bOutputPortReEnabled) && (state == OMX_StateExecuting))
    {
        // then process EBD's
        get_msg(bd_q,&p1,&p2,&ident);
    }
    else if((qsize = get_q_size(data_q)) &&
            (bOutputPortReEnabled) && (state == OMX_StateExecuting))
    {
        // if no FLUSH and FBD's then process FTB's
        get_msg(data_q, &p1,&p2,&ident);
    }
    else if(state == OMX_StateLoaded)
    {
        DEBUG_PRINT("IN: ***in OMX_StateLoaded so exiting\n");
        return ;
    }
    else
    {
        qsize = 0;
        DEBUG_PRINT("OUT--> Empty Queue state=%d %d %d %d\n",state,\
        get_q_size(cmd_q),
        get_q_size(bd_q),
        get_q_size(data_q));


        if(state == OMX_StatePause)
        {
            DEBUG_DETAIL("OUT: SLEEPING AGAIN OUT THREAD\n");
            get_comp()->out_th_sleep();
            goto loopback_out;
        }
    }
    if(qsize > 0)
    {
        id = ident;
        DEBUG_DETAIL("OUT->state[%d]ident[%d]flushq[%d]fbd[%d]dataq[%d]\n",\
        get_state(),
        ident,
        get_q_size(cmd_q),
        get_q_size(bd_q),
        get_q_size(data_q));

        if(id == OMX_COMPONENT_GENERATE_FRAME_DONE)
        {
            frame_done_cb((OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_FTB)
        {
            process_ftb((OMX_HANDLETYPE)p1,
            (OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_EOS)
        {
            get_cb().EventHandler(&(get_comp()->m_cmp),
            get_app_data(),
            OMX_EventBufferFlag,
            1, 1, NULL );
        }
        else if(id == OMX_COMPONENT_SUSPEND)
        {
            DEBUG_PRINT("OUT: Rxed SUSPEND event m_eos_bm=%d\n",\
                                                     get_comp()->get_eos_bm());
            if((get_comp()->get_eos_bm()) != IP_OP_PORT_BITMASK)
            {
                append_data_to_temp_buf();
            }
        }
        else if(id == OMX_COMPONENT_RESUME)
        {
            DEBUG_PRINT("RESUMED...\n");
            //m_cb.EventHandler(&m_cmp, m_app_data,
            //    OMX_EventComponentResumed,0,0,NULL);
        }
        else if(id == OMX_COMPONENT_GENERATE_COMMAND)
        {
            // Execute FLUSH command
            if(p1 == OMX_CommandFlush)
            {
                DEBUG_DETAIL("Executing FLUSH command on Output port\n");
                execute_output_omx_flush();
                if(!m_bufMgr->isEmpty())
                {
                    DEBUG_DETAIL("OUT-->Flush TCXO Buffer\n");
                    m_bufMgr->reset();
                }

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
    return ;
}


void COmxBaseDecOut::append_data_to_temp_buf()
{
    int bytes_read = 0;
    unsigned int tot_bytes = 0;
    unsigned int cur_bytes = 0;
    META_OUT meta_out;
    META_OUT_SUS meta_out_sus;
    unsigned int TS=0;

    DEBUG_PRINT("COmxBaseDecOut::%s \n",__FUNCTION__);
    DEBUG_PRINT("Append: Space left [%d]",\
                 m_bufMgr->getBufFilledSpace());
    while( 1)
    {
        bytes_read = read(m_drv_fd, m_tmp_out_meta_buf,
                         (m_bufsize + sizeof(META_OUT)));
        DEBUG_PRINT("Append-->current bytes_read=%d total=%d\n",bytes_read,
                                                                tot_bytes);
        if(bytes_read > 0)
        {
            memcpy(&meta_out, m_tmp_out_meta_buf, sizeof(META_OUT));

            TS = (meta_out.nTimeStamp)/1000;
            /* user meta out offset to store the pcm frame length*/
            meta_out_sus.offsetVal =(OMX_U32)bytes_read - sizeof(META_OUT);
            meta_out_sus.nTimeStamp = TS;
            meta_out_sus.nFlags = meta_out.nFlags;
            if((meta_out.nFlags & 0x001) == OMX_BUFFERFLAG_EOS)
            {
                DEBUG_PRINT("**********READ=0**********************");
                DEBUG_PRINT("Append:EOS found Residual pcm[%d][%d]",tot_bytes,
                                               m_bufMgr->getBufFilledSpace());
                DEBUG_PRINT("**********READ=0**********************");
                break;
            }
            else
            {
                if(m_bufMgr->getBufFreeSpace() >= (bytes_read + sizeof(META_OUT_SUS)))
                {
                       cur_bytes = m_bufMgr->appendToBuf(
                                         (OMX_U8 *)&meta_out_sus,
                                         sizeof(META_OUT_SUS));
                       if(cur_bytes == 0)
                       {
                            DEBUG_PRINT_ERROR("Append: Reject residual pcm,"
                                     " no more space \n");
                            break;
                       }
                       else
                       {
                           tot_bytes += cur_bytes;
                           cur_bytes = m_bufMgr->appendToBuf(
                                      &m_tmp_out_meta_buf[sizeof(META_OUT)],
                                      (bytes_read - sizeof(META_OUT)));
                           if(cur_bytes == 0)
                           {
                                DEBUG_PRINT_ERROR("Append: Reject meta data,"
                                      " no more space \n");
                                break;
                           }
                           tot_bytes += cur_bytes;
                        }
                }
                else
                {
                       DEBUG_PRINT_ERROR("Append: Reject meta data,"
                                     " no more space \n");
                       break;
                 }
            }
        }
        else /*if(bytes_read ==-1)*/
        {
            DEBUG_PRINT("**********READ=-1**********************");
            DEBUG_PRINT("Append:Read=-1 Residual pcm[%d]",tot_bytes,
                                      m_bufMgr->getBufFilledSpace());
            DEBUG_PRINT("**********READ=-1**********************");
            break;

        }
    }
    // EOS recieved.
    get_comp()->setResumeFlg();
    fake_eos_recieved = OMX_TRUE;
    unsigned int eos_bm =0;
    if((eos_bm = get_comp()->get_eos_bm()) & IP_PORT_BITMASK)
    {
        eos_bm |= OP_PORT_BITMASK;
        get_comp()->set_eos_bm(eos_bm);
    }
    DEBUG_PRINT_ERROR("Rel DSP res, eos_bm[%d] sus[%d] \n",
                                 get_comp()->get_eos_bm(),
                                 get_comp()->getWaitForSuspendCmplFlg());
    ioctl(get_drv_fd(), AUDIO_STOP, 0);

    if(m_bufMgr)
    {
        get_comp()->setFilledSpaceInTcxoBuf(m_bufMgr->getBufFilledSpace());
    }

    if(get_comp()->getWaitForSuspendCmplFlg())
    {
        DEBUG_PRINT_ERROR("Release P-->Executing context to IL client.\n");
        get_comp()->release_wait_for_suspend();
    }

    if(get_comp()->getTimerExpiry())
    {
        get_comp()->post_command(0,0,OMX_COMPONENT_RESUME);
    }
    DEBUG_PRINT("DSP Resources released, timerexpiry=%d",\
                                                get_comp()->getTimerExpiry());
    DEBUG_PRINT_ERROR("Enter into TCXO shutdown mode\n");
    return ;
}
//////////////////////////////////////////////////////////////
// COMXBaseDec base decoder class
//////////////////////////////////////////////////////////////
// Constructor
//

COmxBaseDec::COmxBaseDec(const char *devName, unsigned int sf,
OMX_U8 ch):
COmxBase(devName,sf,ch),
m_ch_cfg(ch),
m_sample_rate(sf)

{
    DEBUG_PRINT("COmxBaseDec::%s \n",__FUNCTION__);
}

// Destructor
COmxBaseDec::~COmxBaseDec()
{
    DEBUG_PRINT("COmxBaseDec::%s \n",__FUNCTION__);

}

OMX_ERRORTYPE COmxBaseDec::component_init(OMX_STRING role)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT("COmxBaseDec::%s role[%s] \n",__FUNCTION__,role);
    eRet = COmxBase::component_init(role);
    if(eRet != OMX_ErrorNone)
        return eRet;

    m_timer = new COmxTimer(this);
    if(m_timer == NULL)
    {
        DEBUG_PRINT_ERROR("%s Failed to create Timer obj\n",__FUNCTION__);
    }
    m_timer->resetTimerExpiry();

    return eRet;
}


/* ======================================================================
FUNCTION
COmxBaseDec::ComponentDeInit

DESCRIPTION
Destroys the component and release memory allocated to the heap.

PARAMETERS

RETURN VALUE
OMX Error None if everything successful.

========================================================================== */

OMX_ERRORTYPE COmxBaseDec::component_deinit(
OMX_HANDLETYPE hComp)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    DEBUG_PRINT("COmxBaseDec::%s \n",__FUNCTION__);
    eRet = COmxBase::component_deinit(hComp);

    m_sample_rate = 0;
    m_ch_cfg = 0;
    DEBUG_PRINT("************************************\n");
    DEBUG_PRINT("COmxBaseDec::DEINIT COMPLETED");
    DEBUG_PRINT("************************************\n");

    return eRet;
}

OMX_ERRORTYPE  COmxBaseDec::process_cmd(OMX_IN OMX_HANDLETYPE hComp,
OMX_IN OMX_COMMANDTYPE  cmd,
OMX_IN OMX_U32       param1,
OMX_IN OMX_PTR      cmdData)
{
    // :: only decoder specific stuffs...
    COmxBase::process_cmd(hComp,cmd,param1,cmdData);
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
COmxBase::AllocateBuffer

DESCRIPTION
Returns zero if all the buffers released..

PARAMETERS
None.

RETURN VALUE
true/false

========================================================================== */
OMX_ERRORTYPE  COmxBaseDec::allocate_buffer(
OMX_IN OMX_HANDLETYPE            hComp,
OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
OMX_IN OMX_U32                   port,
OMX_IN OMX_PTR                   appData,
OMX_IN OMX_U32                   bytes)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    char  *buf_ptr=NULL;
    OMX_BUFFERHEADERTYPE * bufHdr = NULL;

    DEBUG_PRINT("COmxBaseDec::%s \n",__FUNCTION__);
    if(get_state() == OMX_StateInvalid)
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
        unsigned nBufSize = MAX(bytes, get_in_buf_size());
        buf_ptr = (char *) calloc((4+nBufSize + \
        sizeof(OMX_BUFFERHEADERTYPE)+sizeof(META_IN)) , 1);
        if(buf_ptr != NULL)
        {
            bufHdr =  (OMX_BUFFERHEADERTYPE *) buf_ptr;
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));
            bufHdr->pBuffer       = (OMX_U8 *)((buf_ptr) + sizeof(META_IN)+
            sizeof(OMX_BUFFERHEADERTYPE)+ 4);

            eRet = COmxBase::allocate_buffer(hComp,bufferHdr,port,appData,bytes);
        }
        else
        {
            DEBUG_PRINT_ERROR("allocate_buffer: failed to allocate i/p buffer of size %d\n", nBufSize);
            eRet = OMX_ErrorInsufficientResources;
        }
    }
    else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    {
        unsigned nBufSize = MAX(bytes,get_out_buf_size());
        buf_ptr = (char *) calloc( (nBufSize +
        sizeof(OMX_BUFFERHEADERTYPE)+sizeof(META_OUT)),1);
        if(buf_ptr != NULL) {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));
            bufHdr->pBuffer           = (OMX_U8 *)((buf_ptr) + sizeof(META_OUT)+
            sizeof(OMX_BUFFERHEADERTYPE));

            eRet = COmxBase::allocate_buffer(hComp,bufferHdr,port,appData,bytes);
        }
        else
        {
            DEBUG_PRINT_ERROR("allocate_buffer: failed to allocate o/p buffer of size %d\n", nBufSize);
            eRet = OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",
        (int)port);
        eRet = OMX_ErrorBadPortIndex;
    }
    return eRet;
}

OMX_U32 omxBufMgr::appendToBuf(OMX_U8 *srcbuf, OMX_U32 len)
{
    OMX_U32 tempLen=len;
    DEBUG_DETAIL("Remaining space in buffer=%d\n",m_remaining_space);
    DEBUG_DETAIL("Append: write=%p read=%p\n",m_write,m_read);

    if(!m_remaining_space || (m_remaining_space < len))
    {
        m_rejected_bytes += len;
        m_tot_rejected_bytes += len;
        DEBUG_DETAIL("Reject pcm data %d\n",m_rejected_bytes);
        return NULL;
    }

    if(m_write > m_read)
    {
        if((m_write + len) >= m_end )
        {
           OMX_U32 temp=0;
           memcpy(m_write,srcbuf,(m_end - m_write));
           temp = (m_end - m_write);
           m_write=m_start;
           memcpy(m_write,&srcbuf[temp],(len - temp));
           m_write += (len - temp);
        }
        else
        {
            memcpy(m_write,srcbuf,len);
            m_write += len;
        }
    }
    else if(m_write < m_read)
    {
        if((m_write + len) >= m_read )
        {
           OMX_U32 temp=0;
           memcpy(m_write,srcbuf,(m_read - m_write));
           temp = (m_read - m_write);
           m_write += temp;
           tempLen = temp;
        }
        else
        {
            memcpy(m_write,srcbuf,len);
            m_write += len;
        }
    }
    else
    {
        if((m_write + len) >= m_end )
        {
           OMX_U32 temp=0;
           memcpy(m_write,srcbuf,(m_end - m_write));
           temp = (m_end - m_write);
           m_write=m_start;
           memcpy(m_write,&srcbuf[temp],(len - temp));
           m_write += (len - temp);

        }
        else
        {
            memcpy(m_write,srcbuf,len);
            m_write += len;
        }

    }
    m_remaining_space -= tempLen;

    if(m_write >= m_end)
        m_write = m_start;

    DEBUG_DETAIL("==>Remaining space in buffer=%d\n",m_remaining_space);
    return tempLen;
}

OMX_U32 omxBufMgr::emptyToBuf(OMX_U8 *destBuf, OMX_U32 destLen)
{
    if(destBuf == NULL)
    {
        DEBUG_DETAIL("emptyToBuf: NULL destBuf...\n");
        return 0;
    }
    OMX_U32 filledSpace = (OMX_TCXO_BUFFER -1 - m_remaining_space);
    OMX_U32 len=0;

    DEBUG_DETAIL("***********************\n");
    DEBUG_DETAIL("FilledSpace=%d destLen=%d\n",filledSpace,destLen);
    DEBUG_DETAIL("***********************\n");
    if(!filledSpace)
    {
        DEBUG_DETAIL("Buf empty ...\n");
        return 0;
    }
    if(filledSpace > destLen)
    {
        len = destLen;
    }
    else
    {
        len = filledSpace;
    }
    if(m_read > m_write)
    {
        if((m_read + len) >= m_end)
        {
            memcpy(destBuf,m_read,(m_end - m_read));
            memset(m_read,0,(m_end - m_read));

            OMX_U32 temp = (m_end - m_read);
            m_read=m_start;
            memcpy(&destBuf[temp],m_read,(len- temp));
            memset(m_read,0,(len - temp));
            m_read += (len - temp);
        }
        else
        {
            memcpy(destBuf,m_read,len);
            memset(m_read,0,len);
            m_read += len;
        }
    }
    else
    {
        memcpy(destBuf,m_read,len);
        memset(m_read,0,len);
        m_read += len;
    }
    m_remaining_space += len;

    if(m_read >= m_end)
        m_read = m_start;
DEBUG_PRINT("--> Copied %d bytes\n",len);
    return len;
}

void omxBufMgr::print()
{
    unsigned char i=0;
    DEBUG_DETAIL("************************\n");
    DEBUG_DETAIL("m_start=%p m_end=%p m_read=%p m_write=%p "
                 "m_remaining_space=%d m_tot_rejected_bytes=%d\n",
                                 m_start,m_end,
                                 m_read,m_write,
                                 m_remaining_space,m_tot_rejected_bytes);
    while(i < OMX_TCXO_BUFFER)
    {
        DEBUG_DETAIL("%p-->0x%x\n ",&m_start[i],m_start[i]);
        i++;
    }
    DEBUG_DETAIL("\n************************\n");
}


