/*
 *
 * Copyright (C) 2009 Qualcomm Technologies, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <inttypes.h>
#include <linux/msm_audio.h>

#include "msm8k_atu.h"
#include "msm8k_atui.h"

#define CAD_HW_DEVICE_ID_DEFAULT_RX		0x0F
#define NSEC_PER_SEC            1000000000L


/* ATU internal states */
enum atu_state{
	ATU_FREE      = 0,
	ATU_START_REQ = 1,
	ATU_PLAYING   = 2,
	ATU_STOP_REQ  = 3 
};

/* ATU state variable */
struct atu {
	/* Non volatile data */
	enum atu_state			tone_state;
	bool 				exit_thread;
	unsigned int			device;
	unsigned short			rx_vol;
	unsigned short			tx_vol;

	/* OS Specific */
	pthread_cond_t			cond;
	pthread_mutex_t			mutex;
	pthread_t			thread;

	/* Valid only during active session */
	int				dtmf_handle;
	unsigned int			sound_id;
	enum atu_path			path;
	union atu_sound_type		*sound_type;
	atu_cb_func_ptr_type		client_cb_ptr;
	const void			*client_data;  

	/* Parser info							*/
	unsigned int loop_cnt;	/* Loop count for ATU_LOOP_BACK2	*/
	unsigned short label_index;	/* Array index of label tone		*/
	unsigned int label_cnt;	/* Loop count for label			*/
	unsigned int repeat_cnt;	/* Repeat count				*/
	unsigned int scaling;	/* Scale level to be used		*/
	unsigned int time_remain_ms;/* Count of ms before tone is timed out	*/
	unsigned short tone_index;	/* Tone_ptr array index of current tone	*/

	/* Tone playback info						*/
	unsigned short			freq_hi;
	unsigned short			freq_lo;
	unsigned int			playback_time;
	unsigned short			playback_vol;  
};



static struct atu		atu;
static struct msm_gen_dtmf	dtmf;

void atu_play_tone();
void atu_setup_dtmf();
void atu_stop_tone();
void atu_get_sound_info(unsigned short index, unsigned short *tone,
	union atu_tone_data_type *tone_data);
void atu_parse_sound(unsigned int ms_elapsed_time);
void atu_setup_tone();
void worker_thread(void *data);

void atu_play_tone()
{
	static bool tone_playing = false;
	struct msm_gen_dtmf dtmf;


	if (!((tone_playing == false) && (atu.playback_time == 0))) {
		dtmf.dtmf_hi   = atu.freq_hi;
		dtmf.dtmf_low  = atu.freq_lo;
		dtmf.rx_gain   = atu.rx_vol;
		dtmf.tx_gain   = atu.tx_vol;
		dtmf.mixing    = 1;
		dtmf.duration  = (unsigned short)atu.playback_time;
		dtmf.path      = atu.path;
		tone_playing   = (0 == atu.playback_time)? false : true;

		if (atu.dtmf_handle) {
			if (ioctl(atu.dtmf_handle, AUDIO_PLAY_DTMF, &dtmf))
				perror("ATU: Audio Play DTMF failed\n");
		}
	}
}

void atu_setup_dtmf()
{
	atu.dtmf_handle = open("/dev/msm_dtmf", O_RDWR);
	if (atu.dtmf_handle < 0) {
		perror("ATU: cannot open audio device");
		return;
	}
	DEBUG_PRINT(" call ioctl(AUDIO_START)\n");
	if (ioctl(atu.dtmf_handle, AUDIO_START, atu.device))
		perror("ATU: Audio Play DTMF failed\n");
}


void atu_stop_tone()
{
	atu.freq_hi       = 0;
	atu.freq_lo       = 0;
	atu.playback_time = 0;
	atu_play_tone();
	if (atu.dtmf_handle) {
		close(atu.dtmf_handle);
		atu.dtmf_handle = 0;
	}

	atu.tone_state = ATU_FREE;
	DEBUG_PRINT(" Stop tone\n");
	if (atu.client_cb_ptr != NULL)
		atu.client_cb_ptr(ATU_STOP_DONE, atu.client_data);
}

void atu_get_sound_info(unsigned short index, unsigned short *tone,
	union atu_tone_data_type *tone_data)
{
	struct atu_compact_tone_type  compact_tone;
	struct atu_flexible_tone_type flexible_tone;
	const union atu_sound_type    *sound;

