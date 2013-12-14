
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
* @	�ļ����Ƶĳ���
*/
#define FILE_NAME_LEN		(64)

/**
* @ ��ʦ���ٲ��������ļ�
*/
#define TEACH_TRACK_FILE		"teach_track.ini"

/**
* @ ѧ�����ٲ��������ļ�
*/
#define STUDENTS_TRACK_FILE		"students_track.ini"

/**
* @ ��ʦ���ٲ��������ļ�
*/
#define TRACK_STUDENTS_RIGHT_SIDE_FILE		"track_students_right_side.ini"

/**
* @	�����ǰȡ����ͼƬ·��
*/
#define	CLASS_VIEW_JPG	"class_view.gpg"


#define DEBUG_INFO "debug_info"
#define DEBUG_IPADDR "debug_ipaddr"


/**
* @	���������λ����ռ���ֽ�����ͨVISCAռ��9���ֽڣ�����BRC-Z330ռ��8���ֽ�
*/
#define	CAM_PAN_TILT_LEN	(9)

/**
* @	��������Խ�����ռ�ֽ���Ϊ4���ֽ�
*/
#define	CAM_ZOOM_LEN		(4)




/**
* @	Ԥ��λ��������
*/
#define PRESET_NUM_MAX					(60)


/**
* @	�̶���Ϣ��
*/
#define FIXED_MSG						(0xFFF0)


//==============================���ò���============================
/**
* @	�趨���ٷ�Χ����Ϣ��
*/
#define SET_TRACK_RANGE					(0x0001)

/**
* @	�趨����������Ϣ��
*/
#define SET_TRIGGER_RANGE				(0x0002)

/**
* @	������ͷ��ĸ����ߵĸ�				
*/
#define	SET_TRACK_LIMIT_HEIGHT			(0x0003)

/**
* @	����������Ϣ�� 
*/
#define SET_TRACK_TYPE					(0x0004)


/**
* @	���û�������,������,����ٿ���,����������,�켣�ߵ�
*/
#define SET_DRAW_LINE_TYPE				(0x0005)

/**
* @	��������ͷ�����ٶȵĻ�����Ϣ�� 
*/
#define SET_CAM_SPEED_TYPE				(0x0006)

/**
* @	���ô�����������Ϣ�� 
*/
#define SET_TRIGGER_NUM_TYPE			(0x0007)

/**
* @	����Ԥ��λ��Ϣ�� 
*/
#define SET_PRESET_POSITION_TYPE		(0x0008)

/**
* @	��������ͷ����������λ��Ϣ�� 
*/
#define SET_LIMIT_POSITION_TYPE			(0x0009)

/**
* @	����Ԥ��λ��Ϣ��
*/
#define CALL_PRESET_POSITION_TYPE		(0x000a)

/**
* @	���ø��پ���
*/
#define SET_TRACK_DISTANCE_TYPE			(0x000b)

/**
* @	���ø��ٻ��Ƿ����Ŀ���
*/
#define SET_TRACK_IS_ENCODE				(0x000c)

/**
* @ ������ٲ�����Ϣ��
*/
#define SET_TRACK_PARAM					(0x000d)

/**
* @ ����Ԥ����ʱ��
*/
#define SET_START_TRACK_TIME			(0x000e)

/**
* @ ���ø��ٸ�λʱ��
*/
#define SET_RESET_TIME					(0x000f)


/**
* @ ���ü��仯ϵ��sensֵ
*/
#define SET_SENS_VALUE					(0x0010)

/**
* @ ������������
*/
#define SET_SHIELD_RANGE				(0x0011)

/**
* @ �����ֶ�����
*/
#define SET_MANUAL_COMMOND				(0x0012)

/**
* @ ���ö�Ŀ����̨����
*/
#define SET_MULTITARGET_RANGE				(0x0013)


/**
* @ ����Һ���������췶Χ
*/
#define SET_MEANSHIFT_TRIGGER			(0x0014)

/**
* @ ����ȫ��������Χ
*/
#define SET_POS1_TRIGGER				(0x0015)

/**
* @ ����ȫ������������
*/
#define SET_POS1_TRIGGER_SUM			(0x0016)

/**
* @ ���ð�������
*/
#define SET_BLACKBOARD_AREA				(0x0017)

/**
* @ ���û�λ
*/
#define SET_STRATEGY_NO					(0x0018)

/**
* @ ����Һ����ʾ���������ٿ���
*/
#define SET_MEANSHIFT_FLAG				(0x0019)

/**
* @ ����ѧ��ȫ������ʱ��
*/
#define SET_STUDENTS_PANORAMA_SWITCH_NEAR_TIME				(0x001a)

/**
* @ ������ʦ�ߵ�����1�����а����ʱ��
*/
#define SET_TEACHER_BLACKBOARD_TIME1				(0x001b)

/**
* @ �����뿪����1��������ʦ��ʱ��
*/
#define SET_TEACHER_LEAVE_BLACKBOARD_TIME1				(0x001c)

/**
* @ ����ѧ���½�̨�����ø��ٵ�ʱ��
*/
#define SET_STUDENTS_DOWN_TIME				(0x001d)

/**
* @ ������ʦ��������ͷ�ƶ��л���ʦȫ����ʱ��
*/
#define SET_TEACHER_PANORAMA_TIME				(0x001e)

/**
* @ ������ʦ����ͷֹͣ�ƶ��л���ʦ������ʱ��
*/
#define SET_TEACHER_LEAVE_PANORAMA_TIME				(0x001f)

/**
* @ ����ѧ���Ͻ�̨�󱣳���ʦȫ�����ʱ��
*/
#define SET_TEACHER_KEEP_PANORAMA_TIME				(0x0020)

