/*
* Copyright (c) 2010,深圳锐取软件技术有限公司
*
* 文件名称：liveplay.c
* 摘 要：此为使用recplayer(v7.1.2)或actplay(v1.0.3.6)插件或DEC1000连接直播
*		 的服务端代码，测试例程在liveplay_test文件夹
* 当前版本：1.0
* 作 者：huanghh
* 完成日期：2011年2月15日
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>

#include "liveplay.h"



#define RUNNING						1
#define STOPED						0


/*********************************************************************************/
#define LIVE_MAX_RECUSER			10					// 支持的直播连接最大数目
#define LIVE_MAX_PACKET			14600				// 单个发送包的最大大小
#define LIVE_LISTEN_RECPLAY_PORT 	3000				// 直播监听socket的端口
#define	LIVE_UDPSOCKET_PORT		7100				// 发送数据UDPsocket的端口
#define LIVE_LISTEN_MAX_NUM		10					// 直播监听socket的最大监听数
#define	LIVE_MAX_UDPLINK			6					// 发送数据USPsocket的最大数量
// 与支持的视频通道数相对应

#define LIVE_SERVER_VERSION 		"6.0.51.0"			// 服务端版本
#define LIVE_MINPLAYER_VERSION	"4.1.8"				// 
#define	LIVE_CONNECT_KEY_1		"Manager2006"		// 连接密码1
#define	LIVE_CONNECT_KEY_2		"Test2006"			// 连接密码2
// 任一密码验证通过，皆可连接直播

#define LIVEPLAY_LIB_VERSION		"1.0.1"				// 库版本
/*********************************************************************************/


