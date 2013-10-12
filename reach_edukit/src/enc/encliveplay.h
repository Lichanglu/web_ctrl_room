/*********************************************************************
公司名称:	Reach Software
文件名:
创建人:	Huang Haihong				日期:	2011-06-20
修改人:	Huang Haihong				日期:	2011-06-20

说明:
**********************************************************************/


#ifndef	_ENCLIVEPLAY_H_
#define	_ENCLIVEPLAY_H_




#define PORT_LISTEN_DSP1						3100

//#define HEAD_LEN							3
#define HEAD_LEN			sizeof(MSGHEAD)

#define BOX_VER								"DM8168"


#define MIC_INPUT					1
#define LINE_INPUT					0

#define ADTS 						0x53544441
#define AAC							0x43414130
#define PCM							0x4d435030




#define DSP_NUM								4
#define DSP1								0
#define DSP2								1

#define LOGIN_USER							0
#define LOGIN_ADMIN							1

#define PORT_LISTEN_DSP1					3100
#define PORT_UDP_SERVER_LISTEN				11115


typedef int SOCKET;
typedef int DSPFD;
typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef unsigned char  BYTE;



#define TRUE								1
#define FALSE								0


#define KERNELFILENAME						"uImage"
#define FPGAFILENAME						"fpga.bin"



#define INVALID_SOCKET						-1

#define  MSG_MAXCLIENT_ERR					5

#define MAX_CLIENT							6
#define MAX_PACKET							14600

#define TEXT_BUFTYPE_NUMTYPES				2

#define AVIIF_KEYFRAME						0x00000010


#define  MSG_ADDCLIENT						1
#define  MSG_DELETECLIENT					2
#define  MSG_CONNECTSUCC					3
#define  MSG_PASSWORD_ERR					4
#define  MSG_MAXCLIENT_ERR					5
#define  MSG_AUDIODATA						6
#define  MSG_SCREENDATA					7
#define  MSG_HEARTBEAT						8
#define  MSG_PASSWORD						9
#define  MSG_DATAINFO						10
#define  MSG_REQ_I							11
#define  MSG_SET_FRAMERATE				12
#define  MSG_PPT_INDEX						15

#define MSG_SYSPARAMS						16
#define MSG_SETPARAMS						17
#define MSG_RESTARTSYS						18

#define MSG_UpdataFile						19
#define MSG_SAVEPARAMS					20
#define MSG_UpdataFile_FAILS					21
#define MSG_UpdataFile_SUCC					22

#define MSG_DECODE_STATUS					23
#define MSG_DECODE_DISCONNECT			24
#define MSG_DECODE_CONNECT				25

#define MSG_UPDATEFILE_DECODE				26
#define MSG_UPDATEFILE_ROOT				27
#define MSG_UPDATEFILE_WEB				28

#define MSG_MODE_CODE						29
#define MSG_MODE_DECODE					30

#define MSG_ADD_TEXT						33

#define MSG_MOUT							40
#define MSG_SENDFLAG						41
#define MSG_FARCTRL						42
#define MSG_FARCTRLEX						47

#define MSG_TRACKAUTO				48 //设置自动跟踪模块的自动手动模式
#define MSG_SETVGAADJUST					43//边框调整

#define MSG_QUALITYVALUE					46 //设置高清编码器码流模式(0:低 1:中 2:高)

#define MSG_FARCTRLEX      			47
#define MSG_TRACKAUTO				48 //设置自动跟踪模块的自动手动模式
#define MSG_UPLOADIMG 			    49//上传logo图片
#define MSG_GET_LOGOINFO 			50//获取logo图片信息
#define MSG_SET_LOGOINFO 			51//设置logo图片信息

#define	MSG_SET_TRACK_TYPE			110	//老师跟踪机和学生跟踪机通信的消息类型

#define MSG_TEACHER_HEART         111  //教师机心跳
/**
* @ 发送当前帧课堂信息
*/
#define MSG_SEND_CLASSINFO		112

#define MSG_GET_VIDEOPARAM				0x70
#define MSG_SET_VIDEOPARAM				0x71
#define MSG_GET_AUDIOPARAM				0x72
#define MSG_SET_AUDIOPARAM				0x73
#define MSG_REQ_AUDIO						0x74
#define MSG_CHG_PRODUCT					0x75

#define MSG_SET_SYSTIME					0x77
#define MSG_SET_DEFPARAM					0x79

