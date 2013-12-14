/*
 * =====================================================================================
 *
 *       Filename:  command_resolve.h
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


#ifndef _COMMAND_RESOLVE_H_
#define _COMMAND_RESOLVE_H_

#include "control.h"
#include "xml/xml_base.h"
#include "xml_msg_management.h"

#define		KEEP_COURSE_FILE

#define XML_USER_NAME_MAX_LENGTH			(128)
#define XML_USER_PASSWORD_MAX_LENGTH		(128)
#define CONTROL_VERSION_LEN					(64)

#define PICTURE_VALUE_LENGTH				(8)

#define OPERATION_BROADCASE					(0)
#define OPERATION_ONLY_RETURN				(1)

#define IPINFO_MAX_LENGTH					(24)
#define GET_FILELIST_MAX_NUM				(512)

#define ROOM_NAME_MAX_LENGTH				(512)


#define CONTROL_ADMIN_USER					("admin")
#define CONTROL_GUEST_USER					("guest")


#define REQ_VALUE							("RequestMsg")
#define RESP_VALUE							("ResponseMsg")

#define XML_MSG_TYPE_REQ					(0)
#define XML_MSG_TYPE_RESP					(1)
#define XML_MSG_TYPE_INVALID				(-1)

#define ENC_INFO_SPE_VALUE					(0xe004a8c0)	/* ����Ϊ���⴦�� */


/***********************************************************************************/
/* ����XML�Ĺ���ֵ */
#define XML_DOC_VERSION						(BAD_CAST "1.0")
#define XML_TEXT_CODE_TYPE					("UTF-8")
#define XML_NULL_VALUE						("")
#define XML_RETURNCODE_SUCCESS				("1")
#define XML_RETURNCODE_FAILED				("0")
#define XML_RETURNCODE_FAILED2				("2")
#define XML_RETURNCODE_GUEST				("2")
/***********************************************************************************/

/***********************************************************************************/
/* �����������Ϣ������Ӧ��Ϣ�Ĺ������� */
#define REQ_ROOT_KEY						(BAD_CAST REQ_VALUE)
#define RESP_ROOT_KEY						(BAD_CAST RESP_VALUE)

#define MSG_HEAD_KEY						(BAD_CAST "MsgHead")
#define MSG_CODE_KEY						(BAD_CAST "MsgCode")
#define MSG_RETURNCODE_KEY					(BAD_CAST "ReturnCode")
#define MSG_PASSKEY_KEY						(BAD_CAST "PassKey")
#define MSG_BODY_KEY						(BAD_CAST "MsgBody")
#define MSG_AUTH_REQ_KEY					(BAD_CAST "AuthReq")
#define MSG_USER_KEY						(BAD_CAST "User")
#define MSG_PASSWORD_KEY					(BAD_CAST "Password")
#define MSG_GUEST_KEY						(BAD_CAST "Guest")
#define MSG_GUEST_PASSWD_KEY				(BAD_CAST "GuestPasswd")

#define MSG_USERID_KEY						(BAD_CAST "UserID")

#define MSG_POST_TIME_KEY					(BAD_CAST "POST_TIME")
#define MSG_POST_URL_KEY					(BAD_CAST "POST_URL")

#define MSG_TIME_SERIP_KEY					(BAD_CAST "TIME_SERIP")

#define MSG_VIDEO_0_ADDR_KEY				(BAD_CAST "VideoAddr_0")
#define MSG_VIDEO_1_ADDR_KEY				(BAD_CAST "VideoAddr_1")
#define MSG_VIDEO_2_ADDR_KEY				(BAD_CAST "VideoAddr_2")




#define MSG_LANGUAGE_KEY				   	(BAD_CAST "Language")
#define MSG_LANIPTYPE_KEY					(BAD_CAST "LanIpType")
#define MSG_WANIPTYPE_KEY					(BAD_CAST "WanIpType")
#define MSG_MEDIA_ADDR_KEY					(BAD_CAST "MediaAddr")
#define MSG_MANAGER_ADDR_KEY     			 	(BAD_CAST "ManagerAddr")
#define MSG_DIRECTOR_ADDR_KEY				(BAD_CAST "DirectorAddr")

//#define PASS_KEY_LEN						(128)
#define	MSG_CODE_MAX_LEN					(7)
//#define REQ_CODE_TCP_SIZE					(6)
//#define REQ_VALUE_KEY						(64)
#define XML_VALUE_MAX_LENGTH				(256)
/***********************************************************************************/


/***********************************************************************************/
/* ���ڷ�������Ϣ */

