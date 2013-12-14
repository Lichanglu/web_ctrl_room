/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2012年11月1日 09时12分18秒
 *       Revision:
 *       Compiler:  gcc
 *
 *         Author:  黄海洪
 *        Company:  深圳锐取信息技术股份有限公司
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#include "control.h"
#include "command_resolve.h"
#include "common.h"
#include "control_log.h"
#include "ftpcom.h"
#include "./web/weblisten.h"
#include "mount.h"
//#include "lcd.h"
#include "cli_verify.h"
#include "curl.h"



#define SERVER_FTP_UPLOAD_PROGRAME_NAME			("/usr/local/reach/ftpserver &")

#define ROOM_CLIENT_PROGRAME_NAME				("/usr/local/reach/room")
#define ROOM_CLIENT_CONFIG_FILE					("/usr/local/reach/room")
#define ROOM_CLIENT_PROGRAME_CONFIG_NAME_IPADDR	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<RoomCfg>\n\t<IpAddr>%s</IpAddr>\n\t<Port>%d</Port>\n</RoomCfg>")
#define ROOM_CLIENT_PROGRAME_CONFIG_NAME		("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<RoomCfg>\n\t<IpAddr>127.0.0.1</IpAddr>\n\t<Port>%d</Port>\n</RoomCfg>")



server_set *gpser = NULL;

uint32_t g_spec_data = 0;

int32_t gpio_fd = -1;

#if 0
int32_t create_room_config_file()
{
	FILE *fp = NULL;

	fp = fopen((char *)def_log_conf, "w+");
	if(fp == NULL){
		fprintf(stderr, "create_default_log_conf failed!\n");
		return -1;
	}

	fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(fp, "<RoomCfg>");
	fprintf(fp, "<IpAddr>%s</IpAddr>\n", ipaddr);
	fprintf(fp, "<Port>%d</Port>\n", port);
	fprintf(fp, "</RoomCfg>");
	fclose(fp);

	return 0;
}
#endif

int32_t create_room_client_config_file(int32_t port, int8_t *ipaddr, int32_t index)
{
	int8_t filename[256] = {0};
	int8_t text[2048] = {0};

	sprintf((char *)filename, "%s_%d.xml", ROOM_CLIENT_CONFIG_FILE, index);
	unlink((const char *)filename);

	if(NULL == ipaddr)
		sprintf((char *)text, ROOM_CLIENT_PROGRAME_CONFIG_NAME, port);
	else
		sprintf((char *)text, ROOM_CLIENT_PROGRAME_CONFIG_NAME_IPADDR, ipaddr, port);

	FILE *fp = fopen((char *)filename, "w+");
	fwrite(text, r_strlen(text), 1, fp);
	fclose(fp);

	return 0;
}

int32_t start_control_server()
{
	/* TODO: 需添加程序自救 */

	control_env *pcon = NULL;
	http_env *phttp = NULL;
	int32_t index = 0;
	int32_t ret = 0;
//	int8_t cmd[128] = {0};

	setpriority(PRIO_PROCESS, 0, -20);

	ret = start_control_server_task(&gpser);
	if(ret != 0){
		zlog_error(DBGLOG, "---[start_control_server_task]--- failed!\n");
		return -1;
	}

	ret = start_ftpupload_com_task(gpser);
	if(ret != 0){
		zlog_error(DBGLOG, "---[start_ftpupload_com_task]--- failed!\n");
		return -1;
	}
	ftpcom_set_running(&gpser->ftpser);

	pcon = &gpser->forser;
	control_set_running(pcon);
	for(index = 0; index < gpser->pserinfo->ServerInfo.MaxRoom; index++){
		pcon = &gpser->roomser[index];
		control_set_running(pcon);
	}
	phttp = &gpser->http;
	http_set_running(phttp);
	app_weblisten_init(gpser);
	sleep(2);
	return 0;
}

