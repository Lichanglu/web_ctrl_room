#ifndef __MID_SEM_H_2007_3_25__
#define __MID_SEM_H_2007_3_25__


typedef struct mid_sem *mid_sem_t;
mid_sem_t mid_sem_create(void);
int mid_sem_take(mid_sem_t sem, int sec, int usec);
int mid_sem_give(mid_sem_t sem);
void mid_sem_delete(mid_sem_t sem);

#endif /*__MID_OS_SEM_H_2007_3_25__*/
