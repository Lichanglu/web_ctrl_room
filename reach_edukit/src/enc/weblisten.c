#include "weblisten.h"
#include "common.h"
#include <unistd.h>
#include <sys/types.h>
#include <linux/rtc.h>

#include "middle_control.h"
#include "capture.h"
#include "app_signal.h"
#define MP_ENC_INDEX   1

#define r_printf printf

#define THREAD_SUCCESS      (void *) 0
#define THREAD_FAILURE      (void *) -1

#define MAX_LISTEN		 10
#define VERIFYLEN        (4*1024)
#define SYSCMDLEN		 (1024)



#define WEBSELECTTIMEOUT 6

static char plog_head[32];
typedef struct _HDB_MSGHEAD {
	short nLen;
	short nVer;
	char nMsg;
	char szTemp[3];
} HDB_MSGHEAD;




static int select_socket(int socket, int secode)
{
	fd_set fdsr;
	struct timeval tv;

	FD_ZERO(&fdsr);
	FD_SET(socket, &fdsr);

	tv.tv_sec  = secode;
	tv.tv_usec = 0;

	return select(socket + 1, &fdsr, NULL, NULL, &tv);
}

static int get_text_size(char *Data)
{
	int n_Len = strlen(Data);
	unsigned char *lpData = (unsigned char *)Data;
	int i = 0;
	int Sum = 0;

	for(i = 0; /**pStr != '\0'*/ i < n_Len ;) {
		if(*lpData <= 127) {
			i++;
			lpData++;
		} else {
			i += 2;
			lpData += 2;
		}

		Sum++;
	}

	return i;
}

int8_t *basename(int8_t *path)
{
	/* Ignore all the details above for now and make a quick and simple
	   implementaion here */
	int8_t *s1;
	int8_t *s2;

	s1 = (int8_t *)strrchr((char *)path, '/');
	s2 = (int8_t *)strrchr((char *)path, '\\');

	if(s1 && s2) {
		path = (s1 > s2 ? s1 : s2) + 1;
	} else if(s1) {
		path = s1 + 1;
	} else if(s2) {
		path = s2 + 1;
	}

	return path;
}

/*消息头打包*/
static void msgPacket(int identifier, unsigned char *data, webparsetype type, int len, int cmd, int ret)
{
	webMsgInfo  msghead;
	int32_t	cmdlen = sizeof(type);
	int32_t retlen = sizeof(ret);
	msghead.identifier = identifier;
	msghead.type = type;
	msghead.len = len;
	r_memcpy(data, &msghead, MSGINFOHEAD);
	r_memcpy(data + MSGINFOHEAD, &cmd, cmdlen);
	r_memcpy(data + MSGINFOHEAD + cmdlen, &ret, retlen);
	//printf("msghead.len=%d\n", msghead.len);
	return ;
}
typedef struct hv_info_ {
	int input;
	int hporch;
	int vporch;
	int save;
} hv_info_t;


static int set_hv_processed = 1;
static hv_info_t hv_info;
static void *web_process_hv_pthead(void *arg)
{
	capture_set_vga_hv(hv_info.hporch, hv_info.vporch);
	usleep(500000);
	set_hv_processed = 1;
	pthread_detach(pthread_self());
	pthread_exit(NULL);
	return arg;
}

static int web_create_process_hv_tsk(void)
{
	if(set_hv_processed) {
		printf("process set hv,set_hv_processed=%d\n", set_hv_processed);
		pthread_t p;

		if(pthread_create(&p, NULL, web_process_hv_pthead, (void *)NULL)) {
			printf("pthread_create failed !!!\n");
			return -1;
		}

		set_hv_processed = 0;
	} else {
		printf("miss set hv,set_hv_processed=%d\n", set_hv_processed);
	}

	return 0;
}

static int web_set_revise(int hv)
{
	memset(&hv_info, 0, sizeof(hv_info_t));
	short hporch = 0, vporch = 0;
	hporch = hv >> 16;
	vporch = hv & 0xffff;
	printf("hporch=%d,vporch=%d\n",  hporch, vporch);

	hv_info.hporch = hporch;
	hv_info.vporch = vporch;
	hv_info.save = 1;
	web_create_process_hv_tsk();
	return 0;
}

