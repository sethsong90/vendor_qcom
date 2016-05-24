/* =======================================================================
aacfile.cpp
DESCRIPTION
Meaningful description of the definitions contained in this file.
Description must specify if the module is portable specific, mobile
specific, or common to both, and it should alert the reader if the
module contains any conditional definitions which tailors the module to
different targets.  Include any initialization and synchronizing
requirements.

EXTERNALIZED FUNCTIONS
List functions and a brief description that are exported from this file

INITIALIZATION AND SEQUENCING REQUIREMENTS
Detail how to initialize and use this service.  The sequencing aspect
is only needed if the order of operations is important.

Copyright 2009-2013 QUALCOMM Technologies Incorporated, All Rights Reserved.
QUALCOMM Technologies Proprietary and Confidential.

========================================================================== */

/* =======================================================================
                             PERFORCE HEADER
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/AACParserLib/main/latest/src/aacfile.cpp#59 $
$DateTime: 2013/07/07 20:32:51 $
$Change: 4053609 $
========================================================================== */

/* ==========================================================================

INCLUDE FILES FOR MODULE

========================================================================== */
#include "parserinternaldefs.h"
#include "parserdatadef.h"
#include "MMDebugMsg.h"
#include <stdio.h>
#include "aacfile.h"
#include "aacparser.h"
#include "filebase.h"
#include "MMMemory.h"
#include "MMTimer.h"

#define INIT_UNDERRUN_POLL_INTERVAL 500
#define MAX_LOOP_LIMIT 100

#ifdef FEATURE_FILESOURCE_DRM_DCF
#include "IxStream.h"
#endif

#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
#define QTV_3GPP_MIN_NUM_VIDEO_FRAMES_TO_BUFFER 6
#endif //FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD

/* ==========================================================================

DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================== */

/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */


/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Local Object Definitions
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Forward Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                            Function Definitions
** ======================================================================= */
/* ======================================================================== */
/* <EJECT> */
/*===========================================================================*/


/*===========================================================================

FUNCTION
  AACCallbakGetData

DESCRIPTION

DEPENDENCIES
  None

INPUT PARAMETERS:
->
->

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/


uint32 AACCallbakGetData (uint64 nOffset, uint32 nNumBytesRequest,
                          uint8 *pData, uint32 nMaxSize,
                          uint32 u32UserData, bool &bendofdata)
{
  if(u32UserData)
  {
    AACFile *pAACFile = (AACFile *)u32UserData;
    return( pAACFile->FileGetData(nOffset, nNumBytesRequest, nMaxSize, pData,
                                  bendofdata));
  }
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,
              "AACCallbakGetData u32UserData is NULL");
  return 0;
}

/* ======================================================================
FUNCTION:
  AACFile::FileGetData

DESCRIPTION:
  To read the data from the file
INPUT/OUTPUT PARAMETERS:
  nOffset         : Offset from which data is being requested
  nNumBytesRequest: Total number of bytes requested
  ppData          : Buffer to be used for reading the data

RETURN VALUE:
 Number of bytes read

SIDE EFFECTS:
  None.
======================================================================*/
uint32 AACFile::FileGetData(  uint64 nOffset,
                              uint32 nNumBytesRequest,
                              uint32 nMaxSize,
                              uint8 *pData,
                              bool &bendofdata)
{
  uint32 nRead = 0;
  if (m_AACFilePtr != NULL)
  {
#if defined(FEATURE_FILESOURCE_PSEUDO_STREAM) || defined(FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD)
      if((m_bHttpStreaming) && (m_pPort))
      {
        /* If user did not abort the playback*/
        if(!m_bMediaAbort)
        {
          int64 navailoffset = 0;
          if(m_pPort->GetAvailableOffset(&navailoffset,&bendofdata) ==
             video::iSourcePort::DS_SUCCESS)
          {

            /* This is a special scenario, to know whether complete file is
               downloaded or not. When parser requests 10bytes of data from end
               of the file, then validate EOD flag. If flag is set to false,
               then report data under-run. This case is to handle the problem
               in LA framework, where they always return fileSize as available
               data even though complete file is not downloaded. */
            if((uint64)nOffset == m_fileSize - 10 && FALSE == bendofdata)
            {
              navailoffset = nOffset - 10;
            }

            /* If offset required is greater than the available offset, then
               return underrun event. bendofdata flag will be used to determine
               whether it is data under-run or not. aacParser Object will use
               this flag to return either EOF or Under-run, if requested data
               is not provided */
            if(int64(nOffset) >= navailoffset)
            {
              MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
                          "m_bHandleSeekUnderRun, data not available nOffset \
                         %llu navailoffset %lld nNumBytesRequest %lu",
                           nOffset,navailoffset,nNumBytesRequest);
            }
            else
            {
              nRead = FileBase::readFile(m_AACFilePtr, pData, nOffset,
                                   FILESOURCE_MIN(nNumBytesRequest, nMaxSize));
            }
          }
          else
          {
            MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,
                        "GetAvailableOffset failed...");
          }
        }//if(!m_bMediaAbort)
        else
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
                      "Breaking, user Abort is true.");
          bendofdata = true;
        }
      }//if((bHttpStreaming) && (m_bHandleSeekUnderRun) && m_pPort)
      else /* In case of local playback always set bendofdata flag to true. */
