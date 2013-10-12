#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>



#include "mid_task.h"
#include "mid_mutex.h"


//#include "log_common.h"

typedef void *(*pthread_routine)(void *);

#define MID_TASKNUM_MAX		32

static int g_index;
static pthread_t g_threads[MID_TASKNUM_MAX];

static mid_mutex_t g_mutex = NULL;

void mid_task_init(void)
{
	int i;

	for(i = 0; i < MID_TASKNUM_MAX; i ++) {
		g_threads[i] = -1;
	}

	g_mutex = mid_mutex_create();
	g_index = 0;
}

int mid_task_create(const char *name, int stack_size, int priority,
                    mid_func_t func, void *arg)
{
	int index, err;

	if(g_mutex == NULL) {
		printf("mid task not init\n");
	}

	mid_mutex_lock(g_mutex);

	if(g_index >= MID_TASKNUM_MAX) {
		mid_mutex_unlock(g_mutex);
		printf("mid task not init %d, %d, %s\n", g_index, MID_TASKNUM_MAX, name);
		return -1;
	}

	index = g_index;
	g_index ++;
	mid_mutex_unlock(g_mutex);

	err = pthread_create(&g_threads[index], NULL, (pthread_routine)func, arg);


	if(err) {
		printf("pthread_create\n");
		return -1;
	}

	printf("*********************************create pthread name=%s= %p =%s\n", name, &g_threads[index], __FUNCTION__);
	return 0;
}

void mid_task_delay(unsigned int ms)
{
	usleep(ms * 1000);
}
