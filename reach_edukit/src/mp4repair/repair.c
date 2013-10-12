/*-------------------------------------------------------------------------
          Copyright (C), 2012-2013, Reach Tech. Co., Ltd.   
          File name:              
          Author: 徐崇      Version:        Date: 2012.11.13 19:01:39
	      Description:           
	      Function List:         
	      History:               
		    1. Date: 
  			   Author: 
  			   Modification:
-------------------------------------------------------------------------*/
#include "repair.h"
#include "zlog.h"

int8_t XML_PATH[][DIRLEN]=
{
	{"/SD/resource/index.xml"},
	{"/SD/resource/info.xml"},
	{"/HD/resource/index.xml"},
	{"/HD/resource/info.xml"}
};

zlog_category_t *c = NULL;

static void record_stop_msg(int8_t *reqbuf,int8_t *recordid ,int8_t *course_root_dir)
{
	
	r_sprintf((reqbuf + MSG_HEAD_LEN), \
	          "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"\
	          "<RequestMsg>\n"\
	          "\t<MsgHead>\n"\
	          "\t\t<MsgCode>36001</MsgCode>\n"\
	          "\t\t<PassKey>Mp4Repair</PassKey>\n"\
	          "\t</MsgHead>\n"\
	          "\t<MsgBody>\n"\
	          "\t\t<RecReport>\n"\
	          "\t\t\t<RecordID>%s</RecordID>\n"\
	          "\t\t\t<OptType>0</OptType>\n"\
	          "\t\t\t<Result>1</Result>\n"\
	          "\t\t\t<FileName>%s</FileName>\n"\
	          "\t\t\t<PassKey>AllPlatform</PassKey>\n"\
	          "\t\t</RecReport>\n"\
	          "\t</MsgBody>\n"\
	          "</RequestMsg>\n",\
	          recordid,course_root_dir);
}
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

static void info2index(FILE *index_fp, FILE *info_fp)
{
	char line[256] = {0};
	char *p = NULL;
	int width = 0;
	int height = 0;
	int starttime = 0;
	int time = 0;
	char filename[128] = {0};
	char image[128] = {0};
	int ret = 0;
	xml_head(index_fp);
	indexs_head_node(index_fp);
	fflush(index_fp);

	while(1) {
		if(NULL == info_fp) {
			break;
		}

		ret = fscanf(info_fp, FILE_NODE_FORMAT, &width, &height, &starttime, filename);
		p = strstr(filename, "</file>");

		if(NULL == p) {
			break;
		}

		strncpy(image, filename, p - filename);

		p = fgets(line, 255, info_fp);

		if(NULL == p) {
			break;
		}

		time = starttime;
		index_node(index_fp, time, image);
		fflush(index_fp);

	}

	indexs_end_node(index_fp);
}

static void file_complete_copy(FILE *fp1, FILE *fp2, int8_t *strkey)
{
	char line[256] = {0};
	char *p = NULL;

	while(1) {
		p = fgets(line, 255, fp2);

		if(NULL == p) {
			break;
		}
		p = strstr(line, strkey);

		if(NULL == p) {
			break;
		}
		fprintf(fp1, "\t\t\t%s", line);
		fflush(fp1);
	}
}


static int8_t *hd2sd(int8_t *sd_dir, int8_t *hd_dir)
{
	r_strcpy(sd_dir, hd_dir);
	int8_t *p = r_strstr(sd_dir, "/HD/");

	if(NULL == p) {
		return NULL;
	}

	r_memcpy(p, "/SD/", 4);
	return sd_dir;
}


