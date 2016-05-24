/* Copyright (c) 2010 by Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */
#include <stdio.h>
#include <getopt.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "comdef.h"

#define SYS_USER_PPLMN_LIST_MAX_LENGTH 85
#define SYS_PLMN_LIST_MAX_LENGTH 40

#include "voem_if.h"
#include "voem_if_rpc.h"
#include "stringl.h"


static struct option long_options[] = {
    {"wve", required_argument, 0, 'w'},
    {"agc", required_argument, 0, 'g'},
    {"avc", required_argument, 0, 'v'},
    {"rve", required_argument, 0, 'r'},
    {"ec", required_argument, 0, 'e'},
    {"pcmfilter", required_argument,0, 'p'},
    {"wnr", required_argument, 0, 'n'},
    {"fens", required_argument, 0, 'f'},
    {"slowtalk", required_argument, 0, 's'},
    {"pbe", required_argument, 0, 'b'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0},
};
struct voem_if_config {
        int    wve_enable;
        int    agc_enable;
        int    avc_enable;
        int    rve_enable;
        int    ec_enable;
        int    pcmfilter_enable;
	int    wnr_enable;
	int    fens_enable;
	int    slowtalk_enable;
	int    pbe_enable;
};

static struct voem_if_config voemConfig;

void usage(void)
{
	printf("Perform Voice Post Processing Tests\n\n");
#ifndef AUDIO7x27
        printf("-w, --wve  0/1     	Test Wide Voice Extension, disable/enable\n");
	printf("-r, --rve  0/1          Test Receive Voice Enhancement, disable/enable\n");
	printf("-s, --slowtalk 0/1	Test Ezhearing(Slowtalk), disable/enable\n");
#endif
	printf("-g, --agc  0/1		Test Automatic Gain Control, disable/enable\n");
	printf("-v, --avc  0/1		Test Automatic Volume Control, disable/enable\n");
	printf("-e, --ec   0/1          Test Echo Cancellation, disable/enable\n");
	printf("-p, --pcmfilter 0/1     Test PCM Filter, dsable/enable\n");
	printf("-n, --wnr  0/1          Test Wind Noise Rejection, disable/enable\n");
	printf("-f, --fens 0/1		Test Far End Noise Suppress, disable/enable\n");
#ifdef AUDIO7x27A
	printf("-b, --pbe 0/1		Test Psychoacoustic Bass Enhancement, disable/enable\n");
#endif
}


static struct voem_if_config voemConfig;

