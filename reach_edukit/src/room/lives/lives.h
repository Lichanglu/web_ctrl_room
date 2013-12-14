/*
 * =====================================================================================
 *
 *       Filename:  lives.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2012年11月8日 09时12分18秒
 *       Revision:
 *       Compiler:  gcc
 *
 *         Author:  郑岩柏
 *        Company:  深圳锐取信息技术股份有限公司
 *
 * =====================================================================================
 */

#ifndef VIDEO_LIVE_H
#define VIDEO_LIVE_H

#include "stdint.h"
#include "reach_os.h"
#include "media_msg.h"
#include "rtp_build.h"

//附加 Interruption
#define	INTERRUPTION_MSG_ID		22    // 	支持六路编码器 高低码率

#define	VIDEO_SEND_IP_LEN			16
#define	VIDEO_USER_MAX_NUM		10
#define	VIDEO_ENCODE_MAX_NUM		9

#define VIDEO_QUALITY_SD			0
#define	VIDEO_QUALITY_HD			1

#define	VIDEO_ENCODE_INDEX_LEN	VIDEO_ENCODE_MAX_NUM + 1

#define	MSG_QUEUE_ID				"lives"
#define VIDEO_LOCAL_PORT			33100    //add zl

#define	LIVE_DATA_BASE_KEY		54321

#define LIVE_DEBUG_SLEEP_TIME		15


#define MAX_VOD_MEDIA_PACK_LEN	1300	// add zl
#define	MAX_MEDIA_DATA_LEN		(MAX_VOD_MEDIA_PACK_LEN -FH_LEN-MSG_HEAD_LEN)
#define AVIIF_KEYFRAME 				(0x000010L)

// *******************jpeg ***********************
#define	JPEG_INVAILD_FLAG			-1
#define JPEG_NO_SEND_FLAG			0
#define	JPEG_SEND_FLAG				1

// ******************* data type ******************
#define JPEG_DATA_TYPE				1
#define NOT_JPEG_DATA_TYPE		0

/*高低质量标记*/
#define VIDEO_DATA_TYPE_HD			0
#define VIDEO_DATA_TYPE_BD			1

#define VIDEO_H264						0
#define	VIDEO_JPEG						1



// debug pack seq num
typedef struct debug_pack_seq_num
{
	int32_t 			data_quqlity;
	int32_t				video_seq_num_cache;
	int32_t				audio_seq_num_cache;
	int32_t				video_seq_num_loss;
	int32_t				audio_seq_num_loss;

	int32_t 			video_seq_num_total;
	int32_t 			audio_seq_num_total;

}debug_pack_seq_num_t;


typedef struct video_enc_info
{
	int32_t HD_video_sindex;
	int32_t BD_video_sindex;
	int32_t enc_type;

	// time_tamp
	unsigned int HD_video_timestamp_ex;      		// 备份基值 取得是视频HD  用于做减
	unsigned int BD_video_timestamp_ex;				// 备份基值 取得是视频SD  用于做减

	unsigned int HD_video_timestamp;      		// 其实备份基值 取得是音频HD
	unsigned int BD_video_timestamp;				// 其实备份基值 取得是音频SD

	unsigned int HD_video_timestamp_cache;
	unsigned int BD_video_timestamp_cache;

	unsigned int HD_audio_timestamp_cache;
	unsigned int BD_audio_timestamp_cache;

	//RTP
	RTP_BUILD_INFO		*HD_video_rpt_hand;
	RTP_BUILD_INFO		*BD_video_rpt_hand;

	RTP_BUILD_INFO		*HD_audio_rpt_hand;
	RTP_BUILD_INFO		*BD_audio_rpt_hand;

	//debug data
	int32_t HD_recv_flag;
	int32_t BD_recv_flag;

	// debug pack seq num
	debug_pack_seq_num_t 	HD_pack_seq;
	debug_pack_seq_num_t	BD_pack_seq;

}video_enc_info_t;

typedef struct video_sindex_info
{
	int32_t 			vaild_flag;
	int32_t 			enc_num;
	video_enc_info_t    video_enc[VIDEO_ENCODE_MAX_NUM];

	int32_t 			reset_flag;

}video_sindex_info_t;


typedef struct lives_user_addr
{
	int8_t 	m_user_ip[VIDEO_SEND_IP_LEN];
	int32_t m_user_port;

}lives_user_addr_t;


