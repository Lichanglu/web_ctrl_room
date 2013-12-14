#if 1

/*****************************************************************************************
*  用来放置通信的接口，后续WEB,TCP_SDK，或者预计的串口中控，都是通过改接口返回。
*
*
*
*
*******************************************************************************************/
#ifndef _MID_CONTROL_H__
#define _MID_CONTROL_H__

#if 0
enum
{
	VP0 = 0,
	VP1 = 1,
	ALL_VP,
	MAX_VP,
};
#endif

//判断命令字段
#define 	WEB_IDENTIFIER 			0xaaaaaaaa
#define 	TCP_SDK_IDENTIFIER  0xaaaaaaab

#define INPUT_1     2
#define INPUT_2     1
#define XML_TEXT_MAX_LENGTH 4096

#define ENCODESERVER_1260_PORT   30113
#define SDK_LISTEN_PORT		4000
#define	ENCODESERVER_PORT		41110

typedef enum __webparsetype__ {
    INT_TYPE = 0,
    STRING_TYPE,
    STRUCT_TYPE,
} webparsetype;

typedef struct __webMsgInfo_1260 {
	int 			identifier;  //0xaaaaaaaa 标识一个有效数据
	int				len;      //总长
	webparsetype	type;
} webMsgInfo_1260;


//##############################################结构体##end####################################################

//##############################################结构体##end####################################################


//##############################################结构体##end####################################################
//#############################板卡信息####################################

//CARD_1命令 cmd = old_cmd | INPUT_CARD_1
//CARD_1命令 cmd = old_cmd | INPUT_CARD_2

//channel num (cmd>>4)&0xf
//cmd = cmd | (((channel+1)&0xf)<<16) ;


//############信号状态########################
#define	MSG_GETINPUTSIGNALINFO 			0x1112		//获取信号分辨率信息
#define	MSG_SIGNALDETAILINFO			0x1114	//获取信号详细信息
#define	MSG_REVISE_PICTURE				0x1115  //HV值调整
#define	MSG_GETINPUTTYPE				0x111D	 //获取DVI/SDI
#define	MSG_SETINPUTTYPE			    0x1121   //设置DVI/SDI
#define MSG_GETCOLORSPACE				0x1136   //得到颜色空间
#define MSG_SETCOLORSPACE				0x1137   //设置颜色空间
#define MSG_GETHDCPVALUE				0x113F  //获取信号HDCP状态
#define	MSG_GETSIGNALINFO				0x222F  //信号源信息
#define	MSG_GET_ENC_INFO				0x223F  //编码器信息
//###########视频编码信息######################
#define	MSG_GETOUTPUTVIDEOINFO			0x1117	//获取编码输出参数
#define	MSG_SETOUTPUTVIDEOINFO			0x1118	//设置编码输出参数
#define	MSG_SETH264ENCODEPARAM			0x1119  //设置264的高级编码参数设置
#define	MSG_GETVIDEOPARAM				0x111B	 //获取视频参数
#define	MSG_SETVIDEOPARAM				0x111F   //设置视频参数
#define MSG_GETH264ENCODEPARAM 			0x113D  //获取H264高级编码

#define MSG_SETSCALEINFO  				0X2025				//zl
#define MSG_GETSCALEINFO  				0X2026				//zl

#define CONTROL_ROOM_SET_PREVIEW_PASS     	(0X9000)	  //ZL
#define ROOM_CONTROL_GET_PREVIEW_PASS		(0X9001)	  //ZL

#define MSG_GET_REC_INFO					0X2027				//ZL


#define MSG_GETVIDEOENCODEINFO		0X1602
#define MSG_SETVIDEOENCODEINFO		0X1603
#define MSG_GETENABLETEXTINFO		0X1604
#define MSG_SETENABLETEXTINFO		0X1605


//###########音频信息##########################
#define	MSG_GETAUDIOPARAM 				0x111A	 //获取音频参数
#define	MSG_SETAUDIOPARAM				0x111E   //设置音频参数
#define MSG_SETMUTE						0x1150  //设置音频静音
#define MSG_GETMUTE						0x1153 //获取音频静音

