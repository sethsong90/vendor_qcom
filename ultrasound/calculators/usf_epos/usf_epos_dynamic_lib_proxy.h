/*===========================================================================
                           usf_epos_dynamic_lib_proxy.h

DESCRIPTION: Provide a dynamic library proxy for the pen lib.

INITIALIZATION AND SEQUENCING REQUIREMENTS: None

Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/
#ifndef __USF_EPOS_DYNAMIC_LIB_PROXY__
#define __USF_EPOS_DYNAMIC_LIB_PROXY__

/*----------------------------------------------------------------------------
Include files
----------------------------------------------------------------------------*/
#include "usf_dynamic_lib_proxy.h"
#include "usf_epos_defs.h"

/*----------------------------------------------------------------------------
  Defines
----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
  Typedefs
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
  Classes
-----------------------------------------------------------------------------*/

class UsfEposDynamicLibProxy: public UsfDynamicLibProxy
{
private:

  /*============================================================================
    Function pointer typedefs
  ============================================================================*/

  typedef void (*get_dsp_version_fp_t)(char *OutDSPVersionString,
                                       unsigned char *OutDSPVersion);

  typedef void (*get_allocation_sizes_fp_t)(long *OutPointMaxCountPerPen,
                                            long *OutMaxPens,
                                            long *OutSizeOfWorkspace);

  typedef long (*init_dsp_fp_t)(EPoint *InPointBuffer,
                                void *InWorkspace,
                                void *InConfiguration,
                                long ConfigureationLength);

  typedef long(*get_points_fp_t)(long *pPacket,
                                 void *InWorkspace,
                                 FeedbackInfo *OutFeedback);

  typedef void (*set_rotation_axis_fp_t)(void *InWorkspace,
                                         long origin[3],
                                         long direction[3],
                                         long OffScreenZ);

  typedef void (*release_dsp_fp_t)(void *InWorkspace,
                                   void *OutConfiguration);

  typedef void (*agc_fp_t)(short *inPtr16,
                           long *input512,
                           AGC_struct *pAGC,
                           long ChannelNumber,
                           long EnableEvents);

  typedef void (*init_agc_fp_t)(AGC_struct *pAGC);

  typedef void (*update_agc_fp_t)(AGC_struct *pAGC,
                                  short Gain);

  typedef long (*command_fp_t)(void *InWorkspace,
                               long *CommandBuffer);

  typedef void (*init_dump_callbacks_fp_t)(WriteDumpCallback WriteDumpFunc);

  typedef long (*reset_dsp_fp_t)(EPoint *InPointBuffer,
                                 void *InWorkspace);

  typedef long (*load_coeffs_fp_t)(void *InWorkspace,
                                   void *InConfiguration,
                                   long InConfigLength);

  typedef int (*get_persistent_data_fp_t)(void *InWorkspace,
                                          long *Buffer,
                                          int Length);

  typedef void (*set_dsp_trace_callback_fp_t)(TraceCallback callback);

  typedef long (*query_epoint_fp_t)(void *InWorkspace,
                                    EPointType eptype,
                                    long pointnum,
                                    long *result,
                                    long buflen);

  /*============================================================================
    Function pointers
  ============================================================================*/

  get_dsp_version_fp_t          m_get_dsp_version;
  get_allocation_sizes_fp_t     m_get_allocation_sizes;
  init_dsp_fp_t                 m_init_dsp;
  get_points_fp_t               m_get_points;
  set_rotation_axis_fp_t        m_set_rotation_axis;
  release_dsp_fp_t              m_release_dsp;
  agc_fp_t                      m_agc;
  init_agc_fp_t                 m_init_agc;
  update_agc_fp_t               m_update_agc;
  command_fp_t                  m_command;
  init_dump_callbacks_fp_t      m_init_dump_callbacks;
  reset_dsp_fp_t                m_reset_dsp;
  load_coeffs_fp_t              m_load_coeffs;
  get_persistent_data_fp_t      m_get_persistent_data;
  set_dsp_trace_callback_fp_t   m_set_dsp_trace_callback;
  query_epoint_fp_t             m_query_epoint;

protected:
  /*============================================================================
    FUNCTION:  load_all_methods
  ============================================================================*/
  /**
   * Loads all the methods from the library.
   *
   * @return bool - true success
   *                false failure
   */
  bool load_all_methods();

public:

  /*============================================================================
    FUNCTION:  expand_epoints
  ============================================================================*/
  /**
   * Converts the EPoints array to the wrapped array to support
   * the old pen lib API by assigning zero to all the non-existing
   * fields.
   *
   * @return bool - true success
   *                false failure
   */
  bool extend_epoints(void *in_workspace,
                      long num_points,
                      EPoint *in_epoints,
                      usf_extended_epoint_t *out_wrapped_epoints);

  /*============================================================================
    Library method wrapper functions
  ============================================================================*/

  void get_dsp_version(char *OutDSPVersionString,
                       unsigned char *OutDSPVersion);

  void get_allocation_sizes(long *OutPointMaxCountPerPen,
                            long *OutMaxPens,
                            long *OutSizeOfWorkspace);

  long init_dsp(EPoint *InPointBuffer,
                void *InWorkspace,
                void *InConfiguration,
                long ConfigureationLength);

  long get_points(long *pPacket,
                  void *InWorkspace,
                  FeedbackInfo *OutFeedback);

  void set_rotation_axis(void *InWorkspace,
                         long origin[3],
                         long direction[3],
                         long OffScreenZ);

  void release_dsp(void *InWorkspace,
                   void *OutConfiguration);

  void agc(short *inPtr16,
           long *input512,
           AGC_struct *pAGC,
           long ChannelNumber,
           long EnableEvents);

  void init_agc(AGC_struct *pAGC);

  void update_agc(AGC_struct *pAGC,
                  short Gain);

  long command(void *InWorkspace,
               long *CommandBuffer);

  void init_dump_callbacks(WriteDumpCallback WriteDumpFunc);

  long reset_dsp(EPoint *InPointBuffer,
                 void *InWorkspace);

  long load_coeffs(void *InWorkspace,
                   void *InConfiguration,
                   long InConfigLength);

  int get_persistent_data(void *InWorkspace,
                          long *Buffer,
                          int Length);

  void set_dsp_trace_callback(TraceCallback callback);

  long query_epoint(void *InWorkspace,
                    EPointType eptype,
                    long pointnum,
                    long *result,
                    long buflen);

};

#endif //__USF_EPOS_DYNAMIC_LIB_PROXY__
