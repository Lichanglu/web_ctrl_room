
/*
 * auto_track.h
 *
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2012
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */


#ifndef _COMMON_TRACK_H
#define _COMMON_TRACK_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <time.h>
#include <osa.h>
#include <link_api/system.h>
#include <mcfw/interfaces/link_api/trackLink.h>
#include <mcfw/interfaces/link_api/stutrackLink.h>
#include <mcfw/interfaces/link_api/stusidetrackLink.h>
#include "reach.h"
#include "rwini.h"

/**
* @	文件名称的长度
*/
#define FILE_NAME_LEN		(64)

/**
* @ 教师跟踪参数保存文件
*/
#define TEACH_TRACK_FILE		"teach_track.ini"

/**
* @ 学生跟踪参数保存文件
*/
#define STUDENTS_TRACK_FILE		"students_track.ini"

/**
* @ 教师跟踪参数保存文件
*/
#define TRACK_STUDENTS_RIGHT_SIDE_FILE		"track_students_right_side.ini"

/**
* @	保存课前取景的图片路径
*/
#define	CLASS_VIEW_JPG	"class_view.gpg"


#define DEBUG_INFO "debug_info"
#define DEBUG_IPADDR "debug_ipaddr"


/**
* @	摄像机绝对位置所占的字节数普通VISCA占用9个字节，索尼BRC-Z330占用8个字节
*/
#define	CAM_PAN_TILT_LEN	(9)

/**
* @	摄像机绝对焦距所占字节数为4个字节
*/
#define	CAM_ZOOM_LEN		(4)




/**
* @	预置位的最大个数
*/
#define PRESET_NUM_MAX					(60)


/**
* @	固定消息字
*/
#define FIXED_MSG						(0xFFF0)


//==============================设置参数============================
/**
* @	设定跟踪范围的消息字
*/
#define SET_TRACK_RANGE					(0x0001)

/**
* @	设定触发区域消息字
*/
#define SET_TRIGGER_RANGE				(0x0002)

/**
* @	设置人头像的跟踪线的高				
*/
#define	SET_TRACK_LIMIT_HEIGHT			(0x0003)

/**
* @	跟踪类型消息字 
*/
#define SET_TRACK_TYPE					(0x0004)


/**
* @	设置画线类型,调试线,如跟踪框线,跟踪区域线,轨迹线等
*/
#define SET_DRAW_LINE_TYPE				(0x0005)

/**
* @	设置摄像头控制速度的基数消息字 
*/
#define SET_CAM_SPEED_TYPE				(0x0006)

/**
* @	设置触发点总数消息字 
*/
#define SET_TRIGGER_NUM_TYPE			(0x0007)

/**
* @	设置预置位消息字 
*/
#define SET_PRESET_POSITION_TYPE		(0x0008)

/**
* @	设置摄像头上下左右限位消息字 
*/
#define SET_LIMIT_POSITION_TYPE			(0x0009)

/**
* @	调用预置位消息字
*/
#define CALL_PRESET_POSITION_TYPE		(0x000a)

/**
* @	设置跟踪距离
*/
#define SET_TRACK_DISTANCE_TYPE			(0x000b)

/**
* @	设置跟踪机是否编码的开关
*/
#define SET_TRACK_IS_ENCODE				(0x000c)

/**
* @ 导入跟踪参数消息字
*/
#define SET_TRACK_PARAM					(0x000d)

/**
* @ 设置预跟踪时间
*/
#define SET_START_TRACK_TIME			(0x000e)

/**
* @ 设置跟踪复位时间
*/
#define SET_RESET_TIME					(0x000f)


/**
* @ 设置检测变化系数sens值
*/
#define SET_SENS_VALUE					(0x0010)

/**
* @ 设置屏蔽区域
*/
#define SET_SHIELD_RANGE				(0x0011)

/**
* @ 设置手动命令
*/
#define SET_MANUAL_COMMOND				(0x0012)

/**
* @ 设置多目标上台检测框
*/
#define SET_MULTITARGET_RANGE				(0x0013)


/**
* @ 设置液晶屏辅助检范围
*/
#define SET_MEANSHIFT_TRIGGER			(0x0014)

/**
* @ 设置全景触发范围
*/
#define SET_POS1_TRIGGER				(0x0015)

/**
* @ 设置全景触发点总数
*/
#define SET_POS1_TRIGGER_SUM			(0x0016)

/**
* @ 设置板书区域
*/
#define SET_BLACKBOARD_AREA				(0x0017)

/**
* @ 设置机位
*/
#define SET_STRATEGY_NO					(0x0018)

/**
* @ 设置液晶显示屏辅助跟踪开关
*/
#define SET_MEANSHIFT_FLAG				(0x0019)

/**
* @ 设置学生全景保持时间
*/
#define SET_STUDENTS_PANORAMA_SWITCH_NEAR_TIME				(0x001a)

/**
* @ 设置老师走到板书1区域切板书的时间
*/
#define SET_TEACHER_BLACKBOARD_TIME1				(0x001b)

/**
* @ 设置离开板书1区域切老师的时间
*/
#define SET_TEACHER_LEAVE_BLACKBOARD_TIME1				(0x001c)

/**
* @ 设置学生下讲台后不启用跟踪的时间
*/
#define SET_STUDENTS_DOWN_TIME				(0x001d)

