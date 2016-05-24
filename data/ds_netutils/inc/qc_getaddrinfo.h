/*!
  @file
  qc_getaddrinfo.h

  @brief
  This file provides an API to make DNS requests against specific servers

*/

/*===========================================================================

  Copyright (c) 2012-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/
/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
12/6/2012  harouth    Initial version

******************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#ifndef  QC_GETADDRINFO_H
#define QC_GETADDRINFO_H

/*===========================================================================
  FUNCTION:  qc_getaddrinfo
===========================================================================*/
/*!
    @brief
    Retreive the numerical IPv4/IPv6 address of a given host name. The usage
    is identical to the system provided getaddrinfo() with the exception of
    two new parameters to specifiy the servers to make the query against.

    @return
    Same as system getaddrinfo()

*/
/*=========================================================================*/
int qc_getaddrinfo(
	const char *hostname,
	const char *servname,
	const struct addrinfo *hints,
	struct addrinfo **res,
	struct sockaddr_storage *server_address,
	unsigned short int num_addresses);

/*===========================================================================
  FUNCTION:  qc_freeaddrinfo
===========================================================================*/
/*!
    @brief
    Frees allocated data structures. Identical to system supplied freeaddrinfo()

    @return
    none

*/
/*=========================================================================*/
void qc_freeaddrinfo(struct addrinfo *ai);

#endif /* QC_GETADDRINFO_H */
