#ifndef _MID_SOCKET_H__
#define _MID_SOCKET_H__




int mid_socket_set_active(int sockfd, int timeout, int count, int intvl);
int mid_ip_is_multicast(char *ip);
int mid_socket_set_ttl(int socket, int value, int mult);
int mid_socket_set_tos(int socket , int value);
int mid_socket_set_sendbuf(int socket , int buflen);
int mid_socket_set_sendtimeout(int socket, int ms);




#endif
