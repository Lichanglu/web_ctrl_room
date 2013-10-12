/*-------------------------------------------------------------------------
          Copyright (C), 2012-2013, Reach Tech. Co., Ltd.   
          File name:              
          Author: 徐崇      Version:        Date: 2012.11.13 19:02:20
	      Description:           
	      Function List:         
	      History:               
		    1. Date: 
  			   Author: 
  			   Modification:
-------------------------------------------------------------------------*/
#include "sercenterctrl.h"

extern server_set *gpser;
#if 1

static int32_t fd ;
//static int32_t recordstate[3] = {0x2,0x2,0x2};
//static int32_t num = 0;
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



SerialHandle *pserial_handle = NULL;

//暂时保存发生状态
Int32  RtRecordState = 0;

zlog_category_t *MLOG;		//zlog 
static Int32   SerialOperate(UInt32 num,void *prm);
static Int32   serial_msg_record(Int8 *send_buf, void *pcmd);
static Int32   Analyze_State(Int8 *recv_buf, void *pparm);
static Int32   Analyze_Volume(Int8 *recv_buf, void *pparm);
static Int32   Analyze_Warnning(Int8 *recv_buf, void *pparm);
static Int32   Analyze_RecordRep(Int8 *recv_buf, void *pparm);
static Int32   NetOperate(UInt32 len,void *xml);
static Int32   serial_msg_fecc(Int8 *send_buf, void *pcmd);
static Int32   serial_msg_down_paly(Int8 *send_buf, void *pcmd);
static Int32   serial_msg_reboot(Int8 *send_buf, void *pcmd);
static Int32   Analyze_replay(Int8 *recv_buf, void *pparm);
//static Int32   serial_msg_replay(Int8 *send_buf, Int8 *replayfile);
static Int32   serial_msg_pic_synthesis(Int8 *send_buf, void *pcmd);
static Int32   Analyze_reboot(Int8 *recv_buf, void *pparm);
static Int32 SerialWrite(Int8 *cmd, UInt32 len);


extern int32_t SetUsbFunction(int32_t IsUse);
extern int32_t GetUsbFunction(int32_t *IsUse);
extern int8_t *basename(int8_t *path);
extern int ClearScreen(int spi_fd);
extern int ShowString(int fd,char *string,int x, int y);
extern int YinCodeShowString(int fd,char *string,int x, int y);
extern int32_t get_leaf_value(int8_t *value, int32_t value_len, xmlChar *key, xmlNodePtr curNode, xmlDocPtr pdoc);
extern int InitLed();
extern uint32_t get_synt_resoluteion();
extern uint32_t get_synt_bitrate();
extern int GetUsbDevState(int *num, MNUsbDeb *stat, int max);



int32_t CheckTimeOut(SerialHandle *pSerialHandle,SERIAL_MSGCODE msgcode);


static SerialComnd globalSerialCmd[]=
{

	// SERIAL  ->   NET

	// 串口录制消息
	{
		RECORD,	//xml信令ID
		0x80,		//串口信令操作ID
		8,			//串口信令长度
		"Record",	//操作名称
		serial_msg_record,	//封装
		NetOperate,
		1
	},

	//串口中控消息
	{
		FECC,
		0x81,
		12,
		"fcc",
		serial_msg_fecc,
		NetOperate,
		1
	},
	//下载消息
	{
		DOWN,
		0x82,
		7,
		"down",
		serial_msg_down_paly,
		NULL,
		1
	},
	//播放消息
	{
		PLAY,
		0x83,
		7,
		"paly",
		serial_msg_down_paly,
		NetOperate,
		1
	},
	//旋转消息
	{
		SPIN,
		0x84,
		7,
		"spin",
		serial_msg_down_paly,
		NULL,
		1
	},

	//重启消息
	{
		REBOOT,
		0x85,
		6,
		"reboot",
		serial_msg_reboot,
		NetOperate,
		1
	},

	//设置合成分辨率
	{
		PICSYN,
		0x86,
		12,
		"PicSynthesis",
		serial_msg_pic_synthesis,
		NetOperate,
		1
	},

	//NET  ->  SERIAL
	// 串口回复消息
	{
		RECORD,	//xml信令ID
		0x92,		//串口信令操作ID
		9,			//串口信令长度
		"RecordRep",	//操作名称
		Analyze_RecordRep,	//封装
		SerialOperate,
		0
	},
	
	//录制状态消息
	{
		RECORDSTATE,
		0x92,
		9,
		"Recordstate",
		Analyze_State,
		SerialOperate,
		0
	},

	//警告消息
	{
		WARNING,
		0x90,
		7,
		"Warning",
		Analyze_Warnning,
		SerialOperate,
		0
	},

	//音量消息
	{
		VOLUME,
		0x93,
		7,
		"Volume",
		Analyze_Volume,
		SerialOperate,
		0
	},

	//回放
	{
		REBOOT,
		0x96,
		7,
		"play",
		Analyze_replay,
		SerialOperate,
		0
	},	

	//重启回复消息
	{
		REBOOT,
		0x96,
		7,
		"rebootRep",
		Analyze_reboot,
		SerialOperate,
		0
	},	
};

static uint32_t list_dir(int8_t *SrcPath)
{
	if((NULL == SrcPath))
	{
		return 0;
	}

	DIR              *pDir ;
	struct dirent    *ent;
	char    Srcchildpath[1024];
	uint32_t     ret = 0;
	uint32_t totalsize = 0;

	pDir = opendir((const char *)SrcPath);
	if(NULL == pDir)
	{
		printf("Cannot open directory:[ %s ]\n",SrcPath);
		return 0;
	}
	
	while((ent = readdir(pDir))!=NULL)
	{
		if(ent->d_type & DT_DIR)
		{
			struct stat stat_buf;
			if(r_strcmp((const int8_t *)ent->d_name, (const int8_t *)".")==0 || r_strcmp((const int8_t *)ent->d_name, (const int8_t *)"..")==0)
				continue;
				
			sprintf(Srcchildpath,"%s/%s",SrcPath,ent->d_name);

			lstat(Srcchildpath, &stat_buf);

	//		printf("%s %u\n",Srcchildpath,(int)stat_buf.st_size);
			totalsize = totalsize + stat_buf.st_size/1024;
			ret = list_dir((int8_t *)Srcchildpath);
		
			totalsize = totalsize + ret;
		}
		else
		{
		
			int8_t LocalFilePath[1024] = {0};
			struct stat stat_buf;
			sprintf((char *)LocalFilePath,"%s/",SrcPath);
	
			r_strncat(LocalFilePath, (const int8_t *)ent->d_name, (size_t)r_strlen((const int8_t *)ent->d_name));
			lstat((const char *)LocalFilePath, &stat_buf);
			totalsize = totalsize + stat_buf.st_size/1024;
			
			//printf("%s %u\n",LocalFilePath,stat_buf.st_size);
		}
	}
	
	if(0 != closedir(pDir))
	{
		assert(0);
	}
	
	return totalsize;
}


#if 0
static int32_t CopyDirectory_ex(int8_t *SrcPath,int8_t *DstPath)    //zl
{
	if((NULL == SrcPath) || (NULL == DstPath))
	{
		return -1;
	}

	DIR              *pDir ;
	struct dirent    *ent;
	char    Srcchildpath[1024];
	char    Dstchildpath[1024];
	int32_t     ret = 1;


	pDir = opendir((const char *)SrcPath);
	if(NULL == pDir)
	{
		printf("Cannot open directory:[ %s ]\n",SrcPath);
		return -1;
	}
	
	while((ent = readdir(pDir))!=NULL)
	{
		if(ent->d_type & DT_DIR)
		{
		
			if(r_strcmp((const int8_t *)ent->d_name,(const int8_t *)".")==0 || r_strcmp((const int8_t *)ent->d_name,(const int8_t *)"..")==0)
				continue;
				
			sprintf(Srcchildpath,"%s/%s",SrcPath,ent->d_name);
			sprintf(Dstchildpath,"%s/%s",DstPath,ent->d_name);
			mkdir(Dstchildpath,   0777);
			ret = CopyDirectory_ex((int8_t *)Srcchildpath,(int8_t *)Dstchildpath);
			if(ret == -1)
			{
				break;
			}
			printf("------> %s\n",Dstchildpath);
			//num = num + ret;
			
		}
		else
		{
		
			int8_t LocalFilePath[1024] = {0};
			int8_t DstFilePath[1024]  = {0};
			FILE *SrcFile = NULL;
			FILE *DstFile = NULL;
			int8_t buffer[4096] = {0};
			int retrun  = 0;
			
			sprintf((char *)LocalFilePath,"%s/",SrcPath);
			sprintf((char *)DstFilePath,"%s/",DstPath);
			r_strncat(LocalFilePath, (const int8_t *)ent->d_name, r_strlen((const int8_t *)ent->d_name));
			r_strncat(DstFilePath, (const int8_t *)ent->d_name, r_strlen((const int8_t *)ent->d_name));

			/* 隐藏文件过滤 */
			if(ent->d_name[0] == '.')
			{
				continue;
			}
			SrcFile = fopen((const char *)LocalFilePath, "r");
			if(SrcFile == NULL)
			{

				  ret = -1;
				   break;
			}
			
			DstFile = fopen((const char *)DstFilePath, "w");
			if(DstFile == NULL)
			{
				  ret = -1;
				  fclose(SrcFile);
				  break;
			}
			printf("------> %s\n",LocalFilePath);

			while(1)
			{
				if(pserial_handle->UsdStat.IsUsbDev != 1)
				{
					printf("pserial_handle->UsdStat.IsUsbDev != 1\n");
					retrun = -1;
					break;
				}
				
				int n,m= 0;
				n = fread(buffer, 1, 4096 , SrcFile);
				if(n <= 0)
				{
		//			printf("-----------read fail---------\n");
					//retrun = -1;
					break;
				}
				
				m = fwrite(buffer,1,n,DstFile);
				if(m != n)
				{
					retrun = -1;
					printf("-----------write fail---------\n");
					break;
				}
		//		printf("-----------write ok---------\n");
			}

			printf("------> %s\n",DstFilePath);
			fclose(SrcFile);
			fclose(DstFile);
			if(retrun == -1)
			{
				ret = -1;
				break;
			}
			
		}
	}

	if(pDir != NULL)
	{
		closedir(pDir);
	}
	
	return ret;
}
#endif
static int32_t CopyMp4File(int8_t *SrcPath,int8_t *DstPath, int32_t fd)
{
	if((NULL == SrcPath) || (NULL == DstPath)){
		return -1;
	}

	DIR 			 *pDir ;
	struct dirent	 *ent;
	int8_t	Srcchildpath[1024];
	int8_t	Dstchildpath[1024];
	int8_t	string_data[102] = {' '};
	int8_t	procee_data[102] = {0};
	int8_t	complete_rate_num[16] = {0};
	
	int32_t 	ret = 1;
	uint32_t	totalsize_count = 0;
	uint32_t	cur_count = 0;
	uint32_t	complete_rate = 0;
	uint32_t	last_complete_rate = 0;

	pDir = opendir((const char *)SrcPath);
	if(NULL == pDir){
		printf("Cannot open directory:[ %s ]\n",SrcPath);
		return -1;
	}
	while((ent = readdir(pDir))!=NULL){
		if((r_strstr((int8_t *)ent->d_name,(int8_t *)".mp4") != NULL)
			&& (r_strstr((int8_t *)ent->d_name,(int8_t *)"HD") != NULL))
		{
			struct stat stat_buf;

			memset(Srcchildpath, 0, 1024);
			sprintf((char *)Srcchildpath,"%s/",SrcPath);
			r_strncat(Srcchildpath, (const int8_t *)ent->d_name, r_strlen((const int8_t *)ent->d_name));
			
			stat((char *)Srcchildpath, &stat_buf);
			totalsize_count = totalsize_count + stat_buf.st_size/4096;
		}
	}
	seekdir(pDir, 0);
	ShowString(fd,(char *)"0",51,50);
	while((ent = readdir(pDir))!=NULL){
		// 只拷贝根目录下的mp4文件
		if((r_strstr((int8_t *)ent->d_name,(int8_t *)".mp4") != NULL)
			&& (r_strstr((int8_t *)ent->d_name,(int8_t *)"HD") != NULL))
		{
			FILE *SrcFile = NULL;
			FILE *DstFile = NULL;
			int8_t buffer[4096] = {0};
			int retrun	= 0;

			memset(Srcchildpath, 0, 1024);
			memset(Dstchildpath, 0, 1024);
			sprintf((char *)Srcchildpath,"%s/",SrcPath);
			sprintf((char *)Dstchildpath,"%s/",DstPath);
			r_strncat(Srcchildpath, (const int8_t *)ent->d_name, r_strlen((const int8_t *)ent->d_name));
			r_strncat(Dstchildpath, (const int8_t *)ent->d_name, r_strlen((const int8_t *)ent->d_name));

			/* 隐藏文件过滤 */
			if(ent->d_name[0] == '.'){
				continue;
			}
			SrcFile = fopen((const char *)Srcchildpath, "r");
			if(SrcFile == NULL){
				ret = -1;
				break;
			}
			
			DstFile = fopen((const char *)Dstchildpath, "w");
			if(DstFile == NULL){
				ret = -1;
				fclose(SrcFile);
				break;
			}
			printf("------> %s\n",Dstchildpath);
			
			while(1){
				if(pserial_handle->UsdStat.IsUsbDev != 1){
					printf("pserial_handle->UsdStat.IsUsbDev != 1\n");
					retrun = -1;
					break;
				}
				
				int n,m= 0;
				n = fread(buffer, 1, 4096 , SrcFile);
				if(n <= 0){
		//			printf("-----------read fail---------\n");
					//retrun = -1;
					break;
				}
				
				m = fwrite(buffer,1,n,DstFile);
				if(m != n){
					retrun = -1;
					printf("-----------write fail---------\n");
					break;
				}
				cur_count += 100;
				if(totalsize_count > 0){
					complete_rate = cur_count/totalsize_count;
					complete_rate /= 4;
					if((complete_rate > last_complete_rate) && (complete_rate <= 27)){
						r_memset(string_data, ' ', 102);
						r_memset(procee_data, 0, 102);
						r_memset(complete_rate_num, 0, 16);
						r_strncpy(procee_data, string_data, complete_rate+1);
						sprintf((char *)complete_rate_num, "%2d", cur_count/totalsize_count);
						YinCodeShowString(fd,(char *)procee_data,0,50);
						r_usleep(2000);
						ShowString(fd,(char *)complete_rate_num,51,50);
						fprintf(stderr, "\n\ncur_copy.....   = %2d%%, procee_data = %d\n\n\n", cur_count/totalsize_count, complete_rate);
					}
					last_complete_rate = complete_rate;
				}
			}

			printf("------> %s\n",Dstchildpath);
			fclose(SrcFile);
			fclose(DstFile);
			if(retrun == -1){
				ret = -1;
				break;
			}
		}
	}
	
	if(pDir != NULL){
		closedir(pDir);
	}

//	YinCodeShowString(fd,(char *)"100% complete !",6,50);
	
	return ret;
}


static int32_t CopyDir(int8_t *SrcPath, int8_t *DstPath, int fd)
{
	int8_t buffer[1024] = {0};
	if(SrcPath == NULL || DstPath == NULL)
	{
		return -1;
	}


	sprintf((char *)buffer,"%s/%s" ,DstPath,basename((int8_t *)SrcPath));
	printf("----->buffer %s\n",buffer);

	mkdir((char *)buffer,0777);
	
	return CopyMp4File(SrcPath,buffer, fd);

}

#if 0
static Int32 serial_msg_fecctest(Int8 *send_buf, void *pcmd)
{

	UInt8 roomid[10] = {0};
	UInt8 encid[10]  = {0};
	UInt8 type[10]   = {0};
	UInt8 addr[10]   = {0};
	UInt8 speed[10]  = {0};
	UInt8 Num[10]    = {0};

	

	sprintf((char *)roomid,"%d", 0);
	sprintf((char *)encid, "%d", 3);
	sprintf((char *)type,  "%d", 1);
	sprintf((char *)addr,  "%d", 1);
	sprintf((char *)speed, "%d", 5);
	sprintf((char *)Num,   "%d", 0);
		
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr RemoteCtrlReq 		= NULL;
	xmlNodePtr Roomifo		 		= NULL;
	xmlNodePtr EncInfo		 		= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30013");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"ComControl");

	RemoteCtrlReq = xmlNewNode(NULL, BAD_CAST "RemoteCtrlReq");
	xmlAddChild(body_node, RemoteCtrlReq);

	Roomifo = xmlNewNode(NULL, BAD_CAST "RoomInfo");
	xmlAddChild(RemoteCtrlReq, Roomifo);
	
	package_add_xml_leaf(Roomifo, (const xmlChar *)"RoomID", (const int8_t *)roomid);
	
	EncInfo = xmlNewNode(NULL, BAD_CAST "EncInfo");
	xmlAddChild(Roomifo, EncInfo);
	
	package_add_xml_leaf(EncInfo, (const xmlChar *)"ID",   (const int8_t *)encid);
	package_add_xml_leaf(EncInfo, (const xmlChar *)"Type", (const int8_t *)type);
	package_add_xml_leaf(EncInfo, (const xmlChar *)"Addr", (const int8_t *)addr);
	package_add_xml_leaf(EncInfo, (const xmlChar *)"Speed",(const int8_t *)speed);
	package_add_xml_leaf(EncInfo, (const xmlChar *)"Num",  (const int8_t *)Num);

	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}
#endif

static void TimeoutLock()
{
	pthread_mutex_lock(&pserial_handle->timeout);
}

static void TimeoutunLock()
{
	pthread_mutex_unlock(&pserial_handle->timeout);
}

static Int32 TimeoutLockInit()
{
	return pthread_mutex_init(&pserial_handle->timeout, NULL);
}

static Int32 TimeoutdeInit()
{
	return pthread_mutex_destroy(&pserial_handle->timeout);
}



static void SerialLock()
{
	pthread_mutex_lock(&pserial_handle->seriallock);
}

