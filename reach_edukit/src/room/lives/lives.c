#include <pthread.h>
#include "media_msg.h"
#include "reach_os.h"
#include "lives.h"
#include "rtp_struct.h"


static int32_t pkg_num = 0;
static int32_t debug_data_flag = 0;
static int32_t Interruption_msg(int32_t msg_id)
{
	int ret = -1;
	msgque  m_interruption_msg;
	m_interruption_msg.msgtype 	= INTERRUPTION_MSG_ID;
	m_interruption_msg.msgbuf 	= NULL;

	ret = r_msg_send(msg_id, &m_interruption_msg, sizeof(msgque)-sizeof(long), IPC_NOWAIT);
	if(ret < 0)
	{
		nslog(NS_ERROR,"IMPORT_ERROR, INTERRUPTION_MSG ! <error : %d > <msg_id : %d >\n",errno,msg_id);
		return -1;
	}
	nslog(NS_ERROR,"SUCESS   INTERRUPTION_MSG !!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	return 0;
}



// *****************************************************
//function	: 创建数据SOCKET
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t create_video_local_socket(int32_t port)
{
	struct sockaddr_in SrvAddr ;
	int32_t send_buf_size 				= UDPBUFFERSZIE;
	int32_t opt 						= 1;
	int32_t udp_fd						= 0;
	int32_t ret							= 0;
	int32_t optlen						= 0;

	bzero(&SrvAddr, sizeof(struct sockaddr_in));
	SrvAddr.sin_family = AF_INET;
	SrvAddr.sin_port = r_htons(port);
	SrvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	udp_fd = r_socket(AF_INET, SOCK_DGRAM, 0);
	r_setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(bind(udp_fd, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0) {
		nslog(NS_INFO, "bind error:%d\n", errno);
		return -1;
	}

	ret = r_setsockopt(udp_fd, SOL_SOCKET, SO_SNDBUF, (const int8_t *)&send_buf_size, sizeof(int32_t));
	if(ret < 0)
	{
		nslog(NS_ERROR,"SET_SOCKET SEND_BUF IS ERROR! <SEND_BUF : %d >",send_buf_size);

	}
	optlen = sizeof(int32_t);
	send_buf_size = 0;
	ret = r_getsockopt(udp_fd, SOL_SOCKET, SO_SNDBUF, &send_buf_size , &optlen);
	if(ret < 0)
	{
		nslog(NS_ERROR,"GET_SOCKET SEND_BUF IS ERROR! <SEND_BUF : %d >",send_buf_size);
	}

	nslog(NS_INFO,"SEND_BUF --------------------------------- %d",send_buf_size);

	int32_t curTTL = 0; //当前的TTL
 	int32_t lenTTL = sizeof(int);
	int32_t ttl = 128;


	 //设置新的TTL值
 	//r_setsockopt(udp_fd, IPPROTO_IP, IP_TTL, (char *)&ttl, sizeof(ttl));

		//获取当前的TTL值
	//r_getsockopt(udp_fd , IPPROTO_IP, IP_TTL, (char *)&curTTL, &lenTTL);//m_hSocket为UDP套接字

	nslog(NS_INFO,"SEND_TTL --------------------------------- %d",curTTL);

	set_nonblocking(udp_fd);

	return udp_fd;


}


// *****************************************************
//function	: 初始化用户信息
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t init_lives_mode_info_ex(lives_user_info_t *user_info)
{
	int32_t index =0 ;
	r_memset(user_info->video_encode_index,0,VIDEO_ENCODE_MAX_NUM);
	user_info->video_quality_type 	= -1;
	user_info->index 				= -1;
	user_info->video_jpeg_flag		= JPEG_INVAILD_FLAG;
	user_info->vaild_flag			= -1;						//初始化
	user_info->data_udp_fd			= -1;
	user_info->user_id				= -1;

	for(index = 0; index < VIDEO_ENCODE_MAX_NUM ; index ++)
	{
		r_memset(user_info->lives_user_addr[index].m_user_ip , 0 ,VIDEO_SEND_IP_LEN);
		user_info->lives_user_addr[index].m_user_port = 0;
	}
	return OPERATION_SUCC;
}
// *****************************************************
//function	: 初始化直播模块信息
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t	init_lives_mode_info(lives_mode_hand_t *arg)
{
	int32_t index = 0;

	// 0.
	arg->lives_mode_info.mode_valid_flag = 1;
	pthread_mutex_init(&(arg->lives_mode_info.mutex) , NULL);

	// 1. create msg queue
	arg->lives_mode_info.msgid = r_msg_create_u(LIVE_DATA_BASE_KEY + arg->lives_mode_info.room_id);
	if(-1 == arg->lives_mode_info.msgid)
	{
		nslog(NS_ERROR, "FtpServiceInit: create msgqueue fail");
		return OPERATION_ERR;
	}
	nslog(NS_INFO ,"lives->msgid = %d\n",arg->lives_mode_info.msgid);

	// 2. set callback function
	arg->set_lives_user_info 					= set_lives_user_info;
	arg->stop_lives_user_info 					= stop_lives_user_info;
	arg->set_lives_enc_info						= set_lives_enc_info;
	arg->stop_lives_user_info_unusual			= stop_lives_user_info_unusual;
	arg->stop_lives_user_all_unusual			= stop_lives_user_all_unusual;
	arg->recognition_req_strm_proc				= recognition_req_strm_proc;

	// 3. init userinfo
	arg->lives_mode_info.user_num 				= 0;
	for(index = 0 ; index < VIDEO_USER_MAX_NUM ; index ++)
	{
		init_lives_mode_info_ex(&(arg->lives_mode_info.user_info[index]));
	}
	// 4. init jpeg info
	arg->lives_mode_info.jpeg_valid_flag = -1;
	arg->lives_mode_info.jpeg_msgque.msgbuf = NULL;
	arg->lives_mode_info.jpeg_msgque.msgtype = -1;

	// 5. init encoder info
	arg->lives_mode_info.video_sindex.enc_num = 0;
	arg->lives_mode_info.video_sindex.vaild_flag = -1;					//  初始状态
	arg->lives_mode_info.video_sindex.reset_flag = 0;
	for(index = 0 ;index < VIDEO_ENCODE_MAX_NUM ; index ++)
	{
		arg->lives_mode_info.video_sindex.video_enc[index].BD_recv_flag 			= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_recv_flag 			= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_video_sindex			= -1;
		arg->lives_mode_info.video_sindex.video_enc[index].BD_video_sindex			= -1;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_video_timestamp		= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].BD_video_timestamp		= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].enc_type					= -1;

		// init debug pack seq
		arg->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.data_quqlity 		= 1;
		arg->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_cache 	= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_loss 	= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_total	= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_cache 	= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_loss 	= 0 ;
		arg->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_total 	= 0;

		arg->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.data_quqlity 		= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_cache 	= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_loss 	= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_total 	= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_cache 	= 0;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_loss 	= 0 ;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_total 	= 0;

		// init rtp pro video
		arg->lives_mode_info.video_sindex.video_enc[index].HD_video_rpt_hand = NULL;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_video_rpt_hand = rtp_build_init(0,0);
		if(arg->lives_mode_info.video_sindex.video_enc[index].HD_video_rpt_hand != NULL)
		{
			rtp_build_reset_time(arg->lives_mode_info.video_sindex.video_enc[index].HD_video_rpt_hand);
		}
		else
		{
			nslog(NS_ERROR,"import error : rtp_build_init is error!\n");
			return OPERATION_ERR;
		}

		arg->lives_mode_info.video_sindex.video_enc[index].BD_video_rpt_hand = NULL;
		arg->lives_mode_info.video_sindex.video_enc[index].BD_video_rpt_hand = rtp_build_init(0,0);
		if(arg->lives_mode_info.video_sindex.video_enc[index].BD_video_rpt_hand != NULL)
		{
			rtp_build_reset_time(arg->lives_mode_info.video_sindex.video_enc[index].BD_video_rpt_hand);
		}
		else
		{
			nslog(NS_ERROR,"import error : rtp_build_init is error!\n");
			return OPERATION_ERR;
		}

		// init rtp pro audio
		arg->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand = NULL;
		arg->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand = rtp_build_init(0,0);
		if(arg->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand != NULL)
		{
			rtp_build_reset_time(arg->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand);
		}
		else
		{
			nslog(NS_ERROR,"import error : rtp_build_init is error!\n");
			return OPERATION_ERR;
		}

		arg->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand = NULL;
		arg->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand = rtp_build_init(0,0);
		if(arg->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand != NULL)
		{
			rtp_build_reset_time(arg->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand);
		}
		else
		{
			nslog(NS_ERROR,"import error : rtp_build_init is error!\n");
			return OPERATION_ERR;
		}

		#if 0
		if(index == 0)
		{
			arg->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand = NULL;
			arg->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand = rtp_build_init(0,0);
			if(arg->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand != NULL)
			{
				rtp_build_reset_time(arg->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand);
			}
			else
			{
				nslog(NS_ERROR,"import error : rtp_build_init is error!\n");
				return OPERATION_ERR;
			}

			arg->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand = NULL;
			arg->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand = rtp_build_init(0,0);
			if(arg->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand != NULL)
			{
				rtp_build_reset_time(arg->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand);
			}
			else
			{
				nslog(NS_ERROR,"import error : rtp_build_init is error!\n");
				return OPERATION_ERR;
			}
		}
		else
		{
			arg->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand = NULL;
			arg->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand = NULL;
		}
		#endif
	}

	return OPERATION_SUCC;
}

// *****************************************************
//function	: 获取会议室路信息
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t get_video_sindex(int32_t video_type,video_sindex_info_t *arg)
{
	int32_t video_sindex = -1;
	int32_t index =0;
	video_sindex_info_t *temp_video_sindex = (video_sindex_info_t *)arg;
//	nslog(NS_INFO,"ecv _ num : %d -=-------------- %d\n",temp_video_sindex->enc_num,video_type);
	for(index = 0; index < temp_video_sindex->enc_num ;index ++)
	{
//	nslog(NS_INFO ,"------------ %d\n",temp_video_sindex->video_enc[index].enc_type);
		if(temp_video_sindex->video_enc[index].enc_type == video_type)
		{
			video_sindex = index;
			break;
		}
	}
	return video_sindex ;
}
// *****************************************************
//function	:  待发数据的路、类型等信息
//author 	:  zhengyb		2012.12.7
//******************************************************

