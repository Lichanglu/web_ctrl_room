#ifndef _MID_PLATFORM_H__
#define _MID_PLATFORM_H__

#ifdef USE_LINUX_PLATFORM
	#include "mid_linux_platform.h"
#endif

#ifdef USE_WINDOWS_PLATFORM
	#include "mid_win_platform.h"
#endif

#define TCP_BUFF_LEN            1400
//#define RTP_PAYLOAD_LEN         1350
#define RTP_PAYLOAD_LEN         1350

#ifndef GDM_RTSP_SERVER
#define MAX_RTSP_CLIENT_NUM              5
#endif

//add by zhangmin
#define RTSP_VERSION                    "V1.0.0" //rtsp�汾��

//��Ҫ������Эͬ�����ı���
#define AVIIF_KEYFRAME				0x00000010

//�������ֵڼ�·��Դ �����·������
#define MAX_ENCODE_NUM 4

#ifdef GDM_RTSP_SERVER
#define MAX_RTSP_CLIENT_NUM               18
#define PRINTF        				printf
#define ERR_PRN 				PRINTF


//�пش�������
#define APP_CENTER_FILE "D:\\Data\\center_serial.ini"



//defualt center user
#define USER_STRING "viewer"

//default center user passwd ini
#define USER_FILE_NAME "D:\\Data\\rtspuser\\center_control_passwd.ini"
#define RTSP_DIR_NAME  "D:\\Data\\rtspuser"

#endif



#endif
