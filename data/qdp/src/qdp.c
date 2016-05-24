/*!
  @file
  qdp.c

  @brief
  provides an API for Qualcomm data profile management.

*/

/*===========================================================================

  Copyright (c) 2010-2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: //linux/pkgs/proprietary/qc-ril/main/source/qcril_data.c#17 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
07/15/10   js      created file

===========================================================================*/
/*===========================================================================
                           INCLUDE FILES
===========================================================================*/
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "qmi.h"
#include "qmi_wds_utils.h"
#include "qdp_platform.h"
#include "qmi_platform_config.h"
#include "qdp.h"

/* apn */
#define QMI_WDS_UMTS_PROFILE_APN_NAME_TLV_ID 0x14
#define QMI_WDS_CDMA_PROFILE_APN_STRING_TLV_ID 0x21
/* ip family */
#define QMI_WDS_UMTS_PROFILE_PDP_TYPE_TLV_ID 0x11
#define QMI_WDS_CDMA_PROFILE_PDN_TYPE_TLV_ID 0x22
/* profile name */
#define QMI_WDS_UMTS_PROFILE_NAME_TLV_ID 0x10
/* NAI */
#define QMI_WDS_UMTS_PROFILE_USERNAME_TLV_ID 0x1B
#define QMI_WDS_CDMA_PROFILE_USERNAME_TLV_ID 0x1B

typedef struct qdp_param_tech_map_s
{
  qdp_ril_param_idx_t ril_param_idx;
  qdp_tech_t tech_mask; /* 3gpp/3gpp2/both */
  unsigned long param_mask_3gpp;
  int max_len_3gpp;
  unsigned long param_mask_3gpp2;
  int max_len_3gpp2;
} qdp_param_tech_map_t;

/*
 * when RIL does not provide a technology preference,
 * following table can be used to determine, based on the,
 * RIL parameter set, as which technologies to use for
 * subsequent profile look up
 * Currently, we only use APN,  NAI, and IP_FAMILY for
 * profile look up. I am still enumerating
 * all possible RIL parameters for completion and better maintenance.
 *
*/
qdp_param_tech_map_t param_tech_map_tbl[QDP_RIL_PARAM_MAX] =
{

  { QDP_RIL_TECH,
    QDP_NOTECH, /* Not Applicable for the purpose of this table */
    0,
    0,
    0,
    0
  },

  { QDP_RIL_PROFILE_ID,
    QDP_NOTECH, /* Not Applicable for the purpose of this table */
    0,
    0,
    0,
    0
  },

  /* APN is 3GPP concept, but also applies to 3GPP2 (eHRPD) */
  { QDP_RIL_APN,
    (QDP_NOTECH | QDP_3GPP2 | QDP_3GPP),
    QMI_WDS_UMTS_PROFILE_APN_NAME_PARAM_MASK,
    QMI_WDS_MAX_APN_STR_SIZE,
    QMI_WDS_CDMA_PROFILE_APN_STRING_PARAM_MASK,
    QMI_WDS_MAX_APN_STR_SIZE
  },

  /* NAI is 3GPP2 concept, no need to lookup 3GPP profile */
  { QDP_RIL_NAI,
    (QDP_NOTECH | QDP_3GPP2),
    QMI_WDS_UMTS_PROFILE_USERNAME_PARAM_MASK,
    QMI_WDS_MAX_USERNAME_PASS_STR_SIZE,
    QMI_WDS_CDMA_PROFILE_USERNAME_PARAM_MASK,
    QMI_WDS_MAX_USERNAME_PASS_STR_SIZE
  },

  /* password is not used for profile look up */
  { QDP_RIL_PASSWORD,
    QDP_NOTECH,
    QMI_WDS_UMTS_PROFILE_PASSWORD_PARAM_MASK,
    QMI_WDS_MAX_USERNAME_PASS_STR_SIZE,
    QMI_WDS_CDMA_PROFILE_PASSWORD_PARAM_MASK,
    QMI_WDS_MAX_USERNAME_PASS_STR_SIZE
  },

  /* auth is not used for profile look up */
  { QDP_RIL_AUTH,
    QDP_NOTECH,
    QMI_WDS_UMTS_PROFILE_AUTH_PREF_PARAM_MASK,
    sizeof(qmi_wds_auth_pref_type),
    QMI_WDS_CDMA_PROFILE_AUTH_PROTOCOL_PARAM_MASK,
    sizeof(qmi_wds_auth_pref_type)
  },

   /* ip_family is used for profile lookup */
  { QDP_RIL_IP_FAMILY,
    QDP_NOTECH | QDP_3GPP2 | QDP_3GPP,
    QMI_WDS_UMTS_PROFILE_PDP_TYPE_PARAM_MASK,
    sizeof(QDP_RIL_IP_4_6),
    QMI_WDS_CDMA_PROFILE_PDN_TYPE_PARAM_MASK,
    sizeof(QDP_RIL_IP_4_6)
  },
};

#define QDP_VALIDATE_RIL_PARAM_INDEX(i) \
  if (i<QDP_RIL_PARAM_MAX && i>0)

#define QDP_GET_RIL_PARAM_IDX(i) \
  param_tech_map_tbl[i].ril_param_idx

#define QDP_GET_3GPP_MAX_LEN(i) \
  param_tech_map_tbl[i].max_len_3gpp

#define QDP_GET_3GPP2_MAX_LEN(i) \
  param_tech_map_tbl[i].max_len_3gpp2

#define QDP_GET_TECH_MASK(i) \
  param_tech_map_tbl[i].tech_mask

#define QDP_GET_3GPP_PARAM_TYPE(i) \
  param_tech_map_tbl[i].param_mask_3gpp

#define QDP_GET_3GPP2_PARAM_TYPE(i) \
  param_tech_map_tbl[i].param_mask_3gpp2

#define QDP_GET_PARAM_TECH_TABLE_INDEX(param,i) \
  do \
  { \
    int tbl_idx; \
    i = -1; \
    for(tbl_idx=0; tbl_idx<QDP_RIL_PARAM_MAX; tbl_idx++) \
    { \
      if(param_tech_map_tbl[tbl_idx].param == param) \
      { \
        i = tbl_idx; \
        break; \
      } \
    } \
  } \
  while (0)

#define QDP_MALLOC(ptr,size) \
  do \
  { \
    ptr = malloc(size); \
    if (NULL != ptr) \
    { \
      memset(ptr, 0, size); \
      QDP_LOG_DEBUG("QDP: malloc'ed [%p]",ptr); \
    } \
  } while (0)

#define QDP_FREE(ptr) \
  do \
  { \
    if (NULL != ptr) \
    { \
      QDP_LOG_DEBUG("QDP: freeing [%p]",ptr); \
      free(ptr); \
    } \
  } while (0)

typedef struct qdp_param_s
{
  char * buf;
  int len;
} qdp_param_t;

#define QDP_NUM_UMTS_PROFILES_EXPECTED 16
#define QDP_NUM_DEFAULT_CDMA_PROFILES 1
#define QDP_NUM_KDDI_CDMA_PROFILES 22
#define QDP_NUM_EHRPD_CDMA_PROFILES 7
#define QDP_NUM_OMH_CDMA_PROFILES 16
#define QDP_NUM_CDMA_PROFILES_EXPECTED \
  ( \
    QDP_NUM_DEFAULT_CDMA_PROFILES + \
    QDP_NUM_KDDI_CDMA_PROFILES +      \
    QDP_NUM_EHRPD_CDMA_PROFILES + \
    QDP_NUM_OMH_CDMA_PROFILES \
  )
#define QDP_NUM_PROFILES_EXPECTED_MAX \
    QDP_NUM_UMTS_PROFILES_EXPECTED + \
    QDP_NUM_CDMA_PROFILES_EXPECTED

#define QDP_QMI_PORT_LEN 12 /* rmnet_sdioxx */
char global_qmi_port[QDP_QMI_PORT_LEN+1]; /* +1 for ending NULL char */
int global_qmi_wds_hndl = -1;
boolean qdp_inited = FALSE;

#define QDP_CDMA_USER_DEF_PROFILE_MIN 51
#define QDP_CDMA_USER_DEF_PROFILE_MAX 60

#define QDP_INIT_BARRIER \
  do \
  { \
    if (FALSE == qdp_inited) \
    { \
      QDP_LOG_ERROR("%s","qdp not inited"); \
      return QDP_FAILURE; \
    } \
  } while (0)

#define QDP_RIL_DATA_PROFILE_OEM_BASE 1000

typedef struct qdp_profile_meta_info_s
{
  qmi_wds_profile_id_type qmi_type;
  unsigned int ref_count;
} qdp_profile_meta_info_t;

/* this array holds on to the profiles we created */
static qdp_profile_meta_info_t profile_ids[QDP_NUM_PROFILES_EXPECTED_MAX];
#define QDP_INVALID_PROFILE_ID 0xFFFFFFFF
/* max profile name length according to qmi is 32 */
#define QDP_UNIQUE_PROFILE_NAME "qdp_profile"

#define QMI_WDS_UMTS_PROFILE_NAME_TLV_ID 0x10

/* QMI WDS version information */
static qmi_service_version_info   qmi_wds_version;
#define IS_QMI_VER_MET( info, major, minor )                             \
        ((info.major_ver*1000 + info.minor_ver) >= (major*1000 + minor))
/* Profile Persistence Flag was added in WDS version 1.13 */
#define QMI_WDS_PERFLG_VERSION_MAJ (1)
#define QMI_WDS_PERFLG_VERSION_MIN (13)

/*===========================================================================
  FUNCTION:  qdp_insert_profile_id
===========================================================================*/
/*!
    @brief
    inserts given profile id into global profile_ids array. returns
    QDP_FAILURE if no space left.

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
static int qdp_insert_profile_id
(
  int profile_index,
  int technology
)
{
  int i=0;
  for(i=0; i<QDP_NUM_PROFILES_EXPECTED_MAX; i++)
  {
    if (profile_ids[i].qmi_type.profile_index == QDP_INVALID_PROFILE)
    {
      profile_ids[i].qmi_type.profile_index = profile_index;

      switch(technology)
      {
      case QDP_3GPP:
        profile_ids[i].qmi_type.technology = QMI_WDS_PROFILE_TECH_3GPP;
        QDP_LOG_DEBUG("3GPP profile_id [%d] created by QDP", profile_index);
        break;
      case QDP_3GPP2:
        profile_ids[i].qmi_type.technology = QMI_WDS_PROFILE_TECH_3GPP2;
        QDP_LOG_DEBUG("3GPP2 profile_id [%d] created by QDP", profile_index);
        break;
      default:
        QDP_LOG_ERROR("%s","programming error");
        break;
      }

      return QDP_SUCCESS;
    }
  }

  return QDP_FAILURE;
}

/*===========================================================================
  FUNCTION:  qdp_delete_profile_id
===========================================================================*/
/*!
    @brief
    deletes the given profile

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
static int qdp_delete_profile_id
(
  unsigned int profile_id
)
{
  int i=0;
  int rc, qmi_err_code;

  for(i=0; i<QDP_NUM_PROFILES_EXPECTED_MAX; i++)
  {
    if (profile_ids[i].qmi_type.profile_index == profile_id)
    {
      if (profile_ids[i].ref_count != 0)
      {
        QDP_LOG_DEBUG("deleting profile [%d] that is referred to by "
                      "[%d] users", profile_ids[i].qmi_type.profile_index,
                      profile_ids[i].ref_count);
      }

      rc = qmi_wds_delete_profile(global_qmi_wds_hndl,
                                  &profile_ids[i].qmi_type,
                                  &qmi_err_code);
      if (QMI_NO_ERR != rc)
      {
        QDP_LOG_ERROR("could not delete profile [%d]",
                      profile_ids[i].qmi_type.profile_index);
        QDP_LOG_ERROR("qmi returned [%d] with err [%d]",
                      rc, qmi_err_code);
        return QDP_FAILURE;
      }
      else
      {
        QDP_LOG_DEBUG("profile id [%d] successfully deleted",
                      profile_id);
        profile_ids[i].qmi_type.profile_index = QDP_INVALID_PROFILE_ID;
        profile_ids[i].ref_count = 0;
        return QDP_SUCCESS;
      }
    }
  }

  QDP_LOG_ERROR("profile id [%d] not found in our global list",
                profile_id);

  return QDP_FAILURE;
}


/*===========================================================================
  FUNCTION:  qdp_find_profile_id
===========================================================================*/
/*!
    @brief
    Searches profile_ids[] for the specified ID, returns reference
    count as output pointer if not NULL.

    @return
    QDP_SUCCESS if profile ID found,
    QDP_FAILURE otherwise
*/
/*=========================================================================*/
static int qdp_find_profile_id
(
  unsigned int  profile_id,
  unsigned int *ref_count
)
{
  int i=0;

  for(i=0; i<QDP_NUM_PROFILES_EXPECTED_MAX; i++)
  {
    if (profile_ids[i].qmi_type.profile_index == profile_id)
    {
      /* Return reference count if caller interested */
      if( ref_count )
      {
        *ref_count = profile_ids[i].ref_count;
      }
      return QDP_SUCCESS;
    }
  }
  return QDP_FAILURE;
}

/*===========================================================================
  FUNCTION:  qdp_cleanup_profile_ids
===========================================================================*/
/*!
    @brief
    deletes all profiles listed in profile_ids[]

    @return
    none
*/
/*=========================================================================*/
static void qdp_cleanup_profile_ids
(
  void
)
{
  int i=0;
  int rc, qmi_err_code;

  for(i=0; i<QDP_NUM_PROFILES_EXPECTED_MAX; i++)
  {
    if (profile_ids[i].qmi_type.profile_index != QDP_INVALID_PROFILE)
    {
      rc = qmi_wds_delete_profile(global_qmi_wds_hndl,
                                  &profile_ids[i].qmi_type,
                                  &qmi_err_code);
      if (QMI_NO_ERR != rc)
      {
        QDP_LOG_ERROR("could not delete profile [%d]",
                      profile_ids[i].qmi_type.profile_index);
        QDP_LOG_ERROR("qmi returned [%d] with err [%d]",
                      rc, qmi_err_code);
      }
      else
      {
        profile_ids[i].qmi_type.profile_index = QDP_INVALID_PROFILE_ID;
        profile_ids[i].ref_count = 0;
      }
    }
  }
}

