#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>


#include "rec_client.h"



#define UDPBUFFERSZIE   		655360



#define HEART_RETRYNUM			10
#define PORT_RNG				20
#define MSG_MAX_LEN				256
#define	TIMEOUTSEC   			3
#define TCPMSGTIMEOUTSEC 		0

#define BITRATE_MSEC			5000

#define MIN_IDRFRAME_SIZE		2048
#define PACK_HEADER_LEN			48

#define IH264KEYCODE			MAKEFOURCC('H','2','6','4')
#define IAACKEYCODE				MAKEFOURCC('A','D','T','S')
#define DEFAULT_SAMPLE			44100
#define AUDIO_WAIT_MAXMSEC		15
#define MIN_PFRAME_LEN			200

#define	FALSE					(0)
#define	TRUE					(1)


#define MSGTHREADCREATED		0x1
#define HEARTTHREADCREATED		0x2

#define MSG_THREAD_PRIORITY sched_get_priority_max(SCHED_FIFO) - 3
#define HEART_THREAD_PRIORITY	sched_get_priority_max(SCHED_FIFO) - 3
#define LOADER_THREAD_PRIORITY  sched_get_priority_max(SCHED_FIFO) - 2



#define SERVPORT_START    	3000
#define LIVE_BASE_PROT		7000
#define AVIIF_KEYFRAME		0x000010L

/*�ȴ�socket���ճ�ʱʱ��*/
#define WAIT_RECV_TIMEOUT  		3

#define UDP_CHANNLE_NUM 		1
#define MSG_VER		       		1
#define MSG_TYPE_INFO	   		1
#define MSG_TYPE_HEART	   		2
#define MSG_TYPE_PASSWORD  		3
#define MSG_OPEN_FILE      		4      //VOD�ļ���
#define MSG_VOD_START      		5      //��ʼVOD
#define MSG_VOD_STOP       		6      //ֹͣVOD
#define MSG_VOD_PAUSE      		7      //��ͣ
#define MSG_VOD_TIME       		8      //��ʱ�䲥��
#define MSG_TIMER          		9      //��ʱ��
#define MSG_READ_VIDEO     		10     //��ͼ������
#define MSG_READ_AUDIO     		11     //����������
#define MSG_READ_DATA      		12     //����������
#define MSG_HEART_BEAT     		13     //������Ϣ
#define MSG_FILE_LIST      		14     //�ļ��б�
#define MSG_PC_DATA        		15     //pc����

#define MSG_VOD_PWD				16		//�㲥����  ��ӦsData=0 OK sData=1 ���� sData=2 û�е㲥����
#define MSG_VOD_PLAY_FILE		18		//�㲥�ļ�	��ӦsData=0 OK(���FileInfo) sData=1����
#define MSG_VOD_STOP_FILE		19		//ֹͣ����
#define MSG_VOD_PAUSE_FILE		20		//��ͣ����
#define MSG_VOD_GOTO_FILE		21		//����ָ����ʱ�䲥��
#define MSG_VOD_RESTART_FILE	23		//�ָ�����
#define MSG_VOD_PLAY_END		24		//���Ž���
#define MSG_VOD_MARK_INFO		25	  	//����Mark��Ϣ

#define MSG_FIRE_WALL   		35
#define MSG_TYPE_DATA			36		//VGA����

#define MSG_TYPE_USEINFO		40		//RecPlayer�û����� �û����� 1 RecServer, 0 Player
#define MSG_TYPE_USERLIST		41		//�û��б�0 �����б� 1�¼��û���2�û��˳�
#define MSG_TYPE_BDPROXY		42		//�鲥���� 0 �ر� 1��
#define MSG_TYPE_BDSTATUS		43		//��ѯ�鲥����״̬ + Int �����ѯ��UserID 0 Ϊ���С�
#define MSG_TYPE_CHAT			44		//��������
#define MSG_TYPE_TRANSFILE		45		//�����ļ���
#define MSG_TYPE_CALLNAME		46		//����
#define MSG_TYPE_CLOSEUSER		47		//�ر��û� �� UserID
#define MSG_TYPE_CHATMNG		48		//������رջػ�
#define MSG_TYPE_CNNTINFO  		50
#define MSG_NEW_HEART       	51

#define MAX_USER_NAME			64

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                      \
	((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
	 ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

#define USER_RECPLY			0
#define USER_RECSEV			1

#define	AVBASE_PORT			7000
#define AV_PORT(i)			(AVBASE_PORT + i)

typedef unsigned long       DWORD;
typedef unsigned long		ULONG;
typedef unsigned char       BYTE;
typedef unsigned short     	WORD;
typedef int 				BOOL;

/*UDP NetWork Header*/
typedef struct HDB_FRAME_HEAD {
	DWORD ID;	//= mmioFOURCC('R','Q','H','D');
	DWORD nTimeTick;    //ʱ���
	DWORD nFrameLength; //֡���ݳ���
	DWORD nDataCodec;   //���뷽ʽ
	DWORD nFrameRate;   //����֡�� �� ��Ƶ������
	DWORD nWidth;       //���
	DWORD nHight;       //�߶�
	DWORD nColors;      //��ɫ��
	DWORD dwSegment;	//�����ʾ 0��ʾ�м�� 1��ʾ��β�� 2��ʾ��ʼ�� 3��ʾ������
	DWORD dwFlags;		//֡��־ I֡��
	DWORD dwPacketNumber; //
	DWORD nOthers;      //����Ӧ�������
} HDB_FRAME_HEAD;


typedef struct _MsgUserInfo {
	int Type;
	ULONG LocalIP;
	char UserName[MAX_USER_NAME];
} MsgUserInfo;




typedef struct _MsgThreadEnv {
	int						tcpSockfd; //TCP���ӵ�socket
	int						channel;   //ͨ�����
	int						connectMode;//����ģʽ 0��ʾ������1��ʾ�鲥
	const char				*serverip;
	int					*udpsocketfds;
	pthread_mutex_t 		*pSocketmutex;

	REC_CLIENT				*pclient;
} MsgThreadEnv;

typedef struct strMsgHeader {
	unsigned short sLen;		//����
	unsigned short sVer;		//�汾
	unsigned short sMsgType;	//��Ϣ����
	unsigned short sData;	//����
} MsgHeader, *pMsgHeader;

typedef struct _HeartThreadEnv {
	pthread_mutex_t 		*pSocketmutex;
	int						tcpSockfd; //TCP���ӵ�socket
	REC_CLIENT				*pclient;
} HeartThreadEnv;

typedef struct _LoaderEnv {
	int 				    subChlNo;
} LoaderEnv;



int gblGetStopPlay(REC_CLIENT *pinst)
{
	int quit;

	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(pinst->gbl.mutex));
	quit = pinst->gbl.stopPlay;
	pthread_mutex_unlock(&(pinst->gbl.mutex));

	return quit;
}

int gblGetStopPlay2(void *handle)
{
	int quit;

	REC_CLIENT *pinst = (REC_CLIENT *)handle;

	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(pinst->gbl.mutex));
	quit = pinst->gbl.stopPlay;
	pthread_mutex_unlock(&(pinst->gbl.mutex));

	return quit;
}



