/*
 * Copyright (c) 2010 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

/*
    An Open max test application ....
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/ioctl.h>
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "pthread.h"
#include <signal.h>
#include <asm/delay.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
#include "QOMX_AudioExtensions.h" 
#include "QOMX_AudioIndexExtensions.h" 
#ifdef AUDIOV2 
#include "control.h" 
#endif 
#include <linux/ioctl.h>

typedef unsigned char uint8;
typedef unsigned char byte;
typedef unsigned int  uint32;
typedef unsigned int  uint16;
#define AUDAAC_MAX_ADIF_HEADER_LENGTH 64
/* ADTS variable frame header, frame length field  */
#define AUDAAC_ADTS_FRAME_LENGTH_SIZE    13
QOMX_AUDIO_STREAM_INFO_DATA streaminfoparam;
void audaac_rec_install_bits
(
  uint8   *input,
  byte    num_bits_reqd,
  uint32  value,
  uint16  *hdr_bit_index
);

/* maximum ADTS frame header length                */
#define AUDAAC_MAX_ADTS_HEADER_LENGTH 7
void audaac_rec_install_adts_header_variable (uint16  byte_num);
void Release_Encoder();

#ifdef AUDIOV2
unsigned short session_id;
int device_id;
int control = 0;
const char *device="handset_tx";
#define DIR_TX 2
#endif

#define AACHDR_LAYER_SIZE             2
#define AACHDR_CRC_SIZE               1
#define AAC_PROFILE_SIZE              2
#define AAC_SAMPLING_FREQ_INDEX_SIZE  4
#define AAC_ORIGINAL_COPY_SIZE        1
#define AAC_HOME_SIZE                 1

#define MIN(A,B)    (((A) < (B))?(A):(B))

uint8   audaac_header[AUDAAC_MAX_ADTS_HEADER_LENGTH];
unsigned int audaac_hdr_bit_index;


FILE *F1 = NULL;

#define SAMPLE_RATE 48000
#define STEREO      2
uint32_t samplerate = 16000;
uint32_t channels = 2;
uint32_t bitrate = 32000;
uint32_t pcmplayback = 0;
uint32_t tunnel      = 0;
uint32_t rectime     = -1;
uint32_t stream_fmt  = 0;
#define DEBUG_PRINT printf
unsigned to_idle_transition = 0;
int tickcount;


//#define PCM_PLAYBACK /* To write the pcm decoded data to the msm_pcm device for playback*/

#ifdef PCM_PLAYBACK
  int                          m_pcmdrv_fd;

  struct msm_audio_pcm_config {
    uint32_t buffer_size;
    uint32_t buffer_count;
    uint32_t channel_count;
    uint32_t sample_rate;
    uint32_t type;
    uint32_t unused[3];
};

#define AUDIO_IOCTL_MAGIC 'a'
#define AUDIO_START        _IOW(AUDIO_IOCTL_MAGIC, 0, unsigned)
#define AUDIO_STOP         _IOW(AUDIO_IOCTL_MAGIC, 1, unsigned)
#define AUDIO_FLUSH        _IOW(AUDIO_IOCTL_MAGIC, 2, unsigned)
#define AUDIO_GET_CONFIG   _IOR(AUDIO_IOCTL_MAGIC, 3, unsigned)
#define AUDIO_SET_CONFIG   _IOW(AUDIO_IOCTL_MAGIC, 4, unsigned)
#define AUDIO_GET_STATS    _IOR(AUDIO_IOCTL_MAGIC, 5, unsigned)

#endif  // PCM_PLAYBACK

typedef enum adts_sample_index__ {

ADTS_SAMPLE_INDEX_96000=0x0,
ADTS_SAMPLE_INDEX_88200,
ADTS_SAMPLE_INDEX_64000,
ADTS_SAMPLE_INDEX_48000,
ADTS_SAMPLE_INDEX_44100,
ADTS_SAMPLE_INDEX_32000,
ADTS_SAMPLE_INDEX_24000,
ADTS_SAMPLE_INDEX_22050,
ADTS_SAMPLE_INDEX_16000,
ADTS_SAMPLE_INDEX_12000,
ADTS_SAMPLE_INDEX_11025,
ADTS_SAMPLE_INDEX_8000,
ADTS_SAMPLE_INDEX_7350,
ADTS_SAMPLE_INDEX_MAX

}adts_sample_index;
/************************************************************************/
/*                #DEFINES                            */
/************************************************************************/
#define false 0
#define true 1

#define CONFIG_VERSION_SIZE(param) \
    param.nVersion.nVersion = CURRENT_OMX_SPEC_VERSION;\
    param.nSize = sizeof(param);

#define FAILED(result) (result != OMX_ErrorNone)

#define SUCCEEDED(result) (result == OMX_ErrorNone)

/************************************************************************/
/*                GLOBAL DECLARATIONS                     */
/************************************************************************/

pthread_mutex_t lock;
pthread_cond_t cond;
pthread_mutex_t elock;
pthread_cond_t econd;
pthread_cond_t fcond;
FILE * inputBufferFile;
FILE * outputBufferFile;
OMX_PARAM_PORTDEFINITIONTYPE inputportFmt;
OMX_PARAM_PORTDEFINITIONTYPE outputportFmt;
OMX_AUDIO_PARAM_AACPROFILETYPE aacparam;
OMX_AUDIO_PARAM_PCMMODETYPE    pcm_param;
OMX_PORT_PARAM_TYPE portParam;
OMX_ERRORTYPE error;

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
	uint32_t riff_id;
	uint32_t riff_sz;
	uint32_t riff_fmt;
	uint32_t fmt_id;
	uint32_t fmt_sz;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;       /* sample_rate * num_channels * bps / 8 */
	uint16_t block_align;     /* num_channels * bps / 8 */
	uint16_t bits_per_sample;
	uint32_t data_id;
	uint32_t data_sz;
};

