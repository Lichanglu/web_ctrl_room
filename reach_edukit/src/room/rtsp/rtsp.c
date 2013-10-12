/******************************************************************
*
*rtsp.c
*============================================================================
* This file is used for formating h.264/aac  to rtsp flow
* Ver      alpha_1.0
* Author  jbx
* Shenzhen reach 2010.9.6
*============================================================================
*******************************************************************/
#if 0
#ifdef USE_LINUX_PLATFORM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <math.h>
#endif

#ifdef USE_WINDOWS_PLATFORM
#include <stdio.h>
#include   <afx.h>
#include <iostream>
#include <process.h>
#include <stdlib.h>
#include <math.h>
#include <winsock2.h>
#endif

#include "mid_platform.h"
#include "rtsp_client.h"
#include "rtsp_stream_deal.h"
#include "rtsp.h"
#include "rtsp_authentication.h"
#include "app_rtsp_center.h"

#ifdef USE_LINUX_PLATFORM
#include "../log/log_common.h"
#endif
/*VLC connect information*/
static RTSPCliParam           		 	gRTSPCliPara[MAX_RTSP_CLIENT_NUM];
/*rtsp audio nalbuf*/
static NALU_t                             	*g_audio_puiNalBuf  = NULL;
//thread id
static mid_plat_pthread_t 			rtsp_threadid[MAX_RTSP_CLIENT_NUM]  ;
//have a rtsp client
static int                                   	g_rtsp_flag = 0;
static char 						g_local_ip[64]  = {0};

/*Resize Param table*/
typedef struct __THREAD_PARAM__ {
	int                 				nPos;
	mid_plat_socket                 	client_socket;
	mid_plat_sockaddr_in  		client_addr;
} Thread_Param;


static int SendAnnounce(int socket, mid_plat_sockaddr_in Listen_addr, char *localip , STR_CLIENT_MSG *strMsg);
static int ResponseTeardown(mid_plat_socket sock, STR_CLIENT_MSG *strMsg);
static void rtsp_change_video_resolve(int roomid);
static int rtsp_get_client_timeout(void);

//add by zhangmin
#ifdef GDM_RTSP_SERVER
static CString GetCurrectIP(void);
static CString GetCurrectIP()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	char name[255];
	CString ip;
	PHOSTENT hostinfo;
	wVersionRequested = MAKEWORD(2, 0);

	if(WSAStartup(wVersionRequested, &wsaData) == 0) {
		if(gethostname(name, sizeof(name)) == 0) {
			if((hostinfo = gethostbyname(name)) != NULL) {
				ip = inet_ntoa(*(struct in_addr *)*hostinfo->h_addr_list);
			}
		}

		WSACleanup();
	}

	return ip;
}

//get server local ip
int rtsp_get_local_ip(char *buff, int len)
{
	CString ip ;

	if(strlen(g_local_ip) == 0) {
		ip = GetCurrectIP();
		memcpy(g_local_ip, ip.GetBuffer(ip.GetLength()), ip.GetLength());
	}

	PRINTF("g_local_ip = #%s#\n", g_local_ip);

	if(len > strlen(g_local_ip)) {
		len = strlen(g_local_ip);
	}

	memcpy(buff, g_local_ip, len);
	return 0;

}

#else

int rtsp_set_local_ip(unsigned int localipaddr)
{
	struct in_addr          addr_ip;
	char local_ip[16] = {0};
	memcpy(&addr_ip, &localipaddr, 4);
	memcpy(local_ip, inet_ntoa(addr_ip), 16);

	strcpy(g_local_ip, local_ip);
	return 0;
}

//get server local ip
int rtsp_get_local_ip(char *buff, int len)
{
	strcpy(buff, g_local_ip);
	return 0;
}
#endif

static unsigned long getCurrentTimets(void)
{
	struct timeval tv;
	struct timezone tz;
	unsigned long ultime;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec * 1000 + ((tv.tv_usec) / 1000);
	return (ultime);
}


/**************************************************************************************************
                                        为NALU_t结构体分配内存空间
**************************************************************************************************/
static NALU_t *AllocNALU(int buffersize)
{
	NALU_t *n;

	if((n = (NALU_t *)calloc(1, sizeof(NALU_t))) == NULL) {
		exit(0);
	}

	n->max_size = buffersize;

	if((n->buf = (unsigned char *)calloc(buffersize, sizeof(char))) == NULL) {
		free(n);
		exit(0);
	}

	return n;
}



/*********************************************************************************************************
                                        为NALU_t结构体释放内存空间
*********************************************************************************************************/
static void FreeNALU(NALU_t *n)
{
	if(n) {
		if(n->buf) {
			free(n->buf);
			n->buf = NULL;
		}

		free(n);
	}
}

static int GetMsgType(char *pDataFromClient, int usDataLen)
{
	char *p = pDataFromClient;
	int itype = -1;
	int m;

	for(m = 0; m < usDataLen - 7; m++) {
		if(p[m] == 0x4f && p[m + 1] == 0x50 && p[m + 2] == 0x54 && p[m + 3] == 0x49 \
		   && p[m + 4] == 0x4f && p[m + 5] == 0x4e) { //receive option
			itype = OPTION;
			break;
		} else if(p[m] == 0x44 && p[m + 1] == 0x45 && p[m + 2] == 0x53 && p[m + 3] == 0x43 \
		          && p[m + 4] == 0x52 && p[m + 5] == 0x49 && p[m + 6] == 0x42 && p[m + 7] == 0x45) {
			itype = DESCRIBE;
			break;
		} else if(p[m] == 0x53 && p[m + 1] == 0x45 && p[m + 2] == 0x54 && p[m + 3] == 0x55 \
		          && p[m + 4] == 0x50) {
			itype = SETUP;
			break;
		} else if(p[m] == 0x50 && p[m + 1] == 0x4c && p[m + 2] == 0x41 && p[m + 3] == 0x59) {
			itype = PLAY;
			break;
		} else if(p[m] == 0x50 && p[m + 1] == 0x41 && p[m + 2] == 0x55 && p[m + 3] == 0x53 \
		          && p[m + 4] == 0x45) {
			itype = PAUSE;
			break;
		} else if(p[m] == 0x54 && p[m + 1] == 0x45 && p[m + 2] == 0x41 && p[m + 3] == 0x52 \
		          && p[m + 4] == 0x44 && p[m + 5] == 0x4f && p[m + 6] == 0x57 && p[m + 7] == 0x4e) {
			itype = TEARDOWN;
			break;
		} //GET_PARAMETER
		else if(p[m] == 'G' && p[m + 1] == 'E' && p[m + 2] == 'T' && p[m + 3] == '_' \
		        && p[m + 4] == 'P' && p[m + 5] == 'A' && p[m + 6] == 'R' && p[m + 7] == 'A'\
		        && p[m + 8] == 'M' && p[m + 9] == 'E' && p[m + 10] == 'T' && p[m + 11] == 'E' && p[m + 12] == 'R') {
			itype = GET_PARAMETER;
			break;
		} //SET_PARAMETER
		else if(p[m] == 'S' && p[m + 1] == 'E' && p[m + 2] == 'T' && p[m + 3] == '_' \
		        && p[m + 4] == 'P' && p[m + 5] == 'A' && p[m + 6] == 'R' && p[m + 7] == 'A'\
		        && p[m + 8] == 'M' && p[m + 9] == 'E' && p[m + 10] == 'T' && p[m + 11] == 'E' && p[m + 12] == 'R') {
			itype = SET_PARAMETER;
			break;
		} else {
			;
		}
	}

	return itype;

}



static int GetCSeq(char *pDataFromClient, int usDataLen, STR_CLIENT_MSG *strMsg)
{
	char *p = pDataFromClient;
	int i = 0;
	char aucCSeq[10];
	unsigned int uiCSeq = 0;
	int m, j;

	for(m = 0; m < usDataLen; m++) {
		if(p[m] == 0x43 && p[m + 1] == 0x53 && p[m + 2] == 0x65 && p[m + 3] == 0x71 \
		   && p[m + 4] == 0x3a) { //CSeq
			while(((m + 6 + i) < usDataLen) && (p[m + 6 + i] != 0x0a)) {
				aucCSeq[i] = p[m + 6 + i] - 0x30;
				strMsg->aucCSeqSrc[i] = p[m + 6 + i];
				i++;
			}

			strMsg->ucCSeqSrcLen = i - 1;

			for(j = 0; j < i - 1; j++) {
				uiCSeq += aucCSeq[j] * pow(10, (i - 2 - j));
			}

			break;
		}
	}

	//	PRINTF("uicseq = %d\n",uiCSeq);
	return uiCSeq;
}


static int ConventNumberToString(char *pString, int uiNumber)
{
	int i = 0;
	int j = 0;
	char dat[5];

	for(i = 0; i < 5; i++) {
		dat[0] = uiNumber / 10000;
		dat[1] = (uiNumber % 10000) / 1000;
		dat[2] = (uiNumber % 1000) / 100;
		dat[3] = (uiNumber % 100) / 10;
		dat[4] = uiNumber % 10;
	}

	for(j = 0; j < 5; j++) {
		if(dat[j] == 0) {
			i--;
		} else {
			break;
		}
	}

	for(j = 0; j < i; j++) {

		pString[j] = dat[5 - i + j] + 0x30;
	}

	return i;
}


//**************************************************************************
/*RTSP 初始化*/
//**************************************************************************
static int RtspThreadInit(void)
{
	int cli;

	for(cli = 0; cli < MAX_RTSP_CLIENT_NUM; cli++) {
		SETRTSPUDPREADY(0, cli, 0);
		SETRTSPUSED(0, cli, FALSE);
		SETRTSPUDPAUDIOSOCK(0, cli, 0);
		SETRTSPUDPVIDEOSOCK(0, cli, 0);
		SETRTSPROOMID(0, cli, -1);
		SETRTSPCLIENTTYPE(0, cli, VLC_CLIENT);
	}

	g_audio_puiNalBuf = AllocNALU(20000);
	return 0;
}
//**************************************************************************
/*RTSP server exit*/
//**************************************************************************
static int RtspExit()
{
	int cli;

	for(cli = 0; cli < MAX_RTSP_CLIENT_NUM; cli++) {
		SETRTSPUDPREADY(0, cli, 0);
		SETRTSPUSED(0, cli, FALSE);
		SETRTSPUDPAUDIOSOCK(0, cli, 0);
		SETRTSPUDPVIDEOSOCK(0, cli, 0);
		SETRTSPROOMID(0, cli, -1);
	}

	//  pthread_mutex_destroy(&send_rtp_data);
	FreeNALU(g_audio_puiNalBuf);
	g_audio_puiNalBuf = NULL;
	return 0;
}


//**************************************************************************
/*get null RTSPclient index*/
//**************************************************************************
static int RtspGetNullClientIndex()
{
	int cli ;

	for(cli = 0; cli < MAX_RTSP_CLIENT_NUM; cli++) {
		if((!ISRTSPUSED(0, cli))) {
			//need some new
			SETRTSPUSED(0, cli, TRUE);
			SETRTSPCLIENTTYPE(0, cli, VLC_CLIENT);
			SETRTSPUDPAUDIOSOCK(0, cli, -1);
			SETRTSPUDPVIDEOSOCK(0, cli, -1);
			return cli;
		}
	}

	return -1;
}

//**************************************************************************
/*get RTSP clients' number*/
//**************************************************************************
static int RtspGetClientNum()
{
	int cli ;
	int count = 0;

	for(cli = 0; cli < MAX_RTSP_CLIENT_NUM; cli++) {
		if((ISRTSPUSED(0, cli))) {
			count = count + 1;
		}
	}

	return count;
}

/*********************************************************************************************************
                                        获取连接用户信息
*********************************************************************************************************/
static int GetVlcMsgType(char *pDataFromClient, int usDataLen, STR_CLIENT_MSG *strMsg)
{
	int type;
	strMsg->iType = GetMsgType(pDataFromClient, usDataLen);
	strMsg->uiCseq = GetCSeq(pDataFromClient, usDataLen, strMsg);

	if(strMsg->iType >= MAX_OPERATE || strMsg->uiCseq > 65535) {
		return 0;
	}

	type = strMsg->iType;
	return type;
}

