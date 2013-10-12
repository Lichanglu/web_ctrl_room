#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include "rtelnet.h"
#include "ftp_upload.h"

#include "log_common.h"

//#include "sysparam.h"
//#include "common.h"



#define MAX 20
#define CMD_SIZE 108
#define OSIZE           1024   //out buffer

//error ret
#define ENOFIND  -5
#define ENOPARAM  -6    // wrong PARAM
#define EMALLOC		-7
#define SSIZE (1024 * 20)

#define print_line  PRINTF("%d\n", __LINE__);

typedef struct cmd_t {
	char cmd[CMD_SIZE];
	int (*func)(char *in, int inlen, char *out, int outlen);
} CMD_T;

//extern unsigned int g_IframeInterval[MAX_VAL];

extern void rtelnet_uninit(void);
extern int trace_get_debug_level(void);
extern int trace_set_debug_level(int level);
extern int user_get_ftp_ini_url(char *url, int inlen);
extern int user_get_ftp_ini_times(char *buf, int inlen);
extern int user_save_ftp_url(char *url, int inlen);
extern int user_save_ftp_times(char *buf, int inlen);
extern int user_ftp_open(char *url);
extern int user_ftp_close(void);

//extern unsigned int GetCtrlFrame(void);
//extern params_table gSysParaT;

static int get_func(char *in, int inlen, char *out, int outlen);
static int set_func(char *in, int inlen, char *out, int outlen);
static int save_func(char *in, int inlen, char *out, int outlen);

static int get_debug_level_func(char *in, int inlen, char *out, int outlen);
static int get_big_video_info_func(char *in, int inlen, char *out, int outlen);
static int get_audio_info_func(char *in, int inlen, char *out, int outlen);
static int get_ip_info(char *in, int inlen, char *out, int outlen);
static int get_far_ctrl_index_func(char *in, int inlen, char *out, int outlen);
static int get_ts_open_func(char *in, int inlen, char *out, int outlen);
static int get_high_gop_func(char *in, int inlen, char *out, int outlen);
static int get_low_gop_func(char *in, int inlen, char *out, int outlen);
static int get_video_profile_func(char *in, int inlen, char *out, int outlen);
static int get_input_info_func(char *in, int inlen, char *out, int outlen);
static int get_ftp_info_func(char *in, int inlen, char *out, int outlen);


static int set_debug_level_func(char *in, int inlen, char *out, int outlen);
static int set_far_ctrl_index_func(char *in, int inlen, char *out, int outlen);
static int	set_audio_samplerate_func(char *in, int inlen, char *out, int outlen);
static int	set_device_name_func(char *in, int inlen, char *out, int outlen);
static int set_ts_open_func(char *in, int inlen, char *out, int outlen);
static int set_high_gop_func(char *in, int inlen, char *out, int outlen);
static int set_low_gop_func(char *in, int inlen, char *out, int outlen);
static int set_video_profile_func(char *in, int inlen, char *out, int outlen);

static int set_ftp_open_func(char *in, int inlen, char *out, int outlen);
static int set_ftp_close_func(char *in, int inlen, char *out, int outlen);
static int set_ftp_size_func(char *in, int inlen, char *out, int outlen);


static int save_device_name_func(char *in, int inlen, char *out, int outlen);

int rtelnet_port_cmd(int type, char *in, int inlen, char *out, int outlen);


static CMD_T cmd_get_t[] = {
	{"get_debug_level", get_debug_level_func},
	{"get_big_video_info", get_big_video_info_func},
	{"get_audio_info", get_audio_info_func},
	{"get_ip_info", get_ip_info},
	{"get_far_ctrl_index", get_far_ctrl_index_func},
	{"get_ts_open", get_ts_open_func},
	{"get_high_gop", get_high_gop_func},
	{"get_low_gop", get_low_gop_func},
	{"get_video_profile", get_video_profile_func},
	{"get_input_info", get_input_info_func},
	{"get_ftp_info", get_ftp_info_func},
	//add get function
	{"get_max", NULL}
};

static CMD_T cmd_set_t[] = {
	{"set_debug_level", set_debug_level_func},
	{"set_far_ctrl_index", set_far_ctrl_index_func},
	{"set_audio_samplerate", set_audio_samplerate_func},
	{"set_device_name", set_device_name_func},
	{"set_ts_open", set_ts_open_func},
	{"set_high_gop", set_high_gop_func},
	{"set_low_gop", set_low_gop_func},
	{"set_video_profile", set_video_profile_func},
	{"set_ftp_open", set_ftp_open_func},
	{"set_ftp_close", set_ftp_close_func},
	{"set_ftp_size", set_ftp_size_func},
	//add set function
	{"set_max", NULL}
};