int32_t get_video_data_info(msgque *revmsg ,video_sindex_info_t *arg ,video_data_type_t *data_type)
{
	int32_t 		index 						= 0 ;
	int32_t 		ret 						= -1;

	int32_t 		msg_type 					= revmsg->msgtype;
	parse_data_t 	*pd 						= (parse_data_t *)revmsg->msgbuf;

	for(index = 0; index < arg->enc_num ;index ++)
	{
//		nslog(NS_INFO," %d   %d    %d  \n",
//			arg->video_enc[index].enc_type,arg->video_enc[index].HD_video_sindex,arg->video_enc[index].BD_video_sindex);
		if(arg->video_enc[index].enc_type == -1)
		{
			continue;
		}
		else if(arg->video_enc[index].enc_type == VIDEO_H264)
		{
			if(arg->video_enc[index].HD_video_sindex == msg_type )
			{
				if(R_AUDIO == pd->data_type)
				{
					if(arg->video_enc[index].HD_video_timestamp == 0)
					{
						arg->video_enc[index].HD_video_timestamp = data_type->video_timestamp;
						data_type->video_timestamp = 0;
					}
					else
					{
						if(data_type->video_timestamp < 0 )
						{
							nslog(NS_ERROR ,"DATA_TIME_STAMP : %d ---- CACHE_TIME_STAMP : %d <SINDEX : %d > <R_AUDIO_HD>\n",
								data_type->video_timestamp,
								arg->video_enc[index].HD_video_timestamp,
								index);
						}

						// 附加 DEBUG 时间戳
						if(data_type->video_timestamp < arg->video_enc[index].HD_audio_timestamp_cache)
						{
							nslog(NS_ERROR,"<AUDIO> IMPORT_ERROR Wrong sequence  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].HD_audio_timestamp_cache);
							data_type->video_timestamp = arg->video_enc[index].HD_audio_timestamp_cache + 1;
						}
						if(data_type->video_timestamp == arg->video_enc[index].HD_audio_timestamp_cache)
						{

							nslog(NS_ERROR,"<AUDIO equal> IMPORT_ERROR time out  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].HD_audio_timestamp_cache);
							data_type->video_timestamp = arg->video_enc[index].HD_audio_timestamp_cache + 1;
						}

						#if 0
						if(data_type->video_timestamp - arg->video_enc[index].HD_audio_timestamp_cache > 70)
						{
							nslog(NS_ERROR,"<AUDIO> IMPORT_ERROR time out  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].HD_audio_timestamp_cache);
						}
						#endif
						arg->video_enc[index].HD_audio_timestamp_cache = data_type->video_timestamp;

						data_type->video_timestamp = data_type->video_timestamp - arg->video_enc[index].HD_video_timestamp;
						// 附加解决时间戳爆掉问题 应用方案1.
						if(data_type->video_timestamp > 0XFFFFFFFF/90 - 10000)
						//if(data_type->video_timestamp > 300000)  // 5min
						{
							arg->video_enc[index].HD_video_timestamp = 0;
						}

					}

					arg->video_enc[index].HD_recv_flag	=	1;
					data_type->data_quqlity = VIDEO_DATA_TYPE_HD;
					data_type->data_type = VIDEO_H264;
					data_type->video_enc_addr = &(arg->video_enc[index].HD_pack_seq);
					break;
				}
				else
				{
					#if 1
					if(arg->video_enc[index].HD_video_timestamp == 0)
					{
						nslog(NS_ERROR, "HD_video_timestamp == 0!!!!!!!!!!");
						#if 0
						if(index == 0)
						{
							nslog(NS_INFO,"Wait the audio time!<HD>\n");
							return OPERATION_ERR;
						}
						#endif
						arg->video_enc[index].HD_video_timestamp = data_type->video_timestamp;
						data_type->video_timestamp = 0;
					}
					else
					{

						// 附加 DEBUG 时间戳
						if(data_type->video_timestamp < arg->video_enc[index].HD_video_timestamp_cache)
						{
							nslog(NS_ERROR,"<VIDEO> IMPORT_ERROR Wrong sequence  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].HD_video_timestamp_cache);
							data_type->video_timestamp = arg->video_enc[index].HD_video_timestamp_cache + 1;
						}
						if(data_type->video_timestamp == arg->video_enc[index].HD_video_timestamp_cache)
						{
							nslog(NS_ERROR,"<VIDEO equal> IMPORT_ERROR Wrong sequence  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].HD_video_timestamp_cache);
							data_type->video_timestamp = arg->video_enc[index].HD_video_timestamp_cache + 1;
						}
						#if 0
						if(data_type->video_timestamp - arg->video_enc[index].HD_video_timestamp_cache > 70)
						{
							nslog(NS_ERROR,"<VIDEO> IMPORT_ERROR time out  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].HD_video_timestamp_cache);
						}
						#endif

						arg->video_enc[index].HD_video_timestamp_cache = data_type->video_timestamp;

						data_type->video_timestamp = data_type->video_timestamp - arg->video_enc[index].HD_video_timestamp;
						if(data_type->video_timestamp < 0 )
						{
							nslog(NS_ERROR ,"DATA_TIME_STAMP : %d ---- CACHE_TIME_STAMP : %d <SINDEX : %d > <H264_HD>\n",
								data_type->video_timestamp,
								arg->video_enc[index].HD_video_timestamp,
								index);
						}

						// 附加解决时间戳爆掉问题 应用方案1.
						if(data_type->video_timestamp > 0XFFFFFFFF/90 - 10000)
						//if(data_type->video_timestamp > 300000)  // 5min
						{
							arg->video_enc[index].HD_video_timestamp = 0;
						}
					}
					#endif
					arg->video_enc[index].HD_recv_flag	=	1;
					data_type->data_quqlity = VIDEO_DATA_TYPE_HD;
					data_type->data_type = VIDEO_H264;
					data_type->video_enc_addr = &(arg->video_enc[index].HD_pack_seq);
					break;
				}
			}
			if(arg->video_enc[index].BD_video_sindex == msg_type)
			{
				if(R_AUDIO == pd->data_type)
				{
					if(arg->video_enc[index].BD_video_timestamp == 0)
					{
						arg->video_enc[index].BD_video_timestamp = data_type->video_timestamp;
						data_type->video_timestamp = 0;
					}
					else
					{

						// 附加 DEBUG 时间戳
						if(data_type->video_timestamp < arg->video_enc[index].BD_audio_timestamp_cache)
						{
							nslog(NS_ERROR,"<AUDIO> IMPORT_ERROR Wrong sequence  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].BD_audio_timestamp_cache);
							data_type->video_timestamp = arg->video_enc[index].BD_audio_timestamp_cache + 1;

						}
						if(data_type->video_timestamp == arg->video_enc[index].BD_audio_timestamp_cache)
						{
							nslog(NS_ERROR,"<AUDIO equal> IMPORT_ERROR Wrong sequence  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].BD_audio_timestamp_cache);
							data_type->video_timestamp = arg->video_enc[index].BD_audio_timestamp_cache + 1;

						}
				#if 0
						if(data_type->video_timestamp - arg->video_enc[index].BD_audio_timestamp_cache > 70)
						{
							nslog(NS_ERROR,"<AUDIO> IMPORT_ERROR time out  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].BD_audio_timestamp_cache);
						}
				#endif
						arg->video_enc[index].BD_audio_timestamp_cache = data_type->video_timestamp;

						data_type->video_timestamp = data_type->video_timestamp - arg->video_enc[index].BD_video_timestamp;
						if(data_type->video_timestamp < 0 )
						{
							nslog(NS_ERROR ,"DATA_TIME_STAMP : %d ---- CACHE_TIME_STAMP : %d <SINDEX : %d > <R_AUDIO_BD>\n",
								data_type->video_timestamp,
								arg->video_enc[index].BD_video_timestamp,
								index);
						}

						// 附加解决时间戳爆掉问题 应用方案1.
						if(data_type->video_timestamp > 0XFFFFFFFF/90 - 10000)
						//if(data_type->video_timestamp > 300000)  // 5min
						{
							arg->video_enc[index].BD_video_timestamp = 0;
						}

					}

					arg->video_enc[index].BD_recv_flag	=	1;
					data_type->data_quqlity = VIDEO_DATA_TYPE_BD;
					data_type->data_type = VIDEO_H264;
					data_type->video_enc_addr = &(arg->video_enc[index].BD_pack_seq);
					break;
				}
				else
				{
#if 1
					if(arg->video_enc[index].BD_video_timestamp == 0)
					{
						nslog(NS_ERROR, "BD_video_timestamp == 0!!!!!!!!!!");
			#if 0
						if(index == 0)
						{
							nslog(NS_INFO,"Wait the audio time! <BD> \n");
							return OPERATION_ERR;
						}
			#endif
						arg->video_enc[index].BD_video_timestamp = data_type->video_timestamp;
						data_type->video_timestamp = 0;
					}
					else
					{
						// 附加 DEBUG 时间戳
						if(data_type->video_timestamp < arg->video_enc[index].BD_video_timestamp_cache)
						{
							nslog(NS_ERROR,"<VIDEO> IMPORT_ERROR Wrong sequence  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].BD_video_timestamp_cache);
							data_type->video_timestamp = arg->video_enc[index].BD_video_timestamp_cache + 1;
						}

						if(data_type->video_timestamp == arg->video_enc[index].BD_video_timestamp_cache)
						{
							nslog(NS_ERROR,"<VIDEO equal > IMPORT_ERROR Wrong sequence  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].BD_video_timestamp_cache);
							data_type->video_timestamp = arg->video_enc[index].BD_video_timestamp_cache + 1;
						}
						#if 0
						if(data_type->video_timestamp - arg->video_enc[index].BD_video_timestamp_cache > 70)
						{
							nslog(NS_ERROR,"<VIDEO> IMPORT_ERROR time out  <SINDEX : %d> <MSG_TYPE : %d> <NEW_TIME: %u> <OLD_TIME : %u>\n",
								index,msg_type,data_type->video_timestamp,arg->video_enc[index].BD_video_timestamp_cache);
						}

						#endif
						arg->video_enc[index].BD_video_timestamp_cache = data_type->video_timestamp;
						data_type->video_timestamp = data_type->video_timestamp - arg->video_enc[index].BD_video_timestamp;
						if(data_type->video_timestamp < 0 )
						{
							nslog(NS_ERROR ,"DATA_TIME_STAMP : %d ---- CACHE_TIME_STAMP : %d <SINDEX : %d > <H264_BD>\n",
								data_type->video_timestamp,
								arg->video_enc[index].BD_video_timestamp,
								index);
						}
						// 附加解决时间戳爆掉问题 应用方案1.
						if(data_type->video_timestamp > 0XFFFFFFFF/90 - 10000)
						//if(data_type->video_timestamp > 300000)  // 5min
						{
							arg->video_enc[index].BD_video_timestamp = 0;
						}
					}
#endif
					arg->video_enc[index].BD_recv_flag	=	1;
					data_type->data_quqlity = VIDEO_DATA_TYPE_BD;
					data_type->data_type = VIDEO_H264;
					data_type->video_enc_addr = &(arg->video_enc[index].BD_pack_seq);
					break;
				}
			}
		}
		else if(arg->video_enc[index].enc_type == VIDEO_JPEG)
		{
			#if 1
			if(arg->video_enc[index].HD_video_timestamp == 0)
			{
				arg->video_enc[index].HD_video_timestamp = data_type->video_timestamp;
				data_type->video_timestamp = 0;
			}
			else
			{
				data_type->video_timestamp = data_type->video_timestamp - arg->video_enc[index].HD_video_timestamp;

				// 附加解决时间戳爆掉问题 应用方案1.
				if(data_type->video_timestamp > 0XFFFFFFFF/90 - 10000)
				//if(data_type->video_timestamp > 300000)  // 5min
				{
					arg->video_enc[index].HD_video_timestamp = 0;
				}
			}
			#endif

			if(arg->video_enc[index].HD_video_sindex == msg_type )
			{
				arg->video_enc[index].HD_recv_flag	=	1;
				data_type->data_quqlity = VIDEO_DATA_TYPE_HD;
				data_type->data_type = VIDEO_JPEG;
				data_type->video_enc_addr = &(arg->video_enc[index].HD_pack_seq);
				break;
			}
		}
		else
		{
			nslog(NS_ERROR,"arg->video_enc.enc_type is error --- enc_type: %d\n",arg->video_enc[index].enc_type);
			return OPERATION_ERR;
		}
	}
	if(index < arg->enc_num)
	{
		data_type->video_sindex 	= index;
		switch(index)
		{
			case 0:
				data_type->data_sindex = '1';
				break;
			case 1:
				data_type->data_sindex = '2';
				break;
			case 2:
				data_type->data_sindex = '3';
				break;
			case 3:
				data_type->data_sindex = '4';
				break;
			case 4:
				data_type->data_sindex = '5';
				break;
			case 5:
				data_type->data_sindex = '6';
				break;
			case 6:
				data_type->data_sindex = '7';
				break;
			case 7:
				data_type->data_sindex = '8';
				break;
			case 8:
				data_type->data_sindex = '9';
				break;
			default:
				nslog(NS_ERROR,"HERE IS A ERROR!\n");
				break;
		}
	}
	else
	{
		debug_data_flag ++ ;
		if(debug_data_flag == 1000)
		{
			nslog(NS_INFO,"not find the data_type with the msg : %d, index = %d, enc_num = %d\n",msg_type, index, arg->enc_num);
			debug_data_flag = 0;
		}
		return OPERATION_ERR;
	}
	return OPERATION_SUCC;
}

// *****************************************************
//function	: 音频采样率索引值与实际值转换
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t switch_audio_sample_rate(int32_t sample_rate)
{
	int32_t sample_index = 0;
	switch(sample_rate)
	{
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

// *****************************************************
//function	: 根据帧头信息赋值RTP 扩展头
//author 	: zhengyb		2012.12.7
//******************************************************

void set_hdb_freame_head(FRAMEHEAD *fh, parse_data_t *pd)
{

	if(R_VIDEO == pd->data_type)
	{
		fh->codec 	= 0;
		fh->framerate 	= pd->sample_rate;
		fh->height 		= pd->height;
		fh->width 		= pd->width;
		fh->samplerate	= 0;
		fh->framelength	= pd->data_len;
		fh->reserve		= 0;
		fh->Iframe		= 0;
		if(pd->flags != 0)
		{
			fh->Iframe = 1;
		}

	}
	else if(R_JPEG == pd->data_type)
	{
		fh->codec 	= 2;

		fh->framerate 	= pd->sample_rate;
		fh->height 		= pd->height;
		fh->width 		= pd->width;
		fh->samplerate	= 0;
		fh->framelength	= pd->data_len;
		fh->reserve		= 0;
		fh->Iframe		= 0;
		if(pd->flags != 0)
		{
			fh->Iframe = 1;
		}
	}
	else if(R_AUDIO == pd->data_type)
	{
		fh->codec 		= 1;
		fh->framerate 	= 0;
		fh->height 		= pd->height;
		fh->width 		= pd->width;
		fh->framelength	= pd->data_len;
		fh->samplerate	= switch_audio_sample_rate(pd->sample_rate);
		fh->reserve		= 0;
		fh->Iframe 		= 0;
		if(pd->flags != 0)
		{
			fh->Iframe = 1;
		}
	}
	else
	{
		nslog(NS_ERROR,"pd->data_type is error!\n");
	}
}
// *****************************************************
//function	: 判断用户是否有发送权限
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t distinguish_send_flag(video_data_type_t *data_type,lives_user_info_t *user_info)
{
	int32_t index = 0;

	if(data_type->data_type != VIDEO_JPEG)
	{
		if(user_info->video_quality_type != data_type->data_quqlity)
		{
			return 0;
		}
	}
	for(index = 0; index < VIDEO_ENCODE_INDEX_LEN ; index ++)
	{
		if(user_info->video_encode_index[index] == data_type->data_sindex)
		{
			return 1;
		}
	}

	return 0;
}
// *****************************************************
//function	: 设定用户未发送JPEG 标记位
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t set_jpeg_send_flag(lives_user_info_t *user)
{
	user->video_jpeg_flag = JPEG_NO_SEND_FLAG;
	#if 0
	int32_t index = 0;
	for(index = 0 ;index < VIDEO_ENCODE_INDEX_LEN ; index++)
	{
		if(user->video_encode_index[index] == '3')
		{
			user->video_jpeg_flag = JPEG_NO_SEND_FLAG;
			break;
		}
	}
	#endif
	return 	OPERATION_SUCC;
}
// *****************************************************
//function	: 依据待发数据类型调用各自的接口
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t send_video_data_pdg(int8_t *send_buf ,video_send_user_t *data_user_info,int32_t data_type ,RTP_BUILD_INFO *rtp_hand)
{
	int32_t 		ret								= -1;
	parse_data_t 	*pd 						   	= (parse_data_t *)send_buf;
	FRAMEHEAD		frame_head;
	int32_t 		mtu								= MAX_VOD_MEDIA_PACK_LEN - 20;
	int32_t 		index							= 0;

	set_hdb_freame_head(&frame_head, pd);

	#if 0
	if(R_VIDEO == pd->data_type)
	{
		nslog(NS_INFO,"<R_VIDEO>  ++++++++++++++ <RTP_TIME : %u>  <TYPE : %d> \n",data_user_info->video_timestamp,pd->data_type);
	}
	else
	{
		nslog(NS_INFO,"<R_AUDIO>  ++++++++++++++ <RTP_TIME : %u>  <TYPE : %d> \n",data_user_info->video_timestamp,pd->data_type);
	}
	#endif

	if(R_VIDEO == pd->data_type)
	{
		#if 1
		ret = rtp_build_video_data(rtp_hand,pd->data_len,pd->data,pd->flags,mtu ,data_user_info->video_timestamp,(void *)data_user_info,&frame_head);
		if(ret < 0 )
		{
			nslog(NS_ERROR ,"rtp_build_video_data is error!\n");
		}
		#endif

	}
	else if(R_JPEG == pd->data_type)
	{
		#if 1
			ret = rtp_build_jpeg_data(rtp_hand,pd->data_len,pd->data,data_user_info->video_timestamp,mtu ,(void *)data_user_info,&frame_head);
			if(ret < 0 )
			{
				nslog(NS_ERROR ,"rtp_build_jpeg_data is error!\n");
			}
		#endif
	}
	else
	{
		#if 1
		ret = rtp_build_audio_data(rtp_hand,pd->data_len,pd->data,pd->sample_rate,mtu ,data_user_info->video_timestamp,(void *)data_user_info,&frame_head);
		if(ret < 0 )
		{
			nslog(NS_ERROR ,"rtp_build_audio_data is error!\n");
		}
		#endif
	}

	//  3 distinguish jpeg or not_jpeg
	if(data_type == VIDEO_JPEG)
	{
		#if 1
		for(index =0 ;index < data_user_info->user_num ; index ++)
		{
			data_user_info->user_addr[index]->video_jpeg_flag = JPEG_SEND_FLAG;
		}
		#endif
	}
	if(data_type != VIDEO_JPEG)
	{
		r_free(pd->data);
		pd->data = NULL;
		r_free(send_buf);
		send_buf = NULL;
	}
	return OPERATION_SUCC;
}

// *****************************************************
//function	: 依据待发数据获取发送用户信息
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t get_send_userinfo(video_data_type_t *data_type,video_send_user_t *data_user_info,lives_mode_hand_t *arg)
{
	int32_t index 		= 0;
	int32_t user_num 	= 0;

	if(data_type == NULL || data_user_info == NULL || arg == NULL)
	{
		return OPERATION_ERR;
	}
	lives_mode_hand_t *handle = (lives_mode_hand_t *)arg;
	lives_user_info_t *temp_user = NULL;

	data_user_info->video_sindex 		= 	data_type->video_sindex;
	data_user_info->video_timestamp	=	data_type->video_timestamp;
	data_user_info->video_enc_addr    =  	data_type->video_enc_addr;

//	nslog(NS_INFO,"date_sindex : %c \n",data_type->data_sindex);
	pthread_mutex_lock(&(handle->lives_mode_info.mutex));

	temp_user = handle->lives_mode_info.user_info;
	for(index =0;index < VIDEO_USER_MAX_NUM ;index ++)
	{
		if(temp_user->index != -1)
		{
			if(distinguish_send_flag(data_type,temp_user) == 1)
			{

			//	nslog(NS_INFO,"IP : %s \n",temp_user->lives_user_addr[data_type->video_sindex].m_user_ip);
				r_memcpy(data_user_info->user_addr_info[user_num].user_addr_info.m_user_ip,temp_user->lives_user_addr[data_type->video_sindex].m_user_ip ,VIDEO_SEND_IP_LEN);
				data_user_info->user_addr_info[user_num].user_addr_info.m_user_port = temp_user->lives_user_addr[data_type->video_sindex].m_user_port;
				data_user_info->user_addr_info[user_num].udp_fd = temp_user->data_udp_fd ;
			//	nslog(NS_INFO,"IP : %s \n",data_user_info->user_addr_info[user_num].m_user_ip);

			// debug   user_addr ???
				data_user_info->user_addr[user_num] = temp_user;
				user_num ++ ;
				data_user_info->user_num ++ ;

				// debug
				temp_user->user_debug_info[data_type->video_sindex].user_recv_flag = 1;
				temp_user->user_debug_info[data_type->video_sindex].video_sindex = data_type->video_sindex;
			}
		}
		temp_user++;
	}
	if(user_num == 0)
	{
	// 用户数位	0 这里FREE AND RETURN
	//	nslog(NS_ERROR,"LIVE_MODE FIND ------ <date_sindex : %c >",data_type->data_sindex);
		pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
		return OPERATION_ERR;
	}
	pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
	return OPERATION_SUCC;
}


// *****************************************************
//function	: 获取需要发送JPEG 数据用户信息
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t get_send_userinfo_jpeg(int32_t video_sindex,video_send_user_t *data_user_info,lives_mode_hand_t *arg)
{
	int32_t index = 0;
	int32_t user_index = 0;
	if(data_user_info == NULL || arg == NULL)
	{
			return OPERATION_ERR;
	}
	lives_mode_hand_t *handle = (lives_mode_hand_t *)arg;
	lives_user_info_t *temp_user = arg->lives_mode_info.user_info;

	data_user_info->video_sindex = video_sindex;
	pthread_mutex_lock(&(handle->lives_mode_info.mutex));
	for(index = 0 ;index <VIDEO_USER_MAX_NUM;index ++)
	{
		if(temp_user->index != -1)
		{
			if(temp_user->video_jpeg_flag == JPEG_NO_SEND_FLAG )
			{
				r_memcpy(data_user_info->user_addr_info[user_index].user_addr_info.m_user_ip,temp_user->lives_user_addr[video_sindex].m_user_ip ,VIDEO_SEND_IP_LEN);
				data_user_info->user_addr_info[user_index].user_addr_info.m_user_port = temp_user->lives_user_addr[video_sindex].m_user_port;
				data_user_info->user_addr_info[user_index].udp_fd = temp_user->data_udp_fd;
				data_user_info->user_addr[user_index] = temp_user;
				user_index ++ ;
				data_user_info->user_num ++ ;
				// debug
	//			nslog(NS_INFO,"Important information <FIRST SEND JPEG>: USER_ADDR : %s ----USER : %d\n",temp_user->lives_user_addr[video_sindex].m_user_ip,temp_user->lives_user_addr[video_sindex].m_user_port);
			}
			temp_user++;
		}
	}
	pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
	// 用户数为0 这里直接return
	return OPERATION_SUCC;
}

// *****************************************************
//function	: 处理JPEG 缓冲发送给未发送jpeg用户
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t deal_video_jpeg_data(lives_mode_hand_t *arg)
{
	int32_t video_sindex = -1;
	int32_t index = 0;

	RTP_BUILD_INFO							*temp_rtp_hand =NULL;

	video_data_type_t data_type;
	data_type.data_quqlity					= -1;
	data_type.data_sindex					= '0';
	data_type.data_type						= -1;
	data_type.video_sindex					= -1;
	data_type.video_timestamp				= 0;
	data_type.video_enc_addr				= NULL;

	video_send_user_t data_user_info;
	data_user_info.user_num					= 0;
	data_user_info.video_sindex				= -1;
	data_user_info.video_timestamp			= 0;
	data_user_info.video_enc_addr			= NULL;
	for(index = 0; index < VIDEO_USER_MAX_NUM ;index ++)
	{
		r_memset(data_user_info.user_addr_info[index].user_addr_info.m_user_ip , 0 ,VIDEO_SEND_IP_LEN);
		data_user_info.user_addr_info[index].user_addr_info.m_user_port = 0;
		data_user_info.user_addr[index] = NULL;
	}

	// 直播模块中是否备份有有效JPEG CACHE
	if(arg->lives_mode_info.jpeg_valid_flag != 0)
	{
//		nslog(NS_INFO,"live_mode no jpeg cache!\n");
		return OPERATION_SUCC;
	}

	// 1. get the video sindex
	video_sindex = get_video_sindex(VIDEO_JPEG , &(arg->lives_mode_info.video_sindex));
	if(video_sindex == -1)
	{
		nslog(NS_ERROR ,"get_video_sindex is error!\n");
		return OPERATION_SUCC;
	}

	if(get_send_userinfo_jpeg(video_sindex,&data_user_info,arg) < 0)
	{
		nslog(NS_ERROR ,"get_send_userinfo_jpeg is error!\n");
		return OPERATION_SUCC;
	}

	temp_rtp_hand = arg->lives_mode_info.video_sindex.video_enc[video_sindex].HD_video_rpt_hand;
	data_user_info.video_enc_addr = &(arg->lives_mode_info.video_sindex.video_enc[video_sindex].HD_pack_seq);

	if(data_user_info.user_num != 0)
	{
		nslog(NS_INFO,"XXXXXXXXXXXXXXXXXXX\n");
		send_video_data_pdg(arg->lives_mode_info.jpeg_msgque.msgbuf,&data_user_info,VIDEO_JPEG,temp_rtp_hand);
	}
	return OPERATION_SUCC;

}

// *****************************************************
//function	: 处理接受数据
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t local_media_data_pkg(msgque *revmsg,lives_mode_hand_t *arg)
{

	int32_t	index							= 0;
	int32_t ret = -1;

	RTP_BUILD_INFO							*temp_rtp_hand =NULL;

	int32_t jpeg_flag						= 0;

	parse_data_t *pd 						= (parse_data_t *)revmsg->msgbuf;
	parse_data_t *pd_temp					= NULL;

	video_data_type_t data_type;
	data_type.data_quqlity					= -1;
	data_type.data_sindex					= '0';
	data_type.data_type						= -1;
	data_type.video_sindex					= -1;
	data_type.video_timestamp				= pd->time_tick;
	data_type.video_enc_addr				= NULL;

	video_send_user_t data_user_info;
	data_user_info.user_num					= 0;
	data_user_info.video_sindex				= -1;
	data_user_info.video_timestamp			= 0;
	data_user_info.video_enc_addr			= NULL;

	// debug
	#if 0
	if(R_VIDEO == pd->data_type)
	{
		nslog(NS_WARN,"<R_VIDEO>  ----------- <RECV_TIME : %u>  <TYPE : %d> \n",data_type.video_timestamp,pd->data_type);
	}
	else
	{
		nslog(NS_WARN,"<R_AUDIO>  ----------- <RECV_TIME : %u>  <TYPE : %d> \n",data_type.video_timestamp,pd->data_type);
	}
	#endif

	for(index = 0; index < VIDEO_USER_MAX_NUM ;index ++)
	{
		r_memset(data_user_info.user_addr_info[index].user_addr_info.m_user_ip , 0 ,VIDEO_SEND_IP_LEN);
		data_user_info.user_addr_info[index].user_addr_info.m_user_port = 0;
		data_user_info.user_addr_info[index].udp_fd = -1;
		data_user_info.user_addr[index] = NULL;
	}

	// 附加:		正好赶上连下两个JPEG 也许会出现问题
//	deal_video_jpeg_data(arg);

#if 1
//	nslog(NS_INFO , "< %d > < %d >  m_data_codec : %d   data_type : %d \n",JPG_CODEC_TYPE,R_JPEG,hdb_data->m_data_codec,pd->data_type);
	if(pd->data_type == R_JPEG)
	{
		if(arg->lives_mode_info.jpeg_valid_flag == -1)
		{
			arg->lives_mode_info.jpeg_valid_flag = 0;

			nslog(NS_INFO,"jpeg_valid_flag IS :%d\n",arg->lives_mode_info.jpeg_valid_flag);
		}
		else
		{
			if(arg->lives_mode_info.jpeg_msgque.msgbuf != NULL)
			{
				pd_temp = (parse_data_t *)(arg->lives_mode_info.jpeg_msgque.msgbuf);
				r_free(pd_temp->data);
				pd_temp->data = NULL;
				r_free(pd_temp);
				pd_temp = NULL;
			}
			#if 0
			else
			{
				if(arg->lives_mode_info.jpeg_msgque.msgbuf != NULL)
				{
					pd_temp = (parse_data_t *)arg->lives_mode_info.jpeg_msgque.msgbuf;
					r_free(pd_temp->data);
					pd_temp->data = NULL;
					r_free(pd_temp);
					pd_temp = NULL;
				}
				arg->lives_mode_info.jpeg_valid_flag = -1;
				arg->lives_mode_info.jpeg_msgque.msgtype = -1;
				arg->lives_mode_info.jpeg_msgque.msgbuf = NULL;
				nslog(NS_ERROR,"The msgque is error --- buf_addr : %p --- msg_type : %d\n",arg->lives_mode_info.jpeg_msgque.msgbuf,arg->lives_mode_info.jpeg_msgque.msgtype);
			}
			#endif
		}

		arg->lives_mode_info.jpeg_msgque.msgbuf  	= revmsg->msgbuf;
		arg->lives_mode_info.jpeg_msgque.msgtype 	= revmsg->msgtype;
		jpeg_flag = 1;
	}
#endif

	if(arg->lives_mode_info.user_num == 0)
	{
		if(jpeg_flag == 0)
		{
			r_free(pd->data);
			pd->data = NULL;
			r_free(pd);
			pd = NULL;
		}
		return OPERATION_SUCC;
	}

	// 1. get the data_type
	if(get_video_data_info(revmsg,&(arg->lives_mode_info.video_sindex),&data_type) < 0 )
	{
		if(jpeg_flag == 0)
		{
			r_free(pd->data);
			pd->data = NULL;
			r_free(pd);
			pd = NULL;
		}
		return OPERATION_ERR;
	}
	// 2. get the user_info
	if(get_send_userinfo(&data_type,&data_user_info,arg) < 0)
	{
		if(jpeg_flag == 0)
		{
			r_free(pd->data);
			pd->data = NULL;
			r_free(pd);
			pd = NULL;
		}
		return OPERATION_ERR;
	}


	// 3.   set RTP_BUILD_INFO
	if(R_VIDEO == pd->data_type)
	{
		if(data_type.data_quqlity == VIDEO_DATA_TYPE_HD)
		{
			temp_rtp_hand = arg->lives_mode_info.video_sindex.video_enc[data_type.video_sindex].HD_video_rpt_hand;
		}
		else
		{
			temp_rtp_hand = arg->lives_mode_info.video_sindex.video_enc[data_type.video_sindex].BD_video_rpt_hand;
		}
	}
	else if(R_JPEG == pd->data_type)
	{
		temp_rtp_hand = arg->lives_mode_info.video_sindex.video_enc[data_type.video_sindex].HD_video_rpt_hand;
	}
	else
	{
		if(data_type.data_quqlity == VIDEO_DATA_TYPE_HD)
		{
			temp_rtp_hand = arg->lives_mode_info.video_sindex.video_enc[data_type.video_sindex].HD_audio_rpt_hand;
		}
		else
		{
			temp_rtp_hand = arg->lives_mode_info.video_sindex.video_enc[data_type.video_sindex].BD_audio_rpt_hand;
		}
	}

	// 4.   send video
	if(data_user_info.user_num != 0)
	{
		send_video_data_pdg(revmsg->msgbuf,&data_user_info,data_type.data_type,temp_rtp_hand);
	}
	#if 0
	if(jpeg_flag == 0)
	{
		deal_video_jpeg_data(arg);
	}
	#endif
	return OPERATION_SUCC;
}

// *****************************************************
//function	: 设定发送用户信息表
//author 	: zhengyb   2012.12.7
//******************************************************

int32_t set_lives_user_info(lives_user_info_t *user , void *arg)
{
	lives_mode_hand_t *handle = (lives_mode_hand_t *)arg;
	int32_t index 		= 0;
	int32_t index_ex 	= 0 ;
	int32_t ret			= -1;
	int32_t	udp_port	= 0;
	static int32_t live_req_count = 0;
	static int32_t live_req_succ_count = 0;

	pthread_mutex_lock(&(handle->lives_mode_info.mutex));
	nslog(NS_INFO,"xxxxx!\n");

	for(index = 0; index < VIDEO_ENCODE_MAX_NUM; index++){
		nslog(NS_INFO, "ip: %s  port:%d", user->lives_user_addr[index].m_user_ip,
								user->lives_user_addr[index].m_user_port);
	}

	if(handle->lives_mode_info.user_num == VIDEO_USER_MAX_NUM)
	{
		pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
		return OPERATION_ERR;
	}

	for(index = 0;index < VIDEO_USER_MAX_NUM ;index++)
	{
		ret = compare_user_info(user,&(handle->lives_mode_info.user_info[index]));
		if(ret == 0)
		{
			pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
			nslog(NS_ERROR,"NS_INFO,THE USER IS HAVEING!\n");
			return OPERATION_SUCC;
		}
	}

	for(index =0 ;index <VIDEO_USER_MAX_NUM ; index ++)
	{
		if(handle->lives_mode_info.user_info[index].index == -1)
		{
			// create
			udp_port = VIDEO_LOCAL_PORT + 20*(handle->lives_mode_info.room_id) + index;
			nslog(NS_INFO , "---<USER_ADDR : %s > --- <USER_DATA_PORT : %d >---\n",handle->lives_mode_info.user_info[index].lives_user_addr[0].m_user_ip,udp_port);
			handle->lives_mode_info.user_info[index].data_udp_fd = create_video_local_socket(udp_port);
			if(handle->lives_mode_info.user_info[index].data_udp_fd < 0)
			{
				nslog(NS_ERROR , "Create the socket of user is error!");
				pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
				return OPERATION_ERR;
			}

			handle->lives_mode_info.user_info[index].video_quality_type 	= user->video_quality_type;
			handle->lives_mode_info.user_info[index].user_id				= user->user_id;
			r_memcpy(handle->lives_mode_info.user_info[index].video_encode_index,user->video_encode_index,VIDEO_ENCODE_INDEX_LEN);
			for(index_ex = 0; index_ex < VIDEO_ENCODE_MAX_NUM ;index_ex ++)
			{
				r_memcpy(handle->lives_mode_info.user_info[index].lives_user_addr[index_ex].m_user_ip , user->lives_user_addr[index_ex].m_user_ip,VIDEO_SEND_IP_LEN);
				handle->lives_mode_info.user_info[index].lives_user_addr[index_ex].m_user_port = user->lives_user_addr[index_ex].m_user_port;
	//			r_memcpy(handle->lives_mode_info.user_info[index].lives_user_addr[index_ex].m_user_ip , "192.168.4.18",VIDEO_SEND_IP_LEN);
	//			handle->lives_mode_info.user_info[index].lives_user_addr[index_ex].m_user_port = 23000+2*index_ex;
			}
			handle->lives_mode_info.user_info[index].vaild_flag = 1;			//开启
			handle->lives_mode_info.user_num ++ ;

			set_jpeg_send_flag(&(handle->lives_mode_info.user_info[index]));
			handle->lives_mode_info.user_info[index].index = index ;

			break;
		}
		else
		{
			nslog(NS_INFO,"ip is %s \n",handle->lives_mode_info.user_info[index].lives_user_addr[0].m_user_ip);

		}
	}
#if 0
	for(index =0 ;index <VIDEO_USER_MAX_NUM ; index ++)
	{
		//if(handle->lives_mode_info.user_info[index].index != -1)
		{
			printf("user_index is %d\n",handle->lives_mode_info.user_info[index].index);
			printf("set user quality is -------------- %d\n",handle->lives_mode_info.user_info[index].video_quality_type);
		}
	}
#endif
#if 0
	for(index =0 ;index <VIDEO_USER_MAX_NUM ; index ++)
	{
	//	if(handle->lives_mode_info.user_info[index].index != -1)
		{
	//		printf("yanzheng : ip is %s \n",handle->lives_mode_info.user_info[index].lives_user_addr[0].m_user_ip);

		}
	}
#endif

	// add zl
	if(Interruption_msg(handle->lives_mode_info.msgid)< 0)
	{
		pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
		return OPERATION_ERR;
	}

	pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
	return OPERATION_SUCC;
}
// *****************************************************
//function	: 依据用户IP 以及端口比较异或
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t compare_user_info(lives_user_info_t *local_user,lives_user_info_t *net_user)
{
	int32_t index = 0 ;
	int32_t	ret   = 0;
	for(index = 0;index < VIDEO_ENCODE_MAX_NUM ; index ++)
	{
		if(r_strcmp(local_user->lives_user_addr[index].m_user_ip , net_user->lives_user_addr[index].m_user_ip) != 0)
		{
			ret = -1;
			break;
		}
		if(local_user->lives_user_addr[index].m_user_port != net_user->lives_user_addr[index].m_user_port)
		{
			ret = -1;
			break;
		}
	}
	return ret ;
}
// *****************************************************
//function	: 停止某发送用户清除其信息(用户主动发送)
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t stop_lives_user_info(lives_user_info_t *user,void *arg)
{
	lives_mode_hand_t *handle = (lives_mode_hand_t *)arg;
	int32_t index 		= 0;
	int32_t ret 		= -1;
	static int32_t stop_req_count = 0;
	static int32_t stop_req_succ_count = 0;
	pthread_mutex_lock(&(handle->lives_mode_info.mutex));
	nslog(NS_INFO,"yyyyy!\n");

	for(index = 0; index < VIDEO_ENCODE_MAX_NUM; index++){
		nslog(NS_INFO, "ip: %s  port:%d", user->lives_user_addr[index].m_user_ip,
								user->lives_user_addr[index].m_user_port);
	}

	if(handle->lives_mode_info.user_num == 0)
	{

		pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
		return OPERATION_ERR;
	}
	for(index = 0;index < VIDEO_USER_MAX_NUM ;index++)
	{
		ret = compare_user_info(user,&(handle->lives_mode_info.user_info[index]));
		if(ret == 0)
		{
			nslog(NS_INFO, "=========== %d, %s, %d\n", handle->lives_mode_info.user_info[index].user_id,
														handle->lives_mode_info.user_info[index].lives_user_addr[0].m_user_ip,
														handle->lives_mode_info.user_info[index].lives_user_addr[0].m_user_port);

			handle->lives_mode_info.user_info[index].vaild_flag = 0; 				//关闭

			// add zl
			if(Interruption_msg(handle->lives_mode_info.msgid)< 0)
			{
				pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
				return OPERATION_ERR;
			}
			pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
			return OPERATION_SUCC;
		}
	}

	pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
	return OPERATION_ERR;
}

// *****************************************************
//function	: 停止某发送用户清除其信息(用户异常掉线由中控转发)
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t stop_lives_user_info_unusual(int32_t user_id,void *arg)
{
	lives_mode_hand_t *handle = (lives_mode_hand_t *)arg;
	int32_t index 		= 0;
	int32_t ret 		= -1;
	static int32_t unusual_stop_count = 0;
	pthread_mutex_lock(&(handle->lives_mode_info.mutex));
	if(handle->lives_mode_info.user_num == 0)
	{
		pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
		return OPERATION_ERR;
	}
	for(index = 0;index < VIDEO_USER_MAX_NUM ;index++)
	{
		if(handle->lives_mode_info.user_info[index].user_id == user_id)
		{
			nslog(NS_INFO, "||||||||||||| %d, %s, %d\n", handle->lives_mode_info.user_info[index].user_id,
														handle->lives_mode_info.user_info[index].lives_user_addr[0].m_user_ip,
														handle->lives_mode_info.user_info[index].lives_user_addr[0].m_user_port);
			handle->lives_mode_info.user_info[index].vaild_flag = 0; 				//关闭

			// add zl
			if(Interruption_msg(handle->lives_mode_info.msgid)< 0)
			{
				pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
				return OPERATION_ERR;
			}
			pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
			return OPERATION_SUCC;
		}
	}
	// add zl
//	if(Interruption_msg(handle->lives_mode_info.msgid)< 0)
//	{
//		pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
//		return OPERATION_ERR;
//	}
	pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
	return OPERATION_ERR;
}


// *****************************************************
//function	: 停止改教室的所有用户
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t stop_lives_user_all_unusual(void *arg)
{
	lives_mode_hand_t *handle = (lives_mode_hand_t *)arg;
	int32_t index 		= 0;
	int32_t ret 		= -1;

	pthread_mutex_lock(&(handle->lives_mode_info.mutex));
	if(handle->lives_mode_info.user_num == 0)
	{
		pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
		return OPERATION_ERR;
	}
	for(index = 0;index < VIDEO_USER_MAX_NUM ;index++)
	{
		if(handle->lives_mode_info.user_info[index].vaild_flag != -1)
		{
			nslog(NS_INFO, "~~~~~~~~~~~~ %d, %s, %d\n", handle->lives_mode_info.user_info[index].user_id,
														handle->lives_mode_info.user_info[index].lives_user_addr[0].m_user_ip,
														handle->lives_mode_info.user_info[index].lives_user_addr[0].m_user_port);
			handle->lives_mode_info.user_info[index].vaild_flag = 0; 				//关闭
		}
	}
	// add zl
	if(Interruption_msg(handle->lives_mode_info.msgid)< 0)
	{
		pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
		return OPERATION_ERR;
	}
	pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
	return OPERATION_SUCC;
}


// *****************************************************
//function	: 创建直播模块接受消息队列主线程
//author 	: zhengyb		2012.12.7
//******************************************************

void *lives_mode_thread(void *arg)
{
	lives_mode_hand_t *lives_mode_hand 	= (lives_mode_hand_t *)arg;
	int32_t recvlen							= 0;
	msgque	revmsg ;
	parse_data_t *pd 						= NULL;
	parse_data_t *temp_pd 						= NULL;
	int32_t index 							= 0;
	int32_t index_ex 						= 0;

	// debug wirte file
	int8_t jpeg_file_name[24]				= {0};
	int32_t jpeg_file_index				= 0;

	int32_t write_flag = 0;
	while(lives_mode_hand->lives_mode_info.mode_valid_flag)
	{
		// zusai
		recvlen = r_msg_recv(lives_mode_hand->lives_mode_info.msgid, &revmsg, sizeof(msgque)-sizeof(long), (long)0, 0);
		if(recvlen < 0)
		{
			nslog(NS_ERROR, "package msgrcv failed, errmsg = %s, recvlen = %d, errno = %d\n",strerror(errno), recvlen, errno);
			r_usleep(10000);
			continue;
		}

		if(revmsg.msgtype == INTERRUPTION_MSG_ID)
		{
			nslog(NS_ERROR,"RECV	 INTERRUPTION_MSG !!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			//重置编码器信息
			reset_live_mode_enc(lives_mode_hand);
			//重置用户信息
			reset_live_mode_user(lives_mode_hand);
			//无用户重置编码器时间戳基准值
			reset_live_mode_enc_time(lives_mode_hand);
			nslog(NS_ERROR,"SUCESS	 INTERRUPTION_MSG !!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			continue;
		}

		//预处理JPEG
		deal_video_jpeg_data(lives_mode_hand);
		pd = (parse_data_t *)(revmsg.msgbuf);

	//	nslog(NS_ERROR, "revmsg.msgtype = %ld\n", revmsg.msgtype);
	//	nslog(NS_ERROR, "revmsg.msgbuf = %p\n", revmsg.msgbuf);
//		nslog(NS_ERROR, "pd->data = %p\n", pd->data);

//		nslog(NS_ERROR, "..........................\n");
//		pd = (parse_data_t *)(revmsg.msgbuf);

		#if 0	// debug zhengyb
		if(revmsg.msgtype == 5)
		{
			jpeg_file_index ++ ;
			r_memset(jpeg_file_name , 0,24);
			pd = (parse_data_t *)(revmsg.msgbuf);
			sprintf(jpeg_file_name, "jpeg_cache_%d.jpg", jpeg_file_index);
			printf("jpeg file name is ---------------- %s\n",jpeg_file_name);

			FILE *fp = NULL;
			fp = fopen(jpeg_file_name, "a");
			if (fp == NULL) {
				printf("NS_ERROR, fopen error");
			}
			write_long_tcp_data(fp, pd->data, pd->data_len);
			fclose(fp);
		}
		#endif

		local_media_data_pkg(&revmsg,lives_mode_hand);

	}

	// free msg_queue
	while(1)
	{
		recvlen = r_msg_recv(lives_mode_hand->lives_mode_info.msgid, &revmsg, sizeof(msgque)-sizeof(long),(long)0, IPC_NOWAIT);

		if(recvlen >= 0)
		{
			pd = (parse_data_t *)(revmsg.msgbuf);
			if(revmsg.msgbuf != NULL)
			{
				if(pd->data != NULL)
				{
					r_free(pd->data);
					pd->data = NULL;
				}
				r_free(revmsg.msgbuf);
				revmsg.msgbuf = NULL;
			}
		}
		else
		{
			break;
		}
	}

	for(index = 0; index < 15;index ++)
	{
		if(lives_mode_hand->lives_mode_info.debug_thread_valid_flag == 0)
		{
			 // 1.  msg queue
			 if(lives_mode_hand->lives_mode_info.jpeg_msgque.msgbuf != NULL)
			 {
			 	temp_pd= (parse_data_t *)(lives_mode_hand->lives_mode_info.jpeg_msgque.msgbuf);
			 	if(temp_pd->data != NULL)
			 	{
			 		r_free(temp_pd->data);
					temp_pd->data = NULL;
			 	}
			 	r_free(lives_mode_hand->lives_mode_info.jpeg_msgque.msgbuf);
				lives_mode_hand->lives_mode_info.jpeg_msgque.msgbuf = NULL;
			 }
			 // 2. encode info
			 for(index_ex =0 ;index_ex < VIDEO_ENCODE_MAX_NUM ; index_ex ++)
			 {
			 	#if 0
				if(index_ex == 0)
				{
					rtp_build_uninit((void **)&(lives_mode_hand->lives_mode_info.video_sindex.video_enc[index_ex].BD_audio_rpt_hand));
			 		rtp_build_uninit((void **)&(lives_mode_hand->lives_mode_info.video_sindex.video_enc[index_ex].HD_audio_rpt_hand));
				}
				#endif
				rtp_build_uninit((void **)&(lives_mode_hand->lives_mode_info.video_sindex.video_enc[index_ex].BD_audio_rpt_hand));
				rtp_build_uninit((void **)&(lives_mode_hand->lives_mode_info.video_sindex.video_enc[index_ex].HD_audio_rpt_hand));
			 	rtp_build_uninit((void **)&(lives_mode_hand->lives_mode_info.video_sindex.video_enc[index_ex].BD_video_rpt_hand));
			 	rtp_build_uninit((void **)&(lives_mode_hand->lives_mode_info.video_sindex.video_enc[index_ex].HD_video_rpt_hand));

			 }
			// 3. free mutex
			pthread_mutex_destroy(&(lives_mode_hand->lives_mode_info.mutex));

		 	nslog(NS_ERROR,"UNREGISTER LIVE MODE IS OK!");
			return	(void *)OPERATION_SUCC;
		}
		r_sleep(1);
	}

	nslog(NS_ERROR,"REGISTER LIVE MODE IS ERROR!");
	return	(void *)OPERATION_ERR;
}

// *****************************************************
//function	:  创建直播模块debug 信息日志线程
//author 	:  zhengyb		2012.12.7
//******************************************************

void *lives_debug_thread(void *arg)
{
	lives_mode_hand_t *lives_mode_hand 	= (lives_mode_hand_t *)arg;
	int32_t index = 0;
	int32_t user_index =0;
	int32_t enc_num		=	0;
	int32_t user_num	= 	0;

	lives_mode_hand->lives_mode_info.debug_thread_valid_flag = 1;

	while(lives_mode_hand != NULL && lives_mode_hand->lives_mode_info.mode_valid_flag == 1)
	{
		enc_num		=	lives_mode_hand->lives_mode_info.video_sindex.enc_num;
		user_num	= 	lives_mode_hand->lives_mode_info.user_num;
		// 1 debug lives info
		nslog(NS_DEBUG,"LIVE_DEBUG: <ECV_NUM : %d>-----------------------------------------------------------------  ",enc_num);
		for(index =0; index < enc_num ; index ++)
		{
			nslog(NS_DEBUG,"<VIDEO_INDEX : %d ><DATA_TYEP : %d ><HD_MSG_TYPE : %d ><BD_MSG_TYPE : %d ><HD_RECV_FLAG : %d ><BD_RECV_FLAG : %d >",
					index,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].enc_type,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].HD_video_sindex,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].BD_video_sindex,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].HD_recv_flag,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].BD_recv_flag);

			lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].BD_recv_flag = 0;
			lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].HD_recv_flag = 0;
		}

		// 2. debug user info

		nslog(NS_DEBUG,"USER_DEBUG: <USER_NUM : %d>-----------------------------------------------------------------  ",user_num);
		for(user_index = 0; user_index < VIDEO_USER_MAX_NUM ; user_index ++)
		{
			if(lives_mode_hand->lives_mode_info.user_info[user_index].index != -1)
			{
				nslog(NS_DEBUG, "<VIDEO_ENCODE : %s >",lives_mode_hand->lives_mode_info.user_info[user_index].video_encode_index);
				for(index = 0;index < enc_num ; index ++)
				{
					nslog(NS_DEBUG,"<VIDEO_QUALITY : %d ><VIDEO_INDEX : %d ><USER_ID : %d><USER_PROT : %d ><USER_RECV : %d ><USER_SENG : %d ><USER_ADDR : %s >",
							lives_mode_hand->lives_mode_info.user_info[user_index].video_quality_type,
							index,
							lives_mode_hand->lives_mode_info.user_info[user_index].user_id,
							lives_mode_hand->lives_mode_info.user_info[user_index].lives_user_addr[index].m_user_port,
							lives_mode_hand->lives_mode_info.user_info[user_index].user_debug_info[index].user_recv_flag,
							lives_mode_hand->lives_mode_info.user_info[user_index].user_debug_info[index].user_send_flag,
							lives_mode_hand->lives_mode_info.user_info[user_index].lives_user_addr[index].m_user_ip);

					lives_mode_hand->lives_mode_info.user_info[user_index].user_debug_info[index].user_recv_flag = 0;
					lives_mode_hand->lives_mode_info.user_info[user_index].user_debug_info[index].user_send_flag = 0;
				}
			}
		}
		// 3. debug ecv pack seq
		nslog(NS_DEBUG,"ENC_PACK_SEQ: <USER_NUM : %d>-----------------------------------------------------------------  ",user_num);
		for(index =0; index < enc_num ; index ++)
		{
			if(index == 0)
			{

				nslog(NS_DEBUG,"[AUDIO]<ENC_INDEX : %d ><HD_CACHE : %d ><BD_CACHE : %d ><HD_LOSS : %d ><BD_LOSS : %d ><HD_TOTAL : %d ><BD_TOTAL : %d >",
					index,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_cache,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_cache,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_loss,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_loss,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_total,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_total);
			}

			nslog(NS_DEBUG,"[VIDEO] <ENC_INDEX : %d ><HD_CACHE : %d ><BD_CACHE : %d ><HD_LOSS : %d ><BD_LOSS : %d ><HD_TOTAL : %d ><BD_TOTAL : %d >",
					index,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_cache,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_cache,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_loss,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_loss,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_total,
					lives_mode_hand->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_total);

		}

		for(index =0 ;index < LIVE_DEBUG_SLEEP_TIME ; index++ )
		{
			if(lives_mode_hand->lives_mode_info.mode_valid_flag == 0)
			{
				lives_mode_hand->lives_mode_info.debug_thread_valid_flag = 0;
				return	(void *)OPERATION_SUCC;
			}
			r_sleep(1);
		}
	}

	lives_mode_hand->lives_mode_info.debug_thread_valid_flag = 0;
	return	(void *)OPERATION_SUCC;
}


// *****************************************************
//function	:  注册录播直播模块
//author 	:  zhengyb		2012.12.7
//******************************************************
lives_mode_hand_t *register_room_lives_module(int32_t room_id)
{
	lives_mode_hand_t 	*temp_hand = NULL;
//	pthread_t   		m_lives_thread;

#if 1
	pthread_attr_t	attr;
	pthread_attr_t	 attr_temp;
	struct	sched_param   param;
	struct	sched_param   param_temp;
	pthread_attr_init(&attr);
	pthread_attr_init(&attr_temp);
	pthread_attr_setschedpolicy(&attr,	SCHED_RR);
	param.sched_priority   =   60;

	if(pthread_attr_setschedparam(&attr, &param)< 0 )
	{
		nslog(NS_ERROR ,"IMPORT ERROR ,SET PRIORITY IS ERROR!\n");
	}

	if(pthread_attr_getschedparam(&attr_temp ,&param_temp) < 0)
	{
		nslog(NS_ERROR ,"IMPORT ERROR ,GET PRIORITY IS ERROR!\n");
	}
	else
	{
		nslog(NS_ERROR ,"IMPORT ERROR ,GET PRIORITY IS < %d >!\n",param_temp.sched_priority);
	}

#endif



	temp_hand = (lives_mode_hand_t *)r_malloc(sizeof(lives_mode_hand_t));
	temp_hand->lives_mode_info.room_id = room_id;
	nslog(NS_INFO,"register ROOM id is ------------- %d\n",room_id);
	if(init_lives_mode_info(temp_hand )< 0)
	{
		nslog(NS_ERROR,"ERROR : init_lives_mode_info is error!-----------[%s]\n",__func__);
		r_free(temp_hand);
		temp_hand = NULL;
		return  (void *)OPERATION_ERR;
	}
	if(r_pthread_create(&(temp_hand->lives_mode_info.main_thread_handle), &attr,lives_mode_thread,temp_hand))
	{
		nslog(NS_ERROR,"ERROR : create the mode thread is error!-----------[%s]\n",__func__);
		r_free(temp_hand);
		temp_hand = NULL;
		return  (void *)OPERATION_ERR;
	}

	// create log thread
#if 1
	if(r_pthread_create(&(temp_hand->lives_mode_info.debug_thread_headle), NULL,lives_debug_thread,temp_hand))
	{
		nslog(NS_ERROR,"ERROR : create the debug thread is error!-----------[%s]\n",__func__);
		r_free(temp_hand);
		temp_hand = NULL;
		return	(void *)OPERATION_ERR;
	}
#endif

	return temp_hand;

}
// *****************************************************
//function	: 注销录播直播模块
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t unregister_room_lives_module(lives_mode_hand_t *handle)
{
	if(handle != NULL)
	{
		handle->lives_mode_info.mode_valid_flag = 0;

		r_pthread_join(handle->lives_mode_info.debug_thread_headle, NULL);
		r_pthread_join(handle->lives_mode_info.main_thread_handle, NULL);

		r_free(handle);
		handle = NULL;
	}
	else
	{
		nslog(NS_INFO,"THE HANDLE IS ERROR! <HANDLE_ADDR : %p>\n",handle);
	}
	return OPERATION_SUCC;
}

// *****************************************************
//function	: 提供给外面用户发送数据
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t send_rtp_video(void *arg , int8_t *send_buf,int32_t len)
{
	int32_t index 						= 0;
	int32_t ret 						= -1;
	int32_t temp_len 					= 0;

	//debug
	RTP_FIXED_HEADER					*rtp_head	  		= NULL;
	FRAMEHEAD							*frame_head 		= NULL;
	int32_t								loss_packet_num	= 0;

	int32_t 							loss_pack_falg		= 0;

	if(arg == NULL || send_buf == NULL)
	{
		return OPERATION_ERR;
	}
	video_send_user_t *user = (video_send_user_t *)arg;

	#if 1
	rtp_head    	= (RTP_FIXED_HEADER *)send_buf;
	frame_head 		= (FRAMEHEAD *)(send_buf + 16);

	if(frame_head->codec == 1)
	{
		user->video_enc_addr->audio_seq_num_cache = ntohs(rtp_head->seq_no);
	}
	else
	{
		user->video_enc_addr->video_seq_num_cache = ntohs(rtp_head->seq_no);
	}
	#endif

//	if( pkg_num %10 == 0)
//	{
//		nslog(NS_INFO,"PKG_NUM : %d -----------------\n",pkg_num);
//		r_usleep(1000);
//	}

	for(index = 0; index < user->user_num ; index ++)
	{
		temp_len  = len;
	//nslog(NS_WARN, "udp_fd:[%d] m_user_port:[%d] m_user_ip:[%s]\n",
	//			user->user_addr_info[index].udp_fd,
	//			user->user_addr_info[index].user_addr_info.m_user_port,
	//			user->user_addr_info[index].user_addr_info.m_user_ip);
		ret = async_sendto_data(user->user_addr_info[index].udp_fd, (void *)send_buf, &temp_len, user->user_addr_info[index].user_addr_info.m_user_port, user->user_addr_info[index].user_addr_info.m_user_ip);
		if(ret < 0)
		{
			loss_pack_falg = 1;
		}
		(user->user_addr[index])->user_debug_info[user->video_sindex].user_send_flag = 1;
	}
	// 发送包累计  Debug
	if(frame_head->codec == 1)
	{
		user->video_enc_addr->audio_seq_num_total ++;
	}
	else
	{
		user->video_enc_addr->video_seq_num_total ++;
	}
	// 发送丢包累计 Debug
	if(loss_pack_falg == 1)
	{
		if(frame_head->codec == 1)
		{
			user->video_enc_addr->audio_seq_num_loss ++;
		}
		else
		{
			user->video_enc_addr->video_seq_num_loss ++;
		}
	}
	return OPERATION_SUCC;
}

// *****************************************************
//function	: 供外设定编码 器参数信息
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t set_lives_enc_info(video_sindex_info_t *enc_info ,void *arg)
{
	lives_mode_hand_t *handle = NULL;
	int32_t index =0;
	if(enc_info == NULL || arg == NULL)
	{
		return OPERATION_ERR;
	}

	handle = (lives_mode_hand_t *)arg;
	if(enc_info->enc_num == 0 )
	{
		if(handle->lives_mode_info.video_sindex.vaild_flag == 1)
		{
			handle->lives_mode_info.video_sindex.vaild_flag  = 0;		// 关闭
			// add zl
			if(Interruption_msg(handle->lives_mode_info.msgid)< 0)
			{
				return OPERATION_ERR;
			}
		}
		return OPERATION_SUCC;
	}
	handle->lives_mode_info.video_sindex.enc_num = enc_info->enc_num;
	handle->lives_mode_info.video_sindex.vaild_flag	 = 1;				// 开启
	for(index = 0; index < handle->lives_mode_info.video_sindex.enc_num ; index ++)
	{
		handle->lives_mode_info.video_sindex.video_enc[index].BD_video_sindex = enc_info->video_enc[index].BD_video_sindex;
		handle->lives_mode_info.video_sindex.video_enc[index].HD_video_sindex = enc_info->video_enc[index].HD_video_sindex;
		handle->lives_mode_info.video_sindex.video_enc[index].enc_type = enc_info->video_enc[index].enc_type;
	}

	// add zl
	if(Interruption_msg(handle->lives_mode_info.msgid)< 0)
	{
		return OPERATION_ERR;
	}

	return OPERATION_SUCC;
}

// *****************************************************
//function	: 复位编码 器参数信息
//author 	: zhengyb		2012.12.7
//******************************************************

int32_t reset_live_mode_enc(void *arg)
{
	lives_mode_hand_t *handle 	= (lives_mode_hand_t *)arg;
	int32_t index =0;
	parse_data_t *pd = NULL;
	if(handle->lives_mode_info.video_sindex.vaild_flag == 0)
	{
		nslog(NS_INFO,"reset_live_mode_enc\n");
		for(index = 0 ;index < VIDEO_ENCODE_MAX_NUM ; index ++)
		{
			handle->lives_mode_info.video_sindex.video_enc[index].BD_recv_flag 	= 0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_recv_flag 	= 0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_video_sindex	= -1;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_video_sindex	= -1;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_video_timestamp	= 0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_video_timestamp	= 0;
			handle->lives_mode_info.video_sindex.video_enc[index].enc_type			= -1;

			#if 0
			if(index == 0)
			{
				rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand);
				rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand);
			}
			#endif
			rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand);
			rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand);
			rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].HD_video_rpt_hand);
			rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].BD_video_rpt_hand);

			// init debug pack seq
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.data_quqlity = 1;  // 低吗
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_cache = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_loss = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_total = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_cache =0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_loss = 0 ;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_total = 0;

			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.data_quqlity = 0;	// 高嘛
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_cache = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_loss = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_total = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_cache =0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_loss = 0 ;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_total = 0;


		}
		handle->lives_mode_info.video_sindex.vaild_flag = -1;    // 复位
		handle->lives_mode_info.video_sindex.enc_num	 =0;
		// 1.JPEG 缓存复位
		if(handle->lives_mode_info.jpeg_valid_flag == 0)
		{
			handle->lives_mode_info.jpeg_valid_flag =  -1;
			// 2. 释放JPEG 缓存
			if(handle->lives_mode_info.jpeg_msgque.msgbuf != NULL)
			{
				pd = (parse_data_t *)(handle->lives_mode_info.jpeg_msgque.msgbuf);
				if(pd->data != NULL)
				{
					r_free(pd->data);
					pd->data = NULL;
				}
				r_free(pd);
				pd = NULL;
			}
		}
	}
	return OPERATION_SUCC;
}

