
#ifndef __ROOM_CTRL_INCLUDE__
#define __ROOM_CTRL_INCLUDE__

#include <pthread.h>
#include "lower_msg_ctrl.h"
#include "upper_msg_ctrl.h"
#include "receive_module.h"
#include "lives.h"
#include "record.h"

#define FILEPREFIX "."

/*Time sleep*/
#define ROOM_MSG_TIME_NUM   			20		/*¼�Ƶȴ�ʱ�����*/
#define ROOM_MSG_RESP_TIME  			250000	/*¼�Ƶȴ���ѯʱ��*/
#define ROOM_MSG_TIME_OUT_VAL  			5		/*��ʱ*/
#define ROOM_MSG_TIME_QUE_NUM  			50		/*��ʱ���д�С*/
#define ROOM_MSG_LOGIN_TIME_OUT_VAL  	10		/*��¼��ʱ*/
#define ROOM_MSG_VOLUME_TIME_OUT_VAL  	2		/*������ʱ*/
#define ENC_STATUS_TIME_OUT_VAL			3		/*ENC ״̬�ϱ����ʱ��*/
#define RECORD_INT_TIME_VAL				15		/*¼�Ƽ��ʱ��*/
#define SERVER_INFO_TIME_OUT			30		/*¼�Ƽ��ʱ��*/

// add zl 
#define SERVER_AUTO_HEART_TIME_OUT		5    /*auto �ϱ�¼����Ϣ30024*/
//��Ϣ����KEY
#define MSG_ROOM_CTRL_BASE_KEY 50000

#define MSG_ROOM_CONF_FILE_PREFIX "RoomConfig"

/* Thread create*/
#define UPPERTHREADCREATED 		0x1
#define LOWERTHREADCREATED 		0x2

/*Return Value*/
#define ROOM_RETURN_SUCC		1
#define ROOM_RETURN_FAIL		0
#define ROOM_RETURN_TIMEOUT	  	2

/*�ߵ��������*/
#define MSG_RATETYPE_HD			0
#define MSG_RATETYPE_SD			1

/*macro*/
#define ROOM_PICSYN_MODE_LEN				10		//add zl

#define ROOM_ENC_NUM						9     // add zl
#define ROOM_STR_NUM						(2 * ROOM_ENC_NUM)
#define ROOM_MSG_VAL_LEN					(128)
#define ROOM_MSG_NOTE_LEN					(1024)
#define ROOM_MSG_MAX_LEN					(4096*2)  // add zl
#define RECORD_FILE_MAX_ROOMNAME			(512)
#define RECORD_FILE_MAX_FILENAME			(512)
#define RECORD_FILE_MAX_TEACHERNAME			(512)
#define RECORD_FILE_MAX_COURSENAME			(512)
#define RECORD_FILE_MAX_NOTE				(512)

#define MSG_MEDIACENTER_PASSKEY				"MediaCenter"
#define MSG_MANAGEPLAT_PASSKEY				"ManagePlatform"
#define MSG_RECORD_PASS						"RecServer"
#define MSG_LIVENODE_PASSKEY					"LiveNode"
#define MSG_COMCTRL_PASSKEY					"ComControl"
#define MSG_THIRDCTRL_PASSKEY				"ThirdControl"
#define MSG_ALLPLATFORM_PASSKEY				"AllPlatform"
#define MSG_ROOMCTRL_PASSKEY				"RoomCtrl"
#define MSG_WEBCTRL_PASSKEY				"WebCtrl"

#define MSG_PLATFORM_DIRECTOR_PASSKEY		("DirectorPlatform")
#define MSG_NETCTRL_PASSKEY                 "NetControl"

// add zl 
#define MSG_ROOM_AUTO_PASSKEY					"ROOM_AUTO_REQ"

#define MSG_RECORD_CTRL_STOP					0
#define MSG_RECORD_CTRL_START					1
#define MSG_RECORD_CTRL_PAUSE					2

#define MSG_MIN_VALID_SPACE						(10 * 1024 * 1024)

