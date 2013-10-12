/*******************************************************************************
*   由于ENC1200的系统OS时间长时间运行，比正常快20S左右，影响了需要时间戳计算的
*  的相关应用，所以，特地加入一个应用，每固定时间从RTC时间获取时间，然后同步到
*  系统时间。
*
*
*
*
*
********************************************************************************/

#include <unistd.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "mid_struct.h"
#include "mid_mutex.h"
#include "mid_timer.h"
#include "mid_sem.h"
#include "mid_task.h"
#include "rwini.h"
//#include "log_common.h"

#define TIME_ADJUST_CONFIG "time_adjust_config.ini"
//[adjust]
//need=0 //不需要矫正

static int mid_time_adjust()
{
	unsigned int last_time = 0;
	unsigned int current_time = 0;

	last_time = get_cur_time();

	while(1) {

		current_time = get_cur_time();
		printf("current_time =%d,last_time=%d\n", current_time, last_time);

		if((current_time - last_time) >= 1 * 60 * 1000) {
			last_time = current_time;
			system("hwclock -s"); //set the system time from the hardware clock
			printf("i will set the hwclock -s\n");
		}

		sleep(60);
	}

	return 0;

}



int mid_time_adjust_init()
{
	int need = 1; //默认需要矫正
	char  temp[512] = {0};
	int ret = 0;
	ret =  ConfigGetKey(TIME_ADJUST_CONFIG, "adjust", "need", temp);

	if(ret != 0) {
		printf("Get [%s]:%s from %s failed\n", "adjust", "need", TIME_ADJUST_CONFIG);
	} else {
		need = atoi(temp);

		if(need != 0) {
			need = 1;
		}
	}

	printf("need = %d\n", need);

	if(need == 1) {
		mid_task_create("mid_time_adjust", 0, 0, (mid_func_t)mid_time_adjust, 0);
	}

	return 0;

}