/*
 * =====================================================================================
 *
 *       Filename:  command_resolve.c
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
#include <dirent.h>
#include "common.h"
#include "control_log.h"
#include "xml_base.h"
#include "command_resolve.h"
#include "control.h"
#include "command_protocol.h"
#include "xml_msg_management.h"
#include "mid_http.h"
#include "params.h"
#include "timeout_process.h"
#include "web/weblisten.h"
#include <time.h>
#include "hd_card.h"
#define	SD_ENC_IP	"169.254.0.3"
#define	INVALIID_IP	"0.0.0.0"
#define	IPCAMERA_IP	INVALIID_IP
extern server_set *gpser;

#define PIC_SYNT_RESOLUTIONS		"720x480|704x576|800x600|1024x576|1024x768|1280x720|1920x1080"
#define PIC_SYNT_RATES			"300|500|600|800|1000|1500|2000"
extern int32_t http_set_post_url(con_user *puser, int16_t http_port);
extern int32_t upload_logo_pro_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
        int8_t **send_buf, roomid_array *parray, int32_t user_index, int32_t *data_length, int32_t *xml_len);
extern int32_t send_user_xml_data2(con_user *user_src, con_user *user_dst, int8_t *send_buf,
                                   uint32_t xml_len, uint32_t data_len);
extern int32_t check_platform_unique_and_report(control_env *pcon, con_user *puser, platform_em platform);
extern int32_t set_user_ipaddr(control_env *pcon, int8_t *ipaddr, platform_em platform);
#if 0
int32_t is_req_msg(xmlNodePtr proot)
{
	if(xmlStrcmp(proot->name, REQ_ROOT_KEY) != 0) {
		return -1;
	}

	return 0;
}
int32_t is_resp_msg(xmlNodePtr proot)
{
	if(xmlStrcmp(proot->name, RESP_ROOT_KEY) != 0) {
		return -1;
	}

	return 0;
}
#endif
#define	HD_CON_PORT	12131
static int connect_HD_card()
{
	struct sockaddr_in serv_addr;
	const char* pAddr  = HD_ENC_IP;
	int socketFd;

	socketFd= socket(PF_INET, SOCK_STREAM, 0);
	if(socketFd < 1)
		return -1;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(HD_CON_PORT);
	inet_aton(pAddr,(struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero),8);

	if(connect(socketFd, (struct sockaddr *)&serv_addr,sizeof(struct sockaddr)) == -1)	{
		printf("connect   error!!!errno=%d [%s]\n",errno,strerror(errno));
		close(socketFd);
		return -1;
	}
/*
	set_send_timeout(socketFd, 30);
	set_recv_timeout(socketFd, 30);
	*/
	return socketFd;
}
static int32_t recv_user_xml_data(int32_t socket, int8_t *xml_buf, int16_t *force_msgcode,
												int8_t *plog_head, uint32_t *datalen)
{
	int32_t recvlen = 0;;
	int32_t headlen = 0;
	int32_t return_code = STATUS_FAILED;

	MsgHeader msg;

	headlen = CONTROL_MSGHEAD_LEN;

	if(NULL == xml_buf || NULL == force_msgcode || datalen == NULL){
		zlog_error2(DBGLOG, "---[recv_user_xml_data] failed, xml_buf is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	recvlen = tcp_recv_longdata(socket, (int8_t *)&msg, headlen);
	if(recvlen < headlen || recvlen == -1){
		zlog_error2(DBGLOG, "nLen < HEAD_LEN  errno = %d  nLen = %d\n", errno, recvlen);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

//	msg.sMsgType = ntohs(msg.sMsgType);



//	zlog_debug(OPELOG, "00- sLen = %d, sVer = %d, sMsgType = %d\n", msg.sLen, msg.sVer, msg.sMsgType);

	msg.sMsgType = ntohs(msg.sMsgType);
	msg.sLen = ntohs(msg.sLen);
	msg.sVer = ntohs(msg.sVer);

//	zlog_debug(OPELOG, "11- sLen = %d, sVer = %d, sMsgType = %d\n", msg.sLen, msg.sVer, msg.sMsgType);

	if(((msg.sVer == CONTROL_OLD_MSGHEADER_VERSION) && (msg.sMsgType == CONTROL_OLD_MSGHEADER_MSGCODE))
		|| ((msg.sVer == ntohs(CONTROL_OLD_MSGHEADER_VERSION)) && (msg.sMsgType == ntohs(CONTROL_OLD_MSGHEADER_MSGCODE))))
	{
		*force_msgcode = CONTROL_OLD_MSGHEADER_MSGCODE;
		return_code = STATUS_SUCCESS;
		goto EXIT;
	}

#if 1
	/* FIXME: 此处需作判断 */
	if(!(msg.sVer == CONTROL_NEW_MSGHEADER_VERSION || msg.sVer == CONTROL_NEW_MSGHEADER_MSGCODE)){
		return_code = STATUS_FAILED;
		goto EXIT;
	}
#endif

#if 0
	if(!(msg.sMsgType != CONTROL_NEW_MSGHEADER_MSGCODE || msg.sMsgType != CONTROL_NEW_MSGHEADER_VERSION)){
		return_code = STATUS_FAILED;
		goto EXIT;
	}
#endif

	if(266 != msg.sLen && 213 != msg.sLen){
		zlog_debug2(DBGLOG, "msg.sLen = %d\n", msg.sLen);
	}
	if((msg.sLen - headlen) < 0){
		zlog_error2(DBGLOG, "msg len error, len = %d\n", msg.sLen);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(msg.sLen > CONTROL_DATA_LEN)
		msg.sLen = CONTROL_DATA_LEN;

	recvlen = tcp_recv_longdata(socket, xml_buf, msg.sLen - headlen);
	if(recvlen < msg.sLen-headlen){
		zlog_error2(DBGLOG, "nLen < nMsgLen -HEAD_LEN\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	*datalen = recvlen;

	return_code = STATUS_SUCCESS;

EXIT:

	return return_code;
}

xmlNodePtr get_req_msgcode_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, MSG_HEAD_KEY);
	*pnode = get_children_node(tmp_node, MSG_CODE_KEY);
	return *pnode;
}

xmlNodePtr get_req_passkey_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, MSG_HEAD_KEY);
	*pnode = get_children_node(tmp_node, MSG_PASSKEY_KEY);
	return *pnode;
}

xmlNodePtr get_resq_returncode_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, MSG_HEAD_KEY);
	*pnode = get_children_node(tmp_node, MSG_RETURNCODE_KEY);
	return *pnode;
}


xmlNodePtr get_req_user_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, MSG_BODY_KEY);
	*pnode = get_children_node(tmp_node, MSG_USER_KEY);
	return *pnode;
}

xmlNodePtr get_req_password_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, MSG_BODY_KEY);
	*pnode = get_children_node(tmp_node, MSG_PASSWORD_KEY);
	return *pnode;
}

int32_t package_add_xml_leaf(xmlNodePtr father_node, const xmlChar *key_name,
                             const int8_t *key_value)
{
	xmlNodePtr tmp_node	= NULL;

	tmp_node = xmlNewNode(NULL, key_name);
	xmlAddChild(father_node, tmp_node);
	xmlAddChild(tmp_node, xmlNewText((const xmlChar *)key_value));

	return STATUS_SUCCESS;
}

int32_t package_resp_xml_data(int8_t *out_buf, int32_t roomid, int32_t msgcode,
                              int8_t *ret_code)
{
	int32_t 	return_code = STATUS_FAILED;
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[16] = {0};
	int8_t		buf[16] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;

	if(NULL == out_buf || NULL == ret_code) {
		zlog_error(DBGLOG, "---[package_resp_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, RESP_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	sprintf((char *)msgcode_buf, "%d", msgcode);

	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);
	package_add_xml_leaf(head_node, MSG_RETURNCODE_KEY, ret_code);
	package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, (const int8_t *)PLATFORM_RECSERVER);

	if(roomid == -1) {
		package_add_xml_leaf(root_node, MSG_BODY_KEY, (int8_t *)XML_NULL_VALUE);
	} else {
		body_node = xmlNewNode(NULL, MSG_BODY_KEY);
		xmlAddChild(root_node, body_node);
		sprintf((char *)buf, "%d", roomid);
		package_add_xml_leaf(body_node, MSG_ROOMID_KEY, (int8_t *)buf);
	}

	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	return return_code;;
}

int32_t package_req_xml_data(int8_t *out_buf, platform_em platform,
                             int32_t msgcode, int8_t *ret_code)
{
	int32_t 	return_code = STATUS_FAILED;
	int32_t 	xmllen = 0;
	int32_t		ret = 0;

	int8_t		msgcode_buf[16] = {0};
	int8_t		passkey[PASS_KEY_LEN];

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;

	if(NULL == out_buf || NULL == ret_code) {
		zlog_error(DBGLOG, "---[package_req_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	sprintf((char *)msgcode_buf, "%d", msgcode);
	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);
	package_add_xml_leaf(head_node, MSG_RETURNCODE_KEY, ret_code);

	ret = get_passkey_from_platform(platform, passkey);

	if(0 == ret) {
		package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, passkey);
	}

	package_add_xml_leaf(root_node, MSG_BODY_KEY, (int8_t *)XML_NULL_VALUE);

	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	return return_code;;
}

int32_t package_add_warn_node(xmlNodePtr warns_node, const Warn_t *warn_info)
{
	xmlNodePtr warn_node	= NULL;

	warn_node = xmlNewNode(NULL, MSG_WARN_WARNING_KEY);
	xmlAddChild(warns_node, warn_node);

	package_add_xml_leaf(warn_node, MSG_WARN_ID_KEY, warn_info->ID);
	package_add_xml_leaf(warn_node, MSG_WARN_SOURCE_KEY, warn_info->Source);
	package_add_xml_leaf(warn_node, MSG_WARN_ROOMID_KEY, warn_info->RoomID);
	package_add_xml_leaf(warn_node, MSG_WARN_CODECID_KEY, warn_info->CodecID);
	package_add_xml_leaf(warn_node, MSG_WARN_TIME_KEY, warn_info->Time);
	package_add_xml_leaf(warn_node, MSG_WARN_CONTENT_KEY, warn_info->Content);

	return STATUS_SUCCESS;
}

int32_t package_log_info_report_data(int8_t *out_buf, const Log_t *log_info, int32_t msg_code)
{
	int32_t 	return_code = STATUS_FAILED;
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[16] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;
	xmlNodePtr	log_node  = NULL;

	if(NULL == log_info || NULL == log_info) {
		zlog_error(DBGLOG, "---[package_log_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);

	//RequestMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	//MsgCode
	sprintf((char *)msgcode_buf, "%d", msg_code);
	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	//PassKey
	package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, (int8_t *)PLATFORM_RECSERVER);

	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//Log
	log_node = xmlNewNode(NULL, MSG_LOG_KEY);
	xmlAddChild(body_node, log_node);

	package_add_xml_leaf(log_node, MSG_LOG_TYPE_KEY,	(int8_t *)log_info->Type);
	package_add_xml_leaf(log_node, MSG_LOG_TIME_KEY,	(int8_t *)log_info->Time);
	package_add_xml_leaf(log_node, MSG_LOG_USER_KEY,	(int8_t *)log_info->User);
	package_add_xml_leaf(log_node, MSG_LOG_ADDR_KEY,	(int8_t *)log_info->Addr);
	package_add_xml_leaf(log_node, MSG_LOG_CONTENT_KEY,	(int8_t *)log_info->Content);

	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	return return_code;;
}

int8_t *trans_platform_to_log_info_user2(const int8_t 	*platform, int8_t *log_info_user)
{
	if(log_info_user == NULL) {
		zlog_error(DBGLOG, "---[trans_platform_from_log_info] failed, params is NULL!\n");
		return NULL;
	}

	if(!r_strcmp(platform, (int8_t *)PLATFORM_MEDIACENTER)) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_MEDIA_CENTER);
	} else if(!r_strcmp(platform, (int8_t *)PLATFORM_RECSERVER)) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_CAPTURE);
	} else if(!r_strcmp(platform, (int8_t *)PLATFORM_MANAGEPLATFORM)) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_MANAGE_PLF);
	} else if(!r_strcmp(platform, (int8_t *)PLATFORM_COMCONTROL)) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_CENTER_CTRL);
	} else if(!r_strcmp(platform, (int8_t *)PLATFORM_DIRECTOR)) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_DIRECTOR);
	} else {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_OTHER_CTRL);
	}

	return log_info_user;
}



int8_t *trans_platform_to_log_info_user(const platform_em	platform, int8_t *log_info_user)
{
	if(log_info_user == NULL) {
		zlog_error(DBGLOG, "---[trans_platform_from_log_info] failed, params is NULL!\n");
		return NULL;
	}

	if(platform == MediaCenter) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_MEDIA_CENTER);
	} else if(platform == RecServer) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_CAPTURE);
	} else if(platform == ManagePlatform) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_MANAGE_PLF);
	} else if(platform == IpadUser) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_MANAGE_PLF);
	} else if(platform == ComControl) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_CENTER_CTRL);
	} else if(platform == Director) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_DIRECTOR);
	} else if(platform == NetControl) {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_NET_CTRL);
	} else {
		r_strcpy(log_info_user, (int8_t *)WARN_SOURCE_OTHER_CTRL);
	}

	return log_info_user;
}