static void SerialunLock()
{
	pthread_mutex_unlock(&pserial_handle->seriallock);
}

static void SerialLock1()
{
	pthread_mutex_lock(&pserial_handle->seriallock1);
}

static void SerialunLock1()
{
	pthread_mutex_unlock(&pserial_handle->seriallock1);
}


static Int32 SerialLockInit()
{
	return pthread_mutex_init(&pserial_handle->seriallock, NULL);
}

static Int32 SerialLockdeInit()
{
	return pthread_mutex_destroy(&pserial_handle->seriallock);
}

static Int32 SerialLockInit1()
{
	return pthread_mutex_init(&pserial_handle->seriallock1, NULL);
}

static Int32 SerialLockdeInit1()
{
	return pthread_mutex_destroy(&pserial_handle->seriallock1);
}



static void TcpSockLock()
{
	pthread_mutex_lock(&pserial_handle->socklock);
}

static void TcpSockunLock()
{
	pthread_mutex_unlock(&pserial_handle->socklock);
}

static Int32 TcpSockLockInit()
{
	return pthread_mutex_init(&pserial_handle->socklock, NULL);
}

static Int32 TcpSockLockdeInit()
{
	return pthread_mutex_destroy(&pserial_handle->socklock);
}

int32_t upper_msg_set_time(struct  timeval *time)
{
	if(time == NULL)
	{
		return -1;
	}

	struct  timeval  time_now ;
	time_now.tv_sec = 0;
	time_now.tv_usec = 0;
	gettimeofday(&time_now, 0);

	time->tv_sec = time_now.tv_sec;
	time->tv_usec = time_now.tv_usec;

	return 0;
}


int32_t upper_msg_tim_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
	if(x->tv_sec > y->tv_sec) {
		return   -1;
	}

	if((x->tv_sec == y->tv_sec) && (x->tv_usec > y->tv_usec)) {
		return   -1;
	}

	result->tv_sec = (y->tv_sec - x->tv_sec);
	result->tv_usec = (y->tv_usec - x->tv_usec);

	if(result->tv_usec < 0) {
		result->tv_sec--;
		result->tv_usec += 1000000;
	}

	return   0;
}


int32_t upper_msg_monitor_time_out_status(struct  timeval *time, uint32_t time_out)
{

	struct  timeval  time_now ;
	time_now.tv_sec = 0;
	time_now.tv_usec = 0;
	struct  timeval  time_old;
	time_old.tv_sec = 0;
	time_old.tv_usec = 0 ;
	struct  timeval delta_time ;
	delta_time.tv_sec = 0;
	delta_time.tv_usec = 0;

	gettimeofday(&time_now, 0);
	time_old.tv_sec = time->tv_sec;
	time_old.tv_usec = time->tv_usec;

	if(-1 == upper_msg_tim_subtract(&delta_time, &time_old, &time_now)) {
	
		return -1;
	}

	if(delta_time.tv_sec > time_out) {

		//超时
		return 1;
	}

	//未超时
	return 0;	
}

int32_t AddTimeOut(SerialHandle *pSerialHandle,SERIAL_MSGCODE msgcode, UInt8 state)
{
	TimeOut *pattr = NULL;
	
	TimeoutLock();
	pattr = (TimeOut *)malloc(sizeof(TimeOut)); 
	pattr->msgcode = msgcode;
	pattr->state = state;
	upper_msg_set_time(&pattr->starttime);
	list_add_before(&pattr->stlist, pSerialHandle->pheadnode);
	TimeoutunLock();

	return 0;
}

int32_t DelTimeOut(SerialHandle *pSerialHandle,SERIAL_MSGCODE msgcode)
{
	list_head *pcurnode = NULL;
	TimeOut *pattr = NULL;

	TimeoutLock();
	/* 查找是否存在该设备 */
	list_for_each_item(pcurnode, pSerialHandle->pheadnode)  
    {  
        if(NULL != pcurnode)  
        {  
            pattr = list_entry(pcurnode, TimeOut, stlist);  

		 	if(pattr->msgcode == msgcode)
		 	{
				if((pattr->state > 3) || (pattr->state < 1))
				{
					return 0;
				}

				#if 0
				g_rtprm.close = 2;
				g_rtprm.start = 2;
				g_rtprm.stop  = 2;
				
				if(pattr->state == 1)
				{
					g_rtprm.start = 1;
				}
				else if(pattr->state == 2)
				{
					g_rtprm.stop  = 1;
				}
				else if(pattr->state == 3)
				{
					g_rtprm.close = 1;
				}
				#endif
				list_remove(pcurnode);
				free(pattr);
				
				TimeoutunLock();
				//此消息等待超时
				return 1;
			}
        }  
    }  
	TimeoutunLock();

	return 0;
}

#if 0
static int setSerialParity(int fd,int databits,int stopbits,int parity)
{ 
	struct termios options;	if(tcgetattr(fd,&options)  !=  0) { 
		return(FALSE);  
       }

    options.c_cflag &= ~HUPCL;	
    options.c_lflag = 0;
    options.c_iflag = IGNBRK;
    options.c_oflag = 0;
	options.c_cc[VTIME] = 0;    
	options.c_cc[VMIN] = 0;
	switch (databits) 
	{   
		case 7:           
			options.c_cflag |= CS7; 
			break;
		case 8:     
			options.c_cflag |= CS8;
			break;   
		default:    
			return (FALSE);  
	}
	switch (parity) 
	{   
		case 'n':
		case 'N':    
			options.c_cflag &= ~PARENB;  	/* Clear parity enable */
			options.c_iflag &= ~INPCK;	 	/* Enable parity checking */ 
			break;  
		case 'o':   
		case 'O':     
			options.c_cflag |= (PARODD | PARENB); 
			options.c_iflag |= INPCK;    	/* Disnable parity checking */ 
			break;  
		case 'e':  
		case 'E':   
			options.c_cflag |= PARENB;     	/* Enable parity */    
			options.c_cflag &= ~PARODD;  
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;
		case 'S': 
		case 's':  							/*as no parity*/   
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;			
			break;  
		default:   
         return (FALSE);  
	}  

			options.c_cflag &= ~PARENB;  	/* Clear parity enable */
			options.c_iflag &= ~INPCK;	 	/* Enable parity checking */ 

	switch (stopbits)
	{   
		case 1:    
			options.c_cflag &= ~CSTOPB;  
			break;  
		case 2:    
			options.c_cflag |= CSTOPB;  
			break;
		default:    
			return (FALSE); 
	} 
	/* Set input parity option */ 
	if (parity != 'n')   
		options.c_iflag |= INPCK; 
	tcflush(fd,TCIFLUSH); 				/* define the minimum bytes data to be readed*/
	if (tcsetattr(fd,TCSANOW,&options) != 0)   
	{ 
		return (FALSE);  
	} 
	return (TRUE);  
}
#endif
void setSerialSpeed(int fd, int speed)
{
	int i; 
	int status; 
	struct termios Opt;
	int speed_arr[] = { B115200,B38400, B19200, B9600, B4800, B2400, B1200, B300,
                      B38400, B19200, B9600, B4800, B2400, B1200, B300, };
	int name_arr[] = {115200,38400,  19200,  9600,  4800,  2400,  1200,  300, 38400,  
                                   19200,  9600, 4800, 2400, 1200,  300, };

	tcgetattr(fd, &Opt); 
	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) 
	{ 
		if  (speed == name_arr[i]) 
		{
                tcflush( fd, TCIOFLUSH) ;
                cfsetispeed( &Opt, speed_arr[i] ) ;
                cfsetospeed( &Opt, speed_arr[i] ) ;
                status = tcsetattr(fd,TCSANOW,&Opt);
                if( 0!= status)
                {
                	perror( "tcsetattr error !\n" ) ;
                	return;
                }
                tcflush( fd, TCIOFLUSH) ;
                //return status; 
		}  
	}
}
#if 0
static int openSerialPort(char *port_num)
{
	//return -1;
	int fd;
	if ((fd = open(port_num, O_RDWR)) < 0) {
		printf("ERROR: failed to open %s, errno=%d\n",port_num,errno);	
		return -1;
	}else {		
		printf("Open %s success!\n",port_num);		
	}		
	return fd;
}
#endif

#if 0
static int initSerialPort(char *port_num,int baudrate,
				int databits,int stopbits,int parity)
{
	int fd = -1;
	fd=openSerialPort(port_num);
	if(fd == -1) {
		printf("open port failed \n");
		return -1;
	}
	setSerialSpeed(fd,baudrate);		
	if(setSerialParity(fd,databits,stopbits,parity)==FALSE) {			
		printf("Set Parity Error!\n");			
		if(fd != -1)
			close(fd);
		return -1;		
	}
	return fd;
}
#endif

/* 解析消息类型 */
static int32_t Analyze_MsgCode(int8_t *recv_buf, int32_t *msgcode)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	xmlNodePtr msghead = NULL;
	xmlNodePtr MsgCode = NULL;

	
	char *pMsgCode = NULL;

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_MsgCode: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_MsgCode: init_dom_tree fail");
		goto EXIT;
	}


	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		zlog_error(MLOG, "Analyze_MsgCode: msghead fail");
		goto EXIT;
	}
	
	MsgCode   = get_children_node(msghead, BAD_CAST "MsgCode");
	if(MsgCode == NULL)
	{
		zlog_error(MLOG, "Analyze_MsgCode: not found usrname");
		goto EXIT;
	}
	
	pMsgCode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, MsgCode->xmlChildrenNode, 1);	
	if(pMsgCode == NULL)
	{
		zlog_error(MLOG, "Analyze_MsgCode: not found usrname");
		goto EXIT;
	}
	*msgcode = atoi(pMsgCode);
	return_code = 1;
	
EXIT:
	if(NULL != pMsgCode)
	{
		xmlFree(pMsgCode);
	}
	
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}



static void SetTcpSocket(int socket)
{
	
	TcpSockLock();
	pserial_handle->socket = socket;
	TcpSockunLock();
}

static int GetTcpSocket()
{
	return pserial_handle->socket;
}

static void SetSerialFd(int serialFd)
{
	
	SerialLock();
	pserial_handle->serialFd = serialFd;
	SerialunLock();
}

static void SetSerialFd1(int serialFd)
{
	
	SerialLock();
	pserial_handle->serialFd1 = serialFd;
	SerialunLock();
}

static int GetSerialFd()
{
	return pserial_handle->serialFd;
}

static int GetSerialFd1()
{
	return pserial_handle->serialFd1;
}

static int ReplayingMenu(int spi_fd,int flg)
{
	int8_t *p = (int8_t *)(pserial_handle->FileList + (pserial_handle->ListPosition-1)*50);
	vodplay_ctrl ctrl;
	char Downsendcmd[]={0x3c,0x3c,0xc2,0x94,0x2,0x3e,0x3e};
	char Playcmd[]	  ={0x3c,0x3c,0xc2,0x95,0x2,0x3e,0x3e};
	
	r_memset(&ctrl, 0, sizeof(vodplay_ctrl));
	ClearScreen((int)fd);

	if(pserial_handle->SeekLeve < SEEK_LEVEL_B4)
	{
		pserial_handle->SeekLeve = SEEK_LEVEL_B4;
	}
	else if(pserial_handle->SeekLeve > SEEK_LEVEL_F32)
	{
		pserial_handle->SeekLeve = SEEK_LEVEL_F32;
	}
	
	if(flg == VOD_PLAY)
	{
		char cmd[256] = {0};
		sprintf(cmd,"%s",p);
		printf("ReplayingMenu VOD_PLAY : %s\n",p);
		ShowString(spi_fd,"    Playing Back      ",10,10);
		ShowString(spi_fd,cmd,5,25);
		ctrl.ctrl_mode = VODCTRL_PLAY;
		if(p != NULL && !pserial_handle->CourseNameChange)
		{
			strcpy((char *)ctrl.filename,(const char *)p);
			strcpy((char *)pserial_handle->CourseName,(const char *)p);
			ctrl_vodplay_ctrl(&ctrl);	
		}
	}
	else if(flg == VOD_FF)
	{
		char cmd[256] = {0};
		ShowString(spi_fd,"    Playing Back      ",10,10);
		sprintf(cmd,"%s",pserial_handle->CourseName);
		ShowString(spi_fd,cmd,5,25);

		sprintf(cmd,"    Forward    x%d    ",pserial_handle->SeekLeve);
		ShowString(spi_fd,cmd,10,40);

		ctrl.ctrl_mode = VODCTRL_SEEK;
		ctrl.seek_level =  pserial_handle->SeekLeve;
		ctrl_vodplay_ctrl(&ctrl);
	}
	else if(flg == VOD_REW)
	{
		char cmd[256] = {0};
		ShowString(spi_fd,"    Playing Back      ",10,10);	
		sprintf(cmd,"%s",pserial_handle->CourseName);
		ShowString(spi_fd,cmd,5,25);

		sprintf(cmd,"    Backward   x%d    ",pserial_handle->SeekLeve*(-1));
		ShowString(spi_fd,cmd,10,40);
		ctrl.ctrl_mode = VODCTRL_SEEK;
		ctrl.seek_level = pserial_handle->SeekLeve;
		ctrl_vodplay_ctrl(&ctrl);
	}
	else if(flg == VOD_PAUSE)
	{
		char cmd[256] = {0};
		sprintf(cmd,"%s",pserial_handle->CourseName);
		ShowString(spi_fd,"    Playing Back      ",10,10);
		ShowString(spi_fd,cmd,5,25);
		ShowString(spi_fd,"    Pausing       ",10,40);
		ctrl.ctrl_mode = VODCTRL_PAUSE;
		ctrl_vodplay_ctrl(&ctrl);
		usleep(10000);
		Playcmd[4] = 0x1;
		SerialWrite(Playcmd, 7);
		usleep(1000*100);
		SerialWrite(Downsendcmd,7);
	}
	else if(flg == VOD_STOP)
	{
		char cmd[256] = {0};
		sprintf(cmd,"%s",pserial_handle->CourseName);
		ShowString(spi_fd,"    Playing Back      ",10,10);
		ShowString(spi_fd,cmd,5,25);
		ShowString(spi_fd,"    Stoping       ",10,40);
		ctrl.ctrl_mode = VODCTRL_STOP;
		ctrl_vodplay_ctrl(&ctrl);
		memset(pserial_handle->CourseName, 0x0, sizeof(pserial_handle->CourseName));
		sleep(1);
	}
	else if(flg == VOD_RESUME)
	{
		char cmd[256] = {0};
		ctrl.ctrl_mode = VODCTRL_RESUME;
		ctrl_vodplay_ctrl(&ctrl);
		sprintf(cmd,"%s",pserial_handle->CourseName);
		ShowString(spi_fd,"    Playing Back      ",10,10);
		ShowString(spi_fd,cmd,5,35);
	}
	return 1;
}

/*==============================================================================
    函数: <serial_msg_record>
    功能: <xh_Func:>请求录制消息封装
    参数: 
    Created By 徐崇 2012.11.05 19:53:40 For Ftp
==============================================================================*/
static Int32 serial_msg_record(Int8 *send_buf, void *pcmd)
{
	int8_t buffer[1024] = {0};
	Int8 state[3] = {0};
	Int8 roomid[10] = {0};
	UInt8 *cmd = (UInt8 *)pcmd;
	if((send_buf == NULL)||(cmd == NULL) || (cmd[3] != 0x80 ))
	{
		return -1;
	}
	
	//g_rtprm.close = 2;
	//g_rtprm.start = 2;
	//g_rtprm.stop  = 2;

	//开始录制
	if(cmd[5] == 1)
	{
		r_strcpy((int8_t *)state, (const int8_t *)"1");
		//g_rtprm.start = 1;
	}

	//暂停录制
	else if(cmd[5] == 2)
	{
		r_strcpy((int8_t *)state, (const int8_t *)"2");
		//g_rtprm.stop  = 1;
	}
	//关闭录制
	else if(cmd[5] == 3)
	{
		r_strcpy((int8_t *)state, (const int8_t *)"0");
		//g_rtprm.close = 1;
	}
	else
	{
		return -1;
	}

	
	printf("pserial_handle->LedState == REPLAYINGMENU) && (pserial_handle->PlayState >= 1\n");
	//停止预览
	if((pserial_handle->LedState == REPLAYINGMENU) && (pserial_handle->PlayState >= 1))
	{
		//char Playcmd[] 	={0x3c,0x3c,0xc2,0x95,0x2,0x3e,0x3e};
		char Playcmd[]	={0x3c,0x3c,0xc2,0x95,0x2,0x3e,0x3e};
		char Optcmd[]	={0x3c,0x3c,0xc2,0x92,0x2,0x2,0x2,0x3e,0x3e};

		printf("--------------cmd[5]:[%d]\n", cmd[5]);

		// -----fix me.....------ 4620 	Bug缺陷	新建	B1 中等缺陷	点播回放中按下暂停按钮，按钮无背光显示
		
		//停止回放
		if(cmd[5] == 3)
		{
			pserial_handle->ListPosition = 0;
			pserial_handle->totalfilenum = 0;

			Optcmd[6] = 0x1;
			SerialWrite(Optcmd,9);

			ReplayingMenu(fd,VOD_STOP);
			pserial_handle->LedState = IPMENU;
			pserial_handle->LedFLUSH = 1;
			pserial_handle->PlayState = 0;
			
			Optcmd[6] = 0x2;
			SerialWrite(Optcmd,9);

			Playcmd[4] = 0x2;
			SerialWrite(Playcmd,7);

			return -1;
		}
		//暂停回放
		else if(cmd[5] == 2)
		{

			ReplayingMenu(fd,VOD_PAUSE);
			pserial_handle->PlayState = VOD_PAUSE;

			//SerialWrite(Playcmd,7);
			//usleep(1000*100);

			Optcmd[5] = 0x1;
			SerialWrite(Optcmd,9);

			printf("\n\n\npserial_handle->LedState == REPLAYINGMENU) && (pserial_handle->PlayState >= 1\n\n\n");

			return -1;
		}
		//继续回放
		else if(cmd[5] == 1)
		{
			ReplayingMenu(fd,VOD_RESUME);
			pserial_handle->PlayState = VOD_PLAY;
			return -1;
		}
	}

	if (pserial_handle->PlayState >= VOD_PLAY 
		&& pserial_handle->PlayState <= VOD_REW)
	{
		YinCodeShowString(fd," Please stop play ",10,30);
		printf("\n\nPlaying..., PlayState[%d], give up rec operation, just return.......\n\n\n", pserial_handle->PlayState);
	}
		
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr ctrl_node 			= NULL;


	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30010");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"ComControl");

	ctrl_node = xmlNewNode(NULL, BAD_CAST "RecCtrlReq");
	xmlAddChild(body_node, ctrl_node);

	sprintf(roomid,"%d",cmd[4]);
	srand(time(0));
	sprintf((char *)buffer,"%d",(int)(rand()+ cmd[4]));
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"RoomID",  (const int8_t *)roomid);
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"RecordID", (const int8_t *)buffer);
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"OptType", (const int8_t *)state);
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"RoomName", (const int8_t *)"");
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"AliasName", (const int8_t *)"");
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"TeacherName", (const int8_t *)"");
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"CourseName", (const int8_t *)"");
	package_add_xml_leaf(ctrl_node, (const xmlChar *)"Notes", (const int8_t *)"");
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}


