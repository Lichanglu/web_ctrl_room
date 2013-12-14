/*-------------------------------------------------------------------------
          Copyright (C), 2012-2013, Reach Tech. Co., Ltd.
          File name:
          Author: ���      Version:        Date: 2012.11.20 20:29:13
	      Description:
	      Function List:
	      History:
		    1. Date:
  			   Author:
  			   Modification:
-------------------------------------------------------------------------*/
#include "weblib.h"

/*==============================================================================
    ����: <WebGetDevSerialNumber>
    ����: ��ȡ�豸��Ϣ
    ����:
    ����ֵ:
    Created By ��� 2012.11.26 10:05:42 For Web
==============================================================================*/
int WebGetDevInfo(DevInfo *devinfo)
{
	int inlen  = 0;
	int outlen = 0;
	int ret;
	
	ret = appCmdStructParse(MSG_GETDEVINFO,NULL,inlen,devinfo,&outlen);
	return ret;
}


/*==============================================================================
    ����: <WebGetSysDiskSize>
    ����: ��ȡ����״̬
    ����:
    ����ֵ:
    Created By ��� 2012.11.26 10:05:26 For Web
==============================================================================*/
int WebGetSysDiskInfo(DiskInfo *diskinfo)
{
	int inlen  = 0;
	int outlen = 0;
	int ret;
	
	ret = appCmdStructParse(MSG_GETDISKINFO,NULL,inlen,diskinfo,&outlen);
	return ret;
}



/*==============================================================================
    ����: <WebGet6467Info>
    ����: 
    ����: 
    ����ֵ: 
    Created By ��� 2013.01.28 18:58:26 For Web
==============================================================================*/
int WebGet6467Info(Dm6467Info *dm6467info)
{
	int inlen  = 0;
	int outlen = 0;
	int ret;
	
	ret = appCmdStructParse(MSG_GET6467INFO,NULL,inlen,dm6467info,&outlen);
	return ret;
}




/*==============================================================================
    ����: <WebSetUsrPassword>
    ����: ��������
    ����:
    ����ֵ:
    Created By ��� 2012.11.26 10:05:19 For Web
==============================================================================*/
int WebSetUsrPassword(char *password)
{
	int inlen=strlen(password);
	int outlen = 0,ret = 0;
	char outval[64] = {0};
	ret = appCmdStringParse(MSG_SETUSRPASSWORD, password, inlen, outval,&outlen);
	return ret;
}

/*==============================================================================
    ����: <WebGetUsrPassword>
    ����: ��ȡ����
    ����:
    ����ֵ:
    Created By ��� 2012.11.26 10:05:19 For Web
==============================================================================*/
int WebGetUsrPassword(char *password)
{
	int inlen  = 0;
	int outlen = 0,ret = 0;
	ret = appCmdStringParse(MSG_GETUSRPASSWORD, NULL, inlen, password,&outlen);
	return ret;
}


/*==============================================================================
    ����: <WebSetEnChSwitch>
    ����: ��������
    ����:
    ����ֵ:
    Created By ��� 2012.11.26 10:05:05 For Web
==============================================================================*/
int WebSetEnChSwitch(int language)
{
	int inlen  = sizeof(int);
	int outval = 0;
	int outlen = sizeof(int);
	int ret;
	ret = appCmdIntParse(MSG_SETLANGUAGE, language, inlen, &outval,&outlen);
	return ret;
}


/*==============================================================================
    ����: <WebSetAudioType>
    ����: ������Ƶ
    ����: 
    ����ֵ: 
    Created By ��� 2013.01.28 18:59:54 For Web
==============================================================================*/
int WebSetAudioType(char * type)
{
	int inlen=strlen(type);
	int outlen = 0,ret = 0;
	char outval[64] = {0};
	ret = appCmdStringParse(MSG_SETAUDIOTYPE, type, inlen, outval,&outlen);
	return ret;
}


/*==============================================================================
    ����: <WebGetAudioType>
    ����: ��ȡ��Ƶ
    ����: 
    ����ֵ: 
    Created By ��� 2013.01.28 19:02:11 For Web
==============================================================================*/
int WebGetAudioType(char *type)
{
	int inlen  = 0;
	int outlen = 0,ret = 0;
	ret = appCmdStringParse(MSG_GETAUDIOTYPE, NULL, inlen, type,&outlen);
	return ret;
	return ret;
}


/*==============================================================================
    ����: <WebSetCtrlProto>
    ����: ����ԶңЭ��
    ����: 
    ����ֵ: 
    Created By ��� 2013.01.30 10:09:11 For Web
==============================================================================*/
int WebSetCtrlProto(char *outdex)
{
	int inlen=strlen(outdex);
	int outlen = 0,ret = 0;
	char outval[64] = {0};
	ret = appCmdStringParse(MSG_SET_CTRL_PROTO, outdex, inlen, outval,&outlen);
	return ret;
}

/*==============================================================================
    ����: <WebDelCtrlProto>
    ����: ɾ��ԶңЭ��
    ����: 
    ����ֵ: 
    Created By ��� 2013.01.30 10:09:01 For Web
==============================================================================*/
int WebDelCtrlProto(char *outdex)
{
	int inlen=strlen(outdex);
	int outlen = 0,ret = 0;
	char outval[64] = {0};
	ret = appCmdStringParse(MSG_DEL_CTRL_PROTO, outdex, inlen, outval,&outlen);
	return ret;
}



