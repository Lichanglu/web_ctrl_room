#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "lower_msg_ctrl.h"
#include "room_ctrl.h"

#include "reach_os.h"
#include "media_msg.h"
#include "stdint.h"
#include "xml_base.h"
#include "xml_msg_management.h"

xmlNodePtr upper_msg_get_next_samename_node(xmlNodePtr curNode);
int32_t upper_msg_package_resp_room_info(RoomMsgEnv *pRoom, int8_t *out_buf, int32_t msgcode, int32_t ret_code, int8_t *pass_key, int8_t *user_id);
int32_t lower_msg_package_iFrame_req(int8_t *out_buf, int32_t room_id);

int32_t lower_msg_que_send(int32_t msqid, msgque *msg, int32_t len, int32_t msgflg)
{
	r_msg_send(msqid, msg, len, msgflg);
}

uint32_t lower_msg_get_sample_by_index(uint32_t index)
{
	uint32_t ret = 44100;

	switch(index) {
		case 0:   //8KHz
		case 1:  //32KHz
		case 2:
			ret = 44100;
			break;

		case 3:
			ret = 48000;
			break;

		default:
			ret = 48000;
			break;
	}

	return (ret);
}

int32_t lower_msg_opt_type_trans(int32_t index)
{
	int32_t ret = 0;

	switch(index) {
		case RECORD_RESUME:
			ret = 1;
			break;
		case RECORD_PAUSE:
			ret = 2;
			break;
		case RECORD_CLOSE:
			ret = 0;
			break;

		default:
			ret = 0;
			break;
	}

	return (ret);
}


int32_t lower_msg_get_leaf_value(char *value, int32_t value_len, xmlChar *key, xmlNodePtr curNode, xmlDocPtr pdoc)
{
	int32_t ret 			= -1;
	xmlNodePtr children_ptr = NULL;

	r_bzero(value, value_len);
	children_ptr = get_children_node(curNode, key);
	if (!children_ptr) {
		return ROOM_RETURN_FAIL;
	}

	ret = get_current_node_value(value, value_len, pdoc, children_ptr);
	if (ret == -1)
		return ROOM_RETURN_FAIL;
	else
		return ROOM_RETURN_SUCC;
}

int32_t lower_msg_get_return_code(parse_xml_t *parseXml)
{
	int8_t value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr  tmp_node = NULL;

	tmp_node = get_children_node(parseXml->proot, MSG_HEAD_KEY);
	if (!tmp_node) {
		return ROOM_RETURN_FAIL;
	}

	tmp_node = get_children_node(tmp_node, MSG_RETURNCODE_KEY);
	if (!tmp_node) {
		return ROOM_RETURN_FAIL;
	}

	get_current_node_value(value, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	return atoi(value);
}

int32_t lower_msg_get_enc_status(parse_xml_t *parseXml)
{
	int8_t value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr  tmp_node = NULL;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		nslog(NS_ERROR, "MSG_BODY_KEY error");
		return -1;
	}

	tmp_node = get_children_node(tmp_node, MSG_ENCINFO_KEY);
	if (!tmp_node) {
		nslog(NS_ERROR, "MSG_ENCINFO_KEY error");
		return -1;
	}

	tmp_node = get_children_node(tmp_node, (BAD_CAST "Status"));
	if (!tmp_node) {
		nslog(NS_ERROR, "Status error");
		return -1;
	}

	get_current_node_value(value, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	return atoi(value);
}

int32_t lower_msg_get_record_result(RoomMsgEnv *pRoom, parse_xml_t *parseXml)
{
	int32_t Result						= -1;
	int8_t	value[ROOM_MSG_VAL_LEN] 	= {0};

	if (pRoom == NULL || parseXml == NULL) {
		nslog(NS_ERROR, "[%s] ---[params] error!", __func__);
		return -1;
	}

	//MsgBody
	xmlNodePtr MsgBody_ptr = NULL;
	MsgBody_ptr = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!MsgBody_ptr) {
		return -1;
	}

	//RecCtrlResp
	xmlNodePtr RecCtrlResp_ptr = NULL;
	RecCtrlResp_ptr = get_children_node(MsgBody_ptr, MSG_RECCTRLRESP_KEY);
	if (!RecCtrlResp_ptr) {
		return -1;
	}

	//Result 0—停止录制 1—开启录制 2—暂停录制
	lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RESULT_KEY, RecCtrlResp_ptr, parseXml->pdoc);
	Result = atoi(value);

	return Result;
}


int32_t lower_msg_get_record_status(RoomMsgEnv *pRoom, parse_xml_t *parseXml)
{
	int32_t status						= -1;
	int8_t	value[ROOM_MSG_VAL_LEN] 	= {0};

	if (pRoom == NULL || parseXml == NULL) {
		nslog(NS_ERROR, "[%s] ---[params] error!", __func__);
		return -1;
	}

	//MsgBody
	xmlNodePtr MsgBody_ptr = NULL;
	MsgBody_ptr = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!MsgBody_ptr) {
		return -1;
	}

	//RecReport
	xmlNodePtr RecReport_ptr = NULL;
	RecReport_ptr = get_children_node(MsgBody_ptr, MSG_RECORD_REP_KEY);
	if (!RecReport_ptr) {
		return -1;
	}

	//Result 0—完成录制 1—录制异常
	lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RESULT_KEY, RecReport_ptr, parseXml->pdoc);
	status = atoi(value);

	return status;
}

int32_t lower_msg_get_passkey_from_que(RoomMsgEnv *pRoom, uint32_t msg_code, int8_t *pass_key)
{
	int32_t i = 0;

	for (i = 0; i < ROOM_MSG_TIME_QUE_NUM; i++) {
		if (pRoom->msgTimeQue[i].MsgCode == msg_code) {
			r_strcpy(pass_key, pRoom->msgTimeQue[i].PassKey);
			break;
		}
	}

	if (i < ROOM_MSG_TIME_QUE_NUM)
		return ROOM_RETURN_SUCC;
	else
		return ROOM_RETURN_FAIL;
}

int32_t lower_msg_get_userid_from_que(RoomMsgEnv *pRoom, uint32_t msg_code, int8_t *user_id)
{
	int32_t i = 0;

	for (i = 0; i < ROOM_MSG_TIME_QUE_NUM; i++) {
		if (pRoom->msgTimeQue[i].MsgCode == msg_code) {
			r_strcpy(user_id, pRoom->msgTimeQue[i].UserId);
			break;
		}
	}

	if (i < ROOM_MSG_TIME_QUE_NUM)
		return ROOM_RETURN_SUCC;
	else
		return ROOM_RETURN_FAIL;
}


quality_info *lower_msg_get_quality_info_by_msgq_type(RoomMsgEnv *pRoom, uint32_t stream_id)
{
	int32_t i = 0;
	quality_info *e_info = NULL;

	if (!stream_id || !pRoom) {
		nslog(NS_ERROR, "stream_id = %d, pRoom = %p", stream_id, pRoom);
		return NULL;
	}

	for (i = 0; i < ROOM_ENC_NUM; i++){
//		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
//			continue;
//		}

		if (pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType == stream_id) {
			e_info = &pRoom->RoomInfo.EncInfo[i].HD_QuaInfo;
			break;
		}

		if (pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType == stream_id) {
			e_info = &pRoom->RoomInfo.EncInfo[i].SD_QuaInfo;
			break;
		}
	}

	return e_info;
}

enc_info *lower_msg_get_enc_info_by_msgq_type(RoomMsgEnv *pRoom, long stream_id)
{
	int32_t i = 0;
	enc_info *e_info = NULL;

	if (!stream_id || !pRoom) {
		nslog(NS_ERROR, "stream_id = %d, pRoom = %p", stream_id, pRoom);
		return NULL;
	}

	for (i = 0; i < ROOM_ENC_NUM; i++){
//		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
//			continue;
//		}

		if (pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType == stream_id) {
			e_info = &pRoom->RoomInfo.EncInfo[i];
			break;
		}

		if (pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType == stream_id) {
			e_info = &pRoom->RoomInfo.EncInfo[i];
			break;
		}
	}

	return e_info;
}

stream_handle *lower_msg_get_stream_handle(RoomMsgEnv *pRoom, int8_t *ip, int32_t stream_id)
{
	int32_t i		= 0;
	stream_handle *stream_hdl = NULL;

	for (i = 0; i < pRoom->handle->stream_num; i++) {
		if (!r_strcmp(pRoom->handle->stream_hand[i].ipaddr, ip) &&
			pRoom->handle->stream_hand[i].stream_id == stream_id) {
			stream_hdl = &pRoom->handle->stream_hand[i];
			break;
		}
	}

	return stream_hdl;
}

