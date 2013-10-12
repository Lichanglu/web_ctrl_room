#ifndef _MID_COMMON_H__
#define _MID_COMMON_H__

#include <pthread.h>
#include <sys/types.h>

struct mid_mutex {
	pthread_mutex_t id;
};
typedef struct mid_mutex *mid_mutex_t;

mid_mutex_t mid_mutex_create(void);
int mid_mutex_lock(mid_mutex_t mutex);
int mid_mutex_unlock(mid_mutex_t mutex);
void mid_mutex_destroy(mid_mutex_t *mutex);



#endif
