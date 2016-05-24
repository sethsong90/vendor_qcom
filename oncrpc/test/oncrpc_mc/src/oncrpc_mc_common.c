/******************************************************************************
  @file  oncrpc_mc_common.c
  @brief Contains common function definitions for oncrpc_mc_test_xxxx

  -----------------------------------------------------------------------------
  Copyright (c) 2010, Qualcomm Technologies, Inc., All rights reserved.
  -----------------------------------------------------------------------------
******************************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "oncrpc.h"
#include "oncrpc_mc_common.h"
#include "ping_mdm_rpc.h"
#include "ping_mdm_rpc_rpc.h"
#include "oem_rapi.h"
#include "oem_rapi_rpc.h"

/*===========================================================================
FUNCTION oem_rapi_Test_streaming_function_callback

DESCRIPTION
  Callback function for oem_rapi

DEPENDENCIES
  None

RETURN VALUE
  n/a

SIDE EFFECTS
  None

===========================================================================*/
void oem_rapi_Test_streaming_function_callback
(
oem_rapi_server_event_e_type  event,    /* Server event */
        void *handle,   /* Handle to client data */
        uint32 in_len,   /* Input size */
        byte * input,    /* Input buffer */
        uint32 * out_len,  /* Output size */
        byte * output    /* Output buffer */
)
{
        printf("Callback function called\n");
}

/*===========================================================================
FUNCTION       oem_rapi_streaming_function_Test

DESCRIPTION  Test OEM RAPI by sending a max size block of data.
             This require the default OEM_RAPI implementation on the remote
             server side.

DEPENDENCIES  OEM_RAPI remote server default "echo" implementation.

RETURN VALUE

   1  - returns on success.
   0  - returns on failure

SIDE EFFECTS
   None
===========================================================================*/
int oem_rapi_streaming_function_Test(unsigned int data_size_words)
{
    uint32 i;
    int return_status =  0;
    oem_rapi_client_event_e_type event = OEM_RAPI_CLIENT_EVENT_NONE;
    oem_rapi_streaming_cb_type *callback = NULL;
    void *handle = "some_string";
    /* in_len and input buffer contains data we send */
    uint32 in_len;
    byte input[OEM_RAPI_STREAMING_TEST_DATA_SIZE_IN];
    /* The out_len and output data will be set by the RPC code */
    uint32 out_len =0;
    byte output[OEM_RAPI_STREAMING_TEST_DATA_SIZE_OUT];

    in_len = (data_size_words * 4) < OEM_RAPI_STREAMING_TEST_DATA_SIZE_IN ?
               (data_size_words * 4): OEM_RAPI_STREAMING_TEST_DATA_SIZE_IN;

    for(i=0;i<in_len;i++)
    {
      input[i]=i+1;
    }

    /* calling remote api */
    oem_rapi_streaming_function(event,
                                callback,
                                handle,
                                in_len,
                                input,
                                &out_len,
                                output);

   if(out_len != 0)
   {
     for(i=0;i<out_len;i++)
     {
       if(output[i] != input[i])
       {
         return_status = -1;
         printf("Data does not match at index %d i, expected %u got %u \n",(int)i,
                (unsigned int)input[i],(unsigned int)output[i]);
       }
     }
   }
   return return_status;
}

/*======================================================================
FUNCTION: ping_data_common
======================================================================*/
/*
Description: Function to initialize the data and start the ping test
*/
/*====================================================================*/
int ping_data_common(unsigned int data_size_words)
{
  uint32 sum=0,retsum;
  uint32 *data,i;
  int rc=0;
  /* Setup data block */
  if(data_size_words > PING_MAX_WORDS) {
     data_size_words = PING_MAX_WORDS;
  }

  data = malloc(data_size_words * sizeof(uint32));
  for( i=0;i<data_size_words;i++ )
  {
    data[i]=rand();
    sum=sum^data[i];
  }
  retsum = ping_mdm_data(data,data_size_words);
  if(sum!=retsum) {
     printf("%s: RPC Call checksum failure\n", __func__);
     rc = -1;
  }
  free(data);
  return rc;

}
