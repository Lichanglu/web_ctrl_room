#include "reach_upload.h"

FtpService *pFtpServerHand;

extern int testsocket();
extern int RegisterrSerialControlTask();

static void TcpSockLock()
{
	pthread_mutex_lock(&pFtpServerHand->socklock);
}

static void TcpSockunLock()
{
	pthread_mutex_unlock(&pFtpServerHand->socklock);
}

static int TcpSockLockInit()
{
	return pthread_mutex_init(&pFtpServerHand->socklock, NULL);
}

static int TcpSockLockdeInit()
{
	return pthread_mutex_destroy(&pFtpServerHand->socklock);
}


static void SetTcpSocket(int socket)
{

	TcpSockLock();
	pFtpServerHand->ClientSocket = socket;
	TcpSockunLock();
}

static int GetTcpSocket()
{
	return pFtpServerHand->ClientSocket;
}

static int FtpServiceInit(FtpService *pFtpServerHand)
{
	int rc = 0;
	
	pFtpServerHand->sendflg = 1;		//可以发生定时器
	pFtpServerHand->ClientSocket = -1;	//sokcet初始化
	
	rc = zlog_init("./zlog_upload.conf");
	
	if(rc)
	{
		printf("init failed\n");
		return -1;
	}
	
	pFtpServerHand->c  = zlog_get_category("FtpMain");
	
	if(!pFtpServerHand->c)
	{
		return -1;
	}
	
	zlog_category_t *c = pFtpServerHand->c;
	
	pFtpServerHand->FtpHand = (Ftp_Hand *)r_malloc(sizeof(Ftp_Hand));
	
	if(NULL == pFtpServerHand->FtpHand)
	{
		zlog_error(c, "FtpServiceInit: malloc Ftp_Hand fail");
		return -1;
	}
	
	r_memset(pFtpServerHand->FtpHand, 0x0, sizeof(Ftp_Hand));
	
	pFtpServerHand->config = (Upload_Info *)r_malloc(sizeof(Upload_Info));
	
	if(NULL == pFtpServerHand->config)
	{
		zlog_error(c, "FtpServiceInit: malloc config fail");
		return -1;
	}
	
	r_memset(pFtpServerHand->config, 0x0, sizeof(Upload_Info));
	
	
	pFtpServerHand->msgid = r_msg_create_key((int8_t *)"opt", 10000);
	
	if(-1 == pFtpServerHand->msgid)
	{
		zlog_error(c, "FtpServiceInit: create msgqueue fail");
		return -1;
	}
	
	printf("pFtpServerHand->msgid = %d\n", pFtpServerHand->msgid);
	
	pFtpServerHand->px = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	
	if(NULL == pFtpServerHand->px)
	{
		zlog_error(c, "FtpServiceInit: malloc parse_xml_t fail");
		return -1;
	}
	
	r_memset(pFtpServerHand->px, 0x0, sizeof(parse_xml_t));
	
	if(0 != TcpSockLockInit())
	{
		zlog_error(c, "FtpServiceInit: malloc socklock fail");
		return -1;
	}
	
	return 1;
}

static int DeFtpServiceInit(FtpService *pFtpServerHand)
{

	if(pFtpServerHand != NULL)
	{
		pFtpServerHand->sendflg = 0;		//可以发生定时器
		pFtpServerHand->ClientSocket = -1;	//sokcet初始化
		
		/* ftp 销毁 */
		if(NULL != pFtpServerHand->FtpHand)
		{
			pFtpServerHand->FtpHand->Ftp_DeInit(pFtpServerHand->FtpHand);
			r_free(pFtpServerHand->FtpHand);
			pFtpServerHand->FtpHand = NULL;
		}

		if(NULL != pFtpServerHand->config)
		{
			r_free(pFtpServerHand->config);
			pFtpServerHand->config = NULL;
		}
		/* 上传模块销毁 */
		if(pFtpServerHand->px != NULL)
		{
			DeUploadInit(pFtpServerHand->px);

			r_free(pFtpServerHand->px);
			pFtpServerHand->px = NULL;
		}
	
		TcpSockLockdeInit();
		
		msgctl(pFtpServerHand->msgid, IPC_RMID, NULL);

		r_free(pFtpServerHand);
		pFtpServerHand = NULL;
	}
	
	/* 日志模块销毁 */
	zlog_fini();

	/* xml销毁 */
	xmlCleanupParser();
	return -1;
}



