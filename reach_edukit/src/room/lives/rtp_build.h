#ifndef _RTP_BUILD_H__
#define _RTP_BUILD_H__
/*
**************************************************************************************
函数介绍:
1.void RtpInit()
    功能介绍:
        该函数主要完成RTP 各项参数初始化
            a.初始化锁
            b.分配音频打包所需的内存空间
2.void RtpSetUdp(int UdpVideoSocket, struct sockaddr_in UdpVideoAddr,int UdpAudioSocket, struct sockaddr_in UdpAudioAddr)
    功能介绍:
        该函数设置RTP组播socket和地址，重建时间戳 
            a.设置视频组播的socket和地址
            b.设置音频组播的socket和地址
            c.重建时间戳
    参数说明:
        UdpVideoSocket:视频组播socket
        UdpVideoAddr:视频组播地址
        UdpAudioSocket:音频组播socket
        UdpAudioAddr:音频组播地址
3.RtpVideoPack(int nLen, unsigned char *pData, int nFlag, unsigned char index)
    功能介绍:
        该函数主要完成视频的rtp打包
    参数说明:
        nLen:要打包的264视频长度
        pData:要打包的264视频地址
        nFlag:帧标志，=1 I帧；=0 P帧
4.RtpAudioPack(int nLen, unsigned char * pData, int nFlag, unsigned char index)
    功能介绍:
        该函数主要完成音频的rtp打包
    参数说明:
        nLen:要打包的aac视频长度
        pData:要打包的aac视频地址
5.void RtpExit()
    功能介绍:
        退出 RTP 协议
            a.锁销毁
            b.释放分配的内存 
6.char *RtpGetVersion()
    功能介绍:
        获取当前rtp 版本号
**************************************************************************************
*/
	
// 帧头信息
typedef struct __HDB_FRAME_HEAD 
{   
	
	unsigned char codec:4;          //编码类型  0--H264  1--ADTS  2-JPEG             
	unsigned char samplerate:4;   //采样率    编码器为ADTS （高4位为  0--16kbps 1--32kbps 2--44.1kbps 3--48kbps 4--96kbps）     
	unsigned char framerate;        //帧率            
	unsigned short width;           //实际宽             
	unsigned short height;           //实际高
//	unsigned short resever;				// 保留位
	unsigned char Iframe;				// I 帧标志
	unsigned char reserve;				// 保留位 
	unsigned int  framelength;			// 帧长
}FRAMEHEAD;

//debug 测试扩展头
typedef struct rtp_debug_head
{
	unsigned short 	profile_defined;
	unsigned short 	length;
}rtp_debug_head_t;




//  扩展头信息
typedef struct rtp_extension_head
{
	unsigned short 	profile_defined;
	unsigned short 	length;
	FRAMEHEAD 		frame_head;
	
}rtp_extension_head_t;


typedef struct _RTP_BUILD_INFO_
{
	unsigned int time_org;
	unsigned int seq_num;
	unsigned int ssrc;
	unsigned int payload;
}RTP_BUILD_INFO;

typedef RTP_BUILD_INFO* RTP_BUILD_HANDLE;

int rtp_build_jpeg_data(RTP_BUILD_HANDLE handle,int nLen, unsigned char *pData,unsigned int ts_current ,int rtp_mtu, void *info ,FRAMEHEAD *data_info);
int rtp_build_audio_data(RTP_BUILD_HANDLE handle,int nLen, unsigned char *pData,int samplerate,int mtu,unsigned int nowtime,void *info,FRAMEHEAD *data_info);
int rtp_build_video_data(RTP_BUILD_HANDLE handle,int nLen, unsigned char *pData, int nFlag,int mtu,unsigned int nowtime,void *info,FRAMEHEAD *data_info);
void *rtp_build_init(unsigned int ssrc, unsigned int payload);
void rtp_build_uninit(void **handle);
int rtp_build_reset(RTP_BUILD_HANDLE handle);

int rtp_build_reset_time(RTP_BUILD_HANDLE handle);


#endif

