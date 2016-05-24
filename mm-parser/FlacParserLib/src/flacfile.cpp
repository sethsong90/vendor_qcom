/* =======================================================================
                              flacfile.cpp
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

  Copyright(c) 2009-2013 by Qualcomm Technologies, Inc. All Rights Reserved
  Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             PERFORCE HEADER
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/FlacParserLib/main/latest/src/flacfile.cpp#28 $
========================================================================== */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE
========================================================================== */
#include "FlacParser.h"
#include "flacfile.h"
#include "MMMalloc.h"
#include "atomdefs.h"
#include "utf8conv.h"
#ifdef FEATURE_FILESOURCE_FLAC_PARSER
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
                            Function Definitions
** ======================================================================= */

/* ======================================================================
FUNCTION
  OGGStreamCallbakGetData

DESCRIPTION
  Its a callback function from OGG Parser to read the data.
  This is not implemented by the parser.
  It should be implemented by the app that calls into the parser.

ARGUMENTS
  nOffset                 Offset of the requested data (from beginning),
  nNumBytesRequest        Size of the requested data (in bytes).
  ppData                  Pointer to the buffer for filling in the OGG data
  u32UserData             Extra info From App. Given by user in aviin_open

DEPENDENCIES

RETURN VALUE
  The number of bytes actually being filled in

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
uint32 FlacFileCallbakGetData (uint64 nOffset, uint32 nNumBytesRequest,
                               unsigned char* pData, uint32  nMaxSize,
                                uint32  u32UserData )
{
  if(u32UserData)
  {
    flacfile* pFlacFile = (flacfile*)u32UserData;
    return( ( (pFlacFile->FileGetData(nOffset, nNumBytesRequest, nMaxSize, pData))== nNumBytesRequest )?nNumBytesRequest:0);
  }
  return 0;
}
/* ======================================================================
FUNCTION:
  flacfile::FileGetData

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
uint32 flacfile::FileGetData(uint64 nOffset, uint32 nNumBytesRequest,
                             uint32 nMaxSize, uint8* pData  )
{
  uint32 nRead = 0;
  if( (m_pFilePtr != NULL)&& (!m_bMediaAbort) )
  {
    nRead = FileBase::readFile(m_pFilePtr, pData, nOffset, FILESOURCE_MIN(nNumBytesRequest,nMaxSize));
  }
  return nRead;
}
/* ======================================================================
FUNCTION:
  flacfile::flacfile

DESCRIPTION:
  OGGStream constructor

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
======================================================================*/
flacfile::flacfile(const FILESOURCE_STRING filename
                   ,unsigned char *pFileBuf
                   ,uint64 bufSize)
{
  InitData();
  m_pFilePtr = OSCL_FileOpen(filename, (OSCL_TCHAR *) _T("rb"));
  uint64 size = OSCL_FileSize(filename);
  m_pFlacParser = MM_New_Args(FlacParser,(this,size,FlacFileCallbakGetData));
  (void)ParseMetaData();
}
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
flacfile::flacfile(video::iStreamPort* pport)
{
  InitData();
  m_pPort = pport;
  m_pFilePtr = OSCL_FileOpen(pport);
  int64 noffset = 0;
  uint64 size = 0;
  if(m_pPort)
  {
    if(video::iStreamPort::DS_SUCCESS == m_pPort->GetContentLength(&noffset))
    {
      size = (uint64)noffset;
    }
  }
  m_pFlacParser = MM_New_Args(FlacParser,(this,size,FlacFileCallbakGetData));
  (void)ParseMetaData();
}
#endif
/* ======================================================================
FUNCTION:
  flacfile::InitData

DESCRIPTION:
  Initializes all the data members to default

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
======================================================================*/
void flacfile::InitData()
{
  m_playAudio   = false;
  m_corruptFile = false;
  m_bStreaming  = false;
  m_nNumStreams = 0;
  m_audioLargestSize = 0;
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
  m_pPort = NULL;
#endif
  memset(&m_sampleInfo,0,(FILE_MAX_MEDIA_STREAMS * sizeof(file_sample_info_type)) );
  memset(&m_nDecodedDataSize,0,(FILE_MAX_MEDIA_STREAMS * sizeof(uint32)) );
  memset(&m_nLargestFrame,0,(FILE_MAX_MEDIA_STREAMS * sizeof(uint32)) );

  m_filename = NULL;
  m_pFileBuf = NULL;
  m_FileBufSize = 0;
  m_fileSize = 0;
  m_pFilePtr = NULL;
  m_pFlacParser = NULL;
  m_pFlacIndTrackIdTable = NULL;
  _success = false;
  _fileErrorCode = PARSER_ErrorDefault;
  m_bMediaAbort = false;
//TO DO WHEN HTTP SUPPORT IS TO BE ADDED
//#if defined(FEATURE_QTV_PSEUDO_STREAM) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
//   m_minOffsetRequired = 0;
//   bHttpStreaming = false;
//   bGetMetaDataSize = false;
//   bIsMetaDataParsed = false;
//   m_startupTime = 0;
//   memset(&m_maxPlayableTime,0,(sizeof(uint32) * FILE_MAX_MEDIA_STREAMS));
//#endif //FEATURE_QTV_3GPP_PROGRESSIVE_DNLD
}
void flacfile::SetCriticalSection(MM_HANDLE pcriticalsection)
{
  if(m_pFilePtr)
  {
    m_pFilePtr->pCriticalSection = pcriticalsection;
  }
}
/* ======================================================================
FUNCTION:
  flacfile::~flacfile

DESCRIPTION:
  OGGStream constructor

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
======================================================================*/
flacfile::~flacfile()
{
  if(m_pFilePtr)
  {
      OSCL_FileClose(m_pFilePtr);
  }
  if(m_pFlacParser)
  {
    MM_Delete(m_pFlacParser);
  }
  if(m_pFlacIndTrackIdTable)
  {
    MM_Free(m_pFlacIndTrackIdTable);
  }
}
/* ======================================================================
FUNCTION:
  flacfile::ParseMetaData

DESCRIPTION:
  Starts parsing the MPEG2 transport stream.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
======================================================================*/
bool flacfile::ParseMetaData()
{
  bool bRet = false;
  uint64 localoffset = 0;
  bool bOK = false;

  if(m_pFlacParser)
  {
    while(1)
    {
      if(m_pFlacParser->StartParsing(localoffset, true) == FLACPARSER_SUCCESS)
      {
        if(m_pFlacParser->IsMetaDataParsingDone())
        {
          bOK = true;
          break;
        }
      }
    }
    if(bOK)
    {
      bRet = true;
      _success = true;
      _fileErrorCode = PARSER_ErrorNone;
      m_nNumStreams = m_pFlacParser->GetTotalNumberOfAudioTracks();
      if(m_nNumStreams)
      {
        uint32* idlist = (uint32*)MM_Malloc(m_nNumStreams * sizeof(uint32) );
        m_pFlacIndTrackIdTable = (FlacTrackIdToIndexTable*)MM_Malloc(m_nNumStreams * sizeof(FlacTrackIdToIndexTable));
        if(m_pFlacIndTrackIdTable && idlist)
        {
          memset(m_pFlacIndTrackIdTable,0,m_nNumStreams * sizeof(FlacTrackIdToIndexTable));
          if(m_pFlacParser->GetTrackWholeIDList(idlist) == m_nNumStreams)
          {
            for(uint32 i = 0; i < m_nNumStreams; i++)
            {
              m_pFlacIndTrackIdTable[i].index = (uint8)i;
              m_pFlacIndTrackIdTable[i].bValid = true;
              m_pFlacIndTrackIdTable[i].trackId = idlist[i];
            }
          }
        }
        if(idlist)
        {
          MM_Free(idlist);
        }
      }
    }//if(bOK)
  }
  return bRet;
}
/* ======================================================================
FUNCTION   : getNextMediaSample
DESCRIPTION: gets next sample of the given track.

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
PARSER_ERRORTYPE flacfile::getNextMediaSample(uint32 ulTrackID, uint8 *pucDataBuf,
                                              uint32 *pulBufSize, uint32 &rulIndex)
{
  int32 nBytes = 0;
  FlacParserStatus retError = FLACPARSER_DEFAULT_ERROR;

  /* Validate input params and class variables */
  if(NULL == m_pFlacParser || NULL == pulBufSize || NULL == pucDataBuf ||
     0 == *pulBufSize)
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
               "getNextMediaSample invalid input params!!");
    return PARSER_ErrorInvalidParam;
  }