int32_t lower_msg_iFrame_req(RoomMsgEnv *pRoom, int32_t stream_id)
{
	MsgHeader head;
	int32_t ret_val = 0;
	int8_t *xml_buf = NULL;
	int8_t ip_buf[IP_LEN]		= {0};
	enc_info 		*e_info 	= NULL;
	stream_handle 	*stream_hdl = NULL;

	if (pRoom == NULL) {
		nslog(NS_ERROR, "[%s] ---[param] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	e_info = lower_msg_get_enc_info_by_msgq_type(pRoom, stream_id);
	if (!e_info) {
		nslog(NS_ERROR, "get enc info error");
		return ROOM_RETURN_FAIL;
	}

	msg_ctrl_inet_ntoa(ip_buf, e_info->EncIP);
	stream_hdl = lower_msg_get_stream_handle(pRoom, ip_buf, stream_id);
	if (!stream_hdl) {
		nslog(NS_ERROR, "get stream handle error");
		return ROOM_RETURN_FAIL;
	}

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, "[%s] ---[r_malloc] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	r_bzero(&head, sizeof(MsgHeader));
	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);

	ret_val = lower_msg_package_iFrame_req(xml_buf, pRoom->RoomInfo.RoomId);
	if (ret_val == ROOM_RETURN_SUCC && stream_hdl->sockfd > 0)
		upper_msg_tcp_send(stream_hdl->sockfd, &stream_hdl->mutex, &head, xml_buf);

	if (xml_buf)
		r_free(xml_buf);

	return ROOM_RETURN_SUCC;
}

int32_t lower_msg_start_record(RoomMsgEnv *pRoom)
{
	int32_t i 		= 0;
	int32_t req_num = 0;
	int32_t val_num	= 0;
	int32_t room_info_num = 0;
	int32_t ret_val = 0;
	int32_t rec_return_code = 1;
	course_record_condition_t crc;
	uint32_t req_mask, valid_mask, room_info_mask;

	r_memset(&crc, 0, sizeof(course_record_condition_t));
	ret_val = upper_msg_set_record_condition(pRoom, &crc);
	upper_msg_print_cinfo(&crc);

	if (ret_val == ROOM_RETURN_FAIL) {
		pRoom->RoomInfo.record_stream_mask = 0;
		nslog(NS_ERROR, "upper_msg_set_record_condition error!");
		return 0;
	}

#if 1
	//只要有一路流有效都启动录制
	room_info_num	= upper_msg_get_mask_num(pRoom->RoomInfo.room_info_mask);
	if (room_info_num) {
		rec_return_code = 1;
	} else {
		rec_return_code = 0;
	}

	nslog(NS_INFO, "room_info_num = %d, %s setup record...", room_info_num, room_info_num ? "will":"will not");
#else
	req_num 		= upper_msg_get_mask_num(pRoom->RoomInfo.req_stream_mask);
	val_num 		= upper_msg_get_mask_num(pRoom->RoomInfo.valid_stream_mask);
	room_info_num	= upper_msg_get_mask_num(pRoom->RoomInfo.room_info_mask);

	nslog(NS_INFO, "--- req_num = %d,val_num = %d,room_info_num = %d,req_stream_mask = %d,valid_stream_mask = %d,room_info_mask = %d",\
				req_num, val_num, room_info_num, pRoom->RoomInfo.req_stream_mask, pRoom->RoomInfo.valid_stream_mask, pRoom->RoomInfo.room_info_mask);

	//请求流数和有效流数不等或与教室信息mask 不等或无流时，不启动录制
	if ((req_num != val_num || req_num != room_info_num || val_num == 0)) {
		nslog(NS_WARN, " ---req_num = %d,	val_num = %d, room_info_num = %d!!!\n", req_num, val_num, room_info_num);
		rec_return_code = 0;
	}

	req_mask 		= pRoom->RoomInfo.req_stream_mask;
	valid_mask 		= pRoom->RoomInfo.valid_stream_mask;
	room_info_mask 	= pRoom->RoomInfo.room_info_mask;

	if (((req_mask & valid_mask) == req_mask) && \
		((req_mask & room_info_mask) == req_mask) && \
		(req_mask != 0) ) {
		nslog(NS_INFO, " ---req_mask = %d,	valid_mask = %d, room_info_mask = %d!!!\n", req_mask, valid_mask, room_info_mask);
		rec_return_code = 1;
	}
#endif

	if (rec_return_code == 1) {
		pRoom->record_handle = register_course_record_mode(&crc);
		if(NULL == pRoom->record_handle) {
			nslog(NS_ERROR, "[%s] ---register_course_record_mode is failed !!!\n", __func__);
			rec_return_code = 0;
		}
	} else {
		pRoom->RoomInfo.record_stream_mask = 0;
	}

	if (rec_return_code == 1) {
		ret_val = pRoom->record_handle->record_start(pRoom->record_handle);
		if (ret_val) {
			nslog(NS_ERROR, "[%s] ---record_start is failed !!!\n", __func__);
			rec_return_code = 0;
			unregister_course_record_mode(&pRoom->record_handle);
			pRoom->record_handle = NULL;
		}
	} else {
		pRoom->RoomInfo.record_stream_mask = 0;
	}

	if (rec_return_code == 1) {
		pRoom->lowerEnv.msg_id[RECOMSGID] = pRoom->record_handle->course_data_msgid;
		for (i = 0; i < pRoom->handle->stream_num; i++) {
			if (pRoom->RoomInfo.record_stream_mask & (1 << pRoom->handle->stream_hand[i].stream_id)) {
				pRoom->handle->set_recv_to_rec_msgid(pRoom->lowerEnv.msg_id[RECOMSGID], &pRoom->handle->stream_hand[i]);
				pRoom->handle->set_rec_status(&pRoom->handle->stream_hand[i], START_REC);


				//强制I 帧
				lower_msg_iFrame_req(pRoom, pRoom->handle->stream_hand[i].stream_id);
			}
		}
	} else {
		pRoom->RoomInfo.record_stream_mask = 0;
	}

	if (rec_return_code == 0) {
		pRoom->RoomInfo.record_stream_mask = 0;
	}

	nslog(NS_INFO, "--- start record end.......%s", rec_return_code ? "SUCCESS" : "FAIL");

	return rec_return_code;
}

int32_t lower_msg_start_record_resp(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, int32_t ret_val)
{
	int8_t 		user_id[ROOM_MSG_VAL_LEN]		= {0};
	int8_t		rec_id[ROOM_MSG_VAL_LEN]		= {0};
	int8_t		rec_file[ROOM_MSG_VAL_LEN]		= {0};
	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN]	= {0};

	xmlNodePtr	tmp_node	= NULL;
	MsgHeader	*head		= NULL;
	int8_t		*xml_buf	= NULL;

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, "[%s] ---[r_malloc] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	head = (MsgHeader *)msgq->msgbuf;

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	if (pRoom->record_handle) {
		pRoom->RecStatusInfo.RecStatus = lower_msg_opt_type_trans(pRoom->record_handle->get_record_status(pRoom->record_handle));
		r_strcpy(rec_id, pRoom->record_handle->get_RecordID(pRoom->record_handle));
		r_strcpy(rec_file, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
	} else {
		pRoom->RecStatusInfo.RecStatus = 0;
		r_strcpy(rec_id, pRoom->RecInfo.RecordID);
		r_strcpy(rec_file, pRoom->RecInfo.CourseName);
	}

	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_get_user_id_value(parseXml, user_id);
	upper_msg_package_record_xml(xml_buf, ret_val, rec_file, ret_pass_key, pRoom->RecInfo.userid, rec_id, pRoom->RecInfo.OptType, pRoom->RecStatusInfo.RecStatus);
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

	if (xml_buf)
		r_free(xml_buf);

	return ROOM_RETURN_SUCC;
}

void lower_msg_monitor_stream_status(RoomMsgEnv *pRoom)
{
	int32_t 		i 				= 0;
	int32_t 		ret_val			= 0;
	int8_t 			ip_buf[IP_LEN]	= {0};
	enc_info 		*e_info 		= NULL;
	stream_handle	*stream_hdl 	= NULL;

	for (i = 0; i < ROOM_ENC_NUM; i ++) {
		e_info = &pRoom->RoomInfo.EncInfo[i];

		if (e_info->ID == 0 || e_info->Status == 0) {
			continue;
		}

		//HD
		if (e_info->HD_QuaInfo.Socket > 0) {
			ret_val = upper_msg_monitor_time_out_status(&e_info->HD_QuaInfo.heart_time, ROOM_MSG_LOGIN_TIME_OUT_VAL);
			if (ret_val == ROOM_RETURN_TIMEOUT) {
				msg_ctrl_inet_ntoa(ip_buf, e_info->EncIP);
				stream_hdl = lower_msg_get_stream_handle(pRoom, ip_buf, e_info->HD_QuaInfo.MsgQueType);
				if (stream_hdl) {
					nslog(NS_WARN, "-- HD heart time out....will close the [socket = %d] [ENC_ID = %d][stream = %d]", e_info->HD_QuaInfo.Socket, e_info->ID, e_info->HD_QuaInfo.MsgQueType);
					//pRoom->handle->close_stream_connect(stream_hdl);
					close_socket(e_info->HD_QuaInfo.Socket);

					stream_hdl->sockfd = -1;

					e_info->HD_QuaInfo.Socket = -1;
					e_info->HD_QuaInfo.Status = 0;
					pRoom->RoomInfo.room_info_mask		&= ~(1 << e_info->HD_QuaInfo.MsgQueType);
					//pRoom->RoomInfo.req_stream_mask 	&= ~(1 << e_info->HD_QuaInfo.MsgQueType);
					pRoom->RoomInfo.valid_stream_mask	&= ~(1 << e_info->HD_QuaInfo.MsgQueType);

				}
			} else if (ret_val == ROOM_RETURN_SUCC) {
				;//continue;
			} else if (ret_val == ROOM_RETURN_FAIL) {
				e_info->HD_QuaInfo.heart_time.tv_sec	= 0;
				e_info->HD_QuaInfo.heart_time.tv_usec	= 0;
				gettimeofday(&e_info->HD_QuaInfo.heart_time, 0);
				//continue;
			}
		}

		//SD
		if (e_info->SD_QuaInfo.Socket > 0) {
			ret_val = upper_msg_monitor_time_out_status(&e_info->SD_QuaInfo.heart_time, ROOM_MSG_LOGIN_TIME_OUT_VAL);
			if (ret_val == ROOM_RETURN_TIMEOUT) {
				msg_ctrl_inet_ntoa(ip_buf, e_info->EncIP);
				stream_hdl = lower_msg_get_stream_handle(pRoom, ip_buf, e_info->SD_QuaInfo.MsgQueType);
				if (stream_hdl) {
					nslog(NS_WARN, "-- SD heart time out....will close the [socket = %d] [ENC_ID = %d][stream = %d]", e_info->SD_QuaInfo.Socket, e_info->ID, e_info->SD_QuaInfo.MsgQueType);
					//pRoom->handle->close_stream_connect(stream_hdl);
					close_socket(e_info->SD_QuaInfo.Socket);

					stream_hdl->sockfd = -1;

					e_info->SD_QuaInfo.Socket = -1;
					e_info->SD_QuaInfo.Status = 0;
					pRoom->RoomInfo.room_info_mask		&= ~(1 << e_info->SD_QuaInfo.MsgQueType);
					//pRoom->RoomInfo.req_stream_mask 	&= ~(1 << e_info->SD_QuaInfo.MsgQueType);
					pRoom->RoomInfo.valid_stream_mask	&= ~(1 << e_info->SD_QuaInfo.MsgQueType);

				}
			} else if (ret_val == ROOM_RETURN_SUCC) {
				;//continue;
			} else if (ret_val == ROOM_RETURN_FAIL) {
				e_info->SD_QuaInfo.heart_time.tv_sec	= 0;
				e_info->SD_QuaInfo.heart_time.tv_usec	= 0;
				gettimeofday(&e_info->SD_QuaInfo.heart_time, 0);
				//continue;
			}
		}
	}
}

int32_t lower_msg_check_enc(RoomMsgEnv *pRoom)
{
	int32_t  i 				= 0;
	int32_t  ret_val 		= 0;
	int32_t  JPEG_num		= 0;
	int32_t  JPEG_fist_ID 	= 0;
	int32_t  JPEG_last_ID 	= 0;
	int32_t  H264_last_ID	= 0;
	int8_t 	 ip_buf[IP_LEN]	= {0};
	enc_info *e_info 		= NULL;
	stream_handle	*stream_hdl_close = NULL;

	for (i = 0; i < ROOM_ENC_NUM; i ++) {
		e_info = &pRoom->RoomInfo.EncInfo[i];

		if (e_info->ID == 0) {
			continue;
		}

		if (e_info->EncodeType == EncodeType_H264) {
			H264_last_ID = e_info->ID;
		}

		if (e_info->EncodeType == EncodeType_JPEG) {
			if (JPEG_fist_ID == 0) {
				JPEG_fist_ID = e_info->ID;
			}

			JPEG_num++;
			JPEG_last_ID = e_info->ID;
		}
	}

	//如果有两路以上JPEG，关闭前面的，只剩最后一路
	if (JPEG_num > 1) {
		for (i = 0; i < ROOM_ENC_NUM; i ++) {
			e_info = &pRoom->RoomInfo.EncInfo[i];

			if (e_info->ID == 0) {
				continue;
			}

			//关闭非最后一路JPEG
			if (e_info->EncodeType == EncodeType_JPEG) {
				if (e_info->ID != JPEG_last_ID) {
					msg_ctrl_inet_ntoa(ip_buf, e_info->EncIP);

					//close HD
					stream_hdl_close = lower_msg_get_stream_handle(pRoom, ip_buf, e_info->HD_QuaInfo.MsgQueType);
					if (stream_hdl_close)
						pRoom->handle->close_stream_connect(stream_hdl_close);
					e_info->HD_QuaInfo.Socket = -1;
					e_info->HD_QuaInfo.Status = 0;
					pRoom->RoomInfo.room_info_mask 		&= ~(1 << e_info->HD_QuaInfo.MsgQueType);
					pRoom->RoomInfo.req_stream_mask		&= ~(1 << e_info->HD_QuaInfo.MsgQueType);
					pRoom->RoomInfo.valid_stream_mask 	&= ~(1 << e_info->HD_QuaInfo.MsgQueType);
					nslog(NS_WARN, "close ip = %s, stream_id = %d", ip_buf, e_info->HD_QuaInfo.MsgQueType);

					//close SD
					stream_hdl_close = lower_msg_get_stream_handle(pRoom, ip_buf, e_info->SD_QuaInfo.MsgQueType);
					if (stream_hdl_close)
						pRoom->handle->close_stream_connect(stream_hdl_close);
					e_info->SD_QuaInfo.Socket = -1;
					e_info->SD_QuaInfo.Status = 0;
					pRoom->RoomInfo.room_info_mask 		&= ~(1 << e_info->SD_QuaInfo.MsgQueType);
					pRoom->RoomInfo.req_stream_mask		&= ~(1 << e_info->SD_QuaInfo.MsgQueType);
					pRoom->RoomInfo.valid_stream_mask 	&= ~(1 << e_info->SD_QuaInfo.MsgQueType);
					nslog(NS_WARN, "close ip = %s, stream_id = %d", ip_buf, e_info->SD_QuaInfo.MsgQueType);
				}

				JPEG_num++;
			}
		}
	}

	//JPEG不是最后一路，返回失败
	nslog(NS_INFO, "JPEG_num = %d, JPEG_fist_ID = %d, JPEG_last_ID = %d, H264_last_ID = %d", JPEG_num, JPEG_fist_ID, JPEG_last_ID, H264_last_ID);
	if ((JPEG_last_ID > 0) && (JPEG_last_ID < H264_last_ID)) {
		return ROOM_RETURN_FAIL;
	} else {
		return ROOM_RETURN_SUCC;
	}

}

int32_t lower_msg_save_enc_info(RoomMsgEnv *pRoom, int8_t *xml_buf, long msgType)
{
	int32_t fd							= 0;
	int32_t ret 						= 0;
	int32_t status						= ROOM_RETURN_SUCC;
	int32_t EncInfoId 					= 0;
	int8_t  value[ROOM_MSG_VAL_LEN] 	= {0};
	int8_t	fileName[ROOM_MSG_VAL_LEN] 	= {0};

	enc_info *e_info 					= NULL;

	e_info = lower_msg_get_enc_info_by_msgq_type(pRoom, msgType);
	if (!e_info) {
		nslog(NS_ERROR, "get enc info error");
		return ROOM_RETURN_FAIL;
	}

	EncInfoId = e_info->ID - 1;

	if (pRoom == NULL || xml_buf == NULL) {
		nslog(NS_ERROR, "[%s] ---[params] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	parse_xml_t *parse_xml_cfg = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(NULL == init_dom_tree(parse_xml_cfg, xml_buf)) {
		nslog(NS_ERROR, "[%s] ---[init_dom_tree] is error!", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	if(r_strcmp(parse_xml_cfg->proot->name, RESP_ROOT_KEY)) {
		nslog(NS_ERROR, "[%s] --- is not REQ_ROOT_KEY!", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	//MsgBody
	xmlNodePtr MsgBody_ptr = NULL;
	MsgBody_ptr = get_children_node(parse_xml_cfg->proot, MSG_BODY_KEY);
	if (!MsgBody_ptr) {
		cleanup(ROOM_RETURN_FAIL);
	}

	//RoomInfo
	xmlNodePtr RoomInfo_ptr = NULL;
	RoomInfo_ptr = get_children_node(MsgBody_ptr, MSG_ROOMINFO_KEY);
	if (!RoomInfo_ptr) {
		cleanup(ROOM_RETURN_FAIL);
	}


	//AudioInfo, 只保存第一路编码器的音频信息
	xmlNodePtr AudioInfo_ptr = NULL;
	AudioInfo_ptr = get_children_node(RoomInfo_ptr, MSG_AUDIOINFO_KEY);
	if (AudioInfo_ptr && \
		(msgType == pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.MsgQueType || msgType == pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.MsgQueType)) {

		//InputMode
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_INPUTMODE_KEY, AudioInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pRoom->RoomInfo.AudioInfo.InputMode = atoi(value);

		//SampleRate
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_SAMPLERATE_KEY, AudioInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pRoom->RoomInfo.AudioInfo.SampleRate = lower_msg_get_sample_by_index(atoi(value));

		//Bitrate
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_BITRATE_KEY, AudioInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pRoom->RoomInfo.AudioInfo.Bitrate = atoi(value);

		//Lvolume
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_LVOLUME_KEY, AudioInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pRoom->RoomInfo.AudioInfo.Lvolume = atoi(value);

		//Rvolume
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RVOLUME_KEY, AudioInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pRoom->RoomInfo.AudioInfo.Rvolume = atoi(value);
	}

	//EncInfo
	xmlNodePtr EncInfo_ptr = NULL;
	EncInfo_ptr = get_children_node(RoomInfo_ptr, MSG_ENCINFO_KEY);
	if (!EncInfo_ptr) {
		cleanup(ROOM_RETURN_FAIL);
	}

	//ID
	//lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCINFOID_KEY, EncInfo_ptr, parse_xml_cfg->pdoc);
	//pRoom->RoomInfo.EncInfo[EncInfoId].ID = atoi(value);
	pRoom->RoomInfo.EncInfo[EncInfoId].ID = EncInfoId+1;

	//EncIP
	//lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCIP_KEY, EncInfo_ptr, parse_xml_cfg->pdoc);
	//msg_ctrl_inet_aton(value, &pRoom->RoomInfo.EncInfo[EncInfoId].EncIP);

	//Status
//	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_STATUS_KEY, EncInfo_ptr, parse_xml_cfg->pdoc);
//	if (ret == ROOM_RETURN_SUCC)
//		pRoom->RoomInfo.EncInfo[EncInfoId].Status = atoi(value);

	//QualityInfo [1]
	xmlNodePtr QualityInfo_ptr = NULL;
	QualityInfo_ptr = get_children_node(EncInfo_ptr, MSG_QUALITYINFO_KEY);
	if (!QualityInfo_ptr) {
		cleanup(ROOM_RETURN_FAIL);
	}

	//RateType
	int32_t rateType 			= 0;
	quality_info *pQualityInfo 	= NULL;
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RATETYPE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC) {
		rateType = atoi(value);
		if (rateType == MSG_RATETYPE_HD) {
			pQualityInfo = &pRoom->RoomInfo.EncInfo[EncInfoId].HD_QuaInfo;
		} else {
			pQualityInfo = &pRoom->RoomInfo.EncInfo[EncInfoId].SD_QuaInfo;
		}

		//RateType
		pQualityInfo->RateType = atoi(value);
	} else {
		cleanup(ROOM_RETURN_FAIL);
	}


	//EncBitrate
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCBITRATE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC && atoi(value) > 0)
		pQualityInfo->EncBitrate = atoi(value);

	//EncWidth
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCWIDTH_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC && atoi(value) > 0)
		pQualityInfo->EncWidth = atoi(value);

	//EncHeight
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCHEIGHT_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC && atoi(value) > 0)
		pQualityInfo->EncHeight = atoi(value);

	//EncFrameRate
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCFRAMERATE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC && atoi(value) > 0)
		pQualityInfo->EncFrameRate = atoi(value);


	//QualityInfo [2]
	QualityInfo_ptr = upper_msg_get_next_samename_node(QualityInfo_ptr);
	if (!QualityInfo_ptr) {
		cleanup(ROOM_RETURN_FAIL);
	}

	//RateType
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RATETYPE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC) {
		rateType = atoi(value);
		if (rateType == MSG_RATETYPE_HD) {
			pQualityInfo = &pRoom->RoomInfo.EncInfo[EncInfoId].HD_QuaInfo;
		} else {
			pQualityInfo = &pRoom->RoomInfo.EncInfo[EncInfoId].SD_QuaInfo;
		}

		//RateType
		pQualityInfo->RateType = atoi(value);
	} else {
		cleanup(ROOM_RETURN_FAIL);
	}

	//EncBitrate
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCBITRATE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC && atoi(value) > 0)
		pQualityInfo->EncBitrate = atoi(value);

	//EncWidth
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCWIDTH_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC && atoi(value) > 0)
		pQualityInfo->EncWidth = atoi(value);

	//EncHeight
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCHEIGHT_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC && atoi(value) > 0)
		pQualityInfo->EncHeight = atoi(value);

	//EncFrameRate
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCFRAMERATE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC && atoi(value) > 0)
		pQualityInfo->EncFrameRate = atoi(value);

