
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <termios.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "commontrack.h"
#include <errno.h>
#include "nslog.h"
#include "ppt_index.h"

extern int gRemoteFD;

char ipaddr[256];
char type[8];
int isautotrack = 0;
int isppttxt = 0;
static int times = 0;

PPT_data_info_t ppt_info = {0, {0}};
typedef	struct ppt_network_ {
	int saddr;
	int netmask;
	int gateway;
} ppt_network_t;
ppt_network_t g_ppt_net;

PPT_data_info_t g_ppt_info = {0, {0}};

int getPPTDataInfo(PPT_data_info_t *pPptInfo)
{
	memcpy(pPptInfo, &g_ppt_info, sizeof(PPT_data_info_t));
	//pPptInfo = &g_ppt_info;
	return 0;
}

int getPPTDataInfoStatus()
{
	return g_ppt_info.status;
}
void setPPTDataInfoStatus(int iStatus)
{
	printf("setPPTDataInfoStatus==%d\n", iStatus);
	g_ppt_info.status = iStatus;

}

//处理来串口来的数据
int ProcessPPTdata(unsigned char data[], int len, int fd)
{
	printf("[ProcessPPTdata] data[4] :[0x%x]\n", data[4]);
	char PPTTextBuf[BUFLEN];
	int i = 0;

	if((data[2] == 0x8e) && (data[3] == 0x1) && (data[4] == 0x3)) {
		char ip[16] = {0};
		char mask[16] = {0};
		char gw[16] = {0};
		memset(ipaddr, 0, 256);
		struct in_addr addr, addr2, addr3;
		memcpy(&addr, &(g_ppt_net.saddr), 4);
		memcpy(&addr2, &(g_ppt_net.netmask), 4);
		memcpy(&addr3, &(g_ppt_net.gateway), 4);
		strcpy(ip, inet_ntoa(addr));
		strcpy(mask, inet_ntoa(addr2));
		strcpy(gw, inet_ntoa(addr3));
		sprintf(ipaddr, "XXXXXLan:%s\r\n%s\r\n%s\r\nWan:%s\r\n%s\r\n%s\r\n", ip, mask, gw, ip, mask, gw);
		len = strlen(ipaddr) + 1;
		*(short *)ipaddr = htons(len);
		ipaddr[2] = 0x8e;
		ipaddr[3] = 0x1;
		ipaddr[4] = 0x3;
		ipaddr[len - 1] = 0x8e;
		SendDataToCom(fd, (unsigned char *)ipaddr, strlen(ipaddr + 2) + 2);
		times = 0;
		isppttxt = 1;

	} else if((data[2] == 0x8e) && (data[3] == 0x1) && (data[4] == 0x9)) {

		*(short *)type = htons(7);
		type[2] = 0x8e;
		type[3] = 0x1;
		type[4] = 0x9;
		type[5] = 0x0;
		type[6] = 0x8e;

		send(fd, type, 7, 0);

		for(i = 0; i < 7; i ++) {
			printf("type[%d]:[0x%x]\n", i, type[i]);
		}
	} else if((data[2] == 0x8e) && (data[3] == 0x3) && (data[4] == 0x13)) {
		memset(&g_ppt_info, 0, sizeof(PPT_data_info_t));
		memcpy(g_ppt_info.PPTTextBuf, data + 7, data[6]);	//data[6]是字符串长度不包括后面的0x8e
		setPPTDataInfoStatus(1);

		SetVgaState();
		printf("[ProcessPPTdata1] [%s]\n", g_ppt_info.PPTTextBuf);
	}

	return 0;
}
int remotePPTindexTask(void *pParam)
{
	unsigned char data[256];
	unsigned char szData[BUFLEN];
	char ip[16];
	char mask[16];
	char gw[16];
	unsigned short datalen = 0;
	int len = 0;
	int datacount = 0, i = 0;
	int  cnt, sendsocket, nRet;
	memset(ipaddr, 0, 256);
	struct in_addr addr, addr2, addr3;
	memcpy(&addr, &(g_ppt_net.saddr), 4);
	memcpy(&addr2, &(g_ppt_net.netmask), 4);
	memcpy(&addr3, &(g_ppt_net.gateway), 4);
	strcpy(ip, inet_ntoa(addr));
	strcpy(mask, inet_ntoa(addr2));
	strcpy(gw, inet_ntoa(addr3));
	sprintf(ipaddr, "XXXXXLan:%s\r\n%s\r\n%s\r\nWan:%s\r\n%s\r\n%s\r\n", ip, mask, gw, ip, mask, gw);
	len = strlen(ipaddr) + 1;
	*(short *)ipaddr = htons(len);
	ipaddr[2] = 0x8e;
	ipaddr[3] = 0x1;
	ipaddr[4] = 0x3;
	ipaddr[len - 1] = 0x8e;

	while(1) {

		usleep(100000);
		memset(data, 0, 256);
		len = read(gRemoteFD, data, 256) ; //成功返回数据量大小，失败返回－1

		//printf("len===============%d",len);
		if(len > 0) {
			if((datacount + len) < BUFLEN) {
				memcpy(szData + datacount, data, len);
				datacount += len;
			}

		} else {
			if(datacount > 5) {
				i = 0;

				if(datacount > BUFLEN) {
					datacount = BUFLEN;
				}

				while((i < datacount)) {
					memcpy(&datalen, szData + i, sizeof(short));
					datalen = ntohs(datalen);

					if((datalen > 2) && (datalen <= datacount)) {
						ProcessPPTdata(szData + i, datalen, gRemoteFD);
					}

					i += datalen;

				}

				times = 0;
			} else if(datacount == 0) {
				times++;

				if(times > 100) { //10秒钟收不到数据
					isppttxt = 0;
					times = 0;

				}
			}

			datacount = 0;

		}

	}

	return len;
}

