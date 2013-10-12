/*============================================================================
*  作用： 主要用于设置输出的socket,addr，用于发送逻辑
*   涉及到mult ,rtsp创建 发送clinet
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

#include "mid_mutex.h"
#include "mid_socket.h"
#include "stream_output_struct.h"
#include "ts_build.h"
#include "log_common.h"
#include "app_protocol.h"

#include "stream_output_client.h"

#include "input_to_channel.h"
#include "nslog.h"
static int mult_set_active(int type, int status, int channel);
static Stream_Output_Client  g_stream_client[OUTPUT_SERVER_MAX_NUM];

#if 0
static int write_long_tcp_data(FILE *fd, void *buf, int32_t buflen)
{
	int total_recv = 0;
	int recv_len = 0;

	while(total_recv < buflen) {
		recv_len = write(buf, buflen, 1, fd);

		if(recv_len < 0) {
			nslog(NS_ERROR, "write  error recv_len = %d, errno = %d", recv_len, errno);
		}

		total_recv = recv_len * buflen;

		if(total_recv = buflen) {
			break;
		}
	}

	return total_recv;
}

#endif

/*init all stream client*/
int stream_client_init()
{
	int i = 0;

	for(i = 0; i < OUTPUT_SERVER_MAX_NUM; i++) {
		memset(&(g_stream_client[i]), 0, sizeof(Stream_Output_Client));
		g_stream_client[i].c_used = NOTUSED;
		g_stream_client[i].num = i;
	}

	return 0;
}

Stream_Output_Client *stream_client_get_free()
{
	int i = 0;

	for(i = 0; i < OUTPUT_SERVER_MAX_NUM; i++) {
		if(g_stream_client[i].c_used == NOTUSED) {
			//	*num =i;
			return &(g_stream_client[i]);
		}
	}

	return NULL;
}


int stream_client_open(struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr, int type, Stream_Output_Client *client, int stream_channel)
{
	if(client == NULL || stream_channel >= STREAM_MAX_CHANNEL) {
		PRINTF("Error\n");
		return -1;
	}

	int video_socket = 0;
	int audio_socket = 0;
	int have_audio = 0;
	int num = 0;
	int width, height, sample;
	int ret = 0;

	client->c_used = NOTUSED;

	if(client->video_socket > 0) {
		close(client->video_socket);
	}

	if(client->audio_socket > 0) {
		close(client->audio_socket);
	}

	num = client->num;
	memset(client, 0, sizeof(Stream_Output_Client));
	client->num = num;

	if(audio_addr->sin_port != 0) {
		have_audio = 1;
	}


	//ts only have one addr
	//if(type == TYPE_TS)
	//{
	//	have_audio = 0;
	//}

	//假定video audio必须不同的port
	video_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if(video_socket < 0) {
		PRINTF("ERROR,video socket is error\n");
		return -1;
	}

	memcpy(&(client->video_addr), video_addr, sizeof(struct sockaddr_in));

	mid_socket_set_sendtimeout(video_socket, 500);
	mid_socket_set_sendbuf(video_socket, 1024 * 1024);

	client->video_socket = video_socket;

	if(have_audio) {
		audio_socket = socket(AF_INET, SOCK_DGRAM, 0);

		if(audio_socket < 0) {
			PRINTF("ERROR,audio socket is error\n");
			return -1;
		}

		memcpy(&(client->audio_addr), audio_addr, sizeof(struct sockaddr_in));
		client->audio_socket = audio_socket;

		mid_socket_set_sendtimeout(audio_socket, 500);
		mid_socket_set_sendbuf(audio_socket, 1024 * 1024);

	}

	client->type = type;

	//add socket video and audio info
	ret = app_get_media_info(&width, &height, &sample, 0);

	if(ret == -1) {
		client->media_status = 0;
	} else {
		client->width = width;
		client->height = height;
		client->samplerate = sample;
		client->media_status = 1;
	}

	client->c_used = ISUSED;
	client->usenum ++;

	if(client->usenum == 1) {
		client->video_send = 0;
	}

	client->stream_channel = stream_channel;


	mult_set_active(client->type, +1, stream_channel);


	PRINTF("client->usenum = %d\n", client->usenum);
	//	client->send_status = S_ACTIVE;
	//设置 ttl ,qos
	//设置是否开启流量整形
	//stream_server_set_tc(config->num);
	PRINTF("client create is ok,the client is %d,stream_channel= %d\n", client->num, client->stream_channel);
	return 0;
}

int stream_client_close(Stream_Output_Client *client)
{
	if(client == NULL) {
		PRINTF("Error\n");
		return -1;
	}

	mult_set_active(client->type, -1, client->stream_channel);

	PRINTF("\n");
	//client->type = type;
	client->c_used = NOTUSED;
	client->usenum = 0;
	client->width = 0;
	client->height = 0;
	client->samplerate = 0;
	client->media_status = 0;
	client->video_send = 0;
	client->stream_channel = -1;


	return 0;
}

