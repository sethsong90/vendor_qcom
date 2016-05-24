

/*
    An Open max test application for AMR-NB Encoding....
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "qc_omx_common.h"

typedef unsigned char uint8;
typedef unsigned char byte;
typedef unsigned int  uint32;
typedef unsigned int  uint16;

void Release_Encoder();
FILE *F1 = NULL;
uint32_t channels = 1;
uint32_t frameformat = 0;
uint32_t bitrate = 7;
uint32_t dtx = 0;




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
//pthread_mutex_t flock;
pthread_cond_t fcond;
FILE * outputBufferFile;
OMX_PARAM_PORTDEFINITIONTYPE outputportFmt;
OMX_AUDIO_PARAM_AMRTYPE amrparam;
OMX_PORT_PARAM_TYPE portParam;
OMX_ERRORTYPE error;

static bFileclose = 0;
static unsigned totaldatalen = 0;
/************************************************************************/
/*                GLOBAL INIT                    */
/************************************************************************/

int output_buf_cnt = 0;
int bInputEosReached = 0;
volatile int event_is_done = 0;
volatile int fbd_event_is_done = 0;
const char *out_filename;
int timeStampLfile = 0;
int timestampInterval = 100;
unsigned to_idle_transition = 0;

//* OMX Spec Version supported by the wrappers. Version = 1.1 */
const OMX_U32 CURRENT_OMX_SPEC_VERSION = 0x00000101;
OMX_COMPONENTTYPE* amr_enc_handle = 0;
OMX_BUFFERHEADERTYPE  **pOutputBufHdrs = NULL;

/************************************************************************/
/*                GLOBAL FUNC DECL                        */
/************************************************************************/
int Init_Encoder();
int Rec_Encoder();

/**************************************************************************/
/*                STATIC DECLARATIONS                       */
/**************************************************************************/

static int open_output_file ();
static OMX_ERRORTYPE Allocate_Buffer (OMX_COMPONENTTYPE *amr_enc_handle,
                                      OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                      OMX_U32 nPortIndex,
                                      long bufCntMin, long bufSize);

static OMX_ERRORTYPE EventHandler (OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_PTR pAppData,
                                   OMX_IN OMX_EVENTTYPE eEvent,
                                   OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2,
                                   OMX_IN OMX_PTR pEventData);