#endif //#if defined(FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD)
      {
        bendofdata = true;
        if(!m_bMediaAbort)
        {
          nRead = FileBase::readFile(m_AACFilePtr, pData, nOffset,
                                    FILESOURCE_MIN(nNumBytesRequest,nMaxSize));
        }
        else
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
                      "Breaking, user Abort is true.");
        }
      }
  }
  return nRead;
}

/*===========================================================================

FUNCTION
  AACFile::ParseAACHeader

DESCRIPTION
  creates instnace of AACparser and calls start on parsing AAC file

DEPENDENCIES
  None

INPUT PARAMETERS:
->
->

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/

AACFile:: AACFile(const FILESOURCE_STRING &filename,
                  unsigned char *pFileBuf, uint64 bufSize)
{
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "AACFile:: AACFile");
#endif
  InitData();
  if (pFileBuf && bufSize)
  {
    m_pFileBuf = pFileBuf;
    m_FileBufSize = bufSize;
    m_AACFilePtr = OSCL_FileOpen (pFileBuf, bufSize);
  }
  else
  {
     m_filename = filename;
//#ifdef FEATURE_FILESOURCE_FILE_IO_CACHING
#if 0 //Disabling as the buffer siz has to be decided for caching
     /* Calling with 10K cache  buffer size */
     m_AACFilePtr = OSCL_FileOpen(filename, (OSCL_TCHAR *) _T("rb"),
                                  FILE_READ_BUFFER_SIZE_FOR_AAC );
#else
     m_AACFilePtr = OSCL_FileOpen(filename, (OSCL_TCHAR *) _T("rb"),
                                  AAC_CACHE_SIZE);
#endif
     m_fileSize = OSCL_FileSize( m_filename );

  }
  if(m_AACFilePtr != NULL)
  {
    if(ParseAACHeader()== PARSER_ErrorNone)
    {
      _fileErrorCode = PARSER_ErrorNone;
      _success = true;
    }
  }
}

AACFile::AACFile()
{
  InitData();
}

AACFile::AACFile(IxStream* pixstream)
{
  InitData();
#ifdef FEATURE_FILESOURCE_DRM_DCF
  m_pIxStream = pixstream;
  m_AACFilePtr = OSCL_FileOpen(m_pIxStream);
  if(m_AACFilePtr != NULL)
  {
    if(m_pIxStream)
    {
      (void)m_pIxStream->Size(&m_fileSize);
    }
    if(ParseAACHeader() == PARSER_ErrorNone )
    {
      _fileErrorCode = PARSER_ErrorNone;
      _success = true;
    }
  }
#endif
}
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
AACFile::AACFile(video::iStreamPort* pport)
{
  InitData();
  m_pPort = pport;
  m_bHttpStreaming = true;
  m_AACFilePtr = OSCL_FileOpen(pport);
  if(m_AACFilePtr != NULL)
  {
    if(m_pPort)
    {
      int64 noffset = 0;
      video::iStreamPort::DataSourceType eSourceType =
                      video::iStreamPort::DS_FILE_SOURCE;
      m_fileSize = MAX_FILE_SIZE;
      if(video::iStreamPort::DS_SUCCESS == m_pPort->GetContentLength(&noffset))
      {
        m_fileSize = (uint64)noffset;
      }
      m_pPort->GetSourceType(&eSourceType);
      if (video::iStreamPort::DS_STREAMING_SOURCE != eSourceType)
      {
        m_bHttpStreaming = false;
      }
    }
    if(ParseAACHeader() == PARSER_ErrorNone )
    {
      _fileErrorCode = PARSER_ErrorNone;
      _success = true;
    }
  }
}

bool AACFile::parseHTTPStream ()
{
  if(m_paacParser)
  {
    if(_fileErrorCode == PARSER_ErrorNone)
      return true;
  }
  return false;
}
#endif