/**
* @ 设置老师近景摄像头移动切换老师全景的时间
*/
#define SET_TEACHER_PANORAMA_TIME				(0x001e)

/**
* @ 设置老师摄像头停止移动切换老师近景的时间
*/
#define SET_TEACHER_LEAVE_PANORAMA_TIME				(0x001f)

/**
* @ 设置学生上讲台后保持老师全景的最长时间
*/
#define SET_TEACHER_KEEP_PANORAMA_TIME				(0x0020)

/**
* @ 设置老师切换为学生前的老师镜头保持时间
*/
#define SET_TEACHER_SWITCH_STUDENTS_DELAY_TIME				(0x0021)

/**
* @ 设置学生近景保持的时间
*/
#define SET_STUDENTS_NEAR_KEEP_TIME				(0x0022)

/**
* @ 设置老师丢了时老师全景与学生全景切换延时
*/
#define SET_MV_KEEP_TIME				(0x0023)

/**
* @ 设置vga保持时间
*/
#define SET_VGA_KEEP_TIME				(0x0024)

/**
* @ 设置老师走到板书2区域切板书的时间
*/
#define SET_TEACHER_BLACKBOARD_TIME2				(0x0025)

/**
* @ 设置离开板书2区域切老师的时间
*/
#define SET_TEACHER_LEAVE_BLACKBOARD_TIME2				(0x0026)




//-----------------------------返回参数消息字-------------------------

/**
* @	获取跟踪范围的消息字
*/
#define GET_TRACK_RANGE					(0x0100)

/**
* @	获取触发区域消息字
*/
#define GET_TRIGGER_RANGE				(0x0101)

/**
* @	获取人头像的跟踪线的高				
*/
#define	GET_TRACK_LIMIT_HEIGHT			(0x0102)

/**
* @	获取跟踪类型				
*/
#define	GET_TRACK_TYPE					(0x0103)

/**
* @	获取画线类型				
*/
#define	GET_DRAW_LINE_TYPE				(0x0104)

/**
* @	获取摄像头速度基数			
*/
#define	GET_CAM_SPEED_TYPE				(0x0105)

/**
* @	获取触发点总数			
*/
#define	GET_TRIGGER_NUM_TYPE			(0x0106)

/**
* @	获取跟踪距离			
*/
#define	GET_TRACK_DISTANCE_TYPE			(0x0107)

/**
* @	获取跟踪机是否编码的开关
*/
#define GET_TRACK_IS_ENCODE				(0x0108)

/**
* @ 导出跟踪参数消息字
*/
#define GET_TRACK_PARAM					(0x0109)


/**
* @ 获取预跟踪时间
*/
#define GET_START_TRACK_TIME			(0x010a)

/**
* @ 获取跟踪复位时间
*/
#define GET_RESET_TIME					(0x010b)

/**
* @ 获取检测变化系数sens值
*/
#define GET_SENS_VALUE					(0x010c)

/**
* @ 获取屏蔽区域
*/
#define GET_SHIELD_RANGE				(0x010d)

/**
* @ 获取多目标上台检测框
*/
#define GET_MULTITARGET_RANGE				(0x010e)


/**
* @ 获取液晶屏辅助检范围
*/
#define GET_MEANSHIFT_TRIGGER			(0x010F)

/**
* @ 获取全景触发范围
*/
#define GET_POS1_TRIGGER				(0x0110)

/**
* @ 获取全景触发点总数
*/
#define GET_POS1_TRIGGER_NUM			(0x0111)


/**
* @获取机位
*/
#define GET_STRATEGY_NO					(0x0112)

/**
* @获取液晶显示屏辅助跟踪开关
*/
#define GET_MEANSHIFT_FLAG				(0x0113)

/**
* @ 获取学生全景保持时间
*/
#define GET_STUDENTS_PANORAMA_SWITCH_NEAR_TIME				(0x0114)

/**
* @获取老师走到板书1区域切板书的时间
*/
#define GET_TEACHER_BLACKBOARD_TIME1				(0x0115)

/**
* @ 获取离开板书1区域切老师的时间
*/
#define GET_TEACHER_LEAVE_BLACKBOARD_TIME1				(0x0116)

/**
* @ 获取学生下讲台后不启用跟踪的时间
*/
#define GET_STUDENTS_DOWN_TIME				(0x0117)

/**
* @ 获取老师近景摄像头移动切换老师全景的时间
*/
#define GET_TEACHER_PANORAMA_TIME				(0x0118)

/**
* @ 获取老师摄像头停止移动切换老师近景的时间
*/
#define GET_TEACHER_LEAVE_PANORAMA_TIME				(0x0119)

/**
* @ 获取学生上讲台后保持老师全景的最长时间
*/
#define GET_TEACHER_KEEP_PANORAMA_TIME				(0x011A)

/**
* @获取老师切换为学生前的老师镜头保持时间
*/
#define GET_TEACHER_SWITCH_STUDENTS_DELAY_TIME				(0x011B)

/**
* @ 获取学生近景保持的时间
*/
#define GET_STUDENTS_NEAR_KEEP_TIME				(0x011C)

/**
* @ 获取老师丢了时老师全景与学生全景切换延时
*/
#define GET_MV_KEEP_TIME				(0x011D)

