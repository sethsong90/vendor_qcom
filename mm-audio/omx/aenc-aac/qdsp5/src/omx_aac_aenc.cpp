
/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file omx_aenc_aac.c
  This module contains the implementation of the OpenMAX core & component.

Copyright (c) 2006-2010 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

/*============================================================================
                              Edit History

$Header: //depot/asic/sandbox/users/ronaldk/videodec/OMXVdecCommon/omx_vdec.cpp#1 $
when       who     what, where, why
--------   ---     -------------------------------------------------------
08/13/08   ---     Initial file

============================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <fcntl.h>
#include <omx_aac_aenc.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/time.h>

using namespace std;

void omx_aac_aenc::wait_for_event()
{
    pthread_mutex_lock(&m_event_lock);
    while(m_is_event_done == 0)
    {
        pthread_cond_wait(&cond, &m_event_lock);
    }
    m_is_event_done = 0;
    pthread_mutex_unlock(&m_event_lock);
}

void omx_aac_aenc::event_complete()
{
    pthread_mutex_lock(&m_event_lock);
    if(m_is_event_done == 0) {
        m_is_event_done = 1;
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&m_event_lock);
}

// omx_cmd_queue destructor
omx_aac_aenc::omx_cmd_queue::~omx_cmd_queue()
{
  // Nothing to do
}

// omx cmd queue constructor
omx_aac_aenc::omx_cmd_queue::omx_cmd_queue(): m_read(0),m_write(0),m_size(0)
{
  memset(m_q,      0,sizeof(omx_event)*OMX_CORE_CONTROL_CMDQ_SIZE);
}

// omx cmd queue insert
bool omx_aac_aenc::omx_cmd_queue::insert_entry(unsigned p1, unsigned p2, unsigned id)
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
bool omx_aac_aenc::omx_cmd_queue::delete_entry(unsigned *p1, unsigned *p2, unsigned *id)
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
    DEBUG_PRINT_ERROR("ERROR Delete!!! Command Queue Full");
  }

  return ret;
}

// factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
  return (new omx_aac_aenc);
}


/* ======================================================================
FUNCTION
  omx_aac_aenc::omx_aac_aenc

DESCRIPTION
  Constructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_aac_aenc::omx_aac_aenc(): m_state(OMX_StateInvalid),
                              m_app_data(NULL),
                              m_volume(25),
			      m_is_alloc_buf(0),
                              m_cmd_svr(NULL),
                              m_cmd_cln(NULL),
                              m_cmd_cln_input(NULL),
                              m_drv_fd(-1),
                              frameDuration(0),
                              m_inp_buf_count(0),
                              m_out_buf_count(0),
                              output_buffer_size(OMX_AAC_OUTPUT_BUFFER_SIZE),
                              input_buffer_size(OMX_CORE_INPUT_BUFFER_SIZE),
                              m_flags(0),
                              fcount(0),
                              nTimestamp(0),
                              pcm_input(0),
                              m_is_event_done(0),
                              m_msg_cnt(0),
                              m_num_out_buf(0),
		              m_num_in_buf(0),
                              m_idle_transition(0),
			      m_session_id(0),
			      flag(0),
                              bFlushinprogress(false),
                              m_flush_cnt(0)
{
  int cond_ret = 0;
  memset(&m_cmp,       0,     sizeof(m_cmp));
  memset(&m_cb,        0,      sizeof(m_cb));
  memset(&m_aenc_param, 0, sizeof(m_aenc_param));
  m_aenc_param.nSize = sizeof(m_aenc_param);
  m_aenc_param.eAACProfile = OMX_AUDIO_AACPROFILETYPE(2);
  m_aenc_param.eChannelMode = OMX_AUDIO_CHANNELMODETYPE(0);
  m_aenc_param.nAACERtools = 0;
  m_aenc_param.nAudioBandWidth = 0;
  m_aenc_param.nBitRate = 0;
  m_aenc_param.nFrameLength = 0;
  m_aenc_param.nPortIndex = 0;
  m_aenc_param.nSampleRate = 44100;
  m_aenc_param.nChannels = 2;
  pthread_mutexattr_init(&m_lock_attr);
  pthread_mutex_init(&m_lock, &m_lock_attr);

    pthread_mutexattr_init(&m_event_lock_attr);
    pthread_mutex_init(&m_event_lock, &m_event_lock_attr);

    pthread_mutexattr_init(&m_flush_lock_attr);
    pthread_mutex_init(&m_flush_lock, &m_flush_lock_attr);

    pthread_mutexattr_init(&out_buf_count_lock_attr);
    pthread_mutex_init(&out_buf_count_lock, &out_buf_count_lock_attr);

    pthread_mutexattr_init(&in_buf_count_lock_attr);
    pthread_mutex_init(&in_buf_count_lock, &in_buf_count_lock_attr);

    pthread_mutexattr_init(&m_state_attr);
    pthread_mutex_init(&m_state_lock, &m_state_attr);

    if ((cond_ret = pthread_cond_init (&cond, NULL)) != 0)
    {
       DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for cond\n");
       if (cond_ret == EAGAIN)
         DEBUG_PRINT_ERROR("The system lacked necessary resources(other than mem)\n");
       else if (cond_ret == ENOMEM)
          DEBUG_PRINT_ERROR("Insufficient memory to initialise condition variable\n");
    }
    sem_init(&sem_States,0, 0);
    sem_init(&sem_read_msg,0, 0);

    pthread_mutexattr_init(&m_outputlock_attr);
    pthread_mutex_init(&m_outputlock, &m_outputlock_attr);

    pthread_mutexattr_init(&m_inputlock_attr);
    pthread_mutex_init(&m_inputlock, &m_inputlock_attr);

  return;
}


/* ======================================================================
FUNCTION
  omx_aac_aenc::~omx_aac_aenc

DESCRIPTION
  Destructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_aac_aenc::~omx_aac_aenc()
{
    pthread_mutexattr_destroy(&m_lock_attr);
    pthread_mutexattr_destroy(&m_state_attr);
    pthread_mutexattr_destroy(&out_buf_count_lock_attr);
    pthread_mutexattr_destroy(&in_buf_count_lock_attr);

    pthread_mutexattr_destroy(&m_outputlock_attr);
    pthread_mutex_destroy(&m_outputlock);

    pthread_mutexattr_destroy(&m_inputlock_attr);
    pthread_mutex_destroy(&m_inputlock);

    pthread_mutexattr_destroy(&m_event_lock_attr);
    pthread_mutex_destroy(&m_event_lock);

    pthread_mutexattr_destroy(&m_flush_lock_attr);
    pthread_mutex_destroy(&m_flush_lock);

    pthread_mutex_destroy(&m_lock);
    pthread_mutex_destroy(&m_state_lock);
    pthread_mutex_destroy(&out_buf_count_lock);
    m_drv_fd = -1;
    frameDuration = 0;
    m_inp_buf_count = 0;
    m_flags = 0;
    fcount = 0;
    nTimestamp = 0;
    pcm_input = 0;
    m_num_out_buf = 0;
    m_is_event_done = 0;
    m_idle_transition = 0;
    m_msg_cnt = 0;
    m_out_buf_count = 0;
    pthread_cond_destroy(&cond);
    sem_destroy (&sem_States);
    sem_destroy (&sem_read_msg);
    return;
}

/*=============================================================================
FUNCTION:
flush_ack

DESCRIPTION:


INPUT/OUTPUT PARAMETERS:
None

RETURN VALUE:
None

Dependency:
None

SIDE EFFECTS:
None
=============================================================================*/
void omx_aac_aenc::flush_ack(void)
{
    // Decrement the FLUSH ACK count and notify the waiting recepients
    pthread_mutex_lock(&m_flush_lock);
    --m_flush_cnt;
    if ( 0 == m_flush_cnt )
    {
	event_complete();
    }
    DEBUG_PRINT("Rxed FLUSH ACK cnt=%d\n",m_flush_cnt);
    pthread_mutex_unlock(&m_flush_lock);
}

/**
  @brief memory function for sending EmptyBufferDone event
   back to IL client

  @param bufHdr OMX buffer header to be passed back to IL client
  @return none
 */
void omx_aac_aenc::buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{
  if(m_cb.EmptyBufferDone)
  {
    pthread_mutex_lock(&in_buf_count_lock);
    --m_num_in_buf;
    PrintFrameHdr(OMX_COMPONENT_GENERATE_BUFFER_DONE,bufHdr);

    pthread_mutex_unlock(&in_buf_count_lock);
    m_cb.EmptyBufferDone(&m_cmp, m_app_data, bufHdr);
  }

  return;
}

void omx_aac_aenc::frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{
  if(m_cb.FillBufferDone)
  {
    pthread_mutex_lock(&out_buf_count_lock);
    --m_num_out_buf;
	if(!pcm_input)
	{
		if (fcount == 0)
		{
			bufHdr->nTimeStamp = 0;
			m_swc = (OMX_TICKS)aenc_time_microsec();
			DEBUG_PRINT("FBD-->SWC=%lld\n",m_swc);
		}
		else
		{
			bufHdr->nTimeStamp = \
				((OMX_TICKS)aenc_time_microsec() - m_swc);
		}
	}
    pthread_mutex_unlock(&out_buf_count_lock);
    m_cb.FillBufferDone(&m_cmp, m_app_data, bufHdr);
    PrintFrameHdr(OMX_COMPONENT_GENERATE_FRAME_DONE,bufHdr);
    fcount++;
    m_fbd_cnt++;
  }
  return;
}

long long omx_aac_aenc::aenc_time_microsec()
{
   struct timeval time;
   long long time_microsec = 0;

   if (gettimeofday(&time, NULL) == 0)
   {
      time_microsec = ((long long) time.tv_sec * 1000 * 1000) + 
	                                     ((long long) time.tv_usec) ;
   }
   DEBUG_PRINT("Time in microsecs  %lld\n",time_microsec);
   return time_microsec;
}

/** ======================================================================
 @brief static member function for handling all commands from IL client

  IL client commands are processed and callbacks are generated
  through this routine. Audio Command Server provides the thread context
  for this routine.

void omx_aac_aenc::process_output_cb(void *client_d  @param id command identifier
  @return none
 */

void omx_aac_aenc::process_output_cb(void *client_data, unsigned char id)
{
	unsigned      p1; // Parameter - 1
	unsigned      p2; // Parameter - 2
	unsigned      ident;
	unsigned      qsize=0; // qsize
	unsigned      tot_qsize=0; // qsize
	unsigned int  port =1;
	omx_aac_aenc  *pThis = (omx_aac_aenc *) client_data;
	OMX_STATETYPE state;

	pthread_mutex_lock(&pThis->m_state_lock);
	pThis->get_state(&pThis->m_cmp, &state);
	pthread_mutex_unlock(&pThis->m_state_lock);

	if(state == OMX_StateLoaded)
	{
		return ;
	}
	pthread_mutex_lock(&pThis->m_outputlock);

	qsize = pThis->m_output_ctrl_cmd_q.m_size;
	tot_qsize = pThis->m_output_ctrl_cmd_q.m_size;
	tot_qsize += pThis->m_output_ctrl_fbd_q.m_size;
	tot_qsize += pThis->m_output_q.m_size;

	if ( 0 == tot_qsize )
	{
		pthread_mutex_unlock(&pThis->m_outputlock);
		DEBUG_DETAIL("OUT-->BREAK FROM LOOP...%d\n",tot_qsize);
		return;
	}

	if (qsize)
	{
		pThis->m_output_ctrl_cmd_q.delete_entry(&p1,&p2,&ident);
		DEBUG_PRINT("Deleting id in o/p thread %d \n",ident);
	} else if ( (qsize = pThis->m_output_ctrl_fbd_q.m_size) &&
		(state == OMX_StateExecuting) )
	{
		// then process FBD's
		pThis->m_output_ctrl_fbd_q.delete_entry(&p1,&p2,&ident);
		DEBUG_PRINT("Deleting id in o/p thread %d \n",ident);
	} else if ( (qsize = pThis->m_output_q.m_size) &&
		(state == OMX_StateExecuting) )
	{
		// if no FLUSH and FBD's then process FTB's
		pThis->m_output_q.delete_entry(&p1,&p2,&ident);
		DEBUG_PRINT("Deleting id in o/p thread %d \n",ident);
	} else if ( state == OMX_StateLoaded )
	{
		pthread_mutex_unlock(&pThis->m_outputlock);
		DEBUG_PRINT("IN: ***in OMX_StateLoaded so exiting\n");
		return ;
	} else
	{
		 qsize = 0;
	}
	pthread_mutex_unlock(&pThis->m_outputlock);

	if ( qsize > 0 )
	{
		id = ident;
		ident = 0;
		DEBUG_DETAIL("OUT->state[%d]ident[%d]flushq[%d]fbd[%d]dataq[%d]\n",\
			pThis->m_state,
			ident,
			pThis->m_output_ctrl_cmd_q.m_size,
			pThis->m_output_ctrl_fbd_q.m_size,
			pThis->m_output_q.m_size);

		if ( OMX_COMPONENT_GENERATE_FRAME_DONE == id )
		{
			pThis->frame_done_cb((OMX_BUFFERHEADERTYPE *)p2);
		} else if ( OMX_COMPONENT_GENERATE_FTB == id )
		{
			pThis->fill_this_buffer_proxy((OMX_HANDLETYPE)p1,
							(OMX_BUFFERHEADERTYPE *)p2);
		} else if(id == OMX_COMPONENT_GENERATE_EOS)
		{
			DEBUG_PRINT("Calling pThis->m_cb.EventHandler = %p\n", pThis->m_cb.EventHandler);
			pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
						OMX_EventBufferFlag, 1, 1, &port);
		} else if ( OMX_COMPONENT_GENERATE_COMMAND == id)
		{
			if (OMX_CommandFlush == p1)
			{
				DEBUG_DETAIL(" Executing FLUSH command on Output port\n");
				pThis->execute_output_omx_flush();
			} else
			{
				DEBUG_PRINT_ERROR("ERROR:IN-->Invalid command \n");
			}
		}
		else
		{
			DEBUG_PRINT_ERROR("ERROR:OUT-->Invalid Id[%d]\n",id);
		}
	} else
	{
		DEBUG_DETAIL("ERROR: OUT--> Empty OUTPUTQ\n");
	}
	return;
}

