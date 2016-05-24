/* =======================================================================
                              OGGStream.cpp
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
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/OGGParserLib/main/latest/src/OGGStream.cpp#31 $
========================================================================== */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE
========================================================================== */
#include "OGGStreamParser.h"
#include "OGGStream.h"
#include "MMMalloc.h"
#include "atomdefs.h"
#include "utf8conv.h"
#ifdef FEATURE_FILESOURCE_OGG_PARSER
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
uint32 OGGStreamCallbakGetData (uint64 nOffset, uint32 nNumBytesRequest,
                                 unsigned char* pData, uint32  nMaxSize,
                                 uint32  u32UserData )
{
  uint32 ulDataRead = 0;
  if(u32UserData)
  {
    OGGStream* pOGGStream = (OGGStream*)u32UserData;
    ulDataRead = pOGGStream->FileGetData(nOffset, nNumBytesRequest, nMaxSize,
                                         pData);
  }
  return ulDataRead;
}
/* ======================================================================
FUNCTION:
  OGGStream::FileGetData

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
uint32 OGGStream::FileGetData(uint64 nOffset, uint32 nNumBytesRequest,
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
  OGGStream::OGGStream

DESCRIPTION:
  OGGStream constructor

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
======================================================================*/
OGGStream::OGGStream(const FILESOURCE_STRING filename
                     ,unsigned char *pFileBuf
                     ,uint32 bufSize
                     ,bool bPlayVideo
                     ,bool bPlayAudio
                    )
{
  InitData();
  m_playAudio = bPlayAudio;
  m_playVideo = bPlayVideo;
  m_pFilePtr = OSCL_FileOpen(filename, (OSCL_TCHAR *) _T("rb"));
  m_fileSize = OSCL_FileSize(filename);
  m_pOGGStreamParser = MM_New_Args(OGGStreamParser,(this, m_fileSize,
                                                    bPlayAudio));
  (void)ParseMetaData();
}
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
OGGStream::OGGStream(video::iStreamPort* pport , bool bPlayVideo, bool bPlayAudio)
{
  InitData();
  m_fileSize    = MAX_FILE_SIZE;
  m_pPort       = pport;
  m_playAudio   = bPlayAudio;
  m_playVideo   = bPlayVideo;
  m_pFilePtr    = OSCL_FileOpen(pport);
  int64 noffset = 0;
  if(m_pPort)
  {
    if(video::iStreamPort::DS_SUCCESS == m_pPort->GetContentLength(&noffset) &&
       (noffset))
    {
      m_fileSize = (uint64)noffset;
    }
  }
  m_pOGGStreamParser = MM_New_Args(OGGStreamParser,(this, m_fileSize,
                                                    bPlayAudio));
  (void)ParseMetaData();
}
#endif
/* ======================================================================
FUNCTION:
  OGGStream::InitData

DESCRIPTION:
  Initializes all the data members to default

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
======================================================================*/
void OGGStream::InitData()
{
  m_playAudio       = false;
  m_playVideo       = false;
  m_bStreaming      = false;
  m_bMetaDatainUTF8 = false;
  m_nNumStreams     = 0;

  memset(&m_sampleInfo,0,(FILE_MAX_MEDIA_STREAMS * sizeof(file_sample_info_type)) );

  m_fileSize = 0;
  m_pFilePtr = NULL;
  m_pOGGStreamParser = NULL;
  m_pIndTrackIdTable = NULL;
  _success = false;
  _fileErrorCode = PARSER_ErrorDefault;
  m_bMediaAbort = false;
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
  m_pPort = NULL;
#endif
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
void OGGStream::SetCriticalSection(MM_HANDLE pcriticalsection)
{
  if(m_pFilePtr)
  {
    m_pFilePtr->pCriticalSection = pcriticalsection;
  }
}
/* ======================================================================
FUNCTION:
  OGGStream::~OGGStream

DESCRIPTION:
  OGGStream constructor

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
======================================================================*/
OGGStream::~OGGStream()
{
  if(m_pFilePtr)
  {
      OSCL_FileClose(m_pFilePtr);
  }
  if(m_pOGGStreamParser)
  {
    MM_Delete(m_pOGGStreamParser);
  }
  if(m_pIndTrackIdTable)
  {
    MM_Free(m_pIndTrackIdTable);
  }
}
/* ======================================================================
FUNCTION:
  OGGStream::ParseMetaData

DESCRIPTION:
  Starts parsing the MPEG2 transport stream.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
======================================================================*/
bool OGGStream::ParseMetaData()
{
  bool bRet = false;
  if(m_pOGGStreamParser
#ifdef FEATURE_OGG_AUDIO_ONLY
     && m_playAudio
#endif
     )
  {
    if(m_pOGGStreamParser->StartParsing() == OGGSTREAM_SUCCESS)
    {
      bRet = true;
      _success = true;
      _fileErrorCode = PARSER_ErrorNone;
      m_nNumStreams = m_pOGGStreamParser->GetTotalNumberOfTracks();
      if(m_nNumStreams)
      {
        uint32* idlist = (uint32*)MM_Malloc(m_nNumStreams * sizeof(uint32) );
        m_pIndTrackIdTable = (OggTrackIdToIndexTable*)MM_Malloc(m_nNumStreams * sizeof(OggTrackIdToIndexTable));
        if(m_pIndTrackIdTable && idlist)
        {
          memset(m_pIndTrackIdTable,0,m_nNumStreams * sizeof(OggTrackIdToIndexTable));
          if(m_pOGGStreamParser->GetTrackWholeIDList(idlist) == m_nNumStreams)
          {
            for(uint32 i = 0; i < m_nNumStreams; i++)
            {
              m_pIndTrackIdTable[i].index = (uint8)i;
              m_pIndTrackIdTable[i].bValid = true;
              m_pIndTrackIdTable[i].trackId = idlist[i];
            }
          }
        }
        if(idlist)
        {
          MM_Free(idlist);
        }
      }
    }
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
PARSER_ERRORTYPE OGGStream::getNextMediaSample(uint32 ulTrackID, uint8 *pucDataBuf,
                                               uint32 *pulBufSize, uint32 &rulIndex)
{
  int32 nBytes = 0;
  PARSER_ERRORTYPE retError = PARSER_ErrorDefault;
  OGGStreamStatus eStatus = OGGSTREAM_FAIL;

  /* Validate input params and class variables */
  if(NULL == m_pOGGStreamParser || NULL == pulBufSize || NULL == pucDataBuf ||
     0 == *pulBufSize)
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
               "getNextMediaSample invalid input params!!");
    return PARSER_ErrorInvalidParam;
  }
  uint32 ulSerialNum = m_pOGGStreamParser->GetTrackSerialNo(ulTrackID);

#ifdef OGG_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"OGGStream::getNextMediaSample");
#endif
  nBytes = *pulBufSize;
  eStatus = m_pOGGStreamParser->GetCurrentSample(ulSerialNum, pucDataBuf,
                                                 *pulBufSize, &nBytes);
  if(OGGSTREAM_SUCCESS == eStatus)
    retError = PARSER_ErrorNone;
  /* Apart from success, all other errors will be treated as EOF only.
     If streaming support is added, then we need to change this. */
  else
    retError = PARSER_ErrorEndOfFile;
  if(OGGSTREAM_READ_ERROR == eStatus )
  {
      //In case of read error  in local playback  we assume that the end of stream
      //has arrived  or data is no longer received. Here we return 0 bytes and
      //this will be intepreted as media_end in upper layers.
  }
  if(nBytes > 0 && OGGSTREAM_SUCCESS == eStatus)
  {
    bool  bError     = false;
    bool  bTimeValid = false;
    uint8 index      = MapTrackIdToIndex(&bError, ulTrackID);
    if(!bError)
    {
      m_sampleInfo[index].num_frames = 1;
      m_sampleInfo[index].nGranule   = 0;
      eStatus = m_pOGGStreamParser->GetCurrentSampleTimeStamp(ulSerialNum,
                                    &(m_sampleInfo[index].time),
                                    &(m_sampleInfo[index].nGranule),
                                    &bTimeValid);
      if(eStatus == OGGSTREAM_SUCCESS && bTimeValid)
      {
        m_sampleInfo[index].btimevalid = true;
      }
      else
      {
        m_sampleInfo[index].btimevalid = false;
      }
      m_sampleInfo[index].sample++;
      m_sampleInfo[index].size = nBytes;
      m_sampleInfo[index].sync = 1;
    }
  }
#ifdef OGG_PARSER_DEBUG
  MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
    "getNextMediaSample returing nBytesRead %lu",nBytes);
#endif
  *pulBufSize = nBytes;
  return retError;
}
/* ======================================================================
FUNCTION:
  OGGStream::getMediaTimestampForCurrentSample

DESCRIPTION:
  gets time stamp of current sample of the track.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 time stamp in track time scale unit

SIDE EFFECTS:
  None.
======================================================================*/
uint64 OGGStream::getMediaTimestampForCurrentSample(uint32 id)
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
/* ======================================================================
FUNCTION:
  OGGStream::skipNSyncSamples

DESCRIPTION:
  Skips specified sync samples in forward or backward direction.

INPUT/OUTPUT PARAMETERS:

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
//Need to be implemented when we add support for VIDEO as skipping
//few frames for audio does not really make sense as each frame is only few
//milliseconds
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
uint64 OGGStream::resetPlayback(uint64 repos_time,
                                uint32 id,
                                bool bSetToSyncSample,
                                bool *bError,
                                uint64 currentPosTimeStamp)
{
  uint64 newTS = 0;

  bool bforward = (repos_time > currentPosTimeStamp)?1:0;
  ogg_stream_sample_info ogg_sampleInfo;
  memset(&ogg_sampleInfo,0,sizeof(ogg_stream_sample_info));

  MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
               "resetPlayback id %lu repos_time %llu currentPosTimeStamp %llu",
               id, repos_time, currentPosTimeStamp);

  if(bError && m_pOGGStreamParser)
  {
    uint32 ulSerialNum = m_pOGGStreamParser->GetTrackSerialNo(id);
    *bError = true;
    if(OGGSTREAM_SUCCESS == m_pOGGStreamParser->Seek
       (ulSerialNum,repos_time,currentPosTimeStamp,&ogg_sampleInfo,bforward,false,0)
      )
    {
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,"Seek Succeed, new TS %llu",ogg_sampleInfo.ntime);
      *bError = false;
      _fileErrorCode = PARSER_ErrorNone;
      bool bMapError = false;
      uint8 index = MapTrackIdToIndex(&bMapError,id);
      if(!bMapError)
      {
        m_sampleInfo[index].num_frames = 1;
        m_sampleInfo[index].time = ogg_sampleInfo.ntime;
        m_sampleInfo[index].sample =  ogg_sampleInfo.nsample;
        m_sampleInfo[index].size = ogg_sampleInfo.nsize;
        m_sampleInfo[index].sync = 1;
        newTS = m_sampleInfo[index].time;
      }
      else
      {
        MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_FATAL,
                     "!!!MapTrackIdToIndex failed for trackid %lu!!!",id);
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
  GetClipMetaData

DESCRIPTION:
  Provides different metadata fields info in the o/p buffer

INPUT/OUTPUT PARAMETERS:
  pucDataBuf   : Buffer to store metadata string in Wchar format.
  pulDatabufLen: Buffer Size.
  ienumData    : Enum to indicate which metadata field requested.

RETURN VALUE:
 PARSER_ErrorNone if Success
 Error status, if failure

SIDE EFFECTS:
  None.
======================================================================*/
PARSER_ERRORTYPE OGGStream::GetClipMetaData(wchar_t *pucDataBuf,
                                            uint32 *pulDatabufLen,
                                            FileSourceMetaDataType ienumData)
{
  uint32 ulMetaIndex = MAX_FIELDNAMES_SUPPORTED;
  if((NULL == pulDatabufLen) || (NULL == m_pOGGStreamParser))
    return PARSER_ErrorInvalidParam;

  uint32 ulMetaDataLen = 0;

  m_bMetaDatainUTF8 = false;
  switch (ienumData)
  {
    case FILE_SOURCE_MD_TITLE:
      ulMetaIndex = TAG_OGG_TITLE;
      break;
    case FILE_SOURCE_MD_VERSION:
      ulMetaIndex = TAG_OGG_VERSION;
      break;
    case FILE_SOURCE_MD_ALBUM:
      ulMetaIndex = TAG_OGG_ALBUM;
      break;
    case FILE_SOURCE_MD_TRACK_NUM:
      ulMetaIndex = TAG_OGG_TRACKNUMBER;
      break;
    case FILE_SOURCE_MD_ARTIST:
      ulMetaIndex = TAG_OGG_ARTIST;
      break;
    case FILE_SOURCE_MD_PERFORMANCE:
      ulMetaIndex = TAG_OGG_PERFORMER;
      break;
    case FILE_SOURCE_MD_COPYRIGHT:
      ulMetaIndex = TAG_OGG_COPYRIGHT;
      break;
    case FILE_SOURCE_MD_DESCRIPTION:
      ulMetaIndex = TAG_OGG_DESCRIPTION;
      break;
    case FILE_SOURCE_MD_GENRE:
      ulMetaIndex = TAG_OGG_GENRE;
      break;
    case FILE_SOURCE_MD_CREATION_DATE:
      ulMetaIndex = TAG_OGG_DATE;
      break;
    case FILE_SOURCE_MD_COMPOSER:
      ulMetaIndex = TAG_OGG_COMPOSER;
    break;
    case FILE_SOURCE_MD_ALBUM_ARTIST:
      ulMetaIndex = TAG_OGG_ALBUMARTIST;
      break;
    default:
      ulMetaIndex = MAX_FIELDNAMES_SUPPORTED;
      break;
  }

  if (MAX_FIELDNAMES_SUPPORTED != ulMetaIndex)
  {
    m_pOGGStreamParser->GetClipMetaData(ulMetaIndex, NULL, &ulMetaDataLen);
    ulMetaDataLen++;
    if ((pucDataBuf) && (*pulDatabufLen >= ulMetaDataLen))
    {
      //! Initialize memory with ZERO
      memset(pucDataBuf, 0, *pulDatabufLen);
      m_pOGGStreamParser->GetClipMetaData(ulMetaIndex, (uint8*)pucDataBuf,
                                          &ulMetaDataLen);
      m_bMetaDatainUTF8 = true;
    }
    else if (pucDataBuf)
    {
      return PARSER_ErrorInsufficientBufSize;
    }
    /* It means there is no metadata field for this enum type. Two characters
       are to store NULL characters at the end. */
    if (2 == ulMetaDataLen)
    {
      ulMetaDataLen = 0;
    }
  }
  *pulDatabufLen = ulMetaDataLen * sizeof(wchar_t);

  return PARSER_ErrorNone;
}

