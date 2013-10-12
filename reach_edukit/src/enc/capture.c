#include <stdio.h>

#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>
#include <mcfw/src_linux/devices/inc/device_videoDecoder.h>
#include <mcfw/src_linux/devices/inc/device.h>
#include <mcfw/src_linux/devices/inc/device_videoDecoder.h>
#include <mcfw/src_linux/devices/adv7441/inc/adv7441.h>
#include <mcfw/src_linux/devices/adv7441/src/adv7441_priv.h>
#include <mcfw/src_linux/devices/adv7442/inc/adv7442.h>
#include <mcfw/src_linux/devices/adv7442/src/adv7442_priv.h>
#include "reach.h"
#include "capture.h"
#include "common.h"
//#include "params.h"
#include "select.h"

#include "osd.h"
#include "sd_demo_osd.h"
#include "middle_control.h"
#include "app_signal.h"
#include "input_to_channel.h"
#include "capture.h"
#include "ppt_index.h"
#include "nslog.h"

extern EduKitLinkStruct_t *gEduKit;
extern int32_t audio_mute_flag;
//extern RoomMsgEnv *pRoomInfo;

//extern RoomMsgEnv *pRoomInfo;
extern int32_t upper_msg_enc_status_deal(int32_t enc_id, int32_t status, int32_t mute,
        int32_t sourcenum, int32_t layoutid);
extern int32_t GeEncSrcStatue(int32_t EncId);

extern Int32 Ioctl_Led(int gpio, Int32 led, Int32 state);


//1--有源，0--无源
int g_SignalSate[SIGNAL_INPUT_MAX] = {1, 1};


int MPmode = DEFAULT_ENCODE_MODE;

static void RtpParmLock(int chane)
{
	pthread_mutex_lock(&gEduKit->rtvideoparm[chane].rtlock);
}

static void RtpParmunLock(int chane)
{
	pthread_mutex_unlock(&gEduKit->rtvideoparm[chane].rtlock);
}

int32_t set_synt_layout(int8_t *layout)
{
	int32_t layoutid = 0;
	int32_t ret = 0;

	if(layout == NULL) {
		fprintf(stderr, "set_synt_layout failed, layout is NULL!\n");
		return -1;
	}

	fprintf(stderr, "\n\n\n");
	fprintf(stderr, "set_synt_layout, layout = %s\n", layout);
	fprintf(stderr, "\n\n\n");

	layoutid = atoi((const char *)layout);

	if((gEduKit->layout.cur_singal_num == 2) && (layoutid > 20 && layoutid < 27)) {
		gEduKit->layout.def_layout_2 = layoutid;
		ret = 0;
	} else if((gEduKit->layout.cur_singal_num == 3) && (layoutid > 30 && layoutid < 33)) {
		gEduKit->layout.def_layout_3 = layoutid;
		ret = 0;
	} else {
		ret = -1;
	}

	return ret;
}


//设置采集的图像的大小
int capture_set_input_hw(Uint32 input, Uint32 width, Uint32 height, Uint32 type)
{
	if(input >= SIGNAL_INPUT_MAX) {
		fprintf(stderr, "set_video_hw error, channel = %d\n", input);
		return 0;
	}

	if(width > 1920 || height > 1200 || width == 0 || height == 0) {
		printf("width =%d,height=%d\n", width, height);
		return 0;
	}

	RtpParmLock(input);
	gEduKit->rtvideoparm[input].channel = input;
	gEduKit->rtvideoparm[input].width =  width;
	gEduKit->rtvideoparm[input].height = height;
	gEduKit->rtvideoparm[input].type =   type;
	RtpParmunLock(input);

	return 1;
}

int capture_get_input_type(int input)
{

	return gEduKit->rtvideoparm[input].type;
}


int capture_get_input_hw(Uint32 input, Uint32 *width, Uint32 *height)
{
	if(input >= SIGNAL_INPUT_MAX) {
		fprintf(stderr, "get_video_hw error, channel = %d\n", input);
		return 0;
	}

	if(width == NULL || height == NULL) {
		fprintf(stderr, "get_video_hw error, *width = %p, *height = %p\n",
		        width, height);
		return 0;
	}

	RtpParmLock(input);
	*width = 	gEduKit->rtvideoparm[input].width;
	*height = 	gEduKit->rtvideoparm[input].height;
	RtpParmunLock(input);
	fprintf(stderr, "id: %d, width = %d, height = %d\n", input, *width, *height);

	return 1;
}

