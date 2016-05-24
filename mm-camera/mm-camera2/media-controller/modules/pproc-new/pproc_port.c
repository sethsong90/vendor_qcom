/*============================================================================

  Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/
#include "camera_dbg.h"
#include "cam_intf.h"
#include "cam_types.h"
#include "modules.h"
#include "mct_list.h"
#include "mct_module.h"
#include "mct_port.h"
#include "pproc_module.h"
#include "pproc_port.h"
#include "mct_controller.h"

#if 0
#define DEBUG_PPROC_PORT
#ifdef DEBUG_PPROC_PORT
#undef CDBG
#define CDBG CDBG_ERROR
#endif
#endif
typedef enum {
  PPROC_PORT_RESERVE_OK,
  PPROC_PORT_RESERVE_FAILED,
  PPROC_PORT_RESERVE_NEW_SESSION,
  PPROC_PORT_RESERVE_INVALID
} pproc_port_reserve_result_t;

typedef enum {
  PPROC_PORT_STATE_UNRESERVED,
  PPROC_PORT_STATE_RESERVED,
  PPROC_PORT_STATE_LINKED,
} pproc_port_state_t;

typedef enum _pproc_port_type {
  PPROC_PORT_TYPE_STREAM,
  PPROC_PORT_TYPE_CAPTURE,
  PPROC_PORT_TYPE_INVALID
} pproc_port_type_t;

/** _pproc_port_stream_info:
 *    @state:              state of port for this stream.
 *    @pproc_stream:       mct_stream object representing topology for
 *                         this stream;
 *    @divert_featue_mask: stores feature mask needed to decide
 *                         divert config.
 *
 *  stream info attached in pproc port
 **/
typedef struct _pproc_port_stream_info {
  pproc_port_state_t state;
  mct_stream_t      *pproc_stream;
  mct_stream_info_t *stream_info;
  mct_port_t        *int_link;
  uint32_t           divert_featue_mask;
} pproc_port_stream_info_t;

/** _pproc_port_private:
 *    @streams:            pproc port's attached stream information.
 *    @port_type:          pproc port type attached based on stream;
 *    @num_streams:        total streams attached on this port.
 *    @sessionid:          session id attached on this port.
 *
 *  pproc port's private data structure
 **/
typedef struct _pproc_port_private {
  pproc_port_stream_info_t streams[PPROC_MAX_STREAM_PER_PORT];
  pproc_port_type_t        port_type;
  uint32_t                 num_streams;
  uint32_t                 sessionid;
} pproc_port_private_t;

/** _pproc_port_match_data:
 *    @stream_info:     pproc port's attached stream information.
 *    @port:   pproc port type attached based on stream;
 *
 *  pproc port
 **/
typedef struct _pproc_port_match_data {
  mct_stream_info_t *stream_info;
  mct_port_t *port;
} pproc_port_match_data_t;

/** pproc_port_check_identity
 *    @d1: session+stream identity
 *    @d2: session+stream identity
 *
 *  To find out if both identities are matching;
 *  Return TRUE if matches.
 **/
static boolean pproc_port_check_identity(void *d1, void *d2)
{
  unsigned int v1, v2;

  v1 = *((unsigned int *)d1);
  v2 = *((unsigned int *)d2);

  return ((v1 == v2) ? TRUE : FALSE);
}

/** pproc_port_check_identity_in_port
 *    @data1: this port on which identity needs to be
 *            checked
 *    @data2: session+stream identity
 *
 *  To find out if identities is attached in port;
 *
 *  Return TRUE if matches.
 **/
boolean pproc_port_check_identity_in_port(void *data1, void *data2)
{
  boolean       rc = FALSE;
  mct_port_t   *port = (mct_port_t *)data1;
  unsigned int *identity = (unsigned int *)data2;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  /* check for sanity */
  if (!data1 || !data2) {
    CDBG_ERROR("%s:%d] error data1: %p, data2: %p\n", __func__, __LINE__,
      data1, data2);
    return FALSE;
  }

  MCT_OBJECT_LOCK(port);
  if (mct_list_find_custom(MCT_OBJECT_CHILDREN(port), identity,
    pproc_port_check_identity) != NULL) {
    rc = TRUE;
  }

  MCT_OBJECT_UNLOCK(port);
  CDBG("%s:%d] X rc: %d\n", __func__, __LINE__, rc);
  return rc;
}

/** pproc_port_set_divert_config
 *    @port: pproc module port
 *    @identity: Identity to update the divert event
 *
 *  Set divert config to the submodules. (currently only cpp,
 *  vpe and c2d). If more than one stream is linked then a
 *  second divert config event for dual streamon state is also
 *  sent to submodules
 *
 *  Return TRUE on success, otherwise return FALSE.
 **/
