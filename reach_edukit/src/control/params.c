/*
 * =====================================================================================
 *
 *       Filename:  params.c
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
#include <unistd.h>
#include <errno.h>

#include "xml/xml_base.h"
#include "params.h"
#include "common.h"
#include "control.h"
#include "control_log.h"
#include "command_protocol.h"
#include "command_resolve.h"
#include "reach_os.h"

extern uint32_t g_spec_data;
extern server_set *gpser;


int32_t modify_int32_value(xmlNodePtr pnode, int32_t int32_old, int32_t int32_new, int8_t *key, int8_t *pbuf)
{
	xmlNodePtr node;
	
	/* 此不再进行参数有效性判断，应该在外部判断 */

	if(int32_old != int32_new){
		node = get_children_node(pnode, (const xmlChar *)key);
		if(node){
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			sprintf((char *)pbuf, "%d", int32_new);
			modify_node_value(node, (const xmlChar *)pbuf);
		}
	}

	return 0;
}

int32_t modify_uint32_value(xmlNodePtr pnode, uint32_t uint32_old, uint32_t uint32_new,
										int8_t *key, const int8_t *pbuf)
{
	xmlNodePtr node;
	
	/* 此不再进行参数有效性判断，应该在外部判断 */

	if(uint32_old != uint32_new){
		node = get_children_node(pnode, (const xmlChar *)key);
		r_memset((void *)pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", uint32_new);
		if(node){
			modify_node_value(node, (const xmlChar *)pbuf);
		}
		else{
			package_add_xml_leaf(pnode, (const xmlChar *)key, (const int8_t *)pbuf);
		}
	}

	return 0;
}

int32_t modify_uint16_value(xmlNodePtr pnode, uint16_t uint32_old, uint16_t uint32_new,
										int8_t *key, const int8_t *pbuf)
{
	xmlNodePtr node;
	
	/* 此不再进行参数有效性判断，应该在外部判断 */

	if(uint32_old != uint32_new){
		node = get_children_node(pnode, (const xmlChar *)key);
		if(node){
			r_memset((void *)pbuf, 0, XML_VALUE_MAX_LENGTH);
			sprintf((char *)pbuf, "%d", uint32_new);
			modify_node_value(node, (const xmlChar *)pbuf);
		}
	}

	return 0;
}


int32_t modify_int8_value(xmlNodePtr pnode, int8_t *int8_old, int8_t *int8_new,
										int8_t *key, const int8_t *pbuf)
{
	int32_t ret = -1;
	xmlNodePtr node;
	
	/* 此不再进行参数有效性判断，应该在外部判断 */

	if(r_strcmp((const int8_t *)int8_old, (const int8_t *)int8_new) != 0){
		node = get_children_node(pnode, (const xmlChar *)key);
		if(node){
			modify_node_value(node, (const xmlChar *)int8_new);
			ret = 0;
		}
		else{
			package_add_xml_leaf(pnode, (const xmlChar *)key, (const int8_t *)int8_new);
		}
	}

	return ret;
}

int32_t modify_addr_value(xmlNodePtr pnode, uint32_t uint32_old, uint32_t uint32_new,
										int8_t *key, const int8_t *pbuf)
{
	xmlNodePtr node;
	uint32_t value = 0;
	struct in_addr addr;
	
	/* 此不再进行参数有效性判断，应该在外部判断 */

	if(uint32_old != uint32_new){
		node = get_children_node(pnode, (const xmlChar *)key);
		if(node){
			if(ENC_INFO_SPE_VALUE == uint32_new){
				/* FIXME: 此处为特殊处理 */
				modify_node_value(node, (const xmlChar *)XML_NULL_VALUE);
			}
			else{
				value = uint32_new;
				r_memset((void *)pbuf, 0, XML_VALUE_MAX_LENGTH);
				r_memcpy(&addr, &value, 4);
				r_strcpy((int8_t *)pbuf, (const int8_t *)inet_ntoa(addr));
				modify_node_value(node, (const xmlChar *)pbuf);
			}
		}
	}

	return 0;
}

static int32_t add_user_info(xmlNodePtr pnode, user_info *pinfo)
{
	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_user_info]--- failed, params is NULL!\n");
		return -1;
	}

	xml_add_new_child(pnode, NULL, MSG_USER_KEY, pinfo->username);
	xml_add_new_child(pnode, NULL, MSG_PASSWORD_KEY, pinfo->password);
	xml_add_new_child(pnode, NULL, MSG_GUEST_KEY, pinfo->guest_name);
	xml_add_new_child(pnode, NULL, MSG_GUEST_PASSWD_KEY, pinfo->guest_passwd);
	
	return 0;
}

static int32_t add_camctrl_info(xmlNodePtr pnode, camera_ctrl *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_camctrl_info]--- failed, params is NULL!\n");
		return -1;
	}

	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->video0_addr);
	xml_add_new_child(pnode, NULL, MSG_VIDEO_0_ADDR_KEY, (const xmlChar *)buf);

	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->video1_addr);
	xml_add_new_child(pnode, NULL, MSG_VIDEO_1_ADDR_KEY, (const xmlChar *)buf);

	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->video2_addr);
	xml_add_new_child(pnode, NULL, MSG_VIDEO_2_ADDR_KEY, (const xmlChar *)buf);
	
	return 0;
}


static int32_t add_ftp_info(xmlNodePtr pnode, ftp_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_ftp_info]--- failed, params is NULL!\n");
		return -1;
	}

	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->Mode);
	xml_add_new_child(pnode, NULL, MSG_FTPMODE_KEY,  (const xmlChar *)buf);

	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->MCFTPPort);
	xml_add_new_child(pnode, NULL, MSG_FTPPORT_KEY,  (const xmlChar *)buf);

	xml_add_new_child(pnode, NULL, MSG_FTP_SERVERIP_KEY, pinfo->MCFTPAddr);
	xml_add_new_child(pnode, NULL, MSG_FTP_USER_KEY, pinfo->MCFTPUserName);
	xml_add_new_child(pnode, NULL, MSG_FTP_PASSWORD_KEY, pinfo->MCFTPPassword);
	xml_add_new_child(pnode, NULL, MSG_FTP_UPLOAD_PATH_KEY, pinfo->MCFTPPath);


	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->THRFTPPort);
	xml_add_new_child(pnode, NULL, MSG_THRFTPPORT_KEY,  (const xmlChar *)buf);

	xml_add_new_child(pnode, NULL, MSG_THRFTP_SERVERIP_KEY, pinfo->THRFTPAddr);
	xml_add_new_child(pnode, NULL, MSG_THRFTP_USER_KEY, pinfo->THRFTPUserName);
	xml_add_new_child(pnode, NULL, MSG_THRFTP_PASSWORD_KEY, pinfo->THRFTPPassword);
	xml_add_new_child(pnode, NULL, MSG_THRFTP_UPLOAD_PATH_KEY, pinfo->THRFTPPath);
	
	return 0;
}


static int32_t add_config_info(xmlNodePtr pnode, config_info *pinfo)
{
	struct in_addr addr;

	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_config_info]--- failed, params is NULL!\n");
		return -1;
	}

	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->language);
	xml_add_new_child(pnode, NULL, MSG_LANGUAGE_KEY,  (const xmlChar *)buf);

	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->Laniptype);
	xml_add_new_child(pnode, NULL, MSG_LANIPTYPE_KEY, (const xmlChar *)buf);

	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->Waniptype);
	xml_add_new_child(pnode, NULL, MSG_WANIPTYPE_KEY, (const xmlChar *)buf);

	r_memcpy(&addr, &pinfo->media_addr, 4);
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	r_strcpy(buf, (const int8_t *)inet_ntoa(addr));
	xml_add_new_child(pnode, NULL, MSG_MEDIA_ADDR_KEY, (const xmlChar *)buf);

	r_memcpy(&addr, &pinfo->manager_addr, 4);
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	r_strcpy(buf, (const int8_t *)inet_ntoa(addr));
	xml_add_new_child(pnode, NULL, MSG_MANAGER_ADDR_KEY,(const xmlChar *) buf);
	
	return 0;
}

static int32_t add_heart_beat_info(xmlNodePtr pnode, heart_beat_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_heart_beat_info]--- failed, params is NULL!\n");
		return -1;
	}

	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->post_time);
	xml_add_new_child(pnode, NULL, MSG_POST_TIME_KEY, (const xmlChar *)buf);
	
	xml_add_new_child(pnode, NULL, MSG_POST_URL_KEY, (const xmlChar *)pinfo->post_url);

	xml_add_new_child(pnode, NULL, MSG_TIME_SERIP_KEY, (const xmlChar *)pinfo->time_serip);
	
	return 0;
}

