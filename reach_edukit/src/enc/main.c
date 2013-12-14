#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <assert.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <sys/types.h>
#include <dirent.h>

#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>

#include "reach.h"

#include "capture.h"
#include "select.h"
#include "audio.h"
#include "video.h"
#include "common.h"
#include "display.h"
#include "mid_task.h"
#include "mid_timer.h"
#include "osd.h"
#include "sd_demo_osd.h"
#include "nslog.h"

#include "middle_control.h"
#include "input_to_channel.h"
#include "app_signal.h"
#include "../../reach_share/xml/xml_base.h"

#include "commontrack.h"
#include "studentTcp.h"
#include "teachertracer.h"

#include "new_tcp_com.h"
#include "process_xml_cmd.h"
#include "reach_udp_snd.h"
#include "ppt_index.h"
#include "weblisten.h"
#define CAPTURE_CONFIG_FILE_1			("./input.config_1")
#define CAPTURE_CONFIG_FILE_2			("./input.config_2")

#define NSLOG_CONF		"/usr/local/reach/nslog.conf"
#define NSLOG_CNAME		"edukit_app"
#define NSLOG_OUT		">stdout;"

#if 0
//如修改 请同步修改
#define STU_CHID			0
#define STUSIDE_CHID		1
#define TEACH_CHID		2
#define VGA_CHID			3
#define JPEG_CHID			4
#endif

