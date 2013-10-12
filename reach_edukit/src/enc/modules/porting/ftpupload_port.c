#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <log_common.h>
#include "ftp_upload.h"


#define ERR_NOFILE      -10
#define SEC_NOT_FOUND	-1
#define KEY_NOT_FOUND	-2


extern int set_ftp_upload_flag(int flag);
extern int get_ftp_upload_flag(void);
extern int set_ftp_size(int size);
extern int ConfigGetKey(void *CFG_file, void *section, void *key, void *buf);
extern int ConfigSetKey(void *CFG_file, void *section, void *key, void *buf);
extern int read_ftp_ini(char *url, char *ip, int times);

int get_url_ip(char *url, char *ip);
void app_ftp_init(void);
int user_get_url_ip(char *url, char *ip);
int user_get_ftp_ini_url(char *url, int len);
int user_get_ftp_ini_times(char *buf, int len);
int user_save_ftp_times(char *buf, int inlen);
int user_save_ftp_url(char *buf, int inlen);
int user_get_upload_flag(void);
int user_set_upload_flag(int flag);
int user_set_ftp_size(int size);
int user_ftp_open(char *url);
int user_ftp_close(void);


int user_set_upload_flag(int flag)
{
	if(flag == 0 || flag == 1) {
		return set_ftp_upload_flag(flag);
	} else {
		return -1;
	}
}

int user_get_upload_flag(void)
{
	return get_ftp_upload_flag();
}

void app_ftp_init(void)
{
	int ret, times = 0;
	char url[PATH_SIZE] = {0}, ip[PATH_SIZE] = {0}, buf[PATH_SIZE] = {0};
	ret = ConfigGetKey(FTPPATH, "common", "url", url);

	if(ret < 0) {
		ERR_PRN("INI url error!\n");
		return;
	}

	ret = user_get_url_ip(url, ip);

	if(ret < 0) {
		ERR_PRN("url error!");
		return;
	}

	ret = ConfigGetKey(FTPPATH, "common", "times", buf);

	if(ret < 0) {
		ERR_PRN("ConfigGetKey error!");
		return;
	}

	times = atoi(buf);

	if(times > 0) {
		memset(buf, 0, PATH_SIZE);

		if(times <= 1) {
			sprintf(buf, "0");
		} else {
			sprintf(buf, "%d", (times - 1));
		}

		ret = ConfigSetKey(FTPPATH, "common", "times", buf);

		if(ret < 0) {
			ERR_PRN("write times error!\n");
		}

		read_ftp_ini(url, ip, times);
		user_set_upload_flag(1);
	} else {
		times = 0;
		return ;
	}

	return;
}


//从正规的url里提取url的ip地址存入ip中。正规ftp地址形如ftp://user:passwd@ip/目录
int user_get_url_ip(char *url, char *ip)
{
	char *p = NULL, *q = NULL;
	char tmp[PATH_SIZE] = {0};

	if(url == NULL) {
		return -1;
	}

	memcpy(tmp, url, strlen(url));
	p = strchr(tmp, '@');

	if(p == NULL) {
		ERR_PRN("ini ip error!use default\n");
		return -1;
	} else {
		q = strchr(p + 1, '/');

		if(q == NULL && tmp[strlen(tmp) - 1] != '/') {
			strcpy(ip, p + 1);
		} else if(q != NULL) {
			*q = 0;
			strcpy(ip, p + 1);
		} else {
			ERR_PRN("ini ip error!use default\n");
			return -1;
		}
	}

	return 0;
}


int user_get_ftp_ini_url(char *buf, int len)
{
	int ret = 0;

	if(buf == NULL) {
		buf = (char *)malloc(PATH_SIZE);

		if(buf == NULL) {
			return -2;
		}

		memset(buf, 0, PATH_SIZE);
	}

	memset(buf, 0, len);
	ret = ConfigGetKey(FTPPATH, "common", "url", buf);

	if(ret < 0) {
		ERR_PRN("ConfigGetKey error!\n");
		return -1;
	}

	return 0;
}


int user_get_ftp_ini_times(char *buf, int len)
{
	int ret = 0;

	if(buf == NULL) {
		buf = (char *)malloc(PATH_SIZE);

		if(buf == NULL) {
			return -2;
		}

		memset(buf, 0, PATH_SIZE);
	}

	memset(buf, 0, len);
	ret = ConfigGetKey(FTPPATH, "common", "times", buf);

	if(ret < 0) {
		ERR_PRN("ConfigGetKey error!\n");
		return -1;
	}

	return 0;
}

int user_save_ftp_times(char *buf, int inlen)
{
	int ret = 0;

	if(buf == NULL) {
		return -1;
	}

	ret = ConfigSetKey(FTPPATH, "common", "times", buf);

	if(ret < 0) {
		ERR_PRN("ConfigGetKey error!\n");
		return -1;
	}

	return 0;
}

int user_save_ftp_url(char *buf, int inlen)
{
	int ret = 0;

	if(buf == NULL) {
		return -1;
	}

	if(buf[strlen(buf) - 1] != '/') {
		buf[strlen(buf)] = '/';
		buf[strlen(buf) + 1] = 0;
	}

	ret = ConfigSetKey(FTPPATH, "common", "url", buf);

	if(ret < 0) {
		ERR_PRN("ConfigGetKey error!\n");
	}

	return 0;
}

int user_ftp_open(char *url)
{
	char ip[PATH_SIZE] = {0};

	if(url == NULL || strncmp(url, "ftp:", 4)) {
		return -1;
	}

	user_save_ftp_url(url, strlen(url));
	user_get_url_ip(url, ip);
	user_save_ftp_times(DEFAULT_TIME, sizeof(DEFAULT_TIME));
	set_ftp_url_ip(url, ip);
	user_set_upload_flag(1);
	return 0;
}

int user_ftp_close(void)
{
	int ret = 0;

	if(user_get_upload_flag()) {
		ret = user_set_upload_flag(0);

		if(ret < 0) {
			return -1;
		}
	} else {
		return -1;
	}

	return 0;
}

int user_set_ftp_size(int size)
{
	return set_ftp_size(size);
}