static int32_t add_sys_info(xmlNodePtr pnode, sys_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	struct in_addr addr;

	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_sys_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 时间同步服务器 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	r_memcpy(&addr, &pinfo->TimeServerAddr, 4);
	r_strcpy(buf, (const int8_t *)inet_ntoa(addr));
	xml_add_new_child(pnode, NULL, MSG_TIMESERVER_ADDR_KEY, (const xmlChar *)buf);

	/* 磁盘空间 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->DiskMaxSpace);
	xml_add_new_child(pnode, NULL, MSG_DISK_MAXSPACE_KEY, (const xmlChar *)buf);

	/* 磁盘有效空间 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->DiskAvailableSpace);
	xml_add_new_child(pnode, NULL, MSG_DISK_AVALIABLESPACE_KEY, (const xmlChar *)buf);

	return 0;
}

static int32_t add_server_info(xmlNodePtr pnode, server_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	struct in_addr addr;

	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_server_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 服务器类型 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%s", pinfo->ServerType);
	xml_add_new_child(pnode, NULL, MSG_SERVER_TYPE_KEY, (const xmlChar *)buf);

	/* 服务器序列号 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%s", pinfo->ServerSeries);
	xml_add_new_child(pnode, NULL, MSG_SERVER_SERIES_KEY, (const xmlChar *)buf);
	
#if 1
	/* 服务器版本 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%s", pinfo->ServerVersion);
	xml_add_new_child(pnode, NULL, MSG_SERVER_VERSION_INFO_KEY, (const xmlChar *)buf);
#endif

	/* 最大会议室数量 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->MaxRoom);
	xml_add_new_child(pnode, NULL, MSG_MAXROOM_KEY, (const xmlChar *)buf);

	/* 会议室中最大编码器数量 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->MaxCodecNumInRoom);
	xml_add_new_child(pnode, NULL, MSG_MAXCODENUM_INROOM_KEY, (const xmlChar *)buf);

	/* 内网IP */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	r_memcpy(&addr, &pinfo->LanAddr, 4);
	r_strcpy(buf, (const int8_t *)inet_ntoa(addr));
	xml_add_new_child(pnode, NULL, MSG_LANADDR_KEY, (const xmlChar *)buf);

	/* 内网网关 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	r_memcpy(&addr, &pinfo->LanGateWay, 4);
	r_strcpy(buf, (const int8_t *)inet_ntoa(addr));
	xml_add_new_child(pnode, NULL, MSG_LANGATEWAY_KEY, (const xmlChar *)buf);

	/* 内网掩码 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	r_memcpy(&addr, &pinfo->LanNetmask, 4);
	r_strcpy(buf, (const int8_t *)inet_ntoa(addr));
	xml_add_new_child(pnode, NULL, MSG_LANNETMASK_KEY, (const xmlChar *)buf);

	/* 内网物理地址 */
	xml_add_new_child(pnode, NULL, MSG_LANMAC_KEY, (const xmlChar *)pinfo->LanMac);


	/* 外网IP */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	r_memcpy(&addr, &pinfo->WanAddr, 4);
	r_strcpy(buf, (const int8_t *)inet_ntoa(addr));
	xml_add_new_child(pnode, NULL, MSG_WANADDR_KEY, (const xmlChar *)buf);

	/* 外网网关 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	r_memcpy(&addr, &pinfo->WanGateWay, 4);
	r_strcpy(buf, (const int8_t *)inet_ntoa(addr));
	xml_add_new_child(pnode, NULL, MSG_WANGATEWAY_KEY, (const xmlChar *)buf);

	/* 外网掩码 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	r_memcpy(&addr, &pinfo->WanNetmask, 4);
	r_strcpy(buf, (const int8_t *)inet_ntoa(addr));
	xml_add_new_child(pnode, NULL, MSG_WANNETMASK_KEY, (const xmlChar *)buf);

	/* 外网物理地址 */
	xml_add_new_child(pnode, NULL, MSG_WANMAC_KEY, (const xmlChar *)pinfo->WanMac);


	return 0;
}

static int32_t add_audio_info(xmlNodePtr pnode, audio_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_audio_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 输入模式 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->InputMode);
	xml_add_new_child(pnode, NULL, MSG_INPUTMODE_KEY, (const xmlChar *)buf);

	/* 采样率 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->SampleRate);
	xml_add_new_child(pnode, NULL, MSG_SAMPLERATE_KEY, (const xmlChar *)buf);

	/* 比特率 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->Bitrate);
	xml_add_new_child(pnode, NULL, MSG_BITRATE_KEY, (const xmlChar *)buf);

	/* 左声道音量 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->Lvolume);
	xml_add_new_child(pnode, NULL, MSG_LVOLUME_KEY, (const xmlChar *)buf);

	/* 右声道音量 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->Rvolume);
	xml_add_new_child(pnode, NULL, MSG_RVOLUME_KEY, (const xmlChar *)buf);

	/* InputIndex */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%s", pinfo->InputIndex);
	xml_add_new_child(pnode, NULL, MSG_INPUTINDEX_KEY, (const xmlChar *)buf);
		
	return 0;
}

static int32_t add_quality_info(xmlNodePtr pnode, quality_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_quality_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 是否启用此质量信息 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->enable);
	xml_add_new_child(pnode, NULL, CONFIG_PARAMS_QUALITY_ENABLE_KEY, (const xmlChar *)buf);

	/* 码流类型 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->RateType);
	xml_add_new_child(pnode, NULL, MSG_ENC_RATETYPE_KEY, (const xmlChar *)buf);

	/* 比特率 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->EncBitrate);
	xml_add_new_child(pnode, NULL, MSG_ENC_BITRATE_KEY, (const xmlChar *)buf);

	/* 编码宽 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->EncWidth);
	xml_add_new_child(pnode, NULL, MSG_ENC_WIDTH_KEY, (const xmlChar *)buf);

	/* 编码高 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->EncHeight);
	xml_add_new_child(pnode, NULL, MSG_ENC_HEIGHT_KEY, (const xmlChar *)buf);

	/* 编码帧率 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->EncFrameRate);
	xml_add_new_child(pnode, NULL, MSG_ENC_FRAMERATE_KEY, (const xmlChar *)buf);

	return 0;
}

static int32_t add_enc_info(xmlNodePtr pnode, enc_info *pinfo)
{
	xmlNodePtr pqua_node_hd;
	xmlNodePtr pqua_node_sd;
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
//	struct in_addr addr;
	
	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_enc_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 索引值 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->ID);
	xml_add_new_child(pnode, NULL, MSG_ENC_ID_KEY, (const xmlChar *)buf);
#if 0
	/* 编码器IP */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	r_memcpy(&addr, &pinfo->EncIP, 4);
	r_strcpy(buf, (const int8_t *)inet_ntoa(addr));
	xml_add_new_child(pnode, NULL, MSG_ENC_IP_KEY, (const xmlChar *)buf);
#endif
	/* 编码器IP */
	xml_add_new_child(pnode, NULL, MSG_ENC_IP_KEY, (const xmlChar *)pinfo->EncIP);

	/* 状态 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->Status);
	xml_add_new_child(pnode, NULL, MSG_ENC_STATUS_KEY, (const xmlChar *)buf);

	/* 质量参数 */
	pqua_node_hd = xmlNewNode(NULL, CONFIG_PARAMS_QUALITY_INFO_KEY);
	xmlAddChild(pnode, pqua_node_hd);
	add_quality_info(pqua_node_hd, &pinfo->HD_QuaInfo);

	pqua_node_sd = xmlNewNode(NULL, CONFIG_PARAMS_QUALITY_INFO_KEY);
	xmlAddChild(pnode, pqua_node_sd);
	add_quality_info(pqua_node_sd, &pinfo->SD_QuaInfo);

	return 0;
}

static int32_t add_pic_synt_info(xmlNodePtr pnode, pic_synt_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_pic_synt_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 源数量 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->SourceNum);
	xml_add_new_child(pnode, NULL, MSG_PICTURE_SYNT_SOURCENUM_KEY, (const xmlChar *)buf);

	/* 合成分辨率 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->Resolution);
	xml_add_new_child(pnode, NULL, MSG_PICTURE_SYNT_RESOLUTION_KEY, (const xmlChar *)buf);

	/* 合成布局 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%s", pinfo->Model);
	xml_add_new_child(pnode, NULL, MSG_PICTURE_SYNT_MODEL_KEY, (const xmlChar *)buf);

	/* 二画面默认的布局ID */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->LayoutDef_2);
	xml_add_new_child(pnode, NULL, MSG_LAYOUT_DEF_2_KEY, (const xmlChar *)buf);

	/* 三画面默认的布局ID */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->LayoutDef_3);
	xml_add_new_child(pnode, NULL, MSG_LAYOUT_DEF_3_KEY, (const xmlChar *)buf);
	

	return 0;
}


static int32_t add_room_info(xmlNodePtr pnode, room_info *pinfo)
{
	xmlNodePtr paudio_node;
	xmlNodePtr ppic_synt_node;
	xmlNodePtr penc_node[RECORD_ROOM_MAX_ENC_NUM];
	int32_t index = 0;
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[add_room_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 会议室ID */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->RoomID);
	xml_add_new_child(pnode, NULL, MSG_ROOMID_KEY, (const xmlChar *)buf);

	/* 会议室名称 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%s", pinfo->RoomName);
	xml_add_new_child(pnode, NULL, MSG_ROOMNAME_KEY, (const xmlChar *)buf);

	/* 连接状态 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->ConnStatus);
	xml_add_new_child(pnode, NULL, MSG_CONNECT_STATUS_KEY, (const xmlChar *)buf);

	/* 录制最大时长 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->RecordMaxTime);
	xml_add_new_child(pnode, NULL, MSG_RECORD_MAXTIME_KEY, (const xmlChar *)buf);

	/* 录制状态 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->RecStatus);
	xml_add_new_child(pnode, NULL, MSG_RECORD_STATUS_KEY, (const xmlChar *)buf);

	/* 评教开启状态 */
	r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)buf, "%d", pinfo->IfMark);
	xml_add_new_child(pnode, NULL, MSG_ROOM_IFMARK_KEY, (const xmlChar *)buf);

	/* 录制文件名 */
	xml_add_new_child(pnode, NULL, MSG_ROOM_RECNAME_KEY, (const xmlChar *)pinfo->RecName);

	/* 画面合成信息 */
	ppic_synt_node = xmlNewNode(NULL, CONFIG_PARAMS_PICTURE_SYNT_INFO_KEY);
	xmlAddChild(pnode, ppic_synt_node);
	add_pic_synt_info(ppic_synt_node, &pinfo->PicSyntInfo);

	/* 音频信息 */
	paudio_node = xmlNewNode(NULL, CONFIG_PARAMS_AUDIO_INFO_KEY);
	xmlAddChild(pnode, paudio_node);
	add_audio_info(paudio_node, &pinfo->AudioInfo);

	/* 编码器信息 */
	for(index = 0; index < RECORD_ROOM_MAX_ENC_NUM; index++){
		penc_node[index] = xmlNewNode(NULL, CONFIG_PARAMS_ENC_INFO_KEY);
		xmlAddChild(pnode, penc_node[index]);
		add_enc_info(penc_node[index], &pinfo->EncInfo[index]);
	}

	return 0;
}

static int32_t read_user_info(xmlDocPtr pdoc, xmlNodePtr pnode, user_info *pinfo)
{
	xmlNodePtr node;
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_user_info]--- failed, params is NULL!\n");
		return -1;
	}

	node = get_children_node(pnode, MSG_USER_KEY);
	if(node){
		r_memset(pinfo->username, 0, XML_USER_NAME_MAX_LENGTH);
		get_current_node_value((char *)pinfo->username, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	node = get_children_node(pnode, MSG_PASSWORD_KEY);
	if(node){
		r_memset(pinfo->password, 0, XML_USER_PASSWORD_MAX_LENGTH);
		get_current_node_value((char *)pinfo->password, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	node = get_children_node(pnode, MSG_GUEST_KEY);
	if(node){
		r_memset(pinfo->guest_name, 0, XML_USER_NAME_MAX_LENGTH);
		get_current_node_value((char *)pinfo->guest_name, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	node = get_children_node(pnode, MSG_GUEST_PASSWD_KEY);
	if(node){
		r_memset(pinfo->guest_passwd, 0, XML_USER_PASSWORD_MAX_LENGTH);
		get_current_node_value((char *)pinfo->guest_passwd, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	return 0;
}

static int32_t read_camctrl_info(xmlDocPtr pdoc, xmlNodePtr pnode, camera_ctrl *pinfo)
{
	xmlNodePtr node;
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_camctrl_info]--- failed, params is NULL!\n");
		return -1;
	}

	node = get_children_node(pnode, MSG_VIDEO_0_ADDR_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->video0_addr = atoi((char *)buf);
	}
	else{
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)buf, "%d", pinfo->video0_addr);
		xml_add_new_child(pnode, NULL, MSG_VIDEO_0_ADDR_KEY, (const xmlChar *)buf);
	}

	node = get_children_node(pnode, MSG_VIDEO_1_ADDR_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->video1_addr = atoi((char *)buf);
	}
	else{
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)buf, "%d", pinfo->video1_addr);
		xml_add_new_child(pnode, NULL, MSG_VIDEO_1_ADDR_KEY, (const xmlChar *)buf);
	}

	node = get_children_node(pnode, MSG_VIDEO_2_ADDR_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->video2_addr = atoi((char *)buf);
	}
	else{
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)buf, "%d", pinfo->video2_addr);
		xml_add_new_child(pnode, NULL, MSG_VIDEO_2_ADDR_KEY, (const xmlChar *)buf);
	}

	return 0;
}


