#ifndef MP2_STREAM_DATA_DEFN
#define MP2_STREAM_DATA_DEFN

/* =======================================================================
                              MP2StreamParserDataDefn.h
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
$Header: //source/qcom/qct/multimedia2/Video/Source/FileDemux/MP2ParserLib/main/latest/inc/MP2StreamParserDataDefn.h#35 $
========================================================================== */

#include "MP2StreamParserConstants.h"

/*
*******************************************************************
* Data types common to program stream as well as transport stream.
*******************************************************************
*/
typedef enum track_type_t
{
  TRACK_TYPE_UNKNOWN,
  TRACK_TYPE_AUDIO,
  TRACK_TYPE_VIDEO
}track_type;

typedef enum media_codec_type_
{
  UNKNOWN_AUDIO_VIDEO_CODEC,
  AUDIO_CODEC_AAC,
  AUDIO_CODEC_AC3,
  AUDIO_CODEC_EAC3,
  AUDIO_CODEC_LPCM,
  AUDIO_CODEC_DTS,
  AUDIO_CODEC_MPEG1,
  AUDIO_CODEC_MPEG2,
  AUDIO_CODEC_MP3,
  VIDEO_CODEC_MPEG2,
  VIDEO_CODEC_H264,
  VIDEO_CODEC_VC1
}media_codec_type;

typedef enum{
  AAC_FORMAT_UNKNOWN,
  AAC_FORMAT_ADTS,
  AAC_FORMAT_ADIF,
  AAC_FORMAT_RAW,
  AAC_FORMAT_LOAS,
  AAC_FORMAT_MAX
}aac_format_type;

typedef enum aspect_ratio_
{
  ASPECT_RATIO_1_1 = 1,
  ASPECT_RATIO_4_3,
  ASPECT_RATIO_16_9,
  ASPECT_RATIO_2_21_1
}video_aspect_ratio;

typedef enum frame_rate_
{
  FRAME_RATE_25_FPS =3,
  FRAME_RATE_29_97_FPS,
  FRAME_RATE_30_FPS
}video_frame_rate;

typedef struct _mp2_stream_sample_info_
{
  uint32 nsample;
  uint32 nsize;
  uint64 noffset;
  float  ntime;
  bool   bsync;
  float  delta;
}mp2_stream_sample_info;

typedef struct video_picture_data_
{
  uint16 temporal_reference;
  uint16 picture_coding_type;
  uint16 vbv_delay;
  uint16 full_pel_forward_vector;
  uint16 full_pel_backward_vector;
  uint16 forward_f_code;
  uint16 backward_f_code;
  uint16 extra_bit_picture;
  uint16 extra_information_picture;
}video_picture_data;

typedef struct video_info_
{
  uint16              Width;
  uint16              Height;
  video_aspect_ratio  Aspect_Ratio;
  video_frame_rate    Frame_Rate;
  media_codec_type    Video_Codec;
  video_picture_data  PictureData;
}video_info;

typedef struct audio_info_
{
  media_codec_type Audio_Codec;
  uint32           SamplingFrequency;
  uint8            NumberOfChannels;
  uint32           NumberOfFrameHeaders;
  uint8            DynamicRangeControl;
  uint8            AudioObjectType;
  uint32           Bitrate;
  uint8            ucProtection;
  uint8            ucVersion;
  uint8            ucLayer;
}audio_info;

typedef struct aac_audio_info_
{
  uint32           SamplingFrequency;
  uint8            NumberOfChannels;
  uint8            AudioObjectType;
  uint8            ucCRCPresent;
}aac_audio_info;

/*
*******************************************************************
* Data types applicable to program stream only
*******************************************************************
*/

//Program stream pack and system header
//system target header from mpeg2 program stream
typedef struct _system_header
{
  uint64       noffset;
  uint32       sys_header_start_code;
  uint16       header_length;
  uint32       rate_bound:22;
  uint8        audio_bound:6;
  uint8        fixed_flag:1;
  uint8        csps_flag:1;
  uint8        sys_audio_lock_flag:1;
  uint8        sys_video_lock_flag:1;
  uint8        video_bound:5;
  uint8        packet_restriction_flag:1;
}system_header;

