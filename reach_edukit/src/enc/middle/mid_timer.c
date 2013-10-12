/*****************************************************************************************
*  mid_timer.c  一个精确度到秒级别的定时器
*  调用顺序，APP启动初始化一次 mid_timer_init即可。
*  每次调用 int mid_timer_create(int intval, int loops, mid_func_t func) 来创建定时器
*  其中intval是间隔秒数，loops是循环次数，注意func必须是个不阻塞的函数，避免执行过长，影响
*  精确性。
*  注意，在同步时间的时候，需要加入逻辑，清空当前的定时器。
*
*
*
******************************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include "mid_struct.h"
#include "mid_mutex.h"
#include "mid_timer.h"
#include "mid_sem.h"
#include "mid_task.h"

//#include "log_common.h"

#define TIMER_POOL_SIZE		20

#define HOUR_CLOCK_NUM		(60*60*1000)

typedef struct _msg_timer_t msg_timer_t;
typedef struct _msg_timer_q msg_timer_q;

struct _msg_timer_t {
	msg_timer_t *next;
	mid_func_t func;
	void *arg;
	unsigned int loops;
	unsigned int intval;
	mid_msec_t clock;
};
struct _msg_timer_q {
	msg_timer_t *head;
};

static msg_timer_q	timer_q_pool;
static msg_timer_q	timer_q_time;

static mid_mutex_t	timer_mutex = NULL;
static mid_sem_t	timer_sem = NULL;

static int mid_tool_time2string(int sec, char *buf, char insert);
static int mid_msg_task_create(mid_func_t func);
static void mid_timer_loop(void *arg);
static unsigned int msg_timer_deal(void);
static void msg_timer_free(msg_timer_t *timer);
static msg_timer_t *msg_timer_alloc(void);
static msg_timer_t *msg_timer_dequeue(mid_func_t func, void *arg);
static void msg_timer_queue(msg_timer_t *timer, mid_msec_t clock);

extern unsigned int mid_clock(void);


static void msg_timer_queue(msg_timer_t *timer, mid_msec_t clock)
{
	msg_timer_t *next, *prev;

	while(timer->clock <= clock) {
		mid_msec_t s = timer->intval;
		timer->clock += s * 1000;
	}

	next = timer_q_time.head;
	prev = NULL;

	while(next && next->clock <= timer->clock) {
		prev = next;
		next = next->next;
	}

	timer->next = next;

	if(prev) {
		prev->next = timer;
	} else {
		timer_q_time.head = timer;
	}
}

static msg_timer_t *msg_timer_dequeue(mid_func_t func, void *arg)
{
	msg_timer_t *timer, *prev;

	timer = timer_q_time.head;
	prev = NULL;

	while(timer && (timer->func != func || timer->arg != arg)) {
		prev = timer;
		timer = timer->next;
	}

	if(timer == NULL) {
		return NULL;
	}

	if(prev) {
		prev->next = timer->next;
	} else {
		timer_q_time.head = timer->next;
	}

	return timer;
}

static msg_timer_t *msg_timer_alloc(void)
{
	msg_timer_t *timer = timer_q_pool.head;

	if(timer == NULL) {
		return NULL;
	}

	timer_q_pool.head = timer->next;
	timer->next = NULL;
	return timer;
}

static void msg_timer_free(msg_timer_t *timer)
{
	if(timer == NULL) {
		return;
	}

	timer->next = timer_q_pool.head;
	timer_q_pool.head = timer;
}

static unsigned int msg_timer_deal(void)
{
	mid_func_t timer_funcs[TIMER_POOL_SIZE];
	void *timer_args[TIMER_POOL_SIZE];
	unsigned int i, num;
	mid_msec_t clock, remain;
	msg_timer_t *timer;

	num = 0;
	mid_mutex_lock(timer_mutex);
	clock = mid_clock();

	timer = timer_q_time.head;

	while(timer && timer->clock <= clock) {
		timer_funcs[num] = timer->func;
		timer_args[num] = timer->arg;
		num ++;
		msg_timer_dequeue(timer->func, timer->arg);

		if(timer->loops == 0) {
			msg_timer_queue(timer, clock);
		} else {
			timer->loops --;

			if(timer->loops > 0) {
				msg_timer_queue(timer, clock);
			} else {
				msg_timer_free(timer);
			}
		}

		//记住 下面一行替换成timer = timer->next是错误的
		timer = timer_q_time.head;
	}

	timer = timer_q_time.head;

	if(timer) {
		remain = timer->clock - clock;
	} else {
		remain = HOUR_CLOCK_NUM;
	}

	mid_mutex_unlock(timer_mutex);

	for(i = 0; i < num; i ++) {
		timer_funcs[i](timer_args[i]);
	}

	if(remain > HOUR_CLOCK_NUM) {
		return HOUR_CLOCK_NUM;
	}

	return remain;
}

/*
	intval: 间隔秒数
	loops：定时器执行循环次数
	func：定时到执函数
	在定时器执行的那个函数里不要干太多的事，最好是只给你自己任务发一个消息而已。
 */
