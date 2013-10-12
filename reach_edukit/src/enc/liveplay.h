/*
* Copyright (c) 2010,深圳锐取软件技术有限公司
*
* 文件名称：liveplay.h
* 摘 要：此为使用recplayer(v7.1.2)或actplay(v1.0.3.6)插件或DEC1000连接直播
*		 的服务端代码
*
* 当前版本：1.0
* 作 者：huanghh
* 完成日期：2011年2月15日
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
* 功能说明: 设置支持的最大客户端连接数
*			最大不得大于liveplay.c文件中的宏LIVE_MAX_RECUSER的值
* 参数说明:	---- usernum, 支持的最大客户端连接数
* 返回说明: 成功返回 0， 失败返回-1
*/
int setLiveMaxUserNum(int usernum);


/*  
* 功能说明: 获得支持的最大客户端连接数
* 参数说明:	无
* 返回说明: 返回支持的最大客户端连接数
*/

int getLiveMaxUserNum();


/*  
* 功能说明: 设置调试等级
* 参数说明:	---- DEBUG_LEVEL, 调试等级
* 返回说明: 返回 0
*/
int setLiveDebugLevel(LiveEDebugLevle DEBUG_LEVEL);


/*  
* 功能说明: 直播音频数据的发送
* 参数说明:	---- nLen, 需要发送的帧数据的长度
*			---- pData, 需要发送的帧数据的指针
*			---- nFlag, 需要发送的帧的I帧标志 0--P帧，1--I帧
*			---- index,	通道索引值
*			---- timestmp,	帧时间戳
*			---- pAsys,	音频帧参数
* 返回说明: 成功返回 0， 失败返回-1
*/
int LiveSendAudioDataToRecPlay(int nLen, unsigned char *pData,int nFlag, unsigned char index, unsigned int timestmp, LiveAudioParam *pAsys);


/*  
* 功能说明: 直播视频数据的发送
* 参数说明:	---- nLen, 需要发送的帧数据的长度
*			---- pData, 需要发送的帧数据的指针
*			---- nFlag, 需要发送的帧的I帧标志 0--P帧，1--I帧
*			---- index,	通道索引值
*			---- timestmp,	帧时间戳
*			---- pVsys,	视频帧参数
* 返回说明: 成功返回 0， 失败返回-1
*/
int LiveSendVideoDataToRecPlay(int nLen, unsigned char *pData,	int nFlag, unsigned char index, unsigned int timestmp, LiveVideoParam *pVsys);


/*  
* 功能说明: 开启直播服务
* 参数说明:	无
* 返回说明: 成功返回 0， 失败返回-1
*/
int startLiveplayTask();


/*  
* 功能说明: 关闭直播服务
* 参数说明:	无
* 返回说明: 成功返回 0， 失败返回-1
*/
/*
int stopLiveplayTask();
*/

/*  
* 功能说明: 获取库版本号
* 参数说明:	无参数
* 返回说明: 当前库版本号
*/
char *getLivePlayLibVersion();

#endif

