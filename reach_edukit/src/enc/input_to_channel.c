/********************************************************************
*  专门用来封装channel ,input,select link chid,enclink chid 的对应关系
*
*
*
*
*********************************************************************/

#include <stdio.h>
#include "input_to_channel.h"
/*
*		根据实际的代码,4路channel对应的enclink里面的input chid的顺序可能不一致，
*		所以封装找个函数
*/

static int g_enc_chid[MAX_CHANNEL] = {1, 0, 3, 2};   // zhengyb
//static int g_enc_chid[MAX_CHANNEL] = {0,1}  // zhengyb

//form CHANNEL_INPUT_N to enclik chid
int channel_get_enc_chid(int channel)
{
	if(channel < 0 || channel > MAX_CHANNEL) {
		fprintf(stderr, "channel =%d is invaild\n", channel);
		return -1;
	}

	return g_enc_chid[channel];
}

//from enclink chid to channel_input_N
int enc_chid_get_channel(int enc_chid)
{
	int i = 0;

	for(i = 0 ; i < MAX_CHANNEL; i++) {
		if(enc_chid == g_enc_chid[i]) {
			return i;
		}
	}

	fprintf(stderr, "enc_chid=%d is invaid\n", enc_chid);
	return -1;
}
int channel_set_enc_chid(int channel, int enc_chid)
{

	if(channel < 0 || channel > MAX_CHANNEL) {
		fprintf(stderr, "channel =%d is invaild\n", channel);
		return -1;
	}

	g_enc_chid[channel] = enc_chid;
	return 0;
}


//传入channel ,获取vp 以及high low
int channel_get_input_info(int channel, int *input , int *high)
{
	if(channel == CHANNEL_INPUT_1) {
		*input = SIGNAL_INPUT_1;
		*high = HIGH_STREAM;
	} else if(channel == CHANNEL_INPUT_1_LOW) {
		*input = SIGNAL_INPUT_1;
		*high = LOW_STREAM;
#if 1   //  zhengyb
	} else if(channel == CHANNEL_INPUT_2) {
		*input = SIGNAL_INPUT_2;
		*high = HIGH_STREAM;
	} else if(channel == CHANNEL_INPUT_2_LOW) {
		*input = SIGNAL_INPUT_1;
		*high = LOW_STREAM;
	}

#endif
	return 0;
}


//mergeLink 1输出后，ch对应的最原始的几个信号输入
static int g_input1_mer1_chid = 0;
static int g_input2_mer1_chid = 1;

int input_set_mer1_chid(int input, int chid)
{
	if(input == SIGNAL_INPUT_1) {
		g_input1_mer1_chid = chid;
	} else if(input == SIGNAL_INPUT_2) {
		g_input2_mer1_chid = chid;
	} else {
		fprintf(stderr, "input =%d is invaild\n", input);
		return -1;
	}

	fprintf(stderr, "i will set input %d to mer1 link chid =%d\n", input, chid);
	return 0;
}

/*input get mer1 link chid */
int input_get_mer1_chid(int input)
{
	if(input == SIGNAL_INPUT_1) {
		return g_input1_mer1_chid;
	} else if(input == SIGNAL_INPUT_2) {
		return g_input2_mer1_chid;
	} else {
		fprintf(stderr, "input =%d is invaild\n", input);
		return -1;
	}
}

/*channel get mer1 link chid */
int channel_get_mer1_chid(int channel)
{
	int input = -1;
	int high = -1;
	channel_get_input_info(channel, &input , &high);

	if(input < SIGNAL_INPUT_1 || high != HIGH_STREAM) {
		fprintf(stderr, "input =%d,high=%d is not support.\n", input, high);
		return -1;
	}

	return input_get_mer1_chid(input);

}




