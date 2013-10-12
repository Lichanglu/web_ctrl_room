#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "xml_base.h"
#include "process_xml_cmd.h"
#include "nslog.h"

#define MAX_CLIENT				6

#ifdef SUPPORT_XML_PROTOCOL
#ifndef TRUE
#define TRUE              1
#endif

#ifndef FALSE
#define FALSE             0
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET 			-1
#endif

//static unsigned char		gszSendBuf[MAX_PACKET] = {0};
//static unsigned int		gnSentCount  = 0;



/////////////////////////////////////////////////////////////////////////
#ifdef SUPPORT_IP_MATRIX

#define  MAX_RESOLUTION_WIDTH				1920
#define  MAX_RESOLUTION_HEIGHT				1080
#define  MAX_VIDEO_BATE						20000
#define  MAX_AUDIO_BATE						128000
#endif

#define ROOM_INFO_XML_NO_USEID   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<PassKey>%s</PassKey>\n<MsgCode>%d</MsgCode>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomInfo>\n<RoomID>%d</RoomID>\n<ConnStatus>%d</ConnStatus>\n<RecStatus>%d</RecStatus>\n<RecName>%s</RecName>\n<AudioInfo>\n<InputMode>%d</InputMode>\n<SampleRate>%d</SampleRate>\n<Bitrate>%d</Bitrate>\n<Lvolume>%d</Lvolume>\n	<Rvolume>%d</Rvolume>\n</AudioInfo>\n<EncInfo>\n<ID>%d</ID>\n<EncIP>%s</EncIP>\n<Status>%d</Status>\n<QualityInfo>\n<RateType>%d</RateType>\n<EncBitrate>%d</EncBitrate>\n<EncWidth>%d</EncWidth>\n<EncHeight>%d</EncHeight>\n<EncFrameRate>%d</EncFrameRate>\n</QualityInfo>\n</EncInfo>\n</RoomInfo>\n</MsgBody>\n</ResponseMsg>"

#define PACK_GET_VOL_XML_NO_USEID "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n<VoiceInfo>\n<Volume>%d</Volume>\n</VoiceInfo>\n</MsgBody>\n</ResponseMsg>"

#define PACK_MUTE_XML_NO_USEID 	"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n</ResponseMsg>"

#define PACK_REQUEST_IFRAME_XML_NO_USEID	"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n</ResponseMsg>"

#define PACK_PIC_ADJUST_XML_NO_USEID		"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n</ResponseMsg>"

#define PACK_FIX_RESOLUTION_XML_NO_USEID   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RecCtrlResp>\n<RoomID>%d</RoomID>\n<Result>%d</Result>\n</RecCtrlResp>\n</MsgBody>\n</ResponseMsg>"

#define ROOM_INFO_XML   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<PassKey>%s</PassKey>\n<MsgCode>%d</MsgCode>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomInfo>\n<RoomID>%d</RoomID>\n<ConnStatus>%d</ConnStatus>\n<RecStatus>%d</RecStatus>\n<RecName>%s</RecName>\n<AudioInfo>\n<InputMode>%d</InputMode>\n<SampleRate>%d</SampleRate>\n<Bitrate>%d</Bitrate>\n<Lvolume>%d</Lvolume>\n<Rvolume>%d</Rvolume>\n</AudioInfo>\n<EncInfo>\n<ID>%d</ID>\n<EncIP>%s</EncIP>\n<Status>%d</Status>\n<QualityInfo>\n<RateType>%d</RateType>\n<EncBitrate>%d</EncBitrate>\n<EncWidth>%d</EncWidth>\n<EncHeight>%d</EncHeight>\n<EncFrameRate>%d</EncFrameRate>\n</QualityInfo>\n</EncInfo>\n</RoomInfo>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"

#define PACK_GET_VOL_XML "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n<VoiceInfo>\n<Volume>%d</Volume>\n</VoiceInfo>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"

#define PACK_MUTE_XML 	"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"

#define PACK_REQUEST_IFRAME_XML	"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"

#define PACK_PIC_ADJUST_XML		"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"

#define PACK_REPORT_XML		"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><RequestMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n</MsgHead>\n<MsgBody>\n<EncInfo>\n<Status>%d</Status>\n<Mute>%d</Mute>\n</EncInfo>\n</MsgBody>\n</RequestMsg>"

#define PACK_FIX_RESOLUTION_XML   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RecCtrlResp>\n<RoomID>%d</RoomID>\n<Result>%d</Result>\n</RecCtrlResp>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"


#ifdef SUPPORT_IP_MATRIX
#define PACK_ENCODE_ENABLE_XML   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<EnableInfo>\n<MaxWidth>%d</MaxWidth>\n<MaxHeight>%d</MaxHeight>\n<MaxVideoBite>%d</MaxVideoBite>\n<MaxVideoFrame>%d</MaxVideoFrame>\n<MaxAudioBite>%d</MaxAudioBite>\n</EnableInfo>\n</MsgBody>\n</ResponseMsg>"

#define PACK_GET_SYSTEM_XML   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<Name>%s</Name>\n<SerialNum>%s</SerialNum>\n<MacAddr>%s</MacAddr>\n<IPAddr>%s</IPAddr>\n<GateWay>%s</GateWay>\n<NetMask>%s</NetMask>\n<DeviceVersion>%s</DeviceVersion>\n<VideoRate >%d</VideoRate >\n<DeviceType >%d</DeviceType >\n</MsgBody>\n</ResponseMsg>"
#endif


static int g_audio_param_change[2] = {0};
static int g_video_param_change[2] = {0};

static pthread_mutex_t   g_video_param_mutex[2];
static pthread_mutex_t   g_audio_param_mutex[2];

#define RESP_ROOT_KEY (BAD_CAST "ResponseMsg")
#define REQ_ROOT_KEY (BAD_CAST "RequestMsg")
int is_request_msg(xmlNodePtr  proot)
{
	if(xmlStrcmp(proot->name, REQ_ROOT_KEY) != 0) {
		return 0;
	}

	return 1;
}

int is_response_msg(xmlNodePtr  proot)
{
	if(xmlStrcmp(proot->name, RESP_ROOT_KEY) != 0) {
		return 0;
	}

	return 1;
}

xmlNodePtr msg_get_next_samename_node(xmlNodePtr curNode)
{
	xmlChar *key = NULL;
	xmlNodePtr node = NULL;

	if(NULL == curNode) {
		return NULL;
	}

	key = (xmlChar *)curNode->name;
	curNode = curNode->next;

	while(NULL != curNode) {
		if(!xmlStrcmp(curNode->name, key)) {
			node = curNode;
			break;
		}

		curNode = curNode->next;
	}

	return node;
}


int package_add_xml_leaf(xmlNodePtr child_node, xmlNodePtr far_node, char *key_name, char *key_value)
{
	child_node = xmlNewNode(NULL, BAD_CAST key_name);
	xmlAddChild(far_node, child_node);
	xmlAddChild(child_node, xmlNewText(BAD_CAST key_value));
	return 0;
}

int package_head_msg(char *send_buf, int msg_code_val, char *passkey, char *return_val, char *user_id)
{
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"ResponseMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr code_node 			= NULL;
	xmlNodePtr passkey_node 			= NULL;
	xmlNodePtr return_node 			= NULL;
	xmlNodePtr user_id_node 			= NULL;
	char msgCode[8] = {0};
	sprintf(msgCode, "%d", msg_code_val);
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	package_add_xml_leaf(code_node, head_node, (char *)"MsgCode", (char *)msgCode);
	package_add_xml_leaf(passkey_node, head_node, (char *)"PassKey", (char *)passkey);

	package_add_xml_leaf(return_node, head_node, (char *)"ReturnCode", (char *)return_val);
	package_add_xml_leaf(body_node, root_node, (char *)"MsgBody", (char *)"");
	printf("[package_head_msg] user_id:[%s][%d]\n", user_id, atoi(user_id));

	if(atoi(user_id) != -1) {
		package_add_xml_leaf(user_id_node, root_node, (char *)"UserID", (char *)user_id);
	}

	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	printf("%s\n", (char *)temp_xml_buf);
	memcpy(send_buf, temp_xml_buf, size);
	//printf("%s\n",send_buf);
	xmlFree(temp_xml_buf);
	release_dom_tree(doc);
	return 0;
}
xmlNodePtr get_xml_msgcode(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgHead");
	*pnode = get_children_node(tmp_node, BAD_CAST "MsgCode");
	return *pnode;
}
xmlNodePtr get_xml_passkey(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgHead");
	*pnode = get_children_node(tmp_node, BAD_CAST "PassKey");
	return *pnode;
}