boolean pproc_port_set_divert_config(mct_port_t *pproc_port,
  pproc_port_stream_info_t *port_stream_info, uint32_t identity)
{
  pproc_port_private_t *port_private;
  mct_module_t         *pproc_module;
  mct_module_t         *cpp, *vpe, *c2d;
  boolean               enable_div_unproc = FALSE;
  boolean               enable_div_proc = FALSE;
  pproc_divert_info_t   divert_info;
  uint32_t              large_dim_identity = PPROC_INVALID_IDENTITY;
  uint32_t              small_dim_identity = PPROC_INVALID_IDENTITY;

  if (!pproc_port) {
    CDBG_ERROR("%s:%d] error pproc_port:%p\n", __func__, __LINE__,
      pproc_port);
    return FALSE;
  }

  /* Get port private to check divert config relevant flags for
     unproccessed divert/smartzoom etc., */
  port_private = MCT_OBJECT_PRIVATE(pproc_port);
  if (!port_private) {
    CDBG_ERROR("%s:%d] error port_private:%p\n", __func__, __LINE__,
      port_private);
    return FALSE;
  }

  /* Decide unprocessed divert is needed. Currently when downstream
     module is connected to PPROC we assume unprocessed divert is needed.
     The usecase for now is facedetection and is indicated by feature
     flag in port.*/
  if (port_stream_info->divert_featue_mask &
    PPROC_DIVERT_UNPROCESSED) {
    enable_div_unproc = TRUE;
  }

  /* Decide processed divert is needed. Currently when smartzoom is on,
     we assume processed divert is needed.*/
  if (port_stream_info->divert_featue_mask &
    PPROC_DIVERT_PROCESSED) {
    enable_div_proc = TRUE;
  }

  cpp = pproc_module_get_sub_mod(MCT_OBJECT_PARENT(pproc_port)->data, "cpp");
  vpe = pproc_module_get_sub_mod(MCT_OBJECT_PARENT(pproc_port)->data, "vpe");
  c2d = pproc_module_get_sub_mod(MCT_OBJECT_PARENT(pproc_port)->data, "c2d");

  if (cpp) {
    /* Send divert config info for single streamon for this identity*/
    /* Send the event to CPP submodule */
    memset(&divert_info, 0, sizeof(divert_info));
    divert_info.div_unproc_identity = PPROC_INVALID_IDENTITY;
    if (enable_div_unproc) {
      /* Set unprocess divert identity */
      divert_info.div_unproc_identity = identity;
      divert_info.divert_flags |= PPROC_DIVERT_UNPROCESSED;
    }
    /* Set Process identity */
    divert_info.proc_identity[0] = identity;
    divert_info.div_proc_identity[0] = PPROC_INVALID_IDENTITY;

    if ((port_private->num_streams < 2) && (enable_div_proc)) {
      /* Set process divert identity */
      divert_info.div_proc_identity[0] = identity;
      divert_info.divert_flags |= PPROC_DIVERT_PROCESSED;
    }

    divert_info.proc_identity[1] = PPROC_INVALID_IDENTITY;
    divert_info.div_proc_identity[1] = PPROC_INVALID_IDENTITY;
    divert_info.num_passes = 1;
    divert_info.divert_flags |= PPROC_PROCESS;
    pproc_port_send_divert_config_event(cpp, identity,
      PPROC_CFG_UPDATE_SINGLE, port_stream_info->int_link, &divert_info);

    /* Send the event to C2D submodule */
    memset(&divert_info, 0, sizeof(divert_info));
    /* It is always NO_OP */
    if (c2d) {
    /* send divert config to c2d in case of EIS 2.0 */
      if (port_stream_info->stream_info->is_type == IS_TYPE_EIS_2_0) {
        divert_info.div_unproc_identity = PPROC_INVALID_IDENTITY;
        /* Set Process identity */
        divert_info.proc_identity[0] = identity;
        divert_info.div_proc_identity[0] = PPROC_INVALID_IDENTITY;

        divert_info.proc_identity[1] = PPROC_INVALID_IDENTITY;
        divert_info.div_proc_identity[1] = PPROC_INVALID_IDENTITY;
        divert_info.num_passes = 1;
        divert_info.divert_flags |= PPROC_PROCESS;
        pproc_port_send_divert_config_event(c2d, identity,
          PPROC_CFG_UPDATE_SINGLE, port_stream_info->int_link, &divert_info);
      }

    }

    /* Send divert config info for dual streamon if there are 2 streams*/
    if (port_private->num_streams > 1) {
      /* Send the event to CPP submodule */
      memset(&divert_info, 0, sizeof(divert_info));
      divert_info.div_unproc_identity = PPROC_INVALID_IDENTITY;
      if (enable_div_unproc) {
        /* Set unprocess divert identity */
        divert_info.div_unproc_identity = identity;
        divert_info.divert_flags |= PPROC_DIVERT_UNPROCESSED;
      }
      /* Find the large dimension and set process in CPP */
      if ((port_private->streams[0].stream_info->dim.width >
        port_private->streams[1].stream_info->dim.width) &&
        (port_private->streams[0].stream_info->dim.height >
        port_private->streams[1].stream_info->dim.height)) {
        /* Set stream[0] in CPP */
        large_dim_identity = port_private->streams[0].stream_info->identity;
        small_dim_identity = port_private->streams[1].stream_info->identity;
      } else {
        /* Set stream[0] in CPP */
        small_dim_identity = port_private->streams[0].stream_info->identity;
        large_dim_identity = port_private->streams[1].stream_info->identity;
      }
      divert_info.proc_identity[0] = large_dim_identity;
      divert_info.div_proc_identity[0] = PPROC_INVALID_IDENTITY;
      divert_info.proc_identity[1] = PPROC_INVALID_IDENTITY;
      divert_info.div_proc_identity[1] = PPROC_INVALID_IDENTITY;
      divert_info.num_passes = 1;
      divert_info.divert_flags |= PPROC_PROCESS;
      /* If processed divert is enabled set process in C2D and set
         processed divert in CPP. Else set second process in CPP */
      if (enable_div_proc) {
        /* Set process divert identity */
        divert_info.div_proc_identity[0] = small_dim_identity;
        divert_info.divert_flags |= PPROC_DIVERT_PROCESSED;
      } else {
        divert_info.proc_identity[1] = small_dim_identity;
        divert_info.div_proc_identity[1] = PPROC_INVALID_IDENTITY;
        divert_info.num_passes++;
      }
      pproc_port_send_divert_config_event(cpp, identity,
        PPROC_CFG_UPDATE_DUAL, port_stream_info->int_link, &divert_info);

      /* Send the event to C2D submodule */
      memset(&divert_info, 0, sizeof(divert_info));
      if (enable_div_proc) {
        /* Set process divert identity */
        divert_info.proc_identity[0] = small_dim_identity;
        divert_info.div_proc_identity[0] = PPROC_INVALID_IDENTITY;
        divert_info.proc_identity[1] = PPROC_INVALID_IDENTITY;
        divert_info.div_proc_identity[1] = PPROC_INVALID_IDENTITY;
        divert_info.num_passes = 1;
        divert_info.divert_flags |= PPROC_PROCESS;
        if (c2d)
          pproc_port_send_divert_config_event(c2d, identity,
            PPROC_CFG_UPDATE_DUAL, port_stream_info->int_link, &divert_info);
      }
     /* send divert config to c2d in case of EIS 2.0 */
     if (port_stream_info->stream_info->is_type == IS_TYPE_EIS_2_0) {
       if (c2d) {
         memset(&divert_info, 0, sizeof(divert_info));
         divert_info.div_unproc_identity = PPROC_INVALID_IDENTITY;
         divert_info.proc_identity[0] = large_dim_identity;
         divert_info.div_proc_identity[0] = PPROC_INVALID_IDENTITY;
         divert_info.proc_identity[1] = PPROC_INVALID_IDENTITY;
         divert_info.div_proc_identity[1] = PPROC_INVALID_IDENTITY;
         divert_info.num_passes = 1;
         divert_info.divert_flags |= PPROC_PROCESS;
         pproc_port_send_divert_config_event(c2d, identity,
           PPROC_CFG_UPDATE_DUAL, port_stream_info->int_link, &divert_info);
       }
     }
    }
  } else if (vpe) {
  } else if (c2d) {
    /* Send divert config info for single streamon for this identity*/
    memset(&divert_info, 0, sizeof(divert_info));
    divert_info.div_unproc_identity = PPROC_INVALID_IDENTITY;
    if (enable_div_unproc) {
      /* Set unprocess divert identity */
      divert_info.div_unproc_identity = identity;
      divert_info.divert_flags |= PPROC_DIVERT_UNPROCESSED;
    }
    /* Set Process identity */
    divert_info.proc_identity[0] = identity;
    divert_info.div_proc_identity[0] = PPROC_INVALID_IDENTITY;

    divert_info.proc_identity[1] = PPROC_INVALID_IDENTITY;
    divert_info.div_proc_identity[1] = PPROC_INVALID_IDENTITY;
    divert_info.num_passes = 1;
    divert_info.divert_flags |= PPROC_PROCESS;
    pproc_port_send_divert_config_event(c2d, identity,
      PPROC_CFG_UPDATE_SINGLE, port_stream_info->int_link, &divert_info);

    /* Send divert config info for dual streamon if there are 2 streams*/
    if (port_private->num_streams > 1) {
      memset(&divert_info, 0, sizeof(divert_info));
      divert_info.div_unproc_identity = PPROC_INVALID_IDENTITY;
      if (enable_div_unproc) {
        /* Set unprocess divert identity */
        divert_info.div_unproc_identity = identity;
        divert_info.divert_flags |= PPROC_DIVERT_UNPROCESSED;
      }
      divert_info.proc_identity[0] =
        port_private->streams[0].stream_info->identity;
      divert_info.div_proc_identity[0] = PPROC_INVALID_IDENTITY;
      divert_info.proc_identity[1] =
        port_private->streams[1].stream_info->identity;
      divert_info.div_proc_identity[1] = PPROC_INVALID_IDENTITY;
      divert_info.num_passes = 2;
      divert_info.divert_flags |= PPROC_PROCESS;
      pproc_port_send_divert_config_event(c2d, identity,
        PPROC_CFG_UPDATE_DUAL, port_stream_info->int_link, &divert_info);

    }
   }
  return TRUE;
}

/** pproc_port_reserve_compatible_port
 *    @data1: submods port
 *    @data2: stream attributes used to reserve this port;
 *
 *  To reserve port on module in stream.
 *
 *  Reserve status from submod
 **/
static boolean pproc_port_reserve_compatible_port(void *data1, void *data2)
{
  mct_port_t        *port = (mct_port_t *)data1;
  pproc_port_match_data_t *match_data = (pproc_port_match_data_t *)data2;
  mct_stream_info_t *stream_info;
  mct_port_t *pproc_port;

  if (!port || !match_data) {
    CDBG_ERROR("%s:%d] error port: %p match_data: %p\n", __func__, __LINE__,
      port, match_data);
    return FALSE;
  }

  stream_info = match_data->stream_info;
  pproc_port = match_data->port;
  /* Adopt the same logic as mct_port_check_link to avoid different cont. port mapped to
      same submod port */
  if (port->peer != NULL && port->peer != pproc_port) {
    return FALSE;
  }

  return port->check_caps_reserve(port, NULL, stream_info);
}

/** pproc_port_resrv_port_on_module
 *    @submod:      pproc's submodule in stream
 *    @stream_info: stream attributes used to reserve the port;
 *
 *  To request and reserve port on sub module in stream.
 *
 *  Return port from sub module if success
 **/
static mct_port_t *pproc_port_resrv_port_on_module(mct_module_t *submod,
  mct_stream_info_t *stream_info, mct_port_direction_t direction,
  mct_port_t *pproc_port)
{
  mct_port_t *sub_port = NULL;
  boolean     rc = FALSE;
  mct_list_t *port_holder;
  mct_list_t *submod_port_holder;
  pproc_port_match_data_t port_match_data;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  if (!submod || !stream_info) {
    CDBG_ERROR("%s:%d] error submod: %p, stream_info: %p\n", __func__,
      __LINE__, submod, stream_info);
    return NULL;
  }

  submod_port_holder = MCT_MODULE_SRCPORTS(submod);
  if (direction == MCT_PORT_SINK) {
    submod_port_holder = MCT_MODULE_SINKPORTS(submod);
  }

  /* traverse through the allowed ports in the module, trying to find a
     compatible port */
  port_match_data.stream_info = stream_info;
  port_match_data.port = pproc_port;
  port_holder = mct_list_find_custom(submod_port_holder, &port_match_data,
    pproc_port_reserve_compatible_port);

  if (!port_holder) {
    if (submod->request_new_port) {
      sub_port = submod->request_new_port(stream_info, direction, submod, NULL);

      if (!sub_port) {
        CDBG_ERROR("%s:%d] error creating submod sink port\n", __func__,
          __LINE__);
      }
    }
  } else {
    sub_port = (mct_port_t *)port_holder->data;
  }

  CDBG("%s:%d] X sub_port: %p\n", __func__, __LINE__, sub_port);
  return sub_port;
}