/*Э��ͷ�汾��*/
#define MSG_HEAD_VER			2012
#define ROOM_VERSION			"v1.0.0"

enum {
	EncodeType_H264 = 0,
	EncodeType_JPEG,
	EncodeType_INVALID = -1,
};

typedef enum _platform_em_{
	MediaCenter = 0,		/* ý������ */
	ManagePlatform,			/* ����ƽ̨ */
	IpadUser,				/* ipadԤ���û� */
	RecServer,				/* ����*/
	LiveNode,				/* ֱ���ڵ� */
	ComControl,				/* �����п� */
	Mp4Repair,				/* MP4�޸����� */
	WebCtrl,				/* WEB���ƶ� */
	Director,				/*����ƽ̨*/
	NetControl,             /*�����п�*/
	ThirdControl,			/* ���� */
	AllPlatform,			/* ����ƽ̨ */
	PlatformNum,
	InvalidPlatform			/* ��Ч */
}platform_em;



//==================XML protocol macro=========

#define XML_DOC_VERSION						(BAD_CAST "1.0")
#define XML_TEXT_CODE_TYPE					("UTF-8")

#define MSG_ROOM_CFG_KEY					(BAD_CAST "RoomCfg")
#define MSG_ROOM_IP_KEY						(BAD_CAST "IpAddr")
#define MSG_ROOM_PORT_KEY					(BAD_CAST "Port")

#define REQ_ROOT_KEY 						(BAD_CAST "RequestMsg")
#define RESP_ROOT_KEY 						(BAD_CAST "ResponseMsg")

#define MSG_HEAD_KEY						(BAD_CAST "MsgHead")
#define MSG_CODE_KEY						(BAD_CAST "MsgCode")
#define MSG_RETURNCODE_KEY					(BAD_CAST "ReturnCode")
#define MSG_PASSKEY_KEY						(BAD_CAST "PassKey")
#define MSG_BODY_KEY						(BAD_CAST "MsgBody")
#define MSG_USER_KEY						(BAD_CAST "User")
#define MSG_PASSWORD_KEY					(BAD_CAST "Password")
#define MSG_ROOMINFOREQ_KEY					(BAD_CAST "RoomInfoReq")
#define MSG_ROOMINFORESP_KEY				(BAD_CAST "RoomInfoResp")
#define MSG_SETROOMINFOSREQ_KEY				(BAD_CAST "SetRoomInfoReq")
#define MSG_SETAUDIOINFOSREQ_KEY			(BAD_CAST "SetRoomAudioReq")
#define MSG_ROOMID_KEY						(BAD_CAST "RoomID")
#define MSG_ROOMINFO_KEY					(BAD_CAST "RoomInfo")
#define MSG_CONNECTSTATUS_KEY 				(BAD_CAST "ConnStatus")
#define MSG_AUDIOINFO_KEY					(BAD_CAST "AudioInfo")
#define MSG_INPUTMODE_KEY					(BAD_CAST "InputMode")
#define MSG_SAMPLERATE_KEY					(BAD_CAST "SampleRate")
#define MSG_BITRATE_KEY						(BAD_CAST "Bitrate")
#define MSG_LVOLUME_KEY						(BAD_CAST "Lvolume")
#define MSG_RVOLUME_KEY						(BAD_CAST "Rvolume")
#define MSG_ENCINFO_KEY						(BAD_CAST "EncInfo")
#define MSG_ENCIP_KEY						(BAD_CAST "EncIP")
#define MSG_ENCINFOID_KEY					(BAD_CAST "ID")
#define MSG_QUALITYINFO_KEY					(BAD_CAST "QualityInfo")
#define MSG_STATUS_KEY						(BAD_CAST "Status")
#define MSG_RATETYPE_KEY					(BAD_CAST "RateType")
#define MSG_ENCBITRATE_KEY					(BAD_CAST "EncBitrate")
#define MSG_ENCWIDTH_KEY					(BAD_CAST "EncWidth")
#define MSG_ENCHEIGHT_KEY					(BAD_CAST "EncHeight")
#define MSG_ENCFRAMERATE_KEY				(BAD_CAST "EncFrameRate")
#define MSG_ROOMNAME_KEY					(BAD_CAST "RoomName")
#define MSG_SETSYSPARAM_KEY					(BAD_CAST "SetSysParam")
#define MSG_TIMESERVERADDR_KEY				(BAD_CAST "TimeServerAddr")
#define MSG_DISKSPACE_KEY					(BAD_CAST "DiskSpace")
#define MSG_DISKMAXSPACE_KEY				(BAD_CAST "DiskMaxSpace")
#define MSG_DISKAVAILSPACE_KEY				(BAD_CAST "DiskAvailableSpace")
#define MSG_RECCTRLREQ_KEY					(BAD_CAST "RecCtrlReq")
#define MSG_RECCTRLRESP_KEY					(BAD_CAST "RecCtrlResp")
#define MSG_RECORDID_KEY					(BAD_CAST "RecordID")
#define MSG_OPTTYPE_KEY						(BAD_CAST "OptType")
#define MSG_ROOMNAME_KEY					(BAD_CAST "RoomName")
#define MSG_ALIASNAME_KEY					(BAD_CAST "AliasName")
#define MSG_TEACHERNAME_KEY					(BAD_CAST "TeacherName")
#define MSG_COURSENAME_KEY					(BAD_CAST "CourseName")
#define MSG_NOTES_KEY						(BAD_CAST "Notes")
#define MSG_RESULT_KEY						(BAD_CAST "Result")
#define MSG_STREAMREQ_KEY					(BAD_CAST "StrmReq")
#define MSG_REMOTECTRLREQ_KEY				(BAD_CAST "RemoteCtrlReq")
#define MSG_IFRAMEREQ_KEY					(BAD_CAST "IFrameReq")
#define MSG_ENCODEINDEX_KEY					(BAD_CAST "EncodeIndex")
#define MSG_CONNECTROOMREQ_KEY				(BAD_CAST "ConnectRoomReq")
#define MSG_QUALITY_KEY						(BAD_CAST "Quality")
#define MSG_IPADDR1_KEY						(BAD_CAST "Ipaddr1")
#define MSG_IPADDR2_KEY						(BAD_CAST "Ipaddr2")
#define MSG_IPADDR3_KEY						(BAD_CAST "Ipaddr3")

