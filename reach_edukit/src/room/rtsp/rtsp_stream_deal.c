/*****************************************************************
*  处理stream流数据
*
******************************************************************/


#if 0
#ifdef USE_LINUX_PLATFORM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#ifdef USE_WINDOWS_PLATFORM
#include <stdio.h>
#include <stdlib.h>
//#include <windows.h>
#include "../stdafx.h"
#include "../IniFile.h"
#include "../RecService.h"
#include "../Key.h"
#include "../Msg.h"
#include "../Room.h"
#endif


#include "mid_platform.h"
#include "b64.h"
#include "rtsp.h"
#include "rtsp_stream_deal.h"
#include "app_rtsp_center.h"

#ifdef USE_LINUX_PLATFORM
#include "../log/log_common.h"
#endif

//using namespace std;
static stream_video_handle g_stream_video_handle[MAX_ENCODE_NUM] = {NULL, NULL, NULL, NULL};
static stream_audio_handle g_stream_audio_handle = NULL ;
static int g_stream_init = 0;
//the encode card num
static int g_stream_room_num = 0;
static int FindH264StartNAL(unsigned char *pp);
static int rtsp_stream_get_nal_unit_type(unsigned char nal_code);
static int rtsp_stream_video_filter_sdp_info(unsigned char *pdata, int plen, char *sps_info, int *sps_info_len,
        unsigned 	char *pps_info, int *pps_info_len);
//static int rtsp_stream_get_nal_unit_type(char nal_code);
static void rtsp_sream_reset_av_frame(ONE_FRAME_S *frame);
//static int rtsp_porting_get_nal_unit_type( char nal_code);
static int rtsp_stream_set_room_num(int num);
static int rtsp_audio_stream_init(int id);
static int rtsp_video_stream_init(int num);
static unsigned int rtsp_stream_audio_filter_sdp_info(unsigned char *pdata);
// set encode card num
static int rtsp_stream_set_room_num(int num)
{
	g_stream_room_num  = num;
	return 0;
}

//get encode card num
int rtsp_stream_get_room_num()
{
	return g_stream_room_num;
}



static int rtsp_porting_get_nal_unit_type(char nal_code)
{
	unsigned char nal_unit_type = 0;
	unsigned char nal_ref_idc = 0;
	nal_unit_type = nal_code & 0x1f;
	nal_ref_idc = nal_code & 0x60;

	if(nal_unit_type == 7 || nal_unit_type == 8) {
		if(nal_ref_idc == 0) {
			return -1;
		}

		return nal_unit_type;
	}

	return nal_unit_type;

}


static void rtsp_sream_reset_av_frame(ONE_FRAME_S *frame)
{
	frame->wlen = 0;
	frame->status = 0;
	memset(&(frame->head), 0, sizeof(FRAMEHEAD));
	return ;
}

/************************************************************
*  输入：一个带 FRAMEHEAD的数据包 roomid 表示0,1,2,3
*
*
*
*************************************************************/
int rtsp_stream_rebuild_audio_frame(unsigned char *buff, unsigned int len , int id)
{
	//PRINTF("HAHAHAHA\n");
	FRAMEHEAD *head = (FRAMEHEAD *)buff;
	ONE_FRAME_S *temp = NULL;
	stream_audio_handle handle = NULL;
	int config = 0;
	//	if(app_center_get_stream_flag() == APP_CENTER_STOP)
	//	{
	//		return -1;
	//	}

	if(g_stream_audio_handle == NULL) {
		ERR_PRN("ERROR\n");
		return -1;
	}

	if(len < sizeof(FRAMEHEAD)) {
		PRINTF("ERROR.\n");
		return -1;
	}


	handle = g_stream_audio_handle;
	temp = &(handle->audio_frame);

	if(head->nFrameLength > MAX_AUDIO_LEN) {
		PRINTF("audio LEN is to big,the len = %u\n", head->nFrameLength);
		return -1;
	}

	if(head->dwSegment == 3 || head->dwSegment == 2) {
		if(temp->wlen != 0 || temp->status != FRAME_NULL) {
			PRINTF("ERROR! the aduio  priv frame is not complete");
			PRINTF("ERROR!,the aduio  need len = %u,the w_len= %u\n", temp->head.nFrameLength, temp->wlen);
			rtsp_sream_reset_av_frame(temp);
		}
	} else if((head->dwSegment == 0 || head->dwSegment == 1) && temp->status != FRAME_BUILDING) {
		PRINTF("ERROR!,the audio frame is not complete\n");
		rtsp_sream_reset_av_frame(temp);
		return -1;
	}

	if(head->dwSegment == 2 || head->dwSegment == 3) {
		memcpy(&(temp->head) , head, sizeof(FRAMEHEAD));

		memcpy(temp->data + temp->wlen, buff + sizeof(FRAMEHEAD), len - sizeof(FRAMEHEAD));
		temp->wlen += len - sizeof(FRAMEHEAD);
		temp->status = FRAME_BUILDING;
	} else if(head->dwSegment == 0 || head->dwSegment == 1) {
		memcpy(temp->data + temp->wlen, buff + sizeof(FRAMEHEAD), len - sizeof(FRAMEHEAD));
		temp->wlen += len - sizeof(FRAMEHEAD);
	}

	if(temp->wlen == temp->head.nFrameLength && temp->wlen != 0) {
		temp->status = FRAME_COMPLETE;
	}

	//have a complete frame
	if(temp->status == FRAME_COMPLETE) {
		int sleep_time = 0;
		{
#if 1
			sleep_time = 0;

			if(handle->last_audio_current_time != 0) {
				sleep_time = (temp->head.nTimeTick - handle->last_audio_rtp_time) - (mid_plat_get_runtime() - handle->last_audio_current_time);
				//if(sleep_time > 3)
				//	PRINTF("need audio sleep time = %d,%u\n",sleep_time,temp->head.nTimeTick);
				//	if(sleep_time > 0)
				//		;//usleep(sleep_time *1000);
			}

#endif

			//加入逻辑，用于分析ADTS头
			// resver for future
			if(handle->audio_config <= 0 || handle->samplerate != temp->head.nHight) {
				config =  rtsp_stream_audio_filter_sdp_info(temp->data);

				if(config == -1) {
					PRINTF("ERROR,i can't fiter the audio sdp\n");
				} else {
					PRINTF("I find the config,the samplerate =%u\n", handle->samplerate);
					handle->audio_config = config;
					handle->samplerate  = temp->head.nHight;
				}
			}

			//处理video buff
			//printf("RtspGetClientNum() = %d\n",0);
#if 0

			if(handle->last_audio_rtp_time == 0) {
				temp->head.nTimeTick = 2048 / 2;
			} else {
				temp->head.nTimeTick = handle->last_audio_rtp_time + 2048 / 2;
			}

#endif
			//	if(handle->last_audio_rtp_time > temp->head.nTimeTick)
			//	{
			//		printf("ERROR,audio!!!!!!!!!!!1!!!!!!!!!!!!\n");
			//		printf("ERROR!!!!!!!!!!!2!!!!!last_audio_rtp_time=%x!!!!!!!\n",handle->last_audio_rtp_time);
			//		printf("ERROR!!!!!!!!!!!3!!!!!!!head=%x!!!!!\n",temp->head.nTimeTick);
			//	}
			//add by zhangmin
#ifdef GDM_RTSP_SERVER

			if((app_center_get_stream_flag() != APP_CENTER_STOP) && (handle->audio_config > 0) && (RtspClientIsNull() != 0))
#else
			if((handle->audio_config > 0) && (RtspClientIsNull() != 0))
#endif
			{
				if(handle->last_audio_current_time == 0 || handle->begin_audio_time >  temp->head.nTimeTick)
					//	|| ((temp->head.nTimeTick-handle->begin_audio_time) > 0xffffffff/90 ))  //4//4 housrs
				{
					PRINTF("###############warnning ,audio the begin time > ntimetick,%x,%x\n", handle->begin_audio_time, temp->head.nTimeTick);
					handle->begin_audio_time =  temp->head.nTimeTick;
					//handle->seq = 0;
				}

				handle->last_audio_rtp_time  = temp->head.nTimeTick - handle->begin_audio_time;

				//test
				//	handle->last_audio_rtp_time = 0;
				//				static int g_test_audio= 0;
				//				if(g_test_audio == 0)
				//					handle->last_audio_rtp_time += 0xffffffff/90 -30*60*1000;
				if(handle->last_audio_rtp_time > 0xffffffff / 48) {
					PRINTF("################warnning ,audio2 the begin time > ntimetick,%x,%x\n", handle->begin_audio_time, temp->head.nTimeTick);
					handle->begin_audio_time =  temp->head.nTimeTick;
					handle->last_audio_rtp_time = 0;
					//RtspAdudioSeqReset();
					//					g_test_audio = 1;
				}

				//	if(handle->last_audio_rtp_time  < 0xffffffff/48)
				RtspAudioPack(temp->wlen, (temp->data), handle->last_audio_rtp_time, 0, handle->samplerate);
			} else {
				handle->begin_audio_time = temp->head.nTimeTick;
				handle->last_audio_current_time = 0;
			}

			handle->channel = temp->head.nWidth; //复用为audio sample rate ,
			//handle->samplerate = temp->head.nHight;
			handle->last_audio_current_time = mid_plat_get_runtime();
			//handle->last_audio_rtp_time  = temp->head.nTimeTick;

		}
		rtsp_sream_reset_av_frame(temp);
	}


	return 0;

}

