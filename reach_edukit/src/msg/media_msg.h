#ifndef	__MEDIA_MSG_H__
#define	__MEDIA_MSG_H__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stdint.h"
#include <stdint.h>
#include <errno.h>
#include "nslog.h"
#if	VOD_SERVICE_STATUS
#define VOD_SERVICE
#endif
#define MAX_SINDEX	6
#define MSG_SCREEN_DATA 					(7)		//在tcp通讯中消息头中视频标志
#define MSG_AUDIO_DATA					(6)		//在tcp通讯中消息头中音频标志
#define MSG_ABNORMAL_DISCONNECT                 (2) 		// 标记录播异常断开
#define MSG_FILE_END						(1)		//点播数据文件到尾
#define	T_FILE	"video_test.h264"

#define OPERATION_CONTINUE				(1) 	// 操作继续
#define OPERATION_SUCC					(0)  	//操作成功跳出
#define OPERATION_ERR					(-1)	//操作失败跳出
#define PASSWD_VERIFICATION_ERR			(-2)	//密码验证错误
#define CREATE_THREAD_ERR				(-3)	//创建线程失败
#define TRANSMISSION_DATA_ERR			(-4)	//传输数据失败
#define OPEN_DEV_ERR						(-5)	//打开设备失败
#define MESSAGE_TYPE_ERR					(-6)	//消息类型错误
#define SERVER_IS_CLOLSED				(-8) // 录播正常关闭
#define SERVER_IS_CLOLSED_UNUSUAL 		(-9) //录播异常关闭 
#define	FALSE	0
#define	TRUE	1
#define I_FRAME								(0x10)
typedef struct vod_file_infos
{
	int32_t nFileTime;          //文件播放时间
	
	int32_t nVideoCount;        //视频 总数
	
	int32_t pnVideoNumber; //视频 流序号
	int32_t pnVideoWidth;      //视频 宽
	int32_t pnVideoHeigh;      //视频 高
	int32_t pnVideoColors; //视频 色彩
	int32_t pnVideoCodec;      //视频 编码格式
	
	int8_t bAudio;          //音频 0没有 1有
	int32_t nAudioCodec;        //音频 编码格式
	int32_t nAudioSampleRate;   //音频 采样率
	int32_t nAudioBitRate;      //音频 带宽
	
	int32_t nReserve;           //保留字节长度
	int32_t pReserve;         //保留字节
} vod_file_info_t;

typedef enum _dw_segment
{
    MIDDLE_FRAME, 		//0表示中间包
    LAST_FRAME,			//1表示结尾包
    START_FRAME,		//2表示开始包
    INDEPENDENT_FRAME	//3表示独立包
} dw_segment_t;

/*UDP NetWork Header*/
typedef struct hdb_freame_heads
{
	uint32_t m_id;			//= mmioFOURCC('R','Q','H','D');
	uint32_t m_time_tick;    //时间戳
	uint32_t m_frame_length; //帧数据长度
	uint32_t m_data_codec;   //编码方式
	uint32_t m_frame_rate;   //数据帧率 或 音频采样率
	uint32_t m_width;       //宽度
	uint32_t m_hight;       //高度
	uint32_t m_colors;      //颜色数
	uint32_t m_dw_segment;		//组包标示 0表示中间包 1表示结尾包 2表示开始包 3表示独立包
	uint32_t m_dw_flags;			//帧标志 I帧？
	uint32_t m_dw_packet_number; 	//
	uint32_t m_others;      			//包对应窗口序号
} hdb_freame_head_t;

#define FH_LEN	  sizeof(hdb_freame_head_t)
#define I_FRAME									(0x10)
#define EDU_MSG_VER	2012
typedef struct msg_headers  //消息头
{
	uint16_t m_len;		//长度
	uint16_t m_ver;		//版本
	uint16_t m_msg_type;	//消息类型
	uint16_t m_data;	//保留
} msg_header_t;

typedef struct parse_datas
{
	int32_t index;//教师id
	int32_t data_len;//帧长度
	uint8_t *data;
	int32_t flags;//是否i帧
	int32_t sindex;//第几路
	int32_t audio_sindex;
	int32_t data_type;//数据类型，音视频
	int32_t height;
	int32_t width;
	int32_t sample_rate;
	int64_t time_tick;//时间戳
	int32_t end_flag;
	int32_t blue_flag;
	int32_t code_rate;
} parse_data_t;

typedef enum data_codecs
{
    R_VIDEO,
    R_AUDIO,
    R_JPEG,
    R_XML,
    R_MAX_TYPE
} codec_t;



