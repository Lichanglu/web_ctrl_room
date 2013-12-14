#ifndef __NEW_TCP_COM__H
#define __NEW_TCP_COM__H
#define NEW_TCP_SOCK_PORT      0x20A4
void *open_new_tcp_task(void *arg);
void close_new_tcp_task(int index);
int get_fix_resolution(int index);
#endif //__TCPCOM__H
