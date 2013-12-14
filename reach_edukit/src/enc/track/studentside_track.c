/*
****************************************************
Copyright (C), 1999-1999, Reach Tech. Co., Ltd.

File name:     	audio_track.c

Description:    教师跟踪

Date:     		2012-09-04

Author:	  		sunxl

version:  		V1.0

*****************************************************
*/
#include "commontrack.h"
#include "studentTcp.h"

extern int gStuRemoteFD;
extern EduKitLinkStruct_t	*gEduKit;

extern rightside_trigger_info_t	g_rightside_trigger_info;
/**
* @	表明是否需要重设动态参数标志
*/
static int g_recontrol_flag = 0;
/**
* @	tcp命令长度
*/
#define TCP_MSG_LEN		(256)

/**
* @	设备名称字符串长度的宏
*/
#define DEV_NAME_LEN	(32)

/**
* @	发送控制摄像头的命令的长度
*/
#define CTRL_CMD_LEN	(16)


//===========================教师静态参数相关的宏定义=====================

/**
* @ 教师跟踪的静态参数类型名称
*/
#define STATIC_NAME				"static"

/**
* @ 静态参数的名称
*/
#define VIDEO_WIDTH				"video_width"
#define	VIDEO_HEIGHT			"video_height"
#define PIC_WIDTH				"pic_width"
#define PIC_HEIGHT				"pic_height"




//===========================教师动态参数相关的宏定义=====================

/**
* @ 教师跟踪的动态参数类型名称
*/
#define DYNAMIC_NAME			"dynamic"



/**
* @ 动态参数的名称
*/
#define ZOOM_DISTACE			"zoom_distance"
#define X_OFFSET 				"x_offset"
#define Y_OFFSET				"y_offset"
#define TRIGGER_SUM 			"trigger_sum"
#define TRIGGER0_X0				"trigger0_x0"
#define TRIGGER0_Y0				"trigger0_y0"
#define TRIGGER0_X1				"trigger0_x1"
#define TRIGGER0_Y1				"trigger0_y1"
#define TRIGGER1_X0				"trigger1_x0"
#define TRIGGER1_Y0				"trigger1_y0"
#define TRIGGER1_X1				"trigger1_x1"
#define TRIGGER1_Y1				"trigger1_y1"
#define CONTROL_MODE			"control_mode"
#define SENS					"sens"
#define	MESSAGE					"message"
#define	MODEL_SUM				"model_sum"
#define TRACK_SEL				"track_sel"
#define MID_X					"mid_x"
#define	MID_Y					"mid_y"
#define	LIMIT_HEIGHT			"limit_height"
#define MODEL_MULTIPLE			"model_multiple"
#define	MODEL_LEVEL				"model_level"
#define TRACK_POINT_NUM			"track_point_num"
#define TRACK_POINTX			"track_pointx"
#define TRACK_POINTY			"track_pointy"
#define	START_TRACK_TIME		"start_track_time"
#define RESET_TIME				"reset_time"
#define TRACK_MODE				"track_mode"



/**
* @ 跟踪机对应的编码参数
*/
#define TRACK_STUDENTS_RIGHT_SIDE_ENCODE			"track_students_right_side_encode"

/**
* @	是否编码标志
*/
#define TRACK_STUDENTS_RIGHT_SIDE_IS_ENCODE			"track_students_right_side_is_encode"






/**
* @	和写跟踪配置文件相关
*/
track_save_file_info_t	g_stusidetrack_save_file = {0};


/**
* @	和跟踪有关的编码的一些全局参数
*/
stusidetrack_encode_info_t	g_stusidetrack_encode_info = {0};





int init_save_stusidetrack_mutex(void)
{
	pthread_mutex_init(&g_stusidetrack_save_file.save_track_m, NULL);

	return 0;
}

static int lock_save_stusidetrack_mutex(void)
{
	pthread_mutex_lock(&g_stusidetrack_save_file.save_track_m);
	return 0;
}