/*
	网络端区分的是不同的用户
	本地端区分的是不同的通道
*/
#define PARSE_TO_PKG_SUN_PATH		"/tmp/parse_to_pkg_sun_path.str"
#ifdef VOD_SERVICE
#define LOACAL_DATA_MSG				"/tmp/vod_local_data_msg.str"	//创建虚拟网络通信的文件路径,用于和发送端通信
#else
#define LOACAL_DATA_MSG				"/tmp/live_local_data_msg.str"	//创建虚拟网络通信的文件路径,用于和发送端通信
#define TO_MEDIA_FRAME_SUN_PATH		"/tmp/to_media_frame_sun_path.str"
#define TO_RECORD_SUN_PATH			"/tmp/to_record_sun_path.str"
#endif
#define MSG_TYPE_LOGIN				1	//登录。MsgHeader + UserInfo.
#define MSG_TYPE_DATAREQ			14	//是否需要服务器发送媒体数据MsgHeader+会议室ID(BYTE)+是否需要媒体数据(BYTE) 1表示需要，0表示不需要。
#define MAX_CUR_TO_END		10
#define MSG_TYPE_FIREWALL  			7	//穿透防火墙 MsgHeader+四字节ID号
#define	EV_TIMEOUT	-1

#define USER_LOGIN_MAXUSERS	-2	//用户数满
#define USER_LOGIN_ERROR	-1	//协议不对
#define USER_LOGIN_VALID	1	//验证通过
#define USER_LOGIN_INVALID	0	//无此用户或密码失败

//web_protocol_resolve.c
#define WEB_TCP_LISTEN_PORT		(3008)		//监听web通信的端口
#define HTTP_ALARM_TIME           			 3	        //直播节点定时上报媒体中心时间间隔
#define UDPBUFFERSZIE   					(655360) 	//定义udp的buffer长度

