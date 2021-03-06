/*
****************************************************
Copyright (C), 1999-1999, Reach Tech. Co., Ltd.

File name:     	auto_track.c

Description:    学生跟踪

Date:     		2012-12-11

Author:	  		sunxl

version:  		V1.0

*****************************************************
*/

#include "commontrack.h"
#include "studentTcp.h"

extern int gStuRemoteFD;
extern EduKitLinkStruct_t	*gEduKit;

/**
* @ 帧率,主要用做时间换算上
*/
#define FRAME_NUM		(20)

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
#define TRIGGER_NUM 			"trigger_num"
#define TRIGGER_X				"trigger_x"
#define TRIGGER_Y				"trigger_y"
#define TRIGGER_WIDTH			"trigger_width"
#define TRIGGER_HEIGHT			"trigger_height"
#define TRIGGER_POSITION		"trigger_position"
#define SHIELD_X				"shield_x"
#define SHIELD_Y				"shield_y"
#define SHIELD_WIDTH			"shield_width"
#define SHIELD_HEIGHT			"shield_height"
#define MULTITARGET_X				"multitarget_x"
#define MULTITARGET_Y				"multitarget_y"
#define MULTITARGET_WIDTH			"multitarget_width"
#define MULTITARGET_HEIGHT			"multitarget_height"
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
#define RESET_TIME				"reset_time"
#define	START_TRACK_TIME		"start_track_time"


//===========================教师摄像头控制的相关参数宏定义设置=====================

/**
* @ 教师摄像头控制参数类型名称
*/
#define TEACH_CAM_NAME			"cam_control"

/**
* @	教师摄像头控制参数
*/
#define TEACH_CAM_SPEED			"cam_speed"

/**
* @ 教师摄像头限位控制类型名称
*/
#define TEACH_CAM_LIMIT_POSITION	"limit_position"

/**
* @ 教师摄像头左下限位名称
*/
#define LIMIT_LEFT_DOWN_NAME	"left_down"

/**
* @ 教师摄像头右上限位名称
*/
#define LIMIT_RIGHT_UP_NAME		"right_up"


/**
* @ 教师摄像头预置位控制类型名称
*/
#define TEACH_CAM_PRESET_POSITION	"preset_position"

/**
* @ 教师摄像头预置位位置的上下左右值
*/
#define PRESET_POSITION_VALUE		"position_value"

/**
* @ 教师摄像头预置位焦距位置值
*/
#define PRESET_ZOOM_VALUE			"zoom_value"

/**
* @ 跟踪机对应的编码参数
*/
#define STUDENTS_TRACK_ENCODE			"students_track_encode"

/**
* @	是否编码标志
*/
#define STUDENTS_IS_ENCODE				"students_is_encode"



/**
* @	学生摄像机控制方式
*/
#define CAM_CONTROL_TYPE				"cam_control_type"


/**
* @	设置学生跟踪机是否推近景的参数名称
*/
#define IS_CONTROL_CAM					"is_control_cam"


/**
* @	设置老师机调用预制位时两条命令的间隔时间
*/
#define CAM_ZOOM_PAN_DELAY					"cam_zoom_pan_delay"




#define SUPPORT_STUSIDE_TRACK					"support_stusidetrack"

/**
* @	文件名称的长度
*/
#define FILE_NAME_LEN		(64)


/**
* @	表明是否需要重设动态参数标志
*/
static int g_recontrol_flag = 0;

/**
* @ 控制摄像头需要的参数
*/
static stucam_control_info_t	g_cam_info = {0};


/**
* @	和跟踪有关的编码的一些全局参数
*/
stutrack_encode_info_t	g_stutrack_encode_info = {0};
/**
* @	和课堂信息统计相关的一些全局参数
*/
track_class_info_t g_stuclass_info = {{0}, 0, 0};

/**
* @	和写跟踪配置文件相关
*/
static track_save_file_info_t	g_stutrack_save_file;

/**
* @	学生辅助机上报的消息
*/
rightside_trigger_info_t	g_rightside_trigger_info = {0};



//初始化写跟踪参数配置文件的信号量
int init_save_stutrack_mutex(void)
{
	pthread_mutex_init(&g_stutrack_save_file.save_track_m, NULL);

	return 0;
}

static int lock_save_track_mutex(void)
{
	pthread_mutex_lock(&g_stutrack_save_file.save_track_m);
	return 0;
}

static int unlock_save_track_mutex(void)
{
	pthread_mutex_unlock(&g_stutrack_save_file.save_track_m);
	return 0;
}

int destroy_save_stutrack_mutex(void)
{
	pthread_mutex_destroy(&g_stutrack_save_file.save_track_m);
	return 0 ;
}

