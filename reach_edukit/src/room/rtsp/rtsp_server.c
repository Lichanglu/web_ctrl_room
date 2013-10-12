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
//#include "rtsp_porting_common.h"
#include "our_md5hl.h"

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
//#include "common.h"

#include "rtsp_client.h"
#include "rtsp_stream_deal.h"
#include "rtsp_server.h"
#include "rtsp_authentication.h"
#include "app_rtsp_center.h"
#include "nslog.h"

#ifdef USE_LINUX_PLATFORM
//#include "../log/log_common.h"
#endif
/*VLC connect information*/
static RTSPCliParam           		gRTSPCliPara[MAX_RTSP_CLIENT_NUM];
//thread id
static mid_plat_pthread_t 			rtsp_threadid[MAX_RTSP_CLIENT_NUM];
//have a rtsp client
static int                         	g_rtsp_flag = 0;
static char 						g_local_ip[64]  = {0};

static User_Authentication_t 		g_user_autentication;

static int							g_guest_user_num =0 ;

/*Resize Param table*/
typedef struct __THREAD_PARAM__ {
	int                 				nPos;
	mid_plat_socket                 	client_socket;
	mid_plat_sockaddr_in  		client_addr;
} Thread_Param;

enum{
	INVALID_CLIENT = -1,
	VLC_CLIENT = 0,
	QT_CLIENT,
	STB_TS_CLIENT,
	DEFAULT_CLIENT
};


//static int SendAnnounce(int socket, mid_plat_sockaddr_in Listen_addr, char *localip , STR_CLIENT_MSG *strMsg);
static int ResponseTeardown(mid_plat_socket sock, STR_CLIENT_MSG *strMsg);
static void rtsp_change_video_resolve(int roomid);
static int rtsp_get_client_timeout(void);

extern int mid_socket_set_active(int sockfd, int timeout, int count, int intvl);


extern void rtsp_porting_force_Iframe();
extern int rtsp_porting_server_need_stop();
extern int rtsp_porting_parse_url(char *url, char *localip, int *roomid,char *serv_url);
extern int rtsp_porting_get_ginfo(int *mult, char *ip, unsigned int *vport, unsigned int *aport, int stream_channel);
extern int rtsp_porting_close_client(int pos, void *client);
extern void *rtsp_porting_create_client(int pos, struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr, int stream_channel);

extern int rtsp_porting_get_sdp_describe(char *pSend, int len, int channel, char *rtpip, int vport, int aport);
extern int rtsp_porting_video_filter_sdp_info(unsigned char *pdata, int len,int width, int height, int id);
extern int rtsp_porting_audio_filter_sdp_info(unsigned char *pdata, int channel, int samplerate, int id);
extern int rtsp_stream_get_video_sdp_info(char *buff, int id);
extern unsigned int rtsp_stream_get_audio_sinfo(int id, char *buff, int *rate);
extern void rtsp_porting_sdp_init(void);
extern void rtsp_porting_sdp_destory(void);


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

	nslog(NS_INFO,"g_local_ip = #%s#\n", g_local_ip);

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
static unsigned int getruntimets(void)
{
	unsigned int msec;
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	msec = tp.tv_sec;
	msec = msec * 1000 + tp.tv_nsec / 1000000;
	//printf("mid_clock ultime=%u==%u\n",msec,tp.tv_sec);
	return msec;
}

// add by zhengyb

int set_rtsp_module_realm(char *reaml)
{
	if(reaml == NULL)
	{
		return -1;
	}
	if(SERV_REALM_MA_LEN < strlen(reaml) + 1)
	{
		nslog(NS_ERROR,"THE SETTING REAML IS TOO BIG %d <REAML_MAX_LEN : %d>\n",strlen(reaml) + 1,SERV_REALM_MA_LEN);
		return -1;
	}
	strcpy(g_user_autentication.realm,reaml);
	return 0;
	
}

int set_rtsp_module_userinfo(char *user_name,char *password)
{
	if(user_name == NULL || password == NULL)
	{
		return -1;
	}
	if(USER_NAME_MAX_LEN < strlen(user_name) + 1)
	{
		nslog(NS_ERROR,"THE SETTING REAML IS TOO BIG <USERNAME_MAX_LEN : %d>\n",strlen(user_name) + 1,USER_NAME_MAX_LEN);
		return -1;
	}
	if(USER_PASS_MAX_LEN < strlen(password) + 1)
	{
		nslog(NS_ERROR,"THE SETTING REAML IS TOO BIG <PASSWORD_MAX_LEN : %d>\n",strlen(password) + 1,USER_PASS_MAX_LEN);
		return -1;
	}
	strcpy(g_user_autentication.user_name,user_name);
	strcpy(g_user_autentication.user_pass,password);
	
	return 0;
}

int set_rtsp_module_permissions(int permissions)
{
	if(permissions == 0||permissions == 1)
	{
		g_user_autentication.permissions = permissions;
		return 0;
	}
	return -1;
}


char* strDup(char const* str) {
  if (str == NULL) return NULL;
  int len = strlen(str) + 1;
  char* copy = (char *)malloc(len);

  if (copy != NULL) {
    memcpy(copy, str, len);
  }
  return copy;
}

char* strDupSize(char const* str) {
  if (str == NULL) return NULL;
  int len = strlen(str) + 1;
  char* copy = (char *)malloc(len);

  return copy;
}


void get_reply_md5(char *reply_nonce_Buf)
{

  nslog(NS_ERROR,"get_reply_md5 \n");
  char seedData[100];
  unsigned long long ultime;
  struct timeval timestamp;

  gettimeofday(&timestamp, NULL);
  ultime = timestamp.tv_sec;
  ultime = ultime*1000 + timestamp.tv_usec;
  
  sprintf(seedData, "%lld", ultime);

  nslog(NS_ERROR,"time : %s\n",seedData);
  
  our_MD5Data((unsigned char*)(&seedData), sizeof(seedData), reply_nonce_Buf);

  nslog(NS_ERROR,"md5 : %s\n",reply_nonce_Buf);

}