cleanup:

	if(parse_xml_cfg->pdoc != NULL) {
		release_dom_tree(parse_xml_cfg->pdoc);
	}

	if(parse_xml_cfg) {
		r_free(parse_xml_cfg);
	}

	return status;
}

int32_t lower_msg_load_room_info(RoomMsgEnv *pRoom, int8_t *xml_buf)
{
	int32_t status 			= 0;
	int8_t 	value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr 	EncInfo_ptr	= NULL;
	uint32_t   	EncInfoId	= 0;
	uint32_t   	EncNum		= 0;
	int32_t		ret			= 0;

	if (pRoom == NULL || xml_buf == NULL) {
		nslog(NS_ERROR, "[%s] ---[params] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	parse_xml_t *parse_xml_cfg = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(NULL == init_dom_tree(parse_xml_cfg, xml_buf)) {
		nslog(NS_ERROR, "[%s] ---[init_dom_tree] is error!", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	//MsgBody
	xmlNodePtr MsgBody_ptr = NULL;
	MsgBody_ptr = get_children_node(parse_xml_cfg->proot, MSG_BODY_KEY);
	if (!MsgBody_ptr) {
		cleanup(ROOM_RETURN_FAIL);
	}

	//RoomInfo
	xmlNodePtr RoomInfo_ptr = NULL;
	RoomInfo_ptr = get_children_node(MsgBody_ptr, MSG_ROOMINFO_KEY);
	if (!RoomInfo_ptr) {
		cleanup(ROOM_RETURN_FAIL);
	}

	//ConnStatus
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_CONNECTSTATUS_KEY, RoomInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		pRoom->RoomInfo.ConnectStatus = atoi(value);

	//RecordMaxTime
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RECMAXTIME_REP_KEY, RoomInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		pRoom->RoomInfo.RecordMaxTime = atoi(value);

	//AudioInfo
	xmlNodePtr AudioInfo_ptr = NULL;
	AudioInfo_ptr = get_children_node(RoomInfo_ptr, MSG_AUDIOINFO_KEY);
	if (AudioInfo_ptr) {

		//InputMode
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_INPUTMODE_KEY, AudioInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pRoom->RoomInfo.AudioInfo.InputMode = atoi(value);

		//SampleRate
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_SAMPLERATE_KEY, AudioInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pRoom->RoomInfo.AudioInfo.SampleRate = atoi(value); //lower_msg_get_sample_by_index(atoi(value));

		//Bitrate
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_BITRATE_KEY, AudioInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pRoom->RoomInfo.AudioInfo.Bitrate = atoi(value);

		//Lvolume
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_LVOLUME_KEY, AudioInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pRoom->RoomInfo.AudioInfo.Lvolume = atoi(value);

		//Rvolume
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RVOLUME_KEY, AudioInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pRoom->RoomInfo.AudioInfo.Rvolume = atoi(value);
	}

	//取各路ENC 信息
	EncInfo_ptr = get_children_node(RoomInfo_ptr, MSG_ENCINFO_KEY);
	for (; EncInfo_ptr != NULL; EncInfo_ptr = upper_msg_get_next_samename_node(EncInfo_ptr)) {
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCINFOID_KEY, EncInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			EncInfoId = atoi(value);

		if (EncInfoId < 1 || EncInfoId > ROOM_ENC_NUM) {
			continue;
		}

		pRoom->RoomInfo.EncInfo[EncInfoId-1].ID 		= 0;
		pRoom->RoomInfo.EncInfo[EncInfoId-1].EncodeType = EncodeType_INVALID;
		pRoom->RoomInfo.EncInfo[EncInfoId-1].EncIP 		= 0;

		//EncIP
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCIP_KEY, EncInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC && r_strlen(value)) {
			pRoom->RoomInfo.EncInfo[EncInfoId-1].ID 		= EncInfoId;
			pRoom->RoomInfo.EncInfo[EncInfoId-1].EncodeType = EncodeType_INVALID;
			pRoom->RoomInfo.EncInfo[EncInfoId-1].EncIP = msg_ctrl_inet_aton(value, NULL);
			EncNum++;
		} else {
			continue;
		}

		//QualityInfo
		xmlNodePtr QualityInfo_ptr = NULL;
		QualityInfo_ptr = get_children_node(EncInfo_ptr, MSG_QUALITYINFO_KEY);
		if (!QualityInfo_ptr) {
			continue;
		}

		//RateType
		int32_t rateType			= 0;
		quality_info *pQualityInfo	= NULL;
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RATETYPE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC) {
			rateType = atoi(value);

			if (rateType == MSG_RATETYPE_HD) {
				pQualityInfo = &pRoom->RoomInfo.EncInfo[EncInfoId-1].HD_QuaInfo;
			} else {
				pQualityInfo = &pRoom->RoomInfo.EncInfo[EncInfoId-1].SD_QuaInfo;
			}

			//RateType
			pQualityInfo->RateType = rateType;
		} else {
			continue;
		}

		//EncBitrate
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCBITRATE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pQualityInfo->EncBitrate = atoi(value);

		//EncWidth
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCWIDTH_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pQualityInfo->EncWidth = atoi(value);

		//EncHeight
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCHEIGHT_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pQualityInfo->EncHeight = atoi(value);

		//EncFrameRate
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCFRAMERATE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pQualityInfo->EncFrameRate = atoi(value);

		//QualityInfo
		QualityInfo_ptr = upper_msg_get_next_samename_node(QualityInfo_ptr);
		if (!QualityInfo_ptr) {
			continue;
		}

		//RateType
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RATETYPE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC) {
			rateType = atoi(value);

			if (rateType == MSG_RATETYPE_HD) {
				pQualityInfo = &pRoom->RoomInfo.EncInfo[EncInfoId-1].HD_QuaInfo;
			} else {
				pQualityInfo = &pRoom->RoomInfo.EncInfo[EncInfoId-1].SD_QuaInfo;
			}

			//RateType
			pQualityInfo->RateType = rateType;
		} else {
			continue;
		}

		//EncBitrate
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCBITRATE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pQualityInfo->EncBitrate = atoi(value);

		//EncWidth
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCWIDTH_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pQualityInfo->EncWidth = atoi(value);

		//EncHeight
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCHEIGHT_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pQualityInfo->EncHeight = atoi(value);

		//EncFrameRate
		ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCFRAMERATE_KEY, QualityInfo_ptr, parse_xml_cfg->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			pQualityInfo->EncFrameRate = atoi(value);

	}

	if (EncNum) {
		pRoom->RoomInfo.EncNum = EncNum;
	}

cleanup:

	if(parse_xml_cfg->pdoc != NULL) {
		release_dom_tree(parse_xml_cfg->pdoc);
	}

	if(parse_xml_cfg) {
		r_free(parse_xml_cfg);
	}
}

int32_t lower_msg_init_live_param(video_sindex_info_t *enc_info)
{
	int32_t i 		= 0;
	int32_t max_num = 0;

	enc_info->enc_num = -1;

	max_num = sizeof(enc_info->video_enc)/sizeof(enc_info->video_enc[0]);
	for (i = 0; i < max_num; i++) {
		enc_info->video_enc[i].HD_video_sindex 	= -1;
		enc_info->video_enc[i].BD_video_sindex 	= -1;
		enc_info->video_enc[i].enc_type			= -1;
	}

	nslog(NS_INFO, "-----init live param---- max_num = %d, i = %d", max_num, i);

	return ROOM_RETURN_FAIL;
}

int32_t lower_msg_print_live_param(video_sindex_info_t *enc_info)
{
	int32_t i 		= 0;
	int32_t max_num = 0;

	nslog(NS_INFO, "enc_info->enc_num = %d", enc_info->enc_num);

	max_num = sizeof(enc_info->video_enc)/sizeof(enc_info->video_enc[0]);
	for (i = 0; i < max_num; i++) {
		nslog(NS_INFO, "enc_info->video_enc[%d].HD_video_sindex = %d", i, enc_info->video_enc[i].HD_video_sindex);
		nslog(NS_INFO, "enc_info->video_enc[%d].BD_video_sindex = %d", i, enc_info->video_enc[i].BD_video_sindex);
		nslog(NS_INFO, "enc_info->video_enc[%d].enc_type = %d\n"	 , i, enc_info->video_enc[i].enc_type);
	}

	return ROOM_RETURN_FAIL;
}