#ifdef FLAC_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"flacfile::getNextMediaSample");
#endif

  nBytes = *pulBufSize;
  retError = m_pFlacParser->GetCurrentSample(ulTrackID, pucDataBuf,
                                             *pulBufSize, &nBytes, true);
  if(nBytes > 0 && FLACPARSER_SUCCESS == retError)
  {
    bool bError = false;
    uint8 index = MapTrackIdToIndex(&bError, ulTrackID);
    if(!bError)
    {
      m_sampleInfo[index].num_frames = 1;
      m_pFlacParser->GetCurrentSampleTimeStamp(ulTrackID,&(m_sampleInfo[index].time));
      m_sampleInfo[index].sample++;
      m_sampleInfo[index].btimevalid = true;
      m_sampleInfo[index].size = nBytes;
      m_sampleInfo[index].sync = 1;
      MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
                   "getNextMediaSample Sample# %lu TIME %llu SampleSize %ld ",
                   m_sampleInfo[index].sample,m_sampleInfo[index].time,nBytes);
    }
    else
    {
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
                 "getNextMediaSample MapTrackIdToIndex failed for trackid %lu",
                 ulTrackID);
      return PARSER_ErrorInvalidTrackID;
    }
  }

#ifdef FLAC_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
               "getNextMediaSample returing nBytesRead %ld", nBytes);
