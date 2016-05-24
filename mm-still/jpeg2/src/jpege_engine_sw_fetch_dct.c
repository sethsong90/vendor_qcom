/*========================================================================

*//** @file jpege_engine_sw_fetch_dct.c

@par EXTERNALIZED FUNCTIONS
  jpege_engine_sw_fetch_dct_block_luma
  jpege_engine_sw_fetch_dct_block_chroma

@par INITIALIZATION AND SEQUENCING REQUIREMENTS
  (none)

Copyright (C) 2009 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.

*//*====================================================================== */

/*========================================================================
                             Edit History

$Header$

when       who     what, where, why
--------   ---     -------------------------------------------------------
09/08/09   zhiminl Added jpege_engine_sw_fetch_dct_block_luma() and
                   jpege_engine_sw_fetch_dct_block_chroma().

========================================================================== */

/*------------------------------------------------------------------------
 *                          Include Files
 * ----------------------------------------------------------------------- */

#include "jpege_engine_sw.h"

/* -----------------------------------------------------------------------
 *                          Forward Declarations
 * ----------------------------------------------------------------------- */

extern void jpege_engine_sw_fdct_block(const uint8_t *, DCTELEM *);

/*------------------------------------------------------------------------
 *                          Externalized Function Definitions
 * ----------------------------------------------------------------------- */

#ifndef ARM_ARCH_7A
void jpege_engine_sw_fetch_dct_block_luma
(
    const uint8_t *p_luma_block_origin,
    int16_t       *p_dct_output,
    uint32_t       luma_width
)
{
    const uint8_t *p_luma_block = p_luma_block_origin;
    uint8_t        fetched_block[BLOCK_SIZE];
    uint8_t       *p_fetched_block = fetched_block;
    uint32_t       i;

    // Vertically unpadded part
    for (i = 0; i < BLOCK_HEIGHT; i++)
    {
        // Horizontally unpadded part
        STD_MEMMOVE(p_fetched_block, p_luma_block, (BLOCK_WIDTH * sizeof(uint8_t)));
        p_fetched_block += BLOCK_WIDTH;
        p_luma_block    += luma_width;
    }

    // Do DCT
    jpege_engine_sw_fdct_block(fetched_block, p_dct_output);
}
#endif /* !ARM_ARCH_7A */

#ifndef ARM_ARCH_7A
void jpege_engine_sw_fetch_dct_block_chroma
(
    const uint8_t  *p_chroma_block_origin,
    int16_t        *p_dct_output,
    uint32_t        chroma_width,
    input_format_t  input_format
)
{
    const uint8_t *p_chroma_block = p_chroma_block_origin;
    uint8_t        fetched_block[BLOCK_SIZE * NUM_CHROMA_BLOCKS]; // chroma is interleaved
    uint8_t        deinterleaved_block[BLOCK_SIZE * NUM_CHROMA_BLOCKS];
    uint8_t       *p_fetched_block = fetched_block;
    uint8_t       *p_deinterleaved_block1 = deinterleaved_block;
    uint8_t       *p_deinterleaved_block2 = p_deinterleaved_block1 + BLOCK_SIZE;
    uint8_t       *p_cb_block, *p_cr_block;
    uint32_t       i, j;

    // Vertically unpadded part
    for (i = 0; i < BLOCK_HEIGHT; i++)
    {
        // Horizontally unpadded part
        STD_MEMMOVE(p_fetched_block, p_chroma_block, (BLOCK_WIDTH * sizeof(uint8_t) * 2));
        p_fetched_block += (BLOCK_WIDTH * 2);
        p_chroma_block  += (chroma_width * 2);
    }

    // Deinterleave chroma block
    for (i = 0; i < BLOCK_HEIGHT; i++)
    {
        for (j = 0; j < BLOCK_WIDTH; j++)
        {
            *p_deinterleaved_block1++ = fetched_block[i*BLOCK_WIDTH*2+j*2];
            *p_deinterleaved_block2++ = fetched_block[i*BLOCK_WIDTH*2+j*2+1];
        }
    }

    // Dual Format Selection
    if (input_format == YCrCb)
    {
        p_cb_block = deinterleaved_block + BLOCK_SIZE;
        p_cr_block = deinterleaved_block;
    }
    else
    {
        p_cb_block = deinterleaved_block;
        p_cr_block = deinterleaved_block + BLOCK_SIZE;
    }

    // Do DCT - always in the order of CbCr
    jpege_engine_sw_fdct_block(p_cb_block, p_dct_output);
    jpege_engine_sw_fdct_block(p_cr_block, (p_dct_output + BLOCK_SIZE));
}
#endif /* !ARM_ARCH_7A */

