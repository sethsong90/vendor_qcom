#ifndef _OMX_BASE_ENC_H_
#define _OMX_BASE_ENC_H_
/*=========================================================================
                            O p e n M A X   Component
                                Audio Decoder
*//** @file omx_base_enc.h
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

class COmxBaseEnc: public COmxBase
{

};

class COmxBaseInEnc: public COmxBaseIn
{
public:
    COmxBaseInEnc(){}

    ~COmxBaseInEnc();

};

class COmxBaseOutEnc: public COmxBaseOut
{
    COmxBaseOutEnc(){}
    ~COmxBaseOutEnc();
};

#endif //_OMX_BASE_ENC_H_
