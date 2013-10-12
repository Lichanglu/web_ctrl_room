#ifndef _INPUT_TO_CHANNEL_H__
#define _INPUT_TO_CHANNEL_H__

#define INPUT_DVI 0
#define INPUT_SDI 1

typedef enum _INPUT_
{
	SIGNAL_INPUT_1 = 0,
	SIGNAL_INPUT_2 =1,
	SIGNAL_INPUT_MP =2, //MP 1 and 2
	SIGNAL_INPUT_MAX
} SIGNAL_INPUT;

typedef enum{
	SWITCH_STATUS = 0,
	IS_IND_STATUS = 1,  			//independent status
	IS_MP_STATUS = 2,
	MAX_STATUS,
}Mp_Status;


/*
*  CHANNEL_INPUT_1 :					input 1 +  high stream
*  CHANNEL_INPUT_1_LOW :			input 1 + low stream
*  CHANNEL_INPUT_2:					input 2 +  high stream
*  CHANNEL_INPUT_2_LOW:			input 2 +  low stream
*  CHANNEL_INPUT_MP:				mp input 1 and 2 +high stream
*  CHANNEL_INPUT_MP_LOW:                   mp input 1 and 2 + low stream
*
*/
typedef enum _CHANNEL_{
	CHANNEL_INPUT_1 = 0, 
	CHANNEL_INPUT_1_LOW,
	CHANNEL_INPUT_2,
	CHANNEL_INPUT_2_LOW,
//CHANNEL_IND_MAX,    //默认分离的最大数 
	CHANNEL_INPUT_MP ,       //MP 1 and 2
	CHANNEL_INPUT_MP_LOW, //MP 1 and 2
	CHANNEL_INPUT_MAX,
}_CHANNEL_;

#define MAX_CHANNEL CHANNEL_INPUT_MAX 

#define STREAM_MAX_CHANNEL 	(CHANNEL_INPUT_2_LOW+1)
#define NO_MP_MAX_CHANNEL   	(CHANNEL_INPUT_2_LOW+1)

#define ASSERT_INPUT(input)     do{\
									if(input<0 || input>= SIGNAL_INPUT_MAX){\
										ERR_PRN("Error,input=%d\n",input);\
										return -1;\
									}\
								}while(0)

#define ASSERT_CHANNEL(channel)   do{\
								if(channel<0 || channel>= CHANNEL_INPUT_MAX){\
										ERR_PRN("Error,channel=%d\n",channel);\
									}\
								}while(0)
								
enum{
	LOW_STREAM = 0,
	HIGH_STREAM = 1,
	MAX_STREAM ,
};


extern int check_input_valid(int input);
int channel_get_enc_chid(int channel);
int enc_chid_get_channel(int enc_chid);
int channel_set_enc_chid(int channel, int enc_chid);
int channel_get_input_info(int channel, int *input , int *high);
int input_set_mer1_chid(int input, int chid);
int input_get_mer1_chid(int input);
int channel_get_mer1_chid(int channel);
int input_set_remote(int input, int index);
int input_get_remote(int input);
int input_get_audio_input(int ,int*);
int input_set_audio_input(int );

int get_mp_input(int* input);

extern int get_mp_status();



#endif
