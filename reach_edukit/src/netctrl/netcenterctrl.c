
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include "xml_base.h"

#include "zlog.h"
#include "netcenterctrl.h"

#include "command_protocol.h"


#define CMD_LEN             (5)
#define IPLEN               (16)
#define BUFF_LEN            (8)

#define NET_SUCCESS         (0)
#define NET_FAIL            (-1)

#define NET_RUNNING         (1)
#define NET_STOP            (0)

#define UUID_FILE 	        ("/proc/sys/kernel/random/uuid")

#define NETCTROL_CFG_FILE   ("/usr/local/reach/netcontrol.cfg")
#define NETCTRL_IP          ("192.168.0.60")
#define NETCTRL_PORT        (30001)

#define CENCTRL_IP          ("127.0.0.1")
#define CENCTRL_PORT        (3000)

#define RECORD              (30010)
#define RECORDSTATE         (30024)

#define MAX_XMNL_LEN        (4*1024)

static zlog_category_t *NC_LOG;
static NetctrlHandle *pgNet_Handle;

/* 初始化双向链表 */
#define list_init(head) do			\
{						\
	(head)->next = (head)->prev = (head);	\
} while(0)

/* 在指定元素(where)之后插入新元素(item) */
#define list_add(item, towhere) do	\
{					\
	(item)->next = (towhere)->next;	\
	(item)->prev = (towhere);	\
	(towhere)->next = (item);	\
	(item)->next->prev = (item);	\
} while(0)

/* 在指定元素(where)之前插入新元素(item) */
#define list_add_before(item, towhere)  \
	list_add(item,(towhere)->prev)

/* 删除某个元素 */
#define list_remove(item) do			\
{						\
	(item)->prev->next = (item)->next;	\
	(item)->next->prev = (item)->prev;	\
} while(0)

/* 正向遍历链表中所有元素 */
#define list_for_each_item(item, head)\
	for ((item) = (head)->next; (item) != (head); (item) = (item)->next)

/* 反向遍历链表中所有元素 */
#define list_for_each_item_rev(item, head) \
	for ((item) = (head)->prev; (item) != (head); (item) = (item)->prev)

/* 根据本节点(item)获取节点所在结构体(type)的地址 */
/* 节点item地址(member的地址) - 该链表元素member在结构体中的偏移 */
#define list_entry(item, type, member) \
	((type *)((char *)item - (char *)(&((type *)0)->member)))

/* 判断链表是否为空 */
#define list_is_empty(head)	\
	((head)->next == (head))

/* 获取指定位置上一元素 */
#define list_prev_item(item)\
	((head)->prev)

static int32_t Package_MsgCode(uint8_t *send_buf, NET_MSGCODE msgcode);
static int32_t Analyze_NetCtrl_MsgCode(uint8_t *recv_buf, int32_t *msgcode);
static int32_t SendMsgToCenter(NetctrlHandle *pHandle, uint32_t len,void *xml);
static int32_t SendMsgToNetCtrl(NetctrlHandle *pHandle, uint32_t len,void *sendbuf);
static int32_t InitSocket(const int8_t* paddr,const int32_t port);
static int32_t InitNetCtrl();
static int32_t Analyze_NetCtrl_RecvMsg(NetctrlHandle *pHandle, uint8_t* msg);
static int32_t Analyze_Rec_Rep(uint8_t *recv_buf, void *pparm);
static int32_t Package_Rec_Xml(int8_t *send_buf, NET_MSGCODE msgcode);
static int32_t Package_Authreq_Xml(int8_t *send_buf, NET_MSGCODE msgcode);
static int32_t Analyze_CenCtrl_MsgCode(uint8_t *recv_buf, int32_t *msgcode);
static void Close_NetCtrlFd();
static void Close_CenCtrlFd();
static void SetOpeState(NET_MSGCODE msgcode);
static int32_t GetUUID(int8_t* uuid);

extern int32_t tcp_send_longdata(int32_t sockfd, int8_t* buffer, const int32_t len);
extern int32_t tcp_recv_longdata(int32_t sockfd, int8_t* buffer, const int32_t len);

static NetCtrlComnd gNetCtrlCmd[]=
{

	// NetCtrl  ->   CenterCtrl

	// 网络中控录制开始消息
	{
		REC_START,	//网络信令操作ID
		RECORD,		//xml信令ID
		"Netctrl_Start",	//操作名称
		Package_Rec_Xml,	//封装
		SendMsgToCenter,
		0
	},

	//网络中控录制暂停消息
	{
		REC_PAUSE,
		RECORD,
		"Netctrl_Pause",
		Package_Rec_Xml,
		SendMsgToCenter,
		0
	},
	//网络中控录制停止消息
	{
		REC_STOP,
		RECORD,
		"Netctrl_Stop",
		Package_Rec_Xml,
		SendMsgToCenter,
		0
	},
	//网络中控录制登陆成功消息
	{
		REC_LOGIN,
		0x08,
		"Netctrl_Login",
		Package_Authreq_Xml,
		SendMsgToCenter,
		0
	},
	//网络中控录制心跳返回消息
	{
		REC_HEARTBEAT,
		0x09,
		"Netctrl_Heartbeat",
		NULL,
		NULL,
		0
	},


    //CenterCtrl  -> NetCtrl
	//录制开始状态返回消息
	{
		REC_START,
		RECORD,
		"Rec_Start",
		Package_MsgCode,
		SendMsgToNetCtrl,
		1
	},
	//录制暂停状态返回消息
	{
		REC_PAUSE,
		RECORD,
		"Rec_Pause",
		Package_MsgCode,
		SendMsgToNetCtrl,
		1
	},
	// 录制停止状态返回消息
	{
		REC_STOP,	//xml信令ID
		RECORD,		//串口信令操作ID
		"Rec_Stop",	//操作名称
		Package_MsgCode,	//封装
		SendMsgToNetCtrl,
		1
	},
	//录制登陆状态消息
	{
		REC_LOGIN,
		0x08,
		"Rec_Login",
		Package_MsgCode,
		SendMsgToNetCtrl,
		1
	},
	//录制心跳状态返回消息
	{
		REC_HEARTBEAT,
		0x09,
		"Rec_Heartbeat",
		Package_MsgCode,
		SendMsgToNetCtrl,
		1
	},
};

#define MSGLEN           (sizeof(MsgHeader))
#define	NET_COMND_MAX    (sizeof(gNetCtrlCmd)/sizeof(NetCtrlComnd))