//mpeg2 program stream pack header
typedef struct _pack_header_
{
  uint64          noffset;
  uint32          pack_start_code;
  double          scr_base;
  uint16          scr_extension;
  double          scr_val;
  uint32          program_mux_rate:22;
  uint8           pack_stuffing_length:3;
  system_header*  sys_header;
}pack_header;


/*
*******************************************************************
* Data types applicable to transport stream only
*******************************************************************
*/

//Common data found across each PSI section
typedef struct _common_sect_data
{
  uint64 noffset;
  uint8 pointer_val;
  uint8 table_id;
  uint8 sect_synt_indtor:1;
  uint8 zero_field:1;
  uint8 reserved:2;
  uint16 sect_length:12;
}CommonSectionData;

//AC3 audio descriptor (Section A.2.4)
//Standard: ETSI TS 102 366 V1.2.1 (2008-08)
typedef struct AC3_audio_descriptor_data
{
  uint8  ucDescriptorTag;
  uint8  ucDescriptorLength;
  uint8  ucSamplingRateCode:3;
  uint8  ucBSID:5;
  uint8  ucBitRateCode:6;
  uint8  ucSurrondMode:2;
  uint8  ucBSMod:3;
  uint8  ucNumChannels:4;
  uint8  ucFullSvc:1;
}AC3AudioDescriptor;
//EAC3 audio descriptor Section 3.5 ATSC A/52 Annex G
//Standard: ETSI TS 102 366 V1.2.1 (2008-08)
typedef struct EAC3_audio_descriptor_data
{
  uint8  ucDescriptorTag;
  uint8  ucDescriptorLength;
  uint8  ucReserved:1;
  uint8  ucBSIDFlag:1;
  uint8  ucMainIdFlag:1;
  uint8  ucAsvcFlag:1;
  uint8  ucMixInfoExists:1;
  uint8  ucSubStream1Flag:1;
  uint8  ucSubStream2Flag:1;
  uint8  ucSubStream3Flag:1;
  uint8  ucReserved2:1;
  uint8  ucFullServiceFlag:1;
  uint8  ucAudioServiceType:3;
  uint8  ucNumOfChannels:3;
  uint8  ucLangFlag:1;
  uint8  ucLangFlag2:1;
  uint8  ucBSID:5;
  uint8  ucZeroBits:5;
}EAC3AudioDescriptor;
//AVC descriptor describing AVC video
//page 90, 2.6.64
typedef struct avc_descriptor_data
{
  uint8 descriptor_tag;
  uint8 descriptor_length;
  uint8 profile_idc;
  uint8 constraint_set0_flag:1;
  uint8 constraint_set1_flag:1;
  uint8 constraint_set2_flag:1;
  uint8 AVC_compatible_flags:8;
  uint8 level_idc;
  uint8 AVC_still_present:1;
  uint8 AVC_24_hour_picture_flag:1;
}AVCDescriptor;

//Conditional access descriptor which contains either ECMs or EMMs
//page 69, 2.6.16
typedef struct ca_descriptor_data
{
  uint64 noffset;
  uint8 descriptor_tag;
  uint8 descriptor_length;
  uint16 ca_system_iid;
  uint16 ca_pid;
  uint8* pvt_data_byte;
  uint64 nStartOffsetInFile;
  uint16 nPvtDataBytes;
}CADescriptor;

//Registration descriptor which contains information to
//identify private streams
//page 69, 2.6.8
typedef struct registration_descriptor_data
{
  uint8  ucDescriptorTag;
  uint8  ucDescriptorLength;
  uint32 ullFormatIdentifier;
  uint8* pucAdditionalInfo;
}RegistrationDescriptor;

//Mpeg4 audio descriptor which contains information
//for Mpeg4 audio streams
//page 69, 2.6.38
typedef struct Mpeg4_audio_descriptor_data
{
  uint8  ucDescriptorTag;
  uint8  ucDescriptorLength;
  uint8  ucMpeg4AudioProfileLevel;
}Mpeg4AudioDescriptor;

//Mpeg2 AAC audio descriptor which contains information
//for Mpeg2 audio streams
//page 93, 2.6.68
typedef struct Mp2_AAC_audio_descriptor_data
{
  uint8  ucDescriptorTag;
  uint8  ucDescriptorLength;
  uint8  ucMpeg2AACProfile;
  uint8  ucMpeg2AACChannelConfig;
  uint8  ucMpeg2AACAdditionalInfo;
}Mp2AACAudioDescriptor;

