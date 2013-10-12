/****************************************************************************
*
* multcast_porting.c
*============================================================================
*  作用：  主要用于对接rtp ,ts,rtpts的发送函数，用于库函数调用
*
*
*============================================================================
*****************************************************************************/


#ifdef HAVE_RTSP_MODULE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

//#include "app_stream_output.h"


#include "mid_mutex.h"

#include "ts_build.h"
#include "rtp_build.h"
#include "rtp_ts_build.h"
#include "log_common.h"

#include "stream_output_struct.h"
#include "multicast_output_server.h"
#include "stream_output_client.h"

#include "nslog.h"


#define RTP_PAYLOAD_LEN 1400


extern int app_build_rtpts_data(unsigned int nLen, unsigned char *pData, int rtp_mtu, int frame, unsigned int nowtime, APP_MEDIA_INFO *info);

static void multicast_set_sbytes_process(int type, unsigned int bytes);
/*把一整帧数据转换的TS流根据mtu发送出去*/
static int ts_output_send(unsigned int nLen, unsigned char *pData, int mtu, int type, APP_MEDIA_INFO *info)
{

	int one_rtp_len = 0;

	if(mtu > RTP_PAYLOAD_LEN) {
		mtu = RTP_PAYLOAD_LEN;
	}

	one_rtp_len = 188 * (mtu / 188);

	unsigned 	int sendlen = 0;

	while(sendlen < nLen) {
		if((nLen - sendlen) < one_rtp_len) {
			stream_client_ts_send(pData + sendlen, (nLen - sendlen), type, info);
			//rtsp_ts_send_data((nLen - sendlen),pData+sendlen,frame);
			sendlen += (nLen - sendlen);
		} else {
			stream_client_ts_send(pData + sendlen, one_rtp_len, type, info);
			//rtsp_ts_send_data(one_rtp_len,pData+sendlen,frame);
			sendlen += one_rtp_len;
		}

		//frame --;
	}

	multicast_set_sbytes_process(TYPE_TS, nLen);

	return 0;
}



/*发送TS码流数据给socket*/
int ts_porting_senddata(int type, unsigned char *buff, int len, int begin, unsigned int  nowtime, void *info)
{

	APP_MEDIA_INFO *minfo = (APP_MEDIA_INFO *)info;
	int ts_mtu = 0;
	int rtp_ts_mtu = 0;
	int channel = 0;


	channel = minfo->stream_channel;

	ts_mtu = mult_get_direct_ts_mtu(channel);
	rtp_ts_mtu = mult_get_rtp_ts_mtu(channel);
	//获取ts码流
	//	app_multicast_ts_send_data(buff, len, type);
	//获取rtsp状态
	//stream_client_ts_send(buff, len, type);
	ts_output_send(len, buff, ts_mtu, type, (APP_MEDIA_INFO *)info);

	//if need ts over rtp
	if(mult_rtpts_get_active(channel) == 1) {
		app_build_rtpts_data(len, buff, rtp_ts_mtu, begin, nowtime , info);
	}

	return 0;
}

/*发送rtp码流数据给socket*/
/*type :video 2,audio 1*/
/*begin :begin 1,mid 2, end 3,*/
int rtp_porting_senddata(int type, unsigned char *buff, int len, int begin, void *info)
{
	//app_multicast_rtp_send_data(buff, len, type);
	//	if(type ==1)
	//		PRINTF("len=%d\n",len);
	//	return 0;

	//	nslog(NS_ERROR,"TYPE IS ---------------------- %d \n");
	if(type == 2 || type == 1) {

		stream_client_rtp_send(buff, len, type, info);
	}

	multicast_set_sbytes_process(TYPE_RTP, len);
	return 0;
}

/*发送rtp+ts的组合包*/
int rtpts_porting_senddata(unsigned char *buff, unsigned int len, int begin, void *info)
{
	stream_client_rtpts_send(buff, len, 2, (APP_MEDIA_INFO *)info);
	multicast_set_sbytes_process(TYPE_TS_OVER_RTP, len);
	return 0;
}



static unsigned int g_ts_vbytes = 0;
static unsigned int g_rtp_vbytes = 0;
static unsigned int g_rtpts_vbytes = 0;
static void multicast_set_sbytes_process(int type, unsigned int bytes)
{
	if(type == TYPE_TS) {
		g_ts_vbytes += bytes;
	} else if(type == TYPE_RTP) {
		g_rtp_vbytes += bytes;
	} else if(type == TYPE_TS_OVER_RTP) {
		g_rtpts_vbytes += bytes;
	}

	return ;
}
unsigned int multicast_get_sbytes_process(int type)
{
	unsigned int bytes = 0;

	if(type == TYPE_TS) {
		bytes = g_ts_vbytes;
		g_ts_vbytes = 0;
	} else if(type == TYPE_RTP) {
		bytes = g_rtp_vbytes;
		g_rtp_vbytes = 0;
	} else if(type == TYPE_TS_OVER_RTP) {
		bytes = g_rtpts_vbytes;
		g_rtpts_vbytes = 0;
	}

	return bytes;
}


int app_get_encode_rate(int *vrate, int *arate , int *quality)
{

	return 0;
}



#endif