static Int32 serial_msg_fecc(Int8 *send_buf, void *pcmd)
{

	UInt8 roomid[10] = {0};
	UInt8 encid[10]  = {0};
	UInt8 type[10]   = {0};
	UInt8 addr[10]   = {0};
	UInt8 speed[10]  = {0};
	UInt8 Num[10]    = {0};

	UInt8 *cmd = (UInt8 *)pcmd;
	if((send_buf == NULL)||(cmd == NULL) || (cmd[3] != 0x81 ))
	{
		return -1;
	}

	sprintf((char *)roomid,"%d", cmd[4]);
	sprintf((char *)encid, "%d", cmd[5]);
	sprintf((char *)type,  "%d", cmd[6]);
	sprintf((char *)addr,  "%d", cmd[7]);
	sprintf((char *)speed, "%d", cmd[8]);
	sprintf((char *)Num,   "%d", cmd[9]);
		
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr RemoteCtrlReq 		= NULL;
	xmlNodePtr Roomifo		 		= NULL;
	xmlNodePtr EncInfo		 		= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30013");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"ComControl");

	RemoteCtrlReq = xmlNewNode(NULL, BAD_CAST "RemoteCtrlReq");
	xmlAddChild(body_node, RemoteCtrlReq);

	Roomifo = xmlNewNode(NULL, BAD_CAST "RoomInfo");
	xmlAddChild(RemoteCtrlReq, Roomifo);
	
	package_add_xml_leaf(Roomifo, (const xmlChar *)"RoomID", (const int8_t *)roomid);
	
	EncInfo = xmlNewNode(NULL, BAD_CAST "EncInfo");
	xmlAddChild(Roomifo, EncInfo);
	
	package_add_xml_leaf(EncInfo, (const xmlChar *)"ID",   (const int8_t *)encid);
	package_add_xml_leaf(EncInfo, (const xmlChar *)"Type", (const int8_t *)type);
	package_add_xml_leaf(EncInfo, (const xmlChar *)"Addr", (const int8_t *)addr);
	package_add_xml_leaf(EncInfo, (const xmlChar *)"Speed",(const int8_t *)speed);
	package_add_xml_leaf(EncInfo, (const xmlChar *)"Num",  (const int8_t *)Num);

	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

#if 0
static Int32 serial_msg_replay(Int8 *send_buf, Int8 *replayfile)
{

	UInt8 roomid[10] = {0};

	if((send_buf == NULL)||(replayfile == NULL))
	{
		return -1;
	}
		
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr Replay 				= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30048");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"ComControl");

	Replay = xmlNewNode(NULL, BAD_CAST "Replay");
	xmlAddChild(body_node, Replay);
	
	package_add_xml_leaf(Replay, (const xmlChar *)"RoomID", (const int8_t *)roomid);
	package_add_xml_leaf(Replay, (const xmlChar *)"PlayFile", (const int8_t *)replayfile);

	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}
#endif

int32_t Insertsort(int8_t *array, int8_t *file ,int32_t num)
{
//	printf("num : %d file : %s \n",num,file);
	
	int32_t index = 0;
	int32_t flag = 0;

	int32_t temp_int_src = 0;
	int32_t temp_int_des = 0;
	int8_t	temp_str_src[11] = {0};
	int8_t	temp_str_des[11] = {0};

	int8_t  *temp_str = array;

	int8_t  temp_conversion_in[50] = {0};
	int8_t  temp_conversion_out[50] = {0}; 

	if(num == 0)
	{
		r_memset(array,0,50);
		strncpy((char *)array,(char *)file,strlen((char *)file));
		return 0;
	}
	
	for(index =0; index < num; index++)
	{
		r_memset(temp_str_src , 0, 11);
		r_memset(temp_str_des , 0, 11);
		
		r_memcpy(temp_str_src ,(temp_str + 4),10);
		r_memcpy(temp_str_des ,(file + 4),10);
			
		temp_int_src = atoi((char *)temp_str_src);
		temp_int_des = atoi((char *)temp_str_des);
		if(temp_int_des > temp_int_src)
		{
			strncpy((char *)temp_conversion_out,(char *)file,strlen((char *)file));
			for(; index < num ; index ++)
			{
				r_memcpy(temp_conversion_in,temp_str,50);
				r_memcpy(temp_str,temp_conversion_out,50);
				r_memcpy(temp_conversion_out,temp_conversion_in,50);
				temp_str = temp_str + 50;
			}
			r_memcpy(temp_str,temp_conversion_in,50);

			
			flag = 1;
			break;
		}
		else if(temp_int_des == temp_int_src)
		{
			printf("IMPORT ERROR ! --------- \n");
		}
		else
		{
			;
		}
		temp_str = temp_str + 50;
	}
	if(flag == 0)
	{
		temp_str = array + num *50 ;
		r_memset(temp_str,0,50);
		strncpy((char *)temp_str,(char *)file,strlen((char *)file));
	}

	return 0;
}

#if 0

int32_t SerGetFileList(int8_t *RepairPath, int32_t totalnum,int8_t *list)
{
	if(NULL == RepairPath)
	{
		return -1;
	}
	
	DIR              *pDir;
	struct dirent    *ent;

	DIR              *pDir2;
	struct dirent    *ent2;

	uint32_t	 totalsize = 0;
	int32_t      num = 0;
	int8_t       childpath[RECORD_FILE_MAX_FILENAME*2];
	int8_t       filepath[RECORD_FILE_MAX_FILENAME*2];
	int8_t		 temp[29] = {0};
	
	pDir = opendir((char *)RepairPath);
	if(NULL == pDir){
		printf("Cannot open directory: %s\n",RepairPath);
		return -1;
	}
	
	while((ent = readdir(pDir))!=NULL){
		memset(childpath, 0, RECORD_FILE_MAX_FILENAME*2);
		if(ent->d_type & DT_DIR){
			int8_t md5info[RECORD_FILE_MAX_FILENAME*2] = {0};
			if((r_strcmp((int8_t *)ent->d_name,(int8_t*)".")==0) || (r_strcmp((int8_t *)ent->d_name,(int8_t*)"..")==0)\
			        || (0 == r_strcmp((int8_t *)ent->d_name,(int8_t*)MP4_REPAIR_PATH)))
				continue;
				
			sprintf((char *)childpath,"%s/%s",(char *)RepairPath,(char *)ent->d_name);
			sprintf((char *)md5info,"%s/md5.info",childpath);
			if(0 == access((char *)md5info,0)){
				if(list != NULL){	
					if(strlen((char *)ent->d_name) < 50){
						totalsize = 0;
						pDir2 = opendir((char *)childpath);
						while((ent2 = readdir(pDir2))!=NULL){
							if((r_strstr((int8_t *)ent2->d_name,(int8_t *)".mp4") != NULL)
								&& (r_strstr((int8_t *)ent2->d_name,(int8_t *)"HD") != NULL))
							{
								struct stat stat_buf;

								memset(filepath, 0, RECORD_FILE_MAX_FILENAME*2);
								sprintf((char *)filepath,"%s/",childpath);
								r_strncat(filepath, (const int8_t *)ent2->d_name, r_strlen((const int8_t *)ent2->d_name));
								
								stat(filepath, &stat_buf);
								totalsize = totalsize + stat_buf.st_size/1024/1024;
							}
						}
						closedir(pDir2);
			
						memset(list, 0x20, 50);
						strncpy((char *)list,(const char *)ent->d_name,strlen((char *)ent->d_name));
						
						list += 16;
						memset(list, 0x20, 34);
						strncpy((char *)list,(const char *)"...  ",5);
						
						list += 5;
						memset(list, 0x20, 29);
						memset(temp, 0, 29);
						sprintf(temp, "%u MB", totalsize);		// 文件大小
			//			sprintf(temp, "%uMB", 1024);

						memset(list, 0x20, 29);
						strncpy((char *)list,(const char *)temp, 29);
						list += 29;
					}
				}
				
				num++;
				if(num >= totalnum){
					break;
				}
			}
				
		}
		else{
			continue;
		}
	}
	
	if(0 != closedir(pDir))
	{
	}
	
	return num;
}

#endif

int32_t SerGetFileList(int8_t *RepairPath, int32_t totalnum,int8_t *list)
{
	if(NULL == RepairPath)
	{
		return -1;
	}
	
	int32_t index = 0;
	int8_t *list_temp = list;
	DIR              *pDir ;
	struct dirent	 *ent;
	int32_t 	 num = 0;
	int8_t		 childpath[RECORD_FILE_MAX_FILENAME*2];
	
	pDir = opendir((char *)RepairPath);
	if(NULL == pDir)
	{
		printf("Cannot open directory: %s\n",RepairPath);
		return -1;
	}
	
	while((ent = readdir(pDir))!=NULL)
	{
		if(ent->d_type & DT_DIR)
		{
			int8_t md5info[RECORD_FILE_MAX_FILENAME*2] = {0};
			if((r_strcmp((int8_t *)ent->d_name,(int8_t*)".")==0) || (r_strcmp((int8_t *)ent->d_name,(int8_t*)"..")==0)\
					|| (0 == r_strcmp((int8_t *)ent->d_name,(int8_t*)MP4_REPAIR_PATH)))
				continue;
				
			sprintf((char *)childpath,"%s/%s",(char *)RepairPath,(char *)ent->d_name);
			sprintf((char *)md5info,"%s/md5.info",childpath);
			if(0 == access((char *)md5info,0))
			{
				if(list != NULL)
				{	
					if(strlen((char *)ent->d_name) < 50)
					{
					//	memset(list, 0x0, 50);
					//	strncpy((char *)list,(const char *)ent->d_name,strlen((char *)ent->d_name));
					//	list = list+50;
						Insertsort((int8_t *)list,(int8_t *)ent->d_name,num);
						num ++ ;
						list_temp = list;
					}
				}
				if(num >= totalnum)
				{
					break;
				}
			}
				
		}
		else
		{
			continue;
		}
	}
	
	if(0 != closedir(pDir))
	{
	}
	printf("IMPORT INFO  ------------------------------------------------------- num : %d\n\n\n",num);
	for(index = 0; index < num ; index ++)
	{
		printf("IMPORT INFO <NO : %d > <FILE : %s> \n",index ,list);
		list = list + 50 ;
	}
	
	return num;
}





static int DownIngMenu(int spi_fd, char*path)
{
	int8_t *p = (int8_t *)(pserial_handle->FileList + (pserial_handle->ListPosition-1)*50);
	int8_t buffer[100];
	int8_t cmd[1024];

	r_memset(buffer, 0, 100);
	r_memset(cmd, 0, 100);

	int32_t ret = 0;
	uint32_t UsbFreeSize = 0;


	
	sprintf((char *)buffer,"File: %s",p);
	sprintf((char *)cmd,"/opt/Rec/%s",p);
	
	UsbFreeSize = list_dir(cmd) + 1024;


	if(strlen(path) <= 0){	
		ShowString(fd,"Not found valid device",10,30);
		return 0;
	}
	printf("UsbFreeSize: %u\n",UsbFreeSize);
	printf("FreeSize: %u\n",pserial_handle->UsdStat.FreeSize);
	if((pserial_handle->UsdStat.FreeSize <= UsbFreeSize)){
		ShowString(fd,"Space is not enough",10,30);
		sleep(2);
		return 0;
	}
	
	ShowString(spi_fd,(char *)"State:  Copying",0,15);
	ShowString(spi_fd,(char *)buffer,0,30);

	sleep(1);
	ret = CopyDir((int8_t *)cmd,(int8_t *)path, spi_fd);

	fprintf(stderr, "path = %s\n", path);

	if(1 == ret){
		ClearScreen(fd);
		ShowString(fd,"   Download done   ",10,20);
		ShowString(fd,"   Syncing........   ",10,40);
	}
	else{
		ClearScreen(fd);
		ShowString(fd,"   Download fail   ",10,30);
	}
	system("sync");
	
	return 0;
}

static int DownFileMenu(int spi_fd)
{	
	int8_t FileName[50] = {0};
	int8_t *plistpostion = (int8_t *)pserial_handle->FileList;

	if(pserial_handle->ListPosition == 0)
	{
		pserial_handle->ListPosition = 1;
	}

	
	if((pserial_handle->totalfilenum == 0)|| (pserial_handle->totalfilenum < pserial_handle->ListPosition))
	{
		ClearScreen(fd);
		ShowString(spi_fd,"  Not Found Courseware ",0,27);
		pserial_handle->ListPosition = pserial_handle->totalfilenum;
	}
	else
	{	
		int32_t i       = 0;
		int32_t page    = 0;
		int32_t select  = 0;
		int32_t maxpage = 0;
		int32_t postion = 0;
		//计算页数
		page    = (pserial_handle->ListPosition -1)/4;
		maxpage = (pserial_handle->totalfilenum -1)/4;
		
		//计算选项
		select = (pserial_handle->ListPosition - 1)%4;
		plistpostion = plistpostion + page*4*50;
		
		if(page < maxpage)
		{
			postion = 3;
		}
		else
		{
			postion = (pserial_handle->totalfilenum -1)%4;
		}
		
		for(i = 0; i <= postion; i++)
		{
			memset(FileName,0x20,40);
			FileName[39] = '\0';
			//strlen((const char *)plistpostion + i*50);
			memcpy((char *)FileName,(char *)(plistpostion + i*50),strlen((const char *)(plistpostion + i*50)));
				
			//strncpy(FileName,plistpostion + i*50, 49);
			//printf("%s\n",FileName);
			if(select == i)
			{
				YinCodeShowString(spi_fd,(char *)FileName,0,(5+i*15));

				//memset(pserial_handle->cur_file,0,50);
				//strncpy(pserial_handle->cur_file, FileName, 49);
			}
			else
			{
				ShowString(spi_fd,(char *)FileName,0,(5+i*15));
			}
			
		}

		for(i = postion+1; i < 4; i++)
		{
			ShowString(spi_fd,(char *)"                                   ",0,(5+i*15));
		}
	}
	
	return 1;
}

void *CopyFileTask(void *args)
{
	ClearScreen(fd);
	Int32 record  = 1;
	record = check_all_record_status(gpser);

	if(record == 1){
		ShowString(fd,"Please stop record",10,30);
		sleep(2);
	}
	else if((pserial_handle->UsdStat.IsUsbDev == 1)){
		if(pserial_handle->IsUse == 1){
			DownIngMenu(fd,(char *)pserial_handle->UsdStat.UsbDevName);
		}
		else{
			ShowString(fd,"Please turn on usb function! ",0,20);
			sleep(2);
		}
			
	}
	else{
		ShowString(fd,"Please input usb dev first!  ",4,30);
		sleep(2);
	}
	
	char Downsendcmd[]={0x3c,0x3c,0xc2,0x94,0x2,0x3e,0x3e};
	SerialWrite(Downsendcmd,7);
	
	usleep(10000);
	ClearScreen(fd);
	
	pserial_handle->LedState = IPMENU;
	pserial_handle->LedFLUSH = 1;

	return NULL;
}

static Int32 serial_msg_down_paly(Int8 *send_buf, void *pcmd)
{
	UInt8 *cmd = (UInt8 *)pcmd;
	if((send_buf == NULL)||(cmd == NULL))
	{
		return -1;
	}

	char Downsendcmd[]={0x3c,0x3c,0xc2,0x94,0x2,0x3e,0x3e};
	char Playcmd[]	  ={0x3c,0x3c,0xc2,0x95,0x2,0x3e,0x3e};
	char Optcmd[]	={0x3c,0x3c,0xc2,0x92,0x2,0x2,0x2,0x3e,0x3e};

	printf("-------serial_msg_down_paly--------cmd[3] = %x\n", cmd[3]);
	
	//down
	if(cmd[3] == 0x82)
	{
		
		//printf("----------------------------> %d\n",pserial_handle->LedState);
		//ClearScreen(fd);
		int fielnum = 0;
		if(pserial_handle->LedState == DOWNINGMENU)
	    {
			Downsendcmd[4] = 0x1;
			//DownIngMenu(fd);
		}
		else if(pserial_handle->LedState != DOWNMENU)
		{
			pserial_handle->LedState = DOWNMENU;
			fielnum = SerGetFileList((int8_t*)"/opt/Rec",100,(int8_t *)pserial_handle->FileList);
			pserial_handle->ListPosition = 1;
			pserial_handle->totalfilenum = fielnum;
			Downsendcmd[4] = 0x1;
			DownFileMenu(fd);
		}
		else
		{
			pserial_handle->ListPosition = 0;
			pserial_handle->totalfilenum = 0;
			pserial_handle->LedState = IPMENU;
			pserial_handle->LedFLUSH = 1;
			Downsendcmd[4] = 0x2;
		}

		usleep(1000*100);
		SerialWrite(Downsendcmd,7);

		//前面板bug 太快没反应
		usleep(1000*100);
		Playcmd[4] = 0x2;
		SerialWrite(Playcmd,7);

		printf("----------------down-----------------\n");
	} 
	//play
	else if(cmd[3] == 0x83)
	{
		//ClearScreen(fd);
		int fielnum = 0;

		if ((pserial_handle->LedState == REPLAYINGMENU) && (pserial_handle->PlayState >= 1)) 
		{
			pserial_handle->ListPosition = 0;
			pserial_handle->totalfilenum = 0;
			pserial_handle->LedState = IPMENU;
			pserial_handle->LedFLUSH = 1;
			Playcmd[4] = 0x2;
		}
		else if (pserial_handle->LedState == REPLAYINGMENU || pserial_handle->PlayState >= 1) 
		{
			Playcmd[4] = 0x1;
			pserial_handle->LedState = REPLAYINGMENU ;
			pserial_handle->CourseNameChange = 1;
			ReplayingMenu(fd, pserial_handle->PlayState);
			pserial_handle->CourseNameChange = 0;
		} 
		else if(pserial_handle->LedState != REPLAYMENU)
		{
			pserial_handle->LedState = REPLAYMENU;
			fielnum = SerGetFileList((int8_t*)"/opt/Rec",100,(int8_t *)pserial_handle->FileList);
			pserial_handle->ListPosition = 1;
			pserial_handle->totalfilenum = fielnum;
			Playcmd[4] = 0x1;
			DownFileMenu(fd);
		}
		else 
		{
			pserial_handle->ListPosition = 0;
			pserial_handle->totalfilenum = 0;
			pserial_handle->LedFLUSH = 1;
			pserial_handle->LedState = IPMENU;
			pserial_handle->PlayState = 0;
			Playcmd[4] = 0x2;
		}

		usleep(1000*100);
		SerialWrite(Playcmd,7);
		usleep(1000*100);
		Downsendcmd[4] = 0x2;
		SerialWrite(Downsendcmd,7);
		printf("----------------play-----------------\n");
	} 
	//spin
	else if(cmd[3] == 0x84)
	{

		printf("-------serial_msg_down_paly---spin-----cmd[3:%x][4:%x], LedState:%d\n", cmd[3], cmd[4], pserial_handle->LedState);


		printf("%d----------------------\n",pserial_handle->LedState);
		if(pserial_handle->LedState == DOWNMENU || pserial_handle->LedState == REPLAYMENU)
		{
		//	printf("----------------spin-----------------\n");
			//正旋转
			if(cmd[4] == 1)
			{		
				printf("-------serial_msg_down_paly---spin-----1111111\n");
				pserial_handle->ListPosition++;
				printf("111 ->%d\n",pserial_handle->ListPosition);
				DownFileMenu(fd);
			}
			//饭旋转
			else if (cmd[4] == 2)
			{
				printf("-------serial_msg_down_paly---spin-----222222222\n");

				pserial_handle->ListPosition--;
				if(pserial_handle->ListPosition < 0)
				{
					pserial_handle->ListPosition = 0;
				}
			
			//	printf("111 ->%d\n",pserial_handle->ListPosition);
				DownFileMenu(fd);
			}

			//确定
			else if(cmd[4] == 3)
			{
				if (pserial_handle->LedState == DOWNMENU) 
				{
					pthread_t proc;
					
					pserial_handle->LedState = DOWNINGMENU;
		
					r_pthread_create(&proc,NULL, CopyFileTask, (void *)pserial_handle);
					r_pthread_detach(proc);
				}
				else if (pserial_handle->LedState == REPLAYMENU)
				{
					Int32 record  = 1;
					record = check_all_record_status(gpser);
					
					if(record == 1){
						YinCodeShowString(fd," Please stop record ",10,30);
						sleep(2);
						return -1;
					}
				
					//开始播放
					pserial_handle->LedState =	REPLAYINGMENU;
					pserial_handle->PlayState = VOD_PLAY;
					pserial_handle->SeekLeve  = 0;
					ReplayingMenu(fd,VOD_PLAY);
					//serial_msg_replay(send_buf, pserial_handle->cur_file);
					
				}
			}
			else
			{
	
			}

		//	SerialWrite(sendcmd,7);
		}
		else if(pserial_handle->PlayState >= 1)
		{
			printf("-------serial_msg_down_paly---spin--1---pserial_handle->SeekLeve [%d]\n", pserial_handle->SeekLeve);
		
			//快进
			if(cmd[4] == 1)
			{					
				pserial_handle->SeekLeve++;
				
				if (pserial_handle->SeekLeve > 0)
					ReplayingMenu(fd,VOD_FF);
				else if (pserial_handle->SeekLeve < 0)
					ReplayingMenu(fd,VOD_REW);
				else
					ReplayingMenu(fd,VOD_REW);
			}
			//快退
			else if(cmd[4] == 2)
			{
				pserial_handle->SeekLeve--;
				
				if (pserial_handle->SeekLeve > 0)
					ReplayingMenu(fd,VOD_FF);
				else if (pserial_handle->SeekLeve < 0)
					ReplayingMenu(fd,VOD_REW);
				else
					ReplayingMenu(fd,VOD_FF);
			}
			//播放
			else if(cmd[4] == 3)
			{
				ReplayingMenu(fd,VOD_RESUME);
				
				if (pserial_handle->PlayState == VOD_PAUSE)
					pserial_handle->PlayState = VOD_PLAY;
								
				Optcmd[4] = 0x2;
				Optcmd[5] = 0x2;
				Optcmd[5] = 0x2;
				SerialWrite(Optcmd,9);
			}
			printf("-------serial_msg_down_paly---spin--2---pserial_handle->SeekLeve [%d]\n", pserial_handle->SeekLeve);
			
		}
		else if(pserial_handle->LedState == SELECTUSBMENU)
		{
			int num = 0;
			MNUsbDeb dev[5] = {{{0}, 0, 0, 0}};
			int i = 0;
			//等待设备挂载
			
			GetUsbDevState(&num, dev, 4);
	
			if(num <= 0)
			{
				pserial_handle->LedState = IPMENU;
				pserial_handle->LedFLUSH = 1;
			}
			else 
			{	
				if(num >4 )
					num = 4;

					//快进
				if(cmd[4] == 1)
				{	
					pserial_handle->usbpostin++;

					char cmd[1024] = {0};
					for(i =0; i < num; i++)
					{
						float size = dev[i].free_size;
						size	= size/1024/1024;
						sprintf(cmd,"Vol:%s Free:%0.2fG",dev[i].devname,size);
						
						if(i == (pserial_handle->usbpostin%num))
						{
							YinCodeShowString(fd,cmd,1,(5+i*15));
						}
						else
						{
							ShowString(fd,cmd,1,(5+i*15));
						}
					}
				}
				//快退
				else if(cmd[4] == 2)
				{
					pserial_handle->usbpostin--;
					char cmd[1024] = {0};
					for(i =0; i < num; i++)
					{
						float size = dev[i].free_size;
						size	= size/1024/1024;
						sprintf(cmd,"Vol:%s Free:%0.2fG",dev[i].devname,size);
						
						if(i == (pserial_handle->usbpostin%4))
						{
							YinCodeShowString(fd,cmd,1,(5+i*15));
						}
						else
						{
							ShowString(fd,cmd,1,(5+i*15));
						}
					}
				}
				//播放
				else if(cmd[4] == 3)
				{
					strcpy((char *)pserial_handle->UsdStat.UsbDevName,dev[pserial_handle->usbpostin%num].devname);
					pserial_handle->UsdStat.TotalSize= dev[pserial_handle->usbpostin%num].total_size;
					pserial_handle->UsdStat.FreeSize = dev[pserial_handle->usbpostin%num].free_size;

					ClearScreen(fd);
					ShowString(fd,"     Select succed     ",12,20);
					sleep(1);
					//ReplayingMenu(fd,VOD_RESUME);
					pserial_handle->LedState = IPMENU;
					pserial_handle->LedFLUSH = 1;
				}

					
			}
		}
		
		return 1;
	}
	else
	{
		return -1;
	}

	return 1;
}

static Int32 serial_msg_reboot(Int8 *send_buf, void *pcmd)
{
	UInt8 *cmd = (UInt8 *)pcmd;
	if((send_buf == NULL)||(cmd == NULL) || (cmd[3] != 0x85 ))
	{
		return -1;
	}
		
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node			= NULL;
	xmlNodePtr body_node			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);

	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30012");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"ComControl");
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static Int32 serial_msg_pic_synthesis(Int8 *send_buf, void *pcmd)
{
	UInt8 *cmd = (UInt8 *)pcmd;
	if((send_buf == NULL)||(cmd == NULL) || (cmd[3] != 0x86 ))
	{
		return -1;
	}
		
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node			= NULL;
	xmlNodePtr body_node			= NULL;
	xmlNodePtr PictureSynthesis_node			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);

	//head
	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30047");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"ComControl");

	//body
	PictureSynthesis_node = xmlNewNode(NULL, BAD_CAST "PictureSynthesis");
	xmlAddChild(body_node, PictureSynthesis_node);

	package_add_xml_leaf(PictureSynthesis_node, (const xmlChar *)"RoomID", (const int8_t *)"30047");
	package_add_xml_leaf(PictureSynthesis_node, (const xmlChar *)"RoomID", (const int8_t *)"30047");
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}


