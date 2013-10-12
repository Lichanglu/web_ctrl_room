/*
* Copyright (c) 2010,������ȡ����������޹�˾
*
* �ļ����ƣ�liveplay.h
* ժ Ҫ����Ϊʹ��recplayer(v7.1.2)��actplay(v1.0.3.6)�����DEC1000����ֱ��
*		 �ķ���˴���
*
* ��ǰ�汾��1.0
* �� �ߣ�huanghh
* ������ڣ�2011��2��15��
*
*/

#ifndef __LIVEPLAY_H_
#define __LIVEPLAY_H_ 


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


typedef int LIVE_SOCKET;
typedef unsigned short LIVE_WORD ;
typedef unsigned int   LIVE_DWORD ;
typedef unsigned char  LIVE_BYTE;


/*video param*/
typedef struct _LIVE_VIDEOPARAM
{
    	LIVE_DWORD nDataCodec;   					//Encode Mode
												//video: mmioFOURCC('H','2','6','4');
	LIVE_DWORD nFrameRate;   					//vdieo: FrameRate
	LIVE_DWORD nWidth;       						//video: width
	LIVE_DWORD nHight;       						//video: height
	LIVE_DWORD nColors;      						//video: colors
	LIVE_DWORD nQuality;							//video: qaulity
	LIVE_DWORD picturequality;					//video: pictureqaulity (for record)
	LIVE_WORD 	sCbr;							//video: Qaulity/BitRate
	LIVE_WORD 	sBitrate;							//video: bitrate
}LiveVideoParam;

/*audio param*/
typedef struct _LIVE_AUDIOPARAM
{
	LIVE_DWORD Codec;  							//Encode Type
    	LIVE_DWORD SampleRate; 						//SampleRate
	LIVE_DWORD BitRate;  						//BitRate
	LIVE_DWORD Channel;  						//channel numbers
	LIVE_DWORD SampleBit;  						//SampleBit
	LIVE_BYTE  	LVolume;						//left volume       0 --------31
	LIVE_BYTE  	RVolume;						//right volume      0---------31
	LIVE_WORD  	InputMode;                       //1------ MIC input 0------Line Input
}LiveAudioParam;

#if 1
typedef enum{
	DL_NONE, //=0
	DL_ERROR,  //=1
	DL_WARNING,  //=2
	DL_FLOW,	//=3
	DL_DEBUG,	//=4
	DL_ALL, 	//all
}LiveEDebugLevle;
#endif


/*  
* ����˵��: ����֧�ֵ����ͻ���������
*			��󲻵ô���liveplay.c�ļ��еĺ�LIVE_MAX_RECUSER��ֵ
* ����˵��:	---- usernum, ֧�ֵ����ͻ���������
* ����˵��: �ɹ����� 0�� ʧ�ܷ���-1
*/
int setLiveMaxUserNum(int usernum);


/*  
* ����˵��: ���֧�ֵ����ͻ���������
* ����˵��:	��
* ����˵��: ����֧�ֵ����ͻ���������
*/

int getLiveMaxUserNum();


/*  
* ����˵��: ���õ��Եȼ�
* ����˵��:	---- DEBUG_LEVEL, ���Եȼ�
* ����˵��: ���� 0
*/
int setLiveDebugLevel(LiveEDebugLevle DEBUG_LEVEL);


/*  
* ����˵��: ֱ����Ƶ���ݵķ���
* ����˵��:	---- nLen, ��Ҫ���͵�֡���ݵĳ���
*			---- pData, ��Ҫ���͵�֡���ݵ�ָ��
*			---- nFlag, ��Ҫ���͵�֡��I֡��־ 0--P֡��1--I֡
*			---- index,	ͨ������ֵ
*			---- timestmp,	֡ʱ���
*			---- pAsys,	��Ƶ֡����
* ����˵��: �ɹ����� 0�� ʧ�ܷ���-1
*/
int LiveSendAudioDataToRecPlay(int nLen, unsigned char *pData,int nFlag, unsigned char index, unsigned int timestmp, LiveAudioParam *pAsys);


/*  
* ����˵��: ֱ����Ƶ���ݵķ���
* ����˵��:	---- nLen, ��Ҫ���͵�֡���ݵĳ���
*			---- pData, ��Ҫ���͵�֡���ݵ�ָ��
*			---- nFlag, ��Ҫ���͵�֡��I֡��־ 0--P֡��1--I֡
*			---- index,	ͨ������ֵ
*			---- timestmp,	֡ʱ���
*			---- pVsys,	��Ƶ֡����
* ����˵��: �ɹ����� 0�� ʧ�ܷ���-1
*/
int LiveSendVideoDataToRecPlay(int nLen, unsigned char *pData,	int nFlag, unsigned char index, unsigned int timestmp, LiveVideoParam *pVsys);


/*  
* ����˵��: ����ֱ������
* ����˵��:	��
* ����˵��: �ɹ����� 0�� ʧ�ܷ���-1
*/
int startLiveplayTask();


/*  
* ����˵��: �ر�ֱ������
* ����˵��:	��
* ����˵��: �ɹ����� 0�� ʧ�ܷ���-1
*/
/*
int stopLiveplayTask();
*/

/*  
* ����˵��: ��ȡ��汾��
* ����˵��:	�޲���
* ����˵��: ��ǰ��汾��
*/
char *getLivePlayLibVersion();

#endif

