/*
****************************************************
Copyright (C), 1999-1999, Reach Tech. Co., Ltd.

File name:     	audio_track.c

Description:    ��ʦ����

Date:     		2012-09-04

Author:	  		sunxl

version:  		V1.0

*****************************************************
*/

#include "commontrack.h"

extern int gRemoteFD;
extern EduKitLinkStruct_t	*gEduKit;

/**
* @	tcp�����
*/
#define TCP_MSG_LEN		(256)

/**
* @	�豸�����ַ������ȵĺ�
*/
#define DEV_NAME_LEN	(32)

/**
* @	���Ϳ�������ͷ������ĳ���
*/
#define CTRL_CMD_LEN	(16)

/**
* @	��ʦ���ٵĳ�ʼ��ʱ����Ĳ���
*/
ITRACK_Params 	g_track_param		= {0};
track_other_param_t g_track_other_param ={0};

/**
* @ ��ʦ���ٲ��������ļ�
*/
#define TEACH_TRACK_FILE		"teach_track.ini"


//===========================��ʦ��̬������صĺ궨��=====================

/**
* @ ��ʦ���ٵľ�̬������������
*/
#define STATIC_NAME				"static"

/**
* @ ��̬����������
*/
#define VIDEO_WIDTH				"video_width"
#define	VIDEO_HEIGHT			"video_height"
#define PIC_WIDTH				"pic_width"
#define PIC_HEIGHT				"pic_height"




//===========================��ʦ��̬������صĺ궨��=====================

/**
* @ ��ʦ���ٵĶ�̬������������
*/
#define DYNAMIC_NAME			"dynamic"



/**
* @ ��̬����������
*/
#define ZOOM_DISTACE			"zoom_distance"
#define X_OFFSET 				"x_offset"
#define Y_OFFSET				"y_offset"
#define TRIGGER_SUM 			"trigger_sum"
#define POS1_TRIGGER_SUM 		"pos1_trigger_num"
#define TRIGGER0_X0				"trigger0_x0"
#define TRIGGER0_Y0				"trigger0_y0"
#define TRIGGER0_X1				"trigger0_x1"
#define TRIGGER0_Y1				"trigger0_y1"
#define TRIGGER1_X0				"trigger1_x0"
#define TRIGGER1_Y0				"trigger1_y0"
#define TRIGGER1_X1				"trigger1_x1"
#define TRIGGER1_Y1				"trigger1_y1"
#define MEANSHIFT_TRIGGER_X0	"meanshift_trigger_x0"
#define MEANSHIFT_TRIGGER_Y0	"meanshift_trigger_y0"
#define MEANSHIFT_TRIGGER_X1	"meanshift_trigger_x1"
#define MEANSHIFT_TRIGGER_Y1	"meanshift_trigger_y1"
#define POS1_TRIGGER_X0			"pos1_trigger_x0"
#define POS1_TRIGGER_Y0			"pos1_trigger_y0"
#define POS1_TRIGGER_X1			"pos1_trigger_x1"
#define POS1_TRIGGER_Y1			"pos1_trigger_y1"

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

//===========================��ʦ����ͷ���Ƶ���ز����궨������=====================

/**
* @ ��ʦ����ͷ���Ʋ�����������
*/
#define TEACH_CAM_NAME			"cam_control"

/**
* @	��ʦ����ͷ���Ʋ���
*/
#define TEACH_CAM_SPEED			"cam_speed"

/**
* @ ��ʦ����ͷ��λ������������
*/
#define TEACH_CAM_LIMIT_POSITION	"limit_position"

/**
* @ ��ʦ����ͷ������λ����
*/
#define LIMIT_LEFT_DOWN_NAME	"left_down"

/**
* @ ��ʦ����ͷ������λ����
*/
#define LIMIT_RIGHT_UP_NAME		"right_up"


/**
* @ ��ʦ����ͷԤ��λ������������
*/
#define TEACH_CAM_PRESET_POSITION	"preset_position"

/**
* @ ��ʦ����ͷԤ��λλ�õ���������ֵ
*/
#define PRESET_POSITION_VALUE		"position_value"

/**
* @ ��ʦ����ͷԤ��λ����λ��ֵ
*/
#define PRESET_ZOOM_VALUE			"zoom_value"


/**
* @ ���ٻ���Ӧ�ı������
*/
#define TEACH_TRACK_ENCODE			"teach_track_encode"

/**
* @	�Ƿ�����־
*/
#define TEACH_IS_ENCODE				"teach_is_encode"


//-----------------------��ʦ����������Ʒ�ʽ--------------------------------
/**
* @	��ʦ��������Ʒ�ʽ
*/
#define CAM_CONTROL_TYPE				"cam_control_type"


/**
* @	������ʦ������Ԥ��λʱ��������ļ��ʱ��
*/
#define CAM_ZOOM_PAN_DELAY					"cam_zoom_pan_delay"


//-----------------------------�����л�������Ҫ����Ϣ-------------------------
/**
* @	�л����������Ϣ
*/
#define TRACK_STRATEGY				"track_strategy"


/**
* @	�ڰ�1���λ����Ϣ
*/
#define	BLACKBOARD1_POSITION_LEFT	"blackboard1_position_left"

/**
* @	�ڰ�1�ұ�λ����Ϣ
*/
#define	BLACKBOARD1_POSITION_RIGHT	"blackboard1_position_right"
/**
* @	�ڰ�2���λ����Ϣ
*/
#define	BLACKBOARD2_POSITION_LEFT	"blackboard2_position_left"

/**
* @	�ڰ�2�ұ�λ����Ϣ
*/
#define	BLACKBOARD2_POSITION_RIGHT	"blackboard2_position_right"

/**
* @	ѧ��ȫ����ͷ�л���ѧ����д��ͷ����ʱʱ����Ϣ
*/
#define	STUDENTS_PANORAMA_SWITCH_NEAR_TIME	"students_panorama_switch_near_time"

/**
* @	ȷ��Ϊ����1��ʱ�䣬����ʦ�����ںڰ������ʱ�䳬����ʱ�伴Ϊ����	
*/
#define	TEACHER_BLACKBOARD_TIME1			"teacher_blackboard_time1"

/**
* @	ȷ��Ϊ�뿪����1��ʱ�䣬����ʦ�������ڰ��������ʱ�䳬����ʱ�䣬��Ϊ���ڰ���
*/
#define	TEACHER_LEAVE_BLACKBOARD_TIME1			"teacher_leave_blackboard_time1"

/**
* @	ȷ��Ϊ����2��ʱ�䣬����ʦ�����ںڰ������ʱ�䳬����ʱ�伴Ϊ����	
*/
#define	TEACHER_BLACKBOARD_TIME2			"teacher_blackboard_time2"

/**
* @	ȷ��Ϊ�뿪����2��ʱ�䣬����ʦ�������ڰ��������ʱ�䳬����ʱ�䣬��Ϊ���ڰ���
*/
#define	TEACHER_LEAVE_BLACKBOARD_TIME2			"teacher_leave_blackboard_time2"


//---------------------------------��ʦȫ����ͷ������Ҫ��Ϣ------------------

/**
* @	ѧ���½�̨ʱ��������ʱ�伴�ٴν���ѧ��������Ϣ����λ����
*/
#define	STUDENTS_DOWN_TIME			"students_down_time"

/**
* @ Ҫ�л�����ʦȫ����ͷ����ʱʱ��
*/
#define	TEACHER_PANORAMA_TIME			"teacher_panorama_time"

/**
* @ Ҫ����ʦ�����ȫ����ͷ�л��ߵ���ʱʱ��
*/
#define	TEACHER_LEAVE_PANORAMA_TIME			"teacher_leave_panorama_time"


/**
* @ ��ʦȫ����ͷ�����ʱ��
*/
#define	TEACHER_KEEP_PANORAMA_TIME			"teacher_keep_panorama_time"


/**
* @ ��ʦ�л���ѧ��֮ǰ��ͷ��ʱ
*/
#define	TEACHER_SWITCH_STUDENTS_DELAY_TIME			"teacher_switch_students_delay_time"

/**
* @ѧ��������ͷ���ֵ����ʱ��
*/
#define	STUDENTS_NEAR_KEEP_TIME			"students_near_keep_time"

/**
* @vga��ͷ����ʱ��
*/
#define	VGA_KEEP_TIME			"vga_keep_time"

/**
* @��ʦ���ˣ������ƶ�Ŀ��ʱѧ��ȫ������ʦȫ����ͷ�л���ʱ
*/
#define	MV_KEEP_TIME			"mv_keep_time"

/**
* @��λ��� 
*/
#define	STRATEGY_NO			"strategy_no"

/**
* @meanshift ����
*/
#define MEANSHIFT_FLAG		"meanshift_flag"
//---------------------------------------------------------------------------------------


/**
* @	�����Ƿ���Ҫ���趯̬������־
*/
static int g_recontrol_flag = 0;

/**
* @ ��������ͷ��Ҫ�Ĳ���
*/
static cam_control_info_t	g_cam_info = {0};

/**
* @	�͸����йصı����һЩȫ�ֲ���
*/
 track_encode_info_t	g_track_encode_info = {0};

/**
* @	�Ϳ�����Ϣͳ���йص�һЩȫ�ֲ���
*/
track_class_info_t g_tClassInfo= {{0},0,0};
final_class_info_t g_tFinalClassInfo= {{0},0};

/**
* @	��д���������ļ����
*/
track_save_file_info_t	g_track_save_file = {0};


static int 	g_set_positon_value = 0;
static int		g_set_zoom_value = 0;

/**
* @	������ͺ�ȫ�ֱ���
*/
static track_cam_model_info_t	g_track_cam_model = {0};

/**
* @	 �Ͱ����йص��������λ����Ϣȫ�ֱ���
*/
track_strategy_info_t		g_track_strategy_info = {0};//�л�������ر���

track_strategy_timeinfo_t		g_track_strategy_timeinfo = {0};//���������л�ʱȫ��ͳ��ʱ��

extern zlog_category_t     *ptrackzlog;

extern Int32 gStrategy;

int init_save_track_mutex(void)
{
	pthread_mutex_init(&g_track_save_file.save_track_m, NULL);
	
	return 0;
}

static int lock_save_track_mutex(void)
{
	pthread_mutex_lock(&g_track_save_file.save_track_m);
	return 0;
}

static int unlock_save_track_mutex(void)
{
	pthread_mutex_unlock(&g_track_save_file.save_track_m);
	return 0;
}

int destroy_save_track_mutex(void)
{
	pthread_mutex_destroy(&g_track_save_file.save_track_m);
	
	return 0;
}

static unsigned long writen(int fd, const void *vptr, size_t n)
{
        unsigned long nleft;
        unsigned long nwritten;
        const char      *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
                        if (nwritten < 0 && errno == EINTR)
                                nwritten = 0;           /* and call write() again */
                        else
                                return(-1);                     /* error */
                }

                nleft -= nwritten;
                ptr   += nwritten;
        }
        return(n);
}

/*send data to tty com*/
static int SendDataToCom(int fd,unsigned char *data,int len)
{
        unsigned long real_len = 0 ;
        //int i;

        if((real_len = writen(fd,data,len)) != len)
        {
                printf("SendDataToCom() write tty error\n");
                return -1;
        }
        usleep(20000);
        return (real_len);
}

static int SendDataToComNoDelay(int fd, unsigned char *data, int len)
{
	unsigned long real_len = 0 ;
	if((real_len = writen(fd, data, len)) != len) 
	{
		printf( "SendDataToCom() write tty error\n");
		return -1;
	}
	return (real_len);
}


//#define		ENC_1200_DEBUG



/**
* @ ��������: write_track_static_file()
* @ ��������: д���ٵľ�̬�������ļ���
* @ �������: static_param -- �㷨�ľ�̬����
* @ �������: ��
* @ ����ֵ:   ��
*/
int write_track_static_file(track_static_param_t *static_param)
{
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	lock_save_track_mutex();
	//д��̬������Ƶ��
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", static_param->video_width);
	ret =  ConfigSetKey(config_file, STATIC_NAME, VIDEO_WIDTH, temp);
	
	//д��̬������Ƶ��
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", static_param->video_height);
	ret =  ConfigSetKey(config_file, STATIC_NAME, VIDEO_HEIGHT, temp);
	
	//д��̬�������ź����Ƶ��
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", static_param->pic_width);
	ret =  ConfigSetKey(config_file, STATIC_NAME, PIC_WIDTH, temp);
	
	//д��̬�������ź����Ƶ��
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", static_param->pic_height);
	ret =  ConfigSetKey(config_file, STATIC_NAME, PIC_HEIGHT, temp);
	unlock_save_track_mutex();
	
	return 0;
	
}


/**
* @ ��������: jump_zoom_position()
* @ ��������: ��������ͷת��Ԥ��λ����λ��
* @ �������: cam_position -- Ԥ��λ���
* @ �������: data -- ������ͷ���͵����������
* @ ����ֵ:   ���͵����ݵĳ���
*/
static int jump_zoom_position(unsigned char *data, int cam_position)
{
	int size = 0;
	
//	int 			fd 						= gRemoteFD;
	
	data[0] = 0x81;
	data[1] = 0x01;
	data[2] = 0x04;
	data[3] = 0x47;
	
	data[4] = g_cam_info.cam_position[cam_position].zoom[0];	//zoom��λ����Ϣ
	data[5] = g_cam_info.cam_position[cam_position].zoom[1];	//zoom��λ����Ϣ
	data[6] = g_cam_info.cam_position[cam_position].zoom[2];	//zoom��λ����Ϣ
	data[7] = g_cam_info.cam_position[cam_position].zoom[3];	//zoom��λ����Ϣ
	
	data[8] = 0xff;
	
	size = 9;
	return size;
}

/**
* @ ��������: jump_absolute_position()
* @ ��������: ��������ͷת��Ԥ��λ����������λ��
* @ �������: cam_position -- Ԥ��λ���
* @ �������: data -- ������ͷ���͵����������
* @ ����ֵ:   ���͵����ݵĳ���
*/
static int jump_absolute_position(unsigned char *data, int cam_position)
{
	int size = 0;
	
	//int 			fd 						= gRemoteFD;
	
	data[0] = 0x81;
	data[1] = 0x01;
	data[2] = 0x06;
	data[3] = 0x02;
	data[4] = 0x14;//0x04;		//ˮƽ�����ٶ�
	data[5] = 0x14;//0x05;		//��ֱ�����ٶ�
	
	data[6] = g_cam_info.cam_position[cam_position].pan_tilt[0];	//ˮƽ����λ����Ϣ
	data[7] = g_cam_info.cam_position[cam_position].pan_tilt[1];	//ˮƽ����λ����Ϣ
	data[8] = g_cam_info.cam_position[cam_position].pan_tilt[2];	//ˮƽ����λ����Ϣ
	data[9] = g_cam_info.cam_position[cam_position].pan_tilt[3];	//ˮƽ����λ����Ϣ
	data[10] = g_cam_info.cam_position[cam_position].pan_tilt[4];	//��ֱ����λ����Ϣ
	data[11] = g_cam_info.cam_position[cam_position].pan_tilt[5];	//��ֱ����λ����Ϣ
	data[12] = g_cam_info.cam_position[cam_position].pan_tilt[6];	//��ֱ����λ����Ϣ
	data[13] = g_cam_info.cam_position[cam_position].pan_tilt[7];	//��ֱ����λ����Ϣ
	
	data[14] = 0xff;
	
	size = 15;
	return size;
}

