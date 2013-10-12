#include "daemon.h"

DaemonHand *pDaemeonOper = NULL;
#if 0
typedef struct _top_proc
{
    pid_t pid;
    uid_t uid;
    char name[64];
    int pri, nice;
    unsigned long size, rss;	/* in k */
    int state;
    unsigned long time;
    double pcpu, wcpu;
}top_proc;
    
uid_t proc_owner(pid_t pid)
{
    struct stat sb;
    char buffer[32];
    sprintf(buffer, "%d", pid);

    if (stat(buffer, &sb) < 0)
	return -1;
    else
	return sb.st_uid;
}

#define NCPUSTATES 4

static char *cpustatenames[NCPUSTATES+1] =
{
    "user", "nice", "system", "idle",
    NULL
};

#define NMEMSTATS 6
static char *memorynames[NMEMSTATS+1] =
{
    "K used, ", "K free, ", "K shd, ", "K buf  Swap: ",
    "K used, ", "K free",
    NULL
};

#endif

static void DaemonLock()
{
	pthread_mutex_lock(&pDaemeonOper->daemonlock);
}

static void DaemonunLock()
{
	pthread_mutex_unlock(&pDaemeonOper->daemonlock);
}

static int DaemonLockInit()
{
	return pthread_mutex_init(&pDaemeonOper->daemonlock, NULL);
}

static int DaemondeInit()
{
	return pthread_mutex_destroy(&pDaemeonOper->daemonlock);
}

/*==============================================================================
    函数: <InitDaemon>
    功能: 守护进程初始化
    参数: 
    返回值: 
    Created By 徐崇 2012.12.18 15:06:59 For Web
==============================================================================*/
void InitDaemon( void )
{
	int i;
	pid_t pid;

	//第一步
	if((pid=fork())) 
	{
		exit(0);//是父进程,结束父进程
	} 
	else
	{
	if( pid< 0 ) 
	 exit(1);//fork失败,退出
	}  

	//第二步
	setsid();//是第一个子进程,后台继续运行
	//第一个子进程成为新的会话组长和进程组长,并与控制终端分离

	//防止子进程结束时产生僵尸进程
	#if 0
	signal(SIGPIPE,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGINT,SIG_IGN);
	signal(SIGCHLD,SIG_IGN);
	#endif
	//第三步
	if((pid=fork()))//禁止进程重新打开控制终端 
	{
	exit(0);//是第一个子进程,结束第一个子进程
	} 
	else
	{
	if( pid<0 ) 
	exit(1);//fork失败,退出
	}  

	//第四步
	for(i=0; i<NOFILE; i++)
	{
		//文件描述符暂不关闭 解决继承出错问题
		//	 close(i);//关闭从父进程那里继承的一些已经打开的文件
	}
	//第五步
	chdir("/usr/local/reach");//改变工作目录

	//第六步
	umask(0);//重设文件权限掩码  
}	


/*==============================================================================
    函数: <FindPidByName>
    功能: 查找进程
    参数: 
    返回值: 
    Created By 徐崇 2012.12.18 15:06:09 For Web
==============================================================================*/
uint32_t FindPidByName(NodeAtt* pmodeattr,uint32_t *pidList)
{
	DIR *dir;
	struct dirent *next;
	uint32_t i=0;

	if(pidList == NULL)
	{
		return 0;
	}
	
	///proc中包括当前的进程信息,读取该目录
	dir = opendir("/proc");
	if (!dir)
	{
		return 0;
	}
	
	//遍历
	while ((next = readdir(dir)) != NULL)
	{
		FILE *status;
		int8_t filename[READ_BUF_SIZE];
		int8_t buffer[READ_BUF_SIZE];
		int8_t name[READ_BUF_SIZE];
		int8_t param[READ_BUF_SIZE];
		
		if (r_strcmp((int8_t*)next->d_name, (int8_t*)"..") == 0)
			continue;
			
		if (!isdigit(*next->d_name))
			continue;
			
		sprintf((char *)filename, "/proc/%s/cmdline", next->d_name);
		if (! (status = fopen((char *)filename, "r")) )
		{
			continue;
		}

		r_memset(buffer, 0x0,READ_BUF_SIZE);
		if (fgets((char *)buffer, READ_BUF_SIZE-1, status) == NULL)
		{
			fclose(status);
			continue;
		}
		fclose(status);
		int8_t *p = buffer;


		//获取进程名字
		sscanf((char *)buffer, "%s", (char *)name);
		
		p = (int8_t*)strchr((char *)buffer,0);
		
		p = p+1;

		if(*p != 0)
		{
			//获取运行参数
			sscanf((char *)p,"%s",(char *)param);
		}
		else
		{
			r_strcpy(param,(int8_t*)"");
		}
		
		if ( (r_strcmp(name, (int8_t*)pmodeattr->procname) == 0) 
			 &&(r_strcmp(param, (int8_t*)pmodeattr->param) == 0))
		{
			//printf("find-->[%s] [%s] [%s] [%s] %d\n",buffer,param,pmodeattr->procname,pmodeattr->param,strtol(next->d_name, NULL, 0));
			//printf("%s %d\n",buffer,strtol(next->d_name, NULL, 0));
			//pidList= realloc( pidList, sizeof(long) * (i+2));
			pidList[i++] = strtol(next->d_name, NULL, 0);
		}
	}
	closedir(dir);
	pidList[i]=0;
	return i;
	}

