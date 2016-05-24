/******************************************************************************

                      D S _ S O C K E T _ T E S T . C

******************************************************************************/

/******************************************************************************

  @file    ds_socket_test.c
  @brief   DSS API Interactive Test Program

  DESCRIPTION
  Implementation of an interactive test program for DSS API.

  ---------------------------------------------------------------------------
  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/test/dss/src/ds_socket_test.c#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
11/06/07   vk         Using safe string functions
09/28/07   vk         Initial version

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "stringl.h"
#include "dssocket.h"
#include "ds_fd_pass.h"

#define OPT_CLIENT 0x1
#define OPT_SERVER 0x2
#define OPT_NATIVE 0x4
#define OPT_OPENSERVER 0x8
#define OPT_UDP 0x10
#define OPT_NET 0x20

char ipaddr[128];
int  portnum;
char message[] = "Quick brown fox jumps over a lazy dog.";
int  slowmode;

sint15 Nethdl;
void * Sock_cb_user_data = (void *)0x10405060;
sint15 Sockfd;
uint32 Event_mask;

pthread_mutex_t main_mutx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  main_cond = PTHREAD_COND_INITIALIZER;

void Sleep (int sec) {
	if (slowmode) {
		sleep(sec);
	}
}

void exit_on_eof (void) {
	printf("eof detected, exiting..\n");
	exit(0);
}

void
sigusr1_handler (int sig)
{
    signal(SIGUSR1, &sigusr1_handler);
    printf("in sigusr1_handler\n");
}

void 
wait_for_sock_event (void)
{
	printf("in wait_for_sock_event\n");

    if (pthread_mutex_lock(&main_mutx) != 0) {
        printf("sock_cb: mutex_lock failed!\n");
        assert(0);
    }

    if (pthread_cond_wait(&main_cond, &main_mutx) != 0) {
        printf("sock_cb: cond_signal failed!\n");
        assert(0);
    }

    if (pthread_mutex_unlock(&main_mutx) != 0) {
        printf("sock_cb: mutex_unlock failed!\n");
        assert(0);
    }

    #if 0
	if (!Event_mask) {
    	pause();
	} 
	Event_mask = 0;
    #endif
}

void 
sock_cb 
(
  sint15 dss_nethandle,                                  /* Application id */
  sint15 sockfd,                                      /* socket descriptor */
  uint32 event_mask,                                     /* Event occurred */
  void * sock_cb_user_data       /* User data specfied during registration */
)
{
    assert(Nethdl == dss_nethandle);
    assert(sock_cb_user_data == Sock_cb_user_data);
    assert(sockfd == Sockfd);
    Event_mask = event_mask;

    if (pthread_mutex_lock(&main_mutx) != 0) {
        printf("sock_cb: mutex_lock failed!\n");
        assert(0);
    }

    if (pthread_cond_signal(&main_cond) != 0) {
        printf("sock_cb: cond_signal failed!\n");
        assert(0);
    }

    if (pthread_mutex_unlock(&main_mutx) != 0) {
        printf("sock_cb: mutex_unlock failed!\n");
        assert(0);
    }

    return;
}

sint15
do_tcp_server_accept (sint15 fd)
{
    sint15 newfd;
    struct sockaddr_in saddr_clnt;
    uint16 saddr_clnt_len;
    sint15 ds_errno;
	sint31 event;

    printf("in do_tcp_server_accept\n");

    if (dss_async_select(fd, DS_ACCEPT_EVENT, &ds_errno) != DSS_SUCCESS) {
        printf("dss_async_select failed!\n");
        assert(0);
    }
    wait_for_sock_event();
    if ((event = dss_getnextevent(Nethdl, &fd, &ds_errno)) == DSS_ERROR) {
        assert(0);
    }
    if (event == DS_ACCEPT_EVENT) {
        printf("DS_ACCEPT_EVENT returned..\n");
    } else {
        assert(0);
    }

    if ((newfd = dss_accept(fd, (struct sockaddr *)&saddr_clnt, &saddr_clnt_len, &ds_errno)) == DSS_ERROR) {
        printf("error in accept!\n");
        assert(0);
    }
    return newfd;
}

