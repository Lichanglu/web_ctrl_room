/*
 * =====================================================================================
 *
 *       Filename:  ftpcom.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2012年11月1日 09时12分18秒
 *       Revision:
 *       Compiler:  gcc
 *
 *         Author:  黄海洪
 *        Company:  深圳锐取信息技术股份有限公司
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "ftpcom.h"
#include "control.h"
#include "control_log.h"
#include "common.h"
#include "xml/xml_base.h"
#include "xml_msg_management.h"
#include "http/mid_http.h"
#include "command_resolve.h"
#include "timeout_process.h"


//#define FTP_COM_IPADDR								("192.168.4.45")
#define FTP_COM_IPADDR								("127.0.0.1")

#define FTP_COM_PORT								(16666)
#define FTP_COM_TCP_SOCKET_TIMEOUT					(3)
#define FTP_COM_MSGHEAD_LEN							(sizeof(MsgHeader))

#define FTP_COM_STOPED								(0)
#define FTP_COM_RUNNING								(1)
#define FTP_COM_PREPARING							(2)

#define FTP_COM_OFFLINE								(0)
#define FTP_COM_ONLINE								(1)


#define MAX_XMNL_LEN                                (4*1024)

#define FTP_COM_MAX_EVERY_PACKET_LENGTH				(65535)


int32_t ftpcom_init_env(ftpcom_env *pftp)
{
	if(NULL == pftp)
	{
		zlog_error(DBGLOG, "---[ftpcom_init_env] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_init(&(pftp->ftp_m), NULL);
	pthread_mutex_init(&(pftp->socket_m), NULL);

	r_memset(pftp->ipaddr, 0, 64);
	r_memset(pftp->buf, 0, FTP_COM_DATA_LEN);
	pftp->port = 0;
	pftp->socket = -1;
	pftp->login = FTP_COM_OFFLINE;
	pftp->pserset = NULL;
	pftp->run_status = FTP_COM_PREPARING;

	return 0;
}

int32_t ftpcom_set_running(ftpcom_env *pftp)
{
	if(NULL == pftp)
	{
		zlog_error(DBGLOG, "---[ftpcom_set_login] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(pftp->ftp_m));
	pftp->run_status = FTP_COM_RUNNING;
	pthread_mutex_unlock(&(pftp->ftp_m));

	return 0;
}


int32_t ftpcom_set_server_port(ftpcom_env *pftp, uint16_t port)
{
	if(NULL == pftp)
	{
		zlog_error(DBGLOG, "---[ftpcom_set_server_port] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(pftp->ftp_m));
	pftp->port = port;
	pthread_mutex_unlock(&(pftp->ftp_m));

	return 0;
}

int32_t ftpcom_set_server_ip(ftpcom_env *pftp, const int8_t *ipaddr)
{
	if(NULL == pftp)
	{
		zlog_error(DBGLOG, "---[ftpcom_set_server_ip] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(pftp->ftp_m));
	r_strcpy(pftp->ipaddr, ipaddr);
	pthread_mutex_unlock(&(pftp->ftp_m));

	return 0;
}

int32_t ftpcom_set_login(ftpcom_env *pftp)
{
	if(NULL == pftp)
	{
		zlog_error(DBGLOG, "---[ftpcom_set_login] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(pftp->ftp_m));
	pftp->login = FTP_COM_ONLINE;
	pthread_mutex_unlock(&(pftp->ftp_m));

	return 0;
}

int32_t ftpcom_set_logout(ftpcom_env *pftp)
{
	if(NULL == pftp)
	{
		zlog_error(DBGLOG, "---[ftpcom_set_logout] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(pftp->ftp_m));
	pftp->login = FTP_COM_OFFLINE;
	pthread_mutex_unlock(&(pftp->ftp_m));

	return 0;
}

int32_t ftpcom_check_login_status(ftpcom_env *pftp)
{
	int32_t status = 0;

	if(NULL == pftp)
	{
		zlog_error(DBGLOG, "---[ftpcom_set_logout] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(pftp->ftp_m));
	status = pftp->login;
	pthread_mutex_unlock(&(pftp->ftp_m));

	return status;
}

int32_t ftpcom_set_server_set(ftpcom_env *pftp, server_set *pser)
{
	if(NULL == pftp)
	{
		zlog_error(DBGLOG, "ftpcom_set_server_set faild, handle is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&pftp->ftp_m);
	pftp->pserset = pser;
	pthread_mutex_unlock(&pftp->ftp_m);

	return 0;
}

/* 解析ftp返回失败消息 */
static int32_t analyze_ftp_respfaile(int8_t *recv_buf, int8_t *file_buf, int8_t *reason_buf)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr filename = NULL;
	xmlNodePtr reason = NULL;

	int32_t ret 	= 0;
	int8_t  pbuf[XML_VALUE_MAX_LENGTH]  = {0};


	int8_t *pfilename = NULL;
	int8_t *preason = NULL;

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL || NULL == file_buf || NULL == reason_buf)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: malloc parse_xml_t fail\n");
		return -1;
	}

	zlog_debug(DBGLOG, "%s\n", recv_buf);

	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: init_dom_tree fail\n");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: is_req_msg fail\n");
		goto EXIT;
	}

	msgbody = get_children_node(parse_xml_user->proot, MSG_BODY_KEY);
	if(msgbody == NULL)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: msghead fail\n");
		goto EXIT;
	}

	filename = get_children_node(msgbody, BAD_CAST "FaileFile");
	if(filename == NULL)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: not found usrname\n");
		goto EXIT;
	}

	r_memset(pbuf, 0x0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, parse_xml_user->pdoc, filename);
	if(ret < 0 )
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: record id fail\n");
	}
	r_strcpy(file_buf, pbuf);

	pfilename = (int8_t *)xmlNodeListGetString(parse_xml_user->pdoc, filename->xmlChildrenNode, 1);
	if(pfilename == NULL)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: not found usrname\n");
		goto EXIT;
	}

	reason  = get_children_node(msgbody, BAD_CAST "Reason");
	if(reason == NULL)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: not found password\n");
		goto EXIT;
	}

	r_memset(pbuf, 0x0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, parse_xml_user->pdoc, reason);
	if(ret < 0 )
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: record id fail\n");
	}
	r_strcpy(reason_buf, pbuf);


	preason = (int8_t *)xmlNodeListGetString(parse_xml_user->pdoc, reason->xmlChildrenNode, 1);
	if(preason == NULL)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: not found password\n");
		goto EXIT;
	}

	return_code = 1;