#define MSG_SET_PICPARAM					0x90
#define MSG_GET_PICPARAM					0x91


#define MSG_CHANGE_INPUT					0x92
#define MSG_SEND_INPUT						0x93

#define MSG_PIC_DATA						0x94
#define MSG_LOW_SCREENDATA				0x95//低码率数据
#define MSG_LOW_BITRATE					0x96 //低码率
#define MSG_MUTE							0x97
#define MSG_LOCK_SCREEN					0x99//锁定屏幕
#define MSG_GSCREEN_CHECK					0x9a//绿屏校正
#define MSG_FIX_RESOLUTION				0x9e//导播锁定分辨率


#define	MSG_SET_TEACHER_TRACK_PARAM	0xA0	//教师跟踪参数设置
#define	MSG_SET_STUDENTS_TRACK_PARAM	0xA1	//教师跟踪参数设置

//-------------------------------------------组播----------------------------------------------------------------------------------------------------------------------
#define MSG_MUL_START						0x80            //开启组播 HDB_MSGHEAD
#define MSG_MUL_STOP						0x81            //停止组播 HDB_MSGHEAD
#define MSG_MUL_VIDEOSIZE					0x82            //设置组播视频尺寸，HDB_MSGHEAD +flag(BYTE) flag: 0：原始尺寸，1：缩小
#define MSG_MUL_ADDR_PORT				0x83            //设置组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_STREAM					0x84            //设置组播流，HDB_MSGHEAD +flag(BYTE) flag: 0：所有流，1：音频，2：视频
#define MSG_MUL_STATE						0x85            // 获取组播状态，HDB_MSGHEAD+MulAddr+sizeflag（BYTE）+streamFlag（BYTE）+state(BYTE)(开启/停止).
#define MSG_MUL_SET_AUDIO_ADDR			0x86            //设置音频组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_SET_VIDEO_ADDR			0x87            //设置视频组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_GET_AUDIO_ADDR			0x88            //获取音频组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_GET_VIDEO_ADDR			0x89            //获取视频组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_GET_ADDR					0x90            //获取组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_SET_FRAMEBITRATE			0x91
#define MSG_MUL_GET_FRAMEBITRATE			0x92
#define MSG_HEART							0x93			//心跳

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------



/*if use client*/
#define ISUSED(dsp,cli)							(gDSPCliPara[dsp].cliDATA[cli].bUsed == TRUE)

/*set client used*/
#define SETCLIUSED(dsp,cli,val)					(gDSPCliPara[dsp].cliDATA[cli].bUsed=val)

/*if login client*/
#define ISLOGIN(dsp,cli)							(gDSPCliPara[dsp].cliDATA[cli].bLogIn == TRUE)

/*set client login succeed or failed*/
#define SETCLILOGIN(dsp,cli,val)					(gDSPCliPara[dsp].cliDATA[cli].bLogIn=val)

/*get socket fd*/
#define GETSOCK(dsp,cli)							(gDSPCliPara[dsp].cliDATA[cli].sSocket)

/*set socket fd*/
#define SETSOCK(dsp,cli,val)						(gDSPCliPara[dsp].cliDATA[cli].sSocket=val)

/*current socket if valid*/
#define ISSOCK(dsp,cli)							(gDSPCliPara[dsp].cliDATA[cli].sSocket != INVALID_SOCKET)
#define SETLOWSTREAM(dsp,cli,val)					(gDSPCliPara[dsp].cliDATA[cli].LowStream = val)
#define GETLOWSTREAM(dsp,cli)					(gDSPCliPara[dsp].cliDATA[cli].LowStream)
#define SETLOWSTREAMFLAG(dsp,cli,val)			(gDSPCliPara[dsp].cliDATA[cli].LowStreamFlag = val)
#define GETLOWSTREAMFLAG(dsp,cli)				(gDSPCliPara[dsp].cliDATA[cli].LowStreamFlag)

#define SETPPTINDEX(dsp,cli,val)					(gDSPCliPara[dsp].cliDATA[cli].PPTenable = val)

#define GETPPTINDEX(dsp,cli)						(gDSPCliPara[dsp].cliDATA[cli].PPTenable)

#define SETPPTQUERY(dsp,cli,val)					(gDSPCliPara[dsp].cliDATA[cli].PPTquery = val)

#define GETPPTQUERY(dsp,cli)						(gDSPCliPara[dsp].cliDATA[cli].PPTquery)

/*get dsp handle*/
#define GETDSPFD(dsp)							(gDSPCliPara[dsp].dspFD)