int32_t report_log_info_process(server_set *pserset, const Log_t *log_info)
{
	int32_t ret				= 0;
	int32_t ret_len			= 0;
	int32_t return_code		= STATUS_FAILED;
	int8_t *xml_buf			= NULL;
	int8_t *ret_buf			= NULL;
	int8_t xml_val[XML_VALUE_MAX_LENGTH]	= {0};
	int8_t url[HTTP_SERVER_URL_MAX_LEN] 	= {0};
	int8_t plog_head[ZLOG_LOG_HEAD_LEN] 	= {0};
	all_server_info *pser	= NULL;
	con_user *pforobj = NULL;
	http_env *phttp = NULL;

	if(pserset == NULL || log_info == NULL) {
		zlog_error(DBGLOG, "---[report_log_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	xml_buf = (int8_t *)r_malloc(CONTROL_DATA_LEN);

	if(xml_buf == NULL) {
		zlog_error(DBGLOG, "---[report_log_info_process] failed, r_malloc [%s]!\n", strerror(errno));
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	ret_buf = (int8_t *)r_malloc(CONTROL_XML_DATA_LEN);

	if(NULL == ret_buf) {
		zlog_error(DBGLOG, "---[report_log_info_process] failed, r_malloc [%s]!\n", strerror(errno));
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pser	= pserset->pserinfo;
	phttp 	= &pserset->http;

	r_bzero(xml_buf, CONTROL_DATA_LEN);
	r_bzero(xml_val, XML_VALUE_MAX_LENGTH);
	package_log_info_report_data(xml_buf + CONTROL_MSGHEAD_LEN, log_info, MSGCODE_LOG_REPORT_MC);

	pthread_mutex_lock(&pser->info_m);
	r_memset(url, 0, HTTP_SERVER_URL_MAX_LEN);
	r_strcpy(url, pser->HBeatInfo.post_url);
	pthread_mutex_unlock(&pser->info_m);

	zlog_debug(OPELOG, "---[report_log_info_process]url: %s, pass_key = %s\n", url, xml_val);
	zlog_debug(OPELOG, "---[report_log_info_process]->>>>>>>>>>>> length = %d\n%s\n\n", r_strlen(xml_buf + CONTROL_MSGHEAD_LEN), xml_buf + CONTROL_MSGHEAD_LEN);

	ret_len = CONTROL_XML_DATA_LEN;
	r_memset(ret_buf, 0, CONTROL_DATA_LEN);
	ret = mid_http_post((char *)url, (char *)xml_buf + CONTROL_MSGHEAD_LEN, r_strlen(xml_buf + CONTROL_MSGHEAD_LEN), (char *)ret_buf, &ret_len);

	if(ret == -1) {
		zlog_debug(OPELOG, "---[report_log_info_process] send log to media center failed, ret = %d\n", ret);
	} else {
		zlog_debug(OPELOG, "---[report_log_info_process] send log to media center successful, ret = %d\n", ret);
	}

	pforobj = find_forward_obj(phttp->pserset, ManagePlatform, 0, 0);

	if(pforobj != NULL) {
		r_strcpy(plog_head, pforobj->log_head);
		r_bzero(xml_buf, CONTROL_DATA_LEN);
		package_log_info_report_data(xml_buf + CONTROL_MSGHEAD_LEN, log_info, MSGCODE_LOG_REPORT_MP);

		ret = send_user_xml_data(pforobj, pforobj, xml_buf, ret_buf, &ret_len);

		if(ret != STATUS_SUCCESS) {
			zlog_debug2(OPELOG, "---[report_log_info_process] send log to [platform:%d, socket:%d] failed\n", pforobj->platform, pforobj->tcp_sock);
		} else {
			zlog_debug2(OPELOG, "---[report_log_info_process] send log successful\n");
		}
	} else {
		zlog_debug2(OPELOG, "---[report_log_info_process] find_forward_obj fail\n");
	}

EXIT:

	if(xml_buf) {
		r_free(xml_buf);
	}

	if(ret_buf) {
		r_free(ret_buf);
	}

	return return_code;
}

int8_t *warn_get_current_time_info(int8_t *c_time)
{
	time_t ti;
	struct tm timep;

	if(c_time == NULL) {
		return NULL;
	}

	memset(&ti, 0x0, sizeof(time_t));
	memset(&timep, 0x0, sizeof(struct tm));

	time(&ti);
	localtime_r(&ti, &timep);

	sprintf((char *)c_time, "%04d-%02d-%02d %02d:%02d:%02d", \
	        timep.tm_year + 1900, timep.tm_mon + 1, timep.tm_mday, timep.tm_hour, timep.tm_min, timep.tm_sec);

	return c_time;
}

int32_t get_leaf_value(int8_t *value, int32_t value_len, xmlChar *key, xmlNodePtr curNode, xmlDocPtr pdoc)
{
	int32_t ret 			= -1;
	xmlNodePtr children_ptr = NULL;

	r_bzero(value, value_len);
	children_ptr = get_children_node(curNode, key);

	if(!children_ptr) {
		return 0;
	}

	ret = get_current_node_value((char *)value, value_len, pdoc, children_ptr);

	if(ret == -1) {
		return 0;
	} else {
		return 1;
	}
}


int32_t resolve_msgcode_and_passkey_or_returncode(const int8_t *xml_buf, con_user *puser,
        int32_t *msgcode, int8_t *passkey,
        int32_t *ret_code, int32_t *msgtype)
{
	parse_xml_t	*pxml		= NULL;
	xmlNodePtr	pnode		= NULL;
	int32_t 	return_code	= STATUS_FAILED;
	int32_t		ret			= 0;
	void		*xml_status	= NULL;
	int8_t		*pbuf = NULL;
	int8_t		plog_head[ZLOG_LOG_HEAD_LEN] = {0};

	if((NULL == xml_buf) || (NULL == msgcode) || (NULL == passkey
	        || NULL == ret_code) || NULL == msgtype || NULL == puser) {
		zlog_error(OPELOG, "---[resolve_msgcode_and_passkey_or_returncode] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	r_strcpy(plog_head, puser->log_head);

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	r_memset(passkey, 0, PASS_KEY_LEN);
	*msgcode = 0;
	*ret_code = -1;

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf) {
		zlog_error2(OPELOG, "---[resolve_msgcode_and_passkey_or_returncode] failed, [r_malloc] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	xml_status = init_dom_tree(pxml, (const char *)xml_buf);

	if(NULL == xml_status) {
		zlog_error2(OPELOG, "---[resolve_msgcode_and_passkey_or_returncode] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(0 == r_strcmp((const int8_t *)pxml->proot->name, (const int8_t *)REQ_VALUE)) {
		*msgtype = XML_MSG_TYPE_REQ;
	} else {
		*msgtype = XML_MSG_TYPE_RESP;
	}

	ret = is_req_msg(pxml->proot);

	if(ret < 0) {
		zlog_error2(OPELOG, "---[resolve_msgcode_and_passkey_or_returncode] failed, [is_req_msg] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(get_req_msgcode_node(&pnode, pxml->proot) == NULL) {
		zlog_error2(OPELOG, "---[resolve_msgcode_and_passkey_or_returncode] failed, [get_req_msgcode_node] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	/* msgcode */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, REQ_CODE_TCP_SIZE, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error2(OPELOG, "---[resolve_msgcode_and_passkey_or_returncode] failed, [get_current_node_value] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	*msgcode = atoi((const char *)pbuf);

	/* passkey */
	if(get_req_passkey_node(&pnode, pxml->proot) != NULL) {
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		ret = get_current_node_value((char *)pbuf, REQ_VALUE_KEY, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error2(OPELOG, "---[resolve_msgcode_and_passkey_or_returncode] failed, [get_current_node_value] error!\n");
			return_code = STATUS_FAILED;
			goto EXIT;
		}

		r_strcpy(passkey, pbuf);
	}

	/* return_code */
	if(get_resp_return_code_node(&pnode, pxml->proot) != NULL) {
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		ret = get_current_node_value((char *)pbuf, REQ_VALUE_KEY, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error2(OPELOG, "---[resolve_msgcode_and_passkey_or_returncode] failed, [get_current_node_value] error!\n");
			return_code = STATUS_FAILED;
			goto EXIT;
		}

		*ret_code = atoi((const char *)pbuf);
	}

	return_code = STATUS_SUCCESS;

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}

int32_t package_http_heart_report_xml_data(int8_t *send_buf, all_server_info *pser, int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t xmllen = 0;
	int32_t index = 0;
	int32_t i = 0;
	parse_xml_t *pxml = NULL;
	xmlChar *xmlbuf = NULL;

	xmlNodePtr	pbodynode = NULL;
	xmlNodePtr	preqynode = NULL;
	xmlNodePtr	proomnode = NULL;
	xmlNodePtr	psyntnode = NULL;

	server_info *pser_info = NULL;
	all_server_info *pall_info = NULL;
	room_info *proom_info = NULL;
	//	enc_info  *penc_info  = NULL;
	//parse_xml_t *parse_xml_cfg 		= NULL;


	struct in_addr addr;

	int8_t *pbuf = NULL;

	if(NULL == pser || NULL == send_buf) {
		zlog_error(DBGLOG, "---[package_http_heart_report_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pall_info = (all_server_info *)r_malloc(sizeof(all_server_info));

	if(NULL == pall_info) {
		zlog_error(DBGLOG, "---[package_http_heart_report_xml_data] failed, package_req_xml_data error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pthread_mutex_lock(&pser->info_m);
	r_memcpy(pall_info, pser, sizeof(all_server_info));
	pthread_mutex_unlock(&pser->info_m);

	pser_info = &pall_info->ServerInfo;

	return_code = package_req_xml_data(send_buf, RecServer, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);

	if(return_code != STATUS_SUCCESS) {
		zlog_error(DBGLOG, "---[package_http_heart_report_xml_data] failed, package_req_xml_data error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[package_http_heart_report_xml_data] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)send_buf)) {
		zlog_error(DBGLOG, "---[package_http_heart_report_xml_data] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
	preqynode = xmlNewNode(NULL, MSG_RECSERVER_STATUS_UPDATE_REQ_KEY);
	xmlAddChild(pbodynode, preqynode);

	/* 本服务器IP */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	r_memcpy(&addr, &pser_info->LanAddr, 4);
	strcpy((char *)pbuf, inet_ntoa(addr));
	package_add_xml_leaf(preqynode, MSG_RECSERVER_IP_KEY, pbuf);

	for(index = 0; index < pall_info->ServerInfo.MaxRoom; index++) {
		proom_info = &pall_info->RoomInfo[index];

		proomnode = xmlNewNode(NULL, MSG_ROOMSTATUS_KEY);
		xmlAddChild(preqynode, proomnode);

		//		zlog_debug(OPELOG, "index = %d, roomid = %d\n", index, proom_info->RoomID);

		/* room id */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom_info->RoomID);
		package_add_xml_leaf(proomnode, MSG_ROOMID_KEY, pbuf);

		/* 会议室名称 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%s", proom_info->RoomName);
		package_add_xml_leaf(proomnode, MSG_ROOMNAME_KEY, pbuf);

		/* 连接状态 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		proom_info->ConnStatus = 1;
		sprintf((char *)pbuf, "%d", proom_info->ConnStatus);
		package_add_xml_leaf(proomnode, MSG_CONNECT_STATUS_KEY, pbuf);

		/* TODO: 高中低质量 */
#if 1
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", 0);
		package_add_xml_leaf(proomnode, MSG_ROOMQUALITY_KEY, pbuf);
#endif

		/* 录制状态 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom_info->RecStatus);
		package_add_xml_leaf(proomnode, MSG_ROOM_RECSTATUS_KEY, pbuf);
#if 0

		psyntnode = xmlNewNode(NULL, CONFIG_PARAMS_PICTURE_SYNT_INFO_KEY);
		xmlAddChild(proomnode, psyntnode);

		/* 源数量 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom_info->PicSyntInfo.SourceNum);
		package_add_xml_leaf(psyntnode, MSG_PICTURE_SYNT_SOURCENUM_KEY, pbuf);

		/* 合成画面分辨率 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom_info->PicSyntInfo.Resolution);
		package_add_xml_leaf(psyntnode, MSG_PICTURE_SYNT_RESOLUTION_KEY, pbuf);

		/* 合成画面布局 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%s", proom_info->PicSyntInfo.Model);
		package_add_xml_leaf(psyntnode, MSG_PICTURE_SYNT_MODEL_KEY, pbuf);
#endif

		if(proom_info->RecStatus != 0)
			/* 非停止状态 */
		{
			/* 录制名称 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			package_add_xml_leaf(proomnode, MSG_ROOM_RECNAME_KEY,
			                     (const int8_t *)proom_info->RecName);
			/*当前录制时长*/
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			package_add_xml_leaf(proomnode, MSG_ROOM_RECTIME_KEY,
			                     (const int8_t *)proom_info->RecTime);

			/*录制开始时间*/
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			package_add_xml_leaf(proomnode, MSG_ROOM_RECSTARTTIME_KEY,
			                     (const int8_t *)proom_info->RecStartTime);

			/*RecordID*/
			r_memset(pbuf, 0, 128);
			package_add_xml_leaf(proomnode, MSG_ROOM_RECORDID_KEY,
			                     (const int8_t *)proom_info->RecordID);

			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			sprintf((char *)pbuf, "%d", proom_info->IfMark);
			package_add_xml_leaf(proomnode, MSG_ROOM_IFMARK_KEY, pbuf);
		} else {
			/* 录制名称 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			package_add_xml_leaf(proomnode, MSG_ROOM_RECNAME_KEY,
			                     (const int8_t *)"");

			/* 评教开启状态 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			sprintf((char *)pbuf, "%s", "0");
			package_add_xml_leaf(proomnode, MSG_ROOM_IFMARK_KEY, pbuf);
		}
		/*合成模式种类*/
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom_info->Mode);
		package_add_xml_leaf(proomnode, MSG_PIC_SYNT_MODE_KEY, pbuf);
		/*合成画面宽*/
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom_info->EncInfo[0].HD_QuaInfo.EncWidth);
		package_add_xml_leaf(proomnode, MSG_PIC_SYNT_WIDTH_KEY, pbuf);
		/* 合成画面高 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom_info->EncInfo[0].HD_QuaInfo.EncHeight);
		package_add_xml_leaf(proomnode, MSG_PIC_SYNT_HEIGHT_KEY, pbuf);
		/*合成画面码率*/
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom_info->EncInfo[0].HD_QuaInfo.EncBitrate);
		package_add_xml_leaf(proomnode, MSG_PIC_SYNT_RATE_KEY, pbuf);
		/*合成画面帧率*/
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom_info->EncInfo[0].HD_QuaInfo.EncFrameRate);
		package_add_xml_leaf(proomnode, MSG_PIC_SYNT_FRAMERATE_KEY, pbuf);

		for(i = 0; i < RECORD_ROOM_MAX_ENC_NUM; i++) {
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			sprintf((char *)pbuf, "%d", proom_info->EncInfo[i].Status);

			if(i == 0) {
				package_add_xml_leaf(proomnode, MSG_ROOM_ENC1_STATUS_KEY, pbuf);
			} else if(i == 1) {
				package_add_xml_leaf(proomnode, MSG_ROOM_ENC2_STATUS_KEY, pbuf);
			} else if(i == 2) {
				package_add_xml_leaf(proomnode, MSG_ROOM_ENC3_STATUS_KEY, pbuf);
			} else if(i == 3) {
				package_add_xml_leaf(proomnode, MSG_ROOM_ENC4_STATUS_KEY, pbuf);
			}
		}
	}

#if 0
	xmlNodePtr	pwarnsnode = NULL;
	Warn_t	warn_info;
	xmlNodePtr	ftp_files_list_node = NULL;
	xmlNodePtr	ftp_file_node		= NULL;
	int32_t ftp_file_num = 0;
	int32_t j = 0;
	int32_t ret = 0;
	int8_t fail_file[XML_VALUE_MAX_LENGTH] = {0};
	int8_t fail_rson[XML_VALUE_MAX_LENGTH] = {0};

	pwarnsnode = xmlNewNode(NULL, MSG_WARN_WARNINGS_KEY);
	xmlAddChild(preqynode, pwarnsnode);


	/*ADD Warns*/
	for(index = 0; index < pall_info->ServerInfo.MaxRoom; index++) {
		proom_info = &pall_info->RoomInfo[index];

		if(proom_info->ConnStatus == 0) {
			continue;
		}

		//告警1 : 磁盘空间不足
		if(pall_info->SysInfo.DiskAvailableSpace < RECORD_DISK_MIN_AVALIABLE_SPACE) {
			r_memset(&warn_info, 0x0, sizeof(Warn_t));
			sprintf((char *)warn_info.ID, "%d", WARNID_SHORTAGE_AVALIABLE_SPACE);
			sprintf((char *)warn_info.Source, "%s", WARN_SOURCE_CAPTURE);
			warn_get_current_time_info(warn_info.Time);
			package_add_warn_node(pwarnsnode, &warn_info);
		}

		for(i = 0; i < RECORD_ROOM_MAX_ENC_NUM; i++) {
			penc_info = &proom_info->EncInfo[i];

			if(r_strlen(penc_info->EncIP) == 0) {
				continue;
			}

			//告警2 : 编码器连接异常
			if(penc_info->Status == 0) {
				r_memset(&warn_info, 0x0, sizeof(Warn_t));
				sprintf((char *)warn_info.RoomID, "%d", proom_info->RoomID);
				sprintf((char *)warn_info.ID, "%d", WARNID_ENC_CONNECTION_UNUSUAL);
				sprintf((char *)warn_info.Source, "%s", WARN_SOURCE_CAPTURE);
				sprintf((char *)warn_info.CodecID, "%d", penc_info->ID);
				warn_get_current_time_info(warn_info.Time);
				package_add_warn_node(pwarnsnode, &warn_info);
			}

			//告警4 : 编码器视频无信号
			if(penc_info->Status == 1 && penc_info->vstatus == 0) {
				r_memset(&warn_info, 0x0, sizeof(Warn_t));
				sprintf((char *)warn_info.RoomID, "%d", proom_info->RoomID);
				sprintf((char *)warn_info.ID, "%d", WARNID_ENC_SOURCE_UNUSUAL);
				sprintf((char *)warn_info.Source, "%s", WARN_SOURCE_CAPTURE);
				sprintf((char *)warn_info.CodecID, "%d", penc_info->ID);
				warn_get_current_time_info(warn_info.Time);
				package_add_warn_node(pwarnsnode, &warn_info);
			}
		}

		//告警3 : 无音量------无音频信号或静音, 音量只判断第一个编码器, fix me...........无音频信息???????
		if((r_strlen(proom_info->EncInfo[0].EncIP) > 0 && \
		    proom_info->EncInfo[0].Status == 1 && proom_info->EncInfo[0].mute == 0)) {
			r_memset(&warn_info, 0x0, sizeof(Warn_t));
			sprintf((char *)warn_info.RoomID, "%d", proom_info->RoomID);
			sprintf((char *)warn_info.ID, "%d", WARNID_VOLUME_UNUSUAL);
			sprintf((char *)warn_info.Source, "%s", WARN_SOURCE_CAPTURE);
			warn_get_current_time_info(warn_info.Time);
			package_add_warn_node(pwarnsnode, &warn_info);
		}
	}

#endif

#if 0

	//告警5 : FTP 上传失败
	if(pser->parse_xml_ftp == NULL) {
		pser->parse_xml_ftp = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
		pthread_mutex_lock(&(pser->ftp_file_m));

		if(NULL == init_file_dom_tree(pser->parse_xml_ftp, FTP_FAIL_FILE_LIST_FILE)) {
			check_ftp_file((int8_t *)FTP_FAIL_FILE_LIST_FILE);
			pthread_mutex_unlock(&(pser->ftp_file_m));
			zlog_error(DBGLOG, "---[package_http_heart_report_xml_data] init_file_dom_tree failed!\n");
			goto EXIT2;
		}

		pthread_mutex_unlock(&(pser->ftp_file_m));
	}

	ftp_files_list_node = pser->parse_xml_ftp->proot;

	ftp_file_node = get_children_node(ftp_files_list_node, MSG_FTPFILE_KEY);

	if(ftp_file_node) {
		ftp_file_num = get_current_samename_node_nums(ftp_file_node);

		for(j = 0; j < ftp_file_num && ftp_file_node != NULL; j++) {
			ret = get_leaf_value(fail_file, XML_VALUE_MAX_LENGTH, MSG_FTP_FILENAME_KEY, ftp_file_node, pser->parse_xml_ftp->pdoc);

			if(ret == 0) {
				continue;
			}

			ret = get_leaf_value(fail_rson, XML_VALUE_MAX_LENGTH, MSG_FTP_REASON_KEY  , ftp_file_node, pser->parse_xml_ftp->pdoc);

			if(ret == 0) {
				continue;
			}

			if(access((char *)fail_file, F_OK) != 0) {
				pthread_mutex_lock(&(pser->ftp_file_m));
				modify_ftp_fail_file_node(&(pser->parse_xml_ftp), fail_file, (int8_t *)"", MODIFY_TYPE_DELETE_NODE);
				pthread_mutex_unlock(&(pser->ftp_file_m));
				continue;
			}

			r_memset(&warn_info, 0x0, sizeof(Warn_t));
			sprintf((char *)warn_info.ID, "%d", WARNID_FTP_FAILED);
			sprintf((char *)warn_info.Source, "%s", WARN_SOURCE_CAPTURE);
			sprintf((char *)warn_info.Content, "%s:%s", fail_file, fail_rson);
			warn_get_current_time_info(warn_info.Time);
			package_add_warn_node(pwarnsnode, &warn_info);

			ftp_file_node = find_next_node(ftp_file_node, MSG_FTPFILE_KEY);
		}
	}

#endif

#if 0

	if(pser->parse_xml_ftp->pdoc != NULL) {
		release_dom_tree(pser->parse_xml_ftp->pdoc);
	}

	if(pser->parse_xml_ftp) {
		r_free(pser->parse_xml_ftp);
	}

#endif

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xmlbuf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xmlbuf, xmllen);
	xmlFree(xmlbuf);

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pall_info) {
		r_free(pall_info);
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;


	return return_code;
}


int32_t package_upload_status_report_xml_data(int8_t *out_buf, platform_em platform,
        int32_t msgcode, file_user *pfile_user)
{
	int32_t 	return_code = STATUS_FAILED;
	int32_t 	xmllen = 0;
	int32_t 	ret = 0;

	int8_t		msgcode_buf[16] = {0};
	int8_t		passkey[PASS_KEY_LEN];
	int8_t		*pfilename = NULL;

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;
	xmlNodePtr	req_node = NULL;

	if(NULL == out_buf || NULL == pfile_user) {
		zlog_error(DBGLOG, "---[package_upload_status_report_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	sprintf((char *)msgcode_buf, "%d", msgcode);
	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	ret = get_passkey_from_platform(platform, passkey);

	if(0 == ret) {
		package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, passkey);
	}

	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	req_node = xmlNewNode(NULL, MSG_FILE_UPLOAD_REQ_KEY);
	xmlAddChild(body_node, req_node);

	/* 录制ID */
	package_add_xml_leaf(req_node, MSG_RECORDID_KEY, (int8_t *)pfile_user->recordid);

	/* 录制文件名 */
	pfilename = r_strrchr(pfile_user->filename, '/');

	if((pfilename != NULL) && (r_strlen(pfilename) > 1)) {
		pfilename++;
		package_add_xml_leaf(req_node, MSG_FILE_KEY, (int8_t *)pfilename);
	} else {
		package_add_xml_leaf(req_node, MSG_FILE_KEY, (int8_t *)pfile_user->filename);
	}

	/* FTP服务器IP */
	package_add_xml_leaf(req_node, MSG_FTP_SERVERIP_KEY, (int8_t *)pfile_user->serverip);

	/* 上传文件的路径 */
	package_add_xml_leaf(req_node, MSG_FTP_UPLOAD_PATH_KEY, (int8_t *)pfile_user->path);

	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	return return_code;;
}

int32_t save_buf_to_file(int8_t *file, int8_t *buf, int32_t len)
{
	int32_t fd = 0;
	int32_t ret = 0;

	if(file == NULL || buf == NULL) {
		return -1;
	}

	fd = open((char *)file, O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0755);

	if(fd < 0) {
		zlog_error(DBGLOG, "---[save_buf_to_file] r_open, [%s]\n", strerror(errno));
		return -1;
	}

	ret = write(fd, buf, len);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[save_buf_to_file] r_write, [%s]\n", strerror(errno));
		r_close(fd);
		return -1;
	}

	r_close(fd);
	return 0;
}

void check_ftp_file(int8_t *file)
{
	int32_t fd = 0;
	int32_t 	xmllen		= 0;
	xmlChar 	*xml_buf	= NULL;
	xmlDocPtr	pxml		= NULL;
	xmlNodePtr	tmp_node 	= NULL;

	fd = r_open(file, O_RDONLY);

	if(fd >= 0) {
		r_close(fd);
		return;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//FtpFilesList
	tmp_node = xmlNewNode(NULL, MSG_FTPFILESLIST_KEY);
	xmlDocSetRootElement(pxml, tmp_node);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	save_buf_to_file(file, (int8_t *)xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL) {
		release_dom_tree(pxml);
	}

	return;
}

xmlNodePtr get_ftp_file_node(parse_xml_t *parseXml, int8_t *ftp_file)
{
	int32_t 	ret			= 0;
	int8_t 		tmp_buf[XML_VALUE_MAX_LENGTH]	= {0};
	xmlNodePtr	tmp_node 	= NULL;
	xmlNodePtr 	ret_val		= NULL;

	if(parseXml == NULL || ftp_file == NULL) {
		return NULL;
	}

	tmp_node = get_children_node(parseXml->proot, MSG_FTPFILE_KEY);

	if(!tmp_node) {
		return NULL;
	}

	for(; tmp_node != NULL;) {
		ret = get_leaf_value(tmp_buf, XML_VALUE_MAX_LENGTH, MSG_FTP_FILENAME_KEY, tmp_node, parseXml->pdoc);

		if(ret == 1)
			if(!r_strcmp(tmp_buf, ftp_file)) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = find_next_node(tmp_node, MSG_FTPFILE_KEY);
	}

	return ret_val;
}

int8_t *get_file_name_from_path(int8_t *path, int8_t *file)
{
	int32_t filelen;
	int8_t *ptmp = NULL;

	if(file == NULL || path == NULL) {
		return NULL;
	}

	filelen = r_strlen(path);

	if(filelen > 0) {
		ptmp = (int8_t *)path + filelen - 1;

		if(*ptmp == '/') {
			*ptmp = '\0';
		}

		r_memset(file, 0, XML_VALUE_MAX_LENGTH);

		ptmp = r_strrchr(path, '/');

		if(ptmp) {
			sprintf((char *)file, "%s", ptmp + 1);
		} else {
			sprintf((char *)file, "%s", path);
		}
	} else {
		return NULL;
	}

	return file;
}

//opt:1 添加，opt:0 删除
int32_t modify_ftp_fail_file_node(parse_xml_t **parse_xml_cfg, int8_t *ftp_fail_file, int8_t *ftp_fail_reason, ModifyNode_T opt)
{
	int32_t xml_Len 	= 0;
	int8_t  ftp_fail_time[64]			= {0};
	int32_t 	return_code 		= STATUS_FAILED;
	parse_xml_t *parse_xml_cfg_tmp 	= NULL;
	xmlNodePtr  ftp_files_list_node = NULL;
	xmlNodePtr  ftp_file_node		= NULL;
	xmlNodePtr  tmp_node			= NULL;
	xmlChar *xmlbuf 	= NULL;

	if(NULL == ftp_fail_file || NULL == ftp_fail_reason) {
		zlog_error(DBGLOG, "---[package_ftp_file_fail_info] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	//check_ftp_file((int8_t *)FTP_FAIL_FILE_LIST_FILE);

	if(*parse_xml_cfg == NULL) {
		*parse_xml_cfg = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

		if(NULL == init_file_dom_tree(*parse_xml_cfg, FTP_FAIL_FILE_LIST_FILE)) {
			unlink((char *)FTP_FAIL_FILE_LIST_FILE);
			check_ftp_file((int8_t *)FTP_FAIL_FILE_LIST_FILE);

			if(NULL == init_file_dom_tree(*parse_xml_cfg, FTP_FAIL_FILE_LIST_FILE)) {
				zlog_error(DBGLOG, "---[package_ftp_file_fail_info] init_file_dom_tree failed!\n");
				return_code = STATUS_FAILED;
				goto EXIT;
			}
		}
	}

	parse_xml_cfg_tmp = *parse_xml_cfg;
	ftp_files_list_node	= parse_xml_cfg_tmp->proot;

	if(r_strcmp((int8_t *)ftp_files_list_node->name, (int8_t *)MSG_FTPFILESLIST_KEY)) {
		zlog_error(DBGLOG, "---[package_ftp_file_fail_info] proot name isn't [FtpFilesList]!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	ftp_file_node = get_ftp_file_node(*parse_xml_cfg, ftp_fail_file);

	/*添加节点*/
	if(opt == MODIFY_TYPE_ADD_NODE) {
		/*已存在则更新, 否则添加*/
		if(ftp_file_node) {
			//更新失败原因
			tmp_node = get_children_node(ftp_file_node, MSG_FTP_REASON_KEY);

			if(tmp_node) {
				modify_node_value(tmp_node, (const xmlChar *)ftp_fail_reason);
			}

			//更新时间
			warn_get_current_time_info(ftp_fail_time);
			tmp_node = get_children_node(ftp_file_node, MSG_FTP_TIME_KEY);

			if(tmp_node) {
				modify_node_value(tmp_node, (const xmlChar *)ftp_fail_time);
			}
		} else {
			//添加新FTP 失败文件节点
			ftp_file_node = xmlNewNode(NULL, MSG_FTPFILE_KEY);
			xmlAddChild(ftp_files_list_node, ftp_file_node);

			warn_get_current_time_info(ftp_fail_time);
			package_add_xml_leaf(ftp_file_node, MSG_FTP_TIME_KEY, ftp_fail_time);
			package_add_xml_leaf(ftp_file_node, MSG_FTP_FILENAME_KEY, ftp_fail_file);
			package_add_xml_leaf(ftp_file_node, MSG_FTP_REASON_KEY, ftp_fail_reason);
		}
	} else if(opt == MODIFY_TYPE_DELETE_NODE)
		/*删除节点*/
	{
		if(ftp_file_node) {
			del_cur_node(ftp_file_node);
		}
	}

	xmlDocDumpFormatMemoryEnc(parse_xml_cfg_tmp->pdoc, &xmlbuf, &xml_Len, XML_TEXT_CODE_TYPE, 1);

	save_buf_to_file((int8_t *)FTP_FAIL_FILE_LIST_FILE, (int8_t *)xmlbuf, xml_Len);

	xmlFree(xmlbuf);


EXIT:

#if 0

	if(parse_xml_cfg->pdoc != NULL) {
		release_dom_tree(parse_xml_cfg->pdoc);
	}

	if(parse_xml_cfg) {
		r_free(parse_xml_cfg);
	}

#endif

	return return_code;;
}

int32_t get_opt_type_from_connect_room_resp(int8_t *xml_buf)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr pret_node = NULL;
	int8_t *pbuf = NULL;

	if(NULL == xml_buf) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)xml_buf)) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);

	if(!pnode) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, [get_children_node] [MSG_BODY_KEY]error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pret_node = get_children_node(pnode, MSG_OPT_TYPE_KEY);

	if(!pnode) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, [get_children_node] [MSG_OPT_TYPE_KEY] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pret_node);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp], passkey not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = atoi((const char *)pbuf);

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pxml) {
		r_free(pxml);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	pxml = NULL;

	return return_code;
}

int32_t get_opt_type_from_record_req(int8_t *xml_buf)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr pret_node = NULL;
	int8_t *pbuf = NULL;

	if(NULL == xml_buf) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)xml_buf)) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);

	if(!pnode) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, [get_children_node] [MSG_BODY_KEY]error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pnode, MSG_RECCTRL_REQ_KEY);

	if(!pnode) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, [get_children_node] [MSG_RECCTRL_REQ_KEY]error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pret_node = get_children_node(pnode, MSG_OPT_TYPE_KEY);

	if(!pnode) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp] failed, [get_children_node] [MSG_OPT_TYPE_KEY] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pret_node);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_opt_type_from_connect_room_resp], passkey not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = atoi((const char *)pbuf);

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pxml) {
		r_free(pxml);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	pxml = NULL;

	return return_code;
}

int32_t get_pass_key_from_record_ctrl_resp(int8_t *xml_buf, int8_t *pass_key)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr pret_node = NULL;
	int8_t *pbuf = NULL;

	if(NULL == xml_buf) {
		zlog_error(DBGLOG, "---[get_opt_type_from_record_ctrl_resp] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[get_opt_type_from_record_ctrl_resp] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)xml_buf)) {
		zlog_error(DBGLOG, "---[get_opt_type_from_record_ctrl_resp] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);

	if(!pnode) {
		zlog_error(DBGLOG, "---[get_opt_type_from_record_ctrl_resp] failed, [get_children_node] [MSG_BODY_KEY]error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pret_node = get_children_node(pnode, MSG_RECORD_REPORT_KEY);

	if(!pnode) {
		zlog_error(DBGLOG, "---[get_opt_type_from_record_ctrl_resp] failed, [get_children_node] [MSG_RECORD_REPORT_KEY]error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pret_node = get_children_node(pret_node, MSG_PASSKEY_KEY);

	if(!pnode) {
		zlog_error(DBGLOG, "---[get_opt_type_from_record_ctrl_resp] failed, [get_children_node] [MSG_PASSKEY_KEY] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pret_node);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_opt_type_from_record_ctrl_resp], passkey not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	//	return_code = atoi((const char *)pbuf);
	r_strcpy(pass_key, pbuf);

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pxml) {
		r_free(pxml);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	pxml = NULL;

	return return_code;
}

int32_t get_pass_key_from_resp(int8_t *xml_buf, int8_t *pass_key)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr pret_node = NULL;
	int8_t *pbuf = NULL;

	if(NULL == xml_buf) {
		zlog_error(DBGLOG, "---[get_pass_key_from_resp] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[get_pass_key_from_resp] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)xml_buf)) {
		zlog_error(DBGLOG, "---[get_pass_key_from_resp] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_HEAD_KEY);

	if(!pnode) {
		zlog_error(DBGLOG, "---[get_pass_key_from_resp] failed, [get_children_node] [MSG_BODY_KEY]error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pret_node = get_children_node(pnode, MSG_PASSKEY_KEY);

	if(!pnode) {
		zlog_error(DBGLOG, "---[get_pass_key_from_resp] failed, [get_children_node] [MSG_PASSKEY_KEY] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pret_node);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_pass_key_from_resp], passkey not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	//	return_code = atoi((const char *)pbuf);
	r_strcpy(pass_key, pbuf);

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pxml) {
		r_free(pxml);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	pxml = NULL;

	return return_code;
}


/* 确保recv_buf的长度要大于 CONTROL_DATA_LEN */
int32_t modify_resp_xml_data_passkey(int8_t *recv_buf, int8_t *passkey)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr puserid_node = NULL;
	xmlNodePtr passkey_node = NULL;
	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	if(NULL == recv_buf || NULL == passkey) {
		zlog_error(DBGLOG, "---[modify_resp_xml_data_passkey] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[modify_resp_xml_data_passkey] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_HEAD_KEY);
	passkey_node = get_children_node(pnode, MSG_PASSKEY_KEY);

	/* TODO:修改成功了吗 */
	modify_node_value(passkey_node, (const xmlChar *)passkey);

	puserid_node = get_children_node(pxml->proot, MSG_USERID_KEY);

	if(puserid_node) {
		del_cur_node(puserid_node);
	}

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	/* FIXME，确保recv_buf的长度要大于 CONTROL_DATA_LEN */
	r_memset(recv_buf, 0, CONTROL_DATA_LEN - headlen);

	if(xmllen > (CONTROL_DATA_LEN - headlen)) {
		xmllen = CONTROL_DATA_LEN - headlen;
	}

	r_memcpy(recv_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t get_passkey_and_userid_from_resp_xml_buf(int8_t *xml_buf, int8_t *passkey, int32_t *userid)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr passkey_node = NULL;
	xmlNodePtr puserid_node = NULL;

	int8_t *pbuf = NULL;

	if(NULL == xml_buf || NULL == passkey || NULL == userid) {
		zlog_error(DBGLOG, "---[get_passkey_and_userid_from_resp_xml_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf) {
		zlog_error(DBGLOG, "---[get_passkey_and_userid_from_resp_xml_buf] failed, malloc error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml, (const char *)xml_buf)) {
		zlog_error(DBGLOG, "---[get_passkey_and_userid_from_resp_xml_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_HEAD_KEY);
	passkey_node = get_children_node(pnode, MSG_PASSKEY_KEY);

	ret = get_current_node_value((char *)passkey, 64, pxml->pdoc, passkey_node);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_passkey_and_userid_from_resp_xml_buf], passkey not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	puserid_node = get_children_node(pxml->proot, MSG_USERID_KEY);
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, puserid_node);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_passkey_and_userid_from_resp_xml_buf], userid not found!\n");
		return_code = STATUS_FAILED;
		*userid = 0;
	} else {
		*userid = atoi((const char *)pbuf);
	}

	return_code = STATUS_SUCCESS;

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t get_return_code_from_resp_xml_buf(int8_t *xml_buf)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr pret_node = NULL;
	int8_t *pbuf = NULL;

	if(NULL == xml_buf) {
		zlog_error(DBGLOG, "---[get_return_code_from_resp_xml_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[get_return_code_from_resp_xml_buf] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)xml_buf)) {
		zlog_error(DBGLOG, "---[get_return_code_from_resp_xml_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_HEAD_KEY);
	pret_node = get_children_node(pnode, MSG_RETURNCODE_KEY);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pret_node);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_return_code_from_resp_xml_buf], passkey not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = atoi((const char *)pbuf);

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pxml) {
		r_free(pxml);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	pxml = NULL;

	return return_code;
}

/* 返回值为1时表明此时有会议室在录制 */
int32_t check_all_record_status(server_set *pser)
{
	int32_t ret = 0;
	int32_t index = 0;
	all_server_info *pinfo = NULL;
	room_info *proom_info = NULL;

	if(NULL == pser) {
		zlog_error(DBGLOG, "check_all_record_status failed, pser error!\n");
		return -1;
	}

	pinfo = pser->pserinfo;

	if(NULL == pinfo) {
		zlog_error(DBGLOG, "check_all_record_status failed, pinfo error!\n");
		return -1;
	}

	pthread_mutex_lock(&(pinfo->info_m));

	for(index = 0; index < pinfo->ServerInfo.MaxRoom; index++) {
		proom_info = &pinfo->RoomInfo[index];

		if(proom_info->ConnStatus == CONTROL_TRUE) {
			if(proom_info->RecStatus == 1 || proom_info->RecStatus == 2) {
				ret = 1;
				break;
			}
		}
	}

	pthread_mutex_unlock(&(pinfo->info_m));

	return ret;
}

int32_t init_server_info(server_info *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_server_info failed, paras is NULL!\n");
		return -1;
	}

	r_memset((int8_t *)pinfo->ServerType, 0, CONTROL_VERSION_LEN);
	r_memset((int8_t *)pinfo->ServerSeries, 0, CONTROL_VERSION_LEN);
	r_memset((int8_t *)pinfo->ServerVersion, 0, CONTROL_VERSION_LEN);

	r_strcpy((int8_t *)pinfo->ServerType, (const int8_t *)RECORD_SERVRE_TYPE);
	r_strcpy((int8_t *)pinfo->ServerSeries, (const int8_t *)RECORD_SERVER_SERIES);
	r_strcpy((int8_t *)pinfo->ServerVersion, (const int8_t *)RECORD_SERVER_VERSION);

	pinfo->MaxRoom = CONTROL_DEFAULT_ROOM_COUNT;
	pinfo->MaxCodecNumInRoom = RECORD_ROOM_MAX_ENC_NUM;

	pinfo->LanAddr = inet_addr("192.168.4.179");
	pinfo->LanGateWay = inet_addr("192.168.4.254");
	pinfo->LanNetmask = inet_addr("255.255.255.0");
	r_memset(pinfo->LanMac, 0, 24);
	r_strcpy((int8_t *)pinfo->LanMac, (const int8_t *)"00:11:22:33:44:55");

	pinfo->WanAddr = inet_addr("0.0.0.0");
	pinfo->WanGateWay = inet_addr("0.0.0.0");
	pinfo->WanNetmask = inet_addr("0.0.0.0");
	r_memset(pinfo->WanMac, 0, 24);
	r_strcpy((int8_t *)pinfo->WanMac, (const int8_t *)"00:00:00:00:00:00");


	return 0;
}

int32_t init_config_info(config_info *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_config_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->language 	= 0;
	pinfo->Laniptype 	= 1;
	pinfo->Waniptype 	= 0;
	pinfo->media_addr 	= inet_addr("0.0.0.0");;
	pinfo->manager_addr = inet_addr("0.0.0.0");;


	return 0;
}

int32_t init_camctrl_info(camera_ctrl *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_camctrl_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->video0_addr = 1;
	pinfo->video1_addr = 2;
	pinfo->video2_addr = 3;

	return 0;
}


int32_t init_audio_info(audio_info *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_audio_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->InputMode = 0;
	pinfo->SampleRate = 44100;
	pinfo->Bitrate = 64000;
	pinfo->Lvolume = 80;
	pinfo->Rvolume = 80;
	r_strcpy(pinfo->InputIndex, (int8_t *)"1111");

	return 0;
}

int32_t init_pic_synt_info(pic_synt_info *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_pic_synt_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->SourceNum = 3;
	pinfo->Resolution = 3;
	pinfo->LayoutDef_2 = 21;
	pinfo->LayoutDef_3 = 31;
	r_strcpy(pinfo->Model, (int8_t *)"31");

	return 0;
}


int32_t init_quality_info(quality_info *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_quality_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->enable = 1;
	pinfo->RateType = 0;
	pinfo->EncBitrate = 2048;
	pinfo->EncWidth = 1920;
	pinfo->EncHeight = 1080;
	pinfo->EncFrameRate = 30;

	return 0;
}

static int32_t init_SDquality_info(quality_info *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_quality_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->enable = 1;
	pinfo->RateType = 0;
	pinfo->EncBitrate = 512;
	pinfo->EncWidth = 352;
	pinfo->EncHeight = 288;
	pinfo->EncFrameRate = 30;

	return 0;
}

int32_t init_enc_info(enc_info *pinfo, int32_t index)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_enc_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->ID = index;
	//	pinfo->EncIP = inet_addr("192.168.0.101");
	r_memset(pinfo->EncIP, 0, IPINFO_MAX_LENGTH);

	switch(index) {
		case 1:
		case 2:
		case 3:
			r_strcpy(pinfo->EncIP, (const int8_t *)HD_ENC_IP);
			break;

		case 4:
		case 5:
			r_strcpy(pinfo->EncIP, (const int8_t *)SD_ENC_IP);
			break;

		case 6:
			r_strcpy(pinfo->EncIP, (const int8_t *)IPCAMERA_IP);
			break;

		case 7:
			r_strcpy(pinfo->EncIP, (const int8_t *)IPCAMERA_IP);
			break;

		case 8:
			r_strcpy(pinfo->EncIP, (const int8_t *)IPCAMERA_IP);
			break;

		case 9:
			r_strcpy(pinfo->EncIP, (const int8_t *)IPCAMERA_IP);
			break;

		default:
			r_strcpy(pinfo->EncIP, (const int8_t *)INVALIID_IP);
			break;
	}

	pinfo->Status = 1;

	init_quality_info(&pinfo->HD_QuaInfo);
	pinfo->HD_QuaInfo.RateType = 0;
	init_SDquality_info(&pinfo->SD_QuaInfo);
	pinfo->SD_QuaInfo.RateType = 1;
	pinfo->SD_QuaInfo.EncBitrate = 512;

	return 0;
}

int32_t init_room_info(room_info *pinfo, int32_t index)
{
	int32_t i = 0;

	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_room_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->RoomID = index;
	pinfo->ConnStatus = 1;
	pinfo->RecordMaxTime = 8;
	pinfo->RecStatus = 0;

	r_memset(pinfo->RoomName, 0, ROOM_NAME_MAX_LENGTH);
	r_memset(pinfo->RecName, 0, RECORD_FILE_MAX_FILENAME);

	r_strcpy((int8_t *)pinfo->RoomName, (int8_t *)ROOM_DEFAULT_NAME);

	pinfo->Mode = 0;
	//init_pic_synt_info(&pinfo->PicSyntInfo);

	init_audio_info(&pinfo->AudioInfo);

	for(i = 0; i < RECORD_ROOM_MAX_ENC_NUM; i++) {
		init_enc_info(&pinfo->EncInfo[i], i + 1);
	}

	return 0;
}

int32_t init_user_info(user_info *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_user_info failed, paras is NULL!\n");
		return -1;
	}

	r_strcpy((int8_t *)pinfo->username, (const int8_t *)CONTROL_DEFAULT_USERNAME);
	r_strcpy((int8_t *)pinfo->password, (const int8_t *)CONTROL_DEFAULT_PASSWORD);
	r_strcpy((int8_t *)pinfo->guest_name, (const int8_t *)CONTROL_DEFAULT_GUESTNAME);
	r_strcpy((int8_t *)pinfo->guest_passwd, (const int8_t *)CONTROL_DEFAULT_GUESTPASSWD);

	return 0;
}

int32_t init_heart_beat_info(heart_beat_info *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_heart_beat_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->post_time = HTTP_SERVER_POST_DEFAULT_INTERTIME;
	r_memset(pinfo->post_url, 0, HTTP_SERVER_URL_MAX_LEN);
	//	r_strcpy(pinfo->post_url, (const int8_t *)"http://192.168.4.88/HttpService.action");
	r_memset(pinfo->time_serip, 0, 64);

	return 0;
}

int32_t init_ftp_info(ftp_info *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_ftp_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->Mode = FTP_MODE_MEDIACENTER;
	pinfo->MCFTPPort = 21;

	r_memset(pinfo->MCFTPAddr, 0, 24);
	r_strcpy((int8_t *)pinfo->MCFTPAddr, (const int8_t *)"172.16.4.102");

	r_memset(pinfo->MCFTPUserName, 0, FTP_MAX_USERNAME_LENGTH);
	r_strcpy((int8_t *)pinfo->MCFTPUserName, (const int8_t *)"reach");

	r_memset(pinfo->MCFTPPassword, 0, FTP_MAX_PASSWD_LENGTH);
	r_strcpy((int8_t *)pinfo->MCFTPPassword, (const int8_t *)"reach");

	r_memset(pinfo->MCFTPPath, 0, FTP_MAX_FTPPATH_LENGTH);
	r_strcpy((int8_t *)pinfo->MCFTPPath, (const int8_t *)"/");


	r_memset(pinfo->THRFTPAddr, 0, 24);
	r_strcpy((int8_t *)pinfo->THRFTPAddr, (const int8_t *)"172.16.4.102");

	r_memset(pinfo->THRFTPUserName, 0, FTP_MAX_USERNAME_LENGTH);
	r_strcpy((int8_t *)pinfo->THRFTPUserName, (const int8_t *)"reach");

	r_memset(pinfo->THRFTPPassword, 0, FTP_MAX_PASSWD_LENGTH);
	r_strcpy((int8_t *)pinfo->THRFTPPassword, (const int8_t *)"reach");

	r_memset(pinfo->THRFTPPath, 0, FTP_MAX_FTPPATH_LENGTH);
	r_strcpy((int8_t *)pinfo->THRFTPPath, (const int8_t *)"/");

	return 0;
}



int32_t init_sys_info(sys_info *pinfo)
{
	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_sys_info failed, paras is NULL!\n");
		return -1;
	}

	pinfo->TimeServerAddr = inet_addr("192.168.0.128");
	pinfo->DiskMaxSpace = 0;
	pinfo->DiskAvailableSpace = 0;

	return 0;
}

int32_t init_all_server_info(all_server_info *pinfo)
{
	int32_t index = 0;

	if(NULL == pinfo) {
		zlog_error(DBGLOG, "init_all_server_info failed, paras is NULL!\n");
		return -1;
	}

	pthread_mutex_init(&(pinfo->info_m), NULL);
	pthread_mutex_init(&(pinfo->ftp_file_m), NULL);
	init_user_info(&pinfo->Authentication);
	init_camctrl_info(&pinfo->CamCtrlInfo);
	init_heart_beat_info(&pinfo->HBeatInfo);
	init_ftp_info(&pinfo->FtpInfo);
	init_sys_info(&pinfo->SysInfo);
	init_server_info(&pinfo->ServerInfo);
	init_config_info(&pinfo->ConfigInfo);

	for(index = 0; index < CONTROL_ROOM_SERVER_MAX_USER; index++) {
		init_room_info(&pinfo->RoomInfo[index], index);
	}

	return 0;
}

int32_t live_user_cmp(lives_info *plive_src, lives_info *plive_dst)
{
	int32_t return_code = -1;
	int32_t index = 0;

	lives_addr *paddr_src = NULL;
	lives_addr *paddr_dst = NULL;

	if(NULL == plive_src || NULL == plive_dst) {
		zlog_error(DBGLOG, "---[live_user_cmp] failed, params is NULL!\n");
		return -1;
	}

	if(plive_src->quality_type == plive_dst->quality_type && plive_src->userid == plive_dst->userid) {
		for(index = 0; index < VIDEO_ENCODE_MAX_NUM; index++) {
			paddr_src = &plive_src->addr[index];
			paddr_dst = &plive_dst->addr[index];

			zlog_debug(OPELOG, "src, index: %d, ip: %s, port: %d\n", index, paddr_src->user_ip, paddr_src->user_port);
			zlog_debug(OPELOG, "dst, index: %d, ip: %s, port: %d\n", index, paddr_dst->user_ip, paddr_dst->user_port);

			if((r_strcmp(paddr_src->user_ip, paddr_dst->user_ip) != 0)
			   || (paddr_src->user_port != paddr_dst->user_port)) {
				return_code = -1;
				break;
			}

			return_code = 0;
		}
	}

	return return_code;
}

int32_t clean_live_user_info(lives_info *plive)
{
	int32_t return_code = STATUS_FAILED;
	int32_t index = 0;

	if(NULL == plive) {
		zlog_error(DBGLOG, "---[clean_live_user_info] failed, params is NULL!\n");
		return return_code;
	}

	plive->enable = 0;
	plive->userid = -1;
	plive->quality_type = -1;
	r_memset(plive->encode_index, 0, VIDEO_ENCODE_INDEX_LEN);

	for(index = 0; index < VIDEO_ENCODE_MAX_NUM; index++) {
		r_memset(plive->addr[index].user_ip, 0, VIDEO_SEND_IP_LEN);
		plive->addr[index].user_port = 0;
	}

	return return_code;
}

int32_t clean_user_live_user_info(control_env *penv, int32_t userid)
{
	int32_t index = 0;
	int32_t i = 0;
	int32_t j = 0;

	server_set *pser = NULL;
	control_env *pcon = NULL;
	lives_info *plive = NULL;

	if(NULL == penv) {
		zlog_error(DBGLOG, "---[clean_user_live_user_info] failed, params is NULL!\n");
		return -1;
	}

	pser = penv->pserset;

	if(NULL == pser) {
		zlog_error(DBGLOG, "---[clean_user_live_user_info] failed, pser is NULL!\n");
		return -1;
	}

	if(NULL == pser->pserinfo) {
		zlog_error(DBGLOG, "---[clean_user_live_user_info] failed, pserinfo is NULL!\n");
		return -1;
	}

	for(index = 0; index < pser->pserinfo->ServerInfo.MaxRoom; index++) {
		pcon = &pser->roomser[index];
		pthread_mutex_lock(&pcon->control_m);

		for(i = 0; i < VIDEO_USER_MAX_NUM; i++) {
			plive = &pcon->lives_user[i];

			if(plive->userid == userid) {
				zlog_debug(OPELOG, "---[clean_user_live_user_info] sucess, userid = %d\n", userid);
				plive->enable = 0;
				plive->userid = -1;
				plive->quality_type = -1;
				r_memset(plive->encode_index, 0, VIDEO_ENCODE_INDEX_LEN);

				for(j = 0; j < VIDEO_ENCODE_MAX_NUM; j++) {
					r_memset(plive->addr[j].user_ip, 0, VIDEO_SEND_IP_LEN);
					plive->addr[j].user_port = 0;
				}
			}
		}

		pthread_mutex_unlock(&pcon->control_m);
	}

	return 0;
}


int32_t save_live_user_info(control_env *penv, lives_info *live)
{
	int32_t return_code = STATUS_FAILED;
	int32_t index = 0;
	int32_t max_video = VIDEO_USER_MAX_NUM;
	lives_info *plive = NULL;

	if(NULL == penv || NULL == live) {
		zlog_error(DBGLOG, "---[save_live_user_info] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	return_code = STATUS_SUCCESS;

	if(max_video > 3) {
		max_video = 3;
	}

	pthread_mutex_lock(&penv->control_m);

	for(index = 0; index < max_video; index++) {
		plive = &penv->lives_user[index];

		if(strchr((const char *)live->encode_index, 'A') != NULL) {
			/* 此为请求发送码流 */
			if(0 == plive->enable) {
				r_memcpy(plive, live, sizeof(lives_info));
				plive->enable = 1;
				break;
			}
		} else if(strchr((const char *)live->encode_index, 'S') != NULL) {
			/* 此为请求停止码流 */
			if(1 == plive->enable) {
				if(0 == live_user_cmp(plive, live)) {
					clean_live_user_info(plive);
					plive->enable = 0;
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&penv->control_m);

	return return_code;
}

int32_t print_live_user_info(lives_info *plive)
{
	int32_t index = 0;

	if(NULL == plive) {
		zlog_error(DBGLOG, "---[common_resp_process] failed, params is NULL!\n");
		return -1;
	}

	if(plive->enable == 1) {
		zlog_debug(OPELOG, "\n");
		zlog_debug(OPELOG, "enable %t = %d\n", plive->enable);
		zlog_debug(OPELOG, "userid %t = %d\n", plive->userid);
		zlog_debug(OPELOG, "quality %t = %d\n", plive->quality_type);
		zlog_debug(OPELOG, "encode_index %t = %s\n", plive->encode_index);

		for(index = 0; index < VIDEO_ENCODE_MAX_NUM; index++) {
			zlog_debug(OPELOG, "ipaddr_%d %t = %s\n", index, plive->addr[index].user_ip);
			zlog_debug(OPELOG, "port_%d %t = %d\n", index, plive->addr[index].user_port);
		}

		zlog_debug(OPELOG, "\n");
	}

	return 0;
}

int32_t forward_process(void *user_src, void *user_des, int8_t *send_buf, int32_t msgcode,
                        int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	con_user *puser_src = NULL;
	con_user *puser_des = NULL;
	int8_t passkey[64] = {0};

	if(NULL == user_des || NULL == send_buf || NULL == ret_buf || NULL == ret_len) {
		zlog_error(DBGLOG, "---[forward_process] failed, params is NULL!\n");
		return return_code;
	}

	puser_src = (con_user *)user_src;
	puser_des = (con_user *)user_des;

	get_passkey_from_platform(puser_des->platform, passkey);

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};

	if(puser_src) {
		r_strcpy(plog_head, puser_src->log_head);

		if(RecServer == puser_des->platform) {
			zlog_debug2(OPELOG, "-===-{forwarding [%s:%d] to [%s %d][tcp_sock = %d]}-===\n",
			            find_treaty_text(msgcode), msgcode, passkey, puser_des->port - CONTROL_ROOM_SERVER_PORT_START, puser_des->tcp_sock);
		} else {
			zlog_debug2(OPELOG, "-===-{forwarding [%s:%d] to [%s %d <d:%d s:%d>]}-===\n",
			            find_treaty_text(msgcode), msgcode, passkey, puser_des->index, puser_des->tcp_sock, puser_src->tcp_sock);
		}
	}

	pthread_mutex_lock(&puser_des->sock_m);
	return_code = send_user_xml_data(puser_src, puser_des, send_buf, ret_buf, ret_len);
	pthread_mutex_unlock(&puser_des->sock_m);

	return return_code;
}

int32_t report_set_room_info_log(control_env *penv, con_user *puser, int8_t *recv_buf, platform_em platform)
{
	Log_t log_info;
	int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
	int32_t return_code = STATUS_FAILED;
	int32_t userid = 0;
	//	platform_em platform;
	int8_t *ipaddr = NULL;

	if(penv == NULL || puser == NULL || recv_buf == NULL) {
		zlog_error(OPELOG, "---[report_set_room_info_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}

	r_bzero(&log_info, sizeof(Log_t));

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN, pass_key, &userid);
	//	platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

	ipaddr = timeout_get_con_user_addr(&penv->pserset->timeque, platform, userid, log_info.Addr);

	if(ipaddr == NULL) {
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	}

	sprintf((char *)log_info.Type, "%d", LOG_TYPE_SET_ROOM_INFO);
	warn_get_current_time_info(log_info.Time);
	//	get_pass_key_from_resp(recv_buf+CONTROL_MSGHEAD_LEN, pass_key);
	trans_platform_to_log_info_user2(pass_key, log_info.User);
	return_code = get_return_code_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN);

	if(1 == return_code) {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);

	return STATUS_SUCCESS;
}

int32_t report_set_room_quality_log(control_env *penv, con_user *puser, int8_t *recv_buf, platform_em platform)
{
	Log_t log_info;
	int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
	int32_t return_code = STATUS_FAILED;

	int32_t userid = 0;
	//	platform_em platform;
	int8_t *ipaddr = NULL;

	if(penv == NULL || puser == NULL || recv_buf == NULL) {
		zlog_error(OPELOG, "---[report_set_room_quality_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}

	r_bzero(&log_info, sizeof(Log_t));

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN, pass_key, &userid);
	//	platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

	ipaddr = timeout_get_con_user_addr(&penv->pserset->timeque, platform, userid, log_info.Addr);

	if(ipaddr == NULL) {
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	}

	sprintf((char *)log_info.Type, "%d", LOG_TYPE_SET_QUALITY_INFO);

	warn_get_current_time_info(log_info.Time);
	//	get_pass_key_from_resp(recv_buf+CONTROL_MSGHEAD_LEN, pass_key);
	trans_platform_to_log_info_user2(pass_key, log_info.User);
	return_code = get_return_code_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN);

	if(1 == return_code) {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);

	return STATUS_SUCCESS;
}

int32_t report_set_audio_info_log(control_env *penv, con_user *puser, int8_t *recv_buf, platform_em platform)
{
	Log_t log_info;
	int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
	int32_t return_code = STATUS_FAILED;

	int32_t userid = 0;
	//	platform_em platform;
	int8_t *ipaddr = NULL;

	if(penv == NULL || puser == NULL || recv_buf == NULL) {
		zlog_error(OPELOG, "---[report_set_audio_info_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}


	r_bzero(&log_info, sizeof(Log_t));

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN, pass_key, &userid);
	//	platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

	ipaddr = timeout_get_con_user_addr(&penv->pserset->timeque, platform, userid, log_info.Addr);

	if(ipaddr == NULL) {
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	}

	sprintf((char *)log_info.Type, "%d", LOG_TYPE_SET_AUDIO_INFO);

	warn_get_current_time_info(log_info.Time);
	//	get_pass_key_from_resp(recv_buf+CONTROL_MSGHEAD_LEN, pass_key);
	trans_platform_to_log_info_user2(pass_key, log_info.User);
	return_code = get_return_code_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN);

	if(1 == return_code) {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);

	return STATUS_SUCCESS;
}

int32_t report_connect_room_req_log(control_env *penv, con_user *puser, int8_t *recv_buf, platform_em platform)
{
	Log_t 	log_info;
	int32_t opt_type 	= -1;
	int32_t return_code = STATUS_FAILED;
	int8_t 	pass_key[XML_VALUE_MAX_LENGTH] = {0};
	int8_t  opt_buf[XML_VALUE_MAX_LENGTH] = {0};

	int32_t userid = 0;
	//	platform_em platform;
	int8_t *ipaddr = NULL;

	if(penv == NULL || puser == NULL || recv_buf == NULL) {
		zlog_error(OPELOG, "---[report_connect_room_req_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}


	r_bzero(&log_info, sizeof(Log_t));

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN, pass_key, &userid);
	//	platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

	ipaddr = timeout_get_con_user_addr(&penv->pserset->timeque, platform, userid, log_info.Addr);

	if(ipaddr == NULL) {
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	}

	zlog_debug(OPELOG, "-----report_connect_room_req_log---ipaddr = [%p]-----[%d][%d]------!\n", ipaddr, platform, userid);

	warn_get_current_time_info(log_info.Time);
	//	get_pass_key_from_resp(recv_buf+CONTROL_MSGHEAD_LEN, pass_key);
	trans_platform_to_log_info_user2(pass_key, log_info.User);

	r_bzero(opt_buf, XML_VALUE_MAX_LENGTH);
	opt_type = get_opt_type_from_connect_room_resp(recv_buf + CONTROL_MSGHEAD_LEN);

	if(opt_type == 1) {
		sprintf((char *)log_info.Type, "%d", LOG_TYPE_ROOM_CONNECT);
		r_strcpy(opt_buf, (int8_t *)"Connect");
	} else if(opt_type == 0) {
		sprintf((char *)log_info.Type, "%d", LOG_TYPE_ROOM_DISCONNECT);
		r_strcpy(opt_buf, (int8_t *)"Disconnect");
	} else {
		return STATUS_FAILED;
	}

	return_code = get_return_code_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN);

	if(1 == return_code) {
		sprintf((char *)log_info.Content, "%s:%s", opt_buf, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		sprintf((char *)log_info.Content, "%s:%s", opt_buf, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);


	return STATUS_SUCCESS;
}

int32_t report_upload_logo_picture_log(control_env *penv, con_user *puser, int8_t *recv_buf, platform_em platform)
{
	Log_t 	log_info;
	int32_t return_code = STATUS_FAILED;
	int8_t 	pass_key[XML_VALUE_MAX_LENGTH] = {0};

	int32_t userid = 0;
	//	platform_em platform;
	int8_t *ipaddr = NULL;


	if(penv == NULL || puser == NULL || recv_buf == NULL) {
		zlog_error(OPELOG, "---[report_upload_logo_picture_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}

	r_bzero(&log_info, sizeof(Log_t));

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN, pass_key, &userid);
	//	platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

	ipaddr = timeout_get_con_user_addr(&penv->pserset->timeque, platform, userid, log_info.Addr);

	if(ipaddr == NULL) {
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	}

	sprintf((char *)log_info.Type, "%d", LOG_TYPE_UPLOAD_LOGO);

	warn_get_current_time_info(log_info.Time);
	//	get_pass_key_from_resp(recv_buf+CONTROL_MSGHEAD_LEN, pass_key);
	trans_platform_to_log_info_user2(pass_key, log_info.User);
	return_code = get_return_code_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN);

	if(1 == return_code) {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);

	return STATUS_SUCCESS;
}

int32_t report_video_adjust_log(control_env *penv, con_user *puser, int8_t *recv_buf, platform_em platform)
{
	Log_t	log_info;
	int32_t return_code = STATUS_FAILED;
	int8_t	pass_key[XML_VALUE_MAX_LENGTH] = {0};

	int32_t userid = 0;
	//	platform_em platform;
	int8_t *ipaddr = NULL;

	if(penv == NULL || puser == NULL || recv_buf == NULL) {
		zlog_error(OPELOG, "---[report_video_adjust_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}

	r_bzero(&log_info, sizeof(Log_t));

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN, pass_key, &userid);
	//	platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

	ipaddr = timeout_get_con_user_addr(&penv->pserset->timeque, platform, userid, log_info.Addr);

	if(ipaddr == NULL) {
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	}

	sprintf((char *)log_info.Type, "%d", LOG_TYPE_VIDEO_ADJUST_REQ);

	warn_get_current_time_info(log_info.Time);
	//	get_pass_key_from_resp(recv_buf+CONTROL_MSGHEAD_LEN, pass_key);
	trans_platform_to_log_info_user2(pass_key, log_info.User);
	return_code = get_return_code_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN);

	if(1 == return_code) {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);

	return STATUS_SUCCESS;
}


int32_t report_get_ftp_info_log(control_env *penv, con_user *puser, int8_t *recv_buf, int32_t status)
{
	Log_t	log_info;
	//	int8_t	pass_key[XML_VALUE_MAX_LENGTH] = {0};

	int8_t ipaddr[16] = {0};
	struct in_addr addr;

	if(penv == NULL || puser == NULL || recv_buf == NULL) {
		zlog_error(OPELOG, "---[report_get_ftp_info_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}

	r_memcpy(&addr, &penv->pserset->pserinfo->ServerInfo.LanAddr, 4);
	r_memset(ipaddr, 0, 16);
	r_strcpy((int8_t *)ipaddr, (const int8_t *)inet_ntoa(addr));

	r_bzero(&log_info, sizeof(Log_t));
	sprintf((char *)log_info.Type, "%d", LOG_TYPE_GET_FTP_INFO);
	sprintf((char *)log_info.Addr, "%s", ipaddr);
	sprintf((char *)log_info.User, "%s", WARN_SOURCE_CAPTURE);
	warn_get_current_time_info(log_info.Time);

	//	get_pass_key_from_record_ctrl_resp(recv_buf+CONTROL_MSGHEAD_LEN, pass_key);
	//	trans_platform_to_log_info_user2(pass_key, log_info.User);
	if(status < 0) {
		sprintf((char *)log_info.Content, "%s", (int8_t *)MSG_LOG_FAIL_CONTENT);
	} else {
		sprintf((char *)log_info.Content, "%s", (int8_t *)MSG_LOG_SUCC_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);

	return STATUS_SUCCESS;
}


int32_t report_audio_mute_log(control_env *penv, con_user *puser, int8_t *recv_buf, platform_em platform)
{
	Log_t	log_info;
	int32_t return_code = STATUS_FAILED;
	int8_t 	pass_key[XML_VALUE_MAX_LENGTH] = {0};

	int32_t userid = 0;
	//	platform_em platform;
	int8_t *ipaddr = NULL;

	if(penv == NULL || puser == NULL || recv_buf == NULL) {
		zlog_error(OPELOG, "---[report_audio_mute_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}

	r_bzero(&log_info, sizeof(Log_t));

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN, pass_key, &userid);
	//	platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

	ipaddr = timeout_get_con_user_addr(&penv->pserset->timeque, platform, userid, log_info.Addr);

	if(ipaddr == NULL) {
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	}

	sprintf((char *)log_info.Type, "%d", LOG_TYPE_MUTE_REQ);

	warn_get_current_time_info(log_info.Time);
	//	get_pass_key_from_resp(recv_buf+CONTROL_MSGHEAD_LEN, pass_key);
	trans_platform_to_log_info_user2(pass_key, log_info.User);
	return_code = get_return_code_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN);

	if(1 == return_code) {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);


	return STATUS_SUCCESS;
}


int32_t report_update_server_log(control_env *penv, con_user *puser, int32_t return_code, update_info *update)
{
	Log_t log_info;
	int8_t opt_buf[XML_VALUE_MAX_LENGTH] = {0};

	if(penv == NULL || puser == NULL || update == NULL) {
		zlog_debug(OPELOG, "---[report_update_server_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}

	zlog_debug(OPELOG, "---[report_update_server_log] ========================1===========update.Opereate = %d=========!\n", update->Opereate);

	r_bzero(opt_buf, XML_VALUE_MAX_LENGTH);

	if(update->Opereate == 0) {
		r_strcpy(opt_buf, (int8_t *)"SysUpgrade");
	} else if(update->Opereate == 1) {
		r_strcpy(opt_buf, (int8_t *)"SysRollback");
	}

	r_bzero(&log_info, sizeof(Log_t));
	sprintf((char *)log_info.Type, "%d", LOG_TYPE_UPGRADE_RECSERVER);
	sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	warn_get_current_time_info(log_info.Time);
	trans_platform_to_log_info_user(puser->platform, log_info.User);
	zlog_debug(OPELOG, "---[report_update_server_log] ========================2===========return_code = %d=========!\n", return_code);

	if(STATUS_SUCCESS == return_code) {
		sprintf((char *)log_info.Content, "%s:%s", opt_buf, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		sprintf((char *)log_info.Content, "%s:%s", opt_buf, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);


	return STATUS_SUCCESS;
}

int32_t report_update_server_log2(control_env *penv, int8_t *ipaddr, int32_t ret_val, int32_t Opereate, int8_t *pass_key)
{
	Log_t log_info;
	int8_t opt_buf[XML_VALUE_MAX_LENGTH] = {0};

	if(penv == NULL || ipaddr == NULL || pass_key == NULL) {
		zlog_debug(OPELOG, "---[report_update_server_log2] failed, params[%p][%p][%p] is NULL!\n", penv, ipaddr, pass_key);
		return STATUS_FAILED;
	}

	zlog_debug(OPELOG, "---[report_update_server_log2] ====[%s]=======3===========Opereate = %d=========!\n", ipaddr, Opereate);

	r_bzero(opt_buf, XML_VALUE_MAX_LENGTH);

	if(Opereate == 0) {
		r_strcpy(opt_buf, (int8_t *)"SysUpgrade");
	} else if(Opereate == 1) {
		r_strcpy(opt_buf, (int8_t *)"SysRollback");
	}

	r_bzero(&log_info, sizeof(Log_t));
	sprintf((char *)log_info.Type, "%d", LOG_TYPE_UPGRADE_RECSERVER);
	sprintf((char *)log_info.Addr, "%s", ipaddr);
	warn_get_current_time_info(log_info.Time);
	trans_platform_to_log_info_user2(pass_key, log_info.User);

	if(STATUS_SUCCESS == ret_val) {
		sprintf((char *)log_info.Content, "%s:%s", opt_buf, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		sprintf((char *)log_info.Content, "%s:%s", opt_buf, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	report_log_info_process(penv->pserset, &log_info);


	return STATUS_SUCCESS;
}


int32_t report_add_text_title_log(control_env *penv, con_user *puser, int8_t *recv_buf, platform_em platform)
{
	Log_t log_info;
	int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
	int32_t return_code = STATUS_FAILED;

	int32_t userid = 0;
	//	platform_em platform;
	int8_t *ipaddr = NULL;

	if(penv == NULL || puser == NULL || recv_buf == NULL) {
		zlog_error(DBGLOG, "---[report_add_text_title_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}

	r_bzero(&log_info, sizeof(Log_t));

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN, pass_key, &userid);
	//	platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

	ipaddr = timeout_get_con_user_addr(&penv->pserset->timeque, platform, userid, log_info.Addr);

	if(ipaddr == NULL) {
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	}

	sprintf((char *)log_info.Type, "%d", LOG_TYPE_ADD_TITLE);

	warn_get_current_time_info(log_info.Time);
	//	get_pass_key_from_resp(recv_buf+CONTROL_MSGHEAD_LEN, pass_key);
	trans_platform_to_log_info_user2(pass_key, log_info.User);
	return_code = get_return_code_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN);

	if(1 == return_code) {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);

	return STATUS_SUCCESS;
}

int32_t report_record_status_log(control_env *penv, con_user *puser, record_status *pres, int8_t *recv_buf)
{
	Log_t log_info;
	int8_t opt_buf[XML_VALUE_MAX_LENGTH] = {0};
	int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};

	int32_t userid = 0;
	platform_em platform;
	int8_t *ipaddr = NULL;

	if(penv == NULL || puser == NULL || recv_buf == NULL || pres == NULL) {
		zlog_error(DBGLOG, "---[report_record_status_log] failed, params is NULL!\n");
		return STATUS_FAILED;
	}

	r_bzero(&log_info, sizeof(Log_t));
	r_bzero(pass_key, XML_VALUE_MAX_LENGTH);

	if(pres->OptType == 0) {
		r_strcpy(opt_buf, (int8_t *)"Stop");
		sprintf((char *)log_info.Type, "%d", LOG_TYPE_RECORD_STOP);
	} else if(pres->OptType == 1) {
		r_strcpy(opt_buf, (int8_t *)"Start");
		sprintf((char *)log_info.Type, "%d", LOG_TYPE_RECORD_START);
	} else if(pres->OptType == 2) {
		r_strcpy(opt_buf, (int8_t *)"Pause");
		sprintf((char *)log_info.Type, "%d", LOG_TYPE_RECORD_PAUSE);
	}

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN, pass_key, &userid);
	get_pass_key_from_record_ctrl_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
	platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

	ipaddr = timeout_get_con_user_addr(&penv->pserset->timeque, platform, userid, log_info.Addr);

	if(ipaddr == NULL) {
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	}

	warn_get_current_time_info(log_info.Time);
	trans_platform_to_log_info_user2(pass_key, log_info.User);

	if(1 == pres->Result || 2 == pres->Result) {
		sprintf((char *)log_info.Content, "%s:%s", opt_buf, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		sprintf((char *)log_info.Content, "%s:%s", opt_buf, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------pass_key = [%s]-----------!\n", log_info.Type, pass_key);
	zlog_debug(OPELOG, "--------%s-------------------!\n", recv_buf + CONTROL_MSGHEAD_LEN);
	report_log_info_process(penv->pserset, &log_info);

	return STATUS_SUCCESS;
}


int32_t report_record_status_log2(control_env *penv, con_user *puser, int32_t opt_type, int8_t *recv_buf)
{
	Log_t log_info;
	int8_t opt_buf[XML_VALUE_MAX_LENGTH] = {0};
	int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
	int32_t return_code = STATUS_FAILED;

	int32_t userid = 0;
	//	platform_em platform;
	int8_t *ipaddr = NULL;

	if(penv == NULL || puser == NULL || recv_buf == NULL) {
		zlog_error(DBGLOG, "---[report_record_status_log2] failed, params is NULL!\n");
		return STATUS_FAILED;
	}

	r_bzero(&log_info, sizeof(Log_t));
	r_bzero(pass_key, XML_VALUE_MAX_LENGTH);

	if(opt_type == 0) {
		r_strcpy(opt_buf, (int8_t *)"Stop");
		sprintf((char *)log_info.Type, "%d", LOG_TYPE_RECORD_STOP);
	} else if(opt_type == 1) {
		r_strcpy(opt_buf, (int8_t *)"Start");
		sprintf((char *)log_info.Type, "%d", LOG_TYPE_RECORD_START);
	} else if(opt_type == 2) {
		r_strcpy(opt_buf, (int8_t *)"Pause");
		sprintf((char *)log_info.Type, "%d", LOG_TYPE_RECORD_PAUSE);
	} else {
		return STATUS_FAILED;
	}

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN, pass_key, &userid);
	//	platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

	ipaddr = timeout_get_con_user_addr(&penv->pserset->timeque, puser->platform, userid, log_info.Addr);

	if(ipaddr == NULL) {
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	}

	warn_get_current_time_info(log_info.Time);
	trans_platform_to_log_info_user(puser->platform, log_info.User);

	return_code = get_return_code_from_resp_xml_buf(recv_buf + CONTROL_MSGHEAD_LEN);

	if(1 == return_code) {
		sprintf((char *)log_info.Content, "%s:%s", opt_buf, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		sprintf((char *)log_info.Content, "%s:%s", opt_buf, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------pass_key = [%s]-----------!\n", log_info.Type, pass_key);
	zlog_debug(OPELOG, "--------%s-------------------!\n", recv_buf + CONTROL_MSGHEAD_LEN);
	report_log_info_process(penv->pserset, &log_info);

	return STATUS_SUCCESS;
}

int32_t common_resp_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                            int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = -1;
	int32_t userid = 0;
	int32_t index = 0;
	int32_t ret = 0;
	platform_em platform;
	con_user *pforobj = NULL;
	int8_t passkey[64] = {0};
	roomid_array array;
	int8_t		plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error2(DBGLOG, "---[common_resp_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + headlen, passkey, &userid);
	platform = get_platform_from_passkey(passkey);		/* 转发的目标平台 */
	user_index = timeout_que_find_user(&penv->pserset->timeque, platform, msgcode, userid);

	if(user_index < 0) {
		/* FIXME:不再返回吗? */
		goto EXIT;
	}

	/* FIXME:先删除超时节点还是先进行返回处理? */
	timeout_que_del_user(&penv->pserset->timeque, user_index);

	r_memset(passkey, 0, 64);
	get_passkey_from_platform(puser->platform, passkey);
	modify_resp_xml_data_passkey(recv_buf + headlen, passkey);

	array.room_num = 1;
	array.roomid[0] = 0;

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, platform, array.roomid[index], userid);

		if(NULL == pforobj) {
			zlog_error2(DBGLOG, "---[common_resp_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error2(DBGLOG, "---[common_resp_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(MSGCODE_GET_AUDIO_VOLUME == msgcode) {
			ret = pforobj->pcmd.forward_process(NULL, pforobj, recv_buf, msgcode, ret_buf, ret_len);
		} else {
			ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode, ret_buf, ret_len);
		}

		if(ret < 0) {
			zlog_error2(DBGLOG, "---[common_resp_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	/* FIXME: 如果有多个转发对象的时候? */

EXIT:

	return return_code;
}

int32_t common_resolve_recv_buf_and_add_userid(control_env *pcon, int8_t *recv_buf,
        roomid_array *parray, int32_t user_index,
        const xmlChar *key, int32_t flag)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray || NULL == key) {
		zlog_error(DBGLOG, "---[common_resolvecommon_resolve_recv_buf_and_add_userid_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[common_resolve_recv_buf_and_add_userid] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);

	if(CONTROL_TRUE == flag) {
		pnode = get_children_node(pnode, key);
		proom_node = get_children_node(pnode, MSG_ROOMINFO_KEY);
	} else {
		proom_node = get_children_node(pnode, key);
	}

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[common_resolve_recv_buf_and_add_userid], roomid not found!\n");
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[common_resolve_recv_buf_and_add_userid], roomid error, roomid = %d\n", roomid);
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;

			if(CONTROL_TRUE == flag) {
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
			}
		}
	}



	/* 用户ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", user_index);
	package_add_xml_leaf(pxml->proot, MSG_USERID_KEY, (int8_t *)pbuf);

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	/* FIXME，确保recv_buf的长度要大于 CONTROL_DATA_LEN */
	r_memset(recv_buf, 0, CONTROL_DATA_LEN - headlen);

	if(xmllen > (CONTROL_DATA_LEN - headlen)) {
		xmllen = CONTROL_DATA_LEN - headlen;
	}

	r_memcpy(recv_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

/* 确保recv_buf的长度要大于 CONTROL_DATA_LEN */
int32_t add_userid_to_xml_data(int8_t *recv_buf, int32_t user_index)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	parse_xml_t *pxml = NULL;
	int32_t xmllen = 0;
	int8_t *pbuf = NULL;
	xmlChar *xml_buf = NULL;

	if(NULL == recv_buf) {
		zlog_error(DBGLOG, "---[add_userid_to_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[add_userid_to_xml_data] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	/* 用户ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", user_index);
	package_add_xml_leaf(pxml->proot, MSG_USERID_KEY, (int8_t *)pbuf);

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	/* FIXME，确保recv_buf的长度要大于 CONTROL_DATA_LEN */
	r_memset(recv_buf, 0, CONTROL_DATA_LEN - headlen);

	if(xmllen > (CONTROL_DATA_LEN - headlen)) {
		xmllen = CONTROL_DATA_LEN - headlen;
	}

	r_memcpy(recv_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	if(pbuf) {
		r_free(pbuf);
	}

	pbuf = NULL;

	return return_code;
}

static int32_t package_report_same_platform_xml_buf(con_user *pnew_user, int8_t *send_buf)
{
	int32_t return_code = STATUS_FAILED;

	int8_t		msgcode_buf[16] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;

	int32_t 	xmllen = 0;

	if(NULL == pnew_user || NULL == send_buf) {
		zlog_error(DBGLOG, "---[package_report_same_platform_xml_buf] failed, params error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	/* msgcode */
	r_memset(msgcode_buf, 0, 16);
	sprintf((char *)msgcode_buf, "%d", MSGCODE_REPORT_NEW_PLATFORM_LOGIN);
	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	/* passkey */
	package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, (const int8_t *)PLATFORM_RECSERVER);

	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	/* 新平台的IP */
	package_add_xml_leaf(body_node, MSG_NEW_PLATFORM_LOGIN_REPORT, (int8_t *)pnew_user->ipaddr);

	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	return return_code;
}

/* TODO: */
int32_t report_same_platform_login(con_user *old_puser, con_user *pnew_user)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int8_t *send_buf = NULL;

	if(NULL == old_puser || NULL == pnew_user) {
		zlog_error(DBGLOG, "---[report_same_platform_login] failed, params error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	send_buf = r_malloc(CONTROL_DATA_LEN);

	if(NULL == send_buf) {
		zlog_error(DBGLOG, "---[report_same_platform_login] failed, malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = package_report_same_platform_xml_buf(pnew_user, send_buf + headlen);

	if(STATUS_FAILED == return_code) {
		zlog_error(DBGLOG, "---[report_same_platform_login] failed, package_report_same_platform_xml_buf failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = send_user_xml_data(pnew_user, old_puser, send_buf, NULL, NULL);

EXIT:

	if(send_buf) {
		r_free(send_buf);
	}

	return return_code;
}

/********************************************************************************************/
/* MSGCODE_LOGIN : Login in 消息*/

int32_t login_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                      int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                      int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t userid = 0;
	time_t timep;

	if(NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[login_process] failed, params error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	time(&timep);
	zlog_debug2(OPELOG, "userid = %d\n", (int32_t)timep);
	userid = (int32_t)htonl(timep);
	r_memcpy(send_buf + headlen, &userid, sizeof(int32_t));

	return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

EXIT:

	return return_code;;
}

/********************************************************************************************/


/********************************************************************************************/
/* MSGCODE_CHECK_USER : 用户鉴权*/

int32_t checkuser_resolve_recv_buf(int8_t *recv_buf, con_user *puser, user_info *user)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr pbodynode = NULL;
	xmlNodePtr preqnode = NULL;

	if(NULL == recv_buf || NULL == user || NULL == puser) {
		zlog_error(DBGLOG, "---[checkuser_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error2(DBGLOG, "---[checkuser_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
	preqnode = get_children_node(pbodynode, MSG_AUTH_REQ_KEY);

	pnode = get_children_node(preqnode, MSG_USER_KEY);
	r_memset(user->username, 0, XML_USER_NAME_MAX_LENGTH);
	ret = get_current_node_value((char *)user->username, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error2(DBGLOG, "---[checkuser_resolve_recv_buf] failed, [get_current_node_value] error!\n");
	}

	pnode = get_children_node(preqnode, MSG_PASSWORD_KEY);
	r_memset(user->password, 0, XML_USER_PASSWORD_MAX_LENGTH);
	ret = get_current_node_value((char *)user->password, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error2(DBGLOG, "---[checkuser_resolve_recv_buf] failed, [get_current_node_value] error!\n");
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}

int32_t checkuser_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                          int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                          int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t check_flag  = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	user_info user;
	all_server_info *pserinfo = NULL;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[checkuser_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	return_code = checkuser_resolve_recv_buf(recv_buf + headlen, puser, &user);

	if(return_code != STATUS_SUCCESS) {
		zlog_error2(DBGLOG, "---[checkuser_process] failed, [checkuser_resolve_recv_buf] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(LiveNode == puser->platform) {
		return_code = STATUS_SUCCESS;
	} else if(IpadUser == puser->platform) {
		return_code = STATUS_FAILED;
		pserinfo = penv->pserset->pserinfo;
		pthread_mutex_lock(&pserinfo->info_m);

		/* 用户帐户密码鉴定，正确则返回1，错误则返回0, 且踢掉此用户 */
		//		if(r_strcmp((const int8_t *)pserinfo->Authentication.username, (const int8_t *)user.username) == 0){
		if(r_strcmp((const int8_t *)CONTROL_GUEST_USER, (const int8_t *)user.username) == 0) {
			if(r_strcmp((const int8_t *)pserinfo->Authentication.guest_passwd, (const int8_t *)user.password) == 0) {
				return_code = STATUS_SUCCESS;
			} else {
				return_code = STATUS_FAILED;
			}
		} else {
			return_code = STATUS_FAILED;
		}

		pthread_mutex_unlock(&pserinfo->info_m);
	} else {
		return_code = STATUS_FAILED;
		pserinfo = penv->pserset->pserinfo;
		pthread_mutex_lock(&pserinfo->info_m);

		/* 用户帐户密码鉴定，正确则返回1，错误则返回0,但不踢掉此用户 */
		if(r_strcmp((const int8_t *)CONTROL_ADMIN_USER, (const int8_t *)user.username) == 0) {
			if(r_strcmp((const int8_t *)pserinfo->Authentication.password, (const int8_t *)user.password) == 0) {
				return_code = STATUS_SUCCESS;
			}
		}

		if(r_strcmp((const int8_t *)CONTROL_GUEST_USER, (const int8_t *)user.username) == 0) {
			if(r_strcmp((const int8_t *)pserinfo->Authentication.guest_passwd, (const int8_t *)user.password) == 0) {
				return_code = STATUS_GUESET;
			}
		}

		pthread_mutex_unlock(&pserinfo->info_m);
	}

	if(STATUS_FAILED != return_code) {
		if(Director == puser->platform) {
			r_strcpy(puser->username, (const int8_t *)user.username);
		}
	}

	if(STATUS_SUCCESS == return_code) {
		if(MediaCenter == puser->platform) {
			http_set_post_url(puser, HTTP_SERVER_URL_PORT);
			set_user_ipaddr(penv, puser->ipaddr, puser->platform);
		}

		if(ManagePlatform == puser->platform || Director == puser->platform) {
			check_platform_unique_and_report(penv, puser, puser->platform);
			set_user_ipaddr(penv, puser->ipaddr, puser->platform);
		}
	}

	if(Director == puser->platform || STATUS_GUESET == return_code) {
		check_platform_unique_and_report(penv, puser, puser->platform);
	}

	check_flag = return_code;

	if(STATUS_SUCCESS == return_code) {
		package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);
	} else if(STATUS_FAILED == return_code) {
		package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_FAILED);
	} else {
		package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_GUEST);
	}

	return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

#if 0

	/*鉴权日志上报*/
	if(LiveNode != puser->platform) {
		Log_t log_info;
		r_bzero(&log_info, sizeof(Log_t));
		sprintf((char *)log_info.Type, "%d", LOG_TYPE_AUTH_REQ);
		sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
		warn_get_current_time_info(log_info.Time);

		zlog_debug(OPELOG, "---[checkuser_process] -------------puser->platform = [%d]-----------!\n", puser->platform);
		trans_platform_to_log_info_user(puser->platform, log_info.User);

		if(STATUS_SUCCESS == check_flag) {
			r_strcpy(log_info.Content, (int8_t *)MSG_LOG_SUCC_CONTENT);
		} else {
			r_strcpy(log_info.Content, (int8_t *)MSG_LOG_FAIL_CONTENT);
		}

		report_log_info_process(penv->pserset, &log_info);
	}

#endif


EXIT:

	return return_code;
}

/********************************************************************************************/


/********************************************************************************************/
/* MSGCODE_GET_SERVER_INFO : 获取服务器信息 */

int32_t package_server_info_xml_data(int8_t *send_buf, server_info *pinfo, int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t xmllen = 0;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr  pbodynode = NULL;
	xmlChar *xml_buf = NULL;

	int8_t *pbuf = NULL;
	struct in_addr addr;

	if(NULL == send_buf || NULL == pinfo) {
		zlog_error(DBGLOG, "---[package_server_info_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	package_resp_xml_data(send_buf, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[package_server_info_xml_data] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)send_buf)) {
		zlog_error(DBGLOG, "---[package_server_info_xml_data] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pnode = xmlNewNode(NULL, MSG_RECSERVER_INFO_KEY);
	xmlAddChild(pbodynode, pnode);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%s", pinfo->ServerType);
	package_add_xml_leaf(pnode, MSG_SERVER_TYPE_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%s", pinfo->ServerSeries);
	package_add_xml_leaf(pnode, MSG_SERVER_SERIES_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%s", pinfo->ServerVersion);
	package_add_xml_leaf(pnode, MSG_SERVER_VERSION_INFO_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", pinfo->MaxRoom);
	package_add_xml_leaf(pnode, MSG_MAXROOM_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", pinfo->MaxCodecNumInRoom);
	package_add_xml_leaf(pnode, MSG_MAXCODENUM_INROOM_KEY, pbuf);


	/* 局域网IP信息 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	memcpy(&addr, &pinfo->LanAddr, 4);
	strcpy((char *)pbuf, inet_ntoa(addr));
	package_add_xml_leaf(pnode, MSG_LANADDR_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	memcpy(&addr, &pinfo->LanGateWay, 4);
	strcpy((char *)pbuf, inet_ntoa(addr));
	package_add_xml_leaf(pnode, MSG_LANGATEWAY_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	memcpy(&addr, &pinfo->LanNetmask, 4);
	strcpy((char *)pbuf, inet_ntoa(addr));
	package_add_xml_leaf(pnode, MSG_LANNETMASK_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	JoinMacAddr(pbuf, pinfo->LanMac, 6);
	package_add_xml_leaf(pnode, MSG_LANMAC_KEY, pbuf);


	/* 广域网IP信息 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	memcpy(&addr, &pinfo->WanAddr, 4);
	strcpy((char *)pbuf, inet_ntoa(addr));
	package_add_xml_leaf(pnode, MSG_WANADDR_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	memcpy(&addr, &pinfo->WanGateWay, 4);
	strcpy((char *)pbuf, inet_ntoa(addr));
	package_add_xml_leaf(pnode, MSG_WANGATEWAY_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	memcpy(&addr, &pinfo->WanNetmask, 4);
	strcpy((char *)pbuf, inet_ntoa(addr));
	package_add_xml_leaf(pnode, MSG_WANNETMASK_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	JoinMacAddr(pbuf, pinfo->WanMac, 6);
	package_add_xml_leaf(pnode, MSG_WANMAC_KEY, pbuf);

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}


int32_t get_server_info_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	server_info *pinfo = NULL;
	server_info info;

	if(NULL == penv || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[get_server_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	pinfo = &penv->pserset->pserinfo->ServerInfo;
	pthread_mutex_lock(&penv->pserset->pserinfo->info_m);
	r_memcpy(&info, pinfo, sizeof(server_info));
	pthread_mutex_unlock(&penv->pserset->pserinfo->info_m);

	package_server_info_xml_data(send_buf + headlen, pinfo, msgcode);

	return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

EXIT:

	return return_code;
}

/********************************************************************************************/
static void picSyntResAndRateRange(xmlNodePtr father_node)
{
	int8_t value[64] = {0};
	r_memset(value, 0, 64);
	r_strcpy(value, PIC_SYNT_RESOLUTIONS);
	package_add_xml_leaf(father_node, MSG_ENC_RESOLUTIONS_KEY, value);

	r_memset(value, 0, 64);
	r_strcpy(value, PIC_SYNT_RATES);
	package_add_xml_leaf(father_node, MSG_ENC_BITRATES_KEY, value);
}

/********************************************************************************************/
/* MSGCODE_GET_ROOM_INFO : 获取会议室信息 */

int32_t package_room_info_xml_data(int8_t *send_buf, all_server_info *pinfo,
                                   int32_t msgcode, roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	parse_xml_t *pxml = NULL;
	xmlNodePtr	pbodynode = NULL;
	xmlNodePtr	proompnode = NULL;
	xmlNodePtr	paudiopnode = NULL;
	xmlNodePtr	psyntnode = NULL;
	xmlNodePtr	pvideopnode = NULL;
	xmlNodePtr	pqualitypnode = NULL;

	room_info	*proom = NULL;
	enc_info	*penc = NULL;

	int32_t xmllen = 0;
	xmlChar *xmlbuf = NULL;

	//	struct in_addr addr;

	int32_t index = 0;
	int32_t	i = 0;

	int8_t *pbuf = NULL;

	if(NULL == send_buf || NULL == pinfo || NULL == parray) {
		zlog_error(DBGLOG, "---[package_room_info_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	package_resp_xml_data(send_buf, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[package_room_info_xml_data] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)send_buf)) {
		zlog_error(DBGLOG, "---[package_room_info_xml_data] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);

	for(index = 0; index < parray->room_num; index++) {
		if((parray->roomid[index] > pinfo->ServerInfo.MaxRoom) || (parray->roomid[index] < 0)) {
			continue;
		}

		proom = &pinfo->RoomInfo[parray->roomid[index]];

		proompnode = xmlNewNode(NULL, MSG_ROOMINFO_KEY);
		xmlAddChild(pbodynode, proompnode);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->RoomID);
		package_add_xml_leaf(proompnode, MSG_ROOMID_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->ConnStatus);
		package_add_xml_leaf(proompnode, MSG_CONNECT_STATUS_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->RecordMaxTime);
		package_add_xml_leaf(proompnode, MSG_RECORD_MAXTIME_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->RecStatus);
		package_add_xml_leaf(proompnode, MSG_RECORD_STATUS_KEY, pbuf);

		package_add_xml_leaf(proompnode, MSG_RECNAME_KEY, (const int8_t *)proom->RecName);

		package_add_xml_leaf(proompnode, MSG_ROOMNAME_KEY, (const int8_t *)proom->RoomName);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->Mode);
		package_add_xml_leaf(proompnode, MSG_PICTURE_SYNT_MODEL_KEY, pbuf);
#if 0
		psyntnode = xmlNewNode(NULL, CONFIG_PARAMS_PICTURE_SYNT_INFO_KEY);
		xmlAddChild(proompnode, psyntnode);

		/* 源数量 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->PicSyntInfo.SourceNum);
		package_add_xml_leaf(psyntnode, MSG_PICTURE_SYNT_SOURCENUM_KEY, pbuf);

		/* 合成画面分辨率 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->PicSyntInfo.Resolution);
		package_add_xml_leaf(psyntnode, MSG_PICTURE_SYNT_RESOLUTION_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->PicSyntInfo.LayoutDef_2);
		package_add_xml_leaf(psyntnode, MSG_LAYOUT_DEF_2_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->PicSyntInfo.LayoutDef_3);
		package_add_xml_leaf(psyntnode, MSG_LAYOUT_DEF_3_KEY, pbuf);

		/* 合成画面布局 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%s", proom->PicSyntInfo.Model);
		package_add_xml_leaf(psyntnode, MSG_PICTURE_SYNT_MODEL_KEY, pbuf);
#endif

		paudiopnode = xmlNewNode(NULL, MSG_AUDIONFO_KEY);
		xmlAddChild(proompnode, paudiopnode);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->AudioInfo.InputMode);
		package_add_xml_leaf(paudiopnode, MSG_INPUTMODE_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->AudioInfo.SampleRate);
		package_add_xml_leaf(paudiopnode, MSG_SAMPLERATE_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->AudioInfo.Bitrate);
		package_add_xml_leaf(paudiopnode, MSG_BITRATE_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->AudioInfo.Lvolume);
		package_add_xml_leaf(paudiopnode, MSG_LVOLUME_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->AudioInfo.Rvolume);
		package_add_xml_leaf(paudiopnode, MSG_RVOLUME_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%s", proom->AudioInfo.InputIndex);
		package_add_xml_leaf(paudiopnode, MSG_INPUTINDEX_KEY, pbuf);

		for(i = 0; i < RECORD_ROOM_MAX_ENC_NUM; i++) {
			penc = &proom->EncInfo[i];

			if(penc->Status == 1 || 1)
				/* 返回编码器信息 */
			{
				//			zlog_debug(OPELOG, "penc->EncIP = %s\n", penc->EncIP);
				if(0 == r_strlen(penc->EncIP)) {
					continue;
				}

				pvideopnode = xmlNewNode(NULL, MSG_ENCINFO_KEY);
				xmlAddChild(proompnode, pvideopnode);

				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				sprintf((char *)pbuf, "%d", penc->ID);
				package_add_xml_leaf(pvideopnode, MSG_ENC_ID_KEY, pbuf);

				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
#if 0

				if(ENC_INFO_SPE_VALUE == penc->EncIP) {
					package_add_xml_leaf(pvideopnode, MSG_ENC_IP_KEY, XML_NULL_VALUE);
				} else {
					r_memcpy(&addr, &penc->EncIP, 4);
					strcpy((char *)pbuf, inet_ntoa(addr));
					package_add_xml_leaf(pvideopnode, MSG_ENC_IP_KEY, pbuf);
				}

#endif
				package_add_xml_leaf(pvideopnode, MSG_ENC_IP_KEY, penc->EncIP);

				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);

				if(proom->ConnStatus == 1) {
					sprintf((char *)pbuf, "%d", penc->Status);
				} else {
					sprintf((char *)pbuf, "%d", 0);
				}

				package_add_xml_leaf(pvideopnode, MSG_ENC_STATUS_KEY, pbuf);

				//				if(penc->HD_QuaInfo.enable == 1)
				/* 有高码流 */
				{
					pqualitypnode = xmlNewNode(NULL, MSG_ENC_QUALITYINFO_KEY);
					xmlAddChild(pvideopnode, pqualitypnode);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->HD_QuaInfo.RateType);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_RATETYPE_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->HD_QuaInfo.EncBitrate);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_BITRATE_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->HD_QuaInfo.EncWidth);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_WIDTH_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->HD_QuaInfo.EncHeight);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_HEIGHT_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->HD_QuaInfo.EncFrameRate);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_FRAMERATE_KEY, pbuf);
					if(1 == penc->ID) {
						picSyntResAndRateRange(pqualitypnode);
					}
				}

				//				if(penc->SD_QuaInfo.enable == 1)
				/* 有高码流 */
				if(5 != penc->ID) { /* JPEG只有一路 */
					pqualitypnode = xmlNewNode(NULL, MSG_ENC_QUALITYINFO_KEY);
					xmlAddChild(pvideopnode, pqualitypnode);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->SD_QuaInfo.RateType);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_RATETYPE_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->SD_QuaInfo.EncBitrate);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_BITRATE_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->SD_QuaInfo.EncWidth);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_WIDTH_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->SD_QuaInfo.EncHeight);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_HEIGHT_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->SD_QuaInfo.EncFrameRate);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_FRAMERATE_KEY, pbuf);
				}
			}
		}
	}

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xmlbuf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xmlbuf, xmllen);
	xmlFree(xmlbuf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}

int32_t package_room_info_xml_data2(int8_t *send_buf, all_server_info *pinfo,
                                    int32_t msgcode, int32_t roomid)
{
	int32_t return_code = STATUS_FAILED;
	parse_xml_t *pxml = NULL;
	xmlNodePtr	pbodynode = NULL;
	xmlNodePtr	proompnode = NULL;
	xmlNodePtr	psyntnode = NULL;
	xmlNodePtr	paudiopnode = NULL;
	xmlNodePtr	pvideopnode = NULL;
	xmlNodePtr	pqualitypnode = NULL;

	room_info	*proom = NULL;
	enc_info	*penc = NULL;

	int32_t xmllen = 0;
	xmlChar *xmlbuf = NULL;

	//	struct in_addr addr;

	int32_t i = 0;

	int8_t *pbuf = NULL;

	if(NULL == send_buf || NULL == pinfo) {
		zlog_error(DBGLOG, "---[package_room_info_xml_data2] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	package_resp_xml_data(send_buf, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[package_room_info_xml_data2] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)send_buf)) {
		zlog_error(DBGLOG, "---[package_room_info_xml_data2] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);

	if(roomid >= 0 && roomid < pinfo->ServerInfo.MaxRoom) {
		proom = &pinfo->RoomInfo[roomid];

		proompnode = xmlNewNode(NULL, MSG_ROOMINFO_KEY);
		xmlAddChild(pbodynode, proompnode);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->RoomID);
		package_add_xml_leaf(proompnode, MSG_ROOMID_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->ConnStatus);
		package_add_xml_leaf(proompnode, MSG_CONNECT_STATUS_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->RecordMaxTime);
		package_add_xml_leaf(proompnode, MSG_RECORD_MAXTIME_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->RecStatus);
		package_add_xml_leaf(proompnode, MSG_RECORD_STATUS_KEY, pbuf);

		package_add_xml_leaf(proompnode, MSG_RECNAME_KEY, (const int8_t *)proom->RecName);

		psyntnode = xmlNewNode(NULL, CONFIG_PARAMS_PICTURE_SYNT_INFO_KEY);
		xmlAddChild(proompnode, psyntnode);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->Mode);
		package_add_xml_leaf(proompnode, MSG_PICTURE_SYNT_MODEL_KEY, pbuf);
#if 0
		/* 源数量 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->PicSyntInfo.SourceNum);
		package_add_xml_leaf(psyntnode, MSG_PICTURE_SYNT_SOURCENUM_KEY, pbuf);

		/* 合成画面分辨率 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->PicSyntInfo.Resolution);
		package_add_xml_leaf(psyntnode, MSG_PICTURE_SYNT_RESOLUTION_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->PicSyntInfo.LayoutDef_2);
		package_add_xml_leaf(psyntnode, MSG_LAYOUT_DEF_2_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->PicSyntInfo.LayoutDef_3);
		package_add_xml_leaf(psyntnode, MSG_LAYOUT_DEF_3_KEY, pbuf);

		/* 合成画面布局 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%s", proom->PicSyntInfo.Model);
		package_add_xml_leaf(psyntnode, MSG_PICTURE_SYNT_MODEL_KEY, pbuf);
#endif

		paudiopnode = xmlNewNode(NULL, MSG_AUDIONFO_KEY);
		xmlAddChild(proompnode, paudiopnode);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->AudioInfo.InputMode);
		package_add_xml_leaf(paudiopnode, MSG_INPUTMODE_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->AudioInfo.SampleRate);
		package_add_xml_leaf(paudiopnode, MSG_SAMPLERATE_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->AudioInfo.Bitrate);
		package_add_xml_leaf(paudiopnode, MSG_BITRATE_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->AudioInfo.Lvolume);
		package_add_xml_leaf(paudiopnode, MSG_LVOLUME_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", proom->AudioInfo.Rvolume);
		package_add_xml_leaf(paudiopnode, MSG_RVOLUME_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%s", proom->AudioInfo.InputIndex);
		package_add_xml_leaf(paudiopnode, MSG_INPUTINDEX_KEY, pbuf);

		for(i = 0; i < RECORD_ROOM_MAX_ENC_NUM; i++) {
			penc = &proom->EncInfo[i];

			if(penc->Status == 1 || 1)
				/* 返回编码器信息 */
			{
				pvideopnode = xmlNewNode(NULL, MSG_ENCINFO_KEY);
				xmlAddChild(proompnode, pvideopnode);

				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				sprintf((char *)pbuf, "%d", penc->ID);
				package_add_xml_leaf(pvideopnode, MSG_ENC_ID_KEY, pbuf);

				//			zlog_debug(OPELOG, "roomid = %d\n", roomid);
				//			zlog_debug(OPELOG, "penc->EncIP = %s\n", penc->EncIP);
#if 0

				if(ENC_INFO_SPE_VALUE == penc->EncIP) {
					package_add_xml_leaf(pvideopnode, MSG_ENC_IP_KEY, XML_NULL_VALUE);
				} else {
					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					r_memcpy(&addr, &penc->EncIP, 4);
					strcpy((char *)pbuf, inet_ntoa(addr));
					package_add_xml_leaf(pvideopnode, MSG_ENC_IP_KEY, pbuf);
				}

				zlog_debug(OPELOG, "EncIP = %s\n\n", pbuf);
#endif
				package_add_xml_leaf(pvideopnode, MSG_ENC_IP_KEY, penc->EncIP);

				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				sprintf((char *)pbuf, "%d", penc->Status);
				package_add_xml_leaf(pvideopnode, MSG_ENC_STATUS_KEY, pbuf);

				if(penc->HD_QuaInfo.enable == 1)
					/* 有高码流 */
				{
					pqualitypnode = xmlNewNode(NULL, MSG_ENC_QUALITYINFO_KEY);
					xmlAddChild(pvideopnode, pqualitypnode);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->HD_QuaInfo.RateType);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_RATETYPE_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->HD_QuaInfo.EncBitrate);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_BITRATE_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->HD_QuaInfo.EncWidth);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_WIDTH_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->HD_QuaInfo.EncHeight);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_HEIGHT_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->HD_QuaInfo.EncFrameRate);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_FRAMERATE_KEY, pbuf);
				}

				if(penc->SD_QuaInfo.enable == 1)
					/* 有高码流 */
				{
					pqualitypnode = xmlNewNode(NULL, MSG_ENC_QUALITYINFO_KEY);
					xmlAddChild(pvideopnode, pqualitypnode);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->SD_QuaInfo.RateType);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_RATETYPE_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->SD_QuaInfo.EncBitrate);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_BITRATE_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->SD_QuaInfo.EncWidth);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_WIDTH_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->SD_QuaInfo.EncHeight);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_HEIGHT_KEY, pbuf);

					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					sprintf((char *)pbuf, "%d", penc->SD_QuaInfo.EncFrameRate);
					package_add_xml_leaf(pqualitypnode, MSG_ENC_FRAMERATE_KEY, pbuf);
				}
			}
		}
	}

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xmlbuf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xmlbuf, xmllen);
	xmlFree(xmlbuf);

	//	zlog_debug(OPELOG, "\n\n\n\n\n\n\n");
	//	zlog_debug(OPELOG, "%s\n", send_buf);
	//	zlog_debug(OPELOG, "\n\n\n\n\n\n\n");

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}



int32_t get_room_info_resolve_recv_buf_room_id(control_env *pcon, int8_t *recv_buf,
        roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf_room_id] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;
	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf_room_id] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	proom_node = get_children_node(pnode, MSG_ROOMINFO_REQ_KEY);
	pnode = get_children_node(proom_node, MSG_ROOMID_KEY);	/* 适用多个roomid情况 */
	count = get_current_samename_node_nums(pnode);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			//			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf_room_id], roomid not found!\n");
				pnode = find_next_node(pnode, MSG_ROOMID_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[get_room_info_resolve_recv_buf_room_id], roomid error, roomid = %d\n", roomid);
				proom_node = find_next_node(pnode, MSG_ROOMID_KEY);
				continue;
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
			pnode = find_next_node(pnode, MSG_ROOMID_KEY);
		}
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}




int32_t get_room_info_resolve_recv_buf(control_env *pcon, con_user *puser,
                                       int8_t *recv_buf)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t enc_count = 0;
	int32_t room_count = 0;
	int32_t quality_count = 0;
	int32_t roomid = 0;
	int32_t ratetype = 0;
	int32_t id = 0;
	int32_t id_index = 0;
	int32_t index = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	xmlNodePtr paudio_node = NULL;
	xmlNodePtr psynt_node = NULL;
	xmlNodePtr pvideo_node = NULL;
	xmlNodePtr pquality_node = NULL;

	all_server_info *pser = NULL;
	room_info *proom = NULL;
	room_info *proomt = NULL;
	quality_info *pquality = NULL;

	roomid_array array;	/* 作特殊使用 */

	//	struct in_addr addr;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == puser) {
		zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	proomt = (room_info *)r_malloc(sizeof(room_info));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	//	xmlSaveFormatFile("./test.xml", pxml->pdoc, 1);

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);

	/* 编码器信息节点 */
	proom_node = get_children_node(pnode, MSG_ROOMINFO_KEY);
	room_count = get_current_samename_node_nums(proom_node);

	if(room_count > pser->ServerInfo.MaxRoom) {
		room_count = pser->ServerInfo.MaxRoom;
	}

	if(room_count < 0) {
		room_count = 0;
	}

	//	zlog_debug2(OPELOG, "room_count = %d\n", room_count);

	while(proom_node && room_count--) {
		/* 会议室ID */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], roomid not found!\n");
			proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
			continue;
		}

		roomid = atoi((const char *)pbuf);
		//		zlog_debug2(OPELOG, "RoomID = %d\n", roomid);

		r_memset(&array, 0, sizeof(roomid_array));
		array.room_num = RECORD_ROOM_MAX_ENC_NUM;

		if(roomid == pcon->index && roomid >= 0 && roomid < pser->ServerInfo.MaxRoom) {
			proom = &pser->RoomInfo[roomid];
			r_memcpy(proomt, proom, sizeof(room_info));
			proomt->RoomID = roomid;

			/* 连接状态 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(proom_node, MSG_CONNECT_STATUS_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "ConnStatus = %s\n", pbuf);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], roomid not found!\n");
			} else {
				proomt->ConnStatus = atoi((const char *)pbuf);
			}

			/* 录制最大时长 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(proom_node, MSG_RECORD_MAXTIME_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "RecordMaxTime = %s\n", pbuf);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_RECORD_MAXTIME_KEY);
			} else {
				proomt->RecordMaxTime = atoi((const char *)pbuf);
			}

			/* 录制状态 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(proom_node, MSG_RECORD_STATUS_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "RecStatus = %s\n", pbuf);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_RECORD_STATUS_KEY);
			} else {
				proomt->RecStatus = atoi((const char *)pbuf);
			}

			/* 录制文件名 */
			r_memset(proomt->RecName, 0, RECORD_FILE_MAX_FILENAME);
			pnode = get_children_node(proom_node, MSG_RECNAME_KEY);
			ret = get_current_node_value((char *)proomt->RecName, RECORD_FILE_MAX_FILENAME, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "RecName = %s\n", proomt->RecName);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_RECNAME_KEY);
			}

			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(proom_node, MSG_PICTURE_SYNT_MODEL_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_PICTURE_SYNT_MODEL_KEY);
			} else {
				proomt->Mode = atoi((const char *)pbuf);
			}

#if 0
			/* 合成画面信息结点 */
			psynt_node = get_children_node(proom_node, CONFIG_PARAMS_PICTURE_SYNT_INFO_KEY);

			/* Model */
			r_memset(proomt->PicSyntInfo.Model, 0, PICTURE_VALUE_LENGTH);
			pnode = get_children_node(psynt_node, MSG_PICTURE_SYNT_MODEL_KEY);
			ret = get_current_node_value((char *)proomt->PicSyntInfo.Model, PICTURE_VALUE_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_PICTURE_SYNT_MODEL_KEY);
			}

			/* Resolution */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(psynt_node, MSG_PICTURE_SYNT_RESOLUTION_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_PICTURE_SYNT_RESOLUTION_KEY);
			} else {
				proomt->PicSyntInfo.Resolution = atoi((const char *)pbuf);
			}

			/* SourceNum */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(psynt_node, MSG_PICTURE_SYNT_SOURCENUM_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_PICTURE_SYNT_SOURCENUM_KEY);
			} else {
				proomt->PicSyntInfo.SourceNum = atoi((const char *)pbuf);
			}

			/* LayoutDef_2 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(psynt_node, MSG_LAYOUT_DEF_2_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_LAYOUT_DEF_2_KEY);
			} else {
				proomt->PicSyntInfo.LayoutDef_2 = atoi((const char *)pbuf);
			}

			/* LayoutDef_3 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(psynt_node, MSG_LAYOUT_DEF_3_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_LAYOUT_DEF_3_KEY);
			} else {
				proomt->PicSyntInfo.LayoutDef_3 = atoi((const char *)pbuf);
			}
#endif
			/* 音频信息节点 */
			paudio_node = get_children_node(proom_node, MSG_AUDIONFO_KEY);

			/* 输入模式 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(paudio_node, MSG_INPUTMODE_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "InputMode = %s\n", pbuf);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_INPUTMODE_KEY);
			} else {
				proomt->AudioInfo.InputMode = atoi((const char *)pbuf);
			}

			/* 采样率 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(paudio_node, MSG_SAMPLERATE_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "SampleRate = %s\n", pbuf);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_SAMPLERATE_KEY);
			} else {
				proomt->AudioInfo.SampleRate = atoi((const char *)pbuf);
			}

			/* 比特率 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(paudio_node, MSG_BITRATE_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "Bitrate = %s\n", pbuf);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_BITRATE_KEY);
			} else {
				proomt->AudioInfo.Bitrate = atoi((const char *)pbuf);
			}

			/* 左声道音量 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(paudio_node, MSG_LVOLUME_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "Lvolume = %s\n", pbuf);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_LVOLUME_KEY);
			} else {
				proomt->AudioInfo.Lvolume = atoi((const char *)pbuf);
			}

			/* 右声道音量 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(paudio_node, MSG_RVOLUME_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "Rvolume = %s\n", pbuf);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_RVOLUME_KEY);
			} else {
				proomt->AudioInfo.Rvolume = atoi((const char *)pbuf);
			}

			/* InputIndex */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(paudio_node, MSG_INPUTINDEX_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "Rvolume = %s\n", pbuf);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_INPUTINDEX_KEY);
			} else {
				r_memcpy(proomt->AudioInfo.InputIndex, pbuf, 4);
			}


			/* 编码器信息节点 */
			pvideo_node = get_children_node(proom_node, MSG_ENCINFO_KEY);
			enc_count = get_current_samename_node_nums(pvideo_node);

			if(enc_count > RECORD_ROOM_MAX_ENC_NUM) {
				enc_count = RECORD_ROOM_MAX_ENC_NUM;
			}

			if(enc_count < 0) {
				enc_count = 0;
			}

			//			zlog_debug2(OPELOG, "enc_count = %d\n", enc_count);

			while(pvideo_node && enc_count--) {

				/* 编码器 ID */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pvideo_node, MSG_ENC_ID_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				if(ret < 0) {
					zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_ENC_ID_KEY);
					pvideo_node = find_next_node(pvideo_node, MSG_ENCINFO_KEY);
					continue;
				} else {
					id = atoi((const char *)pbuf);
				}

				if(id < 1 || id > RECORD_ROOM_MAX_ENC_NUM) {
					pvideo_node = find_next_node(pvideo_node, MSG_ENCINFO_KEY);
					continue;
				}

				id_index = id - 1;

				/* 注意索引值 */
				proomt->EncInfo[id_index].ID = id;

				array.data[id_index] = 1;
#if 0
				/* 编码器 IP */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pvideo_node, MSG_ENC_IP_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);
				zlog_debug2(OPELOG, "EncIP = %s, ret = %d\n", pbuf, ret);

				if(ret < 0) {
					zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_ENC_IP_KEY);
					proomt->EncInfo[id_index].EncIP = ENC_INFO_SPE_VALUE;
				} else {
					inet_aton((const char *)pbuf, &addr);
					r_memcpy(&proomt->EncInfo[id_index].EncIP, &addr, 4);

					zlog_debug2(OPELOG, "proomt->EncInfo[id_index].EncIP = %s\n", inet_ntoa(addr));
				}

#endif

				/* 编码器 IP */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pvideo_node, MSG_ENC_IP_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				//				zlog_debug2(OPELOG, "EncIP = %s, ret = %d\n", pbuf, ret);
				if(ret < 0) {
					zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_ENC_IP_KEY);
					r_memset(proomt->EncInfo[id_index].EncIP, 0, IPINFO_MAX_LENGTH);
					//				pvideo_node = find_next_node(pvideo_node, MSG_ENCINFO_KEY);
					//				continue;
				} else {
					r_strncpy(proomt->EncInfo[id_index].EncIP, pbuf, IPINFO_MAX_LENGTH);
				}

				/* 编码器 状态 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pvideo_node, MSG_ENC_STATUS_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				//				zlog_debug2(OPELOG, "Status = %s\n", pbuf);
				if(ret < 0) {
					zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_ENC_STATUS_KEY);
				} else {
					proomt->EncInfo[id_index].Status = atoi((const char *)pbuf);
				}

				/* 高低码流信息节点 */
				pquality_node = get_children_node(pvideo_node, MSG_ENC_QUALITYINFO_KEY);
				quality_count = get_current_samename_node_nums(pquality_node);

				if(quality_count > RECORD_ROOM_ENC_MAX_QUALITY_NUM) {
					quality_count = RECORD_ROOM_ENC_MAX_QUALITY_NUM;
				}

				if(quality_count < 0) {
					quality_count = 0;
				}

				//				zlog_debug2(OPELOG, "quality_count = %d\n", quality_count);

				proomt->EncInfo[id_index].HD_QuaInfo.enable = 0;
				proomt->EncInfo[id_index].SD_QuaInfo.enable = 0;

				while(pquality_node && quality_count--) {
					/* 高低码流类型 */
					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					pnode = get_children_node(pquality_node, MSG_ENC_RATETYPE_KEY);
					ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

					if(ret < 0) {
						zlog_error(OPELOG, "---[get_room_info_resolve_recv_buf], %s not found!\n",
						           MSG_ENC_RATETYPE_KEY);
						pquality_node = find_next_node(pquality_node, MSG_ENC_QUALITYINFO_KEY);
						continue;
					} else {
						ratetype = atoi((const char *)pbuf);

						if(ratetype == 0) {/* 高码流 */
							pquality = &proomt->EncInfo[id_index].HD_QuaInfo;
						} else if(ratetype == 1) {/* 低码流 */
							pquality = &proomt->EncInfo[id_index].SD_QuaInfo;
						}

						pquality->enable = 1;
						pquality->RateType = ratetype;
					}

					//					zlog_debug2(OPELOG, "RateType = %d\n", ratetype);

					/* 码流 */
					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					pnode = get_children_node(pquality_node, MSG_ENC_BITRATE_KEY);
					ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

					//					zlog_debug2(OPELOG, "EncBitrate = %s\n", pbuf);
					if(ret < 0) {
						zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n",
						           MSG_ENC_BITRATE_KEY);
					} else {
						pquality->EncBitrate = atoi((const char *)pbuf);
					}

					/* 视频宽 */
					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					pnode = get_children_node(pquality_node, MSG_ENC_WIDTH_KEY);
					ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

					//					zlog_debug2(OPELOG, "EncWidth = %s\n", pbuf);
					if(ret < 0) {
						zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n",
						           MSG_ENC_WIDTH_KEY);
					} else {
						pquality->EncWidth = atoi((const char *)pbuf);
					}

					/* 视频高 */
					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					pnode = get_children_node(pquality_node, MSG_ENC_HEIGHT_KEY);
					ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

					//					zlog_debug2(OPELOG, "EncHeight = %s\n", pbuf);
					if(ret < 0) {
						zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n",
						           MSG_ENC_HEIGHT_KEY);
					} else {
						pquality->EncHeight = atoi((const char *)pbuf);
					}

					/* 视频帧率 */
					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					pnode = get_children_node(pquality_node, MSG_ENC_FRAMERATE_KEY);
					ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

					//					zlog_debug2(OPELOG, "EncFrameRate = %s\n", pbuf);
					if(ret < 0) {
						zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n",
						           MSG_ENC_FRAMERATE_KEY);
					} else {
						pquality->EncFrameRate = atoi((const char *)pbuf);
					}

					pquality_node = find_next_node(pquality_node, MSG_ENC_QUALITYINFO_KEY);
				}

				pvideo_node = find_next_node(pvideo_node, MSG_ENCINFO_KEY);
			}

			/* 将未出现过的编码器IP信息清除掉 */
			for(index = 0; index < array.room_num; index++) {
				if(array.data[index] == CONTROL_FALSE) {
					//		proomt->EncInfo[index].EncIP = ENC_INFO_SPE_VALUE;
					r_memset(proomt->EncInfo[index].EncIP, 0, IPINFO_MAX_LENGTH);
				}
			}

			/* 更新本地列表 */
			proom = &pser->RoomInfo[roomid];
			zlog_debug2(OPELOG, "save room %d info!!\n", roomid);
			pthread_mutex_lock(&pser->info_m);
			modify_room_info_only((const int8_t *)CONFIG_TABLE_FILE, proom, proomt);
			r_memcpy(proom, proomt, sizeof(room_info));
			pthread_mutex_unlock(&pser->info_m);
		}

		proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
	}

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(proomt) {
		r_free(proomt);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t get_room_info_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                              int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                              int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	all_server_info *pinfo = NULL;
	all_server_info *psinfo = NULL;
	roomid_array array;

	if(NULL == penv || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[get_room_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_memset(send_buf + headlen, 0, CONTROL_DATA_LEN - headlen);

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	pinfo = (all_server_info *)r_malloc(sizeof(all_server_info));

	if(NULL == pinfo) {
		zlog_error2(DBGLOG, "---[get_room_info_process] failed, malloc error\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		/* 发送本地列表 */
		psinfo = penv->pserset->pserinfo;
		pthread_mutex_lock(&psinfo->info_m);
		r_memcpy(pinfo, psinfo, sizeof(all_server_info));
		pthread_mutex_unlock(&psinfo->info_m);

		if(RecServer == puser->platform) {
			array.room_num = 1;
			array.roomid[0] = puser->port - CONTROL_ROOM_SERVER_PORT_START;

			if((array.roomid[0] < 0) || (array.roomid[0] > pinfo->ServerInfo.MaxRoom)) {
				goto EXIT;
			}

			/* 发送给会议室 */
			package_room_info_xml_data2(send_buf + headlen, pinfo, msgcode, array.roomid[0]);
		} else {
			get_room_info_resolve_recv_buf_room_id(penv, recv_buf + headlen, &array);
			package_room_info_xml_data(send_buf + headlen, pinfo, msgcode, &array);
		}

		if(puser->platform == ManagePlatform) {
			return_code = send_user_xml_data(NULL, puser, send_buf, ret_buf, ret_len);
		} else {
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		/* 更新本地列表 */
		return_code = get_room_info_resolve_recv_buf(penv, puser, recv_buf + headlen);
	}

EXIT:

	if(pinfo) {
		r_free(pinfo);
	}

	return return_code;
}


/********************************************************************************************/


/********************************************************************************************/
/* MSGCODE_SET_ROOM_INFO : 设置会议室信息 */

int32_t set_room_info_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
                                       roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pnode = get_children_node(pnode, MSG_SET_ROOMINFO_REQ_KEY);
	proom_node = get_children_node(pnode, MSG_ROOMINFO_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf], roomid not found!\n");
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[set_room_info_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				continue;
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
		}
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t set_room_info_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                  int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[set_room_info_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_SET_ROOMINFO_REQ_KEY, CONTROL_TRUE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);
	timeout_add_con_user_addr(&penv->pserset->timeque, puser->platform, puser->index, puser->ipaddr);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[set_room_info_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[set_room_info_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 5);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

			/*设置教室信息日志上报*/
			//			report_set_room_info_log(penv, puser, send_buf, puser->platform);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

		/*设置教室信息日志上报*/
		//		report_set_room_info_log(penv, puser, send_buf, puser->platform);
	}

EXIT:

	return return_code;
}

int32_t set_room_info_resolve_recv_buf_and_save(control_env *pcon, con_user *puser,
        int8_t *recv_buf)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t enc_count = 0;
	int32_t quality_count = 0;
	int32_t roomid = 0;
	int32_t ratetype = 0;
	int32_t id = 0;
	int32_t id_index = 0;
	int32_t index = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	xmlNodePtr paudio_node = NULL;
	xmlNodePtr psynt_node = NULL;
	xmlNodePtr pvideo_node = NULL;
	xmlNodePtr pquality_node = NULL;

	all_server_info *pser = NULL;
	room_info *proom = NULL;
	room_info *proomt = NULL;
	quality_info *pquality = NULL;

	roomid_array array;	/* 作特殊使用 */

	//	struct in_addr addr;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == puser) {
		zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	proomt = (room_info *)r_malloc(sizeof(room_info));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pnode = get_children_node(pnode, MSG_SET_ROOMINFO_REQ_KEY);

	/* 编码器信息节点 */
	proom_node = get_children_node(pnode, MSG_ROOMINFO_KEY);

	/* 会议室ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], roomid not found!\n");
		proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	roomid = atoi((const char *)pbuf);

	//	zlog_debug2(OPELOG, "roomid = %d\n", roomid);

	r_memset(&array, 0, sizeof(roomid_array));
	array.room_num = RECORD_ROOM_MAX_ENC_NUM;

	if(roomid == pcon->index && roomid >= 0 && roomid < pser->ServerInfo.MaxRoom) {
		proom = &pser->RoomInfo[roomid];
		r_memcpy(proomt, proom, sizeof(room_info));
		proomt->RoomID = roomid;

		/* 连接状态 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(proom_node, MSG_CONNECT_STATUS_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		//		zlog_debug2(OPELOG, "ConnStatus = %s\n", pbuf);
		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], roomid not found!\n");
		} else {
			proomt->ConnStatus = atoi((const char *)pbuf);
		}

		/* 录制最大时长 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(proom_node, MSG_RECORD_MAXTIME_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		//		zlog_debug2(OPELOG, "RecordMaxTime = %s\n", pbuf);
		if(ret < 0) {
			zlog_error(DBGLOG, "---[get_room_info_resolve_recv_buf], %s not found!\n", MSG_RECORD_MAXTIME_KEY);
		} else {
			proomt->RecordMaxTime = atoi((const char *)pbuf);
		}

		/* 录制状态 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(proom_node, MSG_RECORD_STATUS_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		//		zlog_debug2(OPELOG, "RecStatus = %s\n", pbuf);
		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_RECORD_STATUS_KEY);
		} else {
			proomt->RecStatus = atoi((const char *)pbuf);
		}

		/* 录制文件名 */
		r_memset(proomt->RecName, 0, RECORD_FILE_MAX_FILENAME);
		pnode = get_children_node(proom_node, MSG_RECNAME_KEY);
		ret = get_current_node_value((char *)proomt->RecName, RECORD_FILE_MAX_FILENAME, pxml->pdoc, pnode);

		//		zlog_debug2(OPELOG, "RecName = %s\n", proomt->RecName);
		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_RECNAME_KEY);
		}

		/*合成模式*/
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(proom_node, MSG_PICTURE_SYNT_MODEL_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);
		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_PICTURE_SYNT_MODEL_KEY);
		} else {
			proomt->Mode = atoi((const char *)pbuf);
		}

#if 0
		psynt_node = get_children_node(proom_node, CONFIG_PARAMS_PICTURE_SYNT_INFO_KEY);

		pnode = get_children_node(psynt_node, MSG_PICTURE_SYNT_SOURCENUM_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_PICTURE_SYNT_SOURCENUM_KEY);
		} else {
			proomt->PicSyntInfo.SourceNum = atoi((const char *)pbuf);
		}

		pnode = get_children_node(psynt_node, MSG_PICTURE_SYNT_RESOLUTION_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_PICTURE_SYNT_RESOLUTION_KEY);
		} else {
			proomt->PicSyntInfo.Resolution = atoi((const char *)pbuf);
		}

		pnode = get_children_node(psynt_node, MSG_LAYOUT_DEF_2_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_LAYOUT_DEF_2_KEY);
		} else {
			proomt->PicSyntInfo.LayoutDef_2 = atoi((const char *)pbuf);
		}

		pnode = get_children_node(psynt_node, MSG_LAYOUT_DEF_3_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_LAYOUT_DEF_3_KEY);
		} else {
			proomt->PicSyntInfo.LayoutDef_3 = atoi((const char *)pbuf);
		}

		pnode = get_children_node(psynt_node, MSG_LAYOUT_DEF_3_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_LAYOUT_DEF_3_KEY);
		} else {
			proomt->PicSyntInfo.LayoutDef_3 = atoi((const char *)pbuf);
		}

		r_memset(proomt->PicSyntInfo.Model, 0, PICTURE_VALUE_LENGTH);
		pnode = get_children_node(proom_node, MSG_PICTURE_SYNT_MODEL_KEY);
		ret = get_current_node_value((char *)proomt->PicSyntInfo.Model, PICTURE_VALUE_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_PICTURE_SYNT_MODEL_KEY);
		}
