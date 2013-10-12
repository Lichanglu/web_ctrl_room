/*
 * =====================================================================================
 *
 *       Filename:  common.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年11月1日 09时12分18秒
 *       Revision:  
 *       Compiler:  gcc
 *
 *         Author:  黄海洪 
 *        Company:  深圳锐取信息技术股份有限公司
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <termios.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#include "common.h"

int set_send_timeout(int socket, unsigned int time)
{
	struct timeval timeout;
	int ret = 0;
	
	timeout.tv_sec = time;
	timeout.tv_usec = 0;
	
	ret = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO,& timeout, sizeof(timeout));
	if(ret == -1){
		printf("set_send_timeout failed!\n");
	}
	
	return ret;
}

int set_recv_timeout(int socket, unsigned int time)
{
	 struct timeval timeout;
	 int ret = 0;
	 
	 timeout.tv_sec = time;
	 timeout.tv_usec = 0;
	 
	 ret = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	 if(ret < 0){
		printf("set_recv_timeout failed!\n");
	 }
	 
	 return ret;
}

int tcp_recv_longdata(int sockfd, char* buffer, const int len)
{
	int totalRecv = 0;
	int nRecvLen = 0;
	
	while(totalRecv < len){
		nRecvLen = recv(sockfd, buffer + totalRecv, len - totalRecv, 0);
		if(nRecvLen < 1){
			printf( "tcp_recv_longdata failed, err msg:%s\n", strerror(errno));
			return -1;
		}
		
		totalRecv += nRecvLen;
		if(totalRecv == len)
			break;
	}
	
	return totalRecv;
}


int tcp_send_longdata(int sockfd, char* buffer, const int len)
{
	int totalSend = 0;
	int nSendLen = 0;
	
	while(totalSend < len){
		nSendLen = send(sockfd, buffer + totalSend, len - totalSend, 0);
		if(nSendLen < 1){
			printf(  "tcp_send_longdata failed, err msg:%s\n", strerror(errno));
			return -1;
		}
		
		totalSend += nSendLen;
		if(len == totalSend)
			break;
	}
	
	return totalSend;
}


