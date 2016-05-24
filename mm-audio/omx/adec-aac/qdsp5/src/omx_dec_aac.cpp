/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_dec_aac.cpp
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
#include "omx_dec_aac.h"
#include <unistd.h>

using namespace std;
void *get_omx_component_factory_fn()
{
    return (new COmxDecAac);
}
COmxDecAac::COmxDecAac()
:COmxBaseDec((const char*)"/dev/msm_aac",OMX_ADEC_DEFAULT_SF,OMX_ADEC_DEFAULT_CH_CFG)
{
    set_in_buf_size(OMX_CORE_INPUT_BUFFER_SIZE);
    set_out_buf_size(OMX_AAC_OUTPUT_BUFFER_SIZE);
    // pass device name(/dev/msm_aac) in the constructor
    DEBUG_PRINT("COmxDecAac::%s\n",__FUNCTION__);
}

COmxDecAac::~COmxDecAac()
{
    DEBUG_PRINT("COmxDecAac::%s\n",__FUNCTION__);
}

COmxDecAacIn::COmxDecAacIn( COmxDecAac * base, int fd,OMX_CALLBACKTYPE cb,
OMX_U32 buf_size, OMX_U32 sf, OMX_U8 ch,
OMX_BOOL pcm_feedback,
OMX_PTR appdata)
:COmxBaseDecIn(base,fd,cb,buf_size,sf,ch,pcm_feedback,appdata),
m_aac_comp(base),
m_first_aac_header(0),
m_aac_hdr_bit_index(0),
m_bytes_to_skip(0),
m_data_written_to_dsp(0),
m_silence(NULL),
m_pTranscoder(NULL),
m_bsac(OMX_FALSE),
bytesTranscoded(0)
{
    //m_silence = new silenceInsertion;
    DEBUG_PRINT("COmxDecAacIn::%s fd=%d buf_size=%u sf=%u ch=%u pcm=%u\n",\
    __FUNCTION__,fd,(unsigned int)buf_size,(unsigned int)sf,
    (unsigned int)ch,(unsigned int)pcm_feedback);
}