/**
* @获取vga保持时间
*/
#define GET_VGA_KEEP_TIME				(0x011E)


/**
* @获取老师走到板书2区域切板书的时间
*/
#define GET_TEACHER_BLACKBOARD_TIME2				(0x011f)

/**
* @ 获取离开板书2区域切老师的时间
*/
#define GET_TEACHER_LEAVE_BLACKBOARD_TIME2				(0x0120)


//======================================================


#define	MSG_SET_STUSIDETRACK_PARAM	0xA2	//教师跟踪参数设置

//-------------------设置手动命令的子类型---------------------

typedef enum manualMANUAL_COMMAND_ENUM
{
/**
* @	设置是否推拉学生摄像机近景类型
*/
	SET_IS_CONTROL_CAM	=	0x01,


/**
* @	设置预制位时调用跳转到绝对焦距和绝对水平，垂直位置时的时间间隔设置
* @  由于索尼brc-z330摄像机在调完一条命令后必须等待执行完成后，才能发第二条命令，
* @  否则第二条命令将不会执行。
*/
	SET_ZOOM_PAN_DELAY	=	0x02,

/**
* @	设置设置黑板左边区域和右边区域的命令，参数值为0表示左边，为1表示右边 
*/
	SET_BLACKBOARD_REGION	= 0x03
	
}manualMANUAL_COMMAND_e;

/**
* @	最大活跃时间段数
*/
#define MAX_INTERACTION_NUM					(20)

/**
* @	最大起立次数
*/
#define MAX_STANDUP_NUM					(50)


/**
* @	每个统计时间段的长度，分钟表示
*/
#define CLASSINFO_PER_TIME              (15)

/**
* @	设置是否腿学生摄像机近景类型
*/
#define SET_IS_CONTROL_CAM		0x01

#define	PUSH_CLOSE_RANGE		(1)		//推近景
#define NOT_PUSH_CLOSE_RANGE	(0)		//不推近景


/**
* @	设置统计课堂信息
*/
#define SET_IS_GET_CLASSINFO		0x02
#define NO_CLASS_INFO	(0)		//不统计课堂信息
#define	FIND_CLASS_INFO		(1)		//统计课堂信息

/**
* @	确定跟踪框点的个数
*/
#define TRACK_RANGE_NUM					(10)

/**
* @	确定触发框点的个数
*/
#define TRACK_TRIGGER_NUM				(4)
#define STUTRACK_TRIGGER_NUM			(50)


#define STUDENTS_UP		(1)		//学生站起来
#define	STUDENTS_DOWN	(2)		//学生坐下