/*
* Initialize the class members to the default values.
*/
void AACFile::InitData()
{
  memset(&m_audsampleinfo,0,sizeof(m_audsampleinfo));
  m_SEEK_DONE = false;
  m_uSeektime = 0;
  _fileErrorCode = PARSER_ErrorDefault;
  _success = false;
  m_bMediaAbort = false;
  m_bMetaDatainUTF8 = false;
  m_pIxStream = NULL;
  m_filename = NULL;
  m_AACFilePtr = NULL;
  m_bHttpStreaming = false;
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
  m_pPort = NULL;
#endif
  m_fileSize = 0;
  m_pFileBuf = NULL;
  m_FileBufSize = 0;
  m_bStreaming = FALSE;
  m_paacParser = NULL;
}
AACFile::~AACFile()
{
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "AACFile::~AACFile");
#endif
  if(m_AACFilePtr!=NULL)
  {
     OSCL_FileClose(m_AACFilePtr);
     m_AACFilePtr = NULL;
  }

  if(m_paacParser)
  {
     MM_Delete( m_paacParser);
     m_paacParser = NULL;
  }


}
void AACFile::SetCriticalSection(MM_HANDLE pcriticalsection)
{
  if(m_AACFilePtr)
  {
    m_AACFilePtr->pCriticalSection = pcriticalsection;
  }
}
/*===========================================================================

FUNCTION
  AACFile::ParseAACHeader

DESCRIPTION
  creates instnace of AACparser and calls start on parsing AAC file

DEPENDENCIES
  None

INPUT PARAMETERS:
->
->

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/

PARSER_ERRORTYPE AACFile::ParseAACHeader()
{
  PARSER_ERRORTYPE retError = PARSER_ErrorDefault;
  uint32 loopCount = 0;
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "ParseAACHeader");
#endif

  m_paacParser = MM_New_Args(aacParser,((uint32)this,m_fileSize,m_AACFilePtr,
                                        m_bHttpStreaming));

  if(m_paacParser)
  {
    do
    {
      retError = m_paacParser->StartParsing();
      if( retError!= PARSER_ErrorNone)
      {
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_FATAL,
                    "AACFile::ParseAACHeadera failed..retError %d",retError);
#endif
        if(retError == PARSER_ErrorDataUnderRun)
        {
          MM_Timer_Sleep(INIT_UNDERRUN_POLL_INTERVAL);
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
          MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
            "Parser init is not completed due to data unavailable,\
            loopCount= %lu", loopCount);
#endif
          loopCount++;
        }
        _success = false;
      }
    } while(retError == PARSER_ErrorDataUnderRun && loopCount <= MAX_LOOP_LIMIT);
  }
  return (retError);
}

/* ======================================================================
FUNCTION:
  AACFile::getNextMediaSample

DESCRIPTION:
  gets next sample of the given track.

INPUT/OUTPUT PARAMETERS:
@param[in] ulTrackID  TrackID requested
@param[in] pucDataBuf DataBuffer pointer to fill the frame(s)
@param[in/out]
           pulBufSize Amount of data request /
                      Amount of data filled in Buffer
@param[in] rulIndex   Index

RETURN VALUE:
 PARSER_ErrorNone in Successful case /
 Corresponding error code in failure cases

SIDE EFFECTS:
  None.
======================================================================*/