static unsigned long writen(int fd, const void *vptr, size_t n)
{
	unsigned long nleft;
	unsigned long nwritten;
	const char      *ptr;

	ptr = vptr;
	nleft = n;

	while(nleft > 0) {
		if((nwritten = write(fd, ptr, nleft)) <= 0) {
			if(nwritten < 0 && errno == EINTR) {
				nwritten = 0;    /* and call write() again */
			} else {
				return(-1);    /* error */
			}
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}

	return(n);
}

/*send data to tty com*/
static int SendDataToCom(int fd, unsigned char *data, int len)
{
	unsigned long real_len = 0 ;
	//int i;

	if((real_len = writen(fd, data, len)) != len) {
		printf("SendDataToCom() write tty error\n");
		return -1;
	}

	usleep(20000);
	return (real_len);
}



/**
* @ 函数名称: write_track_static_file()
* @ 函数功能: 写跟踪的静态参数到文件中
* @ 输入参数: static_param -- 算法的静态参数
* @ 输出参数: 无
* @ 返回值:   无
*/
int stuwrite_track_static_file(stutrack_static_param_t *static_param)
{
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;

	lock_save_track_mutex();
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	//写静态参数视频宽
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", static_param->video_width);
	ret =  ConfigSetKey((void *)config_file, (void *) STATIC_NAME, (void *)VIDEO_WIDTH, (void *)temp);

	//写静态参数视频高
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", static_param->video_height);
	ret =  ConfigSetKey(config_file, STATIC_NAME, VIDEO_HEIGHT, temp);

	//写静态参数缩放后的视频宽
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", static_param->pic_width);
	ret =  ConfigSetKey(config_file, STATIC_NAME, PIC_WIDTH, temp);

	//写静态参数缩放后的视频高
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", static_param->pic_height);
	ret =  ConfigSetKey(config_file, STATIC_NAME, PIC_HEIGHT, temp);
	unlock_save_track_mutex();

	return 0;
}

/**
* @ 函数名称: set_cam_addr()
* @ 函数功能: 控制摄像头转到0x82地址位
*/
static int set_cam_addr()
{
	int size = 0;
	unsigned char data[4];
	int fd = gStuRemoteFD;

	data[0] = 0x88;
	data[1] = 0x30;
	data[2] = 0x02;
	data[3] = 0xff;
	size = 4;

	SendDataToCom(fd, data, size);
	return size;
}

/**
* @ 函数名称: jump_zoom_position()
* @ 函数功能: 控制摄像头转到预置位
* @ 输入参数: cam_position -- 预置位编号
* @ 输出参数: data -- 向摄像头发送的整个命令包
* @ 返回值:   发送的数据的长度
*/
static int jump_zoom_position(unsigned char *data, int cam_position)
{
	int size = 0;

	//	int 			fd 						= gRemoteFD;

	data[0] = g_cam_info.cam_addr;
	data[1] = 0x01;
	data[2] = 0x04;
	data[3] = 0x47;

	data[4] = g_cam_info.cam_position[cam_position].zoom[0];	//zoom的位置信息
	data[5] = g_cam_info.cam_position[cam_position].zoom[1];	//zoom的位置信息
	data[6] = g_cam_info.cam_position[cam_position].zoom[2];	//zoom的位置信息
	data[7] = g_cam_info.cam_position[cam_position].zoom[3];	//zoom的位置信息

	data[8] = 0xff;

	size = 9;
	return size;
}

/**
* @ 函数名称: jump_absolute_position()
* @ 函数功能: 控制摄像头转到预置位
* @ 输入参数: cam_position -- 预置位编号
* @ 输出参数: data -- 向摄像头发送的整个命令包
* @ 返回值:   发送的数据的长度
*/
static int jump_absolute_position(unsigned char *data, int cam_position)
{
	int size = 0;

	//	int 			fd 						= gRemoteFD;

	data[0] = g_cam_info.cam_addr;
	data[1] = 0x01;
	data[2] = 0x06;
	data[3] = 0x02;
#ifdef SUPORT_SONY_BRC_CAM
	data[4] = 0x18; 	//水平方向速度

	data[5] = 0x00; 	//垂直方向速度

	data[6] = g_cam_info.cam_position[cam_position].pan_tilt[0];	//水平方向位置信息
	data[7] = g_cam_info.cam_position[cam_position].pan_tilt[1];	//水平方向位置信息
	data[8] = g_cam_info.cam_position[cam_position].pan_tilt[2];	//水平方向位置信息
	data[9] = g_cam_info.cam_position[cam_position].pan_tilt[3];	//水平方向位置信息
	data[10] = g_cam_info.cam_position[cam_position].pan_tilt[4];	//水平方向位置信息

	data[11] = g_cam_info.cam_position[cam_position].pan_tilt[5];	//垂直方向位置信息
	data[12] = g_cam_info.cam_position[cam_position].pan_tilt[6];	//垂直方向位置信息
	data[13] = g_cam_info.cam_position[cam_position].pan_tilt[7];	//垂直方向位置信息
	data[14] = g_cam_info.cam_position[cam_position].pan_tilt[8];	//垂直方向位置信息


	data[15] = 0xff;

	size = 16;
	printf("jump_absolute_position 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
#else
	data[4] = 0x18; 	//水平方向速度
	data[5] = 0x14;		//垂直方向速度

	data[6] = g_cam_info.cam_position[cam_position].pan_tilt[0];	//水平方向位置信息
	data[7] = g_cam_info.cam_position[cam_position].pan_tilt[1];	//水平方向位置信息
	data[8] = g_cam_info.cam_position[cam_position].pan_tilt[2];	//水平方向位置信息
	data[9] = g_cam_info.cam_position[cam_position].pan_tilt[3];	//水平方向位置信息
	data[10] = g_cam_info.cam_position[cam_position].pan_tilt[4];	//垂直方向位置信息
	data[11] = g_cam_info.cam_position[cam_position].pan_tilt[5];	//垂直方向位置信息
	data[12] = g_cam_info.cam_position[cam_position].pan_tilt[6];	//垂直方向位置信息
	data[13] = g_cam_info.cam_position[cam_position].pan_tilt[7];	//垂直方向位置信息

	data[14] = 0xff;

	size = 15;
#endif
	return size;
}


/**
* @	函数名称: set_track_range()
* @	函数功能: 设置跟踪区域参数
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_track_range(unsigned char *data, StuITRACK_DynamicParams *dynamic_param)
{
	track_range_info_t *track_info = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	char	text[FILE_NAME_LEN]			= {0};
	int 	ret 	= -1;

	int		index 	= 0;



	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	track_info = (track_range_info_t *)data;

	if(track_info->point_num > TRACK_AREA_POINT_MAX) {
		track_info->point_num = TRACK_AREA_POINT_MAX;
	}

	dynamic_param->track_point_num = track_info->point_num;
	dynamic_param->reset_level = RE_START;
#ifdef ENC_1200_DEBUG
	PRINTF("track_info->point_num = %d\n", track_info->point_num);
#else
	printf("track_info->point_num = %d\n", track_info->point_num);
#endif

	lock_save_track_mutex();
	//存放在配置文件中
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->track_point_num);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRACK_POINT_NUM, temp);

	for(index = 0; index < track_info->point_num; index++) {
		dynamic_param->track_point[index].x = track_info->point[index].x;
		dynamic_param->track_point[index].y = track_info->point[index].y;
#ifdef ENC_1200_DEBUG
		PRINTF("track_info->point[index].x = %d,index = %d\n", track_info->point[index].x, index);
		PRINTF("track_info->point[index].y = %d,index = %d\n", track_info->point[index].y, index);
#else
		printf("track_info->point[index].x = %d,index = %d\n", dynamic_param->track_point[index].x, index);
		printf("track_info->point[index].y = %d,index = %d\n", dynamic_param->track_point[index].y, index);
#endif
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

	unlock_save_track_mutex();

	return 0;
}

/**
* @	函数名称: set_trigger_range()
* @	函数功能: 设置触发区域参数
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_trigger_range(unsigned char *data, StuITRACK_DynamicParams *dynamic_param)
{
	stutrigger_range_info_t *trigger_info = NULL;
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	char	text[FILE_NAME_LEN]			= {0};
	int 	ret 						= -1;
	int		index						= 0;


	trigger_info = (stutrigger_range_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	dynamic_param->reset_level = RE_INIT;

	lock_save_track_mutex();

#if 0
	dynamic_param->trigger_num = trigger_info->point_num;
	sprintf(temp, "%d", dynamic_param->trigger_num);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER_NUM, temp);
#endif

	for(index = 0; index < STUDENTS_TRIGGER_NUM; index++) {
		dynamic_param->trigger_info[index].x = trigger_info->rectangle[index].x;
		dynamic_param->trigger_info[index].y = trigger_info->rectangle[index].y;
		dynamic_param->trigger_info[index].width = trigger_info->rectangle[index].width;
		dynamic_param->trigger_info[index].height = trigger_info->rectangle[index].height;


		printf("trigger_info->point[%d].x = %d\n", index, trigger_info->rectangle[index].x);
		printf("trigger_info->point[%d].y = %d\n", index, trigger_info->rectangle[index].y);
		printf("trigger_info->point[%d].width = %d\n", index, trigger_info->rectangle[index].width);
		printf("trigger_info->point[%d].height = %d\n", index, trigger_info->rectangle[index].height);

		//存放在配置文件中
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, TRIGGER_X);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->trigger_info[index].x);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);

		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, TRIGGER_Y);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->trigger_info[index].y);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);

		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, TRIGGER_WIDTH);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->trigger_info[index].width);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);

		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, TRIGGER_HEIGHT);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->trigger_info[index].height);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);
	}

	unlock_save_track_mutex();

	return 0;
}



/**
* @	函数名称: set_track_type()
* @	函数功能: 设置跟踪类型,0为自动跟踪,1为手动跟踪,并保存到配置文件中
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_track_type(unsigned char *data, StuITRACK_DynamicParams *dynamic_param)
{
	control_type_info_t *track_info = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;

	track_info = (control_type_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	dynamic_param->control_mode = track_info->control_type;
	dynamic_param->reset_level = NO_INIT;

	printf("track_info->control_type = %d\n", track_info->control_type);

	__stucall_preset_position(TRIGGER_POSITION42);
	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->control_mode);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, CONTROL_MODE, temp);
	unlock_save_track_mutex();

	return 0;
}

/**
* @	函数名称: set_draw_line_type()
* @	函数功能: 设置跟踪类型,0为自动跟踪,1为手动跟踪,并保存到配置文件中
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_draw_line_type(unsigned char *data, StuITRACK_DynamicParams *dynamic_param)
{
	draw_line_info_t *track_line_info = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;

	track_line_info = (draw_line_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	dynamic_param->message = track_line_info->message;
	dynamic_param->reset_level = NO_INIT;
#ifdef ENC_1200_DEBUG
	PRINTF("track_line_info->message = %d\n", track_line_info->message);
#else
	printf("track_line_info->message = %d\n", track_line_info->message);
#endif
	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->message);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, MESSAGE, temp);
	unlock_save_track_mutex();

	return 0;
}



/**
* @	函数名称: get_cam_position()
* @	函数功能: 获取摄像头上下左右位置
* @	输入参数: 无
* @ 输出参数: data,返回获取的摄像头的位置信息
* @ 返回值:   无
*/
static int get_cam_position(unsigned char 	*data)
{
	int 			fd 				= gStuRemoteFD;
	unsigned char 	cmd_data[20]	= {0};
	//	int 			ret 			= -1;
	int 			size 			= 0;
	//g_cam_info.cam_addr = 0x81;
	cmd_data[0] = g_cam_info.cam_addr;
	cmd_data[1] = 0x09;
	cmd_data[2] = 0x06;
	cmd_data[3] = 0x12;
	cmd_data[4] = 0xFF;
	size = 5;
	SendDataToCom(fd, cmd_data, size);

	return 0;
}

/**
* @	函数名称: get_cam_zoom()
* @	函数功能: 获取镜头现在的焦距位置
* @	输入参数: 无
* @ 输出参数: data,返回获取的摄像头的焦距信息
* @ 返回值:   无
*/
static int get_cam_zoom(unsigned char 	*data)
{
	int 			fd 				= gStuRemoteFD;
	unsigned char 	cmd_data[20]	= {0};
	//	int 			ret 			= -1;
	int 			size 			= 0;
	//g_cam_info.cam_addr = 0x81;
	cmd_data[0] = g_cam_info.cam_addr;
	cmd_data[1] = 0x09;
	cmd_data[2] = 0x04;
	cmd_data[3] = 0x47;
	cmd_data[4] = 0xFF;
	size = 5;
	SendDataToCom(fd, cmd_data, size);

	return 0;
}


/**
* @	函数名称: set_preset_position()
* @	函数功能: 设置预置位
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
static int set_preset_position(unsigned char *data)
{
	preset_position_info_t 	*preset_position 			= NULL;
	int 					fd 							= gStuRemoteFD;
	//char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	//	char					param_name[FILE_NAME_LEN] 	= {0};
	//	char					text[FILE_NAME_LEN]			= {0};
	unsigned char 			recv_data[32]				= {0};
	//unsigned char 			temp_data[32]				= {0};
	//int 					ret 						= -1;
	//	int						index						= 0;

#ifdef SUPORT_SONY_BRC_CAM
	unsigned char	cmd_data[20]	= {0};
	int 			size			= 0;
#endif
	preset_position = (preset_position_info_t *)data;


	printf("preset_position->preset_position = %d\n", preset_position->preset_position);


	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	if(preset_position->preset_position > (PRESET_NUM_MAX - 1)) {

		printf("preset value must less than 5\n");

		return 0;
	}

#ifdef SUPORT_SONY_BRC_CAM

	if(preset_position->preset_position > 15) {
		preset_position->preset_position = 0;
	}

	cmd_data[0] = g_cam_info.cam_addr;
	cmd_data[1] = 0x01;
	cmd_data[2] = 0x04;
	cmd_data[3] = 0x3F;
	cmd_data[4] = 0x01;

	cmd_data[5] = preset_position->preset_position;
	cmd_data[6] = 0xff;
	size = 7;
	SendDataToCom(fd, cmd_data, size);
#else
	g_stutrack_save_file.set_cmd = SET_PRESET_POSITION;
	g_stutrack_save_file.cmd_param = preset_position->preset_position;

	//设置预置位的坐标值,由单独线程去读取数据并写配置文件
	get_cam_position(recv_data);
	usleep(50000);
	//设置预置位zoom值,由单独线程去读取数据并写配置文件
	get_cam_zoom(&recv_data);
#endif
	return;
}

int stusave_position_zoom(unsigned char *data)
{
	char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	char					param_name[FILE_NAME_LEN] 	= {0};
	char					text[FILE_NAME_LEN]			= {0};
	//	unsigned char 			recv_data[32]				= {0};
	//	unsigned char 			temp_data[32]				= {0};
	int 					ret 						= -1;
	//	int						index						= 0;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);
#ifdef SUPORT_SONY_BRC_CAM

	if((0xa0 == data[0]) && (0x50 == data[1]) && (0xff == data[11])) {
		printf("000g_track_save_file.set_cmd = %d\n", g_track_save_file.set_cmd);

		if(SET_PRESET_POSITION == g_track_save_file.set_cmd) {
			memset(text, 0, FILE_NAME_LEN);
			memset(param_name, 0, FILE_NAME_LEN);
			strcpy(text, PRESET_POSITION_VALUE);
			sprintf(param_name, "%s%d", text, g_track_save_file.cmd_param);
			memset(temp, 0, FILE_NAME_LEN);
			sprintf(temp, "%x%x%x%x%x%x%x%x%x", data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10]);
			lock_save_track_mutex();
			ret =  ConfigSetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);
			unlock_save_track_mutex();
			g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[0] = data[2];
			g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[1] = data[3];
			g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[2] = data[4];
			g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[3] = data[5];
			g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[4] = data[6];
			g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[5] = data[7];
			g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[6] = data[8];
			g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[7] = data[9];
			g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[8] = data[10];
		}
	} else if((0xa0 == data[0]) && (0x50 == data[1]) && (0xff == data[6])) {
		printf("111g_track_save_file.set_cmd = %d\n", g_track_save_file.set_cmd);

		if(SET_PRESET_POSITION == g_track_save_file.set_cmd) {
			memset(text, 0, FILE_NAME_LEN);
			memset(param_name, 0, FILE_NAME_LEN);
			strcpy(text, PRESET_ZOOM_VALUE);
			sprintf(param_name, "%s%d", text, g_track_save_file.cmd_param);
			memset(temp, 0, FILE_NAME_LEN);
			sprintf(temp, "%x%x%x%x", data[2], data[3], data[4], data[5]);
			lock_save_track_mutex();
			ret =  ConfigSetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);
			unlock_save_track_mutex();
			g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[0] = data[2];
			g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[1] = data[3];
			g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[2] = data[4];
			g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[3] = data[5];
		}
	}

#else

	if((0xa0 == data[0]) && (0x50 == data[1]) && (0xff == data[10])) {
		printf("000g_stutrack_save_file.set_cmd = %d\n", g_stutrack_save_file.set_cmd);

		if(SET_PRESET_POSITION == g_stutrack_save_file.set_cmd) {
			memset(text, 0, FILE_NAME_LEN);
			memset(param_name, 0, FILE_NAME_LEN);
			strcpy(text, PRESET_POSITION_VALUE);
			sprintf(param_name, "%s%d", text, g_stutrack_save_file.cmd_param);
			memset(temp, 0, FILE_NAME_LEN);
			sprintf(temp, "%x%x%x%x%x%x%x%x", data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);
			lock_save_track_mutex();
			ret =  ConfigSetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);
			unlock_save_track_mutex();
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].pan_tilt[0] = data[2];
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].pan_tilt[1] = data[3];
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].pan_tilt[2] = data[4];
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].pan_tilt[3] = data[5];
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].pan_tilt[4] = data[6];
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].pan_tilt[5] = data[7];
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].pan_tilt[6] = data[8];
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].pan_tilt[7] = data[9];
		}
	} else if((0xa0 == data[0]) && (0x50 == data[1]) && (0xff == data[6])) {
		printf("111g_stutrack_save_file.set_cmd = %d\n", g_stutrack_save_file.set_cmd);

		if(SET_PRESET_POSITION == g_stutrack_save_file.set_cmd) {
			memset(text, 0, FILE_NAME_LEN);
			memset(param_name, 0, FILE_NAME_LEN);
			strcpy(text, PRESET_ZOOM_VALUE);
			sprintf(param_name, "%s%d", text, g_stutrack_save_file.cmd_param);
			memset(temp, 0, FILE_NAME_LEN);
			sprintf(temp, "%x%x%x%x", data[2], data[3], data[4], data[5]);
			lock_save_track_mutex();
			ret =  ConfigSetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);
			unlock_save_track_mutex();
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].zoom[0] = data[2];
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].zoom[1] = data[3];
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].zoom[2] = data[4];
			g_cam_info.cam_position[g_stutrack_save_file.cmd_param].zoom[3] = data[5];
		}
	}

#endif
}


/**
* @	函数名称: __call_preset_position()
* @	函数功能: 调用预置位
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
int __stucall_preset_position(short position)
{
	int 					fd 							= gStuRemoteFD;
	unsigned char 			cmd_code[CTRL_CMD_LEN] 		= {0};
	int						size						= 0;

#ifdef SUPORT_SONY_BRC_CAM

	if(position > 15) {
		position = 0;
	}

	unsigned char	cmd_data[20]	= {0};

	cmd_data[0] = g_cam_info.cam_addr;
	cmd_data[1] = 0x01;
	cmd_data[2] = 0x04;
	cmd_data[3] = 0x3F;
	cmd_data[4] = 0x02;

	cmd_data[5] = position;
	cmd_data[6] = 0xff;
	size = 7;
	SendDataToCom(fd, cmd_data, size);

#else
	size = jump_zoom_position(&cmd_code, position);

	if(size > 0) {
		SendDataToCom(fd, cmd_code, size);
	}

	usleep(50000);

	size = jump_absolute_position(&cmd_code, position);

	if(size > 0) {
		SendDataToCom(fd, cmd_code, size);
	}

#endif
	return;
}

/**
* @	函数名称: auto_call_preset_position_zoom()
* @	函数功能: 调用预置位对应的焦距位置
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
int auto_call_preset_position_zoom(short position)
{
	int 					fd 							= gStuRemoteFD;
	unsigned char 			cmd_code[CTRL_CMD_LEN] 		= {0};
	int						size						= 0;


	size = jump_zoom_position(&cmd_code, position);

	if(size > 0) {
		SendDataToCom(fd, cmd_code, size);
	}

	return;
}

/**
* @	函数名称: __call_preset_position()
* @	函数功能: 调用预置位
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
int auto_call_preset_position_absolute(short position)
{
	int 					fd 							= gStuRemoteFD;
	unsigned char 			cmd_code[CTRL_CMD_LEN] 		= {0};
	int						size						= 0;


	size = jump_absolute_position(&cmd_code, position);

	if(size > 0) {
		SendDataToCom(fd, cmd_code, size);
	}

	return;
}



/**
* @	函数名称: call_preset_position()
* @	函数功能: 调用预置位
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
static int stucall_preset_position(unsigned char *data)
{
	preset_position_info_t 	*preset_position 			= NULL;
	int 					fd 							= gStuRemoteFD;
	//	unsigned char 			cmd_code[CTRL_CMD_LEN] 	= {0};
	//	int						size						= 0;

	preset_position = (preset_position_info_t *)data;

#ifdef ENC_1200_DEBUG
	PRINTF("call_preset_position preset_position = %d\n", preset_position->preset_position);
#else
	printf("call_preset_position preset_position = %d\n", preset_position->preset_position);
#endif

	if(preset_position->preset_position > (PRESET_NUM_MAX - 1)) {
#ifdef ENC_1200_DEBUG
		PRINTF("call preset value must less than 5\n");
#else
		printf("call preset value must less than 5\n");
#endif
		return 0;
	}

	__stucall_preset_position(preset_position->preset_position);
	return 0 ;
}

/**
* @	函数名称: set_track_is_encode()
* @	函数功能: 设置是否进行编码的标志
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
static int set_track_is_encode(unsigned char *data)
{
	track_is_encode_info_t *encode_info 			= NULL;
	char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	int 					ret 						= -1;

	encode_info = (track_is_encode_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	if(encode_info->isencode > 1) {
		encode_info->isencode = 1;
	}

	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", encode_info->isencode);
	ret =  ConfigSetKey(config_file, STUDENTS_TRACK_ENCODE, STUDENTS_IS_ENCODE, temp);
	unlock_save_track_mutex();

	g_stutrack_encode_info.is_encode = encode_info->isencode;

	if(g_stutrack_encode_info.is_encode == 1) {
		enc_enable_channel(gEduKit->encoderLink.encLink.link_id, STU_CHID);
	} else {
		enc_disable_channel(gEduKit->encoderLink.encLink.link_id, STU_CHID);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("encode_info->isencode = %d\n", encode_info->isencode);
#else
	printf("encode_info->isencode = %d\n", encode_info->isencode);
#endif
	return 0;
}

/**
* @	函数名称: set_track_param()
* @	函数功能: 导入跟踪参数表
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 		  len -- 参数表长度
* @ 输出参数: 无
* @ 返回值:   无
*/
static int set_track_param(unsigned char *data, int len)
{
	char cmd_name[256] = {0};

	FILE *fp;
	fp = fopen("track_temp.ini", "w");
	fwrite(data, len, 1, fp);
	fclose(fp);
	lock_save_track_mutex();
	sprintf(cmd_name, "mv track_temp.ini %s", STUDENTS_TRACK_FILE);
	system(cmd_name);
	unlock_save_track_mutex();

	system("sync");
	sleep(1);
	system("reboot -f");

	return 0;
}

/**
* @	函数名称: set_reset_time()
* @	函数功能: 设置复位时间,即目标丢失后多久开始触发
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_reset_time(unsigned char *data, StuITRACK_DynamicParams *dynamic_param)
{
	reset_time_info_t *reset_time_info = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;

	reset_time_info = (reset_time_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

#ifdef ENC_1200_DEBUG
	PRINTF("reset_time_info->reset_time = %d\n", reset_time_info->reset_time);
#else
	printf("reset_time_info->reset_time = %d\n", reset_time_info->reset_time);
#endif

	dynamic_param->reset_time = reset_time_info->reset_time * FRAME_NUM;
	dynamic_param->reset_level = RE_INIT;

	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->reset_time);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, RESET_TIME, temp);
	unlock_save_track_mutex();

	return 0;
}

/**
* @	函数名称: set_sens_value()
* @	函数功能: 设置检测变化的sens值
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_sens_value(unsigned char *data, StuITRACK_DynamicParams *dynamic_param)
{
	sens_info_t *sens_info = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;

	sens_info = (sens_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

#ifdef ENC_1200_DEBUG
	PRINTF("reset_time_info->reset_time = %d\n", sens_info->sens);
#else
	printf("reset_time_info->reset_time = %d\n", sens_info->sens);
#endif

	dynamic_param->sens = sens_info->sens;
	dynamic_param->reset_level = RE_INIT;

	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->sens);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, SENS, temp);
	unlock_save_track_mutex();

	return 0;
}

/**
* @	函数名称: set_shield_range()
* @	函数功能: 设置屏蔽区域参数
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_shield_range(unsigned char *data, StuITRACK_DynamicParams *dynamic_param)
{
	shield_range_info_t *shield_info = NULL;
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	char	text[FILE_NAME_LEN]			= {0};
	int 	ret 						= -1;
	int		index						= 0;


	shield_info = (shield_range_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	dynamic_param->reset_level = RE_INIT;

	lock_save_track_mutex();

	for(index = 0; index < STUDENTS_SHIELD_NUM; index++) {
		dynamic_param->shield_info[index].x = shield_info->rectangle[index].x;
		dynamic_param->shield_info[index].y = shield_info->rectangle[index].y;
		dynamic_param->shield_info[index].width = shield_info->rectangle[index].width;
		dynamic_param->shield_info[index].height = shield_info->rectangle[index].height;

		//存放在配置文件中
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, SHIELD_X);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->shield_info[index].x);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, SHIELD_Y);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->shield_info[index].y);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, SHIELD_WIDTH);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->shield_info[index].width);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, SHIELD_HEIGHT);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->shield_info[index].height);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);
	}

	unlock_save_track_mutex();
}

/**
* @	函数名称: set_manual_commond()
* @	函数功能: 设置手动命令参数
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
static int set_manual_commond(unsigned char *data)
{
	manual_commond_info_t *manual_commond_info 			= NULL;
	char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	int 					ret 						= -1;

	manual_commond_info = (manual_commond_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	switch(manual_commond_info->type) {
		case SET_IS_CONTROL_CAM:

			if(manual_commond_info->value > 1) {
				manual_commond_info->value = 1;
			}

			lock_save_track_mutex();
			memset(temp, 0, FILE_NAME_LEN);
			sprintf(temp, "%d", manual_commond_info->value);
			ret =  ConfigSetKey(config_file, CAM_CONTROL_TYPE, IS_CONTROL_CAM, temp);
			unlock_save_track_mutex();
			printf("manual_commond_info->value = %d\n", manual_commond_info->value);

			g_stutrack_encode_info.is_control_cam = manual_commond_info->value;
			break;

		case SET_ZOOM_PAN_DELAY:
			if(manual_commond_info->value < 30) {
				manual_commond_info->value = 30;
			}

			lock_save_track_mutex();
			memset(temp, 0, FILE_NAME_LEN);
			sprintf(temp, "%d", manual_commond_info->value);
			ret =  ConfigSetKey(config_file, CAM_CONTROL_TYPE, CAM_ZOOM_PAN_DELAY, temp);
			unlock_save_track_mutex();
			printf("manual_commond_info->value = %d\n", manual_commond_info->value);
			g_stutrack_encode_info.zoom_pan_delay = manual_commond_info->value;
			break;

		default:
			break;
	}

	return 0;
}

/**
* @	函数名称: set_multitarget_range()
* @	函数功能: 设置多目标检测区域参数
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int set_multitarget_range(unsigned char *data, StuITRACK_DynamicParams *dynamic_param)
{
	multitarget_range_info_t *multitarget_info = NULL;
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	char	text[FILE_NAME_LEN]			= {0};
	int 	ret 						= -1;
	int		index						= 0;


	multitarget_info = (multitarget_range_info_t *)data;
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	dynamic_param->reset_level = RE_INIT;

	lock_save_track_mutex();

	for(index = 0; index < STUDENTS_MULTITARGET_NUM; index++) {
		dynamic_param->multitarget_info[index].x = multitarget_info->rectangle[index].x;
		dynamic_param->multitarget_info[index].y = multitarget_info->rectangle[index].y;
		dynamic_param->multitarget_info[index].width = multitarget_info->rectangle[index].width;
		dynamic_param->multitarget_info[index].height = multitarget_info->rectangle[index].height;

		//存放在配置文件中
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, MULTITARGET_X);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->multitarget_info[index].x);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, MULTITARGET_Y);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->multitarget_info[index].y);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, MULTITARGET_WIDTH);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->multitarget_info[index].width);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, MULTITARGET_HEIGHT);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->multitarget_info[index].height);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		usleep(5000);
	}

	unlock_save_track_mutex();

	return 0;
}

/**
* @	函数名称: send_track_range()
* @	函数功能: 发送跟踪框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_track_range(int socket, StuITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};

	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	track_range_info_t	*track_range 	= NULL;
	int				len				= 0;
	int				index			= 0;

	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(track_range_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA1;

	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRACK_RANGE;

	track_range = (track_range_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	track_range->point_num = dynamic_param->track_point_num;
	track_range->state = 1;

	for(index = 0; index < dynamic_param->track_point_num; index++) {
		track_range->point[index].x = dynamic_param->track_point[index].x;
		track_range->point[index].y = dynamic_param->track_point[index].y;
	}

	send(socket, send_buf, len, 0);
	return ;
#else
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
	msg_type = 0xA1;

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
	printf("send_track_range len = %d\n", len);

	send(socket, send_buf, len, 0);
	return 0;

#endif
}


/**
* @	函数名称: send_trigger_range()
* @	函数功能: 发送触发框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_trigger_range(int socket, StuITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};

	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	stutrigger_range_info_t	*trigger_range 	= NULL;
	int				len				= 0;

	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(stutrigger_range_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA1;
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRIGGER_RANGE;
	trigger_range = (stutrigger_range_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	trigger_range->point_num = 2;
	trigger_range->state = 1;

	trigger_range->point[0].x = dynamic_param->trigger_x0;
	trigger_range->point[0].y = dynamic_param->trigger_y0;
	trigger_range->point[1].x = dynamic_param->trigger_x1;
	trigger_range->point[1].y = dynamic_param->trigger_y1;
	send(socket, send_buf, len, 0);

	return ;
#else
	unsigned char send_buf[2048] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	stutrigger_range_info_t	trigger_range 	= {0};

	int				len_next		= 0;
	int				len				= 0;
	int				index			= 0;

	memset(send_buf, 0, 2048);
	len = sizeof(unsigned short) + sizeof(unsigned char) +
	      sizeof(track_header_t) + sizeof(stutrigger_range_info_t);

	msg_len = htons(len);
	msg_type = 0xA1;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_TRIGGER_RANGE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	trigger_range.point_num = STUDENTS_TRIGGER_NUM;
	trigger_range.state = 1;

	for(index = 0; index < STUDENTS_TRIGGER_NUM; index++) {
		trigger_range.rectangle[index].x = dynamic_param->trigger_info[index].x;
		trigger_range.rectangle[index].y = dynamic_param->trigger_info[index].y;
		trigger_range.rectangle[index].width = dynamic_param->trigger_info[index].width;
		trigger_range.rectangle[index].height = dynamic_param->trigger_info[index].height;
	}

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &trigger_range, sizeof(stutrigger_range_info_t));

	send(socket, send_buf, len, 0);
	return 0;
#endif
}


/**
* @	函数名称: send_track_type()
* @	函数功能: 发送跟踪框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_track_type(int socket, StuITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};

	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	control_type_info_t	*control_type 	= NULL;
	int				len				= 0;
	int				index			= 0;


	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(control_type_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA1;


	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRACK_TYPE;

	control_type = (control_type_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	control_type->state = 1;

	control_type->control_type = dynamic_param->control_mode;

	send(socket, send_buf, len, 0);
	return ;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	control_type_info_t	control_type 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(control_type_info_t);

	msg_len = htons(len);
	msg_type = 0xA1;

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
	return 0;
#endif
}

/**
* @	函数名称: send_draw_line_type()
* @	函数功能: 发送跟踪框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_draw_line_type(int socket, StuITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};

	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	draw_line_info_t	*draw_line_info = NULL;
	int				len				= 0;
	int				index			= 0;


	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(draw_line_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA1;

	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_DRAW_LINE_TYPE;

	draw_line_info = (draw_line_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	draw_line_info->state = 1;

	draw_line_info->message = dynamic_param->message;

	send(socket, send_buf, len, 0);
	return ;
#else
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
	msg_type = 0xA1;

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
	return 0;
#endif
}

/**
* @	函数名称: send_track_is_encode()
* @	函数功能: 发送跟踪距离参数信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_track_is_encode(int socket, short is_encode)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};

	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	track_is_encode_info_t *track_is_encode = NULL;
	int				len				= 0;
	int				index			= 0;


	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(track_is_encode_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA1;

	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRACK_IS_ENCODE;

	track_is_encode = (track_is_encode_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	track_is_encode->state = 1;

	track_is_encode->isencode = is_encode;

	send(socket, send_buf, len, 0);
	return ;
#else
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
	msg_type = 0xA1;

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

	printf("track_is_encode.isencode = %d\n", track_is_encode.isencode);

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &track_is_encode, sizeof(track_is_encode_info_t));

	send(socket, send_buf, len, 0);
	return 0;

#endif
}

/**
* @	函数名称: send_track_param()
* @	函数功能: 发送跟踪距离参数信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: 无
* @ 返回值:   无
*/
static int send_track_param(int socket)
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
	sprintf(cmd_name, "%s", STUDENTS_TRACK_FILE);
	lock_save_track_mutex();
	fp = fopen(cmd_name, "r");

	if(fp == NULL) {

		printf("open error %s\n", STUDENTS_TRACK_FILE);
		return 0;
	}

	fread(temp_buf, 8192, 1, fp);

	fseek(fp, 0, SEEK_END);
	file_len = ftell(fp);


	fclose(fp);
	unlock_save_track_mutex();

	printf("cmd_name = %s\n", cmd_name);
	printf("file_len = %d\n", file_len);

	memset(send_buf, 0, 8192);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + file_len;

	msg_len = htons(len);
	msg_type = 0xA1;

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
	return 0;
}

/**
* @	函数名称: send_reset_time()
* @	函数功能: 发送跟踪框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_reset_time(int socket, StuITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};

	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	reset_time_info_t	*reset_time_info 	= NULL;
	int				len				= 0;
	int				index			= 0;

	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(reset_time_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA1;


	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_RESET_TIME;

	reset_time_info = (reset_time_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	reset_time_info->state = 1;

	reset_time_info->reset_time = dynamic_param->reset_time / FRAME_NUM;

	send(socket, send_buf, len, 0);
	return ;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	reset_time_info_t	reset_time_info 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(reset_time_info_t);

	msg_len = htons(len);
	msg_type = 0xA1;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_RESET_TIME;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	reset_time_info.state = 1;
	reset_time_info.reset_time = dynamic_param->reset_time / FRAME_NUM;

	printf("reset_time_info.reset_time  = %d\n", reset_time_info.reset_time);

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &reset_time_info, sizeof(reset_time_info_t));

	send(socket, send_buf, len, 0);
	return 0;
#endif
}

/**
* @	函数名称: send_sens_value()
* @	函数功能: 发送检测变化系数sens值给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_sens_value(int socket, StuITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};

	msg_header_t		*msg_header 		= NULL;
	track_header_t		*track_header 		= NULL;
	sens_info_t	*sens_info 	= NULL;
	int				len				= 0;
	int				index			= 0;

	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(sens_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA1;


	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_SENS_VALUE;

	sens_info = (sens_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	sens_info->state = 1;

	sens_info->sens = dynamic_param->sens;

	send(socket, send_buf, len, 0);
	return ;
#else
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
	msg_type = 0xA1;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_SENS_VALUE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	sens_info.state = 1;
	sens_info.sens = dynamic_param->sens;

	printf("sens_info.sens  = %d\n", sens_info.sens);

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &sens_info, sizeof(sens_info_t));

	send(socket, send_buf, len, 0);
	return 0;
#endif
}

/**
* @	函数名称: send_shield_range()
* @	函数功能: 发送触发框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_shield_range(int socket, StuITRACK_DynamicParams *dynamic_param)
{

	unsigned char send_buf[2048] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	shield_range_info_t	shield_range 	= {0};

	int				len_next		= 0;
	int				len				= 0;
	int				index			= 0;

	memset(send_buf, 0, 2048);
	len = sizeof(unsigned short) + sizeof(unsigned char) +
	      sizeof(track_header_t) + sizeof(shield_range_info_t);

	msg_len = htons(len);
	msg_type = 0xA1;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_SHIELD_RANGE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	shield_range.point_num = STUDENTS_SHIELD_NUM;
	shield_range.state = 1;

	for(index = 0; index < STUDENTS_SHIELD_NUM; index++) {
		shield_range.rectangle[index].x = dynamic_param->shield_info[index].x;
		shield_range.rectangle[index].y = dynamic_param->shield_info[index].y;
		shield_range.rectangle[index].width = dynamic_param->shield_info[index].width;
		shield_range.rectangle[index].height = dynamic_param->shield_info[index].height;
	}

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &shield_range, sizeof(shield_range_info_t));

	send(socket, send_buf, len, 0);
	return 0;

}

/**
* @	函数名称: send_multitarget_range()
* @	函数功能: 发送多目标上台检测框信息给encodemanage
* @	输入参数: socket -- 和encodemanage连接的socket
* @ 输出参数: dynamic_param -- 教师跟踪的动态参数
* @ 返回值:   无
*/
static int send_multitarget_range(int socket, StuITRACK_DynamicParams *dynamic_param)
{

	unsigned char send_buf[2048] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	multitarget_range_info_t	multitarget_range 	= {0};

	int				len_next		= 0;
	int				len				= 0;
	int				index			= 0;

	memset(send_buf, 0, 2048);
	len = sizeof(unsigned short) + sizeof(unsigned char) +
	      sizeof(track_header_t) + sizeof(multitarget_range_info_t);

	msg_len = htons(len);
	msg_type = 0xA1;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_MULTITARGET_RANGE;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	multitarget_range.point_num = STUDENTS_MULTITARGET_NUM;
	multitarget_range.state = 1;

	for(index = 0; index < STUDENTS_MULTITARGET_NUM; index++) {
		multitarget_range.rectangle[index].x = dynamic_param->multitarget_info[index].x;
		multitarget_range.rectangle[index].y = dynamic_param->multitarget_info[index].y;
		multitarget_range.rectangle[index].width = dynamic_param->multitarget_info[index].width;
		multitarget_range.rectangle[index].height = dynamic_param->multitarget_info[index].height;
	}

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &multitarget_range, sizeof(multitarget_range_info_t));

	send(socket, send_buf, len, 0);
	return 0;

}


