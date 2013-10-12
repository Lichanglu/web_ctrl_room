#include <stdio.h>

#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>

#include "reach.h"
#include "common.h"
#include "video.h"
#include "capture.h"
#include "liveplay.h"
#include "mid_mutex.h"
#include "mid_timer.h"
#include "rec_client.h"
#include <nslog.h>
#include <media_msg.h>
#include <reach_os.h>
#include <Media.h>
#include "nslog.h"
#include "reach_udp_recv.h"
#include "reach_udp_snd.h"

#define	ENCODER_MANAGER

#define	AVIIF_KEYFRAME 		(0x000010L)
#define	HD_SRC_BASE_PROT	(0x30B2)

extern int32_t gmax_room_thread;
extern EduKitLinkStruct_t	*gEduKit;
extern int g_input1_have_signal;
extern int g_input2_have_signal;

extern int g_SignalSate[2];

extern void SendDataToClient2(int nLen, unsigned char *pData, int nFlag, unsigned char index, int width, int height);

extern  int read_gaps_in_frame(unsigned char *p,  int *width,  int *hight, unsigned int len);

extern int32_t get_video_frame(int8_t **data, int32_t *length, int32_t *width, int32_t *height, int32_t *flag);

extern int app_get_have_stream();
extern int app_set_media_info(int width, int height, int sample , int num);

extern int StartEncoderMangerServer();
extern void SendDataToClient(int nLen, unsigned char *pData,
                             int nFlag, unsigned char index, int width, int height);


static int g_eindex[RECV_STREAMNUM] = {0, 6};

int eindexToChannel(int eindex)
{
	int i = 0;

	for(i = 0; i < RECV_STREAMNUM; i ++) {
		if(eindex == g_eindex[i]) {
			return i;
		}
	}

	return -1;
}

int32_t all_force_iframe()
{
	enc_force_iframe(gEduKit->encoderLink.encLink.link_id, 0);
	enc_force_iframe(gEduKit->encoderLink.encLink.link_id, 1);

	return 0;
}

static unsigned long long getCurrentTime_zl(void)
{
	struct timeval tv;
	struct timezone tz;
	unsigned long long ultime = 0 ;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec;
	ultime = ultime * 1000 + tv.tv_usec / 1000;

	return (ultime);
}


void *InHostStreamProcess(bits_user_param *param)
{
	Int32 status = -1;
	uint32_t t[4] = {0};
	Int32 idr_header_length0 = 0;
	Int32 idr_header_length1 = 0;

	Int32 length[4] = {0};
	Int32 framecount[4] = {0};
	Bitstream_Buf	*pFullBuf;
	UInt8 *p = NULL;

	if(param == NULL) {
		fprintf(stderr, "InHostStreamProcess: param is NULL!\n");
		return NULL;
	}

	setLiveMaxUserNum(4);

	startLiveplayTask();

	OSA_QueHndl *full_que = &(param->bits_full_que);
	OSA_QueHndl *empty_que = &(param->bits_empty_que);

	char *hTempBuf = (char *)malloc(1920 * 1200);
	char *hTempBuf1 = (char *)malloc(1920 * 1200);

	unsigned char h264Header0[0x40] = {0};
	unsigned char h264Header1[0x40]	= {0};


	//	LiveVideoParam videoParam;

	while(TRUE == param->run_status) {
		status = OSA_queGet(full_que, (Int32 *)(&pFullBuf), OSA_TIMEOUT_FOREVER);
		OSA_assert(status == 0);


#if PRINTF_BITSTREAM_INFO
		length[pFullBuf->channelNum] += pFullBuf->fillLength;

		if(pFullBuf->fillLength > 0) {
			framecount[pFullBuf->channelNum]++;
		}

		if(getostime() - t[pFullBuf->channelNum] >= 5000) {
			fprintf(stderr, "\n##### ch%d: %d x %d	video bitrate = %d kb/s, framerate = %d fps\n",
			        pFullBuf->channelNum,
			        pFullBuf->frameWidth, pFullBuf->frameHeight,
			        length[pFullBuf->channelNum] / 1 / 1024 * 8 / 5,
			        framecount[pFullBuf->channelNum] / 5);

			framecount[pFullBuf->channelNum] = 0;
			length[pFullBuf->channelNum] = 0;
			t[pFullBuf->channelNum] = getostime();
		}

#endif

		if(pFullBuf->channelNum == 0) {
			LiveVideoParam videoParam;
			//		get_video_hw(0, &width, &height);
			videoParam.nWidth = pFullBuf->frameWidth;
			videoParam.nHight = pFullBuf->frameHeight;

			p = (UInt8 *)(pFullBuf->addr);

			if(pFullBuf->isKeyFrame == TRUE) {
				if(p[4] == 0x27 || p[4] == 0x67 || p[4] == 0x47) {
					idr_header_length0 = ParseIDRHeader((unsigned char *)(pFullBuf->addr));
					memcpy(h264Header0, (unsigned char *)(pFullBuf->addr), idr_header_length0);
					LiveSendVideoDataToRecPlay(pFullBuf->fillLength, (unsigned char *)(hTempBuf),
					                           1, 0, 0, &videoParam);

				} else {
					memcpy(hTempBuf, h264Header0, idr_header_length0);
					memcpy(hTempBuf + idr_header_length0, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);
					LiveSendVideoDataToRecPlay(pFullBuf->fillLength, (unsigned char *)(hTempBuf),
					                           1, 0, 0, &videoParam);
				}
			} else {
				LiveSendVideoDataToRecPlay(pFullBuf->fillLength, (unsigned char *)(pFullBuf->addr),
				                           0, 0, 0, &videoParam);
			}
		}

		if(pFullBuf->channelNum == 1) {
			LiveVideoParam videoParam;
			//		get_video_hw(0, &width, &height);
			videoParam.nWidth = pFullBuf->frameWidth;
			videoParam.nHight = pFullBuf->frameHeight;

			p = (UInt8 *)(pFullBuf->addr);

			if(pFullBuf->isKeyFrame == TRUE) {
				if(p[4] == 0x27 || p[4] == 0x67 || p[4] == 0x47) {
					idr_header_length1 = ParseIDRHeader((unsigned char *)(pFullBuf->addr));
					memcpy(h264Header1, (unsigned char *)(pFullBuf->addr), idr_header_length1);
					LiveSendVideoDataToRecPlay(pFullBuf->fillLength, (unsigned char *)(hTempBuf1),
					                           1, 1, 0, &videoParam);

				} else {
					memcpy(hTempBuf1, h264Header1, idr_header_length1);
					memcpy(hTempBuf1 + idr_header_length1, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);
					LiveSendVideoDataToRecPlay(pFullBuf->fillLength, (unsigned char *)(hTempBuf1),
					                           1, 1, 0, &videoParam);
				}
			} else {
				LiveSendVideoDataToRecPlay(pFullBuf->fillLength, (unsigned char *)(pFullBuf->addr),
				                           0, 1, 0, &videoParam);
			}
		}

		status = OSA_quePut(empty_que, (Int32)pFullBuf, OSA_TIMEOUT_NONE);
		OSA_assert(status == 0);
	}

	return NULL;
}