EduKitLinkStruct_t	*gEduKit = NULL;
zlog_category_t     *ptrackzlog  = NULL;
extern track_encode_info_t    g_track_encode_info;
extern track_save_file_info_t g_track_save_file;
extern track_strategy_info_t  g_track_strategy_info;
extern stutrack_encode_info_t	  g_stutrack_encode_info;
extern stusidetrack_encode_info_t	  g_stusidetrack_encode_info;
extern rightside_trigger_info_t	g_rightside_trigger_info;
extern int teachorstu[6];
extern int studentFlag;
int gRemoteFD = -1;
int gStuRemoteFD = -1;
static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl = {
	.isPopulated = 1,
	.ivaMap[0] =
	{
		.EncNumCh  = 2,
		.EncChList = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
		.DecNumCh  = 0,
		.DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},
	.ivaMap[1] =
	{
		.EncNumCh  = 2,
		.EncChList = {2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.DecNumCh  = 0,
		.DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},
	.ivaMap[2] =
	{
		.EncNumCh  = 0,
		.EncChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.DecNumCh  = 2,
		.DecChList = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},
};

extern void trace_init(void);


int32_t read_and_set_cap_config()
{
	FILE *file = NULL;
	int32_t type_1 = 0;
	int32_t type_2 = 0;
	int32_t c = 0;

	file = fopen(CAPTURE_CONFIG_FILE_1, "r");

	if(file == NULL) {
		type_1 = 1;
		file = fopen(CAPTURE_CONFIG_FILE_1, "w");
		fprintf(file, "%d", type_1);
		fclose(file);
	} else {
		c = fgetc(file);

		if((c - '0' >= 3) || (c - '0' < 0)) {
			fclose(file);
			type_1 = 1;
		} else {
			type_1 =  c - '0';
			fclose(file);
		}
	}

	file = fopen(CAPTURE_CONFIG_FILE_2, "r");

	if(file == NULL) {
		type_2 = 1;
		file = fopen(CAPTURE_CONFIG_FILE_2, "w");
		fprintf(file, "%d", type_2);
		fclose(file);
	} else {
		c = fgetc(file);

		if((c - '0' >= 3) || (c - '0' < 0)) {
			fclose(file);
			type_2 = 1;
		} else {
			type_2 =  c - '0';
			fclose(file);
		}
	}

	Cap_SetSource7442Chan(&gEduKit->capLink.adv7442_phandle, type_2);

	gEduKit->vp1_cap_type = type_1;
	gEduKit->vp2_cap_type = type_2;

	fprintf(stderr, "read_and_set_cap_config, type_1 = %d, type_2 = %d\n", type_1, type_2);

	return 0;
}


int32_t get_cap_vp1_config()
{
	return gEduKit->vp1_cap_type;
}

int32_t get_cap_vp2_config()
{
	return gEduKit->vp2_cap_type;
}



int RtpParmLockInit(int chane)
{
	return pthread_mutex_init(&gEduKit->rtvideoparm[chane].rtlock, NULL);
}

int RtpParmLockdeInit(int chane)
{
	return pthread_mutex_destroy(&gEduKit->rtvideoparm[chane].rtlock);
}


int32_t SetEncBitrate(int32_t chId, int32_t Bitrate)
{

	if((gEduKit != NULL) && (Bitrate >= 128 * 1000)) {

		printf("SetEncBitrate EncId [%d] Bitrate[%d]\n", chId, Bitrate);

		enc_set_bitrate(gEduKit->encoderLink.encLink.link_id, chId , Bitrate);
	}

	return 0;
}

static int init_config_file(void)
{
	if(NULL == opendir(".config")) {
		system("mkdir .config");
		fprintf(stderr, "mkdir .config\n");
	}

	return 0;
}

Int32 Ioctl_Led(int gpio, Int32 led, Int32 state)
{
	pthread_mutex_lock(&gEduKit->ledrtlock);
	R_GPIO_data data = {0};

	if(led == 0) {
		data.gpio_num = 34;
		data.gpio_value = state;
		ioctl(gpio, 0x55555555, &data);
	} else if(led == 1) {
		data.gpio_num = 35;
		data.gpio_value = state;
		ioctl(gpio, 0x55555555, &data);
	} else {
		data.gpio_num = 33;
		data.gpio_value = state;
		ioctl(gpio, 0x55555555, &data);
	}

	pthread_mutex_unlock(&gEduKit->ledrtlock);
	return 0;
}



typedef	void SIGFUNC(int);

/*信号包裹函数*/
Sigfunc *SIGNAL(int signo, SIGFUNC *func)
{
	SIGFUNC	*sigfunc;

	if((sigfunc = signal(signo, func)) == SIG_ERR) {
		printf("ERROR:  signal error \n");
	}

	return(sigfunc);
}


int ListenSeries(void *pParam)
{
	unsigned char data[256];
	char szData[256];
	int len;
	fd_set rfds;

	while(1) {

		memset(data, 0, 256);
		FD_ZERO(&rfds);
		FD_SET(gRemoteFD, &rfds);
		select(gRemoteFD + 1, &rfds, NULL, NULL, NULL);
		len = read(gRemoteFD, data, 256) ; //成功返回数据量大小，失败返回－1
		szData[0] = 0;
		szData[1] = 3 + 1;
		szData[2] = 48;
		szData[3] = data[3];
		//printf( "read comm\n");
		//printf( "tea get_cam_position 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10]);
		save_position_zoom(data);

	}

	return len;
}


int StuListenSeries(void *pParam)
{
	unsigned char data[256];
	char szData[256];
	int len;
	fd_set rfds;

	while(1) {

		memset(data, 0, 256);
		FD_ZERO(&rfds);
		FD_SET(gStuRemoteFD, &rfds);
		select(gStuRemoteFD + 1, &rfds, NULL, NULL, NULL);
		len = read(gStuRemoteFD, data, 256) ; //成功返回数据量大小，失败返回－1
		szData[0] = 0;
		szData[1] = 3 + 1;
		szData[2] = 48;
		szData[3] = data[3];
		//	printf("read comm\n");
		//printf("get_cam_position 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10]);


		//save_position_zoom(data);
		stusave_position_zoom(data);

	}

	return len;
}

#define	XML_CMD




Int32 EduKitInit(EduKitLinkStruct_t **phand)
{
	setpriority(PRIO_PROCESS, 0, -20);

	EduKitLinkStruct_t *pEduKit = NULL;

	if(NULL == phand) {
		return -1;
	}

	*phand = malloc(sizeof(EduKitLinkStruct_t));

	if(*phand == NULL) {
		printf("EduKitInit: malloc EduKitLinkStruct_t fail!\n");
		return -1;
	}

	/*取消PIPE坏的信号*/
	SIGNAL(SIGPIPE, SIG_IGN);

	pEduKit = *phand;
	printf("[EduKitInit] pEduKit:[%p]\n", pEduKit);
	memset(pEduKit, 0x0, sizeof(EduKitLinkStruct_t));


	pEduKit->gpiofd = open_gpio_device();

	if(pEduKit->gpiofd == -1) {

		printf("EduKitInit: Open Rgpio fail!\n");
		goto FAIL;
	}




	init_config_file();
	init_HV_table();
	init_kernel_version();
	ReadHDMICfgFile();
	StartEncoderMangerServer();
	RtpParmLockInit(0);
	RtpParmLockInit(1);
	pthread_mutex_init(&pEduKit->LockResolution.rtlock, NULL);
	pthread_mutex_init(&pEduKit->ledrtlock, NULL);
	hdmiDisplayResolution_t *pHDRes = getHdmiDisplayResolution();
	pEduKit->HDMIRes.ResIdx = pHDRes->res;
	pEduKit->HDMIRes.width = atoi(pHDRes->width);
	pEduKit->HDMIRes.height = atoi(pHDRes->height);
	stream_send_cond_t ssc[MAX_UDP_SEND_NUM];
	udp_send_module_handle *pUdpSend[MAX_UDP_SEND_NUM];

	get_local_ip("eth1", ssc[0].ip);
	//send to room
	ssc[0].video_port = UDP_SEND_ROOM_PORT;
	ssc[0].audio_port = UDP_SEND_ROOM_PORT;
	printf("[EduKitInit] ip:[%s] video_port:[%d] audio_port:[%d]",
	       ssc[0].ip, ssc[0].video_port, ssc[0].audio_port);
	pUdpSend[0] = UdpSend_init(&ssc[0]);
	pEduKit->sendhand[0] = pUdpSend[0];

	r_strcpy(ssc[1].ip, UDP_SEND_HD_IP);
	ssc[1].video_port = UDP_SEND_HD_PORT;
	ssc[1].audio_port = UDP_SEND_HD_PORT;
	printf("[EduKitInit] ip:[%s] video_port:[%d] audio_port:[%d]",
	       ssc[1].ip, ssc[1].video_port, ssc[1].audio_port);
	pUdpSend[1] = UdpSend_init(&ssc[1]);
	pEduKit->sendhand[1] = pUdpSend[1];


#ifdef HAVE_JPEG
	r_strcpy(ssc[2].ip, ssc[0].ip);
	ssc[2].video_port = UDP_SEND_JPEG_ROOM_PORT;
	ssc[2].audio_port = UDP_SEND_JPEG_ROOM_PORT;
	printf("[EduKitInit] ip:[%s] video_port:[%d] audio_port:[%d]",
	       ssc[2].ip, ssc[2].video_port, ssc[2].audio_port);
	pUdpSend[2] = UdpSend_init(&ssc[2]);
	pEduKit->sendhand[2] = pUdpSend[2];
#endif
	printf("[EduKitInit] pUdpSend[0]:[%p] pUdpSend[1]:[%p]\n", pUdpSend[0], pUdpSend[1]);
	pEduKit->encoderLink.bitsparam.appdata = pUdpSend;
	pEduKit->encoderLink.bits_callback_fxn = inHostStreamUdpSendProcess;

	pEduKit->decoderLink.bits_callback_fxn = OutHostStreamProcess;
	//link 初始化
	reach_edukit_link_init(pEduKit);

	//xml初始化
	xmlInitParser();

	mid_task_init();

	mid_timer_init();

	//配置采集设备
	reach_config_cap_device(pEduKit);
	VCAP_VIDEO_SOURCE_CH_STATUS_S	videoStatus;
	cap_config_tvp5158(&(pEduKit->capLink.tvp5158_phandle), 0, &videoStatus);
	/* 音频 */
	audio_dsp_system_init();


	//系统初始化
	host_system_init();

	//检测单板
	vpss_detect_board();

	//重置设备
	vpss_reset_video_devices();

	tiler_disable_allocator();

	//协处理器分配
	video_set_ch2ivahd_map_tbl(&systemVid_encDecIvaChMapTbl);

	//配置源侦测类型
	read_and_set_cap_config();

	Ioctl_Led(pEduKit->gpiofd, 2, 1);
	pthread_t id_ppt;

	if(pthread_create(&id_ppt, NULL, (void *)PPTindexTask, (void *)NULL)) {
		printf("create PPTindexTask() failed\n");
		exit(0);
	}

#ifdef TRACK
	pthread_t id_teacher ;
	pthread_t id_student ;
	pthread_t id_listen ;
	pthread_t id_stulisten ;
	pthread_t id_webtcp ;

	ptrackzlog = zlog_get_category("TRACK");

	if(!ptrackzlog) {
		return 1;
	}

	zlog_info(ptrackzlog, "TRACK STUTRACK IS CREATE ING!!!\n");


	//跟踪机新加部分
	InitRemoteStruct(4);

	gRemoteFD    = CameraCtrlInit(0);
	gStuRemoteFD = CameraCtrlInit(1);

	if(gRemoteFD < 0 || gStuRemoteFD < 0) {
		printf("Initial CameraCtrlInit() Error\n");
		zlog_info(ptrackzlog, "Initial CameraCtrlInit() Error\n");
		return 1;
	}


	init_save_track_mutex();
	init_save_stutrack_mutex();
	init_save_stusidetrack_mutex();
	track_init(&pEduKit->osd_dspAlg_Link[0].create_params.trackCreateParams.TrackParms);
	stutrack_init(&pEduKit->osd_dspAlg_Link[0].create_params.stutrackCreateParams.StuTrackParms);
	stusidetrack_init(&pEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams.StuSideTrackParms);
	init_limit_position_set();
	g_stusidetrack_encode_info.is_encode = 0;
	g_stutrack_encode_info.is_encode = 0;
	g_track_encode_info.is_encode = 0;

	SD_Demo_AllocBufInfo bufInfo;
	SD_Demo_allocBuf(0, 1920 * 1080 * 2, 128, &bufInfo);

	if(bufInfo.virtAddr == NULL) {

		zlog_info(ptrackzlog, "SD_Demo_allocBuf Is Error!!!\n");
		return 1;

	}

	AlgLink_StuSideTrackCreateParams *pParams = NULL;
	pParams = &pEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams;
	pParams->StuSideAddr[0][0] = bufInfo.physAddr;
	pParams->StuSideAddr[0][1] = bufInfo.virtAddr;
	printf("SD_Demo_allocBuf %p %d\n", bufInfo.physAddr, bufInfo.bufsize);
	FILE *fp = fopen(CLASS_VIEW_JPG, "r");

	if(NULL != fp) {
		fread((unsigned char *)bufInfo.virtAddr,
		      1920 * 1080, 1, fp);
		fclose(fp);
	}

	if(pthread_create(&id_listen, NULL, (void *)ListenSeries, (void *)NULL)) {
		printf("create ListenSeries() failed\n");
		exit(0);
	}


	if(pthread_create(&id_stulisten, NULL, (void *)StuListenSeries, (void *)NULL)) {
		printf("create StuListenSeries() failed\n");
		exit(0);
	}



	if(pthread_create(&id_student, NULL, (void *)studentTrace, (void *)NULL)) {
		printf("create studentTrace() failed\n");
		exit(0);
	}


	if(pthread_create(&id_teacher, NULL, (void *)TEACHERTRACER, (void *)NULL)) {
		printf("create TEACHERTRACER() failed\n");
		exit(0);
	}

#if 0

	if(pthread_create(&id_webtcp, NULL, (void *)WebListen, (void *)NULL)) {
		printf("create studentTrace() failed\n");
		exit(0);
	}

#endif

	__call_preset_position(TRIGGER_POSITION1);
	__stucall_preset_position(TRIGGER_POSITION42);
	zlog_info(ptrackzlog, "TRACK STUTRACK IS CREATE SUCCESSFUL!!![%s]\n", GetTrackVersion);


	pEduKit->osd_dspAlg_Link[0].create_params.enableSTUTRACKAlg = TRUE;
	pEduKit->osd_dspAlg_Link[0].create_params.enableTRACKAlg	 = TRUE;
	pEduKit->osd_dspAlg_Link[0].create_params.enableSTUSIDETRACKAlg	 = TRUE;

	#if 0
	gEduKit->SupportStuSide = 0;
	if(pEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams.StuSideTrackParms.dynamic_param.control_mode == AUTO_CONTROL) {
		pEduKit->osd_dspAlg_Link[0].create_params.enableSTUSIDETRACKAlg	 = TRUE;
		gEduKit->SupportStuSide = 1;
	}
	#endif


#endif

	return 1;

FAIL:
	free(*phand);
	*phand = NULL;
	return -1;
}

//主要处理dsp消息，
/* A8 Event handler. Processes event received from other cores. */
Int32 Track_eventHandler(UInt32 eventId, Ptr pPrm, Ptr appData)
{
	static unsigned int				flag = 0;
	char data[20] = {0};

	if(eventId == VSYS_EVENT_TRACK_STATUS) {
		AlgLink_TrackChStatus *pTrackStat = (AlgLink_TrackChStatus *)pPrm;
		AlgLink_TrackChStatus  TrackStat = {0};

		memcpy(&TrackStat, pTrackStat, sizeof(AlgLink_TrackChStatus));

		//printf("Ch[%d] cmd_type[%d] track_status[%d] server_cmd[%d] move_direction[%d] move_distance[%d]\n",\
		//	pTrackStat->chId,pTrackStat->args.cmd_type,pTrackStat->args.track_status,\
		//	pTrackStat->args.server_cmd,pTrackStat->args.move_direction,pTrackStat->args.move_distance);
		if(gEduKit->osd_dspAlg_Link[0].create_params.trackCreateParams.TrackParms.dynamic_param.control_mode == AUTO_CONTROL) {

			if(g_track_encode_info.track_status) {
				flag ++;

				if((flag % 25) == 0) {
					g_track_save_file.set_cmd = GET_CUR_POSITION;
					__get_cam_position();
				}
			} else {
				g_track_strategy_info.blackboard_region_flag1 = 0;
				g_track_strategy_info.blackboard_region_flag2 = 0;
			}

			if(TrackStat.args.position1_mv_status != 0) {
				g_track_strategy_info.position1_mv_flag = 1;
				//DEBUG(DL_DEBUG, "position1_mv_flag==1\n");
			} else {
				g_track_strategy_info.position1_mv_flag = 0;
			}

			//if(teachorstu == 1)
			{
				cam_ctrl_cmd(&TrackStat.args);
			}
		}
	} else if(eventId == VSYS_EVENT_STUTRACK_STATUS) {
		AlgLink_StuTrackChStatus *pstuTrackStat = (AlgLink_StuTrackChStatus *)pPrm;
		AlgLink_StuTrackChStatus  stuTrackStat;

		memcpy(&stuTrackStat, pstuTrackStat, sizeof(AlgLink_StuTrackChStatus));

		// printf("Ch[%d] cmd_type[%d] cam_position[%d] server_cmd[%d] size[%d] x[%d] y[%d]\n",\
		//	pTrackStat->chId,pTrackStat->args.cmd_type,pTrackStat->args.cam_position,\
		//	pTrackStat->args.server_cmd,pTrackStat->args.size,pTrackStat->args.c_x,pTrackStat->args.c_y);
		if((gEduKit->osd_dspAlg_Link[0].create_params.stutrackCreateParams.StuTrackParms.dynamic_param.control_mode == AUTO_CONTROL)
		   && ((g_track_strategy_info.students_track_flag == 1) || (studentFlag == 1))) {

			if(gEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams.StuSideTrackParms.dynamic_param.control_mode == AUTO_CONTROL) {
				cam_ctrl_cmd_rightside(&stuTrackStat.args);
			} else {
				stucam_ctrl_cmd(&stuTrackStat.args);
			}
		}

	} else if(eventId == VSYS_EVENT_STUSIDETRACK_STATUS) {
		AlgLink_StuSideTrackChStatus *pstuSideTrackStat = (AlgLink_StuSideTrackChStatus *)pPrm;
		AlgLink_StuSideTrackChStatus  stuSideTrackStat;

		memcpy(&stuSideTrackStat, pstuSideTrackStat, sizeof(AlgLink_StuSideTrackChStatus));


		if(gEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams.StuSideTrackParms.dynamic_param.control_mode == AUTO_CONTROL) {
			static int32_t	track_status = 0;

			if(stuSideTrackStat.args.cmd_type != track_status) {

				track_status = stuSideTrackStat.args.cmd_type;

				if(gEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams.StuSideTrackParms.dynamic_param.track_mode == 0) {
					printf("g_rightside_trigger_info.nTriggerVal = %d \n", stuSideTrackStat.args.cmd_type);
				//	g_rightside_trigger_info.nTriggerType = 0;
					g_rightside_trigger_info.nTriggerVal  = stuSideTrackStat.args.cmd_type;
				} 
				else 
				{
					if(stuSideTrackStat.args.cmd_type == STUDENTS_UP) {

						printf("g_rightside_trigger_info.nTriggerVal = %d \n", stuSideTrackStat.args.cmd_type);
						//g_rightside_trigger_info.nTriggerType = 1;
						g_rightside_trigger_info.nTriggerVal  = stuSideTrackStat.args.cmd_type;
					}
				}
			}

		}


	} else if(eventId == VSYS_EVENT_STUSIDETRACK_SAVE_VIEW) {
		Int8 *addr = gEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams.StuSideAddr[0][1];
		Int32 w = gEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams.StuSideTrackParms.static_param.video_width;
		Int32 h = gEduKit->osd_dspAlg_Link[0].create_params.stusidetrackCreateParams.StuSideTrackParms.static_param.video_height;

		if(addr != NULL) {
			FILE *fp = fopen(CLASS_VIEW_JPG, "w");

			if(NULL != fp) {
				fwrite(addr, w * h, 1, fp);
				fclose(fp);
				system("sync");
				//printf("VSYS_EVENT_STUSIDETRACK_SAVE_VIEW wxh %dx%d \n",w,h);
			}
		}

	}

	return 0;
}





static Int32 reach_print_current_status(EduKitLinkStruct_t *pstruct, Int32 time)
{
	System_LinkChInfo2 params;

	video_calc_cpuload_start();
	vpss_calc_cpuload_start();
	dsp_calc_cpuload_start();

	int count = 0;

	while(1) {

		fprintf(stderr, "demo is running........ %d\n", count++);


		dec_print_ivahd_statistics(pstruct->decoderLink.decLink.link_id);
		sleep(1);
#if 1
		fprintf(stderr, "====================== capture statistics =========================\n");
		cap_print_advstatistics(pstruct->capLink.link_id);
		sleep(1);
		fprintf(stderr, "=====================================================================\n");
#if 1

		sclr_print_statics(pstruct->sclrLink.link_id);
		sleep(1);
		fprintf(stderr, "=====================================================================\n");

		nsf_print_static(pstruct->nsfLink.link_id);
		sleep(1);
		fprintf(stderr, "=====================================================================\n");

		dei_print_statics(pstruct->deiLink.link_id);
		sleep(1);
		fprintf(stderr, "=====================================================================\n");

		fprintf(stderr, "====================== enc ivahd statistics =========================\n");
		enc_print_ivahd_statistics(pstruct->encoderLink.encLink.link_id);
		sleep(1);
		fprintf(stderr, "=====================================================================\n");
		sleep(2);
		fprintf(stderr, "====================== enc statistics =========================\n");
		enc_print_statistics(pstruct->encoderLink.encLink.link_id);
		sleep(1);
		fprintf(stderr, "=====================================================================\n");
		sleep(2);
#endif



#if 1
		fprintf(stderr, "====================== video cpuload =========================\n");
		video_print_cpuload();
		sleep(1);
		fprintf(stderr, "=====================================================================\n");
		sleep(2);
		fprintf(stderr, "====================== vpss cpuload =========================\n");
		vpss_print_cpuload();
		sleep(1);
		fprintf(stderr, "=====================================================================\n");
		sleep(2);

		dsp_print_cpuload();
#endif
		video_calc_cpuload_reset();
		vpss_calc_cpuload_reset();
		dsp_calc_cpuload_reset();

#endif
		sleep(time);
	}

	return 0;
}

static Int32 ipcamera_sclr_cutout(Int32 sclrLinkId, Int32 chid, Int32 width, Int32 height)
{
	System_LinkChInfo2 params;
	params.chid = chid;
	params.chinfo.startX = 32;
	params.chinfo.startY = 24;
	params.chinfo.width = width;
	params.chinfo.height = height; //pal 288 n 240
	sclr_set_inchinfo(sclrLinkId, &params);
	return 0;
}

static xml_new_tcp_task_cmd(void)
{
	int ret = -1;
	pthread_t new_tcp_thread_id[2];
	int index1 = 0, index2 = 1;
	pthread_t st_report_id[2];

	ret = pthread_create(&new_tcp_thread_id[index1], NULL, open_new_tcp_task, (void *)&index1);

	if(ret < 0) {
		printf("create DSP1TCPTask() failed\n");
		return 0;
	}

	ret = pthread_create(&st_report_id[index1], NULL, report_pthread_fxn, (void *)&index1);

	if(ret < 0) {
		printf("create DSP1TCPTask() failed\n");
		return 0;
	}

	ret = pthread_create(&new_tcp_thread_id[index2], NULL, open_new_tcp_task, (void *)&index2);

	if(ret < 0) {
		printf("create DSP2TCPTask() failed\n");
		return 0;
	}

	ret = pthread_create(&st_report_id[index2], NULL, report_pthread_fxn, (void *)&index2);

	if(ret < 0) {
		printf("create DSP2TCPTask() failed\n");
		return 0;
	}

	return ret;
}


static void setHDMIOutRes(EduKitLinkStruct_t *pstruct)
{
	SclrLink_SclrMode sclrparams;
	sclrparams.chId = 4;
	sclrparams.SclrMode = TRUE;
	sclr_set_SclrMode(pstruct->sclrLink.link_id, &sclrparams);
	SclrLink_chDynamicSetOutRes out;
	out.chId = 4;
	out.width = pstruct->HDMIRes.width;
	out.height = pstruct->HDMIRes.height;
	out.pitch[0] = pstruct->HDMIRes.width * 2;
	sclr_set_output_resolution(pstruct->sclrLink.link_id, &out);
}
int main()
{
	Int32 ret = -1;
	Int32 i = 0;
	//初始化Media 库
	///MediaSysInit();


	trace_init();
	nslog_conf_info_t info;
	strcpy(info.conf, NSLOG_CONF);
	strcpy(info.cname, NSLOG_CNAME);
	strcpy(info.output, NSLOG_OUT);

	ret = NslogInit(&info);

	if(ret != 0) {
		return -1;
	}

	//RegDiskDetectMoudle();

	//edukit 初始化
	ret = EduKitInit(&gEduKit);

	if(ret != 1) {
		return -1;
	}

	reach_edukit_link_process(gEduKit);

#ifdef TRACK
	Vsys_registerEventHandler(Track_eventHandler, NULL);
#endif

	reach_edukit_create_and_start(gEduKit);
	reach_audio_enc_process(gEduKit);


	//源检测任务
	reach_video_detect_task_process(gEduKit);
	sleep(3);
	gEduKit->Start = 1;

#if WRITE_YUV_TASK
	reach_writeyuv_process(gEduKit);
#endif
	/*(0,0)(768,0)(0,624)(768,624)*/
	System_LinkChInfo2 params;
	params.chid = 0;
	params.chinfo.startX = 0;
	params.chinfo.startY = 0;
	params.chinfo.width = 704;
	params.chinfo.height = 512; //pal 288 n 240
	sclr_set_inchinfo(gEduKit->sclrLink.link_id, &params);

	params.chid = 1;
	params.chinfo.startX = 768;
	params.chinfo.startY = 0;
	params.chinfo.width = 704;
	params.chinfo.height = 512; //pal 288 n 240
	sclr_set_inchinfo(gEduKit->sclrLink.link_id, &params);

	SclrLink_SclrMode sclrparams;
	sclrparams.chId = 3;//2;
	sclrparams.SclrMode = TRUE;
	//sclr_set_SclrMode(gEduKit->sclrLink.link_id, &sclrparams);

	//学生
	SclrLink_chDynamicSetOutRes out;
	out.chId = 0;
	out.width = 704;
	out.height = 576;
	out.pitch[0] = 1408;

	sclr_set_output_resolution(gEduKit->sclrLink.link_id, &out);
	sclr_set_framerate(gEduKit->sclrLink.link_id, 0, 30, 22);

	//学生辅助
	out.chId = 1;
	out.width = 704;
	out.height = 576;
	out.pitch[0] = 1408;

	sclr_set_output_resolution(gEduKit->sclrLink.link_id, &out);
	sclr_set_framerate(gEduKit->sclrLink.link_id, 1, 30, 10);

	//老师
	out.chId = 3;//2;
	out.width = 704;
	out.height = 576;
	out.pitch[0] = 1408;

	sclr_set_output_resolution(gEduKit->sclrLink.link_id, &out);
	sclr_set_framerate(gEduKit->sclrLink.link_id, 3, 30, 22);

	setHDMIOutRes(gEduKit);
	char date[128] = {0};
	int temp= 0;
	app_init_fpga_version();
#if 0
	//HDMI
	out.chId = 3;
	out.width = gEduKit->HDMIRes.width;
	out.height = gEduKit->HDMIRes.height;
	out.pitch[0] = 1408;
	sclr_set_output_resolution(gEduKit->sclrLink.link_id, &out);
#endif
	//hdmiDisplaySetResolution(gEduKit->disLink.link_id, SYSTEM_STD_XGA_60);

	//ipcamera_sclr_cutout(gEduKit->sclrLink[0].link_id, 2, 1280, 720);
	//	listenSdkTask();	//需起为线程

	//reach_print_current_status(gEduKit, 10);

	g_track_encode_info.is_encode    = 0;
	g_stutrack_encode_info.is_encode = 0;
	g_stusidetrack_encode_info.is_encode = 0;
	enc_disable_channel(gEduKit->encoderLink.encLink.link_id, TEACH_CHID);

	enc_disable_channel(gEduKit->encoderLink.encLink.link_id, STU_CHID);

	enc_disable_channel(gEduKit->encoderLink.encLink.link_id, STUSIDE_CHID);
	sleep(2);
	xml_new_tcp_task_cmd();
	int index = 0;
	enc_app_weblisten_init((void *)&index);
	dsp_calc_cpuload_start();
	EncLink_GetDynParams pEncParams;
	int chid = 0;
	enc_set_fps(gEduKit->encoderLink.encLink.link_id, VGA_CHID, 25, 2500 * 1000);
	enc_set_fps(gEduKit->encoderLink.encLink.link_id, STU_CHID, 22, 512 * 1000);
	enc_set_fps(gEduKit->encoderLink.encLink.link_id, STUSIDE_CHID, 22, 512 * 1000);
	enc_set_fps(gEduKit->encoderLink.encLink.link_id, TEACH_CHID, 22, 512 * 1000);
	enc_set_interval(gEduKit->encoderLink.encLink.link_id, STU_CHID, 44);
	enc_set_interval(gEduKit->encoderLink.encLink.link_id, STUSIDE_CHID, 20);
	enc_set_interval(gEduKit->encoderLink.encLink.link_id, TEACH_CHID, 44);
	enc_set_interval(gEduKit->encoderLink.encLink.link_id, JPEG_CHID, 12);
	sleep(10);

	while(1) {

#if 0
		pEncParams.chId = chid;
		enc_get_dynparams(gEduKit->encoderLink.encLink.link_id, &pEncParams);
		nslog(NS_DEBUG, "EncLink_GetDynParams chId:[%d] width:[%d] height:[%d] bitrate:[%d] fps:[%d] int:[%d]\n",
		      pEncParams.chId, pEncParams.inputWidth, pEncParams.inputHeight,
		      pEncParams.targetBitRate, pEncParams.targetFps, pEncParams.intraFrameInterval);

		if(2 == chid ++) {
			chid = 0;
		}

#endif

		sleep(1);
		dec_print_statistics(gEduKit->decoderLink.decLink.link_id);
		sleep(1);

		cap_print_advstatistics(gEduKit->capLink.link_id);
		sleep(1);

		sclr_print_statics(gEduKit->sclrLink.link_id);
		sleep(1);

		nsf_print_static(gEduKit->nsfLink.link_id);
		sleep(1);

		enc_print_statistics(gEduKit->encoderLink.encLink.link_id);
		sleep(1);

		dsp_print_cpuload();
		sleep(1);
		System_linkControl(gEduKit->osd_dspAlg_Link[0].link_id, ALG_LINK_TRACK_CMD_PRINT_STATE,
		                   NULL, 0, TRUE);

		dsp_calc_cpuload_reset();

	}

	xmlCleanupParser();
	zlog_fini();

	return 0;
}

