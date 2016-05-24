#ifndef AMRSUP_H
#define AMRSUP_H

/*===========================================================================

                    A M R   F R A M I N G   P R O C E S S I N G
                    S U P P L E M E N T A L   F U N C T I O N S

DESCRIPTION
  This header file contains definitions used by the AMR framing functions.

REFERENCES
  3G TS 26.093 - "AMR Speech Codec; Source Controlled Rate Operation"
  3G TS 26.101 - "AMR Speech Codec; Frame structure"

  Copyright(c) 2003 - 2008 by Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.


                      EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

when       who     what, where, why
--------   ---     ----------------------------------------------------------
02/08/08   sud     Added support for AMR-WB Vocoder.
                   Updated Copyright.
07/22/05   ymc     Added FEATURE_AMR_IF1 support.
           ymc     Updated Copyright.
08/18/04   ymc     Added crc_err in amrsup_de_framing.
           ymc     Updated Copyright.
           ymc     Added amrsup_move_valid_bits, amrsup_get_frame_type,
                   amrsup_frame_class_bits, amrsup_frame_len_bits.
06/29/03   ymc     Added amrsup_frame_len.
           ymc     Added frame length return for amrsup_if2_framing and
                   amrsup_if2_de_framing.
04/17/03    sm     Initial revision taken from mvsamr.h.
=================================================================================*/

/* AMR frame type definitions */
typedef enum {
  AMRSUP_SPEECH_GOOD,          /* Good speech frame              */
  AMRSUP_SPEECH_DEGRADED,      /* Speech degraded                */
  AMRSUP_ONSET,                /* onset                          */
  AMRSUP_SPEECH_BAD,           /* Corrupt speech frame (bad CRC) */
  AMRSUP_SID_FIRST,            /* First silence descriptor       */
  AMRSUP_SID_UPDATE,           /* Comfort noise frame            */
  AMRSUP_SID_BAD,              /* Corrupt SID frame (bad CRC)    */
  AMRSUP_NO_DATA,              /* Nothing to transmit            */
  AMRSUP_SPEECH_LOST,          /* Lost speech in downlink        */
  AMRSUP_FRAME_TYPE_MAX
} amrsup_frame_type;

/* AMR frame mode (frame rate) definitions */
typedef enum {
  AMRSUP_MODE_0475,    /* 4.75 kbit /s */
  AMRSUP_MODE_0515,    /* 5.15 kbit /s */
  AMRSUP_MODE_0590,    /* 5.90 kbit /s */
  AMRSUP_MODE_0670,    /* 6.70 kbit /s */
  AMRSUP_MODE_0740,    /* 7.40 kbit /s */
  AMRSUP_MODE_0795,    /* 7.95 kbit /s */
  AMRSUP_MODE_1020,    /* 10.2 kbit /s */
  AMRSUP_MODE_1220,    /* 12.2 kbit /s */
  AMRSUP_MODE_MAX
} amrsup_mode_type;

/* The AMR classes
*/
typedef enum  {
  AMRSUP_CLASS_A,
  AMRSUP_CLASS_B,
  AMRSUP_CLASS_C
} amrsup_class_type;

/* The maximum number of bits in each class */
#define AMRSUP_CLASS_A_MAX 81
#define AMRSUP_CLASS_B_MAX 405
#define AMRSUP_CLASS_C_MAX 60

/* The size of the buffer required to hold the vocoder frame */
#define AMRSUP_VOC_FRAME_BYTES  \
  ((AMRSUP_CLASS_A_MAX + AMRSUP_CLASS_B_MAX + AMRSUP_CLASS_C_MAX + 7) / 8)

/* Size of each AMR class to hold one frame of AMR data */
#define AMRSUP_CLASS_A_BYTES ((AMRSUP_CLASS_A_MAX + 7) / 8)
#define AMRSUP_CLASS_B_BYTES ((AMRSUP_CLASS_B_MAX + 7) / 8)
#define AMRSUP_CLASS_C_BYTES ((AMRSUP_CLASS_C_MAX + 7) / 8)


/* Number of bytes for an AMR IF2 frame */
#define AMRSUP_IF2_FRAME_BYTES 32

