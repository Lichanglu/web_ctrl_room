#include "media_frame.h"
#include "stdint.h"
#include "media_msg.h"
#include "receive_module.h"
// add zl
#include "reach_udp_snd.h"
#include "reach_udp_recv.h"


uint32_t getCurrentTime(void)
{
	struct timeval tv;
	struct timezone tz;
	uint32_t ultime;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return (ultime);
}

uint32_t getIntervalTime(uint32_t prev_time, uint32_t cur_time)
{
	uint32_t cal_time = 0;

	if(prev_time > cur_time) {
		cal_time = prev_time - cur_time;
	} else {
		cal_time = cur_time - prev_time;
	}

	if(0 == cal_time) {
		cal_time = cal_time + 1;
	}

	return cal_time;
}

uint32_t getJpgIntervalTime(uint32_t prev_time, uint32_t cur_time)
{
	uint32_t cal_time = 0;

	if(prev_time > cur_time) {
		cal_time = prev_time - cur_time;
	} else {
		cal_time = cur_time - prev_time;
	}

	if(0 == cal_time) {
		cal_time = cal_time + 1;
	}

	return cal_time;
}


/*==============================================================================
  函数: <pkg_placed_in_frame>
  功能: <组帧>
  参数: int8_t *frame_data,  int32_t part_frame_len,  stream_handle *M_handle
  返回值: 成功返回1，失败返回0.
  Created By liuchsh 2012.11.16 15:40:28 For EDU
  ==============================================================================*/