// add zl
#define MSG_PICTURESYN_KEY					(BAD_CAST "PictureSynthesis")
#define MSG_DIRECTORMODE_KEY					(BAD_CAST "Director")
#define MSG_REMOTEGET_KEY						(BAD_CAST "RemoteCtrlGet")
#define MSG_REMOTESET_KEY						(BAD_CAST "RemoteCtrlSet")



// add zl
#define MSG_IPADDR4_KEY						(BAD_CAST "Ipaddr4")
#define MSG_IPADDR5_KEY						(BAD_CAST "Ipaddr5")
#define MSG_IPADDR6_KEY						(BAD_CAST "Ipaddr6")
#define MSG_IPADDR7_KEY						(BAD_CAST "Ipaddr7")
#define MSG_IPADDR8_KEY						(BAD_CAST "Ipaddr8")
#define MSG_IPADDR9_KEY						(BAD_CAST "Ipaddr9")
#define MSG_PORT1_KEY						(BAD_CAST "Port1")
#define MSG_PORT2_KEY						(BAD_CAST "Port2")
#define MSG_PORT3_KEY						(BAD_CAST "Port3")
// add zl
#define MSG_PORT4_KEY						(BAD_CAST "Port4")
#define MSG_PORT5_KEY						(BAD_CAST "Port5")
#define MSG_PORT6_KEY						(BAD_CAST "Port6")
#define MSG_PORT7_KEY						(BAD_CAST "Port7")
#define MSG_PORT8_KEY						(BAD_CAST "Port8")
#define MSG_PORT9_KEY						(BAD_CAST "Port9")