#if 0
//--------------------------------------------------------------测试------------------------------------------------------//
static int32_t test_volume_rep_msg(char *send_buf, char * state, char *volume)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"ReponseMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr volume_node 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30019");
	package_add_xml_leaf(head_node, (const xmlChar *)"ReturnCode", (const int8_t *)state);

	volume_node = xmlNewNode(NULL, BAD_CAST "VoiceInfo");
	xmlAddChild(body_node, volume_node);

	package_add_xml_leaf(volume_node, (const xmlChar *)"Volume", (const int8_t *)volume);
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

static int32_t test_warning_msg(char *send_buf, char *cmd)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr warnning 			= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"10009");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"ComControl");

	warnning = xmlNewNode(NULL, BAD_CAST "warn");
	xmlAddChild(body_node, warnning);

	package_add_xml_leaf(warnning, (const xmlChar *)"id", (const int8_t *)cmd);
	package_add_xml_leaf(warnning, (const xmlChar *)"source", (const int8_t *)"");
	package_add_xml_leaf(warnning, (const xmlChar *)"roomid", (const int8_t *)"");
	package_add_xml_leaf(warnning, (const xmlChar *)"codecid", (const int8_t *)"");
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

//--------------------------------------------------------------测试------------------------------------------------------//
static int32_t test_state_msg(char *send_buf)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr recstate 			= NULL;
	xmlNodePtr recIp 				= NULL;
	xmlNodePtr room 				= NULL;

	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30024");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"ComControl");

	recstate = xmlNewNode(NULL, BAD_CAST "RecServerStatusUpdateReq");
	xmlAddChild(body_node, recstate);

	recIp = xmlNewNode(NULL, BAD_CAST "RecServerIP");
	xmlAddChild(recstate, recIp);
	room = xmlNewNode(NULL, BAD_CAST "RoomStatus");
	xmlAddChild(recstate, room);

	package_add_xml_leaf(room , (const xmlChar *)"RoomId", (const int8_t *)"0");
	package_add_xml_leaf(room , (const xmlChar *)"ConnStatus", (const int8_t *)"0");
	package_add_xml_leaf(room , (const xmlChar *)"Quality", (const int8_t *)"0");
	package_add_xml_leaf(room , (const xmlChar *)"RecStatus", (const int8_t *)"1");
	package_add_xml_leaf(room , (const xmlChar *)"RecName", (const int8_t *)"0");
	package_add_xml_leaf(room , (const xmlChar *)"IfMark", (const int8_t *)"0");
	package_add_xml_leaf(room , (const xmlChar *)"Status1", (const int8_t *)"1");
	package_add_xml_leaf(room , (const xmlChar *)"Status2", (const int8_t *)"1");
	package_add_xml_leaf(room , (const xmlChar *)"Status3", (const int8_t *)"1");
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);
	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}
#endif


static Int32 Analyze_State(Int8 *recv_buf, void *pparm)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	int32_t i  = 0;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr RecServerStatusUpdateReq = NULL;
	xmlNodePtr RoomStatus = NULL;
		xmlNodePtr		Warns = NULL;
	xmlNodePtr RecStatus = NULL;
	xmlNodePtr		Warn = NULL;
	xmlNodePtr Status[3] = {0};
	
	UInt8 *pRecStatus = NULL;
	UInt8 *pStatus[3] = {0};

	SerialParam *parm = (SerialParam *)(pparm);
	if(parm == NULL)
	{
		zlog_error(MLOG, "Analyze_State: parm is NULL");
		return -1;
	}
	
	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_State: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_State: init_dom_tree fail");
		goto EXIT;
	}

	if(is_req_msg(parse_xml_user->proot) != 1) 
	{		
		zlog_error(MLOG, "Analyze_State: is_req_msg fail");
		goto EXIT;
	}	

	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	if(msgbody == NULL)
	{
		zlog_error(MLOG, "Analyze_State: msghead fail");
		goto EXIT;
	}
	
	RecServerStatusUpdateReq   = get_children_node(msgbody, BAD_CAST "RecServerStatusUpdateReq");
	if(RecServerStatusUpdateReq == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found usrname");
		goto EXIT;
	}

	RoomStatus   = get_children_node(RecServerStatusUpdateReq, BAD_CAST "RoomStatus");
	if(RoomStatus == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found usrname");
		goto EXIT;
	}

	RecStatus   = get_children_node(RoomStatus, BAD_CAST "RecStatus");
	if(RecStatus == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found usrname");
		goto EXIT;
	}
	
	pRecStatus  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, RecStatus->xmlChildrenNode, 1);	
	if(pRecStatus == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found usrname");
		goto EXIT;
	}

	UInt8 recordstate;
	parm->close = 0x2;		//
	parm->start = 0x2;
	parm->stop  = 0x2;
	recordstate = (UInt8)atoi((const char *)pRecStatus);
	if(recordstate == 0) //停止
	{
	//	parm->close = 0x1;	
	}
	//开始
	else if(recordstate == 1)
	{
		parm->start = 0x1;
	}
	// 暂停
	else if(recordstate == 2)
	{
		parm->stop = 0x1;
	}
	else
	{
		zlog_error(MLOG, "Analyze_State: error state [%d]",recordstate);
		goto EXIT;
	}

	parm->videostate[0] = 0x1;
	parm->videostate[1] = 0x1;
	parm->videostate[2] = 0x1;
	parm->warning 		= 0x2;
	Status[0]   = get_children_node(RoomStatus, BAD_CAST "Status1");
	pStatus[0]  = (UInt8*)xmlNodeListGetString(parse_xml_user->pdoc, Status[0]->xmlChildrenNode, 1);	
	if(pStatus[0] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status1");
		goto EXIT;
	}

	Status[1]   = get_children_node(RoomStatus, BAD_CAST "Status2");
	pStatus[1]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[1]->xmlChildrenNode, 1);	
	if(pStatus[1] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status2");
		goto EXIT;
	}

	Status[2]   = get_children_node(RoomStatus, BAD_CAST "Status3");
	pStatus[2]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[2]->xmlChildrenNode, 1);	
	if(pStatus[2] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status3");
		goto EXIT;
	}
	#if 0   // zl question???
	// add zl
		Status[2]   = get_children_node(RoomStatus, BAD_CAST "Status3");
	pStatus[2]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[2]->xmlChildrenNode, 1);	
	if(pStatus[2] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status3");
		goto EXIT;
	}
		Status[2]   = get_children_node(RoomStatus, BAD_CAST "Status3");
	pStatus[2]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[2]->xmlChildrenNode, 1);	
	if(pStatus[2] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status3");
		goto EXIT;
	}
		Status[2]   = get_children_node(RoomStatus, BAD_CAST "Status3");
	pStatus[2]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[2]->xmlChildrenNode, 1);	
	if(pStatus[2] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status3");
		goto EXIT;
	}
		Status[2]   = get_children_node(RoomStatus, BAD_CAST "Status3");
	pStatus[2]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[2]->xmlChildrenNode, 1);	
	if(pStatus[2] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status3");
		goto EXIT;
	}
		Status[2]   = get_children_node(RoomStatus, BAD_CAST "Status3");
	pStatus[2]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[2]->xmlChildrenNode, 1);	
	if(pStatus[2] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status3");
		goto EXIT;
	}
		Status[2]   = get_children_node(RoomStatus, BAD_CAST "Status3");
	pStatus[2]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[2]->xmlChildrenNode, 1);	
	if(pStatus[2] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status3");
		goto EXIT;
	}
		Status[2]   = get_children_node(RoomStatus, BAD_CAST "Status3");
	pStatus[2]  = (UInt8 *)xmlNodeListGetString(parse_xml_user->pdoc, Status[2]->xmlChildrenNode, 1);	
	if(pStatus[2] == NULL)
	{
		zlog_error(MLOG, "Analyze_State: not found Status3");
		goto EXIT;
	}
	#endif
#if 0
	for(i = 0; i <3 ;i++)
	{
		UInt8 videostate = 0x2;
		videostate = atoi(pStatus[i]);

		/* 有源 */
		if(videostate == 1)
		{
			parm->videostate[i] = 0x1;
		}
		/* 无源 */
		else if(videostate == 0)
		{
			parm->videostate[i] = 0x2;
		}
		else
		{
			zlog_error(MLOG, "Analyze_State: not found videostate%d[%d]",i,videostate);
			goto EXIT;
		}
	
	}
