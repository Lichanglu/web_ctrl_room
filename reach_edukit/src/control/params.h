/*
 * =====================================================================================
 *
 *       Filename:  paras.h
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


#ifndef _PARAMS_H_
#define _PARAMS_H_

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "command_resolve.h"
#include "web/webmiddle.h"
#define CONFIG_TABLE_FILE					("/usr/local/reach/.config/control_config.xml")


#define CONFIG_PARAMS_TABLES_KEY			(BAD_CAST "params_table")
#define CONFIG_PARAMS_USER_INFO_KEY			(BAD_CAST "user_info")
#define CONFIG_PARAMS_FTP_INFO_KEY			(BAD_CAST "ftp_info")
#define CONFIG_PARAMS_SYS_INFO_KEY			(BAD_CAST "sys_info")
#define CONFIG_PARAMS_HEART_BEAT_INFO_KEY	(BAD_CAST "heart_beat_info")
#define CONFIG_PARAMS_SERVER_INFO_KEY		(BAD_CAST "server_info")
#define CONFIG_PARAMS_ROOM_INFO_KEY			(BAD_CAST "room_info")
#define CONFIG_PARAMS_AUDIO_INFO_KEY		(BAD_CAST "audio_info")
#define CONFIG_PARAMS_ENC_INFO_KEY			(BAD_CAST "enc_info")
#define CONFIG_PARAMS_QUALITY_INFO_KEY		(BAD_CAST "quality_info")
#define CONFIG_PARAMS_QUALITY_ENABLE_KEY	(BAD_CAST "quality_enable")
#define CONFIG_PARAMS_CONFIG_INFO_KEY       (BAD_CAST "config_info")
#define CONFIG_PARAMS_PICTURE_SYNT_INFO_KEY	(BAD_CAST "pic_synt_info")
#define CONFIG_PARAMS_CAMCTRL_ADDR_KEY      (BAD_CAST "camctrl_addr_info")


#define	FIRST_IPCAMERA_INDEX	5
#define	MAX_IPCAMERA			4


int32_t create_params_table_file(const int8_t *xml_file, all_server_info *ptable);
int32_t read_params_table_file(const int8_t *xml_file, all_server_info *ptable);
int32_t reset_record_params(all_server_info *pinfo);
int32_t modify_params_table_file(const int8_t *xml_file, all_server_info *pold_table, all_server_info *pnew_table);

int32_t modify_room_info_only(const int8_t *xml_file, room_info *pold, room_info *pnew);
int32_t modify_config_only(const int8_t *xml_file, config_info *pold, config_info *pnew);
int32_t modify_server_info_only(const int8_t *xml_file, server_info *pold, server_info *pnew);
int32_t modify_usr_info_only(const int8_t *xml_file, user_info *pold, user_info *pnew);
int32_t modify_sys_info_only(const int8_t *xml_file, sys_info *pold, sys_info *pnew);
int32_t modify_heart_beat_info_only(const int8_t *xml_file, heart_beat_info *pold, heart_beat_info *pnew);
int32_t modify_ftp_info_only(const int8_t *xml_file, ftp_info *pold, ftp_info *pnew);
int32_t modify_pic_synt_info_only(const int8_t *xml_file, pic_synt_info *pold, pic_synt_info *pnew);
int32_t modify_camctrl_info_only(const int8_t *xml_file, camera_ctrl *pold, camera_ctrl *pnew);
int32_t modify_ipcamera_ip_only(Tracer_Info_t *tinfo);



#endif