/**
* @	��������: set_track_range()
* @	��������: ���ø����������
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_track_range(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	track_range_info_t *track_info = NULL;
	
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	char	text[FILE_NAME_LEN]			= {0};
	int 	ret 	= -1;
	
	int		index 	= 0;
	
	
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	track_info = (track_range_info_t *)data;
	
	if(track_info->point_num > TRACK_AREA_POINT_MAX)
	{
		track_info->point_num = TRACK_AREA_POINT_MAX;
	}
	
	dynamic_param->track_point_num = track_info->point_num;
	dynamic_param->reset_level = RE_START;
#ifdef ENC_1200_DEBUG
	PRINTF("track_info->point_num = %d\n", track_info->point_num);
#else
	printf("track_info->point_num = %d\n",track_info->point_num);
#endif
	
	lock_save_track_mutex();
	//����������ļ���
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->track_point_num);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRACK_POINT_NUM, temp);
	
	for(index = 0; index < track_info->point_num; index++)
	{
		dynamic_param->track_point[index].x = track_info->point[index].x;
		dynamic_param->track_point[index].y = track_info->point[index].y;
#ifdef ENC_1200_DEBUG
		PRINTF("track_info->point[index].x = %d,index = %d\n", track_info->point[index].x, index);
		PRINTF("track_info->point[index].y = %d,index = %d\n", track_info->point[index].y, index);
#else
		printf("track_info->point[index].x = %d,index = %d\n",track_info->point[index].x, index);
		printf("track_info->point[index].y = %d,index = %d\n",track_info->point[index].y, index);
#endif
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, TRACK_POINTX);
		sprintf(param_name, "%s%d",text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->track_point[index].x);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
		
		memset(param_name, 0, FILE_NAME_LEN);
		memset(text, 0, FILE_NAME_LEN);
		strcpy(text, TRACK_POINTY);
		sprintf(param_name, "%s%d",text, index);
		memset(temp, 0, FILE_NAME_LEN);
		sprintf(temp, "%d", dynamic_param->track_point[index].y);
		ret =  ConfigSetKey(config_file, DYNAMIC_NAME, param_name, temp);
	}
	unlock_save_track_mutex();
	
	return 0;
}

/**
* @	��������: set_trigger_range()
* @	��������: ���ô����������
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_trigger_range(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	trigger_range_info_t *trigger_info = NULL;
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	trigger_info = (trigger_range_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	dynamic_param->reset_level = RE_INIT;
	
	dynamic_param->trigger[0].trigger_x0= trigger_info->point[0].x;
	dynamic_param->trigger[0].trigger_y0= trigger_info->point[0].y;
	
	dynamic_param->trigger[0].trigger_x1= trigger_info->point[1].x;
	dynamic_param->trigger[0].trigger_y1= trigger_info->point[1].y;
	
	dynamic_param->trigger[1].trigger_x0= trigger_info->point[2].x;
	dynamic_param->trigger[1].trigger_y0= trigger_info->point[2].y;
	
	dynamic_param->trigger[1].trigger_x1= trigger_info->point[3].x;
	dynamic_param->trigger[1].trigger_y1= trigger_info->point[3].y;
	
	printf("yyyyyyyyy recv trigger[0]=%d,%d,%d,%d,trigger[1]=%d,%d,%d,%d\n",
	       trigger_info->point[0].x,trigger_info->point[0].y,trigger_info->point[1].x,trigger_info->point[1].y,
	       trigger_info->point[2].x,trigger_info->point[2].y,trigger_info->point[3].x,trigger_info->point[3].y);
	       
	lock_save_track_mutex();
	//����������ļ���
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->trigger[0].trigger_x0);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER0_X0, temp);
	
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->trigger[0].trigger_y0);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER0_Y0, temp);
	
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->trigger[0].trigger_x1);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER0_X1, temp);
	
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->trigger[0].trigger_y1);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER0_Y1, temp);
	
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->trigger[1].trigger_x0);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER1_X0, temp);
	
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->trigger[1].trigger_y0);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER1_Y0, temp);
	
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->trigger[1].trigger_x1);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER1_X1, temp);
	
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->trigger[1].trigger_y1);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER1_Y1, temp);
	
	unlock_save_track_mutex();
	
	return 0;
}

/**
* @	��������: set_limit_height()
* @	��������: ���ø��������е��޸���,��Ŀ���yֵ�������ֵ�����ٸ���
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_limit_height(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	limit_height_info_t *limit_info = NULL;
	
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	limit_info = (limit_height_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
#ifdef ENC_1200_DEBUG
	PRINTF("limit_info->limit_height = %d\n",limit_info->limit_height);
#else
	printf("limit_info->limit_height = %d\n",limit_info->limit_height);
#endif
	
	dynamic_param->limit_height = limit_info->limit_height;
	dynamic_param->reset_level = RE_INIT;
	
	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->limit_height);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, LIMIT_HEIGHT, temp);
	unlock_save_track_mutex();
	
	return 0;
}

/**
* @	��������: set_track_type()
* @	��������: ���ø�������,0Ϊ�Զ�����,1Ϊ�ֶ�����,�����浽�����ļ���
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_track_type(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	control_type_info_t *track_info = NULL;
	
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	track_info = (control_type_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	dynamic_param->control_mode = track_info->control_type;
	dynamic_param->reset_level = RE_INIT;
	
#ifdef ENC_1200_DEBUG
	PRINTF("track_info->control_type = %d\n",track_info->control_type);
#else
	printf("track_info->control_type = %d\n",track_info->control_type);
#endif
	
	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->control_mode);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, CONTROL_MODE, temp);
	unlock_save_track_mutex();
	
	if(AUTO_CONTROL == dynamic_param->control_mode)
	{
		__call_preset_position(TRIGGER_POSITION1);
	}
	
	return 0;	
}

/**
* @	��������: set_draw_line_type()
* @	��������: ���ø�������,0Ϊ�Զ�����,1Ϊ�ֶ�����,�����浽�����ļ���
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_draw_line_type(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	draw_line_info_t *track_line_info = NULL;
	
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	track_line_info = (draw_line_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	dynamic_param->message = track_line_info->message;
	dynamic_param->reset_level = NO_INIT;
#ifdef ENC_1200_DEBUG
	PRINTF("track_line_info->message = %d\n",track_line_info->message);
#else
	printf("track_line_info->message = %d\n",track_line_info->message);
#endif
	
	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->message);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, MESSAGE, temp);
	unlock_save_track_mutex();
	
	return 0;
}

/**
* @	��������: set_trigger_sum()
* @	��������: ���ô��������,Ĭ��ֵΪ3��
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_trigger_sum(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	trigger_num_info_t *trigger_num_info = NULL;
	
	
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	trigger_num_info = (trigger_num_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	dynamic_param->trigger_sum = trigger_num_info->trigger_num;
	dynamic_param->reset_level = RE_INIT;
	
#ifdef ENC_1200_DEBUG
	PRINTF("trigger_num_info->trigger_num = %d\n",trigger_num_info->trigger_num);
#else
	printf("trigger_num_info->trigger_num = %d\n",trigger_num_info->trigger_num);
#endif
	
	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->trigger_sum);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, TRIGGER_SUM, temp);
	unlock_save_track_mutex();
	
	return 0;
}

/**
* @	��������: set_camara_speed()
* @	��������: ���ô��������,Ĭ��ֵΪ3��
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: cam_contror_info -- ����ͷ���Ʋ����ṹ��
* @ ����ֵ:   ��
*/
static int set_camara_speed(unsigned char *data, cam_control_info_t *cam_contror_info)
{
	cam_speed_info_t *cam_speed_info = NULL;
	
	
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	//char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	cam_speed_info = (cam_speed_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	cam_contror_info->cam_speed = cam_speed_info->cam_speed;
	
#ifdef ENC_1200_DEBUG
	PRINTF("cam_contror_info->cam_speed = %d\n",cam_speed_info->cam_speed);
#else
	printf("cam_contror_info->cam_speed = %d\n",cam_contror_info->cam_speed);
#endif
	
	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", cam_contror_info->cam_speed);
	ret =  ConfigSetKey(config_file, TEACH_CAM_NAME, TEACH_CAM_SPEED, temp);
	unlock_save_track_mutex();
	
	return 0;
}


/**
* @	��������: get_cam_position()
* @	��������: ��ȡ����ͷ��������λ��
* @	�������: ��
* @ �������: data,���ػ�ȡ������ͷ��λ����Ϣ
* @ ����ֵ:   ��
*/
static int get_cam_position(unsigned char 	*data)
{
	int 			fd 				= gRemoteFD;
	unsigned char 	cmd_data[20]	= {0};
//	int 			ret 			= -1;
	int 			size 			= 0;
	
	cmd_data[0] = 0x81;
	cmd_data[1] = 0x09;
	cmd_data[2] = 0x06;
	cmd_data[3] = 0x12;
	cmd_data[4] = 0xFF;
	size = 5;
	SendDataToCom(fd, cmd_data, size);
	
	return 0;
}

/**
* @	��ȡ��ʦ�����λ�ò���
*/
int __get_cam_position(void)
{
	unsigned char data[128] = {0};
	get_cam_position(data);
	
	return 0;
}

/**
* @	��������: get_cam_zoom()
* @	��������: ��ȡ��ͷ���ڵĽ���λ��
* @	�������: ��
* @ �������: data,���ػ�ȡ������ͷ�Ľ�����Ϣ
* @ ����ֵ:   ��
*/
static int get_cam_zoom(unsigned char 	*data)
{
	int 			fd 				= gRemoteFD;
	unsigned char 	cmd_data[20]	= {0};
//	int 			ret 			= -1;
	int 			size 			= 0;
	
	cmd_data[0] = 0x81;
	cmd_data[1] = 0x09;
	cmd_data[2] = 0x04;
	cmd_data[3] = 0x47;
	cmd_data[4] = 0xFF;
	size = 5;
	SendDataToCom(fd, cmd_data, size);
	
	return 0;
}