#define MSG_RECSERVER_INFO_KEY				(BAD_CAST "RecServerInfo")
#define MSG_SERVER_TYPE_KEY					(BAD_CAST "ServerType")
#define MSG_SERVER_SERIES_KEY				(BAD_CAST "ServerSeries")
#define MSG_SERVER_VERSION_INFO_KEY			(BAD_CAST "ServerVersion")
#define MSG_LANADDR_KEY						(BAD_CAST "LanAddr")
#define MSG_LANGATEWAY_KEY					(BAD_CAST "LanGateWay")
#define MSG_LANNETMASK_KEY					(BAD_CAST "LanNetmask")
#define MSG_LANMAC_KEY						(BAD_CAST "LanMac")

#define MSG_WANADDR_KEY						(BAD_CAST "WanAddr")
#define MSG_WANGATEWAY_KEY					(BAD_CAST "WanGateWay")
#define MSG_WANNETMASK_KEY					(BAD_CAST "WanNetmask")
#define MSG_WANMAC_KEY						(BAD_CAST "WanMac")
#define MSG_MAXROOM_KEY						(BAD_CAST "MaxRoom")
#define MSG_MAXCODENUM_INROOM_KEY			(BAD_CAST "MaxCodecNumInRoom")


#define MSG_ROOMINFO_RESP_KEY				(BAD_CAST "RoomInfoResp")
#define MSG_ROOMINFO_REQ_KEY				(BAD_CAST "RoomInfoReq")
#define MSG_ROOMINFO_KEY					(BAD_CAST "RoomInfo")
#define MSG_ROOMID_KEY						(BAD_CAST "RoomID")
#define MSG_ROOMNAME_KEY					(BAD_CAST "RoomName")
#define MSG_CONNECT_STATUS_KEY				(BAD_CAST "ConnStatus")
#define MSG_RECORD_STATUS_KEY				(BAD_CAST "RecStatus")
#define MSG_RECNAME_KEY						(BAD_CAST "RecName")
#define MSG_RECTIME_KEY						(BAD_CAST "RecTime")

#define MSG_AUDIONFO_KEY					(BAD_CAST "AudioInfo")
#define MSG_INPUTMODE_KEY					(BAD_CAST "InputMode")
#define MSG_SAMPLERATE_KEY					(BAD_CAST "SampleRate")
#define MSG_BITRATE_KEY						(BAD_CAST "Bitrate")
#define MSG_LVOLUME_KEY						(BAD_CAST "Lvolume")
#define MSG_RVOLUME_KEY						(BAD_CAST "Rvolume")
#define MSG_INPUTINDEX_KEY					(BAD_CAST "InputIndex")

#define MSG_ENCINFO_KEY						(BAD_CAST "EncInfo")
#define MSG_ENC_ID_KEY						(BAD_CAST "ID")
#define MSG_ENC_IP_KEY						(BAD_CAST "EncIP")
#define MSG_ENC_STATUS_KEY					(BAD_CAST "Status")
#define MSG_ENC_QUALITYINFO_KEY				(BAD_CAST "QualityInfo")
#define MSG_ENC_BITRATE_KEY					(BAD_CAST "EncBitrate")
#define MSG_ENC_RATETYPE_KEY				(BAD_CAST "RateType")
#define MSG_ENC_WIDTH_KEY					(BAD_CAST "EncWidth")
#define MSG_ENC_HEIGHT_KEY					(BAD_CAST "EncHeight")
#define MSG_ENC_FRAMERATE_KEY				(BAD_CAST "EncFrameRate")
#define MSG_ENC_RESOLUTIONS_KEY				(BAD_CAST "EncResolutions")
#define MSG_ENC_BITRATES_KEY					(BAD_CAST "EncBitrates")

#define MSG_ENC_MUTE_KEY					(BAD_CAST "Mute")

#define MSG_PICTURE_SYNT_MODEL_KEY			(BAD_CAST "Model")
#define MSG_PICTURE_SYNT_SOURCENUM_KEY		(BAD_CAST "SourceNum")
#define MSG_PICTURE_SYNT_RESOLUTION_KEY		(BAD_CAST "Resolution")
#define MSG_LAYOUT_DEF_2_KEY				(BAD_CAST "LayoutDef_2")
#define MSG_LAYOUT_DEF_3_KEY				(BAD_CAST "LayoutDef_3")

#define MSG_CAMERA_CTRL_ADDR_KEY			(BAD_CAST "Addr")











#define MSG_SYS_UPGRADE_REQ_KEY				(BAD_CAST "SysUpgradeReq")

#define MSG_SET_ROOMINFO_REQ_KEY			(BAD_CAST "SetRoomInfoReq")
#define MSG_SET_ROOM_AUDIO_INFO_REQ_KEY		(BAD_CAST "SetRoomAudioReq")


