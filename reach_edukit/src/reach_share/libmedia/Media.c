#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Media.h"


#ifdef WIN32
#include <fcntl.h>
#define fseek(f,p,w) _fseeki64((f), (p), (w))
#else
#define fseek(f,p,w) fseeko((f), (p), (w))
#endif


#ifdef WIN32
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib,"winmm.lib")
#endif

int bFFmpegInit = 0;

AVStream *add_audio_stream(AVFormatContext *oc, enum CodecID codec_id,int sample_rate,int bit_rate,int channel,enum AVSampleFormat sample_fmt)
{
    AVCodecContext *c;
	AVCodec * pAVCodec;
    AVStream *st;
	int iRet;
	
    st = avformat_new_stream(oc, NULL);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
        return NULL;
    }
	
    c = st->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_AUDIO;
	
    c->sample_fmt = sample_fmt;
    c->bit_rate = bit_rate ;
    c->sample_rate = sample_rate;
    c->channels = channel;
	c->frame_size = 1024;//cw
	
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	

    return st;
}

int h264_encode_para_set(AVCodecContext * c,int gop,int profile,int preset)
{
	char * pProfile;
	char * pPreset;
	
	if (!c)
		return -1;

	c->gop_size = gop >0 ? gop :180;
	if (profile > 0)
	{
		switch(profile)
		{
		case PROFILE_BASELINE:
			pProfile = "baseline";
			break;
		case PROFILE_MAIN:
			pProfile = "main";
			break;
		case PROFILE_HIGH:
			pProfile = "high";
			break;
		default:
			pProfile = "high";
		}
		av_opt_set(c->priv_data,"profile",pProfile,0);//chenw
	}
	if (preset > 0)
	{
		switch(preset)
		{
		case PRESET_ULTRAFAST:
			pPreset = "ultrafast";
			break;
		case PRESET_SUPERFAST:
			pPreset = "superfast";
			break;
		case PRESET_VERYFAST:
			pPreset = "veryfast";
			break;
		case PRESET_FASTER:
			pPreset = "faster";
			break;
		case PRESET_FAST:
			pPreset = "fast";
			break;
		case PRESET_MEDIUM:
			pPreset = "medium";
			break;
		case PRESET_SLOW:
			pPreset = "slow";
			break;
		case PRESET_SLOWER:
			pPreset = "slower";
			break;
		case PRESET_PLACEBO:
			pPreset = "placebo";
			break;
		default:
			pPreset = "medium";
			break;
		}
		av_opt_set(c->priv_data,"preset",pPreset,0);//chenw
	}
	return 0;
}
AVStream *add_video_stream(AVFormatContext *oc, enum CodecID codec_id,int width,int height,int bitrate,int frame_rate)
{
    AVCodecContext *c;
	AVCodec * pAVCodec = NULL;
    AVStream *st;
	int iRet;
	
	
// 	pAVCodec = avcodec_find_encoder(codec_id);
// 	if (pAVCodec == NULL)
// 	{
// 		printf("Can't find encoder %d!\n",codec_id);
// 		return NULL;
// 	}
    st = avformat_new_stream(oc,NULL);
    if (!st) {
        fprintf(stderr, "Could not alloc stream\n");
        return NULL;
    }
	
    c = st->codec;
    c->codec_id = codec_id;
    c->codec_type = AVMEDIA_TYPE_VIDEO;
    c->width = width;
    c->height = height;
	
    c->time_base.den = STREAM_FRAME_RATE;
    c->time_base.num = STREAM_FRAME_RATE/frame_rate;
	c->pix_fmt = PIX_FMT_YUV420P;
	c->bit_rate = bitrate;
	//c->rc_lookahead = 40;
    c->thread_count = 0;
	
	c->qmin = 10;
	c->qmax = 50;//防止编码第一帧图像质量太差
	
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	
    return st;
}

int write_one_frame(AVFormatContext * oc,AVPacket * pkt)
{
	if(av_interleaved_write_frame(oc, pkt) < 0)
	{
		printf("stream index %d av_interleaved_write_frame fail! \n",pkt->stream_index);
		return -1;
	}
	return 0;
}


int create_media_file(AVFormatContext ** oc,char * OutputFile)    
{
	avformat_alloc_output_context2(oc, NULL, NULL, OutputFile);
    if (!*oc) 
	{
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(oc, NULL, "mpeg", OutputFile);
    }
    if (!*oc) 
	{
        return -1;
    }
	return 0;
}

void MediaSysInit()
{
	if (!bFFmpegInit)
	{
		av_register_all();
		avcodec_register_all();
		avformat_network_init();
		av_log_set_level(AV_LOG_ERROR);
		bFFmpegInit = 1;
	}
}

void MediaWriterInit(MuxWriter * pMuxWriter)
{
	int i;
	
	pMuxWriter->bAsfFile = 0;
	pMuxWriter->bMp4File = 0;
	pMuxWriter->eAudioCodecID = CODEC_ID_NONE;
	pMuxWriter->bsfc = NULL;
	for (i = 0;i < MAX_STREAM_NUM;i++)
	{
		pMuxWriter->abGetSyncFrame[i] = 0;
		pMuxWriter->apVideoCodecCtx[i] = NULL;
	}
	pMuxWriter->pAudioCodecCtx = NULL;
	pMuxWriter->eVideoCodecID = CODEC_ID_NONE;
	
	pMuxWriter->oc = NULL;
	pMuxWriter->bHaveVideo = 1;
	pMuxWriter->bStartWriteFrame = 0;
#ifdef WIN32
	InitializeCriticalSection(&pMuxWriter->cs);
#else
	pthread_mutex_init(&pMuxWriter->mutex,NULL);
#endif
}
#ifdef WIN32
int ANSIToUTF8(char *pszCode, char *UTF8code)
{
	WCHAR Unicode[MAX_PATH]={0}; 
	char utf8[MAX_PATH]={0};
	
	// read char Length
	int nUnicodeSize = MultiByteToWideChar(CP_ACP, 0, pszCode, strlen(pszCode), Unicode, sizeof(Unicode)); 
	
	// read UTF-8 Length
	int nUTF8codeSize = WideCharToMultiByte(CP_UTF8, 0, Unicode, nUnicodeSize, UTF8code, sizeof(Unicode), NULL, NULL); 
	
	// convert to UTF-8 
	MultiByteToWideChar(CP_UTF8, 0, utf8, nUTF8codeSize, Unicode, sizeof(Unicode)); 
	return nUTF8codeSize;
}
#endif

int MediaFileCreate(MuxWriter * pMuxWriter,char * file,int bHaveVideo)
{
	int iRet;
	char * pStr;
	int ch = '.';
	char acUtf8FileName[MAX_PATH];

	memset(acUtf8FileName,0,MAX_PATH);
#ifdef WIN32
	ANSIToUTF8(file,acUtf8FileName);
#else
	strcpy(acUtf8FileName,file);
#endif
	pMuxWriter->oc = NULL;
	iRet = create_media_file(&pMuxWriter->oc,acUtf8FileName);
	if (iRet < 0)
		return iRet;
	
	pStr = strrchr(file,'.');
	if (pStr == NULL)
	{
		printf("file is not specify suffix!\n");
		return -1;
	}
	
	if (strstr(pStr,".mp4") || strstr(pStr,".3gp") || strstr(pStr,".mov"))
	{
		pMuxWriter->bMp4File = 1;
	}
	else if (strstr(pStr,".asf") || strstr(pStr,".wmv") || strstr(pStr,".wma"))
	{
		pMuxWriter->bAsfFile = 1;
	}
	pMuxWriter->bHaveVideo = bHaveVideo;
	strncpy(pMuxWriter->acFileName,file,MAX_PATH);
	av_dump_format(pMuxWriter->oc, 0, file, 1);
	if(!(pMuxWriter->oc->oformat->flags & AVFMT_NOFILE))
	{
		if(avio_open(&pMuxWriter->oc->pb, acUtf8FileName, AVIO_FLAG_WRITE) < 0)
		{
			fprintf(stderr, "Could not open '%s'\n", pMuxWriter->acFileName);
			return -1;	
		}
	}
	return 0;
}

