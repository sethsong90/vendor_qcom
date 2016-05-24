/*****************************************************************************
* Copyright (c) 2013 Qualcomm Technologies, Inc.  All Rights Reserved.       *
* Qualcomm Technologies Proprietary and Confidential.                        *
*****************************************************************************/

#ifndef __QMOBICATCOMPOSER_H__
#define __QMOBICATCOMPOSER_H__


/*===========================================================================
 * Class: QMobicatComposer
 *
 * Description: This class represents the mobicat composer utility
 *
 * Notes: none
 *==========================================================================*/
class QMobicatComposer {

public:

  /** QMobicatComposer
   *
   *  Constructor
   **/
  QMobicatComposer();

  /** ~QMobicatComposer
   *
   *  virtual destructor
   **/
  ~QMobicatComposer();

   /** parse_mobicat_info
   *  @metadata: contains metadata info
   *  @metadata_size - size of metadata
   *
   *  Parse metadata into mobicat tags and return string
   *
   **/
  char* ParseMobicatData(uint8_t *metadata);

private:

  /** mScratchBuf
   *
   *  Temp scratch buffer
   **/
  char *mScratchBuf;

  /** mMobicatStr
   *
   *  Parsed Mobicat String
   **/
  char *mMobicatStr;

  /** parseVal
   *
   * @fmt - output format string
   * @aTag - mobicat tag
   * @aVal - value to parse
   *
   * Parse a value of type T
   */
  template <typename T> void parseVal(const char *fmt,
      const char *aTag, T aVal);

  /** parseValArr
   *
   * @fmt - output format string
   * @aTag - mobicat tag
   * @aValPtr - array to parse
   * @aLen - Length of array
   *
   * Parse an array of type T
   */
  template <typename T> void parseValArr(const char *fmt,
      const char *aTag, T *aValPtr, int aLen);

};

#endif //__QMOBICATCOMPOSER_H__
