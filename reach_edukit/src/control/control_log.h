/*
 * =====================================================================================
 *
 *       Filename:  control_log.h
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


#ifndef _CONTROL_LOG_H_
#define _CONTROL_LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include "zlog.h"



#define ZLOG_CONFIG_FILE						("./control_log.cfg")
#define ZLOG_CONFIG_FILE_BAK					("./control_log_bak.cfg")
#define ZLOG_OPERATION_LOG						("operation_log")
#define ZLOG_DEBUG_LOG							("debug_log")

#define ZLOG_LOG_HEAD_LEN						(64)

#define OPELOG									(g_operation_log)
#define DBGLOG									(g_debug_log)
#define NONE				"\033[m"
#define RED				"\033[0;32;31m"

#define zlog_debug2(cat, format, args...)		zlog_debug(cat, "%s" format, plog_head, ##args)
#define zlog_error2(cat, format, args...)		zlog_error(cat, RED "%s" format NONE, plog_head, ##args)
#define zlog_error3(cat, format, args...)		zlog_error(cat, RED "%s" format NONE, ##args)


extern zlog_category_t *g_operation_log;
extern zlog_category_t *g_debug_log;

int32_t control_log_init();
int32_t control_log_deinit();




#endif