void gblSetStopPlay(REC_CLIENT *pinst, int stop)
{
	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return ;
	}

	pthread_mutex_lock(&(pinst->gbl.mutex));
	pinst->gbl.stopPlay = stop;
	pthread_mutex_unlock(&(pinst->gbl.mutex));
}

void gblSetStopPlay2(void *handle)
{
	REC_CLIENT *pinst = (REC_CLIENT *)handle;

	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return ;
	}

	pthread_mutex_lock(&(pinst->gbl.mutex));
	pinst->gbl.stopPlay = 1;
	pthread_mutex_unlock(&(pinst->gbl.mutex));
}


/* Functions to protect the global data */
int gblGetQuit(REC_CLIENT *pinst)
{
	int quit;

	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(pinst->gbl.mutex));
	quit = pinst->gbl.quit;
	pthread_mutex_unlock(&(pinst->gbl.mutex));

	return quit;
}

void gblSetRun(REC_CLIENT *pinst)
{
	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return ;
	}

	pthread_mutex_lock(&(pinst->gbl.mutex));
	pinst->gbl.quit = FALSE;
	pthread_mutex_unlock(&(pinst->gbl.mutex));
}

void gblSetQuit(REC_CLIENT *pinst)
{
	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return ;
	}

	pthread_mutex_lock(&(pinst->gbl.mutex));
	pinst->gbl.quit = TRUE;
	pthread_mutex_unlock(&(pinst->gbl.mutex));
}


void InitiFrameFlag(REC_CLIENT *pinst)
{
	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return;
	}

	int roomIdx;

	for(roomIdx = 0; roomIdx < ROOM_MAX_BOX; roomIdx++) {
		pinst->iFrameFlag.IFrameRecived[roomIdx] = 0;
		pthread_mutex_init(&(pinst->iFrameFlag.mutex[roomIdx]), NULL);
	}

}

int getIFrameStatus(REC_CLIENT *pinst, int roomIdx)
{
	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return -1;
	}

	int retVal = 0;
	pthread_mutex_lock(&(pinst->iFrameFlag.mutex[roomIdx]));
	retVal  = pinst->iFrameFlag.IFrameRecived[roomIdx];
	pthread_mutex_unlock(&(pinst->iFrameFlag.mutex[roomIdx]));
	return retVal;
}

void setIFrameStatus(REC_CLIENT *pinst, int roomIdx, int status)
{
	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return;
	}

	if(roomIdx >= ROOM_MAX_BOX) {
		return;
	}

	pthread_mutex_lock(&(pinst->iFrameFlag.mutex[roomIdx]));
	pinst->iFrameFlag.IFrameRecived[roomIdx] = status;
	pthread_mutex_unlock(&(pinst->iFrameFlag.mutex[roomIdx]));
}

void initUDPsocket(REC_CLIENT *pinst)
{
	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return;
	}

	int roomIdx;

	for(roomIdx = 0; roomIdx < ROOM_MAX_BOX; roomIdx++) {
		pinst->loaderSockets.udpSocket[roomIdx] = 0;
		pthread_mutex_init(&(pinst->loaderSockets.mutextAccess[roomIdx]), NULL);
	}
}

int getUDPSocket(REC_CLIENT *pinst, int roomIdx)
{
	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return -1;
	}

	int retVal = 0;

	if(roomIdx >= ROOM_MAX_BOX) {
		return -1;
	}

	pthread_mutex_lock(&(pinst->loaderSockets.mutextAccess[roomIdx]));
	retVal  = pinst->loaderSockets.udpSocket[roomIdx];
	pthread_mutex_unlock(&(pinst->loaderSockets.mutextAccess[roomIdx]));

	return retVal;
}

void setUDPSocket(REC_CLIENT *pinst, int roomIdx, int socket)
{
	if(NULL == pinst) {
		fprintf(stderr, "err, pinst is NULL!\n");
		return ;
	}

	if(roomIdx >= ROOM_MAX_BOX) {
		return ;
	}

	pthread_mutex_lock(&(pinst->loaderSockets.mutextAccess[roomIdx]));
	pinst->loaderSockets.udpSocket[roomIdx] = socket;
	pthread_mutex_unlock(&(pinst->loaderSockets.mutextAccess[roomIdx]));
}


/*��װ��TCP�������ݺ���,ȷ�����ݽ�������*/
static int tcpRecvLongData(int sockfd, void *buffer, const int len)
{
	int totalRecv = 0;
	int nRecvLen = 0;

	while(totalRecv < len) {
		nRecvLen = recv(sockfd, buffer + totalRecv, len - totalRecv, 0);

		if(nRecvLen < 1) {
			//			fprintf(stderr, "recv tcp data failed, error message:%s \n",strerror(errno));
			return -1;
		}

		totalRecv += nRecvLen;

		if(totalRecv == len) {
			break;
		}
	}

	return totalRecv;
}


/*��װ��TCP�������ݣ�ȷ��������������*/
static int tcpSendLongData(int sockfd, void *buffer, const int len)
{
	int totalSend = 0;
	int nSendLen = 0;

	while(totalSend < len) {
		nSendLen = send(sockfd, buffer + totalSend, len - totalSend, 0);

		if(nSendLen < 1) {
			fprintf(stderr, "send tcp data failed, error message:%s \n", strerror(errno));
			return -1;
		}

		totalSend += nSendLen;

		if(len == totalSend) {
			break;
		}
	}

	return totalSend;
}

