

#include "commonTcp.h"
#include "commontrack.h"
#include "studentTcp.h"


#define STUDENT_PORT 5556



static int toTeacherSocket = -1;
extern stutrack_encode_info_t	g_stutrack_encode_info;
extern track_class_info_t	g_stuclass_info;

void setToTeacherSocket(int socket)
{
	toTeacherSocket = socket;
	return (void)0;
}

int getToTeacherSocket()
{
	return toTeacherSocket;
}

void toTeacherTcpCmd(void *param)
{
	char buf[256];
	int severSocket, addr;
	severSocket = getToTeacherSocket();
	MSGHEAD msg;
	MSGHEAD		*msg_header 	= NULL;
	int *value = 0;
	//	int lenth;

	while(1) {
		int len;
		severSocket = getToTeacherSocket();

		if(severSocket < 0) {
			setToTeacherSocket(-1);
			break;
		}

		len = recv(severSocket, &msg, sizeof(MSGHEAD), 0);

		if(len < 1) {
			perror("receive data:");
			close(severSocket);
			setToTeacherSocket(-1);
			break;
		}

		switch(msg.nMsg) {
			case MSG_FARCTRL:
				len = recv(severSocket, buf, msg.nLen - sizeof(MSGHEAD), 0);

				if(len < 1) {
					printf("recv teacher trace error\n");
					close(getToTeacherSocket());
					setToTeacherSocket(-1);
				}

				//if(msg.nLen-sizeof(MSGHEAD)<12)
				{
					addr = 2;
					FarCtrlCamera(1, buf, msg.nLen - sizeof(MSGHEAD));
				}
				//else
				{
					//addr=(int)(buf[8] | buf[9] << 8 | buf[10] << 16 | buf[11] << 24);
					//FarCtrlCamera(0,buf,msg.nLen-sizeof(MSGHEAD));
				}

				break;

			case MSG_SET_TRACK_TYPE:
				len = recv(severSocket, buf, msg.nLen - sizeof(MSGHEAD), 0);

				if(len < 1) {
					printf("recv teacher trace error\n");
					close(getToTeacherSocket());
					setToTeacherSocket(-1);
				}

				value = (int *)buf;

				if(*value == 1) {
					g_stutrack_encode_info.is_track = 1;
					gEduKit->osd_dspAlg_Link[0].create_params.stutrackCreateParams.StuTrackParms.dynamic_param.control_mode = AUTO_CONTROL;
					__stucall_preset_position(TRIGGER_POSITION42);
				} else if(*value == 0) {
					g_stutrack_encode_info.is_track = 0;
					gEduKit->osd_dspAlg_Link[0].create_params.stutrackCreateParams.StuTrackParms.dynamic_param.control_mode = MANUAL_CONTROL;
					__stucall_preset_position(TRIGGER_POSITION42);
				}

				printf("recv teacher MSG_SET_TRACK_TYPE value1 = %d\n", buf[0]);
				printf("recv teacher MSG_SET_TRACK_TYPE value1 = %d\n", buf[1]);
				printf("recv teacher MSG_SET_TRACK_TYPE value1 = %d\n", buf[2]);
				printf("recv teacher MSG_SET_TRACK_TYPE value1 = %d\n", buf[3]);
				printf("recv teacher MSG_SET_TRACK_TYPE value = %d\n", *value);
				break;

			default
					:
				break;
		}

	}
}

void SendClassInfotoTeacher()
{
	char buf[256];
	int severSocket;
	MSGHEAD		*msg_header 	= NULL;
	//int *value = 0;
	int lenth;
	int len;
	severSocket = getToTeacherSocket();

	if(severSocket < 0) {
		return;
	}

	msg_header = (MSGHEAD *)buf;
	len = sizeof(MSGHEAD) + sizeof(track_class_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = MSG_SEND_CLASSINFO;
	memcpy(buf + sizeof(MSGHEAD), &g_stuclass_info, sizeof(track_class_info_t));
	lenth = send(severSocket, buf, len, 0);

	if(lenth < 1) {
		printf("send classinfo error\n");
		// close(getToTeacherSocket());
		// setToTeacherSocket(-1);
	}


}

static void STUDENT_TRACETCP(void *param)
{
	pthread_t TCPCmd;
	int clientSocket;
	int listenSocket;
	int opt = 1;
	struct sockaddr_in clientAddr, srvAddr;
	setToTeacherSocket(-1);
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&srvAddr, sizeof(srvAddr));
	srvAddr.sin_family = AF_INET;
	srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvAddr.sin_port = htons(STUDENT_PORT);
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(listenSocket, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0) {
		perror("bind:");
	}

	if(listen(listenSocket, 10) < 0) {
		perror("listen:");
	}

	while(1) {
		int result, len, nSize = 0, nLen;
		len = sizeof(clientAddr);
		clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, (unsigned int *)&len);

		if((setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize,
		               sizeof(nSize))) == -1) {
			perror("setsockopt failed");
		}


		printf("yyyyyyyyyyyy accept socket:ok\n");
		nSize = 0;
		nLen = sizeof(nSize);
		result = getsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, &nSize , (DWORD *)&nLen);


		printf("getsockopt()  socket:%d	result:%d\n", clientSocket, result);

		nSize = 1;

		if(setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, &nSize , sizeof(nSize))) {
			printf("set IPPROTOT error\n");
		}

		if((getToTeacherSocket() >= 0) || (gEduKit->Start == 0)) {
			close(getToTeacherSocket());
			setToTeacherSocket(-1);
		}

		setToTeacherSocket(clientSocket);
		result = pthread_create(&TCPCmd, NULL, (void *)toTeacherTcpCmd, NULL);
	}

}


