#ifdef HAVE_RTSP_MODULE


#ifndef APP_PROTOCOL_HANDLE_H__
#define APP_PROTOCOL_HANDLE_H__







void app_build_ts_init();

void app_build_ts_vdata(unsigned char *buff, int len , unsigned int timestamp, APP_MEDIA_INFO *info);


void app_build_ts_adata(unsigned char *buff, int len, unsigned int timestamp, APP_MEDIA_INFO *info);

void app_build_rtp_init();

int app_build_rtp_vdata(int len, unsigned char *buff, int IsIframe, int mtu, unsigned int rtp_time, APP_MEDIA_INFO *media_info);

int app_build_rtp_adata(int len, unsigned char *buff, int samplerate, int mtu, unsigned int rtp_time, APP_MEDIA_INFO *media_info);

void app_build_rtpts_init();

int app_build_rtpts_data(unsigned int nLen, unsigned char *pData, int rtp_mtu, int frame, unsigned int nowtime, APP_MEDIA_INFO *info);

#endif

#endif