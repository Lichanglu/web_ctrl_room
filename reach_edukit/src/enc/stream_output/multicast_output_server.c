/****************************************************************************
*
* steam_output_server.c
*============================================================================d
*  作用： 主要用于输出流的 multicast SERVER的设置。
*d
*
*============================================================================
*****************************************************************************/

#ifdef HAVE_RTSP_MODULE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>



#include "mid_struct.h"
#include "mid_mutex.h"
#include "mid_timer.h"
#include "mid_socket.h"


#include "stream_output_struct.h"
#include "multicast_output_server.h"
#include "stream_output_common.h"

#include "log_common.h"

#include "stream_output_client.h"
#include "traffic_shaping.h"

#include "input_to_channel.h"
//#include "WEB_CGI/cgi-enc/weblib.h"

#define MULTICAST_SERVER_CONFIG_INI "multicast_server_config.ini"
#define MULTICAST_MAX_CLIENT_NUM RTSP_MAX_CLIENT_NUM
#define MULTICAST_MAX_SERVER_NUM 8

typedef struct MULTICAST_SERVER_OUTPUT_T {
	int s_used;

	//int status; // active,pause,stop
	stream_output_server_config config;
	//	Stream_Output_Client client[MULTICAST_MAX_CLIENT_NUM];
	Stream_Output_Client *client;
	int tc_num;
	mid_mutex_t mutex;
	struct MULTICAST_SERVER_OUTPUT_T *next;
} MULTICAST_Output_Server_NODE;


typedef struct MULTICAST_SERVER_HEAD_T {
	int num; //list num
	mid_mutex_t mutex;
	MULTICAST_Output_Server_NODE *first;
} MULTICAST_OUTPUT_SERVER_HEAD;


/*********************************static*****function****start*******************************************/
static int multicast_list_add_node(MULTICAST_Output_Server_NODE *node);
static int multicast_list_delete_node(MULTICAST_Output_Server_NODE *node);
static MULTICAST_Output_Server_NODE *multicast_list_find_node(stream_output_server_config *config);
static int multicast_list_inset_node(stream_output_server_config *config);
static int multicast_list_check_full(void);
static void multicast_list_print_all(void);
static int multicast_list_get_num(MULTICAST_OUTPUT_SERVER_HEAD *head);
static int multicast_compare_config(stream_output_server_config *old, stream_output_server_config *new1);
static MULTICAST_Output_Server_NODE *multicast_node_create(void);
static int multicast_check_config(stream_output_server_config *oldconfig, stream_output_server_config *newconfig);
static int app_calc_rate(int vrate, int arate, int type);
static int multicast_node_config(stream_output_server_config *config, MULTICAST_Output_Server_NODE *node);
static void multicast_node_printf(MULTICAST_Output_Server_NODE *node);
static int  multicast_node_delete(MULTICAST_Output_Server_NODE **node);
static int multicast_head_delete(MULTICAST_OUTPUT_SERVER_HEAD **head);
static int app_mult_node_config_fread(char *config_file, stream_output_server_config *config, char *table);
static int app_mult_config_fread(MULTICAST_OUTPUT_SERVER_HEAD *handle);
static int app_mult_node_config_fwrite(char *config_name, stream_output_server_config *config, char *section);
static int app_mult_config_fwrite(int reset);
static void app_check_list_mult_status(void);
static int mult_set_mtu(int type, int mtu, int channel);
//static int mult_set_active(int type, int status,int channel);

static int app_multicast_stop_server(stream_output_server_config *config);
static int app_multicast_restart_server(stream_output_server_config *config);
static int app_config_is_same(stream_output_server_config *config, stream_output_server_config *newconfig);
static int app_mult_set_node_tc(MULTICAST_Output_Server_NODE *node);

/*********************************static*****function****end*******************************************/
extern int app_get_encode_rate(int *vrate, int *arate , int *quality);
extern int stream_server_config_printf(stream_output_server_config *config);


static MULTICAST_OUTPUT_SERVER_HEAD *g_mult_head = NULL;