//*******************************************************************************************************************
/*设置建立信息信息*/
//*******************************************************************************************************************
static void PraseSETUPMsg(char *pDataFromClient, int usDataLen, STR_CLIENT_MSG *strMsg)
{
	int i = 0, j = 0;
	char *p = pDataFromClient;
	strMsg->ucTransNum = 0;

	for(i = 0; i < usDataLen; i++) {
		/*Transport*/
		if(p[i] == 0x54 && p[i + 1] == 0x72 && p[i + 2] == 0x61 && p[i + 3] == 0x6e && p[i + 4] == 0x73 && p[i + 5] == 0x70 &&
		   p[i + 6] == 0x6f && p[i + 7] == 0x72 && p[i + 8] == 0x74 && p[i + 9] == 0x3a) {
			for(j = 0; j < 100; j++) {
				if(p[i + 11 + j] != 0x0a) {
					strMsg->aucTrans[j] = p[i + 11 + j];
					strMsg->ucTransNum++;
				} else {
					strMsg->ucTransNum -= 1;
					return;
				}
			}

			strMsg->ucTransNum -= 1;
			//			PRINTF("g_strMsg.ucTransNum = %d,g_strMsg.aucTrans[0] =%c\n",strMsg->ucTransNum,strMsg->aucTrans[0]);

			return;
		}
	}

	strMsg->ucTransNum -= 1;
	return;
}

//*******************************************************************************************************************
/*获取客户端端口*/
//*******************************************************************************************************************
static int GetClientPort(char *pDataFromClient, int usDataLen, STR_CLIENT_MSG *strMsg, int trick_id)
{
	char *p = pDataFromClient;
	unsigned int i = 0;
	int m;
	strMsg->ucClientportLen[trick_id] = 0;

	for(m = 0; m < usDataLen; m++) {
		if(p[m] == 0x63 && p[m + 1] == 0x6c && p[m + 2] == 0x69 && p[m + 3] == 0x65 && p[m + 4] == 0x6e && p[m + 5] == 0x74 \
		   && p[m + 6] == 0x5f && p[m + 7] == 0x70 && p[m + 8] == 0x6f && p[m + 9] == 0x72 && p[m + 10] == 0x74 && p[m + 11] == 0x3d) { //client_port=
			while(p[m + 12 + i] != 0x3b && p[m + 12 + i] != 0x0d && p[m + 13 + i] != 0x0a) { //not  ";"  "nler"
				strMsg->aucClientport[trick_id][i] = p[m + 12 + i];
				strMsg->ucClientportLen[trick_id]++;
				i++;
			}
		}
	}

	//	PRINTF("g_strMsg.ucClientportLen=%d,g_strMsg.aucClientport=%c\n",strMsg->ucClientportLen,strMsg->aucClientport[strMsg->timeused][0]);
	if(i < MIN_CLIENT_PORT_BIT) {
		return ERR_CLIENT_MSG_PORT_TOO_SHORT;
	} else {
		return TRUE;
	}
}


//*******************************************************************************************************************
/*响应rtsp客户端*/
//*******************************************************************************************************************
static int ResponseOption(mid_plat_socket sock, STR_CLIENT_MSG *strMsg)
{
	int iSendDataLen = 0;
	int i = 0;
	char  pSend[TCP_BUFF_LEN] = {0};
	sprintf(pSend, "RTSP/1.0 200 OK\r\nCseq: ");
	iSendDataLen = strlen(pSend);

	for(i = 0; i < strMsg->ucCSeqSrcLen; i++) {
		pSend[iSendDataLen + i] = strMsg->aucCSeqSrc[i];
	}

	strcpy(&pSend[iSendDataLen + i], "\r\nPublic: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE,GET_PARAMETER,SET_PARAMETER\r\n\r\n");
	iSendDataLen = strlen(pSend);
	send(sock, pSend, iSendDataLen, 0);
	PRINTF("\n%s\n", pSend);
	return iSendDataLen;
}

static int ResponseGet_parameter(mid_plat_socket sock, STR_CLIENT_MSG *strMsg)
{
	int iSendDataLen = 0;
	int i = 0;
	char  pSend[TCP_BUFF_LEN] = {0};
	sprintf(pSend, "RTSP/1.0 200 OK\r\nCseq: ");
	iSendDataLen = strlen(pSend);

	for(i = 0; i < strMsg->ucCSeqSrcLen; i++) {
		pSend[iSendDataLen + i] = strMsg->aucCSeqSrc[i];
	}

	strcpy(&pSend[iSendDataLen + i], "\r\n\r\n");
	iSendDataLen = strlen(pSend);
	send(sock, pSend, iSendDataLen, 0);
	PRINTF("\n%s\n", pSend);
	return iSendDataLen;
}

static int ResponseSet_parameter(mid_plat_socket sock, STR_CLIENT_MSG *strMsg)
{
	int iSendDataLen = 0;
	int i = 0;
	char  pSend[TCP_BUFF_LEN] = {0};
	sprintf(pSend, "RTSP/1.0 200 OK\r\nCseq: ");
	iSendDataLen = strlen(pSend);

	for(i = 0; i < strMsg->ucCSeqSrcLen; i++) {
		pSend[iSendDataLen + i] = strMsg->aucCSeqSrc[i];
	}

	strcpy(&pSend[iSendDataLen + i], "\r\n\r\n");
	iSendDataLen = strlen(pSend);
	send(sock, pSend, iSendDataLen, 0);
	PRINTF("\n%s\n", pSend);
	return iSendDataLen;
}