//###########logo ,字幕########################
#define MSG_UPLOADIMG      				0x1139	//上传logo图片
#define MSG_GET_LOGOINFO  				0x113A	//获取logo图片信息
#define MSG_SET_LOGOINFO  				0x113B	//设置logo图片信息
#define MSG_GET_TEXTINFO  				0x1154 //获取字幕状态
#define MSG_SET_TEXTINFO  				0x1155 //设置字幕状态
#define MSG_GET_MAXPOS                 		 0x1156 //获取设置字幕的最大xy坐标


//############系统信息#########################
#define MSG_GETSOFTCONFIGTIME 			0x1110  //获取软件编译时间
#define	MSG_GETCPULOAD  				0x1111	//获取CPU占用率

#define 	MSG_GET_NETWORK					0X111B	//获取网络参数
#define 	MSG_SET_NETWORK					0x111a//设置网络参数
#define	MSG_GETSYSPARAM					0x111C	 //获取系统参数
#define	MSG_SETSYSPARAM					0x1120   //设置系统参数
#define	MSG_UPDATESYS					0x112B  //设置系统升级
#define	MSG_REBOOTSYS					0x112C  //设置系统重启

#define	MSG_REBOOTSYS_F					0xFF2C  //设置系统重启

#define	MSG_GETSERIALNO					0x112A  //获取串号


//############远遥协议############################
#define	MSG_FAR_CTRL						0x1126   //远遥控制
#define MSG_GET_CTRL_PROTO				0x1127 //获取远遥协议
#define	MSG_SET_CTRL_PROTO				0x1128 //设置远遥协议








//###############input begin############################










//###############input end############################


#define	MSG_GETSTREAMPARAM				0x1122
#define	MSG_SETSTREAMPARAM				0x1123
#define	MSG_GETRTSPPARAM				0x1124
#define	MSG_SETRTSPPARAM				0x1125

#define	MSG_SYNCTIME					0x112D
#define	MSG_SAVEPARAM					0x112E
#define	MSG_START_MULT					0x112F
#define	MSG_STOP_MULT					0x1130
//#define	MSG_GET_PROTOCOL				0x1131
#define	MSG_SETDEVICETYPE				0x1132
#define	MSG_GETDEVICETYPE				0x1133
#define MSG_BLUE_REVISE					0x1134
#define MSG_RESTORESET					0x1135







#define MSG_GETENCODETIME				0x1156
#define MSG_SDK_LOGIN 						0x1157
#define MSG_GETSOFTVERSION                     0x1158


// zyb
//#define MSG_SET_SIGNAL_DETECTION			0x2023


/*stream output */
#define MSG_STREAM_GET_PROTOCOL 0x1200
/*rtsp*/
#define MSG_RTSP_GET_USED      0X1201  		/*rtsp is used*/
#define MSG_RTSP_GET_GINFO    0x1202  		/*rtsp get server global info*/
#define MSG_RTSP_SET_GINFO     0X1203 		/*rtsp set server global info*/
#define MSG_RTSP_GET_CINFO    0x1204 		 /*rtsp get common info*/
#define MSG_RTSP_SET_CINFO    0x1205  		/*rtsp set common info*/
#define MSG_RTSP_SET_STATUS   0x1206           /*rtsp set start,stop,pause status*/
#define MSG_RTSP_ADD_SERVER   0x1208          /*add rtsp server*/
#define MSG_RTSP_DEL_SERVER    0x1209        /*del rtsp server*/
#define MSG_GET_SDP_INFO       0x120a
//#define MSG_RTSP_SET_STATUS  0x1310

/*multcast */
#define MSG_MULT_GET_NUM      0X1301  		/*get mult server total num*/
#define MSG_MULT_GET_CINFO    0x1304 		 /*mult get common info*/
#define MSG_MULT_SET_CINFO    0x1305  		/*mult set common info*/
#define MSG_MULT_SET_STATUS   0x1306           /*mult set start,stop,pause status*/
#define MSG_MULT_ADD_SERVER   0x1308          /*add mult server*/
#define MSG_MULT_DEL_SERVER    0x1309        /*del mult server*/
#define MSG_MULT_GET_TS_RATE   0x1310           /*get mult  ts rate*/
#define MSG_MULT_GET_RTP_RATE 0x1311 		/*get mult rtp rate*/
#define MSG_MULT_GET_RTPTS_RATE 0x1312 		/*get mult rtpts rate*/
//#define MSG_MULT_SET_STATUS  0x1310


/*******先临时添加****************/
//#define MSG_SETSHOWTEXTLOGO   0x9901
#define MSG_GET_BUILD_TIME 0x9901

