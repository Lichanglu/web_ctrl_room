#ifdef HAVE_RTSP_MODULE


#ifndef _STREAM_OUTPUT_CLIENT_H__
#define _STREAM_OUTPUT_CLIENT_H__




//typedef struct MEDIA_INFO_T APP_MEDIA_INFO;
int stream_client_init(void);

Stream_Output_Client *stream_client_get_free(void);


int stream_client_open(struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr, int type, Stream_Output_Client *client,int stream_channel);

int stream_client_close(Stream_Output_Client *client);

int stream_client_ts_send(unsigned char *buff, int len, int type, APP_MEDIA_INFO *info);
int stream_client_rtp_send(unsigned char *buff, int len, int type, APP_MEDIA_INFO *info);

int stream_client_rtpts_send(unsigned char *buff, int len, int type, APP_MEDIA_INFO *info);

Stream_Output_Client *stream_client_is_repeat(struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr);

int stream_client_set_mnum(Stream_Output_Client *client, int num);

int steram_client_get_mnum(Stream_Output_Client *client);

int stream_client_set(Stream_Output_Client *client, int ttl, int tos);



#endif


#endif