int VideoStreamAdd(MuxWriter * pMuxWriter,enum CodecID codec_id,int width,int height,int bitrate,int frame_rate)
{
	AVStream * st;
	if (!pMuxWriter->bHaveVideo)
	{
		printf("warning output is should no video!\n");
	}
	st =  add_video_stream(pMuxWriter->oc,codec_id,width,height,bitrate,frame_rate);
	if (st == NULL)
	{
		return -1;
	}
	else
	{
		pMuxWriter->eVideoCodecID = codec_id;
		if (pMuxWriter->bMp4File)
		{
			pMuxWriter->iVideoTimeScale = STREAM_FRAME_RATE;
		}
		else
		{
			pMuxWriter->iVideoTimeScale = st->time_base.den  / st->time_base.num;
		}
		pMuxWriter->apVideoCodecCtx[st->index] = st->codec;
		return st->index;
	}
}

int AudioStreamAdd(MuxWriter * pMuxWriter,enum CodecID codec_id,int sample_rate,int bit_rate,int channel,enum AVSampleFormat sample_fmt)
{
	AVStream * st;
	int iTimeScale;
	
	st = add_audio_stream(pMuxWriter->oc,codec_id,sample_rate,bit_rate,channel,sample_fmt);
	if (st == NULL)
	{
		return -1;
	}
	else
	{
		pMuxWriter->eAudioCodecID = codec_id;
		if (codec_id == CODEC_ID_AAC)
		{
			pMuxWriter->bsfc = av_bitstream_filter_init("aac_adtstoasc");
		}
		pMuxWriter->pAudioCodecCtx = st->codec;
		if (pMuxWriter->bMp4File)
		{
			iTimeScale = st->codec->sample_rate;
		}
		else
		{
			iTimeScale = st->time_base.den /st->time_base.num;
		}
		pMuxWriter->iAudioTimeScale = iTimeScale;
		return st->index;
	}
}
int h264_split(uint8_t *buf, int buf_size)
{
    int i;
    uint32_t state = -1;
    int has_sps= 0;
	
    for(i=0; i<=buf_size; i++){
        if((state&0xFFFFFF1F) == 0x107)
            has_sps=1;
        if((state&0xFFFFFF00) == 0x100 && (state&0xFFFFFF1F) != 0x107 && (state&0xFFFFFF1F) != 0x108 && (state&0xFFFFFF1F) != 0x109){
            if(has_sps){
                while(i>4 && buf[i-5]==0) i--;
                return i-4;
            }
        }
        if (i<buf_size)
            state= (state<<8) | buf[i];
    }
    return 0;
}
int RemoveSpsPps(int8_t * pBuf,int iLen)
{
	int i;
	int iSps;
	int iGetSpsOnceTime = 0 ;
	int iOffset = 0;
	
	if (!pBuf || iLen <=0 )
	{
		return 0;
	}
	for (i = 0;i < iLen;i++)
	{
		if (pBuf[i] == 0 && pBuf[i + 1] == 0 && pBuf[i + 2] == 1)
		{
			iSps = pBuf[i + 3] & 0x1f;
			switch(iSps)
			{
			case 7:
				if (!iGetSpsOnceTime)
					iGetSpsOnceTime = 1;
				else
				{
					if (pBuf[i - 1] == 0) 
						return i - 1;
					else
						return i;
				}
				break;
			case 5:
			case 1:
				return 0;
			}
		}
	}
	return 0;
}
int MediaWriteFrame(MuxWriter * pMuxWriter,uint8_t * pBuf,int iBufLen,int64_t pts,int iStreamIdx,int bKeyFrame,int bAudio)
{
	AVPacket new_pkt;
	int a;
	int iRet;
	
	if (iBufLen > 3 * 1024 * 1024)
	{
		printf("MediaWriteFrame frame len %d is error!!!\n",iBufLen);
		return -1;
	}
	if (pMuxWriter->oc->nb_streams <= iStreamIdx)
	{
		return -1;
	}
	
	if (pMuxWriter->bHaveVideo && !pMuxWriter->bStartWriteFrame && (bAudio || !bKeyFrame))
	{
		printf("Discard packet audio %d stream index %d! \n",bAudio,iStreamIdx);
		return 0;
	}
	if (!bAudio && !pMuxWriter->bHaveVideo)
	{
		printf("skip video!\n");
		iRet = 0;
		return iRet;
	}
	
	av_init_packet(&new_pkt);
	new_pkt.data = pBuf;
	new_pkt.size = iBufLen;
	new_pkt.stream_index = iStreamIdx;
	new_pkt.flags = bKeyFrame?AV_PKT_FLAG_KEY:0;
	if (bAudio)
		new_pkt.pts = new_pkt.dts = pts * pMuxWriter->iAudioTimeScale /1000;
	else
	{
		new_pkt.pts =  pts * pMuxWriter->iVideoTimeScale /1000;
		new_pkt.dts = AV_NOPTS_VALUE;
	}
	
#ifdef WIN32
	EnterCriticalSection(&pMuxWriter->cs);
#else
    pthread_mutex_lock(&pMuxWriter->mutex);
#endif
	
	
	if (!bAudio && !pMuxWriter->abGetSyncFrame[iStreamIdx])
	{
		if (pMuxWriter->bMp4File)
		{
			int iOffset;
			
			iOffset = RemoveSpsPps(pBuf,iBufLen);
			if (iOffset > 0)
			{
				new_pkt.data = pBuf + iOffset;
				new_pkt.size = iBufLen - iOffset;
			}
			
			if (1)
			{
				AVCodecContext * pCodecCtx;
				
				iOffset = h264_split(new_pkt.data,new_pkt.size);
				pCodecCtx = pMuxWriter->oc->streams[iStreamIdx]->codec;
				if (iOffset > 0)
				{
					if (pCodecCtx->extradata_size > 0 && pCodecCtx->extradata)
					{
						av_free(pCodecCtx->extradata);
						pCodecCtx->extradata_size = 0;
					}
					pCodecCtx->extradata = (uint8_t *)av_malloc(iOffset);
					if (!pCodecCtx->extradata)
					{
						printf("av_malloc fail!");
						return -1;
					}
					memcpy( (char*)pCodecCtx->extradata,new_pkt.data,iOffset);
					pCodecCtx->extradata_size = iOffset;
				}
				if (pCodecCtx->extradata_size > 0)
				{
					pMuxWriter->abGetSyncFrame[iStreamIdx] = 1;
					printf("video stream %d get sync frame!\n",iStreamIdx);
				}
				
			}
		}
	}
	if (!pMuxWriter->bStartWriteFrame)
	{
		pMuxWriter->bStartWriteFrame = 1;
		//av_opt_set_int(pMuxWriter->oc->priv_data,"frag_size",10000000,0);
		//av_opt_set_int(pMuxWriter->oc->priv_data,"moov_size",2000000,0);
		avformat_write_header(pMuxWriter->oc,NULL);//写文件头
	}	
	if (bAudio && pMuxWriter->eAudioCodecID == CODEC_ID_AAC && pMuxWriter->bMp4File)
	{
		if (pBuf[0] == 0xff && (pBuf[1] & 0xf0) == 0xf0)
		{
			a = av_bitstream_filter_filter(pMuxWriter->bsfc,pMuxWriter->pAudioCodecCtx,NULL,&new_pkt.data,&new_pkt.size,pBuf,iBufLen,bKeyFrame);
			if(a>0)
			{
				new_pkt.destruct = av_destruct_packet;
			} 
			else if(a<0)
			{
				av_log(NULL, AV_LOG_ERROR, "%s failed for stream %d",
					pMuxWriter->bsfc->filter->name,iStreamIdx);
				printf("a= %d error\n", a);
				iRet =  -1;
				goto WRITE_FRAME_EXIT;
			}
		}
		
	}
	
	//printf("write packet audio %d pts %d ms\n",bAudio,new_pkt.pts);
	
	
	iRet = write_one_frame(pMuxWriter->oc,&new_pkt);
	av_free_packet(&new_pkt);
	
WRITE_FRAME_EXIT:
#ifdef WIN32
	LeaveCriticalSection(&pMuxWriter->cs);
#else
	pthread_mutex_unlock(&pMuxWriter->mutex);
#endif
	return iRet;
	
}

