#ifndef __MID_HTTP_FUN_H__
#define __MID_HTTP_FUN_H__
#include "nslog.h"
#define HTTP_HEAD_LEN_MAX  2440
#define HTTP_BODY_LEN_MAX  10000

#define HTTP_VERSION_1     "HTTP/1.0"
#define HTTP_VERSION_2		 "HTTP/1.1"
#define HTTP_VERSION_LEN   8
#define HTTP_OK            200
#define HTTP_POST_TIME      15

struct HTTP_mid
{
	int buf_max_len;
	int *out_buf_len;
	char *out_buf;
};


int mid_http_post(char *url, char *info, int inlen, char *output, int *outlen);
int mid_http_get(char *url, char *info, int inlen, char *output, int *outlen);

#endif