void
do_tcp_client_connect (sint15 fd, char * ipaddr, int port)
{
    struct sockaddr_in saddr_to;
    sint15 retval;
    sint15 ds_errno;
    sint31 event;

    printf("in do_tcp_client_connect\n");

    memset(&saddr_to, 0, sizeof(saddr_to));
    saddr_to.sin_family = AF_INET;
    saddr_to.sin_port = port;

    if (inet_aton(ipaddr, &saddr_to.sin_addr) == 0) {
        printf("error in inet_aton call!\n");
        exit(0);
    }

    while ((retval = dss_connect(fd, (struct sockaddr *)&saddr_to, sizeof(struct sockaddr_in), &ds_errno))
           != DSS_SUCCESS) 
    {
        printf("dss_connect returned DSS_ERROR\n");
		if (ds_errno == DS_EISCONN) {
			break;
		}
        if (ds_errno != DS_EINPROGRESS) {
            printf("ds_errno not DS_EINPROGRESS!\n");
            assert(0);
        }
        if (dss_async_select(fd, DS_WRITE_EVENT, &ds_errno) != DSS_SUCCESS) {
            printf("dss_async_select failed!\n");
            assert(0);
        }
        wait_for_sock_event();
        if ((event = dss_getnextevent(Nethdl, &fd, &ds_errno)) == DSS_ERROR) {
            assert(0);
        }
        if (event == DS_WRITE_EVENT) {
            printf("DS_WRITE_EVENT returned..\n");
        } else {
            assert(0);
        }
    }
    return;
}

void
do_sendto (sint15 fd, char * msg, int lmsg, char * ipaddr, int port)
{
    int nbytes;
    int index = 0;
    sint15 ds_errno;
    sint31 event;
	int orig_lmsg = lmsg;
    struct sockaddr_in saddr_to;

    memset(&saddr_to, 0, sizeof(saddr_to));
    saddr_to.sin_family = AF_INET;
    saddr_to.sin_port = port;

    if (inet_aton(ipaddr, &saddr_to.sin_addr) == 0) {
        printf("error in inet_aton call!\n");
        assert(0);
    }

    while (lmsg) {
        printf("calling dss_sendto..\n");
        if ((nbytes = dss_sendto(fd, msg + index, lmsg, 0, (struct sockaddr *)&saddr_to, sizeof(saddr_to), &ds_errno)) == DSS_ERROR) {
            if (ds_errno == DS_EWOULDBLOCK) {
                if (dss_async_select(fd, DS_WRITE_EVENT, &ds_errno) != DSS_SUCCESS) {
                    assert(0);
                }
                wait_for_sock_event();
                if ((event = dss_getnextevent(Nethdl, & fd, &ds_errno)) == DSS_ERROR) {
                    assert(0);
                }
                if (event == DS_WRITE_EVENT) {
                    printf("DS_WRITE_EVENT returned..\n");
                } else if (event != 0) {
                    assert(0);
                } else {
                    assert(0);
                }
            } else {
                assert(0);
            }
			continue;
        }
        assert(nbytes == lmsg);
        lmsg -= nbytes;
		assert(lmsg >= 0);
        index += nbytes;
    }
	assert(index == orig_lmsg);
    return;
}

void
do_write (sint15 fd, char * msg, int lmsg)
{
    int nbytes;
    int index = 0;
    sint15 ds_errno;
    sint31 event;
	int orig_lmsg = lmsg;

    while (lmsg) {
        printf("calling dss_write..\n");
        if ((nbytes = dss_write(fd, msg + index, lmsg, &ds_errno)) == DSS_ERROR) {
            if (ds_errno == DS_EWOULDBLOCK) {
                if (dss_async_select(fd, DS_WRITE_EVENT, &ds_errno) != DSS_SUCCESS) {
                    assert(0);
                }
                wait_for_sock_event();
                if ((event = dss_getnextevent(Nethdl, & fd, &ds_errno)) == DSS_ERROR) {
                    assert(0);
                }
                if (event == DS_WRITE_EVENT) {
                    printf("DS_WRITE_EVENT returned..\n");
                } else if (event != 0) {
                    assert(0);
                } else {
                    assert(0);
                }
            } else {
                assert(0);
            }
			continue;
        }
        lmsg -= nbytes;
		assert(lmsg >= 0);
        index += nbytes;
    }
	assert(index == orig_lmsg);
    return;
}

int
do_read_native (int fd, char * msg, int lmsg)
{
	int nread;
    if ((nread = read(fd, msg, lmsg)) < 0) {
        printf("error in read!\n");
        assert(0);
    }
	return nread;
}

void
do_write_native (int fd, char * msg, int lmsg)
{
	int nwrote;

	while (lmsg) {
    	if ((nwrote = write(fd, msg, lmsg)) < 0) {
        	perror("error in write!\n");
        	assert(0);
    	}
		lmsg -= nwrote;
		msg = msg + nwrote;
	}
	return;
}

