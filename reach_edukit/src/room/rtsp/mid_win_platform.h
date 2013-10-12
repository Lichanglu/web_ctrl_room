//add by zhangmin 
#ifdef GDM_RTSP_SERVER


#ifndef _MID_WIN_PLATFORM_H__
#define _MID_WIN_PLATFORM_H__



//转换部分接口，用于同时使用winsock,linx_socket
typedef SOCKET mid_plat_socket;
typedef SOCKADDR_IN mid_plat_sockaddr_in;
typedef unsigned int mid_plat_pthread_t;

typedef  int socklen_t;


//task
extern void mid_plat_task_init(void);
extern int mid_plat_task_create(const char* name, int stack_size, int priority, 
							void* func, void *arg,mid_plat_pthread_t *threadid);
							
							
//sock							
extern int mid_plat_set_sockaddr_in(mid_plat_sockaddr_in *addrsrv,char *ip, unsigned short port);

extern int mid_plat_set_socket_stimeout(mid_plat_socket sSocket,unsigned long time);

extern int mid_plat_close_socket(mid_plat_socket socket);
//time 
extern unsigned long mid_plat_get_runtime(void );	

int mid_plat_set_socket_rtimeout(mid_plat_socket sSocket,unsigned long time);
extern void mid_plat_sleep(int ms);
#endif

#endif

