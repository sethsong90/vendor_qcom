/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2012 by Qualcomm Technologies, Inc.  All rights reserved. All data and   *]
[* information contained in or disclosed by this document is confidental and *]
[* proprietary information of Qualcomm Technologies, Inc. and all rights therein are       *]
[* expressly reserved. By accepting this material the recipient agrees that  *]
[* this  material and the information contained therein is held in confidence*]
[* and in trust and will not be used, copied, reproduced in whole or in part,*]
[* nor its contents revealed in  any manner to others without the express    *]
[* written permission of Qualcomm Technologies, Inc.                                      *]
[*===========================================================================*]
[*****************************************************************************]
[* FILE NAME:   comdef.h                    TYPE: C-header file              *]
[* DESCRIPTION: Contains the typedefs of various data types that should be   *]
[*   used instead of the actual data types so as to have common sizes across *]
[*   platforms.                                                              *]
[*   when       who     what, where, why                                     *]
[*   --------   ---     -----------------------------------------------------*]
[*   06/06/05   sdk     Initial revision                                     *]
[*****************************************************************************/


#ifndef _COMDEF_H_
#define _COMDEF_H_

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE   1                            /* Boolean true value.           */
#define FALSE  0                            /* Boolean false value.          */

#define  ON   1                             /* On value.                     */
#define  OFF  0                             /* Off value.                    */

#ifdef _lint
  #define NULL 0
#endif

#ifndef NULL
  #define NULL  0
#endif

#if defined(__qdsp6__) || defined(__GNUC__) || defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 220000)
#define MEM_ALIGN(t, a)    t __attribute__((aligned(a)))
#elif defined(_MSC_VER)
#define MEM_ALIGN(t, a)    __declspec(align(a)) t
#elif defined(__ARMCC_VERSION)
#define MEM_ALIGN(t, a)    __align(a) t
#else
#error Unknown compiler
#endif

/* Align an offset to multiple of 4 (useful for aligning word offsets
   to 64-bit boundary */
#define ALIGN4(o)         (((o)+3)&(~3))
/* Align an offset to multiple of 8 (useful for aligning byte offsets
   to 64-bit boundary */
#define ALIGN8(o)         (((o)+7)&(~7))

/*=============================================================================
      Standard types
=============================================================================*/
typedef  unsigned char      boolean;        /* Boolean value type.           */

typedef  unsigned long int  uint32;         /* Unsigned 32 bit value         */
typedef  unsigned short     uint16;         /* Unsigned 16 bit value         */
typedef  unsigned char      uint8;          /* Unsigned 8  bit value         */

typedef  signed long int    int32;          /* Signed 32 bit value           */
typedef  signed short       int16;          /* Signed 16 bit value           */
typedef  signed char        int8;           /* Signed 8  bit value           */
#if defined(__GNUC__)
typedef  signed long long   int64;          /* Signed 64 bit value           */
typedef  unsigned long long uint64;         /* Unsigned 64 bit value         */
typedef  signed long long   int40;          /* Signed 40 bit value           */
#else
typedef  signed __int64   int64;            /* Signed 64 bit value           */
typedef  unsigned __int64 uint64;           /* Unsigned 64 bit value         */
typedef  signed __int64   int40;            /* Signed 40 bit value           */
#endif

typedef int16                  Word16;
typedef uint16                 UNS_Word16;
typedef int32                  Word32;
typedef uint32                 UWord32;
//typedef double Word40;                    // 40-bit Long Signed integer
typedef int64                  Word64;

typedef int32                  CWord2x16;
typedef int64                  CWord2x32;

#endif /* _COMDEF_H_ */
