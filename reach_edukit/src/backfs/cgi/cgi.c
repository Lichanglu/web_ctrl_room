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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#endif

#include "htmllib.h"
#include "cgic.h"
#include "Tools.h"
#include "cgi.h"
#include "weblib.h"

#define	UPDATE_FILE	"/var/log/recserver/update.tgz"
#define ACTION_START 100
#define ACTION_UPLOADFILE  ACTION_START+10
#define ACTION_UPLOADFILE_KEY  ACTION_START+11
#define ACTION_LOGIN  ACTION_START+20
#define ACTION_QUERYSYSPARAM  ACTION_START+30
#define ACTION_CHANGEPWD  ACTION_START+40
#define ACTION_NETWORKSET  ACTION_START+50
#define ACTION_UPDATEVIDEOPARAM  ACTION_START+60
#define ACTION_QUERYAVPARAM  ACTION_START+70
#define ACTION_UPDATEAUDIOPARAM  ACTION_START+80
#define ACTION_TEXT_SET  ACTION_START+90
#define ACTION_LOGOUT  ACTION_START+100
#define ACTION_SYSUPGRADE_STATUS ACTION_START+12

#define PAGE_LEFTMENU_SYSPARAMS_SHOW 301
#define PAGE_LEFTMENU_SYSSET_SHOW 302
#define PAGE_SYSINFO_SHOW 303
#define PAGE_INPUT_SHOW 304
#define PAGE_VIDEO_SHOW 305
#define PAGE_AUDIO_SHOW 306
#define PAGE_CAPTIONLOGO_SHOW 308
#define PAGE_REMOTECTRL_SHOW 309
#define PAGE_NETWORK_SHOW 310
#define PAGE_MODIFYPASSWORD_SHOW 311
#define PAGE_RECTIME_SHOW 312
#define PAGE_SYSUPGRADE_SHOW 313
#define PAGE_RESETDEFAULT_SHOW 314
#define PAGE_VIDEOADVANCEDSET_SHOW 318
#define PAGE_ADDOUTPUT_SHOW 319

#define ACTION_NETWORK_SET 406
#define ACTION_INPUT_SET 407
#define ACTION_INPUTDETAILS_GET 408
#define ACTION_ADJUSTSCREEN_SET 409
#define ACTION_AUDIO_SET 410
#define ACTION_CAPTIONLOGO_SET 411
#define PAGE_REMOTECTRL_SET 412
#define ACTION_REMOTECTRL_UPLOAD 413
#define ACTION_REMOTECTRL_DELETE 414
#define ACTION_SETDEFAULT_SET 418
#define ACTION_UPLOADLOGOPIC_SET 419

#define RESULT_SUCCESS 1
#define RESULT_FAILURE 2
#define RESULT_MUSTMULTIIP 3
#define RESULT_NO_ANSWER (-2)

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
#define UPDATE_LANGUAGE 24
#define UPDATE_HIDDEN_VALUE 300
#define ACTION_SERIALNO_SET 420



#define CGI_NAME "encoder.cgi"
#define BUFFER_LEN					1024
#define MAXFILE_NUM					1024
#define FILENAME_MAXLEN				200

#if (defined(DSS_ENC_1200) || defined(DSS_ENC_1260))

#define UPDATEFILEHEAD "7E7E7E7E48454E43"
#else
#define UPDATEFILEHEAD "7E7E7E7E31313030"
#endif
#define BufferLen 2048
#define UPFILEHEADLEN 8

#define WEBVERSION "1.0.1"

typedef struct package
{
	unsigned int version;
	unsigned int len;
	unsigned int reserves;
	unsigned char hash[32];
}Package;


char sys_password[100];
char sys_webpassword[100];
char sys_timeout[100];
char sys_language[100];

char* trim(char * str)
{
	int len=0;
	int i=0;
	len=strlen(str);
	i=len-1;
	for(; i>0; i--)
	{
		if(str[i]==' '||str[i]==0x0A)
			continue;
		else
			break;
	}
	str[i+1]='\0';
	return str;
}

