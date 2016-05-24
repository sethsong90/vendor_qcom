/**********************************************************************
* Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved. *
* Qualcomm Technologies Proprietary and Confidential.                 *
**********************************************************************/

#ifndef __MODULE_IMGLIB_DEFS_H__
#define __MODULE_IMGLIB_DEFS_H__

#include "module_imglib.h"
#include "camera_dbg.h"
#include "modules.h"

#define IMGLIB_MAX_TOPOLOGY_MODULES 10

/** module_imglib_topology_t
 *   @feature_mask: Stream type which this topology will be used
 *   @stream_type: Array of modules used as topology building blocks
 *   @modules: Topology modules
 *
 *   Imagelib topology structure
 **/
typedef struct {
  uint32_t feature_mask;
  cam_stream_type_t stream_type;
  mct_module_init_name_t modules[IMGLIB_MAX_TOPOLOGY_MODULES];
} module_imglib_topology_t;

/* Section for defining imglib internal topologies.
 *
 * Module definitions - Every module in the topology
 * need to be defined with 3 fields:
 *
 * - name: Module name
 * - init_mod: Module init function
 * - deinit_mod: Module deinit function
 *
 * Example:
 *
 * #define MOD_IMGLIB_EXAMPLE { \
 * .name = "imglib_example", \
 * .init_mod = module_example_init, \
 * .deinit_mod = module_example_deinit, } \
 *
 * Topology Definitions
 *  Internal topologies will be created per port. If topology is not
 *  available, port will only accept and redirect the events.
 *
 * 3 Fields:
 *
 * - feature_mask - Optional - If mask is not present it will not be used.
 * - stream_type - Mandatory - Stream type for which topology will be used.
 * - modules - Mandatory - Internal topology modules,
 *    first in the list will be connected with sink port of imglib,
 *    last in the list with source port of imglib,
 *    all modules will be connected with manner first->second->third...
 *
 * Topology selection is done based on stream type and feature mask,
 * if feature mask is not set it will not be used.
 * If there are topologies for same stream type but different masks
 * the topology with more features matching will be selected.
 *
 * Example:
 * Only one module in the topology:
 *
 *  { .stream_type = CAM_STREAM_TYPE_PREVIEW, \
 *     .modules = {MOD_IMGLIB_EXAMPLE}, \
 *  }, \
 *
 * How it looks inside imglib module:
 *  ------------- ------------
 *  ||->MOD_IMGLIB_EXAMPLE->||
 *  --------------------------
 *
 * With more modules in topology:
 *
 *
 *  { .stream_type = CAM_STREAM_TYPE_PREVIEW, \
 *     .modules = {MOD_IMGLIB_EXAMPLE1, MOD_IMGLIB_EXAMPLE2}, \
 *  }, \
 *
 * How it loks inside imglib module:
 *  ------------- ----------------------------------
 *  ||->MOD_IMGLIB_EXAMPLE1->MOD_IMGLIB_EXAMPLE2->||
 *  ------------------------------------------------
 */

/* Modules definition */
#define MOD_IMGLIB_FACEPROC { \
  .name = "imglib_faceproc", \
  .init_mod = module_faceproc_init, \
  .deinit_mod = module_faceproc_deinit, } \

#define MOD_IMGLIB_DENOISE { \
  .name = "imglib_denoise", \
  .init_mod = NULL, \
  .deinit_mod = NULL, } \

#define MOD_IMGLIB_HDR { \
  .name = "hdr", \
  .init_mod = module_hdr_init, \
  .deinit_mod = module_hdr_deinit, } \

#define MOD_IMGLIB_CAC { \
  .name = "imglib_cac", \
  .init_mod = NULL, \
  .deinit_mod = NULL, } \

/* Topology definitions */
#define MOD_IMGLIB_TOPOLOGY_REGISTER(t) \
  static module_imglib_topology_t (t)[] = { \
    { .stream_type = CAM_STREAM_TYPE_PREVIEW, \
      .modules = {MOD_IMGLIB_FACEPROC}, \
    }, \
    { .feature_mask = CAM_QCOM_FEATURE_DENOISE2D, \
      .stream_type = CAM_STREAM_TYPE_SNAPSHOT, \
      .modules = {MOD_IMGLIB_DENOISE}, \
    }, \
    { .feature_mask = CAM_QCOM_FEATURE_HDR, \
      .stream_type = CAM_STREAM_TYPE_OFFLINE_PROC, \
      .modules = {MOD_IMGLIB_HDR}, \
    }, \
    { .feature_mask = CAM_QCOM_FEATURE_REGISTER_FACE, \
      .stream_type = CAM_STREAM_TYPE_OFFLINE_PROC, \
      .modules = {MOD_IMGLIB_FACEPROC}, \
    }, \
  } \

#endif //__MODULE_IMGLIB_DEFS_H__
