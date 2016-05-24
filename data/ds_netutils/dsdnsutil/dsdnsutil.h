/******************************************************************************

  @file    dsdnsutil.h
  @brief   Internal definitions for dsdnsutil

  DESCRIPTION
  Provides internal structures for dsdnsutil.c. This should not be used by
  anything else.

******************************************************************************/
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
#include <arpa/inet.h>

#ifndef DSDNSUTIL_H
#define DSDNSUTIL_H

struct cliopts {
	int dns_server_proto;
	char *dns_server;
	char *query_host;
	bool verbose;
	bool debug;
};

typedef struct cliopts cliopts_t;

union ntopbuf {
	char ipv4 [INET_ADDRSTRLEN];
	char ipv6 [INET6_ADDRSTRLEN];
};

typedef union ntopbuf ntopbuf_t;

const char *usage = 	"Data Services DNS Utility\n"
			"(c) 2012-2013 Qualcomm Technologies Inc. All rights reserved\n"
			"\n"
			"Usage:\n"
			"\t-p - Address format of DNS server. Choose from 4 or 6\n"
			"\t-s - DNS server to make query against\n"
			"\t-q - Hostname to query\n"
			"\t-v - Verbose output\n"
			"\t-d - Debug output\n"
			"\t-h - Show this message\n";

/* Internal routines. Should not be used outside of dsdnsutil */
static void print_usage(void);
static void parse_cli_options(int, char **, cliopts_t *);
static int  validate_inputs(cliopts_t *);
static int  run_dns_query(cliopts_t *);

#endif /* DSDNSUTIL_H */
