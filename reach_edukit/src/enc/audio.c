
#include <stdio.h>

#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>


#include "reach.h"
#include "common.h"
#include "audio.h"

#define  TEST_TWO_AUDIO  1

#include "mid_mutex.h"
#include "mid_timer.h"

#include "input_to_channel.h"
#include <nslog.h>
#include <reach_os.h>
#include "params.h"
#include "media_msg.h"
#include "reach_udp_snd.h"
#define AVIIF_KEYFRAME						0x00000010
char AUDIONAME[2][AUDIO_DEVICE_NAME_LENGTH] = { "default:CARD=mcasp0", "default:CARD=EVM"};



static Int32 adjust_volume(int chId, int LVolume, int RVolume);
static unsigned int getCurrentTime_zl(void);

static unsigned int getCurrentTime_zl(void)
{
	struct timeval tv;
	struct timezone tz;
	unsigned long long ultime = 0 ;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec;
	ultime = ultime * 1000 + tv.tv_usec / 1000;

	return (ultime);
}



/*设置输入的物理接口制式，教育套包不需要此接口*/
int32_t AudioSetInputIndex(int8_t *InputIndex)
{
	printf("Warnning,this is a NULL function,please check it \n");
	return 0;

#if 0
	int8_t input[5] = {0};

	if(InputIndex == NULL) {
		OSA_printf("AudioSetInputIndex: param is NULL!\n");
		return -1;
	}

	if(r_strlen(InputIndex) != 4) {
		OSA_printf("AudioSetInputIndex: param is ERROR! InputIndex = %s\n", InputIndex);
		return -1;
	}

	pthread_mutex_lock(&gavolume.mutex);
	r_bzero(gavolume.InputIndex, sizeof(gavolume.InputIndex));
	r_memcpy(gavolume.InputIndex, InputIndex, 4);
	pthread_mutex_unlock(&gavolume.mutex);

	return 0;






	r_memcpy(input, InputIndex, 4);
	nslog(NS_WARN, "-------------INPUT = %s", input);

	//凤凰头 line in
	if(input[2] == '1') {
		r_system((int8_t *)"amixer cset -c 1 numid=2,iface=MIXER,name='LINPUT2' 7");
		r_system((int8_t *)"amixer cset -c 1 numid=4,iface=MIXER,name='RINPUT2' 7");

		r_system((int8_t *)"amixer cset -c 1 numid=5,iface=MIXER,name='RINPUT3' 200");
		r_system((int8_t *)"amixer cset -c 1 numid=6,iface=MIXER,name='LINPUT3' 200");
	} else if(input[2] == '0') {
		r_system((int8_t *)"amixer cset -c 1 numid=2,iface=MIXER,name='LINPUT2' 0");
		r_system((int8_t *)"amixer cset -c 1 numid=4,iface=MIXER,name='RINPUT2' 0");

		r_system((int8_t *)"amixer cset -c 1 numid=5,iface=MIXER,name='RINPUT3' 0");
		r_system((int8_t *)"amixer cset -c 1 numid=6,iface=MIXER,name='LINPUT3' 0");
	}

	// 三芯口line in
	if(input[0] == '1') {
		r_system((int8_t *)"amixer cset -c 1 numid=1,iface=MIXER,name='LINPUT3' 7");
		r_system((int8_t *)"amixer cset -c 1 numid=3,iface=MIXER,name='RINPUT3' 7");

		r_system((int8_t *)"amixer cset -c 1 numid=5,iface=MIXER,name='RINPUT3' 200");
		r_system((int8_t *)"amixer cset -c 1 numid=6,iface=MIXER,name='LINPUT3' 200");
	} else if(input[0] == '0') {
		r_system((int8_t *)"amixer cset -c 1 numid=1,iface=MIXER,name='LINPUT3' 0");
		r_system((int8_t *)"amixer cset -c 1 numid=3,iface=MIXER,name='RINPUT3' 0");

		r_system((int8_t *)"amixer cset -c 1 numid=5,iface=MIXER,name='RINPUT3' 0");
		r_system((int8_t *)"amixer cset -c 1 numid=6,iface=MIXER,name='LINPUT3' 0");
	}

	//凤凰头mic in
	if(input[3] == '1') {
		r_system((int8_t *)"amixer cset -c 0 numid=2,iface=MIXER,name='LINPUT2' 7");
		r_system((int8_t *)"amixer cset -c 0 numid=4,iface=MIXER,name='RINPUT2' 7");

		r_system((int8_t *)"amixer cset -c 0 numid=5,iface=MIXER,name='RINPUT3' 185");
		r_system((int8_t *)"amixer cset -c 0 numid=6,iface=MIXER,name='LINPUT3' 185");
	} else if(input[3] == '0') {
		r_system((int8_t *)"amixer cset -c 0 numid=2,iface=MIXER,name='LINPUT2' 0");
		r_system((int8_t *)"amixer cset -c 0 numid=4,iface=MIXER,name='RINPUT2' 0");

		r_system((int8_t *)"amixer cset -c 0 numid=5,iface=MIXER,name='RINPUT3' 0");
		r_system((int8_t *)"amixer cset -c 0 numid=6,iface=MIXER,name='LINPUT3' 0");
	}

	//复合口mic in
	if(input[1] == '1') {
		r_system((int8_t *)"amixer cset -c 0 numid=1,iface=MIXER,name='LINPUT3' 7");
		r_system((int8_t *)"amixer cset -c 0 numid=3,iface=MIXER,name='RINPUT3' 7");

		r_system((int8_t *)"amixer cset -c 0 numid=5,iface=MIXER,name='RINPUT3' 185");
		r_system((int8_t *)"amixer cset -c 0 numid=6,iface=MIXER,name='LINPUT3' 185");
	} else if(input[1] == '0') {
		r_system((int8_t *)"amixer cset -c 0 numid=1,iface=MIXER,name='LINPUT3' 0");
		r_system((int8_t *)"amixer cset -c 0 numid=3,iface=MIXER,name='RINPUT3' 0");

		r_system((int8_t *)"amixer cset -c 0 numid=5,iface=MIXER,name='RINPUT3' 0");
		r_system((int8_t *)"amixer cset -c 0 numid=6,iface=MIXER,name='LINPUT3' 0");
	}

	return 0;
#endif
}