/** pproc_port_get_reserved_port
 *    @module: pproc module
 *    @identity: identity for which attached stream info will be
 *             returned
 *
 *  Return pproc sink port for this module
 *
 *  Return SUCCESS: attached port
 *         FAILURE: NULL
 **/
mct_port_t *pproc_port_get_reserved_port(mct_module_t *module,
  unsigned int identity)
{
  mct_list_t *p_list = NULL;

  /* Validate input parameters */
  if (!module) {
    CDBG_ERROR("%s:%d failed: module NULL\n", __func__, __LINE__);
    goto ERROR;
  }
  /* Lock module mutex */
  MCT_OBJECT_LOCK(module);

  /* Pick pproc sink port for this identity */
  p_list = mct_list_find_custom(MCT_MODULE_SINKPORTS(module),
    &identity, pproc_port_check_identity_in_port);

  /* Unlock module mutex */
  MCT_OBJECT_UNLOCK(module);

  if (!p_list) {
    CDBG_ERROR("%s:%d error no matching sink port found\n", __func__, __LINE__);
    goto ERROR;
  }

  /* Return attached port */
  return p_list->data;
ERROR:
  /* Error - return NULL */
  return NULL;
}

/** pproc_port_get_attached_stream_info
 *    @module: pproc module
 *    @identity: identity for which attached stream info will be
 *             returned
 *
 *  1) Find pproc sink port for this module
 *  2) Find pproc port private for the sink port
 *  3) Match stream id and extract stream info from port private
 *
 *  Return SUCCESS: attached stream info
 *         FAILURE: NULL
 **/
mct_stream_info_t *pproc_port_get_attached_stream_info(mct_port_t *port,
  unsigned int identity)
{
  boolean               rc = TRUE;
  unsigned int          i = 0;
  mct_stream_info_t    *stream_info = NULL;
  pproc_port_private_t *port_private = NULL;

  /* Validate input parameters */
  if (!port) {
    CDBG_ERROR("%s:%d failed: port NULL\n", __func__, __LINE__);
    goto ERROR;
  }

  /* Lock port mutex */
  MCT_OBJECT_LOCK(port);

  /* Pick port private */
  port_private = (pproc_port_private_t *)MCT_OBJECT_PRIVATE(port);
  if (!port_private) {
    CDBG_ERROR("%s:%d failed: port private NULL\n", __func__, __LINE__);
    /* Unlock port mutex */
    MCT_OBJECT_UNLOCK(port);
    goto ERROR;
  }

  /* Find stream info from port private */
  for (i = 0; i < PPROC_MAX_STREAM_PER_PORT; i++) {
    if (port_private->streams[i].stream_info &&
       (port_private->streams[i].stream_info->identity == identity)) {
      stream_info = port_private->streams[i].stream_info;
    }
  }

  /* Unlock module mutex */
  MCT_OBJECT_UNLOCK(port);

  /* Return stream info */
  return stream_info;
ERROR:
  /* Error - return NULL */
  return NULL;
}

/** pproc_port_match_module_type
 *    @stream: pproc first submodule in stream
 *
 *  To match module based on module type.
 *
 *  Return success if module type matches
 **/
static boolean pproc_port_match_module_type(void *data1, void *data2)
{
  mct_module_t      *module = (mct_module_t *)data1;
  mct_module_type_identity_t *mod_type =
    (mct_module_type_identity_t *)data2;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  if (!module || !mod_type) {
    CDBG_ERROR("%s:%d] error module: %p, mod_type: %p\n", __func__,
      __LINE__, module, mod_type);
    return FALSE;
  }

  if ((mct_module_find_type(module, mod_type->identity)) == mod_type->type) {
    CDBG("%s:%d] X\n", __func__, __LINE__);
    return TRUE;
  }

  CDBG("%s:%d] X\n", __func__, __LINE__);
  return FALSE;
}

static boolean pproc_port_add_modules_to_stream(
  pproc_port_stream_info_t *port_stream_info, mct_module_t *submod1,
  mct_module_t *submod2, mct_stream_info_t *stream_info, mct_port_t *port)
{
  boolean      rc = FALSE;
  unsigned int module_type = MCT_MODULE_FLAG_SINK;

  CDBG("%s:%d] E\n", __func__, __LINE__);

  if (submod2 && submod1) {
    /* link ops_mod(cac or wnr) + must_mod(cpp or vpe) */
    submod1->set_mod(submod1, MCT_MODULE_FLAG_SOURCE,
      stream_info->identity);
    submod2->set_mod(submod2, MCT_MODULE_FLAG_SINK,
      stream_info->identity);
    port_stream_info->int_link = pproc_port_resrv_port_on_module(submod1,
      stream_info, MCT_PORT_SINK, port);
    if (port_stream_info->int_link) {
      //port_stream_info->int_link->peer = port;
      /* Invoke ext link for the submod port */
      port_stream_info->int_link->ext_link(stream_info->identity,
        port_stream_info->int_link, port);
      rc = mct_port_add_child(stream_info->identity,
        port_stream_info->int_link);
      if (rc == TRUE) {
        rc = mct_stream_link_modules(port_stream_info->pproc_stream,
          submod1, submod2, NULL);
      } else {
        CDBG_ERROR("%s:%d] error adding child\n", __func__, __LINE__);
      }
      /* TODO: Set the divert information to submods */
    }
  } else {
    mct_module_t *submod = submod1;
    if (!submod) {
      submod = submod2;
    }
    if (submod) {
      /* only must_mod(cpp or vpe) is existing */
      submod->set_mod(submod, module_type,
        stream_info->identity);
      port_stream_info->int_link = pproc_port_resrv_port_on_module(submod,
        stream_info, MCT_PORT_SINK,port);
      if (port_stream_info->int_link) {
        //port_stream_info->int_link->peer = port;
        /* Invoke ext link for the submod port */
        port_stream_info->int_link->ext_link(stream_info->identity,
          port_stream_info->int_link, port);
        rc = mct_port_add_child(stream_info->identity,
          port_stream_info->int_link);
        if (rc == TRUE) {
          rc = mct_object_set_parent(MCT_OBJECT_CAST(submod),
            MCT_OBJECT_CAST(port_stream_info->pproc_stream));
        } else {
          CDBG_ERROR("%s:%d] error adding child\n", __func__, __LINE__);
        }
      }
    } else {
      CDBG_ERROR("%s:%d] error submod1 & submode2 are NULL\n", __func__,
        __LINE__);
    }
  }

  /* TODO: Under error need to clean up the stream and reserve/links */
  CDBG("%s:%d] X rc: %d\n", __func__, __LINE__, rc);
  return rc;
}

/** pproc_port_create_stream_topology
 *    @pproc:            pproc module
 *    @port:             this port where topology is created.
 *    @port_stream_info: pproc port stream info structure to
 *                       store stream & related attributes.
 *    @stream_info:      stream attributes used to reserve this
 *                       port;
 *
 *  To create a topoloty based on stream's information.
 *
 *  cpp or vpe is a must module for pproc. Need to check stream
 *  attributes to identify if other sub-modules are needed.
 *
 *  Return TRUE if topology stream is created.
 **/