uint32_t CheckProgramNum(uint32_t *list)
{
	uint32_t num = 0;
	if(list == NULL)
	{
		return num;
	}
	
	while(*list)
	{
		num++;
		list++;
	}
	return num;
}



char *skip_ws(const char *p)
{
    while (isspace(*p)) p++;
    return (char *)p;
}
    
char *skip_token(const char *p)
{
    while (isspace(*p)) p++;
    while (*p && !isspace(*p)) p++;
    return (char *)p;
}


#if 0

int32_t read_one_proc_stat(pid_t pid, top_proc *proc)
{
    char buffer[PROCBUFFERLEN], *p;

    /* grab the proc stat info in one go */
    {
	int fd, len;

	sprintf(buffer, "/proc/%d/stat", pid);

	fd = open(buffer, O_RDONLY);
	if(fd == -1)
	{
		return -1;
	}
	
	len = read(fd, buffer, sizeof(buffer)-1);
	close(fd);

	buffer[len] = '\0';
    }

    proc->uid = proc_owner(pid);

    /* parse out the status */
    p = buffer;
    p = strchr(p, '(')+1;			/* skip pid */
    {
	char *q = strrchr(p, ')');
	int len = q-p;
	if (len >= sizeof(proc->name))
	    len = sizeof(proc->name)-1;
	memcpy(proc->name, p, len);
	proc->name[len] = 0;
	p = q+1;
    }

    p = skip_ws(p);
    switch (*p++)
    {
      case 'R': proc->state = 1; break;
      case 'S': proc->state = 2; break;
      case 'D': proc->state = 3; break;
      case 'Z': proc->state = 4; break;
      case 'T': proc->state = 5; break;
      case 'W': proc->state = 6; break;
    }
    p = skip_token(p);				/* skip ppid */
    p = skip_token(p);				/* skip pgrp */
    p = skip_token(p);				/* skip session */
    p = skip_token(p);				/* skip tty */
    p = skip_token(p);				/* skip tty pgrp */
    p = skip_token(p);				/* skip flags */
    p = skip_token(p);				/* skip min flt */
    p = skip_token(p);				/* skip cmin flt */
    p = skip_token(p);				/* skip maj flt */
    p = skip_token(p);				/* skip cmaj flt */
    
    proc->time = strtoul(p, &p, 10);		/* utime */
    proc->time += strtoul(p, &p, 10);		/* stime */

    p = skip_token(p);				/* skip cutime */
    p = skip_token(p);				/* skip cstime */

    proc->pri = strtol(p, &p, 10);		/* priority */
    proc->nice = strtol(p, &p, 10);		/* nice */

    p = skip_token(p);				/* skip timeout */
    p = skip_token(p);				/* skip it_real_val */
    p = skip_token(p);				/* skip start_time */

    proc->size = bytetok(strtoul(p, &p, 10));	/* vsize */
    proc->rss = pagetok(strtoul(p, &p, 10));	/* rss */
}

void get_system_info( system_info *info)
{
    char buffer[PROCBUFFERLEN];
	uint32_t cp_time = 0;
	uint32_t memory_stats[NMEMSTATS];
    int32_t fd, len;
    uint8_t *p;
    int32_t i;

    /* get the cpu time info */
    {
	fd = r_open("/proc/stat", O_RDONLY);
	if(fd == -1)
	{
		return -1;
	}

	
	len = read(fd, buffer, sizeof(buffer)-1);
	close(fd);
	buffer[len] = '\0';

	
	p = skip_token(buffer);			/* "cpu" */
	cp_time    = strtoul(p, &p, 10);
	
	cp_time  += strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	info->TotalCpu = cp_time;
    }
	
    /* get system wide memory usage */
    {
	char *p;

	fd = r_open("/proc/meminfo", O_RDONLY);
	if(fd == -1)
	{
		return -1;
	}
	
	len = r_read(fd, buffer, sizeof(buffer)-1);
	r_close(fd);
	buffer[len] = '\0';

	/* be prepared for extra columns to appear be seeking
	   to ends of lines */
	
	p = buffer;
	p = skip_token(p);
	memory_stats[0] = strtoul(p, &p, 10); /* total memory */
	info->TotalMem= memory_stats[0];
	
	p = strchr(p, '\n');
	p = skip_token(p);
	memory_stats[1] = strtoul(p, &p, 10); /* free memory */
	
	
	p = strchr(p, '\n');
	p = skip_token(p);
	memory_stats[2] = strtoul(p, &p, 10); /* buffer memory */
	
	p = strchr(p, '\n');
	p = skip_token(p);
	memory_stats[3] = strtoul(p, &p, 10); /* cached memory */
	
	for(i = 0; i< 8 ;i++) {
		p++;
		p = strchr(p, '\n');
	}
	
	p = skip_token(p);
	memory_stats[4] = strtoul(p, &p, 10); /* total swap */
	
	p = strchr(p, '\n');
	p = skip_token(p);
	memory_stats[5] = strtoul(p, &p, 10); /* free swap */
	
    }


}
#endif


