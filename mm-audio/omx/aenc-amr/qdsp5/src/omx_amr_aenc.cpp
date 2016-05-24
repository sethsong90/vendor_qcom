/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file omx_aenc_amr.c
  This module contains the implementation of the OpenMAX core & component.

Copyright (c) 2006-2008 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <fcntl.h>
#include <omx_amr_aenc.h>
#include "amrsup.h"
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/msm_audio_amrnb.h>
#include <stdlib.h>

#define max(x,y) (x >= y?x:y)

typedef struct frameInformation
{
        OMX_U8 mode;
        OMX_U8 frameLen;
}frameInfo;


frameInfo g_frmInfo[16] = {{0, 12}, {1, 13}, {2, 15}, {3, 17},
                           {4, 19}, {5, 20}, {6, 26}, {7, 31},
                           {8, 5}, {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{15, 0}};


/* ======================== 4.75 kbps mode ========================== */
const OMX_U16 amrsup_bit_order_475_a[AMR_CLASS_A_BITS_475] = {
   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
   10,  11,  12,  13,  14,  15,  23,  24,  25,  26,
   27,  28,  48,  49,  61,  62,  82,  83,  47,  46,
   45,  44,  81,  80,  79,  78,  17,  18,  20,  22,
   77,  76

};


const OMX_U16 amrsup_bit_order_475_b[AMR_CLASS_B_BITS_475] = {

  /*------*/  75,  74,  29,  30,  43,  42,  41, 40,

   38,  39,  16,  19,  21,  50,  51,  59,  60,  63,

   64,  72,  73,  84,  85,  93,  94,  32,  33,  35,
   36,  53,  54,  56,  57,  66,  67,  69,  70,  87,
   88,  90,  91,  34,  55,  68,  89,  37,  58,  71,
   92,  31,  52,  65,  86
};

/* ======================== 5.15 kbps mode ========================== */
const OMX_U16 amrsup_bit_order_515_a[AMR_CLASS_A_BITS_515] = {
    7,   6,   5,   4,   3,   2,   1,   0,  15,  14,
   13,  12,  11,  10,   9,   8,  23,  24,  25,  26,
   27,  46,  65,  84,  45,  44,  43,  64,  63,  62,
   83,  82,  81, 102, 101, 100,  42,  61,  80,  99,
   28,  47,  66,  85,  18,  41,  60,  79,  98  /**/
};


const OMX_U16 amrsup_bit_order_515_b[AMR_CLASS_B_BITS_515] = {
  /*----------------------------------------*/  29,
   48,  67,  17,  20,  22,  40,  59,  78,  97,  21,
   30,  49,  68,  86,  19,  16,  87,  39,  38,  58,
   57,  77,  35,  54,  73,  92,  76,  96,  95,  36,
   55,  74,  93,  32,  51,  33,  52,  70,  71,  89,
   90,  31,  50,  69,  88,  37,  56,  75,  94,  34,
   53,  72,  91
};


/* ======================== 5.90 kbps mode ========================== */
const OMX_U16 amrsup_bit_order_590_a[AMR_CLASS_A_BITS_590] = {
    0,   1,   4,   5,   3,   6,   7,   2,  13,  15,
    8,   9,  11,  12,  14,  10,  16,  28,  74,  29,
   75,  27,  73,  26,  72,  30,  76,  51,  97,  50,
   71,  96, 117,  31,  77,  52,  98,  49,  70,  95,
  116,  53,  99,  32,  78,  33,  79,  48,  69,  94,
  115,  47,  68,  93, 114
};


const OMX_U16 amrsup_bit_order_590_b[AMR_CLASS_B_BITS_590] = {
  /*--------------------*/  46,  67,  92, 113,  19,
   21,  23,  22,  18,  17,  20,  24, 111,  43,  89,
  110,  64,  65,  44,  90,  25,  45,  66,  91, 112,
   54, 100,  40,  61,  86, 107,  39,  60,  85, 106,
   36,  57,  82, 103,  35,  56,  81, 102,  34,  55,
   80, 101,  42,  63,  88, 109,  41,  62,  87, 108,
   38,  59,  84, 105,  37,  58,  83, 104
};

/* ======================== 6.70 kbps mode ========================== */
const OMX_U16 amrsup_bit_order_670_a[AMR_CLASS_A_BITS_670] = {
    0,   1,   4,   3,   5,   6,  13,   7,   2,   8,
    9,  11,  15,  12,  14,  10,  28,  82,  29,  83,
   27,  81,  26,  80,  30,  84,  16,  55, 109,  56,
  110,  31,  85,  57, 111,  48,  73, 102, 127,  32,
   86,  51,  76, 105, 130,  52,  77, 106, 131,  58,
  112,  33,  87,  19,  23,  53,  78, 107
};


const OMX_U16 amrsup_bit_order_670_b[AMR_CLASS_B_BITS_670] = {
  /*-----------------------------------*/ 132,  21,
   22,  18,  17,  20,  24,  25,  50,  75, 104, 129,
   47,  72, 101, 126,  54,  79, 108, 133,  46,  71,
  100, 125, 128, 103,  74,  49,  45,  70,  99, 124,
   42,  67,  96, 121,  39,  64,  93, 118,  38,  63,
   92, 117,  35,  60,  89, 114,  34,  59,  88, 113,
   44,  69,  98, 123,  43,  68,  97, 122,  41,  66,
   95, 120,  40,  65,  94, 119,  37,  62,  91, 116,
   36,  61,  90, 115
};

/* ======================== 7.40 kbps mode ========================== */
const OMX_U16 amrsup_bit_order_740_a[AMR_CLASS_A_BITS_740] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
   10,  11,  12,  13,  14,  15,  16,  26,  87,  27,
   88,  28,  89,  29,  90,  30,  91,  51,  80, 112,
  141,  52,  81, 113, 142,  54,  83, 115, 144,  55,
   84, 116, 145,  58, 119,  59, 120,  21,  22,  23,
   17,  18,  19,  31,  60,  92, 121,  56,  85, 117,
  146
};


const OMX_U16 amrsup_bit_order_740_b[AMR_CLASS_B_BITS_740] = {
  /*-*/ 20,  24,  25,  50,  79, 111, 140,  57,  86,
  118, 147,  49,  78, 110, 139,  48,  77,  53,  82,
  114, 143, 109, 138,  47,  76, 108, 137,  32,  33,
   61,  62,  93,  94, 122, 123,  41,  42,  43,  44,
   45,  46,  70,  71,  72,  73,  74,  75, 102, 103,
  104, 105, 106, 107, 131, 132, 133, 134, 135, 136,
   34,  63,  95, 124,  35,  64,  96, 125,  36,  65,
   97, 126,  37,  66,  98, 127,  38,  67,  99, 128,
   39,  68, 100, 129,  40,  69, 101, 130
};


/* ======================== 7.95 kbps mode ========================== */
const OMX_U16 amrsup_bit_order_795_a[AMR_CLASS_A_BITS_795] = {
    8,   7,   6,   5,   4,   3,   2,  14,  16,   9,
   10,  12,  13,  15,  11,  17,  20,  22,  24,  23,
   19,  18,  21,  56,  88, 122, 154,  57,  89, 123,
  155,  58,  90, 124, 156,  52,  84, 118, 150,  53,
   85, 119, 151,  27,  93,  28,  94,  29,  95,  30,
   96,  31,  97,  61, 127,  62, 128,  63, 129,  59,
   91, 125, 157,  32,  98,  64, 130,   1,   0,  25,
   26,  33,  99,  34, 100
};

const OMX_U16 amrsup_bit_order_795_b[AMR_CLASS_B_BITS_795] = {
  /*--------------------*/  65, 131,  66, 132,  54,
   86, 120, 152,  60,  92, 126, 158,  55,  87, 121,
  153, 117, 116, 115,  46,  78, 112, 144,  43,  75,
  109, 141,  40,  72, 106, 138,  36,  68, 102, 134,
  114, 149, 148, 147, 146,  83,  82,  81,  80,  51,
   50,  49,  48,  47,  45,  44,  42,  39,  35,  79,
   77,  76,  74,  71,  67, 113, 111, 110, 108, 105,
  101, 145, 143, 142, 140, 137, 133,  41,  73, 107,
  139,  37,  69, 103, 135,  38,  70, 104, 136
};


/* ======================== 10.2 kbps mode ========================== */
const OMX_U16 amrsup_bit_order_102_a[AMR_CLASS_A_BITS_102] = {
    7,   6,   5,   4,   3,   2,   1,   0,  16,  15,
   14,  13,  12,  11,  10,   9,   8,  26,  27,  28,
   29,  30,  31, 115, 116, 117, 118, 119, 120,  72,
   73, 161, 162,  65,  68,  69, 108, 111, 112, 154,
  157, 158, 197, 200, 201,  32,  33, 121, 122,  74,
   75, 163, 164,  66, 109, 155, 198,  19,  23,  21,
   22,  18,  17,  20,  24
};

const OMX_U16 amrsup_bit_order_102_b[AMR_CLASS_B_BITS_102] = {
  /*--------------------*/  25,  37,  36,  35,  34,
   80,  79,  78,  77, 126, 125, 124, 123, 169, 168,
  167, 166,  70,  67,  71, 113, 110, 114, 159, 156,
  160, 202, 199, 203,  76, 165,  81,  82,  92,  91,
   93,  83,  95,  85,  84,  94, 101, 102,  96, 104,
   86, 103,  87,  97, 127, 128, 138, 137, 139, 129,
  141, 131, 130, 140, 147, 148, 142, 150, 132, 149,
  133, 143, 170, 171, 181, 180, 182, 172, 184, 174,
  173, 183, 190, 191, 185, 193, 175, 192, 176, 186,
   38,  39,  49,  48,  50,  40,  52,  42,  41,  51,
   58,  59,  53,  61
};


const OMX_U16 amrsup_bit_order_102_c[AMR_CLASS_C_BITS_102] = {
  /*---------------*/  43,  60,  44,  54, 194, 179,
  189, 196, 177, 195, 178, 187, 188, 151, 136, 146,
  153, 134, 152, 135, 144, 145, 105,  90, 100, 107,
   88, 106,  89,  98,  99,  62,  47,  57,  64,  45,
   63,  46,  55,  56
};


