#ifndef _OGG_STREAM_H
#define _OGG_STREAM_H
/* =======================================================================
                              OGGStream.h
DESCRIPTION
  Meaningful description of the definitions contained in this file.
  Description must specify if the module is portable specific, mobile
  specific, or common to both, and it should alert the reader if the
  module contains any conditional definitions which tailors the module to
  different targets.

Copyright 2009-2013 QUALCOMM Technologies Incorporated, All Rights Reserved.
QUALCOMM Technologies Proprietary and Confidential.
========================================================================== */
/* =======================================================================
                             PERFORCE HEADER
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/OGGParserLib/main/latest/inc/OGGStream.h#21 $
========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================
                    INCLUDE FILES FOR MODULE
========================================================================== */
#include "AEEStdDef.h"
#include "filebase.h"
#include "filesourcestring.h"
#include "oscl_file_io.h"
#include "OGGStreamDataDef.h"

#ifdef FEATURE_FILESOURCE_OGG_PARSER
/* ==========================================================================
                       DATA DECLARATIONS
========================================================================== */

/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Forward Declarations
** ----------------------------------------------------------------------- */
class OGGStreamParser;
/* -----------------------------------------------------------------------
** Global Constant Data Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Data Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                          Function Declaration
** ======================================================================= */

/* =======================================================================
**                        Class Declarations
** ======================================================================= */

/* ======================================================================
CLASS
  OGGStream

DESCRIPTION
  Thorough, meaningful description of what this function does.

DEPENDENCIES
  List any dependencies for this function, global variables, state,
  resource availability, etc.

RETURN VALUE
  Enumerate possible return values

SIDE EFFECTS
  Detail any side effects.

========================================================================== */
class OGGStream : public FileBase, public Parentable
{
public:

  OGGStream(const FILESOURCE_STRING filename, unsigned char *pFileBuf = NULL,
            uint32 bufSize = 0 ,bool bPlayVideo = false, bool bPlayAudio = true);
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
  OGGStream(video::iStreamPort*,bool bPlayVideo = false, bool bPlayAudio = true);
#endif
  virtual ~OGGStream();
  virtual uint32 FileGetData(uint64, uint32, uint32, uint8*);
  virtual bool ParseMetaData();

  virtual FileSourceStatus GetFileFormat(FileSourceFileFormat& fileFormat)
  {
    fileFormat = FILE_SOURCE_OGG;
    return FILE_SOURCE_SUCCESS;
  }
  virtual uint64 GetFileSize() {return m_fileSize;};
  virtual PARSER_ERRORTYPE getNextMediaSample(uint32 ulTrackID, uint8 *pucDataBuf,
                                              uint32 *pulBufSize, uint32 &rulIndex);
  virtual uint64 getMediaTimestampForCurrentSample(uint32 id);

  virtual uint64 resetPlayback(uint64 repos_time, uint32 id, bool bSetToSyncSample,
                               bool *bError, uint64 currentPosTimeStamp);

  virtual uint8 randomAccessDenied(){return 0;}

  virtual PARSER_ERRORTYPE  GetClipMetaData(wchar_t *pucDataBuf,
                                            uint32 *pulDatabufLen,
                                            FileSourceMetaDataType ienumData);

  // Methods to get the sample rate (i.e. timescales) for the streams and
  // the overall Movie presentation
  virtual uint64 getMovieDuration() const;
  virtual uint32 getMovieTimescale() const;
  virtual uint64 getMovieDurationMsec() const;

  // From Track
  virtual int32 getNumTracks(){return m_nNumStreams;}
  virtual uint32 getTrackWholeIDList(uint32 *ids);

  virtual float  getTrackVideoFrameRate(uint32 id);
  virtual uint32 getTrackVideoFrameWidth(uint32 id);
  virtual uint32 getTrackVideoFrameHeight(uint32 id);

  // From MediaHeader
  virtual uint64 getTrackMediaDuration(uint32 id);
  virtual uint32 getTrackMediaTimescale(uint32 id);
  virtual uint32 getTrackAudioSamplingFreq(uint32 id);

  virtual PARSER_ERRORTYPE peekCurSample(uint32 trackid,
                                         file_sample_info_type *pSampleInfo);

  virtual uint8  getTrackOTIType(uint32 id); // Based on OTI value
  virtual int32  getTrackAverageBitrate(uint32 id);
  virtual int32  getTrackMinBitrate(uint32 id);
  virtual int32  getTrackMaxBitrate(uint32 id);
  virtual int32  getTrackMaxBufferSizeDB(uint32 id);
  virtual unsigned long GetNumAudioChannels(int id);

  // this returns codec specific header and size //
  virtual PARSER_ERRORTYPE getTrackDecoderSpecificInfoContent(uint32 id,
                                                             uint8* buf,
                                                             uint32 *pbufSize);

  virtual uint64 getFileSize(){return m_fileSize;}
  virtual uint8  MapTrackIdToIndex(bool*,uint32);

  virtual void SetCriticalSection(MM_HANDLE);
  virtual bool GetFlacCodecData(int id,flac_format_data* pData);

  virtual bool isMetaDatainUTF8(){return m_bMetaDatainUTF8;};

private:

  void                    InitData();
  bool                    m_playAudio;
  bool                    m_playVideo;
  bool                    m_bMetaDatainUTF8;

  // if we are streaming, rather than playing locally
  bool                    m_bStreaming;
  uint32                  m_nNumStreams;

  file_sample_info_type   m_sampleInfo[FILE_MAX_MEDIA_STREAMS];
  OggTrackIdToIndexTable* m_pIndTrackIdTable;

  uint64                  m_fileSize;
  OSCL_FILE*              m_pFilePtr;

  // OGG parser handle
  OGGStreamParser*    m_pOGGStreamParser;
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
  video::iStreamPort* m_pPort;
#endif
//TO DO WHEN HTTP SUPPORT IS TO BE ADDED
//#if defined(FEATURE_QTV_PSEUDO_STREAM) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
//   uint32 m_minOffsetRequired;
//   bool    bHttpStreaming;
//   boolean bGetMetaDataSize;
//   boolean bIsMetaDataParsed;
//   ParserStatusCode parserState;
//   uint32  m_startupTime;
//   uint32 m_maxPlayableTime[FILE_MAX_MEDIA_STREAMS];
//
//   struct tHttpDataOffset
//   {
//      uint32 Offset;
//      boolean bValid;
//   } m_HttpDataBufferMinOffsetRequired;
//
//   FetchBufferedDataSizeT m_fpFetchBufferedDataSize;
//   FetchBufferedDataT m_fpFetchBufferedData;
//   InstanceHandleT m_QtvInstancehandle;
//
//   void sendParserEvent(ParserStatusCode status);
//   void sendParseHTTPStreamEvent(void);
//
//   // virtual void updateBufferWritePtr ( uint32 writeOffset );
//   //tWMCDecStatus getMetaDataSize ( void );
//
//   virtual bool CanPlayTracks(uint32 pbTime);
//   // virtual bool parseHTTPStream ( void );
//
//   void sendHTTPStreamUnderrunEvent(void);
//   boolean GetHTTPStreamDownLoadedBufferOffset(uint32 *pOffset);
//   bool GetTotalAvgBitRate(uint32 * pBitRate);
//   //tWMCDecStatus GetMediaMaxPlayableTime(U32_WMC *nMaxPBTime);
//
//#endif //#if defined(FEATURE_QTV_PSEUDO_STREAM) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
};
#endif //FEATURE_FILESOURCE_OGG_PARSER
#endif//#ifndef _OGG_STREAM_H

