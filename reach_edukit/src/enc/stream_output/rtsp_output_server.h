#ifdef HAVE_RTSP_MODULE


#ifndef _RTSP_OUTPUT_SERVER_H__
#define _RTSP_OUTPUT_SERVER_H__


/**************************************************************
* rtsp_server_config.ini
*[rtsp]
*used = 1             //just start or stop
*status = 0          //active,pause,stop
*type = 1 	//1 open multicast  0 rtp
*main_ip = 234.255.0.5   //multicast ip
*video_port=9000
*audio_port=9002
*mtu=0
*ttl=60 //just avild for multicast
*tos=0
*active=60 //s
*tc=0
***************************************************************/

void app_rtsp_server_init(void);
int app_rtsp_server_add(stream_output_server_config *config, stream_output_server_config *newconfig);
int app_rtsp_server_delete(void);
int app_rtsp_server_used(void);
int app_rtsp_server_set_global_info(rtsp_server_config *config, rtsp_server_config *newconfig);
int app_rtsp_server_get_global_info(rtsp_server_config *config);
int app_rtsp_server_get_common_info(stream_output_server_config *config);
int app_rtsp_server_set_common_info(stream_output_server_config *in, stream_output_server_config *out);
int app_rtsp_server_set_status(stream_output_server_config *in, stream_output_server_config *out);
int app_rtsp_get_active(void);
int stream_server_config_printf(stream_output_server_config *config);
int stream_get_rtsp_ginfo(int *mult, char *ip, unsigned int *vport, unsigned int *aport,int stream_channel);
void *stream_create_rtsp_client(int pos, struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr,int stream_channel);
int stream_close_rtsp_client(int pos, void *p);
int stream_get_rtsp_mtu(void);
int stream_rtsp_get_active(void);

int stream_rtsp_set_all_tc();
int stream_rtsp_set_all_config();
int stream_rtsp_del_client_tc(int num);





#endif

#endif
