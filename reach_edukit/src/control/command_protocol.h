/*
 * =====================================================================================
 *
 *       Filename:  command_protocol.h
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


#ifndef _COMMAND_PROTOCOL_H_
#define _COMMAND_PROTOCOL_H_

#include <stdlib.h>
#include <unistd.h>

typedef struct strMsgHeader{
	uint16_t sLen;				//长度
	uint16_t sVer;				//版本
	uint16_t sMsgType;			//消息类型
	uint16_t sData;				//保留
}MsgHeader,*pMsgHeader;

#define CONTROL_FOR_SERVER_PORT						(3000)
#define CONTROL_ROOM_SERVER_PORT_START				(3200)
#define CONTROL_ROOM_SERVER_PORT_END				(3215)

#define CONTROL_FOR_SERVER_MAX_USER					(CONTROL_MAX_USER) /* 不得大于 CONTROL_MAX_USER 值 */
#define CONTROL_MAX_USER							(20)
#define CONTROL_ROOM_SERVER_MAX_USER				(CONTROL_ROOM_SERVER_PORT_END - \
													CONTROL_ROOM_SERVER_PORT_START \
													+ 1)
#define CONTROL_DEFAULT_ROOM_COUNT					(1)

#define RECORD_ID_MAX_LENGTH						(128+2)



/***********************************************************************************/
/* 关于FTP */
#define FTP_MAX_USERNAME_LENGTH				(256)
#define FTP_MAX_PASSWD_LENGTH				(256)
#define FTP_MAX_FTPPATH_LENGTH				(512)
#define FTP_MODE_MEDIACENTER				(0)
#define FTP_MODE_THIRDFTP					(1)
/***********************************************************************************/



/************************************** 信令 ****************************************/

#define CONTROL_OLD_MSGHEADER_MSGCODE				(1)
#define CONTROL_OLD_MSGHEADER_VERSION				(1)
#define CONTROL_NEW_MSGHEADER_MSGCODE				(1)
#define CONTROL_NEW_MSGHEADER_VERSION				(2012)


#define MSGCODE_CHECK_USER							(30001)
#define MSGCODE_GET_SERVER_INFO						(30002)
#define MSGCODE_GET_ROOM_INFO						(30003)
#define MSGCODE_SET_ROOM_INFO						(30004)
#define MSGCODE_SET_ROOM_QUALITY					(30005)
#define MSGCODE_SET_ROOM_AUDIO_INFO					(30006)
#define MSGCODE_GET_FTPUPLOAD_INFO					(30007)
#define MSGCODE_SET_SERVER_SYS_PARAMS				(30008)
#define MSGCODE_GET_SERVER_SYS_PARAMS				(30009)
#define MSGCODE_RECORD_CONTROL						(30010)

#define MSGCODE_STREAM_REQUEST						(30011)
#define MSGCODE_REBOOT_SERVER						(30012)
#define MSGCODE_CAMERA_CTROL						(30013)
#define MSGCODE_IFRAME_REQUEST						(30014)
#define MSGCODE_CONNECT_ROOM_REQUEST				(30015)
#define MSGCODE_UPLOAD_LOGO_PICTURE					(30016)
#define MSGCODE_UPDATE_SERVER						(30017)
#define MSGCODE_ADD_TEXT_TITLE						(30018)
#define MSGCODE_GET_AUDIO_VOLUME					(30019)
#define MSGCODE_SET_AUDIO_MUTE						(30020)

#define MSGCODE_VIDEO_ADJUST						(30021)
#define MSGCODE_FILE_DELETE_REPORT					(30022)
#define MSGCODE_GET_FILELIST_INFO					(30023)
#define MSGCODE_HEART_REPORT_REQ					(30024)

#define MSGCODE_ENCINFO_REPORT_REQ					(30032)

#define MSGCODE_REPORT_NEW_PLATFORM_LOGIN			(30040)

#define MSGCODE_GET_CAMCTL_PROLIST					(30041)
#define MSGCODE_SET_CAMCTL_PRO						(30042)
#define MSGCODE_UPLOAD_CAMCTL_PRO					(30043)
#define MSGCODE_DEL_CAMCTL_PRO						(30044)

#define MSGCODE_GET_ENC_VERINFO						(30045)
#define MSGCODE_GET_AUDIO_INFO						(30046)
#define MSGCODE_PICTURE_SYNTHESIS					(30047)
#define MSGCODE_REPLAY_REQ							(30048)
#define MSGCODE_STUDIO_DIRECTOR_INFO				(30060)





#define MSGCODE_ROOM_REPORT_RECORD_STATUS			(36001)
#define MSGCODE_ROOM_GET_PREREC_INFO				(36002)


#define MSGCODE_LIVENODE_HEART_BEAT					(37001)

#define MSGCODE_WARNING_REPORT						(40001)
#define MSGCODE_LOG_REPORT_MC						(40002)
#define MSGCODE_LOG_REPORT_MP						(30031)

#define MSGCODE_SET_SWMS_LAYOUT					(31101)
#define MSGCODE_GET_SWMS_LAYOUT					(31102)


#define MSGCODE_HTTP_HEART_REPORT_REQ				(10005)
#define MSGCODE_HTTP_FTPINFO_REQ					(10006)
#define MSGCODE_HTTP_FTPUPLOAD_REPORT				(10007)
#define MSGCODE_HTTP_WARMING_REPORT					(10009)



#define WARNING_WARN_ID_4							(4)


#define WARNING_SOURCE_CAPTURE						("CAP")



typedef enum _platform_em_{
	MediaCenter = 0,		/* 媒体中心 */
	ManagePlatform,			/* 管理平台 */
	IpadUser,				/* ipad预览用户 */
	RecServer,				/* 教室*/
	LiveNode,				/* 直播节点 */
	ComControl,				/* 串口中控 */
	Mp4Repair,				/* MP4修复程序 */
	WebCtrl,				/* WEB控制端 */
	Director,				/*导播平台*/
	ThirdControl,			/* 其它 */
	AllPlatform,			/* 所有平台 */
	InvalidPlatform			/* 无效 */
}platform_em;






/************************************************************************************/




#endif