int32_t lower_msg_set_live_info(RoomMsgEnv *pRoom, int32_t msg_type)
{
	int32_t i 			 = 0;
	int32_t ret			 = -1;
	int32_t last_enc_id  = 0;
	int32_t HD_stream_id = 0;
	int32_t SD_stream_id = 0;
	video_sindex_info_t enc_info;

	lower_msg_init_live_param(&enc_info);

//	enc_info.enc_num = pRoom->RoomInfo.EncNum;

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		} else {
			last_enc_id = pRoom->RoomInfo.EncInfo[i].ID;
		}

		HD_stream_id = pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType;
		SD_stream_id = pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType;

		if (pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.Status) {
			enc_info.video_enc[i].HD_video_sindex	= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType;
		}

		if (pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.Status && \
			pRoom->RoomInfo.EncInfo[i].EncodeType == EncodeType_H264) {
			enc_info.video_enc[i].BD_video_sindex	= pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType;
		}

		if (pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.Status || \
			pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.Status) {
			enc_info.video_enc[i].enc_type			= pRoom->RoomInfo.EncInfo[i].EncodeType;
		}
	}

	enc_info.enc_num = last_enc_id;

	lower_msg_print_live_param(&enc_info);

	// zl question???
	#if 1
	if (pRoom->live_handle) {
		pRoom->live_handle->set_lives_enc_info(&enc_info, pRoom->live_handle);

		ret = pRoom->live_handle->recognition_req_strm_proc(pRoom->live_handle, msg_type);
	}
	#endif

	return ret;
}

int32_t lower_msg_req_room_info(RoomMsgEnv *pRoom, msgque *msgq, MsgHeader *head, int8_t *pass_key)
{
	//更新本地信息
	int8_t			*xml_buf	= NULL;
	int32_t 		socket		= -1;
	quality_info	*q_info 	= NULL;
	pthread_mutex_t *mutex		= NULL;
	q_info = lower_msg_get_quality_info_by_msgq_type(pRoom, msgq->msgtype);
	if (!q_info) {
		nslog(NS_ERROR, "get quality info error");
		return ROOM_RETURN_FAIL;
	}


	socket	= q_info->Socket;
	mutex	= q_info->mutex;

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ROOM_RETURN_FAIL;
	}

	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_req_enc_info(pRoom->RoomInfo.RoomId, xml_buf, MSG_GET_ROOM_INFO, pass_key, "0");

	if (socket > 0) {
		upper_msg_tcp_send(socket, mutex, head, xml_buf);
	}

	if (xml_buf)
		r_free(xml_buf);

}

void lower_msg_ctrl_enc_lock_req(RoomMsgEnv *pRoom, msgque *msgq, MsgHeader *head, int32_t opt_type)
{
	int8_t			*xml_buf	= NULL;
	int32_t 		socket		= -1;
	quality_info	*q_info 	= NULL;
	pthread_mutex_t *mutex		= NULL;
	q_info = lower_msg_get_quality_info_by_msgq_type(pRoom, msgq->msgtype);
	if (!q_info) {
		nslog(NS_ERROR, "get quality info error");
		return ;
	}

	socket	= q_info->Socket;
	mutex	= q_info->mutex;

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ;
	}

	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	lower_msg_package_record_req(xml_buf, NULL, opt_type);

	if (socket > 0)
		upper_msg_tcp_send(socket, mutex, head, xml_buf);

	if (xml_buf)
		r_free(xml_buf);
}


int32_t lower_msg_save_enc_login_info(EncLoginInfo *login_info, int8_t *xml_buf)
{
	int32_t fd							= 0;
	int32_t ret 						= 0;
	int32_t status						= ROOM_RETURN_SUCC;
	int32_t EncInfoId 					= 0;
	int8_t  value[ROOM_MSG_VAL_LEN] 	= {0};
	int8_t	fileName[ROOM_MSG_VAL_LEN] 	= {0};


	if (login_info == NULL || xml_buf == NULL) {
		nslog(NS_ERROR, "[%s] ---[params] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	parse_xml_t *parse_xml_cfg = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(NULL == init_dom_tree(parse_xml_cfg, xml_buf)) {
		nslog(NS_ERROR, "[%s] ---[init_dom_tree] is error!", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	if(r_strcmp(parse_xml_cfg->proot->name, RESP_ROOT_KEY)) {
		nslog(NS_ERROR, "[%s] --- is not REQ_ROOT_KEY!", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	//MsgBody
	xmlNodePtr MsgBody_ptr = NULL;
	MsgBody_ptr = get_children_node(parse_xml_cfg->proot, MSG_BODY_KEY);
	if (!MsgBody_ptr) {
		cleanup(ROOM_RETURN_FAIL);
	}

	//Name
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_NAME_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->Name, value);

	//SerialNum
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_SERIALNUM_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->SerialNum, value);

	//MacAddr
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_MACADDR_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->MacAddr, value);

	//IPAddr
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_IPADDR_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->IPAddr, value);

	//GateWay
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_GATEWAY_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->GateWay, value);

	//NetMask
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_NETMASK_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->NetMask, value);

	//DeviceVersion
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_DEVVER_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->DeviceVersion, value);

	//ChannelNum
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_CHNNLNUM_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->ChannelNum, value);

	//PPTIndex
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_PPTIDX_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->PPTIndex, value);

	//LowRate
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_LOWRATE_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->LowRate, value);

	//EncodeType
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCODETYPE_KEY, MsgBody_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(login_info->EncodeType, value);

cleanup:

	if(parse_xml_cfg->pdoc != NULL) {
		release_dom_tree(parse_xml_cfg->pdoc);
	}

	if(parse_xml_cfg) {
		r_free(parse_xml_cfg);
	}

	return status;
}

int32_t lower_msg_save_enc_status_info(RoomMsgEnv *pRoom, int8_t *xml_buf, int32_t msgp_type)
{
	int32_t ret 						= 0;
	int32_t status						= ROOM_RETURN_SUCC;
	int32_t EncInfoId 					= 0;
	int8_t  value[ROOM_MSG_VAL_LEN] 	= {0};

	enc_info *e_info					= NULL;

	if (pRoom == NULL || xml_buf == NULL) {
		nslog(NS_ERROR, " ---[params: pRoom = %p, xml_buf = %p] error!", pRoom, xml_buf);
		return ROOM_RETURN_FAIL;
	}

	if (msgp_type < 1 || msgp_type > ROOM_STR_NUM) {
		nslog(NS_ERROR, " ---[params: msgp_type = %d] error!", msgp_type);
		return ROOM_RETURN_FAIL;
	}

	//根据消息队列type 对应编码器
	e_info = lower_msg_get_enc_info_by_msgq_type(pRoom, msgp_type);
	if (NULL == e_info) {
		nslog(NS_ERROR, " ---[lower_msg_get_enc_info_by_msgq_type[msgtype = %d]] error!", msgp_type);
		return ROOM_RETURN_FAIL;
	}

	parse_xml_t *parse_xml_cfg = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(NULL == init_dom_tree(parse_xml_cfg, xml_buf)) {
		nslog(NS_ERROR, " ---[init_dom_tree] is error!");
		cleanup(ROOM_RETURN_FAIL);
	}

	if(r_strcmp(parse_xml_cfg->proot->name, REQ_ROOT_KEY)) {
		nslog(NS_ERROR, " --- is not REQ_ROOT_KEY!");
		cleanup(ROOM_RETURN_FAIL);
	}

	//MsgBody
	xmlNodePtr MsgBody_ptr = NULL;
	MsgBody_ptr = get_children_node(parse_xml_cfg->proot, MSG_BODY_KEY);
	if (!MsgBody_ptr) {
		cleanup(ROOM_RETURN_FAIL);
	}

	//EncInfo
	xmlNodePtr EncInfo_ptr = NULL;
	EncInfo_ptr = get_children_node(MsgBody_ptr, MSG_ENCINFO_KEY);
	if (!EncInfo_ptr) {
		cleanup(ROOM_RETURN_FAIL);
	}

	//Status
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_STATUS_KEY, EncInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC) {
		if (e_info->s_Status != atoi(value)) {
			status = ROOM_RETURN_TIMEOUT;
		}

		e_info->s_Status = atoi(value);
	}

	//Mute
	ret = lower_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_MUTE_KEY, EncInfo_ptr, parse_xml_cfg->pdoc);
	if (ret == ROOM_RETURN_SUCC) {
		if (e_info->s_Mute != atoi(value)) {
			status = ROOM_RETURN_TIMEOUT;
		}

		e_info->s_Mute = atoi(value);
	}

cleanup:

	if(parse_xml_cfg->pdoc != NULL) {
		release_dom_tree(parse_xml_cfg->pdoc);
	}

	if(parse_xml_cfg) {
		r_free(parse_xml_cfg);
	}

	return status;
}

int32_t lower_msg_package_enc_status_info(RoomMsgEnv *pRoom, int8_t *out_buf, int32_t msgcode, int8_t *pass_key, int8_t *user_id)
{
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pxml = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;


	if(NULL == out_buf || NULL == pRoom || NULL == pass_key || NULL == user_id){
		nslog(NS_ERROR, "[%s]---params is NULL!", __func__);
		return ROOM_RETURN_FAIL;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//RequestMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msgcode);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//RoomID
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.RoomId);
	upper_msg_package_add_xml_leaf(body_node, MSG_ROOMID_KEY, msgcode_buf);

	//EncInfo
	int32_t 	i 				= 0;
	xmlNodePtr	EncInfo_node 	= NULL;
	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0 || pRoom->RoomInfo.EncInfo[i].Status == 0) {
			continue;
		}

		EncInfo_node = xmlNewNode(NULL, MSG_ENCINFO_KEY);
		xmlAddChild(body_node, EncInfo_node);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].ID);
		upper_msg_package_add_xml_leaf(EncInfo_node, MSG_ENCINFOID_KEY, msgcode_buf);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].s_Status);
		upper_msg_package_add_xml_leaf(EncInfo_node, MSG_STATUS_KEY, msgcode_buf);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].s_Mute);
		upper_msg_package_add_xml_leaf(EncInfo_node, MSG_MUTE_KEY, msgcode_buf);
	}

	//UserID
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return ROOM_RETURN_SUCC;
}



int32_t lower_msg_package_req_status_info(int8_t *out_buf, RecStatusInfo *st_info)
{
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pxml = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;

	if(NULL == out_buf){
		nslog(NS_ERROR, "[%s]---params is NULL!", __func__);
		return ROOM_RETURN_FAIL;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//RequestMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	//MsgCode
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", MSG_ROOM_STATUS_REQ);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	//PassKey
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", MSG_RECORD_PASS);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//RecServerStatusUpdateReq
	xmlNodePtr	RecServerStatusUpdateReq_node = NULL;
	RecServerStatusUpdateReq_node = xmlNewNode(NULL, MSG_ROOMSTATUSREQ_KEY);
	xmlAddChild(body_node, RecServerStatusUpdateReq_node);

	//RoomStatus
	xmlNodePtr	RoomStatus_node = NULL;
	RoomStatus_node = xmlNewNode(NULL, MSG_ROOMSTATUS_KEY);
	xmlAddChild(RecServerStatusUpdateReq_node, RoomStatus_node);

	//RoomID
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->RoomID);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_ROOMID_KEY, msgcode_buf);

	//ConnStatus
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->ConnStatus);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_CONNECTSTATUS_KEY, msgcode_buf);

	//Quality
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->Quality);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_QUALITY_KEY, msgcode_buf);

	//RecStatus
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->RecStatus);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_RECSTATUS_KEY, msgcode_buf);

	//RecName
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", st_info->RecName);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_RECNAME_KEY, msgcode_buf);

	//IfMark
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->IfMark);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_IFMARK_KEY, msgcode_buf);

	//Status1
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->Status1);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_STATUS1_KEY, msgcode_buf);

	//Status2
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->Status2);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_STATUS2_KEY, msgcode_buf);

	//Status3
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->Status3);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_STATUS3_KEY, msgcode_buf);
	// add zl
	//Status4
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->Status4);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_STATUS4_KEY, msgcode_buf);
	//Status5
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->Status5);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_STATUS5_KEY, msgcode_buf);
	//Status6
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->Status6);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_STATUS6_KEY, msgcode_buf);
	//Status7
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->Status7);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_STATUS7_KEY, msgcode_buf);
	//Status8
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->Status8);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_STATUS8_KEY, msgcode_buf);
	//Status9
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", st_info->Status9);
	upper_msg_package_add_xml_leaf(RoomStatus_node, MSG_STATUS9_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", "0");
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return ROOM_RETURN_SUCC;
}

