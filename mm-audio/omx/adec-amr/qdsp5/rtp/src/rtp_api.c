/*=*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         RTP_API . C

GENERAL DESCRIPTION

  This file contains the implementation of amr profile. amr profile acts
  as conduite inside RTP layer. The RFC which is based on is RFC3267.

EXTERNALIZED FUNCTIONS
  None.


INITIALIZATION AND SEQUENCING REQUIREMENTS

  Need to init and configure the profile before it becomes usable.


  Copyright (c) 2009 by Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary.  Export of this technology or software is
  regulated by the U.S. Government. Diversion contrary to U.S. law prohibited.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*=*/

/*===========================================================================

$Header: //source/qcom/qct/multimedia/videophone/ps_media/RTP/RTP_profile/rel/1.0/src/qvp_rtp_amr_profile.c#4 $ $DateTime: 2008/10/17 03:10:28 $ $Author: c_ajamd $

                            EDIT HISTORY FOR FILE


when        who    what, where, why
--------    ---    ----------------------------------------------------------
17/10/08    apr    Initial version

*/
#include "rtp_unpack.h"
#include "qvp_rtp_buf.h"
#include "qvp_rtp_packet.h"

#include "qvp_rtp_api.h"
#include "qvp_rtp_amr_profile.h"

#include <stdio.h>
#include <string.h>

#define RTP_AMR_PRINT printf

int16 amr_ft_len_table[QVP_RTP_AMR_FT_RATE_SID + 1] =
{
  95,  /* 4.75 kbps */
  103, /* 5.15 kbps */
  118, /* 118 */
  134, /* 6.7 kbps */
  148, /* 7.4 kbps */
  159, /* 7.95 kbps */
  204, /* 10.2 kbps */
  244, /* 12.2 kbps */
  39   /* SID - 1.95 kbps */
};

qvp_rtp_buf_type *aud_buf;    //Pointer to the output buffer
qvp_rtp_ctx_type *ctxStruct;    //Structure that is used as context for each RTP stream
qvp_rtp_buf_type *pbufType;    //POinter to the input buffer
uint8 *rtp_residual_buf;        //POinter to the residual buffer

uint32 packet_cnt = 1;        //The number of packets


uint8 *aud_buf_data;            //A temporary pointer which stores the pointer to the output buffer
uint32 aud_len;                //The output data length
uint32 org_len;                //The length of the fresh data
uint8 packet[RTP_FIXED_HEADER_SIZE] = {0};    //A temporary array to store the fixed header

struct_toc_info toc_info[MAX_FRAMES] = {{0,0,0,0,0}, {0,0,0,0,0}, {0,0,0,0,0},{0,0,0,0,0}};    //An array of structures that stores the TOC info
uint8 *variable_stream_ptr;    //Pointer that keeps changing depending on whether the data is being
                                    //taken from the residual buffer or the fresh buffer

uint32 residual_len;                //Length of the residual data
uint8 *temp_rtp_residual_buf;    //A temporary pointer to keep track of the base address
                                        //of residual buffer
uint8 *temp_bufType_data;        //A temporary pointer to keep track of the base address of fresh data


/*=======================================================================
FUNCTION
  extract_residual_amr_frames

DESCRIPTION
  function that extracts the AMR frames from the RTP packets.

PARAMETERS
  None.

RETURN VALUE
  void.

=======================================================================*/