static OMX_ERRORTYPE FillBufferDone (OMX_IN OMX_HANDLETYPE hComponent,
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
    switch(eEvent) {
        case OMX_EventCmdComplete:
            DEBUG_PRINT("\n OMX_EventCmdComplete event=%d data1=%d data2=%d\n",(OMX_EVENTTYPE)eEvent,
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
    int bytes_read=0;
    int bytes_writen = 0;
    static unsigned int  count = 0;
    static unsigned int tcount = 0;
    static unsigned int taudcount = 0;
    static unsigned int tlen = 0;
    pthread_t thread;
    int r = 0;
    unsigned char readBuf;
    static int releaseCount = 0;

    if(bInputEosReached || (pBuffer->nFilledLen == 0)) {
        DEBUG_PRINT("\n*********************************************\n");
        DEBUG_PRINT("   EBD::EOS on output port\n ");
        DEBUG_PRINT("   TBD:::De Init the open max here....!!!\n");
        DEBUG_PRINT("*********************************************\n");
        return OMX_ErrorNone;
    }

    DEBUG_PRINT(" FillBufferDone #%d size %d\n", ++count,pBuffer->nFilledLen);

    bytes_writen = fwrite(pBuffer->pBuffer,1,pBuffer->nFilledLen,outputBufferFile);
    if(bytes_writen < pBuffer->nFilledLen)
    {
        DEBUG_PRINT("error: invalid AMR encoded data \n");
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
    if(releaseCount == 1)
    {
        // Dont issue any more FTB
        // Trigger Exe-->Idle Transition
        sleep(1);
        to_idle_transition = 1;
        OMX_SendCommand(amr_enc_handle, OMX_CommandStateSet, OMX_StateIdle,0);
        sleep(1);
        releaseCount++;
        // wait till Idle transition is complete
        // Trigger ReleaseEncoder procedure

    }
    else if(!releaseCount)
    {
        DEBUG_PRINT(" FBD calling FTB");
        OMX_FillThisBuffer(hComponent,pBuffer);
    }
    else{}

    return OMX_ErrorNone;
}


int main(int argc, char **argv)
{
    int bufCnt=0;
    OMX_ERRORTYPE result;
    unsigned char tmp;
    int bytes_writen = 0;

    (void) signal(SIGINT, Release_Encoder);

    pthread_cond_init(&cond, 0);
    pthread_mutex_init(&lock, 0);

    if (argc >= 5) {
        out_filename = argv[1];
        frameformat  = atoi(argv[2]);
            bitrate      = atoi(argv[3]);
               dtx          = atoi(argv[4]);
        if(frameformat != 3){
            DEBUG_PRINT("Only FRAMEFORMAT = 3 (FSF) currently supported\n");
            return 0;
            }
    } else {
          DEBUG_PRINT(" invalid format: \n");
          DEBUG_PRINT("ex: ./mm-aenc-omxamr AMR_OUTPUTFILE FRAMEFORMAT BITRATE DTX\n");
          DEBUG_PRINT("FRAMEFORMAT = 1 (IF1)\n");
          DEBUG_PRINT("FRAMEFORMAT = 2 (IF2)\n");
          DEBUG_PRINT("FRAMEFORMAT = 3 (FSF)\n");
          DEBUG_PRINT("FRAMEFORMAT = 4 (RTP)\n");
          DEBUG_PRINT("BITRATE= 0(MR475),1(MR515),2(MR59),3(MR67),4(MR74),5(MR795),6(MR102),7(MR122)\n");
          DEBUG_PRINT("DTX= 0(off),1(on_VAD1),2(on_VAD2),3(Auto)\n");
          return 0;
    }

    if(Init_Encoder()!= 0x00) {
        DEBUG_PRINT("Encoder Init failed\n");
        return -1;
    }
        fcntl(0, F_SETFL, O_NONBLOCK);
    if(Rec_Encoder() != 0x00) {
        DEBUG_PRINT("Rec_Encoder failed\n");
        return -1;
    }
    // Wait till EOS is reached...
        wait_for_event();
        if(bInputEosReached) {
            fclose(outputBufferFile);
                        DEBUG_PRINT("FILE CLOSED *********************\n");

        DEBUG_PRINT("\nMoving the encoder to loaded state \n");
        OMX_SendCommand(amr_enc_handle, OMX_CommandStateSet, OMX_StateLoaded,0);

        DEBUG_PRINT ("\nDeallocating o/p buffers \n");
        for(bufCnt=0; bufCnt < output_buf_cnt; ++bufCnt) {
            OMX_FreeBuffer(amr_enc_handle, 1, pOutputBufHdrs[bufCnt]);
        }
        wait_for_event();

        fclose(outputBufferFile);

        result = OMX_FreeHandle(amr_enc_handle);
        if (result != OMX_ErrorNone) {
            DEBUG_PRINT ("\nOMX_FreeHandle error. Error code: %d\n", result);
        }

        /* Deinit OpenMAX */

        OMX_Deinit();
        bInputEosReached = false;
        amr_enc_handle = NULL;
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&lock);
        DEBUG_PRINT("*****************************************\n");
        DEBUG_PRINT("******...AMR ENC TEST COMPLETED...***************\n");
        DEBUG_PRINT("*****************************************\n");


      }
        return 0;
}

void Release_Encoder()
{
    static cnt=0;
    int bufCnt=0;
    OMX_ERRORTYPE result;
    DEBUG_PRINT("END OF AMR ENCODING: EXITING PLEASE WAIT\n");
    bInputEosReached = 1;
    event_complete();
    cnt++;
    if(cnt > 1) {

        /* FORCE RESET  */
        amr_enc_handle = NULL;
        bInputEosReached = false;
            result = OMX_FreeHandle(amr_enc_handle);
        if (result != OMX_ErrorNone) {
        DEBUG_PRINT ("\nOMX_FreeHandle error. Error code: %d\n", result);
        }
            /* Deinit OpenMAX */
            OMX_Deinit();
            pthread_cond_destroy(&cond);
            pthread_mutex_destroy(&lock);
        DEBUG_PRINT("*****************************************\n");
        DEBUG_PRINT("******...AMR ENC TEST COMPLETED...***************\n");
        DEBUG_PRINT("*****************************************\n");
        exit(0);
    }
}

int Init_Encoder()
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE omxresult;
    OMX_U32 total = 0;
    OMX_U8** audCompNames;
    typedef OMX_U8* OMX_U8_PTR;
    char *role ="audio_encoder.amr";
    static OMX_CALLBACKTYPE call_back = {
    &EventHandler,NULL,&FillBufferDone
    };

    int i = 0;
    /* Init. the OpenMAX Core */
    DEBUG_PRINT("\nInitializing OpenMAX Core....\n");
    omxresult = OMX_Init();
    if(OMX_ErrorNone != omxresult) {
        DEBUG_PRINT("\n Failed to Init OpenMAX core");
        return -1;
    } else {
        DEBUG_PRINT("\nOpenMAX Core Init Done\n");
    }
    /* Query for audio encoders*/
    DEBUG_PRINT("AMR_test: Before entering OMX_GetComponentOfRole");
    OMX_GetComponentsOfRole(role, &total, 0);
    printf("test\n");
    //DEBUG_PRINT ("\nTotal components of role= :%s :%d", role, total);
    omxresult = OMX_GetHandle((OMX_HANDLETYPE*)(&amr_enc_handle),
            "OMX.qcom.audio.encoder.tunneled.amr", NULL, &call_back);
         printf("test1\n");
    if (FAILED(omxresult)) {
        DEBUG_PRINT ("\nFailed to Load the component\n");
        return -1;
    } else {
        DEBUG_PRINT ("\nComponent is in LOADED state\n");
    }
    /* Get the port information */
    CONFIG_VERSION_SIZE(portParam);
    omxresult = OMX_GetParameter(amr_enc_handle, OMX_IndexParamAudioInit,
                (OMX_PTR)&portParam);

    if(FAILED(omxresult)) {
        DEBUG_PRINT("\nFailed to get Port Param\n");
        return -1;
    } else {
        DEBUG_PRINT ("\nportParam.nPorts:%d\n", portParam.nPorts);
            DEBUG_PRINT ("\nportParam.nStartPortNumber:%d\n",
                         portParam.nStartPortNumber);
    }
    return 0;
}

int Rec_Encoder()
{
    int i;
    int Size=0;
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE ret;
    OMX_STATETYPE state;

    DEBUG_PRINT("sizeof[%d]\n", sizeof(OMX_BUFFERHEADERTYPE));
    if(open_output_file()) {
        DEBUG_PRINT("\n Returning -1");
        return -1;
    }
    /* Query the encoder outport's min buf requirements */
    CONFIG_VERSION_SIZE(outputportFmt);
    /* Port for which the Client needs to obtain info */
    outputportFmt.nPortIndex = portParam.nStartPortNumber + 1;

    OMX_GetParameter(amr_enc_handle,OMX_IndexParamPortDefinition,&outputportFmt);
    DEBUG_PRINT ("\nEnc: Output Buffer Count %d\n", outputportFmt.nBufferCountMin);
    DEBUG_PRINT ("\nEnc: Output Buffer Size %d\n", outputportFmt.nBufferSize);

    if(OMX_DirOutput != outputportFmt.eDir) {
        DEBUG_PRINT ("\nEnc: Expect Output Port\n");
        return -1;
    }
    CONFIG_VERSION_SIZE(amrparam);
    amrparam.nPortIndex   =  0;
    amrparam.nChannels    =  channels;
    amrparam.eAMRFrameFormat = frameformat;
       amrparam.nBitRate        = bitrate;
       amrparam.eAMRBandMode = bitrate+1;
       amrparam.eAMRDTXMode  = dtx;
    OMX_SetParameter(amr_enc_handle,OMX_IndexParamAudioAmr,&amrparam);
    DEBUG_PRINT ("\nOMX_SendCommand Encoder -> IDLE\n");
    OMX_SendCommand(amr_enc_handle, OMX_CommandStateSet, OMX_StateIdle,0);
    /* wait_for_event(); should not wait here event complete status will
    not come until enough buffer are allocated */
    output_buf_cnt = outputportFmt.nBufferCountMin ;
    /* Allocate buffer on encoder's O/Pp port */
    error = Allocate_Buffer(amr_enc_handle, &pOutputBufHdrs, outputportFmt.nPortIndex,
                output_buf_cnt, outputportFmt.nBufferSize);
    if (error != OMX_ErrorNone) {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Output buffer error\n");
        return -1;
    }
    else {
        DEBUG_PRINT ("\nOMX_AllocateBuffer Output buffer success\n");
    }
    wait_for_event();
    DEBUG_PRINT ("\nOMX_SendCommand encoder -> Executing\n");
    OMX_SendCommand(amr_enc_handle, OMX_CommandStateSet, OMX_StateExecuting,0);
    wait_for_event();
    DEBUG_PRINT(" Start sending OMX_FILLthisbuffer\n");
    for(i=0; i < output_buf_cnt; i++) {
        DEBUG_PRINT ("\nOMX_FillThisBuffer on output buf no.%d\n",i);
        pOutputBufHdrs[i]->nOutputPortIndex = 1;
        pOutputBufHdrs[i]->nFlags &= ~OMX_BUFFERFLAG_EOS;
        ret = OMX_FillThisBuffer(amr_enc_handle, pOutputBufHdrs[i]);
        if (OMX_ErrorNone != ret) {
            DEBUG_PRINT("OMX_FillThisBuffer failed with result %d\n", ret);
        }
        else {
            DEBUG_PRINT("OMX_FillThisBuffer success!\n");
        }
    }

    return 0;
}



static OMX_ERRORTYPE Allocate_Buffer( OMX_COMPONENTTYPE *avc_enc_handle,
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs,
                                       OMX_U32 nPortIndex,
                                       long bufCntMin, long bufSize)
{
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    OMX_ERRORTYPE error=OMX_ErrorNone;
    long bufCnt=0;
    *pBufHdrs= (OMX_BUFFERHEADERTYPE **)
           malloc(sizeof(OMX_BUFFERHEADERTYPE*)*bufCntMin);

    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) {
        DEBUG_PRINT("\n OMX_AllocateBuffer No %d \n", bufCnt);
        error = OMX_AllocateBuffer(amr_enc_handle, &((*pBufHdrs)[bufCnt]),
                       nPortIndex, NULL, bufSize);
    }
    return error;
}


static int open_output_file()
{
    int error_code = 0;
        char amr_header[6] = {0x23, 0x21, 0x41, 0x4D, 0x52, 0x0A};
    DEBUG_PRINT("Inside %s filename=%s\n", __FUNCTION__, out_filename);
    outputBufferFile = fopen (out_filename, "wb");
    if (outputBufferFile < 0) {
        DEBUG_PRINT("\ni/p file %s could NOT be opened\n",
                             out_filename);
        error_code = -1;
     }
         if(frameformat == 3)
             fwrite(amr_header,1,6,outputBufferFile);
     return error_code;
}