#endif

	Warns   = get_children_node(RecServerStatusUpdateReq, BAD_CAST "Warns");
	if(Warns == NULL)
	{
	//	zlog_error(MLOG, "Analyze_State: not found Warns");
	//	goto EXIT;
	}
	else
	{

		int8_t value[32] = {0};
		int8_t value1[32] = {0};
		Warn = get_children_node(Warns, BAD_CAST "Warn");	
		for (; Warn != NULL; Warn = find_next_node(Warn, BAD_CAST "Warn")) 
		{

			bzero(value, sizeof(value));
			get_leaf_value(value, sizeof(value), BAD_CAST "CodecID", Warn, parse_xml_user->pdoc);

			bzero(value1, sizeof(value1));
			get_leaf_value(value1, sizeof(value1), BAD_CAST "ID", Warn, parse_xml_user->pdoc);

			if(value1[0] == '4')
			{
				if(value[0] == '1' || value[0] == '2' || value[0] == '3')
				{
					
					parm->videostate[ value[0] - '1'] = 0x2;
				}
				else
				{
					continue;
				}
				
			}

			parm->warning = 0x1;
		}
	}
	
	return_code = 1;
	
EXIT:
	if(NULL != pRecStatus)
	{
		xmlFree(pRecStatus);
	}

	for(i = 0; i <3 ;i++)
	{
		if(NULL != pStatus[i])
		{
			xmlFree(pStatus[i]);
		}
	}
	
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}


/*==============================================================================
    函数: <serial_msg_record>
    功能: <xh_Func:>音量消息下发
    参数: 
    Created By 徐崇 2012.11.05 19:54:19 For Ftp
==============================================================================*/
static int32_t test_volume_req_msg(char *send_buf)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr volume_node 			= NULL;
	
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);


	package_add_xml_leaf(head_node, (const xmlChar *)"MsgCode", (const int8_t *)"30019");
	package_add_xml_leaf(head_node, (const xmlChar *)"PassKey", (const int8_t *)"ComControl");

	volume_node = xmlNewNode(NULL, BAD_CAST "GetVolumeReq");
	xmlAddChild(body_node, volume_node);

	package_add_xml_leaf(volume_node, (const xmlChar *)"RoomID", (const int8_t *)"0");
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);

	xmlFree(temp_xml_buf);

	if(doc != NULL){
		release_dom_tree(doc);
	}

	
	return size;
}




/*==============================================================================
    函数: <Analyze_Volume>
    功能: <xh_Func:>解析音量大小
    参数: 
    Created By 徐崇 2012.11.06 09:06:54 For Ftp
==============================================================================*/
static Int32 Analyze_Volume(Int8 *recv_buf, void *pparm)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode   = NULL;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr volumeinfo = NULL;
	xmlNodePtr volume = NULL;
	char *pvolume = NULL;
	char *pretcode = NULL;

	SerialParam *parm = (SerialParam *)(pparm);
	if(parm == NULL)
	{
		zlog_error(MLOG, "Analyze_State: parm is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_Volume: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_Volume: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		zlog_error(MLOG, "Analyze_Volume: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		zlog_error(MLOG, "Analyze_Volume: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		zlog_error(MLOG, "Analyze_Volume: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		zlog_error(MLOG, "Analyze_Volume: not found pretcode");
		goto EXIT;
	}

	UInt32 irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		zlog_error(MLOG, "Analyze_Volume: ReturnCode [%d]",irecode);
		goto EXIT;
	}
	
	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	if(retcode == NULL)
	{
		zlog_error(MLOG, "Analyze_Volume: retcode fail");
		goto EXIT;
	}
	
	volumeinfo   = get_children_node(msgbody, BAD_CAST "VoiceInfo");
	if(volumeinfo == NULL)
	{
		zlog_error(MLOG, "Analyze_Volume: not found volumeinfo");
		goto EXIT;
	}

	volume   = get_children_node(volumeinfo, BAD_CAST "Volume");
	if(volumeinfo == NULL)
	{
		zlog_error(MLOG, "Analyze_Volume: not found Volume");
		goto EXIT;
	}
	
	pvolume  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, volume->xmlChildrenNode, 1);	
	if(pvolume == NULL)
	{
		zlog_error(MLOG, "Analyze_Volume: not found usrname");
		goto EXIT;
	}

	UInt32 uvolume = 0;
	uvolume = atoi(pvolume);
	if(uvolume > 100 || uvolume <0)
	{
		zlog_error(MLOG, "Analyze_Volume: error volume [%d]",uvolume);
		goto EXIT;
	}
	parm->volume = (UInt8)uvolume *8/100;
	
	return_code = 1;
	
EXIT:
	if(NULL != pvolume)
	{
		xmlFree(pvolume);
	}
	
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}

static Int32 Analyze_RecordRep(Int8 *recv_buf, void *pparm)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	xmlNodePtr msghead = NULL;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr retstate = NULL;
	char *pret = NULL;

	SerialParam *parm = (SerialParam *)(pparm);
	if(parm == NULL)
	{
		zlog_error(MLOG, "Analyze_State: parm is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_RecordRep: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_RecordRep: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		zlog_error(MLOG, "Analyze_RecordRep: is_resp_msg fail");
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		zlog_error(MLOG, "Analyze_RecordRep: msghead fail");
		goto EXIT;
	}

	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	if(msgbody == NULL)
	{
		zlog_error(MLOG, "Analyze_RecordRep: msgbody fail");
		goto EXIT;
	}
	
	retstate   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retstate == NULL)
	{
		zlog_error(MLOG, "Analyze_RecordRep: not found ReturnCode");
		goto EXIT;
	}
	
	pret  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retstate->xmlChildrenNode, 1);	
	if(pret == NULL)
	{
		zlog_error(MLOG, "Analyze_RecordRep: not found usrname");
		goto EXIT;
	}
	

	Int32 iret = -1;
	iret = atoi(pret);
	if(iret != 1)
	{
		RtRecordState = 0;
		zlog_error(MLOG, "Analyze_RecordRep: error record ret[%d]",iret);
		goto EXIT;
	}

	if(RtRecordState <= 0 || RtRecordState > 3)
	{
		zlog_error(MLOG, "Analyze_RecordRep: error RtRecordState ret[%d]",RtRecordState);
		goto EXIT;
	}
	
	parm->close = 2;
	parm->start = 2;
	parm->stop  = 2;
	if(RtRecordState == 1)
	{
		parm->start = 1;
	}
	else if(RtRecordState == 2)
	{
		parm->stop = 1;
	}
	else if(RtRecordState == 3)
	{
		parm->close= 1;
	}
	
	printf("close[%d] start[%d] stop[%d]\n",parm->close, parm->start, parm->stop);
	return_code = 1;
	
EXIT:
	if(NULL != pret)
	{
		xmlFree(pret);
	}
	
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}

static Int32 Analyze_Warnning(Int8 *recv_buf, void *pparm)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr warnning = NULL;
	xmlNodePtr id = NULL;
	char *pwarn = NULL;

	SerialParam *parm = (SerialParam *)(pparm);
	if(parm == NULL)
	{
		zlog_error(MLOG, "Analyze_State: parm is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_Warnning: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_Warnning: init_dom_tree fail");
		goto EXIT;
	}

	if(is_req_msg(parse_xml_user->proot) != 1) 
	{		
		zlog_error(MLOG, "Analyze_Warnning: is_req_msg fail");
		goto EXIT;
	}	

	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	if(msgbody == NULL)
	{
		zlog_error(MLOG, "Analyze_Warnning: msghead fail");
		goto EXIT;
	}
	
	warnning   = get_children_node(msgbody, BAD_CAST "warn");
	if(warnning == NULL)
	{
		zlog_error(MLOG, "Analyze_Warnning: not found usrname");
		goto EXIT;
	}

	id   = get_children_node(warnning, BAD_CAST "id");
	if(id == NULL)
	{
		zlog_error(MLOG, "Analyze_Volume: not found usrname");
		goto EXIT;
	}
	
	pwarn  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, id->xmlChildrenNode, 1);	
	if(pwarn == NULL)
	{
		zlog_error(MLOG, "Analyze_Warnning: not found usrname");
		goto EXIT;
	}
	Int32 iwarn = 0x2;
	iwarn = atoi(pwarn);
	if(iwarn < 0)
	{
		zlog_error(MLOG, "Analyze_Warnning: error iwarn [%d]",iwarn);
		goto EXIT;
	}

	/* 警告显示 */
	if(iwarn >= 1)
	{
		iwarn = 0x1;
	}
	else
	{
		iwarn = 0x2;
	}
	parm->warning = (UInt8)iwarn;
	return_code = 1;
	
EXIT:
	if(NULL != pwarn)
	{
		xmlFree(pwarn);
	}
	
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}

static Int32 Analyze_reboot(Int8 *recv_buf, void *pparm)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode   = NULL;
	char *pvolume = NULL;
	char *pretcode = NULL;

	SerialParam *parm = (SerialParam *)(pparm);
	if(parm == NULL)
	{
		zlog_error(MLOG, "Analyze_reboot: parm is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_reboot: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_reboot: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		zlog_error(MLOG, "Analyze_reboot: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		zlog_error(MLOG, "Analyze_reboot: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		zlog_error(MLOG, "Analyze_reboot: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		zlog_error(MLOG, "Analyze_reboot: not found pretcode");
		goto EXIT;
	}

	UInt32 irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		zlog_error(MLOG, "Analyze_reboot: ReturnCode [%d]",irecode);
		goto EXIT;
	}

	parm->reboot = irecode;
	
	return_code = 1;
	
EXIT:
	if(NULL != pvolume)
	{
		xmlFree(pvolume);
	}
	
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}

static Int32 Analyze_replay(Int8 *recv_buf, void *pparm)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;

	xmlNodePtr msghead    = NULL;
	xmlNodePtr retcode   = NULL;
	xmlNodePtr msgbody = NULL;
	char *pvolume = NULL;
	char *pretcode = NULL;

	SerialParam *parm = (SerialParam *)(pparm);
	if(parm == NULL)
	{
		zlog_error(MLOG, "Analyze_replay: parm is NULL");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_replay: malloc parse_xml_t fail");
		return -1;
	}
	
	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(MLOG, "Analyze_replay: init_dom_tree fail");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1) 
	{		
		zlog_error(MLOG, "Analyze_replay: is_resp_msg fail[%s]",parse_xml_user->proot->name);
		goto EXIT;
	}	

	msghead   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgHead");
	if(msghead == NULL)
	{
		zlog_error(MLOG, "Analyze_replay: msghead fail");
		goto EXIT;
	}

	retcode   = get_children_node(msghead, BAD_CAST "ReturnCode");
	if(retcode == NULL)
	{
		zlog_error(MLOG, "Analyze_replay: retcode fail");
		goto EXIT;
	}

	pretcode  = (char *)xmlNodeListGetString(parse_xml_user->pdoc, retcode->xmlChildrenNode, 1);	
	if(pretcode == NULL)
	{
		zlog_error(MLOG, "Analyze_replay: not found pretcode");
		goto EXIT;
	}

	UInt32 irecode = -1;
	irecode = atoi(pretcode);
	if(irecode != 1)
	{
		zlog_error(MLOG, "Analyze_replay: ReturnCode [%d]",irecode);
		goto EXIT;
	}

	parm->replay = irecode;


	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	if(msghead == NULL)
	{
		zlog_error(MLOG, "Analyze_replay: MsgBody fail");
		goto EXIT;
	}

	retcode   = get_children_node(msgbody, BAD_CAST "Replay");
	if(retcode == NULL)
	{
		zlog_error(MLOG, "Analyze_replay: Replay fail");
		goto EXIT;
	}

	retcode   = get_children_node(retcode, BAD_CAST "PlayFile");
	if(retcode == NULL)
	{
		zlog_error(MLOG, "Analyze_replay: PlayFile fail");
		goto EXIT;
	}

	r_bzero(parm->replayfile, sizeof(parm->replayfile));
	return_code = get_current_node_value(parm->replayfile, sizeof(parm->replayfile)-1, parse_xml_user->pdoc, retcode);
	if(return_code == -1)
	{
		zlog_error(MLOG, "Analyze_replay: get PlayFile fail");
		return_code = -1;
		goto EXIT;
	}
		
	return_code = 1;
	
EXIT:
	if(NULL != pvolume)
	{
		xmlFree(pvolume);
	}
	
	if(parse_xml_user->pdoc != NULL) 
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);
	return return_code;
}


#if 0
/* 解析串口信令 */
static int serialParseCenterCtrlCmd(int  sockfd, char *cmdline, int size)
{

	char 	*token = NULL;
	char 	*p = NULL;
//	char *cmd[] = {"ready","unicast","multicast","vod","resolution","output","filelist","status","screen","shift"};
	
	if(cmdline[0] == 0)
	{
		printf("serialParseCenterCtrlCmd break.....\n");
	
	}
		
	if((cmdline[0] == '\n'))
	{
		
	}

	p = cmdline;
	while((token = strsep(&p,"cd")) != NULL)
	{
		printf("token = %s|%s\n",token,p);

	}

	

	return 0;
}
#endif

void SetSpeed(int fd, int speed)
{
        int i; 
        int status; 
        struct termios Opt;
        int speed_arr[] = { B115200,B38400, B19200, B9600, B4800, B2400, B1200, B300,
                      B38400, B19200, B9600, B4800, B2400, B1200, B300, };
        int name_arr[] = {115200,38400,  19200,  9600,  4800,  2400,  1200,  300, 38400,  
                                   19200,  9600, 4800, 2400, 1200,  300, };

        tcgetattr(fd, &Opt); 
        for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) 
        { 
                if  (speed == name_arr[i]) 
                {
                        tcflush(fd, TCIOFLUSH);     
                        cfsetispeed(&Opt, speed_arr[i]);  
                        cfsetospeed(&Opt, speed_arr[i]);   
                        status = tcsetattr(fd, TCSANOW, &Opt);  
                        if  (status != 0) 
                        {
                                printf("tcsetattr fd, errmsg = %s\n", strerror(errno));  
                                return;     
                        }
                        tcflush(fd,TCIOFLUSH);   
                }  
        }
}

static int SetParity(int fd,int databits,int stopbits,int parity)
{ 
        struct termios options; if(tcgetattr(fd,&options)  !=  0) { 
                printf("SetupSerial 1");     
                return(-1);  
       }
        options.c_cflag &= ~CSIZE; 
        options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
        options.c_oflag  &= ~OPOST;                     /*Output*/
        options.c_iflag   &= ~IXON;                     //0x11
        options.c_iflag   &= ~ICRNL;                    //0x0d
//      options.c_cflag|=CLOCAL;
//      options.c_cflag|=CREAD;
//      options.c_cflag&=~CRTSCTS;

        switch (databits) 
        {   
                case 7:           
                        options.c_cflag |= CS7; 
                        break;
                case 8:     
                        options.c_cflag |= CS8;
                        break;   
                default:    
                        printf("Unsupported data size\n");                      return (-1);  
        }
        switch (parity) 
        {   
                case 'n':
                case 'N':    
                        options.c_cflag &= ~PARENB;     /* Clear parity enable */
                        options.c_iflag &= ~INPCK;              /* Enable parity checking */ 
                        break;  
                case 'o':   
                case 'O':     
                        options.c_cflag |= (PARODD | PARENB); 
                        options.c_iflag |= INPCK;       /* Disnable parity checking */ 
                        break;  
                case 'e':  
                case 'E':   
                        options.c_cflag |= PARENB;      /* Enable parity */    
                        options.c_cflag &= ~PARODD;  
                        options.c_iflag |= INPCK;       /* Disnable parity checking */
                        break;
                case 'S': 
                case 's':                                                       /*as no parity*/   
                        options.c_cflag &= ~PARENB;
                        options.c_cflag &= ~CSTOPB;
                        break;  
                default:   
                        printf("Unsupported parity\n");    
         return (-1);  
        }  

        switch (stopbits)
        {   
                case 1:    
                        options.c_cflag &= ~CSTOPB;  
                        break;  
                case 2:    
                        options.c_cflag |= CSTOPB;  
                        break;
                default:    
                        printf("Unsupported stop bits\n");  
                        return (-1); 
        } 
        /* Set input parity option */ 
        if (parity != 'n')   
                options.c_iflag |= INPCK; 
        tcflush(fd,TCIFLUSH);
        options.c_cc[VTIME] = 0;    
        options.c_cc[VMIN] = 1;                                 /* define the minimum bytes data to be readed*/
        if (tcsetattr(fd,TCSANOW,&options) != 0)   
        { 
                printf("SetupSerial 3");   
                return (-1);  
        } 
        return (0);  
}

static int OpenPort(int port_num)
{
        char port[4][20]={{"/dev/ttyO0"},{"/dev/ttyO1"},{"/dev/ttyO2"},{"/dev/ttyO3"}};
        int fd;

        if(port_num > 3 || port_num <0) {
                printf("port num error:%d\n",port_num);
                return -1;
        }

  
        if((fd = r_open((const int8_t *)port[port_num], O_RDWR| O_NOCTTY )) < 0) //O_NDELAY
		{
                printf("ERROR: failed to open %s, errno=%d\n",port[port_num],errno);
                return -1;
        }else {
                printf("Open %s success, fd = %d \n",port[port_num], fd);
        }
		   fcntl(fd, F_SETFL, FNDELAY);
        return fd;
}

static void ClosePort(int fd)
{
        if(fd !=-1)
               r_close(fd);
}

static int InitPort(int port_num,int baudrate,
                                int databits,int stopbits,int parity)
{
        int fd;

        fd=OpenPort(port_num);
        if(fd == -1) {
                return -1;
        }
        SetSpeed(fd,baudrate);
        if(SetParity(fd,databits,stopbits,parity)==-1) {
                printf("Set Parity Error!\n");
                ClosePort(fd);
                return -1;
        }
        return fd;
}

static unsigned long writen(int fd, const void *vptr, size_t n)
{
        unsigned long nleft;
        unsigned long nwritten;
        const char      *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nwritten = r_write(fd, ptr, nleft)) <= 0) {
                        if (nwritten < 0 && errno == EINTR)
                                nwritten = 0;           /* and call write() again */
                        else
                                return(-1);                     /* error */
                }

                nleft -= nwritten;
                ptr   += nwritten;
        }
        return(n);
}
/* end writen */

/*send data to tty com*/
static int SendDataToCom(int fd,unsigned char *data,int len)
{
        unsigned long real_len = 0 ;
        //int i;

        if((real_len = writen(fd,data,len)) != len)
        {
                printf("SendDataToCom() write tty error\n");
                return -1;
        }
        r_usleep(20000);
        return (real_len);
}

