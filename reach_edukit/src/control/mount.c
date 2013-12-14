#if 1
#include "mount.h"
list_head *pheadnode = NULL;

extern server_set *gpser;
extern SerialHandle *pserial_handle;

extern int YinCodeShowString(int fd,char *string,int x, int y);


int GetDiskState(char *name, unsigned  long *totalsize, unsigned  long *freesize)
{

	list_head *pcurnode = NULL;
	DevAtt *pdevattr = NULL;

	/* 上报消息 */
	list_for_each_item(pcurnode, pheadnode)
	{
        if(NULL != pcurnode)
        {
            pdevattr = list_entry(pcurnode, DevAtt, stlist);
			if(0 == strcmp("",pdevattr->devname))
			{
				*totalsize = pdevattr->total_size;
				*freesize  = pdevattr->free_size;
				//printf("devname [%s] total[%lld] free[%lld] percent[%d%%]\n",\
				pdevattr->devname,pdevattr->total_size,pdevattr->free_size,pdevattr->percent);
				return 1;
			}

		}
    }

	return 0;
}

int GetUsbDevState(int *num, MNUsbDeb *stat, int max)
{

	list_head *pcurnode = NULL;
	DevAtt *pdevattr = NULL;
	int i = 0;
	if(num == NULL || stat == NULL)
	{
		return 0;
	}
	printf("-------------->pcurnode%p\n",pheadnode);
	/* 上报消息 */
	list_for_each_item(pcurnode, pheadnode)
	{
	printf("-------------->pcurnode%p\n",pcurnode);
        if(NULL != pcurnode)
        {
            pdevattr = list_entry(pcurnode, DevAtt, stlist);
		printf("-------------->drv=%s\n",pdevattr->devname);


			//是usb设备
			if(0 == strncmp(pdevattr->devname,"/media/s",strlen("/media/s")))
			{
				if(i >= max)
				{
					break;
				}
				strcpy(stat[i].devname, pdevattr->devname);
				stat[i].total_size = pdevattr->total_size;
				stat[i].free_size  = pdevattr->free_size;
				printf("devname [%s] total[%lld] free[%lld] percent[%d%%]\n",\
				pdevattr->devname,pdevattr->total_size,pdevattr->free_size,pdevattr->percent);
				i++;
			}

		}
    }
	*num = i;

	return 0;
}


