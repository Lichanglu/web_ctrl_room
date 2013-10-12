#ifndef __FILE_WEB_PROTOCOL_MEETING_H
#define __FILE_WEB_PROTOCOL_MEETING_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <zlog.h>
#include "stdint.h"
#include "reach_os.h"
#include "reach_socket.h"
#include "reach_udp_snd.h"
//add zl

#define THREAD_POOL_SIZE						(3*1024*1024)
// add zl
#define NO_COMMUNICATION						(0x0)
#define PUSH_IN_UDP							(0x1)
#define PUSH_IN_TCP							(0x2)
#define PULL_IN_UDP							(0x3)
#define PULL_IN_TCP							(0x4)

// add zl media_data port

#define	UDP_SYN_TO_ROOM_HD					(0x30A1)
#define	UDP_SYN_TO_ROOM_SD					(0x31A1)
#define UDP_SDI1_TO_ROOM						(0x30A2)
#define UDP_SDI2_TO_ROOM						(0x30A3)
#define UDP_ENC1200_TO_ROOM					(0x30A4)
#define UDP_ENC120_TO_ROOM					(0x30A5)
#define UDP_ROOM_TO_SYN_IPENC1				7600//(0x30C6)   // debug zl
#define UDP_ROOM_TO_SYN_IPENC2				7610//(0x30C7)
#define UDP_ROOM_TO_SYN_IPENC3				7620//(0x30C8)
#define UDP_ROOM_TO_SYN_IPENC4				7630//(0x30C9)

#define UDP_ROOM_TO_SYN_IPENC1_BIND			(0x40C6)
#define UDP_ROOM_TO_SYN_IPENC2_BIND			(0x40C7)
#define UDP_ROOM_TO_SYN_IPENC3_BIND			(0x40C8)
#define UDP_ROOM_TO_SYN_IPENC4_BIND			(0x40C9)


// add zl media_xml port						// 后续ENC端口 TCP_SYN_TO_ROOM + ENC_ID
#define TCP_SYN_TO_ROOM						(0x20A1)   // debug zl
#define TCP_SDI1_TO_ROOM						(0X20A2)
#define TCP_SDI2_TO_ROOM						(0X20A3)
#define TCP_ENC1200_TO_ROOM					(0X20A4)
#define TCP_ENC120_TO_ROOM					3400//(0X20A5)
#define TCP_IPENC1_TO_ROOM					3400//(0X20A6)
#define TCP_IPENC2_TO_ROOM					3400//(0X20A7)
#define TCP_IPENC3_TO_ROOM					3400//(0X20A8)
#define TCP_IPENC4_TO_ROOM					3400//(0X20A9)


#define RECV_KEEPLIVE							(0)
#define CONNECT 								(0x1)
#define DISCONNECT 							(0x0)
#define INVALID_ENC							(0X2)    // add zl

#define RUNNING    								CONNECT
#define CLOSEING    							DISCONNECT
#define START_CONNECT					CONNECT
#define START_LIVE							CONNECT
#define START_REC								CONNECT
#define STOP_REC								DISCONNECT
#define STOP_LIVE								DISCONNECT
#define RECV_SELECT_DELAY			(600000)
#define SLEEP_DELAY							(100000)
#define LOOP_DELAY							(3)
#define CLOSE_CONNECT_DELAY     (300000)
#define RECV_KEEPALIVE_DALAY		(5)

#define ENC_USER								"admin"
#define ENC_PASSWORD					"123"
#define ENC_MSGCODE						"31001"
#define ENC_PASSKEY							"RecServer"
#define ENC_VER									(2012)
#define XML_TYPE								(0x1)

#define AUDIO_STREAM						(0x6)		//通用
#define HIGH_STREAM						(0x7)		//enc110 enc1200
#define LOW_STREAM         				(0x95)     //enc1200
#define LOW_STREAM_T						(0x96)    //enc110
#define JPG_STREAM							(0x94)     //enc120
#define JPG_CODEC_TYPE					(0x6765706A)

#define AUDIO_LOW_SAMPLE			(44100)
#define AUDIO_HIGH_SAMPLE			(48000)

#define DEPENDE_PACKGE							(3)
#define MAX_STREAM   							(18)    // add zl
#define CLOSE_SOCK								(-1)