/**
* @ ������ʦ�л�Ϊѧ��ǰ����ʦ��ͷ����ʱ��
*/
#define SET_TEACHER_SWITCH_STUDENTS_DELAY_TIME				(0x0021)

/**
* @ ����ѧ���������ֵ�ʱ��
*/
#define SET_STUDENTS_NEAR_KEEP_TIME				(0x0022)

/**
* @ ������ʦ����ʱ��ʦȫ����ѧ��ȫ���л���ʱ
*/
#define SET_MV_KEEP_TIME				(0x0023)

/**
* @ ����vga����ʱ��
*/
#define SET_VGA_KEEP_TIME				(0x0024)

/**
* @ ������ʦ�ߵ�����2�����а����ʱ��
*/
#define SET_TEACHER_BLACKBOARD_TIME2				(0x0025)

/**
* @ �����뿪����2��������ʦ��ʱ��
*/
#define SET_TEACHER_LEAVE_BLACKBOARD_TIME2				(0x0026)




//-----------------------------���ز�����Ϣ��-------------------------

/**
* @	��ȡ���ٷ�Χ����Ϣ��
*/
#define GET_TRACK_RANGE					(0x0100)

/**
* @	��ȡ����������Ϣ��
*/
#define GET_TRIGGER_RANGE				(0x0101)

/**
* @	��ȡ��ͷ��ĸ����ߵĸ�				
*/
#define	GET_TRACK_LIMIT_HEIGHT			(0x0102)

/**
* @	��ȡ��������				
*/
#define	GET_TRACK_TYPE					(0x0103)

/**
* @	��ȡ��������				
*/
#define	GET_DRAW_LINE_TYPE				(0x0104)

/**
* @	��ȡ����ͷ�ٶȻ���			
*/
#define	GET_CAM_SPEED_TYPE				(0x0105)

/**
* @	��ȡ����������			
*/
#define	GET_TRIGGER_NUM_TYPE			(0x0106)

/**
* @	��ȡ���پ���			
*/
#define	GET_TRACK_DISTANCE_TYPE			(0x0107)

/**
* @	��ȡ���ٻ��Ƿ����Ŀ���
*/
#define GET_TRACK_IS_ENCODE				(0x0108)

/**
* @ �������ٲ�����Ϣ��
*/
#define GET_TRACK_PARAM					(0x0109)


/**
* @ ��ȡԤ����ʱ��
*/
#define GET_START_TRACK_TIME			(0x010a)

/**
* @ ��ȡ���ٸ�λʱ��
*/
#define GET_RESET_TIME					(0x010b)

/**
* @ ��ȡ���仯ϵ��sensֵ
*/
#define GET_SENS_VALUE					(0x010c)

/**
* @ ��ȡ��������
*/
#define GET_SHIELD_RANGE				(0x010d)

/**
* @ ��ȡ��Ŀ����̨����
*/
#define GET_MULTITARGET_RANGE				(0x010e)


/**
* @ ��ȡҺ���������췶Χ
*/
#define GET_MEANSHIFT_TRIGGER			(0x010F)

/**
* @ ��ȡȫ��������Χ
*/
#define GET_POS1_TRIGGER				(0x0110)

/**
* @ ��ȡȫ������������
*/
#define GET_POS1_TRIGGER_NUM			(0x0111)


/**
* @��ȡ��λ
*/
#define GET_STRATEGY_NO					(0x0112)

/**
* @��ȡҺ����ʾ���������ٿ���
*/
#define GET_MEANSHIFT_FLAG				(0x0113)

/**
* @ ��ȡѧ��ȫ������ʱ��
*/
#define GET_STUDENTS_PANORAMA_SWITCH_NEAR_TIME				(0x0114)

/**
* @��ȡ��ʦ�ߵ�����1�����а����ʱ��
*/
#define GET_TEACHER_BLACKBOARD_TIME1				(0x0115)

/**
* @ ��ȡ�뿪����1��������ʦ��ʱ��
*/
#define GET_TEACHER_LEAVE_BLACKBOARD_TIME1				(0x0116)

/**
* @ ��ȡѧ���½�̨�����ø��ٵ�ʱ��
*/
#define GET_STUDENTS_DOWN_TIME				(0x0117)

/**
* @ ��ȡ��ʦ��������ͷ�ƶ��л���ʦȫ����ʱ��
*/
#define GET_TEACHER_PANORAMA_TIME				(0x0118)

/**
* @ ��ȡ��ʦ����ͷֹͣ�ƶ��л���ʦ������ʱ��
*/
#define GET_TEACHER_LEAVE_PANORAMA_TIME				(0x0119)

/**
* @ ��ȡѧ���Ͻ�̨�󱣳���ʦȫ�����ʱ��
*/
#define GET_TEACHER_KEEP_PANORAMA_TIME				(0x011A)

/**
* @��ȡ��ʦ�л�Ϊѧ��ǰ����ʦ��ͷ����ʱ��
*/
#define GET_TEACHER_SWITCH_STUDENTS_DELAY_TIME				(0x011B)

/**
* @ ��ȡѧ���������ֵ�ʱ��
*/
#define GET_STUDENTS_NEAR_KEEP_TIME				(0x011C)

/**
* @ ��ȡ��ʦ����ʱ��ʦȫ����ѧ��ȫ���л���ʱ
*/
#define GET_MV_KEEP_TIME				(0x011D)

/**
* @��ȡvga����ʱ��
*/
#define GET_VGA_KEEP_TIME				(0x011E)


/**
* @��ȡ��ʦ�ߵ�����2�����а����ʱ��
*/
#define GET_TEACHER_BLACKBOARD_TIME2				(0x011f)