//DVD LPCM audio descriptor which contains information
//for audio streams.
typedef struct DVD_LPCM_audio_descriptor_data
{
  uint8  ucDescriptorTag;
  uint8  ucDescriptorLength;
  uint8  ucSamplingFreq;
  uint8  ucBitsPerSample;
  uint8  ucEmphasisFlag;
  uint8  ucChannels;
}DVDLPCMAudioDescriptor;

//ISO 639 language descriptor which contains information
//for audio streams. Section 2.6.19
typedef struct Language_descriptor_data
{
  uint8  ucDescriptorTag;
  uint8  ucDescriptorLength;
  uint32 ullISO639LanguageCode;
  uint8  ucAudioType;
}LanguageDescriptor;

//DTS audio descriptor which contains information
//for DTS audio streams
//refer to DTS spec
typedef struct DTS_audio_descriptor_data
{
  uint8  ucDescriptorTag;
  uint8  ucDescriptorLength;
  uint8  ucSampleRateCode;
  uint8  ucBitrateCode;
  uint8  ucNBlks;
  uint16 ulFSize;
  uint8  ucSurroundMode;
  uint8  ucLFEFlag;
  uint8  ucExtendedSurroundFlag;
  uint8  ucComponentType;
  uint8* pucAdditionalInfo;
}DTSAudioDescriptor;

//DTS-HD audio descriptor which contains information
//for DTS-HD audio streams
//refer to DTS spec
typedef struct DTSHD_substream_asset_struct
{
  uint8  ucAssetConstruction;
  uint8  ucVBRFlag;
  uint8  ucPostEncodeBRScalingFlag;
  uint8  ucComponentTypeFlag;
  uint8  ucLanguageCodeFlag;
  uint16 ulBitrateScaled;
  uint16 ulBitrate;;
  uint8  ucComponentType;
  uint32 ullISO639LanguageCode;
}DTSHDSubstreamAssetStruct;

typedef struct DTSHD_substream_struct
{
  uint8  ucSubstreamLength;
  uint8  ucNumAssets;
  uint8  ucChannelCount;
  uint8  ucLFEFlag;
  uint8  ucSamplingFrequency;
  uint8  ucSampleResolution;
  DTSHDSubstreamAssetStruct* pAssetStruct;
}DTSHDSubstreamStruct;

typedef struct DTSHD_audio_descriptor_data
{
  uint8  ucDescriptorTag;
  uint8  ucDescriptorLength;
  uint8  ucSubstreamCoreFlag;
  uint8  ucSubstream0Flag;
  uint8  ucSubstream1Flag;
  uint8  ucSubstream2Flag;
  uint8  ucSubstream3Flag;
  DTSHDSubstreamStruct* pSubstreamCoreStruct;
  DTSHDSubstreamStruct* pSubstream0Struct;
  DTSHDSubstreamStruct* pSubstream1Struct;
  DTSHDSubstreamStruct* pSubstream2Struct;
  DTSHDSubstreamStruct* pSubstream3Struct;
  uint8* pucAdditionalInfo;
}DTSHDAudioDescriptor;

//Elementary stream descriptor describing each elementary stream
//page 46, 2.4.4.8
typedef struct es_descriptor_data
{
  uint64 noffset;
  uint8 stream_type;
  uint16 elementary_pid;
  uint16 ES_info_length;
  CADescriptor* DescriptorData;
  uint16 nProgDesc;
  uint64 nStartOffsetInFile;
}ESDescriptor;

//conditional access table,page 45, 2.4.4.6
typedef struct _cond_access_sect
{
  CommonSectionData common_sect_data;
  uint32 reserved2:18;
  uint8 version_no:5;
  uint8 current_next_indicator:1;
  uint8 section_no;
  uint8  last_sect_no;
  CADescriptor* DescriptorsData;
  int32  CRC;
}ConditionalAccessSection;

//Program association table, page 43, 2.4.4.3
typedef struct _prog_assoc_sect
{
  CommonSectionData common_sect_data;
  uint16 transport_stream_id;
  uint8  reserved2:2;
  uint8  version_no:5;
  uint8  current_next_indicator:1;
  uint8  section_no;
  uint8  last_sect_no;
  int*   program_numbers;
  int*   ts_PID;
  int32  CRC;
  bool   bPATComplete;//Set after we get a complete PAT
  uint16 nBytesConsumed;
  uint16 nPrograms;
  bool   isAvailable;//Set after we receive atleast 1 section of PAT
}ProgramAssociationSection;

