#ifndef __MEDIEA_H__
#define __MEDIEA_H__
#define __STDC_CONSTANT_MACROS
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
	
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#define MAX_PATH 260
#endif

#ifdef WIN32

#ifdef MEDIALIB_EXPORTS
#define MEDIA_API  __declspec (dllexport)
#else
#define MEDIA_API	__declspec (dllimport)
#endif

#else
#include <unistd.h>
#include <time.h>
#define MEDIA_API 
#endif

	
#define STREAM_FRAME_RATE 1000
#define DEFAULT_AUDIO_SAMPLE_RATE 44100
#define MAX_STREAM_NUM 128
	
	enum H264_PROFILE
	{
		PROFILE_BASELINE = 1,
		PROFILE_MAIN,
		PROFILE_HIGH
	};
	//#ultrafast��superfast��veryfast��faster��fast��medium��slow��slower��veryslow��placebo
	enum PRESET
	{
		PRESET_ULTRAFAST = 1,
		PRESET_SUPERFAST,
		PRESET_VERYFAST,
		PRESET_FASTER,
		PRESET_FAST,
		PRESET_MEDIUM,
		PRESET_SLOW,
		PRESET_SLOWER,
		PRESET_VERYSLOW,
		PRESET_PLACEBO
	};
	
	typedef struct _MuxWriter
	{
		AVFormatContext * oc;
		AVBitStreamFilterContext * bsfc;
		AVCodecContext * pAudioCodecCtx;
		AVCodecContext * apVideoCodecCtx[MAX_STREAM_NUM];
		enum CodecID eAudioCodecID;
		int iAudioTimeScale;
		enum CodecID eVideoCodecID;
		int iVideoTimeScale;
		char acFileName[MAX_PATH];
		int bMp4File;
		int bAsfFile;
		int abGetSyncFrame[MAX_STREAM_NUM];
		int bStartWriteFrame;
		int bHaveVideo;
#ifdef WIN32
		CRITICAL_SECTION cs;
#else
		pthread_mutex_t mutex;
#endif
	}MuxWriter;
	
	
	typedef struct _DemuxReader
	{
		AVFormatContext * ic;
		AVBitStreamFilterContext * aBsfc[MAX_STREAM_NUM];
		AVCodecContext * apVideoCodecCtx[MAX_STREAM_NUM];
		AVCodecContext * pAudioCodecCtx;
		int iVideoNum;
		int aiWidth[MAX_STREAM_NUM];
		int aiHeight[MAX_STREAM_NUM];
		int iVideoStreamIdx;
		enum CodecID eAudioCodecID;
		int iAudioStreamIdx;
		int iSampleRate;
		int iChannel;
		int iHE_AAC;
		enum AVSampleFormat eAudioFmt;
		int iBitRate;
		int64_t iDuration;
		int bStreamEnd;
		int bMp4H264BitStream;
		int bNeedAACAdtsHeader;
#ifdef WIN32
		CRITICAL_SECTION cs;
#else
		pthread_mutex_t mutex;
#endif
	}DemuxReader;
	
	typedef struct _MediaPacket
	{
		int iStreamIdx; //���ļ��ж���֡������������0��ʼ
		int bAudio;     //��ʾ��֡����Ƶ������Ƶ��Ϊ1ʱ����Ƶ����������Ƶ
		int bKeyFrame;  //��ʾ��֡�Ƿ��ǹؼ�֡
		int64_t pts;    //��ʾ��֡��pts,��λ��ms
		int iLen;       //��ʾ֡��
		uint8_t * pData;//֡������ָ��
	}MediaPacket;


