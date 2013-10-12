/*******************************************************
 * Copyright (C), 2011-2018, Shenzhen Reach IT Co., Ltd.
 * @file:     xml_msg_management.c
 * @author:   ¶¡Õ×Öù
 * @version:     1.0
 * @date:   2011-12-22
 * @brief:   xml ¸ñÊ½×Ö·û´®·â×°ºÍ½âÎö
 * @Others: ÒÀÀµ libxml2-2.5.6forlinux
 * @Function List:
 * @History:
 *  1. Date:
 *      Author:
 *     Modification:
 *******************************************************/
#include <stdlib.h>
#include "xml_msg_management.h"

#define RESP_ROOT_KEY (BAD_CAST "ResponseMsg")

#define REQ_ROOT_KEY (BAD_CAST "RequestMsg")

/*######################xml ½âÎö###########################*/

xmlNodePtr get_resp_return_code_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgHead");
	*pnode = get_children_node(tmp_node, BAD_CAST "ReturnCode");
	return *pnode;
}

xmlNodePtr get_resp_recserver_info_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgBody");
	tmp_node = get_children_node(tmp_node, BAD_CAST "GetRecServersInfoResp");
	*pnode = get_children_node(tmp_node, BAD_CAST "RecServerInfo");
	return *pnode;
}

xmlNodePtr get_resp_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr recserver_info_node)
{
	*pnode = get_children_node(recserver_info_node, BAD_CAST "RecServerIP");
	return *pnode;
}

xmlNodePtr get_resp_room_id_node(xmlNodePtr *pnode, xmlNodePtr recserver_info_node)
{
	*pnode = get_children_node(recserver_info_node, BAD_CAST "RoomID");
	return *pnode;
}

int get_recserver_info(recserver_info_t *recserver_info, xmlDocPtr pdoc, xmlNodePtr recservers_info_node)
{
#if 0
	xmlNodePtr recserver_ip_node = NULL;
	xmlNodePtr room_id_node = NULL;
	int total_rooms = 0;
	char room_id[8] = {0};
	get_resp_recserver_ip_node(&recserver_ip_node, recservers_info_node);
	get_current_node_value(recserver_info->recserver_ip, pdoc, recserver_ip_node);
	get_resp_room_id_node(&room_id_node, recservers_info_node);
	total_rooms = get_current_samename_node_nums(room_id_node);

	while(total_rooms) {
		get_current_node_value(room_id, pdoc, room_id_node);
		*(recserver_info->room_id_pool) = atoi(room_id);
		recserver_info->room_id_pool ++;
		room_id_node = room_id_node->next;
		total_rooms --;
	}

#endif
	return 0;
}

int is_req_msg(xmlNodePtr  proot)
{
	if(proot == NULL) {
		return 0;
	}

	if(xmlStrcmp(proot->name, REQ_ROOT_KEY) != 0) {
		return 0;
	}

	return 1;
}
int is_resp_msg(xmlNodePtr  proot)
{
	if(proot == NULL) {
		return 0;
	}

	if(xmlStrcmp(proot->name, RESP_ROOT_KEY) != 0) {
		return 0;
	}

	return 1;
}

xmlNodePtr get_req_msg_code_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgHead");
	*pnode = get_children_node(tmp_node, BAD_CAST "MsgCode");
	return *pnode;
}

xmlNodePtr get_req_pass_key_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgHead");
	*pnode = get_children_node(tmp_node, BAD_CAST "PassKey");
	return *pnode;
}

xmlNodePtr get_req_setRecServerChannel_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgBody");
	tmp_node = get_children_node(tmp_node, BAD_CAST "SetRecServerChannelReq");
	*pnode = get_children_node(tmp_node, BAD_CAST "RecServerIP");
	return *pnode;
}

xmlNodePtr get_req_setRecServerChannel_room_id_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgBody");
	tmp_node = get_children_node(tmp_node, BAD_CAST "SetRecServerChannelReq");
	*pnode = get_children_node(tmp_node, BAD_CAST "RoomID");
	return *pnode;
}

xmlNodePtr get_req_getLiveChannel_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgBody");
	tmp_node = get_children_node(tmp_node, BAD_CAST "GetLiveChannelReq");
	*pnode = get_children_node(tmp_node, BAD_CAST "RecServerIP");
	return *pnode;
}

xmlNodePtr get_req_getLiveChannel_room_id_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgBody");
	tmp_node = get_children_node(tmp_node, BAD_CAST "GetLiveChannelReq");
	*pnode = get_children_node(tmp_node, BAD_CAST "RoomID");
	return *pnode;
}

xmlNodePtr get_req_stopUserLiveReq_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgBody");
	tmp_node = get_children_node(tmp_node, BAD_CAST "StopUserLiveReq");
	*pnode = get_children_node(tmp_node, BAD_CAST "RecServerIP");
	return *pnode;
}

xmlNodePtr get_req_stopUserLiveReq_user_ip_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgBody");
	tmp_node = get_children_node(tmp_node, BAD_CAST "StopUserLiveReq");
	*pnode = get_children_node(tmp_node, BAD_CAST "UserIP");
	return *pnode;
}

xmlNodePtr get_req_stopUserLiveReq_room_id_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgBody");
	tmp_node = get_children_node(tmp_node, BAD_CAST "StopUserLiveReq");
	*pnode = get_children_node(tmp_node, BAD_CAST "ChannelID");
	return *pnode;
}



