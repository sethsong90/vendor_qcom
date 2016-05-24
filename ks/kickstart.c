/*
 * Kickstart: A utility for uploading MSM images using the "DMSS-DL"
 * and Sahara protocols
 *
 * Copyright (C) 2012 Qualcomm Technologies, Inc. All rights reserved.
 *                    Qualcomm Technologies Proprietary/GTDR
 *
 * All data and information contained in or disclosed by this document is
 * confidential and proprietary information of Qualcomm Technologies, Inc. and all
 * rights therein are expressly reserved.  By accepting this material the
 * recipient agrees that this material and the information contained therein
 * is held in confidence and in trust and will not be used, copied, reproduced
 * in whole or in part, nor its contents revealed in any manner to others
 * without the express written permission of Qualcomm Technologies, Inc.
 *
 *
 *  kickstart.c : Tool for uploading MSM images using "DMSS-DL" and Sahara protocols.
 * ==========================================================================================
 *   $Header: //source/qcom/qct/core/storage/tools/kickstart/kickstart.c#10 $
 *   $DateTime: 2010/09/28 12:17:11 $
 *   $Author: niting $
 *
 *  Edit History:
 *  YYYY-MM-DD		who		why
 *  -----------------------------------------------------------------------------
 *  2010-09-28       ng      Added command mode support
 *  2010-10-18       ab      Added memory debug mode support
 *
 *  Copyright 2012 by Qualcomm Technologies, Inc.  All Rights Reserved.
 *
 *==========================================================================================
 */

#include <getopt.h>
#include "dload_protocol.h"
#include "sahara_protocol.h"
#include "kickstart_log.h"

#define NUM_RETRIES (10)

/* global variable for enabling  or disabling verbose logging */
int verbose 		= 0;
int noclosedevnode  = 0;
int using_uart      = 0;
int using_sdio      = 0;
int old_pbl         = 0;
int rx_timeout      = 15;
int using_tty       = 0;

unsigned int malloc_count = 0;
unsigned int free_count = 0;
char upload_prefix_string[PREFIX_STRING_SIZE] = "";

/* print usage information */
void print_usage (FILE *stream)
{
    fprintf (stream, "\nUsage: \n");
    fprintf (stream,
             " -h                     --help                      Display this usage information\n"
             " -n                     --noclosedevnode            Don't close port since it's vanished\n"
             " -v                     --verbose                   Print verbose messages\n"
             " -m                     --memdump                   Force memory debug mode\n"
             " -i                     --image                     Force sahara image transfer mode\n"
             " -c                     --command                   Force command mode\n"
             " -p                     --port                      Device name for USB driver\n"
             " -o                     --old                       Use old PBL boot structure for Dload\n"
             " -r                     --ramdumpimage              Image ID which must be transferred before forcing memory dump\n"
             " -w                     --where                     Path to store files received from target\n"
             " -t                     --timeout                   Port timeout (in seconds) to use for Sahara transfer\n"
             " -d <img_id:file_name>  --dload  <img_id:file_name> Use DMSS download protocol for loading the image\n"
             " -s <img_id:file_name>  --sahara <img_id:file_name> Use Sahara download protocol for transferring the image\n");
    fprintf (stream,
             "\n\nExample usage: \n"
             "\tsudo kickstart -p /dev/ttyUSB0 -d 10:dbl.mbn \n"
             "\tsudo kickstart -p /dev/ttyUSB0 -s 11:osbl.mbn -s 2:amss.mbn\n");
    return;
}