#endif
  *pulBufSize = nBytes;
  if(FLACPARSER_SUCCESS == retError)
    return PARSER_ErrorNone;
  else
    return PARSER_ErrorEndOfFile;
}
/* ======================================================================
FUNCTION:
  flacfile::getMediaTimestampForCurrentSample

DESCRIPTION:
  gets time stamp of current sample of the track.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 time stamp in track time scale unit

SIDE EFFECTS:
  None.
======================================================================*/
uint64 flacfile::getMediaTimestampForCurrentSample(uint32 id)
{
  bool bError = false;
  uint64 ts = 0;
  uint8 index = MapTrackIdToIndex(&bError,id);
  if(!bError)
  {
    ts = m_sampleInfo[index].time;
  }
  return ts;
}
bool flacfile::GetFlacCodecData(int id,flac_format_data* pData)
{
  bool bRet = false;
  if(pData)
  {
    if((!bRet) && (m_pFlacParser))//bRet false means track id is valid
    {
      flac_metadata_streaminfo pstreaminfo;
      memset(&pstreaminfo,0,sizeof(flac_metadata_streaminfo));
      if(FLACPARSER_SUCCESS == m_pFlacParser->GetFlacStreamInfo(id,&pstreaminfo))
      {
        pData->nBitsPerSample = pstreaminfo.nBitsPerSample;
        pData->nChannels= pstreaminfo.nChannels;
        pData->nFixedBlockSize= pstreaminfo.nFixedBlockSize;
        pData->nMaxBlockSize= pstreaminfo.nMaxBlockSize;
        pData->nMinBlockSize= pstreaminfo.nMinBlockSize;
        pData->nMinFrameSize= pstreaminfo.nMinFrameSize;
        pData->nSamplingRate= pstreaminfo.nSamplingRate;
        pData->nTotalSamplesInStream= pstreaminfo.nTotalSamplesInStream;
        bRet = true;
      }
    }
  }
  return bRet;
}
/* ======================================================================
FUNCTION:
  AVIFile::resetPlayback

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
uint64 flacfile::resetPlayback(uint64 repos_time,
                                uint32 id,
                                bool bSetToSyncSample,
                                bool *bError,
                                uint64 currentPosTimeStamp)
{
  uint64 newTS = 0;
  bool bforward = (repos_time > currentPosTimeStamp)?1:0;
  flac_stream_sample_info flac_sampleInfo;
  memset(&flac_sampleInfo,0,sizeof(flac_stream_sample_info));

  MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
             "resetPlayback id %lu, repos_time %llu, currentPosTimeStamp %llu",
             id, repos_time, currentPosTimeStamp);

  if(bError && m_pFlacParser)
  {
    *bError = true;
    if(FLACPARSER_SUCCESS == m_pFlacParser->Seek(id,repos_time,currentPosTimeStamp,&flac_sampleInfo,bforward))
    {
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
                  "Seek Succeed, new TS %llu", flac_sampleInfo.ntime);
      bool bMapError = false;
      uint8 index = MapTrackIdToIndex(&bMapError,id);
      if(!bMapError)
      {
        *bError = false;
        _fileErrorCode = PARSER_ErrorNone;
        m_sampleInfo[index].num_frames = 1;
        m_sampleInfo[index].time = flac_sampleInfo.ntime;
        m_sampleInfo[index].btimevalid = true;
        newTS = m_sampleInfo[index].time;
      }
      else
      {
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_FATAL,
                     "!!!MapTrackIdToIndex failed for trackid %lu!!!", id);
        _fileErrorCode = PARSER_ErrorInvalidTrackID;
      }
    }
    else
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"Seek Failed");
      _fileErrorCode = PARSER_ErrorSeekFail;
    }
  }
  return newTS;
}
/* ======================================================================
FUNCTION:
  flacfile::getTitle

DESCRIPTION:
  gets title

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
FILESOURCE_STRING flacfile::getTitle() const
{
  FILESOURCE_STRING sTitle = NULL;
  /*uint32 tmpsize = 0;
  int index = -1;
  if(m_pFlacParser)
  {
    m_pFlacParser->GetClipMetaDataIndex((uint8*)"TITLE",std_strlen("TITLE"),&index);
    if(index != -1)
    {
      m_pFlacParser->GetClipMetaData(index,NULL,&tmpsize);
      if(tmpsize)
      {
        uint8* tmp =(uint8*)MM_Malloc(tmpsize);
        int sizeneeded = (tmpsize * sizeof(OSCL_TCHAR) ) + (1*sizeof(OSCL_TCHAR));
        OSCL_TCHAR *title =(OSCL_TCHAR*)MM_Malloc(sizeneeded);
        if(tmp && title)
        {
          m_pFlacParser->GetClipMetaData(index,tmp,&tmpsize);
          (void) UTF8ToUnicode( (const signed char*)tmp, tmpsize, title,(int32)sizeneeded );
          sTitle = title;
          MM_Free(tmp);
          MM_Free(title);
        }
      }
    }
  }*/
  return sTitle;
}
/* ======================================================================
FUNCTION:
  flacfile::getAuthor

DESCRIPTION:
  gets Author

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
FILESOURCE_STRING flacfile::getAuthor() const
{
  FILESOURCE_STRING sAuthor = NULL;
 /* uint32 tmpsize = 0;
  int index = -1;
  if(m_pFlacParser)
  {
    m_pFlacParser->GetClipMetaDataIndex((uint8*)"AUTHOR",std_strlen("AUTHOR"),&index);
    if(index != -1)
    {
      m_pFlacParser->GetClipMetaData(index,NULL,&tmpsize);
      if(tmpsize)
      {
        uint8* tmp =(uint8*)MM_Malloc(tmpsize);
        int sizeneeded = (tmpsize * sizeof(OSCL_TCHAR) ) + (1*sizeof(OSCL_TCHAR));
        OSCL_TCHAR *author =(OSCL_TCHAR*)MM_Malloc(sizeneeded);
        if(tmp && author)
        {
          m_pFlacParser->GetClipMetaData(index,tmp,&tmpsize);
          (void) UTF8ToUnicode( (const signed char*)tmp, tmpsize, author,(int32)sizeneeded );
          sAuthor = author;
          MM_Free(tmp);
          MM_Free(author);
        }
      }
    }
  }*/
  return sAuthor;
}
/* ======================================================================
FUNCTION:
  flacfile::getDescription

DESCRIPTION:
  gets description

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
FILESOURCE_STRING flacfile::getDescription() const
{
  FILESOURCE_STRING sDesc = NULL;
 /* uint32 tmpsize = 0;
  int index = -1;
  if(m_pFlacParser)
  {
    m_pFlacParser->GetClipMetaDataIndex((uint8*)"DESCRIPTION",std_strlen("DESCRIPTION"),&index);
    if(index != -1)
    {
      m_pFlacParser->GetClipMetaData(index,NULL,&tmpsize);
      if(tmpsize)
      {
        uint8* tmp =(uint8*)MM_Malloc(tmpsize);
        int sizeneeded = (tmpsize * sizeof(OSCL_TCHAR) ) + (1*sizeof(OSCL_TCHAR));
        OSCL_TCHAR *desc =(OSCL_TCHAR*)MM_Malloc(sizeneeded);
        if(tmp && desc)
        {
          m_pFlacParser->GetClipMetaData(index,tmp,&tmpsize);
          (void) UTF8ToUnicode( (const signed char*)tmp, tmpsize, desc,(int32)sizeneeded );
          sDesc = desc;
          MM_Free(tmp);
          MM_Free(desc);
        }
      }
    }
  }*/
  return sDesc;
}
/* ======================================================================
FUNCTION:
  flacfile::getRating

DESCRIPTION:
  gets rating

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
FILESOURCE_STRING flacfile::getRating() const
{
  FILESOURCE_STRING sRating = NULL;
 /* uint32 tmpsize = 0;
  int index = -1;
  if(m_pFlacParser)
  {
    m_pFlacParser->GetClipMetaDataIndex((uint8*)"RATING",std_strlen("RATING"),&index);
    if(index != -1)
    {
      m_pFlacParser->GetClipMetaData(index,NULL,&tmpsize);
      if(tmpsize)
      {
        uint8* tmp =(uint8*)MM_Malloc(tmpsize);
        int sizeneeded = (tmpsize * sizeof(OSCL_TCHAR) ) + (1*sizeof(OSCL_TCHAR));
        OSCL_TCHAR *rating =(OSCL_TCHAR*)MM_Malloc(sizeneeded);
        if(tmp && rating)
        {
          m_pFlacParser->GetClipMetaData(index,tmp,&tmpsize);
          (void) UTF8ToUnicode( (const signed char*)tmp, tmpsize, rating,(int32)sizeneeded );
          sRating = rating;
          MM_Free(tmp);
          MM_Free(rating);
        }
      }
    }
  }*/
  return sRating;
}
/* ======================================================================
FUNCTION:
  flacfile::getCopyright

DESCRIPTION:
  gets Copyright

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
FILESOURCE_STRING flacfile::getCopyright() const
{
  FILESOURCE_STRING sCopyright = NULL;
  /*uint32 tmpsize = 0;
  int index = -1;
  if(m_pFlacParser)
  {
    m_pFlacParser->GetClipMetaDataIndex((uint8*)"COPYRIGHT",std_strlen("COPYRIGHT"),&index);
    if(index != -1)
    {
      m_pFlacParser->GetClipMetaData(index,NULL,&tmpsize);
      if(tmpsize)
      {
        uint8* tmp =(uint8*)MM_Malloc(tmpsize);
        int sizeneeded = (tmpsize * sizeof(OSCL_TCHAR) ) + (1*sizeof(OSCL_TCHAR));
        OSCL_TCHAR *copyrt =(OSCL_TCHAR*)MM_Malloc(sizeneeded);
        if(tmp && copyrt)
        {
          m_pFlacParser->GetClipMetaData(index,tmp,&tmpsize);
          (void) UTF8ToUnicode( (const signed char*)tmp, tmpsize, copyrt,(int32)sizeneeded );
          sCopyright = copyrt;
          MM_Free(tmp);
          MM_Free(copyrt);
        }
      }
    }
  }*/
  return sCopyright;
}
/* ======================================================================
FUNCTION:
  flacfile::getVersion

DESCRIPTION:
  gets Copyright

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
FILESOURCE_STRING flacfile::getVersion() const
{
  FILESOURCE_STRING sVersion = NULL;
  /*uint32 tmpsize = 0;
  int index = -1;
  if(m_pFlacParser)
  {
    m_pFlacParser->GetClipMetaDataIndex((uint8*)"VERSION",std_strlen("VERSION"),&index);
    if(index != -1)
    {
      m_pFlacParser->GetClipMetaData(index,NULL,&tmpsize);
      if(tmpsize)
      {
        uint8* tmp =(uint8*)MM_Malloc(tmpsize);
        int sizeneeded = (tmpsize * sizeof(OSCL_TCHAR) ) + (1*sizeof(OSCL_TCHAR));
        OSCL_TCHAR *version =(OSCL_TCHAR*)MM_Malloc(sizeneeded);
        if(tmp && version)
        {
          m_pFlacParser->GetClipMetaData(index,tmp,&tmpsize);
          (void) UTF8ToUnicode( (const signed char*)tmp, tmpsize, version,(int32)sizeneeded );
          sVersion = version;
          MM_Free(tmp);
          MM_Free(version);
        }
      }
    }
  }*/
  return sVersion;
}
/* ======================================================================
FUNCTION:
  flacfile::getCreationDate

DESCRIPTION:
  gets Copyright

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
FILESOURCE_STRING flacfile::getCreationDate() const
{
  FILESOURCE_STRING sDate = NULL;
  /*uint32 tmpsize = 0;
  int index = -1;
  if(m_pFlacParser)
  {
    m_pFlacParser->GetClipMetaDataIndex((uint8*)"DATE",std_strlen("DATE"),&index);
    if(index != -1)
    {
      m_pFlacParser->GetClipMetaData(index,NULL,&tmpsize);
      if(tmpsize)
      {
        uint8* tmp =(uint8*)MM_Malloc(tmpsize);
        int sizeneeded = (tmpsize * sizeof(OSCL_TCHAR) ) + (1*sizeof(OSCL_TCHAR));
        OSCL_TCHAR *date =(OSCL_TCHAR*)MM_Malloc(sizeneeded);
        if(tmp && date)
        {
          m_pFlacParser->GetClipMetaData(index,tmp,&tmpsize);
          (void) UTF8ToUnicode( (const signed char*)tmp, tmpsize, date,(int32)sizeneeded );
          sDate = date;
          MM_Free(tmp);
          MM_Free(date);
        }
      }
    }
  }*/
  return sDate;
}
/* ======================================================================
FUNCTION:
  flacfile::getPerf

DESCRIPTION:
  gets Copyright

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
FILESOURCE_STRING flacfile::getPerf() const
{
  FILESOURCE_STRING sPerf = NULL;
 /* uint32 tmpsize = 0;
  int index = -1;
  if(m_pFlacParser)
  {
    m_pFlacParser->GetClipMetaDataIndex((uint8*)"PERFORMER",std_strlen("PERFORMER"),&index);
    if(index != -1)
    {
      m_pFlacParser->GetClipMetaData(index,NULL,&tmpsize);
      if(tmpsize)
      {
        uint8* tmp =(uint8*)MM_Malloc(tmpsize);
        int sizeneeded = (tmpsize * sizeof(OSCL_TCHAR) ) + (1*sizeof(OSCL_TCHAR));
        OSCL_TCHAR *perf =(OSCL_TCHAR*)MM_Malloc(sizeneeded);
        if(tmp && perf)
        {
          m_pFlacParser->GetClipMetaData(index,tmp,&tmpsize);
          (void) UTF8ToUnicode( (const signed char*)tmp, tmpsize, perf,(int32)sizeneeded );
          sPerf = perf;
          MM_Free(tmp);
          MM_Free(perf);
        }
      }
    }
  }*/
  return sPerf;
}
/* ======================================================================
FUNCTION:
  flacfile::getGenre

DESCRIPTION:
  gets Copyright

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
FILESOURCE_STRING flacfile::getGenre() const
{
  FILESOURCE_STRING sGenre = NULL;
 /* uint32 tmpsize = 0;
  int index = -1;
  if(m_pFlacParser)
  {
    m_pFlacParser->GetClipMetaDataIndex((uint8*)"GENRE",std_strlen("GENRE"),&index);
    if(index != -1)
    {
      m_pFlacParser->GetClipMetaData(index,NULL,&tmpsize);
      if(tmpsize)
      {
        uint8* tmp =(uint8*)MM_Malloc(tmpsize);
        int sizeneeded = (tmpsize * sizeof(OSCL_TCHAR) ) + (1*sizeof(OSCL_TCHAR));
        OSCL_TCHAR *genre =(OSCL_TCHAR*)MM_Malloc(sizeneeded);
        if(tmp && genre)
        {
          m_pFlacParser->GetClipMetaData(index,tmp,&tmpsize);
          (void) UTF8ToUnicode( (const signed char*)tmp, tmpsize, genre,(int32)sizeneeded );
          sGenre = genre;
          MM_Free(tmp);
          MM_Free(genre);
        }
      }
    }
  }*/
  return sGenre;
}
/* ======================================================================
FUNCTION:
  flacfile::getMovieDuration

DESCRIPTION:
  gets movie duration in movie timescale unit.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint64 flacfile::getMovieDuration() const
{
  uint64 nDuration = 0;
  if(!m_pFlacParser)
  {
    return nDuration;
  }
  nDuration = getMovieDurationMsec();
  return nDuration;
}
/* ======================================================================
FUNCTION:
  flacfile::getMovieTimescale

DESCRIPTION:
  gets movie timescale

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 flacfile::getMovieTimescale() const
{
  uint32 nTimeScale = 1000;
  if(!m_pFlacParser)
  {
    nTimeScale = 0;
  }
  return nTimeScale;
}
/* ======================================================================
FUNCTION:
  flacfile::getMovieDurationMsec

DESCRIPTION:
  gets movie duration in milli secs

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint64 flacfile::getMovieDurationMsec() const
{
  uint64 nDuration = 0;
  if(!m_pFlacParser)
  {
    return nDuration;
  }
  nDuration = m_pFlacParser->GetClipDurationInMsec();
  return nDuration;
}
/* ======================================================================
FUNCTION:
  flacfile::GetMaximumBitRateForTrack(uint32 trackId)

DESCRIPTION:
  Returns the max bitrate for the track identified by trackId

INPUT/OUTPUT PARAMETERS:
  TrackId

RETURN VALUE:
 Max Data Bit Rate

SIDE EFFECTS:
  None.
======================================================================*/
/* ======================================================================
FUNCTION:
  AVIFile::getTrackWholeIDList

DESCRIPTION:
  gets list of track IDs

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 flacfile::getTrackWholeIDList(uint32 *ids)
{
  if(!m_pFlacParser)
  {
    return 0;
  }
  return (m_pFlacParser->GetTrackWholeIDList(ids));
}
/* ======================================================================
FUNCTION:
  flacfile::getTrackContentVersion

DESCRIPTION:
  gets content version number

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
/* ======================================================================
FUNCTION:
  flacfile::trackRandomAccessDenied

DESCRIPTION:
  gets repositioning permission for the track.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/

/* ======================================================================
FUNCTION:
  flacfile::getTrackMediaDuration

DESCRIPTION:
  gets track duration in track time scale unit

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint64 flacfile::getTrackMediaDuration(uint32 id)
{
  return getMovieDurationMsec();
}
/* ======================================================================
FUNCTION:
  flacfile::getTrackMediaTimescale

DESCRIPTION:
  gets track time scale

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 flacfile::getTrackMediaTimescale(uint32 id)
{
  return 1000;
}
/* ======================================================================
FUNCTION:
  flacfile::getTrackAudioSamplingFreq

DESCRIPTION:
  gets audio track's sampling frequency

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 flacfile::getTrackAudioSamplingFreq(uint32 id)
{
  uint32 freq = 0;
  if(m_pFlacParser)
  {
    freq = m_pFlacParser->GetAudioSamplingFrequency(id);
  }
  return freq;
}

/* ============================================================================
  @brief  Returns number of bits used for eacha audio sample

  @details    This function is used to return no. of bits used for each
              audio sample.

  @param[in]      trackId       Track Id number.

  @return     MKAV_API_SUCCESS indicating sample read successfully.
              Else, it will report corresponding error.
  @note       BufferSize should be more than maximum frame size value.
============================================================================ */
unsigned long flacfile::GetAudioBitsPerSample(int trackId)
{
  uint32 ulBitWidth = 0;
  if(m_pFlacParser)
  {
    ulBitWidth = m_pFlacParser->GetAudioBitsPerSample(trackId);
  }
  return ulBitWidth;
}

