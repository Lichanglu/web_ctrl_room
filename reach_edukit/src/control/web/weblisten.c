#include "weblisten.h"
#include "common.h"
#include "lcd.h"
#include <unistd.h>
#include <sys/types.h>
#include <linux/rtc.h>
#include "control_log.h"
#include "common.h"
#include "control.h"
#include "../enc/middle_control.h"
//#include <iconv.h>
#include "iconv.h"

#include "nslog.h"

#define MP_ENC_INDEX   1

#define r_printf printf

#define THREAD_SUCCESS      (void *) 0
#define THREAD_FAILURE      (void *) -1

#define MAX_LISTEN		 10
#define VERIFYLEN        (4*1024)
#define SYSCMDLEN		 (1024)
//##########################定义升级包临时路径及名称#########################
#define TARGZPATH         "/var/log/recserver/reach/"
#define UPGRADEGZPACK     "update.tar.gz"   //压缩包
#define UPGRADEPACK		  "update.tar"      //TAR包
#define VERIFYFILE        "hash"            //校验文件的名称

#define APPDIRPATH 		  "/usr/local/reach"		    //实际应用程序
#define BACKUPDIR         "/var/log/recserver/reach/.appbackup"       //备份分区
#define TMPBACKUPDIR      "/var/log/recserver/reach/.tmpappbackup"    //备份临时区
#define TMPAPPDIRPATH     "/var/log/recserver/reach/.tmpapp"          //应用临时分区

#define IsHaveErrorApp    "/var/log/recserver/reach/.errorapp"             //标示升级或备份异常 应用备份
#define IsHaveErrorBack   "/var/log/recserver/reach/.errorback"             //标示升级或备份异常 应用备份
#define IsHaveBack		  "/var/log/recserver/reach/.backup" 			 //标示是否存在备份

#define SETETH0STATIC(file,ip,gateway,netmask)\
fprintf(file,"ifconfig eth0 %s netmask %s\n"\
 			"route add default gw %s\n"\
			,ip,netmask,gateway);

#define SETETH0DHCP(file)\
fprintf(file,"/sbin/dhcpcd eth0\n");

#define SETETH1STATIC(file,ip,netmask)\
fprintf(file,"ifconfig eth1 %s netmask %s\n"\
			,ip,netmask);

#define WEBSELECTTIMEOUT 6

static int32_t xml_parse_return_msg(int32_t sockid,char* buffer);
static int32_t xml_packet_get_msg(char* out_buf,int32_t enc_type,int msg_type,int msg_code,const char* sub_key);

static char plog_head[32];
typedef struct _HDB_MSGHEAD
{
	short nLen;
	short nVer;
	char nMsg;
	char szTemp[3];
}HDB_MSGHEAD;

Dm6467Info g6467info = {{0}};
static zlog_category_t *z = NULL;
extern server_set *gpser;


extern int32_t SetUsbFunction(int32_t IsUse);
extern int32_t GetUsbFunction(int32_t *IsUse);
extern int controlWebSetPreviewPassword(char *password);

extern int32_t gpio_fd;


#if 0
int32_t RecoverApp()
{
	int8_t cmd[SYSCMDLEN] = {0};
	sprintf(cmd, "rm -fr %s", APPDIRPATH);
	if(0 == r_system(cmd))
	{
		sprintf(cmd , "cp -fr %s %s/",TMPAPPDIRPATH, APPDIRPATH);
		if(0 == r_system(cmd))
		{
			unlink(TMPAPPDIRPATH);
			sprintf(cmd, "rm -fr %s",TMPAPPDIRPATH);
			r_system(cmd);
			return 1;
		}
	}
	
	return 0;
}
int32_t CheckAppStateRepair()
{
	if(0 == access(IsHaveErrorApp,0))
	{
		return RecoverApp();
	}
	return 1;
}
#endif

static int select_socket(int socket, int secode)
{
	fd_set fdsr;
	struct timeval tv;

	FD_ZERO(&fdsr);
	FD_SET(socket, &fdsr);

	tv.tv_sec  = secode;
	tv.tv_usec = 0;
	
	return select(socket + 1, &fdsr, NULL, NULL, &tv);
}

static int get_text_size(char *Data)
{
	int n_Len = strlen(Data);
	unsigned char *lpData = (unsigned char *)Data;
	int i = 0;
	int Sum = 0;

	for(i = 0; /**pStr != '\0'*/ i < n_Len ;) {
		if(*lpData <= 127) {
			i++;
			lpData++;
		} else {
			i += 2;
			lpData += 2;
		}

		Sum++;
	}

	return i;
}

int8_t *basename(int8_t *path)
{
  /* Ignore all the details above for now and make a quick and simple
     implementaion here */
  int8_t *s1;
  int8_t *s2;

  s1=(int8_t *)strrchr((char *)path, '/');
  s2=(int8_t *)strrchr((char *)path, '\\');

  if(s1 && s2) {
    path = (s1 > s2? s1 : s2)+1;
  }
  else if(s1)
    path = s1 + 1;
  else if(s2)
    path = s2 + 1;

  return path;
}


static int connectToServer()
{
	struct sockaddr_in serv_addr;
	const char* pAddr  = "127.0.0.1";
	int socketFd = -1;
	
	socketFd= socket(PF_INET, SOCK_STREAM, 0);
	if(socketFd < 1)
		return -1;
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(CONTROL_FOR_SERVER_PORT);
	inet_aton(pAddr,(struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero),8);


	if(connect(socketFd, (struct sockaddr *)&serv_addr,sizeof(struct sockaddr)) == -1)	{

		printf("connect fail %s",strerror(errno));
		close(socketFd);
		return -1;
	}

	set_send_timeout(socketFd, 30);
	set_recv_timeout(socketFd, 30);
	return socketFd;
}


int ConnectDM6467()
{
	struct sockaddr_in serv_addr;
	const char* pAddr  = "169.254.0.30";
	int socketFd;
	
	socketFd= socket(PF_INET, SOCK_STREAM, 0);
	if(socketFd < 1)
		return -1;
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(3100);
	inet_aton(pAddr,(struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero),8);
	
	if(connect(socketFd, (struct sockaddr *)&serv_addr,sizeof(struct sockaddr)) == -1)	{
		close(socketFd);
		return -1;
	}

	set_send_timeout(socketFd, 30);
	set_recv_timeout(socketFd, 30);
	return socketFd;
}

int32_t updatekernel(void)
{
	char command[256] = {0};
	char filepath[256] = {0};
	system(command);
	sprintf(filepath,"/var/log/recserver/reach/%s","uImage");
	/*?üD??úo?*/
	if(access(filepath,F_OK) ==0)
	{
			system("flash_eraseall /dev/mtd6");
			sprintf(command,"nandwrite -p /dev/mtd6 /var/log/recserver/reach/%s","uImage");
			system(command);
			system("rm -f /var/log/recserver/reach/uImage");
			system("sync");
	}
	return 0;
}

int GetKernelVersion(int32_t fd, char *version)
{
	static char kversion[64] = {0};
	
	if(fd <= 0 || version == NULL){
		return -1;
	}

	if(strlen(kversion)==0){
		ioctl(fd, 0x11111111, version);
		strcpy(kversion, version);
	}
	else{
		strcpy(version, kversion);
	}
	
	printf("%s\n",version);
	
	return 0;
}

int Dm6467_Upgrade(int8_t *filename)
{
	int socketFd = 0;
	int8_t buffer[1024] = {0};
	unsigned int filelen = 0;
	HDB_MSGHEAD msgHead = {0};

	chown((char *)filename, 0, 0);
	FILE *file = NULL;
	file = fopen((char *)filename,"r");
	if(file == NULL)
	{
		printf("Dm6467_Upgrade Open fail!!\n");
		return 0;
	}

	fread(buffer, 1, 8 , file);

	if(!((buffer[0] == 0x7e)&&(buffer[1] == 0x7e)&&(buffer[2] == 0x7e) && (buffer[3] == 0x7e)
							&& (buffer[4] == 0x48) && (buffer[5] == 0x45) && (buffer[6] == 0x4e) && (buffer[7] == 0x43)))
	{
			printf("Dm6467_Upgrade formt fail!!\n");
		return 1;
	}
	
	socketFd =  ConnectDM6467();

	if(socketFd < 0)
	{
		printf("Dm6467_Upgrade Connect fail!!\n");
		fclose(file);
		return SERVER_SYSUPGRADE_FAILED;
	}

	fseek(file,0L,SEEK_END);
	filelen = ftell(file);
	filelen = filelen - 8;
	
	fseek(file, 8L, SEEK_SET);

	msgHead.nMsg = 0x13;
	msgHead.nVer = 1;
	msgHead.nLen = htons(sizeof(HDB_MSGHEAD)+4);

	tcp_send_longdata(socketFd, (int8_t *)&msgHead, sizeof(HDB_MSGHEAD));

	printf("Start Upgrade len [%u]\n",filelen);
	tcp_send_longdata(socketFd, (int8_t *)&filelen, sizeof(int));

	while(1)
	{
		int n = 0;
		n = fread(buffer, 1, 1024 , file);
		if(n <= 0)
			break;
		tcp_send_longdata(socketFd, (int8_t *)buffer, n);
	}

	fclose(file);

	unlink((char *)filename);

	int select_ret = 0;

	select_ret = select_socket(socketFd, WEBSELECTTIMEOUT*50);
	if (select_ret == 0) {
		printf("select_socket time out error!!![%s]\n", strerror(errno));
		return -1;
	} else if (select_ret < 0) {
		printf("select_socket error!!![%s]\n", strerror(errno));
		return -1;
	}

	tcp_recv_longdata(socketFd, (int8_t *)&msgHead, sizeof(HDB_MSGHEAD));
	if(msgHead.nMsg == 21)
	{
		printf("Dm6467_Upgrade is fail!!!\n");
		return 1;
	}
	else if(msgHead.nMsg == 22)
	{
		printf("Dm6467_Upgrade is success!!!\n");
		return 0;
	}
	else
	{
		printf("unkown error!!!\n");
		return 1;
	}

	return 1;
}

int32_t StartUpgrade()
{
	FILE *file = fopen(IsHaveErrorApp,"w");
	if(NULL != file)
	{
		fclose(file);
	}
	r_system((int8_t*)"sync");
	return 1;
}

int32_t StopUpgreade()
{
	unlink(IsHaveErrorApp);
	r_system((int8_t*)"sync");
	return 1;
}

int32_t StartBackup()
{
	FILE *file = fopen(IsHaveErrorBack,"w");
	if(NULL != file)
	{
		fclose(file);
	}
	r_system((int8_t*)"sync");
	return 1;
}

int32_t StopBackup()
{
	unlink(IsHaveErrorBack);
	r_system((int8_t*)"sync");
	return 1;
}

/*==============================================================================
    函数: <CheckIsHaveBackUp>
    功能: 检测是否有备份
    参数: 
    返回值: 
    Created By 徐崇 2012.12.14 21:19:13 For Web
==============================================================================*/
int32_t CheckIsHaveBackUp()
{
	if(0 == access(IsHaveBack,0))
	{
		return 1;
	}
	return 0;
}

int32_t uninstallpack()
{
	int8_t cmd[SYSCMDLEN] = {0};
	/* 卸载升级包*/
	sprintf((char *)cmd, "rm -f %s%s",TARGZPATH,UPGRADEPACK);
	r_system(cmd);
	return 1;
}

/*==============================================================================
    函数: <rebootsys>
    功能: 重启
    参数: 
    返回值: 
    Created By 徐崇 2012.12.14 21:19:02 For Web
==============================================================================*/
int32_t rebootsys(int32_t time)
{
//	stop_lcd_display_task();
	zlog_debug(z," <GOD + 1> MSG_SYSREBOOT\n");

    pid_t pid = fork();      
    if (pid == 0)    
    {
    	
		zlog_debug(z," <GOD + 2> MSG_SYSREBOOT\n");
	   reboot_server2();
	   exit(1);
    }
	zlog_debug(z," <GOD + 3> MSG_SYSREBOOT\n");

	return 1;
}

/*==============================================================================
    函数: <TmpBackupApp>
    功能: 暂存应用
    参数:
    返回值:
    Created By 徐崇 2012.11.26 09:24:47 For Web
==============================================================================*/
int32_t TmpBackupApp()
{
	int8_t cmd[SYSCMDLEN] = {0};
	sprintf((char *)cmd, "rm -fr %s",TMPAPPDIRPATH);
	printf("-----cmd[%s]\n",cmd);
	if(0 == r_system(cmd))
	{
		// 暂存应用
		sprintf((char *)cmd, "cp %s %s -fr",APPDIRPATH, TMPAPPDIRPATH);
		printf("-----cmd[%s]\n",cmd);
		if(0 == r_system(cmd))
		{
			r_system((int8_t*)"sync");
			return 1;
		}
		else
		{
			sprintf((char *)cmd, "rm -fr %s",TMPAPPDIRPATH);
			printf("-----cmd[%s]\n",cmd);
			r_system(cmd);
		}
	}
	return 0;
}

/*==============================================================================
    函数: <AppUpgrade>
    功能: 升级操作
    参数:
    返回值:
    Created By 徐崇 2012.11.26 09:38:15 For Web
==============================================================================*/
int32_t AppUpgrade()
{
	int8_t cmd[SYSCMDLEN] = {0};

	//开始升级
	StartUpgrade();
	sprintf((char *)cmd, "tar -xvf %s%s -C /",TARGZPATH, UPGRADEPACK);
	printf("-----cmd[%s]\n",cmd);
	if(0 == r_system(cmd))
	{
		r_system((int8_t*)"sync");
		//升级成功
		StopUpgreade();
		return 1;
	}

	//升级失败
	sprintf((char *)cmd, "rm %s -fr",APPDIRPATH);
	r_system(cmd);

	//恢复
	sprintf((char *)cmd, "mv %s %s -f",TMPAPPDIRPATH, APPDIRPATH);
	r_system(cmd);
	
	r_system((int8_t*)"sync");
	//升级结束
	StopUpgreade();
	return 0;
}


/*==============================================================================
    函数: <verifyUpgradePackage>
    功能: 校验包合法
    参数: 
    返回值: 
    Created By 徐崇 2012.12.14 21:17:15 For Web
==============================================================================*/
static int32_t verifyUpgradePackage(int8_t *filename)
{
	int32_t ret 				= -1;
	int8_t cmd[SYSCMDLEN]       = {0};
//	int8_t buffer[VERIFYLEN*2]  = {0};
//	int8_t *pHash           = NULL;
//	int8_t *pbuffer         = buffer;
	FILE   *hashfile        = NULL;
	FILE   *updatefile      = NULL;

	//移动升级包
	sprintf((char *)cmd, "mv -f %s %s%s ",filename,TARGZPATH, UPGRADEGZPACK);
	printf("-----cmd[%s]\n",cmd);
	ret = r_system(cmd);
	if(0 != ret)
	{
		ret = -1;
		goto FAIL;
	}

	//解压升级包
	sprintf((char *)cmd, "gunzip -f %s%s",TARGZPATH, UPGRADEGZPACK);
	printf("-----cmd[%s]\n",cmd);
	ret = r_system(cmd);
	if(0 != ret)
	{
		ret = -1;
		goto FAIL;
	}

	#if 0
	//提取校验文件
	sprintf((char *)cmd, "tar -xvf %s%s -C %s %s",TARGZPATH, UPGRADEPACK,TARGZPATH,VERIFYFILE);
	printf("-----cmd[%s]\n",cmd);
	ret = r_system(cmd);
	if(0 != ret)
	{
		ret = -1;
		goto FAIL;
	}


	//删除包中检验文件
	sprintf((char *)cmd, "tar --delete -f %s%s %s",TARGZPATH, UPGRADEPACK, VERIFYFILE);
	printf("-----cmd[%s]\n",cmd);
	ret = r_system(cmd);
	if(0 != ret)
	{
		ret = -1;
		goto FAIL;
	}

	//校验
	sprintf((char *)cmd, "%s%s",TARGZPATH, UPGRADEPACK);
	updatefile = fopen((char *)cmd,"r");
	if(updatefile == NULL)
	{
		ret = -1;
		goto FAIL;
	}
	
	if(VERIFYLEN == fread(pbuffer,1, VERIFYLEN, updatefile))
	{
		if(0 == fseek(updatefile,-(VERIFYLEN),SEEK_END))
		{
			if(VERIFYLEN == fread(pbuffer + VERIFYLEN, 1, VERIFYLEN, updatefile))
			{
				pHash = (int8_t*)MDString((char *)pbuffer, VERIFYLEN*2);
			}
		}
	}
	
	if(NULL == pHash)
	{
		ret = -1;
		goto FAIL;
	}
	sprintf((char *)cmd, "%s%s",TARGZPATH, VERIFYFILE);
	hashfile = fopen((char *)cmd,"r");
	if(hashfile == NULL)
	{
		ret = -1;
		goto FAIL;
	}
	
	fgets((char *)buffer, 33, hashfile);
	printf("buffer =[%s][%s]\n",buffer, pHash);
	if(0 != r_strncmp(buffer, pHash, 32))
	{
		ret = -1;
		goto FAIL;
	}

	#endif
	
	ret = 1;
FAIL:
	if(NULL != updatefile)
	{
		fclose(updatefile);
	}
	
	if(NULL != hashfile)
	{
		fclose(hashfile);
	}
	
	return ret;
	
}