typedef enum _switch_cmd_type
{
	//1画面布局
	SWITCH_TEATHER = 1,		//切换老师机命令
	SWITCH_STUDENTS= 3,		//切换学生机命令
	SWITCH_VGA					= 255,		//切换VGA命令
	SWITCH_BLACKBOARD1			= 4,		//切换板书摄像机命令
	SWITCH_STUDENTS_PANORAMA 	= 5,		//切换学生全景摄像机
	SWITCH_TEACHER_PANORAMA		= 6,		//切换讲台摄像机全景
	SWITCH_BLACKBOARD2			= 6,        //板书2 和 老师全景不共存

	//2画面布局
    SWITCH_2_VGA_TEATHER        = 0x211,        //VGA大 老师小 左上
    SWITCH_2_TEATHER_STU        = 0x213,        //老师大 学生小 左上
    SWITCH_2_STU_TEATHER        = 0x214,        //学生大 老师小 左上
    SWITCH_2_STUPANORAMA_TEATHER    = 0x215,        //学生全景 老师 左上
	
    SWITCH_2_TEATHER_AND_VGA    = 0x221,        //老师左 VGA右       等分
    SWITCH_2_STU_AND_TEATHER    = 0x222,        //学生左 老师右      等分
    SWITCH_2_STUPANORAMA_AND_TEATHER    = 0x223, //学生全景左 老师右 等分 
    SWITCH_2_TEATHER_AND_STU    = 0x224,         //老师左 学生右     等分

	SWITCH_2_VGA_TEATHER_1      = 0x231,        //VGA大 老师小 右下
	SWITCH_2_STUPANORAMA_TEATHER_1    = 0x232,        //学生全景 老师小 右下
	SWITCH_2_STU_TEATHER_1      = 0x233,        //学生大 老师小 右下                  ///

	SWITCH_2_VGA_TEATHER_2      = 0x241,           //VGA大 老师大 右上
	SWITCH_2_STU_TEATHER_2      = 0x242,          //学生大 老师右上
	SWITCH_2_STUPANORAMA_TEATHER_2      = 0x243,          //学生全景 老师右上

	//3画面布局
	SWITCH_3_VGA_TEATHER_STU    = 0x311,         //VGA大 老师近景 学生近景
	SWITCH_3_VGA_TEACHPANORAMA_STU    = 0x312,    //VGA大 老师全景 学生近景
	SWITCH_3_VGA_TEATHER_STUPANORAMA  = 0x313,    //VGA大 老师近景 学生全景
	SWITCH_3_VGA_TEACHPANORAMA_STUPANORAMA  = 0x314,  //VGA大 老师全景 学生全景
	
	SWITCH_3_BLACKBOARD1_TEATHER_STU  = 0x315,   //板书大 老师近景 学生近景
	SWITCH_3_BLACKBOARD1_TEACHPANORAMA_STU  = 0x316, //板书大 老师全景 学生近景
	SWITCH_3_BLACKBOARD1_TEATHER_STUPANORAMA  = 0x317,//板书大 老师近景 学生全景
	SWITCH_3_BLACKBOARD1_TEACHPANORAMA_STUPANORAMA  = 0x318,//板书大 老师全景  学生全景

	SWITCH_3_BLACKBOARD2_TEATHER_STUPANORAMA  = 0x319,//板书大 老师近景 学生全景
	SWITCH_3_BLACKBOARD2_TEATHER_STU  = 0x31a,//板书大 老师近景  学生近景

	//4画面布局
	SWITCH_4_VGA_TEATHER_STU_TEACHPANORAMA  = 0x411,              //VGA大 老师近景 学生近景
	SWITCH_4_VGA_TEACHPANORAMA_STU_TEACHPANORAMA  = 0x412,        //VGA大 老师远景 学生近景
	SWITCH_4_VGA_TEATHER_STUPANORAMA_TEACHPANORAMA  = 0x413,              //VGA大 老师近景 学生远景
	SWITCH_4_VGA_TEACHPANORAMA_STUPANORAMA_TEACHPANORAMA  = 0x414,        //VGA大 老师远景 学生远景

	SWITCH_4_BLACKBOARD1_TEATHER_STU_TEACHPANORAMA  = 0x415,      		  //板书大 老师近景 学生近景
	SWITCH_4_BLACKBOARD1_TEATHER_STUPANORAMA_TEACHPANORAMA  = 0x416,      //板书大 老师近景 学生远景
	SWITCH_4_BLACKBOARD1_TEACHPANORAMA_STU_TEACHPANORAMA  = 0x417,      		  //板书大 老师远景 学生近景
	SWITCH_4_BLACKBOARD1_TEACHPANORAMA_STUPANORAMA_TEACHPANORAMA  = 0x418,      //板书大 老师远景 学生远景

	SWITCH_4_VGA_TEACHPER_STU_STUPANORAMA = 0x419,                      //VGA大 老师近景 学生近景 学生远景
	SWITCH_4_BLACKBOARD1_TEACHPER_STU_STUPANORAMA = 0x41a,                      //板书大 老师近景 学生近景 学生远景

	SWITCH_4_BLACKBOARD2_TEACHPER_STU_BLACKBOARD1 = 0x41b,                      //板书2大 老师近景 学生近景 板书1
	SWITCH_4_BLACKBOARD2_TEACHPER_STUPANORAMA_BLACKBOARD1 = 0x41c,               //板书2大 老师近景 学生远景 板书1
	
	//5画面布局
	SWITCH_5    = 0x511,

	//6画面布局
	SWITCH_6_1    = 0x611,  //六画面第一种
	SWITCH_6_2    = 0x621,  //六画面第二种

	SWITCH_NUM 
}switch_cmd_type_e;



/**
* @ 控制摄像头方式
*/
typedef enum _control_cam_mode
{
	AUTO_CONTROL = 0,		//自动控制
	MANUAL_CONTROL,			//手动控制
	MAX_NUM_CONTROL
}control_cam_mode_e;

/**
* @ 控制画线的类型
*/
typedef enum _draw_line_type
{
	DRAW_NO = 0,			//不画线
	DRAW_TRIGGER_TRACK,		//表示画跟踪框和触发框,限高线,和跟踪区域框
	DRAW_SLANT_LINE,		//画斜线表示画45度,135度,90度,180度线
	DRAW_MODEL,				//画模版
	DRAW_SKIN_COLOR,		//画检测区域的头像的肤色部分
	DRAW_TRACK_TRAJECTORY,	//画跟踪轨迹线
	DRAW_TRACK_DIRECT,		//画跟踪方向线
	DRAW_TRACK_SPEED,		//画跟踪速度线
	MAX_NUM_DRAW
}draw_line_type_e;

/**
* @	左右上下限位的的分类
*/
typedef enum _limit_position_type
{
	LIMIT_DOWN_LEFT = 0,
	LIMIT_UP_RIGHT,	
	LIMIT_CLEAR
}limit_position_type_e;



/**
* @ 发送数据消息头结构
*/
typedef struct _msg_header
{
	unsigned short	nLen;					//通过htons转换的值,包括结构体本身和数据
	unsigned short	nVer;					//版本号(暂不用)
	unsigned char	nMsg;					//消息类型
	unsigned char	szTemp[3];				//保留字节
} msg_header_t;

/**
* @ ENC110发送数据消息头结构
*/
typedef struct _msg_header_110
{
	unsigned char	nMsg;					//消息类型
	unsigned short	nLen;					//通过htons转换的值,包括结构体本身和数据
	
} msg_header_110_t;

/**
* @ 跟踪消息头结构
*/
typedef struct _track_header
{
	unsigned short len;
	unsigned short fixd_msg;		//固定消息字
	unsigned short msg_type;		//消息类型
	unsigned short reserve;			//预留
}track_header_t;

/**
* @	坐标点的描述 
*/
typedef struct _point_info
{
	unsigned short x;	//x坐标
	unsigned short y;	//y坐标
}point_info_t;