static int32_t awrite_file(char *file, void *buf, int buflen)
{
	static FILE *fp = NULL;

	if(NULL == fp) {
		fp =	fopen(file, "w");

		if(NULL == fp) {
			nslog(NS_ERROR, "fopen  : %s", strerror(errno));
			return -1;
		}
	}

	printf("[write_file] buf : [%p], buflen : [%d] , fp : [%p]\n", buf, buflen, fp);
	fwrite(buf, buflen, 1, fp);
	return 0;
}


#if 1
static void *AudioPcmDataInProcess(audio_struct *audioinst)
{
	audio_buf       *pa_inbuf = NULL;
	Int32			ret = 0;

	Void			*aread_handle = NULL;


	if(audioinst == NULL) {
		printf("AudioPcmDataInProcess: param is NULL!\n");
		return NULL;
	}

	aread_handle = (Void *)audioinst[0].pacaphandle;


	while(TRUE) {
		checkAudioCapParam(audioinst);//检测音频参数有没有变化
		ret = audio_indata_get_empty_buf(&(audioinst[0].paenchandle->indata_user_handle), &pa_inbuf);

		if(ret < 0) {
			printf("audio get empty buf error!\n");
			break;
		}

		ret = audio_read_frame(aread_handle, (Int8 *)(pa_inbuf->addr));

		if(ret < 0) {
			printf("audio read frame error!\n");
			break;
		}

		//静音，设置pcm为0
		if(1 == get_mute_status(0)) {
			memset(pa_inbuf->addr, 0, pa_inbuf->buf_size);
		}

		//awrite_file("./pcm_001.pcm", pa_inbuf->addr, pa_inbuf->buf_size);

		ret = audio_indata_put_full_buf(&(audioinst[0].paenchandle->indata_user_handle), pa_inbuf);

		if(ret < 0) {
			printf("audio put full buf error!\n");
			break;
		}

	}

	return NULL;
}
#endif