void extract_residual_amr_frames()
{
    uint32 frame_cnt = 0;
    uint32 i = 0;
    int32 j = 0;
    boolean flag = TRUE;

    uint8 FT;
    uint16 len = 0;

   RTP_AMR_PRINT("Inside extract_residual_amr_frames()\n");
    //count the number of AMR frames present in the RTP packet
    while(flag && residual_len)
    {
        toc_info[frame_cnt].toc = *variable_stream_ptr;
        RTP_AMR_PRINT("Value of toc: %x\n", toc_info[frame_cnt].toc);
        toc_info[frame_cnt].flag = (*variable_stream_ptr & TOC_FLAG_MASK) >> OFFSET_SEVEN;
        flag = toc_info[frame_cnt].flag;
        toc_info[frame_cnt].mode = (*variable_stream_ptr & TOC_MODE_MASK) >> OFFSET_THREE;
        toc_info[frame_cnt].fqi = (*variable_stream_ptr & TOC_FQI_MASK) >> OFFSET_TWO;

        variable_stream_ptr++;
        rtp_residual_buf++;
        residual_len--;

        frame_cnt++;
    }

    //If the data in the residual buffer is fully consumed but still some of the toc's are
    //present in the fresh buffer then shift the pointer to the fresh buffer and again run the
    //loop to count the toc's
    if(flag && (residual_len == 0))
    {
        memset(temp_rtp_residual_buf, 0, sizeof(uint8) * RTP_RESIDUAL_BUFFER_SIZE);
        rtp_residual_buf = temp_rtp_residual_buf;
        variable_stream_ptr = temp_bufType_data;
    }
    while(flag && org_len)
    {
        toc_info[frame_cnt].toc = *variable_stream_ptr;
        toc_info[frame_cnt].flag = (*variable_stream_ptr & TOC_FLAG_MASK) >> OFFSET_SEVEN;
        flag = toc_info[frame_cnt].flag;
        toc_info[frame_cnt].mode = (*variable_stream_ptr & TOC_MODE_MASK) >> OFFSET_THREE;
        toc_info[frame_cnt].fqi = (*variable_stream_ptr & TOC_FQI_MASK) >> OFFSET_TWO;

        variable_stream_ptr++;
        temp_bufType_data++;
        org_len--;

        frame_cnt++;
    }

    //Extracting the AMR frames and writing to the output buffer
    //If the data in the residual buffer gets over then the data will be taken from
    //the fresh buffer by moving the pointer
    i = 0;
    len = 0;
    while(i < frame_cnt)
    {
        RTP_AMR_PRINT("Value of toc: %x\n", toc_info[i].toc);
        FT = toc_info[i].mode;
        if(((FT > QVP_RTP_AMR_FT_RATE_SID) && (FT < QVP_RTP_AMR_FT_RATE_NOD)) || FT > QVP_RTP_AMR_FT_RATE_NOD)
        {
            RTP_AMR_PRINT("Invalid FT\n");
            return;
        }
        if(FT == QVP_RTP_AMR_FT_RATE_NOD)
        {
            len = 0;
        }
        else
        {
            len = (amr_ft_len_table[FT] + 7) / 8;
        }
        *(aud_buf->data) = toc_info[i].toc & TOC_MASK;
        if(residual_len >= len)
        {
            memcpy(aud_buf->data + 1, variable_stream_ptr, len);
            variable_stream_ptr += len;
            residual_len -= len;
            rtp_residual_buf += len;
        }
        else
        {
            memcpy(aud_buf->data + 1, variable_stream_ptr, residual_len);
            variable_stream_ptr = temp_bufType_data;
            memcpy(aud_buf->data + residual_len + 1, variable_stream_ptr, len - residual_len);

            variable_stream_ptr += len - residual_len;
            org_len -= (len - residual_len);
            temp_bufType_data += len - residual_len;
            residual_len = 0;
            memset(temp_rtp_residual_buf, 0, sizeof(uint8) * RTP_RESIDUAL_BUFFER_SIZE);
            rtp_residual_buf = temp_rtp_residual_buf;
        }
        i++;
        RTP_AMR_PRINT("Contents of aud_buf->data:\n");
        for(j = 0; j < len + 1; j++)
        {
            RTP_AMR_PRINT("%x ", aud_buf->data[j]);
        }
        RTP_AMR_PRINT("\n");
        aud_buf->data += len + 1;
        aud_len += len + 1;

        if(residual_len == 0)
        {
            rtp_residual_buf = temp_rtp_residual_buf;
            memset(rtp_residual_buf, 0, sizeof(uint8) * RTP_RESIDUAL_BUFFER_SIZE);
        }
    }
}