static int32_t TimeoutLockInit()
{
	return pthread_mutex_init(&pgNet_Handle->timeoutlock, NULL);
}

static int32_t TimeoutLockdeInit()
{
	return pthread_mutex_destroy(&pgNet_Handle->timeoutlock);
}

static int32_t NetSockLockInit()
{
	return pthread_mutex_init(&pgNet_Handle->ncsocklock, NULL);
}

static int32_t NetSockLockdeInit()
{
	return pthread_mutex_destroy(&pgNet_Handle->ncsocklock);
}

static int32_t OpeStateLockInit()
{
	return pthread_mutex_init(&pgNet_Handle->opestatelock, NULL);
}

static int32_t OpeStateLockdeInit()
{
	return pthread_mutex_destroy(&pgNet_Handle->opestatelock);
}

static void TimeoutLock()
{
	pthread_mutex_lock(&pgNet_Handle->timeoutlock);
}

static void TimeoutunLock()
{
	pthread_mutex_unlock(&pgNet_Handle->timeoutlock);
}

static void NetSockLock()
{
	pthread_mutex_lock(&pgNet_Handle->ncsocklock);
}

static void NetSockunLock()
{
	pthread_mutex_unlock(&pgNet_Handle->ncsocklock);
}

static int32_t CenSockLockInit()
{
	return pthread_mutex_init(&pgNet_Handle->ccsocklock, NULL);
}

static int32_t CenSockLockdeInit()
{
	return pthread_mutex_destroy(&pgNet_Handle->ccsocklock);
}

static void CenSockLock()
{
	pthread_mutex_lock(&pgNet_Handle->ccsocklock);
}

static void CenSockunLock()
{
	pthread_mutex_unlock(&pgNet_Handle->ccsocklock);
}

static void Close_NetCtrlFd()
{
    NetSockLock();
    if(pgNet_Handle->netctrlfd > 0)
    {
        r_close(pgNet_Handle->netctrlfd);
    }
    pgNet_Handle->netctrlfd = -1;
    NetSockunLock();
}

static void Close_CenCtrlFd()
{
    CenSockLock();
    if(pgNet_Handle->cenctrlfd > 0)
    {
        r_close(pgNet_Handle->cenctrlfd);
    }
    pgNet_Handle->cenctrlfd = -1;
    CenSockunLock();
}

static void SetOpeState(NET_MSGCODE msgcode)
{
    pthread_mutex_lock(&pgNet_Handle->opestatelock);
    pgNet_Handle->operstate = msgcode;
    pthread_mutex_unlock(&pgNet_Handle->opestatelock);
}

static int32_t upper_msg_set_time(struct timeval *time)
{
	if(time == NULL)
	{
		return -1;
	}

	struct  timeval  time_now ;
	time_now.tv_sec = 0;
	time_now.tv_usec = 0;
	gettimeofday(&time_now, 0);

	time->tv_sec = time_now.tv_sec;
	time->tv_usec = time_now.tv_usec;

	return 0;
}


static int32_t upper_msg_tim_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
	if(x->tv_sec > y->tv_sec) {
		return   -1;
	}

	if((x->tv_sec == y->tv_sec) && (x->tv_usec > y->tv_usec)) {
		return   -1;
	}

	result->tv_sec = (y->tv_sec - x->tv_sec);
	result->tv_usec = (y->tv_usec - x->tv_usec);

	if(result->tv_usec < 0) {
		result->tv_sec--;
		result->tv_usec += 1000000;
	}

	return   0;
}

static int32_t upper_msg_monitor_time_out_status(struct timeval *time, uint32_t time_out)
{
	struct  timeval  time_now ;
	time_now.tv_sec = 0;
	time_now.tv_usec = 0;
	struct  timeval  time_old;
	time_old.tv_sec = 0;
	time_old.tv_usec = 0 ;
	struct  timeval delta_time ;
	delta_time.tv_sec = 0;
	delta_time.tv_usec = 0;

	gettimeofday(&time_now, 0);
	time_old.tv_sec = time->tv_sec;
	time_old.tv_usec = time->tv_usec;

	if(-1 == upper_msg_tim_subtract(&delta_time, &time_old, &time_now)) {
			return -1;
	}

	if(delta_time.tv_sec > time_out) {
		//超时
		return 1;
	}

	//未超时
	return 0;
}

static int32_t AddTimeOut(NetctrlHandle *pHandle, NET_MSGCODE msgcode, uint8_t state)
{
	TimeOut *pattr = NULL;

	TimeoutLock();
	pattr = (TimeOut *)malloc(sizeof(TimeOut));
	pattr->msgcode = msgcode;
	//pattr->state = state;
	upper_msg_set_time(&pattr->starttime);
	list_add_before(&pattr->stlist, pHandle->pheadnode);
	TimeoutunLock();

	return 0;
}

static int32_t DelTimeOut(NetctrlHandle *pHandle, NET_MSGCODE msgcode)
{
	list_head *pcurnode = NULL;
	TimeOut *pattr = NULL;

	TimeoutLock();
	/* 查找是否存在该设备 */
	list_for_each_item(pcurnode, pHandle->pheadnode)
    {
        if(NULL != pcurnode)
        {
            pattr = list_entry(pcurnode, TimeOut, stlist);

		 	if(pattr->msgcode == msgcode)
		 	{
				list_remove(pcurnode);
				r_free(pattr);
				pattr = NULL;

				TimeoutunLock();
				//此消息等待超时
				return 1;
			}
        }
    }
	TimeoutunLock();

	return 0;
}

static int32_t CheckTimeOut(NetctrlHandle *pHandle, NET_MSGCODE msgcode)
{
	list_head *pcurnode = NULL;
	TimeOut *pattr = NULL;

	TimeoutLock();
	/* 查找是否存在该设备 */
	list_for_each_item(pcurnode, pHandle->pheadnode)
    {
        if(NULL != pcurnode)
        {
            pattr = list_entry(pcurnode, TimeOut, stlist);

		 	if(pattr->msgcode == msgcode)
		 	{
				TimeoutunLock();
				//此消息等待超时
				return 1;
			}
        }
    }
	TimeoutunLock();
	return 0;
}

