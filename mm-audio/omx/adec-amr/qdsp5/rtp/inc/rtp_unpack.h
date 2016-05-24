#ifndef _RTP_UNPACK_H
#define _RTP_UNPACK_H

#define FEATURE_QVPHONE_RTP
#define FEATURE_ALIGNED_QWORD
#define PACKED

#define QVP_RTP_MTU_DLFT            330       /* for testing mtu size */


//values are for testing purpose
#define RTP_INPUT_BUFFER_SIZE 1300//1600
#define RTP_OUTPUT_BUFFER_SIZE 1008
#define RTP_RESIDUAL_BUFFER_SIZE 100

#define TOC_MASK 0x7f
#define TOC_FLAG_MASK 0x80
#define TOC_MODE_MASK 0x78
#define TOC_FQI_MASK 0x40

#define RTP_VERSION_MASK 0xC0
#define RTP_VERSION 0x80
#define RTP_PAYLOAD_TYPE 0x61

#define OFFSET_SEVEN 7
#define OFFSET_THREE 3
#define OFFSET_TWO 2

#define MAX_FRAMES 4
#define RTP_FIXED_HEADER_SIZE 13

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */

#define  ON   1    /* On value. */
#define  OFF  0    /* Off value. */

typedef  unsigned char      boolean;     /* Boolean value type. */

typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
typedef  unsigned short     uint16;      /* Unsigned 16 bit value */
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */

typedef  signed long int    int32;       /* Signed 32 bit value */
typedef  signed short       int16;       /* Signed 16 bit value */
typedef  signed char        int8;        /* Signed 8  bit value */

/* This group are the deprecated types.  Their use should be
** discontinued and new code should use the types above
*/
typedef  unsigned char     byte;         /* Unsigned 8  bit value type. */
typedef  unsigned short    word;         /* Unsinged 16 bit value type. */
typedef  unsigned long     dword;        /* Unsigned 32 bit value type. */

typedef  unsigned char     uint1;        /* Unsigned 8  bit value type. */
typedef  unsigned short    uint2;        /* Unsigned 16 bit value type. */
typedef  unsigned long     uint4;        /* Unsigned 32 bit value type. */
typedef  unsigned long long      uint64;       /* Signed 64 bit value */

typedef  signed char       int1;         /* Signed 8  bit value type. */
typedef  signed short      int2;         /* Signed 16 bit value type. */
typedef  long int          int4;         /* Signed 32 bit value type. */

typedef  signed long       sint31;       /* Signed 32 bit value */
typedef  signed short      sint15;       /* Signed 16 bit value */
typedef  signed char       sint7;        /* Signed 8  bit value */

typedef unsigned int size_t;

#ifndef QW_DECLARED
#define QW_DECLARED
typedef unsigned long qword[ 2 ];

#ifdef FEATURE_ALIGNED_QWORD
  typedef unsigned long qc_qword[ 2 ];
#else
  typedef PACKED unsigned long qc_qword[ 2 ];
#endif
#endif

#define LOCAL static

#define MSG_HIGH(str, a, b, c) printf("HIGH: " str, a, b, c)
#define MSG_MED(str, a, b, c) printf("HIGH: " str, a, b, c)
#define MSG_LOW(str, a, b, c) printf("HIGH: " str, a, b, c)


#define MSG_SSID_DS_RTP     5016
/*---------------------------------------------------------------------------
  These masks are to be used for support of all legacy messages in the sw.
  The user does not need to remember the names as they will be embedded in
  the appropriate macros.
---------------------------------------------------------------------------*/
#define MSG_LEGACY_LOW      MSG_MASK_0
#define MSG_LEGACY_MED      MSG_MASK_1
#define MSG_LEGACY_HIGH     MSG_MASK_2
#define MSG_LEGACY_ERROR    MSG_MASK_3
#define MSG_LEGACY_FATAL    MSG_MASK_4
//extern FILE *rtplog;
#define MSG_3( a, b, xx_fmt, xx_arg1, xx_arg2, xx_arg3) {printf( xx_fmt, xx_arg1, xx_arg2, xx_arg3 ); printf("\n");}
#define  ERR(format, code1, code2, code3) {printf( format, code1, code2, code3 ); printf("\n");}

#ifndef MAX
   #define  MAX( x, y ) ( ((x) > (y)) ? (x) : (y) )
#endif

#ifndef MIN
   #define  MIN( x, y ) ( ((x) < (y)) ? (x) : (y) )
#endif

//uint8 rtp_api(qvp_rtp_buf_type *aud_buf, qvp_rtp_ctx_type *ctxStruct, qvp_rtp_buf_type *pbufType);

void*  qvp_rtp_malloc
(
  size_t buf_size
);

//uint8 rtp_api(qvp_rtp_buf_type *aud_buf, qvp_rtp_ctx_type *ctxStruct, qvp_rtp_buf_type *pbufType);

#endif //_RTP_UNPACK_H