/* ======================== 12.2 kbps mode ========================== */
const OMX_U16 amrsup_bit_order_122_a[AMR_CLASS_A_BITS_122] = {
     0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
    10,  11,  12,  13,  14,  23,  15,  16,  17,  18,
    19,  20,  21,  22,  24,  25,  26,  27,  28,  38,
   141,  39, 142,  40, 143,  41, 144,  42, 145,  43,
   146,  44, 147,  45, 148,  46, 149,  47,  97, 150,
   200,  48,  98, 151, 201,  49,  99, 152, 202,  86,
   136, 189, 239,  87, 137, 190, 240,  88, 138, 191,
   241,  91, 194,  92, 195,  93, 196,  94, 197,  95,
   198
};

const OMX_U16 amrsup_bit_order_122_b[AMR_CLASS_B_BITS_122] = {
   /**/  29,  30,  31,  32,  33,  34,  35,  50, 100,
   153, 203,  89, 139, 192, 242,  51, 101, 154, 204,
    55, 105, 158, 208,  90, 140, 193, 243,  59, 109,
   162, 212,  63, 113, 166, 216,  67, 117, 170, 220,
    36,  37,  54,  53,  52,  58,  57,  56,  62,  61,
    60,  66,  65,  64,  70,  69,  68, 104, 103, 102,
   108, 107, 106, 112, 111, 110, 116, 115, 114, 120,
   119, 118, 157, 156, 155, 161, 160, 159, 165, 164,
   163, 169, 168, 167, 173, 172, 171, 207, 206, 205,
   211, 210, 209, 215, 214, 213, 219, 218, 217, 223,
   222, 221,  73,  72
};


const OMX_U16 amrsup_bit_order_122_c[AMR_CLASS_C_BITS_122] = {
   /* ------------- */  71,  76,  75,  74,  79,  78,
    77,  82,  81,  80,  85,  84,  83, 123, 122, 121,
   126, 125, 124, 129, 128, 127, 132, 131, 130, 135,
   134, 133, 176, 175, 174, 179, 178, 177, 182, 181,
   180, 185, 184, 183, 188, 187, 186, 226, 225, 224,
   229, 228, 227, 232, 231, 230, 235, 234, 233, 238,
   237, 236,  96, 199
};

//for IF2 deframing
/* ======================== SID mode ========================== */
const OMX_U16 amrsup_bit_order_sid[AMR_CLASS_A_BITS_SID] = {
   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
   20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
   30,  31,  32,  33,  34,  35,  36,  37,  38
};


const amrsup_frame_order_type amrsup_475_framing = {
  AMR_CLASS_A_BITS_475,
  (OMX_U16 *) amrsup_bit_order_475_a,
  AMR_CLASS_B_BITS_475,
  (OMX_U16 *) amrsup_bit_order_475_b,
  AMR_CLASS_C_BITS_475,
  NULL
};
const amrsup_frame_order_type amrsup_515_framing = {
  AMR_CLASS_A_BITS_515,
  (OMX_U16 *) amrsup_bit_order_515_a,
  AMR_CLASS_B_BITS_515,
  (OMX_U16 *) amrsup_bit_order_515_b,
  AMR_CLASS_C_BITS_515,
  NULL
};

const amrsup_frame_order_type amrsup_590_framing = {
  AMR_CLASS_A_BITS_590,
  (OMX_U16 *) amrsup_bit_order_590_a,
  AMR_CLASS_B_BITS_590,
  (OMX_U16 *) amrsup_bit_order_590_b,
  AMR_CLASS_C_BITS_590,
  NULL
};

const amrsup_frame_order_type amrsup_670_framing = {
  AMR_CLASS_A_BITS_670,
  (OMX_U16 *) amrsup_bit_order_670_a,
  AMR_CLASS_B_BITS_670,
  (OMX_U16 *) amrsup_bit_order_670_b,
  AMR_CLASS_C_BITS_670,
  NULL
};

const amrsup_frame_order_type amrsup_740_framing = {
  AMR_CLASS_A_BITS_740,
  (OMX_U16 *) amrsup_bit_order_740_a,
  AMR_CLASS_B_BITS_740,
  (OMX_U16 *) amrsup_bit_order_740_b,
  AMR_CLASS_C_BITS_740,
  NULL
};

const amrsup_frame_order_type amrsup_795_framing = {
  AMR_CLASS_A_BITS_795,
  (OMX_U16 *) amrsup_bit_order_795_a,
  AMR_CLASS_B_BITS_795,
  (OMX_U16 *) amrsup_bit_order_795_b,
  AMR_CLASS_C_BITS_795,
  NULL
};

const amrsup_frame_order_type amrsup_102_framing = {
  AMR_CLASS_A_BITS_102,
  (OMX_U16 *) amrsup_bit_order_102_a,
  AMR_CLASS_B_BITS_102,
  (OMX_U16 *) amrsup_bit_order_102_b,
  AMR_CLASS_C_BITS_102,
  (OMX_U16 *) amrsup_bit_order_102_c
};

const amrsup_frame_order_type amrsup_122_framing = {
  AMR_CLASS_A_BITS_122,
  (OMX_U16 *) amrsup_bit_order_122_a,
  AMR_CLASS_B_BITS_122,
  (OMX_U16 *) amrsup_bit_order_122_b,
  AMR_CLASS_C_BITS_122,
  (OMX_U16 *) amrsup_bit_order_122_c
};

/* Bit ordering tables indexed by amrsup_mode_type */
const amrsup_frame_order_type *amrsup_framing_tables[] = {
  &amrsup_475_framing,
  &amrsup_515_framing,
  &amrsup_590_framing,
  &amrsup_670_framing,
  &amrsup_740_framing,
  &amrsup_795_framing,
  &amrsup_102_framing,
  &amrsup_122_framing,
};



/*===========================================================================

FUNCTION amrsup_frame_len_bits

DESCRIPTION
  This function will determine number of bits of AMR vocoder frame length
based on the frame type and frame rate.

DEPENDENCIES
  None.

RETURN VALUE
  number of bits of AMR frame

SIDE EFFECTS
  None.

===========================================================================*/
int amrsup_frame_len_bits(
  amrsup_frame_type frame_type,
  amrsup_mode_type amr_mode
)
{
  int frame_len=0;


  switch (frame_type)
  {
    case AMRSUP_SPEECH_GOOD :
    case AMRSUP_SPEECH_DEGRADED :
    case AMRSUP_ONSET :
    case AMRSUP_SPEECH_BAD :
      if (amr_mode >= AMRSUP_MODE_MAX)
      {
        frame_len = 0;
      }
      else
      {
        frame_len = amrsup_framing_tables[amr_mode]->len_a
                    + amrsup_framing_tables[amr_mode]->len_b
                    + amrsup_framing_tables[amr_mode]->len_c;
      }
      break;

    case AMRSUP_SID_FIRST :
    case AMRSUP_SID_UPDATE :
    case AMRSUP_SID_BAD :
      frame_len = AMR_CLASS_A_BITS_SID;
      break;

    case AMRSUP_NO_DATA :
    case AMRSUP_SPEECH_LOST :
    default :
      frame_len = 0;
  }

  return frame_len;
}



/*===========================================================================

FUNCTION amrsup_frame_len

DESCRIPTION
  This function will determine number of bytes of AMR vocoder frame length
based on the frame type and frame rate.

DEPENDENCIES
  None.

RETURN VALUE
  number of bytes of AMR frame

SIDE EFFECTS
  None.

===========================================================================*/
int amrsup_frame_len(
  amrsup_frame_type frame_type,
  amrsup_mode_type amr_mode
)
{
  int frame_len = amrsup_frame_len_bits(frame_type, amr_mode);

  frame_len = (frame_len + 7) / 8;
  return frame_len;
}

/*===========================================================================

FUNCTION amrsup_tx_order

DESCRIPTION
  Use a bit ordering table to order bits from their original sequence.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void amrsup_tx_order(
  OMX_U8       *dst_frame,
  int         *dst_bit_index,
  OMX_U8       *src,
  int         num_bits,
  const OMX_U16 *order
)
{
  OMX_U32 dst_mask = 0x00000080 >> ((*dst_bit_index) & 0x7);
  OMX_U8  *dst = &dst_frame[((unsigned int) *dst_bit_index) >> 3];
  OMX_U32 src_bit, src_mask;

  /* Prepare to process all bits
  */
  *dst_bit_index += num_bits;
  num_bits++;

  while(--num_bits) {
    /* Get the location of the bit in the input buffer */
    src_bit  = (OMX_U32) *order++;
    src_mask = 0x00000080 >> (src_bit & 0x7);

    /* Set the value of the output bit equal to the input bit */
    if (src[src_bit >> 3] & src_mask) {
      *dst |= (OMX_U8) dst_mask;
    }

    /* Set the destination bit mask and increment pointer if necessary */
    dst_mask >>= 1;
    if (dst_mask == 0) {
      dst_mask = 0x00000080;
      dst++;
    }
  }
} /* amrsup_tx_order */

#ifdef FORMAT_IF2

/*===========================================================================

FUNCTION amrsup_if2_tx_order

DESCRIPTION
  Use a bit ordering table to order bits from their original sequence.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void amrsup_if2_tx_order(
  OMX_U8       *dst_frame,
  int         *dst_bit_index,
  OMX_U8       *src,
  int         num_bits,
  const  OMX_U16  *order
)
{
  OMX_U32 dst_mask = 0x00000001 << ((*dst_bit_index) & 0x7);
  OMX_U8 *dst = &dst_frame[((unsigned int) *dst_bit_index) >> 3];
  OMX_U32 src_bit, src_mask;

  /* Prepare to process all bits
  */
  *dst_bit_index += num_bits;
  num_bits++;

  while(--num_bits) {
    /* Get the location of the bit in the input buffer */
    src_bit  = (OMX_U32) *order++;
    src_mask = 0x00000080 >> (src_bit & 0x7);

    /* Set the value of the output bit equal to the input bit */
    if (src[src_bit >> 3] & src_mask) {
      *dst |= (OMX_U8) dst_mask;
    }

    /* Set the destination bit mask and increment pointer if necessary */
    dst_mask <<= 1;
    if (dst_mask == 0x00000100) {
      dst_mask = 0x00000001;
      dst++;
    }
  }
} /* amrsup_if2_tx_order */