void get_check_md5(char *check_nonce_buf,char* cmdName,Autenticatino_Info_t *aut_info,User_Authentication_t user_info)
{
	
	char ha1Buf[33] = {0};
	char ha2Buf[33] = {0};
	unsigned int ha1DataLen = 0;
	unsigned int ha2DataLen = 0;
	unsigned int last_len = 0;
//	char username[10] 	= "admin"; 
//	char password[10] 	= "admin"; 
//	char realm[10]    	= "zhengyb";
//	char cmd[10]		= "DESCRIBE";
//	char url[50]		= "rtsp://192.168.4.149:554/stream0/high";
//	char nonce[50]		= "d4d6c4e34ca63476c9e012b908a9a258";

	nslog(NS_ERROR,"user_name : %s-----------realm : %s -----------user_pass : %s\n",
		user_info.user_name,user_info.realm,user_info.user_pass);
	
	ha1DataLen = strlen(user_info.user_name) + 1 + strlen(user_info.realm) + 1 + strlen(user_info.user_pass);
	char* temp_1 = (char *)malloc(ha1DataLen +1);
	sprintf((char*)temp_1, "%s:%s:%s", user_info.user_name, user_info.realm, user_info.user_pass);
	our_MD5Data((const unsigned char *)temp_1, ha1DataLen, ha1Buf);

	nslog(NS_ERROR,"cmd : %s ------------- url : %s \n",
		cmdName,aut_info->url);
	ha2DataLen = strlen(cmdName) + 1 + strlen(aut_info->url);
	char* temp_2 = (char *)malloc(ha2DataLen +1);
	sprintf((char*)temp_2, "%s:%s", cmdName, aut_info->url);
	our_MD5Data((const unsigned char *)temp_2, ha2DataLen, ha2Buf);
	
	last_len = 32 + 1 + strlen(aut_info->nonce) + 1 + 32;
	char* temp_3 = (char *)malloc(last_len +1);
	sprintf((char*)temp_3, "%s:%s:%s",ha1Buf, aut_info->nonce, ha2Buf);
	
	
	our_MD5Data((const unsigned char *)temp_3, last_len, check_nonce_buf);
	

	free(temp_1);
	free(temp_2);
	free(temp_3);
	
}


static int parseAuthorizationHeader(char *buf,
												char ** username,
												char ** realm,
												char ** nonce, 
												char ** uri,
												char ** response)
{
  // Initialize the result parameters to default values:
 // username = realm = nonce = uri = response = NULL;

  // First, find "Authorization:"
  while (1) {
    if (*buf == '\0') return -1; // not found
    if (strncasecmp(buf, "Authorization: Digest ", 22) == 0){
		break;
    }
    ++buf;
  }

  // Then, run through each of the fields, looking for ones we handle:
  char const* fields = buf + 22;
  while (*fields == ' ') ++fields;
  char* parameter = strDupSize(fields);
  char* value = strDupSize(fields);
  
  while (1) {
    value[0] = '\0';
    if (sscanf(fields, "%[^=]=\"%[^\"]\"", parameter, value) != 2 &&
		sscanf(fields, "%[^=]=\"\"", parameter) != 1) {
	  nslog(NS_ERROR,"zhengzebiaodashi!\n");	
      break;
    }
    if (strcmp(parameter, "username") == 0) {
      *username = strDup(value);
    } else if (strcmp(parameter, "realm") == 0) {
    
      *realm = strDup(value);
    } else if (strcmp(parameter, "nonce") == 0) {
    
      *nonce = strDup(value);
    } else if (strcmp(parameter, "uri") == 0) {
    
     *uri = strDup(value);
    } else if (strcmp(parameter, "response") == 0) {
    
      *response = strDup(value);
    }

    fields += strlen(parameter) + 2 /*="*/ + strlen(value) + 1 /*"*/;
    while (*fields == ',' || *fields == ' ') ++fields;
        // skip over any separating ',' and ' ' chars
    if (*fields == '\0' || *fields == '\r' || *fields == '\n') break;
  }

  free(parameter);
  parameter =NULL;
  free(value);
  value = NULL;
  return 0;
}


int authenticationOK(int socket , char* cmdName,Autenticatino_Info_t *aut_info,User_Authentication_t user_info,char* fullRequestStr,int seqid)
{
	char * username = NULL;
	char * realm = NULL;
	char * nonce = NULL;
	char * uri = NULL; 
	char * response = NULL;
	int success = -1;
	char serv_nonce_buf[33] = {0};
	char fResponseBuffer[1024] = {0};
	int iSendDataLen = 0;
	
	do {
    // To authenticate, we first need to have a nonce set up
    // from a previous attempt:
    if(aut_info->url[0] == '\0' || aut_info->nonce[0] == '\0')
    {
    	nslog(NS_ERROR,"%p\n",aut_info->url);
    	break;
    }
	parseAuthorizationHeader(fullRequestStr,&username, &realm, &nonce, &uri, &response);
	// 2.识别收到信息是否缺少键值
    if (username == NULL
	|| realm == NULL || strcmp(realm, user_info.realm) != 0
	|| nonce == NULL || strcmp(nonce, aut_info->nonce) != 0
	|| uri == NULL || response == NULL) {
      break;
    }
	
	// 1. 获取RTSP服务器的MD5值用于检验
	
	get_check_md5(serv_nonce_buf,cmdName,aut_info,user_info);
	
//	nslog(NS_ERROR,"realm : %s  ------\n nonce : %s  -----\n uri:%s -----\n response :%s ------\nusername : %s\n",
//		realm,nonce,uri,response,username);
//	nslog(NS_ERROR,"user_info.realm : %s ----------- aut_info->nonce : %s\n",
//		user_info.realm,aut_info->nonce);

	// 3. 校验本地MD5值 与客户端请求中的MD5值

	nslog(NS_ERROR,"SERV_MD5: %s ---- CLIENT_MD5: %s \n",serv_nonce_buf,response);
	if(strcmp(serv_nonce_buf, response) == 0)
	{
		success = 0;
		break;
	}
  } while (0);
	if(username != NULL)
	{
		free(username);
		username = NULL;	
	}
	if(realm != NULL)
	{
		free(realm);
		realm = NULL;	
	}
	if(uri != NULL)
	{
		free(uri);
		uri = NULL;	
	}
	if(response != NULL)
	{
		free(response);
		response = NULL;	
	}
	if(nonce != NULL)
	{
		free(nonce);
		nonce = NULL;	
	}

	if (success == 0)
		return 0;
	get_reply_md5(aut_info->nonce);
 	sprintf((char*)fResponseBuffer,
   	"RTSP/1.0 401 Unauthorized\r\n"
   	"CSeq: %d\r\n"
   	"WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"\r\n\r\n",
   	seqid,
   	user_info.realm, aut_info->nonce);
	nslog(NS_ERROR,"oh!\n");

	iSendDataLen = strlen(fResponseBuffer);
	send(socket, fResponseBuffer, iSendDataLen, 0);
	nslog(NS_INFO,"\n%s\n", fResponseBuffer);
  return success;
	
	
}



