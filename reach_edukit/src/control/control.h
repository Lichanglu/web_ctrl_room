/*
 * =====================================================================================
 *
 *       Filename:  control.h
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


#ifndef _CONTROL_H_
#define _CONTROL_H_

#include "stdint.h"
#include "ftpcom.h"
#include "command_protocol.h"
#include "control_log.h"
#include "timeout_process.h"


#define RECORD_SERVER_VERSION							("0.0.0.0")


#define RECORD_SERVRE_TYPE								("EDU-SD")
#define RECORD_SERVER_SERIES							("_EDU-SD_0000")

#define ROOM_DEFAULT_NAME								("EDU-SD_Name")


#define CONTROL_DEFAULT_USERNAME						("admin")
#define CONTROL_DEFAULT_PASSWORD						("admin")

#define CONTROL_DEFAULT_GUESTNAME						("guest")
#define CONTROL_DEFAULT_GUESTPASSWD						("guest")


#define MP4_REPAIR_PATH									("recovery")
#define MP4_RECORD_PATH									("/opt")
#define MP4_RECORD_PATH2								("/opt/Rec")

#define DM6467_UPDATE_PACKET							("/var/log/recserver/reach/dm6467.bin")


#define	STATUS_SUCCESS					    			(0)
#define	STATUS_FAILED									(-1)

#define CONTROL_TRUE									(1)
#define CONTROL_FALSE									(0)
#define CONTROL_TIMEOUT									(2)

#define CONTROL_XML_DATA_LEN							(25600)
#define CONTROL_DATA_LEN								(8192)
#define CONTROL_MSGHEAD_LEN								(sizeof(MsgHeader))
#define CONTROL_LISTEN_MAX_NUM							(10)
#define CONTROL_MAX_COMMAND								(100)

#define CONTROL_STOPED									(0)
#define CONTROL_RUNNING									(1)
#define CONTROL_PREPARING								(2)

#define CONTROL_USER_STATUS_OFFLINE						(0)
#define CONTROL_USER_STATUS_ONLINE						(1)

#define CONTROL_COMMAND_XML_VERSION						(0)
#define CONTROL_COMMAND_BIN_VERSION						(1)

#define CONTROL_SERVER_TCP_SOCKET_SEND_TIMEOUT			(10)
#define CONTROL_SERVER_TCP_SOCKET_RECV_TIMEOUT			(10)
#define CONTROL_USER_TCP_SOCKET_SEND_TIMEOUT			(5)
#define CONTROL_USER_TCP_SOCKET_RECV_TIMEOUT			(5)

#define LOCAL_LOOP_INTERFACE							("127.0.0.1")

#define LIVENODE_INDEX_HEAD								(0x100)


/***********************************************************************************/
/* 关于HTTP上报 */
#define HTTP_SERVER_URL_MAX_LEN				(100)
#define HTTP_SERVER_URL_PORT				(8080)
#define HTTP_SERVER_POST_DEFAULT_INTERTIME	(3);

/***********************************************************************************/

#define	VIDEO_SEND_IP_LEN					(24)
#define	VIDEO_USER_MAX_NUM					(10)
#define	VIDEO_ENCODE_MAX_NUM				(6)
#define	VIDEO_ENCODE_INDEX_LEN				(VIDEO_ENCODE_MAX_NUM + 1)


#define CONTROL_UPDATE_FILE					("/var/log/recserver/update_Mn.tar.gz")


typedef struct _con_ack_{
	int32_t has_return;
	int32_t msgcode;
	int32_t msgtype;
}con_ack;

typedef struct _http_env_{
	int32_t			run_status;

	struct _server_set_ *pserset;

	pthread_t		http_thid;
	pthread_mutex_t	http_m;
}http_env;

typedef struct _user_ope_obj_{
	int32_t (*forward_process)(void *user_src, void *user_dst, int8_t *send_buf, int32_t msgcode,
									int8_t *recv_buf, int32_t *recv_len);
}user_ope_obj;

typedef struct _lives_addr_{
	int8_t 	user_ip[VIDEO_SEND_IP_LEN];
	int32_t user_port;

}lives_addr;