/*==============================================================================
    函数: <SysBackup>
    功能: 系统备份
    参数: 
    返回值: 
    Created By 徐崇 2012.12.14 21:17:07 For Web
==============================================================================*/
int32_t SysBackup()
{
	int8_t cmd[SYSCMDLEN] = {0};
	r_mkdir((int8_t *)BACKUPDIR, S_IRWXU);
	
	//清除暂存文件
	sprintf((char *)cmd ,"rm %s  -rf ",TMPBACKUPDIR);
	if(0 != r_system(cmd))
	{
		return 0;
	}
	
	StartBackup();
	//暂存备份文件
	sprintf((char *)cmd ,"mv %s %s -f",BACKUPDIR, TMPBACKUPDIR);
	if(0 != r_system(cmd))
	{
		return 0;
	}

	// 备份应用
	sprintf((char *)cmd ,"mv %s %s -f",TMPAPPDIRPATH, BACKUPDIR);
	if(0 != r_system(cmd))
	{
		//备份失败
		sprintf((char *)cmd ,"rm %s -fr",BACKUPDIR);
		r_system(cmd);

		//恢复备份区
		sprintf((char *)cmd ,"mv  %s %s -f",TMPBACKUPDIR, BACKUPDIR);
		r_system(cmd);
		StopBackup();
		return 0;
	}
	else
	{
		r_system((int8_t*)"sync");
		StopBackup();
		FILE *file = fopen(IsHaveBack,"w");
		fclose(file);
		//删除暂存文件
		sprintf((char *)cmd ,"rm %s -fr",TMPBACKUPDIR);
		r_system(cmd);
	}
	
	return 1;
}

/*==============================================================================
    函数: <SysRollback>
    功能: 版本回退
    参数: 
    返回值: 
    Created By 徐崇 2012.11.26 11:14:20 For Web
==============================================================================*/
int32_t SysRollback()
{
	int32_t web_ret = SERVER_SYSROLLBACK_FAILED;
	int8_t cmd[SYSCMDLEN] = {0};
	if(1 == CheckIsHaveBackUp())
	{

		sprintf((char *)cmd,"mkdir -p %s",APPDIRPATH);
		r_system(cmd);
		
		//暂存应用
		if(1 != TmpBackupApp())
		{
			return web_ret;
		}

		printf("TmpBackupApp is success!\n");
		
		StartUpgrade();
		//卸载应用
		sprintf((char *)cmd,"rm -fr %s",APPDIRPATH);
		if(0 != r_system(cmd))
		{

			printf("%s Is fail!!/n",cmd);
			//恢复应用
			sprintf((char *)cmd,"mv -f %s %s",TMPAPPDIRPATH, APPDIRPATH);
			r_system(cmd);
			StopUpgreade();
			goto FAIL;
		}
		else
		{
			
			sprintf((char *)cmd,"cp -fr %s %s/",BACKUPDIR, APPDIRPATH);
			if(0 != r_system(cmd))
			{
				//回退失败
				sprintf((char *)cmd,"rm -fr %s",APPDIRPATH);
				r_system(cmd);

				//恢复
				sprintf((char *)cmd,"mv -f %s %s",TMPAPPDIRPATH, APPDIRPATH);
				r_system(cmd);

				r_system((int8_t*)"sync");
				StopUpgreade();
				goto FAIL;
			}
			
		}
		r_system((int8_t*)"sync");
		//回退成功
		StopUpgreade();
		sprintf((char *)cmd,"rm -fr %s",BACKUPDIR);
		r_system(cmd);
		unlink(IsHaveErrorBack);
		unlink(IsHaveErrorApp);
		unlink(IsHaveBack);
		web_ret = SERVER_RET_OK;
	}
	else
	{
		web_ret = SERVER_VERIFYFILE_FAILED;
		return web_ret;
	}
	
FAIL:
	sprintf((char *)cmd, "rm -fr %s",TMPAPPDIRPATH);
	r_system(cmd);
	r_system((int8_t*)"sync");
	return web_ret;
}


/*==============================================================================
    函数: <UpgradeApp>
    功能: 升级
    参数: 
    返回值: 
    Created By 徐崇 2012.12.14 21:16:37 For Web
==============================================================================*/
int32_t UpgradeApp(int8_t *filename)
{
	int32_t IsUpgrade = 0;
	int8_t  cmd[SYSCMDLEN] = {0};
	int32_t web_ret = SERVER_SYSUPGRADE_FAILED;
	
	//保证存在以下路

	r_system((int8_t *)"mkdir -p /var/log/recserver/reach");
	//r_mkdir((int8_t*)"/var",S_IRWXU);
	//r_mkdir((int8_t*)"/var/tmp",S_IRWXU);
	//r_mkdir((int8_t*)"/var/log/recserver/reach",S_IRWXU);
	//r_mkdir((int8_t*)TARGZPATH, S_I	RWXU);
	//r_mkdir((int8_t*)APPDIRPATH,S_IRWXU);
	
	/* 检验安装包是否正确 */
	IsUpgrade = verifyUpgradePackage(filename);
	printf("------>%d\n",IsUpgrade);
	
	/* 安装包校验失败 */
	if(1 != IsUpgrade)
	{
		web_ret = SERVER_VERIFYFILE_FAILED;
		goto FAIL;
	}

	printf("verifyUpgradePackage Is success!!\n");
	// 暂存应用
	if(1 != TmpBackupApp())
	{
		web_ret = SERVER_VERIFYFILE_FAILED;
		goto FAIL;
	}
	else
	{
		printf("------TmpBackupApp success\n");
		// 升级
		if(1 == AppUpgrade())
		{
			web_ret = SERVER_RET_OK;
			printf("------AppUpgrade success\n");

			//升级成功开始备份
			SysBackup();
			printf("------SysBackup success\n");
		}

		printf("AppUpgrade Done ....\n");
		sprintf((char *)cmd, "rm -fr %s",TMPAPPDIRPATH);
		r_system(cmd);
	}

	
FAIL:
	uninstallpack();
	r_system((int8_t*)"sync");
	return web_ret;
}



/*==============================================================================
    函数: <GetDevInfo>
    功能: 获取设备信息
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:15:51 For Web
==============================================================================*/
int32_t	GetDevInfo(DevInfo *devinfo)
{
	struct in_addr addr;
	if((NULL == gpser) ||(NULL == gpser->pserinfo)|| (devinfo == NULL))
	{
		return 0;
	}
	
	server_info info = {{0}};
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&info, &gpser->pserinfo->ServerInfo, sizeof(server_info));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);

	r_memset((int8_t *)devinfo->devmodel, 0, 64);
	r_strcpy((int8_t *)devinfo->devmodel, (const int8_t *)info.ServerType);
	
	r_strcpy((int8_t *)devinfo->serialnum, (const int8_t *)info.ServerSeries);
	r_strcpy((int8_t *)devinfo->sysversion, (const int8_t *)info.ServerVersion);

	r_memcpy(&addr, &info.LanAddr, 4);
	r_strcpy((int8_t *)devinfo->serverip, (int8_t *)inet_ntoa(addr));

	config_info confinfo = {0};
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&confinfo, &gpser->pserinfo->ConfigInfo, sizeof(config_info));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);

	if(gpser->pserinfo->FtpInfo.Mode != FTP_MODE_MEDIACENTER){
		fprintf(stderr, "\n\n\n\n\n\n\n\nn\n\n\n\nn\n\n\n\n\n\n\n\n\n");
		r_memset((int8_t *)devinfo->mediactrip, 0, 20);
		r_strcpy((int8_t *)devinfo->mediactrip, (int8_t *)"0.0.0.0");
	}
	else{
		r_memcpy(&addr, &confinfo.media_addr, 4);
		r_strcpy((int8_t *)devinfo->mediactrip, (int8_t *)inet_ntoa(addr));
	}
	
	r_memcpy(&addr, &confinfo.manager_addr, 4);
	r_strcpy((int8_t *)devinfo->manageplatformip, (int8_t *)inet_ntoa(addr));
	
	if(0 != GetKernelVersion(gpio_fd, (char *)devinfo->kernelversion))
	{
		memset(devinfo->kernelversion, 0, 20);
		strcpy(devinfo->kernelversion,"unkown version");
	}
	return 1;
}

/*==============================================================================
    函数: <GetDiskInfo>
    功能: 获取磁盘信息
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:16:05 For Web
==============================================================================*/
int32_t	GetDiskInfo(DiskInfo *Diskinfo)
{
	if((NULL == gpser) || (NULL == gpser->pserinfo) || (Diskinfo == NULL))
	{
		return 0;
	}
	
	sys_info info = {0};
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&info, &gpser->pserinfo->SysInfo, sizeof(sys_info));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);

	Diskinfo->disksize    = info.DiskMaxSpace;
	Diskinfo->remainsize  = info.DiskAvailableSpace;

	return 1;
}


static int32_t Dm6467info_msg(int8_t *send_buf)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr EncVerReq 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30045");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"WebCtrl");

	EncVerReq = xmlNewNode(NULL, BAD_CAST "EncVerReq");
	xmlAddChild(body_node, EncVerReq);

	package_add_xml_leaf(EncVerReq, (const xmlChar *)"RoomID", (const int8_t *)"0");
	package_add_xml_leaf(EncVerReq, (const xmlChar *)"EncodeIndex", (const int8_t *)"3");
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t SetCtrlProto_msg(int8_t *send_buf, int8_t *proto)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr RemoteCtrlSet 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30042");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"WebCtrl");

	RemoteCtrlSet = xmlNewNode(NULL, BAD_CAST "RemoteCtrlSet");
	xmlAddChild(body_node, RemoteCtrlSet);

	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"RoomID", (const int8_t *)"0");
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"EncodeIndex", (const int8_t *)"3");
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"Pro", (const int8_t *)proto);
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t SetAudio_msg(int8_t *send_buf, int8_t *proto)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr SetRoomAudioReq 		= NULL;
	xmlNodePtr AudioInfo 			= NULL;
	xmlNodePtr RoomInfo  			= NULL;
	
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30006");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"WebCtrl");

	SetRoomAudioReq = xmlNewNode(NULL, BAD_CAST "SetRoomAudioReq");
	xmlAddChild(body_node, SetRoomAudioReq);

	RoomInfo = xmlNewNode(NULL, BAD_CAST "RoomInfo");
	xmlAddChild(SetRoomAudioReq, RoomInfo);


	package_add_xml_leaf(RoomInfo, (const xmlChar *)"RoomID", (const int8_t *)"0");

	AudioInfo = xmlNewNode(NULL, BAD_CAST "AudioInfo");
	xmlAddChild(RoomInfo, AudioInfo);

	package_add_xml_leaf(AudioInfo, (const xmlChar *)"InputMode", (const int8_t *)"0");
	package_add_xml_leaf(AudioInfo, (const xmlChar *)"InputIndex", (const int8_t *)proto);
	package_add_xml_leaf(AudioInfo, (const xmlChar *)"SampleRate", (const int8_t *)"0");
	package_add_xml_leaf(AudioInfo, (const xmlChar *)"Bitrate", (const int8_t *)"0");
	package_add_xml_leaf(AudioInfo, (const xmlChar *)"Lvolume", (const int8_t *)"0");
	package_add_xml_leaf(AudioInfo, (const xmlChar *)"Rvolume", (const int8_t *)"0");
	
	xmlChar *temp_xml_buf;
	
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t GetAudio_msg(int8_t *send_buf)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr GetRoomAudioReq 		= NULL;
	
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30046");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"WebCtrl");

	GetRoomAudioReq = xmlNewNode(NULL, BAD_CAST "GetRoomAudioReq");
	xmlAddChild(body_node, GetRoomAudioReq);

	package_add_xml_leaf(GetRoomAudioReq, (const xmlChar *)"RoomID", (const int8_t *)"0");
	xmlChar *temp_xml_buf;
	
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}



static int32_t DelCtrlProto_msg(int8_t *send_buf, int8_t *proto)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr RemoteCtrlSet 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30044");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"WebCtrl");

	RemoteCtrlSet = xmlNewNode(NULL, BAD_CAST "RemoteCtrlDel");
	xmlAddChild(body_node, RemoteCtrlSet);

	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"RoomID", (const int8_t *)"0");
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"EncodeIndex", (const int8_t *)"3");
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"Pro", (const int8_t *)proto);
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t UpgradeCtrlProto_msg(int8_t *send_buf, int8_t *proto, int32_t len)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr RemoteCtrlSet 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30043");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"WebCtrl");

	RemoteCtrlSet = xmlNewNode(NULL, BAD_CAST "UploadRemoteCtrlPro");
	xmlAddChild(body_node, RemoteCtrlSet);

	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"RoomID", (const int8_t *)"0");
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"EncodeIndex", (const int8_t *)"3");
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"ProName", (const int8_t *)proto);

	int8_t buffer[10] = {0};
	sprintf((char *)buffer, "%d",len);
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"Length", (const int8_t *)buffer);
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t UpgradeLogoProto_msg(int8_t *send_buf, LogoConfig *config, int32_t len)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr RemoteCtrlSet 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30016");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"WebCtrl");

	RemoteCtrlSet = xmlNewNode(NULL, BAD_CAST "UploadLogReq");
	xmlAddChild(body_node, RemoteCtrlSet);

	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"RoomID", (const int8_t *)"0");

	int8_t buffer[10] = {0};
	sprintf((char *)buffer, "%d",config->input);

	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"EncodeIndex", (const int8_t *)buffer);

	sprintf((char *)buffer, "%d",config->LogoPositon);
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"Position", (const int8_t *)buffer);

	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"Format", (const int8_t *)"png");
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"Transparency", (const int8_t *)"50");

	sprintf((char *)buffer, "%d",len);
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"LogoLength", (const int8_t *)buffer);

	if(config->LogoPositon <= 0)
	{
		package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"Enable", (const int8_t *)"0");
	}
	else
	{
		package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"Enable", (const int8_t *)"1");
	}


	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t Analyze_SetCtrlProto_msg(int8_t *recv_buf)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode    = NULL;
	char *pretcode = NULL;

	if(recv_buf == NULL)
	{
		printf ("Analyze_SetCtrlProto: pinfo is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_SetCtrlProto: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_SetCtrlProto: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		printf(  "Analyze_SetCtrlProto: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		printf(  "Analyze_SetCtrlProto: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		printf( "Analyze_SetCtrlProto: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		printf( "Analyze_SetCtrlProto: not found pretcode");
		goto EXIT;
	}

	int32_t irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		printf( "Analyze_SetCtrlProto: ReturnCode [%d]",irecode);
		goto EXIT;
	}
	
	
	return_code = 1;
	
EXIT:


	if(NULL != pretcode)
	{
		xmlFree(pretcode);
	}

	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}

static int32_t Analyze_AddTextProto_msg(int8_t *recv_buf)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode    = NULL;
	char *pretcode = NULL;

	if(recv_buf == NULL)
	{
		printf ("Analyze_SetCtrlProto: pinfo is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_SetCtrlProto: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_SetCtrlProto: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		printf(  "Analyze_SetCtrlProto: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		printf(  "Analyze_SetCtrlProto: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		printf( "Analyze_SetCtrlProto: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		printf( "Analyze_SetCtrlProto: not found pretcode");
		goto EXIT;
	}

	int32_t irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		printf( "Analyze_SetCtrlProto: ReturnCode [%d]",irecode);
		goto EXIT;
	}
	
	
	return_code = 1;
	
EXIT:


	if(NULL != pretcode)
	{
		xmlFree(pretcode);
	}

	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}


static int32_t Analyze_DelCtrlProto_msg(int8_t *recv_buf)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode    = NULL;
	char *pretcode = NULL;

	if(recv_buf == NULL)
	{
		printf ("Analyze_DelCtrlProto_msg: pinfo is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_DelCtrlProto_msg: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_DelCtrlProto_msg: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		printf(  "Analyze_DelCtrlProto_msg: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		printf(  "Analyze_DelCtrlProto_msg: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		printf( "Analyze_DelCtrlProto_msg: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		printf( "Analyze_DelCtrlProto_msg: not found pretcode");
		goto EXIT;
	}

	int32_t irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		printf( "Analyze_DelCtrlProto_msg: ReturnCode [%d]",irecode);
		goto EXIT;
	}
	
	
	return_code = 1;
	
EXIT:


	if(NULL != pretcode)
	{
		xmlFree(pretcode);
	}

	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}


static int32_t Analyze_UpgradeCtrlProto_msg(int8_t *recv_buf)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode    = NULL;
	char *pretcode = NULL;

	if(recv_buf == NULL)
	{
		printf ("Analyze_DelCtrlProto_msg: pinfo is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_DelCtrlProto_msg: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_DelCtrlProto_msg: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		printf(  "Analyze_DelCtrlProto_msg: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		printf(  "Analyze_DelCtrlProto_msg: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		printf( "Analyze_DelCtrlProto_msg: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		printf( "Analyze_DelCtrlProto_msg: not found pretcode");
		goto EXIT;
	}

	int32_t irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		printf( "Analyze_DelCtrlProto_msg: ReturnCode [%d]",irecode);
		goto EXIT;
	}
	
	
	return_code = 1;
	
EXIT:


	if(NULL != pretcode)
	{
		xmlFree(pretcode);
	}

	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}


static int32_t Analyze_UpgradeLogoProto_msg(int8_t *recv_buf)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode    = NULL;
	char *pretcode = NULL;

	if(recv_buf == NULL)
	{
		printf ("Analyze_UpgradeLogoProto_msg: pinfo is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_UpgradeLogoProto_msg: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_UpgradeLogoProto_msg: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		printf(  "Analyze_UpgradeLogoProto_msg: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		printf(  "Analyze_UpgradeLogoProto_msg: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		printf( "Analyze_UpgradeLogoProto_msg: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		printf( "Analyze_UpgradeLogoProto_msg: not found pretcode");
		goto EXIT;
	}

	int32_t irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		printf( "Analyze_UpgradeLogoProto_msg: ReturnCode [%d]",irecode);
		goto EXIT;
	}
	
	
	return_code = 1;
	
EXIT:


	if(NULL != pretcode)
	{
		xmlFree(pretcode);
	}

	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}


