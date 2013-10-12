#ifdef HAVE_RTSP_MODULE


#ifndef _APP_STREAM_OUTPUT_H__
#define _APP_STREAM_OUTPUT_H__

typedef struct APP_VIDEO_INFO_T {
	int width;
	int height;
	unsigned int timestamp;

	int IsIframe;
	unsigned int rtp_time;

	//8168 need
	int stream_channel;
} APP_VIDEO_DATA_INFO;

typedef struct APP_AUDIO_INFO_T {
	int channel;
	int samplerate;
	unsigned int timestamp;

	int recreate; /*表示重建编码器，需要刷新  audio sdp info*/
	unsigned int rtp_time;
	//8168 need
	int stream_channel;
} APP_AUDIO_DATA_INFO;



void app_stream_output_init(unsigned int localip1,char *user_name,char *user_pass,char *reaml,int permissions);
int app_stream_video_output(unsigned char *buff, int len, APP_VIDEO_DATA_INFO *info);
int app_stream_audio_output(unsigned char *buff, int len, APP_AUDIO_DATA_INFO *info);

//void app_build_reset_time();



#endif

#endif