static int ConnectUniCast(const char *pAddr, const char *pPwd, int nChannel)
{
	int 					servSockfd	= 0;
	int 					sendlen 	= 0;
	int						fileflags	= 0;

	struct sockaddr_in		serv_addr;
	struct timeval			tv;

	MsgHeader				msg;

	servSockfd = socket(PF_INET, SOCK_STREAM, 0);
	sendlen = sizeof(MsgHeader);

	if(servSockfd < 1) {
		fprintf(stderr, "Create Socket failed,error message:%s \n", strerror(errno));
		return -1;
	}

	fprintf(stderr, "002--ConnectUniCast! socket = %d, ip = %s, channel = %d\n",
	        servSockfd, pAddr, nChannel);

	serv_addr.sin_family	= AF_INET;
	serv_addr.sin_port		= htons(SERVPORT_START + nChannel);

	inet_aton(pAddr, (struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero), 8);
	tv.tv_sec	= TIMEOUTSEC;
	tv.tv_usec	= 500;
	setsockopt(servSockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
	setsockopt(servSockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));

	if(connect(servSockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "Connet to live server failed, error message:%s \n", strerror(errno));
		close(servSockfd);
		return -1;
	}


	if((fileflags = fcntl(servSockfd, F_GETFL, 0)) == -1) {
		fprintf(stderr, "fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		return -1;
	}

	if(fcntl(servSockfd, F_SETFL, fileflags & (~O_NONBLOCK)) == -1) {
		fprintf(stderr, "fcntl F_SETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		return -1;
	}


	msg.sVer		= MSG_VER;
	msg.sData		= 0;
	msg.sMsgType	= MSG_TYPE_PASSWORD;
	msg.sLen = htons(sizeof(MsgHeader) + strlen(pPwd));

	if(tcpSendLongData(servSockfd, &msg, sendlen) < sendlen) {
		fprintf(stderr, "Send Message Header to server failed error message:%s \n", strerror(errno));
		close(servSockfd);
		return -1;
	}

	sendlen = strlen(pPwd);

	if(tcpSendLongData(servSockfd, (void *)pPwd, sendlen) < sendlen) {
		fprintf(stderr, "Send Password to server failed error message:%s \n", strerror(errno));
		close(servSockfd);
		return -1;
	}

	sendlen = sizeof(MsgHeader);

	if(tcpRecvLongData(servSockfd, &msg, sendlen) < sendlen) {
		fprintf(stderr, "Recevice Message  from server failed error message:%s \n", strerror(errno));
		close(servSockfd);
		return -1;
	}

	if(msg.sMsgType != MSG_TYPE_PASSWORD) {
		fprintf(stderr, "Error Message \n");
		close(servSockfd);
		return -1;
	}

	if(msg.sData != 0) {
		int ret = 0 - 1 - msg.sData;
		fprintf(stderr, "ret = %d \n", ret);
		close(servSockfd);
		return ret;

	}

	int nVerLen = ntohs(msg.sLen) - sizeof(MsgHeader);
	int nRevLen  = 0;

	if(nVerLen > 0) {
		char *pVer = malloc(nVerLen + 1);
		int nrecvlen = 0;
		pVer[nVerLen] = 0;

		while(nRevLen < nVerLen) {
			nrecvlen = tcpRecvLongData(servSockfd, (BYTE *)pVer + nRevLen, nVerLen - nRevLen);

			if(nrecvlen < 1) {
				fprintf(stderr, "recv the nVerlen Failed \n");
				free(pVer);
				return -1;
			}

			nRevLen += nrecvlen;
		}

		free(pVer);
	}

	return servSockfd;
}



int SendUserIDPack(const int CnntMode, const int userID, const int idx,
                   const int channel, const char *serverip, int sockfd)
{
	int nPort = AVBASE_PORT + idx + 100;
	int sendlen;
	int temp = 0xe7e7e7e7;

	BYTE pBuf[12];
	temp = 0x7e7e7e7e;
	struct sockaddr_in serv_addr;

	memcpy(pBuf, &temp, 4);
	temp = idx + 1;
	memcpy(pBuf + 4, &temp, 4);
	temp = userID;
	memcpy(pBuf + 8, &temp, 4);

	if(!CnntMode) {
		nPort += PORT_RNG * channel;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(nPort);
	inet_aton(serverip, (struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero), 8);
	sendlen = sendto(sockfd,  pBuf,  12, 0, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in));
	fprintf(stderr, "SendUserIDPack success, sendlen = %d\n", sendlen);

	if(sendlen == -1) {
		fprintf(stderr, "errno = %d,strerror(errno = %s) \n", errno, strerror(errno));
		return -1;
	}

	return sendlen;
}


static int SendUserInfoTcpMsgToServer(int sockfd)
{
	struct sockaddr_in lcadr;
	const int nl = sizeof(MsgUserInfo) + sizeof(MsgHeader);
	BYTE pData[nl];
	MsgUserInfo msgUserInfo;
	MsgHeader	msgHeader;
	socklen_t ln = sizeof(lcadr);

	msgHeader.sVer = MSG_VER;
	msgHeader.sData = 0;
	msgHeader.sMsgType = MSG_TYPE_USEINFO;
	msgHeader.sLen = htons(nl);
	memcpy(pData, &msgHeader, sizeof(MsgHeader));
	msgUserInfo.Type = USER_RECPLY;
	msgUserInfo.LocalIP = 0;

	if(getsockname(sockfd, (struct sockaddr *)&lcadr, &ln) == 0) {
		msgUserInfo.LocalIP = lcadr.sin_addr.s_addr;
	}

	strcpy(msgUserInfo.UserName, "admin");
	memcpy(pData + sizeof(MsgHeader), &msgUserInfo, sizeof(MsgUserInfo));

	if(tcpSendLongData(sockfd, pData, nl) < 1) {
		printf("send user info to server failed \n");
		return -1;
	}

	return 0;
}

void *HeartThread(void *arg)
{
	MsgHeader				msg;
	HeartThreadEnv			*envp		= (HeartThreadEnv *)arg;
	int 					sockfd		= envp->tcpSockfd;
	int 					msglen		= sizeof(MsgHeader);
	int 					sendnum 	= 0;
	REC_CLIENT				*pinst		= envp->pclient;

	msg.sVer		= MSG_VER;
	msg.sData		= 0;
	msg.sMsgType	= MSG_NEW_HEART;
	msg.sLen		= htons(msglen);

	fprintf(stderr, "HeartThread start!\n");

	while(!gblGetStopPlay(pinst)) {
		pthread_mutex_lock(envp->pSocketmutex);
		sendnum = tcpSendLongData(sockfd, &msg, msglen);

		//		fprintf(stderr, "sendnum = %d, sockfd = %d\n", sendnum, sockfd);
		if(sendnum < 0) {
			sendnum = tcpSendLongData(sockfd, &msg, msglen);

			if(sendnum < 0) {
				sendnum = tcpSendLongData(sockfd, &msg, msglen);

				if(sendnum < 0) {
					fprintf(stderr, "heart thread exit  failed send = %d,errno = %d\n", sendnum, errno);
					fprintf(stderr, "error MSg = %s \n", strerror(errno));
					pthread_mutex_unlock(envp->pSocketmutex);
					goto cleanup;
				}
			}
		}

		pthread_mutex_unlock(envp->pSocketmutex);
		usleep(1000000);
	}

cleanup:

	fprintf(stderr, "HeartThread exit!\n");

	//	gblSetStopPlay(pinst, TRUE);

	return 0;
}

void *ServerMsgThread(void *arg)
{
	MsgThreadEnv			*envp					= (MsgThreadEnv *)arg;
	char					msgdata[MSG_MAX_LEN]	= {0};
	MsgHeader				msg;
	int 					sockfd					= envp->tcpSockfd;
	int 					roomid;
	int 					msgdatalen				= 0;
	int 					nRevLen 				= 0;
	int 					recvlen 				= sizeof(MsgHeader);
	int 					userID					= 0;
	int 					udpSocket				= 0;

	REC_CLIENT				*pinst					= envp->pclient;


	/*���ý��ճ�ʱʱ��*/
	struct timeval timeout;
	timeout.tv_sec = TCPMSGTIMEOUTSEC;
	timeout.tv_sec = 3;
	timeout.tv_usec = 200000;

	pthread_mutex_lock(envp->pSocketmutex);
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(struct timeval));
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, sizeof(struct timeval));
	pthread_mutex_unlock(envp->pSocketmutex);

	//	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	fprintf(stderr, "ServerMsgThread start!\n");


	fprintf(stderr, "errno ==  EAGAIN, socket = %d\n", sockfd);

	while(sockfd) {
		userID = 0;
		msgdatalen = 0;
		recvlen = sizeof(MsgHeader);

		pthread_mutex_lock(envp->pSocketmutex);
		nRevLen = tcpRecvLongData(sockfd, &msg, recvlen);
		pthread_mutex_unlock(envp->pSocketmutex);

		if(nRevLen < recvlen) {
			if(errno ==  EAGAIN) {
				//				fprintf(stderr, "errno ==  EAGAIN, socket = %d\n", sockfd);
				usleep(100000);
			}

			usleep(1000000);
			continue;
		}

		fprintf(stderr, "errno ==  EAGAIN, socket = %d\n", sockfd);
		msgdatalen = ntohs(msg.sLen) - recvlen;

		if((msg.sMsgType == MSG_FIRE_WALL) && (msgdatalen >= 4)) {
			pthread_mutex_lock(envp->pSocketmutex);

			if(tcpRecvLongData(sockfd, &userID, 4) < 4) {
				printf("received msg_fire_wall user id failed \n");
				pthread_mutex_unlock(envp->pSocketmutex);
				goto cleanup;
			}

			pthread_mutex_unlock(envp->pSocketmutex);

			msgdatalen -= 4;
		}

		if(msgdatalen > 0) {
			int factrecvlen = 0;

			do {
				recvlen = msgdatalen > MSG_MAX_LEN ? MSG_MAX_LEN : msgdatalen;
				pthread_mutex_lock(envp->pSocketmutex);
				factrecvlen = tcpRecvLongData(sockfd, msgdata, recvlen);

				if(factrecvlen < 1) {
					printf("recive data failed error message =%s \n", strerror(errno));
					pthread_mutex_unlock(envp->pSocketmutex);
					goto cleanup;
				} else {
					msgdatalen -= factrecvlen;
				}

				pthread_mutex_unlock(envp->pSocketmutex);
			} while(msgdatalen > 0 && !gblGetStopPlay(pinst));
		}

		printf("msg.sMsgType = %d,%d\n", msg.sMsgType, msgdatalen);

		switch(msg.sMsgType) {
			case MSG_FIRE_WALL: {
				/*��͸����ǽ*/
				printf("001--throuth the  fire wall \n");
				pthread_mutex_lock(envp->pSocketmutex);
				SendUserInfoTcpMsgToServer(sockfd);
				printf("002--throuth the  fire wall \n");
				pthread_mutex_unlock(envp->pSocketmutex);

				for(roomid = 0; roomid < ROOM_MAX_BOX; roomid++) {
					printf("003--throuth the  fire wall \n");

					if(userID != 0) {
						printf("004--throuth the  fire wall \n");
						udpSocket = envp->udpsocketfds[roomid];
						pthread_mutex_lock(envp->pSocketmutex);
						printf("005--throuth the  fire wall \n");

						if(SendUserIDPack(envp->connectMode, userID, roomid,
						                  envp->channel, envp->serverip, udpSocket) < 1) {
							fprintf(stderr, "Send User ID Failed \n");
							pthread_mutex_unlock(envp->pSocketmutex);
							goto cleanup;
						}

						printf("006--throuth the  fire wall \n");
						pthread_mutex_unlock(envp->pSocketmutex);
					}

					printf("serverMsgthread udpSocket = %d\n", udpSocket);
					setUDPSocket(pinst, roomid, udpSocket);
				}
			}
			break;

			case MSG_TYPE_INFO:
				break;

			case MSG_TYPE_DATA: { //VGA����

			}
			break;

			default
					:
				break;
		}
	}

cleanup:

	//	gblSetStopPlay(pinst, TRUE);

	return 0;
}