struct audio_pvt_data {
	int afd;
	int mode; /* tunnel, non-tunnel */
	int recsize;
	int frame_count;
	unsigned avail;
	unsigned org_avail;
	int channels;
	int freq;
	char *next;
	char *org_next;
	int bitspersample;
};


struct wav_header hdr;
int fd;
struct audio_pvt_data *audio_data = NULL;

static unsigned totaldatalen = 0;
/************************************************************************/
/*                GLOBAL INIT                    */
/************************************************************************/

int input_buf_cnt = 0;
int output_buf_cnt = 0;
int used_ip_buf_cnt = 0;
volatile int event_is_done = 0;
volatile int ebd_event_is_done = 0;
volatile int fbd_event_is_done = 0;
int ebd_cnt;
volatile int bInputEosReached = 0;
volatile int bEosReached = 0;
int bFlushing = false;
int bPause    = false;
int ip_buf_size = 0;
unsigned int bEos_sent = 0;
const char *in_filename;
const char *out_filename;
const char *input_filename;

int timeStampLfile = 0;
int timestampInterval = 100;

//* OMX Spec Version supported by the wrappers. Version = 1.1 */
const OMX_U32 CURRENT_OMX_SPEC_VERSION = 0x00000101;
OMX_COMPONENTTYPE* aac_enc_handle = 0;

OMX_BUFFERHEADERTYPE  **pInputBufHdrs = NULL;
OMX_BUFFERHEADERTYPE  **pOutputBufHdrs = NULL;

/************************************************************************/
/*                GLOBAL FUNC DECL                        */
/************************************************************************/
int Init_Encoder();
int Play_Encoder();

/**************************************************************************/
/*                STATIC DECLARATIONS                       */
/**************************************************************************/

static int open_audio_file ();
static OMX_ERRORTYPE Allocate_Buffer ( OMX_COMPONENTTYPE *aac_enc_handle,
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                       OMX_U32 nPortIndex,
                                       long bufCntMin, long bufSize);


static OMX_ERRORTYPE EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_PTR pAppData,
                                  OMX_IN OMX_EVENTTYPE eEvent,
                                  OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2,
                                  OMX_IN OMX_PTR pEventData);
static OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_PTR pAppData,
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE FillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_PTR pAppData,
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static int read_input_file();

void wait_for_event(void)
{
	pthread_mutex_lock(&lock);
	DEBUG_PRINT("%s: event_is_done=%d", __FUNCTION__, event_is_done);
	while (event_is_done == 0) {
		pthread_cond_wait(&cond, &lock);
	}
	event_is_done = 0;
	pthread_mutex_unlock(&lock);
}

void event_complete(void )
{
	pthread_mutex_lock(&lock);
	if (event_is_done == 0) {
	event_is_done = 1;
		pthread_cond_broadcast(&cond);
	}
	pthread_mutex_unlock(&lock);
}


