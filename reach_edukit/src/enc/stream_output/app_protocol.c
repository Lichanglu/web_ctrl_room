#ifdef HAVE_RTSP_MODULE


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include "stream_output_struct.h"
#include "app_protocol.h"
//#include "log_common.h"

#include "input_to_channel.h"
#include "nslog.h"

Protocol gProtocol;

int app_protocol_inif()
{
	return 0;
}

int app_protocol_get(Protocol *info)
{
	return 0;
}

int app_get_have_stream()
{
	return 1;
	//	if(	gProtocol.status  == 0)
	//		return 0;
	//	else
	//		return 1;
}


static APP_MEDIA_INFO g_media_info[STREAM_MAX_CHANNEL] ;

int app_set_media_info(int width, int height, int sample , int num)
{
	if(num < 0 || num >= STREAM_MAX_CHANNEL) {
		PRINTF("NUM is error\n");
		return -1;
	}

	if(352 <= width &&  width <= 1980) {
		g_media_info[num].width = width;
	}

	if(288 <= height &&  height <= 1200) {
		g_media_info[num].height = height;
	}

	//now just support sample 44100/48000
	if(sample == 44100 || sample == 48000) {
		g_media_info[num].sample = sample;
	}

	//	nslog(NS_ERROR,"WIDTH_OLD : %d WIDTH_NEW : %d  HEIGHT_OLD : %d HEIGHT_NEW : %d SAMPLE_OLD : %d SAMPLE_NES : %d\n",
	//		width,g_media_info[num].width,height,g_media_info[num].height,sample,g_media_info[num].sample);
	return 0;

}

int app_get_media_info(int *width, int *height, int *sample, int num)
{
	int w, h, s;
	w = h = s = 0;
	w = g_media_info[num].width;
	h = g_media_info[num].height;
	s = g_media_info[num].sample;

	if(352 <= w &&  w <= 1980) {
		*width = w;
	} else {
		PRINTF("error,width is error,=%d.", w);
		return -1;
	}

	if(288 <= h &&  h <= 1200) {
		*height = h;
	} else {
		PRINTF("error,height is error,=%d.", h);
		return -1;
	}

	//now just support sample 44100/48000
	if(s  == 44100 || s  == 48000) {
		*sample = s;
	} else {
		PRINTF("error,sample is error,=%d.", s);
		return -1;
	}

	return 0;
}

#endif
