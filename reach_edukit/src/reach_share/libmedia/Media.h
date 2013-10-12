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
	//#ultrafast、superfast、veryfast、faster、fast、medium、slow、slower、veryslow、placebo
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
		int iStreamIdx; //从文件中读出帧的流索引，从0开始
		int bAudio;     //表示此帧是音频还是视频，为1时是音频，否则是视频
		int bKeyFrame;  //表示此帧是否是关键帧
		int64_t pts;    //表示此帧的pts,单位是ms
		int iLen;       //表示帧长
		uint8_t * pData;//帧的数据指针
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
	功能:支技多路视频和一路音频mp4或者asf文件读和写，并且支持只写音频的mp4和asf文件，同时支持windows和linux平台.
	需要注意的问题：
	1. VideoStreamAdd，AudioStreamAdd 添加的流，如果添加成功，会返回流索引，这个流索引在调用MediaWriteFrame写入时使用，即 iStreamIdx
	2. 应用需要保证调用MediaWriteFrame 时给的第一路流的pts 是递增的，不然此帧写不进去
	*/
	//注册ffmpeg format和codec
	MEDIA_API void MediaSysInit(); 
	
	//媒体文件复用结构初化化
	MEDIA_API void MediaWriterInit(MuxWriter * pMuxWriter);
	//file 写媒体文件名称, bHaveVideo 表示创建的媒体文件是否有视频
	MEDIA_API int MediaFileCreate(MuxWriter * pMuxWriter,char * file,int bHaveVideo);
	//视频流添加，返回值即为此流的索引，如果是创建多视频流文件，需要多次调用，codec_id 编码视频ID,h.264是CODEC_ID_H264,frame_rate 设置为 STREAM_FRAME_RATE
	//gop 是I帧间隔，profile,表示h.264的profile,preset表示 x264 预置参数
	MEDIA_API int VideoStreamAdd(MuxWriter * pMuxWriter,enum CodecID codec_id,int width,int height,int bitrate,int frame_rate);
	//音频流添加，返回值即为此流的索引
	MEDIA_API int AudioStreamAdd(MuxWriter * pMuxWriter,enum CodecID codec_id,int sample_rate,int bit_rate,int channel,enum AVSampleFormat sample_fmt);
	//写帧函数，pts 单位是ms, bAudio 表示是否是音频
	MEDIA_API int MediaWriteFrame(MuxWriter * pMuxWriter,uint8_t * pBuf,int iBufLen,int64_t pts,int iStreamIdx,int bKeyFrame,int bAudio);
	//媒体文件写尾，在写完所有帧后，最后调用
	MEDIA_API int MediaWriteTrailer(MuxWriter * pMuxWriter);
	MEDIA_API void MediaWriterClose(MuxWriter * pMuxWriter);
	
	
	//媒体文件解复用初始化
	MEDIA_API void MediaReaderInit(DemuxReader * pDemuxReader);
	//file 输入的文件名
	MEDIA_API int MediaOpenFile(DemuxReader * pDemuxReader,char * file);
	//pMediaPkt 是传入的 MediaPacket 结构指针，函数内部会分配内存来保存读的帧，所以如果调用成功，使用完后，需要释放pMediaPkt->pData指向的内存
	MEDIA_API int MediaReadFrame(DemuxReader * pDemuxReader,MediaPacket * pMediaPkt);
	//iSeekTime 文件绝对时间，单位是ms
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
