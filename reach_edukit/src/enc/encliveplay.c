#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctype.h>

#include "reach.h"
#include "encliveplay.h"
#include "video.h"
#include "audio.h"
#include "app_network.h"
#include "capture.h"
#include "commontrack.h"

#if 0
//如修改 请同步修改
#define STU_CHID			0
#define STUSIDE_CHID		1
#define TEACH_CHID		2
#define VGA_CHID			3
#define JPEG_CHID			4
#endif

//对应各个客户端类型
int flag[6] = { -1, -1, -1, -1, -1, -1}; //   0 老师 和 学生 1 学生辅助 2VGA
int teachorstu[6] = { -1, -1, -1, -1, -1, -1}; //0 stu 1 teach  2 stuside
int studentFlag = 0; //标识学生是否连接


#define  PORT_ONE 0
extern EduKitLinkStruct_t	*gEduKit;
extern track_encode_info_t	g_track_encode_info;
extern stutrack_encode_info_t	g_stutrack_encode_info;
extern stusidetrack_encode_info_t	g_stusidetrack_encode_info;
int 	gRunStatus = 1;
static unsigned long g_video_time = 0;
static unsigned int gnAudioCount = 0;
sem_t	g_sem;

AudioParam		gAudioParam;
SYSPARAMS		gSysParsm;

DSPCliParam		gDSPCliPara[DSP_NUM];


static unsigned int				gnSentCount					= 0;

static unsigned char			gszSendBuf[MAX_PACKET];
static unsigned char            gszAudioBuf[14600];
VideoParam gvideoPara;

pthread_mutex_t g_send_m;

extern track_strategy_info_t		g_track_strategy_info;
extern Int32 StudentUp;
extern Int32 gMpMode;
static unsigned long getCurrentTime2(void)
{
	struct timeval tv;
	struct timezone tz;
	unsigned long ultime;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec * 1000 + ((tv.tv_usec) / 1000);
	return (ultime);
}
/*消息头打包*/
void PackHeaderMSG(BYTE *data,
                   BYTE type, WORD len)
{
	MSGHEAD  *p;
	p = (MSGHEAD *)data;
	memset(p, 0, HEAD_LEN);
	p->nLen = htons(len);
	p->nMsg = type;
	return ;
}

/*初始化视频参数表*/
static void InitVideoParam(VideoParam *video)
{
	bzero(video, sizeof(VideoParam));
	video->nDataCodec = 0x34363248;	//"H264"
	video->nWidth = 1920;			//video width
	video->nHight = 1080;			//video height
	video->nQuality = 45;				//quality (5---90)
	video->sCbr = 0;					// 0---quality  1---bitrate
	video->nFrameRate = 25;			//current framerate
	video->sBitrate = 4096;				//bitrate (128k---4096k)
	video->nColors = 24;			//24

}

/*set video param table*/
int SetVideoParam(int dsp, unsigned char *data, int len)
{
	unsigned char temp[200];
	VideoParam *Newp, *Oldp;
	memcpy(temp, data, len);
	Oldp = &gvideoPara;
	Newp = (VideoParam *)&temp[0];
	printf("set video param  old nFrameRate : [%d], new nFrameRate : [%d] bitrate : [%d]\n\n",
	       Oldp->nFrameRate, Newp->nFrameRate, Newp->sBitrate);

	memcpy(Oldp, Newp, sizeof(VideoParam));
	VideoEncodeParam vinfo;
	vinfo.sBitrate = Newp->sBitrate;
	vinfo.nFrameRate = Newp->nFrameRate;
	setVideoEncodeParam(0, &vinfo);
	return 0;
}

/*get video param*/
void GetVideoParam(int socket, int dsp, unsigned char *data, int len)
{
	unsigned char temp[200];
	int length = 0, ret;
	length = HEAD_LEN + sizeof(gvideoPara);
	PackHeaderMSG(temp, MSG_GET_VIDEOPARAM, length);

	memcpy(temp + HEAD_LEN, &gvideoPara, sizeof(gvideoPara));
	ret = WriteData(socket, temp, length);

	if(ret < 0) {
		printf("Send GetAudioParam()return:%d socket:%d\n", ret, socket);
	}

	return ;
}

/*modify standard system param*/
static void InitStandardParam(SYSPARAMS *psys)
{
	memset(psys, 0, sizeof(*psys));
	psys->dwAddr = GetIPaddr("eth0");
	psys->dwGateWay =  getGateWay("eth0");
	psys->dwNetMark = GetNetmask("eth0");

	psys->szMacAddr[0] = 0x00;
	psys->szMacAddr[1] = 0x09;
	psys->szMacAddr[2] = 0x30;
	psys->szMacAddr[3] = 0x28;
	psys->szMacAddr[4] = 0x12;
	psys->szMacAddr[5] = 0x22;

	strcpy(psys->strName, "EduKit-SD");
	strcpy(psys->strVer, "V1.0.0");
	psys->unChannel = 1;
	psys->bType = 6;  //0 -------VGABOX  3-------200 4-------110 5-------120 6--------1200  8 --ENC-1100
	bzero(psys->nTemp, sizeof(psys->nTemp));

}

#if 0
/*Initial video param table*/
static void InitVideoParam(VideoParam *video)
{
	//全初始化为零
	bzero(video, sizeof(VideoParam));
	video->nDataCodec = 0x34363248;	//"H264"
	video->nWidth = 800;				//视频宽EncoderManage 0---720*480 1---CIF 2 ---QCIF
	video->nHight = 600;				//视频高
	video->nQuality = 45;
	video->nColors = 24;
	video->sCbr = 0;
	video->nFrameRate = 30;			//控制帧率
	video->sBitrate = 1024;
}
#endif

/*Initial audio param table*/
static void InitAudioParam(AudioParam *audio)
{
	//全初始化为零
	bzero(audio, sizeof(AudioParam));
	audio->Codec = ADTS;		//"ADTS"
	///0---8KHz
	///2---44.1KHz 音频采样率 (默认为44100)
	///1---32KHz
	///3---48KHz
	///其他-----96KHz
	audio->SampleRate = 2;
	audio->Channel = 2;					//声道数 (默认为2)
	audio->SampleBit = 16;				//采样位 (默认为16)
	audio->BitRate = 12800;				//编码带宽 (默认为64000)
	audio->InputMode = LINE_INPUT;		//线性输入
	audio->RVolume = 31;					//左右声道音量
	audio->LVolume = 31;
}


static int SetSendTimeOut(int sSocket, unsigned long time)
{
	struct timeval timeout ;
	int ret = 0;

	timeout.tv_sec = time ; //3
	timeout.tv_usec = 0;

	ret = setsockopt(sSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

	if(ret == -1) {
		fprintf(stderr, "setsockopt() Set Send Time Failed\n");
	}

	return ret;
}


static int GetNullClientData(unsigned char dsp)
{
	int cli ;

	for(cli = 0; cli < MAX_CLIENT; cli++) {
		//		fprintf(stderr, "\n\n");
		//		fprintf(stderr, "cnt: %d		used: %d\n", cli, ISUSED(dsp, cli));
		//		fprintf(stderr, "cnt: %d		login: %d\n", cli, ISLOGIN(dsp, cli));
		//		fprintf(stderr, "\n\n\n");
		if((!ISUSED(dsp, cli)) && (!ISLOGIN(dsp, cli))) {
			return cli;
		}
	}

	return -1;
}

Void InitClient(unsigned char dsp)
{
	int cli ;

	for(cli = 0; cli < MAX_CLIENT; cli++) {

		SETSOCK(dsp, cli, INVALID_SOCKET);

	}
}

void ClearLostClient(unsigned char dsp)
{
	int cli ;

	for(cli = 0; cli < MAX_CLIENT; cli++) {
		if(!ISUSED(dsp, cli) && ISSOCK(dsp, cli)) {
			close(GETSOCK(dsp, cli));
			SETSOCK(dsp, cli, INVALID_SOCKET);
			SETCLILOGIN(dsp, cli, FALSE);
			SETPPTINDEX(dsp, cli, FALSE);
			SETPPTQUERY(dsp, cli, FALSE);
		}
	}
}

void GetAudioParam(int socket, int dsp, unsigned char *data, int len)
{
	unsigned char temp[200];
	int length = 0, ret;

	length = HEAD_LEN + sizeof(gAudioParam);
	PackHeaderMSG(temp, MSG_GET_AUDIOPARAM, length);

	memcpy(temp + HEAD_LEN, &gAudioParam, sizeof(gAudioParam));
	ret = WriteData(socket, temp, length);

	if(ret < 0) {
		printf("Send GetAudioParam()return:%d socket:%d\n", ret, socket);
	}


	return ;
}


int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	//	pthread_mutex_lock(&gSetP_m.send_m);
	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf + nWriteLen, nSize - nWriteLen, 0);

		if(nRet < 0) {
			fprintf(stderr, "WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen, s, errno);

			if((errno == ENOBUFS) && (nCount < 10)) {
				fprintf(stderr, "network buffer have been full!\n");
				usleep(10000);
				nCount++;
				continue;
			}

			//			pthread_mutex_unlock(&gSetP_m.send_m);
			return nRet;
		} else if(nRet == 0) {
			fprintf(stderr, "WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen, s, errno);
			fprintf(stderr, "Send Net Data Error nRet= %d\n", nRet);
			continue;
		}

		nWriteLen += nRet;
		nCount = 0;
	}

	//	pthread_mutex_unlock(&gSetP_m.send_m);
	return nWriteLen;
}

