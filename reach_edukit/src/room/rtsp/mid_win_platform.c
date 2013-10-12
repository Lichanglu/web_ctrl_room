/*********************************************************************
*      封装标准的windows platform相关的一些函数
*
*
*
**********************************************************************/

//add by zhangmin
#ifdef GDM_RTSP_SERVER

#ifdef USE_WINDOWS_PLATFORM



#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
//#include <iostream>
#include <process.h>
#include <stdlib.h>
#include <math.h>
#include "mid_platform.h"
//using namespace std;


/*****************************多线程 fun****************************************/
int mid_plat_task_create(const char *name, int stack_size, int priority,
                         unsigned  func, void *arg, mid_plat_pthread_t *threadID)
{
	//	unsigned long threadHandle = 0;
	//	threadHandle =  _beginthreadex(NULL, 0, func, arg, 0, threadID);
	//	if(threadHandle == NULL)
	//	{
	//		printf("ERROR\n");
	//		return -1;
	//	}
	return 0;

}


void mid_task_init(void)
{
	PRINTF("mid_task_init \n");
	return ;
}



/*****************************socket fun****************************************/




//int mid_set_sockaddr_in(mid_plat_sockaddr_in *addrsrv,char *ip,short port)
//{
//	if(ip == NULL)
//	{
//		addrsrv->sin_addr.S_un.S_addr = htonl(INADDR_ANY);
//	}
//	else
//	{
//	//	addrsrv->sin_addr.S_un.S_addr = htonl(ip);
//	}
//	addrsrv->sin_family = AF_INET;
//	addrsrv->sin_port = htons(port);
//	return 0;
//}


//set socket send timeout
//windows time is int ,
int mid_plat_set_socket_stimeout(mid_plat_socket sSocket, unsigned long time2)
{

	int ret = 0;
	int timeout = 0;
	timeout = time2;

	ret = setsockopt(sSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));

	if(ret == -1) {
		PRINTF("setsockopt() Set Send Time Failed\n");
	}

	return ret;
	return 0;
}

//set socket recv timeout
int mid_plat_set_socket_rtimeout(mid_plat_socket sSocket, unsigned long time2)
{

	int ret = 0;

	int timeout = 0;
	timeout = time2;

	ret = setsockopt(sSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

	if(ret == -1) {
		PRINTF("setsockopt() Set Send Time Failed\n");
	}

	return ret;
	return 0;
}


//close socket
int mid_plat_close_socket(mid_plat_socket socket)
{
	closesocket(socket);
	return 0;
}

/****************************时间 fun****************************************/

//获取开机到现在的运行时间，毫秒级别
unsigned long mid_plat_get_runtime(void)
{
	return GetTickCount();
}


//sleep
void mid_plat_sleep(int ms)
{
	Sleep(ms);
}
#endif

#endif