int
do_recvfrom (sint15 fd, char * msg, int lmsg)
{
	int nread;
    sint15 ds_errno;
    sint31 event;
    struct sockaddr_in saddr_from;
    uint16 addrlen;

    if ((nread = dss_recvfrom(fd, msg, lmsg, 0, (struct sockaddr *)&saddr_from, &addrlen, &ds_errno)) != DSS_ERROR) {
		return nread;
	}

	assert(ds_errno == DS_EWOULDBLOCK);

    if (dss_async_select(fd, DS_READ_EVENT, &ds_errno) != DSS_SUCCESS) {
        assert(0);
    }
    wait_for_sock_event();
    if ((event = dss_getnextevent(Nethdl, &fd, &ds_errno)) == DSS_ERROR) {
        assert(0);
    }
    if (event == DS_READ_EVENT) {
        printf("DS_READ_EVENT returned..\n");
    } else if (event != 0) {
        assert(0);
    } else {
		printf("NULL event returned..\n");
        assert(0);
	}

    if ((nread = dss_recvfrom(fd, msg, lmsg, 0, (struct sockaddr *)&saddr_from, &addrlen, &ds_errno)) == DSS_ERROR) {
        printf("error in dss_recvfrom, ds_errno = %d!\n", ds_errno);
        //assert(0);
    }
	return nread;
}

int
do_read (sint15 fd, char * msg, int lmsg)
{
	int nread;
    sint15 ds_errno;
    sint31 event;

    if ((nread = dss_read(fd, msg, lmsg, &ds_errno)) != DSS_ERROR) {
		return nread;
	}

	assert(ds_errno == DS_EWOULDBLOCK);

    if (dss_async_select(fd, DS_READ_EVENT, &ds_errno) != DSS_SUCCESS) {
        assert(0);
    }
    wait_for_sock_event();
    if ((event = dss_getnextevent(Nethdl, &fd, &ds_errno)) == DSS_ERROR) {
        assert(0);
    }
    if (event == DS_READ_EVENT) {
        printf("DS_READ_EVENT returned..\n");
    } else if (event != 0) {
        assert(0);
    } else {
		printf("NULL event returned..\n");
        assert(0);
	}

    if ((nread = dss_read(fd, msg, lmsg, &ds_errno)) == DSS_ERROR) {
        printf("error in read, ds_errno = %d!\n", ds_errno);
        //assert(0);
    }
	return nread;
}

void
do_udp_client (sint15 nethdl, char * ipaddr, int port, char * msg, int lmsg)
{
    sint15 fd;
    sint15 ds_errno;

    if ((fd = dss_socket(nethdl, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &ds_errno))
         == DSS_ERROR) 
    {
        printf("dss_socket returned error!\n");
        exit(0);
    }
    Sockfd = fd;

    while (1) {
        do_sendto(fd, msg, lmsg, ipaddr, port);
        Sleep(5);
    }
}

void
do_tcp_client (sint15 nethdl, char * ipaddr, int port, char * msg, int lmsg)
{
    sint15 fd;
    sint15 ds_errno;
    int i;
    dss_so_linger_type lingeropt;
	uint32 lingeropt_len;

    if ((fd = dss_socket(nethdl, AF_INET, SOCK_STREAM, IPPROTO_TCP, &ds_errno))
         == DSS_ERROR) 
    {
        printf("dss_socket returned error!\n");
        exit(0);
    }
    Sockfd = fd;

    do_tcp_client_connect (fd, ipaddr, port);
    #if 1
    while (1) {
        do_write(fd, msg, lmsg);
        Sleep(5);
    }
    #endif
	#if 0
    for (i = 0; i < 100; ++i) {
        do_write(fd, msg, lmsg);
    }

    lingeropt.l_onoff = 1;
    lingeropt.l_linger = 2000;
	lingeropt_len = sizeof(dss_so_linger_type);
    if (dss_setsockopt(fd, DSS_SOL_SOCKET, DSS_SO_LINGER, &lingeropt, &lingeropt_len, &ds_errno) != DSS_SUCCESS) {
        printf("dss_setsockopt failed with errno = %d\n", ds_errno);
        return;
    }
	printf("setsockopt succ with l_linger = %d\n", lingeropt.l_linger);
    if (dss_close(fd, &ds_errno) != DSS_SUCCESS) {
        printf("dss_close returned error, ds_errno = %d!\n", ds_errno);
    } else {
		printf("close returned success\n");
	}
	#endif
    return;
}

void
do_tcp_client_native (char * ipaddr, int port, char * msg, int lmsg)
{
    int fd;
    struct sockaddr_in saddr_to;

    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket returned error!\n");
        assert(0);
    }
    Sockfd = fd;

    memset(&saddr_to, 0, sizeof(saddr_to));
    saddr_to.sin_family = AF_INET;
    saddr_to.sin_port = port;

    if (inet_aton(ipaddr, &saddr_to.sin_addr) == 0) {
        printf("error in inet_aton call!\n");
        assert(0);
    }

    if (connect(fd, (struct sockaddr *)&saddr_to, sizeof(struct sockaddr_in)) < 0) {
        perror("connect returned error!\n");
        assert(0);
    }

    while (1) {
        do_write_native(fd, msg, lmsg);
        Sleep(5);
    }
}

