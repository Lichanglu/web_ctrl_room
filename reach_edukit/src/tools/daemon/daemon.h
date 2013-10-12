#ifndef _DAEMON_H
#define _DAEMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <sys/param.h>
#include "reach_os.h"
#include <sys/procfs.h>
#include <sys/resource.h>
#include <xml/xml_base.h>
#include <assert.h>
#include "zlog.h"
#include "cli_verify.h"

#define ZLOG "/usr/local/reach/zlog_upload.conf"
#define XML  "/usr/local/reach/daemon.xml"

#define VERSION  "Daemon_v2.0"
#define CPUNUM   2
#define READ_BUF_SIZE 1024
#define PAGE_SHIFT 12
#define bytetok(x)	(((x) + 512) >> 10)
#define pagetok(x)	((x) << (PAGE_SHIFT - 10))
#define PROCBUFFERLEN   4*1024
#define MAXSAMEPROC   10

#define RED				"\033[0;32;31m"
#define NONE				"\033[m"

typedef struct _list_head
{
   struct _list_head 	*next;
   struct _list_head	*prev;
   
}list_head;

typedef struct _DevAtt
{
	int8_t procname[200];   //������
	int8_t 	  param[200];	//���̲���
	uint32_t  max_mem;	    //����ڴ�
	uint32_t  max_cpu;      //���cpu
	uint32_t  active;       //�Ƿ񼤻�
	uint32_t  guardian;     //�Ƿ��ػ�
	uint32_t  time;         //����ʱ���
	int8_t 	  run[20];      //���в��� ������
	
	list_head stlist;
	uint32_t livestate;		//���״̬
	uint32_t rtcputime[MAXSAMEPROC];       //ʵʱ�߳�cpuʱ��
	uint32_t rtsyscputime[MAXSAMEPROC];	   //ʵʱϵͳcpuʱ��
	uint32_t rtmem[MAXSAMEPROC];           //ʵʱ�ڴ�
	uint32_t rtpid[MAXSAMEPROC];		   //�߳�ID
	uint32_t rtprent[MAXSAMEPROC];         //�߳�cpu�ٷֱ�
	uint32_t rttotalcpu[MAXSAMEPROC];	//cpu�ܰٷֱ��ۼ�
	uint32_t rttotalmem[MAXSAMEPROC];	//�ڴ��ۼ�
	int32_t  rttotaltimes[MAXSAMEPROC];	//ͳ�ƴ���
	uint32_t samenum;
}NodeAtt;

typedef struct _system_info
{
  
    uint32_t    TotalMem;
    uint32_t    TotalCpu;
}system_info;

typedef struct _progress_state
{

	int8_t     name[20];
    uint32_t   Mem;
    uint32_t   CpuTime;
	uint32_t   State;
}ProState;

typedef struct _daemon_hand
{
	zlog_category_t *c;
	parse_xml_t *px;
	list_head *phead;
	pthread_mutex_t daemonlock;
	int32_t roomnum;
}DaemonHand;

typedef struct _daemon_config
{
	int8_t name[200];
	int8_t param[20];
	int8_t run[200];
	int8_t active[20];
	int8_t guardian[20];
	int8_t maxcpu[20];
	int8_t maxmem[20];
	int8_t time[20];	
}Daemon_config;

/* ��ʼ��˫������ */
#define list_init(head) do			\
{						\
	(head)->next = (head)->prev = (head);	\
} while(0)

/* ��ָ��Ԫ��(where)֮�������Ԫ��(item) */
#define list_add(item, towhere) do	\
{					\
	(item)->next = (towhere)->next;	\
	(item)->prev = (towhere);	\
	(towhere)->next = (item);	\
	(item)->next->prev = (item);	\
} while(0)

/* ��ָ��Ԫ��(where)֮ǰ������Ԫ��(item) */
#define list_add_before(item, towhere)  \
	list_add(item,(towhere)->prev)

/* ɾ��ĳ��Ԫ�� */
#define list_remove(item) do			\
{						\
	(item)->prev->next = (item)->next;	\
	(item)->next->prev = (item)->prev;	\
} while(0)

/* �����������������Ԫ�� */
#define list_for_each_item(item, head)\
	for ((item) = (head)->next; (item) != (head); (item) = (item)->next)

/* �����������������Ԫ�� */
#define list_for_each_item_rev(item, head) \
	for ((item) = (head)->prev; (item) != (head); (item) = (item)->prev)

/* ���ݱ��ڵ�(item)��ȡ�ڵ����ڽṹ��(type)�ĵ�ַ */
/* �ڵ�item��ַ(member�ĵ�ַ) - ������Ԫ��member�ڽṹ���е�ƫ�� */
#define list_entry(item, type, member) \
	((type *)((char *)item - (char *)(&((type *)0)->member)))

/* �ж������Ƿ�Ϊ�� */
#define list_is_empty(head)	\
	((head)->next == (head))

/* ��ȡָ��λ����һԪ�� */
#define list_prev_item(item)\
	((head)->prev)
#endif