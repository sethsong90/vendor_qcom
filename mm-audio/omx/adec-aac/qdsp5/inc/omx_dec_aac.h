#ifndef _OMX_AAC_DEC_H_
#define _OMX_AAC_DEC_H_
/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_dec_aac.h
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
#include <linux/msm_audio_aac.h>
#include "QOMX_AudioIndexExtensions.h"
#include "QOMX_AudioExtensions.h"

#ifdef _ANDROID_
#undef LOG_TAG
#define LOG_TAG "QC_AACDEC"
#endif


//////////////////////////////////////////////////////////////////
//         DEFINES
//////////////////////////////////////////////////////////////////
#define OMX_ADEC_DEFAULT_SF          44100
#define OMX_ADEC_DEFAULT_CH_CFG      2

#define OMX_CORE_INPUT_BUFFER_SIZE   8192
#define OMX_AAC_OUTPUT_BUFFER_SIZE   9216
#define OMX_ADEC_AAC_FRAME_LEN       1024
#define AAC_RESIDUALDATA_BUFFER_SIZE 2048 
// 14 bytes for input meta data and 4bytes for pseudo raw header
#define OMX_ADEC_SIZEOF_META_BUF     (OMX_CORE_INPUT_BUFFER_SIZE+18)

/* The following defines are used to extract all data from the AAC header.
** Each is divided into
**  the byte offset into the header
**  the byte mask for the bits
**  the number of bits to right-shift to extract a 0-based value
*/
#define AAC_SAMPLING_FREQ_INDEX_SIZE          4
#define AAC_ORIGINAL_COPY_SIZE                1
#define AAC_HOME_SIZE                         1
#define AAC_COPYRIGHT_PRESENT_SIZE            1
#define AAC_PROFILE_SIZE                      2
#define AAC_BITSTREAM_TYPE_SIZE               1
#define AAC_BITRATE_SIZE                      23
#define AAC_NUM_PFE_SIZE                      4
#define AAC_BUFFER_FULLNESS_SIZE              20
#define AAC_ELEMENT_INSTANCE_TAG_SIZE         4
#define AAC_NUM_FRONT_CHANNEL_ELEMENTS_SIZE   4
#define AAC_NUM_SIDE_CHANNEL_ELEMENTS_SIZE    4
#define AAC_NUM_BACK_CHANNEL_ELEMENTS_SIZE    4
#define AAC_NUM_LFE_CHANNEL_ELEMENTS_SIZE     2
#define AAC_NUM_ASSOC_DATA_ELEMENTS_SIZE      3
#define AAC_NUM_VALID_CC_ELEMENTS_SIZE        4
#define AAC_MONO_MIXDOWN_PRESENT_SIZE         1
#define AAC_MONO_MIXDOWN_ELEMENT_SIZE         4
#define AAC_STEREO_MIXDOWN_PRESENT_SIZE       1
#define AAC_STEREO_MIXDOWN_ELEMENT_SIZE       4
#define AAC_MATRIX_MIXDOWN_PRESENT_SIZE       1
#define AAC_MATRIX_MIXDOWN_SIZE               3
#define AAC_FCE_SIZE                          5
#define AAC_SCE_SIZE                          5
#define AAC_BCE_SIZE                          5
#define AAC_LFE_SIZE                          4
#define AAC_ADE_SIZE                          4
#define AAC_VCE_SIZE                          5
#define AAC_COMMENT_FIELD_BYTES_SIZE          8
#define AAC_COMMENT_FIELD_DATA_SIZE           8


#define AAC_MONO_SILENCE_FRAME_SIZE           10
#define AAC_STEREO_SILENCE_FRAME_SIZE         11
//10 bytes
OMX_U8 AAC_MONO_SILENCE_FRAME_DATA[]   =
{0x01, 0x40, 0x20, 0x06, 0x4F, 0xDE, 0x02, 0x70, 0x0C, 0x1C};
// 11 bytes
OMX_U8 AAC_STEREO_SILENCE_FRAME_DATA[] =
{0x21, 0x10, 0x05, 0x00, 0xA0, 0x19, 0x33, 0x87, 0xC0, 0x00, 0x7E};
#define TIMESTAMP_HALFRANGE_THRESHOLD 0x7D0

