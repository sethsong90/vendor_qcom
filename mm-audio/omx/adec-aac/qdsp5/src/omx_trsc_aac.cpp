/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_aac_transcoder.cpp
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

#include "omx_trsc_aac.h"
#ifdef _ANDROID_
#undef LOG_TAG
#define LOG_TAG "QC_AACTRNS"
#endif

COmxBsacTranscoder::COmxBsacTranscoder(OMX_U32 bufSz, OMX_U32 residueBufSz,
                                                      OMX_U16 metainBufSz):
	            m_trans_buffer_start(NULL),
                    m_residual_buffer_start(NULL),
                    m_trans_buffer(NULL),
                    m_residual_buffer(NULL),
                    m_residual_data_len(0),
                    m_is_full_frame(OMX_FALSE),
                    m_inBufSz(bufSz),
                    m_residueBufSz(residueBufSz),
                    m_metainBufSz(metainBufSz),
                    bytesTranscoded(0)
{
    DEBUG_PRINT("%s",__FUNCTION__);
    // Allocate mem here..
    if(!m_trans_buffer_start && !m_residual_buffer_start)
    {
        if ( !m_trans_buffer_start)
            m_trans_buffer_start = (OMX_U8*)malloc(sizeof(OMX_U8)*
                    (m_inBufSz + m_metainBufSz + (m_inBufSz * 8)));
        if ( m_trans_buffer_start == NULL )
        {
                    DEBUG_PRINT_ERROR("UseBuf: Mem alloc failed for m_trans_buffer\n");
                    return;
        }
        else
            DEBUG_PRINT("UseBuf: Mem alloc success for %p \n",m_trans_buffer_start);
        memset( m_trans_buffer_start, 0,
                   (sizeof(OMX_U8)*(m_inBufSz + m_metainBufSz + (m_inBufSz * 8))));
        m_trans_buffer = m_trans_buffer_start;
        m_residual_buffer_start =
                    (OMX_U8 *)malloc(sizeof(OMX_U8) * m_residueBufSz);
        if (NULL == m_residual_buffer_start)
        {
            return;
        }
        else
            DEBUG_PRINT("UseBuf: Mem alloc success for %p \n",m_residual_buffer_start);
        memset(m_residual_buffer_start, 0,
                  sizeof(OMX_U8) * m_residueBufSz);
        m_residual_buffer = m_residual_buffer_start;
    }
    else
    {
        DEBUG_PRINT("m_trans_buffer_start[%p] m_residual_buffer_start[%p]\n",\
                                                       m_trans_buffer_start,
                                                       m_residual_buffer_start);
    }
}


COmxBsacTranscoder::~COmxBsacTranscoder()
{
    DEBUG_PRINT("%s",__FUNCTION__);
    if(m_trans_buffer_start)
    {
        free(m_trans_buffer_start);
	m_trans_buffer_start = NULL;
	m_trans_buffer = NULL;
    }
    if(m_residual_buffer_start)
    {
        free(m_residual_buffer_start);
	m_residual_buffer_start = NULL;
	m_residual_buffer = NULL;
    }
    m_residual_data_len = 0;
    m_is_full_frame = OMX_FALSE;
}