/*=======================================================================
FUNCTION
  extract_new_amr_frames

DESCRIPTION
  function that extracts new AMR frames from the RTP packets.

PARAMETERS
  None.

RETURN VALUE
  void.

=======================================================================*/

void extract_new_amr_frames()
{
    uint32 frame_cnt = 0;
    uint32 i = 0;
    int32 j = 0;
    boolean flag = TRUE;

    uint8 FT;
    uint16 len = 0;

    //Count the toc's
    while((flag == TRUE) && org_len)
    {
        toc_info[frame_cnt].toc = *variable_stream_ptr;
        toc_info[frame_cnt].flag = (*variable_stream_ptr & TOC_FLAG_MASK) >> OFFSET_SEVEN;
        flag = toc_info[frame_cnt].flag;
        toc_info[frame_cnt].mode = (*variable_stream_ptr & TOC_MODE_MASK) >> OFFSET_THREE;
        toc_info[frame_cnt].fqi = (*variable_stream_ptr & TOC_FQI_MASK) >> OFFSET_TWO;

        variable_stream_ptr++;
        temp_bufType_data++;
        org_len--;

        frame_cnt++;
    }
    if(!org_len)
    {
        RTP_AMR_PRINT("No more data\n");
        return;
    }

    //Extract AMR frames and write to the output buffer
    i = 0;
    len = 0;
    while(i < frame_cnt)
    {
        FT = toc_info[i].mode;
        if(((FT > QVP_RTP_AMR_FT_RATE_SID) && (FT < QVP_RTP_AMR_FT_RATE_NOD)) || FT > QVP_RTP_AMR_FT_RATE_NOD)
        {
            RTP_AMR_PRINT("Invalid FT\n");
            return;
        }
        if(FT == QVP_RTP_AMR_FT_RATE_NOD)
        {
            len = 0;
        }
        else
        {
            len = (amr_ft_len_table[FT] + 7) / 8;
        }
        if(org_len >= len)
        {
            *(aud_buf->data) = toc_info[i].toc & TOC_MASK;
            memcpy(aud_buf->data + 1, variable_stream_ptr, len);
            aud_buf->len += len + 1;

            variable_stream_ptr += len;
            org_len -= len;
            temp_bufType_data += len;
        }
        else
        {
            RTP_AMR_PRINT("No more data left in the fresh buffer\n");
            return;
        }
        i++;
        RTP_AMR_PRINT("Contents of aud_buf->data:\n");
        for(j = 0; j < len + 1; j++)
        {
            RTP_AMR_PRINT("%x ", aud_buf->data[j]);
        }
        RTP_AMR_PRINT("\n");
        aud_buf->data += len + 1;
        aud_len += len + 1;
    }
}

/**
@breif function that parses the RTP header from the packets present in the input
buffer and writes the AMR frames to the output buffer
@brief arg_aud_buf pointer to the output buffer
@brief arg_ctxStruct pointer to the structure that specifies the context of RTP stream
@brief arg_pbufType pointer to the structure which stores the input buffer pointer
@brief arg_rtp_residual_buf pointer to the residual buffer

**/

/*=======================================================================
FUNCTION
  rtp_api

DESCRIPTION
  parses the RTP header from the packets present in the input
  and writes the AMR frames to the output buffer
  arg_aud_buf pointer to the output buffer
  arg_ctxStruct pointer to the structure that specifies the context of RTP stream
  arg_pbufType pointer to the structure which stores the input buffer pointer
  arg_rtp_residual_buf pointer to the residual buffer

PARAMETERS
  arg_aud_buf - pinter to the output buffer
  arg_ctxStruct - pointer to the structure that stores RTP context info
  arg_pbufType - pointer to the structure that contains the pointer to the
                    - input data
  arg_rtp_residual_buf - pointer to the residual buffer

RETURN VALUE
  uint8.

=======================================================================*/