/*==============================================================================
    函数: <GetSysCpuTime>
    功能: 获取系统cpu时间
    参数: 
    返回值: 
    Created By 徐崇 2012.12.18 15:05:48 For Web
==============================================================================*/
uint32_t GetSysCpuTime()
{
	char buffer[PROCBUFFERLEN];
	uint32_t cp_time = 0;
    int32_t fd, len;
    char *p;
	fd = r_open((int8_t*)"/proc/stat", (int32_t)O_RDONLY);
	if(fd == -1)
	{
		return -1;
	}

	
	len = read(fd, buffer, sizeof(buffer)-1);
	if(len <= 0)
	{
		r_close(fd);
		return -1;
	}
	
	r_close(fd);
	
	buffer[len] = '\0';
	
	p = skip_token(buffer);			/* "cpu" */
	cp_time  = strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	cp_time  += strtoul(p, &p, 10);
	
	return cp_time;
   
}

/*==============================================================================
    函数: <GetSysTotalMem>
    功能: 获取系统内存
    参数: 
    返回值: 
    Created By 徐崇 2012.12.18 15:05:37 For Web
==============================================================================*/
uint32_t GetSysMemPrent()
{
	char  buffer[PROCBUFFERLEN];
	int totalmem = 1;
	int MemFree = 0;
	int Buffers = 0;
	int Cached = 0;
    int fd, len;
    char *p;

	fd = open("/proc/meminfo", O_RDONLY);
	if(fd == -1)
	{
		return -1;
	}
	
	len = read(fd, buffer, sizeof(buffer)-1);
	if(len <= 0)
	{
		close(fd);
		return -1;
	}
	close(fd);
	buffer[len] = '\0';

	p = buffer;
	p = skip_token(p);
	totalmem = strtoul(p, &p, 10); /* total memory */

	p = strchr(p, '\n');
	p = skip_token(p);
	MemFree = strtoul(p, &p, 10); /* MemFree memory */

	p = strchr(p, '\n');
	p = skip_token(p);
	Buffers = strtoul(p, &p, 10); /* Buffers memory */

	p = strchr(p, '\n');
	p = skip_token(p);
	Cached = strtoul(p, &p, 10); /* Cached memory */

	//printf("totalmem[%d] MemFree[%d] Buffers[%d]  Cached[%d]\n",totalmem,MemFree,Buffers,Cached);
	
	return (totalmem - MemFree - Buffers - Cached)*100/totalmem;
}

/*==============================================================================
    函数: <GetProcStat>
    功能: 获取进程状态
    参数: 
    返回值: 
    Created By 徐崇 2012.12.18 15:05:27 For Web
==============================================================================*/
int32_t GetProcStat(pid_t pid, ProState *state)
{
    char buffer[PROCBUFFERLEN], *p;
	int fd, len;

	sprintf(buffer, "/proc/%d/stat", pid);

	fd = open(buffer, O_RDONLY);
	if(fd == -1)
	{
		return -1;
	}
	
	len = read(fd, buffer, sizeof(buffer)-1);
	close(fd);
	
	buffer[len] = '\0';
  
    p = buffer;
    p = strchr(p, '(')+1;			/* skip pid */
    {
		char *q = strrchr(p, ')');
		int len = q-p;
		if (len >= sizeof(state->name))
		    len = sizeof(state->name)-1;
		memcpy(state->name, p, len);
		state->name[len] = 0;
		p = q+1;
    }

    p = skip_ws(p);
    switch (*p++)
    {
      case 'R': state->State = 1; break;
      case 'S': state->State = 2; break;
      case 'D': state->State = 3; break;
      case 'Z': state->State = 4; break;
      case 'T': state->State = 5; break;
      case 'W': state->State = 6; break;
    }

    p = skip_token(p);				/* skip ppid */
    p = skip_token(p);				/* skip pgrp */
    p = skip_token(p);				/* skip session */
    p = skip_token(p);				/* skip tty */
    p = skip_token(p);				/* skip tty pgrp */
    p = skip_token(p);				/* skip flags */
    p = skip_token(p);				/* skip min flt */
    p = skip_token(p);				/* skip cmin flt */
    p = skip_token(p);				/* skip maj flt */
    p = skip_token(p);				/* skip cmaj flt */
  
    state->CpuTime  = strtoul(p, &p, 10);		/* utime */
    state->CpuTime += strtoul(p, &p, 10);		/* stime */
    p = skip_token(p);				/* skip cutime */
    p = skip_token(p);				/* skip cstime */

    p = skip_token(p);		/* priority */
    p = skip_token(p);		/* nice */

    p = skip_token(p);				/* skip timeout */
    p = skip_token(p);				/* skip it_real_val */
    p = skip_token(p);				/* skip start_time */

    p = skip_token(p);
    state->Mem = pagetok(strtoul(p, &p, 10));	/* rss */

	state->CpuTime = state->CpuTime*CPUNUM;
	if(state->CpuTime <= 0)
		state->CpuTime = 0;
	return 1;
}

int32_t GetCpuOccupancy(uint32_t ToalTime1,uint32_t ToalTime0,uint32_t ProTime1,uint32_t ProTime0)
{

	int32_t prent =  0;
	if((ToalTime1 <= 0) || (ToalTime0 <= 0) || ProTime1 <= 0 || ProTime0 <= 0)
	{
		return 0;
	}

	//printf("time %d  %d\n",ToalTime1-ToalTime0,ProTime1-ProTime0);
	prent = ((ProTime1 - ProTime0)*100)/(ToalTime1 - ToalTime0);//(102*CPUNUM);//(ToalTime1 - ToalTime0);
	if((prent < 0) || (prent > 101*CPUNUM))
	{
		return -1;
	}
	return prent;
}