OMX_ERRORTYPE EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                           OMX_IN OMX_PTR pAppData,
                           OMX_IN OMX_EVENTTYPE eEvent,
                           OMX_IN OMX_U32 nData1, 
						   OMX_IN OMX_U32 nData2,
                           OMX_IN OMX_PTR pEventData)
{
	DEBUG_PRINT("Function %s \n", __FUNCTION__);
	(void )hComponent; /* To avoid warnings */
	(void )pAppData; /* To avoid warnings */
	switch(eEvent) {
		case OMX_EventCmdComplete:
			DEBUG_PRINT("\n OMX_EventCmdComplete event=%d data1=%lu data2=%lu\n",(OMX_EVENTTYPE)eEvent,
											       nData1,nData2);
			    if(to_idle_transition)
				bInputEosReached = 1;
			event_complete();
			break;
		case OMX_EventError:
			DEBUG_PRINT("\n OMX_EventError \n");
			break;
		case OMX_EventPortSettingsChanged:
			DEBUG_PRINT("\n OMX_EventPortSettingsChanged \n");
			break;
		case OMX_EventBufferFlag:
			DEBUG_PRINT("\n EOS event reached event handler in test app \n");
			if(*((OMX_U32 *)pEventData) == 1)
			{
				DEBUG_PRINT("\n EOS event reached event handler from o/p thread\n");
			    bEosReached = 1;
			}	
			else if(*((OMX_U32 *)pEventData) == 0)
			{
				DEBUG_PRINT("\n EOS event reached event handler from i/p thread\n");
			    bInputEosReached = 1;
			}
			break;
		default:
			DEBUG_PRINT("\n Unknown Event \n");
			break;
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE FillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_PTR pAppData,
                              OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
	unsigned int bytes_writen = 0;
	static unsigned int  count = 0;
	unsigned char readBuf;
	static int releaseCount = 0;

	(void)pAppData; /* To avoid warnings */
	if(bInputEosReached || (pBuffer->nFilledLen == 0)) {
		DEBUG_PRINT("\n*********************************************\n");
		DEBUG_PRINT("   EBD::EOS on output port\n ");
		DEBUG_PRINT("   TBD:::De Init the open max here....!!!\n");
		DEBUG_PRINT("*********************************************\n");
		DEBUG_PRINT("tunnel = %d   bEosReached = %d \n", tunnel, bEosReached);
		if (!tunnel && bEosReached) {
			DEBUG_PRINT("stop issuing FTB can issue state transition\n");
			//sleep(1);
			OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StateIdle,0);
			sleep(1);
			to_idle_transition = 1;
			releaseCount++;
			// wait till Idle transition is complete
			// Trigger ReleaseEncoder procedure
		} else
		{
			DEBUG_PRINT("Tunnel mode\n");
		}
		return OMX_ErrorNone;
	}

	DEBUG_PRINT(" FillBufferDone #%d size %lu TS[%lld]\n", ++count,pBuffer->nFilledLen,
							   pBuffer->nTimeStamp);
    if(tunnel == 1)
	{
		audaac_rec_install_adts_header_variable(pBuffer->nFilledLen + AUDAAC_MAX_ADTS_HEADER_LENGTH);
		bytes_writen = fwrite(audaac_header,1,AUDAAC_MAX_ADTS_HEADER_LENGTH,outputBufferFile);
		if(bytes_writen < AUDAAC_MAX_ADTS_HEADER_LENGTH)
		{
			DEBUG_PRINT("error: invalid adts header length\n");
			return OMX_ErrorNone;
		}
    }
	bytes_writen = fwrite(pBuffer->pBuffer,1,pBuffer->nFilledLen,outputBufferFile);
	if(bytes_writen < pBuffer->nFilledLen)
	{
		DEBUG_PRINT("error: invalid AAC encoded data \n");
		return OMX_ErrorNone;
	}


	DEBUG_PRINT(" FillBufferDone size writen to file  %d\n",bytes_writen);
	totaldatalen += bytes_writen ;

	if(!releaseCount)
	{
		if(read(0, &readBuf, 1) == 1)
		{
			if ((readBuf == 13) || (readBuf == 10))
			{
				printf("\n GOT THE ENTER KEY\n");
				releaseCount++;
			}
		}
	}
	if((releaseCount == 1 || rectime == 0 ) && (tunnel == 1))
	{
		// Dont issue any more FTB
		// Trigger Exe-->Idle Transition
		DEBUG_PRINT("stop issuing FTB can issue state transition\n");
		sleep(1);
		OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StateIdle,0);
		sleep(1);
		to_idle_transition = 1;
		releaseCount++;
		// wait till Idle transition is complete
		// Trigger ReleaseEncoder procedure
	}
	else if(!releaseCount)
	{
		DEBUG_PRINT(" FBD calling FTB");
		OMX_FillThisBuffer(hComponent,pBuffer);
		DEBUG_PRINT("EOS reached in fillbuffer done cb\n");
	}
	else{}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_PTR pAppData,
                              OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
	int result = 0;

	(void)pAppData; /* To avoid warnings */
	DEBUG_PRINT("\nFunction %s cnt[%d]\n", __FUNCTION__, ebd_cnt);
	ebd_cnt++;
	used_ip_buf_cnt--;
	if(bInputEosReached) {
		DEBUG_PRINT("\n*********************************************\n");
		DEBUG_PRINT("   EBD::EOS on input port\n ");
		DEBUG_PRINT("   TBD:::De Init the open max here....!!!\n");
		DEBUG_PRINT("*********************************************\n");
		return OMX_ErrorNone;
	}    else if (bFlushing == true) {
		if (used_ip_buf_cnt == 0) {
			fseek(inputBufferFile, 0, 0);
			bFlushing = false;
		} else {
			DEBUG_PRINT("omx_aac_aenc_test: more buffer to come back\n");
			return OMX_ErrorNone;
		}
	}

	result = read_input_file(pBuffer);
	if(result == -1)
	{
		DEBUG_PRINT("Failed to read input data\n");
		bInputEosReached = true;
		      		
	} else {
		pBuffer->nFilledLen = ip_buf_size;
		used_ip_buf_cnt++;
	//	mdelay(10);
		OMX_EmptyThisBuffer(hComponent,pBuffer);
	}
	
	return OMX_ErrorNone;
}

