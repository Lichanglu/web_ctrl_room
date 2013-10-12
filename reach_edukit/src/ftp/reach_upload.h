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
/* ָ���ϴ���·�� */

#define FTPVERSION       "ftp_v5.0"

#define	INVALID_MSGTYPE				(-1)	//ÿ����Ϣ���е���Ϣ�������ͣ���Ϊ��Ч

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
typedef struct _msginfo 	//������Ϣ����
{
	int32_t		msgid;		//��Ϣ���е�ID
	long		mstype;		//��Ϣ���е�����
	
} msginfo;
typedef struct _msgque	  	//������Ϣ���е���Ϣʱ�Ļ�����
{
	long msgtype;			//��Ϣ���е�����
	int8_t *msgbuf;			//ָ��֡���ݵ�ָ�룬������Ϣͷ������
} msgque;
#endif

typedef struct _MsgHeader //��Ϣͷ
{
    unsigned short sLen;		//����
    unsigned short sVer;		//�汾
    unsigned short sMsgType;	//��Ϣ����
    unsigned short sData;	//����
}MsgHeader;


//��Щ��Ϣ�ǿɱ��
typedef struct _Upload_Info
{
	char  UsrName[MAX_USR_PASSWORD];				/* �û��� */
	char  PassWord[MAX_USR_PASSWORD];				/* ����   */
	char  Port[MAX_USR_PASSWORD];			     	/* ����   */
	char  RomteIp[MAX_FILE_PATH_LEN];               /* Զ��IP */
	char  RomtePath[MAX_FILE_PATH_LEN];             /* Զ��·�� */
	unsigned int   MaxSpeed;
}Upload_Info;


typedef struct _FtpService
{
	int sendflg;			//��ʾ��Ϣ��ʱ����Ϣ 
	zlog_category_t *c;		//zlog 
	parse_xml_t *px;			//xml����
	pthread_mutex_t socklock;	//socket ��
	Ftp_Hand *FtpHand;			 //ftp���
	int	 ClientSocket;			//�����socket
	Upload_Info *config;		//ftp����
	int32_t msgid;				//��Ϣ����
	pthread_t			timetaskid;
	pthread_t			dealtaskid;
	pthread_t			servertaskid;
}FtpService;

typedef int (*SendMessage)( char *,  char *, char *, char *,int); 



/* ���Ŀ¼�ļ� */
extern int  DetectionFile(Ftp_Hand *pFtpHand, char *UpLoadFilePath ,char *RomteFilePath,char* pport);
extern int  UpLoadFile(parse_xml_t *px,Ftp_Hand *FtpHand,Upload_Info *UploadConfig,SendMessage pfuntion);
extern int  UploadInit(parse_xml_t *px);
extern int  UploadInit(parse_xml_t *px);
extern void DeUploadInit(parse_xml_t *px);
extern int AddXmlNode(parse_xml_t *px, Upload_Info*config,char *LoalLoadFile, char *UpLoadPath, char *UpLoadIp,char* pport, char *precid);
#endif