int MediaWriteTrailer(MuxWriter * pMuxWriter)
{
	int iRet = 0;
	char * pPtr;
	char acTmp[MAX_PATH] = {0};

	if (pMuxWriter->bStartWriteFrame)
	{
		iRet = av_write_trailer(pMuxWriter->oc);
	}
	strcpy(acTmp,pMuxWriter->acFileName);
	pPtr = strrchr(acTmp,'.');
	if (pPtr)
	{
		strcpy(pPtr,".tmp");
		pPtr = acTmp;
	}
	else
	{
		pPtr = "mp4.tmp";
	}
	remove(pPtr);
	return iRet;
}

void MediaWriterClose(MuxWriter * pMuxWriter)
{
	int i;
	
	if (pMuxWriter->bsfc)
	{
		av_bitstream_filter_close(pMuxWriter->bsfc);
	}
	if (pMuxWriter->oc)
	{
		if (!(pMuxWriter->oc->oformat->flags & AVFMT_NOFILE) && pMuxWriter->oc->pb)
			avio_close(pMuxWriter->oc->pb);
		avformat_free_context(pMuxWriter->oc);
	}
	
#ifdef WIN32
	DeleteCriticalSection(&pMuxWriter->cs);
#else
	pthread_mutex_destroy(&pMuxWriter->mutex);
#endif
}



int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};

void MediaReaderInit(DemuxReader * pDemuxReader)
{
	int i;
	pDemuxReader->ic = NULL;
	pDemuxReader->iDuration = 0;
	
	pDemuxReader->bStreamEnd = 0;
	pDemuxReader->iVideoStreamIdx = -1;
	pDemuxReader->iAudioStreamIdx = -1;
	pDemuxReader->bMp4H264BitStream = 0;
	pDemuxReader->bNeedAACAdtsHeader = 1;//all readed aac frame with adts header
	pDemuxReader->iHE_AAC = 0;
	for (i = 0;i < MAX_STREAM_NUM;i++)
	{
		pDemuxReader->aBsfc[i] = NULL;
		pDemuxReader->aiWidth[i] = 0;
		pDemuxReader->aiHeight[i] = 0;
		pDemuxReader->apVideoCodecCtx[i] = NULL;
	}
	pDemuxReader->iVideoNum = 0;
#ifdef WIN32
	InitializeCriticalSection(&pDemuxReader->cs);
#else
	pthread_mutex_init(&pDemuxReader->mutex,NULL);
#endif
}
int MediaOpenFile(DemuxReader * pDemuxReader,char * file)
{
	int ret;
	int i;
	AVCodecContext * pDecCodecCtx;
	AVFormatContext * ic;
	double dDuration = 0.0f;
	
	if((ret = avformat_open_input(&pDemuxReader->ic, file, NULL, NULL)) < 0)
	{
		printf("Cannot open input file %s \n", file);
		return ret;
	}
	
	if((ret = avformat_find_stream_info(pDemuxReader->ic,NULL)) < 0)
	{
		printf("Cannot find stream information\n");
		return ret;
	}
	
	ic = pDemuxReader->ic;
	for(i = 0; i < (int)ic->nb_streams; i++)
	{
		pDecCodecCtx = ic->streams[i]->codec;
		dDuration = ic->streams[i]->duration * 1000 * ic->streams[i]->time_base.num / ic->streams[i]->time_base.den;
		pDemuxReader->iDuration = pDemuxReader->iDuration > dDuration ?pDemuxReader->iDuration:dDuration;
		if(pDecCodecCtx->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			if (pDemuxReader->iVideoStreamIdx == -1)
			{
				pDemuxReader->iVideoStreamIdx = i;
			}
			pDemuxReader->aiWidth[i] = pDecCodecCtx->width;
			pDemuxReader->aiHeight[i] = pDecCodecCtx->height;
			if (pDecCodecCtx->extradata_size > 0 && pDecCodecCtx->extradata)
			{
				if (pDecCodecCtx->extradata[0] == 1)
				{
					pDemuxReader->aBsfc[i] = av_bitstream_filter_init("h264_mp4toannexb");
				}
			}
			pDemuxReader->apVideoCodecCtx[i] = pDecCodecCtx;
			pDemuxReader->iVideoNum++;
		}
		
		if(pDecCodecCtx->codec_type != AVMEDIA_TYPE_VIDEO)
		{
			pDemuxReader->iAudioStreamIdx = i;
			pDemuxReader->iSampleRate = pDecCodecCtx->sample_rate > 0 ?pDecCodecCtx->sample_rate:DEFAULT_AUDIO_SAMPLE_RATE;
			pDemuxReader->iChannel = pDecCodecCtx->channels == 0 ?2:pDecCodecCtx->channels;
			pDemuxReader->eAudioFmt = pDecCodecCtx->sample_fmt < 0 ? AV_SAMPLE_FMT_S16:pDecCodecCtx->sample_fmt ;
			pDemuxReader->iBitRate = pDecCodecCtx->bit_rate;
			pDemuxReader->eAudioCodecID = pDecCodecCtx->codec_id == CODEC_ID_NONE?CODEC_ID_AAC:pDecCodecCtx->codec_id;
			pDemuxReader->pAudioCodecCtx = pDecCodecCtx;
			if(pDecCodecCtx->extradata && pDecCodecCtx->extradata_size > 0)
			{
				
				if (pDecCodecCtx->extradata_size >= 2)
				{
					int samplerate;
					int samplerate_idx = ((pDecCodecCtx->extradata[0] << 1)  & 0xF) | (pDecCodecCtx->extradata[1] >> 7);
					int channel = (pDecCodecCtx->extradata[1] >> 3) & 0xF;
					
					samplerate = adts_sample_rates[samplerate_idx];
					if (samplerate * 2 == pDecCodecCtx->sample_rate)
					{
						pDemuxReader->iHE_AAC |= 0x1;
					}
					if (channel * 2 == pDecCodecCtx->channels)
					{
						pDemuxReader->iHE_AAC |= 0x2;
					}
				

				}
			}
			continue;
		}
	}
	//printf("%s 's duration is %lld ms\n",file,pDemuxReader->iDuration);
	return 0;
}



static int FindAdtsSRIndex(int sr)
{
    int i;
	
    for (i = 0; i < 16; i++)
    {
        if (sr == adts_sample_rates[i])
            return i;
    }
    return 16 - 1;
}

