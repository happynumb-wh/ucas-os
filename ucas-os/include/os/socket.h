#ifndef	_SYS_SOCKET_H
#define	_SYS_SOCKET_H

#include <type.h>
#include <os/ring_buffer.h>
#include <os/sched.h>
#include <fs/file.h>

#define PF_INET 2
#define AF_INET PF_INET

#define SOCK_STREAM    1
#define SOCK_DGRAM     2

#define SOL_SOCKET      1

#define IPPROTO_TCP      6
#define IPPROTO_UDP      17

#define SOCK_CLOEXEC   02000000
#define SOCK_NONBLOCK  04000
#define FD_CLOEXEC 1
#define O_NONBLOCK (SOCK_NONBLOCK)
#define SO_RCVTIMEO	20

#define MAX_SOCK_NUM 16
#define MAX_WAIT_LIST 8

#define SOCK_CLOSED 0
#define SOCK_LISTEN 1
#define SOCK_ACCEPTED 2
#define SOCK_ESTABLISHED 3

struct in_addr { in_addr_t s_addr; };
struct sockaddr {
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	uint8_t sin_zero[8];
};
// struct sockaddr {
// 	sa_family_t sa_family;
// 	char sa_data[14];
// };
struct socket {
	int domain;
	int type;
	int protocol;
	int backlog;
	uint8_t wait_list[MAX_WAIT_LIST];
	uint8_t used;
	uint8_t status;
	struct sockaddr *addr;
	struct ring_buffer data;
};

extern struct socket sock[MAX_SOCK_NUM + 1]; // 0: unused ; 1~MAX_SOCK_NUM: avaliable
extern mutex_lock_t sock_lock;

int init_socket();
int do_socket(int domain, int type, int protocol);
int do_bind(int sockfd,  struct sockaddr *addr, socklen_t addrlen);
int do_listen(int sockfd, int backlog);
int do_connect(int sockfd,  struct sockaddr *addr,socklen_t addrlen);
int do_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t do_sendto(int sockfd,  void *buf, size_t len, int flags, struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t do_recvfrom(int sockfd, void *buf, size_t len, int flags,struct sockaddr *src_addr, socklen_t *addrlen);
int32_t close_socket(uint32_t sock_num);

int do_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int do_setsockopt(int sockfd, int level, int optname,  void *optval, socklen_t optlen);
#endif