#define LOAS_GA_SPECIFIC_CONFIG(o) \
    (((o != 5) && (o >=1 ) && (o <= 7)) || \
    ((o != 18) && (o >= 17) && (o <= 23)))

#define LOAS_IS_AUD_OBJ_SUPPORTED(x) \
    ((x == 2) || (x == 4) || (x == 5) || (x == 17))

#define LOAS_IS_SFI_SUPPORTED(x) ((x >= 3) && (x <= 0x0B))

/* c is channel config and o is Audio object type */
#define LOAS_IS_CHANNEL_CONFIG_SUPPORTED(c, o) \
    (((c <= 2) && ((o == 2) || (o == 4) || (o == 5))) || \
    (((c == 1) || (c == 2)) && (o == 17)))

#define LOAS_IS_EXT_SFI_SUPPORTED(x)  ((x >= 0x03) && (x <= 0x08))

/* Extension Flag is e and Audio object type is o */
#define LOAS_IS_EXT_FLG_SUPPORTED(e,o) \
    ((((o == 2) || (o == 4) || (o == 5)) && (e == 0)) || \
    ((o == 17) && (e == 1)))


//////////////////////////////////////////////////////////////////
//          TYPE DEFINITIONS
//////////////////////////////////////////////////////////////////
enum in_format{
    FORMAT_ADTS = 0,
    FORMAT_ADIF = 1,
    FORMAT_LOAS = 2,
    FORMAT_RAW  = 3,
};
struct adts_fixed_header
{
    OMX_U16 sync_word;
    OMX_U8  id;
    OMX_U8  layer;
    OMX_U8  protection_absent;
    OMX_U8  profile;
    OMX_U8  sampling_frequency_index;
    OMX_U8  private_bit;
    OMX_U8  channel_configuration;
    OMX_U8  original_copy;
    OMX_U8  home;
    OMX_U8  emphasis;
};
struct adts_var_header
{
    OMX_U8  copyright_id_bit;
    OMX_U8  copyright_id_start;
    OMX_U16 aac_frame_length;
    OMX_U8  adts_buffer_fullness;
    OMX_U8  no_raw_data_blocks_in_frame;
};
struct adts_header
{
    struct adts_fixed_header fixed;
    struct adts_var_header   var;
};
struct aac_raw
{
    OMX_U8 aud_obj_type;
    OMX_U8 freq_index;
    OMX_U8 channel_config;
    OMX_U8 sbr_present_flag;
    OMX_U8 sbr_ps_present_flag;
    OMX_U8 ext_aud_obj_type;
    OMX_U8 ext_freq_index;
    OMX_U8 ext_channel_config;
};
struct adif_header
{
    OMX_U8 variable_bit_rate;
    OMX_U8 aud_obj_type;
    OMX_U8 freq_index;
    OMX_U8 channel_config;
    OMX_U32 sample_rate;
};
struct loas_header
{
    OMX_U8 aud_obj_type;
    OMX_U8 freq_index;
    OMX_U8 channel_config;
};
struct aac_header
{
    in_format input_format;
    union
    {
        struct adts_header adts;
        struct adif_header adif;
        struct loas_header loas;
        struct aac_raw     raw;
    }head;
};
typedef enum {
    AAC_CHANNEL_UNKNOWN = 0,
    AAC_CHANNEL_MONO,       /* Single channel (mono) data*/
    AAC_CHANNEL_DUAL,       /* Stereo data*/
    AAC_CHANNEL_TRIPLE,     /* 3 channels: 1+2 (UNSUPPORTED)*/
    AAC_CHANNEL_QUAD,       /* 4 channels: 1+2+1 (UNSUPPORTED)*/
    AAC_CHANNEL_QUINTUPLE,  /* 5 channels: 1+2+2 (UNSUPPORTED)*/
    AAC_CHANNEL_SEXTUPLE,   /* 5+1 channels: 1+2+2+1 (UNSUPPORTED)*/
    AAC_CHANNEL_OCTUPLE,    /* 7+1 channels: 1+2+2+2+1 (UNSUPPORTED)*/
    AAC_CHANNEL_DUAL_MONO,  /* Dual Mono: 1+1 (Two SCEs)*/
    AAC_CHANNEL_UNSUPPORTED /* Indicating CMX is currently playing*/
    /* unsupported Channel mode.*/
} aac_channel_enum_type;