unsigned char *MakeAdtsHeader(int *dataSize,int samplerate, int channels,int iFrameLen )
{
    unsigned char *data;
    int profile = 1;
    int sr_index = FindAdtsSRIndex(samplerate);
    int skip = 7;
    int framesize = skip + iFrameLen;
	
	
    *dataSize = 7;
	
    data = (unsigned char *)malloc(*dataSize * sizeof(unsigned char));
    memset(data, 0, *dataSize * sizeof(unsigned char));
	
    data[0] += 0xFF; /* 8b: syncword */
	
    data[1] += 0xF0; /* 4b: syncword */
    /* 1b: mpeg id = 0 */
    /* 2b: layer = 0 */
    data[1] += 1; /* 1b: protection absent */
	
    data[2] += ((profile << 6) & 0xC0); /* 2b: profile */
    data[2] += ((sr_index << 2) & 0x3C); /* 4b: sampling_frequency_index */
    /* 1b: private = 0 */
    data[2] += ((channels >> 2) & 0x1); /* 1b: channel_configuration */
	
    data[3] += ((channels << 6) & 0xC0); /* 2b: channel_configuration */
    /* 1b: original */
    /* 1b: home */
    /* 1b: copyright_id */
    /* 1b: copyright_id_start */
    data[3] += ((framesize >> 11) & 0x3); /* 2b: aac_frame_length */
	
    data[4] += ((framesize >> 3) & 0xFF); /* 8b: aac_frame_length */
	
    data[5] += ((framesize << 5) & 0xE0); /* 3b: aac_frame_length */
    data[5] += ((0x7FF >> 6) & 0x1F); /* 5b: adts_buffer_fullness */
	
    data[6] += ((0x7FF << 2) & 0x3F); /* 6b: adts_buffer_fullness */
    /* 2b: num_raw_data_blocks */
	
    return data;
}


int MediaReadFrame(DemuxReader * pDemuxReader,MediaPacket * pMediaPkt)
{
	AVPacket pkt;
	int iRet;
	int iTimeScale;
	AVStream * st;
	int64_t pts;
	
	if (!pDemuxReader || !pMediaPkt)
	{
		return -1;
	}
	av_init_packet(&pkt);
	
#ifdef WIN32
	EnterCriticalSection(&pDemuxReader->cs);
#else
    pthread_mutex_lock(&pDemuxReader->mutex);
#endif
	
	iRet = av_read_frame(pDemuxReader->ic,&pkt);
	if (iRet < 0 )
	{
		pMediaPkt->iLen = 0;
		pMediaPkt->pData = NULL;
		pDemuxReader->bStreamEnd = 1;
#ifdef WIN32
		LeaveCriticalSection(&pDemuxReader->cs);
#else
		pthread_mutex_unlock(&pDemuxReader->mutex);
#endif
		return -1;
	}
	
#ifdef WIN32
	LeaveCriticalSection(&pDemuxReader->cs);
#else
	pthread_mutex_unlock(&pDemuxReader->mutex);
#endif
	
	if (pkt.stream_index == pDemuxReader->iAudioStreamIdx)
	{
		int iAdtsHeaderLen = 0;
		unsigned char * pAdtsHeader = NULL;
		int samplerate;
		int channels;

		pMediaPkt->bAudio = 1;
		pMediaPkt->bKeyFrame = 1;
		//printf("audio pts %lld \n",pkt.pts);
		if (pDemuxReader->bNeedAACAdtsHeader)
		{
			if (pDemuxReader->eAudioCodecID == CODEC_ID_AAC && pkt.size > 7 && pkt.data[0] != 0xff)
			{
				samplerate = (pDemuxReader->iHE_AAC & 0x1)?pDemuxReader->iSampleRate/2:pDemuxReader->iSampleRate;
				channels = (pDemuxReader->iHE_AAC & 0x2)?pDemuxReader->iChannel/2:pDemuxReader->iChannel;
				pAdtsHeader = MakeAdtsHeader(&iAdtsHeaderLen,samplerate,channels,pkt.size);
			}
		}
		pMediaPkt->pData = malloc(pkt.size + iAdtsHeaderLen);
		if (pMediaPkt->pData == NULL)
		{
			printf("MediaReadFrame malloc %d fail!\n",pkt.size);
			pMediaPkt->iLen = 0;
			pMediaPkt->pData = NULL;
			return -1;
		}
		if (iAdtsHeaderLen > 0 && pAdtsHeader)
		{
			memcpy(pMediaPkt->pData,pAdtsHeader,iAdtsHeaderLen);
			free(pAdtsHeader);
		}
		memcpy(pMediaPkt->pData + iAdtsHeaderLen,pkt.data,pkt.size);
		pMediaPkt->iLen = iAdtsHeaderLen + pkt.size;

	}
	else
	{
		AVPacket new_pkt;
		int iStreamIdx = pkt.stream_index;
		
		av_init_packet(&new_pkt);
		new_pkt = pkt;
		pMediaPkt->bAudio = 0;
		pMediaPkt->bKeyFrame = pkt.flags & AV_PKT_FLAG_KEY;
		if (!pDemuxReader->bMp4H264BitStream && pkt.data && pDemuxReader->aBsfc[iStreamIdx])
		{
			av_bitstream_filter_filter(pDemuxReader->aBsfc[iStreamIdx],pDemuxReader->apVideoCodecCtx[iStreamIdx], NULL, &pkt.data, &pkt.size,new_pkt.data,new_pkt.size,new_pkt.flags & AV_PKT_FLAG_KEY);
			av_free_packet(&new_pkt);
		}
		pMediaPkt->pData = malloc(pkt.size);
		if (pMediaPkt->pData == NULL)
		{
			printf("MediaReadFrame malloc %d fail!\n",pkt.size);
			pMediaPkt->iLen = 0;
			pMediaPkt->pData = NULL;
			return -1;
		}
		memcpy(pMediaPkt->pData,pkt.data,pkt.size);
		pMediaPkt->iLen = pkt.size;
	}
	
	pMediaPkt->iStreamIdx = pkt.stream_index;
	st = pDemuxReader->ic->streams[pkt.stream_index];
	iTimeScale =st->time_base.den / st->time_base.num;//  / st->codec->ticks_per_frame;
	pts = pkt.dts == AV_NOPTS_VALUE ?pkt.pts:pkt.dts;
	pMediaPkt->pts = pts * 1000 / iTimeScale;
	
	//printf("read ffmpeg packet pts %I64d dts %I64d  audio %d media pts %I64d ms\n",pkt.pts,pkt.dts,pMediaPkt->bAudio,pMediaPkt->pts);
	av_free_packet(&pkt);
	return 0;
}


int MediaSeek(DemuxReader * pDemuxReader,int iSeekTime)
{
	int ret;
	int64_t seektime;
	
	seektime = (int64_t)iSeekTime * AV_TIME_BASE/1000; 
	
#ifdef WIN32
	EnterCriticalSection(&pDemuxReader->cs);
#else
    pthread_mutex_lock(&pDemuxReader->mutex);
#endif
	printf("seek to time %0.3f s\n", (double)seektime / AV_TIME_BASE);
	ret = avformat_seek_file(pDemuxReader->ic,-1, INT64_MIN, seektime, INT64_MAX, 0);
	
#ifdef WIN32
	LeaveCriticalSection(&pDemuxReader->cs);
#else
	pthread_mutex_unlock(&pDemuxReader->mutex);
#endif
	
	if(ret < 0)
	{
		printf(" could not seek to position %0.3f\n", (double)seektime / AV_TIME_BASE);
	}
	return ret;
}

void MediaReaderClose(DemuxReader * pDemuxReader)
{
	int i;
	
	avformat_close_input(&pDemuxReader->ic);
	for (i = 0;i < MAX_STREAM_NUM;i++)
	{
		if (pDemuxReader->aBsfc[i])
		{
			av_bitstream_filter_close(pDemuxReader->aBsfc[i]);
		}
	}
#ifdef WIN32
	DeleteCriticalSection(&pDemuxReader->cs);
#else
	pthread_mutex_destroy(&pDemuxReader->mutex);
#endif
	
}