/**
* @	��������: set_limit_position()
* @	��������: �������Ϻ�������λ
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
static int set_limit_position(unsigned short limit_position, unsigned char *position_data)
{
	int 					fd 			= gRemoteFD;
	//int 	ret 						= -1;
	int 	size 						= 0;
	//unsigned char recv_data[32]			= {0};
	unsigned char send_data[32]			= {0};
	
	if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
	{
		send_data[0] = 0x81;
		send_data[1] = 0x01;
		send_data[2] = 0x06;
		send_data[3] = 0x07;
		send_data[4] = 0x00;
		send_data[5] = 0x00;	//��ʾ������λ����
		
		//������ҵ�λ����Ϣ
		send_data[6] =  position_data[0];//0x00;
		send_data[7] =  position_data[1];//0x00;
		send_data[8] = position_data[2];//0x00;
		send_data[9] = position_data[3];//0x07;
		send_data[10] = position_data[4];//0x0f;
		
		//�ϻ����µ�λ����Ϣ
		send_data[11] = position_data[5];//0x0f;
		send_data[12] = position_data[6];//0x0f;
		send_data[13] = position_data[7];//0x0e;
		send_data[14] = position_data[8];//0x0e;
		
		send_data[15] = 0xff;
		
		if(LIMIT_DOWN_LEFT == limit_position)
		{
			send_data[5] = 0x00;	//��ʾ������λ����
		}
		else if(LIMIT_UP_RIGHT == limit_position)
		{
		   	send_data[5] = 0x01;		//��ʾ������λ����
		}
		size = 16;
		SendDataToCom(fd, send_data, size);
		
	}
	else
	{
		send_data[0] = 0x81;
		send_data[1] = 0x01;
		send_data[2] = 0x06;
		send_data[3] = 0x07;
		send_data[4] = 0x00;
		send_data[5] = 0x00;	//��ʾ������λ����
		
		//������ҵ�λ����Ϣ
		send_data[6] = position_data[0];
		send_data[7] = position_data[1];
		send_data[8] = position_data[2];
		send_data[9] = position_data[3];
		
		//�ϻ����µ�λ����Ϣ
		send_data[10] = position_data[4];
		send_data[11] = position_data[5];
		send_data[12] = position_data[6];
		send_data[13] = position_data[7];
		
		send_data[14] = 0xff;
		
		if(LIMIT_DOWN_LEFT == limit_position)
		{
			send_data[5] = 0x00;	//��ʾ������λ����
		}
		else if(LIMIT_UP_RIGHT == limit_position)
		{
		   	send_data[5] = 0x01;		//��ʾ������λ����
		}

		size = 15;
		SendDataToCom(fd, send_data, size);
	}
	
	return 0;
}

/**
* @	��������: clear_limit_position()
* @	��������: �������,������λ
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
static int clear_limit_position(void)
{
	int 			fd 				= gRemoteFD;
//	int 			ret 			= -1;
	int 			size 			= 0;
	unsigned char 	send_data[32]	= {0};
	
	if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
	{
		send_data[0] = 0x81;
		send_data[1] = 0x01;
		send_data[2] = 0x06;
		send_data[3] = 0x07;
		send_data[4] = 0x01;
		send_data[5] = 0x00;		//��ʾ������λ���
		
		//������ҵ�λ����Ϣ
		send_data[6] = 0x07;
		send_data[7] = 0x0f;
		send_data[8] = 0x0f;
		send_data[9] = 0x0f;
		send_data[10] = 0x0f;
		
		//�ϻ����µ�λ����Ϣ
		send_data[11] = 0x07;
		send_data[12] = 0x0f;
		send_data[13] = 0x0f;
		send_data[14] = 0x0f;
		
		send_data[15] = 0xff;
		size = 16;
		SendDataToCom(fd, send_data, size);
		sleep(1);
		send_data[5] = 0x01;		//��ʾ������λ���
		SendDataToCom(fd, send_data, size);
	}
	else
	{
		send_data[0] = 0x81;
		send_data[1] = 0x01;
		send_data[2] = 0x06;
		send_data[3] = 0x07;
		send_data[4] = 0x01;
		send_data[5] = 0x00;		//��ʾ������λ���
		
		//������ҵ�λ����Ϣ
		send_data[6] = 0x07;
		send_data[7] = 0x0f;
		send_data[8] = 0x0f;
		send_data[9] = 0x0f;
		
		//�ϻ����µ�λ����Ϣ
		send_data[10] = 0x07;
		send_data[11] = 0x0f;
		send_data[12] = 0x0f;
		send_data[13] = 0x0f;
		
		send_data[14] = 0xff;
		size = 15;
		SendDataToCom(fd, send_data, size);
		
		send_data[5] = 0x01;		//��ʾ������λ���
		SendDataToCom(fd, send_data, size);
	}
	
	return 0;
}



/**
* @	��������: set_or_clear_limit_position()
* @	��������: ���ô��������,Ĭ��ֵΪ3��
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
static int set_or_clear_limit_position(unsigned char *data)
{
	limit_position_info_t 	*limit_position 			= NULL;
//	int 					fd 							= gRemoteFD;
	char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	unsigned char 			recv_data[32]				= {0};
	//unsigned char 			temp_data[32]				= {0};
	int 					ret 						= -1;
//	int						index						= 0;
	
	limit_position = (limit_position_info_t *)data;
	
#ifdef ENC_1200_DEBUG
	PRINTF("limit_position->limit_position = %d\n",limit_position->limit_position);
#else
	printf("limit_position->limit_position = %d\n",limit_position->limit_position);
#endif
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	
	if((LIMIT_DOWN_LEFT == limit_position->limit_position)
	        || (LIMIT_UP_RIGHT == limit_position->limit_position))
	{
		g_track_save_file.set_cmd = SET_LIMITE_POSITION;
		g_track_save_file.cmd_param = limit_position->limit_position;
		get_cam_position(recv_data);
	}
	else
		if(LIMIT_CLEAR == limit_position->limit_position)
		{
			lock_save_track_mutex();
			if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
			{
				strcpy(temp, "7ffff7fff");
			}
			else
			{
				strcpy(temp, "7fff7fff");
			}
			ret =  ConfigSetKey(config_file, TEACH_CAM_LIMIT_POSITION, LIMIT_LEFT_DOWN_NAME, temp);
			ret =  ConfigSetKey(config_file, TEACH_CAM_LIMIT_POSITION, LIMIT_RIGHT_UP_NAME, temp);
			unlock_save_track_mutex();
			clear_limit_position();
		}
		
	return 0;
}

/**
* @	��������: init_limit_position_set()
* @	��������: ��ʼ��λ����,��ϵͳ������������ļ���ȡ����λ��Ϣ��������
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
static int init_limit_position_set(void)
{
	char 			config_file[FILE_NAME_LEN] 	= {0};
//	char			param_name[FILE_NAME_LEN] 	= {0};
	char 			temp[FILE_NAME_LEN]			= {0};
	unsigned char 	temp_data[32]				= {0};
	int 			ret 						= -1;
	int				index						= 0;
	int						a 							= 0;
	int						b 							= 0;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	//����������λ
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, TEACH_CAM_LIMIT_POSITION, LIMIT_LEFT_DOWN_NAME, temp);
	if(ret == 0)
	{
		for(index = 0; index < CAM_PAN_TILT_LEN; index++)
		{
			if((temp[index] >= 0x30) && (temp[index] <= 0x39))
			{
				temp_data[index] = temp[index] - 0x30;
			}
			if((temp[index] >= 0x41) && (temp[index] <= 0x46))
			{
				temp_data[index] = temp[index] - 0x41 + 0xa;
			}
			if((temp[index] >= 0x61) && (temp[index] <= 0x66))
			{
				temp_data[index] = temp[index] - 0x61 + 0xa;
			}
		}
#ifdef ENC_1200_DEBUG
		PRINTF("left temp_data = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",temp_data[0],temp_data[1],temp_data[2],temp_data[3],temp_data[4],temp_data[5],temp_data[6],temp_data[7]);
#else
		printf("left temp_data = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",temp_data[0],temp_data[1],temp_data[2],temp_data[3],temp_data[4],temp_data[5],temp_data[6],temp_data[7]);
#endif
		
		if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
		{
			b = 0;
			b = temp_data[4];
			
			a = temp_data[3];
			
			b = (a << 4) | b;
			a = temp_data[2];
			b = (a << 8) | (b);
			a = temp_data[1];
			b = (a << 12) | (b);
			a = temp_data[0];
			b = (a << 16) | (b);
			g_track_strategy_info.cam_left_limit=b & 0x000fffff;
		}
		else
		{
			b = 0;
			
			b = temp_data[3];
			a = temp_data[2];
			
			b = (a << 4) | (b);
			a = temp_data[1];
			b = (a << 8) | (b);
			a = temp_data[0];
			b = (a << 12) | (b);
			g_track_strategy_info.cam_left_limit=b & 0x0000ffff;
		}
		if(0 != strcmp(temp, "7fff7fff"))
		{
			set_limit_position(LIMIT_DOWN_LEFT, temp_data);
		}
	}
	
	//����������λ
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, TEACH_CAM_LIMIT_POSITION, LIMIT_RIGHT_UP_NAME, temp);
	if(ret == 0)
	{
		for(index = 0; index < CAM_PAN_TILT_LEN; index++)
		{
			if((temp[index] >= 0x30) && (temp[index] <= 0x39))
			{
				temp_data[index] = temp[index] - 0x30;
			}
			if((temp[index] >= 0x41) && (temp[index] <= 0x46))
			{
				temp_data[index] = temp[index] - 0x41 + 0xa;
			}
			if((temp[index] >= 0x61) && (temp[index] <= 0x66))
			{
				temp_data[index] = temp[index] - 0x61 + 0xa;
			}
		}
#ifdef ENC_1200_DEBUG
		PRINTF("right temp_data = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",temp_data[0],temp_data[1],temp_data[2],temp_data[3],temp_data[4],temp_data[5],temp_data[6],temp_data[7]);
#else
		printf("right temp_data = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",temp_data[0],temp_data[1],temp_data[2],temp_data[3],temp_data[4],temp_data[5],temp_data[6],temp_data[7]);
#endif
		if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
		{
			b = 0;
			b = temp_data[4];
			
			a = temp_data[3];
			
			b = (a << 4) | b;
			a = temp_data[2];
			b = (a << 8) | (b);
			a = temp_data[1];
			b = (a << 12) | (b);
			a = temp_data[0];
			b = (a << 16) | (b);
			g_track_strategy_info.cam_right_limit=b & 0x000fffff;
		}
		else
		{
			b = 0;
			
			b = temp_data[3];
			a = temp_data[2];
			
			b = (a << 4) | (b);
			a = temp_data[1];
			b = (a << 8) | (b);
			a = temp_data[0];
			b = (a << 12) | (b);
			g_track_strategy_info.cam_right_limit=b & 0x0000ffff;
		}

		if(0 != strcmp(temp, "7fff7fff"))
		{
			set_limit_position(LIMIT_UP_RIGHT, temp_data);
		}
	}
	return 0;
}

/**
* @	��������: set_preset_position()
* @	��������: ����Ԥ��λ
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
static int set_preset_position(unsigned char *data)
{
	preset_position_info_t 	*preset_position 			= NULL;
	int 					fd 							= gRemoteFD;
	//char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	//char					param_name[FILE_NAME_LEN] 	= {0};
//	char					text[FILE_NAME_LEN]			= {0};
	unsigned char 			recv_data[32]				= {0};
	//unsigned char 			temp_data[32]				= {0};
//	int 					ret 						= -1;
//	int						index						= 0;
	
	unsigned char			cmd_data[20]	= {0};
	int 					size			= 0;
	
	preset_position = (preset_position_info_t *)data;
	
#ifdef ENC_1200_DEBUG
	PRINTF("preset_position->preset_position = %d\n",preset_position->preset_position);
#else
	printf("preset_position->preset_position = %d\n",preset_position->preset_position);
#endif
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	if(preset_position->preset_position > (PRESET_NUM_MAX - 1))
	{
#ifdef ENC_1200_DEBUG
		PRINTF("preset value must less than 5\n");
#else
		printf("preset value must less than 5\n");
#endif
		return 0;
		
	}
	
	if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
	{
		//��sony z330 �����ֻ��16��Ԥ��λ������ҪС�ڵ���15
		if(preset_position->preset_position > 15)
		{
			preset_position->preset_position = 0;
		}
		
		cmd_data[0] = 0x81;
		cmd_data[1] = 0x01;
		cmd_data[2] = 0x04;
		cmd_data[3] = 0x3F;
		cmd_data[4] = 0x01;
		
		cmd_data[5] = preset_position->preset_position;
		cmd_data[6] = 0xff;
		size = 7;
		SendDataToCom(fd, cmd_data, size);
	}
	else
	{
	
		//����Ԥ��λ������ֵ
		//g_set_positon_value = preset_position->preset_position;
		g_track_save_file.set_cmd = SET_PRESET_POSITION;
		g_track_save_file.cmd_param = preset_position->preset_position;
		get_cam_position(recv_data);
		
		usleep(50000);
		//����Ԥ��λzoomֵ
		//g_set_zoom_value = preset_position->preset_position;
		get_cam_zoom(recv_data);
	}
	return 0;
}


int save_position_zoom(unsigned char *data)
{
	char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	char					param_name[FILE_NAME_LEN] 	= {0};
	char					text[FILE_NAME_LEN]			= {0};
//	unsigned char 			recv_data[32]				= {0};
//	unsigned char 			temp_data[32]				= {0};
	int 					ret 						= -1;
//	int 					index						= 0;
	int						a 							= 0;
	int						b 							= 0;
	
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
	{
		if((0x90 == data[0]) && (0x50 == data[1]) && (0xff == data[11]))
		{
			if(SET_PRESET_POSITION == g_track_save_file.set_cmd)
			{
				memset(text, 0, FILE_NAME_LEN);
				memset(param_name, 0, FILE_NAME_LEN);
				strcpy(text, PRESET_POSITION_VALUE);
				sprintf(param_name, "%s%d",text, g_track_save_file.cmd_param);
				memset(temp, 0, FILE_NAME_LEN);
				sprintf(temp, "%x%x%x%x%x%x%x%x%x",data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10]);
				lock_save_track_mutex();
				ret =  ConfigSetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);
				unlock_save_track_mutex();
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[0]= data[2];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[1]= data[3];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[2]= data[4];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[3]= data[5];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[4]= data[6];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[5]= data[7];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[6]= data[8];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[7]= data[9];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[8]= data[10];
			}
			else if(SET_LIMITE_POSITION == g_track_save_file.set_cmd)
			{
				sprintf(temp, "%x%x%x%x%x%x%x%x%x",data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10]);
				printf( "g_track_save_file.cmd_param = %d\n",g_track_save_file.cmd_param);
				if(LIMIT_DOWN_LEFT == g_track_save_file.cmd_param)
				{
					lock_save_track_mutex();
					ret =  ConfigSetKey(config_file, TEACH_CAM_LIMIT_POSITION, LIMIT_LEFT_DOWN_NAME, temp);
					unlock_save_track_mutex();
				}

				if(LIMIT_UP_RIGHT == g_track_save_file.cmd_param)
				{
					lock_save_track_mutex();
					ret =  ConfigSetKey(config_file, TEACH_CAM_LIMIT_POSITION, LIMIT_RIGHT_UP_NAME, temp);
					unlock_save_track_mutex();
				}
			
				set_limit_position(g_track_save_file.cmd_param, &data[2]);

			}
			else if(SET_BLACKBOARD_POSITION == g_track_save_file.set_cmd)
			{

				memset(text, 0, FILE_NAME_LEN);
				memset(param_name, 0, FILE_NAME_LEN);
				memset(temp, 0, FILE_NAME_LEN);

				b = 0;
				b = data[6];
				
				a = data[5];
				
				b = (a << 4) | b;
				a = data[4];
				b = (a << 8) | (b);
				a = data[3];
				b = (a << 12) | (b);
				a = data[2];
				b = (a << 16) | (b);

				//Ϊ0��ʾ����1��ߣ�1��ʾ����1�ұߣ�2��ʾ����2��ߣ�3��ʾ����2�ұ�
				switch(g_track_save_file.cmd_param)
				{
					case 0:
						strcpy(text, BLACKBOARD1_POSITION_LEFT);
						g_track_strategy_info.left_pan_tilt1 = b & 0x000fffff;
						sprintf(temp, "%d",g_track_strategy_info.left_pan_tilt1);
						printf( "z330 set blackboard1 left value = %d\n",g_track_strategy_info.left_pan_tilt1);
						break;
					case 1:
						strcpy(text, BLACKBOARD1_POSITION_RIGHT);
						g_track_strategy_info.right_pan_tilt1 = b & 0x000fffff;
						sprintf(temp, "%d",g_track_strategy_info.right_pan_tilt1);
						printf( "z330 set blackboard1 right value = %d\n",g_track_strategy_info.right_pan_tilt1);
						break;
					case 2:
						strcpy(text, BLACKBOARD2_POSITION_LEFT);
						g_track_strategy_info.left_pan_tilt2 = b & 0x000fffff;
						sprintf(temp, "%d",g_track_strategy_info.left_pan_tilt2);
						printf( "z330 set blackboard2 left value = %d\n",g_track_strategy_info.left_pan_tilt2);
						break;
					case 3:
						strcpy(text, BLACKBOARD2_POSITION_RIGHT);
						g_track_strategy_info.right_pan_tilt2 = b & 0x000fffff;
						sprintf(temp, "%d",g_track_strategy_info.right_pan_tilt2);
						printf( "z330 set blackboard2 right value = %d\n",g_track_strategy_info.right_pan_tilt2);
						break;
					default:
						break;
				}
				sprintf(param_name, "%s",text);
				
				//sprintf(temp, "%x%x%x%x%x",data[2],data[3],data[4],data[5],data[6]);

				lock_save_track_mutex();
				ret =  ConfigSetKey(config_file, TRACK_STRATEGY, param_name, temp);
				unlock_save_track_mutex();

				printf( "z330 set blackboard value temp = %s\n",temp);
			}
			else if(GET_CUR_POSITION == g_track_save_file.set_cmd)
			{
			
				b = 0;
				b = data[6];
				
				a = data[5];
				
				b = (a << 4) | b;
				a = data[4];
				b = (a << 8) | (b);
				a = data[3];
				b = (a << 12) | (b);
				a = data[2];
				b = (a << 16) | (b);
				g_track_strategy_info.cur_pan_tilt = b & 0x000fffff;

				printf( "#############z330 g_track_strategy_info.cur_pan_tilt = %d\n",g_track_strategy_info.cur_pan_tilt);

				if((g_track_strategy_info.cur_pan_tilt >= g_track_strategy_info.left_pan_tilt1)
					|| (g_track_strategy_info.cur_pan_tilt <= g_track_strategy_info.right_pan_tilt1))
				{
					g_track_strategy_info.blackboard_region_flag1 = 1;
				}
				else
				{
					g_track_strategy_info.blackboard_region_flag1 = 0;
				}
				if((g_track_strategy_info.cur_pan_tilt >= g_track_strategy_info.left_pan_tilt2)
					|| (g_track_strategy_info.cur_pan_tilt <= g_track_strategy_info.right_pan_tilt2))
				{
					g_track_strategy_info.blackboard_region_flag2 = 1;
				}
				else
				{
					g_track_strategy_info.blackboard_region_flag2 = 0;
				}
			}
			else
			{
				if(data[2] > 8)
				{
					
				}
			}
		}

		else if((0x90 == data[0]) && (0x50 == data[1]) && (0xff == data[6]))
		{
			if(SET_PRESET_POSITION == g_track_save_file.set_cmd)
			{
				memset(text, 0, FILE_NAME_LEN);
				memset(param_name, 0, FILE_NAME_LEN);
				strcpy(text, PRESET_ZOOM_VALUE);
				sprintf(param_name, "%s%d",text, g_track_save_file.cmd_param);
				memset(temp, 0, FILE_NAME_LEN);
				sprintf(temp, "%x%x%x%x",data[2],data[3],data[4],data[5]);
				lock_save_track_mutex();
				ret =  ConfigSetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);
				unlock_save_track_mutex();
				g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[0] = data[2];
				g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[1] = data[3];
				g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[2] = data[4];
				g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[3] = data[5];
			}
		}

	}
	else
	{
		if((0x90 == data[0]) && (0x50 == data[1]) && (0xff == data[10]))
		{
			if(SET_PRESET_POSITION == g_track_save_file.set_cmd)
			{
				memset(text, 0, FILE_NAME_LEN);
				memset(param_name, 0, FILE_NAME_LEN);
				strcpy(text, PRESET_POSITION_VALUE);
				sprintf(param_name, "%s%d",text, g_track_save_file.cmd_param);
				memset(temp, 0, FILE_NAME_LEN);
				sprintf(temp, "%x%x%x%x%x%x%x%x",data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9]);
				lock_save_track_mutex();
				ret =  ConfigSetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);
				unlock_save_track_mutex();
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[0]= data[2];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[1]= data[3];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[2]= data[4];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[3]= data[5];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[4]= data[6];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[5]= data[7];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[6]= data[8];
				g_cam_info.cam_position[g_track_save_file.cmd_param].pan_tilt[7]= data[9];
			}
			else if(SET_LIMITE_POSITION == g_track_save_file.set_cmd)
			{
				sprintf(temp, "%x%x%x%x%x%x%x%x",data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9]);
				
				if(LIMIT_DOWN_LEFT == g_track_save_file.cmd_param)
				{
					lock_save_track_mutex();
					ret =  ConfigSetKey(config_file, TEACH_CAM_LIMIT_POSITION, LIMIT_LEFT_DOWN_NAME, temp);
					unlock_save_track_mutex();
				}

				if(LIMIT_UP_RIGHT == g_track_save_file.cmd_param)
				{
					lock_save_track_mutex();
					ret =  ConfigSetKey(config_file, TEACH_CAM_LIMIT_POSITION, LIMIT_RIGHT_UP_NAME, temp);
					unlock_save_track_mutex();
				}
			
				set_limit_position(g_track_save_file.cmd_param, &data[2]);

			}
			else if(SET_BLACKBOARD_POSITION == g_track_save_file.set_cmd)
			{

				memset(text, 0, FILE_NAME_LEN);
				memset(param_name, 0, FILE_NAME_LEN);
				memset(temp, 0, FILE_NAME_LEN);

				
				b = 0;
				
				b = data[5];
				a = data[4];
				
				b = (a << 4) | (b);
				a = data[3];
				b = (a << 8) | (b);
				a = data[2];
				b = (a << 12) | (b);
				
				//Ϊ0��ʾ����1��ߣ�1��ʾ����1�ұߣ�2��ʾ����2��ߣ�3��ʾ����2�ұ�
				switch(g_track_save_file.cmd_param)
				{
					case 0:
						strcpy(text, BLACKBOARD1_POSITION_LEFT);
						g_track_strategy_info.left_pan_tilt1 = b & 0x0000ffff;
						sprintf(temp, "%d",g_track_strategy_info.left_pan_tilt1);
						printf( "set blackboard1 left value = %d\n",g_track_strategy_info.left_pan_tilt1);
						break;
					case 1:
						strcpy(text, BLACKBOARD1_POSITION_RIGHT);
						g_track_strategy_info.right_pan_tilt1 = b & 0x0000ffff;
						sprintf(temp, "%d",g_track_strategy_info.right_pan_tilt1);
						printf( "set blackboard1 right value = %d\n",g_track_strategy_info.right_pan_tilt1);
						break;
					case 2:
						strcpy(text, BLACKBOARD2_POSITION_LEFT);
						g_track_strategy_info.left_pan_tilt2 = b & 0x0000ffff;
						sprintf(temp, "%d",g_track_strategy_info.left_pan_tilt2);
						printf( "set blackboard2 left value = %d\n",g_track_strategy_info.left_pan_tilt2);
						break;
					case 3:
						strcpy(text, BLACKBOARD2_POSITION_RIGHT);
						g_track_strategy_info.right_pan_tilt2 = b & 0x0000ffff;
						sprintf(temp, "%d",g_track_strategy_info.right_pan_tilt2);
						printf( "set blackboard2 right value = %d\n",g_track_strategy_info.right_pan_tilt2);
						break;
					default:
						break;
				}
				sprintf(param_name, "%s",text);
				
				//sprintf(temp, "%x%x%x%x",data[2],data[3],data[4],data[5]);
				
				lock_save_track_mutex();
				ret =  ConfigSetKey(config_file, TRACK_STRATEGY, param_name, temp);
				unlock_save_track_mutex();
				printf( "set blackboard value temp = %s\n",temp);

			}
			else if(GET_CUR_POSITION == g_track_save_file.set_cmd)
			{
			
				b = 0;
				
				b = data[5];
				a = data[4];
				
				b = (a << 4) | (b);
				a = data[3];
				b = (a << 8) | (b);
				a = data[2];
				b = (a << 12) | (b);
				g_track_strategy_info.cur_pan_tilt = b & 0x0000ffff;
				//printf( "#############g_track_strategy_info.cur_pan_tilt = %d\n",g_track_strategy_info.cur_pan_tilt);
				if(((g_track_strategy_info.left_pan_tilt1 < 0x8000) && (g_track_strategy_info.right_pan_tilt1 < 0x8000))
					|| ((g_track_strategy_info.left_pan_tilt1 > 0x8000) && (g_track_strategy_info.right_pan_tilt1 > 0x8000)))
				{
					if((g_track_strategy_info.cur_pan_tilt >= g_track_strategy_info.left_pan_tilt1)
						&& (g_track_strategy_info.cur_pan_tilt <= g_track_strategy_info.right_pan_tilt1))
					{
						g_track_strategy_info.blackboard_region_flag1 = 1;
					}
					else
					{
						g_track_strategy_info.blackboard_region_flag1 = 0;
					}

				}
				else
				{
					if((g_track_strategy_info.cur_pan_tilt >= g_track_strategy_info.left_pan_tilt1)
						|| (g_track_strategy_info.cur_pan_tilt <= g_track_strategy_info.right_pan_tilt1))
					{
						g_track_strategy_info.blackboard_region_flag1 = 1;
					}
					else
					{
						g_track_strategy_info.blackboard_region_flag1 = 0;
					}
				}
				if(((g_track_strategy_info.left_pan_tilt2 < 0x8000) && (g_track_strategy_info.right_pan_tilt2 < 0x8000))
					|| ((g_track_strategy_info.left_pan_tilt2 > 0x8000) && (g_track_strategy_info.right_pan_tilt2 > 0x8000)))
				{
					if((g_track_strategy_info.cur_pan_tilt >= g_track_strategy_info.left_pan_tilt2)
						&& (g_track_strategy_info.cur_pan_tilt <= g_track_strategy_info.right_pan_tilt2))
					{
						g_track_strategy_info.blackboard_region_flag2 = 1;
					}
					else
					{
						g_track_strategy_info.blackboard_region_flag2 = 0;
					}

				}
				else
				{
					if((g_track_strategy_info.cur_pan_tilt >= g_track_strategy_info.left_pan_tilt2)
						|| (g_track_strategy_info.cur_pan_tilt <= g_track_strategy_info.right_pan_tilt2))
					{
						g_track_strategy_info.blackboard_region_flag2 = 1;
					}
					else
					{
						g_track_strategy_info.blackboard_region_flag2 = 0;
					}
				}
			}
			else
			{
				if(data[2] > 8)
				{
					
				}
			}
		}

		else if((0x90 == data[0]) && (0x50 == data[1]) && (0xff == data[6]))
		{
			if(SET_PRESET_POSITION == g_track_save_file.set_cmd)
			{
				memset(text, 0, FILE_NAME_LEN);
				memset(param_name, 0, FILE_NAME_LEN);
				strcpy(text, PRESET_ZOOM_VALUE);
				sprintf(param_name, "%s%d",text, g_track_save_file.cmd_param);
				memset(temp, 0, FILE_NAME_LEN);
				sprintf(temp, "%x%x%x%x",data[2],data[3],data[4],data[5]);
				lock_save_track_mutex();
				ret =  ConfigSetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);
				unlock_save_track_mutex();
				g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[0] = data[2];
				g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[1] = data[3];
				g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[2] = data[4];
				g_cam_info.cam_position[g_track_save_file.cmd_param].zoom[3] = data[5];
			}
		}	
	}
	
	return 0;
}

/**
* @	��������: __call_preset_position()
* @	��������: ͨ��Ԥ��λ��ŵ���Ԥ��λ
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
int __call_preset_position(int preset_position)
{
	int 					fd 							= gRemoteFD;
	//char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
//	char					param_name[FILE_NAME_LEN] 	= {0};
//	char					text[FILE_NAME_LEN]			= {0};
	//unsigned char 			recv_data[32]				= {0};
//	unsigned char 			temp_data[32]				= {0};
//	int 					ret 						= -1;
//	int						index						= 0;
	unsigned char 			cmd_code[CTRL_CMD_LEN] 		= {0};
	int						size						= 0;
	
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	if(preset_position > (PRESET_NUM_MAX - 1))
	{
		return -1;
	}
	
	if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
	{
		if(preset_position > 15)
		{
			preset_position = 0;
		}
		unsigned char	cmd_data[20]	= {0};
		
		cmd_data[0] = 0x81;
		cmd_data[1] = 0x01;
		cmd_data[2] = 0x04;
		cmd_data[3] = 0x3F;
		cmd_data[4] = 0x02;
		
		cmd_data[5] = preset_position;
		cmd_data[6] = 0xff;
		size = 7;
		SendDataToCom(fd, cmd_data, size);
	}
	else
	{
		size = jump_zoom_position(cmd_code, preset_position);
		if(size > 0)
		{
			SendDataToCom(fd, cmd_code, size);
		}
		usleep(50000);
		size = jump_absolute_position(&cmd_code, preset_position);
		if(size > 0)
		{
			SendDataToCom(fd, cmd_code, size);
		}
	}
	return 0;
}


/**
* @	��������: call_preset_position()
* @	��������: ����Ԥ��λ
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
static int call_preset_position(unsigned char *data)
{
	preset_position_info_t 	*preset_position 			= NULL;
	int 					fd 							= gRemoteFD;
//	char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	//char					param_name[FILE_NAME_LEN] 	= {0};
	//char					text[FILE_NAME_LEN]			= {0};
	//unsigned char 			recv_data[32]				= {0};
//	unsigned char 			temp_data[32]				= {0};
//	int 					ret 						= -1;
	//int						index						= 0;
	unsigned char 			cmd_code[CTRL_CMD_LEN] 	= {0};
	int						size						= 0;
	
	preset_position = (preset_position_info_t *)data;
	
#ifdef ENC_1200_DEBUG
	PRINTF("call_preset_position preset_position = %d\n",preset_position->preset_position);
#else
	printf("call_preset_position preset_position = %d\n",preset_position->preset_position);
#endif
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	if(preset_position->preset_position > (PRESET_NUM_MAX - 1))
	{
#ifdef ENC_1200_DEBUG
		PRINTF("call preset value must less than 5\n");
#else
		printf("call preset value must less than 5\n");
#endif
		return 0;
	}
	
	
	if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
	{
		if(preset_position->preset_position > 15)
		{
			preset_position->preset_position = 0;
		}
		unsigned char	cmd_data[20]	= {0};
		
		cmd_data[0] = 0x81;
		cmd_data[1] = 0x01;
		cmd_data[2] = 0x04;
		cmd_data[3] = 0x3F;
		cmd_data[4] = 0x02;
		
		cmd_data[5] = preset_position->preset_position;
		cmd_data[6] = 0xff;
		size = 7;
		SendDataToCom(fd, cmd_data, size);
	}
	else
	{
	
		size = jump_zoom_position(cmd_code, preset_position->preset_position);
		if(size > 0)
		{
			SendDataToCom(fd, cmd_code, size);
		}
		size = jump_absolute_position(cmd_code, preset_position->preset_position);
		if(size > 0)
		{
			SendDataToCom(fd, cmd_code, size);
		}
	}
	return 0;
}

/**
* @	��������: set_track_distance()
* @	��������: ���ø��پ������
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_track_distance(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	track_distance_info_t *track_distance_info = NULL;
	
	
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
//	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	track_distance_info = (track_distance_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	dynamic_param->zoom_distance = track_distance_info->zoom_distance;
	dynamic_param->reset_level = RE_INIT;
	
#ifdef ENC_1200_DEBUG
	PRINTF("track_line_info->zoom_distance = %d\n",track_distance_info->zoom_distance);
#else
	printf("track_line_info->zoom_distance = %d\n",track_distance_info->zoom_distance);
#endif
	
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->zoom_distance);
	lock_save_track_mutex();
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, ZOOM_DISTACE, temp);
	unlock_save_track_mutex();
	
	return 0;
}

/**
* @	��������: set_track_is_encode()
* @	��������: �����Ƿ���б���ı�־
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
static int set_track_is_encode(unsigned char *data)
{
	track_is_encode_info_t *encode_info 			= NULL;
	char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	int 					ret 						= -1;
	
	encode_info = (track_is_encode_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	if(encode_info->isencode > 1)
	{
		encode_info->isencode = 1;
	}
	
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", encode_info->isencode);
	lock_save_track_mutex();
	ret =  ConfigSetKey(config_file, TEACH_TRACK_ENCODE, TEACH_IS_ENCODE, temp);
	unlock_save_track_mutex();
	g_track_encode_info.is_encode = encode_info->isencode;

	if(g_track_encode_info.is_encode == 1)
	{
		enc_enable_channel(gEduKit->encoderLink.encLink.link_id, 2);
	}
	else
	{
		enc_disable_channel(gEduKit->encoderLink.encLink.link_id, 2);
	}
		
#ifdef ENC_1200_DEBUG
	PRINTF("encode_info->isencode = %d\n",encode_info->isencode);
#else
	printf("encode_info->isencode = %d\n",encode_info->isencode);
#endif
	return 0;
}

/**
* @	��������: set_track_param()
* @	��������: ������ٲ�����
* @	�������: data -- ��encodemanage���յ������ò�������
* @ 		  len -- ��������
* @ �������: ��
* @ ����ֵ:   ��
*/
static int set_track_param(unsigned char *data, int len)
{
	char cmd_name[256] = {0};
	
	FILE *fp;
	fp = fopen("/usr/local/reach/dvr_rdk/ti816x/track_temp.ini","w");
	fwrite(data,len,1,fp);
	fclose(fp);
	lock_save_track_mutex();
	sprintf(cmd_name, "mv /usr/local/reach/dvr_rdk/ti816x/track_temp.ini /usr/local/reach/dvr_rdk/ti816x/%s", TEACH_TRACK_FILE);
	system(cmd_name);
	unlock_save_track_mutex();
	system("sync");
	sleep(1);
	system("reboot -f");
	
	return 0;
}