/**************************************************************************************************
                                        为NALU_t结构体分配内存空间
**************************************************************************************************/
#if 0
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
#endif

/*********************************************************************************************************
                                        为NALU_t结构体释放内存空间
*********************************************************************************************************/
#if 0
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

#endif

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

// add zhengyb
static int verdict_guest_user_num(int *rtsp_user_num,int *guest_user_num)
{
	int temp_rtsp_user_num = RtspGetClientNum();
	int ret = -1;
	 
	if((g_guest_user_num + temp_rtsp_user_num) > MAX_RTSP_CLIENT_NUM - 1)
	{
		ret = -1;
	}
	else
	{
		ret =  0;
	}

	*rtsp_user_num = temp_rtsp_user_num;
	*guest_user_num = g_guest_user_num;
	return ret;
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

	nslog(NS_INFO,"g_strMsg.ucClientportLen=%d,g_strMsg.aucClientport=%c\n",strMsg->ucClientportLen,strMsg->aucClientport[strMsg->timeused][0]);
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
	nslog(NS_INFO,"\n%s\n", pSend);
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
	nslog(NS_INFO,"\n%s\n", pSend);
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
	nslog(NS_INFO,"\n%s\n", pSend);
	return iSendDataLen;
}


//describe authentication
static int rtsp_describe_authentication(char *buff, int len, char *localip, int *qtflag, int *roomid, char *filename, int *need_timeout,char *serv_url)
{
	if(buff == NULL || len < strlen("DESCRIBE RTSP/1.0")) {
		return -1;
	}

	char user[128] = {0};
	//	char passwd[128] = {0};
	char url[1024] = {0};
	int url_len = 0;
	char *temp = NULL;
	char temp_url_1[512] = {0}; 
//	int  temp_url_1_len = 0;
	char temp_url_2[512] = {0}; 
	int  temp_url_2_len = 0;

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

	nslog(NS_INFO,"*need_timeout = %d\n", *need_timeout);

	if(strncmp(buff, "DESCRIBE ", 9) != 0) {
		nslog(NS_ERROR,"ERROR!!!rtsp_describe_authentication 1.\n");
		return -1;
	}

	//关闭授权
#ifndef GDM_RTSP_SERVER

	if(0) {
		*roomid = 0;
		return 0;
	}

#endif
	temp = strstr(buff + 9, " RTSP/");

	if(temp == NULL) {
		nslog(NS_ERROR,"ERROR!!!rtsp_describe_authentication 2.\n");
		return -1;
	}

	url_len = temp - (buff + 9) ;

	if(url_len < strlen("rtsp://0.0.0.0")) {
		nslog(NS_ERROR,"ERROR!!!rtsp_describe_authentication 3.\n");
		return -1;
	}

	//	PRINTF("url_len = %d\n",url_len);
	strncpy(url, buff + 9, url_len);
	rtsp_porting_parse_url(url,localip,&id,serv_url);
	if(id == -1)
	{
		return -1;
	}
	*roomid = id;
	return 0;
	
	
#if 1
	strcpy(local_ip, localip);
	sprintf(url2,"rtsp://%s",local_ip);
	if(strncmp(url, url2, strlen(url2)) != 0) 
	{
		nslog(NS_ERROR,"ERROR!!!rtsp_describe_authentication 4.\n");
		return -1;
	}
	temp =strstr(url+strlen(url2),"/");
	if(temp == NULL || strlen(temp) == 1)
	{
		nslog(NS_ERROR,"roomid will set to 0\n");
		*roomid = 0;
		return 0;
	}
	if(strncmp(temp+1,"stream",strlen("stream")) == 0)
	{
		if(strncmp(temp+1,"stream0",strlen("stream0")) == 0)
		{
			if(strncmp(temp+1,"stream0/high",strlen("stream0/high")) == 0)
			{
				temp_url_2_len = strlen("stream0/high");
				*roomid = 0;
			}
			else if(strncmp(temp+1,"stream0/low",strlen("stream0/low") )== 0|| strncmp(temp+1,"stream0/LOW",strlen("stream0/LOW")) == 0)
			{
				temp_url_2_len = strlen("stream0/low");
				*roomid = 1;
			}
			else if(strcmp(temp+1,"stream0") == 0)
			{
				temp_url_2_len = strlen("stream0");
				*roomid = 0;
			}
		}
		else if(strncmp(temp+1,"stream1",strlen("stream1")) == 0)
		{
			if(strncmp(temp+1,"stream1/high",strlen("stream1/high")) == 0)
			{
				temp_url_2_len = strlen("stream1/high");
				*roomid = 2;
			}
			else if(strncmp(temp+1,"stream1/low",strlen("stream1/low")) == 0)
			{
				temp_url_2_len = strlen("stream1/low");
				*roomid = 3;
			}
			else if(strcmp(temp+1,"stream1") == 0)
			{
				temp_url_2_len = strlen("stream1");
				*roomid = 2;
			}
		}
		if(*roomid == -1)
		{
			return -1;
		}
		else
		{
			memcpy(temp_url_2,temp+1,temp_url_2_len);
			temp_url_2[temp_url_2_len] ='\0';
			sprintf(temp_url_1, "rtsp://%s:554/%s", local_ip,temp_url_2);
			serv_url = strDup(temp_url_1);
			nslog(NS_ERROR,"SHIRT -------------- %s\n",serv_url);
			return 0;
		}
	}
	else
	{
		nslog(NS_ERROR,"ERROR!!!rtsp_describe_authentication 4.\n");
		return -1;		
	}



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
		nslog(NS_ERROR,"ERROR!!!rtsp_describe_authentication 4.\n");
		return -1;
	}

