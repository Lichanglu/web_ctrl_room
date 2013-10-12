/*
 * =====================================================================================
 *
 *       Filename:  control_log.c
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


#include <stdio.h>

#include "control_log.h"
#include "zlog.h"
#include "control.h"


zlog_category_t *g_operation_log = NULL;
zlog_category_t *g_debug_log = NULL;




int32_t create_default_log_conf(const int8_t *def_log_conf)
{
	FILE *fp = NULL;
	
	fp = fopen((char *)def_log_conf, "w+");
	if(fp == NULL){
		fprintf(stderr, "create_default_log_conf failed!\n");
		return -1;
	}

	fprintf(fp, "[formats]\n");
	fprintf(fp, "simple = %s\n", "\"CONTROL: %D %m\"");
	fprintf(fp, "[rules]\n");
	fprintf(fp, "%s.DEBUG \"./log_file\", 1M; simple\n", ZLOG_OPERATION_LOG);
	fprintf(fp, "%s.DEBUG >stdout; simple\n", ZLOG_DEBUG_LOG);

	fclose(fp);

	return 0;
}

int32_t control_log_init()
{
	int32_t ret = 0;
//	int8_t buf[256] = {0};

	zlog_category_t *cat_1 = NULL;
	zlog_category_t *cat_2 = NULL;

	
	ret = zlog_init(ZLOG_CONFIG_FILE);
	if(ret){
		fprintf(stderr, "zlog init failed\n");
		zlog_init(ZLOG_CONFIG_FILE_BAK);
	}

	cat_1 = zlog_get_category(ZLOG_OPERATION_LOG);
	if(cat_1 == NULL){
		fprintf(stderr, "get g_operation_log fail\n");
	}

	cat_2 = zlog_get_category(ZLOG_DEBUG_LOG);
	if(cat_2 == NULL){
		fprintf(stderr, "get g_debug_log fail\n");
	}
	
	if((NULL == cat_1) || (NULL == cat_2)){
		ret = create_default_log_conf((const int8_t *)ZLOG_CONFIG_FILE_BAK);
		if(ret == -1){
			return -1;
		}
		else {
			zlog_init(ZLOG_CONFIG_FILE_BAK);
			cat_1 = zlog_get_category(ZLOG_OPERATION_LOG);
			if(cat_1 == NULL){
				fprintf(stderr, "get g_operation_log fail\n");
			}

			cat_2 = zlog_get_category(ZLOG_DEBUG_LOG);
			if(cat_2 == NULL){
				fprintf(stderr, "get g_debug_log fail\n");
			}

			
			if((NULL == cat_1) || (NULL == cat_2)){
				zlog_fini();
				fprintf(stderr, "zlog_get_category fail\n");
				return -1;
			}
			else {
#if 0
				r_memset(buf, 0, 256);
				sprintf(buf, "cp %s %s", ZLOG_CONFIG_FILE_BAK, ZLOG_CONFIG_FILE);
				r_system((const int8_t *)buf);
#endif
			}
		}
	}

	g_operation_log = cat_1;
	g_debug_log = cat_2;

	return 0;
}


int32_t control_log_deinit()
{
	zlog_fini();
	return 0;
}