/**
* @	��������: set_start_track_time()
* @	��������: ����Ԥ����ʱ��,��������೤ʱ�俪ʼ����
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_start_track_time(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	start_track_time_info_t *start_time_info = NULL;
	
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
//	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	start_time_info = (start_track_time_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
#ifdef ENC_1200_DEBUG
	PRINTF("start_time_info->start_track_time = %d\n",start_time_info->start_track_time);
#else
	printf("start_time_info->start_track_time = %d\n",start_time_info->start_track_time);
#endif
	
	dynamic_param->start_track_time = start_time_info->start_track_time;
	dynamic_param->reset_level = RE_INIT;
	
	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->start_track_time);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, START_TRACK_TIME, temp);
	unlock_save_track_mutex();
	
	return 0;
}


/**
* @	��������: set_reset_time()
* @	��������: ���ø�λʱ��,��Ŀ�궪ʧ���ÿ�ʼ����
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_reset_time(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	reset_time_info_t *reset_time_info = NULL;
	
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
//	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	reset_time_info = (reset_time_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
#ifdef ENC_1200_DEBUG
	PRINTF("reset_time_info->reset_time = %d\n",reset_time_info->reset_time);
#else
	printf("reset_time_info->reset_time = %d\n",reset_time_info->reset_time);
#endif
	
	dynamic_param->reset_time = reset_time_info->reset_time;
	dynamic_param->reset_level = RE_INIT;
	
	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->reset_time);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, RESET_TIME, temp);
	unlock_save_track_mutex();
	
	return 0;
}


/**
* @	��������: set_sens_value()
* @	��������: ���ü��仯��sensֵ
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_sens_value(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	sens_info_t *sens_info = NULL;
	
	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
//	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	sens_info = (sens_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
#ifdef ENC_1200_DEBUG
	PRINTF("reset_time_info->reset_time = %d\n",sens_info->sens);
#else
	printf("reset_time_info->reset_time = %d\n",sens_info->sens);
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
* @	��������: set_manual_commond()
* @	��������: �����ֶ��������
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
static int set_manual_commond(unsigned char *data)
{
	manual_commond_info_t *manual_commond_info 			= NULL;
	char 					temp[FILE_NAME_LEN]			= {0};
	char 					config_file[FILE_NAME_LEN] 	= {0};
	int 					ret 						= -1;
	
	manual_commond_info = (manual_commond_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);

	printf("manual_commond_info->type = %d manual_commond_info->value %d\n",manual_commond_info->type,manual_commond_info->value);
	switch(manual_commond_info->type)
	{
		case SET_ZOOM_PAN_DELAY:
			if(manual_commond_info->value < 30)
			{
				manual_commond_info->value = 30;
			}
			
			lock_save_track_mutex();
			memset(temp, 0, FILE_NAME_LEN);
			sprintf(temp, "%d", manual_commond_info->value);
			ret =  ConfigSetKey(config_file, CAM_CONTROL_TYPE, CAM_ZOOM_PAN_DELAY, temp);
			unlock_save_track_mutex();
			printf( "manual_commond_info->value = %d\n",manual_commond_info->value);
			g_track_encode_info.zoom_pan_delay = manual_commond_info->value;
			break;
		case SET_BLACKBOARD_REGION:
			if(manual_commond_info->value > 3)
			{
				printf( "set blackboard region param is err!\n");
				break;
			}
			g_track_save_file.set_cmd = SET_BLACKBOARD_POSITION;
			g_track_save_file.cmd_param = manual_commond_info->value;
			__get_cam_position();
			break;
		default:
			break;
	}
	
	
	
	return 0;
}



/**
* @	��������: send_track_range()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_track_range(int socket, ITRACK_DynamicParams *dynamic_param)
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
	msg_header->nMsg = 0xA0;
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRACK_RANGE;
	
	track_range = (track_range_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	track_range->point_num = dynamic_param->track_point_num;
	track_range->state = 1;
	
	for(index = 0; index < dynamic_param->track_point_num; index++)
	{
		track_range->point[index].x = dynamic_param->track_point[index].x;
		track_range->point[index].y = dynamic_param->track_point[index].y;
	}
	
	send(socket, send_buf, len, 0);
	return 0;
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
	msg_type = 0xA0;
	
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
	
	for(index = 0; index < dynamic_param->track_point_num; index++)
	{
		track_range.point[index].x = dynamic_param->track_point[index].x;
		track_range.point[index].y = dynamic_param->track_point[index].y;
	}
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &track_range, sizeof(track_range_info_t));
	
	send(socket, send_buf, len, 0);
	return 0;
#endif
}


/**
* @	��������: send_trigger_range()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_trigger_range(int socket, ITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	trigger_range_info_t	*trigger_range 	= NULL;
	int				len				= 0;
	
	memset(&send_buf, 0, 256);
	
	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(trigger_range_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRIGGER_RANGE;
	trigger_range = (trigger_range_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	trigger_range->point_num = 2;
	trigger_range->state = 1;
	
	trigger_range->point[0].x = dynamic_param->trigger_x0;
	trigger_range->point[0].y = dynamic_param->trigger_y0;
	trigger_range->point[1].x = dynamic_param->trigger_x1;
	trigger_range->point[1].y = dynamic_param->trigger_y1;
	send(socket, send_buf, len, 0);
	
	return 0;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	trigger_range_info_t	trigger_range 	= {0};
	
	int				len_next		= 0;
	int				len				= 0;
	
	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(trigger_range_info_t);
	
	msg_len = htons(len);
	msg_type = 0xA0;
	
	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));
	
	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_TRIGGER_RANGE;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	trigger_range.point_num = 4;
	trigger_range.state = 1;
	
	trigger_range.point[0].x = dynamic_param->trigger[0].trigger_x0;
	trigger_range.point[0].y = dynamic_param->trigger[0].trigger_y0;
	trigger_range.point[1].x = dynamic_param->trigger[0].trigger_x1;
	trigger_range.point[1].y = dynamic_param->trigger[0].trigger_y1;
	
	trigger_range.point[2].x = dynamic_param->trigger[1].trigger_x0;
	trigger_range.point[2].y = dynamic_param->trigger[1].trigger_y0;
	trigger_range.point[3].x = dynamic_param->trigger[1].trigger_x1;
	trigger_range.point[3].y = dynamic_param->trigger[1].trigger_y1;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &trigger_range, sizeof(trigger_range_info_t));
	
	send(socket, send_buf, len, 0);
	return 0;
#endif
}

/**
* @	��������: send_limit_height()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_limit_height(int socket, ITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	limit_height_info_t	*limit_height 	= NULL;
	int				len				= 0;
	int				index			= 0;
	
	memset(&send_buf, 0, 256);
	
	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(limit_height_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;
	
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRACK_LIMIT_HEIGHT;
	
	limit_height = (limit_height_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	limit_height->state = 1;
	
	limit_height->limit_height = dynamic_param->limit_height;
	
	send(socket, send_buf, len, 0);
	return 0;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	limit_height_info_t	limit_height 	= {0};
	
	int				len_next		= 0;
	int				len				= 0;
	
	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(limit_height_info_t);
	
	msg_len = htons(len);
	msg_type = 0xA0;
	
	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));
	
	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_TRACK_LIMIT_HEIGHT;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));
	
	limit_height.state = 1;
	limit_height.limit_height = dynamic_param->limit_height;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &limit_height, sizeof(limit_height_info_t));
	
	send(socket, send_buf, len, 0);
	return 0;
#endif
}

/**
* @	��������: send_track_type()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_track_type(int socket, ITRACK_DynamicParams *dynamic_param)
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
	msg_header->nMsg = 0xA0;
	
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRACK_TYPE;
	
	control_type = (control_type_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	control_type->state = 1;
	
	control_type->control_type = dynamic_param->control_mode;
	
	send(socket, send_buf, len, 0);
	return 0;
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
	msg_type = 0xA0;
	
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
* @	��������: send_draw_line_type()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_draw_line_type(int socket, ITRACK_DynamicParams *dynamic_param)
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
	msg_header->nMsg = 0xA0;
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_DRAW_LINE_TYPE;
	
	draw_line_info = (draw_line_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	draw_line_info->state = 1;
	
	draw_line_info->message = dynamic_param->message;
	
	send(socket, send_buf, len, 0);
	return 0;
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
	msg_type = 0xA0;
	
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
* @	��������: send_trigger_num()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_trigger_num(int socket, ITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	trigger_num_info_t	*trigger_num_info = NULL;
	int				len				= 0;
	int				index			= 0;
	
	
	memset(&send_buf, 0, 256);
	
	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(trigger_num_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRIGGER_NUM_TYPE;
	
	trigger_num_info = (trigger_num_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	trigger_num_info->state = 1;
	
	trigger_num_info->trigger_num = dynamic_param->trigger_sum;
	send(socket, send_buf, len, 0);
	return 0;
#else
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
	msg_type = 0xA0;
	
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
	return 0;
	
#endif
}

/**
* @	��������: send_cam_speed()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: cam_control_info -- ����ͷ���Ʋ���
* @ ����ֵ:   ��
*/
static int send_cam_speed(int socket, cam_control_info_t *cam_control_info)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	cam_speed_info_t	*cam_speed_info = NULL;
	int				len				= 0;
	int				index			= 0;
	
	
	memset(&send_buf, 0, 256);
	
	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(cam_speed_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_CAM_SPEED_TYPE;
	
	cam_speed_info = (cam_speed_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	cam_speed_info->state = 1;
	
	cam_speed_info->cam_speed = cam_control_info->cam_speed;
	
	send(socket, send_buf, len, 0);
	
	return 0;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	cam_speed_info_t	cam_speed_info 	= {0};
	
	int				len_next		= 0;
	int				len				= 0;
	
	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(cam_speed_info_t);
	
	msg_len = htons(len);
	msg_type = 0xA0;
	
	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));
	
	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_CAM_SPEED_TYPE;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));
	
	cam_speed_info.state = 1;
	cam_speed_info.cam_speed = cam_control_info->cam_speed;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &cam_speed_info, sizeof(cam_speed_info_t));
	
	send(socket, send_buf, len, 0);
	return 0;
	
#endif
}