#define AUDIO_ID								(1)  //the 1 high stream transport audio data.
#define AUDIO_IDT								(2) //the 2 low stream transport audio data.
#define  INVALID_MSGTYPE				(-1)
#define  COLLECT_PACKAGE				(-19)

#define MSG_SEND_ERR						(-20)
#define RECEIVE_DATA_ERR				(-21)
#define RECEIVE_MALLOC_ERR			(-22)
#define CONNECT_ERROR					(-23)
#define  LOSE_CONNECT                   (-24)
#define MAX_CONNECT						(-25)

#define MAX_FRAME_LEN					(2*1024*1024)
#define MAX_AUDIO_LEN					(10*1024)  // add zl
#define MAX_TCP_PACKAGE				(64 * 1024)
#define RECV_BUF								(640 * 1024)

#define EXAMIN_TIME							(800)

#define RECV_THREAD_PRIORITY	(50)

#define RECV_REPORT_MSGCODE  		   	"30088"
#define RECV_REPORT_PASSKEY    		  	"RecvModule"
#define RECV_MSGHEAD								"MsgHead"
#define RECV_MSGCODE								 "MsgCode"
#define RECV_RETURNCODE						 "ReturnCode"
#define RECV_LOGIN_MSGCODE				 (31001)
#define RECV_ENC_REPORT_MSGCODE				(30032)
#define RECV_ENC_MAX_LOGIN					(30087)
#define RECV_LOGIN_SUC					 		(1)
#define RECV_LOGIN_ERR					 		(0)
#define RECV_TMP_SIZE								(128)

#define IS_FLAG_BIT_TRUE(flag, bit)			(((1<<bit)&flag)>>bit)
#define SET_FLAG_BIT_TRUE(flag, bit)		(flag |= 1<<bit)
#define SET_FLAG_BIT_FALSE(flag, bit)		(flag &= (~(1<<bit)))

#define RECORD_BIT										(0)
#define LIVE_BIT												(1)
#define REPORT_BIT										(2)				//用于判断是否上报的位。
#define CTRL_BIT												(3)
#define LOGIN_BIT											(4)
#define RECV_VIDEO_I_FRAME_DATA				(5)
#define RECV_FIRST_AUDIO_DATA				(6)



//msg head
typedef struct __MSGHEAD__ {
    uint16_t		nLen;
    uint16_t  	nVer;
    uint8_t	    nMsg;
    uint8_t	    szTemp[3];
} MSGHEAD;

typedef struct  __RECV_REAL_PRINT{
    uint8_t status;			//connect status, 1is connected 0 is not connected.
    uint8_t  data_type;//0:视频,1:音频,2:jpg
    uint8_t  stream_type; // 0高码流，1低码流.
    uint8_t  is_data;//是否有数据0无，1有.
    uint8_t  frame_rate;
	 uint8_t   to_rec;
	 uint8_t  to_live;
    int32_t   width;//宽.
    int32_t   hight;//高.
    int32_t code_rate;
    int32_t sample_rate;
	// add zl
	pthread_mutex_t  print_mutex;
}recv_print;

typedef struct __RECV_REAL_TIME{
    uint32_t   video_init_time;
    uint32_t   audio_init_time;
	 uint8_t     init_video_flag;
	 uint8_t     init_audio_flag;
    uint32_t   time_audio_tick;
	 uint32_t   time_video_tick;
	 uint32_t    video_cal_time;
	 uint32_t   audio_cal_time;
	 uint32_t   prev_video_time;
	 uint32_t   prev_audio_time;
	 uint32_t   prev_video_time_tick;
	 uint32_t   prev_audio_time_tick;
    uint32_t 	 reserve;

}recv_time;

