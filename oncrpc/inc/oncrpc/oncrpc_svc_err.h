/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ S V C _ E R R . H

GENERAL DESCRIPTION
  ONC (Open Network Computing) RPC (Remote Procedure Calls) server-side
  error handling implementation.

EXTERNALIZED FUNCTIONS
  svcerr_noproc
  svcerr_decode
  svcerr_systemerr
  svcerr_auth
  svcerr_weakauth
  svcerr_noprog
  svcerr_progvers
  svcerr_rpcvers

INITIALIZATION AND SEQUENCING REQUIREMENTS
  none.

 Copyright (c) 2005-2006, 2008 Qualcomm Technologies, Inc.  All Rights Reserved.  
 Qualcomm Technologies Proprietary and Confidential.
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

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

#ifndef _ONCRPC_SVC_ERR_H
#define _ONCRPC_SVC_ERR_H

/*===========================================================================

                             EDIT HISTORY FOR MODULE

  $Header: //linux/pkgs/proprietary/oncrpc/main/source/inc/oncrpc/oncrpc_svc_err.h#1 $ $DateTime: 2008/09/16 12:47:58 $ $Author: rruigrok $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
10/03/06    hn     Added support for locking/unlocking RPC services.
04/01/05    clp    Include header cleanup changes and move xdr routines to
                   oncrpcxdr.c.
03/22/05    hn     First revision of this file.


===========================================================================*/

/*===========================================================================

          Function Declarations

===========================================================================*/

/*===========================================================================
FUNCTION: svcerr_noproc

DESCRIPTION: Sends an error reply indicating that the RPC program does not
             support the requested procedure.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_noproc
(
  xdr_s_type  *svc
);

/*===========================================================================
FUNCTION: svcerr_decode

DESCRIPTION: Sends an error reply indicating that the server can't decode the
             arguments.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_decode
(
  xdr_s_type  *svc
);

/*===========================================================================
FUNCTION: svcerr_systemerr

DESCRIPTION: Sends an error reply indicating that the RPC call failed due to
             a system error on the server side. (e.g. memory allocation
             failures etc.)

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_systemerr
(
  xdr_s_type  *svc
);

/*===========================================================================
FUNCTION: svcerr_auth

DESCRIPTION: Sends an error reply indicating that the RPC call failed
             authentication on the server side. The reply contains a field
             that specifies why the authentication failed.

ARGUMENTS: 
   svc       The XDR transport structure for the server side of the RPC
             connection.
   why       Reason for failure of authentication.


RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_auth
(
  xdr_s_type	  *svc,
  oncrpc_auth_stat why
);

/*===========================================================================
FUNCTION: svcerr_weakauth

DESCRIPTION: Sends an error reply indicating that the RPC call failed because
             the call's authentication was too weak.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_weakauth
(
  xdr_s_type  *svc
);

/*===========================================================================
FUNCTION: svcerr_noprog

DESCRIPTION: Sends an error reply indicating that the requeseted RPC Program
             is not exported by the server side.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_noprog
(
  xdr_s_type  *svc
);

/*===========================================================================
FUNCTION: svcerr_proglocked

DESCRIPTION: Sends an error reply indicating that the requeseted RPC Program
             is locked on the server side.

ARGUMENTS: 
   The XDR transport structure for the server side of the RPC connection.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_proglocked
(
  xdr_s_type  *svc
);

/*===========================================================================
FUNCTION: svcerr_progvers

DESCRIPTION: Sends an error reply indicating that the requested version of
             the RPC Program is not supported. Part of the reply will
             specify the lowest and highest versions supported of the
             program.

ARGUMENTS: 
   svc       The XDR transport structure for the server side of the RPC
             connection.
   low_vers  Lowest version number of the program that is supported by the
             server.
   high_vers Highest version number of the program that is supported by the
             server.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_progvers
(
  xdr_s_type  *svc,
  rpcvers_t    low_vers,
  rpcvers_t    high_vers
);

/*===========================================================================
FUNCTION: svcerr_rpcvers

DESCRIPTION: Sends an error reply indicating that the version of RPC used to
             make the call on the client side is not supported by the server
             side. Part of the reply will specify the lowest and highest
             versions of RPC that are supported.

ARGUMENTS: 
   svc       The XDR transport structure for the server side of the RPC
             connection.
   low       Lowest version of RPC that is supported by the server side.
   high      Highest version of RPC that is supported by the server side.

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_rpcvers
(
  xdr_s_type  *svc,
  uint32       low,
  uint32       high
);


/*===========================================================================
FUNCTION: svcerr_default_err

DESCRIPTION: Default error handling for procedure switch case statement.

ARGUMENTS: 
   svc       The XDR transport structure for the server side of the RPC
             connection.
   rqstp     
   ver_table_func  Function version table

RETURN VALUE: 
   None

SIDE EFFECTS: 
   None
===========================================================================*/
void svcerr_default_err
(
  xdr_s_type *svc,
  struct svc_req *rqstp,
  uint32   *(*ver_table_func) (uint32 *size_entries)
);
#endif /* ! _ONCRPC_SVC_ERR_H */