int32_t create_course_record_xml(int8_t *course_root_dir, int32_t course_record_totaltime, stream_type_sindex_sum_t *s)
{
	printf( "start...");

	if(NULL == course_root_dir || NULL == s) {
		printf(  "param is error!");
		return -1;
	}

	int32_t sindex = 0;
	int32_t hd_sindex = 1;
	int32_t sd_sindex = 1;
	int32_t hd_sindex_sum = s->hd_sindex_sum;
	int32_t sd_sindex_sum = s->sd_sindex_sum;
	int32_t jpeg_sindex_sum = s->jpeg_sindex_sum;
	FILE *hd_info_xml_fp = NULL;
	FILE *sd_info_xml_fp = NULL;
	FILE *hd_index_xml_fp = NULL;
	FILE *sd_index_xml_fp = NULL;
	int8_t hd_info_xml[1024] = {0};
	int8_t sd_info_xml[1024] = {0};
	int8_t hd_index_xml[1024] = {0};
	int8_t sd_index_xml[1024] = {0};

	int8_t hd_info_xml_tmp[1024] = {0};
	int8_t sd_info_xml_tmp[1024] = {0};
	int8_t hd_index_xml_tmp[1024] = {0};
	int8_t sd_index_xml_tmp[1024] = {0};

	FILE *info_xml_x_fp = NULL;
	int8_t info_xml_x[1024] = {0};
	FILE *blue_x_fp = NULL;
	int8_t blue_x[1024] = {0};

	r_sprintf(hd_info_xml_tmp, "%s/HD/%s.tmp", course_root_dir, INFO_XML);

	if((hd_info_xml_fp = fopen(hd_info_xml_tmp, "w")) == NULL) {
		printf(  "fopen  : %s", strerror(errno));
		return -1;
	}

	hd2sd(sd_info_xml_tmp, hd_info_xml_tmp);

	if((sd_info_xml_fp = fopen(sd_info_xml_tmp, "w")) == NULL) {
		printf( "fopen  : %s", strerror(errno));
		return -1;
	}

	r_sprintf(hd_index_xml_tmp, "%s/HD/%s.tmp", course_root_dir, INDEX_XML);

	if((hd_index_xml_fp = fopen(hd_index_xml_tmp, "w")) == NULL) {
		printf(  "fopen  : %s", strerror(errno));
		return -1;
	}

	hd2sd(sd_index_xml_tmp, hd_index_xml_tmp);

	if((sd_index_xml_fp = fopen(sd_index_xml_tmp, "w")) == NULL) {
		printf(  "fopen  : %s", strerror(errno));
		return -1;
	}

	for(sindex = 0; sindex < s->hd_sindex_sum; sindex ++) {
		r_sprintf(info_xml_x, "%s/HD/%s.%d", course_root_dir, INFO_XML, sindex + 1);

		if(get_file_size(info_xml_x) <= 0) {
			hd_sindex_sum --;
		}
	}

	for(sindex = 0; sindex < s->sd_sindex_sum; sindex ++) {
		r_sprintf(info_xml_x, "%s/SD/%s.%d", course_root_dir, INFO_XML, sindex + 1);

		if(get_file_size(info_xml_x) <= 0) {
			sd_sindex_sum --;
		}
	}
	if(s->jpeg_sindex_sum > 0) {
		r_sprintf(info_xml_x, "%s/SD/%s.%d", course_root_dir, INFO_XML, sindex + 1);

		if(get_file_size(info_xml_x) <= 0) {
			jpeg_sindex_sum --;
		}
	}
	printf(  "xml_head hd_info_xml_fp start...");
	xml_head(hd_info_xml_fp);
	stream_head_node(hd_info_xml_fp, hd_sindex_sum + jpeg_sindex_sum, course_record_totaltime);
	fflush(hd_info_xml_fp);
	printf(  "xml_head hd_info_xml_fp end...");
	printf( "xml_head sd_info_xml_fp start...");
	xml_head(sd_info_xml_fp);
	stream_head_node(sd_info_xml_fp, sd_sindex_sum + jpeg_sindex_sum, course_record_totaltime);
	fflush(sd_info_xml_fp);
	printf( "xml_head sd_info_xml_fp end...");
	printf(  "channel_node hd_info_xml_fp start...");
	for(sindex = 0; sindex < s->hd_sindex_sum; sindex ++) {

		r_sprintf(info_xml_x, "%s/HD/%s.%d", course_root_dir, INFO_XML, hd_sindex);
		r_sprintf(blue_x, "%s/HD/%s.%d", course_root_dir, BLUE_X, hd_sindex);

		if((info_xml_x_fp = fopen(info_xml_x, "r")) == NULL) {
			printf(  "fopen[%d]  : %s", sindex, strerror(errno));
			return -1;
		}

		if((blue_x_fp = fopen(blue_x, "r")) == NULL) {
			printf(  "fopen[%d] [%s] : %s", sindex, blue_x, strerror(errno));
			return -1;
		}

		if(get_file_size(info_xml_x) > 0) {
			channel_head_node(hd_info_xml_fp, hd_sindex, "mp4");
			file_complete_copy(hd_info_xml_fp, info_xml_x_fp, "</file>");
			channel_blue_start_node(hd_info_xml_fp);
			file_complete_copy(hd_info_xml_fp, blue_x_fp, "/>");
			channel_blue_end_node(hd_info_xml_fp);
			fflush(hd_info_xml_fp);
		}

		hd_sindex ++;
		fclose(blue_x_fp);
		fclose(info_xml_x_fp);
		info_xml_x_fp = NULL;
	}
	printf(  "channel_node hd_info_xml_fp end...");
	printf(  "channel_node sd_info_xml_fp start...");
	for(sindex = 0; sindex < s->sd_sindex_sum; sindex ++) {
		r_sprintf(info_xml_x, "%s/SD/%s.%d", course_root_dir, INFO_XML, sd_sindex);
		r_sprintf(blue_x, "%s/SD/%s.%d", course_root_dir, BLUE_X, sd_sindex);

		if((info_xml_x_fp = fopen(info_xml_x, "r")) == NULL) {
			printf(  "fopen[%d]  : %s", sindex, strerror(errno));
			return -1;
		}

		if((blue_x_fp = fopen(blue_x, "r")) == NULL) {
			printf(  "fopen[%d] [%s] : %s", sindex, blue_x, strerror(errno));
			return -1;
		}

		if(get_file_size(info_xml_x) > 0) {
			channel_head_node(sd_info_xml_fp, sd_sindex, "mp4");
			file_complete_copy(sd_info_xml_fp, info_xml_x_fp, "</file>");
			channel_blue_start_node(sd_info_xml_fp);
			file_complete_copy(sd_info_xml_fp, blue_x_fp, "/>");
			channel_blue_end_node(sd_info_xml_fp);
			fflush(sd_info_xml_fp);
		}

		sd_sindex ++;
		fclose(blue_x_fp);
		fclose(info_xml_x_fp);
		info_xml_x_fp = NULL;
	}
	printf(  "channel_node sd_info_xml_fp end...");
	info_xml_x_fp = NULL;
	printf(  "channel_node jpeg_info_xml_fp start...");
	if(s->jpeg_sindex_sum > 0) {
		r_sprintf(info_xml_x, "%s/SD/%s.%d", course_root_dir, INFO_XML, sd_sindex);

		if((info_xml_x_fp = fopen(info_xml_x, "r")) == NULL) {
			printf(  "fopen[%d]  : %s", sindex, strerror(errno));
		}
		if(jpeg_sindex_sum) {
			channel_head_node(sd_info_xml_fp, sd_sindex, "jpg");
			file_complete_copy(sd_info_xml_fp, info_xml_x_fp, "</file>");
			channel_end_node(sd_info_xml_fp);
			rewind(info_xml_x_fp);
			channel_head_node(hd_info_xml_fp, hd_sindex, "jpg");
			file_complete_copy(hd_info_xml_fp, info_xml_x_fp, "</file>");
			channel_end_node(hd_info_xml_fp);
			fflush(sd_info_xml_fp);
			fflush(hd_info_xml_fp);
		}
	}
	printf( "channel_node jpeg_info_xml_fp end...");
	
	if(NULL != info_xml_x_fp) {
		rewind(info_xml_x_fp);
	}

	info2index(sd_index_xml_fp, info_xml_x_fp);

	if(NULL != info_xml_x_fp) {
		rewind(info_xml_x_fp);
	}

	info2index(hd_index_xml_fp, info_xml_x_fp);
	fflush(sd_index_xml_fp);
	fflush(hd_index_xml_fp);

	if(NULL != info_xml_x_fp) {
		fclose(info_xml_x_fp);
	}


	stream_end_node(hd_info_xml_fp);
	stream_end_node(sd_info_xml_fp);

	fclose(sd_info_xml_fp);
	fclose(hd_info_xml_fp);
	fclose(sd_index_xml_fp);
	fclose(hd_index_xml_fp);

	r_sprintf(hd_info_xml, "%s/HD/%s", course_root_dir, INFO_XML);
	rename(hd_info_xml_tmp, hd_info_xml);
	hd2sd(sd_info_xml, hd_info_xml);
	rename(sd_info_xml_tmp, sd_info_xml);

	r_sprintf(hd_index_xml, "%s/HD/%s", course_root_dir, INDEX_XML);
	rename(hd_index_xml_tmp, hd_index_xml);
	hd2sd(sd_index_xml, hd_index_xml);
	rename(sd_index_xml_tmp, sd_index_xml);
	hd_sindex = 1;
	sd_sindex = 1;

	for(sindex = 0; sindex < s->hd_sindex_sum; sindex ++)  {
		r_sprintf(info_xml_x, "%s/HD/%s.%d", course_root_dir, INFO_XML, hd_sindex);
		remove(info_xml_x);
		r_sprintf(blue_x, "%s/HD/%s.%d", course_root_dir, BLUE_X, hd_sindex);
		remove(blue_x);
		hd_sindex ++;
	}

	for(sindex = 0; sindex < s->sd_sindex_sum + s->jpeg_sindex_sum; sindex ++)  {
		r_sprintf(info_xml_x, "%s/SD/%s.%d", course_root_dir, INFO_XML, sd_sindex);
		remove(info_xml_x);
		r_sprintf(blue_x, "%s/SD/%s.%d", course_root_dir, BLUE_X, sd_sindex);
		remove(blue_x);
		sd_sindex ++;
	}

	printf(  "end...");
	return 0;
}