// *****************************************************
//function	: 复位用户参数信息表
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t reset_live_mode_user(void *arg)
{
	lives_mode_hand_t *handle 	= (lives_mode_hand_t *)arg;
	int32_t index =0;

	pthread_mutex_lock(&(handle->lives_mode_info.mutex));
	for(index = 0;index < VIDEO_USER_MAX_NUM ;index++)
	{
		if(handle->lives_mode_info.user_info[index].vaild_flag == 0)
		{
			nslog(NS_INFO,"CLOSE_SOCKET IS -----------%d \n",handle->lives_mode_info.user_info[index].data_udp_fd);
			r_close(handle->lives_mode_info.user_info[index].data_udp_fd);
			init_lives_mode_info_ex(&(handle->lives_mode_info.user_info[index]));
			handle->lives_mode_info.user_num -- ;
		}
	}
	pthread_mutex_unlock(&(handle->lives_mode_info.mutex));
	return OPERATION_SUCC;
}

// *****************************************************
//function	: 无用户重置编码器时间戳基准值
//author 	: zhengyb		2012.12.7
//******************************************************
#if 1
int32_t reset_live_mode_enc_time(void *arg)
{
	lives_mode_hand_t *handle 	= (lives_mode_hand_t *)arg;
	int32_t index =0;

	if(handle->lives_mode_info.user_num != 0)
	{
		handle->lives_mode_info.video_sindex.reset_flag = 1;
		return OPERATION_SUCC;
	}

	if(handle->lives_mode_info.video_sindex.vaild_flag == 1 && handle->lives_mode_info.video_sindex.reset_flag == 1)
	{
		handle->lives_mode_info.video_sindex.reset_flag = 0;
		for(index = 0 ;index < VIDEO_ENCODE_MAX_NUM ; index ++)
		{
			handle->lives_mode_info.video_sindex.video_enc[index].HD_video_timestamp	= 0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_video_timestamp	= 0;

			handle->lives_mode_info.video_sindex.video_enc[index].HD_video_timestamp_cache	= 0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_video_timestamp_cache	= 0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_audio_timestamp_cache	= 0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_audio_timestamp_cache	= 0;

			#if 0
			if(index == 0)
			{
				rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand);
				rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand);
			}
			#endif
			rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].HD_audio_rpt_hand);
			rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].BD_audio_rpt_hand);
			rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].HD_video_rpt_hand);
			rtp_build_reset(handle->lives_mode_info.video_sindex.video_enc[index].BD_video_rpt_hand);


			// init debug pack seq
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.data_quqlity = 1;  // 低吗
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_cache = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_loss = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.audio_seq_num_total = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_cache =0;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_loss = 0 ;
			handle->lives_mode_info.video_sindex.video_enc[index].BD_pack_seq.video_seq_num_total = 0;

			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.data_quqlity = 0;	// 高嘛
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_cache = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_loss = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.audio_seq_num_total = 0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_cache =0;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_loss = 0 ;
			handle->lives_mode_info.video_sindex.video_enc[index].HD_pack_seq.video_seq_num_total = 0;
		}
	}
	return OPERATION_SUCC;
}
#endif

