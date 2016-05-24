 /******************************************************************************
  @file  test_api_client
  @brief Linux user-space test_api client.

  DESCRIPTION
  Test program for htorpc metacomment testing.
  -----------------------------------------------------------------------------
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  -----------------------------------------------------------------------------
 ******************************************************************************/
/*=====================================================================
                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who      what, where, why
--------   ---      -------------------------------------------------------
06/01/2009 fr       Initial Version.
======================================================================*/
/*=====================================================================

                     INCLUDE FILES FOR MODULE

======================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "oncrpc.h"
#include "test_api.h"
#include "test_api_rpc.h"

#define VARIABLE_ARRAY_LENGTH 50

#define PRINT_PASS_FAIL(ret_val,str) if(ret_val == -1) { printf("TEST::%s::FAILED\n",str);}\
else {printf("TEST::%s::PASS\n",str);}\


/*===========================================================================
  FUNCTION  initialize_oncrpc
===========================================================================*/
/*!
@brief
  Initializes oncrpc and starts the oncrpc task
@return
  - N/A

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/

void initialize_oncrpc(void)
{
  oncrpc_init();
  printf("Oncrpc init done\n");
  oncrpc_task_start();
  printf("Oncrpc task start done \n");
}

/*===========================================================================
  FUNCTION  initialize_client_side_fixed_array
===========================================================================*/
/*!
@brief
  Initialize the data and include the checksum in the last element of the array
@return
  - N/A

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/

void initialize_client_side_fixed_array ( array_t array, uint32 len)
{
    uint32 index;
    uint32 sum = 0;

    if (len == 0) {
        len = TEST_API_ARRAY_DATA_SIZE;
    }

    for (index = 0; index < len - 1; index++ ) {
        array[index] =  index;
        sum ^= index;
    }
    array[len - 1] = sum;
}


/*===========================================================================
  FUNCTION  initialize_client_side_variable_array
===========================================================================*/
/*!
@brief
  Initialize the data and include the checksum in the last element of the array

@return
  - N/A

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/

void initialize_client_side_variable_array ( uint32 *array, uint32 len)
{
    uint32 index;
    uint32 sum = 0;

    if (len == 0) {
        len = TEST_API_ARRAY_DATA_SIZE;
    }
    for (index = 0; index < len - 1; index++ ) {
        array[index] =  index;
        sum ^= index;
    }
    array[len - 1] = sum;
}

/*===========================================================================
  FUNCTION  test_fixed_array
===========================================================================*/
/*!
@brief
  Test for fixed array
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/


int test_fixed_array()
{
    array_t array_data;
    uint32 server_data_integ;
    uint32 client_data_integ;
    uint32 index;
    uint32 sum = 0;
	int ret_val = 0;

    initialize_client_side_fixed_array(array_data, 0);
    server_data_integ = test_api_verify_fixed_array(array_data);

    for (index = 0; index < TEST_API_ARRAY_DATA_SIZE - 1; index++ ) {
        sum ^= array_data[index];
    }
    client_data_integ = sum ^ array_data[TEST_API_ARRAY_DATA_SIZE - 1];

    if (server_data_integ == 0 && client_data_integ == 0) {
		ret_val = 0;
    }
    else
    {
        ret_val = -1;
    }
	return ret_val;
}

/*===========================================================================
  FUNCTION  test_callback_server_implementation
===========================================================================*/
/*!
@brief
  Test to demonstrate callback server implementation
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/

int test_callback_server_implementation()
{
	array_t array_data;
	test_api_registration_ret_code ret_val;
	uint32 clnt_id;
	uint32 task_init;
	uint32 callback_retval = 1;
	uint32 callback_time = 10;
	int test_ret_val = 0;


	initialize_client_side_fixed_array(array_data, 0);

	task_init = test_api_start_service(1);
	if (task_init ==  0)
	{
		printf("Test Api task created\n");
	}
	else
	{
		printf("Test Api task already created \n");
	}
	ret_val = test_api_verify_fixed_array_register_callback( test_api_verify_fixed_array,
															 array_data,
															 callback_time,
															 1,
															 &clnt_id
															 );
   if (ret_val ==  TEST_API_SUCCESS)
   {
	   printf("Registration successful, Client ID provided:%d\n",clnt_id);
   }
   else
   {
	   printf("Registration not successful, probably task is not yet created or Maximum client supported reached\n");
   }

   /* This is polling for the return value on the servers database by making a forward rpc call
      The best solution is to wait in the test for signal from a global callback function that
	  tells the test that callback has been executed and it' ok to continue */

   do {
		callback_retval = test_api_callback_retval( clnt_id,TEST_API_FIXED_ARRAY);
   } while ( callback_retval != 0);

   if (callback_retval == 0)
	{
		printf("Callback executed successfully on client side \n");
	}
	else
	{
		printf("Callback not executed successfully on client side \n");
		test_ret_val = -1;
	}

	test_api_remove_service(1);
	return test_ret_val;
}

