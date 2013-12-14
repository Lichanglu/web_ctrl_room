
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "reach_debug.h"
#include "reach_udp_recv.h"
#include "nslog.h"
static int32_t create_data_udp_sokcet(udp_recv_handle *p_handle);
static uint32_t getNowTime(void);
static int32_t get_jpeg_frame_data_head(udp_recv_freame_head_t *frame_ptr, UdpRecv_FRAMEHEAD *rtp_frame_ptr, UdpRecv_RTP_FIXED_HEADER *rtp_head_prt);
static int32_t get_audio_frame_data(int8_t *recv_buf, int32_t recv_len, int8_t *frame_buf, int32_t *frame_len);
static int aac_payload_demux(int sample_rate, unsigned char *buf, int len, int status, unsigned char *frame_buf, unsigned int *frame_len);
static int OnAudioAacFrame(int samplerate, int frame_length, unsigned char *adts_header);
static int32_t get_video_frame_data(int8_t *recv_buf, int32_t recv_len, int8_t *frame_buf, int32_t *frame_len);
static int32_t get_video_frame_data_head(udp_recv_freame_head_t *frame_ptr, UdpRecv_FRAMEHEAD *rtp_frame_ptr, UdpRecv_RTP_FIXED_HEADER *rtp_head_prt);
static int32_t get_audio_frame_data_head(udp_recv_freame_head_t *frame_ptr, UdpRecv_FRAMEHEAD *rtp_frame_ptr, UdpRecv_RTP_FIXED_HEADER *rtp_head_prt);
static int32_t set_data_codec(UdpRecv_FRAMEHEAD *rtp_frame_ptr);
static int32_t get_jpeg_frame_data(int8_t *recv_buf, int32_t recv_len, int8_t *frame_buf, int32_t *frame_len);
static void udp_recv_close_sock(int32_t fd);
static int32_t pkg_placed_in_frame(int8_t *frame_data,  int32_t part_frame_len,  udp_recv_handle *p_udp_recv_hand);
static int32_t pkg_placed_in_frame_audio(int8_t *frame_data,  int32_t part_frame_len,  udp_recv_handle *p_udp_recv_hand);
static int32_t udp_recv_media_frame_data(udp_recv_handle *p_udp_recv_hand, void *data, int32_t data_len);
static void udp_recv_set_stream_info(udp_recv_stream_info_t *p_stream_info, udp_recv_freame_head_t *frame_ptr, uint32_t timestamp);
static int32_t switch_audio_sample_rate(int32_t sample_index);

static void udp_recv_close_sock(int32_t fd)
{
	shutdown(fd, SHUT_RDWR);
	close(fd);
}
int32_t set_recv_timeout(int32_t socket, uint32_t time)
{
	struct timeval timeout;
	int32_t ret = 0;

	timeout.tv_sec = time;
	timeout.tv_usec = 0;

	ret = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

	return ret;
}