// *****************************************************
//function	: 依据消息队列ID 判断次编码器是否需要请求码流
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t recognition_req_strm_proc(void *arg,int32_t msgtype)
{
	lives_mode_hand_t *handle 	= (lives_mode_hand_t *)arg;
	int32_t index = 0;
	int32_t ret = -1;
	video_data_type_t data_type;
	if(arg == NULL || msgtype < 0)
	{
		nslog(NS_ERROR ,"LIVE_MODE  <ARG_ADDR : %p> <msgtype : %d>\n",arg,msgtype);
		return -1;
	}
	nslog(NS_INFO ,"ENC_NUM : %d  USER_NUM : %d  msgtype : %d \n",handle->lives_mode_info.user_num,handle->lives_mode_info.video_sindex.enc_num,msgtype);
	if(handle->lives_mode_info.user_num == 0 || handle->lives_mode_info.video_sindex.enc_num == 0)
	{
		return 0;
	}

	data_type.data_quqlity					= -1;
	data_type.data_sindex					= '0';
	data_type.data_type						= -1;
	data_type.video_sindex					= -1;
	data_type.video_timestamp				= -1;
	data_type.video_enc_addr				= NULL;

	if(get_advance_request_enc_info(msgtype,&(handle->lives_mode_info.video_sindex),&data_type) < 0)
	{
		return 0;
	}

	ret = get_advance_request_user_info(handle,&data_type);

	return ret;

}