/**
* @	��������: send_track_distance()
* @	��������: ���͸��پ��������Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_track_distance(int socket, ITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	track_distance_info_t *track_distance_info = NULL;
	int				len				= 0;
	int				index			= 0;
	
	
	memset(&send_buf, 0, 256);
	
	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(track_distance_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRACK_DISTANCE_TYPE;
	
	track_distance_info = (track_distance_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	track_distance_info->state = 1;
	
	track_distance_info->zoom_distance = dynamic_param->zoom_distance;
	
	send(socket, send_buf, len, 0);
	return 0;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	track_distance_info_t	track_distance_info 	= {0};
	
	int				len_next		= 0;
	int				len				= 0;
	
	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(track_distance_info_t);
	
	msg_len = htons(len);
	msg_type = 0xA0;
	
	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));
	
	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_TRACK_DISTANCE_TYPE;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));
	
	track_distance_info.state = 1;
	track_distance_info.zoom_distance = dynamic_param->zoom_distance;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &track_distance_info, sizeof(track_distance_info_t));
	
	send(socket, send_buf, len, 0);
	return 0;
	
#endif
}

/**
* @	��������: send_track_is_encode()
* @	��������: ���͸��پ��������Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
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
	msg_header->nMsg = 0xA0;
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRACK_IS_ENCODE;
	
	track_is_encode = (track_is_encode_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	track_is_encode->state = 1;
	
	track_is_encode->isencode = is_encode;
	
	send(socket, send_buf, len, 0);
	return 0;
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
	msg_type = 0xA0;
	
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
	return 0;
	
#endif
}


/**
* @	��������: send_track_param()
* @	��������: ���͸��پ��������Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: ��
* @ ����ֵ:   ��
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
	sprintf(cmd_name, "/usr/local/reach/dvr_rdk/ti816x/%s",TEACH_TRACK_FILE);
	lock_save_track_mutex();
	fp = fopen(cmd_name,"r");
	if(fp == NULL)
	{
		return 0;
	}
	
	fread(temp_buf,8192,1,fp);
	
	fseek(fp,0,SEEK_END);
	file_len = ftell(fp);
	
	
	fclose(fp);
	unlock_save_track_mutex();
	
	printf("cmd_name = %s\n",cmd_name);
	printf("file_len = %d\n",file_len);
	
	memset(send_buf, 0, 8192);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + file_len;
	
	msg_len = htons(len);
	msg_type = 0xA0;

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
* @	��������: send_start_track_time()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_start_track_time(int socket, ITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	start_track_time_info_t	*start_time_info 	= NULL;
	int				len				= 0;
	int				index			= 0;
	
	memset(&send_buf, 0, 256);
	
	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(start_track_time_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;
	
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_START_TRACK_TIME;
	
	start_time_info = (start_track_time_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	start_time_info->state = 1;
	
	start_time_info->start_track_time = dynamic_param->start_track_time;
	
	send(socket, send_buf, len, 0);
	return 0;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	start_track_time_info_t	start_time_info 	= {0};
	
	int				len_next		= 0;
	int				len				= 0;
	
	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(start_track_time_info_t);
	
	msg_len = htons(len);
	msg_type = 0xA0;
	
	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));
	
	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_START_TRACK_TIME;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));
	
	start_time_info.state = 1;
	start_time_info.start_track_time = dynamic_param->start_track_time;
	
	printf("start_time_info.start_track_time  = %d\n",start_time_info.start_track_time );
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &start_time_info, sizeof(start_track_time_info_t));
	
	send(socket, send_buf, len, 0);
	return 0;
#endif
}

/**
* @	��������: send_reset_time()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_reset_time(int socket, ITRACK_DynamicParams *dynamic_param)
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
	msg_header->nMsg = 0xA0;
	
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_RESET_TIME;
	
	reset_time_info = (reset_time_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	reset_time_info->state = 1;
	
	reset_time_info->reset_time = dynamic_param->reset_time;
	
	send(socket, send_buf, len, 0);
	return 0;
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
	msg_type = 0xA0;
	
	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));
	
	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_RESET_TIME;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));
	
	reset_time_info.state = 1;
	reset_time_info.reset_time = dynamic_param->reset_time;
	
	printf("reset_time_info.reset_time  = %d\n",reset_time_info.reset_time );
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &reset_time_info, sizeof(reset_time_info_t));
	
	send(socket, send_buf, len, 0);
	return 0;
#endif
}

/**
* @	��������: send_sens_value()
* @	��������: ���ͼ��仯ϵ��sensֵ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_sens_value(int socket, ITRACK_DynamicParams *dynamic_param)
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
	msg_header->nMsg = 0xA0;
	
	
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_SENS_VALUE;
	
	sens_info = (sens_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];
	
	sens_info->state = 1;
	
	sens_info->sens = dynamic_param->sens;
	
	send(socket, send_buf, len, 0);
	return 0;
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
	msg_type = 0xA0;
	
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
	
	printf("sens_info.sens  = %d\n",sens_info.sens);
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &sens_info, sizeof(sens_info_t));

	send(socket, send_buf, len, 0);
	return 0;
#endif
}


/**
* @	��������: set_meanshift_trigger()
* @	��������: ����Һ���������췶Χ
* @	�������: data -- ��kitutool���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_meanshift_trigger(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	one_range_info_t *trigger_info = NULL;
	char	temp[FILE_NAME_LEN] 		= {0};
	char	config_file[FILE_NAME_LEN]	= {0};
	char	param_name[FILE_NAME_LEN]	= {0};
	int 	ret 						= -1;
	
	trigger_info = (one_range_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);

	dynamic_param->reset_level = RE_INIT;

	dynamic_param->TrackOtherParam.meanshift_trigger.trigger_x0= trigger_info->point[0].x;
	dynamic_param->TrackOtherParam.meanshift_trigger.trigger_y0= trigger_info->point[0].y;

	dynamic_param->TrackOtherParam.meanshift_trigger.trigger_x1= trigger_info->point[1].x;
	dynamic_param->TrackOtherParam.meanshift_trigger.trigger_y1= trigger_info->point[1].y;
	
	
	printf("yyyyyyyyy recv meanshift trigger=%d,%d,%d,%d\n",
		trigger_info->point[0].x,trigger_info->point[0].y,trigger_info->point[1].x,trigger_info->point[1].y);

	lock_save_track_mutex();
	//����������ļ���
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->TrackOtherParam.meanshift_trigger.trigger_x0);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, MEANSHIFT_TRIGGER_X0, temp);

	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->TrackOtherParam.meanshift_trigger.trigger_y0);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, MEANSHIFT_TRIGGER_Y0, temp);

	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->TrackOtherParam.meanshift_trigger.trigger_x1);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, MEANSHIFT_TRIGGER_X1, temp);

	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->TrackOtherParam.meanshift_trigger.trigger_y1);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, MEANSHIFT_TRIGGER_Y1, temp);

	unlock_save_track_mutex();
	return 0;
}


/**
* @	��������: send_meanshift_trigger()
* @	��������: ���͸��ٿ���Ϣ��kitetool
* @	�������: socket -- ��kitetool���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_meanshift_trigger(int socket, ITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	one_range_info_t	*trigger_range 	= NULL;
	int				len				= 0;

	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(one_range_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_MEANSHIFT_TRIGGER;
	trigger_range = (one_range_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	trigger_range->point_num = 2;
	trigger_range->state = 1;
	
	trigger_range->point[0].x = dynamic_param->trigger_x0;
	trigger_range->point[0].y = dynamic_param->trigger_y0;
	trigger_range->point[1].x = dynamic_param->trigger_x1;
	trigger_range->point[1].y = dynamic_param->trigger_y1;
	send(socket, send_buf, len, 0);
	
	return 0;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	one_range_info_t	trigger_range 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(one_range_info_t);

	msg_len = htons(len);
	msg_type = 0xA0;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_MEANSHIFT_TRIGGER;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	trigger_range.point_num = 2;
	trigger_range.state = 1;
	
	trigger_range.point[0].x = dynamic_param->TrackOtherParam.meanshift_trigger.trigger_x0;
	trigger_range.point[0].y = dynamic_param->TrackOtherParam.meanshift_trigger.trigger_y0;
	trigger_range.point[1].x = dynamic_param->TrackOtherParam.meanshift_trigger.trigger_x1;
	trigger_range.point[1].y = dynamic_param->TrackOtherParam.meanshift_trigger.trigger_y1;

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &trigger_range, sizeof(one_range_info_t));

	send(socket, send_buf, len, 0);
	return 0;
#endif
}

/**
* @	��������: set_pos1_trigger()
* @	��������: ����Һ���������췶Χ
* @	�������: data -- ��kitutool���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_pos1_trigger(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	one_range_info_t *trigger_info = NULL;
	char	temp[FILE_NAME_LEN] 		= {0};
	char	config_file[FILE_NAME_LEN]	= {0};
	char	param_name[FILE_NAME_LEN]	= {0};
	int 	ret 						= -1;
	
	trigger_info = (one_range_info_t *)data;
	
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);

	dynamic_param->reset_level = RE_INIT;

	dynamic_param->TrackOtherParam.pos1_trigger.trigger_x0= trigger_info->point[0].x;
	dynamic_param->TrackOtherParam.pos1_trigger.trigger_y0= trigger_info->point[0].y;

	dynamic_param->TrackOtherParam.pos1_trigger.trigger_x1= trigger_info->point[1].x;
	dynamic_param->TrackOtherParam.pos1_trigger.trigger_y1= trigger_info->point[1].y;
	
	
	printf("yyyyyyyyy recv pos1 trigger=%d,%d,%d,%d\n",
		trigger_info->point[0].x,trigger_info->point[0].y,trigger_info->point[1].x,trigger_info->point[1].y);

	lock_save_track_mutex();
	//����������ļ���
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->TrackOtherParam.pos1_trigger.trigger_x0);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, POS1_TRIGGER_X0, temp);

	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->TrackOtherParam.pos1_trigger.trigger_y0);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, POS1_TRIGGER_Y0, temp);

	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->TrackOtherParam.pos1_trigger.trigger_x1);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, POS1_TRIGGER_X1, temp);

	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->TrackOtherParam.pos1_trigger.trigger_y1);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, POS1_TRIGGER_Y1, temp);

	unlock_save_track_mutex();
	return 0;
}


/**
* @	��������: send_pos1_trigger()
* @	��������: ���͸��ٿ���Ϣ��kitetool
* @	�������: socket -- ��kitetool���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_pos1_trigger(int socket, ITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	one_range_info_t	*trigger_range 	= NULL;
	int				len				= 0;

	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(one_range_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;
	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_POS1_TRIGGER;
	trigger_range = (one_range_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	trigger_range->point_num = 2;
	trigger_range->state = 1;
	
	trigger_range->point[0].x = dynamic_param->trigger_x0;
	trigger_range->point[0].y = dynamic_param->trigger_y0;
	trigger_range->point[1].x = dynamic_param->trigger_x1;
	trigger_range->point[1].y = dynamic_param->trigger_y1;
	send(socket, send_buf, len, 0);
	
	return 0;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	one_range_info_t	trigger_range 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(one_range_info_t);

	msg_len = htons(len);
	msg_type = 0xA0;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - sizeof(unsigned short) - sizeof(unsigned char);
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_POS1_TRIGGER;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, sizeof(track_header_t));

	trigger_range.point_num = 2;
	trigger_range.state = 1;
	
	trigger_range.point[0].x = dynamic_param->TrackOtherParam.pos1_trigger.trigger_x0;
	trigger_range.point[0].y = dynamic_param->TrackOtherParam.pos1_trigger.trigger_y0;
	trigger_range.point[1].x = dynamic_param->TrackOtherParam.pos1_trigger.trigger_x1;
	trigger_range.point[1].y = dynamic_param->TrackOtherParam.pos1_trigger.trigger_y1;

	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &trigger_range, sizeof(one_range_info_t));

	send(socket, send_buf, len, 0);
	return 0;
#endif
}

/**
* @	��������: set_pos1_trigger_sum()
* @	��������: ����ȫ������λ�Ĵ��������,Ĭ��ֵΪ20��
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_pos1_trigger_sum(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	trigger_num_info_t *trigger_num_info = NULL;
	

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	trigger_num_info = (trigger_num_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);

	dynamic_param->TrackOtherParam.pos1_trigger_num= trigger_num_info->trigger_num;
	dynamic_param->reset_level = RE_INIT;

	printf( "yyyyyyyyyyy recv pos1_trigger_num = %d\n",dynamic_param->TrackOtherParam.pos1_trigger_num);

	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->TrackOtherParam.pos1_trigger_num);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, POS1_TRIGGER_SUM, temp);
	unlock_save_track_mutex();
	return 0;
}

/**
* @	��������: send_pos1_trigger_sum()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_pos1_trigger_sum(int socket, ITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	trigger_num_info_t	*trigger_num_info = NULL;
	int				len				= 0;
	int				index			= 0;


	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(trigger_num_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;

	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= POS1_TRIGGER_SUM;

	trigger_num_info = (trigger_num_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	trigger_num_info->state = 1;
	
	trigger_num_info->trigger_num = dynamic_param->TrackOtherParam.pos1_trigger_num;
	send(socket, send_buf, len, 0);
	return 0;
#else
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
	msg_type = 0xA0;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - 3;
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_POS1_TRIGGER_NUM;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, 8);

	trigger_num_info.state = 1;
	trigger_num_info.trigger_num = dynamic_param->TrackOtherParam.pos1_trigger_num;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &trigger_num_info, sizeof(trigger_num_info_t));

	send(socket, send_buf, len, 0);
	return 0;

#endif
}

/**
* @	��������: set_blackboard_area()
* @	��������: ���ô��������,Ĭ��ֵΪ3��
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
static int set_blackboard_area(unsigned char *data)
{
	default_msg_info_t *blackboard_info = NULL;

	blackboard_info = (default_msg_info_t *)data;

	if(blackboard_info->value>3)
	{
		printf( "yyyyyyyyyyy recv set_blackboard_area val error ,val= %d\n",blackboard_info->value);
		return -1;
	}
	g_track_save_file.set_cmd = SET_BLACKBOARD_POSITION;
	g_track_save_file.cmd_param = blackboard_info->value;
	__get_cam_position();
#ifdef ENC_1200_DEBUG
	PRINTF("recv set_blackboard_area = %d\n",blackboard_info->value);
#else
	printf( "yyyyyyyyyyy recv set_blackboard_area = %d\n",blackboard_info->value);
#endif
	return 0;
}

/**
* @	��������: set_strategy_no()
* @	��������: ���û�λ,Ĭ��ֵΪ����λ
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_strategy_no(unsigned char *data)
{
	default_msg_info_t *strategy_no_info = NULL;

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	strategy_no_info = (default_msg_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);

	g_track_strategy_info.strategy_no= strategy_no_info->value;
	printf( "yyyyyyyyyyy recv strategy_no = %d\n",strategy_no_info->value);

	gStrategy = g_track_strategy_info.strategy_no;

	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", g_track_strategy_info.strategy_no);
	ret =  ConfigSetKey(config_file, TRACK_STRATEGY, STRATEGY_NO, temp);
	unlock_save_track_mutex();
	return 0;
}

/**
* @	��������: send_strategy_no()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_strategy_no(int socket)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	default_msg_info_t	*strategy_no_info = NULL;
	int				len				= 0;
	int				index			= 0;


	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(default_msg_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;

	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_TRIGGER_NUM_TYPE;

	strategy_no_info = (trigger_num_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	strategy_no_info->state = 1;
	
	strategy_no_info->value= g_track_strategy_info.strategy_no;
	send(socket, send_buf, len, 0);
	return 0;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	default_msg_info_t	strategy_no_info 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(default_msg_info_t);

	msg_len = htons(len);
	msg_type = 0xA0;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - 3;
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_STRATEGY_NO;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, 8);

	strategy_no_info.state = 1;
	strategy_no_info.value= g_track_strategy_info.strategy_no;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &strategy_no_info, sizeof(default_msg_info_t));

	send(socket, send_buf, len, 0);
	return 0;

#endif
}

/**
* @	��������: set_meanshift_flag()
* @	��������: ����meanshift ���أ�Ĭ�ϴ�
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_meanshift_flag(unsigned char *data, ITRACK_DynamicParams *dynamic_param)
{
	default_msg_info_t *meanshift_info = NULL;
	

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	meanshift_info = (default_msg_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);

	dynamic_param->TrackOtherParam.meanshift_flag= meanshift_info->value;
	dynamic_param->reset_level = RE_INIT;

	printf( "yyyyyyyyyyy recv meanshift_flag = %d\n",dynamic_param->TrackOtherParam.meanshift_flag);

	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", dynamic_param->TrackOtherParam.meanshift_flag);
	ret =  ConfigSetKey(config_file, DYNAMIC_NAME, MEANSHIFT_FLAG, temp);
	unlock_save_track_mutex();
	return 0;
}

/**
* @	��������: send_meanshift_flag()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_meanshift_flag(int socket, ITRACK_DynamicParams *dynamic_param)
{
#ifdef ENC_1200_DEBUG
	unsigned char send_buf[256] 	= {0};
	
	msg_header_t		*msg_header 	= NULL;
	track_header_t		*track_header 	= NULL;
	default_msg_info_t	*meanshift_info = NULL;
	int				len				= 0;
	int				index			= 0;


	memset(&send_buf, 0, 256);

	msg_header = (msg_header_t *)send_buf;
	len = sizeof(msg_header_t) + sizeof(track_header_t) + sizeof(default_msg_info_t);
	msg_header->nLen = htons(len);
	msg_header->nMsg = 0xA0;

	track_header = (track_header_t *)&send_buf[sizeof(msg_header_t)];
	track_header->len = len - sizeof(msg_header_t);
	track_header->fixd_msg = FIXED_MSG;
	track_header->msg_type	= GET_MEANSHIFT_FLAG;

	meanshift_info = (trigger_num_info_t *)&send_buf[sizeof(msg_header_t) + sizeof(track_header_t)];

	meanshift_info->state = 1;
	
	meanshift_info->value= dynamic_param->TrackOtherParam.meanshift_flag;
	send(socket, send_buf, len, 0);
	return 0;
#else
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	default_msg_info_t	meanshift_info 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(default_msg_info_t);

	msg_len = htons(len);
	msg_type = 0xA0;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - 3;
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= GET_MEANSHIFT_FLAG;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, 8);

	meanshift_info.state = 1;
	meanshift_info.value= dynamic_param->TrackOtherParam.meanshift_flag;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &meanshift_info, sizeof(default_msg_info_t));

	send(socket, send_buf, len, 0);
	return 0;

#endif
}

/**
* @	��������: set_students_panorama_switch_near_time()
* @	��������: ����set_students_panorama_switch_near_time 
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int set_track_strategy_time(unsigned char *data,void *key,int *pTimeVal)
{
	default_msg_info_t *time_info = NULL;
	

	char 	temp[FILE_NAME_LEN]			= {0};
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int 	ret 						= -1;
	
	time_info = (default_msg_info_t *)data;

	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	*pTimeVal= time_info->value;


	lock_save_track_mutex();
	memset(temp, 0, FILE_NAME_LEN);
	sprintf(temp, "%d", time_info->value);
	ret =  ConfigSetKey(config_file, TRACK_STRATEGY, key, temp);
	unlock_save_track_mutex();
	printf( "yyyyyy recv set %s=%d\n",key,time_info->value);
	return 0;
}

/**
* @	��������: send_track_strategy_time()
* @	��������: ���͸��ٿ���Ϣ��encodemanage
* @	�������: socket -- ��encodemanage���ӵ�socket
* @ �������: dynamic_param -- ��ʦ���ٵĶ�̬����
* @ ����ֵ:   ��
*/
static int send_track_strategy_time(int socket,unsigned short nTimeMsgType,unsigned short nTimeVal)
{
	unsigned char send_buf[256] 	= {0};
	unsigned short msg_len			= 0;
	unsigned char	msg_type		= 0;
	track_header_t		track_header 	= {0};
	default_msg_info_t	time_info 	= {0};

	int				len_next		= 0;
	int				len				= 0;

	memset(send_buf, 0, 256);
	len = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t) + sizeof(default_msg_info_t);

	msg_len = htons(len);
	msg_type = 0xA0;

	memcpy(send_buf, &msg_len, sizeof(unsigned short));
	len_next = sizeof(unsigned short);
	memcpy(send_buf + len_next, &msg_type, sizeof(unsigned char));

	track_header.len = len - 3;
	track_header.fixd_msg = FIXED_MSG;
	track_header.msg_type	= nTimeMsgType;

	len_next = sizeof(unsigned short) + sizeof(unsigned char);
	memcpy(send_buf + len_next, &track_header, 8);

	time_info.state = 1;
	time_info.value= nTimeVal;
	
	len_next = sizeof(unsigned short) + sizeof(unsigned char) + sizeof(track_header_t);
	memcpy(send_buf + len_next, &time_info, sizeof(default_msg_info_t));

	send(socket, send_buf, len, 0);
	return 0;
}