#define MSG_SET_SYS_PARAMS_KEY				(BAD_CAST "SetSysParam")
#define MSG_GET_SYS_PARAMS_KEY				(BAD_CAST "GetSysParam")
#define MSG_RECORD_MAXTIME_KEY				(BAD_CAST "RecordMaxTime")
#define MSG_TIMESERVER_ADDR_KEY				(BAD_CAST "TimeServerAddr")
#define MSG_DISK_MAXSPACE_KEY				(BAD_CAST "DiskMaxSpace")
#define MSG_DISK_AVALIABLESPACE_KEY			(BAD_CAST "DiskAvailableSpace")


#define MSG_VOICEINFO_KEY					(BAD_CAST "VoiceInfo")
#define MSG_VOLUME_KEY						(BAD_CAST "Volume")


#define MSG_RECCTRL_REQ_KEY					(BAD_CAST "RecCtrlReq")
#define MSG_STREAM_REQ_KEY					(BAD_CAST "StrmReq")
#define MSG_REMOTE_CTRL_KEY					(BAD_CAST "RemoteCtrlReq")
#define MSG_GET_REMOTE_CTRL_PRO_KEY			(BAD_CAST "RemoteCtrlGet")
#define MSG_SET_REMOTE_CTRL_PRO_KEY			(BAD_CAST "RemoteCtrlSet")
#define MSG_DEL_REMOTE_CTRL_PRO_KEY			(BAD_CAST "RemoteCtrlDel")
#define MSG_UPLOAD_REMOTE_CTRL_PRO_KEY		(BAD_CAST "UploadRemoteCtrlPro")

#define MSG_GET_ENVVER_INFO_KEY				(BAD_CAST "EncVerReq")
#define MSG_GET_AUDIO_INFO_KEY				(BAD_CAST "GetRoomAudioReq")
#define MSG_PICTURE_SYNTHESIS_KEY			(BAD_CAST "PictureSynthesis")
#define MSG_REPLAY_KEY						(BAD_CAST "Replay")
#define MSG_DIRECTOR_KEY 					(BAD_CAST "Director")

#define MSG_IFRAME_REQ_KEY					(BAD_CAST "IFrameReq")
#define MSG_CONNECT_ROOM_REQ_KEY			(BAD_CAST "ConnectRoomReq")
#define MSG_ADDTITLE_REQ_KEY				(BAD_CAST "AddTitleReq")
#define MSG_GET_VOLUME_REQ_KEY				(BAD_CAST "GetVolumeReq")
#define MSG_AUDIO_MUTE_REQ_KEY				(BAD_CAST "MuteReq")
#define MSG_FILE_DELETE_REQ_KEY				(BAD_CAST "FileDeleteReq")
#define MSG_RECORD_REPORT_KEY				(BAD_CAST "RecReport")
#define MSG_RESULT_KEY						(BAD_CAST "Result")
#define MSG_FILENAME_KEY					(BAD_CAST "FileName")
#define MSG_FILESIZE_KEY					(BAD_CAST "FileSize")
#define MSG_FTP_INFO_KEY					(BAD_CAST "FtpInfo")

#define MSG_LOGO_UPLOADLOG_KEY				(BAD_CAST "UploadLogReq")
#define MSG_LOGO_UPLOADLOG_LEN_KEY			(BAD_CAST "LogoLength")

#define MSG_FILE_DELETE_SUCCESS_KEY			(BAD_CAST "DeleteSuccess")
#define MSG_FILE_DELETE_FAIL_KEY			(BAD_CAST "DeleteFail")





#define MSG_FILE_UPLOAD_PREREQ_KEY			(BAD_CAST "FileUploadPreReq")
#define MSG_FILE_UPLOAD_PRERESQ_KEY			(BAD_CAST "FileUploadPreResp")

#define MSG_FILE_UPLOAD_REQ_KEY				(BAD_CAST "FileUploadedReq")

#define MSG_FILELIST_INFO_KEY				(BAD_CAST "FileListInfo")
#define MSG_FILE_DELETE_RESP_KEY			(BAD_CAST "FileDeleteResp")



#define MSG_FTPMODE_KEY						(BAD_CAST "FtpMode")
#define MSG_FTP_SERVERIP_KEY				(BAD_CAST "FtpServerIP")
#define MSG_FTPPORT_KEY						(BAD_CAST "FtpPort")
#define MSG_FTP_USER_KEY					(BAD_CAST "FtpUser")
#define MSG_FTP_PASSWORD_KEY				(BAD_CAST "FtpPasswd")
#define MSG_FTP_UPLOAD_PATH_KEY				(BAD_CAST "UploadPath")

