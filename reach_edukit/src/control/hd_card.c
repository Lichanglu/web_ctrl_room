/*
author :Lichl
funtion:与高清板进行通信
*/
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "common.h"
#include "control_log.h"
#include "xml_base.h"
#include "command_resolve.h"
#include "control.h"
#include "command_protocol.h"
#include "xml_msg_management.h"
#include "mid_http.h"
#include "params.h"
#include "timeout_process.h"
#include "web/weblisten.h"
#include <time.h>
#include "web/app_update_header.h"
#include "web/app_update.h"
#include "hd_card.h"


typedef struct _HDB_MSGHEAD
{
	short nLen;
	short nVer;
	char nMsg;
	char szTemp[3];
}HDB_MSGHEAD;

#define JPG_HEIGHT   64
#define JPG_WIDTH    64
#define JEPG_FILENEMA  ("/opt/dvr_rdk/ti816x_2.8/logo.jpg")

#define	HD_CON_PORT	12131

typedef struct _Version_Info {
	char app_version[32];
	char kernel_version[32];
	char fpga_version[32];
	char built_time[64];
} Version_Info;

static Version_Info hd_card_version;
static int ctrl_msg_code=0;
static char ctrl_msg_data[128]={0};


extern int select_socket(int socket, int secode);

static int connect_HD_card()
{
	struct sockaddr_in serv_addr;
	const char* pAddr  =HD_ENC_IP;
	int socketFd;
	int flags=0;
	socketFd= socket(PF_INET, SOCK_STREAM, 0);
	if(socketFd < 1)
		return -1;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(HD_CON_PORT);
	inet_aton(pAddr,(struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero),8);

	if(connect(socketFd, (struct sockaddr *)&serv_addr,sizeof(struct sockaddr)) == -1)	{
		lprintf("connect   error!!!errno=%d [%s]\n",errno,strerror(errno));
		close(socketFd);
		return -1;
	}

	//直接返回
	return socketFd;
}

int web_get_hd_version(char* app_version,char* fpga,char *ker,char* build_time)
{
	snprintf(app_version,32,"%s",hd_card_version.app_version);
	snprintf(fpga,32,"%s",hd_card_version.fpga_version);
	snprintf(ker,32,"%s",hd_card_version.kernel_version+8);
	snprintf(build_time,64,"%s",hd_card_version.built_time);
	lprintf("version=%s---fpga=%s---ker=%s---time=%s\n",app_version,fpga,ker,build_time);
	return 0;
}

static int32_t ctrl_get_system_info(void)
{
	HDB_MSGHEAD msgHead = {0};
	int socketFd =  connect_HD_card();
	Version_Info hd_version;
	int len=0;
	if(socketFd < 0)
	{
		lprintf("sys info Connect fail!!\n");
		return SERVER_SYSUPGRADE_FAILED;
	}
	msgHead.nMsg = 0x14;
	msgHead.nVer = 1;
	msgHead.nLen = htons(sizeof(HDB_MSGHEAD)+4);
	lprintf("send befor\n");
	tcp_send_longdata(socketFd, (int8_t *)&msgHead, sizeof(HDB_MSGHEAD));
	lprintf("1111\n");


	int select_ret = 0;
	select_ret = select_socket(socketFd, 6);
	if (select_ret == 0) {
		lprintf("select_socket time out error!!![%s]\n", strerror(errno));
		return -1;
	} else if (select_ret < 0) {
		lprintf("select_socket error!!![%s]\n", strerror(errno));
		return -1;
	}

	memset(&msgHead,0,sizeof(msgHead));
	tcp_recv_longdata(socketFd,&msgHead, sizeof(HDB_MSGHEAD));
	len = ntohs( msgHead.nLen);
	lprintf("len=%d\n",len);
	if(msgHead.nMsg != HD_CARD_RETURN_SUCCESS ||  sizeof(Version_Info) !=  len -sizeof(HDB_MSGHEAD)){
		lprintf("get version failed!\n");
		r_close(socketFd);
		return -1;
	}
	tcp_recv_longdata(socketFd,&hd_version,len-sizeof(HDB_MSGHEAD));
	lprintf("^^send version to Control: \n version=%s \n  ker=%s \n built_time=%s,\nfpga_version=%s\n",hd_version.app_version,hd_version.kernel_version,hd_version.built_time,hd_version.fpga_version);
	memcpy(&hd_card_version,&hd_version,sizeof(Version_Info));
	r_close(socketFd);
	return 1;
}

