/*******************************************************
 * Copyright (C), 2011-2018, Shenzhen Reach IT Co., Ltd.
 * @file:     xml_msg_management.h
 * @author:   ������
 * @version:     1.0
 * @date:   2011-12-22
 * @brief:   xml ��ʽ�ַ�����װ�ͽ���
 * @Others: ���� libxml2-2.5.6forlinux
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
* @brief �洢xml��ʽ�ַ���buffer���ֵ
*/
#define MAX_XML_LEN	(100*1024)
/**
* @brief �洢xml��ʽͨ����Ϣbuffer�����ֵ
*/
#define ROOM_INFO_XML_LEN	256
/**
* @brief �洢��֤��buffer��С
*/
#define IP_ADDR_LEN	16
#define PASS_KEY_LEN	128
#define ROOM_ID_MAX	10
#define REQ_CODE_TCP_SIZE  6
#define REQ_VALUE_KEY  64

/*###############################################*/

/**
* @brief �жϵ�ǰ���ڵ��Ƿ�����Ӧ��Ϣ�ĸ��ڵ�
* @param  [IN] proot ��ǰ���ڵ�
* @exception  none
* @return  �Ƿ���1 ���񷵻�0
* @note  none
* @remarks  none
*/
int is_resp_msg(xmlNodePtr  proot);

/**
* @brief ��ȡ��Ӧ��Ϣ������ڵ�
* @param  [OUT] *pnode ��Ӧ��Ϣ������ڵ�
* @param  [IN] proot ��Ӧ��Ϣ������ڵ�dom�����ڵ�
* @exception  none
* @return  �ɹ�������Ӧ��Ϣ������ڵ㣬ʧ�ܷ���NULL
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
* @brief ��ȡ��Ӧ��Ϣ¼����������Ϣ�ڵ�
* @param  [OUT] *pnode ��Ӧ��Ϣ¼����������Ϣ�ڵ�
* @param  [IN] proot ��Ӧ��Ϣ¼����������Ϣ�ڵ�����dom�����ڵ�
* @exception  none
* @return  �ɹ�������Ӧ��Ϣ¼����������Ϣ�ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_resp_recserver_info_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief ��ȡ��Ӧ��Ϣ¼��������IP �ڵ�
* @param  [OUT] *pnode ��Ӧ��Ϣ¼��������IP �ڵ�
* @param  [IN] proot ��Ӧ��Ϣ¼����������Ϣ�ڵ�
* @exception  none
* @return  �ɹ�������Ӧ��Ϣ¼��������IP �ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_resp_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr recserver_info_node);

/**
* @brief ��ȡ��Ӧ��Ϣͨ��ID  �ڵ�
* @param  [OUT] *pnode ��Ӧ��Ϣ¼ͨ��ID  �ڵ�
* @param  [IN] recserver_info_node  ��Ӧ��Ϣ¼����������Ϣ�ڵ�
* @exception  none
* @return  �ɹ�������Ӧ��Ϣ¼ͨ��ID   �ڵ㣬ʧ�ܷ���NULL
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
* @brief ¼����������Ϣ
* @param  [OUT]  recserver_info �洢¼����������Ϣbuffer
* @param  [IN] pdoc ��Ӧ��Ϣ¼����������Ϣ�ڵ�����dom�����
* @param  [IN] recservers_info_node ��Ӧ��Ϣ¼����������Ϣ�ڵ�
* @exception  none
* @return �ɹ�����0
* @note  none
* @remarks  none
*/
int get_recserver_info(recserver_info_t *recserver_info, xmlDocPtr pdoc, xmlNodePtr recservers_info_node);

/**
* @brief �жϵ�ǰ���ڵ��Ƿ���������Ϣ�ĸ��ڵ�
* @param  [IN] proot ��ǰ���ڵ�
* @exception  none
* @return  �Ƿ���1 ���񷵻�0
* @note  none
* @remarks  none
*/
int is_req_msg(xmlNodePtr  proot);