/*set dsp handle*/
#define SETDSPFD(dsp,val)							(gDSPCliPara[dsp].dspFD=val)

/*if handle valid*/
#define ISDSPFD(dsp)								(gDSPCliPara[dsp].dspFD != INVALID_FD)

/*client login type*/
#define SETLOGINTYPE(dsp,cli,val)					(gDSPCliPara[dsp].cliDATA[cli].nLogInType=val)

/*get login type*/
#define GETLOGINTYPE(dsp,cli)						(gDSPCliPara[dsp].cliDATA[cli].nLogInType)


#define FPGA_UPDATE							0x55AA
#define FPGA_NO_UPDATE							0xAA55

/*FPGA update Flag*/
extern int g_FPGA_update;


#define IIS_FPGA_UPDATE()						(FPGA_UPDATE == g_FPGA_update)
#define SET_FPGA_UPDATE(update)					(g_FPGA_update = update)

#define ZOOMIN									(0xAAAAAA55)
#define ZOOMOUT									(0xAA55AA55)
#define RESIZE_INVALID							(-1)

#define ISZOOMOUT								(ZOOMOUT == gResizeP.flag)//
#define ISZOOMIN		(ZOOMIN == gResizeP.flag)//放大
#define NORESIZE		(RESIZE_INVALID == gResizeP.flag)
#define INIT_RESIZE_PARAM	{RESIZE_INVALID,RECREATE_INVALID,5,0,0,800,600,800,600};

typedef struct __HDB_SIZETYPE
{
	int nflag; // =0 原始大小 =1 缩小到指定尺寸
	int width;
	int height;
}SizeType;

/*client infomation*/
typedef struct _ClientData
{
	int bUsed;
	int sSocket;
	int bLogIn;
	int nLogInType;
	unsigned char PPTenable;
	unsigned char LowStream;
	unsigned char LowStreamFlag;
	unsigned char PPTquery;
}ClientData;

/*DSP client param*/
typedef struct __DSPCliParam
{
	int dspFD;     	//DSP handle
	ClientData cliDATA[MAX_CLIENT]; //client param infomation
}DSPCliParam;






typedef struct _CTime {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
}CTime;

/*录制信息*/
typedef struct _RecDataInfo {
	char  IpAddr[16];		//录制数据的iP地址
	int   bConnected;		//是否已连接
	int   nDataCodec;		//编码方式 JPEG为 0x1001
	int   nFrameRate;		//数据帧率
	int   nWidth;			//数据的录制宽度
	int   nHight;			//高度
	int   nColors;			//颜色数
	int   nOthers;			//保留
	char  Version[16];		//版本信息
}RecDataInfo;

typedef struct _WhoIsMsg
{
	unsigned char	szFlag[4];	//must 0x7e7e7e7e
	unsigned char	szMacAddr[8];
	unsigned int	dwAddr;
	unsigned int	dwGateWay;
	unsigned int	dwNetMask;
	char			strName[16];
	int				nFrame;
	int				nColors;
	int				nQuality;
	char			strVer[10];
	int				nType;			//6 ---- ENC1200
}WhoIsMsg;

/*message header*/
typedef struct __HDB_MSGHEAD
{
	WORD	nLen;
	WORD	nVer;				//version
	BYTE	nMsg;				//message type
	BYTE	szTemp[3];			//reserve
}MSGHEAD;

/*音频和视频网络发送数据头*/
typedef struct __HDB_FRAME_HEAD {
	DWORD ID;								//=mmioFOURCC('4','D','S','P');
	DWORD nTimeTick;    					//时间戳
	DWORD nFrameLength; 					//帧长度
	DWORD nDataCodec;   					//编码类型
	//编码方式
	//视频 :mmioFOURCC('H','2','6','4');
	//音频 :mmioFOURCC('A','D','T','S');
	DWORD nFrameRate;   					//视频  :帧率
	//音频 :采样率 (default:44100)
	DWORD nWidth;       					//视频  :宽
	//音频 :通道数 (default:2)
	DWORD nHight;       					//视频  :高
	//音频 :采样位 (default:16)
	DWORD nColors;      					//视频  :颜色数  (default: 24)
	//音频 :音频码率 (default:64000)
	DWORD dwSegment;						//分包标志位
	DWORD dwFlags;							//视频:  I 帧标志
	//音频:  保留
	DWORD dwPacketNumber; 					//包序号
	DWORD nOthers;      					//保留
} FRAMEHEAD;


