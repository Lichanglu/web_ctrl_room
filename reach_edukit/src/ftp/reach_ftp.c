/*-------------------------------------------------------------------------
          Copyright (C), 2012-2013, Reach Tech. Co., Ltd.   
          File name:              
          Author: 徐崇      Version:        Date: 2012.10.12 10:31:01
	      Description:           
	      Function List:         
	      History:               
		    1. Date: 
  			   Author: 
  			   Modification:
-------------------------------------------------------------------------*/

#include "reach_ftp.h"
static zlog_category_t * g_zlog;

static const char *reach_ftp_strerror(int ret)
{
	switch(ret)
	{
		case REACH_FTP_PARAM_ERROR:
			return "[Ftp] config param error";
		case REACH_FTP_FOPEN_ERROR:
			return "[Ftp] open upfile failed";
		case REACH_FTP_GETFILESTAT_ERROR:
			return "[Ftp] Get File Stat failed";
		case REACH_UPLOAD_DIRECTORY_INVALID:
			return "[Upload] Invalid directory";
	    case REACH_UPLOAD_PARAM_NULL:
			return "[Upload] param is null";
		case REACH_UPLOAD_XMLFTP_NULL:
			return "[Upload] XmlFile or FtpConfig is error";
		case REACH_UPLOAD_SAVEXML_ERROR:
			return "[Upload] Save Xml File is error";
		default:
			return "UnKnow Error!!!\n";
	}
}


const char *ftp_strerror(int ret)
{
	if((ret > REACH_FTP_ERROR) && (ret < REACH_LAST))
	{
		return reach_ftp_strerror(ret);
	}
	else
	{
		return curl_easy_strerror(ret);
	}
}

static char *basename(char *path)
{
  /* Ignore all the details above for now and make a quick and simple
     implementaion here */
  char *s1;
  char *s2;

  s1=strrchr(path, '/');
  s2=strrchr(path, '\\');

  if(s1 && s2) {
    path = (s1 > s2? s1 : s2)+1;
  }
  else if(s1)
    path = s1 + 1;
  else if(s2)
    path = s2 + 1;

  return path;
}

/* parse headers for Content-Length */ 
/*==============================================================================
    函数: <getcontentlengthfunc>
    功能: <xh_Func:>
    参数: 
    Created By 徐崇 2012.10.16 16:46:05 For Ftp
==============================================================================*/
static size_t getcontentlengthfunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
  int r;
  long len = 0;
 
  /* _snscanf() is Win32 specific */ 
  r = sscanf(ptr,"Content-Length: %ld\n", &len);

  if (r) /* Microsoft: we don't read the specs */ 
    *((long *) stream) = len;
 
  return size * nmemb;
}



struct data 
{
  char trace_ascii; /* 1 or 0 */
};


int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{

  const char *text;
  (void)handle; /* prevent compiler warning */

  switch (type) {
  case CURLINFO_TEXT:
 	zlog_debug(g_zlog,data);
  default: /* in case a new one is introduced to shock us */
    return 0;

  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
	zlog_debug(g_zlog,data);
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
	zlog_debug(g_zlog,data);
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

  return 0;
}




/* discard downloaded data */ 
/*==============================================================================
    函数: <discardfunc>
    功能: <xh_Func:>
    参数: 
    Created By 徐崇 2012.10.16 16:46:21 For Ftp
==============================================================================*/
size_t discardfunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
  return size * nmemb;
}


/*==============================================================================
    函数: <Read_Callback>
    功能: <xh_Func:>
    参数: 
    Created By 徐崇 2012.10.16 16:46:25 For Ftp
==============================================================================*/
static size_t Read_Callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	curl_off_t nread;	
	size_t retcode = fread(ptr, size, nmemb, stream);
	
	nread = (curl_off_t)retcode;
	
	return retcode;
}


/*==============================================================================
    函数: <Ftp_Init>
    功能: <xh_Func:>
    参数: 
    Created By 徐崇 2012.10.12 10:29:31 For Ftp
==============================================================================*/
int Ftp_Init(void  *_phand)
{
	Ftp_Hand *phand = (Ftp_Hand*)_phand;
	if(phand == NULL)
	{
		return -1;
	}
	printf("ftp init .....\n");
	curl_global_init(CURL_GLOBAL_ALL);
	phand->pcurl =  curl_easy_init();
	return  1;
}