static int32_t GetInfoTotalTime(int8_t *path)
{
	char line[256] = {0};
	char *p = NULL;
	int width = 0;
	int height = 0;
	int totaltime = 0;
	int time = 0;
	char filename[128] = {0};
	int ret = 0;

	FILE *info_fp= fopen(path,"r");
	if(info_fp == NULL)
	{
		return -1;
	}
	
	while(1)
	{
		ret = fscanf(info_fp, FILE_NODE_FORMAT1, &width, &height, &totaltime, filename);
		p = strstr(filename, "</file>");
		if(NULL == p) {
			break;
		}

		p = fgets(line, 255, info_fp);

		if(NULL == p) {
			break;
		}

		time += totaltime;
	}
	
	fclose(info_fp);
	return time;
}

/*==============================================================================
    函数: <ErgodicDirectory>
    功能: <xh_Func:>遍历目录
    参数: 
    Created By 徐崇 2012.11.13 19:02:51 For Ftp
==============================================================================*/
static int32_t ErgodicDirectory(int8_t *RepairPath,int8_t *FileName, operator pfun, void *parm)
{
	if(NULL == RepairPath)
	{
		return -1;
	}

	DIR              *pDir ;
	struct dirent    *ent;
	int32_t      num = 0;
	char    childpath[MAX_FILE_PATH_LEN];
	int32_t     ret = 0;


	pDir = opendir(RepairPath);
	if(NULL == pDir)
	{
		printf("Cannot open directory:[ %s ]\n",RepairPath);
		return -1;
	}
	
	while((ent = readdir(pDir))!=NULL)
	{
		if(ent->d_type & DT_DIR)
		{
		
			if(r_strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)
				continue;
				
			sprintf(childpath,"%s/%s",RepairPath,ent->d_name);
			
			ret = ErgodicDirectory(childpath,FileName, pfun, parm);

			num = num + ret;
			
		}
		else
		{
		
			int8_t LocalFilePath[MAX_FILE_PATH_LEN];
			sprintf(LocalFilePath,"%s/",RepairPath);
			r_strncat(LocalFilePath, ent->d_name, r_strlen(ent->d_name));

			/* 隐藏文件过滤 */
			if(ent->d_name[0] == '.')
			{
				continue;
			}

			if(NULL != FileName)
			{
				int8_t *p = NULL;
			
				int8_t buffer[MAX_FILE_PATH_LEN] = {0};
				r_strcpy(buffer ,ent->d_name);
				p = strrchr(buffer,'.');
				if(p == NULL)
				{
					continue;
				}		
						
				if(0 == r_strcmp(p,FileName))
				{
					if(pfun != NULL)
					{
						if(1 == pfun(LocalFilePath,parm))
						{
							num++;
						}
					}
					else
					{
						num++;
					}
				}				
				
			}
			else
			{
				if(pfun == NULL)
				{
					num++;
				}
				else
				{
					if(1 == pfun(LocalFilePath,parm))
					{
						num++;
					}
				}
			}
			
		}
	}
	
	if(0 != closedir(pDir))
	{
		assert(0);
	}
	
	return num;
}

static int32_t IsStream(int8_t *RepairPath, void *parm)
{
	if((NULL == RepairPath) || (NULL == parm))
	{
		return 0;
	}
	int8_t *p = basename(RepairPath);
	if(0 == r_strncmp(p, parm, r_strlen(parm)))
	{
		if((p[r_strlen(parm)] > '9') || (p[r_strlen(parm)] < '0'))
		{
			return 0;
		}
			
		return 1;
	}
	return 0; 
}