EXIT:
	if(NULL != pfilename)
	{
		xmlFree(pfilename);
	}
	if(NULL != preason)
	{
		xmlFree(preason);
	}

	if(parse_xml_user != NULL)
	{
		if(parse_xml_user->pdoc != NULL)
		{
			release_dom_tree(parse_xml_user->pdoc);
		}
	}

	r_free(parse_xml_user);

	return return_code;
}

/* 解析ftp返回成功消息 */
static int32_t analyze_ftp_respsuccess(int8_t *recv_buf, int8_t *filename, int8_t *precid,
													int8_t *ipaddr, int8_t *path)
{
	parse_xml_t *parse_xml_user = NULL;
	int32_t return_code = -1;
	xmlNodePtr msgbody = NULL;
	xmlNodePtr pnode = NULL;
	int32_t ret = 0;

	int8_t *pfilename = NULL;
	int8_t *pbuf = NULL;

	if(NULL == filename || NULL == precid || NULL == recv_buf || NULL == ipaddr || NULL == path)
	{
		zlog_error(DBGLOG, "analyze_ftp_respsuccess: params is NULL!\n");
		return -1;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(parse_xml_user == NULL)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: malloc parse_xml_t fail\n");
		return -1;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	init_dom_tree(parse_xml_user, (const char *)recv_buf);
	if(parse_xml_user == NULL)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: init_dom_tree fail\n");
		goto EXIT;
	}

	if(is_resp_msg(parse_xml_user->proot) != 1)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: is_req_msg fail\n");
		goto EXIT;
	}

	msgbody = get_children_node(parse_xml_user->proot, MSG_BODY_KEY);
	if(msgbody == NULL)
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: msghead fail\n");
		goto EXIT;
	}

	pnode = get_children_node(msgbody, BAD_CAST "RecordID");
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, parse_xml_user->pdoc, pnode);
	if(ret < 0 )
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: record id fail\n");
	}
	r_strcpy(precid, pbuf);


	pnode = get_children_node(msgbody, BAD_CAST "SuccessFile");
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, parse_xml_user->pdoc, pnode);
	if(ret < 0 )
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: filename fail\n");
	}
	r_strcpy(filename, pbuf);


	pnode = get_children_node(msgbody, BAD_CAST "UpLoadServerIp");
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, parse_xml_user->pdoc, pnode);
	if(ret < 0 )
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: UpLoadServerIp fail\n");
	}
	r_strncpy(ipaddr, pbuf, 20);


	pnode = get_children_node(msgbody, BAD_CAST "UpLoadRomtePath");
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, parse_xml_user->pdoc, pnode);
	if(ret < 0 )
	{
		zlog_error(DBGLOG, "Analyze_ftp_upload: UpLoadRomtePath fail\n");
	}
	r_strcpy(path, pbuf);

	return_code = 1;

