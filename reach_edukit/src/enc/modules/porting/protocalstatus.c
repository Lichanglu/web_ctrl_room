/*
****************************************************************************************************
协议状态保存模块
    该模块的功能是 保存协议的状态，包括设置的参数和端口，开启状态等，这样
    即便编码器重启，重启后不用再web上再次开启协议即可连接。
****************************************************************************************************
*/

#ifdef HAVE_RTSP_MODULE
#if 0
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#include "rtsp_server.h"

#include "protocalstatus.h"
#include "log_common.h"
#include "sysparam.h"



/********************************开启/关闭重启后状态有效代码*******************************************/
#ifdef DSS_ENC_1100_1200
#define SAVE_TS_STATUS
#endif

#define PROTOCALSTATUS "protocal.ini"

extern MultAddr                     s_MultAddr[MAX_PROTOCOL];
MultAddr                            g_MidMultAddr[MAX_PROTOCOL];
/*system/audio/video param table*/
extern params_table                   gSysParaT;
extern int                          g_UdpTSSocket[MAX_PROTOCOL];
extern struct sockaddr_in           g_UdpTSAddr[MAX_PROTOCOL];
extern int                          g_UdpaudioSocket;
extern struct sockaddr_in           g_UdpaudioAddr;
/*
*****************************************************************************************
initial protocol status,get status from ini in /opt/reach/protocal.ini
ts and rtp need save ip and port,rtmp and rtsp not need ,they get local ip and save it .
*****************************************************************************************
*/
#if 1
int protocal_test(int type, MultAddr *addr)
{
	if(type == 0) {
		TsSetUdp(g_UdpTSSocket[type], g_UdpTSAddr[type]);
		memcpy(&(s_MultAddr[type]), addr, sizeof(MultAddr));
	} else if(type == 1) {
		RtpSetUdp(g_UdpTSSocket[type], g_UdpTSAddr[type], g_UdpaudioSocket, g_UdpaudioAddr);
		memcpy(&(s_MultAddr[type]), addr, sizeof(MultAddr));
	} else if(type == 2) {
		memcpy(&(s_MultAddr[type]), addr, sizeof(MultAddr));
	}

	return 0;
}
#endif


void InitProtocalStatus()
{
	return 0;

#ifdef SAVE_TS_STATUS
	int ret = 0;
	char temp[512] = {0};
	struct in_addr          addr_ip;
	char config_file[64] = PROTOCALSTATUS;



	//-------------------------ts-----------------------------------------
	ret = ConfigGetKey(config_file, "ts", "ipaddr", temp);

	if(ret == 0) {
		memcpy(g_MidMultAddr[TS_STREAM].chIP, temp, 16);
		DEBUG(DL_FLOW, "ts ip : %s\n\n", g_MidMultAddr[TS_STREAM].chIP);
	}

	ret = ConfigGetKey(config_file, "ts", "port", temp);

	if(ret == 0) {
		g_MidMultAddr[TS_STREAM].nPort = atoi(temp);
		DEBUG(DL_FLOW, "ts port : %d\n\n", g_MidMultAddr[TS_STREAM].nPort);
	}

	ret = ConfigGetKey(config_file, "ts", "status", temp);

	if(ret == 0) {
		g_MidMultAddr[TS_STREAM].nStatus = atoi(temp);
		DEBUG(DL_FLOW, "ts onoff : %d\n\n", g_MidMultAddr[TS_STREAM].nStatus);
	}

	//-------------------------rtp-----------------------------------------
	ret = ConfigGetKey(config_file, "rtp", "ipaddr", temp);

	if(ret == 0) {
		memcpy(g_MidMultAddr[RTP_STREAM].chIP, temp, 16);
		DEBUG(DL_FLOW, "rtp ip : %s\n\n", g_MidMultAddr[RTP_STREAM].chIP);
	}

	ret = ConfigGetKey(config_file, "rtp", "port", temp);

	if(ret == 0) {
		g_MidMultAddr[RTP_STREAM].nPort = atoi(temp);
		DEBUG(DL_FLOW, "rtp port : %d\n\n", g_MidMultAddr[RTP_STREAM].nPort);
	}

	ret = ConfigGetKey(config_file, "rtp", "status", temp);

	if(ret == 0) {
		g_MidMultAddr[RTP_STREAM].nStatus = atoi(temp);
		DEBUG(DL_FLOW, "rtp onoff : %d\n\n", g_MidMultAddr[RTP_STREAM].nStatus);
	}


	//-------------------------rtsp-----------------------------------------
	memcpy(&addr_ip, &gSysParaT.sysPara.dwAddr, 4);
	memcpy(g_MidMultAddr[RTSP_STREAM].chIP, inet_ntoa(addr_ip), 16);
	memcpy(temp, inet_ntoa(addr_ip), 16);

	g_MidMultAddr[RTSP_STREAM].nPort = 554;
	ret = ConfigSetKey(config_file, "rtsp", "ipaddr", temp);

	if(ret == 0) {
		DEBUG(DL_FLOW, "rtsp ip : %s\n\n", g_MidMultAddr[RTSP_STREAM].chIP);
	}

	sprintf(temp, "%d", g_MidMultAddr[RTSP_STREAM].nPort);

	ret = ConfigSetKey(config_file, "rtsp", "port", temp);

	if(ret == 0) {
		DEBUG(DL_FLOW, "rtsp port : %d\n\n", g_MidMultAddr[RTSP_STREAM].nPort);
	}


	ret = ConfigGetKey(config_file, "rtsp", "status", temp);
	PRINTF("ret = %d,temp=%s\n", ret, temp);

	if(ret == 0) {
		g_MidMultAddr[RTSP_STREAM].nStatus = atoi(temp);
		DEBUG(DL_FLOW, "rtsp onoff : %d\n\n", g_MidMultAddr[RTSP_STREAM].nStatus);
	}


	//-------------------------rtmp-----------------------------------------
	memcpy(&addr_ip, &gSysParaT.sysPara.dwAddr, 4);
	memcpy(g_MidMultAddr[RTMP_STREAM].chIP, inet_ntoa(addr_ip), 16);
	memcpy(temp, inet_ntoa(addr_ip), 16);

	g_MidMultAddr[RTMP_STREAM].nPort = RTMP_LISTEN_PORT;
	ret = ConfigSetKey(config_file, "rtmp", "ipaddr", temp);

	if(ret == 0) {
		DEBUG(DL_FLOW, "rtmp ip : %s\n\n", g_MidMultAddr[RTMP_STREAM].chIP);
	}


	sprintf(temp, "%d", g_MidMultAddr[RTMP_STREAM].nPort);
	ret = ConfigSetKey(config_file, "rtmp", "port", temp);

	if(ret == 0) {
		DEBUG(DL_FLOW, "rtmp port : %d\n\n", g_MidMultAddr[RTMP_STREAM].nPort);
	}


	ret = ConfigGetKey(config_file, "rtmp", "status", temp);

	if(ret == 0) {
		g_MidMultAddr[RTMP_STREAM].nStatus = atoi(temp);
		DEBUG(DL_FLOW, "rtsp onoff : %d\n\n", g_MidMultAddr[RTMP_STREAM].nStatus);
	}

	//----------------------------------------------Build socket ------------------------------------------//
	if(MULT_START == g_MidMultAddr[TS_STREAM].nStatus) {
		DEBUG(DL_FLOW, "\n------------ TS START ------------\n\n");
		SetEStoTSMultPort(&g_MidMultAddr[TS_STREAM], TS_STREAM);
		TsSetUdp(g_UdpTSSocket[TS_STREAM], g_UdpTSAddr[TS_STREAM]);
		DEBUG(DL_FLOW, "------------ s_MultAddr[0].nStatus = %d ------------\n", g_MidMultAddr[0].nStatus);
	}

	if(MULT_START == g_MidMultAddr[RTP_STREAM].nStatus) {
		DEBUG(DL_FLOW, "\n------------ RTP START ------------\n\n");
		SetEStoTSMultPort(&g_MidMultAddr[RTP_STREAM], RTP_STREAM);
		RtpSetUdp(g_UdpTSSocket[RTP_STREAM], g_UdpTSAddr[RTP_STREAM], g_UdpaudioSocket, g_UdpaudioAddr);
		DEBUG(DL_FLOW, "------------ s_MultAddr[1].nStatus = %d ------------\n", g_MidMultAddr[RTP_STREAM].nStatus);

	}


	//------------------------------save params-----------------------------------------------//
	for(ret = 0; ret < MAX_PROTOCOL; ret++) {
		if(strlen(g_MidMultAddr[ret].chIP) != 0) {
			memcpy(&s_MultAddr[ret].chIP, &g_MidMultAddr[ret].chIP, sizeof(MultAddr));
		}

	}

#endif
	return ;
}