#define MSG_CHANNEL_NUM 0x9902



//add by lichl
#define MSG_SET_SWMS_INFO				0x1f00
#define MSG_GET_SWMS_INFO				0x1f01
#define MSG_SET_HDMI_RES				0X1f04
#define MSG_GET_HDMI_RES				0x1f05

#define WEB_SET_TRACER_INFO				0x1f06
#define WEB_GET_TRACER_INFO				0x1f07

#define WEB_GET_RAMOTE_CTRL			0x1f08
#define WEB_SET_RAMOTE_CTRL			0x1f09



#define MSG_TO_ROOM_VODPLAY_CTRL	(0X8024)



//##############################################命令字段##end####################################################

//##############################################错误码##begin####################################################

#define 				CLIENT_RETSUCCESSVALUE 				0
#define 				CLIENT_ERR_UNKNOWCMD					0xf1
#define					CLIENT_ERR_PARAMLEN						0xf2
#define 				CLIENT_ERR_TCPCONNECT					0xf3
#define   			CLIENT_ERR_TCPSEND						0xf4
#define 				CLIENT_ERR_TCPRECV						0xf5
#define 				CLIENT_ERR_PARAMSVALUE				0xf6
#define  				CLIENT_ERR_USERNAME						0xf7
#define 				CLIENT_ERR_PASSWORD						0xf8


#define    			SERVER_RET_OK 								0X00
#define        		SERVER_RET_UNKOWN_CMD 						0xf01
#define        		SERVER_RET_INVAID_PARM_LEN  				0xf02               //编码器通信结构体长度不一致，需要查对应通信的头文件
#define        		SERVER_RET_INVAID_PARM_VALUE  				0xf03
#define        		SERVER_RET_SOCK_MAX_NUM  					0xf04
#define        		SERVER_RET_RECV_FAILED  					0xf05
#define        		SERVER_RET_SEND_FAILED  					0xf06
#define        		SERVER_RET_ID_ERROR  						0xf07
#define       		SERVER_RET_USER_INVALIED  					0xf08
#define 			SERVER_INTERNAL_ERROR						0xf09                //编码器内部错误，出现该错误，必须马上查原因
#define             SERVER_VERIFYFILE_FAILED				    0XF0A
#define             SERVER_SYSUPGRADE_FAILED				    0XF0B
#define             SERVER_SYSROLLBACK_FAILED					0xF0C
#define             SERVER_GETDEVINFO_FAILED       				0xF0D
#define             SERVER_HAVERECORD_FAILED					0xF0E


//##############################################错误码##end####################################################

#define  webMsgInfo webMsgInfo_1260
#define MSGINFOHEAD			sizeof(webMsgInfo_1260)

typedef struct REMOTE_CTRL_INFO{
	int tea_procotol;
	int tea_addr;
	int stu_procotol;
	int stu_addr;
}Remote_Ctrl_t;

typedef enum __OutputResolution_1 {
    LOCK_AUTO = 0,
	LOCK_CUSTOM = 1,
    LOCK_1920x1080P = 2,
    LOCK_1280x720P = 3,
    LOCK_1400x1050_VGA = 4,
    LOCK_1366x768_VGA = 5,
    LOCK_1280x1024_VGA = 6,
    LOCK_1280x768_VGA = 7,
    LOCK_1024x768_VGA = 8,
    LOCK_720x480_D1 = 9,
    LOCK_352x288_D1 = 10,
    LOCK_MAX
} OutputResolution_1;

typedef struct _SYSTEM_INFO_{
	char serial_no[64];
	char type_no[64];
	char web_version[16];
	char ctrl_version[16];
	char ctrl_built_time[64];
	char hd_version[32];
	char hd_ker_version[32];
	char hd_FPGA_version[32];
	char hd_built_time[64];
	int total_space;
	int available_space;
	char ip[20];
}System_Info_t;

typedef struct move_mode_Info_{
	int model;
	int res;
}Moive_Info_t;

typedef struct _SCALE_INFO_{
	int resolution;  //OutputResolution
	int scalemode;   //o: ratio 1:full
	int outwidth;    //custom width
	int outheight;    //custom heigh
	int	sign_flag_1;
	int	sign_flag_2;
}WebScaleInfo;

