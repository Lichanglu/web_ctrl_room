#ifndef __CGI__H
#define __CGI__H
#define USERNAME "admin"
#define WEBUSERNAME "operator"



#define CL_WEB_VERSION						("2.1.0")


#define INPUT_1 1
#define INPUT_2 2

#define HIGH_STREAM 1
#define LOW_STREAM 2


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
