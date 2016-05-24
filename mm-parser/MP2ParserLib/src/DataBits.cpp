/* =======================================================================
                              DataBits.cpp
DESCRIPTION
  Meaningful description of the definitions contained in this file.
  Description must specify if the module is portable specific, mobile
  specific, or common to both, and it should alert the reader if the
  module contains any conditional definitions which tailors the module to
  different targets.

Copyright (c) 2009-2013 Qualcomm Technologies, Inc.  All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             PERFORCE HEADER
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/MP2ParserLib/main/latest/src/DataBits.cpp#30 $
========================================================================== */
#include "MP2StreamParser.h"
#include "string.h"
#include "MMDebugMsg.h"
#include "MMMemory.h"
#include "MMMalloc.h"
#include "MP2Stream.h"

#include "math.h"
/*! ======================================================================
@brief  getBytesValue

@detail    Returns the value associated with nBytes located in Data

@param[in] N/A

@return    Value associated with nBytes else returns 0
@note      None.
========================================================================== */
uint32 MP2StreamParser::getBytesValue(int nBytes,uint8* Data)
{
  uint32 val = 0;
  int itr = 0;
  while(Data && (nBytes > 0) && (nBytes <= (int)sizeof(uint32)) )
  {
    val = val <<8;
    val += Data[itr++];
    nBytes--;
  }
  return val;
}
/*! ======================================================================
@brief  make33BitValue

@detail    Returns the 33 bit value.Various bits fragments are passed in which are
        used to construct the value
@param[in] N/A

@return    Value associated with nBytes else returns 0
@note      None.
========================================================================== */
uint64 MP2StreamParser::make33BitValue(uint8 ms3bits,uint16 middle15bits,uint16 ls15bits)
{
  uint64 result = 0;

  //first copy over most significant 3 bits as we construct the number by <<

  for(int i = 0; i <= 2; i++)
  {
    result <<= 1;
    if(ms3bits & 0x80)
    {
      result = result | (uint64)0x01;
    }
    else
    {
      result = result | (uint64)0x00;
    }
    ms3bits <<= 1;
  }
  //Now copy remaining 30 bits
  for(int i = 0; i <= 14; i++)
  {
    result <<= 1;
    if(middle15bits & (uint16)0x4000)
    {
      result = result | (uint64)0x01;
    }
    else
    {
      result = result | (uint64)0x00;
    }
    middle15bits <<= 1;
  }
  for(int i = 0; i <= 14; i++)
  {
    result <<= 1;
    if(ls15bits & (uint16)0x4000)
    {
      result = result | (uint64)0x01;
    }
    else
    {
      result = result | (uint64)0x00;
    }
    ls15bits <<= 1;
  }
  return result;
}
/*! ======================================================================
@brief  make15BitValue

@detail    Returns a 15 value.Various bits fragments are passed in which are
        used to construct the value.
@param[in] N/A

@return    Value associated with nBytes else returns 0
@note      None.
========================================================================== */
uint16 MP2StreamParser::make15BitValue(uint8 topbits,uint16 lowerbits)
{
  uint16 result = 0;
  //first copy over most significant 2 bits as we construct the number by <<
  for(int i = 0; i <= 1; i++)
  {
    result <<= 1;
    if(topbits & 0x80)
    {
      result = result | (uint16)0x01;
    }
    else
    {
      result = result | (uint16)0x00;
    }
    topbits <<= 1;
  }
  //Now copy remaining 13 bits
  for(int i = 0; i <= 12; i++)
  {
    result <<= 1;
    if(lowerbits & 0x8000)
    {
      result = result | (uint16)0x01;
    }
    else
    {
      result = result | (uint16)0x00;
    }
    lowerbits <<= 1;
  }
  return result;
}

