#ifndef ONCRPC_XDR_STD_H
#define ONCRPC_XDR_STD_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
                          O N C R P C _ X D R _ S T D . H

GENERAL DESCRIPTION
  This is the header file for oncrpc XDR which uses the standard
  network byte ordering. (STD = Standard).

 Copyright (c) 2005, 2007-2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_xdr_std.h#3 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
10/12/07   ptm        Remove code that is no longer global.
07/10/07   ptm        Change d word to uint 32
04/13/05   ptm        Add protocol type to xdr_std_create and xdr_std_xdr_init.
03/16/05   clp        Added header to source file.
===========================================================================*/

extern xdr_s_type *xdr_std_create( xport_s_type   *xport,
                                   uint32          event,
                                   rpcprot_t       protocol );

extern boolean xdr_std_xdr_init( xdr_s_type       *xdr,
                                 xport_s_type     *xport,
                                 uint32            event,
                                 rpcprot_t protocol );

#endif /* ONCRPC_XDR_STD_H */