PARSER_ERRORTYPE AACFile::getNextMediaSample(uint32 ulTrackID, uint8 *pucDataBuf,
                                             uint32 *pulBufSize, uint32 &rulIndex)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "getNextMediaSample");
  uint32 nOutDataSize = 0;
  PARSER_ERRORTYPE retStatus = PARSER_ErrorDefault;
  aac_decode_info AACDecodeinfo;

  /* Validate input params and class variables */
  if(NULL == m_paacParser || NULL == pulBufSize || NULL == pucDataBuf ||
     0 == *pulBufSize)
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
               "getNextMediaSample invalid input params!!");
    return PARSER_ErrorInvalidParam;
  }

  nOutDataSize = *pulBufSize;
  retStatus = m_paacParser->GetCurrentSample(pucDataBuf, *pulBufSize,&nOutDataSize);

  if(m_SEEK_DONE == true || m_paacParser->m_isFirstTSValid == true)
  {
    m_audsampleinfo.btimevalid = true;
  }
  else
  {
    m_audsampleinfo.btimevalid = false;
  }
  if(((PARSER_ErrorNone == retStatus) ||
      (PARSER_ErrorEndOfFile == retStatus)) &&
     (PARSER_ErrorNone == m_paacParser->GetAACDecodeInfo(&AACDecodeinfo)))
  {
    if(AACDecodeinfo.aac_subformat_type == AAC_FORMAT_ADTS)
    {
      //check if parser is configured to putput one adts frame
      bool bframe = false;
      (void)m_paacParser->GetAudioOutputMode
           (&bframe, FILE_SOURCE_MEDIA_OUTPUT_SINGLE_AUDIO_FRAME);
      if(bframe)
      {
        //Time stamp information would be available..
        if(m_audsampleinfo.delta == 0)
        {
          //set the delta only once
          m_audsampleinfo.delta = m_paacParser->GetCurrentTime() -
                                 m_audsampleinfo.time;
        }
        m_audsampleinfo.time = m_paacParser->GetCurrentTime();
        m_audsampleinfo.btimevalid = true;
      }
    }//if(AACDecodeinfo.aac_subformat_type == AAC_FORMAT_ADTS)
  }//if(PARSER_ErrorNone == m_paacParser->GetAACDecodeInfo(&AACDecodeinfo))
  m_SEEK_DONE = false;

  *pulBufSize = nOutDataSize;
  return retStatus;
}
/* ======================================================================
FUNCTION:
  AACFile::getMediaTimestampForCurrentSample

DESCRIPTION:
  gets time stamp of current sample of the track.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 time stamp in track time scale unit

SIDE EFFECTS:
  None.
======================================================================*/
uint64 AACFile::getMediaTimestampForCurrentSample(uint32 id)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "getMediaTimestampForCurrentSample");
  uint64 nTimeStamp = 0;
  if(m_paacParser && m_paacParser->m_isFirstTSValid == true && m_paacParser->m_firstFrame)
  {
    nTimeStamp = m_paacParser->getMediaTimestampForCurrentSample(id);
  }
  else
  {
    nTimeStamp = m_audsampleinfo.time;
  }
  return nTimeStamp;
}
/* ======================================================================
FUNCTION:
  AACFile::GetLastRetrievedSampleOffset

DESCRIPTION:
  Returns the offset of the last retrieved sample

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint64 AACFile::GetLastRetrievedSampleOffset(uint32 trackid)
{
  uint64 offset = 0;
  if(m_paacParser)
  {
    offset = m_paacParser->m_nCurrOffset;
  }
  MM_MSG_PRIO1(MM_GENERAL, MM_PRIO_HIGH, "GetLastRetrievedSampleOffset %llu",offset);
  return offset;
}

/* ======================================================================
FUNCTION:
  AACFile::getBaseTime

DESCRIPTION:
  gets base time of the clip.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
bool AACFile::getBaseTime(uint32 id, uint64* nBaseTime)
{
  bool nRet = false;
  *nBaseTime = 0;
  if(m_paacParser)
  {
    nRet = m_paacParser->GetBaseTime(nBaseTime);
  }
  return nRet;
}
/* ======================================================================
FUNCTION:
  AACFile::setBaseTime

DESCRIPTION:
  sets base time of the clip.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
bool AACFile::setBaseTime(uint32 id,uint64 nBaseTime)
{
  bool bRet = false;
  if(m_paacParser && nBaseTime)
  {
    bRet = m_paacParser->SetBaseTime(nBaseTime);
  }
  return bRet;
}

/* ======================================================================
FUNCTION:
  AACFile::resetPlayback

DESCRIPTION:
  resets the playback time to given time(pos) for a track.
  Also tells if we need to goto closest sync sample or not.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/

uint64 AACFile::resetPlayback(  uint64 repos_time, uint32 id, bool bSetToSyncSample,
                                bool *bError, uint64 currentPosTimeStamp)
{
  PARSER_ERRORTYPE status = PARSER_ErrorNone;
  uint64 nSeekedTime = 0;

  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "resetPlayback");
  if(m_paacParser)
  {
    *bError = true;
    status = m_paacParser->Seek(repos_time, &nSeekedTime);
    if(PARSER_ErrorNone == status || PARSER_ErrorDataUnderRun == status)
    {
      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH, "repos_time passed in %llu returned time nSeekedTime %llu",
                   repos_time,nSeekedTime);
      if(PARSER_ErrorDataUnderRun == status)
      {
        _fileErrorCode = PARSER_ErrorDataUnderRun;
        *bError = true;
      }
      else
      {
        m_uSeektime = nSeekedTime;
        *bError = false;
        if(!m_uSeektime)
        {
          m_paacParser->init_file_position();
        }
        m_audsampleinfo.time = m_uSeektime;
        m_SEEK_DONE = true;
        _fileErrorCode = PARSER_ErrorNone;
      }
    }
    else
    {
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_ERROR, "Reposition failed for track id = %lu", id);
      // this is to avoid unnecessary disturbance, when repositioning can not be done
      *bError = true;
      _fileErrorCode = PARSER_ErrorSeekFail;
    }

  }
  return m_uSeektime;
}

/* ======================================================================
FUNCTION:
  AACFile::GetClipMetaData

DESCRIPTION:
  Provides different metadata fields info in the o/p buffer

INPUT/OUTPUT PARAMETERS:
  pucDataBuf and pulDatabufLen.

RETURN VALUE:
 PARSER_ErrorNone if Success
 Error status, if failure

SIDE EFFECTS:
  None.
======================================================================*/
PARSER_ERRORTYPE AACFile::GetClipMetaData(wchar_t *pucDataBuf,
                                          uint32 *pulDatabufLen,
                                          FileSourceMetaDataType ienumData)
{
  if(pulDatabufLen == NULL || m_paacParser == NULL)
    return PARSER_ErrorInvalidParam;

  metadata_id3v1_type* id3v1_info =
    (metadata_id3v1_type*)m_paacParser->get_id3v1_info();
  metadata_id3v2_type* id3v2_info =
    (metadata_id3v2_type*)m_paacParser->get_id3v2_info();
  text_frame *textFrame = NULL;
  encoder_delay_tag_info *pEncDelay = NULL;
  uint8* id3v1Str = NULL;
  uint32 id3v1StrLen = 0;
  m_bMetaDatainUTF8 = false;

  /* These delay parameters will be stored in hex string format in ID3
     metadata.
     At FileSource, these parameters will be converted back to normal decimal
     values. */
  if(((FILE_SOURCE_MD_ENC_DELAY == ienumData) ||
      (FILE_SOURCE_MD_PADDING_DELAY == ienumData)) &&
       (pucDataBuf))
  {
    if(FILE_SOURCE_MD_ENC_DELAY == ienumData)
    {
      if (id3v2_info)
      {
        pEncDelay = &id3v2_info->encoder_delay_tag;
      }
      if (pEncDelay)
      {
        memcpy(pucDataBuf, pEncDelay->ucEncoderDelay, sizeof(uint64));
      }
    }
    else if(FILE_SOURCE_MD_PADDING_DELAY == ienumData)
    {
      if (id3v2_info)
      {
        pEncDelay = &id3v2_info->encoder_delay_tag;
      }
      if (pEncDelay)
      {
        memcpy(pucDataBuf, pEncDelay->ucPaddingDelay, sizeof(uint64));
      }
    }
    *pulDatabufLen = sizeof(uint64);
    return PARSER_ErrorNone;
  }

  if(id3v2_info)
  {
    switch (ienumData)
    {
      case FILE_SOURCE_MD_ALBUM:
        textFrame   = &id3v2_info->album;
      break;
      case FILE_SOURCE_MD_ARTIST:
        textFrame = &id3v2_info->album_artist;
      break;
      case FILE_SOURCE_MD_REC_YEAR:
        textFrame = &id3v2_info->year;
      break;
      case FILE_SOURCE_MD_TITLE:
        textFrame = &id3v2_info->title;
      break;
      case FILE_SOURCE_MD_TRACK_NUM:
        textFrame = &id3v2_info->track;
      break;
      case FILE_SOURCE_MD_GENRE:
        textFrame = &id3v2_info->genre;
      break;
      case FILE_SOURCE_MD_AUTHOR:
        textFrame = &id3v2_info->publisher;
      break;
      case FILE_SOURCE_MD_COMPOSER:
        textFrame = &id3v2_info->composer;
      break;
      default:
        break;
    }
  }

  if(textFrame && textFrame->text)
  {
    if (pucDataBuf)
    {
      if((*pulDatabufLen < (textFrame->textLen * sizeof(wchar_t)) / 2 &&
          01 == textFrame->encoding) ||
         (*pulDatabufLen < textFrame->textLen * sizeof(wchar_t) &&
          00 == textFrame->encoding))
      {
        return PARSER_ErrorInsufficientBufSize;
      }
      if(00 == textFrame->encoding)
      {
        CharToWideChar(textFrame->text, textFrame->textLen,
                       pucDataBuf, *pulDatabufLen, FALSE);
      }
      else
      {
        /* If data is already in UTF8 codec format, Parser will copy data
           directly into output buffer and sets the UTF8 flag. Based on this
           flag, Client will not do any extra processing on the output data.
           In normal circumstances, client will convert the data into UTF8
           format, before processing. */
        if(ID3V2_FRAME_TEXT_ENCODING_UTF_8 == textFrame->encoding)
        {
          memset(pucDataBuf, 0, *pulDatabufLen);
          memcpy(pucDataBuf, textFrame->text, textFrame->textLen);
          m_bMetaDatainUTF8 = true;
        }
        else if(2 == sizeof(wchar_t))
        {
          memcpy(pucDataBuf, textFrame->text, textFrame->textLen);
        }
        else
        {
          CharToWideChar(textFrame->text, textFrame->textLen,
                         pucDataBuf, *pulDatabufLen, TRUE);
        }
      }
    }
    /* If encoding type used is UniCode (which is equivalent to 16bits). */
    if(01 == textFrame->encoding)
      *pulDatabufLen = (textFrame->textLen * sizeof(wchar_t)) / 2;
    else
      *pulDatabufLen = textFrame->textLen * sizeof(wchar_t);
  }//if(id3v2_info && ..
  else if(id3v1_info)
  {
    switch (ienumData)
    {
      case FILE_SOURCE_MD_ALBUM:
        id3v1StrLen = MAX_ID3V1_TEXT_SIZE;
        id3v1Str    = id3v1_info->album;
      break;
      case FILE_SOURCE_MD_ARTIST:
        id3v1StrLen = MAX_ID3V1_TEXT_SIZE;
        id3v1Str    = id3v1_info->artist;
      break;
      case FILE_SOURCE_MD_TITLE:
        id3v1StrLen = MAX_ID3V1_TEXT_SIZE;
        id3v1Str    = id3v1_info->title;
      break;
      case FILE_SOURCE_MD_REC_YEAR:
        id3v1StrLen = MAX_ID3V1_YEAR_TEXT_SIZE;
        id3v1Str    = id3v1_info->year;
      break;
      case FILE_SOURCE_MD_TRACK_NUM:
      {
        char tempString[4];
        memset(tempString, 0, 4);
#ifdef _ANDROID_
        snprintf(tempString, 4, (char*)"%d", (int)id3v1_info->track);
#else
        std_strlprintf(tempString, 4, (char*)"%d", (int)id3v1_info->track);
#endif
        id3v1StrLen = 4;
        id3v1Str    = (uint8 *)tempString;
      }
      break;
      default:
      break;
    }
    if (pucDataBuf && id3v1Str)
    {
      if(*pulDatabufLen < id3v1StrLen * sizeof(wchar_t))
      {
        return PARSER_ErrorInsufficientBufSize;
      }

      //Flag at the end indicate whether input string is in UTF8 or YTF16
      //foratm. 'FALSE' means UTF8, 'TRUE' means UTF16
      CharToWideChar((char*)id3v1Str, id3v1StrLen, pucDataBuf,
                     *pulDatabufLen, FALSE);
    }

    // Update output buf length value
    *pulDatabufLen = id3v1StrLen * sizeof(wchar_t);
  }//if(id3v1_info)
  else
  {
    *pulDatabufLen = 0;
  }

  return PARSER_ErrorNone;
}