/*==============================================================================
    函数: <CreateXmlProc>
    功能: 创建xml进程
    参数: 
    返回值: 
    Created By 徐崇 2012.12.21 10:17:15 For Web
==============================================================================*/
static int32_t CreateXmlProc(xmlNodePtr pnode,Daemon_config *pinfo)
{
	if(NULL == pnode || pinfo == NULL){
		return -1;
	}

	xml_add_new_child(pnode, NULL, BAD_CAST "name", (xmlChar *)pinfo->name);
	xml_add_new_child(pnode, NULL, BAD_CAST "param", (xmlChar *)pinfo->param);
	xml_add_new_child(pnode, NULL, BAD_CAST "active", (xmlChar *)pinfo->active);
	xml_add_new_child(pnode, NULL, BAD_CAST "guardian", (xmlChar *)pinfo->guardian);
	xml_add_new_child(pnode, NULL, BAD_CAST "maxcpu", (xmlChar *)pinfo->maxcpu);
	xml_add_new_child(pnode, NULL, BAD_CAST "maxmem", (xmlChar *)pinfo->maxmem);
	xml_add_new_child(pnode, NULL, BAD_CAST "time", (xmlChar *)pinfo->time);
	xml_add_new_child(pnode, NULL, BAD_CAST "run", (xmlChar *)pinfo->run);
	return 0;
}

/*==============================================================================
    函数: <CreateXmlConfig>
    功能: 创建xml
    参数: 
    返回值: 
    Created By 徐崇 2012.12.21 10:17:04 For Web
==============================================================================*/
static int32_t CreateXmlConfig()
{
	parse_xml_t px;
	xmlNodePtr proc;
	Daemon_config config = {{0}};
	int ret = -1;
	int i = 0;
	
	px.pdoc = xmlNewDoc(BAD_CAST"1.0"); 			//定义文档和节点指针
	if(NULL == px.pdoc)
	{
		zlog_error(pDaemeonOper->c, "xmlNewDoc failed, file: .UoloadConfig\n");
		return ret;
	}
	
	px.proot= xmlNewNode(NULL, BAD_CAST"Daemon");
	xmlDocSetRootElement(px.pdoc, px.proot);		//设置根节点

	r_strcpy(config.active,(int8_t*)"1");
	r_strcpy(config.guardian,(int8_t*)"1");
	r_strcpy(config.maxcpu, (int8_t*)"300");
	r_strcpy(config.maxmem, (int8_t*)"4000000");
	r_strcpy(config.time,  (int8_t*) "60");

	proc = xmlNewNode(NULL, BAD_CAST "proc");
	xmlAddChild(px.proot, proc);
	r_strcpy(config.name,(int8_t*)"/usr/local/reach/control");
	r_strcpy(config.param,(int8_t*)"");
	CreateXmlProc(proc, &config);


	proc = xmlNewNode(NULL, BAD_CAST "proc");
	xmlAddChild(px.proot, proc);
	r_strcpy(config.name,(int8_t*)"/usr/local/reach/ftpserver");
	r_strcpy(config.param,(int8_t*)"");
	CreateXmlProc(proc, &config);

	proc = xmlNewNode(NULL, BAD_CAST "proc");
	xmlAddChild(px.proot, proc);
	r_strcpy(config.name,(int8_t*)"/usr/local/reach/sericalctrl");
	r_strcpy(config.param,(int8_t*)"");
	CreateXmlProc(proc, &config);

	for(i = 0; i < pDaemeonOper->roomnum; i++)
	{
		int8_t roomid[20] = {0};
		proc = xmlNewNode(NULL, BAD_CAST "proc");
		xmlAddChild(px.proot, proc);
		r_strcpy(config.name,(int8_t*)"/usr/local/reach/room");
		sprintf((char *)roomid,"%d",i);
		r_strcpy(config.param,roomid);
		CreateXmlProc(proc, &config);
	}

	ret = xmlSaveFormatFileEnc(XML, px.pdoc, "UTF-8", 1);
	if(-1 == ret)
	{
		zlog_error(pDaemeonOper->c, "xml save params table failed !!!\n");
	}
	else
	{
		ret = 1;
	}

	release_dom_tree(px.pdoc);
	xmlCleanupParser();

	return ret;
}

