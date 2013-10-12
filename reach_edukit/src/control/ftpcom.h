/*
 * =====================================================================================
 *
 *       Filename:  ftpcom.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012��11��1�� 09ʱ12��18��
 *       Revision:  
 *       Compiler:  gcc
 *
 *         Author:  �ƺ��� 
 *        Company:  ������ȡ��Ϣ�����ɷ����޹�˾
 *
 * =====================================================================================
 */


#ifndef _FTPCOM_H_
#define _FTPCOM_H_


enum FTP_COMND
{
	FTP_COM_MSGTYPE_SERVER_CONFIG  = 0x4000,
	FTP_COM_MSGTYPE_SERVER_UPLOAD_FILE,
	FTP_COM_MSGTYPE_UPLOAD_SUCCESS = 0x5000,
	FTP_COM_MSGTYPE_UPLOAD_ERROR,
};

#define FTP_COM_DATA_LEN							(4096)
#define FTP_COM_MAX_FILENAME_LENGTH					(1024)

typedef struct _ftp_ope_obj_{
	int32_t (*set_server_info)(void *, const int8_t *, const int8_t *);
	int32_t (*upload_file)(void *, int8_t *, int8_t *, int8_t *, int8_t *, int8_t *);
	int32_t (*save_filename)(void *, int8_t *);
}ftp_ope_obj;


typedef struct _ftpcom_env_{
	int8_t		ipaddr[64];
	uint32_t	login;
	uint32_t	port;
	int32_t		socket;
	int32_t		run_status;
	int8_t		buf[FTP_COM_DATA_LEN];
	/* FIXME: �ļ��������Ƿ����·�����������ط������ļ����ĳ����Ƿ�һ�� */
	int8_t		filename[FTP_COM_MAX_FILENAME_LENGTH];

	ftp_ope_obj	pcmd;
	struct _server_set_ *pserset;

	pthread_mutex_t ftp_m;
	pthread_mutex_t socket_m;
}ftpcom_env;


int32_t start_ftpupload_com_task(struct _server_set_ *pser);
int32_t ftpcom_set_running(ftpcom_env *pftp);


#endif