/*AAC 数据处理*/
static void *AudioBitsDataOutProcess(audio_struct *audioinst)
{
	Int32			ret = 0;
	audio_buf       *pa_outbuf = NULL;

	//Int32 return_code = 0;
	Int32 i = 0;
	udp_send_module_handle *sendhand = (udp_send_module_handle *)(gEduKit->sendhand[0]);


	if(audioinst == NULL || sendhand == NULL) {
		OSA_error("AudioBitsDataOutProcess: audioinst is NULL!\n");
		return NULL;
	}

	printf("[AudioBitsDataOutProcess] user_data:[%p]\n", sendhand->udp_hand.src_data);
	//int32_t index = 0;
	//int32_t audio_mute_count = 0;
	frame_info_t frame_info;

	memset(&frame_info, 0, sizeof(frame_info_t));


	int x = 0;

	while(TRUE) {
		ret = audio_outdata_get_full_buf(&(audioinst[i].paenchandle->outdata_user_handle), &pa_outbuf);

		if(ret < 0) {
			OSA_printf("audio get full buf error!\n");
			break;
		}

		pa_outbuf->sample_rate = 44100;
		pa_outbuf->bit_rate = 128000;
		frame_info.m_hight = pa_outbuf->sample_rate;
		frame_info.m_frame_length = pa_outbuf->fill_length;
		frame_info.time_tick = getCurrentTime_zl();
		//printf("[AudioBitsDataOutProcess] time_tick:[%u]\n", frame_info.time_tick);
		frame_info.is_blue = 0;
		frame_info.m_dw_flags = AVIIF_KEYFRAME;
		frame_info.m_frame_rate = 0;
		frame_info.m_data_codec = AAC_CODEC_TYPE;
		memcpy(sendhand->udp_hand.src_data, pa_outbuf->addr, pa_outbuf->buf_size);

		if(pa_outbuf->buf_size != 4096) {
			UdpSend_rtp_data(sendhand, &frame_info);
		}

#if 0

		if(pa_outbuf->buf_size != 4096 && x < 3000) {
			printf("write audio x=%d\n", x++);
			awrite_file("./xx001.aac", pa_outbuf->addr, pa_outbuf->buf_size);
		}

#endif

		//测试代码，发送encodemange
		if(pa_outbuf->buf_size != 4096) {
			SendAudioToClient110(pa_outbuf->buf_size, pa_outbuf->addr, 1, 2, 16);
		}

		ret = audio_outdata_put_empty_buf(&(audioinst[i].paenchandle->outdata_user_handle), pa_outbuf);

		if(ret < 0) {
			OSA_printf("audio put empty buf error!\n");
			break;
		}
	}
}