typedef struct _lives_info_{
	uint32_t	enable;
	uint32_t	userid;
	int32_t		quality_type;
	lives_addr	addr[VIDEO_ENCODE_MAX_NUM];
	int8_t		encode_index[VIDEO_ENCODE_INDEX_LEN];
}lives_info;


typedef struct _con_user_{
	int32_t			tcp_sock;						/* 用户通讯socket */
	int32_t 		login_ok;						/* 本用户的登录状态 */
	int32_t			index;							/* 保存的本用户的索引值 */
	int32_t			heart_beat;						/* 心跳在线标志*/
	int32_t			run_status;
	uint32_t		datalen;

	uint32_t		art_volume[CONTROL_ROOM_SERVER_MAX_USER];	/* 实时的音量值 */

	platform_em		platform;						/* 平台类型 */
	con_ack			ack;
	int8_t			recv_buf[CONTROL_DATA_LEN];		/* 用于接收线程的接收缓冲区 */
	int8_t			send_buf[CONTROL_DATA_LEN];		/* 用于接收线程的发送缓冲区 */
	int8_t			log_head[ZLOG_LOG_HEAD_LEN];	/* 用于保存本用户的日志头信息 */
	user_ope_obj	pcmd;							/* 可上报到至本用户的客户端的方法集 */
//	lives_user_info_t		lives_user;				/* 用于保存直播节点请求码流的信息 */
	struct _control_env_	*pconenv;				/* 保存的此控制模块的总环境变量 */
	int8_t			ipaddr[64];						/* 本用户客户端的IP地址 */
	uint16_t		port;							/* 本用户所连服务端的端口 */
	uint16_t		peerport;
	pthread_mutex_t	sock_m;							/* 本用户socket收发时的互斥量 */
	pthread_mutex_t	user_m;							/* 修改本用户其它数据时的互斥量 */
}con_user;

typedef struct _control_env_{
	uint32_t		usercnt;
	uint32_t		index;
	uint32_t		max_user_num;
	uint32_t		con_link[CONTROL_MAX_USER];
	int32_t			run_status;
	int32_t			ser_socket;
	int32_t			manager_flag;		/* 管理平台刚登录时的标志，用于让30024心跳快速发给管理平台 */
	uint16_t		ser_port;

	struct _server_set_ *pserset;
	con_user		user[CONTROL_MAX_USER];
	lives_info		lives_user[VIDEO_USER_MAX_NUM];	/* 直播信息，仅会议室使用 */

	pthread_t		clthid[CONTROL_MAX_USER];
	pthread_t		serthid;
	pthread_mutex_t	control_m;
}control_env;

typedef struct _command_ope_obj_{
	int32_t	msgcode;
	int32_t (*deal_process)(control_env *penv, con_user *puser, int8_t *recv_buf,
								int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
								int32_t msgcode);
	int8_t ope_text[64];
}command_ope_obj;

typedef struct _server_set_{
	http_env	http;	/* HTTP上报的环境变量，仅供媒体中心和定时上报任务使用 */
	control_env forser;
	control_env roomser[CONTROL_ROOM_SERVER_MAX_USER];
	ftpcom_env	ftpser;

	timeout_que timeque;	//消息超时队列
	file_que	fileque;

	struct _all_server_info_ *pserinfo;
}server_set;


int32_t start_control_server_task(server_set **ppser);
int32_t control_set_running(control_env *pcon);
int32_t http_set_running(http_env *phttp);
int32_t send_user_xml_data(con_user *user_src, con_user *user_dst, int8_t *send_buf,
										int8_t *ret_buf, int32_t *ret_len);
int32_t send_http_user_xml_data(struct _all_server_info_ *pinfo, int8_t *send_buf,
										int8_t *ret_buf, int32_t *ret_len);

con_user *find_forward_obj(struct _server_set_ *penv, platform_em platform, uint32_t room_index, uint32_t livenode_index);
int8_t *find_treaty_text(int32_t treaty_msg_code);
int32_t wait_a_moment(con_user *puser, int32_t msgcode, uint32_t second);
int32_t get_passkey_from_platform(platform_em platform, int8_t *passkey);
platform_em get_platform_from_passkey(int8_t *passkey);

int32_t http_set_stop(http_env *phttp);
int32_t control_set_stop(control_env *pcon);

con_user *find_forward_obj2(server_set *pser, platform_em platform, uint32_t mc_index);




#endif