/**
* @ ��ȡ�뿪����2��������ʦ��ʱ��
*/
#define GET_TEACHER_LEAVE_BLACKBOARD_TIME2				(0x0120)


//======================================================


#define	MSG_SET_STUSIDETRACK_PARAM	0xA2	//��ʦ���ٲ�������

//-------------------�����ֶ������������---------------------

typedef enum manualMANUAL_COMMAND_ENUM
{
/**
* @	�����Ƿ�����ѧ���������������
*/
	SET_IS_CONTROL_CAM	=	0x01,


/**
* @	����Ԥ��λʱ������ת�����Խ���;���ˮƽ����ֱλ��ʱ��ʱ��������
* @  ��������brc-z330������ڵ���һ����������ȴ�ִ����ɺ󣬲��ܷ��ڶ������
* @  ����ڶ����������ִ�С�
*/
	SET_ZOOM_PAN_DELAY	=	0x02,

/**
* @	�������úڰ����������ұ�������������ֵΪ0��ʾ��ߣ�Ϊ1��ʾ�ұ� 
*/
	SET_BLACKBOARD_REGION	= 0x03
	
}manualMANUAL_COMMAND_e;

/**
* @	����Ծʱ�����
*/
#define MAX_INTERACTION_NUM					(20)

/**
* @	�����������
*/
#define MAX_STANDUP_NUM					(50)


/**
* @	ÿ��ͳ��ʱ��εĳ��ȣ����ӱ�ʾ
*/
#define CLASSINFO_PER_TIME              (15)

/**
* @	�����Ƿ���ѧ���������������
*/
#define SET_IS_CONTROL_CAM		0x01

#define	PUSH_CLOSE_RANGE		(1)		//�ƽ���
#define NOT_PUSH_CLOSE_RANGE	(0)		//���ƽ���


/**
* @	����ͳ�ƿ�����Ϣ
*/
#define SET_IS_GET_CLASSINFO		0x02
#define NO_CLASS_INFO	(0)		//��ͳ�ƿ�����Ϣ
#define	FIND_CLASS_INFO		(1)		//ͳ�ƿ�����Ϣ

/**
* @	ȷ�����ٿ��ĸ���
*/
#define TRACK_RANGE_NUM					(10)

/**
* @	ȷ���������ĸ���
*/
#define TRACK_TRIGGER_NUM				(4)
#define STUTRACK_TRIGGER_NUM			(50)


#define STUDENTS_UP		(1)		//ѧ��վ����
#define	STUDENTS_DOWN	(2)		//ѧ������

typedef enum _switch_cmd_type
{
	//1���沼��
	SWITCH_TEATHER = 1,		//�л���ʦ������
	SWITCH_STUDENTS= 3,		//�л�ѧ��������
	SWITCH_VGA					= 255,		//�л�VGA����
	SWITCH_BLACKBOARD1			= 4,		//�л��������������
	SWITCH_STUDENTS_PANORAMA 	= 5,		//�л�ѧ��ȫ�������
	SWITCH_TEACHER_PANORAMA		= 6,		//�л���̨�����ȫ��
	SWITCH_BLACKBOARD2			= 6,        //����2 �� ��ʦȫ��������

	//2���沼��
    SWITCH_2_VGA_TEATHER        = 0x211,        //VGA�� ��ʦС ����
    SWITCH_2_TEATHER_STU        = 0x213,        //��ʦ�� ѧ��С ����
    SWITCH_2_STU_TEATHER        = 0x214,        //ѧ���� ��ʦС ����
    SWITCH_2_STUPANORAMA_TEATHER    = 0x215,        //ѧ��ȫ�� ��ʦ ����
	
    SWITCH_2_TEATHER_AND_VGA    = 0x221,        //��ʦ�� VGA��       �ȷ�
    SWITCH_2_STU_AND_TEATHER    = 0x222,        //ѧ���� ��ʦ��      �ȷ�
    SWITCH_2_STUPANORAMA_AND_TEATHER    = 0x223, //ѧ��ȫ���� ��ʦ�� �ȷ� 
    SWITCH_2_TEATHER_AND_STU    = 0x224,         //��ʦ�� ѧ����     �ȷ�

	SWITCH_2_VGA_TEATHER_1      = 0x231,        //VGA�� ��ʦС ����
	SWITCH_2_STUPANORAMA_TEATHER_1    = 0x232,        //ѧ��ȫ�� ��ʦС ����
	SWITCH_2_STU_TEATHER_1      = 0x233,        //ѧ���� ��ʦС ����                  ///

	SWITCH_2_VGA_TEATHER_2      = 0x241,           //VGA�� ��ʦ�� ����
	SWITCH_2_STU_TEATHER_2      = 0x242,          //ѧ���� ��ʦ����
	SWITCH_2_STUPANORAMA_TEATHER_2      = 0x243,          //ѧ��ȫ�� ��ʦ����

	//3���沼��
	SWITCH_3_VGA_TEATHER_STU    = 0x311,         //VGA�� ��ʦ���� ѧ������
	SWITCH_3_VGA_TEACHPANORAMA_STU    = 0x312,    //VGA�� ��ʦȫ�� ѧ������
	SWITCH_3_VGA_TEATHER_STUPANORAMA  = 0x313,    //VGA�� ��ʦ���� ѧ��ȫ��
	SWITCH_3_VGA_TEACHPANORAMA_STUPANORAMA  = 0x314,  //VGA�� ��ʦȫ�� ѧ��ȫ��
	
	SWITCH_3_BLACKBOARD1_TEATHER_STU  = 0x315,   //����� ��ʦ���� ѧ������
	SWITCH_3_BLACKBOARD1_TEACHPANORAMA_STU  = 0x316, //����� ��ʦȫ�� ѧ������
	SWITCH_3_BLACKBOARD1_TEATHER_STUPANORAMA  = 0x317,//����� ��ʦ���� ѧ��ȫ��
	SWITCH_3_BLACKBOARD1_TEACHPANORAMA_STUPANORAMA  = 0x318,//����� ��ʦȫ��  ѧ��ȫ��

	SWITCH_3_BLACKBOARD2_TEATHER_STUPANORAMA  = 0x319,//����� ��ʦ���� ѧ��ȫ��
	SWITCH_3_BLACKBOARD2_TEATHER_STU  = 0x31a,//����� ��ʦ����  ѧ������

	//4���沼��
	SWITCH_4_VGA_TEATHER_STU_TEACHPANORAMA  = 0x411,              //VGA�� ��ʦ���� ѧ������
	SWITCH_4_VGA_TEACHPANORAMA_STU_TEACHPANORAMA  = 0x412,        //VGA�� ��ʦԶ�� ѧ������
	SWITCH_4_VGA_TEATHER_STUPANORAMA_TEACHPANORAMA  = 0x413,              //VGA�� ��ʦ���� ѧ��Զ��
	SWITCH_4_VGA_TEACHPANORAMA_STUPANORAMA_TEACHPANORAMA  = 0x414,        //VGA�� ��ʦԶ�� ѧ��Զ��

	SWITCH_4_BLACKBOARD1_TEATHER_STU_TEACHPANORAMA  = 0x415,      		  //����� ��ʦ���� ѧ������
	SWITCH_4_BLACKBOARD1_TEATHER_STUPANORAMA_TEACHPANORAMA  = 0x416,      //����� ��ʦ���� ѧ��Զ��
	SWITCH_4_BLACKBOARD1_TEACHPANORAMA_STU_TEACHPANORAMA  = 0x417,      		  //����� ��ʦԶ�� ѧ������
	SWITCH_4_BLACKBOARD1_TEACHPANORAMA_STUPANORAMA_TEACHPANORAMA  = 0x418,      //����� ��ʦԶ�� ѧ��Զ��

	SWITCH_4_VGA_TEACHPER_STU_STUPANORAMA = 0x419,                      //VGA�� ��ʦ���� ѧ������ ѧ��Զ��
	SWITCH_4_BLACKBOARD1_TEACHPER_STU_STUPANORAMA = 0x41a,                      //����� ��ʦ���� ѧ������ ѧ��Զ��

	SWITCH_4_BLACKBOARD2_TEACHPER_STU_BLACKBOARD1 = 0x41b,                      //����2�� ��ʦ���� ѧ������ ����1
	SWITCH_4_BLACKBOARD2_TEACHPER_STUPANORAMA_BLACKBOARD1 = 0x41c,               //����2�� ��ʦ���� ѧ��Զ�� ����1
	
	//5���沼��
	SWITCH_5    = 0x511,

	//6���沼��
	SWITCH_6_1    = 0x611,  //�������һ��
	SWITCH_6_2    = 0x621,  //������ڶ���

	SWITCH_NUM 
}switch_cmd_type_e;