static int32_t SetCtrlProto(int8_t * proto)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t sockid = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	sockid = connectToServer();

	if(sockid <= 0)
	{
		return -1;
	}

	len = SetCtrlProto_msg(buffer+sizeof(MsgHeader), proto);
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =     r_htons(2012);

	
	printf("%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}


	int select_ret = 0;

	select_ret = select_socket(sockid, WEBSELECTTIMEOUT);
	if (select_ret == 0) {
		printf("select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		printf("select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}

	memset(buffer, 0x0, 2048);
	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{
		close(sockid);
		printf("recv MsgHeader is fail\n");
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		printf("tcp_recv_longdata is fail %d\n",xmlen);
		close(sockid);
		return -1;
	}

	printf("----->ret %s\n",buffer);

	if(1 != Analyze_SetCtrlProto_msg(buffer))
	{
		close(sockid);
		return -1;
	}

	close(sockid);
	return 1;

}

int code_convert(char *from_charset, char *to_charset, char *inbuf, int inlen, char *outbuf, int outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;
	cd = iconv_open(to_charset, from_charset);
	printf("to_charset = %s, from_charset = %s \n", to_charset, from_charset);

	if((iconv_t) - 1 == cd) {
		printf("open the char set failed,errno = %d,strerror(errno) = %s \n", errno, strerror(errno));
		return -1;
	}

	memset(outbuf, 0, outlen);

	if(iconv(cd, pin, (size_t *)&inlen, pout, (size_t *)&outlen) == (size_t) - 1) {
		printf("conver char set failed,errno = %d,strerror(errno) = %s \n", errno, strerror(errno));
		return -1;
	}

	iconv_close(cd);
	return 0;
}


static int32_t DelCtrlProto(int8_t * proto)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t sockid = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	sockid = connectToServer();

	if(sockid <= 0)
	{
		return -1;
	}

	len = DelCtrlProto_msg(buffer+sizeof(MsgHeader), proto);
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =     r_htons(2012);

	
	printf("%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}

	int select_ret = 0;

	select_ret = select_socket(sockid, WEBSELECTTIMEOUT);
	if (select_ret == 0) {
		printf("select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		printf("select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}


	memset(buffer, 0x0, 2048);
	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{
		close(sockid);
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		close(sockid);
		return -1;
	}

	printf("%s\n",buffer);

	if(1 != Analyze_DelCtrlProto_msg(buffer))
	{
		close(sockid);
		return -1;
	}

	close(sockid);
	return 1;

}

static int32_t SetAudioProto(int8_t * proto)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t sockid = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	sockid = connectToServer();

	if(sockid <= 0)
	{
		return -1;
	}

	len = SetAudio_msg(buffer+sizeof(MsgHeader), proto);
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =     r_htons(2012);

	
	printf("%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}

	int select_ret = 0;

	select_ret = select_socket(sockid, WEBSELECTTIMEOUT);
	if (select_ret == 0) {
		printf("select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		printf("select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}


	memset(buffer, 0x0, 2048);
	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{
		close(sockid);
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		close(sockid);
		return -1;
	}

	printf("%s\n",buffer);

	close(sockid);
	return 1;

}


static int32_t UpgradeCtrlProto(int8_t * proto)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t filelen = 0;
	int32_t sockid = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	FILE *file = NULL;
	
	file = fopen((char *)proto,"r");
	if(file == NULL)
	{
		return -1;
	}

	fseek(file,0L,SEEK_END);
	filelen = ftell(file);
	fseek(file,0L,SEEK_SET);
	
	if(filelen >= 1024*1024 || filelen <= 0)
	{
		fclose(file);
		return -1;
	}
	sockid = connectToServer();

	if(sockid <= 0)
	{
		fclose(file);
		return -1;
	}
	int8_t *protoname = basename(proto);

	len = UpgradeCtrlProto_msg(buffer + sizeof(MsgHeader), protoname, filelen);
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =     r_htons(2012);

	
	printf("%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		fclose(file);
		close(sockid);
		return -1;
	}
	memset(buffer ,0x0, 2048);
	while(1)
	{
		int n = 0;
		n = fread(buffer, 1, 1024 , file);
		if(n <= 0)
			break;
		tcp_send_longdata(sockid,  (int8_t *)buffer, n);

		
		printf("--->%s %d\n",buffer,n);
	}
	fclose(file);

	int select_ret = 0;

	select_ret = select_socket(sockid, WEBSELECTTIMEOUT*10);
	if (select_ret == 0) {
		printf("select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		printf("select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}


	memset(buffer, 0x0, 2048);
	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{
		
		close(sockid);
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		close(sockid);
		return -1;
	}

	printf("%s\n",buffer);

	if(1 != Analyze_UpgradeCtrlProto_msg(buffer))
	{
		close(sockid);
		return -1;
	}
	close(sockid);
	return 1;

}

#if 1
static int32_t UpgradeLogo(LogoConfig *config)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t filelen = 0;
	int32_t sockid = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	FILE *file = NULL;

#if 0
	if(config->input == 1)
	{
		system("cp /tmp/logo.png /usr/local/reach/dvr_rdk/ti816x/logo_0.png");
	}
	else if(config->input == 2)
	{
		system("cp /tmp/logo.png /usr/local/reach/dvr_rdk/ti816x/logo_1.png");
	}
#endif

	//需上传logo
	if(strlen(config->logoname) > 0)
	{
	
		file = fopen(config->logoname,"r");
		if(file == NULL)
		{
			return -1;
		}

		fseek(file,0L,SEEK_END);
		filelen = ftell(file);
		fseek(file,0L,SEEK_SET);
		
		if(filelen >= 1024*1024*2)
		{
			fclose(file);
			return -1;
		}
	}

	//设置logo
	else
	{
		filelen = 0;
	}


	sockid = connectToServer();

	if(sockid <= 0)
	{
		printf("connectToServer is fail!! %d\n",sockid);
		if(file != NULL)
			fclose(file);
		return -1;
	}


	len = UpgradeLogoProto_msg(buffer + sizeof(MsgHeader),config, filelen);
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =     r_htons(2012);

	
//	printf("%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		if(file != NULL)
			fclose(file);
		close(sockid);
		return -1;
	}

	
	memset(buffer ,0x0, 2048);

	if(strlen(config->logoname) > 0)
	{
		while(1)
		{
			int n = 0;
			n = fread(buffer, 1, 1024 , file);
			if(n <= 0)
				break;
			tcp_send_longdata(sockid,  (int8_t *)buffer, n);

			
	//		printf("--->%s %d\n",buffer,n);
		}

		if(file != NULL)
		fclose(file);
	}

	int select_ret = 0;

	select_ret = select_socket(sockid, WEBSELECTTIMEOUT*10);
	if (select_ret == 0) {
		printf("select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		printf("select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}

	
	memset(buffer, 0x0, 2048);

	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{

		close(sockid);
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		close(sockid);
		return -1;
	}

//	printf("%s\n",buffer);

	if(1 != Analyze_UpgradeLogoProto_msg(buffer))
	{
		close(sockid);
		return -1;
	}

	close(sockid);
	return 1;

}



#endif

static int32_t Analyze_Dm6467info_msg(int8_t *recv_buf, Dm6467Info* pinfo)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode   = NULL;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr EncVerReq = NULL;
	xmlNodePtr SysVer = NULL;
	xmlNodePtr KernelVer = NULL;
	xmlNodePtr FpgaVer = NULL;
	char *pSysVer = NULL;
	char *pretcode = NULL;
	char *pKernelVer = NULL;
	char *pFpgaVer = NULL;


	if(pinfo == NULL)
	{
		printf ("Analyze_State: pinfo is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_Dm6467info: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_Dm6467info: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		printf(  "Analyze_Dm6467info: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		printf(  "Analyze_Dm6467info: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		printf( "Analyze_Dm6467info: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		printf( "Analyze_Dm6467info: not found pretcode");
		goto EXIT;
	}

	int32_t irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		printf( "Analyze_Dm6467info: ReturnCode [%d]",irecode);
		goto EXIT;
	}
	
	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	if(msgbody == NULL)
	{
		printf( "Analyze_Dm6467info: MsgBody fail");
		goto EXIT;
	}

	EncVerReq   = get_children_node(msgbody, BAD_CAST "EncVerReq");
	if(EncVerReq == NULL)
	{
		printf( "Analyze_Dm6467info: EncVerReq fail");
		goto EXIT;
	}
	
	SysVer   = get_children_node(EncVerReq, BAD_CAST "SysVer");
	if(SysVer == NULL)
	{
		printf(  "Analyze_Dm6467info: not found SysVer");
		goto EXIT;
	}

	pSysVer  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, SysVer->xmlChildrenNode, 1);	
	if(pSysVer == NULL)
	{
		printf(  "Analyze_Dm6467info: not found pSysVer");
		goto EXIT;
	}

	KernelVer   = get_children_node(EncVerReq, BAD_CAST "KernelVer");
	if(KernelVer == NULL)
	{
		printf(  "Analyze_Dm6467info: not found KernelVer");
		goto EXIT;
	}

	pKernelVer  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, KernelVer->xmlChildrenNode, 1);	
	if(pKernelVer == NULL)
	{
		printf(  "Analyze_Dm6467info: not found usrname");
		goto EXIT;
	}


	FpgaVer   = get_children_node(EncVerReq, BAD_CAST "FpgaVer");
	if(FpgaVer == NULL)
	{
		printf(  "Analyze_Dm6467info: not found FpgaVer");
		goto EXIT;
	}

	pFpgaVer  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, FpgaVer->xmlChildrenNode, 1);	
	if(pFpgaVer == NULL)
	{
		printf(  "Analyze_Dm6467info: not found FpgaVer");
		goto EXIT;
	}

	memset((char*)pinfo,0x0,sizeof(Dm6467Info));
	strncpy(pinfo->sysversion,   pSysVer,63);
	strncpy(pinfo->uImageversion, pKernelVer,63);
	strncpy(pinfo->fpgaversion,  pFpgaVer,63);
	
	return_code = 1;
	
EXIT:
	if(NULL != pSysVer)
	{
		xmlFree(pSysVer);
	}

	if(NULL != pretcode)
	{
		xmlFree(pretcode);
	}

	if(NULL != pKernelVer)
	{
		xmlFree(pKernelVer);
	}

	if(NULL != pFpgaVer)
	{
		xmlFree(pFpgaVer);
	}
	
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}

int32_t	GetDm6467Info(Dm6467Info* info)
{
	int8_t buffer[2048] = {0};
	int32_t len = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	if((info == NULL))
	{
		return -1;
	}
	
	int32_t sockid = connectToServer();

	if(sockid <= 0)
	{
		return -1;
	}
	
	len	= Dm6467info_msg(buffer+ sizeof(MsgHeader));
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =     r_htons(2012);

	printf("%s\n",buffer + sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}

	int select_ret = 0;

	select_ret = select_socket(sockid, WEBSELECTTIMEOUT);
	if (select_ret == 0) {
		printf("select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		printf("select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}


	memset(buffer, 0x0, 2048);
	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{
		close(sockid);
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		close(sockid);
		return -1;
	}

	printf("%s\n",buffer);

	if(1 != Analyze_Dm6467info_msg(buffer, info))
	{
		close(sockid);
		return -1;
	}

	close(sockid);
	return 1;
}

static int32_t GetCtrlProto_msg(int8_t *send_buf)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr RemoteCtrlSet 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30041");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"WebCtrl");

	RemoteCtrlSet = xmlNewNode(NULL, BAD_CAST "RemoteCtrlGet");
	xmlAddChild(body_node, RemoteCtrlSet);

	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"RoomID", (const int8_t *)"0");
	package_add_xml_leaf(RemoteCtrlSet, (const xmlChar *)"EncodeIndex", (const int8_t *)"3");
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t Analyze_GetProto_msg(int8_t *recv_buf, ProtoList* pinfo)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode   = NULL;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr CurPro = NULL;
	xmlNodePtr ProList = NULL;
	xmlNodePtr proto   = NULL;
	xmlNodePtr			RemoteCtrlGet = NULL;
	char *pCurPro = NULL;
	char *pretcode = NULL;
	char *pproto = NULL;


	if(pinfo == NULL)
	{
		printf ("Analyze_State: pinfo is NULL");
		return -1;
	}

	memset(pinfo, 0x0, sizeof(ProtoList));
	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_CtlProto: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_CtlProto: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		printf(  "Analyze_CtlProto: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		printf(  "Analyze_CtlProto: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		printf( "Analyze_CtlProto: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		printf( "Analyze_CtlProto: not found pretcode");
		goto EXIT;
	}

	int32_t irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		printf( "Analyze_CtlProto: ReturnCode [%d]",irecode);
		goto EXIT;
	}
	
	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	if(msgbody == NULL)
	{
		printf( "Analyze_CtlProto: MsgBody fail");
		goto EXIT;
	}


	RemoteCtrlGet   = get_children_node(msgbody, BAD_CAST "RemoteCtrlGet");
	if(RemoteCtrlGet == NULL)
	{
		printf( "Analyze_CtlProto: RemoteCtrlGet fail");
		goto EXIT;
	}
	
	CurPro   = get_children_node(RemoteCtrlGet, BAD_CAST "CurPro");
	if(CurPro == NULL)
	{
		printf(  "Analyze_CtlProto: not found CurPro");
		goto EXIT;
	}

	pCurPro  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, CurPro->xmlChildrenNode, 1);	
	if(pCurPro == NULL)
	{
		printf(  "Analyze_CtlProto: not found pCurPro");
		goto EXIT;
	}

	ProList   = get_children_node(RemoteCtrlGet, BAD_CAST "ProList");
	if(ProList == NULL)
	{
		printf(  "Analyze_CtlProto: not found ProList");
		goto EXIT;
	}

	strcpy(pinfo->curproto,pCurPro);
	printf("CurProt %s\n",pinfo->curproto);

	proto = ProList->children;
		
	while((proto = find_next_node(proto, BAD_CAST "Pro")))
	{
		pproto  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, proto->xmlChildrenNode, 1);	
		if(pproto == NULL)
		{
			printf(  "Analyze_CtlProto: not found pproto");
			break;
		}

		strcpy(pinfo->protolist[pinfo->num], pproto);

		printf("protolist : %s\n",pinfo->protolist[pinfo->num]);
		pinfo->num ++;
	
		xmlFree(pproto);
		pproto = NULL;
	}


	return_code = 1;
	
EXIT:

	if(NULL != pCurPro)
	{
		xmlFree(pCurPro);
	}

	if(NULL != pretcode)
	{
		xmlFree(pretcode);
	}

	if(NULL != pproto)
	{
		xmlFree(pproto);
	}
	
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}

static int32_t Analyze_AudioProto_msg(int8_t *recv_buf, int8_t* proto)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode   = NULL;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr AudioInfo = NULL;
	xmlNodePtr InputIndex = NULL;
	xmlNodePtr GetRoomAudioReq = NULL;
	char *pInputIndex = NULL;
	char *pretcode = NULL;

	if(proto == NULL)
	{
		printf ("Analyze_AudioProto_msg: pinfo is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_AudioProto_msg: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		printf( "Analyze_AudioProto_msg: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		printf(  "Analyze_AudioProto_msg: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		printf(  "Analyze_AudioProto_msg: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		printf( "Analyze_AudioProto_msg: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		printf( "Analyze_AudioProto_msg: not found pretcode");
		goto EXIT;
	}

	int32_t irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		printf( "Analyze_AudioProto_msg: ReturnCode [%d]",irecode);
		goto EXIT;
	}
	
	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	if(msgbody == NULL)
	{
		printf( "Analyze_AudioProto_msg: MsgBody fail");
		goto EXIT;
	}


	GetRoomAudioReq   = get_children_node(msgbody, BAD_CAST "GetRoomAudioReq");
	if(GetRoomAudioReq == NULL)
	{
		printf( "Analyze_AudioProto_msg: GetRoomAudioReq fail");
		goto EXIT;
	}
	
	AudioInfo   = get_children_node(GetRoomAudioReq, BAD_CAST "AudioInfo");
	if(AudioInfo == NULL)
	{
		printf(  "Analyze_AudioProto_msg: not found AudioInfo");
		goto EXIT;
	}

	InputIndex   = get_children_node(AudioInfo, BAD_CAST "InputIndex");
	if(InputIndex == NULL)
	{
		printf(  "Analyze_AudioProto_msg: not found InputIndex");
		goto EXIT;
	}


	pInputIndex  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, InputIndex->xmlChildrenNode, 1);	
	if(pInputIndex == NULL)
	{
		printf(  "Analyze_AudioProto_msg: not found pInputIndex");
		goto EXIT;
	}


	strcpy((char *)proto,pInputIndex);
	printf("CurProt %s\n",proto);

	return_code = 1;
	
EXIT:


	if(NULL != pretcode)
	{
		xmlFree(pretcode);
	}

	if(NULL != pInputIndex)
	{
		xmlFree(pInputIndex);
	}
	
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}