int 
do_get_fd_from_os (void)
{
    int ufd_rpc;
    int ufd;
    int sfd;
    char spath[] = "/tmp/vaibhavk_tmp_uds_clnt.socket";
    int spathlen;

#ifdef FEATURE_DS_LINUX_TEST_OS

    spathlen = sizeof(spath);

    if ((ufd_rpc = ds_open_uds_clnt_for_rpc()) < 0) {
        printf("ds_open_uds_clnt_for_rpc failed!\n");
        assert(0);
    }

	unlink(spath);
    if ((ufd = ds_open_uds_srvr(spath)) < 0) {
        printf("ds_open_uds_srvr failed!\n");
        assert(0);
    }
    if (ds_send_socket_cmd_over_uds(ufd_rpc, spath, spathlen) < 0) {
        printf("ds_send_socket_cmd_over_uds failed!\n");
        assert(0);
    } 

    if ((sfd = ds_recv_fd(ufd)) < 0) {
        printf("ds_recv_fd returned error!\n");
        assert(0);
    }

#else

#endif

    return sfd;
}

void
do_tcp_client_os (sint15 nethdl, char * ipaddr, int port, char * msg, int lmsg)
{
    sint15 fd;
    sint15 ds_errno;

    ds_errno = do_get_fd_from_os();
    if ((fd = dss_socket(nethdl, AF_INET, SOCK_STREAM, IPPROTO_TCP, &ds_errno))
         == DSS_ERROR) 
    {
        printf("dss_socket returned error!\n");
        exit(0);
    }
    Sockfd = fd;

    do_tcp_client_connect (fd, ipaddr, port);
    while (1) {
        do_write(fd, msg, lmsg);
        Sleep(5);
    }
}

void
do_udp_client_os (sint15 nethdl, char * ipaddr, int port, char * msg, int lmsg)
{
    int fd;
    sint15 ds_errno;
    struct sockaddr_in saddr_to;
	int rval;

    fd = do_get_fd_from_os();
    Sockfd = fd;

    memset(&saddr_to, 0, sizeof(saddr_to));
    saddr_to.sin_family = AF_INET;
    saddr_to.sin_port = port;

    if (inet_aton(ipaddr, &saddr_to.sin_addr) == 0) {
        printf("error in inet_aton call!\n");
        assert(0);
    }

    if ((rval = sendto(fd, msg, lmsg, 0, (struct sockaddr *)&saddr_to, sizeof(saddr_to))) < 0) {
        perror("sendto call failed!\n");
        assert(0);
    } else if (rval != lmsg) {
        printf("sendto sent less bytes than message length!\n");
    }

    printf("Sent test packet..\n");
    return;
}

void
clear_buffer (char * buf, int buflen)
{
	memset(buf, 0, buflen);
}

void
match_data (char * msg, int msglen)
{
	static int index = 0;
	int lmatch;
	int msgindex = 0;
	while (msglen)
	{
		lmatch = sizeof(message)-1 - index;
		if (lmatch > msglen) {
			lmatch = msglen;
		}
		if (strncmp(message+index, msg+msgindex, lmatch) != 0) {
			printf("error in matching data!\n");
			assert(0);
		}
		index += lmatch;
		if (index >= sizeof(message)-1) {
			assert(index == sizeof(message)-1);
			index = 0;
			printf("matched\n");
		}
		msglen -= lmatch;
		assert(msglen >= 0);
		msgindex += lmatch;
	}
}

void
do_tcp_server_native (char * ipaddr, int port)
{
    int fd;
    struct sockaddr_in saddr_my, saddr_clnt;
    int saddr_clnt_len;
    char msg[128];
    int lmsg = 128;
    int lmsgread;
	int index;

    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("socket call returned error!\n");
        assert(0);
    }

    memset(&saddr_my, 0, sizeof(saddr_my));
    saddr_my.sin_family = AF_INET;
    saddr_my.sin_port = port;

    if (inet_aton(ipaddr, &saddr_my.sin_addr) == 0) {
        printf("error in inet_aton call!\n");
        assert(0);
    }

    if (bind(fd, (struct sockaddr *)&saddr_my, sizeof(saddr_my)) < 0) {
        printf("error in bind!\n");
        assert(0);
    }

    if (listen(fd, 1) < 0) {
        printf("error in listen!\n");
        assert(0);
    }

    if ((Sockfd = accept(fd, (struct sockaddr *)&saddr_clnt, &saddr_clnt_len)) < 0) {
        printf("error in accept!\n");
        assert(0);
    }

    while (1) {
        lmsgread = do_read_native(Sockfd, msg, lmsg);
		if (lmsgread == 0)
			exit_on_eof();
        /* printf("%s\n", msg); */
		match_data(msg, lmsgread);
		clear_buffer(msg, lmsg);
    }
}