/**
* @	函数名称: set_students_track_param()
* @	函数功能: 设置跟踪参数
* @	输入参数: data -- 从encodemanage接收到的设置参数数据
* @ 输出参数: 无
* @ 返回值:   无
*/
int set_students_track_param(unsigned char *data, int socket)
{

	track_header_t *track_header = (track_header_t *)data;

	StuITRACK_DynamicParams	*pdynamic_param = NULL;
	pdynamic_param = &gEduKit->osd_dspAlg_Link[0].create_params.stutrackCreateParams.StuTrackParms.dynamic_param;



	printf("set_students_track_param = %x\n", track_header->msg_type);
	g_recontrol_flag = 0;

	switch(track_header->msg_type) {
		case SET_TRACK_RANGE:
			set_track_range(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_TRIGGER_RANGE:
			set_trigger_range(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_TRACK_TYPE:
			set_track_type(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_DRAW_LINE_TYPE:
			set_draw_line_type(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_PRESET_POSITION_TYPE:
			set_preset_position(data + sizeof(track_header_t));
			g_recontrol_flag = 0;
			break;

		case CALL_PRESET_POSITION_TYPE:
			stucall_preset_position(data + sizeof(track_header_t));
			g_recontrol_flag = 0;
			break;

		case SET_TRACK_IS_ENCODE:
			set_track_is_encode(data + sizeof(track_header_t));
			g_recontrol_flag = 0;
			break;

		case SET_TRACK_PARAM:
			set_track_param(data + sizeof(track_header_t), track_header->len - sizeof(track_header_t));
			g_recontrol_flag = 0;
			break;

		case SET_RESET_TIME:
			set_reset_time(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_SENS_VALUE:
			set_sens_value(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_SHIELD_RANGE:
			set_shield_range(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_MULTITARGET_RANGE:
			set_multitarget_range(data + sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;

		case SET_MANUAL_COMMOND:
			set_manual_commond(data + sizeof(track_header_t));
			break;

		case GET_TRACK_RANGE:
			send_track_range(socket, pdynamic_param);
			g_recontrol_flag = 0;
			break;

		case GET_TRIGGER_RANGE:
			send_trigger_range(socket, pdynamic_param);
			g_recontrol_flag = 0;
			break;

		case GET_TRACK_TYPE:
			send_track_type(socket, pdynamic_param);
			g_recontrol_flag = 0;
			break;

		case GET_DRAW_LINE_TYPE:
			send_draw_line_type(socket, pdynamic_param);
			g_recontrol_flag = 0;
			break;

		case GET_TRACK_IS_ENCODE:
			send_track_is_encode(socket, g_stutrack_encode_info.is_encode);
			g_recontrol_flag = 0;
			break;

		case GET_TRACK_PARAM:
			send_track_param(socket);
			g_recontrol_flag = 0;
			break;

		case GET_RESET_TIME:
			send_reset_time(socket, pdynamic_param);
			g_recontrol_flag = 0;
			break;

		case GET_SENS_VALUE:
			send_sens_value(socket, pdynamic_param);
			g_recontrol_flag = 0;
			break;

		case GET_SHIELD_RANGE:
			send_shield_range(socket, pdynamic_param);
			g_recontrol_flag = 0;
			break;

		case GET_MULTITARGET_RANGE:
			send_multitarget_range(socket, pdynamic_param);
			g_recontrol_flag = 0;
			break;

		default:
			g_recontrol_flag = 0;
			break;
	}

	if(g_recontrol_flag == 1) {
		System_linkControl(gEduKit->osd_dspAlg_Link[0].link_id,
		                   ALG_LINK_STUTRACK_CMD_SET_CHANNEL_WIN_PRM,
		                   &gEduKit->osd_dspAlg_Link[0].create_params.stutrackCreateParams,
		                   sizeof(AlgLink_StuTrackCreateParams),
		                   FALSE);
		g_recontrol_flag = 0;
	}

	return 0;
}



/**
* @ 函数名称: ctrl_position_preset()
* @ 函数功能: 控制摄像头转到预置位
* @ 输入参数: cam_position -- 预置位编号
* @ 输出参数: data -- 向摄像头发送的整个命令包
* @ 返回值:   发送的数据的长度
*/
static int ctrl_position_preset(unsigned char *data, int cam_position)
{
	int size = 0;

	__stucall_preset_position(cam_position);

	return size;
}


/**
* @	函数名称: ctrl_cam_rotation()
* @ 函数功能: 控制摄像头左右转动或者停止转动
* @ 输入参数: direction -- 移动方向
* @			  distance -- 移动距离
* @ 输出参数: data	-- 向摄像头发送的整个数据包
* @ 返回值:   数据包长度
*/
static int ctrl_cam_rotation(unsigned char *data, int direction, int distance)
{
	int size = 0;

	if(CAMERA_MOVE_STOP == direction) {
		data[0] = g_cam_info.cam_addr;
		data[1] = 0x01;
		data[2] = 0x06;
		data[3] = 0x01;
		data[4] = 0x01;
		data[5] = 0x01;
		data[6] = 0x03;
		data[7] = 0x03;
		data[8] = 0xff;
		size = 9;
	} else {
		data[0] = g_cam_info.cam_addr;
		data[1] = 0x01;
		data[2] = 0x06;
		data[3] = 0x01;
		data[4] = distance / g_cam_info.cam_speed + 1;

		if(data[4] > 0x18) {
			data[4] = 0x18;
		}

		data[5] = 0x01;

		if(CAMERA_MOVE_LEFT == direction) {
			data[6] = 0x02;
		} else if(CAMERA_MOVE_RIGHT == direction) {
			data[6] = 0x01;
		}

		data[7] = 0x03;
		data[8] = 0xff;
		size = 9;
	}

	return size;
}

/**
* @	函数名称: package_cam_cmd()
* @ 函数功能: 控制摄像头操作
* @ 输入参数: output_param -- 跟踪算法返回的参数
* @ 输出参数: data	-- 向摄像头发送的整个数据包
* @ 返回值:   数据包长度
*/
static int package_cam_cmd(unsigned char *data, ITRACK_OutArgs *output_param)
{
	int ret = 0;
	int cam_speed = 0;

	switch(output_param->cmd_type) {
		case CAM_CTRL_POSITION:	//控制摄像头跳转到预置位

			//printf( "reset position output_param->cam_position = %d\n",output_param->cam_position);

			ret = ctrl_position_preset(data, output_param->cam_position);
			break;

		default
				:
			break;
	}

	return (ret);
}


static int get_class_info(StuITRACK_OutArgs *output_param)
{
	if((output_param->nStandUpPos[0] != 0)
	   || (output_param->nStandUpPos[1] != 0)
	   || (output_param->nStandUpPos[2] != 0)
	   || (output_param->nStandUpPos[3] != 0)
	   || (output_param->nFindMoveUpNum != 0)
	   || (output_param->nFindMoveDownNum != 0)
	  ) {
		printf("output_param->nStandUpPos[4] = %d,%d,%d,%d,nFindMoveUpNum=%d,nFindMoveDownNum=%d\n",
		       output_param->nStandUpPos[0], output_param->nStandUpPos[1], output_param->nStandUpPos[2], output_param->nStandUpPos[3],
		       output_param->nFindMoveUpNum, output_param->nFindMoveDownNum);
		g_stuclass_info.nStandupPos[0] = output_param->nStandUpPos[0];
		g_stuclass_info.nStandupPos[1] = output_param->nStandUpPos[1];
		g_stuclass_info.nStandupPos[2] = output_param->nStandUpPos[2];
		g_stuclass_info.nStandupPos[3] = output_param->nStandUpPos[3];
		g_stuclass_info.nUpToPlatformTimes = output_param->nFindMoveUpNum;
		g_stuclass_info.nDownToStudentsAreaTimes = output_param->nFindMoveDownNum;

		SendClassInfotoTeacher();
	}

	return 0;
}


static int get_class_info_rightside()
{
	memset(&g_stuclass_info, 0, sizeof(track_class_info_t));

	if(g_stutrack_encode_info.nStandUpPos != 0) {
		printf("send class info nStandUpPos = %d\n", g_stutrack_encode_info.nStandUpPos - 1);
		g_stuclass_info.nStandupPos[g_stutrack_encode_info.nStandUpPos - 1] = 1;
		g_stutrack_encode_info.nStandUpPos = 0;
		SendClassInfotoTeacher();
	}

	return 0;
}

/**
* @	函数名称: cam_ctrl_cmd()
* @ 函数功能: 响应跟踪算法,并向摄像头发送转动命令
* @ 输入参数: output_param -- 跟踪算法返回的参数
* @ 输出参数: 无
* @ 返回值:   无
*/
int stucam_ctrl_cmd(StuITRACK_OutArgs *output_param)
{

	get_class_info(output_param);

	if(g_stutrack_encode_info.last_position_no != output_param->cam_position) {
		//printf("output_param->cam_position = %d\n",output_param->cam_position);
		if(NOT_PUSH_CLOSE_RANGE != g_stutrack_encode_info.is_control_cam) {
			if(0 != output_param->cam_position) {
				__stucall_preset_position(output_param->cam_position);
			}
		} else {
			__stucall_preset_position(42);	//调用全景，42表示全景预置位
		}

		if((TRIGGER_POSITION42 == output_param->cam_position)
		   && (0 == output_param->cmd_type)) {
			g_stutrack_encode_info.send_cmd = SWITCH_TEATHER;
			g_stutrack_encode_info.last_send_cmd = SWITCH_TEATHER;
		} else {
			if(0 != output_param->cmd_type) {
				g_stutrack_encode_info.send_cmd = SWITCH_STUDENTS;
				g_stutrack_encode_info.last_send_cmd = SWITCH_STUDENTS;
			}
		}

		g_stutrack_encode_info.last_position_no = output_param->cam_position;

	} else {
		if(TRIGGER_POSITION42 == output_param->cam_position) {
			if(1 == output_param->cmd_type) {
				if(g_stutrack_encode_info.last_send_cmd != SWITCH_STUDENTS) {
					g_stutrack_encode_info.send_cmd = SWITCH_STUDENTS;
					g_stutrack_encode_info.last_send_cmd = SWITCH_STUDENTS;
					printf("find multi target up,SWITCH_STUDENTS\n");
				}
			}
		}
	}
}

/**
* @	函数名称: cam_ctrl_cmd_rightside()
* @ 函数功能: 有学生辅助检测时用,响应跟踪算法,并向摄像头发送转动命令
* @ 输入参数: output_param -- 跟踪算法返回的参数
* @ 输出参数: 无
* @ 返回值:   无
*/
int cam_ctrl_cmd_rightside(StuITRACK_OutArgs *output_param)
{

	StuITRACK_DynamicParams *pdynamic_param = &gEduKit->osd_dspAlg_Link[0].create_params.stutrackCreateParams.StuTrackParms.dynamic_param;

	int	i = 0;

	//记录最后一次起立的位置
	for(i = 0; i < 4; i++) {
		if(output_param->nStandUpPos[i] != 0) {
			g_stutrack_encode_info.nStandUpPos = i + 1;
		}
	}

	if(g_rightside_trigger_info.nTriggerType == 0) {
		//课前取景

		if((g_stutrack_encode_info.nLastTriggerVal != 1) && (g_rightside_trigger_info.nTriggerVal == 1)) {
			g_stutrack_encode_info.nOnlyRightSideUpDelay = 0;
			g_stutrack_encode_info.nLastTriggerVal = 1;
			g_stutrack_encode_info.send_cmd = SWITCH_STUDENTS;
			g_stutrack_encode_info.last_send_cmd = SWITCH_STUDENTS;
			printf("only right side students find up,SWITCH_STUDENTS\n");
			get_class_info_rightside();
		}

		if(g_stutrack_encode_info.last_send_cmd == SWITCH_STUDENTS) {
			g_stutrack_encode_info.nOnlyRightSideUpDelay++;

			if(g_stutrack_encode_info.nOnlyRightSideUpDelay >= pdynamic_param->reset_time) {
				g_stutrack_encode_info.nOnlyRightSideUpDelay = 0;
				g_stutrack_encode_info.send_cmd = SWITCH_TEATHER;
				g_stutrack_encode_info.last_send_cmd = SWITCH_TEATHER;
				g_stutrack_encode_info.last_position_no = 42;
				__stucall_preset_position(42); //调用全景，42表示全景预置位
				printf("only right side up,SWITCH_TEATHER\n");
			}
		}

		if(g_rightside_trigger_info.nTriggerVal == 1) {
			//printf("output_param->cam_position = %d,g_track_encode_info.last_position_no=%d\n",output_param->cam_position,g_track_encode_info.last_position_no);
			if(g_stutrack_encode_info.last_position_no != output_param->cam_position) {
				if(0 != output_param->cmd_type) {
					if(NOT_PUSH_CLOSE_RANGE != g_stutrack_encode_info.is_control_cam) {
						if(0 != output_param->cam_position) {
							__stucall_preset_position(output_param->cam_position);
						}
					} else {
						__stucall_preset_position(42); //调用全景，42表示全景预置位
					}

					g_stutrack_encode_info.nOnlyRightSideUpDelay = 0;
					g_stutrack_encode_info.last_position_no = output_param->cam_position;
					g_stutrack_encode_info.send_cmd = SWITCH_STUDENTS;
					g_stutrack_encode_info.last_send_cmd = SWITCH_STUDENTS;
					printf("1output_param->cam_position = %d,SWITCH_STUDENTS\n", output_param->cam_position);
					get_class_info_rightside();
				}
			} else {
				if(TRIGGER_POSITION42 == output_param->cam_position) {
					if(1 == output_param->cmd_type) {
						if(g_stutrack_encode_info.last_send_cmd != SWITCH_STUDENTS) {
							g_stutrack_encode_info.nOnlyRightSideUpDelay = 0;
							g_stutrack_encode_info.send_cmd = SWITCH_STUDENTS;
							g_stutrack_encode_info.last_send_cmd = SWITCH_STUDENTS;
							printf("find multi target up,SWITCH_STUDENTS\n");
							get_class_info_rightside();
						}
					}
				}
			}
		} else if(g_rightside_trigger_info.nTriggerVal == 2) {
			//坐下了
			//如果有起立也要清掉误触发
			output_param->nStandUpPos[0] = 0;
			output_param->nStandUpPos[1] = 0;
			output_param->nStandUpPos[2] = 0;
			output_param->nStandUpPos[3] = 0;
			g_stutrack_encode_info.nOnlyRightSideUpDelay = 0;
			//if(g_track_encode_info.last_position_no != output_param->cam_position)
			{
				__stucall_preset_position(42); //调用全景，42表示全景预置位
				g_stutrack_encode_info.send_cmd = SWITCH_TEATHER;
				g_stutrack_encode_info.last_send_cmd = SWITCH_TEATHER;
				printf("1output_param->cam_position = %d,SWITCH_TEATHER\n", output_param->cam_position);
			}
			g_stutrack_encode_info.last_position_no = 42;
			g_rightside_trigger_info.nTriggerVal = 0;
			g_stutrack_encode_info.nLastTriggerVal = g_rightside_trigger_info.nTriggerVal;
		}

		//printf("output_param->cam_position = %d\n",output_param->cam_position);
		get_class_info(output_param);
		g_stutrack_encode_info.nTriggerValDelay = 0;
	} else if(g_rightside_trigger_info.nTriggerType == 1) {
		//课中取景

		if(g_stutrack_encode_info.last_send_cmd == SWITCH_STUDENTS) {
			g_stutrack_encode_info.nOnlyRightSideUpDelay++;

			if(g_stutrack_encode_info.nOnlyRightSideUpDelay >= pdynamic_param->reset_time) {
				g_stutrack_encode_info.nOnlyRightSideUpDelay = 0;
				g_stutrack_encode_info.send_cmd = SWITCH_TEATHER;
				g_stutrack_encode_info.last_send_cmd = SWITCH_TEATHER;
				g_stutrack_encode_info.last_position_no = 42;
				__stucall_preset_position(42); //调用全景，42表示全景预置位
				printf("only right side up,SWITCH_TEATHER\n");
			}
		}

		if(g_rightside_trigger_info.nTriggerVal == 1) {
			//printf("output_param->cam_position = %d,g_track_encode_info.last_position_no=%d\n",output_param->cam_position,g_track_encode_info.last_position_no);

			printf("last_position_no = %d cam_position = %d cmd_type = %d\n", g_stutrack_encode_info.last_position_no, output_param->cam_position, output_param->cmd_type);

			if(g_stutrack_encode_info.last_position_no != output_param->cam_position) {
				if(NOT_PUSH_CLOSE_RANGE != g_stutrack_encode_info.is_control_cam) {
					if(0 != output_param->cam_position) {
						__stucall_preset_position(output_param->cam_position);
					}
				} else {
					__stucall_preset_position(42);	//调用全景，42表示全景预置位
				}

				if((TRIGGER_POSITION42 == output_param->cam_position)
				   && (0 == output_param->cmd_type)) {
					g_stutrack_encode_info.nOnlyRightSideUpDelay = 0;
					g_stutrack_encode_info.send_cmd = SWITCH_TEATHER;
					g_stutrack_encode_info.last_send_cmd = SWITCH_TEATHER;
					printf("2output_param->cam_position = %d,SWITCH_TEATHER\n", output_param->cam_position);
				} else {
					if(0 != output_param->cmd_type) {
						g_stutrack_encode_info.nOnlyRightSideUpDelay = 0;
						g_stutrack_encode_info.send_cmd = SWITCH_STUDENTS;
						g_stutrack_encode_info.last_send_cmd = SWITCH_STUDENTS;
						printf("2output_param->cam_position = %d,SWITCH_STUDENTS\n", output_param->cam_position);
						get_class_info_rightside();
					}
				}

				g_stutrack_encode_info.last_position_no = output_param->cam_position;

				g_rightside_trigger_info.nTriggerVal = 0;
				g_stutrack_encode_info.nTriggerValDelay = 0;
			} else {
				if(TRIGGER_POSITION42 == output_param->cam_position) {
					if(1 == output_param->cmd_type) {
						if(g_stutrack_encode_info.last_send_cmd != SWITCH_STUDENTS) {
							g_stutrack_encode_info.nOnlyRightSideUpDelay = 0;
							g_stutrack_encode_info.send_cmd = SWITCH_STUDENTS;
							g_stutrack_encode_info.last_send_cmd = SWITCH_STUDENTS;
							printf("find multi target up,SWITCH_STUDENTS\n");
							get_class_info_rightside();
						}
					}
				}
			}

			g_stutrack_encode_info.nTriggerValDelay++;

			if(g_stutrack_encode_info.nTriggerValDelay >= 30) {
				g_rightside_trigger_info.nTriggerVal = 0;
				g_stutrack_encode_info.nTriggerValDelay = 0;
				printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			}
		}

		//printf("output_param->cam_position = %d\n",output_param->cam_position);

	}
}



int server_set_stutrack_type(short type)
{

	StuITRACK_DynamicParams *pdynamic_param = &gEduKit->osd_dspAlg_Link[0].create_params.stutrackCreateParams.StuTrackParms.dynamic_param;

	if((AUTO_CONTROL == pdynamic_param->control_mode) && (AUTO_CONTROL == type)) {

		printf("stu pdynamic_param->control_mode = AUTO_CONTROL, not need set!!!\n");
		return 0;
	}


	pdynamic_param->control_mode = type;

	if(AUTO_CONTROL == pdynamic_param->control_mode) {
		__stucall_preset_position(TRIGGER_POSITION42);
		//g_recontrol_flag = 1;
	}

	System_linkControl(gEduKit->osd_dspAlg_Link[0].link_id,
	                   ALG_LINK_STUTRACK_CMD_SET_CHANNEL_WIN_PRM,
	                   &gEduKit->osd_dspAlg_Link[0].create_params.stutrackCreateParams,
	                   sizeof(AlgLink_StuTrackCreateParams),
	                   FALSE);

	return 0;
}


/**
* @	函数名称: track_init()
* @	函数功能: 初始化跟踪参数
* @ 输入参数:
* @ 输出参数:
* @ 返回值:
*/
int stutrack_init(StuITRACK_Params *track_param)
{
	int 	ret = -1;
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char 	temp[FILE_NAME_LEN]			= {0};
	char	text[FILE_NAME_LEN]			= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int		index						= 0;
	int		i 							= 0;

	//memset(track_param,0,sizeof(ITRACK_Params));
	track_param->size = sizeof(StuITRACK_Params);

	//track_param->dynamic_param.reset_time = 300;
	track_param->dynamic_param.reset_level = RE_START;

	track_param->dynamic_param.trigger_num = STUDENTS_TRIGGER_NUM;


	//学生跟踪配置文件名称
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);

	//-----------------------静态参数读取------------------------
	//视频宽度
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, VIDEO_WIDTH, temp);

	if(ret != 0) {
		track_param->static_param.video_width = 704;
	} else {
		track_param->static_param.video_width = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("track_param->static_param.video_width = %d\n", track_param->static_param.video_width);
#else
	printf("track_param->static_param.video_width = %d\n", track_param->static_param.video_width);
#endif

	//视频高度
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, VIDEO_HEIGHT, temp);

	if(ret != 0) {
		track_param->static_param.video_height = 576;
	} else {
		track_param->static_param.video_height = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("track_param->static_param.video_height = %d\n", track_param->static_param.video_height);
#else
	printf("track_param->static_param.video_height = %d\n", track_param->static_param.video_height);
#endif

	//跟踪处理图像的宽度
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, PIC_WIDTH, temp);

	if(ret != 0) {
		track_param->static_param.pic_width = 176;
	} else {
		track_param->static_param.pic_width = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("track_param->static_param.pic_width = %d\n", track_param->static_param.pic_width);
#else
	printf("track_param->static_param.pic_width = %d\n", track_param->static_param.pic_width);
#endif

	//跟踪处理图像的高度
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, PIC_HEIGHT, temp);

	if(ret != 0) {
		track_param->static_param.pic_height = 144;
	} else {
		track_param->static_param.pic_height = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("track_param->static_param.pic_height = %d\n", track_param->static_param.pic_height);
#else
	printf("track_param->static_param.pic_height = %d\n", track_param->static_param.pic_height);
#endif

	//---------------------------动态参数的读取----------------------------
	//跟踪中丢失目标后,到下次重新跟踪时间,又叫复位时间
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, RESET_TIME, temp);

	if(ret != 0) {
		track_param->dynamic_param.reset_time = 80;
	} else {
		track_param->dynamic_param.reset_time = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.reset_time = %d\n", track_param->dynamic_param.reset_time);
#else
	printf("track_param->dynamic_param.reset_time = %d\n", track_param->dynamic_param.reset_time);
#endif

	//横坐标缩放倍数
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, X_OFFSET, temp);

	if(ret != 0) {
		track_param->dynamic_param.x_offset = 4;
	} else {
		track_param->dynamic_param.x_offset = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.x_offset = %d\n", track_param->dynamic_param.x_offset);
#else
	printf("track_param->dynamic_param.x_offset = %d\n", track_param->dynamic_param.x_offset);
#endif

	//纵坐标缩放倍数
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, Y_OFFSET, temp);

	if(ret != 0) {
		track_param->dynamic_param.y_offset = 4;
	} else {
		track_param->dynamic_param.y_offset = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.y_offset = %d\n", track_param->dynamic_param.y_offset);
#else
	printf("track_param->dynamic_param.y_offset = %d\n", track_param->dynamic_param.y_offset);
#endif

	//读取触发区域的参数值
	for(index = 0; index < STUDENTS_TRIGGER_NUM; index++) {
		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, TRIGGER_X);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.trigger_info[index].x = 0;
		} else {
			track_param->dynamic_param.trigger_info[index].x = atoi(temp);
		}


		printf("track_param->dynamic_param.trigger_info[index].x = %d,index = %d\n", track_param->dynamic_param.trigger_info[index].x, index);

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, TRIGGER_Y);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.trigger_info[index].y = 0;
		} else {
			track_param->dynamic_param.trigger_info[index].y = atoi(temp);
		}

		printf("track_param->dynamic_param.trigger_info[index].y = %d,index = %d\n", track_param->dynamic_param.trigger_info[index].y, index);

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, TRIGGER_WIDTH);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.trigger_info[index].width = 0;
		} else {
			track_param->dynamic_param.trigger_info[index].width = atoi(temp);
		}

		printf("track_param->dynamic_param.trigger_info[index].width = %d,index = %d\n", track_param->dynamic_param.trigger_info[index].width, index);

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, TRIGGER_HEIGHT);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.trigger_info[index].height = 0;
		} else {
			track_param->dynamic_param.trigger_info[index].height = atoi(temp);
		}

		printf("track_param->dynamic_param.trigger_info[index].height = %d,index = %d\n", track_param->dynamic_param.trigger_info[index].height, index);
	}

	//读取屏蔽区域的参数值
	for(index = 0; index < STUDENTS_SHIELD_NUM; index++) {
		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, SHIELD_X);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.shield_info[index].x = 0;
		} else {
			track_param->dynamic_param.shield_info[index].x = atoi(temp);
		}

		printf("track_param->dynamic_param.shield_info[index].x = %d,index = %d\n", track_param->dynamic_param.shield_info[index].x, index);

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, SHIELD_Y);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.shield_info[index].y = 0;
		} else {
			track_param->dynamic_param.shield_info[index].y = atoi(temp);
		}

		printf("track_param->dynamic_param.shield_info[index].y = %d,index = %d\n", track_param->dynamic_param.shield_info[index].y, index);

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, SHIELD_WIDTH);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.shield_info[index].width = 0;
		} else {
			track_param->dynamic_param.shield_info[index].width = atoi(temp);
		}

		printf("track_param->dynamic_param.shield_info[index].width = %d,index = %d\n", track_param->dynamic_param.shield_info[index].width, index);

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, SHIELD_HEIGHT);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.shield_info[index].height = 0;
		} else {
			track_param->dynamic_param.shield_info[index].height = atoi(temp);
		}

		printf("track_param->dynamic_param.shield_info[index].height = %d,index = %d\n", track_param->dynamic_param.shield_info[index].height, index);
	}

	//读取多目标上台检测区域的参数值
	for(index = 0; index < STUDENTS_MULTITARGET_NUM; index++) {
		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, MULTITARGET_X);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.multitarget_info[index].x = 0;
		} else {
			track_param->dynamic_param.multitarget_info[index].x = atoi(temp);
		}

		printf("track_param->dynamic_param.multitarget_info[index].x = %d,index = %d\n", track_param->dynamic_param.multitarget_info[index].x, index);

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, MULTITARGET_Y);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.multitarget_info[index].y = 0;
		} else {
			track_param->dynamic_param.multitarget_info[index].y = atoi(temp);
		}

		printf("track_param->dynamic_param.multitarget_info[index].y = %d,index = %d\n", track_param->dynamic_param.multitarget_info[index].y, index);

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, MULTITARGET_WIDTH);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.multitarget_info[index].width = 0;
		} else {
			track_param->dynamic_param.multitarget_info[index].width = atoi(temp);
		}

		printf("track_param->dynamic_param.multitarget_info[index].width = %d,index = %d\n", track_param->dynamic_param.multitarget_info[index].width, index);

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, MULTITARGET_HEIGHT);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.multitarget_info[index].height = 0;
		} else {
			track_param->dynamic_param.multitarget_info[index].height = atoi(temp);
		}

		printf("track_param->dynamic_param.multitarget_info[index].height = %d,index = %d\n", track_param->dynamic_param.multitarget_info[index].height, index);
	}

	//sens值,越小边缘值找到越多
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, SENS, temp);

	if(ret != 0) {
		track_param->dynamic_param.sens = 26;
	} else {
		track_param->dynamic_param.sens = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.sens = %d\n", track_param->dynamic_param.sens);
#else
	printf("track_param->dynamic_param.sens = %d\n", track_param->dynamic_param.sens);
#endif

	//画线标志
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MESSAGE, temp);

	if(ret != 0) {
		track_param->dynamic_param.message = 1;
	} else {
		track_param->dynamic_param.message = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.message = %d\n", track_param->dynamic_param.message);
#else
	printf("track_param->dynamic_param.message = %d\n", track_param->dynamic_param.message);
#endif


	//跟踪区域点数
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRACK_POINT_NUM, temp);

	if(ret != 0) {
		track_param->dynamic_param.track_point_num = 4;
	} else {
		track_param->dynamic_param.track_point_num = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.track_point_num = %d\n", track_param->dynamic_param.track_point_num);
#else
	printf("track_param->dynamic_param.track_point_num = %d\n", track_param->dynamic_param.track_point_num);
#endif

	for(index = 0; index < track_param->dynamic_param.track_point_num; index++) {
		//跟踪区域点的坐标
		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, TRACK_POINTX);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.track_point[index].x = 0;
		} else {
			track_param->dynamic_param.track_point[index].x = atoi(temp);
		}

		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, TRACK_POINTY);
		sprintf(param_name, "%s%d", text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);

		if(ret != 0) {
			track_param->dynamic_param.track_point[index].y = 0;
		} else {
			track_param->dynamic_param.track_point[index].y = atoi(temp);
		}

