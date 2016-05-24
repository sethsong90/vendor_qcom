// -*- Mode: C++ -*-
/*============================================================
// FILE: aacmetadata.h
//
// SERVICES: Audio
//
// DESCRIPTION:
/// @file aacmetadata.h
/// Declarations for meta data . Audio Metadata includes
/// Content metadata and technical metadata.
/// Content metadata is defined as data that describes the content such as:
/// IMelody, ID3v1 and ID3v2.
/// Technical metadata is defined as data that describes the audio data
/// such as SampleRate, BitRate, etc.
///
/// Copyright (c) 2011-2012 Qualcomm Technologies, Inc.
/// All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.

========================================================================== */
/* =======================================================================
                             PERFORCE HEADER
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/AACParserLib/main/latest/inc/aacmetadata.h#7 $
$DateTime: 2012/06/04 03:05:53 $
$Change: 2472042 $

========================================================================== */

#ifndef __AAC_METADATA_H__
#define __AAC_METADATA_H__

//============================================================
// INCLUDES
//============================================================
// CS
#include "AEEStdDef.h" // std typedefs, ie. byte, uint16, uint32, etc.
#include "aacheaders.h"
#include "aacerrors.h"
#include "id3.h"

//============================================================
// DATA TYPES
//============================================================

//============================================================
// CONSTANTS
//============================================================

//============================================================
// CLASS: aacmetadata
//
// DESCRIPTION:
/// \brief Interface that provides the Metadata for aac format
///
/// This interface defines methods for getting the Metadata that is found
/// in the audio data. It is organized into user data and technical data.
/// User data may be one or more of IMelody, ID3v1, and ID3v2.
/// Technical metadata is particular to the Aduio format and take from either
/// the file header, or first couple of frame headers, or calculated from
/// the data found.
///
//============================================================
class aacmetadata {

//============================================================
// PUBLIC CLASS ATTRIBUTES
//============================================================
public:

//============================================================
// PRIVATE CLASS ATTRIBUTES
//============================================================
private:

  tech_data_aac m_aac_tech_metadata;

  metadata_id3v1_type *m_pid3v1;

  metadata_id3v2_type *m_pid3v2;

  bool m_bid3v1_present;

  bool m_bid3v2_present;

//============================================================
// PUBLIC CLASS METHODS
//============================================================
public:

  /// Constructor
  aacmetadata ();

  /// Destructor
  ~aacmetadata ();

//============================================================
/// IAudioMetadata interface functions
//============================================================

  /// Flag set to true if metadata is present
  /// param [out] id3v1_present - true is metadata is present
  uint32 id3v1_is_present (/*rout*/ boolean* id3v1_present);

  /// Flag set to true if metadata is present
  /// param [out] id3v2_present - true is metadata is present
  uint32 id3v2_is_present (/*rout*/ boolean* id3v2_present);

  /// Flag set to true if metadata is present
  /// param [out] imelody_present - true is metadata is present
  uint32 imelody_is_present (/*rout*/ boolean* imelody_present);

  /// ID3v1 Metadata, only valid when <b>id3v1_present</b> is true.
  /// Get ID3v1 metadata
  /// @returns id3v1 info set via set_id3v1;
  metadata_id3v1_type* get_id3v1 ();

  /// Set ID3v1 metadata
  /// @param [in] id3v1 - metadata struct
  uint32 set_id3v1 (/*in*/ const metadata_id3v1_type* id3v1);

  /// ID3v2 Metadata, only valid when <b>id3v2_present</b> is true.
  /// Get ID3v2 metadata
  /// @returns id3v2 info set via set_id3v2;
  metadata_id3v2_type* get_id3v2 ();
  /// Set ID3v2 metadata
  /// @param [in] id3v2 - metadata struct
  uint32 set_id3v2 (/*in*/ const metadata_id3v2_type* id3v2);

  /// IMelody Metadata, only valid when <b>imelody_present</b> is true.
  /// Get IMelody data structure
  /// @param [out] imelody - IMelody data struct
  uint32 get_imelody (/*rout*/ metadata_imelody_type* imelody);

  /// Set IMelody data structure
  /// @param [in] imelody - IMelody data struct
  uint32 set_imelody (/*in*/ const metadata_imelody_type* imelody);
  /// \brief Technical metadata extracted from file and frame headers.
  ///
  /// This is a union of technical data structures for each media type
  /// since each media type provides a different set of technical data.
  /// @param [out] tech_data - technical metadata union
  uint32 get_tech_metadata (/*rout*/ tech_data_aac* tech_data);

  /// \brief show class attributes, for debug
  ///
  /// Output class attributes for debugging. These functions should
  /// compile into no-ops when DEBUG is not defined. A string may be
  /// passed to include caller information.
  /// @param [in] str - string to display
  void show(const char* str);

  /// \brief shortcut version of show function, string is NULL
  void show();

//============================================================
// PROTECTED CLASS METHODS
//============================================================
protected:
  //===========================================================
  // These functions are available to be overriden by the child class
  // they are however not part of the public interface
  //===========================================================

//============================================================
// PRIVATE CLASS METHODS
//============================================================
private:

  void free_ID3v2_metadata_memory();

  friend class aacParser;

};// end_class
#endif // __AACMETADATA_H__