int main(int argc, char **argv)
{
	int bufCnt=0;
	OMX_ERRORTYPE result = OMX_ErrorNone;

	pthread_cond_init(&cond, 0);
	pthread_mutex_init(&lock, 0);

	if(argc < 2)
	{
		DEBUG_PRINT("Invalid no of params\n");
		DEBUG_PRINT("ex: ./mm-aenc-omxaac TUNNEL AAC_OUTPUTFILE SAMPFREQ CHANNEL PCMPLAYBACK RECORDTIME BITRATE\n");
		DEBUG_PRINT("ex: ./mm-aenc-omxaac TUNNEL INPUT_FILE_NAME AAC_OUTPUTFILE SAMPFREQ CHANNEL PCMPLAYBACK BITRATE\n");
		return 0;
	}
        tunnel  = atoi(argv[1]);
	if(tunnel == 1)
	{
		if(argc >= 8)
		{
			out_filename = argv[2];
			samplerate = atoi(argv[3]);
			channels = atoi(argv[4]);
			pcmplayback = atoi(argv[5]);
			rectime = atoi(argv[6]);
			bitrate = atoi(argv[7]);
		} else
		{
			DEBUG_PRINT(" invalid format: \n");
			DEBUG_PRINT("ex: ./mm-aenc-omxaac TUNNEL AAC_OUTPUTFILE SAMPFREQ CHANNEL PCMPLAYBACK RECORDTIME BITRATE\n");
			DEBUG_PRINT( "TUNNEL = 1 (ENCODED AAC SAMPLES WRITEN TO AAC_OUTPUTFILE)\n");
			DEBUG_PRINT( "RECORDTIME in minutes for AST Automation\n");
			DEBUG_PRINT( "BITRATE in bits/sec \n");
			return 0;
		}
	} else if(tunnel == 0)
	{
		if(argc >= 9)
		{
			input_filename = argv[2];
			out_filename = argv[3];
			samplerate = atoi(argv[4]);
			channels = atoi(argv[5]);
			pcmplayback = atoi(argv[6]);
			bitrate = atoi(argv[7]);
			stream_fmt = atoi(argv[8]);
			rectime = 0;
		} else
		{
			DEBUG_PRINT(" invalid format: \n");
			DEBUG_PRINT("ex: ./mm-aenc-omxaac TUNNEL INPUT_FILE_NAME AAC_OUTPUTFILE SAMPFREQ CHANNEL PCMPLAYBACK BITRATE STREAM_FORMAT\n");
			DEBUG_PRINT( "TUNNEL = 0 (ENCODED AAC SAMPLES WRITEN TO AAC_OUTPUTFILE)\n");
			DEBUG_PRINT( "RECORDTIME in minutes for AST Automation\n");
			DEBUG_PRINT( "BITRATE in bits/sec \n");
			return 0;
		}
	}else
	{
		DEBUG_PRINT("Inavlid option \n");
		return 0;
	}
	if(Init_Encoder()!= 0x00)
	{
		DEBUG_PRINT("Decoder Init failed\n");
		return -1;
	}

	fcntl(0, F_SETFL, O_NONBLOCK);

	if(Play_Encoder() != 0x00)
	{
		DEBUG_PRINT("Play_Decoder failed\n");
		return -1;
	}

	// Wait till EOS is reached...
	if(rectime)
	{
		sleep(rectime);
		rectime = 0;
		wait_for_event();
	}
	else
	{
		wait_for_event();
	}

	if(bInputEosReached)
	{
		DEBUG_PRINT("\nMoving the encoder to loaded state \n");
		OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StateLoaded,0);

		DEBUG_PRINT ("\nFillBufferDone: Deallocating o/p buffers \n");
		for(bufCnt=0; bufCnt < output_buf_cnt; ++bufCnt) {
			result = OMX_FreeBuffer(aac_enc_handle, 1, pOutputBufHdrs[bufCnt]);
			if(result != OMX_ErrorNone)
			{
				DEBUG_PRINT("Failed to free output buffer %p\n",pOutputBufHdrs[bufCnt]);
			}
		}
		if(tunnel == 0)
		{
			for(bufCnt=0; bufCnt < input_buf_cnt; ++bufCnt)
			{
				OMX_FreeBuffer(aac_enc_handle, 0, pInputBufHdrs[bufCnt]);
				if(result != OMX_ErrorNone)
				{
					DEBUG_PRINT("Failed to free input buffer %p\n",pInputBufHdrs[bufCnt]);
				}
			}
		}
		wait_for_event();

		fclose(outputBufferFile);

		result = OMX_FreeHandle(aac_enc_handle);
		if (result != OMX_ErrorNone) {
			DEBUG_PRINT ("\nOMX_FreeHandle error. Error code: %d\n", result);
		}

		/* Deinit OpenMAX */
		#ifdef AUDIOV2
		if(tunnel)
		{
			if (msm_route_stream(DIR_TX,session_id,device_id, 0))
			{
				DEBUG_PRINT("\ncould not set stream routing\n");
				return -1;
			}
			if (msm_en_device(device_id, 0))
			{
				DEBUG_PRINT("\ncould not enable device\n");
				return -1;
			}
			msm_mixer_close();
		}
		#endif

		OMX_Deinit();

		ebd_cnt=0;
		bInputEosReached = false;
		aac_enc_handle = NULL;
		pthread_cond_destroy(&cond);
		pthread_mutex_destroy(&lock);
		DEBUG_PRINT("*****************************************\n");
		DEBUG_PRINT("******...AAC ENC TEST COMPLETED...***************\n");
		DEBUG_PRINT("*****************************************\n");
	}
    return 0;
}

int Init_Encoder()
{
	DEBUG_PRINT("Inside %s \n", __FUNCTION__);
	OMX_ERRORTYPE omxresult;
	OMX_U32 total = 0;
	typedef OMX_U8* OMX_U8_PTR;
	char *role ="audio_encoder.aac";

	static OMX_CALLBACKTYPE call_back = {
		&EventHandler,&EmptyBufferDone,&FillBufferDone
	};
	DEBUG_PRINT("EventHandler address in Init_Encoder %p \n",&EventHandler);

	/* Init. the OpenMAX Core */
	DEBUG_PRINT("\nInitializing OpenMAX Core....\n");
	omxresult = OMX_Init();

	if(OMX_ErrorNone != omxresult) {
		DEBUG_PRINT("\n Failed to Init OpenMAX core");
		return -1;
	}
	else {
		DEBUG_PRINT("\nOpenMAX Core Init Done\n");
	}

	/* Query for audio encoders*/
	DEBUG_PRINT("Aac_test: Before entering OMX_GetComponentOfRole");
	OMX_GetComponentsOfRole(role, &total, 0);
	DEBUG_PRINT ("\nTotal components of role=%s :%lu", role, total);

	if(tunnel == 1)
	{
		omxresult = OMX_GetHandle((OMX_HANDLETYPE*)(&aac_enc_handle),
					"OMX.qcom.audio.encoder.tunneled.aac", NULL, &call_back);
	}
	else
	{
		DEBUG_PRINT("Getting NT mode handle\n");
		omxresult = OMX_GetHandle((OMX_HANDLETYPE*)(&aac_enc_handle),
					"OMX.qcom.audio.encoder.aac", NULL, &call_back);
	}



	if (FAILED(omxresult)) {
		DEBUG_PRINT ("\nFailed to Load the component: %d \n",omxresult);
		return -1;
	}
	else
	{
		DEBUG_PRINT ("\nComponent is in LOADED state\n");
	}


	/* Get the port information */
	CONFIG_VERSION_SIZE(portParam);
	omxresult = OMX_GetParameter(aac_enc_handle, OMX_IndexParamAudioInit,
				(OMX_PTR)&portParam);

	if(FAILED(omxresult)) {
		DEBUG_PRINT("\nFailed to get Port Param\n");
		return -1;
	}
	else
	{
		DEBUG_PRINT ("\nportParam.nPorts:%lu\n", portParam.nPorts);
		DEBUG_PRINT ("\nportParam.nStartPortNumber:%lu\n",
				portParam.nStartPortNumber);
	}
	return 0;
}