static OMX_U32 aac_frequency_index[16] = { 96000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350,
    0x000,//invalid index
    0x000,//invalid index
    0x000 // no index, value provided is actual frequency
};

///////////////////////////////////////////////
///////////////////////////////////////////////
#include "omx_trsc_aac.h" 
class COmxDecAac;
class silenceInsertion;

class COmxAacEventHandler: public COmxBaseEventHandler
{
public:
    COmxAacEventHandler(COmxDecAac *base, int drv_fd,
    OMX_BOOL suspensionPolicy,
    OMX_STATETYPE state);
    ~COmxAacEventHandler();

    OMX_ERRORTYPE processEvents();
    inline COmxDecAac* get_comp()
    {
        return m_comp;
    }
private:
    COmxDecAac    *m_comp;
};

class COmxAacEventHandler_7x27: public COmxBaseEventHandler
{
public:
    COmxAacEventHandler_7x27(COmxDecAac *base, int drv_fd,
    OMX_BOOL suspensionPolicy,
    OMX_STATETYPE state);
    ~COmxAacEventHandler_7x27();

    OMX_ERRORTYPE processEvents();
    inline COmxDecAac* get_comp()
    {
        return m_comp;
    }
private:
    COmxDecAac    *m_comp;
};


class COmxDecAac:public COmxBaseDec
{
public:
    //COmxDecAac(OMX_U8 *devName, unsigned int sf, OMX_U8 ch);
    COmxDecAac();
    ~COmxDecAac();

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

    inline void set_aac_param(OMX_AUDIO_PARAM_AACPROFILETYPE *aacparam)
    {
        memcpy(&m_aac_param,aacparam,sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
    }
    inline OMX_AUDIO_PARAM_AACPROFILETYPE& get_aac_param()
    {
        return m_aac_param;
    }
private:

    ////////////////////////////////////////////////////////////////////////
    // Declarations
    ////////////////////////////////////////////////////////////////////////

    OMX_AUDIO_PARAM_AACPROFILETYPE m_aac_param;
};

class COmxDecAacIn: public COmxBaseDecIn
{
public:
    COmxDecAacIn(COmxDecAac* base, int fd,OMX_CALLBACKTYPE cb,
    OMX_U32 buf_size, OMX_U32 sf, OMX_U8 ch,
    OMX_BOOL pcm_feedback,
    OMX_PTR appdata);
    ~COmxDecAacIn();

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
        m_first_aac_header = 0;
    };

private:


    ///////////////////////////////////////////////////////////////
    // Private method
    ///////////////////////////////////////////////////////////////
    OMX_ERRORTYPE process_etb(OMX_IN OMX_HANDLETYPE hComp,
    OMX_BUFFERHEADERTYPE  *buffer);
    OMX_ERRORTYPE process_aac( 
                                 OMX_BUFFERHEADERTYPE  *pBufHdr, 
                                 OMX_U8                *pDest, 
                                 OMX_U32               &len, 
                                 META_IN               &meta_in,bool adif= false); 
    OMX_ERRORTYPE  aac_frameheader_parser(OMX_BUFFERHEADERTYPE *buffer,
    struct aac_header    *header);

    OMX_ERRORTYPE  set_aac_config();

    void audaac_extract_adif_header(OMX_U8            *data,
    struct aac_header *aac_header_info);

    int audaac_extract_loas_header(OMX_U8            *data,OMX_U32 len,
    struct aac_header *aac_header_info);

    void audaac_extract_bits(OMX_U8*, OMX_U8, OMX_U32*);