static int unlock_save_stusidetrack_mutex(void)
{
	pthread_mutex_unlock(&g_stusidetrack_save_file.save_track_m);
	return 0;
}

int destroy_save_stusidetrack_mutex(void)
{
	pthread_mutex_destroy(&g_stusidetrack_save_file.save_track_m);
}

/**
* @	函数名称: set_track_students_right_side_is_encode()
* @	函数功能: 设置是否进行编码的标志
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
static int set_track_students_right_side_is_encode(unsigned char *data)
{
	track_is_encode_info_t *encode_info 			= NULL;
	char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	int 					ret 						= -1;

	encode_info = (track_is_encode_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TRACK_STUDENTS_RIGHT_SIDE_FILE);

	if(encode_info->isencode > 1) {
		encode_info->isencode = 1;
	}

	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", encode_info->isencode);
	lock_save_stusidetrack_mutex();
	ret =  ConfigSetKey(config_file, TRACK_STUDENTS_RIGHT_SIDE_ENCODE, TRACK_STUDENTS_RIGHT_SIDE_IS_ENCODE, temp);
	unlock_save_stusidetrack_mutex();
	g_stusidetrack_encode_info.is_encode = encode_info->isencode;

	if(g_stusidetrack_encode_info.is_encode) {

		enc_enable_channel(gEduKit->encoderLink.encLink.link_id, STUSIDE_CHID);

	} else {

		enc_disable_channel(gEduKit->encoderLink.encLink.link_id, STUSIDE_CHID);

	}

	printf("encode_info->isencode = %d\n", encode_info->isencode);
	return;
}


/**
* @	函数名称: set_track_students_right_side_type()
* @	函数功能: 设置跟踪类型,0为自动跟踪,1为手动跟踪,并保存到配置文件中
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_track_students_right_side_type(unsigned char *data, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{
	control_type_info_t *track_info = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;

	track_info = (control_type_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TRACK_STUDENTS_RIGHT_SIDE_FILE);

	dynamic_param->control_mode = track_info->control_type;
	dynamic_param->reset_level = RE_INIT;

	printf("track_info->control_type = %d\n", track_info->control_type);

	lock_save_stusidetrack_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->control_mode);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, CONTROL_MODE, temp);
	unlock_save_stusidetrack_mutex();

}



/**
* @	函数名称: set_track_students_right_side_param()
* @	函数功能: 导入跟踪参数表
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 		  len -- 参数表长度
* @ 输出参数: 无
* @ 返回值:   无
*/
static int set_track_students_right_side_param(unsigned char *data, int len)
{

	char cmd_name[256] = {0};
	FILE *fp;
	fp = fopen("track_students_right_side_temp.ini", "w");
	fwrite(data, len, 1, fp);
	fclose(fp);
	lock_save_stusidetrack_mutex();
	sprintf(cmd_name, "mv track_students_right_side_temp.ini %s", TRACK_STUDENTS_RIGHT_SIDE_FILE);
	system(cmd_name);
	unlock_save_stusidetrack_mutex();
	system("sync");
	sleep(1);
	system("reboot -f");
}

