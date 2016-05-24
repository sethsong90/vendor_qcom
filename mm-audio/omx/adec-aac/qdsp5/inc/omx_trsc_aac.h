#ifndef _OMX_TRNS_AAC_H_
#define _OMX_TRSC_AAC_H_

/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_aac_transcoder.h
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

#include "omx_base_trsc.h"

class COmxBsacTranscoder:public COmxBaseTranscoder
{
public:
    COmxBsacTranscoder(OMX_U32 inBufSz, OMX_U32 residueBufSz,OMX_U16 metainBufSz);

    virtual ~COmxBsacTranscoder();

    virtual OMX_U8 *transcodeData(OMX_U8* src, OMX_U32 src_len,OMX_U32 *dest_len);
    OMX_U8 transcode_frame(OMX_U8 **src,OMX_U32 srcDataLen,OMX_U32 *srcDataConsumed);
private:
    OMX_U8 *m_trans_buffer_start;
    OMX_U8 *m_residual_buffer_start;
    OMX_U8 *m_trans_buffer;
    OMX_U8 *m_residual_buffer;
    OMX_U32 m_residual_data_len;
    OMX_BOOL m_is_full_frame;
    OMX_U32 m_inBufSz;
    OMX_U32 m_residueBufSz;
    OMX_U32 m_metainBufSz;
    OMX_U32 bytesTranscoded;    
};

#endif // OMX_AAC_TRANSCODER_H_