int main(int argc, char **argv)
{
	int cmd;
	int option_index = 0;
        int exit_flag = 0;
	char *cvalue = NULL;
	int value =0;

	memset(&voemConfig, 0, sizeof(struct voem_if_config));

	/* init oncrpc and start oncrpc task */
	oncrpc_init();
	oncrpc_task_start();

	voem_ifcb_app_init();

	while (1) {
#ifdef AUDIO7x27
		cmd = getopt_long( argc, argv, "g:v:e:p:n:f:h", long_options, &option_index);
#elif defined(AUDIO7x27A)
		cmd = getopt_long( argc, argv, "w:g:v:r:e:p:n:f:s:b:h", long_options, &option_index);
#else
		cmd = getopt_long( argc, argv, "w:g:v:r:e:p:n:f:s:h", long_options, &option_index);
#endif
		if (cmd == -1)
                        break;
		switch( cmd ) {
			case 'w':
				cvalue = optarg;
			 	value = atoi(cvalue);
				if (value >=1)
					voemConfig.wve_enable = VOEM_ENABLE;
				else
					voemConfig.wve_enable = VOEM_DISABLE;
				/* enable/disable WVE */
				printf(" configure WVE, value=%d sent to modem\n",voemConfig.wve_enable);
				voem_pp_control(VOEM_PPBLOCK_WVE, voemConfig.wve_enable);
				break;
			case 'g':
				cvalue = optarg;
                                value = atoi(cvalue);
				if (value >=1)
					voemConfig.agc_enable = VOEM_ENABLE;
				else
                                        voemConfig.agc_enable = VOEM_DISABLE;
                                /* enable/disable AGC */
				printf(" configure AGC, value=%d sent to modem\n",voemConfig.agc_enable);
                                voem_pp_control(VOEM_PPBLOCK_AGC, voemConfig.agc_enable);
				break;
			case 'v':
				cvalue = optarg;
                                value = atoi(cvalue);
				if (value >=1)
					voemConfig.avc_enable = VOEM_ENABLE;
				else
                                        voemConfig.avc_enable = VOEM_DISABLE;
                                /* enable/disable AVC */
                                printf(" configure AVC, value=%d sent to modem\n",voemConfig.avc_enable);
                                voem_pp_control(VOEM_PPBLOCK_AVC, voemConfig.avc_enable);
				break;
			case 'r':
				cvalue = optarg;
                                value = atoi(cvalue);
				if (value >=1)
					voemConfig.rve_enable = VOEM_ENABLE;
				else
                                        voemConfig.rve_enable = VOEM_DISABLE;
                                /* enable/disable RVE */
                                printf(" configure RVE, value=%d sent to modem\n",voemConfig.rve_enable);
                                voem_pp_control(VOEM_PPBLOCK_RVE, voemConfig.rve_enable);
				break;
			case 'e':
				cvalue = optarg;
                                value = atoi(cvalue);
				if (value >=1)
					voemConfig.ec_enable = VOEM_ENABLE;
				else
                                        voemConfig.ec_enable = VOEM_DISABLE;
                                /* enable/disable EC */
				printf(" configure EC, value=%d sent to modem\n",voemConfig.ec_enable);
                                voem_pp_control(VOEM_PPBLOCK_EC, voemConfig.ec_enable);
				break;
			case 'h':
                                usage();
                                break;
			case 'p':
				cvalue = optarg;
                                value = atoi(cvalue);
				if (value >=1)
					voemConfig.pcmfilter_enable = VOEM_ENABLE;
				else
                                        voemConfig.pcmfilter_enable = VOEM_DISABLE;
                                /* enable/disable PCMFILTER */
			      	printf(" configure PCMFILTER, value=%d sent to modem\n",voemConfig.pcmfilter_enable);
                                voem_pp_control(VOEM_PPBLOCK_PCMFILTER, voemConfig.pcmfilter_enable);
				break;
			case 'n':
				cvalue = optarg;
                                value = atoi(cvalue);
                                if (value >=1)
                                        voemConfig.wnr_enable = VOEM_ENABLE;
                                else
                                        voemConfig.wnr_enable = VOEM_DISABLE;
                                /* enable/disable WNR */
				printf(" configure WNR, value=%d sent to modem\n",voemConfig.wnr_enable);
                                voem_pp_control(VOEM_PPBLOCK_WNR, voemConfig.wnr_enable);
                                break;
			case 'f':
				cvalue = optarg;
                                value = atoi(cvalue);
                                if (value >=1)
                                        voemConfig.fens_enable = VOEM_ENABLE;
                                else
                                        voemConfig.fens_enable = VOEM_DISABLE;
                                /* enable/disable FENS */
				printf(" configure FENS, value=%d sent to modem\n",voemConfig.fens_enable);
                                voem_pp_control(VOEM_PPBLOCK_FENS, voemConfig.fens_enable);
                                break;
			case 's':
				cvalue = optarg;
                                value = atoi(cvalue);
                                if (value >=1)
                                        voemConfig.slowtalk_enable = VOEM_ENABLE;
                                else
                                        voemConfig.slowtalk_enable = VOEM_DISABLE;
                                /* enable/disable SLOWTALK */
				printf(" configure SLOWTALK, value=%d sent to modem\n",voemConfig.slowtalk_enable);
                                voem_pp_control(VOEM_PPBLOCK_SLOWTALK, voemConfig.slowtalk_enable);
                                break;
			case 'b':
				cvalue = optarg;
                                value = atoi(cvalue);
                                if (value >=1)
                                        voemConfig.pbe_enable = VOEM_ENABLE;
                                else
                                        voemConfig.pbe_enable = VOEM_DISABLE;
                                /* enable/disable PBE */
				printf(" configure PBE, value=%d sent to modem\n",voemConfig.pbe_enable);
                                voem_pp_control(VOEM_PPBLOCK_PBE, voemConfig.pbe_enable);
                                break;
                        default:
                                usage();
                }//switch
        }//while

	sleep(5);
	oncrpc_task_stop();
	return 0;

}
