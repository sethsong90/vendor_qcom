/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_dec_ac3.cpp
This module contains the class definition for openMAX decoder component.

Copyright (c) 2012 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*=====================================================================*/
/*=========================================================================
                            Edit History

$Header:
when       who     what, where, why
--------   ---     -------------------------------------------------------
=========================================================================*/
#include "omx_ac3_adec.h"
#include <unistd.h>

using namespace std;
void *get_omx_component_factory_fn()
{
    return (new COmxDecAc3);
}
COmxDecAc3::COmxDecAc3()
:COmxBaseDec((const char*)"/dev/msm_ac3",OMX_ADEC_DEFAULT_SF,OMX_ADEC_DEFAULT_CH_CFG)
{
    set_in_buf_size(OMX_CORE_INPUT_BUFFER_SIZE);
    set_out_buf_size(OMX_AC3_OUTPUT_BUFFER_SIZE);

    // pass device name(/dev/msm_ac3) in the constructor
    DEBUG_PRINT("COmxDecAc3::%s, size %d\n",__FUNCTION__,OMX_AC3_OUTPUT_BUFFER_SIZE);
}

COmxDecAc3::~COmxDecAc3()
{
    DEBUG_PRINT("COmxDecAc3::%s\n",__FUNCTION__);
}

COmxDecAc3In::COmxDecAc3In( COmxDecAc3 * base, int fd,OMX_CALLBACKTYPE cb,
OMX_U32 buf_size, OMX_U32 sf, OMX_U8 ch,
OMX_BOOL pcm_feedback,
OMX_PTR appdata)
:COmxBaseDecIn(base,fd,cb,buf_size,sf,ch,pcm_feedback,appdata),
m_first_ac3_buffer(1),
m_ac3_comp(base),
m_data_written_to_dsp(0)
{
    DEBUG_PRINT("COmxDecAc3In::%s fd=%d buf_size=%u sf=%u ch=%u pcm=%u\n",\
    __FUNCTION__,fd,(unsigned int)buf_size,(unsigned int)sf,
    (unsigned int)ch,(unsigned int)pcm_feedback);
}

COmxDecAc3In::~COmxDecAc3In()
{
    DEBUG_PRINT_ERROR("COmxDecAc3In:%s\n",__FUNCTION__);
    m_first_ac3_buffer = 0;
    m_ac3_comp = NULL;
    m_data_written_to_dsp = 0;
    memset(&m_ac3_comp,0,sizeof(QOMX_AUDIO_PARAM_AC3TYPE));
}