void
do_udp_server (sint15 nethdl, char * ipaddr, int port)
{
    sint15 fd;
    struct sockaddr_in saddr_my;
    char msg[128];
    int lmsg = 128;
    int lmsgread;
	int index;
    sint15 ds_errno;

    if ((fd = dss_socket(nethdl, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &ds_errno))
         == DSS_ERROR) 
    {
        printf("dss_socket returned error!\n");
        exit(0);
    }

    memset(&saddr_my, 0, sizeof(saddr_my));
    saddr_my.sin_family = AF_INET;
    saddr_my.sin_port = port;

    if (inet_aton(ipaddr, &saddr_my.sin_addr) == 0) {
        printf("error in inet_aton call!\n");
        assert(0);
    }

    if (dss_bind(fd, (struct sockaddr *)&saddr_my, sizeof(saddr_my), &ds_errno) == DSS_ERROR) {
        printf("error in dss_bind!\n");
        assert(0);
    }

	Sockfd = fd;

    while (1) {
        lmsgread = do_recvfrom(Sockfd, msg, lmsg);
		if (lmsgread > 0) {
        	/* printf("%s\n", msg); */
			match_data(msg, lmsgread);
			clear_buffer(msg, lmsg);
		}
    }
}

void
do_tcp_server (sint15 nethdl, char * ipaddr, int port)
{
    sint15 fd;
    struct sockaddr_in saddr_my;
    char msg[128];
    int lmsg = 128;
    int lmsgread;
	int index;
    sint15 ds_errno;

    if ((fd = dss_socket(nethdl, AF_INET, SOCK_STREAM, IPPROTO_TCP, &ds_errno))
         == DSS_ERROR) 
    {
        printf("dss_socket returned error!\n");
        exit(0);
    }

    memset(&saddr_my, 0, sizeof(saddr_my));
    saddr_my.sin_family = AF_INET;
    saddr_my.sin_port = port;

    if (inet_aton(ipaddr, &saddr_my.sin_addr) == 0) {
        printf("error in inet_aton call!\n");
        assert(0);
    }

    if (dss_bind(fd, (struct sockaddr *)&saddr_my, sizeof(saddr_my), &ds_errno) == DSS_ERROR) {
        printf("error in dss_bind!\n");
        assert(0);
    }

    if (dss_listen(fd, 1, &ds_errno) == DSS_ERROR) {
        printf("error in dss_listen!\n");
        assert(0);
    }

	Sockfd = fd;
    Sockfd = do_tcp_server_accept(fd);

    while (1) {
        lmsgread = do_read(Sockfd, msg, lmsg);
		if (lmsgread == 0)
			exit_on_eof();
		if (lmsgread > 0) {
        	/* printf("%s\n", msg); */
			match_data(msg, lmsgread);
			clear_buffer(msg, lmsg);
		}
    }
}

int
do_openserver_open_socket (const char * dev, int domain, int type, int protocol)
{
    int fd;
    if ((fd = socket(domain, type, protocol)) < 0) {
        perror("socket call failed!");
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, dev, sizeof(char *)) < 0) {
        perror("setsockopt call failed!");
        close(fd);
        return -1;
    }
    return fd;
}