/* ======================================================================
FUNCTION:
  OGGStream::getMovieDuration

DESCRIPTION:
  gets movie duration in movie timescale unit.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint64 OGGStream::getMovieDuration() const
{
  uint64 nDuration = 0;
  if(!m_pOGGStreamParser)
  {
    return nDuration;
  }
  nDuration = getMovieDurationMsec();
  return nDuration;
}
/* ======================================================================
FUNCTION:
  OGGStream::getMovieTimescale

DESCRIPTION:
  gets movie timescale

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 OGGStream::getMovieTimescale() const
{
  uint32 nTimeScale = 1000;
  if(!m_pOGGStreamParser)
  {
    nTimeScale = 0;
  }
  return nTimeScale;
}
/* ======================================================================
FUNCTION:
  OGGStream::getMovieDurationMsec

DESCRIPTION:
  gets movie duration in milli secs

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint64 OGGStream::getMovieDurationMsec() const
{
  uint64 nDuration = 0;
  if(!m_pOGGStreamParser)
  {
    return nDuration;
  }
  nDuration = m_pOGGStreamParser->GetClipDurationInMsec();
  return nDuration;
}
bool OGGStream::GetFlacCodecData(int id,flac_format_data* pData)
{
  bool bRet = false;
#ifdef FEATURE_FILESOURCE_FLAC_PARSER
  if(pData)
  {
    bRet = true;
    (void)MapTrackIdToIndex(&bRet,id);
    if((!bRet) && (m_pOGGStreamParser) )//false means track id is valid
    {
      flac_metadata_streaminfo pstreaminfo;
      if(OGGSTREAM_SUCCESS == m_pOGGStreamParser->GetFlacStreamInfo(id,&pstreaminfo))
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
#endif
  return bRet;
}
/* ======================================================================
FUNCTION:
  OGGStream::GetMaximumBitRateForTrack(uint32 trackId)

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
uint32 OGGStream::getTrackWholeIDList(uint32 *ids)
{
  if(!m_pOGGStreamParser)
  {
    return 0;
  }
  return (m_pOGGStreamParser->GetTrackWholeIDList(ids));
}
/* ======================================================================
FUNCTION:
  OGGStream::getTrackContentVersion

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
  OGGStream::trackRandomAccessDenied

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
  OGGStream::getTrackVideoFrameRate

DESCRIPTION:
  gets track video (if video) frame rate

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
float OGGStream::getTrackVideoFrameRate(uint32 id)
{
  float frate = 0.0;
  if(m_pOGGStreamParser)
  {
    frate = m_pOGGStreamParser->GetVideoFrameRate(id);
  }
  return frate;
}
/* ======================================================================
FUNCTION:
  OGGStream::getTrackVideoFrameWidth

DESCRIPTION:
  returns video track's frame width.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 OGGStream::getTrackVideoFrameWidth(uint32 id)
{
  uint32 width = 0;
  if(m_pOGGStreamParser)
  {
    width = m_pOGGStreamParser->GetVideoWidth(id);
  }
  return width;
}
/* ======================================================================
FUNCTION:
  AVIFile::getTrackVideoFrameHeight

DESCRIPTION:
  returns video track's frame height.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 OGGStream::getTrackVideoFrameHeight(uint32 id)
{
  uint32 height = 0;
  if(m_pOGGStreamParser)
  {
    height = m_pOGGStreamParser->GetVideoHeight(id);
  }
  return height;
}
/* ======================================================================
FUNCTION:
  OGGStream::getTrackMediaDuration

DESCRIPTION:
  gets track duration in track time scale unit

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint64 OGGStream::getTrackMediaDuration(uint32 id)
{
  return getMovieDurationMsec();
}
/* ======================================================================
FUNCTION:
  OGGStream::getTrackMediaTimescale

DESCRIPTION:
  gets track time scale

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 OGGStream::getTrackMediaTimescale(uint32 id)
{
  return 1000;
}
/* ======================================================================
FUNCTION:
  OGGStream::getTrackAudioSamplingFreq

DESCRIPTION:
  gets audio track's sampling frequency

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint32 OGGStream::getTrackAudioSamplingFreq(uint32 id)
{
  uint32 freq = 0;
  if(m_pOGGStreamParser)
  {
    freq = m_pOGGStreamParser->GetAudioSamplingFrequency(id);
  }
  return freq;
}
/* ======================================================================
FUNCTION:
  OGGStream::GetNumAudioChannels

DESCRIPTION:
  returns number of audio channels

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
unsigned long OGGStream::GetNumAudioChannels(int id)
{
  uint32 nchnls = 0;
  if(m_pOGGStreamParser)
  {
    nchnls = m_pOGGStreamParser->GetNumberOfAudioChannels(id);
  }
  return nchnls;
}
/* ======================================================================
FUNCTION:
  OGGStream::peekCurSample

DESCRIPTION:
  gets information about current sample of a track

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
PARSER_ERRORTYPE OGGStream::peekCurSample(uint32 trackid, file_sample_info_type *pSampleInfo)
{
  PARSER_ERRORTYPE reterror = PARSER_ErrorDefault;
  if((!m_pOGGStreamParser) || (!pSampleInfo))
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL,"OGGStream::peekCurSample invalid argument");
  }
  else
  {
    bool bError = false;
    uint8 index = MapTrackIdToIndex(&bError,trackid);
    if(!bError)
    {
      *pSampleInfo = m_sampleInfo[index];
      m_sampleInfo[index].nGranule = 0;
      reterror = PARSER_ErrorNone;
    }
  }
  return reterror;
}
/* ======================================================================
FUNCTION:
  OGGStream::getTrackOTIType

DESCRIPTION:
  gets track OTI value

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint8 OGGStream::getTrackOTIType(uint32 id)
{
  uint8 format = 0xFF;
  ogg_media_codec_type codec_type;

  if(m_pOGGStreamParser)
  {
    //Convert trackID to SerialNum
    id = m_pOGGStreamParser->GetTrackSerialNo(id);
    codec_type = m_pOGGStreamParser->GetTrackType(id);
    switch(codec_type)
    {
      case OGG_AUDIO_CODEC_VORBIS:
      {
        format = (uint8)VORBIS_AUDIO;
      }
      break;
      case OGG_AUDIO_CODEC_FLAC:
      {
        format = (uint8)FLAC_AUDIO;
      }
      break;
#ifdef FEATURE_FILESOURCE_OGG_THEORA_CODEC
      case OGG_VIDEO_CODEC_THEORA:
      {
        format = (uint8)THEORA_VIDEO;
      }
      break;
#endif
      default:
      break;
    }
  }
  return format;
}
/* ======================================================================
FUNCTION:
  OGGStream::getTrackMaxBufferSizeDB

DESCRIPTION:
  gets maximum buffer size to play the track

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
int32  OGGStream::getTrackMaxBufferSizeDB(uint32 id)
{
  int32 bufsize = MAX_PAGE_SIZE;
  //ogg_media_codec_type codec_type;
  if(!m_pOGGStreamParser)
  {
    return 0;
  }
  else
  {
      bufsize = m_pOGGStreamParser->GetTrackMaxBufferSize(id);
  }
  return bufsize;
}
/* ======================================================================
FUNCTION:
  OGGStream::getTrackAverageBitrate

DESCRIPTION:
  gets track's average bitrate

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 average bitrate

SIDE EFFECTS:
  None.
======================================================================*/
int32 OGGStream::getTrackAverageBitrate(uint32 id)
{
  uint32 bitrate = 0;
  if(m_pOGGStreamParser)
  {
    bitrate = m_pOGGStreamParser->GetTrackAverageBitRate(id);
  }
  return bitrate;
}

