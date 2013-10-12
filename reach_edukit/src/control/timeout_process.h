/*
 * =====================================================================================
 *
 *       Filename:  timeout_process.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年11月1日 09时12分18秒
 *       Revision:  
 *       Compiler:  gcc
 *
 *         Author:  黄海洪 
 *        Company:  深圳锐取信息技术股份有限公司
 *
 * =====================================================================================
 */

#ifndef _TIMEOUT_PROCESS_H_
#define _TIMEOUT_PROCESS_H_

#include <pthread.h>

#include "command_protocol.h"
#include "common.h"
#include "ftpcom.h"



#define TIMEOUT_QUE_LENGTH							(20)
#define TIMEOUT_WAIT_MAX_SECOND						(10)
#define TIMEOUT_WAIT_EVERY_TIME						(25000)

#define FILE_QUE_LENGTH								(64)



typedef struct _timeout_user_{
	uint32_t		enable;
	platform_em		platform;						/* 平台类型 */
	int32_t			msgcode;
	int32_t			roomid;							/* 仅对发送给会议室的信令有效 */
	int32_t			userid;
	pthread_mutex_t	user_m;							/* 修改本用户其它数据时的互斥量 */
	
	int8_t  ipaddr[16];
}timeout_user;

typedef struct _timeout_que_{
	uint32_t		usercnt;
	uint32_t		con_link[TIMEOUT_QUE_LENGTH];
	timeout_user	user[TIMEOUT_QUE_LENGTH];
	pthread_mutex_t	mutex_m;
}timeout_que;

typedef struct _file_user_{
	uint32_t		enable;
	int8_t			filename[FTP_COM_MAX_FILENAME_LENGTH];
	int8_t			recordid[RECORD_ID_MAX_LENGTH];
	int8_t			path[FTP_MAX_FTPPATH_LENGTH];
	int8_t			serverip[24];
	pthread_mutex_t	user_m;							/* 修改本用户其它数据时的互斥量 */
}file_user;

typedef struct _file_que_{
	uint32_t		usercnt;
	uint32_t		con_link[FILE_QUE_LENGTH];
	file_user		user[FILE_QUE_LENGTH];
	pthread_mutex_t	mutex_m;
}file_que;


int32_t timeout_que_init(timeout_que *ptimeq);
int32_t timeout_que_add_user(timeout_que *ptimeq, platform_em platform, int32_t msgcode, int32_t userid);
int32_t timeout_que_del_user(timeout_que *ptimeq, int32_t user_index);
int32_t timeout_que_find_user(timeout_que *ptimeq, platform_em platform, int32_t msgcode, int32_t userid);
int32_t timeout_que_wait_a_moment(timeout_que *ptimeq, platform_em platform, int32_t msgcode,
													int32_t userid, uint32_t second);
int32_t timeout_que_cleanup(timeout_que *ptimeq, platform_em platform, int32_t userid);

int32_t file_que_init(file_que *pfileq);
int32_t file_que_add_user(file_que *pfileq, int8_t *filename, int8_t *recordid, int8_t *path, int8_t *serverip);
int32_t file_que_del_user(file_que *pfileq, int32_t user_index);
int32_t file_que_find_user(file_que *pfileq, int8_t *recordid);
file_user *file_que_get_user(file_que *pfileq, int32_t user_index);

int32_t timeout_que_find_user2(timeout_que *ptimeq, platform_em platform, 
									int32_t msgcode, int32_t userid, int32_t roomid, int32_t *out_userid);
int32_t timeout_que_add_user2(timeout_que *ptimeq, platform_em platform,
									int32_t msgcode, int32_t userid, int32_t roomid);


int32_t timeout_add_con_user_addr(timeout_que *ptimeq, platform_em platform, int32_t userid, int8_t *ipaddr);
int8_t * timeout_get_con_user_addr(timeout_que *ptimeq, platform_em platform, int32_t userid, int8_t *ipaddr);





#endif

