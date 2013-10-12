#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "weblib.h"
#include "webTcpCom.h"

#include "../../enc/middle_control.h"
#include "../../enc/input_to_channel.h"
#define KERNEL_VERSION_LENGTH		16



/********************input signal  function begin**********************************/
#if 0
/*WEB get MP status*/
int WebGetMpInfo(Mp_Info* info)
{
	int ret=0;
	Mp_Info inval ;
	int inlen=sizeof(Mp_Info);
	int outlen=0;
	int cmd=MSG_GET_MP_INFO;
	ret =appCmdStructParse(cmd,&inval,inlen,info,&outlen);
	return ret;
}

/*WEB set MP status*/
int WebSetMpInfo(Mp_Info *info)
{
	int ret=0;
	int inlen = sizeof(Mp_Info);
	int outlen=0;
	Mp_Info out;
	int cmd=MSG_SET_MP_INFO;
	ret =appCmdStructParse(cmd,info,inlen,&out,&outlen);
	return ret;
}
#endif

/*Web get input source*/
int  WebGetinputSource(int *outval,int channel)
{
	int inval = 0;
	int inlen=sizeof(inputtype);
	int outlen = 0,ret = 0;
	int cmd = MSG_GETINPUTTYPE;
	cmd = cmd | (((channel+1)&0xf)<<16) ;	
	ret =appCmdIntParse(cmd,inval,inlen,outval,&outlen);
	if(*outval == DVI-2)
		*outval = DVI;
	else if (*outval == SDI-2)
		*outval = SDI;
	return ret;
}


/*Set Input Source Mode*/
int WebSetinputSource(int inval,int *outval,int channel)
{
	int inlen=sizeof(int);
	int	outlen=sizeof(int);
	int ret;
	int cmd = MSG_SETINPUTTYPE;
	cmd = cmd | (((channel+1)&0xf)<<16) ;	
	if(inval == DVI )
		inval = DVI-2;
	else if (inval == SDI)
		inval = SDI-2;

	
	ret =appCmdIntParse(cmd,inval,inlen,outval,&outlen);
	return *outval;
}

/*set color space*/
int webSetColorSpace(int inval,int *outval,int channel)
{
	int inlen=sizeof(int);
	int outlen=sizeof(int);
	int ret;
	int cmd = MSG_SETCOLORSPACE;
	cmd = cmd | (((channel+1)&0xf)<<16) ;		
	ret =appCmdIntParse(cmd,inval,inlen,outval,&outlen);
	return *outval;
}

/* get input signal color spacce*/
int webGetColorSpace(int *outval,int channel)
{	
	int inval=0,inlen=sizeof(int);
	int outlen=sizeof(int);
	int ret;
	int cmd = MSG_GETCOLORSPACE;
	cmd = cmd | (((channel+1)&0xf)<<16) ;		
	ret =appCmdIntParse(cmd,inval,inlen,outval,&outlen);
	return *outval;
}

/*get input signal HDCP flag*/
int webGetHDCPFlag(int *outval,int channel)
{	
	int inval=0,inlen=sizeof(int);
	int outlen=sizeof(int);
	int ret;
	int cmd = MSG_GETHDCPVALUE;
	cmd = cmd | (((channel+1)&0xf)<<16) ;		
	ret =appCmdIntParse(cmd,inval,inlen,outval,&outlen);
	return *outval;
}
/*
int webSetCbCr(int *outval,int channel)
{	
	int inval=0,inlen=sizeof(int);
	int outlen=sizeof(int);
	int ret;
	int cmd = MSG_SETCBCR;
	cmd = cmd | (((channel+1)&0xf)<<16) ;		
	ret =appCmdIntParse(cmd,inval,inlen,outval,&outlen);
	return *outval;
}
*/
/*get input signal mode string info ,*/
int webgetInputSignalInfo(char *outval,int channel)
{
	int inlen=strlen(outval);
	int outlen = 0,ret = 0;
	int cmd = MSG_GETINPUTSIGNALINFO;
	cmd = cmd | (((channel+1)&0xf)<<16) ;		
	ret =appCmdStringParse(cmd,NULL,inlen,outval,&outlen);
	return ret;
}


