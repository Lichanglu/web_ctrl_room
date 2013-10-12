#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "xml_base.h"
#include "new_tcp_com.h"
#include "process_xml_cmd.h"
#include "nslog.h"
#if 1
//#ifdef SUPPORT_XML_PROTOCOL
#define MAX_CLIENT				6
#define INVALID_SOCKET 			-1
#define THREAD_FAILURE      (void *) -1
static 	sem_t	g_new_tcp_lock[2];
static pthread_t cli_pthread_id[2][6];
static int 	gRunStatus[2] = {1, 1};
#define TRUE  1
#define FALSE 0
typedef struct client_msg_arg_ {
	int index;
	int pos;
} client_msg_arg_t;

typedef struct __MSGHEAD__ {
	uint16_t		nLen;
	uint16_t  	nVer;
	uint8_t	    nMsg;
	uint8_t	    szTemp[3];
} MSGHEAD;

static uint32_t getCurrentTime(void)
{
	struct timeval tv;
	struct timezone tz;
	uint32_t ultime;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return (ultime);
}


//录制时锁定音频参数标志位

static int g_fix_resolution[2] = {0};
void set_fix_resolution(int index, int value)
{
	if((value == 0) || (value == 1)  || (value == 2)) {
		g_fix_resolution[index] = value;
	}
}

int get_fix_resolution(int index)
{
	return g_fix_resolution[index];
}

//用于与录播服务器通信的心跳接口。
int add_heart_count(int index, int cli)
{
	int value = 0;
	pthread_mutex_lock(&(g_client_para[index].cliDATA[cli].heart_count_mutex));
	g_client_para[index].cliDATA[cli].heartCount++;
	value = g_client_para[index].cliDATA[cli].heartCount;
	pthread_mutex_unlock(&(g_client_para[index].cliDATA[cli].heart_count_mutex));
	return value;
}

int cut_heart_count(int index, int cli)
{
	int value;
	pthread_mutex_lock(&(g_client_para[index].cliDATA[cli].heart_count_mutex));
	g_client_para[index].cliDATA[cli].heartCount--;
	value = g_client_para[index].cliDATA[cli].heartCount;
	pthread_mutex_unlock(&(g_client_para[index].cliDATA[cli].heart_count_mutex));

	return value;
}

int set_heart_count(int index, int cli, int value)
{
	pthread_mutex_lock(&(g_client_para[index].cliDATA[cli].heart_count_mutex));
	g_client_para[index].cliDATA[cli].heartCount = value;
	pthread_mutex_unlock(&(g_client_para[index].cliDATA[cli].heart_count_mutex));
	return value;
}

int get_heart_count(int index, int cli)
{
	int value;
	pthread_mutex_lock(&(g_client_para[index].cliDATA[cli].heart_count_mutex));
	value = g_client_para[index].cliDATA[cli].heartCount;
	pthread_mutex_unlock(&(g_client_para[index].cliDATA[cli].heart_count_mutex));
	return value;
}

int tcp_recv(int  s, char *buff, int len, int flags)
{
	int toplen = 0;
	int readlen = 0;

	while(len - toplen > 0) {
		readlen =  recv(s, buff + toplen, len - toplen, flags);

		if(readlen <= 0) {
			if(errno == EAGAIN) {
				PRINTF("ERROR,errno = %d,msg=%s,len =%d\n", errno, strerror(errno), readlen);
				usleep(10000);
				continue;
			}

			PRINTF("ERROR,errno = %d,msg=%s,len =%d\n", errno, strerror(errno), readlen);
			return -1;
		}

		if(readlen != len - toplen) {
			PRINTF("WARNNING,i read the buff len = %d,i need len = %d\n", readlen, len);
		}

		toplen += readlen;
	}

	return toplen;
}