#define MSG_ADDTITLEREQ_KEY					(BAD_CAST "AddTitleReq")
#define MSG_UPLOADLOGREQ_KEY			 	(BAD_CAST "UploadLogReq")
#define MSG_GETVOLUMEREQ_KEY			  	(BAD_CAST "GetVolumeReq")
#define MSG_MUTEREQ_KEY						(BAD_CAST "MuteReq")
#define MSG_WARNREQ_KEY						(BAD_CAST "warn")
#define MSG_LOGREQ_KEY						(BAD_CAST "log")
#define MSG_FILENAME_KEY					(BAD_CAST "FileName")
#define MSG_LOGLEVEL_KEY					(BAD_CAST "level")
#define MSG_LOGCONTENT_KEY					(BAD_CAST "content")
#define MSG_WARNID_KEY						(BAD_CAST "id")
#define MSG_WARNSOURCE_KEY					(BAD_CAST "source")
#define MSG_WARNCODECID_KEY					(BAD_CAST "codecid")
#define MSG_ROOMSTATUSREQ_KEY 				(BAD_CAST "RecServerStatusUpdateReq")
#define MSG_RECSERVERIP_KEY 				(BAD_CAST "RecServerIP")
#define MSG_ROOMSTATUS_KEY 					(BAD_CAST "RoomStatus")
#define MSG_RECSTATUS_KEY					(BAD_CAST "RecStatus")
#define MSG_RECNAME_KEY						(BAD_CAST "RecName")
#define MSG_RECTIME_KEY						(BAD_CAST "RecTime")
#define MSG_RECSTARTTIME_KEY					(BAD_CAST "RecStartTime")

#define MSG_IFMARK_KEY						(BAD_CAST "IfMark")
#define MSG_STATUS1_KEY						(BAD_CAST "Status1")
#define MSG_STATUS2_KEY						(BAD_CAST "Status2")
#define MSG_STATUS3_KEY						(BAD_CAST "Status3")
// add zl
#define MSG_STATUS4_KEY						(BAD_CAST "Status4")
#define MSG_STATUS5_KEY						(BAD_CAST "Status5")
#define MSG_STATUS6_KEY						(BAD_CAST "Status6")
#define MSG_STATUS7_KEY						(BAD_CAST "Status7")
#define MSG_STATUS8_KEY						(BAD_CAST "Status8")
#define MSG_STATUS9_KEY						(BAD_CAST "Status9")

#define MSG_TYPE_KEY						(BAD_CAST "Type")
#define MSG_ADDR_KEY						(BAD_CAST "Addr")
#define MSG_SPEED_KEY						(BAD_CAST "Speed")
#define MSG_NUM_KEY							(BAD_CAST "Num")
#define MSG_NAME_KEY 						(BAD_CAST "Name")
#define MSG_SERIALNUM_KEY					(BAD_CAST "SerialNum")
#define MSG_MACADDR_KEY						(BAD_CAST "MacAddr")
#define MSG_IPADDR_KEY						(BAD_CAST "IPAddr")
#define MSG_GATEWAY_KEY						(BAD_CAST "GateWay")
#define MSG_NETMASK_KEY						(BAD_CAST "NetMask")
#define MSG_DEVVER_KEY						(BAD_CAST "DeviceVersion")
#define MSG_CHNNLNUM_KEY					(BAD_CAST "ChannelNum")
#define MSG_PPTIDX_KEY						(BAD_CAST "PPTIndex")
#define MSG_LOWRATE_KEY						(BAD_CAST "LowRate")
#define MSG_ENCODETYPE_KEY					(BAD_CAST "EncodeType")
#define MSG_USERID_KEY						(BAD_CAST "UserID")
#define MSG_GETSYSPARAM_KEY			 		(BAD_CAST "GetSysParam")
#define MSG_MUTE_KEY			 			(BAD_CAST "Mute")

//================MSG Code define=================