/*==============================================================================
    ����: <WebUpgradeCtrlProto>
    ����: �ϴ�ԶңЭ��
    ����: 
    ����ֵ: 
    Created By ��� 2013.01.30 10:08:54 For Web
==============================================================================*/
int WebUpgradeCtrlProto(char *outdex)
{
	int inlen=strlen(outdex);
	int outlen = 0,ret = 0;
	char outval[64] = {0};
	ret = appCmdStringParse(MSG_UPGRADE_CTRL_PROTO, outdex, inlen, outval,&outlen);
	return ret;
}


/*==============================================================================
    ����: <WebGetCtrlProto>
    ����: ��ȡԶңЭ��
    ����: 
    ����ֵ: 
    Created By ��� 2013.01.30 10:08:39 For Web
==============================================================================*/
int WebGetCtrlProto(ProtoList *outdex)
{
	int inlen  = 0;
	int outlen = 0;
	int ret;
	ret = appCmdStructParse(MSG_GET_CTRL_PROTO,NULL,inlen,outdex,&outlen);
	return ret;
}



/*==============================================================================
    ����: <WebGetEnChSwitch>
    ����:  ��ȡ����
    ����:
    ����ֵ:
    Created By  2012.11.26 10:05:05 For Web
==============================================================================*/
int WebGetEnChSwitch(int *language)
{
	int inlen  = 0;
	int outlen = sizeof(int);
	int ret;
	ret = appCmdIntParse(MSG_GETLANGUAGE, 0, inlen, language, &outlen);
	return ret;
}


/*==============================================================================
    ����: <WebSetIpconfig>
    ����: ����IP
    ����: 1 ���Զ� 0 ���ֶ�
    ����ֵ:
    Created By ��� 2012.11.26 10:04:54 For Web
==============================================================================*/
int WebSetLanIpconfig(IpConfig *ipconfig)
{
	int  inlen = sizeof(IpConfig);
	int  outlen = 0,ret = 0;
	char outval[64] = {0};
	ret = appCmdStructParse(MSG_SETLANIPCONFIG, ipconfig, inlen, outval, &outlen);
	return ret;
}


int WebSetWanIpconfig(IpConfig *ipconfig)
{
	int inlen = sizeof(IpConfig);
	int  outlen = 0,ret = 0;
	char outval[64] = {0};
	ret = appCmdStructParse(MSG_SETWANIPCONFIG, ipconfig, inlen, outval, &outlen);
	return ret;
}


int WebSetOsdconfig(OsdConfig *osdconfig)
{
	int inlen = sizeof(OsdConfig);
	int  outlen = 0,ret = 0;
	char outval[264] = {0};
	ret = appCmdStructParse(MSG_SETOSDCONFIG, osdconfig, inlen, outval, &outlen);
	return ret;
}

int WebSetLogoconfig(LogoConfig *logoconfig)
{
	int inlen = sizeof(LogoConfig);
	int  outlen = 0,ret = 0;
	char outval[264] = {0};
	ret = appCmdStructParse(MSG_SETLOGOCONFIG, logoconfig, inlen, outval, &outlen);
	return ret;
}



/*==============================================================================
    ����: <WebGetIpconfig>
    ����: ��ȡIP
    ����:
    ����ֵ:
    Created By ��� 2012.11.26 10:04:54 For Web
==============================================================================*/
int WebGetLanIpconfig(IpConfig *ipconfig)
{
	int inlen = sizeof(IpConfig);
	int  outlen = sizeof(IpConfig),ret = 0;
	ret = appCmdStructParse(MSG_GETLANIPCONFIG, NULL, inlen, ipconfig, &outlen);
	return ret;
}

int WebGetWanIpconfig(IpConfig *ipconfig)
{
	int inlen = sizeof(IpConfig);
	int  outlen = sizeof(IpConfig),ret = 0;
	ret = appCmdStructParse(MSG_GETWANIPCONFIG, NULL, inlen, ipconfig, &outlen);
	return ret;
}

/*==============================================================================
    ����: <WebSysUpgrade>
    ����: ϵͳ����
    ����:
    ����ֵ:
    Created By ��� 2012.11.26 10:04:49 For Web
==============================================================================*/
int WebSysUpgrade(char *filename)
{
	int inlen = strlen(filename);
	int outlen = 0,ret = 0;
	char outval[264] = {0};
	ret = appCmdStringParse(MSG_SYSUPGRADE, filename, inlen, outval,&outlen);
	return ret;
}

int WebSysUpgrade_6467(char *filename)
{
	int inlen = strlen(filename);
	int outlen = 0,ret = 0;
	char outval[264] = {0};
	ret = appCmdStringParse(MSG_SYSUPGRADE_6467, filename, inlen, outval,&outlen);
	return ret;
}


/*==============================================================================
    ����: <WebSysRollBack>
    ����: ϵͳ����
    ����:
    ����ֵ:
    Created By ��� 2012.11.26 10:04:29 For Web
==============================================================================*/
int WebSysRollBack(char *filename)
{
	int inlen = strlen(filename);
	int outlen = 0,ret = 0;
	char outval[264] = {0};
	ret = appCmdStringParse(MSG_SYSROLLBACK, filename, inlen, outval,&outlen);
	return ret;
}

int WebSetDevSerialnum(char *num)
{
	int inlen = strlen(num);
	int outlen = 0,ret = 0;
	char outval[264] = {0};
	ret = appCmdStringParse(MSG_SETSERIALNUM, num, inlen, outval,&outlen);
	return ret;
}

int WebRebootSys(int reboot)
{
	int inlen  = sizeof(int);
	int outval = 0;
	int outlen = sizeof(int);
	int ret;
	ret = appCmdIntParse(MSG_SYSREBOOT, reboot, inlen, &outval,&outlen);
	return ret;
}