static int detection(char *p, list_head *pnode)
{

//	char *pstart = NULL;
//	char tmp[20];
	char Mdev[100] = {0};
	char Mname[100] = {0};
	DevAtt *pdevatt = NULL;
	DevAtt *ptmpdevatt = NULL;
	list_head *pcurnode = NULL;
	int DevFound  = 0;
//	int len = 0;
    struct    statfs    stat;
	bzero(&stat,sizeof(struct statfs));
	if(NULL == p)
	{
		return 0;
	}

	#if 0
	//TO DO 针对磁盘状态修改
	/* 获取设备名称 */
	//pstart = strstr(p,"/media/sd");
	pstart = strstr(p,"/boot");
	if(pstart == NULL)
	{
		return 1;
	}
	strncpy(tmp,pstart,11);
	tmp[11]='\0';
	pstart = pstart + 11;
	#endif

	//TO DO 针对磁盘状态修改
	/* 获取设备名称 */
	//pstart = strstr(p,"/media/sd");
	#if 0
	pstart = strstr(p,PARTITION);
	if(pstart == NULL)
	{
		return 1;
	}
	#endif

	while(1)
	{

		sscanf(p,"%s%s",Mdev,Mname);
		p = strchr(p, '\n');
		if(p == NULL)
		{
			return 1;
		}
        p++;

		//printf("Mdev[%s] Mname[%s]\n",Mdev,Mname);
		if(0 == strncmp(Mdev,"/dev/s",strlen("/dev/s")))
		{
			if(0 == strncmp(Mname,"/opt/Rec",strlen("/opt/Rec")))
			{
				//printf("----->Mdev[%s] Mname[%s]\n",Mdev,Mname);
				break;
			}
			else if(0 == strncmp(Mname,"/media/s",strlen("/media/s")))
			{
				break;
			}
		}
	}

	#if 0
	len = strlen(PARTITION);
	strncpy(tmp,PARTITION,len);
	tmp[len]='\0';
	pstart = pstart + len;

	#endif

	/* 查找是否存在该设备 */
	list_for_each_item(pcurnode, pnode)
    {
        if(NULL != pcurnode)
        {
            pdevatt = list_entry(pcurnode, DevAtt, stlist);
			if(0 == strcmp(pdevatt->devname, Mname))
			{
				ptmpdevatt = pdevatt;
				DevFound++;
			}
        }
    }

	/* 不存在则新添加节点 */
	if(DevFound == 0)
	{
		pdevatt = (DevAtt *)malloc(sizeof(DevAtt));
		strcpy(pdevatt->devname,Mname);
		pdevatt->IsExist = 1;
		pdevatt->neednotice = 1;
		list_add_before(&pdevatt->stlist, pnode);
		//printf("add node %s \n",pdevatt->devname);
	}
	else
	/* 存在则重新赋值 */
	{
		pdevatt	= ptmpdevatt;
		strcpy(pdevatt->devname,Mname);
		pdevatt->IsExist = 1;
		pdevatt->neednotice = 0;
	}

	if(-1 != statfs(Mname,&stat))
	{
		pdevatt->total_size = (uint64_t)((uint64_t)stat.f_blocks*(uint64_t)stat.f_bsize)/1024;//(stat.f_blocks - stat.f_bfree);
		pdevatt->free_size = (uint64_t)((uint64_t)stat.f_bavail*(uint64_t)stat.f_bsize)/1024;
		pdevatt->percent    = (stat.f_blocks - stat.f_bfree ) * 100 / (stat.f_blocks - stat.f_bfree + stat.f_bavail) + 1;
	}
	else
	{
		//printf("%s\n",tmp);
		pdevatt->total_size = 0;
		pdevatt->free_size  = 0;
		pdevatt->percent    = 0;
	}


	detection(p,pnode);
	return 1;
}

static DevAtt s_dev_info;
int disk_get_usb_devname(char* mount_path)
{
	DevAtt dev;
	if(NULL!=mount_path && 1 == s_dev_info.IsExist){
		memcpy(mount_path,s_dev_info.devname,sizeof(s_dev_info.devname));
		printf("mount_path=%s,%s!\n",mount_path,s_dev_info.devname);
		return 0;
	}
	return -1;
}