int Play_Encoder()
{
	int i;
	int result = 0;
	DEBUG_PRINT("Inside %s \n", __FUNCTION__);
	OMX_ERRORTYPE ret;
	OMX_INDEXTYPE index;
	DEBUG_PRINT("sizeof[%d]\n", sizeof(OMX_BUFFERHEADERTYPE));

	audio_data = (struct audio_pvt_data *)malloc(sizeof(struct audio_pvt_data));
	DEBUG_PRINT("Size of audio_data %d \n",sizeof(struct audio_pvt_data));
	if(!audio_data)
	{
		DEBUG_PRINT("Failed to allocate memory to audio_data \n");
		return -1;
	}

	/* open the i/p and o/p files based on the video file format passed */
	if(open_audio_file()) {
		DEBUG_PRINT("\n Returning -1");
		return -1;
	}

	/* Query the encoder input min buf requirements */
	CONFIG_VERSION_SIZE(inputportFmt);

	/* Port for which the Client needs to obtain info */
	inputportFmt.nPortIndex = portParam.nStartPortNumber;

	OMX_GetParameter(aac_enc_handle,OMX_IndexParamPortDefinition,&inputportFmt);
	DEBUG_PRINT ("\nEnc Input Buffer Count %lu\n", inputportFmt.nBufferCountMin);
	DEBUG_PRINT ("\nEnc: Input Buffer Size %lu\n", inputportFmt.nBufferSize);
        input_buf_cnt = inputportFmt.nBufferCountMin;
	if(OMX_DirInput != inputportFmt.eDir) {
		DEBUG_PRINT ("\nEnc: Expect Input Port\n");
		return -1;
	}


	/* Query the encoder outport's min buf requirements */
	CONFIG_VERSION_SIZE(outputportFmt);
	/* Port for which the Client needs to obtain info */
	outputportFmt.nPortIndex = portParam.nStartPortNumber + 1;

	OMX_GetParameter(aac_enc_handle,OMX_IndexParamPortDefinition,&outputportFmt);
	DEBUG_PRINT ("\nEnc: Output Buffer Count %lu\n", outputportFmt.nBufferCountMin);
	DEBUG_PRINT ("\nEnc: Output Buffer Size %lu\n", outputportFmt.nBufferSize);

	if(OMX_DirOutput != outputportFmt.eDir) {
	DEBUG_PRINT ("\nEnc: Expect Output Port\n");
	return -1;
	}


	CONFIG_VERSION_SIZE(aacparam);
	CONFIG_VERSION_SIZE(pcm_param);

	pcm_param.nPortIndex = 0;
	pcm_param.nSamplingRate = samplerate;
	pcm_param.nChannels = channels;
	OMX_SetParameter(aac_enc_handle,OMX_IndexParamAudioPcm,&pcm_param);


	aacparam.nPortIndex   =  1;
	aacparam.nChannels    =  channels; //2 ; /* 1-> mono 2-> stereo*/
	aacparam.nBitRate     =  bitrate;
	aacparam.nSampleRate  =  samplerate;
	aacparam.eChannelMode =  OMX_AUDIO_ChannelModeStereo;
	//aacparam.eAACStreamFormat    =  OMX_AUDIO_AACStreamFormatMP4ADTS;
	if(stream_fmt == 0)
		aacparam.eAACStreamFormat    =  OMX_AUDIO_AACStreamFormatMP4ADTS;
	else if(stream_fmt == 1)
		aacparam.eAACStreamFormat    =  OMX_AUDIO_AACStreamFormatADIF;
	else if(stream_fmt == 2)
		aacparam.eAACStreamFormat    =  OMX_AUDIO_AACStreamFormatMP4FF;
	else if(stream_fmt == 3)
		aacparam.eAACStreamFormat    =  OMX_AUDIO_AACStreamFormatRAW;
	else
	{
		DEBUG_PRINT("Format not supported in main \n");
		return -1;
	}
	OMX_SetParameter(aac_enc_handle,OMX_IndexParamAudioAac,&aacparam);


	OMX_GetExtensionIndex(aac_enc_handle,"OMX.Qualcomm.index.audio.sessionId",&index); 
	OMX_GetParameter(aac_enc_handle,index,&streaminfoparam); 
	#ifdef AUDIOV2
	if(tunnel)
	{
		session_id = streaminfoparam.sessionId; 
		control = msm_mixer_open("/dev/snd/controlC0", 0); 
		if(control < 0) 
		printf("ERROR opening the device\n"); 
		device_id = msm_get_device(device); 
		DEBUG_PRINT ("\ndevice_id = %d\n",device_id); 
		DEBUG_PRINT("\nsession_id = %d\n",session_id); 
		if (msm_en_device(device_id, 1)) 
		{ 
			perror("could not enable device\n"); 
			return -1; 
		} 

		if (msm_route_stream(DIR_TX,session_id,device_id, 1)) 
		{ 
			perror("could not set stream routing\n"); 
			return -1; 
		} 
	}
	#endif


	DEBUG_PRINT ("\nOMX_SendCommand Encoder -> IDLE\n");
	OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StateIdle,0);
	/* wait_for_event(); should not wait here event complete status will
	not come until enough buffer are allocated */


	output_buf_cnt = outputportFmt.nBufferCountMin ;

	/* Allocate buffer on encoder's O/Pp port */
	error = Allocate_Buffer(aac_enc_handle, &pOutputBufHdrs, outputportFmt.nPortIndex,
			    output_buf_cnt, outputportFmt.nBufferSize);
	if (error != OMX_ErrorNone) {
		DEBUG_PRINT ("\nOMX_AllocateBuffer Output buffer error\n");
		return -1;
	}
	else {
		DEBUG_PRINT ("\nOMX_AllocateBuffer Output buffer success\n");
	}

	/* Allocate buffers on encoder's I/P port */
	if(tunnel == 0)
	{
		error = Allocate_Buffer(aac_enc_handle, &pInputBufHdrs, inputportFmt.nPortIndex,
				    input_buf_cnt, inputportFmt.nBufferSize);
		if (error != OMX_ErrorNone) {
			DEBUG_PRINT ("\nOMX_AllocateBuffer Input buffer error\n");
			return -1;
		}
		else {
			DEBUG_PRINT ("\nOMX_AllocateBuffer input buffer success\n");
		}
        }
	wait_for_event();

	if (tunnel == 1)
	{
		DEBUG_PRINT ("\nOMX_SendCommand to enable TUNNEL MODE during IDLE\n");
		OMX_SendCommand(aac_enc_handle, OMX_CommandPortDisable,0,0); // disable input port
		wait_for_event();
	}

	DEBUG_PRINT ("\nOMX_SendCommand encoder -> Executing\n");
	OMX_SendCommand(aac_enc_handle, OMX_CommandStateSet, OMX_StateExecuting,0);
	wait_for_event();

	DEBUG_PRINT(" Start sending OMX_FILLthisbuffer\n");

	for(i=0; i < output_buf_cnt; i++) {
		DEBUG_PRINT ("\nOMX_FillThisBuffer on output buf no.%d\n",i);
		pOutputBufHdrs[i]->nOutputPortIndex = 1;
		pOutputBufHdrs[i]->nFlags &= ~OMX_BUFFERFLAG_EOS;
		ret = OMX_FillThisBuffer(aac_enc_handle, pOutputBufHdrs[i]);
		if (OMX_ErrorNone != ret) {
			DEBUG_PRINT("OMX_FillThisBuffer failed with result %d\n", ret);
		}
		else {
			DEBUG_PRINT("OMX_FillThisBuffer success!\n");
		}
	}

	if(tunnel == 0)
	{
		DEBUG_PRINT(" Start sending OMX_emptythisbuffer\n");
		for (i = 0;i < input_buf_cnt;i++) {

			DEBUG_PRINT ("\nOMX_EmptyThisBuffer on Input buf no.%d\n",i);
			pInputBufHdrs[i]->nInputPortIndex = 0;
			result = read_input_file(pInputBufHdrs[i]);
			if(result == -1)
			{
				DEBUG_PRINT("Failed to read input data\n");
				return -1;
			}
			DEBUG_PRINT("Success in reading input data\n");
			pInputBufHdrs[i]->nFilledLen = ip_buf_size;
			pInputBufHdrs[i]->nInputPortIndex = 0;
			used_ip_buf_cnt++;
			ret = OMX_EmptyThisBuffer(aac_enc_handle, pInputBufHdrs[i]);
			if (OMX_ErrorNone != ret) {
				DEBUG_PRINT("OMX_EmptyThisBuffer failed with result %d\n", ret);
			}
			else {
				DEBUG_PRINT("OMX_EmptyThisBuffer success!\n");
			}
		}
	}

	return 0;
}



