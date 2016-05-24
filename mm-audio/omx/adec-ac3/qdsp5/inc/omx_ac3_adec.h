#ifndef _OMX_AC3_DEC_H_
#define _OMX_AC3_DEC_H_
/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_dec_ac3.h
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
#include "omx_base_dec.h"
#include <linux/msm_audio_ac3.h>
#include "QOMX_AudioIndexExtensions.h"
#include "QOMX_AudioExtensions.h"

#ifdef _ANDROID_
#undef LOG_TAG
#define LOG_TAG "QC_AC3DEC"
#endif


//////////////////////////////////////////////////////////////////
//         DEFINES
//////////////////////////////////////////////////////////////////
#define OMX_ADEC_DEFAULT_SF          44100
#define OMX_ADEC_DEFAULT_CH_CFG      2
#define OMX_ADEC_DEFAULT_PCM_SCALE_FACTOR 100
#define OMX_ADEC_DEFAULT_DYNAMIC_SCALE_BOOST 100
#define OMX_ADEC_DEFAULT_DYNAMIC_SCALE_CUT 100

#define OMX_CORE_INPUT_BUFFER_SIZE   4096
#define OMX_AC3_OUTPUT_BUFFER_SIZE   6144

//////////////////////////////////////////////////////////////////
//          TYPE DEFINITIONS
//////////////////////////////////////////////////////////////////
class COmxDecAc3;

class COmxAc3EventHandler: public COmxBaseEventHandler
{
public:
    COmxAc3EventHandler(COmxDecAc3 *base, int drv_fd,
    OMX_BOOL suspensionPolicy,
    OMX_STATETYPE state);
    ~COmxAc3EventHandler();

    OMX_ERRORTYPE processEvents();
    inline COmxDecAc3* get_comp()
    {
        return m_comp;
    }
private:
    COmxDecAc3    *m_comp;
};

class COmxDecAc3:public COmxBaseDec
{
public:
    COmxDecAc3();
    ~COmxDecAc3();

    OMX_ERRORTYPE component_init(
    OMX_STRING role);

    OMX_ERRORTYPE component_deinit(
    OMX_HANDLETYPE hComp);

    OMX_ERRORTYPE component_role_enum(
    OMX_HANDLETYPE hComp,
    OMX_U8         *role,
    OMX_U32        index);

    OMX_ERRORTYPE get_parameter(
    OMX_HANDLETYPE hComp,
    OMX_INDEXTYPE  paramIndex,
    OMX_PTR        paramData);

    OMX_ERRORTYPE get_extension_index(
    OMX_HANDLETYPE hComp,
    OMX_STRING     paramName,
    OMX_INDEXTYPE  *indexType);

    OMX_ERRORTYPE set_parameter(
    OMX_HANDLETYPE hComp,
    OMX_INDEXTYPE  paramIndex,
    OMX_PTR        paramData);
    void process_command_msg( unsigned char id);

    inline void set_ac3_param(QOMX_AUDIO_PARAM_AC3TYPE *ac3param)
    {
        memcpy(&m_ac3_param,ac3param,sizeof(QOMX_AUDIO_PARAM_AC3TYPE));
    }
    inline QOMX_AUDIO_PARAM_AC3TYPE get_ac3_param()
    {
        return m_ac3_param;
    }

    inline void set_ac3_pp(QOMX_AUDIO_PARAM_AC3PP *ac3pp)
    {
        memcpy(&m_ac3_pp,ac3pp,sizeof(QOMX_AUDIO_PARAM_AC3PP));
    }
    inline QOMX_AUDIO_PARAM_AC3PP get_ac3_pp()
    {
        return m_ac3_pp;
    }
private:

    ////////////////////////////////////////////////////////////////////////
    // Declarations
    ////////////////////////////////////////////////////////////////////////

    QOMX_AUDIO_PARAM_AC3TYPE m_ac3_param;
    QOMX_AUDIO_PARAM_AC3PP m_ac3_pp;
};

class COmxDecAc3In: public COmxBaseDecIn
{
public:
    COmxDecAc3In(COmxDecAc3* base, int fd,OMX_CALLBACKTYPE cb,
    OMX_U32 buf_size, OMX_U32 sf, OMX_U8 ch,
    OMX_BOOL pcm_feedback,
    OMX_PTR appdata);
    ~COmxDecAc3In();

    virtual void process_in_port_msg( unsigned char id);

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


    ///////////////////////////////////////////////////////////////
    // Private method
    ///////////////////////////////////////////////////////////////
    OMX_ERRORTYPE process_etb(OMX_IN OMX_HANDLETYPE hComp,
    OMX_BUFFERHEADERTYPE  *buffer);
    OMX_ERRORTYPE process_ac3(
                                 OMX_BUFFERHEADERTYPE  *pBufHdr,
                                 OMX_U8                *pDest,
                                 OMX_U32               &len,
                                 META_IN               &meta_in);
    OMX_ERRORTYPE  set_ac3_config();

    COmxDecAc3                     *m_ac3_comp;
    int                            m_first_ac3_buffer;
    bool                           m_data_written_to_dsp;
    QOMX_AUDIO_PARAM_AC3TYPE       m_ac3_param;
    QOMX_AUDIO_PARAM_AC3PP         m_ac3_pp;
    OMX_BOOL                        m_bsac;
};

class COmxDecAc3Out: public COmxBaseDecOut
{
public:
    COmxDecAc3Out(COmxBase * base,
    int fd,OMX_CALLBACKTYPE cb,
    OMX_BOOL pcm_feedback, OMX_PTR appdata,int buf_size):
    COmxBaseDecOut(base,fd,cb,pcm_feedback,appdata,buf_size){}

    ~COmxDecAc3Out();

private:
    //FTBP
    bool process_ftb(OMX_IN OMX_HANDLETYPE hComp,
    OMX_BUFFERHEADERTYPE  *buffer);

};
#endif // _OMX_AC3_DEC_H_