#define MSG_THRFTP_SERVERIP_KEY				(BAD_CAST "THRFtpServerIP")
#define MSG_THRFTPPORT_KEY					(BAD_CAST "THRFtpPort")
#define MSG_THRFTP_USER_KEY					(BAD_CAST "THRFtpUser")
#define MSG_THRFTP_PASSWORD_KEY				(BAD_CAST "THRFtpPasswd")
#define MSG_THRFTP_UPLOAD_PATH_KEY			(BAD_CAST "THRUploadPath")


#define MSG_SYS_UPGRADE_OPEREATE_KEY		(BAD_CAST "Opereate")
#define MSG_SYS_UPGRADE_LENGTH_KEY			(BAD_CAST "Length")



#define MSG_OPT_TYPE_KEY					(BAD_CAST "OptType")
#define MSG_FILE_KEY						(BAD_CAST "File")
#define MSG_RECCTRL_RESP_KEY				(BAD_CAST "RecCtrlResp")


#define MSG_RECSERVER_STATUS_UPDATE_REQ_KEY	(BAD_CAST "RecServerStatusUpdateReq")
#define MSG_RECSERVER_IP_KEY				(BAD_CAST "RecServerIP")
#define MSG_ROOMSTATUS_KEY					(BAD_CAST "RoomStatus")
#define MSG_ROOMQUALITY_KEY					(BAD_CAST "Quality")
#define MSG_ROOM_RECSTATUS_KEY				(BAD_CAST "RecStatus")
#define MSG_ROOM_RECNAME_KEY				(BAD_CAST "RecName")
#define MSG_ROOM_RECTIME_KEY				(BAD_CAST "RecTime")
#define MSG_ROOM_RECSTARTTIME_KEY			(BAD_CAST "RecStartTime")
#define MSG_ROOM_RECORDID_KEY				(BAD_CAST "RecordID")
#define MSG_ROOM_IFMARK_KEY					(BAD_CAST "IfMark")
#define MSG_ROOM_ENC1_STATUS_KEY			(BAD_CAST "Status1")
#define MSG_ROOM_ENC2_STATUS_KEY			(BAD_CAST "Status2")
#define MSG_ROOM_ENC3_STATUS_KEY			(BAD_CAST "Status3")
#define MSG_ROOM_ENC4_STATUS_KEY			(BAD_CAST "Status4")
#define MSG_ROOM_ENC5_STATUS_KEY			(BAD_CAST "Status5")
#define MSG_ROOM_ENC6_STATUS_KEY			(BAD_CAST "Status6")
#define MSG_ROOM_ENC7_STATUS_KEY			(BAD_CAST "Status7")
#define MSG_ROOM_ENC8_STATUS_KEY			(BAD_CAST "Status8")
#define MSG_ROOM_ENC9_STATUS_KEY			(BAD_CAST "Status9")
#define MSG_PIC_SYNT_MODE_KEY				(BAD_CAST "Model")
#define MSG_PIC_SYNT_WIDTH_KEY				(BAD_CAST "Width")
#define MSG_PIC_SYNT_HEIGHT_KEY				(BAD_CAST "Height")
#define MSG_PIC_SYNT_RATE_KEY				(BAD_CAST "Rate")
#define MSG_PIC_SYNT_FRAMERATE_KEY			(BAD_CAST "FrameRate")


#define MSG_ROOM_PREREC_INFO_REQ_KEY		(BAD_CAST "PreRecInfoReq")
#define MSG_ROOM_RECPATH_KEY				(BAD_CAST "RecPath")

#define MSG_ENCODE_INDEX_KEY				(BAD_CAST "EncodeIndex")

#define MSG_NEW_PLATFORM_LOGIN_REPORT		(BAD_CAST "NewPlatformIP")
















/***********************************************************************************/

#define WARNING_SOURCE_MAX_LENGTH			(24)





/***********************************************************************************/
/* ����ƽ̨���� */
#define PLATFORM_MEDIACENTER				("MediaCenter")
#define PLATFORM_MANAGEPLATFORM				("ManagePlatform")
#define PLATFORM_IPADUSER					("IpadUser")
#define PLATFORM_RECSERVER					("RecServer")
#define PLATFORM_LIVENODE					("LiveNode")
#define PLATFORM_COMCONTROL					("ComControl")
#define PLATFORM_MP4RREPAIR					("Mp4Repair")
#define PLATFORM_WEBCTRL					("WebCtrl")
#define PLATFORM_THIRDCONTROL				("ThirdControl")
#define PLATFORM_ALLPLATFORM				("AllPlatform")
#define PLATFORM_DIRECTOR					("DirectorPlatform")
#define PLATFORM_NETCONTROL                 ("NetControl")
/***********************************************************************************/