#endif

/*===========================================================================*/
int amrsup_if1_framing(
  OMX_U8                     *vocoder_packet,
  amrsup_frame_type          frame_type,
  amrsup_mode_type           amr_mode,
  OMX_U8                     *if1_frame,
  amrsup_if1_frame_info_type *if1_frame_info
)
{
  amrsup_frame_order_type *ordering_table;
  int frame_len = 0;
  int i;

  if(amr_mode >= AMRSUP_MODE_MAX)
  {
    DEBUG_PRINT("Invalid AMR_Mode : %d",amr_mode);
    return 0;
  }

  /* Initialize IF1 frame data and info */
  if1_frame_info->fqi = TRUE;

  if1_frame_info->amr_type = AMRSUP_CODEC_AMR_NB;

  memset(if1_frame, 0,
           amrsup_frame_len(AMRSUP_SPEECH_GOOD, AMRSUP_MODE_1220));


  switch (frame_type)
  {
    case AMRSUP_SID_BAD:
      if1_frame_info->fqi = FALSE;
      /* fall thru */

    case AMRSUP_SID_FIRST:
    case AMRSUP_SID_UPDATE:
      /* Set frame type index */
      if1_frame_info->frame_type_index
      = AMRSUP_FRAME_TYPE_INDEX_AMR_SID;


      /* ===== Encoding SID frame ===== */
      /* copy the sid frame to class_a data */
      for (i=0; i<5; i++)
      {
        if1_frame[i] = vocoder_packet[i];
      }

      /* Set the SID type : SID_FIRST: Bit 35 = 0, SID_UPDATE : Bit 35 = 1 */
      if (frame_type == AMRSUP_SID_FIRST)
      {
        if1_frame[4] &= ~0x10;
      }

      if (frame_type == AMRSUP_SID_UPDATE)
      {
        if1_frame[4] |= 0x10;
      }
      else
      {
      /* Set the mode (Bit 36 - 38 = amr_mode with bits swapped)
      */
      if1_frame[4] |= (((OMX_U8)amr_mode << 3) & 0x08)
        | (((OMX_U8)amr_mode << 1) & 0x04) | (((OMX_U8)amr_mode >> 1) & 0x02);

      frame_len = AMR_CLASS_A_BITS_SID;
      }

      break;


    case AMRSUP_SPEECH_BAD:
      if1_frame_info->fqi = FALSE;
      /* fall thru */

    case AMRSUP_SPEECH_GOOD:
      /* Set frame type index */

        if1_frame_info->frame_type_index
        = (amrsup_frame_type_index_type)(amr_mode);

      /* ===== Encoding Speech frame ===== */
      /* Clear num bits in frame */
      frame_len = 0;

      /* Select ordering table */
      ordering_table =
      (amrsup_frame_order_type*)amrsup_framing_tables[(OMX_U8)amr_mode];

      amrsup_tx_order(
        if1_frame,
        &frame_len,
        vocoder_packet,
        ordering_table->len_a,
        ordering_table->class_a
      );

      amrsup_tx_order(
        if1_frame,
        &frame_len,
        vocoder_packet,
        ordering_table->len_b,
        ordering_table->class_b
      );

      amrsup_tx_order(
        if1_frame,
        &frame_len,
        vocoder_packet,
        ordering_table->len_c,
        ordering_table->class_c
      );


      /* frame_len already updated with correct number of bits */
      break;



    default:
      DEBUG_PRINT("Unsupported frame type %d", frame_type);
      /* fall thru */

    case AMRSUP_NO_DATA:
      /* Set frame type index */
      if1_frame_info->frame_type_index = AMRSUP_FRAME_TYPE_INDEX_NO_DATA;

      frame_len = 0;

      break;
  }  /* end switch */


  /* convert bit length to byte length */
  frame_len = (frame_len + 7) / 8;

  return frame_len;
}

#ifdef IF2_FORMAT

/*===========================================================================

FUNCTION amrsup_if2_framing

DESCRIPTION
  Performs the transmit side framing function.  Generates AMR IF2 ordered data
  from the vocoder packet and frame type.

DEPENDENCIES
  None.

RETURN VALUE
  number of bytes of encoded frame.

SIDE EFFECTS
  None.

===========================================================================*/
int amrsup_if2_framing(
  OMX_U8 *vocoder_packet,
  amrsup_frame_type frame_type,
  amrsup_mode_type amr_mode,
  OMX_U8 *if2_frame
)
{
  amrsup_frame_order_type *ordering_table;
  int initial_bit_index;
  int frame_len=0;


  /* Clear destination
  */
  (void) memset(if2_frame, 0, AMRSUP_IF2_FRAME_BYTES);

  switch (frame_type) {
    case AMRSUP_SID_FIRST:
    case AMRSUP_SID_UPDATE:
      initial_bit_index = 4;
      if2_frame[0] = AMRSUP_FRAME_TYPE_INDEX_AMR_SID;

      /* Copy the sid frame to class_a data
      */
      amrsup_if2_tx_order(
        if2_frame,
        &initial_bit_index,
        vocoder_packet,
        AMR_CLASS_A_BITS_SID,
        amrsup_bit_order_sid
      );

      if(frame_type == AMRSUP_SID_FIRST) {
        /* Set the SID FIRST type (Bit 39 = 0)
        */
        if2_frame[4] &= ~0x80;
      } else {
        /* Set the SID UPDATE type (Bit 39 = 1)
        */
        if2_frame[4] |= 0x80;
      }

      /* Set the mode (Bit 40 - 42 = amr_mode)
      */
      if2_frame[5] = (OMX_U8)amr_mode & AMRSUP_FRAME_TYPE_INDEX_SPEECH_MASK;

      break;

    case AMRSUP_NO_DATA:
      initial_bit_index = 4;
      if2_frame[0] = AMRSUP_FRAME_TYPE_INDEX_NO_DATA;

      break;

    case AMRSUP_SPEECH_GOOD:
      ordering_table =
        (amrsup_frame_order_type *) amrsup_framing_tables[amr_mode];
      initial_bit_index = 4;
      if2_frame[0] = amr_mode & AMRSUP_FRAME_TYPE_INDEX_SPEECH_MASK;

      amrsup_if2_tx_order(
        if2_frame,
        &initial_bit_index,
        vocoder_packet,
        ordering_table->len_a,
        ordering_table->class_a
      );

      amrsup_if2_tx_order(
        if2_frame,
        &initial_bit_index,
        vocoder_packet,
        ordering_table->len_b,
        ordering_table->class_b
      );

      amrsup_if2_tx_order(
        if2_frame,
        &initial_bit_index,
        vocoder_packet,
        ordering_table->len_c,
        ordering_table->class_c
      );

      break;

    case AMRSUP_SPEECH_BAD:
    case AMRSUP_SID_BAD:
    default:
      /* Error - the vocoder never generates these frame types */
      initial_bit_index = 4;
      if2_frame[0] = AMRSUP_FRAME_TYPE_INDEX_NO_DATA;

      break;
  }  /* end switch */


  /* initial_bit_index has counted total number of encoded bits. */
  frame_len = (initial_bit_index + 7) / 8;
  return frame_len;
} /* amrsup_if2_framing */

#endif

using namespace std;

// omx_cmd_queue destructor
omx_amr_aenc::omx_cmd_queue::~omx_cmd_queue()
{
  // Nothing to do
}

// omx cmd queue constructor
omx_amr_aenc::omx_cmd_queue::omx_cmd_queue(): m_read(0),m_write(0),m_size(0)
{
  memset(m_q,      0,sizeof(omx_event)*OMX_CORE_CONTROL_CMDQ_SIZE);
}

// omx cmd queue insert
bool omx_amr_aenc::omx_cmd_queue::insert_entry(unsigned p1, unsigned p2, unsigned id)
{
  bool ret = true;
  if(m_size < OMX_CORE_CONTROL_CMDQ_SIZE)
  {
    m_q[m_write].id       = id;
    m_q[m_write].param1   = p1;
    m_q[m_write].param2   = p2;
    m_write++;
    m_size ++;
    if(m_write >= OMX_CORE_CONTROL_CMDQ_SIZE)
    {
      m_write = 0;
    }
  }
  else
  {
    ret = false;
    DEBUG_PRINT_ERROR("ERROR!!! Command Queue Full");
  }
  return ret;
}

// omx cmd queue delete
bool omx_amr_aenc::omx_cmd_queue::delete_entry(unsigned *p1, unsigned *p2, unsigned *id)
{
  bool ret = true;
  if (m_size > 0)
  {
    *id = m_q[m_read].id;
    *p1 = m_q[m_read].param1;
    *p2 = m_q[m_read].param2;
    // Move the read pointer ahead
    ++m_read;
    --m_size;
    if(m_read >= OMX_CORE_CONTROL_CMDQ_SIZE)
    {
      m_read = 0;

    }
  }
  else
  {
    ret = false;
    DEBUG_PRINT_ERROR("ERROR Delete!!! Command Queue Full");
  }
  return ret;
}

// factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
  return (new omx_amr_aenc);
}


