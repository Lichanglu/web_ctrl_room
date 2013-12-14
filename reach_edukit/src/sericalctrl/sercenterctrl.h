
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
	Int8	ipaddr[64];      //ip��ַ
	UInt32	port;			//�˿�
	Int32	socket;			//������
	Int32	serialFd;		
	Int32	serialFd1;
	Int32	ledfd;
	Int32	RunStatus;	    //����״̬
	Int32   LedState;	    //led״̬
	Int32   LedFLUSH;		//led����ˢ�£���Ҫ�Ƕ�ҳ��ѭ���ӳ�
	Int32   PlayState;		//Ԥ��״̬
	Int32   SeekLeve;	    //������� 
	Int8    *FileList;      //�ɼ��б� ע��ɼ�����
	Int32    ListPosition;  //�ɼ��б�λ��
	Int8	CourseName[256];  //�ɼ��б�λ��
	Int32	CourseNameChange;  //�ɼ��б�λ��
	Int8    totalfilenum;   //�ܵĿμ���
	Int8	buf[SERIAL_DATA_LEN];
	UsbDev  UsdStat;	    // USB״̬
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
	int8_t version[64];				/*�汾*/
	int8_t serial_num[64];			/*���к�*/

	uint32_t ip_addr;				/*IP��ַ*/
	uint32_t gateway;				/*����*/

	uint32_t net_mask;				/*��������*/
	int8_t mac_addr[24];			/*MAC��ַ*/

	uint32_t DiskMaxSpace;			/*�ܿռ�*/
	uint32_t DiskAvailableSpace; 	/*ʣ��ռ�*/
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

