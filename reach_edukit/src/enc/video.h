#ifndef _VIDEO_H_
#define _VIDEO_H_
#include "reach.h"
#define VODFILE_MAX_FILENAME					(512)
#define	RECV_STREAMNUM			2
#define DEC_STREAMNUM			2
#define	UDP_SEND_ROOM_PORT	(0x30a4)
#define	UDP_SEND_HD_PORT		(0x30c4)
//JPEG SEND
#define	UDP_SEND_JPEG_ROOM_PORT	(0x30a5)
#define	UDP_SEND_HD_IP			("169.254.0.4")
#define	MAX_UDP_SEND_NUM		(2+1)
#define	ETH_NAME				"eth1"
#define	MAX_FRAME_LEN		(1024 * 1024*2)
typedef struct VideoEncodeParam_{
	unsigned int 			uWidth;
	unsigned int 			uHeight;
	unsigned int 		 	nFrameRate;
	unsigned int			IFrameinterval;
	unsigned int 			sBitrate;
	unsigned int 			sCbr;
	unsigned int 			nQuality;
}VideoEncodeParam;

int setVideoEncodeParam(int chId, VideoEncodeParam *vinfo);
int getVideoEncodeParam(int chId, VideoEncodeParam *vinfo);
int app_video_request_iframe(int channel);

void *inHostStreamUdpSendProcess(bits_user_param *param);
void *OutHostStreamProcess(bits_user_param *param);

#endif