int32_t pkg_placed_in_frame(int8_t *frame_data,  int32_t part_frame_len,  stream_handle *M_handle)
{
	hdb_freame_head_t *fh = (hdb_freame_head_t *)frame_data;
	int32_t return_code = OPERATION_SUCC;
	uint32_t current_time = 0;
	uint32_t video_cal_time = 0, video_cal_sys_time = 0;


	if(NULL == M_handle || NULL == frame_data) {
		nslog(NS_ERROR, "M_handle = %p, frame_data = %p", M_handle, frame_data);
		return OPERATION_ERR;
	}

#if 1
	current_time = getCurrentTime();

	if(0 >= M_handle->recv_time_t.init_video_flag) {
		M_handle->recv_time_t.video_init_time = fh->m_time_tick;
		M_handle->recv_time_t.init_video_flag = 1;
		M_handle->recv_time_t.video_cal_time = 0;
		M_handle->recv_time_t.prev_video_time_tick = fh->m_time_tick;
		nslog(NS_INFO, "init video_flag  id=%d", M_handle->stream_id);
	}

	if(fh->m_time_tick > M_handle->recv_time_t.video_init_time) {
		video_cal_time = fh->m_time_tick - M_handle->recv_time_t.video_init_time;
	} else {
		video_cal_time = M_handle->recv_time_t.video_init_time - fh->m_time_tick;
	}

	if(H264_CODEC_TYPE == fh->m_data_codec) {
		if(fh->m_time_tick < M_handle->recv_time_t.prev_video_time_tick ||
		   (fh->m_time_tick > M_handle->recv_time_t.prev_video_time_tick &&
		    (fh->m_time_tick - M_handle->recv_time_t.prev_video_time_tick) > EXAMIN_TIME) &&
		   M_handle->av_connect_flag == 0) {
			M_handle->recv_time_t.time_video_tick = M_handle->recv_time_t.time_video_tick + getIntervalTime(M_handle->recv_time_t.prev_video_time, current_time);
			M_handle->recv_time_t.video_init_time = fh->m_time_tick;

			M_handle->recv_time_t.video_cal_time = M_handle->recv_time_t.time_video_tick;

			nslog(NS_WARN, "video:time_cal = %u, current_time = %u,vo_init_time = %u, tm_vo_tick = %u, vo_cal_time=%u, gap_time = %uid = %d",
			      video_cal_time,
			      current_time,
			      M_handle->recv_time_t.video_init_time,
			      M_handle->recv_time_t.time_video_tick,
			      M_handle->recv_time_t.video_cal_time,
			      M_handle->recv_time_t.prev_video_time_tick - current_time,
			      M_handle->stream_id);
		} else {
			M_handle->av_connect_flag == 0;
			M_handle->recv_time_t.time_video_tick = video_cal_time + M_handle->recv_time_t.video_cal_time;
		}
	}


	if(JPG_CODEC_TYPE == fh->m_data_codec) {
		if(fh->m_time_tick < M_handle->recv_time_t.prev_video_time_tick) {
			M_handle->recv_time_t.time_video_tick = M_handle->recv_time_t.time_video_tick + getJpgIntervalTime(M_handle->recv_time_t.prev_video_time, current_time);

			M_handle->recv_time_t.video_init_time = fh->m_time_tick;

			M_handle->recv_time_t.video_cal_time = M_handle->recv_time_t.time_video_tick;

			nslog(NS_WARN, "JPG:time_cal = %u, prev_video_time_cal = %u, time_video_tick = %u, video_cal_time=%u, id = %d",
			      video_cal_time,
			      M_handle->recv_time_t.prev_video_time_tick,
			      M_handle->recv_time_t.time_video_tick,
			      M_handle->recv_time_t.video_cal_time,
			      M_handle->stream_id);
		} else {
			M_handle->recv_time_t.time_video_tick = video_cal_time + M_handle->recv_time_t.video_cal_time;
		}
	}

	M_handle->recv_time_t.prev_video_time_tick = fh->m_time_tick;
	M_handle->recv_time_t.prev_video_time = current_time;
#endif

	switch(fh->m_dw_segment) {
		case START_FRAME:

			if(fh->m_frame_length > MAX_FRAME_LEN) {
				nslog(NS_ERROR, "fh->m_frame_length : [%d]", fh->m_frame_length);
				return_code = OPERATION_ERR;
				break;
			}

			if(((M_handle->offset) + part_frame_len) > fh->m_frame_length) {
				M_handle->offset  = 0;
				return_code = OPERATION_ERR;
				nslog(NS_WARN, "(M_handle->offset)+ part_frame_len = %d fh->m_frame_length = %d, id = %d", (M_handle->offset) + part_frame_len, fh->m_frame_length, M_handle->stream_id);
				break;
			}

			M_handle->offset = 0;
			r_memcpy(M_handle->frame_data, frame_data + FH_LEN, part_frame_len);
			M_handle->offset += part_frame_len;;
			return_code = OPERATION_SUCC;
			break;

		case MIDDLE_FRAME:      //0表示中间包

			if(M_handle->offset + part_frame_len > fh->m_frame_length) {
				M_handle->offset  = 0;
				return_code = OPERATION_SUCC;
				nslog(NS_WARN, "(M_handle->offset)+ part_frame_len = %d fh->m_frame_length = %d, id = %d", (M_handle->offset) + part_frame_len, fh->m_frame_length, M_handle->stream_id);
				break;
			}

			r_memcpy(M_handle->frame_data + M_handle->offset, frame_data + FH_LEN, part_frame_len);
			M_handle->offset += part_frame_len;
			return_code = OPERATION_SUCC;
			break;

		case LAST_FRAME:            //表示结尾包

			if(M_handle->offset + part_frame_len > fh->m_frame_length) {
				M_handle->offset  = 0;
				return_code = OPERATION_SUCC;
				nslog(NS_WARN, "(M_handle->offset)+ part_frame_len = %d fh->m_frame_length = %d, id = %d", (M_handle->offset) + part_frame_len, fh->m_frame_length, M_handle->stream_id);
				break;
			}

			r_memcpy(M_handle->frame_data + M_handle->offset, frame_data + FH_LEN, part_frame_len);
			M_handle->offset = 0;
			return_code = OPERATION_CONTINUE;
			break;

		case  INDEPENDENT_FRAME:
			r_memcpy(M_handle->frame_data, frame_data + FH_LEN, fh->m_frame_length);
			return_code = OPERATION_CONTINUE;
			break;

		default:
			nslog(NS_WARN, "fh->m_dw_segment : [%d]", fh->m_dw_segment);
			return_code = OPERATION_SUCC;
			break;
	}

	return return_code;
}


