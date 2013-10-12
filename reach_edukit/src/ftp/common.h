/*
 * =====================================================================================
 *
 *       Filename:  common.h
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


#ifndef _COMMON_H_
#define _COMMON_H_


#include <assert.h>

int set_send_timeout(int socket, unsigned int time);
int set_recv_timeout(int socket, unsigned int time);

int tcp_recv_longdata(int sockfd, char* buffer, const int len);
int tcp_send_longdata(int sockfd, char* buffer, const int len);





#endif