/*===========================================================================
  FUNCTION:  qdp_init_profile_ids
===========================================================================*/
/*!
    @brief
    inits profile meta info array

    @return
    none
*/
/*=========================================================================*/
static void qdp_init_profile_ids
(
  void
)
{
  int i=0;

  for(i=0; i<QDP_NUM_PROFILES_EXPECTED_MAX; i++)
  {
    profile_ids[i].qmi_type.profile_index = QDP_INVALID_PROFILE;
    profile_ids[i].ref_count = 0;
  }
}

/*===========================================================================
  FUNCTION:  qdp_free_qdp_params
===========================================================================*/
/*!
    @brief
    frees memory associated with qdp_param_t array

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
static void qdp_free_qdp_params
(
  qdp_param_t * param_arr,
  unsigned int size
)
{
  unsigned int i=0;

  if (NULL == param_arr)
  {
    QDP_LOG_ERROR("%s","NULL param recvd");
    return;
  }

  for(i=0; i<size; i++)
  {
    if (param_arr[i].len > 0)
    {
      /* yes, QDP_FREE checks for NULL */
      QDP_FREE(param_arr[i].buf);
    }
  }
}

/*===========================================================================
  FUNCTION:  qdp_prepare_params_mask
===========================================================================*/
/*!
    @brief
    given set of qdp_param_t parameters (ordered with qdp_ril_param_idx_t),
    this function prepares param_mask for profile look up

    @params
    params: input set of qdp_param_t parameters
    params_mask: output params mask
    technology: QDP_3GPP / QDP_3GPP2

    @notes

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
int qdp_prepare_params_mask
(
  const qdp_param_t * params, /* array index indicates which parameter
                                * it is based on qdp_ril_param_idx_t */
  unsigned long *params_mask,
  int technology
)
{
  int i=0;
  int ret = QDP_FAILURE;

  QDP_LOG_DEBUG("%s","qdp_prepare_params_mask ENTRY");

  do
  {

    if (NULL == params ||
        NULL == params_mask)
    {
      QDP_LOG_ERROR("%s","invalid params");
      break;
    }

    if (QDP_3GPP != technology &&
        QDP_3GPP2 != technology)
    {
      QDP_LOG_ERROR("invalid tech [%d] rcvd", technology);
      break;
    }

    *params_mask = 0;
    for(i=0; i<QDP_RIL_PARAM_MAX; i++)
    {
      if (params[i].len > 0)
      {
        /* use tech_mask to determine if a parameter
         * needs to be matched for the given technology  */
        if (QDP_GET_TECH_MASK(i) & technology)
        {
          if (technology & QDP_3GPP)
          {
            QDP_LOG_DEBUG("setting mask [%p] for 3GPP [%d]",
                          QDP_GET_3GPP_PARAM_TYPE(i), i);
            *params_mask |= QDP_GET_3GPP_PARAM_TYPE(i);
          }
          else
          {
            QDP_LOG_DEBUG("setting mask [%p] for 3GPP2 [%d]",
                          QDP_GET_3GPP_PARAM_TYPE(i), i);
            *params_mask |= QDP_GET_3GPP2_PARAM_TYPE(i);
          }
        }
      }
    } /* for */

    ret = QDP_SUCCESS;
  } while (0);

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_prepare_params_mask EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_prepare_params_mask EXIT success");
  }

  return ret;
}

/*===========================================================================
  FUNCTION:  qdp_get_3gpp_profile_pdn_type
===========================================================================*/
/*!
    @brief
    This function returns the PDN type of the given 3gpp profile

    @param
    prof_params - 3gpp2 profile parameters

    @return
    PDN type of the given profile
*/
/*=========================================================================*/
static qdp_profile_pdn_type
qdp_get_3gpp_profile_pdn_type
(
  qmi_wds_profile_params_type *prof_params
)
{
  qdp_profile_pdn_type  pdn_type = QDP_PROFILE_PDN_TYPE_INVALID;
  int ret = QDP_FAILURE;


  QDP_LOG_DEBUG("%s","qdp_get_3gpp_profile_pdn_type ENTRY");

  do
  {
    if (!prof_params)
    {
      QDP_LOG_ERROR("%s","invalid params");
      break;
    }

    if (prof_params->umts_profile_params.param_mask & QMI_WDS_UMTS_PROFILE_PDP_TYPE_PARAM_MASK)
    {
      switch (prof_params->umts_profile_params.pdp_type)
      {
        case QMI_WDS_PDP_TYPE_IPV4:
          pdn_type = QDP_PROFILE_PDN_TYPE_IPV4;
          break;

        case QMI_WDS_PDP_TYPE_IPV6:
          pdn_type = QDP_PROFILE_PDN_TYPE_IPV6;
          break;

        case QMI_WDS_PDP_TYPE_IPV4V6:
          pdn_type = QDP_PROFILE_PDN_TYPE_IPV4V6;
          break;

        default:
          QDP_LOG_ERROR("unknown umts pdp_type=%d\n",
                        prof_params->umts_profile_params.pdp_type);
          break;
      }
    }

    ret = QDP_SUCCESS;
  }
  while (0);

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_get_3gpp_profile_pdn_type EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_get_3gpp_profile_pdn_type EXIT success");
  }

  return pdn_type;
}


/*===========================================================================
  FUNCTION:  qdp_get_3gpp2_profile_pdn_type
===========================================================================*/
/*!
    @brief
    This function returns the PDN type of the given 3gpp2 profile

    @param
    prof_params - 3gpp2 profile parameters

    @return
    PDN type of the given profile
*/
/*=========================================================================*/
static qdp_profile_pdn_type
qdp_get_3gpp2_profile_pdn_type
(
  qmi_wds_profile_params_type *prof_params
)
{
  qdp_profile_pdn_type  pdn_type = QDP_PROFILE_PDN_TYPE_INVALID;
  int ret = QDP_FAILURE;


  QDP_LOG_DEBUG("%s","qdp_get_3gpp2_profile_pdn_type ENTRY");

  do
  {
    if (!prof_params)
    {
      QDP_LOG_ERROR("%s","invalid params");
      break;
    }

    if (prof_params->cdma_profile_params.param_mask & QMI_WDS_CDMA_PROFILE_PDN_TYPE_PARAM_MASK)
    {
      switch (prof_params->cdma_profile_params.pdn_type)
      {
      case QMI_WDS_IPV4_PDN_TYPE:
        pdn_type = QDP_PROFILE_PDN_TYPE_IPV4;
        break;

      case QMI_WDS_IPV6_PDN_TYPE:
        pdn_type = QDP_PROFILE_PDN_TYPE_IPV6;
        break;

      case QMI_WDS_IPV4_OR_IPV6_PDN_TYPE:
        pdn_type = QDP_PROFILE_PDN_TYPE_IPV4V6;
        break;

      default:
        QDP_LOG_ERROR("unknown cdma pdp_type=%d\n",
                      prof_params->cdma_profile_params.pdn_type);
        break;
      }
    }

    ret = QDP_SUCCESS;
  }
  while (0);

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_get_3gpp2_profile_pdn_type EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_get_3gpp2_profile_pdn_type EXIT success");
  }

  return pdn_type;
}

