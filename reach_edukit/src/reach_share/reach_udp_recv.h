#ifndef __REACH_UDP_RECV_H
#define  __REACH_UDP_RECV_H  

#include "stdint.h"

#define UdpRecv_DEBUG	(1)

#ifdef UdpRecv_DEBUG
#define udp_recv_err_print(a...) {printf("[%s %s %d] errno = %d ", __TIME__, __func__, __LINE__, errno);printf(a);putchar('\n');}
#define udp_recv_warn_print(a...) {printf("[%s %s %d] warn ", __TIME__, __func__, __LINE__, errno);printf(a);putchar('\n');}
#define UdpRecv_PRINT(a...)             {printf("[%s %s %d] ", __TIME__, __func__, __LINE__);printf(a);putchar('\n');}
#endif

#define UdpRecv_THREAD_PRIORITY	  (50)

/*UDP NetWork Header*/
typedef struct udp_recv_freame_head {
	uint32_t m_id;			//= mmioFOUdpRecvCC('R','Q','H','D');
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
} udp_recv_freame_head_t;

#define UdpRecv_FH_LEN	  sizeof(udp_recv_freame_head_t)
#define UdpRecv_I_FRAME									(0x10)

#define UdpRecv_IP_LEN				(16)
#define UdpRecv_MAX_UDP_LEN							(1500)
#define UdpRecv_BUF_MAX_SIZE   						655360