void omx_aac_aenc::process_event_cb(void *client_data, unsigned char id)
{
  unsigned      p1; // Parameter - 1
  unsigned      p2; // Parameter - 2
  unsigned      ident;
  unsigned qsize=0; // qsize
  omx_aac_aenc  *pThis              = (omx_aac_aenc *) client_data;
  OMX_STATETYPE state;

  DEBUG_PRINT("OMXCntrlProessMsgCb[%x,%d] Enter:", (unsigned) client_data,
              (unsigned)id);
  if(!pThis)
  {
    DEBUG_PRINT_ERROR("ERROR : ProcessMsgCb: Context is incorrect; bailing out\n");
    return;
  }

  // Protect the shared queue data structure
  do
  {
    pthread_mutex_lock(&pThis->m_lock);

    pthread_mutex_lock(&pThis->m_state_lock);
    pThis->get_state(&pThis->m_cmp, &state);
    pthread_mutex_unlock(&pThis->m_state_lock);

    qsize = pThis->m_cmd_q.m_size;
    DEBUG_PRINT("CMD-->QSIZE=%d state=%d\n",\
                              pThis->m_cmd_q.m_size,state);

    if(qsize)
    {
      pThis->m_cmd_q.delete_entry(&p1,&p2,&ident);
    } else {
      OMX_STATETYPE state;

      qsize = pThis->m_data_q.m_size;
      pthread_mutex_lock(&pThis->m_state_lock);
      pThis->get_state(&pThis->m_cmp, &state);
      pthread_mutex_unlock(&pThis->m_state_lock);

      if ((qsize) && (state == OMX_StateExecuting))
      {
        pThis->m_data_q.delete_entry(&p1, &p2, &ident);
      } else
      {
        qsize = 0;
      }
    }

    if(qsize)
    {
      pThis->m_msg_cnt ++;
    }
    pthread_mutex_unlock(&pThis->m_lock);

    if(qsize > 0)
    {
      id = ident;
      DEBUG_PRINT("Process ->state[%d]id[%d]\n",pThis->m_state,ident);
      if(id == OMX_COMPONENT_GENERATE_EVENT)
      {
        if (pThis->m_cb.EventHandler)
        {
          if (p1 == OMX_CommandStateSet)
          {
             pthread_mutex_lock(&pThis->m_state_lock);
             pThis->m_state = (OMX_STATETYPE) p2;
             pthread_mutex_unlock(&pThis->m_state_lock);
             DEBUG_PRINT("Process -> state set to %d \n", pThis->m_state);
          }

          if (pThis->m_state == OMX_StateInvalid) {
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data, /* What exactly should I return */
                                     OMX_EventError, OMX_ErrorInvalidState,
                                     0, NULL );
          } else {
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                     OMX_EventCmdComplete, p1, p2, NULL );
          }

        }
        else
        {
          DEBUG_PRINT_ERROR("Error: ProcessMsgCb ignored due to NULL callbacks\n");
        }
      }
      else if(id == OMX_COMPONENT_GENERATE_BUFFER_DONE)
      {
        pThis->buffer_done_cb((OMX_BUFFERHEADERTYPE *)p2);
      }
      else if(id == OMX_COMPONENT_GENERATE_ETB)
      {
        pThis->empty_this_buffer_proxy((OMX_HANDLETYPE)p1,(OMX_BUFFERHEADERTYPE *)p2);
      }
      else if(id == OMX_COMPONENT_GENERATE_COMMAND)
      {
        pThis->send_command_proxy(&pThis->m_cmp,(OMX_COMMANDTYPE)p1,(OMX_U32)p2,(OMX_PTR)NULL);
      }
      else
      {
        DEBUG_PRINT_ERROR("Error: ProcessMsgCb Ignored due to Invalid Identifier\n");
      }
      DEBUG_PRINT("OMXCntrlProessMsgCb[%x,%d] Exit: \n",
                  (unsigned)client_data,(unsigned)id);
    }
    else
    {
      DEBUG_PRINT("Error: ProcessMsgCb Ignored due to empty CmdQ\n");
    }
    pthread_mutex_lock(&pThis->m_lock);
    qsize = pThis->m_cmd_q.m_size;
    pthread_mutex_unlock(&pThis->m_lock);
  } while(qsize>0);
  return;
}

void omx_aac_aenc::process_input_cb(void *client_data, unsigned char id)
{
	unsigned      p1; // Parameter - 1
	unsigned      p2; // Parameter - 2
	unsigned      ident;
	unsigned      qsize=0; // qsize
	unsigned      tot_qsize=0; // qsize
	unsigned int port = 0;
	omx_aac_aenc  *pThis = (omx_aac_aenc *) client_data;
	OMX_STATETYPE state;

	pthread_mutex_lock(&pThis->m_state_lock);
	pThis->get_state(&pThis->m_cmp, &state);
	pthread_mutex_unlock(&pThis->m_state_lock);

	DEBUG_PRINT("Input thread function -------------1 \n");
	if(state == OMX_StateLoaded)
	{
		return ;
	}

	pthread_mutex_lock(&pThis->m_inputlock);
	qsize = pThis->m_input_ctrl_cmd_q.m_size;
	tot_qsize = pThis->m_input_ctrl_cmd_q.m_size;
	tot_qsize += pThis->m_input_ctrl_ebd_q.m_size;
	tot_qsize += pThis->m_input_q.m_size;

	if ( 0 == tot_qsize )
	{
		pthread_mutex_unlock(&pThis->m_inputlock);
		DEBUG_DETAIL("OUT-->BREAK FROM LOOP...%d\n",tot_qsize);
		return;
	}

	if (qsize)
	{
		pThis->m_input_ctrl_cmd_q.delete_entry(&p1,&p2,&ident);
	        DEBUG_PRINT("Deleting id %d \n",ident);
	} else if ( (qsize = pThis->m_input_ctrl_ebd_q.m_size) &&
			(state == OMX_StateExecuting) )
	{
		pThis->m_input_ctrl_ebd_q.delete_entry(&p1,&p2,&ident);
		DEBUG_PRINT("Deleting id %d \n",ident);
	} else if ( (qsize = pThis->m_input_q.m_size) &&
			(state == OMX_StateExecuting) )
	{
		pThis->m_input_q.delete_entry(&p1,&p2,&ident);
		DEBUG_PRINT("Deleting id %d \n",ident);
	} else if ( state == OMX_StateLoaded )
	{
		pthread_mutex_unlock(&pThis->m_inputlock);
		DEBUG_PRINT("IN: ***in OMX_StateLoaded so exiting\n");
		return ;
	} else
	{
		 qsize = 0;
	}
	pthread_mutex_unlock(&pThis->m_inputlock);

	if ( qsize > 0 )
	{
		id = ident;
		ident = 0;
		DEBUG_DETAIL("IN->state[%d]ident[%d]flushq[%d]fbd[%d]dataq[%d]\n",\
				pThis->m_state,
				ident,
				pThis->m_input_ctrl_cmd_q.m_size,
				pThis->m_input_ctrl_ebd_q.m_size,
				pThis->m_input_q.m_size);
		if ( OMX_COMPONENT_GENERATE_BUFFER_DONE == id )
		{
			pThis->buffer_done_cb((OMX_BUFFERHEADERTYPE *)p2);
		} else if ( OMX_COMPONENT_GENERATE_ETB == id )
		{
			DEBUG_PRINT("Processing ETB \n");
			pThis->empty_this_buffer_proxy((OMX_HANDLETYPE)p1,
							(OMX_BUFFERHEADERTYPE *)p2);
		} else if(id == OMX_COMPONENT_GENERATE_EOS)
		{
			DEBUG_PRINT("Posting input EOS event to client\n");
			pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
						OMX_EventBufferFlag, 0, 1, &port);
		} else if ( OMX_COMPONENT_GENERATE_COMMAND == id)
		{
			if (OMX_CommandFlush == p1)
			{
				DEBUG_DETAIL(" Executing FLUSH command on Input port\n");
				pThis->execute_input_omx_flush();
			} else
			{
				DEBUG_PRINT_ERROR("ERROR:IN-->Invalid command \n");
			}
		} else
		{
			DEBUG_PRINT_ERROR("ERROR:IN-->Invalid Id[%d]\n",id);
		}
	} else
	{
		DEBUG_DETAIL("ERROR: IN--> Empty INPUTQ\n");
	}
	return;
}


/**
 @brief member function for performing component initialization

 @param role C string mandating role of this component
 @return Error status
 */