typedef struct _low_bitrate
{
	int nWidth;		//宽
	int nHeight;		//高
	int nFrame;		//帧率
	int nBitrate;		//带宽
	int nTemp;		//保留
}low_bitrate;

/*音频参数表*/
typedef struct __AUDIOPARAM {
	DWORD Codec;  							//音频编码ID
	DWORD SampleRate; 						//采样率
	DWORD BitRate;  						//码率
	DWORD Channel;  						//通道号
	DWORD SampleBit;  						//采样位
	BYTE  LVolume;							//左声音     0 --------31
	BYTE  RVolume;							//右声音     0---------31
	WORD  InputMode;                       ////1------ MIC input 0------Line Input
} AudioParam;

/*系统参数表*/
typedef struct __SYSPARAMS {
	unsigned char szMacAddr[8];				//MAC 地址
	unsigned int dwAddr;					//IP 地址
	unsigned int dwGateWay;					//网关
	unsigned int dwNetMark;					//子网掩码
	char strName[16];						//产品序列号
	char strVer[10];						//编码器版本号
	WORD unChannel;							//通道数
	char bType; 							//区分编码器类型  0 -------VGABOX     3-------200 4-------110 5-------120 6--------1200  8---ENC-1100
	char bTemp[3]; 							//保留
	int nTemp[7];							//保留
} SYSPARAMS;


/*视频参数表*/
typedef struct __VIDEOPARAM {
	DWORD nDataCodec;   					//编码方式
	//视频: mmioFOURCC('H','2','6','4');
	DWORD nFrameRate;   					//视频::帧率
	DWORD nWidth;       					//视频::宽
	DWORD nHight;       					//视频: 高
	DWORD nColors;      					//视频: 颜色
	DWORD nQuality;							//视频: 质量
	WORD sCbr;								//视频: 定质量/定码率
	WORD sBitrate;							//视频: 码率
} VideoParam;

typedef struct _DataHeader
{
    DWORD   id;
	long    biWidth;							//VGA：Width		Audio：SampleRate
	long    biHeight;							//VGA：Height	Audio：BitRate
	WORD    biBitCount;							//VGA：BitCount	Audio：Channel
	DWORD   fccCompressedHandler;				//VGA：mmioFOURCC (‘H’,’2’,’6’,’4’)
											    //Audio：mmioFOURCC(‘A’,’D’,’T’,’S’)
	DWORD   fccHandler;	   						//Audio：SampleBit
    DWORD   dwCompressedDataLength;
    DWORD   dwFlags;
    WORD    dwSegment;
	long    dwPacketNumber;
}DataHeader;


typedef struct _SysParamsV2 {
	unsigned char szMacAddr[8];
	unsigned int dwAddr;
	unsigned int dwGateWay;
	unsigned int dwNetMark;
	char strName[16];
	int nFrame;
	int nColors;
	int nQuality;
	char strVer[10];
	char strAdminPassword[16];
	char strUserPassword[16];
	short sLeftRight;
	short sUpDown;
	////// ver 2.0//////////////////
	short sCbr;  //
	short sBitrate; //
	short sCodeType;//  0---RCH0     0f -----H264
	char  strCodeVer[8];    //
	BYTE bVmode;		//视频模式    0 ------- 4CIF   1 ------CIF   2------QCIF
	BYTE bType;
	char sTemp[26];
	//	int nTemp[7];
	////// ver 1.0
} SysParamsV2;


extern void GetAudioParam(int socket,int dsp,unsigned char *data,int len);

extern int SetAudioParam(int dsp,unsigned char *data,int len);

extern int SetSysParams(SYSPARAMS *pold, SYSPARAMS *pnew);

extern int StartEncoderMangerServer();

//extern void SendAudioToClient(int nLen, unsigned char *pData,
  //                     int nFlag, const unsigned char index, AudioParam *paparam);

extern void SendAudioToClient(int nLen, unsigned char *pData,
                       int nFlag, unsigned char index, unsigned int samplerate);

extern void SendDataToClient(int nLen, unsigned char *pData, int nFlag, unsigned char index, int width, int height);
extern int WriteData(int s, void *pBuf, int nSize);

void SendAudioToClient110(int nLen, unsigned char *pData,
                       int nFlag, unsigned char index, unsigned int samplerate);

void SendDataToClient110(int nLen, unsigned char *pData,
                         int nFlag, unsigned char index , int width, int height);
#endif