/**
* @	函数名称: set_track_students_right_side_range()
* @	函数功能: 设置跟踪区域参数
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_track_students_right_side_range(unsigned char *data, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{
	track_range_info_t *track_info = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	char	text[FILE_NAME_LEN]			= {0};
	int 	ret 	= -1;

	int		index 	= 0;



	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TRACK_STUDENTS_RIGHT_SIDE_FILE);

	track_info = (track_range_info_t *)data;

	if(track_info->point_num > TRACK_RANGE_NUM) {
		track_info->point_num = TRACK_RANGE_NUM;
	}

	dynamic_param->track_point_num = track_info->point_num;
	dynamic_param->reset_level = RE_START;

	printf("track_info->point_num = %d\n", track_info->point_num);

	lock_save_stusidetrack_mutex();
	//存放在配置文件中
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->track_point_num);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRACK_POINT_NUM, temp);

	for(index = 0; index < track_info->point_num; index++) {
		dynamic_param->track_point[index].x = track_info->point[index].x;
		dynamic_param->track_point[index].y = track_info->point[index].y;

		printf("track_info->point[index].x = %d,index = %d\n", track_info->point[index].x, index);
		printf("track_info->point[index].y = %d,index = %d\n", track_info->point[index].y, index);

		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, TRACK_POINTX);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->track_point[index].x);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);

		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, TRACK_POINTY);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->track_point[index].y);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
	}

	unlock_save_stusidetrack_mutex();

	g_stusidetrack_encode_info.is_save_class_view = 1;
}

/**
* @	函数名称: set_track_students_right_side_sens_value()
* @	函数功能: 设置检测变化的sens值
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_track_students_right_side_sens_value(unsigned char *data, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{
	sens_info_t *sens_info = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;

	sens_info = (sens_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TRACK_STUDENTS_RIGHT_SIDE_FILE);


	printf("sens_info->sens = %d\n", sens_info->sens);

	dynamic_param->sens = sens_info->sens;
	dynamic_param->reset_level = RE_INIT;

	lock_save_stusidetrack_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->sens);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, SENS, temp);
	unlock_save_stusidetrack_mutex();
}





/**
* @	函数名称: set_track_students_right_side_mode()
* @	函数功能: 设置跟踪模式，0为何原始数据比较，1为何上一帧比较是否为变化
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_track_students_right_side_mode(unsigned char *data, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{
	stusidetrack_mode_info_t *track_mode = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;

	track_mode = (stusidetrack_mode_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TRACK_STUDENTS_RIGHT_SIDE_FILE);

	dynamic_param->track_mode = track_mode->track_mode;
	dynamic_param->reset_level = RE_INIT;

	g_rightside_trigger_info.nTriggerType = dynamic_param->track_mode;
	printf("track_info->track_mode = %d\n", track_mode->track_mode);

	lock_save_stusidetrack_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->control_mode);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRACK_MODE, temp);
	unlock_save_stusidetrack_mutex();

}

/**
* @	函数名称: set_track_students_right_side_manual_commond()
* @	函数功能: 设置手动命令参数
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
static int set_track_students_right_side_manual_commond(unsigned char *data)
{
	manual_commond_info_t *manual_commond_info 			= NULL;
	char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	int 					ret 						= -1;

	manual_commond_info = (manual_commond_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TRACK_STUDENTS_RIGHT_SIDE_FILE);

	switch(manual_commond_info->type) {
		default:
			break;
	}

	return;
}


/**
* @	函数名称: set_draw_line_type()
* @	函数功能: 设置跟踪类型,0为自动跟踪,1为手动跟踪,并保存到配置文件中
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_track_students_right_side_draw_line_type(unsigned char *data, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{
	draw_line_info_t *track_line_info = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;

	track_line_info = (draw_line_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TRACK_STUDENTS_RIGHT_SIDE_FILE);

	dynamic_param->message = track_line_info->message;
	dynamic_param->reset_level = NO_INIT;

	printf("track_line_info->message = %d\n", track_line_info->message);

	lock_save_stusidetrack_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->message);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, MESSAGE, temp);
	unlock_save_stusidetrack_mutex();
}

/**
* @	函数名称: set_track_students_right_side_trigger_sum()
* @	函数功能: 设置触发点个数,默认值为3个
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_track_students_right_side_trigger_sum(unsigned char *data, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{
	trigger_num_info_t *trigger_num_info = NULL;


	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;

	trigger_num_info = (trigger_num_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TRACK_STUDENTS_RIGHT_SIDE_FILE);

	dynamic_param->trigger_sum = trigger_num_info->trigger_num;
	dynamic_param->reset_level = RE_INIT;


	printf("trigger_num_info->trigger_num = %d\n", trigger_num_info->trigger_num);

	lock_save_stusidetrack_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->trigger_sum);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER_SUM, temp);
	unlock_save_stusidetrack_mutex();
}



/**
* @	函数名称: send_track_students_right_side_is_encode()
* @	函数功能: 发送跟踪距离参数信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_track_students_right_side_is_encode(int socket, short is_encode)
{

	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	track_is_encode_info_t	track_is_encode 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(track_is_encode_info_t);

	msg_len = htons(len);
	msg_type = MSG_SET_STUSIDETRACK_PARAM;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_TRACK_IS_ENCODE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	track_is_encode.state = 1;
	track_is_encode.isencode = is_encode;

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &track_is_encode, sizeof(track_is_encode_info_t));

	send(socket, send_buf, len, 0);
	return;

}


/**
* @	函数名称: send_track_students_right_side_type()
* @	函数功能: 发送跟踪框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_track_students_right_side_type(int socket, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{

	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	control_type_info_t	control_type 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	printf("#############dynamic_param->control_mode = %d#######\n", dynamic_param->control_mode);

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(control_type_info_t);

	msg_len = htons(len);
	msg_type = MSG_SET_STUSIDETRACK_PARAM;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_TRACK_TYPE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	control_type.state = 1;
	control_type.control_type = dynamic_param->control_mode;

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &control_type, sizeof(control_type_info_t));

	send(socket, send_buf, len, 0);
	return ;
}


/**
* @	函数名称: send_track_students_right_side_param()
* @	函数功能: 发送跟踪距离参数信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: 无
* @ 返回值:   无
*/
static int send_track_students_right_side_param(int socket)
{
	unsigned char send_buf[8192] 	= {0};
	unsigned char temp_buf[8192] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};

	int				len_next		= 0;
	int				len				= 0;
	int				file_len		= 0;

	char cmd_name[256] = {0};
	FILE *fp;
	sprintf(cmd_name, "%s", TRACK_STUDENTS_RIGHT_SIDE_FILE);
	lock_save_stusidetrack_mutex();
	fp = fopen(cmd_name, "r");

	fread(temp_buf, 8192, 1, fp);

	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);


	fclose(fp);
	unlock_save_stusidetrack_mutex();

	printf("cmd_name = %s\n", cmd_name);
	printf("file_len = %d\n", file_len);

	memset(send_buf, 0, 8192);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + file_len;

	msg_len = htons(len);
	msg_type = MSG_SET_STUSIDETRACK_PARAM;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_TRACK_PARAM;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));


	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, temp_buf, file_len);

	send(socket, send_buf, len, 0);
	return;
}



