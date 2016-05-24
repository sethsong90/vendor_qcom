/******************************************************************************
  ---------------------------------------------------------------------------
  Copyright (c) 2012 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
*******************************************************************************/

#include "qmi_client.h"
#include "qmi_idl_lib.h"
#include "qmi_cci_target_ext.h"
#include <string.h>
#include "test_service_v01.h"
static int pending_inds = 0;
static int pending_async = 0;

/*=============================================================================
  CALLBACK FUNCTION test_service_ind_cb
=============================================================================*/
/*!
@brief
  This callback function is called by the QCCI infrastructure when
  infrastructure receives an indication for this client

@param[in]   user_handle         Opaque handle used by the infrastructure to
				 identify different services.

@param[in]   msg_id              Message ID of the indication

@param[in]  ind_buf              Buffer holding the encoded indication

@param[in]  ind_buf_len          Length of the encoded indication

@param[in]  ind_cb_data          Cookie value supplied by the client during registration

*/
/*=========================================================================*/
void test_service_ind_cb
(
 qmi_client_type                user_handle,
 unsigned long                  msg_id,
 unsigned char                  *ind_buf,
 int                            ind_buf_len,
 void                           *ind_cb_data
)
{
	--pending_inds;
	printf("TEST_SERVICE: Indication: msg_id=0x%x buf_len=%d pending_ind=%d\n", (unsigned int)msg_id,
		   ind_buf_len, pending_inds);
}

/*=============================================================================
  CALLBACK FUNCTION test_service_rx_cb
=============================================================================*/
/*!
@brief
  This callback function is called by the QCCI infrastructure when
  infrastructure receives an asynchronous response for this client

@param[in]   user_handle         Opaque handle used by the infrastructure to
				 identify different services.

@param[in]   msg_id              Message ID of the response

@param[in]   buf                 Buffer holding the decoded response

@param[in]   len                 Length of the decoded response

@param[in]   resp_cb_data        Cookie value supplied by the client

@param[in]   transp_err          Error value

*/
/*=========================================================================*/
void test_service_rx_cb
(
 qmi_client_type                user_handle,
 unsigned long                  msg_id,
 void                           *buf,
 int                            len,
 void                           *resp_cb_data,
 qmi_client_error_type          transp_err
 )
{
	--pending_async;
	/* Print the appropriate message based on the message ID */
	switch (msg_id)
	{
		case QMI_TEST_RESP_V01:
			printf("TEST: Async Test Response: %s\n",((test_ping_resp_msg_v01 *)buf)->pong);
			break;
		case QMI_TEST_DATA_RESP_V01:
			printf("TEST: Async Test Data Length: %d\n",((test_data_resp_msg_v01 *)buf)->data_len);
			break;
		case QMI_TEST_DATA_IND_REG_RESP_V01:
			printf("TEST: Async Test Data Ind Registration Success.\n");
			break;
		default:
			break;
	}
}

/*=============================================================================
  FUNCTION test_service_basic_test
=============================================================================*/
/*!
@brief
  This function sends a number of basic test_service messages asynchronously

@param[in]   clnt                Client handle needed to send messages

@param[in]   txn                 Transaction handle

@param[in]   num_msgs            Number of messages to send

*/
/*=========================================================================*/
void test_service_basic_test
(
	qmi_client_type *clnt,
	qmi_txn_handle *txn,
	int num_msgs
)
{
	int i,rc;
	test_ping_req_msg_v01 req;
	test_ping_resp_msg_v01 resp;
	/* Set the value of the basic test_service request */
	req.client_name_valid = 0;
	memcpy(&req, "ping", 4);
	printf("TEST: Basic Ping Test with %d async ping messages.\n",num_msgs);
	for (i=0;i<num_msgs;++i)
	{
		rc = qmi_client_send_msg_async(*clnt, QMI_TEST_REQ_V01, &req, sizeof(req),
									   &resp, sizeof(resp), test_service_rx_cb, 2, txn);
		printf("TEST: qmi_client_send_msg_async returned %d on loop %d\n", rc,i);
		if (rc != 0){
			printf("TEST: send_msg_async error: %d\n",rc);
		  exit(1);
		}
		++pending_async;
		usleep(200);
	}
	/* Wait until all pending async messages have been received */
	while (pending_async != 0)
	{
		usleep(500);
	}
}

