/*
 * =====================================================================================
 *
 *       Filename:  timeout_process.c
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

#include <stdio.h>

#include "control.h"
#include "command_protocol.h"
#include "command_resolve.h"
#include "timeout_process.h"


int32_t timeout_que_init(timeout_que *ptimeq)
{
	int32_t index = 0;
	
	if(NULL == ptimeq){
		zlog_error(DBGLOG, "--[timeout_que_init] failed, params is NULL!");
		return -1;
	}

	pthread_mutex_init(&(ptimeq->mutex_m), NULL);
	ptimeq->usercnt = 0;
	for(index = 0; index < TIMEOUT_QUE_LENGTH; index++){
		ptimeq->con_link[index] = 0;
		ptimeq->user[index].enable = 0;
		ptimeq->user[index].msgcode = -1;
		ptimeq->user[index].userid = -1;
		ptimeq->user[index].roomid = -1;
		ptimeq->user[index].platform = InvalidPlatform;
	}

	return 0;
}

int32_t timeout_que_all_cleanup(timeout_que *ptimeq)
{
	int32_t index = 0;
	timeout_user *puser = NULL;
	
	if(NULL == ptimeq){
		zlog_error(DBGLOG, "--[timeout_que_cleanup] failed, params is NULL!");
		return -1;
	}

	pthread_mutex_lock(&ptimeq->mutex_m);
	for(index = 0; index < TIMEOUT_QUE_LENGTH; index++){
		puser = &ptimeq->user[index];
		ptimeq->user[index].platform = InvalidPlatform;
		ptimeq->user[index].msgcode = -1;
		ptimeq->user[index].enable = 0;
		ptimeq->user[index].userid = -1;
		ptimeq->con_link[index] = 0;
	}
	ptimeq->usercnt = 0;
	pthread_mutex_unlock(&ptimeq->mutex_m);

	return 0;
}


int32_t timeout_que_add_user(timeout_que *ptimeq, platform_em platform, int32_t msgcode, int32_t userid)
{
	int32_t index = 0;

	if(NULL == ptimeq){
		zlog_error(DBGLOG, "--[timeout_que_add_user] failed, params is NULL!");
		return -1;
	}

	zlog_debug(OPELOG, "prepare add to timeout queue!, platform = %d, msgcode = %d, userid = %d\n",
										platform, msgcode, userid);

	pthread_mutex_lock(&ptimeq->mutex_m);
	for(index = 0; index < TIMEOUT_QUE_LENGTH; index++){
		if(0 == ptimeq->con_link[index]){
			ptimeq->con_link[index] = 1;
			ptimeq->usercnt++;
			ptimeq->user[index].enable = 1;
			ptimeq->user[index].platform = platform;
			ptimeq->user[index].msgcode = msgcode;
			ptimeq->user[index].userid = userid;
			pthread_mutex_unlock(&ptimeq->mutex_m);
			zlog_debug(OPELOG, "do add to timeout queue!, platform = %d, msgcode = %d, userid = %d\n",
										platform, msgcode, userid);
			return index;
		}
	}
	pthread_mutex_unlock(&ptimeq->mutex_m);

	/* 无法插入队列，需要清理队列 (前提是超时队列的总个数要大于control进程同时处理信令的总个数) */
	timeout_que_all_cleanup(ptimeq);

	return -1;	
}

int32_t timeout_que_add_user2(timeout_que *ptimeq, platform_em platform,
									int32_t msgcode, int32_t userid, int32_t roomid)
{
	int32_t index = 0;

	if(NULL == ptimeq || MSGCODE_RECORD_CONTROL != msgcode){
		zlog_error(DBGLOG, "--[timeout_que_add_user2] failed, params is NULL!");
		return -1;
	}

	zlog_debug(OPELOG, "prepare add to timeout queue2!, platform = %d, msgcode = %d, userid = %d, roomid = %d\n",
										platform, msgcode, userid, roomid);

	pthread_mutex_lock(&ptimeq->mutex_m);
	for(index = 0; index < TIMEOUT_QUE_LENGTH; index++){
		if(0 == ptimeq->con_link[index]){
			ptimeq->con_link[index] = 1;
			ptimeq->usercnt++;
			ptimeq->user[index].enable = 1;
			ptimeq->user[index].platform = platform;
			ptimeq->user[index].msgcode = msgcode;
			ptimeq->user[index].userid = userid;
			ptimeq->user[index].roomid = roomid;
			pthread_mutex_unlock(&ptimeq->mutex_m);
			zlog_debug(OPELOG, "do add to timeout_que_add_user2 queue!, platform = %d, msgcode = %d, userid = %d\n",
										platform, msgcode, userid);
			return index;
		}
	}
	pthread_mutex_unlock(&ptimeq->mutex_m);

	/* 无法插入队列，需要清理队列 (前提是超时队列的总个数要大于control进程同时处理信令的总个数) */
	timeout_que_all_cleanup(ptimeq);

	return -1;	
}