#endif
		/* 音频信息节点 */
		paudio_node = get_children_node(proom_node, MSG_AUDIONFO_KEY);

		/* 输入模式 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(paudio_node, MSG_INPUTMODE_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		//		zlog_debug2(OPELOG, "InputMode = %s\n", pbuf);
		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_INPUTMODE_KEY);
		} else {
			proomt->AudioInfo.InputMode = atoi((const char *)pbuf);
		}

		/* 采样率 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(paudio_node, MSG_SAMPLERATE_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		//		zlog_debug2(OPELOG, "SampleRate = %s\n", pbuf);
		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_SAMPLERATE_KEY);
		} else {
			proomt->AudioInfo.SampleRate = atoi((const char *)pbuf);
		}

		/* 比特率 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(paudio_node, MSG_BITRATE_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		//		zlog_debug2(OPELOG, "Bitrate = %s\n", pbuf);
		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_BITRATE_KEY);
		} else {
			proomt->AudioInfo.Bitrate = atoi((const char *)pbuf);
		}

		/* 左声道音量 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(paudio_node, MSG_LVOLUME_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		//		zlog_debug2(OPELOG, "Lvolume = %s\n", pbuf);
		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_LVOLUME_KEY);
		} else {
			proomt->AudioInfo.Lvolume = atoi((const char *)pbuf);
		}

		/* 右声道音量 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(paudio_node, MSG_RVOLUME_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		//		zlog_debug2(OPELOG, "Rvolume = %s\n", pbuf);
		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_RVOLUME_KEY);
		} else {
			proomt->AudioInfo.Rvolume = atoi((const char *)pbuf);
		}

		/* 编码器信息节点 */
		pvideo_node = get_children_node(proom_node, MSG_ENCINFO_KEY);
		enc_count = get_current_samename_node_nums(pvideo_node);

		if(enc_count > RECORD_ROOM_MAX_ENC_NUM) {
			enc_count = RECORD_ROOM_MAX_ENC_NUM;
		}

		if(enc_count < 0) {
			enc_count = 0;
		}

		//		zlog_debug2(OPELOG, "enc_count = %d\n", enc_count);

		while(pvideo_node && enc_count--) {

			/* 编码器 ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(pvideo_node, MSG_ENC_ID_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_ENC_ID_KEY);
				pvideo_node = find_next_node(pvideo_node, MSG_ENCINFO_KEY);
				continue;
			} else {
				id = atoi((const char *)pbuf);
			}

			if(id < 1 || id > RECORD_ROOM_MAX_ENC_NUM) {
				pvideo_node = find_next_node(pvideo_node, MSG_ENCINFO_KEY);
				continue;
			}

			id_index = id - 1;

			/* 注意索引值 */
			proomt->EncInfo[id_index].ID = id;

			array.data[id_index] = 1;
#if 0
			/* 编码器 IP */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(pvideo_node, MSG_ENC_IP_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);
			zlog_debug2(OPELOG, "EncIP = %s\n", pbuf);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_ENC_IP_KEY);
				proomt->EncInfo[id_index].EncIP = ENC_INFO_SPE_VALUE;
			} else {
				inet_aton((const char *)pbuf, &addr);
				r_memcpy(&proomt->EncInfo[id_index].EncIP, &addr, 4);

				zlog_debug2(OPELOG, "proomt->EncInfo[id_index].EncIP = %s\n", inet_ntoa(addr));
			}