//¼���ɼ��豸TcpͨѶ�ӿ�XML-MsgCode��Χ��30001~30099
#define MSG_REC_DEV_START 		30000
#define MSG_GET_ROOM_INFO 		30003
#define MSG_SET_ROOM_INFO 		30004
#define MSG_SET_QUALITY_INFO 	30005
#define MSG_SET_AUDIO_INFO 		30006
#define MSG_SET_SYS_PARAM		30008
#define MSG_GET_SYS_PARAM		30009
#define MSG_RECORD_CTRL			30010
#define MSG_REQUEST_CODE_STREAM 30011
#define MSG_REMOTE_CTRL 		30013
#define MSG_IFRAMEREQ_CTRL 		30014
#define MSG_CONNECT_ROOM_REQ	30015
#define MSG_SEND_LOGO_PIC		30016
#define MSG_ADD_TITLE_REQ		30018
#define MSG_VOLUME_REQ			30019
#define MSG_MUTE_REQ			30020
#define MSG_PICADJUST_REQ		30021
#define MSG_ROOM_STATUS_REQ		30024
#define MSG_ROOM_WARN_REQ		30030
#define MSG_ROOM_LOG_REQ		30031
#define MSG_ENC_STATUSINFO_REQ	30032
#define MSG_ENC_STATUS_REQ		30088
#define MSG_REC_DEV_END			30099

//��������¼������Э��XML-MsgCode��Χ��31001~31099
#define MSG_ENC_DEV_START 		31000
#define MSG_ENC_LOGIN			31001
#define MSG_ENC_RESTART			31002
#define MSG_ENC_UPDATE			31004
#define MSG_ENC_DEV_END			31099

//�ϳɱ�������¼������Э��
#define MSGCODE_GET_CAMCTL_PROLIST	30041
#define MSGCODE_SET_CAMCTL_PRO		30042
#define MSG_ENC_IMAGE_SYNTHESIS		30047
#define MSG_ECN_DIRECTOR_MODE			30060


//Old¼���������ͻ����ҵ��ڲ�ͨѶ�ӿ�, XML-MsgCode��Χ��36001~36099
#define MSG_RECORD_REQ			36001
#define MSG_SYSTEM_REQ			36002

#define MSG_RECORD_REP_KEY		(BAD_CAST "RecReport")
#define MSG_PRERECINFO_REP_KEY	(BAD_CAST "PreRecInfoReq")
#define MSG_RECPATH_REP_KEY		(BAD_CAST "RecPath")
#define MSG_SEVSERIES_REP_KEY	(BAD_CAST "ServerSeries")
#define MSG_RECMAXTIME_REP_KEY	(BAD_CAST "RecordMaxTime")
#define MSG_PICSYNC_MODEL_KEY		(BAD_CAST "Model")




//=======================================

/*��Ϣ���и�ͨ����Ӧ����Ϣ��Ӧ���*/
#define MSG_QUEUE_REC_CLR			0x0
#define MSG_QUEUE_REC_ONE			0x1
#define MSG_QUEUE_REC_TWO			0x2
#define MSG_QUEUE_REC_THR			0x4
#define MSG_QUEUE_REC_FOU			0x8
#define MSG_QUEUE_REC_FIV			0x10
#define MSG_QUEUE_REC_SIX			0x20
#define MSG_QUEUE_REC_SEV			0x40
#define MSG_QUEUE_REC_EIG			0x80
#define MSG_QUEUE_REC_NIN			0x100
#define MSG_QUEUE_REC_TEN			0x200
#define MSG_QUEUE_REC_ELE			0x400
#define MSG_QUEUE_REC_TWE			0x800
#define MSG_QUEUE_REC_ALL			0xFFFFFF

#define MSG_QUEUE_REC_FLG_BIT		30
#define MSG_QUEUE_REC_FLG   		(1 << MSG_QUEUE_REC_FLG_BIT) //��30 λ���ڱ�Ƿ���ֵ, 1-�ɹ���0-ʧ��

