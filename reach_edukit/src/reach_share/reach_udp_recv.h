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

    unsigned char codec:4;          //��������  0--H264  1--ADTS  2-JPEG             
    unsigned char samplerate:4;   //������    ������ΪADTS ����4λΪ  0--16kbps 1--32kbps 2--44.1kbps 3--48kbps 4--96kbps�� 12 ������     
    unsigned char framerate;        //֡��            
    unsigned short width;           //ʵ�ʿ�        
    unsigned short height;           //ʵ�ʸ� 
    unsigned char Iframe;				// I ֡��־
    unsigned char reserve;				// ����λ 
    unsigned int  framelength;			// ֡��
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

typedef struct udp_recv_msg_headers {//��Ϣͷ
	uint16_t m_len;		//����
	uint16_t m_ver;		//�汾
	uint16_t m_msg_type;	//��Ϣ����
	uint16_t m_data;	//����
} udp_recv_msg_header_t;

typedef enum udp_recv_stream_status {
    STREAM_PAUSE,
    STREAM_START
}udp_recv_stream_status_t;

typedef struct udp_recv_stream_info_t{
	uint32_t  type;//��������AAC?H264?JPG.
	int32_t 	width; //��
	int32_t   height;//�ߡ�
//	int signal_flag;
	int32_t samplerate;// ������
	int32_t IDR_FLAG;//i ֡
	int32_t timestamp; //��ȡ����������ʱ�䡣		
	int32_t is_blue;
	int32_t m_colors;
	int32_t frame_rate;
		
	int32_t video_toal_num; //������������ͳ��
	int32_t video_lost_num;
	uint8_t 	ip[UdpRecv_IP_LEN]; //���ն�ip��ַ��
	uint16_t 	port;//���ն˶˿ڡ�
	uint32_t  udp_fd;
	uint8_t    reserve;
	void *		user_data;//�û������ָ�롣
}udp_recv_stream_info_t;     //�� ��Ϣ�ṹ��

typedef struct udp_recv_real_time{
    uint32_t   video_init_time;//��Ƶ��ʼʱ�䡣
    uint32_t   audio_init_time;
	 uint8_t     init_video_flag;//��ʼ����Ƶʱ��ı�־.
	 uint8_t     init_audio_flag;
    uint32_t   time_audio_tick;//����ȥ����Ƶʱ�䡣
	 uint32_t   time_video_tick;
	 uint32_t    video_cal_time;//������ʱ�����ʼ��ʱ���ľ���ֵ.
	 uint32_t   audio_cal_time;
	 uint32_t   prev_video_time;//��һ�εı�����Ƶʱ�䡣
	 uint32_t   prev_audio_time;
	 uint32_t   prev_video_time_tick;//��һ���������Ƶʱ�䡣
	 uint32_t   prev_audio_time_tick;
    uint32_t 	 reserve;//������debug��
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
	int	(*func)(udp_recv_stream_info_t *p_stream_info, char *data, int len);//����ָ�룬������յ�һ֡���ݡ�
}udp_recv_handle;

typedef enum _udp_recv_segment {
	UdpRecv_MIDDLE_FRAME, 		//0��ʾ�м��
	UdpRecv_LAST_FRAME,			//1��ʾ��β�� 
	UdpRecv_START_FRAME,		//2��ʾ��ʼ��
	UdpRecv_INDEPENDENT_FRAME	//3��ʾ������
} udp_recv_segment_t;


#define Ur_MsgLen    											sizeof(udp_recv_msg_header_t)
#define UdpRecv_OPERATION_ERR								(-1)
#define UdpRecv_OPERATION_SUCC							(0)
#define UdpRecv_OPERATION_CONTINUE					(1) 	// ��������
#define UdpRecv_MAX_FRAME_LEN							(2*1024*1024)
#define UdpRecv_MAX_AUDIO_LEN							(512*1024)
#define UdpRecv_FRAME_OK										(0x80)
#define UdpRecv_THREAD_RUN									(0x1)
#define UdpRecv_THREAD_EXIT									(0x0)

#define UdpRecv_EXAMIN_TIME									(800)

typedef struct stream_recv_cond_ {
	char ip[UdpRecv_IP_LEN];//���ն�ip��ַ
	unsigned short port;//���ն˶˿�
	void *user_data;
	int (*func)(udp_recv_stream_info_t *p_stream_info, char *data, int len);//����ָ�룬������յ�һ֡���ݡ�
} stream_recv_cond_t;

//��ʼ��,
udp_recv_handle *udp_recv_init(stream_recv_cond_t *src);
//����ʼ��
void udp_recv_deinit(udp_recv_handle *p_udp_hand);
//��ͣ����
int udp_recv_pause(udp_recv_handle *p_udp_hand);
//��ʼ����
int udp_recv_start(udp_recv_handle *p_udp_hand);
//�ص�������
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
