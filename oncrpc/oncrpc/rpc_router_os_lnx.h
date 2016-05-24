#ifndef RPC_IOCTL_H
#define RPC_IOCTL_H

#include <linux/ioctl.h>

struct rpcrouter_ioctl_server_args {
        uint32 prog;
        uint32 vers;
};

#define RPC_ROUTER_IOCTL_MAGIC (0xC1)

#define RPC_ROUTER_IOCTL_GET_VERSION \
        _IOR(RPC_ROUTER_IOCTL_MAGIC, 0, unsigned int)

#define RPC_ROUTER_IOCTL_GET_MTU \
        _IOR(RPC_ROUTER_IOCTL_MAGIC, 1, unsigned int)

#define RPC_ROUTER_IOCTL_REGISTER_SERVER \
        _IOWR(RPC_ROUTER_IOCTL_MAGIC, 2, unsigned int)

#define RPC_ROUTER_IOCTL_UNREGISTER_SERVER \
        _IOWR(RPC_ROUTER_IOCTL_MAGIC, 3, unsigned int)

#define RPC_ROUTER_IOCTL_CLEAR_NETRESET \
        _IOWR(RPC_ROUTER_IOCTL_MAGIC, 4, unsigned int)

#define RPC_ROUTER_IOCTL_GET_CURR_PKT_SIZE \
	_IOR(RPC_ROUTER_IOCTL_MAGIC, 5, unsigned int)

#endif /* RPC_IOCTL_H */