void MediaPacketFree(MediaPacket * pkt)
{
	if (pkt && pkt->iLen > 0 && pkt->pData)
	{
		free(pkt->pData);
		pkt->pData = NULL;
		pkt->iLen = 0;
	}
}

int MediaTranscodeInit(TransContext * pTransContext,char * InputFile,char * OutputFile)
{
	pTransContext->eAudioDecID = CODEC_ID_NONE;
	pTransContext->eAudioEncID = CODEC_ID_NONE;
	pTransContext->eVideoDecID = CODEC_ID_NONE;
	pTransContext->eVideoEncID = CODEC_ID_NONE;
	pTransContext->pAudioDecCtx = NULL;
	pTransContext->pAudioEncCtx = NULL;
	pTransContext->pVideoDecCtx = NULL;
	pTransContext->pVideoEncCtx = NULL;
	pTransContext->iDecVideoStreamIdx = -1;
	pTransContext->iEncVideoStreamIdx = -1;
	pTransContext->iDecAudioStreamIdx = -1;
	pTransContext->iEncVideoStreamIdx = -1;
	pTransContext->img_convert_ctx = NULL;
	pTransContext->bGetVideoDecFrame = 0;
	pTransContext->eSampleFmt = AV_SAMPLE_FMT_S16;
	pTransContext->iBitRate = 64000;
	pTransContext->iChannels = 2;
	pTransContext->iSampleRate = 48000;
	pTransContext->bGetEnoughAudioPcm = 0;
	pTransContext->iPcmLen = 0;
	pTransContext->dAudioStartPts = -1;
	pTransContext->dAudioPts = 0;
	pTransContext->bOpenInput = 0;
	pTransContext->bOpenOutput =0;

	if (InputFile)
	{
		AVCodec * pDecCodec;
		AVCodecContext * pDecCodecCtx;
		int i,ret;

		MediaReaderInit(&pTransContext->DemuxRdr);
		MediaOpenFile(&pTransContext->DemuxRdr,InputFile);
		pTransContext->bOpenInput = 1;


		for (i = 0;i < pTransContext->DemuxRdr.ic->nb_streams;i++)
		{
			enum CodecID eCodecID;

		

			pDecCodecCtx = pTransContext->DemuxRdr.ic->streams[i]->codec;
			eCodecID = (pDecCodecCtx->codec_id == CODEC_ID_NONE) && (pDecCodecCtx->codec_type != AVMEDIA_TYPE_VIDEO )? CODEC_ID_AAC:pDecCodecCtx->codec_id;
			pDecCodec = avcodec_find_decoder(eCodecID);
			if (!pDecCodec) 
			{
				fprintf(stderr, "avcodec_find_decoder find codec id %d fail!\n",eCodecID);
				return -1;
			}
			if ((ret = avcodec_open2(pDecCodecCtx,pDecCodec,NULL)) < 0) 
			{
				printf("could not open decoder codec error %d\n",ret);
				return -1;
			}
		}
		pTransContext->eAudioDecID = pTransContext->DemuxRdr.eAudioCodecID;
		pTransContext->pAudioDecCtx = pTransContext->DemuxRdr.pAudioCodecCtx;
	}
	if (OutputFile)
	{
		int bHaveVideo =  1;

		if (pTransContext->bOpenInput && pTransContext->DemuxRdr.iVideoNum < 0) bHaveVideo = 0;
		MediaWriterInit(&pTransContext->MuxWtr);
		MediaFileCreate(&pTransContext->MuxWtr,OutputFile,bHaveVideo);
		pTransContext->bOpenOutput = 1;
	}
}

int AudioDecInfoInit(TransContext * pTransContext,enum CodecID eCodecID,int samplerate,int bitrate,int channels,enum AVSampleFormat eFmt)
{
	AVCodec * pDecCodec     = NULL;
	AVCodecContext * pDecCodecCtx   = NULL;
	int ret;
	
	pDecCodec = avcodec_find_decoder(eCodecID);
	if (!pDecCodec) 
	{
		fprintf(stderr, "avcodec_find_decoder find codec id %d fail!\n",eCodecID);
		return -1;
	}
	
	if (!pTransContext->bOpenInput)
	{
		pDecCodecCtx = avcodec_alloc_context3(pDecCodec);
		pDecCodecCtx->bit_rate    = bitrate;
		pDecCodecCtx->sample_rate = samplerate;
		pDecCodecCtx->channels    = channels;
		pDecCodecCtx->sample_fmt  = eFmt;
		
		
		pTransContext->eAudioDecID = eCodecID;
		pTransContext->iSampleRate = samplerate;
		pTransContext->iBitRate = bitrate;
		pTransContext->iChannels = channels;
		pTransContext->eSampleFmt = eFmt;

		if ((ret = avcodec_open2(pDecCodecCtx,pDecCodec,NULL)) < 0) 
		{
			printf("could not open decoder codec error %d\n",ret);
			return -1;
		}
		pTransContext->pAudioDecCtx = pDecCodecCtx;

	}

	
	return 0;
}
int AudioEncInfoInit(TransContext * pTransContext,enum CodecID eCodecID,int samplerate,int bitrate,int channels,enum AVSampleFormat eFmt)
{
	int idx;
	AVCodec * pEncCodec     = NULL;
	AVCodecContext * pEncCodecCtx   = NULL;

	
	//pEncCodec = avcodec_find_encoder_by_name("libfdk_aac");//libaacplus libfaac
	pEncCodec = avcodec_find_encoder(eCodecID);
	if (!pEncCodec) 
	{
		fprintf(stderr, "encoder  %d not found\n",eCodecID);
		return -1;
	}
	

	if (pTransContext->bOpenOutput)
	{
		idx = AudioStreamAdd(&pTransContext->MuxWtr,eCodecID,samplerate,bitrate,channels,eFmt);
		pTransContext->pAudioEncCtx = pTransContext->MuxWtr.pAudioCodecCtx;
		pTransContext->iEncAudioStreamIdx = idx;
	}
	else
	{
		pEncCodecCtx = avcodec_alloc_context3(pEncCodec);
		
		pEncCodecCtx->sample_rate = samplerate;
		pEncCodecCtx->channels = channels;
		pEncCodecCtx->bit_rate = bitrate;
		pEncCodecCtx->sample_fmt = eFmt;

		pTransContext->pAudioEncCtx = pEncCodecCtx;
	}
	//pTransContext->pAudioEncCtx->profile = FF_PROFILE_AAC_HE_V2;
	if (avcodec_open2(pTransContext->pAudioEncCtx,pEncCodec,NULL) < 0) 
	{
		fprintf(stderr, "could not open encoder codec\n");
		return -1;
	}

	return 0;
}