/* ======================================================================
FUNCTION:
  OGGStream::getTrackAverageBitrate

DESCRIPTION:
  gets track's average bitrate

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 average bitrate

SIDE EFFECTS:
  None.
======================================================================*/
int32 OGGStream::getTrackMinBitrate(uint32 id)
{
  uint32 bitrate = 0;
  if(m_pOGGStreamParser)
  {
    bitrate = m_pOGGStreamParser->GetTrackMinBitRate(id);
  }
  return bitrate;
}

/* ======================================================================
FUNCTION:
  OGGStream::getTrackAverageBitrate

DESCRIPTION:
  gets track's average bitrate

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 average bitrate

SIDE EFFECTS:
  None.
======================================================================*/
int32 OGGStream::getTrackMaxBitrate(uint32 id)
{
  uint32 bitrate = 0;
  if(m_pOGGStreamParser)
  {
    bitrate = m_pOGGStreamParser->GetTrackMaxBitRate(id);
  }
  return bitrate;
}

/* ======================================================================
FUNCTION:
  OGGStream::getTrackDecoderSpecificInfoContent

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
PARSER_ERRORTYPE OGGStream::getTrackDecoderSpecificInfoContent(uint32 id, uint8* buf, uint32 *pbufSize)
{
  PARSER_ERRORTYPE errorCode = PARSER_ErrorDefault;
  if( (!m_pOGGStreamParser) || (!pbufSize) )
  {
    return errorCode;
  }
  uint32 size = m_pOGGStreamParser->GetCodecHeaderSize(id);
  uint8* header = m_pOGGStreamParser->GetCodecHeader(id);
  if(buf && (*pbufSize >= size) && size && header)
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
  OGGStream::MapTrackIdToIndex

DESCRIPTION:
  Maps given track-id to track index.

INPUT/OUTPUT PARAMETERS:
  None.

RETURN VALUE:
 none

SIDE EFFECTS:
  None.
======================================================================*/
uint8 OGGStream::MapTrackIdToIndex(bool* bError,uint32 trackid)
{
  uint8 index = 0;
  if(bError)
  {
    *bError = true;
    for(uint32 i = 0; i < m_nNumStreams; i++)
    {
      if( (m_pIndTrackIdTable[i].trackId == trackid) &&
        (m_pIndTrackIdTable[i].bValid) )
      {
        index = m_pIndTrackIdTable[i].index;
        *bError = false;
        break;
      }
    }
  }
  if(bError && *bError)
  {
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_FATAL,
                "OGGStream::MapTrackIdToIndex failed for trackid %lu",trackid);
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
//void OGGStream::sendParserEvent(ParserStatusCode status)
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
////void OGGStream::sendParseHTTPStreamEvent(void)
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
////void OGGStream::updateBufferWritePtr ( uint32 writeOffset )
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
////tWMCDecStatus OGGStream::getMetaDataSize ( void )
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
////bool OGGStream::parseHTTPStream ( void )
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
////     {
////           //QTV_PS_PARSER_STATUS_RESUME
////           bIsMetaDataParsed = TRUE;
////           m_HttpDataBufferMinOffsetRequired.bValid = FALSE;
////           sendParserEvent(Common::PARSER_RESUME);
////           returnStatus = true;
////           MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "Parser State = Common::PARSER_RESUME");
////         }
////     else
////     {
////           //QTV_PS_PARSER_STATUS_PAUSED
////           sendParserEvent(Common::PARSER_PAUSE);
////           MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "Parser State = Common::PARSER_PAUSE");
////           returnStatus = false;
////     }
////       }
////     }
////
////     if ((parserState == Common::PARSER_RESUME) && CanPlayTracks(m_startupTime) )
////     {
////        //QTV_PS_PARSER_STATUS_READY
////        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "Parser State = Common::PARSER_READY");
////        sendParserEvent(Common::PARSER_READY);
////    returnStatus = true;
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
////void OGGStream::sendHTTPStreamUnderrunEvent(void)
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
////boolean OGGStream::GetHTTPStreamDownLoadedBufferOffset(U32_WMC * pOffset)
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
//bool OGGStream::GetTotalAvgBitRate(uint32 * pBitRate)
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
//bool OGGStream::CanPlayTracks(uint32 nTotalPBTime)
//{
//    return true;
//}
//
///* ======================================================================
//FUNCTION:
//  OGGStream::GetMediaMaxTimeStampPlayable
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
////tWMCDecStatus OGGStream::GetMediaMaxPlayableTime(U32_WMC *nMaxPBTime)
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
#endif //FEATURE_FILESOURCE_OGG_PARSER
