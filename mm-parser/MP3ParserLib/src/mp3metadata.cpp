// -*- Mode: C++ -*-
/* =======================================================================
                              mp3metadata.cpp
DESCRIPTION

  Defines an interface to access Audio Metadata. Audio Metadata includes
  Content metadata and technical metadata.
  Content metadata is defined as data that describes the content such as:
  IMelody, ID3v1 and ID3v2.
  Technical metadata is defined as data that describes the audio data
  such as SampleRate, BitRate, etc.

  Copyright (c) 2009-2013 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
========================================================================== */

/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/MP3ParserLib/main/latest/src/mp3metadata.cpp#20 $
$DateTime: 2013/08/22 23:16:43 $
$Change: 4322584 $

========================================================================== */

//=============================================================================
// INCLUDES
//=============================================================================

//local includes
#include "mp3metadata.h"
#include "mp3parser.h"
#include "MMMemory.h"
#include "MMDebugMsg.h"
#include "MMMemory.h"

//=============================================================================
// GLOBALS
//=============================================================================

//=============================================================================
// CONSTANTS
//=============================================================================

//=============================================================================
// FUNCTION DEFINATONS
//=============================================================================

//=============================================================
// FUNCTION : Constructor
//
// DESCRIPTION
//  Constructor for mp3metadata class
//
// PARAMETERS
//  pEnv : CS Env required for malloc,createinstance etc.
//
// RETURN VALUE
//  None
//
// SIDE EFFECTS
//  None
//=============================================================
//
mp3metadata::mp3metadata()
{
   //Initialize instance variables
   memset (&m_techmetadata,0, sizeof(m_techmetadata));
   for (uint32 ulIndex = 0; ulIndex < MAX_ID3_ELEMENTS; ulIndex++)
   {
     m_pid3v2[ulIndex] = NULL;
   }

   m_id3v1            = NULL;
   m_ulTotalID3V2Tags = 0;

   m_id3v1_present = false;
   m_id3v2_present = false;
}
//=============================================================
// FUNCTION : Destructor
//
// DESCRIPTION
//  Destructor for mp3metadata class
//
// PARAMETERS
//  None
//
// RETURN VALUE
//  None
//
// SIDE EFFECTS
//  None
//=============================================================
//
mp3metadata::~mp3metadata()
{
   if(m_ulTotalID3V2Tags)
   {
      free_ID3v2_metadata_memory();
      for (uint32 ulIndex = 0; ulIndex < m_ulTotalID3V2Tags; ulIndex++)
      {
        MM_Delete( m_pid3v2[ulIndex]);
        m_pid3v2[ulIndex] = NULL;
      }
   }
   m_ulTotalID3V2Tags = 0;
   MM_Delete( m_id3v1);
   m_id3v1 = NULL;
}

//=============================================================================
// FUNCTION : set_id3v1
//
// DESCRIPTION
//  Set ID3v1 metadata
//
// PARAMETERS
//  id3v1 - metadata struct
//
// RETURN VALUE
//  MP3_SUCCESS : success
//  MP3_INVALID_PARM : invalid parameters
//  MP3_FAILURE: id3v1 tag is already set
//  AEE_ENOMEMORY : Not enough memory
//
// SIDE EFFECTS
//  None
//=============================================================================
//
PARSER_ERRORTYPE mp3metadata::set_id3v1 (/*in*/ const metadata_id3v1_type* id3v1)
{
  PARSER_ERRORTYPE result = PARSER_ErrorNone;
   if (!id3v1)
   {
     result = PARSER_ErrorInvalidParam;
   }
   if (m_id3v1)
   {
      result = PARSER_ErrorDefault;
   }
   if(result == PARSER_ErrorNone)
   {
     m_id3v1_present = true;
     m_id3v1 = MM_New(metadata_id3v1_type);
     if (!m_id3v1 )
     {
       result = PARSER_ErrorInsufficientBufSize;
     }
     else
     {
       (void) std_memmove(m_id3v1, id3v1, STD_SIZEOF(*m_id3v1));
     }
   }
   return result;
}

//=============================================================================
// FUNCTION : set_id3v2
//
// DESCRIPTION
//  Set ID3v2 metadata
//
// PARAMETERS
//  id3v2 - metadata struct
//
// RETURN VALUE
//  MP3_SUCCESS : success
//  MP3_INVALID_PARM : invalid parameters
//  MP3_FAILURE: id3v2 tag is already set
//  AEE_ENOMEMORY : Not enough memory
//
// SIDE EFFECTS
//  None
//=============================================================================
//
PARSER_ERRORTYPE mp3metadata::set_id3v2 (/*in*/ const metadata_id3v2_type* pid3v2)
{
   PARSER_ERRORTYPE result = PARSER_ErrorNone;
#ifdef FEATURE_FILESOURCE_MP3_PARSER_DEBUG
   MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"mp3metadata::set_id3v2");