int AudioDeocde(TransContext * pTransContext,MediaPacket * pMediaPkt)
{
	AVFrame AudioFrame;
	int bGetFrame = 0;
	AVCodecContext * pAVCodecCtx = pTransContext->pAudioDecCtx;
	int channels = pAVCodecCtx->channels;
	int iBytes = av_get_bytes_per_sample(pAVCodecCtx->sample_fmt);
	AVPacket pkt;
	int ret;
				
	if (!pTransContext || !pMediaPkt)
		return -1;
	
	avcodec_get_frame_defaults(&AudioFrame);
	av_init_packet(&pkt);
	pkt.data = pMediaPkt->pData;
	pkt.size = pMediaPkt->iLen;


	while(1)
	{
		ret = avcodec_decode_audio4(pAVCodecCtx,&AudioFrame,&bGetFrame,&pkt);
		if (ret > MAX_AUDIO_PCM_BUF - pTransContext->iPcmLen)
		{
			printf("Ouput Pcm Buf is too small error! need %d bytes\n",ret + pTransContext->iPcmLen);
			return -1;
		}
		memcpy(pTransContext->ucAudioPcmBuf + pTransContext->iPcmLen,AudioFrame.data[0],AudioFrame.nb_samples * channels * iBytes);
		pTransContext->iPcmLen += AudioFrame.nb_samples * channels * iBytes;
		if (bGetFrame && ret > 0 && ret < pkt.size)
		{
			pkt.data = pkt.data + ret;
			pkt.size -= ret;
		}
		else
		{
			break;
		}
	}
	pTransContext->bGetEnoughAudioPcm = pTransContext->iPcmLen >= pTransContext->pAudioEncCtx->frame_size * pTransContext->pAudioEncCtx->channels * av_get_bytes_per_sample(pTransContext->pAudioEncCtx->sample_fmt);
	return 0;
}

int AudioEncode(TransContext * pTransContext,MediaPacket * OutPkt)
{
	AVCodecContext * pEncCodecCtx = pTransContext->pAudioEncCtx;
	int iAudioEsLen;
	int iFrameSize = pEncCodecCtx->frame_size;
	int channels = pEncCodecCtx->channels;
	int iBytes = av_get_bytes_per_sample(pEncCodecCtx->sample_fmt);
	int iFrameLen = iFrameSize * channels * iBytes;
	int ret;
	
	OutPkt->iLen = 0;
	
	if (pTransContext->iPcmLen >= iFrameLen)
	{
		iAudioEsLen = MAX_AUDIO_ES_LEN;
		OutPkt->pData = malloc(iAudioEsLen);
		if (!OutPkt->pData)
		{
			printf("audio es malloc fail! %d\n",iAudioEsLen);
			return -1;
		}
		ret = avcodec_encode_audio(pEncCodecCtx,OutPkt->pData,iAudioEsLen,(short *)pTransContext->ucAudioPcmBuf);
		if (ret > 0)
		{
			OutPkt->iLen = ret;
			OutPkt->pts = pTransContext->dAudioPts;
			OutPkt->bAudio = 1;
			OutPkt->bKeyFrame = 1;
			OutPkt->iStreamIdx = pTransContext->iEncAudioStreamIdx;
			if (pTransContext->iPcmLen > iFrameLen)
			{
				memmove(pTransContext->ucAudioPcmBuf,pTransContext->ucAudioPcmBuf + iFrameLen,pTransContext->iPcmLen - iFrameLen);
			}
			pTransContext->dAudioPts += 1000.0 * iFrameSize/pEncCodecCtx->sample_rate;
			pTransContext->iPcmLen -= iFrameLen;
			pTransContext->bGetEnoughAudioPcm = pTransContext->iPcmLen >= iFrameLen;
			return 0;
		}
		else
		{
			free(OutPkt->pData);
			OutPkt->pData = NULL;
		}
	}
	pTransContext->bGetEnoughAudioPcm = pTransContext->iPcmLen >= iFrameLen;
	OutPkt->iLen = 0;
	return -1;	
}

int VideoDecInfoInit(TransContext * pTransContext,enum CodecID eCodecID,int width,int height,int iOutWidth,int iOutHeight)
{
	AVCodec * pDecCodec     = NULL;
	AVCodecContext * pDecCodecCtx   = NULL;
	int ret;
	
	pDecCodec = avcodec_find_decoder(eCodecID);
	if (!pDecCodec) 
	{
		fprintf(stderr, "avcodec_find_decoder can't find codec id %d fail!\n",eCodecID);
		return -1;
	}
	if (!pTransContext->bOpenInput)
	{
		pDecCodecCtx = avcodec_alloc_context3(pDecCodec);
		pDecCodecCtx->width = width;
		pDecCodecCtx->height = height;
		pTransContext->pVideoDecCtx = pDecCodecCtx;
		pTransContext->eVideoDecID = eCodecID;
		if ((ret = avcodec_open2(pDecCodecCtx,pDecCodec,NULL)) < 0) 
		{
			printf("could not open decoder codec error %d\n",ret);
			return -1;
		}	
	}
	else
	{
		pTransContext->DemuxRdr.bMp4H264BitStream = 1;
	}
	pTransContext->iDecOutWidth = pTransContext->iDecOutHeight = 0;
	if (iOutWidth > 0 && iOutHeight > 0 && (width != iOutWidth || height !=iOutHeight))
	{
		pTransContext->iDecOutWidth = iOutWidth;
		pTransContext->iDecOutHeight = iOutHeight;
		pTransContext->img_convert_ctx = sws_getContext(width,height,PIX_FMT_YUV420P,iOutWidth, iOutHeight,PIX_FMT_YUV420P,SWS_BICUBIC, NULL, NULL, NULL);
		if (pTransContext->img_convert_ctx == NULL)
		{
			printf("sws_getContext src width %d src height %d out width %d out height %d fail!!!",width,height,iOutWidth,iOutHeight);
		}
	}

	return 0;
}



int VideoEncInfoInit(TransContext * pTransContext,enum CodecID eCodecID,int width,int height,int bitrate,int fps,int gop,int profile,int preset)
{
	int idx = 0;
	AVCodec * pEncCodec     = NULL;
	AVCodecContext * pEncCodecCtx   = NULL;
	
	pEncCodec = avcodec_find_encoder(eCodecID);
	if (!pEncCodec) 
	{
		fprintf(stderr, "encoder  %d not found\n",eCodecID);
		return -1;
	}
	fps = fps > 0 ? fps:25;
	if (pTransContext->bOpenOutput)
	{
		idx = VideoStreamAdd(&pTransContext->MuxWtr,eCodecID,width,height,bitrate,fps);
		pTransContext->pVideoEncCtx = pTransContext->MuxWtr.apVideoCodecCtx[idx];
	}
	else
	{
		
		pEncCodecCtx = avcodec_alloc_context3(pEncCodec);
		pEncCodecCtx->width = width;
		pEncCodecCtx->height = height;
		pEncCodecCtx->bit_rate = bitrate;
		pEncCodecCtx->pix_fmt = eCodecID == CODEC_ID_MJPEG?PIX_FMT_YUVJ420P:PIX_FMT_YUV420P;
	    pEncCodecCtx->bit_rate = bitrate;
		pEncCodecCtx->time_base.den = STREAM_FRAME_RATE;
		pEncCodecCtx->time_base.num = fps;

		pTransContext->pVideoEncCtx = pEncCodecCtx;
	}
	if (eCodecID == CODEC_ID_H264)
		h264_encode_para_set(pTransContext->pVideoEncCtx,gop,profile,preset);

	if (avcodec_open2(pTransContext->pVideoEncCtx,pEncCodec,NULL) < 0) 
	{
		fprintf(stderr, "could not open encoder codec\n");
		return -1;
	}
	return idx;
}