/***********************************************************************************/
/* ����¼�Ʋ��� */
#define RECORD_FILE_MAX_ROOMNAME			(512)
#define RECORD_FILE_MAX_FILENAME			(512)
#define RECORD_FILE_MAX_TEACHERNAME			(512)
#define RECORD_FILE_MAX_COURSENAME			(512)

#define RECORD_ROOM_ENC_MAX_QUALITY_NUM		(2)
#define RECORD_ROOM_MAX_ENC_NUM				(9)
#define RECORD_OPERATE_STOP_RECORD			(0)
#define RECORD_OPERATE_START_RECORD			(1)
#define RECORD_OPERATE_PAUSE_RECORD			(2)
#define RECORD_OPERATE_ISSTARTING_RECORD	(3)

#define RECORD_DISK_MIN_AVALIABLE_SPACE		(5*1024)	/*������С�ռ䷧ֵ*/

typedef enum {
	MODIFY_TYPE_DELETE_NODE = 0,	/*ɾ���ڵ�*/
	MODIFY_TYPE_ADD_NODE,			/*��ӽڵ�*/
}ModifyNode_T;

typedef enum {
	WARNID_SHORTAGE_AVALIABLE_SPACE = 1,	/*���̿ռ䲻��*/
	WARNID_ENC_CONNECTION_UNUSUAL,			/*�����������쳣*/
	WARNID_VOLUME_UNUSUAL,					/*������*/
	WARNID_ENC_SOURCE_UNUSUAL,				/*��������Դ*/
	WARNID_FTP_FAILED						/*FTP �ϴ�ʧ��*/
}WarnID_T;

typedef enum {
	LOG_TYPE_AUTH_REQ 			= 30001,	/*��Ȩ*/
	LOG_TYPE_SET_ROOM_INFO 		= 30004,	/*���ý�����Ϣ*/
	LOG_TYPE_SET_QUALITY_INFO 	= 30005,	/*������������*/
	LOG_TYPE_SET_AUDIO_INFO 	= 30006,	/*������Ƶ��Ϣ*/
	LOG_TYPE_GET_FTP_INFO 		= 10006,	/*��ȡFTP ��Ϣ*/
	LOG_TYPE_FTP_TRANS 			= 10007,	/*FTP �ϴ�*/
	LOG_TYPE_SET_RECSERVER_INFO = 30008,	/*����¼��ϵͳ����*/
	LOG_TYPE_RECORD_START 		= 30010,	/*����¼��*/
	LOG_TYPE_RECORD_STOP 		= 30010,	/*ֹͣ¼��*/
	LOG_TYPE_RECORD_PAUSE 		= 30010,	/*��ͣ¼��*/
	LOG_TYPE_REBOOT_RECSERVER 	= 30012,	/*����¼��*/
	LOG_TYPE_ROOM_CONNECT 		= 30015,	/*���ӽ���*/
	LOG_TYPE_ROOM_DISCONNECT 	= 30015,	/*�Ͽ�����*/
	LOG_TYPE_UPLOAD_LOGO 		= 30016,	/*�ϴ�LOGO*/
	LOG_TYPE_UPGRADE_RECSERVER 	= 30017,	/*����¼��ϵͳ*/
	LOG_TYPE_ADD_TITLE 			= 30018,	/*�����Ļ*/
	LOG_TYPE_MUTE_REQ 			= 30020,	/*����*/
	LOG_TYPE_VIDEO_ADJUST_REQ 	= 30021,	/*ͼ��΢��*/
}LogType_T;

#define MSG_FTPFILESLIST_KEY			(BAD_CAST "FtpFilesList")
#define MSG_FTPFILE_KEY					(BAD_CAST "FtpFile")
#define MSG_FTP_TIME_KEY				(BAD_CAST "Time")
#define MSG_FTP_FILENAME_KEY			(BAD_CAST "FileName")
#define MSG_FTP_REASON_KEY				(BAD_CAST "Reason")

#define MSG_WARN_WARNINGS_KEY			(BAD_CAST "Warns")
#define MSG_WARN_WARNING_KEY			(BAD_CAST "Warn")
#define MSG_WARN_SOURCE_KEY				(BAD_CAST "Source")
#define MSG_WARN_ROOMID_KEY				(BAD_CAST "RoomID")
#define MSG_WARN_CODECID_KEY			(BAD_CAST "CodecID")
#define MSG_WARN_ID_KEY					(BAD_CAST "ID")
#define MSG_WARN_TIME_KEY				(BAD_CAST "Time")
#define MSG_WARN_CONTENT_KEY			(BAD_CAST "Content")