/*! ======================================================================
@brief  make9BitValue

@detail    Returns a 9 bit value.Various bits fragments are passed in which are
        used to construct the value.
@param[in] N/A

@return    Value associated with nBytes else returns 0
@note      None.
========================================================================== */
uint16 MP2StreamParser::make9BitValue(uint8 topbits,uint8 lowerbits)
{
  uint16 result = 0;
  //first copy over most significant 2 bits as we construct the number by <<
  for(int i = 0; i <= 1; i++)
  {
    result <<= 1;
    if(topbits & 0x80)
    {
      result = result | (uint16)0x01;
    }
    else
    {
      result = result | (uint16)0x00;
    }
    topbits <<= 1;
  }
  //Now copy remaining 7 bits
  for(int i = 0; i <= 6; i++)
  {
    result <<= 1;
    if(lowerbits & 0x80)
    {
      result = result | (uint16)0x01;
    }
    else
    {
      result = result | (uint16)0x00;
    }
    lowerbits <<= 1;
  }
  return result;
}

/*! ======================================================================
@brief  make22BitValue

@detail    Returns the 22 bit value.Various bits fragments are passed in which are
        used to construct the value
@param[in] N/A

@return    Value associated with nBytes else returns 0
@note      None.
========================================================================== */
uint32 MP2StreamParser::make22BitValue(uint16 part1,uint8 part2)
{
  uint32 result = 0;
  //first copy over most significant 15 bits as we construct the number by <<
  for(int i = 0; i <= 14; i++)
  {
    result <<= 1;
    if(part1 & 0x8000)
    {
      result = result | (uint32)0x01;
    }
    else
    {
      result = result | (uint32)0x00;
    }
    part1 <<= 1;
  }
  //Now copy remaining 7 bits
  for(int i = 0; i <= 6; i++)
  {
    result <<= 1;
    if(part2 & 0x80)
    {
      result = result | (uint32)0x01;
    }
    else
    {
      result = result | (uint32)0x00;
    }
    part2 <<= 1;
  }
  return result;
}
/*! ======================================================================
@brief  make42BitValue

@detail    Returns the 42 bit value.Various bits fragments are passed in which are
        used to construct the value
@param[in] N/A

@return    Value associated with nBytes else returns 0
@note      None.
========================================================================== */
uint64 MP2StreamParser::make42BitValue(uint64 part1, uint16 part2)
{
  uint64 result = 0;
  //first copy over most significant 16 bits as we construct the number by <<
  for(int i = 0; i <= 32; i++)
  {
    result <<= 1;
    if(part1 & 0x8000000000000000ULL)
    {
      result = result | (uint64)0x01;
    }
    else
    {
      result = result | (uint64)0x00;
    }
    part1 <<= 1;
  }
  //Now copy remaining bits
  for(int i = 0; i <= 8; i++)
  {
    result <<= 1;
    if(part2 & 0x8000)
    {
      result = result | (uint64)0x01;
    }
    else
    {
      result = result | (uint64)0x00;
    }
    part2 <<= 1;
  }
  return result;
}
/*===========================================================================
FUNCTION:
  getByteFromBitStream

DESCRIPTION:
  This function gets given number of bits from the given bit of source in the
  destination byte.

INPUT/OUTPUT PARAMETERS:
  uint8   *pByte      destination byte
  uint8   *pSrc       source stream of bits
  int      nFromBit   from which bit of source
  int      nBits      number of bits to copy in byte (it should not be more than 8)

  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
void MP2StreamParser::getByteFromBitStream(uint8 *pByte, uint8 *pSrc, int nFromBit, int nBits)
{
  int a, b, i;
  uint8 temp;
  *pByte = 0;       /* reset all the bits */
  if(nBits <=8 )
  {
    for( i=0; i < nBits; i++)
    {
      a = nFromBit/8;
      b = nFromBit%8;
      *pByte = *pByte << 1;     /* make space for next bit */
      temp = pSrc[a] << b;
      *pByte |= temp >> 7;      /* OR after masking all other bits */
      nFromBit++;
    }
  }
}
/*===========================================================================
FUNCTION:
  readMpeg2StreamData

DESCRIPTION:
  This API invokes the callback function to read the data into the parser.
  Parser does not implement callback function, it's the responsibility
  of the parser's client to implement actual reading of the data.
  This API helps in centralizing all the read and facilitate setting parser
  state especially when reaad fails because of data under-run(http streaming)

INPUT/OUTPUT PARAMETERS:
  nOffset           Offset to read from
  nNumBytesRequest  Number of bytes to read
  pData             Buffer to store read data
  nMaxSize          Maximum size of the buffer
  u32UserData       Userdata passed in while instantiating parser object

  None.

RETURN VALUE:
  None.

SIDE EFFECTS:
  None.
===========================================================================*/
uint32  MP2StreamParser::readMpeg2StreamData (uint64 ullOffset, uint32 nNumBytesRequest,
                                              unsigned char* pData,
                                              uint32  nMaxSize, uint32  u32UserData )
{
  uint32 nBytesRead =0;
  if (( m_ullCurrentStart<= ullOffset) &&
      (m_ullCurrentEnd >= (ullOffset+nNumBytesRequest)))
  {
#ifdef MPEG2_PARSER_DEBUG
    MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_HIGH,
      "MP2StreamParser::readMpeg2StreamData m_ullCurrentEnd=%llu, m_ullCurrentStart=%llu",
      m_ullCurrentEnd,m_ullCurrentStart);
#endif
    nBytesRead = FILESOURCE_MIN(nNumBytesRequest, nMaxSize);
    memcpy(pData, &m_pReadBuffer[ullOffset - m_ullCurrentStart], nBytesRead);
  }
  else
  {
    if( (m_nCurrOffset + MPEG2_FILE_READ_CHUNK_SIZE) > m_nFileSize)
    {
      nBytesRead = MP2StreamCallbakGetData(ullOffset,
      nNumBytesRequest, m_pReadBuffer, nMaxSize,
      (uint32)u32UserData);
    }
    else
    {
      nBytesRead = MP2StreamCallbakGetData(ullOffset,
        MPEG2_FILE_READ_CHUNK_SIZE, m_pReadBuffer, nMaxSize,
        (uint32)u32UserData);
    }
    if(nBytesRead != 0)
    {
#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
        "Read %d bytes from file ",nBytesRead);
#endif
      m_ullCurrentStart = ullOffset;
      m_ullCurrentEnd = ullOffset + nBytesRead;
      if (nBytesRead >= nNumBytesRequest)
      {
        nBytesRead = nNumBytesRequest;
      }
      nBytesRead = FILESOURCE_MIN(nBytesRead, nMaxSize);
      memcpy(pData, m_pReadBuffer, nBytesRead);
    }
  }
  if(m_bMediaAbort && !nBytesRead)
  {
    m_eParserState = MP2STREAM_READ_ERROR;
    MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
        "user set Media abort, m_availOffset %llu",m_availOffset);
    return 0;
  }
  if((!nBytesRead ) || (nBytesRead != nNumBytesRequest))
  {
#ifdef FEATURE_FILESOURCE_3GPP_PROGRESSIVE_DNLD
    if(m_bHttpStreaming)
    {
      uint64 availOffset = 0;
      boolean bEnd = false;
      MP2Stream* pMP2TStream = (MP2Stream*)m_pUserData;
#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
          "readMpeg2StreamData:Incomplete offset %llu bytesRequested %lu nBytesRead %lu",
          ullOffset,nNumBytesRequest, nBytesRead);
#endif
      nBytesRead = 0;
      (void)pMP2TStream->CheckAvailableDataSize(&availOffset,&bEnd);
      m_availOffset = availOffset;
      if(!bEnd)
      {
        m_eParserState = MP2STREAM_DATA_UNDER_RUN;
#ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
            "readMpeg2StreamData underrun offset %llu bytesRequested %lu availOffset %llu",
            ullOffset,nNumBytesRequest, availOffset);
#endif
      }
      else
      {
#ifdef MPEG2_PARSER_DEBUG
        MM_MSG_PRIO3(MM_FILE_OPS, MM_PRIO_HIGH,
            "readMpeg2StreamData EOF offset %llu bytesRequested %lu availOffset %llu",
            ullOffset,nNumBytesRequest, availOffset);
#endif
        m_eParserState = MP2STREAM_EOF;
      }
    }
    else
#endif
    {
      m_eParserState = MP2STREAM_READ_ERROR;
#ifdef MPEG2_PARSER_DEBUG
      MM_MSG_PRIO2(MM_FILE_OPS, MM_PRIO_FATAL,
          "readMpeg2StreamData read Failed offset %llu bytesRequested %lu",
          ullOffset,nNumBytesRequest);
#endif
    }
  }
  return nBytesRead;
}