/*===========================================================================
  FUNCTION:  qdp_match_3gpp_profile_params
===========================================================================*/
/*!
    @brief
    this function matches a set of profile parameters (typically received
    as a user input in the form of qdp_param_t array) with the parameters
    of a given profile and returns the result.

    @params
    params: input set of qdp_param_t parameters
    prof_params: parameters of a profile
    prof_id: Modem profile ID
    match_found: place holder for return boolean (TRUE/FALSE)
    lookup_error: specific lookup error code

    @notes

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
static int qdp_match_3gpp_profile_params
(
  const qdp_param_t * params, /* array index indicates which parameter
                                * it is based on qdp_ril_param_idx_t */
  qmi_wds_profile_params_type * prof_params,
  qmi_wds_profile_id_type * prof_id,
  boolean * match_found,
  qdp_error_t * lookup_error
)
{
  int ret = QDP_FAILURE;
  int reti = QDP_SUCCESS;
  int i=0;

  QDP_LOG_DEBUG("%s","qdp_match_3gpp_profile_params ENTRY");

  do
  {
    if (NULL == params ||
        NULL == prof_params ||
        NULL == prof_id ||
        NULL == match_found ||
        NULL == lookup_error )
    {
      QDP_LOG_ERROR("%s","invalid params");
      break;
    }

    /* Initialize output parameters */
    *match_found = TRUE;

    /* this for loop looks for a mismatch */
    for(i=0; i<QDP_RIL_PARAM_MAX; i++)
    {
      /* try to match non-zero parameter */
      if (params[i].len > 0)
      {
        if (NULL == params[i].buf)
        {
          QDP_LOG_ERROR("%s","programming error: NULL buffer found"
                        "where length is not zero");
          reti = QDP_FAILURE;
          break;
        }
        switch(i)
        {
        case QDP_RIL_APN:
          if ((!(prof_params->umts_profile_params.param_mask &
                 QMI_WDS_UMTS_PROFILE_APN_NAME_PARAM_MASK)) ||
              NULL == prof_params->umts_profile_params.apn_name)
          {
            QDP_LOG_DEBUG("%s","modem profile parameter APN not available");
            *match_found = FALSE;
            break;
          }

          if (std_strlen(prof_params->umts_profile_params.apn_name) ==
                 (unsigned int)params[i].len)
          {
            if (strncasecmp(
                  params[i].buf,
                  prof_params->umts_profile_params.apn_name,
                  params[i].len) == 0)
            {
              QDP_LOG_DEBUG("APN [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("APN [%s] did not match", params[i].buf);
              QDP_LOG_DEBUG("Modem contains [%s]", prof_params->cdma_profile_params.apn_name);
              *match_found = FALSE;
            }
          }
          else
          {
            QDP_LOG_DEBUG("%s", "APN: Lengths did not match");
            QDP_LOG_DEBUG("Modem APN len [%d], Input APN len [%d]",
                          std_strlen(prof_params->umts_profile_params.apn_name),
                          params[i].len);

            *match_found = FALSE;
          }
          break;
        case QDP_RIL_NAI:
          QDP_LOG_DEBUG("%s","QDP_RIL_NAI is not used "
                        "for matching 3GPP profile");
          break;
        case QDP_RIL_PASSWORD:
          QDP_LOG_DEBUG("%s","QDP_RIL_PASSWORD is not used "
                        "for matching 3GPP profile");
          break;
        case QDP_RIL_AUTH:
          QDP_LOG_DEBUG("%s","QDP_RIL_AUTH is not used "
                        "for matching 3GPP profile");
#if 0
          /* This code is kept for later enablement (if needed) */
          if(!(prof_params->umts_profile_params.param_mask &
               QMI_WDS_UMTS_PROFILE_AUTH_PREF_PARAM_MASK))
          {
            QDP_LOG_DEBUG("%s","modem profile parameter AUTH_PREF not available");
            *match_found = FALSE;
            break;
          }
          switch(prof_params->umts_profile_params.auth_pref)
          {
          case QMI_WDS_AUTH_PREF_PAP_CHAP_NOT_ALLOWED:
            if (strncasecmp(
                  params[i].buf,
                  QDP_RIL_PAP_CHAP_NOT_ALLOWED,
                  QDP_RIL_AUTH_LEN) == 0)
            {
              QDP_LOG_DEBUG("auth_pref [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("auth_pref [%s] did not match", params[i].buf);
              *match_found = FALSE;
            }
            break;
          case QMI_WDS_AUTH_PREF_PAP_ONLY_ALLOWED:
            if (strncasecmp(
                  params[i].buf,
                  QDP_RIL_PAP_ONLY_ALLOWED,
                  QDP_RIL_AUTH_LEN) == 0)
            {
              QDP_LOG_DEBUG("auth_pref [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("auth_pref [%s] did not match", params[i].buf);
              *match_found = FALSE;
            }
            break;
          case QMI_WDS_AUTH_PREF_CHAP_ONLY_ALLOWED:
            if (strncasecmp(
                  params[i].buf,
                  QDP_RIL_CHAP_ONLY_ALLOWED,
                  QDP_RIL_AUTH_LEN) == 0)
            {
              QDP_LOG_DEBUG("auth_pref [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("auth_pref [%s] did not match", params[i].buf);
              *match_found = FALSE;
            }
            break;
          case QMI_WDS_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED:
            if (strncasecmp(
                  params[i].buf,
                  QDP_RIL_PAP_CHAP_BOTH_ALLOWED,
                  QDP_RIL_AUTH_LEN) == 0)
            {
              QDP_LOG_DEBUG("auth_pref [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("auth_pref [%s] did not match", params[i].buf);
              *match_found = FALSE;
            }
            break;
          default:
            QDP_LOG_DEBUG("profile contains invalid value [%d]",
                          prof_params->umts_profile_params.auth_pref);
            *match_found = FALSE;
            break;
          }
#endif
          break;
        case QDP_RIL_IP_FAMILY:
          if(!(prof_params->umts_profile_params.param_mask &
               QMI_WDS_UMTS_PROFILE_PDP_TYPE_PARAM_MASK))
          {
            QDP_LOG_DEBUG("%s","modem profile parameter PDP_TYPE not available");
            *match_found = FALSE;
            break;
          }
          switch(prof_params->umts_profile_params.pdp_type)
          {
          case QMI_WDS_PDP_TYPE_IP:
            if ((strncasecmp(
                   params[i].buf,
                   QDP_RIL_IP_4,
                   std_strlen(QDP_RIL_IP_4)+1) == 0) ||
                (strncasecmp(
                   params[i].buf,
                   QDP_RIL_IP_4_6,
                   std_strlen(QDP_RIL_IP_4_6)+1) == 0))
            {
              QDP_LOG_DEBUG("ip_family [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("ip_family [%s] did not match", params[i].buf);
              *match_found = FALSE;
              /* Restrict lookup_error to OEM pre-provisioned
                 persistent profiles. This is determined by checking
                 the profile ID against list of those which were
                 created by QDP itself.  This assumes only one
                 instance of QDP active in system.
                 Note Modem QMI currently does not store persistent
                 flag in profile data so this cannot be used. */
              if( QDP_FAILURE == qdp_find_profile_id( prof_id->profile_index, NULL) )
              {
                *lookup_error = QDP_ERROR_ONLY_IPV4_ALLOWED;
                QDP_LOG_DEBUG("lookup_error updated to %d", *lookup_error);
              }
            }
            break;
          case QMI_WDS_PDP_TYPE_IPV6:
            if ((strncasecmp(
                   params[i].buf,
                   QDP_RIL_IP_6,
                   std_strlen(QDP_RIL_IP_6)+1) == 0) ||
                (strncasecmp(
                   params[i].buf,
                   QDP_RIL_IP_4_6,
                   std_strlen(QDP_RIL_IP_4_6)+1) == 0))

            {
              QDP_LOG_DEBUG("ip_family [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("ip_family [%s] did not match", params[i].buf);
              *match_found = FALSE;
              /* Restrict lookup_error to OEM pre-provisioned
                 persistent profiles. This is determined by checking
                 the profile ID against list of those which were
                 created by QDP itself.  This assumes only one
                 instance of QDP active in system.
                 Note Modem QMI currently does not store persistent
                 flag in profile data so this cannot be used. */
              if( QDP_FAILURE == qdp_find_profile_id( prof_id->profile_index, NULL) )
              {
                *lookup_error = QDP_ERROR_ONLY_IPV6_ALLOWED;
                QDP_LOG_DEBUG("lookup_error updated to %d", *lookup_error);
              }
            }
            break;
          case QMI_WDS_PDP_TYPE_IPV4V6:
            QDP_LOG_DEBUG("%s","any ip_family supported");
            break;
          default:
            QDP_LOG_DEBUG("invalid ip family [%d] found in profile",
                          prof_params->umts_profile_params.pdp_type);
            *match_found = FALSE;
            break;
          }
          break;
        default:
          /* not an error case. we ignore the parameters we don't care for */
          QDP_LOG_DEBUG("matching [%d] (qdp_ril_param_idx_t) not supported", i);
          break;
        } /* switch */
        if (*match_found == FALSE)
        {
          break;
        }
      } /* if */

    } /* for */
    if (QDP_SUCCESS != reti)
    {
      break;
    }

    ret = QDP_SUCCESS;
  } while (0);


  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_match_3gpp_profile_params EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_match_3gpp_profile_params EXIT success");
  }

  return ret;

}

/*===========================================================================
  FUNCTION:  qdp_match_3gpp2_profile_params
===========================================================================*/
/*!
    @brief
    this function matches a set of profile parameters (typically received
    as a user input in the form of qdp_param_t array) with the parameters
    of a given profile and returns the result.

    @params
    params: input set of qdp_param_t parameters
    prof_params: parameters of a profile
    prof_id: Modem profile ID
    match_found: place holder for return boolean (TRUE/FALSE)
    lookup_error: specific lookup error code

    @notes

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
static int qdp_match_3gpp2_profile_params
(
  const qdp_param_t * params, /* array index indicates which parameter
                                * it is based on qdp_ril_param_idx_t */
  qmi_wds_profile_params_type * prof_params,
  qmi_wds_profile_id_type * prof_id,
  boolean * match_found,
  qdp_error_t * lookup_error
)
{
  int ret = QDP_FAILURE;
  int reti = QDP_SUCCESS;
  int i=0;

  QDP_LOG_DEBUG("%s","qdp_match_3gpp2_profile_params ENTRY");

  do
  {
    if (NULL == params ||
        NULL == prof_params ||
        NULL == prof_id ||
        NULL == match_found ||
        NULL == lookup_error )
    {
      QDP_LOG_ERROR("%s","invalid params");
      break;
    }

    /* Initialize output parameters */
    *match_found = TRUE;

    /* this for loop looks for a mismatch */
    for(i=0; i<QDP_RIL_PARAM_MAX; i++)
    {
      /* try to match non-zero parameter */
      if (params[i].len > 0)
      {
        if (NULL == params[i].buf)
        {
          QDP_LOG_ERROR("%s","programming error: NULL buffer found"
                        "where length is non zero");
          reti = QDP_FAILURE;
          break;
        }
        switch(i)
        {
        case QDP_RIL_APN:
          if ((!(prof_params->cdma_profile_params.param_mask &
                 QMI_WDS_CDMA_PROFILE_APN_STRING_PARAM_MASK)) ||
              NULL == prof_params->cdma_profile_params.apn_name)
          {
            QDP_LOG_DEBUG("modem profile parameter APN is NULL. param_mask = [%p]",
                          prof_params->cdma_profile_params.param_mask);
            *match_found = FALSE;
            break;
          }
          if (std_strlen(prof_params->cdma_profile_params.apn_name) ==
                 (unsigned int)params[i].len)
          {
            if (strncasecmp(
                params[i].buf,
                prof_params->cdma_profile_params.apn_name,
                params[i].len) == 0)
            {
              QDP_LOG_DEBUG("APN [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("APN [%s] did not match", params[i].buf);
              QDP_LOG_DEBUG("Modem contains [%s]", prof_params->cdma_profile_params.apn_name);
              *match_found = FALSE;
           }
          }
          else
          {
            QDP_LOG_DEBUG("%s", "APN: Lengths did not match");
            QDP_LOG_DEBUG("Modem APN len [%d], Input APN len [%d]",
                          std_strlen(prof_params->cdma_profile_params.apn_name),
                          params[i].len);

            *match_found = FALSE;
          }
          break;
        case QDP_RIL_NAI:
          if ((!(prof_params->cdma_profile_params.param_mask &
                 QMI_WDS_CDMA_PROFILE_USERNAME_PARAM_MASK)) ||
              NULL == prof_params->cdma_profile_params.username)
          {
            QDP_LOG_DEBUG("%s","modem profile parameter NAI is NULL");
            *match_found = FALSE;
            break;
          }
          if (strncasecmp(
                params[i].buf,
                prof_params->cdma_profile_params.username,
                params[i].len) == 0)
          {
            QDP_LOG_DEBUG("NAI [%s] matched", params[i].buf);
          }
          else
          {
            QDP_LOG_DEBUG("NAI [%s] did not match", params[i].buf);
            QDP_LOG_DEBUG("Modem contains [%s]", prof_params->cdma_profile_params.username);
            *match_found = FALSE;
          }
          break;
        case QDP_RIL_PASSWORD:
          QDP_LOG_DEBUG("%s","QDP_RIL_PASSWORD is not used "
                        "for matching 3GPP2 profile");
          break;
        case QDP_RIL_AUTH:
          QDP_LOG_DEBUG("%s","QDP_RIL_AUTH is not used "
                        "for matching 3GPP2 profile");
#if 0
          /* This code is kept for later enablement (if needed) */
          if(!(prof_params->cdma_profile_params.param_mask &
               QMI_WDS_CDMA_PROFILE_AUTH_PROTOCOL_PARAM_MASK))
          {
            QDP_LOG_DEBUG("%s","modem profile parameter AUTH_PROTOCOL not available");
            *match_found = FALSE;
            break;
          }
          switch(prof_params->cdma_profile_params.auth_protocol)
          {
          case QMI_WDS_AUTH_PREF_PAP_CHAP_NOT_ALLOWED:
            if (strncasecmp(
                  params[i].buf,
                  QDP_RIL_PAP_CHAP_NOT_ALLOWED,
                  QDP_RIL_AUTH_LEN) == 0)
            {
              QDP_LOG_DEBUG("auth_pref [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("auth_pref [%s] did not match", params[i].buf);
              *match_found = FALSE;
            }
            break;
          case QMI_WDS_AUTH_PREF_PAP_ONLY_ALLOWED:
            if (strncasecmp(
                  params[i].buf,
                  QDP_RIL_PAP_ONLY_ALLOWED,
                  QDP_RIL_AUTH_LEN) == 0)
            {
              QDP_LOG_DEBUG("auth_pref [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("auth_pref [%s] did not match", params[i].buf);
              *match_found = FALSE;
            }
            break;
          case QMI_WDS_AUTH_PREF_CHAP_ONLY_ALLOWED:
            if (strncasecmp(
                  params[i].buf,
                  QDP_RIL_CHAP_ONLY_ALLOWED,
                  QDP_RIL_AUTH_LEN) == 0)
            {
              QDP_LOG_DEBUG("auth_pref [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("auth_pref [%s] did not match", params[i].buf);
              *match_found = FALSE;
            }
            break;
          case QMI_WDS_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED:
            if (strncasecmp(
                  params[i].buf,
                  QDP_RIL_PAP_CHAP_BOTH_ALLOWED,
                  QDP_RIL_AUTH_LEN) == 0)
            {
              QDP_LOG_DEBUG("auth_pref [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("auth_pref [%s] did not match", params[i].buf);
              *match_found = FALSE;
            }
            break;
          default:
            QDP_LOG_DEBUG("profile contains invalid value [%d]",
                          prof_params->cdma_profile_params.auth_protocol);
            *match_found = FALSE;
            break;
          }
#endif
          break;
        case QDP_RIL_IP_FAMILY:
          if(!(prof_params->cdma_profile_params.param_mask &
               QMI_WDS_CDMA_PROFILE_PDN_TYPE_PARAM_MASK))
          {
            QDP_LOG_DEBUG("%s","modem profile parameter PDN_TYPE not available");
            *match_found = FALSE;
            break;
          }
          switch(prof_params->cdma_profile_params.pdn_type)
          {
          case QMI_WDS_IPV4_PDN_TYPE:
            if ((strncasecmp(
                   params[i].buf,
                   QDP_RIL_IP_4,
                   std_strlen(QDP_RIL_IP_4)+1) == 0) ||
                (strncasecmp(
                   params[i].buf,
                   QDP_RIL_IP_4_6,
                   std_strlen(QDP_RIL_IP_4_6)+1) == 0))
            {
              QDP_LOG_DEBUG("ip_family [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("ip_family [%s] did not match", params[i].buf);
              *match_found = FALSE;
              /* Restrict lookup_error to OEM pre-provisioned
                 persistent profiles. This is determined by checking
                 the profile ID against list of those which were
                 created by QDP itself.  This assumes only one
                 instance of QDP active in system.
                 Note Modem QMI currently does not store persistent
                 flag in profile data so this cannot be used. */
              if( (QDP_FAILURE == qdp_find_profile_id( prof_id->profile_index, NULL)) &&
                  ((prof_id->profile_index < QDP_CDMA_USER_DEF_PROFILE_MIN)||
                   (prof_id->profile_index > QDP_CDMA_USER_DEF_PROFILE_MAX)))
              {
                *lookup_error = QDP_ERROR_ONLY_IPV4_ALLOWED;
                QDP_LOG_DEBUG("lookup_error updated to %d", *lookup_error);
              }
            }
            break;
          case QMI_WDS_IPV6_PDN_TYPE:
            if ((strncasecmp(
                   params[i].buf,
                   QDP_RIL_IP_6,
                   std_strlen(QDP_RIL_IP_6)+1) == 0) ||
                (strncasecmp(
                   params[i].buf,
                   QDP_RIL_IP_4_6,
                   std_strlen(QDP_RIL_IP_4_6)+1) == 0))
            {
              QDP_LOG_DEBUG("ip_family [%s] matched", params[i].buf);
            }
            else
            {
              QDP_LOG_DEBUG("ip_family [%s] did not match", params[i].buf);
              *match_found = FALSE;
              /* Restrict lookup_error to OEM pre-provisioned
                 persistent profiles. This is determined by checking
                 the profile ID against list of those which were
                 created by QDP itself.  This assumes only one
                 instance of QDP active in system.
                 Note Modem QMI currently does not store persistent
                 flag in profile data so this cannot be used. */
              if( (QDP_FAILURE == qdp_find_profile_id( prof_id->profile_index, NULL)) &&
                  ((prof_id->profile_index < QDP_CDMA_USER_DEF_PROFILE_MIN)||
                   (prof_id->profile_index > QDP_CDMA_USER_DEF_PROFILE_MAX)))
              {
                *lookup_error = QDP_ERROR_ONLY_IPV6_ALLOWED;
                QDP_LOG_DEBUG("lookup_error updated to %d", *lookup_error);
              }
            }
            break;
          case QMI_WDS_IPV4_OR_IPV6_PDN_TYPE:
            QDP_LOG_DEBUG("%s","any ip_family supported");
            break;
          case QMI_WDS_UNSPECIFIED_PDN_TYPE:
            QDP_LOG_DEBUG("%s","unspecified ip_family found in profile");
            *match_found = FALSE;
            break;
          default:
            QDP_LOG_DEBUG("invalid ip family [%d] found in profile",
                          prof_params->cdma_profile_params.pdn_type);
            *match_found = FALSE;
            break;
          }
          break;
        default:
          /* not an error case. we ignore the parameters we don't care for */
          QDP_LOG_DEBUG("matching [%d] (qdp_ril_param_idx_t) not supported", i);
          break;
        } /* switch */
      } /* if */
      if (*match_found == FALSE)
      {
        break;
      }

    } /* for */
    if (reti != QDP_SUCCESS)
    {
      break;
    }

    ret = QDP_SUCCESS;
  } while (0);


  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_match_3gpp2_profile_params EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_match_3gpp2_profile_params EXIT success");
  }

  return ret;

}

/*===========================================================================
  FUNCTION:  qdp_3gpp_multi_param_search
===========================================================================*/
/*!
    @brief
    searches through list of 3GPP profiles using the combination of
    given parameters (ANDed together) as a key
    If one of more profiles are found based on this search, it
    returns the very first profile id in the place holder.

    @params
    params: pointer to the array of parameters such that *param[i] is
    the ith parameter. This array must be exactly QDP_RIL_PARAM_MAX
    long, and must contain parameters in the order specified by
    qdp_ril_param_idx_t. If a parameter is not required, it can be
    left out NULL in the array.
    profile_num: placeholder for the returned profile id (if found).
    lookup_error: specific lookup error code
    p_params: out parameter to hold the profile parameters for a given
    profile id before it is updated. Will be used to restore the profile
    in case LTE attach fails. For non-LTE attach cases this parameter will
    be NULL.

    @notes

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
int qdp_3gpp_multi_param_search
(
  const qdp_param_t * params, /* array index indicates which parameter
                               * it is based on qdp_ril_param_idx_t */
  unsigned int * profile_num, /* out placeholder, *profile_num
                               * must be set to 0 by caller */
  qdp_profile_pdn_type *profile_pdn_type,  /* Profile PDN type */
  qmi_wds_profile_params_type * p_params,  /* out placeholder */
  qdp_error_t * lookup_error  /* out placeholder */
)
{
  int i=0;
  int ret = QDP_FAILURE;
  int reti = QDP_SUCCESS;
  int rc, qmi_err_code;
  qmi_wds_profile_tech_type profile_tech;
  int num_elements_expected_3gpp = QDP_NUM_UMTS_PROFILES_EXPECTED;
  qmi_wds_profile_list_type * result_list_3gpp = NULL;
  qmi_wds_profile_id_type prof_id;
  qmi_wds_profile_params_type prof_params;
  boolean match_found = FALSE;

  QDP_INIT_BARRIER;

  do
  {
    QDP_LOG_DEBUG("%s","qdp_3gpp_multi_param_search ENTRY");

    if (NULL == params           ||
        NULL == profile_num      ||
        NULL == profile_pdn_type ||
        NULL == lookup_error )
    {
      QDP_LOG_ERROR("%s","invalid params");
      break;
    }

    if (QDP_INVALID_PROFILE != *profile_num)
    {
      QDP_LOG_ERROR("output placeholder [%d] is not [%d]",
                    *profile_num, QDP_INVALID_PROFILE);
      break;
    }

    if (QDP_PROFILE_PDN_TYPE_INVALID != *profile_pdn_type)
    {
      QDP_LOG_ERROR("output placeholder [%d] is not [%d]",
                    *profile_pdn_type, QDP_PROFILE_PDN_TYPE_INVALID);
      break;
    }

    QDP_MALLOC(result_list_3gpp,
               sizeof(qmi_wds_profile_list_type)*QDP_NUM_UMTS_PROFILES_EXPECTED);
    if (NULL == result_list_3gpp)
    {
      QDP_LOG_ERROR("%s","memory error");
      break;
    }

    profile_tech = QMI_WDS_PROFILE_TECH_3GPP;

    rc = qmi_wds_utils_get_profile_list(
      global_qmi_wds_hndl,
      &profile_tech,
      NULL,
      result_list_3gpp,
      &num_elements_expected_3gpp,
      &qmi_err_code);

    if (QMI_NO_ERR != rc)
    {
      QDP_LOG_ERROR("get_profile_list failed with error [%d] " \
                    "qmi_err_code [%d]", rc, qmi_err_code);
      break;
    }

    QDP_LOG_DEBUG("get_profile_list for profile_tech [%d] "   \
                  "returned [%d] profile ids",
                  profile_tech, num_elements_expected_3gpp);

    memset(&prof_params, 0, sizeof(prof_params));
#if 0
    rc = qdp_prepare_params_mask(
      params,
      &(prof_params.umts_profile_params.param_mask),
      QDP_3GPP);
    if (QDP_SUCCESS != rc)
    {
      QDP_LOG_ERROR("%s","couldn't prepare params for lookup");
      break;
    }
#endif

    reti = QDP_SUCCESS;
    for(i=0; i<num_elements_expected_3gpp; i++)
    {
      prof_id.technology = QMI_WDS_PROFILE_TECH_3GPP;
      prof_id.profile_index = result_list_3gpp[i].profile_index;

      rc = qmi_wds_query_profile(
        global_qmi_wds_hndl,
        &prof_id,
        &prof_params,
        &qmi_err_code
        );
      if (QMI_NO_ERR != rc)
      {
        QDP_LOG_ERROR("query_profile failed with error [%d] " \
                      "qmi_err_code [%d]", rc, qmi_err_code);
        reti = QDP_FAILURE;
        break;
      }
      else
      {
        QDP_LOG_DEBUG("successfully queried 3GPP profile [%d]",
                      prof_id.profile_index);
      }

      /* every time there is specific lookup error that needs to be
         handled in QDP, the lookup_error variable will be updated.
      */
      rc = qdp_match_3gpp_profile_params( params,
                                          &prof_params,
                                          &prof_id,
                                          &match_found,
                                          lookup_error );
      if (QDP_FAILURE == rc)
      {
        QDP_LOG_ERROR("%s","programming error");
        reti = QDP_FAILURE;
        break;
      }

      if(match_found)
      {
        *profile_num = prof_id.profile_index;
        *profile_pdn_type = qdp_get_3gpp_profile_pdn_type(&prof_params);

        if (NULL != p_params)
        {
          /* Copy the profile information into the out placeholder for profile params */
          memcpy( p_params, &prof_params, sizeof(qmi_wds_profile_params_type));
        }

        /* reset lookup error if it was set while matching
           any profiles queried before this point */
        QDP_LOG_DEBUG("%s", "resetting lookup_error - if any");
        *lookup_error = QDP_ERROR_NONE;
        break;
      }

    } /* for */
    if (QDP_SUCCESS != reti)
    {
      break;
    }

    ret = QDP_SUCCESS;
  } while (0);

  if (NULL != result_list_3gpp)
  {
    QDP_FREE(result_list_3gpp);
  }

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_3gpp_multi_param_search EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_3gpp_multi_param_search EXIT success");
  }

  return ret;
}

/*===========================================================================
  FUNCTION:  qdp_3gpp2_multi_param_search
===========================================================================*/
/*!
    @brief
    searches through list of 3GPP2 profiles using the combination of
    given parameters (ANDed together) as a key
    If one of more profiles are found based on this search, it
    returns the profile ids in profile_num_list placeholder

    @params
    params: pointer to the array of parameters such that *param[i] is
    the ith parameter. This array must be exactly QDP_RIL_PARAM_MAX
    long, and must contain parameters in the order specified by
    qdp_ril_param_idx_t. If a parameter is not required, it can be
    left out NULL in the array.
    profile_num: placeholder for the returned profile id (if found).
    lookup_error: specific lookup error code

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
int qdp_3gpp2_multi_param_search
(
  const qdp_param_t* params,  /* array index indicates which parameter
                               * it is based on qdp_ril_param_idx_t */
  unsigned int * profile_num, /* out placeholder, *profile_num
                               * must be set to 0 by caller */
  qdp_profile_pdn_type *profile_pdn_type,  /* Profile PDN type */
  qdp_error_t * lookup_error  /* out placeholder */
)
{
  int i=0;
  int ret = QDP_FAILURE;
  int reti = QDP_SUCCESS;
  int rc, qmi_err_code;
  qmi_wds_profile_tech_type profile_tech;
  int num_elements_expected_3gpp2 = QDP_NUM_CDMA_PROFILES_EXPECTED;
  qmi_wds_profile_list_type * result_list_3gpp2 = NULL;
  qmi_wds_profile_id_type prof_id;
  qmi_wds_profile_params_type prof_params;
  boolean match_found = FALSE;

  QDP_INIT_BARRIER;

  do
  {
    QDP_LOG_DEBUG("%s","qdp_3gpp2_multi_param_search ENTRY");

    if (NULL == params           ||
        NULL == profile_num      ||
        NULL == profile_pdn_type ||
        NULL == lookup_error )
    {
      QDP_LOG_ERROR("%s","invalid params");
      break;
    }

    if (QDP_INVALID_PROFILE != *profile_num)
    {
      QDP_LOG_ERROR("output placeholder [%d] is not [%d]",
                    *profile_num, QDP_INVALID_PROFILE);
      break;
    }

    if (QDP_PROFILE_PDN_TYPE_INVALID != *profile_pdn_type)
    {
      QDP_LOG_ERROR("output placeholder [%d] is not [%d]",
                    *profile_pdn_type, QDP_PROFILE_PDN_TYPE_INVALID);
      break;
    }

    QDP_MALLOC(result_list_3gpp2,
               sizeof(qmi_wds_profile_list_type)*QDP_NUM_CDMA_PROFILES_EXPECTED);
    if (NULL == result_list_3gpp2)
    {
      QDP_LOG_ERROR("%s","memory error");
      break;
    }

    profile_tech = QMI_WDS_PROFILE_TECH_3GPP2;

    rc = qmi_wds_utils_get_profile_list(
      global_qmi_wds_hndl,
      &profile_tech,
      NULL,
      result_list_3gpp2,
      &num_elements_expected_3gpp2,
      &qmi_err_code);

    if (QMI_NO_ERR != rc)
    {
      QDP_LOG_ERROR("get_profile_list failed with error [%d] " \
                    "qmi_err_code [%d]", rc, qmi_err_code);
      break;
    }

    QDP_LOG_DEBUG("get_profile_list for profile_tech [%d] "   \
                  "returned [%d] profile ids",
                  profile_tech, num_elements_expected_3gpp2);

    memset(&prof_params, 0, sizeof(prof_params));
#if 0
    rc = qdp_prepare_params_mask(
      params,
      &(prof_params.cdma_profile_params.param_mask),
      QDP_3GPP2);

    if (QDP_SUCCESS != rc)
    {
      QDP_LOG_ERROR("%s","couldn't prepare params for lookup");
      break;
    }
#endif

    reti = QDP_SUCCESS;
    for(i=0; i<num_elements_expected_3gpp2; i++)
    {
      prof_id.technology = QMI_WDS_PROFILE_TECH_3GPP2;
      prof_id.profile_index = result_list_3gpp2[i].profile_index;

      rc = qmi_wds_query_profile(
        global_qmi_wds_hndl,
        &prof_id,
        &prof_params,
        &qmi_err_code
        );
      if (QMI_NO_ERR != rc)
      {
        QDP_LOG_ERROR("query_profile failed with error [%d] " \
                      "qmi_err_code [%d]", rc, qmi_err_code);
        reti = QDP_FAILURE;
        break;
      }
      else
      {
        QDP_LOG_DEBUG("successfully queried 3GPP2 profile [%d]",
                      prof_id.profile_index);
      }

      /* every time there is specific lookup error that needs to be
         handled in QDP, the lookup_error variable will be updated */
      rc = qdp_match_3gpp2_profile_params( params,
                                           &prof_params,
                                           &prof_id,
                                           &match_found,
                                           lookup_error );
      if (QDP_FAILURE == rc)
      {
        QDP_LOG_ERROR("%s","programming error");
        reti = QDP_FAILURE;
        break;
      }

      if(match_found)
      {
        *profile_num = prof_id.profile_index;
        *profile_pdn_type = qdp_get_3gpp2_profile_pdn_type(&prof_params);

        /* reset lookup error if it was set while matching
           any profiles queried before this point */
        QDP_LOG_DEBUG("%s", "resetting lookup_error - if any");
        *lookup_error = QDP_ERROR_NONE;
        break;
      }

    } /* for */
    if (QDP_SUCCESS != reti)
    {
      break;
    }

    ret = QDP_SUCCESS;
  } while (0);

  if (NULL != result_list_3gpp2)
  {
    QDP_FREE(result_list_3gpp2);
  }

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_3gpp2_multi_param_search EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_3gpp2_multi_param_search EXIT success");
  }

  return ret;
}

/*===========================================================================
  FUNCTION: qdp_3gpp_profile_update
===========================================================================*/
/*!
  @brief
  Updates profile information on the modem

*/
/*=========================================================================*/
static int qdp_3gpp_profile_update
(
  qdp_param_t                 * params,
  unsigned int                  profile_id,
  int                         * error_code
)
{
  int rc, i, ret = QDP_FAILURE;
  qmi_wds_profile_id_type     prof_id;
  qmi_wds_profile_params_type profile_params;

  QDP_LOG_DEBUG("%s","qdp_3gpp_profile_update ENTRY");

  do
  {
    if (params == NULL)
    {
      QDP_LOG_ERROR("Invalid profile params for profile_id [%d]", profile_id);
      break;
    }

    QDP_LOG_DEBUG("Trying to update profile for profile id [%d]", profile_id);

    prof_id.technology = QMI_WDS_PROFILE_TECH_3GPP;
    prof_id.profile_index = profile_id;

    memset(&profile_params, 0, sizeof(profile_params));

    for (i=0; i<QDP_RIL_PARAM_MAX; i++)
    {
      switch (i)
      {
      case QDP_RIL_APN:
        if (params[i].len > QMI_WDS_MAX_APN_STR_SIZE-1)
        {
          QDP_LOG_ERROR("Invalid length [%d] of APN [%s] rcvd",
                        params[i].len, params[i].buf);
          ret = QDP_FAILURE;
          break;
        }
        else if (params[i].len > 0)
        {
          profile_params.umts_profile_params.param_mask |=
            QMI_WDS_UMTS_PROFILE_APN_NAME_PARAM_MASK;
          strlcpy(profile_params.umts_profile_params.apn_name,
                  params[i].buf,
                  QMI_WDS_MAX_APN_STR_SIZE);
        }
        break;
      case QDP_RIL_NAI:
        if (params[i].len > QMI_WDS_MAX_USERNAME_PASS_STR_SIZE-1)
        {
          QDP_LOG_ERROR("Invalid length [%d] of USERNAME [%s] rcvd",
                        params[i].len, params[i].buf);
          ret = QDP_FAILURE;
          break;
        }
        else if (params[i].len > 0)
        {
          profile_params.umts_profile_params.param_mask |=
            QMI_WDS_UMTS_PROFILE_USERNAME_PARAM_MASK;
          strlcpy(profile_params.umts_profile_params.username,
                  params[i].buf,
                  QMI_WDS_MAX_USERNAME_PASS_STR_SIZE);
        }
        break;
      case QDP_RIL_PASSWORD:
        if (params[i].len > QMI_WDS_MAX_USERNAME_PASS_STR_SIZE-1)
        {
          QDP_LOG_ERROR("Invalid length [%d] of PASSWORD [%s] rcvd",
                        params[i].len, params[i].buf);
          ret = QDP_FAILURE;
          break;
        }
        else if (params[i].len > 0)
        {
          profile_params.umts_profile_params.param_mask |=
            QMI_WDS_UMTS_PROFILE_PASSWORD_PARAM_MASK;
          strlcpy(profile_params.umts_profile_params.password,
                  params[i].buf,
                  QMI_WDS_MAX_USERNAME_PASS_STR_SIZE);
        }
        break;
      case QDP_RIL_AUTH:
        if (params[i].len > 0)
        {
          if (strcmp(params[i].buf, QDP_RIL_PAP_CHAP_NOT_ALLOWED) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_AUTH_PREF_PARAM_MASK;
            profile_params.umts_profile_params.auth_pref =
              QMI_WDS_AUTH_PREF_PAP_CHAP_NOT_ALLOWED;
          }
          else if (strcmp(params[i].buf, QDP_RIL_PAP_ONLY_ALLOWED) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_AUTH_PREF_PARAM_MASK;
            profile_params.umts_profile_params.auth_pref =
              QMI_WDS_AUTH_PREF_PAP_ONLY_ALLOWED;
          }
          else if (strcmp(params[i].buf, QDP_RIL_CHAP_ONLY_ALLOWED) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_AUTH_PREF_PARAM_MASK;
            profile_params.umts_profile_params.auth_pref =
              QMI_WDS_AUTH_PREF_CHAP_ONLY_ALLOWED;
          }
          else if (strcmp(params[i].buf, QDP_RIL_PAP_CHAP_BOTH_ALLOWED) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_AUTH_PREF_PARAM_MASK;
            profile_params.umts_profile_params.auth_pref =
              QMI_WDS_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED;
          }
          else
          {
            QDP_LOG_ERROR("invalid auth_pref [%s]", params[i].buf);
            ret = QDP_FAILURE;
            break;
          }
        }
        break;
      case QDP_RIL_IP_FAMILY:
        if (params[i].len > 0)
        {
          if (strcmp(params[i].buf, QDP_RIL_IP_4) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_PDP_TYPE_PARAM_MASK;
            profile_params.umts_profile_params.pdp_type =
              QMI_WDS_PDP_TYPE_IP;
          }
          else if(strcmp(params[i].buf, QDP_RIL_IP_6) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_PDP_TYPE_PARAM_MASK;
            profile_params.umts_profile_params.pdp_type =
              QMI_WDS_PDP_TYPE_IPV6;
          }
          else if(strcmp(params[i].buf, QDP_RIL_IP_4_6) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_PDP_TYPE_PARAM_MASK;
            profile_params.umts_profile_params.pdp_type =
              QMI_WDS_PDP_TYPE_IPV4V6;
          }
          else
          {
            QDP_LOG_ERROR("invalid ip_family [%s]", params[i].buf);
            ret = QDP_FAILURE;
            break;
          }
        }
        break;
      default:
        QDP_LOG_DEBUG("ignoring RIL param idx [%d]", i);
        break;
      }
    }

    /* Update profile on modem */
    rc = qmi_wds_modify_profile( global_qmi_wds_hndl,
                                 &prof_id,
                                 &profile_params,
                                 error_code);

    if (QMI_NO_ERR != rc)
    {
      break;
    }

    ret = QDP_SUCCESS;
  } while (0);

  QDP_LOG_DEBUG("%s","qdp_3gpp_profile_update EXIT");

  return ret;
} /* qdp_3gpp_profile_update */

/*===========================================================================
  FUNCTION:  qdp_3gpp_profile_create
===========================================================================*/
/*!
    @brief
    creates a new 3gpp profile based on the default 3gpp profile and
    overrides any parameters that are given as input to this function.

    reference count on the newly created profile id is initialized with
    value 1

    @params
    params:
    num_params: number of parameters given as input
    params: pointer to the array of parameters such that *param[i] is
    the ith parameter
    profile_id: placeholder to return the newly created profile id
    p_params: out parameter to hold the profile parameters for a given
    profile id before it is updated. Will be used to restore the profile
    in case LTE attach fails. For non-LTE attach cases this parameter will
    be NULL.
    is_persistent : used for LTE atatch profile creation

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
int qdp_3gpp_profile_create
(
  qdp_param_t * params,
  unsigned int * profile_id_ptr,
  qdp_profile_pdn_type *profile_pdn_type,
  qmi_wds_profile_params_type * p_params, /* out placeholder */
  unsigned int is_persistent
)
{
  qmi_wds_profile_id_type profile_id;
  qmi_wds_profile_params_type profile_params;
  int ret = QDP_FAILURE, i=0, reti = QDP_SUCCESS, rc;
  int qmi_err_code;

  QDP_INIT_BARRIER;

  QDP_LOG_DEBUG("%s","qdp_3gpp_profile_create ENTRY");

  do
  {

    if (NULL == params         ||
        NULL == profile_id_ptr ||
        NULL == profile_pdn_type)
    {
      QDP_LOG_ERROR("%s","NULL start_nw_params received");
      break;
    }

    if (QDP_INVALID_PROFILE != *profile_id_ptr)
    {
      QDP_LOG_ERROR("%s","placeholder contains non-zero value");
      break;
    }

    if (QDP_PROFILE_PDN_TYPE_INVALID != *profile_pdn_type)
    {
      QDP_LOG_ERROR("%s","placeholder contains non-zero value");
      break;
    }

    /* start with clean params */
    memset(&profile_params, 0, sizeof(profile_params));
    memset(&profile_id, 0, sizeof(profile_id));

    reti = QDP_SUCCESS;
    for(i=0; i<QDP_RIL_PARAM_MAX; i++)
    {
      switch(i)
      {
      case QDP_RIL_APN:
        if (params[i].len > QMI_WDS_MAX_APN_STR_SIZE-1)
        {
          QDP_LOG_ERROR("Invalid length [%d] of APN [%s] rcvd",
                        params[i].len, params[i].buf);
          reti = QDP_FAILURE;
          break;
        }
        else if (params[i].len > 0)
        {
          profile_params.umts_profile_params.param_mask |=
            QMI_WDS_UMTS_PROFILE_APN_NAME_PARAM_MASK;
          strlcpy(profile_params.umts_profile_params.apn_name,
                  params[i].buf,
                  QMI_WDS_MAX_APN_STR_SIZE);
        }
        break;
      case QDP_RIL_NAI:
        if (params[i].len > QMI_WDS_MAX_USERNAME_PASS_STR_SIZE-1)
        {
          QDP_LOG_ERROR("Invalid length [%d] of USERNAME [%s] rcvd",
                        params[i].len, params[i].buf);
          reti = QDP_FAILURE;
          break;
        }
        else if (params[i].len > 0)
        {
          profile_params.umts_profile_params.param_mask |=
            QMI_WDS_UMTS_PROFILE_USERNAME_PARAM_MASK;
          strlcpy(profile_params.umts_profile_params.username,
                  params[i].buf,
                  QMI_WDS_MAX_USERNAME_PASS_STR_SIZE);
        }
        break;
      case QDP_RIL_PASSWORD:
        if (params[i].len > QMI_WDS_MAX_USERNAME_PASS_STR_SIZE-1)
        {
          QDP_LOG_ERROR("Invalid length [%d] of PASSWORD [%s] rcvd",
                        params[i].len, params[i].buf);
          reti = QDP_FAILURE;
          break;
        }
        else if (params[i].len > 0)
        {
          profile_params.umts_profile_params.param_mask |=
            QMI_WDS_UMTS_PROFILE_PASSWORD_PARAM_MASK;
          strlcpy(profile_params.umts_profile_params.password,
                  params[i].buf,
                  QMI_WDS_MAX_USERNAME_PASS_STR_SIZE);
        }
        break;
      case QDP_RIL_AUTH:
        if (params[i].len > 0)
        {
          if (strcmp(params[i].buf, QDP_RIL_PAP_CHAP_NOT_ALLOWED) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_AUTH_PREF_PARAM_MASK;
            profile_params.umts_profile_params.auth_pref =
              QMI_WDS_AUTH_PREF_PAP_CHAP_NOT_ALLOWED;
          }
          else if (strcmp(params[i].buf, QDP_RIL_PAP_ONLY_ALLOWED) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_AUTH_PREF_PARAM_MASK;
            profile_params.umts_profile_params.auth_pref =
              QMI_WDS_AUTH_PREF_PAP_ONLY_ALLOWED;
          }
          else if (strcmp(params[i].buf, QDP_RIL_CHAP_ONLY_ALLOWED) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_AUTH_PREF_PARAM_MASK;
            profile_params.umts_profile_params.auth_pref =
              QMI_WDS_AUTH_PREF_CHAP_ONLY_ALLOWED;
          }
          else if (strcmp(params[i].buf, QDP_RIL_PAP_CHAP_BOTH_ALLOWED) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_AUTH_PREF_PARAM_MASK;
            profile_params.umts_profile_params.auth_pref =
              QMI_WDS_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED;
          }
          else
          {
            QDP_LOG_ERROR("invalid auth_pref [%s]", params[i].buf);
            reti = QDP_FAILURE;
            break;
          }
        }
        break;
      case QDP_RIL_IP_FAMILY:
        if (params[i].len > 0)
        {
          if (strcmp(params[i].buf, QDP_RIL_IP_4) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_PDP_TYPE_PARAM_MASK;
            profile_params.umts_profile_params.pdp_type =
              QMI_WDS_PDP_TYPE_IP;
          }
          else if(strcmp(params[i].buf, QDP_RIL_IP_6) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_PDP_TYPE_PARAM_MASK;
            profile_params.umts_profile_params.pdp_type =
              QMI_WDS_PDP_TYPE_IPV6;
          }
          else if(strcmp(params[i].buf, QDP_RIL_IP_4_6) == 0)
          {
            profile_params.umts_profile_params.param_mask |=
              QMI_WDS_UMTS_PROFILE_PDP_TYPE_PARAM_MASK;
            profile_params.umts_profile_params.pdp_type =
              QMI_WDS_PDP_TYPE_IPV4V6;
          }
          else
          {
            QDP_LOG_ERROR("invalid ip_family [%s]", params[i].buf);
            reti = QDP_FAILURE;
            break;
          }
        }
        break;
      default:
        QDP_LOG_DEBUG("ignoring RIL param idx [%d]", i);
        break;
      } /* switch */
      if (QDP_FAILURE == reti)
      {
        break;
      }
    } /* for */
    if (QDP_FAILURE == reti)
    {
      break;
    }

    profile_id.technology = QMI_WDS_PROFILE_TECH_3GPP;

    if( TRUE == is_persistent)
    {
          /* Set profile to be persistent */
      profile_params.umts_profile_params.param_mask |=
            QMI_WDS_UMTS_PROFILE_IS_PERSISTENT_PARAM_MASK;
      profile_params.umts_profile_params.is_persistent = QMI_WDS_TRUE;
    }
    /* Check for non-persistent profile support */
    else if( IS_QMI_VER_MET( qmi_wds_version,
                        QMI_WDS_PERFLG_VERSION_MAJ, QMI_WDS_PERFLG_VERSION_MIN ) )
    {
      /* Set profile to be non-persistent */
      profile_params.umts_profile_params.param_mask |=
        QMI_WDS_UMTS_PROFILE_IS_PERSISTENT_PARAM_MASK;
      profile_params.umts_profile_params.is_persistent = QMI_WDS_FALSE;
    }

    /* set proprietary name for identification */
    profile_params.umts_profile_params.param_mask |=
      QMI_WDS_UMTS_PROFILE_NAME_PARAM_MASK;
    strlcpy(profile_params.umts_profile_params.profile_name,
            QDP_UNIQUE_PROFILE_NAME,
            QMI_WDS_MAX_PROFILE_STR_SIZE);

    rc = qmi_wds_create_profile( global_qmi_wds_hndl,
                                 &profile_id,
                                 &profile_params,
                                 &qmi_err_code);
    if (QMI_NO_ERR != rc)
    {
      QDP_LOG_ERROR("could not create wds profile. " \
                    "qmi returned [%d] qmi_err_code [%d]",
                    rc,qmi_err_code);
      break;
    }
    QDP_LOG_DEBUG("profile [%d] created",
                  profile_id.profile_index);

    *profile_id_ptr = profile_id.profile_index;
    *profile_pdn_type = qdp_get_3gpp_profile_pdn_type(&profile_params);

    if (NULL != p_params)
    {
      /* Copy the profile information into the out placeholder for profile params */
      memcpy( p_params, &profile_params, sizeof(qmi_wds_profile_params_type));
    }

    ret = QDP_SUCCESS;
  } while (0);

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_3gpp_profile_create EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_3gpp_profile_create EXIT success");
  }

  return ret;
}

/*===========================================================================
  FUNCTION:  qdp_3gpp2_profile_create
===========================================================================*/
/*!
    @brief
    creates a new 3gpp2 profile based on the default 3gpp profile and
    overrides any parameters that are given as input to this function.

    reference count on the newly created profile id is initialized with
    value 1

    @params
    params:
    num_params: number of parameters given as input
    params: pointer to the array of parameters such that *param[i] is
    the ith parameter
    profile_id: placeholder to return the newly created profile id

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
int qdp_3gpp2_profile_create
(
  qdp_param_t * params,
  unsigned int * profile_id_ptr,
  qdp_profile_pdn_type *profile_pdn_type
)
{
  qmi_wds_profile_id_type profile_id;
  qmi_wds_profile_params_type profile_params;
  int ret = QDP_FAILURE, i=0, reti = QDP_SUCCESS, rc;
  int qmi_err_code;

  QDP_LOG_DEBUG("%s","qdp_3gpp2_profile_create ENTRY");

  QDP_INIT_BARRIER;

  do
  {
    if (NULL == params ||
        NULL == profile_id_ptr)
    {
      QDP_LOG_ERROR("%s","NULL start_nw_params received");
      break;
    }

    if (QDP_INVALID_PROFILE != *profile_id_ptr)
    {
      QDP_LOG_ERROR("%s","placeholder contains non-zero value");
      break;
    }

    /* start with clean set of params */
    memset(&profile_params, 0, sizeof(profile_params));
    memset(&profile_id, 0, sizeof(profile_id));

    reti = QDP_SUCCESS;
    for(i=0; i<QDP_RIL_PARAM_MAX; i++)
    {
      switch(i)
      {
      case QDP_RIL_APN:
        if (params[i].len > QMI_WDS_MAX_APN_STR_SIZE-1)
        {
          QDP_LOG_ERROR("Invalid length [%d] of APN [%s] rcvd",
                        params[i].len, params[i].buf);
          reti = QDP_FAILURE;
          break;
        }
        else if (params[i].len > 0)
        {
          profile_params.cdma_profile_params.param_mask |=
            QMI_WDS_CDMA_PROFILE_APN_STRING_PARAM_MASK;
          strlcpy(profile_params.cdma_profile_params.apn_name,
                  params[i].buf,
                  QMI_WDS_MAX_APN_STR_SIZE);
        }
        break;
      case QDP_RIL_NAI:
        if (params[i].len > QMI_WDS_MAX_USERNAME_PASS_STR_SIZE-1)
        {
          QDP_LOG_ERROR("Invalid length [%d] of APN [%s] rcvd",
                        params[i].len, params[i].buf);
          reti = QDP_FAILURE;
          break;
        }
        else if (params[i].len > 0)
        {
          profile_params.cdma_profile_params.param_mask |=
            QMI_WDS_CDMA_PROFILE_USERNAME_PARAM_MASK;
          strlcpy(profile_params.cdma_profile_params.username,
                  params[i].buf,
                  QMI_WDS_MAX_USERNAME_PASS_STR_SIZE);
        }
        break;
      case QDP_RIL_PASSWORD:
        if (params[i].len > QMI_WDS_MAX_USERNAME_PASS_STR_SIZE-1)
        {
          QDP_LOG_ERROR("Invalid length [%d] of APN [%s] rcvd",
                        params[i].len, params[i].buf);
          reti = QDP_FAILURE;
          break;
        }
        else if (params[i].len > 0)
        {
          profile_params.cdma_profile_params.param_mask |=
            QMI_WDS_CDMA_PROFILE_PASSWORD_PARAM_MASK;
          strlcpy(profile_params.cdma_profile_params.password,
                  params[i].buf,
                  QMI_WDS_MAX_USERNAME_PASS_STR_SIZE);
        }
        break;
      case QDP_RIL_AUTH:
        if (params[i].len > 0)
        {
          if (strcmp(params[i].buf, QDP_RIL_PAP_CHAP_NOT_ALLOWED) == 0)
          {
            profile_params.cdma_profile_params.param_mask |=
              QMI_WDS_CDMA_PROFILE_AUTH_PROTOCOL_PARAM_MASK;
            profile_params.cdma_profile_params.auth_protocol =
              QMI_WDS_AUTH_PREF_PAP_CHAP_NOT_ALLOWED;
          }
          else if (strcmp(params[i].buf, QDP_RIL_PAP_ONLY_ALLOWED) == 0)
          {
            profile_params.cdma_profile_params.param_mask |=
              QMI_WDS_CDMA_PROFILE_AUTH_PROTOCOL_PARAM_MASK;
            profile_params.cdma_profile_params.auth_protocol =
              QMI_WDS_AUTH_PREF_PAP_ONLY_ALLOWED;
          }
          else if (strcmp(params[i].buf, QDP_RIL_CHAP_ONLY_ALLOWED) == 0)
          {
            profile_params.cdma_profile_params.param_mask |=
              QMI_WDS_CDMA_PROFILE_AUTH_PROTOCOL_PARAM_MASK;
            profile_params.cdma_profile_params.auth_protocol =
              QMI_WDS_AUTH_PREF_CHAP_ONLY_ALLOWED;
          }
          else if (strcmp(params[i].buf, QDP_RIL_PAP_CHAP_BOTH_ALLOWED) == 0)
          {
            profile_params.cdma_profile_params.param_mask |=
              QMI_WDS_CDMA_PROFILE_AUTH_PROTOCOL_PARAM_MASK;
            profile_params.cdma_profile_params.auth_protocol =
              QMI_WDS_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED;
          }
          else
          {
            QDP_LOG_ERROR("invalid auth_pref [%s]", params[i].buf);
            reti = QDP_FAILURE;
            break;
          }
        }
        break;
      case QDP_RIL_IP_FAMILY:
        if (params[i].len > 0)
        {
          if (strcmp(params[i].buf, QDP_RIL_IP_4) == 0)
          {
            profile_params.cdma_profile_params.param_mask |=
              QMI_WDS_CDMA_PROFILE_PDN_TYPE_PARAM_MASK;
            profile_params.cdma_profile_params.pdn_type =
              QMI_WDS_IPV4_PDN_TYPE;
          }
          else if(strcmp(params[i].buf, QDP_RIL_IP_6) == 0)
          {
            profile_params.cdma_profile_params.param_mask |=
              QMI_WDS_CDMA_PROFILE_PDN_TYPE_PARAM_MASK;
            profile_params.cdma_profile_params.pdn_type =
              QMI_WDS_IPV6_PDN_TYPE;
          }
          else if(strcmp(params[i].buf, QDP_RIL_IP_4_6) == 0)
          {
            profile_params.cdma_profile_params.param_mask |=
              QMI_WDS_CDMA_PROFILE_PDN_TYPE_PARAM_MASK;
            profile_params.cdma_profile_params.pdn_type =
              QMI_WDS_IPV4_OR_IPV6_PDN_TYPE;
          }
          else
          {
            QDP_LOG_ERROR("invalid ip_family [%s]", params[i].buf);
            reti = QDP_FAILURE;
            break;
          }
        }
        break;
      default:
        QDP_LOG_DEBUG("ignoring RIL param idx [%d]", i);
        break;
      } /* switch */
      if (QDP_FAILURE == reti)
      {
        break;
      }
    } /* for */
    if (QDP_FAILURE == reti)
    {
      break;
    }

    profile_id.technology = QMI_WDS_PROFILE_TECH_3GPP2;

    /* Check for non-persistent profile support */
    if( IS_QMI_VER_MET( qmi_wds_version,
                        QMI_WDS_PERFLG_VERSION_MAJ, QMI_WDS_PERFLG_VERSION_MIN ) )
    {
      /* Set profile to be non-persistent */
      profile_params.cdma_profile_params.param_mask |=
        QMI_WDS_CDMA_PROFILE_IS_PERSISTENT_PARAM_MASK;
      profile_params.cdma_profile_params.is_persistent = QMI_WDS_FALSE;
    }

    rc = qmi_wds_create_profile( global_qmi_wds_hndl,
                                 &profile_id,
                                 &profile_params,
                                 &qmi_err_code);
    if (QMI_NO_ERR != rc)
    {
      QDP_LOG_ERROR("could not create wds profile. " \
                    "qmi returned [%d] qmi_err_code [%d]",
                    rc, qmi_err_code);
      break;
    }
    QDP_LOG_DEBUG("profile [%d] created",
                  profile_id.profile_index);

    *profile_id_ptr = profile_id.profile_index;
    *profile_pdn_type = qdp_get_3gpp2_profile_pdn_type(&profile_params);

    ret = QDP_SUCCESS;
  } while (0);

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_3gpp2_profile_create EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_3gpp2_profile_create EXIT success");
  }

  return ret;
}

/*===========================================================================
  FUNCTION:  qdp_profile_ref_up
===========================================================================*/
/*!
    @brief
    increases the reference count on the given profile

    @params
    profile_id: profile id

    @return
    QDP_FAILURE
    QDP_SUCCESS
*/
/*=========================================================================*/
int qdp_profile_ref_up
(
  unsigned int profile_id
)
{
  int i=0;
  for(i=0; i<QDP_NUM_PROFILES_EXPECTED_MAX; i++)
  {
    if (profile_ids[i].qmi_type.profile_index == profile_id)
    {
      profile_ids[i].ref_count++;
      QDP_LOG_DEBUG("new ref_count on profile [%d] is [%d]",
                    profile_id, profile_ids[i].ref_count);
      return QDP_SUCCESS;
    }
  }

  QDP_LOG_DEBUG("profile id [%d] not found in QDP list", profile_id);
  return QDP_FAILURE;
}

/*===========================================================================
  FUNCTION:  qdp_profile_release
===========================================================================*/
/*!
    @brief
    decreases the reference count on the given profile

    profile is automatically deleted when ref count goes to zero

    @params
    profile_id: profile id

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
int qdp_profile_release
(
  unsigned int profile_id
)
{
  int i=0;

  QDP_LOG_DEBUG("%s","qdp_profile_release ENTRY");

  for(i=0; i<QDP_NUM_PROFILES_EXPECTED_MAX; i++)
  {
    if (profile_ids[i].qmi_type.profile_index == profile_id)
    {
      if (0 == profile_ids[i].ref_count)
      {
        QDP_LOG_ERROR("ref count on profile id [%d] is already zero",
                      profile_id);
        return QDP_FAILURE;
      }

      profile_ids[i].ref_count--;
      QDP_LOG_DEBUG("new ref_count on profile [%d] is [%d]",
                    profile_id, profile_ids[i].ref_count);

      if (0 == profile_ids[i].ref_count)
      {
        QDP_LOG_DEBUG("deleting profile id [%d] now", profile_id);
        qdp_delete_profile_id(profile_id);
      }

      return QDP_SUCCESS;
    }
  }

  QDP_LOG_DEBUG("profile id [%d] not found in QDP list", profile_id);
  return QDP_FAILURE;
}

/*===========================================================================
  FUNCTION:  qdp_lte_attach_profile_lookup
===========================================================================*/
/*!
  @brief
  This function is used to query and update LTE attach profiles on the modem.
  It receives the new LTE attach parameters and tries to lookup valid 3GPP
  profiles. If a valid profile does not exist then a new profile will be
  created with the profile parameters received in the input. If a valid
  profile exists, it will be updated with the new parameters.

  @params
  param_strings: a pointer to the array of parameter string which contains
  the new LTE attach parameters.

  profile_id_lte: the profile id which will be updated.

  input_prof_id: The input profile id from legacy behaviour in which case
  this will hold the default LTE attach profile id or index 0 entry from
  attach pdn list or Invalid if no entry in the attach list

  prof_params: out parameter which will hold the profile parameters
  before the profile is updated. If the LTE attach process fails we would
  need this to restore the modem profile to its previous state

  @return
  QDP_SUCCESS
  QDP_FAILURE
*/
/*=========================================================================*/
int qdp_lte_attach_profile_lookup
(
  const char                  ** param_strings,
  unsigned int                * profile_id_lte,
  qdp_profile_pdn_type        * profile_pdn_type_lte,
  unsigned int                * input_prof_id,
  qmi_wds_profile_params_type * prof_params,
  qdp_error_info_t            * error_info
)
{
  int ret = QDP_FAILURE, rc, qmi_err_code;
  int i=0, temp_len = 0;
  int error_code = 0;
  int create_profile = FALSE;

  /* profile lookup params */
  qdp_param_t params_lte[QDP_RIL_PARAM_MAX];
  qdp_error_t lookup_error =  QDP_ERROR_NONE;

  /* Profile id type that will be used for the default
   * LTE attach process */
  qmi_wds_profile_id_type        default_prof_id;

  memset(params_lte, 0, sizeof(params_lte));
  QDP_LOG_DEBUG("%s","qdp_lte_attach_profile_lookup ENTRY");

  do
  {
    if( NULL == param_strings          ||
        NULL == profile_id_lte         ||
        NULL == profile_pdn_type_lte   ||
        NULL == input_prof_id          ||
        NULL == error_info )
    {
      QDP_LOG_ERROR("%s","NULL params rcvd");
      break;
    }

    /* go through each of the RIL parameter received in order to
     * determine which technology we need to look up profile for
     * prepare profile_look_up parameters */
    for(i=0; i<QDP_RIL_PARAM_MAX; i++)
    {
      if (param_strings[QDP_GET_RIL_PARAM_IDX(i)] != NULL &&
          strlen(param_strings[QDP_GET_RIL_PARAM_IDX(i)]) != 0)
      {
        /* +1 for NULL end character */
        temp_len = strlen(param_strings[QDP_GET_RIL_PARAM_IDX(i)])+1;
        if(temp_len > QDP_GET_3GPP_MAX_LEN(i))
        {
          QDP_LOG_ERROR("RIL param length too long [%d] for ril index [%d]",
                        temp_len, i);
        }
        else
        {
          QDP_MALLOC(params_lte[i].buf, temp_len);
        }

        if(NULL != params_lte[i].buf)
        {
          params_lte[i].len = temp_len-1;
          strlcpy(params_lte[i].buf,
                  param_strings[QDP_GET_RIL_PARAM_IDX(i)],
                  temp_len);
          QDP_LOG_DEBUG("copied [%s], len[%d] to [%p] loc",
                        param_strings[QDP_GET_RIL_PARAM_IDX(i)],
                        params_lte[i].len,
                        params_lte[i].buf);
        }
        else
        {
          QDP_LOG_ERROR("memory error while trying to allocate 3gpp "
                        "param for [%s]", param_strings[QDP_GET_RIL_PARAM_IDX(i)]);
        }
      } /* for each valid RIL parameter */
    } /* for each RIL parameter */

    /* search the all modem profiles  first*/
    *profile_id_lte = QDP_INVALID_PROFILE;
    *profile_pdn_type_lte  = QDP_PROFILE_PDN_TYPE_INVALID;

    error_info->error = QDP_ERROR_NONE;
    error_info->tech = QDP_NOTECH;

    /* Search for a matching profile */
    rc = qdp_3gpp_multi_param_search( params_lte,
                                      profile_id_lte,
                                      profile_pdn_type_lte,
                                      prof_params,
                                      &lookup_error);

    if (QDP_SUCCESS != rc)
    {
      QDP_LOG_ERROR("qdp_3gpp_multi_param_search returned err [%d]", lookup_error);
      break;
    }

    if (QDP_INVALID_PROFILE == *profile_id_lte)
    {
      if( QDP_ERROR_NONE != lookup_error )
      {
        QDP_LOG_DEBUG("Profile lookup error[%d], no profile created",
                        lookup_error );
        error_info->error = lookup_error;
        error_info->tech = QDP_3GPP;
        break;
      }
    }

    /* case 1: No look up error,One or more profiles found */
    if (QDP_INVALID_PROFILE != *profile_id_lte)
    {
       QDP_LOG_DEBUG("found [%d] 3gpp profile id",
                      *profile_id_lte);

       /* Update the profile
              * But Can we update modem profile then??? */
       rc = qdp_3gpp_profile_update( params_lte,
                                     *profile_id_lte,
                                     &error_code);

       if (QDP_SUCCESS != rc)
       {
         QDP_LOG_ERROR("Could not update profile [%d] on modem with LTE info,"
                         "Error code [%d][%d]",*profile_id_lte, rc, error_code);
         break;
       }
    }
    /* case 2:No look up error,no modem profile found and if input prof id given */
    else if (QDP_INVALID_PROFILE != *input_prof_id)
    {
      /* Prepare to lookup profile */
      create_profile = FALSE;
      default_prof_id.technology = QMI_WDS_PROFILE_TECH_3GPP;
      default_prof_id.profile_index = *input_prof_id;

      rc = qmi_wds_query_profile( global_qmi_wds_hndl,
                                  &default_prof_id,
                                  prof_params,
                                  &qmi_err_code);

       /* if query failed , let us the create the profile */

      if (QMI_NO_ERR != rc)
      {
        QDP_LOG_ERROR("Querying default profile failed with error code [%d][%d]",
                        rc, qmi_err_code);
         create_profile = TRUE;
      }
      else
      {
         if ((prof_params != NULL) && (((prof_params->umts_profile_params).param_mask) &
              QMI_WDS_UMTS_PROFILE_NAME_PARAM_MASK))
         {
           if(strncasecmp(QDP_UNIQUE_PROFILE_NAME,
                         (prof_params->umts_profile_params).profile_name,
                         std_strlen(QDP_UNIQUE_PROFILE_NAME)))
           {
             QDP_LOG_DEBUG("%s","it is not a QDP created profile,create new prof");
             create_profile = TRUE;
           }
         }
      }
      /* query fail or not a QDP profile then create a new prof */
      if (TRUE == create_profile)
      {
        QDP_LOG_ERROR("%s","Querying default profile failed or it is a modem profile ");

        /* Create the default profile */
        *profile_id_lte = QDP_INVALID_PROFILE;

        rc = qdp_3gpp_profile_create( params_lte,
                                      profile_id_lte,
                                      profile_pdn_type_lte,
                                      prof_params,
                                      TRUE);

        if (QDP_SUCCESS != rc)
        {
          QDP_LOG_ERROR("%s", "Default profile creation failed!");
          break;
        }
        else if (*profile_id_lte != QDP_INVALID_PROFILE)
        {
          qdp_insert_profile_id(*profile_id_lte, QDP_3GPP);
          qdp_profile_ref_up(*profile_id_lte);
        }
      }
       /* query passed , input prof id is a QDP created prof id,so we can update it */
      else
      {
        QDP_LOG_DEBUG("Profile [%d] already exists", *input_prof_id);
        /* Update the profile */
        rc = qdp_3gpp_profile_update( params_lte,
                                      *input_prof_id,
                                      &error_code);

        if (QDP_SUCCESS != rc)
        {
          QDP_LOG_ERROR("Could not update profile [%d] on modem with LTE"
                          "information,Error code [%d][%d]",*input_prof_id,
                           rc, error_code);
          break;
        }
        *profile_id_lte = *input_prof_id;
       }
    }
      /* case 3: No profile look up error,No matching profile found at modem,
         * no input prof id provided, so create a new one- empty list  */
    else if (QDP_INVALID_PROFILE == *input_prof_id)
    {
        QDP_LOG_DEBUG("%s", "qdp_3gpp_multi_param_search could not find a valid profile");
        rc = qdp_3gpp_profile_create( params_lte,
                                      profile_id_lte,
                                      profile_pdn_type_lte,
                                      prof_params,
                                      TRUE);
        if (QDP_SUCCESS != rc)
        {
           QDP_LOG_ERROR("qdp_3gpp_profile_create failed [%d]",rc);
           break;
        }
        else if (QDP_INVALID_PROFILE != *profile_id_lte)
        {
           qdp_insert_profile_id(*profile_id_lte, QDP_3GPP);
           qdp_profile_ref_up(*profile_id_lte);
        }
    }
    ret = QDP_SUCCESS;
  } while (0);

  /* clean up memory */
  qdp_free_qdp_params(params_lte, QDP_RIL_PARAM_MAX);

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_lte_attach_profile_lookup EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_lte_attach_profile_lookup EXIT success");
  }

  return ret;

} /* qdp_lte_attach_profile_lookup */
/*===========================================================================
  FUNCTION:  qdp_profile_look_up
===========================================================================*/
/*!
    @brief
    Given the set of RIL SETUP_DATA_CALL parameter string array
    (please refer to qdp_ril_param_idx_t to see what order strings
    are expected to be in, to omit certain parameter(s), leave the
    corresponding entry NULL in the array), this function determines
    the applicable technologies, and returns the corresponding 3gpp
    and/or 3gpp2 profile id(s) to be used for data call route look up.
    If required, this function would create new profile(s) on behalf
    of the caller.

    if a non-zero profile_id is returned for a given technology
    (i.e. 3gpp, 3gpp2), the caller of this API is automatically
    assumed to have a reference count on it, which, in turn, must
    be released by calling qdp_profile_release() API when ever
    the caller no longer needs to sustain a data call with that
    profile id.

    @params
    param_strings: a pointer to the array of parameter string such
    that *param_strings[i] is ith parameter string
    profile_id_3gpp: placeholder for the 3gpp profile id (output)
    profile_id_3gpp2: placeholder for the 3gpp2 profile id (output)
    ril_tech: current technology specified at RIL API
    error_info: place holder for specific lookup error information

    @Examples
    User can query both 3gpp and 3gpp2 profile ids using subset of parameters
    listed in qdp_ril_param_idx_t.

    Example 1: if user provides valid values for
    QDP_RIL_TECH = QDP_RIL_3GPP2
    QDP_RIL_APN = "3GPP2_APN"
    qdp_profile_look_up() will try to look up *only* 3gpp2 profile id with
    APN set to "3GPP2_APN"

    Example 2: If user provides valid values for
    QDP_RIL_TECH = "QDP_RIL_AUTO"
    QDP_RIL_APN = "APN"
    QDP_RIL_NAI = "USER"
    qdp_profile_look_up() will try to look up
    * 3gpp profile using "APN" (NAI does not apply to 3GPP profiles)
    * 3gpp2 profile using "APN", and "USER"

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
int qdp_profile_look_up
(
  const char  ** param_strings,    /* the order of params must match with the
                                      order specified in qdp_ril_param_idx_t */
  unsigned int * profile_id_3gpp,  /* value (not pointer it-self) must
                                      be set to zero by caller */
  qdp_profile_pdn_type *profile_pdn_type_3gpp,  /* 3gpp profile PDN type */
  unsigned int * profile_id_3gpp2, /* value must be set to zero by caller */
  qdp_profile_pdn_type *profile_pdn_type_3gpp2, /* 3gpp2 profile PDN type */
  qdp_error_info_t * error_info
)
{
  /* based on the input RIL parameters,
   * figure out which technologies are
   * applicable and perform profile look
   * ups for it */
  int ret = QDP_FAILURE, rc;
  int i=0, temp_len = 0;
  boolean lookup_3gpp_profile = FALSE;
  boolean lookup_3gpp2_profile = FALSE;
  int tech_mask = 0;
  int temp_profile_id = 0;
  /* profile lookup params */
  qdp_param_t params_3gpp[QDP_RIL_PARAM_MAX];
  qdp_param_t params_3gpp2[QDP_RIL_PARAM_MAX];
  qdp_error_t lookup_error =  QDP_ERROR_NONE;

  memset(params_3gpp, 0, sizeof(params_3gpp));
  memset(params_3gpp2, 0, sizeof(params_3gpp2));
  QDP_LOG_DEBUG("%s","qdp_profile_look_up ENTRY");

  do
  {
    if( NULL == param_strings          ||
        NULL == profile_id_3gpp        ||
        NULL == profile_pdn_type_3gpp  ||
        NULL == profile_id_3gpp2       ||
        NULL == profile_pdn_type_3gpp2 ||
        NULL == error_info )
    {
      QDP_LOG_ERROR("%s","NULL params rcvd");
      break;
    }

    /* don't want to step on real profile ids */
    if (*profile_id_3gpp || *profile_id_3gpp2)
    {
      QDP_LOG_ERROR("%s","non-zero values rcvd");
      break;
    }

    *profile_id_3gpp = QDP_INVALID_PROFILE;
    *profile_id_3gpp2 = QDP_INVALID_PROFILE;
    *profile_pdn_type_3gpp  = QDP_PROFILE_PDN_TYPE_INVALID;
    *profile_pdn_type_3gpp2 = QDP_PROFILE_PDN_TYPE_INVALID;

    error_info->error = QDP_ERROR_NONE;
    error_info->tech = QDP_NOTECH;

    /* did RIL provide a technology preference? */
    if (NULL != param_strings[QDP_RIL_TECH])
    {
      if (0 == strncmp(QDP_RIL_3GPP,
                       param_strings[QDP_RIL_TECH],
                       QDP_RIL_TECH_LEN))
      {
        QDP_LOG_DEBUG("%s", "RIL prefers 3GPP");
        lookup_3gpp_profile = TRUE;
      }
      else if(0 == strncmp(QDP_RIL_3GPP2,
                           param_strings[QDP_RIL_TECH],
                           QDP_RIL_TECH_LEN))
      {
        QDP_LOG_DEBUG("%s", "RIL prefers 3GPP2");
        lookup_3gpp2_profile = TRUE;
      }
      else if(0 == strncmp(QDP_RIL_AUTO,
                           param_strings[QDP_RIL_TECH],
                           QDP_RIL_TECH_LEN))
      {
        QDP_LOG_DEBUG("%s", "RIL prefers Automatic tech");
        /* we will use the RIL parameters in order to
         * determine as for which technology profile need
         * to be looked up */
      }
      else
      {
        QDP_LOG_ERROR("RIL provied invalid tech [%s]",
                        param_strings[QDP_RIL_TECH]);
        break;
      }
    }

    /* did RIL already provide a profile id? */
    if (NULL != param_strings[QDP_RIL_PROFILE_ID])
    {
      temp_profile_id = atoi(param_strings[QDP_RIL_PROFILE_ID]);
      if (temp_profile_id < QDP_RIL_DATA_PROFILE_OEM_BASE)
      {
        QDP_LOG_DEBUG("RIL did not provide a valid OEM profile [%d]",
                        temp_profile_id);
        /* we will defer to profile_look_up that happens later
         * in this function */
      }
      else
      {
        temp_profile_id = (temp_profile_id -
                           QDP_RIL_DATA_PROFILE_OEM_BASE);
        if (TRUE == lookup_3gpp_profile)
        {
          QDP_LOG_DEBUG("RIL provided 3GPP profile id [%d]",
                          temp_profile_id);
          *profile_id_3gpp = temp_profile_id;
          ret = QDP_SUCCESS;
          break;
        }
        else if (TRUE == lookup_3gpp2_profile)
        {
          QDP_LOG_DEBUG("RIL provided 3GPP2 profile id [%d]",
                          temp_profile_id);
          *profile_id_3gpp2 = temp_profile_id;
          ret = QDP_SUCCESS;
          break;
        }
        else
        {
          QDP_LOG_DEBUG("RIL provided 3GPP2, and 3GPP profile id [%d]",
                          temp_profile_id);
          *profile_id_3gpp = temp_profile_id;
          *profile_id_3gpp2 = temp_profile_id;
          ret = QDP_SUCCESS;
          break;
        }
      }
    }

    /* go through each of the RIL parameter received in order to
     * determine which technology we need to look up profile for
     * prepare profile_look_up parameters */
    for(i=0; i<QDP_RIL_PARAM_MAX; i++)
    {

      if (param_strings[QDP_GET_RIL_PARAM_IDX(i)] != NULL &&
          std_strlen(param_strings[QDP_GET_RIL_PARAM_IDX(i)]) != 0)
      {
        /* keep track of technology for all parameters together */
        tech_mask |= QDP_GET_TECH_MASK(i);

        if (QDP_GET_TECH_MASK(i) & QDP_3GPP)
        {
          /* +1 for NULL end character */
          temp_len = std_strlen(param_strings[QDP_GET_RIL_PARAM_IDX(i)])+1;
          if(temp_len > QDP_GET_3GPP_MAX_LEN(i))
          {
            QDP_LOG_ERROR("RIL param length too long [%d]",
                          temp_len);
          }
          else
          {
            QDP_MALLOC(params_3gpp[i].buf, temp_len);
          }

          if(NULL != params_3gpp[i].buf)
          {
            params_3gpp[i].len = temp_len-1;
            strlcpy(params_3gpp[i].buf,
                    param_strings[QDP_GET_RIL_PARAM_IDX(i)],
                    temp_len);
            QDP_LOG_DEBUG("copied [%s], len[%d] to [%p] loc",
                          param_strings[QDP_GET_RIL_PARAM_IDX(i)],
                          params_3gpp[i].len,
                          params_3gpp[i].buf);
          }
          else
          {
            QDP_LOG_ERROR("memory error while trying to allocate 3gpp "
                          "param for [%s]", param_strings[QDP_GET_RIL_PARAM_IDX(i)]);
          }
        } /* if 3GPP */

        if (QDP_GET_TECH_MASK(i) & QDP_3GPP2)
        {
          /* +1 for NULL end character */
          temp_len = std_strlen(param_strings[QDP_GET_RIL_PARAM_IDX(i)])+1;
          if(temp_len > QDP_GET_3GPP2_MAX_LEN(i))
          {
            QDP_LOG_ERROR("RIL param length too long [%d]",
                          temp_len);
          }
          else
          {
            QDP_MALLOC(params_3gpp2[i].buf, temp_len);
          }

          if(NULL != params_3gpp2[i].buf)
          {
            params_3gpp2[i].len = temp_len - 1;
            QDP_LOG_DEBUG("copying [%s] len [%d] to [%p] loc",
                          param_strings[QDP_GET_RIL_PARAM_IDX(i)],
                          params_3gpp2[i].len,
                          params_3gpp2[i].buf);
            strlcpy(params_3gpp2[i].buf,
                    param_strings[QDP_GET_RIL_PARAM_IDX(i)],
                    temp_len);
          }
          else
          {
            QDP_LOG_ERROR("memory error while trying to allocate 3gpp2 "
                          "param for [%s]", param_strings[QDP_GET_RIL_PARAM_IDX(i)]);
          }
        } /* if 3GPP2 */

      } /* for each valid RIL parameter */

    } /* for each RIL parameter */

    /* if RIL did not prefer a technology, infer what
     * profile lookup will be required based on the
     * parameter set */
    if (FALSE == lookup_3gpp_profile &&
        FALSE == lookup_3gpp2_profile)
    {
        if (tech_mask & QDP_3GPP)
        {
          lookup_3gpp_profile = TRUE;
        }
        if (tech_mask & QDP_3GPP2)
        {
          lookup_3gpp2_profile = TRUE;
        }
    }

    /* now we know which technology profile db needs tobe
     * looked up*/
    if (TRUE == lookup_3gpp_profile)
    {
      rc = qdp_3gpp_multi_param_search( params_3gpp,
                                        profile_id_3gpp,
                                        profile_pdn_type_3gpp,
                                        NULL,
                                        &lookup_error);
      if (QDP_SUCCESS != rc)
      {
        QDP_LOG_ERROR("qdp_3gpp_multi_param_search returned err [%d]", rc);
        /* do not break here...we want to continue 3gpp2 lookup */
      }
      else if(QDP_INVALID_PROFILE == *profile_id_3gpp)
      {
        QDP_LOG_DEBUG("qdp_3gpp_multi_param_search unsuccessful,"
                      " found [%d] profile id", *profile_id_3gpp);

        if( QDP_ERROR_NONE == lookup_error )
        {
          /* create a profile */
          rc = qdp_3gpp_profile_create(params_3gpp,
                                       profile_id_3gpp,
                                       profile_pdn_type_3gpp,
                                       NULL,
                                       FALSE);
          if (QDP_SUCCESS != rc)
          {
            QDP_LOG_ERROR("qdp_3gpp_profile_create failed [%d]",
                          rc);
            /* do not break, we would still like to query
             * for 3gpp2 */
          }
          if (*profile_id_3gpp != QDP_INVALID_PROFILE)
          {
            /* track the profile id for later clean up */
            qdp_insert_profile_id(*profile_id_3gpp, QDP_3GPP);
            qdp_profile_ref_up(*profile_id_3gpp);
          }
        }
        else
        {
          QDP_LOG_DEBUG("Profile lookup error[%d], no profile created",
                        lookup_error );
          error_info->error = lookup_error;
          error_info->tech = QDP_3GPP;
          break;
        }
      }
      else /* one or more profiles found */
      {
        QDP_LOG_DEBUG("found [%d] 3gpp profile id",
                        *profile_id_3gpp);
        /* check to see if this profile is one of ours */
        if (QDP_SUCCESS == qdp_profile_ref_up(*profile_id_3gpp))
        {
          QDP_LOG_DEBUG("profile id [%d] is a temp 3gpp profile",
                        *profile_id_3gpp);
        }
        else
        {
          QDP_LOG_DEBUG("profile id [%d] is an existing modem 3gpp profile",
                        *profile_id_3gpp);
        }
      }
    }

    if (TRUE == lookup_3gpp2_profile)
    {
      rc = qdp_3gpp2_multi_param_search( params_3gpp2,
                                         profile_id_3gpp2,
                                         profile_pdn_type_3gpp2,
                                         &lookup_error );
      if (QDP_SUCCESS != rc)
      {
        QDP_LOG_ERROR("qdp_3gpp2_multi_param_search returned err [%d]", rc);
        break;
      }

      if (QDP_INVALID_PROFILE == *profile_id_3gpp2)
      {
        QDP_LOG_DEBUG("qdp_3gpp2_multi_param_search unsuccessful, "
                       "found [%d] profile id", *profile_id_3gpp2);

        if( QDP_ERROR_NONE == lookup_error )
        {
          /* create a profile */
          rc = qdp_3gpp2_profile_create(params_3gpp2,
                                        profile_id_3gpp2,
                                        profile_pdn_type_3gpp2);
          if (QDP_SUCCESS != rc)
          {
            QDP_LOG_ERROR("qdp_3gpp2_profile_create failed [%d]",
                          rc);
            break;
          }
          if (*profile_id_3gpp2 != QDP_INVALID_PROFILE)
          {
            /* track the profile id for later clean up */
            qdp_insert_profile_id(*profile_id_3gpp2, QDP_3GPP2);
            qdp_profile_ref_up(*profile_id_3gpp2);
          }
        }
        else
        {
          QDP_LOG_DEBUG("Profile lookup error[%d], no profile created",
                        lookup_error );
          error_info->error = lookup_error;
          error_info->tech = QDP_3GPP2;
          break;
        }
      }
      else
      {
        QDP_LOG_DEBUG("3gpp2 profile [%d] found",
                        *profile_id_3gpp2);
        /* check to see if this profile is one of ours */
        if (QDP_SUCCESS == qdp_profile_ref_up(*profile_id_3gpp2))
        {
          QDP_LOG_DEBUG("profile id [%d] is a temp 3gpp2 profile",
                        *profile_id_3gpp2);
        }
        else
        {
          QDP_LOG_DEBUG("profile id [%d] is an existing modem 3gpp2 profile",
                        *profile_id_3gpp2);
        }
      }
    }

    ret = QDP_SUCCESS;
  } while (0);

  /* clean up memory */
  qdp_free_qdp_params(params_3gpp, QDP_RIL_PARAM_MAX);
  qdp_free_qdp_params(params_3gpp2, QDP_RIL_PARAM_MAX);

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_profile_look_up EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_profile_look_up EXIT success");
  }
  return ret;
}

