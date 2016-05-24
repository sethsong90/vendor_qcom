#ifndef AMRSUPI_H
#define AMRSUPI_H

/*===========================================================================

                  M U L T I M O D E   V O I C E   S E R V I C E S
                    A M R   F R A M I N G   P R O C E S S I N G

DESCRIPTION
  This header file contains definitions used by the AMR framing functions.

REFERENCES
  3G TS 26.093 - "AMR Speech Codec; Source Controlled Rate Operation"
  3G TS 26.101 - "AMR Speech Codec; Frame structure"

  Copyright(c) 2003 - 2008 by Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
===========================================================================*/

/*===========================================================================

                    D A T A   D E F I N I T I O N S

===========================================================================*/

/* Definitions for the number of bits in classes A, B, and C for each AMR
** codec mode.
*/

//start

//typedef unsigned char OMX_U8;
//typedef unsigned short OMX_U16;
//typedef unsigned long OMX_U32;

#define AUDFADEC_AMR_FRAME_TYPE_MASK     0x78
#define AUDFADEC_AMR_FRAME_TYPE_SHIFT    3
#define AUDFADEC_AMR_FRAME_QUALITY_MASK  0x04

//#define INPUT_BUFFER_SIZE 1008
//#define OUTPUT_BUFFER_SIZE 3024
#define RESIDUALDATA_BUFFER_SIZE 1024
#define MAX_FRAME_LEN 32
#define TRANS_LENGTH 36

#define TRUE 1
#define FALSE 0

//end


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


/* Structure containing bit orders
*/
typedef struct {
  int   len_a;
  OMX_U16 *class_a;
  int   len_b;
  OMX_U16 *class_b;
  int   len_c;
  OMX_U16 *class_c;
} amrsup_frame_order_type;

#endif /* AMRSUPI_H */