#define MSG_LOG_KEY						(BAD_CAST "Log")
#define MSG_LOG_TYPE_KEY				(BAD_CAST "Type")
#define MSG_LOG_TIME_KEY				(BAD_CAST "Time")
#define MSG_LOG_USER_KEY				(BAD_CAST "User")
#define MSG_LOG_ADDR_KEY				(BAD_CAST "Addr")
#define MSG_LOG_CONTENT_KEY				(BAD_CAST "Content")

#define MSG_LOG_SUCC_CONTENT			("Success")
#define MSG_LOG_FAIL_CONTENT			("Fail")

#define WARN_SOURCE_MEDIA_CENTER		"MC"
#define WARN_SOURCE_CAPTURE				"CAP"
#define WARN_SOURCE_TOUCH_CTRL			"TC"
#define WARN_SOURCE_CENTER_CTRL			"CC"
#define WARN_SOURCE_MANAGE_PLF			"MP"
#define WARN_SOURCE_DIRECTOR			"DI"
#define WARN_SOURCE_OTHER_CTRL			"OC"
#define WARN_SOURCE_NET_CTRL            "NC"

#define FTP_FAIL_FILE_LIST_FILE 		"ftp_fail_files_list.xml"

typedef struct _warn_
{
	int8_t ID[XML_VALUE_MAX_LENGTH];
	int8_t Source[XML_VALUE_MAX_LENGTH];
	int8_t RoomID[XML_VALUE_MAX_LENGTH];
	int8_t CodecID[XML_VALUE_MAX_LENGTH];
	int8_t Time[XML_VALUE_MAX_LENGTH];
	int8_t Content[2*XML_VALUE_MAX_LENGTH];
} Warn_t;

typedef struct _log_
{
	int8_t Type[XML_VALUE_MAX_LENGTH];
	int8_t Time[XML_VALUE_MAX_LENGTH];
	int8_t User[XML_VALUE_MAX_LENGTH];
	int8_t Addr[XML_VALUE_MAX_LENGTH];
	int8_t Content[2*XML_VALUE_MAX_LENGTH];
} Log_t;


#define MSG_ROOTID_KEY						(BAD_CAST "RoomID")
#define MSG_RECORDID_KEY					(BAD_CAST "RecordID")
#define MSG_OPTTYPE_KEY						(BAD_CAST "OptType")




/***********************************************************************************/

#define ERRNO_IS_RECORDING					(2)

/***********************************************************************************/





/***********************************************************************************/


#define CONTROL_PROCESS_PARAMS_TABLES		(control_env *penv, con_user *puser, int8_t *recv_buf, \
										int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len, 	\
										int32_t msgcode)



/***********************************************************************************/

typedef struct _roomid_array_
{
	int32_t room_num;
	int32_t roomid[CONTROL_ROOM_SERVER_MAX_USER];
	int32_t data[CONTROL_ROOM_SERVER_MAX_USER];
	int32_t msgtype;
} roomid_array;

typedef struct _warning_report_
{
	uint32_t	warn_id;
	uint32_t	room_id;
	uint32_t	codec_id;
	int8_t		source[WARNING_SOURCE_MAX_LENGTH];
}warning_report;


typedef struct _xml_resolve_msg_heads_
{
	int32_t return_code;
	int8_t msg_code[MSG_CODE_MAX_LEN];
	int8_t pass_key[PASS_KEY_LEN];
} xml_resolve_msg_heads;

typedef struct _user_info_
{
	uint8_t username[XML_USER_NAME_MAX_LENGTH];
	uint8_t password[XML_USER_PASSWORD_MAX_LENGTH];
	uint8_t guest_name[XML_USER_NAME_MAX_LENGTH];
	uint8_t guest_passwd[XML_USER_PASSWORD_MAX_LENGTH];
} user_info;

typedef struct _heart_beat_info_
{
	int32_t			post_time;
	int8_t			post_url[HTTP_SERVER_URL_MAX_LEN];
	int8_t			time_serip[64];
}heart_beat_info;

typedef struct _rec_control_reqst_
{
	uint32_t	RoomID;
	uint32_t	OptType;

	int8_t		RecordID[RECORD_ID_MAX_LENGTH];
	uint8_t		RoomName[RECORD_FILE_MAX_ROOMNAME];
	uint8_t		AliasName[RECORD_FILE_MAX_FILENAME];
	uint8_t		TeacherName[RECORD_FILE_MAX_TEACHERNAME];
	uint8_t		CourseName[RECORD_FILE_MAX_COURSENAME];
} rec_control_reqst;