#define	UdpRecv_MAKEFOUdpRecvCC(ch0, ch1, ch2, ch3)                      \
		((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |   \
		((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))

#define	UdpRecv_H264_CODEC_TYPE   			UdpRecv_MAKEFOUdpRecvCC('H','2','6','4')
#define	UdpRecv_AAC_CODEC_TYPE    			UdpRecv_MAKEFOUdpRecvCC('A','D','T','S')
#define 	UdpRecv_RQHD_CODEC_TYPE			UdpRecv_MAKEFOUdpRecvCC('R','Q','H','D')
#define   UdpRecv_JPEG_CODEC_TYPE 			UdpRecv_MAKEFOUdpRecvCC('J','P','E','G')

typedef struct UdpRecv_HDB_FRAME_HEAD 
{   

    unsigned char codec:4;          //编码类型  0--H264  1--ADTS  2-JPEG             
    unsigned char samplerate:4;   //采样率    编码器为ADTS （高4位为  0--16kbps 1--32kbps 2--44.1kbps 3--48kbps 4--96kbps， 12 蓝屏）     
    unsigned char framerate;        //帧率            
    unsigned short width;           //实际宽        
    unsigned short height;           //实际高 
    unsigned char Iframe;				// I 帧标志
    unsigned char reserve;				// 保留位 
    unsigned int  framelength;			// 帧长
}UdpRecv_FRAMEHEAD;


typedef struct 
{
    /**//* byte 0 */
    unsigned char csrc_len:4;        /**//* expect 0 */
    unsigned char extension:1;        /**//* expect 1, see RTP_OP below */
    unsigned char padding:1;        /**//* expect 0 */
    unsigned char version:2;        /**//* expect 2 */
    /**//* byte 1 */
    unsigned char payload:7;        /**//* RTP_PAYLOAD_RTSP */
    unsigned char marker:1;        /**//* expect 1 */
    /**//* bytes 2, 3 */
    unsigned short seq_no;            
    /**//* bytes 4-7 */
    unsigned  int timestamp;        
    /**//* bytes 8-11 */
    unsigned int ssrc;            /**//* stream number is used here. */

    //unsigned long scrc;				// ZHENYB
}UdpRecv_RTP_FIXED_HEADER;

typedef struct {
    //byte 0
    unsigned char TYPE:5;
    unsigned char NRI:2; 
    unsigned char F:1;     
}UdpRecv_FU_INDICATOR; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
    unsigned char R:1;
    unsigned char E:1;
    unsigned char S:1;    
} UdpRecv_FU_HEADER; /**//* 1 BYTES */

#define UdpRecv_BLUE_FLAG  (12)

typedef struct udp_recv_msg_headers {//消息头
	uint16_t m_len;		//长度
	uint16_t m_ver;		//版本
	uint16_t m_msg_type;	//消息类型
	uint16_t m_data;	//保留
} udp_recv_msg_header_t;

typedef enum udp_recv_stream_status {
    STREAM_PAUSE,
    STREAM_START
}udp_recv_stream_status_t;

typedef struct udp_recv_stream_info_t{
	uint32_t  type;//数据类型AAC?H264?JPG.
	int32_t 	width; //宽。
	int32_t   height;//高。
//	int signal_flag;
	int32_t samplerate;// 采样率
	int32_t IDR_FLAG;//i 帧
	int32_t timestamp; //获取的启动掉电时间。		
	int32_t is_blue;
	int32_t m_colors;
	int32_t frame_rate;
		
	int32_t video_toal_num; //包总数。丢包统计
	int32_t video_lost_num;
	uint8_t 	ip[UdpRecv_IP_LEN]; //接收端ip地址。
	uint16_t 	port;//接收端端口。
	uint32_t  udp_fd;
	uint8_t    reserve;
	void *		user_data;//用户传入的指针。
}udp_recv_stream_info_t;     //流 信息结构体

typedef struct udp_recv_real_time{
    uint32_t   video_init_time;//视频初始时间。
    uint32_t   audio_init_time;
	 uint8_t     init_video_flag;//初始换视频时间的标志.
	 uint8_t     init_audio_flag;
    uint32_t   time_audio_tick;//传出去的视频时间。
	 uint32_t   time_video_tick;
	 uint32_t    video_cal_time;//传来的时间与初始的时间差的绝对值.
	 uint32_t   audio_cal_time;
	 uint32_t   prev_video_time;//上一次的本地视频时间。
	 uint32_t   prev_audio_time;
	 uint32_t   prev_video_time_tick;//上一个传入的视频时间。
	 uint32_t   prev_audio_time_tick;
    uint32_t 	 reserve;//保留供debug。
}udp_recv_time;

typedef int	(*func)(udp_recv_stream_info_t *p_stream_info, char *data, int len);

typedef struct udp_recv_handle_t{
	udp_recv_stream_status_t    stream_status; 
	udp_recv_stream_info_t       stream_info;
	udp_recv_time		  udp_recv_time_t;
	uint8_t 				 *video_data;
	uint8_t 				 *audio_data;
	uint32_t 				 video_offset;
	uint32_t 				 audio_offset;
	pthread_t            upd_pt;
	uint8_t					 pthread_status;
	int32_t 				stream_num;
	int	(*func)(udp_recv_stream_info_t *p_stream_info, char *data, int len);//函数指针，处理接收到一帧数据。
}udp_recv_handle;

typedef enum _udp_recv_segment {
	UdpRecv_MIDDLE_FRAME, 		//0表示中间包
	UdpRecv_LAST_FRAME,			//1表示结尾包 
	UdpRecv_START_FRAME,		//2表示开始包
	UdpRecv_INDEPENDENT_FRAME	//3表示独立包
} udp_recv_segment_t;


#define Ur_MsgLen    											sizeof(udp_recv_msg_header_t)
#define UdpRecv_OPERATION_ERR								(-1)
#define UdpRecv_OPERATION_SUCC							(0)
#define UdpRecv_OPERATION_CONTINUE					(1) 	// 操作继续
#define UdpRecv_MAX_FRAME_LEN							(2*1024*1024)
#define UdpRecv_MAX_AUDIO_LEN							(512*1024)
#define UdpRecv_FRAME_OK										(0x80)
#define UdpRecv_THREAD_RUN									(0x1)
#define UdpRecv_THREAD_EXIT									(0x0)

#define UdpRecv_EXAMIN_TIME									(800)

typedef struct stream_recv_cond_ {
	char ip[UdpRecv_IP_LEN];//接收端ip地址
	unsigned short port;//接收端端口
	void *user_data;
	int (*func)(udp_recv_stream_info_t *p_stream_info, char *data, int len);//函数指针，处理接收到一帧数据。
} stream_recv_cond_t;

//初始化,
udp_recv_handle *udp_recv_init(stream_recv_cond_t *src);
//反初始化
void udp_recv_deinit(udp_recv_handle *p_udp_hand);
//暂停接受
int udp_recv_pause(udp_recv_handle *p_udp_hand);
//开始接受
int udp_recv_start(udp_recv_handle *p_udp_hand);
//回调函数。
int udp_recv_func(udp_recv_stream_info_t *p_stream_info,char *data,int len);

#endif

//Example for use this module
//user can realize this func
#if 0

int udp_recv_func(udp_recv_stream_info_t *p_stream_info,char *data,int len)
{
		// test write file
		//deal data
		retudp_recvn 0;
}
#endif

#if 0


int udp_recv_main(void)
{
	int ret = -0;
	udp_recv_stream_info_t *p_stream_info  = NULL;
	char data[UdpRecv_MAX_FRAME_LEN] = {0};
	int len = 0;
	udp_recv_handle *p_recv_hand = NULL;
	int count = 0;
	p_recv_hand = udp_recv_init(NULL);
	udp_recv_start(p_recv_hand);
	while (1){
		if (count == 30){
				udp_recv_deinit(p_recv_hand);
				break;
		}
		count;
		sleep(1);	
	}
	UdpRecv_PRINT("main exit");
    retudp_recvn 0;

}

#endif
