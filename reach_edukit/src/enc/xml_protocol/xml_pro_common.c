
#include "audio.h"
#include "video.h"
#include "capture.h"
#include "xml_pro_common.h"

#ifndef XML_CHECK_INPUT
#define XML_CHECK_INPUT(index) {PRINTF("--input=%d--\n",index);\
		if(index < 0 || index > 2){\
			printf("input =%d\n",index);\
			return -1;}\
	}
#endif

#define	DEVICE_TYPE			"EDUKIT-SD"
#define	BASE_ENC_SERIALNO	"0000000"
#define	BASE_VERSION		"0.1"
static void get_enc_serialno(char *seriano)
{
	strcpy(seriano, BASE_ENC_SERIALNO);
}

int get_system_info(int index, SYSTEMINFO *info)
{
	XML_CHECK_INPUT(index)
	char seriano[32] = {0};


	get_enc_serialno(seriano);

	strcpy(info->deviceType, DEVICE_TYPE);
	memcpy(info->strName, seriano, strlen(seriano));
	memcpy(info->szMacAddr, "0000000", 8);
	info->dwAddr = 0;
	info->dwGateWay = 0;
	info->dwNetMark = 0;


	snprintf(info->strVer, sizeof(info->strVer), "%s", BASE_VERSION);
	info->channelNum = index;

	if(1 == index) {
		info->encodeType = EncodeType_JPEG;
	} else {
		info->encodeType = EncodeType_H264;
	}

	return 0;
}

int get_enc_channel(int index)
{
	if(0 == index) {
		return 0;
	}
}

/*Force I frame */
int setXmlForceIframe(int index)
{
	XML_CHECK_INPUT(index);
	int channel = get_enc_channel(index);
	app_video_request_iframe(channel);
	return 0;
}


void getXmlVideoParams(int index, RATE_INFO *high_rate)
{
	int channel = get_enc_channel(index);
	memset(high_rate, 0, sizeof(RATE_INFO));
	VideoEncodeParam vinfo;
	getVideoEncodeParam(channel, &vinfo);
	high_rate->rateType = 0;
	high_rate->nWidth = vinfo.uWidth;
	high_rate->nHeight = vinfo.uHeight;
	high_rate->nFrame = vinfo.nFrameRate;
	high_rate->nBitrate = vinfo.sBitrate;
}

static int SampleBitToIndex(int SampleBit)
{
	if(44100 == SampleBit) {
		return 2;
	}

	return 3;
}

void getXmlAudioParams(int index, AUDIO_PARAM  *audio_info)
{
	int channel = get_enc_channel(index);
	memset(audio_info, 0, sizeof(AUDIO_PARAM));
	AudioEncodeParam ainfo;
	getAudioEncodeParam(channel, &ainfo);
	audio_info->InputMode = 0;
	audio_info->BitRate = ainfo.BitRate;
	audio_info->LVolume = 0;//ainfo.LVolume;
	audio_info->RVolume = 0;//ainfo.RVolume;
	audio_info->SampleRate = SampleBitToIndex(ainfo.SampleRate);
}

void setXmlVideoParams(int index, RATE_INFO *high_rate)
{
	XML_CHECK_INPUT(index);
	int channel = get_enc_channel(index);
	VideoEncodeParam vinfo;
	memset(&vinfo, 0, sizeof(VideoEncodeParam));
	vinfo.sBitrate = high_rate->nBitrate;
	vinfo.nFrameRate = high_rate->nFrame;
	setVideoEncodeParam(channel, &vinfo);
}

static int  SampleIndexToBit(int index)
{
	uint32_t ret = 44100;

	switch(index) {
		case 0:   //8KHz
		case 1:  //32KHz
		case 2:
			ret = 44100;
			break;

		case 3:
			ret = 48000;
			break;

		default:
			ret = 48000;
			break;
	}

	return (ret);
}


void setXmlAudioParams(int index, AUDIO_PARAM  *audio_info)
{
	XML_CHECK_INPUT(index);
	int channel = get_enc_channel(index);
	AudioEncodeParam ainfo;
	memset(&ainfo, 0, sizeof(AudioEncodeParam));
	ainfo.Channel = 0;
	ainfo.Mute = 0;
	ainfo.MicType = 0;
	ainfo.InputMode = 0;
	ainfo.BitRate = 128000;//audio_info->BitRate;
	ainfo.SampleRate = 44100;//SampleIndexToBit(audio_info->SampleRate);
	setAudioEncodeParam(channel, &ainfo);
}

int getXmlMuteStatus(int index)
{
	XML_CHECK_INPUT(index);
	int channel = get_enc_channel(index);
	return get_mute_status(channel);
}

void setXmlMuteStatus(int index, int status)
{
	XML_CHECK_INPUT(index);
	int channel = get_enc_channel(index);
	set_mute_status(channel, status);
}

int  setXmlVgaHV(int index, int h, int v)
{
	return capture_set_vga_hv(h, v);
}

int getXmlVideoDetect(int index)
{
	XML_CHECK_INPUT(index);
	int channel = get_enc_channel(index);

	if(0 == channel) {
		return getVGADetect();
	}

	return 0;
}

int  read_client_num(int index)
{
	return 1;
}

void GetDeviceType(char *dtype)
{
	strcpy(dtype, DEVICE_TYPE);
}


