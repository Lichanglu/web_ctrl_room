/*
 * =====================================================================================
 *
 *       Filename:  common.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012��11��1�� 09ʱ12��18��
 *       Revision:  
 *       Compiler:  gcc
 *
 *         Author:  �ƺ��� 
 *        Company:  ������ȡ��Ϣ�����ɷ����޹�˾
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