xmlNodePtr get_xml_msgbody_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	*pnode = get_children_node(proot, BAD_CAST "MsgBody");
	return *pnode;
}

xmlNodePtr get_response_node(xmlNodePtr *pnode, xmlNodePtr recserver_info_node, const char *string)
{
	*pnode = get_children_node(recserver_info_node, BAD_CAST string);
	return *pnode;
}

//解析用户登录xml信令。

int parse_xml_login_msg(parse_xml_t *parse_xml_info, user_login_info *login_info)
{
	xmlNodePtr pnode_username_xml = NULL;
	xmlNodePtr pnode_password_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	int ret = 0;
	int len = 0;


	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_username_xml, pnode_msg_body_xml, "UserName") == NULL) {
		PRINTF("pnode_username_xml=%p\n", pnode_username_xml);
		return -1;
	}

	len = sizeof(login_info->username);
	ret = get_current_node_value(login_info->username, len, parse_xml_info->pdoc,  pnode_username_xml);

	if(ret < 0) {
		PRINTF("get username failed\n");
		return -1;
	}

	if(get_response_node(&pnode_password_xml, pnode_msg_body_xml, "Password") == NULL) {
		PRINTF("pnode_password_xml=%p\n", pnode_password_xml);
		return -1;
	}

	len = sizeof(login_info->password);
	ret = get_current_node_value(login_info->password, len, parse_xml_info->pdoc,  pnode_password_xml);

	if(ret < 0) {
		PRINTF("get password failed\n");
		return -1;
	}

	PRINTF("username=%s,password=%s\n", login_info->username, login_info->password);
	return 0;
}

//解析UserIDxml信令

int parse_xml_user_id_msg(parse_xml_t *parse_xml_info, char *user_id)
{
	xmlNodePtr userid_node = NULL;
	char tmpbuf[16] = {0};
	userid_node = get_children_node(parse_xml_info->proot, BAD_CAST "UserID");

	if(userid_node == NULL) {
		sprintf(user_id, "%c%c", '-', '1');
		PRINTF("get UserID node failed user_id=%s\n", user_id);
		return -1;
	}

	if(get_current_node_value(tmpbuf, 16, parse_xml_info->pdoc, userid_node) <  0) {
		sprintf(user_id, "%c%c", '-', '1');
		PRINTF("get UserID node  value  failed.user_id=%s\n", user_id);
		return -1;
	}

	strcpy(user_id, tmpbuf);

	return 0;
}

int process_get_room_info_cmd(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd  = GETSOCK_NEW(index, pos);
	AUDIO_PARAM audio;
	RATE_INFO high_rate;
	int room_id = *roomid;
	memset(send_buf, 0, sizeof(send_buf));
	getXmlVideoParams(index, &high_rate);
	getXmlAudioParams(index, &audio);
	package_room_info_msg(send_buf, &audio, &high_rate, msgCode, passkey, 1, user_id, room_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}


//解析请求码流xml信令。
int parse_xml_request_rate_msg(int index, int pos, parse_xml_t *parse_xml_info, char *user_id, int *roomId, char *passkey)
{
	xmlNodePtr pnode_strm_req_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	xmlNodePtr pnode_quality_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr tmp_pnode_xml = NULL;
	int ret = 0;
	char roomid[8] = {0};
	char quality[8] = {0};
	int len = 0;

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_strm_req_xml, pnode_msg_body_xml, "StrmReq") == NULL) {
		PRINTF("get strmreq failed\n");
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_strm_req_xml, "RoomID") == NULL) {
		PRINTF("get ROOMID failed\n");
		return -1;
	}

	len  = sizeof(roomid);
	ret = get_current_node_value(roomid, len, parse_xml_info->pdoc,  pnode_room_id_xml);

	if(ret < 0) {
		PRINTF("get roomid value failed\n");
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_quality_xml, pnode_strm_req_xml, "Quality") == NULL) {
		PRINTF("get Quality failed\n");
		return -1;
	}

	len = sizeof(quality);
	ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  pnode_quality_xml);

	if(ret < 0) {
		PRINTF("get roomid value failed\n");
		return -1;
	}

	PRINTF("index:%d pos=%d,roomid=%s,quality=%s\n", index, pos, roomid, quality);

	if(atoi(quality) == 0 || atoi(quality) == 1) {
		SET_PARSE_LOW_RATE_FLAG(index, pos, atoi(quality));
	}

#ifdef SUPPORT_IP_MATRIX

	if(strcmp(passkey, "Decode") == 0) {
		if(get_response_node(&tmp_pnode_xml, pnode_strm_req_xml, "EncodeIndex") == NULL) {
			PRINTF("get Quality failed\n");
			return -1;
		}

		ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  tmp_pnode_xml);

		if(ret < 0) {
			PRINTF("get roomid value failed\n");
			return -1;
		}

		if(get_response_node(&tmp_pnode_xml, pnode_strm_req_xml, "Ipaddr") == NULL) {
			PRINTF("get Quality failed\n");
			return -1;
		}

		ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  tmp_pnode_xml);

		if(ret < 0) {
			PRINTF("get roomid value failed\n");
			return -1;
		}

		if(get_response_node(&tmp_pnode_xml, pnode_strm_req_xml, "Port") == NULL) {
			PRINTF("get Quality failed\n");
			return -1;
		}

		ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  tmp_pnode_xml);

		if(ret < 0) {
			PRINTF("get roomid value failed\n");
			return -1;
		}

		if(get_response_node(&tmp_pnode_xml, pnode_strm_req_xml, "StreamType") == NULL) {
			PRINTF("get Quality failed\n");
			return -1;
		}

		ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  tmp_pnode_xml);

		if(ret < 0) {
			PRINTF("get roomid value failed\n");
			return -1;
		}

		if(get_response_node(&tmp_pnode_xml, pnode_strm_req_xml, "Name") == NULL) {
			PRINTF("get Quality failed\n");
			return -1;
		}

		ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  tmp_pnode_xml);

		if(ret < 0) {
			PRINTF("get roomid value failed\n");
			return -1;
		}
	}

#endif

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}



//解析设置高低质量xml信令