/**
* @	��������: set_teacher_track_param()
* @	��������: ���ý�ʦ���ٲ���
* @	�������: data -- ��encodemanage���յ������ò�������
* @ �������: ��
* @ ����ֵ:   ��
*/
int set_teacher_track_param(unsigned char *data, int socket)
{

	track_header_t *track_header = (track_header_t *)data;
	ITRACK_DynamicParams	*pdynamic_param = NULL;
	
	pdynamic_param = &gEduKit->osd_dspAlg_Link[0].create_params.trackCreateParams.TrackParms.dynamic_param;
	
	printf( "set_teacher_track_param = %x\n",track_header->msg_type);
	
	switch(track_header->msg_type)
	{
		case SET_TRACK_RANGE:
			set_track_range(data+sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;
			
		case SET_TRIGGER_RANGE:
			set_trigger_range(data+sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;
		case SET_TRACK_LIMIT_HEIGHT:
			set_limit_height(data+sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;
		case SET_TRACK_TYPE:
			set_track_type(data+sizeof(track_header_t), pdynamic_param);
			
			g_recontrol_flag = 1;
			//printf("22#######################g_recontrol_flag = %d\n",g_recontrol_flag);
			break;
		case SET_DRAW_LINE_TYPE:
			set_draw_line_type(data+sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;
		case SET_TRIGGER_NUM_TYPE:
			set_trigger_sum(data+sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;
		case SET_CAM_SPEED_TYPE:
			set_camara_speed(data+sizeof(track_header_t), &g_cam_info);
			//g_recontrol_flag = 0;
			break;
		case SET_PRESET_POSITION_TYPE:
			set_preset_position(data+sizeof(track_header_t));
			//g_recontrol_flag = 0;
			break;
		case SET_LIMIT_POSITION_TYPE:
			set_or_clear_limit_position(data+sizeof(track_header_t));
			//g_recontrol_flag = 0;
			break;
			
		case CALL_PRESET_POSITION_TYPE:
			call_preset_position(data+sizeof(track_header_t));
			break;
			
		case SET_TRACK_DISTANCE_TYPE:
			set_track_distance(data+sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;
		case SET_TRACK_IS_ENCODE:
			set_track_is_encode(data+sizeof(track_header_t));
			//g_recontrol_flag = 0;
			break;
		case SET_TRACK_PARAM:
			set_track_param(data+sizeof(track_header_t),track_header->len - sizeof(track_header_t));
			//g_recontrol_flag = 0;
			break;
			
		case SET_START_TRACK_TIME:
			set_start_track_time(data+sizeof(track_header_t),pdynamic_param);
			g_recontrol_flag = 1;
			break;
		case SET_RESET_TIME:
			set_reset_time(data+sizeof(track_header_t),pdynamic_param);
			g_recontrol_flag = 1;
			break;
			
		case SET_SENS_VALUE:
			set_sens_value(data+sizeof(track_header_t),pdynamic_param);
			g_recontrol_flag = 1;
			break;
		case SET_MANUAL_COMMOND:
			set_manual_commond(data+sizeof(track_header_t));
			break;	
		case SET_MEANSHIFT_TRIGGER:
			set_meanshift_trigger(data+sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;	
		case SET_POS1_TRIGGER:
			set_pos1_trigger(data+sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;	
		case SET_POS1_TRIGGER_SUM:
			set_pos1_trigger_sum(data+sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;	
		case SET_BLACKBOARD_AREA:
			set_blackboard_area(data+sizeof(track_header_t));
			break;	
		case SET_STRATEGY_NO:
			set_strategy_no(data+sizeof(track_header_t));
			
			break;	
		case SET_MEANSHIFT_FLAG:
			set_meanshift_flag(data+sizeof(track_header_t), pdynamic_param);
			g_recontrol_flag = 1;
			break;	
		case SET_STUDENTS_PANORAMA_SWITCH_NEAR_TIME:
			set_track_strategy_time(data+sizeof(track_header_t),STUDENTS_PANORAMA_SWITCH_NEAR_TIME,&g_track_strategy_info.students_panorama_switch_near_time);
			break;	
		case SET_TEACHER_BLACKBOARD_TIME1:
			set_track_strategy_time(data+sizeof(track_header_t),TEACHER_BLACKBOARD_TIME1,&g_track_strategy_info.teacher_blackboard_time1);
			break;	
		case SET_TEACHER_LEAVE_BLACKBOARD_TIME1:
			set_track_strategy_time(data+sizeof(track_header_t),TEACHER_LEAVE_BLACKBOARD_TIME1,&g_track_strategy_info.teacher_leave_blackboard_time1);
			break;	
		case SET_STUDENTS_DOWN_TIME:
			set_track_strategy_time(data+sizeof(track_header_t),STUDENTS_DOWN_TIME,&g_track_strategy_info.students_down_time);
			break;	
		case SET_TEACHER_PANORAMA_TIME:
			set_track_strategy_time(data+sizeof(track_header_t),TEACHER_PANORAMA_TIME,&g_track_strategy_info.teacher_panorama_time);
			break;	
		case SET_TEACHER_LEAVE_PANORAMA_TIME:
			set_track_strategy_time(data+sizeof(track_header_t),TEACHER_LEAVE_PANORAMA_TIME,&g_track_strategy_info.teacher_leave_panorama_time);
			break;	
		case SET_TEACHER_KEEP_PANORAMA_TIME:
			set_track_strategy_time(data+sizeof(track_header_t),TEACHER_KEEP_PANORAMA_TIME,&g_track_strategy_info.teacher_keep_panorama_time);
			break;	
		case SET_TEACHER_SWITCH_STUDENTS_DELAY_TIME:
			set_track_strategy_time(data+sizeof(track_header_t),TEACHER_SWITCH_STUDENTS_DELAY_TIME,&g_track_strategy_info.teacher_switch_students_delay_time);
			break;	
		case SET_STUDENTS_NEAR_KEEP_TIME:
			set_track_strategy_time(data+sizeof(track_header_t),STUDENTS_NEAR_KEEP_TIME,&g_track_strategy_info.students_near_keep_time);
			break;	
		case SET_MV_KEEP_TIME:
			set_track_strategy_time(data+sizeof(track_header_t),MV_KEEP_TIME,&g_track_strategy_info.mv_keep_time);
			break;	
		case SET_VGA_KEEP_TIME:
			set_track_strategy_time(data+sizeof(track_header_t),VGA_KEEP_TIME,&g_track_strategy_info.vga_keep_time);
			break;	
		case SET_TEACHER_BLACKBOARD_TIME2:
			set_track_strategy_time(data+sizeof(track_header_t),TEACHER_BLACKBOARD_TIME2,&g_track_strategy_info.teacher_blackboard_time2);
			break;	
		case SET_TEACHER_LEAVE_BLACKBOARD_TIME2:
			set_track_strategy_time(data+sizeof(track_header_t),TEACHER_LEAVE_BLACKBOARD_TIME2,&g_track_strategy_info.teacher_leave_blackboard_time2);
			break;	
			
/////////////////////////	��ȡ��������		
		case GET_TRACK_RANGE:
			send_track_range(socket,pdynamic_param);
			//g_recontrol_flag = 0;
			break;
			
		case GET_TRIGGER_RANGE:
			send_trigger_range(socket,pdynamic_param);
			//g_recontrol_flag = 0;
			break;
			
		case GET_TRACK_LIMIT_HEIGHT:
			send_limit_height(socket,pdynamic_param);
			//g_recontrol_flag = 0;
			break;
			
		case GET_TRACK_TYPE:
			send_track_type(socket,pdynamic_param);
			//g_recontrol_flag = 0;
			break;
			
		case GET_DRAW_LINE_TYPE:
			send_draw_line_type(socket,pdynamic_param);
			//g_recontrol_flag = 0;
			break;
			
		case GET_TRIGGER_NUM_TYPE:
			send_trigger_num(socket,pdynamic_param);
			//g_recontrol_flag = 0;
			break;
			
		case GET_CAM_SPEED_TYPE:
			send_cam_speed(socket,&g_cam_info);
			//g_recontrol_flag = 0;
			break;
			
		case GET_TRACK_DISTANCE_TYPE:
			send_track_distance(socket,pdynamic_param);
			//g_recontrol_flag = 0;
			break;
		case GET_TRACK_IS_ENCODE:
			send_track_is_encode(socket, g_track_encode_info.is_encode);
			break;
		case GET_TRACK_PARAM:
			send_track_param(socket);
			//g_recontrol_flag = 0;1368
			break;
			
		case GET_START_TRACK_TIME:
			send_start_track_time(socket,pdynamic_param);
			//g_recontrol_flag = 0;
			break;
			
		case GET_RESET_TIME:
			send_reset_time(socket,pdynamic_param);
			//g_recontrol_flag = 0;
			break;
		case GET_SENS_VALUE:
			send_sens_value(socket,pdynamic_param);
			//g_recontrol_flag = 0;
			break;
		case GET_MEANSHIFT_TRIGGER:
			send_meanshift_trigger(socket,pdynamic_param);
			break;	
		case GET_POS1_TRIGGER:
			send_pos1_trigger(socket,pdynamic_param);
			break;	
		case GET_POS1_TRIGGER_NUM:
			send_pos1_trigger_sum(socket,pdynamic_param);
			break;	
		case GET_STRATEGY_NO:
			send_strategy_no(socket);
			break;	
		case GET_MEANSHIFT_FLAG:
			send_meanshift_flag(socket,pdynamic_param);
			break;	
		case GET_STUDENTS_PANORAMA_SWITCH_NEAR_TIME:
			send_track_strategy_time(socket,GET_STUDENTS_PANORAMA_SWITCH_NEAR_TIME,g_track_strategy_info.students_panorama_switch_near_time);
			break;	
		case GET_TEACHER_BLACKBOARD_TIME1:
			send_track_strategy_time(socket,GET_TEACHER_BLACKBOARD_TIME1,g_track_strategy_info.teacher_blackboard_time1);
			break;	
		case GET_TEACHER_LEAVE_BLACKBOARD_TIME1:
			send_track_strategy_time(socket,GET_TEACHER_LEAVE_BLACKBOARD_TIME1,g_track_strategy_info.teacher_leave_blackboard_time1);
			break;	
		case GET_STUDENTS_DOWN_TIME:
			send_track_strategy_time(socket,GET_STUDENTS_DOWN_TIME,g_track_strategy_info.students_down_time);
			break;	
		case GET_TEACHER_PANORAMA_TIME:
			send_track_strategy_time(socket,GET_TEACHER_PANORAMA_TIME,g_track_strategy_info.teacher_panorama_time);
			break;	
		case GET_TEACHER_LEAVE_PANORAMA_TIME:
			send_track_strategy_time(socket,GET_TEACHER_LEAVE_PANORAMA_TIME,g_track_strategy_info.teacher_leave_panorama_time);
			break;	
		case GET_TEACHER_KEEP_PANORAMA_TIME:
			send_track_strategy_time(socket,GET_TEACHER_KEEP_PANORAMA_TIME,g_track_strategy_info.teacher_keep_panorama_time);
			break;	
		case GET_TEACHER_SWITCH_STUDENTS_DELAY_TIME:
			send_track_strategy_time(socket,GET_TEACHER_SWITCH_STUDENTS_DELAY_TIME,g_track_strategy_info.teacher_switch_students_delay_time);
			break;	
		case GET_STUDENTS_NEAR_KEEP_TIME:
			send_track_strategy_time(socket,GET_STUDENTS_NEAR_KEEP_TIME,g_track_strategy_info.students_near_keep_time);
			break;	
		case GET_MV_KEEP_TIME:
			send_track_strategy_time(socket,GET_MV_KEEP_TIME,g_track_strategy_info.mv_keep_time);
			break;	
		case GET_VGA_KEEP_TIME:
			send_track_strategy_time(socket,GET_VGA_KEEP_TIME,g_track_strategy_info.vga_keep_time);
			break;	
		case GET_TEACHER_BLACKBOARD_TIME2:
			send_track_strategy_time(socket,GET_TEACHER_BLACKBOARD_TIME2,g_track_strategy_info.teacher_blackboard_time2);
			break;	
		case GET_TEACHER_LEAVE_BLACKBOARD_TIME2:
			send_track_strategy_time(socket,GET_TEACHER_LEAVE_BLACKBOARD_TIME2,g_track_strategy_info.teacher_leave_blackboard_time2);
			break;	
		default
				:
			//g_recontrol_flag = 0;
			break;
	}
	
	if(g_recontrol_flag == 1)
	{
		System_linkControl(gEduKit->osd_dspAlg_Link[0].link_id,
		                   ALG_LINK_TRACK_CMD_SET_CHANNEL_WIN_PRM,
		                   &gEduKit->osd_dspAlg_Link[0].create_params.trackCreateParams,
		                   sizeof(AlgLink_TrackCreateParams),
		                   FALSE);
		                   
		g_recontrol_flag = 0;
	}
	
	return 0;
}



/**
* @ ��������: ctrl_position_preset()
* @ ��������: ��������ͷת��Ԥ��λ
* @ �������: cam_position -- Ԥ��λ���
* @ �������: data -- ������ͷ���͵����������
* @ ����ֵ:   ���͵����ݵĳ���
*/
static int ctrl_position_preset(unsigned char *data, int cam_position)
{
	int size = 0;
	
	if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
	{
		int 					fd							= gRemoteFD;
		
		if(cam_position > 15)
		{
			cam_position = 0;
		}
		
		printf("#########cam_position = %d\n",cam_position);
		unsigned char	cmd_data[20]	= {0};
		
		cmd_data[0] = 0x81;
		cmd_data[1] = 0x01;
		cmd_data[2] = 0x04;
		cmd_data[3] = 0x3F;
		cmd_data[4] = 0x02;
		
		cmd_data[5] = cam_position;
		cmd_data[6] = 0xff;
		size = 7;
		SendDataToCom(fd, cmd_data, size);
	}
	else
	{
		g_cam_info.control_flag = MOVE_ZOOM;
		g_cam_info.cam_position_value	= cam_position;
	}
	return 0;
}


/**
* @	��������: ctrl_cam_rotation()
* @ ��������: ��������ͷ����ת������ֹͣת��
* @ �������: direction -- �ƶ�����
* @			  distance -- �ƶ�����
* @ �������: data	-- ������ͷ���͵��������ݰ�
* @ ����ֵ:   ���ݰ�����
*/
static int ctrl_cam_rotation(unsigned char *data, int direction, int distance)
{
	int size = 0;
	
	//����ֹͣת��
	if(CAMERA_MOVE_STOP == direction)
	{
		data[0] = 0x81;
		data[1] = 0x01;
		data[2] = 0x06;
		data[3] = 0x01;
		data[4] = 0x01;
		data[5] = 0x01;
		data[6] = 0x03;
		data[7] = 0x03;
		data[8] = 0xff;
		size = 9;
	}
	else
	{
		//����ת��
		data[0] = 0x81;
		data[1] = 0x01;
		data[2] = 0x06;
		data[3] = 0x01;
		data[4] = distance/g_cam_info.cam_speed+1;
		if (data[4] >0x18)
		{
			data[4] = 0x18;
		}
		data[5] = 0x01;
		if(CAMERA_MOVE_LEFT == direction)
		{
			data[6] = 0x02;
		}
		else if(CAMERA_MOVE_RIGHT == direction)
		{
			data[6] = 0x01;
		}
		data[7] = 0x03;
		data[8] = 0xff;
		size = 9;
	}
	return size;
}

/**
* @	��������: package_cam_cmd()
* @ ��������: ��������ͷ����
* @ �������: output_param -- �����㷨���صĲ���
* @ �������: data	-- ������ͷ���͵��������ݰ�
* @ ����ֵ:   ���ݰ�����
*/
static int package_cam_cmd(unsigned char *data, ITRACK_OutArgs *output_param)
{
	int ret = 0;
	int cam_speed = 0;
	
	if(output_param->cam_position > 42 )
	{
		assert(0);
		return 0 ;
	}
	
	switch(output_param->cmd_type)
	{
		case	CAM_CTRL_POSITION:	//��������ͷ��ת��Ԥ��λ
#ifdef ENC_1200_DEBUG
			PRINTF("reset position output_param->cam_position = %u\n",output_param->cam_position);
#else
			printf("reset position output_param->cam_position = %u\n",output_param->cam_position);
#endif
			
			ret = ctrl_position_preset(data, output_param->cam_position);
			break;
		case	CAM_CTRL_ROTATION:	//��������ͷת��������
			if(CAMERA_MOVE_STOP == output_param->move_direction)
			{
				g_track_strategy_info.move_flag	=	0;
			}
			else
			{
				g_track_strategy_info.move_flag =	1;
			}
			cam_speed = output_param->move_distance/g_cam_info.cam_speed+1;
			
			if(cam_speed > 0x18)
			{
				cam_speed = 0x18;
			}
			if((output_param->move_direction == g_cam_info.cam_last_direction)
			        && (cam_speed == g_cam_info.cam_last_speed))
			{
				ret = 0;
			}
			else
			
			{
			
			
				g_cam_info.cam_last_direction = output_param->move_direction;
				g_cam_info.cam_last_speed = cam_speed;
				ret = ctrl_cam_rotation(data, output_param->move_direction, output_param->move_distance);
			}
			break;
		case	CAM_CTRL_STOP:		//��������ͷֹͣת������
		
			break;
		case	CAM_CTRL_ZOOM:		//��������ͷ�������
		
			break;
		default
				:
			break;
	}
	return (ret);
}


/**
* @	��������: cam_ctrl_cmd()
* @ ��������: ��Ӧ�����㷨,��������ͷ����ת������
* @ �������: output_param -- �����㷨���صĲ���
* @ �������: ��
* @ ����ֵ:   ��
*/

int cam_ctrl_cmd(ITRACK_OutArgs *output_param)
{
	unsigned char 	cmd_code[CTRL_CMD_LEN] 	= {0};
	int				size 					= 0;
	int 			fd 						= gRemoteFD;
//	int				send_students;
	int				nTeacherDownFlag=0;

  //output_param->track_status = 1;
	//���ٶ�ʧ
	if((g_track_encode_info.track_status != 0)&&(output_param->track_status == 0))
	{
		printf( "yyyyyyyyyyyyyy teacher down to students!\n");
		nTeacherDownFlag=1;
		if(g_tFinalClassInfo.nClassInfoFlag == 1)
		{
			g_tFinalClassInfo.tSendClassInfo.nTeacherToStudentsAreaTimes++;
			printf( "yyyyyyyyyyyyyy nTeacherToStudentsAreaTimes = %d\n",g_tFinalClassInfo.tSendClassInfo.nTeacherToStudentsAreaTimes);
		}
	}

	if(g_track_encode_info.track_status != output_param->track_status)
	{
		zlog_debug(ptrackzlog,"[TEACHER] teacher is %s\n", (output_param->track_status == 1)?"tracking":"losing");
	}
	
	g_track_encode_info.track_status = output_param->track_status;
	
	memset(cmd_code, 0, CTRL_CMD_LEN);

	
	//printf("---------------> %d %d\n",output_param->cmd_type,output_param->cam_position);
	if(CAM_CTRL_POSITION == output_param->cmd_type)
	{
		if(TRIGGER_POSITION1 == output_param->cam_position)
		{
			g_track_strategy_info.switch_cmd_author = AUTHOR_TEACHER;
			g_track_strategy_info.send_cmd = SWITCH_STUDENTS;

			zlog_debug(ptrackzlog,"[TEACHER] Fome Teach Switch cmd: SWITCH_STUDENTS  author: AUTHOR_TEACHER\n");
		}
		else
		{
			g_track_strategy_info.switch_cmd_author = AUTHOR_TEACHER;
			g_track_strategy_info.send_cmd = SWITCH_TEATHER;
			zlog_debug(ptrackzlog,"[TEACHER] Fome Teach Switch cmd: SWITCH_TEATHER  author: AUTHOR_TEACHER\n");
		}
	}
	
	if(MOVE_ZOOM == g_cam_info.control_flag)
	{
		size = jump_zoom_position(cmd_code, g_cam_info.cam_position_value);
		g_cam_info.control_flag = MOVE_PAN_TILT;
	}
	else if(MOVE_PAN_TILT == g_cam_info.control_flag)
	{
		size = jump_absolute_position(cmd_code, g_cam_info.cam_position_value);
		g_cam_info.control_flag = MOVE_NOT;
	}
	else
	{
		size = package_cam_cmd(cmd_code, output_param);
	}
	
	if(size > 0)
	{
		SendDataToComNoDelay(fd, cmd_code, size);
	}
	
	return 0;
}

int server_set_track_type(short type)
{


	ITRACK_DynamicParams *pdynamic_param = &gEduKit->osd_dspAlg_Link[0].create_params.trackCreateParams.TrackParms.dynamic_param;
	if((AUTO_CONTROL == pdynamic_param->control_mode) && (AUTO_CONTROL == type))
	{
		//return 0;
	}

	pdynamic_param->control_mode = type;
	pdynamic_param->reset_level = RE_START;
	if(AUTO_CONTROL == pdynamic_param->control_mode)
	{
		__call_preset_position(TRIGGER_POSITION1);
	}

	System_linkControl(gEduKit->osd_dspAlg_Link[0].link_id,
		                   ALG_LINK_TRACK_CMD_SET_CHANNEL_WIN_PRM,
		                   &gEduKit->osd_dspAlg_Link[0].create_params.trackCreateParams,
		                   sizeof(AlgLink_TrackCreateParams),
		                   FALSE);

	return 0;
}


/**
* @	��������: track_init()
* @	��������: ��ʼ�����ٲ���
* @ �������:
* @ �������:
* @ ����ֵ:
*/
int track_init(ITRACK_Params *track_param)
{
	int 	ret = -1;
	char 	config_file[FILE_NAME_LEN] 	= {0};
	char 	temp[FILE_NAME_LEN]			= {0};
	char	text[FILE_NAME_LEN]			= {0};
	char	param_name[FILE_NAME_LEN] 	= {0};
	int		index						= 0;
	int		i 							= 0;
//	unsigned char data[5]				= {0};
	
	memset(track_param,0,sizeof(ITRACK_Params));
	track_param->size = sizeof(ITRACK_Params);
	//��Ƶ���
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, VIDEO_WIDTH, temp);
	track_param->static_param.video_width = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->static_param.video_width = %d\n",track_param->static_param.video_width);
#else
	printf("track_param->static_param.video_width = %d\n",track_param->static_param.video_width);
#endif
	if(ret != 0)
	{
		track_param->static_param.video_width = 1280;
	}
	
	
	//��Ƶ�߶�
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, VIDEO_HEIGHT, temp);
	track_param->static_param.video_height = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->static_param.video_height = %d\n",track_param->static_param.video_height);
#else
	printf("track_param->static_param.video_height = %d\n",track_param->static_param.video_height);
#endif
	if(ret != 0)
	{
		track_param->static_param.video_height = 720;
	}
	
	//���ٴ���ͼ��Ŀ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, PIC_WIDTH, temp);
	track_param->static_param.pic_width = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->static_param.pic_width = %d\n",track_param->static_param.pic_width);
#else
	printf("track_param->static_param.pic_width = %d\n",track_param->static_param.pic_width);
#endif
	if(ret != 0)
	{
		track_param->static_param.pic_width = 160;
	}
	
	//���ٴ���ͼ��ĸ߶�
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, STATIC_NAME, PIC_HEIGHT, temp);
	track_param->static_param.pic_height = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->static_param.pic_height = %d\n",track_param->static_param.pic_height);
#else
	printf("track_param->static_param.pic_height = %d\n",track_param->static_param.pic_height);
#endif
	if(ret != 0)
	{
		track_param->static_param.pic_height = 90;
	}
	
	//�����д�����ʼ���ٵ�ʱ��,�ֽ�Ԥ����ʱ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, START_TRACK_TIME, temp);
	track_param->dynamic_param.start_track_time = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.start_track_time = %d\n",track_param->dynamic_param.start_track_time);
#else
	printf("track_param->dynamic_param.start_track_time = %d\n",track_param->dynamic_param.start_track_time);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.start_track_time = 80;
	}
	
	//�����ж�ʧĿ���,���´����¸���ʱ��,�ֽи�λʱ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, RESET_TIME, temp);
	track_param->dynamic_param.reset_time = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.reset_time = %d\n",track_param->dynamic_param.reset_time);
#else
	printf("track_param->dynamic_param.reset_time = %d\n",track_param->dynamic_param.reset_time);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.reset_time = 80;
	}
	
	//��̬�����Ķ�ȡ
	//�����Ľ���Զ��ѡ��,ֵԽ��,��������ԽԶ,��0Ϊ�������
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, ZOOM_DISTACE, temp);
	track_param->dynamic_param.zoom_distance = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.zoom_distance = %d\n",track_param->dynamic_param.zoom_distance);
#else
	printf("track_param->dynamic_param.zoom_distance = %d\n",track_param->dynamic_param.zoom_distance);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.zoom_distance= 0;
	}
	
	//���������ű���
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, X_OFFSET, temp);
	track_param->dynamic_param.x_offset = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.x_offset = %d\n",track_param->dynamic_param.x_offset);
#else
	printf("track_param->dynamic_param.x_offset = %d\n",track_param->dynamic_param.x_offset);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.x_offset = 8;
	}
	
	//���������ű���
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, Y_OFFSET, temp);
	track_param->dynamic_param.y_offset = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.y_offset = %d\n",track_param->dynamic_param.y_offset);