static int32_t read_ftp_info(xmlDocPtr pdoc, xmlNodePtr pnode, ftp_info *pinfo)
{
	xmlNodePtr node;
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_ftp_info]--- failed, params is NULL!\n");
		return -1;
	}

	node = get_children_node(pnode, MSG_FTPMODE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->Mode = atoi((char *)buf);
	}
	else{
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)buf, "%d", pinfo->Mode);
		xml_add_new_child(pnode, NULL, MSG_FTPMODE_KEY, (const xmlChar *)buf);
	}

	node = get_children_node(pnode, MSG_FTPPORT_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->MCFTPPort = atoi((char *)buf);
	}
	
	node = get_children_node(pnode, MSG_FTP_SERVERIP_KEY);
	if(node){
		r_memset(pinfo->MCFTPAddr, 0, 24);
		get_current_node_value((char *)pinfo->MCFTPAddr, 24, pdoc, node);
	}

	node = get_children_node(pnode, MSG_FTP_USER_KEY);
	if(node){
		r_memset(pinfo->MCFTPUserName, 0, FTP_MAX_USERNAME_LENGTH);
		get_current_node_value((char *)pinfo->MCFTPUserName, FTP_MAX_USERNAME_LENGTH, pdoc, node);
	}

	node = get_children_node(pnode, MSG_FTP_PASSWORD_KEY);
	if(node){
		r_memset(pinfo->MCFTPPassword, 0, FTP_MAX_PASSWD_LENGTH);
		get_current_node_value((char *)pinfo->MCFTPPassword, FTP_MAX_PASSWD_LENGTH, pdoc, node);
	}

	node = get_children_node(pnode, MSG_FTP_UPLOAD_PATH_KEY);
	if(node){
		r_memset(pinfo->MCFTPPath, 0, FTP_MAX_FTPPATH_LENGTH);
		get_current_node_value((char *)pinfo->MCFTPPath, FTP_MAX_FTPPATH_LENGTH, pdoc, node);
	}

	

	node = get_children_node(pnode, MSG_THRFTPPORT_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->THRFTPPort = atoi((char *)buf);
	}
	else{
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)buf, "%d", pinfo->THRFTPPort);
		xml_add_new_child(pnode, NULL, MSG_THRFTPPORT_KEY, (const xmlChar *)buf);
	}
	
	node = get_children_node(pnode, MSG_THRFTP_SERVERIP_KEY);
	if(node){
		r_memset(pinfo->THRFTPAddr, 0, 24);
		get_current_node_value((char *)pinfo->THRFTPAddr, 24, pdoc, node);
	}
	else{
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)buf, "%s", pinfo->THRFTPAddr);
		xml_add_new_child(pnode, NULL, MSG_THRFTP_SERVERIP_KEY, (const xmlChar *)buf);
	}

	node = get_children_node(pnode, MSG_THRFTP_USER_KEY);
	if(node){
		r_memset(pinfo->THRFTPUserName, 0, FTP_MAX_USERNAME_LENGTH);
		get_current_node_value((char *)pinfo->THRFTPUserName, FTP_MAX_USERNAME_LENGTH, pdoc, node);
	}
	else{
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)buf, "%s", pinfo->THRFTPUserName);
		xml_add_new_child(pnode, NULL, MSG_THRFTP_USER_KEY, (const xmlChar *)buf);
	}

	node = get_children_node(pnode, MSG_THRFTP_PASSWORD_KEY);
	if(node){
		r_memset(pinfo->THRFTPPassword, 0, FTP_MAX_PASSWD_LENGTH);
		get_current_node_value((char *)pinfo->THRFTPPassword, FTP_MAX_PASSWD_LENGTH, pdoc, node);
	}
	else{
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)buf, "%s", pinfo->THRFTPPassword);
		xml_add_new_child(pnode, NULL, MSG_THRFTP_PASSWORD_KEY, (const xmlChar *)buf);
	}

	node = get_children_node(pnode, MSG_THRFTP_UPLOAD_PATH_KEY);
	if(node){
		r_memset(pinfo->THRFTPPath, 0, FTP_MAX_FTPPATH_LENGTH);
		get_current_node_value((char *)pinfo->THRFTPPath, FTP_MAX_FTPPATH_LENGTH, pdoc, node);
	}
	else{
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)buf, "%s", pinfo->THRFTPPath);
		xml_add_new_child(pnode, NULL, MSG_THRFTP_UPLOAD_PATH_KEY, (const xmlChar *)buf);
	}
	
	return 0;
}



static int32_t read_config_info(xmlDocPtr pdoc, xmlNodePtr pnode, config_info *pinfo)
{
	xmlNodePtr node;
	struct in_addr addr;
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_config_info]--- failed, params is NULL!\n");
		return -1;
	}

	node = get_children_node(pnode, MSG_LANGUAGE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->language = atoi((char *)buf);
	}
	
	node = get_children_node(pnode, MSG_LANIPTYPE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->Laniptype = atoi((char *)buf);
	}

	node = get_children_node(pnode, MSG_WANIPTYPE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->Waniptype = atoi((char *)buf);
	}

#if 1
	node = get_children_node(pnode, MSG_MEDIA_ADDR_KEY);
	if(node){
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		inet_aton((const char *)buf, &addr);
		r_memcpy(&pinfo->media_addr, &addr, 4);
	}

	node = get_children_node(pnode, MSG_MANAGER_ADDR_KEY);
	if(node){
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		inet_aton((const char *)buf, &addr);
		r_memcpy(&pinfo->manager_addr, &addr, 4);
	}
#endif
	
	return 0;
}

static int32_t read_heart_beat_info(xmlDocPtr pdoc, xmlNodePtr pnode, heart_beat_info *pinfo)
{
	xmlNodePtr node;
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_heart_beat_info]--- failed, params is NULL!\n");
		return -1;
	}

	node = get_children_node(pnode, MSG_POST_TIME_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->post_time = atoi((const char *)buf);
	}

	node = get_children_node(pnode, MSG_POST_URL_KEY);
	if(node){
		r_memset(pinfo->post_url, 0, HTTP_SERVER_URL_MAX_LEN);
		get_current_node_value((char *)pinfo->post_url, HTTP_SERVER_URL_MAX_LEN, pdoc, node);
	}

	node = get_children_node(pnode, MSG_TIME_SERIP_KEY);
	if(node){
		r_memset(pinfo->time_serip, 0, 64);
		get_current_node_value((char *)pinfo->time_serip, 64, pdoc, node);
	}
	else{
		xml_add_new_child(pnode, NULL, MSG_TIME_SERIP_KEY, (const xmlChar *)pinfo->time_serip);
	}

	return 0;
}



static int32_t read_sys_info(xmlDocPtr pdoc, xmlNodePtr pnode, sys_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	struct in_addr addr;
	xmlNodePtr node;
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_sys_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 时间同步服务器 */
	node = get_children_node(pnode, MSG_TIMESERVER_ADDR_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		inet_aton((const char *)buf, &addr);
		r_memcpy(&pinfo->TimeServerAddr, &addr, 4);
	}

	/* 磁盘空间 */
	node = get_children_node(pnode, MSG_DISK_MAXSPACE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->DiskMaxSpace = atoi((const char *)buf);
	}

	/* 磁盘有效空间 */
	node = get_children_node(pnode, MSG_DISK_AVALIABLESPACE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->DiskAvailableSpace = atoi((const char *)buf);
	}

	return 0;
}