hdmiDisplayResolution_t	gHRes[3] = {
	{266, "1024", "768"},
	{9, "1280", "720"},
	{13, "1920", "1080"}
};
#define	HDMI_CFG_FILE	"/usr/local/reach/.config/hdmi.cfg"
int32_t gHDMIResIdx = 0;
static int32_t setHDMIResIdx(int32_t HDMIResIdx)
{
	if(HDMIResIdx < 0 || HDMIResIdx > 1) {
		gHDMIResIdx = 0;
	}

	gHDMIResIdx = HDMIResIdx;
	return HDMIResIdx;
}

static int32_t getHDMIResIdx(void)
{
	return gHDMIResIdx;
}

int32_t ReadHDMICfgFile(void)
{
	FILE *CfgFile = NULL;
	int8_t buf[2] = {0};

	int32_t nRet = -1;
	int32_t ResIdx = 1;

	if(access(HDMI_CFG_FILE, F_OK) != 0) {
		CfgFile = r_fopen((const int8_t *)HDMI_CFG_FILE, (const int8_t *)"w");

		if(NULL == CfgFile) {
			printf("Create hdmi.cfg Error!\n");
			return 0;
		}

		nRet = fwrite("1", 1, 1, CfgFile);

		if(nRet <= 0) {

			fclose(CfgFile);
			printf("fwrite hdmi.cfg Error!\n");
			return 0;
		}
	} else {
		CfgFile = r_fopen((const int8_t *)HDMI_CFG_FILE, (const int8_t *)"r");

		if(NULL == CfgFile) {
			printf("Open hdmi.cfg Error![%s]\n", strerror(errno));
			return 0;
		}


		nRet = fread(buf, 1, 1 , CfgFile);

		if(nRet <= 0) {

			fclose(CfgFile);
			printf("Read hdmi.cfg Error!\n");
			return 0;
		}

		ResIdx = atoi(buf);
		setHDMIResIdx(ResIdx - 1);
	}

	fclose(CfgFile);

	return ResIdx;
}

static int32_t WriteHDMICfgFile(void *buf, int buflen)
{
	static FILE *fp = NULL;

	if(NULL == fp) {
		fp =	fopen(HDMI_CFG_FILE, "w");

		if(NULL == fp) {
			printf("fopen  : %s", strerror(errno));
			return -1;
		}
	}

	printf("[WriteHDMICfgFile] buf : [%p], buflen : [%d] , fp : [%p]\n", buf, buflen, fp);
	fwrite(buf, buflen, 1, fp);
	fclose(fp);

	setHDMIResIdx(atoi(buf));
	return 0;
}


hdmiDisplayResolution_t *getHdmiDisplayResolution(void)
{
	return &gHRes[getHDMIResIdx()];
}

int getSignalInfo(SignalInfo_t *SignalInfo)
{
	int width = 0;
	int height = 0;
	memset(SignalInfo, 0, sizeof(SignalInfo_t));
	capture_get_input_hw(1, &width, &height);
	Device_VideoDecoderExternInforms info = {{0}, 0};
	GetVgaSourceState(&info);
	sprintf(SignalInfo->Signal, "%dx%d", width, height);
	SignalInfo->HPV = info.SignalHpv;
	SignalInfo->TMDS = info.SignalTmds;
	SignalInfo->RGB_YPRPR = info.SignalYPbPr;
	return 1;
}

int getEncInfo(EncInfo_t *enc_info)
{
	sprintf(enc_info->fpga_version, "0x%X", app_get_fpga_version());
	strcpy(enc_info->kernel_version, get_kernel_version());
}