#ifdef ENC_1200_DEBUG
		PRINTF("track_param->dynamic_param.track_point[%d].x = %d\n", index, track_param->dynamic_param.track_point[index].x);
		PRINTF("track_param->dynamic_param.track_point[%d].y = %d\n", index, track_param->dynamic_param.track_point[index].y);
#else
		printf("track_param->dynamic_param.track_point[%d].x = %d\n", index, track_param->dynamic_param.track_point[index].x);
		printf("track_param->dynamic_param.track_point[%d].y = %d\n", index, track_param->dynamic_param.track_point[index].y);
#endif
	}


	//摄像头预置位的位置信息,0的位置暂时保留,没有使用
	for(index = 1; index < PRESET_NUM_MAX; index++) {
		memset(temp, 0, FILE_NAME_LEN);
		strcpy(text, PRESET_POSITION_VALUE);
		sprintf(param_name, "%s%d", text, index);

		ret =  ConfigGetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);

		if(ret == 0) {
			for(i = 0; i < 8; i++) {
				if((temp[i] >= 0x30) && (temp[i] <= 0x39)) {
					g_cam_info.cam_position[index].pan_tilt[i] = temp[i] - 0x30;
				}

				if((temp[i] >= 0x41) && (temp[i] <= 0x46)) {
					g_cam_info.cam_position[index].pan_tilt[i] = temp[i] - 0x41 + 0xa;
				}

				if((temp[i] >= 0x61) && (temp[i] <= 0x66)) {
					g_cam_info.cam_position[index].pan_tilt[i] = temp[i] - 0x61 + 0xa;
				}
			}
		} else {
			g_cam_info.cam_position[index].pan_tilt[0] = 0x07;
			g_cam_info.cam_position[index].pan_tilt[1] = 0x0f;
			g_cam_info.cam_position[index].pan_tilt[2] = 0x0f;
			g_cam_info.cam_position[index].pan_tilt[3] = 0x0f;
			g_cam_info.cam_position[index].pan_tilt[4] = 0x07;
			g_cam_info.cam_position[index].pan_tilt[5] = 0x0f;
			g_cam_info.cam_position[index].pan_tilt[6] = 0x0f;
			g_cam_info.cam_position[index].pan_tilt[7] = 0x0f;
		}