static NetCtrlComnd* GetNetOperate(NET_MSGCODE msgcode, int32_t direc)
{
    //printf(" GetNetOperate, msgcode: %x \n", msgcode);
    uint32_t i = 0;
	for(i = 0; i < NET_COMND_MAX ;i++)
	{
		if((gNetCtrlCmd[i].netcmd == msgcode) && (gNetCtrlCmd[i].direction == direc))
		{
			break ;
		}
	}

	if(i >= NET_COMND_MAX)
	{
		return NULL;
	}

	return &gNetCtrlCmd[i];
}


//初始化Socket
int32_t InitSocket(const int8_t * paddr, const int32_t port)
{
    int32_t ret = 0;

	int net_socket = r_socket(AF_INET, SOCK_STREAM, 0);
	if(net_socket < 0)
	{
		zlog_error(NC_LOG,"ConnectSerialServer failed, err msg: %s ", strerror(errno));
		return NET_FAIL;
	}

	struct sockaddr_in		serv_addr;
	r_bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family	= AF_INET;
	serv_addr.sin_port		= htons(port);
	r_inet_aton(paddr, (struct in_addr *)&serv_addr.sin_addr);

	ret = r_connect(net_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if(ret < 0)
	{
		//zlog_error(NC_LOG, "connect failed, err msg: %s ", strerror(errno));
		r_close(net_socket);
		return NET_FAIL;
	}

	int opt = 1;
	if(r_setsockopt(net_socket,SOL_SOCKET,SO_REUSEADDR,(uint8_t *)&opt,sizeof(opt)) < 0)
	{
	    zlog_error(NC_LOG, "r_setsockopt failed, err msg: %s", strerror(errno));
	    r_close(net_socket);
	    return NET_FAIL;
	}

	//set_send_timeout(net_socket, 30);
	//set_recv_timeout(net_socket, 30);

	return net_socket;
}

static int32_t InitNetCtrl()
{
    NC_LOG = zlog_get_category("NetControlLog");

    pgNet_Handle = (NetctrlHandle *)r_malloc(sizeof(NetctrlHandle));
	if(NULL == pgNet_Handle)
	{
		zlog_error(NC_LOG,"InitNetCtrl failed, malloc NetCtrl handle error! ");
		return NET_FAIL;
	}
	r_memset(pgNet_Handle, 0x0, sizeof(NetctrlHandle));

	list_head *headnode = NULL;
	headnode = (list_head *)r_malloc(sizeof(list_head));
	if(NULL == headnode)
	{
		printf("InitNetCtrl failed, malloc list_head error, err msg = %s\n",strerror(errno));
		r_free(pgNet_Handle);
		pgNet_Handle = NULL;
		return NET_FAIL;
	}

	pgNet_Handle->pheadnode = headnode;
	list_init(pgNet_Handle->pheadnode);

	pgNet_Handle->runstate = NET_STOP;
	pgNet_Handle->operstate = REC_STOP;
	pgNet_Handle->netctrlfd = -1;
	pgNet_Handle->cenctrlfd = -1;
    //pgNet_Handle->hbstate   = 0;

	CenSockLockInit();
    NetSockLockInit();
    TimeoutLockInit();
    OpeStateLockInit();

	return NET_SUCCESS;
}

static int32_t Analyze_CenState(int8_t *recv_buf, void *pparm)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	int32_t i  = 0;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr RecServerStatusUpdateReq = NULL;
	xmlNodePtr RoomStatus = NULL;
	xmlNodePtr RecStatus = NULL;

	uint8_t *pRecStatus = NULL;
	uint8_t *pStatus[3] = {0};

	NET_MSGCODE *parm = (NET_MSGCODE *)(pparm);
	if(parm == NULL)
	{
		zlog_error(NC_LOG, "Analyze_State: pHandle is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(NC_LOG, "Analyze_State: malloc parse_xml_t fail");
		return -1;
	}

	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(NC_LOG, "Analyze_State: init_dom_tree fail");
		goto EXIT;
	}

	if(is_req_msg(parse_xml_user->proot) != 1)
	{
		zlog_error(NC_LOG, "Analyze_State: is_req_msg fail");
		goto EXIT;
	}

	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	if(msgbody == NULL)
	{
		zlog_error(NC_LOG, "Analyze_State: msghead fail");
		goto EXIT;
	}

	RecServerStatusUpdateReq   = get_children_node(msgbody, BAD_CAST "RecServerStatusUpdateReq");
	if(RecServerStatusUpdateReq == NULL)
	{
		zlog_error(NC_LOG, "Analyze_State: not found usrname");
		goto EXIT;
	}

	RoomStatus   = get_children_node(RecServerStatusUpdateReq, BAD_CAST "RoomStatus");
	if(RoomStatus == NULL)
	{
		zlog_error(NC_LOG, "Analyze_State: not found usrname");
		goto EXIT;
	}

	RecStatus   = get_children_node(RoomStatus, BAD_CAST "RecStatus");
	if(RecStatus == NULL)
	{
		zlog_error(NC_LOG, "Analyze_State: not found usrname");
		goto EXIT;
	}

	pRecStatus  = (uint8_t *)xmlNodeListGetString(parse_xml_user->pdoc, RecStatus->xmlChildrenNode, 1);
	if(pRecStatus == NULL)
	{
		zlog_error(NC_LOG, "Analyze_State: not found usrname");
		goto EXIT;
	}

	uint8_t recordstate;

	recordstate = (uint8_t)atoi((const char *)pRecStatus);
	if(recordstate == 0) //停止
	{
		*parm = REC_STOP;
	}
	//开始
	else if(recordstate == 1)
	{
		*parm = REC_START;
	}
	// 暂停
	else if(recordstate == 2)
	{
		*parm = REC_PAUSE;
	}
	else
	{
		zlog_error(NC_LOG, "Analyze_State: error state [%d]",recordstate);
		goto EXIT;
	}

#if 0
	parm->videostate[0] = 0x1;
	parm->videostate[1] = 0x1;
	parm->videostate[2] = 0x1;
	parm->warning 		= 0x2;
	Status[0]   = get_children_node(RoomStatus, BAD_CAST "Status1");
	pStatus[0]  = (UInt8*)xmlNodeListGetString(parse_xml_user->pdoc, Status[0]->xmlChildrenNode, 1);
	if(pStatus[0] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status1");
		goto EXIT;
	}

	Status[1]   = get_children_node(RoomStatus, BAD_CAST "Status2");
	pStatus[1]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[1]->xmlChildrenNode, 1);
	if(pStatus[1] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status2");
		goto EXIT;
	}

	Status[2]   = get_children_node(RoomStatus, BAD_CAST "Status3");
	pStatus[2]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[2]->xmlChildrenNode, 1);
	if(pStatus[2] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status3");
		goto EXIT;
	}
