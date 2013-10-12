/*********************************************************************
公司名称:     Reach Software
文件名:	         remotectrl.h
创 建 人：    yangshaohua                    日期：2009-01-14
修 改 人：    DUHAO                            日期：2010-6-22
功能描述：摄像头远遥控制模块头文件

其他说明： 通过串口通讯控制摄像头的旋转等操作
**********************************************************************/
#ifndef __REMOTECTRL__
#define __REMOTECTRL__
#include "common.h"
#define SLID_MAX_NUM  			11
#define ADDR_MAX_NUM			17   //struct __ADDRESS__ 
#define ADDR_CMD_MAX_NUM		64   //struct __CTRLCODE__

#define UPSTART					0
#define UPSTOP					1

#define DOWNSTART				2
#define DOWNSTOP				3

#define LEFTSTART				4
#define LEFTSTOP				5

#define RIGHTSTART				6
#define RIGHTSTOP				7

#define DADDSTART				8
#define DADDSTOP				9

#define DDECSTART				10
#define DDECSTOP				11

#define REL_POSITION			12
#define DDEC_DIRECT				13

#define ZOOMPOSQUERY			14
#define PAN_TILTPOSQUERY		15

#define POSITIONPRESET			16//预置位
#define POSITIONPRESETSET		17//设置预置位

#define LIMITE_SET              18
#define LIMITE_CLEAR            19

#define CTRL_CODE_MAX_NUM		18   //struct __CTRLCODE__
#define PROTNUM 				5	//远程遥控协议总个数
/*远摇控制结构体配置文件*/
typedef struct __HVSEEPSLID__ {
	BYTE speed[SLID_MAX_NUM];
} HVSSlid;

typedef struct __ADDRESS__ {
	BYTE addr[ADDR_MAX_NUM];
} ADDR;

typedef struct __CTRLCODE__ {
	char comm[ADDR_CMD_MAX_NUM];
} CTRLCODE;

typedef struct __COMPARAM__ {
	int stopbits;
	int databits;
	int parity;
	int baud;
} COMPARAM;
typedef struct tagRECT {
	int    left;
	int    top;
	int    right;
	int    bottom;
} RECT;


typedef struct tagPOINT {
	int  x;
	int  y;
} POINT;
typedef struct __REMOTE_CONFIG__ {
	BYTE ptzName[50];
	/*水平方向*/
	HVSSlid HSSlid;
	/*垂直方向*/
	HVSSlid VSSlid;
	/*指令地址*/
	ADDR Addr;
	/*控制命令*/
	CTRLCODE CCode[CTRL_CODE_MAX_NUM];
	/*串口属性*/
	COMPARAM comm;
} RemoteConfig;


/*摄像头的控制*/
extern int CameraControl();
/*初始化远摇的配置文件*/
extern int InitRemoteStruct(int);

/*摄像头初始化*/
extern int CameraCtrlInit(int com);
/*远摇API */
extern int CCtrlUpStart(int fd, int addr, int speed);
extern int CCtrlUpStop(int fd, int addr);
extern int CCtrlDownStart(int fd, int addr, int speed);
extern int CCtrlDownStop(int fd, int addr);
extern int CCtrlLeftStart(int fd, int addr, int speed);
extern int CCtrlLeftStop(int fd, int addr);
extern int CCtrlRightStart(int fd, int addr, int speed);
extern int CCtrlRightStop(int fd, int addr);
extern int CCtrlZoomAddStart(int fd, int addr);
extern int CCtrlZoomAddStop(int fd, int addr);
extern int CCtrlZoomDecStart(int fd, int addr);
extern int CCtrlZoomDecStop(int fd, int addr);
extern int CCtrlPositionPreset(int fd, int addr, int position);
extern int CCtrlSetPositionPreset(int fd, int addr, int position);
extern void FarCtrlCamera(int dsp, unsigned char *data, int len);

extern int SendDataToCom(int fd,unsigned char *data,int len);

#endif   //__REMOTECTRL__