static int multicast_list_add_node(MULTICAST_Output_Server_NODE *node)
{
	if(g_mult_head == NULL || node == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	MULTICAST_Output_Server_NODE *temp = NULL;
	mid_mutex_lock(g_mult_head->mutex);

	if(g_mult_head->first == NULL) {
		g_mult_head->first = node;

	} else {
		temp = g_mult_head->first;

		while(temp->next != NULL) {
			temp = temp->next;
		}

		temp->next = node;
	}

	g_mult_head->num++;
	PRINTF("g_mult_head->num = %d\n", g_mult_head->num);
	mid_mutex_unlock(g_mult_head->mutex);
	return 0;
}

static int multicast_list_delete_node(MULTICAST_Output_Server_NODE *node)
{
	if(g_mult_head == NULL || node == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	MULTICAST_Output_Server_NODE *temp = NULL;
	//MULTICAST_Output_Server_NODE *temp2 = NULL;
	mid_mutex_lock(g_mult_head->mutex);

	temp = g_mult_head->first;

	PRINTF("temp=%p,node=%p\n", temp, node);

	if(temp == node) {
		g_mult_head->first = node->next;
		g_mult_head->num --;
	} else {
		while(temp->next != node && temp->next != NULL) {
			temp = temp->next;
		}

		if(temp->next == NULL) {
			PRINTF("ERROR,can't find the node\n");
			temp->next = NULL;
		} else {
			temp->next = node->next;
			g_mult_head->num --;
		}
	}

	if(g_mult_head->num < 0) {
		ERR_PRN("\n");
	}

	//close the socket
	//	multicast_client_delete(&(node->client[0]));
	stream_client_close(node->client);

	//close tc num
	tc_del_element(node->tc_num);
	node->tc_num = -1;

	multicast_node_delete(&node);

	mid_mutex_unlock(g_mult_head->mutex);
	multicast_list_print_all();
	return 0;

}

static MULTICAST_Output_Server_NODE *multicast_list_find_node(stream_output_server_config *config)
{
	if(g_mult_head == NULL) {
		ERR_PRN("\n");
		return NULL;
	}

	int ret = 0;
	MULTICAST_Output_Server_NODE *temp = NULL;
	mid_mutex_lock(g_mult_head->mutex);

	temp = g_mult_head->first;

	while(temp != NULL) {
		ret = multicast_compare_config(&(temp->config), config);

		if(ret == 1) {
			PRINTF("i find the node ,for server %s:%d-%d.\n", config->main_ip, config->video_port, config->audio_port);
			break;
		}

		temp = temp->next;
	}

	mid_mutex_unlock(g_mult_head->mutex);

	if(temp == NULL) {
		return NULL;
	}

	return temp;

}


static int multicast_list_inset_node(stream_output_server_config *config)
{
	int ret = 0;
	MULTICAST_Output_Server_NODE *node = NULL;
	//add server node by config
	ret = multicast_list_check_full();

	if(ret != 0) {
		PRINTF("Warnning,the multicast list is full\n");
		return -1;
	}

	node = NULL;
	node = multicast_node_create();

	if(node == NULL) {
		PRINTF("Warnning,the multicast crete node is NULL\n");
		return -1;
	}

	ret = multicast_node_config(config, node);

	if(ret == -1) {
		PRINTF("Warnning,the multicast config node is failed\n");
		multicast_node_delete(&node);
		return -1;
	}

	ret = multicast_list_add_node(node);

	if(ret == -1) {
		PRINTF("Warnning,the multicast add node is failed\n");
		multicast_node_delete(&node);
		return -1;
	}

	//PRINTF the node for debug
	multicast_node_printf(node);
	return 0;

}



static int multicast_list_check_full()
{
	if(g_mult_head == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	int num = 0;
	mid_mutex_lock(g_mult_head->mutex);

	num = multicast_list_get_num(g_mult_head);
	mid_mutex_unlock(g_mult_head->mutex);

	if(num >= MULTICAST_MAX_SERVER_NUM) {
		return 1;
	} else {
		return 0;
	}
}

static void multicast_list_print_all()
{
	if(g_mult_head == NULL) {
		ERR_PRN("\n");
		return ;
	}

	MULTICAST_Output_Server_NODE *node = NULL;
	mid_mutex_lock(g_mult_head->mutex);
	int i = 0;
	node = g_mult_head->first;
	PRINTF("MULT node num=%d\n", g_mult_head->num);

	while(node != NULL) {
		PRINTF("the %d server.\n", i);
		multicast_node_printf(node);
		node = node->next;
		i++;
	}

	mid_mutex_unlock(g_mult_head->mutex);
	return ;
}

static int multicast_list_get_num(MULTICAST_OUTPUT_SERVER_HEAD *head)
{
	if(head == NULL) {
		ERR_PRN("\n");
		return 0;
	}

	int i = 0;
	MULTICAST_Output_Server_NODE *node = head->first;

	while(node != NULL) {
		i++;
		node = node->next;
	}

	if(i != head->num) {
		PRINTF("Warnning,the node num is not same,%d=%d\n", i, head->num);
	}

	head->num = i;
	return i;

}
static int multicast_compare_config(stream_output_server_config *old, stream_output_server_config *new)
{
	if(old->type != new->type) {
		return -1;
	}

	if(strcmp(old->main_ip, new->main_ip) != 0) {
		return -1;
	}

	if(old->video_port != new->video_port) {
		return -1;
	}

	//if(old->type != TYPE_TS) {
	//	if(old->audio_port != new->audio_port) {
	//		return -1;
	//	}
	//}

	return 1;

}



static MULTICAST_Output_Server_NODE *multicast_node_create()
{
	MULTICAST_Output_Server_NODE *node = NULL;
	node = (MULTICAST_Output_Server_NODE *)malloc(sizeof(MULTICAST_Output_Server_NODE));

	if(node == NULL) {
		ERR_PRN("\n");
		return NULL;
	}

	memset(node, 0, sizeof(MULTICAST_Output_Server_NODE));

	node->mutex = mid_mutex_create();

	if(node->mutex == NULL) {
		PRINTF("Error,create mutex is failed\n");
		multicast_node_delete(&node);
		node = NULL;
		return NULL;
	}

	node->tc_num = -1;
	return node;

}

/*校验config之后，重新设置*/
static int multicast_check_config(stream_output_server_config *oldconfig, stream_output_server_config *newconfig)
{
	memcpy(newconfig, oldconfig, sizeof(stream_output_server_config));

	if(newconfig->type == TYPE_TS) {
		//must check ip is multicast ip
	} else if(newconfig->type == TYPE_RTP) {
		//must check ip is not multicast return -1
	}


	if(newconfig->mtu < 0 || newconfig->mtu > 1500) {
		newconfig->mtu = 1500;
	}

	if(newconfig->tos > 0xff) {
		newconfig->tos = 0;
	}

	if(newconfig->ttl > 128) {
		newconfig->ttl = 128;
	}

	return 0;
}

/*rate,ts,rtp,rtsp,rtpts*/
static float RATE_PERCENT[9][5] = {
	{768, 1.228, 1.180, 1.013, 1.244},         //596  0
	{1000, 1.155, 1.141, 1.012, 1.170},           // 1M	1
	{2000, 1.089, 1.081, 1.012, 1.102},           // 2M	2
	{3000, 1.067, 1.055, 1.012, 1.079},           // 3M	3
	{4000, 1.055, 1.044, 1.012, 1.067},           // 4	4
	{5000, 1.050, 1.039, 1.012, 1.060},          // 5	5
	{6000, 1.044, 1.033, 1.012, 1.055},           // 6M	6
	{7000, 1.041, 1.030, 1.012, 1.052},           // 7M	7
	{10000, 1.035, 1.025, 1.012, 1.046},        // 10m  8
};

static int app_calc_rate(int vrate, int arate, int type)
{
	PRINTF("\n");
	int rate = 0;

	int stream_rate = 0;
	float rates = 0;
	float percent = 0.0;
	int i = 0;

	if(type == TYPE_RTP) {
		rate = vrate;
	} else {
		rate = vrate + arate;
	}

	if(rate < 800) {
		rates = rate * RATE_PERCENT[0][type];
		stream_rate = rates;
		return stream_rate;
	} else if(rate > 10000) {
		rates = rate * RATE_PERCENT[8][type];
		stream_rate = rates;
		return stream_rate;
	} else if(rate < 1000) {
		i = 0;
	} else if(rate > 7000) {
		i = 7;
	} else {
		i = rate / 1000;
	}

	percent = (RATE_PERCENT[i][type] + (float)(rate - RATE_PERCENT[i][0]) * (RATE_PERCENT[i][type] - RATE_PERCENT[i + 1][type]) / (RATE_PERCENT[i + 1][0] - RATE_PERCENT[i][0]));
	rates = rate * percent;
	stream_rate = rates;
	PRINTF("rate =%d,stream_rate=%f,percent=%f.", rate, rates, percent);
	return stream_rate;
}

int app_trans_rate(int vrate, int arate, int type)
{
	return app_calc_rate(vrate, arate, type);

}

static int multicast_node_config(stream_output_server_config *config, MULTICAST_Output_Server_NODE *node)
{
	if(config == NULL || node == NULL) {
		return -1;
	}

	int num = 0;
	int vrate = 0;
	int arate = 0;
	int trate = 0;
	int crate = 0;
	int vcbr = 0;
	int stream_channel = 0;
	stream_output_server_config newconfig ;
	memset(&newconfig, 0, sizeof(stream_output_server_config));
	//maybe need set the handle config.
	multicast_check_config(config, &newconfig);
	PRINTF("status=%d==%d\n", newconfig.status, config->status);
	node->config = newconfig;
	PRINTF("status=%d\n", newconfig.status);
	//memcpy(config,&new,sizeof(stream_output_server_config));

	stream_channel = node->config.stream_channel;
	PRINTF("stream_channel = %d\n", stream_channel);

	if(config->status != S_ACTIVE) {
		PRINTF("the config=%d is not active,didn't need init the client.\n", config->status);
		return 0;
	}

	if(node->client != NULL) {
		stream_client_close(node->client);

		//close tc num
		tc_del_element(node->tc_num);
		node->tc_num = -1;
	}

	node->client = NULL;

	char *ip = config->main_ip;
	int vport = config->video_port;


	//ts must have same video and audio port
	if(config->type == TYPE_TS || config->type == TYPE_TS_OVER_RTP) {
		config->audio_port = vport;
	}

	int aport = config->audio_port;
	PRINTF("aport = %d\n", aport);

	struct sockaddr_in video_addr;

	struct sockaddr_in audio_addr;

	int have_audio = 0;

	int ret = 0;

	Stream_Output_Client *client = NULL;

	//memset(&client, 0, sizeof(Stream_Output_Client));

	memset(&video_addr, 0, sizeof(struct sockaddr_in));

	memset(&audio_addr, 0, sizeof(struct sockaddr_in));

	if(aport != 0) {
		have_audio = 1;
	}

	inet_pton(AF_INET, ip, &video_addr.sin_addr);
	video_addr.sin_port = htons(vport);
	video_addr.sin_family = AF_INET;

	if(have_audio) {
		inet_pton(AF_INET, ip, &audio_addr.sin_addr);
		audio_addr.sin_port = htons(aport);
		audio_addr.sin_family = AF_INET;
	}

	client = stream_client_get_free();

	if(client == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	// get video and audio config

	node->client = client;
	stream_client_open(&video_addr, &audio_addr, config->type, client, stream_channel);
	//	ret = multicast_client_create(&video_addr, &audio_addr, config->type, &client);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	//set client socket
	mid_socket_set_tos(client->video_socket, node->config.tos);
	mid_socket_set_tos(client->audio_socket, node->config.tos);

	if(mid_ip_is_multicast(node->config.main_ip) == 1) {
		mid_socket_set_ttl(client->video_socket, node->config.ttl, 1);
		mid_socket_set_ttl(client->audio_socket, node->config.ttl, 1);
	} else {
		mid_socket_set_ttl(client->video_socket, node->config.ttl, 0);
		mid_socket_set_ttl(client->audio_socket, node->config.ttl, 0);
	}

	app_get_encode_rate(&vrate, &arate, &vcbr);

	//node->config.tc_flag = ISUSED;

	if(vcbr == 1 && node->config.tc_flag == ISUSED) {
		//set tc flag
		num = client->num + 1;

		trate = app_trans_rate(vrate, arate, node->config.type);
		crate = trate * (node->config.tc_rate + 100) / 100;
		//set rate
		ret = tc_add_element(num, node->config.main_ip, node->config.video_port, trate, crate);

		if(ret < 0) {
			PRINTF("Warnning\n");
			node->tc_num = -1;
		} else {
			node->tc_num = num;
		}
	} else {
		node->tc_num = -1;
	}

	//	ret = multicast_client_config(&client, config);

	//	if(ret == -1) {
	//		ERR_PRN("\n");
	//		return -1;
	//	}

	mid_mutex_lock(node->mutex);
	node->s_used = ISUSED;
	//	memcpy(&(node->client[0]), &client, sizeof(Stream_Output_Client));
	mid_mutex_unlock(node->mutex);
	return 0;

}


static void multicast_node_printf(MULTICAST_Output_Server_NODE *node)
{
	return ;
	char ch[32] = {0};
	char ch2[32] = {0};
	int port1 = 0;
	int port2 = 0;
	PRINTF("+++++node =%p\n", node);
	PRINTF("+++++server:%s:%d-%d\n", node->config.main_ip, node->config.video_port, node->config.audio_port);
	PRINTF("+++++mtu:%d,client=%p\n", node->config.mtu, node->client);

	if(node->client != NULL) {
		PRINTF("+++++video_socke:%d,audio_socke:%d\n", node->client->video_socket, node->client->audio_socket);

		inet_ntop(AF_INET, &(node->client->video_addr.sin_addr), ch, 16);
		inet_ntop(AF_INET, &(node->client->audio_addr.sin_addr), ch2, 16);
		port1 = ntohs(node->client->video_addr.sin_port);
		port2 = ntohs(node->client->audio_addr.sin_port);
		PRINTF("+++++video:%s:%d,audio:%s:%d\n", ch, port1, ch2, port2);
	}

	return ;
}

static int  multicast_node_delete(MULTICAST_Output_Server_NODE **node)
{
	if(*node == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	//	multicast_client_delete(&(((*node)->client)[0]));
	stream_client_close((*node)->client);
	//close tc num
	tc_del_element((*node)->tc_num);
	(*node)->tc_num = -1;

	(*node)->client = NULL;
	MULTICAST_Output_Server_NODE *temp = *node;

	if(temp->mutex != NULL) {
		mid_mutex_destroy(&(temp->mutex));
	}

	temp->mutex = NULL;
	free(temp);
	*node = NULL;
	return 0;
}


static int multicast_head_delete(MULTICAST_OUTPUT_SERVER_HEAD **head)
{
	if(*head == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	MULTICAST_OUTPUT_SERVER_HEAD *temp = *head;

	if(temp->mutex != NULL) {
		mid_mutex_destroy(&(temp->mutex));
	}

	temp->mutex = NULL;

	free(*head);
	*head = NULL;
	return 0;
}

static int app_mult_node_config_fread(char *config_file, stream_output_server_config *config, char *table)
{
	char 			temp[512] = {0};
	//	int 			ret  = -1 ;
	int 	value = 0;




	if(app_ini_read_int(config_file, table, "status", &value) != -1) {
		config->status = value;
	} else {
		ERR_PRN("failed get [%s] \n", table);
		return -1;
	}

	if(app_ini_read_int(config_file, table, "type", &value) != -1) {
		if(value == TYPE_TS || value == TYPE_RTP || value == TYPE_TS_OVER_RTP) {
			config->type = value;
		} else {
			config->type = TYPE_RTP;
		}
	} else {
		ERR_PRN("failed get [%s] \n", table);
		return -1;
	}

	if(app_ini_read_string(config_file, table, "main_ip", temp, sizeof(temp)) != -1) {
		strcpy(config->main_ip, temp);
	} else {
		ERR_PRN("failed get [%s] \n", table);
		return -1;
	}

	if(app_ini_read_int(config_file, table, "video_port", &value) != -1) {
		config->video_port = value;
	} else {
		ERR_PRN("failed get [%s] \n", table);
		return -1;
	}

	if(app_ini_read_int(config_file, table, "audio_port", &value) != -1) {
		config->audio_port = value;
	} else {
		ERR_PRN("failed get [%s] \n", table);
		return -1;
	}

	if(app_ini_read_int(config_file, table, "mtu", &value) != -1) {
		config->mtu = value;
	}


	if(app_ini_read_int(config_file, table, "ttl", &value) != -1) {
		config->ttl = value;
	}

	if(app_ini_read_int(config_file, table, "tos", &value) != -1) {
		config->tos = value;
	}

	if(app_ini_read_int(config_file, table, "tc", &value) != -1) {
		config->tc_flag = value;
	} else {
		ERR_PRN("failed get [%s] \n", table);
		return -1;
	}

	if(app_ini_read_int(config_file, table, "stream_channel", &value) != -1) {
		if(value < 0 || value >= STREAM_MAX_CHANNEL) {
			ERR_PRN("failed get [%s] \n", table);
			return -1;
		}

		config->stream_channel = value;
	} else {
		ERR_PRN("failed get [%s] \n", table);
		return -1;
	}



	return 0;
}


static int app_mult_config_fread(MULTICAST_OUTPUT_SERVER_HEAD *handle)
{
	char 			temp[512] = {0};
	int 			ret  = -1 ;
	int 	value = 0;
	int i = 0;
	char 			config_file[256] = {0};

	stream_output_server_config config;

	app_check_list_mult_status();
	//	MULTICAST_Output_Server_NODE *node =NULL;


	strcpy(config_file, MULTICAST_SERVER_CONFIG_INI);


	ret = app_ini_read_int(config_file, "total", "num", &value);

	if(ret == -1 || value == 0 || value > MULTICAST_MAX_SERVER_NUM) {
		PRINTF("warning ,the multicast is not used,value=%d\n", value);
		return -1;
	}

	for(i = 0 ; i < value; i++) {
		sprintf(temp, "server%d", i);
		memset(&config, 0, sizeof(stream_output_server_config));
		ret = app_mult_node_config_fread(config_file, &config, temp);

		if(ret == -1) {
			PRINTF("read node from ini is failed\n");
			continue;
		}

		ret = multicast_list_inset_node(&config);

		if(ret == -1) {
			PRINTF("Warnning,the multicast inset node is failed\n");
			return -1;
		}

		//PRINTF the node for debug
		//multicast_node_printf(node);
	}

	return 0;

}




static int app_mult_node_config_fwrite(char *config_name, stream_output_server_config *config, char *section)
{
	int ret = 0;
	int value = 0;
	value = config->tc_flag;
	ret = app_ini_write_int(config_name, section, "tc", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config->tos;
	ret = app_ini_write_int(config_name, section, "tos", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config->ttl;
	ret = app_ini_write_int(config_name, section, "ttl", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}


	value = config->mtu;
	ret = app_ini_write_int(config_name, section, "mtu", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config->audio_port;
	ret = app_ini_write_int(config_name, section, "audio_port", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config->video_port;
	ret = app_ini_write_int(config_name, section, "video_port", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}


	ret = app_ini_write_string(config_name, section, "main_ip", config->main_ip);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}


	value = config->type;
	ret = app_ini_write_int(config_name, section, "type", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config->status;
	ret = app_ini_write_int(config_name, section, "status", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config->stream_channel;
	ret = app_ini_write_int(config_name, section, "stream_channel", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}


	return 0;

}

int app_mult_config_reset()
{
	app_mult_config_fwrite(1);
	return 0;
}

static int app_mult_config_save()
{
	app_mult_config_fwrite(0);
	return 0;
}
static int app_mult_config_fwrite(int reset)
{
	//	return 0;
	PRINTF("\n");

	if(g_mult_head == NULL) {
		PRINTF("Error\n");
		return 0;
	}

	MULTICAST_OUTPUT_SERVER_HEAD *handle = g_mult_head;

	if(handle == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	int total = 0;
	int ret = 0;
	//int value = 0;
	int i = 0;
	char bak_name[512] = {0};
	char section[256] = {0};
	char cmd[512] = {0};
	MULTICAST_Output_Server_NODE *node = NULL;
	sprintf(bak_name, "%s.bak", MULTICAST_SERVER_CONFIG_INI);


	app_check_list_mult_status();

	mid_mutex_lock(handle->mutex);
	/*写接口到bak文件*/
	total = multicast_list_get_num(handle);
	node = handle->first;

	if(reset == 1) {
		total = 0;
	}

	/*write toal num*/
	ret = app_ini_write_int(bak_name, "total", "num", total);

	if(ret == -1) {
		ERR_PRN("\n");
		goto EXIT;
	}

	/*write the node*/
	i = 0;

	while(node != NULL) {
		sprintf(section, "server%d", i);
		ret = app_mult_node_config_fwrite(bak_name, &(node->config), section);

		if(ret == -1) {
			ERR_PRN("\n");
			goto EXIT;
		}

		node = node->next;
		i++;
	}

	/*write the end*/
	ret = app_ini_write_int(bak_name, "end", "flag", 1);

	if(ret == -1) {
		ERR_PRN("\n");
		goto EXIT;
	}

	/*拷贝bak文件到ini*/
	if(ret != -1) {
		sprintf(cmd, "cp -rf %s %s;rm -rf %s", bak_name, MULTICAST_SERVER_CONFIG_INI, bak_name);
		system(cmd);
		PRINTF("i will run cmd:%s\n", cmd);
	}

EXIT:
	mid_mutex_unlock(handle->mutex);

	return ret;
}



static void app_check_list_mult_status()
{
	if(g_mult_head == NULL) {
		PRINTF("Error\n");
		return ;
	}

	mid_mutex_lock(g_mult_head->mutex);
	MULTICAST_Output_Server_NODE *node = g_mult_head->first;

	int direct_ts_status[STREAM_MAX_CHANNEL];
	int direct_rtp_status[STREAM_MAX_CHANNEL];
	int rtp_ts_status[STREAM_MAX_CHANNEL] ;

	int direct_ts_mtu[STREAM_MAX_CHANNEL] ;
	int direct_rtp_mtu[STREAM_MAX_CHANNEL];
	int rtp_ts_mtu[STREAM_MAX_CHANNEL] ;

	int channel = 0;
	int i = 0;

	for(i = 0 ; i < STREAM_MAX_CHANNEL ; i++) {
		direct_ts_status[i] = 0;
		direct_rtp_status[i] = 0;
		rtp_ts_status[i] = 0;

		direct_ts_mtu[i] = 1450;
		direct_rtp_mtu[i] = 1450;
		rtp_ts_mtu[i] = 1450;
	}

	while(node != NULL) {
		channel = node->config.stream_channel;
		PRINTF("channel = %d\n", channel);

		if((node->s_used == ISUSED) && (node->config.type == TYPE_TS)
		   && (node->config.status == S_ACTIVE)) {
			(direct_ts_status[channel])++;

			if(direct_ts_mtu[channel] > node->config.mtu) {
				direct_ts_mtu[channel] = node->config.mtu;
			}
		}

		if((node->s_used == ISUSED) && (node->config.type == TYPE_RTP)
		   && (node->config.status == S_ACTIVE)) {
			(direct_rtp_status[channel])++;

			if(direct_rtp_mtu[channel] > node->config.mtu) {
				direct_rtp_mtu[channel] = node->config.mtu;
			}
		}

		if((node->s_used == ISUSED) && (node->config.type == TYPE_TS_OVER_RTP)
		   && (node->config.status  == S_ACTIVE)) {
			(rtp_ts_status[channel])++;

			if(rtp_ts_mtu[channel] > node->config.mtu) {
				rtp_ts_mtu[channel] = node->config.mtu;
			}
		}

		node = node->next;
	}

	mid_mutex_unlock(g_mult_head->mutex);

	for(i = 0 ; i < STREAM_MAX_CHANNEL ; i++) {
		//	mult_set_active(TYPE_TS, direct_ts_status[i],i);
		//	mult_set_active(TYPE_RTP, direct_rtp_status[i],i);
		//	mult_set_active(TYPE_TS_OVER_RTP, rtp_ts_status[i],i);

		mult_set_mtu(TYPE_TS , direct_ts_mtu[i], i);
		mult_set_mtu(TYPE_RTP , direct_rtp_mtu[i], i);
		mult_set_mtu(TYPE_TS_OVER_RTP , rtp_ts_mtu[i], i);
	}

	return ;

}

void app_multicast_server_init()
{
	if(g_mult_head != NULL) {
		PRINTF("Warnning,the handle is already init\n");
		return ;
	}

	int ret = 0;
	g_mult_head = (MULTICAST_OUTPUT_SERVER_HEAD *)malloc(sizeof(MULTICAST_OUTPUT_SERVER_HEAD));

	if(g_mult_head == NULL) {
		PRINTF("Error,malloc handle is failed \n");
		return ;
	}

	memset(g_mult_head, 0, sizeof(MULTICAST_OUTPUT_SERVER_HEAD));

	g_mult_head->mutex = mid_mutex_create();

	if(g_mult_head->mutex == NULL) {
		PRINTF("Error,create mutex is failed\n");
		multicast_head_delete(&g_mult_head);
		g_mult_head = NULL;
		return;
	}

	//read from flash

	ret = app_mult_config_fread(g_mult_head);

	if(ret == -1) {
		PRINTF("Debug,there is not have the multicast \n");
		//return;
	}

	//print list
	multicast_list_print_all();



	//save to flash
	app_check_list_mult_status();
	app_mult_config_save();

	return;
}



int app_multicast_get_total_num()
{
	if(g_mult_head == NULL) {
		PRINTF("Warnning \n");
		return 0;
	}

	int total = 0;
	mid_mutex_lock(g_mult_head->mutex);
	total = g_mult_head->num;
	mid_mutex_unlock(g_mult_head->mutex);
	return total;
}


/*==============================================================================
    Function: 	<app_multicast_get_config>
    Input: 	1-8
    Output:
    Description:
    Modify:
    Auth: 		zhangmin
    Date: 		2012.04.06
==============================================================================*/
int app_multicast_get_config(int num, stream_output_server_config *config)
{
	if(g_mult_head == NULL || num > MULTICAST_MAX_SERVER_NUM) {
		PRINTF("Warnning,the handle is already init\n");
		return  -1;
	}

	int i = 0;
	MULTICAST_Output_Server_NODE *node = NULL;
	mid_mutex_lock(g_mult_head->mutex);
	node = g_mult_head->first;

	for(i = 0; i < num ; i++) {
		if(node != NULL) {
			node = node->next;
		}
	}

	if(node == NULL) {
		mid_mutex_unlock(g_mult_head->mutex);
		return -1;
	}

	PRINTF("num = %d,node =%p\n", num, node);
	memcpy(config, &(node->config), sizeof(stream_output_server_config));
	mid_mutex_unlock(g_mult_head->mutex);
	//multicast_list_print_all();
	return 0;
}

int app_multicast_add_server(stream_output_server_config *config, stream_output_server_config *newconfig)
{
	int ret = 0;
	MULTICAST_Output_Server_NODE *node = NULL;

	if((config->status == S_DEL) || (config->type == TYPE_RTSP)) {
		ERR_PRN("Error,status=%d,type=%d\n", config->status, config->type);
		return -1;
	}

	node  = multicast_list_find_node(config);

	if(node != NULL) {
		memcpy(newconfig, &(node->config), sizeof(stream_output_server_config));
		ERR_PRN("this config is repeat,add server failed\n");
		return 0;
	}

	stream_server_config_printf(config);
	ret = multicast_list_inset_node(config);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	node  = multicast_list_find_node(config);

	if(node == NULL) {
		//memcpy(newconfig, &(node->config), sizeof(stream_output_server_config));
		ERR_PRN("Error,add server failed\n");
		return 0;
	}

	node->config.status = S_ACTIVE;

	memcpy(newconfig, &(node->config), sizeof(stream_output_server_config));

	//save to flash
	app_check_list_mult_status();
	PRINTF("\n");
	mid_timer_create(3, 1, (mid_func_t)app_mult_config_save);
	//	app_mult_config_fwrite();

	return 0;
}

int app_multicast_delete_server(stream_output_server_config *config)
{
	int ret = 0;
	MULTICAST_Output_Server_NODE *node = NULL;
	node  = multicast_list_find_node(config);

	if(node == NULL) {
		PRINTF("Warnning,this node didn't find.\n");
		return 0;
	}

	ret = multicast_list_delete_node(node);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	node = NULL;

	//save to flash
	app_check_list_mult_status();
	mid_timer_create(3, 1, (mid_func_t)app_mult_config_save);
	//app_mult_config_fwrite();

	return 0;
}

static int app_multicast_stop_server(stream_output_server_config *config)
{
	//int ret = 0;
	MULTICAST_Output_Server_NODE *node = NULL;
	node  = multicast_list_find_node(config);

	if(node == NULL) {
		PRINTF("Warnning,this node didn't find.\n");
		return 0;
	}

	if(config->status == node->config.status) {
		return 0;
	}

	//	multicast_client_delete(&(node->client[0]));
	stream_client_close(node->client);
	//close tc num
	tc_del_element(node->tc_num);
	node->tc_num = -1;

	(node)->client = NULL;
	node->config.status = S_STOP;


	//save to flash
	app_check_list_mult_status();
	mid_timer_create(3, 1, (mid_func_t)app_mult_config_save);
	//app_mult_config_fwrite();

	return 0;
}

static int app_multicast_restart_server(stream_output_server_config *config)
{
	int ret = 0;
	MULTICAST_Output_Server_NODE *node = NULL;
	node  = multicast_list_find_node(config);

	if(node == NULL) {
		PRINTF("Warnning,this node didn't find.\n");
		return 0;
	}

	if(node->config.status == config->status) {
		PRINTF("Error\n");
		return -1;
	}

	node->config.status = config->status;
	//just only change status
	ret = multicast_node_config(&(node->config), node);

	if(ret == -1) {
		return -1;
	}

	//save to flash
	app_check_list_mult_status();
	mid_timer_create(3, 1, (mid_func_t)app_mult_config_save);


	return 0;
}

static int app_config_is_same(stream_output_server_config *config, stream_output_server_config *newconfig)
{
	if(config->type != newconfig->type) {
		return -2;
	}

	//is it rtsp ?
	if(config->type == TYPE_RTSP) {
		return 0;
	}

	if(strcmp(config->main_ip, newconfig->main_ip) != 0) {
		return -1;
	}

	if(config->video_port != newconfig->video_port) {
		return -1;
	}

	if((config->type != TYPE_TS  && config->type != TYPE_TS_OVER_RTP) && config->audio_port != newconfig->audio_port) {
		return -1;
	}

	if((config->mtu != newconfig->mtu) || (config->ttl != newconfig->ttl) || (config->tos != newconfig->tos)) {
		return -3;
	}

	if((config->tc_flag != newconfig->tc_flag) || (config->tc_flag == 1 && config->tc_rate != newconfig->tc_rate)) {
		return -3;
	}

	return 0;
}

int app_multicast_set_config(stream_output_server_config *config, stream_output_server_config *newconfig2)
{
	int ret = 0;
	MULTICAST_Output_Server_NODE *node = NULL;
	node  = multicast_list_find_node(config);

	if(node == NULL) {
		PRINTF("Warnning,this node didn't find.\n");
		return 0;
	}

	config->status = node->config.status;

	ret = app_config_is_same(&(node->config), config);

	if(ret == 0) {
		PRINTF("THIS config is same,not need set\n");
		return 0;
	}




	PRINTF("need to set config ,the config ret = %d,node->config.status=%d=%d\n", ret, node->config.status, config->status);
	multicast_node_config(config, node);


	memcpy(newconfig2, &(node->config), sizeof(stream_output_server_config));
	//save to flash
	app_check_list_mult_status();
	mid_timer_create(3, 1, (mid_func_t)app_mult_config_save);


	return 0;
}

int app_multicast_set_status(stream_output_server_config *config, stream_output_server_config *newconfig)
{
	//int ret = 0;
	//MULTICAST_Output_Server_NODE *node = NULL;


	if(config->status == S_ACTIVE) {
		app_multicast_restart_server(config);
	} else if(config->status == S_STOP) {
		app_multicast_stop_server(config);
	}

	app_check_list_mult_status();
	mid_timer_create(3, 1, (mid_func_t)app_mult_config_save);
	//save to flash
	//	app_mult_config_fwrite();

	return 0;
}







/*设置某个节点的状态*/
static int app_mult_set_node_tc(MULTICAST_Output_Server_NODE *node)
{
	if(node == NULL) {
		PRINTF("Error\n");
		return -1;
	}

	int num = 0;
	int vrate = 0;
	int vcbr = 0;
	int arate = 0;
	int trate = 0;
	int crate = 0;
	int ret = 0;

	app_get_encode_rate(&vrate, &arate, &vcbr);

	if(node->config.tc_flag == NOTUSED || vcbr == 0) {
		if(node->tc_num != -1) {
			tc_del_element(node->tc_num);
			node->tc_num = -1;
		}

		return 0;
	}

	if(node->config.tc_flag == ISUSED) {
		if(node->client == NULL) {
			if(node->tc_num != -1) {
				tc_del_element(node->tc_num);
				node->tc_num = -1;
			}

			return 0;
		}

		num = node->client->num + 1;


		if(vcbr == 1) {
			trate = app_trans_rate(vrate, arate, node->config.type);
			crate = trate * (node->config.tc_rate + 100) / 100;
			//set rate
			ret = tc_add_element(num, node->config.main_ip, node->config.video_port, trate, crate);

			if(ret < 0) {
				PRINTF("Warnning\n");
				node->tc_num = -1;
			} else {
				node->tc_num = num;
			}
		}
	}

	return 0;
}

/*重新设置所有的 tc*/
int app_mult_set_all_tc()
{
	if(g_mult_head == NULL) {
		PRINTF("Warnning \n");
		return 0;
	}

	PRINTF("\n");
	mid_mutex_lock(g_mult_head->mutex);
	MULTICAST_Output_Server_NODE *node = NULL;
	node = g_mult_head->first;

	while(node != NULL) {
		app_mult_set_node_tc(node);
		node = node->next;
	}

	mid_mutex_unlock(g_mult_head->mutex);
	return 0;
}


#if 0
static int g_direct_rtp_active[MAX_CHANNEL] = {0};
static int g_direct_ts_active[MAX_CHANNEL] = {0};
static int g_rtp_over_ts_active[MAX_CHANNEL] = {0};
int mult_rtp_get_active(int channel)
{
	if(g_direct_rtp_active[channel] > 0) {
		return 1;
	} else {
		return 0;
	}
}

int mult_ts_get_active(int channel)
{
	if(g_direct_ts_active[channel] > 0) {
		return 1;
	} else {
		return 0;
	}
}

int mult_rtpts_get_active(int channel)
{
	if(g_rtp_over_ts_active[channel] > 0) {
		return 1;
	} else {
		return 0;
	}
}

static int mult_set_active(int type, int status, int channel)
{
	if(type == TYPE_TS) {
		g_direct_ts_active[channel] = status;
	} else if(type == TYPE_RTP) {
		g_direct_rtp_active[channel] = status;
	} else if(type == TYPE_TS_OVER_RTP) {
		g_rtp_over_ts_active[channel] = status;
	}

	return 0;
}
#endif
static int g_ts_mtu[STREAM_MAX_CHANNEL] = {1450};
static int g_rtp_mtu[STREAM_MAX_CHANNEL] = {1450};
static int g_rtpts_mtu[STREAM_MAX_CHANNEL] = {1450};
int mult_get_direct_rtp_mtu(int channel)
{
	return g_rtp_mtu[channel];
}
int mult_get_direct_ts_mtu(int channel)
{
	return g_ts_mtu[channel];
}
int mult_get_rtp_ts_mtu(int channel)
{
	return g_rtpts_mtu[channel];
}

static int mult_set_mtu(int type, int mtu, int channel)
{
	PRINTF("type=%d,mtu=%d\n", type, mtu);

	if(type == TYPE_TS) {
		g_ts_mtu[channel] = mtu;
	} else if(type == TYPE_RTP) {
		g_rtp_mtu[channel] = mtu;
	} else if(type == TYPE_TS_OVER_RTP) {
		g_rtpts_mtu[channel] = mtu;
	}

	return 0;
}


#endif
