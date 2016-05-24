/******************************************************************************

                       Q D I _ N E T L I N K . C

******************************************************************************/

/******************************************************************************

  @file    qdi_netlink.c
  @brief   QDI Netlink interface implementation file

  DESCRIPTION
  Implementation file for QDI's netlink interaction for obtaining a interface's
  IP addresses from the kernel

******************************************************************************/
/*===========================================================================

  Copyright (c) 2011-2013 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary and Confidential.

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/


/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
06/01/11   sg         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "comdef.h"
#include "qdi.h"
#include "qdi_netlink.h"
#include "qdi_debug.h"
#include "ds_util.h"


/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/* Max size of the netlink response message */
#define QDI_NL_MAX_MSG_SIZE  (1024 * 4)
#define QDI_NL_INVALID_FD    (-1)

#define QDI_NL_PID_MASK      (0x7FFFFFFF)
#define QDI_NL_PID           (getpid() & QDI_NL_PID_MASK)

/* Request message to send to the kernel to retrieve IP addresses */
typedef struct qdi_nl_getaddr_req_s
{
  struct nlmsghdr   nlh;
  struct ifaddrmsg  ifa;
} qdi_nl_getaddr_req_t;


/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  qdi_nl_get_route_socket
===========================================================================*/
/*!
@brief
  This function creates, binds and returns a netlink route socket

@param
  None

@return
  A valid socket file descriptor value - If successful
  QDI_NL_INVALID_FD                    - Otherwise

*/
/*=========================================================================*/
static int qdi_nl_get_route_socket(void)
{
  struct sockaddr_nl  src_addr;
  int                 sock_fd;

  if ((sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0)
  {
    QDI_LOG_ERROR("qdi_get_netlink_route_socket: socket() failed\n");
    sock_fd = QDI_NL_INVALID_FD;
    goto bail;
  }

  /* Initialize the source address */
  memset(&src_addr, 0, sizeof(src_addr));

  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = QDI_NL_PID;
  src_addr.nl_groups = 0; /* Interested in unicast messages */

  /* Bind the socket to our source address */
  if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0)
  {
    QDI_LOG_ERROR("qdi_get_netlink_route_socket: bind() failed\n");
    close(sock_fd);
    sock_fd = QDI_NL_INVALID_FD;
    goto bail;
  }

bail:
  return sock_fd;
}


/*===========================================================================
  FUNCTION  qdi_nl_send_getattr_req_msg
===========================================================================*/
/*!
@brief
  This function sends a request message of type RTM_GETADDR to obtain the
  IP address information from the kernel

@param
  sock_fd   - A netlink route socket
  ip_family - IP family of the addresses to return

@return
  QDI_SUCCESS - If successful
  QDI_FAILURE - Otherwise

*/
/*=========================================================================*/
static int qdi_nl_send_getaddr_req_msg
(
  int  sock_fd,
  int  af_family
)
{
  struct sockaddr_nl dest_addr;
  struct iovec iov;
  struct msghdr msg;
  qdi_nl_getaddr_req_t req;
  int ret = QDI_FAILURE;


  /* Initialize destination address structure */
  memset(&dest_addr, 0, sizeof(dest_addr));

  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid    = 0;  /* Addressed to kernel */
  dest_addr.nl_groups = 0;  /* This is a unicast message */

  /* Initialize the request message */
  memset(&req, 0, sizeof(req));

  /* Fill the netlink request message */
  req.nlh.nlmsg_len   = sizeof(req);
  req.nlh.nlmsg_pid   = 0;
  req.nlh.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
  req.nlh.nlmsg_type  = RTM_GETADDR;

  /* Set the ip family for which the addresses are requested */
  req.ifa.ifa_family = af_family;
 
  ret = sendto(sock_fd, (void*) &req, sizeof(req), 0,
               (struct sockaddr*) &dest_addr, sizeof(dest_addr));

  QDI_LOG_LOW("sendto() sock_fd=%d, ret=%d\n", sock_fd, ret);

  if (-1 != ret)
  {
    ret = QDI_SUCCESS;
  }

bail:
  return ret;
}