/*==============================================================================
    函数: <AnalysisXML>
    功能: 解析xml
    参数: 
    返回值: 
    Created By 徐崇 2012.12.18 15:05:15 For Web
==============================================================================*/
int32_t AnalysisXML(DaemonHand *pDaemeonOper)
{
	xmlNodePtr FileNode = NULL;
	xmlNodePtr NodeAttr = NULL;
	xmlChar    *pattr  = NULL;
	uint32_t num = 0;
	uint32_t roomnum = 0;
	if(pDaemeonOper == NULL)
	{
		return -1;
	}
	zlog_category_t *c = pDaemeonOper->c;
	FileNode = pDaemeonOper->px->proot->children;
	
	zlog_error(c,"|             name             |param| maxmem |maxcpu|    run   | guardian |  time | num |");
	while(FileNode != NULL)
	{
		num++;
		NodeAtt *pnodeatt = NULL;
		pnodeatt = (NodeAtt *)r_malloc(sizeof(NodeAtt)); 
		if(pnodeatt == NULL)
		{
			return -1;
		}
		
		r_memset(pnodeatt, 0x0, sizeof(NodeAtt));
		
		NodeAttr = get_children_node(FileNode, BAD_CAST "name");
		if(NodeAttr == NULL)
		{
			r_free(pnodeatt);
			return -1;
		}
		pattr    = xmlNodeListGetString(pDaemeonOper->px->pdoc, NodeAttr->xmlChildrenNode, 1);

		if(pattr == NULL)
		{
			r_strcpy(pnodeatt->param,(int8_t*)"");
		}
		else
		{
			r_strcpy(pnodeatt->procname,(int8_t*)pattr);
			xmlFree(pattr);
			pattr = NULL;
		}
		
		NodeAttr = get_children_node(FileNode, BAD_CAST "param");
		if(NodeAttr == NULL)
		{
			r_free(pnodeatt);
			return -1;
		}
		pattr    = xmlNodeListGetString(pDaemeonOper->px->pdoc, NodeAttr->xmlChildrenNode, 1);	
		if(pattr == NULL)
		{
			r_strcpy(pnodeatt->param,(int8_t*)"");
		}
		else
		{
			r_strcpy(pnodeatt->param,(int8_t*)pattr);
			xmlFree(pattr);
			pattr = NULL;
		}
		

		NodeAttr = get_children_node(FileNode, BAD_CAST "active");
		if(NodeAttr == NULL)
		{
			r_free(pnodeatt);
			return -1;
		}
		pattr    = xmlNodeListGetString(pDaemeonOper->px->pdoc, NodeAttr->xmlChildrenNode, 1);	
		if(pattr == NULL)
		{
			pnodeatt->active = 0;
		}
		else
		{
			if(0 == r_strcmp(pnodeatt->procname,(int8_t*)"/usr/local/reach/room"))
			{
				if(roomnum < pDaemeonOper->roomnum)
				{
					pnodeatt->active = 1;
					roomnum++;
				}
				else
				{
					pnodeatt->active = 0;
				}
			}
			else
			{
				pnodeatt->active = atoi((char*)pattr);
			}
			
			xmlFree(pattr);
			pattr = NULL;
		}



		NodeAttr = get_children_node(FileNode, BAD_CAST "guardian");
		if(NodeAttr == NULL)
		{
			r_free(pnodeatt);
			return -1;
		}
		pattr    = xmlNodeListGetString(pDaemeonOper->px->pdoc, NodeAttr->xmlChildrenNode, 1);	

		if(pattr == NULL)
		{
			pnodeatt->guardian = 0;
		}
		else
		{
			pnodeatt->guardian = atoi((char*)pattr);
			xmlFree(pattr);
			pattr = NULL;
		}
		
		NodeAttr = get_children_node(FileNode, BAD_CAST "maxcpu");
		if(NodeAttr == NULL)
		{
			r_free(pnodeatt);
			return -1;
		}
		pattr    = xmlNodeListGetString(pDaemeonOper->px->pdoc, NodeAttr->xmlChildrenNode, 1);

		if(pattr == NULL)
		{
			pnodeatt->max_cpu = 100;
		}
		else
		{
			pnodeatt->max_cpu = atoi((char*)pattr);
			xmlFree(pattr);
			pattr = NULL;
		}
	
		NodeAttr = get_children_node(FileNode, BAD_CAST "maxmem");
		if(NodeAttr == NULL)
		{
			r_free(pnodeatt);
			return -1;
		}
		pattr    = xmlNodeListGetString(pDaemeonOper->px->pdoc, NodeAttr->xmlChildrenNode, 1);	
		if(pattr == NULL)
		{
			pnodeatt->max_mem = 0;
		}
		else
		{
			pnodeatt->max_mem= atoi((char*)pattr);
			xmlFree(pattr);
			pattr = NULL;
		}

		NodeAttr = get_children_node(FileNode, BAD_CAST "time");
		if(NodeAttr == NULL)
		{
			r_free(pnodeatt);
			return -1;
		}
		pattr    = xmlNodeListGetString(pDaemeonOper->px->pdoc, NodeAttr->xmlChildrenNode, 1);	
		if(pattr == NULL)
		{
			pnodeatt->time = 5;
		}
		else
		{
			pnodeatt->time= atoi((char*)pattr);
			xmlFree(pattr);
			pattr = NULL;
		}
		
		NodeAttr = get_children_node(FileNode, BAD_CAST "run");
		if(NodeAttr == NULL)
		{
			r_free(pnodeatt);
			return -1;
		}
		pattr    = xmlNodeListGetString(pDaemeonOper->px->pdoc, NodeAttr->xmlChildrenNode, 1);	
		if(pattr == NULL)
		{
			r_strcpy(pnodeatt->run,(int8_t*)"");
		}
		else
		{
			r_strcpy(pnodeatt->run,(int8_t*)pattr);
			xmlFree(pattr);
			pattr = NULL;
		}

		if(pnodeatt->active == 1)
		{
		
			list_add_before(&pnodeatt->stlist, pDaemeonOper->phead);

			zlog_error(c,"|%30s|%5s|%8d|%6d|%10s|%10d|%7d|%5d|",pnodeatt->procname,\
			pnodeatt->param,pnodeatt->max_mem,pnodeatt->max_cpu,pnodeatt->run,pnodeatt->guardian,pnodeatt->time,num);
		}
		else
		{
			r_free(pnodeatt);
		}

	
		
		FileNode = FileNode->next;
	}
	
	if(pDaemeonOper->px->pdoc != NULL)
	{
		release_dom_tree(pDaemeonOper->px->pdoc);
		xmlCleanupParser();
		pDaemeonOper->px->pdoc = NULL;
	}


	if(pDaemeonOper->px != NULL)
	{
		r_free(pDaemeonOper->px);
		pDaemeonOper->px = NULL;
	}
	return 1;
}