/* Frame types for 4-bit frame type as in 3GPP TS 26.101 v3.2.0, Sec.4.1.1 */
typedef enum {
  AMRSUP_FRAME_TYPE_INDEX_0475    = 0,    /* 4.75 kbit /s    */
  AMRSUP_FRAME_TYPE_INDEX_0515    = 1,    /* 5.15 kbit /s    */
  AMRSUP_FRAME_TYPE_INDEX_0590    = 2,    /* 5.90 kbit /s    */
  AMRSUP_FRAME_TYPE_INDEX_0670    = 3,    /* 6.70 kbit /s    */
  AMRSUP_FRAME_TYPE_INDEX_0740    = 4,    /* 7.40 kbit /s    */
  AMRSUP_FRAME_TYPE_INDEX_0795    = 5,    /* 7.95 kbit /s    */
  AMRSUP_FRAME_TYPE_INDEX_1020    = 6,    /* 10.2 kbit /s    */
  AMRSUP_FRAME_TYPE_INDEX_1220    = 7,    /* 12.2 kbit /s    */
  AMRSUP_FRAME_TYPE_INDEX_AMR_SID = 8,    /* SID frame       */
/* Frame types 9-11 are not supported */
  AMRSUP_FRAME_TYPE_INDEX_NO_DATA = 15,   /* No data         */
  AMRSUP_FRAME_TYPE_INDEX_MAX,
  AMRSUP_FRAME_TYPE_INDEX_UNDEF = AMRSUP_FRAME_TYPE_INDEX_MAX
} amrsup_frame_type_index_type;

#define AMRSUP_FRAME_TYPE_INDEX_MASK         0x0F /* All frame types */
#define AMRSUP_FRAME_TYPE_INDEX_SPEECH_MASK  0x07 /* Speech frame    */

typedef enum {
  AMRSUP_CODEC_AMR_NB,
  AMRSUP_CODEC_AMR_WB,
  AMRSUP_CODEC_MAX
} amrsup_codec_type;

/* IF1-encoded frame info */
typedef struct {
  amrsup_frame_type_index_type frame_type_index;
  OMX_U8 fqi;    /* frame quality indicator: TRUE: good frame, FALSE: bad */
  amrsup_codec_type amr_type;   /* AMR-NB or AMR-WB */
} amrsup_if1_frame_info_type;

#define AUDFADEC_AMR_FRAME_TYPE_MASK     0x78
#define AUDFADEC_AMR_FRAME_TYPE_SHIFT    3
#define AUDFADEC_AMR_FRAME_QUALITY_MASK  0x04

#define AMR_CLASS_A_BITS_BAD   0

#define AMR_CLASS_A_BITS_SID  39

#define AMR_CLASS_A_BITS_475  42
#define AMR_CLASS_B_BITS_475  53
#define AMR_CLASS_C_BITS_475   0

#define AMR_CLASS_A_BITS_515  49
#define AMR_CLASS_B_BITS_515  54
#define AMR_CLASS_C_BITS_515   0

#define AMR_CLASS_A_BITS_590  55
#define AMR_CLASS_B_BITS_590  63
#define AMR_CLASS_C_BITS_590   0

#define AMR_CLASS_A_BITS_670  58
#define AMR_CLASS_B_BITS_670  76
#define AMR_CLASS_C_BITS_670   0

#define AMR_CLASS_A_BITS_740  61
#define AMR_CLASS_B_BITS_740  87
#define AMR_CLASS_C_BITS_740   0

#define AMR_CLASS_A_BITS_795  75
#define AMR_CLASS_B_BITS_795  84
#define AMR_CLASS_C_BITS_795   0

#define AMR_CLASS_A_BITS_102  65
#define AMR_CLASS_B_BITS_102  99
#define AMR_CLASS_C_BITS_102  40

#define AMR_CLASS_A_BITS_122  81
#define AMR_CLASS_B_BITS_122 103
#define AMR_CLASS_C_BITS_122  60

typedef struct {
  int   len_a;
  OMX_U16 *class_a;
  int   len_b;
  OMX_U16 *class_b;
  int   len_c;
  OMX_U16 *class_c;
} amrsup_frame_order_type;

/* <EJECT> */