/* ======================================================================
FUNCTION
  omx_amr_aenc::omx_amr_aenc

DESCRIPTION
  Constructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_amr_aenc::omx_amr_aenc(): m_state(OMX_StateInvalid),
                              m_app_data(NULL),
                              m_cmd_svr(NULL),
                              m_drv_fd(-1),
                              m_inp_buf_count(0),
                              m_flags(0),
                              output_buffer_size(OMX_AMR_OUTPUT_BUFFER_SIZE)
{
  memset(&m_cmp,       0,     sizeof(m_cmp));
  memset(&m_cb,        0,      sizeof(m_cb));

  pthread_mutexattr_init(&m_lock_attr);
  pthread_mutex_init(&m_lock, &m_lock_attr);

    pthread_mutexattr_init(&m_outputlock_attr);
    pthread_mutex_init(&m_outputlock, &m_outputlock_attr);

    pthread_mutexattr_init(&m_state_lock_attr);
    pthread_mutex_init(&m_state_lock, &m_state_lock_attr);

  return;
}


/* ======================================================================
FUNCTION
  omx_amr_aenc::~omx_amr_aenc

DESCRIPTION
  Destructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_amr_aenc::~omx_amr_aenc()
{
  pthread_mutexattr_destroy(&m_lock_attr);
  pthread_mutex_destroy(&m_lock);

    pthread_mutexattr_destroy(&m_state_lock_attr);
    pthread_mutex_destroy(&m_state_lock);
    pthread_mutexattr_destroy(&m_outputlock_attr);
    pthread_mutex_destroy(&m_outputlock);

  return;
}

/**
  @brief memory function for sending EmptyBufferDone event
   back to IL client

  @param bufHdr OMX buffer header to be passed back to IL client
  @return none
 */
void omx_amr_aenc::buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{

  if(m_cb.EmptyBufferDone)
  {
    PrintFrameHdr(bufHdr);

    m_cb.EmptyBufferDone(&m_cmp, m_app_data, bufHdr);
  }

  return;
}

void omx_amr_aenc::frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{

  if(m_cb.FillBufferDone)
  {
    PrintFrameHdr(bufHdr);
    if (fcount == 0) {
        bufHdr->nTimeStamp = nTimestamp;
        DEBUG_PRINT(" frame_done_cb : time stamp of first output buffer = %lld \n",bufHdr->nTimeStamp);
    }
    else
    {
        nTimestamp += frameDuration;
        bufHdr->nTimeStamp = nTimestamp;
        DEBUG_PRINT(" frame_done_cb : time stamp of output buffer = %llu \n",bufHdr->nTimeStamp);
    }
    fcount++;

    m_cb.FillBufferDone(&m_cmp, m_app_data, bufHdr);
  }

  return;
}

/** ======================================================================
 @brief static member function for handling all commands from IL client

  IL client commands are processed and callbacks are generated
  through this routine. Audio Command Server provides the thread context
  for this routine.

void omx_amr_aenc::process_output_cb(void *client_d  @param id command identifier
  @return none
 */

void omx_amr_aenc::process_output_cb(void *client_data, unsigned char id)
{
  unsigned      p1; // Parameter - 1
  unsigned      p2; // Parameter - 2
  unsigned      ident;
  unsigned         qsize=0; // qsize
  omx_amr_aenc  *pThis              = (omx_amr_aenc *) client_data;

    OMX_STATETYPE state;
    DEBUG_PRINT("Inside OUT thread...\n");
    pthread_mutex_lock(&pThis->m_state_lock);
    state = pThis->m_state;
    pthread_mutex_unlock(&pThis->m_state_lock);
    if ( state == OMX_StateLoaded )
  {
        DEBUG_PRINT(" OUT: IN LOADED STATE RETURN\n");
        return;
    }
    pthread_mutex_lock(&pThis->m_outputlock);

    qsize = pThis->m_output_q.m_size;
    DEBUG_PRINT("OUT-->qsize=%d\n",qsize);

    if(qsize)
    {
      pThis->m_output_q.delete_entry(&p1,&p2,&ident);
    }
    pthread_mutex_unlock(&pThis->m_outputlock);

    if(qsize > 0)
    {
      id = ident;
      DEBUG_PRINT("OUT->state[%d]id[%d]\n",pThis->m_state,ident);

      if(id == OMX_COMPONENT_GENERATE_FRAME_DONE)
      {
          DEBUG_PRINT(" processing OMX_COMPONENT_GENERATE_FRAME_DONE \n");
          pThis->frame_done_cb((OMX_BUFFERHEADERTYPE *)p2);
      }
      else if(id == OMX_COMPONENT_GENERATE_FTB)
      {
          DEBUG_PRINT(" processing OMX_COMPONENT_GENERATE_FTB \n");
          pThis->fill_this_buffer_proxy((OMX_HANDLETYPE)p1,
                                        (OMX_BUFFERHEADERTYPE *)p2);
      }
      else
      {
        DEBUG_PRINT_ERROR("OUT-->Invalid id=%d\n",id);
      }
    }
    else
    {
        DEBUG_PRINT("OUT-->Empty Q...%d\n",pThis->m_output_q.m_size);
    }
  return;

}
void omx_amr_aenc::process_event_cb(void *client_data, unsigned char id)
{
  unsigned      p1; // Parameter - 1
  unsigned      p2; // Parameter - 2
  unsigned      ident;
  unsigned qsize=0; // qsize
  omx_amr_aenc  *pThis              = (omx_amr_aenc *) client_data;

  DEBUG_PRINT("OMXCntrlProessMsgCb[%x,%d] Enter:" , (unsigned) client_data,(unsigned)id);
  if(!pThis)
  {
    DEBUG_PRINT_ERROR("ERROR : ProcessMsgCb: Context is incorrect; bailing out\n");
    return;
  }
   do{

  // Protect the shared queue data structure
    pthread_mutex_lock(&pThis->m_lock);

    qsize = pThis->m_cmd_q.m_size;

    if(qsize)
    {
      pThis->m_cmd_q.delete_entry(&p1,&p2,&ident);
    } else {
      OMX_STATETYPE state;

      qsize = pThis->m_data_q.m_size;
      pThis->get_state(&pThis->m_cmp, &state);

      if ((qsize) && (state == OMX_StateExecuting))
      {
        pThis->m_data_q.delete_entry(&p1, &p2, &ident);
      } else
      {
        qsize = 0;
      }
    }

    if(qsize)
    {
      pThis->m_msg_cnt ++;
    }
    pthread_mutex_unlock(&pThis->m_lock);

    if(qsize > 0)
    {
      id = ident;
      DEBUG_PRINT("Process ->%d[%d]ebd %d %x\n",pThis->m_state,ident, pThis->m_etb_cnt,
            pThis->m_flags >> 3);
      if(id == OMX_COMPONENT_GENERATE_EVENT)
      {
        if (pThis->m_cb.EventHandler)
        {
          if (p1 == OMX_CommandStateSet)
          {
             pthread_mutex_lock(&pThis->m_state_lock);
             pThis->m_state = (OMX_STATETYPE) p2;
             pthread_mutex_unlock(&pThis->m_state_lock);

             DEBUG_PRINT("Process -> state set to %d \n", pThis->m_state);
          }

          if (pThis->m_state == OMX_StateInvalid) {
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                     OMX_EventError, OMX_ErrorInvalidState,
                                     0, NULL );
          } else {
            pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                     OMX_EventCmdComplete, p1, p2, NULL );
          }

        }
        else
        {
          DEBUG_PRINT_ERROR("Error: ProcessMsgCb ignored due to NULL callbacks\n");
        }
      }
      else if(id == OMX_COMPONENT_GENERATE_BUFFER_DONE)
      {
        pThis->buffer_done_cb((OMX_BUFFERHEADERTYPE *)p2);
      }
      else if(id == OMX_COMPONENT_GENERATE_ETB)
      {
        pThis->empty_this_buffer_proxy((OMX_HANDLETYPE)p1,(OMX_BUFFERHEADERTYPE *)p2);
      }
      else if(id == OMX_COMPONENT_GENERATE_COMMAND)
      {
        pThis->send_command_proxy(&pThis->m_cmp,(OMX_COMMANDTYPE)p1,(OMX_U32)p2,(OMX_PTR)NULL);
      }
      else
      {
        DEBUG_PRINT_ERROR("Error: ProcessMsgCb Ignored due to Invalid Identifier\n");
      }
      DEBUG_PRINT("OMXCntrlProessMsgCb[%x,%d] Exit: \n",
                  (unsigned)client_data,(unsigned)id);
    }
    else
    {
      DEBUG_PRINT("Error: ProcessMsgCb Ignored due to empty CmdQ\n");
    }
    pthread_mutex_lock(&pThis->m_lock);
    qsize = pThis->m_cmd_q.m_size;
    pthread_mutex_unlock(&pThis->m_lock);
  } while(qsize>0);
  return;
}



/**
 @brief member function for performing component initialization

 @param role C string mandating role of this component
 @return Error status
 */
OMX_ERRORTYPE omx_amr_aenc::component_init(OMX_STRING role)
{

  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  /* Ignore role */

  m_state                   = OMX_StateLoaded;
  /* DSP does not give information about the bitstream
     randomly assign the value right now. Query will result in
     incorrect param */
  memset(&m_aenc_param, 0, sizeof(m_aenc_param));
  m_aenc_param.nSize = sizeof(m_aenc_param);
  m_volume = 25; /* Close to unity gain */
  m_aenc_param.nChannels = 1;
  /* default calculation of frame duration */
  frameDuration = 0;
  fcount = 0;
  nTimestamp = 0;
  m_recPath=0;

  DEBUG_PRINT(" component init: role = %s\n",role);
  if ( !strcmp(role,"OMX.qcom.audio.encoder.amr") )
  {
	pcm_feedback = 1;
	DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
  } else if ( !strcmp(role,"OMX.qcom.audio.encoder.tunneled.amr") )
  {
	pcm_feedback = 0;
	DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
  } else
  {
	DEBUG_PRINT("\ncomponent_init: Component %s LOADED is invalid\n", role);
  }

  if(0 == pcm_feedback)
  {
        m_drv_fd = open("/dev/msm_amrnb_in",O_RDONLY);
  }
  else
  {
        m_drv_fd = open("/dev/msm_amrnb_in",O_RDWR);
  }

  if (m_drv_fd < 0)
  {
      DEBUG_PRINT_ERROR("OMXCORE-SM: device msm_amrnb_in open fail\n");
      eRet =  OMX_ErrorInsufficientResources;
  }
  if(ioctl(m_drv_fd, AUDIO_GET_SESSION_ID,&m_session_id) == -1)
  {
      DEBUG_PRINT("AUDIO_GET_SESSION_ID FAILED\n");
  }
  return eRet;
}