#ifdef ENC_1200_DEBUG
		PRINTF("###########index = %d,%s\n", index, temp);
#else
		printf("###########index = %d,%s\n", index, temp);
#endif

		memset(temp, 0, FILE_NAME_LEN);
		strcpy(text, PRESET_ZOOM_VALUE);
		sprintf(param_name, "%s%d", text, index);

		ret =  ConfigGetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);

		if(ret == 0) {
			for(i = 0; i < 4; i++) {
				if((temp[i] >= 0x30) && (temp[i] <= 0x39)) {
					g_cam_info.cam_position[index].zoom[i] = temp[i] - 0x30;
				}

				if((temp[i] >= 0x41) && (temp[i] <= 0x46)) {
					g_cam_info.cam_position[index].zoom[i] = temp[i] - 0x41 + 0xa;
				}

				if((temp[i] >= 0x61) && (temp[i] <= 0x66)) {
					g_cam_info.cam_position[index].zoom[i] = temp[i] - 0x61 + 0xa;
				}
			}
		} else {
			g_cam_info.cam_position[index].zoom[0] = 0x07;
			g_cam_info.cam_position[index].zoom[1] = 0x0f;
			g_cam_info.cam_position[index].zoom[2] = 0x0f;
			g_cam_info.cam_position[index].zoom[3] = 0x0f;
		}

	}

	//跟踪机是否编码
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STUDENTS_TRACK_ENCODE, STUDENTS_IS_ENCODE, temp);


	if(ret != 0) {
		g_stutrack_encode_info.is_encode = 1;
	} else {
		g_stutrack_encode_info.is_encode = atoi(temp);
	}