    COmxDecAac                     *m_aac_comp;
    int                            m_first_aac_header;
    unsigned                       m_aac_hdr_bit_index;
    unsigned int                   m_bytes_to_skip;
    bool                           m_data_written_to_dsp;
    OMX_AUDIO_PARAM_AACPROFILETYPE m_aac_param;
    silenceInsertion               *m_silence;
    COmxBsacTranscoder             *m_pTranscoder;
    OMX_BOOL                        m_bsac;
    OMX_U32                        bytesTranscoded;
};
class COmxDecAacIn_7x27: public COmxBaseDecIn
{
public:
    COmxDecAacIn_7x27(COmxDecAac* base, int fd,OMX_CALLBACKTYPE cb,
    OMX_U32 buf_size, OMX_U32 sf, OMX_U8 ch,
    OMX_BOOL pcm_feedback,
    OMX_PTR appdata);
    ~COmxDecAacIn_7x27();

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
        m_first_aac_header = 0;
    };
    
private:


    ///////////////////////////////////////////////////////////////
    // Private method
    ///////////////////////////////////////////////////////////////
    OMX_ERRORTYPE process_etb(OMX_IN OMX_HANDLETYPE hComp,
    OMX_BUFFERHEADERTYPE  *buffer);
    OMX_ERRORTYPE process_aac(
                                 OMX_BUFFERHEADERTYPE  *pBufHdr,
                                 OMX_U8                *pDest,
                                 OMX_U32               &len,
                                 META_IN               &meta_in,bool adif= false);
    OMX_ERRORTYPE  aac_frameheader_parser(OMX_BUFFERHEADERTYPE *buffer,
    struct aac_header    *header);

    OMX_ERRORTYPE  set_aac_config();

    void audaac_extract_adif_header(OMX_U8            *data,
    struct aac_header *aac_header_info);

    int audaac_extract_loas_header(OMX_U8            *data,OMX_U32 len,
    struct aac_header *aac_header_info);

    void audaac_extract_bits(OMX_U8*, OMX_U8, OMX_U32*);

    COmxDecAac                     *m_aac_comp;
    int                            m_first_aac_header;
    unsigned                       m_aac_hdr_bit_index;
    unsigned int                   m_bytes_to_skip;
    bool                           m_data_written_to_dsp;
    OMX_AUDIO_PARAM_AACPROFILETYPE m_aac_param;
    silenceInsertion               *m_silence;
    COmxBsacTranscoder             *m_pTranscoder;
    OMX_BOOL                        m_bsac;
    OMX_U32                        bytesTranscoded;
};
class COmxDecAacOut: public COmxBaseDecOut
{
public:
    COmxDecAacOut(COmxBase * base,
    int fd,OMX_CALLBACKTYPE cb,
    OMX_BOOL pcm_feedback, OMX_PTR appdata,int buf_size):
    COmxBaseDecOut(base,fd,cb,pcm_feedback,appdata,buf_size){}

    ~COmxDecAacOut();

private:
    //FTBP
    bool process_ftb(OMX_IN OMX_HANDLETYPE hComp,
    OMX_BUFFERHEADERTYPE  *buffer);

};

class silenceInsertion
{
public:
    silenceInsertion():m_silenceInsertionRequired(OMX_FALSE),
    m_previousFrameTimestamp(0),
    m_currentFrameTimestamp(0),
    m_numSilenceFrames(0),
    m_milliSecPerFrame(0)
    {}

    ~silenceInsertion();

   OMX_BOOL  checkForTimeStampGap( OMX_U32 SF, OMX_U32 frameLen);

    OMX_ERRORTYPE  insertSilenceInBuffer(OMX_U8 *buffer,OMX_U32 frameLen, OMX_U8 numOfCh);

    inline OMX_BOOL silence_required()
    {
        return m_silenceInsertionRequired;
    }
    inline void set_curr_ts(OMX_TICKS ts)
    {
        m_currentFrameTimestamp = ts;
    }
    inline unsigned int get_curr_ts()
    {
        return m_currentFrameTimestamp ;
    }
    inline void set_prev_ts(OMX_TICKS ts)
    {
        m_previousFrameTimestamp = ts;
    }
    inline unsigned int get_num_sil_frame()
    {
        return m_numSilenceFrames;
    }
private:
    OMX_BOOL     m_silenceInsertionRequired;

    OMX_TICKS  m_previousFrameTimestamp;
    OMX_TICKS  m_currentFrameTimestamp;
    unsigned int m_numSilenceFrames;
    //Formula used: TS in ms = (samples * 1000/sampling_freq)
    unsigned int m_milliSecPerFrame ;
};

#endif // _OMX_AAC_DEC_H_