/**

 @brief member function to retrieve version of component



 @param hComp handle to this component instance
 @param componentName name of component
 @param componentVersion  pointer to memory space which stores the
       version number
 @param specVersion pointer to memory sapce which stores version of
        openMax specification
 @param componentUUID
 @return Error status
 */
OMX_ERRORTYPE  omx_amr_aenc::get_component_version(OMX_IN OMX_HANDLETYPE               hComp,
                                                  OMX_OUT OMX_STRING          componentName,
                                                  OMX_OUT OMX_VERSIONTYPE* componentVersion,
                                                  OMX_OUT OMX_VERSIONTYPE*      specVersion,
                                                  OMX_OUT OMX_UUIDTYPE*       componentUUID)
{
  (void)hComp;
  (void)componentName;
  (void)componentVersion;
  (void)specVersion;
  (void)componentUUID;
  /* TBD -- Return the proper version */
  return OMX_ErrorNone;
}
/**
  @brief member function handles command from IL client

  This function simply queue up commands from IL client.
  Commands will be processed in command server thread context later

  @param hComp handle to component instance
  @param cmd type of command
  @param param1 parameters associated with the command type
  @param cmdData
  @return Error status
*/
OMX_ERRORTYPE  omx_amr_aenc::send_command(OMX_IN OMX_HANDLETYPE hComp,
                                          OMX_IN OMX_COMMANDTYPE  cmd,
                                          OMX_IN OMX_U32       param1,
                                          OMX_IN OMX_PTR      cmdData)
{
  (void)hComp;
  (void)cmdData;
  if(!m_cmd_svr)
  {
    m_cmd_svr = aenc_svr_start(process_event_cb, this);
    if(!m_cmd_svr)
    {
      m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventError, cmd, param1, NULL );
      DEBUG_PRINT_ERROR("ERROR!!! comand server open failed\n");
      return OMX_ErrorHardware;
    }
  }

  if(!m_cmd_cln)
  {
    m_cmd_cln = aenc_svr_start(process_output_cb, this);
    if(!m_cmd_cln)
    {
      m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventError, cmd, param1, NULL );
      DEBUG_PRINT_ERROR("ERROR!!! comand Client open failed\n");
      return OMX_ErrorHardware;
    }
  }
  post_event((unsigned)cmd,(unsigned)param1,OMX_COMPONENT_GENERATE_COMMAND, true);
  return OMX_ErrorNone;
}

/**
 @brief member function performs actual processing of commands excluding
  empty buffer call

 @param hComp handle to component
 @param cmd command type
 @param param1 parameter associated with the command
 @param cmdData

 @return error status
*/
OMX_ERRORTYPE  omx_amr_aenc::send_command_proxy(OMX_IN OMX_HANDLETYPE hComp,
                                          OMX_IN OMX_COMMANDTYPE  cmd,
                                          OMX_IN OMX_U32       param1,
                                          OMX_IN OMX_PTR      cmdData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  //   Handle only IDLE and executing
  OMX_STATETYPE eState = (OMX_STATETYPE) param1;
  int bFlag = 1;

  (void)hComp;
  (void)cmdData;
  if(cmd == OMX_CommandStateSet)
  {
    /***************************/
    /* Current State is Loaded */
    /***************************/
    if(m_state == OMX_StateLoaded)
    {
      if(eState == OMX_StateIdle)
      {
          if (allocate_done()) {
            DEBUG_PRINT("OMXCORE-SM: Loaded->Idle\n");
          } else {
            DEBUG_PRINT("OMXCORE-SM: Loaded-->Idle-Pending\n");
            BITMASK_SET(&m_flags, OMX_COMPONENT_IDLE_PENDING);
            bFlag = 0;
          }
      }
      else
      {
        DEBUG_PRINT_ERROR("OMXCORE-SM: Loaded-->Invalid(%d Not Handled)\n",eState);
        eRet = OMX_ErrorBadParameter;
      }
    }

    /***************************/
    /* Current State is IDLE */
    /***************************/
    else if(m_state == OMX_StateIdle)
    {
      if(eState == OMX_StateLoaded)
      {
        if(release_done())
        {

          DEBUG_PRINT("OMXCORE-SM: Idle-->Loaded\n");
        }
        else
        {
          DEBUG_PRINT("OMXCORE-SM: Idle-->Loaded-Pending\n");
          BITMASK_SET(&m_flags, OMX_COMPONENT_LOADING_PENDING);
          // Skip the event notification
          bFlag = 0;
        }
      }
      else if(eState == OMX_StateExecuting)
      {
        struct msm_audio_config drv_config;
        DEBUG_PRINT("configure Driver for AMR Encoding sample rate = %d \n",8000);
        ioctl(m_drv_fd, AUDIO_GET_CONFIG, &drv_config);
        drv_config.sample_rate = 8000;
        drv_config.channel_count = 1;
        drv_config.type = 1;
        ioctl(m_drv_fd, AUDIO_SET_CONFIG, &drv_config);

#ifdef AUDIOV2
        struct msm_audio_amrnb_enc_config_v2 amrnb_cfg_v2;
        struct msm_audio_stream_config drv_stream_config;
        struct msm_voicerec_mode cfg;
        if(ioctl(m_drv_fd, AUDIO_GET_STREAM_CONFIG, &drv_stream_config) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_GET_STREAM_CONFIG failed, errno[%d]\n", errno);
        }
        drv_stream_config.buffer_size  = OMX_AMR_OUTPUT_BUFFER_SIZE;
        drv_stream_config.buffer_count = 2;//OMX_CORE_NUM_OUTPUT_BUFFERS;
        if(ioctl(m_drv_fd, AUDIO_SET_STREAM_CONFIG, &drv_stream_config) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_SET_STREAM_CONFIG failed, errno[%d]\n", errno);
        }

        if(ioctl(m_drv_fd,AUDIO_GET_AMRNB_ENC_CONFIG_V2 ,&amrnb_cfg_v2) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_GET_AMRNB_ENC_CONFIG_V2 failed, errno[%d]\n", errno);
        }

        amrnb_cfg_v2.band_mode = pbitrate;
        amrnb_cfg_v2.dtx_enable = pdtx;
        if(ioctl(m_drv_fd,AUDIO_SET_AMRNB_ENC_CONFIG_V2 ,&amrnb_cfg_v2) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_SET_AMRNB_ENC_CONFIG_V2 failed, errno[%d]\n", errno);
        }
        cfg.rec_mode = m_recPath;
        if(ioctl(m_drv_fd,AUDIO_SET_INCALL ,&cfg) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_SET_INCALL failed, errno[%d]\n", errno);
        }
#else
        struct msm_audio_amrnb_enc_config amrnb_cfg;
        amrnb_cfg.enc_mode = pbitrate;
        if(pdtx == 1){
            pdtx=65535;
        }
        amrnb_cfg.dtx_mode_enable = pdtx;
        ioctl(m_drv_fd, AUDIO_SET_AMRNB_ENC_CONFIG, &amrnb_cfg);

#endif
        frameDuration = FRAME_DURATION * MILLISECOND;
        if(ioctl(m_drv_fd, AUDIO_START, 0) == -1)
        {
            DEBUG_PRINT("ioctl AUDIO_START failed, errno[%d]\n", errno);
        }

        DEBUG_PRINT("OMXCORE-SM: Idle-->Executing\n");
      }
      else
      {
        DEBUG_PRINT_ERROR("OMXCORE-SM: Idle --> %d Not Handled\n",eState);
        eRet = OMX_ErrorBadParameter;
      }
    }

    /******************************/
    /* Current State is Executing */
    /******************************/
    else if(m_state == OMX_StateExecuting)
    {
       if(eState == OMX_StateIdle)
       {
         DEBUG_PRINT("OMXCORE-SM: Executing --> Idle \n");
         execute_omx_flush();
         ioctl(m_drv_fd, AUDIO_STOP, 0);
       }
       else if(eState == OMX_StatePause)
       {
         /* ioctl(m_drv_fd, AUDIO_PAUSE, 0); Not implemented at this point */
         DEBUG_PRINT("OMXCORE-SM: Executing --> Paused \n");
       }
       else
       {
         DEBUG_PRINT_ERROR("OMXCORE-SM: Executing --> %d Not Handled\n",eState);
         eRet = OMX_ErrorBadParameter;
       }
    }
    /***************************/
    /* Current State is Pause  */
    /***************************/
    else if(m_state == OMX_StatePause)
    {
      if(eState == OMX_StateExecuting)
      {
        /* ioctl(m_drv_fd, AUDIO_RESUME, 0); Not implemented at this point */
        DEBUG_PRINT("OMXCORE-SM: Paused --> Executing \n");
      }
      else if(eState == OMX_StateIdle)
      {
        DEBUG_PRINT("OMXCORE-SM: Paused --> Idle \n");
        execute_omx_flush();
        ioctl(m_drv_fd, AUDIO_STOP, 0);
      }
      else
      {
        DEBUG_PRINT("OMXCORE-SM: Paused --> %d Not Handled\n",eState);
        eRet = OMX_ErrorBadParameter;
      }
    }
    else
    {
      DEBUG_PRINT_ERROR("OMXCORE-SM: %d --> %d(Not Handled)\n",m_state,eState);
      eRet = OMX_ErrorBadParameter;
    }
  }
  else
  {
    DEBUG_PRINT_ERROR("Error: Invalid Command received other than StateSet (%d)\n",cmd);
    eRet = OMX_ErrorNotImplemented;
  }
  if(eRet == OMX_ErrorNone && bFlag)
  {
    post_event(cmd,eState,OMX_COMPONENT_GENERATE_EVENT, true);
  }
  return eRet;
}