/*signal hv revise*/
int webRevisePicture(short hporch,short vporch,int channel)
{
	int outval=0;
	int outlen=0;
	int ret;
	unsigned int in=0;
	int cmd = MSG_REVISE_PICTURE;
	cmd = cmd | (((channel+1)&0xf)<<16) ;
	
	in = (hporch<<16)|(vporch&0xffff);
	//memcpy(inval,&hporch,sizeof(hporch));
//	memcpy(inval+sizeof(short),&vporch,sizeof(vporch));
	ret = appCmdIntParse(cmd,in,sizeof(in),&outval,&outlen);
	return ret;
}



/*signal sdi  hv revise*//*
int webSDIRevisePicture(short hporch,short vporch,int channel)
{
	int outval=0;
	int outlen=0;
	int ret;
	unsigned int in=0;
	int cmd = MSG_SDI_REVISE_PICTURE;
	cmd = cmd | (((channel+1)&0xf)<<16) ;	
	in = (hporch<<16)|(vporch&0xffff);
	ret = appCmdIntParse(cmd,in,sizeof(in),&outval,&outlen);
	return ret;
}
*/
/*get signal detail info*/
int webSignalDetailInfo(char *buf,int inlen,int channel)
{
	int outlen,ret=0;
	int cmd = MSG_SIGNALDETAILINFO;
	cmd = cmd | (((channel+1)&0xf)<<16) ;
	ret = appCmdStringParse(cmd,NULL,inlen,buf,&outlen);
	return ret;
}

/*set signal SDIAdjust*//*
int webSetSignalSDIAdjust(int invalue,int channel)
{
	int outvalue=0,outlen=0,ret=0;
	int inlen = sizeof(int);
	int cmd = MSG_SETSIGNALSDIADJUST;
	cmd = cmd | (((channel+1)&0xf)<<16) ;
	ret = appCmdIntParse(cmd,invalue,inlen,&outvalue,&outlen);
	return ret;
}
*/
/*get signal SDIAdjust*//*
int webGetSignalSDIAdjust(int *outvalue,int channel)
{
	int invalue=0,outlen=0,ret=0;
	int inlen = sizeof(int);
	int cmd = MSG_GETSIGNALSDIADJUST;
	cmd = cmd | (((channel+1)&0xf)<<16) ;
	ret = appCmdIntParse(cmd,invalue,inlen,outvalue,&outlen);
	return ret;
}
*/

/********************input signal  function end**********************************/


/**************************video control begin**********************************/

/*获得放大参数表*/
int WebGetScaleParam(WebScaleInfo *info,int channel)
{
	int inlen=sizeof(WebScaleInfo);
	int outlen = 0,ret = 0;
	int cmd = MSG_GETSCALEINFO;
	cmd = cmd | (((channel+1)&0xf)<<16) ;	
	ret =appCmdStructParse(cmd,NULL,inlen, (void*)info,&outlen);
	return ret;
}

/*设置放大参数*/
int WebSetScaleParam(WebScaleInfo *ininfo,WebScaleInfo *outinfo,int channel)
{
	int inlen=sizeof(WebScaleInfo);
	int outlen = 0,ret = 0;
	int cmd = MSG_SETSCALEINFO;
	cmd = cmd | (((channel+1)&0xf)<<16) ;	
	
	ret =appCmdStructParse(cmd,ininfo,inlen, (void*)outinfo,&outlen);
	return ret;
}

/*获得视频编码参数表*/
int WebGetVideoEncodeParam(WebVideoEncodeInfo *info,int channel)
{
	int inlen=sizeof(WebVideoEncodeInfo);
	int outlen = 0,ret = 0;
	int cmd = MSG_GETVIDEOENCODEINFO;
	cmd = cmd | (((channel+1)&0xf)<<16) ;	
		
	ret =appCmdStructParse(cmd,NULL,inlen, (void*)info,&outlen);
	return ret;
}

/*设置视频编码参数*/
int WebSetVideoEncodeParam(WebVideoEncodeInfo *ininfo,WebVideoEncodeInfo *outinfo,int channel)
{
	int inlen=sizeof(WebVideoEncodeInfo);
	int outlen = 0,ret = 0;
	int cmd = MSG_SETVIDEOENCODEINFO;
	cmd = cmd | (((channel+1)&0xf)<<16) ;	
	ret =appCmdStructParse(cmd,ininfo,inlen, (void*)outinfo,&outlen);
	return ret;
}


/**************************video control end**********************************/


/**************************audio control begin**********************************/

