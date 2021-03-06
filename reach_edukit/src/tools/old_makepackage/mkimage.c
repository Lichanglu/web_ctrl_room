/*-------------------------------------------------------------------------
          Copyright (C), 2012-2013, Reach Tech. Co., Ltd.
          File name:
          Author: 徐崇      Version:        Date: 2012.10.12 10:31:01
	      Description:
	      Function List:
	      History:
		    1. Date:
  			   Author:
  			   Modification:
-------------------------------------------------------------------------*/
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
#define  MAX_FILE_PATH_LEN      (2*1024)
#define VERIFYLEN        (4*1024)

static int8_t *basename(int8_t *path)
{
  /* Ignore all the details above for now and make a quick and simple
     implementaion here */
  char *s1;
  char *s2;

  s1=strrchr(path, '/');
  s2=strrchr(path, '\\');

  if(s1 && s2) {
    path = (s1 > s2? s1 : s2)+1;
  }
  else if(s1)
    path = s1 + 1;
  else if(s2)
    path = s2 + 1;

  return path;
}

/*==============================================================================
    函数: <ScnDir>
    功能: <xh_Func:>
    参数:
    Created By 徐崇 2012.10.17 10:39:26 For Ftp
==============================================================================*/
static int ScnDir(FILE *plistfile,int8_t *DirPath)
{
	DIR              *pDir ;
	struct dirent    *ent;
	int32_t      i = 0;
	char    childpath[MAX_FILE_PATH_LEN];
	
	int32_t     ret = 0;
	int32_t     num = 0;
	pDir = opendir(DirPath);
	if(NULL == pDir)
	{
		printf("Cannot open directory: %s",DirPath);
		return -1;
	}

	while((ent = readdir(pDir))!=NULL)
	{
		if(ent->d_type & DT_DIR)
		{
		
			if(r_strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)
				continue;
				
			sprintf(childpath,"%s/%s",DirPath,ent->d_name);
			ret = ScnDir(plistfile, childpath);
			num = ret + num;
		}
		else
		{
			int8_t LocalFilePath[MAX_FILE_PATH_LEN];
			sprintf(LocalFilePath,"%s/",DirPath);
			r_strncat(LocalFilePath, ent->d_name, r_strlen(ent->d_name));
			printf("%s\n",LocalFilePath);
			
			/* 隐藏文件过滤 */
			if(ent->d_name[0] == '.')
			{
				continue;
			}
			num++;
			fprintf(plistfile,"  <file>%s</file>\n",LocalFilePath);
			
		}
	}
	
	if(0 != closedir(pDir))
	{
		assert(0);
	}
	return num;
	
}

/*==============================================================================
    函数: <UpLoadFile>
    功能: <xh_Func:>
    参数:
    Created By 徐崇 2012.10.16 16:24:27 For Ftp
==============================================================================*/
int32_t  DirDetectFile(int8_t *UpLoadFilePath)
{

	int8_t filename[MAX_FILE_PATH_LEN];
	FILE *pmd5file = NULL;
	FILE *plistfile = NULL;
	int32_t  ret = 0;
	if((UpLoadFilePath == NULL))
	{
		return -1;
	}

	r_strcpy(filename,UpLoadFilePath);
	r_strncat(filename,"/filelist.xml",r_strlen("filelist.xml")+1);
	
	plistfile = fopen(filename,"w+");
	if(NULL == plistfile)
	{
		return -1;
	}

	fprintf(plistfile,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(plistfile,"<DirContent>\n");

	

	ret = ScnDir(plistfile, UpLoadFilePath);
	fprintf(plistfile,"	<num>%d</num>\n",ret);
	fprintf(plistfile,"</DirContent>\n");
	
	
	if(plistfile != NULL)
	{
		fclose(plistfile);
	}
	
	return ret;
}

void main(int arg,char **args)
{
	DIR* ret = NULL;
	int8_t  cmd[200] = { 0 };
	if(arg != 2)
	{
		printf("example: mkupImage [dir].. \n");
		return ;
	}

	ret = opendir(args[1]);
	if(NULL == ret)
	{
		printf("Cannot open directory: %s\n",args[1]);
		return;
	}
	closedir(ret);
	if(0 >= DirDetectFile(args[1]))
	{
		printf("Cannot find file or find failed!!\n");
		return ;
	}

	sprintf(cmd, "tar -cvf ./update.tar %s",args[1]);
	if(0 != system(cmd))
	{
		printf("tar update.tar is failed!!!\n");
		return ;
	}
	
	/* 计算哈希值 */
	int8_t *pHash = "0";
				
	int8_t buffer[8*1024] = {0};
	FILE *tarfile;
	int8_t *pbuffer = buffer;
		
	tarfile = fopen("update.tar","r");
	if(tarfile != NULL)
	{
		//hash写0		
		if(VERIFYLEN == fread(pbuffer,1, VERIFYLEN, tarfile))
		{
			if(0 == fseek(tarfile,-(VERIFYLEN),SEEK_END))
		 	{
				if(VERIFYLEN == fread(pbuffer+VERIFYLEN, 1, VERIFYLEN, tarfile))
				{
					pHash = MDString(pbuffer,VERIFYLEN*2);
				}
			}
		}		
	}
	else
	{
		printf("open update.tar is fail!!!\n");
		return ;
	}
				
	FILE *file = fopen("hash","w+");
	if(file == NULL)
	{
		printf("create hash is fail!!\n");
		return ;
	}
	printf("hash [ %s ]\n",pHash);
	
	fprintf(file,"%s",pHash);
	fclose(file);

	/* 添加hash值文件 */
	sprintf(cmd,"tar -rvf update.tar hash");
	if(0 != system(cmd))
	{
		printf("tar -rvf update.tar hash is failed!!\n");
		system("rm -f update.tar hash");
		return;
	}

	/* 压缩文件 */
	sprintf(cmd,"gzip ./update.tar -f");
	if(0 != system(cmd))
	{
		printf("gzip update.tar is failed!!\n");
		system("rm -f update.tar hash");
		return;
	}
	system("rm -f hash");
	printf("create upgrade file update.tar.gz is success!!\n");

}

