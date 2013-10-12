/*****************************************************************
* 用来校验用户和密码是否正确
*
******************************************************************/

//add by zhangmin
#ifdef GDM_RTSP_SERVER


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <windows.h>
#include "rtsp_authentication.h"

#include "mid_platform.h"

static char  g_user_string[64] = {"viewer"};
static char  g_passwd_string[64] = {0};

static int app_authentication_test(char *usr, char *passwd);





static int app_authentication_test(char *usr, char *passwd)
{
	//mutex
	//unmutex
	if(strcmp(usr, "szreach") != 0) {
		return -1;
	}

	if(strcmp(passwd, "szreach") != 0) {
		return -1;
	}

	return 0;
}





static int app_build_rand_passwd(char *buff)
{
	int r_num = 0;
	int l_num = 0;
	//set rand
	unsigned long ttime = mid_plat_get_runtime();
	srand((int)ttime);
	r_num = rand() % (32767 - 10000 + 1) + 10000;
	l_num = rand() % (32767 - 11111 + 1) + 11111;

	sprintf(buff, "%d%d", l_num, r_num);
	buff[10] = '\0';
	//	PRINTF("l_num = %d,r_num=%d,buff= #%s#\n",l_num,r_num,buff);
	return 0;
}

static int app_authentication_save_file(char *section, char *key, char *value, char *filename)
{
	//return -1;
	if(section == NULL || key == NULL || value == NULL || filename == NULL) {
		return -1;
	}

	WritePrivateProfileString(section, key, value, filename);
	return 0;
}
//停止user/passwd
int app_authentication_stop(void)
{
	//mutex
	memset(g_passwd_string, 0, sizeof(g_passwd_string));

	app_authentication_save_file("control", "status", "0", USER_FILE_NAME);
	app_authentication_save_file("control", "user", "viewer", USER_FILE_NAME);
	app_authentication_save_file("control", "passwd", "0000000000", USER_FILE_NAME);
	//unmutex
	return 0;
}

//开始user/passwd
int app_authentication_start(void)
{
	//mutex
	app_build_rand_passwd(g_passwd_string);
	app_authentication_save_file("control", "status", "1", USER_FILE_NAME);
	app_authentication_save_file("control", "user", "viewer", USER_FILE_NAME);
	app_authentication_save_file("control", "passwd", g_passwd_string, USER_FILE_NAME);

	return 0;
	//unmutex
}

int app_authentication_get_passwd(char *buff)
{
	strcpy(buff, g_passwd_string);
	return 0;
}

int app_authentication_get_user(char *buff)
{
	strcpy(buff, g_user_string);
	return 0;
}

int app_authentication(char *usr, char *passwd)
{

	//	return app_authentication_test(usr,passwd);
	//mutex
	//unmutex
	if(strcmp(usr, g_user_string) != 0) {
		return -1;
	}

	if(strcmp(passwd, g_passwd_string) != 0) {
		return -1;
	}

	return 0;
}

// 2 begin ,1 failed,3 表示正在中
int app_rtsp_authentication_begin(char *usr)
{
	int value = 0;
	char filename[256] = {0};
	sprintf(filename, "%s\\%s.ini", RTSP_DIR_NAME, usr);
	value = GetPrivateProfileInt("user", "status", 1, filename);
	PRINTF("ERROR,the file is %s,the value = %d\n", filename, value);

	if(value == 1) {
		return -1;
	} else if(value == 3) {
		PRINTF("Warnning,the url is now playing\n");
		return -1;
	} else if(value == 2) {
		app_authentication_save_file("user", "status", "3", filename);
		return 0;
	}

}


int app_rtsp_authentication_stop(char *usr)
{
	if(strlen(usr) == 0) {
		return -1;
	}

	int ret = 0;
	char filename[256] = {0};
	sprintf(filename, "%s\\%s.ini", RTSP_DIR_NAME, usr);
	ret = DeleteFile(filename);
	PRINTF("Deletefile name = %s\n", filename, ret);

	return 0;
}

//delele dir
int app_rtsp_authentication_clear()
{
	char  command[512] = {0};
	sprintf(command, "del %s\\*.ini", RTSP_DIR_NAME);
	PRINTF("I will 1command ,the command = %s\n", command);
	system(command);

	sprintf(command, "md %s", RTSP_DIR_NAME);
	PRINTF("I will 2command ,the command = %s\n", command);
	system(command);

	app_authentication_stop();
	return 0;
}
#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rtsp_authentication.h"

int app_authentication(char *usr, char *passwd)
{
	if(strcmp(usr, "szreach") != 0) {
		return -1;
	}

	if(strcmp(passwd, "szreach") != 0) {
		return -1;
	}

	return 0;
}


#endif