/*获得音频编码参数表*/
int WebGetAudioEncodeParam(WEB_AudioEncodeParam *info,int channel)
{
	int inlen=sizeof(WEB_AudioEncodeParam);
	int outlen = 0,ret = 0;
	int cmd = MSG_GETAUDIOPARAM;
	cmd = cmd | (((channel+1)&0xf)<<16) ;	
		
	ret =appCmdStructParse(cmd,NULL,inlen, (void*)info,&outlen);
	return ret;
}

/*设置音频编码参数*/
int WebSetAudioEncodeParam(WEB_AudioEncodeParam *ininfo,WEB_AudioEncodeParam *outinfo,int channel)
{
	int inlen=sizeof(WEB_AudioEncodeParam);
	int outlen = 0,ret = 0;
	int cmd = MSG_SETAUDIOPARAM;
	cmd = cmd | (((channel+1)&0xf)<<16) ;	
	ret =appCmdStructParse(cmd,ininfo,inlen, (void*)outinfo,&outlen);
	return ret;
}



/**************************audio control begin**********************************/


/**************************text/logo control begin ****************************/

/*获得TEXT,LOGO开启参数表*/
int WebGetEnabelTextParam(WebEnableTextInfo *info,int channel)
{
	int inlen=sizeof(WebEnableTextInfo);
	int outlen = 0,ret = 0;
	int cmd = MSG_GETENABLETEXTINFO;

	ret =appCmdStructParse(cmd,NULL,inlen, (void*)info,&outlen);
	return ret;
}

/*设置TEXT,LOGO开启参数*/
int WebSetEnabelTextParam(WebEnableTextInfo *ininfo,WebEnableTextInfo *outinfo,int channel)
{
	int inlen=sizeof(WebEnableTextInfo);
	int outlen = 0,ret = 0;
	int cmd = MSG_SETENABLETEXTINFO;
	ret =appCmdStructParse(cmd,ininfo,inlen, (void*)outinfo,&outlen);
	return ret;
}


/*设置TEXT*/
int WebSetTextOsd(TextInfo *ininfo,TextInfo *outinfo,int channel)
{
	int inlen=sizeof(TextInfo);
	int outlen = 0,ret = 0;
	int cmd = MSG_SET_TEXTINFO;
	
	ret =appCmdStructParse(cmd,ininfo,inlen, (void*)outinfo,&outlen);
	return ret;
}

/*设置TEXT*/
int WebGetTextOsd(TextInfo *ininfo,TextInfo *outinfo,int channel)
{
	int inlen=sizeof(TextInfo);
	int outlen = 0,ret = 0;
	int cmd = MSG_GET_TEXTINFO;	
	ret =appCmdStructParse(cmd,ininfo,inlen, (void*)outinfo,&outlen);
	return ret;
}



/*set LOGO */
int WebSetLogoOsd(LogoInfo *ininfo,LogoInfo *outinfo,int channel)
{
	int inlen=sizeof(LogoInfo);
	int outlen = 0,ret = 0;
	int cmd = MSG_SET_LOGOINFO;
	
	ret =appCmdStructParse(cmd,ininfo,inlen, (void*)outinfo,&outlen);
	return ret;
}


/*GET logo*/
int WebGetLogoOsd(LogoInfo *ininfo,LogoInfo *outinfo,int channel)
{
	int inlen=sizeof(LogoInfo);
	int outlen = 0,ret = 0;
	int cmd = MSG_GET_LOGOINFO;
	
	ret =appCmdStructParse(cmd,ininfo,inlen, (void*)outinfo,&outlen);
	return ret;
}


//获得text/logo最大的xy坐标
int WebGetMaxPos(int channel,int* max_x_pos,int *max_y_pos)
{
//	int input = SIGNAL_INPUT_1;
//	int high = HIGH_STREAM;
//	int width,height;
	int out=0;
	int in=0;

	int inlen=sizeof(in);
	int outlen = 0,ret = 0;
	int cmd = MSG_GET_MAXPOS;

	ret =appCmdIntParse(cmd,in,inlen,&out,&outlen);
	if(out < 1)
	{
		*max_x_pos=1856;
		*max_y_pos=960;
	}

	*max_x_pos  = (out>>16)&0xffff;
	*max_y_pos =  out&0xffff;
//	*out = (x_pos<<16) | y_pos;
	//PRINTF("max_x_pos=%d,max_y_pos");
	return 0;
}


