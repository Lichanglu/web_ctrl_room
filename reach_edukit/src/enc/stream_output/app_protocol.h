#ifdef HAVE_RTSP_MODULE


#ifndef _APP_PROTOCOL_H__
#define _APP_PROTOCOL_H__

#include "middle_control.h"


int app_protocol_get(Protocol *info);
int app_set_media_info(int width, int height, int sample , int num);
int app_get_have_stream();
int app_get_media_info(int *width, int *height, int *sample, int num);





#endif

#endif