typedef struct WEB_AUDIOEncodePARAM_{
	unsigned int 	Codec;  							//Encode Type
	unsigned int 	SampleRateIndex; 						//SampleRate 44.1 48
	unsigned int 	BitRate;  						//BitRate  128000
	unsigned int 	Channel;  						//channel numbers
	unsigned int 	SampleBit;  						//SampleBit  16
	unsigned char  	LVolume;					//left volume       0 --------31
	unsigned char  	RVolume;					//right volume      0---------31
	unsigned short  InputMode;             // 需要在上面做支持，区分平衡与非平衡MAC
	unsigned int	mp_input;               // 合成输出号1  or 2
	unsigned int 	Mute;                    //mute 0 not  1 mute
	unsigned int    type;                    //line in  or Mic
}WEB_AudioEncodeParam;



typedef struct _PANEL_REC_INFO_{

	int rec_total_time;
	char rec_file_path[256];

}PanelRecInfo;


typedef struct _VIDEO_ENCODE_INFO_{
//	int 					enable;        //maybe not need
//	SceneConfiguration   	preset ;			//场景配置选项。
	unsigned int 		 	nFrameRate;   	//视频帧率
	unsigned int			IFrameinterval; 	//I 帧间隔
	unsigned int 			sBitrate;			//视频码率
	unsigned int 			sCbr;
	unsigned int 			nQuality;
}WebVideoEncodeInfo;

typedef struct _ENABLE_TEXT_INFO_
{
	int text_show;
	int logo_show;
}WebEnableTextInfo;


typedef enum {
    LOCK_SCALE = 1,
    UNLOCK_SCALE = 2
} LOCK_SCALE_STATUS;

#define VIDEO_QUALITY 0
#define VIDEO_SBITRATE 1

typedef struct _RTP_SDP_INFO_T{
	int channel;
	char ip[16];
	int vport;
	int aport;
	char buff[1024];
}RTP_SDP_INFO;


typedef struct __SYSPARAMS {
	unsigned char szMacAddr[8];				//MAC address
	unsigned int dwAddr;					//IP address
	unsigned int dwGateWay;					//getway
	unsigned int dwNetMark;					//sub network mask
	char strName[16];						//encode box name
	char strVer[10];						//Encode box version
	unsigned short unChannel;							//channel numbers
	char bType; 							//flag  0 -------VGABOX     3-------200 4-------110 5-------120 6--------1200
	char bTemp[3]; 							//bTemp[0] ------1260
	int nTemp[7];							//reserve  nTemp[0] 表示DHCP FLAG
} SYSPARAMS;


typedef struct _DATE_TIME_INFO {
	int year;
	int month;
	int mday;
	int hours;
	int min;
	int sec;
} DATE_TIME_INFO;

typedef enum __inputtype {
    VGA = 0,
    YCBCR,
    DVI,
    SDI,
    AUTO
} inputtype;

typedef enum __ENUM_PROTOCOL__ {
    OUTPUT_TS_STREAM = 0,
    OUTPUT_RTP_STREAM,
    OUTPUT_RTSP_STREAM,
    OUTPUT_RTMP_STREAM,
    OUTPUT_MAX_PROTOCOL
} Enum_Protocol;


//色彩空间转换
typedef struct pic_revise {
	short hporch;    //col
	short vporch;    //row
	int temp[4];
} pic_revise ;

typedef enum _POS_TYPE_T{
    ABSOLUTE2= 0,
    TOP_LEFT ,
    TOP_RIGHT ,
    BOTTOM_LEFT ,
    BOTTOM_RIGHT ,
    CENTERED
} POS_TYPE;

#if 1
typedef struct __textinfo {
	int xpos;
	int ypos;
	int width;
	int height;
	int showtime;
	int enable;
	int alpha;
	int show;
	int postype;
	char msgtext[128];
} TextInfo;
#endif
typedef enum __colorspace {
    RGB = 0,
    YUV,
} ColorSpace;
//	输入信号源的信息。
typedef struct InputSignalInfo {
	int  mode;			//信号源分辨率信息。
	ColorSpace  spaceFlag;			//彩色空间标志。
	unsigned int 	hdcpFlag;			//开启HDCP版权保护标志。
	unsigned int		reservation;		//保留。
} InputSignalInfo;

typedef enum __SceneConfiguration {
    movies = 0,
    motionless,
    motion,
    custom,
} SceneConfiguration;