int32_t lower_msg_package_record_req(int8_t *out_buf, RecInfo *rec_info, int32_t opt_type)
{
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pxml = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;

	if(NULL == out_buf){
		nslog(NS_ERROR, "[%s]---params is NULL!", __func__);
		return ROOM_RETURN_FAIL;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//RequestMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	//MsgCode
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", MSG_RECORD_CTRL);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	//PassKey
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", MSG_ROOMCTRL_PASSKEY);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//RecCtrlReq
	xmlNodePtr	RecCtrlReq = NULL;
	RecCtrlReq = xmlNewNode(NULL, MSG_RECCTRLREQ_KEY);
	xmlAddChild(body_node, RecCtrlReq);

	//RoomID
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", 0);
	upper_msg_package_add_xml_leaf(RecCtrlReq, MSG_ROOMID_KEY, msgcode_buf);

	//RecordID
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", "RecordID");
	upper_msg_package_add_xml_leaf(RecCtrlReq, MSG_CONNECTSTATUS_KEY, msgcode_buf);

	//OptType
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", opt_type);
	upper_msg_package_add_xml_leaf(RecCtrlReq, MSG_OPTTYPE_KEY, msgcode_buf);

	//RoomName
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", "RoomName");
	upper_msg_package_add_xml_leaf(RecCtrlReq, MSG_ROOMNAME_KEY, msgcode_buf);

	//AliasName
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", "AliasName");
	upper_msg_package_add_xml_leaf(RecCtrlReq, MSG_ALIASNAME_KEY, msgcode_buf);

	//TeacherName
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", "TeacherName");
	upper_msg_package_add_xml_leaf(RecCtrlReq, MSG_TEACHERNAME_KEY, msgcode_buf);

	//CourseName
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", "CourseName");
	upper_msg_package_add_xml_leaf(RecCtrlReq, MSG_COURSENAME_KEY, msgcode_buf);

	//Notes
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", "Notes");
	upper_msg_package_add_xml_leaf(RecCtrlReq, MSG_NOTES_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", "0");
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return ROOM_RETURN_SUCC;
}

int32_t lower_msg_package_iFrame_req(int8_t *out_buf, int32_t room_id)
{
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pxml = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;

	if(NULL == out_buf){
		nslog(NS_ERROR, "[%s]---params is NULL!", __func__);
		return ROOM_RETURN_FAIL;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//RequestMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	//MsgCode
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", MSG_IFRAMEREQ_CTRL);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	//PassKey
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", MSG_ROOMCTRL_PASSKEY);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//IFrameReq
	xmlNodePtr	IFrameReq = NULL;
	IFrameReq = xmlNewNode(NULL, MSG_IFRAMEREQ_KEY);
	xmlAddChild(body_node, IFrameReq);

	//RoomID
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", room_id);
	upper_msg_package_add_xml_leaf(IFrameReq, MSG_ROOMID_KEY, msgcode_buf);

	//EncodeIndex
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", "123");
	upper_msg_package_add_xml_leaf(IFrameReq, MSG_ENCODEINDEX_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", "0");
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return ROOM_RETURN_SUCC;
}


void lower_msg_send_xml_to_enc(RoomMsgEnv *pRoom, int8_t *xml_buf)
{
	int32_t i				= 0;
	int32_t socket 			= -1;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex 	= NULL;
	MsgHeader head;

	if (!xml_buf) {
		nslog(NS_ERROR, "[%s] ---param error!", __func__);
		return;
	}
	r_bzero(&head, sizeof(MsgHeader));

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		}

		e_info = &pRoom->RoomInfo.EncInfo[i];

		//HD
		socket = e_info->HD_QuaInfo.Socket;
		mutex  = e_info->HD_QuaInfo.mutex;

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, &head, xml_buf);
		}
		nslog(NS_INFO, "[%s] --- HD -- EncNum = %d, i = %d, socket = %d, valid_stream_mask = %d, MsgQueType = %d", __func__, pRoom->RoomInfo.EncNum, i, socket, pRoom->RoomInfo.valid_stream_mask, e_info->HD_QuaInfo.MsgQueType);

		//JPEG
		if (e_info->EncodeType == EncodeType_JPEG) {
			nslog(NS_INFO, "Enc %d is JPEG, don't send record xml to it's SD, continue for next", e_info->ID);
			continue;
		}

		//SD
		socket = e_info->SD_QuaInfo.Socket;
		mutex  = e_info->SD_QuaInfo.mutex;

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, &head, xml_buf);
		}
		nslog(NS_INFO, "[%s] --- SD -- EncNum = %d, i = %d, socket = %d, valid_stream_mask = %d, MsgQueType = %d", __func__, pRoom->RoomInfo.EncNum, i, socket, pRoom->RoomInfo.valid_stream_mask, e_info->SD_QuaInfo.MsgQueType);
	}

}


int32_t lower_msg_get_platfrom_key(parse_xml_t *parseXml)
{
	xmlNodePtr 	pNode 	= NULL;
	int32_t		ret_val	= -1;
	int8_t 		value[ROOM_MSG_VAL_LEN] = {0};

	pNode = get_req_pass_key_node(&pNode, parseXml->proot);
	if (!pNode) {
		nslog(NS_ERROR, "[%s] ---[get_req_pass_key_node] error!", __func__);
		return -1;
	}

	r_bzero(value, ROOM_MSG_VAL_LEN);
	get_current_node_value(value, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode);

	if (!r_strcmp(value, MSG_MEDIACENTER_PASSKEY)) {
		ret_val = MediaCenter;
	}
	else if (!r_strcmp(value, MSG_MANAGEPLAT_PASSKEY)) {
		ret_val = ManagePlatform;
	}
	else if (!r_strcmp(value, MSG_RECORD_PASS)) {
		ret_val = RecServer;
	}
	else if (!r_strcmp(value, MSG_LIVENODE_PASSKEY)) {
		ret_val = LiveNode;
	}
	else if (!r_strcmp(value, MSG_COMCTRL_PASSKEY)) {
		ret_val = ComControl;
	}
	else if (!r_strcmp(value, MSG_THIRDCTRL_PASSKEY)) {
		ret_val = ThirdControl;
	}
	else if (!r_strcmp(value, MSG_ALLPLATFORM_PASSKEY)) {
		ret_val = AllPlatform;
	}
	else if (!r_strcmp(value, MSG_PLATFORM_DIRECTOR_PASSKEY)) {
		ret_val = Director;
	}
	else {
		ret_val = -1;
	}

	return ret_val;
}

int32_t lower_msg_get_record_platfrom_key(parse_xml_t *parseXml)
{
	xmlNodePtr 	pNode 	= NULL;
	int32_t		ret_val	= -1;
	int8_t 		value[ROOM_MSG_VAL_LEN] = {0};

	pNode = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!pNode) {
		nslog(NS_ERROR, "[%s] ---[get_req_pass_key_node] error!", __func__);
		return -1;
	}

	pNode = get_children_node(pNode, MSG_RECORD_REP_KEY);
	if (!pNode) {
		nslog(NS_ERROR, "[%s] ---[get_req_pass_key_node] error!", __func__);
		return -1;
	}

	pNode = get_children_node(pNode, MSG_PASSKEY_KEY);
	if (!pNode) {
		nslog(NS_ERROR, "[%s] ---[get_req_pass_key_node] error!", __func__);
		return -1;
	}

	r_bzero(value, ROOM_MSG_VAL_LEN);
	get_current_node_value(value, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode);

	if (!r_strcmp(value, MSG_MEDIACENTER_PASSKEY)) {
		ret_val = MediaCenter;
	}
	else if (!r_strcmp(value, MSG_MANAGEPLAT_PASSKEY)) {
		ret_val = ManagePlatform;
	}
	else if (!r_strcmp(value, MSG_RECORD_PASS)) {
		ret_val = RecServer;
	}
	else if (!r_strcmp(value, MSG_LIVENODE_PASSKEY)) {
		ret_val = LiveNode;
	}
	else if (!r_strcmp(value, MSG_COMCTRL_PASSKEY)) {
		ret_val = ComControl;
	}
	else if (!r_strcmp(value, MSG_THIRDCTRL_PASSKEY)) {
		ret_val = ThirdControl;
	}
	else if (!r_strcmp(value, MSG_ALLPLATFORM_PASSKEY)) {
		ret_val = AllPlatform;
	}
	else {
		ret_val = -1;
	}

	return ret_val;
}

int32_t lower_msg_get_room_info_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	MsgHeader	out_head;
	int8_t file[ROOM_MSG_VAL_LEN] = {0};

	int8_t *out_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!out_buf) {
		nslog(NS_ERROR, "[%s] ---[r_malloc] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	if (!lower_msg_get_return_code(parseXml)) {
		nslog(NS_ERROR, " ---[lower_msg_get_return_code = 0] error!");
		return ROOM_RETURN_FAIL;
	}

	//存储编码器信息
	lower_msg_save_enc_info(pRoom, xml, msgq->msgtype);

	pRoom->RoomInfo.room_info_mask |= (1 << msgq->msgtype);

	//更新录制状态和录制名称
	if (pRoom->record_handle) {
		pRoom->RoomInfo.RecordStatus = lower_msg_opt_type_trans(pRoom->record_handle->get_record_status(pRoom->record_handle));
		r_strcpy(pRoom->RoomInfo.RcdName, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
	} else {
		pRoom->RoomInfo.RecordStatus = 0;
		r_strcpy(pRoom->RoomInfo.RcdName, pRoom->RecInfo.CourseName);
	}

	//有改变就保存 到文件
	r_bzero(out_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_resp_room_info(pRoom, out_buf, MSG_GET_ROOM_INFO, ROOM_RETURN_SUCC, MSG_RECORD_PASS, "0");

//	sprintf(file, "%s_%d.xml", MSG_ROOM_CONF_FILE_PREFIX, pRoom->RoomInfo.RoomId);
//	upper_msg_write_file(file, out_buf);

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		//编码器主动上报的就直接封包上报
//		upper_msg_package_resp_room_info(pRoom, out_buf, MSG_GET_ROOM_INFO, ROOM_RETURN_SUCC, MSG_RECORD_PASS, "0");

		//上报消息
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, &out_head, out_buf);

		if (out_buf)
			r_free(out_buf);

		return ROOM_RETURN_SUCC;
	}

	//标记哪些编码器已上报信息
	pRoom->roomInfoMask[index] &= ~(1 << msgq->msgtype);

	if (pRoom->roomInfoMask[index] == MSG_QUEUE_REC_CLR) {
		upper_msg_time_que_del_node(pRoom, parseXml, head);
	}

	if (out_buf)
		r_free(out_buf);

	return ROOM_RETURN_SUCC;
}

int32_t lower_msg_resp_xml_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, uint32_t *mask, uint32_t msgcode)
{
	//标记哪些编码器已上报信息
	*mask &= ~(1 << msgq->msgtype);

	//所有回应为成功，才回应成功
	if (upper_msg_get_mask_bit_val(*mask, MSG_QUEUE_REC_FLG_BIT)) {
		int32_t ret = lower_msg_get_return_code(parseXml);
		if (!ret) {
			upper_msg_set_mask_bit_val(mask, MSG_QUEUE_REC_FLG_BIT, 0);
		}
	}

	//等待所有ENC 回应后才向上回应消息
	if (upper_msg_is_all_enc_resp(*mask)) {
		int32_t ret_value 		= 0;
		MsgHeader *head 		= NULL;
		xmlNodePtr pNode_tmp	= NULL;
		int8_t user_id[ROOM_MSG_VAL_LEN] 		= {0};
		int8_t ret_code_buf[ROOM_MSG_VAL_LEN] 	= {0};
		int8_t ret_pass_key[ROOM_MSG_VAL_LEN] 	= {0};


		int8_t *xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
		if (!xml_buf) {
			nslog(NS_ERROR, "[%s] ---[r_malloc] error!", __func__);
			return ROOM_RETURN_FAIL;
		}

		if (upper_msg_get_mask_bit_val(*mask, MSG_QUEUE_REC_FLG_BIT)) {
			ret_value = 1;
		} else {
			ret_value = 0;
		}

		head = (MsgHeader *)msgq->msgbuf;
		sprintf(ret_code_buf, "%d", ret_value);

		//连接教室从超时队列获取pass key
		if (msgcode == MSG_CONNECT_ROOM_REQ) {
			sprintf(ret_code_buf, "%d", 1);
			ret_value = lower_msg_get_passkey_from_que(pRoom, msgcode, ret_pass_key);
		} else {
			pNode_tmp = get_req_pass_key_node(&pNode_tmp, parseXml->proot);
			ret_value = get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode_tmp);
			if (ret_value == 0)
				ret_value = ROOM_RETURN_SUCC;
			else
				ret_value = ROOM_RETURN_FAIL;
		}

		//获取失败，用RecServer 回复
		if (ret_value == ROOM_RETURN_FAIL) {
			r_bzero(ret_pass_key, ROOM_MSG_VAL_LEN);
			r_strcpy(ret_pass_key, MSG_RECORD_PASS);
		}

		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		if (lower_msg_get_userid_from_que(pRoom, msgcode, user_id) == ROOM_RETURN_FAIL)
			upper_msg_get_user_id_value(parseXml, user_id);

		if (msgcode == MSG_CONNECT_ROOM_REQ)
			upper_msg_package_connc_room_req_xml(xml_buf, msgcode, ret_code_buf, ret_pass_key, 1, user_id, pRoom->RoomInfo.RoomId);
		else
			upper_msg_package_reponse_xml(xml_buf, msgcode, ret_code_buf, ret_pass_key, user_id, pRoom->RoomInfo.RoomId);
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);
		*mask = MSG_QUEUE_REC_ALL;
		upper_msg_time_que_del_node(pRoom, parseXml, head);

		if (xml_buf)
			r_free(xml_buf);
	}

	return ROOM_RETURN_SUCC;
}

