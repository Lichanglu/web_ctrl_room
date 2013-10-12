#ifdef HAVE_RTSP_MODULE
#ifndef _RTSP_PORTING_SDP_INFO_H__
#define _RTSP_PORTING_SDP_INFO_H__


#define MAX_SPS_INFO_LEN 64
#define MAX_PPS_INFO_LEN MAX_SPS_INFO_LEN

#define 	H264_HEADER_LEN 			0x40





int rtsp_porting_get_sdp_describe(char *pSend, int len, int channel, char *rtpip, int vport, int aport);


int rtsp_porting_video_filter_sdp_info(unsigned char *pdata, int len,int width, int height, int id);
int rtsp_porting_audio_filter_sdp_info(unsigned char *pdata, int channel, int samplerate, int id);
int rtsp_stream_get_video_sdp_info(char *buff, int id);
unsigned int rtsp_stream_get_audio_sinfo(int id, char *buff, int *rate);
void rtsp_porting_sdp_init(void);
void rtsp_porting_sdp_destory(void);



#endif
#endif
