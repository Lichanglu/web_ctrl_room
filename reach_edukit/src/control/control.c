/*
 * =====================================================================================
 *
 *       Filename:  control.c
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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <linux/netlink.h>
#include <netinet/ether.h>
#include <linux/rtnetlink.h>
#include <netdb.h>

#include "control_log.h"
#include "common.h"
#include "control.h"
#include "reach_os.h"
#include "command_protocol.h"
#include "command_resolve.h"
#include "xml_msg_management.h"
#include "mid_http.h"
#include "params.h"





command_ope_obj g_treaty_info[CONTROL_MAX_COMMAND] =
{
	{
		CONTROL_OLD_MSGHEADER_MSGCODE,
		login_process,
		"login_command"
	},
	{
		MSGCODE_CHECK_USER,
		checkuser_process,
		"checkuser command"
	},
	{
		MSGCODE_GET_SERVER_INFO,
		get_server_info_process,
		"get_server_info command"
	},
	{
		MSGCODE_GET_ROOM_INFO,
		get_room_info_process,
		"get_room_info command"
	},
	{
		MSGCODE_SET_ROOM_INFO,
		set_room_info_process,
		"set_room_info command"
	},
	{
		MSGCODE_SET_ROOM_QUALITY,
		set_room_quality_process,
		"set_room_quality command"
	},
	{
		MSGCODE_SET_ROOM_AUDIO_INFO,
		set_room_audio_info_process,
		"set_room_audio_info command"
	},
	{
		MSGCODE_GET_FTPUPLOAD_INFO,
		get_ftpupload_info_process,
		"get_ftpupload_info command"
	},
	{
		MSGCODE_SET_SERVER_SYS_PARAMS,
		set_server_sys_params_process,
		"set_server_sys_params command"
	},
	{
		MSGCODE_GET_SERVER_SYS_PARAMS,
		get_server_sys_params_process,
		"get_server_sys_params command"
	},
	{
		MSGCODE_RECORD_CONTROL,
		rec_control_process,
		"rec_control command"
	},
	{
		MSGCODE_STREAM_REQUEST,
		stream_request_process,
		"stream_request command"
	},
	{
		MSGCODE_REBOOT_SERVER,
		reboot_server_process,
		"reboot_server command"
	},
	{
		MSGCODE_CAMERA_CTROL,
		camera_control_process,
		"camera_control command"
	},
	{
		MSGCODE_IFRAME_REQUEST,
		iframe_request_process,
		"iframe_request command"
	},
	{
		MSGCODE_CONNECT_ROOM_REQUEST,
		connect_room_request_process,
		"connect_room_request command"
	},
	{
		MSGCODE_UPLOAD_LOGO_PICTURE,
		upload_logo_picture_process,
		"upload_logo_picture command"
	},
	{
		MSGCODE_UPDATE_SERVER,
		update_server_process,
		"update_server command"
	},
	{
		MSGCODE_ADD_TEXT_TITLE,
		add_text_title_process,
		"add_text_title command"
	},
	{
		MSGCODE_GET_AUDIO_VOLUME,
		get_audio_volume_process,
		"get_audio_volume command"
	},
	{
		MSGCODE_SET_AUDIO_MUTE,
		set_audio_mute_process,
		"set_audio_mute command"
	},
	{
		MSGCODE_VIDEO_ADJUST,
		video_adjust_process,
		"video_adjust command"
	},
	{
		MSGCODE_FILE_DELETE_REPORT,
		file_delete_report_process,
		"file_delete_report command"
	},
	{
		MSGCODE_GET_FILELIST_INFO,
		get_filelist_info_process,
		"get_filelist_info command"
	},
	{
		MSGCODE_ROOM_REPORT_RECORD_STATUS,
		report_record_status_process,
		"report_record_status command"
	},
	{
		MSGCODE_ROOM_GET_PREREC_INFO,
		get_prerec_info_process,
		"get_prerec_info command"
	},
	{
		MSGCODE_LIVENODE_HEART_BEAT,
		livenode_heart_beat_process,
		"livenode_heart_beat command"
	},
	{
		MSGCODE_HEART_REPORT_REQ,
		room_heart_report_process,
		"room_heart_report command"
	},
	{
		MSGCODE_ENCINFO_REPORT_REQ,
		room_encinfo_report_process,
		"room_encinfo_report command"
	},
	{
		MSGCODE_GET_CAMCTL_PROLIST,
		get_camctl_pro_process,
		"get_camctl_pro command"
	},
	{
		MSGCODE_SET_CAMCTL_PRO,
		set_camctl_pro_process,
		"set_camctl_pro command"
	},
	{
		MSGCODE_UPLOAD_CAMCTL_PRO,
		upload_camctl_pro_process,
		"upload_camctl_pro command"
	},
	{
		MSGCODE_DEL_CAMCTL_PRO,
		delete_camctl_pro_process,
		"delete_camctl_pro command"
	},
	{
		MSGCODE_GET_ENC_VERINFO,
		get_encver_info_process,
		"get_encver_info command"
	},
	{
		MSGCODE_GET_AUDIO_INFO,
		get_audio_info_process,
		"get_audio_info command"
	},
	{
		MSGCODE_PICTURE_SYNTHESIS,
		picture_synthesis_process,
		"picture synthesis command"
	},
	{
		MSGCODE_REPLAY_REQ,
		picture_replay_process,
		"playing back command"
	},
	{
		MSGCODE_STUDIO_DIRECTOR_INFO,
		studio_director_info_process,
		"studio director info command"
	}
};

static int32_t get_server_log_head(int16_t port, int8_t *plog_head, int8_t *ipaddr)
{
	if(NULL == plog_head){
		zlog_error(DBGLOG, "---[get_server_log_head] failed!, params is NULL!\n");
		return -1;
	}

	if(CONTROL_FOR_SERVER_PORT == port){
		sprintf((char *)plog_head, "FORE_SERVER [version: %s][%s]-->:\t\t", RECORD_SERVER_VERSION, ipaddr);
	}
	else if(port >= CONTROL_ROOM_SERVER_PORT_START && port <= CONTROL_ROOM_SERVER_PORT_END){
		sprintf((char *)plog_head, "ROOM_SERVER_%d_[%s]-->:\t\t", port - CONTROL_ROOM_SERVER_PORT_START, ipaddr);
	}
	else{
		return -1;
	}

	return 0;
}

int32_t get_user_log_head(int16_t port, int32_t user_index, platform_em platform,
								int8_t *plog_head, int8_t *ipaddr)
{
	if(NULL == plog_head){
		zlog_error(DBGLOG, "---[get_user_log_head] failed!, params is NULL!\n");
		return -1;
	}

	if(CONTROL_FOR_SERVER_PORT == port){
		switch(platform)
		{
			case MediaCenter:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t", ipaddr, PLATFORM_MEDIACENTER, user_index);
				break;
			case ManagePlatform:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t", ipaddr, PLATFORM_MANAGEPLATFORM, user_index);
				break;
			case IpadUser:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t", ipaddr, PLATFORM_IPADUSER, user_index);
				break;
			case ComControl:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t", ipaddr, PLATFORM_COMCONTROL, user_index);
				break;
		    case NetControl:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t", ipaddr, PLATFORM_NETCONTROL, user_index);
				break;
			case Mp4Repair:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t", ipaddr, PLATFORM_MP4RREPAIR, user_index);
				break;
			case WebCtrl:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t", ipaddr, PLATFORM_WEBCTRL, user_index);
				break;
			case ThirdControl:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t", ipaddr, PLATFORM_THIRDCONTROL, user_index);
				break;
			case RecServer:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t\t", ipaddr, PLATFORM_RECSERVER, user_index);
				break;
			case LiveNode:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t\t", ipaddr, PLATFORM_LIVENODE, user_index);
				break;
			case AllPlatform:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t", ipaddr, PLATFORM_ALLPLATFORM, user_index);
				break;
			case Director:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s %d ]>:\t", ipaddr, PLATFORM_DIRECTOR, user_index);
				break;
			case InvalidPlatform:
			default:
				sprintf((char *)plog_head, "FORE_SERVER->[%s %s]>:\t", ipaddr, "unknow platform");
				break;
		}
	}
	else if(port >= CONTROL_ROOM_SERVER_PORT_START && port <= CONTROL_ROOM_SERVER_PORT_END){
		sprintf((char *)plog_head, "ROOM_SERVER->[%s %s %d ]>:\t\t", ipaddr, PLATFORM_RECSERVER, port - CONTROL_ROOM_SERVER_PORT_START);
	}
	else{
		return -1;
	}

	return 0;
}

int8_t *get_msgtype_value(int32_t msgtype, int8_t *value)
{
	if(NULL == value){
		zlog_error(DBGLOG, "---[get_msgtype_value] failed, ");
		return value;
	}

	r_memset(value, 24, 0);
	if(XML_MSG_TYPE_RESP == msgtype){
		r_strcpy(value, (const int8_t *)"response msg");
	}
	else{
		r_strcpy(value, (const int8_t *)"request msg");
	}

	return value;
}

static inline int32_t set_user_msgtype(con_user *puser, int32_t msgtype)
{
	if(NULL == puser){
		zlog_error(DBGLOG, "---[set_user_msgtype] failed, params is NULL\n");
		return -1;
	}

	puser->ack.msgtype = msgtype;

	return 0;
}


int32_t http_init_env(http_env *phttp)
{
	if(NULL == phttp){
		zlog_error(DBGLOG, "http_init_env faild, handle is NULL!\n");
		return -1;
	}

	phttp->pserset = NULL;
	phttp->run_status = CONTROL_PREPARING;
//	phttp->post_time = HTTP_SERVER_POST_DEFAULT_INTERTIME;
//	r_memset(phttp->post_url, 0, HTTP_SERVER_URL_MAX_LEN);
//	r_strcpy(phttp->post_url, (const int8_t *)"http://192.168.6.5/HttpService.action");
	pthread_mutex_init(&(phttp->http_m), NULL);

	return 0;
}

int32_t http_set_running(http_env *phttp)
{
	if(NULL == phttp){
	   zlog_error(DBGLOG, "http_set_running faild, handle is NULL!\n");
	   return -1;
	}

	pthread_mutex_lock(&phttp->http_m);
	phttp->run_status = CONTROL_RUNNING;
	pthread_mutex_unlock(&phttp->http_m);

	return 0;
}

int32_t http_set_stop(http_env *phttp)
{
	if(NULL == phttp){
	   zlog_error(DBGLOG, "http_set_stop faild, handle is NULL!\n");
	   return -1;
	}

	pthread_mutex_lock(&phttp->http_m);
	phttp->run_status = CONTROL_STOPED;
	pthread_mutex_unlock(&phttp->http_m);

	return 0;
}



int32_t http_set_server_set(http_env *phttp, server_set *pser)
{
	if(NULL == phttp){
       zlog_error(DBGLOG, "http_set_server_set faild, handle is NULL!\n");
       return -1;
	}

	pthread_mutex_lock(&phttp->http_m);
	phttp->pserset = pser;
	pthread_mutex_unlock(&phttp->http_m);

	return 0;
}

int32_t http_set_post_url(con_user *puser, int16_t http_port)
{
	http_env *phttp = NULL;
	all_server_info *pinfo = NULL;
	heart_beat_info hb_new;
	int8_t buf[HTTP_SERVER_URL_MAX_LEN] = {0};

	if(NULL == puser){
		zlog_error(DBGLOG, "http_set_post_url faild, handle is NULL!\n");
		return -1;
	}

	phttp = &puser->pconenv->pserset->http;
	pinfo = puser->pconenv->pserset->pserinfo;

	sprintf((char *)buf, "http://%s/HttpService.action", puser->ipaddr);

	pthread_mutex_lock(&(pinfo->info_m));
	r_memcpy(&hb_new, &pinfo->HBeatInfo, sizeof(heart_beat_info));
	r_memset(hb_new.post_url, 0, HTTP_SERVER_URL_MAX_LEN);
	r_strcpy(hb_new.post_url, buf);
	r_memset(hb_new.time_serip, 0, 64);
	r_strncpy(hb_new.time_serip, puser->ipaddr, 24);
	modify_heart_beat_info_only((const int8_t *)CONFIG_TABLE_FILE, &pinfo->HBeatInfo, &hb_new);
	r_memcpy(&pinfo->HBeatInfo, &hb_new, sizeof(heart_beat_info));
	pthread_mutex_unlock(&(pinfo->info_m));

	return 0;
}


int32_t control_init_env(control_env *pcon)
{
	uint32_t index = 0;
	uint32_t j = 0;
	con_user *puser = NULL;
	lives_info *plive = NULL;

	if(NULL == pcon){
		zlog_error(DBGLOG, "control_init_env faild, handle is NULL!\n");
		return -1;
	}

	pcon->usercnt = 0;
	pcon->ser_socket = -1;
	pcon->max_user_num = CONTROL_MAX_USER;
	pcon->run_status = CONTROL_PREPARING;
	pthread_mutex_init(&(pcon->control_m), NULL);
	pcon->ser_port = CONTROL_FOR_SERVER_PORT;
	pcon->pserset = NULL;

	for(index = 0; index < CONTROL_MAX_USER; index++){
		puser = &pcon->user[index];
		pthread_mutex_init(&(puser->user_m), NULL);
		pthread_mutex_init(&(puser->sock_m), NULL);
		pcon->con_link[index] = 0;
		puser->index = -1;
		puser->ack.has_return = CONTROL_FALSE;
		puser->ack.msgcode = -1;
		puser->platform = InvalidPlatform;
		puser->login_ok = CONTROL_USER_STATUS_OFFLINE;
		puser->tcp_sock = -1;
		puser->run_status = CONTROL_STOPED;

		for(j = 0; j < CONTROL_ROOM_SERVER_MAX_USER; j++){
			puser->art_volume[j] = 0;
		}

		puser->pcmd.forward_process = NULL;
		puser->pconenv = pcon;
		puser->port = 0;
		r_memset(puser->ipaddr, 0, 64);
		r_memset(puser->log_head, 0, ZLOG_LOG_HEAD_LEN);
	}

	for(index = 0; index < VIDEO_USER_MAX_NUM; index++){
		plive = &pcon->lives_user[index];
		plive->enable = 0;
		plive->userid = 0;
		plive->quality_type = -1;
		r_memset(plive->encode_index, 0, VIDEO_ENCODE_INDEX_LEN);
		for(j = 0; j < VIDEO_ENCODE_MAX_NUM; j++){
			r_memset(plive->addr[j].user_ip, 0, VIDEO_SEND_IP_LEN);
			plive->addr[j].user_port = 0;
		}
	}

	return 0;
}

int32_t control_set_running(control_env *pcon)
{
	if(NULL == pcon){
	   zlog_error(DBGLOG, "control_set_running faild, handle is NULL!\n");
	   return -1;
	}

	pthread_mutex_lock(&pcon->control_m);
	pcon->run_status = CONTROL_RUNNING;
	pthread_mutex_unlock(&pcon->control_m);

	return 0;
}

int32_t control_set_stop(control_env *pcon)
{
	if(NULL == pcon){
	   zlog_error(DBGLOG, "control_set_stop faild, handle is NULL!\n");
	   return -1;
	}

	pthread_mutex_lock(&pcon->control_m);
	pcon->run_status = CONTROL_STOPED;
	pthread_mutex_unlock(&pcon->control_m);

	return 0;
}



int32_t control_set_server_set(control_env *pcon, server_set *pser)
{
	if(NULL == pcon){
       zlog_error(DBGLOG, "control_set_server_set faild, handle is NULL!\n");
       return -1;
	}

	pthread_mutex_lock(&pcon->control_m);
	pcon->pserset = pser;
	pthread_mutex_unlock(&pcon->control_m);

	return 0;
}

int32_t control_set_server_port(control_env *pcon, uint16_t port)
{
	if(NULL == pcon){
       zlog_error(DBGLOG, "control_set_server_port faild, handle is NULL!\n");
       return -1;
	}

	pthread_mutex_lock(&pcon->control_m);
	pcon->ser_port = port;
	pthread_mutex_unlock(&pcon->control_m);

	return 0;
}


int32_t control_set_max_user_num(control_env *pcon,
															uint32_t max_user_num)
{
	if(NULL == pcon){
		zlog_error(DBGLOG, "control_set_max_user_num faild, handle is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&pcon->control_m);
	pcon->max_user_num = max_user_num;
	pthread_mutex_unlock(&pcon->control_m);

	return 0;
}

inline uint32_t control_get_max_user_num(control_env *pcon)
{
	uint32_t max_user_num = 0;

	if(NULL == pcon){
		zlog_error(DBGLOG, "control_get_max_user_num faild, handle is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&pcon->control_m);
	max_user_num = pcon->max_user_num;
	pthread_mutex_unlock(&pcon->control_m);

	return max_user_num;
}

static inline int32_t control_set_server_socket(control_env *pcon, int32_t socket)
{
	if(NULL == pcon){
		zlog_error(DBGLOG, "control_set_server_socket faild, handle is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&pcon->control_m);
	pcon->ser_socket = socket;
	pthread_mutex_unlock(&pcon->control_m);

	return 0;
}

static inline int32_t control_get_server_socket(control_env *pcon)
{
	int32_t socket = 0;

	if(NULL == pcon){
		zlog_error(DBGLOG, "control_get_server_socket faild, handle is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&pcon->control_m);
	socket = pcon->ser_socket;
	pthread_mutex_unlock(&pcon->control_m);

	return socket;
}

int32_t set_user_ipaddr(control_env *pcon, int8_t *ipaddr, platform_em platform)
{
	struct in_addr addr;

	if(NULL == pcon || NULL == ipaddr){
		zlog_error(DBGLOG, "set_user_ipaddr failed, params is NULL!\n");
		return -1;
	}

	if(pcon->pserset != NULL){
		if(pcon->pserset->pserinfo != NULL){
			pthread_mutex_lock(&pcon->pserset->pserinfo->info_m);
			inet_aton((const char *)ipaddr, &addr);
			if(ManagePlatform == platform) {
				r_memcpy(&pcon->pserset->pserinfo->ConfigInfo.manager_addr, &addr, 4);
			} else if(MediaCenter == platform) {
				r_memcpy(&pcon->pserset->pserinfo->ConfigInfo.media_addr, &addr, 4);
			}else if(Director == platform) {
				r_memcpy(&pcon->pserset->pserinfo->ConfigInfo.director_addr, &addr, 4);
			}
			pthread_mutex_unlock(&pcon->pserset->pserinfo->info_m);
		}
	}

	return 0;
}


static inline int32_t get_conuser_num(control_env *pcon)
{
	int32_t user_count = 0;

	if(NULL == pcon){
		zlog_error(DBGLOG, "get_conuser_num faild, handle is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&pcon->control_m);
	user_count = pcon->usercnt;
	pthread_mutex_unlock(&pcon->control_m);

	return user_count;
}

static inline void set_user_login(control_env *pcon, int32_t user_index)
{
	if(NULL == pcon){
		zlog_error(DBGLOG, "set_user_login faild, handle is NULL!\n");
		return ;
	}

	if(user_index < 0 || user_index >= CONTROL_MAX_USER){
		zlog_error(DBGLOG, "set_user_login faild, index error, index = %d\n", user_index);
		return ;
	}

	pthread_mutex_lock(&(pcon->user[user_index].user_m));
	pcon->user[user_index].login_ok = CONTROL_USER_STATUS_ONLINE;
	pcon->usercnt++;
	pthread_mutex_unlock(&(pcon->user[user_index].user_m));
}

static inline void set_user_logout(control_env *pcon, int32_t user_index)
{
	if(NULL == pcon){
		zlog_error(DBGLOG, "set_user_logout faild, handle is NULL!\n");
		return ;
	}

	if(user_index < 0 || user_index >= CONTROL_MAX_USER){
		zlog_error(DBGLOG, "set_user_login faild, index error, index = %d\n", user_index);
		return ;
	}

	pthread_mutex_lock(&(pcon->user[user_index].user_m));
	pcon->user[user_index].login_ok = CONTROL_USER_STATUS_OFFLINE;
	pcon->usercnt--;
	pthread_mutex_unlock(&(pcon->user[user_index].user_m));
}



static inline void set_conuser_tcpsock(con_user *puser, int socket)
{
	if(NULL == puser){
		zlog_error(DBGLOG, "set_con_user_tcpsock faild, handle is NULL!\n");
		return ;
	}

	pthread_mutex_lock(&(puser->user_m));
	puser->tcp_sock = socket;
	pthread_mutex_unlock(&(puser->user_m));
}

static inline int32_t get_conuser_tcpsock(con_user *puser)
{
	int32_t socket = 0;

	if(NULL == puser){
		zlog_error(DBGLOG, "get_conuser_tcpsock faild, handle is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(puser->user_m));
	socket = puser->tcp_sock;
	pthread_mutex_unlock(&(puser->user_m));

	return socket;
}

static inline void clear_conuser_tcpsock(con_user *puser)
{
	if(NULL == puser){
		zlog_error(DBGLOG, "clear_conuser_tcpsock faild, handle is NULL!\n");
		return ;
	}

	pthread_mutex_lock(&(puser->user_m));
	puser->tcp_sock = -1;
	pthread_mutex_unlock(&(puser->user_m));
}

int32_t check_platform_unique_and_report(control_env *pcon, con_user *puser, platform_em platform)
{
	int32_t index = 0;
	int32_t have_platform = 0;

	if(NULL == pcon || NULL == puser){
		zlog_error(DBGLOG, "check_platform_unique_and_report faild, handle is NULL!\n");
		return -1;
	}

	for(index = 0; index < CONTROL_FOR_SERVER_MAX_USER; index++){
		if(pcon->user[index].login_ok == CONTROL_USER_STATUS_ONLINE){
			if(puser->index == index)
				continue;
			if(pcon->user[index].platform == platform){
				if(ManagePlatform == platform ){
					report_same_platform_login(&pcon->user[index], puser);
					usleep(15000);
					close(pcon->user[index].tcp_sock);
				}

				if(Director == platform) {
					if(!r_strcmp(pcon->user[index].username, puser->username)) {
						report_same_platform_login(&pcon->user[index], puser);
						usleep(15000);
						close(pcon->user[index].tcp_sock);
					}
				}
				usleep(20000);
				pcon->user[index].run_status = CONTROL_STOPED;
				usleep(30000);
				have_platform = 1;
				break;
			}
		}
	}

	return have_platform;
}

int32_t wait_a_moment(con_user *puser, int32_t msgcode, uint32_t second)
{
	int32_t count = 0;

	if(NULL == puser){
		zlog_error(DBGLOG, "wait_a_moment faild, handle is NULL!\n");
		return -1;
	}

	if(second > 10)
		second = 10;

	count = second*2;

	while(count--){
		zlog_debug(OPELOG, "wait_a_moment is waiting!!!\n");
#if 0
		zlog_debug(DBGLOG, "---[wait_a_moment], puser = %p\n", puser);
		zlog_debug(DBGLOG, "---[wait_a_moment], has_return = %d\n", puser->ack.has_return);
		zlog_debug(DBGLOG, "---[wait_a_moment], msgcode = %d\n", puser->ack.msgcode);
		zlog_debug(DBGLOG, "---[wait_a_moment], msgtype = %d\n", puser->ack.msgtype);
#endif
		if(CONTROL_TRUE == puser->ack.has_return && puser->ack.msgcode == msgcode
								&& puser->ack.msgtype == XML_MSG_TYPE_RESP)
		{
			break;
		}
		usleep(500000);
	}

	zlog_debug(OPELOG, "wait_a_moment time up!!!\n");

	return 0;
}

platform_em get_platform_from_passkey(int8_t *passkey)
{
	platform_em platform = InvalidPlatform;

//	zlog_debug(OPELOG, "passkey = %s\n", passkey);

	if(NULL == passkey){
		zlog_error(OPELOG, "get_platform_from_passkey failed, handle is NULL!\n");
		return InvalidPlatform;
	}

	if(r_strcmp(passkey, (const int8_t *)PLATFORM_MEDIACENTER) == 0){
		platform = MediaCenter;
	}
	else if(r_strcmp(passkey, (const int8_t *)PLATFORM_MANAGEPLATFORM) == 0){
		platform = ManagePlatform;
	}
	else if(r_strcmp(passkey, (const int8_t *)PLATFORM_IPADUSER) == 0){
		platform = IpadUser;
	}
	else if(r_strcmp(passkey, (const int8_t *)PLATFORM_COMCONTROL) == 0){
		platform = ComControl;
	}
	else if(r_strcmp(passkey, (const int8_t *)PLATFORM_NETCONTROL) == 0){
		platform = NetControl;
	}
	else if(r_strcmp(passkey, (const int8_t *)PLATFORM_MP4RREPAIR) == 0){
		platform = Mp4Repair;
	}
	else if(r_strcmp(passkey, (const int8_t *)PLATFORM_WEBCTRL) == 0){
		platform = WebCtrl;
	}
	else if(r_strcmp(passkey, (const int8_t *)PLATFORM_THIRDCONTROL) == 0){
		platform = ThirdControl;
	}
	else if(r_strcmp(passkey, (const int8_t *)PLATFORM_RECSERVER) == 0){
		platform = RecServer;
	}
	else if(r_strcmp(passkey, (const int8_t *)PLATFORM_LIVENODE) == 0){
		platform = LiveNode;
	}
	else if(r_strcmp(passkey, (const int8_t *)PLATFORM_DIRECTOR) == 0){
		platform = Director;
	}
	else {
		platform = InvalidPlatform;
	}

	return platform;
}

int32_t get_passkey_from_platform(platform_em platform, int8_t *passkey)
{
	if(NULL == passkey || (platform < MediaCenter || platform >= InvalidPlatform)){
		zlog_error(DBGLOG, "get_passkey_from_platform failed, handle is NULL!\n");
		return -1;
	}

	switch(platform)
	{
		case MediaCenter:
			r_strcpy(passkey, (const int8_t *)PLATFORM_MEDIACENTER);
			break;
		case ManagePlatform:
			r_strcpy(passkey, (const int8_t *)PLATFORM_MANAGEPLATFORM);
			break;
		case IpadUser:
			r_strcpy(passkey, (const int8_t *)PLATFORM_IPADUSER);
			break;
		case ComControl:
			r_strcpy(passkey, (const int8_t *)PLATFORM_COMCONTROL);
			break;
		case NetControl:
			r_strcpy(passkey, (const int8_t *)PLATFORM_NETCONTROL);
			break;
		case Mp4Repair:
			r_strcpy(passkey, (const int8_t *)PLATFORM_MP4RREPAIR);
			break;
		case WebCtrl:
			r_strcpy(passkey, (const int8_t *)PLATFORM_WEBCTRL);
			break;
		case ThirdControl:
			r_strcpy(passkey, (const int8_t *)PLATFORM_THIRDCONTROL);
			break;
		case RecServer:
			r_strcpy(passkey, (const int8_t *)PLATFORM_RECSERVER);
			break;
		case LiveNode:
			r_strcpy(passkey, (const int8_t *)PLATFORM_LIVENODE);
			break;
		case AllPlatform:
			r_strcpy(passkey, (const int8_t *)PLATFORM_ALLPLATFORM);
			break;
		case Director:
			r_strcpy(passkey, (const int8_t *)PLATFORM_DIRECTOR);
			break;
		case InvalidPlatform:
		default:
			break;
	}

	return 0;
}


static inline int32_t set_conuser_platform(con_user *puser, platform_em platform)
{
	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};

	if(NULL == puser){
		zlog_error(DBGLOG, "set_conuser_platform faild, handle is NULL!\n");
		return -1;
	}

	r_strcpy(plog_head, puser->log_head);

	if(platform < MediaCenter || platform >= InvalidPlatform){
		zlog_error2(DBGLOG, "set_conuser_platform faild, platform error!\n");
		return -1;
	}

	pthread_mutex_lock(&(puser->user_m));
	puser->platform = platform;
	pthread_mutex_unlock(&(puser->user_m));

	return 0;
}

static inline platform_em get_conuser_platform(con_user *puser)
{
	platform_em platform = InvalidPlatform;

	if(NULL == puser){
		zlog_error(DBGLOG, "get_conuser_platform faild, handle is NULL!\n");
		return InvalidPlatform;
	}

	pthread_mutex_lock(&(puser->user_m));
	platform = puser->platform;
	pthread_mutex_unlock(&(puser->user_m));

	return platform;
}




/* 获取用户索引 */
int32_t get_conuser_index(control_env *pcon)
{
	int32_t index = 0;

	if(NULL == pcon){
		zlog_error(DBGLOG, "get_conuser_index faild, handle is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&pcon->control_m);
	for(index = 0; index < pcon->max_user_num; index++) {
		if(!pcon->con_link[index]) {
			pcon->con_link[index] = 1;
			pthread_mutex_unlock(&pcon->control_m);
			return index;
		}
	}
	pthread_mutex_unlock(&pcon->control_m);

	zlog_debug(DBGLOG, "control: max recuser!\n");

	return -1;
}

static void remove_conuser_index(control_env *pcon, int32_t user_index)
{
	if(NULL == pcon){
		zlog_error(DBGLOG, "remove_conuser_index faild, handle is NULL!\n");
		return ;
	}

	pthread_mutex_lock(&pcon->control_m);
	pcon->con_link[user_index] = 0;
	pcon->user[user_index].tcp_sock = -1;
	pthread_mutex_unlock(&pcon->control_m);

	return ;
}

void *get_treaty_info(void)
{
	return g_treaty_info;
}

int32_t find_treaty_index(int32_t treaty_msg_code)
{
	int32_t return_code = STATUS_FAILED;
	int32_t index = 0;
	command_ope_obj *ptreaty_info = NULL;

	ptreaty_info = (command_ope_obj *)get_treaty_info();
	for(index = 0; index < CONTROL_MAX_COMMAND ; index ++){
//		zlog_debug(DBGLOG, "---[find_treaty_index], ptreaty_info[%d].msgcode = %d, treaty_msg_code = %d\n",
//								index, ptreaty_info[index].msgcode, treaty_msg_code);
		if(ptreaty_info[index].msgcode == treaty_msg_code) {
			return_code = index ;
			break;
		}
	}

	return return_code ;
}

int8_t *find_treaty_text(int32_t treaty_msg_code)
{
	int32_t index = 0;
	int8_t *ptext = NULL;
	command_ope_obj *ptreaty_info = NULL;

	ptreaty_info = (command_ope_obj *)get_treaty_info();
	for(index = 0; index < CONTROL_MAX_COMMAND ; index ++){
//		zlog_debug(DBGLOG, "---[find_treaty_index], ptreaty_info[%d].msgcode = %d, treaty_msg_code = %d\n",
//								index, ptreaty_info[index].msgcode, treaty_msg_code);
		if(ptreaty_info[index].msgcode == treaty_msg_code) {
			ptext = ptreaty_info[index].ope_text;
			break;
		}
	}

	return ptext;
}


static int32_t recv_user_xml_data(int32_t socket, int8_t *xml_buf, int16_t *force_msgcode,
												int8_t *plog_head, uint32_t *datalen)
{
	int32_t recvlen = 0;;
	int32_t headlen = 0;
	int32_t return_code = STATUS_FAILED;

	MsgHeader msg;

	headlen = CONTROL_MSGHEAD_LEN;

	if(NULL == xml_buf || NULL == force_msgcode || datalen == NULL){
		zlog_error2(DBGLOG, "---[recv_user_xml_data] failed, xml_buf is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	recvlen = tcp_recv_longdata(socket, (int8_t *)&msg, headlen);
	if(recvlen < headlen || recvlen == -1){
		zlog_error2(DBGLOG, "nLen < HEAD_LEN  errno = %d  nLen = %d\n", errno, recvlen);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

//	msg.sMsgType = ntohs(msg.sMsgType);



//	zlog_debug(OPELOG, "00- sLen = %d, sVer = %d, sMsgType = %d\n", msg.sLen, msg.sVer, msg.sMsgType);

	msg.sMsgType = ntohs(msg.sMsgType);
	msg.sLen = ntohs(msg.sLen);
	msg.sVer = ntohs(msg.sVer);

	//zlog_debug(OPELOG, "xx11- sLen = %d, sVer = %d, sMsgType = %d\n", msg.sLen, msg.sVer, msg.sMsgType);

	if(((msg.sVer == CONTROL_OLD_MSGHEADER_VERSION) && (msg.sMsgType == CONTROL_OLD_MSGHEADER_MSGCODE))
		|| ((msg.sVer == ntohs(CONTROL_OLD_MSGHEADER_VERSION)) && (msg.sMsgType == ntohs(CONTROL_OLD_MSGHEADER_MSGCODE))))
	{
		*force_msgcode = CONTROL_OLD_MSGHEADER_MSGCODE;
		return_code = STATUS_SUCCESS;
		goto EXIT;
	}

#if 1
	/* FIXME: 此处需作判断 */
	if(!(msg.sVer == CONTROL_NEW_MSGHEADER_VERSION || msg.sVer == CONTROL_NEW_MSGHEADER_MSGCODE)){
		return_code = STATUS_FAILED;
		goto EXIT;
	}
#endif

#if 0
	if(!(msg.sMsgType != CONTROL_NEW_MSGHEADER_MSGCODE || msg.sMsgType != CONTROL_NEW_MSGHEADER_VERSION)){
		return_code = STATUS_FAILED;
		goto EXIT;
	}
#endif

	if(266 != msg.sLen && 213 != msg.sLen){
		zlog_debug2(DBGLOG, "msg.sLen = %d\n", msg.sLen);
	}
	if((msg.sLen - headlen) < 0){
		zlog_error2(DBGLOG, "msg len error, len = %d\n", msg.sLen);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(msg.sLen > CONTROL_DATA_LEN)
		msg.sLen = CONTROL_DATA_LEN;

	recvlen = tcp_recv_longdata(socket, xml_buf, msg.sLen - headlen);
	if(recvlen < msg.sLen-headlen){
		zlog_error2(DBGLOG, "nLen < nMsgLen -HEAD_LEN\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	*datalen = recvlen;

	return_code = STATUS_SUCCESS;

EXIT:

	return return_code;
}

int32_t send_user_xml_data(con_user *user_src, con_user *user_dst, int8_t *send_buf,
										int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t datelen = 0;
	int32_t headlen = 0;
	int32_t ret = 0;

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	MsgHeader msg;

	if(NULL == user_dst || NULL == send_buf){
		zlog_error(DBGLOG, "send_user_xml_data failed , params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(user_src)
		r_strcpy(plog_head, user_src->log_head);

	headlen = CONTROL_MSGHEAD_LEN;
	datelen = r_strlen((const int8_t *)&send_buf[headlen]);

	msg.sLen = htons(datelen + headlen);
	msg.sMsgType = htons(CONTROL_NEW_MSGHEADER_MSGCODE);
	msg.sVer = htons(CONTROL_NEW_MSGHEADER_VERSION);
	msg.sData = 0;

	r_memcpy(send_buf, &msg, headlen);

	if(user_src){
		zlog_debug2(OPELOG, "+++++++++++++++ send_user_xml_data send buf: %d , fd = %d ++++++++++++++\n", datelen + headlen, user_dst->tcp_sock);
		zlog_debug2(OPELOG, "\n%s\n", send_buf+headlen);
		zlog_debug2(OPELOG, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
	}

	ret = tcp_send_longdata(user_dst->tcp_sock, send_buf, datelen + headlen);
	//zlog_debug2(OPELOG, "+++++++++++++++ send length %d ++++++++++++++\n", ret);
	if(ret > 0)
		return_code = STATUS_SUCCESS;

EXIT:

	return return_code;
}

int32_t send_user_xml_data2(con_user *user_src, con_user *user_dst, int8_t *send_buf,
										uint32_t xml_len, uint32_t data_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t datelen = 0;
	int32_t headlen = 0;
	int32_t ret = 0;

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	MsgHeader msg;

	if(NULL == user_dst || NULL == send_buf){
		zlog_error(DBGLOG, "send_user_xml_data2 failed , params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(user_src)
		r_strcpy(plog_head, user_src->log_head);

	headlen = CONTROL_MSGHEAD_LEN;
	datelen = xml_len;

	msg.sLen = htons(datelen + headlen);
	msg.sMsgType = htons(CONTROL_NEW_MSGHEADER_MSGCODE);
	msg.sVer = htons(CONTROL_NEW_MSGHEADER_VERSION);
	msg.sData = 0;

	r_memcpy(send_buf, &msg, headlen);

	if(user_src){
		zlog_debug2(OPELOG, "+++++++++++++++ send_user_xml_data send buf: %d ++++++++++++++\n", datelen + headlen);
		zlog_debug2(OPELOG, "\n%s\n", send_buf+headlen);
		zlog_debug2(OPELOG, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");

		printf("1  %s\n",send_buf + headlen);
		printf("2  %s\n",send_buf + headlen + datelen);;
	}

	ret = tcp_send_longdata(user_dst->tcp_sock, send_buf, datelen + headlen + data_len);
	if(ret > 0)
		return_code = STATUS_SUCCESS;

EXIT:

	return return_code;
}


int32_t send_http_user_xml_data(all_server_info *pinfo, int8_t *send_buf,
										int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t datelen = 0;
	int32_t headlen = 0;
	heart_beat_info *phbeat = NULL;
	int8_t url[HTTP_SERVER_URL_MAX_LEN] = {0};

	if(NULL == pinfo || NULL == send_buf){
		zlog_error(DBGLOG, "send_http_user_xml_data failed , params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	headlen = CONTROL_MSGHEAD_LEN;
	datelen = r_strlen((const int8_t *)&send_buf[headlen]);

	zlog_debug(OPELOG, "------------------- send_http_user_xml_data send buf -------------------\n");
	zlog_debug(OPELOG, "\n\n%s\n", send_buf+headlen);
	zlog_debug(OPELOG, "------------------------------------------------------------------------\n\n");

	phbeat = &pinfo->HBeatInfo;

	pthread_mutex_lock(&pinfo->info_m);
	r_strcpy(url, phbeat->post_url);
	pthread_mutex_unlock(&pinfo->info_m);

	zlog_debug(OPELOG, "url: %s\n", url);
	zlog_debug(OPELOG, "datelen: %d\n", datelen);
	zlog_debug(OPELOG, "ret_buf: %p\n", ret_buf);

	return_code = mid_http_post((char *)url, (char *)(send_buf+headlen), datelen,
									(char *)ret_buf, (int *)ret_len);

	zlog_debug(OPELOG, "ret_len: %p\n", ret_len);

	return_code = STATUS_SUCCESS;

EXIT:

	return return_code;
}



con_user *find_forward_obj(server_set *pser, platform_em platform, uint32_t room_index, uint32_t userid)
{
	int32_t i = 0;
	con_user *puser = NULL;
	control_env *proomser = NULL;
	control_env *forser = NULL;

//	zlog_error(OPELOG, "find_forward_obj object: %d\n", platform);

	if(NULL == pser || platform == InvalidPlatform){
		zlog_error(DBGLOG, "find_forward_obj failed, params error!\n");
		return NULL;
	}

	if(platform == RecServer){		/* 会议室 */
		if(room_index > pser->pserinfo->ServerInfo.MaxRoom){
			zlog_error(DBGLOG, "find_forward_obj failed, room_index error!\n");
			return NULL;
		}

		proomser = &pser->roomser[room_index];
		pthread_mutex_lock(&(proomser->user[0].user_m));
//		zlog_debug(DBGLOG, "---[find_forward_obj], index = %d, login_ok = %d\n",
//										index, proomser->user[0].login_ok);
//		zlog_debug(DBGLOG, "---[find_forward_obj], room platform = %d, platform = %d\n",
//										proomser->user[0].platform, platform);
		if(proomser->user[0].login_ok == CONTROL_USER_STATUS_ONLINE
			&& proomser->user[0].platform == platform)
		{
			puser = &proomser->user[0];
		}
		pthread_mutex_unlock(&(proomser->user[0].user_m));
	}
	else if(platform == LiveNode || platform == IpadUser){		/* 直播节点 */
		if(userid > CONTROL_FOR_SERVER_MAX_USER){
			zlog_error(DBGLOG, "find_forward_obj failed, livenode_index error!\n");
			return NULL;
		}

		forser = &pser->forser;
		for(i = 0; i < CONTROL_FOR_SERVER_MAX_USER; i++){
//			zlog_debug(DBGLOG, "---[find_forward_obj], index = %d, login_ok = %d, platform = %d, in platform = %d\n",
//										i, forser->user[i].login_ok, forser->user[i].platform, platform);
			pthread_mutex_lock(&(forser->user[i].user_m));
			if(forser->user[i].login_ok == CONTROL_USER_STATUS_ONLINE
					&& forser->user[i].platform == platform && forser->user[i].index == userid)
			{
				puser = &forser->user[i];
				pthread_mutex_unlock(&(forser->user[i].user_m));
				break;
			}
			pthread_mutex_unlock(&(forser->user[i].user_m));
		}
	}
	else {		/* 其它平台 */
		forser = &pser->forser;
		for(i = 0; i < CONTROL_FOR_SERVER_MAX_USER; i++){
//			zlog_debug(DBGLOG, "---[find_forward_obj], index = %d, login_ok = %d, platform = %d, in platform = %d\n",
//										i, forser->user[i].login_ok, forser->user[i].platform, platform);
			pthread_mutex_lock(&(forser->user[i].user_m));
			if(forser->user[i].login_ok == CONTROL_USER_STATUS_ONLINE
					&& forser->user[i].run_status == CONTROL_RUNNING
					&& forser->user[i].platform == platform)
			{
				puser = &forser->user[i];
				pthread_mutex_unlock(&(forser->user[i].user_m));
				break;
			}
			pthread_mutex_unlock(&(forser->user[i].user_m));
		}
	}

	return puser;
}

con_user *find_forward_obj2(server_set *pser, platform_em platform, uint32_t mc_index)
{
	int32_t i = 0;
	con_user *puser = NULL;
	control_env *forser = NULL;

	/* 仅平台为媒体中心，msgcode为30010使用 */

	if(NULL == pser || platform == InvalidPlatform){
		zlog_error(DBGLOG, "find_forward_obj failed, params error!\n");
		return NULL;
	}

	if(MediaCenter == platform){
		forser = &pser->forser;
		for(i = 0; i < CONTROL_FOR_SERVER_MAX_USER; i++){
			pthread_mutex_lock(&(forser->user[i].user_m));
			if(forser->user[i].login_ok == CONTROL_USER_STATUS_ONLINE
					&& forser->user[i].platform == platform && forser->user[i].index == mc_index)
			{
				puser = &forser->user[i];
				pthread_mutex_unlock(&(forser->user[i].user_m));
				break;
			}
			pthread_mutex_unlock(&(forser->user[i].user_m));
		}
	}

	return puser;
}


static int32_t set_user_function_set(con_user *puser, platform_em platform)
{
	int32_t record_code = STATUS_FAILED;

	if(NULL == puser){
		zlog_error(DBGLOG, "set_user_function_set failed, params is NULL!\n");
		record_code = STATUS_FAILED;
		goto EXIT;
	}

	pthread_mutex_lock(&puser->user_m);

	/* 目前不管客户端是什么平台，都赋予所有的转发方法 */
	switch(platform)
	{
		case MediaCenter:
		case ManagePlatform:
		case IpadUser:
		case RecServer:
		case LiveNode:
		case ComControl:
		case NetControl:
		case Mp4Repair:
		case WebCtrl:
		case ThirdControl:
		case InvalidPlatform:
		case Director:
		default:
			{
				puser->pcmd.forward_process = forward_process;
			}
			break;
	}

	pthread_mutex_unlock(&puser->user_m);

	record_code = STATUS_SUCCESS;

EXIT:

	return record_code;
}


static void *control_user_process(con_user *puser)
{
	int32_t headlen = 0;
	int32_t user_index = 0;
	int32_t ret_len = 0;
	int32_t socket = 0;
	int32_t msgcode = 0;

	int16_t force_msgcode = 0;
	int8_t passkey[PASS_KEY_LEN];
	int8_t *xml_buf = NULL;
	int8_t *snd_buf = NULL;
	int8_t *ret_buf = NULL;
	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	int8_t value[24] = {0};

	int32_t return_status = STATUS_FAILED;
	int32_t treaty_index = 0;
	int32_t ret_code = -1;
	int32_t seret = 0;
	int32_t msgtype = -1;
	int32_t j = 0;

	uint32_t datalen = 0;

	fd_set readfd;
	struct timeval timeout;

	command_ope_obj *ptreaty = (command_ope_obj *)get_treaty_info();

	MsgHeader msg;
	platform_em platform = InvalidPlatform;
	platform_em lastplatform = InvalidPlatform;

	zlog_debug(DBGLOG, "~~~~~~~~~~~~~~~~~~~~~~ start control_user_process ~~~~~~~~~~~~~~~~~~~~~\n");

	if(NULL == puser){
		zlog_error(DBGLOG, "control_user_process failed, puser is NULL!\n");
		pthread_detach(pthread_self());
		return NULL;
	}

	control_env *pconenv = (control_env	*)puser->pconenv;
	user_index = puser->index;
	xml_buf = puser->recv_buf;
	snd_buf = puser->send_buf;

	r_memset(plog_head, 0, 32);
	get_user_log_head(puser->port, 0, InvalidPlatform, plog_head, puser->ipaddr);

	if(NULL == pconenv){
		zlog_error2(DBGLOG, "control_user_process failed, pconenv is NULL!\n");
		pthread_detach(pthread_self());
		return NULL;
	}

	all_server_info *pser = pconenv->pserset->pserinfo;

	if(user_index >= control_get_max_user_num(pconenv) || user_index < 0){
		zlog_error2(DBGLOG, "exit control_user_process failed, index error, index = %d\n\n",
								user_index);
		pthread_detach(pthread_self());
		return NULL;
	}

	ret_buf = (int8_t *)r_malloc(CONTROL_DATA_LEN);
	socket = get_conuser_tcpsock(puser);
	zlog_debug2(DBGLOG, "enter control_user_process function, index = %d\n", user_index);

	set_send_timeout(socket, CONTROL_USER_TCP_SOCKET_SEND_TIMEOUT);
//	set_recv_timeout(socket, CONTROL_USER_TCP_SOCKET_RECV_TIMEOUT);

	r_memset(&msg,0,sizeof(msg));

#if 0
	int32_t fileflags;
	fileflags = fcntl(socket, F_GETFL, 0);
	if(fileflags < 0){
		zlog_error(DBGLOG, "fcntl F_GETFL failed, err msg: %s\n", strerror(errno));
		goto EXIT;
	}

	ret = fcntl(socket, F_SETFL, fileflags & (~O_NONBLOCK));
	if(ret < 0){
		zlog_error(DBGLOG, "fcntl F_SETFL err msg: %s\n", strerror(errno));
		goto EXIT;
	}
#endif

	if(puser->port != CONTROL_FOR_SERVER_PORT){
		platform = RecServer;
		set_conuser_platform(puser, platform);
		set_user_function_set(puser, platform);
	}

	set_user_login(pconenv, user_index);

	zlog_debug2(OPELOG, "user login!\n");

	puser->run_status = CONTROL_RUNNING;
#if 0
	static int32_t req_a_count = 0;
	static int32_t req_s_count = 0;
#endif
	headlen = CONTROL_MSGHEAD_LEN;
	while(CONTROL_RUNNING == pconenv->run_status && CONTROL_RUNNING == puser->run_status){
		r_memset(xml_buf, 0, CONTROL_DATA_LEN);
		r_memset(snd_buf, 0, CONTROL_DATA_LEN);
		r_memset(passkey, 0, PASS_KEY_LEN);
		force_msgcode = -1;
		ret_code = -1;
		msgtype = -1;
		datalen = 0;

		seret = 0;
		timeout.tv_sec = 15;	/* 注意:此时间需要大于与直播节点和管理平台的心跳间隔时间 */
		timeout.tv_usec = 100;
		FD_ZERO(&readfd);
		FD_SET(socket, &readfd);

		seret = select(socket+1, &readfd, NULL, NULL, &timeout);
		if(seret == 0){	/* 超时 */
			if(LiveNode == puser->platform
				|| ManagePlatform == puser->platform
				|| IpadUser == puser->platform
				|| Director == puser->platform){
				if(CONTROL_FALSE == puser->heart_beat){/* 心跳不在了 */
					zlog_debug(OPELOG, "no heart beat, close peer client, platform = %d, index = %d!\n",
														puser->platform, puser->index);
					break;
				}
				puser->heart_beat = CONTROL_FALSE;
				if(LiveNode == puser->platform){
	//				int8_t cmmd[128] = {0};
					zlog_debug(OPELOG, "############# port = %d, index = %d #############\n", puser->peerport, puser->index);
	//				sprintf(cmmd, "tcpdump -i eth0 -s 0 -c 200 port %d -w ./LIVE_NODE_%d", puser->peerport, puser->index);
	//				r_system(cmmd);
				}
			}
#if 0
			if(RecServer == puser->platform){
				puser->heart_beat++;
				if(puser->heart_beat > 3)
					break;
			}
#endif
			usleep(10000);
			continue;
		}
		else if(seret < 0){ /* 异常 */
			zlog_error2(OPELOG, "---[control_user_process] failed, [select] error, errmsg = %s\n",
									strerror(errno));
			break;
		}

		if(!FD_ISSET(socket, &readfd)){
			usleep(10000);
			continue;
		}

		/* 接收XML文本数据 */
		pthread_mutex_lock(&puser->sock_m);
		return_status = recv_user_xml_data(socket, xml_buf+headlen, &force_msgcode, plog_head, &datalen);
		if(return_status != STATUS_SUCCESS){
			zlog_error2(DBGLOG, "---[control_user_process] failed, [recv_user_xml_data] error!\n");
			pthread_mutex_unlock(&puser->sock_m);
			break;
		}
		puser->datalen = datalen;
#if 0
		zlog_debug2(OPELOG, "======================== recv buf ========================\n");
		zlog_debug2(OPELOG, "\n\n%s\n", xml_buf+headlen);
		zlog_debug2(OPELOG, "==========================================================\n\n");
#endif
		if(force_msgcode == CONTROL_OLD_MSGHEADER_MSGCODE){
			msgcode = force_msgcode;		// 强制使用此msgcode
		}
		else {
			/* 解析 msgcode 和 passkey */
			return_status = resolve_msgcode_and_passkey_or_returncode(xml_buf+headlen, puser,
															&msgcode, passkey,
															&ret_code, &msgtype);
			if(return_status != STATUS_SUCCESS){
				zlog_error2(OPELOG, "\n======================== recv buf failed!!!========================\n");
				zlog_error2(OPELOG, "\n\n%s\n\n", xml_buf+headlen);
				zlog_error2(OPELOG, "==========================================================\n\n");
				pthread_mutex_unlock(&puser->sock_m);
				continue;
			}

			/* 获取平台类型 */
			if(puser->port != CONTROL_FOR_SERVER_PORT){
				platform = RecServer;
			}
			else {
				platform = get_platform_from_passkey(passkey);
				if(platform == InvalidPlatform && ret_code == -1){ /* 平台无效且此为非响应消息 */
					zlog_error2(OPELOG, "---[control_user_process] failed, platform error, passkey = %s\n", passkey);
					pthread_mutex_unlock(&puser->sock_m);
					break;
				}
				if(MediaCenter == platform){
					if(pser->FtpInfo.Mode != FTP_MODE_MEDIACENTER){
						zlog_error2(OPELOG, "---[control_user_process] failed, this is third ftp mode!\n");
						pthread_mutex_unlock(&puser->sock_m);
						break;
					}
				}
			}

			/* 除了第一次，后续不进行平台检查，短连接的除外 */
			if(lastplatform == InvalidPlatform){
				/* 检查在线平台唯一性 */

				lastplatform = platform;

				/* 设置本平台类型 */
				return_status = set_conuser_platform(puser, platform);
				if(return_status != STATUS_SUCCESS){
					zlog_error2(OPELOG, "---[control_user_process] failed, [set_conuser_platform] error!\n");
					pthread_mutex_unlock(&puser->sock_m);
					break;
				}

//				if(MediaCenter == platform){
//					http_set_post_url(puser, HTTP_SERVER_URL_PORT);
//				}

				if(ManagePlatform == platform ||Director == puser->platform){
					pconenv->manager_flag = CONTROL_TRUE;
				}

				/* 注册接口集 */
				return_status = set_user_function_set(puser, platform);
				if(return_status != STATUS_SUCCESS){
					zlog_error2(OPELOG, "---[control_user_process] failed, [set_user_function_set] error!\n");
					pthread_mutex_unlock(&puser->sock_m);
					break;
				}
			}
			r_memset(puser->log_head, 0, 32);
			r_memset(plog_head, 0, 32);
			get_user_log_head(puser->port, user_index, platform, puser->log_head, puser->ipaddr);
			r_strcpy(plog_head, puser->log_head);
		}
		pthread_mutex_unlock(&puser->sock_m);

		if(msgcode != MSGCODE_HEART_REPORT_REQ && msgcode != MSGCODE_ENCINFO_REPORT_REQ
			&& msgcode != MSGCODE_LIVENODE_HEART_BEAT && msgcode != MSGCODE_GET_AUDIO_VOLUME
			&& msgcode != MSGCODE_GET_SERVER_SYS_PARAMS && !(msgcode == MSGCODE_GET_ROOM_INFO
			&& (puser->platform == ManagePlatform || puser->platform == IpadUser || Director == puser->platform)))
		{
			zlog_debug2(OPELOG, "======================== recv buf ========================\n");
			zlog_debug2(OPELOG, "\n xmllen:[%d][%s]\n", r_strlen(xml_buf+headlen), xml_buf+headlen);
			zlog_debug2(OPELOG, "==========================================================\n\n");
		}

		set_user_msgtype(puser, msgtype);

		if(LiveNode == puser->platform
			|| ManagePlatform == puser->platform
			|| IpadUser== puser->platform
			|| Director == puser->platform){
			puser->heart_beat = CONTROL_TRUE;
		}

		if(RecServer == puser->platform){
			puser->heart_beat = 1;
		}

		if(msgcode != MSGCODE_HEART_REPORT_REQ && msgcode != MSGCODE_ENCINFO_REPORT_REQ
			&& msgcode != MSGCODE_LIVENODE_HEART_BEAT && msgcode != MSGCODE_GET_AUDIO_VOLUME
			&& msgcode != MSGCODE_GET_SERVER_SYS_PARAMS && !(msgcode == MSGCODE_GET_ROOM_INFO
			&&(puser->platform == ManagePlatform || puser->platform == IpadUser || Director == puser->platform)))
		{
			zlog_debug2(OPELOG, "->>>-{preparing do [%s:%d] <%s>}->>>\n", find_treaty_text(msgcode),
																msgcode, get_msgtype_value(msgtype, value));
		}
#if 0
		if((r_strcmp(puser->ipaddr, "192.168.4.3")==0) && msgcode == 30011){

			if(r_strstr(xml_buf+headlen, "<EncodeIndex>A")!=NULL){
				req_a_count++;
			}
			else if(r_strstr(xml_buf+headlen, "<EncodeIndex>S")!=NULL){
				req_s_count++;
			}
			zlog_debug2(OPELOG, "req_a_count = %d, req_s_count = %d\n", req_a_count, req_s_count);
		}
#endif
		/* 查找对应msgcode的处理方法 */
		treaty_index = find_treaty_index(msgcode);
		if(treaty_index < 0 || treaty_index > CONTROL_MAX_COMMAND){
			zlog_debug2(DBGLOG, "---[control_user_process] failed, [find_treaty_index] error!, msgcode = %d\n", msgcode);
			zlog_debug2(OPELOG, "-<<<-[find_treaty_index] error! <%s> -<<<\n", get_msgtype_value(msgtype, value));
			continue;
		}

		if(ptreaty[treaty_index].deal_process != NULL){
			return_status = ptreaty[treaty_index].deal_process(pconenv, puser, xml_buf,
														snd_buf, ret_buf, &ret_len, msgcode);
			if(return_status != STATUS_SUCCESS && puser->ack.msgtype == XML_MSG_TYPE_REQ){
				zlog_debug2(DBGLOG, "------[control_user_process] failed, [deal_process] error, msgcode = %d\n", msgcode);
				zlog_debug2(OPELOG, "-<<<-[failed to do %s:%d] <%s> -<<<\n", find_treaty_text(msgcode), msgcode,
																	get_msgtype_value(msgtype, value));
			}
			else{
				if(msgcode != MSGCODE_HEART_REPORT_REQ && msgcode != MSGCODE_ENCINFO_REPORT_REQ
					&& msgcode != MSGCODE_LIVENODE_HEART_BEAT && msgcode != MSGCODE_GET_AUDIO_VOLUME
					&& msgcode != MSGCODE_GET_SERVER_SYS_PARAMS && !(msgcode == MSGCODE_GET_ROOM_INFO
					&&(puser->platform == ManagePlatform || puser->platform == IpadUser ||Director == puser->platform)))
				{
					zlog_debug2(OPELOG, "-<<<-{completed process [%s:%d] <%s>}-<<<\n\n", find_treaty_text(msgcode), msgcode,
																get_msgtype_value(msgtype, value));
				}
			}
		}
		zlog_debug2(DBGLOG, "-debug{completed process [%s:%d] <%s>}-<<<\n\n", find_treaty_text(msgcode), msgcode,
													get_msgtype_value(msgtype, value));
		if(LiveNode == puser->platform
			|| ManagePlatform == puser->platform
			|| IpadUser== puser->platform
			|| Director == puser->platform){
			puser->heart_beat = CONTROL_TRUE;
		}

		if(RecServer == puser->platform){
			puser->heart_beat = 1;
		}
	}

	if(puser->platform != RecServer && puser->platform != MediaCenter && puser->platform != InvalidPlatform){
		/* 先停止由此直播节点请求的码流，然后退出 */
		stream_request_cleanup_lives_user_info(pconenv, puser);
		clean_user_live_user_info(pconenv, puser->index);
	}

	puser->run_status = CONTROL_STOPED;

	if(puser->platform != InvalidPlatform)
		timeout_que_cleanup(&pconenv->pserset->timeque, puser->platform, puser->index);

	if(ret_buf)
		r_free(ret_buf);

	set_user_logout(pconenv, user_index);
	zlog_debug2(OPELOG, "user logout!\n");

	clear_conuser_tcpsock(puser);
	close(socket);

	puser->index = -1;
	puser->pcmd.forward_process = NULL;
	puser->pconenv = NULL;
	puser->platform = InvalidPlatform;

	for(j = 0; j < CONTROL_ROOM_SERVER_MAX_USER; j++){
		puser->art_volume[j] = 0;
	}

	remove_conuser_index(pconenv, user_index);

	zlog_debug2(OPELOG, "----------------------- Exit ControlUser Thread ! -----------------------\n");
	pthread_detach(pthread_self());

	return NULL;
}


static void *control_server_thr(void *args)
{
	struct sockaddr_in ser_addr;
	struct sockaddr_in cli_addr;

	int32_t	nlen = 0;
	int32_t result = 0;
	int32_t cnt = 0;
	int32_t cli_socket = 0;
	int32_t ser_socket = 0;
	int32_t user_index = 0;
	int32_t ret = 0;

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};

	con_user *puser = NULL;

	if(NULL == args){
		zlog_error(DBGLOG, "control_server_thr failed, args is NULL!\n");
		return NULL;
	}

	control_env *pconenv = (control_env *)args;

	r_memset(plog_head, 0, 32);
	get_server_log_head(pconenv->ser_port, plog_head, NULL);

	zlog_debug2(OPELOG, "****************** start control_server_thr ******************\n");
	zlog_debug2(DBGLOG, "listening port = %d\n", pconenv->ser_port);

	bzero(&ser_addr, sizeof(struct sockaddr_in));
	ser_addr.sin_family	= AF_INET;
	ser_addr.sin_port	= htons(pconenv->ser_port);

	if(pconenv->ser_port != CONTROL_FOR_SERVER_PORT){
		ser_addr.sin_addr.s_addr= inet_addr(LOCAL_LOOP_INTERFACE);
	}
	else{
		ser_addr.sin_addr.s_addr= htonl(INADDR_ANY);
	}

	ser_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(ser_socket < 0){
		zlog_error2(DBGLOG, "create control_server_thr listening socket failed, err msg: %s\n",
								strerror(errno));
		pthread_detach(pthread_self());
		return NULL;
	}

	control_set_server_socket(pconenv, ser_socket);

	int32_t opt = 1;
	setsockopt(ser_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	ret = bind(ser_socket, (struct sockaddr*)&ser_addr, sizeof(ser_addr));
	if(ret < 0){
		zlog_error2(DBGLOG, "control_server_thr failed, bind error, err msg: %s\n", strerror(errno));
		goto EXIT;
	}

	set_send_timeout(ser_socket, CONTROL_SERVER_TCP_SOCKET_SEND_TIMEOUT);
	set_recv_timeout(ser_socket, CONTROL_SERVER_TCP_SOCKET_RECV_TIMEOUT);

	ret = listen(ser_socket, CONTROL_LISTEN_MAX_NUM);
	if(ret < 0){
		zlog_error2(DBGLOG, "control_server_thr failed, listen error, err msg: %s", strerror(errno));
		goto EXIT;
	}

	while(pconenv->run_status != CONTROL_STOPED){
		if(CONTROL_PREPARING == pconenv->run_status){
//			zlog_debug(DBGLOG, "now control is in preparing status\n");
			usleep(500000);
			continue;
		}

		r_memset(&cli_addr, 0, sizeof(struct sockaddr_in));
		nlen = sizeof(struct sockaddr_in);
		cli_socket = accept(ser_socket, (void*)&cli_addr, (socklen_t *)&nlen);
		if(cli_socket > 0){
			r_memset(plog_head, 0, 32);
			get_server_log_head(pconenv->ser_port, plog_head, (int8_t *)inet_ntoa(cli_addr.sin_addr));

			/* 连接有效，获取用户索引 */
			user_index = get_conuser_index(pconenv);
			if(user_index < 0){
				zlog_error2(DBGLOG, "accept failed, invalid user index, inex = %d\n", user_index);
				close(cli_socket);
				usleep(20000);
				continue;
			}
			puser = &pconenv->user[user_index];

			zlog_debug2(OPELOG, "accept a new client!\n");

			zlog_debug2(DBGLOG, "=============================================================\n");
			zlog_debug2(DBGLOG, "control count = %d\n", get_conuser_num(pconenv));
			zlog_debug2(DBGLOG, "control userindex = %d\n", user_index);
			zlog_debug2(DBGLOG, "control socket = %d\n", cli_socket);
			zlog_debug2(DBGLOG, "=============================================================\n");

			set_conuser_tcpsock(puser, cli_socket);

			puser->pconenv = pconenv;
			puser->index = user_index;
			result = pthread_create(&pconenv->clthid[cnt], NULL, (void *)control_user_process, puser);
			if(result < 0){
				close(cli_socket);
				clear_conuser_tcpsock(puser);
				remove_conuser_index(pconenv, user_index);
				zlog_error2(DBGLOG, "creat pthread clientmsg error  = %d!, err msg = %s\n", errno, strerror(errno));
				usleep(20000);
				continue;
			}
			else if(result == 0){
				/* 连接成功*/
				puser->port = pconenv->ser_port;
				r_memset(puser->ipaddr, 0, 64);
				r_strcpy(puser->ipaddr, (const int8_t *)inet_ntoa(cli_addr.sin_addr));
				puser->peerport = ntohs(cli_addr.sin_port);
				zlog_debug2(OPELOG, "create live client server success!!!\n");
				usleep(30000);
			}
		} else{
			if(errno == EAGAIN) {
				usleep(100000);
			}
		}
	}

EXIT:

	close(ser_socket);
	pconenv->ser_socket = -1;
	pconenv->run_status = CONTROL_STOPED;

	zlog_debug2(DBGLOG, "-------------- control server thread exit !! --------------\n");

	pthread_detach(pthread_self());
	return NULL;
}

inline int32_t micro_sleep(control_env *penv, int32_t second)
{
	uint32_t count = 0;

	if(NULL == penv || second < 0 || second > 10){
		zlog_error(DBGLOG, "micro_sleep failed, second = %d\n", second);
		sleep(1);
		return -1;
	}

	count = second*10;
	while(count--){
		if(CONTROL_TRUE == penv->manager_flag){
			penv->manager_flag = CONTROL_FALSE;
			break;
		}
		usleep(100000);
	}

	return 0;
}

static int send_xml_heart(server_set *pserset, platform_em platform)
{
	con_user *pforobj = NULL;
	int ret = STATUS_FAILED;
	int send_fail_count = 0;
	all_server_info *pser = pserset->pserinfo;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int8_t * send_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);

	pforobj = find_forward_obj(pserset, platform, 0, 0);
	if(pforobj != NULL){
		r_memset(send_buf, 0, CONTROL_DATA_LEN);
		package_http_heart_report_xml_data(send_buf+headlen, pser, MSGCODE_HEART_REPORT_REQ);
		zlog_debug(DBGLOG, "\n%s\n\n", send_buf+headlen);
		ret = send_user_xml_data(NULL, pforobj, send_buf, NULL, NULL);
		if(ret != STATUS_SUCCESS){
			send_fail_count++;
			if((send_fail_count*pser->HBeatInfo.post_time) > 20){
				/* 管理平台已不存在了，但线程没有退出 */
				zlog_debug(OPELOG, "send heart beat to manager timeout, close the manager!\n");
				pforobj->run_status = CONTROL_STOPED;
			}
		}else{
			send_fail_count = 0;
		}
	}
	return ret;
}

static void *control_ManagePlatform_heart_thr(void *args)
{
	int32_t ret_len = 0;
	http_env *phttp = NULL;
	all_server_info *pser = NULL;
	int8_t *send_buf = NULL;
	int8_t *ret_buf = NULL;
	int8_t url[HTTP_SERVER_URL_MAX_LEN] = {0};
	int8_t ipaddr[16] = {0};
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t manager_send_fail_count = 0;
	int32_t control_send_fail_count = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	control_env *penv = NULL;

	if(NULL == args){
		zlog_error(DBGLOG, "control_http_post_thr failed, args is NULL!\n");
		goto EXIT;
	}

	phttp = (http_env *)args;
	pser = phttp->pserset->pserinfo;
	if(NULL == pser){
		zlog_error(DBGLOG, "control_http_post_thr failed, all server info not found!\n");
		goto EXIT;
	}

	penv = &phttp->pserset->forser;
	if(NULL == penv){
		zlog_error(DBGLOG, "control_http_post_thr failed, forserver not found!\n");
		goto EXIT;
	}

	send_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);
	ret_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);
	if(NULL == send_buf || NULL == ret_buf){
		zlog_error(DBGLOG, "control_http_post_thr failed, malloc failed!\n");
		goto EXIT;
	}

	while(phttp->run_status != CONTROL_STOPED){
		if(CONTROL_PREPARING == phttp->run_status){
			zlog_debug(DBGLOG, "http post task is in preparing status!\n");
			usleep(500000);
			continue;
		}

		/* 与管理平台的心跳 */
		pforobj = find_forward_obj(phttp->pserset, ManagePlatform, 0, 0);
		if(pforobj != NULL){
			r_memset(send_buf, 0, CONTROL_DATA_LEN);
			package_http_heart_report_xml_data(send_buf+headlen, pser, MSGCODE_HEART_REPORT_REQ);
			zlog_debug(DBGLOG, "\n[%s]\n\n", send_buf+headlen);
			ret = send_user_xml_data(NULL, pforobj, send_buf, ret_buf, &ret_len);
			if(ret != STATUS_SUCCESS){
				manager_send_fail_count++;
				if((manager_send_fail_count*pser->HBeatInfo.post_time) > 20){
					/* 管理平台已不存在了，但线程没有退出 */
					zlog_debug(OPELOG, "send heart beat to manager timeout, close the manager!\n");
					pforobj->run_status = CONTROL_STOPED;
				}
			}else{
				manager_send_fail_count = 0;
			}
		}


		/* TODO: 广播，还是仅发送给媒体中心? */
		micro_sleep(penv, pser->HBeatInfo.post_time);
//		r_sleep(pser->HBeatInfo.post_time);
	}

EXIT:
	if(send_buf)
		r_free(send_buf);

	if(ret_buf)
		r_free(ret_buf);

	pthread_detach(pthread_self());
	return NULL;
}


static void *control_Director_heart_thr(void *args)
{
	int32_t ret_len = 0;
	http_env *phttp = NULL;
	all_server_info *pser = NULL;
	int8_t *send_buf = NULL;
	int8_t *ret_buf = NULL;
	int8_t url[HTTP_SERVER_URL_MAX_LEN] = {0};
	int8_t ipaddr[16] = {0};
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t manager_send_fail_count = 0;
	int32_t control_send_fail_count = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	control_env *penv = NULL;

	if(NULL == args){
		zlog_error(DBGLOG, "control_http_post_thr failed, args is NULL!\n");
		goto EXIT;
	}

	phttp = (http_env *)args;
	pser = phttp->pserset->pserinfo;
	if(NULL == pser){
		zlog_error(DBGLOG, "control_http_post_thr failed, all server info not found!\n");
		goto EXIT;
	}

	penv = &phttp->pserset->forser;
	if(NULL == penv){
		zlog_error(DBGLOG, "control_http_post_thr failed, forserver not found!\n");
		goto EXIT;
	}

	send_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);
	ret_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);
	if(NULL == send_buf || NULL == ret_buf){
		zlog_error(DBGLOG, "control_http_post_thr failed, malloc failed!\n");
		goto EXIT;
	}

	while(phttp->run_status != CONTROL_STOPED){
		if(CONTROL_PREPARING == phttp->run_status){
			zlog_debug(DBGLOG, "http post task is in preparing status!\n");
			usleep(500000);
			continue;
		}

		/* 与导播台的心跳 */
		pforobj = find_forward_obj(phttp->pserset, Director, 0, 0);
		if(pforobj != NULL){
			r_memset(send_buf, 0, CONTROL_DATA_LEN);
			package_http_heart_report_xml_data(send_buf+headlen, pser, MSGCODE_HEART_REPORT_REQ);
			zlog_debug(DBGLOG, "\n[%s]\n\n", send_buf+headlen);
			ret = send_user_xml_data(NULL, pforobj, send_buf, ret_buf, &ret_len);
			if(ret != STATUS_SUCCESS){
				manager_send_fail_count++;
				if((manager_send_fail_count*pser->HBeatInfo.post_time) > 20){
					/* 管理平台已不存在了，但线程没有退出 */
					zlog_debug(OPELOG, "send heart beat to manager timeout, close the manager!\n");
					pforobj->run_status = CONTROL_STOPED;
				}
			}else{
				manager_send_fail_count = 0;
			}
		}

		/* TODO: 广播，还是仅发送给媒体中心? */
		micro_sleep(penv, pser->HBeatInfo.post_time);
//		r_sleep(pser->HBeatInfo.post_time);
	}

EXIT:
	if(send_buf)
		r_free(send_buf);

	if(ret_buf)
		r_free(ret_buf);

	pthread_detach(pthread_self());
	return NULL;
}


static void *control_ComControl_heart_thr(void *args)
{
	int32_t ret_len = 0;
	http_env *phttp = NULL;
	all_server_info *pser = NULL;
	int8_t *send_buf = NULL;
	int8_t *ret_buf = NULL;
	int8_t url[HTTP_SERVER_URL_MAX_LEN] = {0};
	int8_t ipaddr[16] = {0};
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t manager_send_fail_count = 0;
	int32_t control_send_fail_count = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	control_env *penv = NULL;

	if(NULL == args){
		zlog_error(DBGLOG, "control_http_post_thr failed, args is NULL!\n");
		goto EXIT;
	}

	phttp = (http_env *)args;
	pser = phttp->pserset->pserinfo;
	if(NULL == pser){
		zlog_error(DBGLOG, "control_http_post_thr failed, all server info not found!\n");
		goto EXIT;
	}

	penv = &phttp->pserset->forser;
	if(NULL == penv){
		zlog_error(DBGLOG, "control_http_post_thr failed, forserver not found!\n");
		goto EXIT;
	}

	send_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);
	ret_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);
	if(NULL == send_buf || NULL == ret_buf){
		zlog_error(DBGLOG, "control_http_post_thr failed, malloc failed!\n");
		goto EXIT;
	}

	while(phttp->run_status != CONTROL_STOPED){
		if(CONTROL_PREPARING == phttp->run_status){
			zlog_debug(DBGLOG, "http post task is in preparing status!\n");
			usleep(500000);
			continue;
		}

		/* 与中控 的心跳 */
		pforobj = find_forward_obj(phttp->pserset, ComControl, 0, 0);
		if(pforobj != NULL){
			r_memset(send_buf, 0, CONTROL_DATA_LEN);
			package_http_heart_report_xml_data(send_buf+headlen, pser, MSGCODE_HEART_REPORT_REQ);
//			zlog_debug(OPELOG, "\n%s\n\n", send_buf+headlen);
			ret = send_user_xml_data(NULL, pforobj, send_buf, ret_buf, &ret_len);
			if(ret != STATUS_SUCCESS){
				control_send_fail_count++;
				if((control_send_fail_count*pser->HBeatInfo.post_time) > 20){
					/* 中控已不存在了，但线程没有退出 */
					zlog_debug(OPELOG, "send heart beat to ComControl timeout, close the ComControl!\n");
					pforobj->run_status = CONTROL_STOPED;
				}
			}else{
				control_send_fail_count = 0;
			}
		}


		/* TODO: 广播，还是仅发送给媒体中心? */
		micro_sleep(penv, pser->HBeatInfo.post_time);
//		r_sleep(pser->HBeatInfo.post_time);
	}

EXIT:
	if(send_buf)
		r_free(send_buf);

	if(ret_buf)
		r_free(ret_buf);

	pthread_detach(pthread_self());
	return NULL;
}


static void *control_NetControl_heart_thr(void *args)
{
	int32_t ret_len = 0;
	http_env *phttp = NULL;
	all_server_info *pser = NULL;
	int8_t *send_buf = NULL;
	int8_t *ret_buf = NULL;
	int8_t url[HTTP_SERVER_URL_MAX_LEN] = {0};
	int8_t ipaddr[16] = {0};
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t manager_send_fail_count = 0;
	int32_t control_send_fail_count = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	control_env *penv = NULL;

	if(NULL == args){
		zlog_error(DBGLOG, "control_http_post_thr failed, args is NULL!\n");
		goto EXIT;
	}

	phttp = (http_env *)args;
	pser = phttp->pserset->pserinfo;
	if(NULL == pser){
		zlog_error(DBGLOG, "control_http_post_thr failed, all server info not found!\n");
		goto EXIT;
	}

	penv = &phttp->pserset->forser;
	if(NULL == penv){
		zlog_error(DBGLOG, "control_http_post_thr failed, forserver not found!\n");
		goto EXIT;
	}

	send_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);
	ret_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);
	if(NULL == send_buf || NULL == ret_buf){
		zlog_error(DBGLOG, "control_http_post_thr failed, malloc failed!\n");
		goto EXIT;
	}

	while(phttp->run_status != CONTROL_STOPED){
		if(CONTROL_PREPARING == phttp->run_status){
			zlog_debug(DBGLOG, "http post task is in preparing status!\n");
			usleep(500000);
			continue;
		}

		/* 与NETCTRL 的心跳 */
		pforobj = find_forward_obj(phttp->pserset, NetControl, 0, 0);
		if(pforobj != NULL){
			r_memset(send_buf, 0, CONTROL_DATA_LEN);
			package_http_heart_report_xml_data(send_buf+headlen, pser, MSGCODE_HEART_REPORT_REQ);
//			zlog_debug(OPELOG, "\n%s\n\n", send_buf+headlen);
			ret = send_user_xml_data(NULL, pforobj, send_buf, ret_buf, &ret_len);
			if(ret != STATUS_SUCCESS){
				control_send_fail_count++;
				if((control_send_fail_count*pser->HBeatInfo.post_time) > 20){
					/* NETCTRL已不存在了，但线程没有退出 */
					zlog_debug(OPELOG, "send heart beat to NetControl timeout, close the NetControl!\n");
					pforobj->run_status = CONTROL_STOPED;
				}
			}else{
				control_send_fail_count = 0;
			}
		}


		/* TODO: 广播，还是仅发送给媒体中心? */
		micro_sleep(penv, pser->HBeatInfo.post_time);
//		r_sleep(pser->HBeatInfo.post_time);
	}

EXIT:
	if(send_buf)
		r_free(send_buf);

	if(ret_buf)
		r_free(ret_buf);

	pthread_detach(pthread_self());
	return NULL;
}

static void *control_http_post_thr(void *args)
{
	int32_t ret_len = 0;
	http_env *phttp = NULL;
	all_server_info *pser = NULL;
	int8_t *send_buf = NULL;
	int8_t *ret_buf = NULL;
	int8_t url[HTTP_SERVER_URL_MAX_LEN] = {0};
	int8_t ipaddr[16] = {0};
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t manager_send_fail_count = 0;
	int32_t control_send_fail_count = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	control_env *penv = NULL;

	if(NULL == args){
		zlog_error(DBGLOG, "control_http_post_thr failed, args is NULL!\n");
		goto EXIT;
	}

	phttp = (http_env *)args;
	pser = phttp->pserset->pserinfo;
	if(NULL == pser){
		zlog_error(DBGLOG, "control_http_post_thr failed, all server info not found!\n");
		goto EXIT;
	}

	penv = &phttp->pserset->forser;
	if(NULL == penv){
		zlog_error(DBGLOG, "control_http_post_thr failed, forserver not found!\n");
		goto EXIT;
	}

	send_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);
	ret_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);
	if(NULL == send_buf || NULL == ret_buf){
		zlog_error(DBGLOG, "control_http_post_thr failed, malloc failed!\n");
		goto EXIT;
	}

	while(phttp->run_status != CONTROL_STOPED){
		if(CONTROL_PREPARING == phttp->run_status){
			zlog_debug(DBGLOG, "http post task is in preparing status!\n");
			usleep(500000);
			continue;
		}

		/* 打包服务器信息 */
		r_memset(send_buf, 0, CONTROL_XML_DATA_LEN);
		package_http_heart_report_xml_data(send_buf, pser, MSGCODE_HTTP_HEART_REPORT_REQ);

		pthread_mutex_lock(&pser->info_m);
		r_memset(url, 0, HTTP_SERVER_URL_MAX_LEN);
		r_strcpy(url, pser->HBeatInfo.post_url);
		pthread_mutex_unlock(&pser->info_m);


		if(pser->FtpInfo.Mode != FTP_MODE_THIRDFTP){
			/* 与媒体中心的心跳 */
			ret_len = CONTROL_XML_DATA_LEN;
	//		zlog_debug(DBGLOG, "url: %s\n", url);
	//		zlog_debug(DBGLOG, "->>>>>>>>>>>> length = %d\n%s\n\n", r_strlen(send_buf), send_buf);
			r_memset(ret_buf, 0, CONTROL_DATA_LEN);
			ret = mid_http_post((char *)url, (char *)send_buf, r_strlen(send_buf), (char *)ret_buf, &ret_len);
			if(ret == -1){
				zlog_debug(DBGLOG, "send heart beat to media center failed, ret = %d\n", ret);
#if 1
				pthread_mutex_lock(&pser->info_m);
				r_bzero(ipaddr, sizeof(ipaddr));
				r_strcpy(ipaddr, (int8_t *)"0.0.0.0");
				penv->pserset->pserinfo->ConfigInfo.media_addr = inet_addr((char *)ipaddr);
				pthread_mutex_unlock(&pser->info_m);
#endif
			} else {
#if 1
				//从媒体中心URL 中提取IP 地址
				pthread_mutex_lock(&pser->info_m);
				r_bzero(ipaddr, sizeof(ipaddr));
				sscanf((char *)(url), "%*[^/]//%[^/]/", ipaddr);
				penv->pserset->pserinfo->ConfigInfo.media_addr = inet_addr((char *)ipaddr);
				pthread_mutex_unlock(&pser->info_m);
#endif
			}
		}

		/* TODO: 广播，还是仅发送给媒体中心? */
		micro_sleep(penv, pser->HBeatInfo.post_time);
//		r_sleep(pser->HBeatInfo.post_time);
	}

EXIT:
	if(send_buf)
		r_free(send_buf);

	if(ret_buf)
		r_free(ret_buf);

	pthread_detach(pthread_self());
	return NULL;
}


int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
  struct nlmsghdr *nlHdr;
  int readLen = 0, msgLen = 0;
  do{
    //收到内核的应答
    if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)
    {
      perror("SOCK READ: ");
      return -1;
    }

    nlHdr = (struct nlmsghdr *)bufPtr;
    //检查header是否有效
    if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))
    {
      perror("Error in recieved packet");
      return -1;
    }


    if(nlHdr->nlmsg_type == NLMSG_DONE)
    {
      break;
    }
    else
    {

      bufPtr += readLen;
      msgLen += readLen;
    }


    if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
    {

     break;
    }
  } while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
  return msgLen;
}


int32_t set_ipinfo(all_server_info *pinfo)
{
	server_info  newInfo;
	uint32_t ip;

	if(NULL == pinfo){
		zlog_debug(DBGLOG, "--[set_ipinfo] failed, params is NULL!\n");
		return -1;
	}

	r_memcpy(&newInfo, &pinfo->ServerInfo, sizeof(server_info));

	zlog_debug(DBGLOG, "    eth0 GetIp ....\n");

	//采用默认值
	ip = GetIPaddr((int8_t*)"eth0");
	if(-1 == ip){
		newInfo.LanAddr    = inet_addr("169.254.1.11");
		newInfo.LanNetmask = inet_addr("255.255.255.0");
		newInfo.LanGateWay = inet_addr("169.254.1.1");
		r_system((const int8_t *)"ifconfig eth0 169.254.1.11 netmask 255.255.255.0");
		r_system((const int8_t *)"route add default gw 169.254.1.1 dev eth0");
	}
	else{
		newInfo.LanAddr    = ip;
		newInfo.LanNetmask = GetNetmask((int8_t*)"eth0");
		newInfo.LanGateWay = ip_get_proc((int8_t*)"eth0");
	}

	modify_server_info_only((const int8_t *)CONFIG_TABLE_FILE, &pinfo->ServerInfo, &newInfo);
	r_memcpy(&pinfo->ServerInfo, &newInfo, sizeof(server_info));

	r_memcpy(&newInfo, &pinfo->ServerInfo, sizeof(server_info));
	//采用默认值
	newInfo.WanAddr    = inet_addr("0.0.0.0");
	newInfo.WanNetmask = inet_addr("0.0.0.0");
	newInfo.WanGateWay = inet_addr("0.0.0.0");
	modify_server_info_only((const int8_t *)CONFIG_TABLE_FILE, &pinfo->ServerInfo, &newInfo);
	r_memcpy(&pinfo->ServerInfo, &newInfo, sizeof(server_info));

	return 1;
}


int32_t start_control_server_task(server_set **ppser)
{
	int32_t return_code = STATUS_FAILED;
	int32_t index = 0;
	server_set *pser = NULL;
	control_env *pcon = NULL;
	http_env *phttp = NULL;
	all_server_info *pinfo = NULL;
	int ret = 0;
	int8_t ipbuf[64]= {0};
//	struct in_addr addr;

	pser = (server_set *)r_malloc(sizeof(server_set));
	if(NULL == pser){
		zlog_error(DBGLOG, "start_control_server_task failed, r_malloc control handle error, err msg = %s\n",
								strerror(errno));
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_memset(pser, 0, sizeof(server_set));

	timeout_que_init(&pser->timeque);
	file_que_init(&pser->fileque);

	pinfo = (all_server_info *)r_malloc(sizeof(all_server_info));
	if(NULL == pinfo){
		zlog_error(DBGLOG, "start_control_server_task failed, r_malloc control handle error, err msg = %s\n",
								strerror(errno));
		return_code = STATUS_FAILED;
		goto EXIT;
	}
	init_all_server_info(pinfo);

	if(access(CONFIG_TABLE_FILE, F_OK) != 0){
		return_code = create_params_table_file((const int8_t *)CONFIG_TABLE_FILE, pinfo);
		if(return_code == STATUS_FAILED){
			zlog_debug(OPELOG, "--[start_control_server_task] failed, create_params_table_file error!\n");
		} else {
			zlog_debug(OPELOG, "--[start_control_server_task] success, create_params_table_file successful!\n");
		}
	}
	else {
		zlog_debug(OPELOG, "--[start_control_server_task], read_params_table_file !\n");
		ret = read_params_table_file((const int8_t *)CONFIG_TABLE_FILE, pinfo);
		if(ret < 0) {
			remove(CONFIG_TABLE_FILE);
			return_code = create_params_table_file((const int8_t *)CONFIG_TABLE_FILE, pinfo);
			if(return_code == STATUS_FAILED){
				zlog_debug(OPELOG, "--[read_params_table_file] failed, create_params_table_file error!\n");
			} else {
				zlog_debug(OPELOG, "--[read_params_table_file] failed, create_params_table_file successful!\n");
			}
		}
	}

	/* 同步时间 */
	pthread_mutex_lock(&pinfo->info_m);
	r_strcpy(ipbuf, pinfo->HBeatInfo.time_serip);
	pthread_mutex_unlock(&pinfo->info_m);
	ntp_time_sync(ipbuf);

	reset_record_params(pinfo);

	set_ipinfo(pinfo);
	pser->pserinfo = pinfo;

	/* 创建用于监听媒体中心、管理平台、第三方中控、直播节点的服务线程 */

	pcon = &pser->forser;
	control_init_env(pcon);
	control_set_server_set(pcon, pser);
	control_set_server_port(pcon, CONTROL_FOR_SERVER_PORT);
	control_set_max_user_num(pcon, CONTROL_FOR_SERVER_MAX_USER);
	return_code = pthread_create(&pcon->serthid, NULL,
							(void *)control_server_thr, (void *)pcon);
	if(return_code < 0){
		zlog_error(DBGLOG, "start_control_server_task failed, pthread_create error , err msg = %s\n",
								strerror(errno));
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	/* 创建用于监听会议室的服务线程 */
	for(index = 0; index < pinfo->ServerInfo.MaxRoom; index++){
		pcon = &pser->roomser[index];
		pcon->index = index;
		control_init_env(pcon);
		control_set_server_set(pcon, pser);
		control_set_server_port(pcon, CONTROL_ROOM_SERVER_PORT_START+index);
		control_set_max_user_num(pcon, 1);
		return_code = pthread_create(&pcon->serthid, NULL,
						(void *)control_server_thr, (void *)pcon);
		if(return_code < 0){
			zlog_error(DBGLOG, "start_control_server_task failed, pthread_create error , err msg = %s\n",
									strerror(errno));
			return_code = STATUS_FAILED;
			goto EXIT;
		}
	}

	phttp = &pser->http;
	http_init_env(phttp);
	http_set_server_set(phttp, pser);
	return_code = pthread_create(&phttp->http_thid, NULL,
							(void *)control_http_post_thr, (void *)phttp);
	if(return_code < 0){
		zlog_error(DBGLOG, "start_control_server_task failed, control_http_post_thr error , err msg = %s\n",
								strerror(errno));
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = pthread_create(&phttp->director_thid, NULL,
							(void *)control_Director_heart_thr, (void *)phttp);
	if(return_code < 0){
		zlog_error(DBGLOG, "start_control_server_task failed, control_Director_heart_thr error , err msg = %s\n",
								strerror(errno));
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = pthread_create(&phttp->m_thid, NULL,
							(void *)control_ManagePlatform_heart_thr, (void *)phttp);
	if(return_code < 0){
		zlog_error(DBGLOG, "start_control_server_task failed, control_ManagePlatform_heart_thr error , err msg = %s\n",
								strerror(errno));
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = pthread_create(&phttp->com_thid, NULL,
							(void *)control_ComControl_heart_thr, (void *)phttp);
	if(return_code < 0){
		zlog_error(DBGLOG, "start_control_server_task failed, control_ComControl_heart_thr error , err msg = %s\n",
								strerror(errno));
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = pthread_create(&phttp->net_thid, NULL,
							(void *)control_NetControl_heart_thr, (void *)phttp);
	if(return_code < 0){
		zlog_error(DBGLOG, "start_control_server_task failed, control_NetControl_heart_thr error , err msg = %s\n",
								strerror(errno));
		return_code = STATUS_FAILED;
		goto EXIT;
	}



	*ppser = pser;

	return STATUS_SUCCESS;

EXIT:

	if(pser){
		r_free(pser);
	}

	if(pinfo){
		r_free(pinfo);
	}

	*ppser = NULL;

	return STATUS_FAILED;
}