int main (int argc, char *argv[])
{

    int    option;                                            /*holds the option from getopt_long*/
    int    enable_dload_transfer=0, enable_sahara_transfer=0; /*flags for checking dload or sahara transfer*/
    char  *port_name = NULL;                                  /*device file name */
	char *PathToSaveFiles = NULL;         /* where the memory dump files go */
    struct com_state *m_comm = NULL;                          /*holds com structure instance for device io operations*/
    const char *const short_options = "hp:vnd:s:w:micor:t:lg:";             /*possible cmd line short options */
    const struct option long_options[] = {                    /*possible cmd line long options*/
        { "help",    		0, NULL, 'h' },
        { "port",    		1, NULL, 'p' },
        { "verbose", 		0, NULL, 'v' },
        { "noclosedevnode", 0, NULL, 'n' },
        { "command", 		0, NULL, 'c' },
        { "memdump", 		0, NULL, 'm' },
        { "image",  		0, NULL, 'i' },
        { "dload",   		1, NULL, 'd' },
        { "sahara",  		1, NULL, 's' },
        { "prefix",         1, NULL, 'g' },
        { "where",  		1, NULL, 'w' },
        { "old", 	    	0, NULL, 'o' },
        { "ramdumpimage",	1, NULL, 'r' },
        { "timeout",	    1, NULL, 't' },
        { "loop",           0, NULL, 'l' },
        { NULL,      		0, NULL,  0  }
    };

    enum boot_sahara_mode sahara_mode = SAHARA_MODE_LAST;
    int memdebugImage = -1;
    int sahara_loop = 0;

    /* check argument count, print the usage help and quit */
    if (argc < 2) {
        print_usage (stderr);
        return EFAILED;
    }

    /*initialize list that will contain <id, file_name> mapping*/
    if (SUCCESS != init_input_file_list()) {
       dbg (ERROR, "Failed to initialize file list");
       return EFAILED;
    }

    /* parse command-line options */
    do {
        option = getopt_long (argc, argv, short_options, long_options, NULL);

        switch (option) {
        case -1:                /* no more option arguments */
            break;

        case 'h':               /* -h or --help */
            print_usage (stdout);
            destroy_input_file_list();
            return 0;

        case 'p':               /* Get the port string name */
            port_name = optarg;
			dbg (INFO,"\nport_name=%s\n",port_name);
            break;

        case 'd':               /* -d or --dload */
            /*add the input to <id, file_name> list*/
            if (SUCCESS != push_input_to_file_list(optarg)) {
               dbg (ERROR, "Failed to add file to file list");
               destroy_input_file_list();
               return EFAILED;
            }
            enable_dload_transfer=1;
            break;

        case 's':               /* -s or --sahara */
            /*add the input to <id,file_name> list*/
            if (SUCCESS != push_input_to_file_list(optarg)) {
               dbg (ERROR, "Failed to add file to file list");
               destroy_input_file_list();
               return EFAILED;
            }

			//dbg (INFO,"\npush_input_to_file_list=%s\n\n",optarg);

            enable_sahara_transfer=1;
            break;

        case 'i':               /* -i or --image */
            sahara_mode 	= SAHARA_MODE_IMAGE_TX_PENDING;
            break;

        case 'v':               /* -v or --verbose */
            verbose = 1;
            break;

        case 'n':               /* -n or --noclosedevnode */
            noclosedevnode = 1;
            break;

        case 'm':               /* -m or --memdump */
            sahara_mode 	= SAHARA_MODE_MEMORY_DEBUG;
            enable_sahara_transfer = 1;
            break;

        case 'o':               /* -o or --old */
            old_pbl 	= 1;
            break;

        case 'r':               /* -r or --ramdumpimage */
            memdebugImage 	= atoi(optarg);
            break;

        case 'g':               /* -g or --prefix */
            if (strlcpy(upload_prefix_string, optarg, sizeof(upload_prefix_string)) >= sizeof(upload_prefix_string)) {
                dbg(ERROR, "prefix string length %d is too long (%d bytes max), NOT ADDED!!", strlen(optarg) + 1, sizeof(upload_prefix_string));
                destroy_input_file_list();
                return EFAILED;
            }
            break;

        case 't':               /* -t or --timeout */
            rx_timeout 	= atoi(optarg);
            break;

        case 'w':               /* -w or --where - path for memory dump */
			PathToSaveFiles = optarg;

            dbg (INFO,"\nParsing 'where to save memorydump' options");

			if( PathToSaveFiles[strlen(PathToSaveFiles)-1]=='/' )
				dbg (INFO,"\nPathToSaveFiles=%s\n",PathToSaveFiles);
			else
			{
				dbg(ERROR, "\nERROR: Path for memory dump must end with a \"/\"");
				dbg(ERROR, "\n       should be \"-w /path/to/save/memorydump/\"\n\n");
				PathToSaveFiles=NULL;
				exit(1);
			}

            break;

        case 'c':               /* -c or --command */
            sahara_mode = SAHARA_MODE_COMMAND;
            break;

        case 'l':               /* -l or --loop */
            sahara_loop = 1;
            break;

        default:                /* unknown option. */
            dbg (ERROR, "unrecognized option");
            print_usage (stderr);
            destroy_input_file_list();
            return EFAILED;
        }
    } while (option != -1);
    
    /*check if the device name input is given or not*/
    if (NULL == port_name) {
        dbg (ERROR, "Port device name not specified; use -p option.");
        destroy_input_file_list();
        return EFAILED;
    }

    if (strstr(port_name, "/dev/ttyS") || strstr(port_name, "/dev/ttyHSL"))
        using_uart = 1;
    if (strstr(port_name, "sdio"))
        using_sdio = 1;
    using_tty = (strspn(port_name, "/dev/tty") == strlen("/dev/tty"));

    if(enable_dload_transfer) {
        char* dload_filename;
        if (old_pbl)
            dload_filename = get_file_from_file_list(DBL_IMG_OLD);
        else
            dload_filename = get_file_from_file_list(DBL_IMG);

        if( dload_filename) {
			int i = 0;
            dbg (INFO, "dload_filename: %s", dload_filename);

            /* connect and configure the device */
			if(NULL == (m_comm = connect_and_configure_device (port_name, DLOAD_PROTOCOL)))
			{
                dbg(ERROR, "Configuring the device failed - DLOAD\n\n");
                return EFAILED;
			}

            /* download the input image in DLOAD mode */
            dbg (INFO, "Upload the input image \"%s\" in DLOAD mode", dload_filename);
            if (SUCCESS != dload_image (m_comm, dload_filename, noclosedevnode)) {
                dbg (ERROR, "Uploading the image in DLOAD mode failed");
                return EFAILED;
            }
            return SUCCESS;
        }
        else
        {
            dbg (ERROR, "Input file name not specified");
            destroy_input_file_list();
            return EFAILED;
        }
        return SUCCESS;
    }

    if(enable_sahara_transfer)
    {
		int i;

        /* connect and configure the device */
        for(i=0;i<NUM_RETRIES;i++)
		{
        	if(NULL == (m_comm = connect_and_configure_device (port_name, SAHARA_PROTOCOL)))
		        dbg (INFO,"Configuring the device failed - SAHARA - %i of %d\n\n",i, NUM_RETRIES);
			else
				break;
        }

		if(i==NUM_RETRIES)
		{
            dbg(ERROR, "Configuring the device failed - SAHARA - %d attempts\n\n", NUM_RETRIES);
            deinit_com_port (m_comm, noclosedevnode);
            return EFAILED;
		}

        do {
            if (SUCCESS != start_sahara_based_transfer (m_comm, sahara_mode, PathToSaveFiles, memdebugImage))
            {
               dbg (ERROR, "Uploading  Image using Sahara protocol failed");
               use_wakelock(WAKELOCK_RELEASE);
               deinit_com_port (m_comm, noclosedevnode);
               return EFAILED;
            }
            else
               dbg (STATUS, "Sahara protocol completed");
        } while (sahara_loop);
    }
    destroy_input_file_list();
    deinit_com_port (m_comm, noclosedevnode);
    return SUCCESS;
}

