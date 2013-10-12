#include <stdlib.h>
#include "xml_cfg_management.h"

#define LIVE_CFG_ROOT_KEY (BAD_CAST "LiveCfg")
#define VOD_CFG_ROOT_KEY (BAD_CAST "VodCfg")
int32_t is_live_cfg(xmlNodePtr  proot)
{
	return xmlStrcmp(proot->name, LIVE_CFG_ROOT_KEY);
}
int32_t is_vod_cfg(xmlNodePtr  proot)
{
	return xmlStrcmp(proot->name, VOD_CFG_ROOT_KEY);
}
xmlNodePtr get_cfg_info_UserLimit_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	*pnode = get_children_node(proot, BAD_CAST "UserLimit");
	return *pnode;
}

xmlNodePtr get_cfg_info_NetCard_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	*pnode = get_children_node(proot, BAD_CAST "NetCard");
	return *pnode;
}

xmlNodePtr get_cfg_info_NetBandwidthLimit_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	*pnode = get_children_node(proot, BAD_CAST "NetBandwidthLimit");
	return *pnode;
}

xmlNodePtr get_cfg_info_RecType_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	*pnode = get_children_node(proot, BAD_CAST "RecType");
	return *pnode;
}

xmlNodePtr get_cfg_info_WebTcpListenPort_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	*pnode = get_children_node(proot, BAD_CAST "WebTcpListenPort");
	return *pnode;
}

xmlNodePtr get_cfg_info_VodFilePath_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	*pnode = get_children_node(proot, BAD_CAST "VodFilePath");
	return *pnode;
}