static CMD_T cmd_save_t[] = {
	{"save_device_name", save_device_name_func},
};


static int set_debug_level_func(char *in, int inlen, char *out, int outlen)
{
	int ret;

	if(!(ret = atoi(in)) && in[1] != '0') {
		outlen = ENOPARAM;
	} else {
		if(ret >= 0 && ret <= 4) {
			trace_set_debug_level(ret);
			sprintf(out, "level = %d", ret);
			outlen = strlen(out);
		} else {
			outlen = ENOPARAM;
		}
	}

	return outlen;
}

static int get_debug_level_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}

static int get_big_video_info_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}

static int get_audio_info_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}

static int get_ip_info(char *in, int inlen, char *out, int outlen)
{

	return 0;
}

static int get_far_ctrl_index_func(char *in, int inlen, char *out, int outlen)
{

	return 0;
}

static int get_ts_open_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}

static int get_high_gop_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}


static int get_low_gop_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}

static int get_video_profile_func(char *in, int inlen, char *out, int outlen)
{


	return 0;
}

static int get_input_info_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}

static int get_ftp_info_func(char *in, int inlen, char *out, int outlen)
{
	int ret = 0;
	char buf[OSIZE] = {0}, tmp[OSIZE] = {0};
	ret = user_get_ftp_ini_url(buf, OSIZE);

	if(ret < 0) {
		snprintf(out, outlen, "get ftp url failed\n");
		return -1;
	}

	ret = user_get_ftp_ini_times(tmp, OSIZE);

	if(ret < 0) {
		snprintf(out, outlen, "get ftp times failed\n");
		return -1;
	}

	snprintf(out, outlen, "The ftp url is %s, the ftp times is %s", buf, tmp);
	return 0;
}





static int set_far_ctrl_index_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}

static int	set_audio_samplerate_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}
static int	set_device_name_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}

static int save_device_name_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}



static int set_ts_open_func(char *in, int inlen, char *out, int outlen)
{
	char *p = NULL;
	int  val = 0;
	int flag = 0;
	p = strrchr(in, ' ');

	if(p == NULL) {
		PRINTF("%s,%d,the string  is not space char \n", __func__, __LINE__);
		return ENOPARAM;
	}

	PRINTF("%s,%d,p=%s\n", __func__, __LINE__, p);

	val = atoi(p);

	if(val == 0 || val == 1) {
		//	flag = saveStreamValue(PROTOCOL_NAME,val);
		if(flag != 0) {
			PRINTF("%s,%d,set stream protocol status failed\n", __func__, __LINE__);
			return ENOFIND;
		}
	}

	PRINTF("%s,%d,set stream protocol status success\n", __func__, __LINE__);
	return 0;
}



static int set_high_gop_func(char *in, int inlen, char *out, int outlen)
{

	return 0;
}

static int set_ftp_open_func(char *in, int inlen, char *out, int outlen)
{
	int ret = 0;

	if(NULL == out) {
		out = (char *)malloc(OSIZE);

		if(NULL == out) {
			return EMALLOC;
		}
	}

	if(in[0] == ' ' && in[1] != ' ') {
		ret = user_ftp_open((in + 1));

		if(ret < 0) {
			snprintf(out, sizeof("open ftp failed"), "open ftp failed");
			return -1;
		}

		snprintf(out, sizeof("open ftp success!"), "open ftp success!");
		return 0;
	}

	snprintf(out, sizeof("open ftp failed!"), "open ftp failed!");
	return -1;
}

static int set_ftp_close_func(char *in, int inlen, char *out, int outlen)
{
	int ret = 0;
	ret = user_ftp_close();

	if(ret < 0) {
		snprintf(out, sizeof("close ftp failed! ftp server not start"), "close ftp failed! ftp server not start");
		return -1;
	}

	snprintf(out, sizeof("close success!"), "close success!");
	return 0;
}

static int set_ftp_size_func(char *in, int inlen, char *out, int outlen)
{
	int size = 0;

	if(in[0] != '0') {
		size = atoi(in);
		user_set_ftp_size(size);
		snprintf(out, sizeof("set ftp size success!"), "set ftp size success!");
	} else {
		user_set_ftp_size(0);
	}

	return 0;
}

static int set_low_gop_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}




static int set_video_profile_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}


