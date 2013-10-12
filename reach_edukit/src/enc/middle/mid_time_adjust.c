/*******************************************************************************
*   ����ENC1200��ϵͳOSʱ�䳤ʱ�����У���������20S���ң�Ӱ������Ҫʱ��������
*  �����Ӧ�ã����ԣ��صؼ���һ��Ӧ�ã�ÿ�̶�ʱ���RTCʱ���ȡʱ�䣬Ȼ��ͬ����
*  ϵͳʱ�䡣
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
//need=0 //����Ҫ����

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
	int need = 1; //Ĭ����Ҫ����
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