/**
* @	框的描述 
*/
typedef struct _rectangle_info
{
	unsigned short x;	//x坐标
	unsigned short y;	//y坐标
	unsigned short width;	//框的高
	unsigned short height;	//框的宽
}rectangle_info_t;



/**
* @	跟踪框信息
*/
typedef struct _track_range_info
{
	unsigned char state;						//启用或撤销操作
	unsigned char point_num;					//确定跟踪区域的点数
	unsigned short video_width;					//视频的宽
	unsigned short video_height;				//视频的高
	point_info_t point[TRACK_RANGE_NUM];		//跟踪框
}track_range_info_t;


/**
* @	meanshift or pos1检测框信息
*/
typedef struct _one_range_info
{
	unsigned char state;						//启用或撤销操作
	unsigned char point_num;					//确定跟踪区域的点数
	unsigned short video_width;					//视频的宽
	unsigned short video_height;				//视频的高
	point_info_t point[2];		//触发框
}one_range_info_t;


/**
* @	用于设置黑板区域，以及获取时间值等通用结构体
*/
typedef struct _default_msg_info
{
	unsigned char 	state;		//启用或撤销操作
	unsigned short 	value;	
}default_msg_info_t;


/**
* @	触发框信息
*/
typedef struct _stutrigger_range_info
{
	unsigned char state;						//启用或撤销操作
	unsigned char point_num;					//确定跟踪区域的点数
	unsigned short video_width;					//视频的宽
	unsigned short video_height;				//视频的高
	rectangle_info_t rectangle[STUTRACK_TRIGGER_NUM];		//跟踪框
}stutrigger_range_info_t;

/**
* @	触发框信息
*/
typedef struct _trigger_range_info
{
	unsigned char state;						//启用或撤销操作
	unsigned char point_num;					//确定跟踪区域的点数
	unsigned short video_width;					//视频的宽
	unsigned short video_height;				//视频的高
	point_info_t point[TRACK_TRIGGER_NUM];		//触发框
}trigger_range_info_t;

/**
* @	限高信息,在目标的y值大于此值时不在跟踪(即目标太矮)
*/
typedef struct _limit_height_info
{
	unsigned char 	state;			//启用或撤销操作
	unsigned short 	limit_height;	//限高高度
}limit_height_info_t;

/**
* @	自动或手动控制
*/
typedef struct _control_type_info
{
	unsigned char 	state;			//启用或撤销操作
	control_cam_mode_e 	control_type;	//控制类型0是自动控制,1为手动控制
}control_type_info_t;

/**
* @	画调试线的控制
*/
typedef struct _draw_line_info
{
	unsigned char 		state;		//启用或撤销操作
	draw_line_type_e 	message;	//message类型
}draw_line_info_t;

/**
* @	设置摄像头速度基数
*/
typedef struct _cam_speed_info
{
	unsigned char 		state;		//启用或撤销操作
	unsigned short 	cam_speed;		//摄像头速度基数,即用要移动的距离除以此值,则为摄像头速度
}cam_speed_info_t;

/**
* @	设置触发点总数
*/
typedef struct _trigger_num_info
{
	unsigned char 		state;		//启用或撤销操作
	unsigned short 	trigger_num;	//触发点总数,即触发区域检测到有这么多个点触发才认为是触发
}trigger_num_info_t;

/**
* @	设置预置位
*/
typedef struct _preset_position_info
{
	unsigned char 		state;		//启用或撤销操作
	unsigned short 		preset_position;//设置的预置位,范围从1到255
}preset_position_info_t;

/**
* @	设置限位开关
*/
typedef struct _limit_position_info
{
	unsigned char 			state;			//启用或撤销操作
	//limit_position_type_e 	limit_position;	//设置摄像头的上下左右的最大移动位置
	unsigned short 	limit_position;	//设置摄像头的上下左右的最大移动位置
}limit_position_info_t;

/**
* @	设置跟踪距离
*/
typedef struct _track_distance_info
{
	unsigned char 	state;			//启用或撤销操作
	unsigned short 	zoom_distance;	//近景的焦距远近选择,值越大,焦距拉的越远,即0为近距离跟踪,1为远距离跟踪
}track_distance_info_t;

/**
* @	设置跟踪机是否编码(协议中用的结构体)
*/
typedef struct _track_is_encode_info
{
	unsigned char 	state;			//启用或撤销操作
	unsigned short 	isencode;		//跟踪机是否编码的标志,0表示不编码,1表示编码
}track_is_encode_info_t;

/**
* @ 课堂信息统计结构体
*/
typedef struct _track_class_info
{
	unsigned short	nStandupPos[4];			//前面左  右，  后面左右分别对应起立次数
	unsigned short	nUpToPlatformTimes;			//学生上台总次数
	unsigned short	nDownToStudentsAreaTimes;//下台总次数
}track_class_info_t;