//rebuild video frame
int rtsp_stream_rebuild_video_frame(unsigned char *buff, unsigned int len , int id)
{
	int ret = 0;
	FRAMEHEAD *head = (FRAMEHEAD *)buff;
	ONE_FRAME_S *temp = NULL;
	int idr_flag = 0;
	stream_video_handle handle = g_stream_video_handle[id];

	//	if(app_center_get_stream_flag() == APP_CENTER_STOP)
	//	{
	//		return -1;
	//	}

	if(id < 0 || id > rtsp_stream_get_room_num() - 1 || g_stream_video_handle[id] == NULL) {
		ERR_PRN("video rebuild,roomid =%d,the g_stream_num =%d\n", id, rtsp_stream_get_room_num());
		return -1;
	}

	if(len < sizeof(FRAMEHEAD)) {
		PRINTF("video rebuild,ERROR.the frame len is too small.\n");
		return -1;
	}

	temp = &(handle->video_frame);

	if(head->nFrameLength > MAX_VIDEO_LEN) {
		PRINTF("video rebuild,LEN is to big,the len = %u\n", head->nFrameLength);
		return -1;
	}

	if(head->dwSegment == 3 || head->dwSegment == 2) {
		if(temp->wlen != 0 || temp->status != FRAME_NULL) {
			PRINTF("ERROR! video rebuild,the priv frame is not complete");
			PRINTF("ERROR!,video rebuild,the need len = %u,the w_len= %u\n", temp->head.nFrameLength, temp->wlen);
			rtsp_sream_reset_av_frame(temp);
		}
	} else if((head->dwSegment == 0 || head->dwSegment == 1) && temp->status != FRAME_BUILDING) {
		PRINTF("ERROR!video rebuild,the frame is not complete.\n");
		rtsp_sream_reset_av_frame(temp);
		return -1;
	}

	if(head->dwSegment == 2 || head->dwSegment == 3) {
		memcpy(&(temp->head) , head, sizeof(FRAMEHEAD));

		memcpy(temp->data + temp->wlen, buff + sizeof(FRAMEHEAD), len - sizeof(FRAMEHEAD));
		temp->wlen += len - sizeof(FRAMEHEAD);
		temp->status = FRAME_BUILDING;
	} else if(head->dwSegment == 0 || head->dwSegment == 1) {
		memcpy(temp->data + temp->wlen, buff + sizeof(FRAMEHEAD), len - sizeof(FRAMEHEAD));
		temp->wlen += len - sizeof(FRAMEHEAD);
	}

	if(temp->wlen == temp->head.nFrameLength && temp->wlen != 0) {
		temp->status = FRAME_COMPLETE;
	}

	if(temp->status == FRAME_COMPLETE) {
		int sleep_time = 0;
		{
#if 1
			sleep_time = 0;

			if(handle->last_video_current_time != 0) {
				sleep_time = (temp->head.nTimeTick - handle->last_video_rtp_time) - (mid_plat_get_runtime() - handle->last_video_current_time);
				//if(sleep_time > 15)
				//	PRINTF("need video sleep time = %d,%u\n",sleep_time,temp->head.nTimeTick);
				//	if(sleep_time > 0)
				//		;//usleep(sleep_time *1000);
			}

			if(temp->head.dwFlags == AVIIF_KEYFRAME)
				;//	PRINTF("video rebuild .temp->head.dwFlags = %x,,,%d-==%d\n",temp->head.dwFlags,temp->head.nWidth,handle->width);

			if((temp->head.dwFlags == AVIIF_KEYFRAME)
			   && ((handle->sdp_flag == 0) || (temp->head.nWidth != handle->width || temp->head.nHight != handle->height))) {
				PRINTF("video rebuilded,i will parse idr header \n");
				//rtsp_change_sdp_info(id);
				ret = rtsp_stream_video_filter_sdp_info(temp->data, temp->wlen, (char *)handle->sps_info, (int *) & (handle->sps_info_len),
				                                        handle->pps_info, (int *) & (handle->pps_info_len));

				if(ret < 0) {
					ERR_PRN("Error! video rebuild,parse sdp info error\n");
				} else {
					PRINTF("video rebuild,success,parse sdp info ok.\n");
					handle->sdp_flag = 1;
					handle->width = temp->head.nWidth;
					handle->height = temp->head.nHight;
				}
			}

			//	if(handle->last_video_rtp_time > temp->head.nTimeTick)
			//	{
			///		printf("ERROR,video!!!!!!!!!!!1!!!!!!!!!!!!\n");
			//		printf("ERROR!!!!!!!!!!!2!!!!!!!!last head =%x!!!!\n",handle->last_video_rtp_time);
			//		printf("ERROR!!!!!!!!!!!3!!!!!!!!!head=%x!!!\n",temp->head.nTimeTick);
			//	}

			//处理video buff
			//add by zhangmin
#ifdef GDM_RTSP_SERVER

			if((app_center_get_stream_flag() != APP_CENTER_STOP) && RtspClientIsNull() != 0)
#else
			if(RtspClientIsNull() != 0)
#endif
			{
				//考虑切分辨率的效果问题
				//quicttime client must close
				if(handle->width != temp->head.nWidth  || handle->height != temp->head.nHight) {
					rtsp_change_sdp_info(id);
					PRINTF("video rebuild,the width changed,so this buff do not seed to rtsp client\n");
				}

				//vlc client is OK
				if(handle->last_video_current_time  == 0 || handle->begin_video_time >  temp->head.nTimeTick)
					//	|| ((temp->head.nTimeTick-handle->begin_video_time) > 3*60*1000 )) //4
				{
					PRINTF("##################warnning ,video the begin time > ntimetick,%x,%x\n", handle->begin_video_time, temp->head.nTimeTick);
					handle->begin_video_time =  temp->head.nTimeTick;
					//handle->seq = 0;
				}

				handle->last_video_rtp_time  = temp->head.nTimeTick - handle->begin_video_time;

				//test
				//handle->last_video_rtp_time = 0;
				//				static int g_test_video= 0;
				//				if(g_test_video == 0)
				//					handle->last_video_rtp_time +=  0xffffffff/90 -30*60*1000;

				if(handle->last_video_rtp_time > 0xffffffff / 90) {
					PRINTF("###############warnning ,video2 the begin time > ntimetick,%x,%x\n", handle->begin_video_time, temp->head.nTimeTick);
					handle->begin_video_time =  temp->head.nTimeTick;
					handle->last_video_rtp_time = 0;
					//handle->seq = 0;
					//					g_test_video = 1;
				}

				if(temp->head.dwFlags == AVIIF_KEYFRAME) {
					idr_flag = 1;
				}

				//if(handle->last_video_rtp_time < (0xffffffff/90))
				RtspVideoPack(temp->wlen, (temp->data), 1, handle->last_video_rtp_time, (int *) & (handle->seq), id, idr_flag);
			} else {
				handle->begin_video_time =  temp->head.nTimeTick;
				handle->last_video_current_time = 0;
			}

			//handle->width =temp->head.nWidth;
			//handle->height = temp->head.nHight;
			handle->last_video_current_time = mid_plat_get_runtime();
			//handle->last_video_rtp_time  = temp->head.nTimeTick;
			//printf("send video %u\n",temp->wlen);
#endif
		}
		rtsp_sream_reset_av_frame(temp);
	}

	return 0;
}