int app_reboot_HD_card(void)
{
	int socketFd = connect_HD_card();
	HDB_MSGHEAD msgHead = {0};

	msgHead.nVer = 1;
	msgHead.nLen = htons(sizeof(HDB_MSGHEAD));
	msgHead.nMsg= CTRL_GET_HD_REBOOT_MSG;
	tcp_send_longdata(socketFd, (int8_t *)&msgHead, sizeof(HDB_MSGHEAD));
	r_close(socketFd);
	return 0;
}
extern int  hd_card_update_flag;
int app_update_HD_card(int8_t *filename)
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
		hd_card_update_flag=2;
		lprintf("app_update_HD_card Open fail!!\n");
		return 0;
	}
#if 1
	fread(buffer, 1, 8 , file);

	if(!((buffer[0] == 0x7e)&&(buffer[1] == 0x7e)&&(buffer[2] == 0x7e) && (buffer[3] == 0x7e)
							&& (buffer[4] == 0x48) && (buffer[5] == 0x45) && (buffer[6] == 0x4e) && (buffer[7] == 0x43)))
	{
		hd_card_update_flag=2;
		lprintf("update formt fail!!\n");
		return 1;
	}
#endif
	socketFd =  connect_HD_card();

	if(socketFd < 0)
	{
		hd_card_update_flag=2;
		lprintf("update Connect fail!!\n");
		fclose(file);
		return -1;
	}

	fseek(file,0L,SEEK_END);
	filelen = ftell(file);
	filelen = filelen - 8;

	fseek(file, 8L, SEEK_SET);

	msgHead.nMsg = CTRL_UPDATE_HD_CARD_MSG;
	msgHead.nVer = 1;
	msgHead.nLen = htons(sizeof(HDB_MSGHEAD)+4);

	tcp_send_longdata(socketFd, (int8_t *)&msgHead, sizeof(HDB_MSGHEAD));

	lprintf("Start Upgrade len [%u]\n",filelen);
	tcp_send_longdata(socketFd, (int8_t *)&filelen, sizeof(int));
	lprintf(" al!!!");
	while(1)
	{
		int n = 0;
		n = fread(buffer, 1, 1024 , file);
		if(n <= 0){
			break;
		}
		tcp_send_longdata(socketFd, (int8_t *)buffer, n);
	}
	lprintf("send finished!\n");
	fclose(file);

	unlink((char *)filename);

	int select_ret = 0;

	select_ret = select_socket(socketFd, 6);
	if (select_ret == 0) {
		lprintf("select_socket time out error!!![%s]\n", strerror(errno));
		hd_card_update_flag=2;
		return -1;
	} else if (select_ret < 0) {
		lprintf("select_socket error!!![%s]\n", strerror(errno));
		hd_card_update_flag=2;
		return -1;
	}

	tcp_recv_longdata(socketFd, (int8_t *)&msgHead, sizeof(HDB_MSGHEAD));
	if(msgHead.nMsg == HD_CARD_RETURN_FAIL)
	{
		printf("update is fail!!!\n");
		r_close(socketFd);
		hd_card_update_flag=2;
		return -1;
	}
	else if(msgHead.nMsg == HD_CARD_RETURN_SUCCESS)
	{
		char com[128]={0};
		snprintf(com,128,"mv %s %s",HD_EDUKIT_PACKET,HD_REBACK_PATH);
		system(com);
		lprintf("hd update is success!!!\n");
		r_close(socketFd);
		hd_card_update_flag=2;
		return 1;
	}
	else
	{
		lprintf("msg=%d   unkown error!!!\n",msgHead.nMsg);
		r_close(socketFd);
		hd_card_update_flag=2;
		return -1;
	}
	r_close(socketFd);
	hd_card_update_flag=2;
	return -1;
}

