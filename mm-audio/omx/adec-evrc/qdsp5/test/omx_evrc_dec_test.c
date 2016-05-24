/*
 * Copyright (c) 2010 Qualcomm Technologies, Inc.  All Rights Reserved.  
 * Qualcomm Technologies Proprietary and Confidential. 
 */

/*
    An Open max test application for EVRC component....
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <linux/ioctl.h>
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "QOMX_AudioExtensions.h"
#include "QOMX_AudioIndexExtensions.h"
#ifdef AUDIOV2
#include "control.h"
#endif
#include <signal.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <linux/msm_audio.h>

uint32_t samplerate = 8000;
uint32_t channels = 1;
uint32_t pcmplayback = 0;
uint32_t tunnel      = 0;
uint32_t filewrite   = 0;

#ifdef _DEBUG

#define DEBUG_PRINT(args...) printf("%s:%d ", __FUNCTION__, __LINE__); \
printf(args)

#define DEBUG_PRINT_ERROR(args...) printf("%s:%d ", __FUNCTION__, __LINE__); \
printf(args)

#else

#define DEBUG_PRINT
#define DEBUG_PRINT_ERROR

#endif



#define PCM_PLAYBACK /* To write the pcm decoded data to the msm_pcm device for playback*/

extern OMX_U8 frameFormat;

#ifdef PCM_PLAYBACK
  int                          m_pcmdrv_fd;
#endif  // PCM_PLAYBACK


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

pthread_mutex_t lock1;
pthread_mutexattr_t lock1_attr;

pthread_cond_t cond;
pthread_mutex_t elock;
pthread_cond_t econd;
pthread_cond_t fcond;
FILE * inputBufferFile;
FILE * outputBufferFile;
OMX_PARAM_PORTDEFINITIONTYPE inputportFmt;
OMX_PARAM_PORTDEFINITIONTYPE outputportFmt;
OMX_AUDIO_PARAM_EVRCTYPE evrcparam;
QOMX_AUDIO_STREAM_INFO_DATA streaminfoparam;
OMX_PORT_PARAM_TYPE portParam;
OMX_ERRORTYPE error;



/* http://ccrma.stanford.edu/courses/422/projects/WaveFormat/ */

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

static int bFileclose = 0;

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
int bOutputEosReached = 0;
int bInputEosReached = 0;
#ifdef AUDIOV2
unsigned short session_id;
unsigned short session_id_hpcm;
int device_id;
int control = 0;
const char *device="handset_rx";
int devmgr_fd;
#endif

static int etb_done = 0;

int bFlushing = false;
int bPause    = false;
const char *in_filename;
const char out_filename[512];
int chunksize =0;
OMX_U8* pBuffer_tmp = NULL;


int timeStampLfile = 0;
int timestampInterval = 100;

//* OMX Spec Version supported by the wrappers. Version = 1.1 */
const OMX_U32 CURRENT_OMX_SPEC_VERSION = 0x00000101;
OMX_COMPONENTTYPE* evrc_dec_handle = 0;

OMX_BUFFERHEADERTYPE  **pInputBufHdrs = NULL;
OMX_BUFFERHEADERTYPE  **pOutputBufHdrs = NULL;

/************************************************************************/
/*                GLOBAL FUNC DECL                        */
/************************************************************************/
int Init_Decoder(OMX_STRING audio_component);
int Play_Decoder();

OMX_STRING aud_comp;

/**************************************************************************/
/*                STATIC DECLARATIONS                       */
/**************************************************************************/