#else
	printf("track_param->dynamic_param.y_offset = %d\n",track_param->dynamic_param.y_offset);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.y_offset = 8;
	}
	
	//����������
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRIGGER_SUM, temp);
	track_param->dynamic_param.trigger_sum = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.trigger_sum = %d\n",track_param->dynamic_param.trigger_sum);
#else
	printf("track_param->dynamic_param.trigger_sum = %d\n",track_param->dynamic_param.trigger_sum);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.trigger_sum = 3;
	}
	//ȫ������������
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, POS1_TRIGGER_SUM, temp);
	track_param->dynamic_param.TrackOtherParam.pos1_trigger_num = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.TrackOtherParam.pos1_trigger_num = %d\n",track_param->dynamic_param.TrackOtherParam.pos1_trigger_num);
#else
	printf( "track_param->dynamic_param.TrackOtherParam.pos1_trigger_num = %d\n",track_param->dynamic_param.TrackOtherParam.pos1_trigger_num);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.TrackOtherParam.pos1_trigger_num = 15;
	}


	//������������϶����xֵ,������0
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRIGGER0_X0, temp);
	track_param->dynamic_param.trigger[0].trigger_x0 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.trigger[0].trigger_x0 = %d\n",track_param->dynamic_param.trigger[0].trigger_x0);
#else
	printf( "track_param->dynamic_param.trigger[0].trigger_x0 = %d\n",track_param->dynamic_param.trigger[0].trigger_x0);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.trigger[0].trigger_x0 = 70;
	}
	
	//������������϶����Yֵ,������0
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRIGGER0_Y0, temp);
	track_param->dynamic_param.trigger[0].trigger_y0 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.trigger[0].trigger_y0 = %d\n",track_param->dynamic_param.trigger[0].trigger_y0);
#else
	printf( "track_param->dynamic_param.trigger[0].trigger_y0 = %d\n",track_param->dynamic_param.trigger[0].trigger_y0);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.trigger[0].trigger_y0 = 18;
	}
	
	//������������¶����xֵ,������0
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRIGGER0_X1, temp);
	track_param->dynamic_param.trigger[0].trigger_x1 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.trigger[0].trigger_x1 = %d\n",track_param->dynamic_param.trigger[0].trigger_x1);
#else
	printf( "track_param->dynamic_param.trigger[0].trigger_x1 = %d\n",track_param->dynamic_param.trigger[0].trigger_x1);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.trigger[0].trigger_x1 = 100;
	}
	
	//������������¶����yֵ,������0
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRIGGER0_Y1, temp);
	track_param->dynamic_param.trigger[0].trigger_y1 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.trigger[0].trigger_y1 = %d\n",track_param->dynamic_param.trigger[0].trigger_y1);
#else
	printf( "track_param->dynamic_param.trigger[0].trigger_y1 = %d\n",track_param->dynamic_param.trigger[0].trigger_y1);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.trigger[0].trigger_y1 = 24;
	}
	
	
	//������������϶����xֵ,������1
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRIGGER1_X0, temp);
	track_param->dynamic_param.trigger[1].trigger_x0 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.trigger[1].trigger_x0 = %d\n",track_param->dynamic_param.trigger[1].trigger_x0);
#else
	printf( "track_param->dynamic_param.trigger[1].trigger_x0 = %d\n",track_param->dynamic_param.trigger[1].trigger_x0);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.trigger[1].trigger_x0 = 70;
	}
	
	//������������϶����Yֵ,������1
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRIGGER1_Y0, temp);
	track_param->dynamic_param.trigger[1].trigger_y0 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.trigger[1].trigger_y0 = %d\n",track_param->dynamic_param.trigger[1].trigger_y0);
#else
	printf( "track_param->dynamic_param.trigger[1].trigger_y0 = %d\n",track_param->dynamic_param.trigger[1].trigger_y0);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.trigger[1].trigger_y0 = 40;
	}
	
	//������������¶����xֵ,������1
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRIGGER1_X1, temp);
	track_param->dynamic_param.trigger[1].trigger_x1 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.trigger[1].trigger_x1 = %d\n",track_param->dynamic_param.trigger[1].trigger_x1);
#else
	printf( "track_param->dynamic_param.trigger[1].trigger_x1 = %d\n",track_param->dynamic_param.trigger[1].trigger_x1);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.trigger[1].trigger_x1 = 100;
	}
	
	//������������¶����yֵ,������1
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRIGGER1_Y1, temp);
	track_param->dynamic_param.trigger[1].trigger_y1 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.trigger[1].trigger_y1 = %d\n",track_param->dynamic_param.trigger[1].trigger_y1);
#else
	printf( "track_param->dynamic_param.trigger[1].trigger_y1 = %d\n",track_param->dynamic_param.trigger[1].trigger_y1);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.trigger[1].trigger_y1 = 46;
	}

	//����Һ���������췶Χ�����϶����xֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MEANSHIFT_TRIGGER_X0, temp);
	track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x0 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x0 = %d\n",track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x0);
#else
	printf( "track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x0 = %d\n",track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x0);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x0 = 0;
	}

	//����Һ���������췶Χ�����϶����Yֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MEANSHIFT_TRIGGER_Y0, temp);
	track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y0 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y0 = %d\n",track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y0);
#else
	printf( "track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y0 = %d\n",track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y0);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y0 = track_param->static_param.video_height*2/3;
	}

	//����Һ���������췶Χ�����¶����xֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MEANSHIFT_TRIGGER_X1, temp);
	track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x1 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x1 = %d\n",track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x1);
#else
	printf( "track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x1 = %d\n",track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x1);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_x1 = track_param->static_param.video_width-1;
	}

	//����Һ���������췶Χ�����¶����yֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MEANSHIFT_TRIGGER_Y1, temp);
	track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y1 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y1 = %d\n",track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y1);
#else
	printf( "track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y1 = %d\n",track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y1);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.TrackOtherParam.meanshift_trigger.trigger_y1 = track_param->static_param.video_height*7/9;
	}

	//����ȫ����ⷶΧ�����϶����xֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, POS1_TRIGGER_X0, temp);
	track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x0 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x0 = %d\n",track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x0);
#else
	printf( "track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x0 = %d\n",track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x0);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x0 = 0;
	}

	//����ȫ����ⷶΧ�����϶����Yֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, POS1_TRIGGER_Y0, temp);
	track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y0 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y0 = %d\n",track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y0);
#else
	printf( "track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y0 = %d\n",track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y0);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y0 = track_param->static_param.video_height*7/12;
	}

	//����ȫ����ⷶΧ�����¶����xֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, POS1_TRIGGER_X1, temp);
	track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x1 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x1 = %d\n",track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x1);
#else
	printf( "track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x1 = %d\n",track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x1);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_x1 = track_param->static_param.video_width-1;
	}

	//����ȫ����ⷶΧ�����¶����yֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, POS1_TRIGGER_Y1, temp);
	track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y1 = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y1 = %d\n",track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y1);
#else
	printf( "track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y1 = %d\n",track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y1);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.TrackOtherParam.pos1_trigger.trigger_y1 = track_param->static_param.video_height*2/3;
	}
	//meanshift flag,Ĭ��Ϊ1����״̬
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MEANSHIFT_FLAG, temp);
	track_param->dynamic_param.TrackOtherParam.meanshift_flag= atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.TrackOtherParam.meanshift_flag = %d\n",track_param->dynamic_param.TrackOtherParam.meanshift_flag);
#else
	printf( "track_param->dynamic_param.TrackOtherParam.meanshift_flag = %d\n",track_param->dynamic_param.TrackOtherParam.meanshift_flag);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.TrackOtherParam.meanshift_flag = 1;
	}

	//����ģʽ,���Զ�ģʽ�����ֶ�ģʽ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, CONTROL_MODE, temp);
	track_param->dynamic_param.control_mode = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.control_mode = %d\n",track_param->dynamic_param.control_mode);
#else
	printf("track_param->dynamic_param.control_mode = %d\n",track_param->dynamic_param.control_mode);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.control_mode = 0;
	}
	
	//sensֵ,ԽС��Եֵ�ҵ�Խ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, SENS, temp);
	track_param->dynamic_param.sens = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.sens = %d\n",track_param->dynamic_param.sens);
#else
	printf("track_param->dynamic_param.sens = %d\n",track_param->dynamic_param.sens);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.sens = 56;
	}
	
	//���߱�־
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MESSAGE, temp);
	track_param->dynamic_param.message = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.message = %d\n",track_param->dynamic_param.message);
#else
	printf("track_param->dynamic_param.message = %d\n",track_param->dynamic_param.message);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.message = 1;
	}
	
	//ģ�������ǰΪ10��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MODEL_SUM, temp);
	track_param->dynamic_param.model_sum = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.model_sum = %d\n",track_param->dynamic_param.model_sum);
