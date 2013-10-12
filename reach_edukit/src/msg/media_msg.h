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
#define MSG_SCREEN_DATA 					(7)		//��tcpͨѶ����Ϣͷ����Ƶ��־
#define MSG_AUDIO_DATA					(6)		//��tcpͨѶ����Ϣͷ����Ƶ��־
#define MSG_ABNORMAL_DISCONNECT                 (2) 		// ���¼���쳣�Ͽ�
#define MSG_FILE_END						(1)		//�㲥�����ļ���β
#define	T_FILE	"video_test.h264"

#define OPERATION_CONTINUE				(1) 	// ��������
#define OPERATION_SUCC					(0)  	//�����ɹ�����
#define OPERATION_ERR					(-1)	//����ʧ������
#define PASSWD_VERIFICATION_ERR			(-2)	//������֤����
#define CREATE_THREAD_ERR				(-3)	//�����߳�ʧ��
#define TRANSMISSION_DATA_ERR			(-4)	//��������ʧ��
#define OPEN_DEV_ERR						(-5)	//���豸ʧ��
#define MESSAGE_TYPE_ERR					(-6)	//��Ϣ���ʹ���
#define SERVER_IS_CLOLSED				(-8) // ¼�������ر�
#define SERVER_IS_CLOLSED_UNUSUAL 		(-9) //¼���쳣�ر� 
#define	FALSE	0
#define	TRUE	1
#define I_FRAME								(0x10)
typedef struct vod_file_infos
{
	int32_t nFileTime;          //�ļ�����ʱ��
	
	int32_t nVideoCount;        //��Ƶ ����
	
	int32_t pnVideoNumber; //��Ƶ �����
	int32_t pnVideoWidth;      //��Ƶ ��
	int32_t pnVideoHeigh;      //��Ƶ ��
	int32_t pnVideoColors; //��Ƶ ɫ��
	int32_t pnVideoCodec;      //��Ƶ �����ʽ
	
	int8_t bAudio;          //��Ƶ 0û�� 1��
	int32_t nAudioCodec;        //��Ƶ �����ʽ
	int32_t nAudioSampleRate;   //��Ƶ ������
	int32_t nAudioBitRate;      //��Ƶ ����
	
	int32_t nReserve;           //�����ֽڳ���
	int32_t pReserve;         //�����ֽ�
} vod_file_info_t;

typedef enum _dw_segment
{
    MIDDLE_FRAME, 		//0��ʾ�м��
    LAST_FRAME,			//1��ʾ��β��
    START_FRAME,		//2��ʾ��ʼ��
    INDEPENDENT_FRAME	//3��ʾ������
} dw_segment_t;

/*UDP NetWork Header*/
typedef struct hdb_freame_heads
{
	uint32_t m_id;			//= mmioFOURCC('R','Q','H','D');
	uint32_t m_time_tick;    //ʱ���
	uint32_t m_frame_length; //֡���ݳ���
	uint32_t m_data_codec;   //���뷽ʽ
	uint32_t m_frame_rate;   //����֡�� �� ��Ƶ������
	uint32_t m_width;       //���
	uint32_t m_hight;       //�߶�
	uint32_t m_colors;      //��ɫ��
	uint32_t m_dw_segment;		//�����ʾ 0��ʾ�м�� 1��ʾ��β�� 2��ʾ��ʼ�� 3��ʾ������
	uint32_t m_dw_flags;			//֡��־ I֡��
	uint32_t m_dw_packet_number; 	//
	uint32_t m_others;      			//����Ӧ�������
} hdb_freame_head_t;

#define FH_LEN	  sizeof(hdb_freame_head_t)
#define I_FRAME									(0x10)
#define EDU_MSG_VER	2012
typedef struct msg_headers  //��Ϣͷ
{
	uint16_t m_len;		//����
	uint16_t m_ver;		//�汾
	uint16_t m_msg_type;	//��Ϣ����
	uint16_t m_data;	//����
} msg_header_t;

