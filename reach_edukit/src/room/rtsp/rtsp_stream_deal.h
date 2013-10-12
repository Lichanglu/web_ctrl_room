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
	FRAMEHEAD 	head;    //编码器发送过来的
	unsigned int   		wlen;    //已经写入的数据
	unsigned int 		status;   //空0，构建中1，构建完成2
	unsigned char 		*data;
}ONE_FRAME_S;



typedef struct STREAM_VIDEO_HANDLE_T
{
	unsigned int 	size ;		//sizeof(struct ENCODE_HANDLE_T)
	unsigned int 	roomid;     //区分第几路编码器 0/1/2/3   
	unsigned int 	stream_num; // 0 无流 1 只有视频 2 只有音频 3 音视频
	
	//video
	unsigned int 	width ;     //宽  记录上一次的宽高
	unsigned int 	height ;    //高
	unsigned long begin_video_time;//记录当前RTP发送第一个数据的时间
	unsigned long last_video_current_time ; //记录最后一次视频RTP发送时间
	unsigned long last_video_rtp_time;  	//记录上一次发送VIDEO的rtp包里带的时间信息
	unsigned int 	sdp_flag ;             //是否有sdp info
	unsigned char 	sps_info[MAX_SPS_INFO_LEN];
	unsigned int 	sps_info_len;
	unsigned char 	pps_info[MAX_SPS_INFO_LEN];
	unsigned int 	pps_info_len;
	
	//audio
//	int samplerate; //音频采样率
//	int audio_config ; // 是否有audio config 
//	unsigned long last_audio_current_time; //记录最后一次音频RTP发送时间
//	unsigned long last_video_rtp_time;  //记录上一次发送AUDIO的rtp包里带的时间信息
	
	unsigned short seq ; //rtp seq num
	
	ONE_FRAME_S   video_frame;
//	ONE_FRAME_S   audio_frame;
		
//	mid_mutex_t mutex; //锁 
}stream_video_handle_t;

typedef struct STREAM_AUDIO_HANDLE_T
{
	unsigned int size ;		//sizeof(struct ENCODE_HANDLE_T)
	unsigned int roomid;     //区分第几路编码器 0/1/2/3   
	unsigned int stream_num; // 0 无流 1 只有视频 2 只有音频 3 音视频
	
	//video
//	int width ;     //宽  记录上一次的宽高
//	int height ;    //高
//	unsigned long last_video_current_time ; //记录最后一次视频RTP发送时间
//	unsigned long last_video_rtp_time;  	//记录上一次发送VIDEO的rtp包里带的时间信息
//	int sdp_flag ;             //是否有sdp info
	
	//audio
	unsigned int channel;  //默认为2
	unsigned int samplerate; //音频采样率
	unsigned int audio_config ; // 是否有audio config 
	unsigned long begin_audio_time;//记录当前RTP发送第一个数据的时间
	unsigned long last_audio_current_time; //记录最后一次音频RTP发送时间
	unsigned long last_audio_rtp_time;  //记录上一次发送AUDIO的rtp包里带的时间信息
	
	unsigned short seq ; //rtp seq num
	
//	ONE_FRAME_S   video_frame;
	ONE_FRAME_S   audio_frame;
		
	//mid_mutex_t mutex; //锁 
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
