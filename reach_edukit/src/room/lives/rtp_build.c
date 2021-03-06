/*****************************************************************************
*
*rtsp.c
*============================================================================
* This file is used for formating h.264/aac  to rtsp flow
* Ver      alpha_1.0
* Author  jbx
* Shenzhen reach 2010.9.6
*============================================================================
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "rtp_build.h"
#include "rtp_struct.h"
#include "media_msg.h"

//#include "../log/log_common.h"

#include "lives.h"



//static unsigned int            g_time_org = 0;

#if 0
static NALU_t                  *g_audio_puiNalBuf = NULL;




/**************************************************************************************************
                                        为NALU_t结构体分配内存空间
**************************************************************************************************/
static NALU_t *AllocNALU(int buffersize)
{
	NALU_t *n;

	if((n = (NALU_t *)calloc(1, sizeof(NALU_t))) == NULL) {
		exit(0);
	}

	n->max_size = buffersize;

	if((n->buf = (char *)calloc(buffersize, sizeof(char))) == NULL) {
		free(n);
		exit(0);
	}

	return n;
}



/*********************************************************************************************************
                                        为NALU_t结构体释放内存空间
*********************************************************************************************************/
static void FreeNALU(NALU_t *n)
{
	if(n) {
		if(n->buf) {
			free(n->buf);
			n->buf = NULL;
		}

		free(n);
	}
}


/********************************************************************************************************************
RTP 各项参数初始化
1.初始化锁
2.分配音频打包所需的内存空间
*********************************************************************************************************************/
void rtp_build_init()
{
	g_audio_puiNalBuf = AllocNALU(20000);
	return ;
}



/********************************************************************************************************************
退出 RTP 协议
1.锁销毁
2.释放分配的内存
*********************************************************************************************************************/
void rtb_build_uninit()
{

	FreeNALU(g_audio_puiNalBuf);
	return;
}

#endif

/********************************************************************************************************************
发送 RTP video 数据
*********************************************************************************************************************/
#if 0
static int SendMultCastVideoData(char *pData, int length,void *info)
{
	rtp_porting_senddata(2, pData, length, 0,info);
	return 0;
}

/********************************************************************************************************************
发送 RTP audio 数据
*********************************************************************************************************************/
static int SendMultCastAudioData(char *pData, int length,void *info)
{
	//nslog(NS_ERROR,"len = %d\n",length);
	rtp_porting_senddata(1, pData, length, 0,info);
	return 0;
}
int32_t send_rtp_video(video_data_user_t *user , int8_t *send_buf,int32_t len)


#endif