#endif

	return_code = 1;

EXIT:
	if(NULL != pRecStatus)
	{
		xmlFree(pRecStatus);
	}

	for(i = 0; i <3 ;i++)
	{
		if(NULL != pStatus[i])
		{
			xmlFree(pStatus[i]);
		}
	}

	if(parse_xml_user->pdoc != NULL)
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}

static int32_t Analyze_Rec_Rep(uint8_t *recv_buf, void *pparm)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	xmlNodePtr msghead = NULL;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr retstate = NULL;
	char *pret = NULL;
    //printf(" Analyze_Rec_Rep  \n");
	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(NC_LOG, "Analyze_RecordRep: malloc parse_xml_t fail");
		return NET_FAIL;
	}

	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(NC_LOG, "Analyze_RecordRep: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1)
	{
		zlog_error(NC_LOG, "Analyze_RecordRep: is_resp_msg fail");
		goto EXIT;
	}

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		zlog_error(NC_LOG, "Analyze_RecordRep: msghead fail");
		goto EXIT;
	}

	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	if(msgbody == NULL)
	{
		zlog_error(NC_LOG, "Analyze_RecordRep: msgbody fail");
		goto EXIT;
	}

	retstate   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retstate == NULL)
	{
		zlog_error(NC_LOG, "Analyze_RecordRep: not found ReturnCode");
		goto EXIT;
	}

	pret  = (int8_t* )xmlNodeListGetString(parse_xml_user->pdoc, retstate->xmlChildrenNode, 1);
	if(pret == NULL)
	{
		zlog_error(NC_LOG, "Analyze_RecordRep: not found usrname");
		goto EXIT;
	}

	int32_t iret = -1;
	iret = atoi(pret);
	//iret = 1; //强制将结果设置成功，用于调试
	if(iret != 1)
	{
		//RtRecordState = 0;
		zlog_error(NC_LOG, "Analyze_RecordRep: error record ret[%d]",iret);
		goto EXIT;
	}


	//printf("close[%d] start[%d] stop[%d]\n",parm->close, parm->start, parm->stop);
	return_code = NET_SUCCESS;

EXIT:
	if(NULL != pret)
	{
		xmlFree(pret);
	}

	if(parse_xml_user->pdoc != NULL)
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}

/* 解析Center Control消息类型 */
static int32_t Analyze_CenCtrl_MsgCode(uint8_t *recv_buf, int32_t *msgcode)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	xmlNodePtr msghead = NULL;
	xmlNodePtr MsgCode = NULL;

	//printf(" Analyze_CenCtrl_MsgCode \n");
	uint8_t *pMsgCode = NULL;

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(NC_LOG, "Analyze_MsgCode: malloc parse_xml_t fail");
		return -1;
	}

	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(NC_LOG, "Analyze_MsgCode: init_dom_tree fail");
		goto EXIT;
	}


	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		zlog_error(NC_LOG, "Analyze_MsgCode: msghead fail");
		goto EXIT;
	}

	MsgCode   = get_children_node(msghead, BAD_CAST "MsgCode");
	if(MsgCode == NULL)
	{
		zlog_error(NC_LOG, "Analyze_MsgCode: not found usrname");
		goto EXIT;
	}

	pMsgCode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, MsgCode->xmlChildrenNode, 1);
	if(pMsgCode == NULL)
	{
		zlog_error(NC_LOG, "Analyze_MsgCode: not found usrname");
		goto EXIT;
	}
	*msgcode = atoi(pMsgCode);
	return_code = 1;

EXIT:
	if(NULL != pMsgCode)
	{
		xmlFree(pMsgCode);
	}

	if(parse_xml_user->pdoc != NULL)
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);

	return return_code;
}

//解析接收到的录播消息
static int32_t Analyze_NetCtrl_MsgCode(uint8_t *recv_buf, int32_t *msgcode)
{
    //printf(" Analyze_NetCtrl_MsgCode \n");
    if(NULL == recv_buf || NULL == msgcode)
    {
        zlog_error(NC_LOG,"Invalid input Parameter ");
        return NET_FAIL;
    }

    uint8_t *pRecvCmd = recv_buf;
    if(0xFF != pRecvCmd[0] || 0x07 != pRecvCmd[1] || 0x07 != pRecvCmd[2]
        || 0x55 != pRecvCmd[4])
    {
        zlog_error(NC_LOG,"Invalid recv cmd! recv: %x %x %x %x %x  ", recv_buf[0],recv_buf[1],recv_buf[2],recv_buf[3],recv_buf[4]);
        return NET_FAIL;
    }
    switch(pRecvCmd[3])
    {
        case REC_LOGIN:
        case REC_HEARTBEAT:
        case REC_PAUSE:
        case REC_START:
        case REC_STOP:
            *msgcode = pRecvCmd[3];
            break;
        default:
            *msgcode = -1;
            break;
    }

    if(*msgcode <= 0)
    {
        zlog_error(NC_LOG,"Invalid recv cmd! msgcode: %d ", *msgcode);
        return NET_FAIL;
    }

    //printf("Analyze Netctrl Msg, msgcode %x \n", *msgcode);

    return NET_SUCCESS;
}


//封装发送的消息
static int32_t Package_MsgCode(uint8_t *send_buf, NET_MSGCODE msgcode)
{
    //printf(" Package_MsgCode \n");
    uint8_t sSendCmd[BUFF_LEN] = {0xFF, 0x01, 0x06, 0x00, 0x55};

    if(NULL == send_buf)
    {
        zlog_error(NC_LOG,"Invalid input Parameter ");
        return NET_FAIL;
    }

    switch(msgcode)
    {
        case REC_LOGIN:
            sSendCmd[3] = 0x08;
            break;
        case REC_HEARTBEAT:
            sSendCmd[3] = 0x09;
            break;
        case REC_PAUSE:
            sSendCmd[3] = 0x06;
            break;
        case REC_START:
            sSendCmd[3] = 0x05;
            break;
        case REC_STOP:
            sSendCmd[3] = 0x07;
            break;

        default:
            break;
    }

    if (0x00 == sSendCmd[3])
    {
        zlog_error(NC_LOG,"Invalid pakeage cmd! msgcode: %c ", sSendCmd[3]);
        return NET_FAIL;
    }

    r_strcpy((int8_t*)send_buf, (const int8_t*)sSendCmd);

    return NET_SUCCESS;

}