uint8 rtp_api(qvp_rtp_buf_type *arg_aud_buf, qvp_rtp_ctx_type *arg_ctxStruct, qvp_rtp_buf_type *arg_pbufType, uint8 *arg_rtp_residual_buf)
{
    uint32 length = 0;
    int32 i = 0;
    uint32 out_len = 0;    //Output length

    uint8 FT;            //Indicates the AMR mode
    uint8 fqi = 0;        //AMR frame quality indicator
    uint16 len = 0;        //AMR frame length
    uint16 toc;            //The byte containing the interleaving bit, mode and fqi for AMR frame

    uint32 frame_cnt = 0;
    uint8 frame_index;
    uint8 *aud_data;        //Pointer that indecates the start of AMR frames

    uint16 offset_temp = 0; // for parsing tocs
    uint32 offset = 0;        //The RTP header offset
    uint16 err1;            //Error status
    qvp_rtp_amr_oal_hdr_param_type hdr;
    qvp_rtp_status_type err;

    qvp_rtp_buf_type bufType;    //POinter to the input data

    aud_buf = arg_aud_buf;        //Stores the pointer to the output buffer that is
                                //received as argument
    ctxStruct = arg_ctxStruct;    //Pointer to the context strcure
    pbufType = arg_pbufType;
    rtp_residual_buf = arg_rtp_residual_buf;

    bufType.data = pbufType->data;

    bufType.len = pbufType->len;
    bufType.head_room = 0;

    temp_bufType_data = bufType.data;

    RTP_AMR_PRINT("\nSTART PROCESSING:\n\n");

    aud_buf_data = aud_buf->data;

    aud_buf->len = 0;

    org_len = bufType.len;
    aud_len = 0;

    //If the residual buffer conatins some data
    if(residual_len > 0)
    {
        variable_stream_ptr = rtp_residual_buf;
        temp_rtp_residual_buf = rtp_residual_buf;
        bufType.data = variable_stream_ptr;
        while(residual_len)
        {
            RTP_AMR_PRINT("Taking data from residual_buf\n");

            RTP_AMR_PRINT("--------------------------------------------------------------\n");
            RTP_AMR_PRINT("PACKET - %lu\n", packet_cnt);
            RTP_AMR_PRINT("--------------------------------------------------------------\n");

            if(residual_len >= RTP_FIXED_HEADER_SIZE)
            {
                //Parse the RTP header that is 12 bytes long
                err = qvp_rtp_unpack(ctxStruct, &bufType);

                if(err != QVP_RTP_SUCCESS)
                {
                    RTP_AMR_PRINT("Returned with error %d\n", err);
                    return -1;
                }
                RTP_AMR_PRINT("bufType.parse_offset = %d\n", bufType.parse_offset);
                RTP_AMR_PRINT("Payload: \n");

                offset = bufType.parse_offset/8;

                //Parse the fixed header
                err1 = qvp_rtp_parse_amr_fixed_hdr_oa((bufType.data + offset), bufType.len, &hdr);
                if(err1 == 0)
                {
                    RTP_AMR_PRINT("qvp_rtp_parse_amr_fixed_hdr_oa ERROR %d\n", err);
                    return -1;
                }
                bufType.head_room = 0;
                offset = 0;
                residual_len -= RTP_FIXED_HEADER_SIZE;
                rtp_residual_buf += RTP_FIXED_HEADER_SIZE;

                //After parsing the RTP header if the data left in the RTP buffer
                //is non zero
                if(residual_len > 0)
                {
                    variable_stream_ptr = rtp_residual_buf;
                    extract_residual_amr_frames();
                }

                //If no data is left in the RTP buffer then change the pointer to point to
                //the fresh data
                else
                {
                    variable_stream_ptr = temp_bufType_data;
                    extract_new_amr_frames();
                    packet_cnt++;
                    bufType.head_room = 0;
                    offset = 0;
                    rtp_residual_buf = temp_rtp_residual_buf;
                    memset(rtp_residual_buf, 0, sizeof(uint8) * RTP_RESIDUAL_BUFFER_SIZE);
                    break;
                }
            }

            //If the length of the data present in the RTP buffer is less than
            //the fixed header size then copy the data present in the residual buffer
            //in a temporary array and copy the remaining data from the fresh buffer
            //into the temporary array so that it forms the full RTP header and then shift the
            //pointer to point to the fresh buffer and start taking packets from fresh buffer
            else
            {
                memcpy(packet, rtp_residual_buf, residual_len);
                memcpy(packet + residual_len, temp_bufType_data, RTP_FIXED_HEADER_SIZE - residual_len);
                bufType.data = packet;
                err = qvp_rtp_unpack(ctxStruct, &bufType);

                if(err != QVP_RTP_SUCCESS)
                {
                    RTP_AMR_PRINT("Returned with error %d\n", err);
                    return -1;
                }
                RTP_AMR_PRINT("bufType.parse_offset = %d\n", bufType.parse_offset);
                RTP_AMR_PRINT("Payload: \n");

                offset = bufType.parse_offset/8;
                err1 = qvp_rtp_parse_amr_fixed_hdr_oa((bufType.data + offset), bufType.len, &hdr);
                if(err1 == 0)
                {
                    RTP_AMR_PRINT("qvp_rtp_parse_amr_fixed_hdr_oa ERROR %d\n", err);
                    return -1;
                }
                bufType.head_room = 0;
                offset = 0;
                temp_bufType_data += RTP_FIXED_HEADER_SIZE - residual_len;
                org_len -= (RTP_FIXED_HEADER_SIZE - residual_len);
                residual_len = 0;
                rtp_residual_buf = temp_rtp_residual_buf;
                memset(rtp_residual_buf, 0, sizeof(uint8) * RTP_RESIDUAL_BUFFER_SIZE);
                variable_stream_ptr = temp_bufType_data;
                extract_new_amr_frames();
            }
            packet_cnt++;
        }
    }

    //For the first time when there is no data in the residual buffer the packets are taken
    //directly from the fresh buffer

    bufType.data = temp_bufType_data;
    bufType.len = org_len;
    RTP_AMR_PRINT("Taking data from new buffer\n");


    //Loop until all the length of the data in the fresh buffer becomes <=100 bytes
    //This is just to ensure that the residual buffer always contains data starting
    //with RTP header
    while(bufType.data)
    {
        RTP_AMR_PRINT("--------------------------------------------------------------\n");
        RTP_AMR_PRINT("PACKET - %lu\n", packet_cnt);
        RTP_AMR_PRINT("--------------------------------------------------------------\n");
        err = qvp_rtp_unpack(ctxStruct, &bufType);

        if(err != QVP_RTP_SUCCESS)
        {
            RTP_AMR_PRINT("Returned with error %d\n", err);
            return -1;
        }
        RTP_AMR_PRINT("bufType.parse_offset = %d\n", bufType.parse_offset);
        RTP_AMR_PRINT("Payload: \n");

        offset = bufType.parse_offset/8;
        err1 = qvp_rtp_parse_amr_fixed_hdr_oa((bufType.data + offset), bufType.len, &hdr);
        if(err1 == 0)
        {
            RTP_AMR_PRINT("qvp_rtp_parse_amr_fixed_hdr_oa ERROR %d\n", err);
            return -1;
        }
        bufType.len--;

        //Counting the number of frames in the packet
        frame_cnt = qvp_rtp_amr_count_tocs_oa((bufType.data + offset + QVP_RTP_AMR_OL_FIXED_HDR_SIZE), bufType.len - QVP_RTP_AMR_OL_FIXED_HDR_SIZE);
        RTP_AMR_PRINT("No. of frames present in the packet: %lu\n", frame_cnt);

        aud_len = bufType.len - QVP_RTP_AMR_OL_FIXED_HDR_SIZE - frame_cnt;
        aud_data = bufType.data + offset + QVP_RTP_AMR_OL_FIXED_HDR_SIZE + frame_cnt;
        RTP_AMR_PRINT("aud_len = %lu\n", aud_len);

        frame_index = 0;

        length = 0;

        //Extract the AMR frames
        while(frame_index < frame_cnt)
        {
            RTP_AMR_PRINT("--------------------------------\n");
            RTP_AMR_PRINT("frame - %c of packet - %lu\n", frame_index + 1, packet_cnt);
            RTP_AMR_PRINT("--------------------------------\n");
            toc = *(bufType.data + offset + QVP_RTP_AMR_OL_FIXED_HDR_SIZE + offset_temp);

            offset_temp++;
            FT = (toc & 0x78) >> 3;
            fqi = (toc & 0x04) >> 2;
            if(((FT > QVP_RTP_AMR_FT_RATE_SID) && (FT < QVP_RTP_AMR_FT_RATE_NOD)) || ( FT > QVP_RTP_AMR_FT_RATE_NOD))
            {
                RTP_AMR_PRINT("Invalid FT\n");
                return -1;
            }
            if(FT == QVP_RTP_AMR_FT_RATE_NOD)
            {
                len = 0;
            }
            else
            {
                len = (amr_ft_len_table[FT] + 7) / 8;
            }
            RTP_AMR_PRINT("len - %d\n", len);

            if(aud_len < len)
            {
                RTP_AMR_PRINT("Insufficient aud_len\n");
                return -1;
            }
            if(bufType.len < len)
            {
                RTP_AMR_PRINT("Data present in bufType.data is less\n");
                break;
            }
            *(aud_buf->data) = toc & 0x7f;
            memcpy( aud_buf->data + 1, aud_data, len);

            RTP_AMR_PRINT("Contents of aud_buf->data:\n");
            for(i = 0; i < len + 1; i++)
            {
                RTP_AMR_PRINT("%x ", aud_buf->data[i]);
            }
            RTP_AMR_PRINT("\n");
            aud_buf->data += len + 1;
            aud_data += len;
            aud_buf->len += len + 1;
            length += len;
            RTP_AMR_PRINT("\n\n");
            bufType.len -= 1 + len;
            frame_index++;

            if((bufType.len <= 0) && (frame_index < frame_cnt))
            {
                RTP_AMR_PRINT("Need some more data\n");
                break;
            }
        }

        bufType.data +=  offset + QVP_RTP_AMR_OL_FIXED_HDR_SIZE + frame_cnt + length;

        bufType.head_room = 0;
        aud_len = 0;
        aud_data = 0;
        offset = 0;
        offset_temp = 0;
        aud_buf->len = 0;
        out_len += frame_cnt + length;
        RTP_AMR_PRINT("out_len = %lu\n", out_len);

        if(bufType.len > 0)
        {
            //RTP_AMR_PRINT("*(bufType.data) = %x\n", *(bufType.data));
            if((((*bufType.data) & RTP_VERSION_MASK) != RTP_VERSION) || //If RTP version != 2 or
                (*(bufType.data+1) != RTP_PAYLOAD_TYPE))        //if payload type != AMR
            {
                RTP_AMR_PRINT("\nEND PROCESSING\n");
                break;
            }
            packet_cnt++;
        }
        else
        {
            RTP_AMR_PRINT("Full data has been consumed:\n");
            break;
        }

        //When the data length in the input buffer becomes <=100 then copy all
        //the remaining data into residual buffer and update the residual length.
        if(bufType.len <= RTP_RESIDUAL_BUFFER_SIZE && bufType.len > 0)
        {
            memcpy(rtp_residual_buf, bufType.data, bufType.len);
            residual_len = bufType.len;
            bufType.len = 0;
            //bufType.data = NULL;
            break;
        }
    }
    aud_buf->data = aud_buf_data;
    aud_buf->len = out_len;

    return 0;
}