Int32 reach_audio_enc_process(EduKitLinkStruct_t *pstruct)
{
	audio_encdec_params		paparams;
	AudioEncodeParam	 	audio_param;
	audio_capture_params	paread_params;
	Int32 ret = 0;
	int i = 0;

	if(NULL == pstruct) {
		fprintf(stderr, "reach_audio_enc_process failed, params error!\n");
		return -1;
	}

	//pthread_mutex_init(&gavolume.mutex, NULL);

	memset(&audio_param, 0, sizeof(AudioEncodeParam));
	audio_param.BitRate = 128000;
	audio_param.Channel = 2;
	audio_param.Codec = 0x53544441;
	audio_param.InputMode = 0;
	audio_param.LVolume = 20;
	audio_param.MicType = 0;
	audio_param.Mute = 0;
	audio_param.RVolume = 20;
	audio_param.SampleBit = 16;
	audio_param.SampleRate = 44100;


	paread_params.bufsize = 4096;
	paread_params.channel = audio_param.Channel;

	strcpy(paread_params.device_name, AUDIONAME[0]);

	paread_params.samplebit = audio_param.SampleBit;

	assert(audio_param.SampleBit == 8 || audio_param.SampleBit == 16);
	paread_params.samplerate = audio_param.SampleRate;
	paread_params.mode = AUDIO_INPUT_MODE;
	paread_params.inputmode = audio_param.InputMode;
	paread_params.mictype = 0;//mic类型

	//create pcm read
	ret = audio_device_create_inst((void *) & (pstruct->audioencLink[0]), &paread_params);

	if(ret < 0) {
		OSA_printf("audio_capture_create_inst failed!\n");
		return -1;
	}

	paparams.inst_type = AAC_ENC;
	paparams.aenc_create_param.encoderType = AUDIO_CODEC_TYPE_AAC_LC;
	paparams.aenc_create_param.sampleRate = audio_param.SampleRate;
	paparams.aenc_create_param.bitRate = audio_param.BitRate;
	paparams.aenc_create_param.numberOfChannels = audio_param.Channel;
	memcpy(&(pstruct->audioencLink[i].paparam), &(paparams), sizeof(audio_encdec_params));


	audio_setCapParamInput(0, audio_param.InputMode); //设置音频输入模式

	//create audio encode
	ret = audio_encdec_create_inst(pstruct->audioencLink, AudioPcmDataInProcess, AudioBitsDataOutProcess, &paparams, NULL);

	if(ret < 0) {
		OSA_error("audio_encdec_create_inst failed!\n");
		return -1;
	}

	setAudioEncodeParam(0, &audio_param);
	adjust_volume(0, 25, 25);
	return 0;
}



