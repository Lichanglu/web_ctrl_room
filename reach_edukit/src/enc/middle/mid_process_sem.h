#ifndef _MID_PROCESS_MUTEX_H__
#define _MID_PROCESS_MUTEX_H__


int mid_creat_sem(int proj_id, int nsems);
int mid_process_mutex(int semid, int num);
int mid_process_unmutex(int semid, int num);
int mid_process_detele(int semid);
#endif
