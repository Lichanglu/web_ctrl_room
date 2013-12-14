       /* cgi.c */
#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>		/* time() */
#include <fcntl.h>
#include <sys/stat.h>
#define LINUX
#ifdef LINUX
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#endif
//

#include "htmllib.h"
#include "cgic.h"
#include "Tools.h"
#include "cgi.h"
#include "weblib.h"
#include "../../enc/middle_control.h"
#include "../../enc/input_to_channel.h"
#include "webTcpCom.h"
#include "../../control/web/webmiddle.h"
//#include "../../control/command_resolve.h"
#include "../../enc/stream_output/stream_output_struct.h"
//#include "app_web_stream_output.h"




#define ACTION_START 100
#define ACTION_UPLOADFILE  ACTION_START+10
#define ACTION_LOGIN  ACTION_START+20
#define ACTION_QUERYSYSPARAM  ACTION_START+30
#define ACTION_CHANGEPWD  ACTION_START+40
#define ACTION_NETWORKSET  ACTION_START+50
#define ACTION_UPDATEVIDEOPARAM  ACTION_START+60
#define ACTION_QUERYAVPARAM  ACTION_START+70
#define ACTION_UPDATEAUDIOPARAM  ACTION_START+80
#define ACTION_TEXT_SET  ACTION_START+90
#define ACTION_LOGOUT  ACTION_START+100

//added by Adams begin.
#define PAGE_LEFTMENU_SYSPARAMS_SHOW 301
#define PAGE_LEFTMENU_SYSSET_SHOW 302
#define PAGE_SYSINFO_SHOW 303
#define PAGE_INPUT_SHOW 304
#define PAGE_MOVIEMODEL_SHOW 305
#define PAGE_TRACKINGSET_SHOW 306
#define PAGE_CAPTIONLOGO_SHOW 308
#define PAGE_REMOTECTRL_SHOW 307
#define PAGE_NETWORK_SHOW 310
#define PAGE_MODIFYPASSWORD_SHOW 311
#define PAGE_NetControlIp_SHOW 312
#define PAGE_SYSUPGRADE_SHOW 313
#define PAGE_FTP_SHOW 314
#define PAGE_INPUTDETAILS_SHOW 315
#define PAGE_RTSP_PLAYURL_SHOW 321
#define PAGE_FILEMGR_SHOW 323

#define ACTION_MOVIEMODEL_SET1 401
#define ACTION_MOVIEMODEL_SET2 402
#define ACTION_TRACKINGSET_SET 403
#define ACTION_REMOTECTRL_SET1 404
#define ACTION_REMOTECTRL_SET2 405
#define ACTION_NETWORK_SET 406
#define ACTION_INPUT_SET 407
#define ACTION_INPUTDETAILS_GET 408
#define ACTION_ADJUSTSCREEN_SET 409
#define ACTION_AUDIO_SET 410
#define ACTION_CAPTIONLOGO_SET 411
#define ACTION_PTZ_CTRL 413
#define ACTION_SYSUPGRADE_STATUS 414
#define ACTION_VIDEOADVANCEDSET_SET 415
#define ACTION_FILEMGR_DELETE 416
#define ACTION_FILEMGR_USBCOPY 417
#define ACTION_SETDEFAULT_SET 418
#define ACTION_UPLOADLOGOPIC_SET 419
#define ACTION_DOWNLOAD_SDP 420
#define ACTION_MERGE_SET 421
#define ACTION_ENCADVANCED_SET 422
#define ACTION_SERIALNO_SET 423
#define ACTION_CBCR_SET 424
#define ACTION_INPUTTYPE_GET 425
#define ACTION_NETCONTROL_IP_SET 433
#define ACTION_FTP_SET 435
#define ACTION_USB_SHOW 436
#define ACTION_USBSTATUS_GET 437
#define ACTION_USBCOPY 438
#define ACTION_FTPTOOL_DOWNLOAD 439
#define PAGE_FILEMGR_FILELIST 440
#define ACTION_GETCOPYSTATUS 441
#define ACTION_GETFTPUSERPW 442

#define RESULT_SUCCESS 1
#define RESULT_FAILURE 2
#define RESULT_MUSTMULTIIP 3
#define RESULT_NO_ANSWER (-2)
//added by Adams end.


#define PAGE_LOGIN_SHOW 201
#define PAGE_TIMEOUT_SHOW 202
#define PAGE_TOP_SHOW 203
#define PAGE_DOWN_SHOW 204
#define PAGE_MAIN_SHOW 205
#define PAGE_SYSTEM_INFO 206
#define PAGE_SYSTEM_SHOW 207
#define PAGE_PARAMETER_SHOW 208
#define SAVE_PLAY_CFG 209
#define TVOD_FILE 210
#define TVOD_OPER 211
#define SAVE_SHOW_MODE 212
#define SAVE_OUTPUT 213

#define RESTART_SHOW 214
#define RESTART_SYS 215
#define SERVER_SHOW 216
#define SERVER_LIST 217
#define SERVER_ADD_SHOW 218
#define SERVER_ADD 219
#define SERVER_MOD_SHOW 220
#define SERVER_MOD 221
#define SERVER_DEL 222
#define SERVER_LIST_SHOW 223
#define ACTION_SYNC 224
#define ACTION_SAVEUPDATE 225
#define UPDATE_MULTI_SET 31
#define SCREEN_CHANGE 152
#define SCREEN_GREEN 151
#define YTKZ_PROTO 226
#define YTKZ 227
#define UPDATE_LANGUAGE 24
#define UPDATE_HIDDEN_VALUE 300
//#define SETPHY	228
//#define GETPHY	229

#define CGI_NAME "encoder.cgi"
#define BUFFER_LEN					1024
#define MAXFILE_NUM					1024
#define FILENAME_MAXLEN				200

#if (defined(DSS_ENC_1200) || defined(DSS_ENC_1260))

#define UPDATEFILEHEAD "7E7E7E7E48454E43"
#else
#define UPDATEFILEHEAD "7E7E7E7E31313030"
#endif
#define BufferLen 500
#define UPFILEHEADLEN 8

#define WEBVERSION "1.0.2"


#define INPUT1_HIGH_STRING "input 1 / high"
#define INPUT1_LOW_STRING  "input 1 / low"
#define INPUT2_HIGH_STRING "input 2 / high"
#define INPUT2_LOW_STRING  "input 2 / low"


char sys_password[100];
char sys_webpassword[100];
char sys_timeout[100];
char sys_language[100];
char script_language[100];

typedef struct package
{
	unsigned int version;
	unsigned int len;
	unsigned int reserves;
	unsigned char hash[32];
}Package;


char* trim(char * str)
{
	int len=0;
	int i=0;
	len=strlen(str);
	i=len-1;
	for(;i>0;i--)
	{
		if(str[i]==' '||str[i]==0x0A)
			continue;
		else
			break;
	}
	str[i+1]='\0';
	return str;
}

static void gettexttodisplay(TextInfo* info)
{
	strcpy(info->msgtext,"");
	info->xpos=0;
	info->ypos=0;
	info->showtime=1;

}
//0???
//-1???
static int loginaction()
{
	char username[100] = {0};
    char password[100] = {0};
	//char strTime[100] = {0};
	cgiFormString("username", username, sizeof(username));
	cgiFormString("password", password, sizeof(password));

	if((!strcmp(username,"admin")&&!strcmp(password,sys_password))||(!strcmp(username,"guest")&&!strcmp(password,sys_webpassword))||(!strcmp(username,"ReachWebAdmin")&&!strcmp(password,"ReachWebAdmin2006")))
	{
		long now;
		now = (long)time(NULL);
		showPage("./index.html",sys_language);
		fprintf(cgiOut, "<script type='text/javascript' src='../ValidationEngine/js/languages/jquery.validationEngine-%s.js'></script>", script_language);
		fprintf(cgiOut,"<script type='text/javascript'>");
		fprintf(cgiOut, "setLanguageType('%s');", sys_language);
		fprintf(cgiOut,"</script>");
		cgiSetCookieUser(username);
		cgiSetCookienoTime("sessiontime",now);
		return 0;
	}
	else
	{
		char tmp[150]={0};
		getLanguageValue(sys_language,"UserOrPasswordError",tmp);
		forwardPage(CGI_NAME,PAGE_LOGIN_SHOW,tmp);
		return -1;
	}
}