/*===========================================================================
  FUNCTION  test_fixed_array_not_full
===========================================================================*/
/*!
@brief
  Test to demonstrate a fixed array that is not full
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/

int test_fixed_array_not_full()
{
    array_t array_data;
    uint32 server_data_integ;
    uint32 client_data_integ;
    uint32 index;
    uint32 sum = 0;
    uint32 array_length = 70;
	int test_ret_val = 0;


    initialize_client_side_fixed_array(array_data,array_length);
    server_data_integ = test_api_verify_fixed_array_not_full(array_data,array_length);

    for (index = 0; index < array_length - 1; index++ ) {
        sum ^= array_data[index];
    }
    client_data_integ = sum ^ array_data[array_length - 1];


    test_ret_val =  (server_data_integ == 0 &&  client_data_integ == 0) ? 0 : -1;
	return test_ret_val;
}

/*===========================================================================
  FUNCTION  test_variable_array
===========================================================================*/
/*!
@brief
  Test to demonstrate sending/receiving variable array
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/
int test_variable_array()
{
    uint32 *array_data;
    uint32 server_data_integ;
    uint32 client_data_integ;
    uint32 index;
    uint32 sum = 0;
    uint32 array_length = 70;
	int test_ret_val = 0;

    array_data = malloc(sizeof(uint32) * array_length );

    initialize_client_side_variable_array(array_data,array_length);
    server_data_integ = test_api_verify_variable_array(array_data,array_length);

    for (index = 0; index < array_length - 1; index++ ) {
        sum ^= array_data[index];
    }

    client_data_integ = sum ^ array_data[array_length - 1];

	test_ret_val = (server_data_integ == 0 && client_data_integ == 0)? 0 : -1;
    free(array_data);
	return test_ret_val;
}

/*===========================================================================
  FUNCTION  test_my_data_struct_1
===========================================================================*/
/*!
@brief
  Test to demonstrate sending/receiving data of type test_api_struct_t_1
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/

int test_my_data_struct_1(uint32 length, string_t str )
{
    test_api_struct_t_1 *data;
    uint32 server_data_integ;
    uint32 client_fixed_data_integ;
    uint32 client_variable_data_integ;
    uint32 index;
    uint32 sum_fixed = 0;
    uint32 sum_varray = 0;
	int test_ret_val = 0;

    data = malloc(sizeof (test_api_struct_t_1));

    data->variable_len = length;
    data->variable_array = malloc(sizeof(uint32) * data->variable_len);
    data->str = str;

    initialize_client_side_fixed_array(data->fixed_array, 0);
    initialize_client_side_variable_array(data->variable_array,data->variable_len);

    server_data_integ = test_api_verify_struct_1_data(data);

    for (index = 0; index < TEST_API_ARRAY_DATA_SIZE - 1; index++ ) {
        sum_fixed ^= data->fixed_array[index];
    }

    client_fixed_data_integ = sum_fixed ^ data->fixed_array[TEST_API_ARRAY_DATA_SIZE - 1];


    for (index = 0; index < data->variable_len - 1; index++ ) {
        sum_varray ^= data->variable_array[index];
    }

    client_variable_data_integ = sum_varray ^ data->variable_array[data->variable_len - 1];

	test_ret_val = (client_fixed_data_integ == 0 && client_variable_data_integ == 0 && server_data_integ == 0) ? 0 : -1;

    free(data);
    free(data->variable_array);

	return test_ret_val;
}


/*===========================================================================
  FUNCTION  test_my_data_union_1
===========================================================================*/
/*!
@brief
  Test to demonstrate sending/receiving data of type test_api_union_1
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/


int test_my_data_union_1(uint32 length, string_t str )
{
  test_api_union_t_1 *data;
  test_api_union_t_1 *fixed_array_data;
  uint32 server_data_integ;
  uint32 client_fixed_data_integ;
  uint32 client_variable_data_integ;
  uint32 index;
  uint32 sum_fixed = 0;
  uint32 sum_varray = 0;
  int test_ret_val = 0;

  data = malloc(sizeof(test_api_union_t_1));
  fixed_array_data = malloc(sizeof (test_api_union_t_1));
  data->struct_data = malloc(sizeof(test_api_struct_t_1));
  data->struct_data->variable_array = malloc(sizeof(uint32) * length);
  data->struct_data->variable_len = length;
  data->struct_data->str = str;

  initialize_client_side_fixed_array(fixed_array_data->fixed_array, 0);

  initialize_client_side_variable_array(data->struct_data->variable_array,length);

  initialize_client_side_fixed_array(data->struct_data->fixed_array,0);

  /* sending struct in the union */
  server_data_integ = test_api_verify_union_1_data ( data, SEND_STRUCT_ONLY);

  for (index = 0; index < TEST_API_ARRAY_DATA_SIZE - 1; index++ )
  {
    sum_fixed ^=  data->struct_data->fixed_array[index];
  }

  sum_fixed ^= data->struct_data->fixed_array[ TEST_API_ARRAY_DATA_SIZE - 1 ];

  for (index = 0; index < data->struct_data->variable_len - 1; index++ )
  {
     sum_varray ^=  data->struct_data->variable_array[index];
  }

  sum_varray ^= data->struct_data->variable_array[data->struct_data->variable_len - 1];

  test_ret_val = (sum_fixed == 0 && sum_varray == 0 && server_data_integ == 0 ) ? 0 : -1;

  /* Sending fixed array in the union */

  sum_fixed = 0;

  server_data_integ = test_api_verify_union_1_data ( fixed_array_data, SEND_FIXED_ARRAY );

  for (index = 0; index < TEST_API_ARRAY_DATA_SIZE - 1; index++ )
  {
    sum_fixed ^=  fixed_array_data->fixed_array[index];
  }

  sum_fixed ^= fixed_array_data->fixed_array[TEST_API_ARRAY_DATA_SIZE - 1];

  test_ret_val =  ( sum_fixed == 0 && server_data_integ == 0 ) ? 0 : -1;

  free(data->struct_data->variable_array);
  free(data->struct_data);
  free(data);
  free(fixed_array_data);
  return test_ret_val;
}

