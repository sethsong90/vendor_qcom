/*****************************************************************************
* Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.       *
* Qualcomm Technologies Proprietary and Confidential.                        *
*****************************************************************************/

#ifndef __QEXIF_COMPOSER_PARAMS_H__
#define __QEXIF_COMPOSER_PARAMS_H__

extern "C" {
#include "exif.h"
#include "exif_private.h"
#include <stdlib.h>
}
#include "QEncodeParams.h"
#include "QICommon.h"

/*===========================================================================
 * Class: QExifComposerParams
 *
 * Description: This class represents the exif composer parameters
 *
 * Notes: none
 *==========================================================================*/
class QExifComposerParams
{
public:

  /** QExifComposerParams
   *
   *  constructor
   **/
  QExifComposerParams();

  /** App2HeaderLen
   *
   *  returns app2 marker header length
   **/
  inline uint32_t App2HeaderLen()
  {
    return mApp2Len;
  }

  /** Exif
   *
   *  returns exif tag object
   **/
  inline exif_info_obj_t *Exif()
  {
    return mExifObjInfo;
  }

  /** EncodeParams
   *  @aThumb: flag to indicate the encode parameters belong to
   *         thumbnail or main image
   *
   *  returns encode parameters
   **/
  inline QIEncodeParams& EncodeParams(bool aThumb = false)
  {
    return (aThumb) ? *mThumbEncodeParams : *mMainEncodeParams;
  }

  /** Subsampling
   *  @aThumb: flag to indicate the subsampling belong to
   *         thumbnail or main image
   *
   *  returns image subsampling
   **/
  inline QISubsampling& Subsampling(bool aThumb = false)
  {
    return (aThumb) ? mThumbSS : mMainSS;
  }

  /** SetAppHeaderLen
   *  @aApp2Len: app2 marker header length
   *
   *  sets app2 marker length
   **/
  inline void SetAppHeaderLen(uint32_t aApp2Len)
  {
    mApp2Len = aApp2Len;
  }

  /** SetExif
   *  @aExif: exif object
   *
   *  sets exif info
   **/
  inline void SetExif(exif_info_obj_t *aExif)
  {
    mExifObjInfo = aExif;
  }

  /** SetEncodeParams
   *  @aParams: encode parameters
   *  @aThumb: flag to indicate the subsampling belong to
   *         thumbnail or main image
   *
   *  sets encode parameters
   **/
  void SetEncodeParams(QIEncodeParams &aParams, bool aThumb = false);

  /** SetSubSampling
   *  @aSS: image subsampling
   *  @aThumb: flag to indicate the subsampling belong to
   *         thumbnail or main image
   *
   *  sets encode parameters
   **/
  void SetSubSampling(QISubsampling aSS, bool aThumb = false);

  /** SetMobicat
   *  @aMobicatStr: mobicat str
   *
   *  sets mobicat str
   **/
  inline void SetMobicat(char* aMobicatStr)
  {
    mMobicatStr = aMobicatStr;
  }

  /** MobicatStr
   *
   *  returns mobicat str
   **/
  inline char* GetMobicat()
  {
    return mMobicatStr;
  }

  /** SetMobicatFlag
   *  @aMobicatFlag: flag
   *
   *  enables mobicat to true or false
   **/
  inline void SetMobicatFlag(bool aMobicatFlag)
  {
    mEnableMobicat = aMobicatFlag;
  }

  /** GetMobicatFlag
   *
   *  returns if mobicat is enabled or not
   **/
  inline bool GetMobicatFlag()
  {
    return mEnableMobicat;
  }

private:

  /** mApp2Len
   *
   *  App2 header length
   **/
  uint32_t mApp2Len;

  /** mExifObjInfo
   *
   *  exif object info
   **/
  exif_info_obj_t *mExifObjInfo;

  /** mMainEncodeParams
   *
   *  main encode parameters
   **/
  QIEncodeParams *mMainEncodeParams;

  /** mThumbEncodeParams
   *
   *  thumbnail encode parameters
   **/
  QIEncodeParams *mThumbEncodeParams;

  /** mMainSS
   *
   *  main subsampling
   **/
  QISubsampling mMainSS;

  /** mMainSS
   *
   *  thumbnail subsampling
   **/
  QISubsampling mThumbSS;

  /** mMobicatStr
   *
   *  parsed Mobicat data
   **/
  char* mMobicatStr;

  /** mEnableMobicat
   *
   *  mobicat enabled flag
   **/
  bool mEnableMobicat;

};

#endif //__QEXIF_COMPOSER_PARAMS_H__