/*==============================================================================
    函数: <Ftp_Config>
    功能: <xh_Func:>
    参数: 
    Created By 徐崇 2012.10.12 10:50:18 For Ftp
==============================================================================*/
int	 Ftp_Config(void *_phand,Ftp_ParamConfig *config)
{
	Ftp_Hand *phand = (Ftp_Hand*)_phand;
	CURL *curl = (CURL *)phand->pcurl;
	if((NULL == curl)||(NULL == config))
	{
		return REACH_FTP_PARAM_ERROR;
	}
		
	if(NULL != config->UsrName)
	{
		
		curl_easy_setopt(curl, CURLOPT_USERNAME, config->UsrName);
	}

	if(NULL != config->PassWord)
	{	
		
		curl_easy_setopt(curl, CURLOPT_PASSWORD, config->PassWord);
	}
	
	if(TRUE == config->IsCreateDirs)
	{
		curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS,1);
	}

	if(0 < config->ConnecTimeout)
	{
		zlog_debug(g_zlog,"config ConnecTimeout = %d\n", config->ConnecTimeout);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT , config->ConnecTimeout);
	}

	if(0 < config->Timeout)
	{
		zlog_debug(g_zlog,"config Timeout = %d\n", config->Timeout);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->Timeout);
	}

	if((TRUE == config->IsProgress) && (NULL != config->Progress))
	{
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, config->Progress);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, NULL);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS,0L);
	}

	if(0 < config->MaxSendSeeed)
	{
		zlog_debug(g_zlog,"config MaxSendSeeed = %d\n",(curl_off_t)config->MaxSendSeeed);
		curl_easy_setopt(curl, CURLOPT_MAX_SEND_SPEED_LARGE, (curl_off_t)config->MaxSendSeeed);
	}
	

	if(0 < config->FtpPort)
	{
		zlog_debug(g_zlog,"config FtpPort = %d\n",config->FtpPort);
		curl_easy_setopt(curl,CURLOPT_PORT,config->FtpPort);
	}
	
	return 1;
}

int	 Ftp_Base_Config(void *_phand)
{
	Ftp_Hand *phand = (Ftp_Hand*)_phand;
	CURL *curl = (CURL *)phand->pcurl;
	if(NULL == curl)
	{
		return REACH_FTP_PARAM_ERROR;
	}
		
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, Read_Callback);

	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT , 20);
	
	//curl_easy_setopt(curl, CURLOPT_TIMEOUT, 7200);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT , 1*1024);
	curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME ,  20);

	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, getcontentlengthfunc);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, 20);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discardfunc);
	curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS,1);	
		
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 10);
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 10);
	curl_easy_setopt(curl, CURLOPT_FTPPORT, "-"); /* disable passive mode */
	g_zlog = NULL;
	if(TRUE)
	{
		g_zlog = zlog_get_category("FtpMoudle");
		if (!g_zlog)
		{
			curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, NULL);
		}
		else
		{
			curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);	
		}
		
	}
	
	curl_easy_setopt(curl, CURLOPT_STDERR, NULL);
	return 1;
}