/**
 @brief member function that flushes buffers that are pending to be written
  to driver

 @param none
 @return bool value indicating whether flushing is carried out successfully
*/
bool omx_amr_aenc::execute_omx_flush(void) /* Should flush be executed in order? */
{
  bool bRet = true;
  OMX_BUFFERHEADERTYPE *omx_buf;
  unsigned      p1; // Parameter - 1
  unsigned      p2; // Parameter - 2
  unsigned      ident;

  ioctl( m_drv_fd, AUDIO_FLUSH, 0);

  pthread_mutex_lock(&m_lock);

  DEBUG_PRINT("execute flush \n");
  while((m_data_q.delete_entry(&p1, &p2, &ident)) == true) {
    omx_buf = (OMX_BUFFERHEADERTYPE *) p2;

    DEBUG_PRINT("buf_addr=%p \n", omx_buf);
    post_event((unsigned) &m_cmp, (unsigned) omx_buf,
               OMX_COMPONENT_GENERATE_BUFFER_DONE, false);
  }

  pthread_mutex_unlock(&m_lock);

  return bRet;
}

/**
  @brief member function that posts command
  in the command queue

  @param p1 first paramter for the command
  @param p2 second parameter for the command
  @param id command ID
  @param lock self-locking mode
  @return bool indicating command being queued
 */
bool omx_amr_aenc::post_event(unsigned int p1,
                              unsigned int p2,
                              unsigned int id,
                              bool lock)
{
    bool bRet = false;

    (void)lock;
    pthread_mutex_lock(&m_lock);

    if (id == OMX_COMPONENT_GENERATE_ETB)
    {
    m_data_q.insert_entry(p1,p2,id);
    }
    else
    {
    m_cmd_q.insert_entry(p1,p2,id);
  }

  if(m_cmd_svr)
  {
    bRet = true;
    aenc_svr_post_msg(m_cmd_svr, id);
  }

  DEBUG_PRINT("Post -->%d[%d]ebd %d  %x \n",m_state,
          id, m_etb_cnt,
          m_flags >> 3);
    pthread_mutex_unlock(&m_lock);
  return bRet;
}


bool omx_amr_aenc::post_output(unsigned int p1,
                              unsigned int p2,
                              unsigned int id,
                              bool lock)
{
  bool bRet = false;

    (void)lock;
    pthread_mutex_lock(&m_outputlock);
  m_output_q.insert_entry(p1,p2,id);
  if(m_cmd_cln)
  {
    bRet = true;
        DEBUG_PRINT(" Post Output ID = %d cnt=%d\n",id,m_output_q.m_size);
    aenc_output_post_msg(m_cmd_cln, id);
  }


    DEBUG_PRINT("PostOut -->state[%d]id[%d]qsize[%d]\n",m_state,id,
                                                        m_output_q.m_size);
    pthread_mutex_unlock(&m_outputlock);
  return bRet;
}
/**
  @brief member function that return parameters to IL client

  @param hComp handle to component instance
  @param paramIndex Parameter type
  @param paramData pointer to memory space which would hold the
        paramter
  @return error status
*/
OMX_ERRORTYPE  omx_amr_aenc::get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                           OMX_IN OMX_INDEXTYPE paramIndex,
                                           OMX_INOUT OMX_PTR     paramData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  (void)hComp;
  switch(paramIndex)
  {
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
        portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

        DEBUG_PRINT("OMX_IndexParamPortDefinition portDefn->nPortIndex = %lu\n",portDefn->nPortIndex);

        portDefn->nVersion.nVersion = OMX_SPEC_VERSION;
        portDefn->nSize = sizeof(portDefn);
        portDefn->bEnabled   = OMX_TRUE;
        portDefn->bPopulated = OMX_TRUE;
        portDefn->eDomain    = OMX_PortDomainAudio;

     if (1 == portDefn->nPortIndex)
      {
        portDefn->eDir =  OMX_DirOutput;
        portDefn->nBufferCountActual = 1;/* What if the component does not restrict how many buffer to take */
        portDefn->nBufferCountMin    = OMX_NUM_DEFAULT_BUF;
        portDefn->nBufferSize        = output_buffer_size;
        portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
        portDefn->format.audio.eEncoding = OMX_AUDIO_CodingAMR;
        portDefn->format.audio.pNativeRender = 0;
      }
      else
      {
        portDefn->eDir =  OMX_DirMax;
        DEBUG_PRINT_ERROR("Bad Port idx %d\n", (int)portDefn->nPortIndex);
        eRet = OMX_ErrorBadPortIndex;
      }

      break;
    }

    case OMX_IndexParamAudioInit:
    {
      OMX_PORT_PARAM_TYPE *portParamType =
                              (OMX_PORT_PARAM_TYPE *) paramData;
      DEBUG_PRINT("OMX_IndexParamAudioInit\n");

      portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
      portParamType->nSize = sizeof(portParamType);
      portParamType->nPorts           = 2;
      portParamType->nStartPortNumber = 0;
      break;
    }

    case OMX_IndexParamAudioPortFormat:
    {
      OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
                                  (OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
      DEBUG_PRINT("OMX_IndexParamAudioPortFormat\n");
      portFormatType->nVersion.nVersion = OMX_SPEC_VERSION;
      portFormatType->nSize = sizeof(portFormatType);

      if (OMX_CORE_OUTPUT_PORT_INDEX == portFormatType->nPortIndex)
      {
        if (1 == portFormatType->nIndex) { /* What is the intention of nIndex */
          portFormatType->eEncoding = OMX_AUDIO_CodingAMR;
        }
      } else {
        eRet = OMX_ErrorBadPortIndex;
      }
      break;
    }

    case OMX_IndexParamAudioAmr:
    {
      OMX_AUDIO_PARAM_AMRTYPE *amrParam = (OMX_AUDIO_PARAM_AMRTYPE *) paramData;
      DEBUG_PRINT("OMX_IndexParamAudioAmr\n");
      *amrParam = m_aenc_param;

      break;
    }
    case QOMX_IndexParamAudioSessionId:
    {
       QOMX_AUDIO_STREAM_INFO_DATA *streaminfoparam =
                (QOMX_AUDIO_STREAM_INFO_DATA *) paramData;
      streaminfoparam->sessionId = m_session_id;
       break;
    }
    case QOMX_IndexParamAudioVoiceRecord:
    {
        QOMX_AUDIO_CONFIG_VOICERECORDTYPE *recordPath =
           (QOMX_AUDIO_CONFIG_VOICERECORDTYPE*)paramData;
        recordPath->eVoiceRecordMode = (QOMX_AUDIO_VOICERECORDMODETYPE)m_recPath;
    }

    default:
    {
      DEBUG_PRINT_ERROR("unknown param %08x\n", paramIndex);
      eRet = OMX_ErrorBadParameter;
    }

  }

  return eRet;

}

/**
 @brief member function that set paramter from IL client

 @param hComp handle to component instance
 @param paramIndex parameter type
 @param paramData pointer to memory space which holds the paramter
 @return error status
 */