int SaveProtocalStatus(const MultAddr *port, int protocol)
{
#ifdef SAVE_TS_STATUS
	char temp[512] = {0};
	char config_file[64] = PROTOCALSTATUS;
	int ret = 0;
	memcpy(&g_MidMultAddr[protocol].chIP, port, sizeof(MultAddr));

	if(TS_STREAM == protocol) {
		memcpy(temp, port->chIP, 16);
		ret = ConfigSetKey(config_file, "ts", "ipaddr", temp);

		if(ret != 0) {
			DEBUG(DL_FLOW, "set ts ipaddr failed\n");
		}

		sprintf(temp, "%d", port->nPort);
		ret = ConfigSetKey(config_file, "ts", "port", temp);

		if(ret != 0) {
			DEBUG(DL_FLOW, "set ts port failed\n");
		}

		sprintf(temp, "%d", port->nStatus);
		ret = ConfigSetKey(config_file, "ts", "status", temp);

		if(ret != 0) {
			DEBUG(DL_FLOW, "set ts onoff status failed\n");
		}

	}

	if(RTP_STREAM == protocol) {
		memcpy(temp, port->chIP, 16);
		ret = ConfigSetKey(config_file, "rtp", "ipaddr", temp);

		if(ret != 0) {
			DEBUG(DL_FLOW, "set rtp ipaddr failed\n");
		}

		sprintf(temp, "%d", port->nPort);
		ret = ConfigSetKey(config_file, "rtp", "port", temp);

		if(ret != 0) {
			DEBUG(DL_FLOW, "set rtp port failed\n");
		}

		sprintf(temp, "%d", port->nStatus);
		ret = ConfigSetKey(config_file, "rtp", "status", temp);

		if(ret != 0) {
			DEBUG(DL_FLOW, "set ts onoff status failed\n");
		}
	}

	if(RTSP_STREAM == protocol) {
		sprintf(temp, "%d", port->nStatus);
		PRINTF("port->nStatus =%d,\n", port->nStatus);
		ret = ConfigSetKey(config_file, "rtsp", "status", temp);

		if(ret != 0) {
			DEBUG(DL_FLOW, "set rtsp onoff status failed\n");
		}
	}

	if(RTMP_STREAM == protocol) {
		sprintf(temp, "%d", port->nStatus);
		ret = ConfigSetKey(config_file, "rtmp", "status", temp);

		if(ret != 0) {
			DEBUG(DL_FLOW, "set rtmp onoff status failed\n");
		}
	}

#endif
	return 0;
}
#endif

#endif