/*主要通过IDR帧来设置sdp info 和开关client*/
int rtsp_stream_parse_video_iframe(unsigned char *buff, unsigned int len , int id)
{
	int ret = 0;
	FRAMEHEAD *head = (FRAMEHEAD *)buff;
	ONE_FRAME_S *temp = NULL;
	int idr_flag = 0;
	stream_video_handle handle = g_stream_video_handle[id];

	//	if(app_center_get_stream_flag() == APP_CENTER_STOP)
	//	{
	//		return -1;
	//	}

	if(id < 0 || id > rtsp_stream_get_room_num() - 1 || g_stream_video_handle[id] == NULL) {
		ERR_PRN("video rebuild,roomid =%d,the g_stream_num =%d\n", id, rtsp_stream_get_room_num());
		return -1;
	}

	if(len < sizeof(FRAMEHEAD)) {
		PRINTF("video rebuild,ERROR.the frame len is too small.\n");
		return -1;
	}

	temp = &(handle->video_frame);

	if(head->nFrameLength > MAX_VIDEO_LEN) {
		PRINTF("video rebuild,LEN is to big,the len = %u\n", head->nFrameLength);
		return -1;
	}

	if(head->dwSegment == 3 || head->dwSegment == 2) {
		if(temp->wlen != 0 || temp->status != FRAME_NULL) {
			PRINTF("ERROR! video rebuild,the priv frame is not complete");
			PRINTF("ERROR!,video rebuild,the need len = %u,the w_len= %u\n", temp->head.nFrameLength, temp->wlen);
			rtsp_sream_reset_av_frame(temp);
		}
	} else if((head->dwSegment == 0 || head->dwSegment == 1) && temp->status != FRAME_BUILDING) {
		PRINTF("ERROR!video rebuild,the frame is not complete.\n");
		rtsp_sream_reset_av_frame(temp);
		return -1;
	}

	if(head->dwSegment == 2 || head->dwSegment == 3) {
		memcpy(&(temp->head) , head, sizeof(FRAMEHEAD));

		memcpy(temp->data + temp->wlen, buff + sizeof(FRAMEHEAD), len - sizeof(FRAMEHEAD));
		temp->wlen += len - sizeof(FRAMEHEAD);
		temp->status = FRAME_BUILDING;
	} else if(head->dwSegment == 0 || head->dwSegment == 1) {
		memcpy(temp->data + temp->wlen, buff + sizeof(FRAMEHEAD), len - sizeof(FRAMEHEAD));
		temp->wlen += len - sizeof(FRAMEHEAD);
	}

	if(temp->wlen == temp->head.nFrameLength && temp->wlen != 0) {
		temp->status = FRAME_COMPLETE;
	}

	if(temp->status == FRAME_COMPLETE) {
		int sleep_time = 0;
		{
#if 1
			sleep_time = 0;

			if(handle->last_video_current_time != 0) {
				sleep_time = (temp->head.nTimeTick - handle->last_video_rtp_time) - (mid_plat_get_runtime() - handle->last_video_current_time);
				//if(sleep_time > 15)
				//	PRINTF("need video sleep time = %d,%u\n",sleep_time,temp->head.nTimeTick);
				//	if(sleep_time > 0)
				//		;//usleep(sleep_time *1000);
			}

			if(temp->head.dwFlags == AVIIF_KEYFRAME)
				;//	PRINTF("video rebuild .temp->head.dwFlags = %x,,,%d-==%d\n",temp->head.dwFlags,temp->head.nWidth,handle->width);

			if((temp->head.dwFlags == AVIIF_KEYFRAME)
			   && ((handle->sdp_flag == 0) || (temp->head.nWidth != handle->width || temp->head.nHight != handle->height))) {
				PRINTF("video rebuilded,i will parse idr header \n");
				//rtsp_change_sdp_info(id);
				ret = rtsp_stream_video_filter_sdp_info(temp->data, temp->wlen, (char *)handle->sps_info, (int *) & (handle->sps_info_len),
				                                        handle->pps_info, (int *) & (handle->pps_info_len));

				if(ret < 0) {
					ERR_PRN("Error! video rebuild,parse sdp info error\n");
				} else {
					PRINTF("video rebuild,success,parse sdp info ok.\n");
					handle->sdp_flag = 1;
					handle->width = temp->head.nWidth;
					handle->height = temp->head.nHight;
				}
			}

			//	if(handle->last_video_rtp_time > temp->head.nTimeTick)
			//	{
			///		printf("ERROR,video!!!!!!!!!!!1!!!!!!!!!!!!\n");
			//		printf("ERROR!!!!!!!!!!!2!!!!!!!!last head =%x!!!!\n",handle->last_video_rtp_time);
			//		printf("ERROR!!!!!!!!!!!3!!!!!!!!!head=%x!!!\n",temp->head.nTimeTick);
			//	}

			//处理video buff
			//add by zhangmin
#ifdef GDM_RTSP_SERVER

			if((app_center_get_stream_flag() != APP_CENTER_STOP) && RtspClientIsNull() != 0)
#else
			if(RtspClientIsNull() != 0)
#endif
			{
				//考虑切分辨率的效果问题
				//quicttime client must close
				if(handle->width != temp->head.nWidth  || handle->height != temp->head.nHight) {
					rtsp_change_sdp_info(id);
					PRINTF("video rebuild,the width changed,so this buff do not seed to rtsp client\n");
				}

				//vlc client is OK
				if(handle->last_video_current_time  == 0 || handle->begin_video_time >  temp->head.nTimeTick)
					//	|| ((temp->head.nTimeTick-handle->begin_video_time) > 3*60*1000 )) //4
				{
					PRINTF("##################warnning ,video the begin time > ntimetick,%x,%x\n", handle->begin_video_time, temp->head.nTimeTick);
					handle->begin_video_time =  temp->head.nTimeTick;
					//handle->seq = 0;
				}

				handle->last_video_rtp_time  = temp->head.nTimeTick - handle->begin_video_time;

				//test
				//handle->last_video_rtp_time = 0;
				//				static int g_test_video= 0;
				//				if(g_test_video == 0)
				//					handle->last_video_rtp_time +=  0xffffffff/90 -30*60*1000;

				if(handle->last_video_rtp_time > 0xffffffff / 90) {
					PRINTF("###############warnning ,video2 the begin time > ntimetick,%x,%x\n", handle->begin_video_time, temp->head.nTimeTick);
					handle->begin_video_time =  temp->head.nTimeTick;
					handle->last_video_rtp_time = 0;
					//handle->seq = 0;
					//					g_test_video = 1;
				}

				if(temp->head.dwFlags == AVIIF_KEYFRAME) {
					idr_flag = 1;
				}

				//if(handle->last_video_rtp_time < (0xffffffff/90))
				RtspVideoPack(temp->wlen, (temp->data), 1, handle->last_video_rtp_time, (int *) & (handle->seq), id, idr_flag);
			} else {
				handle->begin_video_time =  temp->head.nTimeTick;
				handle->last_video_current_time = 0;
			}

			//handle->width =temp->head.nWidth;
			//handle->height = temp->head.nHight;
			handle->last_video_current_time = mid_plat_get_runtime();
			//handle->last_video_rtp_time  = temp->head.nTimeTick;
			//printf("send video %u\n",temp->wlen);
#endif
		}
		rtsp_sream_reset_av_frame(temp);
	}

	return 0;
}




