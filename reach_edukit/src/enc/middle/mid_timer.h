#ifndef _MID_TIMER_H__
#define _MID_TIMER_H__

#include "mid_struct.h"

int mid_timer_create0(int intval, int loops, mid_func_t func, void *arg);
int mid_timer_create(int intval, int loops, mid_func_t func);
void mid_timer_delete0(mid_func_t func, void *arg);
void mid_timer_delete(mid_func_t func);
void mid_timer_init(void);
void mid_timer_print(void);
unsigned long long app_get_start_time();
int app_set_start_time();
unsigned long long getCurrentTime2(void);

unsigned int get_cur_time(void);




#endif