int save_ftp_times_func(char *in, int inlen, char *out, int outlen)
{
	int ret = 0;
	ret = user_save_ftp_times((in + 1), OSIZE);

	if(ret < 0) {
		snprintf(out, outlen, "save ftp times failed\n");
		return -1;
	}

	snprintf(out, outlen, "The ftp times is %s", (in + 1));
	return 0;
}

int save_ftp_url_func(char *in, int inlen, char *out, int outlen)
{
	int ret = 0;
	ret = user_save_ftp_url((in + 1), OSIZE);

	if(ret < 0) {
		snprintf(out, outlen, "save ftp times failed\n");
		return -1;
	}

	snprintf(out, outlen, "The ftp times is %s", (in + 1));
	return 0;
}




static int set_func(char *in, int inlen, char *out, int outlen)
{
	int i = 0, ret;
	int cmd_len = 0;

	for(i = 0; i < sizeof(cmd_set_t) / sizeof(CMD_T); i++) {
		ret = strncmp(in, cmd_set_t[i].cmd, strlen(cmd_set_t[i].cmd));

		//find cmd
		if(ret == 0) {
			cmd_len	= strlen(cmd_set_t[i].cmd);

			if(cmd_set_t[i].func != NULL)

			{
				ret = cmd_set_t[i].func(in + cmd_len, inlen - cmd_len, out, outlen);
			}

			return ret;
		}
	}

	return ENOFIND;
}


static int get_func(char *in, int inlen, char *out, int outlen)
{
	int i = 0, ret;
	int cmd_len = 0;

	for(i = 0; i < sizeof(cmd_get_t) / sizeof(CMD_T); i++) {
		ret = strncmp(in, cmd_get_t[i].cmd, strlen(cmd_get_t[i].cmd));

		//find cmd
		if(ret == 0) {
			cmd_len	= strlen(cmd_get_t[i].cmd);

			if(cmd_get_t[i].func != NULL) {
				ret = cmd_get_t[i].func(in + cmd_len, inlen - cmd_len, out, outlen);
			}

			return ret;
		}
	}

	return ENOFIND;
}

static int save_func(char *in, int inlen, char *out, int outlen)
{
	return 0;
}

static int help_func(char *in, int inlen, char *out, int outlen)
{
	int i = 0;

	for(i = 0; i < sizeof(cmd_get_t) / sizeof(CMD_T); i++) {
		if(outlen - strlen(out) > 1) {
			snprintf(out + strlen(out), outlen - strlen(out), "%s\r\n", cmd_get_t[i].cmd);
		}
	}

	for(i = 0; i < sizeof(cmd_set_t) / sizeof(CMD_T); i++) {
		if(outlen - strlen(out) > 1) {
			snprintf(out + strlen(out), outlen - strlen(out), "%s\r\n", cmd_set_t[i].cmd);
		}
	}

	for(i = 0; i < sizeof(cmd_save_t) / sizeof(CMD_T); i++) {
		if(outlen - strlen(out) > 1) {
			snprintf(out + strlen(out), outlen - strlen(out), "%s\r\n", cmd_save_t[i].cmd);
		}
	}


	return 0;
}

int rtelnet_port_cmd(int type, char *in, int inlen, char *out, int outlen)
{
	int ret = 0;
	char buf[OSIZE] = {0};

	if(out == NULL) {
		out = (char *)malloc(OSIZE);

		if(NULL == out) {
			return EMALLOC;
		}
	}

	memset(out, 0, outlen);

	if(!strncmp(in, "get_", 4)) {
		ret = get_func(in, inlen, buf, sizeof(buf));
	} else if(!strncmp(in, "set_", 4)) {
		ret = set_func(in, inlen, buf, sizeof(buf));
	} else if(!strncmp(in, "save", 4)) {
		ret = save_func(in, inlen, buf, sizeof(buf));
	} else if(!strncmp(in, "help", 4)) {
		ret = help_func(in, inlen, buf, sizeof(buf));
	} else {
		ret = ENOFIND;
	}

	if(ret >= 0) {
		snprintf(out, outlen,  "%s cmd success %s\r\n>", in, buf);
		ret = strlen(out) + 1;
	} else if(ret == ENOFIND) {
		snprintf(out, outlen,  "%s cmd not found!\r\n>", in);
		ret = strlen(out) + 1;
	} else if(ret == ENOPARAM) {
		snprintf(buf, outlen,  "%s failed param wrong!\r\n>", in);
		ret = strlen(out) + 1;
	} else {
		snprintf(out, outlen, "%s cmd faild, %s\r\n>", in, buf);
		ret = strlen(out) + 1;
	}

	return ret;
}