int32_t timeout_que_del_user(timeout_que *ptimeq, int32_t user_index)
{
	if(NULL == ptimeq){
		zlog_error(DBGLOG, "--[timeout_que_del_user] failed, params is NULL!");
		return -1;
	}

	if(user_index < 0 || user_index >= TIMEOUT_QUE_LENGTH){
		zlog_error(DBGLOG, "--[timeout_que_del_user] failed, user_index error!");
		return -1;
	}

	pthread_mutex_lock(&ptimeq->mutex_m);
	zlog_debug(OPELOG, "timeout queue node deleting, platform = %d, msgcode = %d, userid = %d, user_index = %d\n",
												ptimeq->user[user_index].platform,
												ptimeq->user[user_index].msgcode,
												ptimeq->user[user_index].userid,
												user_index);
	ptimeq->user[user_index].platform = InvalidPlatform;
	ptimeq->user[user_index].msgcode = -1;
	ptimeq->user[user_index].enable = 0;
	ptimeq->user[user_index].userid = -1;
	ptimeq->con_link[user_index] = 0;
	ptimeq->usercnt--;
	pthread_mutex_unlock(&ptimeq->mutex_m);
	
	return 0;	
}

int32_t timeout_que_find_user(timeout_que *ptimeq, platform_em platform, int32_t msgcode, int32_t userid)
{
	int32_t index = 0;
	int32_t user_index = -1;
	timeout_user *puser = NULL;
	
	if(NULL == ptimeq){
		zlog_error(DBGLOG, "--[timeout_que_find_user] failed, params is NULL!");
		return -1;
	}

	pthread_mutex_lock(&ptimeq->mutex_m);
	for(index = 0; index < TIMEOUT_QUE_LENGTH; index++){
		puser = &ptimeq->user[index];
#if 0		
		zlog_debug(OPELOG, "enable = %d, 1:platform = %d, msgcode = %d, index = %d, userid = %d\n",
							puser->enable, puser->platform, puser->msgcode, index, puser->userid);
		zlog_debug(OPELOG, "enable = %d, 2:platform = %d, msgcode = %d, index = %d, userid = %d\n\n",
							puser->enable, platform, msgcode, index, userid);
#endif
		/* FIXME:此处暂时作特殊处理 */
		if(LiveNode != platform){
			if(puser->enable == 1 && puser->platform == platform && puser->msgcode == msgcode){
				user_index = index;
				break;
			}
		}
		else{
			if(puser->enable == 1 && puser->platform == platform && puser->msgcode == msgcode && puser->userid == userid){
				user_index = index;
				zlog_debug(OPELOG, "find livenode timeout node, user_index = %d!\n", user_index);
				break;
			}
		}
//		zlog_debug(OPELOG, "--[timeout_que_find_user] failed, platform = %d, msgcode = %d, index = %d, userid = %d\n",
//							platform, msgcode, index, userid);
	}
	pthread_mutex_unlock(&ptimeq->mutex_m);

	return user_index;
}

/* 此接口仅供临时使用 */
int32_t timeout_que_find_user2(timeout_que *ptimeq, platform_em platform, 
									int32_t msgcode, int32_t userid, int32_t roomid, int32_t *out_userid)
{
	int32_t index = 0;
	int32_t user_index = -1;
	timeout_user *puser = NULL;
	
	if(NULL == ptimeq || MSGCODE_RECORD_CONTROL != msgcode || NULL == out_userid){
		zlog_error(DBGLOG, "--[timeout_que_find_user2] failed, params is NULL!");
		return -1;
	}

	pthread_mutex_lock(&ptimeq->mutex_m);
	for(index = 0; index < TIMEOUT_QUE_LENGTH; index++){
		puser = &ptimeq->user[index];
		/* FIXME:此处暂时作特殊处理 */
		if(LiveNode != platform){
			if(puser->enable == 1 && puser->platform == platform && puser->msgcode == msgcode && puser->roomid == roomid){
				user_index = index;
				*out_userid = puser->userid;
				break;
			}
		}
		else{
			if(puser->enable == 1 && puser->platform == platform && puser->msgcode == msgcode && puser->userid == userid){
				user_index = index;
				zlog_debug(OPELOG, "find livenode timeout node, user_index = %d!\n", user_index);
				break;
			}
		}
//		zlog_debug(OPELOG, "--[timeout_que_find_user2] failed, platform = %d, msgcode = %d, index = %d, roomid = %d, puser->roomid = %d\n",
//							platform, msgcode, index, roomid, puser->roomid);
	}
	pthread_mutex_unlock(&ptimeq->mutex_m);

	return user_index;
}


