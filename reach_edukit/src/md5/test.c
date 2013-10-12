#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>  
#include <assert.h>
#include "string.h"
#include "reach_os.h"
#include "md5lib.h"

#include "dirmd5.h"
#if 0

#define MAX_XML_LEN 1024
int32_t GetFileList(int8_t *RepairPath, int32_t depth)
{
	if(NULL == RepairPath)
	{
		return -1;
	}

	DIR              *pDir ;
	struct dirent    *ent;
	int32_t      num = 0;
	int8_t       childpath[MAX_XML_LEN];
	int32_t     ret = 0;


	pDir = opendir(RepairPath);
	if(NULL == pDir)
	{
		printf("Cannot open directory: %s\n",RepairPath);
		return -1;
	}
	
	while((ent = readdir(pDir))!=NULL)
	{
		if(ent->d_type & DT_DIR)
		{
			int8_t md5info[512*2] = {0};
			if((r_strcmp((int8_t *)ent->d_name,(int8_t*)".")==0) || (r_strcmp((int8_t *)ent->d_name,(int8_t*)"..")==0)\
			        || (0 == r_strcmp((int8_t *)ent->d_name,(int8_t*)"recovery")))
				continue;
				
			sprintf((char *)childpath,"%s/%s",(char *)RepairPath,(char *)ent->d_name);
			sprintf((char *)md5info,"%s/md5.info",childpath);
			if(0 == access((char *)md5info,0))
			{
				//package_add_xml_leaf(node, MSG_FILE_KEY, &childpath[r_strlen((int8_t*)MP4_RECORD_PATH)+1]);

				printf("%s\n",childpath);
				num++;
				if(num >= depth)
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
	
	return num;
}

int main(int arg,char **args)
{	
	//GetFileList("/opt/Rec",5);
		//DirDetectionFile(".");

	char*p =  MDFile (args[1]);
	printf("%s %d\n",p,sizeof(unsigned long int));
}

#endif