/**
* @ ��������ͷ��ʽ
*/
typedef enum _control_cam_mode
{
	AUTO_CONTROL = 0,		//�Զ�����
	MANUAL_CONTROL,			//�ֶ�����
	MAX_NUM_CONTROL
}control_cam_mode_e;

/**
* @ ���ƻ��ߵ�����
*/
typedef enum _draw_line_type
{
	DRAW_NO = 0,			//������
	DRAW_TRIGGER_TRACK,		//��ʾ�����ٿ�ʹ�����,�޸���,�͸��������
	DRAW_SLANT_LINE,		//��б�߱�ʾ��45��,135��,90��,180����
	DRAW_MODEL,				//��ģ��
	DRAW_SKIN_COLOR,		//����������ͷ��ķ�ɫ����
	DRAW_TRACK_TRAJECTORY,	//�����ٹ켣��
	DRAW_TRACK_DIRECT,		//�����ٷ�����
	DRAW_TRACK_SPEED,		//�������ٶ���
	MAX_NUM_DRAW
}draw_line_type_e;

/**
* @	����������λ�ĵķ���
*/
typedef enum _limit_position_type
{
	LIMIT_DOWN_LEFT = 0,
	LIMIT_UP_RIGHT,	
	LIMIT_CLEAR
}limit_position_type_e;



/**
* @ ����������Ϣͷ�ṹ
*/
typedef struct _msg_header
{
	unsigned short	nLen;					//ͨ��htonsת����ֵ,�����ṹ�屾�������
	unsigned short	nVer;					//�汾��(�ݲ���)
	unsigned char	nMsg;					//��Ϣ����
	unsigned char	szTemp[3];				//�����ֽ�
} msg_header_t;

/**
* @ ENC110����������Ϣͷ�ṹ
*/
typedef struct _msg_header_110
{
	unsigned char	nMsg;					//��Ϣ����
	unsigned short	nLen;					//ͨ��htonsת����ֵ,�����ṹ�屾�������
	
} msg_header_110_t;

/**
* @ ������Ϣͷ�ṹ
*/
typedef struct _track_header
{
	unsigned short len;
	unsigned short fixd_msg;		//�̶���Ϣ��
	unsigned short msg_type;		//��Ϣ����
	unsigned short reserve;			//Ԥ��
}track_header_t;

/**
* @	���������� 
*/
typedef struct _point_info
{
	unsigned short x;	//x����
	unsigned short y;	//y����
}point_info_t;

/**
* @	������� 
*/
typedef struct _rectangle_info
{
	unsigned short x;	//x����
	unsigned short y;	//y����
	unsigned short width;	//��ĸ�
	unsigned short height;	//��Ŀ�
}rectangle_info_t;