	sound = atu.sound_type;
	switch( sound->type ) {
	case ATU_COMPACT_SOUND:
		compact_tone = (((struct atu_compact_sound_type *)sound)
			->tone_array)[index];
		*tone        = compact_tone.tone;
		*tone_data   = compact_tone.param;
		break;

	case ATU_FLEXIBLE_SOUND:
		flexible_tone = (((struct atu_flexible_sound_type *)sound)
			->tone_array)[index];
		*tone         = flexible_tone.tone;
		*tone_data    = flexible_tone.param;
		break;

	case ATU_INVALID_SOUND:
        default:
                break;
	}
}

void atu_parse_sound(unsigned int ms_elapsed_time)
{
	unsigned int            time_remain_ms;
	unsigned short            tone_index;

	unsigned short			tone;        /* temp tone      */
	union atu_tone_data_type	tone_data;   /* temp tone data */
  
	/* Get a pointer to the sound status for the sound being aged.  */
	tone_index  = atu.tone_index;

	/* get the current tone and duration from the sound structure */
	atu_get_sound_info(tone_index, &tone, &tone_data);

	/* Count of ms left on this tone */
	time_remain_ms = atu.time_remain_ms;
	if (time_remain_ms <= ms_elapsed_time) {
		do {
			ms_elapsed_time -= time_remain_ms;     /* Account for some time  */

			tone_index++;                          /* Next tone in sequence  */
                       
			/* get the current tone and duration from the sound structure */
			atu_get_sound_info(tone_index, &tone, &tone_data);
			/* Process special tones (stop, repeat, loop-back-2) */
			switch (tone) {
			/* Stop playing */
			case ATU_STOP:
				atu_stop_tone(); 
				return; /* discontinue this tone   */

			/* Callback then silence */
			case ATU_CALLBACK_SILENCE:
				if (atu.client_cb_ptr != NULL)  {
					atu.client_cb_ptr(ATU_REPEAT, atu.client_data );
				}
				break;

			/* Call Callback and repeat from start */
			case ATU_RPT:
				if (atu.client_cb_ptr != NULL)  {
					atu.client_cb_ptr(ATU_REPEAT, atu.client_data);
				}
			/* Fall through to next case */

			/* Repeat from start */
			case ATU_RPT_NOCALLBACK:
				if (atu.repeat_cnt != 0) {
					atu.repeat_cnt--;             /* decrement count */
					/* Check loop count, if it goes to 0, the loop is done. */
					if (atu.repeat_cnt == 0) {   /* stop this tone        */
					atu_stop_tone();
					return;                         /* discontinue this tone */
					}
				} else if (tone_data.data != 0) {
					atu.repeat_cnt = tone_data.data;
				}

				tone_index = 0;                    /* Loop back to first tone */
				atu_get_sound_info(tone_index,
					&tone, &tone_data);
				break;

			/* Loop back 2 tones */
			case ATU_LOOP_BACK2:
				if (atu.loop_cnt == 0) {
					atu.loop_cnt = tone_data.data;
				} else {      /* Else, the count is initialized, decrement it */
					--atu.loop_cnt;
				}

				/* Check loop count, if it goes to 0, the loop is done. */
				if (atu.loop_cnt == 0) {      /* Continue to next tone   */
					time_remain_ms = 0;              /* No time in this 'tone'  */
					continue;                        /* Continue from the top   */
				} else {                           /* Loop back 2 tones       */
					tone_index -= 2;                 /* Adjust the tone index   */
					atu_get_sound_info(tone_index, &tone, &tone_data);
				}
				break;

			/* Record the position of the label. */
			case ATU_LABEL:
				atu.label_index = tone_index;
				break;
  
			/* Loop back to the label */
			case ATU_BACK_TO_LABEL:
				if (atu.label_index == 0xff) {
				//(TBD)     ERR_FATAL("No label to loop to",0,0,0);
				}

				if (atu.label_cnt == 0) {
					atu.label_cnt = tone_data.data;
				} else {      /* Else, the count is initialized, decrement it */
					--atu.label_cnt;
				}

				/* Check loop count, if it goes to 0, the loop is done. */
				if (atu.label_cnt == 0) {     /* Continue to next tone   */
					atu.label_index = 0xff;     /* Done with this loop     */
					time_remain_ms       = 0;        /* No time in this 'tone'  */
				} else {                           /* Loop back to the label */
					tone_index     = atu.label_index;
					time_remain_ms = 0;              /* No time in this 'tone'  */
				}
				continue;                          /* Continue from the top   */
  
			/* Repeat the last tone forever */
			case ATU_RPT1:

				tone_index -= 1;                   /* Adjust the tone index   */
				atu_get_sound_info(tone_index, &tone, &tone_data);
				break;

			case ATU_VOL_SCALE:
          
				atu.scaling = tone_data.data;   /* Update vol scale       */
				time_remain_ms   = 0;           /* No time in this 'tone' */
				continue;  /* continue to prevent tone_data from being counted */ 
  
			default:                               /* No special action      */
				break;
			}
			time_remain_ms = tone_data.duration_ms;
		} while(time_remain_ms <= ms_elapsed_time);

		atu.tone_index = tone_index;        /* Update tone index       */
	}

	atu.time_remain_ms = time_remain_ms - ms_elapsed_time;
}