/*===========================================================================
  FUNCTION  qdi_nl_parse_getaddr_resp_msg
===========================================================================*/
/*!
@brief
  This function waits for a RTM_GETADDR response message from the kernel and
  then parses the message to extract IP addresses

@param
  sock_fd   - A netlink route socket
  iface_req - The iface for which IP address is being requested
  ip_family - IP family of the addresses to return
  addr      - Return the corresponding address

@return
  QDI_SUCCESS - If successful
  QDI_FAILURE - Otherwise

*/
/*=========================================================================*/
static int qdi_nl_parse_getaddr_resp_msg
(
  int              sock_fd,
  const char       *ifname,
  int              ip_family,
  dsi_addr_info_t  *addr_info,
  qmi_wds_iface_name_type tech_name
)
{
  struct iovec iov;
  struct sockaddr_nl sa;
  struct msghdr msg = { (void *)&sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
  struct nlmsghdr *nlh = NULL;
  unsigned int buflen = 0;
  int ret = QDI_FAILURE;
  void *buf = NULL;
  int fd = QDI_NL_INVALID_FD;
  ssize_t recvsize;
  struct sockaddr_in *sin   = NULL;
  struct sockaddr_in6 *sin6 = NULL;
  unsigned char req_scope = (QMI_WDS_IFACE_NAME_MODEM_LINK_LOCAL == tech_name) ? RT_SCOPE_LINK :
                                                                                 RT_SCOPE_UNIVERSE;

  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    QDI_LOG_ERROR("qdi_nl_parse_getaddr_resp_msg: failed to create socket\n");
    goto bail;
  }

  /* Allocate and initialize buffer to read message */
  buf = calloc(1, NLMSG_SPACE(QDI_NL_MAX_MSG_SIZE));
  if (NULL == buf)
  {
    QDI_LOG_ERROR("qdi_nl_parse_getaddr_resp_msg: memory alloc failure\n");
    goto bail;
  }

  iov.iov_base = buf;
  iov.iov_len  = NLMSG_SPACE(QDI_NL_MAX_MSG_SIZE);

  /* Read message from kernel */
  if ((recvsize = recvmsg(sock_fd, &msg, 0)) < 0)
  {
    QDI_LOG_ERROR("recvmsg failed");
  }
  else
  {
    buflen = recvsize;
    QDI_LOG_LOW("received response from kernel size=%d\n", buflen);
  }

  nlh = (struct nlmsghdr *)buf;

  /* Parse the message one header at a time */
  while (NLMSG_OK(nlh, buflen))
  {
    struct ifaddrmsg *ifa;
    struct rtattr *rta;
    int rtattrlen;
    struct ifreq ifr;

    ifa = (struct ifaddrmsg *)NLMSG_DATA(nlh);
    rta = (struct rtattr *)IFA_RTA(ifa);

    /* Make sure that the requested and received address family is the same */
    if (ip_family != ifa->ifa_family)
    {
      QDI_LOG_ERROR("qdi_nl_parse_getaddr_resp_msg: ip familes don't match\n");
      goto bail;
    }

    /* Get the interface name corresponding to the interface index */
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_ifindex = ifa->ifa_index;

    if (-1 == ioctl(fd, SIOCGIFNAME, &ifr))
    {
      QDI_LOG_ERROR("qdi_nl_parse_getaddr_resp_msg: failed to get iface name\n");
      ifr.ifr_name[0] = '\0';
    }
    else
    {
      ifr.ifr_name[IFNAMSIZ-1] = '\0';
      QDI_LOG_LOW("iface name=%s, scope=%d\n", ifr.ifr_name, ifa->ifa_scope);
    }

    rtattrlen = IFA_PAYLOAD(nlh);

    /* Parse the RTM_GETADDR attributes */
    while (RTA_OK(rta, rtattrlen))
    {
      switch (rta->rta_type)
      {
        case IFA_ADDRESS:
          /* If this is the required interface with the requested scope,
             copy over the address */
          if (0 == strncmp(ifname, ifr.ifr_name, IFNAMSIZ) &&
              req_scope == ifa->ifa_scope)
          {
            if (AF_INET  != ifa->ifa_family &&
                AF_INET6 != ifa->ifa_family)
            {
              break;
            }

            addr_info->iface_addr_s.addr.ss_family = ifa->ifa_family;
            addr_info->iface_addr_s.valid_addr = TRUE;
            addr_info->iface_mask = ifa->ifa_prefixlen;

            if (AF_INET == ifa->ifa_family)
            {
              sin = (struct sockaddr_in *)&addr_info->iface_addr_s.addr;
              memcpy(SASTORAGE_DATA(addr_info->iface_addr_s.addr),
                     RTA_DATA(rta),
                     sizeof(sin->sin_addr));
            }
            else if (AF_INET6 == ifa->ifa_family)
            {
              sin6 = (struct sockaddr_in6 *)&addr_info->iface_addr_s.addr;
              memcpy(SASTORAGE_DATA(addr_info->iface_addr_s.addr),
                     RTA_DATA(rta),
                     sizeof(sin6->sin6_addr));
            }

            ret = QDI_SUCCESS;
            goto bail;
          }
          break;

        default:
          QDI_LOG_LOW("rta_type=%#x\n", rta->rta_type);
          break;
      }

      rta = RTA_NEXT(rta, rtattrlen);
    }

    /* Advance to next header */
    nlh = NLMSG_NEXT(nlh, buflen);
  }
 
bail:
  if (QDI_NL_INVALID_FD != fd)
  {
    close(fd);
  }

  /* Free the allocated buffer */
  if (buf) {
      free(buf);
  }
  return ret;
}


