
#ifndef __XML_BASE__
#define __XML_BASE__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reach_os.h"

#include "parser.h"
#include "tree.h"
/**
*@brief dom������Ϣ
*/
typedef struct parse_xmls {
	/** @dom�����*/
	xmlDocPtr pdoc;	
	/** @dom���ĸ��ڵ�*/
	xmlNodePtr  proot;	
} parse_xml_t;

/**
* @brief ��xml��ʽ�ַ�������dom������ȡ����ڵ�
* @param  [OUT] parse_xml ����dom������Ϣ
* @param  [IN] xml	xml�ַ���
* @exception  none  
* @return  none
* @note  none
* @remarks  none
*/
void *init_dom_tree(parse_xml_t *parse_xml, const char *xml);

/**
* @brief ���ļ���Ϊfile_name��xml�ļ�����dom������ȡ����ڵ�
* @param  [OUT] parse_xml ����dom������Ϣ
* @param  [IN] file_name	xml�ļ���
* @exception  none  
* @return  none
* @note  none
* @remarks  none
*/
void *init_file_dom_tree(parse_xml_t *parse_xml, const char *file_name);

/**
* @brief �ͷ�����dom��
* @param  [OUT] pdoc dom�����
* @exception  none
* @return  none
* @note  none
* @remarks  none
*/
void release_dom_tree(xmlDocPtr pdoc);

/**
* @brief ��ȡ�ӽڵ�
* @param  [IN] curNode ��ǰ�ڵ�
* @param  [IN] �ӽڵ���
* @exception  none  
* @return  �ӽڵ�
* @note  none
* @remarks  none
*/
xmlNodePtr get_children_node(xmlNodePtr curNode, const xmlChar * key);

/**
* @brief ��ȡ��ǰ�ڵ��ֵ
* @param  [OUT] value �洢��ȡ�Ľڵ��ֵbuffer
* @param  [OUT] pdoc dom�����
* @param  [IN] curNode ��ǰҪ��ȡֵ�Ľڵ�
* @exception  none  
* @return  �ɹ�����0 ��ʧ�ܷ���-1
* @note  none
* @remarks  none
*/
int32_t get_current_node_value(char *value, int32_t value_len, xmlDocPtr pdoc,  xmlNodePtr 
curNode);

/**
* @brief ��ȡ��ǰͬ���ڵ����
* @param  [IN]  curNode ��ǰ�ڵ�
* @exception  none
* @return  �ɹ����ص�ǰͬ���ڵ����
* @note  none
* @remarks  none
*/
int get_current_samename_node_nums(xmlNodePtr curNode);

/**
* @brief �����µĽڵ�
* @param  [IN]  parent Ҫ���ӽڵ�ĸ��ڵ�
* @param  [IN]	name	Ҫ���ӽڵ������
* @param  [IN]  content ���ӽڵ�Ĳ���
* @exception  none
* @return  ���������ڵ�
* @note  none
* @remarks  none
*/
xmlNodePtr	add_new_node(xmlNodePtr parent,const xmlChar *name,const xmlChar *content);

/**
* @brief �޸Ľڵ��ֵ
* @param  [IN]  cur Ҫ�޸ĵĽڵ�
* @param  [IN]	content	Ҫ�޸Ľڵ��ֵ
* @exception  none
* @return  ����0�ɹ�
* @note  none
* @remarks  none
*/
int	modify_node_value(xmlNodePtr cur,const xmlChar *content);

/**
* @brief ɾ���ڵ�
* @param  [IN]  pnode Ҫɾ���Ľڵ�
* @exception  none
* @return  ����0�ɹ�
* @note  none
* @remarks  none
*/
xmlNodePtr del_cur_node(xmlNodePtr pnode);

/**
* @brief ���ýڵ�����
* @param  [IN]  node Ҫ�޸����ԵĽڵ�
* @param  [IN]  name �ڵ��е�����
* @param  [IN]  value ���Ե�ֵ
* @exception  none
* @return  ����xmlAttrPtr���͵Ľṹ��
* @note  none
* @remarks  none
*/
xmlAttrPtr xml_prop_set(xmlNodePtr node,const xmlChar *name,const xmlChar *value);

/**
* @briefɾ���ڵ�����
* @param  [IN]  curNode Ҫɾ�����ԵĽڵ�
* @param  [IN]  attr_name �ڵ��е�����
* @exception  none
* @return  ����0
* @note  none
* @remarks  none
*/
int xml_prop_del(xmlNodePtr curNode, const xmlChar *attr_name);

/**
* @brief ����µĽڵ�
* @param  [IN]  parent Ҫ���ӽڵ�ĸ��ڵ�
* @param  [IN]  ns ��ʱ����
* @param  [IN]  name ���ӽڵ������
* @param  [IN]  content ���ӽڵ��ֵ,��NULLʱ��������Ϊ<cctv />
* @exception  none
* @return  ���ؽڵ�ָ��
* @note  none
* @remarks  none
*/
xmlNodePtr xml_add_new_child(xmlNodePtr parent, xmlNsPtr ns, 
	const xmlChar *name, const xmlChar *content);

/**
* @brief ���ýڵ�ֵ
* @param  [IN]  cur Ҫ�޸�ֵ�Ľڵ�
* @param  [IN]  content �ڵ�ֵ
* @param  [IN]  len �ڵ�ֵ�ĳ���
* @exception  none
* @return  ���ؽڵ�ָ��
* @note  none
* @remarks  none
*/
int	xml_node_value_set(xmlNodePtr cur,const xmlChar *content,int len);

xmlNodePtr find_next_node(xmlNodePtr cur_node, const xmlChar *node_name);



#endif //__XML_BASE__