int VideoDeocde(TransContext * pTransContext,MediaPacket * InPkt)
{
	AVPacket pkt;
	int bGetPicture = 0;
	int ret;
	AVCodecContext * pDecCodecCtx;
	int iOffset;
	AVFrame DstFrame;
	int iFrameSize;
	char * pFrameBuf;



	av_init_packet(&pkt);
	iOffset = RemoveSpsPps(InPkt->pData,InPkt->iLen);
	pkt.size = InPkt->iLen - iOffset;
	pkt.data = InPkt->pData + iOffset;
	pkt.flags = InPkt->bKeyFrame?AV_PKT_FLAG_KEY:0;
	pkt.pts = InPkt->pts;
	//av_dup_packet(&pkt);
	

	
	pkt.stream_index = InPkt->iStreamIdx;
	if (pTransContext->bOpenInput && InPkt->iStreamIdx >= 0)
	{
		pDecCodecCtx = pTransContext->DemuxRdr.ic->streams[InPkt->iStreamIdx]->codec;
	}
	else
	{
		pDecCodecCtx = pTransContext->pVideoDecCtx;
	}
    pkt.pts = InPkt->pts;
	while(1)
	{
		ret = avcodec_decode_video2(pDecCodecCtx,&pTransContext->VideoDecFrame,&bGetPicture,&pkt);
		pTransContext->bGetVideoDecFrame = bGetPicture;
		pTransContext->VideoDecFrame.pts = pkt.pts;
		pTransContext->VideoDecFrame.pict_type = AV_PICTURE_TYPE_NONE;
		if (ret > 0)
		{
			pkt.data = pkt.data + ret;
			pkt.size -= ret;
			if (pkt.size > 0 && !bGetPicture)
			{
				continue;
			}
			else if (pkt.size > 0 && bGetPicture)
			{
				printf("Discard some data %d bytes not decode!!! perhaps one packet have multiple frames!!\n",pkt.size);
			}
		}
		break;
	}
	if (bGetPicture && pTransContext->img_convert_ctx)
	{
		DstFrame.pts = InPkt->pts;//pts
		iFrameSize = avpicture_get_size(PIX_FMT_YUV420P,pTransContext->iDecOutWidth,pTransContext->iDecOutHeight);
		pFrameBuf = (uint8_t*)av_malloc(iFrameSize);
		if (pFrameBuf == NULL)
		{
			printf("pFrameBuf av_malloc %d fail!\n",iFrameSize);
			return -1;
		}
		avpicture_fill((AVPicture *)&DstFrame, pFrameBuf, PIX_FMT_YUV420P,pTransContext->iDecOutWidth,pTransContext->iDecOutHeight);

		ret = sws_scale(pTransContext->img_convert_ctx,(const uint8_t * const *)pTransContext->VideoDecFrame.data,pTransContext->VideoDecFrame.linesize,0,pDecCodecCtx->height,(uint8_t * const *)DstFrame.data,DstFrame.linesize);	
		if (ret < 0)
		{
			printf("sws_scale fail!\n");
			return -1;
		}
		pTransContext->VideoDecFrame = DstFrame;
	}
	return 0;
}
int VideoEncode(TransContext * pTransContext,MediaPacket * OutPkt)
{
	int iOutEsLen = MAX_VIDEO_ES_LEN;
	char * pOutEs;
	int ret;
	int64_t pts;

	pOutEs = malloc(iOutEsLen);
	if (!pOutEs)
	{
		printf("av_malloc encode out video es fail!\n");
		OutPkt->iLen = 0;
		return -1;
	}
	if (pTransContext->bGetVideoDecFrame == 0)
	{
		ret = avcodec_encode_video(pTransContext->pVideoEncCtx,pOutEs,iOutEsLen,NULL);
	}
	else
	{
		pTransContext->VideoDecFrame.pts = pTransContext->VideoDecFrame.pts * pTransContext->pVideoEncCtx->time_base.den / pTransContext->pVideoEncCtx->time_base.num /1000;
		ret = avcodec_encode_video(pTransContext->pVideoEncCtx,pOutEs,iOutEsLen,&pTransContext->VideoDecFrame);
	}
	if (ret > 0)
	{
		OutPkt->pData = pOutEs;
		OutPkt->iLen = ret;
		OutPkt->pts = pTransContext->pVideoEncCtx->coded_frame->pts * pTransContext->pVideoEncCtx->time_base.num * 1000 / pTransContext->pVideoEncCtx->time_base.den;//pTransContext->VideoDecFrame.pts;
		OutPkt->bAudio = 0;
		OutPkt->bKeyFrame = pTransContext->pVideoEncCtx->coded_frame->key_frame;
	}
	else
	{
		OutPkt->iLen = 0;
		free(pOutEs);
		OutPkt->pData = NULL;
	}
	if (pTransContext->bGetVideoDecFrame == 0 && ret <= 0)
	{
		return -1;
	}
	
	if (pTransContext->img_convert_ctx && pTransContext->bGetVideoDecFrame && pTransContext->VideoDecFrame.data[0])
			av_free(pTransContext->VideoDecFrame.data[0]);
	pTransContext->bGetVideoDecFrame = 0;
	return 0;
}

int MediaTranscodeClose(TransContext * pTransContext)
{
	if (pTransContext->pAudioDecCtx)
	{
		avcodec_close(pTransContext->pAudioDecCtx);//不关会出现内存泄露
	}

	if (pTransContext->pVideoDecCtx)
	{
		avcodec_close(pTransContext->pVideoDecCtx);
	}
	if (pTransContext->pAudioEncCtx)
	{
		avcodec_close(pTransContext->pAudioEncCtx);
	}
	if (pTransContext->pVideoEncCtx)
	{
		avcodec_close(pTransContext->pVideoEncCtx);
	}
	if (pTransContext->bOpenInput)
	{
		MediaReaderClose(&pTransContext->DemuxRdr);
	}
	else
	{
		if (pTransContext->pAudioDecCtx)
			av_free(pTransContext->pAudioDecCtx);
		if (pTransContext->pVideoDecCtx)
			av_free(pTransContext->pVideoDecCtx);
	}
	if (pTransContext->bOpenOutput)
	{
		MediaWriteTrailer(&pTransContext->MuxWtr);
		MediaWriterClose(&pTransContext->MuxWtr);
	}
	else
	{
		if (pTransContext->pAudioEncCtx)
			av_free(pTransContext->pAudioEncCtx);
		if (pTransContext->pVideoEncCtx)
			av_free(pTransContext->pVideoEncCtx);
	}
	
	return 0;
}


#define EXTRA_DATA_OFFSET 2000
#define FRAME_INFO_OFFSET 4000