/*==============================================================================
    函数: <CheckMp4Repair>
    功能: <xh_Func:>
    参数: 
    Created By 徐崇 2012.11.13 19:01:33 For Ftp
==============================================================================*/
static int32_t CheckMp4Repair(int8_t *RepairPath, int32_t *totalnum)
{
	int32_t ret = 0;
	int32_t IsRepair = 0;
	int8_t  *pDirMp4 = r_malloc(MAX_FILE_PATH_LEN);

	if((NULL == pDirMp4) || (NULL == totalnum))
	{
		return 0;
	}

	*totalnum = 0;

	/* 高码流文件 */
	r_strcpy(pDirMp4, RepairPath);
	r_strncat(pDirMp4, HD_VIDEO, r_strlen(HD_VIDEO));
	int32_t num = 0;
	num  = ErgodicDirectory(pDirMp4,REPAIR_TMPFORM, NULL, NULL);
	if(num > 0)
	{
		*totalnum += num;
		ret = ret|REPAIR_HD;
	}

	/* 低码流文件 */
	r_strcpy(pDirMp4, RepairPath);
	r_strncat(pDirMp4, SD_VIDEO, r_strlen(HD_VIDEO));
	num  = ErgodicDirectory(pDirMp4, REPAIR_TMPFORM, NULL, NULL);

	if(num > 0)
	{
		*totalnum += num;
		ret = ret|REPAIR_SD;
	}

	if(NULL != pDirMp4)
	{
		r_free(pDirMp4);
	}

	return ret;
}

/*==============================================================================
    函数: <RepairXmlInfoFile>
    功能: <xh_Func:>通过tmp路径修复info文件
    参数: flag 1 HD  0 SD
    Created By 徐崇 2012.11.16 11:39:02 For Ftp
==============================================================================*/
static int32_t RepairXmlInfoFile(int8_t *RepairPath,int8_t *RepairtmpPath,int32_t flag)
{
	int8_t pMP4path[MAX_FILE_PATH_LEN] = {0};
	int8_t *pInfopath = NULL;
	int32_t ret = -1;
	int32_t IsPre  = 0;
	FILE *pfile = NULL;
	
	if(NULL == RepairPath)
	{
		return ret;
	}

	pInfopath = r_malloc(MAX_FILE_PATH_LEN);
	if(NULL == pInfopath)
	{
		goto FAIL;
	}

	/* 获取MP4路径 */
	r_strcpy(pMP4path, RepairtmpPath);
	int8_t *pMP4 = r_strstr(pMP4path,".tmp");
	if(pMP4 == NULL)
	{
		goto FAIL;
	}
	r_strcpy(pMP4,".mp4");

	int8_t *pindex = basename(pMP4path);
	/* 获取info路径 */
	r_strcpy(pInfopath,  RepairPath);

	if(1 == flag)
	{
		r_strncat(pInfopath, INFO_XML_HD_INDEX, r_strlen(INFO_XML_HD_INDEX));
	}
	else if(0 == flag)
	{
		r_strncat(pInfopath, INFO_XML_SD_INDEX, r_strlen(INFO_XML_SD_INDEX));
	}
	else
	{
		goto FAIL;
	}
	
	int8_t  inexbuffer[100] = {0};
	int32_t index = 0;

	int8_t filename[100] = {0};
	r_strcpy(filename, pindex);

	int8_t *p = strchr(filename, '_');
	if(p == NULL)
	{
		goto FAIL;
	}

	/* 判断是否产生预览图 */
	if(p[1] == '1')
	{
		IsPre = 1;
	}
	*p = '\0';
	index	= atoi(filename);

	sprintf(inexbuffer,"%d",index);
	r_strncat(pInfopath, inexbuffer ,r_strlen(inexbuffer));


	/* 解析MP4 */
	DemuxReader mux;
	MediaReaderInit(&mux);

	if(0 != MediaOpenFile(&mux,pMP4path))
	{
		zlog_error(c,"MediaOpenFile Fail..........[%s]\n",pMP4path);
		goto FAIL;
	}

	pfile = fopen(pInfopath,"a+");
	if(pfile == NULL)
	{
		zlog_error(c,"MediaOpenFile Fail..........%s\n",pInfopath);
		goto FAIL;
	}
	
	fprintf(pfile,"<file width=\"%d\" height=\"%d\" totaltime=\"%lld\">videos/%s</file>\n",\
		mux.aiWidth[mux.iVideoStreamIdx], mux.aiHeight[mux.iVideoStreamIdx],mux.iDuration/1000,pindex);

	fclose(pfile);
	if(IsPre == 1)
	{
		int8_t *pImagepath = r_malloc(MAX_FILE_PATH_LEN);
		if(NULL == pImagepath)
		{
			goto FAIL;
		}

		/* 获取info路径 */
		r_strcpy(pImagepath,  RepairPath);
		if(1 == flag)
		{
			r_strncat(pImagepath, HD_RESOURCE_IMAGE_PRE, r_strlen(HD_RESOURCE_IMAGE_PRE));
		}
		else if(0 == flag)
		{
			r_strncat(pImagepath, SD_RESOURCE_IMAGE_PRE, r_strlen(SD_RESOURCE_IMAGE_PRE));
		}
		else
		{	
			r_free(pImagepath);
			goto FAIL;
		}

		sprintf(inexbuffer,"start%d.jpg",index);

		r_strncat(pImagepath, inexbuffer ,r_strlen(inexbuffer));
		
		MediaGenIndexImage(pMP4path, pImagepath,0,0,0);

		r_free(pImagepath);
	}

	ret = 1;
	
	
FAIL:

	
	if(NULL != pInfopath)
	{
		r_free(pInfopath);
	}

	
	return ret;
}

static int32_t GetXmlInfoNum(int8_t *RootPath, int flag)
{
	int32_t num = 0;

	if(NULL == RootPath)
	{
		return num;
	}

	int8_t *pdir = r_malloc(MAX_FILE_PATH_LEN);
	if(NULL == pdir)
	{
		goto FAIL;
	}
	
	r_strcpy(pdir, RootPath);

	if(HD_STREAM == flag)
	{
		r_strncat(pdir, "/HD/resource", r_strlen("/HD/resource"));
	}
	else if(SD_STREAM == flag)
	{
		r_strncat(pdir, "/SD/resource", r_strlen("/SD/resource"));
	}
	else
	{
		goto FAIL;
	}
	int8_t buffer[MAX_REPAIR_NUM] = {0};
	r_strcpy(buffer, "info.xml.");
	
	
	num = ErgodicDirectory(pdir, NULL, IsStream, (void *)buffer);


FAIL:
	if(NULL != pdir)
	{
		r_free(pdir);
	}
	return num;
}