/**
* @	���ٿ���Ϣ
*/
typedef struct _track_range_info
{
	unsigned char state;						//���û�������
	unsigned char point_num;					//ȷ����������ĵ���
	unsigned short video_width;					//��Ƶ�Ŀ�
	unsigned short video_height;				//��Ƶ�ĸ�
	point_info_t point[TRACK_RANGE_NUM];		//���ٿ�
}track_range_info_t;


/**
* @	meanshift or pos1������Ϣ
*/
typedef struct _one_range_info
{
	unsigned char state;						//���û�������
	unsigned char point_num;					//ȷ����������ĵ���
	unsigned short video_width;					//��Ƶ�Ŀ�
	unsigned short video_height;				//��Ƶ�ĸ�
	point_info_t point[2];		//������
}one_range_info_t;


/**
* @	�������úڰ������Լ���ȡʱ��ֵ��ͨ�ýṹ��
*/
typedef struct _default_msg_info
{
	unsigned char 	state;		//���û�������
	unsigned short 	value;	
}default_msg_info_t;


/**
* @	��������Ϣ
*/
typedef struct _stutrigger_range_info
{
	unsigned char state;						//���û�������
	unsigned char point_num;					//ȷ����������ĵ���
	unsigned short video_width;					//��Ƶ�Ŀ�
	unsigned short video_height;				//��Ƶ�ĸ�
	rectangle_info_t rectangle[STUTRACK_TRIGGER_NUM];		//���ٿ�
}stutrigger_range_info_t;

/**
* @	��������Ϣ
*/
typedef struct _trigger_range_info
{
	unsigned char state;						//���û�������
	unsigned char point_num;					//ȷ����������ĵ���
	unsigned short video_width;					//��Ƶ�Ŀ�
	unsigned short video_height;				//��Ƶ�ĸ�
	point_info_t point[TRACK_TRIGGER_NUM];		//������
}trigger_range_info_t;

/**
* @	�޸���Ϣ,��Ŀ���yֵ���ڴ�ֵʱ���ڸ���(��Ŀ��̫��)
*/
typedef struct _limit_height_info
{
	unsigned char 	state;			//���û�������
	unsigned short 	limit_height;	//�޸߸߶�
}limit_height_info_t;

/**
* @	�Զ����ֶ�����
*/
typedef struct _control_type_info
{
	unsigned char 	state;			//���û�������
	control_cam_mode_e 	control_type;	//��������0���Զ�����,1Ϊ�ֶ�����
}control_type_info_t;

/**
* @	�������ߵĿ���
*/
typedef struct _draw_line_info
{
	unsigned char 		state;		//���û�������
	draw_line_type_e 	message;	//message����
}draw_line_info_t;

/**
* @	��������ͷ�ٶȻ���
*/
typedef struct _cam_speed_info
{
	unsigned char 		state;		//���û�������
	unsigned short 	cam_speed;		//����ͷ�ٶȻ���,����Ҫ�ƶ��ľ�����Դ�ֵ,��Ϊ����ͷ�ٶ�
}cam_speed_info_t;

/**
* @	���ô���������
*/
typedef struct _trigger_num_info
{
	unsigned char 		state;		//���û�������
	unsigned short 	trigger_num;	//����������,�����������⵽����ô����㴥������Ϊ�Ǵ���
}trigger_num_info_t;

/**
* @	����Ԥ��λ
*/
typedef struct _preset_position_info
{
	unsigned char 		state;		//���û�������
	unsigned short 		preset_position;//���õ�Ԥ��λ,��Χ��1��255
}preset_position_info_t;

/**
* @	������λ����
*/
typedef struct _limit_position_info
{
	unsigned char 			state;			//���û�������
	//limit_position_type_e 	limit_position;	//��������ͷ���������ҵ�����ƶ�λ��
	unsigned short 	limit_position;	//��������ͷ���������ҵ�����ƶ�λ��
}limit_position_info_t;

/**
* @	���ø��پ���
*/
typedef struct _track_distance_info
{
	unsigned char 	state;			//���û�������
	unsigned short 	zoom_distance;	//�����Ľ���Զ��ѡ��,ֵԽ��,��������ԽԶ,��0Ϊ���������,1ΪԶ�������
}track_distance_info_t;

/**
* @	���ø��ٻ��Ƿ����(Э�����õĽṹ��)
*/
typedef struct _track_is_encode_info
{
	unsigned char 	state;			//���û�������
	unsigned short 	isencode;		//���ٻ��Ƿ����ı�־,0��ʾ������,1��ʾ����
}track_is_encode_info_t;

/**
* @ ������Ϣͳ�ƽṹ��
*/
typedef struct _track_class_info
{
	unsigned short	nStandupPos[4];			//ǰ����  �ң�  �������ҷֱ��Ӧ��������
	unsigned short	nUpToPlatformTimes;			//ѧ����̨�ܴ���
	unsigned short	nDownToStudentsAreaTimes;//��̨�ܴ���
}track_class_info_t;

/**
* @ ������Ϣͳ�ƽṹ��.�ϱ���web����Ϣ�ṹ��
*/
typedef struct _send_class_info
{
	unsigned short  nClassType;//�ڿ�����
	unsigned short  nUpTimesSum;//�����ܴ���
	unsigned short	nStandupPos[4];			//ǰ����  �ң�  �������ҷֱ��Ӧ��������
	unsigned short	nUpToPlatformTimes;			//ѧ����̨�ܴ���
	unsigned short	nTeacherToStudentsAreaTimes;//��ʦ��̨�ܴ���
	unsigned short  nPerTimeLen;//ÿ��ʱ��γ���
	unsigned short 	nAllTimesNum;//�ܹ���ʱ�����
	unsigned short  nInteractionNum[MAX_INTERACTION_NUM];//�ֱ��Ǹ���ʱ��εĻ�Ծ����
	unsigned short 	nStandupTimePoint[MAX_STANDUP_NUM];//�ֱ��Ǹ�����������������ʱ���
}send_class_info_t;