int32_t midParseInt(int32_t identifier, int32_t fd, int8_t *data, int32_t len)
{
	int32_t recvlen;
	int32_t cmd = 0;
	int32_t actdata = 0;
	int32_t ret = 0;
	int32_t web_ret = SERVER_RET_OK;

	int32_t need_send = 0;


	int8_t senddata[1024] = {0};
	int32_t totallen = 0;
	int8_t actbuf[2] = {0};
	recvlen = r_recv(fd, data, len, 0);
	int32_t out = 0;
	printf("recvlen = %d\n", recvlen);

	if(recvlen < 0) {
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		need_send = 1;
		goto EXIT;
	}

	r_memcpy(&cmd, data, sizeof(int));
	r_memcpy(&actdata, data + sizeof(int), len - sizeof(int));

	printf("cmd = 0x%04x\n", cmd);

	switch(cmd) {

		case MSG_SET_HDMI_RES: {
			printf("MSG_SET_HDMI_RES:[%d]\n", actdata);
			sprintf(actbuf, "%d", actdata);
			web_ret = WriteHDMICfgFile(actbuf, sizeof(actbuf));
			need_send = 1;
		}
		break;

		case MSG_GET_HDMI_RES: {
			out = getHDMIResIdx() + 1;
			need_send = 1;
		}
		break;

		case MSG_REVISE_PICTURE: {
			web_set_revise(actdata);
			need_send = 1;
		}
		break;

		default:
			printf("unkonwn cmd = %04x\n", cmd);
			need_send = 1;
			web_ret = SERVER_RET_UNKOWN_CMD;
			break;
	}

	if(ret < 0) {
		web_ret = SERVER_RET_INVAID_PARM_VALUE;
	}

EXIT:

	if(need_send == 1) {
		totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + sizeof(out);
		msgPacket(identifier, (unsigned char *)senddata, INT_TYPE, totallen, cmd, web_ret);
		r_memcpy(senddata + (totallen - sizeof(out)), &out, sizeof(out));
		r_send(fd, senddata, totallen, 0);

		if(web_ret != SERVER_RET_OK) {
			printf("ERROR,the cmd =0x%x,ret= 0x%x\n", cmd, web_ret);
		}
	}

	return 0;
}

int web_get_signal_detail_info(char *out, int vallen)
{
	char tmpbuf[128] = {0};
	SignalInfo_t SignalInfo;
	int temp = getSignalInfo(&SignalInfo);

	//get signal info
	if(0 ==  temp) {
		printf("WARNING,get video signal_info failed!ModeID=%d\n", -1);
		sprintf(out, "Signal:\\t No Signal!\\n");
		memset(tmpbuf, 0, strlen(tmpbuf));
		sprintf(tmpbuf, "HPV:\\t  0\\n");
		strcat(out, tmpbuf);
		memset(tmpbuf, 0, strlen(tmpbuf));
		sprintf(tmpbuf, "TMDS:\\t  0\\n");
		strcat(out, tmpbuf);
		memset(tmpbuf, 0, strlen(tmpbuf));
		sprintf(tmpbuf, "VsyncF:\\t 0\\n");
		strcat(out, tmpbuf);
		memset(tmpbuf, 0, strlen(tmpbuf));
		sprintf(tmpbuf, "HDCP:\\t 0\\n");
		strcat(out, tmpbuf);
		memset(tmpbuf, 0, strlen(tmpbuf));
		sprintf(tmpbuf, "RGB_YPRPR: 0\\n");
		strcat(out, tmpbuf);

		vallen = strlen(out);
		return -1;
	}

	sprintf(out, "Signal:\\t  %s\\n", SignalInfo.Signal);
	memset(tmpbuf, 0, strlen(tmpbuf));
	sprintf(tmpbuf, "HPV:\\t  %d\\n", SignalInfo.HPV);
	strcat(out, tmpbuf);
	memset(tmpbuf, 0, strlen(tmpbuf));
	sprintf(tmpbuf, "TMDS:\\t  %d\\n", SignalInfo.TMDS);
	strcat(out, tmpbuf);
	memset(tmpbuf, 0, strlen(tmpbuf));
	sprintf(tmpbuf, "VsyncF:\\t  %d\\n", SignalInfo.VsyncF);
	strcat(out, tmpbuf);
	memset(tmpbuf, 0, strlen(tmpbuf));
	sprintf(tmpbuf, "HDCP:\\t  %d\\n", SignalInfo.HDCP);
	strcat(out, tmpbuf);
	memset(tmpbuf, 0, strlen(tmpbuf));
	sprintf(tmpbuf, "RGB_YPRPR: %d\\n", SignalInfo.RGB_YPRPR);
	strcat(out, tmpbuf);

	vallen = strlen(out);

	return 0;
}