#endif
#if 0
			/* 编码器 IP */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(pvideo_node, MSG_ENC_IP_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);
			zlog_debug2(OPELOG, "EncIP = %s\n", pbuf);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_ENC_IP_KEY);
				r_memset(proomt->EncInfo[id_index].EncIP, 0, IPINFO_MAX_LENGTH);
			} else {
				r_strncpy(proomt->EncInfo[id_index].EncIP, pbuf, IPINFO_MAX_LENGTH);
			}

#endif
			/* 编码器 状态 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(pvideo_node, MSG_ENC_STATUS_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			//			zlog_debug2(OPELOG, "Status = %s\n", pbuf);
			if(ret < 0) {
				zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n", MSG_ENC_STATUS_KEY);
			} else {
				proomt->EncInfo[id_index].Status = atoi((const char *)pbuf);
			}

			/* 高低码流信息节点 */
			pquality_node = get_children_node(pvideo_node, MSG_ENC_QUALITYINFO_KEY);
			quality_count = get_current_samename_node_nums(pquality_node);

			if(quality_count > RECORD_ROOM_ENC_MAX_QUALITY_NUM) {
				quality_count = RECORD_ROOM_ENC_MAX_QUALITY_NUM;
			}

			if(quality_count < 0) {
				quality_count = 0;
			}

			//			zlog_debug2(OPELOG, "quality_count = %d\n", quality_count);

			while(pquality_node && quality_count--) {
				/* 高低码流类型 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pquality_node, MSG_ENC_RATETYPE_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				if(ret < 0) {
					zlog_error(OPELOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n",
					           MSG_ENC_RATETYPE_KEY);
					pquality_node = find_next_node(pquality_node, MSG_ENC_QUALITYINFO_KEY);
					continue;
				} else {
					ratetype = atoi((const char *)pbuf);

					if(ratetype == 0)
						/* 高码流 */
					{
						pquality = &proomt->EncInfo[id_index].HD_QuaInfo;
					} else if(ratetype == 1)
						/* 低码流 */
					{
						pquality = &proomt->EncInfo[id_index].SD_QuaInfo;
					}

					pquality->enable = 1;
					pquality->RateType = ratetype;
				}

				//				zlog_debug2(OPELOG, "RateType = %d\n", ratetype);

				/* 码流 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pquality_node, MSG_ENC_BITRATE_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				//				zlog_debug2(OPELOG, "EncBitrate = %s\n", pbuf);
				if(ret < 0) {
					zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n",
					           MSG_ENC_BITRATE_KEY);
				} else {
					pquality->EncBitrate = atoi((const char *)pbuf);
				}

				/* 视频宽 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pquality_node, MSG_ENC_WIDTH_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				//				zlog_debug2(OPELOG, "EncWidth = %s\n", pbuf);
				if(ret < 0) {
					zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n",
					           MSG_ENC_WIDTH_KEY);
				} else {
					pquality->EncWidth = atoi((const char *)pbuf);
				}

				/* 视频高 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pquality_node, MSG_ENC_HEIGHT_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				//				zlog_debug2(OPELOG, "EncHeight = %s\n", pbuf);
				if(ret < 0) {
					zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n",
					           MSG_ENC_HEIGHT_KEY);
				} else {
					pquality->EncHeight = atoi((const char *)pbuf);
				}

				/* 视频帧率 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pquality_node, MSG_ENC_FRAMERATE_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				//				zlog_debug2(OPELOG, "EncFrameRate = %s\n", pbuf);
				if(ret < 0) {
					zlog_error(DBGLOG, "---[set_room_info_resolve_recv_buf_and_save], %s not found!\n",
					           MSG_ENC_FRAMERATE_KEY);
				} else {
					pquality->EncFrameRate = atoi((const char *)pbuf);
				}

				pquality_node = find_next_node(pquality_node, MSG_ENC_QUALITYINFO_KEY);
			}

			pvideo_node = find_next_node(pvideo_node, MSG_ENCINFO_KEY);
		}

		/* 将未出现过的编码器IP信息清除掉 */
		for(index = 0; index < array.room_num; index++) {
			if(array.data[index] == CONTROL_FALSE) {
				//				proomt->EncInfo[index].EncIP = ENC_INFO_SPE_VALUE;
				r_memset(proomt->EncInfo[index].EncIP, 0, IPINFO_MAX_LENGTH);
			}
		}

		/* 更新本地列表 */
		proom = &pser->RoomInfo[roomid];
		zlog_debug2(OPELOG, "set_room_info_resolve_recv_buf_and_save: save room %d info!!\n", roomid);
		pthread_mutex_lock(&pser->info_m);
		modify_room_info_only((const int8_t *)CONFIG_TABLE_FILE, proom, proomt);
		r_memcpy(proom, proomt, sizeof(room_info));
		pthread_mutex_unlock(&pser->info_m);
		return_code = STATUS_SUCCESS;
	}

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(proomt) {
		r_free(proomt);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}