static int32_t	GetCtrlProto(ProtoList* proto)
{
	int8_t buffer[2048] = {0};
	int32_t len = 0;

	MsgHeader *pmsg = (MsgHeader *)buffer;
	if((proto == NULL)) 
	{
		return -1;
	}
	
	int32_t sockid = connectToServer();

	if(sockid <= 0)
	{
		return -1;
	}


	len	= GetCtrlProto_msg(buffer + sizeof(MsgHeader));

	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =     r_htons(2012);

	printf("%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}

	int select_ret = 0;

	select_ret = select_socket(sockid, WEBSELECTTIMEOUT);
	if (select_ret == 0) {
		printf("select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		printf("select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}


	memset(buffer, 0x0, 2048);
	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{
		close(sockid);
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		close(sockid);
		return -1;
	}

	printf("%s\n",buffer);

	if(1 != Analyze_GetProto_msg(buffer, proto))
	{
		close(sockid);
		return -1;
	}

	close(sockid);
	return 1;
}


static int32_t	GetAudioProto(int8_t * proto)
{
	int8_t buffer[2048] = {0};
	int32_t len = 0;

	MsgHeader *pmsg = (MsgHeader *)buffer;
	if((proto == NULL)) 
	{
		return -1;
	}
	
	int32_t sockid = connectToServer();

	if(sockid <= 0)
	{
		return -1;
	}


	len	= GetAudio_msg(buffer + sizeof(MsgHeader));

	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =     r_htons(2012);

	printf("%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}

	int select_ret = 0;

	select_ret = select_socket(sockid, WEBSELECTTIMEOUT);
	if (select_ret == 0) {
		printf("select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		printf("select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}


	memset(buffer, 0x0, 2048);
	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{
		close(sockid);
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		close(sockid);
		return -1;
	}

	printf("%s\n",buffer);

	if(1 != Analyze_AudioProto_msg(buffer, proto))
	{
		close(sockid);
		return -1;
	}

	close(sockid);
	return 1;
}

int32_t GetThrFtpinfo(ftp_info *ftpinfo, server_set *pser)
{
	int32_t ret = -1;
	if((NULL == pser)|| (ftpinfo == NULL)){
		return ret;
	}		
	r_pthread_mutex_lock(&pser->pserinfo->info_m);
	r_memcpy(ftpinfo, &pser->pserinfo->FtpInfo, sizeof(ftp_info));
	r_pthread_mutex_unlock(&pser->pserinfo->info_m);

	return 1;
}

int32_t SetThrFtpinfo(ftp_info *ftpinfo, server_set *pser)
{
	int32_t ret = -1;

	ftp_info new_info = {0};
	ftp_info old_info = {0};
	
	if((NULL == pser)|| (ftpinfo == NULL)){
		return ret;
	}

	r_pthread_mutex_lock(&pser->pserinfo->info_m);
	r_memcpy(&old_info, &pser->pserinfo->FtpInfo, sizeof(ftp_info));
	r_memcpy(&new_info, &pser->pserinfo->FtpInfo, sizeof(ftp_info));
	new_info.Mode = ftpinfo->Mode;
	new_info.THRFTPPort = ftpinfo->THRFTPPort;
	r_memset(new_info.THRFTPAddr, 0, 24);
	r_memcpy(new_info.THRFTPAddr, ftpinfo->THRFTPAddr, 24);
	r_memset(new_info.THRFTPUserName, 0, FTP_MAX_USERNAME_LENGTH);
	r_memcpy(new_info.THRFTPUserName, ftpinfo->THRFTPUserName, FTP_MAX_USERNAME_LENGTH);
	r_memset(new_info.THRFTPPassword, 0, FTP_MAX_PASSWD_LENGTH);
	r_memcpy(new_info.THRFTPPassword, ftpinfo->THRFTPPassword, FTP_MAX_PASSWD_LENGTH);
	r_memset(new_info.THRFTPPath, 0, FTP_MAX_FTPPATH_LENGTH);
	r_memcpy(new_info.THRFTPPath, ftpinfo->THRFTPPath, FTP_MAX_FTPPATH_LENGTH);
	modify_ftp_info_only((const int8_t *)CONFIG_TABLE_FILE, &old_info, &new_info);
	r_memcpy(&pser->pserinfo->FtpInfo, &new_info, sizeof(ftp_info));
	r_pthread_mutex_unlock(&pser->pserinfo->info_m);

	
	zlog_debug(z,"MSG_SET_THR_FTPINFO IS OVER!+++++++++++++++++++++++<GOD +1>\n");
	return 1;
}



/*==============================================================================
    函数: <SetLanIpconfig>
    功能: 设置局域网ip
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:16:19 For Web
==============================================================================*/
int32_t SetLanIpconfig(IpConfig *ipconfig)
{
	int32_t ret = -1;
	if((NULL == gpser)||(NULL == gpser->pserinfo)|| (ipconfig == NULL))
	{
		return ret;
	}

	/* TO DO : 目前自动获取ip需重启生效*/
	if((ipconfig->iptype != 0) && (ipconfig->iptype != 1))
	{
		return ret;
	}
	
	if(ipconfig->iptype == 0)
	{
		server_info newinfo = {{0}};
		server_info oldinfo = {{0}};
		
		r_pthread_mutex_lock(&gpser->pserinfo->info_m);
		r_memcpy(&oldinfo, &gpser->pserinfo->ServerInfo, sizeof(server_info));
		r_pthread_mutex_unlock(&gpser->pserinfo->info_m);


		r_memcpy(&newinfo, &oldinfo, sizeof(server_info));
		newinfo.LanAddr 	= inet_addr(ipconfig->ipaddr);
		newinfo.LanNetmask  = inet_addr(ipconfig->netmask);
		newinfo.LanGateWay  = inet_addr(ipconfig->gateway);
		ret = modify_server_info_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);
		if(0 == ret)
		{
			r_pthread_mutex_lock(&gpser->pserinfo->info_m);
			r_memcpy(&gpser->pserinfo->ServerInfo, &newinfo, sizeof(server_info));
			r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
		}

	}

	config_info oldinfo = {0};
	config_info newinfo = {0};

	//获取实时数据
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&oldinfo, &gpser->pserinfo->ConfigInfo, sizeof(config_info));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);


	//修改XML
	r_memcpy(&newinfo, &oldinfo, sizeof(config_info));
	newinfo.Laniptype = ipconfig->iptype;
	ret = modify_config_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);
	if(0 == ret)
	{
		//修改实时数据
		r_pthread_mutex_lock(&gpser->pserinfo->info_m);	
		r_memcpy(&gpser->pserinfo->ConfigInfo, &newinfo, sizeof(config_info));
		r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
	}
	
	FILE *file = fopen("/etc/rc3.d/S90ReachEth0Config","w");
	if(file != NULL)
	{
		if(newinfo.Laniptype == 0)
		{
			SETETH0STATIC(file,ipconfig->ipaddr,ipconfig->gateway, ipconfig->netmask);
		}
		else
		{
			SETETH0DHCP(file);
		}
		fclose(file);
	}
	chown("/etc/rc3.d/S90ReachConfig", 0, 0);
	return 1;
}

/*==============================================================================
    函数: <SetWanIpconfig>
    功能: 设置以太网IP
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:16:27 For Web
==============================================================================*/
int32_t SetWanIpconfig(IpConfig *ipconfig)
{
	int32_t ret = -1;
	if((NULL == gpser)||(NULL == gpser->pserinfo)|| (ipconfig == NULL))
	{
		return ret;
	}

	/* TO DO : 目前自动获取ip需重启生效*/
	if((ipconfig->iptype != 0) && (ipconfig->iptype != 1))
	{
		return ret;
	}
	
	if(ipconfig->iptype == 0)
	{
		server_info newinfo = {{0}};
		server_info oldinfo = {{0}};
		
		r_pthread_mutex_lock(&gpser->pserinfo->info_m);
		r_memcpy(&oldinfo, &gpser->pserinfo->ServerInfo, sizeof(server_info));
		r_pthread_mutex_unlock(&gpser->pserinfo->info_m);

		r_memcpy(&newinfo, &oldinfo, sizeof(server_info));
		newinfo.WanAddr 	= inet_addr(ipconfig->ipaddr);
		newinfo.WanNetmask  = inet_addr(ipconfig->netmask);
		newinfo.WanGateWay  = inet_addr(ipconfig->gateway);
		ret = modify_server_info_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);
		if(0 == ret)
		{
			r_pthread_mutex_lock(&gpser->pserinfo->info_m);
			r_memcpy(&gpser->pserinfo->ServerInfo, &newinfo, sizeof(server_info));
			r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
		}

	}

	config_info oldinfo = {0};
	config_info newinfo = {0};

	//获取实时数据
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&oldinfo, &gpser->pserinfo->ConfigInfo, sizeof(config_info));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);


	//修改XML
	r_memcpy(&newinfo, &oldinfo, sizeof(config_info));
	newinfo.Waniptype = ipconfig->iptype;
	ret = modify_config_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);
	if(0 == ret)
	{
		//修改实时数据
		r_pthread_mutex_lock(&gpser->pserinfo->info_m);	
		r_memcpy(&gpser->pserinfo->ConfigInfo, &newinfo, sizeof(config_info));
		r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
	}


	FILE *file = fopen("/etc/rc3.d/S91ReachEth1Config","w");
	if(file != NULL)
	{
		if(newinfo.Laniptype == 0)
		{
			SETETH1STATIC(file,ipconfig->ipaddr, ipconfig->netmask);
		}
		else
		{
			SETETH1STATIC(file,ipconfig->ipaddr, ipconfig->netmask);
		}
		fclose(file);
	}


	return 1;
}


/*==============================================================================
    函数: <GetLanIpconfig>
    功能: 获取局域网IP
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:16:36 For Web
==============================================================================*/
int32_t GetLanIpconfig(IpConfig *ipconfig, server_set *pser)
{
	int32_t ret = -1;
	struct in_addr addr;
	
	if((NULL == pser)|| (ipconfig == NULL))
	{
		return ret;
	}
	
	server_info info = {{0}};
	
	r_pthread_mutex_lock(&pser->pserinfo->info_m);
	r_memcpy(&info, &pser->pserinfo->ServerInfo, sizeof(server_info));
	r_pthread_mutex_unlock(&pser->pserinfo->info_m);

	r_memcpy(&addr, &info.LanAddr, 4);
	r_memset(ipconfig->ipaddr, 0, 20);
	r_strcpy((int8_t *)ipconfig->ipaddr, (const int8_t *)inet_ntoa(addr));
	fprintf(stderr, "%s", ipconfig->ipaddr);

	r_memcpy(&addr, &info.LanNetmask, 4);
	r_memset(ipconfig->netmask, 0, 20);
	r_strcpy((int8_t *)ipconfig->netmask, (const int8_t *)inet_ntoa(addr));
	fprintf(stderr, "%s", ipconfig->netmask);

	r_memcpy(&addr, &info.LanGateWay, 4);
	r_memset(ipconfig->gateway, 0, 20);
	r_strcpy((int8_t *)ipconfig->gateway, (const int8_t *)inet_ntoa(addr));
	fprintf(stderr, "%s", ipconfig->gateway);

	config_info confignfo = {0};
		
	r_pthread_mutex_lock(&pser->pserinfo->info_m);
	r_memcpy(&confignfo, &pser->pserinfo->ConfigInfo, sizeof(config_info));
	r_pthread_mutex_unlock(&pser->pserinfo->info_m);
	ipconfig->iptype = confignfo.Laniptype;
	

	return 1;
}

/*==============================================================================
    函数: <GetWanIpconfig>
    功能: 获取以太网IP
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:16:44 For Web
==============================================================================*/
int32_t GetWanIpconfig(IpConfig *ipconfig, server_set *pser)
{
	int32_t ret = -1;
	struct in_addr addr;
	
	if((NULL == pser)|| (ipconfig == NULL))
	{
		return ret;
	}

	server_info info = {{0}};
	
	r_pthread_mutex_lock(&pser->pserinfo->info_m);
	r_memcpy(&info, &pser->pserinfo->ServerInfo, sizeof(server_info));
	r_pthread_mutex_unlock(&pser->pserinfo->info_m);

	r_memcpy(&addr, &info.WanAddr, 4);
	r_memset(ipconfig->ipaddr, 0, 20);
	r_strcpy((int8_t *)ipconfig->ipaddr, (const int8_t *)inet_ntoa(addr));
	fprintf(stderr, "%s", ipconfig->ipaddr);

	r_memcpy(&addr, &info.WanNetmask, 4);
	r_memset(ipconfig->netmask, 0, 20);
	r_strcpy((int8_t *)ipconfig->netmask, (const int8_t *)inet_ntoa(addr));
	fprintf(stderr, "%s", ipconfig->netmask);

	r_memcpy(&addr, &info.WanGateWay, 4);
	r_memset(ipconfig->gateway, 0, 20);
	r_strcpy((int8_t *)ipconfig->gateway, (const int8_t *)inet_ntoa(addr));
	fprintf(stderr, "%s", ipconfig->gateway);

	config_info confignfo = {0};
	
	r_pthread_mutex_lock(&pser->pserinfo->info_m);
	r_memcpy(&confignfo, &pser->pserinfo->ConfigInfo, sizeof(config_info));
	r_pthread_mutex_unlock(&pser->pserinfo->info_m);
	ipconfig->iptype = confignfo.Waniptype;
	
	return 1;
}


/*==============================================================================
    函数: <SetUserPassword>
    功能: 设置密码
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:16:54 For Web
==============================================================================*/
int32_t SetUserPassword(int8_t *password)
{
	int32_t ret = -1;
	if((NULL == gpser)||(NULL == gpser->pserinfo)|| (password == NULL))
	{
		return 0;
	}

	user_info oldinfo = {{0},{0}};
	user_info newinfo = {{0},{0}};

	//获取实时数据
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&oldinfo, &gpser->pserinfo->Authentication, sizeof(user_info));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);

	//修改XML
	r_memcpy(&newinfo, &oldinfo, sizeof(user_info));
	r_strcpy((int8_t *)newinfo.password, (int8_t *)password);

	ret = modify_usr_info_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);

	if(ret == 0)
	{
		//修改实时数据
		r_pthread_mutex_lock(&gpser->pserinfo->info_m);
//		r_memset(gpser->pserinfo->HBeatInfo.post_url, 0, HTTP_SERVER_URL_MAX_LEN);
		r_memcpy(&gpser->pserinfo->Authentication, &newinfo, sizeof(user_info));
		r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
	}
	else
	{
		zlog_debug(z,"there is error! \n");
	}
	return 1;
}

/*==============================================================================
    函数: <SetGuestPassword>
    功能: 设定Guest 密码
    参数: 
    返回值: 
    Created By zhengyb
==============================================================================*/

int32_t SetGuestPassword(int8_t *password)
{
	int32_t ret = -1;
	if((NULL == gpser)||(NULL == gpser->pserinfo)|| (password == NULL))
	{
		return 0;
	}

	user_info oldinfo = {{0},{0}};
	user_info newinfo = {{0},{0}};

	//获取实时数据
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&oldinfo, &gpser->pserinfo->Authentication, sizeof(user_info));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);

	//修改XML
	r_memcpy(&newinfo, &oldinfo, sizeof(user_info));
	r_strcpy((int8_t *)newinfo.guest_passwd, (int8_t *)password);

	ret = modify_usr_info_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);

	if(ret == 0)
	{
		//修改实时数据
		r_pthread_mutex_lock(&gpser->pserinfo->info_m);
//		r_memset(gpser->pserinfo->HBeatInfo.post_url, 0, HTTP_SERVER_URL_MAX_LEN);
		r_memcpy(&gpser->pserinfo->Authentication, &newinfo, sizeof(user_info));
		r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
	}
	return 1;
}

/*==============================================================================
    函数: <SetRoomName>
    功能: 设定教室名
    参数: 
    返回值: 
    Created By zhengyb
==============================================================================*/

int32_t SetRoomName(int8_t *name)
{
	int32_t ret = -1;
	if((NULL == gpser)||(NULL == gpser->pserinfo)|| (name == NULL))
	{
		return 0;
	}

	room_info oldinfo ;
	r_memset(&oldinfo , 0 ,sizeof(room_info));
	room_info newinfo ;
	r_memset(&newinfo , 0 ,sizeof(room_info));

	//获取实时数据
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&oldinfo, &(gpser->pserinfo->RoomInfo[0]), sizeof(room_info));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
	

	//修改XML
	r_memcpy(&newinfo, &oldinfo, sizeof(user_info));
	r_memset((int8_t *)(newinfo.RoomName), 0, ROOM_NAME_MAX_LENGTH);
	r_strcpy((int8_t *)(newinfo.RoomName),(int8_t *)name);
	zlog_debug(z,"MSG_SET_SYSTEM_NAME -----old_name :%s--new_name : %s---- len : %d\n",oldinfo.RoomName,newinfo.RoomName,sizeof(room_info));

	ret = modify_room_info_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);
//	ret = modify_usr_info_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);

	if(ret == 0)
	{
		//修改实时数据
		r_pthread_mutex_lock(&gpser->pserinfo->info_m);
//		r_memset(gpser->pserinfo->HBeatInfo.post_url, 0, HTTP_SERVER_URL_MAX_LEN);
		r_memcpy( &(gpser->pserinfo->RoomInfo[0]), &newinfo, sizeof(user_info));
		r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
	}
	return 1;
}


/*==============================================================================
    函数: <GetUserPassword>
    功能: 获取密码
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:17:01 For Web
==============================================================================*/
int32_t GetUserPassword(int8_t *password, server_set *pser)
{
	if((NULL == password)||(pser == NULL))
	{
		return 0;
	}

	r_pthread_mutex_lock(&pser->pserinfo->info_m);
	r_strcpy((int8_t *)password, (const int8_t *)&pser->pserinfo->Authentication.password);
	r_pthread_mutex_unlock(&pser->pserinfo->info_m);

	return 1;
}
/*==============================================================================
    函数: <GetGuestPassword>
    功能: 获取密码
    参数: 
    返回值: 
    Created By zhengyb
==============================================================================*/

int32_t GetGuestPassword(int8_t *password, server_set *pser)
{
	if((NULL == password)||(pser == NULL))
	{
		return 0;
	}

	r_pthread_mutex_lock(&pser->pserinfo->info_m);
	r_strcpy((int8_t *)password, (const int8_t *)&pser->pserinfo->Authentication.guest_passwd);
	r_pthread_mutex_unlock(&pser->pserinfo->info_m);

	return 1;
}
/*==============================================================================
    函数: <GetRoomName>
    功能: 获取密码
    参数: 
    返回值: 
    Created By zhengyb
==============================================================================*/

int32_t GetRoomName(int8_t *name, server_set *pser)
{
	if((NULL == name)||(pser == NULL))
	{
		return 0;
	}

	r_pthread_mutex_lock(&pser->pserinfo->info_m);
	r_strcpy((int8_t *)name, (const int8_t *)&pser->pserinfo->RoomInfo[0].RoomName);
	r_pthread_mutex_unlock(&pser->pserinfo->info_m);

	return 1;
}


