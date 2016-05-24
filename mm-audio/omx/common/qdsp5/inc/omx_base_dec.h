#ifndef _OMX_BASE_DEC_H_
#define _OMX_BASE_DEC_H_

/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_base_dec.h
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

#include"omx_base.h"

#define IP_PORT_BITMASK                 0x02
#define OP_PORT_BITMASK                 0x01
#define IP_OP_PORT_BITMASK              0x03
#define OMX_TCXO_BUFFER                 (256*1024)

#ifdef _ANDROID_
#undef LOG_TAG
#define LOG_TAG "QC_BASEDEC"
#endif

class omxBufMgr;

class COmxBaseDec: public COmxBase
{
public:
    COmxBaseDec(const char *devName, unsigned int sf, OMX_U8 ch);

    virtual ~COmxBaseDec();

    virtual OMX_ERRORTYPE component_init(
    OMX_STRING role);

    virtual OMX_ERRORTYPE component_deinit(
    OMX_HANDLETYPE hComp);

    virtual OMX_ERRORTYPE allocate_buffer(
    OMX_HANDLETYPE       hComp,
    OMX_BUFFERHEADERTYPE **bufferHdr,
    OMX_U32              port,
    OMX_PTR              appData,
    OMX_U32              bytes);

    OMX_ERRORTYPE process_cmd(OMX_HANDLETYPE  hComp,
    OMX_COMMANDTYPE cmd,
    OMX_U32         param1,
    OMX_PTR         cmdData);
private:

    ///////////////////////////////////////////////////////////
    // Private Methods
    ///////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////
    // Declarations
    ///////////////////////////////////////////////////////////////////////

    OMX_U8                         m_ch_cfg;
    unsigned int                   m_sample_rate;
    OMX_U8                         m_devicename[256];
};

class COmxBaseDecIn: public COmxBaseIn
{
public:
    COmxBaseDecIn(COmxBase * base, int fd,OMX_CALLBACKTYPE cb,
    OMX_U32 buf_size, OMX_U32 sf, OMX_U8 ch,
    OMX_BOOL pcm_feedback,
    OMX_PTR appdata);

    virtual ~COmxBaseDecIn();

    //virtual transcode_data(); //tbd

    virtual void process_in_port_msg( unsigned char id);

    inline OMX_U8 get_ch_cfg()
    {
        return m_ch_cfg;
    }
    inline void set_ch_cfg(OMX_U8 ch_cfg)
    {
        m_ch_cfg = ch_cfg;
    }
    inline unsigned int get_sample_rate()
    {
        return m_sample_rate;
    }
    inline void set_sample_rate(unsigned int sample_rate)
    {
        m_sample_rate = sample_rate;
    }
    inline OMX_U8* get_tmp_meta_buf()
    {
        if(m_tmp_meta_buf)
        return m_tmp_meta_buf;
        else
            return NULL;
    }
    bool omx_fake_eos(void);
private:

    unsigned int  m_sample_rate;
    OMX_U8        m_ch_cfg;
    OMX_U8        *m_tmp_meta_buf;

    OMX_PTR       m_app_data;
    OMX_U32       m_default_bufsize;
    OMX_BOOL      m_mode;
};

class COmxBaseDecOut: public COmxBaseOut
{
public:
    COmxBaseDecOut(COmxBase * base, int fd,OMX_CALLBACKTYPE cb,
    OMX_BOOL pcm_feedback, OMX_PTR appdata, int buf_size);
    ~COmxBaseDecOut();

    void process_out_port_msg( unsigned char id);

private:

    bool process_ftb(
    OMX_IN OMX_HANDLETYPE hComp,
    OMX_BUFFERHEADERTYPE  *buffer);

    void append_data_to_temp_buf(void);

    int          m_drv_fd;

    OMX_PTR      m_appdata;

    OMX_BOOL     m_mode;
    OMX_BOOL     fake_eos_recieved;

    OMX_U8       *m_tmp_out_meta_buf;
    OMX_U32      m_bufsize;
    omxBufMgr    *m_bufMgr;
};


class omxBufMgr
{
public:
    omxBufMgr():
             m_remaining_space(OMX_TCXO_BUFFER-1),
             m_rejected_bytes(0),
             m_tot_rejected_bytes(0),
             m_start(NULL),
             m_read(NULL),
             m_write(NULL),
             m_end(NULL)
    {
        m_start = (unsigned char*)
                     malloc(OMX_TCXO_BUFFER*sizeof(unsigned char));
        memset(m_start,0,OMX_TCXO_BUFFER);

        m_end = m_start + OMX_TCXO_BUFFER;
        m_write = m_start;
        m_read = m_start;
        DEBUG_PRINT("%s m_start[%p] m_end[%p]\n",__FUNCTION__,m_start,m_end);
    }

    ~omxBufMgr()
    {
       m_read = NULL;
       m_write= NULL;
       m_end= NULL;
       m_rejected_bytes = 0;
       m_tot_rejected_bytes = 0;
       if(m_start)
           free(m_start);
    }
    inline void reset()
    {
        m_read = m_start;
        m_write = m_start;
        m_remaining_space = OMX_TCXO_BUFFER-1;
        m_rejected_bytes = 0;
    }
    inline bool isEmpty()
    {
        if(m_remaining_space == (OMX_TCXO_BUFFER -1))
            return true;
        else
            return false;
    }
    inline OMX_U32 getBufFilledSpace()
    {
        return (OMX_TCXO_BUFFER -1 - m_remaining_space);
    }
    inline OMX_U32 getBufFreeSpace()
    {
        return (m_remaining_space);
    }

    OMX_U32  appendToBuf(OMX_U8 *srcBuf, OMX_U32 srcLen);
    OMX_U32  emptyToBuf(OMX_U8 *destBuf, OMX_U32 len );

    void print();
private:
    OMX_U32  m_remaining_space;
    OMX_U32  m_rejected_bytes;
    OMX_U32  m_tot_rejected_bytes;
    OMX_U8   *m_start;
    OMX_U8   *m_read;
    OMX_U8   *m_write;
    OMX_U8   *m_end;
};

#endif //_OMX_BASE_DEC_H