/*==============================================================================
    函数: <DamemonInit>
    功能: 初始化
    参数: 
    返回值: 
    Created By 徐崇 2012.12.18 15:05:06 For Web
==============================================================================*/
int32_t DamemonInit(DaemonHand **ppDaemeonOper)
{

	int32_t rc;
	int8_t *pnum = NULL;

	if(ppDaemeonOper == NULL)
	{
		return -1;
	}

	
	DaemonHand	*pDaemeonOper = (DaemonHand *)r_malloc(sizeof(DaemonHand));
	if(NULL == pDaemeonOper)
	{
		return -1;
	}

	r_memset(pDaemeonOper, 0x0, sizeof(DaemonHand));
	*ppDaemeonOper = pDaemeonOper;	

	
	rc = zlog_init(ZLOG);
	if (rc)
	{
		printf("init failed\n");
		return -1;
	}
	
	pDaemeonOper->c  = zlog_get_category("Daemon");
	if (!pDaemeonOper->c)
	{
		return -1;
	}


	if(NULL == check_hw_and_get_pri_data((char **)&pnum))
	{
		zlog_error(pDaemeonOper->c, "Check key is fail!!!");
	//	return -1;
	}
	
	pDaemeonOper->roomnum = 1;// atoi((char *)pnum);
	r_free(pnum);

	if((pDaemeonOper->roomnum == 0 )||( pDaemeonOper->roomnum > 16))
	{
		zlog_error(pDaemeonOper->c, "Check key is invalid!!!");
		return -1;
	}
	
	zlog_category_t *c = pDaemeonOper->c;
	parse_xml_t *px = (parse_xml_t*)r_malloc(sizeof(parse_xml_t));
	if(NULL == px)
	{
		zlog_error(c, "FtpServiceInit: malloc parse_xml_t fail");
		return -1;
	}

	zlog_error(c, "Red  Key RoomNum[%d]",pDaemeonOper->roomnum);

	/* 初始化xml */
	r_memset(px,0x0,sizeof(parse_xml_t));
	char* ret = init_file_dom_tree(px, XML);
	if(ret == NULL)
	{
		zlog_error(c, "init_file_dom_tree failed1, xml file: UploadConfig.xml");

		//创建配置文件
		if(1 != CreateXmlConfig())
		{
			return -1;
		}

		char* ret = init_file_dom_tree(px, XML);
		if(ret == NULL)
		{
			zlog_error(c, "init_file_dom_tree failed2, xml file: UploadConfig.xml");
			return -1;
		}
	}

	list_head *pheadnode = (list_head *)r_malloc(sizeof(list_head));
	if(NULL == pheadnode)
	{
		zlog_error(c, "list_head malloc failed");
		return -1;
	}
	
	pDaemeonOper->phead = pheadnode;
	list_init(pDaemeonOper->phead);
	
	pDaemeonOper->px = px;

	DaemonLockInit();
	return 1;
}

/*==============================================================================
    函数: <deDamemonInit>
    功能: 反初始化
    参数: 
    返回值: 
    Created By 徐崇 2012.12.18 15:04:51 For Web
==============================================================================*/
int32_t deDamemonInit(DaemonHand *pDaemeonOper)
{
	
	if(pDaemeonOper != NULL)
	{
		if(pDaemeonOper->px != NULL)
		{
			if(pDaemeonOper->px->pdoc != NULL)
			{
				release_dom_tree(pDaemeonOper->px->pdoc);
				pDaemeonOper->px->pdoc = NULL;
			}

			r_free(pDaemeonOper->px);
			pDaemeonOper->px = NULL;
		}

		if(pDaemeonOper->phead != NULL)
		{
			NodeAtt *pmodeattr = NULL;
			list_head *pcurnode = NULL;
			/* 查找是否存在该设备 */
			list_for_each_item(pcurnode, pDaemeonOper->phead)  
		    {  
		        if(NULL != pcurnode)  
		        {  
		            pmodeattr = list_entry(pcurnode, NodeAtt, stlist);  
		         
					list_remove(pcurnode);
					r_free(pmodeattr);
					pmodeattr = NULL;
					
		        }  
		    }  
			
			r_free(pDaemeonOper->phead);
			pDaemeonOper->phead = NULL;
		}
	
		r_free(pDaemeonOper);
		pDaemeonOper = NULL;
	}

	DaemondeInit();

	xmlCleanupParser();

		printf("3 \n");
	//zlog_fini();
	return 1;
}

