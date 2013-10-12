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
	int8_t procname[200];   //进程名
	int8_t 	  param[200];	//进程参数
	uint32_t  max_mem;	    //最大内存
	uint32_t  max_cpu;      //最大cpu
	uint32_t  active;       //是否激活
	uint32_t  guardian;     //是否守护
	uint32_t  time;         //检测的时间段
	int8_t 	  run[20];      //运行参数 咱无用
	
	list_head stlist;
	uint32_t livestate;		//存活状态
	uint32_t rtcputime[MAXSAMEPROC];       //实时线程cpu时间
	uint32_t rtsyscputime[MAXSAMEPROC];	   //实时系统cpu时间
	uint32_t rtmem[MAXSAMEPROC];           //实时内存
	uint32_t rtpid[MAXSAMEPROC];		   //线程ID
	uint32_t rtprent[MAXSAMEPROC];         //线程cpu百分比
	uint32_t rttotalcpu[MAXSAMEPROC];	//cpu总百分比累计
	uint32_t rttotalmem[MAXSAMEPROC];	//内存累计
	int32_t  rttotaltimes[MAXSAMEPROC];	//统计次数
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

/* 初始化双向链表 */
#define list_init(head) do			\
{						\
	(head)->next = (head)->prev = (head);	\
} while(0)

/* 在指定元素(where)之后插入新元素(item) */
#define list_add(item, towhere) do	\
{					\
	(item)->next = (towhere)->next;	\
	(item)->prev = (towhere);	\
	(towhere)->next = (item);	\
	(item)->next->prev = (item);	\
} while(0)

/* 在指定元素(where)之前插入新元素(item) */
#define list_add_before(item, towhere)  \
	list_add(item,(towhere)->prev)

/* 删除某个元素 */
#define list_remove(item) do			\
{						\
	(item)->prev->next = (item)->next;	\
	(item)->next->prev = (item)->prev;	\
} while(0)

/* 正向遍历链表中所有元素 */
#define list_for_each_item(item, head)\
	for ((item) = (head)->next; (item) != (head); (item) = (item)->next)

/* 反向遍历链表中所有元素 */
#define list_for_each_item_rev(item, head) \
	for ((item) = (head)->prev; (item) != (head); (item) = (item)->prev)

/* 根据本节点(item)获取节点所在结构体(type)的地址 */
/* 节点item地址(member的地址) - 该链表元素member在结构体中的偏移 */
#define list_entry(item, type, member) \
	((type *)((char *)item - (char *)(&((type *)0)->member)))

/* 判断链表是否为空 */
#define list_is_empty(head)	\
	((head)->next == (head))

/* 获取指定位置上一元素 */
#define list_prev_item(item)\
	((head)->prev)
#endif