OMX_ERRORTYPE  omx_amr_aenc::set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                           OMX_IN OMX_INDEXTYPE paramIndex,
                                           OMX_IN OMX_PTR        paramData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  (void)hComp;
  switch(paramIndex)
  {

    case OMX_IndexParamAudioAmr:
    {
      DEBUG_PRINT("OMX_IndexParamAudioAmr");
      memcpy(&m_aenc_param,paramData,sizeof(OMX_AUDIO_PARAM_AMRTYPE));
      pChannels = m_aenc_param.nChannels;
      pframeformat = m_aenc_param.eAMRFrameFormat;
      pbitrate     = m_aenc_param.nBitRate;
      pdtx         = m_aenc_param.eAMRDTXMode;
      break;
    }
    case QOMX_IndexParamAudioVoiceRecord:
    {
        QOMX_AUDIO_CONFIG_VOICERECORDTYPE *recordPath =
           (QOMX_AUDIO_CONFIG_VOICERECORDTYPE*)paramData;
        m_recPath = recordPath->eVoiceRecordMode ;
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
  omx_amr_aenc::GetConfig

DESCRIPTION
  OMX Get Config Method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::get_config(OMX_IN OMX_HANDLETYPE      hComp,
                                        OMX_IN OMX_INDEXTYPE configIndex,
                                        OMX_INOUT OMX_PTR     configData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  (void)hComp;
  (void)configData;
  switch(configIndex)
  {
  default:
    eRet = OMX_ErrorUnsupportedIndex;
    break;
  }
  return eRet;
}

/* ======================================================================
FUNCTION
  omx_amr_aenc::SetConfig

DESCRIPTION
  OMX Set Config method implementation

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.
========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::set_config(OMX_IN OMX_HANDLETYPE      hComp,
                                        OMX_IN OMX_INDEXTYPE configIndex,
                                        OMX_IN OMX_PTR        configData)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  (void)hComp;
  (void)configData;
  switch(configIndex)
  {
  default:
    eRet = OMX_ErrorUnsupportedIndex;
    break;
  }
  return eRet;
}

/* ======================================================================
FUNCTION
  omx_amr_aenc::GetExtensionIndex

DESCRIPTION
  OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::get_extension_index(OMX_IN OMX_HANDLETYPE      hComp,
                                                OMX_IN OMX_STRING      paramName,
                                                OMX_OUT OMX_INDEXTYPE* indexType)
{
    if((hComp == NULL) || (paramName == NULL) || (indexType == NULL))
    {
    DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
    return OMX_ErrorBadParameter;
    }
    if ( m_state == OMX_StateInvalid )
    {
        DEBUG_PRINT_ERROR("Get Extension Index in Invalid State\n");
    return OMX_ErrorInvalidState;
    }
    if(strncmp(paramName,OMX_QCOM_INDEX_PARAM_SESSIONID,
               strlen(OMX_QCOM_INDEX_PARAM_SESSIONID)) == 0)
    {
    *indexType =(OMX_INDEXTYPE)QOMX_IndexParamAudioSessionId;
        DEBUG_PRINT("Extension index type - %d\n", *indexType);
    }
    else if(strncmp(paramName,OMX_QCOM_INDEX_PARAM_VOICERECORDTYPE,
               strlen(OMX_QCOM_INDEX_PARAM_VOICERECORDTYPE)) == 0)
    {
        *indexType =(OMX_INDEXTYPE)QOMX_IndexParamAudioVoiceRecord;
        DEBUG_PRINT("Extension index type - %d\n", *indexType);
    }
    else
    {
    return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_amr_aenc::GetState

DESCRIPTION
  Returns the state information back to the caller.<TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::get_state(OMX_IN OMX_HANDLETYPE  hComp,
                                       OMX_OUT OMX_STATETYPE* state)
{
  (void)hComp;
  pthread_mutex_lock(&m_state_lock);
  *state = m_state;
  pthread_mutex_unlock(&m_state_lock);

  DEBUG_PRINT("Returning the state %d\n",*state);
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_amr_aenc::ComponentTunnelRequest

DESCRIPTION
  OMX Component Tunnel Request method implementation. <TBD>

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::component_tunnel_request(OMX_IN OMX_HANDLETYPE                hComp,
                                                     OMX_IN OMX_U32                        port,
                                                     OMX_IN OMX_HANDLETYPE        peerComponent,
                                                     OMX_IN OMX_U32                    peerPort,
                                                     OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup)
{
  DEBUG_PRINT_ERROR("Error: component_tunnel_request Not Implemented\n");
  (void)hComp;
  (void)port;
  (void)peerComponent;
  (void)peerPort;
  (void)tunnelSetup;
  return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_amr_aenc::UseBuffer

DESCRIPTION
  OMX Use Buffer method implementation. <TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None , if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::use_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                        OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                        OMX_IN OMX_U32                        port,
                                        OMX_IN OMX_PTR                     appData,
                                        OMX_IN OMX_U32                       bytes,
                                        OMX_IN OMX_U8*                      buffer)
{
  DEBUG_PRINT_ERROR("Error: use_buffer Not implemented\n");
  (void)hComp;
  (void)bufferHdr;
  (void)port;
  (void)appData;
  (void)bytes;
  (void)buffer;
  return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE  omx_amr_aenc::allocate_output_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                                  OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                                  OMX_IN OMX_U32                        port,
                                                  OMX_IN OMX_PTR                     appData,
                                                  OMX_IN OMX_U32                       bytes)
{
  OMX_ERRORTYPE         eRet = OMX_ErrorNone;
  OMX_BUFFERHEADERTYPE            *bufHdr; // buffer header
  unsigned                   nBufSize = max(bytes, OMX_AMR_OUTPUT_BUFFER_SIZE);
  char                       *buf_ptr;

  buf_ptr = (char *) calloc( (nBufSize + sizeof(OMX_BUFFERHEADERTYPE) ) , 1);

  (void)hComp;
  (void)port;
  if (buf_ptr != NULL) {
    bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
    *bufferHdr = bufHdr;
    memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

    bufHdr->pBuffer           = (OMX_U8 *)((buf_ptr) +
                                           sizeof(OMX_BUFFERHEADERTYPE));
    DEBUG_PRINT("AB::bufHdr %p bufHdr->pBuffer %p", bufHdr, bufHdr->pBuffer);
    bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
    bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
    bufHdr->nAllocLen         = nBufSize;
    bufHdr->pAppPrivate       = appData;
    bufHdr->nInputPortIndex   = OMX_CORE_OUTPUT_PORT_INDEX;
    m_output_buf_hdrs.insert(bufHdr, NULL);
    m_out_buf_count++;
    if(m_out_buf_count == 1) {
        pFirstOutputBuf = (OMX_U8 *)bufHdr->pBuffer;
    }
    if(m_out_buf_count == 2) {
        pSecondOutputBuf = (OMX_U8 *)bufHdr->pBuffer;
    }

  } else {
    DEBUG_PRINT("Output buffer memory allocation failed\n");
    eRet =  OMX_ErrorInsufficientResources;
  }

  return eRet;
}


// AllocateBuffer  -- API Call
/* ======================================================================
FUNCTION
  omx_amr_aenc::AllocateBuffer

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::allocate_buffer(OMX_IN OMX_HANDLETYPE                hComp,
                                     OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                     OMX_IN OMX_U32                        port,
                                     OMX_IN OMX_PTR                     appData,
                                     OMX_IN OMX_U32                       bytes)
{

    OMX_ERRORTYPE eRet = OMX_ErrorNone; // OMX return type


    if(port == OMX_CORE_OUTPUT_PORT_INDEX)
    {
      eRet = allocate_output_buffer(hComp,bufferHdr,port,appData,bytes);
    }
    else
    {
      DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",
                        (int)port);
      eRet = OMX_ErrorBadPortIndex;
    }

    if((eRet == OMX_ErrorNone) && (BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)))
    {
      DEBUG_PRINT("Checking for Output Allocate buffer Done");
      if(allocate_done())
      {
        // Send the callback now
        BITMASK_CLEAR(&m_flags, OMX_COMPONENT_IDLE_PENDING);
        post_event(OMX_CommandStateSet,OMX_StateIdle,
                   OMX_COMPONENT_GENERATE_EVENT, true);
      }
    }
    DEBUG_PRINT("Allocate Buffer exit with ret Code %d\n", eRet);
    return eRet;
}

/**
 @brief member function that searches for caller buffer

 @param buffer pointer to buffer header
 @return bool value indicating whether buffer is found
 */
bool omx_amr_aenc::search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{
  bool eRet = false;
  OMX_BUFFERHEADERTYPE *temp;

  //access only in IL client context
  temp = m_input_buf_hdrs.find_ele(buffer);
  if(buffer && temp)
  {
      DEBUG_PRINT("found %p \n", buffer);
      eRet = true;
  }

  return eRet;
}

/**
 @brief member function that searches for caller buffer

 @param buffer pointer to buffer header
 @return bool value indicating whether buffer is found
 */
bool omx_amr_aenc::search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{
  bool eRet = false;
  OMX_BUFFERHEADERTYPE *temp;


  //access only in IL client context
  temp = m_output_buf_hdrs.find_ele(buffer);
  if(buffer && temp)
  {
      DEBUG_PRINT("found %p \n", buffer);
      eRet = true;
  }

  return eRet;
}

// Free Buffer - API call
/**
  @brief member function that handles free buffer command from IL client

  This function is a block-call function that handles IL client request to
  freeing the buffer

  @param hComp handle to component instance
  @param port id of port which holds the buffer
  @param buffer buffer header
  @return Error status
*/
OMX_ERRORTYPE  omx_amr_aenc::free_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                      OMX_IN OMX_U32                 port,
                                      OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  DEBUG_PRINT("buf %p\n", buffer);
  (void)hComp;
  if(port == OMX_CORE_OUTPUT_PORT_INDEX)
  {
   if(search_output_bufhdr(buffer) == true)
    {
      /* Buffer exist */
      //access only in IL client context
      m_output_buf_hdrs.erase(buffer);
      m_out_buf_count--;
    } else {
      DEBUG_PRINT_ERROR("Error: free_buffer , invalid Output buffer header\n");
      eRet = OMX_ErrorBadParameter;
    }
  }
  else
  {
    eRet = OMX_ErrorBadPortIndex;
  }

  if((eRet == OMX_ErrorNone) &&
     (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)))
  {
    if(release_done())
    {
      // Send the callback now
      BITMASK_CLEAR((&m_flags),OMX_COMPONENT_LOADING_PENDING);
      post_event(OMX_CommandStateSet,
                 OMX_StateLoaded,OMX_COMPONENT_GENERATE_EVENT, false);
    }
  }

  return eRet;
}


/**
 @brief member function that that handles empty this buffer command

 This function meremly queue up the command and data would be consumed
 in command server thread context

 @param hComp handle to component instance
 @param buffer pointer to buffer header
 @return error status
 */
OMX_ERRORTYPE  omx_amr_aenc::empty_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                              OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;

  pthread_mutex_lock(&m_lock);
  if ((m_state != OMX_StateExecuting) &&
      (m_state != OMX_StatePause))
  {
    DEBUG_PRINT_ERROR("Invalid state\n");
    eRet = OMX_ErrorInvalidState;
  }
  pthread_mutex_unlock(&m_lock);

  if (eRet == OMX_ErrorNone) {
    if (search_input_bufhdr(buffer) == true) {
      post_event((unsigned)hComp,
                 (unsigned) buffer,OMX_COMPONENT_GENERATE_ETB, false);
    } else {
      DEBUG_PRINT_ERROR("Bad header %p \n", buffer);
      eRet = OMX_ErrorBadParameter;
    }
  }

  return eRet;
}
/**
  @brief member function that writes data to kernel driver

  @param hComp handle to component instance
  @param buffer pointer to buffer header
  @return error status
 */
OMX_ERRORTYPE  omx_amr_aenc::empty_this_buffer_proxy(OMX_IN OMX_HANDLETYPE         hComp,
                                                     OMX_BUFFERHEADERTYPE* buffer)
{

  /* Assume empty this buffer function has already checked
     validity of buffer */
  DEBUG_PRINT("Empty buffer %p to kernel driver\n", buffer);
  post_event((unsigned) & hComp,(unsigned) buffer,OMX_COMPONENT_GENERATE_BUFFER_DONE, true);

  return OMX_ErrorNone;
}


