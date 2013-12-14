#ifndef __PROCESS_XML_CMD__H
#define __PROCESS_XML_CMD__H

#define SUPPORT_XML_PROTOCOL
///#define SUPPORT_IP_MATRIX

#ifdef SUPPORT_XML_PROTOCOL

#include "xml_pro_common.h"

#define NEW_MSG_TCP_LOGIN 				31001
#define NEW_MSG_REQUEST_RATE			30011
#define NEW_MSG_SET_AUDIO				30006
#define NEW_MSG_GET_ROOM_INFO			30003
#define NEW_MSG_SET_QUALITY_INFO		30005
#define NEW_MSG_GET_VOLUME				30019
#define NEW_MSG_MUTE					30020
#define NEW_MSG_PIC_REVISE				30021
#define NEW_MSG_STATUS_REPORT			30032
#define NEW_REQUEST_IFRAME				30014
#define NEW_MSG_ENC_VER_REQ			30045
#define NEW_FIX_RESOLUTION				30010


#define     START          1
#define     STOP           0

#define MAX_CLIENT_NUM 			6

 /*client infomation*/
 typedef struct __NewClientData {
	 int bUsed;
	 int sSocket;
	 int bLogIn;
	 int sendAudioFlag;
	 int LowRateflag;
	 int sendVideoFlag;
	 int parseLowRateFlag;
	 int fixresolutionFlag;
	 int heartCount;
	 pthread_mutex_t heart_count_mutex;
	 unsigned long connect_time;
 } NewClientData;

 /*DSP client param*/
 typedef struct __NewDSPCliParam {
	 NewClientData cliDATA[MAX_CLIENT_NUM]; //client param infomation
 } NewDSPCliParam;


 NewDSPCliParam g_client_para[2];
 /*if use client*/
#define ISUSED_NEW(dsp, cli)				(g_client_para[dsp].cliDATA[cli].bUsed == TRUE)
 /*set client used*/
#define SETCLIUSED_NEW(dsp, cli,val)		(g_client_para[dsp].cliDATA[cli].bUsed=val)
 /*if login client*/
#define ISLOGIN_NEW(dsp, cli)			(g_client_para[dsp].cliDATA[cli].bLogIn == TRUE)
 /*set client login succeed or failed*/
#define SETCLILOGIN_NEW(dsp, cli,val)	(g_client_para[dsp].cliDATA[cli].bLogIn=val)
 /*get socket fd*/
#define GETSOCK_NEW(dsp, cli)			(g_client_para[dsp].cliDATA[cli].sSocket)
 /*set socket fd*/
#define SETSOCK_NEW(dsp, cli,val)		(g_client_para[dsp].cliDATA[cli].sSocket=val)
 /*current socket if valid*/
#define ISSOCK_NEW(dsp, cli)				(g_client_para[dsp].cliDATA[cli].sSocket != INVALID_SOCKET)
 /*set send audio data flag*/
#define SET_SEND_AUDIO_NEW(dsp, cli,val)  	(g_client_para[dsp].cliDATA[cli].sendAudioFlag=val)
 /*get send audio data flag */
#define GET_SEND_AUDIO_NEW(dsp, cli)  		(g_client_para[dsp].cliDATA[cli].sendAudioFlag)
 /*set send video data flag*/
#define SET_SEND_VIDEO_NEW(dsp, cli,val)  	(g_client_para[dsp].cliDATA[cli].sendVideoFlag=val)
 /*get send video data flag */
#define GET_SEND_VIDEO_NEW(dsp, cli)  		(g_client_para[dsp].cliDATA[cli].sendVideoFlag)

 /*set resize flag*/
#define SETLOWRATEFLAG_NEW(dsp, cli,val)  	(g_client_para[dsp].cliDATA[cli].LowRateflag=val)
 /*get resize flag*/
#define GETLOWRATEFLAG_NEW(dsp, cli)  		(g_client_para[dsp].cliDATA[cli].LowRateflag)

 /*get connect time*/
#define GETCONNECTTIME_NEW(dsp, cli)  		(g_client_para[dsp].cliDATA[cli].connect_time)
#define SETCONNECTTIME_NEW(dsp, cli,val)		(g_client_para[dsp].cliDATA[cli].connect_time = val)

#define SET_PARSE_LOW_RATE_FLAG(dsp, cli,val) (g_client_para[dsp].cliDATA[cli].parseLowRateFlag = val)
#define GET_PARSE_LOW_RATE_FLAG(dsp, cli) 	 (g_client_para[dsp].cliDATA[cli].parseLowRateFlag)

#define SET_FIX_RESOLUTION_FLAG(dsp, cli,val)  (g_client_para[dsp].cliDATA[cli].fixresolutionFlag = val)
#define GET_FIX_RESOLUTION_FLAG(dsp, cli)	 (g_client_para[dsp].cliDATA[cli].fixresolutionFlag)


#ifdef SUPPORT_IP_MATRIX
#define SET_PASSKEY_FLAG(dsp, cli,val)  (g_client_para[dsp].cliDATA[cli].passkey = val)
#define GET_PASSKEY_FLAG(dsp, cli)	 (g_client_para[dsp].cliDATA[cli].passkey)
#endif



 typedef struct MsgHeader //消息头
 {
		 unsigned short sLen;		 //
		 unsigned short sVer;		 //1
		 unsigned short sMsgCode;	 //    1
		 unsigned short sData;	 //保留
 }MsgHead;

 typedef struct __xml_msghead{
	 char msgcode[8];
	 char passkey[64];
 }xml_msghead;

 typedef struct user_login_info
 {
	 char username[32];
	 char password[8];
 }user_login_info;




 void *report_pthread_fxn(void *arg);

int process_xml_login_msg(int index, int pos,char *send_buf,user_login_info *login_info, int msgCode, char *passkey,char *user_id);
int process_xml_request_rate_msg(int index, int pos,char *send_buf,int msgCode,char *passkey,char *user_id,int *roomid);
int process_set_audio_cmd(int index, int pos,char *send_buf,AUDIO_PARAM *audio_info,int msgCode,char *passkey,char *user_id,int *roomid);
int process_get_room_info_cmd(int index, int pos,char *send_buf,int msgCode,char *passkey,char *user_id,int *roomid);
int process_mute_cmd(int index, int pos,char *send_buf,int msgCode,char *passkey,char *user_id,int *roomid);
int process_pic_adjust_cmd(int index, int pos,char *send_buf,PICTURE_ADJUST *info,int msgCode,char *passkey,char *user_id,int *roomid);
int process_xml_request_IFrame_msg(int index, int pos,char *send_buf,int msgCode,char*passkey,char *user_id,int *roomid);
#ifdef SUPPORT_IP_MATRIX
int  SendVideoDataToIpMatrixClient(int index, void *pData, int nLen);
int  SendAudioDataToIpMatrixClient(int index, void *pData, int nLen);
#endif //SUPPORT_IP_MATRIX
#endif



#endif