#if 0
void SendAudioToClient(int nLen, unsigned char *pData,
                       int nFlag, const unsigned char index, AudioParam *paparam)
{
	int nRet, nSent, nSendLen, nPacketCount, nMaxDataLen;
	DataHeader	DataFrame;


	int cnt = 0;

	pthread_mutex_lock(&g_send_m);

	bzero(&DataFrame, sizeof(DataHeader));

	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = MAX_PACKET - sizeof(DataHeader) - HEAD_LEN;

	DataFrame.fccCompressedHandler = 0x53544441 ; //ADTS;
	DataFrame.biWidth = paparam->SampleRate;
	DataFrame.biBitCount = paparam->Channel;
	DataFrame.fccHandler = paparam->SampleBit;
	DataFrame.biHeight = paparam->BitRate;

	if(nFlag == 1) {
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}

	while(nSent < nLen) {

		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;
			} else {
				DataFrame.dwSegment = 0;
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;
			} else {
				DataFrame.dwSegment = 1;
			}

			nPacketCount++;
		}

		DataFrame.dwPacketNumber = gnAudioCount++;
		DataFrame.dwCompressedDataLength = nLen;
		memcpy(gszAudioBuf + HEAD_LEN, &DataFrame, sizeof(DataHeader));
		memcpy(gszAudioBuf + sizeof(DataHeader) + HEAD_LEN, pData + nSent, nSendLen);
		gszAudioBuf[0] = (sizeof(DataHeader) + nSendLen + 3) >> 8;
		gszAudioBuf[1] = (sizeof(DataHeader) + nSendLen + 3) & 0xff;
		gszAudioBuf[2] = MSG_AUDIODATA;

		for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
			if(ISUSED(0, cnt) && ISLOGIN(0, cnt)) {
				nRet = WriteData(GETSOCK(0, cnt), gszAudioBuf, nSendLen + sizeof(DataHeader) + HEAD_LEN);

				if(nRet < 0) {
					SETCLIUSED(0, cnt, FALSE);
					SETCLILOGIN(0, cnt, FALSE);
					SETLOWSTREAM(DSP1, cnt, FALSE);
					fprintf(stderr, "Audio Send error index:%d, cnt:%d %d nSendLen:%d  ,nRet:%d\n", index, cnt, errno, nSendLen, nRet);
				}
			}
		}

		nSent += nSendLen;
	}

	pthread_mutex_unlock(&g_send_m);
}
#endif

static unsigned long g_audio_time = 0;


/*set audio header infomation*/
static void SetAudioHeader(FRAMEHEAD *pAudio, AudioParam *pSys)
{
	pAudio->nDataCodec = 0x53544441;					//"ADTS"
	pAudio->nFrameRate = pSys->SampleRate; 			//sample rate 1-----44.1KHz
	pAudio->nWidth = pSys->Channel;					//channel (default: 2)
	pAudio->nHight = pSys->SampleBit;				//sample bit (default: 16)
	pAudio->nColors = pSys->BitRate;					//bitrate  (default:64000)
}

/*send encode audio data to every client*/
void SendAudioToClient(int nLen, unsigned char *pData,
                       int nFlag, unsigned char index, unsigned int samplerate)
{

	pthread_mutex_lock(&g_send_m);
	int nRet, nSent, nSendLen, nPacketCount, nMaxDataLen;
	FRAMEHEAD  DataFrame;
	int cnt = 0;
	MSGHEAD  *p;

	//	ParsePackageLock();
	bzero(&DataFrame, sizeof(FRAMEHEAD));
	DataFrame.dwPacketNumber = gnAudioCount++;

	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = MAX_PACKET - sizeof(FRAMEHEAD) - HEAD_LEN;

	DataFrame.ID = 0x56534434;		// " 4DSP"
	DataFrame.nFrameLength = nLen;


	unsigned int timeTick = 0;

	if(g_audio_time == 0) {
		timeTick = 0 ;
		g_audio_time = getCurrentTime2() ;
	} else {
		timeTick = getCurrentTime2() - g_audio_time;
	}

	DataFrame.nTimeTick = timeTick;
	SetAudioHeader(&DataFrame, &gAudioParam);
	//	DataFrame.nWidth = 2;					//sound counts (default:2)
	//DataFrame.nHight = samplerate;					//samplebit (default:16)

	if(nFlag == 1) {
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}

	while(nSent < nLen) {

		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;
			} else {
				DataFrame.dwSegment = 0;
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;
			} else {
				DataFrame.dwSegment = 1;
			}

			nPacketCount++;
		}

		memcpy(gszAudioBuf + HEAD_LEN, &DataFrame, sizeof(FRAMEHEAD));
		memcpy(gszAudioBuf + sizeof(FRAMEHEAD) + HEAD_LEN, pData + nSent, nSendLen);
		//PackHeaderMSG(gszAudioBuf,MSG_AUDIODATA,(nSendLen+sizeof(FRAMEHEAD)+HEAD_LEN));
		p = (MSGHEAD *)gszAudioBuf;
		memset(p, 0, HEAD_LEN);
		p->nLen = htons((nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN));
		p->nMsg = MSG_AUDIODATA;


		for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
			if(ISUSED(index, cnt) && ISLOGIN(index, cnt)) {
				nRet = WriteData(GETSOCK(index, cnt), gszAudioBuf, nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN);

				if(nRet < 0) {
					SETCLIUSED(index, cnt, FALSE);
					SETCLILOGIN(index, cnt, FALSE);
					printf("Audio Send error index:%d, cnt:%d %d nSendLen:%d  ,nRet:%d\n", index, cnt, errno, nSendLen, nRet);
				}
			}
		}

		nSent += nSendLen;
	}

	pthread_mutex_unlock(&g_send_m);
	//	ParsePackageunLock();
}



