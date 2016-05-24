/*
 * Copyright (C) 2007-2009 Qualcomm Technologies, Inc. All rights reserved. Proprietary and Confidential.
 */

/******************************************************************************
 * aniNlFuncs.h
 *
 * This file contains generic inline functions to manipulate the WNI Netlink 
 * messages. 
 *
 * Krishna Reddy, 05/20/2002
 *
 ******************************************************************************
 * ALL RIGHTS RESERVED, Woodside Networks, Inc. 
 * No part of this file/software maybe copied or reproduced in any form without
 * the explicit permission of:
 *
 * Woodside Networks, Inc.
 * 900 Arastradero Road, Palo Alto, CA 94304, USA.
 * Tel: +1-650-475-4000
 * http://www.woodsidenet.com
 * 
 ******************************************************************************
 * (C) Copyright 2002, Woodside Networks, Inc., Palo Alto, CA 94304, USA
 ******************************************************************************/

#ifndef _ANI_NL_FUNCS_H_
#define _ANI_NL_FUNCS_H_

/* General purpose MACROS to handle the WNI Netlink msgs */
#define ANI_NL_MASK        3

static inline unsigned int aniNlAlign(unsigned int len)
{
    return ((len + ANI_NL_MASK) & ~ANI_NL_MASK);
}

static inline void *aniNlAlignBuf(void *buf)
{
    return (void *)aniNlAlign((unsigned int)buf);
}

/*
 * Does some sanity checks on the pointer to the passed in WNI MSG
 * to verify if it contains a message of valid length.
 */
static inline unsigned int aniNlOk(tAniHdr *wmsg, unsigned int len)
{
    return  ((len > 0) 
            && (wmsg->length >= sizeof(tAniHdr)) 
            && (wmsg->length <= len));
}

/* 
 * Updates the passed in buffer pointer to the next WNI Msg message in the 
 * buffer and also updates the passed in message length parameter.
 */
static inline void aniNlNext(tAniHdr *wmsg, unsigned int *msglen) 
{
    *msglen -= aniNlAlign((unsigned int)wmsg->length);
    wmsg = (tAniHdr *)((char*)wmsg + aniNlAlign((unsigned int)wmsg->length));
}

/* 
 * Determines the aligned length of the WNI MSG including the hdr
 * for a given payload of length 'len'.
 */
static inline unsigned int aniNlLen(unsigned int len) 
{
    return  (aniNlAlign(sizeof(tAniHdr)) + len);
}


/*
 * Given a WNI Msg payload of length 'len' bytes, this function will
 * calculate the total length of the WNI msg including the hdr, payload
 * and alignment bytes, if any:
 * 1. between the tAniHdr and the payload
 * 2. and at the end of the payload.
 */
static inline unsigned int aniNlAlignedMsgLen(unsigned int len)
{
    return aniNlAlign(aniNlLen(len));
}

/* 
 * Gets the address of the message payload after the tAniHdr
 */
static inline void *aniNlData(void *wmsg)
{
    return ((void*)(((char*)wmsg) + aniNlLen(0)));
}

/*
 * Determines the length of the payload excluding the size of the hdr 'tAniHdr'.
 */
static inline unsigned int aniNlPayloadLen(tAniHdr *wmsg)
{
    return ((unsigned int)(wmsg->length) - aniNlLen(0));
}

#endif  /* _ANI_NL_FUNCS_H_ */
