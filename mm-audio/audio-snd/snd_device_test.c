/* Copyright (c) 2008 by Qualcomm Technologies, Inc.  All Rights Reserved. 
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

#include "snd.h"
#include "snd_rpc.h"
#include "stringl.h"


static struct option long_options[] = {
    {"device", required_argument, 0, 'd'},
    {"method",   required_argument, 0, 'm'},
    {"volume",   required_argument, 0, 'v'},
    {"mute-speaker", no_argument, 0, 'i'},
    {"mute-mic", no_argument, 0, 'o'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0},
};

/* This array must parallel long_options[] */
static const char *descriptions[] = {
    "sound output device (number); defaults to handset device",
    "sound method: one of \"voice\", \"key_beep\", \"message\", \"ring\", \"midi\", \"aux\"",
    "volume (number from 1-7); if specified, --method must also be specified",
    "mute speaker (by default unmuted)",
    "mute microphone (by default unmuted)",
    "this help screen"
};


static int get_method(const char *method) {

#define IS_METHOD(name, id) if (!strcmp(method, name)) return SND_METHOD_##id;

    IS_METHOD("voice", VOICE);
    IS_METHOD("key_beep", KEY_BEEP);
    IS_METHOD("message", MESSAGE);
    IS_METHOD("ring", RING);
    IS_METHOD("midi", MIDI);
    IS_METHOD("aux", AUX);

//#undef IS_METHOD

    //FAILIF(1, "Unknown method type!\n");
    return 0;
}

static int get_device(int device) {
        if (device < 0) {
            printf("using default SURF device (%d)\n", SND_DEVICE_DEFAULT);
            return SND_DEVICE_DEFAULT; 
        }
        
        printf("using device %d\n", device);
        return device;
}

void usage(char *exename)
{
        printf("Usage: %s <Operations>\r\n"\
               " \r\n" \
	       "  \t%s [--device|-d <device #>] --method|-m <method> --volume|-v <volume> \n\n"\
	       "  \t%s [--device|-d <device #>] [--mute-speaker|-i] [--mute-mic|-o]\n\n",	
               exename,exename,exename);

      		 fprintf(stdout, "options:\n");
   		 struct option *opt = long_options;
    		const char **desc = descriptions;
   		 while (opt->name) {
        	 fprintf(stdout, "\t-%c/--%s%s: %s\n",
                opt->val,
                opt->name,
                (opt->has_arg ? " (argument)" : ""),
                *desc);
        opt++;
        desc++;
    }
     printf("\n\n\n--- Device List ---\r\n"\
               " \r\n" \
               "  0. Handset \n"\
               "  1. HFK \n"\
               "  2. Mono Headset \n"\
               "  3. Stereo Headset \n"\
               "  4. AHFK           \n"\
               "  5. Stereo DAC     \n"\
               "  6. Speaker phone  \n"\
               "  7. TTY HFK \n"\
               "  8. TTY Headset \n"\
               "  9. TTY VCO  \n"\
               "  10. TTY HCO  \n"\
               "  11. BT INTERCOM \n"\
               "  12. BT Headset  \n"\
               "                  \n\n");
               
        exit(1);
}


int main(int argc, char **argv)
{
	int command;
	char *cvalue = NULL;
	int devIndex = 0; /* default: handset device */
	char *method = "voice"; /* default: voice */
	int vol = 3;   /* default: 3  */
	int mute_mic = 0; /* default: volume not provided */
	int mute_speaker = 0; /* default: do not mute */
	int methIndex;
	int agc_value = 0;
	int avc_value = 0;
                
	int testMeth = SND_METHOD_VOICE;	
	
	
	/* init oncrpc and start oncrpc task */
	oncrpc_init();
	oncrpc_task_start();

   	sndcb_app_init();


        
	while((command = getopt_long( argc, argv, "hiod:m:v:a:c:", long_options, NULL)) != -1) {
		switch( command ) {
			case 'd':
				cvalue = optarg;
				devIndex = atoi(cvalue);
				if(devIndex > 25){
					printf(" pls input the device number (<25) again!\n");
					exit(1);
				} 
            			break;
        		case 'v':
            			cvalue = optarg;
				vol = atoi(cvalue);
				if((vol <1) || (vol > 7)){
					printf(" pls input the volume (1 to 7) again!\n"); 
					exit(1);
				}
            			break;
			case 'm':
				method = strdup(optarg);
				printf("method:%s\n",method);
				break;
       			case 'i': 
				mute_speaker = 1; 
				break;
        		case 'o': 
				mute_mic = 1; 
				break;  
			case 'h':
                                usage(argv[0]);
                                break;
			case 'a':
				cvalue = optarg;
				agc_value = atoi(cvalue);
				break;
			case 'c':
				cvalue = optarg;
				avc_value =atoi(cvalue);
				break;
                        default:
                                //fprintf(stderr, "Invalid argument: %c\n", command);
                                usage(argv[0]);
                }//switch
        }//while
       printf("snd settings:\n"
      		"\tdevice: %d\n"
      		"\tmethod: %s\n"
      		"\tvolume: %d\n"
      		"\tmute mic: %s\n"
		"\tmute speaker: %s\n"
		"\tagc: %d\n"
		"\tavc: %d\n",
      		devIndex, 
		method, 
		vol, 
      		(mute_mic ? "yes" : "no"), 
      		(mute_speaker ? "yes" : "no"),
		agc_value,
		avc_value);
		//(agc_value ? "ON": "OFF"));
     
        /* call snd_set_device */
        devIndex = get_device(devIndex);
        methIndex = get_method(method);
	snd_set_device(devIndex, mute_speaker, mute_mic, NULL, NULL);	

        /*  call snd_set_volume */
	snd_set_volume(devIndex, methIndex, vol, NULL, NULL);

	/* Call AGC */
	snd_agc_ctl(agc_value,NULL, NULL);

	/* Call AVC */
	snd_avc_ctl(avc_value, NULL, NULL);

	/* Prevent the terminate the apps and reset the agc avc value */
	for (;;);

}