static unsigned long g_110e_video_frame_time = 0;
static unsigned long g_110e_audio_frame_time = 0;



int rtsp_stream_rebuild_audio_frame_110e(unsigned char *buff, unsigned int len , int id, DataHeader_110E *param)
{
	DataHeader_110E *temphead = (DataHeader_110E *)param;
	FRAMEHEAD framehead  ;
	FRAMEHEAD *head = &framehead;
	ONE_FRAME_S *temp = NULL;
	stream_audio_handle handle = NULL;
	int config = 0;
	//	if(app_center_get_stream_flag() == APP_CENTER_STOP)
	//	{
	//		return -1;
	//	}

	if(g_stream_audio_handle == NULL) {
		ERR_PRN("ERROR\n");
		return -1;
	}

	//if stare frame ,i will get the head
	//	if(temphead->dwSegment == 2 || temphead->dwSegment == 3)
	{
		unsigned int timeTick = 0;

		if(g_110e_audio_frame_time == 0) {
			timeTick = 0 ;
			g_110e_audio_frame_time = mid_plat_get_runtime();
		} else {
			timeTick = mid_plat_get_runtime() - g_110e_audio_frame_time;
		}

		memset(&framehead, 0, sizeof(framehead));
		framehead.ID = 0x56534434;

		if(temphead->dwSegment == 2 || temphead->dwSegment == 3) {
			framehead.nTimeTick = timeTick;
		}

		framehead.nFrameLength = temphead->dwCompressedDataLength;
		framehead.nDataCodec = temphead->fccHandler;
		framehead.nFrameRate = 30;
		//	framehead.nWidth = temphead->biWidth;
		//110e BIWIDTH = SAMPLERATE
		framehead.nHight = temphead->biWidth;
		framehead.nColors = temphead->biBitCount;
		framehead.dwSegment =  temphead->dwSegment;
		framehead.dwFlags = temphead->dwFlags;
		framehead.dwPacketNumber = temphead->dwPacketNumber;
		framehead.nOthers = 0;
	}

	if(len < sizeof(DataHeader_110E)) {
		PRINTF("ERROR.\n");
		return -1;
	}

	handle = g_stream_audio_handle;
	temp = &(handle->audio_frame);

	if(head->nFrameLength > MAX_AUDIO_LEN) {
		PRINTF("audio LEN is to big,the len = %u\n", head->nFrameLength);
		return -1;
	}

	if(head->dwSegment == 3 || head->dwSegment == 2) {
		if(temp->wlen != 0 || temp->status != FRAME_NULL) {
			PRINTF("ERROR! the aduio  priv frame is not complete");
			PRINTF("ERROR!,the aduio  need len = %u,the w_len= %u\n", temp->head.nFrameLength, temp->wlen);
			rtsp_sream_reset_av_frame(temp);
		}
	} else if((head->dwSegment == 0 || head->dwSegment == 1) && temp->status != FRAME_BUILDING) {
		PRINTF("ERROR!,the audio frame is not complete\n");
		rtsp_sream_reset_av_frame(temp);
		return -1;
	}

	if(head->dwSegment == 2 || head->dwSegment == 3) {
		memcpy(&(temp->head) , head, sizeof(FRAMEHEAD));

		memcpy(temp->data + temp->wlen, buff + sizeof(DataHeader_110E), len - sizeof(DataHeader_110E));
		temp->wlen += len - sizeof(DataHeader_110E);
		temp->status = FRAME_BUILDING;
	} else if(head->dwSegment == 0 || head->dwSegment == 1) {
		memcpy(temp->data + temp->wlen, buff + sizeof(DataHeader_110E), len - sizeof(DataHeader_110E));
		temp->wlen += len - sizeof(DataHeader_110E);
	}

	if(temp->wlen == temp->head.nFrameLength && temp->wlen != 0) {
		temp->status = FRAME_COMPLETE;
	}

	//have a complete frame
	if(temp->status == FRAME_COMPLETE) {
		int sleep_time = 0;
		{
#if 1
			sleep_time = 0;

			if(handle->last_audio_current_time != 0) {
				sleep_time = (temp->head.nTimeTick - handle->last_audio_rtp_time) - (mid_plat_get_runtime() - handle->last_audio_current_time);
				//if(sleep_time > 3)
				//	PRINTF("need audio sleep time = %d,%u\n",sleep_time,temp->head.nTimeTick);
				//	if(sleep_time > 0)
				//		;//usleep(sleep_time *1000);
			}

#endif

			//加入逻辑，用于分析ADTS头
			// resver for future
			if(handle->audio_config <= 0 || handle->samplerate != temp->head.nHight) {
				config =  rtsp_stream_audio_filter_sdp_info(temp->data);

				if(config == -1) {
					PRINTF("ERROR,i can't fiter the audio sdp\n");
				} else {
					PRINTF("I find the config,the samplerate =%u\n", handle->samplerate);
					handle->audio_config = config;
					handle->samplerate  = temp->head.nHight;
				}
			}

			//处理video buff
			//printf("RtspGetClientNum() = %d\n",0);
#if 0

			if(handle->last_audio_rtp_time == 0) {
				temp->head.nTimeTick = 2048 / 2;
			} else {
				temp->head.nTimeTick = handle->last_audio_rtp_time + 2048 / 2;
			}

#endif
			//	if(handle->last_audio_rtp_time > temp->head.nTimeTick)
			//	{
			//		printf("ERROR,audio!!!!!!!!!!!1!!!!!!!!!!!!\n");
			//		printf("ERROR!!!!!!!!!!!2!!!!!last_audio_rtp_time=%x!!!!!!!\n",handle->last_audio_rtp_time);
			//		printf("ERROR!!!!!!!!!!!3!!!!!!!head=%x!!!!!\n",temp->head.nTimeTick);
			//	}
			//add by zhangmin
#ifdef GDM_RTSP_SERVER

			if((app_center_get_stream_flag() != APP_CENTER_STOP) && (handle->audio_config > 0) && (RtspClientIsNull() != 0))
#else
			if((handle->audio_config > 0) && (RtspClientIsNull() != 0))
#endif
			{
				if(handle->last_audio_current_time == 0 || handle->begin_audio_time >  temp->head.nTimeTick)
					//	|| ((temp->head.nTimeTick-handle->begin_audio_time) > 0xffffffff/90 ))  //4//4 housrs
				{
					PRINTF("###############warnning ,audio the begin time > ntimetick,%x,%x\n", handle->begin_audio_time, temp->head.nTimeTick);
					handle->begin_audio_time =  temp->head.nTimeTick;
					//handle->seq = 0;
				}

				handle->last_audio_rtp_time  = temp->head.nTimeTick - handle->begin_audio_time;

				//test
				//	handle->last_audio_rtp_time = 0;
				//				static int g_test_audio= 0;
				//				if(g_test_audio == 0)
				//					handle->last_audio_rtp_time += 0xffffffff/90 -30*60*1000;
				if(handle->last_audio_rtp_time > 0xffffffff / 48) {
					PRINTF("################warnning ,audio2 the begin time > ntimetick,%x,%x\n", handle->begin_audio_time, temp->head.nTimeTick);
					handle->begin_audio_time =  temp->head.nTimeTick;
					handle->last_audio_rtp_time = 0;
					//RtspAdudioSeqReset();
					//					g_test_audio = 1;
				}

				//	if(handle->last_audio_rtp_time  < 0xffffffff/48)
				RtspAudioPack(temp->wlen, (temp->data), handle->last_audio_rtp_time, 0, handle->samplerate);
			} else {
				handle->begin_audio_time = temp->head.nTimeTick;
				handle->last_audio_current_time = 0;
			}

			handle->channel = temp->head.nWidth; //复用为audio sample rate ,
			//handle->samplerate = temp->head.nHight;
			handle->last_audio_current_time = mid_plat_get_runtime();
			//handle->last_audio_rtp_time  = temp->head.nTimeTick;

		}
		rtsp_sream_reset_av_frame(temp);
	}


	return 0;

}