static boolean pproc_port_create_stream_topology(mct_module_t *pproc,
  mct_port_t *port, pproc_port_stream_info_t *port_stream_info,
  mct_stream_info_t *stream_info)
{
  mct_module_t *submod1 = NULL, *submod2 = NULL, *ops_submod = NULL;
  mct_module_t *cac = NULL, *wnr = NULL, *cpp = NULL, *vpe = NULL, *c2d = NULL;
  boolean       rc = TRUE;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  port_stream_info->pproc_stream =
    mct_stream_new(stream_info->identity & 0x0000FFFF);

  if (port_stream_info->pproc_stream == NULL) {
    CDBG_ERROR("%s:%d] error in stream creation\n", __func__, __LINE__);
    return FALSE;
  }

  port_stream_info->pproc_stream->streaminfo = *stream_info;
  port_stream_info->pproc_stream->buffers = stream_info->stream->buffers;

  port_stream_info->pproc_stream->streaminfo.stream =
    port_stream_info->pproc_stream;

  cpp = pproc_module_get_sub_mod(MCT_OBJECT_PARENT(port)->data, "cpp");
  submod1 = cpp;
  if (!submod1) {
    vpe = pproc_module_get_sub_mod(MCT_OBJECT_PARENT(port)->data,"vpe");
    submod1 = vpe;
  }
  c2d = pproc_module_get_sub_mod(MCT_OBJECT_PARENT(port)->data,"c2d");
  if (!submod1) {
    submod1 = c2d;
  } else {
    submod2 = c2d;
  }

  if (!submod1) {
    CDBG_ERROR("%s:%d] error because both cpp/vpe cannot be NULL\n", __func__,
      __LINE__);
    rc = FALSE;
    goto create_topology_done;
  }

  CDBG("%s:%d] feature mask 0x%x 0x%x", __func__, __LINE__,
    port_stream_info->pproc_stream->streaminfo.pp_config.feature_mask,
    port_stream_info->pproc_stream->streaminfo.reprocess_config.
    pp_feature_config.feature_mask);
  if ((port_stream_info->pproc_stream->streaminfo.pp_config.feature_mask &
    CAM_QCOM_FEATURE_CAC) ||
    ((port_stream_info->pproc_stream->streaminfo.reprocess_config.
     pp_feature_config.feature_mask & CAM_QCOM_FEATURE_CAC))) {
    cac = pproc_module_get_sub_mod(MCT_OBJECT_PARENT(port)->data, "cac");
    ops_submod = cac;
  } else {
    ops_submod = NULL;
  }
  if (!ops_submod) {
    wnr = pproc_module_get_sub_mod(MCT_OBJECT_PARENT(port)->data, "wnr");
    ops_submod = wnr;
  }

  port_stream_info->int_link = NULL;
  /* If this is a single module stream, just need to get its sink port;
   * otherwise, need to determin which module needs to be added and use
   * mct_stream_link_modules to link them
   * 1. Set the correct module type per identity
   * 2. Request and reserve the sink port on first module
   * 3. Set the internal link
   * 4. Add identity as child to sink port
   * 5. Add the modules to stream and link them if needed.
   * 6. If applicable set the divert information */
  switch (stream_info->stream_type) {
  case CAM_STREAM_TYPE_POSTVIEW: {
    rc = pproc_port_add_modules_to_stream(port_stream_info, submod1,
      NULL, stream_info, port);
  }
    break;

  case CAM_STREAM_TYPE_PREVIEW: {
    if (stream_info->is_type == IS_TYPE_EIS_2_0) {
      rc = pproc_port_add_modules_to_stream(port_stream_info, submod2,
        submod1, stream_info, port);
    } else {
      rc = pproc_port_add_modules_to_stream(port_stream_info, submod1,
        submod2, stream_info, port);
    }
  }
    break;

  case CAM_STREAM_TYPE_SNAPSHOT: {
    rc = pproc_port_add_modules_to_stream(port_stream_info, ops_submod,
      submod1, stream_info, port);
  }
    break;

  case CAM_STREAM_TYPE_VIDEO: {
    if (stream_info->is_type == IS_TYPE_EIS_2_0) {
      rc = pproc_port_add_modules_to_stream(port_stream_info, submod2,
        submod1, stream_info, port);
    } else {
      rc = pproc_port_add_modules_to_stream(port_stream_info, submod1,
        submod2, stream_info, port);
    }
  }
    break;

  case CAM_STREAM_TYPE_OFFLINE_PROC: {
    rc = pproc_port_add_modules_to_stream(port_stream_info, ops_submod,
      submod1, stream_info, port);
  }
    break;

  case CAM_STREAM_TYPE_RAW:
  default:
    /* pproc should not be linked !*/
    CDBG_ERROR("%s:%d] error in pproc link\n", __func__, __LINE__);
    rc = FALSE;
    break;
  } /* switch (stream_info->stream_type) */

create_topology_done:
  if (rc == FALSE) {
    if (port_stream_info->int_link) {
      port_stream_info->int_link->check_caps_unreserve(
        port_stream_info->int_link, stream_info->identity);
      port_stream_info->int_link = NULL;
    }
    mct_stream_destroy(port_stream_info->pproc_stream);
  }

  CDBG("%s:%d] X rc: %d\n", __func__, __LINE__, rc);
  return rc;
}

/** pproc_port_destroy_stream_topology
 *    @pproc:            pproc module
 *    @port:             this port where topology is to deleted.
 *    @port_stream_info: pproc port stream info structure to
 *                       store stream & related attributes.
 *    @stream_info:      stream attributes used to reserve this
 *                       port;
 *
 *  To delete a topology based on stream's information.
 *
 *  Return TRUE if topology stream is deleted.
 **/
static boolean pproc_port_destroy_stream_topology(mct_module_t *pproc,
  mct_port_t *port, pproc_port_stream_info_t *port_stream_info,
  mct_stream_info_t *stream_info)
{
  mct_port_t   *int_link;
  boolean       rc = TRUE;
  mct_stream_t *stream;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  if (!pproc || !port || !port_stream_info || !stream_info) {
    CDBG_ERROR("%s:%d] error pproc:%p, port:%p, port_stream_info:%p\n",
      __func__, __LINE__, pproc, port, port_stream_info);
    return FALSE;
  }

  int_link = port_stream_info->int_link;
  stream = port_stream_info->pproc_stream;
  /* 1. For the sink port of first module (int_link)in stream do the following,
         a. Do the ext unlink
         b. Do caps unreserve
         c. Remove identity (port's children)
     2. If stream children is more than 2 then operate_unlink on submods
     3. Detach module/stream's child/parent relationship */
  port_stream_info->int_link->un_link(stream_info->identity, int_link, port);
  rc = port_stream_info->int_link->check_caps_unreserve(int_link,
    stream_info->identity);
  if (rc == FALSE) {
    CDBG_ERROR("%s:%d] error in caps unreserve\n", __func__, __LINE__);
    return FALSE;
  }

  mct_port_remove_child(stream_info->identity, int_link);
  port_stream_info->int_link = NULL;

  if (MCT_OBJECT_NUM_CHILDREN(stream) > 1) {
    mct_list_operate_nodes(MCT_OBJECT_CHILDREN(stream),
      mct_stream_operate_unlink, stream);
  } else {
    /* Type is removed in unlink modules,since we have only one
     * module it is not linked, remove type here */
    mct_module_t *single_module = MCT_OBJECT_CHILDREN(stream)->data;
    mct_module_remove_type(single_module, stream->streaminfo.identity);
  }


  /* 1. free stream from module's parent list;
     2. free module object from stream's children list */
  mct_list_free_all_on_data(MCT_OBJECT_CHILDREN(stream),
    mct_stream_remvove_stream_from_module, stream);
  MCT_OBJECT_CHILDREN(stream) = NULL;
  MCT_STREAM_NUM_CHILDREN(stream) = 0;
  pthread_mutex_destroy(MCT_OBJECT_GET_LOCK(stream));
  free(port_stream_info->pproc_stream);
  port_stream_info->pproc_stream = NULL;

  CDBG("%s:%d] X\n", __func__, __LINE__);
  return rc;
}

/** pproc_port_check_port_state
 *    @port:        this pproc module's port;
 *    @stream_info: stream attributes used to reserve this port;
 *
 *  To check the state of port based on stream's information.
 *
 *  If the session ID is different, support can be provided via
 *  create a new port.
 *
 *  Once capabilities are matched,
 *  - If this port has not been used, it can be supported;
 *  - If the requested stream is in existing identity, return
 *    failure;
 *  - If the requested stream belongs to a different session,
 *    the port can not be used;
 *
 *  Return TRUE if port can be reserved.
 **/