/**
* @ 课堂信息统计结构体.上报给web的信息结构体
*/
typedef struct _send_class_info
{
	unsigned short  nClassType;//授课类型
	unsigned short  nUpTimesSum;//起立总次数
	unsigned short	nStandupPos[4];			//前面左  右，  后面左右分别对应起立次数
	unsigned short	nUpToPlatformTimes;			//学生上台总次数
	unsigned short	nTeacherToStudentsAreaTimes;//老师下台总次数
	unsigned short  nPerTimeLen;//每个时间段长度
	unsigned short 	nAllTimesNum;//总共的时间断数
	unsigned short  nInteractionNum[MAX_INTERACTION_NUM];//分别是各个时间段的活跃次数
	unsigned short 	nStandupTimePoint[MAX_STANDUP_NUM];//分别是各个起立动作的起立时间点
}send_class_info_t;

/**
* @ 课堂信息统计结构体.上报给web的信息结构体
*/
typedef struct _final_class_info
{
	send_class_info_t tSendClassInfo;
	unsigned int nClassInfoFlag;
	unsigned long long nClassStartTime;
	unsigned long long nClassEndTime;
}final_class_info_t;

/**
* @ 保存各个预置位的信息
*/
typedef struct _cam_preset_position_info
{
	
	unsigned char pan_tilt[CAM_PAN_TILT_LEN];	//前四个字节为摄像头水平位置,后四个字节为摄像头垂直位置
	unsigned char zoom[CAM_ZOOM_LEN];		//摄像头zoom位置,共有四个字节描述
	int cur_preset_value;		//上次执行的预置位值
}cam_preset_position_info_t;

/**
* @	设置触发后开始跟踪时间
*/
typedef struct _start_track_time_info
{
	unsigned char 	state;				//启用或撤销操作
	unsigned short 	start_track_time;	//开始跟踪时间
}start_track_time_info_t;

/**
* @	设置设置丢失目标后重新开始触发的时间
*/
typedef struct _reset_time_info
{
	unsigned char 	state;				//启用或撤销操作
	unsigned short 	reset_time;			//复位时间
}reset_time_info_t;


/**
* @	设置检测变化的系数sens值
*/
typedef struct _sens_info
{
	unsigned char 	state;				//启用或撤销操作
	unsigned short 	sens;				//sens值
}sens_info_t;

/**
* @	移动摄像头类型
*/
typedef enum _move_cam_type
{
	MOVE_NOT = 0,		//不移动
	MOVE_ZOOM,			//移动焦距
	MOVE_PAN_TILT,		//移动位置
	MOVE_NUM
}move_cam_type_e;

/**
* @	跟踪有关的全局参数
*/
typedef struct _cam_control_info
{
	int		cam_last_speed;							//摄像头上次的移动速度
	camera_move_direction_e		cam_last_direction;	//摄像头上次移动方向
	unsigned short cam_speed;				//摄像头转动速度基数
	int		cam_position_value;				//预置位数
	move_cam_type_e control_flag;			//需要移动zoom为1,需要移动位置为2,都需要为3
	cam_preset_position_info_t	cam_position[PRESET_NUM_MAX];
}cam_control_info_t;



/**
* @	跟踪模式设置，是和原始图像比较还是和上帧图像比较
*/
typedef struct _stusdietrack_mode_info
{
	unsigned char 	state;			//启用或撤销操作
	int16_t 	track_mode;	//控制类型0是自动控制,1为手动控制
}stusidetrack_mode_info_t;

/**
* @	跟踪有关的全局参数
*/
typedef struct _stucam_control_info
{
	int		cam_last_speed;					//摄像头上次的移动速度
	camera_move_direction_e		cam_last_direction;	//摄像头上次移动方向
	unsigned short cam_speed;				//摄像头转动速度基数
	int		cam_position_value;				//预置位数
	move_cam_type_e control_flag;			//需要移动zoom为1,需要移动位置为2,都需要为3
	cam_preset_position_info_t	cam_position[PRESET_NUM_MAX];
	unsigned char	cam_addr;						//摄像机地址
}stucam_control_info_t;


/**
* @ 和编码有关的一些全局变量(保存全局变量中用到的结构体)
*/
typedef struct _track_encode_info
{
	short	is_encode;		//为0表示不编码,为1表示编码
	short	server_cmd;		//向服务器发送的命令,表示现在是切换学生机还老师机
	int		track_status;	//跟踪机状态0表示未跟踪上,1表示跟踪上
	int		zoom_pan_delay;	//跳转到绝对位置时，zoom和调上下左右位置时中间的间隔单位是ms
}track_encode_info_t;


/**
* @ 和编码有关的一些全局变量(保存全局变量中用到的结构体)
*/
typedef struct _stutrack_encode_info
{
	short	is_encode;			//为0表示不编码,为1表示编码
	short	server_cmd;			//向服务器发送的命令,表示现在是切换学生机还老师机
	int		track_status;		//跟踪机状态0表示未跟踪上,1表示跟踪上
	int		send_cmd;			//老师机要发送的命令
	short	last_position_no;	//上次预置位号
	int		is_track;			//是否跟踪1表示跟踪,0表示不跟踪
	unsigned short is_control_cam;//是否控制摄像机推近景，1为推近景，0为不推近景
	int		zoom_pan_delay;	//跳转到绝对位置时，zoom和调上下左右位置时中间的间隔单位是ms
	int		last_send_cmd;			//老师机上一次发送的命令
	int 	nTriggerValDelay;
	int 	nOnlyRightSideUpDelay;
	int 	nLastTriggerVal;
	int		nStandUpPos;
}stutrack_encode_info_t;