int audio_setCapParamInput(int input, int mode)
{
	char cmd[200] = {0};
	int ch = 0;
	memset(cmd, 0, sizeof(cmd));


	if(input == SIGNAL_INPUT_1) {
		ch = 0;
	} else {
		ch = 1;
	}

	if(mode == 1) { //非平衡mic  in
		sprintf(cmd, "amixer cset -c %d numid=36,iface=MIXER,name='AGC Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=93,iface=MIXER,name='Left PGA Mixer Line1L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=94,iface=MIXER,name='Left PGA Mixer Line1R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=95,iface=MIXER,name='Left PGA Mixer Line2L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=87,iface=MIXER,name='Right PGA Mixer Line2R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=96,iface=MIXER,name='Left PGA Mixer Mic3L Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=97,iface=MIXER,name='Left PGA Mixer Mic3R Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=88,iface=MIXER,name='Right PGA Mixer Mic3L Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=89,iface=MIXER,name='Right PGA Mixer Mic3R Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=85,iface=MIXER,name='Right PGA Mixer Line1R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=86,iface=MIXER,name='Right PGA Mixer Line1L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=39,iface=MIXER,name='ADC HPF Cut-off' 1", ch);
		system(cmd);
	} else if(mode == 0) {
		//line in
		sprintf(cmd, "amixer cset -c %d numid=36,iface=MIXER,name='AGC Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=93,iface=MIXER,name='Left PGA Mixer Line1L Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=94,iface=MIXER,name='Left PGA Mixer Line1R Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=95,iface=MIXER,name='Left PGA Mixer Line2L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=87,iface=MIXER,name='Right PGA Mixer Line2R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=96,iface=MIXER,name='Left PGA Mixer Mic3L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=97,iface=MIXER,name='Left PGA Mixer Mic3R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=88,iface=MIXER,name='Right PGA Mixer Mic3L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=89,iface=MIXER,name='Right PGA Mixer Mic3R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=85,iface=MIXER,name='Right PGA Mixer Line1R Switch' 1 ", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=86,iface=MIXER,name='Right PGA Mixer Line1L Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=39,iface=MIXER,name='ADC HPF Cut-off' 0", ch);
		system(cmd);
	} else {
		//平衡mic
		sprintf(cmd, "amixer cset -c %d numid=36,iface=MIXER,name='AGC Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=93,iface=MIXER,name='Left PGA Mixer Line1L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=94,iface=MIXER,name='Left PGA Mixer Line1R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=95,iface=MIXER,name='Left PGA Mixer Line2L Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=87,iface=MIXER,name='Right PGA Mixer Line2R Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=96,iface=MIXER,name='Left PGA Mixer Mic3L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=97,iface=MIXER,name='Left PGA Mixer Mic3R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=88,iface=MIXER,name='Right PGA Mixer Mic3L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=89,iface=MIXER,name='Right PGA Mixer Mic3R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=85,iface=MIXER,name='Right PGA Mixer Line1R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=86,iface=MIXER,name='Right PGA Mixer Line1L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=39,iface=MIXER,name='ADC HPF Cut-off' 1", ch);
		system(cmd);
	}

	return 1;

}


static Int32 adjust_volume(int chId, int LVolume, int RVolume)
{
	int l_value = 0;
	int r_value = 0;

	if(0 == LVolume) {
		l_value = 0;
	} else {
		l_value = 100 + 10 * (LVolume / 3);
	}

	if(0 == RVolume) {
		r_value = 0;
	} else {
		r_value = 100 + 10 * (RVolume / 3);
	}

	printf("num:[%d] LVolume:[%d] RVolume:[%d]\n", chId, LVolume, RVolume);
	int card = chId;

	if(0 == chId) {
		card = 1;
	} else if(1 == chId) {
		card = 0;
	}

	char cmd[256] = {0};
	sprintf(cmd, "amixer -c %d cset numid=5,iface=MIXER,name='PCM Volume' %d ; amixer -c %d cset numid=6,iface=MIXER,name='L ADC VOLUME' %d", card, r_value, card, l_value);
	system(cmd);

	printf("num=%d,card=%d,RVolume=%d,LVolume=%d\n", chId, card, r_value, l_value);
	return 0;
}

static Int32 get_volume(int chId, int *LVolume, int *RVolume)
{
	*LVolume = 100;
	*RVolume = 100;
	return 0;
}

static int g_audio_mute[2] = {0};
int set_mute_status(int chId, int status)
{
	g_audio_mute[chId] = status;
	return 0;
}

int get_mute_status(int chId)
{
	return g_audio_mute[chId];
}

int setAudioEncodeParam(int chId, AudioEncodeParam *ainfo)
{
	AudioCommonInfo cinfo;

	memset(&cinfo, 0, sizeof(cinfo));

	cinfo.input_num = chId;
	cinfo.samplerate = ainfo->SampleRate;
	cinfo.bitrate = ainfo->BitRate;
	cinfo.channel = ainfo->Channel;
	//cinfo.samplebit = ainfo->SampleBit;
	//cinfo.lvolume = ainfo->LVolume;
	//cinfo.rvolume = ainfo->RVolume;
	cinfo.inputmode = ainfo->InputMode;
	//cinfo.mictype = ainfo->MicType;
	cinfo.is_mute = ainfo->Mute;

	audio_setParam(gEduKit->audioencLink, &cinfo);
	audio_setEncParam(gEduKit->audioencLink[chId].paenchandle, ainfo->SampleRate, ainfo->BitRate);
	audio_setCapParamSampleRate(gEduKit->audioencLink[chId].pacaphandle, ainfo->SampleRate);
	//无需设置音量，屏蔽这个接口
	//adjust_volume(chId, cinfo.lvolume, cinfo.rvolume);
	set_mute_status(chId, cinfo.is_mute);

	OSA_printf("audio info set:samplerate=%d,bitrate=%d,channel=%d,samplebit=%d,lv=%d,rv=%d,inputmode=%d,mictype=%d,mute=%d\n", \
	           ainfo->SampleRate, ainfo->BitRate, ainfo->Channel, ainfo->SampleBit, ainfo->LVolume, ainfo->RVolume, ainfo->InputMode, ainfo->MicType, ainfo->Mute);


	return 0;
}

int getAudioEncodeParam(int chId, AudioEncodeParam *ainfo)
{
	AudioCommonInfo cinfo;
	memset(&cinfo, 0, sizeof(AudioCommonInfo));

	cinfo.channel = chId;
	audio_getParam(gEduKit->audioencLink, &cinfo);

	ainfo->SampleRate = cinfo.samplerate;
	ainfo->BitRate = cinfo.bitrate;
	ainfo->Channel = cinfo.channel;
	ainfo->SampleBit = cinfo.samplebit;
	ainfo->LVolume = cinfo.lvolume;
	ainfo->RVolume = cinfo.rvolume;
	ainfo->InputMode = cinfo.inputmode;
	ainfo->MicType = cinfo.mictype;
	ainfo->Mute = cinfo.is_mute;
	OSA_printf("audio info get:samplerate=%d,bitrate=%d,channel=%d,samplebit=%d,lv=%d,rv=%d,inputmode=%d,mictype=%d,mute=%d\n", \
	           ainfo->SampleRate, ainfo->BitRate, ainfo->Channel, ainfo->SampleBit, ainfo->LVolume, ainfo->RVolume, ainfo->InputMode, ainfo->MicType, ainfo->Mute);
	return 0;
}


#if 0
typedef struct _audio_volume_ {
	uint32_t lvolume;
	uint32_t rvolume;
	int8_t InputIndex[5];
	pthread_mutex_t mutex;
} audio_volume;

static Int32 adjust_volume(int chId, int LVolume, int RVolume);

audio_volume gavolume = {0};

int32_t gainput = LINE_INPUT;
//extern int32_t gmax_room_thread;

static int g_audio_update_flag[SIGNAL_INPUT_MAX] = {0, 0};


extern int app_get_have_stream();
extern int app_set_media_info(int width, int height, int sample , int num);
extern void SendAudioToClient(int nLen, unsigned char *pData,
                              int nFlag, unsigned char index, unsigned int samplerate);


int32_t audio_mute_flag = 0;

int32_t get_audio_input_mode()
{
	return gainput;
}

int32_t set_audio_input_mode(int32_t input_mode)
{
	if(input_mode != LINE_INPUT && input_mode != MIC_INPUT) {
		return -1;
	}

	gainput = input_mode;

	return 0;
}

inline int32_t get_audio_index()
{
	if(gainput == MIC_INPUT) {
		return 0;
	} else {
		return 1;
	}

	return 1;
}

int audio_set_update_flag(int input, int flag)
{
	if(input >= 0 && input < SIGNAL_INPUT_MAX) {
		g_audio_update_flag[input] = flag;
		printf("i will set %d input flag =%d\n", input, flag);
	}

	return 0;
}

int audio_get_update_flag(int input)
{
	if(input >= 0 && input < SIGNAL_INPUT_MAX) {
		return g_audio_update_flag[input];
	} else {
		printf("Warnning,input=%d.\n", input);
		return 0;
	}
}


int32_t audio_get_volume(uint32_t *lvolume, uint32_t *rvolume)
{
	if(lvolume == NULL || rvolume == NULL) {
		fprintf(stderr, "audio_get_volume failed, params is NULL!\n");
		return -1;
	}

	if(lvolume != NULL) {
		*lvolume = gavolume.lvolume;
	}

	if(rvolume != NULL) {
		*rvolume = gavolume.rvolume;
	}

	return 0;
}


inline int32_t audio_get_spectrum(int8_t *data, uint32_t length, uint32_t *left_lev, uint32_t *right_lev)
{
	int32_t index = 0;
	int32_t left_count = 0;
	int32_t left_level = 0;
	int32_t right_count = 0;
	int32_t right_level = 0;

	uint32_t aver = 0;
	uint32_t left_allvol = 0;
	int32_t left_aver = 0;
	uint32_t right_allvol = 0;
	int32_t right_aver = 0;

	uint16_t left_vol = 0;
	uint16_t right_vol = 0;

	if((data == NULL) || (left_lev == NULL) | (right_lev == NULL)) {
		fprintf(stderr, "audio_get_spectrum failed, params is NULL!\n");
		return -1;
	}

	if(length < 0) {
		fprintf(stderr, "audio_get_spectrum failed, length error!\n");
		return -1;
	}

	for(index = 0; index < length; index++) {
		left_vol = data[index + 1];
		left_vol = (left_vol << 8) + data[index];

		if(left_vol > 32768) {
			left_vol = 65535 - left_vol;
		}

		left_allvol += (unsigned short)left_vol;
		left_count ++;
		index += 2;
		right_vol = data[index + 1];
		right_vol = (right_vol << 8) + data[index];

		if(right_vol > 32768) {
			right_vol = 65535 - right_vol;
		}

		right_allvol += (unsigned short)right_vol;
		right_count ++;
		index ++;
	}

	left_aver = left_allvol / left_count;
	right_aver = right_allvol / right_count;
	aver = left_aver + right_aver;

#if 1
#if 0

	if((left_aver >= -255 && left_aver <= 255) &&
	   (right_aver >= -255 && right_aver <= 255)) {
		left_level = 1;
	} else {
		left_level = 10;
	}

#endif

	if(aver >= 0 && aver <= 500) {
		left_level = 1;
	} else if(aver > 500 && aver <= 1000) {
		left_level = 5;
	} else if(aver > 1000 && aver <= 1500) {
		left_level = 10;
	} else if(aver > 1500 && aver <= 2000) {
		left_level = 15;
	} else if(aver > 2000 && aver <= 2500) {
		left_level = 20;
	} else if(aver > 2500 && aver <= 3000) {
		left_level = 25;
	} else if(aver > 3000 && aver <= 3500) {
		left_level = 30;
	} else if(aver > 3500 && aver <= 4000) {
		left_level = 35;
	} else if(aver > 4000 && aver <= 4500) {
		left_level = 40;
	} else if(aver > 4500 && aver <= 5000) {
		left_level = 45;
	} else if(aver > 5000 && aver <= 5500) {
		left_level = 50;
	} else if(aver > 5500 && aver <= 6000) {
		left_level = 55;
	} else if(aver > 6000 && aver <= 6500) {
		left_level = 60;
	} else if(aver > 6500 && aver <= 7000) {
		left_level = 65;
	} else if(aver > 7000 && aver <= 7500) {
		left_level = 70;
	} else if(aver > 7500 && aver <= 8000) {
		left_level = 75;
	} else if(aver > 8000 && aver <= 8500) {
		left_level = 80;
	} else if(aver > 8500 && aver <= 9000) {
		left_level = 85;
	} else if(aver > 9000 && aver <= 9500) {
		left_level = 90;
	} else if(aver > 9500 && aver <= 10000) {
		left_level = 95;
	} else {
		left_level = 100;
	}

	//printf("left_level = %d, left_aver = %d, right_aver = %d\n", left_level, left_aver, right_aver);

#else

	if(aver >= 0 && aver <= 2000) {
		left_level = 1;
	} else if(aver > 2000 && aver <= 4000) {
		left_level = 5;
	} else if(aver > 4000 && aver <= 6000) {
		left_level = 10;
	} else if(aver > 6000 && aver <= 8000) {
		left_level = 15;
	} else if(aver > 8000 && aver <= 10000) {
		left_level = 20;
	} else if(aver > 10000 && aver <= 12000) {
		left_level = 25;
	} else if(aver > 12000 && aver <= 14000) {
		left_level = 30;
	} else if(aver > 14000 && aver <= 16000) {
		left_level = 35;
	} else if(aver > 16000 && aver <= 18000) {
		left_level = 40;
	} else if(aver > 18000 && aver <= 20000) {
		left_level = 45;
	} else if(aver > 22000 && aver <= 24000) {
		left_level = 50;
	} else if(aver > 24000 && aver <= 25000) {
		left_level = 55;
	} else if(aver > 25000 && aver <= 26000) {
		left_level = 60;
	} else if(aver > 26000 && aver <= 27000) {
		left_level = 65;
	} else if(aver > 27000 && aver <= 28000) {
		left_level = 70;
	} else if(aver > 28000 && aver <= 29000) {
		left_level = 75;
	} else if(aver > 29000 && aver <= 30000) {
		left_level = 80;
	} else if(aver > 30000 && aver <= 31000) {
		left_level = 85;
	} else if(aver > 31000 && aver <= 32000) {
		left_level = 90;
	} else if(aver > 32000 && aver <= 33000) {
		left_level = 95;
	} else {
		left_level = 100;
	}

#endif

	*left_lev = left_level;
	*right_lev = right_level;

	return 0;
}


int audio_setCapvol(int ch, int mode)
{
	char cmd[200] = {0};
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "amixer cset -c %d numid=36,iface=MIXER,name='AGC Switch' %d", ch, mode);
	system(cmd);
	return 1;
}

