#ifndef __REACH_UDP_SEND_H
#define  __REACH_UDP_SEND_H  // simple  RUdpS

#include <string.h>
#include <strings.h>
#include <malloc.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "stdint.h"
#include "reach_rtp_build.h"

#define SEND_DEBUG	(1)

#ifdef SEND_DEBUG
#define ERR_PRINTF(a...) {printf("[%s %s %d] errno = %d ", __TIME__, __func__, __LINE__, errno);printf(a);putchar('\n');}
#define SUC_PRINTF(a...)             {printf("[%s %s %d] ", __TIME__, __func__, __LINE__);printf(a);putchar('\n');}
#endif

#define RUdpS_BUF_MAX_SIZE   				655360
#define MAX_VOD_MTU								1400
#define UdpSend_Max_Frame_Length		(2 * 1024 * 1024)

#define	RUdpS_MAKEFOURCC(ch0, ch1, ch2, ch3)                      \
		((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |   \
		((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))

#define	RUdpS_H264_CODEC_TYPE   			RUdpS_MAKEFOURCC('H','2','6','4')
#define	RUdpS_AAC_CODEC_TYPE    			RUdpS_MAKEFOURCC('A','D','T','S')
#define 	RUdpS_RQHD_CODEC_TYPE			RUdpS_MAKEFOURCC('R','Q','H','D')
#define   RUdpS_JPEG_CODEC_TYPE 				RUdpS_MAKEFOURCC('J','P','E','G')

#define RUdpS_IP_LEN		(16)

typedef struct RUdpS_debug_seq_num
{
	int32_t				video_seq_num_loss;
	int32_t				audio_seq_num_loss;

	int32_t 			video_seq_num_total;
	int32_t 			audio_seq_num_total;
	
}RUdpS_debug_seq_num_t;

/*UDP NetWork Header*/
typedef struct RUdpS_freame_heads {
	uint32_t m_id;			//= mmioFOURCC('R','Q','H','D');
	uint32_t m_time_tick;    //时间戳
	uint32_t m_frame_length; //帧数据长度
	uint32_t m_data_codec;   //编码方式
	uint32_t m_frame_rate;   //数据帧率 或 音频采样率
	uint32_t m_width;       //宽度
	uint32_t m_hight;       //高度
	uint32_t m_colors;      //颜色数
	uint32_t m_dw_segment;		//组包标示 0表示中间包 1表示结尾包 2表示开始包 3表示独立包
	uint32_t m_dw_flags;			//帧标志 I帧？
	uint32_t m_dw_packet_number; 	//
	uint32_t m_others;      			//包对应窗口序号
} RUdpS_freame_head_t;

typedef struct send_udp_handle_t{
		uint8_t   snd_ip[RUdpS_IP_LEN];
		int32_t   snd_video_port;
		int32_t   snd_audio_port;
		int32_t   snd_fd;

		uint8_t	   *src_data;			// add zl

		uint32_t  base_video_time;
		uint32_t  prev_video_time;
		uint32_t   prev_time_tick;
		uint32_t   reserve;

		RUdpS_debug_seq_num_t     pack_seq;

}send_udp_handle;

typedef struct _UDP_SEND_MODULE_HANDLE{
	send_udp_handle   udp_hand;
	US_RTP_BUILD_HANDLE rtp_hand;
}udp_send_module_handle;

//udp_send 模块初始化需要的结构体
typedef struct stream_send_cond_ {
	char ip[16];		//发送到哪的ip地址。
	unsigned short video_port;//发送视频端口。
	unsigned short audio_port; //发送音频端口。
} stream_send_cond_t;


typedef struct __frame_info_t{
	uint32_t time_tick;
	uint32_t m_data_codec;
	uint32_t m_width;       //宽度
	uint32_t m_hight;       //视频的高或者音频的采样率。
	uint32_t m_dw_flags;
	uint32_t m_frame_length;
	uint32_t m_frame_rate;
	uint32_t is_blue;// 1 blue， 0 no。
}frame_info_t;

#define OPERATION_ERR							(-1)
#define OPERATION_SUCC							(0)
#define OPERATION_CONTINUE				(1) 	// 操作继续

//设置udp发送相关参数.
udp_send_module_handle *UdpSend_init(stream_send_cond_t *src);
//数据拆包成rtp并发送。
int32_t UdpSend_rtp_data(udp_send_module_handle *p_udp_send, frame_info_t *frame_info);
void UdpSend_deinit(udp_send_module_handle *p_udp_send);

#endif