void atu_setup_tone()
{
	struct atu_compact_tone_type   compact_tone;       
	struct atu_flexible_tone_type  flexible_tone;      
	unsigned short		       tone = ATU_SILENCE; 
	struct atu_tdb_dtmf_type       dtmf;               
	const union atu_sound_type     *sound;   

	sound = (atu.sound_type);
        memset(&flexible_tone, 0, sizeof(struct atu_flexible_tone_type));        
 
	/* get current tone */
	switch(atu.sound_type->type) {
	case ATU_COMPACT_SOUND:
		compact_tone = (((struct atu_compact_sound_type *)sound)->tone_array)[atu.tone_index];
		tone         = compact_tone.tone;
		break;

	case ATU_FLEXIBLE_SOUND:
		flexible_tone = (((struct atu_flexible_sound_type *)sound)->tone_array)[atu.tone_index]; 
		tone          = flexible_tone.tone;
		break;


	case ATU_INVALID_SOUND:
	default:
		break;
	}

	if(tone == ATU_FREQ) {
		/* It is only possible to come through this point if flexible_tone */
		/* has been initilaized.                                           */
		/* If ATU_FREQ, use frequencies from freq tone structure           */
		atu.freq_hi = flexible_tone.freq_hi;
		atu.freq_lo = flexible_tone.freq_lo;      
		atu.playback_time = (unsigned short) atu.time_remain_ms;

		if (atu.playback_time > ATU_MAX_PERIOD){
			atu.playback_time = ATU_MAX_PERIOD;
		}
		atu.playback_time +=ATU_DTMF_GRAIN;

	} else if (tone < ATU_LAST_CONTROL_TONE){
		atu.freq_hi = 0;
		atu.freq_lo = 0;
		atu.playback_time = 0;
		atu.playback_vol = -8400;
	} else {
		/* Look up the tone in the tone database */
		DEBUG_PRINT("atu_setup_tone: tone=%d\n",tone);
		(void) atu_tdb_get_tone_freq(tone, &dtmf);
		atu.freq_hi = dtmf.hi;
		atu.freq_lo = dtmf.lo;      
		atu.playback_time = (unsigned short)atu.time_remain_ms;	  
		if (atu.playback_time > ATU_MAX_PERIOD){
			atu.playback_time = ATU_MAX_PERIOD;
		}
		atu.playback_time +=ATU_DTMF_GRAIN;
		
	}
}

void worker_thread(void *data)
{
	unsigned int time_to_wait_nsec = 0;
	unsigned int time_to_wait_sec  = 0;
	int  rc; 
	bool first_tone; 
	unsigned int ms_elapsed_time;
	struct timespec ts;
	struct timeval 	tp;
	struct timeval  time;
	first_tone = false;

	do {
		/* Sleep untill event come back */
		gettimeofday(&tp, NULL);

		ts.tv_sec = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000;
		
		ts.tv_nsec += time_to_wait_nsec;
		ts.tv_sec  += time_to_wait_sec;

		if(ts.tv_nsec >= NSEC_PER_SEC){
                ts.tv_sec++;
		ts.tv_nsec -= NSEC_PER_SEC; 
		}

		pthread_mutex_lock(&atu.mutex);
		if((time_to_wait_nsec == 0) && (time_to_wait_sec == 0))
		{
		rc = pthread_cond_wait(&atu.cond, &atu.mutex);
		}else{
		rc = pthread_cond_timedwait(&atu.cond, &atu.mutex,&ts);
		} 
		gettimeofday(&time, NULL);
		switch(rc) {
		case 0:
			if (atu.tone_state == ATU_START_REQ) {
				atu_setup_dtmf();
				first_tone = true;
				atu.tone_state = ATU_PLAYING;
			DEBUG_PRINT(" into ATU_PLAYING state \n");
			} else if (atu.tone_state == ATU_STOP_REQ) {
				atu_stop_tone();
				time_to_wait_sec  = 0;
				time_to_wait_nsec = 0;
				break;
			} else {
			 //TBD shouldn't reach here ERR FATAL
			}
		case ETIMEDOUT:
			ms_elapsed_time = (first_tone == true)? 0: atu.time_remain_ms;
			first_tone = false;
			atu_parse_sound (ms_elapsed_time); 

			if (atu.tone_state == ATU_PLAYING){
				atu_setup_tone();
				atu_play_tone();

				if(atu.time_remain_ms < 1000){
				time_to_wait_nsec = atu.time_remain_ms * 1000 * 1000;
				time_to_wait_sec = 0;
				}else{
				time_to_wait_nsec = 0;
                                time_to_wait_sec  = atu.time_remain_ms / 1000;
				}

			} else {
				time_to_wait_sec  = 0; 
				time_to_wait_nsec = 0;
			}
			break;
		default:
			printf("Invalid input\n");
			return;
		}

		pthread_mutex_unlock(&atu.mutex);
	} while (false == atu.exit_thread);
	
	return;
}


