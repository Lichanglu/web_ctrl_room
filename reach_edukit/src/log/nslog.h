/*
NodeServer-log module
*/

#ifndef __NSLOG_H__
#define __NSLOG_H__
#include "zlog.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define NONE				"\033[m"
#define RED				"\033[0;32;31m"
#define LIGHT_RED		"\033[1;31m"
#define GREEN			"\033[0;32;32m"
#define LIGHT_GREEN		"\033[1;32m"
#define BLUE				"\033[0;32;34m"
#define LIGHT_BLUE		"\033[1;34m"
#define DARY_GRAY		"\033[1;30m"
#define CYAN				"\033[0;36m"
#define LIGHT_CYAN		"\033[1;36m"
#define PURPLE			"\033[0;35m"
#define LIGHT_PURPLE		"\033[1;35m"
#define BROWN			"\033[0;33m"
#define YELLOW			"\033[1;33m"
#define LIGHT_GRAY		"\033[0;37m"
#define WHITE			"\033[1;37m"


#define NS_DEBUG 		ZLOG_LEVEL_DEBUG
#define NS_INFO  		ZLOG_LEVEL_INFO
#define NS_NOTICE	ZLOG_LEVEL_NOTICE
#define NS_WARN		ZLOG_LEVEL_WARN
#define NS_ERROR 	ZLOG_LEVEL_ERROR
#define NS_FATAL 		ZLOG_LEVEL_FATAL
#if 0
extern int ctrl_room_id;
#define nslog(nLevel, format, args...) \
	dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
	nLevel, "[ROOM_ID:%d]" format, ctrl_room_id, ##args)
#endif
#define nslog(nLevel, format, args...) \
	do{\
		switch(nLevel) {\
			case NS_WARN:\
				dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
				nLevel, YELLOW "[ROOM_ID:0]" format NONE, ##args); \
				break;\
			case NS_ERROR:\
			case NS_FATAL:\
				dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
				nLevel, RED "[ROOM_ID:0]" format NONE, ##args); \
				break;\
			case NS_INFO:\
			case NS_DEBUG:\
			default:\
				dzlog(__FILE__, sizeof(__FILE__)-1, __func__, sizeof(__func__)-1, __LINE__, \
				nLevel, "[ROOM_ID:0]" format, ##args); \
				break;\
		}\
	}while(0)

typedef struct nslog_conf_info_ {
	char conf[512];
	char cname[32];
	char output[512];
}  nslog_conf_info_t;

int NslogInit(nslog_conf_info_t *info);

#ifdef __cplusplus
}
#endif

#endif //__NSLOG_H__