static OMX_ERRORTYPE Allocate_Buffer ( OMX_COMPONENTTYPE *avc_enc_handle,
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                       OMX_U32 nPortIndex,
                                       long bufCntMin, long bufSize)
{
	DEBUG_PRINT("Inside %s \n", __FUNCTION__);
	OMX_ERRORTYPE error=OMX_ErrorNone;
	long bufCnt=0;

	(void)avc_enc_handle; /* To avoid warnings */
	*pBufHdrs= (OMX_BUFFERHEADERTYPE **)
		   malloc(sizeof(OMX_BUFFERHEADERTYPE*)*bufCntMin);

	for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) {
		DEBUG_PRINT("\n OMX_AllocateBuffer No %ld \n", bufCnt);
		error = OMX_AllocateBuffer(aac_enc_handle, &((*pBufHdrs)[bufCnt]),
					   nPortIndex, NULL, bufSize);
	}

	return error;
}



static int fill_pcm_buffer(void *buf, unsigned sz, void *cookie)
{
        struct audio_pvt_data *audio_data = (struct audio_pvt_data *) cookie;
        unsigned cpy_size = 0;
        DEBUG_PRINT("cpy_size = %d audio_data->next = %p buf = %p\n", cpy_size, audio_data->next, buf);
        cpy_size = (sz < audio_data->avail ? sz : audio_data->avail);
        DEBUG_PRINT("cpy_size = %d audio_data->next = %p buf = %p\n", cpy_size, audio_data->next, buf);
        if (audio_data->avail == 0)
	{
		DEBUG_PRINT("audio_data->avail is zero \n");
        	return -1;
	}
        if (!audio_data->next) {
        	printf("error in next buffer returning with out copying\n");
	        return -1;
        }
        if (cpy_size == 0) {
		DEBUG_PRINT("cpy_size is zero \n");
        	return -1;
        }
        memcpy(buf, audio_data->next, cpy_size);
        audio_data->next += cpy_size;
        audio_data->avail -= cpy_size;
        return cpy_size;
}