/**
* @ 和编码有关的一些全局变量(保存全局变量中用到的结构体)
*/
typedef struct _stusidetrack_encode_info
{
	short	is_encode;		//为0表示不编码,为1表示编码
	int		is_save_class_view;	//保存课前取景图片，1是要保存，0不需要保存
	int		students_status;	//学生站立状态，1为站立，2为坐下，0为不做任何操作
	int		last_students_status;	//上次学生站立状态，1为站立，2为坐下，0为不做任何操作
}stusidetrack_encode_info_t;


/**
* @	设置手动命令参数
*/
typedef struct _manual_commond_info
{
	unsigned char 	state;			//启用或撤销操作
	unsigned short 	type;			//手动命令中的子类型，大小为0-65535
	unsigned short 	value;			//手动命令中的值，大小为0-65535
}manual_commond_info_t;


/**
* @	设置参数类型，由于接收摄像机返回位置和焦距信息是在一个线程完成的，所以
* @  当收到位置信息时需要知道是哪个命令来获取位置信息的。
*/
typedef enum	_set_cmd_type
{
	SET_PRESET_POSITION = 1,		//设置预制位
	SET_LIMITE_POSITION = 2,		//设置限位
	GET_CUR_POSITION = 3,			//自动跟踪状态下获取摄像机位置，用于判断是否在半数去域
	SET_BLACKBOARD_POSITION = 4		//设置黑板区域命令
}set_cmd_type_e;

/**
* @ 跟踪设置软件上次设置命令信息
*/
typedef struct _track_save_file_info
{
	pthread_mutex_t save_track_m;	//用于修改跟踪机配置文件时加的锁
	set_cmd_type_e		set_cmd;	//设置软件设置命令
	int		cmd_param;				//命令的详细值
}track_save_file_info_t;

/**
* @	摄像机型号类型
*/
typedef enum _cam_type_info
{
	VISCA_STANDARD 	= 0,		//标准VISCA协议，包括日立的，博胜智达的摄像机
	SONY_BRC_Z330  	= 1, 		//SONY的BRC-Z330协议摄像机协议
	BAOLIN_CAM 		= 2,		//宝林摄像机支持
	CAM_NUM
}cam_type_info_e;

/**
* @ 摄像机型号选择 
*/
typedef struct _track_cam_model_info
{
	cam_type_info_e		cam_type;
	
}track_cam_model_info_t;


/**
* @ 学生辅助机上报的消息结构体
*/
typedef struct _rightside_trigger_info
{
	unsigned int	nTriggerType;//课前取景0/课中取景1			
	unsigned int	nTriggerVal;//1表示有触发，2表示坐下(只有课前取景有用)，0不处理
}rightside_trigger_info_t;



/**
* @	切换命令发起者类型
*/
typedef enum _switch_cmd_author_info
{
	AUTHOR_TEACHER	= 1,	//切换命令是老师发起的
	AUTHOR_STUDENTS	= 2,	//是学生机发送的切换命令
	AUTHOR_NUM
}switch_cmd_author_info_e;

/**
* @	多机位策略需求信息表
*/
typedef struct _track_strategy_info
{
	int	left_pan_tilt1;	//板书1左边	//前四个或者前五个字节为摄像头水平位置,后四个字节为摄像头垂直位置，表示黑板最左边对应摄像机位置
	int	right_pan_tilt1;//板书1右边		//前四个或者前五个字节为摄像头水平位置,后四个字节为摄像头垂直位置，表示黑板最右边对应摄像机位置
	int	left_pan_tilt2;	//板书2左边	//前四个或者前五个字节为摄像头水平位置,后四个字节为摄像头垂直位置，表示黑板最左边对应摄像机位置
	int	right_pan_tilt2;//板书2右边		//前四个或者前五个字节为摄像头水平位置,后四个字节为摄像头垂直位置，表示黑板最右边对应摄像机位置
	int cur_pan_tilt;		//前四个或者前五个字节为摄像头水平位置,后四个字节为摄像头垂直位置，表示摄像机当前所处的位置
	int	blackboard_region_flag1;//是否在板书区域标志，1为在板书区域，0为不在板书区域
	int	blackboard_region_flag2;//是否在板书区域标志，1为在板书区域，0为不在板书区域
	int	students_panorama_switch_near_time;		//学生站起来后，切学生全景摄像机，再切学生特写摄像机的时间，单位是秒
	int	teacher_blackboard_time1;				//老师停留在黑板区域的时间大于这个时间即认为是老师在黑板区域，单位是秒
	int	teacher_leave_blackboard_time1;			//老师离开黑板区域的时间大于这个时间即认为是老师不在黑板区域，单位是秒
	int	teacher_blackboard_time2;				//老师停留在黑板区域的时间大于这个时间即认为是老师在黑板区域，单位是秒
	int	teacher_leave_blackboard_time2;			//老师离开黑板区域的时间大于这个时间即认为是老师不在黑板区域，单位是秒
	int	teacher_panorama_time;					//老师移动后多长时间切换老师全景，单位是秒
	int	teacher_leave_panorama_time;			//老师停下来多久后切换到老师特写，单位是秒
	int	teacher_keep_panorama_time;				//老师全景镜头最大保留时间，单位是秒
	int	move_flag;								//移动标志,老师跟踪机摄像头移动标志
	int	send_cmd;		//老师机当前要发送的命令
	int	last_send_cmd;		//上一次老师机发送的命令
	switch_cmd_author_info_e	switch_cmd_author;			//切换命令发起者是老师机还是学生机
	int teacher_panorama_flag;//老师全景,表示学生上讲台了,多目标检测时用
	int	students_down_time;			//学生下讲台时间大于这个时间即再次接收学生起立信息，单位是秒
	int	students_track_flag;			//是否接收学生起立以及上台信息标志，单位是秒
	int switch_vga_flag;//是否切换vga
	int teacher_switch_students_delay_time;//老师镜头切换到学生镜头延时
	int	students_near_keep_time;			//学生近景镜头保持时间
	int	vga_keep_time;//vga 保持时间,超过这个时间,就由vga镜头切换到别的镜头
	int cam_left_limit;//左限位
	int cam_right_limit;//右限位
	int position1_mv_flag;//发现老师丢了后有移动目标
	int mv_keep_time;//老师丢了，发现移动目标时学生全景和老师全景镜头切换延时
	int strategy_no;//策略序号，两机位为0，四机位为1，五机位为2
}track_strategy_info_t;

