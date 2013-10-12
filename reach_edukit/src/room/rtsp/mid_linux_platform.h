#ifndef _MID_LINUX_PLATFORM_H__
#define _MID_LINUX_PLATFORM_H__



//转换部分接口，用于同时使用winsock,linx_socket
typedef int  mid_plat_socket;

typedef struct sockaddr_in mid_plat_sockaddr_in;

typedef pthread_t mid_plat_pthread_t;

//task
//void mid_plat_task_init(void);
//int mid_plat_task_create(const char* name, int stack_size, int priority, 
//							void* func, void *arg,mid_plat_pthread_t *threadid);
//							
							
//sock							
int mid_plat_set_sockaddr_in(mid_plat_sockaddr_in *addrsrv,char *ip, unsigned short port);
int mid_plat_set_socket_stimeout(mid_plat_socket sSocket,unsigned long time);
int mid_plat_set_socket_rtimeout(mid_plat_socket sSocket,unsigned long time);

int mid_plat_close_socket(mid_plat_socket socket);
//time 
unsigned long mid_plat_get_runtime(void );					

void mid_plat_sleep(int ms);
int mid_plat_compare_sockaddr_in(mid_plat_sockaddr_in *temp1,mid_plat_sockaddr_in *temp2);

#endif
