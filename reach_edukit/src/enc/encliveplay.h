/*********************************************************************
��˾����:	Reach Software
�ļ���:
������:	Huang Haihong				����:	2011-06-20
�޸���:	Huang Haihong				����:	2011-06-20

˵��:
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

#define MSG_TRACKAUTO				48 //�����Զ�����ģ����Զ��ֶ�ģʽ
#define MSG_SETVGAADJUST					43//�߿����

#define MSG_QUALITYVALUE					46 //���ø������������ģʽ(0:�� 1:�� 2:��)

#define MSG_FARCTRLEX      			47
#define MSG_TRACKAUTO				48 //�����Զ�����ģ����Զ��ֶ�ģʽ
#define MSG_UPLOADIMG 			    49//�ϴ�logoͼƬ
#define MSG_GET_LOGOINFO 			50//��ȡlogoͼƬ��Ϣ
#define MSG_SET_LOGOINFO 			51//����logoͼƬ��Ϣ

#define	MSG_SET_TRACK_TYPE			110	//��ʦ���ٻ���ѧ�����ٻ�ͨ�ŵ���Ϣ����

#define MSG_TEACHER_HEART         111  //��ʦ������
/**
* @ ���͵�ǰ֡������Ϣ
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
#define MSG_LOW_SCREENDATA				0x95//����������
#define MSG_LOW_BITRATE					0x96 //������
#define MSG_MUTE							0x97
#define MSG_LOCK_SCREEN					0x99//������Ļ
#define MSG_GSCREEN_CHECK					0x9a//����У��
#define MSG_FIX_RESOLUTION				0x9e//���������ֱ���


#define	MSG_SET_TEACHER_TRACK_PARAM	0xA0	//��ʦ���ٲ�������
#define	MSG_SET_STUDENTS_TRACK_PARAM	0xA1	//��ʦ���ٲ�������

//-------------------------------------------�鲥----------------------------------------------------------------------------------------------------------------------
#define MSG_MUL_START						0x80            //�����鲥 HDB_MSGHEAD
#define MSG_MUL_STOP						0x81            //ֹͣ�鲥 HDB_MSGHEAD
#define MSG_MUL_VIDEOSIZE					0x82            //�����鲥��Ƶ�ߴ磬HDB_MSGHEAD +flag(BYTE) flag: 0��ԭʼ�ߴ磬1����С
#define MSG_MUL_ADDR_PORT				0x83            //�����鲥��ַ�Ͷ˿�, HDB_MSGHEAD+MulAddr
#define MSG_MUL_STREAM					0x84            //�����鲥����HDB_MSGHEAD +flag(BYTE) flag: 0����������1����Ƶ��2����Ƶ
#define MSG_MUL_STATE						0x85            // ��ȡ�鲥״̬��HDB_MSGHEAD+MulAddr+sizeflag��BYTE��+streamFlag��BYTE��+state(BYTE)(����/ֹͣ).
#define MSG_MUL_SET_AUDIO_ADDR			0x86            //������Ƶ�鲥��ַ�Ͷ˿�, HDB_MSGHEAD+MulAddr
#define MSG_MUL_SET_VIDEO_ADDR			0x87            //������Ƶ�鲥��ַ�Ͷ˿�, HDB_MSGHEAD+MulAddr
#define MSG_MUL_GET_AUDIO_ADDR			0x88            //��ȡ��Ƶ�鲥��ַ�Ͷ˿�, HDB_MSGHEAD+MulAddr
#define MSG_MUL_GET_VIDEO_ADDR			0x89            //��ȡ��Ƶ�鲥��ַ�Ͷ˿�, HDB_MSGHEAD+MulAddr
#define MSG_MUL_GET_ADDR					0x90            //��ȡ�鲥��ַ�Ͷ˿�, HDB_MSGHEAD+MulAddr
#define MSG_MUL_SET_FRAMEBITRATE			0x91
#define MSG_MUL_GET_FRAMEBITRATE			0x92
#define MSG_HEART							0x93			//����

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
#define ISZOOMIN		(ZOOMIN == gResizeP.flag)//�Ŵ�
#define NORESIZE		(RESIZE_INVALID == gResizeP.flag)
#define INIT_RESIZE_PARAM	{RESIZE_INVALID,RECREATE_INVALID,5,0,0,800,600,800,600};

typedef struct __HDB_SIZETYPE
{
	int nflag; // =0 ԭʼ��С =1 ��С��ָ���ߴ�
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

/*¼����Ϣ*/
typedef struct _RecDataInfo {
	char  IpAddr[16];		//¼�����ݵ�iP��ַ
	int   bConnected;		//�Ƿ�������
	int   nDataCodec;		//���뷽ʽ JPEGΪ 0x1001
	int   nFrameRate;		//����֡��
	int   nWidth;			//���ݵ�¼�ƿ��
	int   nHight;			//�߶�
	int   nColors;			//��ɫ��
	int   nOthers;			//����
	char  Version[16];		//�汾��Ϣ
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

/*��Ƶ����Ƶ���緢������ͷ*/
typedef struct __HDB_FRAME_HEAD {
	DWORD ID;								//=mmioFOURCC('4','D','S','P');
	DWORD nTimeTick;    					//ʱ���
	DWORD nFrameLength; 					//֡����
	DWORD nDataCodec;   					//��������
	//���뷽ʽ
	//��Ƶ :mmioFOURCC('H','2','6','4');
	//��Ƶ :mmioFOURCC('A','D','T','S');
	DWORD nFrameRate;   					//��Ƶ  :֡��
	//��Ƶ :������ (default:44100)
	DWORD nWidth;       					//��Ƶ  :��
	//��Ƶ :ͨ���� (default:2)
	DWORD nHight;       					//��Ƶ  :��
	//��Ƶ :����λ (default:16)
	DWORD nColors;      					//��Ƶ  :��ɫ��  (default: 24)
	//��Ƶ :��Ƶ���� (default:64000)
	DWORD dwSegment;						//�ְ���־λ
	DWORD dwFlags;							//��Ƶ:  I ֡��־
	//��Ƶ:  ����
	DWORD dwPacketNumber; 					//�����
	DWORD nOthers;      					//����
} FRAMEHEAD;


typedef struct _low_bitrate
{
	int nWidth;		//��
	int nHeight;		//��
	int nFrame;		//֡��
	int nBitrate;		//����
	int nTemp;		//����
}low_bitrate;

/*��Ƶ������*/
typedef struct __AUDIOPARAM {
	DWORD Codec;  							//��Ƶ����ID
	DWORD SampleRate; 						//������
	DWORD BitRate;  						//����
	DWORD Channel;  						//ͨ����
	DWORD SampleBit;  						//����λ
	BYTE  LVolume;							//������     0 --------31
	BYTE  RVolume;							//������     0---------31
	WORD  InputMode;                       ////1------ MIC input 0------Line Input
} AudioParam;

/*ϵͳ������*/
typedef struct __SYSPARAMS {
	unsigned char szMacAddr[8];				//MAC ��ַ
	unsigned int dwAddr;					//IP ��ַ
	unsigned int dwGateWay;					//����
	unsigned int dwNetMark;					//��������
	char strName[16];						//��Ʒ���к�
	char strVer[10];						//�������汾��
	WORD unChannel;							//ͨ����
	char bType; 							//���ֱ���������  0 -------VGABOX     3-------200 4-------110 5-------120 6--------1200  8---ENC-1100
	char bTemp[3]; 							//����
	int nTemp[7];							//����
} SYSPARAMS;


/*��Ƶ������*/
typedef struct __VIDEOPARAM {
	DWORD nDataCodec;   					//���뷽ʽ
	//��Ƶ: mmioFOURCC('H','2','6','4');
	DWORD nFrameRate;   					//��Ƶ::֡��
	DWORD nWidth;       					//��Ƶ::��
	DWORD nHight;       					//��Ƶ: ��
	DWORD nColors;      					//��Ƶ: ��ɫ
	DWORD nQuality;							//��Ƶ: ����
	WORD sCbr;								//��Ƶ: ������/������
	WORD sBitrate;							//��Ƶ: ����
} VideoParam;

typedef struct _DataHeader
{
    DWORD   id;
	long    biWidth;							//VGA��Width		Audio��SampleRate
	long    biHeight;							//VGA��Height	Audio��BitRate
	WORD    biBitCount;							//VGA��BitCount	Audio��Channel
	DWORD   fccCompressedHandler;				//VGA��mmioFOURCC (��H��,��2��,��6��,��4��)
											    //Audio��mmioFOURCC(��A��,��D��,��T��,��S��)
	DWORD   fccHandler;	   						//Audio��SampleBit
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
	BYTE bVmode;		//��Ƶģʽ    0 ------- 4CIF   1 ------CIF   2------QCIF
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