/*==============================================================================
    函数: <Ftp_UpLoadFile>
    功能: <xh_Func:>
    参数: 
    Created By 徐崇 2012.10.12 11:38:54 For Ftp
==============================================================================*/
int  Ftp_UpLoadFile(void *_phand, Ftp_UpLoadParam *upconfig)
{
	FILE *UpLoadFile = NULL;
	struct stat sbuf;  
	struct curl_slist *headerlist = NULL;
	curl_off_t fsize = 0;
	curl_off_t  uploaded_len = 0;
	CURLcode res = CURLE_OK;
	char *UrlPath;
	char OldFileName [MAX_FILE_PATH_LEN]; 
	char NewFileNmae [MAX_FILE_PATH_LEN]; 

	Ftp_Hand *phand = (Ftp_Hand*)_phand;
	CURL *curl = (CURL *)phand->pcurl;
	if((curl == NULL ) || (upconfig == NULL) \
	|| (NULL == upconfig->UpFilePath) || (NULL == upconfig->LocalFilePath))
	{
		return REACH_FTP_PARAM_ERROR;
	}

	UpLoadFile = fopen(upconfig->LocalFilePath, "rb");
	if(NULL == UpLoadFile)
	{
		printf("open file %s faile !!!\n",upconfig->LocalFilePath);
		return REACH_FTP_FOPEN_ERROR;
	}
	
	if(stat(upconfig->LocalFilePath, &sbuf) < 0)
	{  
		fclose(UpLoadFile);
        return REACH_FTP_GETFILESTAT_ERROR;  
    }

    fsize = sbuf.st_size;  
    

	//printf("filepath %s %llu\n",upconfig->LocalFilePath,fsize);

	UrlPath = (char *)malloc(strlen(upconfig->UpFilePath) + 5);
	if(NULL == UrlPath)
	{
		fclose(UpLoadFile);
		return REACH_FTP_PARAM_ERROR;
	}
	
	strcpy(UrlPath, upconfig->UpFilePath);


	strncat(UrlPath, ".tmp", 4);

	sprintf(OldFileName, "RNFR %s", basename(UrlPath));
	sprintf(NewFileNmae, "RNTO %s", basename(upconfig->UpFilePath));

	//printf("%s %s \n",OldFileName,  NewFileNmae);
	headerlist = curl_slist_append(headerlist, OldFileName);
	headerlist = curl_slist_append(headerlist, NewFileNmae);


	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

	if(0 < upconfig->MaxSendSeeed)
	{
		//printf("upconfig->MaxSendSeeed = %ld\n",upconfig->MaxSendSeeed);
		curl_easy_setopt(curl, CURLOPT_MAX_SEND_SPEED_LARGE, (curl_off_t)upconfig->MaxSendSeeed);
	}

	if(NULL != upconfig->FtpPort)
	{
		curl_easy_setopt(curl,CURLOPT_PORT, atoi(upconfig->FtpPort));
	}
	
	curl_easy_setopt(curl,CURLOPT_URL, UrlPath);

	curl_easy_setopt(curl, CURLOPT_READDATA, UpLoadFile);
		
	
	if(upconfig->BreakpointUpLoad == TRUE)
	{
		 
         curl_easy_setopt(curl, CURLOPT_HEADERDATA, &uploaded_len);
		 curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
      	 curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
		 curl_easy_setopt(curl, CURLOPT_POSTQUOTE, NULL);
		 res = curl_easy_perform(curl);
  		 /* Check for errors */
  		 if((0  != uploaded_len) && (uploaded_len < fsize))
  		 {
			curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
			curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
			//curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)uploaded_len);
			//printf("filepath %llu %llu\n",uploaded_len,fsize);
			if(0 != fseek(UpLoadFile, uploaded_len, SEEK_SET))
			{
				fprintf(stderr, "fseek is failed!!!\n");
			}
			curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);
			curl_easy_setopt(curl, CURLOPT_APPEND, 1L);
			res = curl_easy_perform(curl);
			/* Check for errors */
			if(res != CURLE_OK)
			{
				fprintf(stderr, "curl_easy_perform() failed: %s\n",
			        ftp_strerror(res));
			}
  		 }
		 else
		 {
		 	curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
			curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
			//curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
			curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);
			res = curl_easy_perform(curl);
			/* Check for errors */
			if(res != CURLE_OK)
			{
				fprintf(stderr, "curl_easy_perform() failed: %s\n",
			        ftp_strerror(res));
			}
		 }
	}
	else
	{
		curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
		curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist);
		//curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
		res = curl_easy_perform(curl);
		/* Check for errors */
		if(res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
		        curl_easy_strerror(res));
		}
	}
	
	/* clean up the FTP commands list */
	curl_slist_free_all (headerlist);
	fclose(UpLoadFile); /* close the local file */

	if(UrlPath != NULL)
	{
		free(UrlPath);
	}
	return res;
}


/*==============================================================================
    函数: <Ftp_DeInit>
    功能: <xh_Func:>
    参数: 
    Created By 徐崇 2012.10.16 16:24:17 For Ftp
==============================================================================*/
int Ftp_DeInit(void *_phand)
{
	Ftp_Hand *phand = (Ftp_Hand*)_phand;
	CURL *curl = (CURL*)phand->pcurl;
	if(NULL == curl)
	{
		return REACH_FTP_PARAM_ERROR;
	}
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 1;
}


#if 0
/*==============================================================================
    函数: <Progress>
    功能: <xh_Func:>
    参数: 
    Created By 徐崇 2012.10.16 16:46:40 For Ftp
==============================================================================*/
static int Progress(void *p,
                    double dltotal, double dlnow,
                    double ultotal, double ulnow)
{


	fprintf(stderr, "UP: %g of %g %0.1f%%\r\n",ultotal, ulnow ,ulnow*100/ultotal);
	        

	return 0;
}
#endif

int Ftp_Register(Ftp_Hand *pFtpHand)
{
	pFtpHand->Ftp_Config = Ftp_Config;
	pFtpHand->Ftp_DeInit = Ftp_DeInit;
	pFtpHand->Ftp_UpLoadFile = Ftp_UpLoadFile;
	pFtpHand->Ftp_Init   = Ftp_Init;
	pFtpHand->Ftp_BaseConfig = Ftp_Base_Config;
	
	return 1;
}