int32_t pkg_placed_in_frame_audio(int8_t *frame_data,  int32_t part_frame_len,  stream_handle *M_handle)
{
	hdb_freame_head_t *fh = (hdb_freame_head_t *)frame_data;
	int32_t return_code = OPERATION_SUCC;
	uint32_t current_time = 0;
	uint32_t audio_cal_time = 0, audio_cal_sys_time = 0;

	if(NULL == M_handle || NULL == frame_data) {
		nslog(NS_ERROR, "M_handle = %p, frame_data = %p", M_handle, frame_data);
		return OPERATION_ERR;
	}

#if 1
	current_time = getCurrentTime();

	if(0 >= M_handle->recv_time_t.init_audio_flag) {
		M_handle->recv_time_t.audio_init_time = fh->m_time_tick;
		M_handle->recv_time_t.init_audio_flag = 1;
		M_handle->recv_time_t.audio_cal_time = 0;
		M_handle->recv_time_t.prev_audio_time_tick = fh->m_time_tick;
		nslog(NS_INFO, "init audio_flag  id=%d", M_handle->stream_id);
	}

	//	nslog(NS_ERROR, "fh->m_time_tick = %u, M_handle->recv_time_t.audio_init_time=%u, id=%d", fh->m_time_tick, M_handle->recv_time_t.audio_init_time, M_handle->stream_id);

	if(fh->m_time_tick > M_handle->recv_time_t.audio_init_time) {
		audio_cal_time = fh->m_time_tick - M_handle->recv_time_t.audio_init_time;
	} else {
		audio_cal_time = M_handle->recv_time_t.audio_init_time - fh->m_time_tick;
	}

	if(fh->m_time_tick < M_handle->recv_time_t.prev_audio_time_tick ||
	   (fh->m_time_tick > M_handle->recv_time_t.prev_audio_time_tick &&
	    (fh->m_time_tick - M_handle->recv_time_t.prev_audio_time_tick) > EXAMIN_TIME) &&
	   M_handle->av_connect_flag == 0) {

		M_handle->recv_time_t.time_audio_tick = M_handle->recv_time_t.time_audio_tick + getIntervalTime(M_handle->recv_time_t.prev_audio_time, current_time);

		M_handle->recv_time_t.audio_init_time = fh->m_time_tick;

		M_handle->recv_time_t.audio_cal_time = M_handle->recv_time_t.time_audio_tick;

		nslog(NS_WARN, "audio:time_cal = %u, prev_audio_time_cal = %u, time_audio_tick = %u, audio_cal_time=%u, id = %d",
		      audio_cal_time,
		      M_handle->recv_time_t.prev_audio_time_tick,
		      M_handle->recv_time_t.time_audio_tick,
		      M_handle->recv_time_t.audio_cal_time,
		      M_handle->stream_id);
	} else {
		M_handle->av_connect_flag == 0;
		M_handle->recv_time_t.time_audio_tick = audio_cal_time + M_handle->recv_time_t.audio_cal_time;
	}

	M_handle->recv_time_t.prev_audio_time_tick = fh->m_time_tick;
	M_handle->recv_time_t.prev_audio_time = current_time;

#endif


	switch(fh->m_dw_segment) {
		case START_FRAME:
			if(fh->m_frame_length > MAX_AUDIO_LEN) {
				nslog(NS_ERROR, "fh->m_frame_length : [%d]", fh->m_frame_length);
				return_code = OPERATION_ERR;
				break;
			}

			if(((M_handle->audio_offset) + part_frame_len) > fh->m_frame_length) {
				M_handle->audio_offset  = 0;
				return_code = OPERATION_ERR;
				nslog(NS_WARN, "(M_handle->offset)+ part_frame_len = %d fh->m_frame_length = %d, id = %d", (M_handle->offset) + part_frame_len, fh->m_frame_length, M_handle->stream_id);
				break;
			}

			M_handle->audio_offset = 0;
			r_memcpy(M_handle->audio_data, frame_data + FH_LEN, part_frame_len);
			M_handle->audio_offset += part_frame_len;;
			return_code = OPERATION_SUCC;
			break;

		case MIDDLE_FRAME:      //0表示中间包
			if(M_handle->audio_offset + part_frame_len > fh->m_frame_length) {
				M_handle->audio_offset  = 0;
				return_code = OPERATION_SUCC;
				nslog(NS_WARN, "(M_handle->offset)+ part_frame_len = %d fh->m_frame_length = %d, id = %d", (M_handle->offset) + part_frame_len, fh->m_frame_length, M_handle->stream_id);
				break;
			}

			r_memcpy(M_handle->audio_data + M_handle->audio_offset, frame_data + FH_LEN, part_frame_len);
			M_handle->audio_offset += part_frame_len;
			return_code = OPERATION_SUCC;
			break;

		case LAST_FRAME:            //表示结尾包
			if(M_handle->audio_offset + part_frame_len > fh->m_frame_length) {
				M_handle->audio_offset  = 0;
				return_code = OPERATION_SUCC;
				nslog(NS_WARN, "(M_handle->offset)+ part_frame_len = %d fh->m_frame_length = %d, id = %d", (M_handle->offset) + part_frame_len, fh->m_frame_length, M_handle->stream_id);
				break;
			}

			r_memcpy(M_handle->audio_data + M_handle->audio_offset, frame_data + FH_LEN, part_frame_len);
			M_handle->audio_offset = 0;
			return_code = OPERATION_CONTINUE;
			break;

		case  INDEPENDENT_FRAME:
			r_memcpy(M_handle->audio_data, frame_data + FH_LEN, fh->m_frame_length);
			return_code = OPERATION_CONTINUE;
			break;

		default:
			nslog(NS_WARN, "fh->m_dw_segment : [%d]", fh->m_dw_segment);
			return_code = OPERATION_SUCC;
			break;
	}

	return return_code;
}