static Int32 SerialWrite(Int8 *cmd, UInt32 len)
{
	Int32 sendlen = -1;
	SerialLock();
	Int32 SerialFd = GetSerialFd();

	usleep(1000*30);
	if(-1 != SerialFd)
	{
		sendlen = SendDataToCom(SerialFd,(unsigned char *)cmd,len);
		/* 一次性读16个字符 */
		//sendlen = write(SerialFd, cmd, len);
	}
	else
	{
		printf("SerialWrite: Get SericalWrite fail\n");
	}
	SerialunLock();

	SerialLock1();
	SerialFd = GetSerialFd1();
	sendlen = -1;
	if(-1 != SerialFd)
	{
		sendlen = SendDataToCom(SerialFd,(unsigned char *)cmd,len);
		/* 一次性读16个字符 */
		//sendlen = write(SerialFd, cmd, len);
	}
	else
	{
		printf("SerialWrite: Get SericalWrite fail\n");
	}
	SerialunLock1();

	

	return sendlen;
}

/*==============================================================================
    函数: <SerialOperate>
    功能: <xh_Func:>串口操作
    参数: 
    Created By 徐崇 2012.11.05 17:39:51 For Ftp
==============================================================================*/
static Int32 SerialOperate(UInt32 num, void *pprm)
{
	SERIAL_MSGCODE commnd;
	Int32 cmdlen = 0;
	Int8 *pcmd 	= NULL;
	if(num >= COMND_MAX)
	{
		zlog_error(MLOG, "SerialOperate: COMND_MAX is invalid");
		return -1;
	}

	SerialParam *prm = (SerialParam *)(pprm);
	if(prm == NULL)
	{
		zlog_error(MLOG, "SerialOperate: parm is NULL");
		return -1;
	}
	
	commnd = globalSerialCmd[num].commnd;
	cmdlen = globalSerialCmd[num].serialcmdlen;

	if(commnd == RECORDSTATE)
	{
		/* 反馈源状态 */
		Int8 cmd[]={0x3c,0x3c,0xc2,0x91,0x2,0x2,0x2,0x3e,0x3e};
		pcmd = cmd;
		cmd[4] = prm->videostate[0];
		cmd[5] = prm->videostate[1];
		cmd[6] = prm->videostate[2];

		
		if(0 >= SerialWrite(pcmd, cmdlen))
		{
			return -1;
		}
		zlog_debug(MLOG, "SerialWrite: src state [%x] [%x] [%x]\n",cmd[4],cmd[5],cmd[6]);

		/* 反馈录制状态 */
		if (pserial_handle->PlayState <= 0)		
		{
			cmd[3] = 0x92;
			cmd[4] = prm->start;
			cmd[5] = prm->stop;
			cmd[6] = prm->close;
			if(0 >= SerialWrite(pcmd, cmdlen))
			{
				return -1;
			}
			zlog_debug(MLOG, "SerialWrite: record state [%x] [%x] [%x]\n",cmd[4],cmd[5],cmd[6]);
		}
		
		Int8 cmd2[]={0x3c,0x3c,0xc2,0x90,0x1,0x3e,0x3e};
		pcmd   = cmd2;
		pcmd[4] = prm->warning;
		if(0 >= SerialWrite(pcmd, 7))
		{
			return -1;
		}
		zlog_debug(MLOG, "SerialWrite: warning state [%x]\n",cmd[4]);
		
	}
	else if(commnd == WARNING)
	{
		Int8 cmd[]={0x3c,0x3c,0xc2,0x90,0x1,0x3e,0x3e};
		pcmd   = cmd;
		pcmd[4] = prm->warning;
		if(0 >= SerialWrite(pcmd, cmdlen))
		{
			return -1;
		}
		zlog_debug(MLOG, "SerialWrite: warning state [%x]\n",cmd[4]);
	}
	else if(commnd == VOLUME)
	{
		Int8 cmd[]={0x3c,0x3c,0xc2,0x93,0x0,0x3e,0x3e};
		pcmd = cmd;
		pcmd[4] = prm->volume;
		if(0 >= SerialWrite(pcmd, cmdlen))
		{
			return -1;
		}
		zlog_debug(MLOG, "SerialWrite: VOLUME state [%x]\n",cmd[4]);
	}
	else if(commnd == RECORD)
	{
		/* 反录制状态 */
		Int8 cmd[]={0x3c,0x3c,0xc2,0x92,0x2,0x2,0x2,0x3e,0x3e};
		pcmd = cmd;
		
		/* 反馈录制状态 */
		cmd[4] = prm->start;
		cmd[5] = prm->stop;
		cmd[6] = prm->close;

		printf("-------------SerialWrite: RECORD state [%x] [%x] [%x] len[%d]\n",cmd[4],cmd[5],cmd[6],cmdlen);

		if(0 >= SerialWrite(pcmd, cmdlen))
		{
			return -1;
		}
		zlog_debug(MLOG, "SerialWrite: RECORD state [%x] [%x] [%x] len[%d]\n",cmd[4],cmd[5],cmd[6],cmdlen);

		#if 1
		if(cmd[6] == 0x1)
		{
			sleep(1);
			cmd[6] = 0x2;
			SerialWrite(pcmd, cmdlen);
			pserial_handle->LedState = IPMENU;
			pserial_handle->LedFLUSH = 1;
		}
		#endif
	}
	else if (commnd == REBOOT)
	{
		/*重启操作反馈*/
		int spi_fd = fd;
		if (prm->reboot == 1) {
			pserial_handle->LedState = REBOOTMENU;
			ClearScreen(spi_fd);
			ShowString(spi_fd, "Rebooting...", 0, 20);
			zlog_debug(MLOG, "Rebooting...[%x]\n", prm->reboot);
		} else {
			ShowString(spi_fd, "Reboot Error", 0, 20);
			zlog_debug(MLOG, "Reboot...[%x]\n", prm->reboot);
		}
	}
	else if (commnd == PLAY)
	{	

		#if 0
		/*回放*/
		int spi_fd = fd;
		
		ClearScreen(spi_fd);

		if (prm->replay == 1) {
			pserial_handle->LedState = REPLAYINGMENU;
			ShowString(spi_fd, "Playing Back", 0, 20);
			ShowString(spi_fd, prm->replayfile, 0, 35);
			zlog_debug(MLOG, "Playing Back...[%x]\n", prm->replay);
		} else {
			ShowString(spi_fd, "Playing Back Error", 0, 20);
			ShowString(spi_fd, prm->replayfile, 0, 35);
			zlog_debug(MLOG, "Playing Back...[%x]\n", prm->replay);
		}
		#endif
	}
	else
	{
		return -1;
	}

	return 1;
}


static Int32 NetOperate(UInt32 len,void *xml)
{
	UInt8 *pxml = (UInt8 *)xml;
	
	if(pxml == NULL|| len <= 0)
	{
		return -1;
	}
	
	TcpSockLock();
	Int32 socket = GetTcpSocket();
	if(socket > 0)
	{
		MsgHeader *pmsg = (MsgHeader *)pxml;
		int32_t slen = 0;
		slen = len + MSGLEN;
		pmsg->sLen = r_htons(slen);
		pmsg->sMsgType = 0;
		pmsg->sVer = r_htons(2012);
		Int32 nameLength = tcp_send_longdata(socket, (int8_t *)pxml, slen);
 
	    if (nameLength < 0)
	    {
	       	zlog_error(MLOG, "tcp_send_longdata: nameLength is error!!");
	 
	    }
	}
	TcpSockunLock();

	return 1;
}

static SerialComnd *GetSerialOperate(UInt32 num)
{
	if(num >= COMND_MAX)
	{
		return NULL;
	}
	return &globalSerialCmd[num];
}


/*==============================================================================
    函数: <NGetGlobalCmdIndex>
    功能: <xh_Func:> 获取网络信令操作ID
    参数: 
    Created By 徐崇 2012.11.05 17:02:39 For Ftp
==============================================================================*/
static UInt32 NGetGlobalCmdIndex(UInt32 cmd, Int32 direction)
{
	UInt32 i = 0;
	for(i = 0; i < COMND_MAX ;i++)
	{
		if((globalSerialCmd[i].commnd == cmd) &&(globalSerialCmd[i].direction == direction))
		{
			return i;
		}
	}
	
	return -1;
}


/*==============================================================================
    函数: <GetGlobalCmdIndex>
    功能: <xh_Func:>  获取串口信令操作ID
    参数: 
    Created By 徐崇 2012.11.05 17:01:55 For Ftp
==============================================================================*/
static UInt32 SGetGlobalCmdIndex(UInt8 *cmd, Int32 direction)
{
	UInt32 i = 0;
	for(i = 0; i < COMND_MAX ;i++)
	{
		if((globalSerialCmd[i].serialcmd == (*cmd)) && (globalSerialCmd[i].direction == direction))
		{
			return i;
		}
	}
	
	return -1;
}

static Int32 CheckCmdVail(UInt8 *pcmd)
{
	UInt32 i = -1;
	if(pcmd == NULL)
	{
		return -1;
	}

	if((pcmd[0] == 0x3c) && (pcmd[1] == 0x3c) && (pcmd[2] == 0xc2))
	{
		UInt32 len = 0;

		
		i = SGetGlobalCmdIndex(&pcmd[3], 1);
		if(i == -1)
		{
			return -1;
		}
		
	    len = globalSerialCmd[i].serialcmdlen;
	//		printf("---->%x %d %d\n",pcmd[3],i,len);
		if((pcmd[len-1]) != 0x3e || (pcmd[len-2] != 0x3e))
		{
			return -1;
		}
	}
	return i;
}

static Int32 GetCmdLen(UInt8 *pcmd, Int32 direction)
{
	UInt32 i = -1;
	if(pcmd == NULL)
	{
		return -1;
	}

	if((pcmd[0] == 0x3c) && (pcmd[1] == 0x3c) && (pcmd[2] == 0xc2))
	{
		UInt32 len = 0;

		
		i = SGetGlobalCmdIndex(&pcmd[3], direction);
		if(i == -1)
		{
			return -1;
		}
		
	    len = globalSerialCmd[i].serialcmdlen;
		if(len > 0)
		{
			return len;
		}
		
	}
	return i;
}


//主要接受处理控制面板消息
static void *SerialCenterctrlThread(void *args)
{
	UInt8 *pcmd = NULL;
	UInt8 *pxml = NULL;
	fd_set rfds;
	struct timeval tv_timeout;
	
	Int32 offlen = 0;
	Int32 cmdlen = 0;
	
	Int32 	seret = 0;
	Int32 	serialFd;
	
	if(NULL == args)
	{
		zlog_error(MLOG, "SerialClientTask: args is invalid!!");
		return NULL;
	}

	SerialHandle *phandle = (SerialHandle *)args;

	pcmd = (UInt8*)r_malloc(SERIAL_CMD_LEN);
	if(pcmd == NULL)
	{
		zlog_error(MLOG, "SERIAL_CMD_LEN r_malloc failed \n");
		return NULL;
	}

	pxml = (UInt8*)r_malloc(MAX_XMNL_LEN);
	if(pxml == NULL)
	{
		zlog_error(MLOG, "MAX_XMNL_LEN r_malloc failed \n");
		return NULL;
	}

	
AGAIN:
	while(1)
	{
		//serialFd= initSerialPort("/dev/ttyS1",9600,8,1,'N');
		serialFd = InitPort(0, 9600,8,1,'N');
		if(serialFd < 0)
		{
			printf("init serial port failed\n");
			zlog_error(MLOG, "init serial port failed \n");
		}
		else
		{
			zlog_debug(MLOG, "init serial port success [%d] \n",serialFd);
			break;
		}
		r_sleep(10);
	}

	SetSerialFd(serialFd);
	Int8 sendcmd[]={0x3c,0x3c,0xc2,0x92,0x2,0x2,0x2,0x3e,0x3e};
	SerialWrite(sendcmd,9);
	usleep(100*1000);
	printf("\n\n----------------SerialCenterctrlThread {0x3c,0x3c,0xc2,0x92,0x2,0x2,0x2,0x3e,0x3e}------------------\n\n\n");
	
	while(1)
	{
		UInt32 num = 0;
		if(offlen == 0)
		{
			r_memset(pcmd, 0x0, sizeof(Int8)*SERIAL_CMD_LEN);
		}
		
		tv_timeout.tv_sec = 5;
		tv_timeout.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(serialFd,&rfds);
		seret = r_select(serialFd+1,&rfds,NULL,NULL,&tv_timeout);
		if(seret > 0)
			// FD已准备好
		{
			if(FD_ISSET(serialFd, &rfds))
			{		
				zlog_debug(MLOG,"select recv serical recv date.\n");
				int len  = -1;			
				SerialLock();

				//printf("offlen = %d\n",offlen);
				/* 重新接收 */
				if(offlen < 4)
				{
					/* 先读3个字节 */
					len = r_read(serialFd, pcmd + offlen, 4 - offlen);
					offlen = offlen + len;

					if(offlen < 4)
					{
						offlen = len;
						cmdlen = 0;
						SerialunLock();
						continue;
					}
					
					cmdlen = GetCmdLen(pcmd,1);	
				//	printf("%d %x %d\n",cmdlen,pcmd[3],offlen);
					if(cmdlen <= 0)
					{
						offlen = 0;
						cmdlen = 0;
						SerialunLock();
						continue;
					}
				}

					
				len = r_read(serialFd, pcmd + offlen, cmdlen - offlen);
					//printf("--->2 %d %d %d\n",len,offlen, cmdlen);
				offlen = len + offlen;
				if(offlen < cmdlen)
				{
					SerialunLock();
					continue;
				}
			
				offlen = 0;
				cmdlen = 0;
				SerialunLock();
				
				//for(i = 0; i<16; i++)
				{
					//zlog_debug(MLOG,"[%x] %d",pcmd[4],len);
				}
				//printf("\n");

				/* 检查信令是否有效 */
				num  = CheckCmdVail(pcmd);
				if(num == -1)
				{
					zlog_error(MLOG, "CheckCmdVail: cmd[%d] is invalid!!",num);
					continue;
				}

				if((pserial_handle->LedState == DOWNINGMENU))
						continue;
				
				//检测是否等待超时
				if(pcmd[3] == 0x80)
				{
					if(1 == CheckTimeOut(phandle,  RECORD))
					{
						continue;
					}
				}

				{
					zlog_debug(MLOG,"[%x] %d",pcmd[4],len);
				}
				
				/* 获取操作句柄 */
				SerialComnd *operate;
				operate = GetSerialOperate(num);
				if(operate == NULL)
				{
					zlog_error(MLOG, "GetSerialOperate: cmd[%d] operate is invalid!!",num);
					continue;
				}
			
				r_memset(pxml, 0x0, MAX_XMNL_LEN);

				if(operate->PackageAnalysis == NULL)
				{
					continue;
				}
				
				/* 封装信令 */
				len = operate->PackageAnalysis((Int8 *)(pxml + MSGLEN), pcmd);
				if(len == -1)
				{
					zlog_error(MLOG, "PackageMsg: cmd[%s] PackageMsg is fail!!",operate->cmdname);
					continue;
				}

				if(operate->Operate == NULL)
				{
					continue;
				}
				
				/* 发送信令 */
				if(1 != operate->Operate(len, pxml))
				{
					zlog_error(MLOG, "NetOperate: net send fail!!");
				}
				else
				{
					zlog_debug(MLOG, "%s", pxml + MSGLEN);
					 //超时机制暂时只针对录制
					if(pcmd[3] == 0x80)	
					{
						RtRecordState = pcmd[5];	
					}
					
					AddTimeOut(phandle,RECORD,pcmd[5]);
				}
				
			}

			
		}
		else if(seret == 0)
		// 超时
		{
			//printf("time out !!!!\n");
			offlen = 0;
			cmdlen = 0;
			r_usleep(1000);
			continue;
		}
		else if(seret < 0)
					// 异常
		{	
			SetSerialFd(-1);
			r_close(serialFd);
			r_usleep(1000);		
			goto AGAIN;
		}
	}
	
	SetSerialFd(-1);
	if(serialFd >= 0)
		ClosePort(serialFd);
	if(pcmd != NULL)
	{
		r_free(pcmd);
	}
	
	if(pxml != NULL)
	{
		r_free(pxml);
	}
	
}



//主要接受处理前面板消息

