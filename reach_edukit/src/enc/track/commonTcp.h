#ifndef _COMMON_TCP_H_
#define  _COMMON_TCP_H_
/*****************************************************/
//此文件为教师机学生机内部通信定义  不要包含在.H文件中
/*****************************************************/



#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h> 
#include <sys/types.h>
#include <arpa/inet.h> 
#include <errno.h>


typedef int SOCKET;
typedef int DSPFD;
typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef unsigned char  BYTE;



#define H264_HEADER_LEN 		0x40
#define H264_IFRAME_SKIPNUM 	0x1E

/*
#############LCD order  start#################
*/
#define FLAG_CLR			0x01
#define FLAG_RESET			0x02
#define FLAG_WAYLEFT		0x05
#define FLAG_WAYRIGHT		0x06
#define FLAG_HIDECURSOR		0x0C
#define FLAG_SHOWCURSOR		0x0F
#define FLAG_MOVELEFT		0x10
#define FLAG_MOVERIGHT		0x14
#define FLAG_SCREENLEFT		0x18
#define FLAG_SCREENRIGHT	0x1C
#define FLAG_SETCURSOR		0x80
#define FLAG_LIGHTSHOW		0x28
#define FLAG_LIGHTHIDE		0x2C

#define FLAG_UP				0xB8
#define FLAG_DOWN			0xE8
#define FLAG_ENTER			0xD8
#define FLAG_ESCAPE			0x78

#define FLAG_NULL			0x1
#define FLAG_CENTER			0x2
#define FLAG_LEFT			0x4
#define FLAG_RIGHT			0x8
#define FLAG_NEWLINE		0x10

#define ONE_LINE_POS		0x00
#define TWO_LINE_POS		0x40

#define MAX_DISPLAY_CHAR	16

/*
#############LCD order  end#################
*/


#define ADTS 					0x53544441
#define AAC						0x43414130
#define PCM						0x4d435030

#define MIC_INPUT				1
#define LINE_INPUT				0

#define MAX_CLIENT				6
#define MAX_PACKET				14600

#define MAX_FAR_CTRL_NUM		5

#define LOGIN_USER				0
#define LOGIN_ADMIN				1

#define INVALID_SOCKET 			-1

#define  MSG_ADDCLIENT      	1
#define  MSG_DELETECLIENT   	2
#define  MSG_CONNECTSUCC    	3
#define  MSG_PASSWORD_ERR   	4
#define  MSG_MAXCLIENT_ERR  	5
#define  MSG_AUDIODATA			6
#define  MSG_SCREENDATA     	7
#define  MSG_HEARTBEAT      	8
#define  MSG_PASSWORD       	9
#define  MSG_DATAINFO       	10
#define  MSG_REQ_I          	11
#define  MSG_SET_FRAMERATE  	12
#define  MSG_PPT_INDEX  		15

#define MSG_SYSPARAMS			16
#define MSG_SETPARAMS			17
#define MSG_RESTARTSYS			18

#define MSG_UpdataFile			19
#define MSG_SAVEPARAMS			20
#define MSG_UpdataFile_FAILS		21
#define MSG_UpdataFile_SUCC			22

#define MSG_DECODE_STATUS			23
#define MSG_DECODE_DISCONNECT		24
#define MSG_DECODE_CONNECT			25

#define MSG_UPDATEFILE_DECODE 		26
#define MSG_UPDATEFILE_ROOT 		27
#define MSG_UPDATEFILE_WEB 			28

#define MSG_MODE_CODE				29
#define MSG_MODE_DECODE				30

#define MSG_ADD_TEXT				33

#define MSG_MOUT          			40
#define MSG_SENDFLAG    			41
#define MSG_FARCTRL      			42

#define MSG_VGA_ADJUST				43

#define MSG_QUALITYVALUE			46 //设置高清编码器码流模式(0:低 1:中 2:高) 
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
#define MSG_SET_MPMODE		    113	


//与web通讯相关信息
#define WEBMSG_RECORD_STATE 0x0001 //录制状态
#define WEBMSG_RECORD_START 0x0002 //开始录制
#define WEBMSG_RECORD_END 0x0003 //结束录制
#define WEBMSG_RECORD_SENDCLASSINFO 0x0004 //上报课堂信息
#define WEBMSG_RECORD_GETMAXSOCKET 0x0005 //同时连接的socket太多