/*==============================================================================
    函数: <SetLanguage>
    功能: 设置语言
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:17:06 For Web
==============================================================================*/
int32_t SetLanguage(int32_t language)
{
	int32_t ret = -1;
	if((NULL == gpser)||(NULL == gpser->pserinfo))
	{
		return ret;
	}
	
	config_info oldinfo = {0};
	config_info newinfo = {0};
	//获取实时数据
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&oldinfo, &gpser->pserinfo->ConfigInfo, sizeof(config_info));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
		
	//修改XML
	r_memcpy(&newinfo, &oldinfo, sizeof(config_info));
	newinfo.language = language;
	ret = modify_config_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);
	if(ret == 0)
	{
		//修改实时数据
		r_pthread_mutex_lock(&gpser->pserinfo->info_m);	
		r_memcpy(&gpser->pserinfo->ConfigInfo, &newinfo, sizeof(config_info));
		r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
	}
	else
	{
		zlog_debug(z,"there is error ! <%d>\n",ret);
	}
	
	return ret;
}

/*==============================================================================
    函数: <GetLanguage>
    功能: 获取语言
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:17:13 For Web
==============================================================================*/
int32_t GetLanguage(int32_t *language, server_set *pser)
{
	int32_t ret = -1;
	if((NULL == pser)||(NULL == language))
	{
		return ret;
	}
	
	config_info info = {0};
	
	r_pthread_mutex_lock(&pser->pserinfo->info_m);
	r_memcpy(&info, &pser->pserinfo->ConfigInfo, sizeof(config_info));
	r_pthread_mutex_unlock(&pser->pserinfo->info_m);
		
	*language = info.language;

	zlog_debug(z,"<GOD >  ------ Language : %d \n",info.language);

	return 0;
}

int32_t SetDevSerialNumber(int8_t *pNumber)
{
	int32_t ret =  -1;
	if((NULL == gpser)||(NULL == pNumber))
	{
		return ret;
	}
	
	server_info newinfo = {{0}};
	server_info oldinfo = {{0}};
	
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&oldinfo, &gpser->pserinfo->ServerInfo, sizeof(server_info));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);

	r_memcpy(&newinfo, &oldinfo, sizeof(server_info));
	r_strcpy((int8_t *)newinfo.ServerSeries, pNumber);
	ret = modify_server_info_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);
	if(0 == ret)
	{
		r_pthread_mutex_lock(&gpser->pserinfo->info_m);
		r_memcpy(&gpser->pserinfo->ServerInfo, &newinfo, sizeof(server_info));
		r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
	}
	return 1;
}

int32_t SetAddrBitsInfo(AddrBitConfig *addr_bit_info)//zyb
{	
	int32_t ret =  -1;
	if((NULL == gpser)||(NULL == addr_bit_info))
	{
		return ret;
	}
	
	camera_ctrl newinfo = {0};
	camera_ctrl oldinfo = {0};
	
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	r_memcpy(&oldinfo, &gpser->pserinfo->CamCtrlInfo, sizeof(camera_ctrl));
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);

	r_memcpy(&newinfo, &oldinfo, sizeof(camera_ctrl));
	
	newinfo.video0_addr = addr_bit_info->addr_1;
	newinfo.video1_addr = addr_bit_info->addr_2;
	newinfo.video2_addr = addr_bit_info->addr_3;
	
	ret = modify_camctrl_info_only((const int8_t *)CONFIG_TABLE_FILE, &oldinfo, &newinfo);
	if(0 == ret)
	{
		r_pthread_mutex_lock(&gpser->pserinfo->info_m);
		r_memcpy(&gpser->pserinfo->CamCtrlInfo, &newinfo, sizeof(camera_ctrl));
		r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
	}
	return 1;
}

int32_t GetAddrBitsInfo(AddrBitConfig *addr_bit_info)//zyb
{
	int32_t ret =  -1;
	if((NULL == gpser)||(NULL == addr_bit_info))
	{
		return ret;
	}
	
	r_pthread_mutex_lock(&gpser->pserinfo->info_m);
	addr_bit_info->addr_1 = gpser->pserinfo->CamCtrlInfo.video0_addr;
	addr_bit_info->addr_2 = gpser->pserinfo->CamCtrlInfo.video1_addr;
	addr_bit_info->addr_3 = gpser->pserinfo->CamCtrlInfo.video2_addr;
	r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
	
	return 1;
}

static void set_rtc_clock(int32_t nYear, int32_t nMonth, int32_t nDay, int32_t nHour, int32_t nMinute, int32_t nSecond)  //zl
{
	if(nSecond < 1) {
		nSecond  = 10;
	}

	int32_t fd = 0;
	struct rtc_time rtc_tm;
	fd = open("/dev/rtc0", O_RDWR);

	if(fd == -1) {
		fprintf(stderr, "Cannot open RTC device due to following reason.\n");
		perror("/dev/rtc0");
		fd = 0;
		return;
	}

	rtc_tm.tm_mday = nDay;
	rtc_tm.tm_mon = nMonth - 1;
	rtc_tm.tm_year = nYear - 1900;
	rtc_tm.tm_hour = nHour;
	rtc_tm.tm_min = nMinute;
	rtc_tm.tm_sec = nSecond;

	char setStr[256] = {0};
	sprintf((char *)setStr, "date %02d%02d%02d%02d%04d", nMonth, nDay, nHour, nMinute, nYear);
	//system(setStr);
	r_system((const int8_t *)setStr);

	zlog_debug(z,"%s\n", setStr);

	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);
	rtc_tm.tm_wday = p->tm_wday;
	int32_t retval = ioctl(fd, RTC_SET_TIME, &rtc_tm);

	if(retval == -1) {
		fprintf(stderr, "Cannot do ioctl() due to following reason.\n");
		perror("ioctl");
		r_close(fd);
		return;
	}

	r_close(fd);
}


static int setRtcTime(int32_t nYear, int32_t nMonth, int32_t nDay, int32_t nHour, int32_t nMinute, int32_t nSecond)  //zl
{
	if(nSecond < 1) {
		nSecond  = 10;
	}

	set_rtc_clock(nYear, nMonth, nDay, nHour,
	              nMinute, nSecond);
	return 0;
}

static void getSysTime(SysTimeConfig *dtinfo)  //ZL
{
	long ts;
	struct tm *ptm = NULL;

	ts = time(NULL);
	ptm = localtime((const time_t *)&ts);
	dtinfo->year = ptm->tm_year + 1900;
	dtinfo->month = ptm->tm_mon + 1;
	dtinfo->mday = ptm->tm_mday;
	dtinfo->hours = ptm->tm_hour;
	dtinfo->min = ptm->tm_min;
	dtinfo->sec = ptm->tm_sec;

}

static int32_t set_encode_time(int8_t *data, int32_t vallen)  //zl
{
	if(vallen  != sizeof(SysTimeConfig)) {
		zlog_debug(z,"WRONG:vallen  !== sizeof(SysTimeConfig\n");
		return -1;
	}

	

	SysTimeConfig *dtinfo = (SysTimeConfig *)data;
	//	int ret = 0;
	//	memcpy(&dtinfo, data, vallen);
	zlog_debug(z,"dtinfo.year:%d,dtinfo.month:%d,dtinfo.mday:%d,dtinfo.hours:%d,dtinfo.min:%d,dtinfo.sec:%d\n",
	       dtinfo->year, dtinfo->month, dtinfo->mday, dtinfo->hours, dtinfo->min, dtinfo->sec);
	//zlog_debug(z,"1 the run time =%d=%lld\n", get_run_time(), getCurrentTime());
	setRtcTime(dtinfo->year, dtinfo->month, dtinfo->mday, dtinfo->hours,
	           dtinfo->min, dtinfo->sec);
	//zlog_debug(z,"2 the run time =%d=%lld\n", get_run_time(), getCurrentTime());

	return 0;
}
static int get_encode_time(char *data, int vallen)
{
	//	int ret = 0;
	SysTimeConfig dtinfo;
	r_memset(&dtinfo , 0, sizeof(SysTimeConfig));
	getSysTime(&dtinfo);
	memcpy(data, &dtinfo, vallen);
	zlog_debug(z,"getSysTime...\n");
	return 0;
}




/*消息头打包*/
static void msgPacket(int identifier, unsigned char *data, webparsetype type, int len, int cmd, int ret)
{
	webMsgInfo  msghead;
	int32_t	cmdlen = sizeof(type);
	int32_t retlen = sizeof(ret);
	msghead.identifier = identifier;
	msghead.type = type;
	msghead.len = len;
	r_memcpy(data, &msghead, MSGINFOHEAD);
	r_memcpy(data + MSGINFOHEAD, &cmd, cmdlen);
	r_memcpy(data + MSGINFOHEAD + cmdlen, &ret, retlen);
	//printf("msghead.len=%d\n", msghead.len);
	return ;
}

typedef enum XML_MSG_TYPE{
	XML_GET =0,
	XML_SET = 1,
}XML_MSG_TYPE;


#define XML_ASSERT(y)   do{\
						if(!(y))\
							{printf("error!!!%s failed!\n\n",#y);\
							return -1;}\
						}while(0);

static int32_t xml_packet_leaf_INT_data(xmlNodePtr node_ptr,const char * key,int data)
{	
	int8_t buffer[20] = {0};
	sprintf((char *)buffer,"%d",data);
	package_add_xml_leaf(node_ptr, (const xmlChar *)key, (const int8_t *)buffer);
	return 0;
}

static int32_t xml_packet_leaf_CHAR_data(xmlNodePtr node_ptr,const char * key,char* data)
{	
	package_add_xml_leaf(node_ptr, (const xmlChar *)key, (const int8_t *)data);
	return 0;
}


static int process_INT_xml_node(xmlNodePtr pnode_xml,parse_xml_t *parse_xml_info,const xmlChar *key)
{
	int ret=0;
	char body[8]={0};
	int len=8;
//	PRINTF("key--%s!\n",key);
	xmlNodePtr temp = get_children_node(pnode_xml, BAD_CAST key);
	XML_ASSERT(NULL != temp);
	ret = get_current_node_value(body, len, parse_xml_info->pdoc,  temp);
	XML_ASSERT(ret==0);
//	PRINTF("........body=%s\n",body);
	return atoi(body);
}

static int process_CHAR_xml_node(xmlNodePtr pnode_xml,parse_xml_t *parse_xml_info,const xmlChar *key,char* data,int data_len)
{
	int ret=0;
	XML_ASSERT(NULL!=data);
	//PRINTF("key:%s\n",key);
	xmlNodePtr temp = get_children_node(pnode_xml, BAD_CAST  key);
	XML_ASSERT(NULL != temp);
	ret = get_current_node_value(data, data_len, parse_xml_info->pdoc,  temp);
	XML_ASSERT(ret==0);
	return ret;
}


static int32_t xml_packet_swms_info(char* out_buf,int32_t enc_type,int msg_type,int32_t src_num,int res,int32_t layout)
{
	
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");
	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr PictureSynthesis 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);

	xml_packet_leaf_INT_data(head_node,"MsgCode",30047 );
	xml_packet_leaf_CHAR_data(head_node,"PassKey","WebCtrl" );

	PictureSynthesis = xmlNewNode(NULL, BAD_CAST "PictureSynthesis");
	xmlAddChild(body_node, PictureSynthesis);
	xml_packet_leaf_INT_data(PictureSynthesis,"RoomID",0 );
	xml_packet_leaf_INT_data(PictureSynthesis,"EncodeIndex ",enc_type);
	xml_packet_leaf_INT_data(PictureSynthesis,"Type ",msg_type);
	xml_packet_leaf_INT_data(PictureSynthesis,"SourceNum ",src_num);
	xml_packet_leaf_INT_data(PictureSynthesis,"Resolution ",res);
	xml_packet_leaf_INT_data(PictureSynthesis,"Model ",layout);
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(out_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}