void SendDataToClient(int nLen, unsigned char *pData,
                      int nFlag, unsigned char index, int width, int height)
{

	pthread_mutex_lock(&g_send_m);

	int nRet, nSent, nSendLen, nPacketCount, nMaxDataLen;
	FRAMEHEAD	DataFrame;
	int cnt = 0;
	SOCKET	sendsocket = 0;
	MSGHEAD	*p;
	//pthread_mutex_lock(&g_send_m);
	bzero(&DataFrame, sizeof(FRAMEHEAD));
	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = MAX_PACKET - sizeof(FRAMEHEAD) - HEAD_LEN;


	DataFrame.ID = 0x56534434;		// "4DSP"
	DataFrame.nFrameLength = nLen;
	DataFrame.nDataCodec = 0x34363248;		//"H264"

	unsigned int timeTick = 0;

	if(g_video_time == 0) {
		timeTick = 0 ;
		g_video_time = getCurrentTime2();
	} else {

		timeTick = getCurrentTime2() - g_video_time;

	}

	DataFrame.nTimeTick = timeTick;

	DataFrame.nWidth = width;		//video width
	DataFrame.nHight = height;		//video height

	if(DataFrame.nHight == 1088) {
		DataFrame.nHight = 1080;
	}


	if(nFlag == 1)
		//if I frame
	{
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}


	while(nSent < nLen) {
		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;    //start frame
			} else {
				DataFrame.dwSegment = 0;    //middle frame
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;    //first frame and last frame
			} else {
				DataFrame.dwSegment = 1;    //last frame
			}

			nPacketCount++;
		}


		DataFrame.dwPacketNumber = gnSentCount++;
		memcpy(gszSendBuf + HEAD_LEN, &DataFrame, sizeof(FRAMEHEAD));
		memcpy(gszSendBuf + sizeof(FRAMEHEAD) + HEAD_LEN, pData + nSent, nSendLen);
		p = (MSGHEAD *)gszSendBuf;
		memset(p, 0, HEAD_LEN);
		p->nLen = htons((nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN));
		p->nMsg = MSG_SCREENDATA;

		//send multi client
		for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
			if(ISUSED(0, cnt) && ISLOGIN(0, cnt) && !GETLOWSTREAM(0, cnt)) {
				sendsocket = GETSOCK(0, cnt);

				if(sendsocket > 0) {
					nRet = WriteData(sendsocket, gszSendBuf, nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN);

					if(nRet < 0) {
						SETCLIUSED(0, cnt, FALSE);
						SETCLILOGIN(0, cnt, FALSE);
						SETPPTQUERY(0, cnt, FALSE);
						printf("Error: SOCK = %d count = %d  errno = %d  ret = %d\n", sendsocket, cnt, errno, nRet);
					}
				}
			}

		}

		nSent += nSendLen;
	}

	pthread_mutex_unlock(&g_send_m);
}

#define HEAD_LEN110	3
/*send encode data to every client*/
void SendDataToClient110(int nLen, unsigned char *pData,
                         int nFlag, unsigned char index , int width, int height)
{
	int nRet, nSent, nSendLen, nPacketCount, nMaxDataLen;
	DataHeader  DataFrame;
	int cnt = 0;
	SOCKET  sendsocket = 0;

	pthread_mutex_lock(&g_send_m);
	bzero(&DataFrame, sizeof(DataHeader));
	static uint32_t time = 0;
	static uint32_t test = 0;
	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = MAX_PACKET - sizeof(DataHeader) - HEAD_LEN110;
	DataFrame.id = 0x1001;

	DataFrame.dwCompressedDataLength = nLen;

	DataFrame.biBitCount = 24;  				//颜色数

	DataFrame.biWidth = width;		//视频宽
	DataFrame.biHeight = height; 	//视频高

	if(nFlag == 1)	{	//if I frame
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}

	DataFrame.fccCompressedHandler = 0x34363248;//0x30484352; H264
	DataFrame.fccHandler = 0x34363248;//0x30484352;  H264




	while(nSent < nLen) {
		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;    //start frame
			} else {
				DataFrame.dwSegment = 0;    //middle frame
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;    //first frame and last frame
			} else {
				DataFrame.dwSegment = 1;    //last frame
			}

			nPacketCount++;
		}

		DataFrame.dwPacketNumber = gnSentCount++;
		memcpy(gszSendBuf + HEAD_LEN110, &DataFrame, sizeof(DataHeader));
		memcpy(gszSendBuf + sizeof(DataHeader) + HEAD_LEN110, pData + nSent, nSendLen);
		gszSendBuf[0] = (nSendLen + sizeof(DataHeader) + HEAD_LEN110) >> 8;
		gszSendBuf[1] = (nSendLen + sizeof(DataHeader) + HEAD_LEN110) & 0xff;
		gszSendBuf[2] = MSG_SCREENDATA;


		//send multi client
		for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
			if(ISUSED(0, cnt) && ISLOGIN(0, cnt)) {
				sendsocket = GETSOCK(0, cnt);

				int sendflag = 0;

				//学生全景
				if((index == 0) && (flag[cnt] == 0) && (teachorstu[cnt] == 0)) {
					sendflag = 1;
				}
				//老师全景 or 学生辅助
				else if((index == 1) && (flag[cnt] == 0) && (teachorstu[cnt] == 1)) {
					sendflag = 1;
				}
				//VGA
				else if((index == 2) && (flag[cnt] == 2) && (teachorstu[cnt] == -1)) {

					sendflag = 1;
				} else if((index == 3) && (teachorstu[cnt] == 2) && (flag[cnt] == 0)) {
					sendflag = 1;

				}

				if((sendsocket > 0) && (sendflag == 1))  {


					if((index == 0) && (flag[cnt] == 0) && (teachorstu[cnt] == 0) && (cnt == 0)) {
						test++;

						if(getostime() - time > 500) {
							printf("time is @%d socket = %d cnt =%d\n", getostime() - time, sendsocket, cnt);
						} else if(getostime() - time > 50)

						{
							printf("time is #%d socket = %d cnt = %d\n", getostime() - time, sendsocket, cnt);
						}

						time = getostime();

#if 0

						if(test == 440) {
							test = 0;
							sleep(2);
						}

#endif
					}

					nRet = WriteData(sendsocket, gszSendBuf, nSendLen + sizeof(DataHeader) + HEAD_LEN110);

					if(nRet < 0) {
						SETCLIUSED(0, cnt, FALSE);
						SETCLILOGIN(0, cnt, FALSE);

						printf("Send to socket Error: SOCK = %d count = %d  errno = %d  ret = %d\n", sendsocket, cnt, errno, nRet);
					}
				}
			}
		}

		nSent += nSendLen;
	}

	pthread_mutex_unlock(&g_send_m);
}
static int32_t write_aacfile(char *file, void *buf, int buflen)
{
	static FILE *fp = NULL;

	if(NULL == fp) {
		fp =	fopen(file, "w");

		if(NULL == fp) {
			printf("fopen  : %s", strerror(errno));
			return -1;
		}
	}

	printf("[write_file] buf : [%p], buflen : [%d] , fp : [%p]\n", buf, buflen, fp);
	fwrite(buf, buflen, 1, fp);
	return 0;
}

static void SetAudioHeader110(DataHeader *pAudio, AudioParam *pSys)
{
	pAudio->fccCompressedHandler = 0x53544441 ; //ADTS;
	pAudio->biWidth = pSys->SampleRate; 			//   1--44.1KHz 音频采样率 (默认为44100)
	pAudio->biBitCount = pSys->Channel;					//声道数 (默认为2)
	pAudio->fccHandler = pSys->SampleBit;				//采样位 (默认为16)
	pAudio->biHeight = pSys->BitRate;			//sample rate 1-----44.1KHz

}


void SendAudioToClient110(int nLen, unsigned char *pData,
                          int nFlag, unsigned char index, unsigned int samplerate)
{
	int nRet, nSent, nSendLen, nPacketCount, nMaxDataLen;
	DataHeader  DataFrame;


	int cnt = 0;


	bzero(&DataFrame, sizeof(DataHeader));

	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = MAX_PACKET - sizeof(DataHeader) - HEAD_LEN110;


	if(nFlag == 1) {
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}

	while(nSent < nLen) {

		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;
			} else {
				DataFrame.dwSegment = 0;
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;
			} else {
				DataFrame.dwSegment = 1;
			}

			nPacketCount++;
		}

		SetAudioHeader110(&DataFrame, &gAudioParam);
		DataFrame.dwCompressedDataLength = nLen;
		DataFrame.dwPacketNumber = gnAudioCount++;
		memcpy(gszAudioBuf + HEAD_LEN110, &DataFrame, sizeof(DataHeader));
		memcpy(gszAudioBuf + sizeof(DataHeader) + HEAD_LEN110, pData + nSent, nSendLen);
		gszAudioBuf[0] = (sizeof(DataHeader) + nSendLen + HEAD_LEN110) >> 8;
		gszAudioBuf[1] = (sizeof(DataHeader) + nSendLen + HEAD_LEN110) & 0xff;
		gszAudioBuf[2] = MSG_AUDIODATA;

		for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
			if(ISUSED(0, cnt) && ISLOGIN(0, cnt) && 2 == index) {
				///printf("[SendAudioToClient110] WriteData GETSOCK[%d]:[%d] \n", index, GETSOCK(0, cnt));
				nRet = WriteData(GETSOCK(0, cnt), gszAudioBuf, nSendLen + sizeof(DataHeader) + HEAD_LEN110);

				if(nRet < 0) {
					SETCLIUSED(0, cnt, FALSE);
					SETCLILOGIN(0, cnt, FALSE);
					fprintf(stderr, "Audio Send error index:%d, cnt:%d %d nSendLen:%d  ,nRet:%d\n", index, cnt, errno, nSendLen, nRet);
				}

			}
		}

		nSent += nSendLen;
	}

}