static int32_t Analyze_CenCtrl_RecvMsg(NetctrlHandle *pHandle, uint8_t* msg)
{
    uint8_t* pmsg = msg;
    int8_t sendbuf[CMD_LEN] = {0};
    int32_t nRet = -1;
    //printf(" Analyze_CenCtrl_RecvMsg \n");
    if(NULL == pHandle || NULL == pmsg)
    {
        zlog_error(NC_LOG, "Invailed paraments, pHandle or pmsg is NULL ");
        return NET_FAIL;
    }

    int32_t CenCode = -1;
    if(-1 == Analyze_CenCtrl_MsgCode(pmsg,&CenCode))
    {
        zlog_error(NC_LOG, "Analyze_CenCtrl_MsgCode failed!");
    	return NET_FAIL;
    }

    if(CenCode != 30019 && CenCode != 30024)
        zlog_debug(NC_LOG, "Cencode:%d ",CenCode);

    if(CenCode == 30024) //Center Control 心跳
    {
        NET_MSGCODE msgcode = REC_STOP;

        if (1 != Analyze_CenState(pmsg, &msgcode))
        {
            zlog_error(NC_LOG, " Analyze_CenState HeartBeat Failed!");
            return NET_FAIL;
        }

        //if(msgcode == pHandle->operstate)
        //{
        //    zlog_debug(NC_LOG, "Cenctrl HeartBeat Resp state is the same as NetCtrl, returned directly!");
        //}
        SetOpeState(msgcode);
    }
    else if(NET_FAIL == Analyze_Rec_Rep(pmsg, pHandle))
    {
        zlog_error(NC_LOG, "Analyze_Rec_Rep failed!");
        if(30001 == CenCode)
        {
            pHandle->runstate = NET_STOP;
        }
        return NET_FAIL;
    }

    if(30001 == CenCode)
    {
        zlog_debug(NC_LOG, "Auth Netctrl to Centrol Success, returned directly!");
        return NET_SUCCESS;
    }


    //zlog_debug(NC_LOG, "Recv Cenctrl ******operstate = %x ", pHandle->operstate);
    NetCtrlComnd* pCmdHandle = GetNetOperate(pHandle->operstate, 1);
    if (NULL == pCmdHandle)
    {
        zlog_error(NC_LOG, "GetNetOperate NULL ");
        return NET_FAIL;
    }

    if(NULL != pCmdHandle->Package_Msg)
    {
        nRet = pCmdHandle->Package_Msg(sendbuf, pHandle->operstate);
        if (NET_FAIL == nRet)
        {
            zlog_error(NC_LOG, "PackageMsg operstate: %d is fail!!", pHandle->operstate);
            return NET_FAIL;
        }
    }

    if(NULL != pCmdHandle->Handle_Msg)
    {
        if(NET_FAIL == pCmdHandle->Handle_Msg(pHandle, CMD_LEN, sendbuf))
        {
            zlog_error(NC_LOG, "Handle_Msg operstate: %d is fail!!", pHandle->operstate);
            return NET_FAIL;
        }
    }

    AddTimeOut(pHandle, pHandle->operstate, 0);

    zlog_debug(NC_LOG, "Send to Netctrl success!msgcode[%d],fd[%d]", pHandle->operstate, pHandle->netctrlfd);
    return NET_SUCCESS;

}

static int32_t Analyze_NetCtrl_RecvMsg(NetctrlHandle *pHandle, uint8_t* msg)
{
    //printf(" Analyze_NetCtrl_RecvMsg\n");
    int msgcode = -1;

    uint8_t* sendxml = NULL;

    if(NULL == pHandle)
    {
        zlog_error(NC_LOG, "Invailed paraments, pHandle is NULL \n");
        return NET_FAIL;
    }

    if(NET_FAIL == Analyze_NetCtrl_MsgCode(msg, &msgcode))
    {
        zlog_error(NC_LOG, "Recv msg error! Continue....");
        return NET_FAIL;
    }

    NetCtrlComnd* pCmdHandle = GetNetOperate(msgcode, 0);
    if(NULL == pCmdHandle)
    {
        zlog_error(NC_LOG, "GetNetOperate NULL ");
        return NET_FAIL;
    }
    //*********************
    //心跳，登陆消息需要另外处理
    //*********************
    if(NET_STOP == pHandle->runstate)
    {
        if(REC_LOGIN == msgcode)
        {
            pHandle->runstate = NET_RUNNING;
            zlog_debug(NC_LOG, "Login! Continue....");
            //return NET_SUCCESS;
        }
        else
        {
            zlog_error(NC_LOG, "Not Login! Continue....");
            return NET_FAIL;
        }
    }

    if(REC_HEARTBEAT == msgcode)
    {
        AddTimeOut(pHandle, REC_HEARTBEAT, 0);
        zlog_debug(NC_LOG, "REC_HEARTBEAT! Continue....");
        return NET_SUCCESS;
    }

    if (1 == CheckTimeOut(pHandle, msgcode))
    {
        //返回CenCtrl的结果后，NetCtrl会再返回一个信令，直接返回
        DelTimeOut(pHandle, msgcode);
        zlog_debug(NC_LOG, "Recv Net Ctrl resp!");
        return NET_SUCCESS;
    }

    if (msgcode == pHandle->operstate)
    {
        //如果连接的两个信令相同，直接不返回，不作操作
        zlog_debug(NC_LOG, "The same operation, returned directly!");
        return NET_SUCCESS;
    }

    //AddTimeOut(pHandle, msgcode, 0);

    sendxml = (uint8_t*)r_malloc(MAX_XMNL_LEN);
    if(NULL == sendxml)
    {
        zlog_error(NC_LOG, "r_malloc failed!");
        return NET_FAIL;
    }

    r_memset(sendxml, 0, MAX_XMNL_LEN);
    int nLen = 0;

    if(NULL != pCmdHandle->Package_Msg)
    {
        nLen = pCmdHandle->Package_Msg(sendxml+MSGLEN, msgcode);
        if (-1 == nLen)
        {
            zlog_error(NC_LOG, "PackageMsg cmd[%d]  is fail!!", msgcode);
            return NET_FAIL;
        }
        //printf("send xml size %d \n", nLen);
    }

    if(NULL != pCmdHandle->Handle_Msg)
    {
        if(NET_FAIL == pCmdHandle->Handle_Msg(pHandle, nLen, sendxml))
        {
            zlog_error(NC_LOG, "Handle_Msg cmd[%d]  is fail!!", msgcode);
            return NET_FAIL;
        }
    }

    if(REC_LOGIN != msgcode)
    {
        SetOpeState(msgcode);
    }
    zlog_debug(NC_LOG, "Send to Cenctrl success!msgcode: %d, fd[%d]",pHandle->operstate, pHandle->cenctrlfd);

    if(NULL != sendxml)
    {
        r_free(sendxml);
        sendxml = NULL;
    }

    return NET_SUCCESS;
}