/*===========================================================================
  FUNCTION:  qdp_init_profile_cleanup
===========================================================================*/
/*!
    @brief
    Cleans up temp profiles (if any) at initialization time.
    Temp profiles, when created, are created with a unique
    profile name QDP_UNIQUE_PROFILE_NAME. Thus profile_name is used as a
    search parameter to identify all previously created
    temp profiles.

    @params
    default_port: default qmi port to be used for QDP operations

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
int qdp_init_profile_cleanup(void)
{
  int ret = QDP_FAILURE, i, rc, qmi_err_code, reti;
  int num_elements_expected = QDP_NUM_UMTS_PROFILES_EXPECTED;
  qmi_wds_profile_list_type result_list[QDP_NUM_UMTS_PROFILES_EXPECTED];
  qmi_wds_profile_id_type qmi_profile_id;
  qmi_wds_profile_tech_type profile_tech;
  qmi_wds_profile_params_type prof_params;

  do
  {
    memset(result_list, 0, sizeof(result_list));
    /* query umts profile id. */
    profile_tech = QMI_WDS_PROFILE_TECH_3GPP;
    rc = qmi_wds_utils_get_profile_list( global_qmi_wds_hndl,
                                         &profile_tech,
                                         NULL,
                                         result_list,
                                         &num_elements_expected,
                                         &qmi_err_code);
    if (QMI_NO_ERR != rc)
    {
      QDP_LOG_ERROR("couldn't get 3gpp profile list that matches [%s] name",
                    QDP_UNIQUE_PROFILE_NAME);
      QDP_LOG_ERROR("rc = [%d], qmi_err = [%d]", rc, qmi_err_code);
      break;
    }

    reti = QDP_SUCCESS;
    for (i=0; i<num_elements_expected; i++)
    {
      qmi_profile_id.technology = QMI_WDS_PROFILE_TECH_3GPP;
      qmi_profile_id.profile_index = result_list[i].profile_index;

      memset(&prof_params, 0, sizeof(prof_params));

      rc = qmi_wds_query_profile(
        global_qmi_wds_hndl,
        &qmi_profile_id,
        &prof_params,
        &qmi_err_code
        );
      if (QMI_NO_ERR != rc)
      {
        QDP_LOG_ERROR("query_profile failed with error [%d] " \
                      "qmi_err_code [%d]", rc, qmi_err_code);
        reti = QDP_FAILURE;
        break;
      }
      else if(prof_params.umts_profile_params.param_mask &
              QMI_WDS_UMTS_PROFILE_NAME_PARAM_MASK)
      {
        if(!strncasecmp(QDP_UNIQUE_PROFILE_NAME,
                        prof_params.umts_profile_params.profile_name,
                        std_strlen(QDP_UNIQUE_PROFILE_NAME)))
        {
          QDP_LOG_DEBUG("3gpp profile [%d] has name [%s]",
                        qmi_profile_id.profile_index,
                        prof_params.umts_profile_params.profile_name);
          rc = qmi_wds_delete_profile(global_qmi_wds_hndl,
                                      &qmi_profile_id,
                                      &qmi_err_code);
          if (QMI_NO_ERR != rc)
          {
            QDP_LOG_ERROR("**profile leak**: couldn't delete temp profile [%d],"
                          "return [%d], err [%d]",
                          result_list[i].profile_index, rc, qmi_err_code);
            reti = QDP_FAILURE;
            /* don't break as we want to try all profiles if possible */
          }
        }
        else
        {
          QDP_LOG_DEBUG("3gpp profile [%d] doesn't have name [%s]",
                        qmi_profile_id.profile_index,
                        QDP_UNIQUE_PROFILE_NAME);
        }
      } /* profile_name is set on profile */

    } /* for */
    if (QDP_SUCCESS != reti)
    {
      break;
    }

    ret = QDP_SUCCESS;
  } while (0);

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_init_profile_cleanup EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_init_profile_cleanup EXIT success");
  }

  return ret;

}