//	if(*(url + strlen(url2) + 1) != '?') {
//		PRINTF("ERROR!!!rtsp_describe_authentication 5.\n");
//		return -1;
//	}

	//snprintf(room,2,"%s",url+strlen(url2) );
	memcpy(room, (url + strlen(url2)), 1);
	id = atoi(room);

	nslog(NS_INFO,"=%s=,room=#%s#,roomid = %d\n", url + strlen(url2) , room, id);

	//if(id < 0 || id > rtsp_stream_get_room_num() - 1) {
//	if(id < 0 || id > rtsp_stream_get_room_num() - 1) {
//		PRINTF("ERROR!!!rtsp_describe_authentication 6.\n");
//		return -1;
//	}
	if(id < 0)
		id = 0;
	*roomid = id;
	return 0;

	//	if((temp =strchr(url+strlen(url2) + 2,' ')) == NULL)
	//	{
	//		return -1;
	//	}
	//	strcpy(passwd,temp+1);
	//	*temp = '\0';
	strcpy(user, url + strlen(url2) + 2);
	nslog(NS_INFO,"user =$%s$\n", user);

#ifdef GDM_RTSP_SERVER
	ret =  app_rtsp_authentication_begin(user);

	if(ret  == 0) {
		strcpy(filename, user);
	}

#endif

	return ret ;
}


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
//	int				ret = 0;
	char				audio_config[16] = {0};
	int 				have_audio = 0;
//	char 				nonceBuf[33];
	char temp[50] = "d4d6c4e34ca63476c9e012b908a9a258";
	
	nslog(NS_INFO,"roomid=%d \n",roomid);
	if(roomid == -1)
	{
		sprintf(pSend, "RTSP/1.0   404   Stream Not Found\r\nCSeq: %d\r\n\r\n", strMsg->uiCseq);
		iSendDataLen = strlen(pSend);
		send(socket, pSend, iSendDataLen, 0);
		nslog(NS_INFO,"\n%s\n", pSend);
		return -1;
	}
	if(0)
	{
		#if 0
		"RTSP/1.0 401 Unauthorized\r\n"
	   	"CSeq: %s\r\n"
	   	"%s"
	   	"WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"\r\n\r\n",
		#endif
	
	//	setRealmAndRandomNonce(nonceBuf);
		
		sprintf(pSend,"RTSP/1.0 401 Unauthorized\r\nCseq: %d\r\nWWW-Authenticate: Digest realm=\"zhengyb\", nonce=\"%s\"\r\n\r\n",strMsg->uiCseq,temp);
		iSendDataLen = strlen(pSend);
		nslog(NS_INFO,"-----------------------------------------------------\n%s\n", pSend);
		send(socket, pSend, iSendDataLen, 0);
		return -1;
	}

	
	have_audio = rtsp_stream_get_audio_sinfo(roomid, audio_config, &audio_sample);

	if(have_audio < 0) {
		nslog(NS_ERROR,"Error\n");
	}

	int mult = 0;
	char ip[32] = {0};
	unsigned int vport = 0;
	unsigned int aport = 0;
	rtsp_porting_get_ginfo(&mult, ip, &vport, &aport,roomid);

	if(mult == 0 || strlen(ip) == 0) {
		strcpy(ip, localip);
	}

	sdpinfo_len =  rtsp_stream_get_video_sdp_info(sdpinfo, roomid);
#ifndef GDM_RTSP_SERVER

	if(rtsp_porting_server_need_stop() == 1) {
		sdpinfo_len = -1;
	}