int WebGetTextPos(int type,int *x,int *y)
{
	if(type == TOP_LEFT)
	{
		*x = 0;
		*y = 0;
	}
	else if(type == TOP_RIGHT)
	{
		*x = 1920-64;
		*y = 0;		
	}
	else if(type == BOTTOM_LEFT)
	{
		*x = 0;
		*y = 1080-64 ;			
	}
	else if(type == BOTTOM_RIGHT)
	{
		*x = 1920 -64;
		*y = 1080-64 ;		
	}
	else if(type == CENTERED)
	{
		*x = (1920 - 16*10)/2;
		*y = 1080/2 -8;
	}
	else
	{
		*x = 0;
		*y = 0;	
	}
	return 0;
}

int WebGetLogoPos(int type,int *x,int *y)
{
	if(type == TOP_LEFT)
	{
		*x = 0;
		*y = 0;
	}
	else if(type == TOP_RIGHT)
	{
		*x = 1920 ;
		*y = 0;		
	}
	else if(type == BOTTOM_LEFT)
	{
		*x = 0;
		*y = 1080;			
	}
	else if(type == BOTTOM_RIGHT)
	{
		*x = 1920;
		*y = 1080 ;		
	}
	else if(type == CENTERED)
	{
		*x = (1920 )/2;
		*y = 1080/2 ;
	}
	else
	{
		*x = 0;
		*y = 0;	
	}
	return 0;
}

/**************************text/logo control begin ****************************/


/**************************far control begin**********************************/

/*remote control protocol*/
int WebGetCtrlProto(int *outdex,int channel)
{
	int index = 0;
	int inlen=sizeof(int);
	int	outlen=sizeof(int);
	int ret;
	int cmd=MSG_GET_CTRL_PROTO;
	cmd = cmd | (((channel+1)&0xf)<<16) ;
	
	ret =appCmdIntParse(cmd,index,inlen,outdex,&outlen);
	return ret;
}


/*set control protocol*/
int WebSetCtrlProto(int index,int *outdex,int channel)
{
	int inlen=sizeof(int);
	int	outlen=sizeof(int);
	int ret;
	int cmd=MSG_SET_CTRL_PROTO;	
	cmd = cmd | (((channel+1)&0xf)<<16) ;
	ret =appCmdIntParse(cmd,index,inlen,outdex,&outlen);
	return ret;
}

/*com far ctrl camera*/
int webFarCtrlCamera(int addr,int type,int speed,int channel)
{
	char inval[12]={0};
	int inlen=sizeof(inval);
	int outlen = 0,ret = 0;
	char outval[64]={0};
	int cmd = MSG_FAR_CTRL;
	memcpy(inval,&type,sizeof(type));
	memcpy(inval+sizeof(type),&speed,sizeof(speed));
	memcpy(inval+sizeof(type)+sizeof(speed),&addr,sizeof(addr));
	cmd = cmd | (((channel+1)&0xf)<<16) ;
	ret =appCmdStringParse(cmd,inval,inlen,outval,&outlen);
	return ret;
}

/**************************far control end************************************/

/**************************network control begin**********************************/

/**************************network control begin**********************************/




/*reboot */
int webRebootSystem(void)
{
	int inval=0,inlen=sizeof(int);
	int outval=0,outlen=sizeof(int);
	int ret;
	
	ret =appCmdIntParse(MSG_REBOOTSYS,inval,inlen,&outval,&outlen);
	return ret;
}


/*
int webGetSysInfo(Enc2000_Sys* out,int *outlen)
{
	int ret =0;
	int cmd = MSG_GETSYSPARAM;
	ret = appCmdStructParse(cmd, NULL, sizeof(Enc2000_Sys), out, outlen);
	return ret;
}
*/

/*Web Sync time*/
int Websynctime(DATE_TIME_INFO* inval,DATE_TIME_INFO *outval)
{
	int inlen=sizeof(DATE_TIME_INFO);
	int outlen = 0,ret = 0;
	ret =appCmdStructParse(MSG_SYNCTIME,inval,inlen,outval,&outlen);
	return ret;
}

/*web get encode time*/
int WebgetEncodetime(DATE_TIME_INFO *outval)
{
	int inlen=sizeof(DATE_TIME_INFO);
	int outlen = 0,ret = 0;
	ret =appCmdStructParse(MSG_GETENCODETIME,NULL,inlen,outval,&outlen);
	return ret;
}