int32_t timeout_que_wait_a_moment(timeout_que *ptimeq, platform_em platform,
												int32_t msgcode, int32_t userid, uint32_t second)
{
	int32_t ret = -1;
	int32_t count = 0;
	
	if(NULL == ptimeq){
		zlog_error(DBGLOG, "--[timeout_que_wait_a_moment] failed, params is NULL!");
		return -1;
	}

	if(second > TIMEOUT_WAIT_MAX_SECOND)
		second = TIMEOUT_WAIT_MAX_SECOND;

	count = second*40;
	while(count--){
//		zlog_debug(OPELOG, "timeout_que_wait_a_moment %d\n", count);
		ret = timeout_que_find_user(ptimeq, platform, msgcode, userid);
		if(ret < 0){
			/* 找不到结点了 */
			ret = 0;
			break;
		}
		ret = -1;
		usleep(TIMEOUT_WAIT_EVERY_TIME);
		continue;
	}
	if(ret == -1)
		zlog_debug(OPELOG, "timeout_que_wait timeup!!\n");
	else
		zlog_debug(OPELOG, "timeout_que has been process!!\n");

	return ret;
}

int32_t timeout_que_cleanup(timeout_que *ptimeq, platform_em platform, int32_t userid)
{
	int32_t index = 0;
	timeout_user *puser = NULL;
	
	if(NULL == ptimeq){
		zlog_error(DBGLOG, "--[timeout_que_cleanup] failed, params is NULL!");
		return -1;
	}

	pthread_mutex_lock(&ptimeq->mutex_m);
	for(index = 0; index < TIMEOUT_QUE_LENGTH; index++){
		puser = &ptimeq->user[index];
		/* FIXME:此处暂时作特殊处理 */
//		if(LiveNode != platform){
//			puser->userid = userid;
//		}
		if(puser->platform == platform && puser->userid == userid){
			ptimeq->user[index].platform = InvalidPlatform;
			ptimeq->user[index].msgcode = -1;
			ptimeq->user[index].enable = 0;
			ptimeq->con_link[index] = 0;
			ptimeq->usercnt--;
			break;
		}
	}
	pthread_mutex_unlock(&ptimeq->mutex_m);

	return 0;
}

int32_t timeout_add_con_user_addr(timeout_que *ptimeq, platform_em platform, int32_t userid, int8_t *ipaddr)
{
	int32_t index = 0;
	timeout_user *puser = NULL;
	
	if(NULL == ptimeq || NULL == ipaddr){
		zlog_error(DBGLOG, "--[timeout_add_con_user_addr] failed, params is NULL!");
		return -1;
	}

	pthread_mutex_lock(&ptimeq->mutex_m);
	for(index = 0; index < TIMEOUT_QUE_LENGTH; index++){
		puser = &ptimeq->user[index];

		if(puser->platform == platform && puser->userid == userid){
			zlog_debug(OPELOG, "--[timeout_add_con_user_addr] [%d][%d][%s]!\n", platform, userid, ipaddr);
			r_bzero(ptimeq->user[index].ipaddr,sizeof(ptimeq->user[index].ipaddr));
			r_strcpy(ptimeq->user[index].ipaddr, ipaddr);
			break;
		}
	}
	pthread_mutex_unlock(&ptimeq->mutex_m);

	return 0;
}

int8_t * timeout_get_con_user_addr(timeout_que *ptimeq, platform_em platform, int32_t userid, int8_t *ipaddr)
{
	int32_t index = 0;
	timeout_user *puser = NULL;
	int8_t *tmp = NULL;
	
	if(NULL == ptimeq || NULL == ipaddr){
		zlog_error(DBGLOG, "--[timeout_get_con_user_addr] failed, params is NULL!");
		return NULL;
	}

	pthread_mutex_lock(&ptimeq->mutex_m);
	for(index = 0; index < TIMEOUT_QUE_LENGTH; index++){
		puser = &ptimeq->user[index];

		if(puser->platform == platform && puser->userid == userid){
			zlog_debug(OPELOG, "--[timeout_get_con_user_addr] [%d][%d][%s]!\n", platform, userid, ptimeq->user[index].ipaddr);
			r_strcpy(ipaddr, ptimeq->user[index].ipaddr);
			tmp = ipaddr;
			break;
		}
	}
	pthread_mutex_unlock(&ptimeq->mutex_m);

	return tmp;
}


int32_t file_que_init(file_que *pfileq)
{
	int32_t index = 0;
	
	if(NULL == pfileq){
		zlog_error(OPELOG, "--[file_que_init] failed, params is NULL!");
		return -1;
	}

	pthread_mutex_init(&(pfileq->mutex_m), NULL);
	pfileq->usercnt = 0;
	for(index = 0; index < FILE_QUE_LENGTH; index++){
		pfileq->con_link[index] = 0;
		pfileq->user[index].enable = 0;
		r_memset(pfileq->user[index].filename, 0, FTP_COM_MAX_FILENAME_LENGTH);
		r_memset(pfileq->user[index].recordid, 0, RECORD_ID_MAX_LENGTH);
		r_memset(pfileq->user[index].path, 0, FTP_MAX_FTPPATH_LENGTH);
		r_memset(pfileq->user[index].serverip, 0, 24);
	}

	return 0;
}