static void *recv_6467_stream(bits_user_param *param)
{
	Int32	status	= -1;
	int32_t ret = 0;
	//	int32_t recvlen = 0;

	int32_t length = 0;
	int32_t width = 0;
	int32_t height = 0;
	int32_t flags = 0;
	int32_t count = 0;

	int8_t *data = NULL;

	Bitstream_Buf	*pEmptyBuf	= NULL;

	if(param == NULL) {
		fprintf(stderr, "recv_6467_stream: param is NULL!\n");
		return NULL;
	}

	OSA_QueHndl *empty_que = &(param->bits_empty_que);
	OSA_QueHndl *full_que  =  &(param->bits_full_que);
	//	audio_encdec_handle *pahandle = param->appdata;

	while(1) {
		ret = get_video_frame(&data, &length, &width, &height, &flags);

		if(ret < 0) {
			usleep(50000);
			continue;
		}

		status = OSA_queGet(empty_que, (Int32 *)(&pEmptyBuf), OSA_TIMEOUT_FOREVER);

		if(status != 0) {
			usleep(50000);
			continue;
		}

		memcpy((unsigned char *)pEmptyBuf->addr, data, length);

		pEmptyBuf->fillLength = length;
		pEmptyBuf->channelNum = 0;
		pEmptyBuf->frameWidth = 1920;//width;
		pEmptyBuf->frameHeight = 1080;//height;
		pEmptyBuf->timeStamp = getostime();
		pEmptyBuf->isKeyFrame = flags;
		pEmptyBuf->seqId = count++;
		pEmptyBuf->doNotDisplay = 1;

		r_free(data);

		status = OSA_quePut(full_que, (Int32)pEmptyBuf, OSA_TIMEOUT_NONE);

		if(status != 0) {
			usleep(50000);
			continue;
		}
	}

	return NULL;
}

