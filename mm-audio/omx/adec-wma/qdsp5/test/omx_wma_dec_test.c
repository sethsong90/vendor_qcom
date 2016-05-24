/*
 * Copyright (c) 2010 Qualcomm Technologies, Inc.  All Rights Reserved.  
 * Qualcomm Technologies Proprietary and Confidential. 
 */


/*
An Open max test application for WMA component....
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
#include <signal.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <linux/msm_audio.h>
#include "QOMX_AudioExtensions.h"
#include "QOMX_AudioIndexExtensions.h"

#ifdef AUDIOV2
#include "control.h"
#endif
#ifdef AUDIOV2
unsigned short session_id;
unsigned short session_id_hpcm;
int device_id;
int control = 0;
const char *device="handset_rx";
int devmgr_fd;
#endif
uint32_t flushinprogress = 0;

uint32_t samplerate= 0;
uint32_t channels = 0;
uint32_t BitRate= 0;
uint32_t BlockAlign= 0;
uint32_t virtual_chunk_len = 0;
uint32_t EncodeOptions=0;
uint32_t packet_size  = 0;
uint32_t packet_padding_size = 0;
uint32_t total_payload_data = 0;
uint32_t version = 0;
uint32_t bitspersample = 0;
uint32_t formattag = 0;
uint32_t advencopt = 0;
uint32_t advencopt2 = 0;
uint32_t tunnel      = 0;
uint32_t filewrite   = 0;
int start_done = 0;

#define DEBUG_PRINT(args...) printf("%s:%d ", __FUNCTION__, __LINE__); \
    printf(args)

#define DEBUG_PRINT_ERROR(args...) printf("%s:%d ", __FUNCTION__, __LINE__); \
    printf(args)
int                          m_pcmdrv_fd =-1;


/************************************************************************/
/*                              #DEFINES                                */
/************************************************************************/
#define false 0
#define true 1

#define CONFIG_VERSION_SIZE(param) \
    param.nVersion.nVersion = CURRENT_OMX_SPEC_VERSION;\
    param.nSize = sizeof(param);

#define FAILED(result) (result != OMX_ErrorNone)

#define SUCCEEDED(result) (result == OMX_ErrorNone)
#define DATA_HEADERSIZE 50
#define WMAPRO 1
/************************************************************************/
/*                              GLOBAL DECLARATIONS                     */
/************************************************************************/

pthread_mutex_t lock;
pthread_mutex_t lock1;
pthread_mutex_t dlock;
pthread_mutexattr_t lock1_attr;
pthread_cond_t cond;
pthread_cond_t dcond;
pthread_mutex_t elock;
pthread_cond_t econd;
pthread_cond_t fcond;
FILE * inputBufferFile;
FILE * outputBufferFile;
OMX_PARAM_PORTDEFINITIONTYPE inputportFmt;
OMX_PARAM_PORTDEFINITIONTYPE outputportFmt;
OMX_AUDIO_PARAM_WMATYPE wmaparam;
QOMX_AUDIO_PARAM_WMA10PROTYPE wmaparam10Pro;
OMX_PORT_PARAM_TYPE portParam;
OMX_ERRORTYPE error;



/* http://ccrma.stanford.edu/courses/422/projects/WaveFormat/ */

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

static int bFileclose = 0;
int bReconfigureOutputPort = 0;

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
QOMX_AUDIO_STREAM_INFO_DATA streaminfoparam;
/************************************************************************/
/*                          GLOBAL INIT                                 */
/************************************************************************/

int input_buf_cnt = 0;
int output_buf_cnt = 0;
int used_ip_buf_cnt = 0;
volatile int event_is_done = 0;
volatile int disable_event_is_done = 0;
volatile int ebd_event_is_done = 0;
volatile int fbd_event_is_done = 0;
int ebd_cnt;
int fbd_cnt;
int bOutputEosReached = 0;
int bInputEosReached = 0;
static int etb_done = 0;
int bFlushing = false;
int bPause    = false;
const char *in_filename;
const char *out_filename;
int chunksize =0;
OMX_U8* pBuffer_tmp = NULL;



int timeStampLfile = 0;
int timestampInterval = 100;


//* OMX Spec Version supported by the wrappers. Version = 1.1 */
const OMX_U32 CURRENT_OMX_SPEC_VERSION = 0x00000101;
OMX_COMPONENTTYPE* wma_dec_handle = 0;