/* ======================================================================
FUNCTION:
  AACFile::getAlbumArt

DESCRIPTION:
  Provides picture data (album art) in the o/p buffer

INPUT/OUTPUT PARAMETERS:
  pucDataBuf and pulDatabufLen.

RETURN VALUE:
 PARSER_ErrorNone if Success
 Error status, if failure

SIDE EFFECTS:
  None.
======================================================================*/
PARSER_ERRORTYPE AACFile::getAlbumArt(wchar_t *pucDataBuf, uint32 *pulDatabufLen)
{
  if(pulDatabufLen == NULL || m_paacParser == NULL)
  {
    return PARSER_ErrorInvalidParam;
  }

  //no APIC tag in id3v1, so check for version 2 and above
  if(m_paacParser->get_id3v2_info())
  {
    metadata_id3v2_type* info =
      (metadata_id3v2_type*)m_paacParser->get_id3v2_info();
    if(info->pic_info.pic_data)
    {
      if (pucDataBuf)
      {
        FS_ALBUM_ART_METADATA *temp = (FS_ALBUM_ART_METADATA*)pucDataBuf;
        if(*pulDatabufLen < info->pic_info.pic_data_len +
                            FIXED_APIC_FIELDS_LENGTH)
        {
          return PARSER_ErrorInsufficientBufSize;
        }
        memset(pucDataBuf, 0, FIXED_APIC_FIELDS_LENGTH);
        temp->ucTextEncodeType = info->pic_info.text_enc;
        temp->imgFormat        = info->pic_info.img_format;
        temp->picType          = info->pic_info.pict_type;
        temp->ulPicDataLen     = info->pic_info.pic_data_len;

        memcpy(temp->ucDesc,info->pic_info.desc,MAX_DESC_LEN);
        memcpy(temp->pucPicData,info->pic_info.pic_data,
               info->pic_info.pic_data_len);
        memcpy(temp->ucImgFormatStr,info->pic_info.img_format_string,
               MAX_IMG_FORMAT_LEN);
      }
      *pulDatabufLen = info->pic_info.pic_data_len + FIXED_APIC_FIELDS_LENGTH;
    }//if(info && ..
    else
    {
      *pulDatabufLen = 0;
    }
  }

  return PARSER_ErrorNone;
}