//rebuild video frame
int rtsp_stream_rebuild_video_frame_110e(unsigned char *buff, unsigned int len , int id, DataHeader_110E *param)
{
	int ret = 0;
	DataHeader_110E *temphead = (DataHeader_110E *)param;
	FRAMEHEAD framehead  ;
	FRAMEHEAD *head = &framehead;
	ONE_FRAME_S *temp = NULL;
	int idr_flag = 0;
	stream_video_handle handle = g_stream_video_handle[id];
	//printf("11\n");
	//if stare frame ,i will get the head
	//	if(temphead->dwSegment == 2 || temphead->dwSegment == 3)
	{
		unsigned int timeTick = 0;

		if(g_110e_video_frame_time == 0) {
			timeTick = 0 ;
			g_110e_video_frame_time = mid_plat_get_runtime();
		} else {
			timeTick = mid_plat_get_runtime() - g_110e_video_frame_time;
		}

		memset(&framehead, 0, sizeof(framehead));
		framehead.ID = 0x56534434;

		if(temphead->dwSegment == 2 || temphead->dwSegment == 3) {
			framehead.nTimeTick = timeTick;
		}

		framehead.nFrameLength = temphead->dwCompressedDataLength;
		framehead.nDataCodec = temphead->fccHandler;
		framehead.nFrameRate = 30;
		framehead.nWidth = temphead->biWidth;
		framehead.nHight = temphead->biHeight;
		framehead.nColors = temphead->biBitCount;
		framehead.dwSegment =  temphead->dwSegment;
		framehead.dwFlags = temphead->dwFlags;
		framehead.dwPacketNumber = temphead->dwPacketNumber;
		framehead.nOthers = 0;
	}

	//PRINTF("\n");
	//	if(app_center_get_stream_flag() == APP_CENTER_STOP)
	//	{
	//		return -1;
	//	}

	if(id < 0 || id > rtsp_stream_get_room_num() - 1 || g_stream_video_handle[id] == NULL) {
		ERR_PRN("video rebuild,roomid =%d,the g_stream_num =%d\n", id, rtsp_stream_get_room_num());
		return -1;
	}

	if(len < sizeof(DataHeader_110E)) {
		PRINTF("video rebuild,ERROR.the frame len is too small.\n");
		return -1;
	}

	temp = &(handle->video_frame);

	if(head->nFrameLength > MAX_VIDEO_LEN) {
		PRINTF("video rebuild,LEN is to big,the len = %u\n", head->nFrameLength);
		return -1;
	}

	if(head->dwSegment == 3 || head->dwSegment == 2) {
		if(temp->wlen != 0 || temp->status != FRAME_NULL) {
			PRINTF("ERROR! video rebuild,the priv frame is not complete");
			PRINTF("ERROR!,video rebuild,the need len = %u,the w_len= %u\n", temp->head.nFrameLength, temp->wlen);
			rtsp_sream_reset_av_frame(temp);
		}
	} else if((head->dwSegment == 0 || head->dwSegment == 1) && temp->status != FRAME_BUILDING) {
		PRINTF("ERROR!video rebuild,the frame is not complete.\n");
		rtsp_sream_reset_av_frame(temp);
		return -1;
	}

	if(head->dwSegment == 2 || head->dwSegment == 3) {


		memcpy(&(temp->head) , head, sizeof(FRAMEHEAD));

		memcpy(temp->data + temp->wlen, buff + sizeof(DataHeader_110E), len - sizeof(DataHeader_110E));
		temp->wlen += len - sizeof(DataHeader_110E);
		temp->status = FRAME_BUILDING;
	} else if(head->dwSegment == 0 || head->dwSegment == 1) {
		memcpy(temp->data + temp->wlen, buff + sizeof(DataHeader_110E), len - sizeof(DataHeader_110E));
		temp->wlen += len - sizeof(DataHeader_110E);
	}

	if(temp->wlen == temp->head.nFrameLength && temp->wlen != 0) {
		temp->status = FRAME_COMPLETE;
	}

	if(temp->status == FRAME_COMPLETE) {
		int sleep_time = 0;
		{
#if 1
			sleep_time = 0;

			if(handle->last_video_current_time != 0) {
				sleep_time = (temp->head.nTimeTick - handle->last_video_rtp_time) - (mid_plat_get_runtime() - handle->last_video_current_time);
				//if(sleep_time > 15)
				//	PRINTF("need video sleep time = %d,%u\n",sleep_time,temp->head.nTimeTick);
				//	if(sleep_time > 0)
				//		;//usleep(sleep_time *1000);
			}

			if(temp->head.dwFlags == AVIIF_KEYFRAME)
				;//	PRINTF("video rebuild .temp->head.dwFlags = %x,,,%d-==%d\n",temp->head.dwFlags,temp->head.nWidth,handle->width);

			if((temp->head.dwFlags == AVIIF_KEYFRAME)
			   && ((handle->sdp_flag == 0) || (temp->head.nWidth != handle->width || temp->head.nHight != handle->height))) {
				PRINTF("video rebuilded,i will parse idr header \n");
				//rtsp_change_sdp_info(id);
				ret = rtsp_stream_video_filter_sdp_info(temp->data, temp->wlen, (char *)handle->sps_info, (int *) & (handle->sps_info_len),
				                                        handle->pps_info, (int *) & (handle->pps_info_len));

				if(ret < 0) {
					ERR_PRN("Error! video rebuild,parse sdp info error\n");
				} else {
					PRINTF("video rebuild,success,parse sdp info ok.\n");
					handle->sdp_flag = 1;
					handle->width = temp->head.nWidth;
					handle->height = temp->head.nHight;
				}
			}

			//	if(handle->last_video_rtp_time > temp->head.nTimeTick)
			//	{
			///		printf("ERROR,video!!!!!!!!!!!1!!!!!!!!!!!!\n");
			//		printf("ERROR!!!!!!!!!!!2!!!!!!!!last head =%x!!!!\n",handle->last_video_rtp_time);
			//		printf("ERROR!!!!!!!!!!!3!!!!!!!!!head=%x!!!\n",temp->head.nTimeTick);
			//	}

			//处理video buff
			//add by zhangmin
#ifdef GDM_RTSP_SERVER

			if((app_center_get_stream_flag() != APP_CENTER_STOP) && RtspClientIsNull() != 0)
#else
			if(RtspClientIsNull() != 0)
#endif
			{
				//考虑切分辨率的效果问题
				//quicttime client must close
				if(handle->width != temp->head.nWidth  || handle->height != temp->head.nHight) {
					rtsp_change_sdp_info(id);
					PRINTF("video rebuild,the width changed,so this buff do not seed to rtsp client\n");
				}

				//vlc client is OK
				if(handle->last_video_current_time  == 0 || handle->begin_video_time >  temp->head.nTimeTick)
					//	|| ((temp->head.nTimeTick-handle->begin_video_time) > 3*60*1000 )) //4
				{
					PRINTF("##################warnning ,video the begin time > ntimetick,%x,%x\n", handle->begin_video_time, temp->head.nTimeTick);
					handle->begin_video_time =  temp->head.nTimeTick;
					//handle->seq = 0;
				}

				handle->last_video_rtp_time  = temp->head.nTimeTick - handle->begin_video_time;

				//test
				//handle->last_video_rtp_time = 0;
				//				static int g_test_video= 0;
				//				if(g_test_video == 0)
				//					handle->last_video_rtp_time +=  0xffffffff/90 -30*60*1000;

				if(handle->last_video_rtp_time > 0xffffffff / 90) {
					PRINTF("###############warnning ,video2 the begin time > ntimetick,%x,%x\n", handle->begin_video_time, temp->head.nTimeTick);
					handle->begin_video_time =  temp->head.nTimeTick;
					handle->last_video_rtp_time = 0;
					//handle->seq = 0;
					//					g_test_video = 1;
				}

				if(temp->head.dwFlags == AVIIF_KEYFRAME) {
					idr_flag = 1;
				}

				//if(handle->last_video_rtp_time < (0xffffffff/90))
				RtspVideoPack(temp->wlen, (temp->data), 1, handle->last_video_rtp_time, (int *) & (handle->seq), id, idr_flag);
			} else {
				handle->begin_video_time =  temp->head.nTimeTick;
				handle->last_video_current_time = 0;
			}

			//handle->width =temp->head.nWidth;
			//handle->height = temp->head.nHight;
			handle->last_video_current_time = mid_plat_get_runtime();
			//handle->last_video_rtp_time  = temp->head.nTimeTick;
			//printf("send video %u\n",temp->wlen);
#endif
		}
		rtsp_sream_reset_av_frame(temp);
	}

	return 0;
}