/**
* @ ������Ϣͳ�ƽṹ��.�ϱ���web����Ϣ�ṹ��
*/
typedef struct _final_class_info
{
	send_class_info_t tSendClassInfo;
	unsigned int nClassInfoFlag;
	unsigned long long nClassStartTime;
	unsigned long long nClassEndTime;
}final_class_info_t;

/**
* @ �������Ԥ��λ����Ϣ
*/
typedef struct _cam_preset_position_info
{
	
	unsigned char pan_tilt[CAM_PAN_TILT_LEN];	//ǰ�ĸ��ֽ�Ϊ����ͷˮƽλ��,���ĸ��ֽ�Ϊ����ͷ��ֱλ��
	unsigned char zoom[CAM_ZOOM_LEN];		//����ͷzoomλ��,�����ĸ��ֽ�����
	int cur_preset_value;		//�ϴ�ִ�е�Ԥ��λֵ
}cam_preset_position_info_t;

/**
* @	���ô�����ʼ����ʱ��
*/
typedef struct _start_track_time_info
{
	unsigned char 	state;				//���û�������
	unsigned short 	start_track_time;	//��ʼ����ʱ��
}start_track_time_info_t;

/**
* @	�������ö�ʧĿ������¿�ʼ������ʱ��
*/
typedef struct _reset_time_info
{
	unsigned char 	state;				//���û�������
	unsigned short 	reset_time;			//��λʱ��
}reset_time_info_t;


/**
* @	���ü��仯��ϵ��sensֵ
*/
typedef struct _sens_info
{
	unsigned char 	state;				//���û�������
	unsigned short 	sens;				//sensֵ
}sens_info_t;

/**
* @	�ƶ�����ͷ����
*/
typedef enum _move_cam_type
{
	MOVE_NOT = 0,		//���ƶ�
	MOVE_ZOOM,			//�ƶ�����
	MOVE_PAN_TILT,		//�ƶ�λ��
	MOVE_NUM
}move_cam_type_e;

/**
* @	�����йص�ȫ�ֲ���
*/
typedef struct _cam_control_info
{
	int		cam_last_speed;							//����ͷ�ϴε��ƶ��ٶ�
	camera_move_direction_e		cam_last_direction;	//����ͷ�ϴ��ƶ�����
	unsigned short cam_speed;				//����ͷת���ٶȻ���
	int		cam_position_value;				//Ԥ��λ��
	move_cam_type_e control_flag;			//��Ҫ�ƶ�zoomΪ1,��Ҫ�ƶ�λ��Ϊ2,����ҪΪ3
	cam_preset_position_info_t	cam_position[PRESET_NUM_MAX];
}cam_control_info_t;



/**
* @	����ģʽ���ã��Ǻ�ԭʼͼ��Ƚϻ��Ǻ���֡ͼ��Ƚ�
*/
typedef struct _stusdietrack_mode_info
{
	unsigned char 	state;			//���û�������
	int16_t 	track_mode;	//��������0���Զ�����,1Ϊ�ֶ�����
}stusidetrack_mode_info_t;

/**
* @	�����йص�ȫ�ֲ���
*/
typedef struct _stucam_control_info
{
	int		cam_last_speed;					//����ͷ�ϴε��ƶ��ٶ�
	camera_move_direction_e		cam_last_direction;	//����ͷ�ϴ��ƶ�����
	unsigned short cam_speed;				//����ͷת���ٶȻ���
	int		cam_position_value;				//Ԥ��λ��
	move_cam_type_e control_flag;			//��Ҫ�ƶ�zoomΪ1,��Ҫ�ƶ�λ��Ϊ2,����ҪΪ3
	cam_preset_position_info_t	cam_position[PRESET_NUM_MAX];
	unsigned char	cam_addr;						//�������ַ
}stucam_control_info_t;


/**
* @ �ͱ����йص�һЩȫ�ֱ���(����ȫ�ֱ������õ��Ľṹ��)
*/
typedef struct _track_encode_info
{
	short	is_encode;		//Ϊ0��ʾ������,Ϊ1��ʾ����
	short	server_cmd;		//����������͵�����,��ʾ�������л�ѧ��������ʦ��
	int		track_status;	//���ٻ�״̬0��ʾδ������,1��ʾ������
	int		zoom_pan_delay;	//��ת������λ��ʱ��zoom�͵���������λ��ʱ�м�ļ����λ��ms
}track_encode_info_t;


/**
* @ �ͱ����йص�һЩȫ�ֱ���(����ȫ�ֱ������õ��Ľṹ��)
*/
typedef struct _stutrack_encode_info
{
	short	is_encode;			//Ϊ0��ʾ������,Ϊ1��ʾ����
	short	server_cmd;			//����������͵�����,��ʾ�������л�ѧ��������ʦ��
	int		track_status;		//���ٻ�״̬0��ʾδ������,1��ʾ������
	int		send_cmd;			//��ʦ��Ҫ���͵�����
	short	last_position_no;	//�ϴ�Ԥ��λ��
	int		is_track;			//�Ƿ����1��ʾ����,0��ʾ������
	unsigned short is_control_cam;//�Ƿ����������ƽ�����1Ϊ�ƽ�����0Ϊ���ƽ���
	int		zoom_pan_delay;	//��ת������λ��ʱ��zoom�͵���������λ��ʱ�м�ļ����λ��ms
	int		last_send_cmd;			//��ʦ����һ�η��͵�����
	int 	nTriggerValDelay;
	int 	nOnlyRightSideUpDelay;
	int 	nLastTriggerVal;
	int		nStandUpPos;
}stutrack_encode_info_t;