int rtp_build_jpeg_data(RTP_BUILD_HANDLE handle,int nLen, unsigned char *pData,unsigned int ts_current,int rtp_mtu, void *info ,FRAMEHEAD *data_info)
{

//	printf("usernum ----- %d \n",data_user_info->user_num);

//	int32_t  							temp_len		= 

	int32_t 							send_total_len	= 0;
	int32_t								send_buf_num	= 0;
	int32_t 							bytes			= 0;

	unsigned short *seq = (unsigned short *)(&(handle->seq_num));
	unsigned char                   	sendbuf[1500] = {0};
	
	RTP_FIXED_HEADER					*rtp_hdr;
	rtp_hdr 							= (RTP_FIXED_HEADER *)&sendbuf[0];
	rtp_hdr->payload	                = H264;
	rtp_hdr->version	                = 2;
	rtp_hdr->marker                    	= 0;
	rtp_hdr->extension					= 1;			// zhengyb 标示含有rtp 扩展头
//	rtp_hdr->extension					= 0;

	rtp_hdr->ssrc		                = htonl(10);
	rtp_hdr->timestamp		            = htonl(ts_current);
//	rtp_hdr->scrc						= htonl(0);			// zhengyb ???

	// 添加rtp 扩展头以及扩展头信息中的帧头信息zhengyb

#if 1

	rtp_debug_head_t 					*debug_head;

	debug_head = (rtp_debug_head_t *)&sendbuf[12];
	
	debug_head->profile_defined			= htons(0);
	debug_head->length					= htons(3);
	
#endif
	FRAMEHEAD							*frame_head;
	frame_head = (FRAMEHEAD *)&sendbuf[16];
	frame_head->codec		= data_info->codec;
//	frame_head->samplerate	= data_info->samplerate;
	frame_head->framerate	= data_info->framerate;
	frame_head->Iframe		= data_info->Iframe;


#if 1
	frame_head->height   	= htons(data_info->height);
	frame_head->width 		= htons(data_info->width);
	frame_head->reserve		= data_info->reserve;
	frame_head->framelength = htonl(data_info->framelength);
#endif
#if 0
	frame_head->height   	= data_info->height;
	frame_head->width 		= data_info->width;
	frame_head->resever		= data_info->resever;
	frame_head->framelength = data_info->framelength;
#endif	
#if 0
	printf("framelength  ---- %d ---- \n",data_info->framelength);
	printf("codec  ---- %d ---- \n",frame_head->codec);
	printf("framerate  ---- %d ---- \n",frame_head->framerate);
	printf("height  ---- %d---- \n",frame_head->height);
	printf("width  ---- %d---- \n",frame_head->width);
	printf("resever  ---- %d---- \n",frame_head->reserve);
	printf("samplerate  ---- %d---- \n",frame_head->samplerate);
#endif
	if(nLen <= rtp_mtu - 28) 
	{
		//设置rtp M 位；
		rtp_hdr->marker = 1;

		rtp_hdr->seq_no 	= htons((*seq)++); //序列号，每发送一个RTP包增1
		frame_head->samplerate = 3;
		r_memcpy(&sendbuf[28], pData, nLen);
		bytes	= nLen + 28;
	//	printf("samplerate  ---- %d---- \n",frame_head->samplerate);
	//	data_user_info = (video_data_user_t *)info;
	//	printf("usernum ----- %d \n",data_user_info->user_num);
		if(send_rtp_video(info,sendbuf,bytes) < 0)
		{
			//printf("send_rtp_video is eror!\n");
			return -1;
		}
	}
	 else if(nLen > rtp_mtu - 28)
	{
		//得到该nalu需要用多少长度为1400字节的RTP包来发送
		int k = 0, l = 0;
		int t = 0; //用于指示当前发送的是第几个分片RTP包

		l = nLen % (rtp_mtu- 28); //最后一个RTP包的需要装载的字节数

		if(l == 0) {
			k = nLen / (rtp_mtu - 28) -1; //需要k个1400字节的RTP包
			l = (rtp_mtu - 28);
		} else {
			k = nLen / (rtp_mtu - 28) ; //需要k个1400字节的RTP包
		}

		while(t <= k)
		{
		//	r_memset(&sendbuf[28] , 0 ,1472);
			rtp_hdr->seq_no = htons((*seq)++); //序列号，每发送一个RTP包增1

			if(!t)								//发送第一个分片包
			{
				rtp_hdr->marker = 0;
				
				
				frame_head->samplerate = 2;
				r_memcpy(&sendbuf[28], pData, (rtp_mtu - 28));
				
				t++;
				bytes = rtp_mtu;
				// 发送
		//	printf("samplerate	---- %d---- \n",frame_head->samplerate);
				if(send_rtp_video(info,sendbuf,bytes) < 0)
				{
					//printf("send_rtp_video is eror!\n");
					return -1;
				}
				
			}
			else if(k == t)						//发送最后一个分片包
			{
				rtp_hdr->marker = 1;
				frame_head->samplerate = 1;
			//	printf("222_bengin  ----%d ----- %d ----- %d!\n",t*(rtp_mtu-28),l,nLen);
			//	printf("data_addr : ----- %p\n",(pData + t*(rtp_mtu-28)));
				
				r_memcpy(&sendbuf[28], pData + t*(rtp_mtu-28), l);
			//	printf("222_stop !\n");
				t++;
				bytes = l + 28;
				// 发送
			//	printf("samplerate	---- %d---- \n",frame_head->samplerate);
				if(send_rtp_video(info,sendbuf,bytes) < 0)
				{
					//printf("send_rtp_video is eror!\n");
					return -1;
				}
			}
			else if(t < k && 0!= t) 			// 发送中间的分片包
			{
				rtp_hdr->marker = 0;
				frame_head->samplerate = 0;
				r_memcpy(&sendbuf[28], pData + t*(rtp_mtu-28) , (rtp_mtu - 28));
				t++;
				bytes = rtp_mtu;
				// 发送
			//	printf("samplerate	---- %d---- \n",frame_head->samplerate);
				if(send_rtp_video(info,sendbuf,bytes) < 0)
				{
					//printf("send_rtp_video is eror!\n");
					return -1;
				}
			}
			
		}
	
	}
	return 0;
}