/*===========================================================================
  FUNCTION:  qdp_init
===========================================================================*/
/*!
    @brief
    initializes qdp module

    @params
    default_port: default qmi port to be used for QDP operations

    @return
    QDP_SUCCESS
    QDP_FAILURE
*/
/*=========================================================================*/
int qdp_init
(
  const char * default_port
)
{
  int ret = QDP_FAILURE;
  int qmi_ret, qmi_err_code;

  QDP_LOG_DEBUG("%s","qdp_init ENTRY");

  if (TRUE == qdp_inited)
  {
    QDP_LOG_ERROR("%s","qdp already inited");
    return QDP_SUCCESS;
  }

  do
  {

    if (NULL == default_port)
    {
      QDP_LOG_ERROR("%s","qdp_init: ERROR: NULL default port rcvd");
      break;
    }

    strlcpy(global_qmi_port, default_port, QDP_QMI_PORT_LEN);

    /* don't check for return status
     * this may return error if already
     * initialized */
    qmi_init(NULL,NULL);

    QDP_LOG_DEBUG("qdp_init: init conn on [%s]", global_qmi_port);
    qmi_ret = qmi_connection_init(global_qmi_port,
                                  &qmi_err_code);
    if (QMI_NO_ERR != qmi_ret)
    {
      QDP_LOG_ERROR("qmi_connection_init failed with error [%d][%d]",
                    qmi_ret, qmi_err_code);
      break;
    }

    /* get QMI WDS hndl */
    /* global qmi port is set by qdp_init */
    global_qmi_wds_hndl = qmi_wds_srvc_init_client(global_qmi_port,
                                                   NULL,
                                                   NULL,
                                                   &qmi_err_code);

    if (global_qmi_wds_hndl < 0)
    {
      QDP_LOG_ERROR("invalid qmi_wds_hndl [%p] returned. "
                    "qmi_err_code is set to [%d]", global_qmi_wds_hndl,
                    qmi_err_code);
      break;
    }

    /* Query QMI WDS version, as some features are not available on
     * older Modem packages */
    memset( &qmi_wds_version, 0x0, sizeof(qmi_wds_version) );
    if( QMI_NO_ERR !=
        (qmi_ret = qmi_service_get_version( global_qmi_port,
                                            QMI_WDS_SERVICE,
                                            &qmi_wds_version,
                                            &qmi_err_code )) )
    {
      QDP_LOG_ERROR( "failed to query WDS version rc[%d] qmi_err[%d]",
                     qmi_ret, qmi_err_code );
      break;
    }
    else
    {
      QDP_LOG_DEBUG( "QMI WDS verions reported [%d.%d]",
                     qmi_wds_version.major_ver, qmi_wds_version.minor_ver );
    }

    /* Check for non-persistent profile support */
    if( !IS_QMI_VER_MET( qmi_wds_version,
                         QMI_WDS_PERFLG_VERSION_MAJ, QMI_WDS_PERFLG_VERSION_MIN ) )
    {
      /* clean up left over profiles from last time (if any) */
      /* don't care of return value here */
      qdp_init_profile_cleanup();
    }

    QDP_LOG_DEBUG("acquired global wds hndl [%p]", global_qmi_wds_hndl);

    qdp_platform_init();

    /* init qdp profile meta info */
    qdp_init_profile_ids();

    qdp_inited = TRUE;

    ret = QDP_SUCCESS;
  } while (0);

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_init EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_init EXIT success");
  }

  return ret;

}