static int open_audio_file ();
static int Read_Buffer(OMX_BUFFERHEADERTYPE  *pBufHdr );
static OMX_ERRORTYPE Allocate_Buffer ( OMX_COMPONENTTYPE *evrc_dec_handle,
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
                           OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2,
                           OMX_IN OMX_PTR pEventData)
{
    DEBUG_PRINT("Function %s \n", __FUNCTION__);

    int bufCnt=0;

    if(hComponent == NULL)
    {
        pAppData = NULL;
        pEventData = NULL;
        DEBUG_PRINT("Returning OMX_ErrorBadParameter.\n");
        return OMX_ErrorBadParameter;
    }
    switch(eEvent) {
        case OMX_EventCmdComplete:
            DEBUG_PRINT("*********************************************\n");
            DEBUG_PRINT("\n OMX_EventCmdComplete \n");
            DEBUG_PRINT("*********************************************\n");
            if(OMX_CommandPortDisable == (OMX_COMMANDTYPE)nData1)
            {
                DEBUG_PRINT("******************************************\n");
                DEBUG_PRINT("Recieved DISABLE Event Command Complete[%d]\n",(int)nData2);
                DEBUG_PRINT("******************************************\n");
            }
            else if(OMX_CommandPortEnable == (OMX_COMMANDTYPE)nData1)
            {
                DEBUG_PRINT("*********************************************\n");
                DEBUG_PRINT("Recieved ENABLE Event Command Complete[%d]\n", (int)nData2);
                DEBUG_PRINT("*********************************************\n");
            }
            else if(OMX_CommandFlush== (OMX_COMMANDTYPE)nData1)
            {
                DEBUG_PRINT("*********************************************\n");
                DEBUG_PRINT("Recieved FLUSH Event Command Complete[%d]\n", (int)nData2);
                DEBUG_PRINT("*********************************************\n");
            }
            event_complete();
            break;
        case OMX_EventError:
            DEBUG_PRINT("*********************************************\n");
            DEBUG_PRINT("\n OMX_EventError \n");
            DEBUG_PRINT("*********************************************\n");
            if(OMX_ErrorInvalidState == (OMX_ERRORTYPE)nData1)
            {
               DEBUG_PRINT("\n OMX_ErrorInvalidState \n");
               for(bufCnt=0; bufCnt < input_buf_cnt; ++bufCnt)
               {
                  OMX_FreeBuffer(evrc_dec_handle, 0, pInputBufHdrs[bufCnt]);
               }
               if(tunnel == 0)
               {
                   for(bufCnt=0; bufCnt < output_buf_cnt; ++bufCnt)
                   {
                     OMX_FreeBuffer(evrc_dec_handle, 1, pOutputBufHdrs[bufCnt]);
                   }
               }

               DEBUG_PRINT("*********************************************\n");
               DEBUG_PRINT("\n Component Deinitialized \n");
               DEBUG_PRINT("*********************************************\n");
               exit(0);
            }
            else if(OMX_ErrorComponentSuspended == (OMX_ERRORTYPE)nData1)
            {
               DEBUG_PRINT("*********************************************\n");
               DEBUG_PRINT("\n Component Received Suspend Event \n");
               DEBUG_PRINT("*********************************************\n");
            }
            break;

        case OMX_EventPortSettingsChanged:
            DEBUG_PRINT("*********************************************\n");
            DEBUG_PRINT("\n OMX_EventPortSettingsChanged \n");
            DEBUG_PRINT("*********************************************\n");
            event_complete();
            break;
       case OMX_EventBufferFlag:
            DEBUG_PRINT("*********************************************\n");
            DEBUG_PRINT("\n OMX_Bufferflag \n");
            DEBUG_PRINT("*********************************************\n");
            event_complete();
            break;


       case OMX_EventComponentResumed:
           DEBUG_PRINT("*********************************************\n");
           DEBUG_PRINT("\n Component Received Suspend Event \n");
           DEBUG_PRINT("*********************************************\n");
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
   int i = 0;
   int bytes_writen = 0;
   static int count = 0;
   static int copy_done = 0;
   static int start_done = 0;
   static int length_filled = 0;
   static int spill_length = 0;
   static int pcm_buf_size = 4800;
   static int pcm_buf_count = 2;
   struct msm_audio_config drv_pcm_config;

    if(hComponent == NULL)
    {
        pAppData = NULL;
        DEBUG_PRINT("Returning OMX_ErrorBadParameter.\n");
        return OMX_ErrorBadParameter;
    }

    if((count == 0) && (pcmplayback))
   {
       DEBUG_PRINT(" open pcm device \n");
       m_pcmdrv_fd = open("/dev/msm_pcm_out", O_RDWR);
       if (m_pcmdrv_fd < 0)
       {
          DEBUG_PRINT("Cannot open audio device\n");
          return -1;
       }
       else
       {
          DEBUG_PRINT("Open pcm device successfull\n");
          DEBUG_PRINT("Configure Driver for PCM playback \n");
                  ioctl(m_pcmdrv_fd, AUDIO_GET_CONFIG, &drv_pcm_config);
              DEBUG_PRINT("drv_pcm_config.buffer_count %d \n", drv_pcm_config.buffer_count);
          DEBUG_PRINT("drv_pcm_config.buffer_size %d \n", drv_pcm_config.buffer_size);
                  drv_pcm_config.sample_rate = samplerate; //SAMPLE_RATE; //m_adec_param.nSampleRate;
          drv_pcm_config.channel_count = channels;  /* 1-> mono 2-> stereo*/
          ioctl(m_pcmdrv_fd, AUDIO_SET_CONFIG, &drv_pcm_config);
          DEBUG_PRINT("Configure Driver for PCM playback \n");
          ioctl(m_pcmdrv_fd, AUDIO_GET_CONFIG, &drv_pcm_config);
          DEBUG_PRINT("drv_pcm_config.buffer_count %d \n", drv_pcm_config.buffer_count);
          DEBUG_PRINT("drv_pcm_config.buffer_size %d \n", drv_pcm_config.buffer_size);
          pcm_buf_size = drv_pcm_config.buffer_size;
          pcm_buf_count = drv_pcm_config.buffer_count;
#ifdef AUDIOV2
          ioctl(m_pcmdrv_fd, AUDIO_GET_SESSION_ID, &session_id_hpcm);
          DEBUG_PRINT("session id 0x%4x \n", session_id_hpcm);
			if(devmgr_fd >= 0)
			{
				write_devctlcmd(devmgr_fd, "-cmd=register_session_rx -sid=",  session_id_hpcm);
			}
			else
			{
				if (msm_route_stream(1, session_id_hpcm,device_id, 1))
				{
				DEBUG_PRINT("could not set stream routing\n");
				return -1;
				}
			}
#endif

       }
       pBuffer_tmp= (OMX_U8*)malloc(pcm_buf_count*sizeof(OMX_U8)*pcm_buf_size);
       if (pBuffer_tmp == NULL)
       {
           return -1;
       }
       else
       {
           memset(pBuffer_tmp, 0, pcm_buf_count*pcm_buf_size);
       }
   }
   count++;
   DEBUG_PRINT(" FillBufferDone #%d size %d\n", count, (int)pBuffer->nFilledLen);

    if(bOutputEosReached)
    {
        return OMX_ErrorNone;
    }

    if((tunnel == 0) && (filewrite == 1))
    {
        bytes_writen =
        fwrite(pBuffer->pBuffer,1,pBuffer->nFilledLen,outputBufferFile);
        DEBUG_PRINT(" FillBufferDone size writen to file  %d\n",bytes_writen);
        totaldatalen += bytes_writen ;
    }

#ifdef PCM_PLAYBACK
    if(pcmplayback && pBuffer->nFilledLen)
{
        if(start_done == 0)
        {
            if((signed)(length_filled+pBuffer->nFilledLen)>=(pcm_buf_count*pcm_buf_size))
            {
                spill_length = (pBuffer->nFilledLen-(pcm_buf_count*pcm_buf_size)+length_filled);
                memcpy (pBuffer_tmp+length_filled, pBuffer->pBuffer, ((pcm_buf_count*pcm_buf_size)-length_filled));
                length_filled = (pcm_buf_count*pcm_buf_size);
                copy_done = 1;
            }
            else
            {
                memcpy (pBuffer_tmp+length_filled, pBuffer->pBuffer, pBuffer->nFilledLen);
                length_filled +=pBuffer->nFilledLen;
            }
            if (copy_done == 1)
        {
                   for (i=0; i<pcm_buf_count; i++)
            {
                    if (write(m_pcmdrv_fd, pBuffer_tmp+i*pcm_buf_size, pcm_buf_size ) != pcm_buf_size)
                    {
              DEBUG_PRINT("FillBufferDone: Write data to PCM failed\n");
                        return -1;
              }

           }
               DEBUG_PRINT("AUDIO_START called for PCM \n");
               ioctl(m_pcmdrv_fd, AUDIO_START, 0);
           if (spill_length != 0)
           {
                   if (write(m_pcmdrv_fd, pBuffer->pBuffer+((pBuffer->nFilledLen)-spill_length), spill_length) != spill_length)
                   {
                  DEBUG_PRINT("FillBufferDone: Write data to PCM failed\n");
                       return -1;
                   }
           }
               if (pBuffer_tmp)
               {
                   free(pBuffer_tmp);
               pBuffer_tmp =NULL;
               }
               copy_done = 0;
               start_done = 1;
            }
        }
        else
    {
            if (write(m_pcmdrv_fd, pBuffer->pBuffer, pBuffer->nFilledLen ) !=
                (signed)pBuffer->nFilledLen)
            {
        DEBUG_PRINT("FillBufferDone: Write data to PCM failed\n");
        return OMX_ErrorNone;
            }
    }

        DEBUG_PRINT(" FillBufferDone: writing data to pcm device for play succesfull \n");
}
#endif   // PCM_PLAYBACK


    if(pBuffer->nFlags != OMX_BUFFERFLAG_EOS)
    {
        DEBUG_PRINT(" FBD calling FTB");
        OMX_FillThisBuffer(hComponent,pBuffer);
    }
    else
    {
        DEBUG_PRINT(" FBD EOS REACHED...........\n");
        bOutputEosReached = true;
        return OMX_ErrorNone;
    }
    return OMX_ErrorNone;
}


OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_PTR pAppData,
                              OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    int readBytes =0;

    if(hComponent == NULL)
    {
        pAppData = NULL;
        DEBUG_PRINT("Returning OMX_ErrorBadParameter.\n");
        return OMX_ErrorBadParameter;
    }
    DEBUG_PRINT("\nFunction %s cnt[%d]\n", __FUNCTION__, ebd_cnt);
    ebd_cnt++;
    used_ip_buf_cnt--;

    pthread_mutex_lock(&lock1);
    if(!etb_done)
    {
        DEBUG_PRINT("\n*********************************************\n");
        DEBUG_PRINT("Wait till first set of buffers are given to component\n");
        DEBUG_PRINT("\n*********************************************\n");
        etb_done++;
        pthread_mutex_unlock(&lock1);
        wait_for_event();
    }
    else
    {
        pthread_mutex_unlock(&lock1);
    }

    if(bInputEosReached) {
        DEBUG_PRINT("\n*********************************************\n");
        DEBUG_PRINT("   EBD::EOS on input port\n ");
        DEBUG_PRINT("   TBD:::De Init the open max here....!!!\n");
        DEBUG_PRINT("*********************************************\n");
        return OMX_ErrorNone;
    }    else if (bFlushing == true) {
      if (used_ip_buf_cnt == 0) {
            //fseek(inputBufferFile, 0, 0);
        bFlushing = false;
      } else {
        DEBUG_PRINT("omx_evrc_adec_test: more buffer to come back\n");
        return OMX_ErrorNone;
      }
    }
    if((readBytes = Read_Buffer(pBuffer)) > 0) {
        pBuffer->nFilledLen = readBytes;
        used_ip_buf_cnt++;
        OMX_EmptyThisBuffer(hComponent,pBuffer);
    }
    else{
        pBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
        bInputEosReached = true;
        pBuffer->nFilledLen = 0;
        OMX_EmptyThisBuffer(hComponent,pBuffer);
        DEBUG_PRINT("******************************\n");
        DEBUG_PRINT("EBD--> EOS ENCOUNTERED..n");
        DEBUG_PRINT("******************************\n");
    }
    return OMX_ErrorNone;
}