static int read_input_file(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
	static int tmp1 = 0;
	unsigned long int duration = 0;
	DEBUG_PRINT("Inside read_input_file \n");
        if(tmp1 == 0)
        {
		fd = open(input_filename, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "playwav: cannot open '%s'\n", input_filename);
			return -1;
		}
                tmp1++;
		if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
			fprintf(stderr, "playwav: cannot read header\n");
			return -1;
		}
		fprintf(stderr,"playwav: %d ch, %d hz, %d bit, %s\n",
			hdr.num_channels, hdr.sample_rate, hdr.bits_per_sample,
			hdr.audio_format == FORMAT_PCM ? "PCM" : "unknown");

		if ((hdr.riff_id != ID_RIFF) ||
		(hdr.riff_fmt != ID_WAVE) ||
		(hdr.fmt_id != ID_FMT)) {
			fprintf(stderr, "playwav: '%s' is not a riff/wave file\n",
				input_filename);
			return -1;
		}
		if ((hdr.audio_format != FORMAT_PCM) ||
		(hdr.fmt_sz != 16)) {
			fprintf(stderr, "playwav: '%s' is not pcm format\n", input_filename);
			return -1;
		}
		if (hdr.bits_per_sample != 16) {
			fprintf(stderr, "playwav: '%s' is not 16bit per sample\n", input_filename);
			return -1;
		}

		audio_data->next = (char*)malloc(hdr.data_sz);
		if (!audio_data->next) {
			DEBUG_PRINT("could not allocate %d bytes\n", hdr.data_sz);
			return -1;
		}
		audio_data->org_next = audio_data->next;
		printf(" play_file: count=%d,next=%p\n", hdr.data_sz, audio_data->next);
		if (read(fd, audio_data->next, hdr.data_sz) != (ssize_t)hdr.data_sz) {
			fprintf(stderr,"could not read %d bytes\n", hdr.data_sz);
			return -1;
		}
		audio_data->avail = hdr.data_sz;
		audio_data->org_avail = audio_data->avail;
	}

        ip_buf_size = fill_pcm_buffer(pBufHdr->pBuffer,8192,(void *)audio_data);
	if((ip_buf_size < 8192) && !bEos_sent)
	{
		DEBUG_PRINT("Reached EOS \n");
		duration = audio_data->frame_count * ((ip_buf_size* 1000) /(2 * channels * samplerate));
		DEBUG_PRINT("Duration = %ld\n",duration);
		pBufHdr->nFlags = 1;
		pBufHdr->nTickCount = tickcount++;
		pBufHdr->nFilledLen = ip_buf_size;
		pBufHdr->nTimeStamp = duration;
		bEos_sent = 1;
	}
	else if(ip_buf_size == 8192)
	{
		DEBUG_PRINT("Success in filling i/p pcm buffer %d \n",ip_buf_size);
		duration = audio_data->frame_count * ((ip_buf_size* 1000) /(2 * channels * samplerate));
		DEBUG_PRINT("Duration = %ld\n",duration);
		pBufHdr->nFlags = 0;
		pBufHdr->nTickCount = tickcount++;
		pBufHdr->nFilledLen = ip_buf_size;
		pBufHdr->nTimeStamp = duration;
	}
	else
	{
		DEBUG_PRINT("In else condition %d \n",ip_buf_size);
		return -1;
	}
        audio_data->frame_count++;
	return 0;

}

//In Encoder this Should Open a PCM or WAV file for input.

static int open_audio_file ()
{
    int error_code = 0;

    DEBUG_PRINT("Inside %s filename=%s\n", __FUNCTION__, out_filename);
                outputBufferFile = fopen (out_filename, "wb");
    if (outputBufferFile == NULL) {
	DEBUG_PRINT("\ni/p file %s could NOT be opened\n",
			out_filename);
	error_code = -1;
    }

    return error_code;
}


void audaac_rec_install_bits
(
  uint8   *input,
  byte    num_bits_reqd,
  uint32  value,
  uint16  *hdr_bit_index
)
{
	uint32 byte_index;
	byte   bit_index;
	byte   bits_avail_in_byte;
	byte   num_to_copy;
	byte   byte_to_copy;

	byte   num_remaining = num_bits_reqd;
	uint8  bit_mask;

	bit_mask = 0xFF;

	while (num_remaining) {

		byte_index = (*hdr_bit_index) >> 3;
		bit_index  = (*hdr_bit_index) &  0x07;

		bits_avail_in_byte = 8 - bit_index;

		num_to_copy = MIN(bits_avail_in_byte, num_remaining);

		byte_to_copy = ((uint8)((value >> (num_remaining - num_to_copy)) & 0xFF) <<
			    (bits_avail_in_byte - num_to_copy));

		input[byte_index] &= ((uint8)(bit_mask << bits_avail_in_byte));
		input[byte_index] |= byte_to_copy;

		*hdr_bit_index += num_to_copy;

		num_remaining -= num_to_copy;
	} /* while (num_remaining) */
} /* audaac_rec_install_bits */

