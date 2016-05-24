/******************************************************************************
  @file oem_rapi_test
  @brief Linux user-space application to test remote_api of OEM_RAPI.

  DESCRIPTION
  Test and example on how to use OEM_RAPI interface.

  -----------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  when       who      what, where, why
  --------   ---      -------------------------------------------------------
  09/21/09            Initial version

======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include "oncrpc.h"
#include "oem_rapi.h"
#include "oem_rapi_rpc.h"


/*=====================================================================
      Constants and Macros
======================================================================*/

/*=====================================================================
     Forward Declaration
======================================================================*/
void oem_rapi_null_test_info(void);
int  oem_rapi_null_test(void);
int oem_rapi_test_Setup(void);
int oem_rapi_test_Cleanup(void);
extern void oem_rapicb_app_init(void);

/*===========================================================================

FUNCTION      oem_rapi_test_Setup

DESCRIPTION
   Initializes ONCRPC routines for the test

DEPENDENCIES
   None

RETURN VALUE
   1 - Test setup successful

SIDE EFFECTS
   None

===========================================================================*/
int oem_rapi_test_Setup()
{
   oncrpc_init();
   oncrpc_task_start();

   printf("oem_rapicb_app_init...");
   oem_rapicb_app_init();

   printf("Starting OEM_RAPI Tests...");
   fflush(stdout);

   return 1;
}/* end oem_rapi_test_Setup */


/*===========================================================================
FUNCTION oem_rapi_null_test

DESCRIPTION
  This function verifies null interface function of OEM_RAPI API

DEPENDENCIES
  None

RETURN VALUE
  returns 0 on success, 1 on failure

SIDE EFFECTS
  None

===========================================================================*/
int oem_rapi_null_test(void)
{
  uint32 found;

  printf("Verifying Null Interface for API: OEM_RAPI\n");
  found = oem_rapi_null();
  if(!found)
  {
     printf("NOT OK\n");
     printf("ONCRPC NULL INTERFACE TEST FOR OEM_RAPI API COMPLETE...\n");
     return 0;
  }
  else
  {
     printf("OK\n");
     printf("ONCRPC NULL INTERFACE TEST FOR OEM_RAPI API COMPLETE...\n");
     return 1;
  }
}


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
#define OEM_RAPI_STREAMING_TEST_DATA_SIZE_IN (OEM_RAPI_MAX_SERVER_INPUT_BUFFER_SIZE)
#define OEM_RAPI_STREAMING_TEST_DATA_SIZE_OUT (OEM_RAPI_MAX_CLIENT_OUTPUT_BUFFER_SIZE )
int oem_rapi_streaming_function_Test(void)
{
    uint32 i;
    int return_status =  1;
	oem_rapi_client_event_e_type event = OEM_RAPI_CLIENT_EVENT_NONE;
	oem_rapi_streaming_cb_type *callback
			= oem_rapi_Test_streaming_function_callback;
	void *handle = "some_string";
    /* in_len and input buffer contains data we send */
    uint32 in_len = OEM_RAPI_STREAMING_TEST_DATA_SIZE_IN;
    byte input[OEM_RAPI_STREAMING_TEST_DATA_SIZE_IN];
    /* The out_len and output data will be set by the RPC code */
	uint32 out_len =0;
    byte output[OEM_RAPI_STREAMING_TEST_DATA_SIZE_OUT];

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

   printf("Received OUTPUT Length %u \n",(unsigned int)out_len);
   if(out_len != 0)
   {
     printf(">>>DATA:");
     for(i=0;i<out_len;i++)
     {
       printf("%d,",output[i]);
       if(output[i] != input[i])
       {
         return_status = 0;
         printf("Data does not match at index %d i, expected %u got %u \n",(int)i,
                (unsigned int)input[i],(unsigned int)output[i]);
       }
     }
     printf("<<<<\n");
     if(return_status == 1)
     {
       printf("Verified data match\n");
     }
   }
   return return_status;
}

/*===========================================================================
  FUNCTION main
===========================================================================*/
/*!
@brief
  Main program entry

@return
  0

@note
  - Dependencies
    - ONCRPC
  - Side Effects
*/
/*=========================================================================*/
int main(int argc, char **argv)
{

  /* set max allowed threads */
  oem_rapi_test_Setup();

  if(!oem_rapi_null_test())
  {
     printf("FAIL\n");
     return -1;
  }

  if(!oem_rapi_streaming_function_Test())
  {
     printf("FAIL\n");
     return -1;
  }

  printf("PASS\n");
  return 0;
} /* End main */