static int32_t analyze_ftp_config(int8_t *recv_buf)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr usrname = NULL;
	xmlNodePtr password = NULL;
	xmlChar *pusrname = NULL;
	xmlChar *ppassword = NULL;
	
	zlog_category_t *c = pFtpServerHand->c;
	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	
	if(parse_xml_user == NULL)
	{
		zlog_error(c, "analyze_ftp_config: malloc parse_xml_t fail");
		return -1;
	}
	
	
	if(NULL == init_dom_tree(parse_xml_user, (char *)recv_buf))
	{
	
		zlog_error(c, "analyze_ftp_config: init_dom_tree fail");
		goto EXIT;
	}
	
	if(is_req_msg(parse_xml_user->proot) != 1)
	{
		zlog_error(c, "analyze_ftp_config: is_req_msg fail");
		goto EXIT;
	}
	
	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	
	if(msgbody == NULL)
	{
		zlog_error(c, "analyze_ftp_config: msghead fail");
		goto EXIT;
	}
	
	usrname   = get_children_node(msgbody, BAD_CAST "usrname");
	
	if(usrname == NULL)
	{
		zlog_error(c, "analyze_ftp_config: not found usrname");
		goto EXIT;
	}
	
	pusrname  = xmlNodeListGetString(parse_xml_user->pdoc, usrname->xmlChildrenNode, 1);
	
	if(pusrname == NULL)
	{
		zlog_error(c, "analyze_ftp_config: not found usrname");
		goto EXIT;
	}
	
	password  = get_children_node(msgbody, BAD_CAST "password");
	
	if(password == NULL)
	{
		zlog_error(c, "analyze_ftp_config: not found password");
		goto EXIT;
	}
	
	ppassword = xmlNodeListGetString(parse_xml_user->pdoc, password->xmlChildrenNode, 1);
	
	if(ppassword == NULL)
	{
		zlog_error(c, "analyze_ftp_config: not found password");
		goto EXIT;
	}
	
	
	//pFtpServerHand->config->ConnecTimeout = 10;
	//pFtpServerHand->config->IsCreateDirs  = 1;
	r_strcpy((int8_t *)pFtpServerHand->config->UsrName,  (int8_t *)pusrname);
	r_strcpy((int8_t *)pFtpServerHand->config->PassWord, (int8_t *)ppassword);
	
	//config.FtpPort       = atoi(pport);
	
	return_code = 1;
	
EXIT:

	if(NULL != pusrname)
	{
		xmlFree(pusrname);
	}
	
	if(NULL != ppassword)
	{
		xmlFree(ppassword);
	}
	
	
	if(parse_xml_user->pdoc != NULL)
	{
		release_dom_tree(parse_xml_user->pdoc);
	}
	
	r_free(parse_xml_user);
	
	if(1 != return_code)
	{
		zlog_error(c, "%s",recv_buf);
	}
	return return_code;
}

static int32_t analyze_ftp_upload(int8_t *recv_buf, int8_t *dir, int8_t *path,int8_t *ip ,int8_t *ftpport, int8_t *precid)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	xmlNodePtr msgbody;
	xmlNodePtr localpath;
	xmlNodePtr romtepath;
	xmlNodePtr romteip;
	xmlNodePtr port;
	xmlNodePtr recid;
	
	xmlChar *plocalpath = NULL;
	xmlChar *promteip = NULL;
	xmlChar *promtepath = NULL;
	xmlChar *pport = NULL;
	xmlChar *pprecid = NULL;

	zlog_category_t *c = pFtpServerHand->c;
	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	
	if(parse_xml_user == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: malloc parse_xml_t fail");
		return -1;
	}
	
	if(NULL == init_dom_tree(parse_xml_user, (const char *)recv_buf))
	{
	
		zlog_error(c, "Analyze_ftp_upload: init_dom_tree fail");
		goto EXIT;
	}
	
	
	if(is_req_msg(parse_xml_user->proot) != 1)
	{
		zlog_error(c, "Analyze_ftp_upload: is_req_msg");
		goto EXIT;
	}
	
	msgbody   = get_children_node(parse_xml_user->proot, BAD_CAST "MsgBody");
	
	if(msgbody == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found msghead");
		goto EXIT;
	}
	
	localpath   = get_children_node(msgbody, BAD_CAST "UpLoadLocalPath");
	
	if(localpath == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found UpLoadLocalPath");
		goto EXIT;
	}
	
	plocalpath  = xmlNodeListGetString(parse_xml_user->pdoc, localpath->xmlChildrenNode, 1);
	
	if(plocalpath == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found UpLoadLocalPath");
		goto EXIT;
	}


	
	romteip  = get_children_node(msgbody, BAD_CAST "UpLoadServerIp");
	
	if(romteip == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found UpLoadServerIp");
		goto EXIT;
	}
	
	promteip = xmlNodeListGetString(parse_xml_user->pdoc, romteip->xmlChildrenNode, 1);
	
	if(promteip == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found UpLoadServerIp");
		goto EXIT;
	}


	romtepath  = get_children_node(msgbody, BAD_CAST "UpLoadRomtePath");
	
	if(romtepath == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found UpLoadRomtePath");
		goto EXIT;
	}
	
	promtepath = xmlNodeListGetString(parse_xml_user->pdoc, romtepath->xmlChildrenNode, 1);
	
	if(promtepath == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found UpLoadRomtePath");
	//	goto EXIT;
	}

	
	port  = get_children_node(msgbody, BAD_CAST "Port");
	
	if(port == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found port");
		goto EXIT;
	}
	
	pport     = xmlNodeListGetString(parse_xml_user->pdoc, port->xmlChildrenNode, 1);
	
	if(pport == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found port");
		goto EXIT;
	}

	recid  = get_children_node(msgbody, BAD_CAST "RecordID");
	
	if(recid == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found RecordID");
		goto EXIT;
	}
	
	pprecid     = xmlNodeListGetString(parse_xml_user->pdoc, recid->xmlChildrenNode, 1);
	
	if(pprecid == NULL)
	{
		zlog_error(c, "Analyze_ftp_upload: not found RecordID");
		goto EXIT;
	}

	
	r_strcpy(dir, (int8_t *)plocalpath);
	r_strcpy(ip, (int8_t *)promteip);
	if(promtepath != NULL)
	{
		r_strcpy(path, (int8_t *)promtepath);
	}
	else
	{
		r_strcpy(path, (int8_t *)"/");
	}
	
	r_strcpy(ftpport, (int8_t *)pport);
	r_strcpy(precid,(int8_t*)pprecid);
	return_code = 1;
	
