#ifndef REACH_UPLOAD_H_
#define REACH_UPLOAD_H_

#include "reach_ftp.h"
#include <xml/xml_base.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include "zlog.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "common.h"
#include <sys/resource.h>
#include "reach_os.h"
#include "xml_msg_management.h"
#include "dirmd5.h"
/* 指定上传的路径 */

#define FTPVERSION       "ftp_v5.0"

#define	INVALID_MSGTYPE				(-1)	//每个消息队列的消息包的类型，此为无效

#define SELECTTIMEOUT  600
#define TRYNUM       (3)
#define MSGLEN                        (sizeof(MsgHeader))
#define SERVERPORTNUM					 (16666)
#define MAXXMLSIZE                       (1024*2)
#define MAXRECVSIZE                      (65535)
#define FTP_COM_TCP_SOCKET_TIMEOUT					(3)


#define RESP_ROOT_KEY (BAD_CAST "ResponseMsg")
#define REQ_ROOT_KEY (BAD_CAST "RequestMsg")
enum FTP_COMND
{
	FTP_COM_MSGTYPE_SERVER_CONFIG  = 0x4000,
	FTP_COM_MSGTYPE_SERVER_UPLOAD_FILE,
	FTP_COM_MSGTYPE_UPLOAD_SUCCESS = 0x5000,
	FTP_COM_MSGTYPE_UPLOAD_ERROR,
};

#if 0
typedef struct _msginfo 	//描述消息队列
{
	int32_t		msgid;		//消息队列的ID
	long		mstype;		//消息队列的类型
	
} msginfo;
typedef struct _msgque	  	//发送消息队列的消息时的缓冲区
{
	long msgtype;			//消息队列的类型
	int8_t *msgbuf;			//指向帧数据的指针，包括消息头的内容
} msgque;
#endif

typedef struct _MsgHeader //消息头
{
    unsigned short sLen;		//长度
    unsigned short sVer;		//版本
    unsigned short sMsgType;	//消息类型
    unsigned short sData;	//保留
}MsgHeader;


//这些信息是可变的
typedef struct _Upload_Info
{
	char  UsrName[MAX_USR_PASSWORD];				/* 用户名 */
	char  PassWord[MAX_USR_PASSWORD];				/* 密码   */
	char  Port[MAX_USR_PASSWORD];			     	/* 密码   */
	char  RomteIp[MAX_FILE_PATH_LEN];               /* 远端IP */
	char  RomtePath[MAX_FILE_PATH_LEN];             /* 远端路径 */
	unsigned int   MaxSpeed;
}Upload_Info;


typedef struct _FtpService
{
	int sendflg;			//标示消息定时器消息 
	zlog_category_t *c;		//zlog 
	parse_xml_t *px;			//xml管理
	pthread_mutex_t socklock;	//socket 锁
	Ftp_Hand *FtpHand;			 //ftp句柄
	int	 ClientSocket;			//服务端socket
	Upload_Info *config;		//ftp配置
	int32_t msgid;				//消息队列
	pthread_t			timetaskid;
	pthread_t			dealtaskid;
	pthread_t			servertaskid;
}FtpService;

typedef int (*SendMessage)( char *,  char *, char *, char *,int); 



/* 检测目录文件 */
extern int  DetectionFile(Ftp_Hand *pFtpHand, char *UpLoadFilePath ,char *RomteFilePath,char* pport);
extern int  UpLoadFile(parse_xml_t *px,Ftp_Hand *FtpHand,Upload_Info *UploadConfig,SendMessage pfuntion);
extern int  UploadInit(parse_xml_t *px);
extern int  UploadInit(parse_xml_t *px);
extern void DeUploadInit(parse_xml_t *px);
extern int AddXmlNode(parse_xml_t *px, Upload_Info*config,char *LoalLoadFile, char *UpLoadPath, char *UpLoadIp,char* pport, char *precid);
#endif