void atu_init()
{
	int rc;

	atu.exit_thread = false;
	atu.tone_state  = ATU_FREE;
	atu.device      = CAD_HW_DEVICE_ID_DEFAULT_RX;
	atu.rx_vol      = 0; // in mb; unity gain
	atu.tx_vol      = 0; // in mb; unity gain

	rc = pthread_cond_init(&atu.cond,NULL);
	if (rc < 0) {
		printf("Error create conditional \n");
		return;
	}
	pthread_mutex_init(&atu.mutex, 0);

	rc = pthread_create(&atu.thread, NULL, worker_thread, &atu);
	if (rc < 0 ) {
		printf("Error create new thread\n");
		return;
	}

}


void atu_dinit()
{

	atu.exit_thread = true;
	pthread_cond_signal(&atu.cond);

	pthread_join(atu.thread, NULL);

        pthread_mutex_destroy(&atu.mutex);
        pthread_cond_destroy(&atu.cond);
}

void atu_set_device(unsigned int device)
{
	atu.device = device;
}


void atu_set_rx_volume(unsigned short rx_vol)
{
	atu.rx_vol = rx_vol;
}

void atu_set_tx_volume(unsigned short tx_vol)
{
	atu.tx_vol = tx_vol;
}

int atu_start_sound_id(unsigned int sound_id, unsigned int repeat_cnt,
			   enum atu_path	tone_path,
			   atu_cb_func_ptr_type cb_ptr,
			   const void		*client_data)
{  
	unsigned int			rc  = ATU_FAILURE;
	unsigned short			tone;
	union atu_tone_data_type	tone_data;
  
	pthread_mutex_lock(&atu.mutex);

	if ((atu.tone_state == ATU_FREE) &&
		(atu_db_get_sound(sound_id,&atu.sound_type) == ATUI_SUCCESS)) {
		atu.sound_id   = sound_id;
		atu.path       = tone_path;
		atu.tone_index = 0;
		atu.client_cb_ptr = cb_ptr;
		atu.client_data   = client_data;
		atu.label_cnt     = 0;
		atu.label_index   = 0;
		atu.scaling       = 0;
		atu.repeat_cnt    = repeat_cnt;

		/*Get and set tone duration for the first tone         */
                 /* Question to Ankur about what this if for*/
		atu_get_sound_info(0, &tone, &tone_data);	
		atu.time_remain_ms = tone_data.duration_ms;

		atu.tone_state    = ATU_START_REQ;

		rc = ATU_SUCCESS;
	}
	pthread_mutex_unlock(&atu.mutex);
  
	if (rc == ATU_FAILURE){
		if (atu.tone_state != ATU_FREE)
			rc = ATU_BUSY;
	} else {    
		pthread_cond_signal(&atu.cond);
	}
	return rc;
}

int atu_start_dtmf(unsigned short f_hi_hz, unsigned short f_low_hz,
		       unsigned short		tone_duration_ms,
		       enum atu_path		tone_path,
		       atu_cb_func_ptr_type	cb_ptr,
		       const void		*client_data)
{
	pthread_mutex_lock(&atu.mutex);
	pthread_mutex_unlock(&atu.mutex);
	return ATU_FAILURE;
}


int atu_stop(void)
{
	int rc;

	rc = ATU_SUCCESS;
	pthread_mutex_lock(&atu.mutex);
  
	if (atu.tone_state != ATU_FREE) {
		atu.tone_state = ATU_STOP_REQ;
	} else {
		rc = ATU_INSTOP_STATE;
	}
	pthread_mutex_unlock(&atu.mutex);  
	if (rc == ATU_SUCCESS) {
		pthread_cond_signal(&atu.cond);
	}
	return rc;
}