typedef struct parse_datas
{
	int32_t index;//��ʦid
	int32_t data_len;//֡����
	uint8_t *data;
	int32_t flags;//�Ƿ�i֡
	int32_t sindex;//�ڼ�·
	int32_t audio_sindex;
	int32_t data_type;//�������ͣ�����Ƶ
	int32_t height;
	int32_t width;
	int32_t sample_rate;
	int64_t time_tick;//ʱ���
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
	��������ֵ��ǲ�ͬ���û�
	���ض����ֵ��ǲ�ͬ��ͨ��
*/
#define PARSE_TO_PKG_SUN_PATH		"/tmp/parse_to_pkg_sun_path.str"
#ifdef VOD_SERVICE
#define LOACAL_DATA_MSG				"/tmp/vod_local_data_msg.str"	//������������ͨ�ŵ��ļ�·��,���ںͷ��Ͷ�ͨ��
#else
#define LOACAL_DATA_MSG				"/tmp/live_local_data_msg.str"	//������������ͨ�ŵ��ļ�·��,���ںͷ��Ͷ�ͨ��
#define TO_MEDIA_FRAME_SUN_PATH		"/tmp/to_media_frame_sun_path.str"
#define TO_RECORD_SUN_PATH			"/tmp/to_record_sun_path.str"
#endif
#define MSG_TYPE_LOGIN				1	//��¼��MsgHeader + UserInfo.
#define MSG_TYPE_DATAREQ			14	//�Ƿ���Ҫ����������ý������MsgHeader+������ID(BYTE)+�Ƿ���Ҫý������(BYTE) 1��ʾ��Ҫ��0��ʾ����Ҫ��
#define MAX_CUR_TO_END		10
#define MSG_TYPE_FIREWALL  			7	//��͸����ǽ MsgHeader+���ֽ�ID��
#define	EV_TIMEOUT	-1

#define USER_LOGIN_MAXUSERS	-2	//�û�����
#define USER_LOGIN_ERROR	-1	//Э�鲻��
#define USER_LOGIN_VALID	1	//��֤ͨ��
#define USER_LOGIN_INVALID	0	//�޴��û�������ʧ��

//web_protocol_resolve.c
#define WEB_TCP_LISTEN_PORT		(3008)		//����webͨ�ŵĶ˿�
#define HTTP_ALARM_TIME           			 3	        //ֱ���ڵ㶨ʱ�ϱ�ý������ʱ����
#define UDPBUFFERSZIE   					(655360) 	//����udp��buffer����

#define	USER_NAME               "reach"
#define	USER_PASSWD     "reachplayer"
#define	UNITE_PASSWD     "reachserver"
#define MAX_STREAMS 7
//*********************************POWERLIVE PLATFROM ***********************
#define	LIVE_CONNECT_KEY_1			"Manager2006"		// ��������1
#define	LIVE_CONNECT_KEY_2			"Test2006"			// ��������2
#define 	LIVE_SERVER_VERSION 		"6.0.51.0"				// ����˰汾
#define 	LIVE_MINPLAYER_VERSION		"4.1.8"				// 
#define	LIVE_MSG_UDPSOCKET_PORT	7100				//powerlive msg UDPsocket�Ķ˿�
#define    LIVE_DATA_UDPSOCKET_PORT 	7200				//powerlive data UDPsocket�Ķ˿�
#define	MAX_MSG_UDPSOCKET		6				//powerlive msg UDPsocket���������
#define	SERV_POWERLIVE_PORT	3000
/**********************************************/
#define	MAKEFOURCC(ch0, ch1, ch2, ch3)                      \
		((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |   \
		((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))

#define	VOD_CONNECT_KEY_1			"RecVod2006"		// �㲥��������1
#define	VOD_CONNECT_KEY_2			"123"			// �㲥��������2
#define 	VOD_MSG_UDPSOCKET_PORT 	9100				//powerlive msg UDPsocket�Ķ˿�
#define    VOD_DATA_UDPSOCKET_PORT 	9200				//powerlive data UDPsocket�Ķ˿�
#define	VOD_MAX_MSG_UDPSOCKET		6				//powerlive msg UDPsocket���������
#define	VOD_SERV_POWERLIVE_PORT	3300
#define	H264_CODEC_TYPE   			MAKEFOURCC('H','2','6','4')
#define	AAC_CODEC_TYPE    			MAKEFOURCC('A','D','T','S')
#define 	RQHD_CODEC_TYPE			MAKEFOURCC('R','Q','H','D')
#define JPEG_CODEC_TYPE 			MAKEFOURCC('J','P','E','G')

#define	CODEC_COLORS				24
#define	MSG_VOD_PWD             			16      //�㲥����  ��ӦsData=0 OK sData=1 ���� sData=2 û�е㲥����
#define 	MSG_VOD_PLAY_FILE       		18      //�㲥�ļ� ��ӦsData=0 OK(���FileInfo) sData=1����
#define 	MSG_VOD_STOP_FILE       		19      //ֹͣ����
#define	MSG_VOD_PAUSE_FILE     		20      //��ͣ����
#define 	MSG_VOD_GOTO_FILE       		21      //����ָ����ʱ�䲥��
#define 	MSG_VOD_HEART           			22      //������Ϣ
#define	MSG_VOD_RESUME_FILE        	23      //��ͣ��ָ�����
#define	MSG_VOD_PLAY_END			24	//���Ž���
#define	MSG_VOD_MARK_INFO			25	  //����Mark��Ϣ


/**********************************************/
#define MSG_VER		       				1
#define MSG_TYPE_INFO	   			1
#define MSG_TYPE_PASSWORD  			3
#define MSG_TYPE_CTRL	   			30 		//Զң��Ϣ
#define MSG_TYPE_TITLE	   			31 		//��Ļ��Ϣ
#define MSG_TYPE_PIC	   				32 		//��ͼ������(ɫ�����ȵ�)
#define MSG_TYPE_GETINFO   			33		//��ȡ��Ϣ���LAUNET_VIPARAM
#define MSG_TYPE_MARK	   			34		//���ʵʱ˵��������ұ�ע
#define MSG_TYPE_ID		   			35		//�û�ID
#define MSG_TYPE_DATA				36		//VGA����

#define MSG_TYPE_USEINFO				40		//RecPlayer�û����� �û����� 1 RecServer, 0 Player
#define MSG_TYPE_USERLIST			41		//�û��б�0 �����б� 1�¼��û���2�û��˳�
#define MSG_TYPE_BDPROXY			42		//�鲥���� 0 �ر� 1��
#define MSG_TYPE_BDSTATUS			43		//��ѯ�鲥����״̬ + Int > 0 �����ѯ��UserID 0 Ϊ���С�
#define MSG_TYPE_CHAT				44		//��������
#define MSG_TYPE_TRANSFILE			45		//�����ļ���
#define MSG_TYPE_CALLNAME			46		//����
#define MSG_TYPE_CLOSEUSER			47		//�ر��û� �� UserID
#define MSG_TYPE_CHATMNG			48		//������رջػ�

#define MSG_TYPE_BOXINFO			50		//BOX������Ϣ
#define MSG_TYPE_HEART				51		//������Ϣ
// *************************************************************************

typedef enum USER_FLAGe
{
    USER_FLAG_PLAYER,               //?��2����??��
    USER_FLAG_UNITE,                //??��a��??��
    USER_FLAG_MANAGER,              //?������1������??����
    USER_FLAG_LCD,                  //LCD
    USER_FLAG_WEB,                  //WEB
    USER_FLAG_OTHER,                //????��a2?3��D��
    USER_FLAG_DEC                   //?a???��
} USER_FLAG;

typedef struct net_user_infos
{
	int8_t uname[20];				//�û���
	int8_t upasswd[20];			//����
	USER_FLAG uflag;
} net_user_info_t;

#define	MAX_CH		1024
#define 	MSG_HEAD_LEN	sizeof(msg_header_t)


#define 	POWOLIVE_PLATFORM		0
#define 	BEYONDSYS_PLATFORM 	1



//���ͷ��������ַ���������Ϣ
typedef struct channel_infos
{
	uint16_t 	m_msg_type;		//��Ϣ����,��ʾ����Ƶ������Ƶ����
	int32_t	m_channel;		//���ұ��,��ͨ�����               ֱ������ͨ���ĸ���
	int32_t	m_sindex;			//һ������·���				·�ĸ���!!!!
	void		*m_data;			//���ݵ�ַ��ָ��
	uint16_t	m_data_len;		//���ݳ���
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