/**
* @brief ��ȡ������Ϣ��ڵ�
* @param  [OUT] *pnode ������Ϣ��ڵ�
* @param  [IN] proot ������Ϣ��ڵ�����dom���ĸ��ڵ�
* @exception  none
* @return  �ɹ�������Ϣ��ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_msg_code_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief ��ȡ������Ϣ��֤��ڵ�
* @param  [OUT] *pnode ������Ϣ��֤��ڵ�
* @param  [IN] proot ������Ϣ��֤��ڵ�����dom���ĸ��ڵ�
* @exception  none
* @return  �ɹ�������Ϣ��֤��ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_pass_key_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief ��ȡ����¼��������IP �ڵ�
* @param  [OUT] *pnode ����¼��������IP �ڵ�
* @param  [IN] proot ����¼��������IP �ڵ�����dom���ĸ��ڵ�
* @exception  none
* @return  �ɹ���������¼��������IP �ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_setRecServerChannel_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief ��ȡ����¼��������ͨ��ID �ڵ�
* @param  [OUT] *pnode ����¼��������ͨ��ID �ڵ�
* @param  [IN] proot ����¼��������ͨ��ID �ڵ�����dom���ĸ��ڵ�
* @exception  none
* @return  �ɹ���������¼��������ͨ��ID �ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_setRecServerChannel_room_id_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief ��ȡֱ��ͨ����¼��IP �ڵ�
* @param  [OUT] *pnode ֱ��ͨ����¼��IP �ڵ�
* @param  [IN] proot ֱ��ͨ����¼��IP �ڵ�����dom���ĸ��ڵ�
* @exception  none
* @return  �ɹ����� ֱ��ͨ����¼��IP �ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_getLiveChannel_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief ��ȡֱ��ͨ��ID �ڵ�
* @param  [OUT] *pnode ֱ��ͨ��ID �ڵ�
* @param  [IN] proot ֱ��ͨ��ID �ڵ�����dom���ĸ��ڵ�
* @exception  none
* @return  �ɹ����� ֱ��ͨ��ID �ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_getLiveChannel_room_id_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief ��ȡֹͣ�û�ֱ����¼��IP �ڵ�
* @param  [OUT] *pnode ֹͣ�û�ֱ����¼��IP �ڵ�
* @param  [IN] proot ֹͣ�û�ֱ����¼��IP �ڵ�����dom���ĸ��ڵ�
* @exception  none
* @return  �ɹ����� ֹͣ�û�ֱ����¼��IP �ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_stopUserLiveReq_recserver_ip_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief ��ȡֹͣ�û�ֱ�����û�IP �ڵ�
* @param  [OUT] *pnode ֹͣ�û�ֱ�����û�IP �ڵ�
* @param  [IN] proot ֹͣ�û�ֱ�����û�IP �ڵ�����dom���ĸ��ڵ�
* @exception  none
* @return  �ɹ����� ֹͣ�û�ֱ�����û�IP �ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_stopUserLiveReq_user_ip_node(xmlNodePtr *pnode, xmlNodePtr proot);

/**
* @brief ��ȡֹͣ�û�ֱ��ͨ��ID �ڵ�
* @param  [OUT] *pnode ֹͣ�û�ֱ��ͨ��ID �ڵ�
* @param  [IN] proot ֹͣ�û�ֱ��ͨ��ID �ڵ�����dom���ĸ��ڵ�
* @exception  none
* @return  �ɹ����� ֹͣ�û�ֱ��ͨ��ID �ڵ㣬ʧ�ܷ���NULL
* @note  none
* @remarks  none
* @see  get_resp_return_code_node
*/
xmlNodePtr get_req_stopUserLiveReq_room_id_node(xmlNodePtr *pnode, xmlNodePtr proot);

/*@}*/
#endif //__XML_MSG__

