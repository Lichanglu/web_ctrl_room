#ifdef HAVE_RTSP_MODULE


#ifndef MULTICAST_OUTPUT_SERVER_H__
#define MULTICAST_OUTPUT_SERVER_H__


/**************************************************************
* multicast_server_config.ini
*[total]
*num=5
*[server0]
*status = 0          //active,pause,stop
*type = 1 	//1 open multicast  0 rtp
*main_ip = 234.255.0.5   //multicast ip
*video_port=9000
*audio_port=9002
*mtu=0
*ttl=60 //just avild for multicast
*tos=0
*tc=0
*[server1]
*status = 0          //active,pause,stop
*type = 1 	//1 open multicast  0 rtp
*main_ip = 234.255.0.5   //multicast ip
*video_port=9000
*audio_port=9002
*mtu=0
*ttl=60 //just avild for multicast
*tos=0
*tc=0
*[end]
*flag= 1
***************************************************************/


void app_multicast_server_init(void);
int app_multicast_get_total_num(void);
int app_multicast_get_config(int num, stream_output_server_config *config);
int app_multicast_add_server(stream_output_server_config *config, stream_output_server_config *newconfig);
int app_multicast_delete_server(stream_output_server_config *config);
int app_multicast_set_config(stream_output_server_config *config, stream_output_server_config *newconfig2);
int app_multicast_set_status(stream_output_server_config *config, stream_output_server_config *newconfig);
int app_mult_set_all_tc(void);
int mult_rtp_get_active(int channel);
int mult_ts_get_active(int channel);
int mult_rtpts_get_active(int channel);
int mult_get_direct_rtp_mtu(int channel);
int mult_get_direct_ts_mtu(int channel);
int mult_get_rtp_ts_mtu(int channel);
int stream_get_rate(int cmd);

int app_trans_rate(int vrate, int arate, int type);




#endif

#endif
