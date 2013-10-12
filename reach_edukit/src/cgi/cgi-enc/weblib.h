#ifndef _WEBINTERFACE_H
#define _WEBINTERFACE_H 



#include "../../enc/middle_control.h"


/*�����Ļ*/
extern int Webaddtexttodisplay(TextInfo* info);

/*����ֵ0�ɹ���-1ʧ��*/
/*������Ƶ����*/
//extern int WebSetAudioParam(AudioParam *inaudio,AudioParam *outaudio);

/*������Ƶ����*/
//extern int WebSetVideoParam(VideoParam* invideo,VideoParam *outvideo);
/*����ϵͳ����*/
extern int WebSetSysParam(SYSPARAMS *insys,SYSPARAMS *outsys);
/*Web get input source*/
//extern int  WebGetinputSource(inputtype *outval,int channel);
/*�л�����Դ*/
//extern int WebSetinputSource(inputtype inval,inputtype *outval,int channel);
/*4.ϵͳ����*/
extern int WebUpdateFile(char* filename);
/*�ϴ�logo ͼƬ*/
extern int WebUploadLogoFile(char* filename,int channel);
/*5.ϵͳ����*/
extern int webRebootSystem(void);
/*ʱ��ͬ��*/
extern int Websynctime(DATE_TIME_INFO* inval,DATE_TIME_INFO *outval);

//extern void WebGetkernelversion(char* version);
//extern void WebGetfpgaversion(char* version);
extern int WebSaveParam(void);

/*web far control start*/
extern int webFarCtrlCamera(int addr,int type,int speed,int );
/*remote control protocol*/
extern int WebGetCtrlProto(int *,int);
/*set control protocol*/
extern int WebSetCtrlProto(int index,int *outdex,int);
/*web revise picture*/
extern int WebRevisePicture(short hporch,short vporch);
//extern int WebBlueRevise(void);
//��ȡɫ�ʿռ�ֵ��YUV or RGB
//extern int webGetColorSpace(ColorSpace *inval,int channel);
//����ɫ�ʿռ�ֵ��
//extern int webSetColorSpace(ColorSpace inval,ColorSpace *outval,int channnel);

extern int webAddLOGODisplay(LogoInfo * info);
//��ȡ�豸�ͺš�
extern int WebGetDevideType(char *outval,int len);
extern int WebSetDeviceType(const char *inval,char *outval);    // devicetype len must be 15 bit
//�ظ���������
extern int restoreSet(void);
//��ȡ��̬ip��־��
//extern int webGetDHCPFlag(int *outval);
//�����Ƿ�̬��ȡip��
//extern int webSetDHCPFlag(int inval,int *outval);
//ƫɫ����
int webSetCbCr(int *outval,int channel);

//��ȡH264�߼����������
extern int webGetH264encodeParam(H264EncodeParam *outval);
//H264�߼�����������á�
extern int webSetH264encodeParam(H264EncodeParam *inval,H264EncodeParam *outval);
//��ȡ��Ƶ���������
int webGetOutputVideoParam(OutputVideoInfo *outval);
//���������Ƶ������
extern int webSetOutputVideoParam(OutputVideoInfo *inval,OutputVideoInfo *outval);
//�����ź�Դ����ϸ��Ϣ��
extern int webSignalDetailInfo(char *buf,int len,int channel);
//��Ļ΢����
extern int webRevisePicture(short hporch,short vporch,int channel);
//��ȡ�����ź�Դ��Ϣ��
extern int webgetInputSignalInfo(char *outval,int channel);
//��ȡcpu ռ���ʡ�
extern int webGetCPULoad(int *outval);
//��ȡ����汾����ʱ�䡣
extern int webGetSoftConfigTime(void);
//extern int WebGetPHYmod(void);
//extern int WebSetPHYmod(int mod);


int webGetColorSpace(int *outval,int channel);
int webGetHDCPFlag(int *outval,int channel);
int  WebGetinputSource(int *outval,int channel);
//int WebGetMpInfo(Mp_Info* info);
int WebGetScaleParam(WebScaleInfo *info,int channel);
int WebGetVideoEncodeParam(WebVideoEncodeInfo *info,int channel);
int WebSetScaleParam(WebScaleInfo *ininfo,WebScaleInfo *outinfo,int channel);
int WebSetVideoEncodeParam(WebVideoEncodeInfo *ininfo,WebVideoEncodeInfo *outinfo,int channel);
//int WebGetAudioEncodeParam(WEB_AudioEncodeParam *info,int channel);
//int WebSetAudioEncodeParam(WEB_AudioEncodeParam *ininfo,WEB_AudioEncodeParam *outinfo,int channel);

int webSetChannel(int channel);
int WebGetTextOsd(TextInfo *ininfo,TextInfo *outinfo,int channel);
int WebGetLogoOsd(LogoInfo *ininfo,LogoInfo *outinfo,int channel);
int WebGetMaxPos(int channel,int* max_x_pos,int *max_y_pos);
int WebGetTextPos(int type,int *x,int *y);
int WebGetLogoPos(int type,int *x,int *y);
int WebGetLogoPos(int type,int *x,int *y);
int WebSetLogoOsd(LogoInfo *ininfo,LogoInfo *outinfo,int channel);
//int webGetSysInfo(Enc2000_Sys* out,int *outlen);
int WebgetEncodetime(DATE_TIME_INFO *outval);
int webGetEncodingFrofile(int *status);
int webGetScaleMode(int *status);
int Websetrestore(void);
//int WebSetMpInfo(Mp_Info *info);
int webSetEncodingFrofile(int status);
int webSetScaleMode(int status);
int WebSetSerialNo(char* serialNo,int inlen);
int WebSetTextOsd(TextInfo *ininfo,TextInfo *outinfo,int channel);
int WebSetinputSource(int inval,int *outval,int channel);
int webSetColorSpace(int inval,int *outval,int channel);

int webSDIRevisePicture(short hporch,short vporch,int channel);

#endif