static int32_t GetHaveJpeg(int8_t *RootPath, int flag)
{
	int32_t num = 0;

	if(NULL == RootPath)
	{
		return num;
	}

	int8_t *pdir = r_malloc(MAX_FILE_PATH_LEN);
	if(NULL == pdir)
	{
		goto FAIL;
	}
	
	r_strcpy(pdir, RootPath);

	if(HD_STREAM == flag)
	{
		r_strncat(pdir, "/HD/resource/images/1.jpg", r_strlen("/HD/resource/images/1.jpg"));
	}
	else if(SD_STREAM == flag)
	{
		r_strncat(pdir, "/SD/resource/images/1.jpg", r_strlen("/SD/resource/images/1.jpg"));
	}
	else
	{
		goto FAIL;
	}

	if(0 == access(pdir,0))
	{
		num = 1;
	}
	else
	{
		num = 0;
	}
	
FAIL:
	if(NULL != pdir)
	{
		r_free(pdir);
	}
	return num;
}


/*==============================================================================
    函数: <CheckXmlRepair>
    功能: <xh_Func:>检查XML是否需要修复 
    参数:  返回 1需要修复HD 0 SD  返回 -1 不需要修复
    Created By 徐崇 2012.11.14 15:28:00 For Ftp
==============================================================================*/
static int32_t CheckXmlRepair(int8_t *RepairPath)
{
	int32_t IsRepair = 0;

 	if(NULL == RepairPath)
 	{
		return -1;
	}

	int8_t  *pDirXML = r_malloc(MAX_FILE_PATH_LEN);

	int32_t i = 0;

	for(i = 0 ; i < MAX_XML_PATH; i++)
	{
		r_strcpy(pDirXML, RepairPath);
		r_strncat(pDirXML, XML_PATH[i], r_strlen(XML_PATH[i]));
		if(0 != access(pDirXML,0))
		{
			IsRepair = 1;
			goto FAIL;
		}

	}
	IsRepair = 0;
	
FAIL:
	if(NULL != pDirXML)
	{
		r_free(pDirXML);
	}
	return IsRepair;
}


/*==============================================================================
    函数: <RepairMp4File>
    功能: <xh_Func:>修复MP4文件
    参数: 
    Created By 徐崇 2012.11.16 15:43:40 For Ftp
==============================================================================*/
static int32_t RepairMp4File(int8_t *RepairPath, void *parm)
{	
	int64_t ret_val = 0;
	int32_t ret = -1;
	if(NULL == RepairPath)
	{
		return ret;
	}
	
	int8_t  *pDirMp4 = r_malloc(MAX_FILE_PATH_LEN);
	int8_t  *pOutMp4 = r_malloc(MAX_FILE_PATH_LEN);
	
	r_strcpy(pDirMp4, RepairPath);
	
	int8_t *p = strrchr(pDirMp4, '.');
	if(p == NULL)
	{
		goto FAIL;
	}

	r_strcpy(p, REPAIR_VIDEOFORM);

	r_strcpy(pOutMp4,pDirMp4);
	r_strncat(pOutMp4, REPAIR_TEMPFORM, r_strlen(REPAIR_TEMPFORM));


	/* 判断文件是否存在 */
	if( (0 != access(RepairPath,0)) || (0 != access(pDirMp4,0)))
	{
		goto FAIL;
	}

	//printf("%s %s %s\n",RepairPath, pDirMp4, pOutMp4);
	
	zlog_error(c,"Mp4Recovery Ing ..................[%s] [%s] [%s]\n",RepairPath,pDirMp4,pOutMp4);
	ret_val = Mp4Recovery(RepairPath, pDirMp4,pOutMp4);
	//ret= Mp4Recovery("/home/reach/xuchong/root/recv0/NewScmv2.2/HD/resource/videos/2_1.tmp","/home/reach/xuchong/root/recv0/NewScmv2.2/HD/resource/videos/2_1.mp4","/home/reach/xuchong/root/recv0/NewScmv2.2/HD/resource/videos/3.mp4");
	zlog_error(c,"Mp4Recovery Done..................[ret = %lld]\n", ret_val);
	if(ret_val == -1)
	{
		ret = -1;
		goto FAIL;
	}
	else
	{
		ret = remove(RepairPath);
		if(ret != 0)
		{
			goto FAIL;
		}
		
		ret = rename(pOutMp4,pDirMp4);
		if(ret != 0)
		{
			goto FAIL;
		}

	}

	ret = 1;

FAIL:

	if(NULL != pDirMp4)
	{
		r_free(pDirMp4);
	}
	if(NULL != pOutMp4)
	{
		r_free(pOutMp4);
	}
	return ret;
}


/*==============================================================================
    函数: <SaveFileName>
    功能: <xh_Func:>保存待修复文件
    参数: 
    Created By 徐崇 2012.11.16 15:43:53 For Ftp
==============================================================================*/
static int32_t SaveFileName(int8_t *RepairPath, void *parm)
{

	if(NULL == RepairPath)
	{
		return -1;
	}

	int8_t  *pDirMp4 = r_malloc(MAX_FILE_PATH_LEN);
	if(NULL == pDirMp4)
	{
		goto FAIL;
	}
	
	r_strcpy(pDirMp4, RepairPath);
	
	int8_t **pparm = (int8_t **)parm;

	while(NULL != *pparm)
	{
		pparm++;
	}


	*pparm = r_malloc(DIRLEN);
	if(NULL == *pparm)
	{
		return -1;
	}

	r_strcpy(*pparm,basename(pDirMp4));

	//printf("%s\n",*pparm);
FAIL:
	if(NULL != pDirMp4)
	{
		r_free(pDirMp4);
	}
	return 1;
}