/*��Ϣ���ж�Ӧ��������ͨ�����*/
#define MSG_QUEUE_REC_ONE_ID		1
#define MSG_QUEUE_REC_TWO_ID		2
#define MSG_QUEUE_REC_THR_ID		3
#define MSG_QUEUE_REC_FOU_ID		4
#define MSG_QUEUE_REC_FIV_ID		5

/* Cleans up cleanly after a failure */
#define cleanup(x)		\
			status = (x);\
			goto cleanup;

typedef struct _GlobalData_ {
	int32_t         quit;                /* Global quit flag */
	pthread_mutex_t mutex;               /* Mutex to protect the global data */
} rGlobalData;


static inline int32_t rCtrlGblGetQuit(rGlobalData *glb)
{
	int32_t quit;

	pthread_mutex_lock(&glb->mutex);
	quit = glb->quit;
	pthread_mutex_unlock(&glb->mutex);

	return quit;
}

static inline void rCtrlGblSetQuit(rGlobalData *glb)
{
	pthread_mutex_lock(&glb->mutex);
	glb->quit = 1;
	pthread_mutex_unlock(&glb->mutex);
}

static inline void rCtrlGblSetRun(rGlobalData *glb)
{
	pthread_mutex_lock(&glb->mutex);
	glb->quit = 0;
	pthread_mutex_unlock(&glb->mutex);
}


typedef struct _EncLoginInfo_ {
	uint8_t Name[ROOM_MSG_VAL_LEN];
	uint8_t SerialNum[ROOM_MSG_VAL_LEN];
	uint8_t MacAddr[ROOM_MSG_VAL_LEN];
	uint8_t IPAddr[ROOM_MSG_VAL_LEN];
	uint8_t GateWay[ROOM_MSG_VAL_LEN];
	uint8_t NetMask[ROOM_MSG_VAL_LEN];
	uint8_t DeviceVersion[ROOM_MSG_VAL_LEN];
	uint8_t ChannelNum[ROOM_MSG_VAL_LEN];
	uint8_t PPTIndex[ROOM_MSG_VAL_LEN];
	uint8_t LowRate[ROOM_MSG_VAL_LEN];
	uint8_t EncodeType[ROOM_MSG_VAL_LEN];
}EncLoginInfo, *pEncLoginInfo;

typedef struct _RemoteCtrlInfo_ {
	uint32_t ID;
	uint32_t Type;
	uint32_t Addr;
	uint32_t Speed;
	uint32_t Num;
}RemoteCtrlInfo, *pRemoteCtrlInfo;

typedef struct _RoomMsgEnv_ RoomMsgEnv;

typedef struct _MsgTimeQueue_ {
	uint32_t MsgCode;
	uint8_t PassKey[ROOM_MSG_VAL_LEN];
	uint8_t UserId[ROOM_MSG_VAL_LEN];
	MsgHeader head;
	struct  timeval   time;
	int32_t (*time_out_process_fun)(RoomMsgEnv *pRoom, int32_t socket, pthread_mutex_t *mutex, MsgHeader *head, uint32_t msg_code, int8_t *pass_key, int8_t *user_id, int32_t room_id);
}MsgTimeQueue, *pMsgTimeQueue;

typedef struct _audio_info_{
	uint32_t	InputMode;		/* 1-Mic in   0-Line In */
	uint32_t	SampleRate;
	uint32_t	Bitrate;
	uint32_t	Lvolume;
	uint32_t	Rvolume;
}audio_info;

typedef struct _quality_info_{
	uint32_t	RateType;		/* 0-������  1-������ */
	uint32_t	EncBitrate;
	uint32_t	EncWidth;
	uint32_t	EncHeight;
	uint32_t	EncFrameRate;

	uint32_t	Status; 		//��״̬0-�쳣 1-����
	int32_t 	MsgQueType;		//��Ϣ����type
	int32_t 	Socket;			//����Ӧsocket
    pthread_mutex_t *mutex;
	struct  timeval heart_time;	//������ʱ
}quality_info;