static boolean pproc_port_check_port_state(mct_port_t *port,
  mct_stream_info_t *stream_info)
{
  boolean               rc = TRUE;
  pproc_port_private_t *port_private;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  if ((port->caps.port_caps_type != MCT_PORT_CAPS_FRAME) ||
    (port->caps.u.frame.format_flag != MCT_PORT_CAP_FORMAT_YCBCR) ||
    (port->caps.u.frame.size_flag != MCT_PORT_CAP_SIZE_20MB)) {
    CDBG_ERROR("%s:%d error because port_pproc caps is wrong.\n", __func__,
      __LINE__);
    rc = FALSE;
    goto check_port_state_done;
  }

  port_private = (pproc_port_private_t *)MCT_OBJECT_PRIVATE(port);

  if(port_private->num_streams >= PPROC_MAX_STREAM_PER_PORT) {
    CDBG("%s:%d] info: max streams reached\n", __func__, __LINE__);
    rc = FALSE;
    goto check_port_state_done;
  }

  if ((port_private->sessionid) &&
    (port_private->sessionid != (stream_info->identity & 0xFFFF0000))) {
    CDBG("%s:%d] info: reserved by session:%d\n", __func__, __LINE__,
      port_private->sessionid);
    rc = FALSE;
    goto check_port_state_done;
  }

  switch(port_private->port_type) {
  case PPROC_PORT_TYPE_INVALID: {
    port_private->num_streams = 0;
    if (stream_info->streaming_mode == CAM_STREAMING_MODE_CONTINUOUS) {
      port_private->port_type = PPROC_PORT_TYPE_STREAM;
    } else if (stream_info->streaming_mode == CAM_STREAMING_MODE_BURST) {
      port_private->port_type = PPROC_PORT_TYPE_CAPTURE;
    }
  }
    break;

  case PPROC_PORT_TYPE_STREAM: {
    if ((stream_info->streaming_mode != CAM_STREAMING_MODE_CONTINUOUS) ||
      (stream_info->stream_type == CAM_STREAM_TYPE_OFFLINE_PROC)) {
      CDBG("%s:%d] info: streaming mode doesn't match\n", __func__, __LINE__);
      rc = FALSE;
    }
  }
    break;

  case PPROC_PORT_TYPE_CAPTURE:
    /* for capture port, only one stream per port */
    rc = FALSE;
    break;
  default:
    CDBG_ERROR("%s:%d] error bad port_type=%d\n", __func__, __LINE__,
      port_private->port_type);
    rc = FALSE;
    break;
  }

check_port_state_done:
  CDBG("%s:%d] X rc: %d\n", __func__, __LINE__, rc);
  return rc;
}

/** pproc_port_sink_check_caps_reserve
 *    @port:        this pproc module's port;
 *    @peer_caps:   the capability of peer port which wants to
 *                  match interface port;
 *    @stream_info: stream attributes used to reserve this port;
 *
 *  To reserve a sink port based on stream's information.
 *
 *  If the session ID is different, support can be provided via
 *  create a new port in case sub-module can do so during
 *  request new port.
 *
 *  Once capabilities are matched,
 *  - If this port has not been used, it can be supported;
 *  - If the requested stream is in existing identity, return
 *    failure;
 *  - If the requested stream belongs to a different session,
 *    the port can not be used;
 *  - For both sink port and source port, need to handle
 *    internal  cap reserve.
 *
 *  Return TRUE if port can be reserved.
 **/
static boolean pproc_port_sink_check_caps_reserve(mct_port_t *port, void *caps,
  void *info)
{
  int                         i;
  boolean                     rc = TRUE;
  mct_port_caps_t            *peer_caps;
  pproc_port_private_t       *port_private;
  mct_module_t               *module;
  mct_list_t                 *list;
  pproc_port_reserve_result_t result;
  mct_stream_info_t          *stream_info = (mct_stream_info_t *)info;

  CDBG("%s:%d] E\n", __func__, __LINE__);

  MCT_OBJECT_LOCK(port);

  if (!port || !caps || !stream_info ||
      strncmp(MCT_OBJECT_NAME(port), "pproc_sink", strlen("pproc_sink"))) {
    CDBG_ERROR("%s: error because invalid parameters!\n", __func__);
    rc = FALSE;
    goto reserve_done;
  }
  peer_caps = (mct_port_caps_t *)caps;

  if (peer_caps && peer_caps->port_caps_type != MCT_PORT_CAPS_FRAME) {
    CDBG_LOW("%s:%d] error because caps Type:%d not supported.\n", __func__,
      __LINE__, peer_caps->port_caps_type);
    rc = FALSE;
    goto reserve_done;
  }

  if (pproc_port_check_port_state(port, stream_info) == FALSE) {
    rc = FALSE;
    goto reserve_done;
  }

  port_private = (pproc_port_private_t *)MCT_OBJECT_PRIVATE(port);
  /* reserve the port for this stream */
  for(i=0; i < PPROC_MAX_STREAM_PER_PORT; i++) {
    if(port_private->streams[i].state == PPROC_PORT_STATE_UNRESERVED) {
      port_private->streams[i].state = PPROC_PORT_STATE_RESERVED;
      port_private->streams[i].stream_info = stream_info;
      port_private->num_streams++;
      rc = pproc_port_create_stream_topology(MCT_PORT_PARENT(port)->data, port,
        &port_private->streams[i], stream_info);

      if (rc == TRUE) {
        port_private->sessionid = stream_info->identity & 0xFFFF0000;
        rc = pproc_port_set_divert_config(port, &port_private->streams[i],
          stream_info->identity);
      }

      break;
    }
  }

  if(i == PPROC_MAX_STREAM_PER_PORT) {
    CDBG_ERROR("%s:%d] error, unexpected error!!!", __func__, __LINE__);
    /* TODO: need to reset the port_type and streaming_mode and handle
       num_streams */
    rc = FALSE;
  }

reserve_done:
  MCT_OBJECT_UNLOCK(port);
  CDBG("%s:%d] X rc: %d\n", __func__, __LINE__, rc);
  return rc;
}

/** pproc_port_sink_check_caps_unreserve
 *    @port: this port object to remove the session/stream;
 *    @identity: session+stream identity.
 *
 *    Return FALSE if the identity is not existing.
 *
 *  This function unreserves the identity on this port.
 **/
static boolean pproc_port_sink_check_caps_unreserve(mct_port_t *port,
  unsigned int identity)
{
  int                         i;
  boolean                     rc = TRUE;
  pproc_port_private_t       *port_private;
  mct_module_t               *module;
  mct_list_t                 *list;
  pproc_port_reserve_result_t result;

  CDBG("%s:%d] E\n", __func__, __LINE__);

  MCT_OBJECT_LOCK(port);

  if (!port ||
      strncmp(MCT_OBJECT_NAME(port), "pproc_sink", strlen("pproc_sink"))) {
    CDBG_ERROR("%s: error because invalid parameters!\n", __func__);
    rc = FALSE;
    goto pproc_unreserve_done;
  }

  module = MCT_MODULE_CAST(MCT_PORT_PARENT(port)->data);
  /* 1. Check the identity is mapped
     2. Destroy stream topology
     3. Change stream state to UNRESERVED and decrement num_streams
     4. If num_streams is zero change port_type to invalid & remove session */
  port_private = (pproc_port_private_t *)MCT_OBJECT_PRIVATE(port);
  for (i=0; i < PPROC_MAX_STREAM_PER_PORT; i++) {
    if ((port_private->streams[i].state == PPROC_PORT_STATE_RESERVED) &&
      (port_private->streams[i].stream_info->identity == identity)) {
      port_private->streams[i].state = PPROC_PORT_STATE_UNRESERVED;
      rc = pproc_port_destroy_stream_topology(MCT_PORT_PARENT(port)->data,
        port, &port_private->streams[i],
        port_private->streams[i].stream_info);

      if (rc == TRUE) {
        port_private->num_streams--;
        port_private->streams[i].divert_featue_mask = 0;
        if (!port_private->num_streams) {
          port_private->sessionid = 0;
          port_private->port_type = PPROC_PORT_TYPE_INVALID;
        }
        port_private->streams[i].stream_info = NULL;
      }
      break;
    }
  }

  if(i == PPROC_MAX_STREAM_PER_PORT) {
    CDBG_ERROR("%s:%d] error, unexpected error!!!", __func__, __LINE__);
    /* TODO: need to reset the port_type and streaming_mode and handle
       num_streams */
    rc = FALSE;
  }

  /* Because MCT is not calling the set_mod for every stream that is reserved
     we clear it here. */
  module->set_mod(module, MCT_MODULE_FLAG_INVALID, identity);

pproc_unreserve_done:
  MCT_OBJECT_UNLOCK(port);
  CDBG("%s:%d] X rc: %d\n", __func__, __LINE__, rc);
  return rc;
}

/** pproc_port_source_check_caps_reserve
 *    @port:        this pproc module's source port;
 *    @peer_caps:   the capability of peer port which wants to
 *                  match interface port;
 *    @stream_info: stream attributes used to reserve this port;
 *
 *  To reserve a source port based on stream's information.
 *
 *  If the session ID is different, support can be provided via
 *  create a new port.
 *
 *  Once capabilities are matched,
 *  - If this port has not been used, it can be supported;
 *  - If the requested stream is in existing identity, return
 *    failure;
 *  - If the requested stream belongs to a different session,
 *    the port can not be used;
 *  - Might need to create new topology; but for now only
 *    possible case is unprocessed divert path.
 *
 *  Return TRUE if port can be reserved.
 **/
