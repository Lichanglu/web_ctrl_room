#ifndef __CGI__H
#define __CGI__H
#define USERNAME "admin"
#define WEBUSERNAME "operator"

#define WEB_INPUT_1 1
#define WEB_INPUT_2 2
#define WEB_INPUT_3 3

#define WEB_HIGH_STREAM 1
#define WEB_LOW_STREAM 2


extern char sys_password[100];
extern char sys_webpassword[100];
extern char sys_timeout[100];
extern char sys_language[100];
typedef struct __ServerCfg {
	int ip[4];
	int	channelno;
} ServerCfg;

typedef struct __SysSetCfg {
	unsigned int vstdConfig;
} SysSetCfg;

typedef struct __ServerInfoCfg {
	int id;
	int ip[4];
	char name[40];
	int port;
	int	channelno;
} ServerInfoCfg;

#endif