#if 0
int webGetDHCPFlag(int *outval)
{
	int inval = 0;
	int inlen=sizeof(int);
	int outlen = 0,ret = 0;
	ret =appCmdIntParse(MSG_GETDHCPFLAG,inval,inlen, outval,&outlen);
	return ret;
}

int webSetDHCPFlag(int inval,int *outval)
{
	int inlen=sizeof(int);
	int outlen = 0,ret = 0;
	ret =appCmdIntParse(MSG_SETDHCPFLAG,inval,inlen,outval,&outlen);
	return ret;
}

#endif

/*Restroe to faction mode*/
int Websetrestore(void)
{
	int inlen=sizeof(int);
	int outlen = 0,ret = 0;
	int inval = 0;
	int outval=0;
	ret =appCmdIntParse(MSG_RESTORESET,inval,inlen,&outval,&outlen);
	return ret;
}


/*get encode type,ex,enc1260/enc2000*/
int WebGetDevideType(char *outval,int inlen)
{
	int outlen = 0,ret = 0;
//	int inval = 0;
	ret =appCmdStringParse(MSG_GETDEVICETYPE,NULL,inlen,outval,&outlen);
	return ret;
}


/*ret: 0 {success}, -1 {file error} ,-2 {update failed}  */
int WebUpdateFile(char* filename)
{
	int inlen = strlen(filename);
	int outlen=0;
	int ret;
	char outval[256] = {0};
	ret = appCmdStringParse(MSG_UPDATESYS, filename,inlen,outval ,&outlen);
	return ret;
}

/*update logo file */
int WebUploadLogoFile(char* filename,int channel)
{
	int inlen = strlen(filename);
	int outlen=0;
	int ret;
	char outval[256] = {0};
	int cmd = MSG_UPLOADIMG;
	cmd = cmd | (((channel+1)&0xf)<<16) ;
	
	ret = appCmdStringParse(cmd, filename,inlen,outval ,&outlen);
	return ret;
}

/*just for test channel*/
int webSetChannel(int channel)
{
	int inlen=sizeof(int);
	int outlen=sizeof(int);
	int outval = 0;
	int ret;
	ret =appCmdIntParse(MSG_CHANNEL_NUM,channel,inlen,&outval,&outlen);
	return outval;
}

/*
int WebSetSerialNo(char* serialNo,int inlen)
{
	int outlen=0;
	int ret;
	char outval[256] = {0};
	int cmd = MSG_SETSERIALNO;
	
	ret = appCmdStringParse(cmd, serialNo,inlen,outval ,&outlen);
	return ret;
}*/

int WebSetSwmsLayout(Moive_Info_t in)
{
	int inlen=sizeof(Moive_Info_t);
	int outlen=sizeof(Moive_Info_t);
	Moive_Info_t outval ;
	int ret =appCmdStructParse(MSG_SET_SWMS_LAYOUT,&in,inlen,&outval,&outlen);
	return ret;
}
int WebGetSwmsLayout(Moive_Info_t* out)
{
	Moive_Info_t  inval ;
	int inlen=sizeof(Moive_Info_t);
	int outlen=0;
	int ret =appCmdStructParse(MSG_GET_SWMS_LAYOUT,&inval,inlen,out,&outlen);
	return ret;
}

int WebSetHDMIRes(int res)
{
	int len = sizeof( int);
	int inlen=sizeof(int);
	int outlen=sizeof(int);
	int outval = 0;
	int ret;
	ret =appCmdIntParse(MSG_SET_HDMI_RES,res,inlen,&outval,&outlen);
	return outval;
}

int WebGetHDMIRes(int* res)
{
	int inval = 0;
	int inlen=sizeof(int);
	int outlen=sizeof(int);
	int outval = 0;
	int ret;
	ret =appCmdIntParse(MSG_GET_HDMI_RES,inval,inlen,res,&outlen);
	return ret;
}

int WebGetSystemInfo(System_Info_t *ininfo,System_Info_t *outinfo)
{
	int inlen=sizeof(System_Info_t);
	int outlen = 0,ret = 0;
	int cmd = MSG_GETSYSPARAM;	
	ret =appCmdStructParse(cmd,ininfo,inlen, (void*)outinfo,&outlen);
	return ret;
}