static void EncoderProcess(void *pParams)
{
	int					sSocket;
	WORD					length;

	char			szData[256] = {0}, szPass[] = "123";;

	int 					nPos					= 0;
	int 					nMsgLen 				= 0;
	int 					*pnPos					= (int *)pParams;
	int 					nLen					= 0;
	MSGHEAD header, *phead;
	fprintf(stderr, "Enter DSP1TcpComMsg() function!!\n");
	nPos = *pnPos;
	free(pParams);
	sSocket = GETSOCK(DSP1, nPos);

	phead = &header;
	memset(&header, 0, sizeof(MSGHEAD));
	/*设置TCP发送超时 3s*/
	SetSendTimeOut(sSocket, 3);
	memset(szData, 0, 256);
	sem_post(&g_sem);

	while(gRunStatus) {
		memset(szData, 0, sizeof(szData));

		if(sSocket <= 0) {
			fprintf(stderr, "sSocket<0 !\n");
			goto ExitClientMsg;
		}

		nLen = recv(sSocket, szData, HEAD_LEN, 0);
		fprintf(stderr, "receive length:%d,errno=%d\n", nLen, errno);
		memcpy(phead, szData, HEAD_LEN);

		if(nLen < HEAD_LEN || nLen == -1) {
			fprintf(stderr, "recv nLen < 2 || nLen == -1  goto ExitClientMsg\n ");
			goto ExitClientMsg;
		}

		nMsgLen = ntohs(*((short *)szData));
		phead->nLen = nMsgLen;
		nLen = recv(sSocket, szData + HEAD_LEN, nMsgLen - HEAD_LEN, 0);
		fprintf(stderr, "nMsgLen = %d,szData[2]=%d!\n", nMsgLen, szData[2]);

		if(nLen < nMsgLen - HEAD_LEN) {
			fprintf(stderr, "nLen < nMsgLen -2  goto ExitClientMsg\n ");
			goto ExitClientMsg;
		}

		fprintf(stderr, "------- socket = %d, type = %d, nLen = %d, nMsgLen = %d -------\n", sSocket, phead->nMsg, nLen, nMsgLen);

		/*信令解析处理*/
		switch(phead->nMsg) {
			case MSG_REQ_AUDIO:
				fprintf(stderr, "DSP1 request Audio Data \n");
				break;

			case MSG_GET_AUDIOPARAM:
				fprintf(stderr, "DSP1 Get AudioParam \n");

				GetAudioParam(sSocket, PORT_ONE, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN);
				break;

			case MSG_SET_AUDIOPARAM:
				fprintf(stderr, "DSP1 Set AudioParam \n");
				//			SetAudioParam(DSP1,(unsigned char *)&szData[3],nMsgLen-3);
				break;

			case MSG_GET_VIDEOPARAM:

				GetVideoParam(sSocket, PORT_ONE, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN);

				fprintf(stderr, "DSP1 Get VideoParam \n");
				break;

			case MSG_SET_VIDEOPARAM:

				SetVideoParam(PORT_ONE, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN);
				fprintf(stderr, "DSP1 Set VideoParam \n");
				break;

			case MSG_SETVGAADJUST:
				fprintf(stderr, "DSP1 ReviseVGAPic vga picture!\n");
				//			ReviseVGAPic(DSP1,(unsigned char *)&szData[3],nMsgLen-3);
				break;

			case MSG_GSCREEN_CHECK:
				//			GreenScreenAjust();
				break;

			case MSG_SET_DEFPARAM:
#if 0
				fprintf(stderr, "MSG_SET_DEFPARAM\n");
				unsigned char ParamBuf[200];
				SysParamsV2 *Newp, *Oldp;
				int ret = 0;

				memcpy(ParamBuf, &szData[3], nMsgLen - 3);
				gSysParaT.sysPara.sCodeType = 0x0F;
				Oldp = &gSysParaT.sysPara;
				Newp = (SysParamsV2 *)&ParamBuf[0];

				ret = SetSysParams(Oldp, Newp);
				SetEncodeVideoParams(&gVAenc_p, Newp);
#endif
				break;

			case MSG_ADD_TEXT:	//字幕
				printf("haaaaaaa\n");
				fprintf(stderr, "enter into MSG_ADD_TEXT \n");

				//			AddOsdText(PORT_ONE, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN);
#if 0
				RecAVTitle recavtitle;

				if(g_osd_flag == 0) { //no font file
					break;
				}

				memset(&recavtitle, 0x00000, sizeof(RecAVTitle));
				memcpy(&recavtitle, &szData[3], nMsgLen - 3);
				fprintf(stderr, "display the text =%s \n", recavtitle.Text);
				display_time = 0;

				if(recavtitle.len > 0) {
					int len = GetTextLen(recavtitle.Text);
					int size_len = GetSizeLen(recavtitle.Text);
					int charlen = strlen(recavtitle.Text);

					if(len != 0) {
						display_text = 1;
					} else {
						display_text = 0;
					}

					if(checkCHN(recavtitle.Text, charlen)) {
						char temp[512] = {0};
						code_convert("gb2312", "utf-32", recavtitle.Text, charlen, temp, charlen * 4);
						drawTextBuffer(temp, size_len);
					} else {
						drawTextBuffer(recavtitle.Text, size_len);
					}

					if(len != recavtitle.len) {
						fprintf(stderr, "display the time \n");
						drawTimeBuffer();
						display_time = 1;
					}

					textXpos = recavtitle.x;
					textYpos = recavtitle.y;

					if(textXpos % 32) {
						textXpos -= (textXpos % 32);
					}

					if(textYpos % 32) {
						textYpos -= (textYpos % 32);
					}

					fprintf(stderr, "textXpos = %d  textYpos = %d\n\n", textXpos, textYpos);

					if((GetVGAHeight() - textYpos) < 64) {
						textYpos = GetVGAHeight() - 64;
					}
				} else {
					display_text = 0;
				}

				fprintf(stderr, "DSP1 Add Text\n");
#endif
				break;

			case MSG_FARCTRL:
				fprintf(stderr, "DSP1 Far Control\n");

				//				FarCtrlCamera(DSP1,(unsigned char *)&szData[3],nMsgLen-3);
				//GreenScreenAjust();
				//ReviseVGAPic(DSP1,&szData[3],nMsgLen-3);
				break;

			case MSG_SET_SYSTIME:
				fprintf(stderr, "DSP1 Set system Time\n");
				break;

			case MSG_PASSWORD: { //登陆请求
				if(!(strcmp("sawyer", szData + HEAD_LEN))) {
					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);

				} else if(szData[HEAD_LEN] == 'A' && !strcmp(szPass, szData + HEAD_LEN + 1)) {
					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);
					fprintf(stderr, "logo Admin!\n");
				} else if(szData[HEAD_LEN] == 'U' && !strcmp(szPass, szData + HEAD_LEN + 1)) {

					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_USER);
					fprintf(stderr, "logo User!\n");
				} else {
					PackHeaderMSG((BYTE *)szData, MSG_PASSWORD_ERR, HEAD_LEN);
					send(sSocket, szData, HEAD_LEN, 0);
					fprintf(stderr, "logo error!\n");
					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);
					goto ExitClientMsg;   //
				}


				PackHeaderMSG((BYTE *)szData, MSG_CONNECTSUCC, HEAD_LEN);
				send(sSocket, szData, HEAD_LEN, 0);
				fprintf(stderr, "send MSG_CONNECTSUCC!\n");
				/*length*/
				length = HEAD_LEN + sizeof(gSysParsm);
				PackHeaderMSG((BYTE *)szData, MSG_SYSPARAMS, length);

				memcpy(szData + HEAD_LEN, &gSysParsm, sizeof(gSysParsm));
				send(sSocket, szData, length, 0);
				/*set client login succeed*/
				SETCLIUSED(PORT_ONE, nPos, TRUE);
				SETCLILOGIN(PORT_ONE, nPos, TRUE);
				break;
			}

			case MSG_SYSPARAMS: { //获取系统参数

				length = HEAD_LEN + sizeof(gSysParsm);
				PackHeaderMSG((BYTE *)szData, MSG_SYSPARAMS, length);
				memcpy(szData + HEAD_LEN, &gSysParsm, sizeof(gSysParsm));
				send(sSocket, szData, length, 0);

				fprintf(stderr, "Get Sys Params ..........................\n");
			}
			break;

			case MSG_SETPARAMS: {	//设置系统参数

				fprintf(stderr, "Set Params! gSysParams %d Bytes\n", sizeof(gSysParsm));
				unsigned char ParamBuf[200];
				SYSPARAMS *Newp;

				memcpy(ParamBuf, &szData[HEAD_LEN], nMsgLen - HEAD_LEN);

				Newp = (SYSPARAMS *)&ParamBuf[0];


				fprintf(stderr, "==================================================\n");
				fprintf(stderr, "new:	unChannel = %d\n", Newp->unChannel);
				fprintf(stderr, "new:	dwAddr    = %d\n", Newp->dwAddr);
				fprintf(stderr, "new:	strName   = %s\n", Newp->strName);
				fprintf(stderr, "new:	bType     = %d\n", Newp->bType);
				fprintf(stderr, "==================================================\n");

				//enc_set_fps(gEduKit.encLink.link_id, 0, (Newp->nFrame)*1000, (Newp->sBitrate)*1000);

			}
			break;

			case MSG_SAVEPARAMS:		//保存参数到flash
				fprintf(stderr, "Save Params!\n");
				//		SaveIPinfo(IPINFO_NAME, &gSysParaT);
				//		SaveSysParams(DSP1, CONFIG_NAME, &gSysParaT);
				break;

			case MSG_RESTARTSYS:
				fprintf(stderr, "Restart sys\n");
				system("reboot -f");
				break;

			case MSG_UpdataFile: { //升级过程
				int ret = 0 ;

				//		ret = DoUpdateProgram(sSocket,szData,nMsgLen);
				if(ret <= 1) {
					goto ExitClientMsg;
				}
			}
			break;

			case MSG_UPDATEFILE_ROOT:
				fprintf(stderr, "Updata Root file\n");
				break;

			case MSG_REQ_I:
				//				gReqIframe1 = 1;
				//				gReqIframe2 = 1;
				fprintf(stderr, "Request I Frame!\n");
				break;

			case MSG_PIC_DATA:
				//				SetPPTIndex(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				fprintf(stderr, "SetPPTIndex\n");
				break;

			case MSG_MUTE:
				//				SetNosound(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				fprintf(stderr, "NoSound!\n");
				break;

			case MSG_LOCK_SCREEN:
				//				LockScreen(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				break;

			case MSG_LOW_BITRATE:
				//				SetLowStream(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				//				SETLOWSTREAMFLAG(DSP1, nPos, 1);
				//				gLowStreamFlag = 1;
				break;

			case MSG_FIX_RESOLUTION:
				//				FixResolution(sSocket,nPos,(unsigned char *)(&szData[3]),nMsgLen-3);
				break;

			case MSG_SEND_INPUT:
				break;

			default
					:
				fprintf(stderr, "Error Cmd=%d!\n", phead->nMsg);
				break;
		}

	}