/* ======================================================================
FUNCTION:
  AACFile::getMovieDuration

DESCRIPTION:
  gets movie duration in movie timescale unit.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint64 AACFile::getMovieDuration() const
{
  uint64 nDuration = 0;
  if(m_paacParser)
  {
    nDuration = m_paacParser->GetClipDurationInMsec();
  }
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH, "getMovieDuration %llu",nDuration);
  return nDuration;
}
/* ======================================================================
FUNCTION:
  AACFile::randomAccessDenied

DESCRIPTION:
  Returns non zero is seek is not supported else returns 0(seek is supported)
INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint8 AACFile::randomAccessDenied()
{
  uint8 nSeekDenied = 1;
  if(m_paacParser)
  {
    nSeekDenied = m_paacParser->RandomAccessDenied();
  }
  return nSeekDenied;
}
/* ======================================================================
FUNCTION:
  AACFile::getMovieTimescale

DESCRIPTION:
  gets movie timescale

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 AACFile::getMovieTimescale() const
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "getMovieTimescale");
  return AAC_STREAM_TIME_SCALE;
}
/* ======================================================================
FUNCTION:
  AACFile::getTrackWholeIDList

DESCRIPTION:
  gets list of track IDs

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 AACFile::getTrackWholeIDList(uint32 *ids)
{
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "getTrackWholeIDList");
#endif
  int32 nTracks = 0;
  if((!m_paacParser)||(!ids))
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "getTrackWholeIDList !m_paacParser|| !ids is TRUE..reporting 0 tracks");
  }
  else
  {
    nTracks = AUDIO_AAC_MAX_TRACKS;
    *ids = nTracks;
  }
  return nTracks;
}
/* ======================================================================
FUNCTION:
  AACFile::getTrackMediaDuration

DESCRIPTION:
  gets track duration in track time scale unit

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint64 AACFile::getTrackMediaDuration(uint32 id)
{
  uint64 nTrackDuration = 0;
  nTrackDuration = getMovieDuration();
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH, "getTrackMediaDuration %llu",nTrackDuration);
  return nTrackDuration;
}
/* ======================================================================
FUNCTION:
  AACFile::getTrackMediaTimescale

DESCRIPTION:
  gets track time scale

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 AACFile::getTrackMediaTimescale(uint32 id)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "getTrackMediaTimescale");
  return AAC_STREAM_TIME_SCALE;
}
/* ======================================================================
FUNCTION:
  AACFile::getTrackAudioSamplingFreq

DESCRIPTION:
  gets audio track's sampling frequency

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 AACFile::getTrackAudioSamplingFreq(uint32 id)
{
  uint32 nSamplingFreq = 0;
  if(m_paacParser)
  {
    aac_header_aach saac_header_aach;
    if(m_paacParser->GetAACHeader(&saac_header_aach)==PARSER_ErrorNone)
    {
      nSamplingFreq = saac_header_aach.nSampleRate;
    }
  }
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH, "getTrackAudioSamplingFreq %lu",nSamplingFreq);
 return nSamplingFreq;
}

/* ======================================================================
FUNCTION:
  AACFile::getTrackAverageBitrate

DESCRIPTION:
  gets audio track's bit rate

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
int32 AACFile::getTrackAverageBitrate(uint32 id)
{
  int32 nBitRate = 0;
  if(m_paacParser)
  {
    aac_header_aach saac_header_aach;
    if(m_paacParser->GetAACHeader(&saac_header_aach)==PARSER_ErrorNone)
    {
      nBitRate = (int32)saac_header_aach.nBitRate;
    }
  }
 return nBitRate;
}

/* ======================================================================
FUNCTION:
  AACFile::getTrackOTIType

DESCRIPTION:
  returns Object Type Identifier corresponding to AAC format
  (ADIF/ADTS/LOAS/RAW)

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 OTIType

SIDE EFFECTS:
  None.
======================================================================*/

