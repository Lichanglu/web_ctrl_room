#ifndef __UPPER_MSG_CTRL_INCLUDE__
#define __UPPER_MSG_CTRL_INCLUDE__

#include <pthread.h>
#include "xml_base.h"
#include "reach_socket.h"

typedef struct MsgHeader {
	uint16_t sLen; 	//���� ����ʶ����xml��Ϣ�ı��ĳ���+��Ϣͷ���ȣ�
	uint16_t sVer; 	//�汾 �̶�Ϊ2012
	uint16_t sMsgType; //��Ϣʶ���롣Ŀǰֻ��ֱ����������Ч���������Ч
	uint16_t sData;	//����
}MsgHeader;

typedef struct UpperMsgEnv {
	int32_t tcp_socket;
	int8_t tcp_ip[IP_LEN];
	int32_t tcp_port;
	
	pthread_mutex_t   tcp_socket_mutex;
} UpperMsgEnv;


//�ϲ���Ϣ�̴߳���
void *upper_msg_process(void *arg);

//��ָ��socket ��������ӿ�
int32_t upper_msg_tcp_send(int32_t socket, pthread_mutex_t *mutex, MsgHeader *head, int8_t *xml);

//��ȡָ��parseXml ��MSG CODE ֵ
int32_t upper_msg_get_msg_code_value(parse_xml_t *parseXml);

//��װ������Ϣ�����
int32_t upper_msg_package_req_enc_info(int32_t room_id, int8_t *out_buf, int32_t msgcode, int8_t *pass_key, int8_t *user_id);

#endif /*__UPPER_MSG_CTRL_INCLUDE__*/

