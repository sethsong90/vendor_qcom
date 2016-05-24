/************************************************************************* */
/**
 * @file
 * @brief implementation for the parsing of the meta data for video formats
 * like H264 and MPEG4
 * 
 * Copyright 2011-2012 Qualcomm Technologies, Inc., All Rights Reserved.
 *
 ************************************************************************* */
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/FileBaseLib/main/latest/src/H264HeaderParser.cpp#6 $
$DateTime: 2012/03/07 21:17:30 $
$Change: 2256197 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* =======================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
/* Streamer library files */
#include "H264HeaderParser.h"
//#include "IPStreamSourceUtils.h"
#include "parserdatadef.h"
#include "MMDebugMsg.h"
#include "MMMalloc.h"
#include <string.h>

/* =======================================================================
**                         DATA DEFINATIONS
** ======================================================================= */

/* =======================================================================
**                        Class & Function Declarations
** ======================================================================= */

/******************************************************************************
 ** This class is used to convert an H.264 NALU (network abstraction layer
 ** unit) into RBSP (raw byte sequence payload) and extract bits from it.
 *****************************************************************************/
class H264HeaderRbspParser
{
public:
  H264HeaderRbspParser (const uint8 *begin, const uint8 *end);

  virtual ~H264HeaderRbspParser ();

  uint32 next ();
  void advance ();
  uint32 u (uint32 n);
  uint32 ue ();
  int32 se ();

private:
  const uint8 *begin, *end;
  int32 pos;
  uint32 bit;
  uint32 cursor;
  bool advanceNeeded;

};


H264HeaderRbspParser::H264HeaderRbspParser (const uint8 *_begin, const uint8 *_end)
: begin (_begin), end(_end), pos (- 1), bit (0),
cursor (0xFFFFFF), advanceNeeded (true)
{
}


// Destructor
/*lint -e{1540}  Pointer member neither freed nor zeroed by destructor
 * No problem
 */
H264HeaderRbspParser::~H264HeaderRbspParser () {}

// Return next RBSP byte as a word
uint32 H264HeaderRbspParser::next ()
{
  if (advanceNeeded) advance ();
  //return static_cast<uint32> (*pos);
  return static_cast<uint32> (begin[pos]);
}

// Advance RBSP decoder to next byte
void H264HeaderRbspParser::advance ()
{
  ++pos;

  //if (pos >= stop)
  if (begin + pos == end)
  {
    return;
  }
  cursor <<= 8;
  //cursor |= static_cast<uint32> (*pos);
  cursor |= static_cast<uint32> (begin[pos]);
  if ((cursor & 0xFFFFFF) == 0x000003)
  {
    advance ();
  }
  advanceNeeded = false;
}

// Decode unsigned integer
uint32 H264HeaderRbspParser::u (uint32 n)
{
  uint32 i, s, x = 0;
  for (i = 0; i < n; i += s)
  {
    s = static_cast<uint32>FILESOURCE_MIN(static_cast<int>(8 - bit), 
                                   static_cast<int>(n - i));
    x <<= s;

    x |= ((next () >> ((8 - static_cast<uint32>(bit)) - s)) & 
          ((1 << s) - 1));

    bit = (bit + s) % 8;
    if (!bit)
    {
      advanceNeeded = true;
    }
  }
  return x;
}

// Decode unsigned integer Exp-Golomb-coded syntax element
uint32 H264HeaderRbspParser::ue ()
{
  int leadingZeroBits = -1;
  for (uint32 b = 0; !b; ++leadingZeroBits)
  {
    b = u (1);
  }
  return((1 << leadingZeroBits) - 1) + u (static_cast<uint32>(leadingZeroBits));
}

// Decode signed integer Exp-Golomb-coded syntax element
int32 H264HeaderRbspParser::se ()
{
  const uint32 x = ue ();
  if (!x) return 0;
  else if (x & 1) return static_cast<int32> ((x >> 1) + 1);
  else return - static_cast<int32> (x >> 1);
}

H264HeaderParser::H264HeaderParser()
{
  m_streamInfo.m_index = -1;
  for (int i=0; i<MAX_SETS; i++)
  {
    std_memset(&m_streamInfo.m_seq[i], 0x00, sizeof(m_streamInfo.m_seq[i]));
    std_memset(&m_streamInfo.m_pic[i], 0x00, sizeof(m_streamInfo.m_pic[i]));
    m_streamInfo.m_seq[i].seqSetID = -1;
    m_streamInfo.m_seq[i].picSetID = -1;
    m_streamInfo.m_pic[i].picSetID = -1;
    m_streamInfo.m_pic[i].seqSetID = -1;
  }
}