/*===========================================================================

FUNCTION amrsup_if1_framing

DESCRIPTION
  Performs the transmit side framing function.  Generates AMR IF1 ordered data
  from the vocoder packet and frame type.

DEPENDENCIES
  None.

RETURN VALUE
  number of bytes of encoded frame.
  if1_frame : IF1-encoded frame.
  if1_frame_info : holds frame information of IF1-encoded frame.

SIDE EFFECTS
  None.

===========================================================================*/
extern int amrsup_if1_framing(
  OMX_U8                      *vocoder_packet,
  amrsup_frame_type          frame_type,
  amrsup_mode_type           amr_mode,
  OMX_U8                      *if1_frame,
  amrsup_if1_frame_info_type *if1_frame_info
);

/* <EJECT> */
/*===========================================================================

FUNCTION amrsup_if1_de_framing

DESCRIPTION
  Performs the receive side de-framing function. Generates a vocoder packet
  and frame type information from the AMR IF1 input data.

DEPENDENCIES
  None.

RETURN VALUE
  number of bytes of decoded frame.
  vocoder_packet : decoded packet
  frame_type     : AMR frame type of decoded packet
  amr_mode       : AMR frame rate of decoded packet

SIDE EFFECTS
  None.

===========================================================================*/
extern int amrsup_if1_de_framing(
  OMX_U8                      *vocoder_packet,
  amrsup_frame_type          *frame_type,
  amrsup_mode_type           *amr_mode,
  OMX_U8                      *if1_frame,
  amrsup_if1_frame_info_type *if1_frame_info
);



/* <EJECT> */


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
extern int amrsup_if2_framing(
  OMX_U8              *vocoder_packet,
  amrsup_frame_type  frame_type,
  amrsup_mode_type   amr_mode,
  OMX_U8              *if2_frame
);


/* <EJECT> */
/*===========================================================================

FUNCTION amrsup_if2_de_framing

DESCRIPTION
  Performs the receive side de-framing function. Generates a vocoder packet
  and frame type information from the AMR IF2 input data.

DEPENDENCIES
  None.

RETURN VALUE
  number of bytes of decoded frame.

SIDE EFFECTS
  None.

===========================================================================*/
extern int amrsup_if2_de_framing(
  OMX_U8              *vocoder_packet,
  amrsup_frame_type  *frame_type,
  amrsup_mode_type   *amr_mode,
  OMX_U8              *if2_frame
);



/* <EJECT> */

/*===========================================================================

FUNCTION amrsup_audfmt_framing

DESCRIPTION
  Performs the transmit side framing function. Generates class-divided and
  ordered data from the vocoder packet and frame type input data.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void amrsup_audfmt_framing(
  OMX_U8              *vocoder_packet,
  amrsup_frame_type  frame_type,
  amrsup_mode_type   amr_mode,
  OMX_U8              *amr_data
);


/* <EJECT> */
/*===========================================================================

FUNCTION amrsup_audfmt_de_framing

DESCRIPTION
  Performs the receive side de-framing function. Generates a vocoder packet
  and frame type information from the class-divided and ordered input data.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void amrsup_audfmt_de_framing(
  OMX_U8              *vocoder_packet,
  amrsup_frame_type  frame_type,
  amrsup_mode_type   amr_mode,
  OMX_U8              *amr_data
);



/* <EJECT> */
/*===========================================================================

FUNCTION amrsup_frame_class_bits

DESCRIPTION
  This function will determine number of bits of certain class
based on the frame type and frame rate.

DEPENDENCIES
  None.

RETURN VALUE
  number of bits of AMR frame in certain class

SIDE EFFECTS
  None.

===========================================================================*/
extern int amrsup_frame_class_bits(
  amrsup_frame_type frame_type,
  amrsup_mode_type amr_mode,
  amrsup_class_type amr_class
);


/* <EJECT> */
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
extern int amrsup_frame_len_bits(
  amrsup_frame_type frame_type,
  amrsup_mode_type amr_mode
);


/* <EJECT> */
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
extern int amrsup_frame_len(
  amrsup_frame_type frame_type,
  amrsup_mode_type amr_mode
);

#endif  /* AMRSUP_H */

