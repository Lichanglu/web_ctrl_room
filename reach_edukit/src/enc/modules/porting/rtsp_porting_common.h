
#ifndef _RTSP_PORTING_COMMON_H__
#define _RTSP_PORTING_COMMON_H__

#ifdef HAVE_RTSP_MODULE

/*
* RTSP使用前需要强制I帧，用于VLC/QT快速播放
*/
//void rtsp_porting_force_Iframe();
int rtsp_porting_server_need_stop();
int rtsp_porting_parse_url(char *url, char *localip, int *roomid,char *serv_url);
int rtsp_porting_get_ginfo(int *mult, char *ip, unsigned int *vport, unsigned int *aport, int stream_channel);
int rtsp_porting_close_client(int pos, void *client);
void *rtsp_porting_create_client(int pos, struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr, int stream_channel);
#endif

#endif
