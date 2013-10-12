#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mid_sem.h"

//#include "log_common.h"
#include <unistd.h>


struct mid_sem {
	int fds[2];
};

mid_sem_t mid_sem_create(void)
{
	mid_sem_t sem;

	sem = (mid_sem_t)malloc(sizeof(struct mid_sem));

	if(sem == NULL) {
		printf("ind_malloc\n");
		return NULL;
	}

	if(pipe(sem->fds)) {
		free(sem);
		printf("pipe\n");
		return NULL;
	}

	return sem;
}

int mid_sem_take(mid_sem_t sem, int sec, int usec)
{
	int ret;
	char buf[1];

	fd_set rset;
	struct timeval tv;

	if(sem == NULL || sec < 0 || usec < 0) {
		printf("sem = %p, sec = %d, usec = %d\n", sem, sec, usec);
	}

	tv.tv_sec = sec;
	tv.tv_usec = usec;

	FD_ZERO(&rset);
	FD_SET(sem->fds[0], &rset);

	ret = select(sem->fds[0] + 1, &rset, NULL, NULL, &tv);

	if(ret == 0) {
		return -1;
	}

	if(ret <= 0) {
		printf("select ret = %d\n", ret);
		return -1;
	}

	ret = read(sem->fds[0], buf, 1);

	if(ret != 1) {
		printf("ret = %d\n", ret);
		return -1;
	}

	return 0;
}

int mid_sem_give(mid_sem_t sem)
{
	int ret;
	char buf[1];

	fd_set wset;
	struct timeval tv;

	if(sem == NULL) {
		printf("sem = %p\n", sem);
	}

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&wset);
	FD_SET(sem->fds[1], &wset);

	ret = select(sem->fds[1] + 1, NULL, &wset, NULL, &tv);

	if(ret == 0) {
		goto Err;
	}

	if(ret <= 0) {
		printf("select ret = %d\n", ret);
	}

	ret = write(sem->fds[1], buf, 1);

	if(ret != 1) {
		printf("ret = %d\n", ret);
	}

	return 0;
Err:
	return -1;
}

void mid_sem_delete(mid_sem_t sem)
{
	if(sem == NULL) {
		printf("sem = %p\n", sem);
		return;
	}

	close(sem->fds[0]);
	close(sem->fds[1]);
	free(sem);

	return;
}