/* ======================================================================
FUNCTION:
  flacfile::GetNumAudioChannels

DESCRIPTION:
  returns number of audio channels

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
unsigned long flacfile::GetNumAudioChannels(int id)
{
  uint32 nchnls = 0;
  if(m_pFlacParser)
  {
    nchnls = m_pFlacParser->GetNumberOfAudioChannels(id);
  }
  return nchnls;
}
/* ======================================================================
FUNCTION:
  flacfile::peekCurSample

DESCRIPTION:
  gets information about current sample of a track

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
PARSER_ERRORTYPE flacfile::peekCurSample(uint32 trackid, file_sample_info_type *pSampleInfo)
{
  PARSER_ERRORTYPE reterror = PARSER_ErrorDefault;
  if((!m_pFlacParser) || (!pSampleInfo))
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"flacfile::peekCurSample invalid argument");
  }
  else
  {
    bool bError = false;
    uint8 index = MapTrackIdToIndex(&bError,trackid);
    if(!bError)
    {
      *pSampleInfo = m_sampleInfo[index];
      reterror = PARSER_ErrorNone;
    }
  }
  return reterror;
}
/* ======================================================================
FUNCTION:
  flacfile::getTrackOTIType

DESCRIPTION:
  gets track OTI value

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint8 flacfile::getTrackOTIType(uint32 id)
{
  return FLAC_AUDIO;
}
/* ======================================================================
FUNCTION:
  flacfile::getTrackMaxBufferSizeDB

DESCRIPTION:
  gets maximum buffer size to play the track

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
int32  flacfile::getTrackMaxBufferSizeDB(uint32 id)
{
  uint32 bufSize = 0;
  if(m_pFlacParser)
  {
    bufSize = m_pFlacParser->GetFlacMaxBufferSize(id);
  }
  return bufSize;
}
/* ======================================================================
FUNCTION:
  flacfile::getTrackAverageBitrate

DESCRIPTION:
  gets track's average bitrate

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 average bitrate

SIDE EFFECTS:
  None.
======================================================================*/
int32 flacfile::getTrackAverageBitrate(uint32 id)
{
  uint32 bitrate = 0;
  if(m_pFlacParser)
  {
    bitrate = m_pFlacParser->GetTrackAverageBitRate(id);
  }
  return bitrate;
}