static int32_t read_server_info(xmlDocPtr pdoc, xmlNodePtr pnode, server_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	struct in_addr addr;
	xmlNodePtr node;
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_server_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 服务器类型 */
	node = get_children_node(pnode, MSG_SERVER_TYPE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		r_memset(pinfo->ServerType, 0, CONTROL_VERSION_LEN);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		r_strcpy((int8_t *)pinfo->ServerType, (const int8_t *)buf);
	}

	/* 服务器序列号 */
	node = get_children_node(pnode, MSG_SERVER_SERIES_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		r_memset(pinfo->ServerSeries, 0, CONTROL_VERSION_LEN);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		r_strcpy((int8_t *)pinfo->ServerSeries, (const int8_t *)buf);
	}
#if 0
	/* 服务器版本 */
	node = get_children_node(pnode, MSG_SERVER_VERSION_INFO_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		r_strcpy((int8_t *)pinfo->ServerVersion, (const int8_t *)buf);
	}
#endif
	/* 最大会议室数量 */
	node = get_children_node(pnode, MSG_MAXROOM_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->MaxRoom = atoi((const char *)buf);
	}

	/* TODO:使用KEY的值 */
//	pinfo->MaxRoom = CONTROL_DEFAULT_ROOM_COUNT;
	pinfo->MaxRoom = g_spec_data;

	/* 会议室中最大编码器数量 */
	node = get_children_node(pnode, MSG_MAXCODENUM_INROOM_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->MaxCodecNumInRoom = atoi((const char *)buf);
	}

	/* 内网IP */
	node = get_children_node(pnode, MSG_LANADDR_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		inet_aton((const char *)buf, &addr);
		r_memcpy(&pinfo->LanAddr, &addr, 4);
	}

	/* 内网网关 */
	node = get_children_node(pnode, MSG_LANGATEWAY_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		inet_aton((const char *)buf, &addr);
		r_memcpy(&pinfo->LanGateWay, &addr, 4);
	}

	/* 内网掩码 */
	node = get_children_node(pnode, MSG_LANNETMASK_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		inet_aton((const char *)buf, &addr);
		r_memcpy(&pinfo->LanNetmask, &addr, 4);
	}

	/* 内网物理地址 */
	node = get_children_node(pnode, MSG_LANMAC_KEY);
	if(node){
		r_memset(pinfo->LanMac, 0, 24);
		get_current_node_value((char *)pinfo->LanMac, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	/* 外网IP */
	node = get_children_node(pnode, MSG_WANADDR_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		inet_aton((const char *)buf, &addr);
		r_memcpy(&pinfo->WanAddr, &addr, 4);
	}

	/* 外网网关 */
	node = get_children_node(pnode, MSG_WANGATEWAY_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		inet_aton((const char *)buf, &addr);
		r_memcpy(&pinfo->WanGateWay, &addr, 4);
	}

	/* 外网掩码 */
	node = get_children_node(pnode, MSG_WANNETMASK_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		inet_aton((const char *)buf, &addr);
		r_memcpy(&pinfo->WanNetmask, &addr, 4);
	}

	/* 外网物理地址 */
	node = get_children_node(pnode, MSG_WANMAC_KEY);
	if(node){
		r_memset(pinfo->WanMac, 0, 24);
		get_current_node_value((char *)pinfo->WanMac, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	return 0;
}

static int32_t read_audio_info(xmlDocPtr pdoc, xmlNodePtr pnode, audio_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	xmlNodePtr node;
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_sys_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 输入模式 */
	node = get_children_node(pnode, MSG_INPUTMODE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->InputMode = atoi((const char *)buf);
	}


	/* 采样率 */
	node = get_children_node(pnode, MSG_SAMPLERATE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->SampleRate = atoi((const char *)buf);
	}

	/* 比特率 */
	node = get_children_node(pnode, MSG_BITRATE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->Bitrate = atoi((const char *)buf);
	}

	/* 左声道音量 */
	node = get_children_node(pnode, MSG_LVOLUME_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->Lvolume = atoi((const char *)buf);
	}

	/* 右声道音量 */
	node = get_children_node(pnode, MSG_RVOLUME_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->Rvolume = atoi((const char *)buf);
	}

	/* InputIndex */
	node = get_children_node(pnode, MSG_INPUTINDEX_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		r_memcpy(pinfo->InputIndex, (const void *)buf, 4);
	}

	return 0;
}

static int32_t read_pic_synt_info(xmlDocPtr pdoc, xmlNodePtr pnode, pic_synt_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	xmlNodePtr node;
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_pic_synt_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 源数量 */
	node = get_children_node(pnode, MSG_PICTURE_SYNT_SOURCENUM_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->SourceNum = atoi((const char *)buf);
	}


	/* 合成分辨率 */
	node = get_children_node(pnode, MSG_PICTURE_SYNT_RESOLUTION_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->Resolution = atoi((const char *)buf);
	}

	/* 合成布局 */
	node = get_children_node(pnode, MSG_PICTURE_SYNT_MODEL_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		r_memcpy(pinfo->Model, (const void *)buf, 4);
	}

	/* 二画面默认的布局ID */
	node = get_children_node(pnode, MSG_LAYOUT_DEF_2_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->LayoutDef_2 = atoi((const char *)buf);
	}

	/* 三画面默认的布局ID */
	node = get_children_node(pnode, MSG_LAYOUT_DEF_3_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->LayoutDef_3 = atoi((const char *)buf);
	}

	return 0;
}



static int32_t read_quality_info(xmlDocPtr pdoc, xmlNodePtr pnode, quality_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	xmlNodePtr node;
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_quality_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 是否启用此质量信息 */
	node = get_children_node(pnode, CONFIG_PARAMS_QUALITY_ENABLE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->enable = atoi((const char *)buf);
	}

	/* 码流类型 */
	node = get_children_node(pnode, MSG_ENC_RATETYPE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->RateType = atoi((const char *)buf);
	}

	/* 比特率 */
	node = get_children_node(pnode, MSG_ENC_BITRATE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->EncBitrate = atoi((const char *)buf);
	}

	/* 编码宽 */
	node = get_children_node(pnode, MSG_ENC_WIDTH_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->EncWidth = atoi((const char *)buf);
	}

	/* 编码高 */
	node = get_children_node(pnode, MSG_ENC_HEIGHT_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->EncHeight = atoi((const char *)buf);
	}

	/* 编码帧率 */
	node = get_children_node(pnode, MSG_ENC_FRAMERATE_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->EncFrameRate = atoi((const char *)buf);
	}

	return 0;
}

static int32_t read_enc_info(xmlDocPtr pdoc, xmlNodePtr pnode, enc_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
//	struct in_addr addr;
	xmlNodePtr node;
	xmlNodePtr pqua_node;
	int32_t rate_type = 0;
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_enc_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 索引值 */
	node = get_children_node(pnode, MSG_ENC_ID_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->ID = atoi((const char *)buf);
	}
	
#if 0
	/* 编码器IP */
	node = get_children_node(pnode, MSG_ENC_IP_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		zlog_debug(OPELOG, "read: buf = %s, r_strlen(buf) = %d\n", buf, r_strlen(buf));
		if(0==r_strlen(buf) &&0){
			pinfo->EncIP = ENC_INFO_SPE_VALUE;
		}
		else{
			inet_aton((char *)buf, &addr);
			zlog_debug(OPELOG, "read: addr = %u\n", addr);
			r_memcpy(&pinfo->EncIP, &addr, 4);
		}
	}
	else{
		pinfo->EncIP = ENC_INFO_SPE_VALUE;
	}
	zlog_debug(OPELOG, "read: pinfo->EncIP = %u\n\n", pinfo->EncIP);
#endif

	/* 编码器IP */
	r_memset(pinfo->EncIP, 0, IPINFO_MAX_LENGTH);
	node = get_children_node(pnode, MSG_ENC_IP_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		if(r_strlen(buf)>0){
			r_strncpy(pinfo->EncIP, buf, IPINFO_MAX_LENGTH);
		}
	}

	/* 状态 */
	node = get_children_node(pnode, MSG_ENC_STATUS_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->Status = atoi((const char *)buf);
	}

	/* 质量参数 */
	pqua_node = get_children_node(pnode, CONFIG_PARAMS_QUALITY_INFO_KEY);
	if(pqua_node){
		node = get_children_node(pqua_node, MSG_ENC_RATETYPE_KEY);
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		rate_type = atoi((const char *)buf);

		if(rate_type == 0){ /* 高码流 */
			read_quality_info(pdoc, pqua_node, &pinfo->HD_QuaInfo);
		}
		else{ /* 低码流 */
			read_quality_info(pdoc, pqua_node, &pinfo->SD_QuaInfo);
		}
	}

	pqua_node = find_next_node(pqua_node, CONFIG_PARAMS_QUALITY_INFO_KEY);
	if(pqua_node){
		node = get_children_node(pqua_node, MSG_ENC_RATETYPE_KEY);
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		rate_type = atoi((const char *)buf);

		if(rate_type == 0){ /* 高码流 */
			read_quality_info(pdoc, pqua_node, &pinfo->HD_QuaInfo);
		}
		else{ /* 低码流 */
			read_quality_info(pdoc, pqua_node, &pinfo->SD_QuaInfo);
		}
	}
	
	return 0;
}

static int32_t read_room_info(xmlDocPtr pdoc, xmlNodePtr pnode, room_info *pinfo)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	xmlNodePtr node;
	xmlNodePtr paudio_node;
	xmlNodePtr ppic_synt_node;
	xmlNodePtr penc_node[RECORD_ROOM_MAX_ENC_NUM];
	int32_t index = 0;
	
	if(NULL == pdoc || NULL == pinfo || NULL == pnode){
		zlog_error(DBGLOG, "---[read_room_info]--- failed, params is NULL!\n");
		return -1;
	}

	/* 会议室ID */
	node = get_children_node(pnode, MSG_ROOMID_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->RoomID = atoi((const char *)buf);
	}

	/* 会议室名称 */
	node = get_children_node(pnode, MSG_ROOMNAME_KEY);
	if(node){
		r_memset((char *)pinfo->RoomName,0,XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)pinfo->RoomName, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	/* 连接状态 */
	node = get_children_node(pnode, MSG_CONNECT_STATUS_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->ConnStatus = atoi((const char *)buf);
	}

	/* 录制最大时长 */
	node = get_children_node(pnode, MSG_RECORD_MAXTIME_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->RecordMaxTime = atoi((const char *)buf);
	}

	/* 录制状态 */
	node = get_children_node(pnode, MSG_RECORD_STATUS_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->RecStatus = atoi((const char *)buf);
	}

	/* 评教开启状态 */
	node = get_children_node(pnode, MSG_ROOM_IFMARK_KEY);
	if(node){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, pdoc, node);
		pinfo->IfMark = atoi((const char *)buf);
	}

	/* 录制文件名 */
	node = get_children_node(pnode, MSG_ROOM_RECNAME_KEY);
	if(node){
		get_current_node_value((char *)pinfo->RecName, XML_VALUE_MAX_LENGTH, pdoc, node);
	}

	/* 合成画面信息 */
	ppic_synt_node = get_children_node(pnode, CONFIG_PARAMS_PICTURE_SYNT_INFO_KEY);
	read_pic_synt_info(pdoc, ppic_synt_node, &pinfo->PicSyntInfo);

	/* 音频信息 */
	paudio_node = get_children_node(pnode, CONFIG_PARAMS_AUDIO_INFO_KEY);
	read_audio_info(pdoc, paudio_node, &pinfo->AudioInfo);

	/* 编码器信息 */
	for(index = 0; index < RECORD_ROOM_MAX_ENC_NUM; index++){
		if(index == 0)
			penc_node[index] = get_children_node(pnode, CONFIG_PARAMS_ENC_INFO_KEY);
		else
			penc_node[index] = find_next_node(penc_node[index-1], CONFIG_PARAMS_ENC_INFO_KEY);
		read_enc_info(pdoc, penc_node[index], &pinfo->EncInfo[index]);
	}

	return 0;
}

static int32_t modify_ftp_info(xmlNodePtr pnode, ftp_info *pold, ftp_info *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pnode) {
		zlog_error(DBGLOG, "modify_ftp_info error, params is NULL!\n");
		return -1;
	}

	modify_uint16_value(pnode, pold->Mode, pnew->Mode, (int8_t *)MSG_FTPMODE_KEY, buf);
	modify_uint16_value(pnode, pold->MCFTPPort, pnew->MCFTPPort, (int8_t *)MSG_FTPPORT_KEY, buf);
	modify_int8_value(pnode, (int8_t *)pold->MCFTPAddr, (int8_t *)pnew->MCFTPAddr, (int8_t *)MSG_FTP_SERVERIP_KEY, buf);
	modify_int8_value(pnode, (int8_t *)pold->MCFTPUserName, (int8_t *)pnew->MCFTPUserName, (int8_t *)MSG_FTP_USER_KEY, buf);
	modify_int8_value(pnode, (int8_t *)pold->MCFTPPassword, (int8_t *)pnew->MCFTPPassword, (int8_t *)MSG_FTP_PASSWORD_KEY, buf);
	modify_int8_value(pnode, (int8_t *)pold->MCFTPPath, (int8_t *)pnew->MCFTPPath, (int8_t *)MSG_FTP_UPLOAD_PATH_KEY, buf);

	modify_uint16_value(pnode, pold->THRFTPPort, pnew->THRFTPPort, (int8_t *)MSG_THRFTPPORT_KEY, buf);
	modify_int8_value(pnode, (int8_t *)pold->THRFTPAddr, (int8_t *)pnew->THRFTPAddr, (int8_t *)MSG_THRFTP_SERVERIP_KEY, buf);
	modify_int8_value(pnode, (int8_t *)pold->THRFTPUserName, (int8_t *)pnew->THRFTPUserName, (int8_t *)MSG_THRFTP_USER_KEY, buf);
	modify_int8_value(pnode, (int8_t *)pold->THRFTPPassword, (int8_t *)pnew->THRFTPPassword, (int8_t *)MSG_THRFTP_PASSWORD_KEY, buf);
	modify_int8_value(pnode, (int8_t *)pold->THRFTPPath, (int8_t *)pnew->THRFTPPath, (int8_t *)MSG_THRFTP_UPLOAD_PATH_KEY, buf);
	
	return 0;
}

int32_t modify_ftp_info_only(const int8_t *xml_file, ftp_info *pold, ftp_info *pnew)
{
	xmlNodePtr pusr_info_node;
	parse_xml_t px;
	int32_t ret = -1;
	
	/* 需要在外部加锁 */

	if(NULL == xml_file || NULL == pold || NULL == pnew){
		zlog_debug(DBGLOG, "--[modify_ftp_info_only] failed, params is NULL!\n");
		return ret;
	}

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[modify_ftp_info_only] failed, init_file_dom_tree error, file: %s\n", xml_file);
		goto cleanup;
	}

	/* 会议室信息 */
	pusr_info_node = get_children_node(px.proot, CONFIG_PARAMS_FTP_INFO_KEY);
	ret = modify_ftp_info(pusr_info_node, pold, pnew);

	xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	
cleanup:

	release_dom_tree(px.pdoc);

//	r_system((const int8_t *)"sync");

	return ret;
}


static int32_t modify_config_info(xmlNodePtr pnode, config_info *pold, config_info *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pnode) {
		zlog_error(DBGLOG, "modify_language_info error, params is NULL!\n");
		return -1;
	}

	modify_int32_value(pnode, pold->language,   pnew->language, (int8_t *)MSG_LANGUAGE_KEY, buf);
	modify_int32_value(pnode, pold->Laniptype,  pnew->Laniptype, (int8_t *)MSG_LANIPTYPE_KEY, buf);
	modify_int32_value(pnode, pold->Waniptype,  pnew->Waniptype, (int8_t *)MSG_WANIPTYPE_KEY, buf);
	modify_addr_value(pnode,  pold->media_addr,   pnew->media_addr,   (int8_t *)MSG_MEDIA_ADDR_KEY,   buf);
	modify_addr_value(pnode,  pold->manager_addr, pnew->manager_addr, (int8_t *)MSG_MANAGER_ADDR_KEY, buf);
	return 0;
}