static int SendRtpNalu(NALU_t *nalu , unsigned short *seq_num, unsigned int ts_current , int roomid, int end, int mtu,void *info, FRAMEHEAD *data_info)
{
	int 	rtp_mtu = mtu;
	char				 sendbuf[1500] = {0};
	RTP_FIXED_HEADER			*rtp_hdr;
	rtp_extension_head_t		*rtp_exten_head;
	char                        *nalu_payload;
	FU_INDICATOR	            *fu_ind;
	FU_HEADER		            *fu_hdr;
	int                          bytes = 0;

	int total_len = 0;
	memset(sendbuf, 0, 20);
	//设置RTP HEADER，
	rtp_hdr 							= (RTP_FIXED_HEADER *)&sendbuf[0];
	rtp_hdr->payload	                = H264;
	rtp_hdr->version	                = 2;
	rtp_hdr->marker                    = 0;
	rtp_hdr->extension					= 1;			// zhengyb 标示含有rtp 扩展头
//	rtp_hdr->extension					= 0;

	rtp_hdr->ssrc		                = htonl(10);
	rtp_hdr->timestamp		            = htonl(ts_current);
//	nslog(NS_INFO,"<R_VIDEO>  ________________ <SEND_TIME_BEFORE : %u>	<SEND_TIME_AFTER : %u>\n",ts_current,rtp_hdr->timestamp);
//	printf(" time is  ---------------- %lld +==================== %d\n",rtp_hdr->timestamp,ts_current);
//	rtp_hdr->scrc						= htonl(0);			// zhengyb ???

	// 添加rtp 扩展头以及扩展头信息中的帧头信息zhengyb

#if 1

	rtp_debug_head_t 					*debug_head;

	debug_head = (rtp_debug_head_t *)&sendbuf[12];
	
	debug_head->profile_defined			= htons(0);
	debug_head->length					= htons(3);
	
#endif
	FRAMEHEAD							*frame_head;
	frame_head = (FRAMEHEAD *)&sendbuf[16];
	frame_head->codec		= data_info->codec;
	frame_head->samplerate	= data_info->samplerate;
	frame_head->framerate	= data_info->framerate;

#if 1
	frame_head->height   	= htons(data_info->height);
	frame_head->width 		= htons(data_info->width);
	frame_head->Iframe		= data_info->Iframe;
	frame_head->reserve		= data_info->reserve;
	frame_head->framelength = htonl(data_info->framelength);
#endif
#if 0
	frame_head->height   	= data_info->height;
	frame_head->width 		= data_info->width;
	frame_head->resever		= data_info->resever;
	frame_head->framelength = data_info->framelength;
#endif	
#if 0
	printf("framelength  ---- %d ---- \n",data_info->framelength);
	printf("codec  ---- %d ---- \n",frame_head->codec);
	printf("framerate  ---- %d ---- \n",frame_head->framerate);
	printf("height  ---- %d---- \n",frame_head->height);
	printf("width  ---- %d---- \n",frame_head->width);
	printf("resever  ---- %d---- \n",frame_head->reserve);
	printf("Iframe  ---- %d---- \n",frame_head->Iframe);
	printf("samplerate  ---- %d---- \n\n\n",frame_head->samplerate);

	
#endif
#if 0
	if(frame_head->Iframe != 0)
	{
		printf("find the I frame!\n");
		printf("Iframe  ---- %d---- \n",frame_head->Iframe);
	}
#endif
	
	if((nalu->len - 1) <= rtp_mtu) {
		//设置rtp M 位；

		rtp_hdr->marker = 1;
		rtp_hdr->seq_no 	= htons((*seq_num)++); //序列号，每发送一个RTP包增1
		r_memcpy(&sendbuf[28], nalu->buf, nalu->len);
		bytes = nalu->len + 28 ; 					//获得sendbuf的长度,为nalu的长度（包含NALU头但除去起始前缀）加上rtp_header的固定长度12字节
		//	SendRtspVideoData(sendbuf, bytes,nalu->nal_unit_type,roomid);
	//	SendMultCastVideoData(sendbuf, bytes,info);
		if(send_rtp_video(info,sendbuf,bytes) < 0)
		{
			//printf("send_rtp_video is eror!\n");
			return -1;
		}
		
		total_len = nalu->len;
	} else if((nalu->len - 1) > rtp_mtu) {
		//得到该nalu需要用多少长度为1400字节的RTP包来发送
		int k = 0, l = 0;
		int t = 0; //用于指示当前发送的是第几个分片RTP包

		l = (nalu->len - 1) % rtp_mtu; //最后一个RTP包的需要装载的字节数

		if(l == 0) {
			k = (nalu->len - 1) / rtp_mtu - 1; //需要k个1400字节的RTP包
			l = rtp_mtu;
		} else {
			k = (nalu->len - 1) / rtp_mtu; //需要k个1400字节的RTP包
		}

		while(t <= k) {
			rtp_hdr->seq_no = htons((*seq_num)++); //序列号，每发送一个RTP包增1

			if(!t) { //发送一个需要分片的NALU的第一个分片，置FU HEADER的S位
				//设置rtp M 位；
				rtp_hdr->marker = 0;
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
				fu_ind = (FU_INDICATOR *)&sendbuf[28]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
				fu_ind->F = nalu->forbidden_bit;
				fu_ind->NRI = nalu->nal_reference_idc >> 5;
				fu_ind->TYPE = 28;

				//设置FU HEADER,并将这个HEADER填入sendbuf[13]
				fu_hdr = (FU_HEADER *)&sendbuf[29];
				fu_hdr->E = 0;
				fu_hdr->R = 0;
				fu_hdr->S = 1;
				fu_hdr->TYPE = nalu->nal_unit_type;

				nalu_payload = &sendbuf[30]; //同理将sendbuf[14]赋给nalu_payload
				r_memcpy(nalu_payload, nalu->buf + 1, rtp_mtu); //去掉NALU头

				//	nslog(NS_ERROR,"%x,ser_no=%d\n",nalu_payload[0],*seq_num);
				bytes = rtp_mtu + 30;						//获得sendbuf的长度,为nalu的长度（除去起始前缀和NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节
			//	SendRtspVideoData(sendbuf, bytes,0,roomid);
		//		SendMultCastVideoData(sendbuf, bytes,info);

				if(send_rtp_video(info,sendbuf,bytes) < 0)
				{
					//printf("send_rtp_video is eror!\n");
					return -1;
				}
			
				total_len += rtp_mtu;
				t++;
			}
			//发送一个需要分片的NALU的非第一个分片，清零FU HEADER的S位，如果该分片是该NALU的最后一个分片，置FU HEADER的E位
			else if(k == t) { //发送的是最后一个分片，注意最后一个分片的长度可能超过1400字节（当l>1386时）。
				//设置rtp M 位；当前传输的是最后一个分片时该位置1
				rtp_hdr->marker = 1;
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
				fu_ind = (FU_INDICATOR *)&sendbuf[28]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
				fu_ind->F = nalu->forbidden_bit;
				fu_ind->NRI = nalu->nal_reference_idc >> 5;
				fu_ind->TYPE = 28;

				//设置FU HEADER,并将这个HEADER填入sendbuf[13]
				fu_hdr = (FU_HEADER *)&sendbuf[29];
				fu_hdr->R = 0;
				fu_hdr->S = 0;
				fu_hdr->TYPE = nalu->nal_unit_type;
				fu_hdr->E = 1;

				nalu_payload = &sendbuf[30]; //同理将sendbuf[14]的地址赋给nalu_payload
				r_memcpy(nalu_payload, nalu->buf + t * rtp_mtu + 1, l); //将nalu最后剩余的l-1(去掉了一个字节的NALU头)字节内容写入sendbuf[14]开始的字符串。
				bytes = l + 30;		//获得sendbuf的长度,为剩余nalu的长度l-1加上rtp_header，FU_INDICATOR,FU_HEADER三个包头共14字节
				//	nslog(NS_ERROR,"%x,ser_no=%d\n",nalu_payload[0],*seq_num);
				//		SendRtspVideoData(sendbuf, bytes,0,roomid);
		//		SendMultCastVideoData(sendbuf, bytes,info);

				if(send_rtp_video(info,sendbuf,bytes) < 0)
				{
					//printf("send_rtp_video is eror!\n");
					return -1;
				}
				total_len += l;
				t++;
			} else if(t < k && 0 != t) {
				//设置rtp M 位；
				rtp_hdr->marker = 0;
				//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
				fu_ind = (FU_INDICATOR *)&sendbuf[28]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
				fu_ind->F = nalu->forbidden_bit;
				fu_ind->NRI = nalu->nal_reference_idc >> 5;
				fu_ind->TYPE = 28;

				//设置FU HEADER,并将这个HEADER填入sendbuf[13]
				fu_hdr = (FU_HEADER *)&sendbuf[29];
				fu_hdr->R = 0;
				fu_hdr->S = 0;
				fu_hdr->E = 0;
				fu_hdr->TYPE = nalu->nal_unit_type;

				nalu_payload = &sendbuf[30]; //同理将sendbuf[14]的地址赋给nalu_payload
				r_memcpy(nalu_payload, nalu->buf + t * rtp_mtu + 1, rtp_mtu); //去掉起始前缀的nalu剩余内容写入sendbuf[14]开始的字符串。
				bytes = rtp_mtu + 30;						//获得sendbuf的长度,为nalu的长度（除去原NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节

				//	SendRtspVideoData(sendbuf, bytes,0,roomid);
		//		SendMultCastVideoData(sendbuf, bytes,info);

				if(send_rtp_video(info,sendbuf,bytes) < 0)
				{
					//printf("send_rtp_video is eror!\n");
					return -1;
				}
				total_len += rtp_mtu;
				t++;
			}
		}
	}

	//if(total_len+1 != nalu->len )
	//{
	//	nslog(NS_ERROR,"nalu send len =%d,nalu len= %d\n",total_len,nalu->len);
	//}
	return 0;
}