static boolean pproc_port_source_check_caps_reserve(mct_port_t *port,
  void *caps, void *info)
{
  mct_module_t         *pproc;
  mct_list_t           *p_list;
  mct_port_t           *sink_port;
  boolean               rc = TRUE;
  int                   i, j;
  pproc_port_private_t *port_private;
  pproc_port_private_t *sink_port_private;
  mct_module_type_identity_t     mod_type;
  mct_module_t         *sink_submodule;
  mct_stream_info_t    *stream_info = (mct_stream_info_t *)info;
  mct_event_t           event;
  pproc_divert_config_t divert_config;

  CDBG("%s:%d] E\n", __func__, __LINE__);

  MCT_OBJECT_LOCK(port);

  /* sanity check */
  if (!port || !stream_info ||
      strncmp(MCT_OBJECT_NAME(port), "pproc_source", strlen("pproc_source"))) {
    CDBG_ERROR("%s:%d] error port %p, stream_info: %p\n", __func__, __LINE__,
      port, stream_info);
    rc = FALSE;
    goto source_reserve_done;
  }

  if (pproc_port_check_port_state(port, stream_info) == FALSE) {
    rc = FALSE;
    goto source_reserve_done;
  }

  port_private = MCT_OBJECT_PRIVATE(port);
  /* reserve the port for this stream */
  for(i=0; i < PPROC_MAX_STREAM_PER_PORT; i++) {
    if(port_private->streams[i].state == PPROC_PORT_STATE_UNRESERVED) {
      port_private->streams[i].state = PPROC_PORT_STATE_RESERVED;
      port_private->streams[i].stream_info = stream_info;
      port_private->num_streams++;

      /* Get the module's sink ports and retrieve the existing stream */
      pproc = (mct_module_t *)MCT_PORT_PARENT(port)->data;
      MCT_OBJECT_LOCK(pproc);
      p_list = mct_list_find_custom(MCT_MODULE_SINKPORTS(pproc),
        &stream_info->identity, pproc_port_check_identity_in_port);
      MCT_OBJECT_UNLOCK(pproc);

      if (!p_list) {
        CDBG_ERROR("%s:%d] error no matching sink port found\n", __func__,
          __LINE__);
        rc = FALSE;
        goto source_reserve_done;
      }

      sink_port = (mct_port_t *)p_list->data;
      /* Pick up the last module in the stream attached to sink port */
      sink_port_private = MCT_OBJECT_PRIVATE(sink_port);
      for (j = 0; j < PPROC_MAX_STREAM_PER_PORT; j++) {
        if (sink_port_private->streams[j].stream_info &&
          sink_port_private->streams[j].stream_info->identity ==
          stream_info->identity) {
          /* TODO: Handle sink modules also */
          mod_type.type = MCT_MODULE_FLAG_SINK;
          mod_type.identity = stream_info->identity;
          p_list = mct_list_find_custom(
            MCT_STREAM_CHILDREN(sink_port_private->streams[j].pproc_stream),
            &mod_type, pproc_port_match_module_type);

          if (!p_list) {
            CDBG_ERROR("%s:%d] error invalid stream\n", __func__, __LINE__);
            rc = FALSE;
            goto source_reserve_done;
          }

          sink_submodule = (mct_module_t *)p_list->data;
          port_private->streams[i].int_link =
            pproc_port_resrv_port_on_module(sink_submodule, stream_info,
            MCT_PORT_SRC, port);
          if (port_private->streams[i].int_link) {
            /* Invoke ext link for the submod port */
            port_private->streams[i].int_link->ext_link(stream_info->identity,
              port_private->streams[i].int_link, port);
            rc = mct_port_add_child(stream_info->identity,
              port_private->streams[i].int_link);
            /* Change the module type */
            /* TODO: Handle sink modules also */
            sink_submodule->set_mod(sink_submodule, MCT_MODULE_FLAG_INDEXABLE,
              stream_info->identity);
            break;
          }
        }
      }

      if(j == PPROC_MAX_STREAM_PER_PORT) {
        CDBG_ERROR("%s:%d] error, unexpected error!!!", __func__, __LINE__);
        rc = FALSE;
      }

      if (rc == TRUE) {
        port_private->sessionid = stream_info->identity & 0xFFFF0000;
        port_private->streams[i].pproc_stream =
          sink_port_private->streams[j].pproc_stream;
      }

      break;
    }
  }

  if(i == PPROC_MAX_STREAM_PER_PORT) {
    CDBG_ERROR("%s:%d] error, unexpected error!!!", __func__, __LINE__);
    rc = FALSE;
  }

source_reserve_done:
  MCT_OBJECT_UNLOCK(port);
  CDBG("%s:%d] X rc: %d\n", __func__, __LINE__, rc);
  return rc;
}

/** pproc_port_source_check_caps_unreserve
 *    @port: this port object to remove the session/stream;
 *    @identity: session+stream identity.
 *
 *    Return FALSE if the identity is not existing.
 *
 *  This function unreserves the identity on this port.
 **/
static boolean pproc_port_source_check_caps_unreserve(mct_port_t *port,
  unsigned int identity)
{
  int                         i;
  boolean                     rc = TRUE;
  pproc_port_private_t       *port_private;
  mct_module_t               *module;
  mct_list_t                 *list;
  pproc_port_reserve_result_t result;
  pproc_port_stream_info_t   *port_stream_info;

  CDBG("%s:%d] E\n", __func__, __LINE__);

  MCT_OBJECT_LOCK(port);

  if (!port ||
      strncmp(MCT_OBJECT_NAME(port), "pproc_source", strlen("pproc_source"))) {
    CDBG_ERROR("%s: error because invalid parameters!\n", __func__);
    rc = FALSE;
    goto pproc_src_unreserve_done;
  }

  module = MCT_MODULE_CAST(MCT_PORT_PARENT(port)->data);
  /* 1. Check the identity is mapped
     2. Destroy stream topology
     3. Change stream state to UNRESERVED and decrement num_streams
     4. If num_streams is zero change port_type to invalid & remove session */
  port_private = (pproc_port_private_t *)MCT_OBJECT_PRIVATE(port);
  for (i=0; i < PPROC_MAX_STREAM_PER_PORT; i++) {
    if ((port_private->streams[i].state == PPROC_PORT_STATE_RESERVED) &&
      (port_private->streams[i].stream_info->identity == identity)) {
      port_private->streams[i].state = PPROC_PORT_STATE_UNRESERVED;
      port_stream_info = &port_private->streams[i];
      port_stream_info->int_link->un_link(identity, port_stream_info->int_link,
        port);
      rc = port_stream_info->int_link->check_caps_unreserve(
        port_stream_info->int_link, identity);
      mct_port_remove_child(identity, port_stream_info->int_link);
      port_stream_info->int_link = NULL;
      port_private->num_streams--;
      port_private->streams[i].divert_featue_mask = 0;
      if (!port_private->num_streams) {
        port_private->sessionid = 0;
        port_private->port_type = PPROC_PORT_TYPE_INVALID;
      }
      break;
    }
  }

  if(i == PPROC_MAX_STREAM_PER_PORT) {
    CDBG_ERROR("%s:%d] error, unexpected error!!!", __func__, __LINE__);
    /* TODO: need to reset the port_type and streaming_mode and handle
       num_streams */
    rc = FALSE;
  }

pproc_src_unreserve_done:
  MCT_OBJECT_UNLOCK(port);
  CDBG("%s:%d] X rc: %d\n", __func__, __LINE__, rc);
  return rc;
}

/** pproc_port_set_caps
 *    @port: port object which the caps to be set;
 *    @caps: this port's capability.
 *
 *  Return TRUE if it is valid port.
 *
 *  Function overwrites a ports capability.
 **/
static boolean pproc_port_set_caps(mct_port_t *port,
  mct_port_caps_t *caps)
{
  CDBG("%s:%d] E\n", __func__, __LINE__);
  if (!port || !caps) {
    CDBG_ERROR("%s:%d] error port: %p caps: %p\n", __func__, __LINE__, port,
      caps);
    return FALSE;
  }

  if (strncmp(MCT_OBJECT_NAME(port), "pproc_sink", strlen("pproc_sink")) &&
      strncmp(MCT_OBJECT_NAME(port), "pproc_source", strlen("pproc_source"))) {
    CDBG_ERROR("%s:%d] error invalid port\n", __func__, __LINE__);
    return FALSE;
  }

  port->caps = *caps;
  CDBG("%s:%d] X\n", __func__, __LINE__);
  return TRUE;
}

/** pproc_port_check_divert_type
 *    @port:  this port from where the event should go
 *    @event: event object to send upstream or downstream
 *
 *  Check existence of downstream module. If yes, send
 *  downstream event to check for processed/unprocessed divert
 *  to be set on pproc submodules.
 *
 *  Return TRUE for sucess.
 **/