OMX_U8 *COmxBsacTranscoder::transcodeData(OMX_U8* src, OMX_U32 src_len,
                                  OMX_U32 *dest_len)
{ 

    OMX_U8 *srcStart = src;
    OMX_U32 srcDataConsumed = 0;
    OMX_U8 *dst = 0;
    
    if(src_len)
    {
    
        m_trans_buffer =  m_trans_buffer_start;
        m_trans_buffer +=14;
        DEBUG_PRINT("INSIDE BSAC\n");
        while (srcDataConsumed < (src_len)){
            if (transcode_frame(&srcStart,src_len,&srcDataConsumed) == OMX_FALSE)
	    {
		    dst = 0;
		    *dest_len = 0;
		    return dst;
	    }
        }
        DEBUG_PRINT("\n Bytes Transcoded = %u\n", (unsigned int)bytesTranscoded);
        if ((signed)bytesTranscoded > 0)
        {
            *dest_len = bytesTranscoded;
            bytesTranscoded = 0;
            m_trans_buffer = m_trans_buffer_start;
            dst = m_trans_buffer;
        }

        return dst;
     }
     else
     {
         m_trans_buffer = m_trans_buffer_start;
         dst = m_trans_buffer;
         *dest_len=0;
         return dst;
     }

}
OMX_U8 COmxBsacTranscoder::transcode_frame(OMX_U8 **src,
                                     OMX_U32 srcDataLen,
                                     OMX_U32 *srcDataConsumed
                                     )
{

    unsigned short framelen = 0;
    unsigned short templen = 0;
    unsigned char temp = 0;
    DEBUG_PRINT("INSIDE TRANSCODE\n");
    /*First 11 bits of a frame is Frame length*/
    if (m_residual_data_len == 0)
    {
        framelen = **src;
        DEBUG_PRINT("**src is %d\n", **src);
        framelen = (framelen << 8) & 0xFF00;
        DEBUG_PRINT("Inside framelen\n");
        framelen = framelen | *(*src + 1);
        framelen = (framelen >> 5) & 0x07FF;
        DEBUG_PRINT("OUTSIDE\n");
        DEBUG_PRINT("framelen is %d\n", framelen);
	if (framelen > srcDataLen) {
		DEBUG_PRINT("framelen is greater than buffer size\n");
		return OMX_FALSE;
	}

    } else
    {
        framelen = *(m_residual_buffer);
        framelen = (framelen << 8) & 0xFF00;
        framelen = framelen | (*(m_residual_buffer + 1));
        framelen = (framelen >> 5) & 0x07FF;
        DEBUG_PRINT("m_residual_buffer is %d\n", framelen);

    }

    if (m_residual_data_len == 0)
    {

        if ((srcDataLen - *srcDataConsumed) >= framelen)
        {
            *m_trans_buffer= 0xFF;
            m_trans_buffer++;
            *m_trans_buffer = 0xFF;
            m_trans_buffer++;
            templen = framelen + 4;
            temp = templen >> 8;
            memcpy(m_trans_buffer, &temp, 1);
            m_trans_buffer++;
            temp = templen;
            memcpy(m_trans_buffer, &temp, 1);
            m_trans_buffer++;
            memcpy(m_trans_buffer, *src, framelen);
            *src += framelen;
            *srcDataConsumed += framelen;
             m_trans_buffer =  m_trans_buffer + framelen;
            for(int i = 0; i < 4; i++)
            {
                *m_trans_buffer = 0x00;
                m_trans_buffer++;
            }
            bytesTranscoded += framelen + 8;
            }
         else
         {
            memcpy(m_residual_buffer, *src, (srcDataLen - *srcDataConsumed));
            DEBUG_PRINT("m_residual_data_len =%u\n",(unsigned int)m_residual_data_len);
            m_residual_data_len += (srcDataLen - *srcDataConsumed);
            DEBUG_PRINT("m_residual_data_len =%d\n",(unsigned int)m_residual_data_len);
            *srcDataConsumed = srcDataLen;

        }
    } else
    {
        *m_trans_buffer = 0xFF;
         m_trans_buffer++;
        *m_trans_buffer = 0xFF;
         m_trans_buffer++;
         templen = framelen + 4;
         temp = templen >> 8;
         memcpy(m_trans_buffer, &temp, 1);
         m_trans_buffer++;
         temp = templen;
         memcpy(m_trans_buffer, &temp, 1);
         m_trans_buffer++;

         memcpy(m_trans_buffer, m_residual_buffer, m_residual_data_len);
         m_trans_buffer += m_residual_data_len;
        if (srcDataLen >= (framelen - m_residual_data_len))
        {
            DEBUG_PRINT("**src inside residual buffer =%d\n",**src);
            memcpy(m_trans_buffer, *src,
                (framelen - m_residual_data_len));
            *src += (framelen - m_residual_data_len);
             m_trans_buffer += (framelen - m_residual_data_len) ;
            *srcDataConsumed +=
                (framelen - m_residual_data_len);
            for(int i = 0; i < 4; i++)
            {
                *m_trans_buffer = 0x00;
                 m_trans_buffer++;
            }
            m_residual_data_len = 0;
            m_residual_buffer = m_residual_buffer_start;
            memset(m_residual_buffer, 0,
                   sizeof(OMX_U8) * m_residueBufSz);
            bytesTranscoded += framelen + 8;
        } else
        {
            memcpy(m_residual_buffer, *src, srcDataLen);
            *src += srcDataLen;
            *srcDataConsumed = srcDataLen;
            m_residual_data_len+=srcDataLen;
            DEBUG_PRINT_ERROR("Insufficient data\n");

        }

    }

    return OMX_TRUE;
}