static int32_t create_data_udp_sokcet(udp_recv_handle *p_handle)
{
	int 				udp_fd = 0;
	int 				optval 						= 1;
	socklen_t 	size 							= sizeof(struct sockaddr);
	int 				optlen							= 0;
	int 				upd_max_buf			= UdpRecv_BUF_MAX_SIZE;
	int  				ret 								= 0;
	struct sockaddr_in 	addr;

	memset((void *)&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(p_handle->stream_info.port);
	addr.sin_addr.s_addr = inet_addr(p_handle->stream_info.ip);
	udp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	setsockopt(udp_fd, SOL_SOCKET, SO_REUSEADDR, &optval, size);

	if((ret = bind(udp_fd, (struct sockaddr *)&addr, size) < 0)) {
		udp_recv_err_print("[create_data_udp_sokcet] udp_fd:[%d] is failed!!!\n", udp_fd);
		return UdpRecv_OPERATION_ERR;
	}

	ret = setsockopt(udp_fd, SOL_SOCKET, SO_RCVBUF, &upd_max_buf, sizeof(int32_t));

	if(ret < 0) {
		udp_recv_err_print("SET_SOCKET RECV_BUF IS ERROR! <SOCKET : %d> <SEND_BUF : %d > <ERROR_MESSAGE : %s >", udp_fd, upd_max_buf, strerror(errno));
		return UdpRecv_OPERATION_ERR;
	}

	optlen = sizeof(int);
	upd_max_buf = 0 ;
	ret = getsockopt(udp_fd, SOL_SOCKET, SO_RCVBUF, &upd_max_buf , &optlen);

	if(ret < 0) {
		udp_recv_err_print("GET_SOCKET RECV_BUF IS ERROR! <SOCKET : %d> <SEND_BUF : %d > <ERROR_MESSAGE : %s >", udp_fd, upd_max_buf, strerror(errno));
	}

	if(set_recv_timeout(udp_fd, 1) < 0) {
		nslog(NS_ERROR, "SET RECV_TIME IS ERROR!\n");
	} else {
		nslog(NS_ERROR, "SET RECV_TIME IS OK!\n");
	}

	UdpRecv_PRINT("udp_fd = %d, upd_max_buf = %d, id = %d, ip = %s, port = %d\n", udp_fd, upd_max_buf, p_handle->stream_num,
	              p_handle->stream_info.ip, p_handle->stream_info.port);
	return udp_fd;

}

static uint32_t getNowTime(void)
{
	struct timeval tv;
	struct timezone tz;
	uint32_t ultime;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return (ultime);
}

static int32_t set_data_codec(UdpRecv_FRAMEHEAD *rtp_frame_ptr)
{
	int32_t data_codec = 0;

	if(rtp_frame_ptr->codec == 0) {
		data_codec = UdpRecv_H264_CODEC_TYPE;
	} else if(rtp_frame_ptr->codec == 1) {
		data_codec = UdpRecv_AAC_CODEC_TYPE;
	} else if(rtp_frame_ptr->codec == 2) {
		data_codec = UdpRecv_JPEG_CODEC_TYPE;
	} else {
		data_codec = -1;
	}

	return data_codec;
}

static int32_t switch_audio_sample_rate(int32_t sample_index)
{
	int32_t sample_rate = 0;

	switch(sample_index) {
		case 0:
			sample_rate = 16000;
			break;

		case 1:
			sample_rate = 32000;
			break;

		case 2:
			sample_rate = 44100;
			break;

		case 3:
			sample_rate = 48000;
			break;

		case 4:
			sample_rate = 96000;
			break;

		default:
			sample_rate = 3;
			break;
	}

	return sample_rate;
}
void *udp_recv_deal(void *data)
{
	udp_recv_handle *p_udp_recv_hand = (udp_recv_handle *)data;
	int32_t ret = 0;
	int8_t recv_buf[UdpRecv_MAX_UDP_LEN]	= {0};
	int32_t recv_bytes	= 0;
	int8_t  in_data[UdpRecv_MAX_UDP_LEN]	= {0};
	int32_t data_len 		= 0;

	udp_recv_freame_head_t *frame_ptr			= NULL;
	UdpRecv_FRAMEHEAD *rtp_frame_ptr 				= NULL;
	UdpRecv_RTP_FIXED_HEADER *rtp_head_prt 	= NULL;

	int32_t 		data_codec 								= 0;
	int32_t 		temp_time									= 0;
	int32_t      temp_frame_flag						= 0;

	int32_t prev_pakage_num = 0;
	int32_t now_pakage_num = 0;
	int32_t lost_num 					= 0;
	//统计一下处理数据所花时间。
	uint32_t  time_now 			= 0;

	if(NULL == p_udp_recv_hand) {
		udp_recv_err_print("p_udp_recv_hand = [%p]", p_udp_recv_hand);
		goto EXIT;
	}

	UdpRecv_PRINT("udp_stream_recv_deal now");
	//pthread_detach(pthread_self());
	p_udp_recv_hand->stream_info.udp_fd = create_data_udp_sokcet(p_udp_recv_hand);

	if(0 > p_udp_recv_hand->stream_info.udp_fd) {
		udp_recv_err_print("create_data_udp_sokcet, udp_fd = %d==port=%x", p_udp_recv_hand->stream_info.udp_fd, p_udp_recv_hand->stream_info.port);
		return NULL;
	}

	nslog(NS_WARN, "create_data_udp_sokcet, udp_fd = %d==port=%x", p_udp_recv_hand->stream_info.udp_fd, p_udp_recv_hand->stream_info.port);

	while(1 == p_udp_recv_hand->pthread_status) {

		if(STREAM_START == p_udp_recv_hand->stream_status) {
			//			if(p_udp_recv_hand->stream_info.port == 0x30a5) {
			//nslog(NS_WARN, "2p_udp_recv_hand->stream_info.udp_fd:[%d]\n", p_udp_recv_hand->stream_info.udp_fd);
			//			}

			recv_bytes = recv(p_udp_recv_hand->stream_info.udp_fd, recv_buf, UdpRecv_MAX_UDP_LEN, 0);

			//			if(p_udp_recv_hand->stream_info.port == 0x30a5) {
			//nslog(NS_WARN, "p_udp_recv_hand->stream_info.udp_fd:[%d]\n", p_udp_recv_hand->stream_info.udp_fd);
			//			}

			if(recv_bytes < 1) {
				if(errno == EAGAIN || errno == EINTR) {
					//nslog(NS_ERROR,"RECV TIME OUT !\n");
					continue;
				}

				goto EXIT;
			}

			if(recv_bytes < 28) {
				udp_recv_err_print("NS_ERROR ,recv the recv_bytes is too small \n");
				goto EXIT;
			}

			time_now = getNowTime();
			frame_ptr       = (udp_recv_freame_head_t *)(in_data + Ur_MsgLen);
			rtp_head_prt 	= (UdpRecv_RTP_FIXED_HEADER *)recv_buf;
			rtp_frame_ptr 	= (UdpRecv_FRAMEHEAD *)(recv_buf + 16);
			data_len = 0;

			//malloc data space
			if(!(p_udp_recv_hand->video_data && p_udp_recv_hand->audio_data)) {
				if(!p_udp_recv_hand->video_data) {
					p_udp_recv_hand->video_data = (uint8_t *)r_malloc(UdpRecv_MAX_FRAME_LEN);

					if(NULL == p_udp_recv_hand->video_data) {
						udp_recv_err_print("malloc");
						goto EXIT;
					}
				}

				if(!p_udp_recv_hand->audio_data) {
					p_udp_recv_hand->audio_data = (uint8_t *)r_malloc(UdpRecv_MAX_FRAME_LEN);

					if(NULL == p_udp_recv_hand->audio_data) {
						udp_recv_err_print("malloc");
						goto EXIT;
					}
				}
			}

			data_codec = set_data_codec(rtp_frame_ptr);

			if(data_codec == -1) {
				udp_recv_err_print("NS_ERROR, set_data_codec is error\n ");
			}

			frame_ptr->m_data_codec = data_codec;

			//统计丢包
			if(frame_ptr->m_data_codec == UdpRecv_H264_CODEC_TYPE) {
				now_pakage_num = ntohs(rtp_head_prt->seq_no);

				if(prev_pakage_num == 0) {
					prev_pakage_num = now_pakage_num;
				}

				if((now_pakage_num - prev_pakage_num) > 1) {
					lost_num += now_pakage_num - prev_pakage_num;
					udp_recv_err_print("prev_num = %d,now_no = %d, lost_num = %d, bytes = %d, frame_len = %u, id = %d",
					                   prev_pakage_num, now_pakage_num, lost_num, recv_bytes, ntohl(rtp_frame_ptr->framelength), p_udp_recv_hand->stream_num);
					udp_recv_err_print("video:h = %u, w = %u, frame_len = %u, seq_no = %d", ntohs(rtp_frame_ptr->height),
					                   ntohs(rtp_frame_ptr->width), ntohl(rtp_frame_ptr->framelength), now_pakage_num);
				}

				// udp_recv_err_print("prev_num = %d,now_no = %d, lost_num = %d, bytes = %d, frame_len = %u, id = %d",
				//prev_pakage_num, now_pakage_num, lost_num, recv_bytes, ntohl(rtp_frame_ptr->framelength), p_udp_recv_hand->stream_num);
				prev_pakage_num = now_pakage_num;
			} else if(frame_ptr->m_data_codec == UdpRecv_AAC_CODEC_TYPE) {
				// udp_recv_err_print( "audio:h = %u, w = %u, frame_len = %u, now_no = %d, bytes = %d", ntohs(rtp_frame_ptr->height),
				//  ntohs(rtp_frame_ptr->width), ntohl(rtp_frame_ptr->framelength), ntohs(rtp_head_prt->seq_no),  recv_bytes);
			}

			// 2. 处理音频数据 and  获取发送相应数据包
			if(frame_ptr->m_data_codec == UdpRecv_AAC_CODEC_TYPE) {
				//	r_free(frame_buf);
				//	frame_buf =NULL;
				//	continue;
				// 获取音频数据
				get_audio_frame_data(recv_buf, recv_bytes, (in_data + UdpRecv_FH_LEN + Ur_MsgLen), &data_len);

				// 湖区音频数据帧头信息
				get_audio_frame_data_head(frame_ptr, rtp_frame_ptr, rtp_head_prt);

#if 1

				// 附加计算音频时间戳
				if(frame_ptr->m_frame_rate == 48000 || frame_ptr->m_frame_rate == 96000) {
					temp_time = frame_ptr->m_time_tick / 48;

				} else if(frame_ptr->m_frame_rate == 44100 || frame_ptr->m_frame_rate == 88200) {
					temp_time = (frame_ptr->m_time_tick * 10) / 441;
				} else {
					udp_recv_err_print("AUDIO_RATE IS ERROR <LOCAL_SINDEX : %d><RATE: %d>\n",
					                   p_udp_recv_hand->stream_num, frame_ptr->m_frame_rate);
				}

#endif
				frame_ptr->m_time_tick = temp_time;

				//frame_ptr->m_frame_rate = frame_ptr->m_frame_rate/2 ;
				//	frame_ptr->m_frame_rate = frame_ptr->m_frame_rate;
				//	printf("m_frame_length : %d ------------ frame_len :%d \n",frame_ptr->m_frame_length,frame_len);
				//	frame_ptr->m_frame_length = frame_len - 7; // 音频帧长要减7
				// Debug 音频比特率固定为64000
				frame_ptr->m_colors = 128000;

#if 0
				printf("< %d >\n", index);
				printf("m_id ----------- %d\n", frame_ptr->m_id);
				printf("m_time_tick ----------- %ul\n", frame_ptr->m_time_tick);
				printf("m_frame_length ----------- %d\n", frame_ptr->m_frame_length);
				printf("m_data_codec ----------- %d\n", frame_ptr->m_data_codec);
				printf("m_frame_rate ----------- %d\n", frame_ptr->m_frame_rate);
				printf("m_width ----------- %d\n", frame_ptr->m_width);
				printf("m_hight ----------- %d\n", frame_ptr->m_hight);
				printf("m_colors ----------- %d\n", frame_ptr->m_colors);
				printf("m_dw_segment ----------- %d\n", frame_ptr->m_dw_segment);
				printf("m_dw_flags ----------- %d\n", frame_ptr->m_dw_flags);
				printf("m_dw_packet_number ----------- %d\n", frame_ptr->m_dw_packet_number);
				printf("m_others ----------- %d\n\n\n", frame_ptr->m_others);
#endif
			} else if(frame_ptr->m_data_codec == UdpRecv_H264_CODEC_TYPE) {
				//	r_free(frame_buf);
				//	frame_buf =NULL;
				//	continue;
				// 获取视频数据
				temp_frame_flag = get_video_frame_data(recv_buf, recv_bytes, (in_data + UdpRecv_FH_LEN + Ur_MsgLen), &data_len);

				// 获取视频数据帧头信息
				get_video_frame_data_head(frame_ptr, rtp_frame_ptr, rtp_head_prt);
				//	nslog(NS_INFO,"---%u --- %u ---- <LOCAL_SINDEX : %d> <LOCAL_INDEXl:%d > <SERV_IP : %s > <SERV_INDEX : %d>\n",
				//		rtp_head_prt->timestamp,frame_ptr->m_time_tick,index ,local_index,local_info->m_serv_ip,local_info->m_serv_channle);

				if(temp_frame_flag == -1) {
					temp_frame_flag = 0;
#if 0
					nslog(NS_ERROR , "NS_INFO------- <LOCAL_SINDEX : %d> <LOCAL_INDEXl:%d > <SERV_IP : %s > <SERV_INDEX : %d> <FRAME_LEN : %d>\n",
					      index , local_index, local_info->m_serv_ip, local_info->m_serv_channle, frame_ptr->m_frame_length);

					nslog(NS_ERROR , "< %d >\n", index);
					nslog(NS_ERROR , "m_id ----------- %d\n", frame_ptr->m_id);
					nslog(NS_ERROR , "m_time_tick ----------- %ul\n", frame_ptr->m_time_tick);
					nslog(NS_ERROR , "m_frame_length ----------- %d\n", frame_ptr->m_frame_length);
					nslog(NS_ERROR , "m_frame_rate ----------- %d\n", frame_ptr->m_frame_rate);
					nslog(NS_ERROR , "m_width ----------- %d\n", frame_ptr->m_width);
					nslog(NS_ERROR , "m_hight ----------- %d\n", frame_ptr->m_hight);
					nslog(NS_ERROR , "m_dw_segment ----------- %d\n", frame_ptr->m_dw_segment);
					nslog(NS_ERROR , "m_dw_flags ----------- %d\n", frame_ptr->m_dw_flags);
					nslog(NS_ERROR , "m_dw_packet_number ----------- %d\n", frame_ptr->m_dw_packet_number);
#endif
				}

				frame_ptr->m_dw_segment = temp_frame_flag ;

				if(frame_ptr->m_frame_rate == 0) {
					frame_ptr->m_frame_rate = 25;
				}

				// 附加计视频时间戳
#if 1
				temp_time = frame_ptr->m_time_tick / 90;

				frame_ptr->m_time_tick = temp_time;
#endif

#if 0

				if(index == 0 && frame_ptr->m_dw_flags == 16) {
					if(frame_ptr->m_dw_segment == 2 || frame_ptr->m_dw_segment == 3) {
						if(frame_buf[4 + UdpRecv_FH_LEN + MSG_HEAD_LEN] == 0x67 || frame_buf[4 + UdpRecv_FH_LEN + MSG_HEAD_LEN] == 0x47 || frame_buf[4 + UdpRecv_FH_LEN + MSG_HEAD_LEN] == 0x27) {
							printf("i find buf! ---------- index : %d --------local_index : %d\n", index, local_index);
							I_frame_flag = 1;
						}
					}
				}



#endif
				//	frame_ptr->m_frame_length = frame_len;

				// 设定H264 视频开启写文件
#if 0//data_debug_write_flag
				nslog(NS_INFO, "11111111111\n");

				if(frame_ptr->m_dw_segment == 2 || frame_ptr->m_dw_segment == 3) {
					if(frame_buf[4 + UdpRecv_FH_LEN + MSG_HEAD_LEN] == 0x67 || frame_buf[4 + UdpRecv_FH_LEN + MSG_HEAD_LEN] == 0x47 || frame_buf[4 + UdpRecv_FH_LEN + MSG_HEAD_LEN] == 0x27) {
						//	printf("i find buf!\n");
						//	frame_ptr->m_dw_flags = 1;
						// 做异常 zhengyb
						local_info->data_sindex_ifram[index] = 1;
					}
				}

#endif
			} else if(frame_ptr->m_data_codec == UdpRecv_JPEG_CODEC_TYPE) {
				//	r_free(frame_buf);
				//	frame_buf =NULL;
				//	continue;
				//	printf("-------------frame_ptr->m_data_codec == UdpRecv_JPEG_CODEC_TYPE-------------------\n");
				// 获取音频数据
				get_jpeg_frame_data(recv_buf, recv_bytes, (in_data + UdpRecv_FH_LEN + Ur_MsgLen), &data_len);

				// 湖区音频数据帧头信息
				get_jpeg_frame_data_head(frame_ptr, rtp_frame_ptr, rtp_head_prt);

				// 设定JPEG 开启写文件
#if 0//data_debug_write_flag

				nslog(NS_INFO, "2222222222\n");

				if(frame_ptr->m_dw_segment == 2) {
					local_info->jpeg_file_index ++;
					//printf("jpeg_file_index is ---------------- %d\n",jpeg_file_index);
				}

#endif

#if 0
				printf("JPEG_CODE_TYPE is %d\n", JPEG_CODE_TYPE);
				printf("m_id ----------- %d\n", frame_ptr->m_id);
				printf("m_time_tick ----------- %d\n", frame_ptr->m_time_tick);
				printf("m_frame_length ----------- %d\n", frame_ptr->m_frame_length);
				printf("m_data_codec ----------- %d\n", frame_ptr->m_data_codec);
				printf("m_frame_rate ----------- %d\n", frame_ptr->m_frame_rate);
				printf("m_width ----------- %d\n", frame_ptr->m_width);
				printf("m_hight ----------- %d\n", frame_ptr->m_hight);
				printf("m_colors ----------- %d\n", frame_ptr->m_colors);
				printf("m_dw_segment ----------- %d\n", frame_ptr->m_dw_segment);
				printf("m_dw_flags ----------- %d\n", frame_ptr->m_dw_flags);
				printf("m_dw_packet_number ----------- %d\n", frame_ptr->m_dw_packet_number);
				printf("m_others ----------- %d\n", frame_ptr->m_others);
#endif

			} else {
				udp_recv_err_print("frame_ptr->m_data_codec is error! ---- %d\n", frame_ptr->m_data_codec);
			}

			//continue;

			ret = udp_recv_media_frame_data(p_udp_recv_hand, in_data, data_len + UdpRecv_FH_LEN + Ur_MsgLen);

			if(UdpRecv_FRAME_OK == ret && NULL != p_udp_recv_hand->func) {
				p_udp_recv_hand->stream_info.video_toal_num += 1;

				if(UdpRecv_H264_CODEC_TYPE == frame_ptr->m_data_codec || UdpRecv_JPEG_CODEC_TYPE == frame_ptr->m_data_codec) {
					udp_recv_set_stream_info(&p_udp_recv_hand->stream_info, frame_ptr, p_udp_recv_hand->udp_recv_time_t.time_video_tick);
					//nslog(NS_WARN, "[udp_recv_deal] frame_ptr->m_frame_length:[%d]\n", frame_ptr->m_frame_length);
					p_udp_recv_hand->func(&p_udp_recv_hand->stream_info, p_udp_recv_hand->video_data, frame_ptr->m_frame_length);
				} else if(UdpRecv_AAC_CODEC_TYPE == frame_ptr->m_data_codec) {
					udp_recv_set_stream_info(&p_udp_recv_hand->stream_info, frame_ptr, p_udp_recv_hand->udp_recv_time_t.time_audio_tick);
					//nslog(NS_ERROR, "[udp_recv_deal] frame_ptr->m_frame_length:[%d]\n", frame_ptr->m_frame_length);
					p_udp_recv_hand->func(&p_udp_recv_hand->stream_info, p_udp_recv_hand->audio_data, frame_ptr->m_frame_length);
				}
			}

			memset(in_data, 0, UdpRecv_MAX_UDP_LEN);
			memset(recv_buf, 0, UdpRecv_MAX_UDP_LEN);
			// udp_recv_err_print("getNowTime() = %u, time_now = %u, time_cal = %u", getNowTime(), time_now, getNowTime() - time_now);
			data_len = 0;
			recv_bytes = 0;
		} else {
			usleep(20000);
		}

	}

EXIT:
	UdpRecv_PRINT("udp_stream_recv_deal  pthread is exit");
	return NULL;
}

udp_recv_handle *udp_recv_init(stream_recv_cond_t *src)
{
	udp_recv_handle *p_udp_recv_hand = NULL;

	if(NULL == src) {
		udp_recv_err_print("src = [%p]", src);
	}

	p_udp_recv_hand = (udp_recv_handle *)r_malloc(sizeof(udp_recv_handle));

	if(NULL == p_udp_recv_hand) {
		udp_recv_err_print("malloc error");
		return NULL;
	}

	memset(p_udp_recv_hand, 0, sizeof(udp_recv_handle));

	if(src->func != NULL) {
		p_udp_recv_hand->func = src->func;
	} else {
		udp_recv_warn_print("udp_recv_func is null\n");
	}

	p_udp_recv_hand->pthread_status = UdpRecv_THREAD_RUN;
	p_udp_recv_hand->stream_info.port = src->port;
	snprintf(p_udp_recv_hand->stream_info.ip, UdpRecv_IP_LEN, "%s", src->ip);
	p_udp_recv_hand->stream_info.user_data = src->user_data;

	pthread_attr_t attr;
	struct sched_param param;
	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	param.sched_priority = UdpRecv_THREAD_PRIORITY;
	pthread_attr_setschedparam(&attr, &param);

	if(pthread_create(&(p_udp_recv_hand->upd_pt), &attr, udp_recv_deal, (void *)p_udp_recv_hand)) {
		udp_recv_err_print("pthread_create error");
		return NULL;
	}

	pthread_attr_destroy(&attr);
	return p_udp_recv_hand;
}

// *****************************************************
//function	:  依据RTP 扩展头信息获取JEPG帧头信息
//author	:  zhengyb		2012.12.7
//******************************************************
static int32_t get_jpeg_frame_data_head(udp_recv_freame_head_t *frame_ptr, UdpRecv_FRAMEHEAD *rtp_frame_ptr, UdpRecv_RTP_FIXED_HEADER *rtp_head_prt)
{
	frame_ptr->m_frame_rate = rtp_frame_ptr->framerate;

	frame_ptr->m_colors = 0;
	frame_ptr->m_others = 0;
	frame_ptr->m_time_tick 		= ntohl(rtp_head_prt->timestamp);
	frame_ptr->m_frame_length 		= ntohl(rtp_frame_ptr->framelength);
	frame_ptr->m_width				= ntohs(rtp_frame_ptr->width);
	frame_ptr->m_hight				= ntohs(rtp_frame_ptr->height);
	frame_ptr->m_dw_packet_number  = ntohs(rtp_head_prt->seq_no);
	frame_ptr->m_dw_segment 		= rtp_frame_ptr->samplerate;

	return 0;
}

// *****************************************************
//function	:  获取JPEG 数据
//author	:  zhengyb		2012.12.7
//******************************************************
static int32_t get_jpeg_frame_data(int8_t *recv_buf, int32_t recv_len, int8_t *frame_buf, int32_t *frame_len)
{
	*frame_len = recv_len - 28;
	memcpy(frame_buf, (recv_buf + 28), recv_len - 28);

	return 0;
}

static int32_t get_audio_frame_data_head(udp_recv_freame_head_t *frame_ptr, UdpRecv_FRAMEHEAD *rtp_frame_ptr, UdpRecv_RTP_FIXED_HEADER *rtp_head_prt)
{
	frame_ptr->m_frame_rate = ntohs(rtp_frame_ptr->height);
	frame_ptr->m_dw_segment = 3;									// 先默认为 1400 的独立报后期再更改支持多包情况

	frame_ptr->m_colors = 0;
	frame_ptr->m_others = 0;
	frame_ptr->m_time_tick 		= ntohl(rtp_head_prt->timestamp);
	//frame_ptr->m_frame_length 		= ntohl(rtp_frame_ptr->framelength) - 7;// 为啥要减7 ?.
	frame_ptr->m_frame_length 		= ntohl(rtp_frame_ptr->framelength);// 为啥要减7 ?.
	frame_ptr->m_width				= ntohs(rtp_frame_ptr->width);
	frame_ptr->m_hight				= ntohs(rtp_frame_ptr->height);
	frame_ptr->m_dw_packet_number  = ntohs(rtp_head_prt->seq_no);
	frame_ptr->m_frame_rate		= switch_audio_sample_rate(rtp_frame_ptr->samplerate);

	frame_ptr->m_dw_flags			= rtp_frame_ptr->Iframe ;

	if(frame_ptr->m_dw_flags != 0) {
		frame_ptr->m_dw_flags = 16;
	}

	if(rtp_frame_ptr->reserve == 1) {
		frame_ptr->m_frame_rate = frame_ptr->m_frame_rate * 2;
		nslog(NS_ERROR, "need to *2 !----- frame_ptr->m_frame_rate : %d\n", frame_ptr->m_frame_rate);
	}

	return 0;
}

// *****************************************************
//function	:  依据RTP 扩展头信息获取H264帧头信息
//author	:  zhengyb		2012.12.7
//******************************************************
static int32_t get_video_frame_data_head(udp_recv_freame_head_t *frame_ptr, UdpRecv_FRAMEHEAD *rtp_frame_ptr, UdpRecv_RTP_FIXED_HEADER *rtp_head_prt)
{
	// I 帧标示位  1. 可以通过数据中获取 2.可以通过保留位获取
	// 采取第一种方法 保留位用于 传音频的码流

	frame_ptr->m_frame_rate = rtp_frame_ptr->framerate;
	frame_ptr->m_colors = 0;
	frame_ptr->m_time_tick 		= ntohl(rtp_head_prt->timestamp);
	//nslog(NS_INFO,"COMING_TIME : %u ----- TRAN_TIME : %u\n",rtp_head_prt->timestamp,frame_ptr->m_time_tick);
	frame_ptr->m_frame_length 		= ntohl(rtp_frame_ptr->framelength);
	frame_ptr->m_width				= ntohs(rtp_frame_ptr->width);
	frame_ptr->m_hight				= ntohs(rtp_frame_ptr->height);
	frame_ptr->m_dw_packet_number  = ntohs(rtp_head_prt->seq_no);

	frame_ptr->m_dw_flags			= rtp_frame_ptr->Iframe ;

	if(UdpRecv_BLUE_FLAG == rtp_frame_ptr->samplerate) {
		frame_ptr->m_others = 1;
	} else {
		frame_ptr->m_others = 0;
	}

	if(frame_ptr->m_dw_flags != 0) {
		frame_ptr->m_dw_flags = 16;
	}
}

// *****************************************************
//function	:  获取H264 数据
//author	:  zhengyb		2012.12.7
//******************************************************
static int32_t get_video_frame_data(int8_t *recv_buf, int32_t recv_len, int8_t *frame_buf, int32_t *frame_len)
{
	UdpRecv_FU_INDICATOR	             *fu_ind;
	UdpRecv_FU_HEADER		             *fu_hdr;
	UdpRecv_RTP_FIXED_HEADER       	 	 *rtp_hdr	= (UdpRecv_RTP_FIXED_HEADER *)recv_buf;
	UdpRecv_FRAMEHEAD 			*rtp_frame_ptr	  	= (UdpRecv_FRAMEHEAD *)(recv_buf + 16);

	int ret = 0;

	int32_t	frame_flag						= 0;

	fu_ind = (UdpRecv_FU_INDICATOR *) & (recv_buf[28]);
	fu_hdr = (UdpRecv_FU_HEADER *) & (recv_buf[29]);

	//分片
	if(fu_ind->TYPE ==  28) {
		//起始  新帧
		if(fu_hdr->S == 0x1) {
			if(rtp_frame_ptr->Iframe == 1) {
				frame_flag = 0;
			} else {
				frame_flag = 2;
			}

			//表示是新帧
			frame_buf[(*frame_len)++] = 0;
			frame_buf[(*frame_len)++] = 0;
			frame_buf[(*frame_len)++] = 0;
			frame_buf[(*frame_len)++] = 1;
			frame_buf[(*frame_len)++] = ((fu_ind->F) << 7) | ((fu_ind->NRI) << 5) | fu_hdr->TYPE;
			memcpy((frame_buf + (*frame_len)), (recv_buf + 30), recv_len - 30);
			((*frame_len)) += recv_len - 30;
		} else {
			//  if( rtp_hdr->timestamp  ==  rtp_prev_hdr->timestamp ){

			memcpy(frame_buf + ((*frame_len)), (recv_buf + 30), recv_len - 30);

			(*frame_len) += recv_len - 30;

			if(fu_hdr->E && 0x1) {
				frame_flag = 1;
			} else {
				frame_flag = 0;
			}
		}
	} else {
		//			return -2;//非RTP包
		frame_buf[(*frame_len)++] = 0;
		frame_buf[(*frame_len)++] = 0;
		frame_buf[(*frame_len)++] = 0;
		frame_buf[(*frame_len)++] = 1;

		memcpy((frame_buf + (*frame_len)), (recv_buf + 28), recv_len - 28);
		(*frame_len) += recv_len - 28;


		//	printf("rtp_frame_ptr->Iframe : %d\n",rtp_frame_ptr->Iframe);
		if(rtp_frame_ptr->Iframe == 1) {
			if(frame_buf[4] == 0x67 || frame_buf[4] == 0x47 || frame_buf[4] == 0x27) {
				frame_flag = 2;
			} else if(frame_buf[4] == 0x68 || frame_buf[4] == 0x48 || frame_buf[4] == 0x28) {
				frame_flag = 0;
			} else if(frame_buf[4] == 0x65 || frame_buf[4] == 0x45 || frame_buf[4] == 0x25) {
				frame_flag = 1;
			} else {
				udp_recv_err_print("-----------[%x]-----------------NOT BULE DATA!\n", frame_buf[4]);
				//	printf("-----------[%x]-----------------NOT BULE DATA!\n",frame_buf[4]);
				frame_flag = -1;
			}
		} else {
			frame_flag = 3;
		}
	}

	return frame_flag;
}

//  音频处理时先当一个包1400 可以发全一帧!!!
// (后期再处理一包发不全的长音频帧)
// 注意: 接受到的RTP 音频帧数据时没有ADTS 头的
// 暂不还原为有ADTS
// *****************************************************
//function	:  获取AUDIO 数据
//author	:  zhengyb		2012.12.7
//******************************************************

static int OnAudioAacFrame(int samplerate, int frame_length, unsigned char *adts_header)
{
	int channels = 2;
	unsigned int obj_type = 1;
	unsigned int num_data_block = frame_length / 1024;

	// include the header length also
	//	frame_length += 7;
	/* We want the same metadata */
	/* Generate ADTS header */
	if(adts_header == NULL) {
		return 0;
	}

	/* Sync point over a full byte */
	adts_header[0] = 0xFF;
	/* Sync point continued over first 4 bits + static 4 bits
	 * (ID, layer, protection)*/
	adts_header[1] = 0xF1;
	/* Object type over first 2 bits */
	adts_header[2] = obj_type << 6;
	/* rate index over next 4 bits */
	adts_header[2] |= (samplerate << 2);
	/* channels over last 2 bits */
	adts_header[2] |= (channels & 0x4) >> 2;
	/* channels continued over next 2 bits + 4 bits at zero */
	adts_header[3] = (channels & 0x3) << 6;
	/* frame size over last 2 bits */
	adts_header[3] |= (frame_length & 0x1800) >> 11;
	/* frame size continued over full byte */
	adts_header[4] = (frame_length & 0x1FF8) >> 3;
	/* frame size continued first 3 bits */
	adts_header[5] = (frame_length & 0x7) << 5;
	/* buffer fullness (0x7FF for VBR) over 5 last bits*/
	adts_header[5] |= 0x01;
	/* buffer fullness (0x7FF for VBR) continued over 6 first bits + 2 zeros
	 * number of raw data blocks */
	adts_header[6] = 0x8c;// one raw data blocks .
	adts_header[6] |= num_data_block & 0x03; //Set raw Data blocks.


	return 0;
}

#if 0
static int OnAudioAacFrame(int samplerate, int frame_length, unsigned char *adts_header)
{
	unsigned int framelen = frame_length; //aac数据的长度
	unsigned char channels = 2;//双通道立体声
	unsigned char obj_type = 1;//一般是1
	unsigned int num_data_block = (framelen - 7) / 1024; //该值一般为0，很少有1的
	// unsigned int samplerate=6;//6为48khz，7为44khz
	adts_header[0] = 0xFF; //必须的
	adts_header[1] = 0xF1; //必须的
	adts_header[2] = (obj_type) << 6; //mp4标准
	adts_header[2] |= samplerate << 2; //采样率
	adts_header[2] &= 0xFD; //双通道
	adts_header[2] |= (channels) >> 2; //双通道
	adts_header[3] = (channels) << 6; //双通道
	adts_header[3] |= (channels) << 6; //双通道
	adts_header[3] &= 0xC3;
	adts_header[3] |= (framelen & 0x1800) >> 11; //帧长度
	adts_header[4] = ((framelen) & 0x7F8) >> 3; //帧长度
	adts_header[5] = ((framelen) & 0x7) << 5; //帧长度
	adts_header[5] |= 0x01;
	adts_header[6] = 0x8C;
	adts_header[6] |= num_data_block & 0x00;

}
#endif

static int aac_payload_demux(int sample_rate, unsigned char *buf, int len, int status, unsigned char *frame_buf, unsigned int *frame_len)
{
	unsigned char header[32] = {0};

	if((*frame_len) == 0) {
		(*frame_len) += 7;
	}

	memcpy(frame_buf + (*frame_len) , buf + 4 , len - 4);
	(*frame_len) += (len - 4);

	if(status == 1) {
		OnAudioAacFrame(sample_rate, (*frame_len), (unsigned char *)header);
		memcpy(frame_buf, header, 7);
		return 1;
	}

	return 0;
}
static int32_t get_audio_frame_data(int8_t *recv_buf, int32_t recv_len, int8_t *frame_buf, int32_t *frame_len)
{
	UdpRecv_FRAMEHEAD *rtp_frame_ptr 				= NULL;
	UdpRecv_RTP_FIXED_HEADER *rtp_head_prt 	= NULL;
	rtp_head_prt 	= (UdpRecv_RTP_FIXED_HEADER *)recv_buf;
	rtp_frame_ptr 	= (UdpRecv_FRAMEHEAD *)(recv_buf + 16);
	int32_t config = 0;
	int32_t samplate = 0;
	int32_t ret = 0;

	//当前假定所有音频帧为i帧。
	if(2 == rtp_frame_ptr->samplerate) {
		samplate = 44100;
		config = 1390;
	} else if(3 == rtp_frame_ptr->samplerate) {
		config = 1310;
		samplate = 48000;
	}

	//ret = aac_payload_demux(recv_buf + 28, recv_len - 28, 1, config,  6, frame_buf, (unsigned int *)frame_len);
	ret = aac_payload_demux(4, recv_buf + 28, recv_len - 28, 1, frame_buf, frame_len);

	// nslog(NS_WARN, "ret = %d,recv_len = %d,frame_len = %d", ret ,recv_len,  *frame_len);
	return 0;

}


static void udp_recv_set_stream_info(udp_recv_stream_info_t *p_stream_info, udp_recv_freame_head_t *frame_ptr, uint32_t timestamp)
{
	p_stream_info->type = frame_ptr->m_data_codec;
	p_stream_info->height = frame_ptr->m_hight;
	p_stream_info->width = frame_ptr->m_width;
	p_stream_info->IDR_FLAG = frame_ptr->m_dw_flags;
	p_stream_info->is_blue = !(frame_ptr->m_others);
	p_stream_info->m_colors = frame_ptr->m_colors;
	p_stream_info->timestamp = timestamp;
	p_stream_info->frame_rate = frame_ptr->m_frame_rate;

	if(UdpRecv_AAC_CODEC_TYPE == frame_ptr->m_data_codec) {
		p_stream_info->samplerate = frame_ptr->m_frame_rate;
	}
}

#if 0  //test write file
int32_t write_long_tcp_data(FILE *fd, void *buf, int32_t buflen)
{
	int32_t total_udp_recv = 0;
	int32_t udp_recv_len = 0;

	while(total_udp_recv < buflen) {
		udp_recv_len = fwrite(buf, buflen, 1, fd);

		if(udp_recv_len < 0) {
			udp_recv_err_print("write  error udp_recv_len = %d, errno = %d", udp_recv_len, errno);
		}

		total_udp_recv = udp_recv_len * buflen;

		if(total_udp_recv = buflen) {
			break;
		}
	}

	return total_udp_recv;
}

#endif

int udp_recv_start(udp_recv_handle *p_udp_recv_hand)
{
	if(NULL == p_udp_recv_hand) {
		udp_recv_err_print("p_udp_hand = [%p]", p_udp_recv_hand);
		return UdpRecv_OPERATION_ERR;
	}

	p_udp_recv_hand->stream_status = STREAM_START;
}

int udp_recv_pause(udp_recv_handle *p_udp_recv_hand)
{
	if(NULL == p_udp_recv_hand) {
		udp_recv_err_print("p_udp_hand = [%p]", p_udp_recv_hand);
		return UdpRecv_OPERATION_ERR;
	}

	p_udp_recv_hand->stream_status = STREAM_PAUSE;
}

uint32_t udp_recv_getCudp_recvrentTime(void)
{
	struct timeval tv;
	struct timezone tz;
	uint32_t ultime;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return (ultime);
}

uint32_t udp_recv_getIntervalTime(uint32_t prev_time, uint32_t cudp_recv_time)
{
	uint32_t cal_time = 0;

	if(prev_time > cudp_recv_time) {
		cal_time = prev_time - cudp_recv_time;
	} else {
		cal_time = cudp_recv_time - prev_time;
	}

	if(0 == cal_time) {
		cal_time = cal_time + 1;
	}

	return cal_time;
}

uint32_t udp_recv_getJpgIntervalTime(uint32_t prev_time, uint32_t cudp_recv_time)
{
	uint32_t cal_time = 0;

	if(prev_time > cudp_recv_time) {
		cal_time = prev_time - cudp_recv_time;
	} else {
		cal_time = cudp_recv_time - prev_time;
	}

	if(0 == cal_time) {
		cal_time = cal_time + 1;
	}

	return cal_time;
}

void udp_recv_deinit(udp_recv_handle *p_udp_recv_hand)
{
	if(NULL == p_udp_recv_hand) {
		udp_recv_err_print("p_udp_recv_hand = [%p]", p_udp_recv_hand);
		return;
	}

	if(STREAM_START ==  p_udp_recv_hand->stream_status) {
		udp_recv_pause(p_udp_recv_hand);
	}

	p_udp_recv_hand->pthread_status = UdpRecv_THREAD_EXIT;
	pthread_join(p_udp_recv_hand->upd_pt, NULL);

	if(p_udp_recv_hand->stream_info.udp_fd > 0) {
		udp_recv_close_sock(p_udp_recv_hand->stream_info.udp_fd);
		p_udp_recv_hand->stream_info.udp_fd = -1;
	}

	if(p_udp_recv_hand->video_data) {
		r_free(p_udp_recv_hand->video_data);
		p_udp_recv_hand->video_data = NULL;
	}

	if(p_udp_recv_hand->audio_data) {
		r_free(p_udp_recv_hand->audio_data);
		p_udp_recv_hand->audio_data = NULL;
	}

	p_udp_recv_hand->func = NULL;

	if(p_udp_recv_hand) {
		r_free(p_udp_recv_hand);
		p_udp_recv_hand = NULL;
	}

}

/*==============================================================================
  函数: <pkg_placed_in_frame>
  功能: <组帧>
  参数: int8_t *frame_data,  int32_t part_frame_len,  stream_handle *M_handle
  返回值: 成功返回1，失败返回0.
  Created By liuchsh 2012.11.16 15:40:28 For EDU
  ==============================================================================*/
static int32_t pkg_placed_in_frame(int8_t *frame_data,  int32_t part_frame_len,  udp_recv_handle *p_udp_recv_hand)
{
	udp_recv_freame_head_t *fh = (udp_recv_freame_head_t *)frame_data;
	int32_t return_code = UdpRecv_OPERATION_SUCC;
	uint32_t cudp_recvrent_time = 0;
	uint32_t video_cal_time = 0, video_cal_sys_time = 0;


	if(NULL == p_udp_recv_hand || NULL == frame_data) {
		udp_recv_err_print("M_handle = %p, frame_data = %p", p_udp_recv_hand, frame_data);
		return UdpRecv_OPERATION_ERR;
	}

#if 1
	cudp_recvrent_time = udp_recv_getCudp_recvrentTime();

	if(0 >= p_udp_recv_hand->udp_recv_time_t.init_video_flag) {
		p_udp_recv_hand->udp_recv_time_t.video_init_time = fh->m_time_tick;
		p_udp_recv_hand->udp_recv_time_t.init_video_flag = 1;
		p_udp_recv_hand->udp_recv_time_t.video_cal_time = 0;
		p_udp_recv_hand->udp_recv_time_t.prev_video_time_tick = fh->m_time_tick;
		// nslog(NS_INFO, "init video_flag  id=%d", M_handle->stream_id);
	}

	if(fh->m_time_tick > p_udp_recv_hand->udp_recv_time_t.video_init_time) {
		video_cal_time = fh->m_time_tick - p_udp_recv_hand->udp_recv_time_t.video_init_time;
	} else {
		video_cal_time = p_udp_recv_hand->udp_recv_time_t.video_init_time - fh->m_time_tick;
	}

	if(UdpRecv_H264_CODEC_TYPE == fh->m_data_codec) {
		if(fh->m_time_tick < p_udp_recv_hand->udp_recv_time_t.prev_video_time_tick ||
		   (fh->m_time_tick > p_udp_recv_hand->udp_recv_time_t.prev_video_time_tick &&
		    (fh->m_time_tick - p_udp_recv_hand->udp_recv_time_t.prev_video_time_tick) > UdpRecv_EXAMIN_TIME)) {
			p_udp_recv_hand->udp_recv_time_t.time_video_tick = p_udp_recv_hand->udp_recv_time_t.time_video_tick + udp_recv_getIntervalTime(p_udp_recv_hand->udp_recv_time_t.prev_video_time, cudp_recvrent_time);
			p_udp_recv_hand->udp_recv_time_t.video_init_time = fh->m_time_tick;

			p_udp_recv_hand->udp_recv_time_t.video_cal_time = p_udp_recv_hand->udp_recv_time_t.time_video_tick;

			udp_recv_err_print("video:time_cal = %u, cudp_recvrent_time = %u,vo_init_time = %u, tm_vo_tick = %u, vo_cal_time=%u, gap_time = %uid = %d",
			                   video_cal_time,
			                   cudp_recvrent_time,
			                   p_udp_recv_hand->udp_recv_time_t.video_init_time,
			                   p_udp_recv_hand->udp_recv_time_t.time_video_tick,
			                   p_udp_recv_hand->udp_recv_time_t.video_cal_time,
			                   p_udp_recv_hand->udp_recv_time_t.prev_video_time_tick - cudp_recvrent_time,
			                   p_udp_recv_hand->stream_num);
		} else {
			p_udp_recv_hand->udp_recv_time_t.time_video_tick = video_cal_time + p_udp_recv_hand->udp_recv_time_t.video_cal_time;
		}
	}


	if(UdpRecv_JPEG_CODEC_TYPE == fh->m_data_codec) {
		if(fh->m_time_tick < p_udp_recv_hand->udp_recv_time_t.prev_video_time_tick) {
			p_udp_recv_hand->udp_recv_time_t.time_video_tick = p_udp_recv_hand->udp_recv_time_t.time_video_tick + udp_recv_getJpgIntervalTime(p_udp_recv_hand->udp_recv_time_t.prev_video_time, cudp_recvrent_time);

			p_udp_recv_hand->udp_recv_time_t.video_init_time = fh->m_time_tick;

			p_udp_recv_hand->udp_recv_time_t.video_cal_time = p_udp_recv_hand->udp_recv_time_t.time_video_tick;

			udp_recv_err_print("JPG:time_cal = %u, prev_video_time_cal = %u, time_video_tick = %u, video_cal_time=%u, id = %d",
			                   video_cal_time,
			                   p_udp_recv_hand->udp_recv_time_t.prev_video_time_tick,
			                   p_udp_recv_hand->udp_recv_time_t.time_video_tick,
			                   p_udp_recv_hand->udp_recv_time_t.video_cal_time,
			                   p_udp_recv_hand->stream_num);
		} else {
			p_udp_recv_hand->udp_recv_time_t.time_video_tick = video_cal_time + p_udp_recv_hand->udp_recv_time_t.video_cal_time;
		}
	}

	p_udp_recv_hand->udp_recv_time_t.prev_video_time_tick = fh->m_time_tick;
	p_udp_recv_hand->udp_recv_time_t.prev_video_time = cudp_recvrent_time;
#endif

	switch(fh->m_dw_segment) {
		case UdpRecv_START_FRAME:

			if(fh->m_frame_length > UdpRecv_MAX_FRAME_LEN) {
				udp_recv_err_print("fh->m_frame_length : [%d]", fh->m_frame_length);
				return_code = UdpRecv_OPERATION_ERR;
				break;
			}

			p_udp_recv_hand->video_offset = 0;
			memcpy(p_udp_recv_hand->video_data, frame_data + UdpRecv_FH_LEN, part_frame_len);
			p_udp_recv_hand->video_offset += part_frame_len;;
			return_code = UdpRecv_OPERATION_SUCC;
			break;

		case UdpRecv_MIDDLE_FRAME:      //0表示中间包

			if(p_udp_recv_hand->video_offset + part_frame_len > fh->m_frame_length) {
				p_udp_recv_hand->video_offset  = 0;
				return_code = UdpRecv_OPERATION_SUCC;
				p_udp_recv_hand->stream_info.video_lost_num += 1;
				udp_recv_err_print("(M_handle->offset)+ part_frame_len = %d fh->m_frame_length = %d, id = %d", (p_udp_recv_hand->video_offset) + part_frame_len, fh->m_frame_length, p_udp_recv_hand->stream_num);
				break;
			}

			memcpy(p_udp_recv_hand->video_data + p_udp_recv_hand->video_offset, frame_data + UdpRecv_FH_LEN, part_frame_len);
			p_udp_recv_hand->video_offset += part_frame_len;
			return_code = UdpRecv_OPERATION_SUCC;
			break;

		case UdpRecv_LAST_FRAME:            //表示结尾包

			if(p_udp_recv_hand->video_offset + part_frame_len > fh->m_frame_length) {
				p_udp_recv_hand->video_offset  = 0;
				return_code = UdpRecv_OPERATION_SUCC;
				p_udp_recv_hand->stream_info.video_lost_num += 1;
				udp_recv_err_print("(M_handle->offset)+ part_frame_len = %d fh->m_frame_length = %d, id = %d", (p_udp_recv_hand->video_offset) + part_frame_len, fh->m_frame_length, p_udp_recv_hand->stream_num);
				break;
			}

			memcpy(p_udp_recv_hand->video_data + p_udp_recv_hand->video_offset, frame_data + UdpRecv_FH_LEN, part_frame_len);
			p_udp_recv_hand->video_offset = 0;
			return_code = UdpRecv_OPERATION_CONTINUE;
			break;

		case  UdpRecv_INDEPENDENT_FRAME:
			memcpy(p_udp_recv_hand->video_data, frame_data + UdpRecv_FH_LEN, fh->m_frame_length);
			return_code = UdpRecv_OPERATION_CONTINUE;
			break;

		default:
			udp_recv_err_print("fh->m_dw_segment : [%d]", fh->m_dw_segment);
			return_code = UdpRecv_OPERATION_SUCC;
			break;
	}

	return return_code;
}


static int32_t pkg_placed_in_frame_audio(int8_t *frame_data,  int32_t part_frame_len,  udp_recv_handle *p_udp_recv_hand)
{
	udp_recv_freame_head_t *fh = (udp_recv_freame_head_t *)frame_data;
	int32_t return_code = UdpRecv_OPERATION_SUCC;
	uint32_t cudp_recvrent_time = 0;
	uint32_t audio_cal_time = 0, audio_cal_sys_time = 0;

	if(NULL == p_udp_recv_hand || NULL == frame_data) {
		udp_recv_err_print("M_handle = %p, frame_data = %p", p_udp_recv_hand, frame_data);
		return UdpRecv_OPERATION_ERR;
	}

#if 1
	cudp_recvrent_time = udp_recv_getCudp_recvrentTime();

	if(0 >= p_udp_recv_hand->udp_recv_time_t.init_audio_flag) {
		p_udp_recv_hand->udp_recv_time_t.audio_init_time = fh->m_time_tick;
		p_udp_recv_hand->udp_recv_time_t.init_audio_flag = 1;
		p_udp_recv_hand->udp_recv_time_t.audio_cal_time = 0;
		p_udp_recv_hand->udp_recv_time_t.prev_audio_time_tick = fh->m_time_tick;
		//nslog(NS_INFO, "init audio_flag  id=%d", M_handle->stream_id);
	}

	//	nslog(NS_ERROR, "fh->m_time_tick = %u, M_handle->udp_recv_time_t.audio_init_time=%u, id=%d", fh->m_time_tick, M_handle->udp_recv_time_t.audio_init_time, M_handle->stream_id);

	if(fh->m_time_tick > p_udp_recv_hand->udp_recv_time_t.audio_init_time) {
		audio_cal_time = fh->m_time_tick - p_udp_recv_hand->udp_recv_time_t.audio_init_time;
	} else {
		audio_cal_time = p_udp_recv_hand->udp_recv_time_t.audio_init_time - fh->m_time_tick;
	}

	if(fh->m_time_tick < p_udp_recv_hand->udp_recv_time_t.prev_audio_time_tick ||
	   (fh->m_time_tick > p_udp_recv_hand->udp_recv_time_t.prev_audio_time_tick &&
	    (fh->m_time_tick - p_udp_recv_hand->udp_recv_time_t.prev_audio_time_tick) > UdpRecv_EXAMIN_TIME)) {

		p_udp_recv_hand->udp_recv_time_t.time_audio_tick = p_udp_recv_hand->udp_recv_time_t.time_audio_tick + udp_recv_getIntervalTime(p_udp_recv_hand->udp_recv_time_t.prev_audio_time, cudp_recvrent_time);

		p_udp_recv_hand->udp_recv_time_t.audio_init_time = fh->m_time_tick;

		p_udp_recv_hand->udp_recv_time_t.audio_cal_time = p_udp_recv_hand->udp_recv_time_t.time_audio_tick;

		udp_recv_err_print("audio:time_cal = %u, prev_audio_time_cal = %u, time_audio_tick = %u, audio_cal_time=%u, id = %d",
		                   audio_cal_time,
		                   p_udp_recv_hand->udp_recv_time_t.prev_audio_time_tick,
		                   p_udp_recv_hand->udp_recv_time_t.time_audio_tick,
		                   p_udp_recv_hand->udp_recv_time_t.audio_cal_time,
		                   p_udp_recv_hand->stream_num);
	} else {
		p_udp_recv_hand->udp_recv_time_t.time_audio_tick = audio_cal_time + p_udp_recv_hand->udp_recv_time_t.audio_cal_time;
	}


	p_udp_recv_hand->udp_recv_time_t.prev_audio_time_tick = fh->m_time_tick;
	p_udp_recv_hand->udp_recv_time_t.prev_audio_time = cudp_recvrent_time;

#endif


	switch(fh->m_dw_segment) {
		case UdpRecv_START_FRAME:
			if(fh->m_frame_length > UdpRecv_MAX_AUDIO_LEN) {
				udp_recv_err_print("fh->m_frame_length : [%d]", fh->m_frame_length);
				return_code = UdpRecv_OPERATION_ERR;
				break;
			}

			//already delete old think
			p_udp_recv_hand->audio_offset = 0;
			memcpy(p_udp_recv_hand->audio_data, frame_data + UdpRecv_FH_LEN, part_frame_len);
			p_udp_recv_hand->audio_offset += part_frame_len;;
			return_code = UdpRecv_OPERATION_SUCC;
			break;

		case UdpRecv_MIDDLE_FRAME:      //0表示中间包
			if(p_udp_recv_hand->audio_offset + part_frame_len > fh->m_frame_length) {
				p_udp_recv_hand->audio_offset  = 0;
				return_code = UdpRecv_OPERATION_SUCC;
				udp_recv_err_print("(M_handle->offset)+ part_frame_len = %d fh->m_frame_length = %d, id = %d", (p_udp_recv_hand->audio_offset) + part_frame_len, fh->m_frame_length, p_udp_recv_hand->stream_num);
				break;
			}

			memcpy(p_udp_recv_hand->audio_data + p_udp_recv_hand->audio_offset, frame_data + UdpRecv_FH_LEN, part_frame_len);
			p_udp_recv_hand->audio_offset += part_frame_len;
			return_code = UdpRecv_OPERATION_SUCC;
			break;

		case UdpRecv_LAST_FRAME:            //表示结尾包
			if(p_udp_recv_hand->audio_offset + part_frame_len > fh->m_frame_length) {
				p_udp_recv_hand->audio_offset  = 0;
				return_code = UdpRecv_OPERATION_SUCC;
				udp_recv_err_print("(M_handle->offset)+ part_frame_len = %d fh->m_frame_length = %d, id = %d", (p_udp_recv_hand->audio_offset) + part_frame_len, fh->m_frame_length, p_udp_recv_hand->stream_num);
				break;
			}

			memcpy(p_udp_recv_hand->audio_data + p_udp_recv_hand->audio_offset, frame_data + UdpRecv_FH_LEN, part_frame_len);
			p_udp_recv_hand->audio_offset = 0;
			return_code = UdpRecv_OPERATION_CONTINUE;
			break;

		case  UdpRecv_INDEPENDENT_FRAME:
			memcpy(p_udp_recv_hand->audio_data, frame_data + UdpRecv_FH_LEN, fh->m_frame_length);
			return_code = UdpRecv_OPERATION_CONTINUE;
			break;

		default:
			udp_recv_err_print("fh->m_dw_segment : [%d]", fh->m_dw_segment);
			return_code = UdpRecv_OPERATION_SUCC;
			break;
	}

	return return_code;
}

static int32_t udp_recv_media_frame_data(udp_recv_handle *p_udp_recv_hand, void *data, int32_t data_len)
{
	udp_recv_freame_head_t *fh = (udp_recv_freame_head_t *)(data + Ur_MsgLen);
	int32_t frame_data_len = data_len - UdpRecv_FH_LEN - Ur_MsgLen;
	udp_recv_msg_header_t  m_live_msgque;
	int return_code = UdpRecv_OPERATION_SUCC;
	//nslog(NS_INFO, "packet_number = %u, frame_length = %u, dw_segment =%u, part = %u, time_tk = %u",
	//fh->m_dw_packet_number, fh->m_frame_length,  fh->m_dw_segment, frame_data_len, fh->m_time_tick);
	// assert(M_handle && data);

	if(NULL == p_udp_recv_hand || NULL == data) {
		udp_recv_err_print("M_handle = %p, data = %p", p_udp_recv_hand, data);
		return UdpRecv_OPERATION_ERR;
	}

	if(UdpRecv_H264_CODEC_TYPE == fh->m_data_codec || UdpRecv_JPEG_CODEC_TYPE == fh->m_data_codec) {
		if((return_code = pkg_placed_in_frame((int8_t *)(data + Ur_MsgLen), frame_data_len, p_udp_recv_hand)) < UdpRecv_OPERATION_CONTINUE) {
			return UdpRecv_OPERATION_SUCC;
		} else if(UdpRecv_OPERATION_CONTINUE == return_code) {
			return UdpRecv_FRAME_OK;
		}
	}

	if(UdpRecv_AAC_CODEC_TYPE == fh->m_data_codec) {
		if((return_code = pkg_placed_in_frame_audio((int8_t *)(data + Ur_MsgLen), frame_data_len, p_udp_recv_hand)) < UdpRecv_OPERATION_CONTINUE) {
			return UdpRecv_OPERATION_SUCC;
		} else if(UdpRecv_OPERATION_CONTINUE == return_code) {
			return UdpRecv_FRAME_OK;
		}
	}

	return return_code;
}

