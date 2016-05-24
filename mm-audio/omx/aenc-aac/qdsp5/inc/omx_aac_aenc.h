/*============================================================================
                            O p e n M A X   Component
                                Audio AAC Encoder

*//** @file comx_aac_aenc.h
  This module contains the class definition for openMAX encoder component.

Copyright (c) 2006-2010 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

/*============================================================================
                              Edit History

$Header: //depot/asic/sandbox/users/ronaldk/videodec/OMXVdecCommon/omx_vdec.h#1 $
when       who     what, where, why
--------   ---     -------------------------------------------------------
08/13/2008  pl     Initial version

============================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

#include<stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <inttypes.h>
#include <unistd.h>

#include "OMX_Core.h"
#include "OMX_Audio.h"
#include <linux/msm_audio.h>
#include <linux/msm_audio_aac.h>
#include "aenc_svr.h"
#include "qc_omx_component.h"
#include "Map.h"
#include <semaphore.h>
#include "QOMX_AudioExtensions.h"
#include "QOMX_AudioIndexExtensions.h"

extern "C" {
  void * get_omx_component_factory_fn(void);
}

//////////////////////////////////////////////////////////////////////////////
//                       Module specific globals
//////////////////////////////////////////////////////////////////////////////



#define OMX_SPEC_VERSION  0x00000101



//////////////////////////////////////////////////////////////////////////////
//               Macros
//////////////////////////////////////////////////////////////////////////////
//

#ifdef DEBUG_PRINT
#undef DEBUG_PRINT
#define DEBUG_PRINT
#endif

#define PrintFrameHdr(i,bufHdr) DEBUG_PRINT("i=%d bufHdr %x buf %x size %d TS %d\n",i,\
                           (unsigned) bufHdr,                                     \
                           (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->pBuffer,   \
                           (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nFilledLen,\
                           (unsigned)((OMX_BUFFERHEADERTYPE *)bufHdr)->nTimeStamp)

// BitMask Management logic
#define BITS_PER_BYTE 8
#define BITMASK_SIZE(mIndex)            (((mIndex) + BITS_PER_BYTE - 1)/BITS_PER_BYTE)
#define BITMASK_OFFSET(mIndex)          ((mIndex)/BITS_PER_BYTE)
#define BITMASK_FLAG(mIndex)            (1 << ((mIndex) % BITS_PER_BYTE))
#define BITMASK_CLEAR(mArray,mIndex)    (mArray)[BITMASK_OFFSET(mIndex)] &=  ~(BITMASK_FLAG(mIndex))
#define BITMASK_SET(mArray,mIndex)      (mArray)[BITMASK_OFFSET(mIndex)] |=  BITMASK_FLAG(mIndex)
#define BITMASK_PRESENT(mArray,mIndex)  ((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex)  )
#define BITMASK_ABSENT(mArray,mIndex)   (((mArray)[BITMASK_OFFSET(mIndex)] & BITMASK_FLAG(mIndex)  ) == 0x0)

#define max(x,y) (x >= y?x:y)
#define min(x,y) (x <= y?x:y)

#define OMX_AENC_VOLUME_STEP 0x147
#define OMX_AENC_MIN         0
#define OMX_AENC_MAX         100

//ADTS Header
#define AUDAAC_MAX_ADTS_HEADER_LENGTH 7
#define AACHDR_LAYER_SIZE             2
#define AACHDR_CRC_SIZE               1
#define AAC_PROFILE_SIZE              2
#define AAC_SAMPLING_FREQ_INDEX_SIZE  4
#define AAC_ORIGINAL_COPY_SIZE        1
#define AAC_HOME_SIZE                 1
#define AUDAAC_ADTS_FRAME_LENGTH_SIZE    13

//ADIF Header
#define AUDAAC_MAX_ADIF_HEADER_LENGTH 17
#define ADIF_ID                       32
#define COPY_RIGHT_PRESENT            1
#define ORIGINAL_COPY                 1
#define HOME                          1
#define BITSTREAM_TYPE                1
#define BITRATE                       23
#define NUM_PROGRAM_CONFIG_ELEMENTS    4

//From Decoder
#define AAC_SAMPLING_FREQ_INDEX_SIZE          4
#define AAC_PROFILE_SIZE                      2
#define AAC_ELEMENT_INSTANCE_TAG_SIZE         4
#define AAC_NUM_FRONT_CHANNEL_ELEMENTS_SIZE   4
#define AAC_NUM_SIDE_CHANNEL_ELEMENTS_SIZE    4
#define AAC_NUM_BACK_CHANNEL_ELEMENTS_SIZE    4
#define AAC_NUM_LFE_CHANNEL_ELEMENTS_SIZE     2
#define AAC_NUM_ASSOC_DATA_ELEMENTS_SIZE      3
#define AAC_NUM_VALID_CC_ELEMENTS_SIZE        4
#define AAC_MONO_MIXDOWN_PRESENT_SIZE         1
#define AAC_MONO_MIXDOWN_ELEMENT_SIZE         4
#define AAC_STEREO_MIXDOWN_PRESENT_SIZE       1
#define AAC_STEREO_MIXDOWN_ELEMENT_SIZE       4
#define AAC_MATRIX_MIXDOWN_PRESENT_SIZE       1
#define AAC_MATRIX_MIXDOWN_ELEMENT_SIZE       3
#define AAC_FCE_SIZE                          5
#define AAC_SCE_SIZE                          5
#define AAC_BCE_SIZE                          5
#define AAC_LFE_SIZE                          4
#define AAC_ADE_SIZE                          4
#define AAC_VCE_SIZE                          5
#define AAC_COMMENT_FIELD_BYTES_SIZE          8
#define AAC_COMMENT_FIELD_DATA_SIZE           8



//Raw Header
#define AUDAAC_MAX_MP4FF_HEADER_LENGTH  2

#define AUDAAC_MP4FF_OBJ_TYPE           5
#define AUDAAC_MP4FF_FREQ_IDX           4
#define AUDAAC_MP4FF_CH_CONFIG          4


#define NON_TUNNEL  1
#define TUNNEL      0
#define NUM_PORTS   2
#define EOS_FLAG    0x0001
#define DEFAULT_BITRATE 64000
#define DEFAULT_CHANNEL_CNT 2
#define DEFAULT_SAMPLINGRATE 44100

struct sample_rate_idx {
	OMX_U32 sample_rate;
	OMX_U32 sample_rate_idx;
};
static struct sample_rate_idx sample_idx_tbl[10] = {
	{8000, 0x0b},
	{11025, 0x0a},
	{12000, 0x09},
	{16000, 0x08},
	{22050, 0x07},
	{24000, 0x06},
	{32000, 0x05},
	{44100, 0x04},
	{48000, 0x03},
	{64000, 0x02},
};

// OMX aac audio encoder class
class omx_aac_aenc: public qc_omx_component
{
  public:
           omx_aac_aenc();  // constructor
  virtual ~omx_aac_aenc();  // destructor

  OMX_ERRORTYPE allocate_buffer(OMX_HANDLETYPE             hComp,
                 OMX_BUFFERHEADERTYPE **bufferHdr,
                 OMX_U32                     port,
                 OMX_PTR                  appData,
                 OMX_U32                    bytes);

  OMX_ERRORTYPE allocate_output_buffer(OMX_HANDLETYPE             hComp,
                                     OMX_BUFFERHEADERTYPE **bufferHdr,
                                     OMX_U32 port,OMX_PTR     appData,
                                     OMX_U32                    bytes);

  OMX_ERRORTYPE component_deinit(OMX_HANDLETYPE hComp);

  OMX_ERRORTYPE component_init(OMX_STRING role);

  OMX_ERRORTYPE component_role_enum(OMX_HANDLETYPE hComp,
                                  OMX_U8         *role,
                                  OMX_U32        index);

  OMX_ERRORTYPE component_tunnel_request(OMX_HANDLETYPE             hComp,
                                       OMX_U32                     port,
                                       OMX_HANDLETYPE     peerComponent,
                                       OMX_U32                 peerPort,
                                       OMX_TUNNELSETUPTYPE *tunnelSetup);

  OMX_ERRORTYPE empty_this_buffer(OMX_HANDLETYPE         hComp,
                                OMX_BUFFERHEADERTYPE *buffer);


  OMX_ERRORTYPE empty_this_buffer_proxy(OMX_HANDLETYPE         hComp,
                                OMX_BUFFERHEADERTYPE *buffer);


  OMX_ERRORTYPE fill_this_buffer(OMX_HANDLETYPE         hComp,
                               OMX_BUFFERHEADERTYPE *buffer);

  OMX_ERRORTYPE fill_this_buffer_proxy(OMX_HANDLETYPE         hComp,
                               OMX_BUFFERHEADERTYPE *buffer);

  OMX_ERRORTYPE free_buffer(OMX_HANDLETYPE         hComp,
                           OMX_U32                 port,
                           OMX_BUFFERHEADERTYPE *buffer);

  OMX_ERRORTYPE get_component_version(OMX_HANDLETYPE              hComp,
                                    OMX_STRING          componentName,
                                    OMX_VERSIONTYPE *componentVersion,
                                    OMX_VERSIONTYPE *     specVersion,
                                    OMX_UUIDTYPE       *componentUUID);

  OMX_ERRORTYPE get_config(OMX_HANDLETYPE      hComp,
                          OMX_INDEXTYPE configIndex,
                          OMX_PTR        configData);

  OMX_ERRORTYPE get_extension_index(OMX_HANDLETYPE     hComp,
                                  OMX_STRING     paramName,
                                  OMX_INDEXTYPE *indexType);

  OMX_ERRORTYPE get_parameter(OMX_HANDLETYPE hComp,
                             OMX_INDEXTYPE paramIndex,
                             OMX_PTR paramData);

  OMX_ERRORTYPE get_state(OMX_HANDLETYPE hComp,
                         OMX_STATETYPE *state);

  void buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr);
  void frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr);

  static void process_event_cb(void *client_data,
                               unsigned char id);

  static void process_output_cb(void *client_data,
                               unsigned char id);
  //Input thread callback
  static void process_input_cb(void *client_data,
                               unsigned char id);

  void audaac_rec_install_bits(OMX_U8 *input,
				OMX_U8 num_bits_reqd,
				OMX_U32  value,
				OMX_U16 *hdr_bit_index);
  void audaac_rec_install_adts_header_variable (OMX_U16  byte_num,
                                                OMX_U32 sample_index,
                                                OMX_U8 channel_config,
                                                OMX_U8 *audaac_header_adts);
  void audaac_rec_install_adif_header_variable (OMX_U16  byte_num,
                                                OMX_U32 sample_index,
                                                OMX_U8 channel_config,
					        OMX_U8 *audaac_header_adif);
  void audaac_rec_install_mp4ff_header_variable (OMX_U16  byte_num,
                                               OMX_U32 sample_index,
                                               OMX_U8 channel_config,
                                               OMX_U8 *audaac_header_raw);


  OMX_U32 map_adts_sample_index(OMX_U32 srate);

  OMX_ERRORTYPE send_command_proxy(OMX_HANDLETYPE hComp,
                            OMX_COMMANDTYPE  cmd,
                            OMX_U32       param1,
                            OMX_PTR      cmdData);
  OMX_ERRORTYPE send_command(OMX_HANDLETYPE hComp,
                            OMX_COMMANDTYPE  cmd,
                            OMX_U32       param1,
                            OMX_PTR      cmdData);

  bool post_event(unsigned int p1,
                  unsigned int p2,
                  unsigned int id );

  bool post_output(unsigned int p1,
                  unsigned int p2,
                  unsigned int id);

  bool post_input(unsigned int p1,
                  unsigned int p2,
                  unsigned int id);

  OMX_ERRORTYPE set_callbacks(OMX_HANDLETYPE hComp,
                             OMX_CALLBACKTYPE *callbacks,
                             OMX_PTR appData);

  OMX_ERRORTYPE set_config(OMX_HANDLETYPE hComp,
                          OMX_INDEXTYPE configIndex,
                          OMX_PTR configData);

  OMX_ERRORTYPE set_parameter(OMX_HANDLETYPE hComp,
                             OMX_INDEXTYPE paramIndex,
                             OMX_PTR paramData);

  OMX_ERRORTYPE use_buffer(OMX_HANDLETYPE             hComp,
                          OMX_BUFFERHEADERTYPE **bufferHdr,
                          OMX_U32                     port,
                          OMX_PTR                  appData,
                          OMX_U32                    bytes,
                          OMX_U8                  *buffer);

  OMX_ERRORTYPE use_EGL_image(OMX_HANDLETYPE             hComp,
                            OMX_BUFFERHEADERTYPE **bufferHdr,
                            OMX_U32                     port,
                            OMX_PTR                  appData,
                            void *                  eglImage);

private:


// Bit Positions
enum flags_bit_positions
{
  // Defer transition to IDLE
  OMX_COMPONENT_IDLE_PENDING            =0x1,
  // Defer transition to LOADING
  OMX_COMPONENT_LOADING_PENDING         =0x2,

  OMX_COMPONENT_MUTED                   =0x3
};

// Deferred callback identifiers
enum
{
  //Event Callbacks from the component thread context
  OMX_COMPONENT_GENERATE_EVENT       = 0x1,

  //Buffer Done callbacks from component thread context
  OMX_COMPONENT_GENERATE_BUFFER_DONE = 0x2,

  OMX_COMPONENT_GENERATE_ETB         = 0x3,
  //Command
  OMX_COMPONENT_GENERATE_COMMAND     = 0x4,
  OMX_COMPONENT_GENERATE_FRAME_DONE  = 0x05,
  OMX_COMPONENT_GENERATE_FTB         = 0x06,
  OMX_COMPONENT_GENERATE_EOS        = 0x07,
  OMX_COMPONENT_SUSPEND              = 0x08
};

static const unsigned OMX_CORE_NUM_INPUT_BUFFERS  =   2;
static const unsigned OMX_CORE_NUM_OUTPUT_BUFFERS  =   16;

static const unsigned OMX_CORE_INPUT_BUFFER_SIZE  = 8192; 
static const unsigned OMX_CORE_CONTROL_CMDQ_SIZE  = 100;
static const unsigned OMX_AAC_OUTPUT_BUFFER_SIZE = 1536;
static const unsigned NUM_SAMPLES_PER_FRAME = 1024;
static const unsigned MILLISECOND = 1000;
typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*>
                                                   input_buffer_map;

typedef Map<OMX_BUFFERHEADERTYPE*, OMX_BUFFERHEADERTYPE*>
                                                   output_buffer_map;

enum port_indexes
{
  OMX_CORE_INPUT_PORT_INDEX        =0,
  OMX_CORE_OUTPUT_PORT_INDEX       =1
};

struct omx_event
{
  unsigned param1;
  unsigned param2;
  unsigned id;
};

struct omx_cmd_queue
{
  omx_event m_q[OMX_CORE_CONTROL_CMDQ_SIZE];
  unsigned m_read;
  unsigned m_write;
  unsigned m_size;

  omx_cmd_queue();
  ~omx_cmd_queue();
  bool insert_entry(unsigned p1, unsigned p2, unsigned id);
  bool delete_entry(unsigned *p1,unsigned *p2, unsigned *id);
};
typedef struct metadata_input
{
	OMX_U16   offsetVal;
	OMX_TICKS nTimeStamp;
	OMX_U32   nFlags;
	OMX_U16   error_flag;
	OMX_U16   samplingFreq;
	OMX_U16   channels;
	OMX_U32   tickCount;
}__attribute__((packed)) META_IN;

typedef struct metadata_output
{
	OMX_U16   offsetVal;
	OMX_TICKS nTimeStamp;
	OMX_U32   nFlags;
}__attribute__((packed))META_OUT;


  OMX_STATETYPE                       m_state;    // OMX State
  OMX_PTR                             m_app_data;    // Application data
  OMX_CALLBACKTYPE                    m_cb;       // Application callbacks
  OMX_AUDIO_PARAM_AACPROFILETYPE      m_aenc_param; // Cache aac encoder parameter
  OMX_AUDIO_PARAM_PCMMODETYPE         m_pcm_param; // Cache pcm  parameter
  OMX_PARAM_PORTDEFINITIONTYPE        m_input_param;
  OMX_PARAM_PORTDEFINITIONTYPE        m_output_param;

  OMX_S32                             m_volume;   //Unit to be determined
  OMX_U32                             tickcount;
  OMX_U8			      m_is_alloc_buf;
  struct aenc_cmd_svr                 *m_cmd_svr;  // Command server instance
  struct aenc_cmd_cln                 *m_cmd_cln;  // Client server instance
  struct aenc_cmd_cln                 *m_cmd_cln_input;  // for input thread

  int                                 m_drv_fd;   // Kernel device node file handle
  unsigned int                        frameDuration; // holds the duration of each frame

  input_buffer_map                    m_input_buf_hdrs; //Input buffer header list
  output_buffer_map                   m_output_buf_hdrs; //Output buffer header list
  OMX_U8                              *m_tmp_out_meta_buf;    // Required only in NT mode
  OMX_U8                              *m_tmp_in_meta_buf;     // Required only in NT mode
  OMX_S32                             sample_idx;

  omx_cmd_queue                       m_data_q;  // Data command Q
  omx_cmd_queue                       m_cmd_q;   // Command Q for rest of the events
  omx_cmd_queue                       m_output_q;   // Command Q for rest of the events
  omx_cmd_queue                       m_output_ctrl_cmd_q;
  omx_cmd_queue                       m_output_ctrl_fbd_q;
  omx_cmd_queue                       m_input_q;   // input port events
  omx_cmd_queue                       m_input_ctrl_cmd_q;
  omx_cmd_queue                       m_input_ctrl_ebd_q;
	
  unsigned int                        m_inp_buf_count;   // Number of Input Buffers
  unsigned int                        m_out_buf_count;   // Number of Output Buffers
  unsigned int                        output_buffer_size;
  unsigned int                        input_buffer_size;
  unsigned int                        m_flags;   // encapsulate the waiting states.
  unsigned int                        fcount;
  unsigned int                        nTimestamp;
  unsigned int                        pcm_input;    // enable tunnel or non-tunnel mode
  volatile int                        m_is_event_done;

  pthread_mutex_t                     m_lock;
  pthread_mutex_t                     m_state_lock;

  pthread_mutex_t                     m_outputlock;
  pthread_mutexattr_t                 m_outputlock_attr;

  pthread_mutex_t                     m_inputlock; //Input thread mutex
  pthread_mutexattr_t                 m_inputlock_attr;

  pthread_mutex_t                     out_buf_count_lock;
  pthread_mutex_t                     in_buf_count_lock;
  pthread_mutex_t                     m_event_lock;
  pthread_mutex_t                     m_flush_lock;

  pthread_cond_t                      cond;
  pthread_mutexattr_t                 m_state_attr;
  pthread_mutexattr_t                 m_lock_attr;
  pthread_mutexattr_t                 out_buf_count_lock_attr;
  pthread_mutexattr_t                 in_buf_count_lock_attr;
  pthread_mutexattr_t                 m_event_lock_attr;
  pthread_mutexattr_t                 m_flush_lock_attr;
  unsigned                            m_msg_cnt; // message count

  unsigned                            m_fbd_cnt;
  unsigned                            m_num_out_buf;
  unsigned                            m_num_in_buf;
  sem_t                               sem_States;
  sem_t                               sem_read_msg;
  unsigned                            m_idle_transition;
  unsigned short                      m_session_id;
  //start world clock
  OMX_TICKS                           m_swc;
  int                                 flag;
  bool                                bFlushinprogress;
  OMX_U8                              m_flush_cnt ;


  /* Private function */
  OMX_ERRORTYPE allocate_input_buffer(OMX_HANDLETYPE             hComp,
                                      OMX_BUFFERHEADERTYPE **bufferHdr,
                                      OMX_U32                     port,
                                      OMX_PTR                  appData,
                                      OMX_U32                    bytes);
  bool allocate_done(void);
  bool release_done();
  bool execute_omx_flush(OMX_IN OMX_U32 param1, bool cmd_cmpl=true);
  bool execute_input_omx_flush(void);
  bool execute_output_omx_flush(void);
  void flush_ack(void);
  bool search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer);
  bool search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer);

    void wait_for_event();

    void event_complete();

    long long aenc_time_microsec();
};