/*==============================================================================
    函数: <cmp>
    功能: <xh_Func:>修复文件排序
    参数: 
    Created By 徐崇 2012.11.16 15:44:11 For Ftp
==============================================================================*/
static int cmp ( const void *a , const void *b ) 
{ 
	return strcmp((*(int8_t **)a) , *(int8_t **)b ); 
} 


/*==============================================================================
    函数: <RepairMp4XmlFile>
    功能: <xh_Func:>修复MP4和XML文件
    参数: 
    Created By 徐崇 2012.11.16 15:44:24 For Ftp
==============================================================================*/
static int32_t RepairMp4XmlFile(int8_t *RepairPath, int32_t flag)
{
	int32_t ret = -1;
	int8_t *RepairName[MAX_REPAIR_NUM] = {0};
	int32_t i = 0;
	int32_t num = 0;
	int32_t success = 0;
	
	int8_t  *pDirMp4 = r_malloc(MAX_FILE_PATH_LEN);
	if(NULL == pDirMp4)
	{
		return ret;
	}
	
	r_strcpy(pDirMp4, RepairPath);
	if(1 == flag)
	{
		r_strncat(pDirMp4, HD_VIDEO, r_strlen(HD_VIDEO));
	}
	else if(0 == flag)
	{
		r_strncat(pDirMp4, SD_VIDEO, r_strlen(HD_VIDEO));
	}
	else
	{
		goto FAIL;
	}
	

	/* 保存需要修复的文件名称 */
	num	= ErgodicDirectory(pDirMp4, REPAIR_TMPFORM, SaveFileName, (void *)RepairName);

	/* 对文件进行排序 */
	qsort(RepairName,num,sizeof(int8_t **),cmp); 


	/* 对文件进行修复 */
	for(i = 0; i < num; i++)
	{
		int8_t  *pTmpFile = r_malloc(MAX_FILE_PATH_LEN);
		if(NULL == pTmpFile)
		{
			goto FAIL;
		}
		
		r_strcpy(pTmpFile, pDirMp4);
		r_strncat(pTmpFile, "/", 1);
		r_strncat(pTmpFile, RepairName[i], r_strlen(RepairName[i]));

		/* 修复成功 */
		if(1 == RepairMp4File(pTmpFile, NULL))
		{
			/* 修复info文件 */
			RepairXmlInfoFile(RepairPath, pTmpFile, flag);
				
			success++;
		}
		else
		{
			continue;
		}
		r_free(pTmpFile);
	}

	ret = success;
	
FAIL:
	
	if(NULL != pDirMp4)
	{
		r_free(pDirMp4);
	}
	
	for(i = 0; i < num; i++)
	{
		if(NULL != RepairName[i])
		{
			r_free(RepairName[i]);
		}
	}
	return ret;
}

/*==============================================================================
    函数: <RepairVideoFile>
    功能: <xh_Func:>修复视频文件
    参数: 
    Created By 徐崇 2012.11.14 14:23:56 For Ftp
==============================================================================*/
static int32_t RepairVideoFile(int8_t *RepairPath, int32_t index)
{
	int32_t num = 0;

	if((NULL == RepairPath) || (0 == index))
	{
		return -1;
	}
	/* 修复高码流 */
	if((REPAIR_HD == index) || (REPAIR_ALL == index))
	{
		num = RepairMp4XmlFile(RepairPath, 1);
	}

	/* 修复低码流 */
	if((REPAIR_SD == index) || (REPAIR_ALL == index))
	{
		num += RepairMp4XmlFile(RepairPath, 0);
		
	}
	
	return num;
}


static int32_t RepairBuleFile(int8_t *RepairPath, int32_t time)
{

	int32_t ret = 0;
	uint32_t len = 0;
	int32_t stattime = -1;
	int32_t endtime  = -1;
	FILE *file = NULL;
	
	file = fopen(RepairPath,"rw+");
	if(file == NULL)
	{
		return -1;
	}

	//空文件
	if(fgetc(file) == EOF)
	{
		fclose(file);
		return -1;
	}

	if(0 != fseek(file, 0L, SEEK_END))
	{
		fclose(file);
		return -1;
	}
	
	//只有一行
	len = ftell(file);
	if(len < 50)
	{
		fseek(file, 0L, SEEK_SET);
		fscanf(file,"<bluetime  start=\"%d\" end=\"%d\"/>", &stattime, &endtime);
	
		if(endtime == 0)
		{
			fseek(file, 0L, SEEK_SET);
			blue_node(file,stattime,time);
		}
	}
	else
	{
		int offlen = 0;
		while(1)
		{	 
			  if(fgetc(file) !='\n')
			  {
			     fseek(file, -2L, SEEK_CUR);
				 offlen++;
				 if(offlen > 50)
				    break;
			  }
			  else
		    	break;
		}
		
		fscanf(file,"<bluetime  start=\"%d\" end=\"%d\"/>", &stattime, &endtime);
		if(endtime == 0)
		{
			fseek(file, -offlen, SEEK_END);
			blue_node(file,stattime,time);
		}
	}
	
	fclose(file);	
	return 1;
}

static int32_t RepairBule(int8_t *RepairPath, int32_t hdnum, int32_t sdnum, int32_t time)
{
	int32_t i = 1;
	if((hdnum == 0) && (sdnum == 0))
	{
		return -1;
	}
	
	int8_t  *pDirBlue = r_malloc(MAX_FILE_PATH_LEN);

	for(i = 1;i<=hdnum;i++)
	{
		int8_t buffer[DIRLEN] = {0};
		r_strcpy(pDirBlue, RepairPath);

		sprintf(buffer, "/HD/resource/blue.%d",i);
		r_strncat(pDirBlue,buffer, r_strlen(buffer));
		RepairBuleFile(pDirBlue, time);
	}
	
	for(i = 1; i <= sdnum; i++)
	{
		int8_t buffer[DIRLEN] = {0};
		r_strcpy(pDirBlue, RepairPath);

		sprintf(buffer, "/SD/resource/blue.%d",i);
		r_strncat(pDirBlue,buffer, r_strlen(buffer));
		RepairBuleFile(pDirBlue, time);
		
	}
	//assert(0);
	return -1;
}