//static unsigned short                           g_seq_num = 0;
int rtp_build_video_data(RTP_BUILD_HANDLE handle,int nLen, unsigned char *pData, int nFlag, int rtp_mtu, unsigned int nowtime, void *info ,FRAMEHEAD *data_info)
//(int nLen,  unsigned char *pData, int nFlag,unsigned int timetick ,int *seq,int roomid,int idr_flag)
{
	if(handle == NULL)
	{
		nslog(NS_ERROR,"ERROR,the video handle is NULL\n");
		return -1;
	}

	if(rtp_mtu < 228) {
		nslog(NS_ERROR,"Error\n");
		return 0;
	}

	//int time_org = handle->time_org; 
	
	int roomid = 0;
	unsigned int timetick = 0;
	unsigned short *seq = (unsigned short *)(&(handle->seq_num));
//	printf("nowtime =%u==%u\n",nowtime,handle->time_org);
	if((handle->time_org) == 0 || nowtime < (handle->time_org)) {
		(handle->time_org) = nowtime;
	}

	timetick = nowtime - (handle->time_org);

//	nslog(NS_INFO,"<R_VIDEO>  ============== <NOW_TIME : %u>	<TIME_ORG : %u> <TIME_TICK : %u>\n",nowtime,handle->time_org,timetick);

	NALU_t		nalu;
	unsigned char *pstart;

	unsigned int ts_current  = 0;
	unsigned int mytime = 0;

	unsigned char *pos = pData;
	unsigned char *ptail = pData + nLen - 4;
	unsigned char *temp1, *temp2;

	unsigned char nalu_value = 0;
	temp1 = temp2 = NULL;
	int send_total_len = 0;

	//14 = rtp head len +2
	int mtu = rtp_mtu - 14;
	int cnt = 0;

	for(;;) {
		mytime = (unsigned int)timetick;
		//ts_current = mytime * 90;
	//	printf("mytiem=%x\n",mytime);
	//	mytime +=  (0xffffffff/44-1000*120);
		ts_current =mytime * 90 ;

		if(ts_current >= 0XFFFFFFFF)
		{
			//	nslog(NS_ERROR,"Warnning,the ts_current is too big =%lx\n",ts_current);
				(handle->time_org) = 0;
		}

					
	//	printf("video_time = %x\n",ts_current);
		pstart = pos;
		memset(&nalu, 0 , sizeof(nalu));

		nalu_value = 0;

		//判断头是否nalu头 0001 或者 001 ,不是退出
		if(!((*pos == 0 && *(pos + 1) == 0 && *(pos + 2) == 0 && *(pos + 3) == 1))) {
			nslog(NS_ERROR,"read nalu header failed!\n");
		} else {
			temp1 = pos;
			nalu_value = *(pos + 4);
		}

		if(((nalu_value & 0x1f) == 7) || ((nalu_value & 0x1f) == 8))
		{
			//找到下一个nalu头 0001 或者 001 , 或者到帧尾
			do 
			{
				pos++;
			}while((*pos != 0 || *(pos + 1) != 0 || *(pos + 2) != 0 || *(pos + 3) != 1)
			        && (pos < ptail));
		} 
		else
		{
			pos = ptail;
		}

		if(pos >= ptail)

		{
			//如果是到达帧尾， 则把整个剩余数据作为一个nalu单元发送
			nalu.buf = pstart + 4;
			nalu.len = pData - pstart + nLen  - 4 ;
			//nslog(NS_ERROR,"nalu_len=%d,the len =%d,%d,%p,%p,%p\n",nalu.len,temp2-temp1 ,ptail-pstart,pstart,pos,ptail);
			send_total_len += (nalu.len + 4);
			nalu.forbidden_bit = nalu.buf[0] & 0x80;
			nalu.nal_reference_idc = nalu.buf[0] & 0x60;
			nalu.nal_unit_type = (nalu.buf[0]) & 0x1f;
			/*
			                        DEBUG(DL_FLOW,"send last nalu pkt! len = %d frame_len = %d flag = %d \
			                        pdata = 0x%x pos = 0x%x pstart = 0x%x\n", nalu.len, nLen ,nFlag\
			                        , pData , pos , pstart);
			*/
			SendRtpNalu(&nalu, (unsigned short *)seq, ts_current, roomid, 1, mtu,info,data_info);
			//nslog(NS_ERROR,"seq =%d,time =%ld\n",seq,ts_current);
			break;
		} else {
			//发送一个nalu单元
			nalu.buf = pstart + 4;
			nalu.len = pos - pstart - 4;
			send_total_len += (nalu.len + 4);
			nalu.forbidden_bit = nalu.buf[0] & 0x80;
			nalu.nal_reference_idc = nalu.buf[0] & 0x60;
			nalu.nal_unit_type = (nalu.buf[0]) & 0x1f;
			/*
			                        DEBUG(DL_FLOW,"send nalu pkt! len = %d frame_len = %d flag = %d\
			                        pdata = 0x%x pos = 0x%x pstart = 0x%x\n", nalu.len, nLen ,nFlag\
			                        , pData , pos , pstart);
			 */
			SendRtpNalu(&nalu, (unsigned short *)seq, ts_current, roomid, 0, mtu,info,data_info);
			//nslog(NS_ERROR,"seq =%d,time =%ld\n",seq,ts_current);
		}
	}

	if(send_total_len != nLen) {
		nslog(NS_ERROR,"send_total_len = %d,nLen=%d\n", send_total_len, nLen);
	}

	return 0;
}