static int32_t modify_camctrl_info(xmlNodePtr pnode, camera_ctrl *pold, camera_ctrl *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pnode) {
		zlog_error(DBGLOG, "modify_camctrl_info error, params is NULL!\n");
		return -1;
	}

	modify_int32_value(pnode, pold->video0_addr,  pnew->video0_addr, (int8_t *)MSG_VIDEO_0_ADDR_KEY, buf);
	modify_int32_value(pnode, pold->video1_addr,  pnew->video1_addr, (int8_t *)MSG_VIDEO_1_ADDR_KEY, buf);
	modify_int32_value(pnode, pold->video2_addr,  pnew->video2_addr, (int8_t *)MSG_VIDEO_2_ADDR_KEY, buf);

	return 0;
}


static int32_t modify_user_info(xmlNodePtr pnode, user_info *pold, user_info *pnew)
{
	int32_t ret1 = 0;
	int32_t ret2 = 0;
	int32_t ret3 = 0;
	int32_t ret4 = 0;
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		zlog_error(DBGLOG, "modify_user_info error, params is NULL!\n");
		return -1;
	}

	ret1 = modify_int8_value(pnode, (int8_t *)pold->username, (int8_t *)pnew->username, (int8_t *)MSG_USER_KEY, buf);
	ret2 = modify_int8_value(pnode, (int8_t *)pold->password, (int8_t *)pnew->password, (int8_t *)MSG_PASSWORD_KEY, buf);

	ret3 = modify_int8_value(pnode, (int8_t *)pold->guest_name, (int8_t *)pnew->guest_name, (int8_t *)MSG_GUEST_KEY, buf);
	ret4 = modify_int8_value(pnode, (int8_t *)pold->guest_passwd, (int8_t *)pnew->guest_passwd, (int8_t *)MSG_GUEST_PASSWD_KEY, buf);

	if(0 == ret1 || 0 == ret2 || 0 == ret3 || 0 == ret4)
		ret1 = 0;

	return ret1;
}

static int32_t modify_heart_beat_info(xmlNodePtr pnode, heart_beat_info *pold, heart_beat_info *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		zlog_error(DBGLOG, "modify_heart_beat_info error, params is NULL!\n");
		return -1;
	}

	modify_int32_value(pnode, pold->post_time, pnew->post_time, (int8_t *)MSG_POST_TIME_KEY, buf);
	modify_int8_value(pnode, (int8_t *)pold->post_url, (int8_t *)pnew->post_url, (int8_t *)MSG_POST_URL_KEY, buf);
	modify_int8_value(pnode, (int8_t *)pold->time_serip, (int8_t *)pnew->time_serip, (int8_t *)MSG_TIME_SERIP_KEY, buf);

	return 0;
}

int32_t modify_heart_beat_info_only(const int8_t *xml_file, heart_beat_info *pold, heart_beat_info *pnew)
{
	xmlNodePtr pusr_info_node;
	parse_xml_t px;
	int32_t ret = 0;

	/* 需要在外部加锁 */

	if(NULL == xml_file || NULL == pold || NULL == pnew){
		zlog_debug(DBGLOG, "--[modify_heart_beat_info_only] failed, params is NULL!\n");
		return ret;
	}

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[modify_heart_beat_info_only] failed, init_file_dom_tree error, file: %s\n", xml_file);
		goto cleanup;
	}

	pusr_info_node = get_children_node(px.proot, CONFIG_PARAMS_HEART_BEAT_INFO_KEY);
	ret = modify_heart_beat_info(pusr_info_node, pold, pnew);

	xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	
cleanup:

	release_dom_tree(px.pdoc);

//	r_system((const int8_t *)"sync");

	return ret;
}



static int32_t modify_sys_info(xmlNodePtr pnode, sys_info *pold, sys_info *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		zlog_error(DBGLOG, "modify_sys_info error, params is NULL!\n");
		return -1;
	}

	/* 时间同步服务器 */
	modify_addr_value(pnode, pold->TimeServerAddr, pnew->TimeServerAddr, (int8_t *)MSG_TIMESERVER_ADDR_KEY, buf);

	/* 最大磁盘空间 */
	modify_uint32_value(pnode, pold->DiskMaxSpace, pnew->DiskMaxSpace, (int8_t *)MSG_DISK_MAXSPACE_KEY, buf);

	/* 磁盘有效空间 */
	modify_uint32_value(pnode, pold->DiskAvailableSpace, pnew->DiskAvailableSpace, (int8_t *)MSG_DISK_AVALIABLESPACE_KEY, buf);

	return 0;
}

int32_t modify_sys_info_only(const int8_t *xml_file, sys_info *pold, sys_info *pnew)
{
	xmlNodePtr pusr_info_node;
	parse_xml_t px;
	int32_t ret = -1;
	
	/* 需要在外部加锁 */

	if(NULL == xml_file || NULL == pold || NULL == pnew){
		zlog_debug(DBGLOG, "--[modify_sys_info_only] failed, params is NULL!\n");
		return ret;
	}

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[modify_sys_info_only] failed, init_file_dom_tree error, file: %s\n", xml_file);
		goto cleanup;
	}

	/* 会议室信息 */
	pusr_info_node = get_children_node(px.proot, CONFIG_PARAMS_SYS_INFO_KEY);
	ret = modify_sys_info(pusr_info_node, pold, pnew);

	xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	
cleanup:

	release_dom_tree(px.pdoc);

//	r_system((const int8_t *)"sync");

	return ret;
}


static int32_t modify_server_info(xmlNodePtr pnode, server_info *pold, server_info *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		zlog_error(DBGLOG, "modify_server_info error, params is NULL!\n");
		return -1;
	}

	/* 服务器类型 */
	modify_int8_value(pnode, (int8_t *)pold->ServerType, (int8_t *)pnew->ServerType, (int8_t *)MSG_SERVER_TYPE_KEY, buf);

	/* 服务器序列号 */
	modify_int8_value(pnode, (int8_t *)pold->ServerSeries, (int8_t *)pnew->ServerSeries, (int8_t *)MSG_SERVER_SERIES_KEY, buf);

	/* 服务器版本 */
	modify_int8_value(pnode, (int8_t *)pold->ServerVersion, (int8_t *)pnew->ServerVersion, (int8_t *)MSG_SERVER_VERSION_INFO_KEY, buf);

	/* 最大会议室数量 */
	modify_uint32_value(pnode, pold->MaxRoom, pnew->MaxRoom, (int8_t *)MSG_MAXROOM_KEY, buf);

	/* 会议室中最大编码器数量 */
	modify_uint32_value(pnode, pold->MaxCodecNumInRoom, pnew->MaxCodecNumInRoom, (int8_t *)MSG_MAXCODENUM_INROOM_KEY, buf);

	/* 内网IP */
	modify_addr_value(pnode, pold->LanAddr, pnew->LanAddr, (int8_t *)MSG_LANADDR_KEY, buf);

	/* 内网网关 */
	modify_addr_value(pnode, pold->LanGateWay, pnew->LanGateWay, (int8_t *)MSG_LANGATEWAY_KEY, buf);

	/* 内网掩码 */
	modify_addr_value(pnode, pold->LanNetmask, pnew->LanNetmask, (int8_t *)MSG_LANNETMASK_KEY, buf);

	/* 内网物理地址 */
	modify_int8_value(pnode, (int8_t *)pold->LanMac, (int8_t *)pnew->LanMac, (int8_t *)MSG_LANMAC_KEY, buf);

	/* 外网IP */
	modify_addr_value(pnode, pold->WanAddr, pnew->WanAddr, (int8_t *)MSG_WANADDR_KEY, buf);

	/* 外网网关 */
	modify_addr_value(pnode, pold->WanGateWay, pnew->WanGateWay, (int8_t *)MSG_WANGATEWAY_KEY, buf);

	/* 外网掩码 */
	modify_addr_value(pnode, pold->WanNetmask, pnew->WanNetmask, (int8_t *)MSG_WANNETMASK_KEY, buf);

	/* 外网物理地址 */
	modify_int8_value(pnode, (int8_t *)pold->WanMac, (int8_t *)pnew->WanMac, (int8_t *)MSG_WANMAC_KEY, buf);

	return 0;
}

