#ifndef _BKFS_MIDDLE_H
#define _BKFS_MIDDLE_H

//判断命令字段
#define 	WEB_IDENTIFIER 			0xaaaaaaaa
#define 	TCP_SDK_IDENTIFIER      0xaaaaaaab


#define WEBSERVER  		    (20010)
#define SDK_LISTEN_PORT		(4000)
 
#define ENCODESERVER_PORT WEBSERVER


//#####################系统信息######################
#define MSG_GETDEVINFO						(0X1000)
#define MSG_GETDISKINFO						(0X1001)
#define MSG_GET6467INFO						(0X1002)
		


//###################################################
#define MSG_SETUSRPASSWORD					(0X2000)
#define MSG_GETUSRPASSWORD					(0X2001)
#define MSG_SETLANGUAGE					    (0X2002)
#define MSG_GETLANGUAGE					    (0X2003)

#define MSG_SETLANIPCONFIG                     (0X2004)
#define MSG_GETLANIPCONFIG                     (0X2005)

#define MSG_SETWANIPCONFIG                     (0X2006)
#define MSG_GETWANIPCONFIG                     (0X2007)


#define MSG_GETAUDIOTYPE                       (0X2011)
#define MSG_SETAUDIOTYPE                       (0X2012)


#define MSG_SET_CTRL_PROTO					   (0X2013)
#define MSG_GET_CTRL_PROTO					   (0x2014)
#define MSG_DEL_CTRL_PROTO					   (0X2015)
#define MSG_UPGRADE_CTRL_PROTO				   (0X2016)
#define MSG_SETOSDCONFIG					   (0X2017)
#define MSG_SETLOGOCONFIG					   (0X2018)

//#####################系统升级######################
#define MSG_SYSUPGRADE						(0X2008)
#define MSG_SYSROLLBACK						(0X2009)
#define MSG_SYSUPGRADE_6467                  (0X3008)
#define MSG_SYSUPGRADE_ALL                  (0X4008)
//#####################系统重启######################
#define MSG_SYSREBOOT						(0X200a)

#define MSG_SETSERIALNUM					(0X200b)



//####################服务端错误返回码######################
#define    			SERVER_RET_OK 								0X00
#define        		SERVER_RET_UNKOWN_CMD 						0xf01
#define        		SERVER_RET_INVAID_PARM_LEN  				0xf02               //编码器通信结构体长度不一致，需要查对应通信的头文件
#define        		SERVER_RET_INVAID_PARM_VALUE  				0xf03
#define        		SERVER_RET_SOCK_MAX_NUM  			        0xf04
#define        		SERVER_RET_RECV_FAILED  			        0xf05
#define        		SERVER_RET_SEND_FAILED  			        0xf06
#define        		SERVER_RET_ID_ERROR  					    0xf07
#define       		SERVER_RET_USER_INVALIED  		            0xf08 
#define 			SERVER_INTERNAL_ERROR					    0xf09
#define             SERVER_VERIFYFILE_FAILED				    0XF0A
#define             SERVER_SYSUPGRADE_FAILED				    0XF0B
#define             SERVER_SYSROLLBACK_FAILED					0xF0C
#define             SERVER_GETDEVINFO_FAILED       				0xF0D       
#define             SERVER_HAVERECORD_FAILED					0xF0E

//#####################客户端错误返回码######################
#define 				CLIENT_RETSUCCESSVALUE 					0
#define 				CLIENT_ERR_UNKNOWCMD					0xf1
#define					CLIENT_ERR_PARAMLEN						0xf2
#define 				CLIENT_ERR_TCPCONNECT					0xf3
#define   				CLIENT_ERR_TCPSEND						0xf4
#define 				CLIENT_ERR_TCPRECV						0xf5
#define 				CLIENT_ERR_PARAMSVALUE					0xf6
#define  				CLIENT_ERR_USERNAME						0xf7
#define 				CLIENT_ERR_PASSWORD						0xf8



#define MSGINFOHEAD			sizeof(webMsgInfo)


typedef enum __webparsetype {
    INT_TYPE = 0,
    STRING_TYPE,
    STRUCT_TYPE,
} webparsetype;


typedef struct __webMsgInfo{
	int 			identifier;  //0xaaaaaaaa 标识一个有效数据
	int				len;      //总长
	webparsetype	type;
} webMsgInfo;

typedef struct _dev_info
{
	char serialnum[64];
	char devmodel[64];
	char sysversion[64];
	char serverip[20];
	char mediactrip[20];
	char manageplatformip[20];
	char kernelversion[20];
}DevInfo;

typedef struct _disk_info
{
	unsigned int disksize;
	unsigned int remainsize;
}DiskInfo;

typedef struct _dm6467_info
{
	char sysversion[64];
	char uImageversion[64];
	char fpgaversion[64];
	
}Dm6467Info;

typedef struct _Proto_info
{
	char curproto[20];
	int  num;
	char protolist[15][20];
}ProtoList;

typedef struct _ip_config
{
	int  iptype;
	char ipaddr[20];
	char netmask[20];
	char gateway[20];
}IpConfig;

typedef struct _osd_config
{
	int  input;
	int  TitlePositon;
	int  DisTime;
	char Title[200];
}OsdConfig;

typedef struct _logo_config
{
	int  input;
	int  LogoPositon;
	char logoname[256];
}LogoConfig;

#endif