//static unsigned short g_audio_seq_num = 0;
int rtp_build_audio_data(RTP_BUILD_HANDLE handle,int nLen, unsigned char *pData, int samplerate, int rtp_mtu, unsigned int nowtime,void *info,FRAMEHEAD *data_info)
{


	if(handle == NULL)
	{
		//printf("ERROR,the audio handle is NULL\n");
		nslog(NS_ERROR,"ERROR,the audio handle is NULL\n");
		return -1;
	}

	rtp_extension_head_t		*rtp_exten_head;

	int payload = handle->payload;
	int ssrc = handle->ssrc;
	
	int begin_len = 0;
	unsigned char                   sendbuf[1500] = {0};
	//unsigned char                           *sendbuf = gszAudioRtspBuf;
	int                             pLen;
	int                             offset[5] = {0};
	int                             ucFrameLenH = 0, ucFrameLenL = 0;
	int	                            bytes = 0;

	unsigned int                   mytime = 0;
	unsigned int                   ts_current_audio = 0;
	int                     			audio_sample = samplerate; //rtsp_stream_get_audio_samplerate();
	int audio_frame_len = 0;
	int temp_len = 0;

	char audio_puiNalBuf[2048] = {0};
	int  audio_nal_len = 0;
	int i = 0;
	int j = 0;
	//14 = rtp head len +2
	int mtu = rtp_mtu - 14;
	/*临时版本modify by zm  2012.04.27  */
	//int mtu = nLen + 12;
	int roomid = 0;
	unsigned int timetick = 0;
	int *seq = &(handle->seq_num);
//	printf("AUDIO nowtime =%u==%u\n",nowtime,g_time_org);
	//printf("nowtime=0x%x,g_time_org=0x%x\n",nowtime,g_time_org);
	if((handle->time_org)== 0 || nowtime < (handle->time_org)) {
		(handle->time_org) = nowtime;
	}

	timetick = nowtime - (handle->time_org);

	
//	nslog(NS_INFO,"<R_AUDIO>  ============== <NOW_TIME : %u>	<TIME_ORG : %u> <TIME_TICK : %u>\n",nowtime,handle->time_org,timetick);

	//printf("audio_sample is ---------------- %d\n",audio_sample);
	unsigned int framelen = 0;
	framelen = ((pData[3] & 0x03) << 9) | (pData[4] << 3) | ((pData[5] & 0xe0) >> 5);

//	printf("framlen is  %d ----- nLen : %d\n",framelen,nLen);
	//nslog(NS_ERROR,"pData[0] =%x,%x,%x,%x,len=%d\n",pData[0],pData[1],pData[2],pData[3],nLen)	;
	for(i = 0; i < nLen - 4; i++) {
		if(pData[i] == 0xff && pData[i + 1] == 0xf1)
		{
				//just 48KHZ/44.1KZH
			if((pData[i + 2] == 0x58) || (pData[i + 2] == 0x5c)||
				 (pData[i + 2] == 0x6c)|| (pData[i + 2] == 0x60)||
				 (pData[i + 2] == 0x4c)|| (pData[i + 2] == 0x50))
			{
				offset[j] = i;
				j++;
			}
		}
	}
	//	if((pData[i] == 0xff && pData[i + 1] == 0xf1 && pData[i + 2] == 0x58)
	//	   || (pData[i] == 0xff && pData[i + 1] == 0xf1 && pData[i + 2] == 0x5c)
	//	   || (pData[i] == 0xff && pData[i + 1] == 0xf1 && pData[i + 2] == 0x6c)
	//	   || (pData[i] == 0xff && pData[i + 1] == 0xf1 && pData[i + 2] == 0x60)
	//	   || (pData[i] == 0xff && pData[i + 1] == 0xf1 && pData[i + 2] == 0x4c)) {



	if(j > 1) {
		nslog(NS_ERROR,"RtspAudioPack j=%d=%d\n", j, nLen);
		//printf("pData[0] =%x,%x,%x,%x,len=%d\n",pData[0],pData[1],pData[2],pData[3],nLen)	;
		//printf("pData[0] =%x,%x,%x,%x,len=%d\n",pData[4],pData[5],pData[6],pData[7],nLen)	;
	}

	if(framelen == nLen && j >= 1) {
		j = 1;
	}

	// j 数据为几 就代表几个帧数据
	// 到这里代表传进来的数据是一帧数据 并且是 //just 48KHZ/44.1KZH   zhengyb
	for(i = 0; i < j; i++) {

		pLen = offset[i + 1] - offset[i];
	//	printf(" j : %d  i : %d \n",j,i);	//zhengyb
		if(i == j - 1) {
			pLen = nLen - offset[i];
		}
	//	printf("plen ----- %d\n",pLen);
		temp_len = audio_frame_len = pLen - 7;
		//nslog(NS_ERROR,"RtspAudioPack  temp_len = %d\n",temp_len);
		mytime = (unsigned int)timetick ;

		while(temp_len > 0) {
			if(temp_len >=  mtu) {
				//g_audio_puiNalBuf->len = mtu;
				audio_nal_len = mtu;
				//nslog(NS_ERROR,"temp_len = %d,mtu=%d\n", temp_len, mtu);
				//nslog(NS_ERROR,"i=%d,the len =%d=0x%x\n", i, g_audio_puiNalBuf->len, g_audio_puiNalBuf->len);
			} else {
				//g_audio_puiNalBuf->len = temp_len;
				audio_nal_len = temp_len;
			}

			ucFrameLenH = audio_frame_len / 32;
			ucFrameLenL = (audio_frame_len % 32) * 8;

		//	g_audio_puiNalBuf->buf[0] = 0;
		//	g_audio_puiNalBuf->buf[1] = 0x10;
		//	g_audio_puiNalBuf->buf[2] = ucFrameLenH;
		//	g_audio_puiNalBuf->buf[3] = ucFrameLenL;
			
			audio_puiNalBuf[0] = 0;
			audio_puiNalBuf[1] = 0x10;
			audio_puiNalBuf[2] = ucFrameLenH;
			audio_puiNalBuf[3] = ucFrameLenL;
			begin_len =  offset[i] + 7 + audio_frame_len - temp_len;
			r_memcpy(&audio_puiNalBuf[4], pData +begin_len , audio_nal_len);

			//intf("begin_len=%d,the len=%d\n",begin_len,g_audio_puiNalBuf->len);
			
			//temp_len -= 	g_audio_puiNalBuf->len;
			temp_len -= 	audio_nal_len;
			memset(sendbuf, 0, sizeof(sendbuf));

			//mytime += (0xffffffff/44.1-1000*120);
			
			if(audio_sample == 16000) {
				ts_current_audio = mytime * 16;
			} else if(audio_sample == 32000) {
				ts_current_audio = mytime * 32;
			} else if(audio_sample == 44100) {
				ts_current_audio = mytime * 44 +mytime/10;
			} else if(audio_sample == 96000) {
				ts_current_audio =  mytime * 96;
			} else {
				ts_current_audio = mytime * 48;
			}
			//printf("ts_current_audio =%lx,my=%x==%d\n",ts_current_audio,mytime,timetick);
			//if audio time is >0xffffffff,need reset 0;
			if(ts_current_audio >= 0XFFFFFFFF)
			{
			//	nslog(NS_ERROR,"Warnning,the ts_current_audio is too big =%lx\n",ts_current_audio);
				(handle->time_org) = 0;
			}
			//ts_current_audio = timetick;
			//	static int g_test_audio = 0;
			//	g_test_audio ++;
			//	if(g_test_audio %40 ==0)
			//	printf("audio timetick =0x%x,the ts_current_audio  =0x%x\n",timetick,ts_current_audio);
			//bytes = g_audio_puiNalBuf->len + 16 ;
			bytes = audio_nal_len + 16 + 16;

			#if 1
			// 修改
			RTP_FIXED_HEADER_AUDIO *p;
			p = (RTP_FIXED_HEADER_AUDIO *)sendbuf;
			//p->byte1 = 0x80;
			#if 1
			p->csrc_len 		= 0;
			p->extension 		= 1;
			p->padding   		= 0;
			p->version			= 2;
			#endif
			if(temp_len != 0) {
				p->byte2 = 0x61;
			} else {
				p->byte2 = 0xe1;
			}

			p->seq_no = htons((*seq)++);
			p->timestamp = htonl(ts_current_audio);
		//	nslog(NS_INFO,"<R_AUDIO>  ________________ <SEND_TIME_BEFORE : %u>	<SEND_TIME_AFTER : %u>\n",ts_current_audio,p->timestamp);
			p->ssrc = htonl(ssrc);
			#endif


			#if 1

			rtp_debug_head_t 					*debug_head;

			debug_head = (rtp_debug_head_t *)&sendbuf[12];
	
			debug_head->profile_defined			= htons(0);
			debug_head->length					= htons(3);
	
			#endif

			FRAMEHEAD							*frame_head;
			frame_head = (FRAMEHEAD *)&sendbuf[16];
			frame_head->codec     	= data_info->codec;
			frame_head->samplerate	= data_info->samplerate;
			frame_head->framerate 	= data_info->framerate;
		//	frame_head->height		= htons(data_info->height);
			frame_head->height		= htons(samplerate);
			frame_head->width		= htons(data_info->width);
			frame_head->Iframe		= data_info->Iframe;
			/* reserve为1时，接收端的采样率需要X2 */
			frame_head->reserve 	= 1;
			frame_head->framelength = htonl(data_info->framelength);

//			printf("frame_head->framelength is %d   ------------ %d\n",frame_head->framelength,data_info->framelength);
			#if 0
			printf("codec  ---- %d\n",frame_head->codec);
			printf("framerate  ---- %d\n",frame_head->framerate);
			printf("height  ---- %d\n",frame_head->height);
			printf("width  ---- %d\n",frame_head->width);
			printf("resever  ---- %d\n",frame_head->resever);
			printf("samplerate  ---- %d\n",frame_head->samplerate);
			#endif
			
			r_memcpy(&sendbuf[28], audio_puiNalBuf, audio_nal_len+ 4);

			if(j == 1) {
				//		SendRtspAudioData(sendbuf,bytes,roomid);
	//			SendMultCastAudioData(sendbuf, bytes,info);

	//			printf("send to ok!\n");
				if(send_rtp_video(info,sendbuf,bytes) < 0)
				{
					//printf("send_rtp_video is eror!\n");
					return -1;
				}
			} else {
				nslog(NS_ERROR,"ERROR!!!!SendRtspAudioData not send.\n");
			}
		
		}

	}

	//printf("audio bytes =%d,the len=%d\n",bytes,nLen);
	return 0;
}