adts_sample_index  map_adts_sample_index(uint32 srate)
{
	adts_sample_index ret;

	switch(srate){

		case 96000:
		ret= ADTS_SAMPLE_INDEX_96000;
		break;
		case 88200:
		ret= ADTS_SAMPLE_INDEX_88200;
		break;
		case 64000:
		ret= ADTS_SAMPLE_INDEX_64000;
		break;
		case 48000:
		ret=ADTS_SAMPLE_INDEX_48000;
		break;
		case 44100:
		ret=ADTS_SAMPLE_INDEX_44100;
		break;
		case 32000:
		ret=ADTS_SAMPLE_INDEX_32000;
		break;
		case 24000:
		ret=ADTS_SAMPLE_INDEX_24000;
		break;
		case 22050:
		ret=ADTS_SAMPLE_INDEX_22050;
		break;
		case 16000:
		ret=ADTS_SAMPLE_INDEX_16000;
		break;
		case 12000:
		ret=ADTS_SAMPLE_INDEX_12000;
		break;
		case 11025:
		ret=ADTS_SAMPLE_INDEX_11025;
		break;
		case 8000:
		ret=ADTS_SAMPLE_INDEX_8000;
		break;
		case 7350:
		ret=ADTS_SAMPLE_INDEX_7350;
		break;
		default:
		ret=ADTS_SAMPLE_INDEX_44100;
		break;
	}
	return ret;
}

void audaac_rec_install_adts_header_variable (uint16  byte_num)
{
	//uint16  bit_index=0;

	adts_sample_index srate_enum;
	uint32  value;

	uint32   sample_index = samplerate;
	uint8   channel_config = channels;

	/* Store Sync word first */
	audaac_header[0] = 0xFF;
	audaac_header[1] = 0xF0;

	audaac_hdr_bit_index = 12;

	/* ID field, 1 bit */
	value = 1;
	audaac_rec_install_bits(audaac_header,
			  1,
			  value,
			  &(audaac_hdr_bit_index));

	/* Layer field, 2 bits */
	value = 0;
	audaac_rec_install_bits(audaac_header,
			  AACHDR_LAYER_SIZE,
			  value,
			  &(audaac_hdr_bit_index));

	/* Protection_absent field, 1 bit */
	value = 1;
	audaac_rec_install_bits(audaac_header,
			  AACHDR_CRC_SIZE,
			  value,
			  &(audaac_hdr_bit_index));

	/* profile_ObjectType field, 2 bit */
	value = 1;
	audaac_rec_install_bits(audaac_header,
			  AAC_PROFILE_SIZE,
			  value,
			  &(audaac_hdr_bit_index));

	/* sampling_frequency_index field, 4 bits */
	srate_enum = map_adts_sample_index(sample_index);
	audaac_rec_install_bits(audaac_header,
			  AAC_SAMPLING_FREQ_INDEX_SIZE,
			  (uint32)srate_enum,
			  &(audaac_hdr_bit_index));

	DEBUG_PRINT("Inside = %s \n",__FUNCTION__);
	DEBUG_PRINT("sample_index=%d; srate_enum = %d \n",sample_index,srate_enum);

	/* pravate_bit field, 1 bits */
	audaac_rec_install_bits(audaac_header,
			  1,
			  0,
			  &(audaac_hdr_bit_index));

	/* channel_configuration field, 3 bits */
	audaac_rec_install_bits(audaac_header,
			  3,
			  channel_config,
			  &(audaac_hdr_bit_index));


	/* original/copy field, 1 bits */
	audaac_rec_install_bits(audaac_header,
			  AAC_ORIGINAL_COPY_SIZE,
			  0,
			  &(audaac_hdr_bit_index));


	/* home field, 1 bits */
	audaac_rec_install_bits(audaac_header,
			  AAC_HOME_SIZE,
			  0,
			  &(audaac_hdr_bit_index));

	// bit_index = audaac_hdr_bit_index;
	// bit_index += 2;

	/* copyr. id. bit, 1 bits */
	audaac_rec_install_bits(audaac_header,
			  1,
			  0,
			  &(audaac_hdr_bit_index));

	/* copyr. id. start, 1 bits */
	audaac_rec_install_bits(audaac_header,
			  1,
			  0,
			  &(audaac_hdr_bit_index));

	/* aac_frame_length field, 13 bits */
	audaac_rec_install_bits(audaac_header,
			  AUDAAC_ADTS_FRAME_LENGTH_SIZE,
			  byte_num,
			  &audaac_hdr_bit_index);

	/* adts_buffer_fullness field, 11 bits */
	audaac_rec_install_bits(audaac_header,
			  11,
			  0x660,/*0x660 = CBR,0x7FF = VBR*/
			  &audaac_hdr_bit_index);

	/* number_of_raw_data_blocks_in_frame, 2 bits */
	audaac_rec_install_bits(audaac_header,
			  2,
			  0,
			  &audaac_hdr_bit_index);

} /* audaac_rec_install_adts_header_variable */
