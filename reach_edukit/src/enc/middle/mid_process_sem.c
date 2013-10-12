/*********************************************************************
*   创建进程之间的互斥锁
*                                add by zm
*												2011-3-5
**********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<sys/ipc.h>
#include<sys/sem.h>

//#include "log_common.h"
static void get_sem_val2(int semid, int num);
#define DEFAULT_FILE_PATH "/opt/reach"
#if 0
#define get_sem_val(semid,num)  printf(" ");\
	get_sem_val2(semid,num);
#else
#define get_sem_val(semid,num)  get_sem_val2(semid,num)
#endif
int mid_creat_sem(int proj_id, int nsems)
{
	key_t key = 0;
	int semid = -1;
	key = ftok(DEFAULT_FILE_PATH, proj_id);

	if(key == -1) {
		printf("ERROR.....ftok...mid_creat_sem fail.\n");
		return -1;
	}

	printf("the sem key is %d.\n", key);

	semid = semget(key, nsems + 1, IPC_CREAT);

	if(semid < 0) {
		printf("ERROR....segget error...mid_creat_sem fail.\n");
		return -1;
	}

	get_sem_val(semid, 0);
	//判断信号量是否已经初始化，默认第一个信号量做为判断。
	int val = 0;
	val = semctl(semid, 0, GETVAL);
	printf("val = %d\n", val);

	if(val == 0) {
		printf("2val = %d\n", val);
		int i = 0;

		//设置所有信号量为1，为互斥做准备
		for(i = 0; i < nsems + 1; i++) {
			semctl(semid, i, SETVAL, 1);
		}
	} else {
		printf("this sem is already init \n");
	}

	get_sem_val(semid, 0);
	get_sem_val(semid, 1);
	return semid;
}

int mid_process_mutex(int semid, int num)
{
	if(semid == -1 || num == 0) {
		return -1;
	}

	int ret = 0;
	static struct sembuf lock = {0, -1, SEM_UNDO};
	lock.sem_num = num;
	get_sem_val(semid, num);
	ret = semop(semid, &lock, 1);
	get_sem_val(semid, num);
	return ret;
}

int mid_process_unmutex(int semid, int num)
{
	if(semid == -1 || num == 0) {
		return -1;
	}

	int ret = 0;
	static struct sembuf lock = {0, 1, SEM_UNDO};
	lock.sem_num = num;

	get_sem_val(semid, num);
	ret = semop(semid, &lock, 1);
	get_sem_val(semid, num);
	return ret ;
}

int mid_process_detele(int semid)
{
	if(semid == -1) {
		return -1;
	}

	semctl(semid, 0, IPC_RMID, 1);
	return 0;
}

static void get_sem_val2(int semid, int num)
{

	int val = 0;
	val = semctl(semid, num, GETVAL);
	//printf("debug...the semid = %d,the num = %d,val = %d\n",semid,num,val);
	return ;

}