typedef struct _enc_info_{
	uint32_t		ID;				/* ����������ֵ */
	uint32_t		EncIP;			/* ������IP */
	uint32_t		Status;			/* ������״̬ 0-�쳣 1-���� */
	quality_info	HD_QuaInfo;
	quality_info	SD_QuaInfo;

	int32_t 		EncodeType;		/*��������ĸ�ʽ0-H264��1-JPEG*/
	EncLoginInfo	LoginInfo;		/*��������¼��Ϣ*/
	uint32_t   		s_Status;		/*0-��Դ  1-��Դ*/
	uint32_t  		s_Mute;			/*0-����  1-������*/
}enc_info;

typedef struct _room_info_{
	uint32_t	RoomId;				/* ������IDֵ */
	uint32_t	ConnectStatus;		/* ����״̬ 0���쳣 1������ */
	uint32_t	RecordStatus;		/* ¼��״̬0��ֹͣ 1��¼�� 2����ͣ */

	uint8_t		RcdName[ROOM_MSG_VAL_LEN];	/* ¼������ */
	uint32_t 	RecordMaxTime;		/*���¼��ʱ��*/

	audio_info	AudioInfo;
	enc_info	EncInfo[ROOM_ENC_NUM];

	// add zl
	uint32_t	PicSynMode;	 /*�ϳ�ģʽ*/
	
	struct  timeval   s_time;		/*��ʱ�ϱ�*/

	int32_t     EncNum;				/*���õı���������*/
	uint32_t    req_stream_mask;		/*�������������*/
	uint32_t    room_info_mask;		/*��ȡ������Ϣ�ı��*/
	uint32_t    valid_stream_mask;	/*��Ч����������*/
	uint32_t    record_stream_mask;	/*¼�Ƶ��������*/
}room_info;

typedef struct RecInfo {
	uint32_t	ServerIp;
	uint8_t		RecName[ROOM_MSG_VAL_LEN];

	uint8_t 	RecordID[ROOM_MSG_VAL_LEN];	//¼��ID��Ϊ����¼������
	uint32_t 	OptType;	//0��ֹͣ¼�� 1������¼�� 2����ͣ¼��
	uint8_t		RoomName[ROOM_MSG_VAL_LEN];
	uint8_t 	AliasName[ROOM_MSG_VAL_LEN];
	uint8_t		TeacherName[ROOM_MSG_VAL_LEN];
	uint8_t 	CourseName[ROOM_MSG_VAL_LEN];
	uint8_t		Notes[ROOM_MSG_NOTE_LEN];

	uint8_t 	RecPath[ROOM_MSG_VAL_LEN];
	uint8_t 	ServerSeries[ROOM_MSG_VAL_LEN];
	uint32_t 	RecordMaxTime;
	struct  timeval RecordStopTime;	//¼��ֹͣʱ��

	int8_t		userid[ROOM_MSG_VAL_LEN];

	int32_t		opt_mutex_lock; //0����¼�Ʋ���״̬ ,  ���㡪¼�Ʋ�����
}RecInfo, *pRecInfo;

typedef struct RecStatusInfo {
	uint32_t	RoomID;
	uint32_t	ConnStatus;

	uint32_t 	Quality;
	uint32_t 	RecStatus;
	uint8_t		RecName[ROOM_MSG_VAL_LEN];
	uint32_t 	IfMark;
	uint32_t	Status1;
	uint32_t 	Status2;
	uint32_t	Status3;
	uint32_t	Status4;
	uint32_t 	Status5;
	uint32_t	Status6;
	uint32_t	Status7;
	uint32_t 	Status8;
	uint32_t	Status9;
}RecStatusInfo, *pRecStatusInfo;

typedef struct StrmReq {
	uint32_t Quality;
	int8_t   EncodeIndex[ROOM_MSG_VAL_LEN];
	uint32_t Ipaddr1;
	uint32_t Port1;
	uint32_t Ipaddr2;
	uint32_t Port2;
	uint32_t Ipaddr3;
	uint32_t Port3;
	uint32_t Ipaddr4;
	uint32_t Port4;
	uint32_t Ipaddr5;
	uint32_t Port5;
	uint32_t Ipaddr6;
	uint32_t Port6;
	uint32_t Ipaddr7;
	uint32_t Port7;
	uint32_t Ipaddr8;
	uint32_t Port8;
	uint32_t Ipaddr9;
	uint32_t Port9;
}StrmReq, *pStrmReq;

