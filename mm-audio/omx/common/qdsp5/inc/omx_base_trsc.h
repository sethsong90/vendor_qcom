#ifndef _OMX_BASE_TRSC_H_
#define _OMX_BASE_TRSC_H_

/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_base_transcoder.h
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
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "OMX_Core.h"
#include "OMX_Audio.h"
#include "qc_omx_component.h"
#include "qc_omx_msg.h"
class COmxBaseTranscoder
{
public:
    COmxBaseTranscoder();
    virtual ~COmxBaseTranscoder();

    virtual OMX_U8 *transcodeData(OMX_U8* src, OMX_U32 src_len,
		                   OMX_U32 *dest_len) =0;
private:

};

#endif // OMX_BASE_TRANSCODER_H_
