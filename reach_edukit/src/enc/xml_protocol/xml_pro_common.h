#ifndef	__XML_PRO_COMMON_H__
#define	__XML_PRO_COMMON_H__
#include "log_common.h"

 typedef struct __SYSINFO
 {
	 char deviceType[16];					 //�豸����
	 char strName[16];						 //��Ʒ���к�
	 unsigned char szMacAddr[8];			 //MAC ��ַ
	 unsigned int dwAddr;					 //IP ��ַ
	 unsigned int dwGateWay;				 //����
	 unsigned int dwNetMark;				 //��������
	 char strVer[10];						 //�������汾��
	 unsigned short channelNum; 			 //ͨ����
	 unsigned short pptIndex;				 //PPT������ʶ
	 unsigned short lowRate;
	 unsigned short encodeType;
 }SYSTEMINFO;


 typedef struct __RATE_IFNO {
	 int rateInfoNum;
	 int rateType;	 //���ʱ�־λ��0-----��/ 1------�͡�
	 int nWidth;	 //��
	 int nHeight;		 //��
	 int nFrame;	 //֡��
	 int nBitrate;	 //����
	 int nTemp; 	 //����
 } RATE_INFO;


 typedef struct __AUDIO_PARAM
 {
	 unsigned short  InputMode; 					  ////1------ MIC input 0------Line Input
	 unsigned int SampleRate;						 //�����ʽڵ��
	 unsigned int BitRate;						 //����
	 unsigned char	LVolume;						 //������	  0 --------31
	 unsigned char	RVolume;						 //������	  0---------31
 }AUDIO_PARAM;

 typedef struct __PICTURE_ADJUST
 {
	 short hporch;	   // �󣺸�ֵ���ң���ֵ
	 short vporch;	   // �ϣ���ֵ���£���ֵ
	 short color_trans;
	 int   temp[4];
 }PICTURE_ADJUST;

#ifdef SUPPORT_IP_MATRIX
 typedef struct __LOCK_PARAM
 {
	 int video_lock;
	 int audio_lock;
	 int resulotion_lock;
 }LOCK_PARAM;
#endif

enum {
	EncodeType_H264 = 0,
	EncodeType_JPEG,
	EncodeType_INVALID = -1,
};

void getXmlVideoParams(int index, RATE_INFO *high_rate);
void getXmlAudioParams(int index, AUDIO_PARAM  *audio_info);
void setXmlVideoParams(int index, RATE_INFO *high_rate);
void setXmlAudioParams(int index, AUDIO_PARAM  *audio_info);
int  setXmlHV(int index, int h, int v);
int getXmlVideoDetect(int index);
#endif	//__XML_PRO_COMMON_H__