static stream_audio_handle rtsp_stream_new_audio_handle()
{
	stream_audio_handle handle = NULL;
	unsigned	char *atemp = NULL;
	handle = (stream_audio_handle)malloc(sizeof(stream_audio_handle_t));

	if(handle == NULL) {
		ERR_PRN("Error,malloc the audio handle is failed\n");
		return NULL;
	}

	memset(handle, 0, sizeof(stream_audio_handle_t));
	handle->size = sizeof(stream_audio_handle_t);
	//    handle->mutex = mid_mutex_create();
	atemp = (unsigned char *)malloc(sizeof(char) * MAX_AUDIO_LEN);

	if(atemp == NULL) {
		ERR_PRN("Error,malloc the audio buff is Error\n");

		if(handle) {
			free(handle);
		}

		if(atemp) {
			free(atemp);
		}

		return NULL;
	}

	handle->audio_frame.data = atemp;

	return handle ;
}

static stream_video_handle rtsp_stream_new_video_handle()
{
	stream_video_handle handle = NULL;
	unsigned	char *temp = NULL;
	handle = (stream_video_handle)malloc(sizeof(stream_video_handle_t));

	if(handle == NULL) {
		ERR_PRN("Error,malloc the handle is failed\n");
		return NULL;
	}

	memset(handle, 0, sizeof(stream_video_handle_t));
	handle->size = sizeof(stream_video_handle_t);
	//    handle->mutex = mid_mutex_create();
	temp = (unsigned char *)malloc(sizeof(char) * MAX_VIDEO_LEN);

	if(temp == NULL) {
		ERR_PRN("Error,malloc the video buff is Error\n");

		if(handle) {
			free(handle);
		}

		if(temp) {
			free(temp);
		}

		return NULL;
	}

	handle->video_frame.data = temp;


	return handle ;
}


