/*============================================================================
                            O p e n M A X   Component
                                Audio AMR Encoder

*//** @file comx_amr_aenc.h
  This module contains the class definition for openMAX encoder component.

Copyright (c) 2006-2008 Qualcomm Technologies, Inc.
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


#include <pthread.h>
#include <inttypes.h>
#include <unistd.h>

#include "OMX_Core.h"
#include "OMX_Audio.h"
#include "aenc_svr.h"
#include "qc_omx_component.h"
#include "Map.h"
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
#define PrintFrameHdr(bufHdr) DEBUG_PRINT("bufHdr %x buf %x size %d TS %d\n",     \
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
#define TRUE  1
#define FALSE 0

// OMX amr audio encoder class
class omx_amr_aenc: public qc_omx_component
{
  public:
           omx_amr_aenc();  // constructor
  virtual ~omx_amr_aenc();  // destructor

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
                  unsigned int id,
                  bool lock);

  bool post_output(unsigned int p1,
                  unsigned int p2,
                  unsigned int id,
                  bool lock);

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

  OMX_COMPONENT_GENERATE_FRAME_DONE,


  OMX_COMPONENT_GENERATE_FTB


};

static const unsigned OMX_AMR_NATIVE_OUTPUT_BUFFER_SIZE  = 36;
static const unsigned OMX_AMR_OUTPUT_BUFFER_SIZE  = 288;
static const unsigned OMX_NUM_DEFAULT_BUF = 2;
static const unsigned SRC_OFFSET = 294; //3206;
static const unsigned HEADER_LENGTH = 6;
static const unsigned NATIVE_FRAME_LENGTH = 36;
static const unsigned OMX_CORE_CONTROL_CMDQ_SIZE  = 100;
static const unsigned FRAME_DURATION = 20;
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

  OMX_STATETYPE                m_state;    // OMX State
  OMX_PTR                      m_app_data;    // Application data
  OMX_CALLBACKTYPE             m_cb;       // Application callbacks
  OMX_AUDIO_PARAM_AMRTYPE      m_aenc_param; // Cache amr encoder parameter
  OMX_S32                      m_volume;   //Unit to be determined
  struct aenc_cmd_svr          *m_cmd_svr,*m_cmd_cln;  // Command server instance

  int                          m_drv_fd;   // Kernel device node file handle
  unsigned int                 frameDuration; // holds the duration of each frame
  OMX_U8*                      pFirstOutputBuf; // holds the address of the first outputbuf
  OMX_U8*                      pSecondOutputBuf; // holds the address of the second outputbuf
  input_buffer_map             m_input_buf_hdrs; //Input buffer header list
  output_buffer_map            m_output_buf_hdrs; //Output buffer header list

  omx_cmd_queue                m_data_q;  // Data command Q
  omx_cmd_queue                m_cmd_q;   // Command Q for rest of the events
  omx_cmd_queue                m_output_q;   // Command Q for rest of the events
  unsigned int                 m_out_buf_count;   // Number of Output Buffers
  unsigned int                 m_inp_buf_count;
  unsigned int                 m_flags;   // encapsulate the waiting states.
  unsigned int                 fcount;
  unsigned int                 nTimestamp;
  unsigned int                 pcm_feedback;    // enable tunnel or non-tunnel mode
  unsigned int                 pChannels;
  unsigned int                 pbitrate ;
  unsigned int                 pframeformat;
  unsigned int                 pdtx;
  unsigned int                 output_buffer_size;

  pthread_mutex_t              m_lock;
  pthread_mutexattr_t          m_lock_attr;
  unsigned                     m_msg_cnt; // message count

  unsigned                     m_cmd_cnt; // command count
  unsigned                     m_etb_cnt; // Empty This Buffer count
  unsigned                     m_ebd_cnt; // Empty Buffer Done Count

  unsigned short               m_session_id;
  OMX_U32                      m_recPath;

  pthread_mutex_t                m_state_lock;
  pthread_mutex_t                m_outputlock;
  pthread_mutexattr_t            m_outputlock_attr;
  pthread_mutexattr_t            m_state_lock_attr;
  /* Private function */
  OMX_ERRORTYPE allocate_input_buffer(OMX_HANDLETYPE             hComp,
                                      OMX_BUFFERHEADERTYPE **bufferHdr,
                                      OMX_U32                     port,
                                      OMX_PTR                  appData,
                                      OMX_U32                    bytes);
  bool allocate_done(void);
  bool release_done();
  bool execute_omx_flush(void);
  bool search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer);
  bool search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer);
};