#ifdef ENC_1200_DEBUG
	PRINTF("g_stutrack_encode_info.is_encode = %d\n", g_stutrack_encode_info.is_encode);
#else
	printf("g_stutrack_encode_info.is_encode = %d\n", g_stutrack_encode_info.is_encode);
#endif

	//学生机是否推近景标志，为1为推近景，为0不推近景
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, CAM_CONTROL_TYPE, IS_CONTROL_CAM, temp);


	if(ret != 0) {
		g_stutrack_encode_info.is_control_cam = 1;
	} else {
		g_stutrack_encode_info.is_control_cam = atoi(temp);
	}


	printf("g_track_encode_info.is_encode = %d\n", g_stutrack_encode_info.is_control_cam);



	//
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, SUPPORT_STUSIDE_TRACK, temp);


	if(ret != 0) {
		gEduKit->SupportStuSide = 0;
	} else {
		gEduKit->SupportStuSide = atoi(temp);
	}


	printf("gEduKit->SupportStuSide = %d\n", gEduKit->SupportStuSide);



	//摄像机延时参数读取，为调用预制位时两个命令之间的延时参数
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, STUDENTS_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, CAM_CONTROL_TYPE, CAM_ZOOM_PAN_DELAY, temp);
	g_stutrack_encode_info.zoom_pan_delay = atoi(temp);

	printf("g_track_encode_info.zoom_pan_delay = %d\n", g_stutrack_encode_info.zoom_pan_delay);

	if(ret != 0) {
		g_stutrack_encode_info.zoom_pan_delay = 500;
	}

	//摄像头地址
	g_cam_info.cam_addr = 0x82;

	g_stutrack_encode_info.is_track = 1;
	//set_cam_addr();

	return 0;
}