EXIT:

	if(plocalpath != NULL)
	{
		xmlFree(plocalpath);
	}
	
	if(promteip != NULL)
	{
		xmlFree(promteip);
	}

	if(promtepath != NULL)
	{
		xmlFree(promtepath);
	}
	
	if(pport != NULL)
	{
		xmlFree(pport);
	}

	if(pprecid != NULL)
	{
		xmlFree(pprecid);
	}
	
	if(parse_xml_user->pdoc != NULL)
	{
		release_dom_tree(parse_xml_user->pdoc);
	}
	
	r_free(parse_xml_user);
	if(1 != return_code)
	{
		zlog_error(c, "%s",recv_buf);
	}
	return return_code;
}

static int32_t package_add_xml_leaf(xmlNodePtr child_node, xmlNodePtr far_node, char *key_name, char *key_value)
{
	child_node = xmlNewNode(NULL, BAD_CAST key_name);
	xmlAddChild(far_node, child_node);
	xmlAddChild(child_node, xmlNewText(BAD_CAST key_value));
	return 1;
}

static int32_t ftp_uoload_fail(char *send_buf, char *localfile, char *reason)
{
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");
	
	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"ResponseMsg");
	xmlDocSetRootElement(doc, root_node);
	
	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr temp 				= NULL;
	
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	package_add_xml_leaf(temp, head_node, "MsgCode", "");
	package_add_xml_leaf(temp, head_node, "PassKey", "");
	
	
	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);
	
	package_add_xml_leaf(temp, body_node, "FaileFile", localfile);
	package_add_xml_leaf(temp, body_node, "Reason", reason);
	
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