Int32 GetVgaSourceState(Device_VideoDecoderExternInforms *signal_info)
{

	if(signal_info == NULL) {
		return -1;
	}

	Device_adv7442Control((Device_Adv7442Handle)(&gEduKit->capLink.adv7442_phandle), IOCTL_DEVICE_VIDEO_DECODER_EXTERN_INFORM, signal_info, NULL);

	fprintf(stderr, "%s,Mode=%d,frequency=%d,tmds=%d,hpv=%d\n", signal_info->DeviceName, signal_info->ModeID, signal_info->SignalFreq,
	        signal_info->SignalTmds, signal_info->SignalHpv);

	return signal_info->SignalTmds;

}

int capture_set_ColorTrans(int input, int ColorTrans)
{
	printf("\n\ninput = %d, ColorTrans = %d\n\n", input, ColorTrans);


	return 0;
}
/*目前不支持数字信号*/
int capture_set_vga_hv(int h, int v)
{
	short  h_pos = 0, v_pos = 0;
	HVTable hv_table;
	int pos = 0, new_pos = 0;

	fprintf(stderr, "move H=%d,V=%d \n", h, v);
	memset(&hv_table, 0, sizeof(HVTable));
	get_HV_table(&hv_table);

	cap_get_adv7442_HV(&gEduKit->capLink.adv7442_phandle, &pos);
	h_pos = (pos & 0xFFFF0000) >> 16;
	v_pos = pos & 0xFFFF;
	new_pos = ((h_pos + h) << 16) | (v_pos + v);

	fprintf(stderr, "h_pos=%d,v_pos=%d,pos=%d\n", h_pos, v_pos, pos);
	cap_set_adv7442_HV(&gEduKit->capLink.adv7442_phandle, new_pos);

	if(h == 1 || h == -1) {
		cap_invert_cbcr_adv7442_HV(&gEduKit->capLink.adv7442_phandle, 1);
	}

	Device_VideoDecoderExternInforms info = {{0}, 0};

	if(0 == GetVgaSourceState(&info)) {
		if(info.SignalTmds == 1) {
			hv_table.digital[info.ModeID].hporch = h + h_pos;
			hv_table.digital[info.ModeID].vporch = v + v_pos;
		} else if(info.SignalTmds == 0) { //analog
			hv_table.analog[info.ModeID].hporch = h + h_pos;
			hv_table.analog[info.ModeID].vporch = v + v_pos;
		}

		write_HV_table(&hv_table, info.SignalTmds);
	} else {
		fprintf(stderr, "get_signal_info failed!\n");
	}

	return 0;
}

Int32 vgaResolutionChangeHV(void)
{
	Int32 h = 0, v = 0;
	HVTable hv_table = {{{0}}};
	Int32 digital = 0;
	int pos = 0, new_pos = 0;
	Device_VideoDecoderExternInforms info = {{0}, 0};

	get_HV_table(&hv_table);

	if(0 != GetVgaSourceState(&info)) {
		return -1;
	}

	if(info.ModeID >= 0) {
		if(info.SignalTmds == 1) { //digital
			h = hv_table.digital[info.ModeID].hporch;
			v = hv_table.digital[info.ModeID].vporch;
		} else if(info.SignalTmds == 0) { //analog
			h	= hv_table.analog[info.ModeID].hporch;
			v	= hv_table.analog[info.ModeID].vporch;
		}

		printf("[%d] [%d] [%d]\n", h, v, info.ModeID);

		if((h == 0) && (v == 0)) {
			return 0;
		}


		new_pos = ((h) << 16) | (v);

		cap_set_adv7442_HV(&gEduKit->capLink.adv7442_phandle, new_pos);

		printf("ResolutionChangeHV digital[%d] pos[%d] old pos[%d] h[%d] v[%d]\n", digital, new_pos, pos, h, v);
	} else {
		fprintf(stderr, "get_signal_info failed!\n");
	}

	return 0;
}