void signal_handler(int sig_id) {

  /* Flush */


   if (sig_id == SIGUSR1) {
    DEBUG_PRINT("%s Initiate flushing\n", __FUNCTION__);
    bFlushing = true;
    OMX_SendCommand(evrc_dec_handle, OMX_CommandFlush, OMX_ALL, NULL);
  } else if (sig_id == SIGUSR2) {
    if (bPause == true) {
      DEBUG_PRINT("%s resume playback\n", __FUNCTION__);
      bPause = false;
      OMX_SendCommand(evrc_dec_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    } else {
      DEBUG_PRINT("%s pause playback\n", __FUNCTION__);
      bPause = true;
      OMX_SendCommand(evrc_dec_handle, OMX_CommandStateSet, OMX_StatePause, NULL);
    }
  }
}

int main(int argc, char **argv)
{
    int bufCnt=0;
    OMX_ERRORTYPE result;
    struct sigaction sa;

    struct wav_header hdr;
    int bytes_writen = 0;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &signal_handler;
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    pthread_cond_init(&cond, 0);
    pthread_mutex_init(&lock, 0);

    pthread_mutexattr_init(&lock1_attr);
    pthread_mutex_init(&lock1, &lock1_attr);

    if (argc >= 4)
    {
        in_filename = argv[1];
        pcmplayback = atoi(argv[2]);
        tunnel  = atoi(argv[3]);
        filewrite = atoi(argv[4]);
        strncpy(out_filename,argv[1],strlen(argv[1]));
        strcat(out_filename,".wav");

        if (tunnel == 1)
        {
            pcmplayback = 0; /* This feature holds good only for non tunnel mode*/
            filewrite = 0;  /* File write not supported in tunnel mode */
        }
    }
    else
    {
        DEBUG_PRINT(" invalid format: \n");
        DEBUG_PRINT("ex: ./mm-adec-omxevrc EVRCINPUTFILE PCMPLAYBACK TUNNEL FILEWRITE\n");
        DEBUG_PRINT( "PCMPLAYBACK = 1 (ENABLES PCM PLAYBACK IN NON TUNNEL MODE) \n");
        DEBUG_PRINT( "PCMPLAYBACK = 0 (DISABLES PCM PLAYBACK IN NON TUNNEL MODE) \n");
        DEBUG_PRINT( "TUNNEL = 1 (DECODED EVRC SAMPLES IS PLAYED BACK)\n");
        DEBUG_PRINT( "TUNNEL = 0 (DECODED EVRC SAMPLES IS LOOPED BACK TO THE USER APP)\n");
        DEBUG_PRINT( "FILEWRITE = 1 (ENABLES PCM FILEWRITE IN NON TUNNEL MODE) \n");
        DEBUG_PRINT( "FILEWRITE = 0 (DISABLES PCM FILEWRITE IN NON TUNNEL MODE) \n");
        return 0;
    }


    if(tunnel == 0)
    {
        aud_comp = "OMX.qcom.audio.decoder.evrchw";
    }
    else
    {
        aud_comp = "OMX.qcom.audio.decoder.tunneled.evrc";
    }

    DEBUG_PRINT(" OMX test app : aud_comp = %s\n",aud_comp);


    if(Init_Decoder(aud_comp)!= 0x00)
    {
        DEBUG_PRINT("Decoder Init failed\n");
        return -1;
    }

    if(Play_Decoder() != 0x00)
    {
        DEBUG_PRINT("Play_Decoder failed\n");
        return -1;
    }

    // Wait till EOS is reached...
    wait_for_event();
    if(bOutputEosReached || (tunnel && bInputEosReached))
    {
#ifdef PCM_PLAYBACK
        if(pcmplayback == 1)
        {
            fsync(m_pcmdrv_fd);
            sleep(1);
            ioctl(m_pcmdrv_fd, AUDIO_STOP, 0);
#ifdef AUDIOV2
			if(devmgr_fd >= 0)
			{
				write_devctlcmd(devmgr_fd, "-cmd=unregister_session_rx -sid=", session_id_hpcm);
			}
			else
			{
				if (msm_route_stream(1, session_id_hpcm, device_id, 0))
				{
					DEBUG_PRINT("\ncould not set stream routing\n");
				}
			}
#endif
            close(m_pcmdrv_fd);
            m_pcmdrv_fd = -1;
            DEBUG_PRINT(" PCM device closed succesfully \n");
        }
#endif // PCM_PLAYBACK

        if((tunnel == 0) && (filewrite == 1))
        {
            hdr.riff_id = ID_RIFF;
            hdr.riff_sz = 0;
            hdr.riff_fmt = ID_WAVE;
            hdr.fmt_id = ID_FMT;
            hdr.fmt_sz = 16;
            hdr.audio_format = FORMAT_PCM;
            hdr.num_channels = channels;
            hdr.sample_rate = samplerate;
            hdr.byte_rate = hdr.sample_rate * hdr.num_channels * 2;
            hdr.block_align = hdr.num_channels * 2;
            hdr.bits_per_sample = 16;
            hdr.data_id = ID_DATA;
            hdr.data_sz = 0;

            DEBUG_PRINT("output file closed and EOS reached total decoded data length %d\n",totaldatalen);
            hdr.data_sz = totaldatalen;
            hdr.riff_sz = totaldatalen + 8 + 16 + 8;
            fseek(outputBufferFile, 0L , SEEK_SET);
            bytes_writen = fwrite(&hdr,1,sizeof(hdr),outputBufferFile);
            if (bytes_writen <= 0)
            {
                DEBUG_PRINT("Invalid Wav header write failed\n");
            }
            bFileclose = 1;
            fclose(outputBufferFile);
        }
        DEBUG_PRINT("\nMoving the decoder to idle state \n");
        OMX_SendCommand(evrc_dec_handle,
            OMX_CommandStateSet, OMX_StateIdle,0);
        wait_for_event();
        DEBUG_PRINT("\nMoving the decoder to loaded state \n");
        OMX_SendCommand(evrc_dec_handle,
            OMX_CommandStateSet, OMX_StateLoaded,0);
        DEBUG_PRINT("\nFillBufferDone: Deallocating i/p buffers \n");
        for(bufCnt=0; bufCnt < input_buf_cnt; ++bufCnt)
        {
            OMX_FreeBuffer(evrc_dec_handle, 0, pInputBufHdrs[bufCnt]);
        }
        if(tunnel == 0)
        {
            DEBUG_PRINT("\nFillBufferDone: Deallocating o/p buffers \n");
            for(bufCnt=0; bufCnt < output_buf_cnt; ++bufCnt)
            {
                OMX_FreeBuffer(evrc_dec_handle, 1, pOutputBufHdrs[bufCnt]);
            }
        }
        ebd_cnt=0;
        bInputEosReached = false;
        wait_for_event();
        ebd_cnt=0;
        bOutputEosReached = false;
        result = OMX_FreeHandle(evrc_dec_handle);
        if (result != OMX_ErrorNone)
        {
            DEBUG_PRINT("\nOMX_FreeHandle error. Error code: %d\n", result);
        }
        evrc_dec_handle = NULL;
#ifdef AUDIOV2
		if(devmgr_fd >= 0)
		{
			write_devctlcmd(devmgr_fd, "-cmd=unregister_session_rx -sid=", session_id);
			close(devmgr_fd);
		}
		else
		{
			if (msm_route_stream(1,session_id,device_id, 0))
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

        /* Deinit OpenMAX */
        OMX_Deinit();
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&lock);

        pthread_mutexattr_destroy(&lock1_attr);
        pthread_mutex_destroy(&lock1);
        etb_done = 0;

        DEBUG_PRINT("*****************************************\n");
        DEBUG_PRINT("******...TEST COMPLETED...***************\n");
        DEBUG_PRINT("*****************************************\n");
    }
    return 0;
}


int Init_Decoder(OMX_STRING audio_component)
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE omxresult;
    typedef OMX_U8* OMX_U8_PTR;


    static OMX_CALLBACKTYPE call_back = {
        &EventHandler,&EmptyBufferDone,&FillBufferDone
    };

    DEBUG_PRINT("Inside Play_Decoder - channels = %d\n", channels);
    DEBUG_PRINT("Inside Play_Decoder - pcmplayback = %d\n", pcmplayback);
    DEBUG_PRINT("Inside Play_Decoder - tunnel = %d\n", tunnel);



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

    omxresult = OMX_GetHandle((OMX_HANDLETYPE*)(&evrc_dec_handle),
                        audio_component, NULL, &call_back);
    if (FAILED(omxresult)) {
        DEBUG_PRINT("\nFailed to Load the component OMX.qcom.audio.decoder.evrc\n");
    return -1;
    }
    else
    {
        DEBUG_PRINT("\nComponent is in LOADED state\n");
    }

    /* Get the port information */
    CONFIG_VERSION_SIZE(portParam);
    omxresult = OMX_GetParameter(evrc_dec_handle, OMX_IndexParamAudioInit,
                                (OMX_PTR)&portParam);

    if(FAILED(omxresult)) {
        DEBUG_PRINT("\nFailed to get Port Param\n");
    return -1;
    }
    else
    {
        DEBUG_PRINT("\nportParam.nPorts:%d\n", (int)portParam.nPorts);
        DEBUG_PRINT("\nportParam.nStartPortNumber:%d\n",
            (int)portParam.nStartPortNumber);
    }
    return 0;
}