int rtp_build_reset_time(RTP_BUILD_HANDLE handle)
{
	return 0;
	if(handle == NULL)
	{
		nslog(NS_ERROR,"ERROR,the handle is NULL\n");
		return -1;
	}
	handle->time_org = 0;
}


void *rtp_build_init(unsigned int ssrc, unsigned int payload)
{
	RTP_BUILD_HANDLE handle = NULL;
	handle = (RTP_BUILD_HANDLE) malloc(sizeof(RTP_BUILD_INFO));
	if(handle == NULL)
	{
	//	printf("Error,rtp build init failed\n");
		return NULL;
	}
	memset(handle,0,sizeof(RTP_BUILD_INFO));
	handle->ssrc = ssrc;
	handle->payload = payload;
	return handle;
}

int rtp_build_reset(RTP_BUILD_HANDLE handle)
{
	if(handle == NULL)
	{
		nslog(NS_ERROR,"ERROR,the handle is NULL\n");
		return -1;
	}
	handle->payload = 0;
	handle->seq_num = 0;
	handle->ssrc = 0;
	handle->time_org = 0;
	return 0;
	
}

void rtp_build_uninit(void **handle)
{
	if((*handle) != NULL)
	{
		free(*handle);
	}
	*handle= NULL;
	return ;
}

