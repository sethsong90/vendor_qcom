/*===========================================================================
                           usf_epos_dynamic_lib_proxy.cpp

DESCRIPTION: Implement a dynamic library proxy for the pen lib.

INITIALIZATION AND SEQUENCING REQUIREMENTS: None

Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
=============================================================================*/

/*----------------------------------------------------------------------------
Include files
----------------------------------------------------------------------------*/
#include "usf_log.h"
#include "usf_epos_dynamic_lib_proxy.h"
#include <stdlib.h>

/*----------------------------------------------------------------------------
  Defines
----------------------------------------------------------------------------*/

#define GET_METHOD(symbol_name, type, new_api_method)              \
    m_##type = (type##_fp_t)get_method(#symbol_name);              \
    if (NULL == m_##type)                                          \
    {                                                              \
      if (new_api_method)                                          \
      {                                                            \
        LOGW("%s: Cannot load symbol "#symbol_name,__FUNCTION__);  \
      }                                                            \
      else                                                         \
      {                                                            \
        LOGE("%s: Cannot load symbol "#symbol_name,__FUNCTION__);  \
        return false;                                              \
      }                                                            \
    }

/*-----------------------------------------------------------------------------
  Typedefs
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
  Static Variable Definitions
-----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
  Function definitions
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
  Function implementation
------------------------------------------------------------------------------*/

/*============================================================================
  FUNCTION:  load_all_methods
============================================================================*/
/**
 * See function definition in header file
 */
bool UsfEposDynamicLibProxy::load_all_methods()
{
  GET_METHOD(GetDSPVersion, get_dsp_version, false);
  GET_METHOD(GetAllocationSizes, get_allocation_sizes, false);
  GET_METHOD(InitDSP, init_dsp, false);
  GET_METHOD(GetPoints, get_points, false);
  GET_METHOD(ReleaseDSP, release_dsp, false);
  GET_METHOD(AGC, agc, false);
  GET_METHOD(InitAGC, init_agc, false);
  GET_METHOD(UpdateAGC, update_agc, false);
  GET_METHOD(Command, command, false);
  GET_METHOD(InitDumpCallbacks, init_dump_callbacks, false);
  GET_METHOD(ResetDSP, reset_dsp, false);
  GET_METHOD(LoadCoeffs, load_coeffs, false);
  GET_METHOD(GetPersistentData, get_persistent_data, false);
  GET_METHOD(SetDSPTraceCallback, set_dsp_trace_callback, false);
  // QueryEPoint and SetRotationAxis are a new API 3.3.x.x methods.
  // In order to support the old API, the absence of these methods
  // doesn't cause an error.
  GET_METHOD(SetRotationAxis, set_rotation_axis, true);
  GET_METHOD(QueryEPoint, query_epoint, true);

  return true;
}

/*============================================================================
  FUNCTION:  expand_epoints
============================================================================*/
/**
 * See function definition in header file
 */
bool UsfEposDynamicLibProxy::extend_epoints(void *in_workspace,
                                            long num_points,
                                            EPoint *in_epoints,
                                            usf_extended_epoint_t *out_wrapped_epoints)
{
  if ((NULL == in_epoints) ||
      (NULL == out_wrapped_epoints))
  {
    LOGE("%s: Invalid parameters",
         __FUNCTION__);
    return false;
  }

  for (int i = 0; i < num_points; i++)
  {
    // Copy the basic EPoint fields (old API)
    memcpy(&out_wrapped_epoints[i],
           &in_epoints[i],
           sizeof(in_epoints[i]));

    // Fill in the new API fields
    if ( 0 >= query_epoint(in_workspace,
                           POINT_TiltZ,
                           i,
                           &(out_wrapped_epoints[i].TiltZ),
                           sizeof(out_wrapped_epoints[i].TiltZ) / sizeof(long)))
    {
      LOGE("%s: QueryEPoint failed",
           __FUNCTION__);
      return false;
    }

    if (0 >= query_epoint(in_workspace,
                          POINT_WorkingPlane,
                          i,
                          &(out_wrapped_epoints[i].WorkingPlane),
                          sizeof(out_wrapped_epoints[i].WorkingPlane) / sizeof(long)))
    {
      LOGE("%s: QueryEPoint failed",
           __FUNCTION__);
      return false;
    }
  }

  return true;
}

/*============================================================================
  Library method wrapper functions
============================================================================*/

void UsfEposDynamicLibProxy::get_dsp_version(char *OutDSPVersionString,
                                             unsigned char *OutDSPVersion)
{
  (*m_get_dsp_version)(OutDSPVersionString,
                       OutDSPVersion);
}

void UsfEposDynamicLibProxy::get_allocation_sizes(long *OutPointMaxCountPerPen,
                                                  long *OutMaxPens,
                                                  long *OutSizeOfWorkspace)
{
  (*m_get_allocation_sizes)(OutPointMaxCountPerPen,
                            OutMaxPens,
                            OutSizeOfWorkspace);
}

long UsfEposDynamicLibProxy::init_dsp(EPoint *InPointBuffer,
                                      void *InWorkspace,
                                      void *InConfiguration,
                                      long ConfigureationLength)
{
  return (*m_init_dsp)(InPointBuffer,
                       InWorkspace,
                       InConfiguration,
                       ConfigureationLength);
}

long UsfEposDynamicLibProxy::get_points(long *pPacket,
                                        void *InWorkspace,
                                        FeedbackInfo *OutFeedback)
{
  return (*m_get_points)(pPacket,
                         InWorkspace,
                         OutFeedback);
}

void UsfEposDynamicLibProxy::set_rotation_axis(void *InWorkspace,
                                               long origin[3],
                                               long direction[3],
                                               long OffScreenZ)
{
  if (NULL != m_set_rotation_axis)
  {
    (*m_set_rotation_axis)(InWorkspace,
                           origin,
                           direction,
                           OffScreenZ);
  }
}

void UsfEposDynamicLibProxy::release_dsp(void *InWorkspace,
                                         void *OutConfiguration)
{
  (*m_release_dsp)(InWorkspace,
                   OutConfiguration);
}

void UsfEposDynamicLibProxy::agc(short *inPtr16,
                                 long *input512,
                                 AGC_struct *pAGC,
                                 long ChannelNumber,
                                 long EnableEvents)
{
  (*m_agc)(inPtr16,
           input512,
           pAGC,
           ChannelNumber,
           EnableEvents);
}

void UsfEposDynamicLibProxy::init_agc(AGC_struct *pAGC)
{
  (*m_init_agc)(pAGC);
}

void UsfEposDynamicLibProxy::update_agc(AGC_struct *pAGC,
                                        short Gain)
{
  (*m_update_agc)(pAGC,
                  Gain);
}

long UsfEposDynamicLibProxy::command(void *InWorkspace,
                                     long *CommandBuffer)
{
  return (*m_command)(InWorkspace,
                      CommandBuffer);
}

void UsfEposDynamicLibProxy::init_dump_callbacks(WriteDumpCallback WriteDumpFunc)
{
  (*m_init_dump_callbacks)(WriteDumpFunc);
}

long UsfEposDynamicLibProxy::reset_dsp(EPoint *InPointBuffer,
                                       void *InWorkspace)
{
  return (*m_reset_dsp)(InPointBuffer,
                        InWorkspace);
}

long UsfEposDynamicLibProxy::load_coeffs(void *InWorkspace,
                                         void *InConfiguration,
                                         long InConfigLength)
{
  return (*m_load_coeffs)(InWorkspace,
                          InConfiguration,
                          InConfigLength);
}

int UsfEposDynamicLibProxy::get_persistent_data(void *InWorkspace,
                                                long *Buffer,
                                                int Length)
{
  return (*m_get_persistent_data)(InWorkspace,
                                  Buffer,
                                  Length);
}

void UsfEposDynamicLibProxy::set_dsp_trace_callback(TraceCallback callback)
{
  (*m_set_dsp_trace_callback)(callback);
}

long UsfEposDynamicLibProxy::query_epoint(void *InWorkspace,
                                          EPointType eptype,
                                          long pointnum,
                                          long *result,
                                          long buflen)
{
  if (NULL != m_query_epoint)
  {
    return m_query_epoint(InWorkspace,
                          eptype,
                          pointnum,
                          result,
                          buflen);
  }
  *result = 0;
  return 1;
}