uint8  AACFile::getTrackOTIType(uint32 /*id*/)
{
  uint8 format = 0xff;
  if (m_paacParser)
  {
    aac_decode_info AACDecodeinfo;
    m_paacParser->GetAACDecodeInfo(&AACDecodeinfo);

    switch (AACDecodeinfo.aac_subformat_type)
    {
      case AAC_FORMAT_ADTS:
           format = AAC_ADTS_AUDIO;
           break;

      case AAC_FORMAT_ADIF:
           format = AAC_ADIF_AUDIO;
           break;

      case AAC_FORMAT_RAW:
           format = MPEG4_AUDIO;
           break;

      case AAC_FORMAT_LOAS:
           format = AAC_LOAS_AUDIO;
           break;

      default:
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
          "AACFile::getTrackOTIType unknown AAC format");

    }
  }
  return format;
}

/* ======================================================================
FUNCTION:
  AACFile::GetNumAudioChannels

DESCRIPTION:
  returns number of audio channels

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 AACFile::GetNumAudioChannels(int id)
{
  unsigned long nChannels = 0;
  if(m_paacParser)
  {
    aac_header_aach saac_header_aach;
    if(m_paacParser->GetAACHeader(&saac_header_aach)==PARSER_ErrorNone)
    {
      nChannels = saac_header_aach.nChannels;
    }
  }
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH, "GetNumAudioChannels %lu",nChannels);
  return nChannels;
}

/* ======================================================================
FUNCTION:
  AACFile::GetAACAudioProfile

DESCRIPTION:
  returns AAC Audio profile

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 AACFile::GetAACAudioProfile(uint32 id)
{
  uint32 ulAACAudioProfile = 0;
  if(m_paacParser)
  {
    aac_decode_info saac_decode_info;
    if(m_paacParser->GetAACDecodeInfo(&saac_decode_info)==PARSER_ErrorNone)
    {
      ulAACAudioProfile = saac_decode_info.audio_object;
    }
  }
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_MEDIUM, "GetAACAudioProfile %lu",
               ulAACAudioProfile);
  return ulAACAudioProfile;
}
/* ======================================================================
FUNCTION:
  AACFile::GetAACAudioFormat

DESCRIPTION:
  returns AAC Audio format

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 AACFile::GetAACAudioFormat(uint32 id)
{
  unsigned long AACAudioFormat = 0;
  if(m_paacParser)
  {
    aac_decode_info saac_decode_info;
    if(m_paacParser->GetAACDecodeInfo(&saac_decode_info)==PARSER_ErrorNone)
    {
      switch(saac_decode_info.aac_subformat_type)
      {
        case 0:
          AACAudioFormat = AAC_FORMAT_UNKNOWN;
        break;

        case 1:
          AACAudioFormat = AAC_FORMAT_ADTS;
        break;

        case 2:
          AACAudioFormat = AAC_FORMAT_ADIF;
        break;

        case 3:
          AACAudioFormat = AAC_FORMAT_RAW;
        break;

        case 4:
          AACAudioFormat = AAC_FORMAT_LOAS;
        break;

        default:
          AACAudioFormat = AAC_FORMAT_UNKNOWN;
        break;
      }
    }
  }
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH, "GetAACAudioProfile %lu",AACAudioFormat);
  return AACAudioFormat;
}
//===========================================================================
//
//FUNCTION
//  AACFile::CheckAacFormat
//
//DESCRIPTION
//  check the given file is aac or not
//
//DEPENDENCIES
//  None
//
//INPUT PARAMETERS:
//->
//->
//
//RETURN VALUE
//  None
//
//SIDE EFFECTS
// None
//===========================================================================*/
bool AACFile::CheckAacFormat()
{
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "CheckAacFormat");
#endif
  bool result = false;
  if(m_paacParser)
  {
    result = m_paacParser->is_aac_format();
  }
  return  result;
}