typedef struct room_stream_handle
{
    uint8_t status;        //is going to connect encode
    uint8_t pthread_status;
    int32_t stream_id;   //stream id
    int32_t msg_recv_to_ctrl;  //receive data to live module
    int32_t msg_recv_to_live;
    int32_t msg_recv_to_rec;
    uint8_t rec_status;
    uint8_t live_status;
    int32_t offset;		//a offset  for organizing  frame;
    int8_t *frame_data;//a buf  for organizing  frame .
    int32_t  audio_offset;
    int8_t *audio_data;
    recv_print recv_pri;
    recv_time recv_time_t;
    uint32_t  log_flag;//打印控制.
    uint8_t  	alive_flag;
	 uint8_t   av_connect_flag;//是否上次断过网，导致断开接收音视频。
    pthread_t pid;  //thread id
    /*connect value*/
    uint8_t ipaddr[IP_LEN];
    uint16_t port;
    int32_t sockfd;   //connect server
    pthread_mutex_t mutex;
    pthread_mutex_t  alive_mutex;
    pthread_mutex_t  status_mutex;
	int32_t		enc_id;
	int32_t		push_module;  			// zl
	int32_t 	pull_module;
	int32_t		pull_udp_port;			// 绑定接受端口
	int32_t		push_udp_port;			// 发送端端口

	int32_t  	push_module_flag;
	int32_t 	pull_module_flag;

	uint8_t		push_ip[IP_LEN];

}stream_handle;

typedef struct __RECV_PARAM {
    uint8_t status;
    int32_t stream_id;
    uint8_t ipaddr[IP_LEN];
    uint16_t 	enc_id;
}recv_param;

typedef struct recve_room_handle
{
    int32_t stream_num;// set the max num stream.
    stream_handle   stream_hand[MAX_STREAM];
    int32_t (*init_room_module)(void *recv_hand);
    int32_t (*deinit_room_module)(void *recv_hand);
    int32_t (*set_room_info)(recv_param *param, void *recv_hand, int32_t param_len);
    int32_t (*close_room_connect)(void *recv_hand);
    int32_t (*open_room_connect)(void *recv_hand);
    int32_t (*close_stream_connect)(stream_handle *stream_hand);
    int32_t (*open_stream_connect)(stream_handle *stream_hand);
    int32_t (*get_stream_connect)(stream_handle *stream_hand);
    int32_t (*set_recv_to_ctrl_msgid)(int32_t msgid, stream_handle *stream_hand);
    int32_t (*set_recv_to_live_msgid)(int32_t msgid, stream_handle *stream_hand);
    int32_t (*set_recv_to_rec_msgid)(int32_t msgid, stream_handle *stream_hand);
    int32_t (*set_rec_status)(stream_handle *stream_hand, uint8_t status);
    int32_t (*get_rec_status)(stream_handle *stream_hand);
    int32_t (*set_live_status)(stream_handle *stream_hand, uint8_t status);
    int32_t (*get_live_status)(stream_handle *stream_hand);
    int32_t (*get_stream_socket) (stream_handle *stream_hand);
}recv_room_handle;

#define  m_msgque msgque

#ifndef RECEIVE_XML_MODULE
#define RECEIVE_XML_MODULE
#define NODE_LEN			(100)
#define XML_MSGCODE_LEN	(20)
#define XML_PASSKEY_LEN	(50)
#define XML_MSG_LEN		(4096)
#define XML_MSG_USER_LEN    XML_MSGCODE_LEN
#define XML_MSG_PASS_LEN    XML_MSGCODE_LEN

typedef struct receive_xml_handle{
    int8_t msgcode[XML_MSGCODE_LEN];
    int8_t passkey[XML_PASSKEY_LEN];
    int8_t xml_buf[XML_MSG_LEN];
    int8_t user[XML_MSG_USER_LEN];
    int8_t password[XML_MSG_PASS_LEN];
}recv_xml_handle;
#endif

#define MsgLen 		 sizeof(MSGHEAD)
#define MsgqueLen  sizeof(m_msgque)
#define RoomLen		sizeof(recv_room_handle)


int32_t  get_stream_status(stream_handle *stream_hand);
int32_t get_rec_status(stream_handle *stream_hand);
int32_t get_live_status(stream_handle *stream_hand);
int32_t recv_print_status(recv_room_handle *recv_handle);
int32_t recv_sleep_second(stream_handle *p_handle, int32_t count);
int32_t memset_pri_status(recv_print *pinfo);
int8_t unregister_room_receive_module(recv_room_handle *handle);
recv_room_handle *register_room_receive_module(int32_t _max_stream_num);

#endif


