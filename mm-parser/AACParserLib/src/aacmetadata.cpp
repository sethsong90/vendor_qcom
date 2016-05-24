/* -*- Mode: C++ -*-
============================================================
 FILE: aacmetadata.cpp

 SERVICES: Audio

 DESCRIPTION:
 @file IAudioMetadata.h
 Defines an interface to access Audio Metadata. Audio Metadata includes
 Content metadata and technical metadata.
 Content metadata is defined as data that describes the content such as:
 IMelody, ID3v1 and ID3v2.
 Technical metadata is defined as data that describes the audio data
 such as SampleRate, BitRate, etc.

 Copyright (c) 2009-2013 QUALCOMM Technologies Incorporated.
 All Rights Reserved.
 QUALCOMM Technologies Proprietary and Confidential.
============================================================*/

/* =======================================================================
                             PERFORCE HEADER
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/AACParserLib/main/latest/src/aacmetadata.cpp#23 $
$DateTime: 2012/06/26 04:43:28 $
$Change: 2535490 $
========================================================================== */

#define _AAC_METADATA_CPP_

//============================================================
// INCLUDES
//============================================================
#include "parserdatadef.h"
#include "aacmetadata.h"
#include "filebase.h"
#include "MMDebugMsg.h"
#include "MMMemory.h"

//=============================================================
// GLOBALS
//=============================================================

//=============================================================
// MACROS
//=============================================================

//=============================================================
// CONSTANTS
//=============================================================

//=============================================================
// FUNCTION DEFINITONS
//=============================================================

//=============================================================
//   Class aacmetadata
//=============================================================