#if 0
//describe authentication
static int rtsp_describe_authentication(char *buff, int len, char *localip, int *qtflag, int *roomid)
{

	if(buff == NULL || len < strlen("DESCRIBE RTSP/1.0")) {
		return -1;
	}

	char user[128] = {0};
	char passwd[128] = {0};
	char url[1024] = {0};
	int url_len = 0;
	char *temp = NULL;

	char local_ip[16] = {0};
	char url2[1024] = {0};
	int id = -1;
	char room[1024] = {0};

	if(strstr(buff, "QuickTime") != NULL) {
		*qtflag = QT_CLIENT;
	} else if(strstr(buff, "VLC") != NULL) {
		*qtflag = VLC_CLIENT;
	} else {
		*qtflag = VLC_CLIENT;
	}

	if(strncmp(buff, "DESCRIBE ", 9) != 0) {
		PRINTF("ERROR!!!rtsp_describe_authentication 1.\n");
		return -1;
	}

	//关闭授权
	//	return 0;

	temp = strstr(buff + 9, " RTSP/");

	if(temp == NULL) {
		PRINTF("ERROR!!!rtsp_describe_authentication 2.\n");
		return -1;
	}

	url_len = temp - (buff + 9) ;

	if(url_len < strlen("rtsp://0.0.0.0")) {
		PRINTF("ERROR!!!rtsp_describe_authentication 3.\n");
		return -1;
	}

	//	PRINTF("url_len = %d\n",url_len);
	strncpy(url, buff + 9, url_len);

#if 1
	strcpy(local_ip, localip);
	sprintf(url2, "rtsp://%s:554/card", local_ip);
#else
	strcpy(url2, url);
	temp = strstr(url2, "/card");

	if(temp == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	*(temp + strlen("/card")) = '\0';
#endif

	//	PRINTF("client describe url is %s,the must url is %s.\n",url,url2);
	if(strncmp(url, url2, strlen(url2)) != 0) {
		PRINTF("ERROR!!!rtsp_describe_authentication 4.\n");
		return -1;
	}

	if(*(url + strlen(url2) + 1) != '?') {
		PRINTF("ERROR!!!rtsp_describe_authentication 5.\n");
		return -1;
	}

	//snprintf(room,2,"%s",url+strlen(url2) );
	memcpy(room, (url + strlen(url2)), 1);
	id = atoi(room);

	//	PRINTF("=%s=,room=#%s#,roomid = %d\n",url+strlen(url2) ,room,id);
	if(id < 0 || id > rtsp_stream_get_room_num() - 1) {
		PRINTF("ERROR!!!rtsp_describe_authentication 6.\n");
		return -1;
	}

	*roomid = id;

	if((temp = strchr(url + strlen(url2) + 2, '=')) == NULL) {
		return -1;
	}

	strcpy(passwd, temp + 1);
	*temp = '\0';
	strcpy(user, url + strlen(url2) + 2);
	//	PRINTF("user =$%s$,pass=$%s$\n",user,passwd);
	return app_authentication(user, passwd);
}
#else
//describe authentication
static int rtsp_describe_authentication(char *buff, int len, char *localip, int *qtflag, int *roomid, char *filename, int *need_timeout)
{


	if(buff == NULL || len < strlen("DESCRIBE RTSP/1.0")) {
		return -1;
	}

	char user[128] = {0};
	//	char passwd[128] = {0};
	char url[1024] = {0};
	int url_len = 0;
	char *temp = NULL;

	char local_ip[16] = {0};
	char url2[1024] = {0};
	int id = -1;
	char room[1024] = {0};
	int ret = 0;

	if(strstr(buff, "QuickTime") != NULL) {
		*qtflag = QT_CLIENT;
	} else if(strstr(buff, "VLC") != NULL) {
		*qtflag = VLC_CLIENT;
	} else {
		*qtflag = VLC_CLIENT;
	}

	//just for the stb
	if(strstr(buff, ".ts") != NULL) {
		*qtflag = STB_TS_CLIENT;
	}

	//just for client timeout
	if(strstr(buff, "timeout") != NULL) {
		*need_timeout = 1;
	}

	PRINTF("*need_timeout = %d\n", *need_timeout);

	if(strncmp(buff, "DESCRIBE ", 9) != 0) {
		PRINTF("ERROR!!!rtsp_describe_authentication 1.\n");
		return -1;
	}

	//关闭授权
#ifndef GDM_RTSP_SERVER

	if(1) {
		*roomid = 0;
		return 0;
	}

#endif
	temp = strstr(buff + 9, " RTSP/");

	if(temp == NULL) {
		PRINTF("ERROR!!!rtsp_describe_authentication 2.\n");
		return -1;
	}

	url_len = temp - (buff + 9) ;

	if(url_len < strlen("rtsp://0.0.0.0")) {
		PRINTF("ERROR!!!rtsp_describe_authentication 3.\n");
		return -1;
	}

	//	PRINTF("url_len = %d\n",url_len);
	strncpy(url, buff + 9, url_len);

#if 1
	strcpy(local_ip, localip);
	sprintf(url2, "rtsp://%s:554/card", local_ip);
#else
	strcpy(url2, url);
	temp = strstr(url2, "/card");

	if(temp == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	*(temp + strlen("/card")) = '\0';
#endif

	//	PRINTF("client describe url is %s,the must url is %s.\n",url,url2);
	if(strncmp(url, url2, strlen(url2)) != 0) {
		PRINTF("ERROR!!!rtsp_describe_authentication 4.\n");
		return -1;
	}

	if(*(url + strlen(url2) + 1) != '?') {
		PRINTF("ERROR!!!rtsp_describe_authentication 5.\n");
		return -1;
	}

	//snprintf(room,2,"%s",url+strlen(url2) );
	memcpy(room, (url + strlen(url2)), 1);
	id = atoi(room);

	PRINTF("=%s=,room=#%s#,roomid = %d\n", url + strlen(url2) , room, id);

	if(id < 0 || id > rtsp_stream_get_room_num() - 1) {
		PRINTF("ERROR!!!rtsp_describe_authentication 6.\n");
		return -1;
	}

	*roomid = id;


	//	if((temp =strchr(url+strlen(url2) + 2,' ')) == NULL)
	//	{
	//		return -1;
	//	}
	//	strcpy(passwd,temp+1);
	//	*temp = '\0';
	strcpy(user, url + strlen(url2) + 2);
	PRINTF("user =$%s$\n", user);

#ifdef GDM_RTSP_SERVER
	ret =  app_rtsp_authentication_begin(user);

	if(ret  == 0) {
		strcpy(filename, user);
	}

#endif

	return ret ;
}
#endif

//*******************************************************************************************************************
/*向rtsp客户端发送流信息*/
//*******************************************************************************************************************
#define VIDEO_TRACKID 1
#define AUDIO_TRACKID 2
static int ResponseDescribe(mid_plat_socket socket, mid_plat_sockaddr_in Listen_addr, char *localip , int authencation, STR_CLIENT_MSG *strMsg)
{
	int                     	iSendDataLen = 0, iSendDataLen2 = 0, i = 0;
	int                     	usStringLen = 0;
	int                     	listen_port;
	int                     	audio_sample = 48000;
	char                    	pSend2[1000];
	char                    	local_ip[16] = {0};
	char                    	client_ip[16];
	char                  	pSend[TCP_BUFF_LEN] = {0};
	int 				roomid = strMsg->roomid;
	int 				sdpinfo_len = 0;
	char 			sdpinfo[1024] = {0};
	int				ret = 0;
	char				audio_config[16] = {0};
	int 				a_ret = 0;
	PRINTF("\n");
	a_ret = rtsp_stream_get_audio_sinfo(0, audio_config, &audio_sample);

	if(a_ret < 0) {
		PRINTF("Error\n");
	}

	int mult = 0;
	char ip[32] = {0};
	int vport = 0;
	int aport = 0;
	rtsp_porting_get_ginfo(&mult, ip, &vport, &aport);

	if(mult == 0) {
		strcpy(ip, local_ip);
	}

	sdpinfo_len =  rtsp_stream_get_video_sdp_info(sdpinfo, roomid);
#ifndef GDM_RTSP_SERVER

	if(rtsp_porting_server_need_stop() == 1) {
		sdpinfo_len = -1;
	}

#endif

	if(authencation == -1 || sdpinfo_len == -1) {
		sprintf(pSend, "RTSP/1.0   404   Stream Not Found\r\nCSeq: %d\r\n\r\n", strMsg->uiCseq);
		iSendDataLen = strlen(pSend);
		send(socket, pSend, iSendDataLen, 0);
		PRINTF("\n%s\n", pSend);
		return -1;
	}

	sprintf(local_ip, "%s", localip);
	memcpy(client_ip, inet_ntoa(Listen_addr.sin_addr), 16);
	listen_port = RTSP_LISTEN_PORT;
	sprintf(pSend2, "v=0\r\no=- 2890844256 2890842807 IN IP4 %s\r\nc=IN IP4 %s\r\nt=0 0 \r\n", local_ip, ip); //inet_ntoa(Listen_addr.sin_addr));
	iSendDataLen2 = strlen(pSend2);

	sprintf(&pSend2[iSendDataLen2], "m=video %u RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\n", vport);
	iSendDataLen2 = strlen(pSend2);

	if(sdpinfo_len != -1) {
		sprintf(&pSend2[iSendDataLen2], "a=fmtp:96 packetization-mode=1;profile-level-id=428028;sprop-parameter-sets=%s\r\n", sdpinfo);
		iSendDataLen2 = strlen(pSend2);
	}

	sprintf(&pSend2[iSendDataLen2], "a=control:rtsp://%s:%d/trackID=%d\r\n", local_ip, listen_port, VIDEO_TRACKID);
	iSendDataLen2 = strlen(pSend2);

	//only the card0 have audio
	if(roomid == 0) {
		sprintf(&pSend2[iSendDataLen2], "m=audio %u RTP/AVP 96\r\na=rtpmap:96 mpeg4-generic/%d/2\r\n", aport, audio_sample);
		iSendDataLen2 = strlen(pSend2);

		//ret = rtsp_stream_get_audio_sdp_info(audio_config);
#if 1

		if(a_ret < 0) {
			PRINTF("ERROR,get audio sdp failed .\n");
		} else {
			sprintf(&pSend2[iSendDataLen2], "a=fmtp:96 streamtype=5; mode=AAC-hbr; config=%s; SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1\r\na=control:rtsp://%s:%d/trackID=%d\r\n", audio_config, local_ip, listen_port, AUDIO_TRACKID);
			iSendDataLen2 = strlen(pSend2);
		}

#endif
	}

	strcpy(pSend, "RTSP/1.0 200 OK\r\nContent-type: application/sdp\r\nServer: RRS 0.1\r\nContent-Length: ");
	iSendDataLen = strlen(pSend);
	usStringLen = ConventNumberToString(&pSend[iSendDataLen], iSendDataLen2);
	strcpy(&pSend[iSendDataLen + usStringLen], "\r\nCache-Control: no-cache\r\nCseq: ");
	iSendDataLen = strlen(pSend);

	for(i = 0; i < strMsg->ucCSeqSrcLen; i++) {
		pSend[iSendDataLen + i] = strMsg->aucCSeqSrc[i];
	}

	strcpy(&pSend[iSendDataLen + i], "\r\n\r\n");
	iSendDataLen = strlen(pSend);
	send(socket, pSend, iSendDataLen, 0);
	PRINTF("\n%s", pSend);
	send(socket, pSend2, iSendDataLen2, 0);
	PRINTF("\n%s\n", pSend2);
	return (iSendDataLen + iSendDataLen2);

}


static int ResponseDescribeTs(mid_plat_socket socket, mid_plat_sockaddr_in Listen_addr, char *localip , int authencation, STR_CLIENT_MSG *strMsg)
{
	int                     	iSendDataLen = 0, iSendDataLen2 = 0, i = 0;
	int                     	usStringLen = 0;
	int                     	listen_port;
	//int                     	audio_sample = rtsp_stream_get_audio_samplerate();
	char                    	pSend2[1000];
	char                    	local_ip[16] = {0};
	char                    	client_ip[16];
	char                  	pSend[TCP_BUFF_LEN] = {0};
	int 				roomid = strMsg->roomid;
	int 				sdpinfo_len = 0;
	char 			sdpinfo[1024] = {0};
	//	int				ret = 0;

	sdpinfo_len =  rtsp_stream_get_video_sdp_info(sdpinfo, roomid);
#ifndef GDM_RTSP_SERVER

	if(rtsp_porting_server_need_stop() == 1) {
		sdpinfo_len = -1;
	}

#endif

	if(authencation == -1 || sdpinfo_len == -1) {
		sprintf(pSend, "RTSP/1.0   404   Stream Not Found\r\nCSeq: %d\r\n\r\n", strMsg->uiCseq);
		iSendDataLen = strlen(pSend);
		send(socket, pSend, iSendDataLen, 0);
		PRINTF("\n%s\n", pSend);
		return -1;
	}

	sprintf(local_ip, "%s", localip);
	memcpy(client_ip, inet_ntoa(Listen_addr.sin_addr), 16);
	listen_port = RTSP_LISTEN_PORT;
	sprintf(pSend2, "v=0\r\no=- %u %u IN IP4 %s\r\n", getCurrentTimets(), getCurrentTimets(), local_ip);
	iSendDataLen2 = strlen(pSend2);

	sprintf(&pSend2[iSendDataLen2], "s=MPEG Transport Stream, streamed by the LIVE555 Media Server\r\ni=3.ts\r\nt=0 0\r\na=tool:LIVE555 Streaming Media v2011.01.19\r\n");
	iSendDataLen2 = strlen(pSend2);
	sprintf(&pSend2[iSendDataLen2], "a=type:broadcast\r\na=control:* \r\na=range:npt=0-\r\n");
	iSendDataLen2 = strlen(pSend2);

	sprintf(&pSend2[iSendDataLen2], "a=x-qt-text-nam:MPEG Transport Stream, streamed by the LIVE555 Media Server\r\na=x-qt-text-inf:3.ts\r\nm=video 0 RTP/AVP 33\r\nc=IN IP4 0.0.0.0\r\nb=AS:5000\r\na=control:track%d\r\n", VIDEO_TRACKID);
	//	sprintf(&pSend2[iSendDataLen2],"a=x-qt-text-nam:MPEG Transport Stream, streamed by the LIVE555 Media Server \r\na=x-qt-text-inf:3.ts \r\nm=video 0 RTP/AVP 33 \r\nc=IN IP4 0.0.0.0 \r\nb=AS:5000 \r\na=control:rtsp://%s:%d/trackID=%d \r\n",local_ip,listen_port,VIDEO_TRACKID);
	iSendDataLen2 = strlen(pSend2);





	strcpy(pSend, "RTSP/1.0 200 OK\r\nCseq: ");
	iSendDataLen = strlen(pSend);

	for(i = 0; i < strMsg->ucCSeqSrcLen; i++) {
		pSend[iSendDataLen + i] = strMsg->aucCSeqSrc[i];
	}

	iSendDataLen = strlen(pSend);
	strcpy(&pSend[iSendDataLen], "\r\nContent-Type: application/sdp\r\nServer: RRS 0.1\r\nCache-Control: no-cache\r\nContent-Length: ");
	iSendDataLen = strlen(pSend);
	usStringLen = ConventNumberToString(&pSend[iSendDataLen], iSendDataLen2);
	strcpy(&pSend[iSendDataLen + usStringLen], "\r\n");
	iSendDataLen = strlen(pSend);
	strcpy(&pSend[iSendDataLen], "\r\n");
	iSendDataLen = strlen(pSend);
	send(socket, pSend, iSendDataLen, 0);
	PRINTF("\n%s", pSend);
	send(socket, pSend2, iSendDataLen2, 0);
	PRINTF("\n%s\n", pSend2);
	return (iSendDataLen + iSendDataLen2);

}




//*******************************************************************************************************************
/*建立响应*/
//*******************************************************************************************************************
static int rtsp_find_trackid(char *buff, int len)
{
	if(buff  == NULL) {
		return -1;
	}

	char *p = NULL;
	char tempbuff[1024] = {0};
	int id = 0;
	sprintf(tempbuff, "%s", buff);
	p = strstr(tempbuff, "/trackID=");
	PRINTF("\n");

	if(p == NULL) {
		return -1;
	}

	p = p + strlen("/trackID=") ;
	*(p + 1) = '\0';
	id = atoi(p);
	PRINTF("i find the trackid = %d\n", id);
	return id;
}

static int ResponseSetup(char *pReceiveBuf, int ReceiveBufLen, mid_plat_socket sock, STR_CLIENT_MSG *strMsg, int timeout)
{
	int iSendDataLen = 0;
	unsigned int i = 0;
	char  pSend[TCP_BUFF_LEN] = {0};
	int is_video = 1;
	int id = 0;
	id = rtsp_find_trackid(pReceiveBuf, ReceiveBufLen);

	if(id == VIDEO_TRACKID) {
		is_video = 1;
	} else if(id == AUDIO_TRACKID) {
		is_video = 2;
	} else {
		is_video = 1;
	}

	PraseSETUPMsg(pReceiveBuf, ReceiveBufLen, strMsg);
	GetClientPort(pReceiveBuf, ReceiveBufLen, strMsg, is_video - 1);

	strcpy(pSend, "RTSP/1.0 200 OK\r\nServer: RRS 0.1\r\nTransport: ");
	iSendDataLen = strlen(pSend);

	for(i = 0; i < strMsg->ucTransNum; i++) {
		pSend[iSendDataLen + i] = strMsg->aucTrans[i];
	}

	for(i = 0; i < strMsg->ucClientportLen[strMsg->timeused]; i++) {
		pSend[iSendDataLen + strMsg->ucTransNum + i ] = strMsg->aucClientport[strMsg->timeused][i];
	}

	//need client timeout
	if(timeout != 0) {
		sprintf(&pSend[iSendDataLen + strMsg->ucTransNum + strMsg->ucClientportLen[strMsg->timeused]], "\r\nContent-Length: 0\r\nCache-Control: no-cache\r\nSession: 11478; timeout=%d\r\nCseq: ", timeout);
	} else {
		strcpy(&pSend[iSendDataLen + strMsg->ucTransNum + strMsg->ucClientportLen[strMsg->timeused]], "\r\nContent-Length: 0\r\nCache-Control: no-cache\r\nSession: 11478\r\nCseq: ");
	}

	iSendDataLen = strlen(pSend);

	for(i = 0; i < strMsg->ucCSeqSrcLen; i++) {
		pSend[iSendDataLen + i] = strMsg->aucCSeqSrc[i];
	}

	strcpy(&pSend[iSendDataLen + i], "\r\n\r\n");
	iSendDataLen = strlen(pSend);
	send(sock, pSend, iSendDataLen, 0);
	PRINTF("\n%s\n", pSend);
	strMsg->timeused = strMsg->timeused + 1;

	if(strMsg->timeused == 2) {
		strMsg->timeused = 0;
	}

	return iSendDataLen;
}


//*******************************************************************************************************************
/*响应播放*/
//*******************************************************************************************************************
static int ResponsePlay(mid_plat_socket sock, STR_CLIENT_MSG *strMsg)
{
	int iSendDataLen = 0;
	unsigned int i = 0;
	char  pSend[TCP_BUFF_LEN] = {0};
	strcpy(pSend, "RTSP/1.0 200 OK\r\nServer: RRS 0.1\r\nSession: 11478\r\nRange: npt=0.000- \r\n");
	iSendDataLen = strlen(pSend);

	sprintf(&pSend[iSendDataLen], "Cseq: ");
	iSendDataLen = strlen(pSend);

	for(i = 0; i < strMsg->ucCSeqSrcLen; i++) {
		pSend[iSendDataLen + i] = strMsg->aucCSeqSrc[i];
	}

	strcpy(&pSend[iSendDataLen + i], "\r\n\r\n");
	iSendDataLen = strlen(pSend);
	send(sock, pSend, iSendDataLen, 0);
	PRINTF("\n%s\n", pSend);

	return iSendDataLen;
}



//*******************************************************************************************************************
/*响应 pause*/
//*******************************************************************************************************************
//pause
static int RtspPauseClient(int nPos)
{
	mid_plat_socket socket = -1;
	SETRTSPUDPREADY(0, nPos, 0);
	//	SETRTSPROOMID(0, nPos, -1);
	//	SETRTSPUSED(0, nPos, FALSE);
	socket = GETRTSPUDPAUDIOSOCK(0, nPos);
	mid_plat_close_socket(socket);
	SETRTSPUDPAUDIOSOCK(0, nPos, -1);
	socket = GETRTSPUDPVIDEOSOCK(0, nPos);
	mid_plat_close_socket(socket);

	SETRTSPUDPVIDEOSOCK(0, nPos, -1);

	char *client = NULL;
	client = GETRTSPCLIENT(0, nPos);
	rtsp_porting_close_client(nPos,client);
	client = NULL;
	SETRTSPCLIENT(0, nPos, client);
	//	SETRTSPCLIENTTYPE(0,nPos,VLC_CLIENT);
	return 0;
}

int ResponsePause(mid_plat_socket sock, STR_CLIENT_MSG *strMsg)
{
	int iSendDataLen = 0;
	unsigned int i = 0;
	char  pSend[TCP_BUFF_LEN] = {0};
	strcpy(pSend, "RTSP/1.0 200 OK\r\nServer: RRS 0.1\r\nSession: 11478\r\n");
	iSendDataLen = strlen(pSend);

	sprintf(&pSend[iSendDataLen], "Cseq: ");
	iSendDataLen = strlen(pSend);

	for(i = 0; i < strMsg->ucCSeqSrcLen; i++) {
		pSend[iSendDataLen + i] = strMsg->aucCSeqSrc[i];
	}

	strcpy(&pSend[iSendDataLen + i], "\r\n\r\n");
	iSendDataLen = strlen(pSend);
	send(sock, pSend, iSendDataLen, 0);
	PRINTF("\n%s\n", pSend);
	return iSendDataLen;
}


static int SendAnnounce(int socket, mid_plat_sockaddr_in Listen_addr, char *localip, STR_CLIENT_MSG *strMsg)
{
	return 0;
	//    	struct in_addr          addr_ip;
	//    	int                     iSendDataLen=0,iSendDataLen2=0,i=0;
	////	int                     usStringLen=0;
	//    	int                     listen_port;
	//    	int                     audio_sample=48000;
	//	char                    pSend2[1000]= {0};
	//	char				pSend3[1024] = {0};
	//    	char                    local_ip[16];
	//    	char                    client_ip[16];
	////	char                  pSend[TCP_BUFF_LEN]={0};
	//
	//	int sdpinfo_len = 0;
	//	char sdpinfo[1024] = {0};
	//  	//sdpinfo_len = rtsp_porting_get_video_sdp_info(sdpinfo);
	// 	 sdpinfo_len =  rtsp_stream_get_video_sdp_info(sdpinfo,0);
	//    	memcpy(&addr_ip,&localipaddr,4);
	//    	memcpy(local_ip,inet_ntoa(addr_ip),16);
	//    	memcpy(client_ip,inet_ntoa(Listen_addr.sin_addr),16);
	//    	listen_port = RTSP_LISTEN_PORT;
	//   	 sprintf(pSend2,"v=0\r\no=- 2890844256 2890842807 IN IP4 %s\r\nc=IN IP4 %s\r\nt=0 0 \r\n",local_ip,inet_ntoa(Listen_addr.sin_addr));
	//   	 iSendDataLen2=strlen(pSend2);
	//
	//    	sprintf(&pSend2[iSendDataLen2],"m=video 0 RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\n");
	//	iSendDataLen2=strlen(pSend2);
	//	if(sdpinfo_len != -1)
	//	{
	//		sprintf(&pSend2[iSendDataLen2],"a=fmtp:96 packetization-mode=1;profile-level-id=428028;sprop-parameter-sets=%s \r\n",sdpinfo);
	//		iSendDataLen2=strlen(pSend2);
	//	}
	//	sprintf(&pSend2[iSendDataLen2],"a=control:rtsp://%s:%d/trackID=1 \r\n",local_ip,listen_port);
	//	iSendDataLen2=strlen(pSend2);
	//
	//	sprintf(&pSend2[iSendDataLen2],"m=audio 0 RTP/AVP 96\r\na=rtpmap:96 mpeg4-generic/%d/2\r\n",audio_sample);
	//	iSendDataLen2=strlen(pSend2);
	//	sprintf(&pSend2[iSendDataLen2],"a=fmtp:96 streamtype=5; mode=AAC-hbr; config=1310; SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=1\r\na=control:rtsp://%s:%d/trackID=2 \r\n",local_ip,listen_port);
	//	iSendDataLen2=strlen(pSend2);
	//	strMsg->uiCseq++;
	//	sprintf(pSend3,"ANNOUNCE rtsp://192.168.7.48:554 RTSP/1.0\r\nContent-Type: application/sdp\r\nCSeq: %d",strMsg->uiCseq);
	//	iSendDataLen=strlen(pSend3);
	////	for(i=0;i<g_strMsg.ucCSeqSrcLen;i++)
	////	{
	////		pSend3[iSendDataLen+i]=g_strMsg.aucCSeqSrc[i];
	////	}
	//	strcpy(&pSend3[iSendDataLen+i],"\r\n\r\n");
	//
	//	iSendDataLen=strlen(pSend3);
	//	send(socket,pSend3,iSendDataLen,0);
	//	PRINTF("\n%s\n",pSend3);
	//
	//	send(socket,pSend2,iSendDataLen2,0);
	//	PRINTF("\n%s\n",pSend2);
	//	return (iSendDataLen+iSendDataLen2);

}


//*************************************************************************************************
/*退出*/
//*************************************************************************************************/
static int ResponseTeardown(mid_plat_socket sock, STR_CLIENT_MSG *strMsg)
{
	int iSendDataLen = 0;
	unsigned int i = 0;
	char  pSend[TCP_BUFF_LEN] = {0};
	strcpy(pSend, "RTSP/1.0 200 OK\r\nServer: RRS 0.1\r\n");
	iSendDataLen = strlen(pSend);


	sprintf(&pSend[iSendDataLen], "Cseq: ");
	iSendDataLen = strlen(pSend);

	for(i = 0; i < strMsg->ucCSeqSrcLen; i++) {
		pSend[iSendDataLen + i] = strMsg->aucCSeqSrc[i];
	}




	strcpy(&pSend[iSendDataLen + i], "\r\n\r\n");
	iSendDataLen = strlen(pSend);
	send(sock, pSend, iSendDataLen, 0);
	PRINTF("\n%s\n", pSend);
	return iSendDataLen;
}


/**************************************************************************************************
                    获取rtsp port
**************************************************************************************************/
static unsigned short g_rtsp_port = RTSP_LISTEN_PORT;
int RtspSetPort(unsigned short port)
{
	g_rtsp_port = port;
	return 0;
}
static int RtspGetPort()
{
	int RtspPort;
	RtspPort = g_rtsp_port;
	return (RtspPort);
}
//**************************************************************************
/*delete rtsp client*/
//**************************************************************************
static int RtspDelClient(int nPos)
{
	mid_plat_socket socket = -1;
	mid_plat_sockaddr_in temp_addr ;
	char *client = NULL;
	memset(&temp_addr, 0, sizeof(mid_plat_sockaddr_in));

	SETRTSPUDPREADY(0, nPos, 0);
	SETRTSPROOMID(0, nPos, -1);
	SETRTSPUSED(0, nPos, FALSE);
	socket = GETRTSPUDPAUDIOSOCK(0, nPos);
	mid_plat_close_socket(socket);
	SETRTSPUDPAUDIOSOCK(0, nPos, -1);
	socket = GETRTSPUDPVIDEOSOCK(0, nPos);
	mid_plat_close_socket(socket);

	SETRTSPUDPAUDIOADDR(0, nPos, temp_addr);
	SETRTSPUDPVIDEOADDR(0, nPos, temp_addr);
	SETRTSPADDR(0, nPos, temp_addr);

	SETRTSPUDPVIDEOSOCK(0, nPos, -1);
	SETRTSPCLIENTTYPE(0, nPos, VLC_CLIENT);

	client = GETRTSPCLIENT(0, nPos);
	rtsp_porting_close_client(client);
	client = NULL;
	SETRTSPCLIENT(0, nPos, client);

	return 0;
}

/*
*切换长宽后，需要关闭QT
*/
static void rtsp_change_video_resolve(int roomid)
{
	int cli  = 0;

	//	PRINTF("g_rtsp_flag=%d\n",g_rtsp_flag);
	if(g_rtsp_flag != 1) {
		PRINTF("warnning ,the rtsp have no client .\n");
		return ;
	}

	for(cli = 0; cli < MAX_RTSP_CLIENT_NUM; cli++) {
		//vlc 也关闭
		//if((roomid == GETRTSPROOMID(0,cli)) && (GETRTSPCLIENTTYPE(0, cli) == QT_CLIENT))
		if(roomid == GETRTSPROOMID(0, cli)) {
			SETRTSPCLIENTTYPE(0, cli, INVALID_CLIENT);
			PRINTF("the video resolve is change ,the %d client is invalid,the room is %d.\n", cli, roomid);
		}
	}

	return ;
}






/***************************************************************************************************
                                        发送 RTSP video数据
****************************************************************************************************/
//type=0 : rtp+264  type=1 : rtp+ts
static int SendRtspVideoData(char *pData, int length, int type, int roomid)
{
	return 0;
	int                     cnt, nRet;
	mid_plat_socket                     sendsocket;
	mid_plat_sockaddr_in      sendaddr;

	//PRINTF("the length = %d\n",length);
	if(type == 0) {
		for(cnt = 0; cnt < MAX_RTSP_CLIENT_NUM; cnt++) {
			//        pthread_mutex_lock(&send_rtp_data);
			if((roomid == GETRTSPROOMID(0, cnt)) && (GETRTSPUDPREADY(0, cnt) == 2) && (GETRTSPCLIENTTYPE(0, cnt) == VLC_CLIENT || GETRTSPCLIENTTYPE(0, cnt) == QT_CLIENT)) {
				//	if((type == 7 || type == 8) && (GETRTSPCLIENTTYPE(0, cnt) == QT_CLIENT))
				if(0) {
					;//PRINTF("QT didn't need the pps/ssp\n");
				} else {
					sendsocket = GETRTSPUDPVIDEOSOCK(0, cnt);
					sendaddr = GETRTSPUDPVIDEOADDR(0, cnt);
					nRet = sendto(sendsocket, pData, length, 0, \
					              (struct sockaddr *) &sendaddr, sizeof(mid_plat_sockaddr_in));

					if(nRet < length) {
						PRINTF("rtsp video: lost video data\n");
					}

					if(nRet <= 0) {
						PRINTF("sendto video data failed !!, errno = %d, err msg = %s\n", errno, strerror(errno));

					}
				}
			}

			//        pthread_mutex_unlock(&send_rtp_data);
		}
	} else if(type == 1) {
		for(cnt = 0; cnt < MAX_RTSP_CLIENT_NUM; cnt++) {
			//        pthread_mutex_lock(&send_rtp_data);
			if((roomid == GETRTSPROOMID(0, cnt)) && (GETRTSPUDPREADY(0, cnt) == 2) && (GETRTSPCLIENTTYPE(0, cnt) == STB_TS_CLIENT)) {
				//	if((type == 7 || type == 8) && (GETRTSPCLIENTTYPE(0, cnt) == QT_CLIENT))
				if(0) {
					;//PRINTF("QT didn't need the pps/ssp\n");
				} else {
					//		PRINTF("---len =%d\n",length);
					sendsocket = GETRTSPUDPVIDEOSOCK(0, cnt);
					sendaddr = GETRTSPUDPVIDEOADDR(0, cnt);
					nRet = sendto(sendsocket, pData, length, 0, \
					              (struct sockaddr *) &sendaddr, sizeof(mid_plat_sockaddr_in));

					if(nRet < length) {
						PRINTF("rtsp video: lost video data\n");
					}

					if(nRet <= 0) {
						PRINTF("sendto video data failed !!, errno = %d, err msg = %s\n", errno, strerror(errno));

					}
				}
			}

			//        pthread_mutex_unlock(&send_rtp_data);
		}
	}

	return 0;
}

/*********************************************************************************************************
                                        发送 RTSP audio数据
*********************************************************************************************************/
static int SendRtspAudioData(unsigned char *pData, int length, int roomid)
{
	return 0;
	int                     cnt, nRet;
	mid_plat_socket                     sendsocket;
	mid_plat_sockaddr_in      sendaddr;

	//PRINTF("roomid = %d\n",roomid);
	for(cnt = 0; cnt < MAX_RTSP_CLIENT_NUM; cnt++) {
		if((roomid == GETRTSPROOMID(0, cnt)) && (GETRTSPUDPREADY(0, cnt) == 2)  && (GETRTSPCLIENTTYPE(0, cnt) == VLC_CLIENT || GETRTSPCLIENTTYPE(0, cnt) == QT_CLIENT)) {
			sendsocket = GETRTSPUDPAUDIOSOCK(0, cnt);

			if(sendsocket <= 0) {
				continue;
			}

			sendaddr = GETRTSPUDPAUDIOADDR(0, cnt);
			nRet = sendto(sendsocket, (const char *)pData, length, 0, \
			              (struct sockaddr *) &sendaddr, sizeof(mid_plat_sockaddr_in));

			if(nRet < length) {
				PRINTF("rtsp audio: lost audio data\n");
			}

			if(nRet <= 0) {
				PRINTF("sendto audio data failed !!, errno = %d, err msg = %s\n", errno, strerror(errno));
			}
		}
	}

	return 0;
}
/*********************************************************************************************************
                                        RTSP video打包函数
**********************************************************************************************************/


static int SendRtpNalu(NALU_t *nalu , unsigned short *seq_num, unsigned long ts_current , int roomid, int end)
{
	char				 sendbuf[1500] = {0};
	RTP_FIXED_HEADER			*rtp_hdr;
	char                        *nalu_payload;
	FU_INDICATOR	            *fu_ind;
	FU_HEADER		            *fu_hdr;
	int                          bytes = 0;

	int total_len = 0;
	memset(sendbuf, 0, 20);
	//设置RTP HEADER，
	rtp_hdr 							= (RTP_FIXED_HEADER *)&sendbuf[0];
	rtp_hdr->payload	                = H264;
	rtp_hdr->version	                = 2;
	rtp_hdr->marker                     = 0;
	rtp_hdr->ssrc		                = htonl(10);
	rtp_hdr->timestamp		            = htonl(ts_current);

	if((nalu->len - 1) <= RTP_PAYLOAD_LEN) {
		//设置rtp M 位；

		rtp_hdr->marker = 1;
		rtp_hdr->seq_no 	= htons((*seq_num)++); //序列号，每发送一个RTP包增1
		memcpy(&sendbuf[12], nalu->buf, nalu->len);
		bytes = nalu->len + 12 ; 					//获得sendbuf的长度,为nalu的长度（包含NALU头但除去起始前缀）加上rtp_header的固定长度12字节
		//SendRtspVideoData(sendbuf, bytes,nalu->nal_unit_type,roomid);
		SendRtspVideoData(sendbuf, bytes, 0, roomid);

		//if(sendbuf[12] == 0x67 || sendbuf[12] == 0x68)
		//PRINTF("%u,the len=%d,buf[1]=%x=rtp_hdr->marker=%d\n,",ts_current,bytes,sendbuf[12],rtp_hdr->marker);

		total_len = nalu->len;
	} else if((nalu->len - 1) > RTP_PAYLOAD_LEN) {
		//得到该nalu需要用多少长度为1400字节的RTP包来发送
		int k = 0, l = 0;
		int t = 0; //用于指示当前发送的是第几个分片RTP包

		l = (nalu->len - 1) % RTP_PAYLOAD_LEN; //最后一个RTP包的需要装载的字节数

		if(l == 0) {
			k = (nalu->len - 1) / RTP_PAYLOAD_LEN - 1; //需要k个1400字节的RTP包
			l = RTP_PAYLOAD_LEN;
		} else {
			k = (nalu->len - 1) / RTP_PAYLOAD_LEN; //需要k个1400字节的RTP包
		}

		while(t <= k) {
			rtp_hdr->seq_no = htons((*seq_num)++); //序列号，每发送一个RTP包增1

			if(!t) { //发送一个需要分片的NALU的第一个分片，置FU HEADER的S位
				//设置rtp M 位；
				rtp_hdr->marker = 0;
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
				fu_ind = (FU_INDICATOR *)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
				fu_ind->F = nalu->forbidden_bit;
				fu_ind->NRI = nalu->nal_reference_idc >> 5;
				fu_ind->TYPE = 28;

				//设置FU HEADER,并将这个HEADER填入sendbuf[13]
				fu_hdr = (FU_HEADER *)&sendbuf[13];
				fu_hdr->E = 0;
				fu_hdr->R = 0;
				fu_hdr->S = 1;
				fu_hdr->TYPE = nalu->nal_unit_type;

				nalu_payload = &sendbuf[14]; //同理将sendbuf[14]赋给nalu_payload
				memcpy(nalu_payload, nalu->buf + 1, RTP_PAYLOAD_LEN); //去掉NALU头

				//	PRINTF("%x,ser_no=%d\n",nalu_payload[0],*seq_num);
				bytes = RTP_PAYLOAD_LEN + 14;						//获得sendbuf的长度,为nalu的长度（除去起始前缀和NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节
				SendRtspVideoData(sendbuf, bytes, 0, roomid);
				total_len += RTP_PAYLOAD_LEN;
				t++;
			}
			//发送一个需要分片的NALU的非第一个分片，清零FU HEADER的S位，如果该分片是该NALU的最后一个分片，置FU HEADER的E位
			else if(k == t) { //发送的是最后一个分片，注意最后一个分片的长度可能超过1400字节（当l>1386时）。
				//设置rtp M 位；当前传输的是最后一个分片时该位置1
				rtp_hdr->marker = 1;
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
				fu_ind = (FU_INDICATOR *)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
				fu_ind->F = nalu->forbidden_bit;
				fu_ind->NRI = nalu->nal_reference_idc >> 5;
				fu_ind->TYPE = 28;

				//设置FU HEADER,并将这个HEADER填入sendbuf[13]
				fu_hdr = (FU_HEADER *)&sendbuf[13];
				fu_hdr->R = 0;
				fu_hdr->S = 0;
				fu_hdr->TYPE = nalu->nal_unit_type;
				fu_hdr->E = 1;

				nalu_payload = &sendbuf[14]; //同理将sendbuf[14]的地址赋给nalu_payload
				memcpy(nalu_payload, nalu->buf + t * RTP_PAYLOAD_LEN + 1, l); //将nalu最后剩余的l-1(去掉了一个字节的NALU头)字节内容写入sendbuf[14]开始的字符串。
				bytes = l + 14;		//获得sendbuf的长度,为剩余nalu的长度l-1加上rtp_header，FU_INDICATOR,FU_HEADER三个包头共14字节
				//	PRINTF("%x,ser_no=%d\n",nalu_payload[0],*seq_num);
				SendRtspVideoData(sendbuf, bytes, 0, roomid);
				total_len += l;
				t++;
			} else if(t < k && 0 != t) {
				//设置rtp M 位；
				rtp_hdr->marker = 0;
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
				fu_ind = (FU_INDICATOR *)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
				fu_ind->F = nalu->forbidden_bit;
				fu_ind->NRI = nalu->nal_reference_idc >> 5;
				fu_ind->TYPE = 28;

				//设置FU HEADER,并将这个HEADER填入sendbuf[13]
				fu_hdr = (FU_HEADER *)&sendbuf[13];
				fu_hdr->R = 0;
				fu_hdr->S = 0;
				fu_hdr->E = 0;
				fu_hdr->TYPE = nalu->nal_unit_type;

				nalu_payload = &sendbuf[14]; //同理将sendbuf[14]的地址赋给nalu_payload
				memcpy(nalu_payload, nalu->buf + t * RTP_PAYLOAD_LEN + 1, RTP_PAYLOAD_LEN); //去掉起始前缀的nalu剩余内容写入sendbuf[14]开始的字符串。
				bytes = RTP_PAYLOAD_LEN + 14;						//获得sendbuf的长度,为nalu的长度（除去原NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节

				SendRtspVideoData(sendbuf, bytes, 0, roomid);
				total_len += RTP_PAYLOAD_LEN;
				t++;
			}
		}
	}

	//if(total_len+1 != nalu->len )
	//{
	//PRINTF("nalu send len =%d,nalu len= %d\n",total_len,nalu->len);
	//	}
	return 0;
}


//*******************************************************************************************************************
/*add rtsp udp client*/
//*******************************************************************************************************************
#if 0
static int RtspAddClient(mid_plat_sockaddr_in client_addr, int nPos, STR_CLIENT_MSG *strMsg)
{
	int      UdpRtspVideoSocket = 0;
	int      UdpRtspAudioSocket = 0;
	int 	   have_audio_socket = 0;
	int      buf_num;
#ifndef GDM_RTSP_SERVER
	struct timeval tv;
#endif
	mid_plat_sockaddr_in temp_client_addr;
	mid_plat_sockaddr_in temp_video_addr;
	mid_plat_sockaddr_in temp_audio_addr;

	mid_plat_sockaddr_in video_addr;
	mid_plat_sockaddr_in audio_addr;
	unsigned int socklen;
	int m, n;

	int video_port = 0;

	int audio_port = 0;

	int roomid = strMsg->roomid;

	socklen = sizeof(mid_plat_sockaddr_in);
	//	memset(&peeraddr, 0, socklen);
	memset(&video_addr, 0, socklen);
	memset(&audio_addr, 0, socklen);

	for(m = 0; m < strMsg->ucClientportLen[0]; m++) {
		if(strMsg->aucClientport[0][m] == 0x2d) {
			break;
		}
	}

	for(n = 0; n < m; n++) {
		video_port += (strMsg->aucClientport[0][n] - 0x30) * pow(10, (m - n - 1));
	}

	PRINTF("video socket m=%d,port=%d,audio_port = %d\n", m, video_port);

	//must have video port else will return -1;
	if(m == 0 || video_port == 0) {
		PRINTF("ERROR,the video socket port is error,m=%d,video_port= %d\n", m, video_port);
		return -1;
	}

	// bzero(&peeraddr,socklen);
	//memset(&peeraddr,0,socklen);
	video_addr.sin_addr.s_addr = client_addr.sin_addr.s_addr;
	video_addr.sin_port = htons(video_port);
	video_addr.sin_family = AF_INET;


	for(m = 0; m < strMsg->ucClientportLen[1]; m++) {
		if(strMsg->aucClientport[1][m] == 0x2d) {
			break;
		}
	}

	for(n = 0; n < m; n++) {
		audio_port += (strMsg->aucClientport[1][n] - 0x30) * pow(10, (m - n - 1));
	}

	PRINTF("aduio socket m=%d,port=%d\n", m, audio_port);

	if(m == 0 || audio_port == 0) {
		PRINTF("WARNNING,the audio socket port is error,m=%d,audio_port= %d\n", m, audio_port);
		have_audio_socket = -1;
	} else {
		//bzero(&peeraddr,socklen);
		memset(&audio_addr, 0, socklen);
		audio_addr.sin_addr.s_addr = client_addr.sin_addr.s_addr;
		audio_addr.sin_port = htons(audio_port);
		audio_addr.sin_family = AF_INET;
	}


	//compare the all client socket
	for(m = 0; m < MAX_RTSP_CLIENT_NUM ; m++) {
		if((roomid == GETRTSPROOMID(0, m)) && (GETRTSPUDPREADY(0, m) == 2)) {
			temp_client_addr = GETRTSPADDR(0, m);

			if(temp_client_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
				PRINTF("DEBUG,the client %d and %d is the same ip\n", m, nPos);
				temp_video_addr = GETRTSPUDPVIDEOADDR(0, m);
				temp_audio_addr = GETRTSPUDPAUDIOADDR(0, m);

				if(mid_plat_compare_sockaddr_in(&video_addr, &temp_video_addr) == 0
				   || (mid_plat_compare_sockaddr_in(&audio_addr, &temp_audio_addr) == 0
				       && have_audio_socket == 0)) {
					PRINTF("WARNNING,the client %d and %d is have same video/audio socket\n", m, nPos);
					SETRTSPCLIENTTYPE(0, m, INVALID_CLIENT);
					SETRTSPUDPREADY(0, m, 0);
					PRINTF("WARNNING,i will set the % client valid\n", m);
				}
			}
		}
	}

	/* 创建video socket 用于 audio UDP通讯 */
	UdpRtspVideoSocket = socket(AF_INET, SOCK_DGRAM, 0);

	if(UdpRtspVideoSocket < 0) {
		perror("socket creating error\n");
		return -1;
	}

	int opt = 1;
	setsockopt(UdpRtspVideoSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

#ifndef GDM_RTSP_SERVER
	tv.tv_sec = 10;
	tv.tv_usec = 10000;
	setsockopt(UdpRtspVideoSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));

	int tvLen = sizeof(struct timeval);
	int ret = getsockopt(UdpRtspVideoSocket, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, (socklen_t *)&tvLen);

	if(ret < 0) {
		PRINTF("audio getsockopt failed\n");
	}

#endif

	buf_num = 1024 * 1024;
	setsockopt(UdpRtspVideoSocket, SOL_SOCKET, SO_SNDBUF, (const char *)&buf_num, sizeof(buf_num));

	//socklen_t optlen;
	int optlen;
	buf_num = 1;
	optlen = sizeof(buf_num);
	getsockopt(UdpRtspVideoSocket, SOL_SOCKET, SO_SNDBUF, (char *)&buf_num, (socklen_t *)&optlen);
	//    	PRINTF("audio buf_num = %d \n",buf_num);

	SETRTSPUDPVIDEOADDR(0, nPos, video_addr);
	SETRTSPUDPVIDEOSOCK(0, nPos, UdpRtspVideoSocket);


	if(have_audio_socket == 0) {
		/* 创建audio socket 用于 video UDP通讯 */
		UdpRtspAudioSocket = socket(AF_INET, SOCK_DGRAM, 0);

		if(UdpRtspAudioSocket < 0) {
			perror("socket creating error\n");
			return -1;
		}

		opt = 1;
		setsockopt(UdpRtspAudioSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

#ifndef GDM_RTSP_SERVER
		tv.tv_sec = 10;
		tv.tv_usec = 10000;
		setsockopt(UdpRtspAudioSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(struct timeval));
		tvLen = sizeof(struct timeval);
		ret = getsockopt(UdpRtspAudioSocket, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, (socklen_t *)&tvLen);

		if(ret < 0) {
			PRINTF("audio getsockopt failed\n");
		}

		mid_plat_set_socket_stimeout(UdpRtspAudioSocket, 10010);
#endif
#if 1
		buf_num = 1024 * 1024;
		setsockopt(UdpRtspAudioSocket, SOL_SOCKET, SO_SNDBUF, (const char *)&buf_num, sizeof(buf_num));
#endif
		optlen = sizeof(buf_num);
		getsockopt(UdpRtspAudioSocket, SOL_SOCKET, SO_SNDBUF, (char *)&buf_num, (socklen_t *)&optlen);
		//	PRINTF("video send buffer = %d\n",buf_num);

		SETRTSPUDPAUDIOADDR(0, nPos, audio_addr);
		SETRTSPUDPAUDIOSOCK(0, nPos, UdpRtspAudioSocket);
	}


	SETRTSPROOMID(0, nPos, roomid);
	SETRTSPUDPREADY(0, nPos, 1);

	//i will save the socket addr int  static struct;
	SETRTSPADDR(0, nPos, client_addr);


	PRINTF("create rtsp client sock success\n");
	return 0;
}
#else
static int RtspAddClient(mid_plat_sockaddr_in client_addr, int nPos, STR_CLIENT_MSG *strMsg)
{
	//int      UdpRtspVideoSocket = 0;
	//int      UdpRtspAudioSocket = 0;
	int 	   have_audio_socket = 0;
	int      buf_num;
#ifndef GDM_RTSP_SERVER
	struct timeval tv;
#endif
	mid_plat_sockaddr_in temp_client_addr;
	mid_plat_sockaddr_in temp_video_addr;
	mid_plat_sockaddr_in temp_audio_addr;

	mid_plat_sockaddr_in video_addr;
	mid_plat_sockaddr_in audio_addr;
	unsigned int socklen;
	int m, n;

	int video_port = 0;

	int audio_port = 0;

	int roomid = strMsg->roomid;
	char *client = NULL;
	int mult = 0;
	char ip[32] = {0};
	int vport = 0;
	int aport = 0;



	socklen = sizeof(mid_plat_sockaddr_in);
	//	memset(&peeraddr, 0, socklen);
	memset(&video_addr, 0, socklen);
	memset(&audio_addr, 0, socklen);

	for(m = 0; m < strMsg->ucClientportLen[0]; m++) {
		if(strMsg->aucClientport[0][m] == 0x2d) {
			break;
		}
	}

	for(n = 0; n < m; n++) {
		video_port += (strMsg->aucClientport[0][n] - 0x30) * pow(10, (m - n - 1));
	}

	PRINTF("video socket m=%d,port=%d,audio_port = %d\n", m, video_port);

	//must have video port else will return -1;
	if(m == 0 || video_port == 0) {
		PRINTF("ERROR,the video socket port is error,m=%d,video_port= %d\n", m, video_port);
		return -1;
	}

	// bzero(&peeraddr,socklen);
	//memset(&peeraddr,0,socklen);

	rtsp_porting_get_ginfo(&mult, ip, &vport, &aport);

	if(mult == 1) {
		inet_pton(AF_INET, ip, &video_addr.sin_addr);
	} else {
		video_addr.sin_addr.s_addr = client_addr.sin_addr.s_addr;
	}

	//   	video_addr.sin_addr.s_addr = client_addr.sin_addr.s_addr;
	video_addr.sin_port = htons(video_port);
	video_addr.sin_family = AF_INET;


	for(m = 0; m < strMsg->ucClientportLen[1]; m++) {
		if(strMsg->aucClientport[1][m] == 0x2d) {
			break;
		}
	}

	for(n = 0; n < m; n++) {
		audio_port += (strMsg->aucClientport[1][n] - 0x30) * pow(10, (m - n - 1));
	}

	PRINTF("aduio socket m=%d,port=%d\n", m, audio_port);

	if(m == 0 || audio_port == 0) {
		PRINTF("WARNNING,the audio socket port is error,m=%d,audio_port= %d\n", m, audio_port);
		have_audio_socket = -1;
	} else {
		//bzero(&peeraddr,socklen);
		memset(&audio_addr, 0, socklen);

		if(mult == 1) {
			inet_pton(AF_INET, ip, &audio_addr.sin_addr);
		} else {
			audio_addr.sin_addr.s_addr = client_addr.sin_addr.s_addr;
		}

		audio_addr.sin_port = htons(audio_port);
		audio_addr.sin_family = AF_INET;
	}


	//compare the all client socket
	//if have repeat
	client = rtsp_porting_create_client(&video_addr, &audio_addr);

	if(client == NULL) {
		PRINTF("create client is ERROR\n");
		return 0;
	}

	PRINTF("create client succes ,client=%p\n", client);

	SETRTSPCLIENT(0, nPos, client);

	SETRTSPUDPREADY(0, nPos, 1);

	//i will save the socket addr int  static struct;
	SETRTSPADDR(0, nPos, client_addr);


	PRINTF("create rtsp client sock success\n");
	return 0;
}


#endif

//*******************************************************************************************
/*
                    开启rtsp线程，接收用户
*/
//*******************************************************************************************
//rtsp clinet num just for ts
static int g_ts_client_num = 0;
static int rtsp_ts_set_client_num(int flag)
{
	g_ts_client_num += flag;

	if(g_ts_client_num < 0) {
		g_ts_client_num = 0;
	}

	return 0;
}
int rtsp_ts_get_client_num()
{
	return g_ts_client_num ;
}

//add by zhangmin
#ifdef GDM_RTSP_SERVER
static unsigned __stdcall  RtspVlcContact(void *pParams)
#else
static int  RtspVlcContact(void *pParams)
#endif
{
	char pRecv_temp[TCP_BUFF_LEN * 2] = {0};
	char                            pRecv[TCP_BUFF_LEN * 2] = {0};
	int                              cnt = 0;
	int                             nPos = 0;
	int					tnum = 0;
	int                             			msg_type = 0;
	Thread_Param                   		*rtsp_thread = (Thread_Param *)pParams;
	mid_plat_sockaddr_in              	Listen_addr;
	char                    				local_ip[64] = {0};
	mid_plat_socket                  	sSocket = rtsp_thread->client_socket;
	nPos = rtsp_thread->nPos;
	int                             resvlen = 0;
	int resvlen_temp = 0;
	int 					recv_errno = 0;
	STR_CLIENT_MSG 		strMsg;
	int 					client_flag = VLC_CLIENT;
	int ret = 0;
	long last_time = 0;
	long current_time = 0;

	int timeout = rtsp_get_client_timeout();
	int need_timeout = 0;

	char filename[256] = {0};

	memcpy(&Listen_addr, &rtsp_thread->client_addr, sizeof(mid_plat_sockaddr_in));
	PRINTF("----------have client--------num=%d-----------------------------\n", RtspGetClientNum());
	PRINTF("\n------------- the %d th vlc connecting -----------------\n\n", (nPos + 1));
	memset(pRecv, 0, sizeof(pRecv));
	rtsp_get_local_ip(local_ip, sizeof(local_ip));


	/*set timeout 3s*/
	mid_plat_set_socket_stimeout(sSocket, 3000);
	mid_plat_set_socket_rtimeout(sSocket, 5000);

	memset(&strMsg, 0, sizeof(STR_CLIENT_MSG));
	strMsg.roomid = -1;

	while(1) {
		if(need_timeout == 1) {
			if(last_time == 0) {
				last_time = getCurrentTimets();
			}

			current_time = getCurrentTimets();

			//PRINTF("current_time =%u,last_time=%u\n",current_time,last_time);
			if(current_time - last_time > rtsp_get_client_timeout() * 2000) {
				PRINTF("WARNNING,the %d client is timeout ,the time is %d,i will close it.\n", nPos, current_time - last_time);
				goto ExitClientMsg;
			}
		}

		if(sSocket <= 0) {
			PRINTF("ERROR ,the file = %s,the line =%d\n", __FILE__, __LINE__);
			goto ExitClientMsg;
		}

		if(GETRTSPCLIENTTYPE(0, nPos) == INVALID_CLIENT) {
			tnum++;

			if(tnum > 5) {
				PRINTF("The %d client is invaild\n", nPos);
				goto ExitClientMsg;
			}
		}

		memset(pRecv_temp, 0, sizeof(pRecv_temp));
		resvlen_temp = recv(sSocket, pRecv_temp, TCP_BUFF_LEN, 0);

		if(resvlen_temp == 0) {
			PRINTF("ERROR ,the file = %s,the line =%d\n", __FILE__, __LINE__);
			PRINTF("the client is close the socket \n");
			goto ExitClientMsg;
		}

		if(resvlen_temp < 0) {
#ifdef GDM_RTSP_SERVER
			recv_errno = WSAGetLastError();
#endif

			if(errno == 11 || recv_errno == 10060) {
				continue;
			} else {
				PRINTF("resvlen = %d,errno=%d,=%d\n", resvlen_temp, errno, recv_errno);
				PRINTF("will goto ,the file = %s,the line =%d\n", __FILE__, __LINE__);
				goto ExitClientMsg;
			}
		}

		if(need_timeout == 1) {
			last_time = getCurrentTimets();
		}

		if(strstr(pRecv_temp, "\r\n\r\n") == NULL) {
			//	PRINTF("Warnning,the msg is not complete,the msg =%s=\n",pRecv_temp);
			memset(pRecv, 0, sizeof(pRecv));
			memcpy(pRecv, pRecv_temp, resvlen_temp);
			continue;
		}

		strcat(pRecv, pRecv_temp);
		resvlen = strlen(pRecv);
		PRINTF("resvlen=%d,\n%s\n", resvlen, pRecv);
		msg_type = GetVlcMsgType(pRecv, resvlen, &strMsg);

		if(msg_type) {
			switch(msg_type) {
				case OPTION:
					ResponseOption(sSocket, &strMsg);
					break;

				case DESCRIBE:

					if(rtsp_describe_authentication(pRecv, resvlen, local_ip, &client_flag, &(strMsg.roomid), filename, &need_timeout) == 0) {
						if(client_flag == STB_TS_CLIENT) {
							ret = ResponseDescribeTs(sSocket, Listen_addr, local_ip, 0, &strMsg);
						} else {
							ret = ResponseDescribe(sSocket, Listen_addr, local_ip, 0, &strMsg);
						}
					} else {
						ret = ResponseDescribe(sSocket, Listen_addr, local_ip, -1, &strMsg);
						PRINTF("ERROR,the file = %s,the line =%d\n", __FILE__, __LINE__);
						goto ExitClientMsg;

					}

					if(ret == -1) {
						goto ExitClientMsg;
					}

					//设置客户端
					SETRTSPCLIENTTYPE(0, nPos, client_flag);

					if(client_flag == STB_TS_CLIENT) {
						rtsp_ts_set_client_num(1);
					}

					break;

				case SETUP:
					if(need_timeout == 1) {
						timeout =  rtsp_get_client_timeout();
					} else {
						timeout = 0;
					}

					ResponseSetup(pRecv, resvlen, sSocket, &strMsg, timeout);
					break;

				case PLAY:
					ret = RtspAddClient(Listen_addr, nPos, &strMsg);

					if(ret == -1) {
						goto ExitClientMsg;
					}

					ResponsePlay(sSocket, &strMsg);
					//force iframe
					rtsp_stream_force_Iframe(strMsg.roomid);

					if(0 == g_rtsp_flag) { //when fist vlc connect
						g_rtsp_flag = 1;
					}


					PRINTF("I success create the %d client ,the client type is %d\n", nPos, GETRTSPCLIENTTYPE(0, nPos));
					break;

				case ANNOUNCE:
					//SendAnnounce(sSocket,Listen_addr,local_ip,&strMsg);
					break;

				case PAUSE:
					RtspPauseClient(nPos);
					ResponsePause(sSocket, &strMsg);
					//					PRINTF("the service is not support PAUSE\n");
					break;

				case TEARDOWN:
					ResponseTeardown(sSocket, &strMsg);
					goto ExitClientMsg;

				case SET_PARAMETER:
					ResponseSet_parameter(sSocket, &strMsg);
					break;

				case GET_PARAMETER:
					ResponseGet_parameter(sSocket, &strMsg);
					break;

				default:
					PRINTF("ERROR ,goto exitclientmsg.the file = %s,the line =%d\n", __FILE__, __LINE__);
					goto ExitClientMsg;
					//break;
			}
		}

		memset(pRecv, 0, sizeof(pRecv));
	}

ExitClientMsg:
	PRINTF("Exit RtspCastComMsg nPos = %d sSocket = %d\n", nPos, sSocket);

#ifdef GDM_RTSP_SERVER
	app_rtsp_authentication_stop(filename);
#endif

	RtspDelClient(nPos);

	if(client_flag == STB_TS_CLIENT) {
		rtsp_ts_set_client_num(-1);;
	}

	//if there is no vlc to connect ,then stop rtp package
	cnt = RtspGetClientNum();

	if(cnt) {
		g_rtsp_flag = 1;
	} else {
		g_rtsp_flag = 0;
		//PRINTF("rtsp client byebye!\n");
	}

	PRINTF(" Exit rtsp %d client Pthread!!client num =%d!!ts client num = %d\n", nPos, cnt, g_ts_client_num);
	rtsp_threadid[nPos] = 0;
	return 0;

}



//---------------------------------------RTSP TASK--------------------------------------------------
//add by zhangmin
#ifdef GDM_RTSP_SERVER
static unsigned __stdcall  RtspTask(void *pParams)
#else
static int  RtspTask(void *pParams)
#endif

{
	mid_plat_sockaddr_in              SrvAddr, ClientAddr;
	int                             nLen;
	int                             RtspCliSock = 0;
	int                             RtspSerSock = 0;
	Thread_Param                    rtspthread0;
	Thread_Param                    *rtspthread = &rtspthread0;
	int                             nPos = 0;
	int                             opt = 1;
	int ret = 0;
	//	PRINTF("Rtsp Task........GetPid():%d\n",getpid());
	RtspThreadInit();
RTSPSTARTRUN:
	PRINTF("RtspTask start...\n");
	//bzero( &SrvAddr, sizeof(mid_plat_sockaddr_in) );
	memset(&SrvAddr, 0 , sizeof(mid_plat_sockaddr_in));
	SrvAddr.sin_family = AF_INET;
	SrvAddr.sin_port = htons(RtspGetPort());
	SrvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

#ifdef GDM_RTSP_SERVER
	mid_plat_pthread_t threadHandle;
#endif

	if(RtspSerSock > 0) {
		mid_plat_close_socket(RtspSerSock);
	}

	RtspSerSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(RtspSerSock < 0) {
		PRINTF("ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		return -1;
	}

	setsockopt(RtspSerSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

	if(bind(RtspSerSock, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0) {
		PRINTF("in RtspTask:bind error:%d,error msg: = %s\n", errno, strerror(errno));
		return -1;
	}

	if(listen(RtspSerSock, 2) < 0) {
		PRINTF("listen error:%d,error msg: = %s", errno, strerror(errno));
		return -1;
	}

	while(1) {
#ifndef GDM_RTSP_SERVER
		//	if(rtsp_porting_server_need_stop() == 1)
		//	{
		//		usleep(400000);
		//		continue;
		//	}
#endif

		memset(&ClientAddr, 0, sizeof(mid_plat_sockaddr_in));
		nLen = sizeof(mid_plat_sockaddr_in);
		RtspCliSock = accept(RtspSerSock, (struct sockaddr *)&ClientAddr, (socklen_t *) &nLen);

		if(RtspCliSock > 0) {
			nPos = RtspGetNullClientIndex();
			PRINTF("new rtsp client number: %d \n", nPos);

			if(-1 == nPos) {
				fprintf(stderr, "ERROR: max client error\n");
				mid_plat_close_socket(RtspCliSock);
				mid_plat_sleep(500);
			} else {
				int nSize = 0;
				int result;
				nSize = 1;

				if((setsockopt(RtspCliSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&nSize, sizeof(nSize))) == -1) {
					perror("setsockopt failed");
				}

				nSize = 0;
				nLen = sizeof(nLen);
				result = getsockopt(RtspCliSock, SOL_SOCKET, SO_SNDBUF, (char *)&nSize , (socklen_t *)&nLen);

				if(result) {
					fprintf(stderr, "getsockopt() errno:%d socket:%d  result:%d\n", errno, RtspCliSock, result);
				}

				nSize = 1;

				if(setsockopt(RtspCliSock, IPPROTO_TCP, TCP_NODELAY, (const char *)&nSize , sizeof(nSize))) {
					fprintf(stderr, "Setsockopt error%d\n", errno);
				}

				rtspthread->nPos = nPos;
				rtspthread->client_socket = RtspCliSock;
				memcpy(&rtspthread->client_addr, &ClientAddr, sizeof(mid_plat_sockaddr_in));

				//add by zhangmin
#ifdef GDM_RTSP_SERVER
				threadHandle =  _beginthreadex(NULL, 0, RtspVlcContact, (void *)rtspthread, 0, &rtsp_threadid[nPos]);
				PRINTF("threadHandle == %d\n", threadHandle);

				//threadHandle will return 0,if it is failed
				if(threadHandle == 0) {
					RtspDelClient(nPos);
					mid_plat_close_socket(RtspCliSock);
					fprintf(stderr, "creat pthread ClientMsg error  = %d!\n" , errno);
					continue;
				}
			}

#else
				ret = pthread_create(&rtsp_threadid[nPos], NULL, (void *)RtspVlcContact, (void *)rtspthread);

				if(ret) {
					RtspDelClient(nPos);
					mid_plat_close_socket(RtspCliSock);
					fprintf(stderr, "creat pthread ClientMsg error  = %d!\n" , errno);
					continue;
				}
			}

#endif
		}
		else {
			//	if(errno == ECONNABORTED||errno == EAGAIN)  //软件原因中断
			if(errno == EAGAIN) {
				PRINTF("rtsp errno =%d program again start!!!\n", errno);
				mid_plat_sleep(100);
				continue;
			}

			if(RtspSerSock > 0) {
				mid_plat_close_socket(RtspSerSock);
			}

			goto RTSPSTARTRUN;
		}
	}
	RtspExit();
	PRINTF("close the gServSock server\n");

	if(RtspSerSock > 0)
	{
		PRINTF("close gserv socket \n");
		mid_plat_close_socket(RtspSerSock);
	}

	RtspSerSock = 0;
	return 0;
}

static int g_init_flag = 0;
//add by zhangmin
#ifdef GDM_RTSP_SERVER
void rtsp_module_init(void)
{
	if(g_init_flag != 0) {
		return ;
	}

	PRINTF("now i init the rtsp module,build by zhangmin\n");
	g_init_flag = 1;
	app_rtsp_authentication_clear();
	//rtsp_stream_init(4,0);
	app_center_init("COM1");
	_beginthreadex(NULL, 0, RtspTask, NULL, 0, NULL);
	return ;
}
#else
void rtsp_module_init(void)
{
	if(g_init_flag != 0) {
		return ;
	}

	PRINTF("now i init the rtsp module,build by zhangmin\n");
	g_init_flag = 1;
	//	app_rtsp_authentication_clear();
	//rtsp_stream_init(1,0);
	//	app_center_init("COM1");
	//	 _beginthreadex(NULL, 0, RtspTask, NULL, 0, NULL);
	pthread_t threadid = 0;
	pthread_create(&threadid, NULL, (void *)RtspTask, NULL);
	return ;
}
#endif


int RtspVideoPack(int nLen,  unsigned char *pData, int nFlag, unsigned int timetick , int *seq, int roomid, int idr_flag)
{
	return 0;

	if(g_rtsp_flag != 1) {
		return -1;
	}

	//PRINTF("the nLen= %d\n",nLen);
	NALU_t		nalu;
	unsigned char *pstart;

	unsigned long ts_current  = 0;
	unsigned long mytime = 0;

	unsigned char *pos = pData;
	unsigned char *ptail = pData + nLen - 4;
	unsigned char *temp1, *temp2;
	temp1 = temp2 = NULL;
	int send_total_len = 0;
	unsigned char nalu_value = 0;
	int cnt = 0;

	if(idr_flag == 1) {
		for(cnt = 0; cnt < MAX_RTSP_CLIENT_NUM; cnt++) {
			if((roomid == GETRTSPROOMID(0, cnt)) && (GETRTSPUDPREADY(0, cnt) == 1) && (GETRTSPCLIENTTYPE(0, cnt) == VLC_CLIENT || GETRTSPCLIENTTYPE(0, cnt) == QT_CLIENT)) {
				SETRTSPUDPREADY(0, cnt, 2);
				PRINTF("I will push the data to the %d client\n", cnt);
			}

		}
	}

	//PRINTF("Pdata[0]=0x%02x,Pdata[1]=0x%02x,Pdata[2]=0x%02x,Pdata[3]=0x%02x,len =%d\n",
	//pos[0],pos[1],pos[2],pos[3],nLen);

	for(;;) {
		mytime = timetick;
		ts_current = mytime * 90;
		//	static int g_test_time = 0;
		//	g_test_time ++;
		//	if(g_test_time %30 == 0)
		//		printf("mytime-=0x%x,ts_current=0x%x\n",mytime,ts_current);
		pstart = pos;
		memset(&nalu, 0 , sizeof(nalu));

		//判断头是否nalu头 0001 或者 001 ,不是退出
		if(!((*pos == 0 && *(pos + 1) == 0 && *(pos + 2) == 0 && *(pos + 3) == 1))) {
			PRINTF("read nalu header failed!\n");
		} else {
			temp1 = pos;
			nalu_value = *(pos + 4);
		}

		if(((nalu_value & 0x1f) == 7) || ((nalu_value & 0x1f) == 8)) {
			//找到下一个nalu头 0001 或者 001 , 或者到帧尾
			do {
				pos++;
			} while((*pos != 0 || *(pos + 1) != 0 || *(pos + 2) != 0 || *(pos + 3) != 1)
			        && (pos < ptail));
		} else {
			pos = ptail;
		}

		//	temp2 = pos;

		if(pos >= ptail) {
			//如果是到达帧尾， 则把整个剩余数据作为一个nalu单元发送
			nalu.buf = pstart + 4;
			nalu.len = pData - pstart + nLen  - 4 ;
			//PRINTF("nalu_len=%d,the len =%d,%d,%p,%p,%p\n",nalu.len,temp2-temp1 ,ptail-pstart,pstart,pos,ptail);
			send_total_len += (nalu.len + 4);
			nalu.forbidden_bit = nalu.buf[0] & 0x80;
			nalu.nal_reference_idc = nalu.buf[0] & 0x60;
			nalu.nal_unit_type = (nalu.buf[0]) & 0x1f;
			/*
			                        DEBUG(DL_FLOW,"send last nalu pkt! len = %d frame_len = %d flag = %d \
			                        pdata = 0x%x pos = 0x%x pstart = 0x%x\n", nalu.len, nLen ,nFlag\
			                        , pData , pos , pstart);
			*/
			SendRtpNalu(&nalu, (unsigned short *)seq, ts_current, roomid, 1);
			//PRINTF("seq =%d,time =%ld\n",seq,ts_current);
			break;
		} else {
			//发送一个nalu单元
			nalu.buf = pstart + 4;
			nalu.len = pos - pstart - 4;
			send_total_len += (nalu.len + 4);
			nalu.forbidden_bit = nalu.buf[0] & 0x80;
			nalu.nal_reference_idc = nalu.buf[0] & 0x60;
			nalu.nal_unit_type = (nalu.buf[0]) & 0x1f;
			/*
			                        DEBUG(DL_FLOW,"send nalu pkt! len = %d frame_len = %d flag = %d\
			                        pdata = 0x%x pos = 0x%x pstart = 0x%x\n", nalu.len, nLen ,nFlag\
			                        , pData , pos , pstart);
			 */
			SendRtpNalu(&nalu, (unsigned short *)seq, ts_current, roomid, 0);
			//PRINTF("seq =%d,time =%ld\n",seq,ts_current);
		}
	}

	if(send_total_len != nLen) {
		PRINTF("send_total_len = %d,nLen=%d\n", send_total_len, nLen);
	}

	return 0;
}


/*********************************************************************************************************
                                        RTSP audio 打包函数
**********************************************************************************************************/
static unsigned short           g_audio_seq_num = 0;
int RtspAdudioSeqReset()
{
	g_audio_seq_num = 0;
	return 0;
}

int RtspAudioPack(int nLen,	unsigned char *pData, unsigned int timetick, int roomid, unsigned int samplerate)
{
	return 0;
	unsigned char                   gszAudioRtspBuf[1500] = {0};
	unsigned char                           *sendbuf = gszAudioRtspBuf;
	int                             pLen;
	int                             offset[5] = {0};
	int                             ucFrameLenH = 0, ucFrameLenL = 0;
	int	                            bytes = 0;

	unsigned long                   mytime = 0;
	unsigned long                   ts_current_audio = 0;
	int                     			audio_sample = samplerate; //rtsp_stream_get_audio_samplerate();
	int audio_frame_len = 0;
	int temp_len = 0;

	int i = 0;
	int j = 0;

	unsigned int framelen = 0;
	framelen = ((pData[3] & 0x03) << 9) | (pData[4] << 3) | ((pData[5] & 0xe0) >> 5);


	//PRINTF("pData[0] =%x,%x,%x,%x,len=%d\n",pData[0],pData[1],pData[2],pData[3],nLen)	;
	for(i = 0; i < nLen - 4; i++) {
		if((pData[i] == 0xff && pData[i + 1] == 0xf1 && pData[i + 2] == 0x58)
		   || (pData[i] == 0xff && pData[i + 1] == 0xf1 && pData[i + 2] == 0x5c)
		   || (pData[i] == 0xff && pData[i + 1] == 0xf1 && pData[i + 2] == 0x6c)
		   || (pData[i] == 0xff && pData[i + 1] == 0xf1 && pData[i + 2] == 0x60)
		   || (pData[i] == 0xff && pData[i + 1] == 0xf1 && pData[i + 2] == 0x4c)) {

			offset[j] = i;
			j++;
		}
	}

	if(j > 1) {
		PRINTF("RtspAudioPack j=%d=%d\n", j, nLen);
		//printf("pData[0] =%x,%x,%x,%x,len=%d\n",pData[0],pData[1],pData[2],pData[3],nLen)	;
		//printf("pData[0] =%x,%x,%x,%x,len=%d\n",pData[4],pData[5],pData[6],pData[7],nLen)	;
	}

	if(framelen == nLen && j >= 1) {
		j = 1;
	}


	for(i = 0; i < j; i++) {



		pLen = offset[i + 1] - offset[i];

		if(i == j - 1) {
			pLen = nLen - offset[i];
		}

		temp_len = audio_frame_len = pLen - 7;
		//PRINTF("RtspAudioPack  temp_len = %d\n",temp_len);
		mytime = timetick ;

		while(temp_len > 0) {
			if(temp_len >=  RTP_PAYLOAD_LEN) {
				g_audio_puiNalBuf->len = RTP_PAYLOAD_LEN;
				PRINTF("i=%d,the len =%d=0x%x\n", i, g_audio_puiNalBuf->len, g_audio_puiNalBuf->len);
			} else {
				g_audio_puiNalBuf->len = temp_len;
			}

			ucFrameLenH = audio_frame_len / 32;
			ucFrameLenL = (audio_frame_len % 32) * 8;
			g_audio_puiNalBuf->buf[0] = 0;
			g_audio_puiNalBuf->buf[1] = 0x10;
			g_audio_puiNalBuf->buf[2] = ucFrameLenH;
			g_audio_puiNalBuf->buf[3] = ucFrameLenL;
			memcpy(&g_audio_puiNalBuf->buf[4], pData + offset[i] + 7 + audio_frame_len - temp_len , g_audio_puiNalBuf->len);

			temp_len -= 	g_audio_puiNalBuf->len;
			memset(sendbuf, 0, TCP_BUFF_LEN);

			if(audio_sample == 16000) {
				ts_current_audio = mytime * 16;
			} else if(audio_sample == 32000) {
				ts_current_audio = mytime * 32;
			} else if(audio_sample == 44100) {
				ts_current_audio = mytime * 44.1;
			} else if(audio_sample == 96000) {
				ts_current_audio = mytime * 96;
			} else {
				ts_current_audio = mytime * 48;
			}

			//ts_current_audio = timetick;
			//	static int g_test_audio = 0;
			//	g_test_audio ++;
			//	if(g_test_audio %40 ==0)
			//	printf("audio timetick =0x%x,the ts_current_audio  =0x%x\n",timetick,ts_current_audio);
			bytes = g_audio_puiNalBuf->len + 16 ;

			RTP_FIXED_HEADER_AUDIO *p;
			p = (RTP_FIXED_HEADER_AUDIO *)sendbuf;
			p->byte1 = 0x80;

			if(temp_len != 0) {
				p->byte2 = 0x60;
			} else {
				p->byte2 = 0xe0;
			}

			p->seq_no = htons(g_audio_seq_num++);
			p->timestamp = htonl(ts_current_audio);
			p->ssrc = htonl(10);
			memcpy(&sendbuf[12], g_audio_puiNalBuf->buf, g_audio_puiNalBuf->len + 4);

			if(j == 1) {
				SendRtspAudioData(sendbuf, bytes, roomid);
			} else {
				PRINTF("ERROR!!!!SendRtspAudioData not send.\n");
			}
		}

	}

	//printf("audio bytes =%d,the len=%d\n",bytes,nLen);
	return 0;
}


// SDP改变，需要关闭QT
void rtsp_change_sdp_info(int roomid)
{
	rtsp_change_video_resolve(roomid);
	return ;
}

void rtsp_close_all_client(void)
{
	int cli  = 0;

	for(cli = 0; cli < MAX_RTSP_CLIENT_NUM; cli++) {
		SETRTSPCLIENTTYPE(0, cli, INVALID_CLIENT);
	}

	return ;
}





/**************************************************************************************************
                    获取rtsp 当前版本号
**************************************************************************************************/
char *RtspGetVersion()
{
	char *version;
	version = RTSP_VERSION;
	return (version);
}


//is have client
int RtspClientIsNull(void)
{
	return g_rtsp_flag ;
}







/*默认只支持1NALU打包*/
static unsigned short  g_ts_seq_num = 0;
static unsigned long g_ts_current_time = 0;
int rtsp_ts_send_data(int len , char *buff, int frame_flag)
{
	if(g_rtsp_flag != 1 || rtsp_ts_get_client_num() <= 0) {
		return -1;
	}

	if(len > RTP_PAYLOAD_LEN) {
		PRINTF("ERROR,check it ,the ts len is too big\n");
		return -1;
	}

	int cnt = 0;

	if(frame_flag == 1) {
		for(cnt = 0; cnt < MAX_RTSP_CLIENT_NUM; cnt++) {
			if((0 == GETRTSPROOMID(0, cnt)) && (GETRTSPUDPREADY(0, cnt) == 1) && (GETRTSPCLIENTTYPE(0, cnt) == STB_TS_CLIENT)) {
				SETRTSPUDPREADY(0, cnt, 2);
				PRINTF("I will push the data to the %d client\n", cnt);
			}

		}
	}



	unsigned long  current_time = 0;
	unsigned long  rtptime = 0;
	char	          sendbuf[1500] = {0};
	RTP_FIXED_HEADER			*rtp_hdr;

	rtp_hdr 							= (RTP_FIXED_HEADER *)&sendbuf[0];
	rtp_hdr->payload	                = 33;
	rtp_hdr->version	                = 2;

	rtp_hdr->marker                     = 0;
	rtp_hdr->ssrc		                = htonl(12);

	current_time = getCurrentTimets();

	if(g_ts_current_time == 0) {
		g_ts_current_time = current_time;
	}

	rtptime  = (current_time - g_ts_current_time) * 90;
	//g_ts_current_time = current_time;
	//	PRINTF("rtptime = %u \n",rtptime);
	rtp_hdr->timestamp	= htonl(rtptime);
	rtp_hdr->seq_no = htons((g_ts_seq_num)++);

	if(buff[0] != 0x47) {
		PRINTF("len = %d,buff = 0x%x,0x%x,0x%x,0x%x\n", len, buff[0], buff[1], buff[2], buff[3]);
	}

	memcpy(&sendbuf[12], buff, len);
	len += 12;
	SendRtspVideoData(sendbuf, len, 1, 0);

	return 0;
}

#define DEFAULT_CLINET_TIMEOUT 60
static int rtsp_get_client_timeout()
{
	return DEFAULT_CLINET_TIMEOUT;
}


int rtsp_set_tcp_active(int second)
{
	return 0;
}
int rtsp_set_tcp_mtu(int mtu)
{
	return 0;
}
int rtsp_set_multicast_set(int flag, char *ip, unsigned short port)
{
	return 0;
}

#endif