void get_signal_info(char *out)
{
	char *sname = NULL;
	Device_VideoDecoderExternInforms info = {{0}, 0};
	GetVgaSourceState(&info);
	sname = get_signal_analog_name(info.ModeID);
	if(sname) {
		r_strcpy(out, sname);
	}
}

int32_t midParseString(int32_t identifier, int32_t fd, int8_t *data, int32_t len)
{
	int32_t recvlen;
	int32_t cmd = 0;
	int8_t actdata[4096] = {0};
	int32_t vallen = 0;

	int8_t senddata[1024] = {0};
	int32_t totallen = 0;

	int8_t  out[4096] = "unknown cmd.";
	int32_t web_ret = SERVER_RET_OK;
	int32_t need_send = 0;

	recvlen = r_recv(fd, data, len, 0);

	vallen = len - sizeof(int);

	if(recvlen < 0 || vallen > sizeof(actdata)) {
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		need_send = 1;
		goto EXIT;
	}

	r_memset(out, 0, 4096);
	r_memcpy(&cmd, data, sizeof(int));
	r_memcpy(actdata, data + sizeof(int), vallen);

	switch(cmd) {
		case MSG_SIGNALDETAILINFO: {
			need_send = 1;
			printf("--MSG_SIGNALDETAILINFO--\n ");
			web_get_signal_detail_info(out, len);
			break;
		}
		case MSG_GETSIGNALINFO: {
			need_send = 1;
			get_signal_info(out);

			printf("--MSG_GETSIGNALINFO--[%s]\n ", out);
			break;
		}
		default:
			printf("Warnning,the cmd %d is UNKOWN\n", cmd);
			need_send = 1;
			break;
	}

EXIT:

	if(need_send == 1) {
		totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + r_strlen(out);
		msgPacket(identifier, (unsigned char *)senddata, STRING_TYPE, totallen, cmd, web_ret);
		r_memcpy(senddata + (totallen - r_strlen(out)), out, r_strlen(out));
		r_send(fd, senddata, totallen, 0);

		if(web_ret != SERVER_RET_OK) {
			printf("ERROR,the cmd =0x%x,ret= 0x%x\n", cmd, web_ret);
		}
	}

	return 0;
}

int32_t midParseStruct(int32_t identifier, int32_t fd, int8_t *data, int32_t len)
{
	int32_t recvlen;
	int32_t cmd = 0;
	//int8_t actualdata[4096] = {0};
	char out[4096] = {0};
	int32_t  vallen = 0;
	int32_t  status = 0;

	int8_t senddata[5120] = {0};
	int32_t totallen = 0;
	int32_t ret = 0;
	int channel = 0;
	int32_t web_ret =   SERVER_RET_OK;
	int32_t need_send = 0;

	int32_t subcmd = 0;
	recvlen = r_recv(fd, data, len, 0);

	if(recvlen < 0) {
		printf("recv failed,errno = %d,error message:%s \n", errno, strerror(errno));
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		status = -1;
		goto EXIT;
	}

	//vallen = len - sizeof(int);

	r_memset(out, 0, 4096);
	r_memcpy(&cmd, data, sizeof(int));
	//r_memcpy(out, data + sizeof(int), recvlen - sizeof(int));
	vallen = recvlen - sizeof(int);

	printf("-----> recv msgtype[%x]\n", cmd);

	switch(cmd) {

		case MSG_GET_ENC_INFO:
			getEncInfo(out);
			EncInfo_t *enc_info = (EncInfo_t *)out;
			printf("[midParseStruct] kernel_version:[%s] fpga_version:[%s]\n", enc_info->kernel_version, enc_info->fpga_version);
			need_send = 1;

			break;


		default:
			need_send = 1;
			web_ret = SERVER_RET_UNKOWN_CMD;
			break;
	}

EXIT:
	system("sync");

	if(need_send == 1) {
		totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + vallen;
		msgPacket(identifier, (unsigned char *)senddata, STRING_TYPE, totallen, cmd, web_ret);
		r_memcpy(senddata + (totallen - vallen), out, vallen);

		if(web_ret != SERVER_RET_OK) {
			printf("ERROR, the cmd = 0x%x, ret = 0x%x\n", subcmd, web_ret);
		}

		int ret = r_send(fd, senddata, totallen, 0);

		if(ret != totallen) {
			printf("SEND_ERROR, the cmd = 0x%x", subcmd);
		}

	}

	printf("GOD IS OVER!\n");
	return status;
}