/*Initial client infomation*/
void InitClientData_new(int index)
{
	int cli;

	for(cli = 0; cli < MAX_CLIENT; cli++) 	{
		SETCLIUSED_NEW(index, cli, FALSE);
		SETSOCK_NEW(index, cli, INVALID_SOCKET);
		SETCLILOGIN_NEW(index, cli, FALSE);
		SETLOWRATEFLAG_NEW(index, cli, STOP);
		SET_SEND_AUDIO_NEW(index, cli, FALSE);
		SET_SEND_VIDEO_NEW(index, cli, FALSE);
		SET_PARSE_LOW_RATE_FLAG(index, cli, INVALID_SOCKET);
		set_heart_count(index, cli, FALSE);
#ifdef SUPPORT_IP_MATRIX
		SET_PASSKEY_FLAG(index, cli, 0);
#endif
		pthread_mutex_init(&(g_client_para[index].cliDATA[cli].heart_count_mutex), NULL);


	}

	return;
}
void ClearLostClient_new(int index)
{
	int cli ;
	unsigned long currenttime = 0;
	currenttime = getCurrentTime();

	for(cli = 0; cli < MAX_CLIENT; cli++) {
		if(!ISUSED_NEW(index, cli) && ISSOCK_NEW(index, cli)) {
			PRINTF("[ClearLostClient] Socket = %d \n", GETSOCK_NEW(index, cli));
			shutdown(GETSOCK_NEW(index, cli), 2);
			close(GETSOCK_NEW(index, cli));
			SETSOCK_NEW(index, cli, INVALID_SOCKET);
			SETCLILOGIN_NEW(index, cli, FALSE) ;
		}

		if(ISUSED_NEW(index, cli) && (!ISLOGIN_NEW(index, cli))) {
			if((currenttime - GETCONNECTTIME_NEW(index, cli)) > 1000) {
				PRINTF("\n\n\n\n ClearLostClient[%d] Socket = %d,client %d \n\n\n\n", index, GETSOCK_NEW(index, cli), cli);
				shutdown(GETSOCK_NEW(index, cli), 2);
				close(GETSOCK_NEW(index, cli));
				SETSOCK_NEW(index, cli, INVALID_SOCKET);
				SETCLIUSED_NEW(index, cli, FALSE);
				SETCLILOGIN_NEW(index, cli, FALSE) ;
			}
		}

	}
}


/*get null client index*/
static int GetNullClientData_new(int index)
{
	int cli ;
	int connected_num;
	connected_num = read_client_num(index);

	if(connected_num >= MAX_CLIENT) {
		PRINTF("total client still up to max client num %d\n", connected_num);
		return -1;
	} else {
		PRINTF("cli = %d\n", connected_num);

		for(cli = 0; cli < MAX_CLIENT; cli++) {
			PRINTF("ISUSED_NEW(cli)=%d,(ISLOGIN_NEW(cli))=%d\n", (ISUSED_NEW(index, cli)), (ISLOGIN_NEW(index, cli)));

			if((!ISUSED_NEW(index, cli)) && (!ISLOGIN_NEW(index, cli))) {
				PRINTF("[add Client]index: [%d]number = %d \n", index, cli);
				return cli;
			}
		}
	}

	return -1;
}

static int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;


	while(nWriteLen < nSize) {

		nRet = send(s, (char *)pBuf + nWriteLen, nSize - nWriteLen, 0);

		if(nRet < 0) {
			if((errno == ENOBUFS || errno == 11) && (nCount < 10)) {
				ERR_PRN("network buffer have been full! errno[%d]:[%s]\n", errno, strerror(errno));
				usleep(10000);
				nCount++;
				continue;
			}

			ERR_PRN("ENOBUFS:[%d] socket:[%d] errno[%d]:[%s]\n", ENOBUFS, s, errno, strerror(errno));
			return nRet;
		} else if(nRet == 0) {
			ERR_PRN("Send Net Data Error nRet= %d\n", nRet);
			continue;
		}

		nWriteLen += nRet;
		nCount = 0;
	}

	return nWriteLen;
}