int parse_xml_set_quality_msg(int index , int pos, parse_xml_t *parse_xml_info, RATE_INFO *rate_info, int *roomId, char *user_id)
{
	//PRINTF("rate_info=%p\n",tmp_rate_info);
	xmlNodePtr pnode_room_info_req_xml = NULL;
	xmlNodePtr pnode_room_info_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	xmlNodePtr pnode_enc_info_xml = NULL;
	xmlNodePtr pnode_enc_id_xml = NULL;
	xmlNodePtr pnode_quality_info_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_tmp_xml = NULL;
	//RATE_INFO rate_info;

	int  quality_num = 0;
	char roomid[8] = {0};
	char tmpbuf[64] = {0};
	int  idx = 0;

	getXmlVideoParams(index, &rate_info[0]);
	PRINTF("old high rate info:\n num=%d,type=%d,bitrate=%d,width=%d,height=%d,frame=%d\n", rate_info[0].rateInfoNum, rate_info[0].rateType
	       , rate_info[0].nBitrate, rate_info[0].nWidth, rate_info[0].nHeight, rate_info[0].nFrame);
	PRINTF("old low rate info:\n num=%d,type=%d,bitrate=%d,width=%d,height=%d,frame=%d\n", rate_info[0].rateInfoNum, rate_info[1].rateType
	       , rate_info[1].nBitrate, rate_info[1].nWidth, rate_info[1].nHeight, rate_info[1].nFrame);

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_info_req_xml, pnode_msg_body_xml, "SetRoomInfoReq") == NULL) {
		PRINTF("set room info req failed\n");
		return -1;
	}

	if(get_response_node(&pnode_room_info_xml, pnode_room_info_req_xml, "RoomInfo") == NULL) {
		PRINTF("RoomInfo  failed\n");
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_room_info_xml, "RoomID") == NULL) {
		PRINTF("RoomID failed\n");
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("RoomID  value  failed\n");
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_enc_info_xml, pnode_room_info_xml, "EncInfo") == NULL) {
		PRINTF("EncInfo  failed\n");
		return -1;
	}

	if(get_response_node(&pnode_enc_id_xml, pnode_enc_info_xml, "ID") == NULL) {
		PRINTF("ID failed\n");
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc, pnode_enc_id_xml) < 0) {
		PRINTF("ID   value failed\n");
		return -1;
	}

	if(get_response_node(&pnode_quality_info_xml, pnode_enc_info_xml, "QualityInfo") == NULL) {
		PRINTF("QualityInfo failed\n");
		return -1;
	}

	quality_num = get_current_samename_node_nums(pnode_quality_info_xml);
	PRINTF("quality =%d\n", quality_num);

	for(idx = 0; idx < quality_num; idx++) {
#if 1
		rate_info[idx].rateInfoNum = quality_num;

		//memset(&rate_info,0,sizeof(RATE_INFO));
		if(get_response_node(&pnode_tmp_xml, pnode_quality_info_xml, "RateType") != NULL) {
			memset(tmpbuf, 0, sizeof(tmpbuf));

			if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
				PRINTF("RateType value failed\n");
				//return -1;
			}

			if(tmpbuf[0] == '0' || tmpbuf[0] == '1') {
				rate_info[idx].rateType = atoi(tmpbuf);
			}
		}

		if(get_response_node(&pnode_tmp_xml, pnode_quality_info_xml, "EncBitrate") != NULL) {
			memset(tmpbuf, 0, sizeof(tmpbuf));

			if(get_current_node_value(tmpbuf, 64, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
				PRINTF("EncBitrate value failed\n");
				//return -1;
			}

			if(strlen(tmpbuf) != 0) {
				rate_info[idx].nBitrate = atoi(tmpbuf);
			}
		}


		if(get_response_node(&pnode_tmp_xml, pnode_quality_info_xml, "EncWidth") != NULL) {
			memset(tmpbuf, 0, sizeof(tmpbuf));

			if(get_current_node_value(tmpbuf, 64, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
				PRINTF("EncWidth value failed\n");
				//return -1;
			}

			if(strlen(tmpbuf) != 0) {
				rate_info[idx].nWidth = atoi(tmpbuf);
			}
		}


		if(get_response_node(&pnode_tmp_xml, pnode_quality_info_xml, "EncHeight") != NULL) {
			memset(tmpbuf, 0, sizeof(tmpbuf));

			if(get_current_node_value(tmpbuf, 64, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
				PRINTF("EncHeight value failed\n");
				//return -1;
			}

			if(strlen(tmpbuf) != 0) {
				rate_info[idx].nHeight = atoi(tmpbuf);
			}
		}


		if(get_response_node(&pnode_tmp_xml, pnode_quality_info_xml, "EncFrameRate") != NULL) {
			memset(tmpbuf, 0, sizeof(tmpbuf));

			if(get_current_node_value(tmpbuf, 64, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
				PRINTF("EncFrameRate value failed\n");
				//return -1;
			}

			if(strlen(tmpbuf) != 0) {
				rate_info[idx].nFrame = atoi(tmpbuf);
			}
		}

#endif

		PRINTF("idx =%d,type=%d,bitrate=%d,width=%d,height=%d,frame=%d\n", idx, rate_info[idx].rateType
		       , rate_info[idx].nBitrate, rate_info[idx].nWidth, rate_info[idx].nHeight, rate_info[idx].nFrame);
		pnode_quality_info_xml = msg_get_next_samename_node(pnode_quality_info_xml);
		PRINTF("pnode_quality_info_xml = %p\n", pnode_quality_info_xml);
	}

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}

//解析设置音频xml信令

int parse_xml_set_audio_info_msg(int index, int pos, parse_xml_t *parse_xml_info, AUDIO_PARAM *audio_info, int *roomId, char *user_id)
{

	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_room_audio_xml = NULL;
	xmlNodePtr pnode_room_info_xml = NULL;
	xmlNodePtr pnode_audio_info_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	xmlNodePtr pnode_tmp_xml = NULL;

	char roomid[8] = {0};
	char tmpbuf[8] = {0};

	getXmlAudioParams(index, audio_info);

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_audio_xml, pnode_msg_body_xml, "SetRoomAudioReq") == NULL) {
		PRINTF("SetRoomAudioReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_info_xml, pnode_room_audio_xml, "RoomInfo") == NULL) {
		PRINTF("RoomInfo failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_room_info_xml, "RoomID") == NULL) {
		PRINTF("RoomID failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("get RoomID value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_audio_info_xml, pnode_room_info_xml, "AudioInfo") == NULL) {
		PRINTF("AudioInfo failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		//return -1;
	}

	if(get_response_node(&pnode_tmp_xml, pnode_audio_info_xml, "InputMode") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
			PRINTF("get InputMode value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(atoi(tmpbuf) == 0 || atoi(tmpbuf) == 1) {
			audio_info->InputMode = atoi(tmpbuf);
		}
	}


	if(get_response_node(&pnode_tmp_xml, pnode_audio_info_xml, "SampleRate") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
			PRINTF("get SampleRate value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(atoi(tmpbuf) <= 3 &&  atoi(tmpbuf) >= 0) {
			audio_info->SampleRate = atoi(tmpbuf);
		}
	}

	if(get_response_node(&pnode_tmp_xml, pnode_audio_info_xml, "Bitrate") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
			PRINTF("get Bitrate value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(atoi(tmpbuf) <= 128000 && atoi(tmpbuf) >= 0) {
			audio_info->BitRate = atoi(tmpbuf);
		}

	}

	if(get_response_node(&pnode_tmp_xml, pnode_audio_info_xml, "Lvolume") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
			PRINTF("get Lvolume value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(atoi(tmpbuf) >= 0 && atoi(tmpbuf) <= 100) {
			audio_info->LVolume = atoi(tmpbuf);
		}
	}

	if(get_response_node(&pnode_tmp_xml, pnode_audio_info_xml, "Rvolume") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
			PRINTF("get Rvolume value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(atoi(tmpbuf) >= 0 && atoi(tmpbuf) <= 100) {
			audio_info->RVolume = atoi(tmpbuf);
		}

	}

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	PRINTF("client[%d] mode=%d,sample=%d,bitrate=%d,lv=%d,rv=%d\n", pos, audio_info->InputMode,
	       audio_info->SampleRate, audio_info->BitRate, audio_info->LVolume, audio_info->RVolume);
	return 0;
}

//解析获取教室信息xml信令
int parse_xml_get_room_info_msg(int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_room_info_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}


	if(get_response_node(&pnode_room_info_xml, pnode_msg_body_xml, "RoomInfoReq") == NULL) {
		PRINTF("RoomInfoReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_room_info_xml, "RoomID") == NULL) {
		PRINTF("RoomID failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("get RoomID value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}


int parse_xml_sys_update_msg(parse_xml_t *parse_xml_info, xmlNodePtr pnode_msg_body_xml)
{
	xmlNodePtr pnode_operate_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};
	get_response_node(&pnode_operate_xml, pnode_msg_body_xml, "Operate");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);

	get_response_node(&pnode_room_id_xml, pnode_operate_xml, "Length");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);

	get_response_node(&pnode_room_id_xml, pnode_operate_xml, "Context");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);

	return 0;
}


int parse_xml_upload_logo_msg(parse_xml_t *parse_xml_info, xmlNodePtr pnode_msg_body_xml)
{
	xmlNodePtr pnode_upload_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};
	get_response_node(&pnode_upload_xml, pnode_msg_body_xml, "UploadLogReq");

	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "RoomID");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "EncodeIndex");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "Format");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "Transparency");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "LogoLength");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);

	return 0;
}


int parse_xml_add_title_msg(parse_xml_t *parse_xml_info, xmlNodePtr pnode_msg_body_xml)
{
	xmlNodePtr pnode_upload_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};
	get_response_node(&pnode_upload_xml, pnode_msg_body_xml, "UploadLogReq");

	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "RoomID");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "EncodeIndex");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "Title");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "Time");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);

	return 0;
}