static int32_t modify_audio_info(xmlNodePtr pnode, audio_info *pold, audio_info *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		zlog_error(DBGLOG, "modify_audio_info error, params is NULL!\n");
		return -1;
	}

	/* 输入模式 */
	modify_uint32_value(pnode, pold->InputMode, pnew->InputMode, (int8_t *)MSG_INPUTMODE_KEY, buf);

	/* 采样率 */
	modify_uint32_value(pnode, pold->SampleRate, pnew->SampleRate, (int8_t *)MSG_SAMPLERATE_KEY, buf);

	/* 比特率 */
	modify_uint32_value(pnode, pold->Bitrate, pnew->Bitrate, (int8_t *)MSG_BITRATE_KEY, buf);

	/* 左声道音量 */
	modify_uint32_value(pnode, pold->Lvolume, pnew->Lvolume, (int8_t *)MSG_LVOLUME_KEY, buf);

	/* 右声道音量 */
	modify_uint32_value(pnode, pold->Rvolume, pnew->Rvolume, (int8_t *)MSG_RVOLUME_KEY, buf);

	/* 右声道音量 */
	modify_int8_value(pnode, pold->InputIndex, pnew->InputIndex, (int8_t *)MSG_INPUTINDEX_KEY, buf);

	return 0;
}

static int32_t modify_pic_synt_info(xmlNodePtr pnode, pic_synt_info *pold, pic_synt_info *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		zlog_error(DBGLOG, "modify_pic_synt_info error, params is NULL!\n");
		return -1;
	}

	/* 源数量 */
	modify_uint32_value(pnode, pold->SourceNum, pnew->SourceNum, (int8_t *)MSG_PICTURE_SYNT_SOURCENUM_KEY, buf);

	/* 合成分辨率 */
	modify_uint32_value(pnode, pold->Resolution, pnew->Resolution, (int8_t *)MSG_PICTURE_SYNT_RESOLUTION_KEY, buf);

	/* 合成布局 */
	modify_int8_value(pnode, pold->Model, pnew->Model, (int8_t *)MSG_PICTURE_SYNT_MODEL_KEY, buf);

	/* 二画面默认的布局ID */
	modify_uint32_value(pnode, pold->LayoutDef_2, pnew->LayoutDef_2, (int8_t *)MSG_LAYOUT_DEF_2_KEY, buf);

	/* 三画面默认的布局ID */
	modify_uint32_value(pnode, pold->LayoutDef_3, pnew->LayoutDef_3, (int8_t *)MSG_LAYOUT_DEF_3_KEY, buf);

	return 0;
}

int32_t modify_pic_synt_info_only(const int8_t *xml_file, pic_synt_info *pold, pic_synt_info *pnew)
{
	xmlNodePtr ppic_synt_info_node;
	xmlNodePtr proom;
	parse_xml_t px;
	int32_t ret = 0;

	/* 需要在外部加锁 */

	if(NULL == xml_file || NULL == pold || NULL == pnew){
		zlog_debug(DBGLOG, "--[modify_pic_synt_info_only] failed, params is NULL!\n");
		return ret;
	}

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[modify_pic_synt_info_only] failed, init_file_dom_tree error, file: %s\n", xml_file);
		goto cleanup;
	}

	proom = get_children_node(px.proot, CONFIG_PARAMS_ROOM_INFO_KEY);
	ppic_synt_info_node = get_children_node(proom, CONFIG_PARAMS_PICTURE_SYNT_INFO_KEY);
	ret = modify_pic_synt_info(ppic_synt_info_node, pold, pnew);

	xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	
cleanup:

	release_dom_tree(px.pdoc);

//	r_system((const int8_t *)"sync");

	return ret;
}




static int32_t modify_quality_info(xmlNodePtr pnode, quality_info *pold, quality_info *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		zlog_error(DBGLOG, "modify_quality_info error, params is NULL!\n");
		return -1;
	}

	/* 是否启用此质量信息 */
	modify_uint32_value(pnode, pold->enable, pnew->enable, (int8_t *)CONFIG_PARAMS_QUALITY_ENABLE_KEY, buf);

	/* 码流类型 */
	modify_uint32_value(pnode, pold->RateType, pnew->RateType, (int8_t *)MSG_ENC_RATETYPE_KEY, buf);

	/* 比特率 */
	modify_uint32_value(pnode, pold->EncBitrate, pnew->EncBitrate, (int8_t *)MSG_ENC_BITRATE_KEY, buf);

	/* 编码宽 */
	modify_uint32_value(pnode, pold->EncWidth, pnew->EncWidth, (int8_t *)MSG_ENC_WIDTH_KEY, buf);

	/* 编码高 */
	modify_uint32_value(pnode, pold->EncHeight, pnew->EncHeight, (int8_t *)MSG_ENC_HEIGHT_KEY, buf);

	/* 编码帧率 */
	modify_uint32_value(pnode, pold->EncFrameRate, pnew->EncFrameRate, (int8_t *)MSG_ENC_FRAMERATE_KEY, buf);

	return 0;
}

static int32_t modify_enc_info(xmlNodePtr pnode, enc_info *pold, enc_info *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	xmlNodePtr pqua_node;

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		zlog_error(DBGLOG, "modify_enc_info error, params is NULL!\n");
		return -1;
	}

	/* 索引值 */
	modify_uint32_value(pnode, pold->ID, pnew->ID, (int8_t *)MSG_ENC_ID_KEY, buf);
#if 0
	/* 编码器IP */
	modify_addr_value(pnode, pold->EncIP, pnew->EncIP, (int8_t *)MSG_ENC_IP_KEY, buf);
#endif

	/* 编码器IP */
	modify_int8_value(pnode, pold->EncIP, pnew->EncIP, (int8_t *)MSG_ENC_IP_KEY, buf);

	/* 状态 */
	modify_uint32_value(pnode, pold->Status, pnew->Status, (int8_t *)MSG_ENC_STATUS_KEY, buf);

	/* 质量参数 */
	/* FIXME:注意顺序 */
	pqua_node = get_children_node(pnode, CONFIG_PARAMS_QUALITY_INFO_KEY);
	if(pqua_node){
		modify_quality_info(pqua_node, &pold->HD_QuaInfo, &pnew->HD_QuaInfo);
	}

	pqua_node = find_next_node(pqua_node, CONFIG_PARAMS_QUALITY_INFO_KEY);
	if(pqua_node){
		modify_quality_info(pqua_node, &pold->SD_QuaInfo, &pnew->SD_QuaInfo);
	}

	return 0;
}

static int32_t modify_room_info(xmlNodePtr pnode, room_info *pold, room_info *pnew)
{
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	xmlNodePtr paudio_node;
	xmlNodePtr ppic_synt_node;
	xmlNodePtr penc_node;
	int32_t index = 0;

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		zlog_error(DBGLOG, "modify_room_info error, params is NULL!\n");
		return -1;
	}

	/* 会议室ID */
	modify_uint32_value(pnode, pold->RoomID, pnew->RoomID, (int8_t *)MSG_ROOMID_KEY, buf);

	/* 会议室名称 */
	modify_int8_value(pnode, (int8_t *)pold->RoomName, (int8_t *)pnew->RoomName, (int8_t *)MSG_ROOMNAME_KEY, buf);

	/* 连接状态 */
	modify_uint32_value(pnode, pold->ConnStatus, pnew->ConnStatus, (int8_t *)MSG_CONNECT_STATUS_KEY, buf);

	/* 录制最大时长 */
	modify_uint32_value(pnode, pold->RecordMaxTime, pnew->RecordMaxTime, (int8_t *)MSG_RECORD_MAXTIME_KEY, buf);

	/* 录制状态 */
	modify_uint32_value(pnode, pold->RecStatus, pnew->RecStatus, (int8_t *)MSG_RECORD_STATUS_KEY, buf);

	/* 评教开启状态 */
	modify_uint32_value(pnode, pold->IfMark, pnew->IfMark, (int8_t *)MSG_ROOM_IFMARK_KEY, buf);

	/* 录制文件名 */
	modify_int8_value(pnode, (int8_t *)pold->RecName, (int8_t *)pnew->RecName, (int8_t *)MSG_ROOM_RECNAME_KEY, buf);

	zlog_debug(OPELOG, "Resolution = %d, %d\n", pold->PicSyntInfo.Resolution, pnew->PicSyntInfo.Resolution);
	zlog_debug(OPELOG, "Model = %s, %s\n", pold->PicSyntInfo.Model, pnew->PicSyntInfo.Model);
	zlog_debug(OPELOG, "SourceNum = %d, %d\n", pold->PicSyntInfo.SourceNum, pnew->PicSyntInfo.SourceNum);

	/* 合成画面信息 */
	ppic_synt_node = get_children_node(pnode, CONFIG_PARAMS_PICTURE_SYNT_INFO_KEY);
	modify_pic_synt_info(ppic_synt_node, &pold->PicSyntInfo, &pnew->PicSyntInfo);

	/* 音频信息 */
	paudio_node = get_children_node(pnode, CONFIG_PARAMS_AUDIO_INFO_KEY);
	modify_audio_info(paudio_node, &pold->AudioInfo, &pnew->AudioInfo);

	/* 编码器信息 */
	/* FIXME: 不按顺序时??? */
	penc_node = get_children_node(pnode, CONFIG_PARAMS_ENC_INFO_KEY);
	for(index = 0; index < RECORD_ROOM_MAX_ENC_NUM; index++){
		modify_enc_info(penc_node, &pold->EncInfo[index], &pnew->EncInfo[index]);
		penc_node = find_next_node(penc_node, CONFIG_PARAMS_ENC_INFO_KEY);
	}
	

	return 0;
}

