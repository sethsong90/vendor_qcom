/**********************************************************************************
  $Revision: #1 $
  Copyright 2003 - 2004 Open Interface North America, Inc. All rights reserved.
***********************************************************************************/

/**
@file
Functions for manipulating input bitstreams.

@ingroup codec_internal
*/

/**
@addtogroup codec_internal
@{
*/

#include "oi_stddefs.h"
#include "oi_bitstream.h"
#include "oi_assert.h"

PRIVATE void OI_BITSTREAM_ReadInit(OI_BITSTREAM *bs,
                                   const OI_BYTE *buffer)
{
    bs->value = ((OI_INT32)buffer[0] << 16) | ((OI_INT32)buffer[1] << 8) | (buffer[2]);
    bs->ptr.r = buffer + 3;
    bs->bitPtr = 8;
}

PRIVATE OI_UINT32 OI_BITSTREAM_ReadUINT(OI_BITSTREAM *bs, OI_UINT bits)
{
    OI_UINT32 result;

    OI_BITSTREAM_READUINT(result, bits, bs->ptr.r, bs->value, bs->bitPtr);

    return result;
}

PRIVATE OI_UINT8 OI_BITSTREAM_ReadUINT4Aligned(OI_BITSTREAM *bs)
{
    OI_UINT32 result;

    OI_ASSERT(bs->bitPtr < 16);
    OI_ASSERT(bs->bitPtr % 4 == 0);

    if (bs->bitPtr == 8) {
        result = bs->value << 8;
        bs->bitPtr = 12;
    } else {
        result = bs->value << 12;
        bs->value = (bs->value << 8) | *bs->ptr.r++;
        bs->bitPtr = 8;
    }
    result >>= 28;
    OI_ASSERT(result < (1u << 4));
    return (OI_UINT8)result;
}

PRIVATE OI_UINT8 OI_BITSTREAM_ReadUINT8Aligned(OI_BITSTREAM *bs)
{
    OI_UINT32 result;
    OI_ASSERT(bs->bitPtr == 8);

    result = bs->value >> 16;
    bs->value = (bs->value << 8) | *bs->ptr.r++;

    return (OI_UINT8)result;
}

/**
@}
*/