/* ======================================================================
FUNCTION:
  flacfile::getTrackDecoderSpecificInfoContent

DESCRIPTION:
  this returns codec Header/size for given track id

INPUT/OUTPUT PARAMETERS:
  @in id: Track identifier
  @in buf: Buffer to copy codec header
  @in/@out: pbufSize: Size of codec header
  When buf is NULL, function returns the size of codec header into pbufSize.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
PARSER_ERRORTYPE flacfile::getTrackDecoderSpecificInfoContent(uint32 id, uint8* buf, uint32 *pbufSize)
{
  PARSER_ERRORTYPE errorCode = PARSER_ErrorDefault;
  if( (!m_pFlacParser) || (!pbufSize) )
  {
    return errorCode;
  }
  uint32 size = m_pFlacParser->GetCodecHeaderSize(id);
  uint8* header = m_pFlacParser->GetCodecHeader(id);
  if(buf && (*pbufSize >= size) && size)
  {
    memcpy(buf,header,size);
    *pbufSize = size;
    errorCode = PARSER_ErrorNone;
  }
  else if(size > 0)
  {
    *pbufSize = size;
    errorCode = PARSER_ErrorNone;
  }
  return errorCode;
}

/* ======================================================================
FUNCTION:
  flacfile::MapTrackIdToIndex

DESCRIPTION:
  Maps given track-id to track index.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint8 flacfile::MapTrackIdToIndex(bool* bError,uint32 trackid)
{
  uint8 index = 0;
  if(bError)
  {
    *bError = true;
    for(uint32 i = 0; i < m_nNumStreams; i++)
    {
      if( (m_pFlacIndTrackIdTable[i].trackId == trackid) &&
        (m_pFlacIndTrackIdTable[i].bValid) )
      {
        index = m_pFlacIndTrackIdTable[i].index;
        *bError = false;
        break;
      }
    }
  }
  if(bError && *bError)
  {
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_FATAL,
                 "flacfile::MapTrackIdToIndex failed for trackid %lu",trackid);
  }
  return index;
}
//#if defined(FEATURE_QTV_PSEUDO_STREAM) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
///*===========================================================================
//
//FUNCTION  sendParserEvent
//
//DESCRIPTION
//  Public method used send parser events
//
//===========================================================================*/
//void flacfile::sendParserEvent(ParserStatusCode status)
//{
//  }
////*===========================================================================
////
////FUNCTION  sendParseHTTPStreamEvent
////
////DESCRIPTION
////  Public method used to switch contexts and call the parseHttpStream.
////
////===========================================================================*/
////void flacfile::sendParseHTTPStreamEvent(void)
////{
////  QTV_PROCESS_HTTP_STREAM_type *pEvent = QCCreateMessage(QTV_PROCESS_HTTP_STREAM, m_pMpeg4Player);
////
////  if (pEvent)
////  {
////    pEvent->bHasAudio = (bool) m_playAudio;
////    pEvent->bHasVideo = (bool) m_playVideo;
////    pEvent->bHasText = (bool) m_playText;
////    QCUtils::PostMessage(pEvent, 0, NULL);
////  }
////}
////
/////*===========================================================================
////
////FUNCTION  updateBufferWritePtr
////
////DESCRIPTION
////  Public method used to update the write buffer offset during Http streaming.
////
////===========================================================================*/
////void flacfile::updateBufferWritePtr ( uint32 writeOffset )
////{
////  WMFDecoderEx *pDecoder = (WMFDecoderEx *)m_hASFDecoder;
////  //Executing in the UI thread context.
////
////  if(pDecoder)
////  {
////    pDecoder->wHttpDataBuffer.Offset = writeOffset;
////    pDecoder->wHttpDataBuffer.bValid = TRUE;
////  }
////
////  if((parserState == Common::PARSER_PAUSE) || (parserState == Common::PARSER_RESUME))
////  {
////     //check if we got sufficient data to start parsing the
////     //meta data.
////     sendParseHTTPStreamEvent();
////  }
////}
////
////
/////*===========================================================================
////
////FUNCTION  getMetaDataSize
////
////DESCRIPTION
////  Public method used to determine the meta-data size of the fragment.
////
////===========================================================================*/
////tWMCDecStatus flacfile::getMetaDataSize ( void )
////{
////  tWMCDecStatus wmerr = WMCDec_Fail;
////  uint32 nHttpDownLoadBufferOffset = 0;
////  boolean bHttpDownLoadBufferOffsetValid = GetHTTPStreamDownLoadedBufferOffset(&nHttpDownLoadBufferOffset);
////  WMFDecoderEx *pDecoder = (WMFDecoderEx *)m_hASFDecoder;
////  U32_WMC nAsfHeaderSize = 0;
////
////  if( pDecoder && bHttpDownLoadBufferOffsetValid && (nHttpDownLoadBufferOffset > (MIN_OBJECT_SIZE + sizeof(U32_WMC) + 2*sizeof(U8_WMC)) ) )
////  {
////    wmerr = GetAsfHeaderSize(&m_hASFDecoder,&nAsfHeaderSize );
////  }
////  if(wmerr == WMCDec_Succeeded)
////  {
////    m_HttpDataBufferMinOffsetRequired.Offset = nAsfHeaderSize;
////    m_HttpDataBufferMinOffsetRequired.bValid = TRUE;
////    bGetMetaDataSize = FALSE;
////    return wmerr;
////  }
////  else
////  {
////    bGetMetaDataSize = TRUE;
////    return wmerr;
////  }
////
////  return WMCDec_Fail;
////}
////
/////*===========================================================================
////
////FUNCTION  parseHTTPStream
////
////DESCRIPTION
////  Public method used to parse the Http Stream.
////
////===========================================================================*/
////bool flacfile::parseHTTPStream ( void )
////{
////
////  tWMCDecStatus wmerr = WMCDec_Succeeded;
////  bool returnStatus = true;
////  uint32 nHttpDownLoadBufferOffset = 0;
////  boolean bHttpDownLoadBufferOffsetValid = GetHTTPStreamDownLoadedBufferOffset(&nHttpDownLoadBufferOffset);
////
////
////  if(bGetMetaDataSize)
////  {
////     wmerr = getMetaDataSize();
////  }
////
////  if(wmerr != WMCDec_Succeeded)
////  {
////    //QTV_PS_PARSER_STATUS_PAUSED
////    sendParserEvent(Common::PARSER_PAUSE);
////    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "Parser State = Common::PARSER_PAUSE");
////    returnStatus = false;
////  }
////  else if(wmerr == WMCDec_Succeeded)
////  {
////     if((nHttpDownLoadBufferOffset >= m_HttpDataBufferMinOffsetRequired.Offset)
////        && bHttpDownLoadBufferOffsetValid && m_HttpDataBufferMinOffsetRequired.bValid)
////     {
////       if( !bIsMetaDataParsed )
////       {
////         if(ParseMetaData() == WMCDec_Succeeded)
////   {
////           //QTV_PS_PARSER_STATUS_RESUME
////           bIsMetaDataParsed = TRUE;
////           m_HttpDataBufferMinOffsetRequired.bValid = FALSE;
////           sendParserEvent(Common::PARSER_RESUME);
////           returnStatus = true;
////           MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "Parser State = Common::PARSER_RESUME");
////         }
////   else
////   {
////           //QTV_PS_PARSER_STATUS_PAUSED
////           sendParserEvent(Common::PARSER_PAUSE);
////           MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "Parser State = Common::PARSER_PAUSE");
////           returnStatus = false;
////   }
////       }
////     }
////
////     if ((parserState == Common::PARSER_RESUME) && CanPlayTracks(m_startupTime) )
////     {
////        //QTV_PS_PARSER_STATUS_READY
////        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "Parser State = Common::PARSER_READY");
////        sendParserEvent(Common::PARSER_READY);
////  returnStatus = true;
////      }
////      else
////      {
////        returnStatus = false;
////      }
////  }
////
////  return returnStatus;
////}
////
////
/////*===========================================================================
////
////FUNCTION  sendHTTPStreamUnderrunEvent
////
////DESCRIPTION
////  Public method used to switch contexts and notify the player about buffer-underrun.
////
////===========================================================================*/
////void flacfile::sendHTTPStreamUnderrunEvent(void)
////{
////  QTV_HTTP_STREAM_BUFFER_UNDERRUN_EVENT_type *pEvent = QCCreateMessage(QTV_HTTP_STREAM_BUFFER_UNDERRUN_EVENT, m_pMpeg4Player);
////
////  if (pEvent)
////  {
////    pEvent->bAudio = (bool) m_playAudio;
////    pEvent->bVideo = (bool) m_playVideo;
////    pEvent->bText = (bool) m_playText;
////    QCUtils::PostMessage(pEvent, 0, NULL);
////  }
////}
/////*===========================================================================
////
////FUNCTION  GetHTTPStreamDownLoadedBufferOffset
////
////DESCRIPTION
////  Public method used to switch contexts and notify the player about buffer-underrun.
////
////===========================================================================*/
////boolean flacfile::GetHTTPStreamDownLoadedBufferOffset(U32_WMC * pOffset)
////{
////  WMFDecoderEx *pDecoder = (WMFDecoderEx *)m_hASFDecoder;
////
////  if(pDecoder && pOffset)
////  {
////    if(m_fpFetchBufferedDataSize)
////    {
////      //Pull interface so pull dnld data size from OEM
////      m_fpFetchBufferedDataSize( 0, &(pDecoder->wHttpDataBuffer.Offset, m_QtvInstancehandle) );
////      pDecoder->wHttpDataBuffer.bValid = TRUE;
////    }
////    if( pDecoder->wHttpDataBuffer.bValid )
////    {
////      *pOffset = pDecoder->wHttpDataBuffer.Offset;
////      return TRUE;
////    }
////  }
////  return FALSE;
////}
////
///*===========================================================================
//
//FUNCTION  GetTotalAvgBitRate
//
//DESCRIPTION
//  Public method used to switch contexts and notify the player about buffer-underrun.
//
//===========================================================================*/
//bool flacfile::GetTotalAvgBitRate(uint32 * pBitRate)
//{
//
//  return true;
//}
//
///*===========================================================================
//
//FUNCTION  CanPlayTracks
//
//DESCRIPTION
//  Public method used to switch contexts and notify the player about buffer-underrun.
//
//===========================================================================*/
//bool flacfile::CanPlayTracks(uint32 nTotalPBTime)
//{
//    return true;
//}
//
///* ======================================================================
//FUNCTION:
//  flacfile::GetMediaMaxTimeStampPlayable
//
//DESCRIPTION:
//  gets time stamp of current sample of the track.
//
//INPUT/OUTPUT PARAMETERS:
//  None.
//
//RETURN VALUE:
// time stamp in track time scale unit
//
//SIDE EFFECTS:
//  None.
//======================================================================*/
////tWMCDecStatus flacfile::GetMediaMaxPlayableTime(U32_WMC *nMaxPBTime)
////{
////  uint32 nMaxPlayableTime = 0;  // default max playable time
////
////  if( (m_pStreamDecodePattern == NULL) || (nMaxPBTime == NULL) )
////  {
////    return WMCDec_InValidArguments;
////  }
////  for(uint16 i=0; i<(int)m_nNumStreams; i++)
////  {
////    if( m_maxPlayableTime[i] && (m_pStreamDecodePattern[i].tPattern != Discard_WMC) )
////    {
////      if(!nMaxPlayableTime)
////      {
////        /* initialize with valid track sample time */
////        nMaxPlayableTime = m_maxPlayableTime[i];
////        continue;
////      }
////      /* Take the MIN value to make sure all tracks are playable atleast nMaxPlayableTime */
////      nMaxPlayableTime = MIN(m_maxPlayableTime[i],nMaxPlayableTime);
////    }
////  }
////
////    *nMaxPBTime = nMaxPlayableTime;
////
////  return WMCDec_Succeeded;
////}
//#endif //  FEATURE_QTV_3GPP_PROGRESSIVE_DNLD
#endif //#define FEATURE_FILESOURCE_FLAC_PARSER