static int rtsp_video_stream_init(int num)
{
	int i = 0;

	if(num > MAX_ENCODE_NUM) {
		num = MAX_ENCODE_NUM;
	}


	for(i = 0; i < num ; i++) {
		g_stream_video_handle[i] = rtsp_stream_new_video_handle();

		if(g_stream_video_handle[i] == NULL) {
			ERR_PRN("Error!rtsp_video_stream_init.\n");
			return -1;
		}

		g_stream_video_handle[i]->roomid = i;
	}

	rtsp_stream_set_room_num(num);
	return 0;
}

static int rtsp_audio_stream_init(int id)
{
	g_stream_audio_handle = rtsp_stream_new_audio_handle();

	if(g_stream_audio_handle == NULL) {
		ERR_PRN("Error!rtsp_audio_stream_init.\n");
		return -1;
	}

	g_stream_audio_handle->roomid = id;
	return 0;
}



//stream init
int rtsp_stream_init(int video_num, int audio_id)
{
	if(g_stream_init != 0) {
		return 0;
	}

	rtsp_video_stream_init(video_num);
	rtsp_audio_stream_init(audio_id);
	g_stream_init = 1;
	return 0;
}



/*Find H264 start NAL*/
static int FindH264StartNAL(unsigned char *pp)
{
	/*is for 00 00 00 01 Nal header*/
	if(pp[0] != 0 || pp[1] != 0 || pp[2] != 0 || pp[3] != 1) {
		return 0;
	} else {
		return 1;
	}
}
#define H264_HEADER_LEN 		0x40

/*parse IDR header length*/
static int rtsp_stream_video_filter_sdp_info(unsigned char *pdata, int plen, char *sps_info, int *sps_info_len,
        unsigned 	char *pps_info, int *pps_info_len)
{
	int len = 0x17;
	int ret = 0, I_frame_header_length = 0;
	unsigned	char *find = pdata;
	int temp_len = 0;
	unsigned char *nalu_ptr = NULL;
	int nalu_len = 0;
	char nal_unit_type = 0;

	while(plen) {
		ret = FindH264StartNAL((unsigned char *)find);

		if(ret) {
			I_frame_header_length++ ;

			if(I_frame_header_length == 2) {
				temp_len = (int)(find - pdata);
			}

			if(I_frame_header_length >= 3) {
				break;
			} else {
				find += 3; //find next NAL header
				plen = plen - 3;
			}
		}

		//		PRINTF("ERROR,the data = 0x%02x,%02x,%02x,%02x\n",pdata[0],pdata[1],pdata[2],pdata[3]);
		//		return -1;
		find++;
		plen --;
	}

	len = (int)(find - pdata);

	//	PRINTF("length = %d\n",len);
	if(len > H264_HEADER_LEN) {
		len = 0x18;
	}

	PRINTF("I_frame_header_length = %d\n", I_frame_header_length);

	if(I_frame_header_length < 3) {
		return -1;
	}

	nalu_ptr = pdata;
	nalu_len = temp_len;
	nal_unit_type = rtsp_stream_get_nal_unit_type(nalu_ptr[4]);
	//	PRINTF("len = %d,temp_len = %d,nalu_ptr[4] =0x%02x,nal_unit_type = %d\n",len,temp_len,nalu_ptr[4],nal_unit_type);

	if(nal_unit_type == 7) {
		memcpy(sps_info, &(nalu_ptr[4]), nalu_len - 4);
		*sps_info_len = nalu_len - 4;
	} else	if(nal_unit_type == 8) {
		memcpy(pps_info, &(nalu_ptr[4]), nalu_len - 4);
		*pps_info_len = nalu_len - 4 ;
	}


	nalu_ptr = pdata + temp_len ;
	nalu_len = len - temp_len;
	nal_unit_type = rtsp_porting_get_nal_unit_type(nalu_ptr[4]);
	//	PRINTF("len = %d,temp_len = %d,nalu_ptr[4] =0x%02x,nal_unit_type = %d\n",len,temp_len,nalu_ptr[4],nal_unit_type);

	if(nal_unit_type == 7) {
		memcpy(sps_info, &(nalu_ptr[4]), nalu_len - 4);
		*sps_info_len = nalu_len - 4;
	} else	if(nal_unit_type == 8) {
		memcpy(pps_info, &(nalu_ptr[4]), nalu_len - 4);
		*pps_info_len = nalu_len - 4;
	}

	//mid_mutex_unlock(g_rtsp_video_sdp_mutex);
	return 0;
}