static int32_t xml_parse_return_msg(int32_t sockid,char* buffer)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr PictureSynthesis_xml=NULL;

	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode    = NULL;
	char *pretcode = NULL;
	int select_ret = 0;
	select_ret = select_socket(sockid, WEBSELECTTIMEOUT);
	if (select_ret == 0) {
		zlog_debug2(OPELOG,"select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		zlog_debug2(OPELOG,"select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}
	memset(buffer,0,2048);
	
	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{
		close(sockid);
		zlog_debug2(OPELOG,"recv MsgHeader is fail\n");
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		zlog_debug2(OPELOG,"tcp_recv_longdata is fail %d\n",xmlen);
		close(sockid);
		return -1;
	}
	
	zlog_debug2(OPELOG,"recv-----> %s\n",buffer);
	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_debug2(OPELOG, "Analyze_SetCtrlProto: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)buffer);
	if(parse_xml_user == NULL)
	{
		zlog_debug2(OPELOG,"Analyze_SetCtrlProto: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		zlog_debug2(OPELOG,  "Analyze_SetCtrlProto: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		zlog_debug2(OPELOG, "Analyze_SetCtrlProto: msghead fail");
		goto EXIT;
	}

	return_code   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(return_code == 0)
	{
		return_code=-1;
		zlog_debug2(OPELOG, "Analyze_SetCtrlProto: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		zlog_debug2(OPELOG, "Analyze_SetCtrlProto: not found pretcode");
		goto EXIT;
	}

	int32_t irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		zlog_debug2(OPELOG,"Analyze_SetCtrlProto: ReturnCode [%d]",irecode);
		goto EXIT;
	}	
//	return_code = 1;

EXIT:
	if(NULL != pretcode)
	{
		xmlFree(pretcode);
	}
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}

static int32_t xml_packet_set_text_msg(int8_t *send_buf,TextInfo *config)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr AddTitleReq 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	xml_packet_leaf_INT_data(head_node, "MsgCode", 30018);
	xml_packet_leaf_CHAR_data(head_node, "PassKey", "WebCtrl");

	AddTitleReq = xmlNewNode(NULL, BAD_CAST "AddTitleReq");
	xmlAddChild(body_node, AddTitleReq);
	xml_packet_leaf_INT_data(AddTitleReq, "RoomID", 0);
	xml_packet_leaf_INT_data(AddTitleReq, "EncodeIndex", 1);
	xml_packet_leaf_CHAR_data(AddTitleReq, "Title", config->msgtext);
	xml_packet_leaf_INT_data(AddTitleReq, "Time", config->showtime);
	xml_packet_leaf_INT_data(AddTitleReq, "Position", config->postype);
	xml_packet_leaf_INT_data(AddTitleReq, "Xpos", config->xpos);
	xml_packet_leaf_INT_data(AddTitleReq, "Ypos", config->ypos);
	xml_packet_leaf_INT_data(AddTitleReq, "Alpha", config->alpha);
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t web_process_set_text_msg(TextInfo* text)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t sockid = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	sockid = connectToServer();

	if(sockid <= 0)
	{
		return -1;
	}

	len = xml_packet_set_text_msg(buffer+sizeof(MsgHeader), text);
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =     r_htons(2012);

	
	zlog_debug2(OPELOG,"%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}


	int select_ret = 0;

	select_ret = select_socket(sockid, WEBSELECTTIMEOUT);
	if (select_ret == 0) {
		zlog_debug2(OPELOG,"select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		zlog_debug2(OPELOG,"select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}

	memset(buffer, 0x0, 2048);
	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{
		close(sockid);
		zlog_debug2(OPELOG,"recv MsgHeader is fail\n");
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		zlog_debug2(OPELOG,"tcp_recv_longdata is fail %d\n",xmlen);
		close(sockid);
		return -1;
	}

	zlog_debug2(OPELOG,"-----> %s\n",buffer);

	if(1 != xml_parse_return_msg(sockid,buffer))
	{
		close(sockid);
		return -1;
	}

	close(sockid);
	return 1;

}


static int32_t xml_packet_logo_msg(int8_t *send_buf,LogoInfo *config)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr UploadLogReq 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);

	xml_packet_leaf_INT_data(head_node, "MsgCode", 30016);
	xml_packet_leaf_CHAR_data(head_node, "PassKey", "WebCtrl");

	UploadLogReq = xmlNewNode(NULL, BAD_CAST "UploadLogReq");
	xmlAddChild(body_node, UploadLogReq);

	xml_packet_leaf_INT_data(UploadLogReq, "RoomID", 0);
	xml_packet_leaf_INT_data(UploadLogReq, "EncodeIndex", 1);
	xml_packet_leaf_INT_data(UploadLogReq, "Format", 0);
	xml_packet_leaf_INT_data(UploadLogReq, "Transparency", config->alpha);
	xml_packet_leaf_INT_data(UploadLogReq, "Position", config->postype);
	xml_packet_leaf_INT_data(UploadLogReq, "Xpos", config->x);
	xml_packet_leaf_INT_data(UploadLogReq, "Ypos", config->y);

	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t web_process_set_logo_msg(LogoInfo* logo)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t sockid = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	sockid = connectToServer();

	if(sockid <= 0)
	{
		return -1;
	}

	len = xml_packet_logo_msg(buffer+sizeof(MsgHeader), logo);
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =     r_htons(2012);

	
	zlog_debug2(OPELOG,"%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}


	int select_ret = 0;

	select_ret = select_socket(sockid, WEBSELECTTIMEOUT);
	if (select_ret == 0) {
		zlog_debug2(OPELOG,"select_socket time out error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	} else if (select_ret < 0) {
		zlog_debug2(OPELOG,"select_socket error!!![%s]\n", strerror(errno));
		close(sockid);
		return -1;
	}

	memset(buffer, 0x0, 2048);
	if(sizeof(MsgHeader) != tcp_recv_longdata(sockid,  (int8_t *)buffer, sizeof(MsgHeader)))
	{
		close(sockid);
		zlog_debug2(OPELOG,"recv MsgHeader is fail\n");
		return -1;
	}

	MsgHeader *p = (MsgHeader* )buffer;
	int32_t xmlen = ntohs(p->sLen) - sizeof(MsgHeader) ;

	memset(buffer, 0x0, 2048);
	if(xmlen != tcp_recv_longdata(sockid,  (int8_t *)buffer, xmlen))
	{

		zlog_debug2(OPELOG,"tcp_recv_longdata is fail %d\n",xmlen);
		close(sockid);
		return -1;
	}

	zlog_debug2(OPELOG,"-----> %s\n",buffer);

	if(1 != xml_parse_return_msg(sockid,buffer))
	{
		close(sockid);
		return -1;
	}

	close(sockid);
	return 1;

}


static int32_t web_process_set_swms_info(int index,int res)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t sockid = 0;
	int enc_type=1;
	int msg_type = XML_SET;
	int src_num=0;
	int layout = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	sockid = connectToServer();
	if(sockid <= 0)
	{
		return -1;
	}
	switch(index){
		case 1:
			layout=0x211;
			src_num=2;
			break;
		case 2:
			layout=0x212;
			src_num=2;
			break;
		case 3:
			layout=0x213;
			src_num=2;
			break;
		case 4:
			layout=0x221;
			src_num=2;
			break;
		case 5:
			layout=0x222;
			src_num=2;
			break;
		case 6:
			layout=0x311;
			src_num=3;
			break;
		case 7:
			layout=0x411;
			src_num=4;
			break;
		case 8:
			layout=0x511;
			src_num=5;
			break;
		case 9:
			layout=0x611;
			src_num=6;
			break;
		case 10:
			layout=1;
			src_num=1;
			break;
		default:
			layout=1;
			src_num=1;
			break;
		}
	len = xml_packet_swms_info(buffer+sizeof(MsgHeader),enc_type,msg_type,src_num,res,layout);
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =    r_htons(2012);

	
	zlog_debug2(OPELOG,"send----->%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}
//----------------------------------------send finished
//----------------------------------------recv begin
	if(1 != xml_parse_return_msg(sockid,buffer))
	{
		close(sockid);
		return -1;
	}
	close(sockid);
	return 1;

}


static int32_t web_process_get_swms_info(int* index,int* res)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t sockid = 0;
	int enc_type=MP_ENC_INDEX;
	int msg_type = XML_GET;
	int src_num=0;
	int layout = 0;
	int ret = -1;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	parse_xml_t *parse_xml_info = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr PictureSynthesis_xml=NULL;
	sockid = connectToServer();	
	XML_ASSERT(sockid>0);
	zlog_debug2(OPELOG,"sockid=%d\n",sockid);
	//----------------------send msg
	len = xml_packet_get_msg(buffer+sizeof(MsgHeader),enc_type,msg_type,MSGCODE_PICTURE_SYNTHESIS,"PictureSynthesis");
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =    r_htons(2012);	
	zlog_debug2(OPELOG,"send------->\n%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}
	//--------------------------return msg
	ret = xml_parse_return_msg(sockid,buffer);
	parse_xml_info = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	XML_ASSERT(NULL!=parse_xml_info);
		
	init_dom_tree(parse_xml_info, (const char *)buffer);
	if(parse_xml_info == NULL)
	{
		zlog_debug2(OPELOG, " init_dom_tree fail");
		goto EXIT;
	}
	
	pnode_msg_body_xml =  get_children_node(parse_xml_info->proot, BAD_CAST "MsgBody");
	if(pnode_msg_body_xml == NULL)
	{
		zlog_debug2(OPELOG, "pnode_msg_body_xml fail");
		goto EXIT;
	}
	
	PictureSynthesis_xml = get_children_node(pnode_msg_body_xml,"PictureSynthesis");
	if(PictureSynthesis_xml == NULL)
	{
		zlog_debug2(OPELOG, "PictureSynthesis_xml fail");
		goto EXIT;
	}
	*res= process_INT_xml_node (PictureSynthesis_xml,parse_xml_info,"Resolution") ;
	if(*res< 0)
	{
		zlog_debug2(OPELOG, "res fail");
		goto EXIT;
	}
		

	layout= process_INT_xml_node (PictureSynthesis_xml,parse_xml_info,"Model") ;
	if(layout< 0)
	{
		zlog_debug2(OPELOG, "layout fail");
		goto EXIT;
	}
	switch(layout){
		case 0x211:	*index=1;break;
		case 0x212:*index=2;	break;
		case 0x213:*index=3;	break;
		case 0x221:*index=4;	break;
		case 0x222:*index=5;	break;
		case 0x311:*index=6;	break;
		case 0x411:*index=7;	break;
		case 0x511:*index=8;	break;
		case 0x611:*index=9;	break;
		case 1:*index=10;	break;
		default:*index=10;	break;
	}
EXIT:
	if(parse_xml_info->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_info->pdoc);
	}

	r_free(parse_xml_info);
	return ret;

}
static int32_t xml_packet_get_msg(char* out_buf,int32_t enc_type,int msg_type,int msg_code,const char* sub_key)
{
	
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");
	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr PictureSynthesis 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);

	xml_packet_leaf_INT_data(head_node,"MsgCode",msg_code );
	xml_packet_leaf_CHAR_data(head_node,"PassKey","WebCtrl" );

	PictureSynthesis = xmlNewNode(NULL, BAD_CAST sub_key);
	xmlAddChild(body_node, PictureSynthesis);
	xml_packet_leaf_INT_data(PictureSynthesis,"RoomID",0 );
	xml_packet_leaf_INT_data(PictureSynthesis,"EncodeIndex",enc_type);
	xml_packet_leaf_INT_data(PictureSynthesis,"Type ",msg_type);
//	xml_packet_leaf_INT_data(PictureSynthesis,"SourceNum",0);
//	xml_packet_leaf_INT_data(PictureSynthesis,"Resolution",0);
//	xml_packet_leaf_INT_data(PictureSynthesis,"Model",0);
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(out_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}


static int32_t web_process_get_text_info(TextInfo *info)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t sockid = 0;
	int enc_type=MP_ENC_INDEX;
	int msg_type = XML_GET;
	int src_num=0;
	int ret =-1;
	int layout = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	parse_xml_t *parse_xml_info = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr AddTitleReq_xml=NULL;
	sockid = connectToServer();	
	XML_ASSERT(sockid>0);
	//----------------------send msg
	len = xml_packet_get_msg(buffer+sizeof(MsgHeader),enc_type,msg_type,30018,"AddTitleReq");
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =    r_htons(2012);	
	zlog_debug2(OPELOG,"%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}
	//--------------------------return msg
	ret = xml_parse_return_msg(sockid,buffer);
	parse_xml_info = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	XML_ASSERT(NULL!=parse_xml_info);
	
	init_dom_tree(parse_xml_info, (const char *)buffer);
	if(parse_xml_info == NULL)
	{
		zlog_debug2(OPELOG," init_dom_tree fail");
		goto EXIT;
	}
	
	pnode_msg_body_xml =  get_children_node(parse_xml_info->proot, BAD_CAST "MsgBody");
	if(pnode_msg_body_xml == NULL)
	{
		zlog_debug2(OPELOG,"pnode_msg_body_xml fail");
		goto EXIT;
	}
	
	AddTitleReq_xml = get_children_node(pnode_msg_body_xml,"AddTitleReq");
	if(AddTitleReq_xml == NULL)
	{
		zlog_debug2(OPELOG, "PictureSynthesis_xml fail");
		goto EXIT;
	}
	
	info->postype= process_INT_xml_node (AddTitleReq_xml,parse_xml_info,"Position") ;
	if(layout< 0)
	{
		zlog_debug2(OPELOG, "layout fail");
		goto EXIT;
	}
	process_CHAR_xml_node (AddTitleReq_xml,parse_xml_info,"Title",info->msgtext ,sizeof(info->msgtext )) ;

	
	info->showtime= process_INT_xml_node (AddTitleReq_xml,parse_xml_info,"Time") ;	
	if(layout< 0)
	{
		zlog_debug2(OPELOG, "layout fail");
		goto EXIT;
	}
	
	info->alpha= process_INT_xml_node (AddTitleReq_xml,parse_xml_info,"Transparency") ;
	if(info->postype< 0)
	{
		zlog_debug2(OPELOG, "layout fail");
		goto EXIT;
	}
	info->xpos= process_INT_xml_node (AddTitleReq_xml,parse_xml_info,"Xpos") ;
	if(info->postype< 0)
	{
		zlog_debug2(OPELOG, "layout fail");
		goto EXIT;
	}
	
	info->ypos= process_INT_xml_node (AddTitleReq_xml,parse_xml_info,"Ypos") ;
	if(info->postype< 0)
	{
		zlog_debug2(OPELOG, "layout fail");
		goto EXIT;
	}
	
EXIT:
	if(parse_xml_info->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_info->pdoc);
	}

	r_free(parse_xml_info);
	return ret;

}


static int32_t web_process_get_logo_info(LogoInfo *info)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t sockid = 0;
	int enc_type=MP_ENC_INDEX;
	int msg_type = XML_GET;
	int src_num=0;
	int layout = 0;
	int ret=-1;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	parse_xml_t *parse_xml_info = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr AddTitleReq_xml=NULL;
	sockid = connectToServer();	
	XML_ASSERT(sockid>0);
	//----------------------send msg
	len =  xml_packet_get_msg(buffer+sizeof(MsgHeader),enc_type,msg_type,30016,"UploadLogReq");
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =    r_htons(2012);	
	zlog_debug2(OPELOG,"%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}
	//--------------------------return msg
	ret=	xml_parse_return_msg(sockid,buffer);
	parse_xml_info = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	XML_ASSERT(NULL!=parse_xml_info);
	
	init_dom_tree(parse_xml_info, (const char *)buffer);
	if(parse_xml_info == NULL)
	{
		printf( " init_dom_tree fail");
		goto EXIT;
	}
	
	pnode_msg_body_xml =  get_children_node(parse_xml_info->proot, BAD_CAST "MsgBody");
	if(pnode_msg_body_xml == NULL)
	{
		printf( "pnode_msg_body_xml fail");
		goto EXIT;
	}
	
	AddTitleReq_xml = get_children_node(pnode_msg_body_xml,"UploadLogReq");
	if(AddTitleReq_xml == NULL)
	{
		printf( "PictureSynthesis_xml fail");
		goto EXIT;
	}
	
	info->postype= process_INT_xml_node (AddTitleReq_xml,parse_xml_info,"Position") ;
	if(info->postype< 0)
	{
		printf( "layout fail");
		goto EXIT;
	}
	info->alpha= process_INT_xml_node (AddTitleReq_xml,parse_xml_info,"Transparency") ;
	if(info->postype< 0)
	{
		printf( "layout fail");
		goto EXIT;
	}
	info->x= process_INT_xml_node (AddTitleReq_xml,parse_xml_info,"Xpos") ;
	if(info->postype< 0)
	{
		printf( "layout fail");
		goto EXIT;
	}
	
	info->y= process_INT_xml_node (AddTitleReq_xml,parse_xml_info,"Ypos") ;
	if(info->postype< 0)
	{
		printf( "layout fail");
		goto EXIT;
	}
	
	
EXIT:
	if(parse_xml_info->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_info->pdoc);
	}

	r_free(parse_xml_info);
	return ret;

}


static int32_t web_process_get_system_info(System_Info_t*info)
{

	int8_t buffer[2048] = {0};
	int32_t len = 0;
	int32_t sockid = 0;
	int enc_type=MP_ENC_INDEX;
	int msg_type = XML_GET;
	int src_num=0;
	int ret = -1;
	int retcode = 0;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	parse_xml_t *parse_xml_info = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr EncVerReq_xml=NULL;	
	xmlNodePtr msghead    = NULL;
	sockid = connectToServer();	
	XML_ASSERT(sockid>0);
	//----------------------send msg
	len =  xml_packet_get_msg(buffer+sizeof(MsgHeader),enc_type,msg_type,30045,"EncVerReq");
	uint16_t sLen = sizeof(MsgHeader) + len;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = 0;
	pmsg->sVer =    r_htons(2012);	
	zlog_debug2(OPELOG,"%s\n",buffer+sizeof(MsgHeader));
	if(sLen != tcp_send_longdata(sockid,  (int8_t *)buffer, sLen))
	{
		close(sockid);
		return -1;
	}
	//--------------------------return msg
	ret = xml_parse_return_msg(sockid,buffer);
	if(1!= ret){
		zlog_debug2(OPELOG,"return 0\n");
		goto EXIT;		
	}
	
	parse_xml_info = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	XML_ASSERT(NULL!=parse_xml_info);
	
	init_dom_tree(parse_xml_info, (const char *)buffer);
	if(parse_xml_info == NULL)
	{
		printf( " init_dom_tree fail");
		goto EXIT;
	}
	
	pnode_msg_body_xml =  get_children_node(parse_xml_info->proot, BAD_CAST "MsgBody");
	if(pnode_msg_body_xml == NULL)
	{
		printf( "pnode_msg_body_xml fail");
		goto EXIT;
	}
	

	msghead   = get_children_node(parse_xml_info->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		printf(  "Analyze_SetCtrlProto: msghead fail");
		goto EXIT;
	}

	retcode   = process_INT_xml_node (msghead,parse_xml_info,"ReturnCode") ; 
	if(retcode == 0)
	{
		printf( "process_INT_xml_node: retcode fail");
		goto EXIT;
	}
	
	EncVerReq_xml = get_children_node(pnode_msg_body_xml,"EncVerReq");
	if(EncVerReq_xml == NULL)
	{
		printf( "PictureSynthesis_xml fail");
		goto EXIT;
	}
	
	process_CHAR_xml_node (EncVerReq_xml,parse_xml_info,"SysVer",info->hd_version,sizeof(info->hd_version)) ;
	if(info->hd_version == NULL)
	{
		printf( "hd_version fail");
		goto EXIT;
	}
	
EXIT:
	if(parse_xml_info->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_info->pdoc);
	}

	r_free(parse_xml_info);
	return 1;

}

int32_t midParseInt(int32_t identifier, int32_t fd, int8_t *data, int32_t len)
{
	int32_t recvlen;
	int32_t cmd = 0;
	int32_t actdata = 0;
	int32_t ret = 0;
	int32_t web_ret = SERVER_RET_OK;
	
	int32_t need_send = 0;
	
	
	int8_t senddata[1024] = {0};
	int32_t totallen = 0;
	
	recvlen = r_recv(fd, data, len, 0);
	int32_t out = 0;
	printf("recvlen = %d\n",recvlen);
	if(recvlen < 0)
	{
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		need_send = 1;
		goto EXIT;
	}
	
	r_memcpy(&cmd, data, sizeof(int));
	r_memcpy(&actdata, data + sizeof(int), len - sizeof(int));
	
	printf("cmd = 0x%04x\n", cmd);
	
	switch(cmd)
	{
		case MSG_SET_HDMI_RES:
			need_send=1;
			zlog_debug2(OPELOG,"--MSG_SET_HDMI_RES--\n");
			//xmL 转发
			zlog_debug2(OPELOG,"actdata=%d\n",actdata);
			break;
			
		case MSG_GET_HDMI_RES:
			need_send=1;
			zlog_debug2(OPELOG,"--MSG_GET_HDMI_RES--\n");
			//xmL 转发
			out=2;
			break;
	#if 0
		case MSG_SETLANGUAGE:
			{
				zlog_debug(z,"MSG_SETLANGUAGE %d\n",actdata);
				web_ret = SetLanguage(actdata);
				zlog_debug(z,"MSG_SETLANGUAGE <%d>\n",web_ret);
				need_send = 1;
			}
			break;
			
		case MSG_GETLANGUAGE:
			{
				zlog_debug(z,"MSG_GETLANGUAGE\n");
				web_ret = GetLanguage(&out, gpser);
				
				zlog_debug(z,"MSG_GETLANGUAGE   <%d >\n",web_ret);
				need_send = 1;
			}
			break;
		case MSG_SYSREBOOT:
			{
				zlog_debug(z,"MSG_SYSREBOOT\n");
				if (1 == check_all_record_status(gpser))
				{
					 web_ret = SERVER_HAVERECORD_FAILED;
				}
				else
				{
					zlog_debug(z," <GOD + 0> MSG_SYSREBOOT\n");
					rebootsys(5);
				}
				need_send = 1;	
			}
			break;

		case MSG_SETUSB_SWITCH: //zL
			{
				zlog_debug(z,"MSG_SETUSB_SWITCH %d\n",actdata);
            //  1 开启 2 关闭

				if (1 != SetUsbFunction(actdata))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				need_send = 1;
				break;		
			}
		case MSG_GETUSB_SWITCH: //zL
			{
				zlog_debug(z,"MSG_GETUSB_SWITCH\n");
            //  1 开启 2 关闭
				
			//  add DealProc  zhengyb
				if(1 != GetUsbFunction(&out))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				need_send = 1;
				break;		
			}
		#if 0
		case MSG_SET_SYSTEM_NAME: //zL
			{
				zlog_debug(z,"MSG_SET_SYSTEM_NAME\n");
				
			//  add DealProc  zhengyb
			//	if(1 != SetAddrBitsInfo(addr_bit_info))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				need_send = 1;
				break;
			}
		case MSG_GET_SYSTEM_NAME: //zL
			{
				zlog_debug(z,"MSG_GET_SYSTEM_NAME\n");
				
			//  add DealProc  zhengyb
			//	if(1 != SetAddrBitsInfo(addr_bit_info))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				need_send = 1;
				break;
			}	
		
			break;	

		#endif	
#endif
		default:
			printf("unkonwn cmd = %04x\n", cmd);
			need_send = 1;
			web_ret = SERVER_RET_UNKOWN_CMD;
			break;
	}
	
	if(ret < 0)
	{
		web_ret = SERVER_RET_INVAID_PARM_VALUE;
	}
	
EXIT:

	if(need_send == 1)
	{
		totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + sizeof(out);
		msgPacket(identifier, (unsigned char *)senddata, INT_TYPE, totallen, cmd, web_ret);
		r_memcpy(senddata + (totallen - sizeof(out)), &out, sizeof(out));
		zlog_debug(z,"the cmd =%04x,the value=%d,the ret=%04x\n", cmd, out, web_ret);
		r_send(fd, senddata, totallen, 0);
		
		if(web_ret != SERVER_RET_OK)
		{
			printf("ERROR,the cmd =0x%x,ret= 0x%x\n", cmd, web_ret);
		}
	}
	
	return 0;
}