//test write file
int32_t write_long_tcp_data(FILE *fd, void *buf, int32_t buflen)
{
	int32_t total_recv = 0;
	int32_t recv_len = 0;

	while(total_recv < buflen) {
		recv_len = fwrite(buf, buflen, 1, fd);

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

/*==============================================================================
  函数: <send_msg_to_rec>
  功能: <发数据消息给录制>
  参数: hdb_freame_head_t *fh, stream_handle *M_handle
  返回值: 成功返回0，失败返回-1.
  Created By liuchsh 2012.11.16 15:37:50 For EDU
  ==============================================================================*/
static int32_t send_msg_to_rec(hdb_freame_head_t *fh, stream_handle *M_handle)
{
	int return_code = OPERATION_SUCC;
	int8_t *rec_frame_data = NULL;
	parse_data_t *rec_frame_head = NULL;
	m_msgque  m_rec_msgque  ;

	rec_frame_data = (int8_t *)r_malloc(fh->m_frame_length);

	if(NULL == rec_frame_data) {
		nslog(NS_ERROR, "malloc is error !!!");
		return_code = RECEIVE_MALLOC_ERR;
		return return_code;
	}

	rec_frame_head = (parse_data_t *)r_malloc(sizeof(parse_data_t));

	if(NULL == rec_frame_head) {
		if(rec_frame_data) {
			r_free(rec_frame_data);
			rec_frame_data = NULL;
		}

		return_code = RECEIVE_MALLOC_ERR;
		return return_code;
	}

	r_memset(rec_frame_head, 0, sizeof(parse_data_t));
	r_memset(rec_frame_data, 0, fh->m_frame_length);

	if(H264_CODEC_TYPE == fh->m_data_codec) {
		r_memcpy(rec_frame_data, M_handle->frame_data, fh->m_frame_length);
		rec_frame_head->data_type = R_VIDEO;
		rec_frame_head->sample_rate = fh->m_frame_rate;
		rec_frame_head->time_tick = M_handle->recv_time_t.time_video_tick;

	} else if(AAC_CODEC_TYPE == fh->m_data_codec) {
		r_memcpy(rec_frame_data, M_handle->audio_data, fh->m_frame_length);
		rec_frame_head->data_type = R_AUDIO;
		rec_frame_head->code_rate = fh->m_colors;
		rec_frame_head->sample_rate = fh->m_hight;
		rec_frame_head->time_tick = M_handle->recv_time_t.time_audio_tick;

	} else if(JPG_CODEC_TYPE == fh->m_data_codec) {
		r_memcpy(rec_frame_data, M_handle->frame_data, fh->m_frame_length);
		rec_frame_head->data_type = R_JPEG;
		rec_frame_head->time_tick = M_handle->recv_time_t.time_video_tick;
	}

	rec_frame_head->data_len = fh->m_frame_length;
	rec_frame_head->data = rec_frame_data;
	rec_frame_data = NULL;
	rec_frame_head->flags = fh->m_dw_flags;
	rec_frame_head->sindex = M_handle->stream_id - 1;

	rec_frame_head->index = 0;
	rec_frame_head->audio_sindex = M_handle->stream_id - 1;
	rec_frame_head->height = fh->m_hight;
	rec_frame_head->width = fh->m_width;
	rec_frame_head->blue_flag = !(fh->m_others);

	m_rec_msgque.msgtype = M_handle->stream_id;
	m_rec_msgque.msgbuf = (int8_t *)rec_frame_head;
	M_handle->recv_pri.to_rec = 1;
	return_code = r_msg_send(M_handle->msg_recv_to_rec, &m_rec_msgque, MsgqueLen - sizeof(long), IPC_NOWAIT);

	if(0 > return_code) {
		if(rec_frame_head->data) {
			r_free(rec_frame_head->data);
			rec_frame_head->data = NULL;
		}

		if(rec_frame_head) {
			r_free(rec_frame_head);
			rec_frame_head = NULL;
		}

		if(FALSE == IS_FLAG_BIT_TRUE(M_handle->log_flag, RECORD_BIT)) {
			nslog(NS_WARN, "msgsnd to record failed, msgid = %d, errno = %d, stream_id = %d", M_handle->msg_recv_to_rec, errno, M_handle->stream_id);
			SET_FLAG_BIT_TRUE(M_handle->log_flag, RECORD_BIT);//置第0位为1.
		}

		M_handle->recv_pri.to_rec = 0;
		return OPERATION_SUCC;
	}

	SET_FLAG_BIT_FALSE(M_handle->log_flag, RECORD_BIT);//置第0位为0.
	return OPERATION_SUCC;
}

static int32_t send_msg_to_live(hdb_freame_head_t *fh, stream_handle *M_handle)
{
	int return_code = OPERATION_SUCC;
	int8_t *live_frame_data = NULL;
	parse_data_t *live_frame_head = NULL;
	m_msgque  m_live_msgque  ;

	if(NULL == M_handle->frame_data || NULL == M_handle->audio_data) {
		nslog(NS_ERROR, "M_handle->frame_data = %p, M_handle->audio_data = %p, stream_id = %d", M_handle->frame_data, M_handle->audio_data, M_handle->stream_id);
		return OPERATION_ERR;
	}

	live_frame_data = (int8_t *)r_malloc(fh->m_frame_length);

	if(NULL == live_frame_data) {
		nslog(NS_ERROR, "malloc is error !!!");
		return_code = RECEIVE_MALLOC_ERR;
		return return_code;
	}

	live_frame_head = (parse_data_t *)r_malloc(sizeof(parse_data_t));

	if(NULL == live_frame_head) {
		if(live_frame_data) {
			r_free(live_frame_data);
			live_frame_data = NULL;
		}

		return_code = RECEIVE_MALLOC_ERR;
		return return_code;
	}

	r_memset(live_frame_head, 0, sizeof(parse_data_t));
	r_memset(live_frame_data, 0, fh->m_frame_length);

	//nslog(NS_WARN, "fh->m_data_codec:[%d]\n", fh->m_data_codec);
	if(H264_CODEC_TYPE == fh->m_data_codec) {
		r_memcpy(live_frame_data, M_handle->frame_data, fh->m_frame_length);
		live_frame_head->data_type = R_VIDEO;
		live_frame_head->sample_rate = fh->m_frame_rate;
		live_frame_head->time_tick = M_handle->recv_time_t.time_video_tick;

	} else if(AAC_CODEC_TYPE == fh->m_data_codec) {
		r_memcpy(live_frame_data, M_handle->audio_data, fh->m_frame_length);
		live_frame_head->data_type = R_AUDIO;
		live_frame_head->code_rate = fh->m_colors;
		live_frame_head->sample_rate = fh->m_hight;
		live_frame_head->time_tick = M_handle->recv_time_t.time_audio_tick;

	} else if(JPG_CODEC_TYPE == fh->m_data_codec) {
		r_memcpy(live_frame_data, M_handle->frame_data, fh->m_frame_length);
		live_frame_head->data_type = R_JPEG;
		live_frame_head->time_tick = M_handle->recv_time_t.time_video_tick;

	}

	live_frame_head->data_len = fh->m_frame_length;
	live_frame_head->data = live_frame_data;
	live_frame_data = NULL;
	live_frame_head->flags = fh->m_dw_flags;
	live_frame_head->sindex = M_handle->stream_id - 1;
	live_frame_head->index = 0;
	live_frame_head->audio_sindex = M_handle->stream_id - 1;
	live_frame_head->height = fh->m_hight;
	live_frame_head->width = fh->m_width;
	live_frame_head->end_flag = M_handle->live_status;
	live_frame_head->blue_flag = !(fh->m_others);

	m_live_msgque.msgtype = M_handle->stream_id;
	m_live_msgque.msgbuf = (int8_t *)live_frame_head;

	M_handle->recv_pri.to_live = 1;

	return_code = r_msg_send(M_handle->msg_recv_to_live, &m_live_msgque, MsgqueLen - sizeof(long), IPC_NOWAIT);

	if(0 > return_code) {
		if(live_frame_head->data) {
			r_free(live_frame_head->data);
			live_frame_head->data = NULL;
		}

		if(live_frame_head) {
			r_free(live_frame_head);
			live_frame_head = NULL;
		}

		if(FALSE == IS_FLAG_BIT_TRUE(M_handle->log_flag, LIVE_BIT)) {
			nslog(NS_WARN, "msgsnd to record failed, msgid = %d, errno = %d, stream_id=%d", M_handle->msg_recv_to_live, errno, M_handle->stream_id);
			SET_FLAG_BIT_TRUE(M_handle->log_flag, LIVE_BIT);//置第1位为1.
		}

		M_handle->recv_pri.to_live = 0;
		return OPERATION_SUCC;
	}

	SET_FLAG_BIT_FALSE(M_handle->log_flag, LIVE_BIT);//置第1位为0.
	return OPERATION_SUCC;
}

// add zl
int32_t send_media_frame_data(stream_handle *M_handle, void *data, int32_t data_len, udp_send_module_handle *p_udp_hand)
{
	hdb_freame_head_t *fh = (hdb_freame_head_t *)data;
	int32_t frame_data_len = data_len - FH_LEN;
	m_msgque  m_live_msgque;
	int return_code = OPERATION_SUCC;

	//nslog(NS_INFO, "fh->m_dw_packet_number = %u, stream_id = %d, fh->m_frame_length = %d, part = %d, fh->m_dw_segment =%d, type = %d, fh->m_time_tick = %hu", fh->m_dw_packet_number, M_handle->stream_id, fh->m_frame_length, frame_data_len, fh->m_dw_segment, (fh->m_data_codec == AAC_CODEC_TYPE)?1:0, fh->m_time_tick);

	// assert(M_handle && data);

	if(NULL == M_handle || NULL == data) {
		nslog(NS_ERROR, "M_handle = %p, data = %p", M_handle, data);
		return OPERATION_ERR;
	}

	if((!(AUDIO_ID == M_handle->stream_id || AUDIO_IDT == M_handle->stream_id)) && AAC_CODEC_TYPE == fh->m_data_codec) {
		return OPERATION_SUCC;
	}

	// add zl questino ??
	if(AAC_CODEC_TYPE == fh->m_data_codec) {
		return OPERATION_SUCC;//add by lcs
	}

	if(H264_CODEC_TYPE == fh->m_data_codec || JPG_CODEC_TYPE == fh->m_data_codec) {

		M_handle->recv_pri.width = fh->m_width;
		M_handle->recv_pri.hight = fh->m_hight;
		M_handle->recv_pri.frame_rate = fh->m_frame_rate;

		if(pkg_placed_in_frame((int8_t *)(data), frame_data_len, M_handle) < OPERATION_CONTINUE) {
			return OPERATION_SUCC;
		}

		if(FALSE == IS_FLAG_BIT_TRUE(M_handle->log_flag, RECV_VIDEO_I_FRAME_DATA) && I_FRAME == fh->m_dw_flags) {
			SET_FLAG_BIT_TRUE(M_handle->log_flag, RECV_VIDEO_I_FRAME_DATA);
			//            nslog(NS_INFO, "M_handle->recv_time_t.time_tick = %u getCurrentTime() = %u, stream_id = %d", M_handle->recv_time_t.time_tick, getCurrentTime(), M_handle->stream_id);
		}


		if((TRUE == IS_FLAG_BIT_TRUE(M_handle->log_flag, RECV_VIDEO_I_FRAME_DATA)) || JPG_CODEC_TYPE == fh->m_data_codec) {


#if 0   // add zl

			if(0 > M_handle->recv_time_t.reserve) {
				M_handle->recv_time_t.reserve = M_handle->recv_time_t.time_video_tick;
			}

			if(M_handle->recv_time_t.time_video_tick < M_handle->recv_time_t.reserve)
				nslog(NS_ERROR, "VIDEO:M_handle->recv_time_t.time_video_tick=%u,M_handle->recv_time_t.reserve=%u, id=%d",
				      M_handle->recv_time_t.time_video_tick,
				      M_handle->recv_time_t.reserve,
				      M_handle->stream_id);

			M_handle->recv_time_t.reserve = M_handle->recv_time_t.time_video_tick;

#endif

			if(START_REC == get_rec_status(M_handle)) {
				return_code = send_msg_to_rec(fh, M_handle);
			}

			if(START_LIVE == get_live_status(M_handle)) {
				return_code = send_msg_to_live(fh, M_handle);
			}

			// add zl
			if(NULL != p_udp_hand) {
				send_udp_handle *udp_hand = &p_udp_hand->udp_hand;
				frame_info_t frame_info;
				//	udp_hand->user_data = M_handle->frame_data;
				frame_info.m_frame_length = fh->m_frame_length;
				frame_info.time_tick = M_handle->recv_time_t.time_video_tick;
				frame_info.is_blue = !(fh->m_others);
				frame_info.m_data_codec = fh->m_data_codec;
				frame_info.m_dw_flags = fh->m_dw_flags;
				frame_info.m_hight = fh->m_hight;
				frame_info.m_width = fh->m_width;
				frame_info.m_frame_rate = fh->m_frame_rate;
				// add zl
				memcpy(udp_hand->src_data, M_handle->frame_data, fh->m_frame_length);
				UdpSend_rtp_data(p_udp_hand, &frame_info);
			}
		}

	}

	if(AAC_CODEC_TYPE == fh->m_data_codec) {

		M_handle->recv_pri.stream_type = 2;
		M_handle->recv_pri.data_type = R_AUDIO;
		M_handle->recv_pri.code_rate = fh->m_colors;
		M_handle->recv_pri.sample_rate = fh->m_hight;

		if(pkg_placed_in_frame_audio((int8_t *)(data), frame_data_len, M_handle) < OPERATION_CONTINUE) {
			return OPERATION_SUCC;
		}

		if(TRUE == IS_FLAG_BIT_TRUE(M_handle->log_flag, RECV_VIDEO_I_FRAME_DATA)) {
			if(START_REC == get_rec_status(M_handle)) {
				return_code = send_msg_to_rec(fh, M_handle);
			}

			if(START_LIVE == get_live_status(M_handle)) {
				return_code = send_msg_to_live(fh, M_handle);
			}

			// add zl
			if(NULL != p_udp_hand) {
				send_udp_handle *udp_hand = &p_udp_hand->udp_hand;
				frame_info_t frame_info;
				//				udp_hand->user_data = M_handle->audio_data;
				frame_info.m_frame_length = fh->m_frame_length;
				frame_info.time_tick = M_handle->recv_time_t.time_audio_tick;
				frame_info.is_blue = !(fh->m_others);
				frame_info.m_data_codec = fh->m_data_codec;
				frame_info.m_dw_flags = fh->m_dw_flags;
				frame_info.m_hight = fh->m_hight;
				frame_info.m_width = fh->m_width;
				frame_info.m_frame_rate = fh->m_frame_rate;
				memcpy(udp_hand->src_data, M_handle->audio_data, fh->m_frame_length);
				UdpSend_rtp_data(p_udp_hand, &frame_info);
			}
		}

	}

#if  0

	if(H264_CODEC_TYPE == fh->m_data_codec && I_FRAME == fh->m_dw_flags) {
		SET_FLAG_BIT_TRUE(M_handle->log_flag, 4);
	}

	if(H264_CODEC_TYPE == fh->m_data_codec && TRUE ==  IS_FLAG_BIT_TRUE(M_handle->log_flag, 4)) {
		FILE *fp = NULL;
		char buf[20] = {0};
		sprintf(buf, "%d.264", M_handle->stream_id);
		fp = fopen(buf, "a");

		if(fp == NULL) {
			nslog(NS_ERROR, "fopen error, errno = %d", errno);
			return OPERATION_SUCC;
		}

		//nslog(NS_INFO, "M_handle->frame_data = %p, fh->m_frame_length = %d", M_handle->frame_data, fh->m_frame_length);
		write_long_tcp_data(fp, M_handle->frame_data,  fh->m_frame_length);
		fclose(fp);
		fp = NULL;
	}

#if 0

	if(AAC_CODEC_TYPE == fh->m_data_codec) {
		FILE *fp = NULL;
		char buf[20] = {0};
		sprintf(buf, "%d.aac", M_handle->stream_id);
		fp = fopen(buf, "a");

		if(fp == NULL) {
			nslog(NS_ERROR, "fopen error, errno = %d", errno);
			return OPERATION_SUCC;
		}

		//nslog(NS_INFO, "M_handle->offset = %d, fh->m_frame_length = %d, strlen(frame_data) = %lld", M_handle->offset, fh->m_frame_length, r_strlen(M_handle->frame_data));
		write_long_tcp_data(fp, M_handle->audio_data,  fh->m_frame_length);
		fclose(fp);
		fp = NULL;
	}

	if(JPG_CODEC_TYPE == fh->m_data_codec) {
		FILE *fp = NULL;
		char buf[20] = {0};
		sprintf(buf, "%d.jpg", fh->m_dw_packet_number);
		fp = fopen(buf, "a");

		if(fp == NULL) {
			nslog(NS_ERROR, "fopen error, errno = %d", errno);
			return OPERATION_SUCC;
		}

		//nslog(NS_INFO, "M_handle->offset = %d, fh->m_frame_length = %d, strlen(frame_data) = %lld", M_handle->offset, fh->m_frame_length, r_strlen(M_handle->frame_data));
		write_long_tcp_data(fp, M_handle->frame_data,  fh->m_frame_length);
		fclose(fp);
		fp = NULL;
	}

#endif
#endif
	return return_code;
}


// add zl callback func
int recv_deal_udp_recv_data(udp_recv_stream_info_t *p_stream_info, char *data, int len)
{
	if(NULL == p_stream_info || NULL == data) {
		nslog(NS_ERROR, "M_handle = %p, data = %p", p_stream_info, data);
		return OPERATION_ERR;
	}

	if(NULL == p_stream_info->user_data) {
		nslog(NS_ERROR, "p_stream_info->user_data = %p", p_stream_info->user_data);
		return OPERATION_ERR;
	}

	//nslog(NS_WARN, "len:[%d]\n", len);
	stream_handle *M_handle = p_stream_info->user_data;
	int32_t return_code = 0;
	hdb_freame_head_t fh;

	memset(&fh, 0, sizeof(hdb_freame_head_t));


	fh.m_data_codec = p_stream_info->type;
	fh.m_hight = p_stream_info->height;
	fh.m_width = p_stream_info->width;
	fh.m_dw_flags = p_stream_info->IDR_FLAG;
	fh.m_others = !(p_stream_info->is_blue);
	fh.m_colors = p_stream_info->m_colors;
	fh.m_frame_rate = p_stream_info->frame_rate;
	fh.m_frame_length = len;
	if(AAC_CODEC_TYPE == fh.m_data_codec) {

		fh.m_frame_rate = p_stream_info->samplerate;
		M_handle->recv_time_t.time_audio_tick = p_stream_info->timestamp;

#if 1
		M_handle->recv_pri.stream_type = 2;
		M_handle->recv_pri.data_type = R_AUDIO;
		M_handle->recv_pri.code_rate = fh.m_colors;
		M_handle->recv_pri.sample_rate = fh.m_hight;
		M_handle->recv_pri.is_data = 1;
		M_handle->recv_pri.frame_rate = fh.m_frame_rate;
#endif
		r_memcpy(M_handle->audio_data, data, fh.m_frame_length);// 都写两个函数可以少10次拷贝。

		if(START_REC == get_rec_status(M_handle)) {
			return_code = send_msg_to_rec(&fh, M_handle);
		} else {
			M_handle->recv_pri.to_rec = 0;
		}

		if(START_LIVE == get_live_status(M_handle)) {
			return_code = send_msg_to_live(&fh, M_handle);
		} else {
			M_handle->recv_pri.to_live = 0;
		}
	}

	if(H264_CODEC_TYPE == fh.m_data_codec || JPEG_CODEC_TYPE == fh.m_data_codec) {

#if 1
		M_handle->recv_pri.stream_type = 0;
		M_handle->recv_pri.data_type = R_VIDEO;
		M_handle->recv_pri.is_data = 1;
		M_handle->recv_pri.width = fh.m_width;
		M_handle->recv_pri.hight = fh.m_hight;
		M_handle->recv_pri.frame_rate = fh.m_frame_rate;
#endif
		M_handle->recv_time_t.time_video_tick = p_stream_info->timestamp;
		r_memcpy(M_handle->frame_data, data, fh.m_frame_length);

		if(START_REC == get_rec_status(M_handle)) {
			return_code = send_msg_to_rec(&fh, M_handle);
		} else {
			M_handle->recv_pri.to_rec = 0;
		}

		if(START_LIVE == get_live_status(M_handle)) {
			return_code = send_msg_to_live(&fh, M_handle);
		} else {
			M_handle->recv_pri.to_live = 0;
		}
	}

}