static int rtsp_stream_get_nal_unit_type(unsigned char nal_code)
{
	unsigned char nal_unit_type = 0;
	unsigned char nal_ref_idc = 0;
	nal_unit_type = nal_code & 0x1f;
	nal_ref_idc = nal_code & 0x60;

	if(nal_unit_type == 7 || nal_unit_type == 8) {
		if(nal_ref_idc == 0) {
			return -1;
		}

		return nal_unit_type;
	}

	return nal_unit_type;

}


/*
*	获取系统的SDP信息，主要在IDR包头里面查找SPS,PPS
*
*/
int rtsp_stream_get_video_sdp_info(char *buff, int id)
{
	int inlen = 0;
	char in[128] = {0};
	int outlen = 256;
	char out[256] = {0};
	int b64_len = 0;
	int b64_len1 = 0;
	stream_video_handle handle = NULL;

	//int ret = 0;
	if(id < 0 || id > rtsp_stream_get_room_num() - 1 || g_stream_video_handle[id] == NULL) {
		ERR_PRN("ERROR,rtsp get video sdp error,roomid =%d,the g_stream_num =%d\n", id, rtsp_stream_get_room_num());
		return -1;
	}


	handle = g_stream_video_handle[id];

	if(handle->sdp_flag == 0 || handle->sps_info_len == 0 || handle->pps_info_len == 0) {
		//		PRINTF("ERROR!the id have no sdp info.\n");
		return -1;
	}

	memcpy(in, handle->sps_info, handle->sps_info_len);
	inlen = handle->sps_info_len;

	if(inlen == 0) {
		return -1;
	}

	//	PRINTF("sps info ,inlen = %d\n",inlen);

	b64_len = app_b64_encode(in, inlen, out, outlen);

	if(b64_len < inlen) {
		return -1;
	}

	memcpy(buff, out, b64_len);

	sprintf(buff + b64_len, ",");

	memset(in, 0, sizeof(in));
	memset(out, 0, sizeof(out));

	memcpy(in, handle->pps_info, handle->pps_info_len);
	inlen = handle->pps_info_len;

	if(inlen == 0) {
		return -1;
	}

	//	PRINTF("sps info ,inlen = %d\n",inlen);

	b64_len1 = app_b64_encode(in, inlen, out, outlen);

	if(b64_len1 < inlen) {
		return -1;
	}

	memcpy(buff + b64_len + 1, out, b64_len1);

	PRINTF("BUFF = %s\n", buff);

	return 0;
}


typedef struct audio_aac_adts_fixed_header_t {
	int syncword;
	unsigned char id;
	unsigned char layer;
	unsigned char profile;
	unsigned char sampling_frequency_index;
	unsigned char channel_configuration;
} audio_aac_adts_fixed_header_s;



/*parse audio aac adts header length*/
static unsigned int rtsp_stream_audio_filter_sdp_info(unsigned char *pdata)
{
	if(pdata == NULL) {
		return -1;
	}

	PRINTF("the audio adts header [0]=0x%x,[1]=0x%x,[2]=0x%x,[3]=0x%x\n", pdata[0], pdata[1], pdata[2], pdata[3]);
	int syncword = 0;
	unsigned char id;
	unsigned char layer;
	unsigned char profile;
	unsigned char sampling_frequency_index;
	unsigned char channel_configuration;
	audio_aac_adts_fixed_header_s audio_adts_header;
	unsigned int config = 0;

	syncword = (pdata[0] << 4) | ((pdata[1] & 0xf0) >> 4) ;

	if(syncword != 0x0fff) {
		PRINTF("the syncword =0x%x\n", syncword);
		return -1;
	}

	id = (pdata[1] & 0x08) >> 3;
	layer = (pdata[1] & 0x06) >> 1;
	profile = (pdata[2] & 0xc0) >> 6;
	sampling_frequency_index = (pdata[2] & 0x3c) >> 2;
	channel_configuration = ((pdata[2] & 0x01) >> 2) | ((pdata[3] & 0xc0) >> 6);
	PRINTF("layer=%d,the profile=%d,the sampling=0x%x,channel_configura=%d\n",
	       layer, profile, sampling_frequency_index, channel_configuration);

	memset(&audio_adts_header, 0, sizeof(audio_adts_header));
	audio_adts_header.id = id;
	audio_adts_header.layer = layer;
	audio_adts_header.profile = profile;
	audio_adts_header.sampling_frequency_index = sampling_frequency_index;
	audio_adts_header.channel_configuration = channel_configuration;

	config = 0x1000 | (((audio_adts_header.sampling_frequency_index & 0x0f) >> 1) << 8)
	         | ((audio_adts_header.sampling_frequency_index & 0x01) << 7)
	         | ((audio_adts_header.channel_configuration) << 3) ;
	PRINTF("g_audio_adts_header.sampling_frequency_index =%d,channel=%d,config=%04x\n", audio_adts_header.sampling_frequency_index
	       , audio_adts_header.channel_configuration, config);


	return config;
}




//default for 48000
int rtsp_stream_get_audio_sdp_info(char *buff)
{
	stream_audio_handle handle = NULL;
	handle = g_stream_audio_handle;

	if(handle->audio_config <= 0) {
		PRINTF("ERROR,Now i can't find audio sdp info\n");
		return -1;
	}

	sprintf(buff, "%04x", handle->audio_config);
	PRINTF("the audio config = %s,\n", buff);
	return 0;
}

unsigned int rtsp_stream_get_audio_samplerate(void)
{
	unsigned int samplerate = g_stream_audio_handle->samplerate;

	if(samplerate != 44100 && samplerate != 48000) {
		samplerate = 48000;
	}

	return samplerate;
}


//add by zhangmin
#ifdef GDM_RTSP_SERVER
CRoom *g_rtsp_theroom = NULL;
void rtsp_stream_set_handle(void *theroom)
{
	g_rtsp_theroom = (CRoom *)theroom;

}

//force ifream
//注意，需要调整为roomid 单独设置i帧
int rtsp_stream_force_Iframe(int roomid)
{
	g_rtsp_theroom->ForceKeyFrame();
	return 0;
}

#else
//force ifream
//注意，需要调整为roomid 单独设置i帧

extern void rtsp_porting_force_Iframe(void);
int rtsp_stream_force_Iframe(int roomid)
{
	rtsp_porting_force_Iframe();
	return 0;
}
#endif


#endif