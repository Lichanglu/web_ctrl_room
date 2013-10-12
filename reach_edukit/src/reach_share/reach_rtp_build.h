#ifndef _US_RTP_BUILD_H__
#define _US_RTP_BUILD_H__  //simple  udp_send  -->US
/*
**************************************************************************************
��������:
1.void RtpInit()
    ���ܽ���:
        �ú�����Ҫ���RTP ���������ʼ��
            a.��ʼ����
            b.������Ƶ���������ڴ�ռ�
2.void RtpSetUdp(int UdpVideoSocket, struct sockaddr_in UdpVideoAddr,int UdpAudioSocket, struct sockaddr_in UdpAudioAddr)
    ���ܽ���:
        �ú�������RTP�鲥socket�͵�ַ���ؽ�ʱ��� 
            a.������Ƶ�鲥��socket�͵�ַ
            b.������Ƶ�鲥��socket�͵�ַ
            c.�ؽ�ʱ���
    ����˵��:
        UdpVideoSocket:��Ƶ�鲥socket
        UdpVideoAddr:��Ƶ�鲥��ַ
        UdpAudioSocket:��Ƶ�鲥socket
        UdpAudioAddr:��Ƶ�鲥��ַ
3.RtpVideoPack(int nLen, unsigned char *pData, int nFlag, unsigned char index)
    ���ܽ���:
        �ú�����Ҫ�����Ƶ��rtp���
    ����˵��:
        nLen:Ҫ�����264��Ƶ����
        pData:Ҫ�����264��Ƶ��ַ
        nFlag:֡��־��=1 I֡��=0 P֡
4.RtpAudioPack(int nLen, unsigned char * pData, int nFlag, unsigned char index)
    ���ܽ���:
        �ú�����Ҫ�����Ƶ��rtp���
    ����˵��:
        nLen:Ҫ�����aac��Ƶ����
        pData:Ҫ�����aac��Ƶ��ַ
5.void RtpExit()
    ���ܽ���:
        �˳� RTP Э��
            a.������
            b.�ͷŷ�����ڴ� 
6.char *RtpGetVersion()
    ���ܽ���:
        ��ȡ��ǰrtp �汾��
**************************************************************************************
*/
	
// ֡ͷ��Ϣ
typedef struct __US_HDB_FRAME_HEAD 
{   
	
	unsigned char codec:4;          //��������  0--H264  1--ADTS  2-JPEG             
	unsigned char samplerate:4;   //������    ������ΪADTS ����4λΪ  0--16kbps 1--32kbps 2--44.1kbps 3--48kbps 4--96kbps��     
	unsigned char framerate;        //֡��            
	unsigned short width;           //ʵ�ʿ�             
	unsigned short height;           //ʵ�ʸ�
//	unsigned short resever;				// ����λ
	unsigned char Iframe;				// I ֡��־
	unsigned char reserve;				// ����λ 
	unsigned int  framelength;			// ֡��
}US_FRAMEHEAD;

//debug ������չͷ
typedef struct us_rtp_debug_head
{
	unsigned short 	profile_defined;
	unsigned short 	length;
}us_rtp_debug_head_t;




//  ��չͷ��Ϣ
typedef struct us_rtp_extension_head
{
	unsigned short 	profile_defined;
	unsigned short 	length;
	US_FRAMEHEAD 		frame_head;
	
}us_rtp_extension_head_t;


typedef struct _US_RTP_BUILD_INFO_
{
	unsigned int time_org;
	unsigned int video_seq_num;
	unsigned int audio_seq_num;
	unsigned int jpg_seq_num;
	unsigned int ssrc;
	unsigned int payload;
}US_RTP_BUILD_INFO;

typedef US_RTP_BUILD_INFO* US_RTP_BUILD_HANDLE;

int us_rtp_build_jpeg_data(US_RTP_BUILD_HANDLE handle,int nLen, unsigned char *pData,unsigned int ts_current,int rtp_mtu, void *info, US_FRAMEHEAD *data_info);
int us_rtp_build_audio_data(US_RTP_BUILD_HANDLE handle,int nLen, unsigned char *pData, int samplerate, int rtp_mtu, unsigned int nowtime,void *info,US_FRAMEHEAD *data_info);
int us_rtp_build_video_data(US_RTP_BUILD_HANDLE handle,int nLen, unsigned char *pData, int rtp_mtu, unsigned int nowtime, void *info,US_FRAMEHEAD *data_info);
void *us_rtp_build_init(unsigned int ssrc, unsigned int payload);
void us_rtp_build_uninit(void **handle);
int us_rtp_build_reset(US_RTP_BUILD_HANDLE handle);

int us_rtp_build_reset_time(US_RTP_BUILD_HANDLE handle);


#endif

