/*
 *
 * Copyright (C) 2009 Qualcomm Technologies, Inc.
 *
 */


#ifndef ATU_H
#define ATU_H

#define ATU_SUCCESS		 1
#define ATU_FAILURE		-1
#define ATU_BUSY		-2
#define ATU_INSTOP_STATE	-3

#ifdef _DEBUG

#define DEBUG_PRINT(args...) printf("%s:%d ", __FUNCTION__, __LINE__); \
                             printf(args)

#define DEBUG_PRINT_ERROR(args...) printf("%s:%d ", __FUNCTION__, __LINE__); \
                       printf(args)
#else

#define DEBUG_PRINT
#define DEBUG_PRINT_ERROR

#endif

enum atu_path{
	ATU_TONE_PATH_LOCAL,	/* DTMF's on local audio        */
	ATU_TONE_PATH_TX,	/* Transmit DTMFs               */
	ATU_TONE_PATH_BOTH,	/* Tx and sound DTMF's locally  */
	/* DO NOT USE: Force this enum to be a 32bit type */
	ATU_TONE_PATH_32BIT_DUMMY = 0xFFFFFFFF
};

/*
	The sound call back function argument type.
	See below call back prototype.
*/
enum atu_status{
	ATU_REPEAT,	/* Current sound has reached a repeat */
	ATU_STOP_DONE	/* Stop request has been completed */
};

typedef void (*atu_cb_func_ptr_type)(enum atu_status  status,
		const void  *client_data);


void atu_set_device(unsigned int device);
void atu_set_rx_volume(unsigned short rx_vol);
void atu_set_tx_volume(unsigned short rx_vol);

int atu_start_sound_id(unsigned int			sound_id, 
			   unsigned int			repeat_cnt,
	                   enum atu_path		tone_path,
                           atu_cb_func_ptr_type		cb_ptr,
			   const void			*client_data);

int atu_start_dtmf(unsigned short			f_hi_hz,
		       unsigned short			f_low_hz,
		       unsigned short			tone_duration_ms,
		       enum atu_path			tone_path,
		       atu_cb_func_ptr_type		cb_ptr,
		       const void			*client_data);


int atu_stop (void);
void atu_init();
void atu_dinit();
#endif