/*==============================================================================
    函数: <StartApp>
    功能: 重新拉起进程
    参数: 
    返回值: 
    Created By 徐崇 2012.12.18 15:04:36 For Web
==============================================================================*/
int32_t StartApp(NodeAtt *pdaemon)
{
	pid_t childpid;
	char cmd[1024] = {0};

	if((childpid = fork())== -1) //fork( )命令
	{
		
	}
	else if(childpid == 0) // This is child process!
	{
		pid_t pid;
		int ret  = 0;
		if ((pid = fork()) < 0)     
		{      
			zlog_error(pDaemeonOper->c,"[%s %s] Fork error! [%s]",pdaemon->procname,pdaemon->param,strerror(errno)); 
	 		exit(-1);     
	    }     
	    else if (pid > 0)    
	    {
	        exit(0); 
	    }
		
		setsid();
		umask(0);
		chdir("/usr/local/reach");
		//重新拉起进程
		sprintf(cmd,"%s %s",(char *)pdaemon->procname,(char *)pdaemon->param);
		ret = execl("/bin/sh", "sh", "-c", cmd, (char *)0);
		if(ret == -1)
		{
			zlog_error(pDaemeonOper->c,"[%s %s] execl fail! [%s]",pdaemon->procname,pdaemon->param,strerror(errno)); 
		}
		exit(0);
	}
	else //This is  parent process! 
	{
		
	}

	if (waitpid(childpid, NULL, 0) != childpid) /* wait for first child */    
	{     
		zlog_error(pDaemeonOper->c,"[%s %s] Waitpid error!",pdaemon->procname,pdaemon->param);       
	}    
	return 1;
}


/*==============================================================================
    函数: <CheckPid>
    功能: 检测进程状态 暂不支持同名同参多进程守护
    参数: 
    返回值: 
    Created By 徐崇 2012.12.15 17:20:24 For Web
==============================================================================*/
int32_t CheckPid(NodeAtt *pdaemon)
{
	int32_t i = 0;
	DaemonLock();
	
	//检测进程异常
	if((pdaemon->livestate == 0) && (pdaemon->guardian == 1) && (pdaemon->samenum == 0))
	{
		//拉起进程
		StartApp(pdaemon);
		pdaemon->livestate++;
		zlog_error(pDaemeonOper->c,RED"StartApp name[%s %s]"NONE,pdaemon->procname,pdaemon->param);
	}
	
	for(i = 0; i < pdaemon->samenum; i++)
	{
		
		if(pdaemon->rttotaltimes[i] >= pdaemon->time)
		{	
			uint32_t prent = pdaemon->rttotalcpu[i]/pdaemon->rttotaltimes[i];
			uint32_t mem   = pdaemon->rttotalmem[i]/pdaemon->rttotaltimes[i];
			uint32_t memprent = GetSysMemPrent();
			
			zlog_error(pDaemeonOper->c,"pid[%d] name[%s %s] live[%d] guardian[%d]"\
				" cpu[%d] mem[%u] time[%d] rttime[%d] prent[%d] mem[%u] usemem[%d%%]",\
				pdaemon->rtpid[i],pdaemon->procname,pdaemon->param,pdaemon->livestate,pdaemon->guardian,\
				pdaemon->rtprent[i],pdaemon->rtmem[i],pdaemon->time,pdaemon->rttotaltimes[i],prent,mem,memprent);
			
			//检测内存和cpu状态
			if( ( prent  >= pdaemon->max_cpu)
				|| 
				( ( mem  >= pdaemon->max_mem) && (memprent >= 90) && (memprent <= 100))
		  	  )
			{
				zlog_error(pDaemeonOper->c,RED"Kill pid[%10d] name[%30s %5s] maxmem[%u] maxcpu[%d] rtmem[%u] rtcpu[%d] use[%d%%]"NONE,
				pdaemon->rtpid[i],pdaemon->procname,pdaemon->param,pdaemon->max_mem,pdaemon->max_cpu,mem,prent,memprent);

				//杀死进程						
				if(0 == kill(pdaemon->rtpid[i], SIGKILL))
				{
					pdaemon->rtpid[i]   = 0;
					pdaemon->rtmem[i]   = 0;
					pdaemon->rtprent[i] = 0;

					//拉起进程
					StartApp(pdaemon);
					zlog_error(pDaemeonOper->c,RED"StartApp name[%s %s]"NONE,pdaemon->procname,pdaemon->param);
				}
				else
				{
					zlog_error(pDaemeonOper->c,RED"Don't kill pid[%10d] name[%30s] param[%5s]"NONE,pdaemon->rtpid[i],pdaemon->procname,pdaemon->param);
				}
			}

			pdaemon->rttotaltimes[i] = 0;
			pdaemon->rttotalcpu[i] = 0;
			pdaemon->rttotalmem[i] = 0;
		}
			
	}

	 DaemonunLock();
	 return 1;
}