#endif
   if (!pid3v2)
   {
      result = PARSER_ErrorInvalidParam;
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,
                  "mp3metadata::set_id3v2 MP3_INVALID_PARM");
   }
   if (MAX_ID3_ELEMENTS == m_ulTotalID3V2Tags)
   {
#ifdef FEATURE_FILESOURCE_MP3_PARSER_DEBUG
     MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH,"Multiple ID3V2 detected...");
#endif
     MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
                  "Deleting the one stored earlier @ FileOffet %llu",
                  m_pid3v2[m_ulTotalID3V2Tags - 1]->file_position);
     MM_Delete(m_pid3v2[m_ulTotalID3V2Tags - 1]);
     m_pid3v2[m_ulTotalID3V2Tags - 1] = NULL;
   }
   if (result == PARSER_ErrorNone)
   {
     m_id3v2_present = true;
     m_pid3v2[m_ulTotalID3V2Tags] = MM_New(metadata_id3v2_type);
     if (NULL == m_pid3v2[m_ulTotalID3V2Tags])
     {
       if (!m_ulTotalID3V2Tags)
       {
         result = PARSER_ErrorInsufficientBufSize;
       }
     }
     else
     {
       (void) std_memmove(m_pid3v2[m_ulTotalID3V2Tags], pid3v2,
                          STD_SIZEOF(*pid3v2));
#ifdef FEATURE_FILESOURCE_MP3_PARSER_DEBUG
       MM_MSG_PRIO1(MM_FILE_OPS, MM_PRIO_HIGH,
                    "Stored ID3 located @ FileOffset %llu",
                    pid3v2->file_position);
#endif
       m_ulTotalID3V2Tags++;
     }
   }
   return result;
}