int tcp_send_data(int sockfd, char *send_buf)
{
	int send_len = 0;
	int ret = 0;
	MsgHead msg_head;
	char tmpbuf[2050] = {0};
	send_len = strlen(send_buf) + sizeof(MsgHead);
	msg_head.sLen = htons(send_len);
	msg_head.sVer = htons(2012);
	msg_head.sMsgCode = 1;
	msg_head.sData = 0;
	//	PRINTF("sockfd=%d\n%s\n", sockfd, send_buf);
	memcpy(tmpbuf, &msg_head, sizeof(MSGHEAD));
	memcpy(tmpbuf + sizeof(MSGHEAD), send_buf, strlen(send_buf));
	ret = WriteData(sockfd, tmpbuf, send_len);

	nslog(NS_INFO, "send_len = %d,len=%d,ret=%d,msgcode=%d\n", send_len, strlen(send_buf), ret, msg_head.sMsgCode);

	if(ret < 0) {
		PRINTF("send xml context failed errno[%d]:[%s]\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}

static void set_ip_matrix_userid(int index, char *user_id)
{
	sprintf(user_id, "%d", index);
}

void  *recv_client_msg_thread(void *pParams)
{
	PRINTF("pid=%d\n", getpid());
	char szData[2048] = {0};
	unsigned  int nLen;
	int 	total_len;
	int sSocket;
	int nPos;
	int index = -1;
	MsgHead  msg_info;
	client_msg_arg_t *arg = (client_msg_arg_t *)pParams;
	nPos = arg->pos;
	index = arg->index;
	PRINTF("[enter recv_client_msg_thread] index:[%d] pos:[%d]\n", index, nPos);
	char user_id[16] = {0};
	int  msg_code = 0;
	char  passkey[64] = {0};
	int  count = 0;

	struct timeval timeout ;
	int ret = 0;

	if(pParams != NULL) {
		PRINTF("enter free(pParams)\n");
		free(pParams);  //free memory
		pParams = NULL;
	}

	sSocket = GETSOCK_NEW(index, nPos);
	PRINTF("index:[%d] sockfd =%d,pos =%d\n", index, sSocket, nPos);
	timeout.tv_sec = 3 ; //3
	timeout.tv_usec = 0;

	ret = setsockopt(sSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

	if(ret == -1) {
		ERR_PRN("setsockopt() Set Send Time Failed\n");
	}

	sem_post(&g_new_tcp_lock[index]);
	PRINTF("recv_client_msg_thread     sem_post!!!!\n");
	set_ip_matrix_userid(index, user_id);

	while(gRunStatus[index]) {
		memset(szData, 0, sizeof(szData));

		if(sSocket <= 0) {
			goto ExitClientMsg;
		}

		total_len = tcp_recv(sSocket, &msg_info, sizeof(msg_info), 0);


		if(total_len == -1) {
			ERR_PRN("error:%d,error msg: = %s\n", errno, strerror(errno));
			goto ExitClientMsg;
		}

		if(ntohs(msg_info.sVer) != 2012) {
			ERR_PRN(" sver error:%d,error msg: = %s\n", errno, strerror(errno));
			goto ExitClientMsg;
		}

		total_len = ntohs(msg_info.sLen) - sizeof(msg_info);

		if(total_len <= 0) {
			PRINTF("nMsgLen  < HEAD_LEN\n");
			goto ExitClientMsg;
		}

		if(total_len > 0) {
			nLen = tcp_recv(sSocket, szData, total_len, 0);

			if(nLen == -1 || nLen < total_len) {
				ERR_PRN("nLen < nMsgLen -HEAD_LEN\n");

				goto ExitClientMsg;
			}


			if(parse_cli_xml_msg(index, nPos, szData, &msg_code, passkey, user_id) < 0) {
				ERR_PRN("parse xml msg failed\n");
				//goto ExitClientMsg;
			}
		}

#if 0

		if(process_cli_xml_msg(nPos, msg_code, passkey, user_id) < 0) {
			PRINTF("process xml msg failed\n");
			//goto ExitClientMsg;
		}

#endif

		//PRINTF( "Switch End!\n");
	}

ExitClientMsg:
	//cli_pthread_id[nPos] = 0;
	PRINTF("Exit Client Msg thread:%d nPos = %d sSocket = %d\n", pthread_self(), nPos, sSocket);

	if(sSocket == GETSOCK_NEW(index, nPos)) {
		PRINTF("Exit Client Msg index:[%d] nPos = %d sSocket = %d\n", index, nPos, sSocket);
		SETCLIUSED_NEW(index, nPos, FALSE);
		SET_SEND_AUDIO_NEW(index, nPos, FALSE);
		SETCLILOGIN_NEW(index, nPos, FALSE);
		SETLOWRATEFLAG_NEW(index, nPos, STOP);
		SET_SEND_VIDEO_NEW(index, nPos, FALSE);
		SET_PARSE_LOW_RATE_FLAG(index, nPos, INVALID_SOCKET);
		SET_FIX_RESOLUTION_FLAG(index, nPos, FALSE);
		//IISCloseLowRate_new();
		set_heart_count(index, nPos, FALSE);
		close(sSocket);
		SETSOCK_NEW(index, nPos, INVALID_SOCKET);
#ifdef SUPPORT_IP_MATRIX

		if(get_audio_lock_resolution(index) > 0  && (GET_PASSKEY_FLAG(index, nPos) == 1)) {
			cut_audio_lock_resolution(index);
		}

		if(get_video_lock_resolution(index) > 0 && (GET_PASSKEY_FLAG(index, nPos) == 1)) {
			cut_video_lock_resolution(index);
		}

		if(get_ipmatrix_lock_resolution(index) > 0 && (GET_PASSKEY_FLAG(index, nPos) == 1)) {
			cut_ipmatrix_lock_resolution(index);
		}

		SET_PASSKEY_FLAG(index, nPos, 0);
#endif
	} else {

		int cli ;

		for(cli = 0; cli < MAX_CLIENT; cli++) {
			PRINTF("index:[%d] socket = %d, blogin=%d,bused=%d,sendAudioFlag=%d\n",
			       index, g_client_para[index].cliDATA[cli].sSocket,
			       g_client_para[index].cliDATA[cli].bLogIn,
			       g_client_para[index].cliDATA[cli].bUsed,
			       g_client_para[index].cliDATA[cli].sendAudioFlag);
		}

		PRINTF("index:[%d] socket =%d,pos=%d,s=%d\n",
		       index, sSocket, nPos, GETSOCK_NEW(index, nPos));
	}

	pthread_detach(pthread_self());
	pthread_exit(NULL);
}


int create_signal_cli_thread(int index, int sockfd, struct sockaddr_in *cli_addr)
{
	char newipconnect[20] = {0};
	//打印客户端ip//
	inet_ntop(AF_INET, (void *) & (cli_addr->sin_addr), newipconnect, 16);
	PRINTF("sockfd =%d ip =%s\n", sockfd, newipconnect);
	int nPos = 0;
	int nLen = 0;
	int cli_num = 0;
	char send_buf[256] = {0};
	char user_id[8] = {0};
	char dtype[16] = {0};
	unsigned int cur_time = 0;
	GetDeviceType(dtype);
	nPos = GetNullClientData_new(index);
	cli_num = read_client_num(index);
	PRINTF("GetNullClientData_new[%d] = %d\n", index, nPos);
	nLen = sizeof(struct sockaddr_in);

	if(-1 == nPos || cli_num >= 6) 	{
		//需要告警上报
		package_head_msg(send_buf, 30087, dtype, "0", user_id);
		tcp_send_data(sockfd, send_buf);
		ERR_PRN("ERROR: max client error\n");
		close(sockfd);
		sockfd = -1;
	} else {
		int nSize = 0;
		int result;
		client_msg_arg_t *arg = malloc(sizeof(client_msg_arg_t));
		/* set client used */

		PRINTF("index : [%d]pos =%d,sockfd =%d\n", index, nPos, sockfd);
		SETCLIUSED_NEW(index, nPos, TRUE);
		SETCLILOGIN_NEW(index, nPos, TRUE);
		SETSOCK_NEW(index, nPos, sockfd);
		cur_time = getCurrentTime();
		SETCONNECTTIME_NEW(index, nPos, cur_time);
		nSize = 1;

		if((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize,
		               sizeof(nSize))) == -1) {
			perror("setsockopt failed");
		}

		nSize = 0;
		nLen = sizeof(nLen);
		result = getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &nSize , &nLen);

		if(result) {
			ERR_PRN("getsockopt() errno:%d socket:%d  result:%d\n", errno, sockfd, result);
		}

		nSize = 1;
		PRINTF("Clent:%s connected,nPos:%d socket:%d!\n", newipconnect, nPos, sockfd);

		//Create ClientMsg task!
		arg->pos = nPos;
		arg->index = index;
		result = pthread_create(&cli_pthread_id[index][nPos], NULL, (void *)recv_client_msg_thread, (void *)arg);

		if(result) {
			close(sockfd);
			sockfd = -1;
			ERR_PRN("creat pthread ClientMsg error  = %d!\n" , errno);
			return 0;
		}

		sem_wait(&g_new_tcp_lock[index]);
	}
}
void *open_new_tcp_task(void *arg)
{
	PRINTF("open_new_tcp_task is start\n");
	int index = *(int *)arg;
	int serv_sockfd = -1, cli_sockfd;
	struct sockaddr_in serv_addr, cli_addr;
	int file_flag = -1;
	int len = 0;
	int opt = 1;
	int ipos = 0;
	int clientsocket = -1;
	int ret = 0;
	int ipaddr;
	PRINTF("open_new_tcp_task index:[%d]\n", index);
	sem_init((sem_t *)&g_new_tcp_lock[index], 0, 0);
	int eth = index;
	char ip[16] = {0};
	InitClientData_new(index);
	ipaddr = get_local_ip("eth0", ip);
	PRINTF("open_new_tcp_task index:[%d] ipaddr:[%x]\n", index, ipaddr);
RECREATE_TCP_SOCK:

	bzero(&serv_addr, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(NEW_TCP_SOCK_PORT);
	serv_addr.sin_addr.s_addr = ipaddr;
	serv_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(serv_sockfd < 0)  {
		ERR_PRN("ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		gRunStatus[index] = 0;
		return NULL;
	}

	setsockopt(serv_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)  {
		ERR_PRN("bind terror:%d,error msg: = %s\n", errno, strerror(errno));
		gRunStatus[index] = 0;
		return NULL;
	}

	if(listen(serv_sockfd, 10) < 0) {
		ERR_PRN("listen error:%d,error msg: = %s", errno, strerror(errno));
		gRunStatus[index] = 0;
		return NULL;
	}

#if 0

	if((file_flag = fcntl(serv_sockfd, F_GETFL, 0)) == -1) {
		ERR_PRN("fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));

		//gblSetQuit();
		return;
	}

	if(fcntl(serv_sockfd, F_SETFL, file_flag | O_NONBLOCK) == -1) {
		ERR_PRN("fcntl F_SETFL error:%d,error msg: = %s\n", errno, strerror(errno));

		//gblSetQuit();
		return;
	}

#endif
	PRINTF("sockfd=%d\n", serv_sockfd);

	while(gRunStatus[index]) {
		memset(&cli_addr, 0, sizeof(struct sockaddr_in));
		len = sizeof(struct sockaddr_in);
		cli_sockfd = accept(serv_sockfd , (void *)&cli_addr, &len);

		if(cli_sockfd > 0) {
			PRINTF("serv_sockfd=%d,cli_sockfd=%d\n", serv_sockfd, cli_sockfd);
			create_signal_cli_thread(index, cli_sockfd, &cli_addr);
			PRINTF("sem_wait() semphone inval!!!\n");
		} else {
			if(errno == ECONNABORTED || errno == EAGAIN) { //软件原因中断
				usleep(100000);
				continue;
			}

			usleep(3000000);
		}
	}

	for(ipos = 0; ipos < MAX_CLIENT_NUM; ipos++) {
		if(cli_pthread_id[index][ipos]) {
			clientsocket = GETSOCK_NEW(index, ipos);

			if(clientsocket != INVALID_SOCKET) {
				close(clientsocket);
				SETSOCK_NEW(index, ipos, INVALID_SOCKET);
			}

			if(pthread_join(cli_pthread_id[index][ipos], &ret) == 0) {
				if(ret == THREAD_FAILURE) {
					ERR_PRN("drawtimethread ret == THREAD_FAILURE \n");
				}
			}
		}
	}
}

void close_new_tcp_task(int index)
{
	gRunStatus[index] = 0;
}




#endif