/*==============================================================================
    函数: <RepairXmlFile>
    功能: <xh_Func:>修复XML文件
    参数: 
    Created By 徐崇 2012.11.16 15:44:39 For Ftp
==============================================================================*/
static int32_t RepairXmlFile(int8_t *RepairPath)
{

	stream_type_t *ptype = NULL;
	if(NULL == RepairPath)
	{
		return -1;
	}
	
	int8_t  *pDirXML = r_malloc(MAX_FILE_PATH_LEN);
	r_strcpy(pDirXML, RepairPath);
	
	r_strncat(pDirXML, "/HD/resource/info.xml.1", r_strlen("/HD/resource/info.xml.1"));
	
	int32_t totaltime = 0;
	totaltime = GetInfoTotalTime(pDirXML);
	
	if(totaltime < 0)
	{
		r_strcpy(pDirXML, RepairPath);
		r_strncat(pDirXML, "/SD/resource/info.xml.2", r_strlen("/SD/resource/info.xml.1"));
		totaltime = GetInfoTotalTime(pDirXML);
		if(totaltime < 0)
		{
			goto FAIL;
		}
	}
	
	int32_t vidoehdnum = 0;
	int32_t vidoesdnum = 0;
	int32_t jpegnum = 0;
	
	vidoehdnum = GetXmlInfoNum(RepairPath,HD_STREAM);	
	vidoesdnum = GetXmlInfoNum(RepairPath,SD_STREAM);
	jpegnum    = GetHaveJpeg(RepairPath, HD_STREAM);

	if((vidoehdnum + jpegnum) <= 0)
	{
		goto FAIL;
	}	

	ptype = r_malloc(sizeof(stream_type_t)*(vidoehdnum + vidoesdnum));
	if(ptype == NULL)
	{
		goto FAIL;
	}
	
	int32_t i,j = 0;
	for(i = 0; i < vidoehdnum; i++)
	{
		ptype[i] = RECORD_HD;
	}

	for(i = 0; i < vidoesdnum; i++)
	{
		ptype[vidoehdnum+i] = RECORD_SD;
	}
	
	if(jpegnum == 1)
	{
		ptype[vidoehdnum + vidoesdnum -1] = RECORD_JPEG;
	}

	for(i = 0; i < vidoehdnum + vidoesdnum; i++)
	{
		printf("%d\n",ptype[i]);
	}

	RepairBule(RepairPath,vidoehdnum,vidoesdnum,totaltime);
	stream_type_sindex_sum_t s;
	
	s.hd_sindex_sum = vidoehdnum;
	s.sd_sindex_sum = vidoesdnum - jpegnum;
	s.jpeg_sindex_sum = jpegnum;
		
	printf("1----------%s %d\n",RepairPath,totaltime);
	create_course_record_xml(RepairPath, totaltime , &s);
	printf("2----------%s %d %d %p\n",RepairPath,vidoehdnum + vidoesdnum,jpegnum,ptype);

FAIL:
	if(NULL != pDirXML)
	{
		r_free(pDirXML);
	}
	if(NULL != ptype)
	{
		r_free(ptype);
	}
	
	return 1;
}






/*==============================================================================
    函数: <ReadFile>
    功能: <xh_Func:>读索引文件
    参数: 
    Created By 徐崇 2012.11.13 19:03:14 For Ftp
==============================================================================*/
static int32_t ReadFile(int8_t *RepairPath, void *parm)
{

	if(NULL == RepairPath)
	{
		return -1;
	}
	
	int8_t **pparm = (int8_t **)parm;

	while(NULL != *pparm)
	{
		pparm++;
	}


	*pparm = r_malloc(DIRLEN);
	if(NULL == *pparm)
	{
		return -1;
	}
	
	int8_t *p = r_strstr(RepairPath, RECOVERY_PATH);
	if(NULL == p)
	{
		r_free(*pparm);
		return  -1;
	}
	p =  p + r_strlen(RECOVERY_PATH);
	
	strcpy(*pparm, p);

	//unlink(RepairPath);
	return 1;
}

/*==============================================================================
    函数: <CheckRepairFile>
    功能: <xh_Func:>检查索引
    参数: 
    Created By 徐崇 2012.11.13 19:03:30 For Ftp
==============================================================================*/
static int32_t CheckRepairIndex(int8_t *RepairPath,int8_t **RepairName)
{
	int32_t num = 0;
	if((NULL == RepairPath) || (NULL == RepairName))
	{
		return 0;
	}

	num = ErgodicDirectory(RepairPath, NULL, ReadFile, (void *)RepairName);
	
	return num;
}

static int connectToDecodeServer()
{
	struct sockaddr_in serv_addr;
	const char* pAddr  = "127.0.0.1";
	int socketFd = -1;
	
	socketFd= socket(PF_INET, SOCK_STREAM, 0);
	if(socketFd < 1)
		return -1;
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(3000);
	inet_aton(pAddr,(struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero),8);
	
	if(connect(socketFd, (struct sockaddr *)&serv_addr,sizeof(struct sockaddr)) == -1)	{
		close(socketFd);
		return -1;
	}
	return socketFd;
}