static int32_t GetUUID(int8_t* uuid)
{
    if(NULL == uuid)
    {
        return NET_FAIL;
    }

    FILE *fd = r_fopen((const int8_t *)UUID_FILE, (const int8_t *)"r");
	if (NULL == fd)
	{
			zlog_error(NC_LOG, "open failed: %s", strerror(errno));
			return NET_FAIL;
	}
	int8_t buf[128] = {0};
	int nRet = fread(buf, 128, 1, fd);
	if (nRet < 0)
	{
			zlog_error(NC_LOG, "Read Faild: %s\n", strerror(errno));
			return NET_FAIL;
	}

	char buf1[16] = {0};
    char buf2[16] = {0};
    char buf3[16] = {0};
	char buf4[16] = {0};
	char buf5[16] = {0};
	sscanf((const int8_t*)buf, "%[0-9a-z]-%[0-9a-z]-%[0-9a-z]-%[0-9a-z]-%[0-9a-z]", buf1, buf2, buf3,buf4,buf5);
	r_memset(buf, 0, 128);
	sprintf((int8_t*)buf, "%s%s%s%s%s", buf1, buf2, buf3, buf4, buf5);

    r_strncpy(uuid, (int8_t*)buf, 128);

	return NET_SUCCESS;
}

//封装 Control Auth req XML 信息
static int32_t Package_Authreq_Xml(int8_t *send_buf, NET_MSGCODE msgcode)
{
	if(send_buf == NULL || msgcode != REC_LOGIN)
	{
		return -1;
	}

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr ctrl_node 			= NULL;


	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30001");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"NetControl");

	ctrl_node = xmlNewNode(NULL, BAD_CAST "AuthReq");
	xmlAddChild(body_node, ctrl_node);

	package_add_xml_leaf(ctrl_node, (const xmlChar *)"User",  "admin");
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"Password", "admin");

	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	//printf("XML\n%s \n size %d \n", send_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

//封装Recode XML 信息
static int32_t Package_Rec_Xml(int8_t *send_buf, NET_MSGCODE msgcode)
{
    //printf(" Package_Rec_Xml, msgcode: %x  \n", msgcode);
	int8_t buffer[1024] = {0};
	int8_t state[3] = {0};
	int8_t roomid[10] = {0};
	if(send_buf == NULL)
	{
		return -1;
	}


	//开始录制
	if(msgcode == REC_START)
	{
		r_strcpy((int8_t *)state, (const int8_t *)"1");
	}

	//暂停录制
	else if(msgcode == REC_PAUSE)
	{
		r_strcpy((int8_t *)state, (const int8_t *)"2");
	}
	//关闭录制
	else if(msgcode == REC_STOP)
	{
		r_strcpy((int8_t *)state, (const int8_t *)"0");
	}
	else
	{
		return -1;
	}


	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr ctrl_node 			= NULL;


	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30010");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"NetControl");

	ctrl_node = xmlNewNode(NULL, BAD_CAST "RecCtrlReq");
	xmlAddChild(body_node, ctrl_node);

	sprintf((char *)roomid,"%d",0);
	srand(time(0));
	if(NET_FAIL == GetUUID(buffer))
	{
	    return -1;
	}
	//printf("RecordID %s \n", buffer);
	//sprintf((char *)buffer,"%d",(int)(rand()+ 0));
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"RoomID",  (const int8_t *)roomid);
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"RecordID", (const int8_t *)buffer);
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"OptType", (const int8_t *)state);
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"RoomName", (const int8_t *)"");
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"AliasName", (const int8_t *)"");
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"TeacherName", (const int8_t *)"");
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"CourseName", (const int8_t *)"");
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"Notes", (const int8_t *)"");

	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	//printf("XML\n%s \n", send_buf);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t SendMsgToCenter(NetctrlHandle *pHandle, uint32_t len,void *xml)
{
    //printf(" SendMsgToCenter  \n");

    uint8_t *pxml = (uint8_t *)xml;

	if(pxml == NULL|| len <= 0)
	{
		return NET_FAIL;
	}

	if(pHandle->cenctrlfd > 0)
	{
		MsgHeader *pmsg = (MsgHeader *)pxml;
		int32_t slen = 0;
		slen = len + MSGLEN;
		pmsg->sLen = r_htons(slen);
		pmsg->sMsgType = 0;
		pmsg->sVer = r_htons(2012);

		CenSockLock();
		int32_t nameLength = tcp_send_longdata(pHandle->cenctrlfd, (int8_t *)pxml, slen);
        CenSockunLock();

	    if (nameLength < 0)
	    {
	       	zlog_error(NC_LOG, "tcp_send_longdata: nameLength is error!!");
	       	return NET_FAIL;

	    }
	    //printf("Send XML to Control success! fd :%d \n", pHandle->cenctrlfd);
	}
	else
	{
	    zlog_error(NC_LOG, "pHandle->cenctrlfd is erorr!" );
	    return NET_FAIL;
	}

	return NET_SUCCESS;

}

static int32_t SendMsgToNetCtrl(NetctrlHandle *pHandle, uint32_t len,void *sendbuf)
{
    //printf(" SendMsgToNetCtrl \n");
    uint8_t *sbuf = (uint8_t *)sendbuf;
    int nRet = -1;

	if(NULL == pHandle || sbuf == NULL|| len <= 0)
	{
	    zlog_error(NC_LOG, "input paraments invalied!");
		return NET_FAIL;
	}


	NetSockLock();
	if(pHandle->netctrlfd > 0)
	{
	    nRet = r_send(pHandle->netctrlfd, sbuf, len, 0);

		if (nRet < 0)
		{
		    NetSockunLock();
			zlog_error(NC_LOG, "r_send cmd is error! err = %s",strerror(errno));
			return NET_FAIL;
		}
	}
	else
	{
	    NetSockunLock();
	    zlog_error(NC_LOG, " pHandle->netctrlfd error! " );
	    return NET_FAIL;
	}
    NetSockunLock();
	return NET_SUCCESS;

}

