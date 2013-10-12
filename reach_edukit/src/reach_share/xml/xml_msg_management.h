/*******************************************************
 * Copyright (C), 2011-2018, Shenzhen Reach IT Co., Ltd.
 * @file:     xml_msg_management.h
 * @author:   丁兆柱
 * @version:     1.0
 * @date:   2011-12-22
 * @brief:   xml 格式字符串封装和解析
 * @Others: 依赖 libxml2-2.5.6forlinux
 * @Function List:
 * @History:
 *  1. Date:
 *      Author:
 *     Modification:
 *******************************************************/

#ifndef __XML_MSG__
#define __XML_MSG__

/** @ingroup     */
/*@{*/

#include "xml_base.h"
/**
* @brief 存储xml格式字符串buffer最大值
*/
#define MAX_XML_LEN	(100*1024)
/**
* @brief 存储xml格式通道信息buffer的最大值
*/
#define ROOM_INFO_XML_LEN	256
/**
* @brief 存储验证码buffer大小
*/
#define IP_ADDR_LEN	16
#define PASS_KEY_LEN	128
#define ROOM_ID_MAX	10
#define REQ_CODE_TCP_SIZE  6
#define REQ_VALUE_KEY  64

/*###############################################*/

/**
* @brief 判断当前根节点是否是响应消息的根节点
* @param  [IN] proot 当前根节点
* @exception  none
* @return  是返回1 ，否返回0
* @note  none
* @remarks  none
*/
int is_resp_msg(xmlNodePtr  proot);

/**
* @brief 获取响应消息返回码节点
* @param  [OUT] *pnode 响应消息返回码节点
* @param  [IN] proot 响应消息返回码节点dom树根节点
* @exception  none
* @return  成功返回响应消息返回码节点，失败返回NULL
* @note  none
* @remarks  none
* @code
*	#include "xml_msg_management.h"
*	#define MSGHEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?><ResponseMsg><MsgHead><ReturnCode>1</ReturnCode><ReturnCode>2</ReturnCode><ReturnCode>3</ReturnCode><ReturnCode>4</ReturnCode></MsgHead><MsgBody/></ResponseMsg>"
*	char value[8] = {0};
*	xmlNodePtr pnode;
*	parse_xml_t parse_xml;
*	init_dom_tree(&parse_xml, MSGHEAD);
*	get_resp_return_code_node(&pnode, parse_xml.proot);
*	get_current_node_value(value,value_len, parse_xml.pdoc, pnode);
*	printf("msg_head:[%s]\n", value);
*	release_dom_tree(parse_xml.pdoc);
* @endcode
*/
xmlNodePtr get_resp_return_code_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief 获取响应消息录播服务器信息节点
* @param  [OUT] *pnode 响应消息录播服务器信息节点
* @param  [IN] proot 响应消息录播服务器信息节点所在dom树根节点
* @exception  none
* @return  成功返回响应消息录播服务器信息节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_resp_recserver_info_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief 获取响应消息录播服务器IP 节点
* @param  [OUT] *pnode 响应消息录播服务器IP 节点
* @param  [IN] proot 响应消息录播服务器信息节点
* @exception  none
* @return  成功返回响应消息录播服务器IP 节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_resp_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr recserver_info_node);

/**
* @brief 获取响应消息通道ID  节点
* @param  [OUT] *pnode 响应消息录通道ID  节点
* @param  [IN] recserver_info_node  响应消息录播服务器信息节点
* @exception  none
* @return  成功返回响应消息录通道ID   节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_resp_room_id_node(xmlNodePtr *pnode, xmlNodePtr recserver_info_node);

typedef struct recserver_infos
{
	char recserver_ip[IP_ADDR_LEN];
	int32_t room_id_pool[ROOM_ID_MAX];
} recserver_info_t;

/**
* @brief 录播服务器信息
* @param  [OUT]  recserver_info 存储录播服务器信息buffer
* @param  [IN] pdoc 响应消息录播服务器信息节点所在dom树句柄
* @param  [IN] recservers_info_node 响应消息录播服务器信息节点
* @exception  none
* @return 成功返回0
* @note  none
* @remarks  none
*/
int get_recserver_info(recserver_info_t *recserver_info, xmlDocPtr pdoc, xmlNodePtr recservers_info_node);

/**
* @brief 判断当前根节点是否是请求消息的根节点
* @param  [IN] proot 当前根节点
* @exception  none
* @return  是返回1 ，否返回0
* @note  none
* @remarks  none
*/
int is_req_msg(xmlNodePtr  proot);

/**
* @brief 获取请求消息码节点
* @param  [OUT] *pnode 请求消息码节点
* @param  [IN] proot 请求消息码节点所在dom树的根节点
* @exception  none
* @return  成功返回消息码节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_msg_code_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief 获取请求消息验证码节点
* @param  [OUT] *pnode 请求消息验证码节点
* @param  [IN] proot 请求消息验证码节点所在dom树的根节点
* @exception  none
* @return  成功返回消息验证码节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_pass_key_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief 获取设置录播服务器IP 节点
* @param  [OUT] *pnode 设置录播服务器IP 节点
* @param  [IN] proot 设置录播服务器IP 节点所在dom树的根节点
* @exception  none
* @return  成功返回设置录播服务器IP 节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_setRecServerChannel_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief 获取设置录播服务器通道ID 节点
* @param  [OUT] *pnode 设置录播服务器通道ID 节点
* @param  [IN] proot 设置录播服务器通道ID 节点所在dom树的根节点
* @exception  none
* @return  成功返回设置录播服务器通道ID 节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_setRecServerChannel_room_id_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief 获取直播通道的录播IP 节点
* @param  [OUT] *pnode 直播通道的录播IP 节点
* @param  [IN] proot 直播通道的录播IP 节点所在dom树的根节点
* @exception  none
* @return  成功返回 直播通道的录播IP 节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_getLiveChannel_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief 获取直播通道ID 节点
* @param  [OUT] *pnode 直播通道ID 节点
* @param  [IN] proot 直播通道ID 节点所在dom树的根节点
* @exception  none
* @return  成功返回 直播通道ID 节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_getLiveChannel_room_id_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief 获取停止用户直播的录播IP 节点
* @param  [OUT] *pnode 停止用户直播的录播IP 节点
* @param  [IN] proot 停止用户直播的录播IP 节点所在dom树的根节点
* @exception  none
* @return  成功返回 停止用户直播的录播IP 节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_stopUserLiveReq_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief 获取停止用户直播的用户IP 节点
* @param  [OUT] *pnode 停止用户直播的用户IP 节点
* @param  [IN] proot 停止用户直播的用户IP 节点所在dom树的根节点
* @exception  none
* @return  成功返回 停止用户直播的用户IP 节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_stopUserLiveReq_user_ip_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief 获取停止用户直播通道ID 节点
* @param  [OUT] *pnode 停止用户直播通道ID 节点
* @param  [IN] proot 停止用户直播通道ID 节点所在dom树的根节点
* @exception  none
* @return  成功返回 停止用户直播通道ID 节点，失败返回NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_stopUserLiveReq_room_id_node(xmlNodePtr *pnode, xmlNodePtr proot);

/*@}*/
#endif //__XML_MSG__

