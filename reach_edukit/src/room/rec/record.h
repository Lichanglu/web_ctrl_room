#ifndef	__RECORD_H__
#define	__RECORD_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "media_msg.h"
#include "stdint.h"
#include "reach_os.h"
#include "reach_socket.h"
#include "Media.h"

#define	COURSE_TEMPLATE_DIR		"/usr/local/reach/course_temp"
#define	RECORD_DATA_BASE_KEY		20000
#define	RECORD_MSG_BASE_KEY		30000
#define	REQ_MSG_BASE_TYPE			30
#define	RECORD_DEFAULT_ROOT_DIR	"/opt/Rec"
#define 	DEFAULT_MAX_COURSE_DUR	(8 * 3600)
#define MAX_SPLIT_LEN				(700 * 1000 * 1000)
#define	DEFAULT_DURATION			30

typedef struct _stream_type_sindex_sum {
	int32_t hd_sindex_sum;
	int32_t sd_sindex_sum;
	int32_t jpeg_sindex_sum;
} stream_type_sindex_sum_t;

typedef struct _record_report_info {
	int32_t sindex;
	int32_t width;
	int32_t height;
	int32_t totaltime;
	uint32_t totalsize;
	int8_t filename[128];
	int32_t blue_flag;
	int32_t data_status;
} record_report_info_t;


typedef enum _record_status {
    RECORD_RESUME,
    RECORD_PAUSE,
    RECORD_CLOSE
} record_status_t;

typedef enum _stream_type {
	RECORD_HD = 1,
	RECORD_SD,
	RECORD_JPEG
} stream_type_t;

typedef struct _mp4_record mp4_record_t;
typedef struct _jpeg_record  jpeg_record_t;
typedef struct _mp4_course_record mp4_course_record_t;
typedef struct _course_record course_record_t;

typedef struct _record_info {
	stream_type_t stream_type;
	int32_t width;
	int32_t height;
	int32_t audio_SampleRate;
	int32_t audio_BitRate;
} record_info_t;

typedef struct _jpeg_record_condition {
	int32_t sindex;
	int8_t record_file_dir[1024];
	record_info_t record_info;
	int32_t course_msg_fd;
	int32_t course_data_fd;
	int32_t (*get_jpeg_buf_info_callback)(jpeg_record_t *jr, parse_data_t *buf_info);
	void (*release_jpeg_buf_callback)(parse_data_t *buf_info);
} jpeg_record_condition_t;

typedef struct _mp4_record_condition {
	int32_t sindex;
	int8_t record_file[1024];
	record_info_t record_info;
	int32_t course_msg_fd;
	int32_t course_data_fd;
	int32_t (*get_media_buf_info_callback)(mp4_record_t *mr, parse_data_t *buf_info);
	void (*release_media_buf_callback)(parse_data_t *buf_info);
} mp4_record_condition_t;

struct _mp4_record {
	int32_t sindex;
	MuxWriter MW;
	int8_t filename[128];
	record_status_t rs;
	int32_t course_msg_fd;
	int32_t course_data_fd;
	void *handle;
	pthread_t p;
	record_info_t record_info;
	int32_t (*record_start)(mp4_record_t *mr);
	void (*record_resume)(mp4_record_t *mr);
	void (*record_pause)(mp4_record_t *mr);
	void (*record_close)(mp4_record_t *mr);
	
	int32_t (*get_media_buf_info_callback)(mp4_record_t *mr, parse_data_t *buf_info);
	void (*release_media_buf_callback)(parse_data_t *buf_info);
} ;
struct _jpeg_record {
	int32_t sindex;
	record_status_t rs;
	int32_t course_msg_fd;
	int32_t course_data_fd;
	int8_t jpeg_dir[512];
	void *handle;
	pthread_t p;
	record_info_t record_info;
	int32_t (*record_start)(jpeg_record_t *jr);
	void (*record_resume)(jpeg_record_t *jr);
	void (*record_pause)(jpeg_record_t *jr);
	void (*record_close)(jpeg_record_t *jr);
	
	int32_t (*get_jpeg_buf_info_callback)(jpeg_record_t *jr, parse_data_t *buf_info);
	void (*release_jpeg_buf_callback)(parse_data_t *buf_info);
};

typedef struct _mp4_course_record_condition {
	int8_t course_root_dir[256];
	int32_t video_split_index;
	int32_t course_msg_fd;
	int32_t course_data_fd;
	int32_t roomid;
	int32_t mindex_sum;
	record_info_t rinfo[MAX_STREAMS];
	int32_t (*get_media_buf_info_callback)(mp4_record_t *mr, parse_data_t *buf_info);
	void (*release_media_buf_callback)(parse_data_t *buf_info);
} mp4_course_record_condition_t;


struct _mp4_course_record {
	int32_t mindex_sum;
	mp4_record_t *mr[MAX_STREAMS];
	int32_t (*record_start)(mp4_course_record_t *mcr);
	void (*record_resume)(mp4_course_record_t *mcr);
	void (*record_pause)(mp4_course_record_t *mcr);
	void (*record_close)(mp4_course_record_t *mcr);
} ;

typedef struct _ContentInfo {
	int8_t RecordID[132];
	int8_t MainTeacher[132];
	int8_t CName[260];
	int8_t ScmType[32];
	int8_t RecDateTime[32];
	int8_t Notes[1028];
} ContentInfo_t;

typedef struct _course_record_condition {
	ContentInfo_t cinfo;
	int8_t record_root_dir[128];
	int8_t dev_id[64];
	int32_t room_ctrl_msg_fd;
	int32_t roomid;
	int32_t sindex_sum;
	int32_t max_course_record_duration;	//unit:s
	record_info_t rinfo[MAX_STREAMS];
	
	int32_t (*get_jpeg_buf_info_callback)(jpeg_record_t *jr, parse_data_t *buf_info);
	void (*release_jpeg_buf_callback)(parse_data_t *buf_info);
	int32_t (*get_media_buf_info_callback)(mp4_record_t *mr, parse_data_t *buf_info);
	void (*release_media_buf_callback)(parse_data_t *buf_info);
} course_record_condition_t;


struct _course_record {
	mp4_course_record_condition_t crc;
	mp4_course_record_t *mcr;
	jpeg_record_t *jr;
	record_status_t rs;
	int8_t course_root_dir[256];
	int8_t recovery_course[256];
	int8_t RecordID[129];
	int32_t sindex_sum;
	int32_t course_msg_fd;
	int32_t course_record_totaltime; //unit:s
	int32_t max_course_record_duration;	//unit:s
	pthread_t p;
	int32_t room_ctrl_msg_fd;
	stream_type_sindex_sum_t s;
	record_report_info_t rep_info_tmp[MAX_STREAMS];
	pthread_mutex_t print_mutex;
	pthread_t print_p;
	int32_t print_close;
	int32_t course_data_msgid;
	int32_t (*record_start)(course_record_t *cr);
	void (*record_resume)(course_record_t *cr);
	void (*record_pause)(course_record_t *cr);
	void (*record_close)(course_record_t *cr);
	record_status_t (*get_record_status)(course_record_t *cr);
	int8_t* (*get_course_root_dir)(course_record_t *cr);
	 int8_t* (*get_RecordID)(course_record_t *cr);
} ;

course_record_t *register_course_record_mode(course_record_condition_t *cond);
void unregister_course_record_mode(course_record_t **cr);

#endif //__RECORD_H__