ExitClientMsg:
	fprintf(stderr, "\n|||||||| go down  |||||||||\n\n");
	fprintf(stderr, " socket = %d %d\n", GETSOCK(DSP1, nPos), sSocket);

	SETCLIUSED(DSP1, nPos, FALSE);
	SETSOCK(DSP1, nPos, INVALID_SOCKET);
	SETLOGINTYPE(DSP1, nPos, LOGIN_USER);
	SETCLILOGIN(DSP1, nPos, FALSE);
	SETPPTINDEX(DSP1, nPos, FALSE);
	SETPPTQUERY(DSP1, nPos, FALSE);
	SETLOWSTREAM(DSP1, nPos, 0);

	if(GETLOWSTREAMFLAG(DSP1, nPos)) {
		SETLOWSTREAMFLAG(DSP1, nPos, 0);
		//		gLowStreamFlag = 0;
	}

	close(sSocket);

	pthread_detach(pthread_self());
	pthread_exit(0);
}

static void EncoderProcess110(void *pParams)
{
	int					sSocket = -1;
	WORD					length;

	char			szData[8192] = {0}, szPass[] = "reach123";;

	int 					nPos					= 0;
	int 					nMsgLen 				= 0;
	int 					*pnPos					= (int *)pParams;
	int 					nLen					= 0;


	fprintf(stderr, "Enter EncoderProcess110() function!!\n");
	nPos = *pnPos;
	free(pParams);
	sSocket = GETSOCK(DSP1, nPos);
	SysParamsV2 sys2;

	/*设置TCP发送超时 3s*/
	SetSendTimeOut(sSocket, 3);
	memset(szData, 0, 256);
	sem_post(&g_sem);

	while(gRunStatus) {
		memset(szData, 0, sizeof(szData));

		if(sSocket <= 0) {
			fprintf(stderr, "sSocket<0 !\n");
			goto ExitClientMsg;
		}

		nLen = recv(sSocket, szData, 2, 0);
		fprintf(stderr, "receive length:%d,errno=%d\n", nLen, errno);

		if(nLen < 2 || nLen == -1) {
			fprintf(stderr, "recv nLen < 2 || nLen == -1  goto ExitClientMsg\n ");
			goto ExitClientMsg;
		}

		nMsgLen = ntohs(*((short *)szData));
		nLen = recv_long_tcp_data(sSocket, szData + 2, nMsgLen - 2);
		printf("recv nLen = %d\n", nLen);
		printf("nMsgLen = %d,szData[2]=%d!\n", nMsgLen, szData[2]);
		printf("szData[3],szData[4]=%d,%d,%d,%d!\n", szData[3], szData[4], szData[5], szData[6]);
		printf("szData[7],szData[8}=%d,%d,%d,%d!\n", szData[7], szData[8], szData[9], szData[10]);

		if(nLen < nMsgLen - 2) {
			fprintf(stderr, "nLen < nMsgLen -2  goto ExitClientMsg\n ");
			goto ExitClientMsg;
		}

		fprintf(stderr, "------- socket = %d, type = %x, nLen = %d, nMsgLen = %d -------\n", sSocket, szData[2], nLen, nMsgLen);

		/*信令解析处理*/
		switch(szData[2]) {
			case MSG_REQ_AUDIO:
				fprintf(stderr, "DSP1 request Audio Data \n");
				break;

			case MSG_GET_AUDIOPARAM:
				fprintf(stderr, "DSP1 Get AudioParam \n");

				GetAudioParam(sSocket, PORT_ONE, (BYTE *)&szData[3], nMsgLen - 3);
				break;

			case MSG_SET_AUDIOPARAM:
				fprintf(stderr, "DSP1 Set AudioParam \n");
				//SetAudioParam(DSP1,(unsigned char *)&szData[3],nMsgLen-3);
				break;

			case MSG_GET_VIDEOPARAM:

				GetVideoParam(sSocket, PORT_ONE, (BYTE *)&szData[3], nMsgLen - 3);

				fprintf(stderr, "DSP1 Get VideoParam \n");
				break;

			case MSG_SET_VIDEOPARAM:

				SetVideoParam(PORT_ONE, (BYTE *)&szData[3], nMsgLen - 3);
				fprintf(stderr, "DSP1 Set VideoParam \n");
				break;

			case MSG_SETVGAADJUST:
				fprintf(stderr, "DSP1 ReviseVGAPic vga picture!\n");
				//			ReviseVGAPic(DSP1,(unsigned char *)&szData[3],nMsgLen-3);
				break;

			case MSG_GSCREEN_CHECK:
				//			GreenScreenAjust();
				break;

			case MSG_SET_DEFPARAM:
#if 0
				fprintf(stderr, "MSG_SET_DEFPARAM\n");
				unsigned char ParamBuf[200];
				SysParamsV2 *Newp, *Oldp;
				int ret = 0;

				memcpy(ParamBuf, &szData[3], nMsgLen - 3);
				gSysParaT.sysPara.sCodeType = 0x0F;
				Oldp = &gSysParaT.sysPara;
				Newp = (SysParamsV2 *)&ParamBuf[0];

				ret = SetSysParams(Oldp, Newp);
				SetEncodeVideoParams(&gVAenc_p, Newp);
#endif
				break;

			case MSG_ADD_TEXT:	//字幕
				printf("haaaaaaa\n");
				fprintf(stderr, "enter into MSG_ADD_TEXT \n");

				//			AddOsdText(PORT_ONE, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN);
#if 0
				RecAVTitle recavtitle;

				if(g_osd_flag == 0) { //no font file
					break;
				}

				memset(&recavtitle, 0x00000, sizeof(RecAVTitle));
				memcpy(&recavtitle, &szData[3], nMsgLen - 3);
				fprintf(stderr, "display the text =%s \n", recavtitle.Text);
				display_time = 0;

				if(recavtitle.len > 0) {
					int len = GetTextLen(recavtitle.Text);
					int size_len = GetSizeLen(recavtitle.Text);
					int charlen = strlen(recavtitle.Text);

					if(len != 0) {
						display_text = 1;
					} else {
						display_text = 0;
					}

					if(checkCHN(recavtitle.Text, charlen)) {
						char temp[512] = {0};
						code_convert("gb2312", "utf-32", recavtitle.Text, charlen, temp, charlen * 4);
						drawTextBuffer(temp, size_len);
					} else {
						drawTextBuffer(recavtitle.Text, size_len);
					}

					if(len != recavtitle.len) {
						fprintf(stderr, "display the time \n");
						drawTimeBuffer();
						display_time = 1;
					}

					textXpos = recavtitle.x;
					textYpos = recavtitle.y;

					if(textXpos % 32) {
						textXpos -= (textXpos % 32);
					}

					if(textYpos % 32) {
						textYpos -= (textYpos % 32);
					}

					fprintf(stderr, "textXpos = %d  textYpos = %d\n\n", textXpos, textYpos);

					if((GetVGAHeight() - textYpos) < 64) {
						textYpos = GetVGAHeight() - 64;
					}
				} else {
					display_text = 0;
				}

				fprintf(stderr, "DSP1 Add Text\n");
#endif
				break;

			case MSG_FARCTRL:
				fprintf(stderr, "DSP1 Far Control %d\n", teachorstu[nPos]);

				if(teachorstu[nPos] == 0) {
					FarCtrlCamera(1, (unsigned char *)&szData[3], nMsgLen - 3);
				} else if(teachorstu[nPos] == 1) {
					FarCtrlCamera(0, (unsigned char *)&szData[3], nMsgLen - 3);
				}

				//GreenScreenAjust();
				//ReviseVGAPic(DSP1,&szData[3],nMsgLen-3);
				break;

			case MSG_SET_SYSTIME:
				fprintf(stderr, "DSP1 Set system Time\n");
				break;

			case MSG_PASSWORD: { //登陆请求
				RecDataInfo theDataInfo;

				if(!(strcmp("sawyer", szData + 3))) {
					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);

				} else if((szData[3] == 'A') && (!strcmp("reach123", szData + 3 + 1)  || !strcmp("123", szData + 3 + 1))) {
					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);
					fprintf(stderr, "logo Admin [%s]!\n", szData + 3 + 1);


					if(!strcmp("123", szData + 3 + 1)) {
						//学生辅助
						flag[nPos] = 1;
					} else {
						//老师 和 学生
						flag[nPos] = 0;
					}
				} else if((szData[3] == 'U') && ((!strcmp("reach123", szData + 3 + 1)) || (!strcmp("123", szData + 3 + 1)))) {

					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_USER);
					fprintf(stderr, "logo User!\n");
				} else if((szData[3] == 'A') && (!strcmp("1234", szData + 3 + 1))) {

					flag[nPos] = 2;

				}
				//老师
				else if((szData[3] == 'A') && (!strcmp("1", szData + 3 + 1))) {

					flag[nPos] = 0;
					teachorstu[nPos] = 1;
					enc_enable_channel(gEduKit->encoderLink.encLink.link_id, TEACH_CHID);
				}
				//学生
				else if((szData[3] == 'A') && (!strcmp("2", szData + 3 + 1))) {

					flag[nPos] = 0;
					teachorstu[nPos] = 0;
					enc_enable_channel(gEduKit->encoderLink.encLink.link_id, STU_CHID);
				}
				//学生辅助
				else if((szData[3] == 'A') && (!strcmp("3", szData + 3 + 1))) {

					flag[nPos] = 0;
					teachorstu[nPos] = 2;
					enc_enable_channel(gEduKit->encoderLink.encLink.link_id, STUSIDE_CHID);
				} else {
					szData[0] = 0;
					szData[1] = 3;
					szData[2] = MSG_PASSWORD_ERR;
					//	PackHeaderMSG((BYTE *)szData, MSG_PASSWORD_ERR, 3);
					send(sSocket, szData, 3, 0);
					fprintf(stderr, "logo error!\n");
					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);

					flag[nPos] = -1;
					teachorstu[nPos] = -1;
					goto ExitClientMsg;   //
				}

				szData[0] = 0;
				szData[1] = 3;
				szData[2] = MSG_CONNECTSUCC;
				send(sSocket, szData, 3, 0);
				printf("send MSG_CONNECTSUCC");
				theDataInfo.nColors = 16;
				theDataInfo.nDataCodec = 0x1001;//JPEG?a 0x1001
				theDataInfo.nHight = 1920;
				theDataInfo.nWidth = 1088;
				theDataInfo.nFrameRate = 30;
				strcpy(theDataInfo.Version, "VGABOX");
				memcpy(szData + 3, &theDataInfo, sizeof(RecDataInfo));
				szData[0] = 0;
				szData[1] = (char)(3 + sizeof(theDataInfo));
				szData[2] = MSG_DATAINFO;
				send(sSocket, szData, 3 + sizeof(theDataInfo), 0);
				szData[0] = 0;
				szData[1] = (char)(3 + sizeof(SysParamsV2));
				szData[2] = MSG_SYSPARAMS;
				memcpy(sys2.szMacAddr, gSysParsm.szMacAddr, 8);
				sys2.dwAddr = gSysParsm.dwAddr;
				sys2.dwGateWay = gSysParsm.dwGateWay;
				sys2.dwNetMark = gSysParsm.dwNetMark;
				strcpy(sys2.strName, gSysParsm.strName);
				sys2.nFrame = gvideoPara.nFrameRate;
				sys2.nQuality = gvideoPara.nQuality;
				sys2.sBitrate = gvideoPara.sBitrate;
				sys2.nColors = gvideoPara.nColors;
				sys2.sCbr = gvideoPara.sCbr;
				strcpy(sys2.strVer, gSysParsm.strVer);
				memcpy(szData + 3, &sys2, sizeof(SysParamsV2));
				send(sSocket, szData, 3 + sizeof(SysParamsV2), 0);
				printf("user log!  DSP:%d  client:%d \n", DSP1, nPos);
				SETCLIUSED(DSP1, nPos, TRUE);
				SETCLILOGIN(DSP1, nPos, TRUE);
				printf("DSP:%d	 socket:%d\n", DSP1, GETSOCK(DSP1, nPos));
				printf("ISUSED=%d, ISLOGIN=%d\n", ISUSED(DSP1, nPos), ISLOGIN(DSP1, nPos));
				g_track_encode_info.is_encode = 1;
				g_stutrack_encode_info.is_encode = 1;
				g_stusidetrack_encode_info.is_encode = 1;

				break;
			}

			case MSG_SYSPARAMS: { //获取系统参数

				length = 3 + sizeof(gSysParsm);
				PackHeaderMSG((BYTE *)szData, MSG_SYSPARAMS, length);
				memcpy(szData + 3, &gSysParsm, sizeof(gSysParsm));
				send(sSocket, szData, length, 0);

				fprintf(stderr, "Get Sys Params ..........................\n");
			}
			break;

			case MSG_SETPARAMS: {	//设置系统参数

				fprintf(stderr, "Set Params! gSysParams %d Bytes\n", sizeof(SysParamsV2));
				unsigned char ParamBuf[200];
				SysParamsV2 *Newp;

				memcpy(ParamBuf, &szData[3], nMsgLen - 3);

				Newp = (SysParamsV2 *)&ParamBuf[0];


				fprintf(stderr, "==================================================\n");
				fprintf(stderr, "new:	nFrame    = %d\n", Newp->nFrame);
				fprintf(stderr, "new:	nQuality  = %d\n", Newp->nQuality);
				//fprintf(stderr, "new:	strName   = %s\n", Newp->strName);
				//fprintf(stderr, "new:	bType     = %d\n", Newp->bType);
				fprintf(stderr, "==================================================\n");

				//g_track_encode_info.track_status = 1;
				if(Newp->nFrame == 1) {
					SetVgaState();
				}

				//学生站立
				else if(Newp->nFrame == 2) {
					g_track_encode_info.track_status = 1;
					g_track_strategy_info.switch_cmd_author = AUTHOR_STUDENTS;
					g_track_strategy_info.send_cmd			= SWITCH_STUDENTS;
					StudentUp = 1;

				}
				//学生坐下
				else if(Newp->nFrame == 3) {
					g_track_encode_info.track_status = 1;
					g_track_strategy_info.switch_cmd_author = AUTHOR_STUDENTS;
					g_track_strategy_info.send_cmd			= SWITCH_TEATHER;
					StudentUp = 0;

				}
				//讲台
				else if(Newp->nFrame == 4) {
					g_track_encode_info.track_status = 1;
					g_track_strategy_info.move_flag = 0;
					g_track_strategy_info.blackboard_region_flag1 = 0;
					g_track_strategy_info.blackboard_region_flag2 = 0;
				} else  if(Newp->nFrame == 5) {
					g_track_encode_info.track_status = 1;
					g_track_strategy_info.move_flag = 1;
					g_track_strategy_info.blackboard_region_flag1 = 0;
					g_track_strategy_info.blackboard_region_flag2 = 0;
				}

				else  if(Newp->nFrame == 6) {
					g_track_encode_info.track_status = 1;
					g_track_strategy_info.move_flag = 0;
					g_track_strategy_info.blackboard_region_flag1 = 1;
					g_track_strategy_info.blackboard_region_flag2 = 0;
				} else  if(Newp->nFrame == 7) {
					g_track_encode_info.track_status = 1;
					g_track_strategy_info.move_flag = 0;
					g_track_strategy_info.blackboard_region_flag1 = 0;
					g_track_strategy_info.blackboard_region_flag2 = 1;
				}

				else  if(Newp->nFrame == 8) {

					g_track_encode_info.track_status = 0;
					g_track_strategy_info.position1_mv_flag = 0;
					g_track_strategy_info.move_flag = 0;
					g_track_strategy_info.blackboard_region_flag1 = 0;
					g_track_strategy_info.blackboard_region_flag2 = 1;
				} else  if(Newp->nFrame == 9) {

					g_track_encode_info.track_status = 0;
					g_track_strategy_info.position1_mv_flag = 1;
					g_track_strategy_info.move_flag = 0;
					g_track_strategy_info.blackboard_region_flag1 = 0;
					g_track_strategy_info.blackboard_region_flag2 = 0;
				} else if(Newp->nFrame == 10) {
					teacherTracerMove(Newp->nQuality);
				}
				else if(Newp->nFrame == 11)
				{
					//DelTimeOut(999, 0);
					//AddTimeOut(999, 0, 10);
					//gTrackState.FirstPic = 255;
					//teacherTracerMove(255);
					SwitchFirstPic(10,Newp->nQuality - 5);
					break;
				}



				if(Newp->nQuality == 5) {
					gMpMode = 1;
				} else {
					gMpMode = Newp->nQuality + 10;
				}



				//gMpMode = Newp->nQuality;
				//enc_set_fps(gEduKit.encLink.link_id, 0, (Newp->nFrame)*1000, (Newp->sBitrate)*1000);

			}
			break;

			case MSG_SAVEPARAMS:		//保存参数到flash
				fprintf(stderr, "Save Params!\n");
				//		SaveIPinfo(IPINFO_NAME, &gSysParaT);
				//		SaveSysParams(DSP1, CONFIG_NAME, &gSysParaT);
				break;

			case MSG_RESTARTSYS:
				fprintf(stderr, "Restart sys\n");
				system("reboot -f");
				break;

			case MSG_UpdataFile: { //升级过程
				int ret = 0 ;

				//		ret = DoUpdateProgram(sSocket,szData,nMsgLen);
				if(ret <= 1) {
					goto ExitClientMsg;
				}
			}
			break;

			case MSG_UPDATEFILE_ROOT:
				fprintf(stderr, "Updata Root file\n");
				break;

			case MSG_REQ_I:
				//				gReqIframe1 = 1;
				//				gReqIframe2 = 1;
				fprintf(stderr, "Request I Frame!\n");
				break;

			case MSG_PIC_DATA:
				//				SetPPTIndex(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				fprintf(stderr, "SetPPTIndex\n");
				break;

			case MSG_MUTE:
				//				SetNosound(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				fprintf(stderr, "NoSound!\n");
				break;

			case MSG_LOCK_SCREEN:
				//				LockScreen(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				break;

			case MSG_LOW_BITRATE:
				//				SetLowStream(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				//				SETLOWSTREAMFLAG(DSP1, nPos, 1);
				//				gLowStreamFlag = 1;
				break;

			case MSG_FIX_RESOLUTION:
				//				FixResolution(sSocket,nPos,(unsigned char *)(&szData[3]),nMsgLen-3);
				break;

			case MSG_SEND_INPUT:
				break;

			case MSG_SET_STUDENTS_TRACK_PARAM: {
				teachorstu[nPos] = 0;
				studentFlag = 1;
				char track_param[8192] = {0};
				memcpy(&track_param, &szData[3], nMsgLen - 3);
				enc_enable_channel(gEduKit->encoderLink.encLink.link_id, STU_CHID);
				set_students_track_param(track_param, sSocket);
				break;
			}

			case MSG_SET_TEACHER_TRACK_PARAM: {
				teachorstu[nPos] = 1;
				char track_param[8192] = {0};
				memcpy(&track_param, &szData[3], nMsgLen - 3);
				enc_enable_channel(gEduKit->encoderLink.encLink.link_id, TEACH_CHID);
				set_teacher_track_param(track_param, sSocket);
				break;
			}

			case MSG_SET_STUSIDETRACK_PARAM: {
				teachorstu[nPos] = 2;
				char track_param[8192] = {0};
				memcpy(&track_param, &szData[3], nMsgLen - 3);
				enc_enable_channel(gEduKit->encoderLink.encLink.link_id, STUSIDE_CHID);
				track_students_right_side_param(track_param, sSocket);
				break;
			}

			default:
				fprintf(stderr, "Error Cmd=%d!\n", &szData[3]);
				break;
		}

	}