/*===========================================================================
  FUNCTION  test_my_data_union_2
===========================================================================*/
/*!
@brief
  Test to demonstrate sending/receiving data of type test_api_union_t_2
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/


int test_my_data_union_2 ( uint32 length, string_t str)
{
  test_api_union_t_2 *data;
  test_api_union_t_2 *fixed_array_data;
  uint32 server_data_integ;
  uint32 index;
  uint32 sum_fixed = 0;
  uint32 sum_varray = 0;
  int test_ret_val = 0;


  data = malloc(sizeof(test_api_union_t_2));
  fixed_array_data = malloc(sizeof(test_api_union_t_2));
  data->struct_data.variable_array = malloc(sizeof(uint32)*length);
  data->struct_data.variable_len = length;
  data->struct_data.str = str;

  initialize_client_side_fixed_array(fixed_array_data->fixed_array, 0);

  initialize_client_side_fixed_array(data->struct_data.fixed_array,0);

  initialize_client_side_variable_array(data->struct_data.variable_array, length);

  /* Sending struct only */
  server_data_integ = test_api_verify_union_2_data(data,SEND_STRUCT_ONLY);

  for (index = 0; index < TEST_API_ARRAY_DATA_SIZE - 1; index++ )
  {
    sum_fixed ^=  data->struct_data.fixed_array[index];
  }

  sum_fixed ^= data->struct_data.fixed_array[ TEST_API_ARRAY_DATA_SIZE - 1 ];

  for (index = 0; index < data->struct_data.variable_len - 1; index++ )
  {
     sum_varray ^=  data->struct_data.variable_array[index];
  }

  sum_varray ^= data->struct_data.variable_array[data->struct_data.variable_len - 1];

  if (sum_fixed == 0 && sum_varray == 0 && server_data_integ == 0 )
  {
     printf("TEST FOR UNION-2 PASSED \n");
  }
  else
  {
    printf("TEST FOR UNION-2 FAILED \n");
	test_ret_val = -1;
  }

  /* Sending fixed array in the union */

  sum_fixed = 0;

  server_data_integ = test_api_verify_union_2_data ( fixed_array_data, SEND_FIXED_ARRAY );

  for (index = 0; index < TEST_API_ARRAY_DATA_SIZE - 1; index++ )
  {
    sum_fixed ^=  fixed_array_data->fixed_array[index];
  }

  sum_fixed ^= fixed_array_data->fixed_array[ TEST_API_ARRAY_DATA_SIZE - 1 ];

  if ( sum_fixed == 0 && server_data_integ == 0 )
  {
    printf(" TEST FOR UNION-2 WITH FIXED ARRAY PASSED \n");
  }
  else
  {
    printf(" TEST FOR UNION-2 WITH FIXED ARRAY FAILED \n");
	test_ret_val = -1;
  }

  free(data->struct_data.variable_array);
  free(data);
  free(fixed_array_data);
  return test_ret_val;
}

