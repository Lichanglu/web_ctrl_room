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

#include "reach_upload.h"

static zlog_category_t *g_zlog;
static pthread_mutex_t xmllock;

static char *basename(char *path)
{
	/* Ignore all the details above for now and make a quick and simple
	   implementaion here */
	char *s1;
	char *s2;
	
	s1=strrchr(path, '/');
	s2=strrchr(path, '\\');
	
	if(s1 && s2)
	{
		path = (s1 > s2? s1 : s2)+1;
	}
	else
		if(s1)
			path = s1 + 1;
		else
			if(s2)
				path = s2 + 1;
				
	return path;
}

/*==============================================================================
    函数: <ScnDir>
    功能: <xh_Func:>
    参数:
    Created By 徐崇 2012.10.17 10:39:26 For Ftp
==============================================================================*/
int  ScnDir(Ftp_Hand *pFtpHand, char *DirPath, char *RomteFilePath , int DirLen, char * pport)
{
	DIR              *pDir ;
	struct dirent    *ent;
	char    childpath[MAX_FILE_PATH_LEN];
	
	int     ret = CURLE_OK;
	pDir = opendir(DirPath);
	if(NULL == pDir)
	{
		zlog_error(g_zlog, "Cannot open directory: %s",DirPath);
		return REACH_UPLOAD_DIRECTORY_INVALID;
	}
	
	while((ent = readdir(pDir))!=NULL)
	{
		if(ent->d_type & DT_DIR)
		{

			if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)
				continue;
				
			sprintf(childpath,"%s/%s",DirPath,ent->d_name);
			
			ret = ScnDir(pFtpHand,childpath,RomteFilePath,DirLen,pport);
			
			if(ret != CURLE_OK)
				break;
				
		}
		else
		{
		
			char LocalFilePath[MAX_FILE_PATH_LEN];
			char UpFilePath[MAX_FILE_PATH_LEN] = {0};
			strcpy(UpFilePath,RomteFilePath);//REMOTE_URL;
			
			sprintf(LocalFilePath,"%s/",DirPath);
			sprintf(UpFilePath,"%s/",UpFilePath);
			strncat(LocalFilePath, ent->d_name, strlen(ent->d_name));
			
			/* 隐藏文件过滤 */
			if(ent->d_name[0] == '.')
			{
				continue;
			}

			if(strcmp(ent->d_name, "download.info") == 0){
				continue;
			}

			if((strstr(ent->d_name, ".mp4") != NULL) && ((strstr(ent->d_name, "HD") != NULL) || (strstr(ent->d_name, "SD") != NULL))){
				continue;
			}
			
			strcpy(UpFilePath + strlen(UpFilePath), LocalFilePath + DirLen);
			Ftp_UpLoadParam	upconfig = {0};
			upconfig.BreakpointUpLoad = TRUE;
			upconfig.FtpPort = pport;
			upconfig.LocalFilePath = LocalFilePath;
			upconfig.UpFilePath    = UpFilePath;
			int num = TRYNUM;
			while(num)
			{
				ret = pFtpHand->Ftp_UpLoadFile((void *)pFtpHand, &upconfig);
				
				if(ret == CURLE_OK)
				{
					zlog_debug(g_zlog, "UpLoad File %s succeed",upconfig.LocalFilePath);
					break;
					
				}
				else
				{
					zlog_error(g_zlog, "UpLoad File %s fail.. error num[%d] try num[%d] %s\n",upconfig.LocalFilePath,ret,TRYNUM - num,ftp_strerror(ret));
				}
				num--;
			}
			
			if((ret != CURLE_OK) && (num == 0))
			{
				zlog_error(g_zlog, RED"UpLoad File %s fail.. error num[%d] trynums[%d] %s\n"NONE,upconfig.LocalFilePath,ret,TRYNUM,ftp_strerror(ret));
				break;
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
int  DetectionFile(Ftp_Hand *pFtpHand, char *UpLoadFilePath ,char *RomteFilePath,char* pport)
{
	if((pFtpHand == NULL) || (UpLoadFilePath == NULL)
	        || (RomteFilePath == NULL) || (pport == NULL))
	{
		return REACH_UPLOAD_PARAM_NULL;
	}
	int Len = strlen(UpLoadFilePath) - strlen(basename(UpLoadFilePath));
	return ScnDir(pFtpHand,UpLoadFilePath,RomteFilePath ,Len , pport);
}



/*==============================================================================
    函数: <UploadXmlInit>
    功能: <xh_Func:>
    参数:
    Created By 徐崇 2012.10.17 10:39:41 For Ftp
==============================================================================*/
int UploadInit(parse_xml_t *px)
{
	if(px == NULL)
	{
		return -1;
	}
	
	g_zlog = zlog_get_category("FtpUpload");
	if (!g_zlog)
	{
		return -1;
	}
	pthread_mutex_init(&xmllock, NULL);
	
RESTART:
	
	/* 如果文件不存在，则创建文件 */
	if(access("./UploadConfig.xml", F_OK) != 0)
	{
		xmlNodePtr WaitUpLoadFile;
		px->pdoc = xmlNewDoc(BAD_CAST"1.0"); 			//定义文档和节点指针
		if(NULL == px->pdoc)
		{
			zlog_error(g_zlog, "xmlNewDoc failed, file: .UoloadConfig\n");
			return -1;
		}
		px->proot= xmlNewNode(NULL, BAD_CAST"FtpFileSata");
		xmlDocSetRootElement(px->pdoc, px->proot);		//设置根节点

		xml_add_new_child(px->proot, NULL, BAD_CAST "MaxSpeed", (xmlChar *)"2097152");
		xml_add_new_child(px->proot, NULL, BAD_CAST "User", (xmlChar *)"user");
		xml_add_new_child(px->proot, NULL, BAD_CAST "Passwd", (xmlChar *)"Passwd");
		xml_add_new_child(px->proot, NULL, BAD_CAST "RomteIp", (xmlChar *)"0.0.0.0");
		xml_add_new_child(px->proot, NULL, BAD_CAST "RomtePath", (xmlChar *)"/");
		xml_add_new_child(px->proot, NULL, BAD_CAST "Port", (xmlChar *)"21");
		
		WaitUpLoadFile = xmlNewNode(NULL, BAD_CAST "WaitUpLoadFile");
		xmlAddChild(px->proot, WaitUpLoadFile);

		int ret = xmlSaveFormatFileEnc("UploadConfig.xml", px->pdoc, "UTF-8", 1);
		if(-1 == ret)
		{
			zlog_error(g_zlog, "xml save params table failed !!!\n");
			ret = -1;
		}
		
	}
	else
	{
		char* ret = init_file_dom_tree(px, "UploadConfig.xml");
		if(ret == NULL)
		{
			zlog_error(g_zlog, "init_file_dom_tree failed, xml file: UploadConfig.xml");
			system("rm -f UploadConfig.xml");
			goto RESTART;
			//return -1;
		}
	
	}
	return 1;
}

/*==============================================================================
    函数: <AddXmlNode>
    功能: <xh_Func:>
    参数:
    Created By 徐崇 2012.10.17 10:39:34 For Ftp
==============================================================================*/
int AddXmlNode(parse_xml_t *px, Upload_Info*config,char *LoalLoadFile, char *UpLoadPath, char *UpLoadIp,char* pport, char *precid)
{
	xmlNodePtr WaitUpLoadFile;
	xmlNodePtr FileNode;

	xmlNodePtr TmpNode;

	int ret = -1;
	
	if((px == NULL) || (config == NULL) || (LoalLoadFile == NULL) || (pport == NULL))
	{
		return -1;
	}

	//md5校验
	DirDetectionFile((int8_t*)LoalLoadFile);

	pthread_mutex_lock(&xmllock);
	WaitUpLoadFile = get_children_node(px->proot, BAD_CAST"WaitUpLoadFile");
	if(NULL == WaitUpLoadFile)
	{
		zlog_error(g_zlog, "get sys_param node failed, xml file: UploadConfig.xml");
		r_pthread_mutex_unlock(&xmllock);
		return -1;
	}
	
	FileNode = xmlNewNode(NULL, BAD_CAST ("FileStat"));
	
	if(FileNode == NULL)
	{
		zlog_error(g_zlog, "xmlNewNode FileStat fail!!!");
		r_pthread_mutex_unlock(&xmllock);
		return -1;
	}
		
	xmlAddChild(WaitUpLoadFile, FileNode);
	xmlNewTextChild(FileNode, NULL, BAD_CAST"LocalFilePath", BAD_CAST(LoalLoadFile));
	xmlNewTextChild(FileNode, NULL, BAD_CAST"RecordID", BAD_CAST(precid));

	TmpNode = get_children_node(px->proot, (xmlChar *)"RomteIp");
	if(TmpNode != NULL)
	{
		modify_node_value(TmpNode, (const xmlChar *)UpLoadIp);
	}
	
	TmpNode = get_children_node(px->proot, (xmlChar *)"RomtePath");
	if(TmpNode != NULL)
	{
		modify_node_value(TmpNode, (const xmlChar *)UpLoadPath);
	}
	
	TmpNode = get_children_node(px->proot, (xmlChar *)"Port");
	if(TmpNode != NULL)
	{
		modify_node_value(TmpNode, (const xmlChar *)pport);
	}

	TmpNode = get_children_node(px->proot, (xmlChar *)"User");
	if(TmpNode != NULL)
	{
		modify_node_value(TmpNode, (const xmlChar *)config->UsrName);
	}

	TmpNode = get_children_node(px->proot, (xmlChar *)"Passwd");
	if(TmpNode != NULL)
	{
		modify_node_value(TmpNode, (const xmlChar *)config->PassWord);
	}
	
	ret = xmlSaveFormatFileEnc("./UploadConfig.xml", px->pdoc, "UTF-8", 1);
	if(-1 == ret)
	{
		zlog_error(g_zlog, "xml save params table failed !!!");
		r_pthread_mutex_unlock(&xmllock);
		return -1;
	}


	strcpy(config->Port, pport);
	strcpy(config->RomteIp, UpLoadIp);
	strcpy(config->RomtePath, UpLoadPath);
	
	pthread_mutex_unlock(&xmllock);

	return 1;
	
}

/*==============================================================================
    函数: <ReadXmlFile>
    功能: <xh_Func:>
    参数:
    Created By 徐崇 2012.10.17 10:39:49 For Ftp
==============================================================================*/
int UpLoadFile(parse_xml_t *px,Ftp_Hand *pFtpHand,Upload_Info *UploadConfig,SendMessage pfuntion)
{

	xmlNodePtr WaitUpLoadFile = NULL;
	xmlNodePtr FileNode = NULL;
	xmlNodePtr LocalFileNodeStat = NULL;
	
	xmlNodePtr Precid = NULL;
	int ret = -1;
	
	if((px == NULL) || (pFtpHand == NULL) || (UploadConfig == NULL))
	{
		ret = REACH_UPLOAD_XMLFTP_NULL;
		pfuntion("FtpUpload serious error",(char *)ftp_strerror(ret),NULL,NULL,0);
		return -1;
	}
	
	pthread_mutex_lock(&xmllock);
	WaitUpLoadFile = get_children_node(px->proot, BAD_CAST"WaitUpLoadFile");
	pthread_mutex_unlock(&xmllock);
	
	if(NULL == WaitUpLoadFile)
	{
		ret = REACH_UPLOAD_XMLFTP_NULL;
		pfuntion("FtpUpload serious error",(char *)ftp_strerror(ret),NULL,NULL,0);
		zlog_error(g_zlog, "get sys_param node failed, xml file: UploadConfig.xml");
		return -1;
	}
	
	
	FileNode	= WaitUpLoadFile->children;
	while(FileNode != NULL)
	{
		xmlChar  *LocalFilePath = NULL;
		xmlChar  *pprecid = NULL;
		int8_t RomtePath[MAX_FILE_PATH_LEN] = {0};
		int8_t Port[MAX_FILE_PATH_LEN] = {0};
	
		
		r_pthread_mutex_lock(&xmllock);
		LocalFileNodeStat   = get_children_node(FileNode, BAD_CAST "LocalFilePath");
		Precid 			    = get_children_node(FileNode, BAD_CAST "RecordID");
		
		if((!LocalFileNodeStat)||(!Precid))
		{
		
			pfuntion("Read XML Failed","XML Is Damge",NULL,NULL,0);
			r_pthread_mutex_unlock(&xmllock);
			zlog_error(g_zlog, "UpLoad failed: Get_children_node ERROR");
			goto EXIT;
		}
		
		LocalFilePath = xmlNodeListGetString(px->pdoc, LocalFileNodeStat->xmlChildrenNode, 1);
		pprecid       = xmlNodeListGetString(px->pdoc, Precid->xmlChildrenNode, 1);
		
		r_pthread_mutex_unlock(&xmllock);

		if(LocalFilePath == NULL)
		{
			pfuntion("Read XML Failed","XML Is NULL",NULL,NULL,0);
			zlog_error(g_zlog, "UpLoad failed: LocalFilePath  pprecid NULL");
			goto EXIT;
		}

		//配置上传信息
		Ftp_ParamConfig	 config = {{0}};
		sprintf((char *)RomtePath,"ftp://%s%s",UploadConfig->RomteIp,UploadConfig->RomtePath);
		//r_strcpy(RomtePath , (int8_t *)UploadConfig->RomtePath);
		r_strcpy(Port ,		 (int8_t *)UploadConfig->Port);
		r_strcpy((int8_t *)config.UsrName,  (int8_t *)UploadConfig->UsrName);
		r_strcpy((int8_t *)config.PassWord, (int8_t *)UploadConfig->PassWord);
		config.MaxSendSeeed = UploadConfig->MaxSpeed;
			
		/* Ftp配置 */
		if(1 != pFtpHand->Ftp_Config(pFtpHand,&config))
		{
			zlog_error(g_zlog, "Ftp_Config: Ftp_Config fail UsrName[%s] PassWord[%s] Port[%s]",UploadConfig->UsrName,UploadConfig->PassWord);
			
		}


		/* 遍历指定目录下文件，保存XML */
		ret = DetectionFile(pFtpHand, (char *)LocalFilePath,(char *)RomtePath ,(char *)Port);
		if(CURLE_OK != ret)
		{
			pfuntion((char *)LocalFilePath,(char *)ftp_strerror(ret),NULL,NULL,0);
			zlog_error(g_zlog, "UpLoad %s failed: DetectionFile ERROR",LocalFilePath);
		}
		else
		{
		
			pfuntion((char *)LocalFilePath,(char *)pprecid,UploadConfig->RomteIp,UploadConfig->RomtePath,1);
			zlog_debug(g_zlog, "UpLoad File %s succeed",LocalFilePath);
		}
		
EXIT:
		r_pthread_mutex_lock(&xmllock);
		if((NULL != FileNode) && ((ret == CURLE_OK) ||( ret == REACH_UPLOAD_DIRECTORY_INVALID)))
		{
			FileNode = del_cur_node(FileNode);
		}
		else 
		{
			FileNode = FileNode->next;
		}
		
		ret = xmlSaveFormatFileEnc("UploadConfig.xml", px->pdoc, "UTF-8", 1);
		if(-1 == ret)
		{
			ret = REACH_UPLOAD_SAVEXML_ERROR;
			pfuntion((char *)LocalFilePath,(char *)ftp_strerror(ret),NULL,NULL,0);
			zlog_error(g_zlog, "xml save params table failed !!!");
		}
		r_pthread_mutex_unlock(&xmllock);
		
		if(LocalFilePath != NULL)
		{
			xmlFree(LocalFilePath);
		}
		
		if(pprecid != NULL)
		{
			xmlFree(pprecid);
		}
		
	}
	return 1;
}

/*==============================================================================
    函数: <DeUploadXmlInit>
    功能: <xh_Func:>
    参数:
    Created By 徐崇 2012.10.17 10:39:57 For Ftp
==============================================================================*/
void DeUploadInit(parse_xml_t *px)
{
	pthread_mutex_destroy(&xmllock);
	release_dom_tree(px->pdoc);
}


