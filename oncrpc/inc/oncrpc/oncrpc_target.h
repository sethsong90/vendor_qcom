#ifndef _RPC_TYPES_H
#define _RPC_TYPES_H 1
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                   O N C R P C T Y P E S _ L I N U X . H

GENERAL DESCRIPTION

  This header file provides mappings between the original ONCRPC
  types, and the native types. 

  ONCRPC (Open Network Computing Remote Procedure Calls) is a way of
  calling functions from one machine to another.  This is described in
  detail in the following RFCs: 1831, 1832, 1833

INITIALIZATION AND SEQUENCING REQUIREMENTS

 Copyright (c) 2005,2007 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc/oncrpc_target.h#1 $ 

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/11/07    al     Add more definitions required for compiling glue code
11/20/07    ptm    Add heap size definition.
10/08/07    ptm    Add some of the priority definitions.
07/10/07    ptm    Remove featurization.
05/29/07    clp    Created Linux OS depended version of file.
05/21/07    hn     Moved definitions of protocols to oncrpctypes.h.
05/09/07    hn     Defined ONCRPC_INFINITY constant to -1.
03/31/05    clp    cleaned out unused stuff.
03/16/05    clp    Added header to source file.
===========================================================================*/


/* Parts of this file are derived from the original Sun (ONC) RPC
 * code, under the following copyright:
 */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
/* fixincludes should not add extern "C" to this file */
/*
 * Rpc additions to <sys/types.h>
 */ 


#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

#ifndef COMDEF_H
#define COMDEF_H
/* If we are included from an application, we don't want to include
 * ANY AMSS header files, as extra stuff conflicts with the native
 * header files, hence for this case, we define what we need here.
 * However, it is ok to include AMSS header files from the oncrpc
 * library code itself, since it needs all of the various types.  In
 * this case, we need to include comdef before including this file to
 * not get these definitions. 
 */
/* Types we need from comdef.h */
typedef  unsigned char      boolean;     /* Boolean value type. */
typedef  unsigned long long uint64;      /* Unsigned 64 bit value */
typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
typedef  unsigned short     uint16;      /* Unsigned 16 bit value */
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */
typedef  signed long long   int64;       /* Signed 64 bit value */
typedef  signed long int    int32;       /* Signed 32 bit value */
typedef  signed short       int16;       /* Signed 16 bit value */
typedef  signed char        int8;        /* Signed 8  bit value */

typedef  unsigned long      dword;       /* Unsigned 32 bit value */
typedef  unsigned short     word;        /* Unsigned 16 bit value */
typedef  unsigned char      byte;        /* Unsigned 8  bit value */
typedef  signed char        int1;        /* Unsigned 8  bit value */
typedef  signed short       int2;        /* Unsigned 16 bit value */
typedef  signed long        int4;        /* Unsigned 32 bit value */

typedef  signed long        sint31;      /* Signed 32 bit value */
typedef  signed short       sint15;      /* Signed 16 bit value */
typedef  signed char        sint7;       /* Signed 8  bit value */

/*********************** BEGIN PACK() Definition ***************************/

#if defined __GNUC__
  #define PACK(x)       x __attribute__((__packed__))
#elif defined __GNUG__
  #define PACK(x)       x __attribute__((__packed__))
#elif defined __arm
  #define PACK(x)       __packed x
#elif defined _WIN32
  #define PACK(x)       x /* Microsoft uses #pragma pack() prologue/epilogue */
#else
  #define PACK(x)       x __attribute__((__packed__))
#endif

/********************** END PACK() Definition *****************************/

#define ON  1
#define OFF 0

#define FSIZ( type, field ) sizeof( ((type *) 0)->field )

#if defined(__ARMCC_VERSION) 
  #define PACKED __packed
  #define PACKED_POST
  #define ALIGN(__value) __align(__value)
  #define POST_ALIGN(__value)
  #define INLINE __inline
  #define inline __inline
#else  /* __GNUC__ */
  #define PACKED 
  #define ALIGN(__value) __attribute__((__aligned__(__value)))
  #define POST_ALIGN(__value)
  #define PACKED_POST    __attribute__((__packed__))
  #ifndef INLINE
    #define INLINE inline
  #endif
#endif /* defined (__GNUC__) */
#endif /* COMDEF_H */

/* For htonl et al. */
#include <netinet/in.h>

#include "queue.h"
#ifndef QW_H
#define QW_H
typedef unsigned long qword[ 2 ];
#endif /* QW_H */

/* Type we need from DSM */
#ifndef DSM_H
#define DSM_H
struct dsm_item_s;
typedef struct dsm_item_s dsm_item_type;
typedef int dsm_mempool_id_enum_type;
struct dsm_watermark_type_s;
typedef struct dsm_watermark_type_s dsm_watermark_type;
#endif /*DSM_H*/



/* RPC types */
typedef  int32 bool_t;
typedef  int32 enum_t;
typedef uint32 rpcprog_t;
typedef uint32 rpcvers_t;
typedef uint32 rpcproc_t;
typedef  int32 rpcprot_t;
typedef uint32 rpcport_t;

#define __BEGIN_DECLS 
#define __END_DECLS
#ifndef __THROW
#define __THROW
#endif /* THROW */

typedef int64 quad_t;
typedef uint64 u_quad_t;

#ifndef TRUE
#define TRUE (1)
#endif /* TRUE */

#ifndef FALSE
#define FALSE (0)
#endif /* FALSE */



#define ONCRPC_INFINITY (-1)

/* Define the non-standard protocols -
   the standard ones are IPPROTO_TCP and IPPROTO_UDP */
#define ONCRPC_SIO_PROTOCOL            ((rpcprot_t)(-1))
#define ONCRPC_DIAG_PROTOCOL           ((rpcprot_t)(-2))
#define ONCRPC_SM_PROTOCOL             ((rpcprot_t)(-3))
#define ONCRPC_RTR_PROTOCOL            ((rpcprot_t)(-4))
#define ONCRPC_LO_PROTOCOL             ((rpcprot_t)(-5))

#if defined(FEATURE_ONCRPC_SM_IS_ROUTER)
#undef FEATURE_ONCRPC_SM
#define FEATURE_ONCRPC_SM
#undef  ONCRPC_SM_PROTOCOL
#define ONCRPC_SM_PROTOCOL ONCRPC_RTR_PROTOCOL
#define svcsm_create(a,b) svcrtr_create(a,b)
#endif /* FEATURE_ONCRPC_SM_IS_ROUTER */

#define CONCAT(a, s) a s

/* Task Priority Defines */

#define TASK_PRIORITY(x)        (x)

/* Assumes that priority 0 is higher than priority 1 */
#define ONCRPC_PROXY_WAIT_PRI_ORDER        0
#define ONCRPC_PROXY_BASE_PRI_ORDER        1

#define ONCRPC_HEAP_NUM_BLOCKS 6

#endif /* rpc/types.h */