#else
	printf("track_param->dynamic_param.model_sum = %d\n",track_param->dynamic_param.model_sum);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.model_sum = 10;
	}
	
	//track_selĿǰ����Ҫ����,Ĭ��ֵΪ1
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRACK_SEL, temp);
	track_param->dynamic_param.track_sel = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.track_sel = %d\n",track_param->dynamic_param.track_sel);
#else
	printf("track_param->dynamic_param.track_sel = %d\n",track_param->dynamic_param.track_sel);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.track_sel = 1;
	}
	
	//Ϊ���ٴ����ͼ��Ŀ���м�ֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MID_X, temp);
	track_param->dynamic_param.mid_x = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.mid_x = %d\n",track_param->dynamic_param.mid_x);
#else
	printf("track_param->dynamic_param.mid_x = %d\n",track_param->dynamic_param.mid_x);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.mid_x = 80;
	}
	
	//Ϊ���ٴ����ͼ��ĸߵ��м�ֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MID_Y, temp);
	track_param->dynamic_param.mid_y = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.mid_y = %d\n",track_param->dynamic_param.mid_y);
#else
	printf("track_param->dynamic_param.mid_y = %d\n",track_param->dynamic_param.mid_y);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.mid_y = 40;
	}
	
	//���ٵ��޸���,���˵�������߶�ʱ�����ٸ���,��������Ź����ͼ��Ĵ�С���Ե�
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, LIMIT_HEIGHT, temp);
	track_param->dynamic_param.limit_height = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.limit_height = %d\n",track_param->dynamic_param.limit_height);
#else
	printf("track_param->dynamic_param.limit_height = %d\n",track_param->dynamic_param.limit_height);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.limit_height = 60;
	}
	
	//б�߸�����ƥ�䷧ֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MODEL_MULTIPLE, temp);
	track_param->dynamic_param.model_multiple = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.model_multiple = %d\n",track_param->dynamic_param.model_multiple);
#else
	printf("track_param->dynamic_param.model_multiple = %d\n",track_param->dynamic_param.model_multiple);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.model_multiple = 12;
	}
	
	//ģ��ƥ���Ȩ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, MODEL_LEVEL, temp);
	track_param->dynamic_param.model_level = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.model_level = %d\n",track_param->dynamic_param.model_level);
#else
	printf("track_param->dynamic_param.model_level = %d\n",track_param->dynamic_param.model_level);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.model_level = 12;
	}
	
	//�����������
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, DYNAMIC_NAME, TRACK_POINT_NUM, temp);
	track_param->dynamic_param.track_point_num = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.track_point_num = %d\n",track_param->dynamic_param.track_point_num);
#else
	printf("track_param->dynamic_param.track_point_num = %d\n",track_param->dynamic_param.track_point_num);
#endif
	if(ret != 0)
	{
		track_param->dynamic_param.track_point_num = 4;
	}
	
	for(index = 0; index < track_param->dynamic_param.track_point_num; index++)
	{
		//��������������
		memset(config_file, 0, FILE_NAME_LEN);
		strcpy(config_file, TEACH_TRACK_FILE);
		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, TRACK_POINTX);
		sprintf(param_name, "%s%d",text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);
		track_param->dynamic_param.track_point[index].x = atoi(temp);
		
		memset(config_file, 0, FILE_NAME_LEN);
		strcpy(config_file, TEACH_TRACK_FILE);
		memset(text, 0, FILE_NAME_LEN);
		memset(param_name, 0, FILE_NAME_LEN);
		strcpy(text, TRACK_POINTY);
		sprintf(param_name, "%s%d",text, index);
		memset(temp, 0, FILE_NAME_LEN);
		ret =  ConfigGetKey(config_file, DYNAMIC_NAME, param_name, temp);
		track_param->dynamic_param.track_point[index].y = atoi(temp);
#ifdef ENC_1200_DEBUG
		PRINTF("track_param->dynamic_param.track_point[%d].x = %d\n",index,track_param->dynamic_param.track_point[index].x);
		PRINTF("track_param->dynamic_param.track_point[%d].y = %d\n",index,track_param->dynamic_param.track_point[index].y);
#else
		printf("track_param->dynamic_param.track_point[%d].x = %d\n",index,track_param->dynamic_param.track_point[index].x);
		printf("track_param->dynamic_param.track_point[%d].y = %d\n",index,track_param->dynamic_param.track_point[index].y);
#endif
		if(ret != 0)
		{
			track_param->dynamic_param.track_point[0].x	= 0;
			track_param->dynamic_param.track_point[0].y	= 50;
			track_param->dynamic_param.track_point[1].x	= 0;
			track_param->dynamic_param.track_point[1].y	= 500;
			track_param->dynamic_param.track_point[2].x	= 1280;
			track_param->dynamic_param.track_point[2].y	= 500;
			track_param->dynamic_param.track_point[3].x	= 1280;
			track_param->dynamic_param.track_point[3].y	= 50;
			break;
		}
	}
	
	
	
	//����ͷת���ٶȻ���ֵ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, TEACH_CAM_NAME, TEACH_CAM_SPEED, temp);
	g_cam_info.cam_speed = atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("track_param->dynamic_param.model_level = %d\n",g_cam_info.cam_speed);
#else
	printf("track_param->dynamic_param.model_level = %d\n",g_cam_info.cam_speed);
#endif
	if(ret != 0)
	{
		g_cam_info.cam_speed = 16;
	}
	
	
	//����ͷԤ��λ��λ����Ϣ,0��λ����ʱ����,û��ʹ��
	for(index = 1; index < PRESET_NUM_MAX; index++)
	{
		memset(config_file, 0, FILE_NAME_LEN);
		strcpy(config_file, TEACH_TRACK_FILE);
		
		memset(temp, 0, FILE_NAME_LEN);
		strcpy(text, PRESET_POSITION_VALUE);
		sprintf(param_name, "%s%d",text, index);
		
		ret =  ConfigGetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);
#ifdef ENC_1200_DEBUG
		PRINTF("###########index = %d,%s\n",index, temp);
#else
		printf("###########index = %d,%s\n",index, temp);
#endif
		if(ret == 0)
		{
			for(i = 0; i < CAM_PAN_TILT_LEN; i++)
			{
				if((temp[i] >= 0x30) && (temp[i] <= 0x39))
				{
					g_cam_info.cam_position[index].pan_tilt[i] = temp[i] - 0x30;
				}
				if((temp[i] >= 0x41) && (temp[i] <= 0x46))
				{
					g_cam_info.cam_position[index].pan_tilt[i] = temp[i] - 0x41 + 0xa;
				}
				if((temp[i] >= 0x61) && (temp[i] <= 0x66))
				{
					g_cam_info.cam_position[index].pan_tilt[i] = temp[i] - 0x61 + 0xa;
				}
			}
		}
		else
		{
			if(SONY_BRC_Z330 == g_track_cam_model.cam_type)
			{
			
				g_cam_info.cam_position[index].pan_tilt[0] = 0x07;
				g_cam_info.cam_position[index].pan_tilt[1] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[2] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[3] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[4] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[5] = 0x07;
				g_cam_info.cam_position[index].pan_tilt[6] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[7] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[8] = 0x0f;
				
			}
			else
			{
				g_cam_info.cam_position[index].pan_tilt[0] = 0x07;
				g_cam_info.cam_position[index].pan_tilt[1] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[2] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[3] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[4] = 0x07;
				g_cam_info.cam_position[index].pan_tilt[5] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[6] = 0x0f;
				g_cam_info.cam_position[index].pan_tilt[7] = 0x0f;
			}
		}
		
		memset(temp, 0, FILE_NAME_LEN);
		strcpy(text, PRESET_ZOOM_VALUE);
		sprintf(param_name, "%s%d",text, index);
		
		ret =  ConfigGetKey(config_file, TEACH_CAM_PRESET_POSITION, param_name, temp);
		if(ret == 0)
		{
			for(i = 0; i < CAM_ZOOM_LEN; i++)
			{
				if((temp[i] >= 0x30) && (temp[i] <= 0x39))
				{
					g_cam_info.cam_position[index].zoom[i] = temp[i] - 0x30;
				}
				if((temp[i] >= 0x41) && (temp[i] <= 0x46))
				{
					g_cam_info.cam_position[index].zoom[i] = temp[i] - 0x41 + 0xa;
				}
				if((temp[i] >= 0x61) && (temp[i] <= 0x66))
				{
					g_cam_info.cam_position[index].zoom[i] = temp[i] - 0x61 + 0xa;
				}
			}
		}
		else
		{
			g_cam_info.cam_position[index].zoom[0] = 0x07;
			g_cam_info.cam_position[index].zoom[1] = 0x0f;
			g_cam_info.cam_position[index].zoom[2] = 0x0f;
			g_cam_info.cam_position[index].zoom[3] = 0x0f;
		}
		
	}
	
	
	//���ٻ��Ƿ����
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	memset(temp, 0, FILE_NAME_LEN);
	ret =  ConfigGetKey(config_file, TEACH_TRACK_ENCODE, TEACH_IS_ENCODE, temp);
	g_track_encode_info.is_encode= atoi(temp);
#ifdef ENC_1200_DEBUG
	PRINTF("g_track_encode_info.is_encode = %d\n",g_track_encode_info.is_encode);
#else
	printf("g_track_encode_info.is_encode = %d\n",g_track_encode_info.is_encode);
#endif
	if(ret != 0)
	{
		g_track_encode_info.is_encode = 1;
	}	


	//��ȡ�ڰ�1��߱�Եλ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, BLACKBOARD1_POSITION_LEFT);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.left_pan_tilt1 = atoi(temp);
		printf( "init g_track_strategy_info.left_pan_tilt1 = 0x%x\n"
, g_track_strategy_info.left_pan_tilt1);
	}
	else
	{
		g_track_strategy_info.left_pan_tilt1 = 0xfa60;
	}

	//��ȡ�ڰ�1�ұ߱�Եλ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, BLACKBOARD1_POSITION_RIGHT);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.right_pan_tilt1 = atoi(temp);
		printf( "init g_track_strategy_info.right_pan_tilt1 = 0x%x\n"
, g_track_strategy_info.right_pan_tilt1);
	}
	else
	{
		g_track_strategy_info.right_pan_tilt1 = 0x05a0;
	}
	//��ȡ�ڰ�2��߱�Եλ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, BLACKBOARD2_POSITION_LEFT);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.left_pan_tilt2 = atoi(temp);
		printf( "init g_track_strategy_info.left_pan_tilt2 = 0x%x\n"
, g_track_strategy_info.left_pan_tilt2);
	}
	else
	{
		g_track_strategy_info.left_pan_tilt2 = 0xfa60;
	}

	//��ȡ�ڰ�2�ұ߱�Եλ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, BLACKBOARD2_POSITION_RIGHT);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.right_pan_tilt2 = atoi(temp);
		printf( "init g_track_strategy_info.right_pan_tilt2 = 0x%x\n"
, g_track_strategy_info.right_pan_tilt2);
	}
	else
	{
		g_track_strategy_info.right_pan_tilt2 = 0x05a0;
	}
	
	//ѧ��ȫ����ͷ�л���ѧ����д��ͷ����ʱʱ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, STUDENTS_PANORAMA_SWITCH_NEAR_TIME);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.students_panorama_switch_near_time = atoi(temp);
		printf( "init g_track_strategy_info.students_panorama_switch_near_time = 0x%x\n"
		        , g_track_strategy_info.students_panorama_switch_near_time);
	}
	else
	{
		g_track_strategy_info.students_panorama_switch_near_time = 3;
	}

	//����ʦ���پ�ͷ�л������龵ͷ1ʱ����������ʦ�߽�������������л�������
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, TEACHER_BLACKBOARD_TIME1);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.teacher_blackboard_time1 = atoi(temp);
		printf( "init g_track_strategy_info.teacher_blackboard_time1 = 0x%x\n"
, g_track_strategy_info.teacher_blackboard_time1);
	}
	else
	{
		g_track_strategy_info.teacher_blackboard_time1 = 2;
	}

	//�ɰ��龵ͷ�л�����ʦ���پ�ͷ1ʱ����������ʦ�Ӱ��������ߵ������ط��л�����ʦ���پ�ͷ��ʱ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, TEACHER_LEAVE_BLACKBOARD_TIME1);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.teacher_leave_blackboard_time1 = atoi(temp);
		printf( "init g_track_strategy_info.teacher_leave_blackboard_time1 = 0x%x\n"
, g_track_strategy_info.teacher_leave_blackboard_time1);
	}
	else
	{
		g_track_strategy_info.teacher_leave_blackboard_time1 = 2;
	}

	//����ʦ���پ�ͷ�л������龵ͷ2ʱ����������ʦ�߽�������������л�������
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, TEACHER_BLACKBOARD_TIME2);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.teacher_blackboard_time2 = atoi(temp);
		printf( "init g_track_strategy_info.teacher_blackboard_time2 = 0x%x\n"
, g_track_strategy_info.teacher_blackboard_time2);
	}
	else
	{
		g_track_strategy_info.teacher_blackboard_time2 = 2;
	}

	//�ɰ��龵ͷ�л�����ʦ���پ�ͷ2ʱ����������ʦ�Ӱ��������ߵ������ط��л�����ʦ���پ�ͷ��ʱ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, TEACHER_LEAVE_BLACKBOARD_TIME2);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.teacher_leave_blackboard_time2 = atoi(temp);
		printf( "init g_track_strategy_info.teacher_leave_blackboard_time2 = 0x%x\n"
, g_track_strategy_info.teacher_leave_blackboard_time2);
	}
	else
	{
		g_track_strategy_info.teacher_leave_blackboard_time2 = 2;
	}
	
	//ѧ���½�̨ʱ��������ʱ�伴�ٴν���ѧ��������Ϣ����λ����
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, STUDENTS_DOWN_TIME);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.students_down_time= atoi(temp);
		printf( "init g_track_strategy_info.students_down_time = 0x%x\n"
, g_track_strategy_info.students_down_time);
	}
	else
	{
		g_track_strategy_info.students_down_time = 8;
	}
	
	
	
	//�л���ʦȫ��Ҫ����ʱ�����л���ʦȫ��ʱ��ʱһ��ʱ�����л���ʦȫ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, TEACHER_PANORAMA_TIME);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.teacher_panorama_time = atoi(temp);
		printf( "init g_track_strategy_info.teacher_panorama_time = 0x%x\n"
					, g_track_strategy_info.teacher_panorama_time);
	}
	else
	{
		g_track_strategy_info.teacher_panorama_time = 0;
	}
	
	//�л���ʦȫ��Ҫ����ʱ�����л���ʦȫ��ʱ��ʱһ��ʱ�����л���ʦȫ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, TEACHER_LEAVE_PANORAMA_TIME);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.teacher_leave_panorama_time = atoi(temp);
		printf( "init g_track_strategy_info.teacher_leave_panorama_time = 0x%x\n"
		        , g_track_strategy_info.teacher_leave_panorama_time);
	}
	else
	{
		g_track_strategy_info.teacher_leave_panorama_time = 2;
	}
	
	//��ʦȫ�������ʱ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, TEACHER_KEEP_PANORAMA_TIME);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.teacher_keep_panorama_time = atoi(temp);
		printf( "init g_track_strategy_info.teacher_keep_panorama_time = 0x%x\n"
		        , g_track_strategy_info.teacher_keep_panorama_time);
	}
	else
	{
		g_track_strategy_info.teacher_keep_panorama_time = 120;
	}
	
	//ѧ������ʱ������ʦ�л���ѧ����ʱ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, TEACHER_SWITCH_STUDENTS_DELAY_TIME);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.teacher_switch_students_delay_time= atoi(temp);
		printf( "init g_track_strategy_info.teacher_switch_students_delay_time = 0x%x\n"
		        , g_track_strategy_info.teacher_switch_students_delay_time);
	}
	else
	{
		g_track_strategy_info.teacher_switch_students_delay_time= 3;
	}
	
	
	//ѧ��������ͷ���ֵ����ʱ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, STUDENTS_NEAR_KEEP_TIME);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.students_near_keep_time= atoi(temp);
		printf( "init g_track_strategy_info.students_near_keep_time = 0x%x\n"
		        , g_track_strategy_info.students_near_keep_time);
	}
	else
	{
		g_track_strategy_info.students_near_keep_time= 3;
	}
	
	//ѧ��������ͷ���ֵ����ʱ��
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, VGA_KEEP_TIME);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.vga_keep_time= atoi(temp);
		printf( "init g_track_strategy_info.vga_keep_time = 0x%x\n"
		        , g_track_strategy_info.vga_keep_time);
	}
	else
	{
		g_track_strategy_info.vga_keep_time= 5;
	}

	//��ʦ���ˣ������ƶ�Ŀ��ʱѧ��ȫ������ʦȫ����ͷ�л���ʱ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, MV_KEEP_TIME);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.mv_keep_time= atoi(temp);
		printf( "init g_track_strategy_info.mv_keep_time = 0x%x\n"
						, g_track_strategy_info.mv_keep_time);
	}
	else
	{
		g_track_strategy_info.mv_keep_time= 3;
	}

	//��û�λ��Ϣ
	memset(config_file, 0, FILE_NAME_LEN);
	strcpy(config_file, TEACH_TRACK_FILE);
	
	memset(temp, 0, FILE_NAME_LEN);
	strcpy(text, STRATEGY_NO);
	sprintf(param_name, "%s",text);
	
	ret =  ConfigGetKey(config_file, TRACK_STRATEGY, param_name, temp);
	if(ret == 0)
	{
		g_track_strategy_info.strategy_no= atoi(temp);
		gStrategy = g_track_strategy_info.strategy_no;
	printf( "init g_track_strategy_info.strategy_no = 0x%x\n"
						, g_track_strategy_info.strategy_no);
	}
	else
	{
		g_track_strategy_info.strategy_no= 0;
	}
	
	memset(&g_track_strategy_timeinfo,0,sizeof(track_strategy_timeinfo_t));
	g_track_strategy_timeinfo.last_strategy_no=g_track_strategy_info.strategy_no;
	return 0;
	
}