#if 1
int stream_client_set(Stream_Output_Client *client, int ttl, int tos)
{
	if(client == NULL) {
		PRINTF("Error\n");
		return -1;
	}

	char dst_ip[32] = {0};
	int video_socket = client->video_socket;
	int audio_socket = client->audio_socket;

	mid_socket_set_tos(video_socket, tos);
	mid_socket_set_tos(audio_socket, tos);

	inet_ntop(AF_INET, &(client->video_addr.sin_addr), dst_ip, 16);

	if(mid_ip_is_multicast(dst_ip) == 1) {
		mid_socket_set_ttl(video_socket,  ttl, 1);
		mid_socket_set_ttl(audio_socket,  ttl, 1);
	} else {
		mid_socket_set_ttl(video_socket, ttl, 0);
		mid_socket_set_ttl(audio_socket, ttl, 0);
	}

	return 0;

}
#endif

int stream_client_ts_send(unsigned char *buff, int len, int type, APP_MEDIA_INFO *info)
{
	int ret = 0;
	int i = 0;
	int width, height, sample;
	width = height = sample = 0;
	Stream_Output_Client *client = NULL;
	int stream_channel = info->stream_channel;

	for(i = 0 ; i < OUTPUT_SERVER_MAX_NUM ; i++) {
		client = &(g_stream_client[i]);

		//PRINTF("g_stream_client[%d].c_used =%d,type=%d\n",i,g_stream_client[i].c_used,g_stream_client[i].type );
		if(g_stream_client[i].c_used == ISUSED && g_stream_client[i].type == TYPE_TS
		   && g_stream_client[i].stream_channel == stream_channel) {
			//			{
			//				if(client->send_status == 0)
			//				{
			//					//i must init the stream info.
			//					ret = app_get_media_info(&width,&height,&sample,0);
			//					if(ret == -1)
			//					{
			//						client->media_status = 0;
			//					}
			//					else
			//					{
			//						client->width =width;
			//						client->height = height;
			//						client->samplerate =sample;
			//						client->media_status = 1;
			//					}
			//
			//				}
			//			}

			if(type == 2) { //video
				//	if(client->media_status ==1 && client->width == info->width && client->height == info->height)
				ret = sendto(client->video_socket, buff, len, 0, (struct sockaddr *) & (client->video_addr), sizeof(struct sockaddr_in));
			} else {
				if(client->audio_socket > 0) {
					//PRINTF("len=%d\n",len);
					//	if(client->media_status ==1 && client->samplerate == info->sample)

					//	FILE *fp = NULL;
					//	fp = fopen("/var/log/live_data_debug", "a");
					//	if (fp == NULL) {
					//		printf("NS_ERROR, fopen error");
					//	}
					//	write_long_tcp_data(fp, (channel_info.m_data + MSG_HEAD_LEN + FH_LEN), (channel_info.m_data_len - FH_LEN -MSG_HEAD_LEN));
					//	fclose(fp);


					ret = sendto(client->audio_socket, buff, len, 0, (struct sockaddr *) & (client->audio_addr), sizeof(struct sockaddr_in));

				} else {
					ret = len;
				}
			}

			if(ret != len) {
				PRINTF("vsock=%d,asock=%d,type = %d,buf  = %p,len=%d=sendto=%d\n", client->video_socket, client->audio_socket, type, buff, len, ret);
			}
		}

		//PRINTF("vsock=%d,asock=%d,type = %d,buf  = %p,len=%d=sendto=%d\n", client->video_socket, client->audio_socket, type, buff, len, ret);

	}

	return 0;
}

int stream_client_rtp_send(unsigned char *buff, int len, int type, APP_MEDIA_INFO *info)
{
	int ret = 0;
	int i = 0;
	//int width, height, sample;
	Stream_Output_Client *client = NULL;
	int stream_channel = info->stream_channel;

	for(i = 0 ; i < OUTPUT_SERVER_MAX_NUM ; i++) {
		client = &(g_stream_client[i]);

		//PRINTF("type=%d,the %d client ,have channel =%d,stream=%d\n",type,i,g_stream_client[i].stream_channel,stream_channel);
		if(g_stream_client[i].c_used == ISUSED && g_stream_client[i].type == TYPE_RTP
		   && g_stream_client[i].stream_channel == stream_channel) {

			//			{
			//				if(client->send_status == 0)
			//				{
			//					//i must init the stream info.
			//					ret = app_get_media_info(&width,&height,&sample,0);
			//					if(ret == -1)
			//					{
			//						client->media_status = 0;
			//					}
			//					else
			//					{
			//						client->width =width;
			//						client->height = height;
			//						client->samplerate =sample;
			//						client->media_status = 1;
			//					}
			//
			//				}
			//			}

			if(type == 2) {
				if(client->video_send < 50) {
					client->video_send++;
				}

				//	if(client->media_status ==1 && client->width == info->width && client->height == info->height)

				ret =  sendto(client->video_socket, buff, len, 0, (struct sockaddr *) & (client->video_addr), sizeof(struct sockaddr_in));
			} else {

				//	nslog(NS_ERROR,"i=%d,client=%p,type=%d,aport = %d,vport=%d,socket=%d,len=%d\n", i, client, type, ntohs(client->audio_addr.sin_port), ntohs(client->video_addr.sin_port), client->audio_socket, len);

				if(client->audio_socket > 0 && client->video_send > 30) {

					//PRINTF("smplerate = %d,=%d\n",client->samplerate,info->sample);
					//	if(client->media_status ==1 && client->samplerate == info->sample)
					ret = sendto(client->audio_socket, buff, len, 0, (struct sockaddr *) & (client->audio_addr), sizeof(struct sockaddr_in));
				} else {
					ret = len;
				}
			}

			if(ret != len) {
				PRINTF("vsock=%d,asock=%d,type = %d,buf  = %p,len=%d=sendto=%d\n", client->video_socket, client->audio_socket, type, buff, len, ret);
			}
		}
	}

	return 0;
}