#endif

	if(authencation == -1 || sdpinfo_len == -1 ) {
		nslog(NS_INFO,"authencation=%d,sdpinfo_len=%d\n", authencation, sdpinfo_len);
		sprintf(pSend, "RTSP/1.0   404   Stream Not Found\r\nCSeq: %d\r\n\r\n", strMsg->uiCseq);
		iSendDataLen = strlen(pSend);
		send(socket, pSend, iSendDataLen, 0);
		nslog(NS_INFO,"\n%s\n", pSend);
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
	if(have_audio >= 0)
	{
		sprintf(&pSend2[iSendDataLen2], "m=audio %u RTP/AVP 97\r\na=rtpmap:97 mpeg4-generic/%d/2\r\n", aport, audio_sample);
		iSendDataLen2 = strlen(pSend2);
		sprintf(&pSend2[iSendDataLen2], "a=fmtp:97 streamtype=5; mode=AAC-hbr; config=%s; SizeLength=13; IndexLength=3; IndexDeltaLength=3\r\na=control:rtsp://%s:%d/trackID=%d\r\n", audio_config, local_ip, listen_port, AUDIO_TRACKID);
		iSendDataLen2 = strlen(pSend2);
	}
	else
	{
			nslog(NS_ERROR,"ERROR,get audio sdp failed .\n");
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
	nslog(NS_INFO,"\n%s", pSend);
	send(socket, pSend2, iSendDataLen2, 0);
	nslog(NS_INFO,"\n%s\n", pSend2);
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
		nslog(NS_INFO,"\n%s\n", pSend);
		return -1;
	}

	sprintf(local_ip, "%s", localip);
	memcpy(client_ip, inet_ntoa(Listen_addr.sin_addr), 16);
	listen_port = RTSP_LISTEN_PORT;
	sprintf(pSend2, "v=0\r\no=- %ld %ld IN IP4 %s\r\n", getCurrentTimets(), getCurrentTimets(), local_ip);
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
	nslog(NS_INFO,"\n%s", pSend);
	send(socket, pSend2, iSendDataLen2, 0);
	nslog(NS_INFO,"\n%s\n", pSend2);
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
	nslog(NS_INFO,"\n");

	if(p == NULL) {
		return -1;
	}

	p = p + strlen("/trackID=") ;
	*(p + 1) = '\0';
	id = atoi(p);
	nslog(NS_INFO,"i find the trackid = %d\n", id);
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
	nslog(NS_INFO,"\n%s\n", pSend);
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
	nslog(NS_INFO,"\n%s\n", pSend);

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

static int ResponsePause(mid_plat_socket sock, STR_CLIENT_MSG *strMsg)
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
	nslog(NS_INFO,"\n%s\n", pSend);
	return iSendDataLen;
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
	nslog(NS_INFO,"\n%s\n", pSend);
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

static unsigned int g_keepactive_time = 7200; //s
int rtsp_set_keepactive_time(int s)
{
	if(s < 10) {
		s = 10;
	}

	if(s > 900000) {
		s = 900000;
	}

	g_keepactive_time = s;
	return 0;
}

static unsigned int rtsp_get_keepactive_time()
{
	return g_keepactive_time;
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
	rtsp_porting_close_client(nPos,client);
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
	char *client = NULL;
	//	PRINTF("g_rtsp_flag=%d\n",g_rtsp_flag);
	if(g_rtsp_flag != 1) {
		nslog(NS_INFO,"warnning ,the rtsp have no client .\n");
		return ;
	}

	for(cli = 0; cli < MAX_RTSP_CLIENT_NUM; cli++) {
		//vlc 也关闭
		if((roomid == GETRTSPROOMID(0,cli)) && (GETRTSPCLIENTTYPE(0, cli) == QT_CLIENT))
		//if(roomid == GETRTSPROOMID(0, cli)) 
		{
			SETRTSPCLIENTTYPE(0, cli, INVALID_CLIENT);

			client = GETRTSPCLIENT(0, cli);
			rtsp_porting_close_client(cli,client);
			client = NULL;
			SETRTSPCLIENT(0, cli, client);
		
			nslog(NS_INFO,"the video resolve is change ,the %d client is invalid,the room is %d.\n", cli, roomid);
		}
	}

	return ;
}


//*******************************************************************************************************************
/*add rtsp udp client*/
//*******************************************************************************************************************

static int RtspAddClient(mid_plat_sockaddr_in client_addr, int nPos, STR_CLIENT_MSG *strMsg)
{
	//int      UdpRtspVideoSocket = 0;
	//int      UdpRtspAudioSocket = 0;

	int 	   have_audio_socket = 0;
//	int      buf_num;
#ifndef GDM_RTSP_SERVER
//	struct timeval tv;
#endif
//	mid_plat_sockaddr_in temp_client_addr;
//	mid_plat_sockaddr_in temp_video_addr;
//	mid_plat_sockaddr_in temp_audio_addr;

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
	unsigned int vport = 0;
	unsigned int aport = 0;


	int stream_channel =roomid;
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

	nslog(NS_INFO,"video socket m=%d,port=%d,audio_port = %d\n", m, video_port,audio_port);

	//must have video port else will return -1;
	if(m == 0 || video_port == 0) {
		nslog(NS_INFO,"ERROR,the video socket port is error,m=%d,video_port= %d\n", m, video_port);
		return -1;
	}

	// bzero(&peeraddr,socklen);
	//memset(&peeraddr,0,socklen);

	rtsp_porting_get_ginfo(&mult, ip, &vport, &aport,stream_channel);

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
	nslog(NS_INFO,"aduio socket m=%d,port=%d\n", m, audio_port);

	for(n = 0; n < m; n++) {
		audio_port += (strMsg->aucClientport[1][n] - 0x30) * pow(10, (m - n - 1));
	}

	nslog(NS_INFO,"aduio socket m=%d,port=%d\n", m, audio_port);

	if(m == 0 || audio_port == 0) {
		nslog(NS_INFO,"WARNNING,the audio socket port is error,m=%d,audio_port= %d\n", m, audio_port);
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
	client = rtsp_porting_create_client(nPos,&video_addr, &audio_addr,stream_channel);

	if(client == NULL) {
		nslog(NS_ERROR,"create client is ERROR\n");
		return 0;
	}

	nslog(NS_INFO,"create client succes ,client=%p\n", client);

	SETRTSPROOMID(0, nPos, roomid);
	
	SETRTSPCLIENT(0, nPos, client);

	SETRTSPUDPREADY(0, nPos, 1);

	//i will save the socket addr int  static struct;
	SETRTSPADDR(0, nPos, client_addr);


	nslog(NS_INFO,"create rtsp client sock success\n");
	return 0;
}


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
//	char serv_response_md5[33] = {0};

//	User_Authentication_t temp_user_auth;	  //add zhengyb 用户密码变更后清掉所有用户.. 
//	temp_user_auth

	// add zhengyb
//	int debug_num = 0;

	Autenticatino_Info_t 		serv_aut_info;
	memset(serv_aut_info.nonce , 0 ,33);
	memset(serv_aut_info.url , 0 ,128);

	
	int 					client_flag = VLC_CLIENT;
	int ret = 0;
	long last_time = 0;
	long current_time = 0;

//	char return_str[100] = {0};

	int timeout = rtsp_get_client_timeout();
	int need_timeout = 0;

	char filename[256] = {0};

	memcpy(&Listen_addr, &rtsp_thread->client_addr, sizeof(mid_plat_sockaddr_in));
	nslog(NS_INFO,"----------have client--------num=%d-----------------------------\n", RtspGetClientNum());
	nslog(NS_INFO,"\n------------- the %d th vlc connecting -----------------\n\n", (nPos + 1));
	memset(pRecv, 0, sizeof(pRecv));
	rtsp_get_local_ip(local_ip, sizeof(local_ip));


	/*set timeout 3s*/
	mid_plat_set_socket_stimeout(sSocket, 3000);
	mid_plat_set_socket_rtimeout(sSocket, 1000);

	memset(&strMsg, 0, sizeof(STR_CLIENT_MSG));
	strMsg.roomid = -1;

	while(1) {
	#if 0
		if( )
		{
			nslog(NS_ERROR,"GO TO EXIT !\n");
			goto ExitClientMsg;
		}
	#endif		
		if(need_timeout == 1) {
			if(last_time == 0) {
				last_time = getruntimets();
			}

			current_time = getruntimets();

			//PRINTF("current_time =%u,last_time=%u\n",current_time,last_time);
			if(current_time - last_time > rtsp_get_client_timeout() * 2000) {
				nslog(NS_INFO,"WARNNING,the %d client is timeout ,the time is %d,i will close it.\n", nPos, current_time - last_time);
				goto ExitClientMsg;
			}
		}

		if(sSocket <= 0) {
			nslog(NS_ERROR,"ERROR ,the file = %s,the line =%d\n", __FILE__, __LINE__);
			goto ExitClientMsg;
		}

		if(GETRTSPCLIENTTYPE(0, nPos) == INVALID_CLIENT) {
			nslog(NS_INFO,"invailid client\n");
			tnum++;

			if(tnum > 3) {
				nslog(NS_INFO,"The %d client is invaild\n", nPos);
				goto ExitClientMsg;
			}
		}

		memset(pRecv_temp, 0, sizeof(pRecv_temp));
		
		resvlen_temp = recv(sSocket, pRecv_temp, TCP_BUFF_LEN, 0);

		if(resvlen_temp == 0) {
			nslog(NS_ERROR,"ERROR ,the file = %s,the line =%d\n", __FILE__, __LINE__);
			nslog(NS_ERROR,"the client is close the socket \n");
			goto ExitClientMsg;
		}

		if(resvlen_temp < 0) {
#ifdef GDM_RTSP_SERVER
			recv_errno = WSAGetLastError();
#endif

			if(errno == 11 || recv_errno == 10060) {
				continue;
			} else {
				nslog(NS_ERROR,"resvlen = %d,errno=%d,=%d\n", resvlen_temp, errno, recv_errno);
				nslog(NS_ERROR,"will goto ,the file = %s,the line =%d\n", __FILE__, __LINE__);
				goto ExitClientMsg;
			}
		}

		if(need_timeout == 1) {
			last_time = getruntimets();
		}

		if(strstr(pRecv_temp, "\r\n\r\n") == NULL) {
			//	PRINTF("Warnning,the msg is not complete,the msg =%s=\n",pRecv_temp);
			memset(pRecv, 0, sizeof(pRecv));
			memcpy(pRecv, pRecv_temp, resvlen_temp);
			continue;
		}

		strcat(pRecv, pRecv_temp);
		resvlen = strlen(pRecv);
		nslog(NS_INFO,"resvlen=%d,\n%s\n", resvlen, pRecv);
		msg_type = GetVlcMsgType(pRecv, resvlen, &strMsg);

		if(msg_type) {
			switch(msg_type) {
				case OPTION:
					ResponseOption(sSocket, &strMsg);
					break;

				case DESCRIBE:
					if(rtsp_describe_authentication(pRecv, resvlen, local_ip, &client_flag, &(strMsg.roomid), filename, &need_timeout,serv_aut_info.url)==0)
					{
						// add zhengyb   user authentication
						nslog(NS_INFO ,"USER_NAME : %s -------- USER_PASS : %s\n",g_user_autentication.user_name,g_user_autentication.user_pass);
						if(g_user_autentication.permissions == 1)
						{
							if(authenticationOK(sSocket,"DESCRIBE",&serv_aut_info,g_user_autentication,pRecv,strMsg.uiCseq) == -1)
							{
								break;
							}
						}
						if(client_flag == STB_TS_CLIENT)
						{
							ret = ResponseDescribeTs(sSocket, Listen_addr, local_ip, 0, &strMsg);
						} 
						else 
						{
							ret = ResponseDescribe(sSocket, Listen_addr, local_ip, 0, &strMsg);
							break;
						}
					} 
					else 
					{
						ret = ResponseDescribe(sSocket, Listen_addr, local_ip, -1, &strMsg);
						nslog(NS_ERROR,"ERROR,the file = %s,the line =%d\n", __FILE__, __LINE__);
						goto ExitClientMsg;

					}
					if(ret == -1) {
						goto ExitClientMsg;
					}
					nslog(NS_INFO,"rtsp client is %d\n",client_flag);
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
					//rtsp_porting_force_Iframe(strMsg.roomid);    //zl   ???

					if(0 == g_rtsp_flag) { //when fist vlc connect
						g_rtsp_flag = 1;
					}


					nslog(NS_INFO,"I success create the %d client ,the client type is %d\n", nPos, GETRTSPCLIENTTYPE(0, nPos));
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
					nslog(NS_ERROR,"ERROR ,goto exitclientmsg.the file = %s,the line =%d\n", __FILE__, __LINE__);
					goto ExitClientMsg;
					//break;
			}
		}

		memset(pRecv, 0, sizeof(pRecv));
	}

ExitClientMsg:
	nslog(NS_ERROR,"Exit RtspCastComMsg nPos = %d sSocket = %d\n", nPos, sSocket);

#ifdef GDM_RTSP_SERVER
	app_rtsp_authentication_stop(filename);
#endif

	RtspDelClient(nPos);   // 是不是关闭用户数据发送zhengyb?

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

	nslog(NS_INFO," Exit rtsp %d client Pthread!!client num =%d!!ts client num = %d\n", nPos, cnt, g_ts_client_num);
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
	unsigned int activetime = 7200;

	int 							rtsp_user_num = 0;
	int								guest_user_num = 0;
	
	//	PRINTF("Rtsp Task........GetPid():%d\n",getpid());
	RtspThreadInit();
RTSPSTARTRUN:
	nslog(NS_ERROR,"RtspTask start...\n");
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
		nslog(NS_ERROR,"ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		return -1;
	}

	setsockopt(RtspSerSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

	if(bind(RtspSerSock, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0) {
		nslog(NS_ERROR,"in RtspTask:bind error:%d,error msg: = %s\n", errno, strerror(errno));
		return -1;
	}

	if(listen(RtspSerSock, 2) < 0) {
		nslog(NS_ERROR,"listen error:%d,error msg: = %s", errno, strerror(errno));
		return -1;
	}

	while(1) {
#ifndef GDM_RTSP_SERVER
	if(rtsp_porting_server_need_stop() == 1)
	{
			usleep(400000);
			continue;
	}
#endif


	memset(&ClientAddr, 0, sizeof(mid_plat_sockaddr_in));
	nLen = sizeof(mid_plat_sockaddr_in);
	RtspCliSock = accept(RtspSerSock, (struct sockaddr *)&ClientAddr, (socklen_t *) &nLen);

	if(rtsp_porting_server_need_stop() == 1)
	{
		nslog(NS_ERROR,"ERROR,rtsp need stop\n");
		mid_plat_close_socket(RtspCliSock);
		mid_plat_sleep(500);
		continue;			
	}
	
	if(RtspCliSock > 0) 
	{
		// add zhengyb
		nPos = verdict_guest_user_num(&rtsp_user_num,&guest_user_num);
		nslog(NS_INFO,"verdict guest client number:< %d > rtsp client number :< %d > \n", guest_user_num,rtsp_user_num);
		if(-1 == nPos) 
		{
			fprintf(stderr, "ERROR: verdict guest client error  nPos=%d  \n",nPos);
			mid_plat_close_socket(RtspCliSock);
			mid_plat_sleep(500);
			continue;
		} 
	
		nPos = RtspGetNullClientIndex();
		nslog(NS_INFO,"new rtsp client number: %d \n", nPos);
		if(-1 == nPos ) 
		{
			fprintf(stderr, "ERROR: max client error  nPos=%d\n",nPos);
			mid_plat_close_socket(RtspCliSock);
			mid_plat_sleep(500);
			continue;
		} 
		else 
		{
			int nSize = 0;
			int result;
			nSize = 1;

			activetime = rtsp_get_keepactive_time();
			/*modify by zm  2012.04.26  */
			mid_socket_set_active(RtspCliSock, activetime, 3, 30);


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
			nslog(NS_INFO,"threadHandle == %d\n", threadHandle);

			//threadHandle will return 0,if it is failed
			if(threadHandle == 0) {
				RtspDelClient(nPos);
				mid_plat_close_socket(RtspCliSock);
				fprintf(stderr, "creat pthread ClientMsg error  = %d!\n" , errno);
				continue;
			}
	//	}

#else
			ret = pthread_create(&rtsp_threadid[nPos], NULL, (void *)RtspVlcContact, (void *)rtspthread);
			mid_plat_sleep(500);
			if(ret) {
				RtspDelClient(nPos);
				mid_plat_close_socket(RtspCliSock);
				fprintf(stderr, "creat pthread ClientMsg error  = %d!\n" , errno);
				continue;
			}
	//	}

#endif
			}
		}
		else 
		{
			//	if(errno == ECONNABORTED||errno == EAGAIN)  //软件原因中断
			if(errno == EAGAIN) {
				nslog(NS_ERROR,"rtsp errno =%d program again start!!!\n", errno);
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
	nslog(NS_ERROR,"close the gServSock server\n");

	if(RtspSerSock > 0)
	{
		nslog(NS_ERROR,"close gserv socket \n");
		mid_plat_close_socket(RtspSerSock);
	}

	RtspSerSock = 0;
	return 0;
}

static int g_init_flag = 0;
//add by zhangmin
void rtsp_module_init(void)
{
	if(g_init_flag != 0) {
		return ;
	}

	nslog(NS_ERROR,"now i init the rtsp module,build by zhangmin\n");
	g_init_flag = 1;
	//	app_rtsp_authentication_clear();
	//rtsp_stream_init(1,0);
	//	app_center_init("COM1");
	//	 _beginthreadex(NULL, 0, RtspTask, NULL, 0, NULL);
	pthread_t threadid = 0;
	pthread_create(&threadid, NULL, (void *)RtspTask, NULL);
	return ;
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
	char *client = NULL;
	for(cli = 0; cli < MAX_RTSP_CLIENT_NUM; cli++) {
		SETRTSPCLIENTTYPE(0, cli, INVALID_CLIENT);

		client = GETRTSPCLIENT(0, cli);
		rtsp_porting_close_client(cli,client);
		client = NULL;
		SETRTSPCLIENT(0, cli, client);
	}

	return ;
}



//is have client
int RtspClientIsNull(void)
{
	return g_rtsp_flag ;
}



#define DEFAULT_CLINET_TIMEOUT 60
static int rtsp_get_client_timeout()
{
	return rtsp_get_keepactive_time();
}

int rtsp_get_sdp_describe(char *pSend,int len,int roomid)
{
	int                     	iSendDataLen = 0, iSendDataLen2 = 0;
//	int                     	usStringLen = 0;
	int                     	listen_port;
	int                     	audio_sample = 48000;
	char                    	pSend2[1000];
	char                    	local_ip[16] = {0};
//	char                    	client_ip[16];

//	int roomid = 0;
	int 				sdpinfo_len = 0;
	char 			sdpinfo[1024] = {0};
//	int				ret = 0;
	char				audio_config[16] = {0};
	int 				a_ret = 0;
	
	char localip[64]= {0};
	
	rtsp_get_local_ip(localip,sizeof(localip));
	
	
	a_ret = rtsp_stream_get_audio_sinfo(roomid, audio_config, &audio_sample);

	if(a_ret < 0) {
		nslog(NS_ERROR,"Error\n");
	}

	int mult = 0;
	char ip[32] = {0};
	unsigned int vport = 0;
	unsigned int aport = 0;
	rtsp_porting_get_ginfo(&mult, ip, &vport, &aport,roomid);

	if(mult == 0 || strlen(ip) == 0) {
		strcpy(ip, localip);
	}

	sdpinfo_len =  rtsp_stream_get_video_sdp_info(sdpinfo, roomid);

	if(  sdpinfo_len == -1) {
	//	PRINTF("authencation=%d,sdpinfo_len=%d\n", authencation, sdpinfo_len);
		sprintf(pSend, "%s","RTSP/1.0   404   Stream Not Found\r\n \r\n");
		iSendDataLen = strlen(pSend);
	//	send(socket, pSend, iSendDataLen, 0);
		nslog(NS_INFO,"\n%s\n", pSend);
		return -1;
	}

	if(1)
	{
		sprintf(pSend, "%s","RTSP/1.0 401 Unauthorized\r\nServer: RRS 0.1\r\nCseq: 3\r\nWWW-Authenticate: Digest realm=\"Hipcam RealServer\",\r\nnonce=\"fa2ba288a4e39d75bcafa981ef5511b8\"");
		iSendDataLen = strlen(pSend);
		nslog(NS_INFO,"-----------------------------------------------------\n%s\n", pSend);
		return -1;
	}
	

	sprintf(local_ip, "%s", localip);
	listen_port = RTSP_LISTEN_PORT;
//	sprintf(pSend2, "c=IN IP4 %s\r\n", ip); //inet_ntoa(Listen_addr.sin_addr));
//	iSendDataLen2 = strlen(pSend2);

	sprintf(&pSend2[iSendDataLen2], "m=video %u RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\n", vport);
	iSendDataLen2 = strlen(pSend2);

	if(sdpinfo_len != -1) {
		sprintf(&pSend2[iSendDataLen2], "a=fmtp:96 packetization-mode=1;profile-level-id=428028;sprop-parameter-sets=%s\r\n", sdpinfo);
		iSendDataLen2 = strlen(pSend2);
	}

	sprintf(&pSend2[iSendDataLen2], "c=IN IP4 %s\r\n", ip); //inet_ntoa(Listen_addr.sin_addr));
	iSendDataLen2 = strlen(pSend2);
//	sprintf(&pSend2[iSendDataLen2], "a=control:rtsp://%s:%d/trackID=%d\r\n", local_ip, listen_port, VIDEO_TRACKID);
//	iSendDataLen2 = strlen(pSend2);

	//only the card0 have audio
	{
		sprintf(&pSend2[iSendDataLen2], "m=audio %u RTP/AVP 97\r\na=rtpmap:97 mpeg4-generic/%d/2\r\n", aport, audio_sample);
		iSendDataLen2 = strlen(pSend2);

		//ret = rtsp_stream_get_audio_sdp_info(audio_config);
#if 1

		if(a_ret < 0) {
			nslog(NS_ERROR,"ERROR,get audio sdp failed .\n");
		} else {
			sprintf(&pSend2[iSendDataLen2], "a=fmtp:97 streamtype=5; mode=AAC-hbr; config=%s; SizeLength=13; IndexLength=3; IndexDeltaLength=3\r\n", audio_config);//, local_ip, listen_port, AUDIO_TRACKID);
			iSendDataLen2 = strlen(pSend2);
			sprintf(&pSend2[iSendDataLen2], "c=IN IP4 %s\r\n", ip); //inet_ntoa(Listen_addr.sin_addr));
			iSendDataLen2 = strlen(pSend2);
		}

#endif
	}
	snprintf(pSend,len,"%s",pSend2);
	//send(socket, pSend2, iSendDataLen2, 0);
	nslog(NS_INFO,"\n%s\n", pSend2);
	return 0;

}


// add zhengyb
int rtsp_add_guest_user(int *rtsp_user_num,int *guset_user_num)
{
	int temp_rtsp_user_num = RtspGetClientNum();
	int ret = -1;
	
	if( (g_guest_user_num + temp_rtsp_user_num) < MAX_RTSP_CLIENT_NUM)
	{
		g_guest_user_num ++;
		ret = 0;
	}
	else
	{
		 ret = -1;
	}
		
	*rtsp_user_num = temp_rtsp_user_num;
	*guset_user_num = g_guest_user_num;
	return ret ;
}


int rtsp_del_guest_user(int *rtsp_user_num,int *guset_user_num)
{
	int temp_rtsp_user_num = RtspGetClientNum();
	g_guest_user_num -- ;

	*rtsp_user_num = temp_rtsp_user_num;
	*guset_user_num = g_guest_user_num;
	return 0;
}

int rtsp_del_guest_user_all(int *rtsp_user_num,int *guset_user_num)
{
	int temp_rtsp_user_num = RtspGetClientNum();
	g_guest_user_num = 0;

	*rtsp_user_num = temp_rtsp_user_num;
	*guset_user_num = g_guest_user_num;

	return 0;
}