typedef struct _camera_ctrl_
{
	uint32_t	video0_addr;
	uint32_t	video1_addr;
	uint32_t	video2_addr;
} camera_ctrl;


typedef struct _server_info_
{
	uint8_t		ServerVersion[CONTROL_VERSION_LEN];
	uint8_t		ServerType[CONTROL_VERSION_LEN];
	uint8_t		ServerSeries[CONTROL_VERSION_LEN];

	uint32_t	MaxRoom;
	uint32_t	MaxCodecNumInRoom;

	uint32_t	LanAddr;
	uint32_t	LanGateWay;
	uint32_t	LanNetmask;
	uint8_t		LanMac[24];

	uint32_t	WanAddr;
	uint32_t	WanGateWay;
	uint32_t	WanNetmask;
	uint8_t		WanMac[24];
} server_info;

typedef struct _audio_info_
{
	uint32_t	InputMode;		/* 1-Mic in   0-Line In */
	uint32_t	SampleRate;
	uint32_t	Bitrate;
	uint32_t	Lvolume;
	uint32_t	Rvolume;
	int8_t		InputIndex[5];
} audio_info;

typedef struct _quality_info_
{
	uint32_t	enable;
	uint32_t	RateType;		/* 0-������  1-������ */
	uint32_t	EncBitrate;
	uint32_t	EncWidth;
	uint32_t	EncHeight;
	uint32_t	EncFrameRate;
} quality_info;

typedef struct _pic_synt_info_
{
	int8_t		Model[PICTURE_VALUE_LENGTH];	/* �ϳ�ģʽ */
	uint32_t	SourceNum;						/* Դ���� */
	uint32_t	Resolution;						/* �ֱ��� */
	uint32_t	LayoutDef_2;					/* ������Ĭ�ϵĲ���ID */
	uint32_t	LayoutDef_3;					/* ������Ĭ�ϵĲ���ID */
}pic_synt_info;


typedef struct _enc_info_
{
	uint32_t		ID;				/* ����������ֵ */
//	uint32_t		EncIP;			/* ������IP */
	uint32_t		Status;			/* ������״̬ 0-�쳣 1-���� */
	uint32_t		mute;			/* �Ƿ��� 0-�Ǿ��� 1-���� */
	uint32_t		vstatus;		/* �Ƿ���Դ 0-��Դ 1-��Դ */

	int8_t			EncIP[IPINFO_MAX_LENGTH];			/* ������IP */

	quality_info	HD_QuaInfo;
	quality_info	SD_QuaInfo;
} enc_info;

typedef struct _room_info_
{
	uint32_t	RoomID;			/* ������IDֵ */
	uint32_t	ConnStatus;		/* ����״̬ 0���쳣 1������ */
	uint32_t	RecordMaxTime;	/* ¼�����ʱ�� */
	uint32_t	RecStatus;		/* ¼��״̬ 0��ֹͣ 1��¼�� 2����ͣ */
	uint32_t	IfMark;			/* ���̿���״̬ */
	uint32_t	Mode;			/*�ϳ�ģʽ*/

	uint8_t		RoomName[ROOM_NAME_MAX_LENGTH];
	uint8_t		RecName[RECORD_FILE_MAX_FILENAME];	/* ¼������ */
	uint8_t		RecTime[16];
	uint8_t		RecStartTime[32];
	uint8_t		RecordID[132];
	pic_synt_info	PicSyntInfo;
	audio_info	AudioInfo;

	enc_info	EncInfo[RECORD_ROOM_MAX_ENC_NUM];
} room_info;

typedef struct _ftp_info_
{
	uint16_t	Mode;
	uint16_t	MCFTPPort;
	uint8_t		MCFTPAddr[24];
	uint8_t		MCFTPUserName[FTP_MAX_USERNAME_LENGTH];
	uint8_t		MCFTPPassword[FTP_MAX_PASSWD_LENGTH];
	uint8_t		MCFTPPath[FTP_MAX_FTPPATH_LENGTH];

	uint16_t	THRFTPPort;
	uint8_t		THRFTPAddr[24];
	uint8_t		THRFTPUserName[FTP_MAX_USERNAME_LENGTH];
	uint8_t		THRFTPPassword[FTP_MAX_PASSWD_LENGTH];
	uint8_t		THRFTPPath[FTP_MAX_FTPPATH_LENGTH];
} ftp_info;

typedef struct _sys_info_
{
	uint32_t	TimeServerAddr;
 	uint32_t	DiskMaxSpace;
	uint32_t	DiskAvailableSpace;
} sys_info;