int set_control_dei(int vport, int flag)
{
	SelectLink_OutQueChInfo select_prm;
	SelectLink_OutQueChInfo old_select_prm;

	fprintf(stderr, "!!!!!!!!!!!!!!!!!!!  set_control_dei  !!!!!!!!!!!!!!!!!!!\n");

	if(flag == 1)
		// 使用DEI
	{
		if(vport == 0) {
			select_prm.outQueId   = 0;
			select_prm.numOutCh   = 1;
			select_prm.inChNum[0] = 0;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

			old_select_prm.outQueId = 0;
			select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,	&old_select_prm);
			select_prm.outQueId   = 0;
			select_prm.numOutCh   = 2;
			select_prm.inChNum[0] = 0;
			select_prm.inChNum[1] = old_select_prm.inChNum[1];
			select_prm.inChNum[2] = 3;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
		} else if(vport == 1) {
			select_prm.outQueId   = 1;
			select_prm.numOutCh   = 1;
			select_prm.inChNum[0] = 1;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

			old_select_prm.outQueId = 0;
			select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,	&old_select_prm);
			select_prm.outQueId   = 0;
			select_prm.numOutCh   = 3;
			select_prm.inChNum[0] = old_select_prm.inChNum[0];
			select_prm.inChNum[1] = 1;
			select_prm.inChNum[2] = 4;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
		}
	} else if(flag == 0)
		// 关闭DEI
	{
		if(vport == 0) {
			select_prm.outQueId   = 2;
			select_prm.numOutCh   = 1;
			select_prm.inChNum[0] = 0;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

			old_select_prm.outQueId = 0;
			select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,	&old_select_prm);
			select_prm.outQueId   = 0;
			select_prm.numOutCh   = 3;
			select_prm.inChNum[0] = 2;
			select_prm.inChNum[1] = old_select_prm.inChNum[1];
			select_prm.inChNum[2] = 4;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
		} else if(vport == 1) {
			select_prm.outQueId   = 3;
			select_prm.numOutCh   = 1;
			select_prm.inChNum[0] = 1;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

			old_select_prm.outQueId = 0;
			select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,	&old_select_prm);
			select_prm.outQueId   = 0;
			select_prm.numOutCh   = 3;
			select_prm.inChNum[0] = old_select_prm.inChNum[0];
			select_prm.inChNum[1] = 3;
			select_prm.inChNum[2] = 4;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
		}
	} else if(flag == 2)
		// 蓝屏
	{
		if(vport == 0) {
			select_prm.outQueId   = 2;
			select_prm.numOutCh   = 1;
			select_prm.inChNum[0] = 2;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

			old_select_prm.outQueId = 0;
			select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,	&old_select_prm);
			select_prm.outQueId   = 0;
			select_prm.numOutCh   = 3;
			select_prm.inChNum[0] = 2;
			select_prm.inChNum[1] = old_select_prm.inChNum[1];
			select_prm.inChNum[2] = 4;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
		} else if(vport == 1) {
			select_prm.outQueId   = 3;
			select_prm.numOutCh   = 1;
			select_prm.inChNum[0] = 3;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

			old_select_prm.outQueId = 0;
			select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,	&old_select_prm);
			select_prm.outQueId   = 0;
			select_prm.numOutCh   = 3;
			select_prm.inChNum[0] = old_select_prm.inChNum[0];
			select_prm.inChNum[1] = 3;
			select_prm.inChNum[2] = 4;
			select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
		}
	}

	return 0;
}

static int gVGADetect = 0;

void setVGADetect(int iVGADetect)
{
	gVGADetect = iVGADetect;
}

int getVGADetect(void)
{
	return gVGADetect;
}

typedef struct vga_resolution_ {
	int width;
	int height;
} vga_resolution_t;

vga_resolution_t vga_res_support[] = {
	{1440, 900},
	{1400, 1050},
	{1366, 768},
	{1280, 1024},
	{1280, 960},
	{1280, 800},
	{1280, 768},
	{1280, 720},
	{1024, 768},
	{800, 600},
	{640, 480},
	{0, 0}
};

vga_resolution_t dvi_res_support[] = {
	{0, 0}
};