static boolean pproc_port_check_divert_type(mct_module_t *pproc,
  unsigned int identity, uint32_t *divert_mask)
{
  mct_list_t *p_list = NULL;
  mct_port_t *src_port = NULL;
  mct_event_t event;
  boolean     rc = TRUE;

  /* Get the module's sink ports and retrieve the existing stream */
  p_list = mct_list_find_custom(MCT_MODULE_SRCPORTS(pproc),
    &identity, pproc_port_check_identity_in_port);

  if (!p_list) {
    CDBG("%s:%d] No downstream module connected on this identity\n", __func__,
      __LINE__);
    return TRUE;
  }

  src_port = (mct_port_t *)p_list->data;

  /* Send event to downstream module to check for requirement of processed
     or unprocessed frame */
  event.identity  = identity;
  event.type      = MCT_EVENT_MODULE_EVENT;
  event.direction = MCT_EVENT_DOWNSTREAM;
  event.u.module_event.type = MCT_EVENT_MODULE_QUERY_DIVERT_TYPE;
  event.u.module_event.module_event_data = (void *)divert_mask;
  *divert_mask = 0;

  rc = src_port->event_func(src_port, &event);
  if (rc == FALSE) {
    CDBG_ERROR("%s:%d] error: Query divert type failed\n", __func__,
      __LINE__);
    return FALSE;
  }

  return TRUE;
}

/** pproc_port_sink_event
 *    @port:  this port from where the event should go
 *    @event: event object to send upstream or downstream
 *
 *  Because pproc module works no more than a event pass throgh module,
 *  hence its event handling should be fairely straight-forward.
 *
 *  Return TRUE for successful event processing.
 **/
static boolean pproc_port_sink_event(mct_port_t *port, mct_event_t *event)
{
  int                   i;
  boolean               rc = TRUE;
  pproc_port_private_t *port_private;
  mct_list_t           *p_list = NULL;
  uint32_t              divert_mask = 0;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  /* sanity check */
  if (!port || !event) {
    CDBG_ERROR("%s:%d] error port: %p event: %p\n", __func__, __LINE__,
      port, event);
    return FALSE;
  }

  port_private = (pproc_port_private_t *)MCT_OBJECT_PRIVATE(port);
  if (!port_private) {
    CDBG_ERROR("%s:%d] error port_private cannot be null\n", __func__,
      __LINE__);
    return FALSE;
  }

  switch (MCT_EVENT_DIRECTION(event)) {
  case MCT_EVENT_UPSTREAM: {
    if (event->type == MCT_EVENT_MODULE_EVENT) {
      if (event->u.module_event.type ==
        MCT_EVENT_MODULE_PP_SUBMOD_POST_TO_BUS) {
        rc = mct_module_post_bus_msg((mct_module_t *)MCT_PORT_PARENT(port)->data,
          event->u.module_event.module_event_data);
        return rc;
      }
    }
    if (MCT_PORT_PEER(port)) {
      /* always forward the event to upstream */
      rc = mct_port_send_event_to_peer(port, event);
    }
  } /* case MCT_EVENT_TYPE_UPSTREAM */
    break;

  case MCT_EVENT_DOWNSTREAM: {
    for (i = 0; i < PPROC_MAX_STREAM_PER_PORT; i++) {
      if (port_private->streams[i].stream_info
        && (port_private->streams[i].stream_info->identity == event->identity)
        && (port_private->streams[i].int_link)) {
        /* Because mct_stream_t does not have mct_stream_info_t as a pointer,
           We need to take a copy of the stream_info->img_buffer_list in the
           duplicate stream created in pproc. For this reason we need to peek
           into the STREAMON/STREAMOFF events to update the img_buffer_list */
        switch(event->type) {
        case MCT_EVENT_CONTROL_CMD:
          switch(event->u.ctrl_event.type) {
          case MCT_EVENT_CONTROL_STREAMON: {
            mct_stream_info_t *streaminfo =
              (mct_stream_info_t *)event->u.ctrl_event.control_event_data;
            /* Update the stream_info in duplicate stream */
            port_private->streams[i].pproc_stream->streaminfo.img_buffer_list =
              streaminfo->img_buffer_list;

            /* Check divert type needed by downstream module */
            rc = pproc_port_check_divert_type(
              (mct_module_t *)MCT_PORT_PARENT(port)->data,
              streaminfo->identity, &divert_mask);

            if (TRUE == rc) {
              port_private->streams[i].divert_featue_mask |= divert_mask;
              rc = pproc_port_set_divert_config(port,
                &port_private->streams[i], streaminfo->identity);

              if (TRUE == rc) {
                /* Now dispatch the stream to submodules */
                rc = port_private->streams[i].int_link->event_func(
                  port_private->streams[i].int_link, event);
              }
            }
          }
            break;
          case MCT_EVENT_CONTROL_STREAMOFF: {
            rc = port_private->streams[i].int_link->event_func(
              port_private->streams[i].int_link, event);
            /* Update the stream_info in duplicate stream */
            port_private->streams[i].pproc_stream->streaminfo.img_buffer_list =
              NULL;
          }
            break;
          default:
            rc = port_private->streams[i].int_link->event_func(
              port_private->streams[i].int_link, event);
            break;
          }
          break;
        default:
          rc = port_private->streams[i].int_link->event_func(
            port_private->streams[i].int_link, event);
          break;
        }
        break;
      }
    }

  } /* case MCT_EVENT_TYPE_DOWNSTREAM */
    break;

  default:
    rc = FALSE;
    break;
  }

  CDBG("%s:%d] X rc: %d\n", __func__, __LINE__, rc);
  return rc;
}

/** pproc_port_source_event
 *    @port:  this port from where the event should go
 *    @event: event object to send upstream or downstream
 *
 *  Because pproc module works no more than a event pass throgh module,
 *  hence its event handling should be fairely straight-forward.
 *
 *  Return TRUE for successful event processing.
 **/
static boolean pproc_port_source_event(mct_port_t *port, mct_event_t *event)
{
  int                   i;
  boolean               rc = TRUE;
  pproc_port_private_t *port_private;
  mct_list_t           *p_list = NULL;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  /* sanity check */
  if (!port || !event) {
    CDBG_ERROR("%s:%d] error port: %p event: %p\n", __func__, __LINE__,
      port, event);
    return FALSE;
  }

  port_private = (pproc_port_private_t *)MCT_OBJECT_PRIVATE(port);
  if (!port_private) {
    CDBG_ERROR("%s:%d] error port_private cannot be null\n", __func__,
      __LINE__);
    return FALSE;
  }

  switch (MCT_EVENT_DIRECTION(event)) {
  case MCT_EVENT_DOWNSTREAM: {

    if (MCT_PORT_PEER(port)) {
      /* always forward the event to upstream */
      rc = mct_port_send_event_to_peer(port, event);
    }

  } /* case MCT_EVENT_TYPE_UPSTREAM */
    break;

  case MCT_EVENT_UPSTREAM: {
    /* In case of sink port, no need to peek into the event,
     * instead just simply forward the event to internal link */

    for (i = 0; i < PPROC_MAX_STREAM_PER_PORT; i++) {
      if (port_private->streams[i].stream_info
        && port_private->streams[i].stream_info->identity == event->identity
        && (port_private->streams[i].int_link)) {
        rc = port_private->streams[i].int_link->event_func(
          port_private->streams[i].int_link, event);
      }
    }

  } /* case MCT_EVENT_TYPE_DOWNSTREAM */
    break;

  default:
    rc = FALSE;
    break;
  }

  CDBG("%s:%d] X\n", __func__, __LINE__);
  return rc;
}

/** pproc_port_ext_link
 *    @identity:  Identity of session/stream
 *    @port: SINK of pproc ports
 *    @peer: For pproc sink- peer is most likely isp port
 *           For src module -  peer is submodules sink.
 *
 *  Set pproc port's external peer port.
 **/
static boolean pproc_port_ext_link(unsigned int identity,
  mct_port_t *port, mct_port_t *peer)
{
  pproc_port_private_t *port_data;
  int                   i;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  if(!port || !peer) {
    CDBG_ERROR("%s:%d] error port=%p, peer=%p", __func__, __LINE__, port, peer);
    return FALSE;
  }

  MCT_OBJECT_LOCK(port);
  if (MCT_PORT_PEER(port) && (MCT_PORT_PEER(port) != peer)) {
    CDBG_ERROR("%s:%d] error old_peer:%s, new_peer:%s\n", __func__, __LINE__,
      MCT_OBJECT_NAME(MCT_PORT_PEER(port)), MCT_OBJECT_NAME(peer));
    MCT_OBJECT_UNLOCK(port);
    return FALSE;
  }

  /* TODO: Need to ref count for multiple connections */
  port_data = (pproc_port_private_t *)MCT_OBJECT_PRIVATE(port);
  for(i=0; i < PPROC_MAX_STREAM_PER_PORT; i++) {
    if((port_data->streams[i].state == PPROC_PORT_STATE_RESERVED) &&
      (port_data->streams[i].stream_info) &&
      (port_data->streams[i].stream_info->identity == identity)) {
      port_data->streams[i].state = PPROC_PORT_STATE_LINKED;
      if (MCT_OBJECT_REFCOUNT(port) == 0) {
        MCT_PORT_PEER(port) = peer;
      }
      MCT_OBJECT_REFCOUNT(port) += 1;
      MCT_OBJECT_UNLOCK(port);
      CDBG("%s:%d] X\n", __func__, __LINE__);
      return TRUE;
    }
  }

  MCT_OBJECT_UNLOCK(port);
  CDBG("%s:%d] X\n", __func__, __LINE__);
  return FALSE;
}