int32_t set_room_info_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                              int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                              int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;

	if(NULL == penv || NULL == send_buf) {
		zlog_error(DBGLOG, "---[set_room_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		if(RecServer == puser->platform) {
			/* 此处为特殊处理 */
			/* 保存教室信息 */
			return_code = set_room_info_resolve_recv_buf_and_save(penv, puser, recv_buf + headlen);

			//			/* 响应消息 */
			//			return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
		} else {
			return_code = set_room_info_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
		}
	} else {
		/*设置教室信息日志上报*/
		int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
		platform_em platform;

		get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
		platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */
		//		report_set_room_info_log(penv, puser, recv_buf, platform);

		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


/********************************************************************************************/


/********************************************************************************************/
/* MSGCODE_SET_ROOM_QUALITY : 设置高低码流信息 */

int32_t set_room_quality_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
        roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[set_room_quality_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[set_room_quality_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pnode = get_children_node(pnode, MSG_SET_ROOMINFO_REQ_KEY);
	proom_node = get_children_node(pnode, MSG_ROOMINFO_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[set_room_quality_resolve_recv_buf], roomid not found!\n");
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[set_room_quality_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				continue;
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
		}
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t set_room_quality_save_info(control_env *pcon, con_user *puser, int8_t *recv_buf)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t enc_count = 0;
	int32_t quality_count = 0;
	int32_t roomid = 0;
	int32_t ratetype = 0;
	int32_t id = 0;
	int32_t id_index = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	xmlNodePtr pvideo_node = NULL;
	xmlNodePtr pquality_node = NULL;

	all_server_info *pser = NULL;
	room_info *proom = NULL;
	room_info *proomt = NULL;
	quality_info *pquality = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == puser) {
		zlog_error(DBGLOG, "---[set_room_quality_save_info] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	proomt = (room_info *)r_malloc(sizeof(room_info));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[set_room_quality_save_info] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pnode = get_children_node(pnode, MSG_SET_ROOMINFO_REQ_KEY);

	/* 编码器信息节点 */
	proom_node = get_children_node(pnode, MSG_ROOMINFO_KEY);

	/* 会议室ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[set_room_quality_save_info], roomid not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	roomid = atoi((const char *)pbuf);

	//	zlog_debug2(OPELOG, "roomid = %d\n", roomid);

	if(roomid == pcon->index && roomid >= 0 && roomid < pser->ServerInfo.MaxRoom) {
		proom = &pser->RoomInfo[roomid];
		r_memcpy(proomt, proom, sizeof(room_info));
		proomt->RoomID = roomid;

		/* 编码器信息节点 */
		pvideo_node = get_children_node(proom_node, MSG_ENCINFO_KEY);
		enc_count = get_current_samename_node_nums(pvideo_node);

		if(enc_count > RECORD_ROOM_MAX_ENC_NUM) {
			enc_count = RECORD_ROOM_MAX_ENC_NUM;
		}

		if(enc_count < 0) {
			enc_count = 0;
		}

		//		zlog_debug2(OPELOG, "enc_count = %d\n", enc_count);

		while(pvideo_node && enc_count--) {
			/* 编码器 ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			pnode = get_children_node(pvideo_node, MSG_ENC_ID_KEY);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[set_room_quality_save_info], %s not found!\n", MSG_ENC_ID_KEY);
				pvideo_node = find_next_node(pvideo_node, MSG_ENCINFO_KEY);
				continue;
			} else {
				id = atoi((const char *)pbuf);
			}

			if(id < 1 || id > RECORD_ROOM_MAX_ENC_NUM) {
				pvideo_node = find_next_node(pvideo_node, MSG_ENCINFO_KEY);
				continue;
			}

			id_index = id - 1;

			/* 注意索引值 */
			proomt->EncInfo[id_index].ID = id;

			/* 高低码流信息节点 */
			pquality_node = get_children_node(pvideo_node, MSG_ENC_QUALITYINFO_KEY);
			quality_count = get_current_samename_node_nums(pquality_node);

			if(quality_count > RECORD_ROOM_ENC_MAX_QUALITY_NUM) {
				quality_count = RECORD_ROOM_ENC_MAX_QUALITY_NUM;
			}

			if(quality_count < 0) {
				quality_count = 0;
			}

			//			zlog_debug2(OPELOG, "quality_count = %d\n", quality_count);

			while(pquality_node && quality_count--) {
				/* 高低码流类型 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pquality_node, MSG_ENC_RATETYPE_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				if(ret < 0) {
					zlog_error(OPELOG, "---[set_room_quality_save_info], %s not found!\n",
					           MSG_ENC_RATETYPE_KEY);
					pquality_node = find_next_node(pquality_node, MSG_ENC_QUALITYINFO_KEY);
					continue;
				} else {
					ratetype = atoi((const char *)pbuf);

					if(ratetype == 0)
						/* 高码流 */
					{
						pquality = &proomt->EncInfo[id_index].HD_QuaInfo;
					} else if(ratetype == 1)
						/* 低码流 */
					{
						pquality = &proomt->EncInfo[id_index].SD_QuaInfo;
					}

					pquality->enable = 1;
					pquality->RateType = ratetype;
				}

				//				zlog_debug2(OPELOG, "RateType = %d\n", ratetype);

				/* 码流 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pquality_node, MSG_ENC_BITRATE_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				//				zlog_debug2(OPELOG, "EncBitrate = %s\n", pbuf);
				if(ret < 0) {
					zlog_error(DBGLOG, "---[set_room_quality_save_info], %s not found!\n",
					           MSG_ENC_BITRATE_KEY);
				} else {
					pquality->EncBitrate = atoi((const char *)pbuf);
				}

				/* 视频宽 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pquality_node, MSG_ENC_WIDTH_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				//				zlog_debug2(OPELOG, "EncWidth = %s\n", pbuf);
				if(ret < 0) {
					zlog_error(DBGLOG, "---[set_room_quality_save_info], %s not found!\n",
					           MSG_ENC_WIDTH_KEY);
				} else {
					pquality->EncWidth = atoi((const char *)pbuf);
				}

				/* 视频高 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pquality_node, MSG_ENC_HEIGHT_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				//				zlog_debug2(OPELOG, "EncHeight = %s\n", pbuf);
				if(ret < 0) {
					zlog_error(DBGLOG, "---[set_room_quality_save_info], %s not found!\n",
					           MSG_ENC_HEIGHT_KEY);
				} else {
					pquality->EncHeight = atoi((const char *)pbuf);
				}

				/* 视频帧率 */
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
				pnode = get_children_node(pquality_node, MSG_ENC_FRAMERATE_KEY);
				ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

				//				zlog_debug2(OPELOG, "EncFrameRate = %s\n", pbuf);
				if(ret < 0) {
					zlog_error(DBGLOG, "---[set_room_quality_save_info], %s not found!\n",
					           MSG_ENC_FRAMERATE_KEY);
				} else {
					pquality->EncFrameRate = atoi((const char *)pbuf);
				}

				pquality_node = find_next_node(pquality_node, MSG_ENC_QUALITYINFO_KEY);
			}

			pvideo_node = find_next_node(pvideo_node, MSG_ENCINFO_KEY);
		}

		/* 更新本地列表 */
		proom = &pser->RoomInfo[roomid];
		zlog_debug2(OPELOG, "set_room_quality_save_info: save room %d info!!\n", roomid);
		pthread_mutex_lock(&pser->info_m);
		r_memcpy(&proomt->PicSyntInfo, &proom->PicSyntInfo, sizeof(pic_synt_info));	/* 不更新合成画面信息 */
		modify_room_info_only((const int8_t *)CONFIG_TABLE_FILE, proom, proomt);
		r_memcpy(proom, proomt, sizeof(room_info));
		pthread_mutex_unlock(&pser->info_m);
		return_code = STATUS_SUCCESS;
	}

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(proomt) {
		r_free(proomt);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}


int32_t set_room_quality_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                     int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[set_room_quality_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	set_room_quality_save_info(penv, puser, recv_buf + headlen);

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_SET_ROOMINFO_REQ_KEY, CONTROL_TRUE);
	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);
	timeout_add_con_user_addr(&penv->pserset->timeque, puser->platform, puser->index, puser->ipaddr);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[set_room_quality_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[set_room_quality_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_quality_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

			/*设置高低质量日志上报*/
			//			report_set_room_quality_log(penv, puser, send_buf, puser->platform);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

		/*设置高低质量日志上报*/
		//		report_set_room_quality_log(penv, puser, send_buf, puser->platform);
	}

EXIT:

	return return_code;
}


int32_t set_room_quality_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                 int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                 int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == send_buf) {
		zlog_error(DBGLOG, "---[set_room_quality_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = set_room_quality_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		/*设置高低质量日志上报*/
		int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
		platform_em platform;

		get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
		platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */
		//		report_set_room_quality_log(penv, puser, recv_buf, platform);

		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return STATUS_SUCCESS;
}

/********************************************************************************************/


/********************************************************************************************/
/* MSGCODE_SET_ROOM_AUDIO_INFO : 设置音频参数信息 */

int32_t set_room_audio_save_info(control_env *pcon, con_user *puser, int8_t *recv_buf)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	xmlNodePtr pvideo_node = NULL;

	all_server_info *pser = NULL;
	room_info *proom = NULL;
	room_info *proomt = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == puser) {
		zlog_error(DBGLOG, "---[set_room_audio_save_info] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	proomt = (room_info *)r_malloc(sizeof(room_info));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[set_room_audio_save_info] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pnode = get_children_node(pnode, MSG_SET_ROOM_AUDIO_INFO_REQ_KEY);
	proom_node = get_children_node(pnode, MSG_ROOMINFO_KEY);

	/* 会议室ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[set_room_audio_save_info], roomid not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	roomid = atoi((const char *)pbuf);

	//	zlog_debug2(OPELOG, "roomid = %d\n", roomid);

	if(roomid == pcon->index && roomid >= 0 && roomid < pser->ServerInfo.MaxRoom) {
		proom = &pser->RoomInfo[roomid];
		r_memcpy(proomt, proom, sizeof(room_info));
		proomt->RoomID = roomid;

		/* 获取音频inputindex 节点 */
		pvideo_node = get_children_node(proom_node, MSG_AUDIONFO_KEY);
		pvideo_node = get_children_node(pvideo_node, MSG_INPUTINDEX_KEY);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pvideo_node);
		zlog_error2(OPELOG, "set_room_audio_save_info: pbuf = %s!!\n", pbuf);

		r_bzero(proomt->AudioInfo.InputIndex, sizeof(proomt->AudioInfo.InputIndex));
		r_memcpy(proomt->AudioInfo.InputIndex, pbuf, 4);
		zlog_error2(OPELOG, "set_room_audio_save_info: proomt->AudioInfo.InputIndex = %s!!\n", proomt->AudioInfo.InputIndex);

		/* 更新本地列表 */
		proom = &pser->RoomInfo[roomid];
		zlog_debug2(OPELOG, "set_room_audio_save_info: save room %d info!!\n", roomid);
		pthread_mutex_lock(&pser->info_m);
		modify_room_info_only((const int8_t *)CONFIG_TABLE_FILE, proom, proomt);
		r_memcpy(proom, proomt, sizeof(room_info));
		pthread_mutex_unlock(&pser->info_m);
		return_code = STATUS_SUCCESS;
	}

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(proomt) {
		r_free(proomt);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}



int32_t set_room_audio_info_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
        roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[set_room_audio_info_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[set_room_audio_info_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pnode = get_children_node(pnode, MSG_SET_ROOM_AUDIO_INFO_REQ_KEY);
	proom_node = get_children_node(pnode, MSG_ROOMINFO_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[set_room_audio_info_resolve_recv_buf], roomid not found!\n");
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[set_room_audio_info_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				continue;
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
		}
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t set_room_audio_info_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                        int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[set_room_audio_info_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_SET_ROOM_AUDIO_INFO_REQ_KEY, CONTROL_TRUE);

	set_room_audio_save_info(penv, puser, recv_buf + headlen);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);
	timeout_add_con_user_addr(&penv->pserset->timeque, puser->platform, puser->index, puser->ipaddr);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[set_room_audio_info_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[set_room_audio_info_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_room_audio_info_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

			/*设置音频信息日志上报*/
			//			report_set_audio_info_log(penv, puser, send_buf, puser->platform);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

		/*设置音频信息日志上报*/
		//		report_set_audio_info_log(penv, puser, send_buf, puser->platform);
	}

EXIT:

	return return_code;
}


int32_t set_room_audio_info_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                    int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                    int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[set_room_audio_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);


	/* TODO: 注意若是会议室发来的信息 */

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = set_room_audio_info_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		/*设置音频信息日志上报*/
		int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
		platform_em platform;

		get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
		platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */
		//		report_set_audio_info_log(penv, puser, recv_buf, platform);

		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}

/********************************************************************************************/




/********************************************************************************************/
/* MSGCODE_GET_FTPUPLOAD_INFO : 获取FTP上传端口路径信息 (主动)*/

int32_t get_ftpupload_info_resolve_recv_buf(int8_t *recv_buf, ftp_info *pinfo)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr  pbodynode = NULL;
	xmlNodePtr  pinfonode = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pinfo) {
		zlog_error(DBGLOG, "---[get_ftpupload_info_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[get_ftpupload_info_resolve_recv_buf] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[get_ftpupload_info_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pinfonode = get_children_node(pbodynode, MSG_FILE_UPLOAD_PRERESQ_KEY);

	/* FtpServerIP */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(pinfonode, MSG_FTP_SERVERIP_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_ftpupload_info_resolve_recv_buf] failed, get %s error!\n", MSG_FTP_SERVERIP_KEY);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_strncpy((int8_t *)pinfo->MCFTPAddr, (int8_t *)pbuf, 24);

#if 0
	/* FtpPort */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(pinfonode, MSG_FTPPORT_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_ftpupload_info_resolve_recv_buf] failed, get %s error!\n", MSG_FTPPORT_KEY);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pinfo->MCFTPPort = atoi((const char *)pbuf);
#endif

	/* FtpUser */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(pinfonode, MSG_FTP_USER_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_ftpupload_info_resolve_recv_buf] failed, get %s error!\n", MSG_FTP_USER_KEY);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_strcpy((int8_t *)pinfo->MCFTPUserName, (const int8_t *)pbuf);

	/* FtpPasswd */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(pinfonode, MSG_FTP_PASSWORD_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_ftpupload_info_resolve_recv_buf] failed, get %s error!\n", MSG_FTP_PASSWORD_KEY);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_strcpy((int8_t *)pinfo->MCFTPPassword, (const int8_t *)pbuf);

	/* UploadPath */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(pinfonode, MSG_FTP_UPLOAD_PATH_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_ftpupload_info_resolve_recv_buf] failed, get %s error!\n", MSG_FTP_UPLOAD_PATH_KEY);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_strcpy((int8_t *)pinfo->MCFTPPath, (const int8_t *)pbuf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}



int32_t get_ftpupload_info_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                   int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	/* 暂时留空 */


#if 0

	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t ret = 0;
	ftp_info *pinfo = NULL;
	ftpcom_env *pftp = NULL;
	int8_t pipaddr[16] = {0};
	int8_t pport[16] = {0};
	int8_t *premote_addr = NULL;
	struct in_addr addr;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == ret_buf
	   || NULL == puser || NULL == ret_len) {
		zlog_error(DBGLOG, "---[get_ftpupload_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	pinfo = (ftp_info *)r_malloc(sizeof(ftp_info));

	if(NULL == pinfo) {
		zlog_error2(DBGLOG, "---[get_ftpupload_info_process] failed, ftp info malloc error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = get_ftpupload_info_resolve_recv_buf(recv_buf + headlen, pinfo);

	if(STATUS_FAILED == return_code) {
		zlog_error2(DBGLOG, "---[get_ftpupload_info_process] failed, [get_ftpupload_info_resolve_recv_buf] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	premote_addr = (int8_t *)r_malloc(FTP_COM_MAX_FILENAME_LENGTH);

	pftp = &penv->pserset->ftpser;

	if(pftp) {
		/* 发送用户名和密码 */
		ret = pftp->pcmd.set_server_info((void *)pftp, (const int8_t *)pinfo->MCFTPUserName,
		                                 (const int8_t *)pinfo->MCFTPPassword);

		if(ret < 0) {
			zlog_error2(DBGLOG, "---[get_ftpupload_info_process] failed, ftpcom set_server_info error!\n");
			return_code = STATUS_FAILED;
			goto EXIT;
		}

		/* FIXME: 响应消息中没有标识FTP端口，只使用默认的端口? */
		pinfo->MCFTPPort = 21;
		sprintf((char *)pport, "%d", pinfo->MCFTPPort);

		memcpy(&addr, &pinfo->MCFTPAddr, 4);
		r_strcpy(pipaddr, (const int8_t *)inet_ntoa(addr));
		/* FIXME: 媒体中心发过来的地址格式??? */
		sprintf((char *)premote_addr, "ftp:/%s%s", pipaddr, pinfo->MCFTPPath);

		/* 上传文件 */
		ret = pftp->pcmd.upload_file(pftp, pftp->filename, premote_addr, pport);

		if(ret < 0) {
			zlog_error2(DBGLOG, "---[get_ftpupload_info_process] failed, ftpcom set_server_info error!\n");
			return_code = STATUS_FAILED;
			goto EXIT;
		}
	}

EXIT:

	if(premote_addr) {
		r_free(premote_addr);
	}

#endif
	return return_code;
}



/********************************************************************************************/



/********************************************************************************************/
/* MSGCODE_SET_SERVER_SYS_PARAMS : 设置录播服务器系统参数*/

int32_t set_server_sys_params_resolve_recv_buf(int8_t *recv_buf,
        sys_info *psys)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr pfarnode = NULL;
	xmlNodePtr  pbodynode = NULL;

	int8_t *pbuf = NULL;
	struct in_addr addr;

	if(NULL == recv_buf || NULL == psys) {
		zlog_error(DBGLOG, "---[set_server_sys_params_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[set_server_sys_params_resolve_recv_buf] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[set_server_sys_params_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pfarnode = get_children_node(pbodynode, MSG_SET_SYS_PARAMS_KEY);

	/* 时间服务器地址 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(pfarnode, MSG_TIMESERVER_ADDR_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[set_server_sys_params_resolve_recv_buf] failed, [get_current_node_value] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	inet_aton((const char *)pbuf, &addr);
	r_memcpy(&psys->TimeServerAddr, &addr, 4);
#if 0
	/* 最大硬盘大小 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(pfarnode, MSG_TIMESERVER_ADDR_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[set_server_sys_params_resolve_recv_buf] failed, [get_current_node_value] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	psys->DiskSpace = atoi((const char *)pbuf);
#endif
	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}


int32_t set_server_sys_params_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                      int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                      int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t index = 0;
	int32_t ret = 0;

	sys_info new_sys;
	all_server_info *pinfo = NULL;
	con_user *pforobj = NULL;

	if(NULL == penv || NULL == recv_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[set_server_sys_params_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	pinfo = penv->pserset->pserinfo;

	if(NULL == pinfo) {
		zlog_error2(DBGLOG, "---[set_server_sys_params_process] failed, all server info is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pthread_mutex_lock(&pinfo->info_m);
	r_memcpy(&new_sys, &pinfo->SysInfo, sizeof(sys_info));
	pthread_mutex_unlock(&pinfo->info_m);

	set_server_sys_params_resolve_recv_buf(recv_buf + headlen, &new_sys);
	add_userid_to_xml_data(recv_buf + headlen, puser->index);

	for(index = 0; index < pinfo->ServerInfo.MaxRoom; index++) {
		/* 转发给所有在线会议室 */
		pforobj = find_forward_obj(penv->pserset, RecServer, index, 0);

		if(NULL == pforobj) {
			zlog_error2(DBGLOG, "---[set_server_sys_params_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[set_server_sys_params_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error2(DBGLOG, "---[set_server_sys_params_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	pthread_mutex_lock(&pinfo->info_m);
	/* 更新服务器系统信息 */
	modify_sys_info_only((const int8_t *)CONFIG_TABLE_FILE, &pinfo->SysInfo, &new_sys);
	r_memcpy(&pinfo->SysInfo, &new_sys, sizeof(sys_info));
	pthread_mutex_unlock(&pinfo->info_m);

	if(STATUS_SUCCESS == return_code) {
		package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);
	} else {
		package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_FAILED);
	}

	return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);


	/*设置录播信息日志上报*/
	Log_t log_info;
	int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};

	r_bzero(&log_info, sizeof(Log_t));
	sprintf((char *)log_info.Type, "%d", LOG_TYPE_SET_RECSERVER_INFO);
	sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	warn_get_current_time_info(log_info.Time);
	get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
	trans_platform_to_log_info_user2(pass_key, log_info.User);

	if(STATUS_SUCCESS == return_code) {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);


EXIT:

	return STATUS_SUCCESS;
}



/********************************************************************************************/


/********************************************************************************************/
/* MSGCODE_GET_SERVER_SYS_PARAMS : 获取录播服务器系统参数 */

static int32_t package_server_sys_params_xml_data(int8_t *send_buf,
        sys_info *psys, int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr  pbodynode = NULL;
	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	int8_t *pbuf = NULL;
	struct in_addr addr;

	if(NULL == send_buf || NULL == psys) {
		zlog_error(DBGLOG, "---[package_server_sys_params_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	package_resp_xml_data(send_buf, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[package_server_sys_params_xml_data] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)send_buf)) {
		zlog_error(DBGLOG, "---[package_server_sys_params_xml_data] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pnode = xmlNewNode(NULL, MSG_GET_SYS_PARAMS_KEY);
	xmlAddChild(pbodynode, pnode);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	memcpy(&addr, &psys->TimeServerAddr, 4);
	strcpy((char *)pbuf, inet_ntoa(addr));
	package_add_xml_leaf(pnode, MSG_TIMESERVER_ADDR_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", psys->DiskMaxSpace);
	package_add_xml_leaf(pnode, MSG_DISK_MAXSPACE_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", psys->DiskAvailableSpace);
	package_add_xml_leaf(pnode, MSG_DISK_AVALIABLESPACE_KEY, pbuf);

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}


int32_t get_server_sys_params_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                      int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                      int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	all_server_info *pinfo = NULL;
	sys_info sys;

	if(NULL == penv || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[get_server_sys_params_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	pinfo = penv->pserset->pserinfo;
	pthread_mutex_lock(&pinfo->info_m);
	r_memcpy(&sys, &pinfo->SysInfo, sizeof(sys_info));
	pthread_mutex_unlock(&pinfo->info_m);

	package_server_sys_params_xml_data(send_buf + headlen, &sys, msgcode);
	return_code = send_user_xml_data(NULL, puser, send_buf, ret_buf, ret_len);

#if 0
	int32_t index = 0;
	zlog_debug(OPELOG, "~~~~~~~~~~~~~~~~~~~~ roomid = %d\n", puser->port - 3200);

	for(index = 0; index < 10; index++) {
		print_live_user_info(&penv->lives_user[index]);
	}

#endif

EXIT:

	return return_code;
}


/********************************************************************************************/



/********************************************************************************************/
/* MSGCODE_RECORD_CONTROL : 录制控制操作 */

int32_t rec_control_resolve_recv_buf(int8_t *recv_buf, rec_control_reqst *preqst)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr  pbodynode = NULL;
	xmlNodePtr  pctrlynode = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == preqst) {
		zlog_error(DBGLOG, "---[rec_control_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[rec_control_resolve_recv_buf] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[rec_control_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pctrlynode = get_children_node(pbodynode, MSG_RECCTRL_REQ_KEY);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(pctrlynode, MSG_ROOTID_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[rec_control_resolve_recv_buf] failed, get %s error!\n", MSG_ROOTID_KEY);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	preqst->RoomID = atoi((const char *)pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(pctrlynode, MSG_OPTTYPE_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[rec_control_resolve_recv_buf] failed, get %s error!\n", MSG_OPTTYPE_KEY);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	preqst->OptType = atoi((const char *)pbuf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}

static int32_t package_rec_control_resp_data(int8_t *send_buf)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr pbodynode = NULL;
	xmlNodePtr proom_idnode = NULL;
	int8_t *pbuf = NULL;

	int32_t roomid = 0;
	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	if(NULL == send_buf) {
		zlog_error(DBGLOG, "---[package_rec_control_resp_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[package_rec_control_resp_data] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)send_buf)) {
		zlog_error(DBGLOG, "---[package_rec_control_resp_data] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
	proom_idnode = get_children_node(pbodynode, MSG_ROOMID_KEY);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, proom_idnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[package_rec_control_resp_data] failed, get %s error!\n", MSG_ROOTID_KEY);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	roomid = atoi((const char *)pbuf);
	del_cur_node(proom_idnode);

	pnode = xmlNewNode(NULL, MSG_RECCTRL_RESP_KEY);
	xmlAddChild(pbodynode, pnode);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", roomid);
	package_add_xml_leaf(pnode, MSG_ROOMID_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%s", XML_RETURNCODE_SUCCESS);
	package_add_xml_leaf(pnode, MSG_RESULT_KEY, pbuf);


	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}

int32_t rec_control_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                            int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                            int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t ret = 0;
	rec_control_reqst record_info;
	con_user *pforobj = NULL;
	int32_t opt_type = -1;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == ret_buf
	   || NULL == puser || NULL == ret_len) {
		zlog_error(DBGLOG, "---[rec_control_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	return_code = rec_control_resolve_recv_buf(recv_buf + headlen, &record_info);

	if(return_code != STATUS_SUCCESS) {
		zlog_error2(DBGLOG, "---[rec_control_process] failed, [rec_control_resolve_recv_buf] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	add_userid_to_xml_data(recv_buf + headlen, puser->index);

	/* FIXME: 此处作临时特殊处理，以roomid替换userid，仅30010适用 */
	/* 加入超时队列 */
	user_index = timeout_que_add_user2(&penv->pserset->timeque, puser->platform, msgcode, puser->index, record_info.RoomID);

	timeout_add_con_user_addr(&penv->pserset->timeque, puser->platform, puser->index, puser->ipaddr);
	opt_type = get_opt_type_from_record_req(recv_buf + headlen);

	/* 查找转发对象 */
	pforobj = find_forward_obj(penv->pserset, RecServer, record_info.RoomID, 0);

	if(NULL == pforobj) {
		zlog_error2(OPELOG, "---[rec_control_process] failed, [find_forward_obj] error!, obj: %d, roomid = %d\n",
		            RecServer, record_info.RoomID);
		return_code = STATUS_FAILED;

		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		goto EXIT;
	}

	if(NULL == pforobj->pcmd.forward_process) {
		zlog_error(DBGLOG, "---[rec_control_process] failed, [pcmd.forward_process] error!\n");
		return_code = STATUS_FAILED;

		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		goto EXIT;
	}

	/* 转发,注意在此函数返回前，需要释放对recv_buf的使用权 */
	ret = pforobj->pcmd.forward_process(puser, (void *)pforobj, recv_buf, msgcode, ret_buf, ret_len);

	if(ret < 0) {
		zlog_error2(DBGLOG, "---[rec_control_process] failed, [forward_process] error!\n");
	} else {
		return_code = STATUS_SUCCESS;
		zlog_debug2(DBGLOG, "---[rec_control_process]--->: forward_process ok!\n");
	}

	if(user_index >= 0) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 12);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, record_info.RoomID, \
			                      msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

			/*录制操作日志上报*/
			//			report_record_status_log2(penv, puser, opt_type, send_buf);
		}
	}

	return return_code;

EXIT:

	/* 失败强制返回 */
	puser->ack.has_return = CONTROL_TRUE;
	package_resp_xml_data(send_buf + headlen, record_info.RoomID,
	                      msgcode, (int8_t *)XML_RETURNCODE_FAILED);
	/* 封装具体响应的内容 */ /* 注意 TODO: RecCtrlResp */
	package_rec_control_resp_data(send_buf + headlen);
	return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);


	/*录制操作日志上报*/
	//	report_record_status_log2(penv, puser, opt_type, send_buf);

	return return_code;
}
/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_STREAM_REQUEST : 请求码流 */

int32_t stream_request_resolve_recv_buf_and_add_userid(control_env *pcon, int8_t *recv_buf,
        roomid_array *parray, int32_t user_index, int8_t *pEindex,
        const xmlChar *key, int32_t flag)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray || NULL == key || NULL == pEindex) {
		zlog_error(DBGLOG, "---[stream_request_resolve_recv_buf_and_add_userid] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[stream_request_resolve_recv_buf_and_add_userid] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);

	if(CONTROL_TRUE == flag) {
		pnode = get_children_node(pnode, key);
		proom_node = get_children_node(pnode, MSG_ROOMINFO_KEY);
	} else {
		proom_node = get_children_node(pnode, key);
	}

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[stream_request_resolve_recv_buf_and_add_userid], roomid not found!\n");
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[stream_request_resolve_recv_buf_and_add_userid], roomid error, roomid = %d\n", roomid);
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;

			pnode = get_children_node(proom_node, MSG_ENCODE_INDEX_KEY);
			/* 请求类型 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[stream_request_resolve_recv_buf_and_add_userid], EncodeIndex not found!\n");
			} else {
				r_memset(pEindex, 0, 64);
				r_strncpy(pEindex, pbuf, 64);
			}

			parray->room_num++;
			proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
		}
	}

	/* 用户ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", user_index);
	package_add_xml_leaf(pxml->proot, MSG_USERID_KEY, (int8_t *)pbuf);

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	/* FIXME，确保recv_buf的长度要大于 CONTROL_DATA_LEN */
	r_memset(recv_buf, 0, CONTROL_DATA_LEN - headlen);

	if(xmllen > (CONTROL_DATA_LEN - headlen)) {
		xmllen = CONTROL_DATA_LEN - headlen;
	}

	r_memcpy(recv_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}


static int32_t package_cleanup_lives_user_info_xml_data(int8_t *send_buf, int32_t userid)
{
	int32_t 	return_code = STATUS_FAILED;
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[16] = {0};
	int8_t		buf[16] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;
	xmlNodePtr	str_node = NULL;

	if(NULL == send_buf) {
		zlog_error(DBGLOG, "---[package_cleanup_lives_user_info_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(userid < 0 || userid >= CONTROL_MAX_USER) {
		zlog_error(DBGLOG, "---[package_cleanup_lives_user_info_xml_data] failed, userid error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	sprintf((char *)msgcode_buf, "%d", MSGCODE_STREAM_REQUEST);
	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);
	package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, (const int8_t *)PLATFORM_RECSERVER);

	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);
	str_node = xmlNewNode(NULL, MSG_STREAM_REQ_KEY);
	xmlAddChild(body_node, str_node);

	package_add_xml_leaf(str_node, MSG_ENCODE_INDEX_KEY, (int8_t *)"U");

	sprintf((char *)buf, "%d", userid);
	package_add_xml_leaf(root_node, MSG_USERID_KEY, (const int8_t *)buf);

	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	return return_code;;
}

static int32_t package_stream_request_data(int8_t *send_buf, lives_info *plive, int32_t roomid)
{
	int32_t 	return_code = STATUS_FAILED;
	int32_t 	xmllen = 0;
	int32_t		index = 0;
	int8_t		msgcode_buf[16] = {0};
	int8_t		key[32] = {0};
	int8_t 		*pbuf = NULL;

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;
	xmlNodePtr	str_node = NULL;

	if(NULL == send_buf || NULL == plive) {
		zlog_error(DBGLOG, "---[package_stream_request_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf) {
		zlog_error(DBGLOG, "---[package_stream_request_data] failed, malloc error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	sprintf((char *)msgcode_buf, "%d", MSGCODE_STREAM_REQUEST);
	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);
	package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, (const int8_t *)PLATFORM_RECSERVER);

	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	str_node = xmlNewNode(NULL, MSG_STREAM_REQ_KEY);
	xmlAddChild(body_node, str_node);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", roomid);
	package_add_xml_leaf(str_node, MSG_ROOMID_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", plive->quality_type);
	package_add_xml_leaf(str_node, MSG_ROOMQUALITY_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%s", plive->encode_index);
	package_add_xml_leaf(str_node, MSG_ENCODE_INDEX_KEY, pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", plive->userid);
	package_add_xml_leaf(root_node, MSG_USERID_KEY, pbuf);

	for(index = 0; index < VIDEO_ENCODE_MAX_NUM; index++) {
		if(r_strlen(plive->addr[index].user_ip) != 0 && plive->addr[index].user_port != 0) {
			r_memset(key, 0, 32);
			sprintf((char *)key, "Ipaddr%d", index + 1);
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			sprintf((char *)pbuf, "%s", plive->addr[index].user_ip);
			package_add_xml_leaf(str_node, (const xmlChar *)key, pbuf);

			r_memset(key, 0, 32);
			sprintf((char *)key, "Port%d", index + 1);
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			sprintf((char *)pbuf, "%d", plive->addr[index].user_port);
			package_add_xml_leaf(str_node, (const xmlChar *)key, pbuf);
		}
	}


	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	return return_code;;
}



int32_t stream_request_cleanup_lives_user_info(control_env *penv, con_user *puser)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t ret = 0;
	int32_t ret_lenght = 0;
	int32_t index = 0;

	con_user *pforobj = NULL;

	int8_t *send_buf = NULL;

	zlog_debug(OPELOG, "<<<<<<<<<<<<<<<<<<<<<<< stream request start cleanup! >>>>>>>>>>>>>>>>>>>\n");

	if(NULL == puser || NULL == penv) {
		zlog_error(DBGLOG, "---[stream_request_cleanup_lives_user_info] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	send_buf = r_malloc(CONTROL_DATA_LEN);

	if(NULL == send_buf) {
		zlog_error(DBGLOG, "---[stream_request_cleanup_lives_user_info] failed, send_buf is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_memset(send_buf, 0, CONTROL_DATA_LEN);
	return_code = package_cleanup_lives_user_info_xml_data(send_buf + headlen, puser->index);

	if(STATUS_FAILED == return_code) {
		zlog_error(DBGLOG, "---[stream_request_cleanup_lives_user_info] failed, package_cleanup_lives_user_info_xml_data error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	/* 广播给会议室，进行 */
	for(index = 0; index < penv->pserset->pserinfo->ServerInfo.MaxRoom; index++) {
		pforobj = &penv->pserset->roomser[index].user[0];

		if(NULL == pforobj) {
			zlog_error2(DBGLOG, "---[stream_request_cleanup_lives_user_info] failed, get room obj error!\n");
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[stream_request_cleanup_lives_user_info] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, send_buf, MSGCODE_STREAM_REQUEST,
		                                    puser->recv_buf, &ret_lenght);

		if(ret < 0) {
			zlog_error2(DBGLOG, "---[stream_request_cleanup_lives_user_info] failed, [forward_process] error, index = %d\n", index);
			continue;
		}
	}

	zlog_debug(OPELOG, "<<<<<<<<<<<<<<<<<<<<<<< stream request end cleanup! >>>>>>>>>>>>>>>>>>>\n");

	return_code = STATUS_SUCCESS;

EXIT:

	if(send_buf) {
		r_free(send_buf);
	}

	if(STATUS_SUCCESS == return_code) {
		zlog_debug(OPELOG, "<<<<<<<<<<<<<<<<<<<<<<< stream request end clenup! >>>>>>>>>>>>>>>>>>>\n");
	} else {
		zlog_debug(OPELOG, "<<<<<<<<<<<<<<<<<<<<<<< stream request clenup failed! >>>>>>>>>>>>>>>>>>>\n");
	}

	return return_code;
}

int32_t package_livenode_resp_xml_data(int8_t *out_buf, int32_t roomid, int8_t *pEindex, int32_t msgcode,
                                       int8_t *ret_code)
{
	int32_t 	return_code = STATUS_FAILED;
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[16] = {0};
	int8_t		buf[16] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;

	if(NULL == out_buf || NULL == ret_code || NULL == pEindex) {
		zlog_error(DBGLOG, "---[package_livenode_resp_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, RESP_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	sprintf((char *)msgcode_buf, "%d", msgcode);

	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);
	package_add_xml_leaf(head_node, MSG_RETURNCODE_KEY, ret_code);
	package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, (const int8_t *)PLATFORM_RECSERVER);

	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	if(roomid != -1) {
		sprintf((char *)buf, "%d", roomid);
		package_add_xml_leaf(body_node, MSG_ROOMID_KEY, (int8_t *)buf);
	}

	package_add_xml_leaf(body_node, MSG_ENCODE_INDEX_KEY, (int8_t *)pEindex);

	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	return return_code;;
}


int32_t stream_request_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	int8_t encode_index[64] = {0};

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[stream_request_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	r_memset(encode_index, 0, 64);
	stream_request_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	        encode_index, MSG_STREAM_REQ_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	zlog_debug(DBGLOG, "array.room_num = %d\n", array.room_num);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[stream_request_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[stream_request_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[stream_request_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
		//		if(LiveNode == puser->platform){
		/* 保存的直播节点请求码流的用户信息 */
		//			stream_request_resolve_and_save_lives_user_info(puser, recv_buf+headlen);
		//		}
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);

			if(LiveNode == puser->platform) {
				package_livenode_resp_xml_data(send_buf + headlen, array.roomid[0], encode_index, msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			} else {
				package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			}

			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		if(LiveNode == puser->platform) {
			package_livenode_resp_xml_data(send_buf + headlen, array.roomid[0], encode_index, msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		} else {
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		}

		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


/* 确保recv_buf的长度要大于 CONTROL_DATA_LEN */
int32_t modify_resp_xml_data_and_save_live_info(int8_t *recv_buf, int8_t *passkey,
        lives_info *plive, int32_t *returncode, platform_em platform)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	xmlNodePtr puserid_node = NULL;
	xmlNodePtr passkey_node = NULL;
	xmlNodePtr pret_node = NULL;
	int32_t xmllen = 0;
	int32_t index = 0;
	int32_t ret = 0;
	int8_t *pbuf = NULL;
	int8_t key[32] = {0};
	xmlChar *xml_buf = NULL;

	if(NULL == recv_buf || NULL == passkey || NULL == plive || NULL == returncode) {
		zlog_error(DBGLOG, "---[modify_resp_xml_data_and_save_live_info] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[modify_resp_xml_data_and_save_live_info] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);

	proom_node = get_children_node(pnode, MSG_STREAM_REQ_KEY);

#if 0
	/* 会议室ID */
	pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[modify_resp_xml_data_and_save_live_info], roomid not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	roomid = atoi((const char *)pbuf);

	if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
		zlog_error(DBGLOG,
		           "---[modify_resp_xml_data_and_save_live_info], roomid error, roomid = %d\n", roomid);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

#endif

	if(RecServer != platform) {
		/* 请求类型 */
		pnode = get_children_node(proom_node, MSG_ENCODE_INDEX_KEY);
		r_memset(plive->encode_index, 0, VIDEO_ENCODE_INDEX_LEN);
		ret = get_current_node_value((char *)plive->encode_index, VIDEO_ENCODE_INDEX_LEN, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[modify_resp_xml_data_and_save_live_info], EncodeIndex not found!\n");
			return_code = STATUS_FAILED;
			goto EXIT;
		}

		zlog_debug(OPELOG, "encode_index: %s\n", plive->encode_index);

		pnode = get_children_node(proom_node, MSG_ROOMQUALITY_KEY);
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[modify_resp_xml_data_and_save_live_info], quality not found!\n");
			return_code = STATUS_FAILED;
			goto EXIT;
		}

		plive->quality_type = atoi((const char *)pbuf);

		zlog_debug(OPELOG, "quality_type: %d\n", plive->quality_type);

		for(index = 0; index < VIDEO_ENCODE_MAX_NUM; index++) {
			r_memset(key, 0, 32);
			sprintf((char *)key, "Ipaddr%d", index + 1);
			pnode = get_children_node(proom_node, (const xmlChar *)key);
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			r_memset(plive->addr[index].user_ip, 0, VIDEO_SEND_IP_LEN);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[modify_resp_xml_data_and_save_live_info], %s not found!\n", key);
				continue;
			}

			r_strncpy(plive->addr[index].user_ip, pbuf, VIDEO_SEND_IP_LEN);

			r_memset(key, 0, 32);
			sprintf((char *)key, "Port%d", index + 1);
			pnode = get_children_node(proom_node, (const xmlChar *)key);
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			plive->addr[index].user_port = 0;
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[modify_resp_xml_data_and_save_live_info], %s not found!\n", key);
				r_memset(plive->addr[index].user_ip, 0, VIDEO_SEND_IP_LEN);
				continue;
			}

			plive->addr[index].user_port = atoi((const char *)pbuf);

			zlog_debug(OPELOG, "index: %d, ip = %s, port = %d\n", index,
			           plive->addr[index].user_ip, plive->addr[index].user_port);
		}
	}

	pnode = get_children_node(pxml->proot, MSG_HEAD_KEY);
	pret_node = get_children_node(pnode, MSG_RETURNCODE_KEY);
	passkey_node = get_children_node(pnode, MSG_PASSKEY_KEY);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pret_node);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[modify_resp_xml_data_and_save_live_info], ReturnCode not found!\n");
		*returncode = 0;
	} else {
		*returncode = atoi((const char *)pbuf);
	}


	/* TODO:修改成功了吗 */
	modify_node_value(passkey_node, (const xmlChar *)passkey);

	puserid_node = get_children_node(pxml->proot, MSG_USERID_KEY);

	if(puserid_node) {
		if(RecServer != platform) {
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, puserid_node);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[modify_resp_xml_data_and_save_live_info], userid not found!\n");
				plive->userid = -1;
			} else {
				plive->userid = atoi((const char *)pbuf);
			}
		}

		del_cur_node(puserid_node);
	}

	if(proom_node) {
		del_cur_node(proom_node);
	}

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	/* FIXME，确保recv_buf的长度要大于 CONTROL_DATA_LEN */
	r_memset(recv_buf, 0, CONTROL_DATA_LEN - headlen);

	if(xmllen > (CONTROL_DATA_LEN - headlen)) {
		xmllen = CONTROL_DATA_LEN - headlen;
	}

	r_memcpy(recv_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}


int32_t stream_request_resp_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                    int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = -1;
	int32_t userid = 0;
	int32_t index = 0;
	int32_t ret = 0;
	int32_t retcode = 0;
	platform_em platform;
	con_user *pforobj = NULL;
	int8_t passkey[64] = {0};
	roomid_array array;
	lives_info live;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[stream_request_resp_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + headlen, passkey, &userid);
	platform = get_platform_from_passkey(passkey);		/* 转发的目标平台 */

	user_index = timeout_que_find_user(&penv->pserset->timeque, platform, msgcode, userid);

	if(user_index < 0) {
		/* FIXME:不再返回吗? */
		goto EXIT;
	}

	/* FIXME:先删除超时节点还是先进行返回处理? */
	timeout_que_del_user(&penv->pserset->timeque, user_index);

	r_memset(passkey, 0, 64);
	get_passkey_from_platform(puser->platform, passkey);

	r_memset(&live, 0, sizeof(lives_info));
	return_code = modify_resp_xml_data_and_save_live_info(recv_buf + headlen, passkey, &live, &retcode, platform);
	zlog_debug(OPELOG, "return_code = %d, platform = %d, retcode = %d\n", return_code, platform, retcode);

	if(STATUS_SUCCESS == return_code && RecServer == puser->platform && 1 == retcode && live.userid >= 0
	   && platform != RecServer) {
		save_live_user_info(penv, &live);
	}

	array.room_num = 1;
	array.roomid[0] = 0;

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, platform, array.roomid[index], userid);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[stream_request_resp_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[stream_request_resp_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[stream_request_resp_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	/* FIXME: 如果有多个转发对象的时候? */

EXIT:

	return return_code;
}


int32_t stream_request_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                               int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                               int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[stream_request_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = stream_request_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = stream_request_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}



/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_REBOOT_SERVER : 重启服务器 */

int32_t reboot_server_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                              int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                              int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t can_reboot = 0;
	int8_t buf[16] = {0};

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[reboot_server_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	usleep(300000);

	if(check_all_record_status(penv->pserset) != 1) {
		can_reboot = CONTROL_TRUE;
		return_code = package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);
	} else {
		can_reboot = CONTROL_FALSE;
		sprintf((char *)buf, "%d", ERRNO_IS_RECORDING);
		return_code = package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)buf);
	}

	return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

	/*重启录播系统日志上报*/
	Log_t log_info;
	int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};

	r_bzero(&log_info, sizeof(Log_t));
	sprintf((char *)log_info.Type, "%d", LOG_TYPE_REBOOT_RECSERVER);
	sprintf((char *)log_info.Addr, "%s", puser->ipaddr);
	warn_get_current_time_info(log_info.Time);
	get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
	trans_platform_to_log_info_user2(pass_key, log_info.User);

	if(CONTROL_TRUE == can_reboot) {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_SUCC_CONTENT);
	} else {
		r_strcpy(log_info.Content, (int8_t *)MSG_LOG_FAIL_CONTENT);
	}

	zlog_debug(OPELOG, "--------log_info.Type = %s--------puser->platform = [%d]-----------!\n", log_info.Type, puser->platform);
	report_log_info_process(penv->pserset, &log_info);

	if(CONTROL_TRUE == can_reboot) {
		reboot_server();
	}

EXIT:

	return STATUS_SUCCESS;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_CAMERA_CTROL : 远遥摄像头 */

int32_t camera_control_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
                                        roomid_array *parray, int32_t user_index)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;
	int32_t encid = 1;
	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr pctrl_node = NULL;
	xmlNodePtr proom_node = NULL;
	xmlNodePtr penc_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[camera_control_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;
	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[camera_control_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pctrl_node = get_children_node(pnode, MSG_REMOTE_CTRL_KEY);
	proom_node = get_children_node(pctrl_node, MSG_ROOMINFO_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			/* 会议室ID */
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[camera_control_resolve_recv_buf], roomid not found!\n");
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG, "---[camera_control_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			/* ENCINFO 结点 */
			penc_node = get_children_node(proom_node, MSG_ENCINFO_KEY);

			/* ENC ID */
			pnode = get_children_node(penc_node, MSG_ENC_ID_KEY);
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[camera_control_resolve_recv_buf], enc id not found!\n");
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			encid = atoi((const char *)pbuf);

			if(encid <= 0 || encid >= RECORD_ROOM_MAX_ENC_NUM + 1) {
				zlog_error(DBGLOG, "---[camera_control_resolve_recv_buf], enc id error, enc id = %d\n", encid);
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			pnode = get_children_node(penc_node, MSG_CAMERA_CTRL_ADDR_KEY);

			if(pnode) {
				r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);

				switch(encid) {
					case 1:
						zlog_error(OPELOG, "video0_addr -----------= %d\n", pser->CamCtrlInfo.video0_addr);
						sprintf((char *)pbuf, "%d", pser->CamCtrlInfo.video0_addr);
						modify_node_value(pnode, (const xmlChar *)pbuf);
						break;

					case 2:
						zlog_error(OPELOG, "video1_addr -----------= %d\n", pser->CamCtrlInfo.video1_addr);
						sprintf((char *)pbuf, "%d", pser->CamCtrlInfo.video1_addr);
						modify_node_value(pnode, (const xmlChar *)pbuf);
						break;

					case 3:
						zlog_error(OPELOG, "video2_addr -----------= %d\n", pser->CamCtrlInfo.video2_addr);
						sprintf((char *)pbuf, "%d", pser->CamCtrlInfo.video2_addr);
						modify_node_value(pnode, (const xmlChar *)pbuf);
						break;

					default:
						break;
				}
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
			proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
		}
	}

	/* 用户ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", user_index);
	package_add_xml_leaf(pxml->proot, MSG_USERID_KEY, (int8_t *)pbuf);

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	/* FIXME，确保recv_buf的长度要大于 CONTROL_DATA_LEN */
	r_memset(recv_buf, 0, CONTROL_DATA_LEN - headlen);

	if(xmllen > (CONTROL_DATA_LEN - headlen)) {
		xmllen = CONTROL_DATA_LEN - headlen;
	}

	r_memcpy(recv_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t camera_control_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;
	int32_t hdsock = -1;
	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[camera_control_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	camera_control_resolve_recv_buf(penv, recv_buf + headlen, &array, puser->index);
#if 0
	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[camera_control_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[camera_control_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[camera_control_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}
#endif
		hdsock = connect_HD_card();
		MsgHeader *pMhead = (MsgHeader *)recv_buf;
		int32_t sendlen = CONTROL_MSGHEAD_LEN + r_strlen(recv_buf + headlen);
		pMhead->sLen = r_htons(sendlen);
		pMhead->sVer = 1;
		pMhead->sMsgType = 0x17;
		pMhead->sData = 0;
		zlog_error(OPELOG, "slen:[%d]\n", sendlen);
		ret = tcp_send_longdata(hdsock, recv_buf, sendlen);
		if(ret < 0) {
			zlog_error(OPELOG, "---[upload_camctl_pro_req_process] failed, error!\n");
			return_code = STATUS_FAILED;
		} else {
			int datalen = 0;
			int force_msgcode = -1;
			r_memset(recv_buf, 0, r_strlen(recv_buf + headlen));
			return_code = recv_user_xml_data(hdsock, recv_buf+headlen, &force_msgcode, NULL, &datalen);
			/*升级LOGO 日志上报*/
			int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
			platform_em platform;
			zlog_error(OPELOG, "\n[%s]\n", recv_buf+headlen);
			get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
			platform = get_platform_from_passkey(pass_key); 	/* 转发的目标平台 */

			return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
		}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:
	close(hdsock);
	return return_code;
}


int32_t camera_control_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                               int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                               int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[camera_control_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = camera_control_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	}
#if 0
	else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}
#endif
EXIT:

	return return_code;
}



/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_IFRAME_REQUEST : 请求I帧 */

int32_t iframe_request_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
                                        roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[iframe_request_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;
	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[iframe_request_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	proom_node = get_children_node(pnode, MSG_IFRAME_REQ_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[iframe_request_resolve_recv_buf], roomid not found!\n");
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[iframe_request_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				continue;
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
		}
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t iframe_request_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[iframe_request_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_IFRAME_REQ_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[iframe_request_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[iframe_request_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[iframe_request_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}

int32_t iframe_request_resp_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                    int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = -1;
	int32_t userid = 0;
	int32_t index = 0;
	int32_t ret = 0;
	platform_em platform;
	con_user *pforobj = NULL;
	int8_t passkey[64] = {0};
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[iframe_request_resp_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + headlen, passkey, &userid);
	platform = get_platform_from_passkey(passkey);		/* 转发的目标平台 */
	user_index = timeout_que_find_user(&penv->pserset->timeque, platform, msgcode, userid);

	if(user_index < 0) {
		/* FIXME:不再返回吗? */
		goto EXIT;
	}

	/* FIXME:先删除超时节点还是先进行返回处理? */
	timeout_que_del_user(&penv->pserset->timeque, user_index);

	r_memset(passkey, 0, 64);
	get_passkey_from_platform(puser->platform, passkey);
	modify_resp_xml_data_passkey(recv_buf + headlen, passkey);

	array.room_num = 1;
	array.roomid[0] = 0;

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, platform, array.roomid[index], userid);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[iframe_request_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[iframe_request_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[iframe_request_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	/* FIXME: 如果有多个转发对象的时候? */

EXIT:

	return return_code;
}


int32_t iframe_request_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                               int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                               int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[iframe_request_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = iframe_request_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}




/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_CONNECT_ROOM_REQUEST : 连接或断开会议室 */

int32_t connect_room_request_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
        roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;
	int32_t opttype = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[connect_room_request_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;
	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[connect_room_request_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	proom_node = get_children_node(pnode, MSG_CONNECT_ROOM_REQ_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[connect_room_request_resolve_recv_buf], roomid not found!\n");
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[connect_room_request_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				continue;
			}

			pnode = get_children_node(proom_node, MSG_OPT_TYPE_KEY);
			/* 连接或断开会议室的标志值 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[connect_room_request_resolve_recv_buf], roomid not found!\n");
				continue;	/* 读下组 RoomInfo */
			}

			opttype = atoi((const char *)pbuf);

			/* 保存 roomid 和 opttype */
			parray->roomid[parray->room_num] = roomid;
			parray->data[parray->room_num] = opttype;
			parray->room_num++;
		}
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t connect_room_request_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
        int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;
	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[connect_room_request_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_CONNECT_ROOM_REQ_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);
	ret = timeout_add_con_user_addr(&penv->pserset->timeque, puser->platform, puser->index, puser->ipaddr);
	zlog_debug(OPELOG, "----timeout_add_con_user_addr----ret = [%d]-----------!\n", ret);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[connect_room_request_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[connect_room_request_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[connect_room_request_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

#if 0
	pinfo = penv->pserset->pserinfo;

	pthread_mutex_lock(&pinfo->info_m);

	/* 保存连接状态 */
	for(index = 0; index < array.room_num; index++) {
		if(0 == array.data[index]) {
			pinfo->RoomInfo[index].ConnStatus = 0;
		} else if(1 == array.data[index]) {
			pinfo->RoomInfo[index].ConnStatus = 1;
		}
	}

	pthread_mutex_unlock(&pinfo->info_m);
#endif

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 8);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

			/*连接教室日志上报*/
			//			report_connect_room_req_log(penv, puser, send_buf, puser->platform);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

		/*连接教室日志上报*/
		//		report_connect_room_req_log(penv, puser, send_buf, puser->platform);

	}

EXIT:

	return return_code;
}

int32_t connect_room_request_resp_process(control_env *penv, con_user *puser, int8_t *recv_buf,
        int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = -1;
	int32_t userid = 0;
	int32_t index = 0;
	int32_t ret = 0;
	platform_em platform;
	con_user *pforobj = NULL;
	int8_t passkey[64] = {0};
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[connect_room_request_resp_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + headlen, passkey, &userid);
	platform = get_platform_from_passkey(passkey);		/* 转发的目标平台 */

	user_index = timeout_que_find_user(&penv->pserset->timeque, platform, msgcode, userid);

	if(user_index < 0) {
		/* FIXME:不再返回吗? */
		goto EXIT;
	}

	/* FIXME:先删除超时节点还是先进行返回处理? */
	timeout_que_del_user(&penv->pserset->timeque, user_index);

	r_memset(passkey, 0, 64);
	get_passkey_from_platform(puser->platform, passkey);
	modify_resp_xml_data_passkey(recv_buf + headlen, passkey);

	array.room_num = 1;
	array.roomid[0] = 0;

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, platform, array.roomid[index], userid);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[connect_room_request_resp_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[connect_room_request_resp_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[connect_room_request_resp_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	/* FIXME: 如果有多个转发对象的时候? */

EXIT:

	return return_code;
}


int32_t connect_room_request_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                     int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                     int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[connect_room_request_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = connect_room_request_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		/*连接教室日志上报*/
		int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
		platform_em platform;

		get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
		platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */
		//		report_connect_room_req_log(penv, puser, recv_buf, platform);

		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}



/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_UPLOAD_LOGO_PICTURE : 上传LOGO图片 */

int32_t upload_logo_picture_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                        int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[upload_logo_picture_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_LOGO_UPLOADLOG_KEY, CONTROL_FALSE);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[upload_logo_picture_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[upload_logo_picture_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[upload_logo_picture_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	if(user_index >= 0) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);

			if(STATUS_SUCCESS == return_code)
				package_resp_xml_data(send_buf + headlen, array.roomid[0],
				                      msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);
			else
				package_resp_xml_data(send_buf + headlen, array.roomid[0],
				                      msgcode, (int8_t *)XML_RETURNCODE_FAILED);

			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

			/*升级LOGO 日志上报*/
			//		report_upload_logo_picture_log(penv, puser, send_buf, puser->platform);

		}
	}

EXIT:

	return return_code;
}

int32_t upload_logo_picture_req_process2(control_env *penv, con_user *puser, int8_t *recv_buf,
        int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;
	int32_t data_length;
	int32_t xml_len;
	int32_t recvlen = 0;
	int hdsock = -1;
	int8_t *send_buf2 = NULL;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[upload_camctl_pro_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	upload_logo_pro_resolve_recv_buf(penv, recv_buf + headlen, &send_buf2, &array, puser->index,
	                                 &data_length, &xml_len);

	if(data_length > 0) {
		if(send_buf2 != NULL) {
			recvlen = tcp_recv_longdata(puser->tcp_sock, send_buf2 + CONTROL_MSGHEAD_LEN + xml_len, data_length);

			if(recvlen != data_length) {
				array.room_num = 0;
			}
		} else {
			// TODO:防止data_length有值，而send_buf2为NULL的情形
		}
	}
#if 0
	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(OPELOG, "---[upload_camctl_pro_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(OPELOG, "---[upload_camctl_pro_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = send_user_xml_data2(puser, pforobj, send_buf2, xml_len, data_length);

		if(ret < 0) {
			zlog_error(OPELOG, "---[upload_camctl_pro_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}
#endif
	hdsock = connect_HD_card();
	int sendlen = CONTROL_MSGHEAD_LEN + xml_len;
	MsgHeader *pMhead = (MsgHeader *)send_buf2;
	pMhead->sLen = r_htons(sendlen);
	pMhead->sVer = 1;
	pMhead->sMsgType = 0x17;
	pMhead->sData = 0;
	zlog_error(OPELOG, "\n[%d]\n", sendlen);
	ret = tcp_send_longdata(hdsock, send_buf2, sendlen+data_length);
	if(ret < 0) {
		zlog_error(OPELOG, "---[upload_camctl_pro_req_process] failed, error!\n");
		return_code = STATUS_FAILED;
	} else {
		int datalen = 0;
		int force_msgcode = -1;
		r_memset(recv_buf, 0, r_strlen(recv_buf + headlen));
		return_code = recv_user_xml_data(hdsock, recv_buf+headlen, &force_msgcode, NULL, &datalen);
		/*升级LOGO 日志上报*/
		int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
		platform_em platform;
		zlog_error(OPELOG, "\n[%s]\n", recv_buf+headlen);
		get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
		platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */

		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, recv_buf, ret_buf, ret_len);
		}

		system("cp /opt/logo.png /opt/dvr_rdk/ti816x/logo_0.png");
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, recv_buf, ret_buf, ret_len);
	}

	/*升级LOGO 日志上报*/
	//	report_upload_logo_picture_log(penv, puser, send_buf, puser->platform);

EXIT:
	close(hdsock);
	return return_code;
}



int32_t upload_logo_picture_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                    int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                    int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[upload_logo_picture_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);



	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = upload_logo_picture_req_process2(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	}
#if 0
	else {
		/*升级LOGO 日志上报*/
		int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
		platform_em platform;

		get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
		platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */
		//		report_upload_logo_picture_log(penv, puser, recv_buf, platform);

		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}
#endif
EXIT:

	return return_code;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_UPDATE_SERVER : 系统升级 */

int32_t update_server_resolve_recv_buf(int8_t *recv_buf, user_info *puser,
                                       update_info *pupdate)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr preqnode = NULL;
	xmlNodePtr pnode = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == puser || NULL == pupdate) {
		zlog_error(DBGLOG, "---[update_server_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[update_server_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	preqnode = get_children_node(pnode, MSG_SYS_UPGRADE_REQ_KEY);

	/* 只进行服务器升级 */

	/* 升级类型 */
	pnode = get_children_node(preqnode, MSG_SYS_UPGRADE_OPEREATE_KEY);
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[update_server_resolve_recv_buf], operate not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pupdate->Opereate = atoi((const char *)pbuf);

	/* 升级包长度 */
	pnode = get_children_node(preqnode, MSG_SYS_UPGRADE_LENGTH_KEY);
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[update_server_resolve_recv_buf], length not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pupdate->Length = atoi((const char *)pbuf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}




int32_t update_server_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                              int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                              int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	user_info user;
	update_info update;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser || NULL == gpser) {
		zlog_error(DBGLOG, "---[update_server_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	return_code = update_server_resolve_recv_buf(recv_buf + headlen, &user, &update);

	if(return_code != STATUS_SUCCESS) {
		zlog_error(DBGLOG, "---[update_server_process] failed, [update_server_resolve_recv_buf] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

#if 1
	return_code = STATUS_FAILED;

	if(update.Opereate == 0) {
		int32_t ret = 0;
		/* TODO: 接收数据，升级 */
		pthread_mutex_lock(&puser->sock_m);
		ret = check_recvfile((int8_t *)CONTROL_UPDATE_FILE, puser->tcp_sock, update.Length);
		pthread_mutex_unlock(&puser->sock_m);

		if(1 == check_all_record_status(gpser)) {

		} else {
			//8168升级
			if(ret == 0) {
				ret = UpgradeApp((int8_t *)CONTROL_UPDATE_FILE);

				if(ret != SERVER_VERIFYFILE_FAILED) {
					if(ret == SERVER_RET_OK) {

						return_code = app_update_HD_card((int8_t *)DM6467_UPDATE_PACKET);

						if(ret == SERVER_RET_OK) {
							return_code = STATUS_SUCCESS;
						}

						updatekernel();
						rebootsys(5);
					}
				}

			}
			//6467升级
			else if(ret == 1) {
				ret = app_update_HD_card((int8_t *)CONTROL_UPDATE_FILE);

				if(ret == SERVER_RET_OK) {
					return_code = STATUS_SUCCESS;
					//rebootsys(5);
				}

			}
		}
	} else if(update.Opereate == 1) {
		if(1 == check_all_record_status(gpser)) {

		} else {

			int32_t ret = 0;
			ret = SysRollback();

			if(ret != SERVER_VERIFYFILE_FAILED) {
				if(ret == 0) {
					return_code = STATUS_SUCCESS;
					rebootsys(5);
				}
			}


		}

	}

#endif


EXIT:

	if(send_buf != NULL) {
		/*升级录播系统日志上报*/
		//		report_update_server_log(penv, puser, return_code, &update);

		if(STATUS_SUCCESS == return_code) {
			package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);
		} else {
			package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		}

		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

	return return_code;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_ADD_TEXT_TITLE : 添加字幕 */

int32_t add_text_title_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
                                        roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[add_text_title_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;
	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[add_text_title_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	proom_node = get_children_node(pnode, MSG_ADDTITLE_REQ_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[add_text_title_resolve_recv_buf], roomid not found!\n");
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[add_text_title_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				continue;
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
		}
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t add_text_title_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[add_text_title_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_ADDTITLE_REQ_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);
	timeout_add_con_user_addr(&penv->pserset->timeque, puser->platform, puser->index, puser->ipaddr);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[add_text_title_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[add_text_title_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[add_text_title_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

			/*添加字幕日志上报*/
			//			report_add_text_title_log(penv, puser, send_buf, puser->platform);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

		/*添加字幕日志上报*/
		//		report_add_text_title_log(penv, puser, send_buf, puser->platform);

	}

EXIT:

	return return_code;
}

int32_t add_text_title_resp_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                    int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = -1;
	int32_t userid = 0;
	int32_t index = 0;
	int32_t ret = 0;
	platform_em platform;
	con_user *pforobj = NULL;
	int8_t passkey[64] = {0};
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[add_text_title_resp_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	get_passkey_and_userid_from_resp_xml_buf(recv_buf + headlen, passkey, &userid);
	platform = get_platform_from_passkey(passkey);		/* 转发的目标平台 */

	user_index = timeout_que_find_user(&penv->pserset->timeque, platform, msgcode, userid);

	if(user_index < 0) {
		/* FIXME:不再返回吗? */
		goto EXIT;
	}

	/* FIXME:先删除超时节点还是先进行返回处理? */
	timeout_que_del_user(&penv->pserset->timeque, user_index);

	r_memset(passkey, 0, 64);
	get_passkey_from_platform(puser->platform, passkey);
	modify_resp_xml_data_passkey(recv_buf + headlen, passkey);

	array.room_num = 1;
	array.roomid[0] = 0;

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, platform, array.roomid[index], userid);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[add_text_title_resp_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[add_text_title_resp_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[add_text_title_resp_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	/* FIXME: 如果有多个转发对象的时候? */

EXIT:

	return return_code;
}


int32_t add_text_title_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                               int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                               int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[add_text_title_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = add_text_title_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		/*添加字幕日志上报*/
		int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
		platform_em platform;

		get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
		platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */
		//		report_add_text_title_log(penv, puser, recv_buf, platform);

		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_GET_AUDIO_VOLUME : 获取音量值 */

int32_t get_audio_volume_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
        roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[get_audio_volume_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;
	parray->msgtype = XML_MSG_TYPE_REQ;
	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[get_audio_volume_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(r_strcmp((const int8_t *)pxml->proot->name, (const int8_t *)REQ_VALUE) == 0) {
		parray->msgtype = XML_MSG_TYPE_REQ;
	} else {
		parray->msgtype = XML_MSG_TYPE_RESP;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	proom_node = get_children_node(pnode, MSG_GET_VOLUME_REQ_KEY);

	/* 区分本消息为请求消息还是响应消息 */
	count = get_current_samename_node_nums(proom_node);
	zlog_debug(DBGLOG, " count = %d\n", count);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[get_audio_volume_resolve_recv_buf], roomid not found!\n");
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[get_audio_volume_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				continue;
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
		}
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t package_get_audio_volume_resp_xml_data(int8_t *out_buf, uint32_t avlume, int32_t roomid, int32_t msgcode)
{
	int32_t 	return_code = STATUS_FAILED;
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[16] = {0};
	int8_t		buf[16] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;
	xmlNodePtr	info_node = NULL;

	if(NULL == out_buf) {
		zlog_error(DBGLOG, "---[package_get_audio_volume_resp_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, RESP_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	sprintf((char *)msgcode_buf, "%d", msgcode);

	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);
	package_add_xml_leaf(head_node, MSG_RETURNCODE_KEY, (const int8_t *)XML_RETURNCODE_SUCCESS);
	package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, (const int8_t *)PLATFORM_RECSERVER);

	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);
	r_memset(buf, 0, 16);
	sprintf((char *)buf, "%d", roomid);
	package_add_xml_leaf(body_node, MSG_ROOMID_KEY, (int8_t *)buf);

	info_node = xmlNewNode(NULL, MSG_VOICEINFO_KEY);
	xmlAddChild(body_node, info_node);
	r_memset(buf, 0, 16);
	sprintf((char *)buf, "%d", avlume);
	package_add_xml_leaf(info_node, MSG_VOLUME_KEY, (int8_t *)buf);

	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	return return_code;;
}


int32_t get_audio_volume_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                     int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t index = 0;
	int32_t ret = 0;

	uint32_t avolume = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[get_audio_volume_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_GET_VOLUME_REQ_KEY, CONTROL_FALSE);

	if(array.room_num > 0) {
		if((array.roomid[0] >= 0) && (array.roomid[0] < CONTROL_ROOM_SERVER_MAX_USER)) {
			/* 保存当前音量值 */
			avolume = puser->art_volume[array.roomid[0]];

			/* 在转发前将音量值清0 */
			puser->art_volume[array.roomid[0]] = 0;
		}
	}

	package_get_audio_volume_resp_xml_data(send_buf + headlen, avolume, array.roomid[0], msgcode);
	return_code = send_user_xml_data(NULL, puser, send_buf, ret_buf, ret_len);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[get_audio_volume_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[get_audio_volume_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(NULL, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[get_audio_volume_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

EXIT:

	return return_code;
}

/* 确保recv_buf的长度要大于 CONTROL_DATA_LEN */
int32_t get_audio_volume_modify_resp_xml_data(int8_t *recv_buf, int8_t *passkey)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr passkey_node = NULL;
	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	if(NULL == recv_buf || NULL == passkey) {
		zlog_error(DBGLOG, "---[get_audio_volume_modify_resp_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[get_audio_volume_modify_resp_xml_data] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	passkey_node = get_children_node(pnode, MSG_PASSKEY_KEY);

	/* TODO:修改成功了吗 */
	modify_node_value(passkey_node, (const xmlChar *)passkey);

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	/* FIXME，确保recv_buf的长度要大于 CONTROL_DATA_LEN */
	r_memset(recv_buf, 0, CONTROL_DATA_LEN - headlen);

	if(xmllen > (CONTROL_DATA_LEN - headlen)) {
		xmllen = CONTROL_DATA_LEN - headlen;
	}

	r_memcpy(recv_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t get_audio_volume_resolve_resp_xml_buf(int8_t *xml_buf, int8_t *passkey,
        int32_t *userid, uint32_t *volume, int32_t *roomid)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr pbody_node = NULL;
	xmlNodePtr pvoi_node = NULL;
	xmlNodePtr pvolume_node = NULL;
	xmlNodePtr passkey_node = NULL;
	xmlNodePtr puserid_node = NULL;

	int8_t *pbuf = NULL;

	if(NULL == xml_buf || NULL == passkey || NULL == userid || NULL == volume || NULL == roomid) {
		zlog_error(DBGLOG, "---[get_passkey_and_volume_from_resp_xml_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf) {
		zlog_error(DBGLOG, "---[get_passkey_and_volume_from_resp_xml_buf] failed, malloc error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml, (const char *)xml_buf)) {
		zlog_error(DBGLOG, "---[get_passkey_and_volume_from_resp_xml_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_HEAD_KEY);
	passkey_node = get_children_node(pnode, MSG_PASSKEY_KEY);

	ret = get_current_node_value((char *)passkey, 64, pxml->pdoc, passkey_node);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_passkey_and_volume_from_resp_xml_buf], passkey not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbody_node = get_children_node(pxml->proot, MSG_BODY_KEY);

	pnode = get_children_node(pbody_node, MSG_ROOMID_KEY);
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_passkey_and_volume_from_resp_xml_buf], roomid not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	} else {
		*roomid = atoi((const char *)pbuf);
	}

	pvoi_node = get_children_node(pbody_node, MSG_VOICEINFO_KEY);
	pvolume_node = get_children_node(pvoi_node, MSG_VOLUME_KEY);
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pvolume_node);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[get_passkey_and_volume_from_resp_xml_buf], volume not found!\n");
		return_code = STATUS_FAILED;
		*volume = 0;
	} else {
		*volume = atoi((const char *)pbuf);
	}

	puserid_node = get_children_node(pxml->proot, MSG_USERID_KEY);
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, puserid_node);

	if(ret < 0) {
		//		zlog_error(DBGLOG, "---[get_passkey_and_volume_from_resp_xml_buf], userid not found!\n");
		return_code = STATUS_FAILED;
		*userid = 0;
	} else {
		*userid = atoi((const char *)pbuf);
	}

	return_code = STATUS_SUCCESS;

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}


int32_t get_audio_volume_resp_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                      int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t userid = 0;
	int32_t roomid = 0;
	uint32_t avolume = 0;
	platform_em platform;
	con_user *pforobj = NULL;
	int8_t passkey[64] = {0};

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[common_resp_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	get_audio_volume_resolve_resp_xml_buf(recv_buf + headlen, passkey, &userid, &avolume, &roomid);
	platform = get_platform_from_passkey(passkey);		/* 转发的目标平台 */

	/* 查找保存对象 */
	pforobj = find_forward_obj(penv->pserset, platform, 0, userid);

	if(NULL == pforobj) {
		zlog_error(DBGLOG, "---[common_resp_process] failed, [find_forward_obj] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == pforobj->pcmd.forward_process) {
		zlog_error(DBGLOG, "---[common_resp_process] failed, [pcmd.forward_process] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(roomid >= 0 && roomid < CONTROL_ROOM_SERVER_MAX_USER) {
		pforobj->art_volume[roomid] = avolume;
	}

	return_code = STATUS_SUCCESS;

EXIT:

	return return_code;
}


int32_t get_audio_volume_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                 int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                 int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[get_audio_volume_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = get_audio_volume_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = get_audio_volume_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_SET_AUDIO_MUTE : 设置静音 */

int32_t set_audio_mute_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
                                        roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[set_audio_mute_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;
	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[set_audio_mute_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	proom_node = get_children_node(pnode, MSG_AUDIO_MUTE_REQ_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[set_audio_mute_resolve_recv_buf], roomid not found!\n");
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[set_audio_mute_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				continue;
			}

			//		zlog_debug(OPELOG, "roomid = %d\n", roomid);

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
		}
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return STATUS_SUCCESS;
}

int32_t set_audio_mute_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[set_audio_mute_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_AUDIO_MUTE_REQ_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);
	timeout_add_con_user_addr(&penv->pserset->timeque, puser->platform, puser->index, puser->ipaddr);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[set_audio_mute_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[set_audio_mute_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[set_audio_mute_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

			/*静音日志上报*/
			//			report_audio_mute_log(penv, puser, send_buf, puser->platform);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

		/*静音日志上报*/
		//		report_audio_mute_log(penv, puser, send_buf, puser->platform);
	}

EXIT:

	return return_code;
}


int32_t set_audio_mute_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                               int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                               int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[set_audio_mute_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = set_audio_mute_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		/*静音日志上报*/
		int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
		platform_em platform;

		get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
		platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */
		//		report_audio_mute_log(penv, puser, recv_buf, platform);

		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);

	}

EXIT:

	return return_code;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_VIDEO_ADJUST : 视频微调 */

int32_t video_adjust_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
                                      roomid_array *parray)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray) {
		zlog_error(DBGLOG, "---[video_adjust_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;
	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[video_adjust_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	proom_node = get_children_node(pxml->proot, MSG_BODY_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			/* TODO:获取下一节点 */
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[video_adjust_resolve_recv_buf], roomid not found!\n");
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[video_adjust_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				continue;
			}

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
		}
	}

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t video_adjust_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                 int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[video_adjust_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	video_adjust_resolve_recv_buf(penv, recv_buf + headlen, &array);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);
	timeout_add_con_user_addr(&penv->pserset->timeque, puser->platform, puser->index, puser->ipaddr);

	add_userid_to_xml_data(recv_buf + headlen, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[video_adjust_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[video_adjust_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[video_adjust_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

			/*图像微调日志上报*/
			//			report_video_adjust_log(penv, puser, recv_buf, puser->platform);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

		/*图像微调日志上报*/
		//		report_video_adjust_log(penv, puser, recv_buf, puser->platform);
	}

EXIT:

	return return_code;
}


int32_t video_adjust_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                             int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                             int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[video_adjust_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = video_adjust_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		/*图像微调日志上报*/
		int8_t pass_key[XML_VALUE_MAX_LENGTH] = {0};
		platform_em platform;

		get_pass_key_from_resp(recv_buf + CONTROL_MSGHEAD_LEN, pass_key);
		platform = get_platform_from_passkey(pass_key);		/* 转发的目标平台 */
		//		report_video_adjust_log(penv, puser, recv_buf, platform);

		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_FILE_DELETE_REPORT : 文件删除通知 */

int32_t file_delete_report_req_process(control_env *pcon, int8_t *recv_buf,
                                       int8_t *send_buf, int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = -1;
	int32_t cmdret = -1;
	parse_xml_t *pxml_recv = NULL;
	parse_xml_t *pxml_send = NULL;
	xmlNodePtr pnode_recv = NULL;
	xmlNodePtr pnode_send = NULL;
	xmlNodePtr pbodynode_send = NULL;
	xmlNodePtr psucc_node = NULL;
	xmlNodePtr pfail_node = NULL;
	xmlNodePtr proom_node = NULL;

	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	int8_t *file = NULL;
	int8_t *cmd = NULL;
	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == send_buf) {
		zlog_error(DBGLOG, "---[file_delete_report_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pxml_recv = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml_recv, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[file_delete_report_req_process] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	file = (int8_t *)r_malloc(RECORD_FILE_MAX_FILENAME);
	cmd = (int8_t *)r_malloc(RECORD_FILE_MAX_FILENAME + 20);

	if(NULL == file || NULL == cmd || NULL == pbuf) {
		zlog_error(DBGLOG, "---[file_delete_report_req_process], malloc failed!\n");
		goto EXIT;
	}

	package_resp_xml_data(send_buf, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);

	pxml_send = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml_send, (const char *)send_buf)) {
		zlog_error(DBGLOG, "---[file_delete_report_req_process] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode_send = get_children_node(pxml_send->proot, MSG_BODY_KEY);
	pnode_send = xmlNewNode(NULL, MSG_FILE_DELETE_RESP_KEY);
	xmlAddChild(pbodynode_send, pnode_send);

	psucc_node = xmlNewNode(NULL, MSG_FILE_DELETE_SUCCESS_KEY);
	xmlAddChild(pnode_send, psucc_node);

	pfail_node = xmlNewNode(NULL, MSG_FILE_DELETE_FAIL_KEY);
	xmlAddChild(pnode_send, pfail_node);


	pnode_recv = get_children_node(pxml_recv->proot, MSG_BODY_KEY);
	proom_node = get_children_node(pnode_recv, MSG_FILE_DELETE_REQ_KEY);
	pnode_recv = get_children_node(proom_node, MSG_FILE_KEY);

	/* 删除文件 */
	while(pnode_recv) {
		return_code = STATUS_FAILED;
		r_memset(file, 0, RECORD_FILE_MAX_FILENAME);
		ret = get_current_node_value((char *)file, RECORD_FILE_MAX_FILENAME, pxml_recv->pdoc, pnode_recv);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[file_delete_report_req_process], file not found!\n");
		} else {
			r_memset(cmd, 0, RECORD_FILE_MAX_FILENAME + 20);

			if(r_strcmp(file, (const int8_t *)"/") != 0) {
				sprintf((char *)cmd, "rm /opt/Rec/%s -rf", file);
				zlog_debug(OPELOG, "cmd = %s\n", cmd);
#ifndef	KEEP_COURSE_FILE
				cmdret = r_system((const int8_t *)cmd);
#endif	//DELETE_COURSE_FILE
				if(0 == cmdret) {
					/* 删除成功 */
					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					package_add_xml_leaf(psucc_node, MSG_FILE_KEY, file);
					return_code = STATUS_SUCCESS;
				} else {
					/* 删除失败 */
					r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
					package_add_xml_leaf(pfail_node, MSG_FILE_KEY, file);
					return_code = STATUS_FAILED;
				}

			}
		}

		pnode_recv = find_next_node(pnode_recv, MSG_FILE_KEY);
	}


	if(STATUS_SUCCESS == return_code) {
		package_add_xml_leaf(pnode_send, MSG_RESULT_KEY, (const int8_t *)XML_RETURNCODE_SUCCESS);
	} else {
		package_add_xml_leaf(pnode_send, MSG_RESULT_KEY, (const int8_t *)XML_RETURNCODE_FAILED);
	}

	if(STATUS_SUCCESS == return_code) {
		pthread_mutex_lock(&(pcon->pserset->pserinfo->ftp_file_m));
		modify_ftp_fail_file_node(&(pcon->pserset->pserinfo->parse_xml_ftp), file, (int8_t *)"", MODIFY_TYPE_DELETE_NODE);
		pthread_mutex_unlock(&(pcon->pserset->pserinfo->ftp_file_m));
	}

	xmlDocDumpFormatMemoryEnc(pxml_send->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

EXIT:

	if(pxml_recv != NULL) {
		if(pxml_recv->pdoc != NULL) {
			release_dom_tree(pxml_recv->pdoc);
		}
	}

	if(pxml_send != NULL) {
		if(pxml_send->pdoc != NULL) {
			release_dom_tree(pxml_send->pdoc);
		}
	}

	if(pxml_recv) {
		r_free(pxml_recv);
	}

	if(file) {
		r_free(file);
	}

	if(cmd) {
		r_free(cmd);
	}

	return return_code;
}


int32_t file_delete_report_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                   int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[file_delete_report_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	return_code = file_delete_report_req_process(penv, recv_buf + headlen, send_buf + headlen, msgcode);
	return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

EXIT:

	return return_code;
}

int32_t CheckFtpUploadingFile(int8_t *ftp_check_file)
{
	int32_t		i = 0;
	int32_t		ftp_file_num = 0;
	int32_t 	xml_Len 	= 0;
	xmlChar		*xmlbuf	= NULL;
	int32_t 	return_code 		= STATUS_FAILED;
	parse_xml_t *parse_xml_cfg		= NULL;
	xmlNodePtr	ftp_files_list_node = NULL;
	xmlNodePtr	tmp_node			= NULL;
	int8_t 		tmp_buf[XML_VALUE_MAX_LENGTH] = {0};

	if(NULL == ftp_check_file) {
		zlog_error(OPELOG, "---[CheckFtpUploadingFile] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	parse_xml_cfg = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_file_dom_tree(parse_xml_cfg, (char *)"UploadConfig.xml")) {
		zlog_error(OPELOG, "---[CheckFtpUploadingFile] init_file_dom_tree failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	ftp_files_list_node = parse_xml_cfg->proot;

	if(r_strcmp((int8_t *)ftp_files_list_node->name, (int8_t *)"FtpFileSata")) {
		zlog_error(OPELOG, "---[CheckFtpUploadingFile] proot name isn't [FtpFilesList]!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	tmp_node = get_children_node(ftp_files_list_node, (xmlChar *)"WaitUpLoadFile");

	if(!tmp_node) {
		zlog_error(OPELOG, "---[CheckFtpUploadingFile] proot name isn't [WaitUpLoadFile]!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	tmp_node = get_children_node(tmp_node, (xmlChar *)"FileStat");

	if(!tmp_node) {
		zlog_error(OPELOG, "---[CheckFtpUploadingFile] proot name isn't [FileStat]!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	ftp_file_num = get_current_samename_node_nums(tmp_node);

	for(i = 0; i < ftp_file_num && tmp_node != NULL; i++) {
		r_bzero(tmp_buf, XML_VALUE_MAX_LENGTH);
		return_code = get_leaf_value((int8_t *)tmp_buf, XML_VALUE_MAX_LENGTH, (xmlChar *)"LocalFilePath", tmp_node, parse_xml_cfg->pdoc);

		if(return_code == 0) {
			zlog_error(OPELOG, "---[CheckFtpUploadingFile] proot name isn't [get_leaf_value]!, i = %d, ftp_file_num = %d\n", i, ftp_file_num);
			return_code = STATUS_FAILED;
			goto EXIT;
		}

		if(!r_strcmp(ftp_check_file, tmp_buf)) {
			return_code = STATUS_SUCCESS;
			break;
		}

		tmp_node = find_next_node(tmp_node, (xmlChar *)"FileStat");
	}

	xmlDocDumpFormatMemoryEnc(parse_xml_cfg->pdoc, &xmlbuf, &xml_Len, XML_TEXT_CODE_TYPE, 1);
	xmlFree(xmlbuf);

	if(i < ftp_file_num) {
		return_code = STATUS_SUCCESS;
	} else {
		return_code = STATUS_FAILED;
	}

EXIT:

	if(parse_xml_cfg->pdoc != NULL) {
		release_dom_tree(parse_xml_cfg->pdoc);
	}

	if(parse_xml_cfg) {
		r_free(parse_xml_cfg);
	}

	return return_code;;
}

int32_t GetFtpUploadingFile(int8_t *ftp_upload_file, int32_t file_name_len)
{
	int32_t 	xml_Len 	= 0;
	xmlChar		*xmlbuf	= NULL;
	int32_t 	return_code 		= STATUS_FAILED;
	parse_xml_t *parse_xml_cfg		= NULL;
	xmlNodePtr	ftp_files_list_node = NULL;
	xmlNodePtr	tmp_node			= NULL;

	if(NULL == ftp_upload_file) {
		zlog_error(OPELOG, "---[GetFtpUploadingFile] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	parse_xml_cfg = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_file_dom_tree(parse_xml_cfg, (char *)"UploadConfig.xml")) {
		zlog_error(OPELOG, "---[GetFtpUploadingFile] init_file_dom_tree failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	ftp_files_list_node = parse_xml_cfg->proot;

	if(r_strcmp((int8_t *)ftp_files_list_node->name, (int8_t *)"FtpFileSata")) {
		zlog_error(OPELOG, "---[GetFtpUploadingFile] proot name isn't [FtpFilesList]!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	tmp_node = get_children_node(ftp_files_list_node, (xmlChar *)"WaitUpLoadFile");

	if(!tmp_node) {
		zlog_error(OPELOG, "---[GetFtpUploadingFile] proot name isn't [WaitUpLoadFile]!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	tmp_node = get_children_node(tmp_node, (xmlChar *)"FileStat");

	if(!tmp_node) {
		zlog_error(OPELOG, "---[GetFtpUploadingFile] proot name isn't [FileStat]!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	tmp_node = get_children_node(tmp_node, (xmlChar *)"LocalFilePath");

	if(!tmp_node) {
		zlog_error(OPELOG, "---[GetFtpUploadingFile] proot name isn't [LocalFilePath]!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = get_current_node_value((char *)ftp_upload_file, file_name_len, parse_xml_cfg->pdoc, tmp_node);

	if(return_code == -1) {
		zlog_error(OPELOG, "---[GetFtpUploadingFile] proot name isn't [get_current_node_value]!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	xmlDocDumpFormatMemoryEnc(parse_xml_cfg->pdoc, &xmlbuf, &xml_Len, XML_TEXT_CODE_TYPE, 1);
	xmlFree(xmlbuf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(parse_xml_cfg->pdoc != NULL) {
		release_dom_tree(parse_xml_cfg->pdoc);
	}

	if(parse_xml_cfg) {
		r_free(parse_xml_cfg);
	}

	return return_code;;
}

static uint32_t getsize_dir(int8_t *SrcPath)
{
	if((NULL == SrcPath)) {
		return -1;
	}

	DIR              *pDir ;
	struct dirent    *ent;
	char    Srcchildpath[1024];
	uint32_t     ret = 0;
	uint32_t totalsize = 0;

	pDir = opendir((const char *)SrcPath);

	if(NULL == pDir) {
		printf("Cannot open directory:[ %s ]\n", SrcPath);
		return 0;
	}

	while((ent = readdir(pDir)) != NULL) {
		if(ent->d_type & DT_DIR) {
			struct stat stat_buf;

			if(r_strcmp((const int8_t *)ent->d_name, (const int8_t *)".") == 0 || r_strcmp((const int8_t *)ent->d_name, (const int8_t *)"..") == 0) {
				continue;
			}

			sprintf(Srcchildpath, "%s/%s", SrcPath, ent->d_name);

			lstat(Srcchildpath, &stat_buf);

			//		printf("%s %u\n",Srcchildpath,(int)stat_buf.st_size);
			totalsize = totalsize + stat_buf.st_size / 1024;
			ret = getsize_dir((int8_t *)Srcchildpath);

			totalsize = totalsize + ret;
		} else {

			int8_t LocalFilePath[1024] = {0};
			struct stat stat_buf;
			sprintf((char *)LocalFilePath, "%s/", SrcPath);

			r_strncat(LocalFilePath, (const int8_t *)ent->d_name, (size_t)r_strlen((const int8_t *)ent->d_name));
			lstat((const char *)LocalFilePath, &stat_buf);
			totalsize = totalsize + stat_buf.st_size / 1024;

			//printf("%s %u\n",LocalFilePath,stat_buf.st_size);
		}
	}

	if(0 != closedir(pDir)) {
		//	assert(0);
	}

	return totalsize;
}


int32_t GetFileList(xmlNodePtr node, int8_t *RepairPath, int32_t totalnum)
{
	if(NULL == RepairPath) {
		return -1;
	}

	DIR              *pDir ;
	struct dirent    *ent;
	int32_t      num = 0;
	int8_t       childpath[RECORD_FILE_MAX_FILENAME * 2];
	int8_t       ftp_upload_file[RECORD_FILE_MAX_FILENAME] = {0};
	int32_t      get_ftp_upload_flag = STATUS_FAILED;
	xmlNodePtr	file_node = NULL;

	pDir = opendir((char *)RepairPath);

	if(NULL == pDir) {
		printf("Cannot open directory: %s\n", RepairPath);
		return -1;
	}

	get_ftp_upload_flag = GetFtpUploadingFile(ftp_upload_file, RECORD_FILE_MAX_FILENAME);
	zlog_error(OPELOG, "---[GetFtpUploadingFile] [%d:%s]!\n", get_ftp_upload_flag, ftp_upload_file);

	while((ent = readdir(pDir)) != NULL) {
		if(ent->d_type & DT_DIR) {
			int8_t md5info[RECORD_FILE_MAX_FILENAME * 2] = {0};

			if((r_strcmp((int8_t *)ent->d_name, (int8_t *)".") == 0) || (r_strcmp((int8_t *)ent->d_name, (int8_t *)"..") == 0)\
			   || (0 == r_strcmp((int8_t *)ent->d_name, (int8_t *)MP4_REPAIR_PATH))) {
				continue;
			}

			if(get_ftp_upload_flag == STATUS_SUCCESS)
				if(r_strstr(ftp_upload_file, (int8_t *)ent->d_name)) {
					continue;
				}

			sprintf((char *)childpath, "%s/%s", (char *)RepairPath, (char *)ent->d_name);
			sprintf((char *)md5info, "%s/md5.info", childpath);

			if(0 == access((char *)md5info, 0)) {
				unsigned int dirsize = 0;
				char cmd[256] = {0};
				dirsize   = getsize_dir(childpath);
				sprintf(cmd, "%u", dirsize);

				if(dirsize < 0) {
					dirsize = 0;
				}

				file_node = xmlNewNode(NULL, MSG_FILE_KEY);
				xmlAddChild(node, file_node);
				package_add_xml_leaf(file_node, MSG_FILENAME_KEY, &childpath[r_strlen((int8_t *)MP4_RECORD_PATH) + 1]);
				package_add_xml_leaf(file_node, MSG_FILESIZE_KEY, (const int8_t *)cmd);

				num++;

				if(num >= totalnum) {
					break;
				}
			}

		} else {
			continue;
		}
	}

	if(0 != closedir(pDir)) {
	}

	return num;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_GET_FILELIST_INFO : 获取文件列表信息 */

int32_t package_get_filelist_info_resp_xml_data(int8_t **out_buf, int32_t msgcode, int8_t *ret_code)
{
	int32_t 	return_code = STATUS_FAILED;
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[16] = {0};
	int8_t		*pbuf = NULL;
	int32_t 	headlen = CONTROL_MSGHEAD_LEN;

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;
	xmlNodePtr	file_info_node = NULL;

	if(NULL == ret_code) {
		zlog_error(DBGLOG, "---[package_get_filelist_info_resp_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf) {
		zlog_error(DBGLOG, "---[package_get_filelist_info_resp_xml_data] failed, malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, RESP_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	sprintf((char *)msgcode_buf, "%d", msgcode);

	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);
	package_add_xml_leaf(head_node, MSG_RETURNCODE_KEY, ret_code);
	package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, (const int8_t *)PLATFORM_RECSERVER);

	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	file_info_node = xmlNewNode(NULL, MSG_FILELIST_INFO_KEY);
	xmlAddChild(body_node, file_info_node);

	/* TODO:添加实际的读目录文件操作 */
	/* FIXME:注意XML文件大小 */

	GetFileList(file_info_node, (int8_t *)"/opt/Rec", GET_FILELIST_MAX_NUM);

	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);

	if(xmllen <= 0) {
		zlog_error(DBGLOG, "---[package_get_filelist_info_resp_xml_data] failed, xmllen error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	//	printf("%s\n",xml_buf);
	*out_buf = r_malloc(xmllen + headlen + 16);
	r_memset(*out_buf, 0, xmllen + headlen + 16);

	r_memcpy(*out_buf + headlen, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	return return_code;;
}


int32_t get_filelist_info_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                  int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                  int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int8_t *n_send_buf = NULL;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[get_filelist_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	/* FIXME:如果有文件删除失败? */
	/* TOTO: 检查 Result 值 */
	package_get_filelist_info_resp_xml_data(&n_send_buf, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);

	if(n_send_buf != NULL) {
		return_code = send_user_xml_data(puser, puser, n_send_buf, ret_buf, ret_len);
		usleep(5000);
		r_free(n_send_buf);
	}

EXIT:

	return return_code;
}


/********************************************************************************************/

/********************************************************************************************/
/*  */
/********************************************************************************************/

/********************************************************************************************/
/*  */
/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_ROOM_REPORT_RECORD_STATUS : 上报录制状态(传上报给媒体中心或管理平台，广播) */

int32_t report_record_status_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
        record_status *pres)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int8_t *pbuf = NULL;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == pres) {
		zlog_error(DBGLOG, "---[report_record_status_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[report_record_status_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	proom_node = get_children_node(pnode, MSG_RECORD_REPORT_KEY);

	/* 操作类型 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_OPT_TYPE_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[report_record_status_resolve_recv_buf], opt type not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pres->OptType = atoi((const char *)pbuf);

	/* 操作结果 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_RESULT_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[report_record_status_resolve_recv_buf], record result not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pres->Result = atoi((const char *)pbuf);

	/* 文件名 */
	r_memset(pres->FileName, 0, RECORD_FILE_MAX_FILENAME);
	pnode = get_children_node(proom_node, MSG_FILENAME_KEY);
	ret = get_current_node_value((char *)pres->FileName, RECORD_FILE_MAX_FILENAME, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[report_record_status_resolve_recv_buf], %s not found!\n",
		           MSG_FILENAME_KEY);
	}

	/* 录制ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_RECORDID_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[report_record_status_resolve_recv_buf], %s not found!\n",
		           MSG_RECORDID_KEY);
	}

	r_memset(pres->RecordID, 0, RECORD_ID_MAX_LENGTH);
	r_memcpy(pres->RecordID, pbuf, RECORD_ID_MAX_LENGTH - 2);


	/* 平台类型，若为空，则表示为会议室主动上报的消息 */
	/* passkey */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_PASSKEY_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[report_record_status_resolve_recv_buf], %s not found!\n",
		           MSG_PASSKEY_KEY);
		//		return_code = STATUS_FAILED;
		//		goto EXIT;
	}

	if(r_strlen(pbuf) != 0) {
		pres->platform = get_platform_from_passkey(pbuf);
	} else {
		pres->platform = AllPlatform;
	}

	/* 录制状态 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_RECORD_STATUS_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[report_record_status_resolve_recv_buf], %s not found!\n",
		           MSG_RECORD_STATUS_KEY);
	} else {
		pres->RecStatus = atoi((const char *)pbuf);
	}

	/* userid */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(pxml->proot, MSG_USERID_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[report_record_status_resolve_recv_buf], %s not found!\n",
		           MSG_USERID_KEY);
	} else {
		pres->userid = atoi((const char *)pbuf);
	}

	return_code = STATUS_SUCCESS;

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pxml) {
		r_free(pxml);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	pxml = NULL;

	return return_code;
}

static int32_t package_report_record_status_xml_data(int8_t *send_buf,
        record_status *pres, int32_t msgcode,
        int32_t *ret_type)
{
	int32_t return_code = STATUS_FAILED;
	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr  pbodynode = NULL;
	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	int32_t filelen = 0;
	int8_t 	*ptmp 	= NULL;

	int8_t *pbuf = NULL;

	if(NULL == send_buf || NULL == pres || NULL == ret_type) {
		zlog_error(DBGLOG, "---[package_report_record_status_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	if(pres->platform == AllPlatform) {
		/* 广播操作 */
		*ret_type = OPERATION_BROADCASE;
	} else {
		/* 返回操作 */
		*ret_type = OPERATION_ONLY_RETURN;

		if((pres->Result == 1) || (pres->Result == 2))
			package_resp_xml_data(send_buf, -1, MSGCODE_RECORD_CONTROL,
			                      (int8_t *)XML_RETURNCODE_SUCCESS);
		else
			package_resp_xml_data(send_buf, -1, MSGCODE_RECORD_CONTROL,
			                      (int8_t *)XML_RETURNCODE_FAILED);

		pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
		pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

		if(NULL == pbuf || NULL == pxml) {
			zlog_error(DBGLOG, "---[package_report_record_status_xml_data] failed, r_malloc failed!\n");
			return_code = STATUS_FAILED;
			goto EXIT;
		}

		if(NULL == init_dom_tree(pxml, (const char *)send_buf)) {
			zlog_error(DBGLOG, "---[package_report_record_status_xml_data] failed, [init_dom_tree] error!\n");
			return_code = STATUS_FAILED;
			goto EXIT;
		}

		pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
		pnode = xmlNewNode(NULL, MSG_RECCTRL_RESP_KEY);
		xmlAddChild(pbodynode, pnode);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", pres->RoomID);
		package_add_xml_leaf(pnode, MSG_ROOMID_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);

		if(2 == pres->Result) {
			sprintf((char *)pbuf, "%s", (const char *)"1");
		} else {
			sprintf((char *)pbuf, "%d", pres->Result);
		}

		package_add_xml_leaf(pnode, MSG_RESULT_KEY, pbuf);

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%d", pres->RecStatus);
		package_add_xml_leaf(pnode, MSG_RECORD_STATUS_KEY, pbuf);

		filelen = r_strlen(pres->FileName);

		if(filelen > 0) {
			ptmp = (int8_t *)pres->FileName + r_strlen(pres->FileName) - 1;

			if(*ptmp == '/') {
				*ptmp = '\0';
			}

			ptmp = r_strrchr(pres->FileName, '/');
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);

			if(ptmp) {
				sprintf((char *)pbuf, "%s", ptmp + 1);
			} else {
				sprintf((char *)pbuf, "%s", pres->FileName);
			}

			package_add_xml_leaf(pnode, MSG_FILENAME_KEY, pbuf);
		}

		if(r_strlen(pres->RecordID)) {
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			sprintf((char *)pbuf, "%s", pres->RecordID);
			package_add_xml_leaf(pnode, MSG_RECORDID_KEY, pbuf);
		}
	}

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}

int32_t package_ftpinfo_http_req_xml_data(int8_t *send_buf, record_status *pres)
{
	int32_t return_code = STATUS_FAILED;

	int32_t xmllen = 0;
	parse_xml_t *pxml = NULL;
	xmlChar *xmlbuf = NULL;

	xmlNodePtr pnode = NULL;
	xmlNodePtr  pbodynode = NULL;

	int8_t *pbuf = NULL;
	int8_t *filename = NULL;

	if(NULL == pres || NULL == send_buf) {
		zlog_error(DBGLOG, "---[package_ftpinfo_http_req_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = package_req_xml_data(send_buf, RecServer, MSGCODE_HTTP_FTPINFO_REQ, (int8_t *)XML_RETURNCODE_SUCCESS);

	if(return_code != STATUS_SUCCESS) {
		zlog_error(DBGLOG, "---[package_ftpinfo_http_req_xml_data] failed, package_req_xml_data error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[package_ftpinfo_http_req_xml_data] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)send_buf)) {
		zlog_error(DBGLOG, "---[package_ftpinfo_http_req_xml_data] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pnode = xmlNewNode(NULL, MSG_FILE_UPLOAD_PREREQ_KEY);
	xmlAddChild(pbodynode, pnode);

	pres->RecordID[RECORD_ID_MAX_LENGTH - 1] = '\0';
	package_add_xml_leaf(pnode, MSG_RECORDID_KEY, pres->RecordID);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	filename = r_strrchr(pres->FileName, '/');

	if(filename != NULL && (r_strlen(filename) > 1)) {
		filename++;
		sprintf((char *)pbuf, "%s", filename);
		package_add_xml_leaf(pnode, MSG_FILE_KEY, pbuf);
	} else {
		sprintf((char *)pbuf, "%s", (const char *)XML_NULL_VALUE);
		package_add_xml_leaf(pnode, MSG_FILE_KEY, pbuf);
	}

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xmlbuf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xmlbuf, xmllen);
	xmlFree(xmlbuf);

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;


	return return_code;
}


int32_t report_record_status_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                     int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                     int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t ret_type = OPERATION_ONLY_RETURN;
	int32_t user_index = 0;
	int32_t roomid = 0;
	int32_t mc_userid = 0;
	int32_t count = 0;
	int32_t ret = 0;
	int32_t last_rec_status = 0;
	uint8_t *pfile = NULL;
	int8_t *premote_addr = NULL;
	int8_t pport[16] = {0};
	record_status *pres = NULL;
	server_set *pser = NULL;
	con_user *pforobj = NULL;
	ftpcom_env *pftp = NULL;
	all_server_info *pserinfo = NULL;
	ftp_info *pftp_info = NULL;

	room_info *proom = NULL;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[report_record_status_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	/* FIXME: 检查此平台是否是会议室 */

	pftp = &penv->pserset->ftpser;
	pres = (record_status *)r_malloc(sizeof(record_status));
	pser = penv->pserset;

	return_code = report_record_status_resolve_recv_buf(penv, recv_buf + headlen, pres);
	zlog_debug(OPELOG, "001--report_record_status_process, return_code = %d\n", return_code);

	if(STATUS_SUCCESS == return_code) {
		/*录制操作日志上报*/
		//		report_record_status_log(penv, puser, pres, recv_buf);

		/* 返回或广播 录制状态 */
		r_memset(send_buf, 0, CONTROL_DATA_LEN);

		if(puser->platform != Mp4Repair) {
			pres->RoomID = penv->index;

			/* 更新本地录制状态信息，仅保存内存中的录制状态 */
			if(pres->RoomID >= 0 && pres->RoomID < pser->pserinfo->ServerInfo.MaxRoom) {
				pthread_mutex_lock(&pser->pserinfo->info_m);
				proom = &pser->pserinfo->RoomInfo[pres->RoomID];

				if(proom != NULL) {
					/* 临时保存之前的录制状态 */
					last_rec_status = proom->RecStatus;

					if((1 == pres->OptType) && (1 == pres->Result)) {
						/* 此为开始录制且操作成功的响应消息，更新录制状态 */
						proom->RecStatus = 1;
					} else if((0 == pres->OptType) && (1 == pres->Result)) {
						/* 此为停止录制且操作成功的响应消息，更新录制状态 */
						proom->RecStatus = 0;
					}
				}

				pthread_mutex_unlock(&pser->pserinfo->info_m);
			}
		}

		return_code = package_report_record_status_xml_data(send_buf + headlen, pres, msgcode, &ret_type);
		zlog_debug(OPELOG, "002--report_record_status_process, return_code = %d\n", return_code);

		if(STATUS_SUCCESS == return_code) {
			zlog_debug(OPELOG, "003--report_record_status_process, ret_type = %d\n", ret_type);

			if(OPERATION_BROADCASE == ret_type)
				/* TODO: 广播 */
			{

			} else if(OPERATION_ONLY_RETURN == ret_type)
				/* 返回 */
			{
				user_index = -1;

				if(RecServer == puser->platform) {
					roomid = puser->port - CONTROL_ROOM_SERVER_PORT_START;
					pserinfo = penv->pserset->pserinfo;

					if(NULL == pserinfo) {
						zlog_error(OPELOG, "---[report_record_status_process] failed, pserinfo error!\n");
						goto EXIT;
					}

					if(roomid < 0 || roomid >= pserinfo->ServerInfo.MaxRoom) {
						zlog_error(OPELOG, "---[report_record_status_process] failed, roomid error, roomid = %d\n", roomid);
						goto EXIT;
					}

					zlog_debug(OPELOG, "roomid = %d\n", roomid);

					//			user_index = timeout_que_find_user(&penv->pserset->timeque, pres->platform,
					//			                                   MSGCODE_RECORD_CONTROL, pres->userid);
					mc_userid = -1;
					user_index = timeout_que_find_user2(&penv->pserset->timeque, pres->platform,
					                                    MSGCODE_RECORD_CONTROL, pres->userid, roomid, &mc_userid);
					zlog_debug(OPELOG, "004--report_record_status_process, user_index = %d, mc_userid = %d\n",
					           user_index, mc_userid);

					if(user_index >= 0) {
						/* FIXME:先删除超时节点还是先进行返回处理? */
						timeout_que_del_user(&penv->pserset->timeque, user_index);

						/* TODO: 平台 */
						if(pres->platform == MediaCenter) {
							pforobj = find_forward_obj2(pser, pres->platform, mc_userid);
						} else {
							pforobj = find_forward_obj(pser, pres->platform, mc_userid, pres->userid);
						}

						if(pforobj != NULL) {
							if(NULL != pforobj->pcmd.forward_process) {
								ret = pforobj->pcmd.forward_process(puser, pforobj, send_buf,
								                                    MSGCODE_RECORD_CONTROL, ret_buf, ret_len);

								if(ret < 0) {
									zlog_error2(DBGLOG, "---[report_record_status_process] failed, forward_process return value error!\n");
								}
							}
						}
					}
				} else {
					user_index = timeout_que_find_user(&penv->pserset->timeque, pres->platform,
					                                   MSGCODE_RECORD_CONTROL, pres->userid);

					zlog_debug(OPELOG, "004--report_record_status_process, user_index = %d\n", user_index);

					if(user_index >= 0) {
						/* FIXME:先删除超时节点还是先进行返回处理? */
						timeout_que_del_user(&penv->pserset->timeque, user_index);
						/* TODO: 平台 */
						pforobj = find_forward_obj(pser, pres->platform, 0, pres->userid);

						if(pforobj != NULL) {
							if(NULL != pforobj->pcmd.forward_process) {
								ret = pforobj->pcmd.forward_process(puser, pforobj, send_buf,
								                                    MSGCODE_RECORD_CONTROL, ret_buf, ret_len);

								if(ret < 0) {
									zlog_error2(DBGLOG, "---[report_record_status_process] failed, forward_process return value error!\n");
								}
							}
						}
					}
				}


			}
		}

		zlog_debug2(OPELOG, "filename = %s\n", pres->FileName);
		zlog_debug2(OPELOG, "RecordID = %s\n", pres->RecordID);
		zlog_debug2(OPELOG, "OptType = %d\n", pres->OptType);
		zlog_debug2(OPELOG, "Result = %d\n", pres->Result);
		zlog_debug2(OPELOG, "last_rec_status = %d\n", last_rec_status);
		zlog_debug2(OPELOG, "puser->platform = %d\n", puser->platform);

		if(pres->OptType == 0 && pres->Result == 1)
			/* TODO:若为停止录制成功 */
		{
			if(pftp) {
				/* FIXME: 应做成队列形式存储文件信息 */
				if(pftp->pcmd.save_filename != NULL) {
					pftp->pcmd.save_filename(pftp, pres->FileName);
				}
			}

			/* 请求FTP服务器IP和端口及后续一系列上传文件的操作 */
			r_memset(send_buf, 0, CONTROL_DATA_LEN);
			return_code = package_ftpinfo_http_req_xml_data(send_buf + headlen, pres);

			if(STATUS_SUCCESS == return_code) {
				*ret_len = CONTROL_DATA_LEN;
				/* 请求FTP服务器IP和端口等信息 */
				zlog_error2(OPELOG, "--->>>>> %s\n", send_buf + headlen);

				count = 3;

				while(count--) {
					r_memset(ret_buf, 0, CONTROL_DATA_LEN);
					ret = send_http_user_xml_data(pser->pserinfo, send_buf, ret_buf, ret_len);

					if(ret == 0) {
						break;
					}

					zlog_error2(DBGLOG, "---[report_record_status_process] failed, request ftp info error, remine count = %d\n",
					            count);

					if(0 == count) {
						/* 3次请求失败，但仍需向FTP进程请求上传(MD5值计算) */
						//				return_code = STATUS_FAILED;
						break;
					}
				}

				/*获取FTP 上传信息日志上报*/
				//			report_get_ftp_info_log(penv, puser, recv_buf, ret);

				pftp_info = (ftp_info *)r_malloc(sizeof(ftp_info));

				if(NULL == pftp_info) {
					zlog_error2(DBGLOG, "---[report_record_status_process] failed, ftp info malloc error!\n");
					return_code = STATUS_FAILED;
					goto EXIT;
				}

				zlog_error2(OPELOG, "---<<<<<< %s\n", ret_buf);

				return_code = get_ftpupload_info_resolve_recv_buf(ret_buf, pftp_info);

				if(STATUS_FAILED == return_code) {
					/* 请求失败，但仍需向FTP进程请求上传(MD5值计算) */
					zlog_error2(DBGLOG, "---[report_record_status_process] failed, [get_ftpupload_info_resolve_recv_buf] error!\n");

					pserinfo = penv->pserset->pserinfo;

					if(pserinfo != NULL || pftp_info != NULL) {
						/* 使用保存的FTP服务器信息 */
						pthread_mutex_lock(&pserinfo->info_m);
						//	r_memcpy(pftp_info, &pserinfo->FtpInfo, sizeof(ftp_info));
						pftp_info->MCFTPPort = pserinfo->FtpInfo.MCFTPPort;
						r_memcpy(pftp_info->MCFTPAddr, pserinfo->FtpInfo.MCFTPAddr, 24);
						r_memcpy(pftp_info->MCFTPUserName, pserinfo->FtpInfo.MCFTPUserName, FTP_MAX_USERNAME_LENGTH);
						r_memcpy(pftp_info->MCFTPPassword, pserinfo->FtpInfo.MCFTPPassword, FTP_MAX_PASSWD_LENGTH);
						r_memcpy(pftp_info->MCFTPPath, pserinfo->FtpInfo.MCFTPPath, FTP_MAX_FTPPATH_LENGTH);
						pthread_mutex_unlock(&pserinfo->info_m);
					}
				} else {
					pserinfo = penv->pserset->pserinfo;

					if(pserinfo != NULL || pftp_info != NULL) {
						/* 保存请求到的FTP服务器信息 */
						pftp_info->MCFTPPort = 21;
						pthread_mutex_lock(&pserinfo->info_m);
						pftp_info->Mode = pserinfo->FtpInfo.Mode;
						pftp_info->THRFTPPort = pserinfo->FtpInfo.THRFTPPort;
						r_memcpy(pftp_info->THRFTPAddr, pserinfo->FtpInfo.THRFTPAddr, 24);
						r_memcpy(pftp_info->THRFTPUserName, pserinfo->FtpInfo.THRFTPUserName, FTP_MAX_USERNAME_LENGTH);
						r_memcpy(pftp_info->THRFTPPassword, pserinfo->FtpInfo.THRFTPPassword, FTP_MAX_PASSWD_LENGTH);
						r_memcpy(pftp_info->THRFTPPath, pserinfo->FtpInfo.THRFTPPath, FTP_MAX_FTPPATH_LENGTH);
						modify_ftp_info_only((const int8_t *)CONFIG_TABLE_FILE, &pserinfo->FtpInfo, pftp_info);
						r_memcpy(&pserinfo->FtpInfo, pftp_info, sizeof(ftp_info));
						pthread_mutex_unlock(&pserinfo->info_m);
					}
				}

				premote_addr = (int8_t *)r_malloc(FTP_COM_MAX_FILENAME_LENGTH);

				pftp = &penv->pserset->ftpser;

				if(pftp) {
					/* 发送用户名和密码 */
					if(NULL == pftp->pcmd.set_server_info) {
						zlog_error2(DBGLOG, "---[get_ftpupload_info_process] failed, pcmd.set_server_info error!\n");
						return_code = STATUS_FAILED;
						goto EXIT;
					}

					ret = pftp->pcmd.set_server_info((void *)pftp, (const int8_t *)pftp_info->MCFTPUserName,
					                                 (const int8_t *)pftp_info->MCFTPPassword);

					if(ret < 0) {
						zlog_error2(DBGLOG, "---[get_ftpupload_info_process] failed, ftpcom set_server_info error!\n");
						//				return_code = STATUS_FAILED;
						//				goto EXIT;
					}

					/* FIXME: 响应消息中没有标识FTP端口，只使用默认的端口? */

					if(pserinfo->FtpInfo.Mode == FTP_MODE_THIRDFTP) {
						sprintf((char *)pport, "%d", pftp_info->THRFTPPort);
					} else {
						pftp_info->MCFTPPort = 21;
						sprintf((char *)pport, "%d", pftp_info->MCFTPPort);
					}

					r_memset(premote_addr, 0, FTP_COM_MAX_FILENAME_LENGTH);
					/* FIXME: 媒体中心发过来的地址格式??? */
					//			sprintf((char *)premote_addr, "ftp://%s", pftp_info->MCFTPAddr);

					zlog_debug2(OPELOG, "upload: filename = %s\n", pres->FileName);

					if(NULL == pftp->pcmd.upload_file) {
						zlog_error2(DBGLOG, "---[get_ftpupload_info_process] failed, pcmd.upload_file error!\n");
						return_code = STATUS_FAILED;
						goto EXIT;
					}

					/* 上传文件 */
					if(pserinfo->FtpInfo.Mode == FTP_MODE_THIRDFTP) {
						ret = pftp->pcmd.upload_file(pftp, pres->FileName, (int8_t *)pftp_info->THRFTPAddr,
						                             (int8_t *)pftp_info->THRFTPPath, pport, pres->RecordID);
					} else {
						ret = pftp->pcmd.upload_file(pftp, pres->FileName, (int8_t *)pftp_info->MCFTPAddr,
						                             (int8_t *)pftp_info->MCFTPPath, pport, pres->RecordID);
					}

					if(ret < 0) {
						zlog_error2(DBGLOG, "---[get_ftpupload_info_process] failed, ftpcom set_server_info error!\n");
						return_code = STATUS_FAILED;
						goto EXIT;
					}

					/* 如果放不进去了???? */
				}
			}
		}

		puser->ack.msgcode = msgcode;
	}

	/* FIXME: 失败的情形? */

EXIT:

	/* 强制不返回消息给会议室 */

	if(pfile) {
		r_free(pfile);
	}

	if(pftp_info) {
		r_free(pftp_info);
	}

	if(premote_addr) {
		r_free(premote_addr);
	}

	if(pres) {
		r_free(pres);
	}

	return return_code;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_ROOM_GET_PREREC_INFO : 会议室获取预录制信息 */

static int32_t package_get_prerec_info_resp_data(int8_t *send_buf, server_info *pinfo)
{
	int32_t return_code = STATUS_FAILED;
	parse_xml_t *pxml = NULL;
	xmlNodePtr preqnode = NULL;
	xmlNodePtr pbodynode = NULL;
	int8_t *pbuf = NULL;

	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	if(NULL == send_buf) {
		zlog_error(DBGLOG, "---[package_get_prerec_info_resp_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));

	if(NULL == pbuf || NULL == pxml) {
		zlog_error(DBGLOG, "---[package_get_prerec_info_resp_data] failed, r_malloc failed!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == init_dom_tree(pxml, (const char *)send_buf)) {
		zlog_error(DBGLOG, "---[package_get_prerec_info_resp_data] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbodynode = get_children_node(pxml->proot, MSG_BODY_KEY);
	preqnode = xmlNewNode(NULL, MSG_ROOM_PREREC_INFO_REQ_KEY);
	xmlAddChild(pbodynode, preqnode);


	/* TODO:赋具体值 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%s", MP4_RECORD_PATH2);
	package_add_xml_leaf(preqnode, MSG_ROOM_RECPATH_KEY, pbuf);

	if(NULL == pinfo) {
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%s", "unknow device");
		package_add_xml_leaf(preqnode, MSG_SERVER_SERIES_KEY, pbuf);
	} else {
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf((char *)pbuf, "%s", pinfo->ServerSeries);
		package_add_xml_leaf(preqnode, MSG_SERVER_SERIES_KEY, pbuf);
	}

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(send_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf != NULL) {
		r_free(pbuf);
	}

	r_free(pxml);
	pxml = NULL;

	return return_code;
}


int32_t get_prerec_info_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t index = 0;
	int32_t roomid = 0;
	server_info *pinfo = NULL;
	lives_info *plive = NULL;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[get_prerec_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	return_code = STATUS_SUCCESS;

	if(STATUS_SUCCESS == return_code) {
		package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);
	} else {
		package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_FAILED);
	}

	/* 封装具体响应的内容 */ /* 注意 TODO: RecCtrlResp */
	pinfo = &penv->pserset->pserinfo->ServerInfo;
	package_get_prerec_info_resp_data(send_buf + headlen, pinfo);
	return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);

	roomid = puser->port - CONTROL_ROOM_SERVER_PORT_START;
	zlog_debug(OPELOG, "get_prerec_info_process, roomid = %d\n", roomid);

	if(roomid < 0 || roomid > penv->pserset->pserinfo->ServerInfo.MaxRoom) {
		zlog_debug(OPELOG, "get_prerec_info_process, roomid = %d\n", roomid);
		goto EXIT;
	}

	/* 发送当前的请求码流信息给会议室 */
	for(index = 0; index < VIDEO_USER_MAX_NUM; index++) {
		plive = &penv->lives_user[index];

		if(1 == plive->enable) {
			r_memset(send_buf + headlen, 0, CONTROL_DATA_LEN - headlen);
			package_stream_request_data(send_buf + headlen, plive, roomid);
			send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	}

EXIT:

	return return_code;
}



/********************************************************************************************/


/********************************************************************************************/
/* MSGCODE_LIVENODE_HEART_BEAT :  */

int32_t livenode_heart_beat_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                    int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                    int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[livenode_heart_beat_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	puser->heart_beat = CONTROL_TRUE;

	package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);
	return_code = send_user_xml_data(NULL, puser, send_buf, ret_buf, ret_len);

EXIT:

	return return_code;
}

/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_HEART_REPORT_REQ: 心跳(上报) */

int32_t room_heart_report_resolve_recv_buf(control_env *pcon, int8_t *recv_buf)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;
	room_info *proom_info_old = NULL;
	room_info *proom_info_new = NULL;
	int iEnc = 0;
	char cEncStatusKey[16] = {0};
	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	proom_info_new = (room_info *)r_malloc(sizeof(room_info));

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}
	zlog_error(DBGLOG, "\nroom[%s]\n", recv_buf);
	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	pnode = get_children_node(pnode, MSG_RECSERVER_STATUS_UPDATE_REQ_KEY);
	proom_node = get_children_node(pnode, MSG_ROOMSTATUS_KEY);

	/* 会议室ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf], roomid not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	roomid = atoi((const char *)pbuf);

	if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	//	zlog_debug(OPELOG, "roomid = %d\n", roomid);

	proom_info_old = &pser->RoomInfo[roomid];
	r_memcpy(proom_info_new, proom_info_old, sizeof(room_info));

	/* 连接状态 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_CONNECT_STATUS_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf], %s not found!\n", MSG_CONNECT_STATUS_KEY);
		return_code = STATUS_FAILED;
	} else {
		proom_info_new->ConnStatus = atoi((const char *)pbuf);
	}

	/* 录制状态 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_RECORD_STATUS_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf], %s not found!\n", MSG_RECORD_STATUS_KEY);
	} else {
		proom_info_new->RecStatus = atoi((const char *)pbuf);
	}

	/* 录制文件名称 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_RECNAME_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf], %s not found!\n", MSG_RECNAME_KEY);
	} else {
		r_strcpy((int8_t *)proom_info_new->RecName, (const int8_t *)pbuf);
	}


	/* 当前录制时长 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_RECTIME_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf], %s not found!\n", MSG_RECTIME_KEY);
	} else {
		r_strcpy((int8_t *)proom_info_new->RecTime, (const int8_t *)pbuf);
		zlog_error(DBGLOG, "RecTime:[%s]\n", proom_info_new->RecTime);
	}

	/*录制的开始时间*/
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_ROOM_RECSTARTTIME_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf], %s not found!\n", MSG_ROOM_RECSTARTTIME_KEY);
	} else {
		r_strcpy((int8_t *)proom_info_new->RecStartTime, (const int8_t *)pbuf);
		zlog_error(DBGLOG, "RecStartTime:[%s]\n", proom_info_new->RecStartTime);
	}

	/* RecordID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_ROOM_RECORDID_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf], %s not found!\n", MSG_ROOM_RECORDID_KEY);
	} else {
		r_strcpy((int8_t *)proom_info_new->RecordID, (const int8_t *)pbuf);
		zlog_error(DBGLOG, "RecordID:[%s]\n", proom_info_new->RecordID);
	}

	/* 合成模式 */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_PIC_SYNT_MODE_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf], %s not found!\n", MSG_PIC_SYNT_MODE_KEY);
	} else {
		proom_info_new->Mode = atoi((const char *)pbuf);
	}

	for(iEnc = 0; iEnc < RECORD_ROOM_MAX_ENC_NUM; iEnc ++) {
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		sprintf(cEncStatusKey, "Status%d", iEnc+1);
		pnode = get_children_node(proom_node, cEncStatusKey);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[room_heart_report_resolve_recv_buf], %s not found!\n", cEncStatusKey);
		} else {
			proom_info_new->EncInfo[iEnc].Status = atoi((const char *)pbuf);
		}
	}

	pthread_mutex_lock(&pser->info_m);
	modify_room_info_only((const int8_t *)CONFIG_TABLE_FILE, proom_info_old, proom_info_new);
	r_memcpy(proom_info_old, proom_info_new, sizeof(room_info));
	pthread_mutex_unlock(&pser->info_m);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	if(proom_info_new) {
		r_free(proom_info_new);
	}

	return return_code;
}


int32_t room_heart_report_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                      int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[set_room_info_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	room_heart_report_resolve_recv_buf(penv, recv_buf + headlen);
	package_resp_xml_data(send_buf + headlen, -1, msgcode, (int8_t *)XML_RETURNCODE_SUCCESS);
	return_code = send_user_xml_data(NULL, puser, send_buf, ret_buf, ret_len);

EXIT:

	return return_code;
}


int32_t room_heart_report_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                  int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                  int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == send_buf) {
		zlog_error(DBGLOG, "---[room_heart_report_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = room_heart_report_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		//		if(ManagePlatform == puser->platform){
		//			/* 与管理平台的心跳 */
		puser->heart_beat = CONTROL_TRUE;
		//		}
		zlog_error(DBGLOG, "[%s]\n", recv_buf+8);
	}

EXIT:

	return return_code;
}



/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_ENCINFO_REPORT_REQ: */

int32_t room_encinfo_report_resolve_recv_buf(control_env *pcon, con_user *puser, int8_t *recv_buf)
{
	int32_t return_code = STATUS_FAILED;
	int32_t ret = 0;
	int32_t roomid = 0;
	int32_t encid = 0;
	int32_t id = 0;

	//	warning_report warn = {0};

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	xmlNodePtr penc_node = NULL;

	all_server_info *pser = NULL;
	room_info *proom_info_old = NULL;
	room_info *proom_info_new = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == puser) {
		zlog_error(DBGLOG, "---[room_encinfo_report_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);
	proom_info_new = (room_info *)r_malloc(sizeof(room_info));

	zlog_error(DBGLOG, "%s\n", recv_buf);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[room_encinfo_report_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	proom_node = get_children_node(pxml->proot, MSG_BODY_KEY);

	/* 会议室ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_encinfo_report_resolve_recv_buf], roomid not found!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	roomid = atoi((const char *)pbuf);

	if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
		zlog_error(DBGLOG, "---[room_encinfo_report_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	//	zlog_debug(OPELOG, "roomid = %d\n", roomid);

	proom_info_old = &pser->RoomInfo[roomid];
	r_memcpy(proom_info_new, proom_info_old, sizeof(room_info));



	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_PICTURE_SYNT_SOURCENUM_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_encinfo_report_resolve_recv_buf], %s not found!\n", MSG_PICTURE_SYNT_SOURCENUM_KEY);
	} else {
		proom_info_new->PicSyntInfo.SourceNum = atoi((const char *)pbuf);

		if(proom_info_new->PicSyntInfo.SourceNum != proom_info_old->PicSyntInfo.SourceNum) {
			pcon->manager_flag = CONTROL_TRUE;
		}
	}

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	pnode = get_children_node(proom_node, MSG_PICTURE_SYNT_MODEL_KEY);
	ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[room_encinfo_report_resolve_recv_buf], %s not found!\n", MSG_PICTURE_SYNT_MODEL_KEY);
	} else {
		r_strncpy(proom_info_new->PicSyntInfo.Model, pbuf, PICTURE_VALUE_LENGTH);

		if(r_strcmp(proom_info_new->PicSyntInfo.Model, proom_info_old->PicSyntInfo.Model) != 0) {
			pcon->manager_flag = CONTROL_TRUE;
		}
	}

	penc_node = get_children_node(proom_node, MSG_ENCINFO_KEY);

	while(penc_node) {
		/* 编码器ID值 */
		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(penc_node, MSG_ENC_ID_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[room_encinfo_report_resolve_recv_buf], %s not found!\n", MSG_ENC_ID_KEY);
			return_code = STATUS_FAILED;
		} else {
			id = atoi((const char *)pbuf);
		}

		if(id < 1 || id > RECORD_ROOM_MAX_ENC_NUM) {
			penc_node = find_next_node(penc_node, MSG_ENCINFO_KEY);
			continue;
		}

		encid = id - 1;

		/* 注意索引值 */
		proom_info_new->EncInfo[encid].ID = id;

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(penc_node, MSG_ENC_STATUS_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[room_encinfo_report_resolve_recv_buf], %s not found!\n", MSG_ENC_ID_KEY);
			return_code = STATUS_FAILED;
		} else {
			proom_info_new->EncInfo[encid].vstatus = atoi((const char *)pbuf);
			zlog_error(DBGLOG, "---[encid = %d, vstatus = %d]\n", encid, proom_info_new->EncInfo[encid].vstatus);
		}

		r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
		pnode = get_children_node(penc_node, MSG_ENC_MUTE_KEY);
		ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[room_encinfo_report_resolve_recv_buf], %s not found!\n", MSG_ENC_ID_KEY);
			return_code = STATUS_FAILED;
		} else {
			proom_info_new->EncInfo[encid].mute = atoi((const char *)pbuf);
		}

		//		if(proom_info_new->EncInfo[encid].vstatus != proom_info_old->EncInfo[encid].vstatus)
		//		{
		//			r_memset(&warn, 0, sizeof(warning_report));
		//			warn.codec_id = id;
		//			warn.room_id = roomid;
		//			r_strcpy(warn.source, (const int8_t *)WARNING_SOURCE_CAPTURE);
		//			warn.warn_id = WARNING_WARN_ID_4;
		//			warning_report_sending(pcon, puser, &warn);
		//		}
		penc_node = find_next_node(penc_node, MSG_ENCINFO_KEY);
	}

	pic_synt_info *psynt_old = &proom_info_old->PicSyntInfo;
	pic_synt_info *psynt_new = &proom_info_new->PicSyntInfo;

	pthread_mutex_lock(&pser->info_m);
	modify_pic_synt_info_only((const int8_t *)CONFIG_TABLE_FILE, psynt_old, psynt_new);
	r_memcpy(psynt_old, psynt_new, sizeof(pic_synt_info));
	pthread_mutex_unlock(&pser->info_m);

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	if(proom_info_new) {
		r_free(proom_info_new);
	}

	return return_code;
}


int32_t room_encinfo_report_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                    int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                    int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;

	if(NULL == penv || NULL == send_buf) {
		zlog_error(DBGLOG, "---[room_encinfo_report_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = room_encinfo_report_resolve_recv_buf(penv, puser, recv_buf + headlen);
	} else {
		return_code = STATUS_FAILED;
	}

EXIT:

	return return_code;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_WARNING_REPORT:告警上报 */

int32_t package_warning_report_xml_data(int8_t *out_buf, warning_report *preport)
{
	int32_t 	return_code = STATUS_FAILED;
	int32_t 	xmllen = 0;;

	int8_t		msgcode_buf[16] = {0};

	int8_t		*pbuf = NULL;

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pdoc = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;
	xmlNodePtr	warn_node = NULL;

	if(NULL == out_buf || NULL == preport) {
		zlog_error(DBGLOG, "---[package_warning_report_xml_data] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == pbuf) {
		zlog_error(DBGLOG, "---[package_warning_report_xml_data] failed, malloc error\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pdoc = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pdoc, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	sprintf((char *)msgcode_buf, "%d", MSGCODE_WARNING_REPORT);
	package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, (const int8_t *)PLATFORM_RECSERVER);

	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	warn_node = xmlNewNode(NULL, MSG_WARN_WARNING_KEY);
	xmlAddChild(body_node, warn_node);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", preport->warn_id);
	package_add_xml_leaf(warn_node, MSG_WARN_SOURCE_KEY, (int8_t *)pbuf);

	package_add_xml_leaf(warn_node, MSG_WARN_SOURCE_KEY, (int8_t *)preport->source);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", preport->room_id);
	package_add_xml_leaf(warn_node, MSG_WARN_ROOMID_KEY, (int8_t *)pbuf);

	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", preport->codec_id);
	package_add_xml_leaf(warn_node, MSG_WARN_CODECID_KEY, (int8_t *)pbuf);

	xmlDocDumpFormatMemoryEnc(pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	return_code = STATUS_SUCCESS;

EXIT:

	if(pdoc != NULL) {
		release_dom_tree(pdoc);
	}

	if(pbuf) {
		r_free(pbuf);
	}

	return return_code;;
}


int32_t warning_report_sending(control_env *penv, con_user *puser, warning_report *preport)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t ret = 0;
	int8_t *psend_buf = NULL;
	int8_t *precv_buf = NULL;

	int32_t recv_len = 0;

	con_user *pforobj = NULL;

	if(NULL == penv || NULL == puser || NULL == preport) {
		zlog_error(DBGLOG, "---[warning_report_sending] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	psend_buf = r_malloc(CONTROL_DATA_LEN);
	precv_buf = r_malloc(CONTROL_DATA_LEN);

	if(NULL == psend_buf || NULL == precv_buf) {
		zlog_error(DBGLOG, "---[warning_report_sending] failed, malloc error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	r_memset(psend_buf, 0, CONTROL_DATA_LEN);
	return_code = package_warning_report_xml_data(psend_buf + headlen, preport);

	if(return_code != STATUS_SUCCESS) {
		zlog_error(DBGLOG, "---[warning_report_sending] failed, package_warning_report_xml_data error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	/* FIXME: 目前仅发给管理平台 */
	pforobj = find_forward_obj(penv->pserset, ManagePlatform, 0, 0);

	if(NULL == pforobj) {
		zlog_error(DBGLOG, "---[warning_report_sending] failed, [find_forward_obj] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	if(NULL == pforobj->pcmd.forward_process) {
		zlog_error(DBGLOG, "---[warning_report_sending] failed, [pcmd.forward_process] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	ret = pforobj->pcmd.forward_process(puser, pforobj, psend_buf, MSGCODE_WARNING_REPORT,
	                                    precv_buf, &recv_len);

	if(ret < 0) {
		zlog_error(DBGLOG, "---[warning_report_sending] failed, [forward_process] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	return_code = STATUS_SUCCESS;

EXIT:

	if(psend_buf) {
		r_free(psend_buf);
	}

	if(precv_buf) {
		r_free(precv_buf);
	}

	return return_code;
}

int32_t warning_report_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                               int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                               int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == send_buf) {
		zlog_error(DBGLOG, "---[warning_report_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = STATUS_SUCCESS;
	} else {
		/* 暂无响应 */
		return_code = STATUS_FAILED;
	}

EXIT:

	return return_code;
}


/********************************************************************************************/

/********************************************************************************************/
/* MSGCODE_GET_CAMCTL_PROLIST: 获取远遥协议 */

int32_t get_camctl_pro_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[get_camctl_pro_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_GET_REMOTE_CTRL_PRO_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[get_camctl_pro_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[get_camctl_pro_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[get_camctl_pro_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


int32_t get_camctl_pro_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                               int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                               int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[get_camctl_pro_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = get_camctl_pro_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


/********************************************************************************************/
/* MSGCODE_SET_CAMCTL_PRO: 设置远遥协议 */

int32_t set_camctl_pro_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[set_camctl_pro_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_SET_REMOTE_CTRL_PRO_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[set_camctl_pro_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[get_camctl_pro_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[get_camctl_pro_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 2);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


int32_t set_camctl_pro_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                               int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                               int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[set_camctl_pro_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = set_camctl_pro_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


/********************************************************************************************/
/* MSGCODE_UPLOAD_CAMCTL_PRO: 上传远遥协议文件 */

int32_t upload_camctl_pro_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
        int8_t **send_buf, roomid_array *parray, int32_t user_index, int32_t *data_length, int32_t *xml_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray || NULL == data_length || NULL == xml_len) {
		zlog_error(DBGLOG, "---[upload_camctl_pro_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[upload_camctl_pro_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	proom_node = get_children_node(pnode, MSG_UPLOAD_REMOTE_CTRL_PRO_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[upload_camctl_pro_resolve_recv_buf], roomid not found!\n");
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[upload_camctl_pro_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;
			}

			pnode = get_children_node(proom_node, MSG_SYS_UPGRADE_LENGTH_KEY);
			/* 上传文件长度 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[upload_camctl_pro_resolve_recv_buf], length not found!\n");
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			*data_length = atoi((const char *)pbuf);

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
			proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
		}
	}

	/* 用户ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", user_index);
	package_add_xml_leaf(pxml->proot, MSG_USERID_KEY, (int8_t *)pbuf);

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	/* FIXME，确保recv_buf的长度要大于 CONTROL_DATA_LEN */
	r_memset(recv_buf, 0, CONTROL_DATA_LEN - headlen);

	*send_buf = r_malloc(CONTROL_DATA_LEN + xmllen + *data_length + 2);

	if(*send_buf == NULL) {
		zlog_error(OPELOG, "---[upload_camctl_pro_resolve_recv_buf], malloc send buf failed!\n");
		xmlFree(xml_buf);
		goto EXIT;
	}

	r_memcpy(*send_buf + CONTROL_MSGHEAD_LEN, xml_buf, xmllen);
	xmlFree(xml_buf);

	*xml_len = xmllen;

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}

int32_t upload_logo_pro_resolve_recv_buf(control_env *pcon, int8_t *recv_buf,
        int8_t **send_buf, roomid_array *parray, int32_t user_index, int32_t *data_length, int32_t *xml_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t ret = 0;
	int32_t count = 0;
	int32_t roomid = 0;

	parse_xml_t *pxml = NULL;
	xmlNodePtr pnode = NULL;
	xmlNodePtr proom_node = NULL;
	all_server_info *pser = NULL;

	int32_t xmllen = 0;
	xmlChar *xml_buf = NULL;

	int8_t *pbuf = NULL;

	if(NULL == recv_buf || NULL == pcon || NULL == parray || NULL == data_length || NULL == xml_len) {
		zlog_error(DBGLOG, "---[upload_camctl_pro_resolve_recv_buf] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		return return_code;
	}

	parray->room_num = 0;

	pser = pcon->pserset->pserinfo;
	pxml = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	pbuf = r_malloc(XML_VALUE_MAX_LENGTH);

	if(NULL == init_dom_tree(pxml, (const char *)recv_buf)) {
		zlog_error(DBGLOG, "---[upload_camctl_pro_resolve_recv_buf] failed, [init_dom_tree] error!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	pnode = get_children_node(pxml->proot, MSG_BODY_KEY);
	proom_node = get_children_node(pnode, MSG_LOGO_UPLOADLOG_KEY);

	count = get_current_samename_node_nums(proom_node);

	if(count > 0) {
		if(count > pser->ServerInfo.MaxRoom) {
			count = pser->ServerInfo.MaxRoom;
		}

		while(count--) {
			pnode = get_children_node(proom_node, MSG_ROOMID_KEY);
			/* 会议室ID */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[upload_camctl_pro_resolve_recv_buf], roomid not found!\n");
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			roomid = atoi((const char *)pbuf);

			if(roomid < 0 || roomid > pser->ServerInfo.MaxRoom) {
				zlog_error(DBGLOG,
				           "---[upload_camctl_pro_resolve_recv_buf], roomid error, roomid = %d\n", roomid);
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;
			}

			pnode = get_children_node(proom_node, MSG_LOGO_UPLOADLOG_LEN_KEY);

			//pnode = get_children_node(proom_node, MSG_GET_AUDIO_INFO_KEY);
			/* 上传文件长度 */
			r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
			ret = get_current_node_value((char *)pbuf, XML_VALUE_MAX_LENGTH, pxml->pdoc, pnode);

			if(ret < 0) {
				zlog_error(DBGLOG, "---[upload_camctl_pro_resolve_recv_buf], length not found!\n");
				proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
				continue;	/* 读下组 RoomInfo */
			}

			*data_length = atoi((const char *)pbuf);

			/* 保存 roomid */
			parray->roomid[parray->room_num] = roomid;
			parray->room_num++;
			proom_node = find_next_node(proom_node, MSG_ROOMINFO_KEY);
		}
	}

	/* 用户ID */
	r_memset(pbuf, 0, XML_VALUE_MAX_LENGTH);
	sprintf((char *)pbuf, "%d", user_index);
	package_add_xml_leaf(pxml->proot, MSG_USERID_KEY, (int8_t *)pbuf);

	xmlDocDumpFormatMemoryEnc(pxml->pdoc, &xml_buf, &xmllen, XML_TEXT_CODE_TYPE, 1);
	/* FIXME，确保recv_buf的长度要大于 CONTROL_DATA_LEN */
	r_memset(recv_buf, 0, CONTROL_DATA_LEN - headlen);

	*send_buf = r_malloc(CONTROL_DATA_LEN + xmllen + *data_length + 2);

	if(*send_buf == NULL) {
		zlog_error(OPELOG, "---[upload_camctl_pro_resolve_recv_buf], malloc send buf failed!\n");
		xmlFree(xml_buf);
		goto EXIT;
	}

	r_memcpy(*send_buf + CONTROL_MSGHEAD_LEN, xml_buf, xmllen);
	xmlFree(xml_buf);

	*xml_len = xmllen;

	return_code = STATUS_SUCCESS;
EXIT:

	if(pxml != NULL) {
		if(pxml->pdoc != NULL) {
			release_dom_tree(pxml->pdoc);
		}
	}

	if(pbuf) {
		r_free(pbuf);
	}

	if(pxml) {
		r_free(pxml);
	}

	pxml = NULL;

	return return_code;
}


int32_t upload_camctl_pro_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                      int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;
	int32_t data_length;
	int32_t xml_len;
	int32_t recvlen = 0;

	int8_t *send_buf2 = NULL;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[upload_camctl_pro_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	upload_camctl_pro_resolve_recv_buf(penv, recv_buf + headlen, &send_buf2, &array, puser->index,
	                                   &data_length, &xml_len);

	if(data_length > 0) {
		if(send_buf2 != NULL) {
			recvlen = tcp_recv_longdata(puser->tcp_sock, send_buf2 + CONTROL_MSGHEAD_LEN + xml_len, data_length);

			if(recvlen != data_length) {
				array.room_num = 0;
			}
		} else {
			// TODO:防止data_length有值，而send_buf2为NULL的情形
		}
	}

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[upload_camctl_pro_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[upload_camctl_pro_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = send_user_xml_data2(puser, pforobj, send_buf2, xml_len, data_length);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[upload_camctl_pro_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


int32_t upload_camctl_pro_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                  int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                  int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[upload_camctl_pro_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = upload_camctl_pro_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


/********************************************************************************************/
/* MSGCODE_DEL_CAMCTL_PRO: 删除远遥协议文件 */

int32_t delete_camctl_pro_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                      int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[delete_camctl_pro_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_DEL_REMOTE_CTRL_PRO_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[delete_camctl_pro_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[delete_camctl_pro_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[delete_camctl_pro_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


int32_t delete_camctl_pro_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                  int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                  int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[delete_camctl_pro_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = delete_camctl_pro_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


/********************************************************************************************/
/* MSGCODE_GET_ENC_VERINFO: 获取编码器版本 */

int32_t get_encver_info_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                    int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[get_encver_info_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_GET_ENVVER_INFO_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[get_encver_info_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[get_encver_info_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[get_encver_info_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


int32_t get_encver_info_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[get_encver_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = get_encver_info_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}

int32_t get_audio_info_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[get_audio_info_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_GET_AUDIO_INFO_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(DBGLOG, "---[get_audio_info_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(DBGLOG, "---[get_audio_info_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(DBGLOG, "---[get_audio_info_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


int32_t get_audio_info_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                               int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                               int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[get_audio_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = get_audio_info_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}

int32_t picture_synthesis_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                      int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[picture_synthesis_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	array.room_num = 0;
	array.roomid[0] = -1;
	zlog_error(OPELOG, "!########################------recv :%s\n", recv_buf + headlen);
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_PICTURE_SYNTHESIS_KEY, CONTROL_FALSE);
	zlog_error(OPELOG, "!########################------send :%s\n", recv_buf + headlen);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error(OPELOG, "---[picture_synthesis_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error(OPELOG, "---[picture_synthesis_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error(OPELOG, "---[picture_synthesis_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


int32_t picture_synthesis_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                  int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                  int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[picture_synthesis_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = picture_synthesis_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}

int32_t picture_replay_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                   int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(DBGLOG, "---[picture_replay_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t		plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);
	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_REPLAY_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error2(DBGLOG, "---[picture_replay_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error2(DBGLOG, "---[picture_replay_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error2(DBGLOG, "---[picture_replay_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


int32_t picture_replay_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                               int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                               int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[picture_replay_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = picture_replay_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}

int32_t studio_director_info_req_process(control_env *penv, con_user *puser, int8_t *recv_buf,
        int8_t *send_buf, int32_t msgcode, int8_t *ret_buf, int32_t *ret_len)
{
	int32_t return_code = STATUS_FAILED;
	int32_t headlen = CONTROL_MSGHEAD_LEN;
	int32_t user_index = 0;
	int32_t index = 0;
	int32_t ret = 0;

	con_user *pforobj = NULL;
	roomid_array array;

	if(NULL == penv || NULL == recv_buf || NULL == ret_buf || NULL == ret_len || NULL == puser) {
		zlog_error(OPELOG, "---[studio_director_info_req_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t		plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);
	array.room_num = 0;
	array.roomid[0] = -1;
	common_resolve_recv_buf_and_add_userid(penv, recv_buf + headlen, &array, puser->index,
	                                       MSG_DIRECTOR_KEY, CONTROL_FALSE);

	/* FIXME: 如果有多个转发对象的时候? */
	/* 加入超时队列 */
	user_index = timeout_que_add_user(&penv->pserset->timeque, puser->platform, msgcode, puser->index);

	for(index = 0; index < array.room_num; index++) {
		/* 查找转发对象 */
		pforobj = find_forward_obj(penv->pserset, RecServer, array.roomid[index], 0);

		if(NULL == pforobj) {
			zlog_error2(OPELOG, "---[studio_director_info_req_process] failed, [find_forward_obj] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		if(NULL == pforobj->pcmd.forward_process) {
			zlog_error2(OPELOG, "---[studio_director_info_req_process] failed, [pcmd.forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		printf("[studio_director_info_req_process] [%s]\n", recv_buf + headlen);
		ret = pforobj->pcmd.forward_process(puser, pforobj, recv_buf, msgcode,
		                                    ret_buf, ret_len);

		if(ret < 0) {
			zlog_error2(OPELOG, "---[studio_director_info_req_process] failed, [forward_process] error!\n");
			return_code = STATUS_FAILED;
			continue;
		}

		return_code = STATUS_SUCCESS;
	}

	if((STATUS_SUCCESS == return_code) && (user_index >= 0)) {
		/* 等待超时期间不断查询是否已经返回 */
		ret = timeout_que_wait_a_moment(&penv->pserset->timeque, puser->platform, msgcode, puser->index, 6);

		if(ret < 0) {
			/* 已经超时了,返回超时消息 */
			timeout_que_del_user(&penv->pserset->timeque, user_index);
			package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
			return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
		}
	} else {
		if(user_index >= 0) {
			timeout_que_del_user(&penv->pserset->timeque, user_index);
		}

		package_resp_xml_data(send_buf + headlen, array.roomid[0], msgcode, (int8_t *)XML_RETURNCODE_FAILED);
		return_code = send_user_xml_data(puser, puser, send_buf, ret_buf, ret_len);
	}

EXIT:

	return return_code;
}


int32_t studio_director_info_process(control_env *penv, con_user *puser, int8_t *recv_buf,
                                     int8_t *send_buf, int8_t *ret_buf, int32_t *ret_len,
                                     int32_t msgcode)
{
	int32_t return_code = STATUS_FAILED;

	if(NULL == penv || NULL == recv_buf || NULL == send_buf || NULL == puser) {
		zlog_error(DBGLOG, "---[studio_director_info_process] failed, params is NULL!\n");
		return_code = STATUS_FAILED;
		goto EXIT;
	}

	int8_t plog_head[ZLOG_LOG_HEAD_LEN] = {0};
	r_strcpy(plog_head, puser->log_head);

	if(puser->ack.msgtype == XML_MSG_TYPE_REQ) {
		return_code = studio_director_info_req_process(penv, puser, recv_buf, send_buf, msgcode, ret_buf, ret_len);
	} else {
		return_code = common_resp_process(penv, puser, recv_buf, msgcode, ret_buf, ret_len);
	}

EXIT:

	return return_code;

}


/********************************************************************************************/




