static int set_vga_resolution_src(int src_flag)
{
	SelectLink_OutQueChInfo select_prm;
	select_prm.outQueId   = 1;
	select_prm.numOutCh   = 1;

	if(src_flag) {
		select_prm.inChNum[0] = 1;
	} else {
		select_prm.inChNum[0] = 2;
	}

	select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);
}

static int check_vga_resolution_support(int SignalTmds, VCAP_VIDEO_SOURCE_CH_STATUS_S *videoStatus)
{
	int i = 0;
	int src_flag = 0;
	Device_VideoDecoderExternInforms info = {{0}, 0};

	if(GetVgaSourceState(&info) == SignalTmds) {
		for(i = 0; 0 != vga_res_support[i].height; i ++) {
			if(vga_res_support[i].height == videoStatus->frameHeight
			   && vga_res_support[i].width == videoStatus->frameWidth) {
				src_flag = 1;
				break;
			}
		}
	} else {
		for(i = 0; 0 != dvi_res_support[i].height; i ++) {
			if(dvi_res_support[i].height == videoStatus->frameHeight
			   && dvi_res_support[i].width == videoStatus->frameWidth) {
				src_flag = 1;
			}
		}
	}

	set_vga_resolution_src(src_flag);
}

static Void *detect_video_tsk(Void *prm)
{
	//int32_t last_mute_flag = audio_mute_flag;
	Int32 i = 0;
	Int32 index = 0;
	int32_t count = 0;
	int32_t source_num = 3;
	int32_t act_num = 0;
	int32_t cur_layout = 0;
	SelectLink_OutQueChInfo select_prm;
	CaptureLink_BlindInfo blind_params;
	Uint32 capture_width = 0;
	Uint32 capture_height = 0;
	VCAP_VIDEO_SOURCE_CH_STATUS_S videoStatus[2] = {{0}, {0}};
	VCAP_VIDEO_SOURCE_CH_STATUS_S historyStatus[2] = {{0}, {0}};
	cap_struct *pcaphand;
	PPT_data_info_t *pPptInfo;
	//	ResizeParam tmp;
	pcaphand  = (cap_struct *)prm;

	while(pcaphand->isDetect) {
		for(i = 0; i < 2; i++) {
			memset(&videoStatus[i], 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));

			if(i == 0) {
				cap_get_5158_resolution(pcaphand->tvp5158_phandle, &videoStatus[i]);
			} else if(i == 1) {
				cap_get_7442_resolution(&pcaphand->adv7442_phandle, &videoStatus[i]);
			}

			if(videoStatus[i].isVideoDetect) {
				if(1 == i) {
					if(videoStatus[i].frameWidth != historyStatus[i].frameWidth
					   || videoStatus[i].frameHeight != historyStatus[i].frameHeight)

					{
						//HV值调整
						vgaResolutionChangeHV();

						if(videoStatus[i].isInterlaced) {
							capture_set_input_hw(i, videoStatus[i].frameWidth, videoStatus[i].frameHeight * 2, I_Semaphore);
						} else {
							capture_set_input_hw(i, videoStatus[i].frameWidth, videoStatus[i].frameHeight, P_Semaphore);
						}

						historyStatus[i].isVideoDetect = videoStatus[i].isVideoDetect;
						historyStatus[i].frameWidth    = videoStatus[i].frameWidth;
						historyStatus[i].frameHeight   = videoStatus[i].frameHeight;
						historyStatus[i].frameInterval = videoStatus[i].frameInterval;
						historyStatus[i].isInterlaced  = videoStatus[i].isInterlaced;
						historyStatus[i].chId      = 0;
						historyStatus[i].vipInstId = i;
						printf("DetectVideo_drvTsk: resolution change !!historyStatus[%d]: frameWidth:[%d] frameHeight:[%d]\n",
						       i, historyStatus[i].frameWidth, historyStatus[i].frameHeight);
						//System_linkControl(pcaphand->link_id, CAPTURE_LINK_CMD_FORCE_RESET,
						//				NULL, 0, TRUE);

						System_linkControl(pcaphand->link_id, CAPTURE_LINK_CMD_A8_DETECT_VIDEO,
						                   &historyStatus[i], sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S), TRUE);
						capture_get_input_hw(i, &capture_width, &capture_height);
						printf("input:[%d] capture_width:[%d]  capture_height:[%d]\n",
						       0, capture_width, capture_height);

					}

					setVGADetect(1);
					check_vga_resolution_support(0, &videoStatus[i]);
				}
			} else {
				if(1 == i) {
					if(getVGADetect()) {
						setVGADetect(0);
						set_vga_resolution_src(0);
					}
				}
			}



		}

		usleep(1000000);
	}

	return NULL;
}