OMX_BUFFERHEADERTYPE  **pInputBufHdrs = NULL;
OMX_BUFFERHEADERTYPE  **pOutputBufHdrs = NULL;

/************************************************************************/
/*                              GLOBAL FUNC DECL                        */
/************************************************************************/
int Init_Decoder(OMX_STRING audio_component);
int Play_Decoder();
void process_portreconfig();

OMX_STRING aud_comp;

/**************************************************************************/
/*                              STATIC DECLARATIONS                       */
/**************************************************************************/

static int open_audio_file ();
static void write_devctlcmd(int fd, const void *buf, int param);
static int Read_Buffer(OMX_BUFFERHEADERTYPE  *pBufHdr );
static OMX_ERRORTYPE Allocate_Buffer (OMX_BUFFERHEADERTYPE  ***pBufHdrs,
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

void wait_for_disable_event(void)
{
    pthread_mutex_lock(&dlock);
    DEBUG_PRINT("%s: disable_event_is_done=%d", __FUNCTION__, disable_event_is_done);
    while (disable_event_is_done == 0) {
        pthread_cond_wait(&dcond, &dlock);
    }
    disable_event_is_done = 0;
    pthread_mutex_unlock(&dlock);
}

void event_disable_complete(void )
{
    pthread_mutex_lock(&dlock);
    if (disable_event_is_done == 0) {
        disable_event_is_done = 1;
        pthread_cond_broadcast(&dcond);
    }
    pthread_mutex_unlock(&dlock);
}

OMX_ERRORTYPE EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                           OMX_IN OMX_PTR pAppData,
                           OMX_IN OMX_EVENTTYPE eEvent,
                           OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2,
                           OMX_IN OMX_PTR pEventData)
{
    int bufCnt=0;
    DEBUG_PRINT("Function %s \n", __FUNCTION__);

    if(hComponent == NULL)
    {
        pAppData = NULL;
        eEvent = OMX_EventMax;
        pEventData = NULL;
    }
    switch(eEvent)
    {
    case OMX_EventCmdComplete:
        DEBUG_PRINT("*********************************************\n");
        DEBUG_PRINT("\n OMX_EventCmdComplete \n");
        DEBUG_PRINT("*********************************************\n");
        if(OMX_CommandPortDisable == (OMX_COMMANDTYPE)nData1)
        {
            DEBUG_PRINT("******************************************\n");
            DEBUG_PRINT("Recieved DISABLE Event Command Complete[%d]\n", (int)nData2);
            DEBUG_PRINT("******************************************\n");
	    event_disable_complete();
	    break;
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
                OMX_FreeBuffer(wma_dec_handle, 0, pInputBufHdrs[bufCnt]);
            }
            if(tunnel == 0)
            {
                for(bufCnt=0; bufCnt < output_buf_cnt; ++bufCnt)
                {
                     OMX_FreeBuffer(wma_dec_handle, 1, pOutputBufHdrs[bufCnt]);
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
        if(tunnel == 0)
        {
                bReconfigureOutputPort = 1;
        DEBUG_PRINT("*********************************************\n");
        DEBUG_PRINT("\n OMX_EventPortSettingsChanged \n");
        DEBUG_PRINT("*********************************************\n");
        event_complete();
        }
        break;
    case OMX_EventBufferFlag:
        DEBUG_PRINT("*********************************************\n");
        DEBUG_PRINT("\n OMX_Bufferflag \n");
        DEBUG_PRINT("*********************************************\n");
        if(tunnel)
        {
            bInputEosReached = true;
        }
        else
        {
                bOutputEosReached = true;
        }
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
   pAppData = NULL;
   static int copy_done = 0;
   static int length_filled = 0;
   static int spill_length = 0;
   static int pcm_buf_size = 4800;
   static int pcm_buf_count = 2;
   struct msm_audio_config drv_pcm_config;
   if(flushinprogress == 1)
   {
       DEBUG_PRINT(" FillBufferDone: flush is in progress so hold the buffers\n");
       return OMX_ErrorNone;
   }
   fbd_cnt++;
   if(count == 0 && !filewrite)
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
          count++;
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
    DEBUG_PRINT(" FillBufferDone size %d\n",(int)pBuffer->nFilledLen);
    if(bOutputEosReached)
    {
        return OMX_ErrorNone;
    }

    if( (filewrite == 1))
    {
        bytes_writen =
        fwrite(pBuffer->pBuffer,1,pBuffer->nFilledLen,outputBufferFile);
        totaldatalen += bytes_writen ;
    }
    if(!filewrite)
    {
    if(pBuffer->nFilledLen)
        {
            if(start_done == 0)
            {
                if((length_filled+(signed)pBuffer->nFilledLen)>=(signed)(pcm_buf_count*pcm_buf_size))
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
         }
     }


    if(pBuffer->nFlags != OMX_BUFFERFLAG_EOS)
    {
        OMX_FillThisBuffer(hComponent,pBuffer);
    }
    else
    {
        DEBUG_PRINT(" FBD EOS REACHED...........\n");
        bOutputEosReached = true;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_PTR pAppData,
                              OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    int readBytes =0;
    pBuffer->nFlags &= ~ OMX_BUFFERFLAG_EOS;
    pAppData = NULL;

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

    if(bInputEosReached)
    {
        DEBUG_PRINT("\n*********************************************\n");
        DEBUG_PRINT("   EBD::EOS on input port\n ");
        DEBUG_PRINT("   TBD:::De Init the open max here....!!!\n");
        DEBUG_PRINT("*********************************************\n");
        return OMX_ErrorNone;
    }
    else if (bFlushing == true)
    {
        DEBUG_PRINT("omx_wma_adec_test: bFlushing is set to TRUE used_ip_buf_cnt=%d\n",used_ip_buf_cnt);
        if (used_ip_buf_cnt == 0) {
            fseek(inputBufferFile, 0, 0);
            bFlushing = false;
        } else {
            DEBUG_PRINT("omx_wma_adec_test: more buffer to come back\n");
            return OMX_ErrorNone;
        }
    }
    if((readBytes = Read_Buffer(pBuffer)) > 0) {
        pBuffer->nFilledLen = readBytes;
        used_ip_buf_cnt++;
        DEBUG_PRINT("OMX_EmptyThisBuffer:length %d \n",(signed)pBuffer->nFilledLen);
        OMX_EmptyThisBuffer(hComponent,pBuffer);
    }
    else{
        pBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
        bInputEosReached = true;
        pBuffer->nFilledLen = 0;
        OMX_EmptyThisBuffer(hComponent,pBuffer);
        DEBUG_PRINT("EBD..Either EOS or Some Error while reading file\n");
    }
    return OMX_ErrorNone;
}

void signal_handler(int sig_id) {

    /* Flush */

    if (sig_id == SIGUSR1) {
        DEBUG_PRINT("%s Initiate flushing\n", __FUNCTION__);
        bFlushing = true;
        OMX_SendCommand(wma_dec_handle, OMX_CommandFlush, OMX_ALL, NULL);
    } else if (sig_id == SIGUSR2) {
        if (bPause == true) {
            DEBUG_PRINT("%s resume playback\n", __FUNCTION__);
            bPause = false;
            OMX_SendCommand(wma_dec_handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);
        } else {
            DEBUG_PRINT("%s pause playback\n", __FUNCTION__);
            bPause = true;
            OMX_SendCommand(wma_dec_handle, OMX_CommandStateSet, OMX_StatePause, NULL);
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

    pthread_cond_init(&dcond, 0);
    pthread_mutex_init(&dlock, 0);


    if (argc >= 18)
    {
        in_filename = argv[1];
        samplerate= atoi(argv[2]);
        channels = atoi(argv[3]);
        BitRate= atoi(argv[4]);
        BlockAlign= atoi(argv[5]);
        virtual_chunk_len = atoi(argv[6]);
        EncodeOptions=atoi(argv[7]);
        packet_size  = atoi(argv[8]);
        packet_padding_size = atoi(argv[9]);
        total_payload_data = atoi(argv[10]);
        version = atoi(argv[11]);
        bitspersample = atoi(argv[12]);
        formattag = atoi(argv[13]);
        advencopt = atoi(argv[14]);
        advencopt2 = atoi(argv[15]);
        tunnel  = atoi(argv[16]);
        filewrite = atoi(argv[17]);
        out_filename = argv[18];
        if (tunnel == 1)
        {
            filewrite = 0;  /* File write not supported in tunnel mode */
        }
    }
    else
    {
        DEBUG_PRINT(" invalid format: \n");
        DEBUG_PRINT("ex: ./mm-adec-omxwma INPUTFILE SAMPLERATE CHANNELS BITRATE \
        BLOCKALIGN VIRTUALCHUNKLENGTH ENCODEOPT PACKETSIZE \
        PACKETPADDINGSIZE TOTALPAYLOADDATA \
        VERSION BITSPERSAMPLE FORMATTAG ADVENCOPT ADVENOPT2 \
        TUNNEL FILEWRITE OUTFILE\n");
        DEBUG_PRINT( " VERSION = 1 for WMAPRO,0 for WMA \n");
        DEBUG_PRINT( "TUNNEL = 1 (PLAYBACK IN TUNNELED MODE)\n");
        DEBUG_PRINT( "TUNNEL = 0 (PLAYBACK IN NONTUNNELED MODE)\n");
        DEBUG_PRINT( "FILEWRITE = 1 (ENABLES PCM FILEWRITE IN NONTUNNELED MODE ONLY)\n");

        return 0;
    }

    if(version == WMAPRO)
    {
        if(tunnel == 0)
        {
            aud_comp = "OMX.qcom.audio.decoder.wma10Pro";
        }
        else
        {
            aud_comp = "OMX.qcom.audio.decoder.tunneled.wma10Pro";
        }
     }
     else
     {
    if(tunnel == 0)
        {
            aud_comp = "OMX.qcom.audio.decoder.wma";
        }
        else
        {
            aud_comp = "OMX.qcom.audio.decoder.tunneled.wma";
        }
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
   printf("before wait_for_event\n");
   if(bReconfigureOutputPort)
   {
    wait_for_event();
   }
    if(bOutputEosReached || (tunnel && bInputEosReached))
    {

        if(tunnel == 0 || !filewrite)
        {
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
            if(m_pcmdrv_fd >= 0)
            {
                close(m_pcmdrv_fd);
                m_pcmdrv_fd = -1;
                DEBUG_PRINT(" PCM device closed succesfully \n");
            }
            else
            {
                DEBUG_PRINT(" PCM device close failure \n");
            }
        }


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
        OMX_SendCommand(wma_dec_handle,
            OMX_CommandStateSet, OMX_StateIdle,0);
        wait_for_event();
        DEBUG_PRINT("\nMoving the decoder to loaded state \n");
        OMX_SendCommand(wma_dec_handle,
            OMX_CommandStateSet, OMX_StateLoaded,0);
        DEBUG_PRINT("\nFillBufferDone: Deallocating i/p buffers \n");
        for(bufCnt=0; bufCnt < input_buf_cnt; ++bufCnt)
        {
            OMX_FreeBuffer(wma_dec_handle, 0, pInputBufHdrs[bufCnt]);
        }
        if(tunnel == 0)
        {
            DEBUG_PRINT("\nFillBufferDone: Deallocating o/p buffers \n");
            for(bufCnt=0; bufCnt < output_buf_cnt; ++bufCnt)
            {
                OMX_FreeBuffer(wma_dec_handle, 1, pOutputBufHdrs[bufCnt]);
            }
        }
        DEBUG_PRINT( "TOTAL EBD = %d\n TOTAL FBD = %d\n", ebd_cnt,fbd_cnt);
        ebd_cnt=0;
        fbd_cnt=0;
        total_payload_data = 0;
        bInputEosReached = false;
        wait_for_event();
        bOutputEosReached = false;
        result = OMX_FreeHandle(wma_dec_handle);
        if (result != OMX_ErrorNone)
        {
            DEBUG_PRINT("\nOMX_FreeHandle error. Error code: %d\n", result);
        }
        wma_dec_handle = NULL;
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
        bReconfigureOutputPort = 0;
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




    DEBUG_PRINT("Inside Play_Decoder - tunnel = %d\n", tunnel);
    DEBUG_PRINT("Inside Play_Decoder - filewrite = %d\n", filewrite);



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


    omxresult = OMX_GetHandle((OMX_HANDLETYPE*)(&wma_dec_handle),
        audio_component, NULL, &call_back);
    if (FAILED(omxresult)) {
        DEBUG_PRINT("\nFailed to Load the component = %s\n",audio_component);
        return -1;
    }
    else
    {
        DEBUG_PRINT("\nComponent is in LOADED state\n");
    }

    /* Get the port information */
    CONFIG_VERSION_SIZE(portParam);
    omxresult = OMX_GetParameter(wma_dec_handle, OMX_IndexParamAudioInit,
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
    OMX_INDEXTYPE index;
    DEBUG_PRINT("sizeof[%d]\n", sizeof(OMX_BUFFERHEADERTYPE));

    /* open the i/p and o/p files based on the video file format passed */
    if(open_audio_file()) {
        DEBUG_PRINT("\n open_audio_file returning -1");
        return -1;
    }
    /* Query the decoder input min buf requirements */
    CONFIG_VERSION_SIZE(inputportFmt);

    /* Port for which the Client needs to obtain info */
    inputportFmt.nPortIndex = portParam.nStartPortNumber;

    OMX_GetParameter(wma_dec_handle,OMX_IndexParamPortDefinition,&inputportFmt);
    DEBUG_PRINT ("\nDec: Input Buffer Count %d\n", (int)inputportFmt.nBufferCountMin);
    DEBUG_PRINT ("\nDec: Input Buffer Size %d\n", (int)inputportFmt.nBufferSize);

    if(OMX_DirInput != inputportFmt.eDir) {
        DEBUG_PRINT ("\nDec: Expect Input Port\n");
        return -1;
    }
    inputportFmt.nBufferSize = virtual_chunk_len;
    inputportFmt.nBufferCountActual = inputportFmt.nBufferCountMin ;
    OMX_SetParameter(wma_dec_handle,OMX_IndexParamPortDefinition,&inputportFmt);


    if(tunnel == 0)
    {
        /* Query the decoder outport's min buf requirements */
        CONFIG_VERSION_SIZE(outputportFmt);
        /* Port for which the Client needs to obtain info */
        outputportFmt.nPortIndex = portParam.nStartPortNumber + 1;

        OMX_GetParameter(wma_dec_handle,OMX_IndexParamPortDefinition,&outputportFmt);
        DEBUG_PRINT ("\nDec: Output Buffer Count %d\n", (int)outputportFmt.nBufferCountMin);
        DEBUG_PRINT ("\nDec: Output Buffer Size %d\n", (int)outputportFmt.nBufferSize);

        if(OMX_DirOutput != outputportFmt.eDir) {
            DEBUG_PRINT ("\nDec: Expect Output Port\n");
            return -1;

        }

        outputportFmt.nBufferSize = 8192;
        outputportFmt.nBufferCountActual = outputportFmt.nBufferCountMin;
        OMX_SetParameter(wma_dec_handle,OMX_IndexParamPortDefinition,&outputportFmt);

    }
    OMX_GetExtensionIndex(wma_dec_handle,"OMX.Qualcomm.index.audio.sessionId",&index);
    OMX_GetParameter(wma_dec_handle,index,&streaminfoparam);
#ifdef AUDIOV2
    session_id = streaminfoparam.sessionId;
	int devmgr_fd = open("/data/omx_devmgr", O_WRONLY);
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
    if(version == WMAPRO)
    {
        OMX_INDEXTYPE index;
        OMX_GetExtensionIndex(wma_dec_handle,"OMX.Qualcomm.index.audio.wma10Pro",&index);
        OMX_GetParameter(wma_dec_handle,index,&wmaparam10Pro);
        CONFIG_VERSION_SIZE(wmaparam10Pro);
        wmaparam10Pro.nPortIndex   =  0;
        wmaparam10Pro.nChannels    =  channels;
        wmaparam10Pro.nSamplingRate = samplerate;
        wmaparam10Pro.nEncodeOptions = EncodeOptions;
        wmaparam10Pro.nBlockAlign = BlockAlign;
        wmaparam10Pro.nBitRate    = BitRate;
        wmaparam10Pro.formatTag = formattag;
        wmaparam10Pro.advancedEncodeOpt = advencopt;
        wmaparam10Pro.advancedEncodeOpt2 = advencopt2;
        wmaparam10Pro.validBitsPerSample = bitspersample ;
        wmaparam10Pro.nVirtualPktSize = virtual_chunk_len;
        OMX_SetParameter(wma_dec_handle,index,&wmaparam10Pro);
    }
    else
    {
        CONFIG_VERSION_SIZE(wmaparam);

        wmaparam.nPortIndex   =  0;
        wmaparam.nChannels    =  channels;  /* 1-> mono */
        wmaparam.nSamplingRate = samplerate;
        wmaparam.nEncodeOptions = EncodeOptions;
        wmaparam.nBlockAlign = BlockAlign;
        wmaparam.nBitRate    = BitRate;
        OMX_SetParameter(wma_dec_handle,OMX_IndexParamAudioWma,&wmaparam);
    }
    DEBUG_PRINT ("\nOMX_SendCommand Decoder -> IDLE\n");
    OMX_SendCommand(wma_dec_handle, OMX_CommandStateSet, OMX_StateIdle,0);
    /* wait_for_event(); should not wait here event complete status will
    not come until enough buffer are allocated */

    input_buf_cnt = inputportFmt.nBufferCountMin;
    DEBUG_PRINT("Transition to Idle State succesful...\n");
    /* Allocate buffer on decoder's i/p port */
    error = Allocate_Buffer(&pInputBufHdrs, inputportFmt.nPortIndex,
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
        output_buf_cnt = outputportFmt.nBufferCountMin;
        /* Allocate buffer on decoder's O/Pp port */
        error = Allocate_Buffer(&pOutputBufHdrs, outputportFmt.nPortIndex,
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
        OMX_SendCommand(wma_dec_handle, OMX_CommandPortDisable,1,0);
        wait_for_disable_event();
    }

    DEBUG_PRINT ("\nOMX_SendCommand Decoder -> Executing\n");
    OMX_SendCommand(wma_dec_handle, OMX_CommandStateSet, OMX_StateExecuting,0);
    wait_for_event();


    if(tunnel == 0)
    {
        DEBUG_PRINT(" Start sending OMX_FILLthisbuffer\n");

        for(i=0; i < output_buf_cnt; i++) {
            DEBUG_PRINT ("\nOMX_FillThisBuffer on output buf no.%d\n",i);
            pOutputBufHdrs[i]->nOutputPortIndex = 1;
            pOutputBufHdrs[i]->nFlags &= ~OMX_BUFFERFLAG_EOS;
            ret = OMX_FillThisBuffer(wma_dec_handle, pOutputBufHdrs[i]);
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
            DEBUG_PRINT("NO DATA READ\n");
          bInputEosReached = true;
          pInputBufHdrs[i]->nFlags= OMX_BUFFERFLAG_EOS;
        }
        pInputBufHdrs[i]->nFilledLen = Size;
        pInputBufHdrs[i]->nInputPortIndex = 0;
        used_ip_buf_cnt++;
        DEBUG_PRINT("OMX_EmptyThisBuffer:length %d \n",(signed)pInputBufHdrs[i]->nFilledLen);
        ret = OMX_EmptyThisBuffer(wma_dec_handle, pInputBufHdrs[i]);
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
        DEBUG_PRINT("Component is waiting for EBD to be releases, BC signal\n");
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
    while(1)
    {
	    printf("wait for port settings changed event or end of stream\n");
	    wait_for_event();
	    if (bOutputEosReached || (tunnel && bInputEosReached)) {
		    bReconfigureOutputPort = 0;
		    printf("bOutputEosReached || (tunnel && bInputEosReached breaking\n");
		    break;
	    } else {
		    printf("before process_portreconfig call\n");
		    if(tunnel == 0 && bReconfigureOutputPort)
			    process_portreconfig();
	    }
    }
    return 0;
}

static OMX_ERRORTYPE Allocate_Buffer (OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                      OMX_U32 nPortIndex,
                                      long bufCntMin, long bufSize)
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE error=OMX_ErrorNone;
    long bufCnt=0;

    *pBufHdrs= (OMX_BUFFERHEADERTYPE **)
        malloc(sizeof(OMX_BUFFERHEADERTYPE*)*bufCntMin);

    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) {
        DEBUG_PRINT("\n OMX_AllocateBuffer No %d \n", (int)bufCnt);
        error = OMX_AllocateBuffer(wma_dec_handle, &((*pBufHdrs)[bufCnt]),
            nPortIndex, NULL, bufSize);
    }

    return error;
}

static int Read_Buffer (OMX_BUFFERHEADERTYPE  *pBufHdr )
{
    int bytes_read = 0;
    int packet_header_size = 0;
    static int total_bytes_read=0;
    static int num_packets =0;
    pBufHdr->nFilledLen = 0;
    /*Skipping packet header*/
    packet_header_size = packet_size - virtual_chunk_len - packet_padding_size;
    fseek(inputBufferFile, packet_header_size, SEEK_CUR);
    bytes_read = fread(pBufHdr->pBuffer, 1, pBufHdr->nAllocLen , inputBufferFile);
    fseek(inputBufferFile,packet_padding_size, SEEK_CUR);

    pBufHdr->nFilledLen = bytes_read;
    total_bytes_read+=bytes_read;
    num_packets++;

    if(total_bytes_read >=(signed)total_payload_data)
    {
    if(total_bytes_read > (signed)total_payload_data)
        {
            pBufHdr->nFilledLen = pBufHdr->nFilledLen -(total_bytes_read - total_payload_data);
        }
        bytes_read = pBufHdr->nFilledLen;
        total_bytes_read = total_payload_data;
        pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
        bInputEosReached = true;
        DEBUG_PRINT ("END OF PAYLOAD:EOS REACHED \n");
        DEBUG_PRINT("total_bytes_read =%d num_packets =%d\n",total_bytes_read,num_packets);

    }
    if( bytes_read == 0 )
    {
        pBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
        DEBUG_PRINT ("\nBytes read zero\n");
        DEBUG_PRINT("total_bytes_read =%d num_packets =%d\n",total_bytes_read,num_packets - 1);
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

    int Size = 0;
    OMX_U16  headerobject[2] ={0};
    OMX_U16  headerobjectsize = 0;

    DEBUG_PRINT("Inside %s filename=%s\n", __FUNCTION__, in_filename);
    inputBufferFile = fopen (in_filename, "rb");
    if (inputBufferFile == NULL) {
        DEBUG_PRINT("\ni/p file %s could NOT be opened\n",
            in_filename);
        error_code = -1;
    }
    /*Skipping header object ID*/
    fseek(inputBufferFile, 16 , SEEK_CUR);
    /*reading header object size*/
    Size = fread(headerobject, 1, 2, inputBufferFile);
    if(Size <=0 ){
        DEBUG_PRINT("NO DATA READ Parser failed\n");
        error_code = -1;
    }
    headerobjectsize =( (headerobject[0] | headerobject[1] << 8) );
    DEBUG_PRINT("headerobjectsize = %d",headerobjectsize);
    fseek(inputBufferFile, -18 , SEEK_CUR);
    /*Skipping header object size + data object header size*/
    fseek(inputBufferFile,headerobjectsize + DATA_HEADERSIZE, SEEK_SET);

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

        header_len = fwrite(&hdr,1,sizeof(hdr), outputBufferFile);

        if (header_len <= 0)
        {
            DEBUG_PRINT("Invalid Wav header \n");
        }
        DEBUG_PRINT(" Length og wav header is %d \n",header_len );
    }
    return error_code;
}

void process_portreconfig ( )
{
    int bufCnt,i=0;
    OMX_ERRORTYPE ret;
    struct msm_audio_config drv_pcm_config;
    //wait_for_event();
    DEBUG_PRINT("************************************");
    DEBUG_PRINT("RECIEVED EVENT PORT SETTINGS CHANGED EVENT\n");
    DEBUG_PRINT("******************************************\n");

    // wait for port settings changed event


    DEBUG_PRINT("************************************");
    DEBUG_PRINT("NOW SENDING FLUSH CMD\n");
    DEBUG_PRINT("******************************************\n");
    flushinprogress = 1;
    OMX_SendCommand(wma_dec_handle, OMX_CommandFlush, 1, 0);

    wait_for_event();
    DEBUG_PRINT("************************************");
    DEBUG_PRINT("RECIEVED FLUSH EVENT CMPL\n");
    DEBUG_PRINT("******************************************\n");

    // Send DISABLE command
    OMX_SendCommand(wma_dec_handle, OMX_CommandPortDisable, 1, 0);

    DEBUG_PRINT("******************************************\n");
    DEBUG_PRINT("FREEING BUFFERS output_buf_cnt=%d\n",output_buf_cnt);
    DEBUG_PRINT("******************************************\n");
    // Free output Buffer
    for(bufCnt=0; bufCnt < output_buf_cnt; ++bufCnt) {
        OMX_FreeBuffer(wma_dec_handle, 1, pOutputBufHdrs[bufCnt]);
    }

    wait_for_disable_event();
    DEBUG_PRINT("******************************************\n");
    DEBUG_PRINT("DISABLE EVENT RECD\n");
    DEBUG_PRINT("******************************************\n");

        // Send Enable command
    OMX_SendCommand(wma_dec_handle, OMX_CommandPortEnable, 1, 0);
    flushinprogress = 0;
    // AllocateBuffers
    DEBUG_PRINT("******************************************\n");
    DEBUG_PRINT("ALLOC BUFFER AFTER PORT REENABLE");
    DEBUG_PRINT("******************************************\n");
    /* Allocate buffer on decoder's o/p port */
    error = Allocate_Buffer(&pOutputBufHdrs, outputportFmt.nPortIndex,
                            output_buf_cnt, outputportFmt.nBufferSize);
    if (error != OMX_ErrorNone) {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Output buffer error output_buf_cnt=%d\n",output_buf_cnt);
        //return -1;
    }
    else
    {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Output buffer success output_buf_cnt=%d\n",output_buf_cnt);
    }

    DEBUG_PRINT("******************************************\n");
    DEBUG_PRINT("ENABLE EVENTiHANDLER RECD\n");
    DEBUG_PRINT("******************************************\n");
    // wait for enable event to come back
    wait_for_event();
    DEBUG_PRINT(" Calling stop on pcm driver...\n");
    if((tunnel == 0 || !filewrite) && start_done)
    {
        while (fsync(m_pcmdrv_fd) < 0) {
        printf(" fsync failed\n");
        sleep(1);
        }
        ioctl(m_pcmdrv_fd, AUDIO_STOP, 0);
        ioctl(m_pcmdrv_fd, AUDIO_FLUSH, 0);
        sleep(3);
        DEBUG_PRINT("AUDIO_STOP\n");
        start_done = 0;
    }
    if(version == WMAPRO)
    {
        OMX_INDEXTYPE index;
        OMX_GetExtensionIndex(wma_dec_handle,"OMX.Qualcomm.index.audio.wma10Pro",&index);
        OMX_GetParameter(wma_dec_handle,index,&wmaparam10Pro);
        drv_pcm_config.sample_rate = wmaparam10Pro.nSamplingRate;
        drv_pcm_config.channel_count = wmaparam10Pro.nChannels;
        printf("sample =%lu channel = %d\n",wmaparam10Pro.nSamplingRate,wmaparam10Pro.nChannels);
    } else {
        OMX_SetParameter(wma_dec_handle,OMX_IndexParamAudioWma,&wmaparam);
        drv_pcm_config.sample_rate = wmaparam.nSamplingRate;
        drv_pcm_config.channel_count = wmaparam.nChannels;
        printf("sample =%lu channel = %d\n",wmaparam.nSamplingRate,wmaparam.nChannels);
    }
    channels = drv_pcm_config.channel_count;
    samplerate = drv_pcm_config.sample_rate;
    ioctl(m_pcmdrv_fd, AUDIO_SET_CONFIG, &drv_pcm_config);
    DEBUG_PRINT("Configure Driver for PCM playback \n");
    bReconfigureOutputPort = 0;

    DEBUG_PRINT("******************************************\n");
    DEBUG_PRINT("FTB after PORT RENABLE\n");
    DEBUG_PRINT("******************************************\n");
    for(i=0; i < output_buf_cnt; i++) {
        DEBUG_PRINT ("\nOMX_FillThisBuffer on output buf no.%d\n",i);
        pOutputBufHdrs[i]->nOutputPortIndex = 1;
        //pOutputBufHdrs[i]->nFlags &= ~OMX_BUFFERFLAG_EOS;
        ret = OMX_FillThisBuffer(wma_dec_handle, pOutputBufHdrs[i]);
        if (OMX_ErrorNone != ret) {
            DEBUG_PRINT("OMX_FillThisBuffer failed with result %d\n", ret);
        }
        else {
            DEBUG_PRINT("OMX_FillThisBuffer success!\n");
    }
   }
}

static void write_devctlcmd(int fd, const void *buf, int param){
	int nbytes, nbytesWritten;
	char cmdstr[128];
	snprintf(cmdstr, 128, "%s%d\n", (char *)buf, param);
	nbytes = strlen(cmdstr);
	nbytesWritten = write(fd, cmdstr, nbytes);

	if(nbytes != nbytesWritten)
		printf("Failed to write string \"%s\" to omx_devmgr\n",cmdstr);
}