EXIT:
	if(NULL != pfilename)
	{
		xmlFree(pfilename);
	}


	if(parse_xml_user->pdoc != NULL)
	{
		release_dom_tree(parse_xml_user->pdoc);
	}

	r_free(parse_xml_user);

	if(pbuf)
		r_free(pbuf);

	return return_code;
}

/* 配置信令信令生成 */
static int32_t ftp_buffer_config(int8_t *send_buf, const int8_t *usrname,
                                 const int8_t *password)
{
	xmlChar *temp_xml_buf;
	int size = 0;
	xmlDocPtr doc = xmlNewDoc(XML_DOC_VERSION);

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node = NULL;
	xmlNodePtr body_node  = NULL;

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	package_add_xml_leaf(body_node, (const xmlChar *)("usrname"), usrname);
	package_add_xml_leaf(body_node, (const xmlChar *)("password"), password);

	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, temp_xml_buf, size);
	xmlFree(temp_xml_buf);

	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

/* 上传信令生成 */
static int32_t ftp_buffer_upload(int8_t *send_buf, int8_t *localpath,
                                 int8_t *ftp_ipaddr, int8_t *romote_path, int8_t *port, int8_t *precid)
{

	xmlChar *temp_xml_buf;
	int size = 0;
	xmlDocPtr doc = xmlNewDoc(XML_DOC_VERSION);

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"RequestMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	package_add_xml_leaf(body_node, (const xmlChar *)"UpLoadLocalPath", localpath);
	package_add_xml_leaf(body_node, (const xmlChar *)"UpLoadServerIp", ftp_ipaddr);
	package_add_xml_leaf(body_node, (const xmlChar *)"UpLoadRomtePath", romote_path);
	package_add_xml_leaf(body_node, (const xmlChar *)"Port", port);
	package_add_xml_leaf(body_node, (const xmlChar *)"RecordID", precid);

	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, temp_xml_buf, size);
	xmlFree(temp_xml_buf);

	if(doc != NULL){
		release_dom_tree(doc);
	}
	return size;
}

