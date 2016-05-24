/*
 * Copyright (C) 2007-2009 Qualcomm Technologies, Inc. All rights reserved. Proprietary and Confidential.
 */

/******************************************************************************
 * aniTypes.h
 *
 * This file contains the standard type definitions used by various
 * Airgo Applications.
 *
 * U.Loganathan, Krishna Reddy, 09/03/2002
 *
 ******************************************************************************
 * ALL RIGHTS RESERVED, Airgo Networks, Inc. 
 * No part of this file/software maybe copied or reproduced in any form without
 * the written permission of:
 *
 * Airgo Networks, Inc.
 * 900 Arastradero Road, Palo Alto, CA 94304, USA.
 * Tel: +1-650-475-1900
 * http://www.woodsidenet.com
 * 
 ******************************************************************************
 * (C) Copyright 2002, Airgo Networks, Inc., Palo Alto, CA 94304, USA
 ******************************************************************************/
#ifndef _ANI_TYPES_H_
#define _ANI_TYPES_H_


#define ANI_INLINE_FUNCTION static __inline__

// Common type definitions...

typedef unsigned char  tANI_U8;
typedef   signed char  tANI_S8;

typedef unsigned short tANI_U16;
typedef   signed short tANI_S16;

typedef unsigned long  tANI_U32;
typedef   signed long  tANI_S32;

#if defined ANI_OS_TYPE_WINDOWS
typedef unsigned __int64 tANI_U64;
#else
typedef unsigned long long tANI_U64;
#endif

#if defined( ANI_OS_TYPE_WINDOWS )
#if defined(_WIN64)
 typedef unsigned __int64 tANI_U32_OR_PTR;
#else
  typedef unsigned long tANI_U32_OR_PTR;
#endif
#else
  typedef tANI_U32     tANI_U32_OR_PTR;
#endif

// Buffer address; could be virt or phys; could be 32- or 64-bit depending on compile option
typedef tANI_U32_OR_PTR    tANI_BUFFER_ADDR;

typedef tANI_U8 tANI_BOOLEAN;

typedef enum tagAniBoolean 
{
   eANI_BOOLEAN_FALSE = 0,
   eANI_BOOLEAN_TRUE,
              
   eANI_BOOLEAN_OFF = 0,
   eANI_BOOLEAN_ON = 1,
} eAniBoolean;

#define ANI_MAC_ADDR_SIZE ( 6 )
typedef tANI_U8 tAniMacAddr[ ANI_MAC_ADDR_SIZE ];

typedef eAniBoolean tAniBoolean;
typedef unsigned char  ANI_U8;
typedef unsigned short ANI_U16;
typedef unsigned long  ANI_U32;
typedef signed char    ANI_S8;
typedef signed short   ANI_S16;
typedef signed long    ANI_S32;

/**
 * A lightweight structure meant for iteration support. One example of
 * its use is in the aniAsfHashTableGetIterator function. Typically,
 * such functions populate this structure with an array of pointers to
 * the set of data they wish to expose.
 *
 * An API call that populates this structure will make it clear what
 * the responsiblity of the caller is towards freeing any associated
 * storage.
 */
typedef struct sAniAsfIterator {
    int numValues;
    void **values;
} tAniAsfIterator;

typedef int aniError;

/*
 * Structure packing macro
 * Compiler specific (for GCC)
 */
#define ANI_PACK_STRUC __attribute__((packed))

#endif /* ifndef _ANI_TYPES_H_ */