static void *AudioPcmDataOutProcess(audio_struct *param)
{
	audio_buf       *pa_inbuf;
	//	Int32			ret = 0;
	Int32			status = -1;
	Void			*awrite_handle = NULL;


	if(param == NULL) {
		fprintf(stderr, "AudioPcmDataOutProcess: param is NULL!\n");
		return NULL;
	}

	OSA_QueHndl *empty_que = &(param->paenchandle->outdata_user_handle.empty_que);
	OSA_QueHndl *full_que = &(param->paenchandle->outdata_user_handle.full_que);

	awrite_handle = (Void *)param->pacaphandle;

	if(awrite_handle == NULL) {
		fprintf(stderr, "AudioPcmDataOutProcess: awrite_handle is NULL!\n");
		return NULL;
	}

	param->paenchandle->outdata_user_handle.run_status = TRUE;

	while(TRUE == param->paenchandle->outdata_user_handle.run_status) {
		status = OSA_queGet(full_que, (Int32 *)(&pa_inbuf), OSA_TIMEOUT_FOREVER);
		OSA_assert(status == 0);

		usleep(3 * 1000);
		//	ret = audio_write_frame(awrite_handle, (Int8 *)(pa_inbuf->addr));
		//	if(ret < 0) {
		//		fprintf(stderr, "audio write frame error!\n");
		//	}

		status = OSA_quePut(empty_que, (Int32)pa_inbuf, OSA_TIMEOUT_NONE);
		OSA_assert(status == 0);
	}

	return (void *)0;
}