static int32_t ReadCfgFile(int8_t *NetIP)
{
    FILE *CfgFile = NULL;
    int8_t buf[IPLEN] = {0};

    int32_t nRet = -1;

    if(access(NETCTROL_CFG_FILE, F_OK) != 0)
    {
        CfgFile = r_fopen((const int8_t *)NETCTROL_CFG_FILE, (const int8_t *)"w");
        if(NULL == CfgFile)
        {
            zlog_error(NC_LOG, "Create Netcontrol.cfg Error!");
            return NET_FAIL;
        }

        nRet = fwrite(NETCTRL_IP, 1, IPLEN, CfgFile);
        if(nRet <= 0)
        {
            fclose(CfgFile);
            zlog_error(NC_LOG, "fwrite Netcontrol.cfg Error!");
          	return NET_FAIL;
        }

        r_strncpy(NetIP, (int8_t *)NETCTRL_IP,r_strlen((const int8_t *)NETCTRL_IP));
    }
    else
    {
        CfgFile = r_fopen((const int8_t *)NETCTROL_CFG_FILE, (const int8_t *)"r");
        if(NULL == CfgFile)
        {
            zlog_error(NC_LOG, "Open Netcontrol.cfg Error!");
            return NET_FAIL;
        }


        nRet = fread(buf, 1, IPLEN , CfgFile);
        if(nRet <= 0)
        {
            fclose(CfgFile);
            zlog_error(NC_LOG, "Read Netcontrol.cfg Error!");
          	return NET_FAIL;
        }

        r_strncpy(NetIP, (int8_t *)buf, r_strlen(buf));
    }

    fclose(CfgFile);

    return NET_SUCCESS;
}

