/******************************************************************************

  @file    dsdnsutil.c
  @brief   CLI for libdsnetutils - DNS

  DESCRIPTION
  Provides a command line interface on the target to directly excecise the
  modified DNS resolver in ds_netutils

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <qc_getaddrinfo.h>
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>

#include "dsdnsutil.h"

int main (int argc, char **argv)
{
	cliopts_t cloptions;
	int error = 0;

	/* Reset the command line options to defaults */
	memset(&cloptions, 0, sizeof (cliopts_t));
	cloptions.dns_server_proto = 4;

	/* Parse the command line options */
	parse_cli_options(argc, argv, &cloptions);
	if ((error = validate_inputs(&cloptions)))
	{
		print_usage();
	}
	else
	{
		error = run_dns_query(&cloptions);
	}

	return error;
}

/*===========================================================================
  FUNCTION  print_usage
===========================================================================*/
/*!
@brief
 Helper function to print the CLI usage help.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static void print_usage(void)
{
	printf("%s\n", usage);
}

/*===========================================================================
  FUNCTION  parse_cli_options
===========================================================================*/
/*!
@brief
 Helper function to parse the CLI arguments into a cliopts_t structure. Takes
 the unmodified argc and argv from main() as well as a pointer to the options
 structure to populate.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static void parse_cli_options(int argc, char **argv, cliopts_t *options)
{
	//How come this can be a char in GNU/Linux but gives error on LA?
	//error: comparison is always true due to limited range of data type [-Werror=type-limits]
	int c;

	while ( (c = getopt(argc, argv, "vdp:s:q:h?")) != -1)
	{
		switch (c)
		{
		case 'q':
			options->query_host = optarg;
			break;

	        case 'v':
	            	options->verbose = true;
        	    break;

	        case 'd':
	            	options->debug = true;
        	    break;

	        case 's':
	            	options->dns_server = optarg;
        	    break;

       	        case 'p':
	            	options->dns_server_proto = atoi(optarg);
        	    break;

	        default:
	            printf("Unhandled option: %c\n",c);
	        case 'h':
	        case '?':
	            print_usage();
	            exit (0);
	            break;
        	}
    	}
}

/*===========================================================================
  FUNCTION  validate_inputs
===========================================================================*/
/*!
@brief
 Validates inputs. Checks for missing required arguments and other inputs
 out of range.

@return
  integer:
	0: Success
	1: Invalid DNS server address format
	2: Missing/Null DNS server address
	3: Missing/Null question host name
*/
/*=========================================================================*/
static int  validate_inputs(cliopts_t *options)
{
	int error = 0;
	if (options->dns_server_proto != 4 && options->dns_server_proto != 6)
	{
		printf("DNS server address format must be 4 or 6\n");
		error = 1;
	}
	else if (!options->dns_server)
	{
		printf("DNS server must be specified\n");
		error = 2;
	}
	else if (!options->query_host)
	{
		printf("DNS question must be specified\n");
		error = 3;
	}
	return error;
}

/*===========================================================================
  FUNCTION  run_dns_query
===========================================================================*/
/*!
@brief
 Runs the actual DNS query and prints the results

@return
  integer:
	result of qc_getaddrinfo
*/
/*=========================================================================*/
static int  run_dns_query(cliopts_t *options)
{
	struct addrinfo *results;
	struct sockaddr_storage dns_addr;
	ntopbuf_t buffer;

	int error = 0;

	memset(&dns_addr, 0 , sizeof(struct sockaddr_storage));

	/* Parse the DNS server address (either v4 or v6) */
	switch(options->dns_server_proto)
	{
	case 4:
		if (options->debug) printf ("DNS server AF: AF_INET\n");
		dns_addr.ss_family = AF_INET;
		error = inet_pton(dns_addr.ss_family, options->dns_server,
				&( (struct sockaddr_in *) &dns_addr)->sin_addr);
		((struct sockaddr_in *) &dns_addr)->sin_port=htons(53);
		break;
	case 6:
		if (options->debug) printf ("DNS server AF: AF_INET6\n");
		dns_addr.ss_family = AF_INET6;
		error = inet_pton(dns_addr.ss_family, options->dns_server,
				&( (struct sockaddr_in6 *) &dns_addr)->sin6_addr);
		((struct sockaddr_in6 *) &dns_addr)->sin6_port=htons(53);
		break;
	}

	if (error != 1)
	{
		printf("Unable to process specified DNS server address\n");
		return error;
	}

	/* Do the actual getaddrinfo() call. We will not specify any hints*/
	error = qc_getaddrinfo(options->query_host, NULL, NULL, &results, &dns_addr, 1);

	/* Return immediately if there was an error*/
	if (error)
	{
		printf("DNS query failed. Error was: ");
		switch (error)
		{
		case EAI_FAIL:
			printf("EAI_FAIL\n");
			break;

		case EAI_NONAME:
			printf("EAI_NONAME\n");
			break;

		case EAI_FAMILY:
			printf("EAI_FAMILY\n");
			break;

		}
		return error;
	}


	/* Print out the addrinfo structure */
	while (results)
	{
		if (options->verbose || options->debug)
			printf("--- --- --- --- --- --- --- --- ---\n");

		if (options->debug) printf("Got results at %p\n", results);

		if (options->verbose)
		{
			printf("ai_flags:     0x%08X\n", results->ai_flags);
			printf("ai_socktype:  0x%08X\n", results->ai_socktype);
			printf("ai_protocol:  0x%08X\n", results->ai_protocol);
			printf("ai_addrlen:   %d bytes\n", results->ai_addrlen);
			if (results->ai_canonname)
				printf("ai_canonname: %s\n", results->ai_canonname);
			printf("ai_next:      %p\n", results->ai_next);
		}

		switch (results->ai_family)
		{
		case AF_INET:
			if (options->verbose || options->debug)
				printf("Result family: AF_INET\n");
			if (options->debug) printf("Parsing (ntop ) INET\n");
			if (!inet_ntop(AF_INET, &(((struct sockaddr_in *) results->ai_addr)->sin_addr), buffer.ipv4, INET_ADDRSTRLEN))
			{
				if (options->verbose || options->debug)
					printf("Failed to parse this record\n");
				goto next;
			}
			else
			{
				printf("Address: %s\n", buffer.ipv4);
			}
			break;

		case AF_INET6:
			if (options->verbose || options->debug)
				printf("Result family: AF_INET6\n");
			if (options->debug) printf("Parsing (ntop ) INET\n");
			if (!inet_ntop(AF_INET6, &(((struct sockaddr_in6 *) results->ai_addr)->sin6_addr), buffer.ipv6, INET6_ADDRSTRLEN))
			{
				if (options->verbose || options->debug)
					printf("Failed to parse this record\n");
				goto next;
			}
			else
			{
				printf("Address: %s\n", buffer.ipv6);
			}
			break;
		default:
			printf("Unknown address family: %04X\n", results->ai_family & 0xFFFF );
			break;
		}
next:
		results = results->ai_next;
	}

	qc_freeaddrinfo(results);
	return error;
}