typedef struct _config_info_
{
	int32_t language;
	int32_t Laniptype;
	int32_t Waniptype;
	uint32_t media_addr;
	uint32_t manager_addr;
	uint32_t director_addr;
} config_info;

typedef struct _all_server_info_
{
	user_info		Authentication;
	camera_ctrl		CamCtrlInfo;
	heart_beat_info	HBeatInfo;
	ftp_info		FtpInfo;
	sys_info		SysInfo;
	server_info		ServerInfo;
	config_info     ConfigInfo;
	room_info		RoomInfo[CONTROL_ROOM_SERVER_MAX_USER];
	pthread_mutex_t	info_m;
	pthread_mutex_t	ftp_file_m; /*add for read/write ftp fail file*/
	parse_xml_t *parse_xml_ftp; /*save ftp upload fail file list*/
} all_server_info;

typedef struct _record_status_
{
	int32_t RoomID;
	int32_t	Result;
	int32_t	OptType;
	int32_t userid;
	int32_t RecStatus;
	platform_em	platform;
	/* FIXME: �ļ��������Ƿ����·�����������ط������ļ����ĳ����Ƿ�һ�� */
	int8_t	FileName[RECORD_FILE_MAX_FILENAME];
	int8_t	RecordID[RECORD_ID_MAX_LENGTH];
} record_status;

typedef struct _update_info_
{
	uint32_t Opereate;
	uint32_t Length;
} update_info;


/***********************************************************************************/






/********************************************************************************************/
int32_t package_add_xml_leaf(xmlNodePtr father_node, const xmlChar *key_name,
                             const int8_t *key_value);
int32_t resolve_msgcode_and_passkey_or_returncode(const int8_t *xml_buf, con_user *puser,
        int32_t *msgcode, int8_t *passkey,
        int32_t *ret_code, int32_t *msgtype);
int32_t package_http_heart_report_xml_data(int8_t *send_buf, all_server_info *pser, int32_t msgcode);
int32_t init_all_server_info(all_server_info *pinfo);
int32_t package_upload_status_report_xml_data(int8_t *out_buf, platform_em platform,
											int32_t msgcode, file_user *pfile_user);
int32_t stream_request_cleanup_lives_user_info(control_env *penv, con_user *puser);
int32_t report_same_platform_login(con_user *old_puser, con_user *pnew_user);
int32_t warning_report_sending(control_env *penv, con_user *puser, warning_report *preport);
int32_t check_all_record_status(server_set *pser);
int32_t clean_user_live_user_info(control_env *penv, int32_t userid);
/********************************************************************************************/



/********************************************************************************************/
/* ���������ĺ��� */

int32_t login_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t checkuser_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t get_server_info_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t get_room_info_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t set_room_info_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t set_room_quality_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t set_room_audio_info_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t rec_control_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t get_ftpupload_info_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t set_server_sys_params_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t get_server_sys_params_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t rec_control_process CONTROL_PROCESS_PARAMS_TABLES;

int32_t stream_request_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t reboot_server_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t camera_control_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t iframe_request_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t connect_room_request_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t upload_logo_picture_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t update_server_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t add_text_title_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t get_audio_volume_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t set_audio_mute_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t video_adjust_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t file_delete_report_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t get_filelist_info_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t report_record_status_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t get_prerec_info_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t livenode_heart_beat_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t room_heart_report_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t room_encinfo_report_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t get_camctl_pro_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t set_camctl_pro_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t upload_camctl_pro_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t delete_camctl_pro_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t get_encver_info_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t get_audio_info_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t picture_synthesis_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t picture_replay_process CONTROL_PROCESS_PARAMS_TABLES;
int32_t studio_director_info_process CONTROL_PROCESS_PARAMS_TABLES;

/********************************************************************************************/



/********************************************************************************************/
/* ����ת���ĺ��� */
int32_t forward_process(void *user_src, void *user_des, int8_t *send_buf, int32_t msgcode,
                        int8_t *ret_buf, int32_t *ret_len);

/********************************************************************************************/


/********************************************************************************************/
/*FTP �澯�ϱ���غ���*/
int32_t modify_ftp_fail_file_node(parse_xml_t **parse_xml_cfg, int8_t *ftp_fail_file, int8_t *ftp_fail_reason, ModifyNode_T opt);
int8_t *get_file_name_from_path(int8_t *path, int8_t *file);

/*��־�ϱ��������*/
int32_t package_log_report_xml_data(int8_t *out_buf, const Log_t *log_info, const int8_t *pass_key);

int32_t report_log_info_process(server_set *pserset, const Log_t *log_info);
int8_t *warn_get_current_time_info(int8_t *c_time);
void check_ftp_file(int8_t *file);

/********************************************************************************************/


#endif