/*=============================================================================
  FUNCTION test_service_data_test
=============================================================================*/
/*!
@brief
  This function sends a number of data  test_service messages asynchronously

@param[in]   clnt                Client handle needed to send messages

@param[in]   txn                 Transaction handle

@param[in]   num_msgs             Number of data messages to send

@param[in]   msg_size            Size of data messages to send

*/
/*=========================================================================*/
void test_service_data_test
(
	qmi_client_type *clnt,
	qmi_txn_handle *txn,
	int num_msgs,
	int msg_size
)
{
	int i,rc;
	test_data_req_msg_v01 *data_req;
	test_data_resp_msg_v01 *data_resp;
	data_req = (test_data_req_msg_v01*)malloc(sizeof(test_data_req_msg_v01));
	if (!data_req)
	{
		printf("data_req memory alloc failed\n");
		return;
	}
	data_resp = (test_data_resp_msg_v01*)malloc(sizeof(test_data_resp_msg_v01));
	if (!data_resp)
	{
		printf("data_resp memory alloc failed\n");
		free(data_req);
		return;
	}

	memset( data_req, 0, sizeof(test_data_req_msg_v01) );
	memset( data_resp, 0, sizeof(test_data_resp_msg_v01) );
	data_req->data_len = msg_size;
	printf("TEST: Data Test with %d async data messages of size %d.\n",num_msgs,msg_size);
	for (i=0;i<num_msgs;++i)
	{
		rc = qmi_client_send_msg_async(*clnt, QMI_TEST_DATA_REQ_V01, data_req, sizeof(test_data_req_msg_v01),
									   data_resp, sizeof(test_data_resp_msg_v01), test_service_rx_cb, 2, txn);
		printf("TEST: qmi_client_send_msg_async returned %d on loop %d\n", rc,i);
		if (rc != 0){
			printf("TEST: send_msg_async error: %d\n",rc);
		  exit(1);
		}
		++pending_async;
		usleep(500);
	}
	/* Wait until all pending async messages have been received */
	while (pending_async != 0)
	{
		usleep(500);
	}
	free(data_req);
	free(data_resp);
}

/*=============================================================================
  FUNCTION test_service_ind_test
=============================================================================*/
/*!
@brief
  This function tells the service to send a specified number of indication messages

@param[in]   clnt                Client handle needed to send messages

@param[in]   txn                 Transaction handle

@param[in]   num_inds            Number of indications for the service to send

@param[in]   ind_size            Size of indications for the service to send

@param[in]   delay               Amount of time the server should wait between indications

*/
/*=========================================================================*/
void test_service_ind_test
(
	qmi_client_type *clnt,
	qmi_txn_handle *txn,
	int num_inds,
	int ind_size,
	int delay
)
{
	int i,rc;
	test_data_ind_reg_req_msg_v01 data_ind_reg_req;
	test_data_ind_reg_resp_msg_v01 data_ind_reg_resp;

	/* Set the number of pending indications */
	pending_inds = num_inds;
	memset( &data_ind_reg_req, 0, sizeof(test_data_ind_reg_req_msg_v01) );
	memset( &data_ind_reg_resp, 0, sizeof(test_data_ind_reg_resp_msg_v01) );
	data_ind_reg_req.num_inds_valid = 1;
	data_ind_reg_req.num_inds = num_inds;
	/* Send the optional TLVs if these values are passed as arguments */
	if (delay > 0)
	{
		data_ind_reg_req.ms_delay_valid = 1;
		data_ind_reg_req.ms_delay = delay;
	}
	if (ind_size > 0)
	{
		data_ind_reg_req.ind_size_valid = 1;
		data_ind_reg_req.ind_size = ind_size;
	}
	printf("TEST: Data Indication Test with %d indications of size %d.\n",num_inds,ind_size);
	rc = qmi_client_send_msg_async(*clnt, QMI_TEST_DATA_IND_REG_REQ_V01, &data_ind_reg_req,
								   sizeof(data_ind_reg_req),&data_ind_reg_resp,
								   sizeof(data_ind_reg_resp), test_service_rx_cb, 2, txn);
	printf("TEST: qmi_client_send_msg_async returned %d\n", rc);
	if (rc != 0){
		printf("TEST: send_msg_async error: %d\n",rc);
		exit(1);
	}
	++pending_async;
	usleep(200);
	/* Wait until all pending async messages have been received */
	while (pending_async != 0)
	{
		usleep(500);
	}
	/* Wait until all pending indications have been received */
	while (pending_inds != 0)
	{
		usleep(500);
	}
}