//0成功
//-1失败
static int loginaction()
{
	char username[100] = {0};
	char password[100] = {0};
	int ret = 0;
	//char strTime[100] = {0};
	cgiFormString("username", username, sizeof(username));
	cgiFormString("password", password, sizeof(password));

	ret = WebGetUsrPassword(sys_password);
	if(ret == -1 || strlen(sys_password) == 0)
	{
		strcpy(sys_password,USERNAME);
	}

	if((!strcmp(username,"admin")&&!strcmp(password,sys_password))
		||(!strcmp(username,"operator")&&!strcmp(password,sys_webpassword))
		||(!strcmp(username,"reachadministrator")&&!strcmp(password,"reachadministrator")))
	{
		long now;
		now = (long)time(NULL);
		showPage("./index.html",sys_language);
		fprintf(cgiOut,"<script type='text/javascript'>");
		fprintf(cgiOut, "setLanguageType('%s');", sys_language);
		fprintf(cgiOut,"</script>");
		cgiSetCookieUser(username);
		cgiSetCookienoTime("sessiontime",now);
		return 0;

	}
	else
	{
		char tmp[150]= {0};
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
	getLanguageValue(sys_language,"uploadFileNull",languagebuf);
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
		getLanguageValue(sys_language,"uploadFileError",languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s [-2]');parent.closeUploading();</script>",languagebuf);
		return -1;
	}

	if ((rec=cgiGetFormFileName("file", fileNameOnServer)) != cgiFormSuccess)
	{
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s [-3]');parent.closeUploading();</script>",languagebuf);
		return -1;
	}

	//根据表单中的值打开文件
	if ((rec=cgiFormFileOpen("file", &file)) != cgiFormSuccess)
	{
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s  [-4]');parent.closeUploading();</script>",languagebuf);
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
		rec = WebSysUpgrade("/tmp/update.bin");
	}
	else if(platform == 1)
	{
//		rec = WebSysUpgrade_6467("/tmp/update.bin");
	}
	else
	{
		rec = -1;
	}
	
	memset(languagebuf,0,150);
	if(rec == 0)
	{
		getLanguageValue(sys_language,"uploadFileSuccess",languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
	}
	else if(rec == SERVER_HAVERECORD_FAILED)
	{
		getLanguageValue(sys_language,"uploadstateerror",languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		//fopen("/tmp/fail","w");
	}
	else
	{
		getLanguageValue(sys_language,"uploadFileFail",languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
	}
	return 0;
}


static int uploadkey(void)
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
	int rec = -1;
	
	char languagebuf[150]= {0};
	getLanguageValue(sys_language,"uploadFileNull",languagebuf);
	if (cgiFormFileName("file", name, sizeof(name)) != cgiFormSuccess)
	{
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
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


	#if 1
	printf("<script>");
	printf("alert('%s');",name);
	printf("</script>");	
	#endif
	#if 0
	if(strstr(name,".bin") == NULL)
	{
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"uploadFileError",languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}
	#endif

	//根据表单中的值打开文件
	if ((rec=cgiFormFileOpen("file", &file)) != cgiFormSuccess)
	{
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}

	strcpy(fileNameOnServer,"/var/tmp/upload.key");
	
	targetFile=fopen(fileNameOnServer,"wb+");
	if(targetFile==NULL)
	{
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}
	
	while (cgiFormFileRead(file, buffer, BufferLen, &got) ==cgiFormSuccess)
	{
		if(got>0)
		{
		
			fwrite(buffer,1,got,targetFile);
		}
	}

	
	cgiFormFileClose(file);
	fclose(targetFile);
	
	return 0;
}

static int uploadProtoclFile(void)
{
	FILE *targetFile;
	cgiFilePtr file;
	char name[128];
	//int retCode = 0;
	int got = 0;
	int t;
	char *tmpStr=NULL;
	char fileNameOnServer[64];
	char buffer[2050];
	int rec = -1;
	int platform = 0;
	
	char languagebuf[150]= {0};
	getLanguageValue(sys_language,"uploadFileNull",languagebuf);
	if (cgiFormFileName("file", name, sizeof(name)) != cgiFormSuccess)
	{
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
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


	//根据表单中的值打开文件
	if ((rec=cgiFormFileOpen("file", &file)) != cgiFormSuccess)
	{
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}

	ProtoList protoList;
	WebGetCtrlProto(&protoList);
	if(protoList.num >= 15)
	{
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}


	sprintf(fileNameOnServer,"/var/log/recserver/%s",name);
	//strcpy(fileNameOnServer,"/var/log/recserver/protocl.prot");
	system("chmod 777 /var/log/recserver/ -R");
	
	targetFile=fopen(fileNameOnServer,"w");
	if(targetFile==NULL)
	{
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}
	
	while (cgiFormFileRead(file, buffer, BufferLen, &got) ==cgiFormSuccess)
	{
		if(got>0)
		{
			fwrite(buffer,1,got,targetFile);
		}
	}
	
	cgiFormFileClose(file);
	fclose(targetFile);

	WebUpgradeCtrlProto(fileNameOnServer);
	
	memset(languagebuf,0,150);
	if(rec == 0)
	{
		getLanguageValue(sys_language,"uploadSuccess",languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
	}
	return 0;
}

static int uploadLogoFile(LogoConfig *config)
{
	FILE *targetFile;
	cgiFilePtr file;
	char name[1024];
	char fileNameOnServer[256];
	char contentType[1024];
	char buffer[1024];
	int size;
	int got;
	if (cgiFormFileName("file", name, sizeof(name)) != cgiFormSuccess) {


		WebSetLogoconfig(config);
		return -1;
	}
	
	cgiHtmlEscape(name);
	cgiFormFileSize("file", &size);
	cgiFormFileContentType("file", contentType, sizeof(contentType));
	cgiHtmlEscape(contentType);
	if (cgiFormFileOpen("file", &file) != cgiFormSuccess) {
		return -1;
	}
	strcpy(fileNameOnServer,"/tmp/logo.png");
	targetFile=fopen(fileNameOnServer,"wb+");
	while (cgiFormFileRead(file, buffer, sizeof(buffer), &got) == cgiFormSuccess)
	{
		fwrite(buffer,1,got,targetFile);
	}
	cgiFormFileClose(file);
	fclose(targetFile);

	strcpy(config->logoname,fileNameOnServer);
	WebSetLogoconfig(config);
	return 0;
}

static int mid_ip_is_multicast(char *ip)
{
	struct in_addr	 addr ;
	unsigned int 	dwAddr;
	unsigned int 	value;
	
	inet_aton(ip, &addr);
	memcpy(&value, &addr, 4);
	dwAddr = htonl(value);
	
	//printf("ip=%s.dwAddr=0x%08x\n", ip, dwAddr);
	
	if(((dwAddr & 0xFF000000) >> 24) > 223)
	{
		return 1;
	}
	
	return 0;
}

static int changePassword()
{
	char password[21]= {0};
	char passwordagain[21]= {0};
	char oldpassword[21]= {0};
	char tmpbuf[100]= {0};
	int rec=0;
	char username[20]= {0};
	int ret = 0;
	int i = 0;
	cgiFormString("password1",password,21);
	cgiFormString("password2",passwordagain,21);
	cgiFormString("oldpassword",oldpassword,21);
	cgiFormString("username",username,20);
	
	if(strcmp(username, USERNAME) != 0)
	{
		memset(tmpbuf,0,100);
		getLanguageValue(sys_language,"usernameNotExist",tmpbuf);
		fprintf(cgiOut, tmpbuf);
		return -1;
	}

	ret = WebGetUsrPassword(sys_password);
	if(ret == -1 || strlen(sys_password) == 0)
	{
		strcpy(sys_password,USERNAME);
	}

	for(i = 0; i < strlen(password); i++)
	{
		if( !(((password[i] >= '0') && (password[i] <= '9')) 
			|| 
			((password[i] >= 'a') && (password[i] <= 'z')))
		  )
		{
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"passformterror",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
	}
	
	
	if(strcmp(oldpassword,sys_password))//旧密码不正确
	{
		memset(tmpbuf,0,100);
		getLanguageValue(sys_language,"oldpassworderror",tmpbuf);
		fprintf(cgiOut, tmpbuf);
		return -1;
	}
	
	if(strcmp(password,passwordagain))
		//密码不同
	{
		memset(tmpbuf,0,100);
		getLanguageValue(sys_language,"passwordTwiceInputNotSame",tmpbuf);
		fprintf(cgiOut, tmpbuf);
		return -1;
	}
	if(password==NULL||strlen(password)==0||!strcmp(password,""))
		//密码为空
	{
		memset(tmpbuf,0,100);
		getLanguageValue(sys_language,"passwordIsNull",tmpbuf);
		fprintf(cgiOut, tmpbuf);
		return -1;
	}
	rec= WebSetUsrPassword(password);
	updateConfigFile("sysinfo.txt","password",password);
	memset(tmpbuf,0,100);
	getLanguageValue(sys_language,"modifyPasswordSuccess",tmpbuf);
	fprintf(cgiOut, tmpbuf);
	return 0;
}

char* Getline(char* pBuf, char* pLine)
{
	char* ptmp = strchr(pBuf, '\n');
	if(ptmp == NULL)
		return pBuf;
	strncpy(pLine, pBuf, ptmp - pBuf);
	return ptmp + 1;
}

int cgiMain(void)
{
	int actionCode  = 0;
	int ret = 0;
	int lang = 0;
	cgiFormInteger("actioncode", &actionCode, 0);
	memset(sys_password,0,100);
	memset(sys_webpassword,0,100);
	memset(sys_timeout,0,100);
	//Get language config

	WebGetEnChSwitch(&lang);
	if(lang == 0)
	{
		strcpy(sys_language,"cn");
	}else if(lang == 1){
		strcpy(sys_language,"us");
	}else{
		strcpy(sys_language,"hk");
	}
	
	fprintf(cgiOut,"Content-Type:text/html;charset=gb2312\n\n");

	switch (actionCode)
	{
		case ACTION_SERIALNO_SET:
		{
			char serialNO[20] = {0};
			cgiFormString("serialNumber", serialNO, 20);

			WebSetDevSerialnum(serialNO);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);

			break;
		}

		case UPDATE_HIDDEN_VALUE:
		{
			char dtype[20]= {0};
			//char temptype[20] = {0};
			cgiFormString("dtype",dtype,16);
			if(strcmp(dtype,""))
			{
				//WebSetDeviceType(dtype,temptype);
			}
			forwardPageU(CGI_NAME,PAGE_SYSTEM_SHOW,"ReachWebAdmin");
			break;
		}
		
		case UPDATE_LANGUAGE:
		{
			int local= 0;
			cgiFormInteger("local", &local, 0);
			//updateConfigFile("sysinfo.txt","language",local);
			WebSetEnChSwitch(local);
			//fprintf(cgiOut, "window.location.href='encoder.cgi';");
			fprintf(cgiOut, "window.location.reload();");
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

		case ACTION_UPLOADFILE_KEY:
		{
			uploadkey();
		}
		break;
		case ACTION_LOGOUT:
		{
			fprintf(cgiOut, "document.cookie = '';\n");
			fprintf(cgiOut, "parent.document.location.replace('/index.html');");
		}
		break;
		
		case PAGE_LOGIN_SHOW:
			showPage("./login.html",sys_language);
			break;
			
		case PAGE_TIMEOUT_SHOW:
			showPage("./timeout.html",sys_language);
			break;
			
		case PAGE_MAIN_SHOW:
		{
			showPage("./index.html",sys_language);
		}
		break;
		
		case PAGE_SYSTEM_SHOW:
		{
			char user[20]= {0};
			IpConfig sysparams;
			//WebGetSysParam(&sysparams);
#ifdef LINUX
			struct in_addr inAddr;
			cgiFormString("u",user,20);
			showPage("./sys_op.html",sys_language);
			printf("<script>\n");
			memcpy(&inAddr,&sysparams.ipaddr,4);
			printf("document.forms[\"networkinfo\"].LanAddr.value='%s';\n",inet_ntoa(inAddr));
			memcpy(&inAddr,&sysparams.netmask,4);
			printf("document.forms[\"networkinfo\"].LanSubMark.value='%s';\n",inet_ntoa(inAddr));
			memcpy(&inAddr,&sysparams.gateway,4);
			printf("document.forms[\"networkinfo\"].LanGateWay.value='%s';\n",inet_ntoa(inAddr));
			
			if(strcmp(user,"ReachWebAdmin")==0)
			{
				printf("document.forms[\"updatepwd\"].oldpassword.value='%s';\n",sys_password);
				printf("showsetBoxSn.style.display='block';\n");
			}
			
			printf("document.getElementById('localeh').value='%s';\n",sys_language);
			if(!strcmp(sys_language,"us"))
				printf("document.getElementById('uslocal').checked=true;\n");
			else
				printf("document.getElementById('cnlocal').checked=true;\n");
				
				
				
			printf("</script>\n");
#else
			cgiFormString("u",user,20);
			showPage("./sys_op.html",sys_language);
			printf("<script>\n");
			if(!strcmp(user,"ReachWebAdmin"))
			{
				printf("document.forms[\"updatepwd\"].oldpassword.value='%s';\n",sys_password);
			}
			printf("</script>\n");
#endif
		}
		break;
		case PAGE_SYSTEM_INFO:
		
			break;
		case RESTART_SHOW:
		{
			showPage("./restart.html",sys_language);
		}
		break;
		case RESTART_SYS:
		{
			if(WebRebootSys(1) != 0)
			{
				fprintf(cgiOut, "%d", RESULT_FAILURE);
			}
			else
			{
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
			}
		}
		break;
		case ACTION_SAVEUPDATE:
		{
			char tmp[100]= {0};
			int rec = -1;
			//rec = WebSaveParam();
			memset(tmp,0,100);
			if(!rec)
				getLanguageValue(sys_language,"savesucess",tmp);
			else
				getLanguageValue(sys_language,"savefailed",tmp);
			forwardPage(CGI_NAME,PAGE_PARAMETER_SHOW,tmp);
			
		}
		break;
		
		case PAGE_LEFTMENU_SYSPARAMS_SHOW:
		{
			showPage("./leftmenuParamsSettings.html", sys_language);
			break;
		}
		case PAGE_LEFTMENU_SYSSET_SHOW:
		{
			showPage("./leftmenuSystemSettings.html", sys_language);
			break;
		}
		
		/*
		 * sysinfo page show action
		 */
		case PAGE_SYSINFO_SHOW:
		{

			DevInfo devinfo = {0};
			DiskInfo disinfo = {0};
			Dm6467Info dm6467info ={0};
			
			WebGetDevInfo(&devinfo);
			WebGetSysDiskInfo(&disinfo);
			WebGet6467Info(&dm6467info);
				
			float disksize = disinfo.disksize;
			disksize	= disksize/1024/1024;
			
			float remainsize = disinfo.remainsize;
			remainsize  = remainsize/1024/1024;
			char exception[20] = {0};
			getLanguageValue(sys_language,"sysinfoException",exception);
			
			showPage("./sysinfo.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [{'name': 'serialNO','value': '%s','type': 'text' }", devinfo.serialnum);
				fprintf(cgiOut, ",{'name': 'deviceModelNO','value': '%s','type': 'text' }", devinfo.devmodel);
				fprintf(cgiOut, ",{'name': 'sysVersion','value': '%s','type': 'text' }", devinfo.sysversion);
				fprintf(cgiOut, ",{'name': 'webVersion','value': '%s','type': 'text' }", CL_WEB_VERSION);
				fprintf(cgiOut, ",{'name': 'kernelversion','value': '%s','type': 'text' }", devinfo.kernelversion);
				fprintf(cgiOut, ",{'name': 'encVersion','value': '%s','type': 'text' }", dm6467info.sysversion);
				fprintf(cgiOut, ",{'name': 'encKernelVersion','value': '%s','type': 'text' }", dm6467info.uImageversion);
				fprintf(cgiOut, ",{'name': 'FPGAVersion','value': '%s','type': 'text' }", dm6467info.fpgaversion);

				if(disksize < 1)
				{
					fprintf(cgiOut, ",{'name': 'hardDiskSpace','value': '%s','type': 'text' }", exception);
					fprintf(cgiOut, ",{'name': 'freeDiskSpace','value': '%s','type': 'text' }", exception);	
				}
				else
				{
					fprintf(cgiOut, ",{'name': 'hardDiskSpace','value': '%0.2f GByte','type': 'text' }", disksize);
					fprintf(cgiOut, ",{'name': 'freeDiskSpace','value': '%0.2f GByte','type': 'text' }", remainsize);	
				}
				fprintf(cgiOut, ",{'name': 'mediaIp','value': '%s','type': 'text' }", devinfo.mediactrip);
				fprintf(cgiOut, ",{'name': 'IPAddress','value': '%s','type': 'text' }]);\n", devinfo.manageplatformip);
			fprintf(cgiOut, "</script>\n");

			break;
		}
		
		/*
		 * page input show action
		 */
		case PAGE_INPUT_SHOW:
		{
			char inputInfo[100] = {0};
			int colorSpace = 0;
			int hdcp = 0;
			int inputType = 0;
			showPage("./input.html", sys_language);
			
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "setFormItemValue('wmform', [{'name': 'inputInfo','value': '%s','type': 'text' }", inputInfo);
			fprintf(cgiOut, ",{'name': 'colorSpace','value': '%d','type': 'select' }",colorSpace);
			fprintf(cgiOut, ",{'name': 'inputType','value': '%d','type': 'select' }",inputType);
			fprintf(cgiOut, ",{'name': 'hdcp','value': '%d','type': 'text'}]);\n", hdcp);
			fprintf(cgiOut, "formBeautify();\n");
			fprintf(cgiOut, "</script>\n");
			break;
		}
		
		/*
		 * audio page show action
		 */
		case PAGE_AUDIO_SHOW:
		{
			char audio[200] = {0};
			char set[20] = {0};
			int i = 0;
			showPage("./audio.html", sys_language);
			WebGetAudioType(audio);

			int j = 0;
			
			if(audio[0] == '1')
			{
				set[j] = '1';
				j = j+2;
			}	

			if(audio[1] == '1')
			{
				set[j-1] = ',';
				set[j] = '2';
				j = j+2;
			}	

			if(audio[2] == '1')
			{
				set[j-1] = ',';
				set[j] = '3';
				j = j+2;
			}

			if(audio[3] == '1')
			{
				set[j-1] = ',';
				set[j] = '4';
				j = j+2;
			}	
			
		
			
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "initAuto('%s');\n",set);
			fprintf(cgiOut, "</script>\n");

			

			
			break;
		}
		
		/*
		 * audio page set action
		 */
		case ACTION_AUDIO_SET:
		{
			char audio[10] = {0};
			char set[5] = {'0','0','0','0'};
			cgiFormString("audio", audio, 10);
			int i = 0;
			char *p= NULL;
			p = strchr(audio,'2');
			if(NULL != p)
			{
				set[0] = '1';
				set[2] = '1';
			}

			p=strchr(audio,'3');
			if(NULL != p)
			{
				set[1] = '1';
				set[3] = '1';
			}
			
				
			WebSetAudioType(set);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}
		
		/*
		 * page remotectrl show action.
		 */
		case PAGE_REMOTECTRL_SHOW:
		{
			int ptzProtocal = 0;
			ProtoList protoList;
			WebGetCtrlProto(&protoList);
			
			showPage("./remotectrl.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "setProtoList([''");
			int i=0;
			for(i=0; i<protoList.num; i++){
				fprintf(cgiOut, ",'%s'", protoList.protolist[i]);
			}
			fprintf(cgiOut, "]);\n");
			fprintf(cgiOut, "setFormItemValue('wmform', [");
			fprintf(cgiOut, "{'name': 'ptzProtocal','value': '%s','type': 'select'}]);\n", protoList.curproto);
			fprintf(cgiOut, "formBeautify();");
			fprintf(cgiOut, "</script>\n");
			break;
		}
		
		/*
		 * page remotectrl set action.
		 */
		case PAGE_REMOTECTRL_SET:
		{
			char ptzProtocal[20] = {0};
			cgiFormString("ptzProtocal",ptzProtocal,20);
			WebSetCtrlProto(ptzProtocal);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}
		
		case ACTION_REMOTECTRL_DELETE:
		{
			char ptzProtocal[20] = {0};
			cgiFormString("ptzProtocal",ptzProtocal,20);
			WebDelCtrlProto(ptzProtocal);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}
		
		case ACTION_REMOTECTRL_UPLOAD:
		{
			uploadProtoclFile();
		}
		break;
		
		/*
		 * page captionlogo show action
		 */
		case PAGE_CAPTIONLOGO_SHOW:
		{
			int cap_position = 0;
			
			char cap_text[128] = {0};
			int cap_displaytime = 0;
			
			int logo_position = 0;
			int webinput = 1;
			//add by zm
			cgiFormInteger("input", &webinput, 1);
			
			showPage("./captionlogo.html", sys_language);
			
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "setFormItemValue('wmform', [");
			fprintf(cgiOut, ",{'name': 'cap_position','value': '%d','type': 'select' }", cap_position);
			fprintf(cgiOut, ",{'name': 'cap_text','value': '%s','type': 'text' }", cap_text);
			fprintf(cgiOut, ",{'name': 'cap_displaytime','value': '%d','type': 'checkbox' }", cap_displaytime);
			fprintf(cgiOut, ",{'name': 'logo_position','value': '%d','type': 'select' }]);\n", logo_position);
			//fprintf(cgiOut, "formBeautify();");
			fprintf(cgiOut, "initInputTab(%d);\n", webinput);
			fprintf(cgiOut, "initPositionSelect();");
			fprintf(cgiOut, "</script>\n");
			break;
		}
		
		/*
		 * page captionlogo set action.
		 */
		case ACTION_CAPTIONLOGO_SET:
		{
			char tmp[150]= {0};
			int cap_position = 0;
			char cap_text[200] = {0};
			int cap_displaytime = 0;
			
			int logo_position = 0;
			int input = 0;
			
			//TextInfo info = {0};
			
			cgiFormInteger("cap_position", &cap_position, 0);
			cgiFormString("cap_text", cap_text, 1000);
			cgiFormInteger("cap_displaytime", &cap_displaytime, 0);
			cgiFormInteger("logo_position", &logo_position, 0);
			cgiFormInteger("input", &input, 0);

			OsdConfig osdconfig = {0};
			osdconfig.input   = input;
			osdconfig.DisTime = cap_displaytime;
			osdconfig.TitlePositon = cap_position;
			memcpy(osdconfig.Title, cap_text, 199);
			if(input != 3)		
				WebSetOsdconfig(&osdconfig);
 			
			getLanguageValue(sys_language,"opt.success",tmp);
			fprintf(cgiOut, "<script type='text/javascript'>alert('%s');</script>", tmp);
			break;
		}
		
		case ACTION_UPLOADLOGOPIC_SET:
		{
			// update logo pictrue.
			//WebUploadLogo("/opt/reach/logoimg/logo.png");
			int logo_position = 0;
			int input = 0;
			cgiFormInteger("logo_position", &logo_position, 0);
			cgiFormInteger("input", &input, 0);

			LogoConfig config = {0};
			config.input = input;
			config.LogoPositon = logo_position;
			if(input == 3)	
				config.LogoPositon = config.LogoPositon - 1;
			uploadLogoFile(&config);
			
			
		}
		break;
		
		/*
		 * page network show action
		 */
		case PAGE_NETWORK_SHOW:
		{
			int dhcp = 0;
			IpConfig ipConfig={0};
			//memset(&ipConfig, 0, sizeof(IpConfig));
			WebGetWanIpconfig(&ipConfig);
			showPage("./network.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "setFormItemValue('wmform', [");
			//外网IP
			fprintf(cgiOut, "{'name': 'IPAddress','value': '%s','type': 'text' }", ipConfig.ipaddr);
			fprintf(cgiOut, ",{'name': 'subMask','value': '%s','type': 'text' }", ipConfig.netmask);
			fprintf(cgiOut, ",{'name': 'gateWay','value': '%s','type': 'text' }", ipConfig.gateway);
			//fprintf(cgiOut, ",{'name': 'dhcp','value': '%d','type': 'checkbox' }", ipConfig.iptype);
			
			WebGetLanIpconfig(&ipConfig);
			//内网IP
			fprintf(cgiOut, ",{'name': 'IPAddress1','value': '%s','type': 'text' }", ipConfig.ipaddr);
			fprintf(cgiOut, ",{'name': 'subMask1','value': '%s','type': 'text' }", ipConfig.netmask);
			fprintf(cgiOut, ",{'name': 'gateWay1','value': '%s','type': 'text' }", ipConfig.gateway);

			fprintf(cgiOut, ",{'name': 'dhcp1','value': '%d','type': 'checkbox' }]);\n", ipConfig.iptype);
			fprintf(cgiOut, "formBeautify();");
			fprintf(cgiOut, "initDHCPCheckBox();");
			fprintf(cgiOut, "</script>\n");
			break;

		}
		
		/*
		 * page network set action
		 */
		case ACTION_NETWORK_SET:
		{
			int dhcp = 0;
			int dhcp1 = 0;
			char IPAddress[20] = {0};
			char gateWay[20] = {0};
			char subMask[20] = {0};
			char IPAddress1[20] = {0};
			char gateWay1[20] = {0};
			char subMask1[20] = {0};
			char tmp[150]= {0};
			
			IpConfig sysParamsIn;//外网IP
			IpConfig sysParamsIn1;//内网IP
			//外网IP
			cgiFormInteger("dhcp", &dhcp, 0);
			cgiFormString("IPAddress", IPAddress, 20);
			cgiFormString("gateWay", gateWay, 20);
			cgiFormString("subMask", subMask, 20);
			
			//内网IP
			cgiFormInteger("dhcp1", &dhcp1, 0);
			cgiFormString("IPAddress1", IPAddress1, 20);
			cgiFormString("gateWay1", gateWay1, 20);
			cgiFormString("subMask1", subMask1, 20);
		
			sysParamsIn.iptype = 0;
			strcpy(sysParamsIn.ipaddr,"169.254.0.2");
			strcpy(sysParamsIn.netmask,"255.255.255.0");
			strcpy(sysParamsIn.gateway,"169.254.0.254");

			WebSetWanIpconfig(&sysParamsIn);

			sysParamsIn1.iptype = dhcp1;
			strcpy(sysParamsIn1.ipaddr,IPAddress1);
			strcpy(sysParamsIn1.netmask,subMask1);
			strcpy(sysParamsIn1.gateway,gateWay1);

			//调用接口需要修改一下
			WebSetLanIpconfig(&sysParamsIn1);

			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}
		
		case PAGE_MODIFYPASSWORD_SHOW:
		{
			char username[20] = {0};
			
			showPage("./modifypassword.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "formBeautify();");
			fprintf(cgiOut, "</script>\n");
			break;
		}
		
		case PAGE_RECTIME_SHOW:
		{
			showPage("./recordtime.html", sys_language);
			break;
		}
		
		case PAGE_SYSUPGRADE_SHOW:
		{
			showPage("./sysupgrade.html", sys_language);
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
		
		case PAGE_RESETDEFAULT_SHOW:
		{
			showPage("./resetdefault.html", sys_language);
			break;
		}
		
		case ACTION_SETDEFAULT_SET:
		{
			int ret = 0;
			ret = WebSysRollBack("");
			if(ret == 0)
			{
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
		
		default :
		{
			showPage("./login.html",sys_language);
		}
		break;
	}
	return 0;
}
fflush(stdout);
