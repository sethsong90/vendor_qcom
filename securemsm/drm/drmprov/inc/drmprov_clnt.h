#ifndef __DRMPROV_CLIENT_H_
#define __DRMPROV_CLIENT_H_
/*===========================================================================
  Copyright (c) 2012 QUALCOMM TECHONOLOGIES Incorporated.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $Header:

when        who     what, where, why
--------   ---     ----------------------------------------------------------
12/03/12   cz      Initial version and move drmprov api to a seperate header file

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#define FIRMWARE_PATH_LENGTH      128

/********************************************************************************
  Provision DRM keys using sfs.

  @return
  zero        - Success.
  non-zero    - Failure.

  @param[in]      feature_name        feature name
  @param[in]      feature_name_len    feature name length
  @param[in]      file_name           file name
  @param[in]      file_name_len       file name length
  @param[in]     *data                payload of the key.
  @param[in]      size                Size of the key.

  @dependencies
  None

  @sideeffects
  None
********************************************************************************/
long drm_save_keys
(
      uint8*  feature_name,
      uint32  feature_name_len,
      uint8*  file_name,
      uint32  file_name_len,
      uint8*  data,
      uint32  size
);

/********************************************************************************
  Verify provisioned DRM keys using sfs.

  @return
  zero        - Success.
  non-zero    - Failure.

  @param[in]      feature_name        feature name
  @param[in]      feature_name_len    feature name length
  @param[in]      file_name           file name
  @param[in]      file_name_len       file name length
  @param[in]     *data                payload of the key.
  @param[in]      size                Size of the key.

  @dependencies
  None

  @sideeffects
  None
********************************************************************************/
long drm_verify_keys
(
      uint8*  feature_name,
      uint32  feature_name_len,
      uint8*  file_name,
      uint32  file_name_len,
      uint8*  data,
      uint32  size
);

/********************************************************************************
  Finalized the provisioning keys. After this function is called, provisioning
  will not work anymore.

  @return
  zero        - Success.
  non-zero    - Failure.

  @dependencies
  None

  @sideeffects
  None
********************************************************************************/
int drm_prov_finalize();

/********************************************************************************
  Provision DRM keys using sfs.

  @return
  zero        - Success.
  non-zero    - Failure.

  @param[in]      feature_name        feature name
  @param[in]      feature_name_len    feature name length
  @param[in]      file_name           file name
  @param[in]      file_name_len       file name length
  @param[in]     *data                payload of the key.
  @param[in]      size                Size of the key.
  @param[in]      app_id              defined in \vendor\qcom\proprietary\securemsm\tzcommon\inc\app_main.h
  @param[in]     *app_dir             directory of tz applicaion path, e.g., /firmware/image/playready (No extension name)
  @param[in]      mode                reserved

  @dependencies
  None

  @sideeffects
  None
********************************************************************************/
long drm_app_save_keys
(
      uint8* feature_name,
      uint32 feature_name_len,
      uint8* file_name,
      uint32 file_name_len,
      uint8* data,
      uint32 size,
      uint32 app_id,
      uint8* app_dir,
      uint32 mode
);

/********************************************************************************
  Verify provisioned DRM keys using sfs.

  @return
  zero        - Success.
  non-zero    - Failure.

  @param[in]      feature_name        feature name
  @param[in]      feature_name_len    feature name length
  @param[in]      file_name           file name
  @param[in]      file_name_len       file name length
  @param[in]     *data                payload of the key.
  @param[in]      size                Size of the key.
  @param[in]      app_id              defined in \vendor\qcom\proprietary\securemsm\tzcommon\inc\app_main.h
  @param[in]     *app_dir             directory of tz applicaion path, e.g., /firmware/image/playready (No extension name)
  @param[in]      mode                reserved

  @dependencies
  None

  @sideeffects
  None
********************************************************************************/
long drm_app_verify_keys
(
      uint8* feature_name,
      uint32 feature_name_len,
      uint8* file_name,
      uint32 file_name_len,
      uint8* data,
      uint32 size,
      uint32 app_id,
      uint8* app_dir,
      uint32 mode
);

/********************************************************************************
  Finalized the provisioning keys. After this function is called, provisioning
  will not work anymore.

  @return
  zero        - Success.
  non-zero    - Failure.

  @param[in]      app_id              defined in \vendor\qcom\proprietary\securemsm\tzcommon\inc\app_main.h
  @param[in]     *app_dir             directory of tz applicaion path, e.g., /firmware/image/playready (No extension name)

  @dependencies
  None

  @sideeffects
  None
********************************************************************************/
int drm_app_prov_finalize(
      uint32  app_id,
      uint8*  app_dir
);

#endif /* __DRMPROV_CLIENT_H_ */