#define DEBUG(x,level,fmt,arg...) \
	do{\
		if ((x) <= (level)){\
			fprintf(stderr, fmt, ##arg);\
		}\
	}while (0)

#define  MSG_ADDCLIENT      			1
#define  MSG_DELETECLIENT   			2
#define  MSG_CONNECTSUCC    		3
#define  MSG_PASSWORD_ERR   		4
#define  MSG_MAXCLIENT_ERR  		5
#define  MSG_AUDIODATA				6
#define  MSG_SCREENDATA     			7
#define  MSG_HEARTBEAT      			8
#define  MSG_PASSWORD       			9
#define  MSG_DATAINFO       			10
#define  MSG_REQ_I          			11
#define  MSG_SET_FRAMERATE  		12
#define  MSG_PPT_INDEX  			15

#define MSG_SYSPARAMS				16
#define MSG_SETPARAMS				17
#define MSG_RESTARTSYS				18
#define MSG_UpdataFile				19
#define MSG_SAVEPARAMS			20
#define MSG_UpdataFile_FAILS			21
#define MSG_UpdataFile_SUCC			22
#define MSG_DECODE_STATUS			23
#define MSG_DECODE_DISCONNECT	24
#define MSG_DECODE_CONNECT		25
#define MSG_UPDATEFILE_DECODE 	26
#define MSG_UPDATEFILE_ROOT 		27
#define MSG_UPDATEFILE_WEB 		28

#define MSG_MODE_CODE				29
#define MSG_MODE_DECODE			30

#define MSG_ADD_TEXT				33

#define	MSG_EXIT_STREAMING		37

#define MSG_MOUT          				40
#define MSG_SENDFLAG    				41
#define MSG_FARCTRL      				42
#define MSG_VGA_ADJUST				43

#define MSG_GET_VIDEOPARAM		0x70
#define MSG_SET_VIDEOPARAM		0x71
#define MSG_GET_AUDIOPARAM		0x72
#define MSG_SET_AUDIOPARAM		0x73
#define MSG_REQ_AUDIO				0x74
#define MSG_CHG_PRODUCT			0x75

#define MSG_SET_SYSTIME			0x77
#define MSG_SET_DEFPARAM			0x79

#define MSG_SET_PICPARAM			0x90
#define MSG_GET_PICPARAM			0x91


#define MSG_CHANGE_INPUT			0x92


#define MSG_SEND_INPUT				0x93

#define AVIIF_KEYFRAME				0x00000010

#define INVALID_FD					-1


#define MSG_VER		       			1
#define MSG_TYPE_INFO	   			1
#define MSG_TYPE_PASSWORD  		3
#define MSG_TYPE_CTRL	   			30 		//远遥消息
#define MSG_TYPE_TITLE	   			31 		//字幕消息
#define MSG_TYPE_PIC	   			32 		//改图像质量(色度亮度等)
#define MSG_TYPE_GETINFO   			33		//获取信息后跟LAUNET_VIPARAM
#define MSG_TYPE_MARK	   			34		//添加实时说明或会议室备注
#define MSG_TYPE_ID		   			35		//用户ID
#define MSG_TYPE_DATA				36		//VGA数据

#define MSG_TYPE_USEINFO			40		//RecPlayer用户名， 用户种类 1 RecServer, 0 Player
#define MSG_TYPE_USERLIST			41		//用户列表。0 完整列表， 1新加用户、2用户退出
#define MSG_TYPE_BDPROXY			42		//组播代理 0 关闭 1打开
#define MSG_TYPE_BDSTATUS			43		//查询组播代理状态 + Int > 0 代表查询的UserID 0 为所有。
#define MSG_TYPE_CHAT				44		//聊天数据
#define MSG_TYPE_TRANSFILE			45		//传输文件。
#define MSG_TYPE_CALLNAME			46		//点名
#define MSG_TYPE_CLOSEUSER		47		//关闭用户 ＋ UserID
#define MSG_TYPE_CHATMNG			48		//开启或关闭回话

#define MSG_TYPE_BOXINFO			50		//BOX连接信息
#define MSG_TYPE_HEART				51		//心跳消息



/*message header*/
typedef struct __HDB_MSGHEAD {
	/*
	##  length for htons change
	## include length of structure
	## and real data
	*/
	LIVE_WORD	nLen;
	LIVE_WORD	nVer;							//version
	LIVE_BYTE	nMsg;							//message type
	LIVE_BYTE	szTemp[3];						//reserve
} MSGHEAD;

#define HEAD_LEN			sizeof(MSGHEAD)


/*//audio and video frame header */
typedef struct __HDB_FRAME_HEAD {
	LIVE_DWORD ID;								//=mmioFOURCC('4','D','S','P');
	LIVE_DWORD nTimeTick;    					//time
	LIVE_DWORD nFrameLength; 					//length of frame
	LIVE_DWORD nDataCodec;   					//encode type
	//video:mmioFOURCC('H','2','6','4');
	//audio:mmioFOURCC('A','D','T','S');
	LIVE_DWORD nFrameRate;   					//video:framerate
	//aduio:samplerate (default:44100)
	LIVE_DWORD nWidth;       					//video:width
	//audio:channel (default:2)
	LIVE_DWORD nHight;       					//video:height
	//audio:samplebit (default:16)
	LIVE_DWORD nColors;      					//video:colors
	//audio:bandwidth (default:64000)
	LIVE_DWORD dwSegment;						//package flag
	LIVE_DWORD dwFlags;							//video: I frame
	//audio:  reserve
	LIVE_DWORD dwPacketNumber; 					//packages serial number
	LIVE_DWORD nOthers;      					//reserve
} FRAMEHEAD;

typedef struct __MSGHEADER__ {
	unsigned short sLen;		//长度
	unsigned short sVer;		//版本
	unsigned short sMsgType;	//消息类型
	unsigned short sData;	//保留
} MsgHeader, *pMsgHeader;


typedef struct __RECISER__ {
	LIVE_SOCKET		tcp_sock;
	LIVE_SOCKET		udp_sock[LIVE_MAX_UDPLINK];
	int				userID;
	int 				login_ok[LIVE_MAX_UDPLINK];
	struct sockaddr_in UAddr[LIVE_MAX_UDPLINK];
} Recuser;

typedef struct __RECPLAYENV__ {
	unsigned int			usercnt;
	LIVE_SOCKET		 	server_sock;
	pthread_t			clthread[LIVE_MAX_RECUSER];
	Recuser				recUser[LIVE_MAX_RECUSER];
	pthread_mutex_t		index_m;
} RecplayEnv;



static char gLivePlayLibVersion[12] = LIVEPLAY_LIB_VERSION;


static int gLiveMaxUserNum = -1;
static LiveEDebugLevle gLiveDebug_Level = DL_NONE;


/*send data count*/
static unsigned int gnSentCount[LIVE_MAX_UDPLINK] = {0};

/*send audio count*/
static unsigned int gnAudioCount = 0;

/*send data packet*/
static unsigned char gszSendBuf[LIVE_MAX_UDPLINK][LIVE_MAX_PACKET];

/*send audio data max packets*/
static unsigned char gszAudioBuf[LIVE_MAX_PACKET];

static int liveplayTaskStatus = STOPED;

static RecplayEnv	recplayEnv;

unsigned int recLink[LIVE_MAX_RECUSER] = {0};

static pthread_t serverThid, udpThid;


/*set send timeout*/
static int SetSendTimeOut(LIVE_SOCKET sSocket, unsigned long time)
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
static int SetRecvTimeOut(LIVE_SOCKET sSocket, unsigned long time)
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

unsigned int getRecuserNum()
{
	return recplayEnv.usercnt;
}

static void initRecuserNum()
{
	recplayEnv.usercnt = 0;
}

static void addRecuserNum()
{
	recplayEnv.usercnt++;
}

static void clearRecuserNum()
{
	recplayEnv.usercnt--;
}

static void setRecSerSocket(int socket)
{
	recplayEnv.server_sock = socket;
}

static void setRecuserTcpSocket(int userIndex, int socket)
{
	recplayEnv.recUser[userIndex].tcp_sock = socket;
}

int getRecuserTcpSocket(int userIndex)
{
	return recplayEnv.recUser[userIndex].tcp_sock;
}

static void clearRecuserTcpSockt(int userIndex)
{
	recplayEnv.recUser[userIndex].tcp_sock = -1;
}

int getRecSerSocket()
{
	return recplayEnv.server_sock;
}

int getUdpSocket(int userIndex, int index)
{
	return recplayEnv.recUser[userIndex].udp_sock[index];
}

static int setUdpSocket(int userIndex, int index, int socket)
{
	recplayEnv.recUser[userIndex].udp_sock[index] = socket;
	return 0;
}

static void setUserLogin(int userIndex, int index)
{
	recplayEnv.recUser[userIndex].login_ok[index] = 1;
}

static void setUserLogout(int userIndex)
{
	int index;

	for(index = 0; index < LIVE_MAX_UDPLINK; index++)  {
		recplayEnv.recUser[userIndex].login_ok[index] = 0;
	}
}

int getUserLogStatus(int userIndex, int index)
{
	return recplayEnv.recUser[userIndex].login_ok[index];
}

struct sockaddr_in getUdpAddr(int userIndex, int index) {
	return recplayEnv.recUser[userIndex].UAddr[index];
}

int getRecuserIndex()
{
	pthread_mutex_lock(&recplayEnv.index_m);
	int index = 0;

	for(index = 0; index < gLiveMaxUserNum; index++) {
		if(!recLink[index]) {
			recLink[index] = 1;
			pthread_mutex_unlock(&recplayEnv.index_m);
			return index;
		}
	}

	pthread_mutex_unlock(&recplayEnv.index_m);
	printf("max recuser!!!\n");
	return -1;
}

static void removeRecuserIndex(int userIndex)
{
	pthread_mutex_lock(&recplayEnv.index_m);
	recLink[userIndex] = 0;
	recplayEnv.recUser[userIndex].userID = 0;
	recplayEnv.recUser[userIndex].tcp_sock = -1;
	pthread_mutex_unlock(&recplayEnv.index_m);

	return ;
}

int getRecUdpuserIndex(int userID)
{
	int userIndex = 0;

	for(userIndex = 0; userIndex < gLiveMaxUserNum; userIndex++) {
		fprintf(stderr, "userIndex = %d\n", userIndex);

		if(userID == recplayEnv.recUser[userIndex].userID) {
			fprintf(stderr, "userIndex = %d\n", userIndex);
			return userIndex;
		}
	}

	fprintf(stderr, "     userIndex = %d\n", userIndex);
	return -1;
}


/*Test Recplayer  UDP    Demo*/
static int RecPlayProcess(int *puserIndex)
{
	int nLen;
	MsgHeader header, *pheader, sd_header;
	unsigned char szData[1280];
	int fileflags;

	int userIndex = *puserIndex;

	if(userIndex >= gLiveMaxUserNum || userIndex < 0) {
		printf("Exit RecPlayProcess Thread!!!!\n");
		pthread_detach(pthread_self());
		return 0;
	}

	LIVE_SOCKET socket = getRecuserTcpSocket(userIndex);

	printf("enter live RecPlayProcess() function!!\n");
	SetSendTimeOut(socket, 10);
	SetRecvTimeOut(socket, 10);
	memset(&header, 0, sizeof(header));
	pheader = &header;
	sd_header.sLen = htons(sizeof(MsgHeader));
	sd_header.sVer = MSG_VER;
	sd_header.sMsgType = MSG_TYPE_PASSWORD;

	if((fileflags = fcntl(socket, F_GETFL, 0)) == -1) {
		printf("fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		return -1;
	}

	if(fcntl(socket, F_SETFL, fileflags & (~O_NONBLOCK)) == -1) {
		printf("fcntl F_SETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		return -1;
	}

	//	fcntl(socket, F_SETFL, O_NONBLOCK); 		// 是否为阻塞版本的socket

	while(liveplayTaskStatus == RUNNING) {
		memset(szData, 0, 1280);
		nLen = recv(socket, szData, HEAD_LEN, 0);

		if(nLen < HEAD_LEN || nLen == -1)	{
			printf("nLen < HEAD_LEN  errno = %d  nLen = %d\n", errno, nLen);
			goto EXITTHREAD;
		}

		memcpy(&header, szData, HEAD_LEN);

		pheader->sLen = ntohs(pheader->sLen);

		if(pheader->sLen - HEAD_LEN > 0) {
			nLen = recv(socket, szData + HEAD_LEN, pheader->sLen - HEAD_LEN, 0);

			if(nLen < pheader->sLen - HEAD_LEN)	{
				fprintf(stderr, "nLen < nMsgLen -HEAD_LEN\n");
				goto EXITTHREAD;
			}
		}



		switch(pheader->sMsgType) {
			case MSG_TYPE_PASSWORD:					// 判断密码
				if(!(strncmp(LIVE_CONNECT_KEY_1, (char *)(szData + HEAD_LEN), pheader->sLen - HEAD_LEN)) ||
				   !(strncmp(LIVE_CONNECT_KEY_2, (char *)(szData + HEAD_LEN), pheader->sLen - HEAD_LEN))) {
					int ln = strlen(LIVE_SERVER_VERSION) + strlen(LIVE_MINPLAYER_VERSION) + 4;
					char bver[40], buffer[100];
					int userID = 0;
					time_t timep;

					sprintf(bver, "%s %s", LIVE_SERVER_VERSION, LIVE_MINPLAYER_VERSION);
					sd_header.sData = 0; //密码正确
					sd_header.sLen = htons(sizeof(MsgHeader) + ln - 2);
					send(socket, &sd_header, sizeof(MsgHeader), 0);
					send(socket, bver, ln - 2, 0);
					sd_header.sLen = htons(sizeof(MsgHeader) + 4);
					sd_header.sVer = MSG_VER;
					sd_header.sMsgType = MSG_TYPE_ID;

					time(&timep);
					userID = (int)timep;
					fprintf(stderr, "------ userId = %x\n", userID);
					recplayEnv.recUser[userIndex].userID = userID;
					memcpy(buffer, &sd_header, sizeof(MsgHeader));
					memcpy(buffer + sizeof(MsgHeader), &userID, 4);
					send(socket, buffer, sizeof(MsgHeader) + 4, 0);
				} else {
					sd_header.sData = 1; //密码错误
					printf("passwd error!!!\n");
					send(socket, &sd_header, sizeof(MsgHeader), 0);
					goto EXITTHREAD;
				}

				break;

			case MSG_TYPE_HEART:
				DEBUG(DL_DEBUG, gLiveDebug_Level, "liveplay Heat  OK!!\n");
				break;

			case MSG_TYPE_USEINFO:
				DEBUG(DL_DEBUG, gLiveDebug_Level, "MSG_TYPE_USEINFO\n");
				break;

			case MSG_TYPE_ID:
				DEBUG(DL_DEBUG, gLiveDebug_Level, "MSG_TYPE_ID \n");
				break;

			case MSG_TYPE_BDPROXY:
				DEBUG(DL_DEBUG, gLiveDebug_Level, "MSG_TYPE_BDPROXY \n");
				break;

			case MSG_TYPE_CTRL: {
#if 0
				printf("MSG_TYPE_CTRL");
				unsigned char type, speed, num;
				type = *(szData + HEAD_LEN);
				speed = *(szData + HEAD_LEN + 1);
				num = *(szData + HEAD_LEN + 2);
				CameraControl((int)type, (int)speed);
#endif
			}
			break;

			default:
				DEBUG(DL_DEBUG, gLiveDebug_Level, "default Exit !!! pheader->sMsgType  = %d \n", pheader->sMsgType);
				break;
		}

	}

EXITTHREAD:
	close(socket);
	setUserLogout(userIndex);
	removeRecuserIndex(userIndex);
	clearRecuserNum();
	printf("(((((((((((((((((((((((((((((Exit RecPlayProcess Thread!!!!))))))))))))))))))))))))))))\n");
	pthread_detach(pthread_self());
	return 0;
}

/*Replay*/
static int LiveServerThread()
{
	struct sockaddr_in SAddr, CAddr;
	short port = LIVE_LISTEN_RECPLAY_PORT;
	int nLen = 0, result = 0, cnt = 0;
	LIVE_SOCKET sClientSocket = 0;
	LIVE_SOCKET sServerSocket = 0;
	int userIndex = 0;
	MsgHeader header;

	DEBUG(DL_DEBUG, gLiveDebug_Level, "************************** start recplayer live ***************************\n");
	bzero(&SAddr, sizeof(struct sockaddr_in));
	SAddr.sin_family = AF_INET;
	SAddr.sin_port = htons(port);
	SAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	initRecuserNum();

	sServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(sServerSocket < 0) {
		printf("create socket TCP stream!!\n");
		return -1;
	}

	DEBUG(DL_DEBUG, gLiveDebug_Level, "sServerSocket = %d\n", sServerSocket);
	setRecSerSocket(sServerSocket);
	pthread_mutex_init(&recplayEnv.index_m, NULL);

	int opt = 1;
	setsockopt(getRecSerSocket(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(getRecSerSocket(), (struct sockaddr *)&SAddr, sizeof(SAddr)) < 0) {
		printf("bind failed error msg:%s\n", strerror(errno));
		return -1;
	}

	if(listen(getRecSerSocket(), LIVE_LISTEN_MAX_NUM) < 0) {
		printf("listen error:%d,error msg: = %s", errno, strerror(errno));
		return -1;
	}

	//	fcntl(getRecSerSocket(), F_SETFL, O_NONBLOCK);     // 是否为阻塞版本的socket

	while(liveplayTaskStatus == RUNNING) {
		memset(&CAddr, 0, sizeof(struct sockaddr_in));
		nLen = sizeof(struct sockaddr_in);
		//		fprintf(stderr, "before accept ~!!\n");
		sClientSocket = accept(getRecSerSocket(), (void *)&CAddr, (LIVE_DWORD *)&nLen);

		//		fprintf(stderr, "after accept ~!!\n");
		if(sClientSocket > 0) {
			if(getRecuserNum() >= gLiveMaxUserNum) {	// 超过最大用户数
				DEBUG(DL_DEBUG, gLiveDebug_Level, "0000000000000000000000000000000000000000000000000000000000000\n");
				DEBUG(DL_DEBUG, gLiveDebug_Level, "live user count = %d\n", getRecuserNum());
				DEBUG(DL_DEBUG, gLiveDebug_Level, "0000000000000000000000000000000000000000000000000000000000000\n");
				memset(&header, 0, sizeof(header));
				header.sLen = htons(sizeof(MsgHeader));
				header.sVer = MSG_VER;
				header.sMsgType = MSG_TYPE_PASSWORD;
				header.sData = 3;
				send(sClientSocket, &header, sizeof(MsgHeader), 0);
				usleep(100000);
				close(sClientSocket);
				continue;
			}

			userIndex = getRecuserIndex();
			addRecuserNum();
			DEBUG(DL_DEBUG, gLiveDebug_Level, "0000000000000000000000000000000000000000000000000000000000000\n");
			DEBUG(DL_DEBUG, gLiveDebug_Level, "user count = %d\n", getRecuserNum());
			DEBUG(DL_DEBUG, gLiveDebug_Level, "userIndex = %d\n", userIndex);
			DEBUG(DL_DEBUG, gLiveDebug_Level, "0000000000000000000000000000000000000000000000000000000000000\n");

			setRecuserTcpSocket(userIndex, sClientSocket);
			result = pthread_create(&recplayEnv.clthread[cnt], NULL, (void *)RecPlayProcess, &userIndex);

			if(result < 0)	{
				close(sClientSocket);
				clearRecuserTcpSockt(userIndex);
				removeRecuserIndex(userIndex);
				clearRecuserNum();
				fprintf(stderr, "creat pthread ClientMsg error  = %d!, err msg = %s\n" , errno, strerror(errno));
				continue;
			} else if(result == 0) {
				printf("create live client server success!!!\n");
				usleep(100000);
			}
		} else {
			if(errno == EAGAIN) {
				usleep(100000);
			}
		}
	}


	close(getRecSerSocket());
	DEBUG(DL_DEBUG, gLiveDebug_Level, "((((((((((( ServerThread End !! ))))))))))))\n");
	liveplayTaskStatus = STOPED;
	pthread_detach(pthread_self());
	return 0;
}


/**/
static int LiveUDPThread()
{
	/*create UDP  port*/
	struct sockaddr_in SrvAddr ;
	int size;
	struct timeval tv;
	char buffer[20];
	int nLen;
	int cnt;
	int user;
	short port = LIVE_UDPSOCKET_PORT;
	LIVE_SOCKET	udpSocket = -1;
	int userIndex = -1;

	int userID = 0;
	int a, b, c, d;

	DEBUG(DL_DEBUG, gLiveDebug_Level, "Enter UDPThread()\n");

	for(cnt = 0; cnt < LIVE_MAX_UDPLINK; cnt++) {
		bzero(&SrvAddr, sizeof(struct sockaddr_in));
		SrvAddr.sin_family = AF_INET;
		SrvAddr.sin_port = htons(port + cnt);
		SrvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

		if(udpSocket < 0) {
			printf("UDPsend Socket create error:%d\n", errno);
			return -1;
		}

		for(user = 0; user < gLiveMaxUserNum; user++)  {
			setUdpSocket(user, cnt, udpSocket);
		}

		int opt = 1;
		setsockopt(getUdpSocket(0, cnt), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if(bind(getUdpSocket(0, cnt), (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0) {
			printf("bind error:%d, cnt = %d\n", errno, cnt);
			return -1;
		}

		tv.tv_sec = 10;
		tv.tv_usec = 10000;
		setsockopt(getUdpSocket(0, cnt), SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));

		int tvLen = sizeof(struct timeval);
		int ret = getsockopt(getUdpSocket(0, cnt), SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, (socklen_t *)&tvLen);

		if(ret < 0) {
			printf("getsockopt failed, err msg = %s\n", strerror(errno));
		}

		int n ;
		n = 220 * 1024;
		setsockopt(getUdpSocket(0, cnt), SOL_SOCKET, SO_SNDBUF, &n, sizeof(n));

	}

	cnt = 0;
	//	fcntl(getUdpSocket(0, 0), F_SETFL, O_NONBLOCK); 	// 是否为阻塞版本的socket

	while(liveplayTaskStatus == RUNNING) {
		size = sizeof(SrvAddr);
		nLen = recvfrom(getUdpSocket(0, 0), buffer, 12, 0, (struct sockaddr *)&SrvAddr, (socklen_t *)&size);

		if(nLen < 0)  {
			usleep(100000);
			continue;
		}

		if(buffer[0] == 0x7E && buffer[1] == 0x7E && buffer[2] == 0x7E
		   && buffer[3] == 0x7E) {
			a = (int)(buffer[11]) << 24;
			a &= 0xff000000;
			b = (int)(buffer[10]) << 16;
			b &= 0xff0000;
			c = (int)(buffer[9]) << 8;
			c &= 0xff00;
			d = (int)(buffer[8]);
			d &= 0xff;
			userID = a + b + c + d;
			fprintf(stderr, "a = %x, b = %x, c = %x, d = %x\n", a, b, c, d);
			DEBUG(DL_DEBUG, gLiveDebug_Level, "@@ receive userID = %x @@\n", userID);
			userIndex = getRecUdpuserIndex(userID);

			DEBUG(DL_DEBUG, gLiveDebug_Level, "get user Index = %d\n", userIndex);

			if(userIndex < 0) {
				printf("No this user!!!\n");		// 不合法用户
				continue;
			}

			for(cnt = 0; cnt < 2; cnt++) {
				setUserLogin(userIndex, cnt);
				memcpy(&recplayEnv.recUser[userIndex].UAddr[cnt], &SrvAddr, sizeof(struct sockaddr));
				recplayEnv.recUser[userIndex].UAddr[cnt].sin_port = htons(ntohs(SrvAddr.sin_port) + cnt);
				DEBUG(DL_DEBUG, gLiveDebug_Level, "sin_port = %d\n", ntohs(recplayEnv.recUser[userIndex].UAddr[cnt].sin_port));
				DEBUG(DL_DEBUG, gLiveDebug_Level, "IP addr : %s \n", inet_ntoa(recplayEnv.recUser[userIndex].UAddr[cnt].sin_addr));
			}

		}

		fprintf(stderr, "udp thread is running!!!\n");
	}

	for(cnt = 0; cnt < LIVE_MAX_UDPLINK; cnt++) {
		close(getUdpSocket(0, cnt));
	}

	liveplayTaskStatus = STOPED;
	pthread_detach(pthread_self());
	DEBUG(DL_DEBUG, gLiveDebug_Level, "((((((((((( UDPThread End !! ))))))))))))\n");
	return 0;
}

int LiveSendAudioDataToRecPlay(int nLen, unsigned char *pData, int nFlag,
                               unsigned char index, unsigned int timestmp, LiveAudioParam *pAsys)
{
	int nRet, nSent, nSendLen, nPacketCount, nMaxDataLen;
	FRAMEHEAD  DataFrame;


	if(liveplayTaskStatus == STOPED) {
		fprintf(stderr, "server is stop !!!\n");
		return -1;
	}

	bzero(&DataFrame, sizeof(FRAMEHEAD));
	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = LIVE_MAX_PACKET - sizeof(FRAMEHEAD) - HEAD_LEN;

	DataFrame.ID = 0x34363248;
	DataFrame.nFrameLength = nLen;

	DataFrame.nDataCodec = 0x53544441;					//"ADTS"
	DataFrame.nFrameRate = pAsys->SampleRate; 			//sample rate 1-----44.1KHz
	DataFrame.nWidth = pAsys->Channel;					//channel (default: 2)
	DataFrame.nHight = pAsys->SampleBit;				//sample bit (default: 16)
	DataFrame.nColors = pAsys->BitRate;					//bitrate  (default:64000)

	if(nFlag == 1) {
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}

	nSent = 0;
	DataFrame.nTimeTick = timestmp;

	while(nSent < nLen) {
		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;
			} else {
				DataFrame.dwSegment = 0;
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;
			} else {
				DataFrame.dwSegment = 1;
			}

			nPacketCount++;
		}

		DataFrame.dwPacketNumber = gnAudioCount++;
		memcpy(gszAudioBuf, &DataFrame, sizeof(FRAMEHEAD));
		memcpy(gszAudioBuf + sizeof(FRAMEHEAD), pData + nSent, nSendLen);

		int userIndex;
		int temp = -1;

		for(userIndex = 0; userIndex < gLiveMaxUserNum; userIndex++) {
			temp = getUserLogStatus(userIndex, 0);

			if(temp) {
				nRet = sendto(getUdpSocket(0, gLiveMaxUserNum - 1), gszAudioBuf,
				              nSendLen + sizeof(FRAMEHEAD), 0, (struct sockaddr *)&recplayEnv.recUser[userIndex].UAddr[0],
				              sizeof(struct sockaddr));

				if(nRet <= 0) {
					printf("liveplay audio sendto failed !!, errno = %d, err msg = %s\n", errno, strerror(errno));
				}
			}
		}

		nSent += nSendLen;
	}

	return 0;
}

int LiveSendVideoDataToRecPlay(int nLen, unsigned char *pData, int nFlag,
                               unsigned char index, unsigned int timestmp, LiveVideoParam *pVsys)
{
	int 				nRet 				= 0;
	int					nSent 				= 0;
	int 				nSendLen 			= 0;
	int 				nPacketCount 		= 0;
	int 				nMaxDataLen 		= 0;

	FRAMEHEAD  			DataFrame;

	if(liveplayTaskStatus == STOPED) {
		fprintf(stderr, "server is stop !!!\n");
		return -1;
	}

	bzero(&DataFrame, sizeof(FRAMEHEAD));
	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = LIVE_MAX_PACKET - sizeof(FRAMEHEAD);

	DataFrame.ID = 0x34363248;				// "H264"
	DataFrame.nFrameLength = nLen;

	DataFrame.nDataCodec = 0x34363248;		//"H264"

	DataFrame.nWidth = pVsys->nWidth;			//video width
	DataFrame.nHight = pVsys->nHight;			//video height

	if(nFlag == 1)	{	//if I frame
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}

	DataFrame.nTimeTick = timestmp;

	while(nSent < nLen) {
		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;    //start frame
			} else {
				DataFrame.dwSegment = 0;    //middle frame
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;    //first frame and last frame
			} else {
				DataFrame.dwSegment = 1;    //last frame
			}

			nPacketCount++;
		}

		DataFrame.dwPacketNumber = gnSentCount[index]++;

		memcpy(gszSendBuf[index], &DataFrame, sizeof(FRAMEHEAD));
		memcpy(gszSendBuf[index] + sizeof(FRAMEHEAD), pData + nSent, nSendLen);

		int userIndex;
		int temp = -1;

		for(userIndex = 0; userIndex < gLiveMaxUserNum; userIndex++) {
			temp = getUserLogStatus(userIndex, index);

			if(temp) {
				nRet = sendto(getUdpSocket(0, index), gszSendBuf[index], nSendLen + sizeof(FRAMEHEAD), 0,
				              (struct sockaddr *)&recplayEnv.recUser[userIndex].UAddr[index],
				              sizeof(struct sockaddr));

				if(nRet <= 0) {
					printf("liveplay sendto failed !!, errno = %d, err msg = %s\n", errno, strerror(errno));

					if(errno == EAGAIN) {
						usleep(10000);
						nRet = sendto(getUdpSocket(0, index), gszSendBuf[index], nSendLen + sizeof(FRAMEHEAD), 0,
						              (struct sockaddr *)&recplayEnv.recUser[userIndex].UAddr[index],
						              sizeof(struct sockaddr));
					}
				}
			}
		}

		nSent += nSendLen;
	}

	return 0;
}

int setLiveMaxUserNum(int usernum)
{
	if((usernum <= 0) || (usernum > LIVE_MAX_RECUSER)) {
		DEBUG(DL_ERROR, gLiveDebug_Level, "liveplay, invalid user num: usernum = %d\n", usernum);
		return -1;
	}

	if(gLiveMaxUserNum != -1) {
		DEBUG(DL_ERROR, gLiveDebug_Level, "liveplay, user num had been set, user num = %d\n", gLiveMaxUserNum);
	}

	gLiveMaxUserNum = usernum;
	return 0;
}

int getLiveMaxUserNum()
{
	return gLiveMaxUserNum;
}

int setLiveDebugLevel(LiveEDebugLevle DEBUG_LEVEL)
{
	gLiveDebug_Level = DEBUG_LEVEL;
	return 0;
}


int startLiveplayTask()
{
	int ret;

	//	pthread_attr_t			attr;
	//	struct sched_param		schedParam;

	if(liveplayTaskStatus == RUNNING) {			// 任务正在运行
		return -1;
	}

#if 0

	if(pthread_attr_init(&attr)) {
		fprintf(stderr, "Failed to initialize thread attrs\n");
		return -1;
	}

	if(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) {
		fprintf(stderr, "Failed to set schedule inheritance attribute\n");
		return -1;
	}

	if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
		fprintf(stderr, "Failed to set FIFO scheduling policy\n");
		return -1;
	}

	schedParam.sched_priority = 99;
	ret = pthread_attr_setschedparam(&attr, &schedParam);

	if(ret < 0) {
		fprintf(stderr, "Failed to set scheduler parameters\n");
		return -1;
	}

#endif

	if((gLiveMaxUserNum <= 0) || (gLiveMaxUserNum > LIVE_MAX_RECUSER)) {
		fprintf(stderr, "liveplay, max user num error, num = %d\n", gLiveMaxUserNum);
		return 0;
	}

	ret = pthread_create(&serverThid, NULL, (void *)LiveServerThread, (void *)NULL);

	if(ret < 0) {
		fprintf(stderr, "create LiveServerThread() failed\n");
		liveplayTaskStatus = STOPED;
		return -1;
	}

	ret = pthread_create(&udpThid, NULL, (void *)LiveUDPThread, (void *)NULL);

	if(ret < 0) {
		fprintf(stderr, "create LiveUDPThread() failed\n");
		liveplayTaskStatus = STOPED;
		return -1;
	}

	liveplayTaskStatus = RUNNING;
	return 0;
}

/*
int stopLiveplayTask()
{
	return 0;
}
*/

char *getLivePlayLibVersion()
{
	return (char *)gLivePlayLibVersion;
}