void *weblisten(void *arg)
{
	printf("weblisten edukit-sd start\n");
	void                   *status              = 0;
	int32_t 					listenSocket  		= 0 , flags = 0;
	struct sockaddr_in 		addr;
	int32_t len, client_sock, opt = 1;
	struct sockaddr_in client_addr;
	webMsgInfo		webinfo;
	ssize_t			recvlen;
	int index = *(int *)arg;
	int8_t  data[5120] = {0};


	len = sizeof(struct sockaddr_in);
	r_memset(&client_addr, 0, len);
	listenSocket =	r_socket(PF_INET, SOCK_STREAM, 0);

	if(listenSocket < 1) {
		status  = THREAD_FAILURE;
		return status;
	}

	r_memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family =       AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = r_htons(ENCODESERVER_PORT + index);

	r_setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(r_bind(listenSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0) {
		printf("[weblistenThrFxn] bind failed,errno = %d,error message:%s \n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status;
	}

	if(-1 == r_listen(listenSocket, MAX_LISTEN)) {
		printf("listen failed,errno = %d,error message:%s \n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status;
	}


	if((flags = fcntl(listenSocket, F_GETFL, 0)) == -1) {
		printf("fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status ;
	}

	if(fcntl(listenSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
		printf("fcntl F_SETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status ;
	}


	while(1) {
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(listenSocket, &rfds);

		//接收recv_buf 复位为空!
		r_select(FD_SETSIZE, &rfds , NULL , NULL , NULL);
		client_sock = r_accept(listenSocket, (struct sockaddr *)&client_addr, (socklen_t *)&len);

		if(0 > client_sock) {
			if(errno == ECONNABORTED || errno == EAGAIN) {
				//usleep(20000);
				continue;
			}

			printf("weblisten thread Function errno  = %d\n", errno);
			status  = THREAD_FAILURE;
			return status;
		}

		r_memset(&webinfo, 0, sizeof(webinfo));
		recvlen = r_recv(client_sock, &webinfo, sizeof(webinfo), 0);

		if(recvlen < 1) {
			printf("recv failed,errno = %d,error message:%s,client_sock = %d\n", errno, strerror(errno), client_sock);
			status  = THREAD_FAILURE;
			return status;
		}

		if(webinfo.identifier != WEB_IDENTIFIER) {
			printf("id  error,client_sock = %d\n", client_sock);
			status  = THREAD_FAILURE;
			return status;
		}

		len = webinfo.len - sizeof(webinfo);

		printf("----> web deal begin =%d %d\n", webinfo.type, len);

		switch(webinfo.type) {
			case INT_TYPE:
				midParseInt(webinfo.identifier, client_sock, data, len);
				break;

			case STRING_TYPE:
				midParseString(webinfo.identifier, client_sock, data, len);
				break;

			case STRUCT_TYPE:
				midParseStruct(webinfo.identifier, client_sock, data, len);
				break;

			default:
				break;
		}

		printf("----> web deal end =%d\n", webinfo.type);
		shutdown(client_sock, SHUT_RDWR);
		r_close(client_sock);
	}


	r_close(listenSocket);
	printf("Web listen Thread Function Exit!!\n");
	return status;
}



int32_t enc_app_weblisten_init(void *arg)
{
	pthread_t           webListen;

	if(r_pthread_create(&webListen, NULL, weblisten, (void *)arg)) {
		printf("Failed to create web listen thread\n");
		return -1;
	}

	printf("app_weblisten_init\n");
	//	r_pthread_join(webListen,NULL);
	return 0;
}

