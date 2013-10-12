#ifdef HAVE_RTSP_MODULE



/**************************************************************
*  专门用于保存 ts_handle,rtp_handle等相关的handle
*
*
*								add by zm
*
*
*
****************************************************************/
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

#include "mid_timer.h"

#include "rtsp_server.h"
#include "rtp_build.h"
#include "rtp_ts_build.h"
#include "ts_build.h"

#include "stream_output_struct.h"
#include "stream_output_client.h"
#include "multicast_output_server.h"
#include "rtsp_output_server.h"
#include "traffic_shaping.h"
#include "app_stream_output.h"
#include "rtsp_porting_sdp_info.h"

#include "log_common.h"
#include "nslog.h"


#include "input_to_channel.h"
#include "app_protocol_handle.h"

/**direct ts handle**/

static void *g_ts_handle[STREAM_MAX_CHANNEL];

void app_build_ts_init()
{
	int id = 0;

	for(id = 0; id < STREAM_MAX_CHANNEL; id++) {
		if(g_ts_handle[id] != NULL) {
			PRINTF("Warnning,\n");
			return ;
		}

		g_ts_handle[id] = ts_build_init(0x82 + id, 0x49 + id);

		if(g_ts_handle[id] == NULL) {
			PRINTF("Errot,the g_ts_build_handle is NULL\n");
			return ;
		}

		PRINTF("Create ts channel %d handle =%p\n", id, g_ts_handle[id]);
	}


	return ;
}

/*==============================================================================
	函数: app_build_ts_video
	作用:  H264转换到ts
	输入:
	输出:
	作者:  modify by zm    2012.04.26
==============================================================================*/
void app_build_ts_vdata(unsigned char *buff, int len , unsigned int timestamp, APP_MEDIA_INFO *info)
{
	int id = info->stream_channel;

	if(id < 0 || id >= STREAM_MAX_CHANNEL) {
		PRINTF("Error,id %d is invaild\n", id);
		return ;
	}

	if(g_ts_handle[id] == NULL) {
		PRINTF("Error.\n");
		return ;
	}

	//PRINTF("g_ts_handle[%d]=%p,len=%d\n",id,g_ts_handle[id],len);
	ts_build_video_data(g_ts_handle[id], buff, len, timestamp, (void *)info);
	return;
}

/*==============================================================================
	函数: app_build_audio_ts
	作用:  AAC转换到ts
	输入:
	输出:
	作者:  modify by zm    2012.04.26
==============================================================================*/
void app_build_ts_adata(unsigned char *buff, int len, unsigned int timestamp, APP_MEDIA_INFO *info)
{

	int id = info->stream_channel;

	if(id < 0 || id >= STREAM_MAX_CHANNEL) {
		PRINTF("Error,id %d is invaild\n", id);
		return ;
	}



	if(g_ts_handle[id] == NULL) {
		PRINTF("Error.\n");
		return ;
	}

	//PRINTF("g_ts_handle[%d]=%p,len=%d\n",id,g_ts_handle[id],len);
	ts_build_audio_data(g_ts_handle[id], buff, len, timestamp, (void *)info);
	return;
}



/**direct rtp handle**/



static void *g_video_rtp_handle[STREAM_MAX_CHANNEL];
static void *g_audio_rtp_handle[STREAM_MAX_CHANNEL];


void app_build_rtp_init()
{
	int id = 0;

	//初始化video rtp handle
	for(id = 0; id < STREAM_MAX_CHANNEL; id++) {
		if(g_video_rtp_handle[id] != NULL) {
			PRINTF("Warnning,\n");
			return ;
		}

		g_video_rtp_handle[id] = rtp_build_init(0x11 + id, 96);

		if(g_video_rtp_handle[id] == NULL) {
			PRINTF("Errot,the g_ts_build_handle is NULL\n");
			return ;
		}

		PRINTF("Create video rtp channel %d handle =%p\n", id, g_video_rtp_handle[id]);
	}

	//初始化audio rtp handle
	for(id = 0; id < STREAM_MAX_CHANNEL; id++) {
		if(g_audio_rtp_handle[id] != NULL) {
			PRINTF("Warnning,\n");
			return ;
		}

		g_audio_rtp_handle[id] = rtp_build_init(0x31 + id, 97);

		if(g_audio_rtp_handle[id] == NULL) {
			PRINTF("Errot,the g_ts_build_handle is NULL\n");
			return ;
		}

		PRINTF("Create audio rtp channel %d handle =%p\n", id, g_audio_rtp_handle[id]);
	}


	return ;
}


