#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
//#include "common.h"
#include "mid_mutex.h"

//#include "log_common.h"

mid_mutex_t mid_mutex_create(void)
{
	mid_mutex_t mutex;

	mutex = (mid_mutex_t)malloc(sizeof(struct mid_mutex));

	if(mutex == NULL) {
		printf("\n");
		return NULL;
	}

	if(pthread_mutex_init(&mutex->id, NULL)) {
		free(mutex);
		return NULL;
	}

	return mutex;
}

int mid_mutex_lock(mid_mutex_t mutex)
{
	if(mutex == NULL) {
		printf("\n");
		return -1;
	}

	pthread_mutex_lock(&mutex->id);
	return 0;
}

int mid_mutex_unlock(mid_mutex_t mutex)
{
	if(mutex == NULL) {
		printf("\n");
		return -1;
	}

	pthread_mutex_unlock(&mutex->id);

	return 0;
}

void mid_mutex_destroy(mid_mutex_t *mutex)
{
	if(*mutex == NULL) {
		return ;
	}

	free(*mutex);
	*mutex = NULL;
}