ExitClientMsg:
	fprintf(stderr, "\n|||||||| go down  |||||||||\n\n");
	fprintf(stderr, " socket = %d %d\n", GETSOCK(DSP1, nPos), sSocket);

	SETCLIUSED(DSP1, nPos, FALSE);
	SETSOCK(DSP1, nPos, INVALID_SOCKET);
	SETLOGINTYPE(DSP1, nPos, LOGIN_USER);
	SETCLILOGIN(DSP1, nPos, FALSE);
	SETPPTINDEX(DSP1, nPos, FALSE);
	SETPPTQUERY(DSP1, nPos, FALSE);
	SETLOWSTREAM(DSP1, nPos, 0);

	if(GETLOWSTREAMFLAG(DSP1, nPos)) {
		SETLOWSTREAMFLAG(DSP1, nPos, 0);
		//		gLowStreamFlag = 0;
	}

	close(sSocket);

	if((flag[nPos] == 0) && (teachorstu[nPos] == 0)) {
		studentFlag = 0;
	}

	flag[nPos] = -1;
	teachorstu[nPos] = -1;


	pthread_detach(pthread_self());
	pthread_exit(0);
}
#if 0
static void EncoderUspServer(void *pParam)
{
	int						sSocket;
	struct sockaddr_in			SrvAddr;
	struct timeval				tv;
	WhoIsMsg					theMsg;

	int 						nLen, size;

	bzero(&SrvAddr, sizeof(struct sockaddr_in));
	SrvAddr.sin_family = AF_INET;
	SrvAddr.sin_port = htons(PORT_UDP_SERVER_LISTEN);
	SrvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	sSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(sSocket < 0) {
		fprintf(stderr, "DSP1UDPTask create error:%d\n", errno);
		goto WhoIsExit;
	}

	if(bind(sSocket, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0) {
		fprintf(stderr, "bind terror:%d\n", errno);
		goto WhoIsExit;
	}

	nLen = 1;
	tv.tv_sec = 1;
	tv.tv_usec = 10000;
	setsockopt(sSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));

	if(setsockopt(sSocket, SOL_SOCKET, SO_BROADCAST, (void *)&nLen, sizeof(nLen))) {
		fprintf(stderr, "setsockopt SO_BROADCAST error:%d\n", errno);
		goto WhoIsExit;
	}

	fprintf(stderr, "who is task is ready!\n");

	while(gRunStatus) {
		memset(&theMsg, 0, sizeof(theMsg));
		size = sizeof(SrvAddr);
		nLen = recvfrom(sSocket, &theMsg, sizeof(theMsg), 0, (struct sockaddr *)&SrvAddr, (socklen_t *)&size);

		if(nLen < 0) {
			//printf("recvfrom() Error Len:%d  errno:%d\n ",nLen,errno);
			continue;
		}

		if((theMsg.szFlag[0] == 0x7e)
		   && (theMsg.szFlag[1] == 0x7e)
		   && (theMsg.szFlag[2] == 0x7e)
		   && (theMsg.szFlag[3] == 0x7e)) {
			memcpy(theMsg.szFlag + 4, &gSysParsm, sizeof(theMsg) - 4);
			theMsg.nType = gSysParsm.bType;  //6---ENC1200  8--ENC-1100
			theMsg.szFlag[0] = 0x7e;
			theMsg.szFlag[1] = 0x7e;
			theMsg.szFlag[2] = 0x7e;
			theMsg.szFlag[3] = 0x7e;
			strcpy(theMsg.strVer, gSysParsm.strVer);
			strcpy(theMsg.strName, gSysParsm.strName);
			sendto(sSocket, &theMsg, sizeof(theMsg), 0, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr));
			fprintf(stderr, "sendto\n");
		}
	}