/*===========================================================================
  FUNCTION:  qdp_deinit
===========================================================================*/
/*!
    @brief
    free up resources acquired during initialization

    @params

    @return
    none
*/
/*=========================================================================*/
void qdp_deinit
(
  void
)
{
  int ret = QDP_FAILURE;
  int rc;
  int qmi_err_code = QMI_SERVICE_ERR_NONE;

  QDP_LOG_DEBUG("%s","qdp_deinit ENTRY");

  do
  {
    if (TRUE != qdp_inited)
    {
      QDP_LOG_ERROR("%s","qdp not inited yet");
      break;
    }

    qdp_inited = FALSE;

    /* clean up profiles created by qdp module */
    qdp_cleanup_profile_ids();

    if (-1 != global_qmi_wds_hndl)
    {
      QDP_LOG_DEBUG("releasing global wds hndl [%p]", global_qmi_wds_hndl);

      rc = qmi_wds_srvc_release_client(global_qmi_wds_hndl, &qmi_err_code);
      if (rc < 0)
      {
        QDP_LOG_ERROR("wds srvc release failed. qmi_ret=[%d],qmi_err=[%d]",
                      rc,qmi_err_code);
        break;
      }
    }

    ret = QDP_SUCCESS;
  } while (0);

  if (QDP_SUCCESS != ret)
  {
    QDP_LOG_ERROR("%s","qdp_deinit EXIT failed");
  }
  else
  {
    QDP_LOG_DEBUG("%s","qdp_deinit EXIT success");
  }

}