/**
* @ �ͱ����йص�һЩȫ�ֱ���(����ȫ�ֱ������õ��Ľṹ��)
*/
typedef struct _stusidetrack_encode_info
{
	short	is_encode;		//Ϊ0��ʾ������,Ϊ1��ʾ����
	int		is_save_class_view;	//�����ǰȡ��ͼƬ��1��Ҫ���棬0����Ҫ����
	int		students_status;	//ѧ��վ��״̬��1Ϊվ����2Ϊ���£�0Ϊ�����κβ���
	int		last_students_status;	//�ϴ�ѧ��վ��״̬��1Ϊվ����2Ϊ���£�0Ϊ�����κβ���
}stusidetrack_encode_info_t;


/**
* @	�����ֶ��������
*/
typedef struct _manual_commond_info
{
	unsigned char 	state;			//���û�������
	unsigned short 	type;			//�ֶ������е������ͣ���СΪ0-65535
	unsigned short 	value;			//�ֶ������е�ֵ����СΪ0-65535
}manual_commond_info_t;


/**
* @	���ò������ͣ����ڽ������������λ�úͽ�����Ϣ����һ���߳���ɵģ�����
* @  ���յ�λ����Ϣʱ��Ҫ֪�����ĸ���������ȡλ����Ϣ�ġ�
*/
typedef enum	_set_cmd_type
{
	SET_PRESET_POSITION = 1,		//����Ԥ��λ
	SET_LIMITE_POSITION = 2,		//������λ
	GET_CUR_POSITION = 3,			//�Զ�����״̬�»�ȡ�����λ�ã������ж��Ƿ��ڰ���ȥ��
	SET_BLACKBOARD_POSITION = 4		//���úڰ���������
}set_cmd_type_e;

/**
* @ ������������ϴ�����������Ϣ
*/
typedef struct _track_save_file_info
{
	pthread_mutex_t save_track_m;	//�����޸ĸ��ٻ������ļ�ʱ�ӵ���
	set_cmd_type_e		set_cmd;	//���������������
	int		cmd_param;				//�������ϸֵ
}track_save_file_info_t;

/**
* @	������ͺ�����
*/
typedef enum _cam_type_info
{
	VISCA_STANDARD 	= 0,		//��׼VISCAЭ�飬���������ģ���ʤ�Ǵ�������
	SONY_BRC_Z330  	= 1, 		//SONY��BRC-Z330Э�������Э��
	BAOLIN_CAM 		= 2,		//���������֧��
	CAM_NUM
}cam_type_info_e;

/**
* @ ������ͺ�ѡ�� 
*/
typedef struct _track_cam_model_info
{
	cam_type_info_e		cam_type;
	
}track_cam_model_info_t;


/**
* @ ѧ���������ϱ�����Ϣ�ṹ��
*/
typedef struct _rightside_trigger_info
{
	unsigned int	nTriggerType;//��ǰȡ��0/����ȡ��1			
	unsigned int	nTriggerVal;//1��ʾ�д�����2��ʾ����(ֻ�п�ǰȡ������)��0������
}rightside_trigger_info_t;



/**
* @	�л������������
*/
typedef enum _switch_cmd_author_info
{
	AUTHOR_TEACHER	= 1,	//�л���������ʦ�����
	AUTHOR_STUDENTS	= 2,	//��ѧ�������͵��л�����
	AUTHOR_NUM
}switch_cmd_author_info_e;