/** pproc_port_unlink
 *    @identity: identity to be unlinked
 *    @port:     this port to unlink
 *    @peer:     peer to be unlinked
 *
 * This funtion unlink the peer ports of pproc sink, src ports
 * and its peer submodule's port
 *
 *  Return TRUE for successful unlink.
 **/
static void pproc_port_unlink(unsigned int identity,
  mct_port_t *port, mct_port_t *peer)
{
  pproc_port_private_t *port_private;
  int                   i;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  if (!port || !peer) {
    CDBG_ERROR("%s:%d] error port: %p, peer: %p\n", __func__, __LINE__, port,
      peer);
    return;
  }

  MCT_OBJECT_LOCK(port);
  if (MCT_PORT_PEER(port) != peer) {
    CDBG_ERROR("%s:%d] error peer:%p, unlink_peer:%p\n", __func__, __LINE__,
      MCT_PORT_PEER(port), peer);
    MCT_OBJECT_UNLOCK(port);
    return;
  }

  if (MCT_OBJECT_REFCOUNT(port) == 0) {
    CDBG_ERROR("%s:%d] error zero refcount on port\n", __func__, __LINE__);
    MCT_OBJECT_UNLOCK(port);
    return;
  }

  /* TODO: Need to ref count for multiple connections */
  port_private = (pproc_port_private_t *)MCT_OBJECT_PRIVATE(port);
  for(i = 0; i < PPROC_MAX_STREAM_PER_PORT; i++) {
    if(port_private->streams[i].state == PPROC_PORT_STATE_LINKED &&
      port_private->streams[i].stream_info &&
      port_private->streams[i].stream_info->identity == identity) {
      port_private->streams[i].state = PPROC_PORT_STATE_RESERVED;
      MCT_OBJECT_REFCOUNT(port) -= 1;
      if (MCT_OBJECT_REFCOUNT(port) == 0) {
        MCT_PORT_PEER(port) = NULL;
      }
      MCT_OBJECT_UNLOCK(port);
      CDBG("%s:%d] X\n", __func__, __LINE__);
      return;
    }
  }

  MCT_OBJECT_UNLOCK(port);
  CDBG("%s:%d] X\n", __func__, __LINE__);
  return;
}

/** pproc_port_sink_int_link
 *    @identity:
 *    @port:
 **/
static mct_list_t *pproc_port_int_link(unsigned int identity, mct_port_t *port)
{
  /* TODO */
  CDBG("%s:%d] E\n", __func__, __LINE__);
  CDBG("%s:%d] X\n", __func__, __LINE__);
  return NULL;
}

/** pproc_port_deinit
 *    @port: port object to be deinit
 **/
void pproc_port_deinit(mct_port_t *port)
{
  pproc_port_private_t *port_private;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  if (!port) {
    CDBG_ERROR("%s:%d] error port: %p\n", __func__, __LINE__, port);
    return;
  }

  port_private = (pproc_port_private_t *)MCT_OBJECT_PRIVATE(port);
  if (!port_private) {
    CDBG_ERROR("%s:%d] error because missing port_private data\n",
      __func__, __LINE__);
    return;
  }

  if (strncmp(MCT_OBJECT_NAME(port), "pproc_sink", strlen("pproc_sink")) &&
      strncmp(MCT_OBJECT_NAME(port), "pproc_source", strlen("pproc_source"))) {
    CDBG_ERROR("%s:%d] error because port is invalid\n", __func__, __LINE__);
    return;
  }

  /* TODO: need to ensure all attached streams on this port is removed */

  free(port_private);

  mct_port_destroy(port);
  CDBG("%s:%d] X\n", __func__, __LINE__);
  return;
}

/** pproc_port_init
 *    @port: port object to be initialized
 *
 *  Port initialization, use this function to create pproc module's
 *  sink/source ports and overwrite default port methods and install
 *  capabilities.
 *
 *  Return mct_port_t object on success, otherwise return NULL.
 **/
mct_port_t *pproc_port_init(const char *name)
{
  int                   i;
  mct_port_t           *port;
  pproc_port_private_t *port_private;
  mct_port_caps_t       caps;

  CDBG("%s:%d] E\n", __func__, __LINE__);
  port = mct_port_create(name);
  if (port == NULL) {
    CDBG_ERROR("%s:%d] error creating port\n", __func__, __LINE__);
    return NULL;
  }

  port_private = malloc(sizeof(pproc_port_private_t));
  if (port_private == NULL) {
    CDBG_ERROR("%s:%d] error allocating port private\n", __func__, __LINE__);
    goto private_error;
  }

  memset(port_private, 0, sizeof(pproc_port_private_t));

  for (i = 0; i < PPROC_MAX_STREAM_PER_PORT; i++) {
    port_private->streams[i].state = PPROC_PORT_STATE_UNRESERVED;
    port_private->port_type = PPROC_PORT_TYPE_INVALID;
  }

  MCT_OBJECT_PRIVATE(port) = port_private;
  caps.port_caps_type  = MCT_PORT_CAPS_FRAME;

  caps.u.frame.format_flag  = MCT_PORT_CAP_FORMAT_YCBCR;
  caps.u.frame.size_flag    = MCT_PORT_CAP_SIZE_20MB;

  if (!strncmp(name, "pproc_sink", strlen("pproc_sink"))) {
    port->direction = MCT_PORT_SINK;

    mct_port_set_event_func(port, pproc_port_sink_event);
    mct_port_set_check_caps_reserve_func(port,
      pproc_port_sink_check_caps_reserve);
    mct_port_set_check_caps_unreserve_func(port,
      pproc_port_sink_check_caps_unreserve);
  } else if (!strncmp(name, "pproc_source", strlen("pproc_source"))) {
    port->direction = MCT_PORT_SRC;

    mct_port_set_event_func(port, pproc_port_source_event);
    mct_port_set_check_caps_reserve_func(port,
      pproc_port_source_check_caps_reserve);
    mct_port_set_check_caps_unreserve_func(port,
      pproc_port_source_check_caps_unreserve);
  } else {
    CDBG_ERROR("%s:%d] error invalid pproc port\n", __func__, __LINE__);
    goto port_type_error;
  }

  mct_port_set_int_link_func(port, pproc_port_int_link);
  mct_port_set_set_caps_func(port, pproc_port_set_caps);
  mct_port_set_ext_link_func(port, pproc_port_ext_link);
  mct_port_set_unlink_func(port, pproc_port_unlink);

  if (port->set_caps)
    port->set_caps(port, &caps);

  CDBG("%s:%d] X\n", __func__, __LINE__);
  return port;

port_type_error:
  free(port_private);
private_error:
  mct_port_destroy(port);
  CDBG("%s:%d] X\n", __func__, __LINE__);
  return NULL;
}

/** pproc_port_send_divert_config_event
 *    @submodule: submodule that needs divert info
 *    @identity: identity to which divert info is send
 *    @update_mode: single/dual
 *    @int_link: port on which divert info is sent
 *    @divert_info: submodule's divert info
 *
 *  caps_reserve uses this function to send divert config event
 *  to submodules.
 *
 *  Return TRUE on success, otherwise return FALSE.
 **/
boolean pproc_port_send_divert_config_event(mct_module_t *submodule,
  uint32_t identity, pproc_cfg_update_t update_mode, mct_port_t *int_link,
  pproc_divert_info_t *divert_info)
{
  mct_event_t           event;
  pproc_divert_config_t divert_config;

  memset(&event, 0x00, sizeof(mct_event_t));
  event.type = MCT_EVENT_MODULE_EVENT;
  event.direction = MCT_EVENT_DOWNSTREAM;
  event.identity = identity;
  event.u.module_event.type = MCT_EVENT_MODULE_PPROC_DIVERT_INFO;
  event.u.module_event.module_event_data = &divert_config;
  memset(&divert_config, 0, sizeof(pproc_divert_config_t));
  divert_config.name = MCT_OBJECT_NAME(submodule);
  divert_config.update_mode = update_mode;
  divert_config.divert_info = *divert_info;

  return int_link->event_func(int_link, &event);
}