//Program map table,page 46, 2.4.4.8
typedef struct _prog_map_sect
{
  CommonSectionData common_sect_data;
  uint16 program_number;
  uint8  reserved2:2;
  uint8  version_no:5;
  uint8  current_next_indicator:1;
  uint8  section_no;
  uint8  last_sect_no;
  uint16 PCR_PID;
  uint8  reserved3:3;
  uint16 program_info_length:12;
  CADescriptor* programDescriptorsData;
  ESDescriptor* ESDescData;
  uint16 nProgDesc;
  uint16 nESDesc;
  uint16 nBytesConsumed;
  bool   bProgParseComplete;
}ProgramMapSection;

//Program Stream map,page 77, 2.5.4.1 in ISO/IEC 13818-1 : 2000 (E)
typedef struct _prog_stream_map
{
  uint8  current_next_indicator:1;
  uint8  reserved2:2;
  uint8  version_no:5;
  uint16 program_stream_length;
  uint16 elementary_stream_map_len;
  uint16 nESDesc;
  uint16 nBytesConsumed;
  bool   bProgParseComplete;
}ProgramStreamMap;

//Transport stream description table,page 50, Table 2-30-1
typedef struct _desc_sect
{
  CommonSectionData common_sect_data;
  uint8  version_no:5;
  uint8  current_next_indicator:1;
  uint8  section_no;
  uint8  last_sect_no;
  CADescriptor* DescriptorsData;
}DescriptionSection;

//Transport stream section, page 50, Section 2.4.4.13
typedef struct _ts_stream_section
{
  uint8 table_id;
  uint16 sect_length:12;
  uint8  version_no:5;
  uint8  current_next_indicator:1;
  uint8  section_no;
  uint8  last_sect_no;
}TSStreamSection;

//Transport stream adaption field, page 22, Table 2-6, 2.4.3.4
typedef struct AdapField
{
  uint64 noffset;
  uint8 adaption_field_length;
  uint8 discontinuity_indicator: 1;
  uint8 random_access_indicator: 1;
  uint8 es_priority_indicator: 1;
  uint8 PCR_flag: 1;
  uint8 OPCR_flag: 1;
  uint8 splicing_point_flag: 1;
  uint8 transport_pvt_data_flag: 1;
  uint8 adaption_field_extn_flag: 1;
  unsigned long long prog_clk_ref_base: 33;
  uint16        prog_clk_ref_extn: 9;
  unsigned long long orig_prog_clk_ref_base: 33;
  uint16 orig_prog_clk_ref_extn: 9;
  int8 splice_countdown;
  uint8 transport_pvt_Data_length;
  uint8* pvt_data_byte;
  uint8 adaption_field_extn_length;
  uint8 ltw_flag: 1;
  uint8 piecewise_rate_flag: 1;
  uint8 seamless_splice_flag: 1;
  uint8 ltw_valid_flag: 1;
  uint16 ltw_offset;
  uint32 piecewise_rate;
  uint8 splice_type: 4;
  unsigned long long DTS_next_AU: 33;
  uint8 stuffing_byte;
}MP2TStreamAdaptionField;

//Transport stream  packet, page 18, 2.4.3.2
typedef struct Mpeg2TStreamPkt
{
  uint64 noffset;
  uint8 sync_byte;
  uint8 t_error_indicator: 1;
  uint8 pyld_unit_start_indicator: 1;
  uint8 transport_priority: 1;
  uint16 PID: 13;
  uint8 transport_scrambling_control: 2;
  uint8 adaption_field_control: 2;
  uint8 continuity_counter: 4;
  MP2TStreamAdaptionField adaption_field;
  void* data_bytes;
}MP2TransportStreamPkt;

/*
*******************************************************************
* Data types appllicable to PES streams
*******************************************************************
*/
typedef struct _stream_info
{
  track_type   stream_media_type;
  uint16       stream_id;
  uint8        buffer_bound_scale:1;
  uint16       buffer_size_bound;
  uint32       buffer_size;
  uint32       bitRate;
  audio_info   audio_stream_info;
  video_info   video_stream_info;
  bool         bParsed;
}stream_info;