static int NoticeUpCourse(char *path)
{
	int sockfd = 0;

	char xml[4096] = {0}; 
	char buffer[4096] = {0}; 

	FILE *file = NULL;
	int8_t *pRootPath = NULL;
	
	pRootPath= (int8_t *)r_malloc(MAX_FILE_PATH_LEN);
	if(NULL == pRootPath)
	{
		return -1;
	}
	
	MsgHeader	*pmsg  = NULL;
	while(1)
	{
		sockfd = connectToDecodeServer();

		if(sockfd >= 0)
		{
			break ;
		}
		sleep(5);
		continue;
	}

	r_memset(xml, 0x0,4096);
	pmsg = (MsgHeader *)xml;
	pmsg->sMsgType = 0;
	pmsg->sMsgType = r_htons(pmsg->sMsgType);
	pmsg->sVer =     r_htons(2012);

	r_strcpy(pRootPath, path);
	r_strncat(pRootPath, "/ContentInfo.xml", r_strlen("/ContentInfo.xml"));

	file = fopen(pRootPath,"r");
	if(file == NULL)
	{
		zlog_error(c,"fopen Xml fail");
		goto FAIL;
	}

	if(0 == fread(buffer,1,2048,file))
	{
		zlog_error(c,"fread Xml fail");
		goto FAIL;
	}

	int8_t *p = NULL;
	int8_t *pStart = NULL;
	int8_t *pEnd   = NULL;

	/*RecordID不一定固定为32 位*/
#if 1
	pStart = strstr(buffer,"<RecordID>");
	if(pStart == NULL)
	{
		zlog_error(c,"Dont'find RecordID");
		goto FAIL;
	}

	pStart += strlen("<RecordID>");

	pEnd = strstr(buffer,"</RecordID>");
	if(pEnd == NULL)
	{
		zlog_error(c,"Dont'find RecordID");
		goto FAIL;
	}

	*pEnd = '\0';

	p = pStart;
#else
	p = strstr(buffer,"</RecordID>");
	if(p == NULL)
	{
		zlog_error(c,"Dont'find RecordID");
		goto FAIL;
	}
	
	*p = '\0';
	p = p - 32;
#endif
	record_stop_msg(xml,p,path);
	int sLen = sizeof(MsgHeader) + strlen(xml + sizeof(MsgHeader));
    pmsg->sLen     = r_htons(sLen);


	zlog_error(c,"NoticeUpCourse: %s Done",xml + sizeof(MsgHeader));
	printf("%s",xml + sizeof(MsgHeader));
	send(sockfd, xml, sLen, 0);
	
FAIL:
	close(sockfd);

	if(pRootPath!= NULL)
		r_free(pRootPath);

	if(file != NULL)
		fclose(file);
	
	return 1;
	
}



/*==============================================================================
    函数: <RepairCourseware>
    功能: <xh_Func:>修复课件
    参数: 
    Created By 徐崇 2012.11.13 19:03:47 For Ftp
==============================================================================*/
static int32_t RepairCourseware(int8_t *RootPath)
{
	int8_t *pRootPath = NULL;
	int8_t *RepairName[MAX_REPAIR_NUM] = {0};
	int32_t i = 0;
	int32_t rc = 0;
	if(NULL == RootPath)
	{
		return -1;
	}

	pRootPath = (int8_t *)r_malloc(MAX_FILE_PATH_LEN);
	if(NULL == pRootPath)
	{
		return -1;
	}


	rc = zlog_init("./zlog_upload.conf");
	
	if(rc)
	{
		printf("init failed\n");
		return -1;
	}
	
	c  = zlog_get_category("Repair");
	
	if(!c)
	{
		return -1;
	}


	zlog_error(c,"Mp4Repair Ing..........");
	r_strcpy(pRootPath, RootPath);
	r_strncat(pRootPath, RECOVERY_PATH, r_strlen(RECOVERY_PATH));


	/* 检查待修复文件 */
	int32_t num = 0;
	num = CheckRepairIndex(pRootPath, RepairName);
	zlog_error(c,"Mp4Repair CheckRepairIndex [%d]..........",num);
	if(num <= 0)
	{
		zlog_error(c,"Mp4Repair Done ..........",num);
		goto	FAIL;
	}
	
	/*修复MP4文件 */
    for(i = 0; i < num ; i++)
    {
	
		if(RepairName[i] != NULL)
		{
			/* 生成课件目录 */
			r_strcpy(pRootPath, RootPath);
			r_strncat(pRootPath, RepairName[i], r_strlen(RepairName[i]));
			int32_t totalnum = 0;


			#if 1
			/* 检查MP4是否需要修复 */
			int32_t index = CheckMp4Repair(pRootPath, &totalnum);
			if(index !=  0)
			{
				int32_t ret = 0;

				zlog_error(c,"RepairVideoFile ...[%s] [%d]\n",pRootPath, index);
				/* 修复文件 */
				ret = RepairVideoFile(pRootPath, index);
				if(ret != totalnum)
				{
					zlog_error(c,"RepairVideoFile fail: success[%d] total num[%d]\n",ret,totalnum);
				}
				else
				{
					zlog_error(c,"RepairVideoFile success: success[%d] total num[%d]\n",ret,totalnum);
				}
			}
			#endif
		}
	}

	/* 修复XML文件 */
    for(i = 0; i < num ; i++)
    {	
		if(RepairName[i] != NULL)
		{
			/* 生成课件目录 */
			r_strcpy(pRootPath, RootPath);
			r_strncat(pRootPath, RepairName[i], r_strlen(RepairName[i]));
			int32_t ret = 0;

			zlog_error(c,"Start RepairXmlFile ...[%s][%d]\n",pRootPath,num);
			
			//ret  = CheckXmlRepair(pRootPath);

			//修复xml
			RepairXmlFile(pRootPath);
			
			//修复课件md5
			//DirDetectionFile(pRootPath);

			zlog_error(c,"NoticeUpCourse: %s .....",pRootPath);
			//通知上传课件
			NoticeUpCourse(pRootPath);
			
			//删除文件
			r_strcpy(pRootPath, RootPath);
			r_strncat(pRootPath, RECOVERY_PATH, r_strlen(RECOVERY_PATH));
			r_strncat(pRootPath, RepairName[i], r_strlen(RepairName[i]));
			unlink(pRootPath);
		}

	}

	
	
FAIL:

	if(NULL != pRootPath)
	{
		r_free(pRootPath);
	
	}
	
	for(i = 0; i < num; i++)
	{
		if(RepairName[i] != NULL)
		{
			r_free(RepairName[i]);
		}
	}

	return 1;
}




/*==============================================================================
    函数: <main>
    功能: <xh_Func:>
    参数: 
    Created By 徐崇 2012.11.16 15:45:48 For Ftp
==============================================================================*/
int main()
{
	
	
	RepairCourseware("/opt/Rec");

	return 1;
}