static int32_t ftp_uoload_success(char *send_buf, char *localfile, char *precid, char *romteip, char *romotepath)
{
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");
	
	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"ResponseMsg");
	xmlDocSetRootElement(doc, root_node);
	
	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr temp 				= NULL;
	
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	package_add_xml_leaf(temp, head_node, "MsgCode", "");
	package_add_xml_leaf(temp, head_node, "PassKey", "ftpserver");
	
	
	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);
	
	package_add_xml_leaf(temp, body_node, "SuccessFile", localfile);

	if(precid == NULL)
	{
		package_add_xml_leaf(temp, body_node, "RecordID",  "");
	}
	else
	{
		package_add_xml_leaf(temp, body_node, "RecordID",  precid);
	}

	
	package_add_xml_leaf(temp, body_node, "UpLoadServerIp",  romteip);

	if(NULL == romotepath)
	{
		package_add_xml_leaf(temp, body_node, "UpLoadRomtePath",  "");
	}
	else
	{
		package_add_xml_leaf(temp, body_node, "UpLoadRomtePath",  romotepath);
	}
	
	
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
static int32_t test_volume_rep_msg(char *send_buf, char *state, char *volume)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");
	
	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"ReponseMsg");
	xmlDocSetRootElement(doc, root_node);
	
	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr volume_node 			= NULL;
	xmlNodePtr temp 				= NULL;
	
	
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	
	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);
	
	
	package_add_xml_leaf(temp, head_node, "MsgCode", "30019");
	package_add_xml_leaf(temp, head_node, "ReturnCode", state);
	
	volume_node = xmlNewNode(NULL, BAD_CAST "VoiceInfo");
	xmlAddChild(body_node, volume_node);
	
	package_add_xml_leaf(temp, volume_node, "Volume", volume);
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);
	
	xmlFree(temp_xml_buf);
	
	return size;
}
static int32_t test_state_msg(char *send_buf)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");
	
	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"ReponseMsg");
	xmlDocSetRootElement(doc, root_node);
	
	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr recstate 			= NULL;
	xmlNodePtr recIp 				= NULL;
	xmlNodePtr room 				= NULL;
	xmlNodePtr temp                 = NULL;
	
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	
	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);
	
	
	package_add_xml_leaf(temp, head_node, "MsgCode", "30024");
	package_add_xml_leaf(temp, head_node, "PassKey", "ComControl");
	
	recstate = xmlNewNode(NULL, BAD_CAST "RecServerStatusUpdateReq");
	xmlAddChild(body_node, recstate);
	
	recIp = xmlNewNode(NULL, BAD_CAST "RecServerIP");
	xmlAddChild(recstate, recIp);
	room = xmlNewNode(NULL, BAD_CAST "RoomStatus");
	xmlAddChild(recstate, room);
	
	package_add_xml_leaf(temp, room , "RoomId", "0");
	package_add_xml_leaf(temp, room , "ConnStatus", "0");
	package_add_xml_leaf(temp, room , "Quality", "0");
	package_add_xml_leaf(temp, room , "RecStatus", "2");
	package_add_xml_leaf(temp, room , "RecName", "0");
	package_add_xml_leaf(temp, room , "IfMark", "0");
	package_add_xml_leaf(temp, room , "Status1", "0");
	package_add_xml_leaf(temp, room , "Status2", "1");
	package_add_xml_leaf(temp, room , "Status3", "0");
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);
	
	xmlFree(temp_xml_buf);
	
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
	xmlNodePtr temp 				= NULL;
	
	
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	
	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);
	
	
	package_add_xml_leaf(temp, head_node, "MsgCode", "10009");
	package_add_xml_leaf(temp, head_node, "PassKey", "ComControl");
	
	warnning = xmlNewNode(NULL, BAD_CAST "warn");
	xmlAddChild(body_node, warnning);
	
	package_add_xml_leaf(temp, warnning, "id", cmd);
	package_add_xml_leaf(temp, warnning, "source", "");
	package_add_xml_leaf(temp, warnning, "roomid", "");
	package_add_xml_leaf(temp, warnning, "codecid", "");
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);
	
	xmlFree(temp_xml_buf);
	
	return size;
}

static int32_t test_rep_msg(char *send_buf, char *state)
{

	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");
	
	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"ResponseMsg");
	xmlDocSetRootElement(doc, root_node);
	
	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr volume_node 			= NULL;
	xmlNodePtr temp 				= NULL;
	
	
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	
	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);
	
	
	package_add_xml_leaf(temp, head_node, "MsgCode", "30010");
	package_add_xml_leaf(temp, head_node, "ReturnCode", state);
	
	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	r_memcpy(send_buf, temp_xml_buf, size);
	
	xmlFree(temp_xml_buf);
	
	return size;
}
//--------------------------------------------------------------测试------------------------------------------------------//

#endif

