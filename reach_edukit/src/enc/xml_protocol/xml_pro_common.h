#ifndef	__XML_PRO_COMMON_H__
#define	__XML_PRO_COMMON_H__
#include "log_common.h"

 typedef struct __SYSINFO
 {
	 char deviceType[16];					 //设备类型
	 char strName[16];						 //产品序列号
	 unsigned char szMacAddr[8];			 //MAC 地址
	 unsigned int dwAddr;					 //IP 地址
	 unsigned int dwGateWay;				 //网关
	 unsigned int dwNetMark;				 //子网掩码
	 char strVer[10];						 //编码器版本号
	 unsigned short channelNum; 			 //通道数
	 unsigned short pptIndex;				 //PPT索引标识
	 unsigned short lowRate;
	 unsigned short encodeType;
 }SYSTEMINFO;


 typedef struct __RATE_IFNO {
	 int rateInfoNum;
	 int rateType;	 //码率标志位，0-----高/ 1------低。
	 int nWidth;	 //宽
	 int nHeight;		 //高
	 int nFrame;	 //帧率
	 int nBitrate;	 //带宽
	 int nTemp; 	 //保留
 } RATE_INFO;


 typedef struct __AUDIO_PARAM
 {
	 unsigned short  InputMode; 					  ////1------ MIC input 0------Line Input
	 unsigned int SampleRate;						 //采样率节点号
	 unsigned int BitRate;						 //码率
	 unsigned char	LVolume;						 //左声音	  0 --------31
	 unsigned char	RVolume;						 //右声音	  0---------31
 }AUDIO_PARAM;

 typedef struct __PICTURE_ADJUST
 {
	 short hporch;	   // 左：负值，右：正值
	 short vporch;	   // 上：负值，下：正值
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

