
#ifndef _SERCENTERCTRL_H_
#define _SERCENTERCTRL_H_
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include "xml_base.h"

#include "zlog.h"
#include "reach_os.h"
//#include "command_resolve.h"
#include "params.h"
#include "controlwebtcpcom.h"

#define SERIAL_DATA_LEN							(4096)
#define MSGLEN                        			(sizeof(MsgHeader))
#define FAIL									(-1)
#define TRUE									(1)
#define FALSE									(0)
#define SERIAL_RUN								(1)
#define SERIAL_STOP     						(0)

#define SERIAL_SERVER_IPADDR					("127.0.0.1")
#define SERIAL_SERVER_IPADDR_PORT				(3000)

#define MAX_XMNL_LEN                            (4*1024)
#define SERIAL_CMD_LEN							(100)


#define	COMND_MAX    (sizeof(globalSerialCmd)/sizeof(SerialComnd))

enum Menu
{
IPMENU = 0,
MEMMENU,
DOWNMENU,
DOWNINGMENU,
REBOOTMENU,
REPLAYMENU,
REPLAYINGMENU,
RECORDING,
REBOOTING,
MENUINFO,
SELECTUSBMENU
};

enum Vod
{
	VOD_PLAY = 1,
	VOD_PAUSE,
	VOD_STOP,
	VOD_RESUME,
	VOD_FF,
	VOD_REW
};
typedef char  Int8;
typedef unsigned int  UInt32;
typedef int  Int32;
typedef unsigned char  UInt8;
typedef unsigned short UInt16;

typedef struct _list_head
{
   struct _list_head 	*next;
   struct _list_head	*prev;
   
}list_head;


typedef struct _UsbDev
{
	Int32   IsUsbDev;
	Int8   UsbDevName[50];
	UInt32  FreeSize;
	UInt32  TotalSize;
	
}UsbDev;
typedef struct _serial_handle_
{
	Int8	ipaddr[64];      //ip地址
	UInt32	port;			//端口
	Int32	socket;			//不解释
	Int32	serialFd;		
	Int32	serialFd1;
	Int32	ledfd;
	Int32	RunStatus;	    //运行状态
	Int32   LedState;	    //led状态
	Int32   LedFLUSH;		//led立即刷新，主要是多页面循环延迟
	Int32   PlayState;		//预览状态
	Int32   SeekLeve;	    //快进级别 
	Int8    *FileList;      //可见列表 注意可见长度
	Int32    ListPosition;  //可见列表位置
	Int8	CourseName[256];  //可见列表位置
	Int32	CourseNameChange;  //可见列表位置
	Int8    totalfilenum;   //总的课件数
	Int8	buf[SERIAL_DATA_LEN];
	UsbDev  UsdStat;	    // USB状态
	UInt32  IsUse;
	UInt32   usbpostin;
	zlog_category_t *c;		//zlog
	list_head *pheadnode;
	pthread_mutex_t usblock;
	pthread_mutex_t socklock;
	pthread_mutex_t seriallock;
	pthread_mutex_t seriallock1;
	pthread_mutex_t timeout;
} SerialHandle;

typedef enum _SERIAL_COMMND
{
    RECORD       = 30010,
	FECC         = 30013,
	DOWN         = 30014,
	PLAY         = 30048,
	SPIN		 = 30016,
    RECORDSTATE  = 30024,
    WARNING      = 10009,
    VOLUME       = 30019,
    REBOOT		 = 30012,
	PICSYN		 = 30047
} SERIAL_MSGCODE;

typedef struct _SerialParam
{
	Int8 videostate[3];
	Int8 warning;
	Int8 stop;
	Int8 start;
	Int8 close;
	Int8 volume;
	Int8 reboot;
	Int8 replay;
	Int8 replayfile[60];
} SerialParam;

typedef struct _serialcommnd
{
	SERIAL_MSGCODE commnd;
	UInt8 serialcmd;
	UInt8 serialcmdlen;
	Int8 *cmdname;
	Int32 (*PackageAnalysis)(Int8 *,void *);
	Int32 (*Operate)(UInt32,void *);
	Int32  direction;
} SerialComnd;

typedef struct _timeout
{
	SERIAL_MSGCODE msgcode;
	list_head stlist;
	struct  timeval starttime;
	uint8_t state;
}TimeOut;


typedef struct _R_GPIO_data_ {
	unsigned int gpio_num;
	unsigned int gpio_value;
} R_GPIO_data;


typedef struct _params_table_{
	int8_t version[64];				/*版本*/
	int8_t serial_num[64];			/*序列号*/

	uint32_t ip_addr;				/*IP地址*/
	uint32_t gateway;				/*网关*/

	uint32_t net_mask;				/*外网掩码*/
	int8_t mac_addr[24];			/*MAC地址*/

	uint32_t DiskMaxSpace;			/*总空间*/
	uint32_t DiskAvailableSpace; 	/*剩余空间*/
}params_table, *pParams_table;

typedef struct _MountUsbDeb
{
	char devname[20];
	unsigned  long   total_size;
	unsigned  long   free_size;
	int percent;
}MNUsbDeb;


Int32 RegisterrSerialControlTask();
Int32 ClearStateLed();
int32_t CopyDir(int8_t *SrcPath, int8_t *DstPath, int fd);


#endif