COmxDecAacIn::~COmxDecAacIn()
{
    DEBUG_PRINT("COmxDecAacIn:%s\n",__FUNCTION__);
    m_first_aac_header=0;
    m_aac_hdr_bit_index = 0;
    m_aac_comp = NULL;
    m_bytes_to_skip = 0;
    m_data_written_to_dsp = 0;
    m_bsac = OMX_FALSE;
    bytesTranscoded = 0;
    memset(&m_aac_comp,0,sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
    if(m_pTranscoder)
    {
    delete m_pTranscoder;
    m_pTranscoder = NULL;
    }
    if(m_silence)
    {
        delete m_silence;
    }
}

/**
@brief member function for performing component initialization

@param role C string mandating role of this component
@return Error status
*/
OMX_ERRORTYPE COmxDecAac::component_init(OMX_STRING role)
{
    DEBUG_PRINT_ERROR("COmxDecAac::%s role[%s]\n",__FUNCTION__,role);
    struct ipc_info * pCtxt = NULL;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    eRet = COmxBaseDec::component_init(role);
    if(eRet != OMX_ErrorNone)
    {
        return eRet;
    }
    memset(&m_aac_param, 0, sizeof(m_aac_param));
    m_aac_param.nSize = sizeof(m_aac_param);
    m_aac_param.nSampleRate = OMX_ADEC_DEFAULT_SF;
    m_aac_param.nChannels = OMX_ADEC_DEFAULT_CH_CFG;
    m_aac_param.nFrameLength = OMX_ADEC_AAC_FRAME_LEN;
    m_aac_param.eChannelMode = OMX_AUDIO_ChannelModeStereo;
    m_aac_param.eAACProfile = OMX_AUDIO_AACObjectLC;
    m_aac_param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP2ADTS;
    DEBUG_PRINT("\n**************INIT*********************\n");

    if(!strcmp(role,"OMX.qcom.audio.decoder.aac"))
    {
        m_pOut = new COmxBaseDecOut(this,get_drv_fd(),get_cb(),get_pcm_feedback(),
        get_app_data(),OMX_AAC_OUTPUT_BUFFER_SIZE);
        DEBUG_PRINT("\nDecAac:comp_init:role[%s]fd[%d]m_pOut[%p]bufsize=%d\n",\
                               role,get_drv_fd(), m_pOut,OMX_AAC_OUTPUT_BUFFER_SIZE);
        if(!(m_pOut->get_outh_ctxt()))
        {
            pCtxt =
                  omx_thread_create(omx_out_msg, m_pOut,(char*)"OUTPUT_THREAD");
            if(!pCtxt)
            {
                DEBUG_PRINT_ERROR("ERROR!OUTPUT THREAD failed to get created\n");
                return OMX_ErrorHardware;
            }
        }
        m_pOut->set_outh_ctxt(pCtxt);

    }

    DEBUG_PRINT("DecAac:Init fd=%d pcmfb=%d\n",get_drv_fd(),get_pcm_feedback());
    // create input and output port objects...
#ifdef AUDIOV2
    m_pIn = new COmxDecAacIn(this,get_drv_fd(),get_cb(), OMX_CORE_INPUT_BUFFER_SIZE,
    OMX_ADEC_DEFAULT_SF,OMX_ADEC_DEFAULT_CH_CFG,
    get_pcm_feedback(), get_app_data() );
#else
    m_pIn = new COmxDecAacIn_7x27(this,get_drv_fd(),get_cb(), OMX_CORE_INPUT_BUFFER_SIZE,
    OMX_ADEC_DEFAULT_SF,OMX_ADEC_DEFAULT_CH_CFG,
    get_pcm_feedback(), get_app_data() );
#endif
    m_pIn->m_input_port_flushed = OMX_TRUE;
    if(!get_cmdth_ctxt())
    {
        pCtxt = omx_thread_create(omx_cmd_msg,
        this,(char*)"CMD_THREAD");
        if(!pCtxt)
        {
            DEBUG_PRINT_ERROR("ERROR!!!INPUT THREAD failed to get created\n");
            return OMX_ErrorHardware;
        }
        else DEBUG_PRINT("Cmd thread success..%p\n",pCtxt);
        set_cmdth_ctxt(pCtxt);
        DEBUG_PRINT(" CMD TH CREATED...%p\n",pCtxt);
    }
    else DEBUG_PRINT("CMD TH NOT CREATED...\n");
    if(!(m_pIn->get_inth_ctxt()))
    {
        pCtxt = omx_thread_create(omx_in_msg, m_pIn,(char*)"IN_THREAD");
        if(!pCtxt)
        {
            DEBUG_PRINT_ERROR("ERROR!!! COMMAND THREAD failed to get created\n");
            return OMX_ErrorHardware;
        }
        else DEBUG_PRINT("Input thread success..%p\n",pCtxt);
        m_pIn->set_inth_ctxt(pCtxt);
        DEBUG_PRINT(" IN TH CREATED...%p\n",pCtxt);
    }
    else DEBUG_PRINT("IN TH NOT CREATED...\n");
#ifdef AUDIOV2
    m_pEvent = new COmxAacEventHandler(this, get_drv_fd(), OMX_TRUE,OMX_StateLoaded);
#else
    m_pEvent = new COmxAacEventHandler_7x27(this, get_drv_fd(), OMX_TRUE,OMX_StateLoaded);
#endif
    if ( !(m_pEvent->get_eventh_ctxt()))
    {
        pCtxt =
          omx_event_thread_create(omx_event_msg, m_pEvent,(char*)"EVENT_THREAD");
        if ( !pCtxt )
        {
            DEBUG_PRINT_ERROR("ERROR!!! INFO THREAD failed to get created\n");
            return OMX_ErrorHardware;
        }
        m_pEvent->set_eventh_ctxt(pCtxt);
        DEBUG_PRINT(" EVENT TH CREATED...%p\n",pCtxt);
    }
    else DEBUG_PRINT("EVENT TH NOT CREATED...\n");

    DEBUG_PRINT("::%s In[%p] out[%p] event[%p]\n",__FUNCTION__,
                                                  m_pIn,m_pOut,m_pEvent);
    DEBUG_PRINT("\n***********************************\n");
    return eRet;
}

/**
@brief member function that return parameters to IL client

@param hComp handle to component instance
@param paramIndex Parameter type
@param paramData pointer to memory space which would hold the
        paramter
@return error status
*/
OMX_ERRORTYPE  COmxDecAac::get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
OMX_IN OMX_INDEXTYPE paramIndex,
OMX_INOUT OMX_PTR     paramData)
{
    DEBUG_PRINT("COmxDecAac::%s\n",__FUNCTION__);
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if(get_state() == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Get Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if(paramData == NULL)
    {
        DEBUG_PRINT("get_parameter: paramData is NULL\n");
        return OMX_ErrorBadParameter;
    }
    //DEBUG_PRINT("COmxDecAac::get_param, Index=%d\n",paramIndex);
    switch(paramIndex)
    {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamComponentSuspended:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamPriorityMgmt:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamCompBufferSupplier:
    case OMX_IndexParamOtherInit:
    case QOMX_IndexParamAudioSessionId:
        {
            eRet = COmxBase::get_parameter(hComp,paramIndex,paramData);
            break;
        }

    case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
            portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

            DEBUG_PRINT("OMX_IndexParamPortDefinition nPortIndex = %u\n",\
            (unsigned int)portDefn->nPortIndex);
            portDefn->nVersion.nVersion = OMX_SPEC_VERSION;
            portDefn->nSize = sizeof(portDefn);
            portDefn->eDomain    = OMX_PortDomainAudio;
            if (0 == portDefn->nPortIndex)
            {
                portDefn->eDir =  OMX_DirInput;
                portDefn->bEnabled   = get_inp_bEnabled();
                portDefn->bPopulated = get_inp_bPopulated();;
                portDefn->nBufferCountActual = get_inp_act_buf_cnt();
                portDefn->nBufferCountMin    = OMX_CORE_NUM_INPUT_BUFFERS;
                portDefn->nBufferSize        = get_in_buf_size();
                portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
                portDefn->format.audio.eEncoding = OMX_AUDIO_CodingAAC;
                portDefn->format.audio.pNativeRender = 0;
            }
            else if (1 == portDefn->nPortIndex)
            {
                portDefn->eDir =  OMX_DirOutput;
                portDefn->bEnabled   = get_out_bEnabled();
                portDefn->bPopulated = get_out_bPopulated();
                portDefn->nBufferCountActual = get_out_act_buf_cnt();
                portDefn->nBufferCountMin    = OMX_CORE_NUM_OUTPUT_BUFFERS;
                portDefn->nBufferSize        = get_out_buf_size();
                portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
                portDefn->format.audio.eEncoding = OMX_AUDIO_CodingPCM;
                portDefn->format.audio.pNativeRender = 0;
            }
            else
            {
                portDefn->eDir =  OMX_DirMax;
                DEBUG_PRINT_ERROR("Bad Port idx %d\n",(int)portDefn->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case OMX_IndexParamAudioPortFormat:
        {
            OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
            (OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
            DEBUG_PRINT("OMX_IndexParamAudioPortFormat port[%u]\n",\
            (unsigned int)portFormatType->nPortIndex);
            portFormatType->nVersion.nVersion = OMX_SPEC_VERSION;
            portFormatType->nSize = sizeof(portFormatType);

            if (OMX_CORE_INPUT_PORT_INDEX == portFormatType->nPortIndex)
            {
                portFormatType->eEncoding = OMX_AUDIO_CodingAAC;
            }
            else if(OMX_CORE_OUTPUT_PORT_INDEX== portFormatType->nPortIndex)
            {
                portFormatType->eEncoding = OMX_AUDIO_CodingPCM;
            }
            else
            {
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }

    case OMX_IndexParamAudioAac:
        {
            DEBUG_PRINT("GET PARAM-->OMX_IndexParamAudioAac.. \
                SF=%u ch=%u\n",(unsigned int)m_aac_param.nSampleRate,
            (unsigned int)m_aac_param.nChannels);
            OMX_AUDIO_PARAM_AACPROFILETYPE *aacParam =
            (OMX_AUDIO_PARAM_AACPROFILETYPE *) paramData;

            if (OMX_CORE_INPUT_PORT_INDEX== aacParam->nPortIndex)
            {
                memcpy(aacParam,&m_aac_param,
                sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
            }
            else
            {
                DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioAac \
                    OMX_ErrorBadPortIndex %d\n", (int)aacParam->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }

    case OMX_IndexParamAudioPcm:
        {
            OMX_AUDIO_PARAM_PCMMODETYPE *pcmparam =
            (OMX_AUDIO_PARAM_PCMMODETYPE *) paramData;

            if (OMX_CORE_OUTPUT_PORT_INDEX== pcmparam->nPortIndex)
            {
                memcpy(pcmparam, &get_pcm_param(),\
                sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
                pcmparam->nSamplingRate = m_aac_param.nSampleRate;
                pcmparam->nChannels = m_aac_param.nChannels;

                DEBUG_PRINT("get_parameter: Sampling rate %u",
                                         (unsigned int)pcmparam->nSamplingRate);
                DEBUG_PRINT("get_parameter: Number of channels %u",
                                        (unsigned int)pcmparam->nChannels);
            }
            else
            {
                DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioPcm \
                            OMX_ErrorBadPortIndex %u\n",
                (unsigned int)pcmparam->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }

    default:
        {
            DEBUG_PRINT_ERROR("unknown param %08x\n", paramIndex);
            eRet = OMX_ErrorUnsupportedIndex;
        }
    }
    DEBUG_PRINT("Before returning from COmxDecAac:get_param\n");
    return eRet;
}


/**
@brief member function that set paramter from IL client

@param hComp handle to component instance
@param paramIndex parameter type
@param paramData pointer to memory space which holds the paramter
@return error status
*/
OMX_ERRORTYPE  COmxDecAac::set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
OMX_IN OMX_INDEXTYPE paramIndex,
OMX_IN OMX_PTR        paramData)
{
    DEBUG_PRINT("COmxDecAac::%s\n",__FUNCTION__);
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if(get_state() == OMX_StateInvalid)
    {
        DEBUG_PRINT_ERROR("Set Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if(paramData == NULL)
    {
        DEBUG_PRINT_ERROR("param data is NULL");
        return OMX_ErrorBadParameter;
    }
    switch(paramIndex)
    {
    case OMX_IndexParamPriorityMgmt:
    case OMX_IndexParamCompBufferSupplier:
    case OMX_IndexParamAudioPcm:
    case OMX_IndexParamSuspensionPolicy:
    case OMX_IndexParamStandardComponentRole:
        {
            eRet = COmxBase::set_parameter(hComp, paramIndex, paramData);
        }
        break;
    case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
            portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

            DEBUG_PRINT("OMX_IndexParamPortDefinition \
                            portDefn->nPortIndex = %u\n", \
            (unsigned int)portDefn->nPortIndex);
            if(((get_state() == OMX_StateLoaded)&&
                 !BITMASK_PRESENT(&mflags(),OMX_COMPONENT_IDLE_PENDING)) ||
                 ((get_state()== OMX_StateWaitForResources) &&
                        (((OMX_DirInput == portDefn->eDir) &&
                                (get_inp_bEnabled()== true)) ||
                            ((OMX_DirInput == portDefn->eDir) &&
                                (get_out_bEnabled() == true)))) ||
                    ((((OMX_DirInput == portDefn->eDir) &&
                                (get_inp_bEnabled() == false))||
                            ((OMX_DirInput == portDefn->eDir) &&
                                (get_out_bEnabled() == false))) &&
                        (get_state() != OMX_StateWaitForResources)))
            {
                DEBUG_PRINT("Set Parameter called in valid state\n");
            }
            else
            {
                DEBUG_PRINT_ERROR("Set Parameter called in Invalid State\n");
                return OMX_ErrorIncorrectStateOperation;
            }
            DEBUG_PRINT("OMX_IndexParamPortDefinition \
                        portDefn->nPortIndex = %u\n",\
            (unsigned int)portDefn->nPortIndex);
            if (OMX_CORE_INPUT_PORT_INDEX == portDefn->nPortIndex)
            {
                DEBUG_PRINT("SET_PARAM:OMX_IndexParamPortDefinition port[%u]"
                "bufsize[%u] buf_cnt[%u]\n",
                (unsigned int)portDefn->nPortIndex,
                (unsigned int)portDefn->nBufferSize,
                (unsigned int)portDefn->nBufferCountActual);
                if ( portDefn->nBufferCountActual > OMX_CORE_NUM_INPUT_BUFFERS )
                {
                    set_in_act_buf_cnt(portDefn->nBufferCountActual);
                }
                else
                {
                    set_in_act_buf_cnt(OMX_CORE_NUM_INPUT_BUFFERS);
                }
                set_in_buf_size(portDefn->nBufferSize);
            }
            else if (OMX_CORE_OUTPUT_PORT_INDEX == portDefn->nPortIndex)
            {
                DEBUG_PRINT("SET_PARAMETER:OMX_IndexParamPortDefinition port[%u]"
                "bufsize[%u] buf_cnt[%u]\n",
                (unsigned int)portDefn->nPortIndex,

                (unsigned int)portDefn->nBufferSize,
                (unsigned int)portDefn->nBufferCountActual);
                if ( portDefn->nBufferCountActual > OMX_CORE_NUM_OUTPUT_BUFFERS )
                {
                    set_out_act_buf_cnt(portDefn->nBufferCountActual);
                }
                else
                {
                    set_out_act_buf_cnt(OMX_CORE_NUM_OUTPUT_BUFFERS);
                }

                set_out_buf_size(portDefn->nBufferSize);
            }
            else
            {
                DEBUG_PRINT(" set_parameter: Bad Port idx %u",
                (unsigned int)portDefn->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }
    case OMX_IndexParamAudioAac:
        {
            DEBUG_PRINT("SET-PARAM::OMX_IndexParamAudioAAC");
            memcpy(&m_aac_param,paramData,
                                      sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
            DEBUG_PRINT("SF=%u Ch=%u bitrate=%u format=%d\n",
            (unsigned int)m_aac_param.nSampleRate,
            (unsigned int)m_aac_param.nChannels,
            (unsigned int)m_aac_param.nBitRate,
            m_aac_param.eAACStreamFormat);
            set_sf(m_aac_param.nSampleRate);
            set_ch(m_aac_param.nChannels);
            m_pIn->set_sf(m_aac_param.nSampleRate);
            m_pIn->set_ch(m_aac_param.nChannels);

            break;
        }

    case OMX_IndexParamAudioPortFormat:
        {
            OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
            (OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
            DEBUG_PRINT("set_parameter: OMX_IndexParamAudioPortFormat\n");

            if (OMX_CORE_INPUT_PORT_INDEX== portFormatType->nPortIndex)
            {
                portFormatType->eEncoding = OMX_AUDIO_CodingAAC;
            }
            else if(OMX_CORE_OUTPUT_PORT_INDEX == portFormatType->nPortIndex)
            {
                DEBUG_PRINT("set_parameter: OMX_IndexParamAudioFormat: %u\n",
                (unsigned int)portFormatType->nIndex);
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
COmxDecAac::GetExtensionIndex

DESCRIPTION
OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  COmxDecAac::get_extension_index(
OMX_IN OMX_HANDLETYPE      hComp,
OMX_IN OMX_STRING      paramName,
OMX_OUT OMX_INDEXTYPE* indexType)
{
    OMX_ERRORTYPE eRet=OMX_ErrorNone;
    eRet = COmxBase::get_extension_index(hComp, paramName, indexType);
    return eRet;
}

/* ======================================================================
FUNCTION
COmxDecAac::ComponentRoleEnum

DESCRIPTION
OMX Component Role Enum method implementation.

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  COmxDecAac::component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
OMX_OUT OMX_U8*        role,
OMX_IN OMX_U32        index)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    const char *cmp_role = "audio_decoder.aac";

    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
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
COmxDecAac::ComponentDeInit

DESCRIPTION
Destroys the component and release memory allocated to the heap.

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  COmxDecAac::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ((OMX_StateLoaded != get_state()) && (OMX_StateInvalid != get_state()))
    {
        DEBUG_PRINT_ERROR("Warning:Rxed DeInit,not in LOADED state[%d]\n",
        get_state());
    }
    DEBUG_PRINT("COmxDecAac::%s\n",__FUNCTION__);
    eRet = COmxBaseDec::component_deinit(hComp);
    return eRet;
}

void COmxDecAacIn::process_in_port_msg(unsigned char id)
{
    unsigned      p1;       // Parameter - 1
    unsigned      p2;       // Parameter - 2
    unsigned char ident;
    unsigned      qsize     = 0;
    unsigned      tot_qsize = 0;
    OMX_STATETYPE state;

    DEBUG_PRINT("COmxDecAacIn::%s id=%d\n",__FUNCTION__,id);
loopback_in:
    OMX_U32       tcxo_space = get_comp()->getFilledSpaceInTcxoBuf();

    state = get_state();

    if(state == OMX_StateLoaded)
    {
        DEBUG_PRINT(" IN: IN LOADED STATE RETURN\n");
        return;
    }
    // Protect the shared queue data structure

    omx_cmd_queue *cmd_q  = get_cmd_q();
    omx_cmd_queue *bd_q  = get_bd_q();
    omx_cmd_queue *data_q = get_data_q();
    qsize = get_q_size(cmd_q);
    tot_qsize = qsize;
    tot_qsize += get_q_size(bd_q);
    tot_qsize += get_q_size(data_q);

    if ( 0 == tot_qsize )
    {
        DEBUG_DETAIL("IN-->BREAKING FROM IN LOOP");
        return;
    }

    if ((state != OMX_StateExecuting) && !(qsize))
    {
        if(state == OMX_StateLoaded)
        return;

        DEBUG_DETAIL("SLEEPING IN THREAD\n");
        get_comp()->in_th_sleep();
        goto loopback_in;
    }
    else if ((state == OMX_StatePause))
    {
        if(!(qsize = get_q_size(cmd_q)))
        {
            DEBUG_DETAIL("IN: SLEEPING IN THREAD\n");
            get_comp()->in_th_sleep();

            goto loopback_in;
        }
    }
    DEBUG_DETAIL("Input-->QSIZE-flush=%d,ebd=%d QSIZE=%d state=%d tcxo[%lu]\n",\
    get_q_size(cmd_q),
    get_q_size(bd_q),
    get_q_size(data_q),state,tcxo_space);
    p1=0;p2=0;ident=0;
    if(qsize)
    {
        // process FLUSH message
        get_msg(cmd_q, &p1,&p2,&ident);
    }
    else if((qsize = get_q_size(bd_q)) &&
            (state == OMX_StateExecuting))
    {
        get_msg(bd_q, &p1,&p2,&ident);
    }
    else if((qsize = get_q_size(data_q)) &&
            (state == OMX_StateExecuting) && !tcxo_space)
    {
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
        DEBUG_PRINT("IN--> Empty Queue state=%d %d %d %d tcxoFilled[%lu]\n",\
        state,\
        get_q_size(cmd_q),
        get_q_size(bd_q),
        get_q_size(data_q),tcxo_space);

        if(get_state() == OMX_StatePause || tcxo_space)
        {
            DEBUG_DETAIL("IN: SLEEPING AGAIN IN THREAD\n");
            get_comp()->in_th_sleep();
            goto loopback_in;
        }
    }

    if(qsize > 0)
    {
        id = ident;
        DEBUG_DETAIL("Input->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d]\n",\
        get_state(),
        ident,
        get_q_size(cmd_q),
        get_q_size(bd_q),
        get_q_size(data_q));

        if(id == OMX_COMPONENT_GENERATE_BUFFER_DONE)
        {
            buffer_done_cb((OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_ETB)
        {
            process_etb((OMX_HANDLETYPE)p1,
            (OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_COMMAND)
        {
            // Execute FLUSH command
            if(p1 == OMX_CommandFlush)
            {
                DEBUG_DETAIL(" Executing FLUSH command on Input port\n");
                execute_input_omx_flush();
            }
            else
            {
                DEBUG_DETAIL("Invalid command[%d]\n",p1);
            }
        }
        else if(id == OMX_COMPONENT_SUSPEND)
        {
            DEBUG_PRINT("IN:FAKING EOS TO KERNEL : m_eos_bm=%d", \
            get_comp()->get_eos_bm());
            // dont trigger fake EOS when actual EOS has already been sent
            if(!(get_comp()->get_eos_bm() & IP_OP_PORT_BITMASK))
            {
                omx_fake_eos();
            }
            else if((get_comp()->get_eos_bm() & IP_OP_PORT_BITMASK) ==
                    IP_OP_PORT_BITMASK)
            {
                get_comp()->setSuspendFlg();
                get_comp()->setResumeFlg();
                //EOS already reached, dont trigger suspend/resume,
                // but set the flag so that one more suspend/event doesnt
                // get triggered from timeout/driver
                DEBUG_PRINT("IN--> AUDIO_STOP %d %d \n",
                get_comp()->getSuspendFlg(),
                get_comp()->getResumeFlg());
                ioctl(get_drv_fd(), AUDIO_STOP, 0);
                get_comp()->set_dec_state(false);
                if (get_comp()->getWaitForSuspendCmplFlg())
                {
                    DEBUG_PRINT("Release P-->Executing context to IL client.\n");
                    get_comp()->release_wait_for_suspend();
                }

            }
            else
            {
                // suspend event after input eos sent
                DEBUG_PRINT("IN--> Do nothing, m_eos_bm=%d\n",
                                                     get_comp()->get_eos_bm());
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("ERROR:IN-->Invalid Id[%d]\n",id);
        }
    }
    else
    {
        DEBUG_DETAIL("ERROR:IN-->Empty INPUT Q\n");
    }
    return;
}

OMX_ERRORTYPE  COmxDecAacIn::process_etb(
OMX_IN OMX_HANDLETYPE         hComp,
OMX_BUFFERHEADERTYPE* buffer)
{
    struct aac_header aac_header_info;
    int res = 0;
    struct msm_audio_config drv_config;
    OMX_STATETYPE state;
    META_IN meta_in;

    DEBUG_PRINT("COmxDecAacIn::%s\n",__FUNCTION__);

    /* Assume empty this buffer function has already checked
    validity of buffer */

    bool flg= false;
    bool adif = false;
    m_bytes_to_skip = 0;
    PrintFrameHdr(OMX_COMPONENT_GENERATE_ETB,buffer);
    DEBUG_PRINT("ETBP:aac_header=%d m_aac_comp=%p\n",m_first_aac_header,m_aac_comp);
    if(m_aac_comp->get_eos_bm())
    {
        m_aac_comp->set_eos_bm(0);
    }
    if(buffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
        unsigned eos_bm=0;
        eos_bm= get_comp()->get_eos_bm();
        eos_bm |= IP_PORT_BITMASK;
        get_comp()->set_eos_bm(eos_bm);
        DEBUG_PRINT_ERROR("ETBP:EOSFLAG=%d\n",eos_bm);
    }
    memcpy(&m_aac_param, &(m_aac_comp->get_aac_param()),\
    sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

    if(m_first_aac_header == 0)
    {
        DEBUG_PRINT("ETBP: Callin aac frame header parsing\n");
        res = aac_frameheader_parser((OMX_BUFFERHEADERTYPE*)buffer,
        &aac_header_info);
        if(m_bsac) {
            m_pTranscoder = new COmxBsacTranscoder(OMX_CORE_INPUT_BUFFER_SIZE,
                                AAC_RESIDUALDATA_BUFFER_SIZE,sizeof(META_IN));
        }
        if(res ==0)
        {
            m_first_aac_header = 1; /* No more parsing after first frame*/
            if(ioctl(get_drv_fd(), AUDIO_GET_CONFIG, &drv_config) == -1)
            DEBUG_PRINT("ETBP:get-config ioctl failed %d\n",errno);
            else
            DEBUG_PRINT("ETBP: Get config success\n");
            if(aac_header_info.input_format == FORMAT_ADIF)
            {
                // sampling frequency
                drv_config.sample_rate = aac_header_info.head.adif.sample_rate;
                drv_config.channel_count =
                aac_header_info.head.adif.channel_config;
                m_aac_param.nSampleRate = aac_header_info.head.adif.sample_rate;
                m_aac_param.nChannels = aac_header_info.head.adif.channel_config;
                m_aac_param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatADIF;
                adif = true;
                DEBUG_PRINT("ETBP-->ADIF SF=%u ch=%u\n",\
                (unsigned int)m_aac_param.nSampleRate,
                (unsigned int)m_aac_param.nChannels);
            }
            else if(aac_header_info.input_format == FORMAT_ADTS)
            {
                // sampling frequency
                drv_config.sample_rate =
                aac_frequency_index[aac_header_info.head.adts.fixed.\
                sampling_frequency_index];
                DEBUG_PRINT("ETB-->ADTS format, SF index=%u\n",\
                (unsigned int)aac_header_info.head.adts.fixed.
                sampling_frequency_index);
                drv_config.channel_count =
                aac_header_info.head.adts.fixed.channel_configuration;
                m_aac_param.nSampleRate =
                aac_frequency_index[
                aac_header_info.head.adts.fixed.sampling_frequency_index];
                m_aac_param.nChannels =
                aac_header_info.head.adts.fixed.channel_configuration;
                m_aac_param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP4ADTS;
                DEBUG_PRINT("ETBP-->ADTS SF=%u ch=%u\n",\
                (unsigned int)m_aac_param.nSampleRate,
                (unsigned int)m_aac_param.nChannels);
            }
            else if(aac_header_info.input_format == FORMAT_LOAS)
            {
                m_aac_param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP4LOAS;
                drv_config.sample_rate   = aac_frequency_index[
                aac_header_info.head.loas.freq_index];
                drv_config.channel_count =
                aac_header_info.head.loas.channel_config;
                m_aac_param.nSampleRate = aac_frequency_index[
                aac_header_info.head.loas.freq_index];
                m_aac_param.nChannels = aac_header_info.head.loas.channel_config;
                DEBUG_PRINT("ETBP-->LOAS SF=%u ch=%u\n",\
                (unsigned int)m_aac_param.nSampleRate,
                (unsigned int)m_aac_param.nChannels);
            }
            else
            {
                DEBUG_PRINT("ETBP-->RAW AAC FORMAT\n");
                // sampling frequency
                drv_config.sample_rate =
                aac_frequency_index[aac_header_info.head.raw.freq_index];
                drv_config.channel_count =
                aac_header_info.head.raw.channel_config;
                m_aac_param.nSampleRate =
                aac_frequency_index[aac_header_info.head.raw.ext_freq_index];
                m_aac_param.nChannels = aac_header_info.head.raw.channel_config;
                DEBUG_PRINT("sample_rate=%d ch_cnt=%d\n",\
                drv_config.sample_rate,drv_config.channel_count);
                flg = true;
                m_aac_param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;
            }
            if(get_comp()->get_pcm_feedback())
            {
                drv_config.meta_field = 1;
                OMX_U32 th_val=1;
                if(ioctl(get_drv_fd(), AUDIO_SET_ERR_THRESHOLD_VALUE, &th_val) == -1)
                {
                    DEBUG_PRINT("AUDIO_SET_ERR_THRESHOLD_VALUE ioctl failed %d\n",errno);
                }
                else
                {
                    DEBUG_PRINT("AUDIO_SET_ERR_THRESHOLD_VALUE  success\n");
                }
            }
            else
            {
                drv_config.meta_field = 0;
            }
            drv_config.buffer_size = get_comp()->get_in_buf_size();
            drv_config.buffer_count = get_comp()->get_inp_act_buf_cnt();
            drv_config.type = 2;

            if((m_aac_param.nChannels > 2) || (m_aac_param.nChannels == 0))
            {
            DEBUG_PRINT("ETBP:num of ch detected %u is not supported \n",\
            (unsigned int)m_aac_param.nChannels);
            DEBUG_PRINT("SET-CFG:bufsze[%d]bufcnt[%d]chcnt[%d]SF[%d]meta[%d]",\
            drv_config.buffer_size,
            drv_config.buffer_count,
            drv_config.channel_count,
            drv_config.sample_rate,
            drv_config.meta_field);

                m_aac_param.nChannels = 2;
                drv_config.channel_count = 2;
            }
            if(m_aac_param.nSampleRate > 48000)
            {
                m_aac_param.nSampleRate = 48000;
                drv_config.sample_rate  = 48000;
            }
            if(ioctl(get_drv_fd(), AUDIO_SET_CONFIG, &drv_config) == -1)
            DEBUG_PRINT("ETBP:set-config ioctl failed %d\n",errno);
            else
            DEBUG_PRINT("ETBP: set config success\n");
            set_aac_config();

            if ( ioctl(get_drv_fd(), AUDIO_START, 0) < 0 )
            {
                DEBUG_PRINT("ETBP:ioctl audio start failed %d\n",errno);
                //m_first_aac_header=0;
                get_comp()->post_command((unsigned)OMX_CommandStateSet,
                (unsigned)OMX_StateInvalid,
                OMX_COMPONENT_GENERATE_COMMAND);
                get_comp()->post_command(OMX_CommandFlush,-1,
                                         OMX_COMPONENT_GENERATE_COMMAND);
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
                get_comp()->set_dec_state(false);
                return OMX_ErrorInvalidState;
            }
            else
            DEBUG_PRINT("ETBP: audio_start success\n");
            get_comp()->set_dec_state(true);
        }
        else
        {
            DEBUG_PRINT("configure Driver for AAC playback samplerate[%u]\n",\
            (unsigned int)m_aac_param.nSampleRate);
            if(ioctl(get_drv_fd(), AUDIO_GET_CONFIG, &drv_config) == -1)
            DEBUG_PRINT("ETBP:get-config ioctl failed %d\n",errno);
            drv_config.sample_rate = m_aac_param.nSampleRate;
            drv_config.channel_count = m_aac_param.nChannels;
            drv_config.type = 2; // aac decoding
            drv_config.buffer_size = get_comp()->get_in_buf_size();
            drv_config.buffer_count = get_comp()->get_inp_act_buf_cnt();
            if(ioctl(get_drv_fd(), AUDIO_SET_CONFIG, &drv_config) == -1)
            DEBUG_PRINT("ETBP:set-config ioctl failed %d\n",errno);
            set_aac_config();
            if(ioctl(get_drv_fd(), AUDIO_START, 0) < 0)
            {
                DEBUG_PRINT("ETBP:audio start ioctl failed %d\n",errno);
                m_first_aac_header=0;
                get_comp()->post_command((unsigned)OMX_CommandStateSet,
                (unsigned)OMX_StateInvalid,
                OMX_COMPONENT_GENERATE_COMMAND);
                get_comp()->post_command(OMX_CommandFlush,-1,
                                         OMX_COMPONENT_GENERATE_COMMAND);
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);

                get_comp()->set_dec_state(false);
                return OMX_ErrorInvalidState;
            }
            get_comp()->set_dec_state(true);
        }
    }

    m_aac_comp->set_aac_param(&m_aac_param);
    if(!flg)
    {
        OMX_U32 len=0,dst_len =0;
        OMX_U8 *data=NULL;

        if(m_pTranscoder)
        {
            data = m_pTranscoder->transcodeData(buffer->pBuffer,buffer->nFilledLen,&dst_len);
	    if(data == 0 && dst_len == 0)
	    {
                DEBUG_PRINT("ETBP:transcoding failed, corrupt/unsupported clip\n");
                get_comp()->post_command((unsigned)OMX_CommandStateSet,
                (unsigned)OMX_StateInvalid,
                OMX_COMPONENT_GENERATE_COMMAND);
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
                get_comp()->post_command(OMX_CommandFlush,-1,
                                         OMX_COMPONENT_GENERATE_COMMAND);

                return OMX_ErrorInvalidState;
	    }
	    else
		len += dst_len;
        }
        else
        {
             data = get_tmp_meta_buf();
             process_aac(buffer,data,len,meta_in,adif);
        }
        if( get_comp()->get_pcm_feedback())
        {
            meta_in.offsetVal  = sizeof(META_IN);
            meta_in.nTimeStamp =
                            (((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp)*1000;
            meta_in.nFlags     = buffer->nFlags;
            memcpy(data,&meta_in, meta_in.offsetVal);
            len += meta_in.offsetVal;
        }
        else
        {
             // Advance the pointer by size of META_IN as both transcodeData(for bsac) and process_aac
             // would be inserting payloads considering the size of META_IN as well;
             // In other words, ignore META_IN in tunnel mode
             data = data+sizeof(META_IN);
        }
        /* No need insert silence when FF/RW is performed i.e whenever the input port
           flushed do not check for silence insertion */
        if(m_input_port_flushed == OMX_TRUE)
        {
            m_input_port_flushed = OMX_FALSE;
            if(m_silence)
            {
                m_silence->set_curr_ts((((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp));
                if(m_first_aac_header == 1)
                {
                   m_silence->set_prev_ts(0);
                }
                else
                {
                    m_silence->set_prev_ts((((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp));
                }
            }
        } 
        else if(m_silence && (OMX_AUDIO_AACStreamFormatRAW == m_aac_param.eAACStreamFormat))
        {
            m_silence->set_curr_ts((((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp));
            if((m_silence->checkForTimeStampGap(m_aac_param.nSampleRate,m_aac_param.nFrameLength)) == 
                                                                      OMX_TRUE)
            {
                OMX_U8 *silenceBuf = NULL;
                OMX_U32 iLen=0;
                iLen =  sizeof(META_IN) + sizeof(PSEUDO_RAW);
                silenceBuf = (OMX_U8*)malloc(iLen);

                while(m_silence->get_num_sil_frame())
                {
                    m_silence->insertSilenceInBuffer(silenceBuf,m_aac_param.nFrameLength,m_aac_param.nChannels);
                    write(get_drv_fd(),silenceBuf,(sizeof(META_IN)+ sizeof(PSEUDO_RAW)));
                    memset(silenceBuf,0,iLen);
                }
                m_silence->set_curr_ts((((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp));
                free(silenceBuf);
            }
            m_silence->set_prev_ts(m_silence->get_curr_ts());
        }
        if(get_comp()->get_flush_stat())
            {
                buffer->nFilledLen = 0;
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
                return OMX_ErrorNone;
            }
            if(aac_header_info.input_format == FORMAT_ADIF)
            {
                if ((get_comp()->get_pcm_feedback() == 1) &&
                    (meta_in.nFlags & OMX_BUFFERFLAG_EOS))
                {
                    meta_in.nFlags &= !OMX_BUFFERFLAG_EOS;
                    memcpy(data,&meta_in, meta_in.offsetVal);
                    if (buffer->nFilledLen )
                    {
                        write(get_drv_fd(), data, len);
                    }
                    memset((data + meta_in.offsetVal), 0xFF,  DSP_MIN_BUF_SIZE);
                    DEBUG_PRINT("ADIF: NT: Writing 1550 bytes.\n");
                    meta_in.nFlags |= OMX_BUFFERFLAG_EOS;
                    memcpy(data,&meta_in, meta_in.offsetVal);
                    write(get_drv_fd(), data,
                                     ((meta_in.offsetVal) +  DSP_MIN_BUF_SIZE));
                }
                else if ((get_comp()->get_pcm_feedback() == 0) &&
                         (buffer->nFlags & OMX_BUFFERFLAG_EOS))
                {
                    if (buffer->nFilledLen )
                    {
                        write(get_drv_fd(), data, len);
                    }
                    memset(data, 0xFF,DSP_MIN_BUF_SIZE);
                    DEBUG_PRINT("ADIF: T: Writing 1550 bytes.\n");
                    write(get_drv_fd(), data, DSP_MIN_BUF_SIZE);
                    DEBUG_PRINT("EOS OCCURED \n");
                    fsync(get_drv_fd());
                    DEBUG_PRINT("INform input EOS to the client \n");
                    get_cb().EventHandler(&(get_comp()->m_cmp), get_app_data(),
                    OMX_EventBufferFlag,
                    1, 0, NULL );
                }
                else
                {
                    DEBUG_PRINT("Data written to driver len = %lu\n",len);
                    write(get_drv_fd(), data,len);
                }
            }
            else /*(aac_header_info.input_format != FORMAT_ADIF)*/
            {
                DEBUG_PRINT("Data written to driver len = %u\n",(unsigned int)len);
                write(get_drv_fd(), data,len);
                if (buffer->nFlags & 0x01)
                {
                    DEBUG_PRINT("EOS OCCURED \n");
                    DEBUG_PRINT("Writting the Final EOS data to Driver ");
                    if (get_comp()->get_pcm_feedback() == 0)
                    {
                        fsync(get_drv_fd());
                        DEBUG_PRINT("Inform input EOS to client\n");
                        get_cb().EventHandler(&(get_comp()->m_cmp),
                        get_app_data(),
                        OMX_EventBufferFlag,
                        1, 0, NULL );
                    }
                }
            }/*end of else (aac_header_info.input_format != FORMAT_ADIF)*/
        }

    state = get_state();



    if (state == OMX_StateExecuting)
    {
        //DEBUG_DETAIL("ETBP:EBD CB buffer=0x%x\n",buffer);
        buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
    }
    else
    {
        // post the msg to input thread.
        DEBUG_DETAIL("ETBP:Not in Exe state, state[0x%x] bufHdr[%p]\n",\
        state,buffer);
        post_input((unsigned) & hComp,(unsigned) buffer,
        OMX_COMPONENT_GENERATE_BUFFER_DONE);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE  COmxDecAacIn::set_aac_config()
{
    DEBUG_PRINT("COmxDecAacIn::%s\n",__FUNCTION__);
    struct msm_audio_aac_config aac_config;
    int drv_fd = get_drv_fd();

    DEBUG_PRINT("Set-aac-config: Ch=%u SF=%u profile=%d stream=%d chmode=%d\n",
    (unsigned int)m_aac_param.nChannels,
    (unsigned int)m_aac_param.nSampleRate,
    m_aac_param.eAACProfile,m_aac_param.eAACStreamFormat,
    m_aac_param.eChannelMode);

    if (ioctl(drv_fd, AUDIO_GET_AAC_CONFIG, &aac_config)) {
        DEBUG_PRINT("omx_aac_adec::set_aac_config():GET_AAC_CONFIG failed");
        m_first_aac_header=0;
        if(drv_fd >= 0){
            get_comp()->post_command((unsigned)OMX_CommandStateSet,
            (unsigned)OMX_StateInvalid,
            OMX_COMPONENT_GENERATE_COMMAND);
        }
        return OMX_ErrorInvalidComponent;
    }
    else
    DEBUG_PRINT("ETBP: get aac config success\n");

    DEBUG_PRINT("AAC-CFG::format[%d]chcfg[%d]audobj[%d]epcfg[%d]data_res[%d]\n",
    aac_config.format,
    aac_config.channel_configuration,
    aac_config.audio_object,
    aac_config.ep_config,
    aac_config.aac_section_data_resilience_flag);

    DEBUG_PRINT("CFG:scalefact[%d]spectral[%d]sbron[%d]sbrps[%d]dualmono[%d]",\
    aac_config.aac_scalefactor_data_resilience_flag,
    aac_config.aac_spectral_data_resilience_flag,
    aac_config.sbr_on_flag,
    aac_config.sbr_ps_on_flag,
    aac_config.dual_mono_mode);

    switch(m_aac_param.eAACStreamFormat)
    {
    case OMX_AUDIO_AACStreamFormatMP4LOAS:
        aac_config.format = AUDIO_AAC_FORMAT_LOAS;
        break;
    case OMX_AUDIO_AACStreamFormatMP4ADTS:
        aac_config.format = AUDIO_AAC_FORMAT_ADTS;
        break;
    case OMX_AUDIO_AACStreamFormatADIF:
        aac_config.format = AUDIO_AAC_FORMAT_RAW ;
        break;
    case OMX_AUDIO_AACStreamFormatRAW:
    default:
        aac_config.format = AUDIO_AAC_FORMAT_PSUEDO_RAW ;
        if(m_pTranscoder)
        {
            aac_config.audio_object = AUDIO_AAC_OBJECT_BSAC;
        }
        break;
    }

    switch(m_aac_param.eAACProfile)
    {
    case  OMX_AUDIO_AACObjectLC:
    default:
        aac_config.sbr_on_flag = 1;
        aac_config.sbr_ps_on_flag = 1;
        break;
    case  OMX_AUDIO_AACObjectHE:
        aac_config.sbr_on_flag = 1;
        aac_config.sbr_ps_on_flag = 1;
        break;
    case  OMX_AUDIO_AACObjectHE_PS:
        aac_config.sbr_on_flag = 1;
        aac_config.sbr_ps_on_flag = 1;
        break;
    }

    aac_config.channel_configuration = m_aac_param.nChannels;
    if (ioctl(drv_fd, AUDIO_SET_AAC_CONFIG, &aac_config)) {
        DEBUG_PRINT("set_aac_config():AUDIO_SET_AAC_CONFIG failed");
        m_first_aac_header=0;
        get_comp()->post_command((unsigned)OMX_CommandStateSet,
        (unsigned)OMX_StateInvalid,
        OMX_COMPONENT_GENERATE_COMMAND);
        return OMX_ErrorInvalidComponent;
    }
    else
    DEBUG_PRINT("ETBP: set aac config success\n");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE  COmxDecAacIn::aac_frameheader_parser(
OMX_BUFFERHEADERTYPE* bufHdr,struct aac_header *header)
{
    DEBUG_PRINT("COmxDecAacIn::%s\n",__FUNCTION__);
    OMX_U8 *buf = bufHdr->pBuffer;
    OMX_U32 ext_aud_obj_type = 0;

    if( (buf[0] == 0x41) &&  (buf[1] == 0x44) &&
            (buf[2] == 0x49) &&  (buf[3] == 0x46) )
    /*check "ADIF" */
    {
        // format is ADIF
        // parser to parse ADIF
        header->input_format = FORMAT_ADIF;
        audaac_extract_adif_header(bufHdr->pBuffer,header);
    }

    else
    {
        DEBUG_PRINT("Parser-->RAW or Partial frame to be found...\n");
        OMX_U8 *data;
        OMX_U32 temp, ext_flag, status = 0, i = 0;
        OMX_U8 sync_word_found = 0;
        while(i < (bufHdr->nFilledLen))
        {
            if ((buf[i] == 0xFF) && ((buf[i+1] & 0xF6) == 0xF0) )
            {
                OMX_U32 index=0;OMX_U32 sf=0;
                if((i+3) >= (bufHdr->nFilledLen))
                {
                    break;
                }
                sf = (buf[i+2] & 0x3C) >> 2;
                // Get the frequency index from bits 19,20,21,22
                //Get the channel configuration bits 24,25,26
                index = (buf[i+2] & 0x01) << 0x02 ;
                index |= (buf[i+3] & 0xC0) >> 0x06;
                DEBUG_PRINT("PARSER-->Partial frame..ADTS sync word \n");
                DEBUG_PRINT("ADTS-->freq_index=%u ch-cfg=%u\n",\
                (unsigned int)sf,(unsigned int)index);

                //if not ok continue, false sync words found
                if((sf > 12) || (index > 2))
                {
                    DEBUG_PRINT("Parser-->fake sync words sf=%u index=%u\n",\
                    (unsigned int)sf,(unsigned int)index);
                    i++;
                    // invalid SF/ch-cfg, maybe fake sync word found
                    continue;
                }
                sync_word_found = 1;
                m_bytes_to_skip = i;
                DEBUG_PRINT("ADTS-->bytes to skip : %d\n", m_bytes_to_skip);
                header->input_format = FORMAT_ADTS;
                header->head.adts.fixed.channel_configuration = index;
                header->head.adts.fixed.sampling_frequency_index = sf;
                break;
            }
            else if ((buf[i] == 0x56) && ((buf[i+1] & 0xE0) == 0xE0) )
            {
                DEBUG_PRINT("PARSER-->PARTIAL..LOAS\n");
                unsigned int ret = 0;
                //DEBUG_PRINT("i=%u buf[i]=%u buf[i+1] %u\n",i,buf[i], buf[i+1]);

                ret = audaac_extract_loas_header(
                &bufHdr->pBuffer[i],
                (((bufHdr->nFilledLen)-i)*8),header);
                if((int)ret == -1)
                {
                    DEBUG_PRINT("Parser--> fake sync words SF=%u cf=%u\n",\
                    (unsigned int)header->head.loas.freq_index,\
                    (unsigned int)header->head.loas.channel_config);
                    i++;
                    // invalid SF/ch-cfg, maybe fake sync word found
                    continue;
                }
                sync_word_found = 1;
                m_bytes_to_skip = i;
                DEBUG_PRINT("LOAS-->bytes to skip : %d\n", m_bytes_to_skip);

                header->input_format = FORMAT_LOAS;

                break;
            }
            i++;
        }
        if(!sync_word_found)
        {
            // RAW AAC-ADTS PARSER
            header->input_format = FORMAT_RAW;

            m_aac_hdr_bit_index = 0;
            data = (OMX_U8*)buf;

            audaac_extract_bits(data, 5, &temp);
            header->head.raw.aud_obj_type = temp;
            header->head.raw.ext_aud_obj_type = temp;
            DEBUG_DETAIL("RAW-->aud_obj_type=0x%x\n",\
                                                 header->head.raw.aud_obj_type);
            if(header->head.raw.ext_aud_obj_type == 22)
            {
                m_bsac = OMX_TRUE;/*BSAC*/
            }
            audaac_extract_bits(data, AAC_SAMPLING_FREQ_INDEX_SIZE, &temp);
            header->head.raw.freq_index = temp;
            header->head.raw.ext_freq_index = temp;
            DEBUG_DETAIL("RAW-->freq_index=0x%x\n",header->head.raw.freq_index);
            if(header->head.raw.freq_index == 0xf)
            {
                /* Current release does not support sampling freqiencies
                 other than frequencies listed in aac_frequency_index[] */
                /* Extract sampling frequency value */
                audaac_extract_bits(data, 12, &temp);
                audaac_extract_bits(data, 12, &temp);
            }
            /* Extract channel config value */
            audaac_extract_bits(data, 4, &temp);
            header->head.raw.channel_config = temp;
            DEBUG_DETAIL("RAW-->ch cfg=0x%x\n",header->head.raw.channel_config);
            header->head.raw.sbr_present_flag = 0;
            header->head.raw.sbr_ps_present_flag = 0;
            /* If the audioOjectType is SBR or PS */
            if((header->head.raw.aud_obj_type == 5) ||
                    (header->head.raw.aud_obj_type == 29))
            {
                header->head.raw.ext_aud_obj_type = 5;
                header->head.raw.sbr_present_flag = 1;
                if (header->head.raw.aud_obj_type == 29)
                {
                    header->head.raw.ext_aud_obj_type = 29;
                    header->head.raw.sbr_ps_present_flag = 1;
                }
                /* extensionSamplingFrequencyIndex */
                audaac_extract_bits(data, 4, &temp);
                header->head.raw.ext_freq_index = temp;

                if (header->head.raw.ext_freq_index == 0x0f)
                {
                    /* Extract sampling frequency value */
                    audaac_extract_bits(data, 12, &temp);
                    audaac_extract_bits(data, 12, &temp);
                }
                audaac_extract_bits(data, 5, &temp);
                header->head.raw.aud_obj_type = temp;
            }
            /* If audioObjectType is AAC_LC or AAC_LTP */
            if (header->head.raw.aud_obj_type == 2 ||
                    header->head.raw.aud_obj_type == 4)
            {
                /* frame_length_flag : 0 - 1024; 1 - 960
        Current release supports AAC frame length of 1024 only */
                audaac_extract_bits(data, 1, &temp);
                if(temp) status = 1;
                /* dependsOnCoreCoder == 1, core coder has different sampling rate
        in a scalable bitstream. This is mainly set fro AAC_SCALABLE Object only.
        Currrent release does not support AAC_SCALABLE Object */
                audaac_extract_bits(data, 1, &temp);
                if(temp) status = 1;

                /* extensionFlag */
                audaac_extract_bits(data, 1, &ext_flag);

                if(header->head.raw.channel_config == 0 && status == 0)
                {
                    OMX_U32 num_fce, num_bce, num_sce, num_lfe, num_ade, num_vce;
                    OMX_U32 i, profile, samp_freq_idx, num_ch = 0;

                    /* Parse Program configuration element information */
                    audaac_extract_bits(data, AAC_ELEMENT_INSTANCE_TAG_SIZE, &temp);
                    audaac_extract_bits(data, AAC_PROFILE_SIZE, &profile);
                    DEBUG_DETAIL("RAW-->PCE -- profile=0x%lu\n",profile);
                    audaac_extract_bits(data,
                    AAC_SAMPLING_FREQ_INDEX_SIZE,
                    &samp_freq_idx);
                    DEBUG_DETAIL("RAW-->PCE -- samp_freq_idx=%lu\n",samp_freq_idx);
                    audaac_extract_bits(data, AAC_NUM_FRONT_CHANNEL_ELEMENTS_SIZE,
                    &num_fce);
                    DEBUG_DETAIL("RAW-->PCE -- num_fce=%lu\n",num_fce);

                    audaac_extract_bits(data, AAC_NUM_SIDE_CHANNEL_ELEMENTS_SIZE,
                    &num_sce);
                    DEBUG_DETAIL("RAW-->PCE -- num_sce=%lu\n",num_sce);
                    audaac_extract_bits(data, AAC_NUM_BACK_CHANNEL_ELEMENTS_SIZE,
                    &num_bce);
                    DEBUG_DETAIL("RAW-->PCE -- num_bce=%lu\n",num_bce);

                    audaac_extract_bits(data, AAC_NUM_LFE_CHANNEL_ELEMENTS_SIZE,
                    &num_lfe);
                    DEBUG_DETAIL("RAW-->PCE -- num_lfe=%lu\n",num_lfe);

                    audaac_extract_bits(data, AAC_NUM_ASSOC_DATA_ELEMENTS_SIZE,
                    &num_ade);
                    DEBUG_DETAIL("RAW-->PCE -- num_ade=%lu\n",num_ade);

                    audaac_extract_bits(data, AAC_NUM_VALID_CC_ELEMENTS_SIZE,
                    &num_vce);
                    DEBUG_DETAIL("RAW-->PCE -- num_vce=%lu\n",num_vce);

                    audaac_extract_bits(data, AAC_MONO_MIXDOWN_PRESENT_SIZE,
                    &temp);
                    if(temp) {
                        audaac_extract_bits(data, AAC_MONO_MIXDOWN_ELEMENT_SIZE,
                        &temp);
                    }

                    audaac_extract_bits(data, AAC_STEREO_MIXDOWN_PRESENT_SIZE,
                    &temp);
                    if(temp) {
                        audaac_extract_bits(data, AAC_STEREO_MIXDOWN_ELEMENT_SIZE,
                        &temp);
                    }

                    audaac_extract_bits(data, AAC_MATRIX_MIXDOWN_PRESENT_SIZE,
                    &temp);
                    if(temp) {
                        audaac_extract_bits(data, AAC_MATRIX_MIXDOWN_SIZE, &temp);
                    }

                    for(i=0; i<(num_fce+num_sce+num_bce); i++) {
                        /* Is Element CPE ?*/
                        audaac_extract_bits(data, 1, &temp);
                        /* If the element is CPE, increment num channels by 2 */
                        if(temp) num_ch += 2;
                        else     num_ch += 1;
                        DEBUG_DETAIL("RAW-->PCE -- num_ch=%lu\n",num_ch);
                        audaac_extract_bits(data, 4, &temp);
                    }
                    for(i=0; i<num_lfe; i++) {
                        /* LFE element can not be CPE */
                        num_ch += 1;
                        DEBUG_DETAIL("RAW-->PCE -- num_ch=%lu\n",num_ch);
                        audaac_extract_bits(data, AAC_LFE_SIZE, &temp);
                    }
                    for(i=0; i<num_ade; i++) {
                        audaac_extract_bits(data, AAC_ADE_SIZE, &temp);
                    }
                    for(i=0; i<num_vce; i++) {
                        /* Coupling channels can not be counted as output
                         channels as they will be
            coupled with main audio channels */
                        audaac_extract_bits(data, AAC_VCE_SIZE, &temp);
                    }
                    header->head.raw.channel_config = num_ch ;
                    /* byte_alignment() */
                    temp = (OMX_U8)(m_aac_hdr_bit_index % 8);
                    if(temp) {
                        m_aac_hdr_bit_index += 8 - temp;
                    }
                    /* get comment_field_bytes */
                    temp = data[m_aac_hdr_bit_index/8];
                    m_aac_hdr_bit_index += AAC_COMMENT_FIELD_BYTES_SIZE;
                    /* Skip the comment */
                    m_aac_hdr_bit_index += temp * AAC_COMMENT_FIELD_DATA_SIZE;
                    /* byte_alignment() */
                    temp = (OMX_U8)(m_aac_hdr_bit_index % 8);
                    if(temp) {
                        m_aac_hdr_bit_index += 8 - temp;
                    }
                    DEBUG_DETAIL("RAW-->PCE -- m_aac_hdr_bit_index=0x%x\n",\
                    m_aac_hdr_bit_index);
                }
                if (ext_flag)
                {
                    /* 17: ER_AAC_LC 19:ER_AAC_LTP
                    20:ER_AAC_SCALABLE 23:ER_AAC_LD */
                    if (header->head.raw.aud_obj_type == 17 ||
                            header->head.raw.aud_obj_type == 19 ||
                            header->head.raw.aud_obj_type == 20 ||
                            header->head.raw.aud_obj_type == 23)
                    {
                        audaac_extract_bits(data, 1, &temp);
                        audaac_extract_bits(data, 1, &temp);
                        audaac_extract_bits(data, 1, &temp);
                    }

                    /* extensionFlag3 */
                    audaac_extract_bits(data, 1, &temp);
                    if(temp) status = 1;
                }
            }

            /* SBR tool explicit signaling ( backward compatible ) */
            if (ext_aud_obj_type != 5)
            {
                /* syncExtensionType */
                audaac_extract_bits(data, 11, &temp);
                if (temp == 0x2b7)
                {
                    /*extensionAudioObjectType*/
                    audaac_extract_bits(data, 5, &ext_aud_obj_type);
                    if(ext_aud_obj_type == 5)
                    {
                        /*sbrPresentFlag*/
                        audaac_extract_bits(data, 1, &temp);
                        header->head.raw.sbr_present_flag = temp;
                        if(temp)
                        {
                            /*extensionSamplingFrequencyIndex*/
                            audaac_extract_bits(data,
                            AAC_SAMPLING_FREQ_INDEX_SIZE,
                            &temp);
                            header->head.raw.ext_freq_index = temp;
                            if(temp == 0xf)
                            {
                                /*Extract 24 bits for sampling frequency value */
                                audaac_extract_bits(data, 12, &temp);
                                audaac_extract_bits(data, 12, &temp);
                            }

                            /* syncExtensionType */
                            audaac_extract_bits(data, 11, &temp);

                            if (temp == 0x548)
                            {
                                /*psPresentFlag*/
                                audaac_extract_bits(data, 1, &temp);
                                header->head.raw.sbr_ps_present_flag = temp;
                                if(temp)
                                {
                                    /* extAudioObject is PS */
                                    ext_aud_obj_type = 29;
                                }
                            }
                        }
                    }
                    header->head.raw.ext_aud_obj_type = ext_aud_obj_type;
                }
            }
            DEBUG_PRINT("RAW-->ch cfg=0x%x\n",\
            header->head.raw.channel_config);
            DEBUG_PRINT("RAW-->freq_index=0x%x\n",\
            header->head.raw.freq_index);
            DEBUG_PRINT("RAW-->ext_freq_index=0x%x\n",\
            header->head.raw.ext_freq_index);
            DEBUG_PRINT("RAW-->aud_obj_type=0x%x\n",\
            header->head.raw.aud_obj_type);
            DEBUG_PRINT("RAW-->ext_aud_obj_type=0x%x\n",\
            header->head.raw.ext_aud_obj_type);
        }
    }
    return OMX_ErrorNone;
}

void COmxDecAacIn::audaac_extract_adif_header(
OMX_U8 *data,
struct aac_header *aac_header_info)
{
    DEBUG_PRINT("COmxDecAacIn::%s\n",__FUNCTION__);
    OMX_U32  buf8;
    OMX_U32 buf32;
    OMX_U32  num_pfe, num_fce, num_sce, num_bce, num_lfe, num_ade, num_vce;
    OMX_U32  pfe_index;
    OMX_U8  i;
    OMX_U32 temp;

    /* We already parsed the 'ADIF' keyword. Skip it. */
    m_aac_hdr_bit_index = 32;

    audaac_extract_bits(data, AAC_COPYRIGHT_PRESENT_SIZE, &buf8);

    if(buf8) {
        /* Copyright present; Just discard it for now */
        m_aac_hdr_bit_index += 72;
    }

    audaac_extract_bits(data, AAC_ORIGINAL_COPY_SIZE,
    &temp);

    audaac_extract_bits(data, AAC_HOME_SIZE, &temp);

    audaac_extract_bits(data, AAC_BITSTREAM_TYPE_SIZE,
       &temp);
    aac_header_info->head.adif.variable_bit_rate = temp;

    audaac_extract_bits(data, AAC_BITRATE_SIZE, &temp);

    audaac_extract_bits(data, AAC_NUM_PFE_SIZE, &num_pfe);

    for(pfe_index=0; pfe_index<num_pfe+1; pfe_index++) {
        if(!aac_header_info->head.adif.variable_bit_rate) {
            /* Discard */
            audaac_extract_bits(data, AAC_BUFFER_FULLNESS_SIZE, &buf32);
        }

        /* Extract Program Config Element */

        /* Discard element instance tag */
        audaac_extract_bits(data, AAC_ELEMENT_INSTANCE_TAG_SIZE, &buf8);

        audaac_extract_bits(data, AAC_PROFILE_SIZE,
        &buf8);
        aac_header_info->head.adif.aud_obj_type = buf8;

        buf8 = 0;
        audaac_extract_bits(data, AAC_SAMPLING_FREQ_INDEX_SIZE, &buf8);
        aac_header_info->head.adif.freq_index = buf8;
        aac_header_info->head.adif.sample_rate =
                                       aac_frequency_index[buf8];

        audaac_extract_bits(data, AAC_NUM_FRONT_CHANNEL_ELEMENTS_SIZE, &num_fce);

        audaac_extract_bits(data, AAC_NUM_SIDE_CHANNEL_ELEMENTS_SIZE, &num_sce);

        audaac_extract_bits(data, AAC_NUM_BACK_CHANNEL_ELEMENTS_SIZE, &num_bce);

        audaac_extract_bits(data, AAC_NUM_LFE_CHANNEL_ELEMENTS_SIZE, &num_lfe);

        audaac_extract_bits(data, AAC_NUM_ASSOC_DATA_ELEMENTS_SIZE, &num_ade);

        audaac_extract_bits(data, AAC_NUM_VALID_CC_ELEMENTS_SIZE, &num_vce);

        audaac_extract_bits(data, AAC_MONO_MIXDOWN_PRESENT_SIZE, &buf8);
        if(buf8) {
            audaac_extract_bits(data, AAC_MONO_MIXDOWN_ELEMENT_SIZE, &buf8);
        }

        audaac_extract_bits(data, AAC_STEREO_MIXDOWN_PRESENT_SIZE, &buf8);
        if(buf8) {
            audaac_extract_bits(data, AAC_STEREO_MIXDOWN_ELEMENT_SIZE, &buf8);
        }

        audaac_extract_bits(data, AAC_MATRIX_MIXDOWN_PRESENT_SIZE, &buf8);
        if(buf8) {
            audaac_extract_bits(data, AAC_MATRIX_MIXDOWN_SIZE, &buf8);
        }

        for(i=0; i<num_fce; i++) {
            audaac_extract_bits(data, AAC_FCE_SIZE, &buf8);
        }

        for(i=0; i<num_sce; i++) {
            audaac_extract_bits(data, AAC_SCE_SIZE, &buf8);
        }

        for(i=0; i<num_bce; i++) {
            audaac_extract_bits(data, AAC_BCE_SIZE, &buf8);
        }

        for(i=0; i<num_lfe; i++) {
            audaac_extract_bits(data, AAC_LFE_SIZE, &buf8);
        }

        for(i=0; i<num_ade; i++) {
            audaac_extract_bits(data, AAC_ADE_SIZE, &buf8);
        }

        for(i=0; i<num_vce; i++) {
            audaac_extract_bits(data, AAC_VCE_SIZE, &buf8);
        }

        /* byte_alignment() */
        buf8 = (OMX_U8)(m_aac_hdr_bit_index % 8);
        if(buf8) {
            m_aac_hdr_bit_index += 8 - buf8;
        }

        /* get comment_field_bytes */
        buf8 = data[m_aac_hdr_bit_index/8];

        m_aac_hdr_bit_index += AAC_COMMENT_FIELD_BYTES_SIZE;

        /* Skip the comment */
        m_aac_hdr_bit_index += buf8 * AAC_COMMENT_FIELD_DATA_SIZE;
    }

    /* byte_alignment() */
    buf8 = (OMX_U8)(m_aac_hdr_bit_index % 8);
    if(buf8) {
        m_aac_hdr_bit_index += 8 - buf8;
    }

    aac_header_info->head.adif.channel_config =
    (num_fce + num_sce + num_bce +
    num_lfe + num_ade + num_vce) ;
}

void COmxDecAacIn::audaac_extract_bits(
OMX_U8  *input,
OMX_U8  num_bits_reqd,
OMX_U32 *out_buf
)
{
    DEBUG_PRINT("COmxDecAacIn::%s\n",__FUNCTION__);
    OMX_U32 output = 0;
    OMX_U32 value = 0;
    OMX_U32 byte_index;
    OMX_U8   bit_index;
    OMX_U8   bits_avail_in_byte;
    OMX_U8   num_to_copy;
    OMX_U8   mask;
    OMX_U8   num_remaining = num_bits_reqd;

    while(num_remaining) {
        byte_index = m_aac_hdr_bit_index / 8;
        bit_index  = m_aac_hdr_bit_index % 8;

        bits_avail_in_byte = 8 - bit_index;
        num_to_copy = MIN(bits_avail_in_byte, num_remaining);

        mask = ~(0xff << bits_avail_in_byte);

        value = input[byte_index] & mask;
        value = value >> (bits_avail_in_byte - num_to_copy);

        m_aac_hdr_bit_index += num_to_copy;
        num_remaining -= num_to_copy;

        output = (output << num_to_copy) | value;
    }
    *out_buf = output;
}

int COmxDecAacIn::audaac_extract_loas_header(
OMX_U8 *data,OMX_U32 len,
struct aac_header *aac_header_info)
{
    DEBUG_PRINT("COmxDecAacIn::%s\n",__FUNCTION__);
    OMX_U32      aac_frame_length = 0;
    OMX_U32      value    = 0;
    OMX_U32       obj_type = 0;
    OMX_U32       ext_flag = 0;

    OMX_U32       num_fc   = 0;
    OMX_U32       num_sc   = 0;
    OMX_U32       num_bc   = 0;
    OMX_U32       num_lfe  = 0;
    OMX_U32       num_ade  = 0;
    OMX_U32       num_vce  = 0;

    OMX_U16      num_bits_to_skip = 0;
    DEBUG_PRINT("LOAS HEADER -->len=%u\n",(unsigned int)len);
    /* Length is in the bits 11 to 23 from left in the bitstream */
    m_aac_hdr_bit_index = 11;
    if(((m_aac_hdr_bit_index+42)) > len)
    return -1;

    audaac_extract_bits(data, 13, &aac_frame_length);

    /* useSameStreamMux */
    audaac_extract_bits(data, 1, &value);

    if(value)
    {
        return -1;
    }
    /* Audio mux version */
    audaac_extract_bits(data, 1, &value);
    if (value)
    {
        /* Unsupported format */
        return -1;
    }
    /* allStreamsSameTimeFraming */
    audaac_extract_bits(data, 1, &value);
    if (!value)
    {
        /* Unsupported format */
        return -1;
    }
    /* numSubFrames */
    audaac_extract_bits(data, 6, &value);
    if (value)
    {
        /* Unsupported format */
        return -1;
    }
    /* numProgram */
    audaac_extract_bits(data, 4, &value);
    if (value)
    {
        /* Unsupported format */
        return -1;
    }
    /* numLayer */
    audaac_extract_bits(data, 3, &value);
    if (value)
    {
        /* Unsupported format */
        return -1;
    }

    /* Audio specific config */
    /* audioObjectType */
    audaac_extract_bits(data, 5, &value);
    if (!LOAS_IS_AUD_OBJ_SUPPORTED(value))
    {
        /* Unsupported format */
        return -1;
    }
    aac_header_info->head.loas.aud_obj_type = value;

    /* SFI */
    audaac_extract_bits(data, 4, &value);
    if (!LOAS_IS_SFI_SUPPORTED(value))
    {
        /* Unsupported format */
        return -1;
    }
    aac_header_info->head.loas.freq_index = value;

    /* Channel config */
    audaac_extract_bits(data, 4, &value);
    if (!LOAS_IS_CHANNEL_CONFIG_SUPPORTED(value,
                aac_header_info->head.loas.aud_obj_type))
    {
        /* Unsupported format */
        return -1;
    }

    aac_header_info->head.loas.channel_config = value;

    if (aac_header_info->head.loas.aud_obj_type == 5)
    {
        /* Extension SFI */
        if((m_aac_hdr_bit_index+9) > len)
        return -1;
        audaac_extract_bits(data, 4, &value);
        if (!LOAS_IS_EXT_SFI_SUPPORTED(value))
        {
            /* Unsupported format */
            return -1;
        }
        /* Audio object_type */
        audaac_extract_bits(data, 5, &value);
        if (!LOAS_IS_AUD_OBJ_SUPPORTED(value))
        {
            /* Unsupported format */
            return -1;
        }
        aac_header_info->head.loas.aud_obj_type = value;
    }
    obj_type = aac_header_info->head.loas.aud_obj_type;

    if (LOAS_GA_SPECIFIC_CONFIG(obj_type))
    {
        if((m_aac_hdr_bit_index+3) > len)
        return -1;
        /* framelengthFlag */
        audaac_extract_bits(data, 1, &value);
        if (value)
        {
            /* Unsupported format */
            return -1;
        }

        /* dependsOnCoreCoder */
        audaac_extract_bits(data, 1, &value);
        if (value)
        {
            /* Unsupported format */
            return -1;
        }

        /* extensionFlag */
        audaac_extract_bits(data, 1, &ext_flag);
        if (!LOAS_IS_EXT_FLG_SUPPORTED(ext_flag,obj_type) )
        {
            /* Unsupported format */
            return -1;
        }
        if (!aac_header_info->head.loas.channel_config)
        {
            /* Skip 10 bits */
            if((m_aac_hdr_bit_index+45) > len)
            return -1;
            audaac_extract_bits(data, 10, &value);

            audaac_extract_bits(data, 4, &num_fc);
            audaac_extract_bits(data, 4, &num_sc);
            audaac_extract_bits(data, 4, &num_bc);
            audaac_extract_bits(data, 2, &num_lfe);
            audaac_extract_bits(data, 3, &num_ade);
            audaac_extract_bits(data, 4, &num_vce);

            /* mono_mixdown_present */
            audaac_extract_bits(data, 1, &value);
            if (value) {
                /* mono_mixdown_element_number */
                audaac_extract_bits(data, 4, &value);
            }

            /* stereo_mixdown_present */
            audaac_extract_bits(data, 1, &value);
            if (value) {
                /* stereo_mixdown_element_number */
                audaac_extract_bits(data, 4, &value);
            }

            /* matrix_mixdown_idx_present */
            audaac_extract_bits(data, 1, &value);
            if (value) {
                /* matrix_mixdown_idx and presudo_surround_enable */
                audaac_extract_bits(data, 3, &value);
            }
            num_bits_to_skip = (num_fc * 5) + (num_sc * 5) + (num_bc * 5) +
            (num_lfe * 4) + (num_ade * 4) + (num_vce * 5);

            if((m_aac_hdr_bit_index+num_bits_to_skip) > len)
            return -1;

            while (num_bits_to_skip != 0){
                if (num_bits_to_skip > 32) {
                    audaac_extract_bits(data, 32, &value);
                    num_bits_to_skip -= 32;
                } else {
                    audaac_extract_bits(data, num_bits_to_skip, &value);
                    num_bits_to_skip = 0;
                }
            }

            if (m_aac_hdr_bit_index & 0x07)
            {
                m_aac_hdr_bit_index +=  (8 - (m_aac_hdr_bit_index & 0x07));
            }
            if((m_aac_hdr_bit_index + 8) > len)
            return -1;
            /* comment_field_bytes */
            audaac_extract_bits(data, 8, &value);

            num_bits_to_skip = value * 8;

            if((m_aac_hdr_bit_index + num_bits_to_skip) > len)
            return -1;

            while (num_bits_to_skip != 0){
                if (num_bits_to_skip > 32) {
                    audaac_extract_bits(data, 32, &value);
                    num_bits_to_skip -= 32;
                } else {
                    audaac_extract_bits(data, num_bits_to_skip, &value);
                    num_bits_to_skip = 0;
                }
            }
        }

        if (ext_flag)
        {
            if (((obj_type == 17) || (obj_type == 19) ||
                        (obj_type == 20) || (obj_type == 23)))
            {
                if((m_aac_hdr_bit_index + 3) > len)
                return -1;
                audaac_extract_bits(data, 1, &value);
                audaac_extract_bits(data, 1, &value);
                audaac_extract_bits(data, 1, &value);
            }
            /* extensionFlag3 */
            if((m_aac_hdr_bit_index + 1) > len)
            return -1;
            audaac_extract_bits(data, 1, &value);
        }
    }
    if ((obj_type != 18) && (obj_type >= 17) && (obj_type <= 27))
    {
        /* epConfig */
        if((m_aac_hdr_bit_index + 2) > len)
        return -1;
        audaac_extract_bits(data, 2, &value);
        if (value)
        {
            /* Unsupported format */
            return -1;
        }
    }
    /* Back in StreamMuxConfig */
    /* framelengthType */
    if((m_aac_hdr_bit_index + 3) > len)
    return -1;
    audaac_extract_bits(data, 3, &value);
    if (value)
    {
        /* Unsupported format */
        return -1;
    }

    if((aac_header_info->head.loas.freq_index > 12) ||
            (aac_header_info->head.loas.channel_config > 2))
    {
        return -1;
    }
    else
    return 0;
}

COmxAacEventHandler::COmxAacEventHandler(COmxDecAac* base, int drv_fd,
OMX_BOOL suspensionPolicy,
OMX_STATETYPE state):
COmxBaseEventHandler(base, drv_fd, suspensionPolicy, state),
m_comp(base)
{
    DEBUG_PRINT("COmxAacEventHandler::%s\n",__FUNCTION__);

}
COmxAacEventHandler::~COmxAacEventHandler()
{
    DEBUG_PRINT("COmxAacEventHandler::%s\n",__FUNCTION__);
    DEBUG_PRINT("Destructor, COmxAacEventHandler...\n");
}
COmxDecAacIn_7x27::COmxDecAacIn_7x27( COmxDecAac * base, int fd,OMX_CALLBACKTYPE cb,
OMX_U32 buf_size, OMX_U32 sf, OMX_U8 ch,
OMX_BOOL pcm_feedback,
OMX_PTR appdata)
:COmxBaseDecIn(base,fd,cb,buf_size,sf,ch,pcm_feedback,appdata),
m_aac_comp(base),
m_first_aac_header(0),
m_aac_hdr_bit_index(0),
m_bytes_to_skip(0),
m_data_written_to_dsp(0),
m_silence(NULL),
m_pTranscoder(NULL),
m_bsac(OMX_FALSE),
bytesTranscoded(0)
{
    DEBUG_PRINT("COmxDecAacIn::%s fd=%d buf_size=%u sf=%u ch=%u pcm=%u\n",\
    __FUNCTION__,fd,(unsigned int)buf_size,(unsigned int)sf,
    (unsigned int)ch,(unsigned int)pcm_feedback);
	m_silence = new silenceInsertion;
}

COmxDecAacIn_7x27::~COmxDecAacIn_7x27()
{
    DEBUG_PRINT("COmxDecAacIn_7x27:%s\n",__FUNCTION__);
    m_first_aac_header=0;
    m_aac_hdr_bit_index = 0;
    m_aac_comp = NULL;
    m_bytes_to_skip = 0;
    m_data_written_to_dsp = 0;
    m_bsac = OMX_FALSE;
    bytesTranscoded = 0;
    memset(&m_aac_comp,0,sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
    if(m_pTranscoder)
    {
    delete m_pTranscoder;
    m_pTranscoder = NULL;
    }
    if(m_silence)
    {
        delete m_silence;
    }
}

COmxAacEventHandler_7x27::COmxAacEventHandler_7x27(COmxDecAac* base, int drv_fd,
OMX_BOOL suspensionPolicy,
OMX_STATETYPE state):
COmxBaseEventHandler(base, drv_fd, suspensionPolicy, state),
m_comp(base)
{
    DEBUG_PRINT("COmxAacEventHandler_7x27::%s\n",__FUNCTION__);

}
COmxAacEventHandler_7x27::~COmxAacEventHandler_7x27()
{
    DEBUG_PRINT("COmxAacEventHandler_7x27::%s\n",__FUNCTION__);
    DEBUG_PRINT("Destructor, COmxAacEventHandler...\n");
}
OMX_ERRORTYPE COmxAacEventHandler::processEvents()
{
    struct msm_audio_event       dsp_event;
    struct msm_audio_bitstream_info stream_info;
    struct msm_audio_bitstream_error_info error_info;
    int rc = 0;
    DEBUG_PRINT("COmxAacEventHandler::%s\n",__FUNCTION__);
    while(1)
    {
        DEBUG_PRINT("PE:COmxAacEventHandler Waiting for AUDIO_GET_EVENT\n");
        DEBUG_PRINT("COmxAacEventHandler::%s\n",__FUNCTION__);
        rc = ioctl(get_drv_fd(),AUDIO_GET_EVENT,&dsp_event);
        DEBUG_PRINT("PE:Event Thread %d errno=%d, dsp_event %d datawritten=%d",\
         rc,errno, dsp_event.event_type,(get_comp()->getFirstBufSentToDSPFlg()));
        if ((rc == -1))
        {
            DEBUG_PRINT("PE:Event Thread exiting %d",rc);
            return OMX_ErrorUndefined;
        }

        OMX_STATETYPE state = get_state();
        switch ( dsp_event.event_type )
        {
        case AUDIO_EVENT_STREAM_INFO:
            {
               if(!get_comp()->get_pcm_feedback())
                {
                    DEBUG_PRINT("PE:Rxed StreamInfo for tunnel mode\n");
                    break;
                }
                DEBUG_PRINT("PE: Recieved AUDIO_EVENT_STREAM_INFO");
                ioctl(get_drv_fd(), AUDIO_GET_STREAM_INFO,&stream_info);

                struct msm_audio_bitstream_info *pStrm =
                    (struct msm_audio_bitstream_info*)malloc(sizeof(struct msm_audio_bitstream_info));
                memcpy(pStrm,&stream_info,sizeof(struct msm_audio_bitstream_info));

                // command thread needs to free the memory allocated here.
                get_comp()->post_command(0,(unsigned int)pStrm,OMX_COMPONENT_STREAM_INFO);


                break;
            }

        case AUDIO_EVENT_BITSTREAM_ERROR_INFO:
            {
                DEBUG_PRINT("PE: Rxed AUDIO_ERROR_INFO...\n");
                if(!get_comp()->get_pcm_feedback())
                {
                    DEBUG_PRINT("PE:Rxed StreamInfo for tunnel mode\n");
                    break;
                }
                ioctl(get_drv_fd(),AUDIO_GET_BITSTREAM_ERROR_INFO,&error_info);
                DEBUG_PRINT("DEC ID = %d\n",error_info.dec_id);
                DEBUG_PRINT("ERR INDICATOR = %d\n",error_info.err_msg_indicator);
                DEBUG_PRINT("DEC TYPE = %d\n",error_info.err_type);
                if(!get_comp()->get_port_recfg())
                {
                    get_comp()->set_port_recfg(1);
                    get_comp()->out_th_wakeup();
                }
                break;
             }

        case AUDIO_EVENT_SUSPEND:
            {
                if((get_comp()->getSuspendFlg() && get_comp()->getResumeFlg())
                   || (!get_comp()->get_pcm_feedback()))
                {
                    DEBUG_PRINT("PE:Ignoring Event Already in "
                                "Suspended state[%d] suspension_policy[%d] "
                                "event[%d] suspendflg[%d] resumeflg[%d]\n",
                                state, getSuspensionPolicy(),
                                dsp_event.event_type,
                                get_comp()->getSuspendFlg(),
                                get_comp()->getResumeFlg());
                    // Ignore events if not Pause state;
                    continue;
                }

                if((state != OMX_StatePause) ||
                        (getSuspensionPolicy() != OMX_TRUE))
                {
                    DEBUG_PRINT("PE:Ignoring Suspend Event state[%d] "
                                "suspension_policy[%d] event[%d] ",\
                                state, getSuspensionPolicy(),
                                dsp_event.event_type);
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
                    get_comp()->post_command(0,0,OMX_COMPONENT_SUSPEND);
                }

            }
            break;

        case AUDIO_EVENT_RESUME:
            {
                if((get_comp()->getSuspendFlg() && get_comp()->getResumeFlg())
                   || !(get_comp()->get_pcm_feedback()))
                {
                    DEBUG_PRINT("PE:Ignoring Event Comp Already suspended [%d] "
                    "suspension_policy[%d]event[%d]suspendflg[%d]"
                    "resumeflg[%d]\n",\
                    state, getSuspensionPolicy(),
                    dsp_event.event_type,get_comp()->getSuspendFlg(),
                    get_comp()->getResumeFlg());
                    // Ignore events if not Pause state;
                    continue;
                }
                if((state != OMX_StatePause) ||
                        (getSuspensionPolicy() != OMX_TRUE))
                {
                    DEBUG_PRINT("PE:Ignoring Resume Event state[%d] "
                                "suspension_policy[%d] event[%d] ",\
                                state, getSuspensionPolicy(),
                                dsp_event.event_type);
                    // Ignore events if not Pause state;
                    continue;
                }
                else if ( get_comp()->getSuspendFlg() &&
                         !(get_comp()->getResumeFlg() ))
                {
                    DEBUG_PRINT("PE: Recieved AUDIO_EVENT_RESUME");
                    // signal the output thread that RESUME has happened
                    get_comp()->setResumeFlg();
                    get_comp()->post_command(0,0,OMX_COMPONENT_RESUME);
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
OMX_ERRORTYPE COmxAacEventHandler_7x27::processEvents()
{
    struct msm_audio_event       dsp_event;
    struct msm_audio_bitstream_info stream_info;
    int rc = 0;
    OMX_AUDIO_PARAM_AACPROFILETYPE aac_param;

    DEBUG_PRINT("COmxAacEventHandler_7x27::%s\n",__FUNCTION__);
    while(1)
    {
        DEBUG_PRINT("PE:COmxAacEventHandler Waiting for AUDIO_GET_EVENT\n");
        DEBUG_PRINT("COmxAacEventHandler_7x27::%s\n",__FUNCTION__);
        rc = ioctl(get_drv_fd(),AUDIO_GET_EVENT,&dsp_event);
        DEBUG_PRINT("PE:Event Thread %d errno=%d, dsp_event %d datawritten=%d",\
         rc,errno, dsp_event.event_type,(get_comp()->getFirstBufSentToDSPFlg()));
        if ((rc == -1))
        {
            DEBUG_PRINT("PE:Event Thread exiting %d",rc);
            return OMX_ErrorUndefined;
        }

        OMX_STATETYPE state = get_state();
        switch ( dsp_event.event_type )
        {
        case AUDIO_EVENT_STREAM_INFO:
            {
                if(get_comp()->getFirstBufSentToDSPFlg())
                {
                    get_comp()->setFirstBufSentToDSPFlg(false);
                    DEBUG_PRINT("PE: Recieved AUDIO_EVENT_STREAM_INFO");

                    ioctl(get_drv_fd(), AUDIO_GET_STREAM_INFO,&stream_info);

                    DEBUG_PRINT("audio_stream_info: chanInfo=%d, sampleRate=%d "
                    "codec_type = %d bit_stream_info = %d\n",
                    stream_info.chan_info, stream_info.sample_rate,
                    stream_info.codec_type, stream_info.bit_stream_info);

                    memcpy(&aac_param, &(get_comp()->get_aac_param()),\
                    sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

                    if(stream_info.sample_rate != 0)
                        aac_param.nSampleRate = stream_info.sample_rate;
                    if((stream_info.chan_info & 0x0F) != 0)
                        aac_param.nChannels   = (stream_info.chan_info & 0x0F);
                    aac_param.nBitRate    = stream_info.bit_rate;
                    aac_param.nFrameLength= OMX_ADEC_AAC_FRAME_LEN;
                    DEBUG_PRINT("PE: final sf %d final ch %d\n", aac_param.nSampleRate, aac_param.nChannels);
                    if ((aac_param.nSampleRate == 0) ||
                         (aac_param.nChannels == 0))
                    {
                        DEBUG_PRINT("PE:AUDIO_EVENT_STREAM_INFO: invalid Params !!\n");
                        get_comp()->post_command((unsigned)OMX_CommandStateSet,
                        (unsigned)OMX_StateInvalid,
                        OMX_COMPONENT_GENERATE_COMMAND);
                        get_comp()->post_command(OMX_CommandFlush,-1,
                                                 OMX_COMPONENT_GENERATE_COMMAND);

                        return OMX_ErrorInvalidState;
                    }
                    /* If SBR/PS present double the sampling rate */
                    if ( stream_info.bit_stream_info == 1 ||
                            stream_info.bit_stream_info == 2 )
                    {
                        /* PCM driver can support sampling rates upto 48000 only */
                        if(aac_param.nSampleRate <= 24000)
			{
                            aac_param.nSampleRate *= 2;
                            aac_param.nFrameLength *= 2;
			}	 
                    }
                    /* If PS present double the number of channels */
                    if (( stream_info.bit_stream_info == 2) &&
                         (stream_info.chan_info == 1 ))
                    {
                        aac_param.nChannels *= 2;
                    }
                    if(aac_param.nSampleRate > 48000)
                        aac_param.nSampleRate = 48000;
                    if(aac_param.nChannels > 2)
                        aac_param.nChannels = 2;

                    struct msm_audio_aac_config aac_config;
                    if ( ioctl(get_drv_fd(), AUDIO_GET_AAC_CONFIG, &aac_config) == -1 )
                    {
                        DEBUG_PRINT("omx_aac_adec::set_aac_config():GET_AAC_CONFIG failed");
                        get_comp()->post_command((unsigned)OMX_CommandStateSet,
                        (unsigned)OMX_StateInvalid,
                        OMX_COMPONENT_GENERATE_COMMAND);
                    }

                    /* Stop Audio decoding before configuring the DSP */
                    if ( ioctl(get_drv_fd(), AUDIO_STOP, 0) == -1 )
                    {
                        DEBUG_PRINT("PE:AUDIO_STOP failed %d\n", errno);
                    }
                    get_comp()->set_dec_state(false);
                    usleep(5000);
                    /* If the first stream information event returns bitstream
                information as AAC, disable SBR and PS flags and reconfigure DSP */
                    if(stream_info.bit_stream_info == 0)
                    {
                        aac_config.sbr_on_flag    = 0;
                        aac_config.sbr_ps_on_flag = 0;
                    }
                    /* If the first stream information event returns bitstream
                information as SBR, disable PS flag and reconfigure DSP */
                    else if(stream_info.bit_stream_info == 1)
                    {
                        aac_config.sbr_ps_on_flag = 0;
                    }

                    if ( ioctl(get_drv_fd(), AUDIO_SET_AAC_CONFIG, &aac_config) )
                    {
                        DEBUG_PRINT("set_aac_config():AUDIO_SET_AAC_CONFIG failed");
                        get_comp()->post_command((unsigned)OMX_CommandStateSet,
                        (unsigned)OMX_StateInvalid,
                        OMX_COMPONENT_GENERATE_COMMAND);
                    }

                    /* Restart Audio decoding after configuring the DSP */
                    if ( ioctl(get_drv_fd(), AUDIO_START, 0) < 0 )
                    {
                        DEBUG_PRINT("PE:AUDIO_START failed %d\n", errno);
                        get_comp()->post_command((unsigned)OMX_CommandStateSet,
                        (unsigned)OMX_StateInvalid,
                        OMX_COMPONENT_GENERATE_COMMAND);
                        get_comp()->post_command(OMX_CommandFlush,-1,
                                                 OMX_COMPONENT_GENERATE_COMMAND);

                        get_comp()->set_dec_state(false);
                        return OMX_ErrorInvalidState;
                    }
                    get_comp()->set_dec_state(true);
                    // Wake up the input thread only, output thread
                    // will be waken up during detection of port settings;

                    get_comp()->set_aac_param(&aac_param);
              get_comp()->in_th_timedwakeup();

                    break;
                }
                else
                break;
            }

        case AUDIO_EVENT_SUSPEND:
            {
                if((get_comp()->getSuspendFlg() && get_comp()->getResumeFlg())
                   || (!get_comp()->get_pcm_feedback()))
                {
                    DEBUG_PRINT("PE:Ignoring Event Already in "
                                "Suspended state[%d] suspension_policy[%d] "
                                "event[%d] suspendflg[%d] resumeflg[%d]\n",
                                state, getSuspensionPolicy(),
                                dsp_event.event_type,
                                get_comp()->getSuspendFlg(),
                                get_comp()->getResumeFlg());
                    // Ignore events if not Pause state;
                    continue;
                }

                if((state != OMX_StatePause) ||
                        (getSuspensionPolicy() != OMX_TRUE))
                {
                    DEBUG_PRINT("PE:Ignoring Suspend Event state[%d] "
                                "suspension_policy[%d] event[%d] ",\
                                state, getSuspensionPolicy(),
                                dsp_event.event_type);
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
                    get_comp()->post_command(0,0,OMX_COMPONENT_SUSPEND);
                }

            }
            break;

        case AUDIO_EVENT_RESUME:
            {
                if((get_comp()->getSuspendFlg() && get_comp()->getResumeFlg())
                   || !(get_comp()->get_pcm_feedback()))
                {
                    DEBUG_PRINT("PE:Ignoring Event Comp Already suspended [%d] "
                    "suspension_policy[%d]event[%d]suspendflg[%d]"
                    "resumeflg[%d]\n",\
                    state, getSuspensionPolicy(),
                    dsp_event.event_type,get_comp()->getSuspendFlg(),
                    get_comp()->getResumeFlg());
                    // Ignore events if not Pause state;
                    continue;
                }
                if((state != OMX_StatePause) ||
                        (getSuspensionPolicy() != OMX_TRUE))
                {
                    DEBUG_PRINT("PE:Ignoring Resume Event state[%d] "
                                "suspension_policy[%d] event[%d] ",\
                                state, getSuspensionPolicy(),
                                dsp_event.event_type);
                    // Ignore events if not Pause state;
                    continue;
                }
                else if ( get_comp()->getSuspendFlg() &&
                         !(get_comp()->getResumeFlg() ))
                {
                    DEBUG_PRINT("PE: Recieved AUDIO_EVENT_RESUME");
                    // signal the output thread that RESUME has happened
                    get_comp()->setResumeFlg();
                    get_comp()->post_command(0,0,OMX_COMPONENT_RESUME);
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


OMX_BOOL silenceInsertion::checkForTimeStampGap( OMX_U32 SF,OMX_U32 frameLen)
{
    OMX_TICKS TimeGap;

    m_milliSecPerFrame =((frameLen * 1000*1000)/ SF);  // samplesPerFrame * 1000*1000 / sampleRate, 1000*1000 is used as the time base is microseconds 
    DEBUG_PRINT("CheckForSilenceInsertion: frameLen %lu sampleRate %lu iOutputMilliSecPerFrame %d \n",
    			 frameLen, SF, m_milliSecPerFrame);

    m_silenceInsertionRequired = OMX_FALSE;
    TimeGap = m_currentFrameTimestamp - m_previousFrameTimestamp;
    DEBUG_PRINT(" omx_aac_adec::CheckForSilenceInsertion: "
                "CurrentFrameTimestamp %lld PreviousFrameTimestamp %lld"
                "TimestampGap %lld\n",\
                (m_currentFrameTimestamp),
                (m_previousFrameTimestamp),TimeGap);
    if ((TimeGap > TIMESTAMP_HALFRANGE_THRESHOLD) || 
        (TimeGap <= m_milliSecPerFrame ))
    {
        DEBUG_PRINT("omx_aac_adec::CheckForSilenceInsertion - "
                    "No need to insert silence");
        return m_silenceInsertionRequired;
    }

    m_silenceInsertionRequired = OMX_TRUE;
    //Determine the number of silence frames to insert
    m_numSilenceFrames = (TimeGap - m_milliSecPerFrame) / (m_milliSecPerFrame);
    DEBUG_PRINT(" omx_aac_adec::CheckForSilenceInsertion: "
                "SilenceFramesNeeded %d\n",m_numSilenceFrames);
    return m_silenceInsertionRequired;

}

OMX_ERRORTYPE  silenceInsertion::insertSilenceInBuffer(OMX_U8 *buffer, 
                                                       OMX_U32 frameLen, 
                                                       OMX_U8 numOfCh)
{
    OMX_U8  *SilenceInputBuffer = buffer;
    OMX_U32 SilenceFrameLength = 0;
    META_IN meta_in;
    PSEUDO_RAW pseudo_raw;
    OMX_U16 fr_len =0 ;OMX_U16 temp=0;

    m_currentFrameTimestamp = m_previousFrameTimestamp ;

    // Setup the input side for silence frame
    // copy the metadata info from the BufHdr and insert to payload
    meta_in.offsetVal  = sizeof(META_IN);
    meta_in.nTimeStamp =
    (m_milliSecPerFrame + m_currentFrameTimestamp) *1000;
    meta_in.nFlags     = 0;
    memcpy(SilenceInputBuffer,&meta_in, meta_in.offsetVal);
    m_currentFrameTimestamp = (m_milliSecPerFrame + m_currentFrameTimestamp);
    DEBUG_PRINT(" omx_aac_adec::DoSilenceInsertion: "
                "CurrentFrameTimestamp %lld PreviousFrameTimestamp %lld \n",
                (m_currentFrameTimestamp),
                (m_previousFrameTimestamp));
    m_previousFrameTimestamp = m_currentFrameTimestamp;
    m_numSilenceFrames--;

    DEBUG_PRINT("DoSilenceInsertion OUT: "
                "CurrentFrameTimestamp %lld PreviousFrameTimestamp %lld \n",
                (m_currentFrameTimestamp),
                (m_previousFrameTimestamp));

    pseudo_raw.sync_word = 0xFFFF;
    // Number of bytes to be inserted as silence
    SilenceFrameLength = frameLen * numOfCh * 2;
    // Set MSB of length field to indicate DSP to insert silence of requested bytes
	  SilenceFrameLength = 0x8000 | SilenceFrameLength;
    fr_len = (SilenceFrameLength & 0xFF);
    fr_len = fr_len << 8;
    temp = (SilenceFrameLength & 0xFF00);
    temp = temp >> 8;
    fr_len |= temp;
    pseudo_raw.fr_len  = fr_len;
    memcpy(&SilenceInputBuffer[meta_in.offsetVal],&pseudo_raw,sizeof(PSEUDO_RAW));

    return OMX_ErrorNone;
}

silenceInsertion::~silenceInsertion()
{
    DEBUG_PRINT("%s...\n",__func__);
    m_silenceInsertionRequired = OMX_FALSE;
    m_previousFrameTimestamp = 0;
    m_currentFrameTimestamp = 0;
    m_numSilenceFrames = 0;
    m_milliSecPerFrame =0;
}

OMX_ERRORTYPE COmxDecAacIn::process_aac(
                                OMX_BUFFERHEADERTYPE  *buffer,
                                OMX_U8                *pDest,
                                OMX_U32               &len,
                                META_IN               &meta_in, bool adif)
{
    PSEUDO_RAW pseudo_raw;
    OMX_U16 fr_len =0 ;OMX_U16 temp=0;
    pDest = get_tmp_meta_buf();
    OMX_U32 iLen = sizeof(META_IN);
    (void)meta_in;
    DEBUG_PRINT("FILLED LENGHT-----------> %lu\n",\
               ((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen);
    if((buffer->nFlags & OMX_BUFFERFLAG_EOS) && (buffer->nFilledLen == 0))
    {
        len = 0;
        return OMX_ErrorNone;
    }

    if((OMX_AUDIO_AACStreamFormatRAW == m_aac_param.eAACStreamFormat))
    {
        pseudo_raw.sync_word = 0xFFFF;
        fr_len = (((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen) & 0xFF ;
        fr_len = fr_len << 8;
        temp = ((((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen) & 0xFF00);
        temp = temp >> 8;
        fr_len |= temp;
        pseudo_raw.fr_len    = fr_len;
        memcpy(&pDest[iLen],&pseudo_raw,sizeof(PSEUDO_RAW));
        iLen +=  sizeof(PSEUDO_RAW);
    }
    if (adif)
    {
        memcpy(&pDest[iLen],(((OMX_BUFFERHEADERTYPE*)buffer)->pBuffer + (m_aac_hdr_bit_index/8)),
                              (((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen - (m_aac_hdr_bit_index/8)));
        iLen += (((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen - (m_aac_hdr_bit_index/8));

        COmxBaseIn::m_in_pb_stat.tot_in_buf_len += (buffer->nFilledLen - (m_aac_hdr_bit_index/8));
    }
    else if(m_bytes_to_skip)
    {
        memcpy(&pDest[iLen],(((OMX_BUFFERHEADERTYPE*)buffer)->pBuffer + m_bytes_to_skip),
                              (((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen - m_bytes_to_skip));

        iLen += (((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen - m_bytes_to_skip);
        COmxBaseIn::m_in_pb_stat.tot_in_buf_len += (buffer->nFilledLen - m_bytes_to_skip );
        m_bytes_to_skip = 0;
    }
    else
    {
        memcpy(&pDest[iLen],((OMX_BUFFERHEADERTYPE*)buffer)->pBuffer,
                          ((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen);
        iLen += ((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen;

        COmxBaseIn::m_in_pb_stat.tot_in_buf_len += buffer->nFilledLen;
    }
    // Ignore the len of META_IN for now; If NT mode, len will be incremented by 14 after
    // this function returns; if tunnel mode ignore the size of META_IN
    // and advance the data pointer by size of META_IN
    len = iLen - sizeof(META_IN);

    return OMX_ErrorNone;
}

void COmxDecAac::process_command_msg( unsigned char id1)
{
    unsigned     p1;         // Parameter - 1
    unsigned     p2;         // Parameter - 2
    unsigned     ident = 0;
    unsigned     qsize  = 0;
    unsigned     char id=0;
    omx_cmd_queue *cmd_q  = get_cmd_q();
    qsize = get_q_size(cmd_q);
    (void)id1;

    DEBUG_PRINT("COmxAac::%s qsize[%d] state[%d]",__FUNCTION__,
                                               qsize, get_state());
    if(!qsize )
    {
        DEBUG_PRINT("CMD-->BREAKING FROM LOOP\n");
        return;
    }
    get_msg(cmd_q,&p1,&p2,&id);

    ident = id;
    DEBUG_PRINT("::%s->state[%d]ident[%d]cmdq[%d] \n",__FUNCTION__,
    get_state(),ident, qsize);
    switch(ident)
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
                        //DEBUG_PRINT("State transition, wake up in and out th "
                        //            "%d %d\n",is_in_th_sleep,is_out_th_sleep);
                        in_th_wakeup();
                        out_th_wakeup();

                        if((get_state() == OMX_StateExecuting))
                            set_is_pause_to_exe(false);

                    }
                }
                if(get_state() == OMX_StateInvalid)
                {
                    get_cb().EventHandler(&m_cmp,
                    get_app_data(),
                    OMX_EventError,
                    OMX_ErrorInvalidState,
                    0, NULL );
                }
                else
                {
                    get_cb().EventHandler(&m_cmp,
                    get_app_data(),
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
DEBUG_PRINT("OMX_COMPONENT_GENERATE_COMMAND p1=%u p2=%u\n",p1,p2);
            process_cmd(&m_cmp,
            (OMX_COMMANDTYPE)p1,
            (OMX_U32)p2,(OMX_PTR)NULL);
            break;
        }

    case  OMX_COMPONENT_PORTSETTINGS_CHANGED:
        {
            DEBUG_DETAIL("CMD-->RXED PORTSETTINGS_CHANGED");
            get_cb().EventHandler(&m_cmp,
            get_app_data(),
            OMX_EventPortSettingsChanged,
            1, 0, NULL );
            break;
        }

    case OMX_COMPONENT_SUSPEND:
        {
            setSuspendFlg();

            if(!get_dec_state())
            {
                DEBUG_PRINT("DONT PROCESS SUSPEND EVENT, PLAYBACK NOT STARTED\n");
                break;
            }
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
            break;
        }
    case OMX_COMPONENT_STREAM_INFO:
        {
            struct msm_audio_bitstream_info *pStreamInfo = (struct msm_audio_bitstream_info *)p2;
            if(get_eos_bm())
            {
                trigger_eos(OMX_TRUE);
                DEBUG_PRINT_ERROR("CMD:StreamInfo reject, EOS in progress\n");
                free(pStreamInfo);
                break;
            }

            OMX_U32 sr = pStreamInfo->sample_rate;
            OMX_U32 ch = (pStreamInfo->chan_info & 0x0F);
            DEBUG_PRINT("CMD:StreamInfo-->cur SF[%d]New SF[%d] "
                        " cur ch=%d New ch=%d sbr-ps[%d]"
                        " cfg ch=%d sf=%d\n",
                            (unsigned int)m_aac_param.nSampleRate,(unsigned int)sr,
                            (unsigned int)m_aac_param.nChannels,(unsigned int)ch,
                            (unsigned int)pStreamInfo->bit_stream_info,
                            get_ch(),get_sf());
            m_aac_param.nFrameLength= OMX_ADEC_AAC_FRAME_LEN;
            /* If SBR/PS present double the sampling rate */
            if ( (pStreamInfo->bit_stream_info == 1) ||
                 (pStreamInfo->bit_stream_info == 2) )
            {
                /* PCM driver can support sampling rates upto 48000 only */
                if(sr<= 24000)
		{
                    sr *= 2;
                    m_aac_param.nFrameLength *= 2;
		}
            }
            /* If PS present double the number of channels */
            if ( (pStreamInfo->bit_stream_info == 2) &&
                 (pStreamInfo->chan_info == 1 ))
            {
                ch *= 2;
            }

            free(pStreamInfo);
            if(ch > 2)
            {
                ch = 2;
                DEBUG_PRINT("CMD-->Forced channel setting change...\n");
            }
            if(sr > 48000)
            {
                sr = 48000;
                DEBUG_PRINT("CMD-->Forced frequency setting change...\n");
            }
            if(get_eos_bm() || get_flush_stat())
            {
                DEBUG_PRINT("FLUSH/EOS IN PROGRESS...%d %d\n",
                                        get_eos_bm(),get_flush_stat());
                break;

            }
            // if rxed content is different from existing one, trigger port reconfiguration

            if((get_sf() != sr ) ||
               (get_ch() != ch ))
            {
                m_aac_param.nSampleRate = sr;
                m_aac_param.nChannels = ch;
                DEBUG_DETAIL("CMD-->Trigger OMX_EventPortSettingsChanged");
                get_cb().EventHandler(&m_cmp,
                                 get_app_data(),
                                 OMX_EventPortSettingsChanged,
                                 1, 0, NULL );
            }
            else
            {
                // else just wake up output threads
                set_out_bEnabled(OMX_TRUE);
                set_port_recfg(1);
                ioctl( get_drv_fd(), AUDIO_OUTPORT_FLUSH, 0);

                DEBUG_PRINT("CMD--> No OMX_EventPortSettingsChanged");
                DEBUG_DETAIL("CMD:WAKING UP OUT THREADS\n");
                out_th_wakeup();
            }
            set_sf(sr);
            set_ch((OMX_U8)ch);
            break;
        }
    default:
        {
            DEBUG_PRINT_ERROR("CMD->state[%d]id[%d]\n",get_state(),ident);
        }
    }
    return;
}
void COmxDecAacIn_7x27::process_in_port_msg(unsigned char id)
{
    unsigned      p1;       // Parameter - 1
    unsigned      p2;       // Parameter - 2
    unsigned      char ident;
    unsigned      qsize     = 0;
    unsigned      tot_qsize = 0;
    OMX_STATETYPE state;

    DEBUG_PRINT("COmxDecAacIn_7x27::%s id=%d\n",__FUNCTION__,id);
    loopback_in:
    OMX_U32       tcxo_space = get_comp()->getFilledSpaceInTcxoBuf();
    state = get_state();

    if(state == OMX_StateLoaded)
    {
        DEBUG_PRINT(" IN: IN LOADED STATE RETURN\n");
        return;
    }
    // Protect the shared queue data structure

    omx_cmd_queue *cmd_q  = get_cmd_q();
    omx_cmd_queue *bd_q  = get_bd_q();
    omx_cmd_queue *data_q = get_data_q();
    qsize = get_q_size(cmd_q);
    tot_qsize = qsize;
    tot_qsize += get_q_size(bd_q);
    tot_qsize += get_q_size(data_q);

    if ( 0 == tot_qsize )
    {
        DEBUG_DETAIL("IN-->BREAKING FROM IN LOOP");
        return;
    }

    if ((state != OMX_StateExecuting) && !(qsize))
    {
        DEBUG_DETAIL("SLEEPING IN THREAD\n");
        get_comp()->in_th_sleep();
        goto loopback_in;
    }
    else if ((state == OMX_StatePause))
    {
        if(!(qsize = get_q_size(cmd_q)))
        {
            DEBUG_DETAIL("IN: SLEEPING IN THREAD\n");
            get_comp()->in_th_sleep();

            goto loopback_in;
        }
    }
    DEBUG_DETAIL("Input-->QSIZE-flush=%d,ebd=%d QSIZE=%d state=%d tcxo[%lu]\n",\
    get_q_size(cmd_q),
    get_q_size(bd_q),
    get_q_size(data_q),state,tcxo_space);
    p1=0;p2=0;ident=0;
    if(qsize)
    {
        // process FLUSH message
        get_msg(cmd_q, &p1,&p2,&ident);
    }
    else if((qsize = get_q_size(bd_q)) &&
            (state == OMX_StateExecuting))
    {
        get_msg(bd_q, &p1,&p2,&ident);
    }
    else if((qsize = get_q_size(data_q)) &&
            (state == OMX_StateExecuting) && !tcxo_space)
    {
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
        DEBUG_PRINT("IN--> Empty Queue state=%d %d %d %d tcxoFilled[%lu]\n",\
        state,\
        get_q_size(cmd_q),
        get_q_size(bd_q),
        get_q_size(data_q),tcxo_space);

        if(get_state() == OMX_StatePause || tcxo_space)
        {
            DEBUG_DETAIL("IN: SLEEPING AGAIN IN THREAD\n");
            get_comp()->in_th_sleep();
            goto loopback_in;
        }
    }

    if(qsize > 0)
    {
        id = ident;
        DEBUG_DETAIL("Input->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d]\n",\
        get_state(),
        ident,
        get_q_size(cmd_q),
        get_q_size(bd_q),
        get_q_size(data_q));

        if(id == OMX_COMPONENT_GENERATE_BUFFER_DONE)
        {
            buffer_done_cb((OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_ETB)
        {
            process_etb((OMX_HANDLETYPE)p1,
            (OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_COMMAND)
        {
            // Execute FLUSH command
            if(p1 == OMX_CommandFlush)
            {
                DEBUG_DETAIL(" Executing FLUSH command on Input port\n");
                execute_input_omx_flush();
            }
            else
            {
                DEBUG_DETAIL("Invalid command[%d]\n",p1);
            }
        }
        else if(id == OMX_COMPONENT_SUSPEND)
        {
            DEBUG_PRINT("IN:FAKING EOS TO KERNEL : m_eos_bm=%d", \
            get_comp()->get_eos_bm());
            // dont trigger fake EOS when actual EOS has already been sent
            if(!(get_comp()->get_eos_bm() & IP_OP_PORT_BITMASK))
            {
                omx_fake_eos();
            }
            else if((get_comp()->get_eos_bm() & IP_OP_PORT_BITMASK) ==
                    IP_OP_PORT_BITMASK)
            {
                get_comp()->setSuspendFlg();
                get_comp()->setResumeFlg();
                //EOS already reached, dont trigger suspend/resume,
                // but set the flag so that one more suspend/event doesnt
                // get triggered from timeout/driver
                DEBUG_PRINT("IN--> AUDIO_STOP %d %d \n",
                get_comp()->getSuspendFlg(),
                get_comp()->getResumeFlg());
                ioctl(get_drv_fd(), AUDIO_STOP, 0);
                get_comp()->set_dec_state(false);
                if (get_comp()->getWaitForSuspendCmplFlg())
                {
                    DEBUG_PRINT("Release P-->Executing context to IL client.\n");
                    get_comp()->release_wait_for_suspend();
                }
            }
            else
            {
                // suspend event after input eos sent
                DEBUG_PRINT("IN--> Do nothing, m_eos_bm=%d\n",
                                                     get_comp()->get_eos_bm());
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("ERROR:IN-->Invalid Id[%d]\n",id);
        }
    }
    else
    {
        DEBUG_DETAIL("ERROR:IN-->Empty INPUT Q\n");
    }
    return;
}
OMX_ERRORTYPE  COmxDecAacIn_7x27::process_etb(
OMX_IN OMX_HANDLETYPE         hComp,
OMX_BUFFERHEADERTYPE* buffer)
{
    struct aac_header aac_header_info;
    int res = 0;
    struct msm_audio_config drv_config;
    OMX_STATETYPE state;
    META_IN meta_in;

    DEBUG_PRINT("COmxDecAacIn_7x27::%s\n",__FUNCTION__);

    /* Assume empty this buffer function has already checked
    validity of buffer */

    bool flg= false;
    bool adif = false;
    m_bytes_to_skip = 0;
    PrintFrameHdr(OMX_COMPONENT_GENERATE_ETB,buffer);
    DEBUG_PRINT("ETBP:aac_header=%d m_aac_comp=%p\n",m_first_aac_header,m_aac_comp);
    if(m_aac_comp->get_eos_bm())
    {
        m_aac_comp->set_eos_bm(0);
    }

    memcpy(&m_aac_param, &(m_aac_comp->get_aac_param()),\
    sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

    if(m_first_aac_header == 0)
    {
        DEBUG_PRINT("ETBP: Callin aac frame header parsing\n");
        res = aac_frameheader_parser((OMX_BUFFERHEADERTYPE*)buffer,
        &aac_header_info);
        if(m_bsac) {
            m_pTranscoder = new COmxBsacTranscoder(OMX_CORE_INPUT_BUFFER_SIZE,
                                AAC_RESIDUALDATA_BUFFER_SIZE,sizeof(META_IN));
        }
        if(res ==0)
        {
            m_first_aac_header = 1; /* No more parsing after first frame*/
            if(ioctl(get_drv_fd(), AUDIO_GET_CONFIG, &drv_config) == -1)
            DEBUG_PRINT("ETBP:get-config ioctl failed %d\n",errno);
            else
            DEBUG_PRINT("ETBP: Get config success\n");
            if(aac_header_info.input_format == FORMAT_ADIF)
            {
                // sampling frequency
                drv_config.sample_rate = aac_header_info.head.adif.sample_rate;
                drv_config.channel_count =
                aac_header_info.head.adif.channel_config;
                m_aac_param.nSampleRate = aac_header_info.head.adif.sample_rate;
                m_aac_param.nChannels = aac_header_info.head.adif.channel_config;
                m_aac_param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatADIF;
                adif = true;
                DEBUG_PRINT("ETBP-->ADIF SF=%u ch=%u\n",\
                (unsigned int)m_aac_param.nSampleRate,
                (unsigned int)m_aac_param.nChannels);
            }
            else if(aac_header_info.input_format == FORMAT_ADTS)
            {
                // sampling frequency
                drv_config.sample_rate =
                aac_frequency_index[aac_header_info.head.adts.fixed.\
                sampling_frequency_index];
                DEBUG_PRINT("ETB-->ADTS format, SF index=%u\n",\
                (unsigned int)aac_header_info.head.adts.fixed.
                sampling_frequency_index);
                drv_config.channel_count =
                aac_header_info.head.adts.fixed.channel_configuration;
                m_aac_param.nSampleRate =
                aac_frequency_index[
                aac_header_info.head.adts.fixed.sampling_frequency_index];
                m_aac_param.nChannels =
                aac_header_info.head.adts.fixed.channel_configuration;
                m_aac_param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP4ADTS;
                DEBUG_PRINT("ETBP-->ADTS SF=%u ch=%u\n",\
                (unsigned int)m_aac_param.nSampleRate,
                (unsigned int)m_aac_param.nChannels);
            }
            else if(aac_header_info.input_format == FORMAT_LOAS)
            {
                m_aac_param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP4LOAS;
                drv_config.sample_rate   = aac_frequency_index[
                aac_header_info.head.loas.freq_index];
                drv_config.channel_count =
                aac_header_info.head.loas.channel_config;
                m_aac_param.nSampleRate = aac_frequency_index[
                aac_header_info.head.loas.freq_index];
                m_aac_param.nChannels = aac_header_info.head.loas.channel_config;
                DEBUG_PRINT("ETBP-->LOAS SF=%u ch=%u\n",\
                (unsigned int)m_aac_param.nSampleRate,
                (unsigned int)m_aac_param.nChannels);
            }
            else
            {
                DEBUG_PRINT("ETBP-->RAW AAC FORMAT\n");
                // sampling frequency
                drv_config.sample_rate =
                aac_frequency_index[
                                          aac_header_info.head.raw.freq_index];
                drv_config.channel_count =
                aac_header_info.head.raw.channel_config;
                m_aac_param.nSampleRate =
                aac_frequency_index[
                                      aac_header_info.head.raw.ext_freq_index];
                m_aac_param.nChannels = aac_header_info.head.raw.channel_config;
                DEBUG_PRINT("sample_rate=%d ch_cnt=%d\n",\
                drv_config.sample_rate,drv_config.channel_count);
                flg = true;
                m_aac_param.eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;
            }
            if(get_comp()->get_pcm_feedback())
            {
                drv_config.meta_field = 1;
            }
            else
            {
                drv_config.meta_field = 0;
            }
            drv_config.buffer_size = get_comp()->get_in_buf_size();
            drv_config.buffer_count = get_comp()->get_inp_act_buf_cnt();
            drv_config.type = 2;

            if(m_aac_param.nChannels > 2)
            DEBUG_PRINT("ETBP:num of ch detected %u is not supported \n",\
            (unsigned int)m_aac_param.nChannels);
            DEBUG_PRINT("SET-CFG:bufsze[%d]bufcnt[%d]chcnt[%d]SF[%d]meta[%d]",\
            drv_config.buffer_size,
            drv_config.buffer_count,
            drv_config.channel_count,
            drv_config.sample_rate,
            drv_config.meta_field);
            if(ioctl(get_drv_fd(), AUDIO_SET_CONFIG, &drv_config) == -1)
            DEBUG_PRINT("ETBP:set-config ioctl failed %d\n",errno);
            else
            DEBUG_PRINT("ETBP: set config success\n");
            set_aac_config();

            if ( ioctl(get_drv_fd(), AUDIO_START, 0) < 0 )
            {
                DEBUG_PRINT("ETBP:ioctl audio start failed %d\n",errno);
                m_first_aac_header=0;
                get_comp()->post_command((unsigned)OMX_CommandStateSet,
                (unsigned)OMX_StateInvalid,
                OMX_COMPONENT_GENERATE_COMMAND);
                get_comp()->post_command(OMX_CommandFlush,-1,
                                         OMX_COMPONENT_GENERATE_COMMAND);
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);

                get_comp()->set_dec_state(false);
                return OMX_ErrorInvalidState;
            }
            else
            DEBUG_PRINT("ETBP: audio_start success\n");
            get_comp()->set_dec_state(true);
        }
        else
        {
            DEBUG_PRINT("configure Driver for AAC playback samplerate[%u]\n",\
            (unsigned int)m_aac_param.nSampleRate);
            if(ioctl(get_drv_fd(), AUDIO_GET_CONFIG, &drv_config) == -1)
            DEBUG_PRINT("ETBP:get-config ioctl failed %d\n",errno);
            drv_config.sample_rate = m_aac_param.nSampleRate;
            drv_config.channel_count = m_aac_param.nChannels;
            drv_config.type = 2; // aac decoding
            drv_config.buffer_size = get_comp()->get_in_buf_size();
            drv_config.buffer_count = get_comp()->get_inp_act_buf_cnt();
            if(ioctl(get_drv_fd(), AUDIO_SET_CONFIG, &drv_config) == -1)
            DEBUG_PRINT("ETBP:set-config ioctl failed %d\n",errno);
            set_aac_config();
            if(ioctl(get_drv_fd(), AUDIO_START, 0) < 0)
            {
                DEBUG_PRINT("ETBP:audio start ioctl failed %d\n",errno);
                m_first_aac_header=0;
                get_comp()->post_command((unsigned)OMX_CommandStateSet,
                (unsigned)OMX_StateInvalid,
                OMX_COMPONENT_GENERATE_COMMAND);
                get_comp()->post_command(OMX_CommandFlush,-1,
                                         OMX_COMPONENT_GENERATE_COMMAND);
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
                get_comp()->set_dec_state(false);

                return OMX_ErrorInvalidState;
            }
            get_comp()->set_dec_state(true);
        }
    }

    m_aac_comp->set_aac_param(&m_aac_param);
    if(!flg)
    {
        OMX_U32 len=0,dst_len =0;
        OMX_U8 *data=NULL;
        data = get_tmp_meta_buf();


        if(m_pTranscoder)
        {
            data = m_pTranscoder->transcodeData(buffer->pBuffer,buffer->nFilledLen,&dst_len);
	    if(data == 0 && dst_len == 0)
	    {
                DEBUG_PRINT("ETBP:transcoding failed, corrupt/unsupported clip\n");
                get_comp()->post_command((unsigned)OMX_CommandStateSet,
                (unsigned)OMX_StateInvalid,
                OMX_COMPONENT_GENERATE_COMMAND);
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
                get_comp()->post_command(OMX_CommandFlush,-1,
                                         OMX_COMPONENT_GENERATE_COMMAND);

                return OMX_ErrorInvalidState;
	    }
	    else
		len += dst_len;
        }
        else
        {
             data = get_tmp_meta_buf();
             process_aac(buffer,data,len,meta_in,adif);
        }
        if( get_comp()->get_pcm_feedback())
        {
            meta_in.offsetVal  = sizeof(META_IN);
            meta_in.nTimeStamp =
                            (((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp)*1000;
            meta_in.nFlags     = buffer->nFlags;
            memcpy(data,&meta_in, meta_in.offsetVal);
            len += meta_in.offsetVal;
        }
        else
        {
             // Advance the pointer by size of META_IN as both transcodeData(for bsac) and process_aac
             // would be inserting payloads considering the size of META_IN as well;
             // In other words, ignore META_IN in tunnel mode
             data = data+sizeof(META_IN);
        }
        /* No need insert silence when FF/RW is performed i.e whenever the input port
           flushed do not check for silence insertion */
        if(m_input_port_flushed == OMX_TRUE)
        {
            m_input_port_flushed = OMX_FALSE;
            if(m_silence)
            {
                m_silence->set_curr_ts((((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp));
                if(m_first_aac_header == 1)
                {
                    m_silence->set_prev_ts(0);
                }
                else
                {
                    m_silence->set_prev_ts((((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp));
                }
            }
        } 
        else if(m_silence && (OMX_AUDIO_AACStreamFormatRAW == m_aac_param.eAACStreamFormat))
        {
            m_silence->set_curr_ts((((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp));
            if((m_silence->checkForTimeStampGap(m_aac_param.nSampleRate,m_aac_param.nFrameLength)) == 
                                                                      OMX_TRUE)
            {
                OMX_U8 *silenceBuf = NULL;
                OMX_U32 iLen=0;
                iLen =  sizeof(META_IN) + sizeof(PSEUDO_RAW);
                silenceBuf = (OMX_U8*)malloc(iLen);

                while(m_silence->get_num_sil_frame())
                {
                    m_silence->insertSilenceInBuffer(silenceBuf,m_aac_param.nFrameLength,m_aac_param.nChannels);
                    write(get_drv_fd(),silenceBuf,(sizeof(META_IN)+ sizeof(PSEUDO_RAW)));
                    memset(silenceBuf,0,iLen);
                }
                m_silence->set_curr_ts((((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp));
                free(silenceBuf);
            }
            m_silence->set_prev_ts(m_silence->get_curr_ts());
        }
        if ( m_first_aac_header == 1 )
        {
            //pthread_mutex_lock(&m_in_th_lock_1);
            //is_in_th_sleep = true;
            //pthread_mutex_unlock(&m_in_th_lock_1);
            if( (len & 0x1) &&
                (OMX_AUDIO_AACStreamFormatADIF != m_aac_param.eAACStreamFormat))
            {
                /* To make sure that the length of data written to
                DSP is always even */
                data[len] = 0;
                len ++;
            }

            DEBUG_PRINT("Data written to driver len = %u\n",(unsigned int)len);
            get_comp()->setFirstBufSentToDSPFlg(true);

            write(get_drv_fd(), data,len);


            DEBUG_PRINT("ETBP: m_data_written_to_dsp %d", \
            get_comp()->getFirstBufSentToDSPFlg());
            DEBUG_PRINT("ETBP-->Sleeping in thread..\n");
            get_comp()->in_th_timedsleep();
            // added for continous ff+rew, no need to process
            // port settings and send data to decoder;
            // just return from here
            DEBUG_PRINT("ETBP-->Wakingin thread\n");
            get_comp()->setFirstBufSentToDSPFlg(false);
            if(m_aac_comp->get_flush_stat())
            {
                buffer->nFilledLen = 0;
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
                return OMX_ErrorNone;
            }
            // Feed the 1st buffer again as DSP has been stopped and resumed.
            // as part of handling STREAM_INFO_EVENTS.
            write(get_drv_fd(), data,len);
            m_first_aac_header ++;

            memcpy(&m_aac_param, &(m_aac_comp->get_aac_param()),
                   sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));

            if((get_comp()->get_pcm_feedback()) &&
                    ((m_aac_param.nSampleRate != get_sample_rate()) ||
                     (m_aac_param.nChannels != get_ch_cfg()) ||
                     (m_aac_param.nFrameLength != OMX_ADEC_AAC_FRAME_LEN)))
            {
                DEBUG_PRINT("Trigger OMX_COMPONENT_PORTSETTINGS_CHANGED");
                DEBUG_PRINT("Detected SF=%u SF=%u Detected ch=%u cfg ch=%u\n",\
                (unsigned int)m_aac_param.nSampleRate,get_sample_rate(),
                (unsigned int)m_aac_param.nChannels,get_ch_cfg());

                get_comp()->post_command((unsigned) & hComp,(unsigned) buffer,
                OMX_COMPONENT_PORTSETTINGS_CHANGED);
            }
            else
            {
                get_comp()->set_out_bEnabled(OMX_TRUE);
                get_comp()->set_port_recfg(true);
                DEBUG_PRINT("No OMX_COMPONENT_PORTSETTINGS_CHANGED");
                DEBUG_PRINT("SF1=%u SF=%u ch=%u ch=%u\n",\
                (unsigned int)m_aac_param.nSampleRate,
                (unsigned int)get_sample_rate(),
                (unsigned int)m_aac_param.nChannels,
                (unsigned int)get_ch_cfg());
                get_comp()->out_th_wakeup();
            }
        }
        else
        {
            if(aac_header_info.input_format == FORMAT_ADIF)
            {
                if ((get_comp()->get_pcm_feedback() == 1) &&
                    (meta_in.nFlags & OMX_BUFFERFLAG_EOS))
                {
                    meta_in.nFlags &= ~OMX_BUFFERFLAG_EOS;
                    memcpy(data,&meta_in, meta_in.offsetVal);
                    if (buffer->nFilledLen )
                    {
                        write(get_drv_fd(), data, len);
                    }
                    memset((data + meta_in.offsetVal), 0xFF,  DSP_MIN_BUF_SIZE);
                    DEBUG_PRINT("ADIF: NT: Writing 1550 bytes.\n");
                    meta_in.nFlags |= OMX_BUFFERFLAG_EOS;
                    memcpy(data,&meta_in, meta_in.offsetVal);
                    write(get_drv_fd(), data,
                                     ((meta_in.offsetVal) +  DSP_MIN_BUF_SIZE));
                }
                else if ((get_comp()->get_pcm_feedback() == 0) &&
                         (buffer->nFlags & OMX_BUFFERFLAG_EOS))
                {
                    if (buffer->nFilledLen )
                    {
                        write(get_drv_fd(), data, len);
                    }
                    memset(data, 0xFF,DSP_MIN_BUF_SIZE);
                    DEBUG_PRINT("ADIF: T: Writing 1550 bytes.\n");
                    write(get_drv_fd(), data, DSP_MIN_BUF_SIZE);
                    DEBUG_PRINT("EOS OCCURED \n");
                    fsync(get_drv_fd());
                    DEBUG_PRINT("INform input EOS to the client \n");
                    get_cb().EventHandler(&(get_comp()->m_cmp), get_app_data(),
                    OMX_EventBufferFlag,
                    1, 0, NULL );
                }
                else
                {
                    DEBUG_PRINT("Data written to driver len = %lu\n",len);
                    write(get_drv_fd(), data,len);
                }
            }
            else /*(aac_header_info.input_format != FORMAT_ADIF)*/
            {
                DEBUG_PRINT("Data written to driver len = %lu\n",len);
                write(get_drv_fd(), data,len);
                if (buffer->nFlags & 0x01)
                {
                    DEBUG_PRINT("EOS OCCURED \n");
                    DEBUG_PRINT("Writting the Final EOS data to Driver ");
                    if (get_comp()->get_pcm_feedback() == 0)
                    {
                        fsync(get_drv_fd());
                        DEBUG_PRINT("Inform input EOS to client\n");
                        get_cb().EventHandler(&(get_comp()->m_cmp),
                        get_app_data(),
                        OMX_EventBufferFlag,
                        1, 0, NULL );
                    }
                }
            }/*end of else (aac_header_info.input_format != FORMAT_ADIF)*/
        }
    }

    state = get_state();

    if(buffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
        unsigned eos_bm=0;
        eos_bm= get_comp()->get_eos_bm();
        eos_bm |= IP_PORT_BITMASK;
        get_comp()->set_eos_bm(eos_bm);
        DEBUG_PRINT("ETBP:EOSFLAG=%d\n",eos_bm);
    }

    if (state == OMX_StateExecuting)
    {
        //DEBUG_DETAIL("ETBP:EBD CB buffer=0x%x\n",buffer);
        //buffer->nFilledLen = 0;
        buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
    }
    else
    {
        // post the msg to input thread.
        DEBUG_DETAIL("ETBP:Not in Exe state, state[0x%x] bufHdr[%p]\n",\
        state,buffer);
        post_input((unsigned) & hComp,(unsigned) buffer,
        OMX_COMPONENT_GENERATE_BUFFER_DONE);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE  COmxDecAacIn_7x27::set_aac_config()
{
    DEBUG_PRINT("COmxDecAacIn_7x27::%s\n",__FUNCTION__);
    struct msm_audio_aac_config aac_config;
    int drv_fd = get_drv_fd();

    DEBUG_PRINT("Set-aac-config: Ch=%u SF=%u profile=%d stream=%d chmode=%d\n",
    (unsigned int)m_aac_param.nChannels,
    (unsigned int)m_aac_param.nSampleRate,
    m_aac_param.eAACProfile,m_aac_param.eAACStreamFormat,
    m_aac_param.eChannelMode);

    if (ioctl(drv_fd, AUDIO_GET_AAC_CONFIG, &aac_config)) {
        DEBUG_PRINT("omx_aac_adec::set_aac_config():GET_AAC_CONFIG failed");
        m_first_aac_header=0;
        if(drv_fd >= 0){
            get_comp()->post_command((unsigned)OMX_CommandStateSet,
            (unsigned)OMX_StateInvalid,
            OMX_COMPONENT_GENERATE_COMMAND);
        }
        return OMX_ErrorInvalidComponent;
    }
    else
    DEBUG_PRINT("ETBP: get aac config success\n");

    DEBUG_PRINT("AAC-CFG::format[%d]chcfg[%d]audobj[%d]epcfg[%d]data_res[%d]\n",
    aac_config.format,
    aac_config.channel_configuration,
    aac_config.audio_object,
    aac_config.ep_config,
    aac_config.aac_section_data_resilience_flag);

    DEBUG_PRINT("CFG:scalefact[%d]spectral[%d]sbron[%d]sbrps[%d]dualmono[%d]",\
    aac_config.aac_scalefactor_data_resilience_flag,
    aac_config.aac_spectral_data_resilience_flag,
    aac_config.sbr_on_flag,
    aac_config.sbr_ps_on_flag,
    aac_config.dual_mono_mode);

    switch(m_aac_param.eAACStreamFormat)
    {
    case OMX_AUDIO_AACStreamFormatMP4LOAS:
        aac_config.format = AUDIO_AAC_FORMAT_LOAS;
        break;
    case OMX_AUDIO_AACStreamFormatMP4ADTS:
        aac_config.format = AUDIO_AAC_FORMAT_ADTS;
        break;
    case OMX_AUDIO_AACStreamFormatADIF:
        aac_config.format = AUDIO_AAC_FORMAT_RAW ;
        break;
    case OMX_AUDIO_AACStreamFormatRAW:
    default:
        aac_config.format = AUDIO_AAC_FORMAT_PSUEDO_RAW ;
        if(m_pTranscoder)
        {
            aac_config.audio_object = AUDIO_AAC_OBJECT_BSAC;
        }
        break;
    }

    switch(m_aac_param.eAACProfile)
    {
    case  OMX_AUDIO_AACObjectLC:
    default:
        aac_config.sbr_on_flag = 1;
        aac_config.sbr_ps_on_flag = 1;
        break;
    case  OMX_AUDIO_AACObjectHE:
        aac_config.sbr_on_flag = 1;
        aac_config.sbr_ps_on_flag = 1;
        break;
    case  OMX_AUDIO_AACObjectHE_PS:
        aac_config.sbr_on_flag = 1;
        aac_config.sbr_ps_on_flag = 1;
        break;
    }

    aac_config.channel_configuration = m_aac_param.nChannels;
    if (ioctl(drv_fd, AUDIO_SET_AAC_CONFIG, &aac_config)) {
        DEBUG_PRINT("set_aac_config():AUDIO_SET_AAC_CONFIG failed");
        m_first_aac_header=0;
        get_comp()->post_command((unsigned)OMX_CommandStateSet,
        (unsigned)OMX_StateInvalid,
        OMX_COMPONENT_GENERATE_COMMAND);
        return OMX_ErrorInvalidComponent;
    }
    else
    DEBUG_PRINT("ETBP: set aac config success\n");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE  COmxDecAacIn_7x27::aac_frameheader_parser(
OMX_BUFFERHEADERTYPE* bufHdr,struct aac_header *header)
{
    DEBUG_PRINT("COmxDecAacIn_7x27::%s\n",__FUNCTION__);
    OMX_U8 *buf = bufHdr->pBuffer;
    OMX_U32 ext_aud_obj_type = 0;

    if( (buf[0] == 0x41) &&  (buf[1] == 0x44) &&
            (buf[2] == 0x49) &&  (buf[3] == 0x46) )
    /*check "ADIF" */
    {
        // format is ADIF
        // parser to parse ADIF
        header->input_format = FORMAT_ADIF;
        audaac_extract_adif_header(bufHdr->pBuffer,header);
    }

    else
    {
        DEBUG_PRINT("Parser-->RAW or Partial frame to be found...\n");
        OMX_U8 *data;
        OMX_U32 temp, ext_flag, status = 0, i = 0;
        OMX_U8 sync_word_found = 0;
        while(i < (bufHdr->nFilledLen))
        {
            if ((buf[i] == 0xFF) && ((buf[i+1] & 0xF6) == 0xF0) )
            {
                OMX_U32 index=0;OMX_U32 sf=0;
                if((i+3) >= (bufHdr->nFilledLen))
                {
                    break;
                }
                sf = (buf[i+2] & 0x3C) >> 2;
                // Get the frequency index from bits 19,20,21,22
                //Get the channel configuration bits 24,25,26
                index = (buf[i+2] & 0x01) << 0x02 ;
                index |= (buf[i+3] & 0xC0) >> 0x06;
                DEBUG_PRINT("PARSER-->Partial frame..ADTS sync word \n");
                DEBUG_PRINT("ADTS-->freq_index=%u ch-cfg=%u\n",\
                (unsigned int)sf,(unsigned int)index);

                //if not ok continue, false sync words found
                if((sf > 12) || (index > 2))
                {
                    DEBUG_PRINT("Parser-->fake sync words sf=%u index=%u\n",\
                    (unsigned int)sf,(unsigned int)index);
                    i++;
                    // invalid SF/ch-cfg, maybe fake sync word found
                    continue;
                }
                sync_word_found = 1;
                m_bytes_to_skip = i;
                DEBUG_PRINT("ADTS-->bytes to skip : %d\n", m_bytes_to_skip);
                header->input_format = FORMAT_ADTS;
                header->head.adts.fixed.channel_configuration = index;
                header->head.adts.fixed.sampling_frequency_index = sf;
                break;
            }
            else if ((buf[i] == 0x56) && ((buf[i+1] & 0xE0) == 0xE0) )
            {
                DEBUG_PRINT("PARSER-->PARTIAL..LOAS\n");
                unsigned int ret = 0;
                //DEBUG_PRINT("i=%u buf[i]=%u buf[i+1] %u\n",i,buf[i], buf[i+1]);

                ret = audaac_extract_loas_header(
                &bufHdr->pBuffer[i],
                (((bufHdr->nFilledLen)-i)*8),header);
                if((int)ret == -1)
                {
                    DEBUG_PRINT("Parser--> fake sync words SF=%u cf=%u\n",\
                    (unsigned int)header->head.loas.freq_index,\
                    (unsigned int)header->head.loas.channel_config);
                    i++;
                    // invalid SF/ch-cfg, maybe fake sync word found
                    continue;
                }
                sync_word_found = 1;
                m_bytes_to_skip = i;
                DEBUG_PRINT("LOAS-->bytes to skip : %d\n", m_bytes_to_skip);

                header->input_format = FORMAT_LOAS;

                break;
            }
            i++;
        }
        if(!sync_word_found)
        {
            // RAW AAC-ADTS PARSER
            header->input_format = FORMAT_RAW;

            m_aac_hdr_bit_index = 0;
            data = (OMX_U8*)buf;

            audaac_extract_bits(data, 5, &temp);
            header->head.raw.aud_obj_type = temp;
            header->head.raw.ext_aud_obj_type = temp;
            DEBUG_DETAIL("RAW-->aud_obj_type=0x%x\n",\
                                                 header->head.raw.aud_obj_type);
            if(header->head.raw.ext_aud_obj_type == 22)
            {
                m_bsac = OMX_TRUE;/*BSAC*/
            }
            audaac_extract_bits(data, AAC_SAMPLING_FREQ_INDEX_SIZE, &temp);
            header->head.raw.freq_index = temp;
            header->head.raw.ext_freq_index = temp;
            DEBUG_DETAIL("RAW-->freq_index=0x%x\n",header->head.raw.freq_index);
            if(header->head.raw.freq_index == 0xf)
            {
                /* Current release does not support sampling freqiencies
                 other than frequencies listed in aac_frequency_index[] */
                /* Extract sampling frequency value */
                audaac_extract_bits(data, 12, &temp);
                audaac_extract_bits(data, 12, &temp);
            }
            /* Extract channel config value */
            audaac_extract_bits(data, 4, &temp);
            header->head.raw.channel_config = temp;
            DEBUG_DETAIL("RAW-->ch cfg=0x%x\n",header->head.raw.channel_config);
            header->head.raw.sbr_present_flag = 0;
            header->head.raw.sbr_ps_present_flag = 0;
            /* If the audioOjectType is SBR or PS */
            if((header->head.raw.aud_obj_type == 5) ||
                    (header->head.raw.aud_obj_type == 29))
            {
                header->head.raw.ext_aud_obj_type = 5;
                header->head.raw.sbr_present_flag = 1;
                if (header->head.raw.aud_obj_type == 29)
                {
                    header->head.raw.ext_aud_obj_type = 29;
                    header->head.raw.sbr_ps_present_flag = 1;
                }
                /* extensionSamplingFrequencyIndex */
                audaac_extract_bits(data, 4, &temp);
                header->head.raw.ext_freq_index = temp;

                if (header->head.raw.ext_freq_index == 0x0f)
                {
                    /* Extract sampling frequency value */
                    audaac_extract_bits(data, 12, &temp);
                    audaac_extract_bits(data, 12, &temp);
                }
                audaac_extract_bits(data, 5, &temp);
                header->head.raw.aud_obj_type = temp;
            }
            /* If audioObjectType is AAC_LC or AAC_LTP */
            if (header->head.raw.aud_obj_type == 2 ||
                    header->head.raw.aud_obj_type == 4)
            {
                /* frame_length_flag : 0 - 1024; 1 - 960
        Current release supports AAC frame length of 1024 only */
                audaac_extract_bits(data, 1, &temp);
                if(temp) status = 1;
                /* dependsOnCoreCoder == 1, core coder has different sampling rate
        in a scalable bitstream. This is mainly set fro AAC_SCALABLE Object only.
        Currrent release does not support AAC_SCALABLE Object */
                audaac_extract_bits(data, 1, &temp);
                if(temp) status = 1;

                /* extensionFlag */
                audaac_extract_bits(data, 1, &ext_flag);

                if(header->head.raw.channel_config == 0 && status == 0)
                {
                    OMX_U32 num_fce, num_bce, num_sce, num_lfe, num_ade, num_vce;
                    OMX_U32 i, profile, samp_freq_idx, num_ch = 0;

                    /* Parse Program configuration element information */
                    audaac_extract_bits(data, AAC_ELEMENT_INSTANCE_TAG_SIZE, &temp);
                    audaac_extract_bits(data, AAC_PROFILE_SIZE, &profile);
                    DEBUG_DETAIL("RAW-->PCE -- profile=%lu\n",profile);
                    audaac_extract_bits(data,
                    AAC_SAMPLING_FREQ_INDEX_SIZE,
                    &samp_freq_idx);
                    DEBUG_DETAIL("RAW-->PCE -- samp_freq_idx=%lu\n",samp_freq_idx);
                    audaac_extract_bits(data, AAC_NUM_FRONT_CHANNEL_ELEMENTS_SIZE,
                    &num_fce);
                    DEBUG_DETAIL("RAW-->PCE -- num_fce=%lu\n",num_fce);

                    audaac_extract_bits(data, AAC_NUM_SIDE_CHANNEL_ELEMENTS_SIZE,
                    &num_sce);
                    DEBUG_DETAIL("RAW-->PCE -- num_sce=%lu\n",num_sce);
                    audaac_extract_bits(data, AAC_NUM_BACK_CHANNEL_ELEMENTS_SIZE,
                    &num_bce);
                    DEBUG_DETAIL("RAW-->PCE -- num_bce=%lu\n",num_bce);

                    audaac_extract_bits(data, AAC_NUM_LFE_CHANNEL_ELEMENTS_SIZE,
                    &num_lfe);
                    DEBUG_DETAIL("RAW-->PCE -- num_lfe=%lu\n",num_lfe);

                    audaac_extract_bits(data, AAC_NUM_ASSOC_DATA_ELEMENTS_SIZE,
                    &num_ade);
                    DEBUG_DETAIL("RAW-->PCE -- num_ade=%lu\n",num_ade);

                    audaac_extract_bits(data, AAC_NUM_VALID_CC_ELEMENTS_SIZE,
                    &num_vce);
                    DEBUG_DETAIL("RAW-->PCE -- num_vce=%lu\n",num_vce);

                    audaac_extract_bits(data, AAC_MONO_MIXDOWN_PRESENT_SIZE,
                    &temp);
                    if(temp) {
                        audaac_extract_bits(data, AAC_MONO_MIXDOWN_ELEMENT_SIZE,
                        &temp);
                    }

                    audaac_extract_bits(data, AAC_STEREO_MIXDOWN_PRESENT_SIZE,
                    &temp);
                    if(temp) {
                        audaac_extract_bits(data, AAC_STEREO_MIXDOWN_ELEMENT_SIZE,
                        &temp);
                    }

                    audaac_extract_bits(data, AAC_MATRIX_MIXDOWN_PRESENT_SIZE,
                    &temp);
                    if(temp) {
                        audaac_extract_bits(data, AAC_MATRIX_MIXDOWN_SIZE, &temp);
                    }

                    for(i=0; i<(num_fce+num_sce+num_bce); i++) {
                        /* Is Element CPE ?*/
                        audaac_extract_bits(data, 1, &temp);
                        /* If the element is CPE, increment num channels by 2 */
                        if(temp) num_ch += 2;
                        else     num_ch += 1;
                        DEBUG_DETAIL("RAW-->PCE -- num_ch=%lu\n",num_ch);
                        audaac_extract_bits(data, 4, &temp);
                    }
                    for(i=0; i<num_lfe; i++) {
                        /* LFE element can not be CPE */
                        num_ch += 1;
                        DEBUG_DETAIL("RAW-->PCE -- num_ch=%lu\n",num_ch);
                        audaac_extract_bits(data, AAC_LFE_SIZE, &temp);
                    }
                    for(i=0; i<num_ade; i++) {
                        audaac_extract_bits(data, AAC_ADE_SIZE, &temp);
                    }
                    for(i=0; i<num_vce; i++) {
                        /* Coupling channels can not be counted as output
                         channels as they will be
            coupled with main audio channels */
                        audaac_extract_bits(data, AAC_VCE_SIZE, &temp);
                    }
                    header->head.raw.channel_config = num_ch ;
                    /* byte_alignment() */
                    temp = (OMX_U8)(m_aac_hdr_bit_index % 8);
                    if(temp) {
                        m_aac_hdr_bit_index += 8 - temp;
                    }
                    /* get comment_field_bytes */
                    temp = data[m_aac_hdr_bit_index/8];
                    m_aac_hdr_bit_index += AAC_COMMENT_FIELD_BYTES_SIZE;
                    /* Skip the comment */
                    m_aac_hdr_bit_index += temp * AAC_COMMENT_FIELD_DATA_SIZE;
                    /* byte_alignment() */
                    temp = (OMX_U8)(m_aac_hdr_bit_index % 8);
                    if(temp) {
                        m_aac_hdr_bit_index += 8 - temp;
                    }
                    DEBUG_DETAIL("RAW-->PCE -- m_aac_hdr_bit_index=0x%x\n",\
                    m_aac_hdr_bit_index);
                }
                if (ext_flag)
                {
                    /* 17: ER_AAC_LC 19:ER_AAC_LTP
                    20:ER_AAC_SCALABLE 23:ER_AAC_LD */
                    if (header->head.raw.aud_obj_type == 17 ||
                            header->head.raw.aud_obj_type == 19 ||
                            header->head.raw.aud_obj_type == 20 ||
                            header->head.raw.aud_obj_type == 23)
                    {
                        audaac_extract_bits(data, 1, &temp);
                        audaac_extract_bits(data, 1, &temp);
                        audaac_extract_bits(data, 1, &temp);
                    }

                    /* extensionFlag3 */
                    audaac_extract_bits(data, 1, &temp);
                    if(temp) status = 1;
                }
            }

            /* SBR tool explicit signaling ( backward compatible ) */
            if (ext_aud_obj_type != 5)
            {
                /* syncExtensionType */
                audaac_extract_bits(data, 11, &temp);
                if (temp == 0x2b7)
                {
                    /*extensionAudioObjectType*/
                    audaac_extract_bits(data, 5, &ext_aud_obj_type);
                    if(ext_aud_obj_type == 5)
                    {
                        /*sbrPresentFlag*/
                        audaac_extract_bits(data, 1, &temp);
                        header->head.raw.sbr_present_flag = temp;
                        if(temp)
                        {
                            /*extensionSamplingFrequencyIndex*/
                            audaac_extract_bits(data,
                            AAC_SAMPLING_FREQ_INDEX_SIZE,
                            &temp);
                            header->head.raw.ext_freq_index = temp;
                            if(temp == 0xf)
                            {
                                /*Extract 24 bits for sampling frequency value */
                                audaac_extract_bits(data, 12, &temp);
                                audaac_extract_bits(data, 12, &temp);
                            }

                            /* syncExtensionType */
                            audaac_extract_bits(data, 11, &temp);

                            if (temp == 0x548)
                            {
                                /*psPresentFlag*/
                                audaac_extract_bits(data, 1, &temp);
                                header->head.raw.sbr_ps_present_flag = temp;
                                if(temp)
                                {
                                    /* extAudioObject is PS */
                                    ext_aud_obj_type = 29;
                                }
                            }
                        }
                    }
                    header->head.raw.ext_aud_obj_type = ext_aud_obj_type;
                }
            }
            DEBUG_PRINT("RAW-->ch cfg=0x%x\n",\
            header->head.raw.channel_config);
            DEBUG_PRINT("RAW-->freq_index=0x%x\n",\
            header->head.raw.freq_index);
            DEBUG_PRINT("RAW-->ext_freq_index=0x%x\n",\
            header->head.raw.ext_freq_index);
            DEBUG_PRINT("RAW-->aud_obj_type=0x%x\n",\
            header->head.raw.aud_obj_type);
            DEBUG_PRINT("RAW-->ext_aud_obj_type=0x%x\n",\
            header->head.raw.ext_aud_obj_type);
        }
    }
    return OMX_ErrorNone;
}
void COmxDecAacIn_7x27::audaac_extract_adif_header(
OMX_U8 *data,
struct aac_header *aac_header_info)
{
    DEBUG_PRINT("COmxDecAacIn_7x27::%s\n",__FUNCTION__);
    OMX_U32  buf8;
    OMX_U32 buf32;
    OMX_U32  num_pfe, num_fce, num_sce, num_bce, num_lfe, num_ade, num_vce;
    OMX_U32  pfe_index;
    OMX_U8  i;
    OMX_U32 temp;

    /* We already parsed the 'ADIF' keyword. Skip it. */
    m_aac_hdr_bit_index = 32;

    audaac_extract_bits(data, AAC_COPYRIGHT_PRESENT_SIZE, &buf8);

    if(buf8) {
        /* Copyright present; Just discard it for now */
        m_aac_hdr_bit_index += 72;
    }

    audaac_extract_bits(data, AAC_ORIGINAL_COPY_SIZE,
    &temp);

    audaac_extract_bits(data, AAC_HOME_SIZE, &temp);

    audaac_extract_bits(data, AAC_BITSTREAM_TYPE_SIZE,
    &temp);
    aac_header_info->head.adif.variable_bit_rate = temp;

    audaac_extract_bits(data, AAC_BITRATE_SIZE, &temp);

    audaac_extract_bits(data, AAC_NUM_PFE_SIZE, &num_pfe);

    for(pfe_index=0; pfe_index<num_pfe+1; pfe_index++) {
        if(!aac_header_info->head.adif.variable_bit_rate) {
            /* Discard */
            audaac_extract_bits(data, AAC_BUFFER_FULLNESS_SIZE, &buf32);
        }

        /* Extract Program Config Element */

        /* Discard element instance tag */
        audaac_extract_bits(data, AAC_ELEMENT_INSTANCE_TAG_SIZE, &buf8);

        audaac_extract_bits(data, AAC_PROFILE_SIZE,
        &buf8);
        aac_header_info->head.adif.aud_obj_type = buf8;

        buf8 = 0;
        audaac_extract_bits(data, AAC_SAMPLING_FREQ_INDEX_SIZE, &buf8);
        aac_header_info->head.adif.freq_index = buf8;
        aac_header_info->head.adif.sample_rate =
                                       aac_frequency_index[buf8];

        audaac_extract_bits(data, AAC_NUM_FRONT_CHANNEL_ELEMENTS_SIZE, &num_fce);

        audaac_extract_bits(data, AAC_NUM_SIDE_CHANNEL_ELEMENTS_SIZE, &num_sce);

        audaac_extract_bits(data, AAC_NUM_BACK_CHANNEL_ELEMENTS_SIZE, &num_bce);

        audaac_extract_bits(data, AAC_NUM_LFE_CHANNEL_ELEMENTS_SIZE, &num_lfe);

        audaac_extract_bits(data, AAC_NUM_ASSOC_DATA_ELEMENTS_SIZE, &num_ade);

        audaac_extract_bits(data, AAC_NUM_VALID_CC_ELEMENTS_SIZE, &num_vce);

        audaac_extract_bits(data, AAC_MONO_MIXDOWN_PRESENT_SIZE, &buf8);
        if(buf8) {
            audaac_extract_bits(data, AAC_MONO_MIXDOWN_ELEMENT_SIZE, &buf8);
        }

        audaac_extract_bits(data, AAC_STEREO_MIXDOWN_PRESENT_SIZE, &buf8);
        if(buf8) {
            audaac_extract_bits(data, AAC_STEREO_MIXDOWN_ELEMENT_SIZE, &buf8);
        }

        audaac_extract_bits(data, AAC_MATRIX_MIXDOWN_PRESENT_SIZE, &buf8);
        if(buf8) {
            audaac_extract_bits(data, AAC_MATRIX_MIXDOWN_SIZE, &buf8);
        }

        for(i=0; i<num_fce; i++) {
            audaac_extract_bits(data, AAC_FCE_SIZE, &buf8);
        }

        for(i=0; i<num_sce; i++) {
            audaac_extract_bits(data, AAC_SCE_SIZE, &buf8);
        }

        for(i=0; i<num_bce; i++) {
            audaac_extract_bits(data, AAC_BCE_SIZE, &buf8);
        }

        for(i=0; i<num_lfe; i++) {
            audaac_extract_bits(data, AAC_LFE_SIZE, &buf8);
        }

        for(i=0; i<num_ade; i++) {
            audaac_extract_bits(data, AAC_ADE_SIZE, &buf8);
        }

        for(i=0; i<num_vce; i++) {
            audaac_extract_bits(data, AAC_VCE_SIZE, &buf8);
        }

        /* byte_alignment() */
        buf8 = (OMX_U8)(m_aac_hdr_bit_index % 8);
        if(buf8) {
            m_aac_hdr_bit_index += 8 - buf8;
        }

        /* get comment_field_bytes */
        buf8 = data[m_aac_hdr_bit_index/8];

        m_aac_hdr_bit_index += AAC_COMMENT_FIELD_BYTES_SIZE;

        /* Skip the comment */
        m_aac_hdr_bit_index += buf8 * AAC_COMMENT_FIELD_DATA_SIZE;
    }

    /* byte_alignment() */
    buf8 = (OMX_U8)(m_aac_hdr_bit_index % 8);
    if(buf8) {
        m_aac_hdr_bit_index += 8 - buf8;
    }

    aac_header_info->head.adif.channel_config =
    (num_fce + num_sce + num_bce +
    num_lfe + num_ade + num_vce) ;
}
void COmxDecAacIn_7x27::audaac_extract_bits(
OMX_U8  *input,
OMX_U8  num_bits_reqd,
OMX_U32 *out_buf
)
{
    DEBUG_PRINT("COmxDecAacIn_7x27::%s\n",__FUNCTION__);
    OMX_U32 output = 0;
    OMX_U32 value = 0;
    OMX_U32 byte_index;
    OMX_U8   bit_index;
    OMX_U8   bits_avail_in_byte;
    OMX_U8   num_to_copy;
    OMX_U8   mask;
    OMX_U8   num_remaining = num_bits_reqd;

    while(num_remaining) {
        byte_index = m_aac_hdr_bit_index / 8;
        bit_index  = m_aac_hdr_bit_index % 8;

        bits_avail_in_byte = 8 - bit_index;
        num_to_copy = MIN(bits_avail_in_byte, num_remaining);

        mask = ~(0xff << bits_avail_in_byte);

        value = input[byte_index] & mask;
        value = value >> (bits_avail_in_byte - num_to_copy);

        m_aac_hdr_bit_index += num_to_copy;
        num_remaining -= num_to_copy;

        output = (output << num_to_copy) | value;
    }
    *out_buf=output;

}
int COmxDecAacIn_7x27::audaac_extract_loas_header(
OMX_U8 *data,OMX_U32 len,
struct aac_header *aac_header_info)
{
    DEBUG_PRINT("COmxDecAacIn::%s\n",__FUNCTION__);
    OMX_U32      aac_frame_length = 0;
    OMX_U32      value    = 0;
    OMX_U32       obj_type = 0;
    OMX_U32       ext_flag = 0;

    OMX_U32       num_fc   = 0;
    OMX_U32       num_sc   = 0;
    OMX_U32       num_bc   = 0;
    OMX_U32       num_lfe  = 0;
    OMX_U32       num_ade  = 0;
    OMX_U32       num_vce  = 0;

    OMX_U16      num_bits_to_skip = 0;
    DEBUG_PRINT("LOAS HEADER -->len=%u\n",(unsigned int)len);
    /* Length is in the bits 11 to 23 from left in the bitstream */
    m_aac_hdr_bit_index = 11;
    if(((m_aac_hdr_bit_index+42)) > len)
    return -1;

    audaac_extract_bits(data, 13, &aac_frame_length);

    /* useSameStreamMux */
    audaac_extract_bits(data, 1, &value);

    if(value)
    {
        return -1;
    }
    /* Audio mux version */
    audaac_extract_bits(data, 1, &value);
    if (value)
    {
        /* Unsupported format */
        return -1;
    }
    /* allStreamsSameTimeFraming */
    audaac_extract_bits(data, 1, &value);
    if (!value)
    {
        /* Unsupported format */
        return -1;
    }
    /* numSubFrames */
    audaac_extract_bits(data, 6, &value);
    if (value)
    {
        /* Unsupported format */
        return -1;
    }
    /* numProgram */
    audaac_extract_bits(data, 4, &value);
    if (value)
    {
        /* Unsupported format */
        return -1;
    }
    /* numLayer */
    audaac_extract_bits(data, 3, &value);
    if (value)
    {
        /* Unsupported format */
        return -1;
    }

    /* Audio specific config */
    /* audioObjectType */
    audaac_extract_bits(data, 5, &value);
    if (!LOAS_IS_AUD_OBJ_SUPPORTED(value))
    {
        /* Unsupported format */
        return -1;
    }
    aac_header_info->head.loas.aud_obj_type = value;

    /* SFI */
    audaac_extract_bits(data, 4, &value);
    if (!LOAS_IS_SFI_SUPPORTED(value))
    {
        /* Unsupported format */
        return -1;
    }
    aac_header_info->head.loas.freq_index = value;

    /* Channel config */
    audaac_extract_bits(data, 4, &value);
    if (!LOAS_IS_CHANNEL_CONFIG_SUPPORTED(value,
                aac_header_info->head.loas.aud_obj_type))
    {
        /* Unsupported format */
        return -1;
    }

    aac_header_info->head.loas.channel_config = value;

    if (aac_header_info->head.loas.aud_obj_type == 5)
    {
        /* Extension SFI */
        if((m_aac_hdr_bit_index+9) > len)
        return -1;
        audaac_extract_bits(data, 4, &value);
        if (!LOAS_IS_EXT_SFI_SUPPORTED(value))
        {
            /* Unsupported format */
            return -1;
        }
        /* Audio object_type */
        audaac_extract_bits(data, 5, &value);
        if (!LOAS_IS_AUD_OBJ_SUPPORTED(value))
        {
            /* Unsupported format */
            return -1;
        }
        aac_header_info->head.loas.aud_obj_type = value;
    }
    obj_type = aac_header_info->head.loas.aud_obj_type;

    if (LOAS_GA_SPECIFIC_CONFIG(obj_type))
    {
        if((m_aac_hdr_bit_index+3) > len)
        return -1;
        /* framelengthFlag */
        audaac_extract_bits(data, 1, &value);
        if (value)
        {
            /* Unsupported format */
            return -1;
        }

        /* dependsOnCoreCoder */
        audaac_extract_bits(data, 1, &value);
        if (value)
        {
            /* Unsupported format */
            return -1;
        }

        /* extensionFlag */
        audaac_extract_bits(data, 1, &ext_flag);
        if (!LOAS_IS_EXT_FLG_SUPPORTED(ext_flag,obj_type) )
        {
            /* Unsupported format */
            return -1;
        }
        if (!aac_header_info->head.loas.channel_config)
        {
            /* Skip 10 bits */
            if((m_aac_hdr_bit_index+45) > len)
            return -1;
            audaac_extract_bits(data, 10, &value);

            audaac_extract_bits(data, 4, &num_fc);
            audaac_extract_bits(data, 4, &num_sc);
            audaac_extract_bits(data, 4, &num_bc);
            audaac_extract_bits(data, 2, &num_lfe);
            audaac_extract_bits(data, 3, &num_ade);
            audaac_extract_bits(data, 4, &num_vce);

            /* mono_mixdown_present */
            audaac_extract_bits(data, 1, &value);
            if (value) {
                /* mono_mixdown_element_number */
                audaac_extract_bits(data, 4, &value);
            }

            /* stereo_mixdown_present */
            audaac_extract_bits(data, 1, &value);
            if (value) {
                /* stereo_mixdown_element_number */
                audaac_extract_bits(data, 4, &value);
            }

            /* matrix_mixdown_idx_present */
            audaac_extract_bits(data, 1, &value);
            if (value) {
                /* matrix_mixdown_idx and presudo_surround_enable */
                audaac_extract_bits(data, 3, &value);
            }
            num_bits_to_skip = (num_fc * 5) + (num_sc * 5) + (num_bc * 5) +
            (num_lfe * 4) + (num_ade * 4) + (num_vce * 5);

            if((m_aac_hdr_bit_index+num_bits_to_skip) > len)
            return -1;

            while (num_bits_to_skip != 0){
                if (num_bits_to_skip > 32) {
                    audaac_extract_bits(data, 32, &value);
                    num_bits_to_skip -= 32;
                } else {
                    audaac_extract_bits(data, num_bits_to_skip, &value);
                    num_bits_to_skip = 0;
                }
            }

            if (m_aac_hdr_bit_index & 0x07)
            {
                m_aac_hdr_bit_index +=  (8 - (m_aac_hdr_bit_index & 0x07));
            }
            if((m_aac_hdr_bit_index + 8) > len)
            return -1;
            /* comment_field_bytes */
            audaac_extract_bits(data, 8, &value);

            num_bits_to_skip = value * 8;

            if((m_aac_hdr_bit_index + num_bits_to_skip) > len)
            return -1;

            while (num_bits_to_skip != 0){
                if (num_bits_to_skip > 32) {
                    audaac_extract_bits(data, 32, &value);
                    num_bits_to_skip -= 32;
                } else {
                    audaac_extract_bits(data, num_bits_to_skip, &value);
                    num_bits_to_skip = 0;
                }
            }
        }

        if (ext_flag)
        {
            if (((obj_type == 17) || (obj_type == 19) ||
                        (obj_type == 20) || (obj_type == 23)))
            {
                if((m_aac_hdr_bit_index + 3) > len)
                return -1;
                audaac_extract_bits(data, 1, &value);
                audaac_extract_bits(data, 1, &value);
                audaac_extract_bits(data, 1, &value);
            }
            /* extensionFlag3 */
            if((m_aac_hdr_bit_index + 1) > len)
            return -1;
            audaac_extract_bits(data, 1, &value);
        }
    }
    if ((obj_type != 18) && (obj_type >= 17) && (obj_type <= 27))
    {
        /* epConfig */
        if((m_aac_hdr_bit_index + 2) > len)
        return -1;
        audaac_extract_bits(data, 2, &value);
        if (value)
        {
            /* Unsupported format */
            return -1;
        }
    }
    /* Back in StreamMuxConfig */
    /* framelengthType */
    if((m_aac_hdr_bit_index + 3) > len)
    return -1;
    audaac_extract_bits(data, 3, &value);
    if (value)
    {
        /* Unsupported format */
        return -1;
    }

    if((aac_header_info->head.loas.freq_index > 12) ||
            (aac_header_info->head.loas.channel_config > 2))
    {
        return -1;
    }
    else
    return 0;
}
OMX_ERRORTYPE COmxDecAacIn_7x27::process_aac(
                                OMX_BUFFERHEADERTYPE  *buffer,
                                OMX_U8                *pDest,
                                OMX_U32               &len,
                                META_IN               &meta_in, bool adif)
{
    PSEUDO_RAW pseudo_raw;
    OMX_U16 fr_len =0 ;OMX_U16 temp=0;
    pDest = get_tmp_meta_buf();
    OMX_U32 iLen = sizeof(META_IN);
    (void)meta_in;
    //len += sizeof(META_IN);
    if((buffer->nFlags & OMX_BUFFERFLAG_EOS) && (buffer->nFilledLen == 0))
    {
        len = 0;
        return OMX_ErrorNone;
    }
    if((OMX_AUDIO_AACStreamFormatRAW == m_aac_param.eAACStreamFormat))
    {
        pseudo_raw.sync_word = 0xFFFF;
        fr_len = (((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen) & 0xFF ;
        fr_len = fr_len << 8;
        temp = ((((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen) & 0xFF00);
        temp = temp >> 8;
        fr_len |= temp;
        pseudo_raw.fr_len    = fr_len;
        memcpy(&pDest[iLen],&pseudo_raw,sizeof(PSEUDO_RAW));
        iLen +=  sizeof(PSEUDO_RAW);
    }
    //if (m_aac_param.eAACStreamFormat == OMX_AUDIO_AACStreamFormatADIF)
    if (adif)
    {
        memcpy(&pDest[iLen],(((OMX_BUFFERHEADERTYPE*)buffer)->pBuffer + (m_aac_hdr_bit_index/8)),
                              (((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen - (m_aac_hdr_bit_index/8)));
        iLen += (((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen - (m_aac_hdr_bit_index/8));

        COmxBaseIn::m_in_pb_stat.tot_in_buf_len += (buffer->nFilledLen - (m_aac_hdr_bit_index/8));
    }
    else if(m_bytes_to_skip)
    {
        memcpy(&pDest[iLen],(((OMX_BUFFERHEADERTYPE*)buffer)->pBuffer + m_bytes_to_skip),
                              (((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen - m_bytes_to_skip));

        iLen += (((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen - m_bytes_to_skip);
        COmxBaseIn::m_in_pb_stat.tot_in_buf_len += (buffer->nFilledLen - m_bytes_to_skip );
        m_bytes_to_skip = 0;
    }
    else
    {
        memcpy(&pDest[iLen],((OMX_BUFFERHEADERTYPE*)buffer)->pBuffer,
                          ((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen);
        iLen += ((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen;

        COmxBaseIn::m_in_pb_stat.tot_in_buf_len += buffer->nFilledLen;
    }
    // Ignore the len of META_IN for now; If NT mode, len will be incremented by 14 after
    // this function returns; if tunnel mode ignore the size of META_IN
    // and advance the data pointer by size of META_IN
    len = iLen - sizeof(META_IN);

    return OMX_ErrorNone;
}

