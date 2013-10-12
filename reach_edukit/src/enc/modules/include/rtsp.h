/*
**************************************************************************************
    
**************************************************************************************
*/

/*是否开启打印信息*/

#if 0
#ifndef _RTSP_H__
#define _RTSP_H__
#define PRINT_DEUBG

enum{
	INVALID_CLIENT = -1,
	VLC_CLIENT = 0,
	QT_CLIENT,
	STB_TS_CLIENT,
	DEFAULT_CLIENT
};


typedef struct _DataHeader110E
{
	unsigned int id;
	unsigned int biWidth;							//VGA：Width		Audio：SampleRate
	unsigned int biHeight;							//VGA：Height	Audio：BitRate
	unsigned int biBitCount;							//VGA：BitCount	Audio：Channel
	unsigned int fccCompressedHandler;				//VGA：mmioFOURCC (‘H’,’2’,’6’,’4’)
											    //Audio：mmioFOURCC(‘A’,’D’,’T’,’S’)
	unsigned int  fccHandler;	   						//Audio：SampleBit
	unsigned int dwCompressedDataLength;
	unsigned int dwFlags;
	unsigned int  dwSegment;
	unsigned int  dwPacketNumber;
}DataHeader_110E;

int RtspClientIsNull(void);

int RtspVideoPack(int nLen, unsigned char * pData, int nFlag,unsigned int timetick,int *seq,int roomid,int idr_flag);
int RtspAudioPack(int nLen,	unsigned char *pData,unsigned int timetick, int roomid,unsigned int samplerate);

//void RtspTask(void);
void rtsp_module_init(void);
int RtspGetlocalip(unsigned int local_ip);
char *RtspGetVersion(void);
//int RtspGetPort(void);
void rtsp_change_sdp_info( int roomid);

int rtsp_get_local_ip(char *buff,int len);
void rtsp_close_all_client(void );
int RtspAdudioSeqReset(void);

int rtsp_stream_rebuild_audio_frame(unsigned char *buff,unsigned int len ,int id);
int rtsp_stream_rebuild_video_frame(unsigned char *buff,unsigned int len ,int id);
int rtsp_stream_rebuild_video_frame_110e(unsigned char *buff,unsigned int len ,int id,DataHeader_110E *param);
int rtsp_stream_rebuild_audio_frame_110e(unsigned char *buff,unsigned int len ,int id,DataHeader_110E *param);

#endif

#endif