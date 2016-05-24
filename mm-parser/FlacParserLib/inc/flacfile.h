#ifndef _FLAC_FILE_H
#define _FLAC_FILE_H
/* =======================================================================
                              flacfile.h
DESCRIPTION
  Meaningful description of the definitions contained in this file.
  Description must specify if the module is portable specific, mobile
  specific, or common to both, and it should alert the reader if the
  module contains any conditional definitions which tailors the module to
  different targets.

  Copyright(c) 2009-2013 by Qualcomm Technologies, Inc. All Rights Reserved
  Qualcomm Technologies Proprietary and Confidential.
========================================================================== */
/* =======================================================================
                             PERFORCE HEADER
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/FlacParserLib/main/latest/inc/flacfile.h#16 $
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
#include "flacfileDataDef.h"

#ifdef FEATURE_FILESOURCE_FLAC_PARSER
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
class FlacParser;
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
  flacfile

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
class flacfile : public FileBase, public Parentable
{
public:

  flacfile(const FILESOURCE_STRING filename, unsigned char *pFileBuf = NULL,
           uint64 bufSize = 0);
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
  flacfile(video::iStreamPort*);
#endif
  virtual ~flacfile();
  virtual uint32 FileGetData(uint64, uint32, uint32, uint8*);
  virtual bool ParseMetaData();

  virtual FileSourceStatus GetFileFormat(FileSourceFileFormat& fileFormat)
  {
    fileFormat = FILE_SOURCE_FLAC;
    return FILE_SOURCE_SUCCESS;
  }
  virtual uint64 GetFileSize() {return m_fileSize;};
  virtual PARSER_ERRORTYPE getNextMediaSample(uint32 id, uint8 *buf,
                                              uint32 *size, uint32 &index);
  virtual uint64 getMediaTimestampForCurrentSample(uint32 id);

  virtual uint64 resetPlayback(uint64 repos_time, uint32 id, bool bSetToSyncSample,
                               bool *bError, uint64 currentPosTimeStamp);

  virtual uint8 randomAccessDenied(){return 0;}

  virtual FILESOURCE_STRING getTitle() const;
  virtual FILESOURCE_STRING getAuthor() const;
  virtual FILESOURCE_STRING getDescription() const;
  virtual FILESOURCE_STRING getRating() const;
  virtual FILESOURCE_STRING getCopyright() const;
  virtual FILESOURCE_STRING getVersion() const;
  virtual FILESOURCE_STRING getCreationDate() const;
  virtual FILESOURCE_STRING getPerf()const ;
  virtual FILESOURCE_STRING getGenre()const;

  // Methods to get the sample rate (i.e. timescales) for the streams and
  // the overall Movie presentation
  virtual uint64 getMovieDuration() const;
  virtual uint32 getMovieTimescale() const;
  virtual uint64 getMovieDurationMsec() const;

  // From Track
  virtual int32 getNumTracks(){return m_nNumStreams;}
  virtual uint32 getTrackWholeIDList(uint32 *ids);

  // From MediaHeader
  virtual uint64 getTrackMediaDuration(uint32 id);
  virtual uint32 getTrackMediaTimescale(uint32 id);
  virtual uint32 getTrackAudioSamplingFreq(uint32 id);

  virtual PARSER_ERRORTYPE peekCurSample(uint32 trackid,
                                         file_sample_info_type *pSampleInfo);

  virtual uint8  getTrackOTIType(uint32 id); // Based on OTI value
  virtual int32  getTrackAverageBitrate(uint32 id);
  virtual int32  getTrackMaxBufferSizeDB(uint32 id);

  virtual unsigned long GetAudioBitsPerSample(int id);
  virtual unsigned long GetNumAudioChannels(int id);

  // this returns codec specific header and size //
  virtual PARSER_ERRORTYPE getTrackDecoderSpecificInfoContent(uint32 id,
                                                             uint8* buf,
                                                             uint32 *pbufSize);

  virtual uint64 getFileSize(){return m_fileSize;}
  virtual uint8  MapTrackIdToIndex(bool*,uint32);
  virtual void SetCriticalSection(MM_HANDLE);
  virtual bool GetFlacCodecData(int id,flac_format_data* pData);

private:
  FlacTrackIdToIndexTable* m_pFlacIndTrackIdTable;
  void                    InitData();
  bool                    m_playAudio;
  bool                    m_corruptFile;
  // if we are streaming, rather than playing locally
  bool                    m_bStreaming;
  uint32                  m_nNumStreams;
  // These variable are added to know the largest size of audio and video samples respectively//
  uint32                  m_audioLargestSize;
  file_sample_info_type   m_sampleInfo[FILE_MAX_MEDIA_STREAMS];
  uint32                  m_nDecodedDataSize[FILE_MAX_MEDIA_STREAMS];
  uint32                  m_nLargestFrame[FILE_MAX_MEDIA_STREAMS];

  //only one of "m_pFileBuf" or "m_filename" can be non-zero
  FILESOURCE_STRING     m_filename;  // EFS file path //
  unsigned char     *m_pFileBuf;  // pointer to buffer for playback from memory //

  uint64          m_FileBufSize;
  uint64          m_fileSize;
  OSCL_FILE       *m_pFilePtr;

  // FLAC parser handle
  FlacParser*    m_pFlacParser;
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
#endif //#ifdef FEATURE_FILESOURCE_FLAC
#endif//#ifndef _FLAC_FILE_H

