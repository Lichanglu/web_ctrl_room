#ifndef __UPPER_MSG_CTRL_INCLUDE__
#define __UPPER_MSG_CTRL_INCLUDE__

#include <pthread.h>
#include "xml_base.h"
#include "reach_socket.h"

typedef struct MsgHeader {
	uint16_t sLen; 	//长度 （标识后面xml消息文本的长度+消息头长度）
	uint16_t sVer; 	//版本 固定为2012
	uint16_t sMsgType; //消息识别码。目前只有直播数据中有效，信令处理无效
	uint16_t sData;	//保留
}MsgHeader;

typedef struct UpperMsgEnv {
	int32_t tcp_socket;
	int8_t tcp_ip[IP_LEN];
	int32_t tcp_port;
	
	pthread_mutex_t   tcp_socket_mutex;
} UpperMsgEnv;


//上层消息线程处理
void *upper_msg_process(void *arg);

//往指定socket 发送信令接口
int32_t upper_msg_tcp_send(int32_t socket, pthread_mutex_t *mutex, MsgHeader *head, int8_t *xml);

//获取指定parseXml 的MSG CODE 值
int32_t upper_msg_get_msg_code_value(parse_xml_t *parseXml);

//封装教室信息请求包
int32_t upper_msg_package_req_enc_info(int32_t room_id, int8_t *out_buf, int32_t msgcode, int8_t *pass_key, int8_t *user_id);

#endif /*__UPPER_MSG_CTRL_INCLUDE__*/

