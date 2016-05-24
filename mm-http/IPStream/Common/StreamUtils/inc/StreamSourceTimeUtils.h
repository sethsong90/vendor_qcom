#ifndef __STREAMSOURCETIMEUTILS_H__
#define __STREAMSOURCETIMEUTILS_H__
/************************************************************************* */
/**
 * StreamSourceTimeUtils.h
 * @brief Header file for StreamSourceTimeUtils.
 *
 COPYRIGHT 2011-2013 Qualcomm Technologies, Inc.
 All rights reserved. Qualcomm Technologies proprietary and confidential.
 *
 ************************************************************************* */
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Source/IPStream/Common/StreamUtils/main/latest/inc/StreamSourceTimeUtils.h#11 $
$DateTime: 2012/09/18 11:58:23 $
$Change: 2815251 $

========================================================================== */
/* =======================================================================
**               Include files for StreamSourceTimeUtils.h
** ======================================================================= */
#include <AEEStdDef.h>
#include <MMTime.h>

/* -----------------------------------------------------------------------
** Constant / Macro Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Data Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Forward Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                        Class & Function Declarations
** ======================================================================= */
class StreamSourceTimeUtils
{
public:
  static int GetDayIndexFromDate(const int year,
                                 const int month,
                                 const int day);
  static bool ConvertFromISO8601ToRFC822(const char* pISO8601timeStrBuf,
                                         char* pRFC822timeStrBuf,
                                         const int nRFC822timeStrBufLen);
  static bool GetDateFromISO8601Format(const char* pISO8601timeStrBuf,
                                       uint32& year,
                                       uint32& month,
                                       uint32& day);
  static bool GetTimeFromISO8601Format(const char* pISO8601timeStrBuf,
                                       uint32& hour,
                                       uint32& min,
                                       uint32& sec,
                                       uint32& msec);

  static double ConvertSysTimeToMSec(const MM_Time_DateTime& sSysTime);

  static bool GetUTCTimeInMsecsFromXMLDateTime(const char* pISO8601timeStrBuf,
                                               double &UTCTime);

  static bool IsLeapYear(const uint32 year);

private:
};

#endif  /* __STREAMSOURCETIMEUTILS_H__ */
