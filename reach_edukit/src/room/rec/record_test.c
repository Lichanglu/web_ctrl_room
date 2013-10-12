#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reach_os.h"
#include "nslog.h"
#include "record.h"
#include "receive_module.h"
#define COURSE_TEST_DIR	"/home/mp4/course_test"
#define ROOM_KEY		111111
#define	NSLOG_CONF	"/usr/local/reach/live/nslog_live.conf"
#define CONTROL_ADDR					"192.168.4.139"
#define ENCODE_PORT  						(3400)
/*
struct tm {
	int tm_sec;	//seconds
	int tm_min;	//minutes
	int tm_hour;	//hours
	int tm_mday;	//day of the month
	int tm_mon;	//month
	int tm_year;	//year
	int tm_wday;	//day of the week
	int tm_yday;	//day in the year
	int tm_isdst;	//daylight saving time
}
*/
void set_ContentInfo(ContentInfo_t *cinfo)
{
	r_strcpy(cinfo->MainTeacher, "Ding");
	r_strcpy(cinfo->CName, "English");
	r_strcpy(cinfo->ScmType, "Type1");
	r_strcpy(cinfo->Notes, "Course Record Test !!!");
	localtime_t t;
	get_localtime(&t);
	r_sprintf(cinfo->RecDateTime, "%04d-%02d-%02d %02d:%02d:%02d",
	          t.tm_year, t.tm_mon, t.tm_mday,
	          t.tm_hour, t.tm_min, t.tm_sec);
	nslog(NS_INFO, "RecDateTime:[%s]", cinfo->RecDateTime);
}

int32_t get_media_buf_info(mp4_record_t *mr, parse_data_t *buf_info)
{
	msgque msgp;
	r_memset(&msgp, 0, sizeof(msgque));
	nslog(NS_INFO, " mr->course_data_fd[%d]:<%d>",  mr->sindex + 1, mr->course_data_fd);

	if(0 > r_msg_recv(mr->course_data_fd, &msgp, sizeof(msgque) - sizeof(long), mr->sindex + 1, IPC_NOWAIT)) {
		return -1;
	}
	
	nslog(NS_INFO, "msgp.msgbuf:[%p]", ((parse_data_t *)(msgp.msgbuf)));
	nslog(NS_INFO, "sindex:[%d]", ((parse_data_t *)(msgp.msgbuf))->sindex);
	nslog(NS_INFO, "data_len:[%d]", ((parse_data_t *)(msgp.msgbuf))->data_len);
	nslog(NS_INFO, "data:[%p]", ((parse_data_t *)(msgp.msgbuf))->data);
	nslog(NS_INFO, "flags:[%d]", ((parse_data_t *)(msgp.msgbuf))->flags);
	nslog(NS_INFO, "data_type:[%d]", ((parse_data_t *)(msgp.msgbuf))->data_type);
	nslog(NS_INFO, "height:[%d]", ((parse_data_t *)(msgp.msgbuf))->height);
	nslog(NS_INFO, "width:[%d]", ((parse_data_t *)(msgp.msgbuf))->width);
	nslog(NS_INFO, "sample_rate:[%d]", ((parse_data_t *)(msgp.msgbuf))->sample_rate);
	nslog(NS_INFO, "blue_flag:[%d]", ((parse_data_t *)(msgp.msgbuf))->blue_flag);
	
	r_memcpy(buf_info, msgp.msgbuf, sizeof(parse_data_t));
	r_free(msgp.msgbuf);
	return 0;
}

int32_t get_jpeg_buf_info(jpeg_record_t *jr, parse_data_t *buf_info)
{
	msgque msgp;
	r_memset(&msgp, 0, sizeof(msgque));
	nslog(NS_INFO, "jr->course_data_fd[%d]:<%d>", jr->sindex + 1, jr->course_data_fd);

	if(0 > r_msg_recv(jr->course_data_fd, &msgp, sizeof(msgque) - sizeof(long), jr->sindex + 1, IPC_NOWAIT)) {
		return -1;
	}
	nslog(NS_INFO, "msgp.msgbuf:[%p]", ((parse_data_t *)(msgp.msgbuf)));
	nslog(NS_INFO, "sindex:[%d]", ((parse_data_t *)(msgp.msgbuf))->sindex);
	nslog(NS_INFO, "data_len:[%d]", ((parse_data_t *)(msgp.msgbuf))->data_len);
	nslog(NS_INFO, "data:[%p]", ((parse_data_t *)(msgp.msgbuf))->data);
	nslog(NS_INFO, "flags:[%d]", ((parse_data_t *)(msgp.msgbuf))->flags);
	nslog(NS_INFO, "data_type:[%d]", ((parse_data_t *)(msgp.msgbuf))->data_type);
	nslog(NS_INFO, "height:[%d]", ((parse_data_t *)(msgp.msgbuf))->height);
	nslog(NS_INFO, "width:[%d]", ((parse_data_t *)(msgp.msgbuf))->width);
	nslog(NS_INFO, "sample_rate:[%d]", ((parse_data_t *)(msgp.msgbuf))->sample_rate);
	nslog(NS_INFO, "blue_flag:[%d]", ((parse_data_t *)(msgp.msgbuf))->blue_flag);
	
	r_memcpy(buf_info, msgp.msgbuf, sizeof(parse_data_t));
	r_free(msgp.msgbuf);
	return 0;
}
void release_media_buf(parse_data_t *buf_info)
{
	r_free(buf_info->data);
}