/*==============================================================================
    函数: <DaemonDealTask>
    功能: 守护进程处理任务
    参数: 
    返回值: 
    Created By 徐崇 2012.12.18 15:08:59 For Web
==============================================================================*/
static void *DaemonDealTask(void *args)
{

	DaemonHand	*pdaemon = (DaemonHand *)args;
	if(NULL == pdaemon)
	{
		return NULL;
	}

	while(1)
	{
		NodeAtt *pmodeattr = NULL;
		list_head *pcurnode = NULL;

		r_sleep(5);
		/* 查找是否存在该设备 */
		list_for_each_item(pcurnode, pdaemon->phead)  
	    {  
	        if(NULL != pcurnode)  
	        {
				pmodeattr = list_entry(pcurnode, NodeAtt, stlist);
				CheckPid(pmodeattr);
				
	        }
		}
	}
}

#if 0
void Signal_Exit(int signal)//退出
{
 FILE *fp;
 time_t t;
 
	if( (fp=fopen("/usr/local/reach/daemon_exut.log","a"))>=0 )
	{
		t = time(0);
		fprintf(fp,"Time is %s of Siganl_Exit !\n",asctime(localtime(&t)));
		fclose(fp);
		exit(0);
	}
	else
	{
		exit(1);
	}
}
#endif


int __main()
{
	//子进程会继续继承父进程文件描述符
	InitDaemon();

	pthread_t thid;
	int ret = 0;
	zlog_category_t *c  = NULL;

	//优先获取cpu
	r_setpriority(PRIO_PROCESS, 0, -20);

	//初始化
	if(-1 == DamemonInit(&pDaemeonOper))
	{
		printf("DamemonInit is faile!!!\n");
		goto FAIL;
	}


	c = pDaemeonOper->c;

	//解析配置文件
	if(-1 == AnalysisXML(pDaemeonOper))
	{
		zlog_error(pDaemeonOper->c, "AnalysisXML is faile!!!\n");
		goto FAIL;
	}

	zlog_info(c,"DamemonInit Done ... Version[%s]",VERSION);

	//处理任务
	ret = r_pthread_create(&thid, NULL, DaemonDealTask, (void *)pDaemeonOper);
	if(ret)
	{
		zlog_error(pDaemeonOper->c, "DealTask failed, err msg: %s\n", strerror(errno));
		goto FAIL;
	}

	//统计实时cpu mem
	while(1)
	{
		NodeAtt *pmodeattr = NULL;
		list_head *pcurnode = NULL;

		/* 查找是否存在该设备 */
		list_for_each_item(pcurnode, pDaemeonOper->phead)  
	    {  
	        if(NULL != pcurnode)  
	        {  
				uint32_t i = 0;
				uint32_t pidlist[1024] = {0};
				
	            pmodeattr = list_entry(pcurnode, NodeAtt, stlist);  
				/* 是否激活此任务监视 */
				if(pmodeattr->active == 1)
				{
					uint32_t num = 0;
					DaemonLock();

					//寻找进程
					num = FindPidByName(pmodeattr,pidlist);
					if(0 == num)
					{
						//初始参数
						pmodeattr->samenum = 0;
						pmodeattr->livestate = 0;
						r_memset(pmodeattr->rtcputime, 0x0, sizeof(uint32_t)*MAXSAMEPROC*7);
						DaemonunLock();
						zlog_debug(c,"FindPidByName Don't [%s %s] pid",pmodeattr->procname, pmodeattr->param);
						continue;
					}

					//同名 同参进程个数
					pmodeattr->samenum = num;
					pmodeattr->livestate = num;
					
					if(pmodeattr->active == 1)
					{
						for(i = 0; i < num ; i++)
						{	
							ProState state = {{0}};
							int32_t ret = 0;
							pmodeattr->rtpid[i] = pidlist[i];
							ret =  GetProcStat((pid_t)pidlist[i], &state);	
							if(ret == -1)
							{	
								pmodeattr->rtmem[i]        = 0;
								pmodeattr->rtcputime[i]    = 0;
								pmodeattr->rtsyscputime[i] = 0;
								zlog_debug(c,"GetProcStat is fail: pid[%d] name[%s]",pidlist[i],pmodeattr->procname);
							}
							else
							{
								int32_t prent = 0;
								uint32_t curcputime = GetSysCpuTime();							
								prent = GetCpuOccupancy(curcputime,pmodeattr->rtsyscputime[i],state.CpuTime, pmodeattr->rtcputime[i]);
								if(-1 == prent)
								{
									zlog_debug(c,"GetCpuOccupancy is error pid[%d] name[%s]",pidlist[i],pmodeattr->procname);
									prent = 0;
								}
						
								pmodeattr->rtprent[i] = prent;
								zlog_debug(c,"name[%s %s] cpu[%d] mem[%d]",pmodeattr->procname,pmodeattr->param,prent,pmodeattr->rtmem[i]);			
								pmodeattr->rtsyscputime[i] = curcputime;
								pmodeattr->rtcputime[i]    = state.CpuTime;
								pmodeattr->rtmem[i] 	   = state.Mem;
								pmodeattr->rttotalcpu[i]  += prent;
								pmodeattr->rttotalmem[i]  += state.Mem;
  								pmodeattr->rttotaltimes[i]++;
							}
							
						}
					}
					DaemonunLock();
				}
			}  
	    }  
		r_sleep(1);		
	}

	
FAIL:
	printf("deDamemonInit Done ...\n");
	deDamemonInit(pDaemeonOper);
	return 1;
}


int main()
{

	//printf("%d\n",GetSysTotalMem());
	__main();

	return 1;
}