//解析获取音量大小xml
int parse_xml_get_vol_msg(int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_volume_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_volume_xml, pnode_msg_body_xml, "GetVolumeReq") == NULL) {
		PRINTF("GetVolumeReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_volume_xml, "RoomID") == NULL) {
		PRINTF("RoomID failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("get RoomID value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}

//解析静音xml
int parse_xml_mute_msg(int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_volume_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_volume_xml, pnode_msg_body_xml, "MuteReq") == NULL) {
		PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_volume_xml, "RoomID") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("get RoomID value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}

int parse_xml_pic_rev_msg(int pos, parse_xml_t *parse_xml_info, PICTURE_ADJUST *pic_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_msg_body_xml, "RoomID") == NULL) {
		PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_room_id_xml, pnode_msg_body_xml, "EncodeIndex") == NULL) {
		PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	memset(roomid, 0, sizeof(roomid));

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_msg_body_xml, "Hporch") != NULL) {
		memset(roomid, 0, sizeof(roomid));

		if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
			PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(strlen(roomid) != 0) {
			pic_info->hporch = atoi(roomid);
		}
	}

	if(get_response_node(&pnode_room_id_xml, pnode_msg_body_xml, "Vporch") != NULL) {
		memset(roomid, 0, sizeof(roomid));

		if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
			PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(strlen(roomid) != 0) {
			pic_info->vporch = atoi(roomid);
		}
	}

	if(get_response_node(&pnode_room_id_xml, pnode_msg_body_xml, "ColorTrans") != NULL) {
		memset(roomid, 0, sizeof(roomid));

		if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
			PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(strlen(roomid) != 0) {
			pic_info->color_trans = atoi(roomid);
		}
	}

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	PRINTF("h=%d,v=%d,col=%d\n", pic_info->hporch, pic_info->vporch, pic_info->color_trans);

	return 0;
}

//解析请求I帧xml
int parse_xml_request_Iframe_msg(int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_IFrame_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};


	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}


	if(get_response_node(&pnode_IFrame_xml, pnode_msg_body_xml, "IFrameReq") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_IFrame_xml, "RoomID") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_room_id_xml, pnode_IFrame_xml, "EncodeIndex") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}

//新版海外录播解析锁定分辨率xml
int parse_fix_resolution_msg(int index, int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_rec_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[32] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_rec_xml, pnode_msg_body_xml, "RecCtrlReq") == NULL) {
		PRINTF("RecCtrlReq  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "RoomID") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("RoomID  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "OptType") == NULL) {
		PRINTF("OptType   failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	memset(roomid, 0, sizeof(roomid));

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("OptType  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if((atoi(roomid) == 0) || (atoi(roomid) == 1) || (atoi(roomid) == 2)) {
		SET_FIX_RESOLUTION_FLAG(index, pos, atoi(roomid));
		set_fix_resolution(atoi(roomid));
	}

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}

#ifdef SUPPORT_IP_MATRIX

//解析停止码流xml

int parse_xml_stop_rate_msg(int pos, parse_xml_t *parse_xml_info, char *user_id, int *roomId, char *passkey)
{
	xmlNodePtr pnode_strm_req_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	xmlNodePtr pnode_quality_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr tmp_pnode_xml = NULL;
	int ret = 0;
	char roomid[8] = {0};
	char quality[8] = {0};
	int len = 0;

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_strm_req_xml, pnode_msg_body_xml, "StrmReq") == NULL) {
		PRINTF("get strmreq failed\n");
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_strm_req_xml, "RoomID") == NULL) {
		PRINTF("get ROOMID failed\n");
		return -1;
	}

	len  = sizeof(roomid);
	ret = get_current_node_value(roomid, len, parse_xml_info->pdoc,  pnode_room_id_xml);

	if(ret < 0) {
		PRINTF("get roomid value failed\n");
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_quality_xml, pnode_strm_req_xml, "Quality") == NULL) {
		PRINTF("get Quality failed\n");
		return -1;
	}

	len = sizeof(quality);
	ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  pnode_quality_xml);

	if(ret < 0) {
		PRINTF("get roomid value failed\n");
		return -1;
	}

	PRINTF("pos=%d,roomid=%s,quality=%s\n", pos, roomid, quality);

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}


//解析获取系统信息xml
int parse_xml_get_system_msg(parse_xml_t *parse_xml_info, user_login_info *login_info)
{
	xmlNodePtr pnode_username_xml = NULL;
	xmlNodePtr pnode_password_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	int ret = 0;
	int len = 0;


	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_username_xml, pnode_msg_body_xml, "UserName") == NULL) {
		PRINTF("pnode_username_xml=%p\n", pnode_username_xml);
		return -1;
	}

	len = sizeof(login_info->username);
	ret = get_current_node_value(login_info->username, len, parse_xml_info->pdoc,  pnode_username_xml);

	if(ret < 0) {
		PRINTF("get username failed\n");
		return -1;
	}

	if(get_response_node(&pnode_password_xml, pnode_msg_body_xml, "Password") == NULL) {
		PRINTF("pnode_password_xml=%p\n", pnode_password_xml);
		return -1;
	}

	len = sizeof(login_info->password);
	ret = get_current_node_value(login_info->password, len, parse_xml_info->pdoc,  pnode_password_xml);

	if(ret < 0) {
		PRINTF("get password failed\n");
		return -1;
	}

	PRINTF("username=%s,password=%s\n", login_info->username, login_info->password);
	return 0;
}

//解析音视频，分辨率参数


