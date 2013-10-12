#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "reach.h"
#include "reach_audio.h"

typedef struct _AUDIOEncodePARAM_{
	unsigned int 	Codec;  							//Encode Type
	unsigned int 	SampleRate; 						//SampleRate 44.1 48
	unsigned int 	BitRate;  						//BitRate  128000
	unsigned int 	Channel;  						//channel numbers
	unsigned int 	SampleBit;  						//SampleBit  16
	unsigned char  	LVolume;					//left volume       0 --------31
	unsigned char  	RVolume;					//right volume      0---------31
	unsigned short  InputMode;             // 
	unsigned int	MicType;               //
	unsigned int 	Mute;                    //mute 0 not  1 mute
}AudioEncodeParam;


Int32 reach_audio_enc_process(EduKitLinkStruct_t *pstruct);
Int32 reach_audio_dec_process(EduKitLinkStruct_t *pstruct);
int setAudioEncodeParam(int chId, AudioEncodeParam *ainfo);
int getAudioEncodeParam(int chId, AudioEncodeParam *ainfo);
int get_mute_status(int chId);
int set_mute_status(int chId, int status);

int audio_setCapParamInput(int input, int mode);
#endif

