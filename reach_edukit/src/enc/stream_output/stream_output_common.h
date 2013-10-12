#ifdef HAVE_RTSP_MODULE


#ifndef _STREAM_OUTPUT_COMMON_H__
#define _STREAM_OUTPUT_COMMON_H__

int app_ini_read_int(char *config_file, char *table, char *string, int *value);
int app_ini_read_string(char *config_file, char *table, char *string, char *value, int len);
int app_ini_write_int(char *config_file, char *section, char *key, int value);
int app_ini_write_string(char *config_file, char *section, char *key, char *value);
int mid_ip_is_multicast(char *ip);

#endif

#endif