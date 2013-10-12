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
	char  UsrName[MAX_USR_PASSWORD];				/* 用户名 */
	char  PassWord[MAX_USR_PASSWORD];				/* 密码   */
	int  IsCreateDirs;		    /* 是否远端创建目录 1:支持 */
	int	 ConnecTimeout;			/* 连接超时时间 */
	int	 Timeout;				/* 最长连接时间 */
	int  IsProgress;			/* 是否现象进度 */
	int (*Progress)(void *p,double dltotal, double dlnow,\
                    double ultotal, double ulnow);
	int64_t  MaxSendSeeed;
	int  FtpPort;               /* ftp连接端口 */
}Ftp_ParamConfig;

typedef struct _Ftp_UpLoad
{
	char     *UpFilePath;               /* 上传文件路径 */
	char     *LocalFilePath;			/* 本地文件路径 */
	int64_t  MaxSendSeeed;				/* 最大上传速度 */
	int      BreakpointUpLoad; 			/*  1:开启断点续传 0:关闭 */
	char  	 *FtpPort;               	/* ftp连接端口 */
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


