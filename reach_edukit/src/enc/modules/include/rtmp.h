/*
**************************************************************************************
��������:  ����
1.void RtmpInit()
    ���ܽ���:
        �ú�����Ҫ���RTSP ���������ʼ��
            a.��ʼ�����ӿͻ���Ϣ
            b.��ʼ����
            c.������Ƶ���������ڴ�ռ�

2.void RtmpAddClient(struct sockaddr_in client_addr,int sock,int nPos)
    ���ܽ���:
        �ú�����Ҫ������ӿͻ���npos�������ַ
    ����˵��:
        client_addr:���ӵĿͻ��˵������ַ
        sock:���ӿͻ���socket
        nPos:Ҫ���ӵĿͻ��˵����
3.int  RtmpDelClient(int nPos)
    ���ܽ���:
        �ú������ڶϿ����ΪnPos�Ŀͻ���
    ����˵��:
        nPos:Ҫ�Ͽ��Ŀͻ��˵����
4.int  RtmpGetNullClientIndex()
    ���ܽ���:
        �ú������ڻ�ȡһ���յĿͻ��ţ����ͻ���Ϊ5(�������6���ͻ�)
5.int  RtmpGetClientNum()
    ���ܽ���:
        �ú�����Ҫ���ؿͻ��˵���Ŀ�������ж��Ƿ��пͻ�����
6.int  RtmpExit()
    ���ܽ���:
        �˳� RTMP Э��
            a.������
            b.�ͷŷ�����ڴ�    
7.void RtmpSetupConnection(int nPos)
	���ܽ���:
		�������n ���ͻ�֮�������
	����˵��:
		 nPos:Ҫ���ӵĿͻ��˵����
		 
8.RtmpAudioPack(int nLen, unsigned char * pData)
    ���ܽ���:
        �ú�����Ҫ�����Ƶ��rtmp���
    ����˵��:
        nLen:Ҫ�����aac��Ƶ����
        pData:Ҫ�����aac��Ƶ��ַ
        
9.RtmpVideoPack(int nLen, unsigned char *pData)
    ���ܽ���:
        �ú�����Ҫ�����Ƶ��rtmp���
    ����˵��:
        nLen:Ҫ�����264��Ƶ����
        pData:Ҫ�����264��Ƶ��ַ
10.char *RtmpGetVersion()
    ���ܽ���壺
        �ú�����Ҫ���ڻ�ȡrtmp Э��İ汾��
**************************************************************************************
*/

#define RTMP_LISTEN_PORT                1935// 4212//

#define PRINT_DEUBG

extern char *RtmpGetVersion();
extern int  RtmpGetClientNum();
extern void RtmpTask();



extern void RtmpAudioPack(int nLen, unsigned char *pData);
extern void RtmpVideoPack(int nLen, unsigned char *pData);