int32_t modify_room_info_only(const int8_t *xml_file, room_info *pold, room_info *pnew)
{
	xmlNodePtr pusr_info_node;
	xmlNodePtr pnode;
	parse_xml_t px;
	int32_t ret = -1;
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	
	/* 需要在外部加锁 */
	if(NULL == xml_file || NULL == pold || NULL == pnew){
		zlog_debug(DBGLOG, "--[modify_room_info_only] failed, params is NULL!\n");
		return ret;
	}
	
	zlog_debug(DBGLOG,"modify_room_info_only -----old_name :%s--new_name : %s---- len : %d\n",pold->RoomName,pnew->RoomName,sizeof(room_info));
	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[modify_room_info_only] failed, init_file_dom_tree error, file: %s\n", xml_file);
		goto cleanup;
	}

	/* 会议室信息 */

	pusr_info_node = get_children_node(px.proot, CONFIG_PARAMS_ROOM_INFO_KEY);
	while(pusr_info_node){
		pnode = get_children_node(pusr_info_node, MSG_ROOMID_KEY);
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)buf, XML_VALUE_MAX_LENGTH, px.pdoc, pnode);
		if(pnew->RoomID == atoi((const char *)buf)){
			break;
		}
		pusr_info_node = find_next_node(pusr_info_node, CONFIG_PARAMS_ROOM_INFO_KEY);
	}

	if(pusr_info_node){
		ret = modify_room_info(pusr_info_node, pold, pnew);
		xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	}
	
cleanup:
	
	release_dom_tree(px.pdoc);

//	r_system((const int8_t *)"sync");

	return ret;
}



int32_t modify_usr_info_only(const int8_t *xml_file, user_info *pold, user_info *pnew)
{
	xmlNodePtr proom_info_node;
	parse_xml_t px;
	int32_t ret = -1;
	/* 需要在外部加锁 */

	if(NULL == xml_file || NULL == pold || NULL == pnew){
		zlog_debug(DBGLOG, "--[modify_room_info_only] failed, params is NULL!\n");
		return ret;
	}

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[modify_room_info_only] failed, init_file_dom_tree error, file: %s\n", xml_file);
		goto cleanup;
	}

	/* 用户信息 */
	proom_info_node = get_children_node(px.proot, CONFIG_PARAMS_USER_INFO_KEY);
	ret = modify_user_info(proom_info_node, pold, pnew);
	xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	
cleanup:
	
	release_dom_tree(px.pdoc);

//	r_system((const int8_t *)"sync");

	return ret;
}

int32_t modify_server_info_only(const int8_t *xml_file, server_info *pold, server_info *pnew)
{
	xmlNodePtr pserver_info_node;
	parse_xml_t px;
	int32_t ret = -1;
	
	/* 需要在外部加锁 */

	if(NULL == xml_file || NULL == pold || NULL == pnew){
		zlog_debug(DBGLOG, "--[modify_room_info_only] failed, params is NULL!\n");
		return ret;
	}

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[modify_room_info_only] failed, init_file_dom_tree error, file: %s\n", xml_file);
		goto cleanup;
	}

	/* 会议室信息 */
	pserver_info_node = get_children_node(px.proot, CONFIG_PARAMS_SERVER_INFO_KEY);
	ret = modify_server_info(pserver_info_node, pold, pnew);
	xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	
cleanup:
	
	release_dom_tree(px.pdoc);

//	r_system((const int8_t *)"sync");

	return ret;
}




int32_t modify_config_only(const int8_t *xml_file, config_info *pold, config_info *pnew)
{
	xmlNodePtr pusr_info_node;
	parse_xml_t px;
	int32_t ret = -1;
	/* 需要在外部加锁 */

	if(NULL == xml_file || NULL == pold || NULL == pnew){
		zlog_debug(DBGLOG, "--[modify_room_info_only] failed, params is NULL!\n");
		return ret;
	}

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[modify_room_info_only] failed, init_file_dom_tree error, file: %s\n", xml_file);
		goto cleanup;
	}

	/* 普通配置信息 */
	pusr_info_node = get_children_node(px.proot, CONFIG_PARAMS_CONFIG_INFO_KEY);
	ret = modify_config_info(pusr_info_node, pold, pnew);
	xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	
cleanup:
	
	release_dom_tree(px.pdoc);

//	r_system((const int8_t *)"sync");

	return ret;
}

int32_t modify_camctrl_info_only(const int8_t *xml_file, camera_ctrl *pold, camera_ctrl *pnew)
{
	xmlNodePtr pinfo_node;
	parse_xml_t px;
	int32_t ret = -1;
	/* 需要在外部加锁 */

	if(NULL == xml_file || NULL == pold || NULL == pnew){
		zlog_debug(DBGLOG, "--[modify_camctrl_info_only] failed, params is NULL!\n");
		return ret;
	}

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[modify_camctrl_info_only] failed, init_file_dom_tree error, file: %s\n", xml_file);
		goto cleanup;
	}

	/* 普通配置信息 */
	pinfo_node = get_children_node(px.proot, CONFIG_PARAMS_CAMCTRL_ADDR_KEY);
	ret = modify_camctrl_info(pinfo_node, pold, pnew);
	xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	
cleanup:
	
	release_dom_tree(px.pdoc);

	return ret;
}



