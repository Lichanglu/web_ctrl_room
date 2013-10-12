#ifndef	_REACH_H_
#define	_REACH_H_

#include "reach_app_link.h"

#define  IP_ADDR_1 "192.168.4.184"
#define  IP_ADDR_2 "192.168.4.185"
#define 	ETH0 0
#define 	ETH0_1  1

#define ENC2000 2000
#define ENC2060 2060

#define ENC110

#define	LOOK_VGA

#define TRACK


#define MAX_ALG_LINK        (2)
#define MAX_IPC_FRAMES_LINK (2)

/**********************************	  控制台  ************************************/

#define WRITE_YUV_TASK							(0)		// 写YUV图任务
#define PRINTF_BITSTREAM_INFO					(1)		// 打印编码后的比特流信息
#define DEFAULT_ENCODE_MODE						(0)		// 0--单画面 1--多画面

#define VIP0_DEFAULT_CAPTURE_PORT				(0)		// 默认采集口 0 -- DVI
#define VIP1_DEFAULT_CAPTURE_PORT				(0)		//            1 -- SDI


/*********************************************************************************/


#define INPUT_DVI 0

#define MAX_CHANNEL CHANNEL_INPUT_MAX
enum
{
	P_Semaphore,
	I_Semaphore,
	Bule_Semaphore,
};

typedef struct _layout_st_{
	Uint32	running;
	Uint32	video_signal[3];
	Uint32	def_layout_2;
	Uint32	def_layout_3;
	Uint32	cur_singal_num;
	Uint32	cur_layout;
	Uint32	change;
}layout_st;


typedef struct _dsp_alg_struct_ {
	Uint32							link_id;
	AlgLink_CreateParams	create_params;
}dspAlg_struct;


typedef struct _ipcFrames_indsp_struct_ {
	Uint32							link_id;
	IpcFramesInLinkRTOS_CreateParams	create_params;
}ipcFrames_indsp_struct;


typedef struct _RtVideoParam
{
	Int32 channel;
	Int32 width;
	Int32 height;
	Int32 type;
	pthread_mutex_t rtlock;	//锁
}RtVideoParam;

typedef struct _RtlockResolution
{
	Bool   isLock;
	Uint32 width[2];
	Uint32 height[2];
	pthread_mutex_t rtlock;	//锁
}RtlockResolution;

typedef struct _EDUKIT_LINK_STRUCT_ {
	Int32			start_runing;
	Int32			gpiofd;
	Int32			vp1_cap_type;
	Int32			vp2_cap_type;
	void *sendhand;
	void *recvhand;
	layout_st		layout;

	audio_struct	audioencLink[2];
	audio_struct	audiodecLink;
	cap_struct		capLink;
	nullsrc_struct	nullSrcLink;
	dei_struct      deiLink;
	dup_struct	    dupLink;
	merge_struct	mergeLink[3];
	Uint32				nullLinkId;
	NullLink_CreateParams	nullLinkPrm;
	dis_struct		disLink;

	select_struct	selectLink[3];
	nsf_struct      nsfLink;
	sclr_struct     sclrLink;
	videoDecoderLink_t decoderLink;
	videoEncoderLink_t encoderLink;

	Bool 			enableOsdAlgLink;
	dspAlg_struct 		osd_dspAlg_Link[MAX_ALG_LINK];
	ipcFrames_indsp_struct     ipcFrames_indsp_link[MAX_ALG_LINK];

    	UInt32                         ipcFramesOutVpssToDspId[MAX_ALG_LINK];
	IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssToDspPrm[MAX_ALG_LINK];

	RtVideoParam rtvideoparm[2];
	RtlockResolution LockResolution;
	pthread_mutex_t ledrtlock;	//锁
}EduKitLinkStruct_t;

typedef struct _R_GPIO_data_ {
	unsigned int gpio_num;
	unsigned int gpio_value;
} R_GPIO_data;

typedef struct {
	bits_user_param *hand;
	Int32   index;
	Int32 	socktid;
	Bool	IsRun;
	Int32	IsAudio;
	Void	*app_data;
}RecvHand;

#define AUDIO_STREAM						(0x6)		//通用
//#define HIGH_STREAM						(0x7)		//enc110 enc1200
//#define LOW_STREAM         				(0x95)     //enc1200
#define LOW_STREAM_T						(0x96)    //enc110
#define JPG_STREAM							(0x94)     //enc120
#define JPG_CODEC_TYPE					(0x6765706A)

#define CONNECT_SUC  						(0x3)
#define MSG_VAL_LEN						(128)

#define FRAME_OK								(99)
#define MAX_AUDIO_LEN					(512*1024)

#define EXAMIN_TIME							(800)
#define UDP_DEF_PORT									(26008)
#define LOCAL_ADDR										"192.168.4.55"



#define OPERATION_ERR							(-1)
#define OPERATION_SUCC							(0)
#define OPERATION_CONTINUE				(1) 	// 操作继续

#define FH_LEN	  sizeof(hdb_freame_head_t)
#define I_FRAME									(0x10)

#define err_print(a...) {printf("[%s %s %d] errno = %d ", __TIME__, __func__, __LINE__, errno);printf(a);putchar('\n');}
#define PRINT(a...)             {printf("[%s %s %d] ", __TIME__, __func__, __LINE__);printf(a);putchar('\n');}

#define R_ENCODE_PORT    (3100)


extern EduKitLinkStruct_t	*gEduKit;
extern int					MPmode;

Int32 reach_edukit_link_init(EduKitLinkStruct_t *pstruct);
Int32 reach_edukit_link_process(EduKitLinkStruct_t *pstruct);
Int32 reach_edukit_create_and_start(EduKitLinkStruct_t *pstruct);
#endif

