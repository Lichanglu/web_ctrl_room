#ifndef _WEBINTERFACE_H
#define _WEBINTERFACE_H 



#include "../../enc/middle_control.h"


/*添加字幕*/
extern int Webaddtexttodisplay(TextInfo* info);

/*返回值0成功，-1失败*/
/*设置音频参数*/
//extern int WebSetAudioParam(AudioParam *inaudio,AudioParam *outaudio);

/*设置视频参数*/
//extern int WebSetVideoParam(VideoParam* invideo,VideoParam *outvideo);
/*设置系统参数*/
extern int WebSetSysParam(SYSPARAMS *insys,SYSPARAMS *outsys);
/*Web get input source*/
//extern int  WebGetinputSource(inputtype *outval,int channel);
/*切换输入源*/
//extern int WebSetinputSource(inputtype inval,inputtype *outval,int channel);
/*4.系统升级*/
extern int WebUpdateFile(char* filename);
/*上传logo 图片*/
extern int WebUploadLogoFile(char* filename,int channel);
/*5.系统重启*/
extern int webRebootSystem(void);
/*时间同步*/
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
//获取色彩空间值，YUV or RGB
//extern int webGetColorSpace(ColorSpace *inval,int channel);
//设置色彩空间值，
//extern int webSetColorSpace(ColorSpace inval,ColorSpace *outval,int channnel);

extern int webAddLOGODisplay(LogoInfo * info);
//获取设备型号。
extern int WebGetDevideType(char *outval,int len);
extern int WebSetDeviceType(const char *inval,char *outval);    // devicetype len must be 15 bit
//回复出厂设置
extern int restoreSet(void);
//获取动态ip标志。
//extern int webGetDHCPFlag(int *outval);
//设置是否动态获取ip。
//extern int webSetDHCPFlag(int inval,int *outval);
//偏色调整
int webSetCbCr(int *outval,int channel);

//获取H264高级编码参数。
extern int webGetH264encodeParam(H264EncodeParam *outval);
//H264高级编码参数设置。
extern int webSetH264encodeParam(H264EncodeParam *inval,H264EncodeParam *outval);
//获取视频输出参数。
int webGetOutputVideoParam(OutputVideoInfo *outval);
//设置输出视频参数。
extern int webSetOutputVideoParam(OutputVideoInfo *inval,OutputVideoInfo *outval);
//输入信号源的详细信息。
extern int webSignalDetailInfo(char *buf,int len,int channel);
//屏幕微调。
extern int webRevisePicture(short hporch,short vporch,int channel);
//获取输入信号源信息。
extern int webgetInputSignalInfo(char *outval,int channel);
//获取cpu 占用率。
extern int webGetCPULoad(int *outval);
//获取软件版本编译时间。
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