//=============================================================
// FUNCTION : free_ID3v2_metadata_memory
//
// DESCRIPTION
//  Frees the memory allocated by the ID3 object for individual frames.
//
// PARAMETERS
//   None
//
// RETURN VALUE
//  void
//
// SIDE EFFECTS
//  None
//=============================================================
//
void mp3metadata::free_ID3v2_metadata_memory()
{
   int    ulCount = 0;
   uint32 ulIndex = 0;

   for ( ; ulIndex < m_ulTotalID3V2Tags; ulIndex++)
   {
     //Instead of changing complete code, created variable like class variable
     metadata_id3v2_type *m_id3v2 = m_pid3v2[ulIndex];
     // MM_Delete all the generic content in generic frame
     for(ulCount = 0; ulCount < m_id3v2->generic_framesLen; ulCount++)
     {
       if(m_id3v2->generic_frames[ulCount].content)
       {
         MM_Delete (m_id3v2->generic_frames[ulCount].content);
       }
       (m_id3v2->generic_frames[ulCount]).content = NULL;
     }

     // MM_Delete all the text content of text frames
     if(m_id3v2->involved_people_list.text)
      {
        MM_Delete_Array(m_id3v2->involved_people_list.text);
        m_id3v2->involved_people_list.text = NULL;
      }
      if(m_id3v2->album.text)
      {
        MM_Delete_Array(m_id3v2->album.text);
        m_id3v2->album.text = NULL;
      }
      if(m_id3v2->beats_per_minute.text)
      {
        MM_Delete_Array(m_id3v2->beats_per_minute.text);
        m_id3v2->beats_per_minute.text = NULL;
      }
      if(m_id3v2->composer.text)
      {
        MM_Delete_Array(m_id3v2->composer.text);
        m_id3v2->composer.text = NULL;
      }
      if(m_id3v2->genre.text)
      {
        MM_Delete_Array(m_id3v2->genre.text);
        m_id3v2->genre.text = NULL;
      }
      if(m_id3v2->copyright.text)
      {
        MM_Delete_Array(m_id3v2->copyright.text);
        m_id3v2->copyright.text = NULL;
      }
      if(m_id3v2->date.text)
      {
        MM_Delete_Array(m_id3v2->date.text);
        m_id3v2->date.text = NULL;
      }
      if(m_id3v2->playlist_delay.text)
      {
        MM_Delete_Array(m_id3v2->playlist_delay.text);
        m_id3v2->playlist_delay.text = NULL;
      }
      if(m_id3v2->encoder.text)
      {
        MM_Delete_Array(m_id3v2->encoder.text);
        m_id3v2->encoder.text = NULL;
      }
      if(m_id3v2->lyricist.text)
      {
        MM_Delete_Array(m_id3v2->lyricist.text);
        m_id3v2->lyricist.text = NULL;
      }
      if(m_id3v2->file_type.text)
      {
        MM_Delete_Array(m_id3v2->file_type.text);
        m_id3v2->file_type.text = NULL;
      }
      if(m_id3v2->time.text)
      {
        MM_Delete_Array(m_id3v2->time.text);
        m_id3v2->time.text = NULL;
      }
      if(m_id3v2->content_group_desc.text)
      {
        MM_Delete_Array(m_id3v2->content_group_desc.text);
        m_id3v2->content_group_desc.text = NULL;
      }
      if(m_id3v2->title.text)
      {
        MM_Delete_Array(m_id3v2->title.text);
        m_id3v2->title.text = NULL;
      }
      if(m_id3v2->subtitle.text)
      {
        MM_Delete_Array(m_id3v2->subtitle.text);
        m_id3v2->subtitle.text = NULL;
      }
      if(m_id3v2->init_key.text)
      {
        MM_Delete_Array(m_id3v2->init_key.text);
        m_id3v2->init_key.text = NULL;
      }
      if(m_id3v2->languages.text)
      {
        MM_Delete_Array(m_id3v2->languages.text);
        m_id3v2->languages.text = NULL;
      }
      if(m_id3v2->length.text)
      {
        MM_Delete_Array(m_id3v2->length.text);
        m_id3v2->length.text = NULL;
      }
      if(m_id3v2->media_type.text)
      {
        MM_Delete_Array(m_id3v2->media_type.text);
        m_id3v2->media_type.text = NULL;
      }
      if(m_id3v2->original_title.text)
      {
        MM_Delete_Array(m_id3v2->original_title.text);
        m_id3v2->original_title.text = NULL;
      }
      if(m_id3v2->original_filename.text)
      {
        MM_Delete_Array(m_id3v2->original_filename.text);
        m_id3v2->original_filename.text = NULL;
      }
      if(m_id3v2->original_lyricists.text)
      {
        MM_Delete_Array(m_id3v2->original_lyricists.text);
        m_id3v2->original_lyricists.text = NULL;
      }
      if(m_id3v2->original_artist.text)
      {
        MM_Delete_Array(m_id3v2->original_artist.text);
        m_id3v2->original_artist.text = NULL;
      }
      if(m_id3v2->original_release_year.text)
      {
        MM_Delete_Array(m_id3v2->original_release_year.text);
        m_id3v2->original_release_year.text = NULL;
      }
      if(m_id3v2->file_own_licensee.text)
      {
        MM_Delete_Array(m_id3v2->file_own_licensee.text);
        m_id3v2->file_own_licensee.text = NULL;
      }
      if(m_id3v2->album_artist.text)
      {
        MM_Delete_Array(m_id3v2->album_artist.text);
        m_id3v2->album_artist.text = NULL;
      }
      if(m_id3v2->orchestra.text)
      {
        MM_Delete_Array(m_id3v2->orchestra.text);
        m_id3v2->orchestra.text = NULL;
      }
      if(m_id3v2->conductor.text)
      {
        MM_Delete_Array(m_id3v2->conductor.text);
        m_id3v2->conductor.text = NULL;
      }
      if(m_id3v2->remix_mod_by.text)
      {
        MM_Delete_Array(m_id3v2->remix_mod_by.text);
        m_id3v2->remix_mod_by.text = NULL;
      }
      if(m_id3v2->part_of_set.text)
      {
        MM_Delete_Array(m_id3v2->part_of_set.text);
        m_id3v2->part_of_set.text = NULL;
      }
      if(m_id3v2->publisher.text)
      {
        MM_Delete_Array(m_id3v2->publisher.text);
        m_id3v2->publisher.text = NULL;
      }
      if(m_id3v2->track.text)
      {
        MM_Delete_Array(m_id3v2->track.text);
        m_id3v2->track.text = NULL;
      }
      if(m_id3v2->recording_dates.text)
      {
        MM_Delete_Array(m_id3v2->recording_dates.text);
        m_id3v2->recording_dates.text = NULL;
      }
      if(m_id3v2->internet_radio_stn_name.text)
      {
        MM_Delete_Array(m_id3v2->internet_radio_stn_name.text);
        m_id3v2->internet_radio_stn_name.text = NULL;
      }
      if(m_id3v2->internet_radio_stn_owner.text)
      {
        MM_Delete_Array(m_id3v2->internet_radio_stn_owner.text);
        m_id3v2->internet_radio_stn_owner.text = NULL;
      }
      if(m_id3v2->size_in_bytes.text)
      {
        MM_Delete_Array(m_id3v2->size_in_bytes.text);
        m_id3v2->size_in_bytes.text = NULL;
      }
      if(m_id3v2->isrc_code.text)
      {
        MM_Delete_Array(m_id3v2->isrc_code.text);
        m_id3v2->isrc_code.text = NULL;
      }
      if(m_id3v2->sw_hw_enc_settings.text)
      {
        MM_Delete_Array(m_id3v2->sw_hw_enc_settings.text);
        m_id3v2->sw_hw_enc_settings.text = NULL;
      }
      if(m_id3v2->year.text)
      {
        MM_Delete_Array(m_id3v2->year.text);
        m_id3v2->year.text = NULL;
      }
      if(m_id3v2->recomm_buff_size.text)
      {
        MM_Delete_Array(m_id3v2->recomm_buff_size.text);
        m_id3v2->recomm_buff_size.text = NULL;
      }
      if(m_id3v2->user_def_text_info.text)
      {
        MM_Delete_Array(m_id3v2->user_def_text_info.text);
        m_id3v2->user_def_text_info.text = NULL;
      }
      if(m_id3v2->pic_info.pic_data)
      {
        MM_Delete_Array(m_id3v2->pic_info.pic_data);
        m_id3v2->pic_info.pic_data = NULL;
      }
   }
}