#define JEPG_FILE_NEMA  ("/opt/dvr_rdk/ti816x_2.8/logo_temp.jpg")
typedef struct LogoInfo {
	int x;	//显示位置坐标,左上角为0,0
	int y;
	int width;	//logo图片宽高
	int height;
	int enable;	//是否显示logo	,显示:1,不显示:0
	char filename[32];//图片名称15个字符,默认为logo.png
	int alpha;//透明度[0,100]
	int isThrough;//是否透过某一像素值,实现镂空的效果,透过1,不透2
	int filter;//如果isThrough为1，可以设置一个要透的值，一般为0x00或0xFF
	int errcode;//错误码为0表示没有错误，1表示无效的logo文件
	int channel;	//保留 -> 通道
	int postype; // pos type
} LogoInfo;

/*protocol struct*/
typedef struct __PROTOCOL__ {
	/*protocol count*/
	unsigned char count ;
	/*protocol open status  1 -- open */
	/*0--- close all */
	unsigned int status: 1 ;
	/*TS stream*/
	unsigned int TS: 1;
	/*RTP*/
	unsigned int RTP: 1;
	/*RTSP*/
	unsigned int RTSP: 1;
	/*RTMP*/
	unsigned int RTMP: 1;
	/*reserve*/
	unsigned char temp[7];
} Protocol;

/*mult addr*/
typedef struct __MULT_ADDR_PORT {
	char chIP[16];
	unsigned short nPort;
	short nStatus;
} MultAddr;

//输出视频信号的参数设置
typedef struct OutputVideoInfo {
	int resolution;		//设置输出分辨率。
	unsigned int 		 resizeMode;		//缩放模式选择。0：比例拉伸 1：全屏拉伸
	unsigned int			 encodelevel;		//编码级别。0：high profile   1：baseline profile
	SceneConfiguration   preset ;			//场景配置选项。
	unsigned int 		 nFrameRate;   	//视频帧率
	unsigned int			 IFrameinterval; 	//I 帧间隔
	unsigned int 			 sBitrate;			//视频码率。
	int					logo_show;
	int 					text_show;
	//unsigned int			logotext;		//是否显示logo/text。
} OutputVideoInfo;


//H264编码参数设置
typedef struct H264EncodeParam {
	unsigned int		param1;
	unsigned int		param2;
	unsigned int		param3;
	unsigned int		param4;
	unsigned int		param5;
	unsigned int		reservation;		//保留。

} H264EncodeParam;
//媒体流协议参数设置
typedef struct StreamParam {
	Enum_Protocol   	protocol;			//协议选项。
	char				ip[16];			//ip
	unsigned int			port;				//端口号。
	unsigned int			MTUParam;		//MTU值。
	unsigned int			TTLParam;		//TTL值。
	unsigned int			TOSParam;		//TOS值。
	unsigned int			shapFlag;			//流量整型标志。
	unsigned int			actualBandwidth;	//实际带宽。
	unsigned int			reservation;		//保留。
} StreamParam;

typedef enum __StreamProtocol {
    TSoverUDP,
    TSoverRTP,
    DirectRTP,
    QuickTime,
} StreamProtocol;
//rtsp参数设置
typedef struct RtspParam {
	unsigned int 		keepAtctive;
	unsigned int			MTUParam;				//MTU值。
	StreamProtocol	protocol;					//rtsp流传输协议。
	char				boardCastip[16];			//组播ip;
	unsigned int			boardCastPort;				//组播端口号;
	unsigned int			reservation;				//保留。
} RtspParam;
/*当前缩放状态*/
typedef struct __RESIZE_PARAM__ {
	unsigned int dbstatus;//是否处于导播状态
	unsigned int flag ;//是否处于锁定分辨率状态
	unsigned int scalemode ; // 模式
	int black;//填黑边
	int cap_w;			//source width
	int cap_h;			//source height
	int dst_w;			//dest  width
	int dst_h;			//dest height
	int x;
	int y;
	int nFrame;		//帧率
	int nBitrate;   //带宽
	int nTemp;		//保留
} ResizeParam;

typedef struct SignalInfo_ {
	char Signal[32];
	int HPV;
	int TMDS;
	int VsyncF;
	int HDCP;
	int RGB_YPRPR;
} SignalInfo_t;

typedef struct EncInfo_ {
	char fpga_version[8];
	char kernel_version[16];
} EncInfo_t;

#endif
#endif
