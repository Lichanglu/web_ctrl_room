#if 0
#ifndef  _RTSP_STREAM_DEAL_1_H__
#define _RTSP_STREAM_DEAL_1_H__


#define MAX_VIDEO_LEN 1024*1024*4
#define MAX_AUDIO_LEN 1024*1024
#define MAX_SPS_INFO_LEN 64





typedef struct __HDB_FRAME_HEAD {
	unsigned int ID;								//=mmioFOURCC('4','D','S','P');
	unsigned int nTimeTick;    					//time 
	unsigned int nFrameLength; 					//length of frame
	unsigned int nDataCodec;   					//encode type
											//video:mmioFOURCC('H','2','6','4');
											//audio:mmioFOURCC('A','D','T','S');
	unsigned int nFrameRate;   					//video:framerate
										 
	unsigned int nWidth;       					//video:width
										 
	unsigned int nHight;       					//video:height
											//audio:samplebit   (default:44100)
	unsigned int nColors;      					//video:colors
											 
	unsigned int dwSegment;						//package flag
	unsigned int dwFlags;							//video: I frame
											//audio:  reserve
	unsigned int dwPacketNumber; 					//packages serial number
	unsigned int nOthers;      					//reserve
}FRAMEHEAD;




enum{
	FRAME_NULL,
	FRAME_BUILDING,
	FRAME_COMPLETE,
	FRAME_MAX
};


typedef struct  one_frame_t{
	FRAMEHEAD 	head;    //���������͹�����
	unsigned int   		wlen;    //�Ѿ�д�������
	unsigned int 		status;   //��0��������1���������2
	unsigned char 		*data;
}ONE_FRAME_S;



typedef struct STREAM_VIDEO_HANDLE_T
{
	unsigned int 	size ;		//sizeof(struct ENCODE_HANDLE_T)
	unsigned int 	roomid;     //���ֵڼ�·������ 0/1/2/3   
	unsigned int 	stream_num; // 0 ���� 1 ֻ����Ƶ 2 ֻ����Ƶ 3 ����Ƶ
	
	//video
	unsigned int 	width ;     //��  ��¼��һ�εĿ��
	unsigned int 	height ;    //��
	unsigned long begin_video_time;//��¼��ǰRTP���͵�һ�����ݵ�ʱ��
	unsigned long last_video_current_time ; //��¼���һ����ƵRTP����ʱ��
	unsigned long last_video_rtp_time;  	//��¼��һ�η���VIDEO��rtp�������ʱ����Ϣ
	unsigned int 	sdp_flag ;             //�Ƿ���sdp info
	unsigned char 	sps_info[MAX_SPS_INFO_LEN];
	unsigned int 	sps_info_len;
	unsigned char 	pps_info[MAX_SPS_INFO_LEN];
	unsigned int 	pps_info_len;
	
	//audio
//	int samplerate; //��Ƶ������
//	int audio_config ; // �Ƿ���audio config 
//	unsigned long last_audio_current_time; //��¼���һ����ƵRTP����ʱ��
//	unsigned long last_video_rtp_time;  //��¼��һ�η���AUDIO��rtp�������ʱ����Ϣ
	
	unsigned short seq ; //rtp seq num
	
	ONE_FRAME_S   video_frame;
//	ONE_FRAME_S   audio_frame;
		
//	mid_mutex_t mutex; //�� 
}stream_video_handle_t;

typedef struct STREAM_AUDIO_HANDLE_T
{
	unsigned int size ;		//sizeof(struct ENCODE_HANDLE_T)
	unsigned int roomid;     //���ֵڼ�·������ 0/1/2/3   
	unsigned int stream_num; // 0 ���� 1 ֻ����Ƶ 2 ֻ����Ƶ 3 ����Ƶ
	
	//video
//	int width ;     //��  ��¼��һ�εĿ��
//	int height ;    //��
//	unsigned long last_video_current_time ; //��¼���һ����ƵRTP����ʱ��
//	unsigned long last_video_rtp_time;  	//��¼��һ�η���VIDEO��rtp�������ʱ����Ϣ
//	int sdp_flag ;             //�Ƿ���sdp info
	
	//audio
	unsigned int channel;  //Ĭ��Ϊ2
	unsigned int samplerate; //��Ƶ������
	unsigned int audio_config ; // �Ƿ���audio config 
	unsigned long begin_audio_time;//��¼��ǰRTP���͵�һ�����ݵ�ʱ��
	unsigned long last_audio_current_time; //��¼���һ����ƵRTP����ʱ��
	unsigned long last_audio_rtp_time;  //��¼��һ�η���AUDIO��rtp�������ʱ����Ϣ
	
	unsigned short seq ; //rtp seq num
	
//	ONE_FRAME_S   video_frame;
	ONE_FRAME_S   audio_frame;
		
	//mid_mutex_t mutex; //�� 
}stream_audio_handle_t;

typedef stream_audio_handle_t  *stream_audio_handle;
typedef stream_video_handle_t  *stream_video_handle;

int rtsp_stream_get_room_num(void );
int rtsp_stream_init(int video_num,int audio_id);
int rtsp_stream_get_video_sdp_info(char *buff,int id);
int rtsp_stream_get_audio_sdp_info(char *buff);
int rtsp_stream_force_Iframe(int roomid );

int app_authentication(char *usr,char *passwd);
unsigned int rtsp_stream_get_audio_samplerate(void);

void rtsp_stream_set_handle(void *theroom);

#endif

#endif