/**
* @	函数名称: send_track_students_right_side_range()
* @	函数功能: 发送跟踪框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_track_students_right_side_range(int socket, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{

	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	track_range_info_t	track_range 	= {0};
	int				len				= 0;
	int				index			= 0;
	int				len_next		= 0;

	memset(send_buf, 0, 256);


	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(track_range_info_t);

	msg_len = htons(len);
	msg_type = MSG_SET_STUSIDETRACK_PARAM;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);

	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_TRACK_RANGE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));


	track_range.point_num = dynamic_param->track_point_num;
	track_range.state = 1;

	for(index = 0; index < dynamic_param->track_point_num; index++) {
		track_range.point[index].x = dynamic_param->track_point[index].x;
		track_range.point[index].y = dynamic_param->track_point[index].y;
	}

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &track_range, sizeof(track_range_info_t));

	send(socket, send_buf, len, 0);
	return ;
}

/**
* @	函数名称: send_track_students_right_side_sens_value()
* @	函数功能: 发送检测变化系数sens值给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_track_students_right_side_sens_value(int socket, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{

	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	sens_info_t	sens_info 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(sens_info_t);

	msg_len = htons(len);
	msg_type = MSG_SET_STUSIDETRACK_PARAM;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type = GET_SENS_VALUE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	sens_info.state = 1;
	sens_info.sens = dynamic_param->sens;

	printf("sens_info.sens  = %d\n", sens_info.sens);

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &sens_info, sizeof(sens_info_t));

	send(socket, send_buf, len, 0);
	return ;
}


/**
* @	函数名称: send_track_students_right_side_mode()
* @	函数功能: 发送跟踪类型给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_track_students_right_side_mode(int socket, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{

	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	stusidetrack_mode_info_t	track_mode 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	printf("#############dynamic_param->track_mode = %d#######\n", dynamic_param->track_mode);

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(stusidetrack_mode_info_t);

	msg_len = htons(len);
	msg_type = MSG_SET_STUSIDETRACK_PARAM;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_SHIELD_RANGE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	track_mode.state = 1;
	track_mode.track_mode = dynamic_param->track_mode;

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &track_mode, sizeof(stusidetrack_mode_info_t));

	send(socket, send_buf, len, 0);
	return ;
}

/**
* @	函数名称: send_track_students_right_side_draw_line_type()
* @	函数功能: 发送跟踪框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_track_students_right_side_draw_line_type(int socket, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{

	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	draw_line_info_t	draw_line_info 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(draw_line_info_t);

	msg_len = htons(len);
	msg_type = MSG_SET_STUSIDETRACK_PARAM;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_DRAW_LINE_TYPE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	draw_line_info.state = 1;
	draw_line_info.message = dynamic_param->message;

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &draw_line_info, sizeof(draw_line_info_t));

	send(socket, send_buf, len, 0);
	return;
}




/**
* @	函数名称: send_track_students_right_side_trigger_num()
* @	函数功能: 发送跟踪框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_track_students_right_side_trigger_num(int socket, ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t *dynamic_param)
{

	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	trigger_num_info_t	trigger_num_info 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(trigger_num_info_t);

	msg_len = htons(len);
	msg_type = MSG_SET_STUSIDETRACK_PARAM;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - 3;
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_TRIGGER_NUM_TYPE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, 8);

	trigger_num_info.state = 1;
	trigger_num_info.trigger_num = dynamic_param->trigger_sum;

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &trigger_num_info, sizeof(trigger_num_info_t));

	send(socket, send_buf, len, 0);
	return;

}




/**
* @	函数名称: track_students_right_side_param()
* @	函数功能: 设置教师跟踪参数
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
int track_students_right_side_param(unsigned char *data, int socket)
{

	track_header_t *track_header = (track_header_t *)data;
	ITRACK_STUDENTS_RIGHT_SIDE_dynamic_params_t	*pdynamic_param = NULL;
	g_recontrol_flag = 0;
	pdynamic_param = &gEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams.StuSideTrackParms.dynamic_param;
	printf("track_students_right_side_param track_header = %x\n", track_header->msg_type);

	switch(track_header->msg_type) {

			//-----------------------------设置参数----------------------------------------

		case SET_TRACK_IS_ENCODE:
			set_track_students_right_side_is_encode(data + sizeof(track_header_t));
			break;

		case SET_TRACK_TYPE:
			set_track_students_right_side_type(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_TRACK_RANGE: {
			set_track_students_right_side_range(data + sizeof(track_header_t), pdynamic_param);
		}

		g_recontrol_flag = 1;
		break;

		case SET_TRACK_PARAM:
			set_track_students_right_side_param(data + sizeof(track_header_t), track_header->len - sizeof(track_header_t));
			break;

		case SET_TRIGGER_NUM_TYPE:
			set_track_students_right_side_trigger_sum(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_SENS_VALUE:
			set_track_students_right_side_sens_value(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

			//SET_TRACK_MODE
		case SET_MULTITARGET_RANGE:
			set_track_students_right_side_mode(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_MANUAL_COMMOND:
			set_track_students_right_side_manual_commond(data + sizeof(track_header_t));
			g_recontrol_flag = 1;
			break;

		case SET_DRAW_LINE_TYPE:
			set_track_students_right_side_draw_line_type(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;


			//-----------------------------上传参数-----------------------------------------
		case GET_TRACK_IS_ENCODE:
			send_track_students_right_side_is_encode(socket, g_stusidetrack_encode_info.is_encode);
			break;

		case GET_TRACK_TYPE:
			send_track_students_right_side_type(socket, pdynamic_param);
			break;

		case GET_TRACK_RANGE:
			send_track_students_right_side_range(socket, pdynamic_param);
			break;

		case GET_SENS_VALUE:
			send_track_students_right_side_sens_value(socket, pdynamic_param);
			break;

			//GET_TRACK_MODE
		case GET_SHIELD_RANGE:
			send_track_students_right_side_mode(socket, pdynamic_param);
			break;

		case GET_TRIGGER_NUM_TYPE:
			send_track_students_right_side_trigger_num(socket, pdynamic_param);
			break;

		case GET_TRACK_PARAM:
			send_track_students_right_side_param(socket);
			break;

		case GET_DRAW_LINE_TYPE:
			send_track_students_right_side_draw_line_type(socket, pdynamic_param);
			break;

		default:
			break;
	}

	if(g_recontrol_flag == 1) {
		System_linkControl(gEduKit->osd_dspAlg_Link[0].link_id,
		                   ALG_LINK_STUSIDETRACK_CMD_SET_CHANNEL_WIN_PRM,
		                   &gEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams,
		                   sizeof(AlgLink_StuSideTrackCreateParams),
		                   FALSE);

		g_recontrol_flag = 0;
	}


	if((track_header->msg_type == SET_TRACK_RANGE) && (pdynamic_param->track_mode == 0)) {
		AlgLink_StuSideTrackSaveView SaveView;
		SaveView.chId = 0;
		System_linkControl(gEduKit->osd_dspAlg_Link[0].link_id,
		                   ALG_LINK_STUSIDETRACK_CMD_SAVE_VIEW,
		                   &SaveView,
		                   sizeof(AlgLink_StuSideTrackSaveView),
		                   FALSE);
	}

}


int server_set_stusidetrack_type(short type)
{
	StuITRACK_DynamicParams *pdynamic_param = &gEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams.StuSideTrackParms.dynamic_param;

	pdynamic_param->control_mode = type;

	if(AUTO_CONTROL == pdynamic_param->control_mode) {
		//__stucall_preset_position(TRIGGER_POSITION42);
		//g_recontrol_flag = 1;
	}

	System_linkControl(gEduKit->osd_dspAlg_Link[0].link_id,
	                   ALG_LINK_STUSIDETRACK_CMD_SET_CHANNEL_WIN_PRM,
	                   &gEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams,
	                   sizeof(AlgLink_StuSideTrackCreateParams),
	                   FALSE);
	return;
}

/**
* @	函数名称: track_init()
* @	函数功能: 初始化跟踪参数
* @ 输入参数:
* @ 输出参数:
* @ 返回值:
*/
int stusidetrack_init(StuSideITRACK_Params *track_param)
{
	int 	ret = -1;
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char 	temp[FILE_NAME_LEN]			= {0};
	char	text[FILE_NAME_LEN]			= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int		index						= 0;
	int		i 							= 0;
	unsigned char data[5]				= {0};

	memset(track_param, 0, sizeof(StuSideITRACK_Params));
	track_param->size = sizeof(StuSideITRACK_Params);



	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TRACK_STUDENTS_RIGHT_SIDE_FILE);



	//视频宽度
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, VIDEO_WIDTH, temp);
	track_param->static_param.video_width = atoi(temp);

	printf("track_param->static_param.video_width = %d\n", track_param->static_param.video_width);

	if(ret != 0) {
		track_param->static_param.video_width = 704;
	}


	//视频高度
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, VIDEO_HEIGHT, temp);
	track_param->static_param.video_height = atoi(temp);

	printf("track_param->static_param.video_height = %d\n", track_param->static_param.video_height);

	if(ret != 0) {
		track_param->static_param.video_height = 576;
	}

	//跟踪处理图像的宽度
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, PIC_WIDTH, temp);
	track_param->static_param.pic_width = atoi(temp);

	printf("track_param->static_param.pic_width = %d\n", track_param->static_param.pic_width);

	if(ret != 0) {
		track_param->static_param.pic_width = 176;
	}

	//跟踪处理图像的高度
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, PIC_HEIGHT, temp);
	track_param->static_param.pic_height = atoi(temp);

	printf("track_param->static_param.pic_height = %d\n", track_param->static_param.pic_height);

	if(ret != 0) {
		track_param->static_param.pic_height = 144;
	}


	//跟踪机是否编码
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, TRACK_STUDENTS_RIGHT_SIDE_ENCODE, TRACK_STUDENTS_RIGHT_SIDE_IS_ENCODE, temp);
	g_stusidetrack_encode_info.is_encode = atoi(temp);

	printf("g_stusidetrack_encode_info.is_encode = %d\n", g_stusidetrack_encode_info.is_encode);

	if(ret != 0) {
		g_stusidetrack_encode_info.is_encode = 1;
	}

	//控制模式,是自动模式还是手动模式
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, CONTROL_MODE, temp);
	track_param->dynamic_param.control_mode = atoi(temp);

	printf("track_param->dynamic_param.control_mode = %d\n", track_param->dynamic_param.control_mode);

	if(ret != 0) {
		track_param->dynamic_param.control_mode = 0;
	}

	//跟踪区域点数
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRACK_POINT_NUM, temp);
	track_param->dynamic_param.track_point_num = atoi(temp);

	printf("track_param->dynamic_param.track_point_num = %d\n", track_param->dynamic_param.track_point_num);

	if(ret != 0) {
		track_param->dynamic_param.track_point_num = 4;
	}

	for(index = 0; index < track_param->dynamic_param.track_point_num; index++) {
		//跟踪区域点的坐标
		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, TRACK_POINTX);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);
		track_param->dynamic_param.track_point[index].x = atoi(temp);

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, TRACK_POINTY);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);
		track_param->dynamic_param.track_point[index].y = atoi(temp);

		printf("track_param->dynamic_param.track_point[%d].x = %d\n", index, track_param->dynamic_param.track_point[index].x);
		printf("track_param->dynamic_param.track_point[%d].y = %d\n", index, track_param->dynamic_param.track_point[index].y);

		if(ret != 0) {
			track_param->dynamic_param.track_point[0].x	= 0;
			track_param->dynamic_param.track_point[0].y	= 0;
			track_param->dynamic_param.track_point[1].x	= 10;
			track_param->dynamic_param.track_point[1].y	= 0;
			track_param->dynamic_param.track_point[2].x	= 0;
			track_param->dynamic_param.track_point[2].y	= 10;
			track_param->dynamic_param.track_point[3].x	= 10;
			track_param->dynamic_param.track_point[3].y	= 10;
			break;
		}
	}

	//触发点总数
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRIGGER_SUM, temp);
	track_param->dynamic_param.trigger_sum = atoi(temp);

	printf("track_param->dynamic_param.trigger_sum = %d\n", track_param->dynamic_param.trigger_sum);

	if(ret != 0) {
		track_param->dynamic_param.trigger_sum = 100;
	}

	//sens值,越小边缘值找到越多
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, SENS, temp);
	track_param->dynamic_param.sens = atoi(temp);

	printf("track_param->dynamic_param.sens = %d\n", track_param->dynamic_param.sens);

	if(ret != 0) {
		track_param->dynamic_param.sens = 56;
	}

	//跟踪取景方式，0是课前取景，1是课中取景
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRACK_MODE, temp);
	track_param->dynamic_param.track_mode = atoi(temp);

	printf("track_param->dynamic_param.track_mode = %d\n", track_param->dynamic_param.track_mode);

	if(ret != 0) {
		track_param->dynamic_param.track_mode = 0;
	}


	//画线标志
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MESSAGE, temp);
	track_param->dynamic_param.message = atoi(temp);

	printf("track_param->dynamic_param.message = %d\n", track_param->dynamic_param.message);

	if(ret != 0) {
		track_param->dynamic_param.message = 1;
	}

	return 0;

}

