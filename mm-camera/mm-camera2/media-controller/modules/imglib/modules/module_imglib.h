/**********************************************************************
* Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved. *
* Qualcomm Technologies Proprietary and Confidential.                 *
**********************************************************************/

#ifndef __MODULE_IMGLIB_H__
#define __MODULE_IMGLIB_H__

#include "camera_dbg.h"
#include "modules.h"
#include "mct_stream.h"

#define MODULE_IMGLIB_PORT_NAME_LEN 32
#define MODULE_IMGLIB_MAX_NAME_LENGH 50
#define MODULE_IMGLIB_STATIC_PORTS 1

#define MODULE_IMGLIB_MAX_TOPOLOGIES 20

typedef struct {
  int type;
  union {
    int enable_flag;
  }d;
} imglib_event_data_t;

typedef struct {
  int query_type;
  union {
    int img_feature_mask;
  }d;
} module_imglib_caps_t;

typedef struct {
  cam_format_t fomat;
  int width;
  int height;
} imglib_port_info_t;

void module_cac_set_parent(mct_module_t *p_mct_mod, mct_module_t *p_parent);


/** imglib_bm_pool_info_t
 *   @mutex: To serialize access to module resources
 *   @dummy_port: Dummy port needed for module events
 *   @imglib_modules: List holding pointers to active image modules
 *   @topo_num: Number of topologies
 *   @topo: Array of topology lists
 *     module is source and we need to redirect module events to port
 *     events
 *
 *   Imagelib module structure
 **/
typedef struct {
  pthread_mutex_t mutex;
  mct_port_t *dummy_port;
  mct_list_t *imglib_modules;
  uint32_t topology_num;
  mct_list_t **topology;
} module_imglib_t;

/**
 * Function: module_imglib_create_port
 *
 * Description: Create imglib port
 *
 * Arguments:
 *   @p_mct_mod: Pointer to imglib module
 *   @dir: Port direction
 *   @static_p: static created port
 *
 * Return values:
 *   MCTL port pointer \ NULL on fail
 *
 * Notes: Currently supported only source ports
 **/
mct_port_t *module_imglib_create_port(mct_module_t *p_mct_mod,
  mct_port_direction_t dir, boolean static_p);

/**
 * Function: module_imglib_free_port
 *
 * Description: This function is used to free the imglib ports
 *
 * Arguments:
 *   @p_mct_mod: Mctmodule instance pointer
 *   @p_port: Mct port which need to be freed
 *
 * Return values:
 *   TRUE/FALSE
 *
 * Notes: none
 **/
boolean module_imglib_free_port(mct_module_t *p_mct_mod, mct_port_t *p_port);

/**
 * Function: module_imglib_create_dummy_port
 *
 * Description: Create imglib dummy port
 *
 * Arguments:
 *   p_mct_mod - Pointer to imglib module
 *   dir - Port direction
 *
 * Return values:
 *     MCTL port pointer
 *
 * Notes: Currently supported only source ports
 **/
mct_port_t *module_imglib_create_dummy_port(mct_module_t *p_mct_mod,
  mct_port_direction_t dir);

/**
 * Function: module_imglib_get_port_with_identity
 *
 * Description: Search for reserved port with given identity
 *
 * Arguments:
 *   @p_mct_mod: Pointer to imglib module
 *   @identity: Identity to search for
 *   @dir: Port direction
 *
 * Return values:
 *     MCTL port pointer \ NULL on fail
 *
 **/
mct_port_t *module_imglib_get_port_with_identity(mct_module_t *p_mct_mod,
  unsigned int identity, mct_port_direction_t dir);

/**
 * Function: module_imglib_get_dyn_port_with_sessionid
 *
 * Description: Search for reserved dynamic port with given session id
 *
 * Arguments:
 *   @p_mct_mod - Pointer to imglib module
 *   @sessionid - Session id to serch for
 *   @dir - Port direction
 *
 * Return values:
 *     MCTL port pointer \ NULL on fail
 *
 **/
mct_port_t *module_imglib_get_dyn_port_with_sessionid(mct_module_t *p_mct_mod,
  unsigned int sessionid, mct_port_direction_t dir);

/**
 * Function: module_imglib_get_and_reserve_port
 *
 * Description: Search and reserve available port
 *
 * Arguments:
 *   @p_mct_mod - Pointer to imglib module
 *   @stream_info - Stream info
 *   @dir - Port direction
 *   @peer_cap - peer port capabilites
 *
 * Return values:
 *     MCTL port pointer \ NULL on fail
 *
 **/
mct_port_t *module_imglib_get_and_reserve_port(mct_module_t *p_mct_mod,
   mct_stream_info_t *stream_info, mct_port_direction_t dir, void *peer_cap);

/**
 * Function: module_imglib_get_topology
 *
 * Description: Get internal topology list based in stream info
 *
 * Arguments:
 *   @p_mod: Imagelib module
 *   @stream_info: mct_stream_info_t struct

 * Return values:
 *     TRUE/FALSE
 *
 * Notes: none
 **/
mct_list_t *module_imglib_get_topology(mct_module_t *module,
  mct_stream_info_t *stream_info);

/**
 * Function: module_cac_set_parent
 *
 * Description: Interface function for set parent of cac module
 *
 * Arguments:
 *   @p_mct_mod: Cac module
 *   @p_parent: Stream object to be set as cac module parent

 * Return values:
 *     TRUE/FALSE
 *
 * Notes: This is only temporal
 **/
void module_cac_set_parent(mct_module_t *p_mct_mod, mct_module_t *p_parent);

/**
 * Function: module_wnr_set_parent
 *
 * Description: Interface function for set parent of wnr module
 *
 * Arguments:
 *   @p_mct_mod: Cac module
 *   @p_parent: Stream object to be set as wnr module parent

 * Return values:
 *     TRUE/FALSE
 *
 * Notes: This is only temporal
 **/
void module_wnr_set_parent(mct_module_t *p_mct_mod, mct_module_t *p_parent);

#endif //__MODULE_IMGLIB_H__