void
do_openserver (const char *dev, int mode)
{
    int ufd;
    int sfd;
    int tufd;
    char spath[128];
    int spathlen;

#ifdef FEATURE_DS_LINUX_TEST_OS

    if ((ufd = ds_open_uds_srvr_for_rpc()) < 0) {
        printf("ds_open_uds_srvr_for_rpc failed!\n");
        assert(0);
    }
    while (1) {
        spathlen = sizeof(spath)-1;
        memset(spath, 0, spathlen+1);
        if (ds_recv_socket_cmd_over_uds(ufd, spath, &spathlen) < 0) {
            printf("ds_recv_socket_cmd_over_uds failed!\n");
            continue;
        } 
        spath[spathlen] = '\0';
        if (mode == IPPROTO_TCP ) {
            if ((sfd = do_openserver_open_socket(dev, AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                printf("do_openserver_open_socket failed!\n");
                continue;
            }
        } else {
            if ((sfd = do_openserver_open_socket(dev, AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
                printf("do_openserver_open_socket failed!\n");
                continue;
            }
        }
        if ((tufd = ds_open_uds_clnt(spath)) < 0) {
            printf("ds_open_uds_clnt failed!\n");
            close(sfd);
            continue;
        }
        if (ds_send_fd(tufd, sfd) < 0) {
            printf("ds_send_fd failed!\n");
        }
		printf("returned socket %d to client\n", sfd);
        close(tufd);
        close(sfd);
    }

#endif /* FEATURE_DS_LINUX_TEST_OS */
}

void dss_test_net_cb_fcn
(
  sint15            dss_nethandle,                       /* Application id */
  dss_iface_id_type iface_id,                    /* Interfcae id structure */
  sint15            dss_errno, /* type of network error, ENETISCONN, ENETNONET.*/
  void            * net_cb_user_data               /* Call back User data  */
)
{
    printf("In dss_test_net_cb_fcn: nethandle = %d, iface_id = %ld, errno = %d, net_cb_user_data = %d\n",
           dss_nethandle, iface_id, dss_errno, (int) net_cb_user_data);
}

void dss_test_sock_cb_fcn
(
  sint15 dss_nethandle,                                  /* Application id */
  sint15 sockfd,                                      /* socket descriptor */
  uint32 event_mask,                                     /* Event occurred */
  void * sock_cb_user_data       /* User data specfied during registration */
)
{
    return;
}

int dss_test_net_cb_user_data = 5;
int dss_test_sock_cb_user_data = 7;

int dsnet_get_handle_test (int arg1, int arg2) {
    dss_net_policy_info_type net_policy;
    sint15 dss_errno;

    dss_init_net_policy_info(&net_policy);

    net_policy.iface.kind = DSS_IFACE_NAME;
    net_policy.iface.info.name = DSS_IFACE_UMTS;
    net_policy.umts.pdp_profile_num = 1;

    return dsnet_get_handle(&dss_test_net_cb_fcn, (void *)dss_test_net_cb_user_data,
                            &dss_test_sock_cb_fcn, (void *)dss_test_sock_cb_user_data,
                            &net_policy, &dss_errno);
}

int dsnet_release_handle_test (int arg1, int arg2) {
    sint15 dss_errno;

    return dsnet_release_handle(arg1, &dss_errno);
}

#ifndef FEATURE_DS_NO_DCM

int dsnet_start_test (int arg1, int arg2) {
    sint15 dss_errno;

    return dsnet_start(arg1, &dss_errno);
}

int dsnet_stop_test (int arg1, int arg2) {
    sint15 dss_errno;

    return dsnet_stop(arg1, &dss_errno);
}

int dss_netstatus_test (int arg1, int arg2) {
    sint15 dss_errno;
    int rval;

    rval = dss_netstatus(arg1, &dss_errno);

    return dss_errno;
}

int dss_get_iface_id_test (int arg1, int arg2) {
    return dss_get_iface_id(arg1);
}

#endif /* !FEATURE_DS_NO_DCM */

void dss_ev_cb_test_func
(
  dss_iface_ioctl_event_enum_type          event,
  dss_iface_ioctl_event_info_union_type    event_info,
  void                                     *user_data,
  signed short int                         dss_nethandle,
  dss_iface_id_type                        iface_id
)
{
    printf("dss_ev_cb_test_func: event = %d, user_data = %d, nh = %d, if_id = %d\n",
           event, (int)user_data, dss_nethandle, iface_id);
}

#ifndef FEATURE_DS_NO_DCM 

int dss_reg_ev_cb_test (int nh, int if_id) {
    sint15 dss_errno;
    dss_iface_ioctl_ev_cb_type ev_cb;
    int rval = -1;

    ev_cb.event_cb = &dss_ev_cb_test_func;
    ev_cb.user_data_ptr = (void *)5;
    ev_cb.dss_nethandle = nh;

    ev_cb.event = DSS_IFACE_IOCTL_DOWN_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_REG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_UP_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_REG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_GOING_DOWN_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_REG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_COMING_UP_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_REG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    return rval;
}

int dss_dereg_ev_cb_test (int nh, int if_id) {
    sint15 dss_errno;
    dss_iface_ioctl_ev_cb_type ev_cb;
    int rval = -1;

    ev_cb.event_cb = &dss_ev_cb_test_func;
    ev_cb.user_data_ptr = (void *)5;
    ev_cb.dss_nethandle = nh;

    ev_cb.event = DSS_IFACE_IOCTL_DOWN_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_DEREG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_UP_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_DEREG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_GOING_DOWN_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_DEREG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    ev_cb.event = DSS_IFACE_IOCTL_COMING_UP_EV;
    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_DEREG_EVENT_CB,
                           &ev_cb,
                           &dss_errno);

    if (rval < 0) {
        return rval;
    }

    return rval;
}

int dss_ioctl_get_if_name_test (int if_id, int arg2) {
    sint15 dss_errno;
    int rval = -1;
    dss_iface_ioctl_iface_name_type name;

    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_GET_IFACE_NAME,
                           &name,
                           &dss_errno);

    if (rval == 0) {
        printf("name = %x\n", name);
    }
    return rval;
}

int dss_ioctl_get_if_state_test (int if_id, int arg2) {
    sint15 dss_errno;
    int rval = -1;
    dss_iface_ioctl_state_type state;

    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_GET_STATE,
                           &state,
                           &dss_errno);

    if (rval == 0) {
        printf("state = %x\n", state);
    }
    return rval;
}

int dss_ioctl_get_if_addr_test (int if_id, int arg2) {
    sint15 dss_errno;
    int rval = -1;
    dss_iface_ioctl_ipv4_addr_type addr;

    rval = dss_iface_ioctl(if_id, 
                           DSS_IFACE_IOCTL_GET_IPV4_ADDR,
                           &addr,
                           &dss_errno);

    if (rval == 0) {
        printf("addr = %x\n", addr);
    }
    return rval;
}

#endif /* !FEATURE_DS_NO_DCM */

int dss_socket_udp_test (int nh, int arg2) {
    sint15 fd;
    sint15 ds_errno;

    if ((fd = dss_socket(nh, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &ds_errno))
         == DSS_ERROR) 
    {
        printf("dss_socket returned error!\n");
        exit(0);
    }
    Sockfd = fd;
    strlcpy(ipaddr, "10.46.86.132", 128);
    portnum = 1234;

    return fd;
}

int dss_sendto_test (int fd, int arg2) {
    do_sendto(fd, message, sizeof(message)-1, ipaddr, portnum);
    return 0;
}

typedef int (* dsc_test_cmd_f) (int arg1, int arg2);

struct dsc_test_cmd_s {
    dsc_test_cmd_f cmd_f;
    char * desc;
};

struct dsc_test_cmd_s dsc_test_cmd_arr[] = {
	{ dsnet_get_handle_test, "dsnet_get_handle_test <dummy> <dummy>" },
	{ dsnet_release_handle_test, "dsnet_release_handle_test <nh> <dummy>" },
#ifndef FEATURE_DS_NO_DCM
	{ dsnet_start_test, "dsnet_start_test <nh> <dummy>" }, 
    { dsnet_stop_test, "dsnet_stop_test <nh> <dummy>" },
    { dss_netstatus_test, "dss_netstatus_test <nh> <dummy>" },
    { dss_get_iface_id_test, "dss_get_iface_id_test <nh> <dummy>" },
    { dss_reg_ev_cb_test, "dss_reg_ev_cb_test <nh> <if_id>" },
    { dss_dereg_ev_cb_test, "dss_dereg_ev_cb_test <nh> <if_id>" },
    { dss_ioctl_get_if_name_test, "dss_ioctl_get_if_name_test <if_id> <dummy>" },
    { dss_ioctl_get_if_state_test, "dss_ioctl_get_if_state_test <if_id> <dummy>" },
    { dss_ioctl_get_if_addr_test, "dss_ioctl_get_if_addr_test <if_id> <dummy>" },
#endif /* !FEATURE_DS_NO_DCM */
    { dss_socket_udp_test, "dss_socket_udp_test <nh> <dummy>" },
    { dss_sendto_test, "dss_sendto_test <sfd> <dummy>" }
};

void dsc_test_cmd_print (void)
{
    int i;
	int arrsize;

	arrsize = sizeof(dsc_test_cmd_arr)/sizeof(struct dsc_test_cmd_s);

    for (i = 0; i < arrsize; ++i) {
        printf("%d %s\n", i+1, dsc_test_cmd_arr[i].desc);
    }
    return;
}


void do_test_interactive (void)
{
    char buf[128];
    int ret;
    int cmd;
    int arg1, arg2;

    while (1) {
        dsc_test_cmd_print();
        fgets(buf, 128, stdin);
        ret = sscanf(buf, "%d %d %d", &cmd, &arg1, &arg2);
        if (ret < 3) {
            if (cmd == 0) {
                break;
            } else {
                printf("invalid input. ignoring..\n");
            }
        } else {
            ret = (* dsc_test_cmd_arr[cmd-1].cmd_f)(arg1, arg2);
            printf("%s returned %d\n", dsc_test_cmd_arr[cmd-1].desc, ret);
        }
    }
    return;
}

void
do_test (unsigned int options, char * ipaddr, int port, char * msg, int lmsg)
{
    sint15 nethdl;
    sint15 ds_errno;

	if (options & OPT_NET) {
		do_test_interactive();
		return;
	}

    signal(SIGUSR1, &sigusr1_handler);

    nethdl = dss_open_netlib2(&dss_test_net_cb_fcn, NULL, &sock_cb, Sock_cb_user_data, NULL, &ds_errno);

    if (nethdl == DSS_ERROR) {
        printf("dss_open_netlib returned error!\n");
        exit(0);
    }

    Nethdl = nethdl;

    if (options & OPT_CLIENT) {
        if (options & OPT_OPENSERVER) {
            if (options & OPT_UDP) {
                do_udp_client_os(nethdl, ipaddr, port, msg, lmsg);
            } else {
                do_tcp_client_os(nethdl, ipaddr, port, msg, lmsg);
            }
        } else if (options & OPT_NATIVE) {
            do_tcp_client_native(ipaddr, port, msg, lmsg);
        } else {
            if (options & OPT_UDP) {
                do_udp_client(nethdl, ipaddr, port, msg, lmsg);
            } else {
                do_tcp_client(nethdl, ipaddr, port, msg, lmsg);
            }
        }
    } else if (options & OPT_SERVER) {
        if (options & OPT_OPENSERVER) {
            if (options & OPT_UDP) {
                do_openserver(ipaddr, IPPROTO_UDP);
            } else {
                do_openserver(ipaddr, IPPROTO_TCP); /* ipaddr is really devname */
            }
        } else if (options & OPT_NATIVE) {
            do_tcp_server_native(ipaddr, port);
        } else { 
            if (options & OPT_UDP) {
                do_udp_server(nethdl, ipaddr, port);
            } else {
                do_tcp_server(nethdl, ipaddr, port);
            }
        }
    } else {
        printf("what do you want from me!\n");
        exit(0);
    }
}

int 
main (int argc, char * argv[])
{
	int i = 1;
    unsigned int options = 0;

    if (argc < 4) {
        printf("not enough arguments!\n");
        exit(0);
    }

	if (strcmp(argv[i], "-c") == 0) {
        options |= OPT_CLIENT;
		printf("Running as tcp client\n");
	} else if (strcmp(argv[i], "-s") == 0) {
		options |= OPT_SERVER;
		printf("Running as tcp server\n");
    } else if (strcmp(argv[i], "-cu") == 0) {
        options |= OPT_CLIENT;
        options |= OPT_UDP;
        printf("Running as udp client\n");
    } else if (strcmp(argv[i], "-su") == 0) {
        options |= OPT_SERVER;
        options |= OPT_UDP;
        printf("Running as udp server\n");
	} else if (strcmp(argv[i], "-cn") == 0) {
		options |= OPT_CLIENT;
        options |= OPT_NATIVE;
		printf("Running as client using native sockets\n");
	} else if (strcmp(argv[i], "-sn") == 0) {
		options |= OPT_SERVER;
        options |= OPT_NATIVE;
		printf("Running as server using native sockets\n");
    } else if (strcmp(argv[i], "-os") == 0) {
        options |= OPT_OPENSERVER;
		options |= OPT_SERVER;
        printf("Running as open server tcp mode\n");
    } else if (strcmp(argv[i], "-osu") == 0) {
        options |= OPT_OPENSERVER;
		options |= OPT_SERVER;
        options |= OPT_UDP;
        printf("Running as open server udp mode\n");
    } else if (strcmp(argv[i], "-oc") == 0) {
        options |= OPT_OPENSERVER;
		options |= OPT_CLIENT;
        printf("Running as tcp client using open server\n");
    } else if (strcmp(argv[i], "-ocu") == 0) {
        options |= OPT_OPENSERVER;
		options |= OPT_CLIENT;
        options |= OPT_UDP;
        printf("Running as udp client using open server\n");
    } else if (strcmp(argv[i], "-net") == 0) {
        options |= OPT_NET;
        printf("Running in interactive mode for net control testing\n");
	} else {
		printf("client/server option not specified!\n");
		exit(0);
	}

	++i;
    strlcpy(ipaddr, argv[i++], 128);
    portnum = atoi(argv[i++]);

	if (i < argc) {
		if (strcmp(argv[i++], "-slow") == 0) {
			slowmode = 1;
		}
	}

	//dss_init();
    #ifndef FEATURE_DS_LINUX_NO_RPC

	oncrpc_init();
	//time_remote_mtoa_app_init();
	dsc_dcm_apicb_app_init();
	oncrpc_task_start();
	sleep(10);

    #endif

    do_test(options, ipaddr, portnum, message, sizeof(message)-1);
}

int rex_self(void) { return 1; }
