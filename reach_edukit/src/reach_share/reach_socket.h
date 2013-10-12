#ifndef	__REACH_SOCKET_H__
#define	__REACH_SOCKET_H__

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include "nslog.h"
#include "stdint.h"
#define	LISTENQ				50
#define	SERV_IP		"0.0.0.0"
#define	IP_LEN	16
#define IF_NAMESIZE     4
typedef struct sockaddr_in sockaddr_in_t;
typedef struct sockaddr_un sockaddr_un_t;
typedef struct sockaddr	sa_t;
#define	ADDR_LEN	(sizeof(sa_t))
typedef  enum blocks {
    R_NONBLOCK,
    R_BLOCK
} block_t;

typedef enum socket_roles {
    R_CLICENT,
    R_SERVER
} socket_role_t;

int32_t set_nonblocking(int32_t sock);
void set_net_addr(sockaddr_in_t *addr, int8_t *ip, int32_t port);
void set_local_addr(sockaddr_un_t *local_addr, int8_t* unixstr_path);
int32_t get_local_ip(int8_t *eth, int8_t *ipaddr);
int32_t async_recv_data(int32_t fd, void *buf, int32_t buflen);
int32_t async_send_data(int32_t fd, void *buf, int32_t *buflen);
int32_t recv_long_tcp_data(int32_t fd, void *buf, int32_t buflen);
int32_t send_long_tcp_data(int32_t fd, void *buf, int32_t buflen);
int32_t create_local_tcp_socket(int8_t *unixstr_path, socket_role_t sr, block_t type);
void close_socket(int32_t fd);
#endif //__REACH_SOCKET_H__

