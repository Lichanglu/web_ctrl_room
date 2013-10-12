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

#include "dirmd5.h"


static int8_t *basename(int8_t *path)
{
  /* Ignore all the details above for now and make a quick and simple
     implementaion here */
  int8_t *s1;
  int8_t *s2;

  s1=(int8_t *)strrchr((char *)path, '/');
  s2=(int8_t *)strrchr((char *)path, '\\');

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
static int ScnDirMd5(FILE *pfile ,FILE *plistfile,int8_t *DirPath, int8_t *RomteFilePath , int32_t DirLen)
{
	DIR              *pDir ;
	struct dirent    *ent;
	int8_t		childpath[MAX_FILE_PATH_LEN];
	int8_t		newfilename[MAX_FILE_PATH_LEN];
	int8_t		tmpfilename[MAX_FILE_PATH_LEN];

//	int32_t		i = 0;
	int32_t     ret = 1;
	pDir = opendir((char *)DirPath);
	if(NULL == pDir)
	{
		printf("Cannot open directory: %s",DirPath);
		return -1;
	}
	
	while((ent = readdir(pDir))!=NULL)
	{
		if(ent->d_type & DT_DIR)
		{
		
			if(r_strcmp((int8_t *)ent->d_name,(int8_t *)".")==0 || r_strcmp((int8_t *)ent->d_name,(int8_t *)"..")==0)
				continue;
				
			sprintf((char *)childpath,"%s/%s",DirPath,ent->d_name);
			
			ret = ScnDirMd5(pfile, plistfile, childpath,RomteFilePath,DirLen);
			
		}
		else
		{
		
			int8_t LocalFilePath[MAX_FILE_PATH_LEN];
			int8_t UpFilePath[MAX_FILE_PATH_LEN] = {0};
			r_strcpy(UpFilePath,RomteFilePath);;

			sprintf((char *)LocalFilePath,"%s/",DirPath);
			sprintf((char *)UpFilePath,"%s",UpFilePath);
			r_strncat(LocalFilePath, (int8_t *)ent->d_name, r_strlen((int8_t *)ent->d_name));

			r_strcpy(UpFilePath + r_strlen(UpFilePath), LocalFilePath + DirLen);
			
			/* 隐藏文件过滤 */
			if(ent->d_name[0] == '.')
			{
				continue;
			}

			if(0 == r_strcmp((int8_t *)ent->d_name,(int8_t *)"md5.info"))
			{
				continue;
			}

			if(0 == r_strcmp((int8_t *)ent->d_name,(int8_t *)"download.info"))
			{
				continue;
			}
		
			int8_t *pHash = (int8_t *)"0";
			int8_t *pIsmp4 = r_strstr((int8_t *)ent->d_name,(int8_t *)".mp4");
			if(pIsmp4 != NULL)
			{
				int8_t mp4[8*1024] = {0};
				FILE *pmp4file;
				int8_t *pmp4 = mp4;
				
				pmp4file = r_fopen(LocalFilePath,(int8_t *)"r");
				if(pmp4file != NULL)
				{
					//hash写0		
					if(MP4_READ_LEN == fread(pmp4,1, MP4_READ_LEN, pmp4file))
					{
						if(0 == fseek(pmp4file,-(MP4_READ_LEN),SEEK_END))
					 	{
							if(MP4_READ_LEN == fread(pmp4+MP4_READ_LEN, 1, MP4_READ_LEN, pmp4file))
							{
								pHash = (int8_t *)MDString((char *)pmp4,MP4_READ_LEN*2);
							}
						}
					}	
					fclose(pmp4file);
				}
			}
			else
			{
				pHash = (int8_t *)MDFile((char *)LocalFilePath);
			}
			int8_t *p = (int8_t*)strchr((char *)UpFilePath, '/');
			p++;
		    fprintf(pfile,"  <file md5=\"%s\">%s</file>\n",pHash,p);
		//	fprintf(plistfile,"  <file>%s</file>\n",UpFilePath);	
		//	printf("%s\n",ent->d_name);

			if(pIsmp4 != NULL){
				int8_t cmd[256] = {0};
				
				memset(newfilename, 0, MAX_FILE_PATH_LEN);
				memset(tmpfilename, 0, MAX_FILE_PATH_LEN);
				
				char *pnew = NULL;
 				char *ptp = NULL;
 
				strcpy((char *)tmpfilename, (char *)p);
				pnew = (char *)tmpfilename;
 				ptp = strsep(&pnew,"/");

				fprintf(stderr, "tmpfilename: %s\n", tmpfilename);
				fprintf(stderr, "pnew: %s\n", pnew);
				fprintf(stderr, "path: %s\n", DirPath);
				
				if(!strcmp(ptp, "HD")){
					memset(tmpfilename, 0, MAX_FILE_PATH_LEN);
					strcpy((char *)tmpfilename, (char *)DirPath);
					char *ptmpfile = (char *)tmpfilename;
 					char *pend = (char *)strstr(ptmpfile,"/HD/");
					if(pend){
						tmpfilename[pend-ptmpfile] = '\0';
						fprintf(stderr, "tmpfilename: %s\n", tmpfilename);
						sprintf((char *)newfilename,"%s/HD_%s", tmpfilename, ent->d_name);
					}
					sprintf((char *)cmd,"/HD_%s",(int8_t *)ent->d_name);
				}
				else{
					memset(tmpfilename, 0, MAX_FILE_PATH_LEN);
					strcpy((char *)tmpfilename, (char *)DirPath);
					char *ptmpfile = (char *)tmpfilename;
 					char *pend = (char *)strstr(ptmpfile,"/SD/");
					if(pend){
						tmpfilename[pend-ptmpfile] = '\0';
						fprintf(stderr, "tmpfilename: %s\n", tmpfilename);
						sprintf((char *)newfilename,"%s/SD_%s", tmpfilename, ent->d_name);
					}
					sprintf((char *)cmd,"/SD_%s",(int8_t *)ent->d_name);
				}
				
				symlink((char *)LocalFilePath, (char *)newfilename);

				fprintf(stderr, "LocalFilePath: %s\n", LocalFilePath);
				fprintf(stderr, "UpFilePath: %s\n", UpFilePath);
				fprintf(stderr, "path: %s\n", DirPath);
				fprintf(stderr, "newfilename: %s\n", newfilename);
				
				fprintf(plistfile,"  <file md5=\"%s\">%s</file>\n",pHash,cmd);
			}
		}
	}
	
	if(0 != closedir(pDir))
	{
		assert(0);
	}
	return ret;
	
}

/*==============================================================================
    函数: <UpLoadFile>
    功能: <xh_Func:>
    参数:
    Created By 徐崇 2012.10.16 16:24:27 For Ftp
==============================================================================*/
int32_t  DirDetectionFile(int8_t *UpLoadFilePath)
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
	r_strncat(filename,(int8_t *)"/md5.info",r_strlen((int8_t *)"md5.info")+1);
	
	pmd5file = r_fopen(filename,(int8_t *)"w+");
	if(NULL == pmd5file)
	{
		return -1;
	}

#if 1
	r_strcpy(filename,UpLoadFilePath);
	r_strncat(filename,(const int8_t *)"/download.info",r_strlen((const int8_t *)"download.info")+1);
	
	plistfile = r_fopen(filename,(int8_t *)"w+");
	if(NULL == plistfile)
	{
		return -1;
	}
#endif
	
	fprintf(pmd5file,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(pmd5file,"<md5info>\n");
	
	fprintf(plistfile,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(plistfile,"<md5info>\n");

	
	int Len = r_strlen(UpLoadFilePath) - r_strlen(basename(UpLoadFilePath));
	ret = ScnDirMd5(pmd5file, plistfile, UpLoadFilePath, (int8_t*)"", Len);

#if 1
	fprintf(plistfile,"</md5info>\n");
	if(plistfile != NULL)
	{
		fclose(plistfile);
	}
#endif
	
	//	char *pHash = MDFile(filename);
				
  //  fprintf(pmd5file,"  <file md5=\"%s\">%s</file>\n",pHash,filename);
				
	fprintf(pmd5file,"</md5info>\n");
	

	if(pmd5file != NULL)
	{
		fclose(pmd5file);
	}

	return ret;
}
#if 0
void main(int arg,char **args)
{
	DirDetectionFile("/home/reach/xuchong/root/room0");
}
#endif