static int32_t xml_process_upload_logo(int sockfd)
{
	int i=0;
	char buf[1500];
	int len=0;
	int ret=0;
	int total=0;
	FILE *fd = fopen(JEPG_FILENEMA,"r+");
//	XML_ASSERT(fd!=NULL);
	while( len = fread(buf,1,1300,fd) >0){
		ret = send(sockfd,buf,len,0);
		//XML_ASSERT(ret >= 0);
		total+=len;
	}
	fclose(fd);
	lprintf("ret=%d,total_len=%d",ret,total);
	return 0;
}

int web_process_upload_logo(char *indata)
{
	int ret = 1;
	int len=0;
	int sockfd=connect_HD_card();
	char buf[JPG_HEIGHT*JPG_WIDTH*3];
	char com[256] = {0};
	int filelen=0;
	char filename[256] = {0};
	char buffer[1500]={0};
	HDB_MSGHEAD msgHead;

	system("sync");
	snprintf(com,256,"mv %s %s", indata,JEPG_FILENEMA);
	system(com);
	lprintf("success change logo file. com=%s\n", com);


	FILE* file = fopen(JEPG_FILENEMA,"rb+");
	//XML_ASSERT(file!=NULL);


	fseek(file,0L,SEEK_END);
	filelen = ftell(file);
//	filelen = filelen - 8;

	fseek(file, 0L, SEEK_SET);

	msgHead.nMsg = CTRL_UPLOGO_HD_CARD_MSG;
	msgHead.nVer = 1;
	msgHead.nLen = htons(sizeof(HDB_MSGHEAD)+4);

	tcp_send_longdata(sockfd, (int8_t *)&msgHead, sizeof(HDB_MSGHEAD));

	lprintf("Start Upgrade len [%u]\n",filelen);
	tcp_send_longdata(sockfd, (int8_t *)&filelen, sizeof(int));

	while(1)
	{
		int n = 0;
		n = fread(buffer, 1, 1024 , file);
		if(n <= 0){
			break;
		}
		tcp_send_longdata(sockfd, (int8_t *)buffer, n);
	}
	lprintf("send finished!\n");
	fclose(file);

	unlink((char *)filename);

	int select_ret = 0;

	select_ret = select_socket(sockfd, 6);
	if (select_ret == 0) {
		lprintf("select_socket time out error!!![%s]\n", strerror(errno));
		return -1;
	} else if (select_ret < 0) {
		lprintf("select_socket error!!![%s]\n", strerror(errno));
		return -1;
	}

	tcp_recv_longdata(sockfd, (int8_t *)&msgHead, sizeof(HDB_MSGHEAD));
	if(msgHead.nMsg == HD_CARD_RETURN_FAIL)
	{
		lprintf("app_update_HD_card is fail!!!\n");
		r_close(sockfd);
		return 1;
	}
	else if(msgHead.nMsg == HD_CARD_RETURN_SUCCESS)
	{
		lprintf("app_update_HD_card is success!!!\n");
		r_close(sockfd);
		return 0;
	}
	else
	{
		lprintf("msg=%d   unkown error!!!\n",msgHead.nMsg);
		r_close(sockfd);
		return 1;
	}
	r_close(sockfd);
	return ret;
}


int ctrl_send_msg_hd_card(int msg_code,void* data,int data_len)
{
	ctrl_msg_code = msg_code;
	if(data != NULL ){
		memcpy(ctrl_msg_data,data,data_len);
	}else{
		memset(ctrl_msg_data,0,sizeof(128));
	}
}

void*  hd_card_pthread(void)
{
	int sockfd=0;
	int verget=1;
	while(1){
		switch( ctrl_msg_code ){
		case CTRL_UPDATE_HD_CARD_MSG:
			app_update_HD_card(HD_EDUKIT_PACKET);
			break;
		case CTRL_GET_HD_SYSINFO_MSG:			
			ctrl_get_system_info();
			break;
		case CTRL_GET_HD_REBOOT_MSG:
			app_reboot_HD_card();
			break;
		case CTRL_UPLOGO_HD_CARD_MSG:
			break;	
		case CTRL_SINGLE_UPDATE_HD_MSG:
			app_update_HD_card(ctrl_msg_data);
			break;
		defualt:
			break;
		}
		memset(ctrl_msg_data,0,128);
		ctrl_msg_code=0;
		sleep(1);
		if( 1 == verget){
			sleep(3);
			ctrl_get_system_info();
			verget=0;
		}
	}
}