int Play_Decoder()
{
    int i;
    int Size=0;
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE ret;
    OMX_U32 index;

    DEBUG_PRINT("sizeof[%d]\n", sizeof(OMX_BUFFERHEADERTYPE));

    /* open the i/p and o/p files based on the video file format passed */
    if(open_audio_file()) {
        DEBUG_PRINT("\n Returning -1");
    return -1;
    }
    /* Query the decoder input min buf requirements */
    CONFIG_VERSION_SIZE(inputportFmt);

    /* Port for which the Client needs to obtain info */
    inputportFmt.nPortIndex = portParam.nStartPortNumber;

    OMX_GetParameter(evrc_dec_handle,OMX_IndexParamPortDefinition,&inputportFmt);
    DEBUG_PRINT ("\nDec: Input Buffer Count %d\n", (int)inputportFmt.nBufferCountMin);
    DEBUG_PRINT ("\nDec: Input Buffer Size %d\n", (int)inputportFmt.nBufferSize);

    if(OMX_DirInput != inputportFmt.eDir) {
        DEBUG_PRINT ("\nDec: Expect Input Port\n");
    return -1;
    }
   inputportFmt.nBufferCountActual = inputportFmt.nBufferCountMin + 3;
   OMX_SetParameter(evrc_dec_handle,OMX_IndexParamPortDefinition,&inputportFmt);
   OMX_GetExtensionIndex(evrc_dec_handle,"OMX.Qualcomm.index.audio.sessionId",&index);
   OMX_GetParameter(evrc_dec_handle,index,&streaminfoparam);
#ifdef AUDIOV2
	session_id = streaminfoparam.sessionId;
	devmgr_fd = open("/data/omx_devmgr", O_WRONLY);
	if(devmgr_fd >= 0)
	{
		control = 0;
		write_devctlcmd(devmgr_fd, "-cmd=register_session_rx -sid=", session_id);
	}
	else
	{
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

		if (msm_route_stream(1,session_id,device_id, 1))
		{
			perror("could not set stream routing\n");
			return -1;
		}
	}
#endif

if(tunnel == 0)
{
    /* Query the decoder outport's min buf requirements */
    CONFIG_VERSION_SIZE(outputportFmt);
    /* Port for which the Client needs to obtain info */
    outputportFmt.nPortIndex = portParam.nStartPortNumber + 1;

    OMX_GetParameter(evrc_dec_handle,OMX_IndexParamPortDefinition,&outputportFmt);
    DEBUG_PRINT ("\nDec: Output Buffer Count %d\n", (int)outputportFmt.nBufferCountMin);
    DEBUG_PRINT ("\nDec: Output Buffer Size %d\n", (int)outputportFmt.nBufferSize);

    if(OMX_DirOutput != outputportFmt.eDir) {
        DEBUG_PRINT ("\nDec: Expect Output Port\n");
    return -1;
    }
    outputportFmt.nBufferCountActual = outputportFmt.nBufferCountMin+7;
    OMX_SetParameter(evrc_dec_handle,OMX_IndexParamPortDefinition,&outputportFmt);
}

    CONFIG_VERSION_SIZE(evrcparam);


    evrcparam.nPortIndex   =  0;
    evrcparam.nChannels    =  channels;  /* 1-> mono */
    OMX_SetParameter(evrc_dec_handle,OMX_IndexParamAudioEvrc,&evrcparam);


    DEBUG_PRINT ("\nOMX_SendCommand Decoder -> IDLE\n");
    OMX_SendCommand(evrc_dec_handle, OMX_CommandStateSet, OMX_StateIdle,0);
    /* wait_for_event(); should not wait here event complete status will
       not come until enough buffer are allocated */

    input_buf_cnt = 5;
    DEBUG_PRINT("Transition to Idle State succesful...\n");
    /* Allocate buffer on decoder's i/p port */
    error = Allocate_Buffer(evrc_dec_handle, &pInputBufHdrs, inputportFmt.nPortIndex,
                            input_buf_cnt, inputportFmt.nBufferSize);
    if (error != OMX_ErrorNone) {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Input buffer error\n");
    return -1;
    }
    else {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Input buffer success\n");
    }

if(tunnel == 0)
{
    output_buf_cnt = 9 ;
    /* Allocate buffer on decoder's O/Pp port */
    error = Allocate_Buffer(evrc_dec_handle, &pOutputBufHdrs, outputportFmt.nPortIndex,
                            output_buf_cnt, outputportFmt.nBufferSize);
    if (error != OMX_ErrorNone) {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Output buffer error\n");
    return -1;
    }
    else {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Output buffer success\n");
    }
}

    wait_for_event();

if (tunnel == 1)
{
    DEBUG_PRINT ("\nOMX_SendCommand to enable TUNNEL MODE during IDLE\n");
    OMX_SendCommand(evrc_dec_handle, OMX_CommandPortDisable,1,0);
    wait_for_event();
}

    DEBUG_PRINT ("\nOMX_SendCommand Decoder -> Executing\n");
    OMX_SendCommand(evrc_dec_handle, OMX_CommandStateSet, OMX_StateExecuting,0);
    wait_for_event();


if(tunnel == 0)
{
    DEBUG_PRINT(" Start sending OMX_FILLthisbuffer\n");

    for(i=0; i < output_buf_cnt; i++) {
        DEBUG_PRINT ("\nOMX_FillThisBuffer on output buf no.%d\n",i);
        pOutputBufHdrs[i]->nOutputPortIndex = 1;
        pOutputBufHdrs[i]->nFlags &= ~OMX_BUFFERFLAG_EOS;
        ret = OMX_FillThisBuffer(evrc_dec_handle, pOutputBufHdrs[i]);
        if (OMX_ErrorNone != ret) {
            DEBUG_PRINT("OMX_FillThisBuffer failed with result %d\n", ret);
    }
        else {
            DEBUG_PRINT("OMX_FillThisBuffer success!\n");
    }
    }
}

    DEBUG_PRINT(" Start sending OMX_emptythisbuffer\n");
    for (i = 0;i < input_buf_cnt;i++) {

        DEBUG_PRINT ("\nOMX_EmptyThisBuffer on Input buf no.%d\n",i);
        pInputBufHdrs[i]->nInputPortIndex = 0;
        Size = Read_Buffer(pInputBufHdrs[i]);
        if(Size <=0 ){
            DEBUG_PRINT("FILE completely read\n");
            bInputEosReached = true;
            pInputBufHdrs[i]->nFlags= OMX_BUFFERFLAG_EOS;
        }
        pInputBufHdrs[i]->nFilledLen = Size;
        pInputBufHdrs[i]->nInputPortIndex = 0;
        used_ip_buf_cnt++;
        ret = OMX_EmptyThisBuffer(evrc_dec_handle, pInputBufHdrs[i]);
        if (OMX_ErrorNone != ret) {
            DEBUG_PRINT("OMX_EmptyThisBuffer failed with result %d\n", ret);
        }
        else {
            DEBUG_PRINT("OMX_EmptyThisBuffer success!\n");
        }
        if(Size <=0 ){
            break;//eos reached
        }
    }

    pthread_mutex_lock(&lock1);
    if(etb_done)
    {
        DEBUG_PRINT("\n****************************\n");
        DEBUG_PRINT("Component is waiting for EBD to be released.\n");
        DEBUG_PRINT("\n****************************\n");
        event_complete();
    }
    else
    {
        DEBUG_PRINT("\n****************************\n");
        DEBUG_PRINT("EBD not yet happened ...\n");
        DEBUG_PRINT("\n****************************\n");
        etb_done++;
    }
    pthread_mutex_unlock(&lock1);

    return 0;
}