int app_build_rtp_vdata(int len, unsigned char *buff, int IsIframe, int mtu, unsigned int rtp_time, APP_MEDIA_INFO *media_info)
{
	int id = media_info->stream_channel;
	int ret = -1;

	if(id < 0 || id >= STREAM_MAX_CHANNEL) {
		nslog(NS_ERROR, "Error,id %d is invaild\n", id);
		return -1;
	}

	if(g_video_rtp_handle[id] == NULL) {
		nslog(NS_ERROR, "Error.\n");
		return -1;
	}

	//	nslog(NS_ERROR,"g_audio_rtp_handle[%d] = %p,buflen=%d\n, buff %p ,IsIframe : %d ,mtu : %d ,rtp_time : %d , media_info : %p\n",
	//		id, g_video_rtp_handle[id],len,buff,IsIframe,mtu,rtp_time,media_info);
	ret = rtp_build_video_data(g_video_rtp_handle[id], len, buff, IsIframe, mtu, rtp_time, media_info);
	//	nslog(NS_ERROR,"RETURN IS %d \n",ret);
	return 0;
}


int app_build_rtp_adata(int len, unsigned char *buff, int samplerate, int mtu, unsigned int rtp_time, APP_MEDIA_INFO *media_info)
{
	int id = media_info->stream_channel;
	int ret = -1;

	if(id < 0 || id >= STREAM_MAX_CHANNEL) {
		nslog(NS_ERROR, "Error,id %d is invaild\n", id);
		return -1;
	}

	if(g_audio_rtp_handle[id] == NULL) {
		nslog(NS_ERROR, "Error.\n");
		return -1;
	}


	//	nslog(NS_ERROR,"g_audio_rtp_handle[%d] = %p,buflen=%d\n, buff %p ,samplerate : %d ,mtu : %d ,AAArtp_time : %u , media_info : %p\n",
	//		id, g_audio_rtp_handle[id],len,buff,samplerate,mtu,rtp_time,media_info);
	ret = rtp_build_audio_data(g_audio_rtp_handle[id], len, buff, samplerate, mtu, rtp_time, media_info);
	//	nslog(NS_ERROR,"RETURN IS  %d \n",ret);

	return 0;
}



static void *g_rtpts_handle[STREAM_MAX_CHANNEL];

void app_build_rtpts_init()
{
	int id = 0;

	for(id = 0; id < STREAM_MAX_CHANNEL; id++) {
		if(g_rtpts_handle[id] != NULL) {
			PRINTF("Warnning,\n");
			return ;
		}

		g_rtpts_handle[id] = rtp_build_init(0x41, 33);

		if(g_rtpts_handle[id] == NULL) {
			PRINTF("Errot,the g_ts_build_handle is NULL\n");
			return ;
		}

		PRINTF("Create rtp ts channel %d handle =%p\n", id, g_rtpts_handle[id]);
	}


	return ;
}

int app_build_rtpts_data(unsigned int nLen, unsigned char *pData, int rtp_mtu, int frame, unsigned int nowtime, APP_MEDIA_INFO *info)
{
	int id = info->stream_channel;

	if(id < 0 || id >= STREAM_MAX_CHANNEL) {
		PRINTF("Error,id %d is invaild\n", id);
		return -1;
	}

	if(g_rtpts_handle[id] == NULL) {
		PRINTF("Error.\n");
		return -1;
	}

	rtpts_build_data(g_rtpts_handle[id], nLen, pData, rtp_mtu, frame, nowtime, info);

	return 0;
}

#endif