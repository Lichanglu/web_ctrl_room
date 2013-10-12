#ifndef _MOUNT_H
#define _MOUNT_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mntent.h>
#include <sys/statfs.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "command_resolve.h"
#include "params.h"
#include "sercenterctrl.h"
//#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
//#define __USE_FILE_OFFSET64

#define PARTITION "/opt/Rec"

#ifndef NULL
#define NULL 0
#endif

#define FILE_SIZE    (1024*2)


#if 0
typedef struct _list_head
{
   struct _list_head 	*next;
   struct _list_head	*prev;
   
}list_head;
#endif

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


typedef struct _DevAtt
{
	char devname[20];
	list_head stlist;
	int IsExist;
	uint64_t   total_size;
	uint64_t   free_size;
	int percent;
	int neednotice;
}DevAtt;



extern int GetDiskState(char *name, unsigned  long *totalsize, unsigned  long *freesize);
extern int RegDiskDetectMoudle();
extern int GetUsbDevState(int *num, MNUsbDeb *stat, int max);
#endif