/* 配置ftp信息 */
static int32_t ftpcom_set_server_info(void *handle, const int8_t *username,
                                      const int8_t *passwd)
{
	MsgHeader *pmsg;
	ftpcom_env *pftp = NULL;
	int8_t buffer[MAX_XMNL_LEN] = {0};

	if(NULL == handle || NULL == username || NULL == passwd)
	{
		zlog_error(DBGLOG, "---[ftpcom_set_server_info] failed, params is NULL!\n");
		return -1;
	}

	pftp = (ftpcom_env *)handle;

	if(ftpcom_check_login_status(pftp) != FTP_COM_ONLINE)
	{
		zlog_error(DBGLOG, "---[ftpcom_set_server_info] failed, ftp is offline!\n");
		return -1;
	}

	zlog_debug(DBGLOG, "sending ftpcom_set_server_info!\n");

	pthread_mutex_lock(&(pftp->socket_m));
	if(pftp->run_status == FTP_COM_RUNNING && pftp->socket > 0)
	{
		int size = 0;
		pmsg = (MsgHeader*)buffer;
		pmsg->sMsgType = htons(FTP_COM_MSGTYPE_SERVER_CONFIG);
		pmsg->sVer = htons(CONTROL_NEW_MSGHEADER_VERSION);

		size = ftp_buffer_config(buffer+FTP_COM_MSGHEAD_LEN, username, passwd);

		zlog_debug(OPELOG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		zlog_debug(OPELOG, "%s\n", buffer+FTP_COM_MSGHEAD_LEN);
		zlog_debug(OPELOG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

		pmsg->sLen = htons(FTP_COM_MSGHEAD_LEN+size);
		if(0 != size)
		{
			int nameLength = tcp_send_longdata(pftp->socket, buffer, FTP_COM_MSGHEAD_LEN+size);
			if(nameLength < 0)
			{
				zlog_error(DBGLOG,"senf fail");
			}
		}
	}
	pthread_mutex_unlock(&(pftp->socket_m));

	return 0;
}

/* 上传文件 */
static int32_t ftpcom_upload_file(void *handle, int8_t *localpath,
                                  int8_t *ftp_ipaddr, int8_t *remote_path, int8_t *port, int8_t *precid)
{
	MsgHeader *pmsg;
	ftpcom_env *pftp = NULL;
	int8_t buffer[4*1024] = {0};

	if(NULL == handle || NULL == localpath || NULL == ftp_ipaddr || NULL == port || NULL == remote_path)
	{
		zlog_error(DBGLOG, "ftpcom_set_server_info failed, handle is NULL!\n");
		return -1;
	}

	pftp = (ftpcom_env *)handle;

	if(ftpcom_check_login_status(pftp) != FTP_COM_ONLINE)
	{
		zlog_error(DBGLOG, "---[ftpcom_upload_file] failed, ftp is offline!\n");
		return -1;
	}

	zlog_debug(DBGLOG, "sending ftpcom_upload_file!\n");

	pthread_mutex_lock(&(pftp->socket_m));
	if(pftp->run_status == FTP_COM_RUNNING && pftp->socket > 0)
	{
		int size = 0;
		pmsg = (MsgHeader*)buffer;
		pmsg->sMsgType = htons(FTP_COM_MSGTYPE_SERVER_UPLOAD_FILE);
		pmsg->sVer = htons(CONTROL_NEW_MSGHEADER_VERSION);

		size = ftp_buffer_upload(buffer+FTP_COM_MSGHEAD_LEN, localpath, ftp_ipaddr, remote_path, port, precid);
		zlog_debug(OPELOG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		zlog_debug(OPELOG, "%s\n", buffer+FTP_COM_MSGHEAD_LEN);
		zlog_debug(OPELOG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		pmsg->sLen = htons(FTP_COM_MSGHEAD_LEN+size);
		if(0 != size)
		{
			int nameLength = tcp_send_longdata(pftp->socket, buffer, FTP_COM_MSGHEAD_LEN+size);
			if(nameLength < 0)
			{
				zlog_error(DBGLOG, "File name Error! \n");
			}
		}
	}
	pthread_mutex_unlock(&(pftp->socket_m));

	return 0;
}

int32_t ftpcom_save_filename(void *handle, int8_t *filename)
{
	ftpcom_env *pftp = NULL;

	if(NULL == handle|| NULL == filename)
	{
		zlog_error(DBGLOG, "---[ftpcom_save_filename] failed, params is NULL!\n");
		return -1;
	}

	pftp = (ftpcom_env *)handle;

	pthread_mutex_lock(&(pftp->ftp_m));
	r_strcpy(pftp->filename, filename);
	pthread_mutex_unlock(&(pftp->ftp_m));

	return 0;
}

int32_t ftpcom_set_function(ftpcom_env *pftp)
{
	if(NULL == pftp)
	{
		zlog_error(DBGLOG, "---[ftpcom_set_function] failed, params is NULL!\n");
		return -1;
	}

	pthread_mutex_lock(&(pftp->ftp_m));
	pftp->pcmd.set_server_info = ftpcom_set_server_info;
	pftp->pcmd.upload_file = ftpcom_upload_file;
	pftp->pcmd.save_filename = ftpcom_save_filename;
	pthread_mutex_unlock(&(pftp->ftp_m));

	return 0;
}


int32_t connect_to_ftpupload_server(const int8_t* paddr,const int32_t port)
{
	int32_t fileflags = 0;
	int32_t ret = 0;

	struct sockaddr_in client_addr;
	bzero(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	client_addr.sin_port = htons(0);

	int client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(client_socket < 0)
	{
		zlog_error(DBGLOG, "connect_to_ftpupload_server failed, err msg: %s \n", strerror(errno));
		return -1;
	}

	if(bind(client_socket, (struct sockaddr*) &client_addr, sizeof(client_addr)))
	{
		zlog_error(DBGLOG, "Client Bind Port Failed!\n");
		return -1;
	}

	struct sockaddr_in serv_addr;
	bzero(&serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port	= htons(port);
	inet_aton((const char *)paddr, (struct in_addr *)&serv_addr.sin_addr);

	set_send_timeout(client_socket, FTP_COM_TCP_SOCKET_TIMEOUT);
	set_recv_timeout(client_socket, FTP_COM_TCP_SOCKET_TIMEOUT);

	ret = connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	if(ret < 0)
	{
//		zlog_error(DBGLOG, "connect failed, err msg: %s \n", strerror(errno));
		close(client_socket);
		return -1;
	}

	fileflags = fcntl(client_socket, F_GETFL, 0);
	if(fileflags < 0)
	{
		zlog_error(DBGLOG, "fcntl F_GETFL failed, err msg: %s\n", strerror(errno));
		close(client_socket);
		return -1;
	}

	return client_socket;
}

int32_t ftpcom_recv_msghead(ftpcom_env *pftp, int8_t *pbuf)
{
	int32_t recvlen = 0;

	if(NULL == pftp || NULL == pbuf)
	{
		zlog_error(DBGLOG, "--[ftpcom_recv_msghead] failed, params is NULL!\n");
		return -1;
	}

	recvlen = tcp_recv_longdata(pftp->socket, pbuf, FTP_COM_MSGHEAD_LEN);
	if(recvlen != FTP_COM_MSGHEAD_LEN)
	{
		zlog_error(DBGLOG, "--[ftpcom_recv_msghead] failed, recv msghead error, recv_len = %d\n",
		           recvlen);
		return -1;
	}

	return recvlen;
}

int32_t ftpcom_recv_xml_data(ftpcom_env *pftp, int8_t *pbuf, int32_t length)
{
	int32_t recvlen = 0;

	if(NULL == pftp || NULL == pbuf)
	{
		zlog_error(DBGLOG, "--[ftpcom_recv_xml_data] failed, params is NULL!\n");
		return -1;
	}

	recvlen = tcp_recv_longdata(pftp->socket, pbuf, length);
	if(recvlen != length)
	{
		zlog_error(DBGLOG, "--[ftpcom_recv_xml_data] failed, recv msghead error, recv_len = %d\n",
		           recvlen);
		return -1;
	}

	return recvlen;
}


void *ftpupload_com_process(void *args)
{
	int32_t socket = 0;
	int32_t recvlen = 0;
	int32_t datalen = 0;
	int32_t seret = 0;
	int32_t ret_len = 0;
	int32_t length = 0;
	int32_t ret = 0;
	int32_t req_count = 0;
	int8_t *pbuf = NULL;
	MsgHeader *pmsg;
	server_set *pser = NULL;
	file_user *pfile_user = NULL;

	int8_t *send_buf = NULL;
	int8_t *ret_buf = NULL;
	int8_t url[HTTP_SERVER_URL_MAX_LEN] = {0};
	int8_t filename[RECORD_FILE_MAX_FILENAME] = {0};
	int8_t recordid[RECORD_ID_MAX_LENGTH] = {0};
	int8_t serverip[24] = {0};
	int8_t path[FTP_MAX_FTPPATH_LENGTH] = {0};

	int8_t fail_file[XML_VALUE_MAX_LENGTH] = {0};
	int8_t fail_rson[XML_VALUE_MAX_LENGTH] = {0};
	int8_t temp_file[XML_VALUE_MAX_LENGTH] = {0};

	fd_set readfd;
	struct timeval timeout;

	if(NULL == args)
	{
		zlog_error(DBGLOG, "ftpupload_com_process failed, args is NULL!");
		return NULL;
	}

	send_buf = (int8_t *)r_malloc(CONTROL_DATA_LEN);
	ret_buf = (int8_t *)r_malloc(CONTROL_DATA_LEN);
	if(NULL == send_buf || NULL == ret_buf)
	{
		zlog_error(DBGLOG, "ftpupload_com_process failed, malloc failed!\n");
		goto EXIT;
	}

	ftpcom_env *pftp = (ftpcom_env *)args;

	socket = connect_to_ftpupload_server(pftp->ipaddr, pftp->port);
	if(socket < 0)
	{
		zlog_error(DBGLOG, "ftpupload_com_process failed, connect_to_ftpupload_server failed!\n");
		goto EXIT;
	}

	pfile_user = (file_user *)r_malloc(sizeof(file_user));
	if(NULL == pfile_user)
	{
		zlog_error(DBGLOG, "ftpupload_com_process failed, malloc failed!\n");
		goto EXIT;
	}

	pbuf = pftp->buf;
	pftp->socket = socket;
	pser = pftp->pserset;

	ftpcom_set_login(pftp);
	ftpcom_set_function(pftp);
	ftpcom_set_running(pftp);
	while(pftp->run_status != FTP_COM_STOPED)
	{
		if(FTP_COM_PREPARING == pftp->run_status)
		{
//			zlog_debug(DBGLOG, "now ftpcom is in preparing status\n");
			usleep(500000);
			continue;
		}

		seret = 0;
		timeout.tv_sec = 10;
		FD_ZERO(&readfd);
		FD_SET(socket, &readfd);
		r_memset(pbuf, 0x0, FTP_COM_DATA_LEN);
		seret = select(socket+1, &readfd, NULL, NULL, &timeout);
		if(seret > 0 && FD_ISSET(socket, &readfd))
			/* FD已准备好 */
		{
			pthread_mutex_lock(&(pftp->socket_m));
			recvlen = ftpcom_recv_msghead(pftp, pbuf);
			if(recvlen < 0)
			{
				pthread_mutex_unlock(&(pftp->socket_m));
				break;
			}
			pmsg = (MsgHeader* )pbuf;
			length = ntohs(pmsg->sLen) - FTP_COM_MSGHEAD_LEN;

			datalen = ftpcom_recv_xml_data(pftp, pbuf+FTP_COM_MSGHEAD_LEN, length);
			if(datalen != length)
			{
				pthread_mutex_unlock(&(pftp->socket_m));
				break;
			}
			pthread_mutex_unlock(&(pftp->socket_m));

			pmsg->sMsgType = ntohs(pmsg->sMsgType);
			switch(pmsg->sMsgType)
			{
				case FTP_COM_MSGTYPE_UPLOAD_SUCCESS:
				{
					/* 上传成功 */
					r_memset(filename, 0, RECORD_FILE_MAX_FILENAME);
					r_memset(recordid, 0, RECORD_ID_MAX_LENGTH);
					r_memset(serverip, 0, 24);
					r_memset(path, 0, FTP_MAX_FTPPATH_LENGTH);

					zlog_debug(OPELOG, "ftp back xml[%s]\n",pbuf + FTP_COM_MSGHEAD_LEN);
					ret = analyze_ftp_respsuccess(pbuf + FTP_COM_MSGHEAD_LEN, filename, recordid, serverip, path);
					zlog_debug(OPELOG, "ftp upload successful !!!\n");
					zlog_debug(OPELOG, "%s\n", pbuf + FTP_COM_MSGHEAD_LEN);
					if(1==ret){
						pthread_mutex_lock(&pser->pserinfo->info_m);
						r_memset(url, 0, HTTP_SERVER_URL_MAX_LEN);
						r_strcpy(url, pser->pserinfo->HBeatInfo.post_url);
						pthread_mutex_unlock(&pser->pserinfo->info_m);

						r_memset(send_buf, 0, CONTROL_DATA_LEN);
						r_memset(pfile_user, 0, sizeof(file_user));

						r_strcpy(pfile_user->filename, filename);
						r_strcpy(pfile_user->recordid, recordid);
						r_strcpy(pfile_user->serverip, serverip);
						r_strcpy(pfile_user->path, path);

						package_upload_status_report_xml_data(send_buf, RecServer,
								                                      MSGCODE_HTTP_FTPUPLOAD_REPORT, pfile_user);

						ret_len = CONTROL_XML_DATA_LEN;
						zlog_debug(OPELOG, "url: %s\n", url);
						zlog_debug(OPELOG, "%s\n\n", send_buf);

						req_count = 3;
						while(req_count--){
							r_memset(ret_buf, 0, CONTROL_DATA_LEN);
							ret = mid_http_post((char *)url, (char *)send_buf, r_strlen(send_buf), (char *)ret_buf, &ret_len);
							zlog_debug(OPELOG, "\n\n[mid_http_post]ret = %d\n\n", ret);
							if(ret == -1){
								zlog_error(OPELOG, "___report upload completed failed!___, filename = %s\n", pfile_user->filename);
								usleep(50000);
							}
							else{
								break;
							}
						}
						zlog_debug(OPELOG, "\n\n\n\n\n%s\n\n\n\n\n\n", ret_buf);
					}

					r_memset(temp_file, 0x0, XML_VALUE_MAX_LENGTH);
					get_file_name_from_path(filename, temp_file);
					pthread_mutex_lock(&(pser->pserinfo->ftp_file_m));
					modify_ftp_fail_file_node(&(pser->pserinfo->parse_xml_ftp), temp_file, (int8_t *)"", MODIFY_TYPE_DELETE_NODE);
					pthread_mutex_unlock(&(pser->pserinfo->ftp_file_m));

					/*FTP 上传课件日志上报*/
					Log_t log_info;
					int8_t ipaddr[16] = {0};
					struct in_addr addr;

					r_memcpy(&addr, &pftp->pserset->pserinfo->ServerInfo.LanAddr, 4);
					r_memset(ipaddr, 0, 16);
					r_strcpy((int8_t *)ipaddr, (const int8_t *)inet_ntoa(addr));

					r_bzero(&log_info, sizeof(Log_t));
					sprintf((char *)log_info.Type, "%d", LOG_TYPE_FTP_TRANS);
					sprintf((char *)log_info.Addr, "%s", ipaddr);
					sprintf((char *)log_info.User, "%s", WARN_SOURCE_CAPTURE);
					sprintf((char *)log_info.Content, "%s", (int8_t *)MSG_LOG_SUCC_CONTENT);
					warn_get_current_time_info(log_info.Time);

					zlog_debug(OPELOG, "--------log_info.Type = %s-------------------!\n", log_info.Type);
					report_log_info_process(pser, &log_info);
				}
				break;

				case FTP_COM_MSGTYPE_UPLOAD_ERROR:
				{
					zlog_debug(OPELOG, "ftp back xml[%s]",pbuf + FTP_COM_MSGHEAD_LEN);
					/* 上传失败，不上报 */
					r_memset(fail_file, 0x0, XML_VALUE_MAX_LENGTH);
					r_memset(fail_rson, 0x0, XML_VALUE_MAX_LENGTH);
					r_memset(temp_file, 0x0, XML_VALUE_MAX_LENGTH);

					analyze_ftp_respfaile(pbuf + FTP_COM_MSGHEAD_LEN, fail_file, fail_rson);
					get_file_name_from_path(fail_file, temp_file);
					pthread_mutex_lock(&(pser->pserinfo->ftp_file_m));
					modify_ftp_fail_file_node(&(pser->pserinfo->parse_xml_ftp), temp_file, fail_rson, MODIFY_TYPE_ADD_NODE);
					pthread_mutex_unlock(&(pser->pserinfo->ftp_file_m));

					/*FTP 上传课件日志上报*/
					Log_t log_info;
					int8_t ipaddr[16] = {0};
					struct in_addr addr;

					r_memcpy(&addr, &pftp->pserset->pserinfo->ServerInfo.LanAddr, 4);
					r_memset(ipaddr, 0, 16);
					r_strcpy((int8_t *)ipaddr, (const int8_t *)inet_ntoa(addr));

					r_bzero(&log_info, sizeof(Log_t));
					sprintf((char *)log_info.Type, "%d", LOG_TYPE_FTP_TRANS);
					sprintf((char *)log_info.Addr, "%s", ipaddr);
					sprintf((char *)log_info.User, "%s", WARN_SOURCE_CAPTURE);
					sprintf((char *)log_info.Content, "%s", (int8_t *)MSG_LOG_FAIL_CONTENT);
					warn_get_current_time_info(log_info.Time);

					zlog_debug(OPELOG, "--------log_info.Type = %s-------------------!\n", log_info.Type);
					report_log_info_process(pser, &log_info);


					zlog_debug(DBGLOG, "ftp upload failed !!!\n");
				}
				break;
				default
						:
					break;
			}
		}
		else
			if(seret == 0)
				/* 超时 */
			{
				zlog_debug(DBGLOG, "--[ftpcom select timetou]---\n");
				usleep(1000);
				continue;
			}
			else
				if(seret < 0)
					/* 异常 */
				{
					zlog_debug(DBGLOG, "--[ftpcom select error]---\n");
					usleep(1000);
					break;
				}
	}

EXIT:

	if(send_buf)
		r_free(send_buf);

	if(ret_buf)
		r_free(ret_buf);

	if(pfile_user)
		r_free(pfile_user);

	pthread_mutex_lock(&(pftp->ftp_m));
	pftp->run_status = FTP_COM_STOPED;
	pthread_mutex_unlock(&(pftp->ftp_m));

	pthread_mutex_lock(&(pftp->socket_m));
	pftp->socket = -1;
	close(socket);
//	shutdown(socket, SHUT_RDWR);
	pthread_mutex_unlock(&(pftp->socket_m));

	ftpcom_set_logout(pftp);

	return NULL;
}

void *ftpupload_com_detect_task(void *args)
{
	pthread_t thid;
	int32_t ret = 0;
	void *value = NULL;
	ftpcom_env *pftp = NULL;

	if(NULL == args)
	{
		zlog_error(DBGLOG, "ftpupload_com_detect_task failed, args is NULL!");
		return NULL;
	}

	pftp = (ftpcom_env *)args;

	while(1)
	{
		ret = pthread_create(&thid, NULL, ftpupload_com_process, (void *)pftp);
		if(ret)
		{
			zlog_error(DBGLOG, "ftpupload_com_detect_task failed, err msg: %s\n", strerror(errno));
		}

		pthread_join(thid, &value);
		sleep(3);
	}

	pthread_detach(pthread_self());

	return 0;
}

int32_t start_ftpupload_com_task(server_set *pser)
{
	pthread_t thid;
	int32_t ret = 0;

	ftpcom_env *ftp = NULL;

	if(NULL == pser)
	{
		zlog_error(DBGLOG, "start_ftpupload_com_task failed, params is NULL\n");
		return -1;
	}

	ftp = &pser->ftpser;

	ftpcom_init_env(ftp);
	ftpcom_set_server_port(ftp, FTP_COM_PORT);
	ftpcom_set_server_ip(ftp, (const int8_t *)FTP_COM_IPADDR);
	ftpcom_set_server_set(ftp, pser);

	ret = pthread_create(&thid, NULL, ftpupload_com_detect_task, (void *)ftp);
	if(ret)
	{
		zlog_error(DBGLOG, "start_ftpupload_com_task failed, err msg: %s\n", strerror(errno));
		pthread_mutex_destroy(&(ftp->socket_m));
		r_free(ftp);
		ftp = NULL;
		return -1;
	}

	return 0;
}