//debug ******************************************
typedef struct live_debug_info
{
	int32_t live_recv_flag_HD;
	int32_t live_recv_flag_SD;
	int32_t msg_type;
	int32_t data_type;
	int32_t video_index;

}live_debug_info_t;

typedef struct user_debug_info
{
	int32_t user_recv_flag;
	int32_t user_send_flag;
	int32_t video_sindex;

}user_debug_info_t;


typedef struct video_data_type
{
	int8_t 	data_sindex;						// 第一、二、三..路'1' '2' '3'  '...'
	int32_t data_quqlity;						// 数据高低码流  '0 --- HD'   '1 --- SD'
	int32_t data_type;							// 数据类型	''
	int32_t video_sindex;						// 第一、二、三 ..路  1 2 3 ..

	unsigned int 		video_timestamp;		// 时间戳 (取的是时间差)

	debug_pack_seq_num_t 	*video_enc_addr;		// 数据对应编码器地址
}video_data_type_t;


typedef struct lives_user_info
{
	int32_t 			vaild_flag;
	int32_t				index;
	lives_user_addr_t 	lives_user_addr[VIDEO_ENCODE_MAX_NUM];
	int32_t 			video_quality_type;
	int8_t				video_encode_index[VIDEO_ENCODE_INDEX_LEN];
	int32_t				video_jpeg_flag;

	int32_t				data_udp_fd;

	// debug
	user_debug_info_t	user_debug_info[VIDEO_ENCODE_MAX_NUM];

	// 附加
	int32_t 			user_id	;

}lives_user_info_t;

typedef struct lives_user_addr_ex
{
	lives_user_addr_t 		user_addr_info;
	int32_t 				udp_fd;
}lives_user_addr_ex_t;

typedef struct video_send_user
{
	lives_user_addr_ex_t 	user_addr_info[VIDEO_USER_MAX_NUM];
	int32_t 				video_sindex;
	int32_t 				user_num;
//	int32_t 				fd;

	// 用于 Debug 监控用户是否有数据
	lives_user_info_t 		*user_addr[VIDEO_USER_MAX_NUM];
	// 拥有 Debug 监控数据包是否在中间加工时丢失
	debug_pack_seq_num_t    *video_enc_addr;

	unsigned int			video_timestamp;

}video_send_user_t;


typedef struct lives_mode_info
{
	lives_user_info_t 	user_info[VIDEO_USER_MAX_NUM];

	pthread_mutex_t 			mutex ;							// 锁住用户列表

	video_sindex_info_t video_sindex;

	int32_t 			room_id;

	int32_t 			user_num;

	int32_t 			msgid;				//消息队列

	msgque				jpeg_msgque;
	int32_t 			jpeg_valid_flag;

// rtp
//	RTP_BUILD_INFO		*video_rpt_hand;

	// debug flag info
//	live_debug_info_t	live_debug_info[VIDEO_ENCODE_MAX_NUM];


	int32_t 			mode_valid_flag;
	int32_t				debug_thread_valid_flag;

	pthread_t			main_thread_handle;
	pthread_t			debug_thread_headle;

}lives_mode_info_t;


typedef struct lives_mode_hand
{
	lives_mode_info_t	lives_mode_info;

	int32_t (*set_lives_enc_info)(video_sindex_info_t *enc_info,void *arg);
	int32_t (*set_lives_user_info)(lives_user_info_t *user , void *arg);
	int32_t (*stop_lives_user_info)(lives_user_info_t *user,void *arg);
	int32_t (*stop_lives_user_info_unusual)(int32_t user_id,void *arg);
	int32_t (*stop_lives_user_all_unusual)(void *arg);
	int32_t (*recognition_req_strm_proc)(void *arg,int32_t msgtype);

}lives_mode_hand_t;


int32_t recognition_req_strm_proc(void *arg,int32_t msgtype);
int32_t set_lives_enc_info(video_sindex_info_t *enc_info,void *arg);
int32_t set_lives_user_info(lives_user_info_t *user , void *arg);
int32_t stop_lives_user_info(lives_user_info_t *user,void *arg);
int32_t stop_lives_user_info_unusual(int32_t user_id,void *arg);
int32_t stop_lives_user_all_unusual(void *arg);



lives_mode_hand_t *register_room_lives_module(int32_t room_id);
int32_t unregister_room_lives_module(lives_mode_hand_t *handle);

int32_t send_rtp_video(void *arg , int8_t *send_buf,int32_t len);
int32_t reset_live_mode_enc_time(void *arg);


#endif