/*===========================================================================
  FUNCTION  test_my_data_struct_2
===========================================================================*/
/*!
@brief
  Test to demonstrate sending/receiving data of type test_api_struct_t_2
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/
int test_my_data_struct_2 ( uint32 length, string_t str)
{
    test_api_struct_t_2 *data;
    uint32 server_data_integ;
    uint32 sum_fixed = 0;
    uint32 sum_varray = 0;
    uint32 index;
	int test_ret_val = 0;

    data = malloc(sizeof(test_api_struct_t_2));
    data->disc = SEND_STRUCT_ONLY;
    data->union_data.struct_data.variable_array = malloc(sizeof(uint32) * length);
    data->union_data.struct_data.variable_len = length;
    data->union_data.struct_data.str = str;

    initialize_client_side_fixed_array(data->union_data.struct_data.fixed_array,0);

    initialize_client_side_variable_array(data->union_data.struct_data.variable_array,length);

    server_data_integ = test_api_verify_struct_2_data(data);

    for (index = 0; index < TEST_API_ARRAY_DATA_SIZE - 1; index++ )
    {
        sum_fixed ^=  data->union_data.struct_data.fixed_array[index];
    }

    sum_fixed ^= data->union_data.struct_data.fixed_array[ TEST_API_ARRAY_DATA_SIZE - 1 ];

    for (index = 0; index < data->union_data.struct_data.variable_len - 1; index++ )
    {
        sum_varray ^=  data->union_data.struct_data.variable_array[index];
    }

    sum_varray ^= data->union_data.struct_data.variable_array[data->union_data.struct_data.variable_len - 1];

    test_ret_val =  (sum_fixed == 0 && sum_varray == 0 && server_data_integ == 0 ) ? 0 : -1;
    free(data);
	return test_ret_val;

}

/*===========================================================================
  FUNCTION  test_my_data_struct_3
===========================================================================*/
/*!
@brief
  Test to demonstrate sending/receiving data of type test_api_struct_t_3
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/

int test_my_data_struct_3 ( uint32 length, string_t str)
{
    test_api_struct_t_3 *data;
    uint32 server_data_integ;
    uint32 index;
    uint32 sum_fixed_1 = 0;
    uint32 sum_fixed_2 = 0;
    uint32 sum_fixed_3 = 0;
    uint32 sum_varray_1 = 0;
	int test_ret_val = 0;

    data = malloc(sizeof(test_api_struct_t_3));
    data->struct_data.variable_array = malloc(sizeof(uint32)* length);
    data->disc = SEND_FIXED_ARRAY;
    data->struct_data.str = str;
    data->struct_data.variable_len = length;
    data->str = str;

    initialize_client_side_fixed_array(data->struct_data.fixed_array,0);

    initialize_client_side_variable_array(data->struct_data.variable_array,length);

    initialize_client_side_fixed_array(data->union_data.fixed_array,0);

    initialize_client_side_fixed_array(data->fixed_array,0);

    server_data_integ = test_api_verify_struct_3_data(data);

    for (index = 0; index < TEST_API_ARRAY_DATA_SIZE - 1; index++ ) {
        sum_fixed_1 ^=  data->struct_data.fixed_array[index];
    }

    sum_fixed_1 ^= data->struct_data.fixed_array[TEST_API_ARRAY_DATA_SIZE - 1];

    for (index = 0; index < length - 1; index++ ) {
        sum_varray_1 ^=  data->struct_data.variable_array[index];
    }

    sum_varray_1 ^= data->struct_data.variable_array[length - 1];

    for (index = 0; index < TEST_API_ARRAY_DATA_SIZE - 1; index++ ){
        sum_fixed_2 ^=  data->union_data.fixed_array[ index ];
    }

    sum_fixed_2 ^= data->union_data.fixed_array[ TEST_API_ARRAY_DATA_SIZE - 1 ];

    for (index = 0; index < TEST_API_ARRAY_DATA_SIZE - 1; index++ ) {
        sum_fixed_3 ^= data->fixed_array[index];
    }

    sum_fixed_3 ^= data->fixed_array[ TEST_API_ARRAY_DATA_SIZE - 1 ];


    test_ret_val = (server_data_integ == 0 &&
        sum_fixed_1 == 0 &&
        sum_fixed_2 == 0 &&
        sum_fixed_3 == 0 &&
		sum_varray_1 == 0) ? 0 : -1;

    free(data);
    free(data->struct_data.variable_array);
	return test_ret_val;

}

/*===========================================================================
  FUNCTION  test_srv_name
===========================================================================*/
/*!
@brief
  Test to demonstrate the SRVNAME metacomment
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/

int test_srv_name(string_t str)
{
    uint32 clnt_index = 0;
    uint32 server_index = 0;
    string_t str_local = "Executed handle_example_function";
    string_t str_server = NULL;
	int test_ret_val = 0;

    printf(" String before executing on server: %s\n", str );
    str_server = example_function(str);
    if (str_server != NULL ) {
         printf("String after executing on server: %s\n", str_server );

     if (strcmp(str_local,str_server) == 0)
     {
         printf("TEST FOR EXAMPLE_FUNCTION PASSED \n");
     }
     else
     {
         printf("TEST FOR EXAMPLE_FUNCTION FAILED \n");
		 test_ret_val = -1;
     }
    }
     else
     {
         printf(" Rpc message not successful \n");
		 test_ret_val = -1;
     }
	return test_ret_val;
}
/*===========================================================================
  FUNCTION  test_clnt_name
===========================================================================*/
/*!
@brief
  Test to demonstrate the SRVNAME metacomment
@return
  - Returns 0 on success -1 on failure

@note
  - Dependencies
	- N/A
  - Side Effects
	- N/A
*/
/*=========================================================================*/