//升级系统
static int updatesystems(void)
{
	//FILE *targetFile;
	cgiFilePtr file;
	char name[128];
	//int retCode = 0;
	int got = 0;
	int t;
	char *tmpStr=NULL;
	char fileNameOnServer[64];
	char buffer[2050];
	int rec = -1;
	int flag = 0;
	int platform = 0;
	char cmd[128] = {0};

	char languagebuf[150]= {0};
	getLanguageValue(sys_language,"uploadFileFail",languagebuf);
	if (cgiFormFileName("file", name, sizeof(name)) != cgiFormSuccess)
	{
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s [-1]');parent.closeUploading();</script>",languagebuf);
		return -1;
	}

	t=-1;
	//从路径名解析出用户文件名
	while(1)
	{
		tmpStr=strstr(name+t+1,"\\");
		if(NULL==tmpStr)
			tmpStr=strstr(name+t+1,"/");//if "\\" is not path separator, try "/"
		if(NULL!=tmpStr)
			t=(int)(tmpStr-name);
		else
			break;
	}

	#if 0
	printf("<script>");
	printf("alert('%s');",name);
	printf("</script>");
	#endif
	if(strstr(name,".bin")==NULL||(name-strstr(name,".bin"))!=(4-strlen(name)))
	{
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"binfile",languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}

	if ((rec=cgiGetFormFileName("file", fileNameOnServer)) != cgiFormSuccess)
	{
	//	memset(languagebuf,0,150);
	//	getLanguageValue(sys_language,"binfile",languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}

	//根据表单中的值打开文件
	if ((rec=cgiFormFileOpen("file", &file)) != cgiFormSuccess)
	{
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}

	while (cgiFormFileRead(file, buffer, BufferLen, &got) ==cgiFormSuccess)
	{
		if(got>0)
		{
			//效验文件合法
			if(flag == 0)
			{
				if(got > sizeof(Package))
				{
					Package *pack = (Package*)buffer;

					//是8168
					if((pack->version == 2012) && (pack->len == sizeof(Package)))
					{
						//fwrite(buffer + sizeof(Package),1,got- sizeof(Package),targetFile);
						flag = 1;
						platform = 0;
						break;
					}

					//是6467
					else if((buffer[0] == 0x7e)&&(buffer[1] == 0x7e)&&(buffer[2] == 0x7e) && (buffer[3] == 0x7e)
							&& (buffer[4] == 0x48) && (buffer[5] == 0x45) && (buffer[6] == 0x4e) && (buffer[7] == 0x43))
					{
						//fwrite(buffer + 8,1,got - 8,targetFile);

						//fwrite(buffer,1,got,targetFile);
						flag = 1;
						platform = 1;
						break;
					}
				}

				cgiFormFileClose(file);
//				fclose(targetFile);
//				unlink(fileNameOnServer);
				memset(languagebuf,0,150);
				getLanguageValue(sys_language,"uploadFileError",languagebuf);
				fprintf(cgiOut,"<script type='text/javascript'>alert('%s [-5]');parent.closeUploading();</script>",languagebuf);
				return -1;

			}
//			else
//				fwrite(buffer,1,got,targetFile);
		}
	}

	cgiFormFileClose(file);
//	fclose(targetFile);

	sprintf(cmd, "mv %s /tmp/update.bin", fileNameOnServer);
	system(cmd);
	system("chmod 777 /tmp/update.bin");

	if(platform == 0)
	{
		rec = WebUpdateFile("/tmp/update.bin");
	}
	else if(platform == 1)
	{
		rec = WebSysUpgrade_6467("/tmp/update.bin");
	}
	else
	{
		rec = -1;
	}

	memset(languagebuf,0,150);
	if(rec == 0)
	{
		getLanguageValue(sys_language,"uploadFileSuccess",languagebuf);
		//fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
	//	fopen("/tmp/success","w");
	}
	else if(rec == SERVER_HAVERECORD_FAILED)
	{
		getLanguageValue(sys_language,"uploadstateerror",languagebuf);
		//fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		//fopen("/tmp/fail","w");
	}

	else
	{
		getLanguageValue(sys_language,"uploadFileFail",languagebuf);
		//fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
	}
	//webRebootSystem();
	return 0;
}



#if 0
//???LOGO??
static int updateLogoPic(void)
{
	FILE *targetFile;
	cgiFilePtr file;
	char name[128];
	//int retCode = 0;
	int got = 0;
	int t;
	char *tmpStr=NULL;
	char fileNameOnServer[64];
	char buffer[1024];
	int rec = 0;
	int flag = 0;


	char languagebuf[150]={0};
	getLanguageValue(sys_language,"uploadFileFail1",languagebuf);
	if (cgiFormFileName("file", name, sizeof(name)) !=cgiFormSuccess)
	{
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}

	t=-1;
	//??·?????????????????
	while(1){
		tmpStr=strstr(name+t+1,"\\");
		if(NULL==tmpStr)
			tmpStr=strstr(name+t+1,"/");//if "\\" is not path separator, try "/"
		if(NULL!=tmpStr)
			t=(int)(tmpStr-name);
		else
			break;
	}

	if(strstr(name,".png")==NULL||(name-strstr(name,".png"))!=(4-strlen(name)))
	{
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"pngfile",languagebuf);
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}
	//?????е???????
	if ((rec=cgiFormFileOpen("file", &file)) != cgiFormSuccess) {
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}
#ifdef LINUX
	strcpy(fileNameOnServer,"/opt/reach/logo.png");
#else
	strcpy(fileNameOnServer,"D:\\update\\logoimg\\logo.png");
#endif
	targetFile=fopen(fileNameOnServer,"wb+");
	if(targetFile==NULL){
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}

	while (cgiFormFileRead(file, buffer, BufferLen, &got) ==cgiFormSuccess){
		if(got>0){
			if(flag == 0){
			//	char tmpsync[20]={0};
				/*sprintf(tmpsync,"%02X%02X%02X%02X%02X%02X%02X%02X",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);
				if(strcmp(UPDATEFILEHEAD,tmpsync))
				{
					memset(languagebuf,0,150);
					getLanguageValue(sys_language,"uploadFileFail",languagebuf);
					//forwardPage(CGI_NAME,207,languagebuf);
					fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
					return -1;
				}else{*/
					fwrite(buffer+8,1,got-8,targetFile);
				//}
				flag = 1;
			}else
				fwrite(buffer,1,got,targetFile);
		}
	}
	cgiFormFileClose(file);
	fclose(targetFile);
	memset(languagebuf,0,150);
	getLanguageValue(sys_language,"uploadFileSuccess",languagebuf);
	fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
	return 0;
}

#endif
#define JEPG_TEMP_FILENEMA  ("/opt/dvr_rdk/ti816x_2.8/logo_temp.jpg")

static int uploadLogo(int channel)
{
	FILE *targetFile;
	cgiFilePtr file;
	char name[256] ={0};
	//int retCode = 0;
	int got = 0;
	int t;
	char *tmpStr=NULL;
	char fileNameOnServer[64];
	char buffer[1024];
	int ret = 0;
//	int flag = 0;

	char tempname[512] = {0};

	sprintf(tempname,JEPG_TEMP_FILENEMA);
	char languagebuf[150]={0};
	getLanguageValue(sys_language,"uploadFileFail1",languagebuf);
	if (cgiFormFileName("file", name, sizeof(name)) !=cgiFormSuccess)
	{
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}

	t=-1;
	while(1){
		tmpStr=strstr(name+t+1,"\\");
		if(NULL==tmpStr)
			tmpStr=strstr(name+t+1,"/");//if "\\" is not path separator, try "/"
		if(NULL!=tmpStr)
			t=(int)(tmpStr-name);
		else
			break;
	}

	if((strstr(name,".jpg")==NULL && (strstr(name,".JPG")==NULL))
	||((name-strstr(name,".jpg"))!=(4-strlen(name))&&(name-strstr(name,".JPG"))!=(4-strlen(name))))
	{
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"pngfile",languagebuf);
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}
	if ((ret=cgiFormFileOpen("file", &file)) != cgiFormSuccess) {
		//memset(languagebuf,0,150);
		//sprintf(languagebuf,"%d",rec);
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}
#ifdef LINUX
	strcpy(fileNameOnServer,tempname);
#else
	strcpy(fileNameOnServer,"D:\\update\\update.bin");
#endif
	targetFile=fopen(fileNameOnServer,"wb+");

//check file size
	if(targetFile==NULL){
		forwardPage(CGI_NAME,207,languagebuf);
		return -1;
	}

	while (cgiFormFileRead(file, buffer, BufferLen, &got) ==cgiFormSuccess){
		if(got>0){
			#if 0
			if(flag == 0){
				char tmpsync[20]={0};
				sprintf(tmpsync,"%02X%02X%02X%02X%02X%02X%02X%02X",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);
				if(strcmp(UPDATEFILEHEAD,tmpsync))
				{
					memset(languagebuf,0,150);
					getLanguageValue(sys_language,"uploadFileFail1",languagebuf);
					forwardPage(CGI_NAME,207,languagebuf);
					return -1;
				}else{
					fwrite(buffer+8,1,got-8,targetFile);
				}
				flag = 1;
			}else
			#endif
				fwrite(buffer,1,got,targetFile);
		}
	}
	cgiFormFileClose(file);
	fclose(targetFile);
	ret = WebUploadLogoFile(tempname,channel);
	if(ret == -4) {
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"uploadLogoTooLarge",languagebuf);
	} else if(ret == -3) {
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"pngfile",languagebuf);
	}else if(ret == -5){
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"uploadFileSizeFailed",languagebuf);
	}else {
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"uploadFileSuccess1",languagebuf);
	}
	fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
	return 0;
}

static int mid_ip_is_multicast(char *ip) {
	struct in_addr	 addr ;
	unsigned int 	dwAddr;
	unsigned int 	value;

	inet_aton(ip, &addr);
	memcpy(&value, &addr, 4);
	dwAddr = htonl(value);

	//PRINTF("ip=%s.dwAddr=0x%08x\n", ip, dwAddr);

	if(((dwAddr & 0xFF000000) >> 24) > 223 && ((dwAddr & 0xFF000000) >> 24) < 240) {
		return 1;
	}

	return 0;
}

static int mid_ip_is_vailed(char *ip) {
	struct in_addr	 addr ;
	unsigned int 	dwAddr;
	unsigned int 	value;

	inet_aton(ip, &addr);
	memcpy(&value, &addr, 4);
	dwAddr = htonl(value);

	//PRINTF("ip=%s.dwAddr=0x%08x\n", ip, dwAddr);


	if((((dwAddr & 0xFF000000) >> 24) >= 240) || ((((dwAddr & 0xFF000000) >> 24 == 127)))) {
		return 0;
	}

	if(((dwAddr & 0xFF000000) >> 24) < 1) {
		return 0;
	}


	return 1;
}