// *****************************************************
//function	: 依据消息队列ID 与编码器信息获取视频路信息
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t get_advance_request_enc_info(int32_t msg_type,video_sindex_info_t *enc_info,video_data_type_t *data_type)
{
	int32_t index = 0;
	for(index = 0; index < enc_info->enc_num ;index ++)
	{
		if(enc_info->video_enc[index].enc_type == -1)
		{
			continue;
		}
		else if(enc_info->video_enc[index].enc_type == VIDEO_H264)
		{
			if(enc_info->video_enc[index].HD_video_sindex == msg_type )
			{
				data_type->data_quqlity = VIDEO_DATA_TYPE_HD;
				data_type->data_type = VIDEO_H264;
				break;
			}
			if(enc_info->video_enc[index].BD_video_sindex == msg_type)
			{
				data_type->data_quqlity = VIDEO_DATA_TYPE_BD;
				data_type->data_type = VIDEO_H264;
				break;
			}
		}
		else if(enc_info->video_enc[index].enc_type == VIDEO_JPEG)
		{
			if(enc_info->video_enc[index].HD_video_sindex == msg_type )
			{
				data_type->data_quqlity = VIDEO_DATA_TYPE_HD;
				data_type->data_type = VIDEO_JPEG;
				break;
			}
		}
		else
		{
			nslog(NS_ERROR ,"ENC_TYPE  IS ERROR ! <MSG_TYPE : %d>\n",msg_type);
			return OPERATION_ERR;
		}
	}
	if(index < enc_info->enc_num)
	{
		data_type->video_sindex 	= index;
		switch(index)
		{
			case 0:
				data_type->data_sindex = '1';
				break;
			case 1:
				data_type->data_sindex = '2';
				break;
			case 2:
				data_type->data_sindex = '3';
				break;
			case 3:
				data_type->data_sindex = '4';
				break;
			case 4:
				data_type->data_sindex = '5';
				break;
			case 5:
				data_type->data_sindex = '6';
				break;
			case 6:
				data_type->data_sindex = '7';
				break;
			case 7:
				data_type->data_sindex = '8';
				break;
			case 8:
				data_type->data_sindex = '9';
				break;
			default:
				nslog(NS_ERROR,"HERE IS A ERROR!\n");
				break;
		}
	}
	else
	{
		nslog(NS_INFO ,"index < enc_num !\n");
		return OPERATION_ERR;
	}
	return OPERATION_SUCC;
}
// *****************************************************
//function	: 依据路信息获取是否有用户需求请求码流
//author 	: zhengyb		2012.12.7
//******************************************************
int32_t get_advance_request_user_info(lives_mode_hand_t *arg,video_data_type_t *data_type)
{
	int32_t index 		= 0;
	int32_t ret 		= -1;

	if(data_type == NULL || arg == NULL)
	{
		return -1;
	}
	lives_user_info_t *temp_user = NULL;
	pthread_mutex_lock(&(arg->lives_mode_info.mutex));

	temp_user = arg->lives_mode_info.user_info;
	for(index =0;index < VIDEO_USER_MAX_NUM ;index ++)
	{
		if(temp_user->index != -1)
		{
			if(distinguish_send_flag(data_type,temp_user) == 1)
			{
				pthread_mutex_unlock(&(arg->lives_mode_info.mutex));
				return 1;
			}
		}
		temp_user++;
	}
	pthread_mutex_unlock(&(arg->lives_mode_info.mutex));
	return 0;
}





