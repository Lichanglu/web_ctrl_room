
#include "reach_epoll.h"
#include "reach_socket.h"
#include "reach_list.h"
#define	MAXLINE		512
#define	OPEN_MAX	100
#define	USER_PORT	4520
#define	SILVERLIGHT_PORT	943
#define	SERV_IP		"0.0.0.0"
#define	INFTIM		1000
#define	MAX_CHANNEL_ID	500

int32_t add_epoll_event(int32_t epfd, int32_t evfd, int32_t events)
{
	ev_t ev ;
	//设置与要处理的事件相关的文件描述符
	ev.data.fd = evfd;
	//设置要处理的事件类型
	ev.events = events;

	//注册epoll事件
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, evfd, &ev) < 0) {
		nslog(NS_INFO, "add_epoll_event ----- \n");
		nslog(NS_ERROR, "[add_epoll_event] epoll_ctl  : %s", strerror(errno));
		nslog(NS_INFO, "[add_epoll_event] evfd : [%d]\n", evfd);
		return -1;
	}

	return 0;
}

int32_t del_epoll_event(int32_t epfd, int32_t evfd, int32_t events)
{
	ev_t ev ;
	//设置与要处理的事件相关的文件描述符
	ev.data.fd = evfd;
	//设置要处理的事件类型
	ev.events = events;

	//注册epoll事件
	if(epoll_ctl(epfd, EPOLL_CTL_DEL, evfd, &ev) < 0) {
		nslog(NS_ERROR, "[del_epoll_event] epoll_ctl  : %s", strerror(errno));
		nslog(NS_INFO, "[del_epoll_event] evfd : [%d]\n", evfd);
		return -1;
	}

	return 0;
}