#define MSG_GET_VIDEOPARAM			0x70
#define MSG_SET_VIDEOPARAM			0x71
#define MSG_GET_AUDIOPARAM			0x72
#define MSG_SET_AUDIOPARAM			0x73
#define MSG_REQ_AUDIO				0x74
#define MSG_CHG_PRODUCT				0x75

#define MSG_SET_SYSTIME				0x77
#define MSG_SET_DEFPARAM			0x79

#define MSG_SET_PICPARAM			0x90
#define MSG_GET_PICPARAM			0x91


#define MSG_CHANGE_INPUT			0x92
#define MSG_LOW_BITRATE 			0x96 //低码率
#define MSG_MUTE        			0x97
#define MSG_LOCK_SCREEN    			0x99//锁定屏幕 enc110，enc120，enc1100，enc1200

#define MSG_SEND_INPUT				0x93
#define MSG_CAMERACTRL_PROTOCOL     0x9b   //摄像头控制协议修改 消息头后跟一个字节，0：visca，1：p-d，2：p-p

#define MSG_LOCK_FARCTRL            0x9c //请求独占远遥，消息头+1/0(BYTE),1:表示独占，0：表示取消独占。
#define MSG_FARCTRL_LOCKED 			0x9d //远遥已被锁定

#define MSG_FIX_RESOLUTION 			0x9e//导播锁定分辨率


//-------------------------------------------组播----------------------------------------------------------------------------------------------------------------------
#define MSG_MUL_START		        0x80            //开启组播 HDB_MSGHEAD
#define MSG_MUL_STOP		        0x81            //停止组播 HDB_MSGHEAD
#define MSG_MUL_VIDEOSIZE           0x82            //设置组播视频尺寸，HDB_MSGHEAD +flag(BYTE) flag: 0：原始尺寸，1：缩小
#define MSG_MUL_ADDR_PORT           0x83            //设置组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_STREAM	            0x84            //设置组播流，HDB_MSGHEAD +flag(BYTE) flag: 0：所有流，1：音频，2：视频
#define MSG_MUL_STATE               0x85            // 获取组播状态，HDB_MSGHEAD+MulAddr+sizeflag（BYTE）+streamFlag（BYTE）+state(BYTE)(开启/停止).
#define MSG_MUL_SET_AUDIO_ADDR      0x86            //设置音频组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_SET_VIDEO_ADDR      0x87            //设置视频组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_GET_AUDIO_ADDR      0x88            //获取音频组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_GET_VIDEO_ADDR      0x89            //获取视频组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_GET_ADDR            0x90            //获取组播地址和端口, HDB_MSGHEAD+MulAddr
#define MSG_MUL_SET_FRAMEBITRATE	0x91
#define MSG_MUL_GET_FRAMEBITRATE	0x92
#define MSG_HEART					0x93			//心跳
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//与web通讯相关信息
#define RECORD_STATE 0xa0 //录制状态 开启录制反馈
#define RECORD_START 0xa1 //开始录制
#define RECORD_END 0xa2 //结束录制  
#define RECORD_SENDCLASSINFO 0xa3 //上报课堂信息 停止录制反馈
#define RECORD_GETMAXSOCKET 0xa4 //同时连接的socket太多



#ifdef SUPPORT_TRACK
#define	MSG_SET_TEACHER_TRACK_PARAM	0xA0	//教师跟踪参数设置
#endif

#define AVIIF_KEYFRAME				0x00000010

#define INVALID_FD					-1

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~(B(bit))))
#define	TST(bit)	(A(bit) & B(bit))


/*message header*/
typedef struct __HDB_MSGHEAD {
	/*
	##  length for htons change
	## include length of structure
	## and real data
	*/
	WORD	nLen;
	WORD	nVer;							//version
	BYTE	nMsg;							//message type
	BYTE	szTemp[3];						//reserve
} MSGHEAD;

/*web message header*/
typedef struct _WEB_MSGHEAD {
	unsigned short	nLen;
	unsigned short	nType;							//version
} WEBMSGHEAD;


typedef enum {
    DL_NONE,
    DL_ERROR,
    DL_WARNING,
    DL_FLOW,
    DL_DEBUG,
    DL_ALL,
} EDebugLevle;

#define DEBUG_LEVEL   	(DL_ALL)


#define DEBUG(x,fmt,arg...) \
	do {\
		if ((x)<DEBUG_LEVEL){\
			fprintf(stderr, fmt, ##arg);\
		}\
	}while (0)




#endif