int32_t lower_msg_set_quality_info_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t retVal = ROOM_RETURN_SUCC;
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index >= 0) {
		retVal = lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->qualityMask[index], MSG_SET_QUALITY_INFO);
	}

	if (lower_msg_get_return_code(parseXml)) {
		//请求教室信息
		lower_msg_req_room_info(pRoom, msgq, head, MSG_ROOMCTRL_PASSKEY);

		//如果在录制，且该路流请求过码流的，则锁屏
		if (pRoom->record_handle && pRoom->RoomInfo.req_stream_mask & (1 << msgq->msgtype)) {
			lower_msg_ctrl_enc_lock_req(pRoom, msgq, head, 1);

			//强制I 帧
			lower_msg_iFrame_req(pRoom, msgq->msgtype);
		}
	}

	return retVal;

}

int32_t lower_msg_set_audio_info_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t retVal = ROOM_RETURN_FAIL;
	int32_t index  = lower_msg_get_platfrom_key(parseXml);
	if (index >= 0) {
		retVal = lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->audioMask[index], MSG_SET_AUDIO_INFO);
	}

	if (lower_msg_get_return_code(parseXml)) {
		//请求教室信息
		lower_msg_req_room_info(pRoom, msgq, head, MSG_ROOMCTRL_PASSKEY);
	}

	return retVal;
}


int32_t lower_msg_record_ctrl_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index;
	int32_t Result;
	int8_t	*xml_buf	= NULL;

	//不是开始录制的锁分辨率操作，不处理
	Result = lower_msg_get_record_result(pRoom, parseXml);
	if (Result != MSG_RECORD_CTRL_START) {
		nslog(NS_INFO, " ---status = %d, it is not MSG_RECORD_CTRL_START, just return!", Result);
		return ROOM_RETURN_SUCC;
	}

	index = lower_msg_get_platfrom_key(parseXml);
	if (index < 0) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	//标记哪些编码器已上报信息
	pRoom->record_mask[index] &= ~(1 << msgq->msgtype);

	//所有回应为成功，才回应成功
	if (upper_msg_get_mask_bit_val(pRoom->record_mask[index], MSG_QUEUE_REC_FLG_BIT)) {
		int32_t ret = lower_msg_get_return_code(parseXml);
		if (!ret) {
			upper_msg_set_mask_bit_val(pRoom->record_mask[index], MSG_QUEUE_REC_FLG_BIT, 0);
		}
	}

	nslog(NS_INFO, "[%s] ---record_mask[%d] = %d, msgq->msgtype = %d, OptType = %d \n", __func__, index, pRoom->record_mask[index], msgq->msgtype,  pRoom->RecInfo.OptType);

	//等待所有ENC 回应后才向上回应消息
	if (upper_msg_is_all_enc_resp(pRoom->record_mask[index])) {
		int32_t ret_value 	= 0;

		pRoom->record_mask[index] = MSG_QUEUE_REC_FLG;

		if (upper_msg_get_mask_bit_val(pRoom->record_mask[index], MSG_QUEUE_REC_FLG_BIT)) {
			ret_value = 1;
		} else {
			ret_value = 0;
		}

		//启动录制
		if (ret_value && Result == MSG_RECORD_CTRL_START){
			ret_value = lower_msg_start_record(pRoom);
		}

		if (pRoom->record_handle) {
			pRoom->RecStatusInfo.RecStatus = lower_msg_opt_type_trans(pRoom->record_handle->get_record_status(pRoom->record_handle));
			r_strcpy(pRoom->RecStatusInfo.RecName, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
		} else {
			pRoom->RecStatusInfo.RecStatus = 0;
			r_strcpy(pRoom->RecStatusInfo.RecName, pRoom->RecInfo.CourseName);
		}

		//回应
		lower_msg_start_record_resp(pRoom, parseXml, msgq, ret_value);
		upper_msg_time_que_del_node(pRoom, parseXml, head);

		xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
		if (!xml_buf) {
			nslog(NS_ERROR, "[%s] ---[r_malloc] error!", __func__);
			return ROOM_RETURN_FAIL;
		}

		//打开录制不成功，则告诉编码器解锁分辨率
		if(!ret_value) {
			nslog(NS_WARN, "--- start record error, send stop xml to enc! ret_value = %d\n", ret_value);

			r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
			lower_msg_package_record_req(xml_buf, NULL, 0);
			lower_msg_send_xml_to_enc(pRoom, xml_buf);
		}

		//回复状态
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		lower_msg_package_req_status_info(xml_buf, &pRoom->RecStatusInfo);
		nslog(NS_ERROR,"WAHT ----1\n");
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

		if (xml_buf)
			r_free(xml_buf);

	}

	return ROOM_RETURN_SUCC;
}


int32_t lower_msg_request_code_stream_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	int32_t ret_code_stream_code = upper_msg_get_mask_bit_val(pRoom->StrmReqMask[index], MSG_QUEUE_REC_FLG_BIT);

	//标记哪些编码器已上报信息
	pRoom->StrmReqMask[index] &= ~(1 << msgq->msgtype);

	//请求成功后标记
	pRoom->RoomInfo.valid_stream_mask |= (1 << msgq->msgtype);

//	//请求码流成功后，马上发消息请求编码器信息，以更新本地记录
//	lower_msg_req_room_info(pRoom, msgq, head, MSG_ROOMCTRL_PASSKEY);

//	//如果在录制，且该路流请求过码流的，则锁屏
//	if (pRoom->record_handle && pRoom->RoomInfo.req_stream_mask & (1 << msgq->msgtype)) {
//		lower_msg_ctrl_enc_lock_req(pRoom, msgq, head, 1);
//	}

	//所有回应为成功，才回应成功
	if (ret_code_stream_code == 1) {
		ret_code_stream_code = lower_msg_get_return_code(parseXml);
		if (!ret_code_stream_code) {
			upper_msg_set_mask_bit_val(&pRoom->StrmReqMask[index], MSG_QUEUE_REC_FLG_BIT, 0);
		}
	}

	//等待所有ENC 回应后才向上回应消息
	if (upper_msg_is_all_enc_resp(pRoom->StrmReqMask[index]))
	{
		int8_t ret_code[] = "0";
		int8_t *xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
		if (!xml_buf) {
			nslog(NS_ERROR, "[%s] ---[r_malloc] error!", __func__);
			return ROOM_RETURN_FAIL;
		}

		//直播模块也为成功才回应成功
		ret_code_stream_code = upper_msg_get_mask_bit_val(pRoom->StrmReqMask[index], MSG_QUEUE_REC_FLG_BIT);
		if (ret_code_stream_code == 1) {
			ret_code_stream_code = pRoom->StrmReqStatus[index];
		}

		sprintf(ret_code, "%d", ret_code_stream_code);

		int8_t user_id[ROOM_MSG_VAL_LEN]	  = {0};
		int8_t ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
		xmlNodePtr tmp_node = NULL;
		get_req_pass_key_node(&tmp_node, parseXml->proot);
		get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		upper_msg_get_user_id_value(parseXml, user_id);
		upper_msg_package_reponse_xml(xml_buf, MSG_REQUEST_CODE_STREAM, ret_code, ret_pass_key, user_id, pRoom->RoomInfo.RoomId);

		pRoom->StrmReqStatus[index]	= 1;
		pRoom->StrmReqMask[index]	= MSG_QUEUE_REC_FLG;
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

		upper_msg_time_que_del_node(pRoom, parseXml, head);

		if (xml_buf)
			r_free(xml_buf);
	}


	return ROOM_RETURN_SUCC;
}

int32_t lower_msg_remote_ctrl_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	return lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->remoteCtrlMask[index], MSG_REMOTE_CTRL);
}


int32_t lower_msg_iFrame_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	return lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->iframeMask[index], MSG_IFRAMEREQ_CTRL);
}

int32_t lower_msg_send_logo_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	return lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->logoMask[index], MSG_SEND_LOGO_PIC);
}

int32_t lower_msg_add_title_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	return lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->titleMask[index], MSG_ADD_TITLE_REQ);
}

int32_t lower_msg_volume_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t ret_val = ROOM_RETURN_FAIL;
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index >= 0) {
		pRoom->volumeMask[index] = MSG_QUEUE_REC_CLR;
		upper_msg_time_que_del_node(pRoom, parseXml, head);
	}

	ret_val = upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml);

	return ret_val;
}

int32_t lower_msg_mute_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t ret_val = ROOM_RETURN_FAIL;
	int32_t index   = 0;

//	if (lower_msg_get_return_code(parseXml)) {
//		pRoom->RoomInfo.AudioInfo.Lvolume = 0;
//		pRoom->RoomInfo.AudioInfo.Rvolume = 0;
//	}

 	index = lower_msg_get_platfrom_key(parseXml);
	if (index >= 0) {
		pRoom->muteMask[index] = MSG_QUEUE_REC_CLR;
		upper_msg_time_que_del_node(pRoom, parseXml, head);
	}

	ret_val = upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml);

	return ret_val;
}

int32_t lower_msg_pic_adjust_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	return lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->adjustMask[index], MSG_PICADJUST_REQ);
}

// add zl
int32_t lower_msg_get_camctl_ctrl_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	return lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->setRemoteCtrlMask[index], MSGCODE_GET_CAMCTL_PROLIST);
}

int32_t lower_msg_set_camctl_ctrl_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	return lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->getRemoteCtrlMask[index], MSGCODE_SET_CAMCTL_PRO);
}

int32_t lower_msg_image_synthesis_req_node(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	return lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->getPictureSynMask[index], MSG_ENC_IMAGE_SYNTHESIS);
}

int32_t lower_msg_director_mode_ctrl_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, "[%s] ---[lower_msg_get_platfrom_key] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	return lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->setRemoteModeMask[index], MSG_ECN_DIRECTOR_MODE);
}







int32_t lower_msg_enc_login_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int8_t 			ip_buf[IP_LEN]	= {0};
	enc_info 		*e_info 	= NULL;
	quality_info  	*q_info 	= NULL;
	stream_handle 	*stream_hdl = NULL;
	int32_t 		ret_val 	= 0;
	int32_t 		i 			= 0;
	int32_t 		num			= 0;
	int8_t 			*xml_buf	= NULL;

	//标记哪些编码器已上报信息
	pRoom->loginMask &= ~(1 << msgq->msgtype);

	q_info = lower_msg_get_quality_info_by_msgq_type(pRoom, msgq->msgtype);
	if (!q_info) {
		nslog(NS_ERROR, "get quality info error");
		return ROOM_RETURN_FAIL;
	}

	q_info->Status = 1;

	e_info = lower_msg_get_enc_info_by_msgq_type(pRoom, msgq->msgtype);
	if (!e_info) {
		nslog(NS_ERROR, "get enc info error");
		return ROOM_RETURN_FAIL;
	}

	lower_msg_save_enc_login_info(&e_info->LoginInfo, xml);

	e_info->EncodeType = atoi(e_info->LoginInfo.EncodeType);
	if (e_info->HD_QuaInfo.Status == 1 || e_info->SD_QuaInfo.Status == 1) {
		e_info->Status = 1;
	} else {
		e_info->Status = 0;
	}

	#if 0   // add zl
	//JPEG只有一路
	if (e_info->EncodeType == EncodeType_JPEG && e_info->HD_QuaInfo.Status == 1) {
		e_info->Status = 1;
	} else if (e_info->EncodeType == EncodeType_JPEG && e_info->HD_QuaInfo.Status == 0){
		e_info->Status = 0;
	}

	//如果有两路JPEG， 或者JPEG不是最后一路，断开教室连接
	if (e_info->EncodeType == EncodeType_JPEG) {
		ret_val = lower_msg_check_enc(pRoom);
		if (ret_val == ROOM_RETURN_FAIL) {
			nslog(NS_ERROR, "lower_msg_check_enc error, close_room_connect.....");

			//断开教室
			ret_val = pRoom->handle->close_room_connect(pRoom->handle);
			if (ret_val == 0) {
				pRoom->RoomInfo.ConnectStatus		= 0;
				pRoom->RecStatusInfo.ConnStatus 	= 0;
				pRoom->RoomInfo.req_stream_mask 	= MSG_QUEUE_REC_CLR;
				pRoom->RoomInfo.room_info_mask		= MSG_QUEUE_REC_CLR;
				pRoom->RoomInfo.valid_stream_mask	= MSG_QUEUE_REC_CLR;

				for (i = 0; i < ROOM_ENC_NUM; i++) {
					pRoom->RoomInfo.EncInfo[i].Status	= 0;
					pRoom->RoomInfo.EncInfo[i].s_Status = 0;
					pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.Status = 0;
					pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.Status = 0;
				}
			}

			return ROOM_RETURN_FAIL;
		}
	}

	#endif

	//教室连接状态
	for (i = 0, num = 0; i < ROOM_ENC_NUM; i ++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		}

		if (pRoom->RoomInfo.EncInfo[i].Status == 1)
			num++;
	}