OMX_ERRORTYPE omx_aac_aenc::component_init(OMX_STRING role)
{

  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  /* Ignore role */

  m_state                   = OMX_StateLoaded;
  /* DSP does not give information about the bitstream
     randomly assign the value right now. Query will result in
     incorrect param */
  memset(&m_aenc_param, 0, sizeof(m_aenc_param));
  m_aenc_param.nSize = sizeof(m_aenc_param);
  m_aenc_param.nSampleRate = DEFAULT_SAMPLINGRATE;
  m_volume = 25; /* Close to unity gain */
  m_aenc_param.nChannels = DEFAULT_CHANNEL_CNT;
  m_aenc_param.nBitRate = DEFAULT_BITRATE;
  m_aenc_param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;
  memset(&m_pcm_param, 0, sizeof(m_pcm_param));
  m_pcm_param.nSize = sizeof(m_pcm_param);
  m_pcm_param.nSamplingRate = DEFAULT_SAMPLINGRATE;
  m_pcm_param.nChannels = DEFAULT_CHANNEL_CNT;

  fcount = 0;
  nTimestamp = 0;
  DEBUG_PRINT(" Enabling Non-Tunnel mode \n");
  pcm_input = 1;    /* by default enable non-tunnel mode */
  m_num_out_buf = 0;
  m_num_in_buf = 0;
  m_idle_transition = 0;
  frameDuration = 0;
  m_fbd_cnt=0;
  m_swc = 0;
  tickcount = 0;
  m_tmp_in_meta_buf = NULL;
  m_tmp_out_meta_buf = NULL;

  DEBUG_PRINT(" component init: role = %s\n",role);
  if ( !strcmp(role,"OMX.qcom.audio.encoder.aac") )
  {
        pcm_input = 1;
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
  } else if ( !strcmp(role,"OMX.qcom.audio.encoder.tunneled.aac") )
  {
        pcm_input = 0;
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
  } else
  {
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED is invalid\n", role);
  }

  #ifndef AUDIOV2
  if(0 == pcm_input)
  {
        m_drv_fd = open("/dev/msm_pcm_in",O_RDONLY);
  }
  else
  {
        m_drv_fd = open("/dev/msm_pcm_in",O_RDWR);
  }
  #else
  if(0 == pcm_input)
  {
        m_drv_fd = open("/dev/msm_aac_in",O_RDONLY);
  }
  else
  {
        m_drv_fd = open("/dev/msm_aac_in",O_RDWR);
  }
  #endif

  if (m_drv_fd < 0)
  {
      DEBUG_PRINT_ERROR("OMXCORE-SM: device open fail\n");
      eRet =  OMX_ErrorInsufficientResources;
  }
  if(ioctl(m_drv_fd, AUDIO_GET_SESSION_ID,&m_session_id) == -1)
  {
      DEBUG_PRINT("AUDIO_GET_SESSION_ID FAILED\n");
  }
  //For NT mode handling meta_in info
  if (pcm_input)
  {
	m_tmp_in_meta_buf = (OMX_U8*)malloc(sizeof(OMX_U8)*
            (OMX_CORE_INPUT_BUFFER_SIZE + sizeof(META_IN)));
        if ( m_tmp_in_meta_buf == NULL )
            DEBUG_PRINT("Mem alloc failed for in meta buf\n");
	m_tmp_out_meta_buf = (OMX_U8*)malloc(sizeof(OMX_U8)*
            (OMX_AAC_OUTPUT_BUFFER_SIZE + sizeof(META_OUT)));
        if ( m_tmp_out_meta_buf == NULL )
            DEBUG_PRINT("Mem alloc failed for out meta buf\n");
  }

  if(!m_cmd_svr)
  {
    m_cmd_svr = aenc_svr_start(process_event_cb, this);
    if(!m_cmd_svr)
    {
        DEBUG_PRINT_ERROR("ERROR!!! comand server open failed\n");
        return OMX_ErrorInsufficientResources;
    }
	else
		DEBUG_PRINT("Control thread creation success\n");
  }

  if(!m_cmd_cln)
  {
    m_cmd_cln = aenc_cln_start(process_output_cb, this);
    if(!m_cmd_cln)
    {
        DEBUG_PRINT_ERROR("ERROR!!! comand o/p Client open failed\n");
        return OMX_ErrorInsufficientResources;
    }
	else
		DEBUG_PRINT("Output thread creation success\n");
  }

  //Input thread creation for NT mode encoder
  if(pcm_input)
  {
        if(!m_cmd_cln_input)
        {
                m_cmd_cln_input = aenc_cln_start(process_input_cb, this);
                if(!m_cmd_cln_input)
                {
                        DEBUG_PRINT_ERROR("ERROR!!! comand i/p Client open failed\n");
						return OMX_ErrorInsufficientResources;
                }
                else
                        DEBUG_PRINT("Input thread creation success\n");
        }
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
OMX_ERRORTYPE  omx_aac_aenc::get_component_version(OMX_IN OMX_HANDLETYPE               hComp,
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
OMX_ERRORTYPE  omx_aac_aenc::send_command(OMX_IN OMX_HANDLETYPE hComp,
                                          OMX_IN OMX_COMMANDTYPE  cmd,
                                          OMX_IN OMX_U32       param1,
                                          OMX_IN OMX_PTR      cmdData)
{
  int portIndex = (int)param1;

  if(hComp == NULL)
  {
        cmdData = NULL;
	DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
	return OMX_ErrorBadParameter;
  }
  if ( OMX_StateInvalid == m_state )
  {
  	return OMX_ErrorInvalidState;
  }
  if ( (cmd == OMX_CommandFlush) && (portIndex > 1) )
  {
	return OMX_ErrorBadPortIndex;
  }

  post_event((unsigned)cmd,(unsigned)param1,OMX_COMPONENT_GENERATE_COMMAND);

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
OMX_ERRORTYPE  omx_aac_aenc::send_command_proxy(OMX_IN OMX_HANDLETYPE hComp,
                                          OMX_IN OMX_COMMANDTYPE  cmd,
                                          OMX_IN OMX_U32       param1,
                                          OMX_IN OMX_PTR      cmdData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  //   Handle only IDLE and executing
  OMX_STATETYPE eState = (OMX_STATETYPE) param1;
  int bFlag = 1;

  (void)hComp;
  (void)cmdData;
  if(cmd == OMX_CommandStateSet)
  {
    /***************************/
    /* Current State is Loaded */
    /***************************/
    if(m_state == OMX_StateLoaded)
    {
      if(eState == OMX_StateIdle)
      {
          if (allocate_done()) {
            DEBUG_PRINT("OMXCORE-SM: Loaded->Idle\n");
          } else {
            DEBUG_PRINT("OMXCORE-SM: Loaded-->Idle-Pending\n");
            BITMASK_SET(&m_flags, OMX_COMPONENT_IDLE_PENDING);
            bFlag = 0;
          }
      }
      else
      {
        DEBUG_PRINT_ERROR("OMXCORE-SM: Loaded-->Invalid(%d Not Handled)\n",eState);
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
        if(release_done())
        {
		DEBUG_PRINT("OMXCORE-SM: Idle-->Loaded m_out_buf_count =%d\n",\
								    m_out_buf_count);
		post_event(OMX_CommandStateSet,
			OMX_StateLoaded,OMX_COMPONENT_GENERATE_EVENT);
        }
        else
        {
          DEBUG_PRINT("OMXCORE-SM: Idle-->Loaded-Pending m_out_buf_count=%d\n",\
                                                                    m_out_buf_count);
          BITMASK_SET(&m_flags, OMX_COMPONENT_LOADING_PENDING);
          // Skip the event notification
          bFlag = 0;
        }
      }
      else if(eState == OMX_StateExecuting)
      {
        struct msm_audio_aac_enc_config drv_aac_enc_config;
        struct msm_audio_stream_config drv_stream_config;

        DEBUG_PRINT("configure Driver for AAC Encoding sample rate = %lu \n",m_aenc_param.nSampleRate);
        if(ioctl(m_drv_fd, AUDIO_GET_STREAM_CONFIG, &drv_stream_config) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_GET_STREAM_CONFIG failed, errno[%d]\n", errno);
        }
        drv_stream_config.buffer_size  = OMX_AAC_OUTPUT_BUFFER_SIZE;
        drv_stream_config.buffer_count = OMX_CORE_NUM_OUTPUT_BUFFERS;
        if(ioctl(m_drv_fd, AUDIO_SET_STREAM_CONFIG, &drv_stream_config) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_SET_STREAM_CONFIG failed, errno[%d]\n", errno);
        }

        if(ioctl(m_drv_fd, AUDIO_GET_AAC_ENC_CONFIG, &drv_aac_enc_config) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_GET_AAC_ENC_CONFIG failed, errno[%d]\n", errno);
        }
        drv_aac_enc_config.channels = m_aenc_param.nChannels;
        drv_aac_enc_config.sample_rate = m_aenc_param.nSampleRate;
        drv_aac_enc_config.bit_rate =  m_aenc_param.nBitRate;
        frameDuration = (NUM_SAMPLES_PER_FRAME * MILLISECOND)/m_aenc_param.nSampleRate;
        DEBUG_PRINT("channels %lu samplerate %lu bitrate %lu frameduration %d\n"\
        ,m_aenc_param.nChannels,m_aenc_param.nSampleRate, m_aenc_param.nBitRate,frameDuration); 
        if(ioctl(m_drv_fd, AUDIO_SET_AAC_ENC_CONFIG, &drv_aac_enc_config) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_SET_AAC_ENC_CONFIG failed, errno[%d]\n", errno);
        }

        if(ioctl(m_drv_fd, AUDIO_START, 0) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_START failed, errno[%d]\n", errno);
        }

        DEBUG_PRINT("OMXCORE-SM: Idle-->Executing\n");
      }
      else
      {
        DEBUG_PRINT_ERROR("OMXCORE-SM: Idle --> %d Not Handled\n",eState);
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
            if ( !pcm_input )
            {
		bFlushinprogress = true;
                execute_omx_flush(1,false); // Flush output ports
		bFlushinprogress = false;
            } else
            {
		bFlushinprogress = true;
		execute_omx_flush(-1,false); // Flush all ports
		bFlushinprogress = false;
		DEBUG_PRINT("State changed from exe->idle \n");
		DEBUG_PRINT("m_num_out_buf = %d \n  m_num_in_buf = %d\n", m_num_out_buf, m_num_in_buf);
            }
	    post_event(OMX_CommandStateSet,
			OMX_StateIdle,
			OMX_COMPONENT_GENERATE_EVENT);
       }
       else if(eState == OMX_StatePause)
       {
         /* ioctl(m_drv_fd, AUDIO_PAUSE, 0); Not implemented at this point */
         DEBUG_PRINT("OMXCORE-SM: Executing --> Paused \n");
       }
       else
       {
         DEBUG_PRINT_ERROR("OMXCORE-SM: Executing --> %d Not Handled\n",eState);
         eRet = OMX_ErrorBadParameter;
       }
    }
    /***************************/
    /* Current State is Pause  */
    /***************************/
    else if(m_state == OMX_StatePause)
    {
      if(eState == OMX_StateExecuting)
      {
        /* ioctl(m_drv_fd, AUDIO_RESUME, 0); Not implemented at this point */
        DEBUG_PRINT("OMXCORE-SM: Paused --> Executing \n");
      }
      else if(eState == OMX_StateIdle)
      {
        DEBUG_PRINT("OMXCORE-SM: Paused --> Idle \n");
	bFlushinprogress = true;
        execute_omx_flush(-1,false);
	bFlushinprogress = false;
        ioctl(m_drv_fd, AUDIO_STOP, 0);
      }
      else
      {
        DEBUG_PRINT("OMXCORE-SM: Paused --> %d Not Handled\n",eState);
        eRet = OMX_ErrorBadParameter;
      }
    }
    else
    {
      DEBUG_PRINT_ERROR("OMXCORE-SM: %d --> %d(Not Handled)\n",m_state,eState);
      eRet = OMX_ErrorBadParameter;
    }
  }
  else if (cmd == OMX_CommandFlush)
  {

    if ((param1 == OMX_CORE_INPUT_PORT_INDEX) ||
        (param1 == OMX_ALL))
    {
      bFlushinprogress = true;
      execute_omx_flush(OMX_CORE_INPUT_PORT_INDEX, true);
      bFlushinprogress = false;
    }
    else if ((param1 == OMX_CORE_OUTPUT_PORT_INDEX) ||
	     (param1 == OMX_ALL))
    {
      bFlushinprogress = true;
      execute_omx_flush(OMX_CORE_OUTPUT_PORT_INDEX, true);
      bFlushinprogress = false;
    } else
    {
      DEBUG_PRINT_ERROR("Flush wrong port ID");
      eRet = OMX_ErrorBadParameter;
    }

    /* post_event(OMX_CommandFlush,OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT); */
    bFlag = 0;
  }
  else if (cmd == OMX_CommandPortDisable) {

    if (param1 == OMX_CORE_INPUT_PORT_INDEX) {
      pcm_input = 0;    /* enable tunnel mode */
      DEBUG_PRINT(" Enabling Tunnel mode \n");
      post_event(OMX_CommandPortDisable,
                 OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_EVENT);
    }
    else
    {
      DEBUG_PRINT_ERROR("disable wrong port ID");
    }
    bFlag = 0;
  }
  else
  {
    DEBUG_PRINT_ERROR("Error: Invalid Command received other than StateSet (%d)\n",cmd);
    eRet = OMX_ErrorNotImplemented;
  }
  if(eRet == OMX_ErrorNone && bFlag)
  {
    post_event(cmd,eState,OMX_COMPONENT_GENERATE_EVENT);
  }
  return eRet;
}

/*=============================================================================
FUNCTION:
execute_omx_flush

DESCRIPTION:
Function that flushes buffers that are pending to be written to driver

INPUT/OUTPUT PARAMETERS:
[IN] param1
[IN] cmd_cmpl

RETURN VALUE:
true
false

Dependency:
None

SIDE EFFECTS:
None
=============================================================================*/
bool omx_aac_aenc::execute_omx_flush(OMX_IN OMX_U32 param1, bool cmd_cmpl)
{
    bool bRet = true;

    DEBUG_PRINT("Execute_omx_flush Port[%lu]", param1);
    struct timespec abs_timeout;
    abs_timeout.tv_sec = 1;
    abs_timeout.tv_nsec = 0;
    if ( (signed)param1 == -1 )
    {
        DEBUG_PRINT("Execute flush for both I/p and O/p ports\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 2;
        pthread_mutex_unlock(&m_flush_lock);

        // Send Flush commands to input and output threads
        post_input(OMX_CommandFlush,
            OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        post_output(OMX_CommandFlush,
            OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        // Send Flush to the kernel so that the in and out buffers are released
        if ( ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1 )
        DEBUG_PRINT("FLush:ioctl flush failed errno=%d\n",errno);

        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%lu",param1);

        while ( 1 )
        {
            pthread_mutex_lock(&out_buf_count_lock);
            pthread_mutex_lock(&in_buf_count_lock);
            DEBUG_PRINT("Flush:nNumOutputBuf = %d nNumInputBuf=%d\n",\
                m_num_out_buf,m_num_in_buf);
            if ( m_num_out_buf > 0 || m_num_in_buf > 0 )
            {
                pthread_mutex_unlock(&in_buf_count_lock);
                pthread_mutex_unlock(&out_buf_count_lock);
                DEBUG_PRINT(" READ FLUSH PENDING HENCE WAIT\n");
                DEBUG_PRINT("BEFORE READ ioctl_flush\n");
                usleep (10000);
                if ( ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1 )
                    DEBUG_PRINT("Flush: ioctl flush failed %d\n",\
                    errno);
                DEBUG_PRINT("AFTER READ ioctl_flush\n");
                sem_timedwait(&sem_read_msg,&abs_timeout);
                DEBUG_PRINT("AFTER READ SEM_TIMEWAIT\n");
            } else
            {
                pthread_mutex_unlock(&in_buf_count_lock);
                pthread_mutex_unlock(&out_buf_count_lock);
                break;
            }
        }
        wait_for_event();
        DEBUG_PRINT("RECIEVED BOTH FLUSH ACK's param1=%lu cmd_cmpl=%d",\
            param1,cmd_cmpl);

        // If not going to idle state, Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if ( cmd_cmpl )
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                OMX_CommandFlush, OMX_CORE_INPUT_PORT_INDEX, NULL );
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                OMX_CommandFlush, OMX_CORE_OUTPUT_PORT_INDEX, NULL );
            DEBUG_PRINT("Inside FLUSH.. sending FLUSH CMPL\n");
        }
    } else if ( OMX_CORE_INPUT_PORT_INDEX == param1 )
    {
        DEBUG_PRINT("Execute FLUSH for I/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 1;
        pthread_mutex_unlock(&m_flush_lock);
        post_input(OMX_CommandFlush,
            OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        if ( ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1 )
            DEBUG_PRINT("Flush:Input port, ioctl flush failed %d\n",errno);

        // Send Flush to the kernel so that the in and out buffers are released
        // sleep till the FLUSH ACK are done by both the input and output thrds
        DEBUG_DETAIL("Executing FLUSH for I/p port\n");
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%lu",param1);
        wait_for_event();
        DEBUG_DETAIL(" RECIEVED FLUSH ACK FOR I/P PORT param1=%lu",param1);

        // Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if ( cmd_cmpl )
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                OMX_CommandFlush, OMX_CORE_INPUT_PORT_INDEX, NULL );
        }
    } else if ( OMX_CORE_OUTPUT_PORT_INDEX == param1 )
    {
        DEBUG_PRINT("Executing FLUSH for O/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 1;
        pthread_mutex_unlock(&m_flush_lock);
        DEBUG_DETAIL("Executing FLUSH for O/p port\n");
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%lu",param1);

        post_output(OMX_CommandFlush,
            OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        if ( ioctl( m_drv_fd, AUDIO_FLUSH, 0) ==-1 )
            DEBUG_PRINT("Flush:Output port, ioctl flush failed %d\n",errno);

        //  pthread_mutex_unlock(&m_out_th_lock_1);
        // sleep till the FLUSH ACK are done by both the input and output thrds
        wait_for_event();
        // Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if ( cmd_cmpl )
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                OMX_CommandFlush, OMX_CORE_OUTPUT_PORT_INDEX, NULL );
        }
        DEBUG_DETAIL("RECIEVED FLUSH ACK FOR O/P PORT param1=%lu",param1);
    } else
    {
        DEBUG_PRINT("Invalid Port ID[%lu]",param1);

    }
    return bRet;
}
/*=============================================================================
FUNCTION:
execute_input_omx_flush

DESCRIPTION:
Function that flushes buffers that are pending to be written to driver

INPUT/OUTPUT PARAMETERS:
None

RETURN VALUE:
true
false

Dependency:
None

SIDE EFFECTS:
None
=============================================================================*/
bool omx_aac_aenc::execute_input_omx_flush(void)
{
    OMX_BUFFERHEADERTYPE *omx_buf;
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned      ident;
    unsigned      qsize=0; // qsize
    unsigned      ebd_qsize=0; // qsize
    unsigned      tot_qsize=0; // qsize

    DEBUG_PRINT("Execute_omx_flush on input port");

    do
    {
	pthread_mutex_lock(&m_inputlock);
        qsize = m_input_q.m_size;
	ebd_qsize = m_input_ctrl_ebd_q.m_size;
        tot_qsize = qsize;
        tot_qsize += m_input_ctrl_ebd_q.m_size;

        DEBUG_DETAIL("Input FLUSH-->flushq[%d] ebd[%d]dataq[%d]",\
            m_input_ctrl_cmd_q.m_size,
            m_input_ctrl_ebd_q.m_size,qsize);
	pthread_mutex_unlock(&m_inputlock);
        if ( !tot_qsize )
        {
            DEBUG_DETAIL("Input-->BREAKING FROM execute_input_flush LOOP");
            break;
        }
        if ( qsize )
        {
	    pthread_mutex_lock(&m_inputlock);
            m_input_q.delete_entry(&p1, &p2, &ident);
	    pthread_mutex_unlock(&m_inputlock);
            if ( (ident == OMX_COMPONENT_GENERATE_ETB) ||
                (ident == OMX_COMPONENT_GENERATE_BUFFER_DONE) )
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Flush:Input dataq=0x%p \n", omx_buf);
                omx_buf->nFilledLen = 0;
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
            }
        } else if ( ebd_qsize )
        {
	    pthread_mutex_lock(&m_inputlock);
            m_input_ctrl_ebd_q.delete_entry(&p1, &p2, &ident);
	    pthread_mutex_unlock(&m_inputlock);
            if ( ident == OMX_COMPONENT_GENERATE_BUFFER_DONE )
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                omx_buf->nFilledLen = 0;
                DEBUG_DETAIL("Flush:ctrl dataq=0x%p \n", omx_buf);
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
            }
        }
    }while ( tot_qsize>0 );
    DEBUG_DETAIL("*************************\n");
    DEBUG_DETAIL("IN-->FLUSHING DONE\n");
    DEBUG_DETAIL("*************************\n");
    flush_ack();
    return true;
}

/*=============================================================================
FUNCTION:
execute_output_omx_flush

DESCRIPTION:
Function that flushes buffers that are pending to be written to driver

INPUT/OUTPUT PARAMETERS:
None

RETURN VALUE:
true
false

Dependency:
None

SIDE EFFECTS:
None
=============================================================================*/
bool omx_aac_aenc::execute_output_omx_flush(void)
{
    OMX_BUFFERHEADERTYPE *omx_buf;
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned      ident;
    unsigned      qsize=0; // qsize
    unsigned      fbd_qsize=0; // qsize
    unsigned      tot_qsize=0; // qsize

    DEBUG_PRINT("Execute_omx_flush on output port");

    do
    {
	pthread_mutex_lock(&m_outputlock);
        qsize = m_output_q.m_size;
	fbd_qsize =  m_output_ctrl_fbd_q.m_size;
        DEBUG_DETAIL("OUT FLUSH-->flushq[%d] fbd[%d]dataq[%d]",\
            m_output_ctrl_cmd_q.m_size,
            m_output_ctrl_fbd_q.m_size,qsize);
        tot_qsize = qsize;
        tot_qsize += m_output_ctrl_fbd_q.m_size;
	pthread_mutex_unlock(&m_outputlock);
        if ( !tot_qsize )
        {
            DEBUG_DETAIL("OUT-->BREAKING FROM execute_input_flush LOOP");
            break;
        }
        if ( qsize )
        {
            pthread_mutex_lock(&m_outputlock);
            m_output_q.delete_entry(&p1,&p2,&ident);
            pthread_mutex_unlock(&m_outputlock);
            if ( (ident == OMX_COMPONENT_GENERATE_FTB) ||
                (ident == OMX_COMPONENT_GENERATE_FRAME_DONE) )
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Ouput Buf_Addr=%p TS[%u] \n", \
                    omx_buf,nTimestamp);
                omx_buf->nTimeStamp = nTimestamp;
                omx_buf->nFilledLen = 0;
                frame_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                DEBUG_DETAIL("CALLING FBD FROM FLUSH");
            }
        } else if ( fbd_qsize )
        {
            pthread_mutex_lock(&m_outputlock);
            m_output_ctrl_fbd_q.delete_entry(&p1, &p2, &ident);
            pthread_mutex_unlock(&m_outputlock);
            if ( ident == OMX_COMPONENT_GENERATE_FRAME_DONE )
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Ouput Buf_Addr=%p TS[%u] \n", \
                    omx_buf,nTimestamp);
                omx_buf->nTimeStamp = nTimestamp;
                omx_buf->nFilledLen = 0;
                frame_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                DEBUG_DETAIL("CALLING FROM CTRL-FBDQ FROM FLUSH");
            }
        }
    }while ( qsize>0 );
    DEBUG_DETAIL("*************************\n");
    DEBUG_DETAIL("OUT-->FLUSHING DONE\n");
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
bool omx_aac_aenc::post_event(unsigned int p1,
                              unsigned int p2,
                              unsigned int id) 
{
  bool bRet      =                      false;

    pthread_mutex_lock(&m_lock);

  if (id == OMX_COMPONENT_GENERATE_ETB) {
    m_data_q.insert_entry(p1,p2,id);
  } else {
    m_cmd_q.insert_entry(p1,p2,id);
  }

  if(m_cmd_svr)
  {
    bRet = true;
    aenc_svr_post_msg(m_cmd_svr, id);
  }

  DEBUG_PRINT("Postcommand-->state[%d]id[%d]q[%d]\n",\
                                                    m_state,
                                                    id,
                                                    m_cmd_q.m_size);
    pthread_mutex_unlock(&m_lock);

  return bRet;
}


bool omx_aac_aenc::post_output(unsigned int p1,
                              unsigned int p2,
                              unsigned int id)
{
	bool bRet = false;
	pthread_mutex_lock(&m_outputlock);

	if((OMX_COMPONENT_GENERATE_COMMAND == id) ||
		(id == OMX_COMPONENT_SUSPEND) ||
		(id == OMX_COMPONENT_GENERATE_EOS))
	{
		m_output_ctrl_cmd_q.insert_entry(p1,p2,id);
	}
	else if ( (OMX_COMPONENT_GENERATE_FRAME_DONE == id) )
	{
		m_output_ctrl_fbd_q.insert_entry(p1,p2,id);
	}
	else
	{
		m_output_q.insert_entry(p1,p2,id);
	}

	if ( m_cmd_cln )
	{
		bRet = true;
		aenc_output_post_msg( m_cmd_cln, id );
	}

	DEBUG_DETAIL("PostInput-->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d] \n",\
		 m_state,
		 id,
		 m_output_ctrl_cmd_q.m_size,
		 m_output_ctrl_fbd_q.m_size,
		 m_output_q.m_size);

	pthread_mutex_unlock(&m_outputlock);
	return bRet;
}

bool omx_aac_aenc::post_input ( unsigned int p1,
				unsigned int p2,
				unsigned int id)
{

	bool bRet = false;
	pthread_mutex_lock(&m_inputlock);

	if((OMX_COMPONENT_GENERATE_COMMAND == id) ||
		(id == OMX_COMPONENT_SUSPEND) ||
		(id == OMX_COMPONENT_GENERATE_EOS))
	{
		m_input_ctrl_cmd_q.insert_entry(p1,p2,id);
	}
	else if ( (OMX_COMPONENT_GENERATE_BUFFER_DONE == id) )
	{
		m_input_ctrl_ebd_q.insert_entry(p1,p2,id);
	}
	else
	{
		m_input_q.insert_entry(p1,p2,id);
	}

	if ( m_cmd_cln_input )
	{
		bRet = true;
		aenc_input_post_msg( m_cmd_cln_input, id );
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

/**
  @brief member function that return parameters to IL client

  @param hComp handle to component instance
  @param paramIndex Parameter type
  @param paramData pointer to memory space which would hold the
        paramter
  @return error status
*/
OMX_ERRORTYPE  omx_aac_aenc::get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                           OMX_IN OMX_INDEXTYPE paramIndex,
                                           OMX_INOUT OMX_PTR     paramData)
{
	OMX_ERRORTYPE eRet = OMX_ErrorNone;

	(void)hComp;
	switch(paramIndex)
	{
		case OMX_IndexParamPortDefinition:
		{
			OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
			portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

			DEBUG_PRINT("OMX_IndexParamPortDefinition portDefn->nPortIndex = %lu\n",portDefn->nPortIndex);

			portDefn->nVersion.nVersion = OMX_SPEC_VERSION;
			portDefn->nSize = sizeof(portDefn);
			portDefn->bEnabled   = OMX_TRUE;
			portDefn->bPopulated = OMX_TRUE;
			portDefn->eDomain    = OMX_PortDomainAudio;

			if (0 == portDefn->nPortIndex)
			{
				portDefn->eDir =  OMX_DirInput;
				if (m_input_buf_hdrs.size() >= (signed)OMX_CORE_NUM_INPUT_BUFFERS) {
					portDefn->bPopulated = OMX_TRUE;
				} else {
					portDefn->bPopulated = OMX_FALSE;
				}
				/* What if the component does not restrict how many buffer to take */
				portDefn->nBufferCountActual = OMX_CORE_NUM_INPUT_BUFFERS;
				portDefn->nBufferCountMin    = OMX_CORE_NUM_INPUT_BUFFERS;
                                portDefn->nBufferSize        = input_buffer_size;
				portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
				if (portDefn->format.audio.cMIMEType != NULL)
				{
					memcpy(portDefn->format.audio.cMIMEType, "audio/mpeg", sizeof("audio/mpeg"));
				}
				DEBUG_PRINT("OMX_IndexParamPortDefinition input port setting OMX_AUDIO_CodingPCM\n");
				portDefn->format.audio.eEncoding = OMX_AUDIO_CodingPCM;
				portDefn->format.audio.pNativeRender = 0;
			}
			else if (1 == portDefn->nPortIndex)
			{
				portDefn->eDir =  OMX_DirOutput;
				portDefn->nBufferCountActual = OMX_CORE_NUM_OUTPUT_BUFFERS;
				portDefn->nBufferCountMin    = OMX_CORE_NUM_OUTPUT_BUFFERS;
                                portDefn->nBufferSize        = output_buffer_size;
				portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
				portDefn->format.audio.eEncoding = OMX_AUDIO_CodingAAC;
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
			portParamType->nPorts           = NUM_PORTS;
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

			if (OMX_CORE_OUTPUT_PORT_INDEX== portFormatType->nPortIndex)
			{
				DEBUG_PRINT("get_parameter: OMX_IndexParamAudioFormat: "\
					    "%lu\n", portFormatType->nIndex);
				portFormatType->eEncoding = OMX_AUDIO_CodingAAC;
			}
			else if (OMX_CORE_INPUT_PORT_INDEX == portFormatType->nPortIndex)
			{
				DEBUG_PRINT("OMX_IndexParamAudioPortFormat 11 ");
				if (0 == portFormatType->nIndex)
				{ /* What is the intention of nIndex */
					DEBUG_PRINT("YM: OMX_IndexParamAudioPortFormat 12 ");
					portFormatType->eEncoding = OMX_AUDIO_CodingPCM;
				}
			} else {
				DEBUG_PRINT("YM: OMX_IndexParamAudioPortFormat ErrorBadPortIndex ");
				eRet = OMX_ErrorBadPortIndex;
			}
			break;
		}
		case OMX_IndexParamAudioAac:
		{
			OMX_AUDIO_PARAM_AACPROFILETYPE *aacParam = (OMX_AUDIO_PARAM_AACPROFILETYPE *) paramData;
			DEBUG_PRINT("OMX_IndexParamAudioAac\n");
			*aacParam = m_aenc_param;
			break;
		}
		case OMX_IndexParamAudioPcm:
		{
			OMX_AUDIO_PARAM_PCMMODETYPE *pcmParam = (OMX_AUDIO_PARAM_PCMMODETYPE *) paramData;
			DEBUG_PRINT("OMX_IndexParamAudioPcm\n");
			*pcmParam = m_pcm_param;
			break;
		}
		case QOMX_IndexParamAudioSessionId:
		{
			QOMX_AUDIO_STREAM_INFO_DATA *streaminfoparam =
				(QOMX_AUDIO_STREAM_INFO_DATA *) paramData;
			streaminfoparam->sessionId = m_session_id;
			break;
		}
		default:
		{
			DEBUG_PRINT_ERROR("unknown param %08x\n", paramIndex);
			eRet = OMX_ErrorBadParameter;
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
OMX_ERRORTYPE  omx_aac_aenc::set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                           OMX_IN OMX_INDEXTYPE paramIndex,
                                           OMX_IN OMX_PTR        paramData)
{
	OMX_ERRORTYPE eRet = OMX_ErrorNone;
	unsigned int loop=0;

	(void)hComp;
	switch(paramIndex)
	{

		case OMX_IndexParamAudioAac:
		{
			DEBUG_PRINT("OMX_IndexParamAudioAAC");
			memcpy(&m_aenc_param,paramData,sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
			DEBUG_PRINT("channels %lu samplerate %lu bitrate %lu\n"\
					,m_aenc_param.nChannels,m_aenc_param.nSampleRate, m_aenc_param.nBitRate);
			//TODO-assuming PCM config comes first
			if(m_pcm_param.nSamplingRate != m_aenc_param.nSampleRate)
			{
				DEBUG_PRINT_ERROR("Input and Output Sampling rates are not matching\n");
				m_cb.EventHandler(&m_cmp, m_app_data,OMX_EventError,
							OMX_ErrorBadParameter,0, NULL );
			}
			for (loop=0; loop< sizeof(sample_idx_tbl) / \
					 sizeof(struct sample_rate_idx); \
					 loop++)
			{
				if(sample_idx_tbl[loop].sample_rate == m_aenc_param.nSampleRate)
				{
					sample_idx  = sample_idx_tbl[loop].sample_rate_idx;
				}
			}
			break;
		}
		case OMX_IndexParamAudioPcm:
		{
			DEBUG_PRINT("OMX_IndexParamAudioPcm \n");
			memcpy(&m_pcm_param, paramData, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
			break;
		}
		case  OMX_IndexParamAudioPortFormat:
		{

			OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
			(OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
			DEBUG_PRINT("set_parameter: OMX_IndexParamAudioPortFormat\n");

			if (OMX_CORE_INPUT_PORT_INDEX== portFormatType->nPortIndex)
			{
				portFormatType->eEncoding = OMX_AUDIO_CodingPCM;
			} else if (OMX_CORE_OUTPUT_PORT_INDEX == portFormatType->nPortIndex)
			{
				DEBUG_PRINT("set_parameter: OMX_IndexParamAudioFormat:"\
						" %lu\n", portFormatType->nIndex);
				portFormatType->eEncoding = OMX_AUDIO_CodingAAC;
			} else
			{
				DEBUG_PRINT_ERROR("set_parameter: Bad port index %d\n", \
				(int)portFormatType->nPortIndex);
				eRet = OMX_ErrorBadPortIndex;
			}
			break;
		}
		case OMX_IndexParamPortDefinition:
		{
			DEBUG_PRINT("YM: OMX_IndexParamPortDefinition entry ");
			OMX_PARAM_PORTDEFINITIONTYPE  *tmp_param = (OMX_PARAM_PORTDEFINITIONTYPE*)paramData;
			if(tmp_param->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
			{
				DEBUG_PRINT("YM: OMX_IndexParamPortDefinition INPUT PORT");
				memcpy((char *)&m_input_param, (char *)tmp_param, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
			} else if (tmp_param->nPortIndex == OMX_CORE_OUTPUT_PORT_INDEX)
			{
				DEBUG_PRINT("YM: OMX_IndexParamPortDefinition OUTPUT PORT");
				memcpy((char *)&m_output_param, (char *)tmp_param, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
			} else
			{
				DEBUG_PRINT("Set param bad port index %d \n", \
						(int)tmp_param->nPortIndex);
				eRet = OMX_ErrorBadPortIndex;
			}
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
  omx_aac_aenc::GetConfig

DESCRIPTION
  OMX Get Config Method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::get_config(OMX_IN OMX_HANDLETYPE      hComp,
                                        OMX_IN OMX_INDEXTYPE configIndex,
                                        OMX_INOUT OMX_PTR     configData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  (void)hComp;
  switch(configIndex)
  {
  case OMX_IndexConfigAudioVolume:
    {
      OMX_AUDIO_CONFIG_VOLUMETYPE *volume = (OMX_AUDIO_CONFIG_VOLUMETYPE*) configData;

      if (volume->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
      {
        volume->nSize = sizeof(volume);
        volume->nVersion.nVersion = OMX_SPEC_VERSION;
        volume->bLinear = OMX_TRUE;
        volume->sVolume.nValue = m_volume;
        volume->sVolume.nMax   = OMX_AENC_MAX;
        volume->sVolume.nMin   = OMX_AENC_MIN;
      } else {
        eRet = OMX_ErrorBadPortIndex;
      }
    }
    break;

  case OMX_IndexConfigAudioMute:
    {
      OMX_AUDIO_CONFIG_MUTETYPE *mute = (OMX_AUDIO_CONFIG_MUTETYPE*) configData;

      if (mute->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
      {
        mute->nSize = sizeof(mute);
        mute->nVersion.nVersion = OMX_SPEC_VERSION;
        mute->bMute = (BITMASK_PRESENT(&m_flags, OMX_COMPONENT_MUTED)?OMX_TRUE:OMX_FALSE);
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
  omx_aac_aenc::SetConfig

DESCRIPTION
  OMX Set Config method implementation

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.
========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::set_config(OMX_IN OMX_HANDLETYPE      hComp,
                                        OMX_IN OMX_INDEXTYPE configIndex,
                                        OMX_IN OMX_PTR        configData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  (void)hComp;
  switch(configIndex)
  {
    case OMX_IndexConfigAudioVolume:
    {
      OMX_AUDIO_CONFIG_VOLUMETYPE *vol = (OMX_AUDIO_CONFIG_VOLUMETYPE*)
                                          configData;
      if (vol->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
      {
        if ((vol->sVolume.nValue <= OMX_AENC_MAX) &&
            (vol->sVolume.nValue >= OMX_AENC_MIN)) {
          m_volume = vol->sVolume.nValue;
          if (BITMASK_ABSENT(&m_flags, OMX_COMPONENT_MUTED))
          {
            /* ioctl(m_drv_fd, AUDIO_VOLUME, m_volume * OMX_AENC_VOLUME_STEP); */
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
          /* ioctl(m_drv_fd, AUDIO_VOLUME, m_volume * OMX_AENC_VOLUME_STEP); */
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
  omx_aac_aenc::GetExtensionIndex

DESCRIPTION
  OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::get_extension_index(OMX_IN OMX_HANDLETYPE      hComp,
                                                OMX_IN OMX_STRING      paramName,
                                                OMX_OUT OMX_INDEXTYPE* indexType)
{
    if((hComp == NULL) || (paramName == NULL) || (indexType == NULL))
    {
	DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
	return OMX_ErrorBadParameter;
    }
    if ( m_state == OMX_StateInvalid )
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
  omx_aac_aenc::GetState

DESCRIPTION
  Returns the state information back to the caller.<TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::get_state(OMX_IN OMX_HANDLETYPE  hComp,
                                       OMX_OUT OMX_STATETYPE* state)
{
  (void)hComp;
  *state = m_state;
  //DEBUG_PRINT("Returning the state %d\n",*state);
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::ComponentTunnelRequest

DESCRIPTION
  OMX Component Tunnel Request method implementation. <TBD>

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::component_tunnel_request(OMX_IN OMX_HANDLETYPE                hComp,
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

/* ======================================================================
FUNCTION
  omx_aac_aenc::UseBuffer

DESCRIPTION
  OMX Use Buffer method implementation. <TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None , if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::use_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                        OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                        OMX_IN OMX_U32                        port,
                                        OMX_IN OMX_PTR                     appData,
                                        OMX_IN OMX_U32                       bytes,
                                        OMX_IN OMX_U8*                      buffer)
{

  (void)hComp;
  (void)bufferHdr;
  (void)port;
  (void)appData;
  (void)bytes;
  (void)buffer;
  DEBUG_PRINT_ERROR("Error: use_buffer Not implemented\n");
  return OMX_ErrorNotImplemented;
}


/* ======================================================================
FUNCTION
  omx_aac_aenc::AllocateInputBuffer

DESCRIPTION
  Helper function for allocate buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::allocate_input_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                                  OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                                  OMX_IN OMX_U32                        port,
                                                  OMX_IN OMX_PTR                     appData,
                                                  OMX_IN OMX_U32                       bytes)
{
  OMX_ERRORTYPE              eRet = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE       *bufHdr; // buffer header
  unsigned                   nBufSize = max(bytes, OMX_CORE_INPUT_BUFFER_SIZE);
  char                       *buf_ptr;

  (void)hComp;
  (void)port;
  buf_ptr = (char *) calloc( (nBufSize + sizeof(OMX_BUFFERHEADERTYPE) ) , 1);

  if (buf_ptr != NULL) {
    bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
    *bufferHdr = bufHdr;
    memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));


    bufHdr->pBuffer           = (OMX_U8 *)((buf_ptr) +
                                           sizeof(OMX_BUFFERHEADERTYPE));
    DEBUG_PRINT("bufHdr %p bufHdr->pBuffer %p", bufHdr, bufHdr->pBuffer);
    bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
    bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
    bufHdr->nAllocLen         = nBufSize;
    bufHdr->pAppPrivate       = appData;
    bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
    m_input_buf_hdrs.insert(bufHdr, NULL);
    m_inp_buf_count++;
  } else {
    DEBUG_PRINT("Input buffer memory allocation failed\n");
    eRet =  OMX_ErrorInsufficientResources;
  }

  return eRet;
}

OMX_ERRORTYPE  omx_aac_aenc::allocate_output_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                                  OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                                  OMX_IN OMX_U32                        port,
                                                  OMX_IN OMX_PTR                     appData,
                                                  OMX_IN OMX_U32                       bytes)
{
  OMX_ERRORTYPE         eRet = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE            *bufHdr; // buffer header
  char                       *buf_ptr;
  unsigned                   nBufSize = 0;

  (void)hComp;
  (void)port;
  if(pcm_input)
  {
	nBufSize = max(bytes, OMX_AAC_OUTPUT_BUFFER_SIZE+AUDAAC_MAX_ADIF_HEADER_LENGTH);
  }else
  {
	//nBufSize = max(bytes, OMX_AAC_OUTPUT_BUFFER_SIZE+AUDAAC_MAX_ADTS_HEADER_LENGTH);
	nBufSize = max(bytes, OMX_AAC_OUTPUT_BUFFER_SIZE);
  }

  buf_ptr = (char *) calloc( (nBufSize + sizeof(OMX_BUFFERHEADERTYPE) ) , 1);

  if (buf_ptr != NULL) {
    bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
    *bufferHdr = bufHdr;
    memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

    bufHdr->pBuffer           = (OMX_U8 *)((buf_ptr) +
                                           sizeof(OMX_BUFFERHEADERTYPE));
    DEBUG_PRINT("bufHdr %p bufHdr->pBuffer %p", bufHdr, bufHdr->pBuffer);
    bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
    bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
    bufHdr->nAllocLen         = nBufSize;
    bufHdr->pAppPrivate       = appData;
    bufHdr->nInputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX;
    m_output_buf_hdrs.insert(bufHdr, NULL);
    m_out_buf_count++;
  } else {
    DEBUG_PRINT("Output buffer memory allocation failed\n");
    eRet =  OMX_ErrorInsufficientResources;
  }

  return eRet;
}


// AllocateBuffer  -- API Call
/* ======================================================================
FUNCTION
  omx_aac_aenc::AllocateBuffer

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::allocate_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                     OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                     OMX_IN OMX_U32                        port,
                                     OMX_IN OMX_PTR                     appData,
                                     OMX_IN OMX_U32                       bytes)
{

    OMX_ERRORTYPE eRet = OMX_ErrorNone; // OMX return type


    // What if the client calls again.
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

    if((eRet == OMX_ErrorNone) && (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)))
    {
      DEBUG_PRINT("Checking for Output Allocate buffer Done");
      if(allocate_done())
      {
	m_is_alloc_buf++;
        // Send the callback now
        BITMASK_CLEAR(&m_flags, OMX_COMPONENT_IDLE_PENDING);
        post_event(OMX_CommandStateSet,OMX_StateIdle,
                   OMX_COMPONENT_GENERATE_EVENT);
      }
    }
    DEBUG_PRINT("Allocate Buffer exit with ret Code %d\n", eRet);
    return eRet;
}

/**
 @brief member function that searches for caller buffer

 @param buffer pointer to buffer header
 @return bool value indicating whether buffer is found
 */
bool omx_aac_aenc::search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{
  bool eRet = false;
  OMX_BUFFERHEADERTYPE *temp;

  //access only in IL client context
  temp = m_input_buf_hdrs.find_ele(buffer);
  if(buffer && temp)
  {
      DEBUG_PRINT("found input buf hdr %p \n", buffer);
      eRet = true;
  }

  return eRet;
}

/**
 @brief member function that searches for caller buffer

 @param buffer pointer to buffer header
 @return bool value indicating whether buffer is found
 */
bool omx_aac_aenc::search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{
  bool eRet = false;
  OMX_BUFFERHEADERTYPE *temp;


  //access only in IL client context
  temp = m_output_buf_hdrs.find_ele(buffer);
  if(buffer && temp)
  {
      DEBUG_PRINT("found output buf hdr %p \n", buffer);
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
OMX_ERRORTYPE  omx_aac_aenc::free_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                      OMX_IN OMX_U32                 port,
                                      OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
	OMX_ERRORTYPE eRet = OMX_ErrorNone;

	DEBUG_PRINT("Free buf %p port=%lu\n", buffer,port);

	(void)hComp;
	if(port == OMX_CORE_INPUT_PORT_INDEX)
	{
	       if(search_input_bufhdr(buffer) == true)
	       {
		       /* Buffer exist */
		       //access only in IL client context
		       m_input_buf_hdrs.erase(buffer);
		       free(buffer);
		       m_inp_buf_count--;
	       } else {
		       DEBUG_PRINT_ERROR("Error: free_buffer , invalid input buffer header\n");
		       eRet = OMX_ErrorBadParameter;
	       }
	} else if(port == OMX_CORE_OUTPUT_PORT_INDEX)
	{
	       if(search_output_bufhdr(buffer) == true)
	       {
		       /* Buffer exist */
		       //access only in IL client context
		       m_output_buf_hdrs.erase(buffer);
		       free(buffer);
		       m_out_buf_count--;
	       } else {
		       DEBUG_PRINT_ERROR("Error: free_buffer , invalid Output buffer header\n");
		       eRet = OMX_ErrorBadParameter;
	       }
	}
	else
	{
	       eRet = OMX_ErrorBadPortIndex;
	}

	if((eRet == OMX_ErrorNone) &&
	       (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
	{
	       if(release_done())
	       {
		       // Send the callback now
		       BITMASK_CLEAR((&m_flags),OMX_COMPONENT_LOADING_PENDING);
		       post_event(OMX_CommandStateSet,
				OMX_StateLoaded,OMX_COMPONENT_GENERATE_EVENT);
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
OMX_ERRORTYPE  omx_aac_aenc::empty_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                              OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
	OMX_ERRORTYPE eRet = OMX_ErrorNone;

	DEBUG_PRINT("Inside empty_this_buffer \n");
	pthread_mutex_lock(&m_lock);
	if ((m_state != OMX_StateExecuting) &&
		(m_state != OMX_StatePause))
	{
		DEBUG_PRINT_ERROR("Invalid state\n");
		eRet = OMX_ErrorInvalidState;
	}
	pthread_mutex_unlock(&m_lock);

	if (eRet == OMX_ErrorNone) {
		if (search_input_bufhdr(buffer) == true) 
		{
			DEBUG_PRINT("Posting OMX_COMPONENT_GENERATE_ETB to input thread \n");
    			pthread_mutex_lock(&in_buf_count_lock);
			++m_num_in_buf;
    			pthread_mutex_unlock(&in_buf_count_lock);
			DEBUG_PRINT("In empty_this_buffer m_num_in_buf %d \n",m_num_in_buf);
			post_input((unsigned)hComp,
				 (unsigned) buffer,OMX_COMPONENT_GENERATE_ETB);
		} else {
			DEBUG_PRINT_ERROR("Bad header %p \n", buffer);
			eRet = OMX_ErrorBadParameter;
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
OMX_ERRORTYPE  omx_aac_aenc::empty_this_buffer_proxy(OMX_IN OMX_HANDLETYPE         hComp,
                                                     OMX_BUFFERHEADERTYPE* buffer)
{
	META_IN meta_in;
	OMX_U8 *data = NULL;
	OMX_U8 *tmp_buf = NULL;
	int ret = 0;
	OMX_TICKS tmp1 = 0;

	DEBUG_PRINT("Inside empty_this_buffer_proxy \n");

	if(bFlushinprogress)
	{
		DEBUG_PRINT("FTBP: flush in progress, return buffer\n");
		buffer_done_cb(buffer);
		return OMX_ErrorNone;
	}

	tmp_buf = buffer->pBuffer;
	if(buffer->nFlags & 0x01)
	{
		DEBUG_PRINT("EOS Ouccured %lu\n",buffer->nFilledLen);
		post_input((unsigned)hComp,(unsigned) buffer,
		        OMX_COMPONENT_GENERATE_EOS);
	}
	data = m_tmp_in_meta_buf;
	DEBUG_PRINT("NT mode: copying meta_in\n");
	meta_in.offsetVal = sizeof(META_IN);
	tmp1 |= (buffer->nTimeStamp & 0xFFFFFFFF00000000LL) >> 32;
	tmp1 |= (buffer->nTimeStamp & 0x00000000FFFFFFFFLL) << 32;
	meta_in.nTimeStamp = tmp1;
	meta_in.nFlags = buffer->nFlags;
	meta_in.error_flag = 0;
	meta_in.samplingFreq = m_pcm_param.nSamplingRate;
	meta_in.channels = m_pcm_param.nChannels;
	meta_in.tickCount = tickcount++;
	memcpy(data,&meta_in, meta_in.offsetVal);
	memcpy(&data[sizeof(META_IN)], tmp_buf,buffer->nFilledLen);
	DEBUG_PRINT("Requesting to write %lu bytes\n",(buffer->nFilledLen+sizeof(META_IN)));
	ret = write(m_drv_fd, data, buffer->nFilledLen+sizeof(META_IN) );
	if (ret != (ssize_t)(buffer->nFilledLen + sizeof(META_IN))) {
		DEBUG_PRINT("failed to write, driver returned with = %d\n", ret);
	}
	DEBUG_PRINT("came out of write and calling buffer done %d\n",ret);
	DEBUG_PRINT("calling bufferdone m_num_in_buf = %d\n", m_num_in_buf);
	buffer_done_cb(buffer);
	return OMX_ErrorNone;
}

void   omx_aac_aenc::audaac_rec_install_bits(OMX_U8 *input,
                              OMX_U8 num_bits_reqd,
                              OMX_U32  value,
                              OMX_U16 *hdr_bit_index)
{
  OMX_U32 byte_index;
  OMX_U8   bit_index;
  OMX_U8   bits_avail_in_byte;
  OMX_U8   num_to_copy;
  OMX_U8   byte_to_copy;

  OMX_U8   num_remaining = num_bits_reqd;
  OMX_U8  bit_mask;

  bit_mask = 0xFF;

  while (num_remaining) {

    byte_index = (*hdr_bit_index) >> 3;
    bit_index  = (*hdr_bit_index) &  0x07;

    bits_avail_in_byte = 8 - bit_index;

    num_to_copy = min(bits_avail_in_byte, num_remaining);

    byte_to_copy = ((OMX_U8)((value >> (num_remaining - num_to_copy)) & 0xFF) <<
                    (bits_avail_in_byte - num_to_copy));

    input[byte_index] &= ((OMX_U8)(bit_mask << bits_avail_in_byte));
    input[byte_index] |= byte_to_copy;

    *hdr_bit_index += num_to_copy;

    num_remaining -= num_to_copy;
  }
}


void  omx_aac_aenc::audaac_rec_install_mp4ff_header_variable (OMX_U16  byte_num,
                                                        OMX_U32 sample_index,
                                                        OMX_U8 channel_config,
							OMX_U8 *audaac_header_mp4ff)
{
	OMX_U16 audaac_hdr_bit_index;
	audaac_header_mp4ff[0] = 0;
	audaac_header_mp4ff[1] = 0;
        audaac_hdr_bit_index = 0;

	(void)byte_num;

        /* Audio object type, 5 bit */
        audaac_rec_install_bits(audaac_header_mp4ff,
                          AUDAAC_MP4FF_OBJ_TYPE,
                          2,
                          &(audaac_hdr_bit_index));

        /* Frequency index, 4 bit */
        audaac_rec_install_bits(audaac_header_mp4ff,
                          AUDAAC_MP4FF_FREQ_IDX,
                          (OMX_U32)sample_index,
                          &(audaac_hdr_bit_index));

        /* Channel config filed, 4 bit */
        audaac_rec_install_bits(audaac_header_mp4ff,
                          AUDAAC_MP4FF_CH_CONFIG,
                          channel_config,
                          &(audaac_hdr_bit_index));

}


void  omx_aac_aenc::audaac_rec_install_adif_header_variable (OMX_U16  byte_num,
							OMX_U32 sample_index,
							OMX_U8 channel_config,
							OMX_U8 *audaac_header_adif)
{
	OMX_U32 value;
	OMX_U16 audaac_hdr_bit_index;
	OMX_U8  num_pfe, num_fce, num_sce, num_bce;
	OMX_U8  num_lfe, num_ade, num_vce, num_com;
	OMX_U8  i;
	OMX_U8  buf8;
	OMX_U32 dummy;
	OMX_BOOL variable_bit_rate = OMX_FALSE;
	value = 0;

	(void)byte_num;
	(void)channel_config;

	audaac_header_adif[0] = 65;
	audaac_header_adif[1] = 68;
	audaac_header_adif[2] = 73;
	audaac_header_adif[3] = 70;
	audaac_hdr_bit_index = 32;


	/* Copyright present field 1bit */
	value = 0;
	audaac_rec_install_bits(audaac_header_adif,
			  COPY_RIGHT_PRESENT,
			  0,
			  &(audaac_hdr_bit_index));
	/* It is unreachable code, but kept to make it
	   compliance with specification */
	if(value)
	{
		//Copy right info 72bits
		audaac_rec_install_bits(audaac_header_adif,
				  72,
				  dummy,
				  &(audaac_hdr_bit_index));
	}

	/* ORIGINAL_COPY field, 1 bit */
	audaac_rec_install_bits(audaac_header_adif,
			  ORIGINAL_COPY,
			  0,
			  &(audaac_hdr_bit_index));

	/* HOME field, 1 bit */
	audaac_rec_install_bits(audaac_header_adif,
			  HOME,
			  0,
			  &(audaac_hdr_bit_index));

	/* BITSTREAM_TYPE field, 1 bit */
	audaac_rec_install_bits(audaac_header_adif,
			  BITSTREAM_TYPE,
			  0,
			  &(audaac_hdr_bit_index));

	/* BITRATE, 23 bit */
	audaac_rec_install_bits(audaac_header_adif,
			  BITRATE,
			  m_aenc_param.nBitRate,
			  &(audaac_hdr_bit_index));


	/* NUM_PROGRAM_CONFIG_ELEMENTS 4 bit */
        num_pfe = 0;
	audaac_rec_install_bits(audaac_header_adif,
			  NUM_PROGRAM_CONFIG_ELEMENTS,
			  num_pfe,
			  &(audaac_hdr_bit_index));

         /* It is unreachable code, but kept to make it
                   compliance with specification */
	for(i=0; i<num_pfe+1;i++)
	{
		if(variable_bit_rate == OMX_FALSE)
		{
			/*buffer_fullness 20bit*/
			audaac_rec_install_bits(audaac_header_adif,
					  20,
					  0,
					  &(audaac_hdr_bit_index));
		}

		/*element instance_tag field, 4 bits */
		value = 0;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_ELEMENT_INSTANCE_TAG_SIZE,
				  value,
				  &(audaac_hdr_bit_index));
		/* object_type, 2 bits, AAC LC is supported */
		value = 1;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_PROFILE_SIZE,
				  value,
				  &(audaac_hdr_bit_index));

		/*sampling frequency index 4bits*/
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_SAMPLING_FREQ_INDEX_SIZE,
				  (OMX_U32)sample_index,
				  &(audaac_hdr_bit_index));

		/* num_front_channel_elements, 4 bits */
		num_fce = 1;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_NUM_FRONT_CHANNEL_ELEMENTS_SIZE,
				  num_fce,
				  &(audaac_hdr_bit_index));

		/* num_side_channel_elements, 4 bits */
		num_sce = 0;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_NUM_SIDE_CHANNEL_ELEMENTS_SIZE,
				  num_sce,
				  &(audaac_hdr_bit_index));

		/* num_back_channel_elements, 4 bits */
		num_bce = 0;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_NUM_BACK_CHANNEL_ELEMENTS_SIZE,
				  num_bce,
				  &(audaac_hdr_bit_index));

		/* num_lfe_channel_elements, 2 bits */
		num_lfe = 0;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_NUM_LFE_CHANNEL_ELEMENTS_SIZE,
				  num_lfe,
				  &(audaac_hdr_bit_index));

		/* num_assoc_data_elements, 3 bits */
		num_ade = 0;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_NUM_ASSOC_DATA_ELEMENTS_SIZE,
				  num_ade,
				  &(audaac_hdr_bit_index));

		/* num_valid_cc_elements, 4 bits */
		num_vce = 0;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_NUM_VALID_CC_ELEMENTS_SIZE,
				  num_vce,
				  &(audaac_hdr_bit_index));

		/* mono_mixdown_present, 1 bits */
		dummy = 0;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_MONO_MIXDOWN_PRESENT_SIZE,
				  dummy,
				  &(audaac_hdr_bit_index));
		/* It is unreachable code, but kept to make it
		   compliance with specification */
		if(dummy)
		{
			/* mono_mixdown_element, 4 bits */
			value = 0;
			audaac_rec_install_bits(audaac_header_adif,
					  AAC_MONO_MIXDOWN_ELEMENT_SIZE,
					  value,
					  &(audaac_hdr_bit_index));
		}

		/* stereo_mixdown_present, 1 bits */
		dummy = 0;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_STEREO_MIXDOWN_PRESENT_SIZE,
				  dummy,
				  &(audaac_hdr_bit_index));
		/* It is unreachable code, but kept to make it
		   compliance with specification */
		if(dummy)
		{
			/* stereo_mixdown_element, 4 bits */
			value = 0;
			audaac_rec_install_bits(audaac_header_adif,
					  AAC_STEREO_MIXDOWN_ELEMENT_SIZE,
					  value,
					  &(audaac_hdr_bit_index));
		}

		/* matrix_mixdown_present, 1 bits */
		dummy = 0;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_MATRIX_MIXDOWN_PRESENT_SIZE,
				  dummy,
				  &(audaac_hdr_bit_index));
		/* It is unreachable code, but kept to make it
		   compliance with specification */
		if(dummy)
		{
			/* matrix_mixdown_element, 4 bits */
			value = 0;
			audaac_rec_install_bits(audaac_header_adif,
					  AAC_MATRIX_MIXDOWN_ELEMENT_SIZE,
					  value,
					  &(audaac_hdr_bit_index));
		}

		if(m_aenc_param.nChannels == 2)
			value = 16;
		else
			value = 0;

		for(i=0;i<num_fce;i++)
		{
			audaac_rec_install_bits(audaac_header_adif,
					  AAC_FCE_SIZE,
					  value,
					  &(audaac_hdr_bit_index));
		}
		value = 0;
		for(i=0;i<num_sce;i++)
		{
			audaac_rec_install_bits(audaac_header_adif,
					  AAC_SCE_SIZE,
					  value,
					  &(audaac_hdr_bit_index));
		}
		for(i=0;i<num_bce;i++)
		{
			audaac_rec_install_bits(audaac_header_adif,
					  AAC_BCE_SIZE,
					  value,
					  &(audaac_hdr_bit_index));
		}
		for(i=0;i<num_lfe;i++)
		{
			audaac_rec_install_bits(audaac_header_adif,
					  AAC_LFE_SIZE,
					  value,
					  &(audaac_hdr_bit_index));
		}
		for(i=0;i<num_ade;i++)
		{
			audaac_rec_install_bits(audaac_header_adif,
					  AAC_ADE_SIZE,
					  value,
					  &(audaac_hdr_bit_index));
		}
		for(i=0;i<num_vce;i++)
		{
			audaac_rec_install_bits(audaac_header_adif,
					  AAC_VCE_SIZE,
					  value,
					  &(audaac_hdr_bit_index));
		}

		/*Byte alignment */
		buf8 = (OMX_U8)((audaac_hdr_bit_index) & (0x07));
		if(buf8)
		{
			audaac_rec_install_bits(audaac_header_adif,
					  buf8,
					  value,
					  &(audaac_hdr_bit_index));
		}

		//8-bits comment
		num_com = 0;
		audaac_rec_install_bits(audaac_header_adif,
				  AAC_COMMENT_FIELD_BYTES_SIZE,
				  num_com,
				  &(audaac_hdr_bit_index));
		/* It is unreachable code, but kept to make it
		   compliance with specification */
		for(i=0;i<num_com;i++)
		{
			audaac_rec_install_bits(audaac_header_adif,
					  AAC_COMMENT_FIELD_DATA_SIZE,
					  num_com,
					  &(audaac_hdr_bit_index));
		}
	}
}

void  omx_aac_aenc::audaac_rec_install_adts_header_variable (OMX_U16  byte_num,
							     OMX_U32 sample_index,
							     OMX_U8 channel_config,
							     OMX_U8 *audaac_header_adts)
{
  //uint16  bit_index=0;

  OMX_U32  value;
  OMX_U16 audaac_hdr_bit_index;

  /* Store Sync word first */
  audaac_header_adts[0] = 0xFF;
  audaac_header_adts[1] = 0xF0;

  audaac_hdr_bit_index = 12;

  /* ID field, 1 bit */
  value = 1;
  audaac_rec_install_bits(audaac_header_adts,
                          1,
                          value,
                          &(audaac_hdr_bit_index));

  /* Layer field, 2 bits */
  value = 0;
  audaac_rec_install_bits(audaac_header_adts,
                          AACHDR_LAYER_SIZE,
                          value,
                          &(audaac_hdr_bit_index));

  /* Protection_absent field, 1 bit */
  value = 1;
  audaac_rec_install_bits(audaac_header_adts,
                          AACHDR_CRC_SIZE,
                          value,
                          &(audaac_hdr_bit_index));

  /* profile_ObjectType field, 2 bit */
  value = 1;
  audaac_rec_install_bits(audaac_header_adts,
                          AAC_PROFILE_SIZE,
                          value,
                          &(audaac_hdr_bit_index));

  /* sampling_frequency_index field, 4 bits */
  audaac_rec_install_bits(audaac_header_adts,
                          AAC_SAMPLING_FREQ_INDEX_SIZE,
                          (OMX_U32)sample_index,
                          &(audaac_hdr_bit_index));

  /* pravate_bit field, 1 bits */
  audaac_rec_install_bits(audaac_header_adts,
                          1,
                          0,
                          &(audaac_hdr_bit_index));

  /* channel_configuration field, 3 bits */
  audaac_rec_install_bits(audaac_header_adts,
                          3,
                          channel_config,
                          &(audaac_hdr_bit_index));


  /* original/copy field, 1 bits */
  audaac_rec_install_bits(audaac_header_adts,
                          AAC_ORIGINAL_COPY_SIZE,
                          0,
                          &(audaac_hdr_bit_index));


  /* home field, 1 bits */
  audaac_rec_install_bits(audaac_header_adts,
                          AAC_HOME_SIZE,
                          0,
                          &(audaac_hdr_bit_index));

 // bit_index = audaac_hdr_bit_index;
 // bit_index += 2;

	/* copyr. id. bit, 1 bits */
  audaac_rec_install_bits(audaac_header_adts,
                          1,
                          0,
                          &(audaac_hdr_bit_index));

	/* copyr. id. start, 1 bits */
  audaac_rec_install_bits(audaac_header_adts,
                          1,
                          0,
                          &(audaac_hdr_bit_index));

  /* aac_frame_length field, 13 bits */
  audaac_rec_install_bits(audaac_header_adts,
                          AUDAAC_ADTS_FRAME_LENGTH_SIZE,
                          byte_num,
                          &audaac_hdr_bit_index);

  /* adts_buffer_fullness field, 11 bits */
  audaac_rec_install_bits(audaac_header_adts,
                          11,
                          0x660, /* Currently kept with CBR value */
                          &audaac_hdr_bit_index);

  /* number_of_raw_data_locks_in_frame, 2 bits */
  audaac_rec_install_bits(audaac_header_adts,
                          2,
                          0,
                          &audaac_hdr_bit_index);

} /* audaac_rec_install_adts_header_variable */

OMX_ERRORTYPE  omx_aac_aenc::fill_this_buffer_proxy(OMX_IN OMX_HANDLETYPE  hComp,
                                                     OMX_BUFFERHEADERTYPE* buffer)
{
	static int count = 0;
	int nDatalen = 0;
	META_OUT meta_out;
	OMX_TICKS tmp1 = 0;
	OMX_U8 audaac_header_adts[AUDAAC_MAX_ADTS_HEADER_LENGTH];
	OMX_U8 audaac_header_adif[AUDAAC_MAX_ADIF_HEADER_LENGTH];
	OMX_U8 audaac_header_mp4ff[AUDAAC_MAX_MP4FF_HEADER_LENGTH];


	/* Assume empty this buffer function has already checked
	validity of buffer */
	DEBUG_PRINT("Inside fill_this_buffer_proxy \n");

	if(bFlushinprogress)
	{
		DEBUG_PRINT("FTBP: flush in progress, return buffer\n");
		buffer->nFilledLen = 0;
		buffer->nFlags = 0;
		buffer->nTimeStamp= nTimestamp;
		post_output((unsigned)hComp,(unsigned) buffer,
				OMX_COMPONENT_GENERATE_FRAME_DONE);
		return OMX_ErrorNone;
	}

	if(search_output_bufhdr(buffer) == true)
	{
		if(!pcm_input)
		{
			nDatalen = read(m_drv_fd, buffer->pBuffer, OMX_AAC_OUTPUT_BUFFER_SIZE);
			DEBUG_PRINT("FTBP: read buffer %p  #%d of size = %d\n",buffer->pBuffer,
										++count, nDatalen);

			if((nDatalen < 0) || (nDatalen > (signed)OMX_AAC_OUTPUT_BUFFER_SIZE))
			{
				DEBUG_PRINT("FTBP: data length read0 %d\n",nDatalen);
				buffer->nFilledLen = 0;
				frame_done_cb(buffer);
			}
			else
			{
				buffer->nFilledLen = nDatalen;
				DEBUG_PRINT("FTBP: valid data length read = %lu\n",buffer->nFilledLen);
				frame_done_cb(buffer);
			}
		} else
		{
			DEBUG_PRINT("Fill this buffer proxy : reading data from driver\n");
			nDatalen =  read(m_drv_fd, m_tmp_out_meta_buf, OMX_AAC_OUTPUT_BUFFER_SIZE+sizeof(META_OUT));
			DEBUG_PRINT("Datalen read from driver %d \n",nDatalen);
			if(nDatalen <= 0)
			{
				DEBUG_PRINT("Failed to read data from driver : %d \n",nDatalen);
			} else
			{
				DEBUG_PRINT("Read %d bytes from driver \n",(nDatalen-sizeof(META_OUT)));
				memcpy(&meta_out, m_tmp_out_meta_buf,sizeof(META_OUT));
				if(meta_out.nFlags & EOS_FLAG)
				{
					DEBUG_PRINT("REACHED EOS IN fill_this_buffer_proxy\n");
					//Post EOS event to output thread
					post_output((unsigned)hComp,(unsigned) buffer,
							OMX_COMPONENT_GENERATE_FRAME_DONE);
					post_output((unsigned)hComp,(unsigned) buffer,
							OMX_COMPONENT_GENERATE_EOS);
					return OMX_ErrorNone;
				}
				DEBUG_PRINT("Copying the encoded stream to client buffer\n");
				switch(m_aenc_param.eAACStreamFormat)
				{
					case OMX_AUDIO_AACStreamFormatMP4ADTS:
					{
						DEBUG_PRINT("OMX_AUDIO_AACStreamFormatMP4ADTS \n");
						audaac_rec_install_adts_header_variable(((nDatalen-sizeof(META_OUT)) + AUDAAC_MAX_ADTS_HEADER_LENGTH),
										  sample_idx,
										  m_aenc_param.nChannels,
										  audaac_header_adts);
						memcpy(buffer->pBuffer,&audaac_header_adts[0], AUDAAC_MAX_ADTS_HEADER_LENGTH);
						memcpy(buffer->pBuffer+AUDAAC_MAX_ADTS_HEADER_LENGTH, &m_tmp_out_meta_buf[sizeof(META_OUT)],
							nDatalen-sizeof(META_OUT));
						buffer->nFilledLen = nDatalen - sizeof(META_OUT)+AUDAAC_MAX_ADTS_HEADER_LENGTH;
						break;
					}
					case OMX_AUDIO_AACStreamFormatADIF:
					{
						DEBUG_PRINT("OMX_AUDIO_AACStreamFormatADIF \n");
						if(flag == 0)
						{
							audaac_rec_install_adif_header_variable(((nDatalen-sizeof(META_OUT)) + AUDAAC_MAX_ADIF_HEADER_LENGTH),
											  sample_idx,
											  m_aenc_param.nChannels,
											  audaac_header_adif);
							flag++;
							memcpy(buffer->pBuffer,&audaac_header_adif[0], AUDAAC_MAX_ADIF_HEADER_LENGTH);
							memcpy(buffer->pBuffer+AUDAAC_MAX_ADIF_HEADER_LENGTH, &m_tmp_out_meta_buf[sizeof(META_OUT)],
								nDatalen-sizeof(META_OUT));
							buffer->nFilledLen = nDatalen - sizeof(META_OUT)+AUDAAC_MAX_ADIF_HEADER_LENGTH;
						} else
						{
							memcpy(buffer->pBuffer, &m_tmp_out_meta_buf[sizeof(META_OUT)],
								nDatalen-sizeof(META_OUT));
							buffer->nFilledLen = nDatalen - sizeof(META_OUT);
						}
						break;
					}
					case OMX_AUDIO_AACStreamFormatMP4FF:
					{
						DEBUG_PRINT("OMX_AUDIO_AACStreamFormatMP4FF \n");
						if(flag == 0)
						{
							audaac_rec_install_mp4ff_header_variable(((nDatalen-sizeof(META_OUT)) + AUDAAC_MAX_MP4FF_HEADER_LENGTH),
											  sample_idx,
											  m_aenc_param.nChannels,
											  audaac_header_mp4ff);
							flag++;
							memcpy(buffer->pBuffer,&audaac_header_mp4ff[0], AUDAAC_MAX_MP4FF_HEADER_LENGTH);
							memcpy(buffer->pBuffer+AUDAAC_MAX_MP4FF_HEADER_LENGTH, &m_tmp_out_meta_buf[sizeof(META_OUT)],
								nDatalen-sizeof(META_OUT));
							buffer->nFilledLen = nDatalen - sizeof(META_OUT)+AUDAAC_MAX_MP4FF_HEADER_LENGTH;
						} else
						{
							memcpy(buffer->pBuffer, &m_tmp_out_meta_buf[sizeof(META_OUT)],
								nDatalen-sizeof(META_OUT));
							buffer->nFilledLen = nDatalen - sizeof(META_OUT);
						}
						break;
					}
					case OMX_AUDIO_AACStreamFormatRAW:
					{
						DEBUG_PRINT("OMX_AUDIO_AACStreamFormatRAW \n");
						memcpy(buffer->pBuffer, &m_tmp_out_meta_buf[sizeof(META_OUT)],
							nDatalen-sizeof(META_OUT));
						buffer->nFilledLen = nDatalen - sizeof(META_OUT);
						break;
					}
					default:
						DEBUG_PRINT("Format not supported \n");
						break;

				}
				tmp1 |= (meta_out.nTimeStamp & 0xFFFFFFFF00000000LL) >> 32;
				tmp1 |= (meta_out.nTimeStamp & 0x00000000FFFFFFFFLL) << 32;
				buffer->nTimeStamp = tmp1;
				buffer->nFlags = meta_out.nFlags;
				DEBUG_PRINT("Calling frame done cb \n");
			}
			frame_done_cb(buffer);
		}
	}
	else
	{
		DEBUG_PRINT("\n Invalid buffer in FTB \n");
	}

	return OMX_ErrorNone;

}

/* ======================================================================
FUNCTION
  omx_aac_aenc::FillThisBuffer

DESCRIPTION
  IL client uses this method to release the frame buffer
  after displaying them.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::fill_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                                  OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT(" Inside fill_this_buffer \n");

    (void)hComp;

    pthread_mutex_lock(&out_buf_count_lock);
    ++m_num_out_buf;
    DEBUG_DETAIL("FTBP:m_num_out_buf is %d", m_num_out_buf);
    pthread_mutex_unlock(&out_buf_count_lock);

    post_output((unsigned)hComp,
                 (unsigned) buffer,OMX_COMPONENT_GENERATE_FTB);
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::SetCallbacks

DESCRIPTION
  Set the callbacks.

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                                           OMX_IN OMX_CALLBACKTYPE* callbacks,
                                           OMX_IN OMX_PTR             appData)
{
  (void)hComp;
  m_cb       = *callbacks;
  m_app_data =    appData;

  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::ComponentDeInit

DESCRIPTION
  Destroys the component and release memory allocated to the heap.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
  (void)hComp;
  if (OMX_StateLoaded != m_state)
  {
      DEBUG_PRINT_ERROR("Warning: Received DeInit when not in LOADED state, cur_state %d\n",
                   m_state);
      return OMX_ErrorInvalidState;
  }

  if (m_cmd_svr != NULL) {
    aenc_svr_stop(m_cmd_svr);
    m_cmd_svr = NULL;
  }

  if (m_cmd_cln != NULL) {
    aenc_cln_stop(m_cmd_cln);
    m_cmd_cln = NULL;
  }

  if (m_cmd_cln_input != NULL) {
    aenc_cln_stop(m_cmd_cln_input);
    m_cmd_cln_input = NULL;
  }

  if ( m_tmp_out_meta_buf )
        free(m_tmp_out_meta_buf);

  if ( m_tmp_in_meta_buf )
        free(m_tmp_in_meta_buf);

  if ( ioctl( m_drv_fd, AUDIO_STOP, 0) == -1 )
	DEBUG_PRINT("Stop:ioctl stop failed errno=%d\n",errno);


  if (m_drv_fd >= 0) {
    close(m_drv_fd);
	m_drv_fd = -1;
  }
  else
  {
    DEBUG_PRINT(" device close failure \n");
  }
  m_fbd_cnt=0;
  m_num_out_buf = 0;
  m_swc=0;
  m_idle_transition = 0;
  m_is_alloc_buf = 0;
  frameDuration = 0;
  flag = 0;
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_aac_aenc::UseEGLImage

DESCRIPTION
  OMX Use EGL Image method implementation <TBD>.

PARAMETERS
  <TBD>.

RETURN VALUE
  Not Implemented error.

========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::use_EGL_image(OMX_IN OMX_HANDLETYPE                hComp,
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
  omx_aac_aenc::ComponentRoleEnum

DESCRIPTION
  OMX Component Role Enum method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_aac_aenc::component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
                                                OMX_OUT OMX_U8*        role,
                                                OMX_IN OMX_U32        index)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  const char *cmp_role = "audio_encoder.aac";

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
  omx_aac_aenc::AllocateDone

DESCRIPTION
  Checks if entire buffer pool is allocated by IL Client or not.
  Need this to move to IDLE state.

PARAMETERS
  None.

RETURN VALUE
  true/false.

========================================================================== */
bool omx_aac_aenc::allocate_done(void)
{
  if(!pcm_input)	
  	return (m_out_buf_count >= OMX_CORE_NUM_OUTPUT_BUFFERS?true:false);
  else
  {
	if(m_out_buf_count >= OMX_CORE_NUM_OUTPUT_BUFFERS && m_inp_buf_count == OMX_CORE_NUM_INPUT_BUFFERS)
		return true;
	else
		return false;
  }
}


/* ======================================================================
FUNCTION
  omx_aac_aenc::ReleaseDone

DESCRIPTION
  Checks if IL client has released all the buffers.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_aac_aenc::release_done(void)
{
  if(!pcm_input)
  	return (m_out_buf_count == 0?true:false);
  else
  {	
	if(m_out_buf_count == 0 && m_inp_buf_count == 0)
		return true;
	else
		return false;
  }
}