typedef struct WarnInfo{
	uint32_t 	id;
	int8_t   	source[ROOM_MSG_VAL_LEN];
	int32_t  	room_id;
	int32_t		codec_id;
}WarnInfo, *pWarnInfo;

typedef struct _ServerInfo_{
	uint32_t 	TimeServerAddr;
	uint32_t   	DiskMaxSpace;
	uint32_t  	DiskAvailableSpace;

	struct  timeval   time;

	struct  timeval	   time_auto;	
}ServerInfo, *pServerInfo;

struct _RoomMsgEnv_ {
	UpperMsgEnv upperEnv;		//�ϲ�TCP  ��Ϣ
	LowerMsgEnv lowerEnv;		//�²�MSG QUEUE  ��Ϣ

	pthread_t 	pUpper;			//�ϲ�TCP �����߳�
	pthread_t 	pLower;			//�²�MSG QUEUE �����߳�

	MsgTimeQueue msgTimeQue[ROOM_MSG_TIME_QUE_NUM];	//��Ϣ��ʱ����
	pthread_mutex_t   msgQueMutex;

	uint32_t 	initMask;		//��ʼ�����
	uint32_t 	loginMask;		//��¼���

	uint32_t	record_mask_sum[PlatformNum];  // ¼�Ʊ�� ZL
	uint32_t 	record_mask[PlatformNum];	//¼�Ʊ��
	uint32_t 	roomInfoMask[PlatformNum];	//�������ϱ���ȡ��������Ϣ��Ӧ���
	uint32_t 	qualityMask[PlatformNum];	//�������ϱ����øߵ�������Ϣ��Ӧ���
	uint32_t    audioMask[PlatformNum];      //��Ƶ��Ϣ���
	uint32_t    remoteCtrlMask[PlatformNum];	//�ȴ�Զҡ��Ӧ���
	uint32_t    iframeMask[PlatformNum];		//�ȴ�I ֡��Ӧ���
	uint32_t    logoMask[PlatformNum];		//�ȴ�logo �ϴ���Ӧ���
	uint32_t    titleMask[PlatformNum];		//�ȴ���Ļ ��Ӧ���
	uint32_t    adjustMask[PlatformNum];		//�ȴ�ͼ��΢�� ��Ӧ���
	uint32_t    volumeMask[PlatformNum];		//�������
	uint32_t    muteMask[PlatformNum];		//�������
	uint32_t    setRemoteCtrlMask[PlatformNum];		//30042
	uint32_t    getRemoteCtrlMask[PlatformNum];		//30041
	uint32_t    setRemoteModeMask[PlatformNum];		//30047 ���
	uint32_t    getPictureSynMask[PlatformNum];		//30060


	uint32_t 	StrmReqMask[PlatformNum];	//�������ϱ�����������Ӧ���
	uint32_t 	StrmReqStatus[PlatformNum];	//ֱ��ģ�������Ӧ״̬

	recv_room_handle *handle;			//����ģ����
	lives_mode_hand_t *live_handle;		//ֱ��ģ����
	course_record_t *record_handle;		//¼��ģ����
	course_record_t *usb_record_handle;		//¼��ģ����  // zl

	
	room_info 	RoomInfo;		//��������Ϣ
	RecInfo		RecInfo;		//¼�ƿ�����Ϣ
	StrmReq		StrmReq;		//����������Ϣ
	RecStatusInfo RecStatusInfo;//״̬�ϱ���Ϣ

	ServerInfo	server_info;	//��������Ϣ
	rGlobalData glb;			//�˳����
} ;


#endif /*__ROOM_CTRL_INCLUDE__*/