static Void *ledcontrol(Void *prm)
{
	unsigned i = 0;

	while(1) {
		Ioctl_Led(gEduKit->gpiofd, 3, i);
		i = i ^ 1;

		usleep(1000 * 300);
	}

	return NULL;
}
#if 0
static Void *layout_tsk(Void *prm)
{
	EduKitLinkStruct_t *penv = NULL;
	layout_st *playout = NULL;

	int32_t source_num = 3;
	int32_t act_num = 0;
	int32_t i = 0;

	penv  = (EduKitLinkStruct_t *)prm;
	playout = &penv->layout;

	sleep(5);
	playout->running = 1;

	while(playout->running) {
		playout->video_signal[2] = GeEncSrcStatue(2);
		act_num = 0;

		for(i = 0; i < source_num; i++) {
			if(playout->video_signal[i] == 1) {
				act_num++;
				//			printf("%d = video_signal[%d] %d\n",i,playout->video_signal[i], playout->cur_layout);
			}
		}

		playout->cur_singal_num = act_num;

		if(act_num == 0) {
			playout->change = 0;
			playout->cur_layout = 0;
			/* 纯色屏 */
		} else if(act_num == 3)
			/* 3路信号 */
		{
			if(playout->def_layout_3 < 31 || playout->def_layout_3 > 32) {
				playout->def_layout_3 = 31;
			}

			if(playout->cur_layout != playout->def_layout_3 || playout->change) {
				playout->change = 0;
				playout->cur_layout = playout->def_layout_3;
				/* 切换至默认的3路画面布局 */

				gEduKit->swmsLink[0].create_params.layoutPrm.ch2WinMap[0] = 0;
				gEduKit->swmsLink[0].create_params.layoutPrm.ch2WinMap[1] = 1;
				gEduKit->swmsLink[0].create_params.layoutPrm.ch2WinMap[2] = 2;

				printf("swMsSwitchLayout(gEduKit->swmsLink[0].link_id, &gEduKit->swmsLink[0].create_params, playout->def_layout_3,1)\n");
				swMsSwitchLayout(gEduKit->swmsLink[0].link_id, &gEduKit->swmsLink[0].create_params, playout->def_layout_3, 1);
			}
		} else if(act_num == 2)
			/* 2路信号 */
		{
			if(playout->def_layout_2 < 21 || playout->def_layout_2 > 26) {
				playout->def_layout_2 = 21;
			}

			if(playout->cur_layout != playout->def_layout_2 || playout->change) {
				playout->change = 0;
				playout->cur_layout =  playout->def_layout_2;
				/* 切换至默认的2路画面布局 */
				int j = 0;

				for(i = source_num - 1; i  >= 0; i--) {
					if(playout->video_signal[i] == 1) {
						gEduKit->swmsLink[0].create_params.layoutPrm.ch2WinMap[j++] = i;
					}
				}

				printf("swMsSwitchLayout(gEduKit->swmsLink[0].link_id, &gEduKit->swmsLink[0].create_params, playout->def_layout_2,1);");
				swMsSwitchLayout(gEduKit->swmsLink[0].link_id, &gEduKit->swmsLink[0].create_params, playout->def_layout_2, 1);
			}
		} else if((act_num == 1) && (playout->cur_layout != 11 || playout->change))
			/* 1路信号 */
		{
			/* 切换至默认的2路画面布局 */
			playout->change = 0;
			playout->cur_layout = 11;

			for(i = 0; i < source_num; i++) {
				if(playout->video_signal[i] == 1) {
					gEduKit->swmsLink[0].create_params.layoutPrm.ch2WinMap[0] = i;
					swMsSwitchLayout(gEduKit->swmsLink[0].link_id, &gEduKit->swmsLink[0].create_params, 11, 1);
				}
			}
		}

		usleep(300000);
	}

	return NULL;
}
#endif

