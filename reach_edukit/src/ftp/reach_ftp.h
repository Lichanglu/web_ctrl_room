#ifndef REACH_FTPUPLOAD_
#define REACH_FTPUPLOAD_


#undef  _FILE_OFFSET_BITS  
#define  _FILE_OFFSET_BITS  64  

#include <stdio.h>
#include <string.h>
#include "curl.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>  
#include <assert.h>
#include "zlog.h"
#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE  0
#endif

#ifndef NULL
#define NULL (void *)0
#endif


#define MAX_FILE_PATH_LEN  400
#define MAX_USR_PASSWORD   50

#define RED				"\033[0;32;31m"
#define NONE				"\033[m"

typedef enum {

  REACH_FTP_ERROR       = 0X1000,
  REACH_FTP_PARAM_ERROR,
  REACH_FTP_FOPEN_ERROR,
  REACH_FTP_GETFILESTAT_ERROR,
  REACH_UPLOAD_DIRECTORY_INVALID,
  REACH_UPLOAD_PARAM_NULL,
  REACH_UPLOAD_XMLFTP_NULL,
  REACH_UPLOAD_SAVEXML_ERROR,
  REACH_LAST /* never use! */

} FTPcode;


typedef struct _Ftp_Config
{
	char  UsrName[MAX_USR_PASSWORD];				/* �û��� */
	char  PassWord[MAX_USR_PASSWORD];				/* ����   */
	int  IsCreateDirs;		    /* �Ƿ�Զ�˴���Ŀ¼ 1:֧�� */
	int	 ConnecTimeout;			/* ���ӳ�ʱʱ�� */
	int	 Timeout;				/* �����ʱ�� */
	int  IsProgress;			/* �Ƿ�������� */
	int (*Progress)(void *p,double dltotal, double dlnow,\
                    double ultotal, double ulnow);
	int64_t  MaxSendSeeed;
	int  FtpPort;               /* ftp���Ӷ˿� */
}Ftp_ParamConfig;

typedef struct _Ftp_UpLoad
{
	char     *UpFilePath;               /* �ϴ��ļ�·�� */
	char     *LocalFilePath;			/* �����ļ�·�� */
	int64_t  MaxSendSeeed;				/* ����ϴ��ٶ� */
	int      BreakpointUpLoad; 			/*  1:�����ϵ����� 0:�ر� */
	char  	 *FtpPort;               	/* ftp���Ӷ˿� */
}Ftp_UpLoadParam;

typedef struct _Ftp_Hand
{
	int   (*Ftp_Init)(void *phand);
	int	  (*Ftp_Config)(void *phand, Ftp_ParamConfig *config);
	int   (*Ftp_UpLoadFile)(void *phand, Ftp_UpLoadParam *upconfig);
	int   (*Ftp_DeInit)(void *phand);
	int   (*Ftp_BaseConfig)(void *phand);
	void *pcurl;
}Ftp_Hand;


extern int Ftp_Register(Ftp_Hand *pFtpHand);
extern const char *ftp_strerror(int ret);
#endif