static OMX_ERRORTYPE Allocate_Buffer ( OMX_COMPONENTTYPE *avc_dec_handle,
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                       OMX_U32 nPortIndex,
                                       long bufCntMin, long bufSize)
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE error=OMX_ErrorNone;
    long bufCnt=0;

    if(avc_dec_handle == NULL)
    {
        DEBUG_PRINT("Returning OMX_ErrorBadParameter.\n");
        return OMX_ErrorBadParameter;
    }

    *pBufHdrs= (OMX_BUFFERHEADERTYPE **)
                   malloc(sizeof(OMX_BUFFERHEADERTYPE*)*bufCntMin);

    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) {
        DEBUG_PRINT("\n OMX_AllocateBuffer No %d \n", (int)bufCnt);
        error = OMX_AllocateBuffer(evrc_dec_handle, &((*pBufHdrs)[bufCnt]),
                                   nPortIndex, NULL, bufSize);
    }

    return error;
}




static int Read_Buffer (OMX_BUFFERHEADERTYPE  *pBufHdr )
{

    int bytes_read = 0;
    static int totalbytes_read =0;
    static int file_read_cmpl = false;

    DEBUG_PRINT ("Inside Read_Buffer\n");

    pBufHdr->nFilledLen = 0;

    if(file_read_cmpl) return 0;

    bytes_read = fread(pBufHdr->pBuffer, 1, pBufHdr->nAllocLen , inputBufferFile);
    totalbytes_read += bytes_read;
    pBufHdr->nFilledLen = bytes_read;
    if( totalbytes_read > chunksize)
    {
        bytes_read = chunksize - (totalbytes_read - bytes_read );
        pBufHdr->nTimeStamp = timeStampLfile;
        timeStampLfile += timestampInterval;
        file_read_cmpl = true;
        DEBUG_PRINT ("\n***************************\n");
        DEBUG_PRINT ("\nBytes read > chunk size\n");
        DEBUG_PRINT ("totalbytes_read=%d chunk size=%d\n",totalbytes_read,chunksize);
        DEBUG_PRINT ("\n***************************\n");
    }
    else if(bytes_read == 0)
    {
        pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
        pBufHdr->nTimeStamp = timeStampLfile;
        timeStampLfile += timestampInterval;
        DEBUG_PRINT ("\n***************************\n");
        DEBUG_PRINT ("\nBytes read zero\n");
        DEBUG_PRINT ("\n***************************\n");
    }
    else
    {
        pBufHdr->nTimeStamp = timeStampLfile;
        timeStampLfile += timestampInterval;
        DEBUG_PRINT ("\nBytes read is Non zero\n");
    }

    return bytes_read;

}