int test_clnt_name(string_t str)
{
    uint32 clnt_index = 0;
    uint32 server_index = 0;
    string_t str_local = "Executed example_function_1";
    string_t str_server = NULL;
	int test_ret_val = 0;

    printf(" String before executing on server: %s\n", str );
    str_server = remote_example_function(str);
    if (str_server != NULL ) {
         printf("String after executing on server: %s\n", str_server );

     if (strcmp(str_local,str_server) == 0)
     {
         printf(" TEST FOR EXAMPLE_FUNCTION_1 PASSED \n");
     }
     else
     {
         printf(" TEST FOR EXAMPLE_FUNCTION_1 FAILED \n");
		 test_ret_val = -1;
     }
    }
     else
     {
         printf(" Rpc message not successful \n");
		 test_ret_val = -1;
     }
	return test_ret_val;
}

/*===========================================================================
  FUNCTION  main
===========================================================================*/
/*!
@brief
  program enty

@return
  rc

@note
  - Dependencies
    - N/A
  - Side Effects
*/
/*=========================================================================*/
int main(int argc, char *argv[])
{
  int rc = 0;
  initialize_oncrpc();
  /* start the callback server */
  test_apicb_app_init();

  rc = test_fixed_array();
  PRINT_PASS_FAIL(rc,"FIXED ARRAY");

  rc = test_fixed_array_not_full();
  PRINT_PASS_FAIL(rc,"FIXED ARRAY NOT FULL");

  rc = test_variable_array();
  PRINT_PASS_FAIL(rc,"VARIABLE ARRAY");

  rc = test_my_data_struct_1(200,"Test struct");
  PRINT_PASS_FAIL(rc,"STRUCT 1");

  rc = test_my_data_union_1(200,"Union test data");
  PRINT_PASS_FAIL(rc,"UNION 1");

  rc = test_my_data_union_2(200,"Test union");
  PRINT_PASS_FAIL(rc,"UNION 2");

  rc = test_my_data_struct_2(200,"Test struct 2");
  PRINT_PASS_FAIL(rc,"STRUCT 2");

  rc = test_my_data_struct_3(200,"Test struct 3");
  PRINT_PASS_FAIL(rc,"STRUCT 3");

  rc = test_srv_name("Executed function_example_handle");
  PRINT_PASS_FAIL(rc,"SRVNAME");

  rc = test_clnt_name("Executed 1_function_example");
  PRINT_PASS_FAIL(rc,"CLNTNAME");

  rc = test_callback_server_implementation();
  PRINT_PASS_FAIL(rc,"CALLBACK SERVER");

  return 0;

}