typedef struct _pci_pkt_
{
  uint64 noffset;
  uint32 blockno;
  uint16 flags_aps;
  uint16 bitmask_puo;
  uint32 start_pts;
  uint32 end_pts;
  uint8  recording_code[32];
}pci_pkt;

typedef struct _dsi_pkt_
{
  uint64 noffset;
  uint64 end_offset_curr_vobu;
  uint32 blockno;
  uint32 vobu_ea;
  uint32 first_ref_frame_end_block;
  uint32 second_ref_frame_end_block;
  uint32 third_ref_frame_end_block;
  uint16 vobu_vob_idn;
  uint32 vob_v_s_ptm;
  uint32 vob_v_e_ptm;
  uint64 next_vobu_offset;
  uint64 prv_vobu_offset;
  bool   next_vobu_offset_valid;
  bool   prv_vobu_offset_valid;
}dsi_pkt;

//PES extension header,optional
typedef struct PES_PKT_EXTN
{
  //pes extn header,page 33
  bool        pes_extn_pvt_data_flag;
  bool        pes_extn_pack_hdr_flag;
  bool        pes_extn_pkt_seq_cnt_flag;
  bool        pes_extn_std_buffer_flag;
  bool        pes_extn_flag2;
  uint8       pes_pvt_data[PES_EXTN_PVT_DATA_BYTES];
  uint8       pack_field_length;
  pack_header pes_extn_pack_hdr;
  uint8       prog_seq_cnt;
  uint8       mpeg1_mpeg2_iden;
  uint8       original_stuff_length;
  uint8       p_std_buffer_scale;
  uint16      p_std_buffer_size;
  uint8       pes_extn_field_length;
}PESExtn;

//PES packet, page 31, Table 2-17, 2.4.3.6
typedef struct PES_PKT
{
  uint64             noffset;
  uint32             start_code_prefix:24;
  uint32             packet_length;
  uint16             tsPID;
  //derived from stream_id based on audio/video PES packet
  uint8              trackid;
  uint8              scrambling_control: 2;
  uint8              pes_priority: 1;
  uint8              data_align_indicator: 1;
  uint8              copyright: 1;
  uint8              original_copy: 1;
  uint8              pts_dts_flag: 2;
  uint8              escr_flag: 1;
  uint8              es_rate_flag: 1;
  uint8              dsm_trick_mode_flag: 1;
  uint8              add_copy_info_flag: 1;
  uint8              pes_crc_flag: 1;
  uint8              pes_extn_flag: 1;
  PESExtn            pes_extn_hdr;
  uint8              pes_hdr_data_length;
  uint8              marker_field: 1;
  double             pts;
  double             dts;
  double             escr_base;
  uint16             escr_extn: 9;
  double             escr_val;
  uint32             es_rate: 22;
  uint8              trick_mode_control: 3;
  uint8              field_id: 2;
  uint8              intra_slice_refresh: 1;
  uint8              frequency_truncation: 2;
  uint8              rep_cntrol: 5;
  uint8              add_copy: 7;
  uint16             prv_pkt_pes_crc;
  uint8              stuffing_byte;
  void*              pes_pkt_data_byte;
  uint8              padding_byte;
}PESPacket;

typedef struct _avc_codec_info_
{
  uint8* codecInfoBuf;
  uint8   size;
  bool    isValid;
}avc_codec_info;

typedef enum _start_code_type_
{
  START_CODE_DEFAULT = 0,
  AVC_START_CODE_24BIT = 3,
  AVC_START_CODE_32BIT = 4
}start_code_type;

typedef struct _partial_frame_data_
{
  bool haveFrameData;
  uint8 frameBuf[TS_PKT_SIZE*10];
  uint32 len;
  MP2TransportStreamPkt dataTSPkt;
}partial_frame_data;

typedef struct _underrun_frame_data_
{
  uint8* pFrameBuf;
  uint32 nBufSize;
  uint32 nDataLen;
  uint32 bytesLost;
  uint32 nPESLen;
  uint64 nFrameTime;
  uint32 nRandomAccess;
}underrun_frame_data;

#endif//MP2_STREAM_DATA_DEFN