static int open_audio_file ()
{
    int error_code = 0;
    struct wav_header hdr;
    int header_len = 0;
    memset(&hdr,0,sizeof(hdr));
    hdr.riff_id = ID_RIFF;
    hdr.riff_sz = 0;
    hdr.riff_fmt = ID_WAVE;
    hdr.fmt_id = ID_FMT;
    hdr.fmt_sz = 16;
    hdr.audio_format = FORMAT_PCM;
    hdr.num_channels = channels;
    hdr.sample_rate = samplerate;
    hdr.byte_rate = hdr.sample_rate * hdr.num_channels * 2;
    hdr.block_align = hdr.num_channels * 2;
    hdr.bits_per_sample = 16;
    hdr.data_id = ID_DATA;
    hdr.data_sz = 0;
    OMX_U8  chunk_id[] = {'d', 'a', 't', 'a'};
    OMX_U8  parsebuffer[4];
    OMX_U8  parsefmtbuffer[10];
    int readerror = 0;
    int Size = 0;
    OMX_U8  parsechunksize[4];
    const OMX_U8 qcpsup_evrc_guid[] = {
        0x8D,0xD4,0x89,0xE6,
        0x76,0x90,
        0xB5,0x46,
        0x91,0xEF,0x73,0x6A,0x51,0x00,0xCE,0xB4
    };

    DEBUG_PRINT("Inside %s filename=%s\n", __FUNCTION__, in_filename);
    inputBufferFile = fopen (in_filename, "rb");
    if (inputBufferFile == NULL) {
        DEBUG_PRINT("\ni/p file %s could NOT be opened\n",
                                         in_filename);
        error_code = -1;
    }

    /*Qcp parser code*/
   fseek(inputBufferFile, 22 , SEEK_CUR);
   Size = fread(parsefmtbuffer, 1, 10, inputBufferFile);
        if(Size <=0 ){
              DEBUG_PRINT("NO DATA READ Parser failed\n");
              error_code = -1;
              readerror  = 1;
            }
    if(strncmp((char *) parsefmtbuffer, (char *) qcpsup_evrc_guid, 10) == 0)
    {
      DEBUG_PRINT("EVRC file format in qcp recognised\n");
    }
    else
    {
      DEBUG_PRINT("unknownsupported file format in qcp recognised\n");
      error_code = -1;
    }
    while(!readerror){
        Size = fread(parsebuffer, 1, 4, inputBufferFile);
        if(Size <=0 ){
              DEBUG_PRINT("NO DATA READ Parser failed\n");
              error_code = -1;
              readerror  = 1;
              break;
            }

        if(strncmp((char *) chunk_id, (char *) parsebuffer, 4) == 0)
        {
          DEBUG_PRINT("Parser frame header success\n");
          Size = fread(parsechunksize, 1, 4 , inputBufferFile);
          chunksize = (parsechunksize[3] << 24) | (parsechunksize[2] << 16) |
               (parsechunksize[1] << 8)  | (parsechunksize[0]);
          DEBUG_PRINT("Data chunksize = %d\n",chunksize);
          break;
        }
        else
        {
            fseek(inputBufferFile, -3 , SEEK_CUR);

        }
    }

    if((tunnel == 0) && (filewrite == 1))
    {
        DEBUG_PRINT("output file is opened\n");
        outputBufferFile = fopen(out_filename,"wb");
        if (outputBufferFile == NULL)
        {
            DEBUG_PRINT("\no/p file %s could NOT be opened\n",
                                             out_filename);
            error_code = -1;
        }

        header_len = fwrite(&hdr,1,sizeof(hdr),outputBufferFile);


        if (header_len <= 0)
        {
            DEBUG_PRINT("Invalid Wav header \n");
        }
        DEBUG_PRINT(" Length og wav header is %d \n",header_len );
    }
    return error_code;
}

void write_devctlcmd(int fd, const void *buf, int param){
	int nbytes, nbytesWritten;
	char cmdstr[128];
	snprintf(cmdstr, 128, "%s%d\n", buf, param);
	nbytes = strlen(cmdstr);
	nbytesWritten = write(fd, cmdstr, nbytes);

	if(nbytes != nbytesWritten)
		printf("Failed to write string \"%s\" to omx_devmgr\n");
}

