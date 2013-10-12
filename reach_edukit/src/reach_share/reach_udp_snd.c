#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "reach_udp_snd.h"
#include "reach_rtp_build.h"

static void us_set_hdb_freame_head(US_FRAMEHEAD *fh, RUdpS_freame_head_t *pd);
static uint32_t UdpSend_get_time_tick(send_udp_handle *p_udp_hand, uint32_t  time_tick);

static uint32_t UdpSend_get_time_tick(send_udp_handle *p_udp_hand, uint32_t  time_tick)
{
	uint32_t  video_time = 0;

	if(0 == p_udp_hand->base_video_time) {
		p_udp_hand->base_video_time = time_tick;
		video_time = 0;
		p_udp_hand->prev_video_time = video_time;
		p_udp_hand->prev_time_tick = time_tick;
		return video_time;
	} else {
		video_time = time_tick - p_udp_hand->base_video_time;
	}

	if(video_time > 0xFFFFFFFF / 90 - 10000) {
		p_udp_hand->base_video_time = time_tick;
		video_time = 0;
	}

	if(video_time <= p_udp_hand->prev_video_time) {
		video_time += 1;
	}

	p_udp_hand->prev_video_time = video_time;
	p_udp_hand->prev_time_tick = time_tick;
	return video_time;
}
// *****************************************************
//function	: 根据帧头信息赋值RTP 扩展头
//author 	: lcs		2013.7.1
//******************************************************

int32_t UdpSend_switch_audio_sample_rate(int32_t sample_rate)
{
	int32_t sample_index = 0;

	switch(sample_rate) {
		case 16000:
			sample_index = 0;
			break;

		case 32000:
			sample_index = 1;
			break;

		case 44100:
			sample_index = 2;
			break;

		case 48000:
			sample_index = 3;
			break;

		case 96000:
			sample_index = 4;
			break;

		default:
			sample_index = 3;
			break;
	}

	return sample_index;
}


static void UdpSend_set_hdb_freame_head(US_FRAMEHEAD *fh, frame_info_t *frame_info)
{

	if(RUdpS_H264_CODEC_TYPE == frame_info->m_data_codec) {
		fh->codec 	= 0;
		fh->framerate 	= frame_info->m_frame_rate;
		fh->height 		= frame_info->m_hight;
		fh->width 		= frame_info->m_width;
		fh->samplerate	= frame_info->is_blue;		//是否蓝屏；samplerate  为1时蓝屏.
		fh->framelength	= frame_info->m_frame_length;
		fh->reserve		= 0;
		fh->Iframe		= 0;

		if(frame_info->m_dw_flags != 0) {
			fh->Iframe = 1;
		}

	} else if(RUdpS_JPEG_CODEC_TYPE == frame_info->m_data_codec) {
		fh->codec 	= 2;

		//fh->framerate 	= pd->sample_rate;  	old
		fh->framerate 	= frame_info->m_frame_rate;
		fh->height 		= frame_info->m_hight;
		fh->width 		= frame_info->m_width;
		fh->samplerate	= 0;
		fh->framelength	= frame_info->m_frame_length;
		fh->reserve		= 0;
		fh->Iframe		= 0;

		if(frame_info->m_dw_flags != 0) {
			fh->Iframe = 1;
		}
	} else if(RUdpS_AAC_CODEC_TYPE == frame_info->m_data_codec) {
		fh->codec 		= 1;
		fh->framerate 	= 0;
		fh->height 		= 0;
		fh->width 		= 0;
		fh->framelength	= frame_info->m_frame_length;
		fh->samplerate	= UdpSend_switch_audio_sample_rate(frame_info->m_hight);
		fh->reserve		= 0;
		fh->Iframe 		= 0;

		if(frame_info->m_dw_flags != 0) {
			fh->Iframe = 1;
		}
	} else {
		ERR_PRINTF("pd->data_type is error!\n");
	}
}

int32_t UdpSend_rtp_data(udp_send_module_handle *p_udp_hand, frame_info_t *frame_info)
{
	int32_t 		ret								= -1;
	US_FRAMEHEAD		frame_head;
	int32_t 		mtu								= MAX_VOD_MTU - 20;
	int32_t 		index							= 0;
	int32_t			video_time 				= 0;
	send_udp_handle *udp_hand = &p_udp_hand->udp_hand;
	US_RTP_BUILD_HANDLE rtp_hand   = p_udp_hand->rtp_hand;


	//ERR_PRINTF("len = %d, time = %d, fd  = %d", freame_info->data_len, freame_info->video_time, p_udp_hand->udp_hand.snd_fd);
	video_time = UdpSend_get_time_tick(udp_hand, frame_info->time_tick);
	UdpSend_set_hdb_freame_head(&frame_head, frame_info);

	if(RUdpS_H264_CODEC_TYPE == frame_info->m_data_codec) {
#if 1
		ret = UdpSend_rtp_build_video_data(rtp_hand, frame_info->m_frame_length, udp_hand->src_data, mtu , video_time, udp_hand, &frame_head);

		if(ret < 0) {
			ERR_PRINTF("rtp_build_video_data is error!\n");
		}

#endif

	} else if(RUdpS_JPEG_CODEC_TYPE == frame_info->m_data_codec) {
#if 1
		ret = UdpSend_rtp_build_jpeg_data(rtp_hand, frame_info->m_frame_length, udp_hand->src_data, mtu , video_time, udp_hand, &frame_head);

		if(ret < 0) {
			ERR_PRINTF("rtp_build_jpeg_data is error!\n");
		}

#endif
	} else {
#if 1
		ret = UdpSend_rtp_build_audio_data(rtp_hand, frame_info->m_frame_length, udp_hand->src_data, frame_info->m_hight, mtu , video_time, udp_hand, &frame_head);

		if(ret < 0) {
			ERR_PRINTF("rtp_build_audio_data is error!\n");
		}

#endif
	}

	return OPERATION_SUCC;
}