/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  qdi_nl_get_ip_addr_info
===========================================================================*/
/*!
@brief
  This function returns the IP address of the given IP family for the given
  interface

@param
  iface     - Interface on which IP address is being requested
  ip_family - IP family of the address to return
  addr      - Return the corresponding address
  mask      - Return the corresponding mask

@return
  QDI_SUCCESS - If successful
  QDI_FAILURE - Otherwise

*/
/*=========================================================================*/

int qdi_nl_get_ip_addr_info
(
  const char              *iface,
  int                     ip_family,
  dsi_addr_info_t         *addr_info,
  qmi_wds_iface_name_type tech_name
)
{
  int ret = QDI_FAILURE;
  int sock_fd = QDI_NL_INVALID_FD;


  if (NULL == iface                                   ||
      (ip_family != AF_INET && ip_family != AF_INET6) ||
      NULL == addr_info)
  {
    QDI_LOG_ERROR("qdi_nl_get_addr_info: bad param(s)\n");
    goto bail;
  }

  /* Get a netlink route socket */
  sock_fd = qdi_nl_get_route_socket();

  if (QDI_NL_INVALID_FD == sock_fd)
  {
    QDI_LOG_ERROR("qdi_nl_get_addr_info: invalid route socket\n");
    goto bail;
  }

  /* Send a RTM_GETADDR request to the kernel */
  if (QDI_SUCCESS != qdi_nl_send_getaddr_req_msg(sock_fd, ip_family))
  {
    QDI_LOG_ERROR("qdi_nl_get_addr_info: RTM_GETADDR request send failed\n");
    goto bail;
  }

  /* Receive the RTM_GETADDR response and parse it */
  if (QDI_SUCCESS != qdi_nl_parse_getaddr_resp_msg(sock_fd, iface, ip_family, addr_info, tech_name))
  {
    QDI_LOG_ERROR("qdi_nl_get_addr_info: RTM_GETADDR parse response failed\n");
    goto bail;
  }

  ret = QDI_SUCCESS;

bail:
  if (QDI_NL_INVALID_FD != sock_fd)
  {
    close(sock_fd);
  }

  return ret;
}