static void *SerialCenterctrlThreadEx(void *args)
{
	UInt8 *pcmd = NULL;
	UInt8 *pxml = NULL;
	fd_set rfds;
	struct timeval tv_timeout;
	
	Int32 offlen = 0;
	Int32 cmdlen = 0;
	
	Int32 	seret = 0;
	Int32 	serialFd;
	
	if(NULL == args)
	{
		zlog_error(MLOG, "SerialClientTask: args is invalid!!");
		return NULL;
	}

	SerialHandle *phandle = (SerialHandle *)args;

	pcmd = (UInt8 *)r_malloc(SERIAL_CMD_LEN);
	if(pcmd == NULL)
	{
		zlog_error(MLOG, "SERIAL_CMD_LEN r_malloc failed \n");
		return NULL;
	}

	pxml = (UInt8 *)r_malloc(MAX_XMNL_LEN);
	if(pxml == NULL)
	{
		zlog_error(MLOG, "MAX_XMNL_LEN r_malloc failed \n");
		return NULL;
	}

	
AGAIN:
	while(1)
	{
		//serialFd= initSerialPort("/dev/ttyS1",9600,8,1,'N');
		serialFd = InitPort(1, 9600,8,1,'N');
		if(serialFd < 0)
		{
			printf("init serial port failed\n");
			zlog_error(MLOG, "init serial port failed \n");
		}
		else
		{
			zlog_debug(MLOG, "init serial port success [%d] \n",serialFd);
			break;
		}
		r_sleep(10);
	}

	SetSerialFd1(serialFd);

	char sendcmd[]={0x3c,0x3c,0xc2,0x94,0x2,0x3e,0x3e};
	SerialWrite(sendcmd,7);
	sendcmd[3] = 0x95;
	usleep(100*1000);
	SerialWrite(sendcmd,7);
	usleep(200*1000);
	
	char statcmd[]={0x3c,0x3c,0xc2,0x92,0x2,0x2,0x2,0x3e,0x3e};
	SerialWrite(statcmd,9);
printf("\n\n----------------SerialCenterctrlThreadEx {0x3c,0x3c,0xc2,0x92,0x2,0x2,0x2,0x3e,0x3e}------------------\n\n\n");
	while(1)
	{
		UInt32 num = 0;
		if(offlen == 0)
		{
			r_memset(pcmd, 0x0, sizeof(Int8)*SERIAL_CMD_LEN);
		}
		
		tv_timeout.tv_sec = 5;
		tv_timeout.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(serialFd,&rfds);
		seret = r_select(serialFd+1,&rfds,NULL,NULL,&tv_timeout);
		if(seret > 0)
			// FD已准备好
		{
			if(FD_ISSET(serialFd, &rfds))
			{		
				zlog_debug(MLOG,"select recv serical recv date.\n");
				int len  = -1;			
				SerialLock1();

				//printf("offlen = %d\n",offlen);
				/* 重新接收 */
				if(offlen < 4)
				{
					/* 先读3个字节 */
					len = r_read(serialFd, pcmd + offlen, 4 - offlen);
					offlen = offlen + len;

					if(offlen < 4)
					{
						offlen = len;
						cmdlen = 0;
						SerialunLock1();
						continue;
					}
					
					cmdlen = GetCmdLen(pcmd,1);	
				//	printf("%d %x %d\n",cmdlen,pcmd[3],offlen);
					if(cmdlen <= 0)
					{
						offlen = 0;
						cmdlen = 0;
						SerialunLock1();
						continue;
					}
				}

					
				len = r_read(serialFd, pcmd + offlen, cmdlen - offlen);
					//printf("--->2 %d %d %d\n",len,offlen, cmdlen);
				offlen = len + offlen;
				if(offlen < cmdlen)
				{
					SerialunLock1();
					continue;
				}
			
				offlen = 0;
				cmdlen = 0;
				SerialunLock1();
				
				//for(i = 0; i<16; i++)
				{
					//zlog_debug(MLOG,"[%x] %d",pcmd[4],len);
				}
				//printf("\n");

				/* 检查信令是否有效 */
				num  = CheckCmdVail(pcmd);
				if(num == -1)
				{
					zlog_error(MLOG, "CheckCmdVail: cmd[%d] is invalid!!",num);
					continue;
				}

				if((pserial_handle->LedState == DOWNINGMENU))
						continue;

				if((pserial_handle->LedState ==SELECTUSBMENU) &&(pcmd[3] != 0x84))
					continue;
		
				//检测是否等待超时
				if(pcmd[3] == 0x80)
				{
					if(1 == CheckTimeOut(phandle,  RECORD))
					{
						continue;
					}
				}

				{
					zlog_debug(MLOG,"[%x] %d",pcmd[4],len);
				}
				
				/* 获取操作句柄 */
				SerialComnd *operate;
				operate = GetSerialOperate(num);
				if(operate == NULL)
				{
					zlog_error(MLOG, "GetSerialOperate: cmd[%d] operate is invalid!!",num);
					continue;
				}
			
				r_memset(pxml, 0x0, MAX_XMNL_LEN);

				if(operate->PackageAnalysis == NULL)
				{
					continue;
				}
				
				/* 封装信令 */
				len = operate->PackageAnalysis((Int8 *)(pxml + MSGLEN), pcmd);
				if(len == -1)
				{
					zlog_error(MLOG, "PackageMsg: cmd[%s] PackageMsg is fail!!",operate->cmdname);
					continue;
				}

				if(operate->Operate == NULL)
				{
					continue;
				}
				
				/* 发送信令 */
				if(1 != operate->Operate(len, pxml))
				{
					zlog_error(MLOG, "NetOperate: net send fail!!");
				}
				else
				{
					zlog_debug(MLOG, "%s", pxml + MSGLEN);
					 //超时机制暂时只针对录制
					if(pcmd[3] == 0x80)	
					{
						RtRecordState = pcmd[5];	
					}
					
					AddTimeOut(phandle,RECORD,pcmd[5]);
				}
				
			}

			
		}
		else if(seret == 0)
		// 超时
		{
			//printf("time out !!!!\n");
			offlen = 0;
			cmdlen = 0;
			r_usleep(1000);
			continue;
		}
		else if(seret < 0)
					// 异常
		{	
			SetSerialFd1(-1);
			r_close(serialFd);
			r_usleep(1000);		
			goto AGAIN;
		}
	}
	
	SetSerialFd1(-1);
	if(serialFd >= 0)
		ClosePort(serialFd);
	if(pcmd != NULL)
	{
		r_free(pcmd);
	}
	
	if(pxml != NULL)
	{
		r_free(pxml);
	}
	
}



int32_t ConnectSerialServer(const int8_t* paddr,const int32_t port)
{
	Int32 ret = 0;
	struct sockaddr_in client_addr;
    r_bzero(&client_addr, sizeof(client_addr)); 
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = r_htons(INADDR_ANY); 
    client_addr.sin_port = r_htons(0);
	
	int client_socket = r_socket(AF_INET, SOCK_STREAM, 0);
	if(client_socket < 0)
	{
		zlog_error(MLOG,"ConnectSerialServer failed, err msg: %s \n", strerror(errno));
		return -1;	
	}
 
    if (r_bind(client_socket, (struct sockaddr*) &client_addr,
            sizeof(client_addr)))
    {
		zlog_error(MLOG, "Client Bind Port Failed!\n");
        return -1;
    }
	
	struct sockaddr_in		serv_addr;
	r_bzero(&serv_addr, sizeof(serv_addr));
	
	serv_addr.sin_family	= AF_INET;
	serv_addr.sin_port		= htons(port);
	r_inet_aton(paddr, (struct in_addr *)&serv_addr.sin_addr);

	set_send_timeout(client_socket, 3);
	set_recv_timeout(client_socket, 3);

	
	ret = r_connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	
	if(ret < 0)
	{
		zlog_error(MLOG, "connect failed, err msg: %s \n", strerror(errno));
		r_close(client_socket);
		return -1;
	}

	#if 0
	fileflags = fcntl(client_socket, F_GETFL, 0); 
	if(fileflags < 0) 
	{
		zlog_error(MLOG, "fcntl F_GETFL failed, err msg: %s\n", strerror(errno));
		return -1;
	}
	
	ret = fcntl(client_socket, F_SETFL, fileflags | O_NONBLOCK);
	if(ret < 0)
	{
		zlog_error(MLOG, "fcntl F_SETFL failed, err msg: %s\n", strerror(errno));
		return -1;
	}
	#endif
	return client_socket;
}

void *SerialComTask(void *args)
{
	int32_t socket = -1;
	fd_set readfd;
	struct timeval timeout;
	int8_t *pbuf = NULL;
	MsgHeader *pmsg;
	
	if(NULL == args)
	{
		zlog_error(MLOG, "SerialComTask failed, args is NULL!");
		return NULL;
	}
	
	SerialHandle *phandle = (SerialHandle *)args;
	int8_t pcmd[2048]  = {0};

	while(1)
	{
		SetTcpSocket(-1);
		socket = ConnectSerialServer((const int8_t *)phandle->ipaddr, phandle->port);
		if(socket < 0)
		{
			zlog_error(MLOG, "ConnectSerialServer failed!");
			r_sleep(5);
			continue;
		}
		
		zlog_debug(MLOG, "ConnectSerialServer [%d]!\n",socket);
		
		SetTcpSocket(socket);
		pbuf = (int8_t *)phandle->buf;
		phandle->socket = socket;
		phandle->RunStatus = SERIAL_RUN;
		
		while(phandle->RunStatus == SERIAL_RUN)
		{
			int seret = 0;		
			timeout.tv_sec = 60;
		
			FD_ZERO(&readfd);
			FD_SET(socket, &readfd);		
			seret = r_select(socket+1, &readfd, NULL, NULL, &timeout);
			if(seret > 0)
				// FD已准备好
			{
				if(FD_ISSET(socket, &readfd))
				{
					int len 	= 0;
					int recvlen = 0;
					MsgHeader msg;
					
					TcpSockLock();
					recvlen = tcp_recv_longdata(socket, (int8_t *)&msg, MSGLEN);
					if(recvlen < MSGLEN || recvlen == -1){	
						zlog_debug(MLOG, "nLen < HEAD_LEN  errno = %d  nLen = %d\n", errno, recvlen);
						if(errno !=  EAGAIN)
						{
							//fprintf(stderr, "errno !=  EAGAIN, errmsg = %s\n", strerror(errno));
							r_usleep(100000);
						}
						
						SetTcpSocket(-1);
						r_close(socket);
						r_usleep(1000);
						TcpSockunLock();
						break;
					}

					
					msg.sMsgType = ntohs(msg.sMsgType);
					msg.sLen = ntohs(msg.sLen);
					msg.sVer = ntohs(msg.sVer);
					

					r_memset(pcmd,0x0, sizeof(pcmd));
					//r_memcpy(pcmd, (int8_t *)&msg, MSGLEN);

				//	zlog_debug(MLOG, "========= msg.sMsgType = %d, msg.sLen = %d, msg.sVer = %d\n", msg.sMsgType, msg.sLen, msg.sVer);

					//TcpSockLock();
					len = tcp_recv_longdata(socket, pcmd, msg.sLen-MSGLEN);
					//zlog_debug(MLOG, "========= len = %d\n", len);
					TcpSockunLock();
					if(len < 1)
					{							
						if(errno !=  EAGAIN)
						{
							//fprintf(stderr, "errno !=  EAGAIN, errmsg = %s\n", strerror(errno));
							r_usleep(100000);
						}
						
						SetTcpSocket(-1);
						r_close(socket);
						r_usleep(1000);
						break;
					}

					/* 解析NET信令codeID */
				
					
					pmsg = (MsgHeader* )pcmd;
					Int32 MsgCode;
					if(1 != Analyze_MsgCode(pcmd,&MsgCode))
					{
						continue;
					}

					if(MsgCode != 30019 && MsgCode != 30024)
						zlog_debug(MLOG, "%s",pcmd);

					DelTimeOut(phandle, MsgCode);
				
					/* 获取实例ID */
					UInt32 num = NGetGlobalCmdIndex(MsgCode, 0);
					if(num == -1)
					{
						zlog_error(MLOG, "NGetGlobalCmdIndex: MsgCode[%d]  is invalid!!",MsgCode);					
						continue;
					}
					
					SerialComnd *operate;
					operate = GetSerialOperate(num);
					if(operate == NULL)
					{
						zlog_error(MLOG, "GetSerialOperate: cmd[%d] operate is invalid!!",MsgCode);
						
						continue;
					}

					if(operate->Operate == NULL)
					{
						zlog_error(MLOG, "Operate: Operate[%d]  is NUll!!",MsgCode);
						
						continue;
					}

					SerialParam prm;
					r_memset(&prm, 0, sizeof(SerialParam));
					if(1 !=operate->PackageAnalysis((Int8 *)pcmd,&prm))
					{
						zlog_error(MLOG, "PackageAnalysis: Analysis MsgCode[%d] is fail!!",MsgCode);
						
						continue;
					}
			
					if(1!= operate->Operate(num, &prm))
					{
						zlog_error(MLOG, "Operate: Operate[%d]  is invalid!!",num);
						
						continue;
					}
					
				}
			}
			else if(seret == 0)
			// 超时
			{
			//	printf("!!!!!!!!!!!timeout!!!!!!!!!!!!!!!\n");
				r_usleep(1000);
				continue;
			}
			else if(seret < 0)
						// 异常
			{
				zlog_error(pserial_handle->c,"!!!!!!!!!!!select < 0!!!!!!!!!!!!!!!");
				SetTcpSocket(-1);
				r_close(socket);
				r_usleep(1000);		
				break;
			}
			
		}
		
	}

	TcpSockLock();
	phandle->RunStatus = SERIAL_STOP;
	TcpSockunLock();
	
	SetTcpSocket(-1);
	if(socket >= 0)
		r_close(socket);
	if(pcmd != NULL)
	{
		r_free(pcmd);
	}
	
	return NULL;
}

#if 0
void *SerialClientTask(void *args)
{
	pthread_t thid;
	Int32 ret = 0;
	void *value = NULL;

	if(NULL == args)
	{
		zlog_error(MLOG, "SerialClientTask: args is invalid!!");
		return NULL;
	}

	SerialHandle *phandle = (SerialHandle *)args;
	
	while(1)
	{	
		ret = r_pthread_create(&thid, NULL, SerialComTask, (void *)phandle);
		if(ret)
		{
			zlog_error(MLOG, "SerialComTask failed, err msg: %s\n", strerror(errno));
		}

		r_pthread_join(thid, &value);
		r_sleep(10);
	}

	return 0;
}
#endif

void get_realtime_param(params_table *param_table)
{
	if (NULL == gpser || NULL == gpser->pserinfo) {
		zlog_error(DBGLOG, "serinfo is null");
		
		return;
	}

	pthread_mutex_lock(&gpser->pserinfo->info_m);

	r_strcpy(param_table->version, (const int8_t *)gpser->pserinfo->ServerInfo.ServerVersion); 
	r_strcpy(param_table->serial_num, (const int8_t *)gpser->pserinfo->ServerInfo.ServerSeries); 

	param_table->ip_addr	= gpser->pserinfo->ServerInfo.LanAddr; 
	param_table->gateway 	= gpser->pserinfo->ServerInfo.LanGateWay; 

	param_table->net_mask	= gpser->pserinfo->ServerInfo.LanNetmask; 
	r_memcpy(param_table->mac_addr, gpser->pserinfo->ServerInfo.LanMac, sizeof(param_table->mac_addr)); 

	param_table->DiskMaxSpace		= gpser->pserinfo->SysInfo.DiskMaxSpace; 
	param_table->DiskAvailableSpace	= gpser->pserinfo->SysInfo.DiskAvailableSpace; 
	
	pthread_mutex_unlock(&gpser->pserinfo->info_m);

}

static int InfoIpMenu(int spi_fd)
{
	params_table 	param_table;
	r_memset(&param_table, 0, sizeof(params_table));

	get_realtime_param(&param_table);
	
	int8_t Addr[100];
	int8_t NetMask[100];
	int8_t GateWay[100];

	r_memset(Addr, 0, 100);
	r_memset(NetMask, 0, 100);
	r_memset(GateWay, 0, 100);
	
	struct in_addr addr;
	ShowString(spi_fd,"State:      Ready	     ",0,5);

	

	r_memcpy(&addr, &param_table.ip_addr, 4);
	sprintf((char *)Addr,"Addr:      %s",(int8_t *)inet_ntoa(addr));
	
	r_memcpy(&addr, &param_table.net_mask, 4);
	sprintf((char *)NetMask,"NetMask:  %s",(int8_t *)inet_ntoa(addr));
	
	r_memcpy(&addr, &param_table.gateway, 4);
	sprintf((char *)GateWay,"GateWay:  %s",(int8_t *)inet_ntoa(addr));
	
	ShowString(spi_fd,(char *)Addr,0,20);
	ShowString(spi_fd,(char *)NetMask,0,35);
	ShowString(spi_fd,(char *)GateWay,0,50);
	return 1;
}

#if 0
static int InfoMemMenu(int spi_fd)
{
	params_table 	param_table = {0};
	get_realtime_param(&param_table);
	int8_t DiskState[100] = {0};
	int8_t Resolution[100] = {0};
	int8_t Bitrate[100] = {0};
	float disksize = param_table.DiskAvailableSpace/1024/1024;
	sprintf(DiskState, "DiskState:   %0.2fG",disksize);
	sprintf(Resolution,"Resolution:  %s","1080P");
	sprintf(Bitrate,   "Bitrate:     %s","2Mbs");

	ShowString(spi_fd,"State:      Ready	     ",0,5);
	ShowString(spi_fd,DiskState,0,20);
	ShowString(spi_fd,Resolution,0,35);
	ShowString(spi_fd,Bitrate,0,50);
	return 1;
}
#endif

//获取分辨率
int GetResolution(char *cmd)
{
	switch(get_synt_resoluteion())
	{
		case 1:
			sprintf(cmd,"NTSC");
			break;
		case 2:
			sprintf(cmd,"PAL");
			break;
		case 3:
			sprintf(cmd,"720P");
			break;
		case 4:
			sprintf(cmd,"1080P");
			break;
		default:
			sprintf(cmd,"1080P");
			break;
	}
	
	return 0;
}

int GetBiteRate(int *Bitrate)
{
	*Bitrate = get_synt_bitrate();
	return 0;
}

int GetRecordTime(char *cmd)
{
	sprintf(cmd,"Time:   01:01:01      ");
	return 0;
}

int GetSize(char *cmd)
{
	sprintf(cmd,"Size:   1G      ");
	return 0;
}

static int InfoMemMenuE(int spi_fd)
{
	params_table 	param_table;
	get_realtime_param(&param_table);
	int8_t DiskState[100] = {0};
	int8_t Resolution[100] = {0};
	int8_t Bitrate[100] = {0};
	int8_t cmd[256]= {0};
	int32_t bitrate = 0;
	int32_t time = 0;

	//r_memset(&param_table, 0, sizeof(params_table));
	
	float disksize0 = param_table.DiskAvailableSpace;
	float disksize1	= disksize0/1024/1024;

	GetBiteRate(&bitrate);
	time = disksize0/(bitrate+256+372)/450;
	sprintf((char *)DiskState, "FreeDisk:  %0.2fG  %d hours",disksize1,time);

	GetResolution((char *)cmd);
	sprintf((char *)Resolution,"Resolution: %s   ",cmd);
	sprintf((char *)Bitrate,"BitRate:    %d kbps",bitrate);
	ShowString(spi_fd,"State:      Ready	     ",0,5);
	ShowString(spi_fd,(char *)DiskState,0,20);
	ShowString(spi_fd,(char *)Resolution,0,35);
	ShowString(spi_fd,(char *)Bitrate,0,50);

	//ShowString(spi_fd,Bitrate,0,50);
	return 1;
}


