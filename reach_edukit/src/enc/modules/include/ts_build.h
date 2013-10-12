#ifndef _TS_BUILD_H__
#define _TS_BUILD_H__








void ts_build_audio_data(void *temp, unsigned char *pData, int nLen, unsigned int nowtime,void *info);
void ts_build_video_data(void *temp, unsigned char *pData, int nLen, unsigned int nowtime,void *info);
void *ts_build_init(unsigned int vpid, unsigned int apid);
void ts_build_uninit(void *temp);
void ts_build_reset_time(void *temp);
int ts_build_get_version(char *version, int len);


#endif