static void *DiskDetection(void *args)
{

	int file = -1;
	char tmp[FILE_SIZE]={0};
	list_head *pcurnode = NULL;
	DevAtt *pdevattr = NULL;
	DevAtt temp_dev;
	
	file = open("/proc/mounts", O_RDONLY, 0);
	if(-1 == file)
	{
		printf("open fail!!!\n");
		assert(0);
	}

	pheadnode = (list_head *)malloc(sizeof(list_head));
	if(NULL == pheadnode)
	{
		assert(0);
	}
	printf("pheadnode=%p...........\n",pheadnode);
	list_init(pheadnode);
	while(1)
    {
		
		int cur_free=0;
		int max_free=0;
		unsigned int len = 0;
		lseek(file,0,0);
		len = read(file,tmp,FILE_SIZE-1);
		detection(tmp,pheadnode);

		/* 查找是否存在该设备 */
		list_for_each_item(pcurnode, pheadnode)
	    {
	        if(NULL != pcurnode)
	        {
	            pdevattr = list_entry(pcurnode, DevAtt, stlist);

				if(0 == pdevattr->IsExist)
				{
					printf("del node %s\n",pdevattr->devname);

					if(0 == strncmp(pdevattr->devname,"/opt/Rec",strlen("/opt/Rec")))
					{
						//sata设备
						if(gpser != NULL)
						{
							r_pthread_mutex_lock(&gpser->pserinfo->info_m);
							gpser->pserinfo->SysInfo.DiskMaxSpace = 0;
							gpser->pserinfo->SysInfo.DiskAvailableSpace = 0;
							r_pthread_mutex_unlock(&gpser->pserinfo->info_m);

							char cmd[256] = {0};
							sprintf(cmd,"  Warning: Sata Dev Umount  ");
							pserial_handle->LedState = MENUINFO;
							pserial_handle->LedFLUSH = 1;
							//YinCodeShowString(pserial_handle->ledfd,cmd, 5, 30);
						}
					}
					else
					{
						if(pserial_handle != NULL)
						{
							char cmd[256] = {0};
							sprintf(cmd,"    Unmount usb dev    ");
							pserial_handle->LedState = MENUINFO;
							pserial_handle->LedFLUSH = 1;
							//YinCodeShowString(pserial_handle->ledfd,cmd, 5, 30);
							memset(&pserial_handle->UsdStat,0, sizeof(UsbDev));
						}
					}

					list_remove(pcurnode);
					free(pdevattr);
					pdevattr = NULL;
				}
	        }
	    }
	//	memset(&s_dev_info,0,sizeof(DevAtt));
		s_dev_info.IsExist=0;
		/* 上报消息 */
		list_for_each_item(pcurnode, pheadnode)
		{
	        if(NULL != pcurnode)
	        {
	            pdevattr = list_entry(pcurnode, DevAtt, stlist);
				//if(pdevattr->neednotice == 1)
				{
					if(0 == strncmp(pdevattr->devname,"/opt/Rec",strlen("/opt/Rec")))
					{
						//sata设备
						if(gpser != NULL)
						{
							r_pthread_mutex_lock(&gpser->pserinfo->info_m);
							gpser->pserinfo->SysInfo.DiskMaxSpace = pdevattr->total_size;
							gpser->pserinfo->SysInfo.DiskAvailableSpace = pdevattr->free_size;
							r_pthread_mutex_unlock(&gpser->pserinfo->info_m);
						}
					}
					else
					{
						if(pserial_handle != NULL)
						{
							if(pserial_handle->UsdStat.IsUsbDev == 0)
							{
								char cmd[256] = {0};
								pserial_handle->UsdStat.IsUsbDev = 1;
								//这里只是发现USB设备，不做其他处理
								sprintf(cmd,"    Found usb dev plugin...   ");
								pserial_handle->LedState = MENUINFO;
								pserial_handle->LedFLUSH = 1;
							//	YinCodeShowString(pserial_handle->ledfd,cmd, 5, 30);
							}
							//YinCodeShowString(pserial_handle->ledfd,"U", 60, 0);
						}
						//usb设备
					}
					if(0 ==cur_free){
						if( 0 == strncmp(pdevattr->devname,"/media",strlen("/media"))){
							memcpy(&s_dev_info,pdevattr,sizeof(DevAtt));
							cur_free=1;
							s_dev_info.IsExist =1;
						}
					}
//					max_free= max_free>cur_free?max_free:cur_free;
				//	printf("control: devname [%s] total[%lld] free[%lld] percent[%d%%] \n",
				//			pdevattr->devname,pdevattr->total_size,pdevattr->free_size,pdevattr->percent);
				}
				pdevattr->IsExist = 0;
				pdevattr->neednotice = 0;
			}
	    }
		memset(tmp,0,2000);
		sleep(3);

	}

	free(pheadnode);
	pheadnode = NULL;
	close(file);
}

int RegDiskDetectMoudle()
{

	int ret = -1;
	pthread_attr_t		attr;
	pthread_t			pthid;

	ret = pthread_attr_init(&attr);
	if(ret != 0)
	{
		fprintf(stderr, "start_task pthread_attr_init failed, errmsg = %s\n", strerror(errno));
		return -1;
	}
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&pthid, &attr, DiskDetection, NULL);
	if(ret != 0)
	{
		fprintf(stderr, "start_task pthread_create failed, ret = %d, errmsg = %s\n", ret, strerror(errno));

	}

	pthread_attr_destroy(&attr);

	return ret;
}

#endif