static void *InHostStreamProcess2(bits_user_param *param)
{
	Int32 status = -1;
	uint32_t t[4] = {0};
	Int32 idr_header_length0 = 0;
	Int32 idr_header_length1 = 0;

	Int32 length[4] = {0};
	Int32 framecount[4] = {0};
	Bitstream_Buf	*pFullBuf;
	UInt8 *p = NULL;

	if(param == NULL) {
		fprintf(stderr, "InHostStreamProcess: param is NULL!\n");
		return NULL;
	}

	setLiveMaxUserNum(4);
	startLiveplayTask();

	OSA_QueHndl *full_que = &(param->bits_full_que);
	OSA_QueHndl *empty_que = &(param->bits_empty_que);

	char *hTempBuf0 = (char *)malloc(1920 * 1080);
	char *hTempBuf1 = (char *)malloc(1920 * 1080);

	unsigned char h264Header0[0x40] = {0};
	unsigned char h264Header1[0x40]	= {0};

	unsigned int filenum = 0;
	unsigned int totalM = 0;
	LiveVideoParam videoParam;


	while(TRUE == param->run_status) {
		status = OSA_queGet(full_que, (Int32 *)(&pFullBuf), OSA_TIMEOUT_FOREVER);
		OSA_assert(status == 0);


#if 1
		length[pFullBuf->channelNum] += pFullBuf->fillLength;

		if(pFullBuf->fillLength > 0) {
			framecount[pFullBuf->channelNum]++;
		}

		if(getostime() - t[pFullBuf->channelNum] >= 5000) {
			fprintf(stderr, "\n##### ch%d: %d x %d	video bitrate = %d kb/s, framerate = %d fps\n",
			        pFullBuf->channelNum,
			        pFullBuf->frameWidth, pFullBuf->frameHeight,
			        length[pFullBuf->channelNum] / 1 / 1024 * 8 / 5,
			        framecount[pFullBuf->channelNum] / 5);

			framecount[pFullBuf->channelNum] = 0;
			length[pFullBuf->channelNum] = 0;
			t[pFullBuf->channelNum] = getostime();
		}

#endif

		if(pFullBuf->channelNum == 1) {
			//LiveVideoParam videoParam;

			//		get_video_hw(0, &width, &height);
			videoParam.nWidth = pFullBuf->frameWidth;
			videoParam.nHight = pFullBuf->frameHeight;

			p = (UInt8 *)(pFullBuf->addr);

			if(pFullBuf->isKeyFrame == TRUE) {
				if(p[4] == 0x27 || p[4] == 0x67 || p[4] == 0x47) {
					idr_header_length1 = ParseIDRHeader((unsigned char *)(pFullBuf->addr));
					memcpy(h264Header1, (unsigned char *)(pFullBuf->addr), idr_header_length1);
					LiveSendVideoDataToRecPlay(pFullBuf->fillLength, (unsigned char *)(hTempBuf1),
					                           1, 0, 0, &videoParam);

#ifdef ENC110
					SendDataToClient110(pFullBuf->fillLength, (unsigned char *)(hTempBuf1),
					                    1, 0, pFullBuf->frameWidth, pFullBuf->frameHeight);
#else
					SendDataToClient(pFullBuf->fillLength, (unsigned char *)(hTempBuf1),
					                 1, 0, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif

				} else {
					memcpy(hTempBuf1, h264Header1, idr_header_length1);
					memcpy(hTempBuf1 + idr_header_length1, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);
					LiveSendVideoDataToRecPlay(pFullBuf->fillLength, (unsigned char *)(hTempBuf1),
					                           1, 0, 0, &videoParam);
#ifdef ENC110
					SendDataToClient110(pFullBuf->fillLength + idr_header_length1, (unsigned char *)hTempBuf1,
					                    1, 0, pFullBuf->frameWidth, pFullBuf->frameHeight);
#else
					SendDataToClient(pFullBuf->fillLength + idr_header_length1, (unsigned char *)hTempBuf1,
					                 1, 0, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif
				}
			} else {
				LiveSendVideoDataToRecPlay(pFullBuf->fillLength, (unsigned char *)(pFullBuf->addr),
				                           0, 0, 0, &videoParam);
#ifdef ENC110
				SendDataToClient110(pFullBuf->fillLength, (unsigned char *)(pFullBuf->addr),
				                    0, 0, pFullBuf->frameWidth, pFullBuf->frameHeight);
#else
				SendDataToClient(pFullBuf->fillLength, (unsigned char *)(pFullBuf->addr),
				                 0, 0, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif
			}
		}

		if(pFullBuf->channelNum == 2) {

			//	fwrite(pFullBuf->addr,pFullBuf->fillLength,1,fp);
			p = (UInt8 *)(pFullBuf->addr);

			if(pFullBuf->isKeyFrame == TRUE) {
				if(p[4] == 0x27 || p[4] == 0x67 || p[4] == 0x47) {
					idr_header_length0 = ParseIDRHeader((unsigned char *)(pFullBuf->addr));
					memcpy(h264Header0, (unsigned char *)(pFullBuf->addr), idr_header_length0);


#ifdef ENC110
					SendDataToClient110(pFullBuf->fillLength, (unsigned char *)(hTempBuf0),
					                    1, 1, pFullBuf->frameWidth, pFullBuf->frameHeight);
#else

#endif

				} else {
					memcpy(hTempBuf0, h264Header0, idr_header_length0);
					memcpy(hTempBuf0 + idr_header_length0, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);

#ifdef ENC110
					SendDataToClient110(pFullBuf->fillLength + idr_header_length0, (unsigned char *)hTempBuf0,
					                    1, 1, pFullBuf->frameWidth, pFullBuf->frameHeight);
#else

#endif
				}
			} else {

#ifdef ENC110
				SendDataToClient110(pFullBuf->fillLength, (unsigned char *)(pFullBuf->addr),
				                    0, 1, pFullBuf->frameWidth, pFullBuf->frameHeight);
#else

#endif
			}
		}



		status = OSA_quePut(empty_que, (Int32)pFullBuf, OSA_TIMEOUT_NONE);
		OSA_assert(status == 0);
	}

	return NULL;
}
static int isIDR(UInt8 *p)
{
	if(p[4] == 0x27 || p[4] == 0x67 || p[4] == 0x47) {
		return 1;
	}

	return 0;
}

static uint32_t getCurrentTime(void)
{
	struct timeval tv;
	struct timezone tz;
	uint32_t ultime;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return (ultime);
}

static int streamUdpSendInfoStat()
{

}

#ifdef HAVE_JPEG
static void test_fwrite_jpeg(int n, char *buf, int buflen)
{
	FILE *fp = NULL;

	char file[256] = {0};
	sprintf(file, "%d_file.jpg", n);

	if(NULL == fp) {
		fp =	fopen(file, "w");

		if(NULL == fp) {
			nslog(NS_ERROR, "fopen  : %s", strerror(errno));
			return -1;
		}

		printf("[write_file] buf : [%p], buflen : [%d] , fp : [%p]\n", buf, buflen, fp);
		fwrite(buf, buflen, 1, fp);
		fclose(fp);
	}


	return 0;
}

#endif
void *inHostStreamUdpSendProcess(bits_user_param *param)
{
	Int32 status = -1;
	uint32_t t[4] = {0};
	Int32 idr_header_length = 0;
	Int32 idr_header_length0 = 0;
	Int32 idr_header_length1 = 0;

	Int32 length[4] = {0};
	Int32 framecount[4] = {0};
	Bitstream_Buf	*pFullBuf;
	UInt8 *p = NULL;

	if(param == NULL) {
		fprintf(stderr, "InHostStreamProcess: param is NULL!\n");
		return NULL;
	}


	OSA_QueHndl *full_que = &(param->bits_full_que);
	OSA_QueHndl *empty_que = &(param->bits_empty_que);
	udp_send_module_handle *pUdpSend = param->appdata;
	nslog(NS_INFO, "[inHostStreamUdpSendProcess] port:[%x] ip:[%s] user_data:[%p]\n",
	      pUdpSend->udp_hand.snd_video_port,
	      pUdpSend->udp_hand.snd_ip,
	      pUdpSend->udp_hand.src_data);
	unsigned char h264Header[0x40] = {0};
	unsigned char h264Header0[0x40] = {0};
	unsigned char h264Header1[0x40] = {0};


	unsigned int filenum = 0;
	unsigned int totalM = 0;
	LiveVideoParam videoParam;
	frame_info_t frame_info;
	char *hTempBuf0 = (char *)malloc(MAX_FRAME_LEN);
	char *hTempBuf1 = (char *)malloc(MAX_FRAME_LEN);

	int x = 0;

	while(TRUE == param->run_status) {
		status = OSA_queGet(full_que, (Int32 *)(&pFullBuf), OSA_TIMEOUT_FOREVER);
		OSA_assert(status == 0);


#if 1
		length[pFullBuf->channelNum] += pFullBuf->fillLength;

		if(pFullBuf->fillLength > 0) {
			framecount[pFullBuf->channelNum]++;
		}

		if(getostime() - t[pFullBuf->channelNum] >= 5000) {
			nslog(NS_WARN, "\n##### ch%d: %d x %d	video bitrate = %d kb/s, framerate = %d fps\n",
			      pFullBuf->channelNum,
			      pFullBuf->frameWidth, pFullBuf->frameHeight,
			      length[pFullBuf->channelNum] / 1 / 1024 * 8 / 5,
			      framecount[pFullBuf->channelNum] / 5);
			printf( "\n##### ch%d: %d x %d	video bitrate = %d kb/s, framerate = %d fps\n",
			      pFullBuf->channelNum,
			      pFullBuf->frameWidth, pFullBuf->frameHeight,
			      length[pFullBuf->channelNum] / 1 / 1024 * 8 / 5,
			      framecount[pFullBuf->channelNum] / 5);

			framecount[pFullBuf->channelNum] = 0;
			length[pFullBuf->channelNum] = 0;
			t[pFullBuf->channelNum] = getostime();
		}

#endif

		if(pFullBuf->channelNum == 0) {
			frame_info.m_width = pFullBuf->frameWidth;
			frame_info.m_hight = pFullBuf->frameHeight;
			frame_info.m_frame_length = pFullBuf->fillLength;
			frame_info.time_tick = getCurrentTime();//pFullBuf->timeStamp;
			frame_info.is_blue = getVGADetect() ? 0 : 1;
			frame_info.m_dw_flags = 0;
			frame_info.m_frame_rate = 25;
			frame_info.m_data_codec = H264_CODEC_TYPE;
			p = (UInt8 *)(pFullBuf->addr);

			if(TRUE == pFullBuf->isKeyFrame) {
				frame_info.m_dw_flags = AVIIF_KEYFRAME;

				if(isIDR(p)) {
					idr_header_length = ParseIDRHeader((unsigned char *)(pFullBuf->addr));
					memcpy(h264Header, (unsigned char *)(pFullBuf->addr), idr_header_length);

					memcpy(pUdpSend->udp_hand.src_data, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);

					UdpSend_rtp_data(pUdpSend, &frame_info);
#ifdef ENC110
					SendDataToClient110(pFullBuf->fillLength, (unsigned char *)(pUdpSend->udp_hand.src_data),
					                    1, 2, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif

				} else {
					memcpy(pUdpSend->udp_hand.src_data, h264Header, idr_header_length);
					memcpy(pUdpSend->udp_hand.src_data + idr_header_length, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);
					frame_info.m_frame_length = frame_info.m_frame_length + idr_header_length;
					UdpSend_rtp_data(pUdpSend, &frame_info);
#ifdef ENC110
					SendDataToClient110(pFullBuf->fillLength + idr_header_length, (unsigned char *)(pUdpSend->udp_hand.src_data),
					                    1, 2, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif
				}
			} else {
				memcpy(pUdpSend->udp_hand.src_data, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);
				UdpSend_rtp_data(pUdpSend, &frame_info);

#ifdef ENC110
				SendDataToClient110(pFullBuf->fillLength, (unsigned char *)(pUdpSend->udp_hand.src_data),
				                    1, 2, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif
			}
		}


		if(pFullBuf->channelNum == 1) {
			//LiveVideoParam videoParam;

			//		get_video_hw(0, &width, &height);
			videoParam.nWidth = pFullBuf->frameWidth;
			videoParam.nHight = pFullBuf->frameHeight;

			p = (UInt8 *)(pFullBuf->addr);

			if(pFullBuf->isKeyFrame == TRUE) {
				if(p[4] == 0x27 || p[4] == 0x67 || p[4] == 0x47) {
					idr_header_length1 = ParseIDRHeader((unsigned char *)(pFullBuf->addr));
					memcpy(h264Header1, (unsigned char *)(pFullBuf->addr), idr_header_length1);

#ifdef ENC110
					SendDataToClient110(pFullBuf->fillLength, (unsigned char *)(pFullBuf->addr),
					                    1, 0, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif

				} else {
					memcpy(hTempBuf1, h264Header1, idr_header_length1);
					memcpy(hTempBuf1 + idr_header_length1, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);
#ifdef ENC110
					SendDataToClient110(pFullBuf->fillLength + idr_header_length1, (unsigned char *)hTempBuf1,
					                    1, 0, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif
				}
			} else {
#ifdef ENC110
				SendDataToClient110(pFullBuf->fillLength, (unsigned char *)(pFullBuf->addr),
				                    0, 0, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif
			}
		}

		if(pFullBuf->channelNum == 2) {

			//	fwrite(pFullBuf->addr,pFullBuf->fillLength,1,fp);
			p = (UInt8 *)(pFullBuf->addr);

			if(pFullBuf->isKeyFrame == TRUE) {
				if(p[4] == 0x27 || p[4] == 0x67 || p[4] == 0x47) {
					idr_header_length0 = ParseIDRHeader((unsigned char *)(pFullBuf->addr));
					memcpy(h264Header0, (unsigned char *)(pFullBuf->addr), idr_header_length0);


#ifdef ENC110
					SendDataToClient110(pFullBuf->fillLength, (unsigned char *)(pFullBuf->addr),
					                    1, 1, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif

				} else {
					memcpy(hTempBuf0, h264Header0, idr_header_length0);
					memcpy(hTempBuf0 + idr_header_length0, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);

#ifdef ENC110
					SendDataToClient110(pFullBuf->fillLength + idr_header_length0, (unsigned char *)hTempBuf0,
					                    1, 1, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif
				}
			} else {

#ifdef ENC110
				SendDataToClient110(pFullBuf->fillLength, (unsigned char *)(pFullBuf->addr),
				                    0, 1, pFullBuf->frameWidth, pFullBuf->frameHeight);
#endif
			}
		}

		if(3 == pFullBuf->channelNum) {
#ifdef HAVE_JPEG
			printf("---------3---len=%d---w=%d--------------\n", pFullBuf->fillLength, pFullBuf->frameWidth);
			test_fwrite_jpeg(x++, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);
#endif
		}

		status = OSA_quePut(empty_que, (Int32)pFullBuf, OSA_TIMEOUT_NONE);
		OSA_assert(status == 0);
	}

	return NULL;
}


static int  req_enc_bitrate(int sockfd, char *ip)
{
	msg_header_t login_msg;
	char login_buf[MSG_VAL_LEN] = {0}, send_buf[MSG_VAL_LEN] = {0};
	int out_len = 0;
	int return_code = OPERATION_ERR;

#if 1//test request stream
	memset(login_buf, 0, MSG_VAL_LEN);
	memset(&login_msg, 0, sizeof(msg_header_t));

	snprintf(login_buf, 5, "%s", "A123");
	out_len = sizeof("A123");

	login_msg.m_len = htons(out_len + sizeof(msg_header_t));
	login_msg.m_ver = 0;
	login_msg.m_msg_type = 9;

	memcpy(send_buf, &login_msg, sizeof(msg_header_t));
	memcpy(send_buf + sizeof(msg_header_t), login_buf, out_len);
	//nslog(NS_INFO, "sockfd = %d, stream_id = %d", M_handle->sockfd, M_handle->stream_id);
	return_code = send(sockfd, send_buf, out_len + sizeof(msg_header_t), 0);

	if(return_code < 0) {
		err_print("send request bitrate error, sockfd = %d\n", sockfd);
		return OPERATION_ERR;
	}

	return OPERATION_SUCC;
#endif
}



typedef struct _stream_handle {
	int32_t offset;		//a offset  for organizing  frame;
	int8_t *frame_data;//a buf  for organizing  frame .
	int32_t  audio_offset;
	int8_t *audio_data;
	int32_t stream_id;

	int32_t debug_write_flag;
	//	recv_time udp_recv_time_t;

} stream_handle_t;

static int recv_enc_tcp_datas(int sockfd, void *buf, int buflen)
{
	int total_recv = 0;
	int recv_len = 0;
	int count = 0;

	while(total_recv < buflen) {

		recv_len = recv(sockfd, buf + total_recv, buflen - total_recv, 0);

		if(recv_len < 1 && EINTR != errno) {
			err_print("recv tcp data failed, recv_len = %d\n", recv_len);

			if(0 == recv_len) {
				return 0;
			}

			return -1;
		} else if(recv_len < 1 && EINTR == errno) {
			err_print("recv tcp data failed\n");
			usleep(10000);
			continue;
		}

		total_recv += recv_len;

		if(total_recv == buflen) {
			break;
		}
	}

	return total_recv;
}
static int recvFirstIFrame[RECV_STREAMNUM] = {0};
static int recvEmptyBufCount[RECV_STREAMNUM] = {0};
static Int32 getVideoStreamPutVpss(udp_recv_stream_info_t *p_stream_info, char *data, int len)
{
	RecFrame	frame;
	ST_MARK 	mark;
	Int32 status;
	Bitstream_Buf		*pEmptyBuf = NULL;
	bits_user_param *pUserData = (bits_user_param *)(p_stream_info->user_data);
	OSA_QueHndl *full_que = (OSA_QueHndl *) & (pUserData->bits_full_que);
	OSA_QueHndl *empty_que = (OSA_QueHndl *) & (pUserData->bits_empty_que);
	void	 *dechand  =  NULL;
	Int32 idr = 0;
	int index = eindexToChannel(p_stream_info->port - HD_SRC_BASE_PROT);

	if(I_FRAME == p_stream_info->IDR_FLAG) {
		recvFirstIFrame[index] = 1;
	}

	if(!recvFirstIFrame[index]) {
		printf("[getVideoStreamPutVpss] Not iFirstIFrame!!!\n");
		return 0;
	}

	status = OSA_queGet(empty_que, (Int32 *)(&pEmptyBuf), OSA_TIMEOUT_FOREVER);
	OSA_assert(status == 0);
	OSA_assert(pEmptyBuf != NULL);

	if(H264_CODEC_TYPE == p_stream_info->type) {

		memcpy(pEmptyBuf->addr, data, len);

		if(I_FRAME == p_stream_info->IDR_FLAG) {
			idr = 1;
		} else {
			idr = 0;
		}

		pEmptyBuf->fillLength = len;

		if(idr == 1) {
			printf("pEmptyBuf->fillLength == %d\n", pEmptyBuf->fillLength);
		}

		pEmptyBuf->channelNum = index;
		pEmptyBuf->frameWidth = p_stream_info->width;
		pEmptyBuf->frameHeight = p_stream_info->height;
		pEmptyBuf->timeStamp = p_stream_info->timestamp;
		pEmptyBuf->isKeyFrame = idr;
		pEmptyBuf->seqId = recvEmptyBufCount[index];
		pEmptyBuf->doNotDisplay = 1;

		if(0 == recvEmptyBufCount[index]) {
			pEmptyBuf->inputFileChanged = 1;
		} else {
			pEmptyBuf->inputFileChanged = 0;
		}

	}

	status = OSA_quePut(full_que, (Int32)pEmptyBuf, OSA_TIMEOUT_NONE);
	OSA_assert(status == 0);
	pEmptyBuf = NULL;
	recvEmptyBufCount[index] = 1;
	//printf("[getVideoStreamPutVpss] end..\n");
}

#define	udp_stream_recv_init	udp_recv_init
#define	udp_stream_recv_deinit	udp_recv_deinit
#define	udp_stream_recv_start	udp_recv_start
static int FindH264StartNAL1(unsigned char *pp)
{
	/*is for 00 00 00 01 Nal header*/
	if(pp[0]!=0 || pp[1]!=0 || pp[2] !=0 || pp[3] != 1)
		return 0;
	else
		return 1;
}

Int32 ReadH264File(FILE *file, unsigned char * buffer, int *idr)
{

	int len = 0;
	int framelen = 0;
	unsigned char h2640 = 0;
	unsigned char h2641 = 0;
	unsigned char h2642 = 0;
	unsigned char h2643 = 1;
//	printf("sssss\n");
	len = fread(buffer,1,4,file);
	if(len != 4)
	{
		fseek(file, 0,SEEK_SET);
		return 0;
	}
	else
	{
	
	}
//	printf("sssss111\n");
	if(1 == FindH264StartNAL1(buffer))
	{
		framelen = 4;
		printf("first Frame IDR");
	}
	else
	{
		memcpy(&buffer[4], &buffer[0],4);
		
		buffer[0] = 0x0;
		buffer[1] = 0x0;
		buffer[2] = 0x0;
		buffer[3] = 0x1;
		
		framelen = 8;
	}
	
	char *p = &buffer[framelen];
	while(1)
	{
	
		len = fread(p,1,1,file);
		if(len != 1)
		{
			fseek(file, 0,SEEK_SET);
			return 0;
		}
		
		h2640 = p[-3];
		h2641 = p[-2];
		h2642 = p[-1];
		h2643 = p[0];
		framelen ++;
		
		//printf("%x %x %x %x \n",h2640,h2641,h2642,h2643);
		if((h2640 == 0x0 )&&(h2641 == 0x0 )&&(h2642 == 0x0 )&&(h2643 == 0x1 ))
		{
			//	printf("find one frame\n");
			
			framelen	= framelen - 4;
			break;
		}
		
		p++;
	}
	
	if(buffer[4] == 0x27)
	{
		int curlen = 0;
		int curidr = 0;
		*idr = 1;
		
		curlen = ReadH264File(file, buffer + framelen,&curidr);
		if(curlen <= 0)
		{
			return 0;
		}
		else
		{
			framelen	= framelen + curlen;
		}
	}
	
	if(buffer[4] == 0x28)
	{
		int curidr = 0;
		int curlen = 0;
		*idr = 1;
		
		curlen = ReadH264File(file, buffer + framelen,&curidr);
		if(curlen <= 0)
		{
			return 0;
		}
		else
		{
			framelen = framelen + curlen;
		}
	}

	if(buffer[4] == 0x6)
	{
		int curidr = 0;
		int curlen = 0;
		*idr = 1;
		
		curlen = ReadH264File(file, buffer + framelen,&curidr);
		if(curlen <= 0)
		{
			return 0;
		}
		else
		{
			framelen = framelen + curlen;
		}
	}
	
	//printf("%x %x %x %x %x %x %x %x\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[framelen-4]\
	//,buffer[framelen -3],buffer[framelen -2],buffer[framelen -1]);
	
	return framelen;
	
}
Void *UdpRecvFrame_drvTsk(Void *prm)
{
	printf("[UdpRecvFrame_drvTsk] start ...\n");
	RecvHand *recvhand = (RecvHand *)prm;
	bits_user_param *param = (bits_user_param *)recvhand->hand;

	stream_recv_cond_t src;
#if 1
	get_local_ip(ETH_NAME, src.ip);
	src.port = HD_SRC_BASE_PROT + recvhand->index;
	printf("[UdpRecvFrame_drvTsk] index:[%d] ip:[%s] port:[%d]\n",
	       recvhand->index, src.ip, src.port);
	src.user_data = param;
	src.func = getVideoStreamPutVpss;
	udp_recv_handle *phandle;
	phandle = udp_stream_recv_init(&src);

	if(NULL == phandle) {
		udp_stream_recv_deinit(phandle);
		printf("[UdpRecvFrame_drvTsk] udp_stream_recv_init is NULL!!!\n");
		return NULL;
	}

	udp_stream_recv_start(phandle);
#else
	unsigned int t = 0;
	unsigned int length = 0;
	unsigned int framecount = 0;
	Bitstream_Buf		*pEmptyBuf = NULL;
	unsigned int count = 0;
	int status = 0;
	
	FILE *file = fopen("teacher.h264","r");
	if(file == NULL)
	{
		assert(0);
	}
	
	while(1)
	{
		int idr = 0;
		usleep(1000*40);
		int len = 0;
		status = OSA_queGet(&param->bits_empty_que, (Int32 *)(&pEmptyBuf), OSA_TIMEOUT_FOREVER);
		OSA_assert(status == 0);
		OSA_assert(pEmptyBuf != NULL);
		len = ReadH264File(file, pEmptyBuf->addr,&idr);
		if(len == 0)
		{
			len = ReadH264File(file, pEmptyBuf->addr,&idr);
			assert(len != 0);
		}
		else
		{
			pEmptyBuf->fillLength = len;
			
				//printf("pEmptyBuf->fillLength == %d\n",pEmptyBuf->fillLength);
		}

		pEmptyBuf->channelNum = recvhand->index;
		pEmptyBuf->frameWidth = 1280;
		pEmptyBuf->frameHeight = 720;
		pEmptyBuf->timeStamp = 0;
		pEmptyBuf->isKeyFrame =idr;
		pEmptyBuf->seqId = count;
		pEmptyBuf->doNotDisplay = 1;
		if(count == 0)
			pEmptyBuf->inputFileChanged = 1;
		else
			pEmptyBuf->inputFileChanged = 0;

		length += pEmptyBuf->fillLength;
		framecount++;
		
		
		if(getostime() - t >= 5000)
		{
			fprintf(stderr, "\n DEC ch%d: %d x %d	video bitrate = %d kb/s, framerate = %d fps\n",
			        pEmptyBuf->channelNum,
			        pEmptyBuf->frameWidth, pEmptyBuf->frameHeight,
			        length / 1 / 1024 * 8 / 5,
			        framecount / 5);
			        
			framecount = 0;
			length = 0;
			t = getostime();
		}

		status = OSA_quePut(&param->bits_full_que, (Int32)pEmptyBuf, OSA_TIMEOUT_NONE);
		OSA_assert(status == 0);
		pEmptyBuf = NULL;
		count++;
	}

	

#endif
	printf("[UdpRecvFrame_drvTsk] recvhand->index:[%d] end ...\n", recvhand->index);
}

void *OutHostStreamProcess(bits_user_param *param)
{
	Int32 status = -1;
	Int32 i = 0;
	char ipaddr[20] = {0};
	void *handle = NULL;

	if(param == NULL) {
		fprintf(stderr, "OutHostStreamP970rocess: param is NULL!\n");
		return NULL;
	}

	REC_CLIENT *pinst = handle;
	RecvHand recvhand[RECV_STREAMNUM];
	OSA_ThrHndl 				TaskHand[RECV_STREAMNUM];

	for(i = 0; i < RECV_STREAMNUM ; i++) {
		recvhand[i].hand	= param;

		printf("[OutHostStreamProcess] [%d] empty_que:[%p] full_que:[%p]\n", i, &(param->bits_empty_que), &(param->bits_full_que));
		recvhand[i].index	= g_eindex[i];
		recvhand[i].socktid = -1;
		recvhand[i].IsRun	= TRUE;
		recvhand[i].IsAudio = 0;

		status = OSA_thrCreate(&TaskHand[i],
		                       (OSA_ThrEntryFunc)UdpRecvFrame_drvTsk,
		                       99,
		                       0,
		                       &recvhand[i]);
		OSA_assert(status == 0);
	}

	for(i = 0; i < RECV_STREAMNUM; i++) {
		pthread_join(TaskHand[i].hndl, NULL);
	}

	return NULL;
}


//Ç¿ÖÆÇëÇóIÖ¡
int app_video_request_iframe(int channel)
{

	enc_force_iframe(gEduKit->encoderLink.encLink.link_id, channel);
	return 0;
}

int setVideoEncodeParam(int chId, VideoEncodeParam *vinfo)
{
	enc_set_fps(gEduKit->encoderLink.encLink.link_id, chId, vinfo->nFrameRate, vinfo->sBitrate * 1000);
}

int getVideoEncodeParam(int chId, VideoEncodeParam *vinfo)
{
	EncLink_GetDynParams vParams;
	vParams.chId = chId;
	enc_get_dynparams(gEduKit->encoderLink.encLink.link_id, &vParams);
	vinfo->IFrameinterval = vParams.intraFrameInterval;
	vinfo->nFrameRate = vParams.targetFps / 1000;
	vinfo->sBitrate = vParams.targetBitRate / 1000;
	vinfo->uWidth = vParams.inputWidth;
	vinfo->uHeight = vParams.inputHeight;
}