/*set send timeout*/
static int SetSendTimeOut(int sSocket, unsigned long time)
{
	struct timeval timeout ;
	int ret = 0;

	timeout.tv_sec = time ; //3
	timeout.tv_usec = 0;

	ret = setsockopt(sSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

	if(ret == -1) {
		fprintf(stderr, "setsockopt() Set Send Time Failed\n");
	}

	return ret;
}

/*set recv timeout*/
static int SetRecvTimeOut(int sSocket, unsigned long time)
{
	struct timeval timeout ;
	int ret = 0;

	timeout.tv_sec = time ; //3
	timeout.tv_usec = 0;

	ret = setsockopt(sSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	if(ret == -1) {
		fprintf(stderr, "setsockopt() Set Recv Time Failed\n");
	}

	return ret;
}

void PPTComMsg(void *pParams)
{
	nslog(NS_DEBUG, "Come in PPTComMsg\n");
	int                         socklen;
	char                        szData[512];
	int                         nLen;
	int                      sSocket;
	unsigned short datalen = 0;
	socklen                   = sizeof(struct sockaddr_in);
	sSocket                   = *(int *)(pParams);


	/*set timeout 10s*/
	SetRecvTimeOut(sSocket, 10);
	SetSendTimeOut(sSocket, 2);
	/*set recv time out*/
	bzero(szData, 256);

	while(1) {
		memset(szData, 0, sizeof(szData));

		if(sSocket <= 0) {
			goto ExitClientMsg;
		}

		//printf(" ----------------before recv,sSocket = %d----------------\n",sSocket);
		nLen = recv(sSocket, szData, HEADLEN, 0);

		//printf(" ----------------end recv, nlen = %d ----------------\n",nLen);
		if(nLen < HEADLEN || nLen == -1) {
			fprintf(stderr, "HEADLEN=%d < %d  exit!\n", nLen, HEADLEN);
			goto ExitClientMsg;
		}

		memcpy(&datalen, szData, sizeof(short));
		datalen = ntohs(datalen);
		nslog(NS_DEBUG, "Headlen:%d,datalen=%d!\n", nLen, datalen);

		if(datalen > 512) {
			goto ExitClientMsg;
		}

		//printf("rtp receive length:%d,HEAD_LEN:%d\n",nLen,phead->nLen);
		if(datalen - HEADLEN > 0) {
			nLen = recv(sSocket, szData + HEADLEN, datalen - HEADLEN, 0);
			nslog(NS_DEBUG, "datalen = %d,recvlen:%d!\n", datalen, nLen);

			if(nLen < datalen - HEADLEN) {
				nslog(NS_DEBUG, "recvlen < nMsgLen-HEADLEN exit\n");
				goto ExitClientMsg;
			}
		}

		ProcessPPTdata((unsigned char *)szData, datalen, sSocket);
		nslog(NS_DEBUG, "PPT recv End!\n");
	}

ExitClientMsg:
	nslog(NS_ERROR, "[PPTIndex] Exit Pthread!!!!\n");

}

void PPTindexTask(void *pParam)
{

	struct sockaddr_in SrvAddr, ClientAddr;
	int nLen;
	short sPort = PORT_LISTEN_PPT;

	int sclient;
	int fileflags;
	int opt = 1;
	nslog(NS_DEBUG, "PPTindexTask start GetPid():%d\n", getpid());
	int ClientSocket = 0;
	int ServerSocket = 0;
	g_ppt_net.saddr = GetIPaddr("eth0");
	g_ppt_net.netmask = GetNetmask("eth0");
	g_ppt_net.gateway = getGateWay("eth0");
RESTART:
	bzero(&SrvAddr, sizeof(struct sockaddr_in));
	SrvAddr.sin_family = AF_INET;
	SrvAddr.sin_port = htons(sPort);
	SrvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(ServerSocket > 0)	{
		close(ServerSocket);
	}

	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(ServerSocket < 0)  {
		nslog(NS_ERROR, "ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		return;
	}

	setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(ServerSocket, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0)  {
		nslog(NS_ERROR, "bind terror:%d,error msg: = %s\n", errno, strerror(errno));
		return;
	}

	if(listen(ServerSocket, 10) < 0) {
		nslog(NS_ERROR, "listen error:%d,error msg: = %s", errno, strerror(errno));
		return ;
	}

	if((fileflags = fcntl(ServerSocket, F_GETFL, 0)) == -1) {
		nslog(NS_ERROR, "fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		return;
	}

	//if (fcntl(ServerSocket, F_SETFL, fileflags | O_NONBLOCK) == -1)
	//{
	//  nslog(NS_ERROR,"fcntl F_SETFL error:%d,error msg: = %s\n",errno,strerror(errno));
	//  gblSetQuit();
	//  return;
	//}

	nslog(NS_DEBUG, "[PPTIndex] Socket is OK!\n");
	nLen = sizeof(struct sockaddr_in);

	while(1)  {
		memset(&ClientAddr, 0, sizeof(struct sockaddr_in));
		nLen = sizeof(struct sockaddr_in);
		sclient = accept(ServerSocket, (void *)&ClientAddr, (unsigned int *)&nLen);
		printf("sclient=%d\n", sclient);

		if(sclient > 0)  	{
			char chTemp[16], *pchTemp;
			pchTemp = &chTemp[0];
			int nSize = 0;
			int result;

			if(ClientSocket > 0) {
				nslog(NS_WARN, "close ClientSocket = %d\n", ClientSocket);
				ClientSocket  =  -1;
			}

			nSize = 1;

			if((setsockopt(sclient, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize, sizeof(nSize))) == -1) {
				perror("setsockopt failed");
			}

			nSize = 0;
			nLen = sizeof(nLen);
			result = getsockopt(sclient, SOL_SOCKET, SO_SNDBUF, &nSize , (unsigned int *)&nLen);

			if(result) {
				nslog(NS_ERROR, "getsockopt() errno:%d socket:%d  result:%d\n", errno, ClientSocket, result);
			}

			nSize = 1;

			if(setsockopt(sclient, IPPROTO_TCP, TCP_NODELAY, &nSize , sizeof(nSize))) {
				nslog(NS_ERROR, "Setsockopt error%d\n", errno);
			}

			memset(chTemp, 0, 16);
			pchTemp = inet_ntoa(ClientAddr.sin_addr);
			nslog(NS_DEBUG, "[PPTIndex] Clent:%s connected, socket:%d!\n", chTemp, sclient);
			ClientSocket = sclient;
			PPTComMsg(&ClientSocket);
			close(ClientSocket);
			ClientSocket  =  -1;
			nslog(NS_DEBUG, "[PPTIndex] sem_wait() semphone inval!!!  result = %d\n", result);
			continue;
		} else {
			if(errno == ECONNABORTED || errno == EAGAIN) { //软件原因中断
				//				printf("errno =%d program again start!!!\n",errno);
				usleep(100000);
				continue;
			}

			if(ServerSocket > 0)	{
				close(ServerSocket);
			}

			goto RESTART;
		}

	}

	nslog(NS_DEBUG, "close the gServSock \n");

	if(ServerSocket > 0)	{
		nslog(NS_DEBUG, "close gserv socket \n");
		close(ServerSocket);
	}

	ServerSocket = 0;
	return;
}