//=============================================================
// FUNCTION : Constructor
//
// DESCRIPTION
//  Constructor for Aacmetadata class
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
aacmetadata::aacmetadata()
{
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "aacmetadata::aacmetadata");
#endif
  m_pid3v1 = NULL;
  m_pid3v2 = NULL;
  m_bid3v1_present = FALSE;
  m_bid3v2_present = FALSE;
  memset(&m_aac_tech_metadata,0,sizeof(tech_data_aac));
}
//=============================================================
// FUNCTION : Destructor
//
// DESCRIPTION
//  Destructor for Aacmetadata class
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
aacmetadata::~aacmetadata()
{
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "aacmetadata::~aacmetadata");
#endif
  if(m_pid3v2)
  {
    free_ID3v2_metadata_memory();
    MM_Delete( m_pid3v2);
    m_pid3v2 = NULL;
  }
  if(m_pid3v1)
  {
    MM_Delete( m_pid3v1);
  }
  m_pid3v1 = NULL;
}
//=============================================================
// FUNCTION : id3v1_is_present
//
// DESCRIPTION
//  Flag set to true if metadata is present
//
// PARAMETERS
//  id3v1_present - true if metadata is present
//
// RETURN VALUE
//  AAC_SUCCESS | AEE_ECLASSNOTSUPPORT --> success | failure
//
// SIDE EFFECTS
//  None
//=============================================================
//
uint32 aacmetadata::id3v1_is_present (boolean* id3v1_present)
{
  uint32 result = 0;
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "id3v1_is_present");
  if(id3v1_present)
  {
    *id3v1_present = (boolean)m_bid3v1_present;
    result = 1;
  }
  return result;
}
//=============================================================
// FUNCTION : id3v2_is_present
//
// DESCRIPTION
//  Flag set to true if metadata is present
//
// PARAMETERS
//  id3v2_present - true if metadata is present
//
// RETURN VALUE
//  AAC_SUCCESS | AEE_ECLASSNOTSUPPORT --> success | failure
//
// SIDE EFFECTS
//  None
//=============================================================
//
uint32  aacmetadata::id3v2_is_present (boolean* id3v2_present)
{
  uint32 result = 0;
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "id3v1_is_present");
  if(id3v2_present)
  {
    *id3v2_present = (boolean)m_bid3v2_present;
    result = 1;
  }
  return result;
}
//=============================================================
// FUNCTION : imelody_is_present
//
// DESCRIPTION
//  Flag set to true if metadata is present
//
// PARAMETERS
//  imelody_present - true if metadata is present
//
// RETURN VALUE
//  AAC_SUCCESS | AEE_ECLASSNOTSUPPORT --> success | failure
//
// SIDE EFFECTS
//  None
//=============================================================
//
uint32  aacmetadata::imelody_is_present (boolean* imelody_present)
{
  uint32 result = 0;
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "imelody_is_present");
  if(imelody_present)
  {
    *imelody_present = false;
    result = 1;
  }
  return result;
}
//=============================================================
// FUNCTION : get_id3v1
//
// DESCRIPTION
//  ID3v1 Metadata, only valid when <b>id3v1_present</b> is true. Get ID3v1 metadata
//
// RETURN VALUE
//  NULL if no id3v1 data exists else returns id3v1 information
//
// SIDE EFFECTS
//  None
//=============================================================
//
metadata_id3v1_type* aacmetadata::get_id3v1()
{
  return m_pid3v1;
}
//=============================================================
// FUNCTION : set_id3v1
//
// DESCRIPTION
//  Set ID3v1 metadata
//
// PARAMETERS
//  id3v1 - metadata struct
//
// RETURN VALUE
//  AAC_SUCCESS | AAC_FAILURE --> success | failure
//
// SIDE EFFECTS
//  None
//=============================================================
//
uint32  aacmetadata::set_id3v1 (const metadata_id3v1_type* id3v1)
{
  uint32 ret = 0;
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "set_id3v1");
#endif
  m_bid3v1_present = TRUE;
  if(m_pid3v1)
  {
    MM_Delete( m_pid3v1);
    m_pid3v1 = NULL;
  }
  m_pid3v1 = MM_New(metadata_id3v1_type);
  if(id3v1)
  {
    ret = 1;
    (void) std_memmove(m_pid3v1,id3v1,STD_SIZEOF(metadata_id3v1_type));
  }
  return ret;
}
//=============================================================
// FUNCTION : get_id3v2
//
// DESCRIPTION
//  ID3v2 Metadata, only valid when <b>id3v2_present</b> is true.Get ID3v2 metadata
//
// RETURN VALUE
//  NULL if no id3v2 info exists oytherwise returns id3v2 info*
//
// SIDE EFFECTS
//  None
//=============================================================
//
metadata_id3v2_type*  aacmetadata::get_id3v2()
{
  return m_pid3v2;
}
//=============================================================
// FUNCTION : set_id3v2
//
// DESCRIPTION
//  Set ID3v2 metadata
//
// PARAMETERS
//  id3v2 - metadata struct
//
// RETURN VALUE
//  AAC_SUCCESS | AAC_FAILURE --> success | failure
//
// SIDE EFFECTS
//  None
//=============================================================
//
uint32  aacmetadata::set_id3v2 (const metadata_id3v2_type* id3v2)
{
  uint32 result = 0;
#ifdef FEATURE_FILESOURCE_AAC_PARSER_DEBUG
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "set_id3v2");
#endif
  m_bid3v2_present = TRUE;
  if(m_pid3v2)
  {
    MM_Delete( m_pid3v2);
    m_pid3v2 = NULL;
  }
  m_pid3v2 = MM_New(metadata_id3v2_type);
  if(id3v2 && m_pid3v2)
  {
    result = 1;
    (void) std_memmove(m_pid3v2,id3v2,STD_SIZEOF(metadata_id3v2_type));
  }
  return result;
}
//=============================================================
// FUNCTION : get_imelody
//
// DESCRIPTION
//  IMelody Metadata, only valid when <b>imelody_present</b> is true.
//  Get IMelody data structure.
//
// PARAMETERS
//  imelody - IMelody data struct
//
// RETURN VALUE
//  AAC_SUCCESS | AEE_EUNSUPPORTED --> success | failure
//
// SIDE EFFECTS
//  None
//=============================================================
//
uint32  aacmetadata::get_imelody (metadata_imelody_type* imelody)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "get_imelody");
  (void)imelody;
  if(imelody)
  {
    imelody->data_present_flag = 0;
  }
  return PARSER_ErrorNotImplemented;
}
//=============================================================
// FUNCTION : set_imelody
//
// DESCRIPTION
//  Set IMelody data structure.
//
// PARAMETERS
//  imelody - IMelody data struct
//
// RETURN VALUE
//  AAC_SUCCESS | AEE_EUNSUPPORTED --> success | failure
//
// SIDE EFFECTS
//  None
//=============================================================
//
uint32  aacmetadata::set_imelody (const metadata_imelody_type* /*imelody*/)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "set_imelody");
  return PARSER_ErrorNotImplemented;
}
//=============================================================
// FUNCTION : get_tech_metadata
//
// DESCRIPTION
//  Technical metadata extracted from file and frame headers. Returns a union of technical data
//  structures for each media type since each media type provides a different set of
//  technical data.
//
// PARAMETERS
//  tech_data - technical metadata union
//
// RETURN VALUE
//  AAC_SUCCESS | AAC_FAILURE --> success | failure
//
// SIDE EFFECTS
//  None
//=============================================================
//
uint32  aacmetadata::get_tech_metadata (tech_data_aac* tech_data)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "get_tech_metadata");
  uint32 result = PARSER_ErrorNone;
  // Validate input parameters
  if(NULL == tech_data)
  {
    MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL, "get_tech_metadata AAC_INVALID_PARM");
    result = PARSER_ErrorInvalidParam;
  }
  else
  {
    if(m_aac_tech_metadata.type != AUDIO_AAC)
    {
      MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_FATAL, "get_tech_metadata m_aac_tech_metadata.type != AUDIO_AAC");
      result = PARSER_ErrorUnknownCodecType;
    }
    else
    {
      (void) std_memmove(tech_data,&m_aac_tech_metadata,sizeof(tech_data_aac));
    }
  }
  return result;
}
//=============================================================
// FUNCTION : free_ID3v2_metadata_memory
//
// DESCRIPTION
//   Frees the memory allocated by the ID3 object for individual frames.
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
void aacmetadata::free_ID3v2_metadata_memory()
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "free_ID3v2_metadata_memory");
  int i = 0;

  // MM_Delete all the generic content in generic frame
  if (m_pid3v2)
  {
     for(i=0;m_pid3v2->generic_frames && i < m_pid3v2->generic_framesLen;i++)
     {
       if(m_pid3v2->generic_frames[i].content)
       {
         MM_Delete( m_pid3v2->generic_frames[i].content);
       }
       m_pid3v2->generic_frames[i].content = NULL;
     }

     // MM_Delete all generic frame structures
     if(m_pid3v2->generic_frames)
     {
        MM_Free(m_pid3v2->generic_frames);
        m_pid3v2->generic_frames = NULL;
     }
     // MM_Delete all the text content of text frames
     if(m_pid3v2->involved_people_list.text)
      {
        MM_Delete_Array(m_pid3v2->involved_people_list.text);
        m_pid3v2->involved_people_list.text = NULL;
      }
      if(m_pid3v2->album.text)
      {
        MM_Delete_Array(m_pid3v2->album.text);
        m_pid3v2->album.text = NULL;
      }
      if(m_pid3v2->beats_per_minute.text)
      {
        MM_Delete_Array(m_pid3v2->beats_per_minute.text);
        m_pid3v2->beats_per_minute.text = NULL;
      }
      if(m_pid3v2->composer.text)
      {
        MM_Delete_Array(m_pid3v2->composer.text);
        m_pid3v2->composer.text = NULL;
      }
      if(m_pid3v2->genre.text)
      {
        MM_Delete_Array(m_pid3v2->genre.text);
        m_pid3v2->genre.text = NULL;
      }
      if(m_pid3v2->copyright.text)
      {
        MM_Delete_Array(m_pid3v2->copyright.text);
        m_pid3v2->copyright.text = NULL;
      }
      if(m_pid3v2->date.text)
      {
        MM_Delete_Array(m_pid3v2->date.text);
        m_pid3v2->date.text = NULL;
      }
      if(m_pid3v2->playlist_delay.text)
      {
        MM_Delete_Array(m_pid3v2->playlist_delay.text);
        m_pid3v2->playlist_delay.text = NULL;
      }
      if(m_pid3v2->encoder.text)
      {
        MM_Delete_Array(m_pid3v2->encoder.text);
        m_pid3v2->encoder.text = NULL;
      }
      if(m_pid3v2->lyricist.text)
      {
        MM_Delete_Array(m_pid3v2->lyricist.text);
        m_pid3v2->lyricist.text = NULL;
      }
      if(m_pid3v2->file_type.text)
      {
        MM_Delete_Array(m_pid3v2->file_type.text);
        m_pid3v2->file_type.text = NULL;
      }
      if(m_pid3v2->time.text)
      {
        MM_Delete_Array(m_pid3v2->time.text);
        m_pid3v2->time.text = NULL;
      }
      if(m_pid3v2->content_group_desc.text)
      {
        MM_Delete_Array(m_pid3v2->content_group_desc.text);
        m_pid3v2->content_group_desc.text = NULL;
      }
      if(m_pid3v2->title.text)
      {
        MM_Delete_Array(m_pid3v2->title.text);
        m_pid3v2->title.text = NULL;
      }
      if(m_pid3v2->subtitle.text)
      {
        MM_Delete_Array(m_pid3v2->subtitle.text);
        m_pid3v2->subtitle.text = NULL;
      }
      if(m_pid3v2->init_key.text)
      {
        MM_Delete_Array(m_pid3v2->init_key.text);
        m_pid3v2->init_key.text = NULL;
      }
      if(m_pid3v2->languages.text)
      {
        MM_Delete_Array(m_pid3v2->languages.text);
        m_pid3v2->languages.text = NULL;
      }
      if(m_pid3v2->length.text)
      {
        MM_Delete_Array(m_pid3v2->length.text);
        m_pid3v2->length.text = NULL;
      }
      if(m_pid3v2->media_type.text)
      {
        MM_Delete_Array(m_pid3v2->media_type.text);
        m_pid3v2->media_type.text = NULL;
      }
      if(m_pid3v2->original_title.text)
      {
        MM_Delete_Array(m_pid3v2->original_title.text);
        m_pid3v2->original_title.text = NULL;
      }
      if(m_pid3v2->original_filename.text)
      {
        MM_Delete_Array(m_pid3v2->original_filename.text);
        m_pid3v2->original_filename.text = NULL;
      }
      if(m_pid3v2->original_lyricists.text)
      {
        MM_Delete_Array(m_pid3v2->original_lyricists.text);
        m_pid3v2->original_lyricists.text = NULL;
      }
      if(m_pid3v2->original_artist.text)
      {
        MM_Delete_Array(m_pid3v2->original_artist.text);
        m_pid3v2->original_artist.text = NULL;
      }
      if(m_pid3v2->original_release_year.text)
      {
        MM_Delete_Array(m_pid3v2->original_release_year.text);
        m_pid3v2->original_release_year.text = NULL;
      }
      if(m_pid3v2->file_own_licensee.text)
      {
        MM_Delete_Array(m_pid3v2->file_own_licensee.text);
        m_pid3v2->file_own_licensee.text = NULL;
      }
      if(m_pid3v2->album_artist.text)
      {
        MM_Delete_Array(m_pid3v2->album_artist.text);
        m_pid3v2->album_artist.text = NULL;
      }
      if(m_pid3v2->orchestra.text)
      {
        MM_Delete_Array(m_pid3v2->orchestra.text);
        m_pid3v2->orchestra.text = NULL;
      }
      if(m_pid3v2->conductor.text)
      {
        MM_Delete_Array(m_pid3v2->conductor.text);
        m_pid3v2->conductor.text = NULL;
      }
      if(m_pid3v2->remix_mod_by.text)
      {
        MM_Delete_Array(m_pid3v2->remix_mod_by.text);
        m_pid3v2->remix_mod_by.text = NULL;
      }
      if(m_pid3v2->part_of_set.text)
      {
        MM_Delete_Array(m_pid3v2->part_of_set.text);
        m_pid3v2->part_of_set.text = NULL;
      }
      if(m_pid3v2->publisher.text)
      {
        MM_Delete_Array(m_pid3v2->publisher.text);
        m_pid3v2->publisher.text = NULL;
      }
      if(m_pid3v2->track.text)
      {
        MM_Delete_Array(m_pid3v2->track.text);
        m_pid3v2->track.text = NULL;
      }
      if(m_pid3v2->recording_dates.text)
      {
        MM_Delete_Array(m_pid3v2->recording_dates.text);
        m_pid3v2->recording_dates.text = NULL;
      }
      if(m_pid3v2->internet_radio_stn_name.text)
      {
        MM_Delete_Array(m_pid3v2->internet_radio_stn_name.text);
        m_pid3v2->internet_radio_stn_name.text = NULL;
      }
      if(m_pid3v2->internet_radio_stn_owner.text)
      {
        MM_Delete_Array(m_pid3v2->internet_radio_stn_owner.text);
        m_pid3v2->internet_radio_stn_owner.text = NULL;
      }
      if(m_pid3v2->size_in_bytes.text)
      {
        MM_Delete_Array(m_pid3v2->size_in_bytes.text);
        m_pid3v2->size_in_bytes.text = NULL;
      }
      if(m_pid3v2->isrc_code.text)
      {
        MM_Delete_Array(m_pid3v2->isrc_code.text);
        m_pid3v2->isrc_code.text = NULL;
      }
      if(m_pid3v2->sw_hw_enc_settings.text)
      {
        MM_Delete_Array(m_pid3v2->sw_hw_enc_settings.text);
        m_pid3v2->sw_hw_enc_settings.text = NULL;
      }
      if(m_pid3v2->year.text)
      {
        MM_Delete_Array(m_pid3v2->year.text);
        m_pid3v2->year.text = NULL;
      }
      if(m_pid3v2->recomm_buff_size.text)
      {
        MM_Delete_Array(m_pid3v2->recomm_buff_size.text);
        m_pid3v2->recomm_buff_size.text = NULL;
      }
      if(m_pid3v2->user_def_text_info.text)
      {
        MM_Delete_Array(m_pid3v2->user_def_text_info.text);
        m_pid3v2->user_def_text_info.text = NULL;
      }
      if(m_pid3v2->pic_info.pic_data)
      {
        MM_Delete_Array(m_pid3v2->pic_info.pic_data);
        m_pid3v2->pic_info.pic_data = NULL;
      }
  }//if (m_pid3v2)
}
//=============================================================
// FUNCTION : show
//
// DESCRIPTION
//  Output class attributes for debugging. These functions should compile into no-ops
//  when DEBUG is not defined
//
// PARAMETERS
//  str : may include caller information
//
// RETURN VALUE
//  void
//
// SIDE EFFECTS
//  None
//=============================================================
//
void aacmetadata::show(const char* str)
{
  MM_MSG_PRIO(MM_FILE_OPS, MM_PRIO_HIGH, "show(str)");
#ifdef _DEBUG
  ///tech_metadata tech_data;

  ///DBG(LOW,vastr("====== %s ======", str));

  // Show all metadata here
  // For now, there are none

  // Get and show tech_metadata
  ///result = get_tech_metadata (&tech_data);

  ///if(!result) {
         ///DBG(LOW,vastr("tech_metadata.type = %d",tech_data.type));
         ///DBG(LOW,vastr("tech_metadata.aac.format_type = %d",tech_data.aac.format_type));
         ///DBG(LOW,vastr("tech_metadata.aac.layer = %d",tech_data.aac.layer));
         ///DBG(LOW,vastr("tech_metadata.aac.crc_present = %d",tech_data.aac.crc_present));
         ///DBG(LOW,vastr("tech_metadata.aac.audio_object = %d",tech_data.aac.audio_object));
         ///DBG(LOW,vastr("tech_metadata.aac.sample_rate = %d",tech_data.aac.sample_rate));
         ///DBG(LOW,vastr("tech_metadata.aac.is_private = %d",tech_data.aac.is_private));
         ///DBG(LOW,vastr("tech_metadata.aac.channel = %d",tech_data.aac.channel));
         ///DBG(LOW,vastr("tech_metadata.aac.is_original = %d",tech_data.aac.is_original));
         ///DBG(LOW,vastr("tech_metadata.aac.on_home = %d",tech_data.aac.on_home));
         ///DBG(LOW,vastr("tech_metadata.aac.bit_rate = %d",tech_data.aac.bit_rate));
      ///}
      ///else {
         ///DBG(LOW,vastr("No tech metadata available"));
      ///}

   ///DBG(LOW,"================");
#endif
}
//=============================================================
// FUNCTION : show
//
// DESCRIPTION
// Output class attributes for debugging. These functions should compile into no-ops
// when DEBUG is not defined
//
//PARAMETERS
//None
//
//RETURN VALUE
//void
//
//SIDE EFFECTS
//  None
//=============================================================
//
void aacmetadata::show() {}