/**
* @	多机位策略需求信息表,用于统计 
*/
typedef struct _track_strategy_timeinfo
{
	int	last_strategy_no;//上一次的机位号,如果跟当前不一样会做相关初始化变量操作
	int	students_panorama_switch_near_time;//学生全景切换学生前的累积时间,这个时间如果超过策略的延时就切学生近景
	int	blackboard_time1;//老师走入板书区域1的时间,如果只有一个板书区域,则板书区域1生效
	int	leave_blackboard_time1;//老师离开板书区域1的时间,如果只有一个板书区域,则板书区域1生效
	int	blackboard_time2;//老师走入板书2区域时间
	int	leave_blackboard_time2;//老师离开板书2区域时间
	int	panorama_time;//5机位,老师全景时间
	int	leave_panorama_time;//5机位,离开老师全景时间
	
	int	teacher_panorama_switch_near_time;//老师全景切换到老师近景之前的时间,时间统计到了策略设置的时间就切换老师近景
	int teacher_panorama_flag;//当前镜头是否是老师全景
	int	students_down_time;//学生下讲台的时间统计
	int	students_to_platform_time;//学生上讲台的时间统计
	int	switch_flag;// 1可以切换,0 延时未到 不可以切换
	int	switch_delaytime;//标准延时时间,3s
	int	delaytime;//每次切换的延时时间,根据不同切换镜头,得到的延时时间也不一致
	int	on_teacher_flag;//当前镜头是否在老师上,包括老师和板书和老师全景,用于老师切学生时的延时处理
	int	on_teacher_delaytime;//老师镜头的保持时间
	int	vga_delaytime;//vga的时间
	int	mv_time;//老师丢失时,发现运动目标的时间
	int	stu_time;//老师丢失时,切换到学生全景的时间
}track_strategy_timeinfo_t;


/**
* @	多目标上台检测框信息
*/
typedef struct _multitarget_range_info
{
	unsigned char state;							//启用或撤销操作
	unsigned char point_num;						//确定触发区域的点数
	unsigned short video_width;						//视频的宽
	unsigned short video_height;					//视频的高
	rectangle_info_t rectangle[STUDENTS_MULTITARGET_NUM];		//跟踪框
}multitarget_range_info_t;


/**
* @	屏蔽框信息
*/
typedef struct _shield_range_info
{
	unsigned char state;							//启用或撤销操作
	unsigned char point_num;						//确定触发区域的点数
	unsigned short video_width;						//视频的宽
	unsigned short video_height;					//视频的高
	rectangle_info_t rectangle[STUDENTS_SHIELD_NUM];		//跟踪框
}shield_range_info_t;


extern int track_init(ITRACK_Params *track_param);
extern int stutrack_init(StuITRACK_Params *track_param);
extern int stusidetrack_init(StuSideITRACK_Params *track_param);

extern int cam_ctrl_cmd(ITRACK_OutArgs *output_param);
extern int stucam_ctrl_cmd(StuITRACK_OutArgs *output_param);

extern int set_teacher_track_param(unsigned char *data, int socket);
extern int set_students_track_param(unsigned char *data, int socket);
extern int track_students_right_side_param(unsigned char *data, int socket);

extern int write_track_static_file(track_static_param_t *static_param);
extern int stuwrite_track_static_file(stutrack_static_param_t *static_param);

extern int server_set_track_type(short type);
extern int server_set_stutrack_type(short type);
extern int server_set_stusidetrack_type(short type);

extern int save_position_zoom(unsigned char *data);
extern int stusave_position_zoom(unsigned char *data);

extern int init_save_track_mutex(void);
extern int init_save_stutrack_mutex(void);
extern int init_save_stusidetrack_mutex(void);

extern int destroy_save_track_mutex(void);
extern int destroy_save_stutrack_mutex(void);
extern int destroy_save_stusidetrack_mutex(void);

extern  int __stucall_preset_position(short position);
extern	int __call_preset_position(int preset_position);
extern	int		__get_cam_position(void);
extern  Int32 SetVgaState();

#endif