OMX_ERRORTYPE  omx_amr_aenc::fill_this_buffer_proxy(OMX_IN OMX_HANDLETYPE         hComp,
                                                     OMX_BUFFERHEADERTYPE* buffer)
{
    static unsigned int count = 0;
    static int fileheader_set = 0;
    int nDatalen = 0;
    OMX_U8 frameheader = 0;
    OMX_U8 framelength = 0;
    int filled_length = 0;
    OMX_U8 *src;
    OMX_U8 *dst;
    OMX_U8 *dstStart;
    OMX_U32 srcconsumed = 0;
    amrsup_frame_type           frame_type_in;
    amrsup_mode_type            frame_rate_in;
    amrsup_if1_frame_info_type  frame_info_out;
    OMX_U32 native_frame_start = 0;
    OMX_U32 destination_offset = 0;

    OMX_U8 buf[OMX_AMR_NATIVE_OUTPUT_BUFFER_SIZE];
    (void)hComp;
    memset(buf,0,OMX_AMR_NATIVE_OUTPUT_BUFFER_SIZE);
    DEBUG_PRINT("FTBP: %p %p readfd=%d\n",buffer,buffer->pBuffer,m_drv_fd);
    {
        nDatalen = read(m_drv_fd, buf,OMX_AMR_NATIVE_OUTPUT_BUFFER_SIZE);
        DEBUG_PRINT("FTBP:fd=%d read buffer %p  #%d of size = %d\n",m_drv_fd,buffer->pBuffer,
        ++count, nDatalen);
        if((nDatalen < 0) || (nDatalen > (signed)OMX_AMR_NATIVE_OUTPUT_BUFFER_SIZE)){
        DEBUG_PRINT("fill_this_buffer_proxy: Invalid data length read \n");

        } else {
            if(search_output_bufhdr(buffer) == true) {
                src = buf;
                dst = buffer->pBuffer;
                dstStart = buffer->pBuffer;
                switch((OMX_AUDIO_AMRFRAMEFORMATTYPE)pframeformat) {

                    case OMX_AUDIO_AMRFrameFormatFSF:
                    if(!fileheader_set) {
                        *dst++ = 0x23;
                        *dst++ = 0x21;
                        *dst++ = 0x41;
                        *dst++ = 0x4D;
                        *dst++ = 0x52;
                        *dst++ = 0x0A;
                        fileheader_set = 1;
                        filled_length   += HEADER_LENGTH;
                    }
            case OMX_AUDIO_AMRFrameFormatIF1:
          while((signed)srcconsumed < nDatalen){
             src += native_frame_start;
            dst += destination_offset;
            frame_type_in = (amrsup_frame_type)*(src++);
            if(pdtx == 1)
            {
                DEBUG_PRINT("DTX ENABLED********************\n");
                 //Doing frame type conversion when DTX is enabled*/
                 /*  Txframetype                           Rxframetype

                     TX_SPEECH_GOOD     0   ->               RX_SPEECH_GOOD       0
                                                             RX_SPEECH_DEGRADED   1
                                                             RX_ONSET             2
                                                             RX_SPEECH_BAD        3
                     TX_SID_FIRST       1   ->               RX_SID_FIRST         4
                     TX_SID_UPDATE      2   ->               RX_SID_UPDATE        5
                                                             RX_SID_BAD           6
                     TX_NO_DATA         3   ->               RX_NO_DATA           7 */
                switch(frame_type_in)
                {
                    case AMRSUP_SPEECH_DEGRADED:
                        frame_type_in = (amrsup_frame_type)AMRSUP_SID_FIRST;
                        DEBUG_PRINT("TX_SID_FIRST = %d\n",frame_type_in);
                        break;
                    case AMRSUP_ONSET:
                        frame_type_in = (amrsup_frame_type)AMRSUP_SID_UPDATE;
                        DEBUG_PRINT("TX_SID_UPDATE = %d\n",frame_type_in);
                        break;
                    case AMRSUP_SPEECH_BAD:
                        frame_type_in = (amrsup_frame_type)AMRSUP_NO_DATA;
                        DEBUG_PRINT("TX_NO_DATA =%d\n",frame_type_in);
                        break;
                    default:
                        DEBUG_PRINT("Unsupported frame type\n");
                }
            }
            frame_rate_in = (amrsup_mode_type)*(src++);
            amrsup_if1_framing(src,
                       frame_type_in,
                       frame_rate_in,
                       dst + 1,
                       &frame_info_out);
            frameheader = (frame_info_out.frame_type_index << 3) + (frame_info_out.fqi << 2);
            *dst = frameheader;
            framelength = g_frmInfo[frame_rate_in].frameLen;
            destination_offset = framelength + 1;
            filled_length    += framelength + 1;
            native_frame_start = NATIVE_FRAME_LENGTH - 2;
            srcconsumed += NATIVE_FRAME_LENGTH;
            switch(frame_type_in)
            {
                case AMRSUP_NO_DATA:
                filled_length =
                    g_frmInfo[(amrsup_frame_type_index_type)AMRSUP_FRAME_TYPE_INDEX_NO_DATA].frameLen + 1;
                    break;
                case AMRSUP_SID_BAD:
                case AMRSUP_SID_FIRST:
                case AMRSUP_SID_UPDATE:
                    filled_length =
                        g_frmInfo[(amrsup_frame_type_index_type)AMRSUP_FRAME_TYPE_INDEX_AMR_SID].frameLen + 1;
                        break;
                default:
                    DEBUG_PRINT("Unsupported frame type\n");
            }

        }
                    break;
#ifdef IF2_FORMAT /*IF2 has not been tested since lack of If2 decoder support*/
            case OMX_AUDIO_AMRFrameFormatIF2:
            while(srcconsumed < nDatalen){
              src += native_frame_start;
              dst += destination_offset;
              frame_type_in = (amrsup_frame_type)*(src++);
            if(pdtx == 1)
            {
                DEBUG_PRINT("DTX ENABLED********************\n");
                 //Doing frame type conversion when DTX is enabled*/
                 /*  Txframetype                           Rxframetype

                     TX_SPEECH_GOOD     0   ->               RX_SPEECH_GOOD       0
                                                             RX_SPEECH_DEGRADED   1
                                                             RX_ONSET             2
                                                             RX_SPEECH_BAD        3
                     TX_SID_FIRST       1   ->               RX_SID_FIRST         4
                     TX_SID_UPDATE      2   ->               RX_SID_UPDATE        5
                                                             RX_SID_BAD           6
                     TX_NO_DATA         3   ->               RX_NO_DATA           7 */
                switch(frame_type_in)
                {
                    case AMRSUP_SPEECH_DEGRADED:
                        frame_type_in = (amrsup_frame_type)AMRSUP_SID_FIRST;
                        DEBUG_PRINT("TX_SID_FIRST = %d\n",frame_type_in);
                        break;
                    case AMRSUP_ONSET:
                        frame_type_in = (amrsup_frame_type)AMRSUP_SID_UPDATE;
                        DEBUG_PRINT("TX_SID_UPDATE = %d\n",frame_type_in);
                        break;
                    case AMRSUP_SPEECH_BAD:
                        frame_type_in = (amrsup_frame_type)AMRSUP_NO_DATA;
                        DEBUG_PRINT("TX_NO_DATA =%d\n",frame_type_in);
                        break;
                }
            }
              frame_rate_in = (amrsup_mode_type)*(src++);
                   amrsup_if2_framing(src,
                         frame_type_in,
                     frame_rate_in,
                     dst);
                 frameheader = frame_rate_in;
             *dst = frameheader;
             framelength = g_frmInfo[frame_rate_in].frameLen;
             destination_offset = framelength + 1;
             filled_length    += framelength + 1;
             native_frame_start = NATIVE_FRAME_LENGTH - 2;
             srcconsumed += NATIVE_FRAME_LENGTH;
             }
            break;
#endif
                    default :
                    DEBUG_PRINT("UNSUPPORTED AMR FILEFORMAT \n");
                    return OMX_ErrorUnsupportedSetting;
                    break;
                }
            buffer->pBuffer  = dstStart;
            buffer->nFilledLen = filled_length;
            frame_done_cb(buffer);
            }
        }
    }
    return OMX_ErrorNone;

}

/* ======================================================================
FUNCTION
  omx_amr_aenc::FillThisBuffer

DESCRIPTION
  IL client uses this method to release the frame buffer
  after displaying them.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::fill_this_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                                  OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT("FTB :: %p %p\n",buffer,buffer->pBuffer);
    post_output((unsigned)hComp,
                 (unsigned) buffer,OMX_COMPONENT_GENERATE_FTB, false);
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_amr_aenc::SetCallbacks

DESCRIPTION
  Set the callbacks.

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                                           OMX_IN OMX_CALLBACKTYPE* callbacks,
                                           OMX_IN OMX_PTR             appData)
{
  (void)hComp;
  m_cb       = *callbacks;
  m_app_data =    appData;

  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_amr_aenc::ComponentDeInit

DESCRIPTION
  Destroys the component and release memory allocated to the heap.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
  (void)hComp;
  if (OMX_StateLoaded != m_state)
  {
      DEBUG_PRINT_ERROR("Warning: Received DeInit when not in LOADED state, cur_state %d\n",
                   m_state);
      return OMX_ErrorInvalidState;
  }

  if (m_cmd_svr != NULL) {
    aenc_svr_stop(m_cmd_svr);
    m_cmd_svr = NULL;
  }

  if (m_cmd_cln != NULL) {
    aenc_cln_stop(m_cmd_cln);
    m_cmd_cln = NULL;
  }

  m_recPath=0;

  if (m_drv_fd >= 0) {
    close(m_drv_fd);
    m_drv_fd = -1;
  }
  else
  {
    DEBUG_PRINT(" device close failure \n");
  }
  return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_amr_aenc::UseEGLImage

DESCRIPTION
  OMX Use EGL Image method implementation <TBD>.

PARAMETERS
  <TBD>.

RETURN VALUE
  Not Implemented error.

========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::use_EGL_image(OMX_IN OMX_HANDLETYPE                hComp,
                                          OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                          OMX_IN OMX_U32                        port,
                                          OMX_IN OMX_PTR                     appData,
                                          OMX_IN void*                      eglImage)
{
    DEBUG_PRINT_ERROR("Error : use_EGL_image:  Not Implemented \n");
    (void)hComp;
    (void)bufferHdr;
    (void)port;
    (void)appData;
    (void)eglImage;
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_amr_aenc::ComponentRoleEnum

DESCRIPTION
  OMX Component Role Enum method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_amr_aenc::component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
                                                OMX_OUT OMX_U8*        role,
                                                OMX_IN OMX_U32        index)
{
  OMX_ERRORTYPE eRet = OMX_ErrorNone;
  const char *cmp_role = "audio_encoder.amr";

  (void)hComp;
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
  omx_amr_aenc::AllocateDone

DESCRIPTION
  Checks if entire buffer pool is allocated by IL Client or not.
  Need this to move to IDLE state.

PARAMETERS
  None.

RETURN VALUE
  true/false.

========================================================================== */
bool omx_amr_aenc::allocate_done(void)
{
  return (m_out_buf_count >= 1?true:false);
  return TRUE;
}

/* ======================================================================
FUNCTION
  omx_amr_aenc::ReleaseDone

DESCRIPTION
  Checks if IL client has released all the buffers.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_amr_aenc::release_done(void)
{
  return (m_out_buf_count == 0?true:false);

}