int32_t midParseString(int32_t identifier, int32_t fd, int8_t *data, int32_t len)
{
	int32_t recvlen;
	int32_t cmd = 0;
	int8_t actdata[4096] = {0};
	int32_t vallen = 0;
	
	int8_t senddata[1024] = {0};
	int32_t totallen = 0;
	
	int8_t  out[4096] = "unknown cmd.";
	int32_t web_ret = SERVER_RET_OK;
	int32_t need_send = 0;
	
	recvlen = r_recv(fd, data, len, 0);
	
	vallen = len - sizeof(int);
	
	if(recvlen < 0 || vallen > sizeof(actdata))
	{
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		need_send = 1;
		goto EXIT;
	}

	r_memset(out, 0, 4096);
	r_memcpy(&cmd, data, sizeof(int));
	r_memcpy(actdata, data + sizeof(int), vallen);
	printf("cmd = %04x\n", cmd);
	
	switch(cmd)
	{
		case MSG_SETUSRPASSWORD:
			{
				zlog_debug(z,"MSG_SETUSRPASSWORD %s\n",actdata);

				 if(1 != SetUserPassword(actdata))
				 {
				 	web_ret = SERVER_RET_USER_INVALIED;
				 }
				need_send = 1;
			}
			break;
			
		case MSG_SET_CTRL_PROTO:
			{
				zlog_debug(z,"MSG_SET_CTRL_PROTO %s\n",actdata);

				 if(1 != SetCtrlProto(actdata))
				 {
				 	web_ret = SERVER_RET_USER_INVALIED;
				 }
				need_send = 1;
			}
			break;
		case MSG_DEL_CTRL_PROTO:
			{
				zlog_debug(z,"MSG_DEL_CTRL_PROTO %s\n",actdata);

				 if(1 != DelCtrlProto(actdata))
				 {
				 	web_ret = SERVER_RET_USER_INVALIED;
				 }
				need_send = 1;
			}
			break;
		case MSG_UPGRADE_CTRL_PROTO:
			{
				zlog_debug(z,"MSG_UPGRADE_CTRL_PROTO %s\n",actdata);

				 if(1 != UpgradeCtrlProto(actdata))
				 {
				 	web_ret = SERVER_RET_USER_INVALIED;
				 }
				need_send = 1;
			}
			break;
		
		case MSG_GETUSRPASSWORD:
			{
				zlog_debug(z,"MSG_GETUSRPASSWORD %s\n",actdata);

				if(1 != GetUserPassword(out, gpser))
				{
					web_ret = SERVER_RET_USER_INVALIED;
				}
				need_send = 1;
			}
			break;
			
		case MSG_SYSUPGRADE:
			{				
#if 1
				zlog_debug(z,"MSG_SYSUPGRADE");
				if(gpser != NULL)
				{
					if (1 == check_all_record_status(gpser))
					{
						 web_ret = SERVER_HAVERECORD_FAILED;
						 system("touch /tmp/recording"); 						 
					} 
					else 
					{
						pid_t pid = fork();      
					    if (pid == 0)    
					    {
							if(gpser != NULL)
							{
								if (1 == check_all_record_status(gpser))
								{
									 web_ret = SERVER_HAVERECORD_FAILED;
								}
								else
								{
									/* 进行升级 */
									web_ret = UpgradeApp(actdata);
									if(web_ret == SERVER_RET_OK)
									{
										web_ret = Dm6467_Upgrade((int8_t *)DM6467_UPDATE_PACKET);
										updatekernel();
										
									}
								}

							}

							if(web_ret == SERVER_RET_OK)
							{
								system("touch /tmp/success");
								rebootsys(5);
							}
							else if (web_ret == SERVER_HAVERECORD_FAILED) 
							{
								system("touch /tmp/recording"); 
							}
							else
							{
								system("touch /tmp/fail");
							}
						  
					    }

					}
				}
				else
				{
					system("touch /tmp/fail");
				}
#else
				int32_t IsUpgrade;
				int8_t cmd[1024] = {0};
				//IsUpgrade = verifyUpgradePackage(actdata);
				//printf("------>%d\n",IsUpgrade);
				
				/* 安装包校验失败 */
				if(1 != IsUpgrade && 0)
				{
					web_ret = SERVER_VERIFYFILE_FAILED;
				
				}
				else
				{

					//system("flash_eraseall /dev/mtd8");
					system("ubiattach /dev/ubi_ctrl -m 8");
					sleep(2);
					if(0 == system("mount -t ubifs /dev/ubi1_0 /mnt/usb/"))
					{
						sprintf((char *)cmd, "tar -xzvf %s -C %s",actdata,"/mnt/usb");
						if(0 != system(cmd))
						{
							web_ret = SERVER_VERIFYFILE_FAILED;
						}
						system("rm -f /var/log/recserver/update.tar.gz");
						system("sync");
						//rebootsys(5);
					}
					else
					{
						web_ret = SERVER_VERIFYFILE_FAILED;
						system("rm -f /var/log/recserver/update.tar.gz");
					}
				}
			
#endif
				r_strcpy(out, (int8_t *)"MSG_SYSUPGRADE");
				need_send = 1;
			
			}
		break;

		case MSG_SYSUPGRADE_6467:
			{

				zlog_debug(z,"MSG_SYSUPGRADE_6467 %s\n",actdata);
				if(gpser != NULL)
				{
					if (1 == check_all_record_status(gpser))
					{				
						 system("touch /tmp/recording"); 					   
						 web_ret = SERVER_HAVERECORD_FAILED;
					}
					else
					{
						web_ret	= Dm6467_Upgrade(actdata);
						if (web_ret == SERVER_RET_OK)
							system("touch /tmp/success"); 
						else
							system("touch /tmp/fail"); 
					}
				}
				r_strcpy(out, (int8_t *)"MSG_SYSUPGRADE_6467");
				need_send = 1;
			}
			break;
	
		
		case MSG_SYSROLLBACK:
			{	

				if (1 == check_all_record_status(gpser))
				{
					 web_ret = SERVER_HAVERECORD_FAILED;
				}
				else
				{
					//进行回退
					web_ret = SysRollback();
					if(web_ret == SERVER_RET_OK)
					{
						rebootsys(5);
					}
				}
				
				r_strcpy(out, (int8_t *)"MSG_SYSROLLBACK");
				need_send = 1;
			}
		break;
		case MSG_SETSERIALNUM:
			{
				web_ret = SetDevSerialNumber(actdata);
				if(1 != web_ret)
				{
					web_ret = SERVER_RET_INVAID_PARM_LEN;
				}
				need_send = 1;
			}
		break;
		case MSG_SETAUDIOTYPE:
			{

				zlog_debug(z,"MSG_SETAUDIOTYPE %s\n",actdata);
				//web_ret = SetAudioProto(actdata);
				if(1 != SetAudioProto(actdata))
				{
					web_ret = SERVER_RET_INVAID_PARM_LEN;
				}
				need_send = 1;
			}
		break;
		case MSG_GETAUDIOTYPE:
			{
				zlog_debug(z,"MSG_GETAUDIOTYPE\n");
				//web_ret = GetAudioProto(out);
				if(1 != GetAudioProto(out))
				{
					web_ret = SERVER_RET_INVAID_PARM_LEN;
				}

				zlog_debug(z,"%s\n",out);
				need_send = 1;
			}
		break;
		case MSG_GETGUESTPASSWORD:
			{
				zlog_debug(z,"MSG_GETGUESTPASSWORD \n");

				if(1 != GetGuestPassword(out, gpser))
				{
					web_ret = SERVER_RET_USER_INVALIED;
				}
				
				zlog_debug(z,"GET THE RTSP PASSWORD  <%s>  !\n",out);
				need_send = 1;
			}
			break;
		case MSG_SETGUESTPASSWORD:
			{
				zlog_debug(z,"MSG_SETGUESTPASSWORD %s\n",actdata);

				if(1 != SetGuestPassword(actdata))
				{
					web_ret = SERVER_RET_USER_INVALIED;
				}
				else
				{
					
					zlog_debug(z,"SET THE RTSP controlWebSetPreviewPassword !\n");
					controlWebSetPreviewPassword((char *)actdata);
				}
				zlog_debug(z,"SET THE RTSP PASSWORD  <%s>  !\n",actdata);
				need_send = 1;
			}
			break;

		case ROOM_CONTROL_GET_PREVIEW_PASS:
			{
				zlog_debug(z,"ROOM_CONTROL_GET_PREVIEW_PASS \n");
				if(1 != GetGuestPassword(out, gpser))
				{
					web_ret = SERVER_RET_USER_INVALIED;
				}
				zlog_debug(z,"GET THE RTSP ROOM_CONTROL_GET_PREVIEW_PASS  <%s>  !\n",out);
				need_send = 1;
				break;
			}

		case MSG_SET_SYSTEM_NAME: //zL
			{
				zlog_debug(z,"MSG_SET_SYSTEM_NAME\n");
				#if 0
				zlog_debug(z,"MSG_SET_SYSTEM_NAME ------- %s\n",actdata);
				if(1 != SetRoomName(actdata))
				{
					web_ret = SERVER_RET_USER_INVALIED;
				}
				need_send = 1;
				#endif
		#if 1
				int temp_web_ret = -1;
				char stream_temp[512] = {0};
		//		zlog_debug(z,"MSG_SET_SYSTEM_NAME-----%d\n",strlen(actdata));
				temp_web_ret = code_convert("gb2312", "utf-8", (char *)actdata, strlen((const char *)actdata), stream_temp, strlen((const char *)actdata) * 4);
			//	zlog_debug(z,"FUCK !!!!!!!!!!!!!!!!!!!!!!!-------------------<OLD_STREAM : %s >  <OLD_STREAM_LEN : %d>  <NEW_STREAM : %s>\n",
			//		actdata,get_text_size(actdata),stream_temp);

				if (web_ret == 0)
				{
					r_bzero(actdata, 4096);
					r_memcpy(actdata, stream_temp, 512);

				//	zlog_debug(z,"SHIRT !!!!!!!!!!!!!!!!!!!!!!!-------------------<OLD_STREAM : %s >  <OLD_STREAM_LEN : %d>  <NEW_STREAM : %s>\n",
			//			actdata,get_text_size(actdata),stream_temp);

				} 
				else 
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
					break;
				}
				zlog_debug(z,"MSG_SET_SYSTEM_NAME ------- %s\n",actdata);
				if(1 != SetRoomName(actdata))
				{
					web_ret = SERVER_RET_USER_INVALIED;
				}
				need_send = 1;
			#endif	
				break;
			}
		case MSG_GET_SYSTEM_NAME: //zL
			{
				zlog_debug(z,"MSG_GET_SYSTEM_NAME\n");
				
				int temp_web_ret = -1;
				char stream_temp[512] = {0};
				if(1 != GetRoomName(out, gpser))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				zlog_debug(z,"%d ----------MSG_GET_SYSTEM_NAME- <%s> \n",strlen((char *)out),out);
				temp_web_ret = code_convert("utf-8", "gb2312", (char *)out, strlen((const char *)out), stream_temp, strlen((const char *)out)*4);
	//			temp_web_ret = code_convert("gb2312", "utf-8", actdata, strlen(actdata), stream_temp, strlen(actdata) * 4);
			//	zlog_debug(z,"FUCK !!!!!!!!!!!!!!!!!!!!!!!-------------------<OLD_STREAM : %s >  <OLD_STREAM_LEN : %d>  <NEW_STREAM : %s>\n",
			//		out,get_text_size(out),stream_temp);

				if (web_ret == 0)
				{
					r_bzero(out, 4096);
					r_memcpy(out, stream_temp, 512);

					zlog_debug(z,"SHIRT !!!!!!!!!!!!!!!!!!!!!!!-------------------<OLD_STREAM : %s >  <OLD_STREAM_LEN : %d>  <NEW_STREAM : %s>\n",
						out,get_text_size((char *)out),stream_temp);

				} 
				else 
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
					break;
				}

				zlog_debug(z,"MSG_GET_SYSTEM_NAME ------- %s\n",out);
				need_send = 1;
				break;
			}
		default
				:
			printf("Warnning,the cmd %d is UNKOWN\n", cmd);
			need_send = 1;
			break;
	}
	
EXIT:

	if(need_send == 1)
	{
		totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + r_strlen(out);
		msgPacket(identifier, (unsigned char *)senddata, STRING_TYPE, totallen, cmd, web_ret);
		r_memcpy(senddata + (totallen - r_strlen(out)), out, r_strlen(out));
		zlog_debug(z,"the cmd =%04x,the out=%s,the ret=%04x\n", cmd, out, web_ret);
		r_send(fd, senddata, totallen, 0);
		
		if(web_ret != SERVER_RET_OK)
		{
			printf("ERROR,the cmd =0x%x,ret= 0x%x\n", cmd, web_ret);
		}
	}
	
	return 0;
}