int parse_lock_av_param_msg(int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id, LOCK_PARAM *lock, int *lock_count)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_rec_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[32] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_rec_xml, pnode_msg_body_xml, "RecCtrlReq") == NULL) {
		PRINTF("RecCtrlReq  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "RoomID") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("RoomID  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "VideoLock") == NULL) {
		PRINTF("OptType   failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	memset(roomid, 0, sizeof(roomid));

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("OptType  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(atoi(roomid) == 0 || atoi(roomid) == 1) {
		lock->video_lock = atoi(roomid);
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "AudioLock") == NULL) {
		PRINTF("OptType   failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	memset(roomid, 0, sizeof(roomid));

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("OptType  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(atoi(roomid) == 0 || atoi(roomid) == 1) {
		lock->audio_lock = atoi(roomid);
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "RelutionLock") == NULL) {
		PRINTF("OptType   failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	memset(roomid, 0, sizeof(roomid));

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("OptType  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(atoi(roomid) == 0 || atoi(roomid) == 1) {
		lock->resulotion_lock = atoi(roomid);
	}

	if(parse_xml_user_id_msg(parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	PRINTF("audio_lock=%d,video_lock=%d,resolution_lock=%d\n", lock->audio_lock, lock->video_lock, lock->resulotion_lock);

	if(lock->audio_lock == 1 && lock->video_lock == 1 && lock->resulotion_lock == 1) {
		(*lock_count)++;

	} else if(lock->audio_lock == 0 && lock->video_lock == 0 && lock->resulotion_lock == 0) {
		(*lock_count)--;

		if(*lock_count < 0) {
			*lock_count = 0;
		}
	} else
		;

	PRINTF("cli [%d] lock %d counts \n", pos, *lock_count);

	return 0;
}

#endif

//解析并处理xml信令中MsgBody中的内容

int parse_xml_msgbody_msg(int index, int pos, parse_xml_t *parse_xml_info, int msg_code, char *passkey, char *user_id)
{
	nslog(NS_INFO, "pos:[%d] msgcode:[%d] passkey:[%s]\n", pos, msg_code, passkey);

	xmlNodePtr pnode_msg_body_xml = NULL;
	user_login_info login_info;
	RATE_INFO  rate_info[2];
	AUDIO_PARAM audio_info;
	PICTURE_ADJUST pic_info;
	char send_buf[1024] = {0};
	char tmpbuf[512] = {0};
	int roomid = 0;
	int sockfd = GETSOCK_NEW(index, pos);
	int count = 0;

	switch(msg_code) {
			//login info
		case NEW_MSG_TCP_LOGIN: {
			nslog(NS_WARN, "NEW_MSG_TCP_LOGIN\n");
			memset(&login_info, 0, sizeof(login_info));

			if(parse_xml_login_msg(parse_xml_info, &login_info) < 0) {
				nslog(NS_ERROR, "parse login failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_TCP_LOGIN, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_xml_login_msg(index, pos, send_buf, &login_info, NEW_MSG_TCP_LOGIN, passkey, user_id);
		}
		break;

		case NEW_MSG_REQUEST_RATE: {
			nslog(NS_WARN, "NEW_MSG_REQUEST_RATE pos=%d,parse request rate msg %p\n", pos, pnode_msg_body_xml);

			if(parse_xml_request_rate_msg(index, pos, parse_xml_info, user_id, &roomid, passkey) < 0) {
				nslog(NS_ERROR, "parse request rate  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_REQUEST_RATE, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_xml_request_rate_msg(index, pos, send_buf, NEW_MSG_REQUEST_RATE, passkey, user_id, &roomid);
		}
		break;


		case NEW_MSG_SET_QUALITY_INFO: {
			nslog(NS_WARN, "NEW_MSG_SET_QUALITY_INFO parse set quality info msg rate_info=%p,&rate_info=%p\n",
			      rate_info, &rate_info);
			memset(rate_info, 0, sizeof(RATE_INFO) * 2);

			if(parse_xml_set_quality_msg(index, pos, parse_xml_info, rate_info, &roomid, user_id) < 0) {
				nslog(NS_ERROR, "parse quality rate  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_SET_QUALITY_INFO, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_set_quality_cmd(index, pos, send_buf, rate_info, NEW_MSG_SET_QUALITY_INFO, passkey, user_id, &roomid);
		}
		break;
#if 1

		case NEW_MSG_SET_AUDIO: {
			nslog(NS_WARN, "parse set audio info msg\n");
			memset(&audio_info, 0, sizeof(AUDIO_PARAM));

			if(parse_xml_set_audio_info_msg(index, pos, parse_xml_info, &audio_info, &roomid, user_id) < 0) {
				nslog(NS_ERROR, "parse NEW_MSG_SET_AUDIO  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_SET_AUDIO, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_set_audio_cmd(index, pos, send_buf, &audio_info, NEW_MSG_SET_AUDIO, passkey, user_id, &roomid);
		}
		break;

		case NEW_MSG_GET_ROOM_INFO: {
			nslog(NS_WARN, "parse get room info msg\n");

			if(parse_xml_get_room_info_msg(pos, parse_xml_info, &roomid, user_id) < 0) {
				nslog(NS_ERROR, "parse NEW_MSG_GET_ROOM_INFO  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_GET_ROOM_INFO, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_get_room_info_cmd(index, pos, send_buf, NEW_MSG_GET_ROOM_INFO, passkey, user_id, &roomid);
		}
		break;

		case NEW_MSG_GET_VOLUME: {
			nslog(NS_WARN, "NEW_MSG_GET_VOLUME\n");
#if 0

			if(parse_xml_get_vol_msg(pos, parse_xml_info, &roomid, user_id) < 0) {
				nslog(NS_ERROR, "parse get_vol_msg  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_GET_VOLUME, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_volume_cmd(index, pos, send_buf, NEW_MSG_GET_VOLUME, passkey, user_id, &roomid);
#endif
		}
		break;

		case NEW_MSG_MUTE: {
			nslog(NS_WARN, "NEW_MSG_MUTE\n");

			if(parse_xml_mute_msg(pos, parse_xml_info, &roomid, user_id) < 0) {
				nslog(NS_ERROR, "parse parse_xml_mute_msg  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_MUTE, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_mute_cmd(index, pos, send_buf, NEW_MSG_MUTE, passkey, user_id, &roomid);
		}
		break;

		case NEW_MSG_PIC_REVISE: {
			nslog(NS_WARN, "NEW_MSG_PIC_REVISE\n");
			memset(&pic_info, 0, sizeof(PICTURE_ADJUST));

			if(parse_xml_pic_rev_msg(pos, parse_xml_info, &pic_info, &roomid, user_id) < 0) {
				nslog(NS_ERROR, "parse get_vol_msg  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_PIC_REVISE, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_pic_adjust_cmd(index, pos, send_buf, &pic_info, NEW_MSG_PIC_REVISE, passkey, user_id, &roomid);
		}
		break;

		case NEW_REQUEST_IFRAME: {
			nslog(NS_WARN, "NEW_REQUEST_IFRAME\n");

			if(parse_xml_request_Iframe_msg(pos, parse_xml_info, &roomid, user_id) < 0) {
				nslog(NS_ERROR, "parse REQUEST_IFRAME  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_REQUEST_IFRAME, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_xml_request_IFrame_msg(index, pos, send_buf, NEW_REQUEST_IFRAME, passkey, user_id, &roomid);
		}
		break;

#endif

		case NEW_MSG_STATUS_REPORT: {
			nslog(NS_WARN, "NEW_MSG_STATUS_REPORT\n");
			count = cut_heart_count(index, pos);
			//PRINTF("client [%d] parse NEW_MSG_STATUS_REPORT cut heart count = %d\n",pos,count);

		}
		break;

		case NEW_MSG_ENC_VER_REQ: {
			nslog(NS_WARN, "NEW_MSG_ENC_VER_REQ\n");

		}
		break;

		default: {
			nslog(NS_WARN, "unknown xml  msg\n");
		}
		break;


	}

	return 0;
}


int parse_cli_xml_msg(int index, int pos, char *buffer, int *msgCode, char *passkey, char *user_id)
{
	nslog(NS_WARN, "\n[%s]\n", buffer);
	parse_xml_t *parse_xml_info = (parse_xml_t *)malloc(sizeof(parse_xml_t));
	xmlNodePtr pnode_msgcode_xml = NULL;
	xmlNodePtr pnode_passkey_xml = NULL;
	char tmpbuf[64] = {0};
	int ret = 0;
	int len = 0;
	unsigned long t1, t2, t3;

	if(init_dom_tree(parse_xml_info, buffer) == NULL) {
		PRINTF("[init_dom_tree] is error!\n");
		ret = -1;
		goto EXIT;
	}

	if(is_request_msg(parse_xml_info->proot) != 1) {
		PRINTF("request_msg != 1\n");
		ret = -1;
		goto EXIT;
	}

	if(get_xml_msgcode(&pnode_msgcode_xml, parse_xml_info->proot) == NULL) {
		PRINTF("get xml msgcode failed\n");
		ret = -1;
		goto EXIT;
	}

	len = sizeof(tmpbuf);

	if(get_current_node_value(tmpbuf, len, parse_xml_info->pdoc,  pnode_msgcode_xml) != 0) {
		PRINTF("get xml msgcode value failed\n");
		ret = -1;
		goto EXIT;
	}

	if(strlen(tmpbuf) != 0) {
		*msgCode = atoi(tmpbuf);
	}

	if(get_xml_passkey(&pnode_passkey_xml, parse_xml_info->proot) == NULL) {
		PRINTF("get xml passkey failed\n");
		ret = -1;
		goto EXIT;
	}

	len = sizeof(tmpbuf);
	memset(tmpbuf, 0, sizeof(tmpbuf));

	if(get_current_node_value(tmpbuf, len, parse_xml_info->pdoc,  pnode_passkey_xml) != 0) {
		PRINTF("get xml passkey value failed\n");
		ret = -1;
		goto EXIT;
	}

	if(strlen(tmpbuf) != 0) {
		strcpy(passkey, tmpbuf);
	}

#ifdef SUPPORT_IP_MATRIX

	if(strcmp(passkey, "IPMTSServer") == 0) {
		SET_PASSKEY_FLAG(index, pos, 1);
	}

#endif

	if(parse_xml_msgbody_msg(index, pos, parse_xml_info, *msgCode, passkey, user_id) < 0) {
		PRINTF("parse xml msgbody failed\n");
		ret = -1;
		goto EXIT;
	}

EXIT:

	if(parse_xml_info->pdoc != NULL) {
		release_dom_tree(parse_xml_info->pdoc);
		free(parse_xml_info);
		parse_xml_info = NULL;
	}

	return ret;

}



int init_av_change_param_mutex(int index)
{
	pthread_mutex_init(&g_audio_param_mutex[index], NULL);
	pthread_mutex_init(&g_video_param_mutex[index], NULL);
	return 0;
}

void set_audio_param_change_flag(int index, int flag)
{
	pthread_mutex_lock(&g_audio_param_mutex[index]);
	g_audio_param_change[index] = flag;
	pthread_mutex_unlock(&g_audio_param_mutex[index]);
}

int get_audio_param_change_flag(int index)
{
	int flag  =   -1;
	pthread_mutex_lock(&g_audio_param_mutex[index]);
	flag  = g_audio_param_change[index];
	pthread_mutex_unlock(&g_audio_param_mutex[index]);
	return flag;
}

void set_video_param_change_flag(int index, int flag)
{
	pthread_mutex_lock(&g_video_param_mutex[index]);
	g_video_param_change[index] = flag;
	pthread_mutex_unlock(&g_video_param_mutex[index]);
}

int get_video_param_change_flag(int index)
{
	int flag  =   -1;
	pthread_mutex_lock(&g_video_param_mutex[index]);
	flag  = g_video_param_change[index];
	pthread_mutex_unlock(&g_video_param_mutex[index]);
	return flag;
}


static  int  g_low_rate_flag[2] = {0};

void set_low_rate_flag(int index, int flag)
{
	g_low_rate_flag[index] = flag;
}

int get_low_rate_flag(int index)
{
	return g_low_rate_flag[index];
}

int package_login_add_xml_leaf(char *login_val, xmlNodePtr child_node, xmlNodePtr far_node, char *key_name)
{
	char tmpbuf[64] = {0};
	memset(tmpbuf, 0, sizeof(tmpbuf));
	strcpy(tmpbuf, login_val);
	package_add_xml_leaf(child_node, far_node, (char *)key_name, (char *)tmpbuf);
	return 0;
}

int package_login_response_msg(int index, char *send_buf, int msg_code_val, char *passkey, char *return_val, char *user_id)
{

	char msgCode[8] = {0};
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"ResponseMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr code_node 			= NULL;
	xmlNodePtr return_node 			= NULL;
	xmlNodePtr name_node 			= NULL;
	xmlNodePtr serialnum_node 			= NULL;
	xmlNodePtr mac_node 			= NULL;
	xmlNodePtr ip_node 			= NULL;
	xmlNodePtr gateway_node 			= NULL;
	xmlNodePtr netmask_node 			= NULL;
	xmlNodePtr device_ver_node 			= NULL;
	xmlNodePtr channel_node 			= NULL;
	xmlNodePtr ppt_index_node 			= NULL;
	xmlNodePtr lowrate_node 			= NULL;
	xmlNodePtr encodetype_node 			= NULL;
	xmlNodePtr passekey_node 			= NULL;
#ifdef SUPPORT_IP_MATRIX
	xmlNodePtr videorate_node 			= NULL;
	xmlNodePtr devicetype_node 			= NULL;
#endif
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	char tmpbuf[64] = {0};
	char *src = NULL;
	struct in_addr ipaddr;
	SYSTEMINFO   *info = malloc(sizeof(SYSTEMINFO));
	memset(info, 0, sizeof(SYSTEMINFO));
	get_system_info(index, info);
	sprintf(msgCode, "%d", msg_code_val);
	package_add_xml_leaf(code_node, head_node, (char *)"MsgCode", (char *)msgCode);
	package_add_xml_leaf(passekey_node, head_node, (char *)"PassKey", (char *)passkey);
	package_add_xml_leaf(return_node, head_node, (char *)"ReturnCode", (char *)return_val);
	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);
	package_add_xml_leaf(name_node, body_node, (char *)"Name", (char *)info->deviceType);
	PRINTF("info->strName=[%s]\n", info->strName);
	package_login_add_xml_leaf(info->strName, serialnum_node, body_node, (char *)"SerialNum");
	src = info->szMacAddr;
	sprintf(tmpbuf, "%02x:%02x:%02x:%02x:%02x:%02x", src[0],
	        src[1], src[2], src[3], src[4], src[5]);
	PRINTF("mac addr  = %s \n", tmpbuf);
	package_login_add_xml_leaf(tmpbuf, mac_node, body_node, (char *)"MacAddr");
	memcpy(&ipaddr, &(info->dwAddr), 4);
	package_login_add_xml_leaf(inet_ntoa(ipaddr), ip_node, body_node, (char *)"IPAddr");
	memcpy(&ipaddr, &(info->dwGateWay), 4);
	package_login_add_xml_leaf(inet_ntoa(ipaddr), gateway_node, body_node, (char *)"GateWay");
	memcpy(&ipaddr, &(info->dwNetMark), 4);
	package_login_add_xml_leaf(inet_ntoa(ipaddr), netmask_node, body_node, (char *)"NetMask");
	package_login_add_xml_leaf(info->strVer, device_ver_node, body_node, (char *)"DeviceVersion");
	sprintf(tmpbuf, "%d", info->channelNum);
	package_login_add_xml_leaf(tmpbuf, channel_node, body_node, (char *)"ChannelNum");
	sprintf(tmpbuf, "%d", info->pptIndex);
	package_login_add_xml_leaf(tmpbuf, ppt_index_node, body_node, (char *)"PPTIndex");
	sprintf(tmpbuf, "%d", info->lowRate);
	package_login_add_xml_leaf(tmpbuf, lowrate_node, body_node, (char *)"LowRate");
	sprintf(tmpbuf, "%d", info->encodeType);
	package_login_add_xml_leaf(tmpbuf, encodetype_node, body_node, (char *)"EncodeType");

	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	printf("%s\n", (char *)temp_xml_buf);
	memcpy(send_buf, temp_xml_buf, size);
	//printf("%s\n",send_buf);
	xmlFree(temp_xml_buf);
	free(info);
	release_dom_tree(doc);
	return 0;
}

int package_room_info_msg(char *send_buf, AUDIO_PARAM *audio, RATE_INFO *high_rate, int msgcode, char *passkey, int retcode, char *user_id, int roomid)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, ROOM_INFO_XML_NO_USEID, passkey, msgcode, retcode, roomid, 1, 1, "xxx", audio->InputMode, audio->SampleRate,
		        audio->BitRate, audio->LVolume, audio->RVolume, 1, "x.x.x.x", 1, high_rate->rateType, high_rate->nBitrate,
		        high_rate->nWidth, high_rate->nHeight, high_rate->nFrame);
	} else {
		sprintf(send_buf, ROOM_INFO_XML, passkey, msgcode, retcode, roomid, 1, 1, "xxx", audio->InputMode, audio->SampleRate,
		        audio->BitRate, audio->LVolume, audio->RVolume, 1, "x.x.x.x", 1, high_rate->rateType, high_rate->nBitrate,
		        high_rate->nWidth, high_rate->nHeight, high_rate->nFrame, atoi(user_id));
	}

	return 0;
}
int package_get_volume(char *send_buf, int msgcode, char *passkey, int retcode, int vol, char *user_id, int roomid)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, PACK_GET_VOL_XML_NO_USEID, msgcode, passkey, retcode, roomid, vol);
	} else {
		sprintf(send_buf, PACK_GET_VOL_XML, msgcode, passkey, retcode, roomid, vol, atoi(user_id));
	}

	return 0;
}
int package_xml_mute_msg(char *send_buf, int msgcode, char *passkey, int retcode, char *user_id, int roomid)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, PACK_MUTE_XML_NO_USEID, msgcode, passkey, retcode, roomid);
	} else {
		sprintf(send_buf, PACK_MUTE_XML, msgcode, passkey, retcode, roomid, atoi(user_id));
	}

	return 0;
}
int package_xml_request_I_frame_msg(char *send_buf, int msgcode, char *passkey, int retcode, char *user_id, int roomid)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, PACK_REQUEST_IFRAME_XML_NO_USEID, msgcode, passkey, retcode, roomid);
	} else {
		sprintf(send_buf, PACK_REQUEST_IFRAME_XML, msgcode, passkey, retcode, roomid, atoi(user_id));
	}

	return 0;
}
int package_xml_pic_adjust_msg(char *send_buf, int msgcode, char *passkey, int retcode, char *user_id, int roomid)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, PACK_PIC_ADJUST_XML_NO_USEID, msgcode, passkey, retcode, roomid);
	} else {
		sprintf(send_buf, PACK_PIC_ADJUST_XML , msgcode, passkey, retcode, roomid, atoi(user_id));
	}

	return 0;
}


int package_xml_report_msg(char *send_buf, int msgcode, char *passkey, int status, int mute, char *user_id)
{
	sprintf(send_buf, PACK_REPORT_XML , msgcode, passkey, status, mute);
	return 0;
}

int package_xml_fix_resolution_msg(char *send_buf, int msgcode, char *passkey, int retcode, int roomid, int result, char *user_id)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, PACK_FIX_RESOLUTION_XML_NO_USEID, msgcode, passkey, retcode, roomid, result);
	} else {
		sprintf(send_buf, PACK_FIX_RESOLUTION_XML , msgcode, passkey, retcode, roomid, result, atoi(user_id));
	}

	return 0;
}

#ifdef SUPPORT_IP_MATRIX
int package_xml_encode_enable_msg(char *send_buf, int msgcode, char *passkey, int retcode, int roomid, char *user_id)
{
	int max_frame = 60;
	sprintf(send_buf, PACK_ENCODE_ENABLE_XML, msgcode, passkey, retcode, MAX_RESOLUTION_WIDTH, MAX_RESOLUTION_HEIGHT, MAX_VIDEO_BATE, max_frame, MAX_AUDIO_BATE);

	return 0;
}


int package_xml_get_system_msg(int index, char *send_buf, int msgcode, char *passkey, int retcode)
{
	SYSTEMINFO   *info = malloc(sizeof(SYSTEMINFO));
	char tmpbuf[64] = {0};
	char *src = NULL;
	struct in_addr ipaddr, gateway, netmask;
	get_system_info(index, info);
	src = info->szMacAddr;
	sprintf(tmpbuf, "%02x:%02x:%02x:%02x:%02x:%02x", src[0],
	        src[1], src[2], src[3], src[4], src[5]);
	PRINTF("mac addr  = %s \n", tmpbuf);
	memcpy(&ipaddr, &(info->dwAddr), 4);
	memcpy(&gateway, &(info->dwGateWay), 4);
	memcpy(&netmask, &(info->dwNetMark), 4);
	sprintf(send_buf, PACK_GET_SYSTEM_XML, msgcode, passkey, retcode, info->deviceType, info->strName, tmpbuf, inet_ntoa(ipaddr), inet_ntoa(gateway), inet_ntoa(netmask),
	        info->strVer, 20000, 101);

	PRINTF("send_xml=\n%s\n\n", send_buf);
	free(info);
	return 0;
}

#endif


int process_xml_login_msg(int index, int pos, char *send_buf, user_login_info *login_info, int msgCode, char *passkey, char *user_id)
{

	int sockfd = GETSOCK_NEW(index, pos);
	memset(send_buf, 0, sizeof(send_buf));

	if((strcmp(login_info->username, "admin") == 0) &&
	   (strcmp(login_info->password, "123") == 0)) {
		PRINTF("user admin login success\n");
	} else {
		PRINTF("user admin login failed\n");
		package_head_msg(send_buf, msgCode, passkey, "0", user_id);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send login err failed\n");
			return -1;
		}

		return -1;
	}

	package_login_response_msg(index, send_buf, msgCode, passkey, "1", user_id);
	PRINTF("send before\n");

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	PRINTF("send after\n");
	SETCLIUSED_NEW(index, pos, TRUE);
	SETCLILOGIN_NEW(index, pos, TRUE);
	PRINTF("index:[%d] used=%d,login=%d,flag=%d\n",
	       index,
	       ISUSED_NEW(index, pos),
	       ISLOGIN_NEW(index, pos),
	       GETLOWRATEFLAG_NEW(index, pos));

	return 0;
}

int process_xml_request_rate_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd  = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	PRINTF("index:[%d] pos=%d,enter request rate cmd\n", index, pos);
	SET_SEND_AUDIO_NEW(index, pos, TRUE);
	SET_SEND_VIDEO_NEW(index, pos, TRUE);

	package_head_msg(send_buf, msgCode, passkey, "1", user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}

int process_set_quality_cmd(int index, int pos, char *send_buf, RATE_INFO *rate_info, int msgCode, char *passkey, char *user_id, int *roomid)
{
	RATE_INFO *high_rate = NULL;

	int high_rate_flag = 0;
	int sockfd = GETSOCK_NEW(index, pos);
	high_rate = &rate_info[0];

	if(0 == high_rate->rateInfoNum) {
		memset(send_buf, 0, sizeof(send_buf));
		package_head_msg(send_buf, msgCode, passkey, "0", user_id);
		PRINTF("%s\n", send_buf);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send tcp data failed\n");
			return -1;
		}

		return -1;
	} else if(1 == high_rate->rateInfoNum || 2 == high_rate->rateInfoNum) {
		if(0 == high_rate->rateType) {
			setXmlVideoParams(index, high_rate);
		}

	}

	//PRINTF("deal set quality info cmd len=%d\n",sizeof(send_buf));
	memset(send_buf, 0, sizeof(send_buf));
	package_head_msg(send_buf, msgCode, passkey, "1", user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}


int process_set_audio_cmd(int index, int pos, char *send_buf, AUDIO_PARAM *audio_info, int msgCode, char *passkey, char *user_id, int *roomid)
{
	AUDIO_PARAM Newp;
	int ret = 0;
	int sockfd = GETSOCK_NEW(index, pos);

	setXmlAudioParams(index, &Newp);

	package_head_msg(send_buf, msgCode, passkey, "1", user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return ret;
}

int process_volume_cmd(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	int volume = 0;
	AUDIO_PARAM audio;
	getXmlAudioParams(index, &audio);
	volume = 0;
	memset(send_buf, 0, sizeof(send_buf));
	package_get_volume(send_buf, NEW_MSG_GET_VOLUME, passkey, 1, volume, user_id, room_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}

int process_mute_cmd(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd = GETSOCK_NEW(index, pos);
	int mute_flag = -1;
	int room_id = *roomid;

	if(msgCode == NEW_MSG_MUTE) {
		mute_flag = getXmlMuteStatus(index);
		mute_flag ^= 1;
		setXmlMuteStatus(index, mute_flag);
	}

	memset(send_buf, 0, sizeof(send_buf));
	package_xml_mute_msg(send_buf, NEW_MSG_MUTE, passkey, 1, user_id, room_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}


int process_pic_adjust_cmd(int index, int pos, char *send_buf, PICTURE_ADJUST *info, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int             ret = 0;
	int 			sockfd = GETSOCK_NEW(index, pos);
	short           hporch, vporch;
	short 			color_trans = info->color_trans;
	int room_id = *roomid;
	hporch = info->hporch;
	vporch = info->vporch;
	PRINTF("hporch = %d vporch= %d \n", hporch, vporch);
	setXmlVgaHV(hporch, vporch);

	memset(send_buf, 0, sizeof(send_buf));
	package_xml_pic_adjust_msg(send_buf, msgCode, passkey, 1, user_id, room_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}

int process_xml_request_IFrame_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	PRINTF("process request_IFrame msg\n");
	int sockfd = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	memset(send_buf, 0, sizeof(send_buf));
	setXmlForceIframe(index);
	package_xml_request_I_frame_msg(send_buf, msgCode, passkey, 1, user_id, room_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}

int process_fix_resolution_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	int flag = 0;

#ifdef SUPPORT_IP_MATRIX

	if(get_ipmatrix_lock_resolution(index) > 0) {
		package_xml_fix_resolution_msg(send_buf, msgCode, passkey, 0, room_id, GET_FIX_RESOLUTION_FLAG(index, pos), user_id);
		PRINTF("%s\n", send_buf);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send tcp data failed\n");
			return -1;
		}

		return 0;
	}

#endif


	memset(send_buf, 0, sizeof(send_buf));
	package_xml_fix_resolution_msg(send_buf, msgCode, passkey, 1, room_id, GET_FIX_RESOLUTION_FLAG(index, pos), user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}


static int g_audio_resolution[2] = {0};   //  1   加锁     0   解锁

int get_audio_lock_resolution(int index)
{
	return g_audio_resolution[index];
}

int add_audio_lock_resolution(int index)
{
	g_audio_resolution[index] += 1;
	return g_audio_resolution[index];
}

int cut_audio_lock_resolution(int index)
{
	g_audio_resolution[index] -= 1;

	if(g_audio_resolution[index] <= 0) {
		g_audio_resolution[index] = 0;
	}

	return g_audio_resolution[index];
}

static int g_video_resolution[2] = {0, 0};   //  1  加锁     0  解锁

int get_video_lock_resolution(int index)
{
	return g_video_resolution[index];
}

int add_video_lock_resolution(int index)
{
	g_video_resolution[index] += 1;
	return g_video_resolution[index];
}

int cut_video_lock_resolution(int index)
{
	g_video_resolution[index] -= 1;

	if(g_video_resolution[index] <= 0) {
		g_video_param_change[index] = 0;
	}

	return g_video_resolution[index];
}
static int g_ipmatrix_lock_resolution[2] = {0};   //  1  加锁     0  解锁

int get_ipmatrix_lock_resolution(int index)
{
	return g_ipmatrix_lock_resolution[index];
}

int add_ipmatrix_lock_resolution(int index)
{
	g_ipmatrix_lock_resolution[index] += 1;
	return g_ipmatrix_lock_resolution[index];
}

int cut_ipmatrix_lock_resolution(int index)
{
	g_ipmatrix_lock_resolution[index] -= 1;

	if(g_ipmatrix_lock_resolution[index] <= 0) {
		g_ipmatrix_lock_resolution[index] = 0;
	}

	return g_ipmatrix_lock_resolution[index];
}


process_sys_restart_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd = GETSOCK_NEW(index, pos);
	package_head_msg(send_buf, msgCode, passkey, "1", user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	system("sync");
	sleep(4);
	PRINTF("Restart sys\n");
	system("reboot -f");

	return 0;
}



int process_xml_stop_rate_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd  = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	PRINTF("index:[%d] pos=%d,enter request rate cmd\n", index, pos);
	SET_SEND_AUDIO_NEW(index, pos, 0);
	SET_SEND_VIDEO_NEW(index, pos, 0);
	SETLOWRATEFLAG_NEW(index, pos, -1);
	package_head_msg(send_buf, msgCode, passkey, "1", user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}





static int quit_status[2] = {1, 1};
static int get_quit_status(int index)
{
	return quit_status[index];
}

void set_quit_status(int index)
{
	quit_status[index] = 0;
}


void *report_pthread_fxn(void *arg)
{
	PRINTF("[report_pthread_fxn] start ...\n");

	if(NULL == arg) {
		return NULL;
	}

	int mode = 0;
	int audio_flag = 1;
	char send_buf[1024] = {0};
	char device[16] = {0};
	int nPos  = 0;
	int sockfd = -1;
	AUDIO_PARAM Aparam;
	RATE_INFO Rinfo;
	char user_id[16] = {0};
	int i = 0;
	int count = 0;
	int flag = -1;
	int index = *(int *)arg;
	GetDeviceType(device);

	while(get_quit_status(index)) {
		mode = getXmlVideoDetect(index);
		audio_flag = getXmlMuteStatus(index);

		//PRINTF("mode = %d, audio  = %d\n",mode,audio_flag);
		for(i = 0 ; i < MAX_CLIENT_NUM ; i++) {
			//PRINTF("pos :%d,low rate info :GETLOWRATEFLAG_NEW= %d,IISLOWRATESTART_NEW=%d,GET_SEND_VIDEO_NEW=%d,ISUSED_NEW =%d,ISLOGIN_NEW =%d\n",
			//	i,GETLOWRATEFLAG_NEW(i),get_low_rate_flag(),GET_SEND_VIDEO_NEW(i),ISUSED_NEW(i),ISLOGIN_NEW(i));
			if(ISLOGIN_NEW(index, i) && ISUSED_NEW(index, i)) {
				sockfd = GETSOCK_NEW(index, i);

				if(sockfd < 0) {
					printf("sockfd = %d\n", GETSOCK_NEW(index, i));
					sleep(3);
					continue;
				}

				package_xml_report_msg(send_buf, NEW_MSG_STATUS_REPORT, device, mode, audio_flag, user_id);

				//PRINTF("\n================\n%s\n==============\n", send_buf);

				if(tcp_send_data(sockfd, send_buf) < 0) {
					PRINTF("send tcp data failed, socket:[%d] errno=%d,%s\n", sockfd, errno, strerror(errno));
					//return -1;
				} else {
					count = add_heart_count(index, i);
				}

				//PRINTF("client [%d] heart send count = %d\n",i,count);
				if(count > 3) {
					PRINTF("[%d]client heart maybe stop , close cli [%d]\n", i, i);
					{
						SETCLIUSED_NEW(index, i, FALSE);
						SET_SEND_AUDIO_NEW(index, i, FALSE);
						SETCLILOGIN_NEW(index, i, FALSE);
						SET_SEND_VIDEO_NEW(index, i, FALSE);
						SET_FIX_RESOLUTION_FLAG(index, i, FALSE);
						set_heart_count(index, i, FALSE);

						shutdown(sockfd, SHUT_RDWR);
						close(sockfd);
						SETSOCK_NEW(index, i, INVALID_SOCKET);
					}

				}
			}
		}

		sleep(3);
	}


}



static int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	//	pthread_mutex_lock(&gSetP_m.send_m);
	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf + nWriteLen, nSize - nWriteLen, 0);

		if(nRet < 0) {
			WARN_PRN("WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen, errno, s);

			if((errno == ENOBUFS) && (nCount < 10)) {
				fprintf(stderr, "network buffer have been full!\n");
				usleep(10000);
				nCount++;
				continue;
			}

			//			pthread_mutex_unlock(&gSetP_m.send_m);
			return nRet;
		} else if(nRet == 0) {
			fprintf(stderr, "WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen,  errno, s);
			fprintf(stderr, "Send Net Data Error nRet= %d\n", nRet);
			continue;
		}

		nWriteLen += nRet;
		nCount = 0;
	}

	//	pthread_mutex_unlock(&gSetP_m.send_m);
	return nWriteLen;
}


int  SendVideoDataToIpMatrixClient(int index, void *pData, int nLen)
{

	//printf("[SendDataToIpMatrixClient] index:[%d]\n", index);
	int nRet = -1;
	int cnt = 0;
	int	sendsocket = 0;

	//send multi client
	for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
		if(ISUSED_NEW(index, cnt)
		   && ISLOGIN_NEW(index, cnt)
		   && GET_SEND_VIDEO_NEW(index, cnt)) {
			sendsocket = GETSOCK_NEW(index, cnt);

			//printf("[SendDataToIpMatrixClient] sendsocket:[%d]\n", sendsocket);
			if(sendsocket > 0) {
				nRet = WriteData(sendsocket, pData, nLen);

				if(nRet < 0) {
					SETCLIUSED_NEW(index, cnt, FALSE);
					SETCLILOGIN_NEW(index, cnt, FALSE);
					PRINTF("Error: SOCK = %d count = %d  errno[%d]:[%s]  ret = %d\n", sendsocket, cnt, errno, strerror(errno), nRet);
				}
			}
		}
	}

	return 0;
}

int  SendAudioDataToIpMatrixClient(int index, void *pData, int nLen)
{

	//printf("[SendDataToIpMatrixClient] index:[%d]\n", index);
	int nRet = -1;
	int cnt = 0;
	int	sendsocket = 0;

	//send multi client
	for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
		if(ISUSED_NEW(index, cnt)
		   && ISLOGIN_NEW(index, cnt)
		   && GET_SEND_AUDIO_NEW(index, cnt)) {
			sendsocket = GETSOCK_NEW(index, cnt);

			//printf("[SendDataToIpMatrixClient] sendsocket:[%d]\n", sendsocket);
			if(sendsocket > 0) {
				nRet = WriteData(sendsocket, pData, nLen);

				if(nRet < 0) {
					SETCLIUSED_NEW(index, cnt, FALSE);
					SETCLILOGIN_NEW(index, cnt, FALSE);
					PRINTF("Error: SOCK = %d count = %d  errno[%d]:[%s]  ret = %d\n", sendsocket, cnt, errno, strerror(errno), nRet);
				}
			}
		}
	}

	return 0;
}


#endif