Int32 reach_audio_dec_process(EduKitLinkStruct_t *pstruct)
{
	audio_encdec_params		*paparams;
	audio_capture_params	paread_params;

	Int32 ret = -1;

	paparams = &(pstruct->audiodecLink.paparam);
	paparams->inst_type = AAC_DEC;
	paparams->adec_create_param.decoderType = AUDIO_CODEC_TYPE_AAC_LC;
	paparams->adec_create_param.desiredChannelMode = 2;


	paread_params.bufsize = 4096;
	paread_params.channel = 2;

	//strcpy(paread_params.device_name, "default:CARD=mcasp0");
	strcpy(paread_params.device_name, "default:CARD=EVM");

	paread_params.samplebit = 16;
	paread_params.samplerate = 44100;
	paread_params.mode = AUDIO_OUTPUT_MODE;

	ret = audio_device_create_inst((Void **)&pstruct->audiodecLink.pacaphandle, &paread_params);

	if(ret < 0) {
		fprintf(stderr, "audio_device_create_inst failed!\n");
		return -1;
	}

	ret = audio_dec_create_inst(&pstruct->audiodecLink, paparams, NULL, AudioPcmDataOutProcess);

	if(ret < 0) {
		fprintf(stderr, "audio_dec_create_inst failed!\n");
		return -1;
	}

	return 0;
}
#endif