H264HeaderParser::~H264HeaderParser()
{
  for (int i=0; i<MAX_SETS; i++)
  {
    if (m_streamInfo.m_seq[i].nalu)
    {
      MM_Free(m_streamInfo.m_seq[i].nalu);
      m_streamInfo.m_seq[i].nalu = NULL;
    }
    if (m_streamInfo.m_pic[i].nalu)
    {
      MM_Free(m_streamInfo.m_pic[i].nalu);
      m_streamInfo.m_pic[i].nalu = NULL;
    }
  }
}

/*
 * @brief Parase the H264 parameter sets and populates the H264StreamInfo
 *
 * param[in] encodedBytes, the paramter set
 * param[in] totalBytes, the length of the encoded bytes
 */
void H264HeaderParser::parseParameterSet(const unsigned char *encodedBytes,
                                         int totalBytes)
{
  int i = 0;
  // Determine NALU type.
  uint8 naluType = (encodedBytes [0] & 0x1F);

  // Process sequence and parameter set NALUs specially.
  if ((naluType == 7) || (naluType == 8))
  {
    // Parse parameter set ID and other stream information.
    H264ParamNalu newParam;
    //initialize newParam
    memset(&newParam,0,sizeof(newParam));
    H264HeaderRbspParser rbsp (&encodedBytes [1],
                     &encodedBytes [totalBytes]);
    uint32 id;
    if (naluType == 7)
    {
      uint8 profile_idc;
      uint8 chroma_idc = 1;
      uint8 tmp;

      profile_idc = (uint8)rbsp.u(8);

      (void) rbsp.u (16);
      id = newParam.seqSetID = rbsp.ue ();

      if(100 == profile_idc || 110 == profile_idc ||
         122 == profile_idc || 244 == profile_idc ||
          83 == profile_idc ||  86 == profile_idc )
      {
        //chroma_format_idc[ue(v)]
        chroma_idc = (uint8)rbsp.ue();
        if(3 == chroma_idc) rbsp.u(1);
        //skip bit_depth_luma_minus8/chroma_minus8
        (void) rbsp.ue();
        (void) rbsp.ue();
        //qpprime_y_zero_transform_bypass_flag[u(1)]
        tmp = (uint8)rbsp.u(1);
        //seq_scaling_matrix_present_flag[u(1)]
        tmp = (uint8)rbsp.u(1);
        if(1 == tmp)
        {
          MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_MEDIUM,
                      "seq_scaling_matrix_present_flag present!!");
          uint32 ulSizeOfScalingList = 0;

          for(int idx = 0; idx < ((3 != chroma_idc) ? 8: 12); idx++)
          {
            bool seq_scaling_list_present_flag = rbsp.u(1);
            if( seq_scaling_list_present_flag)
            {
              ulSizeOfScalingList = ( idx < 6)? 16 : 64;
              uint32 ulLastScale = 8, ulNextScale = 8, ulDeltaScale = 0;
              for(int idx1 = 0; idx1<ulSizeOfScalingList; idx1++ )
              {
                if(ulNextScale !=0 )
                {
                  ulDeltaScale = rbsp.se();
                  ulNextScale = (ulLastScale + ulDeltaScale + 256)%256;
                }
                ulLastScale = (ulNextScale == 0)? ulLastScale : ulNextScale;
              }
            }//if(seq_scaling_list_present_flag)
          }//for(idx)
        }
      }
      //log2_max_frame_num_minus4(ue(v))
      newParam.log2MaxFrameNumMinus4 = rbsp.ue ();
      //pic_order_cnt_type(ue(v))
      newParam.picOrderCntType = rbsp.ue ();
      if (newParam.picOrderCntType == 0)
      {
        //log2_max_pic_order_cnt_lsb_minus4(ue(v))
        newParam.log2MaxPicOrderCntLsbMinus4 = rbsp.ue ();
      }
      else if (newParam.picOrderCntType == 1)
      {
        //delta_pic_order_always_zero_flag(u(1))
        newParam.deltaPicOrderAlwaysZeroFlag = (rbsp.u (1) == 1);
        //offset_for_non_ref_pic(se(v))
        (void) rbsp.se ();
        //offset_for_top_to_bottom_field(se(v))
        (void) rbsp.se ();
        //num_ref_frames_in_pic_order_cnt_cycle[ue(v)]
        const uint32 numRefFramesInPicOrderCntCycle = rbsp.ue ();
        for (uint32 i = 0; i < numRefFramesInPicOrderCntCycle; ++i)
        {
          //offset_for_ref_frame[se(v)]
          (void) rbsp.se ();
        }
      }
      //num_ref_frames[ue(v)]
      (void) rbsp.ue ();
      //gaps_in_frame_num_value_allowed_flag[u(1)]
      (void) rbsp.u (1);
      //pic_width_in_mbs_minus1[ue(v)]
      newParam.picWidthInMbsMinus1 = rbsp.ue ();
      //pic_height_in_map_units_minus1[ue(v)]
      newParam.picHeightInMapUnitsMinus1 = rbsp.ue ();
      //frame_mbs_only_flag[u(1)]
      newParam.frameMbsOnlyFlag = (rbsp.u (1) == 1);
    }
    else
    {
      id = newParam.picSetID = rbsp.ue ();
      newParam.seqSetID = rbsp.ue ();
      (void) rbsp.u (1);
      newParam.picOrderPresentFlag = (rbsp.u (1) == 1);
    }

    H264ParamNalu *naluSet
      = ((naluType == 7) ? m_streamInfo.m_seq : m_streamInfo.m_pic);

    // We currently don't support updating existing parameter sets.
    for (i=0; i<MAX_SETS; i++)
    {
      if (naluType == 7)
      {
        if ((uint32)naluSet[i].seqSetID == id)
        {
          break;
        }
      }
      else
      {
        if ((uint32)naluSet[i].picSetID == id)
        {
          break;
        }
      }
    }

    if (i != MAX_SETS)
    {
      const int tempSize = naluSet[i].naluSize;
      if ((totalBytes != tempSize)
          || (0 != std_memcmp (&encodedBytes[0],
                               &naluSet[i].nalu[0],
                               totalBytes)))
      {
        MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_ERROR,
                   "parseH264Frame::H.264 stream contains two"
                   " or more parameter set NALUs having the "
                   "same ID -- this requires either a separate "
                   "parameter set ES or multiple sample "
                   "description atoms, neither of which is "
                   "currently supported!");
      }
    }
    // Otherwise, add NALU to appropriate NALU set.
    else
    {
      if (m_streamInfo.m_index < 0)
      {
        m_streamInfo.m_index = 0;
      }

      if ((m_streamInfo.m_index >= 0) && 
          (m_streamInfo.m_index < MAX_SETS))
      {
        naluSet[m_streamInfo.m_index] = newParam;
        naluSet[m_streamInfo.m_index].nalu = 
                         (uint8*)MM_Malloc(totalBytes+1);
        if(naluSet[m_streamInfo.m_index].nalu)
        {
          memcpy(naluSet[m_streamInfo.m_index].nalu, 
                 encodedBytes, totalBytes);
        }
      }
      m_streamInfo.m_index++;
    }
  }
}