static int changePassword()
{
	char password[21]={0};
	char passwordagain[21]={0};
	char oldpassword[21]={0};
	char tmpbuf[100]={0};
	int rec=0;
	char username[20]={0};
	cgiFormString("password1",password,21);
	cgiFormString("password2",passwordagain,21);
	cgiFormString("oldpassword",oldpassword,21);
	cgiFormString("username",username,20);

	if(strcmp(username, USERNAME) != 0 && strcmp(username, WEBUSERNAME) != 0) {
		memset(tmpbuf,0,100);
		getLanguageValue(sys_language,"usernameNotExist",tmpbuf);
		fprintf(cgiOut, tmpbuf);
		return -1;
	}

	if(strcmp(username, USERNAME) == 0) {
		if(strcmp(oldpassword,sys_password))//?????????
		{
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"oldpassworderror",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		if(strcmp(password,passwordagain)){//?????
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"passwordTwiceInputNotSame",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		if(password==NULL||strlen(password)==0||!strcmp(password,"")){//???????
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"passwordIsNull",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		rec=updateConfigFile("sysinfo.txt","password",password);
		memset(tmpbuf,0,100);
		getLanguageValue(sys_language,"modifyPasswordSuccess",tmpbuf);
		fprintf(cgiOut, tmpbuf);
		return 0;

	} else {
		if(strcmp(oldpassword,sys_webpassword))//?????????
		{
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"oldpassworderror",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		if(strcmp(password,passwordagain)){//?????
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"passwordTwiceInputNotSame",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		if(password==NULL||strlen(password)==0||!strcmp(password,"")){//???????
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"passwordIsNull",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		rec=updateConfigFile("sysinfo.txt","webpassword",password);
		memset(tmpbuf,0,100);
		getLanguageValue(sys_language,"modifyPasswordSuccess",tmpbuf);
		fprintf(cgiOut, tmpbuf);
		return 0;
	}

}

char* Getline(char* pBuf, char* pLine)
{
	char* ptmp = strchr(pBuf, '\n');
	if(ptmp == NULL)
		return pBuf;
	strncpy(pLine, pBuf, ptmp - pBuf);
	return ptmp + 1;
}
int webinput_to_channel(int webinput)
{
	int channel = CHANNEL_INPUT_1;
	if(webinput == WEB_INPUT_1)
	{
		channel = CHANNEL_INPUT_1;
	}
	else if(webinput == WEB_INPUT_2)
	{
		channel = CHANNEL_INPUT_2;
	}else {
		channel = CHANNEL_INPUT_MP;
	}

	return channel;
}
void formatTime(time_t time1, char *szTime)
{
	struct tm tm1;
	localtime_r(&time1, &tm1);
	sprintf(szTime, "%4d-%02d-%02d %02d:%02d:%02d",
	tm1.tm_year+1900, tm1.tm_mon+1, tm1.tm_mday,
	tm1.tm_hour, tm1.tm_min,tm1.tm_sec);
}


static int CheckIPNetmask(int ipaddr, int netmask, int gw)
{
	int mask, ip, gateip;
	mask = netmask;
	mask = htonl(mask);
	ip = ipaddr;
	ip = htonl(ip);
	gateip = gw;
	gateip = htonl(gateip);

	if((((ip & 0xFF000000) >> 24) > 223) || ((((ip & 0xFF000000) >> 24 == 127)))) {
		return 0;
	}

	if(((ip & 0xFF000000) >> 24) < 1) {
		return 0;
	}

	if((((gateip & 0xFF000000) >> 24) > 223) || (((gateip & 0xFF000000) >> 24 == 127))) {
		return 0;
	}

	if(((gateip & 0xFF000000) >> 24) < 1) {
		return 0;
	}

	if((ip & mask) == 0) {
		return 0;
	}

	if((ip & (~mask)) == 0) {
		return 0;
	}

	if((~(ip | mask)) == 0) {
		return 0;
	}

	while(mask != 0) {
		if(mask > 0) {
			return 0;
		}

		mask <<= 1;
	}

	return 1;
}

static int web_download_ftptool(void)
{
	int total=0;
	char buf[1024];
	int ret =0;
	FILE* fd = fopen("/var/www/html/download/ReachDownload.exe","rb+");
	if( NULL == fd){
		return -1;
	}
	while(( ret = fread(buf,1,1024,fd)) >= 0){
		if( ret > 0){
			fwrite(buf,1,ret,cgiOut);
		}
	}
	fclose(fd);
	return 0;
}

int cgiMain(void) {
	int actionCode  = 0;
	int ret = 0;
	int lang = 0;
	int need_reboot =0;
	int max_x = 1860;
	int max_y = 1000;

	cgiFormInteger("actioncode", &actionCode, 0);
	memset(sys_password,0,100);
	memset(sys_webpassword,0,100);
	memset(sys_timeout,0,100);
	memset(sys_language,0,100);


	ret = getLanguageValue("sysinfo.txt","password",sys_password);
	if(ret == -1 || strlen(sys_password) == 0) {
		strcpy(sys_password,USERNAME);
	}
	getLanguageValue("sysinfo.txt","webpassword",sys_webpassword);
	if(ret == -1 || strlen(sys_webpassword) == 0) {
		strcpy(sys_webpassword,WEBUSERNAME);
	}
	getLanguageValue("sysinfo.txt","timeout",sys_timeout);
	getLanguageValue("sysinfo.txt","language",sys_language);
	if(ret == -1 || strlen(sys_language) == 0) {
		strcpy(sys_language,"us");
	}

	if(!strcmp(sys_language,"cn"))
	{
		lang = 0;
		strcpy(script_language,"zh_CN");
	}else{
		lang = 1;
		strcpy(script_language,"en");
	}

	if(actionCode == UPDATE_LANGUAGE)
	{
		char local[10]={0};
		cgiFormString("local",local,10);
		if(strcmp(local,"us"))
			//fprintf(cgiOut,"Content-Type:text/html;charset=gbk\n\n");
			fprintf(cgiOut,"Content-Type:text/html;charset=gb2312\n\n");
		else
			//fprintf(cgiOut,"Content-Type:text/html;charset=utf-8\n\n");
			fprintf(cgiOut,"Content-Type:text/html;charset=gb2312\n\n");
	} else if(actionCode == ACTION_DOWNLOAD_SDP) {
		fprintf(cgiOut,"Content-Type:text/plain;charset=gb2312\n");
	} else if(actionCode == ACTION_FTPTOOL_DOWNLOAD) {
		// 不输出相应类型
	} else {
		if(strcmp(sys_language,"us"))
			//fprintf(cgiOut,"Content-Type:text/html;charset=gbk\n\n");
			fprintf(cgiOut,"Content-Type:text/html;charset=gb2312\n\n");
		else
			//fprintf(cgiOut,"Content-Type:text/html;charset=utf-8\n\n");
			fprintf(cgiOut,"Content-Type:text/html;charset=gb2312\n\n");
	}
	switch (actionCode)
	{

		case YTKZ_PROTO:
		{
			int index = 0;
		//	int ptzProtocal = 0;
		//	int outvalue = 0;
			int webinput =WEB_INPUT_1;
			int channel=0;

			cgiFormInteger("use",&index,0);
			WebSetCtrlProto(index,&index,channel);
			forwardPages(CGI_NAME,208);
			break;
		}
		case UPDATE_LANGUAGE:
			{
				char local[10]={0};
				cgiFormString("local",local,10);
				updateConfigFile("sysinfo.txt","language",local);
				fprintf(cgiOut, "window.location.href='encoder.cgi';");
				break;
			}

		case ACTION_LOGIN:
			{
			  loginaction();
			}
			break;

		case ACTION_CHANGEPWD:
		{
			changePassword();
		}
		break;
		case ACTION_UPLOADFILE:
			{
				updatesystems();
			}
		break;

		case ACTION_SYSUPGRADE_STATUS:
		{
			if(0 == access("/var/tmp/success",F_OK))
			{
				fprintf(cgiOut, "%d",RESULT_SUCCESS);
				unlink("/var/tmp/success");
				webRebootSystem();
			}
			else if(0 == access("/var/tmp/fail",0))
			{
				fprintf(cgiOut, "%d", RESULT_FAILURE);
				unlink("/var/tmp/fail");
			}
			else if(0 == access("/var/tmp/recording",0))
			{
				fprintf(cgiOut, "%d", RESULT_MUSTMULTIIP);
				unlink("/var/tmp/recording");
			}
			break;
		}

		case ACTION_SETDEFAULT_SET:
		{
			int ret = 0;
			//need reboot
			ret = WebSysRollBack("");
			if(ret == 0)
			{
				//webRebootSystem();
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
			}
			else if(ret == SERVER_HAVERECORD_FAILED)
			{
				fprintf(cgiOut, "%d", RESULT_MUSTMULTIIP);
			}
			else
			{
				fprintf(cgiOut, "%d", RESULT_FAILURE);
			}

			break;
		}

		case ACTION_LOGOUT:
		{
			fprintf(cgiOut, "document.cookie = '';\n");
			fprintf(cgiOut, "parent.document.location.replace('/index.html');");
		}
		break;
		case PAGE_LOGIN_SHOW:
		{
			showPage("./login.html",sys_language);
			fprintf(cgiOut, "<script type='text/javascript' src='../ValidationEngine/js/languages/jquery.validationEngine-%s.js'></script>", script_language);
			break;
		}
		case PAGE_TIMEOUT_SHOW:
			showPage("./timeout.html",sys_language);
			break;
		case PAGE_TOP_SHOW:
			showPage("./top.html",sys_language);
			break;
		case PAGE_DOWN_SHOW:
			showPage("./down.html",sys_language);
			break;
		case PAGE_MAIN_SHOW:
		{
			showPage("./index.html",sys_language);
			fprintf(cgiOut, "<script type='text/javascript' src='../ValidationEngine/js/languages/jquery.validationEngine-%s.js'></script>", script_language);
		}
		break;

		case PAGE_SYSTEM_INFO:

			break;
		case PAGE_PARAMETER_SHOW:
			break;


		case RESTART_SHOW:
			{
				showPage("./restart.html",sys_language);
			}
			break;
		case RESTART_SYS:
			{
				ret = webfReboot();
				fprintf(cgiOut, "%d", ret);
			}
			break;
		case ACTION_SAVEUPDATE:
			break;
		case ACTION_SYNC:
			{
				char time[30]={0};
				char tmpbuff[100]={0};
				int i = 0;
				int rec = 0;
				DATE_TIME_INFO ctime={0};
				char *d="/";
				char *p;
				cgiFormString("clienttime",time,30);
				p=strtok(time,d);
				while(p)
				{
					if(i==0)
						ctime.year = atoi(p);
					if(i==1)
						ctime.month = atoi(p);
					if(i==2)
						ctime.mday = atoi(p);
					if(i==3)
						ctime.hours = atoi(p);
					if(i==4)
						ctime.min = atoi(p);
					if(i==5)
						ctime.sec = atoi(p);
					p = strtok(NULL,d);
					i++;
				}
				rec = Websynctime(&ctime,&ctime);
				if(rec == 0){
					getLanguageValue(sys_language,"synchronizedTimeSuccess",tmpbuff);
				}else {
					getLanguageValue(sys_language,"synchronizedTimeFail",tmpbuff);
				}
				fprintf(cgiOut, tmpbuff);
			}
			break;
		case PAGE_LEFTMENU_SYSPARAMS_SHOW: {
			showPage("./leftmenuParamsSettings.html", sys_language);
			break;
		}
		case PAGE_LEFTMENU_SYSSET_SHOW: {
			showPage("./leftmenuSystemSettings.html", sys_language);
			break;
		}

		/*
		 * sysinfo page show action
		 */
		case PAGE_SYSINFO_SHOW: {
			int outlen = 0;
			DevInfo devinfo = {0};
			DiskInfo disinfo = {0};
			char deviceModelNO[64] = {0};
			char webVersion[64] = {0};
			char serialNO[64] = {0};
			char NetContorlIp[16] = {0};
			System_Info_t in;
			System_Info_t out;
			EncInfo_t enc_info;
			float disksize = 3000000000;
			float remainsize = 10000000;
			char exception[20] = {0};
			getLanguageValue(sys_language,"sysinfoException",exception);
			webGetEncInfo(&enc_info, 0);
			WebGetSystemInfo(&in,&out);
			WebGetNetControlIp(NetContorlIp);
			disksize = out.total_space;
			remainsize = out.available_space;
			disksize = disksize/1024/1024;
			remainsize = remainsize/1024/1024;
//			appCmdStringParse(MSG_GETSERIALNO, NULL, strlen(serialNO), serialNO, &outlen);
//			WebGetDevideType(deviceModelNO, sizeof(deviceModelNO));
			strcat(webVersion, WEBVERSION);

			showPage("./sysinfo.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			#if 0
				fprintf(cgiOut, "setFormItemValue('wmform', [{'name': 'serialNO','value': '%s','type': 'text' }", devinfo.serialnum);
				fprintf(cgiOut, ",{'name': 'deviceModelNO','value': '%s','type': 'text' }", out.type_no);
				fprintf(cgiOut, ",{'name': 'webVersion','value': '%s','type': 'text' }", webVersion);
				fprintf(cgiOut, ",{'name': 'aiosysversion','value': '%s','type': 'text' }", out.ctrl_version);
				fprintf(cgiOut, ",{'name': 'aiosysenctime','value': '%s','type': 'text' }", out.ctrl_built_time);
				fprintf(cgiOut, ",{'name': 'hdsysversion','value': '%s','type': 'text' }", out.hd_version);
				fprintf(cgiOut, ",{'name': 'hdsysenctime','value': '%s','type': 'text' }", out.hd_built_time);
				if(disksize < 1) {
					fprintf(cgiOut, ",{'name': 'hardDiskSpace','value': '%s','type': 'text' }", exception);
					fprintf(cgiOut, ",{'name': 'freeDiskSpace','value': '%s','type': 'text' }", exception);
				} else {
					fprintf(cgiOut, ",{'name': 'hardDiskSpace','value': '%0.2f GByte','type': 'text' }", disinfo.disksize);
					fprintf(cgiOut, ",{'name': 'freeDiskSpace','value': '%0.2f GByte','type': 'text' }", disinfo.remainsize);
				}
				fprintf(cgiOut, ",{'name': 'mediaIp','value': '%s','type': 'text' }]);\n",out.ip);
				fprintf(cgiOut, ",{'name': 'zhongkongIP','value': '%s','type': 'text' }]);\n",out.ip);
				#endif
				fprintf(cgiOut, "setFormItemValue('wmform', [{'name': 'serialNO','value': '%s','type': 'text' }", out.serial_no);
				fprintf(cgiOut, ",{'name': 'deviceModelNO','value': '%s','type': 'text' }", out.type_no);
				fprintf(cgiOut, ",{'name': 'webVersion','value': '%s','type': 'text' }", webVersion);
				fprintf(cgiOut, ",{'name': 'aiosysversion','value': '%s(%s/%s)','type': 'text' }", out.ctrl_version, enc_info.kernel_version, enc_info.fpga_version);
				fprintf(cgiOut, ",{'name': 'aiosysenctime','value': '%s','type': 'text' }", "2013/11/16 13:36:22");
				fprintf(cgiOut, ",{'name': 'hdsysversion','value': '%s(%s/%s)','type': 'text' }",out.hd_version,out.hd_ker_version,out.hd_FPGA_version);
				fprintf(cgiOut, ",{'name': 'hdsysenctime','value': '%s','type': 'text' }", out.hd_built_time);
				if(disksize < 1) {
					fprintf(cgiOut, ",{'name': 'hardDiskSpace','value': '%s','type': 'text' }", exception);
					fprintf(cgiOut, ",{'name': 'freeDiskSpace','value': '%s','type': 'text' }", exception);
				} else {
					fprintf(cgiOut, ",{'name': 'hardDiskSpace','value': '%0.2f GByte','type': 'text' }", disksize);
					fprintf(cgiOut, ",{'name': 'freeDiskSpace','value': '%0.2f GByte','type': 'text' }", remainsize);
				}
				fprintf(cgiOut, ",{'name': 'mediaIp','value': '%s','type': 'text' }",out.ip);
				fprintf(cgiOut, ",{'name': 'zhongkongIP','value': '%s','type': 'text' }]);\n",NetContorlIp);
			fprintf(cgiOut, "</script>\n");
			break;
		}
#if 0
		// 课件文件夹重命名
		case ACTION_FILEMGR_RENAME:
		{
			char renameCmd[160] = "mv /opt/Rec/";
			char courseName[80]={0};
			char newCourseName[80]={0};
			cgiFormString("courseName", courseName, 80);
			cgiFormString("newCourseName", newCourseName, 80);
			strcat(renameCmd,courseName);
			strcat(renameCmd," /opt/Rec/");
			strcat(renameCmd,newCourseName);
			int exeStatus = system(renameCmd);

			if(exeStatus != -1)
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			else
			fprintf(cgiOut, "%d", RESULT_FAILURE);
			//fprintf(cgiOut, " - %s", renameCmd);
			break;
		}

		case ACTION_FILEMGR_DEL:
		{
			char delCmd[100] = "rm -rf /opt/Rec/";
			char courseName[80]={0};
			cgiFormString("courseName", courseName, 80);
			strcat(delCmd,courseName);
			int exeStatus = system(delCmd);

			if(exeStatus != -1)
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			else
			fprintf(cgiOut, "%d", RESULT_FAILURE);
			break;
		}

#endif
		case PAGE_FILEMGR_SHOW: {
			char recDir[9] = "/opt/Rec";
			char temp[1024] = {0};
			char tempDownloadList[100] = {0};
			char formData[1024*1024] = {0};
			char mtime[26] = {};
			int pageIndex = 0;
			int rowIndex = 1;
			float totalSize = 0;
			struct stat buf;
			char currentDir[100] = {0};
			char currentFile[100] = {0};
			char currentMd5[100] = {0};
			char currentTimeInfo[100] = {0};
			char courseDir[100] ={0};
			char recovery[100] = {0};

			cgiFormInteger("pageIndex",&pageIndex,0);

			strcat(formData,"var formData = [");

			DIR *dirp, *dirList;
			struct dirent *direntp;
			struct dirent *direnList;
			struct stat info;

			dirp = opendir(recDir);
			if(dirp != NULL)
			{

			while(1)
			{
			direntp = readdir(dirp);

			if(direntp == NULL){
			break;
			}

			if(direntp->d_name[0] != '.' && (direntp->d_type & DT_DIR))
			{

			sprintf(currentMd5, "%s", recDir);
			strcat(currentMd5,"/");
			strcat(currentMd5, direntp->d_name);
			strcat(currentMd5, "/md5.info");

			memset(currentTimeInfo, 0, 100);
			sprintf(currentTimeInfo, "%s", recDir);
			strcat(currentTimeInfo,"/");
			strcat(currentTimeInfo, direntp->d_name);

			strcat(currentTimeInfo, "/Time.info");

			if( strcmp(direntp->d_name, "recovery")  == 0 ){
				continue;
			}

		//	if( access(currentMd5, F_OK)  != 0 ){
				sprintf(recovery, "%s", recDir);
				strcat(recovery,"/recovery/");
				strcat(recovery, direntp->d_name);
				if( access(recovery, F_OK) == 0){
					continue;
				}
		//	}


			sprintf(currentDir, "%s", recDir);
			strcat(currentDir,"/");
			strcat(currentDir,direntp->d_name);
			strcat(currentDir,"/HD/resource/videos/");

			dirList = opendir(currentDir);

			if(dirList != NULL)
			{
			while(1)
			{
				direnList = readdir(dirList);

				if(direnList == NULL){
					break;
				}
				if(direnList->d_name[0] != '.'){
					strcat(currentFile, currentDir);
					strcat(currentFile, direnList->d_name);
					stat(currentFile, &buf);
					totalSize = totalSize + (float)buf.st_size;
					if(access(currentTimeInfo, F_OK) != 0){
						formatTime(buf.st_mtime, mtime);
					}
					else{
						FILE *timefp = fopen(currentTimeInfo, "r");
						memset(mtime, 0, 26);
						fread(mtime, 19, 1, timefp);
						fclose(timefp);
					}
					memset(&buf, 0, sizeof(struct stat));
					memset(currentFile,0,100);
					strcat(tempDownloadList, direnList->d_name);
					strcat(tempDownloadList, ",");
				}
			}
			closedir(dirList);
			}
			strcat(formData, "{\"courseName\": \"");
			strcat(formData, direntp->d_name);
			strcat(formData, "\", \"createDate\": \"");
			strcat(formData, mtime);
			strcat(formData, "\", \"size\": ");
			sprintf(temp,"%0.2f", totalSize/(1024*1024));
			strcat(formData, temp);
			strcat(formData, ", \"download\": \"");
			strcat(formData, tempDownloadList);
			strcat(formData, "\"},");
			rowIndex++;
			memset(temp, 0, 1024);
			memset(tempDownloadList,0,100);
			memset(currentDir,0,100);
			totalSize = 0;
			}

			}
			closedir(dirp);
			}

			strcat(formData, "];\n");
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, formData);
			fprintf(cgiOut, "</script>\n");
			showPage("./filemgr.html", sys_language);

			break;
		}

		/*
		 * page input show action
		 */
		case PAGE_INPUT_SHOW: {
			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;
			char outSignalInfo[16] = {0};
			webGetSignalInfo(outSignalInfo, 16, channel);
			showPage("./input.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [{'name': 'inputInfo','value': '%s','type': 'text' }])", outSignalInfo);

			fprintf(cgiOut, "</script>\n");
			break;
		}

		/*
		 * set input action
		 */
		case ACTION_INPUT_SET: {
			int outValue = 0;
//			int outlen = 0;
			int colorSpace = 0,old_colorSpace=0;
			int inputType = 0;
			int oldinput = 0;
			int netreboot =0;

			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;

			//add by zm
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
		//	webSetChannel(webinput);
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}
			//	webSetChannel(webinput);

			cgiFormInteger("colorSpace", &colorSpace, 0);
			cgiFormInteger("inputType", &inputType, 0);
			WebGetinputSource(&oldinput,channel);
			webGetColorSpace(&old_colorSpace,channel);

			//set color space ,for 8168,not need

			if(oldinput != inputType)
			{
				WebSetinputSource(inputType, &outValue,channel);
				netreboot = 5;
			}
			if( old_colorSpace!=colorSpace){
				webSetColorSpace(colorSpace,&old_colorSpace,channel);
			}
			if( 5 == netreboot ){
				webRebootSystem();
			}

			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case ACTION_INPUTTYPE_GET:
			{
 				int oldinput = 0;
				int webinput = WEB_INPUT_1;
				int channel = CHANNEL_INPUT_1;
				cgiFormInteger("input", &webinput, WEB_INPUT_1);
				WebGetinputSource(&oldinput,channel);

				fprintf(cgiOut, "%d", oldinput);
				break;
			}

		case PAGE_MOVIEMODEL_SHOW: {
			Moive_Info_t info;
			int hdmi = 1;
			int mergeRes = 1;
			int autoModel = 1;


			// 调用接口获取值
			ret = WebGetHDMIRes(&hdmi);
			WebGetSwmsLayout(&info);
			mergeRes = info.res;
			autoModel = info.model;
		//	fprintf(cgiOut, "<script type='text/javascript'>alert('ret=%d');</script>", ret);

		//	fprintf(cgiOut, "<script type='text/javascript'>alert('ret=%x');</script>", ret);
			showPage("./movieModel.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "setFormItemValue('wmform', [");
			fprintf(cgiOut, "{'name': 'hdmi','value': '%d','type': 'radio' }", hdmi);
			fprintf(cgiOut, ",{'name': 'mergeRes','value': '%d','type': 'select' }", mergeRes);
			fprintf(cgiOut, ",{'name': 'autoModel','value': '%d','type': 'radio' }]);\n", autoModel);
			fprintf(cgiOut, "</script>\n");
			break;
		}

		case PAGE_TRACKINGSET_SHOW: {
			Tracer_Info_t in;
			Tracer_Info_t out;
			WebGetTracerInfo(&in,&out);
			showPage("./trackingSet.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "setFormItemValue('wmform', [");
			fprintf(cgiOut, "{'name': 'jiwei','value': '%d','type': 'radio' }", out.num);
			fprintf(cgiOut, ",{'name': 'status1','value': '%d','type': 'hidden' }", out.Status[0]);
			fprintf(cgiOut, ",{'name': 'status2','value': '%d','type': 'hidden' }", out.Status[1]);
			fprintf(cgiOut, ",{'name': 'status3','value': '%d','type': 'hidden' }", out.Status[2]);
			fprintf(cgiOut, ",{'name': 'status4','value': '%d','type': 'hidden' }", out.Status[3]);
			fprintf(cgiOut, ",{'name': 'camIP1','value': '%s','type': 'text' }", out.ip[0]);
			fprintf(cgiOut, ",{'name': 'camIP2','value': '%s','type': 'text' }", out.ip[1]);
			fprintf(cgiOut, ",{'name': 'camIP3','value': '%s','type': 'text' }", out.ip[2]);
			fprintf(cgiOut, ",{'name': 'camIP4','value': '%s','type': 'text' }]);\n", out.ip[3]);
			fprintf(cgiOut, "init();");
			fprintf(cgiOut, "</script>\n");

			break;
		}

		/*
		 * audio page set action
		 */
		case ACTION_AUDIO_SET: {
		//	int outvalue = 0;
		//	int outlen = 0;
			int audio_input = 0;
			int SampleRateIndex = 0;
			int audioLevelLeft = 0;
			int audioLevelRight = 0;
			int bitRate = 0;
			int mute = 0;

			int webinput = WEB_INPUT_1;
			int mp_status = IS_IND_STATUS;
			int channel = CHANNEL_INPUT_1;

			cgiFormInteger("audioInput", &audio_input, 0);
			cgiFormInteger("sampleRate", &SampleRateIndex, 0);
			cgiFormInteger("audioLevelLeft", &audioLevelLeft, 0);
			cgiFormInteger("audioLevelRight", &audioLevelRight, 0);
			cgiFormInteger("bitRate", &bitRate, 0);
			cgiFormInteger("mute", &mute, 0);

			WEB_AudioEncodeParam audioParamIn;
			WEB_AudioEncodeParam audioParamOut;
			memset(&audioParamIn, 0, sizeof(WEB_AudioEncodeParam));
			memset(&audioParamOut, 0, sizeof(WEB_AudioEncodeParam));

			WebGetAudioEncodeParam(&audioParamIn, channel);

			audioParamIn.mp_input = audio_input;

			audioParamIn.SampleRateIndex = SampleRateIndex;
			audioParamIn.LVolume 	= audioLevelLeft &0xff;
			audioParamIn.RVolume 	= audioLevelRight &0xff;
			audioParamIn.BitRate 	= bitRate;

			//add by zm becase the volume is from 0-30
			audioParamIn.LVolume 	= audioParamIn.LVolume*3;
			audioParamIn.RVolume 	= audioParamIn.RVolume*3;
			audioParamIn.Mute 		= mute;
			ret = WebSetAudioEncodeParam(&audioParamIn, &audioParamOut,channel);
			if (SERVER_RET_USER_INVALIED == ret ){
				fprintf(cgiOut, "%d", RESULT_NO_ANSWER);
			}else
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case ACTION_REMOTECTRL_SET1: {
			// 接受协议文件的上传
			// uploadProtocol();
			break;
		}

		case ACTION_REMOTECTRL_SET2: {
			int teaCamPro = 0;
			int stuCamPro = 0;
			int teaCamAddrBit = 0;
			int stuCamAddrBit = 0;
			Remote_Ctrl_t info;
			Remote_Ctrl_t out;
			cgiFormInteger("teaCamPro", &teaCamPro, 1);
			cgiFormInteger("stuCamPro", &stuCamPro, 1);
			cgiFormInteger("teaCamAddrBit", &teaCamAddrBit, 1);
			cgiFormInteger("stuCamAddrBit", &stuCamAddrBit, 1);
			info.tea_addr = teaCamAddrBit;
			info.tea_procotol= teaCamPro;
			info.stu_procotol = stuCamPro;
			info.stu_addr= stuCamAddrBit;
			WebSetRemoteCtrl(&info,&out);
			break;
		}

		case ACTION_FILEMGR_DELETE: {
			char courseName[80]={0};
			cgiFormString("courseName", courseName, 80);
			WebDelRecourse(courseName, 80);

			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case ACTION_FILEMGR_USBCOPY: {
//			char copyCmd[256] = {0};
			char courseName[80]={0};
			cgiFormString("courseName", courseName, 80);
//			sprintf(copyCmd,256,"%s",courseName);
			ret = WebDownload(courseName,strlen(courseName));
			if(ret != -1)
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
			else
				fprintf(cgiOut, "%d", RESULT_FAILURE);

			break;
		}

		/*
		 * page captionlogo show action
		 */
		case PAGE_CAPTIONLOGO_SHOW: {
			//int outvalue = 0;
			//int outlen = 0;
			int cap_position = 0;

			int cap_x = 0;
			int cap_y = 0;


			char cap_text[128] = {0};
			int cap_alpha = 0;
			int cap_displaytime = 0;

			int logo_position = 0;
			int logo_x = 0;
			int logo_y = 0;
			int logo_alpha = 0;
			TextInfo textInfo;
			LogoInfo logoInfo;
			TextInfo textIn;
			LogoInfo logoIn;

			int channel = CHANNEL_INPUT_1;
			int webinput = WEB_INPUT_1;
//			WebEnableTextInfo osd_enable;
			int isLogoOn = 0;
			int isTextOn = 0;

			memset(&textInfo, 0, sizeof(TextInfo));
			memset(&logoInfo, 0, sizeof(LogoInfo));
			memset(&textIn, 0, sizeof(TextInfo));
			memset(&logoIn, 0, sizeof(LogoInfo));
//			memset(&osd_enable,0,sizeof(WebEnableTextInfo));

			WebGetTextOsd(&textIn, &textInfo,channel);
			WebGetLogoOsd(&logoIn, &logoInfo,channel);
//			WebGetEnabelTextParam(&osd_enable,channel);

//			WebGetMaxPos(channel, &max_x, &max_y);

			isLogoOn = logoInfo.enable;
			isTextOn = textInfo.enable;

			cap_position = textInfo.postype;
			cap_x = textInfo.xpos;
			cap_y = textInfo.ypos;
			cap_alpha =(1- (textInfo.alpha)/128.0)*100;//to 0-100
			cap_displaytime = textInfo.showtime;
			strcpy(cap_text, textInfo.msgtext);

			logo_position = logoInfo.postype;
			logo_x = logoInfo.x;
			logo_y = logoInfo.y;
			logo_alpha =(1-(logoInfo.alpha)/128.0)*100;//to 0-100
			if( cap_alpha < 0){
				cap_alpha = 0;
				}
			if( logo_alpha < 0){
				logo_alpha = 0;
				}
			showPage("./captionlogo.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [");
				fprintf(cgiOut, "{'name': 'cap_x','value': '%d','type': 'text' }", cap_x);
				fprintf(cgiOut, ",{'name': 'cap_y','value': '%d','type': 'text' }", cap_y);
				fprintf(cgiOut, ",{'name': 'cap_position','value': '%d','type': 'select' }", cap_position);
				fprintf(cgiOut, ",{'name': 'cap_brightness','value': '%d','type': 'text' }", cap_alpha);
				fprintf(cgiOut, ",{'name': 'cap_text','value': '%s','type': 'text' }", cap_text);
				fprintf(cgiOut, ",{'name': 'cap_displaytime','value': '%d','type': 'checkbox' }", cap_displaytime);
				fprintf(cgiOut, ",{'name': 'logo_x','value': '%d','type': 'text' }", logo_x);
				fprintf(cgiOut, ",{'name': 'logo_y','value': '%d','type': 'text' }", logo_y);
				fprintf(cgiOut, ",{'name': 'logo_position','value': '%d','type': 'select' }", logo_position);
				fprintf(cgiOut, ",{'name': 'logo_brightness','value': '%d','type': 'text' }]);\n", logo_alpha);
//				fprintf(cgiOut, "initInputTab(%d,%d);\n", webinput, lang);
				fprintf(cgiOut, "initMaxPos(%d,%d);\n", max_x, max_y);
//				fprintf(cgiOut, "initMergeTab(%d);\n", mp_status);
				//fprintf(cgiOut, "formBeautify();\n");
				fprintf(cgiOut, "fixBrightnessSlider();\n");
				fprintf(cgiOut, "captionCtrlInit(%d);\n", isTextOn);
				fprintf(cgiOut, "logoCtrlInit(%d);\n", isLogoOn);
			fprintf(cgiOut, "</script>\n");
			break;
		}


		/*
		 * page captionlogo set action.
		 */
		case ACTION_CAPTIONLOGO_SET: {
			char tmp[150]={0};
			//int outlen = 0;
			int cap_position = 0;
			int cap_x = 0;
			int cap_y = 0;
			char cap_text[128] = {0};
			int cap_alpha = 0;
			int cap_displaytime = 0;

			int logo_position = 0;
			int logo_x = 0;
			int logo_y = 0;
			int logo_alpha = 0;
			// 是否上传logo图片
	//		int isFileUpload = 0;
			TextInfo textInfoIn;
			TextInfo textInfoOut;
			LogoInfo logoInfoIn;
			LogoInfo logoInfoOut;
			int isLogoOn =0;
			int isTextOn= 0;

			int channel = CHANNEL_INPUT_1;


			memset(&textInfoIn, 0, sizeof(TextInfo));
			memset(&textInfoOut, 0, sizeof(TextInfo));
			memset(&logoInfoIn, 0, sizeof(LogoInfo));
			memset(&logoInfoOut, 0, sizeof(LogoInfo));

			cgiFormInteger("cap_position", &cap_position, 0);
			cgiFormInteger("cap_x", &cap_x, 0);
			cgiFormInteger("cap_y", &cap_y, 0);
			cgiFormString("cap_text", cap_text, 1000);
			cgiFormInteger("cap_displaytime", &cap_displaytime, 0);
			cgiFormInteger("cap_brightness", &cap_alpha, 0);
			cgiFormInteger("logo_position", &logo_position, 0);
			cgiFormInteger("logo_x", &logo_x, 0);
			cgiFormInteger("logo_y", &logo_y, 0);
			cgiFormInteger("logo_brightness", &logo_alpha, 0);

			cgiFormInteger("logo", &isLogoOn, 0);
			cgiFormInteger("caption",&isTextOn,0);

	//		cgiFormInteger("isFileUpload",&isFileUpload,0);

			if(cap_position != ABSOLUTE2) {
				WebGetTextPos(cap_position, &cap_x, &cap_y);
			}

			if(logo_position != ABSOLUTE2) {
				WebGetLogoPos(logo_position, &logo_x, &logo_y);
			}
			if( cap_alpha < 0){
				cap_alpha = 0;
			}
			if( logo_alpha < 0){
				logo_alpha = 0;
			}

			logo_alpha =128*(1- logo_alpha/100.0);
			cap_alpha =128*(1-  cap_alpha/100.0);


			textInfoIn.postype = cap_position;
			textInfoIn.xpos = cap_x;
			textInfoIn.ypos = cap_y;
			strcpy(textInfoIn.msgtext, cap_text);
			textInfoIn.alpha = cap_alpha;
			textInfoIn.showtime = cap_displaytime;
			textInfoIn.enable = isTextOn;

			logoInfoIn.postype = logo_position;
			logoInfoIn.x = logo_x;
			logoInfoIn.y = logo_y;
			logoInfoIn.alpha = logo_alpha;
			logoInfoIn.enable = isLogoOn;
//			logoInfoIn.channel = isFileUpload;

			WebSetTextOsd(&textInfoIn,  &textInfoOut,channel);
			WebSetLogoOsd(&logoInfoIn,  &logoInfoOut,channel);

			getLanguageValue(sys_language,"opt.success",tmp);
			fprintf(cgiOut, "<script type='text/javascript'>alert('%s');</script>", tmp);
			break;
		}

		case ACTION_UPLOADLOGOPIC_SET: {
			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;
			uploadLogo(channel);
			break;
		}

		/*
		 * page remotectrl show action.
		 */
		case PAGE_REMOTECTRL_SHOW: {
			int teaCamPro = 1;
			int stuCamPro = 2;
			int teaCamAddrBit = 3;
			int stuCamAddrBit = 4;
			Remote_Ctrl_t in,out;
//			WebGetRemoteCtrl(&in, &out);

			// 调用接口获取值

/*			showPage("./remotectrl.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [{'name': 'teaCamPro','value': '%d','type': 'select' }", out.tea_procotol);
				fprintf(cgiOut, ",{'name': 'stuCamPro','value': '%d','type': 'select' }",out.stu_procotol);
				fprintf(cgiOut, ",{'name': 'teaCamAddrBit','value': '%d','type': 'select' }",out.tea_addr);
				fprintf(cgiOut, ",{'name': 'stuCamAddrBit','value': '%d','type': 'select'}]);\n", out.stu_addr);
			fprintf(cgiOut, "</script>\n");
*/
			break;
		}

		/*
		 * PTZ control.
		 */
		case ACTION_PTZ_CTRL: {
			int speed = 0;
			int type = 0;
			int addr = 1;
			int webinput =WEB_INPUT_1;
			int channel ;
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
	//		webSetChannel(webinput);

			channel =  webinput_to_channel(webinput);

			cgiFormInteger("speed", &speed, 1);
			cgiFormInteger("type", &type, 14);
			cgiFormInteger("addressbit",&addr,1);

			webFarCtrlCamera(addr,type, speed,channel);
			break;
		}

		/*
		 * page network show action
		 */
		case PAGE_NETWORK_SHOW: {
			int outlen = 0;
			int dhcp = 0;
			IpConfig sysParamsOut;
			struct in_addr inAddr;
			WebGetNetwork(&sysParamsOut,&outlen);
			dhcp = sysParamsOut.iptype;
		
			showPage("./network.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [");
				fprintf(cgiOut, "{'name': 'dhcp','value': '%d','type': 'checkbox' }", dhcp);
				fprintf(cgiOut, ",{'name': 'IPAddress','value': '%s','type': 'text' }", sysParamsOut.ipaddr);
				fprintf(cgiOut, ",{'name': 'subMask','value': '%s','type': 'text' }", sysParamsOut.netmask);
				fprintf(cgiOut, ",{'name': 'gateWay','value': '%s','type': 'text' }]);", sysParamsOut.gateway);
				fprintf(cgiOut, "init();");
			fprintf(cgiOut, "</script>\n");
			break;
		}

		/*
		 * page network set action
		 */
		case ACTION_NETWORK_SET: {
			int outlen = 0;
			int dhcp = 0;
			int r1=0;


			IpConfig sysParamsIn;
			IpConfig sysParamsout;
			memset(&sysParamsIn, 0, sizeof(IpConfig));

			cgiFormInteger("dhcp", &dhcp, 0);
			cgiFormString("IPAddress", sysParamsIn.ipaddr, 20);
			cgiFormString("subMask", sysParamsIn.netmask, 20);
			cgiFormString("gateWay", sysParamsIn.gateway, 20);

			sysParamsIn.iptype = dhcp;

			ret=0;
			r1 = CheckIPNetmask(inet_addr(sysParamsIn.ipaddr),inet_addr(sysParamsIn.netmask),inet_addr(sysParamsIn.gateway));
			if( 0 == r1){
				ret=SERVER_RET_INVAID_PARM_VALUE;
			}
			if(1 ==r1 ){
				ret = appCmdStructParse(MSG_SET_NETWORK, &sysParamsIn, sizeof(IpConfig), &sysParamsout, &outlen);
			}

			if( ret == SERVER_RET_INVAID_PARM_VALUE ){
				fprintf(cgiOut, "%d", r1);
			}else if (SERVER_RET_OK == ret ){
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
				webRebootSystem();
			}else{
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
			}
			//fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case PAGE_MODIFYPASSWORD_SHOW: {
			showPage("./modifypassword.html", sys_language);
			break;
		}

		case PAGE_NetControlIp_SHOW: {
			char ip[16] = {0};

			// 获取网络中控ip
			WebGetNetControlIp(ip);
			showPage("./deviceset.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [");
				fprintf(cgiOut, ",{'name': 'zhongkongIP','value': '%s','type': 'text' }]);", ip);
			fprintf(cgiOut, "</script>\n");
			break;
		}

		case PAGE_SYSUPGRADE_SHOW: {
			char temp[10] = {0};
			char encoderTime[256] = {0};

			DATE_TIME_INFO date_time_info;
			memset(&date_time_info, 0, sizeof(DATE_TIME_INFO));
			WebgetEncodetime(&date_time_info);

			sprintf(temp,"%d", date_time_info.year);
			strcpy(encoderTime, temp);
			strcat(encoderTime, "/");
			memset(temp,0,strlen(temp));
			sprintf(temp,"%d", date_time_info.month);
			strcat(encoderTime, temp);
			strcat(encoderTime, "/");
			memset(temp,0,strlen(temp));
			sprintf(temp,"%d", date_time_info.mday);
			strcat(encoderTime, temp);
			strcat(encoderTime, "/");
			memset(temp,0,strlen(temp));
			sprintf(temp,"%d", date_time_info.hours);
			strcat(encoderTime, temp);
			strcat(encoderTime, "/");
			memset(temp,0,strlen(temp));
			sprintf(temp,"%d", date_time_info.min);
			strcat(encoderTime, temp);
			strcat(encoderTime, "/");
			memset(temp,0,strlen(temp));
			sprintf(temp,"%d", date_time_info.sec);
			strcat(encoderTime, temp);

			showPage("./sysupgrade.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "initPageTime('%s', '%s');", encoderTime, sys_language);
			fprintf(cgiOut, "</script>\n");
			break;
		}

		case PAGE_FTP_SHOW: {
			ftp_info pinfo = {0};
			WebGetThrFtpinfo(&pinfo);
			showPage("./ftp.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "setFormItemValue('wmform', [");
			fprintf(cgiOut, "{'name': 'ftpstatus','value': '%d','type': 'checkbox' }", pinfo.Mode);
			fprintf(cgiOut, ",{'name': 'thrftpport','value': '%d','type': 'text' }", pinfo.THRFTPPort);
			fprintf(cgiOut, ",{'name': 'ftpaddress','value': '%s','type': 'text' }", pinfo.THRFTPAddr);
			fprintf(cgiOut, ",{'name': 'ftpusername','value': '%s','type': 'text' }", pinfo.THRFTPUserName);
			fprintf(cgiOut, ",{'name': 'ftppassword','value': '%s','type': 'text' }", pinfo.THRFTPPassword);
			fprintf(cgiOut, ",{'name': 'ftppath','value': '%s','type': 'text' }]);\n",pinfo.THRFTPPath);
			fprintf(cgiOut, "</script>\n");
			break;
		}

		case PAGE_INPUTDETAILS_SHOW: {
			showPage("./input_details.html", sys_language);
			break;
		}

		case ACTION_INPUTDETAILS_GET: {
			char inputDetailInfo[2048];

			webSignalDetailInfo(inputDetailInfo,2048,CHANNEL_INPUT_1);
			fprintf(cgiOut, "setFormItemValue('wmform_videoAdvancedSet', [");
			fprintf(cgiOut, "{'name': 'inputDetailInfo','value': '%s','type': 'textarea' }]);\n", inputDetailInfo);
			break;
		}

		case ACTION_ADJUSTSCREEN_SET: {
			int speed = 0;
			int value = 0;
			short hporch = 0;
			short vporch =0;

			int sdiajust = 0;//tanqh

			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;

			cgiFormInteger("sdiajust", &sdiajust ,0);
			cgiFormInteger("speed", &speed, 1);
			cgiFormInteger("cmdType" ,&value, 1);
			//webSetChannel(sdiajust);
			if(speed==1)
			{
				hporch = 1;
				vporch = 1;
			}
			if(speed == 5)
			{
				hporch=6;
				vporch = 2;
			}
			if(speed == 10)
			{
				hporch = 10;
				vporch = 4;
			}

			if(value == 0)//Reset
				webRevisePicture(0,0,channel);
			if(value == 5)//right
				webRevisePicture(0-hporch,0,channel);
			if(value == 6)//left
				webRevisePicture(hporch,0,channel);
			if(value == 7)//down
				webRevisePicture(0,0-vporch,channel);
			if(value == 8)//up
				webRevisePicture(0,vporch,channel);


			break;
		}


		case ACTION_VIDEOADVANCEDSET_SET: {
			int param1 = 0;
			int param2 = 0;
			int param3 = 0;
			int param4 = 0;
			int param5 = 0;
			H264EncodeParam h264EncodeParamIn;
			H264EncodeParam h264EncodeParamOut;
			memset(&h264EncodeParamIn, 0, sizeof(H264EncodeParam));
			memset(&h264EncodeParamOut, 0, sizeof(H264EncodeParam));
			cgiFormInteger("param1", &param1, 0);
			cgiFormInteger("param2", &param2, 0);
			cgiFormInteger("param3", &param3, 0);
			cgiFormInteger("param4", &param4, 0);
			cgiFormInteger("param5", &param5, 0);
			h264EncodeParamIn.param1 = param1;
			h264EncodeParamIn.param2 = param2;
			h264EncodeParamIn.param3 = param3;
			h264EncodeParamIn.param4 = param4;
			h264EncodeParamIn.param5 = param5;
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}
#if 0
		case PAGE_RTSP_PLAYURL_SHOW:{
			char url[2048] = {0};
			int isRtspUsed = 0;
			int len = 0;
			char ip_temp[64] = {0};
			stream_output_server_config output_out;
			rtsp_server_config rtsp_config;
			memset(&output_out,0,sizeof(stream_output_server_config));
			memset(&rtsp_config,0,sizeof(rtsp_server_config));

			isRtspUsed = app_rtsp_get_used();

			if(isRtspUsed == 1) {
				//get rtsp info
				app_rtsp_get_cinfo(&output_out);
				app_rtsp_get_ginfo(&rtsp_config);
				sprintf(ip_temp,"rtsp://%s:%d",output_out.main_ip,rtsp_config.s_port);
				len += snprintf(url,sizeof(url) - len,"CH0/HIGH:\n<br/>  %s/stream0/high\n<br/>",ip_temp);
				len += snprintf(url+len,sizeof(url) - len,"CH0/LOW:\n<br/>  %s/stream0/low\n<br/>",ip_temp);
				len += snprintf(url+len,sizeof(url) - len,"CH1/HIGH:\n<br/>  %s/stream1/high\n<br/>",ip_temp);
				len += snprintf(url+len,sizeof(url) - len,"CH1/LOW:\n<br/>  %s/stream1/low\n<br/>",ip_temp);

			}
			else
			{
				sprintf(url,"%s\n","rtsp is invaild\n");
			}

			fprintf(cgiOut,"%s", url);
			break;
		}
#endif
		case ACTION_MERGE_SET:{
		//	char tmp[150]={0};
			int layout = 1;
//			Mp_Info info;
			int mp_status = 1;
			int mergeModel = 0;
			cgiFormInteger("mergestatus", &mp_status, 0);
			cgiFormInteger("layout", &layout, 0);
			cgiFormInteger("mergeModel", &mergeModel, 0);
			if( mergeModel  == 0){
	//			info.win1 = SIGNAL_INPUT_1;
	//			info.win2 = SIGNAL_INPUT_2;
			}else{
	//			info.win1 = SIGNAL_INPUT_2;
//				info.win2 = SIGNAL_INPUT_1;
			}
//			info.mp_status = mp_status;
//			info.layout  = layout;
//			ret = WebSetMpInfo(&info);
		//	fprintf(cgiOut, "<script type='text/javascript'>alert('ret=%x');</script>", ret);
			if(SERVER_RET_USER_INVALIED == ret ){
				fprintf(cgiOut, "%d", RESULT_NO_ANSWER);
			}else{
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
				}
			break;
		}

		case ACTION_ENCADVANCED_SET:{
			int zoomModel = 0;
			int enclevel = 0;
			cgiFormInteger("zoomModel", &zoomModel, 0);
			cgiFormInteger("enclevel", &enclevel, 0);
			if(zoomModel==0)
				zoomModel=1;
			else
				zoomModel=0;

			//
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}
#if 0
		case ACTION_DOWNLOAD_SDP: {
			char  buff[4096];
			int total_len = 0;
			int w_len = 0;
			char inputstring[32]={0};

		 	char ip[16] = {0};
			int videoport = 0;
			int audioport = 0;
		//	int type = 0;

			int channel = CHANNEL_INPUT_1;
			cgiFormString("ip", ip, 16);
			cgiFormInteger("vPort", &videoport, 0);
			cgiFormInteger("aPort", &audioport, 0);
			cgiFormString("iStream",inputstring,sizeof(inputstring));

			if (strcmp(INPUT1_HIGH_STRING,inputstring) == 0)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if (strcmp(INPUT1_LOW_STRING,inputstring) == 0)
			{
				channel = CHANNEL_INPUT_1_LOW;
			}
			else if (strcmp(INPUT2_HIGH_STRING,inputstring) == 0)
			{
				channel = CHANNEL_INPUT_2;
			}
			else if (strcmp(INPUT2_LOW_STRING,inputstring) == 0)
			{
				channel = CHANNEL_INPUT_2_LOW;
			}


			fprintf(cgiOut, "Content-Disposition:attachment;filename=encode.sdp\n\n");

		//	snprintf(buff,sizeof(buff),"ip=%s,port=%d,type=%d\n",ip,videoport,type);

			//need channel num
			webGetSdpInfo(buff , sizeof(buff),channel,ip,videoport,audioport);

			total_len = strlen(buff);

			while(total_len>0)
			{
				w_len = fwrite(buff, 1, total_len -w_len, cgiOut);
				total_len -= w_len;
				if(w_len <= 0)
					break;
			}

			break;
		}
#endif
		case ACTION_SERIALNO_SET:
		{
			char serialNo[256]={0};
			cgiFormString("serialNumber", serialNo, 256);
			WebSetSerialNo(serialNo,256);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case ACTION_CBCR_SET:
		{
			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;
			int out = -1;
			cgiFormInteger("input", &webinput, WEB_INPUT_1);

			//???????
			//webSetCbCr(&out, channel);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case ACTION_MOVIEMODEL_SET1: {
			int hdmi = 0;
			int retv = 1;
			int rets = 1;
			cgiFormInteger("hdmi", &hdmi, 1);
			ret = WebSetHDMIRes(hdmi);
			// do something
			if(!ret) {
				rets = webRebootSystem();

			}

			if( SERVER_HAVERECORD_FAILED == rets) {
				retv = -1;
			}
			fprintf(cgiOut, "%d", retv);
			break;
		}

		case ACTION_MOVIEMODEL_SET2: {
			Moive_Info_t info;
			int mergeRes = 0;
			int autoModel = 0;
			cgiFormInteger("mergeRes", &mergeRes, 1);
			cgiFormInteger("autoModel", &autoModel, 1);
			info.model  = autoModel;
			info.res = mergeRes;
		//	fprintf(cgiOut, "<script type='text/javascript'>alert('autoModel=%d');</script>", autoModel);
			ret = WebSetSwmsLayout(info);


			if(SERVER_RET_USER_INVALIED == ret ){
				fprintf(cgiOut, "%d", RESULT_FAILURE);
			}else{
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
			}
			break;
		}

		case ACTION_TRACKINGSET_SET: {
			Tracer_Info_t in,out;
			int rets = 1;
			int retv = 1;
			cgiFormInteger("jiwei", &(in.num), 1);
			cgiFormString("camIP1", in.ip[0], 16);
			cgiFormString("camIP2", in.ip[1], 16);
			cgiFormString("camIP3", in.ip[2], 16);
			cgiFormString("camIP4", in.ip[3], 16);

			ret = WebSetTracerInfo(&in, &out);
			if(0 == ret){
				rets = webRebootSystem();
			}
			if( SERVER_HAVERECORD_FAILED == rets) {
				retv = -1;
			}

			fprintf(cgiOut, "%d", retv);
			break;
		}

		case ACTION_NETCONTROL_IP_SET: {
			char zhongkongIP[40] = {0};
			char tmp[150]= {0};
			cgiFormString("zhongkongIP", zhongkongIP, 40);
			//设置网络中控ip
			WebSetNetControlIp(zhongkongIP,40);
			getLanguageValue(sys_language,"opt.success",tmp);
			fprintf(cgiOut, "%s", tmp);
			break;
		}

		case ACTION_FTP_SET: {
			ftp_info pinfo;
			memset(&pinfo,0,sizeof(ftp_info));
			int tmpvalue = 0;
			int rets = 1;
			int retv = 1;
			cgiFormInteger("ftpstatus", &tmpvalue, 0);
			pinfo.Mode = tmpvalue;
			cgiFormInteger("thrftpport", &tmpvalue, 0);
			pinfo.THRFTPPort = tmpvalue;
			cgiFormString("ftpaddress", (char *)(pinfo.THRFTPAddr), 26);
			cgiFormString("ftppath", (char *)(pinfo.THRFTPPath), 512);
			cgiFormString("ftpusername", (char *)(pinfo.THRFTPUserName), 256);
			cgiFormString("ftppassword", (char *)(pinfo.THRFTPPassword), 256);
			ret = WebSetThrFtpinfo(&pinfo);

			// do something设置FTP参数
			if(ret == 0 ){
				rets = webRebootSystem();
			}
			if( SERVER_HAVERECORD_FAILED == rets) {
				retv = -1;
			}

			fprintf(cgiOut, "%d", retv);
			break;
		}

		case ACTION_USB_SHOW: {
			int num =0;
			WebGetUSBDiskNum( &num);
			// 获取USB分区信息，返回值为一个int值，表示分区个数
			// 李昌禄提供。。。。。
			fprintf(cgiOut, "%d", num);
			break;
		}

		case ACTION_USBSTATUS_GET: {
			// 获取USB状态是否正常，拷贝U盘在不在，包括空间是否准许拷贝等
			int part = 0;
			cgiFormInteger("part", &part, 1);

			//need 用户选择分区信息
			ret = WebGetUSBStatus(part);//0是正常 -1->U盘未识别  -2->剩余空间不足
			fprintf(cgiOut, "%d", ret);
			break;
		}

		case ACTION_USBCOPY: {
			// 进行一键拷贝
			int part = 0;
			cgiFormInteger("part", &part, 1);
			ret = WebUSBCopy(part);
			fprintf(cgiOut, "%d", ret);
			break;
		}

		case ACTION_FTPTOOL_DOWNLOAD: {
			// FTP工具下载
			char  buff[4096];
			int total_len = 0;
			int w_len = 0;
			FILE *file = NULL;
			file = fopen("/var/www/html/download/ReachDownload.exe","r");
			if(file !=NULL){
				fseek(file,0L,SEEK_END);
				total_len = ftell(file);
				fclose(file);
			}

			fprintf(cgiOut, "Content-Disposition:attachment;filename=ReachDownload.exe");
			fprintf(cgiOut, "\n\n");
			fprintf(cgiOut, "Content-Length:%d", total_len);
			fprintf(cgiOut, "\r\n");
			fprintf(cgiOut, "Content-Type:application/octet-stream");
			fprintf(cgiOut, "\r\n");

			web_download_ftptool();
			break;
		}

		case PAGE_FILEMGR_FILELIST: {
			// 获取文件列表，读取xml文件
			showPage_xml("./file.xml", sys_language);
			break;
		}

		case ACTION_GETCOPYSTATUS:
		{
			if(0 == access("/var/tmp/success",F_OK))
			{
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
				unlink("/var/tmp/success");
			}
			else if(0 == access("/var/tmp/fail",F_OK))
			{
				fprintf(cgiOut, "%d", RESULT_FAILURE);
				unlink("/var/tmp/fail");
			}

			break;
		}

		case ACTION_GETFTPUSERPW: {
			char webusername[100];
			char webpassword[100];
			memset(webusername,0,100);
			memset(webpassword,0,100);
			strcpy(webusername,WEBUSERNAME);
			getLanguageValue("sysinfo.txt","webpassword",webpassword);
			fprintf(cgiOut, "{\"username\" : \"%s\", \"password\" : \"%s\"}", webusername, webpassword);
			break;
		}

		default:
		{
			showPage("./login.html",sys_language);
			fprintf(cgiOut, "<script type='text/javascript' src='../ValidationEngine/js/languages/jquery.validationEngine-%s.js'></script>", script_language);
		}
		break;

}
	return 0;
	fflush(stdout);
}