int32_t file_que_add_user(file_que *pfileq, int8_t *filename, int8_t *recordid, int8_t *path, int8_t *serverip)
{
	int32_t index = 0;

	if(NULL == pfileq || NULL == filename || NULL == recordid || NULL == path || NULL == serverip){
		zlog_error(OPELOG, "--[file_que_add_user] failed, params is NULL!");
		return -1;
	}

	pthread_mutex_lock(&pfileq->mutex_m);
	for(index = 0; index < FILE_QUE_LENGTH; index++){
		zlog_error(OPELOG, "--[file_que_add_user], index = %d\n", index);
		if(0 == pfileq->con_link[index]){
			pfileq->con_link[index] = 1;
			pfileq->usercnt++;
			pfileq->user[index].enable = 1;
			r_strcpy(pfileq->user[index].filename, filename);
			r_strcpy(pfileq->user[index].recordid, recordid);
			r_strcpy(pfileq->user[index].path, path);
			r_strcpy(pfileq->user[index].serverip, serverip);
			pthread_mutex_unlock(&pfileq->mutex_m);

			zlog_error(OPELOG, "--[file_que_add_user], recordid = %s, index = %d\n", recordid, index);
			
			return index;
		}
	}
	pthread_mutex_unlock(&pfileq->mutex_m);

	return 0;	
}

int32_t file_que_del_user(file_que *pfileq, int32_t user_index)
{
	zlog_error(OPELOG, "--[file_que_del_user], pfileq = %p, index = %d usercnt = %d!\n",
									pfileq, user_index, pfileq->usercnt);
	
	if(NULL == pfileq){
		zlog_error(OPELOG, "--[file_que_del_user] failed, params is NULL!");
		return -1;
	}

	if(user_index < 0 || user_index >= FILE_QUE_LENGTH){
		zlog_error(OPELOG, "--[file_que_del_user] failed, user_index error!");
		return -1;
	}

	pthread_mutex_lock(&pfileq->mutex_m);
	r_memset(pfileq->user[user_index].filename, 0, FTP_COM_MAX_FILENAME_LENGTH);
	r_memset(pfileq->user[user_index].recordid, 0, RECORD_ID_MAX_LENGTH);
	r_memset(pfileq->user[user_index].path, 0, FTP_MAX_FTPPATH_LENGTH);
	r_memset(pfileq->user[user_index].serverip, 0, 24);
	pfileq->user[user_index].enable = 0;
	pfileq->con_link[user_index] = 0;
	pfileq->usercnt--;
	pthread_mutex_unlock(&pfileq->mutex_m);
	
	return 0;	
}

int32_t file_que_find_user(file_que *pfileq, int8_t *recordid)
{
	int32_t index = 0;
	int32_t user_index = -1;
	file_user *puser = NULL;
	
	if(NULL == pfileq || NULL == recordid){
		zlog_error(DBGLOG, "--[file_que_find_user] failed, params is NULL!");
		return -1;
	}

	pthread_mutex_lock(&pfileq->mutex_m);
	for(index = 0; index < FILE_QUE_LENGTH; index++){
		puser = &pfileq->user[index];
		zlog_error(OPELOG, "--[file_que_find_user], enable = %d, recordid = %s, puser->recordid = %s\n",
										puser->enable, recordid, puser->recordid);
		if(puser->enable == 1 && (0==r_strcmp(puser->recordid, recordid))){
			user_index = index;
			break;
		}
	}
	pthread_mutex_unlock(&pfileq->mutex_m);

	if(-1 == user_index){
		zlog_debug(OPELOG, "--[file_que_find_user] failed, recordid: %s\n", recordid);
	}
	else{
		zlog_debug(OPELOG, "--[file_que_find_user] success, recordid: %s\n", recordid);
	}

	return user_index;
}

file_user *file_que_get_user(file_que *pfileq, int32_t user_index)
{
	file_user *puser = NULL;
	
	if(NULL == pfileq){
		zlog_error(OPELOG, "--[file_que_get_user] failed, params is NULL!");
		return NULL;
	}

	if(user_index < 0 || user_index >= FILE_QUE_LENGTH){
		zlog_error(OPELOG, "--[file_que_get_user] failed, user_index error!");
		return NULL;
	}

	pthread_mutex_lock(&pfileq->mutex_m);
	puser = &pfileq->user[user_index];
	pthread_mutex_unlock(&pfileq->mutex_m);
	
	return puser;	
}