void *NetCtrlTask(void *args)
{
    int32_t netfd = -1;
    int32_t nRet = -1;
    int8_t NetIP[IPLEN] = {0};

    fd_set readfd;
    struct timeval timeout;

    zlog_debug(NC_LOG, "*********NetCtrlTask begin*****");
    if(NULL == args)
    {
        zlog_error(NC_LOG, "NetCtrlTask failed, args is NULL! ");
        return NULL;
    }
    NetctrlHandle *pHandle = (NetctrlHandle*)args;

    if(NET_FAIL == ReadCfgFile(NetIP))
    {
        zlog_error(NC_LOG, "Get Cfg File failed! ");
        return NULL;
    }

    while(1)
    {
        netfd = InitSocket(NetIP, NETCTRL_PORT);
        if(netfd < 0)
        {
            //zlog_error(NC_LOG,"InitNetCtrl failed, init socket error! ");
            r_sleep(5);
            continue;
        }

        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        nRet = r_setsockopt(netfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        if(nRet < 0)
        {
            printf("set_recv_timeout failed!\n");
        }

        pHandle->netctrlfd = netfd;
        pHandle->runstate = NET_STOP;
        SetOpeState(REC_STOP);

        zlog_debug(NC_LOG, "NetCtrlTask Initsocket [%d] ", netfd);

        while (1)
        {
            timeout.tv_sec = 30;
            timeout.tv_usec = 0;

            FD_ZERO(&readfd);
            FD_SET(netfd, &readfd);

            nRet = r_select(netfd + 1, &readfd, NULL, NULL, &timeout);
            if(nRet > 0) // FD已准备好
            {
                if(FD_ISSET(netfd, &readfd))
                {
                    int recvlen = 0;
                    uint8_t msg[BUFF_LEN] = {0};

                    //printf("******r_recv data******\n");

                    NetSockLock();
                    recvlen = r_recv(netfd, (uint8_t *)msg, CMD_LEN, 0);
                    NetSockunLock();

                    if(recvlen < CMD_LEN|| recvlen == -1)
                    {
                        zlog_error(NC_LOG, "errmsg = %s  recvlen = %d", strerror(errno), recvlen);

                        if(errno !=  EAGAIN)
                        {
                            fprintf(stderr, "errno !=  EAGAIN, errmsg = %s\n", strerror(errno));
                            //r_usleep(10000);
                        }

                        Close_NetCtrlFd(netfd);
                        r_usleep(500*1000);
                        break;
                    }

                    if(NET_FAIL == Analyze_NetCtrl_RecvMsg(pHandle, msg))
                    {
                        r_usleep(1000);
                        continue;
                    }

        	    }
        	}
        	else if(nRet == 0)// 超时
    		{
    			//zlog_error(NC_LOG, "!!!!!!!!!!!timeout!!!!!!!!!!!!!!!");
    			r_usleep(1000);
    			continue;
    		}
    		else if(nRet < 0)// 异常
    		{
    			zlog_error(NC_LOG,"!!!!!!!!!!!select < 0!!!!!!!!!!!!!!!");

    			Close_NetCtrlFd(netfd);
    			r_usleep(1000);
    			break;
    		}

    	}
    }

    zlog_debug(NC_LOG,"!!!!!!!!!!!NetCtrlTask End!!!!!!!!!!!!!!!");
}

void *CenCtrlTask(void *args)
{
    int32_t socket = -1;
    int8_t pcmd[2048]  = {0};

	fd_set readfd;
	struct timeval timeout;

	if(NULL == args)
	{
		zlog_error(NC_LOG, "SerialComTask failed, args is NULL!");
		return NULL;
	}
	NetctrlHandle *phandle = (NetctrlHandle *)args;

    zlog_debug(NC_LOG, "#########CenCtrlTask begin#######");
	while(1)
	{
		socket = InitSocket((const int8_t *)(CENCTRL_IP), CENCTRL_PORT);
		if(socket < 0)
		{
			//zlog_error(NC_LOG, "InitSocket failed!");
			r_sleep(5);
			continue;
		}

		zlog_debug(NC_LOG, "CenCtrlTask InitSocket [%d]! ",socket);

		phandle->cenctrlfd = socket;

		while(1)
		{
			int seret = 0;
			timeout.tv_sec = 0;
			timeout.tv_usec = 3;

			FD_ZERO(&readfd);
			FD_SET(socket, &readfd);
			seret = r_select(socket+1, &readfd, NULL, NULL, NULL);
			if(seret > 0) // FD已准备好
			{
				if(FD_ISSET(socket, &readfd))
				{
					int len 	= 0;
					int recvlen = 0;
					MsgHeader msg;

					CenSockLock();
					//printf("***** CenCtrlTask tcp_recv_longdata ****\n");
					recvlen = tcp_recv_longdata(socket, (int8_t *)&msg, MSGLEN);
					CenSockunLock();

					if(recvlen < MSGLEN || recvlen == -1)
					{
						zlog_debug(NC_LOG, "recvlen < MSGLEN  errno = %d  nLen = %d", errno, recvlen);
						if(errno !=  EAGAIN)
						{
							fprintf(stderr, "errno !=  EAGAIN, errmsg = %s\n", strerror(errno));
							//r_usleep(100000);
						}

						Close_CenCtrlFd(socket);
						r_usleep(500*1000);

						break;
					}

					msg.sMsgType = ntohs(msg.sMsgType);
					msg.sLen = ntohs(msg.sLen);
					msg.sVer = ntohs(msg.sVer);

					//printf("sLen = %d, sMsgType = %d sVer = %d\n", msg.sLen, msg.sMsgType, msg.sVer);

					r_memset(pcmd,0x0, sizeof(pcmd));

					CenSockLock();
					len = tcp_recv_longdata(socket, pcmd, msg.sLen-MSGLEN);
					CenSockunLock();
					//printf("recv buf:\n%s \n", pcmd);

					if(len < 1)
					{
						if(errno !=  EAGAIN)
						{
							fprintf(stderr, "errno !=  EAGAIN, errmsg = %s\n", strerror(errno));
							r_usleep(100000);
						}

						Close_CenCtrlFd(socket);
						r_usleep(500*1000);
						break;
					}

					/* 解析CenCtrl信令codeID */
					if(NET_FAIL == Analyze_CenCtrl_RecvMsg(phandle, pcmd))
					{
					    zlog_error(NC_LOG, "Cenctrl return result error!");
					    r_usleep(1000);
					    continue;
					}

				}
			}
			else if(seret == 0)// 超时
			{
			//	printf("!!!!!!!!!!!timeout!!!!!!!!!!!!!!!\n");
				r_usleep(1000);
				continue;
			}
			else if(seret < 0)// 异常
			{
				zlog_error(NC_LOG,"!!!!!!!!!!!select < 0!!!!!!!!!!!!!!!");

				Close_CenCtrlFd(socket);
				r_usleep(1000);
				break;
			}

		}

	}

	phandle->runstate = NET_STOP;
	Close_CenCtrlFd(socket);

	if(pcmd != NULL)
	{
		r_free(pcmd);
	}

	return NULL;
}

void *TimeOutTask(void *args)
{
	NetctrlHandle *pHandle = (NetctrlHandle *)args;

	list_head *pcurnode = NULL;
	TimeOut *pattr = NULL;

	zlog_debug(NC_LOG, "**************TimeOutTask begin ********");
	while(1)
	{
	    r_sleep(1);

		TimeoutLock();
		/* 查找是否存在该设备 */
		list_for_each_item(pcurnode, pHandle->pheadnode)
	    {
	        if(NULL != pcurnode)
	        {
				int32_t ret_val = 0;
	            pattr = list_entry(pcurnode, TimeOut, stlist);

			 	ret_val = upper_msg_monitor_time_out_status(&pattr->starttime, 10);
				if(1 == ret_val)
				{
					//printf("111111111111111111111 %d\n",pattr->state);
					//超时则删除
					list_remove(pcurnode);
					free(pattr);
				}
	        }
	    }
		TimeoutunLock();
	}
}

void *HeartBeatTask(void *args)
{
    if(NULL == args)
    {
        zlog_error(NC_LOG, "HeartBeatTask failed, args is NULL! ");
        return NULL;
    }


    NetctrlHandle *pHandle = (NetctrlHandle*)args;
    r_sleep(3);
    zlog_debug(NC_LOG, "*********HeartBeatTask begin*****");
    while (1)
    {
        if(pHandle->netctrlfd <= 0)
        {
            r_sleep(5);
            continue;
        }

        if (CheckTimeOut(pHandle, REC_HEARTBEAT))
        {
            DelTimeOut(pHandle, REC_HEARTBEAT);

            if(NET_STOP == pHandle->runstate)
            {
                r_sleep(5);
                continue;
            }

            uint8_t loginmsg[BUFF_LEN] = {0};
            Package_MsgCode(loginmsg, REC_HEARTBEAT);

            SendMsgToNetCtrl(pHandle, CMD_LEN, loginmsg);
            //printf("Send Heartbeat Msg \n");
            r_sleep(10);

        }
        else
        {
            uint8_t loginmsg[BUFF_LEN] = {0};
            Package_MsgCode(loginmsg, REC_LOGIN);

            SendMsgToNetCtrl(pHandle, CMD_LEN, loginmsg);

            AddTimeOut(pHandle, REC_HEARTBEAT, 0);

            r_sleep(3);
        }

    }
}




int32_t RegisterNetControlTask()
{
	int fd;
    pthread_t net_ctrl_id = 0;

    InitNetCtrl();

    if(r_pthread_create(&net_ctrl_id, NULL, NetCtrlTask, (void *)pgNet_Handle))
	{
		zlog_error(NC_LOG, "NetCtrlTask failed, err msg: %s", strerror(errno));
		return NET_FAIL;
	}

	if(r_pthread_create(&net_ctrl_id, NULL, CenCtrlTask, (void *)pgNet_Handle))
	{
		zlog_error(NC_LOG, "HeartBeatTask failed, err msg: %s", strerror(errno));
		return NET_FAIL;
	}

	if(r_pthread_create(&net_ctrl_id, NULL, TimeOutTask, (void *)pgNet_Handle))
	{
		zlog_error(NC_LOG, "TimeOutTask failed, err msg: %s", strerror(errno));
		return NET_FAIL;
	}

	if(r_pthread_create(&net_ctrl_id, NULL, HeartBeatTask, (void *)pgNet_Handle))
	{
		zlog_error(NC_LOG, "HeartBeatTask failed, err msg: %s", strerror(errno));
		return NET_FAIL;
	}


/*
	if(r_pthread_create(&net_ctrl_id, NULL, led_display_IP, (void *)NULL))
	{
		zlog_error(NC_LOG, "HeartBeatTask failed, err msg: %s", strerror(errno));
		return NET_FAIL;
	}
	*/

	while(1)
	{
	    r_usleep(1000*10);
	}
	return 0;
}