int32_t create_params_table_file(const int8_t *xml_file, all_server_info *ptable)
{
	int32_t room_count = 0;
	int32_t index = 0;
	int32_t ret = 0;
	int8_t buf[XML_VALUE_MAX_LENGTH] = {0};
	xmlDocPtr doc;
	xmlNodePtr proot_node;
	xmlNodePtr puser_info_node;
	xmlNodePtr pcamctrl_info_node;
	xmlNodePtr phbeat_info_node;
	xmlNodePtr pconfig_info_node;
	xmlNodePtr psys_info_node;
	xmlNodePtr pserver_info_node;
	xmlNodePtr proom_info_node[CONTROL_ROOM_SERVER_MAX_USER];

	if(NULL == xml_file || NULL == ptable){
		zlog_debug(DBGLOG, "--[create_params_table_file] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&ptable->info_m);

	doc = xmlNewDoc(XML_DOC_VERSION);
	if(NULL == doc) {
		zlog_error(DBGLOG, "--[create_params_table_file]failed, xmlNewDoc error, file: %s\n", xml_file);
		pthread_mutex_unlock(&ptable->info_m);
		return -1;
	}

	/* 根节点 */
	proot_node = xmlNewNode(NULL, CONFIG_PARAMS_TABLES_KEY);
	xmlDocSetRootElement(doc, proot_node);

	/* 用户信息 */
	puser_info_node = xmlNewNode(NULL, CONFIG_PARAMS_USER_INFO_KEY);
	xmlAddChild(proot_node, puser_info_node);
	add_user_info(puser_info_node, &ptable->Authentication);

	/* 远遥地址信息 */
	pcamctrl_info_node = xmlNewNode(NULL, CONFIG_PARAMS_CAMCTRL_ADDR_KEY);
	xmlAddChild(proot_node, pcamctrl_info_node);
	add_camctrl_info(pcamctrl_info_node, &ptable->CamCtrlInfo);

	/* HEART BEAT信息 */
	phbeat_info_node = xmlNewNode(NULL, CONFIG_PARAMS_HEART_BEAT_INFO_KEY);
	xmlAddChild(proot_node, phbeat_info_node);
	add_heart_beat_info(phbeat_info_node, &ptable->HBeatInfo);

	
	/*用户配置 */
	pconfig_info_node = xmlNewNode(NULL, CONFIG_PARAMS_CONFIG_INFO_KEY);
	xmlAddChild(proot_node, pconfig_info_node);

	add_config_info(pconfig_info_node, &ptable->ConfigInfo);

#if 1
	/* 系统信息 */
	psys_info_node = xmlNewNode(NULL, CONFIG_PARAMS_SYS_INFO_KEY);
	xmlAddChild(proot_node, psys_info_node);
	add_sys_info(psys_info_node, &ptable->SysInfo);

	/* 服务器信息 */
	pserver_info_node = xmlNewNode(NULL, CONFIG_PARAMS_SERVER_INFO_KEY);
	xmlAddChild(proot_node, pserver_info_node);
	add_server_info(pserver_info_node, &ptable->ServerInfo);
	
	room_count = ptable->ServerInfo.MaxRoom;
	if(room_count > CONTROL_ROOM_SERVER_MAX_USER){
		room_count = CONTROL_ROOM_SERVER_MAX_USER;
	}

	for(index = 0; index < room_count; index++){
		r_memset(buf, 0, XML_VALUE_MAX_LENGTH);
		proom_info_node[index] = xmlNewNode(NULL, CONFIG_PARAMS_ROOM_INFO_KEY);
		xmlAddChild(proot_node, proom_info_node[index]);
		add_room_info(proom_info_node[index], &ptable->RoomInfo[index]);
	}

#endif

	/*保存新的xml文件*/
	ret = xmlSaveFormatFileEnc((const char *)xml_file, doc, XML_TEXT_CODE_TYPE, 1);
	if(-1 == ret) {
		zlog_error(DBGLOG, "--[create_params_table_file]failed, xml save params table failed !!!\n");
		ret = -1;
		goto cleanup;
	}


cleanup:

	release_dom_tree(doc);

//	r_system((const int8_t *)"sync");

	pthread_mutex_unlock(&ptable->info_m);

	return ret;
}

int32_t read_params_table_file(const int8_t *xml_file, all_server_info *ptable)
{
	int32_t index = 0;
	int32_t room_count = 0;
	
	xmlDocPtr pdoc;
	xmlNodePtr proot_node;
	xmlNodePtr puser_info_node;
	xmlNodePtr pcamtrol_info_node;
	xmlNodePtr phbeat_info_node;
	xmlNodePtr pftp_info_node;
	xmlNodePtr psys_info_node;
	xmlNodePtr pconfig_info_node;
	xmlNodePtr pserver_info_node;
	xmlNodePtr proom_info_node[CONTROL_ROOM_SERVER_MAX_USER];

	parse_xml_t px;

	if(NULL == xml_file || NULL == ptable){
		zlog_debug(DBGLOG, "--[read_params_table_file] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&ptable->info_m);

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[read_params_table_file] failed, init_file_dom_tree error, xml file: %s\n", xml_file);
		goto cleanup;
	}

	pdoc = px.pdoc;
	proot_node = px.proot;

	/* 用户信息 */
	puser_info_node = get_children_node(proot_node, CONFIG_PARAMS_USER_INFO_KEY);
	if(puser_info_node){
		read_user_info(pdoc, puser_info_node, &ptable->Authentication);
	}
	else{
		/* 新增用户信息节点 */
		puser_info_node = xmlNewNode(NULL, CONFIG_PARAMS_USER_INFO_KEY);
		xmlAddChild(proot_node, puser_info_node);
		add_user_info(puser_info_node, &ptable->Authentication);
	}

	/*  远遥地址信息 */
	pcamtrol_info_node = get_children_node(proot_node, CONFIG_PARAMS_CAMCTRL_ADDR_KEY);
	if(pcamtrol_info_node){
		read_camctrl_info(pdoc, pcamtrol_info_node, &ptable->CamCtrlInfo);
	}
	else{
		/* 新增用户信息节点 */
		pcamtrol_info_node = xmlNewNode(NULL, CONFIG_PARAMS_CAMCTRL_ADDR_KEY);
		xmlAddChild(proot_node, pcamtrol_info_node);
		add_camctrl_info(pcamtrol_info_node, &ptable->CamCtrlInfo);
	}

	/* HTTP信息 */
	phbeat_info_node = get_children_node(proot_node, CONFIG_PARAMS_HEART_BEAT_INFO_KEY);
	if(phbeat_info_node){
		read_heart_beat_info(pdoc, phbeat_info_node, &ptable->HBeatInfo);
	}
	else{
		/* 新增HTTP信息节点 */
		phbeat_info_node = xmlNewNode(NULL, CONFIG_PARAMS_HEART_BEAT_INFO_KEY);
		xmlAddChild(proot_node, phbeat_info_node);
		add_heart_beat_info(phbeat_info_node, &ptable->HBeatInfo);
	}

	/* FTP服务器信息 */
	pftp_info_node = get_children_node(proot_node, CONFIG_PARAMS_FTP_INFO_KEY);
	if(pftp_info_node){
		read_ftp_info(pdoc, pftp_info_node, &ptable->FtpInfo);
	}
	else{
		/* 新增FTP服务器信息节点 */
		pftp_info_node = xmlNewNode(NULL, CONFIG_PARAMS_FTP_INFO_KEY);
		xmlAddChild(proot_node, pftp_info_node);
		add_ftp_info(pftp_info_node, &ptable->FtpInfo);
	}

	/* 配置信息 */
	pconfig_info_node = get_children_node(proot_node, CONFIG_PARAMS_CONFIG_INFO_KEY);
	if(pconfig_info_node){
		read_config_info(pdoc, pconfig_info_node, &ptable->ConfigInfo);
	}
	else{
		/* 新增配置信息节点 */
		pconfig_info_node = xmlNewNode(NULL, CONFIG_PARAMS_CONFIG_INFO_KEY);
		xmlAddChild(proot_node, pconfig_info_node);
		add_config_info(pconfig_info_node, &ptable->ConfigInfo);
	}

	/* 系统信息 */
	psys_info_node = get_children_node(proot_node, CONFIG_PARAMS_SYS_INFO_KEY);
	if(psys_info_node){
		read_sys_info(pdoc, psys_info_node, &ptable->SysInfo);
	}
	else{
		/* 新增系统信息节点 */
		psys_info_node = xmlNewNode(NULL, CONFIG_PARAMS_SYS_INFO_KEY);
		xmlAddChild(proot_node, psys_info_node);
		add_sys_info(psys_info_node, &ptable->SysInfo);
	}

	/* 服务器信息 */
	pserver_info_node = get_children_node(proot_node, CONFIG_PARAMS_SERVER_INFO_KEY);
	if(pserver_info_node){
		read_server_info(pdoc, pserver_info_node, &ptable->ServerInfo);
	}
	else{
		/* 新增系统信息节点 */
		pserver_info_node = xmlNewNode(NULL, CONFIG_PARAMS_SERVER_INFO_KEY);
		xmlAddChild(proot_node, pserver_info_node);
		add_server_info(pserver_info_node, &ptable->ServerInfo);
	}

	room_count = ptable->ServerInfo.MaxRoom;
	if(room_count > CONTROL_ROOM_SERVER_MAX_USER){
		room_count = CONTROL_ROOM_SERVER_MAX_USER;
	}

	/* 会议室信息 */
	for(index = 0; index < room_count; index++){
		fprintf(stderr, "0000-room_count = %d\n", room_count);
		if(index == 0)
			proom_info_node[index] = get_children_node(proot_node, CONFIG_PARAMS_ROOM_INFO_KEY);
		else
			proom_info_node[index] = find_next_node(proom_info_node[index-1], CONFIG_PARAMS_ROOM_INFO_KEY);
		
		if(proom_info_node[index]){
			read_room_info(pdoc, proom_info_node[index], &ptable->RoomInfo[index]);
		}
		else{
			/* 新增加会议室信息节点 */
			proom_info_node[index] = xmlNewNode(NULL, CONFIG_PARAMS_ROOM_INFO_KEY);
			xmlAddChild(proot_node, proom_info_node[index]);
			add_room_info(proom_info_node[index], &ptable->RoomInfo[index]);
		}
	}
	
cleanup:

	xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	release_dom_tree(px.pdoc);

	pthread_mutex_unlock(&ptable->info_m);

	return 0;
}

int32_t modify_params_table_file(const int8_t *xml_file, all_server_info *pold_table, all_server_info *pnew_table)
{
	int32_t index = 0;
	int32_t room_count = 0;
	xmlNodePtr puser_info_node;
	xmlNodePtr phbeat_info_node;
	xmlNodePtr psys_info_node;
	xmlNodePtr pserver_info_node;
	xmlNodePtr proom_info_node[CONTROL_ROOM_SERVER_MAX_USER];

	parse_xml_t px;
	xmlDocPtr pdoc;
	xmlNodePtr proot_node;

	if(NULL == xml_file || NULL == pold_table || NULL == pnew_table){
		zlog_debug(DBGLOG, "--[modify_params_table_file] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&pold_table->info_m);

	init_file_dom_tree(&px, (const char *)xml_file);
	if(NULL == px.pdoc || NULL == px.proot){
		zlog_error(DBGLOG, "--[modify_params_table_file] failed, init_file_dom_tree error, xml file: %s\n", xml_file);
		goto cleanup;
	}

	pdoc = px.pdoc;
	proot_node = px.proot;

	/* 用户信息 */
	puser_info_node = get_children_node(proot_node, CONFIG_PARAMS_USER_INFO_KEY);
	if(puser_info_node){
		modify_user_info(puser_info_node, &pold_table->Authentication, &pnew_table->Authentication);
	}

	/* HTTP信息 */
	phbeat_info_node = get_children_node(proot_node, CONFIG_PARAMS_HEART_BEAT_INFO_KEY);
	if(phbeat_info_node){
		modify_heart_beat_info(phbeat_info_node, &pold_table->HBeatInfo, &pnew_table->HBeatInfo);
	}

	/* 系统信息 */
	psys_info_node = get_children_node(proot_node, CONFIG_PARAMS_SYS_INFO_KEY);
	if(psys_info_node){
		modify_sys_info(psys_info_node, &pold_table->SysInfo, &pnew_table->SysInfo);
	}

	/* 服务器信息 */
	pserver_info_node = get_children_node(proot_node, CONFIG_PARAMS_SERVER_INFO_KEY);
	if(pserver_info_node){
		modify_server_info(pserver_info_node, &pold_table->ServerInfo, &pnew_table->ServerInfo);
	}

	room_count = pold_table->ServerInfo.MaxRoom;
	if(room_count > CONTROL_ROOM_SERVER_MAX_USER){
		room_count = CONTROL_ROOM_SERVER_MAX_USER;
	}

	/* 会议室信息 */
	for(index = 0; index < room_count; index++){
		if(index == 0)
			proom_info_node[index] = get_children_node(proot_node, CONFIG_PARAMS_ROOM_INFO_KEY);
		else
			proom_info_node[index] = find_next_node(proom_info_node[index-1], CONFIG_PARAMS_ROOM_INFO_KEY);
		if(proom_info_node[index]){
			modify_room_info(proom_info_node[index], &pold_table->RoomInfo[index], &pnew_table->RoomInfo[index]);
		}
	}
	
cleanup:
	xmlSaveFormatFile((const char *)xml_file, px.pdoc, 1);
	release_dom_tree(px.pdoc);

	pthread_mutex_unlock(&pold_table->info_m);

	return 0;
}

int32_t reset_record_params(all_server_info *pinfo)
{
	int32_t index = 0;
	
	if(NULL == pinfo){
		zlog_debug(DBGLOG, "--[reset_record_params] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&pinfo->info_m);
	for(index = 0; index < pinfo->ServerInfo.MaxRoom && index < CONTROL_ROOM_SERVER_MAX_USER; index++){
		pinfo->RoomInfo[index].RecStatus = 0;
		r_memset(pinfo->RoomInfo[index].RecName, 0, RECORD_FILE_MAX_FILENAME);
	}
	pthread_mutex_unlock(&pinfo->info_m);

	return 0;
}

uint32_t get_synt_resoluteion()
{
	uint32_t resolution;
	server_set *pser = NULL;
	
	if(NULL == gpser){
		zlog_debug(DBGLOG, "--[get_synt_resoluteion] failed, pinfo is NULL!\n");
		return 3;
	}

	pser = gpser;

	if(NULL == pser->pserinfo){
		zlog_debug(DBGLOG, "--[get_synt_resoluteion] failed, pserinfo is NULL!\n");
		return 3;
	}

	pthread_mutex_lock(&pser->pserinfo->info_m);
	resolution = pser->pserinfo->RoomInfo[0].PicSyntInfo.Resolution;
	pthread_mutex_unlock(&pser->pserinfo->info_m);

	return resolution;
}

uint32_t get_synt_bitrate()
{
	uint32_t bitrate;
	server_set *pser = NULL;
	
	if(NULL == gpser){
		zlog_debug(DBGLOG, "--[get_synt_bitrate] failed, pinfo is NULL!\n");
		return 3;
	}

	pser = gpser;

	if(NULL == pser->pserinfo){
		zlog_debug(DBGLOG, "--[get_synt_bitrate] failed, pserinfo is NULL!\n");
		return 3;
	}

	pthread_mutex_lock(&pser->pserinfo->info_m);
	bitrate = pser->pserinfo->RoomInfo[0].EncInfo[0].HD_QuaInfo.EncBitrate;
	pthread_mutex_unlock(&pser->pserinfo->info_m);

	return bitrate;
}



