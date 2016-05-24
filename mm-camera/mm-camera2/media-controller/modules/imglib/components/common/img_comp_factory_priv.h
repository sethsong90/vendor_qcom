/**********************************************************************
* Copyright (c) 2013 Qualcomm Technologies, Inc. All Rights Reserved. *
* Qualcomm Technologies Proprietary and Confidential.                 *
**********************************************************************/

#ifndef __IMG_COMP_FACTORY_PRIV_H__
#define __IMG_COMP_FACTORY_PRIV_H__

#include "img_comp_factory.h"

/** img_comp_reg_t
 *   @role: role of the component
 *   @name: name of the component
 *   @ops: function table for the operation
 *
 *   Registry for the imaging components
 *
 **/
typedef struct {
  img_comp_role_t role;
  char *name;
  img_core_ops_t ops;
} img_comp_reg_t;


/* Since all the components are compiled as part of the
 * same library, the function symbols needs to be exported
 */
/** wd_comp_create
 *   @p_ops: pointer to the image component ops
 *
 *   Create wavelet denoise component
 **/
extern int wd_comp_create(img_component_ops_t *p_ops);

/** wd_comp_load
 *
 *   Load wavelet denoise component
 **/
extern int wd_comp_load();

/** wd_comp_load
 *
 *   UnLoad wavelet denoise component
 **/
extern int wd_comp_unload();

/** hdr_comp_create
 *   @p_ops: pointer to the image component ops
 *
 *   Create HDR component
 **/
extern int hdr_comp_create(img_component_ops_t *p_ops);

/** hdr_comp_load
 *
 *   Load HDR component
 **/
extern int hdr_comp_load();

/** hdr_comp_unload
 *
 *   UnLoad HDR component
 **/
extern int hdr_comp_unload();

/** faceproc_comp_create
 *   @p_ops: pointer to the image component ops
 *
 *   Create faceproc component
 **/
extern int faceproc_comp_create(img_component_ops_t *p_ops);

/** faceproc_comp_load
 *
 *   Load faceproc component
 **/
extern int faceproc_comp_load();

/** faceproc_comp_unload
 *
 *   UnLoad faceproc component
 **/
extern int faceproc_comp_unload();

/** cac_comp_create
 *   @p_ops: pointer to the image component ops
 *
 *   Create cac component
 **/
extern int cac_comp_create(img_component_ops_t *p_ops);

/** cac_comp_load
 *
 *   Load CAC component
 **/
extern int cac_comp_load();

/** cac_comp_unload
 *
 *   UnLoad CAC component
 **/
extern int cac_comp_unload();


#endif //__IMG_COMP_FACTORY_PRIV_H__