udp_send_module_handle *UdpSend_init(stream_send_cond_t *src)
{
	if(NULL  == src->ip || strlen(src->ip) > RUdpS_IP_LEN) {
		ERR_PRINTF("ip = [%p]", __TIME__,  __func__, __LINE__, src->ip);
		return NULL;
	}

	udp_send_module_handle *p_udp_send_module_hand = NULL;
	p_udp_send_module_hand = (udp_send_module_handle *)malloc(sizeof(udp_send_module_handle));

	if(NULL == p_udp_send_module_hand) {
		ERR_PRINTF("malloc error");
		return NULL;
	}

	memset(p_udp_send_module_hand, 0, sizeof(udp_send_module_handle));
	p_udp_send_module_hand->rtp_hand = (US_RTP_BUILD_HANDLE)UdpSend_rtp_build_init(0, 0);
	p_udp_send_module_hand->udp_hand.prev_video_time = 0;
	p_udp_send_module_hand->udp_hand.snd_video_port = src->video_port;
	p_udp_send_module_hand->udp_hand.snd_audio_port = src->audio_port;
	p_udp_send_module_hand->udp_hand.src_data = (uint8_t *)malloc(UdpSend_Max_Frame_Length);

	if(NULL == p_udp_send_module_hand->udp_hand.src_data) {
		ERR_PRINTF("malloc error p_udp_send_module_hand->udp_hand.src_data = [%p]", p_udp_send_module_hand->udp_hand.src_data);
		return NULL;
	}

	sprintf(p_udp_send_module_hand->udp_hand.snd_ip, "%s", src->ip);

	p_udp_send_module_hand->udp_hand.snd_fd = UdpSend_create_sock(&p_udp_send_module_hand->udp_hand);
	return p_udp_send_module_hand;
}

int32_t UdpSend_create_sock(send_udp_handle *p_handle)
{
	int 				udp_fd = 0;
	int 				optval 						= 1;
	socklen_t 	size 							= sizeof(struct sockaddr);
	int 				optlen							= 0;
	int 				upd_max_buf			= RUdpS_BUF_MAX_SIZE;
	int  				ret 								= 0;
	struct sockaddr_in 	addr;

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(p_handle->snd_video_port);
	addr.sin_addr.s_addr = inet_addr(p_handle->snd_ip);
	udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &optval, size);
	ret = setsockopt(udp_fd, SOL_SOCKET, SO_SNDBUF, &upd_max_buf, sizeof(int32_t));

	if(ret < 0) {
		ERR_PRINTF("SET_SOCKET RECV_BUF IS ERROR! <SOCKET : %d> <SEND_BUF : %d > <ERROR_MESSAGE : %s >", udp_fd, upd_max_buf, strerror(errno));
		return OPERATION_ERR;
	}

	optlen = sizeof(int);
	upd_max_buf = 0 ;
	ret = getsockopt(udp_fd, SOL_SOCKET, SO_SNDBUF, &upd_max_buf , &optlen);

	if(ret < 0) {
		ERR_PRINTF("GET_SOCKET RECV_BUF IS ERROR! <SOCKET : %d> <SEND_BUF : %d > <ERROR_MESSAGE : %s >", udp_fd, upd_max_buf, strerror(errno));
	}

	SUC_PRINTF("udp_fd = %d, upd_max_snd_buf = %d, video_port = %d, audio_port  = %d, snd_ip = %s\n", udp_fd, upd_max_buf, p_handle->snd_video_port,
	           p_handle->snd_audio_port, p_handle->snd_ip);
	return udp_fd;

}


void UdpSend_deinit(udp_send_module_handle *p_udp_send)
{
	if(NULL == p_udp_send) {
		ERR_PRINTF("p_udp_send = [%p]", p_udp_send);
	}

	if(NULL != p_udp_send->udp_hand.src_data) {
		free(p_udp_send->udp_hand.src_data);
		p_udp_send->udp_hand.src_data = NULL;
	}

	if(0 < p_udp_send->udp_hand.snd_fd) {
		close_socket(p_udp_send->udp_hand.snd_fd);
		p_udp_send->udp_hand.snd_fd = -1;
	}

	UdpSend_rtp_build_uninit(&p_udp_send->rtp_hand);
	free(p_udp_send);
	p_udp_send = NULL;
}