/* ======================================================================
FUNCTION:
 AACFile::getTrackMaxBufferSizeDB

DESCRIPTION:
  gets maximum buffer size to play the track

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
int32 AACFile::getTrackMaxBufferSizeDB(uint32 id)
{
  int32 bufferSize = AAC_DEFAULT_AUDIO_BUF_SIZE;
  if(!m_paacParser)
  {
    bufferSize = 0;
  }
  else
  {
   aac_audio_info audioInfo;
   if((m_paacParser->GetAudioInfo(&audioInfo))== PARSER_ErrorNone)
   {
     bufferSize = audioInfo.dwSuggestedBufferSize;
   }
  }
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH, "getTrackMaxBufferSizeDB %ld",bufferSize);
  return bufferSize;
}

/* ======================================================================
FUNCTION:
  AACFile::getTrackDecoderSpecificInfoContent

DESCRIPTION:
  gets information about current sample of a track

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
PARSER_ERRORTYPE AACFile::getTrackDecoderSpecificInfoContent(uint32 id, uint8* buf,
                                                             uint32 *pbufSize)
{
  PARSER_ERRORTYPE retvalue = PARSER_ErrorDefault;
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "getTrackDecoderSpecificInfoContent");
  if(m_paacParser && pbufSize)
  {
    if(PARSER_ErrorNone == m_paacParser->GetTrackDecoderSpecificInfoContent(buf,(uint8*)pbufSize) )
    {
      retvalue = PARSER_ErrorNone;
    }
  }
  return retvalue;
}

/* ======================================================================
FUNCTION:
  AACFile::peekCurSample

DESCRIPTION:
  gets information about current sample of a track

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
PARSER_ERRORTYPE AACFile::peekCurSample(uint32 trackid,
                                        file_sample_info_type *pSampleInfo)
{
  /*Return type of this function shoule be changed to MP4_ERROR_CODE*/
  PARSER_ERRORTYPE retval = PARSER_ErrorNone;
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "peekCurSample");
  if( (!m_paacParser)||(!pSampleInfo) )
  {
    retval = PARSER_ErrorDefault;
  }
  else
  {
     *pSampleInfo = m_audsampleinfo;
     retval = PARSER_ErrorNone;
  }
  return retval;
}

/* ======================================================================
FUNCTION:
  SetAudioOutputMode

DESCRIPTION:
  Called by user to set output mode specified by henum

INPUT/OUTPUT PARAMETERS:
  henum-Output mode

RETURN VALUE:
 FILE_SOURCE_SUCCESS if successful in setting output mode
 else returns FILE_SOURCE_FAIL

SIDE EFFECTS:
  None.
======================================================================*/
FileSourceStatus AACFile::SetAudioOutputMode(FileSourceConfigItemEnum henum)
{
  FileSourceStatus status = FILE_SOURCE_FAIL;
  if(m_paacParser)
  {
    status = m_paacParser->SetAudioOutputMode(henum);
  }
  return status;
}
/* ======================================================================
FUNCTION:
  GetAudioOutputMode

DESCRIPTION:
  Called by user to query output mode specified by henum

INPUT/OUTPUT PARAMETERS:
  henum-Output mode to query

RETURN VALUE:
 FILE_SOURCE_SUCCESS if successful in retrieving output mode
 else returns FILE_SOURCE_FAIL

SIDE EFFECTS:
  None.
======================================================================*/
FileSourceStatus AACFile::GetAudioOutputMode(bool* bret,FileSourceConfigItemEnum henum)
{
  FileSourceStatus status = FILE_SOURCE_FAIL;
  if(m_paacParser)
  {
    status = m_paacParser->GetAudioOutputMode(bret,henum);
  }
  return status;
}
