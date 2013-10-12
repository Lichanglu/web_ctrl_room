/*************************************************************************
* 设置流量整形模块，参照李昌绿最早的实现，使用HTB方式
*
*
*
*
**************************************************************************/


#ifdef HAVE_RTSP_MODULE
#include <stdlib.h>
#include  <sys/types.h>
#include  <sys/socket.h>
#include  <netinet/in.h>
#include  <string.h>
#include  <stdio.h>
#include  <unistd.h>
#include  <arpa/inet.h>
#include  <time.h>
#include  <pthread.h>
#include  <sys/select.h>
#include  <string.h>

#include "log_common.h"
#include "traffic_shaping.h"

typedef struct _TC_INFO {
	int class_id;
	char dst_ip[16];
	int dst_port; //des port
	int average_rate;    //Average  kbps
	int ceiling_rate;    //ceiling rate kbps
} TC_INFO;

#define TC_MAX_NUM 10+1
//class id != 0
static TC_INFO  g_tc_info_array[TC_MAX_NUM] ;



/*
*对TC的部分初始化，其中有创建队列、根分类、子分类、过滤器、路由映射
*未对端口进行初始化操作
*
*/
void tc_init(void)
{
	return ;
	char command[256] = {0};

	int i = 0;

	for(i = 0; i < TC_MAX_NUM; i++) {
		memset(&g_tc_info_array[i], 0, sizeof(TC_INFO));
		g_tc_info_array[i].class_id = -1;
	}

	memset(command, 0, sizeof(command));
	snprintf(command, sizeof(command), "tc qdisc add dev eth0 root handle 1: htb default 98 ");
	system(command);
	PRINTF("++++++++++++++++++system command 1= %s.\n", command);

	memset(command, 0, sizeof(command));
	snprintf(command, sizeof(command), "tc class add dev eth0 parent 1:0 classid 1:99 htb rate 100000kbps ceil 100000kbit quantum 6000000");
	system(command);
	PRINTF("++++++++++++++++++system command 2= %s.\n", command);

	PRINTF("\n***************init TC success!****************\n");
	return ;
}


int tc_compare_element(char *dst_ip, unsigned short dst_port, int a_rate, int c_rate, TC_INFO *info)
{
	return 0;

	if(info->average_rate != a_rate || info->ceiling_rate != c_rate) {
		return 1;
	}

	if(info->dst_port != dst_port) {
		return 2;
	}

	if(strcmp(info->dst_ip, dst_ip) != 0) {
		return 2;
	}

	return 0;
}


/*add 一个class对应一路流用来限制码率*/
int tc_add_element(int classid, char *dst_ip, unsigned short dst_port, int a_rate, int c_rate)
{
	return 0;
	//return 0;
	PRINTF("classid=%d,ip %s:%d, a_rate=%d,c_rate=%d\n", classid, dst_ip, dst_port, a_rate, c_rate);

	if(classid > TC_MAX_NUM || dst_ip == NULL || dst_port < 1000 || a_rate == 0 || c_rate == 0) {
		PRINTF("Error\n");
		return 0;
	}

	//if classid is not null ,must del first
	if(g_tc_info_array[classid].class_id != -1) {
		if(tc_compare_element(dst_ip, dst_port, a_rate, c_rate, &(g_tc_info_array[classid])) == 0) {
			PRINTF("the tc info is same\n");
			return 0;
		}

		tc_del_element(classid);
	}


	char command[256] = {0};
	PRINTF("\n***************  add TC route ...     ****************\n");

	memset(command, 0, sizeof(command));
	snprintf(command, sizeof(command), "tc class add dev eth0 parent 1:99 classid 1:%d htb rate %dkbps ceil %dkbit prio 1  quantum 6000000", classid, a_rate, c_rate);
	PRINTF("++++++++++++++++++add system command 1:%s.\n", command);
	system(command);


	memset(command, 0, sizeof(command));
	//add protocol 17=UDP
	snprintf(command, sizeof(command), "tc filter add dev eth0 protocol ip parent 1:0 prio %d u32 match ip protocol 17 0xff match ip dst %s match ip dport %d 0xffff flowid 1:%d", classid, dst_ip, dst_port, classid);
	PRINTF("++++++++++++++++++add system command 2:%s.\n", command);
	system(command);


	g_tc_info_array[classid].class_id = classid;
	snprintf(g_tc_info_array[classid].dst_ip, sizeof(g_tc_info_array[classid].dst_ip), "%s", dst_ip);
	g_tc_info_array[classid].dst_port = dst_port;
	g_tc_info_array[classid].average_rate = a_rate;
	g_tc_info_array[classid].ceiling_rate = c_rate;

	tc_status_print();
	return 0;
}

int tc_del_element(int classid)
{
	return 0;
	//return 0;

	if(classid > TC_MAX_NUM) {
		PRINTF("Error\n");
		return -1;
	}

	if(g_tc_info_array[classid].class_id == -1) {
		PRINTF("Error\n");
		return -1;
	}

	char command[256] = {0};
	TC_INFO info = g_tc_info_array[classid];
	memset(command, 0, sizeof(command));
	snprintf(command, sizeof(command), "tc filter del dev eth0 protocol ip parent 1:0 prio %d u32 match ip dst %s match ip dport %d 0xffff flowid 1:%d", info.class_id, info.dst_ip, info.dst_port, info.class_id);
	PRINTF("++++++++++++++++++del system command 1:%s.\n", command);
	system(command);




	memset(command, 0, sizeof(command));
	snprintf(command, sizeof(command), "tc class del dev eth0 parent 1:99 classid 1:%d htb rate %dkbps ceil %dkbit prio 1  quantum 6000000", info.class_id, info.average_rate, info.ceiling_rate);
	PRINTF("++++++++++++++++++del system command 2:%s.\n", command);
	system(command);


	memset(&(g_tc_info_array[classid]), 0, sizeof(TC_INFO));
	g_tc_info_array[classid].class_id = -1;

	tc_status_print();
	return 0;
}

int tc_set_element(int classid, char *dst_ip, unsigned short dst_port, int a_rate, int c_rate)
{
	return 0;
	return tc_add_element(classid, dst_ip, dst_port, a_rate, c_rate);
}


int tc_status_print()
{
	//return 0;
#ifdef ENABLE_DEUBG
	char command[256] = {0};

	memset(command, 0, sizeof(command));
	snprintf(command, sizeof(command), "tc -s class ls dev eth0");
	PRINTF("++++++++++++++++++command:%s\n", command);
	system(command);


	memset(command, 0, sizeof(command));
	snprintf(command, sizeof(command), "tc -s qdisc ls dev eth0");
	PRINTF("++++++++++++++++++command:%s\n", command);
	system(command);

	memset(command, 0, sizeof(command));
	snprintf(command, sizeof(command), "tc -s filter ls dev eth0");
	PRINTF("++++++++++++++++++command:%s\n", command);
	system(command);
#endif
	return 0;
}

#endif