//	if (num < pRoom->RoomInfo.EncNum) {
	if(num == 0) {
		pRoom->RoomInfo.ConnectStatus = 0;
		pRoom->RecStatusInfo.ConnStatus = 0;
	} else {
		pRoom->RoomInfo.ConnectStatus = 1;
		pRoom->RecStatusInfo.ConnStatus = 1;
	}


	msg_ctrl_inet_ntoa(ip_buf, e_info->EncIP);
	stream_hdl = lower_msg_get_stream_handle(pRoom, ip_buf, msgq->msgtype);
	if (stream_hdl) {
		q_info->Socket	=	stream_hdl->sockfd;
		q_info->mutex	=	&stream_hdl->mutex;
//		q_info->MsgQueType	= msgq->msgtype;
	} else {
		q_info->Socket	=	-1;
		return ROOM_RETURN_FAIL;
	}

	nslog(NS_INFO, "Login ... msgtype = %d, socket = %d, ID = %d, EncodeType = %d", msgq->msgtype, q_info->Socket, e_info->ID, e_info->EncodeType);

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, "malloc error");
	}

//	if (e_info->EncodeType == EncodeType_JPEG &&\
//			  (pRoom->RoomInfo.req_stream_mask & (1 << e_info->HD_QuaInfo.MsgQueType) || \
//			   pRoom->RoomInfo.req_stream_mask & (1 << e_info->SD_QuaInfo.MsgQueType))){
//		pRoom->RoomInfo.req_stream_mask |= (1 << e_info->HD_QuaInfo.MsgQueType);
//		pRoom->RoomInfo.req_stream_mask &= ~(1 << e_info->SD_QuaInfo.MsgQueType);
//	}

//	if (xml_buf && e_info->EncodeType == EncodeType_JPEG && e_info->HD_QuaInfo.Socket > 0 &&\
//			  pRoom->RoomInfo.req_stream_mask & (1 << e_info->HD_QuaInfo.MsgQueType)) {
//		nslog(NS_INFO, " ---[request stream again, JPEG] !");

//		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
//		upper_msg_package_req_stream_info(pRoom, xml_buf, "0", e_info->HD_QuaInfo.RateType);
//		upper_msg_tcp_send(e_info->HD_QuaInfo.Socket, e_info->HD_QuaInfo.mutex, head, xml_buf);
//	}

	//如果是JPEG，则关闭第二路流
	if (e_info->EncodeType == EncodeType_JPEG) {
		if (e_info->SD_QuaInfo.Socket > 0) {
			nslog(NS_WARN, "close socket = %d, msgq->msgtype = %d\n", e_info->SD_QuaInfo.Socket, msgq->msgtype);
			stream_handle	*stream_hdl_close = NULL;

			//取第二路流
			stream_hdl_close = lower_msg_get_stream_handle(pRoom, ip_buf, e_info->SD_QuaInfo.MsgQueType);
			if (stream_hdl_close)
				pRoom->handle->close_stream_connect(stream_hdl_close);
			e_info->SD_QuaInfo.Socket = -1;
			e_info->SD_QuaInfo.Status = 0;
			pRoom->RoomInfo.room_info_mask		&= ~(1 << e_info->SD_QuaInfo.MsgQueType);
			pRoom->RoomInfo.req_stream_mask 	&= ~(1 << e_info->SD_QuaInfo.MsgQueType);
			pRoom->RoomInfo.valid_stream_mask	&= ~(1 << e_info->SD_QuaInfo.MsgQueType);
		}
	}

	//登录回应后，设置直播信息
	ret_val = lower_msg_set_live_info(pRoom, msgq->msgtype);
	nslog(NS_INFO, "ret_val = %d, q_info->Socket = %d", ret_val, q_info->Socket);
	if (xml_buf && ret_val == 1 && q_info->Socket > 0) {
		nslog(NS_INFO, " ---[request stream again] !");

		//登录成功后，如果该路有直播用户，则请求码流
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		upper_msg_package_req_stream_info(pRoom, xml_buf, "0", q_info->RateType);
		upper_msg_tcp_send(q_info->Socket, q_info->mutex, head, xml_buf);
		pRoom->RoomInfo.req_stream_mask |= (1 << msgq->msgtype);
	}

	//登录上后，设置高低质量带宽(只设置高码流)
	if (xml_buf) {
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		ret_val = upper_msg_package_set_quality_info(pRoom, xml_buf, "0", e_info->ID);
		if (ret_val == ROOM_RETURN_SUCC) {
			upper_msg_tcp_send(q_info->Socket, q_info->mutex, head, xml_buf);
		} else {
			//请求教室信息
			lower_msg_req_room_info(pRoom, msgq, head, MSG_ROOMCTRL_PASSKEY);
		}
	}

	if (xml_buf)
		r_free(xml_buf);

	return lower_msg_resp_xml_deal(pRoom, parseXml, msgq, &pRoom->loginMask, MSG_CONNECT_ROOM_REQ);
}


int32_t lower_msg_enc_restart_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{

	return ROOM_RETURN_SUCC;
}

int32_t lower_msg_enc_update_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{

	return ROOM_RETURN_SUCC;
}


int32_t lower_msg_record_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t i 			= 0;
	int32_t status 		= 0;
	int32_t rec_return_code 			= 0;
	int8_t  rec_id[ROOM_MSG_VAL_LEN] 	= {0};

	//如录制完成，则停止录制
	if (pRoom->record_handle) {
		//获取录制状态
		status = lower_msg_get_record_status(pRoom, parseXml);

		nslog(NS_INFO, "===== record OptType = %d, msgq->msgtype = %d\n", status, msgq->msgtype);

		if (status == MSG_RECORD_CTRL_STOP) {
			int8_t *xml_buf = NULL;

			//更新录制课件名称
			if (pRoom->record_handle) {
				r_strcpy(pRoom->RecStatusInfo.RecName, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
				r_strcpy(rec_id, pRoom->record_handle->get_RecordID(pRoom->record_handle));
			} else {
				r_strcpy(pRoom->RecStatusInfo.RecName, pRoom->RecInfo.CourseName);
				r_strcpy(rec_id, pRoom->RecInfo.RecordID);
			}

			//关闭录制流
			for (i = 0; i < pRoom->handle->stream_num; i++) {
				pRoom->handle->set_rec_status(&pRoom->handle->stream_hand[i], DISCONNECT);
			}

			//关闭录制
			pRoom->record_handle->record_close(pRoom->record_handle);
			unregister_course_record_mode(&pRoom->record_handle);
			pRoom->record_handle = NULL;
			rec_return_code = 1;

			//记录操作状态到本地
			pRoom->RecStatusInfo.RecStatus 	= 0;
			pRoom->RecInfo.OptType 			= status;
			pRoom->RoomInfo.record_stream_mask = 0;

			//记录停止时间
//			pRoom->RecInfo.RecordStopTime.tv_sec 	= 0;
//			pRoom->RecInfo.RecordStopTime.tv_usec 	= 0;
//			gettimeofday(&pRoom->RecInfo.RecordStopTime, 0);

			//告诉编码器解锁分辨率
			xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
			if (!xml_buf) {
				nslog(NS_ERROR, "[%s] ---[r_malloc] error!", __func__);
				return ROOM_RETURN_FAIL;
			}

			r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
			lower_msg_package_record_req(xml_buf, NULL, 0);
			lower_msg_send_xml_to_enc(pRoom, xml_buf);

			//回复服务端
			r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
			upper_msg_package_record_xml(xml_buf, rec_return_code, pRoom->RecStatusInfo.RecName, MSG_ALLPLATFORM_PASSKEY, pRoom->RecInfo.userid, rec_id, pRoom->RecInfo.OptType, pRoom->RecStatusInfo.RecStatus);
			upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

			//回复状态
			r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
			lower_msg_package_req_status_info(xml_buf, &pRoom->RecStatusInfo);
			nslog(NS_ERROR,"WAHT ----2\n");
			upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

			if (xml_buf)
				r_free(xml_buf);

//			return upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml);
		}
	}

	return ROOM_RETURN_SUCC;
}


int32_t lower_msg_enc_status_info_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t status 		= 0;
	int32_t ret_status	= 0;
	int8_t 	*xml_buf 	= NULL;
	quality_info *q_info= NULL;

	//存储到本地
	ret_status = lower_msg_save_enc_status_info(pRoom, xml, msgq->msgtype);

	//回复心跳
	q_info = lower_msg_get_quality_info_by_msgq_type(pRoom, msgq->msgtype);
	if (q_info) {
		if (q_info->Socket > 0)
			upper_msg_tcp_send(q_info->Socket, q_info->mutex, head, xml);

		upper_msg_set_time(&q_info->heart_time);
	}

	//判断是否超时
	status = upper_msg_monitor_time_out_status(&pRoom->RoomInfo.s_time, ENC_STATUS_TIME_OUT_VAL);
	if (status == ROOM_RETURN_TIMEOUT || ret_status == ROOM_RETURN_TIMEOUT) {
		xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
		if (!xml_buf) {
			nslog(NS_ERROR, " ---[r_malloc] error!");
			return ROOM_RETURN_FAIL;
		}

		//封包上报
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		lower_msg_package_enc_status_info(pRoom, xml_buf, MSG_ENC_STATUSINFO_REQ, MSG_RECORD_PASS, "0");
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

		pRoom->RoomInfo.s_time.tv_sec	= 0;
		pRoom->RoomInfo.s_time.tv_usec	= 0;
		gettimeofday(&pRoom->RoomInfo.s_time, 0);

		if (xml_buf)
			r_free(xml_buf);
	} else if (status == ROOM_RETURN_SUCC) {
		return ROOM_RETURN_SUCC;
	} else if (status == ROOM_RETURN_FAIL) {
		pRoom->RoomInfo.s_time.tv_sec	= 0;
		pRoom->RoomInfo.s_time.tv_usec	= 0;
		gettimeofday(&pRoom->RoomInfo.s_time, 0);
		return ROOM_RETURN_SUCC;
	}

	return ROOM_RETURN_SUCC;
}


int32_t lower_msg_status_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq, MsgHeader *head, int8_t *xml)
{
	int32_t 		i 			= 0;
	int32_t 		num			= 0;
	int32_t 		ret_val 	= 0;
	quality_info  	*q_info 	= NULL;
	enc_info		*e_info		= NULL;
	stream_handle   *stream_hdl	=NULL;
	RecStatusInfo 	st_info;
	int8_t 			ip_buf[IP_LEN]	= {0};

	//更新本地流状态
	ret_val = lower_msg_get_enc_status(parseXml);
	if (ret_val == -1) {
		nslog(NS_ERROR, "get enc status error");
		return ROOM_RETURN_FAIL;
	}

	q_info = lower_msg_get_quality_info_by_msgq_type(pRoom, msgq->msgtype);
	if (!q_info) {
		nslog(NS_ERROR, "get quality info error");
		return ROOM_RETURN_FAIL;
	}

	q_info->Status = ret_val;

	//改变状态后，重置状态标记
	if (ret_val) {
		upper_msg_set_time(&q_info->heart_time);
		;//pRoom->RoomInfo.valid_stream_mask |= (1 << msgq->msgtype);
	} else {
		pRoom->RoomInfo.room_info_mask &= ~(1 << msgq->msgtype);
		pRoom->RoomInfo.valid_stream_mask &= ~(1 << msgq->msgtype);
	}

	//流状态改变，更新编码器状态
	e_info = lower_msg_get_enc_info_by_msgq_type(pRoom, msgq->msgtype);
	if (!e_info) {
		nslog(NS_ERROR, "get enc info error");
		return ROOM_RETURN_FAIL;
	}

	if (e_info->HD_QuaInfo.Status == 1 || e_info->SD_QuaInfo.Status == 1) {
		e_info->Status = 1;
	} else {
		e_info->Status = 0;
		//e_info->EncodeType = EncodeType_INVALID;
	}

	//JPEG只有一路
	if (e_info->EncodeType == EncodeType_JPEG && e_info->HD_QuaInfo.Status == 1) {
		e_info->Status = 1;
	} else if (e_info->EncodeType == EncodeType_JPEG && e_info->HD_QuaInfo.Status == 0){
		e_info->Status = 0;
		//e_info->EncodeType = EncodeType_INVALID;
	}

	//教室连接状态
	for (i = 0, num = 0; i < ROOM_ENC_NUM; i ++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		}

		if (pRoom->RoomInfo.EncInfo[i].Status == 1)
			num++;
	}