WhoIsExit:
	fprintf(stderr, "DSP1UDPTask \n");

	pthread_detach(pthread_self());
	close(sSocket);
}
#endif


static int EncoderServerThread()
{
	struct sockaddr_in		SrvAddr, ClientAddr;
	int					sClientSocket = -1;
	int					ServSock = -1;

	pthread_t				client_threadid[MAX_CLIENT] = {0};

	short					sPort					= PORT_LISTEN_DSP1;
	void					*ret 					= 0;

	int 					clientsocket				= 0;
	int 					nLen					= 0;;
	int 					ipos						= 0;
	int 					fileflags					= 0;
	int 					opt 					= 1;

	InitClient(DSP1);
	sem_init(&g_sem, 0, 0);

SERVERSTARTRUN:

	bzero(&SrvAddr, sizeof(struct sockaddr_in));
	SrvAddr.sin_family = AF_INET;
	SrvAddr.sin_port = htons(sPort);
	SrvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	ServSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(ServSock < 0) {
		fprintf(stderr, "ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		gRunStatus = 0;
		return -1;
	}

	setsockopt(ServSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(ServSock, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0) {
		fprintf(stderr, "bind terror:%d,error msg: = %s\n", errno, strerror(errno));
		gRunStatus = 0;
		return -1;
	}

	if(listen(ServSock, 10) < 0) {
		fprintf(stderr, "listen error:%d,error msg: = %s", errno, strerror(errno));
		gRunStatus = 0;
		return -1;
	}

	if((fileflags = fcntl(ServSock, F_GETFL, 0)) == -1) {
		fprintf(stderr, "fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		gRunStatus = 0;
		return -1;
	}


	nLen = sizeof(struct sockaddr_in);

	while(gRunStatus) {
		memset(&ClientAddr, 0, sizeof(struct sockaddr_in));

		nLen = sizeof(struct sockaddr_in);
		sClientSocket = accept(ServSock, (void *)&ClientAddr, (DWORD *)&nLen);

		fprintf(stderr, "\n\n\naccept a valid connect!!!!-----------------, socket = %d\n\n\n\n", sClientSocket);

		if((sClientSocket > 0) && (gEduKit->Start == 1)) {

			int nPos = 0;
			ClearLostClient(DSP1);
			nPos = GetNullClientData(DSP1);

			if(-1 == nPos) {
				char chData[20];
				chData[0] = 0;
				chData[1] = 3;
				chData[2] = MSG_MAXCLIENT_ERR;
				send(sClientSocket, chData, 3, 0);
				close(sClientSocket);
				fprintf(stderr, "MAXCLIENT ERR!\n");
			} else {
				int nSize = 0;
				int result;
				int *pnPos = malloc(sizeof(int));

				/* set client used */
				SETCLIUSED(DSP1, nPos, TRUE);
				SETSOCK(DSP1, nPos, sClientSocket) ;

				nSize = 1;

				if((setsockopt(sClientSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize,
				               sizeof(nSize))) == -1) {
					fprintf(stderr, "setsockopt failed");
				}

				nSize = 0;
				nLen = sizeof(nLen);
				result = getsockopt(sClientSocket, SOL_SOCKET, SO_SNDBUF, &nSize , (DWORD *)&nLen);

				if(result) {
					fprintf(stderr, "getsockopt() errno:%d socket:%d  result:%d\n", errno, sClientSocket, result);
				}

				nSize = 1;

				if(setsockopt(sClientSocket, IPPROTO_TCP, TCP_NODELAY, &nSize , sizeof(nSize))) {
					fprintf(stderr, "Setsockopt error%d\n", errno);
				}

				fprintf(stderr, "Clent:%s connected,nPos:%d socket:%d!\n", inet_ntoa(ClientAddr.sin_addr), nPos, sClientSocket);
				*pnPos = nPos;

#ifdef ENC110
				result = pthread_create(&client_threadid[nPos], NULL, (void *)EncoderProcess110, (void *)pnPos);
#else
				result = pthread_create(&client_threadid[nPos], NULL, (void *)EncoderProcess, (void *)pnPos);
#endif

				if(result) {
					close(sClientSocket);	//
					fprintf(stderr, "creat pthread ClientMsg error  = %d!\n" , errno);
					continue;
				}

				sem_wait(&g_sem);
				fprintf(stderr, "sem_wait() semphone inval!!!  result = %d\n", result);
			}
		} else {

			if(sClientSocket > 0) {
				close(sClientSocket);
				printf("close sClientSocket socket!!! %d\n", sClientSocket);
				sClientSocket = -1;
			}

			if(errno == ECONNABORTED || errno == EAGAIN)
				//软件原因中断
			{
				usleep(100000);
				continue;
			}

			if(ServSock > 0) {
				printf("close enclive socket!!! %d\n", ServSock);
				close(ServSock);
				ServSock = -1;
				sleep(1);
			}

			goto SERVERSTARTRUN;
		}

	}

	fprintf(stderr, "begin exit the DSP1TCPTask \n");

	fprintf(stderr, "exit the drawtimethread \n");

	for(ipos = 0; ipos < MAX_CLIENT; ipos++) {
		if(client_threadid[ipos]) {
			clientsocket = GETSOCK(DSP1, ipos);

			if(clientsocket != INVALID_SOCKET) {
				close(clientsocket);
				SETSOCK(DSP1, ipos, INVALID_SOCKET);
			}

			if(pthread_join(client_threadid[ipos], &ret) == 0) {
			}
		}
	}

	fprintf(stderr, "close the encoder server socket \n");

	if(ServSock > 0) {
		fprintf(stderr, "close gserv socket \n");
		close(ServSock);
	}

	ServSock = 0;

	return 0;
}



int StartEncoderMangerServer()
{
	int result;
	pthread_t	serverThid;

	pthread_mutex_init(&g_send_m, NULL);

	InitStandardParam(&gSysParsm);
	InitAudioParam(&gAudioParam);
	InitVideoParam(&gvideoPara);
	result = pthread_create(&serverThid, NULL, (void *)EncoderServerThread, (void *)NULL);

	if(result < 0) {
		fprintf(stderr, "create EncoderServerThread() failed\n");
		return -1;
	}

	return 0;
}



