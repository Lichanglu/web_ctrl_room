#ifdef HAVE_RTSP_MODULE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include "mid_mutex.h"
#include "stream_output_struct.h"
#include "stream_output_common.h"

#include "rwini.h"
#include "log_common.h"







int app_ini_read_int(char *config_file, char *table, char *string, int *value)
{
	char  temp[512] = {0};
	int ret = 0;
	ret =  ConfigGetKey(config_file, table, string, temp);

	if(ret != 0) {
		PRINTF("Get [%s]:%s from %s failed\n", table, string, config_file);
		return -1;
	}


	*value = atoi(temp);

	if(*value == -1) {
		PRINTF("Get [%s]:%s from %s failed\n", table, string, config_file);
		return -1;
	}

	PRINTF("the [%s].%s value =%d,the temp=%s\n", table, string, *value, temp);
	return 0;
}



int app_ini_read_string(char *config_file, char *table, char *string, char *value, int len)
{
	char temp[512] = {0};
	int ret = 0;
	ret =  ConfigGetKey(config_file, table, string, temp);

	if(ret != 0) {
		PRINTF("Get [%s]:%s from %s failed\n", table, string, config_file);
		return -1;
	}

	snprintf(value, len, "%s", temp);
	PRINTF("the [%s].%s value =%s,the temp=%s\n", table, string, value, temp);
	return 0;
}

int app_ini_write_int(char *config_file, char *section, char *key, int value)
{
	char temp[512] = {0};
	int ret = 0;

	sprintf(temp, "%d", value);
	ret =  ConfigSetKey(config_file, section, key, temp);

	if(ret != 0) {
		PRINTF("set [%s]:%s to %s failed\n", section, key, config_file);
		return -1;
	}

	return 0;
}

int app_ini_write_string(char *config_file, char *section, char *key, char *value)
{
	char temp[512] = {0};
	int ret = 0;

	snprintf(temp, sizeof(temp), "%s", value);
	ret =  ConfigSetKey(config_file, section, key, temp);

	if(ret != 0) {
		PRINTF("set [%s]:%s to %s failed\n", section, key, config_file);
		return -1;
	}

	return 0;
}

#endif