#define MAX_AUDIO_PCM_BUF (4 * AVCODEC_MAX_AUDIO_FRAME_SIZE)
#define MAX_AUDIO_ES_LEN  (32 * 1024)
#define MAX_VIDEO_ES_LEN  (512 * 1024)


	typedef struct _TransContext
	{
		DemuxReader DemuxRdr;
		int bOpenInput;
		MuxWriter MuxWtr;
		int bOpenOutput;


		AVCodecContext * pVideoDecCtx;
		enum CodecID eVideoDecID;
		int iDecVideoStreamIdx;
		int iDecOutWidth,iDecOutHeight;
		AVCodecContext * pVideoEncCtx;
		enum CodecID eVideoEncID;
		int iEncVideoStreamIdx;
		double dVideoPts;
		int iEncWitdth;
		int iEncHeight;
		int bGetVideoDecFrame;
		AVFrame VideoDecFrame;
		AVFrame * pVideoEncFrame;
		struct SwsContext * img_convert_ctx;
	


		AVCodecContext * pAudioDecCtx;
		AVCodecContext * pAudioEncCtx;
		enum CodecID eAudioDecID;
		enum CodecID eAudioEncID;
		int iBitRate;
		int iSampleRate;
		int iChannels;
		enum AVSampleFormat eSampleFmt;
		double dAudioPts;
		double dAudioStartPts;
		int bGetEnoughAudioPcm;
		uint8_t  ucAudioPcmBuf[MAX_AUDIO_PCM_BUF];
		int iPcmLen;
		int iDecAudioStreamIdx;
		int iEncAudioStreamIdx;

	}TransContext;


	/*
	����:֧����·��Ƶ��һ·��Ƶmp4����asf�ļ�����д������֧��ֻд��Ƶ��mp4��asf�ļ���ͬʱ֧��windows��linuxƽ̨.
	��Ҫע������⣺
	1. VideoStreamAdd��AudioStreamAdd ��ӵ����������ӳɹ����᷵��������������������ڵ���MediaWriteFrameд��ʱʹ�ã��� iStreamIdx
	2. Ӧ����Ҫ��֤����MediaWriteFrame ʱ���ĵ�һ·����pts �ǵ����ģ���Ȼ��֡д����ȥ
	*/
	//ע��ffmpeg format��codec
	MEDIA_API void MediaSysInit(); 
	
	//ý���ļ����ýṹ������
	MEDIA_API void MediaWriterInit(MuxWriter * pMuxWriter);
	//file дý���ļ�����, bHaveVideo ��ʾ������ý���ļ��Ƿ�����Ƶ
	MEDIA_API int MediaFileCreate(MuxWriter * pMuxWriter,char * file,int bHaveVideo);
	//��Ƶ����ӣ�����ֵ��Ϊ����������������Ǵ�������Ƶ���ļ�����Ҫ��ε��ã�codec_id ������ƵID,h.264��CODEC_ID_H264,frame_rate ����Ϊ STREAM_FRAME_RATE
	//gop ��I֡�����profile,��ʾh.264��profile,preset��ʾ x264 Ԥ�ò���
	MEDIA_API int VideoStreamAdd(MuxWriter * pMuxWriter,enum CodecID codec_id,int width,int height,int bitrate,int frame_rate);
	//��Ƶ����ӣ�����ֵ��Ϊ����������
	MEDIA_API int AudioStreamAdd(MuxWriter * pMuxWriter,enum CodecID codec_id,int sample_rate,int bit_rate,int channel,enum AVSampleFormat sample_fmt);
	//д֡������pts ��λ��ms, bAudio ��ʾ�Ƿ�����Ƶ
	MEDIA_API int MediaWriteFrame(MuxWriter * pMuxWriter,uint8_t * pBuf,int iBufLen,int64_t pts,int iStreamIdx,int bKeyFrame,int bAudio);
	//ý���ļ�дβ����д������֡��������
	MEDIA_API int MediaWriteTrailer(MuxWriter * pMuxWriter);
	MEDIA_API void MediaWriterClose(MuxWriter * pMuxWriter);
	
	
	//ý���ļ��⸴�ó�ʼ��
	MEDIA_API void MediaReaderInit(DemuxReader * pDemuxReader);
	//file ������ļ���
	MEDIA_API int MediaOpenFile(DemuxReader * pDemuxReader,char * file);
	//pMediaPkt �Ǵ���� MediaPacket �ṹָ�룬�����ڲ�������ڴ����������֡������������óɹ���ʹ�������Ҫ�ͷ�pMediaPkt->pDataָ����ڴ�
	MEDIA_API int MediaReadFrame(DemuxReader * pDemuxReader,MediaPacket * pMediaPkt);
	//iSeekTime �ļ�����ʱ�䣬��λ��ms
	MEDIA_API int MediaSeek(DemuxReader * pDemuxReader,int iSeekTime);
	MEDIA_API void MediaReaderClose(DemuxReader * pDemuxReader);
	
	MEDIA_API void MediaPacketFree(MediaPacket * pkt);
	
	MEDIA_API AVStream *add_audio_stream(AVFormatContext *oc, enum CodecID codec_id,int sample_rate,int bit_rate,int channel,enum AVSampleFormat sample_fmt);

	MEDIA_API int MediaTranscodeInit(TransContext * pTransContext,char * InputFile,char * OutputFile);
	MEDIA_API int MediaTranscodeClose(TransContext * pTransContext);

	MEDIA_API int AudioDecInfoInit(TransContext * pTransContext,enum CodecID eCodecID,int samplerate,int bitrate,int channels,enum AVSampleFormat eFmt);
	MEDIA_API int AudioEncInfoInit(TransContext * pTransContext,enum CodecID eCodecID,int samplerate,int bitrate,int channels,enum AVSampleFormat eFmt);
	MEDIA_API int AudioDeocde(TransContext * pTransContext,MediaPacket * InPkt);
	MEDIA_API int AudioEncode(TransContext * pTransContext,MediaPacket * OutPkt);


	MEDIA_API int VideoDecInfoInit(TransContext * pTransContext,enum CodecID eCodecID,int iSrcWidth,int iSrcHeight,int iOutWidth,int iOutHeight);
	MEDIA_API int VideoEncInfoInit(TransContext * pTransContext,enum CodecID eCodecID,int width,int height,int bitrate,int fps,int gop,int profile,int preset);
	MEDIA_API int VideoDeocde(TransContext * pTransContext,MediaPacket * InPkt);
	MEDIA_API int VideoEncode(TransContext * pTransContext,MediaPacket * OutPkt);

	MEDIA_API int h264_encode_para_set(AVCodecContext * c,int gop,int profile,int preset);

	MEDIA_API int64_t Mp4Recovery(char * RecoveryFile,char * InFile,char * OutFile);
	MEDIA_API int MediaGenIndexImage(char * InFile,char * image,int iInternal,int iStartNum,int iMaxImageNum);//iStartNum need ffmpeg 1.0 

#ifdef __cplusplus
}
#endif


#endif