/* 
 * @brief Gets the video dimensions, from the accumulated stream info
 *
 * @param[in] height of the clip
 * @param[in] width of the clip
 */
void H264HeaderParser::GetVideoDimensions( uint16 &height, uint16 &width )
{
  // Store a copy of the first sequence parameter set.
  H264ParamNalu &seq = m_streamInfo.m_seq[0];
  width = 0;
  height = 0;

  if ((m_streamInfo.m_index >= 0) && (m_streamInfo.m_index < MAX_SETS))
  {
    width = (uint16)(16 * (seq.picWidthInMbsMinus1 + 1));
    height = (uint16)(16 * (seq.picHeightInMapUnitsMinus1 + 1));
  }
}

/* 
 * @brief Gets the video dimensions, from the accumulated stream info
 *
 * @param[in] height of the clip
 * @param[in] width of the clip
 */
void H264HeaderParser::GetVideoDimensions( uint32 &height, uint32 &width )
{
  // Store a copy of the first sequence parameter set.
  H264ParamNalu &seq = m_streamInfo.m_seq[0];
  width = 0;
  height = 0;

  if ((m_streamInfo.m_index >= 0) && (m_streamInfo.m_index < MAX_SETS))
  {
    width = 16 * (seq.picWidthInMbsMinus1 + 1);
    height = 16 * (seq.picHeightInMapUnitsMinus1 + 1);
  }
}