int32_t midParseStruct(int32_t identifier, int32_t fd, int8_t *data, int32_t len)
{
	int32_t recvlen;
	int32_t cmd = 0;
	int8_t actualdata[4096] = {0};
	char out[4096] = {0};
	int32_t  vallen = 0;
	int32_t  status = 0;

	int8_t senddata[5120] = {0};
	int32_t totallen = 0;
	int32_t ret=0;
	int channel=0;
	int32_t web_ret =   SERVER_RET_OK;
	int32_t need_send = 0;

	int32_t subcmd = 0;
	recvlen = r_recv(fd, data, len, 0);
	
	if(recvlen < 0)
	{
		printf("recv failed,errno = %d,error message:%s \n", errno, strerror(errno));
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		status = -1;
		goto EXIT;
	}
	
	//vallen = len - sizeof(int);

	r_memset(out, 0, 4096);
	r_memcpy(&cmd, data, sizeof(int));
	r_memcpy(actualdata, data + sizeof(int), recvlen - sizeof(int));
	vallen = recvlen - sizeof(int);
	
	printf("-----> recv msgtype[%x]\n", cmd);
	switch(cmd)
	{		
		case MSG_SET_SWMS_LAYOUT:{
			Moive_Info_t *moive_info =(Moive_Info_t*)actualdata ;
			need_send=1;
			zlog_debug2(OPELOG,"--MSG_SET_SWMS_LAYOUT--\n");
			
			if(vallen != sizeof(Moive_Info_t)) {
				ret = -1;
				zlog_debug2(OPELOG,"WRONG:vallen != sizeof(Moive_Info_t) \n");
				return ret;
			}			
			//xmL 转发
			web_ret = web_process_set_swms_info(moive_info->model,moive_info->res);
			zlog_debug2(OPELOG,"actdata=%d\n",actualdata);
		}
		break;
			
		case MSG_GET_SWMS_LAYOUT:
			{
			Moive_Info_t moive_info;
			zlog_debug2(OPELOG,"--MSG_GET_SWMS_LAYOUT--\n");
			if(vallen != sizeof(Moive_Info_t)) {
				ret = -1;
				zlog_debug2(OPELOG,"WRONG:vallen != sizeof(Moive_Info_t) \n");
				return ret;
			}
			need_send=1;
			web_ret = web_process_get_swms_info(&(moive_info.model),&(moive_info.res));
			memcpy(out,&moive_info,sizeof(Moive_Info_t));
			}
		break;		
#if 0
		case MSG_GETENABLETEXTINFO:
			r_printf("-----MSG_GETENABLETEXTINFO----\n");
			//xml转发
			if(vallen != sizeof(WebEnableTextInfo)) {
				ret = -1;
				r_printf("WRONG:vallen != sizeof(LogoInfo) \n");
				return ret;
			}
			
			need_send = 1;
		break;		

		case MSG_SETENABLETEXTINFO:
			r_printf("-----MSG_SETENABLETEXTINFO----\n");
			//xml转发
			if(vallen != sizeof(WebEnableTextInfo)) {
				ret = -1;
				r_printf("WRONG:vallen != sizeof(LogoInfo) \n");
				return ret;
			}
			need_send = 1;
		break;		
			#endif
		case MSG_GET_LOGOINFO:{
				LogoInfo logo;
			zlog_debug2(OPELOG,"-----MSG_GET_LOGOINFO----\n");
			//xml转发
			if(vallen != sizeof(LogoInfo)) {
				ret = -1;
				zlog_debug2(OPELOG,"WRONG:vallen != sizeof(LogoInfo) \n");
				return ret;
			}
			web_ret = web_process_get_logo_info(&logo);
			
			need_send = 1;
			}
		break;		

		case MSG_SET_LOGOINFO:
			{
			LogoInfo *logo = (LogoInfo*)actualdata;
			zlog_debug2(OPELOG,"-----MSG_SET_LOGOINFO----\n");
			need_send  = 1;
			if(vallen != sizeof(LogoInfo)) {
				ret = -1;
				zlog_debug2(OPELOG,"WRONG:vallen != sizeof(LogoInfo) \n");
				return ret;
			}
			web_ret = web_process_set_logo_msg(logo);
			zlog_debug2(OPELOG,"Logo info:(%x,%x),alpha=%d,pos=%d\n",logo->x,logo->y,logo->alpha,logo->postype);
			//xml转发
			
			}
			break;
			
		case MSG_SET_TEXTINFO:
			{
			TextInfo *text = (TextInfo*)actualdata;
			zlog_debug2(OPELOG,"-----MSG_SET_TEXTINFO----\n");
			need_send = 1;
			if(vallen != sizeof(TextInfo)) {
				zlog_debug2(OPELOG,"vallen=%d \n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				return -1;
			}
			web_process_set_text_msg(text);
			zlog_debug2(OPELOG,"text info:%s,(%x,%x),alpha=%d,time=%d,pos=%d\n",text->msgtext,text->xpos,text->ypos,text->alpha,text->showtime,text->postype);
			//xml转发
			}
			break;

		case MSG_GET_TEXTINFO:
			{
			TextInfo text;
			zlog_debug2(OPELOG,"-----MSG_GET_TEXTINFO----\n");
			need_send = 1;
			if(vallen != sizeof(TextInfo)) {
				zlog_debug2(OPELOG,"vallen=%d \n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				return -1;
			}
			web_process_get_text_info(&text);
			memcpy(&out,&text,sizeof(TextInfo));
			//xml转发
			}
			break;

		case MSG_GETSYSPARAM:			
			{
			System_Info_t sys_info;
			zlog_debug2(OPELOG,"-----MSG_GETSYSPARAM----\n");
			need_send = 1;
			if(vallen != sizeof(System_Info_t)) {
				printf("vallen=%d \n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				return -1;
			}
			//xml转发
			web_process_get_system_info(&sys_info);
			memcpy(out,&sys_info,sizeof(System_Info_t));
			}
			break;
		
#if 0
		case MSG_GETDEVINFO:
		{
			zlog_debug(z,"MSG_GETDEVINFO\n");
			DevInfo *pdevinfo = (DevInfo*)out;
			if(1 != GetDevInfo(pdevinfo))
			{
				web_ret = SERVER_GETDEVINFO_FAILED;
			}
			vallen = sizeof(DevInfo);
			need_send = 1;
			
		}
		break;
		case MSG_GET6467INFO:
		{
			zlog_debug(z,"MSG_GET6467INFO\n");
			Dm6467Info *pinfo = (Dm6467Info*)out;
			if(r_strlen((int8_t*)g6467info.sysversion) > 0)
			{
				r_memcpy((int8_t *)pinfo,(int8_t *)&g6467info, sizeof(Dm6467Info));
			}
			else
			{
	
				if(1 != GetDm6467Info(pinfo))
				{
					r_memset((int8_t *)&g6467info,0x0,sizeof(Dm6467Info));
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				else
				{
					r_memcpy((int8_t *)&g6467info,(int8_t *)pinfo, sizeof(Dm6467Info));
				}
			}
				
			vallen = sizeof(Dm6467Info);
			printf("%s %s %s\n",pinfo->sysversion,pinfo->uImageversion,pinfo->fpgaversion);
			need_send = 1;
		}
		break;
		
		case MSG_GET_CTRL_PROTO:
			{
				zlog_debug(z,"MSG_GET6467INFO\n");
				ProtoList *pinfo = (ProtoList*)out;
				if(1 != GetCtrlProto(pinfo))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				int i = 0;
				for(i = 0; i< pinfo->num;i++)
				printf("%s\n",pinfo->protolist[i]);
				vallen = sizeof(ProtoList);
				need_send = 1;
			}

			break;
		case MSG_GETDISKINFO:
			{
				zlog_debug(z,"MSG_GETDISKINFO\n");
				DiskInfo *pdiskinfo = (DiskInfo*)out;
				if(1 != GetDiskInfo(pdiskinfo))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				
				vallen = sizeof(DiskInfo);
				printf("%u %u\n",pdiskinfo->disksize,pdiskinfo->remainsize);
				need_send = 1;
			}
			break;
			
		case MSG_SETLANIPCONFIG:
			{
				zlog_debug(z,"MSG_SETLANIPCONFIG\n");
				IpConfig *ip = (IpConfig*)actualdata;

				printf("-->%s %s %s %d\n",ip->gateway,ip->ipaddr,ip->netmask,ip->iptype);
				if (0 == check_all_record_status(gpser)) 
				{
					if(1 != SetLanIpconfig(ip))
					{
						web_ret = SERVER_GETDEVINFO_FAILED;
					}
					else
					{
						rebootsys(5);
					}
				} else {
					web_ret = SERVER_HAVERECORD_FAILED;
				}
				vallen = sizeof(IpConfig);
				need_send = 1;
			}
			break;
			
		case MSG_SETWANIPCONFIG:
			{
				zlog_debug(z,"MSG_SETWANIPCONFIG\n");
				IpConfig *ip = (IpConfig*)actualdata;

				printf("-->%s %s %s %d\n",ip->gateway,ip->ipaddr,ip->netmask,ip->iptype);
				if(1 != SetWanIpconfig(ip))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				
				vallen = sizeof(IpConfig);
				need_send = 1;
			}
			break;
			
		case MSG_SETOSDCONFIG:
			{
				zlog_debug(z,"MSG_SETOSDCONFIG\n");
				OsdConfig *osd = (OsdConfig*)actualdata;
				int8_t tmp[200*4+1] = {0};

				web_ret = code_convert("gb2312", "utf-8", (char *)osd->Title, strlen(osd->Title), (char *)tmp, strlen(osd->Title) * 4);
				zlog_debug(z,"-->INPUT %d  TITLEPOSTION %d  DISTIME %d %s %d, tmp = %s\n",osd->input,osd->TitlePositon,osd->DisTime,osd->Title,get_text_size(osd->Title), tmp);
				if (web_ret == 0)
				{
					r_bzero(osd->Title, sizeof(osd->Title));
					r_memcpy(osd->Title, tmp, sizeof(osd->Title)-1);

					zlog_debug(z,"-->INPUT %d  TITLEPOSTION %d  DISTIME %d %s %d, tmp = %s\n",osd->input,osd->TitlePositon,osd->DisTime,osd->Title,get_text_size(osd->Title), tmp);
					if(1 != web_process_set_text_msg(osd))
					{
						
						web_ret = SERVER_GETDEVINFO_FAILED;
					}
				} else {
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				zlog_debug(z,"<GOD!>	<web_ret : %d>\n",web_ret);
				vallen = sizeof(OsdConfig);
				need_send = 1;
			}
			break;
			
		case MSG_SETLOGOCONFIG:
			{
				zlog_debug(z,"MSG_SETLOGOCONFIG\n");
				LogoConfig *logo = (LogoConfig*)actualdata;
				printf("-->INPUT %d LogoPositon %d\n",logo->input,logo->LogoPositon);

				if(1 != UpgradeLogo(logo))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				
				vallen = sizeof(LogoConfig);
				need_send = 1;
			}
			break;
		
		case MSG_GETLANIPCONFIG:
			{
				zlog_debug(z,"MSG_GETLANIPCONFIG\n");
				IpConfig *ipinfo = (IpConfig*)out;
				if(1 != GetLanIpconfig(ipinfo, gpser))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				vallen = sizeof(IpConfig);
				need_send = 1;
				
			}
			break;
			
		case MSG_GETWANIPCONFIG:
			{
				zlog_debug(z,"MSG_GETWANIPCONFIG\n");
				IpConfig *ipinfo = (IpConfig*)out;
				if(1 != GetWanIpconfig(ipinfo, gpser))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				vallen = sizeof(IpConfig);
				need_send = 1;
				
			}
			break;

		case MSG_SETADDRBITS: //zyb
			{
				AddrBitConfig *addr_bit_info = (AddrBitConfig*)actualdata;
				
	//			zlog_debug(z,"MSG_SETADDRBITS ------------------ ADDR_1 : %d ADDR_2 : %d ADDR_3 : %d\n",
	//				addr_bit_info->addr_1,addr_bit_info->addr_2,addr_bit_info->addr_3);
				if(1 != SetAddrBitsInfo(addr_bit_info))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				vallen = sizeof(AddrBitConfig);
				need_send = 1;
				break;
			}
		
		case MSG_GETADDRBITS: //zyb
			{
				AddrBitConfig *addr_bit_info = (AddrBitConfig*)out;
				
				addr_bit_info->addr_1 = 1;
				addr_bit_info->addr_2 = 2;
				addr_bit_info->addr_3 = 3;
	//			zlog_debug(z,"MSG_GETADDRBITS ------------------ ADDR_1 : %d ADDR_2 : %d ADDR_3 : %d\n",
	//				addr_bit_info->addr_1,addr_bit_info->addr_2,addr_bit_info->addr_3);

				if(1 != GetAddrBitsInfo(addr_bit_info))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				vallen = sizeof(AddrBitConfig);
				need_send = 1;
				break;
			}

		case MSG_SET_THR_FTPINFO:
			{
				zlog_debug(z,"MSG_SET_THR_FTPINFO\n");
				ftp_info *pftpinfo2 = (ftp_info*)actualdata;
				
				fprintf(stderr, "\n\n\n\n\n\n");
				fprintf(stderr, "set ftp info!\n");
				fprintf(stderr, "mode: %d\n", pftpinfo2->Mode);
				fprintf(stderr, "ip: \t%s\n", pftpinfo2->THRFTPAddr);
				fprintf(stderr, "port: %d\n", pftpinfo2->THRFTPPort);
				fprintf(stderr, "uname: %s\n", pftpinfo2->THRFTPUserName);
				fprintf(stderr, "passwd: %s\n", pftpinfo2->THRFTPPassword);
				fprintf(stderr, "path: %s\n", pftpinfo2->THRFTPPath);
				fprintf(stderr, "\n\n\n\n\n\n");
				
				if(1 != SetThrFtpinfo(pftpinfo2, gpser))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				vallen = sizeof(ftp_info);
				need_send = 1;    //zl
				zlog_debug(z,"MSG_SET_THR_FTPINFO IS OVER!+++++++++++++++++++++++<GOD>\n");
				break;
			}
		case MSG_GET_THR_FTPINFO:
			{
				ftp_info *pftpinfo = (ftp_info*)out;
				if(1 != GetThrFtpinfo(pftpinfo, gpser))
				{
					web_ret = SERVER_GETDEVINFO_FAILED;
				}
				
				fprintf(stderr, "\n\n\n\n\n\n");
				fprintf(stderr, "get ftp info, size = %d\n", sizeof(ftp_info));
				fprintf(stderr, "mode: %d\n", pftpinfo->Mode);
				fprintf(stderr, "ip: \t%s\n", pftpinfo->THRFTPAddr);
				fprintf(stderr, "port: %d\n", pftpinfo->THRFTPPort);
				fprintf(stderr, "uname: %s\n", pftpinfo->THRFTPUserName);
				fprintf(stderr, "passwd: %s\n", pftpinfo->THRFTPPassword);
				fprintf(stderr, "path: %s\n", pftpinfo->THRFTPPath);
				fprintf(stderr, "\n\n\n\n\n\n");
				
				vallen = sizeof(ftp_info);
				need_send = 1;
				break;
			}
		case MSG_SET_SYNCTIME:
			zlog_debug(z,"cmd = 0x%04x is MSG_SYNCTIME.\n", cmd);
			need_send = 1;
			
			vallen = recvlen - sizeof(int);
			
			if(vallen != sizeof(SysTimeConfig)) {
				zlog_debug(z,"WARNING:vallen != sizeof(SysTimeConfig)!\n");
				goto EXIT;
			}
			if (1 == check_all_record_status(gpser))
			{
				web_ret = SERVER_HAVERECORD_FAILED;
			}
			else
			{
				set_encode_time(actualdata, vallen);
			}
			break;
		
		case MSG_GET_SYSCTIME:
			zlog_debug(z,"cmd = 0x%04x --- MSG_GETENCODETIME.\n",cmd);
			need_send = 1;
			vallen = recvlen - sizeof(int);

			if(vallen != sizeof(SysTimeConfig)) {
				zlog_debug(z,"WARNING:vallen != sizeof(DATE_TIME_INFO)!\n");
				goto EXIT;
			}
			get_encode_time(out, vallen);
			break;
		default:
				
			break;
#endif
		default:
			zlog_debug2(OPELOG,"unkonwn cmd = 0x%04x\n", cmd);
			need_send = 1;
			web_ret = SERVER_RET_UNKOWN_CMD;
			break;
	}
	
EXIT:
	if(need_send == 1)
	{
		totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + vallen;
		msgPacket(identifier, (unsigned char *)senddata, STRING_TYPE, totallen, cmd, web_ret);
		r_memcpy(senddata + (totallen - vallen), out, vallen);
		zlog_debug(z,"-----> the cmd = %04x,the ret = %04x\n", cmd,  web_ret);
		if(web_ret != SERVER_RET_OK)
		{
			zlog_debug(z,"ERROR, the cmd = 0x%x, ret = 0x%x\n", subcmd, web_ret);
		}
		int ret = r_send(fd, senddata, totallen, 0);
		if(ret != totallen)
		{
			zlog_debug(z,"SEND_ERROR, the cmd = 0x%x", subcmd);
		}
		
	}
	zlog_debug(z,"GOD IS OVER!\n");
	return status;
}



void *weblisten()
{
	printf("weblisten2000ThrFxn start\n");
	void                   *status              = 0;
	int32_t 					listenSocket  		= 0 , flags = 0;
	struct sockaddr_in 		addr;
	int32_t len, client_sock, opt = 1;
	struct sockaddr_in client_addr;
	webMsgInfo		webinfo;
	ssize_t			recvlen;
	int8_t  data[5120] = {0};
	
	z  = zlog_get_category("WebLog");
	if(z == NULL)
	{
		fprintf(stderr, "get WebLog  log fail\n");
	}

	
	len = sizeof(struct sockaddr_in);
	r_memset(&client_addr, 0, len);
	listenSocket =	r_socket(PF_INET, SOCK_STREAM, 0);
	
	if(listenSocket < 1)
	{
		status  = THREAD_FAILURE;
		return status;
	}
	
	r_memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family =       AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = r_htons(ENCODESERVER_1260_PORT);
	
	r_setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	if(r_bind(listenSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
	{
		zlog_debug2(OPELOG,"[weblistenThrFxn] bind failed,errno = %d,error message:%s \n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status;
	}
	
	if(-1 == r_listen(listenSocket, MAX_LISTEN))
	{
		zlog_debug2(OPELOG,"listen failed,errno = %d,error message:%s \n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status;
	}
	
	
	if((flags = fcntl(listenSocket, F_GETFL, 0)) == -1)
	{
		zlog_debug2(OPELOG,"fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status ;
	}
	
	if(fcntl(listenSocket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		zlog_debug2(OPELOG,"fcntl F_SETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status ;
	}
	

	while(1)
	{
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(listenSocket, &rfds);
		
		//接收recv_buf 复位为空!
		r_select(FD_SETSIZE, &rfds , NULL , NULL , NULL);
		client_sock = r_accept(listenSocket, (struct sockaddr *)&client_addr, (socklen_t *)&len);
		if(0 > client_sock)
		{
			printf("\n");
			
			if(errno == ECONNABORTED || errno == EAGAIN)
			{
				//usleep(20000);
				continue;
			}
			printf("weblisten thread Function errno  = %d\n", errno);
			status  = THREAD_FAILURE;
			return status;
		}
	
		r_memset(&webinfo, 0, sizeof(webinfo));
		recvlen = r_recv(client_sock, &webinfo, sizeof(webinfo), 0);
		       
		if(recvlen < 1)
		{
			printf("recv failed,errno = %d,error message:%s,client_sock = %d\n", errno, strerror(errno), client_sock);
			status  = THREAD_FAILURE;
			return status;
		}
		
		if(webinfo.identifier != WEB_IDENTIFIER)
		{
			printf("id  error,client_sock = %d\n", client_sock);
			status  = THREAD_FAILURE;
			return status;
		}
		
		len = webinfo.len - sizeof(webinfo);
		
		zlog_debug(z,"----> web deal begin =%d %d\n", webinfo.type,len );
		
		switch(webinfo.type)
		{
			case INT_TYPE:
				midParseInt(webinfo.identifier, client_sock, data, len);
				break;
				
			case STRING_TYPE:
				midParseString(webinfo.identifier, client_sock, data, len);
				break;
				
			case STRUCT_TYPE:
				midParseStruct(webinfo.identifier, client_sock, data, len);
				break;
				
			default
					:
				break;
		}
		
		zlog_debug(z,"----> web deal end =%d\n", webinfo.type);
		shutdown(client_sock, SHUT_RDWR);
		r_close(client_sock);
	}
	
	
	r_close(listenSocket);
	zlog_debug(z,"Web listen Thread Function Exit!!\n");
	return status;
}


extern int32_t get_user_log_head(int16_t port, int32_t user_index,platform_em platform,
								int8_t *plog_head, int8_t *ipaddr);

int32_t app_weblisten_init(int32_t flag)
{
	pthread_t           webListen;
#if 0
	server_set gpser1; 
	struct _all_server_info_ all;
	gpser1.pserinfo = &all;
	gpser1.pserinfo->SysInfo.DiskAvailableSpace = 1024;
	gpser1.pserinfo->SysInfo.DiskMAXSpace = 400;
	gpser1.pserinfo->ServerInfo.LanAddr = inet_addr("192.168.4.32");
	gpser1.pserinfo->ServerInfo.LanGateWay = inet_addr("192.168.0.32");
	gpser1.pserinfo->ServerInfo.LanNetmask = inet_addr("255.255.255.0");

	r_strcpy(gpser1.pserinfo->Authentication.password, "12345");
	r_strcpy(gpser1.pserinfo->Authentication.username, "xuchong");

	gpser1.pserinfo->ConfigInfo.iptype = 0;
	gpser1.pserinfo->ConfigInfo.language = 0;
	
	gpser = &gpser1;
#endif

	r_memset(plog_head, 0, 32);
	get_user_log_head(CONTROL_FOR_SERVER_PORT, 0, WebCtrl, plog_head, "127.0.0.1");

	system("mkdir -p /var/log/reach");
	printf("app_weblisten_init\n");
	if(flag == 1)
	{
		if(r_pthread_create(&webListen, NULL, weblisten, NULL))
		{
			printf("Failed to create web listen thread\n");
			return -1;
		}
	}
	printf("app_weblisten_init\n");
//	r_pthread_join(webListen,NULL);
	return 0;
}