int mid_timer_create0(int intval, int loops, mid_func_t func, void *arg)
{
	//printf("func = %p,intval =%d\n", func, intval);
	msg_timer_t *timer = NULL;
	mid_msec_t clock = 0;

	if(intval <= 0 || loops < 0 || loops > 5 || func == NULL) {
		printf("param error! %d:%d:%p\n", intval, loops, func);
	}

	mid_mutex_lock(timer_mutex);

	timer = msg_timer_dequeue(func, arg);

	if(timer == NULL) {
		timer = msg_timer_alloc();
	}

	if(timer == NULL) {
		mid_mutex_unlock(timer_mutex);
		printf("timer pool is empty!\n");
	}

	clock = mid_clock();
	//printf("++++++++ %d  %d %p\n", intval, msec, func);
	timer->intval = intval;
	timer->clock = clock;
	timer->loops = loops;
	timer->func = func;
	timer->arg = arg;
	msg_timer_queue(timer, clock);

	mid_mutex_unlock(timer_mutex);

	mid_sem_give(timer_sem);

	return 0;
}

int mid_timer_create(int intval, int loops, mid_func_t func)
{

	return mid_timer_create0(intval, loops, func, 0);
}

void mid_timer_print(void)
{
	msg_timer_t *timer;
	char buf[32];

	printf("TIMER:\n");
	mid_mutex_lock(timer_mutex);
	timer = timer_q_time.head;

	while(timer) {
		mid_tool_time2string(timer->clock / 1000, buf, ':');
		printf("%p: %s\n", timer, buf);
		timer = timer->next;
	}

	mid_mutex_unlock(timer_mutex);
}

void mid_timer_delete0(mid_func_t func, void *arg)
{
	mid_mutex_lock(timer_mutex);
	msg_timer_free(msg_timer_dequeue(func, arg));
	mid_mutex_unlock(timer_mutex);
}

void mid_timer_delete(mid_func_t func)
{
	mid_timer_delete0(func, 0);
}

static void mid_timer_loop(void *arg)
{
	unsigned int msec = 0;

	for(;;) {
		msec = msg_timer_deal();
		mid_sem_take(timer_sem, msec / 1000, (msec % 1000) * 1000);
	}
}

void mid_timer_init(void)
{
	int i;

	timer_q_pool.head = NULL;
	timer_q_time.head = NULL;

	for(i = 0; i < TIMER_POOL_SIZE; i ++) {
		msg_timer_free((msg_timer_t *)malloc(sizeof(msg_timer_t)));
	}

	timer_mutex = mid_mutex_create();
	timer_sem = mid_sem_create();

	mid_msg_task_create(mid_timer_loop);
}


// 注意，获取到的是local
static int mid_tool_time2string(int sec, char *buf, char insert)
{
	struct tm t;

	if(buf == NULL) {
		printf("buf is null\n");
	}

	gmtime_r((time_t *)&sec, &t);

	if(insert == 0) {
		sprintf(buf, "%04d%02d%02d%02d%02d%02d",
		        t.tm_year + 1900,
		        t.tm_mon + 1,
		        t.tm_mday,
		        t.tm_hour,
		        t.tm_min,
		        t.tm_sec);
		buf[14] = '\0';
	} else {
		sprintf(buf, "%04d%c%02d%c%02d%c%02d%c%02d%c%02d",
		        t.tm_year + 1900, insert,
		        t.tm_mon + 1, insert,
		        t.tm_mday, insert,
		        t.tm_hour, insert,
		        t.tm_min, insert,
		        t.tm_sec);
		buf[19] = '\0';
	}

	return 0;
}


static int mid_msg_task_create(mid_func_t func)
{
	return mid_task_create("mid_msg", 0, 0, func, 0);
}