int32_t close_control_server()
{
	control_env *pcon = NULL;
	http_env *phttp = NULL;
	all_server_info *pinfo = NULL;

	int32_t index = 0;
	int32_t j = 0;

	phttp = &gpser->http;
	http_set_stop(phttp);

	sleep(6);

	pcon = &gpser->forser;
	control_set_stop(pcon);

	for(index = 0; index < gpser->pserinfo->ServerInfo.MaxRoom; index++){
		pcon = &gpser->roomser[index];
		control_set_stop(pcon);

		for(j = 0; j < pcon->max_user_num; j++){
			if(pcon->user[j].tcp_sock > 0)
				close(pcon->user[j].tcp_sock);
		}
		close(pcon->ser_socket);
	}

	sleep(4);

	for(index = 0; index < pcon->max_user_num; index++){
		if(pcon->user[index].tcp_sock > 0)
			close(pcon->user[index].tcp_sock);
	}
	close(pcon->ser_socket);

	sleep(12);
	pinfo = gpser->pserinfo;

	if (pinfo->parse_xml_ftp->pdoc)
		r_free(pinfo->parse_xml_ftp->pdoc);

	if (pinfo->parse_xml_ftp)
		r_free(pinfo->parse_xml_ftp);

	if(pinfo)
		r_free(pinfo);
	r_free(gpser);

	return 1;
}


int32_t start_room_server(server_set *gpser)
{
	int32_t index = 0;
	int32_t buf[16] = {0};

	if(NULL == gpser){
		zlog_error(DBGLOG, "---[start_room_server]--- failed, params is NULL\n");
		return -1;
	}

	/* 启动各个会议室 */
	for(index = 0; index < gpser->pserinfo->ServerInfo.MaxRoom; index++){
		r_memset(buf, 0, 16);
		sprintf((char *)buf, "%d", index);
//		execl(SERVER_ROOM_PROGRAME_PATH, SERVER_ROOM_PROGRAME_NAME, (char *const *)buf, (char *const *)0);
	}

	return 0;
}

extern void init_led_tsk();

int32_t main(int argc, char **argv)
{
	printf("********control********\n");
	int32_t ret = 0;
//	int8_t *privd = NULL;
	int8_t ipbuf[64]= {0};
	uint32_t max_room = 0;
	uint32_t sync_count = 0;
	localtime_t t;

	Signal(SIGPIPE, SIG_IGN);
	Signal(SIGINT, SIG_IGN);

	xmlInitParser();
	curl_global_init(CURL_GLOBAL_ALL);

	/* 初始化日志模块 */
	ret = control_log_init();
	if(ret < 0){
		fprintf(stderr, "---[control_log_init]--- failed!\n");
		xmlCleanupParser();
		return -1;
	}

#if 0
	int wfd = init_watch_dog(10);
	while(1){
	       write_watch_dog(wfd);
	       sleep(1);
	       fprintf(stderr, "write dog!\n");
	}
#endif

	g_spec_data = max_room;
	g_spec_data = 1;


	gpio_fd = open_gpio_device();


	start_control_server();


	init_led_tsk();

    //printf("***************RegisterNetControlTask******************\n");
	RegDiskDetectMoudle();
    RegisterNetControlTask();



	//sleep(5);
	//start_lcd_display_task();
#ifdef SERIAL_CONTROL
	RegisterrSerialControlTask();
#endif
	/* FIXME: 未作退出 */
	while(1){
		zlog_info(DBGLOG, "control process is running!\n");
		sleep(50);
		get_localtime(&t);
		if(0==t.tm_hour && 0==t.tm_min){
			/* 00:00 时开始同步操作 */
			ret = -1;
			sync_count = 10;
			while(0 != ret){
				sync_count--;
				ret = check_all_record_status(gpser);
				if(1 != ret){
					/* 没有录制 */
					pthread_mutex_lock(&gpser->pserinfo->info_m);
					r_strcpy(ipbuf, gpser->pserinfo->HBeatInfo.time_serip);
					pthread_mutex_unlock(&gpser->pserinfo->info_m);
					ret = ntp_time_sync(ipbuf);
					if(0==ret)
						break;
				}
				/* FIXME:使用微循环? */
				sleep(1800);
				if(0==sync_count){
					/* 持续至凌晨5:00尚未有同步的机会，则放弃同步 */
					break;
				}
			}
		}
	}

	/* 以下仅为内存测试 */
#if 0
	close_control_server();
	sleep(12);
	control_log_deinit();
	fprintf(stderr, "control_log_deinit---------------!\n");
	xmlCleanupParser();
#endif

	return 0;
}