int printf_select_chaninfo(int linkId, int num)
{
	SelectLink_OutQueChInfo select_prm;
	int index = 0;
	int que = 0;

	fprintf(stderr, "======================================\n");

	for(que = 0; que < num; que++) {
		select_prm.outQueId = que;
		select_get_outque_chinfo(linkId, &select_prm);
		fprintf(stderr, "outQueId = %d\n", select_prm.outQueId);
		fprintf(stderr, "numOutCh = %d\n", select_prm.numOutCh);

		for(index = 0; index < select_prm.numOutCh; index++) {
			fprintf(stderr, "inChNum[%d] = %d\n", index, select_prm.inChNum[index]);
		}
	}

	fprintf(stderr, "======================================\n");

	return 0;
}

Int32 reach_video_detect_task_process(EduKitLinkStruct_t *pstruct)
{
	Int32 status = 0;
	pstruct->capLink.isDetect = TRUE;
	status = OSA_thrCreate(&pstruct->capLink.taskHand,
	                       (OSA_ThrEntryFunc)detect_video_tsk,
	                       90,
	                       0,
	                       &pstruct->capLink);
	OSA_assert(status == 0);

	return 0;
}
#if 0
Int32 reach_layout_task_process(EduKitLinkStruct_t *pstruct)
{
	Int32 status = 0;
	pstruct->capLink.isDetect = TRUE;
	status = OSA_thrCreate(&pstruct->capLink.taskHand,
	                       (OSA_ThrEntryFunc)layout_tsk,
	                       90,
	                       0,
	                       pstruct);
	OSA_assert(status == 0);

	return 0;
}
#endif
Int32 reach_ledcontrol_process(EduKitLinkStruct_t *pstruct)
{
	Int32 status = 0;
	pstruct->capLink.isDetect = TRUE;
	status = OSA_thrCreate(&pstruct->capLink.taskHand,
	                       (OSA_ThrEntryFunc)ledcontrol,
	                       90,
	                       0,
	                       pstruct);
	OSA_assert(status == 0);

	return 0;
}


Int32 reach_config_cap_device(EduKitLinkStruct_t *pstruct)
{
	Int32 ret = 0;
	VCAP_VIDEO_SOURCE_CH_STATUS_S	videoStatus;

	//set input 1
	memset(&videoStatus, 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));

	ret = cap_config_adv7442(&(pstruct->capLink.adv7442_phandle), 0, &videoStatus);

	if(ret == 0 && videoStatus.isVideoDetect) {
		fprintf(stderr, "input1:ad7442 video detect, width = %d, height = %d\n",
		        videoStatus.frameWidth, videoStatus.frameHeight);

		if(videoStatus.isInterlaced) {
			capture_set_input_hw(1, videoStatus.frameWidth, videoStatus.frameHeight, I_Semaphore);
		} else {
			capture_set_input_hw(1, videoStatus.frameWidth, videoStatus.frameHeight, P_Semaphore);
		}

	} else {
		capture_set_input_hw(1, 1920, 1080, Bule_Semaphore);
	}

#if 0
	//set input 2
	memset(&videoStatus, 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));
	ret = cap_config_adv7441(&(pstruct->capLink.adv7441_phandle), 0, &videoStatus);

	if(ret == 0 && videoStatus.isVideoDetect) {
		fprintf(stderr, "input2:ad7441 video detect, width = %d, height = %d\n",
		        videoStatus.frameWidth, videoStatus.frameHeight);

		if(videoStatus.isInterlaced) {
			capture_set_input_hw(0, videoStatus.frameWidth, videoStatus.frameHeight, I_Semaphore);
		} else {
			capture_set_input_hw(0, videoStatus.frameWidth, videoStatus.frameHeight, P_Semaphore);
		}
	} else {
		capture_set_input_hw(0, 1920, 1080, Bule_Semaphore);
	}

#endif
	//	set_gpio_value(gCl3000->gpiofd, 40, 0);
	//	set_gpio_value(gCl3000->gpiofd, 39, 0);

	return 1;
}