int Ftp_Report_Info(char *localfile, char *reason, char *romteip, char *romtepath, int falg)
{
	char buffer[MAXXMLSIZE] = {0};
	int size = 0;
	int nameLength = -1;
	MsgHeader *pmsg = (MsgHeader *)buffer;
	zlog_category_t *c = pFtpServerHand->c;
	
	/* 上报成功消息 */
	if(1 == falg)
	{
		pmsg->sMsgType = FTP_COM_MSGTYPE_UPLOAD_SUCCESS;
		size = ftp_uoload_success(buffer + MSGLEN, localfile, reason, romteip,romtepath);
	}
	else
		if(0 == falg)
		{
			pmsg->sMsgType = FTP_COM_MSGTYPE_UPLOAD_ERROR;
			size = ftp_uoload_fail(buffer + MSGLEN, localfile, reason);
		}
		else
		{
			return -1;
		}

	uint16_t sLen = sizeof(MsgHeader) + size;
	pmsg->sLen     = r_htons(sLen);
	pmsg->sMsgType = r_htons(pmsg->sMsgType);
	pmsg->sVer =     r_htons(2012);
	TcpSockLock();
	// 发送文件名
	int	sClientSocket = GetTcpSocket();
	
	if(sClientSocket != -1)
	{
		nameLength = tcp_send_longdata(sClientSocket, buffer, sLen);
		
		if(nameLength < 0)
		{
			zlog_error(c,"%s",buffer + MSGLEN);
		}
		else
		{
			zlog_debug(c,"%s",buffer + MSGLEN);
		}
	}
	
	TcpSockunLock();
	return 1;
}
static void *ServeTask(void *args)
{
	int32_t	msgid = *(int32_t *)args;
	zlog_category_t *c = pFtpServerHand->c;
	zlog_debug(c, "ServeTask: create!!!");
	struct sockaddr_in		SrvAddr, ClientAddr;
	int32_t	ServSock = -1;
	int32_t nLen = 0;
	int32_t opt  = 1;
	MsgHeader *pmsg = NULL;
	int32_t	sClientSocket = -1;
	struct timeval timeout;
SERVERSTARTRUN:

	r_bzero(&SrvAddr, sizeof(struct sockaddr_in));
	SrvAddr.sin_family = AF_INET;
	SrvAddr.sin_port = r_htons(SERVERPORTNUM);
	SrvAddr.sin_addr.s_addr = r_htonl(INADDR_ANY);
	ServSock = r_socket(AF_INET, SOCK_STREAM, 0);
	
	if(ServSock < 0)
	{
		zlog_error(c, "ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		r_sleep(3);
		goto SERVERSTARTRUN;
	}
	
	r_setsockopt(ServSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	if(r_bind(ServSock, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0)
	{
		zlog_error(c, "bind error:%d,error msg: = %s\n", errno, strerror(errno));
		r_sleep(3);
		goto SERVERSTARTRUN;
	}
	
	if(r_listen(ServSock, 1) < 0)
	{
		zlog_error(c, "listen error:%d,error msg: = %s", errno, strerror(errno));
		r_sleep(3);
		goto SERVERSTARTRUN;
	}
	
#if 0
	
	if((ret = fcntl(ServSock, F_GETFL, 0)) == -1)
	{
		fprintf(stderr, "fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		assert(0);
	}
	
	ret = fcntl(ServSock, F_SETFL, ret | O_NONBLOCK);
	
	if(ret < 0)
	{
		fprintf(stderr, "fcntl F_SETFL failed, err msg: %s\n", strerror(errno));
		assert(0);
	}
	
	set_send_timeout(ServSock, FTP_COM_TCP_SOCKET_TIMEOUT);
	set_recv_timeout(ServSock, FTP_COM_TCP_SOCKET_TIMEOUT);
#endif
	
	while(1)
	{
		zlog_debug(c,"--------------wait accept-------------------\n");
		/* 设置socket 为无效 */
		SetTcpSocket(-1);
		r_memset(&ClientAddr, 0, sizeof(struct sockaddr_in));
		nLen = sizeof(struct sockaddr_in);
		sClientSocket = r_accept(ServSock, (void *)&ClientAddr, (unsigned int *)&nLen);
		
		if(sClientSocket > 0)
		{
			int nSize = 1;
			int result;
			zlog_debug(c, "ServeTask: accept socket success!");
			SetTcpSocket(sClientSocket);
			
			if((r_setsockopt(sClientSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize,
			                 sizeof(nSize))) == -1)
			{
				zlog_error(c, "ServeTask: setsockopt failed");
			}
			
			nSize  = 0;
			nLen   = sizeof(nLen);
			result = r_getsockopt(sClientSocket, SOL_SOCKET, SO_SNDBUF, &nSize , (unsigned int *)&nLen);
			
			if(result)
			{
				zlog_error(c, "ServeTask: getsockopt() errno:%d socket:%d  result:%d\n", errno, sClientSocket, result);
			}
			
			nSize = 1;
			
			if(r_setsockopt(sClientSocket, IPPROTO_TCP, TCP_NODELAY, &nSize , sizeof(nSize)))
			{
				zlog_error(c, "ServeTask:Setsockopt error%d\n", errno);
			}

			set_recv_timeout(sClientSocket, FTP_COM_TCP_SOCKET_TIMEOUT);
			set_send_timeout(sClientSocket, FTP_COM_TCP_SOCKET_TIMEOUT);
			
			while(1)
			{
			//	printf("-------server-------\n");
				sClientSocket = GetTcpSocket();
				fd_set readfd;
				int seret = 0;
				timeout.tv_sec = SELECTTIMEOUT;
				FD_ZERO(&readfd);
				FD_SET(sClientSocket, &readfd);
				seret = r_select(sClientSocket + 1, &readfd, NULL, NULL, &timeout);
				if(seret > 0)
					// FD已准备好
				{
					if(FD_ISSET(sClientSocket, &readfd))
					{
						int8_t szData[MAXXMLSIZE] = {0};
						int32_t len = 0;
						r_memset(szData, 0x0, MAXXMLSIZE);
						TcpSockLock();
						
						len = tcp_recv_longdata(sClientSocket, (char *)szData, MSGLEN);
						TcpSockunLock();
						if(len != MSGLEN)
						{
							SetTcpSocket(-1);
							r_close(sClientSocket);
							r_usleep(1000);		
							break;
						}
					
						pmsg = (MsgHeader *)szData;
						pmsg->sVer = r_ntohs(pmsg->sVer);
						if(pmsg->sVer != 2012)
						{
							continue;
						}
						
						pmsg->sLen     = r_ntohs(pmsg->sLen);
						pmsg->sMsgType = r_ntohs(pmsg->sMsgType);
					
						TcpSockLock();
						len = tcp_recv_longdata(sClientSocket, (char *)szData + MSGLEN, pmsg->sLen - MSGLEN);
						TcpSockunLock();
						if(len != (pmsg->sLen - MSGLEN))
						{
							SetTcpSocket(-1);
							r_close(sClientSocket);
							r_usleep(1000);		
							break;
						}
						
						zlog_debug(c, "%s",szData + MSGLEN);
						switch(pmsg->sMsgType)
						{
							
							/* 解析ftp配置 */
							case FTP_COM_MSGTYPE_SERVER_CONFIG:
							{
								zlog_debug(c, "ServeTask: Recv Msg MSGCODE_CONFIG_FTP");
								
								if(1 != analyze_ftp_config(szData + MSGLEN))
								{
									Ftp_Report_Info("FtpConfigFail", "analyze config comnd fail",NULL,NULL, 0);
									zlog_debug(c, "FtpConfigFail: analyze config comnd fail");
								}
							}
							break;
							
							/* 解析上传目录 */
							case FTP_COM_MSGTYPE_SERVER_UPLOAD_FILE:
							{
								int8_t plocalpath[MAX_FILE_PATH_LEN] = {0};
								int8_t promtepath[MAX_FILE_PATH_LEN] = {0};
								int8_t promteip[MAX_FILE_PATH_LEN] = {0};
								int8_t precid[MAX_FILE_PATH_LEN] = {0};
								int8_t pport[5] = {0};
								
								zlog_debug(c, "ServeTask: Recv Msg MSGCODE_UPLOAD_DIR");
								
								if(1 == analyze_ftp_upload(szData + MSGLEN, plocalpath, promtepath,promteip ,pport,precid))
								{
									if(1 != AddXmlNode(pFtpServerHand->px, pFtpServerHand->config,\
										(char *)plocalpath,(char *) promtepath,(char *) promteip,(char *) pport, (char *)precid))
									{
										Ftp_Report_Info((char *)plocalpath, "ServeTask: AddXmlNode isfail",NULL,NULL, 0);
										zlog_error(c, "AddXmlNode fail %s", plocalpath);
									}
									else
									{
										msgque  msgp;
										msgp.msgtype = 2;
										
										zlog_debug(c, "localpath[%s] romtepath[%s]",plocalpath,promtepath);
										
										if(0 != r_msg_send(msgid, &msgp,  sizeof(msgque) - sizeof(long), 0))
										{
											zlog_debug(c, "ServeTask: send Msg MSGCODE_UPLOAD_DIR to DealTask fail");
											Ftp_Report_Info((char *)plocalpath, "ServeTask: send Msg MSGCODE_UPLOAD_DIR to DealTask fail",NULL,NULL, 0);
										}
									}
								}
								else
								{
									zlog_error(c, "analyze_ftp_upload fail");
									Ftp_Report_Info("comnd analyze", "analyze upload comnd fail",NULL,NULL, 0);
								}
								
								break;
							}
							
							default
									:
								break;
						}
					}
				}
				else
					if(seret == 0)
						// 超时
					{
						zlog_info(c, "ServeTask: Select Time Out < 0");
						r_usleep(1000);
						continue;
					}
					else
						if(seret < 0)
							// 异常
						{
							zlog_error(c, "ServeTask: Select SERET < 0");
							r_usleep(1000);
							r_close(sClientSocket);
							SetTcpSocket(-1);
							break;
						}
						
			}
		}
		else
		{
			if(errno == ECONNABORTED || errno == EAGAIN)
				//软件原因中断
			{
				r_usleep(100000);
				continue;
			}
			
			if(ServSock > 0)
			{
				r_close(ServSock);
			}
			
			goto SERVERSTARTRUN;
		}
	}

	return NULL;
}



static void *DealTask(void *args)
{

	int32_t	msgid = *(int32_t *)args;
	msgque	revmsg;
	int32_t recvlen = 0;
	zlog_category_t *c = pFtpServerHand->c;
	zlog_debug(c, "DealTask: create!!!");

	while(1)
	{
		recvlen = r_msg_recv(msgid, &revmsg, sizeof(msgque) - sizeof(long), (long)0, 0);
		
		if(recvlen < 0)
		{
			zlog_debug(c, "package msgrcv failed, errmsg = %s, recvlen = %d, errno = %d\n", strerror(errno), recvlen, errno);
			r_sleep(1);
			continue;
		}
		
		switch(revmsg.msgtype)
		{
		
			case 1:
				break;
				
				/* 定时器消息 检测XML */
			case 2:
			{
				pFtpServerHand->sendflg = 0;
				zlog_debug(c, "DealTask: Began to handle tasks!!!");
				/* 上传XML中文件 */
				UpLoadFile(pFtpServerHand->px, pFtpServerHand->FtpHand, pFtpServerHand->config ,Ftp_Report_Info);
				
				pFtpServerHand->sendflg = 1;
			}
			
			
			break;
			
			default
					:
				break;
		}
		
	}
	return NULL;
}


static void *TimeTask(void *args)
{
	int32_t	msgid = *(int32_t *)args;
	msgque  msgp;
	zlog_category_t *c = pFtpServerHand->c;
	zlog_debug(c, "TimeTask: create!!!");
	
	while(1)
	{
		r_sleep(360);
		
		msgp.msgtype = 2;
		
		if(pFtpServerHand->sendflg == 1)
		{
			r_msg_send(msgid, &msgp,  sizeof(msgque) - sizeof(long), 0);
		}
		
	}
	return NULL;
}

static int32_t ReadXml(FtpService *pFtpServerHand)
{
	xmlNodePtr node;
	if((pFtpServerHand == NULL) || (pFtpServerHand->px == NULL) || (pFtpServerHand->px->proot == NULL))
	{
		return -1;
	}

	/* IP */
	node = get_children_node(pFtpServerHand->px->proot, (xmlChar *)"RomteIp");
	if(node)
	{
		r_memset(pFtpServerHand->config->RomteIp, 0, MAX_FILE_PATH_LEN);
		get_current_node_value((char *)pFtpServerHand->config->RomteIp, MAX_FILE_PATH_LEN, pFtpServerHand->px->pdoc, node);
	}
	else
	{
		r_strcpy((int8_t*)pFtpServerHand->config->RomteIp, (int8_t *)"0.0.0.0");
	}

	node = get_children_node(pFtpServerHand->px->proot, (xmlChar *)"RomtePath");
	if(node)
	{
		r_memset(pFtpServerHand->config->RomtePath, 0, MAX_FILE_PATH_LEN);
		get_current_node_value((char *)pFtpServerHand->config->RomtePath, MAX_FILE_PATH_LEN, pFtpServerHand->px->pdoc, node);
	}
	else
	{
		r_strcpy((int8_t*)pFtpServerHand->config->RomtePath, (int8_t *)"/");
	}

	node = get_children_node(pFtpServerHand->px->proot, (xmlChar *)"Port");
	if(node)
	{
		r_memset(pFtpServerHand->config->Port, 0, MAX_USR_PASSWORD);
		get_current_node_value((char *)pFtpServerHand->config->Port, MAX_USR_PASSWORD, pFtpServerHand->px->pdoc, node);
	}
	else
	{
		r_strcpy((int8_t*)pFtpServerHand->config->Port, (int8_t *)"21");
	}

	node = get_children_node(pFtpServerHand->px->proot, (xmlChar *)"User");
	if(node)
	{
		r_memset(pFtpServerHand->config->UsrName, 0, MAX_USR_PASSWORD);
		get_current_node_value((char *)pFtpServerHand->config->UsrName, MAX_USR_PASSWORD, pFtpServerHand->px->pdoc, node);
	}
	else
	{
		r_strcpy((int8_t*)pFtpServerHand->config->UsrName, (int8_t *)"user");
	}

	node = get_children_node(pFtpServerHand->px->proot, (xmlChar *)"Passwd");
	if(node)
	{
		r_memset(pFtpServerHand->config->PassWord, 0, MAX_USR_PASSWORD);
		get_current_node_value((char *)pFtpServerHand->config->PassWord, MAX_USR_PASSWORD, pFtpServerHand->px->pdoc, node);
	}
	else
	{
		r_strcpy((int8_t*)pFtpServerHand->config->PassWord, (int8_t *)"passwd");
	}

	node = get_children_node(pFtpServerHand->px->proot, (xmlChar *)"MaxSpeed");
	if(node)
	{
		int8_t buffer[1024] = {0};
		//r_memset(pFtpServerHand->config->MaxSpeed, 0, MAX_USR_PASSWORD);
		get_current_node_value((char *)buffer, MAX_USR_PASSWORD, pFtpServerHand->px->pdoc, node);
		pFtpServerHand->config->MaxSpeed = atoi(buffer);
		
		if((pFtpServerHand->config->MaxSpeed < 10240) && (pFtpServerHand->config->MaxSpeed > 0))
		{
			pFtpServerHand->config->MaxSpeed  = 10240;
		}
		else if(pFtpServerHand->config->MaxSpeed == 0)
		{
			//不限速
			pFtpServerHand->config->MaxSpeed  = 0;
		}
	}
	else
	{
		pFtpServerHand->config->MaxSpeed = 2097152;
	}
	return 1;
}


int main()
{
	int ret = -1;
	r_setpriority(PRIO_PROCESS, 0, -20);
	pthread_attr_t		attr;
	/*取消PIPE坏的信号*/
	Signal(SIGPIPE, SIG_IGN);
	
	//app_weblisten_init();
	//sleep(10000);
	pFtpServerHand = (FtpService *)r_malloc(sizeof(FtpService));
	
	if(NULL == pFtpServerHand)
	{
		printf("malloc FtpServerHand fail!!!\n");
		return -1;
	}
	
	r_memset(pFtpServerHand, 0 , sizeof(FtpService));
	
	if(-1 == FtpServiceInit(pFtpServerHand))
	{
		printf("FtpServiceInit fail!!!\n");
		return -1;
	}

	
	zlog_category_t *c = pFtpServerHand->c;
	
	/* 注册ftp模块 */
	Ftp_Register(pFtpServerHand->FtpHand);
	
	/* 初始化ftp模块 */
	ret = pFtpServerHand->FtpHand->Ftp_Init(pFtpServerHand->FtpHand);
	//assert(1 == ret);
	
	ret = pFtpServerHand->FtpHand->Ftp_BaseConfig(pFtpServerHand->FtpHand);
	//assert(1 == ret);
	
	/* 上传模块初始化 */
	if(1 != UploadInit(pFtpServerHand->px))
	{
		zlog_error(c, "main: UploadInit init fail");

		return -1;
	}
	
	ret = ReadXml(pFtpServerHand);

	if(ret != 1)
	{	
		zlog_error(c, "main: ReadXml fail");
		return -1;
	}

	zlog_error(c, "FtpVersion [%s]",FTPVERSION);
	zlog_error(c, "Ip[%s] Path[%s] Port[%s] User[%s] Passwd[%s] MaxSpeed[%u]\n",\
		pFtpServerHand->config->RomteIp,pFtpServerHand->config->RomtePath,pFtpServerHand->config->Port,\
		pFtpServerHand->config->UsrName,pFtpServerHand->config->PassWord,pFtpServerHand->config->MaxSpeed);

	#if 1
	ret = r_pthread_attr_init(&attr);
	
	if(ret != 0)
	{
		zlog_error(c, "Main: pthread_attr_init failed, errmsg = %s\n", strerror(errno));
		
		return -1;
	}

	
	ret = r_pthread_create(&pFtpServerHand->servertaskid, &attr, ServeTask, &pFtpServerHand->msgid);
	
	if(ret != 0)
	{
		zlog_error(c, "Main: ServeTask create failed, ret = %d, errmsg = %s\n", ret, strerror(errno));
		
		return -1;
	}
	
	
	ret = r_pthread_create(&pFtpServerHand->dealtaskid, &attr, DealTask, &pFtpServerHand->msgid);
	
	if(ret != 0)
	{
		zlog_error(c, "Main: DealTask create failed, ret = %d, errmsg = %s\n", ret, strerror(errno));
		return -1;
	}
	
	ret = r_pthread_create(&pFtpServerHand->timetaskid, &attr, TimeTask, &pFtpServerHand->msgid);
	
	if(ret != 0)
	{
		zlog_error(c, "Main: TimeTask create failed, ret = %d, errmsg = %s\n", ret, strerror(errno));
		return -1;
	}
	
	r_pthread_attr_destroy(&attr);
	r_pthread_join(pFtpServerHand->servertaskid, NULL);
	r_pthread_join(pFtpServerHand->timetaskid, NULL);
	r_pthread_join(pFtpServerHand->dealtaskid, NULL);
	#endif
	
	DeFtpServiceInit(pFtpServerHand);
	printf("------close------\n");
	return 1;
}