static int TypeConversion(char *src_str,int src_int)
{
	if(src_int == 0 )
	{
		strcpy(src_str,"00");
	}
	else if(src_int < 10)
	{
		sprintf(src_str,"0%d",src_int);
	}
	else
	{
		sprintf(src_str,"%d",src_int);
	}

	return 0;
}


static int RecordIngMenu(int spi_fd)
{
	char cmd_time[256] = {0};
	char cmd_size[256] = {0};

	char time_hour_str[3] = {0};
	char time_min_str[3] = {0};
	char time_sec_str[3] = {0};

	int time_hour = 0; 
	int time_min = 0; 
	int time_sec = 0; 

	int size_GB = 0; 
	int size_MB = 0; 
	int size_KB = 0; 

	int file_size = 0;
	PanelRecInfo recinfo;
	memset(&recinfo ,0 ,sizeof(PanelRecInfo));
	if(PanelGetRecInfo(&recinfo) <0 )
	{
		printf("error ! RecordIngMenu - PanelGetRecInfo \n");
		GetRecordTime(cmd_time);
		GetSize(cmd_size);
	}
	else
	{
		time_hour 	= recinfo.rec_total_time/3600 ;
		time_min 	= recinfo.rec_total_time%3600/60;
		time_sec 	= recinfo.rec_total_time%3600%60;

		TypeConversion(time_hour_str,time_hour);
		TypeConversion(time_min_str,time_min);
		TypeConversion(time_sec_str,time_sec);
		
		printf("IMPORT INT ----------- <H : %d> <M : %d> <S : %d>\n",time_hour,time_min,time_sec);
		printf("IMPORT STR ----------- <H : %s> <M : %s> <S : %s>\n",time_hour_str,time_min_str,time_sec_str);

		sprintf(cmd_time,"Time:    %s :%s :%s  ",time_hour_str,time_min_str,time_sec_str);


		printf("IMPORT PATH ------------ <%s>\n",recinfo.rec_file_path);
		file_size = list_dir((int8_t *)(recinfo.rec_file_path));

		size_GB = file_size/(1024*1024); 
		size_MB = file_size%(1024*1024)/1024; 
	 	size_KB = file_size%(1024*1024)%1024; 

		printf("IMPORT INT ----------- <GB : %d> <MB : %d> <KB : %d>\n",size_GB,size_MB,size_KB);

		sprintf(cmd_size,"Size:    %dG  %dM  ",size_GB,size_MB);
	
	}
	//ClearScreen(spi_fd);
	ShowString(spi_fd,"State:      Recording	  ",0,5);
	ShowString(spi_fd,cmd_time,0,20);
	ShowString(spi_fd,cmd_size,0,35);
	//ShowString(spi_fd,Resolution,0,35);
	//ShowString(spi_fd,Bitrate,0,50);
	return 1;
}


void *TimeVolumeReq(void *args)
{

	SerialHandle *pSerialHandle = (SerialHandle *)args;
	char VolumeBuff[MAX_XMNL_LEN]={0};
	int len = 0;
	pSerialHandle->LedState = 0;
	uint32_t times = 5;
	r_memset(VolumeBuff, 0x0 ,MAX_XMNL_LEN);
	len = test_volume_req_msg(VolumeBuff + MSGLEN);
	if (len <= 0) {
		zlog_error(MLOG, "TimeVolumeReq test_volume_req_msg ret is error, [len = %d]!!", len);
		return NULL;
	}

	
	while(1)
	{
		if((pSerialHandle->LedState == IPMENU) && (times >= 5 || pserial_handle->LedFLUSH == 1 ))
		{
			ClearScreen(fd);
			InfoIpMenu(fd);
			pSerialHandle->LedState = MEMMENU;
			pserial_handle->LedFLUSH = 0;
			times = 0;
		}
		else if((pSerialHandle->LedState == MEMMENU) && (times >= 5 ||  pserial_handle->LedFLUSH == 1))
		{
			ClearScreen(fd);
			InfoMemMenuE(fd);

			if(check_all_record_status(gpser))
			{
				pSerialHandle->LedState = RECORDING;
			}
			else
			{
				pSerialHandle->LedState = IPMENU;
			}
			
			pserial_handle->LedFLUSH = 0;
			times = 0;
		}
		else if((pSerialHandle->LedState == RECORDING) && (times >= 5 ||  pserial_handle->LedFLUSH == 1))
		{
			ClearScreen(fd);
			RecordIngMenu(fd);
			pSerialHandle->LedState = IPMENU;
			pserial_handle->LedFLUSH = 0;
			times = 0;
		}
		else if((pSerialHandle->LedState == MENUINFO) && (pserial_handle->LedFLUSH == 1))
		{
	
			pserial_handle->LedFLUSH = 0;
			int num;
			MNUsbDeb dev[5] = {{{0}, 0, 0, 0}};
			int i = 0;
			//等待设备挂载
			sleep(3);
			GetUsbDevState(&num, dev, 5);
			if(num <= 0)
			{
				pSerialHandle->LedState = IPMENU;
			}
			else if(num == 1)
			{
				strcpy((char *)pserial_handle->UsdStat.UsbDevName,dev[0].devname);
				pserial_handle->UsdStat.TotalSize= dev[0].total_size;
				pserial_handle->UsdStat.FreeSize = dev[0].free_size;
				ShowString(fd,"     success     ",15,20);
				sleep(1);
				pserial_handle->LedState = IPMENU;
				pserial_handle->LedFLUSH = 1;
			}		
			else
			{	
				ClearScreen(fd);
				pSerialHandle->LedState = SELECTUSBMENU;
			
				char cmd[1024] = {0};
				for(i =0; i < num; i++)
				{
					float size = dev[i].free_size;
					size	= size/1024/1024;
					sprintf(cmd,"Dev:%s Free:%0.2fG",dev[i].devname,size);
					
					if(i == 0)
					{
						YinCodeShowString(fd,cmd,1,(5+i*15));
					}
					else
					{
						ShowString(fd,cmd,1,(5+i*15));
					}
				}
			}
			
			times = 0;
		}

		
		if(1 != NetOperate(len, VolumeBuff))
		{
			zlog_error(MLOG, "TimeVolumeReq NetOperate: is error!!");
		}


		if(pserial_handle->UsdStat.IsUsbDev == 1)
		{

			if((pSerialHandle->LedState != SELECTUSBMENU) && (pSerialHandle->LedState !=DOWNMENU)
				&& (pSerialHandle->LedState !=REPLAYMENU))
				ShowString(fd, "usb", 56, 50);
			
			
		}
		
	#if 0
		r_memset(VolumeBuff, 0x0 ,MAX_XMNL_LEN);
		len = test_volume_req_msg(VolumeBuff + MSGLEN);
	
		if(1 != NetOperate(len, VolumeBuff))
		{
			zlog_error(MLOG, "TimeVolumeReq NetOperate: is error!!");
		}

		

		{

			/* 反馈源状态 */
			Int8 cmd[]={0x3c,0x3c,0xc2,0x91,0x2,0x2,0x2,0x3e,0x3e};
			Int8 *pcmd;
			pcmd = cmd;
			cmd[4] = times%2 + 1;
			cmd[5] = times%2 + 1;
			cmd[6] = times%2 + 1;

			
			if(0 >= SerialWrite(pcmd, 9))
			{
		
			}

			cmd[3] =0x92;
			/* 反馈录制状态 */
			cmd[4] = times%2 + 1;
			cmd[5] = times%2 + 1;
			cmd[6] = times%2 + 1;
			if(0 >= SerialWrite(pcmd, 9))
			{
			
			}

			{
				Int8 cmd[]={0x3c,0x3c,0xc2,0x90,0x1,0x3e,0x3e};
				pcmd   = cmd;
				pcmd[4] = times%2 + 1;
				if(0 >= SerialWrite(pcmd, 7))
				{
					
				}
			}

		}
		#endif
		times++;
		r_sleep(1);
		//usleep(300*1000);
	}

	return NULL;
}

void *TimeOutdeal(void *args)
{
	SerialHandle *pSerialHandle = (SerialHandle *)args;
	list_head *pcurnode = NULL;
	TimeOut *pattr = NULL;
	while(1)
	{
		r_sleep(3);
		TimeoutLock();
		/* 查找是否存在该设备 */
		list_for_each_item(pcurnode, pSerialHandle->pheadnode)  
	    {  
	        if(NULL != pcurnode)  
	        {  
				int32_t ret_val = 0;
	            pattr = list_entry(pcurnode, TimeOut, stlist);  

			 	ret_val = upper_msg_monitor_time_out_status(&pattr->starttime, 1);
				if(1 == ret_val)
				{	
					//printf("111111111111111111111 %d\n",pattr->state);
					//超时则删除
					list_remove(pcurnode);
					free(pattr);
				}
	        }  
	    }  
		TimeoutunLock();
	}
}

int32_t CheckTimeOut(SerialHandle *pSerialHandle,SERIAL_MSGCODE msgcode)
{
	list_head *pcurnode = NULL;
	TimeOut *pattr = NULL;

	TimeoutLock();
	/* 查找是否存在该设备 */
	list_for_each_item(pcurnode, pSerialHandle->pheadnode)  
    {  
        if(NULL != pcurnode)  
        {  
            pattr = list_entry(pcurnode, TimeOut, stlist);  

		 	if(pattr->msgcode == msgcode)
		 	{
				TimeoutunLock();
				//此消息等待超时
				return 1;
			}
        }  
    }  
	TimeoutunLock();
	return 0;
}

int32_t SetUsbFunction(int32_t IsUse)
{
	if(pserial_handle == NULL)
	{
		return 0;
	}
	if(IsUse == 1)
	{
		FILE *file = fopen(".UsbFunction","w");
		if(file != NULL)
		{
			fclose(file);
			pserial_handle->IsUse = 1;
			return 1;
		}
		else
		{
			return 0;
		}
		
	}
	else 
	{
		unlink(".UsbFunction");
		pserial_handle->IsUse = 2;
	}
	return 1;
}

int32_t GetUsbFunction(int32_t *IsUse)
{
	if(pserial_handle == NULL || IsUse == NULL)
	{
		return 0;
	}
	
	if(0 == access(".UsbFunction",0))
	{
		*IsUse = 1;
	}
	else
	{
		*IsUse = 2;
	}
	
	return 1;
}

int32_t SerialInit()
{
	list_head *headnode = NULL;

	/*取消PIPE坏的信号*/
	Signal(SIGPIPE, SIG_IGN);

	#if 0
	rc = zlog_init("/usr/local/reach/zlog_upload.conf");
	if (rc)
	{
		printf("init failed\n");
		return -1;
	}
	else
	{
		printf("zlog init success!!\n");
	}
	#endif
	
	pserial_handle = (SerialHandle *)r_malloc(sizeof(SerialHandle));
	if(NULL == pserial_handle)
	{
		printf("RegisterrSerialControlTask failed, malloc ftp handle error, err msg = %s\n",strerror(errno));
		return -1;
	}

	memset(pserial_handle,0x0, sizeof(SerialHandle));

	headnode = (list_head *)r_malloc(sizeof(list_head));
	if(NULL == headnode)
	{
		printf("RegisterrSerialControlTask failed, malloc list_head error, err msg = %s\n",strerror(errno));
		goto EXIT;
	}

	pserial_handle->FileList  = (Int8 *)r_malloc(50*100);
	if(pserial_handle->FileList == NULL)
	{
		printf("RegisterrSerialControlTask failed, malloc FileList error, err msg = %s\n",strerror(errno));
		goto EXIT;
	}
	
	pserial_handle->c  = zlog_get_category("SerialLog");
	if (!pserial_handle->c)
	{
		goto EXIT;
	}
	
	MLOG = pserial_handle->c;
	pserial_handle->RunStatus = SERIAL_STOP;
	pserial_handle->pheadnode = headnode;
	list_init(pserial_handle->pheadnode);

   //锁初始化
	TimeoutLockInit();
	TcpSockLockInit();
	SerialLockInit();
	SerialLockInit1();
	
	r_strcpy((int8_t *)pserial_handle->ipaddr, (int8_t *)SERIAL_SERVER_IPADDR);
	pserial_handle->port      = SERIAL_SERVER_IPADDR_PORT;

	pserial_handle->ListPosition = 0;
	pserial_handle->totalfilenum = 0;
	zlog_debug(MLOG,"Serial Init success!!\n");

	if(0 == access(".UsbFunction",0))
	{
		pserial_handle->IsUse = 1;
	}
	else
	{
		pserial_handle->IsUse = 2;
	}
	return 1;

EXIT:

	if(NULL != pserial_handle)
	{
		r_free(pserial_handle);
	}

	if(NULL != headnode)
	{
		r_free(headnode);
	}

	/* 日志模块销毁 */
	zlog_fini();
	
	return -1;
}


int32_t deSerialInit()
{

	if(NULL != pserial_handle)
	{

		//清空超时队列
		if(NULL != pserial_handle->pheadnode)
		{
			list_head *pcurnode = NULL;
			list_for_each_item(pcurnode, pserial_handle->pheadnode)  
		    {  
		        if(NULL != pcurnode)  
		        {  
					TimeOut *pattr = NULL;
		            pattr = list_entry(pcurnode, TimeOut, stlist);  
					list_remove(pcurnode);
					r_free(pattr);
				}
		    }  

			r_free(pserial_handle->pheadnode);
			pserial_handle->pheadnode = NULL;
		}  

	    //销毁各种锁
		TimeoutunLock();
		TcpSockLockdeInit();
		SerialLockdeInit();
		SerialLockdeInit1();
		TimeoutdeInit();
		if(pserial_handle->FileList != NULL)
		{
			r_free(pserial_handle->FileList);
		}
		//释放内存
		r_free(pserial_handle);
		pserial_handle = NULL;
		
		//日志销毁
		zlog_fini();

		//XML销毁
		xmlCleanupParser();

		printf("Serial DeInit success!!!\n");
	}

	return 0;
}

Int32 ClearStateLed()
{
	Int8 sendcmd[]={0x3c,0x3c,0xc2,0x94,0x2,0x3e,0x3e};
	Int8 statcmd[]={0x3c,0x3c,0xc2,0x92,0x2,0x2,0x2,0x3e,0x3e};
	Int8 Valuecmd[]={0x3c,0x3c,0xc2,0x93,0x0,0x3e,0x3e};
	SerialWrite(sendcmd,7);
	sendcmd[3] = 0x95;
	usleep(100*1000);
	SerialWrite(sendcmd,7);
	usleep(100*1000);
	SerialWrite(statcmd,9);
	usleep(100*1000);
	SerialWrite(Valuecmd,7);
	printf("\n\n----------------ClearStateLed {0x3c,0x3c,0xc2,0x92,0x2,0x2,0x2,0x3e,0x3e}------------------\n\n\n");

	pserial_handle->LedState = REBOOTING ;
	ClearScreen(pserial_handle->ledfd);
	ShowString(pserial_handle->ledfd, "      Rebooting     ", 10,30);

	return 0;
}

Int32 RegisterrSerialControlTask()
{
	pthread_t thid = 0;
	pthread_t SerialRecvTaskId = 0;
	pthread_t SerialRecvTaskIdEx = 0;
	pthread_t TimeReqVolume = 0;
	pthread_t TimeOutid = 0;
	
	if(1 != SerialInit())
	{
		return -1;
	}

	fd = InitLed();
	if(fd < 0)
	{
		printf("InitLed is fail !!\n");
		return 0;
	}

	pserial_handle->ledfd = fd;
	ClearScreen(fd);

	//Word16x16(fd,30,10,10,0);
	//packet   (fd,10,10,10,0);

	while(0)
	{
		ShowString(fd,"AaBbCcDd12311111111111111",0, 0);
		usleep(100*1000);
		//usleep(100*1000);
		ShowString(fd,"AaBbCcDd12311111111111111",0, 30);
	//	YinCodeShowString(fd,"AaBbCcDd1231111111111",0, 30);
		usleep(100*1000);
	}

		
	#if 1
	#if 1
	if(r_pthread_create(&thid, NULL, SerialComTask, (void *)pserial_handle))
	{
		zlog_error(pserial_handle->c, "start_ftpupload_com_task failed, err msg: %s\n", strerror(errno));
		goto EXIT;
		
	}

	if (r_pthread_create(&SerialRecvTaskId,NULL, SerialCenterctrlThread, (void *)pserial_handle))
	{
		zlog_error(pserial_handle->c,"Failed to create clientThread\n");
		goto EXIT;
	}

	if (r_pthread_create(&SerialRecvTaskIdEx,NULL, SerialCenterctrlThreadEx, (void *)pserial_handle))
	{
		zlog_error(pserial_handle->c,"Failed to create clientThread\n");
		goto EXIT;
	}
	#endif
	
	if (r_pthread_create(&TimeReqVolume,NULL, TimeVolumeReq, (void *)pserial_handle))
	{
		zlog_error(pserial_handle->c,"Failed to create clientThread\n");
		goto EXIT;
	}


	#if 1
	if (r_pthread_create(&TimeOutid,NULL, TimeOutdeal, (void *)pserial_handle))
	{
		zlog_error(pserial_handle->c,"Failed to create TimeOutdeal\n");
		goto EXIT;
	}

	
	#endif
	
	#if 0
	r_pthread_join(thid, NULL);
	r_pthread_join(SerialRecvTaskId, NULL);
	r_pthread_join(TimeReqVolume, NULL);
	r_pthread_join(TimeOutid, NULL);
	#endif
	
EXIT:

	#if 0
	if(thid)
		kill(thid, SIGKILL);
	if(SerialRecvTaskId)
		kill(SerialRecvTaskId, SIGKILL);
	if(TimeReqVolume)
		kill(TimeReqVolume, SIGKILL);
	if(TimeOutid)
		kill(TimeOutid, SIGKILL);
	
	deSerialInit();
	#endif

	#endif
	return 1;
}


#if 0

int main()
{
	RegisterrSerialControlTask();
}
#endif