/*=============================================================================
  FUNCTION main
=============================================================================*/
int main(int argc, char **argv)
{
  qmi_txn_handle txn;
  uint32_t num_services, num_entries=0;
  int rc,service_connect;
  enum test_service_tests{
	  TEST_SERVICE_BASIC_TEST = 1,
	  TEST_SERVICE_DATA_TEST,
	  TEST_SERVICE_IND_TEST
  }test_service_tests_type;
  int i;
  /* Set the defaults for the test_service test arguments */
  int test_service_test_args[3] = {1,0,0};
  qmi_client_type clnt, notifier;
  qmi_cci_os_signal_type os_params;

  qmi_service_info info[10];
  uint32_t port = 10001;

  /* Get the service object for the test_service API */
  qmi_idl_service_object_type test_service_object = test_get_service_object_v01();
  /* Verify that test_get_service_object did not return NULL */
  if (!test_service_object)
  {
	  printf("TEST: test_get_serivce_object failed, verify test_service_v01.h and .c match.\n");
  }

  rc = qmi_client_notifier_init(test_service_object, &os_params, &notifier);

  /* Check if the service is up, if not wait on a signal */
  while(1)
  {
    rc = qmi_client_get_service_list( test_service_object, NULL, NULL, &num_services);
    printf("TEST: qmi_client_get_service_list() returned %d num_services = %d\n", rc, num_services);
    if(rc == QMI_NO_ERR)
      break;
    /* wait for server to come up */
    QMI_CCI_OS_SIGNAL_WAIT(&os_params, 0);
  };

  num_entries = num_services;
  /* The server has come up, store the information in info variable */
  rc = qmi_client_get_service_list( test_service_object, info, &num_entries, &num_services);
  printf("TEST: qmi_client_get_service_list() returned %d num_entries = %d num_services = %d\n", rc, num_entries, num_services);
  service_connect = 0;
  if (num_services > 1)
  {
	printf("%d Test Services found: Choose which one to connect to (numbered starting at 0)\n",num_services);
	do
        {
	  scanf("%d",&service_connect);
        } while(service_connect >= num_services);
  }

  rc = qmi_client_init(&info[service_connect], test_service_object, test_service_ind_cb, NULL, NULL, &clnt);

  printf("TEST: qmi_client_init returned %d\n", rc);
  /* If additional arguments have beens upplied to choose a specific test */
  if (argc > 1)
  {
	  test_service_tests_type = atoi(argv[1]);
	  for (i=2; i<argc; ++i)
	  {
		  test_service_test_args[i-2] = atoi(argv[i]);
	  }
	  switch(test_service_tests_type)
	  {
		  case TEST_SERVICE_BASIC_TEST:
			  test_service_basic_test(&clnt,&txn,test_service_test_args[0]);
			  break;
		  case TEST_SERVICE_DATA_TEST:
			  test_service_data_test(&clnt,&txn,test_service_test_args[0],test_service_test_args[1]);
			  break;
		  case TEST_SERVICE_IND_TEST:
			  test_service_ind_test(&clnt,&txn,test_service_test_args[0],test_service_test_args[1],test_service_test_args[2]);
			  break;
		  default:
			  printf("TEST: Unrecognized test number: %d\n",test_service_tests_type);
	  }
  }else{
	  /* No args passed, perform basic single test_service test */
	  test_service_basic_test(&clnt,&txn,1);
  }

  rc = qmi_client_release(clnt);
  printf("TEST: qmi_client_release of clnt returned %d\n", rc);

  rc = qmi_client_release(notifier);
  printf("TEST: qmi_client_release of notifier returned %d\n", rc);
  return 0;
}