/**
* @	���λ����������Ϣ��
*/
typedef struct _track_strategy_info
{
	int	left_pan_tilt1;	//����1���	//ǰ�ĸ�����ǰ����ֽ�Ϊ����ͷˮƽλ��,���ĸ��ֽ�Ϊ����ͷ��ֱλ�ã���ʾ�ڰ�����߶�Ӧ�����λ��
	int	right_pan_tilt1;//����1�ұ�		//ǰ�ĸ�����ǰ����ֽ�Ϊ����ͷˮƽλ��,���ĸ��ֽ�Ϊ����ͷ��ֱλ�ã���ʾ�ڰ����ұ߶�Ӧ�����λ��
	int	left_pan_tilt2;	//����2���	//ǰ�ĸ�����ǰ����ֽ�Ϊ����ͷˮƽλ��,���ĸ��ֽ�Ϊ����ͷ��ֱλ�ã���ʾ�ڰ�����߶�Ӧ�����λ��
	int	right_pan_tilt2;//����2�ұ�		//ǰ�ĸ�����ǰ����ֽ�Ϊ����ͷˮƽλ��,���ĸ��ֽ�Ϊ����ͷ��ֱλ�ã���ʾ�ڰ����ұ߶�Ӧ�����λ��
	int cur_pan_tilt;		//ǰ�ĸ�����ǰ����ֽ�Ϊ����ͷˮƽλ��,���ĸ��ֽ�Ϊ����ͷ��ֱλ�ã���ʾ�������ǰ������λ��
	int	blackboard_region_flag1;//�Ƿ��ڰ��������־��1Ϊ�ڰ�������0Ϊ���ڰ�������
	int	blackboard_region_flag2;//�Ƿ��ڰ��������־��1Ϊ�ڰ�������0Ϊ���ڰ�������
	int	students_panorama_switch_near_time;		//ѧ��վ��������ѧ��ȫ�������������ѧ����д�������ʱ�䣬��λ����
	int	teacher_blackboard_time1;				//��ʦͣ���ںڰ������ʱ��������ʱ�伴��Ϊ����ʦ�ںڰ����򣬵�λ����
	int	teacher_leave_blackboard_time1;			//��ʦ�뿪�ڰ������ʱ��������ʱ�伴��Ϊ����ʦ���ںڰ����򣬵�λ����
	int	teacher_blackboard_time2;				//��ʦͣ���ںڰ������ʱ��������ʱ�伴��Ϊ����ʦ�ںڰ����򣬵�λ����
	int	teacher_leave_blackboard_time2;			//��ʦ�뿪�ڰ������ʱ��������ʱ�伴��Ϊ����ʦ���ںڰ����򣬵�λ����
	int	teacher_panorama_time;					//��ʦ�ƶ���೤ʱ���л���ʦȫ������λ����
	int	teacher_leave_panorama_time;			//��ʦͣ������ú��л�����ʦ��д����λ����
	int	teacher_keep_panorama_time;				//��ʦȫ����ͷ�����ʱ�䣬��λ����
	int	move_flag;								//�ƶ���־,��ʦ���ٻ�����ͷ�ƶ���־
	int	send_cmd;		//��ʦ����ǰҪ���͵�����
	int	last_send_cmd;		//��һ����ʦ�����͵�����
	switch_cmd_author_info_e	switch_cmd_author;			//�л������������ʦ������ѧ����
	int teacher_panorama_flag;//��ʦȫ��,��ʾѧ���Ͻ�̨��,��Ŀ����ʱ��
	int	students_down_time;			//ѧ���½�̨ʱ��������ʱ�伴�ٴν���ѧ��������Ϣ����λ����
	int	students_track_flag;			//�Ƿ����ѧ�������Լ���̨��Ϣ��־����λ����
	int switch_vga_flag;//�Ƿ��л�vga
	int teacher_switch_students_delay_time;//��ʦ��ͷ�л���ѧ����ͷ��ʱ
	int	students_near_keep_time;			//ѧ��������ͷ����ʱ��
	int	vga_keep_time;//vga ����ʱ��,�������ʱ��,����vga��ͷ�л�����ľ�ͷ
	int cam_left_limit;//����λ
	int cam_right_limit;//����λ
	int position1_mv_flag;//������ʦ���˺����ƶ�Ŀ��
	int mv_keep_time;//��ʦ���ˣ������ƶ�Ŀ��ʱѧ��ȫ������ʦȫ����ͷ�л���ʱ
	int strategy_no;//������ţ�����λΪ0���Ļ�λΪ1�����λΪ2
}track_strategy_info_t;

/**
* @	���λ����������Ϣ��,����ͳ�� 
*/
typedef struct _track_strategy_timeinfo
{
	int	last_strategy_no;//��һ�εĻ�λ��,�������ǰ��һ��������س�ʼ����������
	int	students_panorama_switch_near_time;//ѧ��ȫ���л�ѧ��ǰ���ۻ�ʱ��,���ʱ������������Ե���ʱ����ѧ������
	int	blackboard_time1;//��ʦ�����������1��ʱ��,���ֻ��һ����������,���������1��Ч
	int	leave_blackboard_time1;//��ʦ�뿪��������1��ʱ��,���ֻ��һ����������,���������1��Ч
	int	blackboard_time2;//��ʦ�������2����ʱ��
	int	leave_blackboard_time2;//��ʦ�뿪����2����ʱ��
	int	panorama_time;//5��λ,��ʦȫ��ʱ��
	int	leave_panorama_time;//5��λ,�뿪��ʦȫ��ʱ��
	
	int	teacher_panorama_switch_near_time;//��ʦȫ���л�����ʦ����֮ǰ��ʱ��,ʱ��ͳ�Ƶ��˲������õ�ʱ����л���ʦ����
	int teacher_panorama_flag;//��ǰ��ͷ�Ƿ�����ʦȫ��
	int	students_down_time;//ѧ���½�̨��ʱ��ͳ��
	int	students_to_platform_time;//ѧ���Ͻ�̨��ʱ��ͳ��
	int	switch_flag;// 1�����л�,0 ��ʱδ�� �������л�
	int	switch_delaytime;//��׼��ʱʱ��,3s
	int	delaytime;//ÿ���л�����ʱʱ��,���ݲ�ͬ�л���ͷ,�õ�����ʱʱ��Ҳ��һ��
	int	on_teacher_flag;//��ǰ��ͷ�Ƿ�����ʦ��,������ʦ�Ͱ������ʦȫ��,������ʦ��ѧ��ʱ����ʱ����
	int	on_teacher_delaytime;//��ʦ��ͷ�ı���ʱ��
	int	vga_delaytime;//vga��ʱ��
	int	mv_time;//��ʦ��ʧʱ,�����˶�Ŀ���ʱ��
	int	stu_time;//��ʦ��ʧʱ,�л���ѧ��ȫ����ʱ��
}track_strategy_timeinfo_t;


/**
* @	��Ŀ����̨������Ϣ
*/
typedef struct _multitarget_range_info
{
	unsigned char state;							//���û�������
	unsigned char point_num;						//ȷ����������ĵ���
	unsigned short video_width;						//��Ƶ�Ŀ�
	unsigned short video_height;					//��Ƶ�ĸ�
	rectangle_info_t rectangle[STUDENTS_MULTITARGET_NUM];		//���ٿ�
}multitarget_range_info_t;


/**
* @	���ο���Ϣ
*/
typedef struct _shield_range_info
{
	unsigned char state;							//���û�������
	unsigned char point_num;						//ȷ����������ĵ���
	unsigned short video_width;						//��Ƶ�Ŀ�
	unsigned short video_height;					//��Ƶ�ĸ�
	rectangle_info_t rectangle[STUDENTS_SHIELD_NUM];		//���ٿ�
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

