#ifndef _MID_TASK_H__
#define _MID_TASK_H__

#include "mid_struct.h"

void mid_task_init(void);
int mid_task_create(const char *name, int stack_size, int priority, mid_func_t func, void *arg);
void mid_task_delay(unsigned int ms);

#endif /* _MID_OS_TASK_H_2007_3_24__ */