static void HEART(void *param)
{
	int serverSocket;
	MSGHEAD msg;
	msg.nLen = sizeof(MSGHEAD);
	msg.nMsg = MSG_TEACHER_HEART;

	while(1) {
		int len;
		serverSocket = getToTeacherSocket();

		if(serverSocket < 0) {
			printf("get teacher socket failed\n");
			sleep(4);
			continue;
		} else {
			len = send(serverSocket, &msg, sizeof(MSGHEAD), 0);

			if(len < 1) {
				printf("send data to teacher error\n");
			}
		}

		sleep(4);
	}
}

void SENDDATATOTEACHER(void *pParam)
{
	while(1) {
		//DEBUG(DL_DEBUG, "g_stutrack_encode_info.send_cmd = %d\n",g_stutrack_encode_info.send_cmd);
		if(SWITCH_TEATHER == g_stutrack_encode_info.send_cmd) {
			//DEBUG(DL_DEBUG, "g_stutrack_encode_info.send_cmd = %d\n",g_stutrack_encode_info.send_cmd);
			studentTracerMove(SWITCH_TEATHER);
			g_stutrack_encode_info.send_cmd = 0;
		} else if(SWITCH_STUDENTS == g_stutrack_encode_info.send_cmd) {
			//DEBUG(DL_DEBUG, "g_stutrack_encode_info.send_cmd = %d\n",g_stutrack_encode_info.send_cmd);
			studentTracerMove(SWITCH_STUDENTS);
			g_stutrack_encode_info.send_cmd = 0;
		}

		usleep(20000);

	}
}


void studentTrace(void *param)
{
	pthread_t rtmpid[2];
	int result;
	result = pthread_create(&rtmpid[0], NULL, (void *)STUDENT_TRACETCP, NULL);

	if(result < 0) {
		printf("create DSP1TCPTask() failed\n");
		//exit(0);
	}

	//result = pthread_create(&rtmpid[1], NULL, (void *)HEART, NULL);


	//if(result < 0)
	//{
	//	printf( "create DSP1TCPTask() failed\n");
	//exit(0);
	//}

	result = pthread_create(&rtmpid[1], NULL, (void *)SENDDATATOTEACHER, NULL);


	if(result < 0) {
		printf("create SENDDATATOTEACHER() failed\n");
		//exit(0);
	}


	while(1) {
		sleep(5);
	}
}


int studentTracerMove(unsigned char data)
{
	unsigned char sendData[256];
	int len, num;
	MSGHEAD msg;
	msg.nLen = 1 + sizeof(MSGHEAD);
	msg.nMsg = 48;
	memcpy(sendData, &msg, sizeof(MSGHEAD));
	num = sizeof(MSGHEAD);
	sendData[num] = data;

	if(getToTeacherSocket() < 0) {
		printf("studentTracerMove:get teacher socket failed\n");
		return -1;
	} else {
		len = send(getToTeacherSocket(), sendData, 1 + sizeof(MSGHEAD), 0);

		if(len < 1) {
			printf("send data to teacher error\n");
			return -1;
		}

	}

	return 0;
}

int sendCmdToTeacherTracer(unsigned char *data, int len, int cmd)
{
	unsigned char sendData[256];
	int lenth;
	MSGHEAD msg;
	msg.nLen = len + sizeof(MSGHEAD);
	msg.nMsg = cmd;
	memcpy(sendData, &msg, sizeof(MSGHEAD));
	memcpy(sendData + sizeof(MSGHEAD), data, len);

	if(getToTeacherSocket() < 0) {
		printf("get teacher socket failed\n");
		return -1;
	} else {
		lenth = send(getToTeacherSocket(), sendData, len + sizeof(MSGHEAD), 0);

		if(len < 1) {
			printf("send data to teacher error\n");
			return -1;
		}

	}

	return 0;
}