#define	USER_NAME               "reach"
#define	USER_PASSWD     "reachplayer"
#define	UNITE_PASSWD     "reachserver"
#define MAX_STREAMS 7
//*********************************POWERLIVE PLATFROM ***********************
#define	LIVE_CONNECT_KEY_1			"Manager2006"		// 连接密码1
#define	LIVE_CONNECT_KEY_2			"Test2006"			// 连接密码2
#define 	LIVE_SERVER_VERSION 		"6.0.51.0"				// 服务端版本
#define 	LIVE_MINPLAYER_VERSION		"4.1.8"				// 
#define	LIVE_MSG_UDPSOCKET_PORT	7100				//powerlive msg UDPsocket的端口
#define    LIVE_DATA_UDPSOCKET_PORT 	7200				//powerlive data UDPsocket的端口
#define	MAX_MSG_UDPSOCKET		6				//powerlive msg UDPsocket的最大数量
#define	SERV_POWERLIVE_PORT	3000
/**********************************************/
#define	MAKEFOURCC(ch0, ch1, ch2, ch3)                      \
		((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |   \
		((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))

#define	VOD_CONNECT_KEY_1			"RecVod2006"		// 点播连接密码1
#define	VOD_CONNECT_KEY_2			"123"			// 点播连接密码2
#define 	VOD_MSG_UDPSOCKET_PORT 	9100				//powerlive msg UDPsocket的端口
#define    VOD_DATA_UDPSOCKET_PORT 	9200				//powerlive data UDPsocket的端口
#define	VOD_MAX_MSG_UDPSOCKET		6				//powerlive msg UDPsocket的最大数量
#define	VOD_SERV_POWERLIVE_PORT	3300
#define	H264_CODEC_TYPE   			MAKEFOURCC('H','2','6','4')
#define	AAC_CODEC_TYPE    			MAKEFOURCC('A','D','T','S')
#define 	RQHD_CODEC_TYPE			MAKEFOURCC('R','Q','H','D')
#define JPEG_CODEC_TYPE 			MAKEFOURCC('J','P','E','G')

#define	CODEC_COLORS				24
#define	MSG_VOD_PWD             			16      //点播密码  回应sData=0 OK sData=1 错误 sData=2 没有点播功能
#define 	MSG_VOD_PLAY_FILE       		18      //点播文件 回应sData=0 OK(后跟FileInfo) sData=1错误
#define 	MSG_VOD_STOP_FILE       		19      //停止播放
#define	MSG_VOD_PAUSE_FILE     		20      //暂停播放
#define 	MSG_VOD_GOTO_FILE       		21      //跳到指定的时间播放
#define 	MSG_VOD_HEART           			22      //心跳消息
#define	MSG_VOD_RESUME_FILE        	23      //暂停后恢复播放
#define	MSG_VOD_PLAY_END			24	//播放结束
#define	MSG_VOD_MARK_INFO			25	  //发送Mark信息


/**********************************************/
#define MSG_VER		       				1
#define MSG_TYPE_INFO	   			1
#define MSG_TYPE_PASSWORD  			3
#define MSG_TYPE_CTRL	   			30 		//远遥消息
#define MSG_TYPE_TITLE	   			31 		//字幕消息
#define MSG_TYPE_PIC	   				32 		//改图像质量(色度亮度等)
#define MSG_TYPE_GETINFO   			33		//获取信息后跟LAUNET_VIPARAM
#define MSG_TYPE_MARK	   			34		//添加实时说明或会议室备注
#define MSG_TYPE_ID		   			35		//用户ID
#define MSG_TYPE_DATA				36		//VGA数据

#define MSG_TYPE_USEINFO				40		//RecPlayer用户名， 用户种类 1 RecServer, 0 Player
#define MSG_TYPE_USERLIST			41		//用户列表。0 完整列表， 1新加用户、2用户退出
#define MSG_TYPE_BDPROXY			42		//组播代理 0 关闭 1打开
#define MSG_TYPE_BDSTATUS			43		//查询组播代理状态 + Int > 0 代表查询的UserID 0 为所有。
#define MSG_TYPE_CHAT				44		//聊天数据
#define MSG_TYPE_TRANSFILE			45		//传输文件。
#define MSG_TYPE_CALLNAME			46		//点名
#define MSG_TYPE_CLOSEUSER			47		//关闭用户 ＋ UserID
#define MSG_TYPE_CHATMNG			48		//开启或关闭回话

#define MSG_TYPE_BOXINFO			50		//BOX连接信息
#define MSG_TYPE_HEART				51		//心跳消息
// *************************************************************************

typedef enum USER_FLAGe
{
    USER_FLAG_PLAYER,               //?±2￥ó??§
    USER_FLAG_UNITE,                //??áaó??§
    USER_FLAG_MANAGER,              //?áòé1üàí??ì¨
    USER_FLAG_LCD,                  //LCD
    USER_FLAG_WEB,                  //WEB
    USER_FLAG_OTHER,                //????ía2?3ìDò
    USER_FLAG_DEC                   //?a???÷
} USER_FLAG;

typedef struct net_user_infos
{
	int8_t uname[20];				//用户名
	int8_t upasswd[20];			//密码
	USER_FLAG uflag;
} net_user_info_t;

#define	MAX_CH		1024
#define 	MSG_HEAD_LEN	sizeof(msg_header_t)


#define 	POWOLIVE_PLATFORM		0
#define 	BEYONDSYS_PLATFORM 	1



//向发送服务器部分发送数据信息
typedef struct channel_infos
{
	uint16_t 	m_msg_type;		//消息类型,表示是音频还是视频数据
	int32_t	m_channel;		//教室编号,即通道标号               直播本地通道的概念
	int32_t	m_sindex;			//一个教室路标号				路的概念!!!!
	void		*m_data;			//数据地址的指针
	uint16_t	m_data_len;		//数据长度
	uint16_t	m_platform_flag;
} channel_info_t;

#define	CHANNEL_INFO_SIZE	sizeof(channel_info_t)

typedef enum media_platform
{
    POWOLIVE,
    BEYONDSYS
} media_platform_t;



void pack_header_msg(void *data, uint8_t type, uint16_t len , uint16_t m_persist_data);
void pack_header_data(void *data, uint8_t type, uint16_t len);
void pack_header_msg_xml(void *data, uint16_t len , uint16_t ver);
void pack_unite_user_info(net_user_info_t *nu);
int32_t write_file(char *file, void *buf, int buflen);
void init_channel_infos(channel_info_t *cit);
int32_t get_specified_ip(int8_t *ip);
// ************************ BEYONSYS PLATFROM **************************************
int32_t check_net_user_beyonsys(net_user_info_t *nu);
// ************************ POWERLIVE PLATFROM  ***********************************
int32_t check_net_user_powerlive(int8_t *password , int32_t password_len);
int32_t check_vod_user_powerlive(int8_t *password , int32_t password_len);
void get_sun_path(int8_t *sun_path, int8_t *base_sun_path, int32_t index);
#endif	//__MEDIA_MSG_H__