int64_t Mp4Recovery(char * RecoveryFile,char * InFile,char * OutFile)
{
	MuxWriter MuxWtr;
	FILE * fp,* fpRead;
	uint64_t mdat_pos,offset;
	int64_t dts;
	int i,iAudioStreamIdx,iStreamNum,iStreamIdx,iFrameSize,bVideo,bKeyFrame,iBitRate,iWidth,iHeight,iSampleRate,iChannel;
	enum CodecID eCodecID;
	enum AVSampleFormat eSampleFmt;
	unsigned char * pVideoBuf, * pAudioBuf;
	int iExtraDataNum,iExtraDataSize;
	char acExtraData[1024];
	int64_t last_dts = -1;
	int64_t duration = 0;
	
	fp = fopen(RecoveryFile,"rb");
	if (fp == NULL)
	{
		printf("open %s fail!\n",RecoveryFile);
		return -1;
	}
	
	fpRead = fopen(InFile,"rb");
	if (fp == NULL)
	{
		printf("open %s fail!\n",InFile);
		return -1;
	}
	MediaSysInit();
	MediaWriterInit(&MuxWtr);
	MediaFileCreate(&MuxWtr,OutFile,1);//只写音频
	
	iAudioStreamIdx = -1;

	fread(&iStreamNum,1,4,fp);
	for(i = 0;i < iStreamNum;i++)
	{
		fread(&iStreamIdx,1,4,fp);
		fread(&bVideo,1,4,fp);
		fread(&eCodecID,1,4,fp);
		if (bVideo)
		{
			
			fread(&iBitRate,1,4,fp);
			fread(&iWidth,1,4,fp);
			fread(&iHeight,1,4,fp);
			VideoStreamAdd(&MuxWtr,eCodecID,iWidth,iHeight,iBitRate,25);
		}
		else
		{
			fread(&iSampleRate,1,4,fp);
			fread(&iBitRate,1,4,fp);
			fread(&iChannel,1,4,fp);
			fread(&eSampleFmt,1,4,fp);
			AudioStreamAdd(&MuxWtr,eCodecID,iSampleRate,iBitRate,iChannel,eSampleFmt);
			iAudioStreamIdx = i;
		}
	}
	fseek(fp,EXTRA_DATA_OFFSET,SEEK_SET);
	fread(&iExtraDataNum,1,4,fp);

	
	for(i = 0;i < iExtraDataNum;i++)
	{
		AVCodecContext * pCodecCtx;

		fread(&iStreamIdx,1,4,fp);
		fread(&iExtraDataSize,1,4,fp);
		if (iExtraDataSize > sizeof(acExtraData))
		{
			printf("extradata size %d is too long error!",iExtraDataSize);
			return -1;
		}
		fread(acExtraData,1,iExtraDataSize,fp);
		if (iStreamIdx < 0 || iStreamIdx >= MuxWtr.oc->nb_streams)
		{
			printf("stream index %d error !\n",iStreamIdx);
			return -1;
		}
		if (iExtraDataSize <= 0)
			continue;
		pCodecCtx = MuxWtr.oc->streams[iStreamIdx]->codec;
		pCodecCtx->extradata = av_malloc(iExtraDataSize + FF_INPUT_BUFFER_PADDING_SIZE);
		if (!pCodecCtx->extradata)
		{
			printf("av_malloc %d fail!",iExtraDataSize);
			return -1;
		}
		memcpy(pCodecCtx->extradata,acExtraData,iExtraDataSize);
		pCodecCtx->extradata_size = iExtraDataSize;
	}

	fseek(fp,FRAME_INFO_OFFSET,SEEK_SET);
	while(1)
	{
		int iOff,iRet;
		unsigned int uiNalSize = 0;

		
		fread(&iStreamIdx,1,4,fp);
		fread(&offset,1,8,fp);
		fread(&iFrameSize,1,4,fp);
		fread(&dts,1,8,fp);
		fread(&bKeyFrame,1,4,fp);
		iRet = fseek(fpRead,offset,SEEK_SET);
		if (iRet < 0)
		{
			printf("fseek error! [iRet = %d, offset = %d]\n",iRet, offset);
			break;
		}
		
		if (feof(fp) ) {
			printf("feof(fp) error!\n");
			break;
		}

		if (feof(fpRead)) {
			printf("feof(fpRead) error!\n");
			break;
		}

		if (iFrameSize < 0 || iFrameSize > 3*1024*1024) {
			printf("[iFrameSize = %d] error!\n",iFrameSize);
			break;
		}

		if (iStreamIdx != iAudioStreamIdx)
		{
			pVideoBuf = malloc(iFrameSize);
			if (pVideoBuf == NULL)
			{
				printf("malloc fail! %d\n",iFrameSize);
				//return -1;
				break;
			}
			iRet = fread(pVideoBuf,1,iFrameSize,fpRead);
			if (iRet != iFrameSize)
			{
				printf("fread is error warning\n");
				break;
			}
			for (iOff = 0;iOff < iFrameSize;)
			{
				uiNalSize = pVideoBuf[iOff] << 24 | pVideoBuf[iOff + 1] <<16 | pVideoBuf[iOff + 2] << 8 | pVideoBuf[iOff + 3];
				if (uiNalSize >= iFrameSize)
				{
					printf("video data error!\n");
				    //goto EXIT_ERROR;
					break;
				}
				pVideoBuf[iOff] = pVideoBuf[iOff + 1] = pVideoBuf[iOff + 2] = 0;
				pVideoBuf[iOff + 3] = 1;
				iOff += uiNalSize + 4;
			}
			MediaWriteFrame(&MuxWtr,pVideoBuf,iFrameSize,dts,iStreamIdx,bKeyFrame,0);
			free(pVideoBuf);
			duration = dts;
		}
		else
		{
			pAudioBuf = malloc(iFrameSize);
			if (pAudioBuf == NULL)
			{
				printf("malloc fail! %d\n",iFrameSize);
				//return -1;				
				break;
			}
			iRet = fread(pAudioBuf,1,iFrameSize,fpRead);
			if (iRet != iFrameSize)
			{
				printf("fread is error warning\n");
				break;
			}
			dts = dts * 1000/iSampleRate;
			if (last_dts == -1)
			{
				last_dts = dts;
			}
			else
			{
				if (dts <= last_dts)
				{
					dts += 1;
				}
				last_dts = dts;
			}
			MediaWriteFrame(&MuxWtr,pAudioBuf,iFrameSize,dts,iStreamIdx,bKeyFrame,1);

			free(pAudioBuf);
			duration = dts;
		}	
	}
EXIT_ERROR:
	fclose(fp);
	fclose(fpRead);
	MediaWriteTrailer(&MuxWtr);
	MediaWriterClose(&MuxWtr);
	return duration;
}

int MediaGenIndexImage(char * InFile,char * image,int iInternal,int iStartNum,int iMaxImageNum)
{
	DemuxReader DmxRdr;
	MuxWriter MuxWtr;
	TransContext TransCtx;
	int iWidth,iHeight;
	int ret;
	int iCount = 0;
	char * pStr;

	MediaSysInit();
	MediaReaderInit(&DmxRdr);
	ret = MediaOpenFile(&DmxRdr,InFile);
	if (ret < 0)
		return ret;

	MediaWriterInit(&MuxWtr);
	MediaFileCreate(&MuxWtr,image,1);

	iInternal = iInternal <=0 ?10000:iInternal;
	iStartNum = iStartNum < 0?0:iStartNum;
	iMaxImageNum = iMaxImageNum <= 0 ?1:iMaxImageNum;
	pStr = strchr(image,'%');
	iMaxImageNum = pStr?iMaxImageNum:1;


	MediaTranscodeInit(&TransCtx,NULL,NULL);

	iWidth = DmxRdr.aiWidth[DmxRdr.iVideoStreamIdx];
	iHeight = DmxRdr.aiHeight[DmxRdr.iVideoStreamIdx];
	VideoStreamAdd(&MuxWtr,CODEC_ID_MJPEG,iWidth,iHeight,0,25);

	VideoDecInfoInit(&TransCtx,DmxRdr.apVideoCodecCtx[DmxRdr.iVideoStreamIdx]->codec_id,iWidth,iHeight,iWidth,iHeight);
	VideoEncInfoInit(&TransCtx,CODEC_ID_MJPEG,iWidth,iHeight,0,25,0,0,0);
	av_opt_set_int(MuxWtr.oc->priv_data,"start_number",iStartNum + 1,0);
	while(!DmxRdr.bStreamEnd && iCount < iMaxImageNum)
	{
		MediaPacket pkt,outpkt;
		int iRet;
		
		MediaReadFrame(&DmxRdr,&pkt);
		if (!pkt.bAudio)
		{
			VideoDeocde(&TransCtx,&pkt);
			iRet = VideoEncode(&TransCtx,&outpkt);
			if (outpkt.iLen > 0 && iCount * iInternal <= pkt.pts)
			{
				MediaWriteFrame(&MuxWtr,outpkt.pData,outpkt.iLen,outpkt.pts,0,1,0);
				iCount++;
				free(outpkt.pData);
			}
			else
			{
				printf("deocode fail!\n");
			}
		}
		if (pkt.iLen > 0)
		{
			free(pkt.pData);
		}
	}
	MediaTranscodeClose(&TransCtx);
	MediaReaderClose(&DmxRdr);
	MediaWriterClose(&MuxWtr);
	

}