int livePlay(void *arg)
{
	unsigned int 			initMask						= 0;
	MsgThreadEnv			msgEnv;
	HeartThreadEnv			heartEnv;

	int 					sockefd 						= 0;
	int 					roomid;
	int 					udpsocketfds[ROOM_MAX_BOX]		= {0};
	int 					udpsktfd						= 0;
	int						channel							= 0;
	int						index							= 0;

	void					*ret;
	int 					nRecvBuf						= UDPBUFFERSZIE;

	struct sockaddr_in		local_addr;
	struct sched_param		schedParam;
	struct timeval 			tv;

	pthread_attr_t			attr;
	pthread_mutex_t 		socketmutex;

	pthread_t				msgThread;
	pthread_t				heartThread;

	REC_CLIENT *pinst = (REC_CLIENT *)arg;

	if(NULL == pinst) {
		fprintf(stderr, "liveplay: arg is NULL!\n");
		return -1;
	}

	char ip[64] = {0};

	strcpy(ip, pinst->ipaddr);
	channel = pinst->channel;
	index = pinst->index;

	InitiFrameFlag(pinst);
	initUDPsocket(pinst);
	pthread_mutex_init(&socketmutex, NULL);

	tv.tv_sec 	= 3;
	tv.tv_usec 	= 500;

	gblSetRun(pinst);
	gblSetStopPlay(pinst, FALSE);
	sockefd = ConnectUniCast(ip, "Test2006", channel);

	if(sockefd < 1) {
		fprintf(stderr, "livePlay errno = %d,errmsg = %s\n", errno, strerror(errno));
		goto cleanup;
	}

	if(pthread_attr_init(&attr)) {
		fprintf(stderr, "Failed to initialize thread attrs\n");
		goto cleanup;
	}

	if(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) {
		fprintf(stderr, "Failed to set schedule inheritance attribute\n");
		goto cleanup;
	}

	if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
		fprintf(stderr, "Failed to set FIFO scheduling policy\n");
		goto cleanup;
	}

	/*����UDP �����˿�*/
	for(roomid = 0; roomid < ROOM_MAX_BOX; roomid++) {
		int opt = 1;
		setIFrameStatus(pinst, roomid, 0);
		udpsocketfds[roomid] = socket(PF_INET, SOCK_DGRAM, 0);

		if(udpsocketfds[roomid] < 1) {
			fprintf(stderr, "Failed to Create Socket \n");
			goto cleanup;
		}

		printf("udp create udpsocketfds[roomid] = %d!\n", udpsocketfds[roomid]);
		bzero(&local_addr, sizeof(local_addr));
		local_addr.sin_family		= AF_INET;
		local_addr.sin_addr.s_addr	= htonl(INADDR_ANY);
		local_addr.sin_port 		= htons(roomid + LIVE_BASE_PROT + 10 * index);

		setsockopt(udpsocketfds[roomid], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if(bind(udpsocketfds[roomid], (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1) {
			fprintf(stderr, "Failed to bind port,%s\n", strerror(errno));
			goto cleanup;
		}

		setsockopt(udpsocketfds[roomid], SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
		setsockopt(udpsocketfds[roomid], SOL_SOCKET, SO_RCVBUF, (const char *)&nRecvBuf, sizeof(int));

		schedParam.sched_priority = LOADER_THREAD_PRIORITY;

		if(pthread_attr_setschedparam(&attr, &schedParam)) {
			fprintf(stderr, "Failed to set scheduler parameters\n");
			goto cleanup;
		}

#if 0

		//���������������ȼ�
		if(pthread_create(&loaderThread[roomid], &attr, loaderThrFxn, &loaderEnv[roomid])) {
			fprintf(stderr, "Failed to create loader thread\n");
			goto cleanup;
		}

		initMask |= LOADERTHREADCREATED;
#endif
	}

	fprintf(stderr, "sockefd = %d\n", sockefd);
	/*������Ϣ�߳�*/
	msgEnv.channel = channel;
	/*0��ʾ���� 1��ʾ�鲥*/
	msgEnv.connectMode				= 0;
	msgEnv.serverip 				= ip;
	msgEnv.tcpSockfd				= sockefd;
	msgEnv.udpsocketfds 			= udpsocketfds;
	msgEnv.pSocketmutex 			= &socketmutex;
	msgEnv.pclient					= pinst;

	schedParam.sched_priority		= MSG_THREAD_PRIORITY;

	if(pthread_attr_setschedparam(&attr, &schedParam)) {
		fprintf(stderr, "Failed to set scheduler parameters\n");
		goto cleanup;
	}

	if(pthread_create(&msgThread, NULL, ServerMsgThread, &msgEnv)) {
		fprintf(stderr, "Failed to create ServerMsgThread thread, errmsg =%s\n",
		        strerror(errno));
		goto cleanup;
	}

	initMask |= MSGTHREADCREATED;
	/*���������߳�*/
	heartEnv.tcpSockfd				= sockefd;
	heartEnv.pSocketmutex			= &socketmutex;
	heartEnv.pclient				= pinst;
	schedParam.sched_priority		= HEART_THREAD_PRIORITY;

	if(pthread_attr_setschedparam(&attr, &schedParam)) {
		fprintf(stderr, "Failed to set scheduler parameters\n");
		goto cleanup;
	}

	if(pthread_create(&heartThread, NULL, HeartThread, &heartEnv)) {
		fprintf(stderr, "Failed to create HeartThread video thread\n");
		goto cleanup;
	}


	while(!gblGetStopPlay(pinst)) {
		fprintf(stderr, "rec client is running!, socket 1 = %d, socket 2 = %d\n",
		        pinst->loaderSockets.udpSocket[0],
		        pinst->loaderSockets.udpSocket[1]);
		sleep(5);
	}

cleanup:

	//	gblSetStopPlay(pinst, TRUE);

	if(initMask & MSGTHREADCREATED) {
		pthread_join(msgThread, &ret);
	}

	if(initMask & HEARTTHREADCREATED) {
		pthread_join(heartThread, &ret);
	}

	for(roomid = 0; roomid < ROOM_MAX_BOX; roomid++) {
		udpsktfd = getUDPSocket(pinst, roomid);

		if(udpsktfd) {
			close(udpsktfd);
		} else {
			if(udpsocketfds[roomid]) {
				close(udpsocketfds[roomid]);
			}
		}

		setUDPSocket(pinst, roomid, 0);
	}

	if(sockefd) {
		close(sockefd);
	}

	pthread_mutex_destroy(&socketmutex);

	fprintf(stderr, "liveplay exit!, !gblGetStopPlay(pinst) = %d\n", !gblGetStopPlay(pinst));

	pthread_detach(pthread_self());

	return 0;
}

int recvFrame(void *handle, RecFrame *pFrame, const int channel, const int flag)
{
	int socket = 0;
	int reclen = 0;

	unsigned int offset = 0;
	unsigned int length = 0;

	HDB_FRAME_HEAD	*pheader = NULL;

	REC_CLIENT *pinst = (REC_CLIENT *)handle;

	if(NULL == pinst) {
		fprintf(stderr, "recvFrame: handle is NULL!\n");
		return -1;
	}


	if(channel < 0 || channel >= ROOM_MAX_BOX) {
		fprintf(stderr, "channel error, channel = %d\n", channel);
		return -1;
	}

	socket = pinst->loaderSockets.udpSocket[channel];

	if(socket <= 0) {
		fprintf(stderr, "date socket error, socket = %d\n", socket);
		return -1;
	}

	offset = 0;

	char *pdate = pinst->pdata;

	while(1) {
		//		fprintf(stderr, "\n\n socket = %d\n \n", socket);
		reclen = recv(socket, pdate, MAX_UDP_LEN, 0);

		//		fprintf(stderr, "socket = %d, reclen = %d \n", socket, reclen);
		if(reclen < 1) {
			if(errno !=  EAGAIN) {
				fprintf(stderr, "errno !=  EAGAIN \n");
				usleep(100000);
			}

			usleep(1000);
			continue;
		}

		pheader = (HDB_FRAME_HEAD *)pdate;

		if((pheader->nDataCodec != IH264KEYCODE)
		   && (pheader->nDataCodec != IAACKEYCODE)) {
			continue;
		}

		if((pheader->dwSegment < 0) || (pheader->dwSegment > 3)) {
			continue;
		}

		if((int)pheader->dwFlags < 0) {
			continue;
		}

		if(IH264KEYCODE == pheader->nDataCodec) {
			if(flag == 2)
				// ǿ�ƽ�������֡
			{
				continue;
			}

			if(flag == 0)
				// ǿ�ƽ���I֡
			{
				if((pheader->dwFlags & AVIIF_KEYFRAME) != AVIIF_KEYFRAME) {
					continue;
				}
			}

			if((3 == pheader->dwSegment) || (2 == pheader->dwSegment))
				// ��ʼ��
			{
				if(pheader->nTimeTick <= 0) {
					pheader->nTimeTick = 100;
				}

				if(pheader->nTimeTick == 0xFFFFFFFF) {
					pheader->nTimeTick = 0xFFFFFFFE;
				}

				length = pheader->nFrameLength;

				pFrame->width = pheader->nWidth;
				pFrame->height = pheader->nHight;
				pFrame->length = pheader->nFrameLength;
				pFrame->channel = channel;

				if((pheader->dwFlags & AVIIF_KEYFRAME) == AVIIF_KEYFRAME) {
					pFrame->flag = 1;
				} else {
					pFrame->flag = 0;
				}
			}

			memcpy(pFrame->data + offset, pdate + PACK_HEADER_LEN, reclen - PACK_HEADER_LEN);
			offset += (reclen - PACK_HEADER_LEN);

			//		printf("dwPacketNumber = %d\n", pheader->dwPacketNumber);

			if((3 == pheader->dwSegment) || (1 == pheader->dwSegment)
			   || (pheader->nFrameLength < offset))
				//������
			{
				if(length != offset || pheader->nFrameLength != length) {
					fprintf(stderr, "frameLength = %d,vOffset = %d,pheader->nFrameLength = %d\n",
					        length, offset, (int)pheader->nFrameLength);

					return -1;
				} else {
					if(flag == 0) {
						char *tmp = (char *)pFrame->data;

						/*�ж��Ƿ�IDR֡*/
						if(tmp[0] != 0x00 || tmp[1] != 0x00 ||
						   tmp[2] != 0x00 || tmp[3] != 0x01 || tmp[4] != 0x67) {
							fprintf(stderr, "It's not an IDR frame!\n");
							return -1;
						}
					}
				}

				offset = 0;

				break;
			}
		} else if(IAACKEYCODE == pheader->nDataCodec) {
			if(flag == 0)
				//ǿ�ƽ���I֡
			{
				continue;
			}
		}
	}

	return 0;
}

int recvFrame2(int *socket_st, int st_num, RecFrame *pFrame,
               ST_MARK *pmark, unsigned char *pdata, unsigned int *enable_index)
{
	int reclen = 0;

	int maxfd = 0;

	fd_set readfd;

	struct timeval timeout;

	HDB_FRAME_HEAD	*pheader = NULL;

	if(NULL == socket_st) {
		fprintf(stderr, "recvFrame: socket_st is NULL!\n");
		return -1;
	}


	if(NULL == pFrame) {
		fprintf(stderr, "recvFrame: pFrame is NULL!\n");
		return -1;
	}

	if(NULL == pmark) {
		fprintf(stderr, "recvFrame: *pmark is NULL!\n");
		return -1;
	}


	int index = 0;
	int seret = 0;

	int flag = 1;

	while(1) {
		FD_ZERO(&readfd);
#if 1

		for(index = 0; index < st_num; index++) {
			if(pmark[index].connected == 0) {
				continue;
			}

			//		fprintf(stderr, "     %d       \n", index);
			FD_SET(socket_st[index], &readfd);

			if(socket_st[index] > maxfd) {
				maxfd = socket_st[index];
			}
		}

#endif
		timeout.tv_sec = 1;
		timeout.tv_usec = 100;

		seret = select(maxfd + 1, &readfd, NULL, NULL, &timeout);

		//		fprintf(stderr, "seret = %d\n", seret);
		if(seret > 0)
			// FD��׼����
		{
			for(index = 0; index < st_num; index++) {
				if(FD_ISSET(socket_st[index], &readfd)) {
					//					fprintf(stderr, "socket_st[index] = %d\n", socket_st[index]);
					reclen = recv(socket_st[index], pdata, MAX_UDP_LEN, 0);

					if(reclen < 1) {
						if(errno !=  EAGAIN) {
							fprintf(stderr, "errno !=  EAGAIN, errmsg = %s\n", strerror(errno));
							return -1;
						}

						usleep(1000);
						continue;
					}

					pmark[index].lasttime = getostime();

					pheader = (HDB_FRAME_HEAD *)pdata;

					if((pheader->nDataCodec != IH264KEYCODE)
					   && (pheader->nDataCodec != IAACKEYCODE)) {
						continue;
					}

					if((pheader->dwSegment < 0) || (pheader->dwSegment > 3)) {
						continue;
					}

					if((int)pheader->dwFlags < 0) {
						continue;
					}

					if(IH264KEYCODE == pheader->nDataCodec) {
						if(flag == 2)
							// ǿ�ƽ�������֡
						{
							continue;
						}

						if(flag == 0)
							// ǿ�ƽ���I֡
						{
							if((pheader->dwFlags & AVIIF_KEYFRAME) != AVIIF_KEYFRAME) {
								continue;
							}
						}

						if((3 == pheader->dwSegment) || (2 == pheader->dwSegment))
							// ��ʼ��
						{
							if(pheader->nTimeTick <= 0) {
								pheader->nTimeTick = 100;
							}

							if(pheader->nTimeTick == 0xFFFFFFFF) {
								pheader->nTimeTick = 0xFFFFFFFE;
							}

							pmark[index].length = pheader->nFrameLength;

							pFrame[index].width = pheader->nWidth;
							pFrame[index].height = pheader->nHight;
							pFrame[index].length = pheader->nFrameLength;
							pFrame[index].channel = index;

							if((pheader->dwFlags & AVIIF_KEYFRAME) == AVIIF_KEYFRAME) {
								pFrame[index].flag = 1;
							} else {
								pFrame[index].flag = 0;
							}
						}

						memcpy(pFrame[index].data + pmark[index].offset, pdata + PACK_HEADER_LEN, reclen - PACK_HEADER_LEN);
						pmark[index].offset += (reclen - PACK_HEADER_LEN);

						//		printf("pmark[index].offset = %d\n", pmark[index].offset);

						//		printf("dwPacketNumber = %d\n", pheader->dwPacketNumber);

						if((3 == pheader->dwSegment) || (1 == pheader->dwSegment)
						   || (pheader->nFrameLength < pmark[index].offset))
							//������
						{
							if(pmark[index].length != pmark[index].offset || pheader->nFrameLength != pmark[index].length) {
								fprintf(stderr, "frameLength = %d,vOffset = %d,pheader->nFrameLength = %d\n",
								        pmark[index].length, pmark[index].offset, (int)pheader->nFrameLength);

								pmark[index].offset = 0;
								pmark[index].length = 0;

								return -1;
							} else {
								if(flag == 0) {
									char *tmp = (char *)pFrame[index].data;

									/*�ж��Ƿ�IDR֡*/
									if(tmp[0] != 0x00 || tmp[1] != 0x00 ||
									   tmp[2] != 0x00 || tmp[3] != 0x01 || tmp[4] != 0x67) {
										fprintf(stderr, "It's not an IDR frame!\n");
										return -1;
									}
								}

								*enable_index = index;
								pmark[index].length = 0;
								pmark[index].offset = 0;

								//				fprintf(stderr, "recv a new frame, index = %d\n", index);

								return 2;
							}

							pmark[index].offset = 0;

							break;
						}
					} else if(IAACKEYCODE == pheader->nDataCodec) {
						if(flag == 0)
							//ǿ�ƽ���I֡
						{
							continue;
						}
					}
				}
			}
		} else if(seret == 0)
			// ��ʱ
		{
			usleep(1000);
			return -1;
		} else if(seret < 0)
			// �쳣
		{
			usleep(1000);
			return -1;
		}
	}

	return 0;
}

int recvFrame3(int socket_st, RecFrame *pFrame, unsigned char *pdata)
{
	int reclen = 0;

	int maxfd = 0;

	fd_set readfd;

	struct timeval timeout;
	HDB_FRAME_HEAD	*pheader = NULL;

	if(NULL == pFrame) {
		fprintf(stderr, "recvFrame: pFrame is NULL!\n");
		return -1;
	}

	int seret = 0;

	int flag = 1;

	while(1) {
		FD_ZERO(&readfd);
		FD_SET(socket_st, &readfd);

		if(socket_st > maxfd) {
			maxfd = socket_st;
		}


		timeout.tv_sec = 1;
		timeout.tv_usec = 100;

		seret = select(maxfd + 1, &readfd, NULL, NULL, &timeout);

		if(seret > 0)
			// FD��׼����
		{
			if(FD_ISSET(socket_st, &readfd)) {

				reclen = recv(socket_st, pdata, MAX_UDP_LEN, 0);

				if(reclen < 1) {
					if(errno !=  EAGAIN) {
						fprintf(stderr, "errno !=  EAGAIN, errmsg = %s\n", strerror(errno));
						usleep(100000);
					}

					usleep(1000);
					continue;
				}

				pFrame->videomark.lasttime = getostime();

				pheader = (HDB_FRAME_HEAD *)pdata;

				assert(pheader->nFrameLength <= 1920 * 1080);

				if((pheader->nDataCodec != IH264KEYCODE)
				   && (pheader->nDataCodec != IAACKEYCODE)) {
					continue;
				}

				if((pheader->dwSegment < 0) || (pheader->dwSegment > 3)) {
					continue;
				}

				if((int)pheader->dwFlags < 0) {
					continue;
				}

				if(IH264KEYCODE == pheader->nDataCodec) {
					//fprintf(stderr, "recv number-----------------> %d %d %d\n", pheader->dwPacketNumber,socket_st[index],reclen);

#if 0
					if(pheader->dwPacketNumber != PacketNumber + 1) {
						fprintf(stderr, "recv number-----------------> %d %d\n", pheader->dwPacketNumber, socket_st[index], reclen);
					}

					PacketNumber = pheader->dwPacketNumber;
#endif

					if(flag == 2)
						// ǿ�ƽ�������֡
					{
						continue;
					}

					if(flag == 0)
						// ǿ�ƽ���I֡
					{
						if((pheader->dwFlags & AVIIF_KEYFRAME) != AVIIF_KEYFRAME) {
							continue;
						}
					}

					if((3 == pheader->dwSegment) || (2 == pheader->dwSegment))
						// ��ʼ��
					{
						if(pheader->nTimeTick <= 0) {
							pheader->nTimeTick = 100;
						}

						if(pheader->nTimeTick == 0xFFFFFFFF) {
							pheader->nTimeTick = 0xFFFFFFFE;
						}

						pFrame->videomark.length = pheader->nFrameLength;

						pFrame->width = pheader->nWidth;
						pFrame->height = pheader->nHight;
						pFrame->length = pheader->nFrameLength;

						if((pheader->dwFlags & AVIIF_KEYFRAME) == AVIIF_KEYFRAME) {
							pFrame->flag = 1;
						} else {
							pFrame->flag = 0;
						}
					}

					memcpy(pFrame->data + pFrame->videomark.offset, pdata + PACK_HEADER_LEN, reclen - PACK_HEADER_LEN);
					pFrame->videomark.offset += (reclen - PACK_HEADER_LEN);

					if((3 == pheader->dwSegment) || (1 == pheader->dwSegment)
					   || (pheader->nFrameLength < pFrame->videomark.offset))
						//������
					{
						if(pFrame->videomark.length != pFrame->videomark.offset || pheader->nFrameLength != pFrame->videomark.length) {
							fprintf(stderr, "frameLength = %d,vOffset = %d,pheader->nFrameLength = %d\n",
							        pFrame->videomark.length, pFrame->videomark.offset, (int)pheader->nFrameLength);

							pFrame->videomark.offset = 0;
							pFrame->videomark.length = 0;

							return -1;
						} else {
							if(flag == 0) {
								char *tmp = (char *)pFrame->data;

								/*�ж��Ƿ�IDR֡*/
								if(tmp[0] != 0x00 || tmp[1] != 0x00 ||
								   tmp[2] != 0x00 || tmp[3] != 0x01 || tmp[4] != 0x67) {
									fprintf(stderr, "It's not an IDR frame!\n");
									return -1;
								}
							}

							pFrame->width = pheader->nWidth;
							pFrame->height = pheader->nHight;
							pFrame->length = pheader->nFrameLength;
							pFrame->videomark.length = 0;
							pFrame->videomark.offset = 0;

							pFrame->code = 1;

							return 2;
						}


						break;
					}
				} else if(IAACKEYCODE == pheader->nDataCodec && pFrame->IsAudio) {

					memcpy(pFrame->AudioData + pFrame->Audiomark.offset, pdata + PACK_HEADER_LEN, reclen - PACK_HEADER_LEN);
					pFrame->Audiomark.offset += (reclen - PACK_HEADER_LEN);

					/* ����ǽ����� */
					//||pheader->nFrameLength < aOffset
					if(pheader->dwSegment == 3 || pheader->dwSegment == 1) {
						if(pFrame->Audiomark.offset != pheader->nFrameLength) {
							printf("lost audio rtp !!!\n");
							pFrame->Audiomark.offset = 0;
							return -1;
						} else {
							pFrame->channel = pheader->nWidth;    //ͨ����
							pFrame->length  = pheader->nFrameLength; //����
							pFrame->width   = pheader->nColors;     //������
							pFrame->height  = pheader->nFrameRate ;      //ͨ��
							pFrame->framerate =	pheader->nHight; //������
							pFrame->timestamp = pheader->nTimeTick;
							pFrame->Audiomark.offset = 0;
							pFrame->code = 2;
						}

						return 2;
					} else {

						printf("continue recevie aduio data \n");
					}

				}
			}
		} else if(seret == 0)
			// ��ʱ
		{
			printf("!!!!!!!!!!!timeout!!!!!!!!!!!!!!!\n");
			usleep(1000);
			return -1;
		} else if(seret < 0)
			// �쳣
		{
			printf("!!!!!!!!!!!seret<0!!!!!!!!!!!!!!!\n");
			usleep(1000);
			return -1;
		}
	}

	return 0;
}



static int FindH264StartNAL(unsigned char *pp)
{
	/*is for 00 00 00 01 Nal header*/
	if(pp[0] != 0 || pp[1] != 0 || pp[2] != 0 || pp[3] != 1) {
		return 0;
	} else {
		return 1;
	}
}


int ParseIDRHeader(unsigned char *pdata)
{
	int len = 0x17;
	int ret = 0, I_frame_header_length = 0;
	unsigned char *find = pdata;

	while(find) {
		ret = FindH264StartNAL(find);

		if(ret) {
			I_frame_header_length++ ;

			if(I_frame_header_length >= 3) {
				break;
			} else {
				find += 3;    //find next NAL header
			}
		}

		find++;
	}

	len = (int)(find - pdata);

	if(len > 0x40) {
		len = 0x18;
	}

	return len;
}


int create_rec_client_inst(void **handle, char *ipaddr, int channel, int inst_index)
{
	pthread_t 				thid;

	if(NULL == handle) {
		fprintf(stderr, "handle is NULL!\n");
		return -1;
	}

	REC_CLIENT *pinst = (REC_CLIENT *)malloc(sizeof(REC_CLIENT));

	if(NULL == pinst) {
		fprintf(stderr, "create_rec_client_inst failed!\n");
		return -1;
	}

	strcpy(pinst->ipaddr, ipaddr);
	pinst->channel = channel;
	pinst->index = inst_index;

	if(pthread_create(&thid, NULL, (void *)livePlay, (void *)(pinst))) {
		fprintf(stderr, "failed create livePlay thread!\n");
		goto cleanup;
	}

	*handle = pinst;

	return 0;

cleanup:

	free(pinst);

	return -1;
}