void release_jpeg_buf(parse_data_t *buf_info)
{
	r_free(buf_info->data);
}

int main(int argc, char **argv)
{

	int rc;
	rc = dzlog_init(NSLOG_CONF, "ns_cat");

	if(rc) {
		printf("dzlog_init failed : <%s>\n", NSLOG_CONF);
		return -1;
	}

	MediaSysInit();

	int32_t i = 0;
	recv_param r_param[MAX_STREAM];
	recv_room_handle *recv_handle = NULL;
	r_memset(r_param, 0, sizeof(recv_param) * MAX_STREAM);

	r_memcpy(r_param[0].ipaddr, CONTROL_ADDR, sizeof(CONTROL_ADDR));
	r_param[0].port = ENCODE_PORT;
	r_param[0].stream_id = 1;
	r_param[0].status = RUNNING;

	r_memcpy(r_param[1].ipaddr, CONTROL_ADDR, sizeof(CONTROL_ADDR));
	r_param[1].port = ENCODE_PORT;
	r_param[1].stream_id = 2;
	r_param[1].status = RUNNING;

	r_memcpy(r_param[2].ipaddr, CONTROL_ADDR, sizeof(CONTROL_ADDR));
	r_param[2].port = ENCODE_PORT;
	r_param[2].stream_id = 3;
	r_param[2].status = RUNNING;

	r_memcpy(r_param[3].ipaddr, CONTROL_ADDR, sizeof(CONTROL_ADDR));
	r_param[3].port = ENCODE_PORT;
	r_param[3].stream_id = 4;
	r_param[3].status = RUNNING;

	r_strcpy(r_param[4].ipaddr, "192.168.4.246");
	r_param[4].port = ENCODE_PORT;
	r_param[4].stream_id = 5;
	r_param[4].status = RUNNING;

	course_record_t *cr = NULL;

	course_record_condition_t crc;
	r_memset(&crc, 0, sizeof(course_record_condition_t));
	set_ContentInfo(&(crc.cinfo));
	r_strcpy(crc.record_root_dir, COURSE_TEST_DIR);
	r_strcpy(crc.dev_id, "xxxxxxxxxxxx");

	crc.room_ctrl_msg_fd = r_msg_create_key("/home", 1);

	if(crc.room_ctrl_msg_fd < 0) {
		nslog(NS_ERROR, "crc.room_ctrl_msg_fd is failed");
		return -1;
	}

	crc.get_jpeg_buf_info_callback = get_jpeg_buf_info;
	crc.get_media_buf_info_callback = get_media_buf_info;
	crc.release_jpeg_buf_callback = release_jpeg_buf;
	crc.release_media_buf_callback = release_media_buf;

	crc.roomid = 0;
	crc.sindex_sum = 5;
	crc.max_course_record_duration = 8 * 3600;

	crc.rinfo[0].width = 1280;
	crc.rinfo[0].height = 720;
	crc.rinfo[0].audio_SampleRate = 48000;
	crc.rinfo[0].audio_BitRate = 48000;

	crc.rinfo[1].width = 352;
	crc.rinfo[1].height = 288;
	crc.rinfo[1].audio_SampleRate = 48000;
	crc.rinfo[1].audio_BitRate = 48000;

	crc.rinfo[2].width = 1280;
	crc.rinfo[2].height = 720;
	crc.rinfo[2].audio_SampleRate = 0;
	crc.rinfo[2].audio_BitRate = 0;

	crc.rinfo[3].width = 352;
	crc.rinfo[3].height = 288;
	crc.rinfo[3].audio_SampleRate = 0;
	crc.rinfo[3].audio_BitRate = 0;

	crc.rinfo[4].width = 1024;
	crc.rinfo[4].height = 768;
	crc.rinfo[4].audio_SampleRate = 0;
	crc.rinfo[4].audio_BitRate = 0;


	recv_handle = register_room_receive_module(5);


	cr = register_course_record_mode(&crc);

	if(NULL == cr) {
		nslog(NS_ERROR, "register_course_record_mode is failed !!!\n");
		return -1;
	}

	for(i = 0; i < recv_handle->stream_num; i++) {
		recv_handle->set_recv_to_rec_msgid(cr->course_data_msgid, &(recv_handle->stream_hand[i]));
		recv_handle->set_room_info(&(r_param[i]), &(recv_handle->stream_hand[i]));
		recv_handle->open_room_connect(&recv_handle->stream_hand[i]);
		recv_handle->set_rec_status(&recv_handle->stream_hand[i], CONNECT);
	}

	cr->record_start(cr);

	while(1) {
		sleep(1);
	}

	zlog_fini();

	return 0;
}


