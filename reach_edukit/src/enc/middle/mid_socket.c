#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <math.h>

#include "mid_socket.h"
//#include "log_common.h"


int mid_socket_set_sendtimeout(int socket, int ms)
{
	struct timeval tv;
	int ret = 0;
	tv.tv_sec = ms / 1000;
	tv.tv_usec = 1000 * (ms % 1000);
	ret = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));

	if(ret < 0) {
		printf("set socket send timeout is failed,	errno = %d, err msg = %s\n", errno, strerror(errno));
		return -1;
	}

	int tvLen = sizeof(struct timeval);
	ret = getsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, (socklen_t *)&tvLen);

	if(ret < 0) {
		printf("set socket send timeout is failed,	errno = %d, err msg = %s\n", errno, strerror(errno));
		return -1;
	}

	printf("get socket timeout is %ld ms\n", (tv.tv_sec * 1000 + tv.tv_usec / 1000));
	return 0;
}

int mid_socket_set_sendbuf(int socket , int buflen)
{
	int ret = 0;
	int buf_num = buflen;
	ret =	setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char *)&buf_num, sizeof(buf_num));

	if(ret < 0) {
		printf("set socket send buff num is failed,	errno = %d, err msg = %s\n", errno, strerror(errno));
		return -1;
	}

	int optlen;
	buf_num = 1;
	optlen = sizeof(buf_num);
	ret = getsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&buf_num, (socklen_t *)&optlen);

	if(ret < 0) {
		printf("get socket send buff num is failed,	errno = %d, err msg = %s\n", errno, strerror(errno));
		return -1;
	}

	printf("get socket send buff len =%d\n", buf_num);
	return 0;
}


int mid_socket_set_tos(int socket , int value)
{
	//printf("socket= %d,value=%d\n",socket,value);
	if(socket <= 0 || value < 0) {
		return -1;
	}

	char tos = value & 0xff;
	char tos2 = 0;
	int ret = 0;
	socklen_t len = sizeof(tos);

	ret = setsockopt(socket, IPPROTO_IP, IP_TOS, &tos, len);

	if(ret < 0) {
		printf("set socket IP_TOS is failed,	errno = %d, err msg = %s\n", errno, strerror(errno));
		return -1;
	}


	ret = getsockopt(socket, IPPROTO_IP, IP_TOS,  &tos2, (socklen_t *)&len);

	if(ret < 0) {
		printf("get socket IP_TOS is failed,	errno = %d, err msg = %s\n", errno, strerror(errno));
		return -1;
	}

	printf("socket=%d,tos =0x%x,tos2=0x%x\n", socket, tos, tos2);
	return 0;
}

int mid_socket_set_ttl(int socket, int value, int mult)
{

	//printf("socket= %d,value=%d\n",socket,value);

	if(socket <= 0 || value < 0) {
		return -1;
	}

	int option = 0;
	char ttl2 = 0;
	char ttl = value & 0xff;
	int ret = 0;
	socklen_t len = sizeof(ttl);

	if(mult == 1) {
		option = IP_MULTICAST_TTL;
	} else {
		option = IP_TTL;
	}

#if 1
	ret = getsockopt(socket, IPPROTO_IP, option, &ttl2, (socklen_t *)&len);

	if(ret < 0) {
		printf("get socket IP_TTL is failed,	errno = %d, err msg = %s\n", errno, strerror(errno));
		return -1;
	}

	printf("get mult =%d,option %d ,ttl2=%x\n", mult, option, ttl2);
#endif

	len = sizeof(ttl);
	ret = setsockopt(socket, IPPROTO_IP, option, &ttl, len);

	if(ret < 0) {
		printf("set socket IP_TTL is failed,	errno = %d, err msg = %s\n", errno, strerror(errno));
		return -1;
	}


	ret = getsockopt(socket, IPPROTO_IP, option, &ttl2, (socklen_t *)&len);

	if(ret < 0) {
		printf("get socket IP_TTL is failed,	errno = %d, err msg = %s\n", errno, strerror(errno));
		return -1;
	}

	printf("socket= %d,ttl =0x%x,ttl2=0x%x\n", socket, ttl, ttl2);
	return 0;
}

int mid_ip_is_multicast(char *ip)
{
	struct in_addr	 addr ;
	unsigned int 	dwAddr;
	unsigned int 	value;

	inet_aton(ip, &addr);
	memcpy(&value, &addr, 4);
	dwAddr = htonl(value);

	printf("ip=%s.dwAddr=0x%08x\n", ip, dwAddr);

	if(((dwAddr & 0xFF000000) >> 24) > 223) {
		return 1;
	}

	return 0;
}


static void set_keepalive_params(int sockfd, int timeout, int count, int intvl)
{
	int keepalive_time = timeout;
	int keepalive_probes = count;
	int keepalive_intvl = intvl;

	/*对一个连接进行有效性探测之前运行的最大非活跃时间间隔，默认值为 14400（即 2 个小时）*/
	if(setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, &keepalive_time, sizeof(int)) < 0) {
		printf("TCP_KEEPIDLE failed");
		return;
	}

	/*关闭一个非活跃连接之前进行探测的最大次数，默认为 8 次 */
	if(setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, &keepalive_probes, sizeof(int)) < 0) {
		printf("TCP_KEEPCNT failed");
		return;
	}

	/*两个探测的时间间隔，默认值为 150 即 75 秒,失败时候调用*/
	if(setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, &keepalive_intvl, sizeof(int)) < 0) {
		printf("TCP_KEEPINTVL failed");
		return;
	}

	return;

}


int mid_socket_set_active(int sockfd, int timeout, int count, int intvl)
{
	int optval;
	socklen_t optlen = sizeof(optval);

	/* check the status for the keepalive option */
	if(getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
		printf("getsockopt SO_KEEPALIVE failed");
		return -1;
	}

	printf("SO_KEEPALIVE is %s\n", optval ? "ON" : "OFF");

	/* set the option active */
	optval = 1;
	optlen = sizeof(optval);

	if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
		printf("setsockopt SO_KEEPALIVE failed，reason: %m\n");
		return -1;
	}

	printf("SO_KEEPALIVE on socket\n");

	/* check the status again */
	if(getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0) {
		printf("getsockopt SO_KEEPALIVE again failed");
		return -1;
	}

	set_keepalive_params(sockfd, timeout, count, intvl);
	printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));
	return 0;
}