/**
@brief member function for performing component initialization

@param role C string mandating role of this component
@return Error status
*/
OMX_ERRORTYPE COmxDecAc3::component_init(OMX_STRING role)
{
    DEBUG_PRINT_ERROR("COmxDecAc3::%s role[%s]\n",__FUNCTION__,role);
    struct ipc_info * pCtxt = NULL;
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    eRet = COmxBaseDec::component_init(role);
    if(eRet != OMX_ErrorNone)
    {
        return eRet;
    }

    set_port_recfg(1); // to suit the base class

    memset(&m_ac3_param, 0, sizeof(m_ac3_param));
    m_ac3_param.nSize = sizeof(m_ac3_param);
    m_ac3_param.nSamplingRate = OMX_ADEC_DEFAULT_SF;
    m_ac3_param.nChannels = OMX_ADEC_DEFAULT_CH_CFG;
    m_ac3_param.eFormat = omx_audio_ac3;
    m_ac3_param.eChannelConfig = OMX_AUDIO_AC3_CHANNEL_CONFIG_2_0;
    m_ac3_param.bCompressionOn = OMX_TRUE;
    m_ac3_param.bLfeOn = OMX_TRUE;
    m_ac3_param.bDelaySurroundChannels = OMX_TRUE;

    memset(&m_ac3_pp, 0, sizeof(m_ac3_pp));
    m_ac3_pp.nSize = sizeof(m_ac3_pp);
    m_ac3_pp.eChannelRouting[0] =  OMX_AUDIO_AC3_CHANNEL_LEFT;
    m_ac3_pp.eChannelRouting[1] =  OMX_AUDIO_AC3_CHANNEL_RIGHT;
    m_ac3_pp.eChannelRouting[2] =  OMX_AUDIO_AC3_CHANNEL_CENTER;
    m_ac3_pp.eChannelRouting[3] =  OMX_AUDIO_AC3_CHANNEL_LEFT_SURROUND;
    m_ac3_pp.eChannelRouting[4] =  OMX_AUDIO_AC3_CHANNEL_RIGHT_SURROUND;
    m_ac3_pp.eChannelRouting[5] =  OMX_AUDIO_AC3_CHANNEL_SURROUND;
    m_ac3_pp.eCompressionMode = OMX_AUDIO_AC3_COMPRESSION_MODE_LINE_OUT;
    m_ac3_pp.eStereoMode = OMX_AUDIO_AC3_STEREO_MODE_LO_RO;
    m_ac3_pp.eDualMonoMode = OMX_AUDIO_AC3_DUAL_MONO_MODE_STEREO;
    m_ac3_pp.usPcmScale = OMX_ADEC_DEFAULT_PCM_SCALE_FACTOR;
    m_ac3_pp.usDynamicScaleBoost = OMX_ADEC_DEFAULT_DYNAMIC_SCALE_BOOST;
    m_ac3_pp.usDynamicScaleCut = OMX_ADEC_DEFAULT_DYNAMIC_SCALE_CUT;
    m_ac3_pp.eKaraokeMode = OMX_AUDIO_AC3_KARAOKE_MODE_BOTH_VOCAL;

    DEBUG_PRINT("\n**************INIT*********************\n");

    if(!strcmp(role,"OMX.qcom.audio.decoder.ac3") ||
                     !strcmp(role, "OMX.qcom.audio.decoder.eac3"))
    {
        m_pOut = new COmxBaseDecOut(this,get_drv_fd(),get_cb(),get_pcm_feedback(),
        get_app_data(),OMX_AC3_OUTPUT_BUFFER_SIZE);
        DEBUG_PRINT("\nDecAc3:comp_init:role[%s]fd[%d]m_pOut[%p]bufsize=%d\n",\
                               role,get_drv_fd(), m_pOut,OMX_AC3_OUTPUT_BUFFER_SIZE);
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

    DEBUG_PRINT("DecAc3:Init fd=%d pcmfb=%d\n",get_drv_fd(),get_pcm_feedback());
    // create input and output port objects...
    m_pIn = new COmxDecAc3In(this,get_drv_fd(),get_cb(), OMX_CORE_INPUT_BUFFER_SIZE,
    OMX_ADEC_DEFAULT_SF,OMX_ADEC_DEFAULT_CH_CFG,
    get_pcm_feedback(), get_app_data() );

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
    m_pEvent = new COmxAc3EventHandler(this, get_drv_fd(), OMX_TRUE,OMX_StateLoaded);

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
OMX_ERRORTYPE  COmxDecAc3::get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
OMX_IN OMX_INDEXTYPE paramIndex,
OMX_INOUT OMX_PTR     paramData)
{
    DEBUG_PRINT("COmxDecAc3::%s\n",__FUNCTION__);
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
    //DEBUG_PRINT("COmxDecAc3::get_param, Index=%d\n",paramIndex);
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
                portDefn->format.audio.eEncoding = OMX_AUDIO_CodingAC3;
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
                DEBUG_PRINT("portDefn->nBufferSize %d, get_out_buf_size %d\n",portDefn->nBufferSize,get_out_buf_size());
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
                portFormatType->eEncoding = OMX_AUDIO_CodingAC3;
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

    case QOMX_IndexParamAudioAc3:
        {
            DEBUG_PRINT("GET PARAM-->OMX_IndexParamAudioAc3.. \
                SF=%u ch=%u\n",(unsigned int)m_ac3_param.nSamplingRate,
            (unsigned int)m_ac3_param.nChannels);
            QOMX_AUDIO_PARAM_AC3TYPE *ac3Param =
            (QOMX_AUDIO_PARAM_AC3TYPE *) paramData;

            if (OMX_CORE_INPUT_PORT_INDEX== ac3Param->nPortIndex)
            {
                memcpy(ac3Param,&m_ac3_param,
                sizeof(QOMX_AUDIO_PARAM_AC3TYPE));
            }
            else
            {
                DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioAc3 \
                    OMX_ErrorBadPortIndex %d\n", (int)ac3Param->nPortIndex);
                eRet = OMX_ErrorBadPortIndex;
            }
            break;
        }

    case QOMX_IndexParamAudioAc3PostProc:
        {
            DEBUG_PRINT("GET PARAM-->OMX_IndexParamAudioAc3PostProc.. \
                routing_0=%u routing_2=%u\n",(unsigned int)m_ac3_pp.eChannelRouting[0],
                (unsigned int)m_ac3_pp.eChannelRouting[1]);
            QOMX_AUDIO_PARAM_AC3PP *ac3PP =
            (QOMX_AUDIO_PARAM_AC3PP *) paramData;

            if (OMX_CORE_INPUT_PORT_INDEX== ac3PP->nPortIndex)
            {
                memcpy(ac3PP,&m_ac3_pp,
                sizeof(QOMX_AUDIO_PARAM_AC3PP));
            }
            else
            {
                DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioAc3PostProc \
                    OMX_ErrorBadPortIndex %d\n", (int)ac3PP->nPortIndex);
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
                pcmparam->nSamplingRate = m_ac3_param.nSamplingRate;
                pcmparam->nChannels = m_ac3_param.nChannels;

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
    DEBUG_PRINT("Before returning from COmxDecAc3:get_param\n");
    return eRet;
}


/**
@brief member function that set paramter from IL client

@param hComp handle to component instance
@param paramIndex parameter type
@param paramData pointer to memory space which holds the paramter
@return error status
*/
OMX_ERRORTYPE  COmxDecAc3::set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
OMX_IN OMX_INDEXTYPE paramIndex,
OMX_IN OMX_PTR        paramData)
{
    DEBUG_PRINT("COmxDecAc3::%s\n",__FUNCTION__);
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
    case QOMX_IndexParamAudioAc3:
        {
            DEBUG_PRINT("SET-PARAM::OMX_IndexParamAudioAC3");
            memcpy(&m_ac3_param,paramData,
                                      sizeof(QOMX_AUDIO_PARAM_AC3TYPE));
            DEBUG_PRINT("SF=%u Ch=%u\n",
            (unsigned int)m_ac3_param.nSamplingRate,
            (unsigned int)m_ac3_param.nChannels);
            if((m_ac3_param.nSamplingRate != 48000) && m_ac3_param.nSamplingRate != 44100 &&
                m_ac3_param.nSamplingRate != 32000)
            {
                DEBUG_PRINT_ERROR("Sampling rate detected %u is not supported\n",\
                                        (unsigned int)m_ac3_param.nSamplingRate);
                return OMX_ErrorBadParameter;
            }
            if((m_ac3_param.nChannels > 2) || (m_ac3_param.nChannels == 0))
            {
                DEBUG_PRINT_ERROR("num of ch detected %u is not supported falling back to 2\n",\
                                          (unsigned int)m_ac3_param.nChannels);
                m_ac3_param.nChannels = 2;
            }
            set_sf(m_ac3_param.nSamplingRate);
            set_ch(m_ac3_param.nChannels);
            m_pIn->set_sf(m_ac3_param.nSamplingRate);
            m_pIn->set_ch(m_ac3_param.nChannels);

            break;
        }

    case QOMX_IndexParamAudioAc3PostProc:
        {
            DEBUG_PRINT("SET-PARAM::OMX_IndexParamAudioAC3PostProc");
            memcpy(&m_ac3_pp,paramData,
                                      sizeof(QOMX_AUDIO_PARAM_AC3PP));
            DEBUG_PRINT("GET PARAM-->OMX_IndexParamAudioAc3PostProc.. \
                routing_0=%u routing_2=%u\n",(unsigned int)m_ac3_pp.eChannelRouting[0],
                (unsigned int)m_ac3_pp.eChannelRouting[1]);
            break;
        }
    case OMX_IndexParamAudioPortFormat:
        {
            OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
            (OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
            DEBUG_PRINT("set_parameter: OMX_IndexParamAudioPortFormat\n");

            if (OMX_CORE_INPUT_PORT_INDEX== portFormatType->nPortIndex)
            {
                portFormatType->eEncoding = OMX_AUDIO_CodingAC3;
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
COmxDecAc3::GetExtensionIndex

DESCRIPTION
OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  COmxDecAc3::get_extension_index(
OMX_IN OMX_HANDLETYPE      hComp,
OMX_IN OMX_STRING      paramName,
OMX_OUT OMX_INDEXTYPE* indexType)
{
    OMX_ERRORTYPE eRet;
    eRet = COmxBase::get_extension_index(hComp, paramName, indexType);

    if (eRet != OMX_ErrorNone) {
       if(strncmp(paramName,"OMX.Qualcomm.index.audio.ac3",
           strlen("OMX.Qualcomm.index.audio.ac3")) == 0)
        {
            *indexType =(OMX_INDEXTYPE)QOMX_IndexParamAudioAc3;
            DEBUG_PRINT("Extension index type - %d\n", *indexType);
        }
       else if(strncmp(paramName,"OMX.Qualcomm.index.audio.postproc.ac3",
           strlen("OMX.Qualcomm.index.audio.postproc.ac3")) == 0)
        {
            *indexType =(OMX_INDEXTYPE)QOMX_IndexParamAudioAc3PostProc;
            DEBUG_PRINT("Extension index type - %d\n", *indexType);
        }
       else
           return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;;
}

/* ======================================================================
FUNCTION
COmxDecAc3::ComponentRoleEnum

DESCRIPTION
OMX Component Role Enum method implementation.

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  COmxDecAc3::component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
OMX_OUT OMX_U8*        role,
OMX_IN OMX_U32        index)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    const char *cmp_role = "audio_decoder.ac3";

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
COmxDecAc3::ComponentDeInit

DESCRIPTION
Destroys the component and release memory allocated to the heap.

PARAMETERS
<TBD>.

RETURN VALUE
OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  COmxDecAc3::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
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
    DEBUG_PRINT("COmxDecAc3::%s\n",__FUNCTION__);
    eRet = COmxBaseDec::component_deinit(hComp);
    return eRet;
}

void COmxDecAc3In::process_in_port_msg(unsigned char id)
{
    unsigned      p1;       // Parameter - 1
    unsigned      p2;       // Parameter - 2
    unsigned char ident;
    unsigned      qsize     = 0;
    unsigned      tot_qsize = 0;
    OMX_STATETYPE state;

    DEBUG_PRINT("COmxDecAc3In::%s id=%d\n",__FUNCTION__,id);
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

OMX_ERRORTYPE  COmxDecAc3In::process_etb(
OMX_IN OMX_HANDLETYPE         hComp,
OMX_BUFFERHEADERTYPE* buffer)
{
    int res = 0;
    struct msm_audio_config drv_config;
    OMX_STATETYPE state;
    META_IN meta_in;
    struct QOMX_AUDIO_PARAM_AC3TYPE temp;
    struct QOMX_AUDIO_PARAM_AC3PP temp_1;
    DEBUG_PRINT_ERROR("COmxDecAc3In::%s\n",__FUNCTION__);

    /* Assume empty this buffer function has already checked
    validity of buffer */

    DEBUG_PRINT("ETBP:m_ac3_comp=%p\n",m_ac3_comp);
    if(m_ac3_comp->get_eos_bm())
    {
        m_ac3_comp->set_eos_bm(0);
    }
    if(buffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
        unsigned eos_bm=0;
        eos_bm= get_comp()->get_eos_bm();
        eos_bm |= IP_PORT_BITMASK;
        get_comp()->set_eos_bm(eos_bm);
        DEBUG_PRINT_ERROR("ETBP:EOSFLAG=%d\n",eos_bm);
    }
    temp = m_ac3_comp->get_ac3_param();
    memcpy(&m_ac3_param, &temp, sizeof(QOMX_AUDIO_PARAM_AC3TYPE));

    temp_1 = m_ac3_comp->get_ac3_pp();
    memcpy(&m_ac3_pp, &temp_1, sizeof(QOMX_AUDIO_PARAM_AC3PP));

    if(m_first_ac3_buffer) {
        m_first_ac3_buffer = 0;

       if(ioctl(get_drv_fd(), AUDIO_GET_CONFIG, &drv_config) == -1)
            DEBUG_PRINT("ETBP:get-config ioctl failed %d\n",errno);
       if(get_comp()->get_pcm_feedback())
            drv_config.meta_field = 1;
       else
            drv_config.meta_field = 0;

       drv_config.sample_rate = m_ac3_param.nSamplingRate;
       drv_config.channel_count = m_ac3_param.nChannels;
       drv_config.buffer_size = get_comp()->get_in_buf_size();
       drv_config.buffer_count = get_comp()->get_inp_act_buf_cnt();

       if(ioctl(get_drv_fd(), AUDIO_SET_CONFIG, &drv_config) == -1)
           DEBUG_PRINT("ETBP:set-config ioctl failed %d\n",errno);
       else
           DEBUG_PRINT("ETBP: set config success\n");
       set_ac3_config();

       if ( ioctl(get_drv_fd(), AUDIO_START, 0) < 0 )
       {
           DEBUG_PRINT("ETBP:ioctl audio start failed %d\n",errno);
           get_comp()->post_command((unsigned)OMX_CommandStateSet,
           (unsigned)OMX_StateInvalid,
           OMX_COMPONENT_GENERATE_COMMAND);
           get_comp()->post_command(OMX_CommandFlush,-1,
                       OMX_COMPONENT_GENERATE_COMMAND);
           buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
           m_first_ac3_buffer = 1;
           get_comp()->set_dec_state(false);
           return OMX_ErrorInvalidState;
       }
       else
           DEBUG_PRINT("ETBP: audio_start success\n");
       get_comp()->set_dec_state(true);
    }
    m_ac3_comp->set_ac3_param(&m_ac3_param);

    OMX_U32 len=0,dst_len =0;
    OMX_U8 *data=NULL;

    data = get_tmp_meta_buf();
    process_ac3(buffer,data,len,meta_in);
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
        // Advance the pointer by size of META_IN as process_ac3
        // would be inserting payloads considering the size of META_IN as well;
        // In other words, ignore META_IN in tunnel mode
        data = data+sizeof(META_IN);
    }
    if(m_input_port_flushed == OMX_TRUE)
        m_input_port_flushed = OMX_FALSE;

    if(get_comp()->get_flush_stat())
    {
        buffer->nFilledLen = 0;
        buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
        return OMX_ErrorNone;
    }

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

    state = get_state();

    if (state == OMX_StateExecuting)
    {
        DEBUG_DETAIL("ETBP:EBD CB buffer=0x%x\n",(unsigned int)buffer);
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

OMX_ERRORTYPE  COmxDecAc3In::set_ac3_config()
{
    DEBUG_PRINT("COmxDecAc3In::%s\n",__FUNCTION__);
    struct msm_audio_ac3_config ac3_config;
    int drv_fd = get_drv_fd();

    DEBUG_PRINT("Set-ac3-config: Ch=%u SF=%u\n",
    (unsigned int)m_ac3_param.nChannels,
    (unsigned int)m_ac3_param.nSamplingRate);

    if (ioctl(drv_fd, AUDIO_GET_AC3_CONFIG, &ac3_config)) {
        DEBUG_PRINT("omx_ac3_adec::set_ac3_config():GET_AC3_CONFIG failed");
        if(drv_fd >= 0){
            get_comp()->post_command((unsigned)OMX_CommandStateSet,
            (unsigned)OMX_StateInvalid,
            OMX_COMPONENT_GENERATE_COMMAND);
        }
        return OMX_ErrorInvalidComponent;
    }
    else
        DEBUG_PRINT("ETBP: get ac3 config success\n");
    ac3_config.numChans = m_ac3_param.nChannels;
    ac3_config.kCapableMode = m_ac3_pp.eKaraokeMode;
    ac3_config.compMode = m_ac3_pp.eCompressionMode;
    ac3_config.outLfeOn = m_ac3_param.bLfeOn;
    ac3_config.outputMode = m_ac3_param.eChannelConfig;
    ac3_config.stereoMode = m_ac3_pp.eStereoMode;
    ac3_config.dualMonoMode = m_ac3_pp.eDualMonoMode;
    ac3_config.fsCod = m_ac3_param.nSamplingRate;
    ac3_config.pcmScaleFac = m_ac3_pp.usPcmScale;
    ac3_config.dynRngScaleHi = m_ac3_pp.usDynamicScaleCut;
    ac3_config.dynRngScaleLow = m_ac3_pp.usDynamicScaleBoost;
    ac3_config.channel_routing_mode[0]= m_ac3_pp.eChannelRouting[0];
    ac3_config.channel_routing_mode[1]= m_ac3_pp.eChannelRouting[1];
    ac3_config.channel_routing_mode[2]= m_ac3_pp.eChannelRouting[2];
    ac3_config.channel_routing_mode[3]= m_ac3_pp.eChannelRouting[3];
    ac3_config.channel_routing_mode[4]= m_ac3_pp.eChannelRouting[4];
    ac3_config.channel_routing_mode[5]= m_ac3_pp.eChannelRouting[5];

    if (ioctl(drv_fd, AUDIO_SET_AC3_CONFIG, &ac3_config)) {
        DEBUG_PRINT("set_ac3_config():AUDIO_SET_AC3_CONFIG failed");
        get_comp()->post_command((unsigned)OMX_CommandStateSet,
        (unsigned)OMX_StateInvalid,
        OMX_COMPONENT_GENERATE_COMMAND);
        return OMX_ErrorInvalidComponent;
    }
    else
    DEBUG_PRINT("ETBP: set ac3 config success\n");
    return OMX_ErrorNone;
}

COmxAc3EventHandler::COmxAc3EventHandler(COmxDecAc3* base, int drv_fd,
OMX_BOOL suspensionPolicy,
OMX_STATETYPE state):
COmxBaseEventHandler(base, drv_fd, suspensionPolicy, state),
m_comp(base)
{
    DEBUG_PRINT("COmxAc3EventHandler::%s\n",__FUNCTION__);

}
COmxAc3EventHandler::~COmxAc3EventHandler()
{
    DEBUG_PRINT("COmxAc3EventHandler::%s\n",__FUNCTION__);
    DEBUG_PRINT("Destructor, COmxAc3EventHandler...\n");
}

OMX_ERRORTYPE COmxAc3EventHandler::processEvents()
{
    struct msm_audio_event       dsp_event;
    int rc = 0;
    DEBUG_PRINT("COmxAc3EventHandler::%s\n",__FUNCTION__);
    while(1)
    {
        DEBUG_PRINT("PE:COmxAc3EventHandler Waiting for AUDIO_GET_EVENT\n");
        DEBUG_PRINT("COmxAc3EventHandler::%s\n",__FUNCTION__);
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

OMX_ERRORTYPE COmxDecAc3In::process_ac3(
                                OMX_BUFFERHEADERTYPE  *buffer,
                                OMX_U8                *pDest,
                                OMX_U32               &len,
                                META_IN               &meta_in)
{
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

    memcpy(&pDest[iLen],((OMX_BUFFERHEADERTYPE*)buffer)->pBuffer,
                      ((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen);
/*
    if(buffer->nFilledLen < OMX_ADEC_SIZEOF_BUF_WITHOUT_META) {
	memset(&pDest[iLen+buffer->nFilledLen], 0,
                        OMX_ADEC_SIZEOF_BUF_WITHOUT_META - buffer->nFilledLen);
        buffer->nFilledLen = OMX_ADEC_SIZEOF_BUF_WITHOUT_META;
    }
*/
    iLen += ((OMX_BUFFERHEADERTYPE*)buffer)->nFilledLen;

    COmxBaseIn::m_in_pb_stat.tot_in_buf_len += buffer->nFilledLen;
    // Ignore the len of META_IN for now; If NT mode, len will be incremented by 14 after
    // this function returns; if tunnel mode ignore the size of META_IN
    // and advance the data pointer by size of META_IN
    len = iLen - sizeof(META_IN);

    return OMX_ErrorNone;
}

void COmxDecAc3::process_command_msg( unsigned char id1)
{
    unsigned     p1;         // Parameter - 1
    unsigned     p2;         // Parameter - 2
    unsigned     ident = 0;
    unsigned     qsize  = 0;
    unsigned     char id=0;
    omx_cmd_queue *cmd_q  = get_cmd_q();
    qsize = get_q_size(cmd_q);
    (void)id1;

    DEBUG_PRINT("COmxAc3::%s qsize[%d] state[%d]",__FUNCTION__,
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

    default:
        {
            DEBUG_PRINT_ERROR("CMD->state[%d]id[%d]\n",get_state(),ident);
        }
    }
    return;
}