int stream_client_rtpts_send(unsigned char *buff, int len, int type, APP_MEDIA_INFO *info)
{
	int ret = 0;
	int i = 0;
	//	int width, height, sample;
	Stream_Output_Client *client = NULL;
	int stream_channel = info->stream_channel;

	for(i = 0 ; i < OUTPUT_SERVER_MAX_NUM ; i++) {
		client = &(g_stream_client[i]);

		if(g_stream_client[i].c_used == ISUSED && g_stream_client[i].type == TYPE_TS_OVER_RTP
		   && g_stream_client[i].stream_channel == stream_channel) {

			//			{
			//				if(client->send_status == 0)
			//				{
			//					//i must init the stream info.
			//					ret = app_get_media_info(&width,&height,&sample,0);
			//					if(ret == -1)
			//					{
			//						client->media_status = 0;
			//					}
			//					else
			//					{
			//						client->width =width;
			//						client->height = height;
			//						client->samplerate =sample;
			//						client->media_status = 1;
			//					}
			//
			//				}
			//			}
			//if(client->media_status ==1 && client->width == info->width && client->height == info->height)
			ret = sendto(client->video_socket, buff, len, 0, (struct sockaddr *) & (client->video_addr), sizeof(struct sockaddr_in));

			if(ret != len) {
				PRINTF("vsock=%d,asock=%d,type = %d,buf  = %p,len=%d=sendto=%d\n", client->video_socket, client->audio_socket, type, buff, len, ret);
			}
		}
	}

	return 0;
}


Stream_Output_Client *stream_client_is_repeat(struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr)
{
	Stream_Output_Client *client = NULL;
	int i = 0;

	for(i = 0; i < OUTPUT_SERVER_MAX_NUM ; i++) {
		client = &(g_stream_client[i]);

		if(client->c_used == ISUSED) {
			if(memcmp(video_addr, &(client->video_addr), sizeof(struct sockaddr_in)) == 0) {
				PRINTF("i find the same client\n");
				return client;
			}
		}
	}

	return NULL;
}

int stream_client_set_mnum(Stream_Output_Client *client, int num)
{
	if(client != NULL) {
		if(num >= 0) {
			client->usenum ++;
		} else {
			client->usenum -- ;
		}

		PRINTF("client->usenum = %d\n", client->usenum);
		return 0;
	}

	return 0;
}

int steram_client_get_mnum(Stream_Output_Client *client)
{
	if(client != NULL) {
		PRINTF("client->usenum = %d\n", client->usenum);
		return client->usenum;
	}

	return 0;
}


static int g_direct_rtp_active[STREAM_MAX_CHANNEL] = {0, 0, 0, 0};
static int g_direct_ts_active[STREAM_MAX_CHANNEL] = {0, 0, 0, 0};
static int g_rtp_over_ts_active[STREAM_MAX_CHANNEL] = {0, 0, 0, 0};
int mult_rtp_get_active(int channel)
{
	if(g_direct_rtp_active[channel] > 0) {
		return 1;
	} else {
		return 0;
	}
}

int mult_ts_get_active(int channel)
{
	if(g_direct_ts_active[channel] > 0) {
		return 1;
	} else {
		return 0;
	}
}

int mult_rtpts_get_active(int channel)
{
	if(g_rtp_over_ts_active[channel] > 0) {
		return 1;
	} else {
		return 0;
	}
}

static int mult_set_active(int type, int status, int channel)
{
	PRINTF("the %d channel active ts = %d,rtp=%d,rtpts=%d\n", channel, g_direct_ts_active[channel], g_direct_rtp_active[channel], g_rtp_over_ts_active[channel]);

	PRINTF("type =%d,status=%d\n", type, status);

	if(type == TYPE_TS) {
		g_direct_ts_active[channel] += status;
	} else if(type == TYPE_RTP) {
		g_direct_rtp_active[channel] += status;
	} else if(type == TYPE_TS_OVER_RTP) {
		g_rtp_over_ts_active[channel] += status;
	}

	PRINTF("the %d channel active ts = %d,rtp=%d,rtpts=%d\n", channel, g_direct_ts_active[channel], g_direct_rtp_active[channel], g_rtp_over_ts_active[channel]);
	return 0;
}



#endif