//	if (num < pRoom->RoomInfo.EncNum) {
	if(num == 0) {
		pRoom->RoomInfo.ConnectStatus = 0;
		pRoom->RecStatusInfo.ConnStatus = 0;
	} else {
		pRoom->RoomInfo.ConnectStatus = 1;
		pRoom->RecStatusInfo.ConnStatus = 1;
	}

	nslog(NS_INFO, "ret_val = %d, q_info->Status = %d, e_info->Status = %d, e_info->EncodeType = %d, ConnectStatus = %d", ret_val, q_info->Status, e_info->Status, e_info->EncodeType, pRoom->RoomInfo.ConnectStatus);

	//流状态改变，更新本地流信息
	msg_ctrl_inet_ntoa(ip_buf, e_info->EncIP);
	stream_hdl = lower_msg_get_stream_handle(pRoom, ip_buf, msgq->msgtype);
	if (!stream_hdl) {
		nslog(NS_WARN, "get stream handle error");
//		return ROOM_RETURN_FAIL;
	}

	if (stream_hdl) {
		q_info->Socket	=	stream_hdl->sockfd;
		q_info->mutex	=	&stream_hdl->mutex;
//		q_info->MsgQueType	= msgq->msgtype;
	} else {
		q_info->Socket	=	-1;
	}

	st_info.RoomID 		= pRoom->RoomInfo.RoomId;
	st_info.ConnStatus  = pRoom->RoomInfo.ConnectStatus;
	st_info.Status1		= pRoom->RoomInfo.EncInfo[0].Status;
	st_info.Status2		= pRoom->RoomInfo.EncInfo[1].Status;
	st_info.Status3		= pRoom->RoomInfo.EncInfo[2].Status;

	// add zl
	st_info.Status4		= pRoom->RoomInfo.EncInfo[4].Status;
	st_info.Status5		= pRoom->RoomInfo.EncInfo[5].Status;
	st_info.Status6		= pRoom->RoomInfo.EncInfo[6].Status;
	st_info.Status7		= pRoom->RoomInfo.EncInfo[7].Status;
	st_info.Status8		= pRoom->RoomInfo.EncInfo[8].Status;
	st_info.Status9		= pRoom->RoomInfo.EncInfo[9].Status;


	st_info.IfMark		= 1;
	st_info.Quality  	= 1;

	if (pRoom->record_handle) {
		st_info.RecStatus = lower_msg_opt_type_trans(pRoom->record_handle->get_record_status(pRoom->record_handle));
		r_strcpy(st_info.RecName, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
	} else {
		st_info.RecStatus = 0;
		r_strcpy(st_info.RecName, pRoom->RecInfo.CourseName);
	}

	r_memcpy(&pRoom->RecStatusInfo, &st_info, sizeof(RecStatusInfo));

	int8_t *xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, "[%s] ---[r_malloc] error!", __func__);
		return ROOM_RETURN_FAIL;
	}

	//重新连接后，如果之前有请求码流的，重新请求码流
	if (ret_val && pRoom->RoomInfo.req_stream_mask & (1 << msgq->msgtype)) {
		nslog(NS_INFO, " ---[request stream again] !");
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		upper_msg_package_req_stream_info(pRoom, xml_buf, "0", q_info->RateType);
		upper_msg_tcp_send(q_info->Socket, q_info->mutex, head, xml_buf);
	}

	//回复状态改变
	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	lower_msg_package_req_status_info(xml_buf, &st_info);

	nslog(NS_ERROR,"WAHT ----3\n");
	// debug zl
//	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);


	if (xml_buf)
		r_free(xml_buf);

	return ROOM_RETURN_SUCC;
}


//deal the response xml msg
int32_t lower_msg_deal_response(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq)
{
	int32_t status 			= ROOM_RETURN_SUCC;
	int32_t tcp_socket 		= 0;
	int32_t msgCodeValue 	= 0;

	MsgHeader *head = (MsgHeader *)(msgq->msgbuf);
	int8_t *xml  = (int8_t *)msgq->msgbuf + sizeof(MsgHeader);

	UpperMsgEnv *upperMsgEnv = &(pRoom->upperEnv);
	tcp_socket = upperMsgEnv->tcp_socket;

	msgCodeValue = upper_msg_get_msg_code_value(parseXml);

	if (msgCodeValue != 30019)
		nslog(NS_INFO, "->->->response->->->->->->[ MsgCode = %d , msgtype = %d]->->->->->->->->", msgCodeValue, msgq->msgtype);

	switch (msgCodeValue) {
		case MSG_GET_ROOM_INFO:
			{
				status = lower_msg_get_room_info_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_SET_ROOM_INFO:
			{
				//在upper 端处理完后直接封包回应
			}
			break;

		case MSG_SET_QUALITY_INFO:
			{
				status = lower_msg_set_quality_info_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_SET_AUDIO_INFO:
			{
				status = lower_msg_set_audio_info_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_SET_SYS_PARAM:
			{
				//在upper 端处理完后直接封包回应
			}
			break;

		case MSG_RECORD_CTRL:
			{
				status = lower_msg_record_ctrl_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_REQUEST_CODE_STREAM:
			{
				status = lower_msg_request_code_stream_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_REMOTE_CTRL:
			{
				status = lower_msg_remote_ctrl_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_IFRAMEREQ_CTRL:
			{
				status = lower_msg_iFrame_req_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_CONNECT_ROOM_REQ:
			{
				//在upper 端处理完后直接封包回应
			}
			break;

		case MSG_SEND_LOGO_PIC:
			{
				status = lower_msg_send_logo_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_ADD_TITLE_REQ:
			{
				status = lower_msg_add_title_req_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_VOLUME_REQ:
			{
				status = lower_msg_volume_req_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_MUTE_REQ:
			{
				status = lower_msg_mute_req_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_PICADJUST_REQ:
			{
				status = lower_msg_pic_adjust_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_ENC_LOGIN:
			{
				status = lower_msg_enc_login_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_ENC_RESTART:
			{
				status = lower_msg_enc_restart_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_ENC_UPDATE:
			{
				status = lower_msg_enc_update_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		// add zl

		case MSGCODE_GET_CAMCTL_PROLIST:
			{
				status = lower_msg_get_camctl_ctrl_deal(pRoom, parseXml,msgq, head, xml);
			}
			break;

		case MSGCODE_SET_CAMCTL_PRO:
			{
				status = lower_msg_set_camctl_ctrl_deal(pRoom, parseXml,msgq, head, xml);
			}
			break;

		case MSG_ENC_IMAGE_SYNTHESIS:
			{
				status = lower_msg_image_synthesis_req_node(pRoom, parseXml,msgq, head, xml);
			}
			break;

		case MSG_ECN_DIRECTOR_MODE:
			{
				status = lower_msg_director_mode_ctrl_deal(pRoom, parseXml,msgq, head, xml);
			}
			break;


		default:
			break;
	}

	if (msgCodeValue != 30019)
		nslog(NS_INFO, "<-<-<-response<-<-<-<-<-<-<-[ MsgCode = %d ]<-<-<-<-<-<-<-<-<-<-", msgCodeValue);

	return status;
}

//deal the request xml msg
int32_t lower_msg_deal_request(RoomMsgEnv *pRoom, parse_xml_t *parseXml, msgque *msgq)
{
	int32_t status			= ROOM_RETURN_SUCC;
	int32_t tcp_socket		= 0;
	int32_t msgCodeValue	= 0;

	MsgHeader *head = (MsgHeader *)(msgq->msgbuf);
	int8_t *xml  = (int8_t *)msgq->msgbuf + sizeof(MsgHeader);

	UpperMsgEnv *upperMsgEnv = &(pRoom->upperEnv);
	tcp_socket = upperMsgEnv->tcp_socket;

	msgCodeValue = upper_msg_get_msg_code_value(parseXml);

	nslog(NS_ERROR,"GOD -----------1  <%d>\n",msgCodeValue);


	if (msgCodeValue != MSG_ENC_STATUSINFO_REQ)
		nslog(NS_INFO, "->->->->->request->->->->[ MsgCode = %d, msgq_id = %d, msgtype = %d]->->->->->->->->",\
											msgCodeValue, pRoom->lowerEnv.msg_id[ROOMMSGID], msgq->msgtype);

	switch (msgCodeValue) {
		case MSG_ENC_STATUS_REQ:
			{
				status = lower_msg_status_req_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_RECORD_REQ:
			{
				status = lower_msg_record_req_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		case MSG_ENC_STATUSINFO_REQ:
			{

				nslog(NS_ERROR,"GOD -----------2\n");

				status = lower_msg_enc_status_info_req_deal(pRoom, parseXml, msgq, head, xml);
			}
			break;

		default:
			break;
	}

	if (msgCodeValue != MSG_ENC_STATUSINFO_REQ)
		nslog(NS_INFO, "<-<-<-<-<-request<-<-<-<-<-[ MsgCode = %d ]<-<-<-<-<-<-<-<-<-<-", msgCodeValue);

	return status;
}


int32_t lower_msg_analyze(RoomMsgEnv *pRoom, msgque *msgq)
{
	MsgHeader *head = (MsgHeader *)(msgq->msgbuf);
	int8_t *xml  	= (int8_t *)msgq->msgbuf + sizeof(MsgHeader);

	int32_t 	status 			= ROOM_RETURN_SUCC;
	parse_xml_t *parse_xml_user = NULL;

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if (parse_xml_user == NULL) {
		status = ROOM_RETURN_FAIL;
		nslog(NS_ERROR, " --- r_malloc ERROR!:[%s]", strerror(errno));
		goto EXIT;
	}

	if(NULL == init_dom_tree(parse_xml_user, xml)) {
		status = ROOM_RETURN_FAIL;
		nslog(NS_ERROR, "[%s] ---[init_dom_tree] is error!", __func__);
		goto EXIT;
	}

	if (!parse_xml_user->proot) {
		status = ROOM_RETURN_FAIL;
		nslog(NS_ERROR, " --- parse_xml_user->proot is null!");
		goto EXIT;
	}

	//request msg
	if(is_req_msg(parse_xml_user->proot) == 1) {
		status = lower_msg_deal_request(pRoom, parse_xml_user, msgq);
	}

	//response msg
	if (is_resp_msg(parse_xml_user->proot) == 1) {
		status = lower_msg_deal_response(pRoom, parse_xml_user, msgq);
	}

EXIT:

	if(parse_xml_user->pdoc != NULL) {
		release_dom_tree(parse_xml_user->pdoc);
	}

	if (parse_xml_user != NULL) {
		r_free(parse_xml_user);
		parse_xml_user = NULL;
	}

	return status;
}

int32_t lower_msg_deal(RoomMsgEnv *pRoom, msgque *msg)
{
	MsgHeader *pHead = NULL;
	int8_t 	  *pXml  = NULL;
	int32_t   ret 	 = -1;
	int32_t   status = ROOM_RETURN_SUCC;

	pHead = (MsgHeader *)(msg->msgbuf);
	pXml  = (int8_t *)msg->msgbuf + sizeof(MsgHeader);

	pHead->sLen = ntohs(pHead->sLen);

	ret = upper_msg_head_checkout(pHead);
	if (ret == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, " ---[lower_msg_head_checkout] error!");
		cleanup(ROOM_RETURN_FAIL);
	}

	ret = lower_msg_analyze(pRoom, msg);
	if (ret == ROOM_RETURN_FAIL) {
//		nslog(NS_ERROR, "[%s] ---[lower_msg_analyze] is error!", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

cleanup:

	return status;
}

void lower_msg_recv(RoomMsgEnv *roomMsg)
{
	int32_t ret		= 0;
	int32_t msg_len = 0;
	int32_t msgId 	= 0;

	msgque 	msgp;
	msgque 	msg_deal;
	MsgHeader *pHeader = NULL;

	msgId = roomMsg->lowerEnv.msg_id[ROOMMSGID];

	msg_deal.msgbuf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!msg_deal.msgbuf) {
		nslog(NS_ERROR, " ---[r_malloc] error! [%s]", strerror(errno));
		return;
	}

	while (!rCtrlGblGetQuit(&roomMsg->glb)) {
//		r_usleep(100000);
		r_bzero(&msgp, sizeof(msgp));
//		ret = r_msg_recv(msgId, &msgp, sizeof(msgque), 0, IPC_NOWAIT);

		// zl question ???
		ret = r_msg_recv(msgId, &msgp, sizeof(msgque), 0, 0);
		if (ret == -1) {
			if (errno == EAGAIN || errno == ENOMSG) {
				continue;
			}

			nslog(NS_ERROR, "--- connect failed!----[%s]", strerror(errno));
			break;
		} else if (ret == 0) {
			continue;
		}

		if (msgp.msgbuf == NULL) {
			nslog(NS_ERROR, "msgp.msgbuf = %p", msgp.msgbuf);
			continue;
		}

		//len
		pHeader = (MsgHeader *)msgp.msgbuf;
		msg_len = ntohs(pHeader->sLen);

		if (msg_len > ROOM_MSG_MAX_LEN || \
			msg_len - sizeof(MsgHeader) != r_strlen(msgp.msgbuf + sizeof(MsgHeader))) {
			nslog(NS_ERROR, "sVer = %d, sLen = %d, r_strlen(msgp.msgbuf + sizeof(MsgHeader)) = %d", pHeader->sVer, msg_len, r_strlen(msgp.msgbuf + sizeof(MsgHeader)));
			nslog(NS_ERROR, "\n%s", msgp.msgbuf + sizeof(MsgHeader));
			if (msgp.msgbuf) {
				r_free(msgp.msgbuf);
				msgp.msgbuf = NULL;
				pHeader = NULL;
			}
			continue;
		}

		//buf
		r_bzero(msg_deal.msgbuf, ROOM_MSG_MAX_LEN);
		r_memcpy(msg_deal.msgbuf, msgp.msgbuf, msg_len);

		//type
		msg_deal.msgtype = msgp.msgtype;

		//free
		if (msgp.msgbuf) {
			r_free(msgp.msgbuf);
			msgp.msgbuf = NULL;
			pHeader = NULL;
		}

		lower_msg_deal(roomMsg, &msg_deal);
	}

	if (msg_deal.msgbuf)
		r_free(msg_deal.msgbuf);
}


void *lower_msg_process(void *arg)
{
	RoomMsgEnv  *roomMsg = (RoomMsgEnv *)arg;

	lower_msg_recv(roomMsg);

	nslog(NS_WARN, "lower_msg_process exit...");
	return NULL;
}

