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

#include "upper_msg_ctrl.h"
#include "room_ctrl.h"

#include "reach_os.h"
#include "reach_socket.h"

#include "media_msg.h"
#include "stdint.h"
#include "xml_base.h"
#include "xml_msg_management.h"

int32_t upper_msg_time_out_resp(RoomMsgEnv *pRoom, int32_t socket, pthread_mutex_t *mutex, MsgHeader *head, uint32_t msg_code, int8_t *pass_key, int8_t *user_id, int32_t room_id);
int8_t *upper_msg_get_pass_key_value(parse_xml_t *parseXml, int8_t *pass_key);
int8_t *upper_msg_get_user_id_value(parse_xml_t *parseXml, int8_t *user_id);
void upper_msg_get_server_info(RoomMsgEnv  *pRoom);
xmlNodePtr upper_msg_get_enc_info_node_by_id(xmlNodePtr encInfoNode, xmlDocPtr pdoc, uint32_t encInfoId);
int32_t upper_msg_get_xml_msg_code_value(int8_t *xml);

int32_t get_media_buf_info(mp4_record_t *mr, parse_data_t *buf_info)
{
	msgque msgp;
	r_memset(&msgp, 0, sizeof(msgque));

	if(0 > r_msg_recv(mr->course_data_fd, &msgp, sizeof(msgque) - sizeof(long), mr->sindex + 1, IPC_NOWAIT)) {
		return -1;
	}
#if 0
	nslog(NS_INFO, "msgp.msgbuf:[%p]", ((parse_data_t *)(msgp.msgbuf)));
	nslog(NS_INFO, "sindex:[%d]", ((parse_data_t *)(msgp.msgbuf))->sindex);
	nslog(NS_INFO, "data_len:[%d]", ((parse_data_t *)(msgp.msgbuf))->data_len);
	nslog(NS_INFO, "data:[%p]", ((parse_data_t *)(msgp.msgbuf))->data);
	nslog(NS_INFO, "flags:[%d]", ((parse_data_t *)(msgp.msgbuf))->flags);
	nslog(NS_INFO, "data_type:[%d]", ((parse_data_t *)(msgp.msgbuf))->data_type);
	nslog(NS_INFO, "height:[%d]", ((parse_data_t *)(msgp.msgbuf))->height);
	nslog(NS_INFO, "width:[%d]", ((parse_data_t *)(msgp.msgbuf))->width);
	nslog(NS_INFO, "sample_rate:[%d]", ((parse_data_t *)(msgp.msgbuf))->sample_rate);
	nslog(NS_INFO, "blue_flag:[%d]", ((parse_data_t *)(msgp.msgbuf))->blue_flag);
#endif
	r_memcpy(buf_info, msgp.msgbuf, sizeof(parse_data_t));
	r_free(msgp.msgbuf);
	return 0;
}

int32_t get_jpeg_buf_info(jpeg_record_t *jr, parse_data_t *buf_info)
{
	msgque msgp;
	r_memset(&msgp, 0, sizeof(msgque));

	if(0 > r_msg_recv(jr->course_data_fd, &msgp, sizeof(msgque) - sizeof(long), jr->sindex + 1, IPC_NOWAIT)) {
		return -1;
	}
#if 0
	nslog(NS_INFO, "msgp.msgbuf:[%p]", ((parse_data_t *)(msgp.msgbuf)));
	nslog(NS_INFO, "sindex:[%d]", ((parse_data_t *)(msgp.msgbuf))->sindex);
	nslog(NS_INFO, "data_len:[%d]", ((parse_data_t *)(msgp.msgbuf))->data_len);
	nslog(NS_INFO, "data:[%p]", ((parse_data_t *)(msgp.msgbuf))->data);
	nslog(NS_INFO, "flags:[%d]", ((parse_data_t *)(msgp.msgbuf))->flags);
	nslog(NS_INFO, "data_type:[%d]", ((parse_data_t *)(msgp.msgbuf))->data_type);
	nslog(NS_INFO, "height:[%d]", ((parse_data_t *)(msgp.msgbuf))->height);
	nslog(NS_INFO, "width:[%d]", ((parse_data_t *)(msgp.msgbuf))->width);
	nslog(NS_INFO, "sample_rate:[%d]", ((parse_data_t *)(msgp.msgbuf))->sample_rate);
	nslog(NS_INFO, "blue_flag:[%d]", ((parse_data_t *)(msgp.msgbuf))->blue_flag);
#endif
	r_memcpy(buf_info, msgp.msgbuf, sizeof(parse_data_t));
	r_free(msgp.msgbuf);
	return 0;
}
void release_media_buf(parse_data_t *buf_info)
{
	r_free(buf_info->data);
}

void release_jpeg_buf(parse_data_t *buf_info)
{
	r_free(buf_info->data);
}

int32_t upper_msg_write_file(int8_t *file, int8_t *buf)
{
	int32_t fd = 0;
	fd = open(file, O_RDWR|O_CREAT|O_TRUNC, 0666);
	if (fd < 0) {
		printf("[%s]--- open failed!----[%s]", __func__, strerror(errno));
		return -1;
	}

	int32_t ret = 0;
	ret = write(fd, buf, r_strlen(buf));
	if (ret < 0) {
		printf("[%s]--- write failed!----[%s]", __func__, strerror(errno));
		r_close(fd);
		return -1;
	}

	r_close(fd);
	return 0;
}

int32_t upper_msg_read_file(int8_t *file, int8_t *buf)
{
	int32_t fd = 0;
	fd = open(file, O_RDONLY);
	if (fd < 0) {
		printf("[%s]--- open failed!----[%s]", __func__, strerror(errno));
		return -1;
	}

	int32_t ret = 0;
	ret = read(fd, buf, ROOM_MSG_MAX_LEN);
	if (ret < 0) {
		printf("[%s]--- read failed!----[%s]", __func__, strerror(errno));
		r_close(fd);
		return -1;
	}

	r_close(fd);
	return 0;
}

void upper_msg_set_mask_bit_val(uint32_t *mask, int32_t bit, int32_t val)
{
	if (val)
		*mask = *mask | (1 << bit);
	else
		*mask = *mask & ~(1 << bit);
}

uint32_t upper_msg_get_mask_bit_val(uint32_t mask, int32_t bit)
{
	return ((mask & (1 << bit)) >> bit);
}

uint32_t upper_msg_is_all_enc_resp(uint32_t mask)
{
	return (!(mask & MSG_QUEUE_REC_ALL));
}

int32_t upper_msg_create(int8_t *msg, MsgHeader *head, int8_t *xml)
{
	int32_t msgLen 	= 0;
	int32_t status 	= ROOM_RETURN_SUCC;
	int8_t *pMsg 	= msg;

	if (!head || !xml || !msg) {
		nslog(NS_ERROR, " ---[upper_msg_create] param error!");
		return ROOM_RETURN_FAIL;
	}

	head->sLen = r_strlen(xml) + sizeof(MsgHeader);

	head->sLen = htons(head->sLen);
	head->sVer = htons(MSG_HEAD_VER);
	head->sMsgType = htons(1);

	r_memcpy((void *)pMsg, (void *)head, sizeof(MsgHeader));
	r_memcpy((void *)pMsg+sizeof(MsgHeader), (void *)xml, r_strlen(xml));

	return status;
}

int32_t upper_msg_head_checkout(MsgHeader *head)
{
	int32_t status = ROOM_RETURN_SUCC;

//	nslog(NS_ERROR,"OOXX ---- %d\n",ntohs(head->sVer));
	if (ntohs(head->sVer) != MSG_HEAD_VER) {		
		status = ROOM_RETURN_FAIL;
	}

	return 	status;
}

int32_t upper_msg_tcp_send(int32_t socket, pthread_mutex_t *mutex, MsgHeader *head, int8_t *xml)
{
	int32_t ret 	= 0;
	int32_t status 	= ROOM_RETURN_SUCC;
	int32_t msgLen 	= 0;
	int8_t *msg 	= NULL;

	if (mutex == NULL || head == NULL || xml == NULL) {
		nslog(NS_ERROR, " ---[param mutex = %p, head = %p, xml = %p] is error!", mutex, head, xml);
		status = ROOM_RETURN_FAIL;
		goto exit;
	}

	if (!r_strlen(xml)) {
		nslog(NS_ERROR, " ---[r_strlen(xml) = %d] is error!", r_strlen(xml));
		status = ROOM_RETURN_FAIL;
		goto exit;
	}

	msgLen = r_strlen(xml) + sizeof(MsgHeader);

	msg = (int8_t *)r_malloc(msgLen+1);
	if (!msg) {
		nslog(NS_ERROR, " ---[r_malloc] is error!");
		status = ROOM_RETURN_FAIL;
		goto exit;
	}

	r_bzero((void *)msg, msgLen+1);
	ret = upper_msg_create(msg, head, xml);
	if (ret == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, " ---[upper_msg_create] is error!");
		status = ROOM_RETURN_FAIL;
		goto exit;
	}

#if 1
	ret = upper_msg_get_xml_msg_code_value(xml);

	if (ret != 30019) {
		if (ret != 30032)
			nslog(NS_INFO, " ---send--- socket = %d, send_len = %d, sLen = %d, sVer = %d, xml_code = %d, xml_len = %d\n%s"	, socket, msgLen, ntohs(((MsgHeader *)msg)->sLen), ntohs(((MsgHeader *)msg)->sVer), ret, r_strlen(xml), msg+sizeof(MsgHeader));
		else
			;//nslog(NS_INFO, " ---send--- socket = %d, send_len = %d, sLen = %d, sVer = %d, xml_code = %d, xml_len = %d"		, socket, msgLen, ntohs(((MsgHeader *)msg)->sLen), ntohs(((MsgHeader *)msg)->sVer), ret, r_strlen(xml));
	}
#endif

	if (socket > 0) {
		pthread_mutex_lock(mutex);
		// add zl import !!!!!!!!!!!!!!!!!
		send_long_tcp_data(socket, (void *)msg, msgLen);
		pthread_mutex_unlock(mutex);
	}

exit:

	if (msg) {
		r_free(msg);
		msg = NULL;
	}

	return status;
}

int32_t upper_msg_tim_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
	if(x->tv_sec > y->tv_sec) {
		return   -1;
	}

	if((x->tv_sec == y->tv_sec) && (x->tv_usec > y->tv_usec)) {
		return   -1;
	}

	result->tv_sec = (y->tv_sec - x->tv_sec);
	result->tv_usec = (y->tv_usec - x->tv_usec);

	if(result->tv_usec < 0) {
		result->tv_sec--;
		result->tv_usec += 1000000;
	}

	return   0;
}

int32_t upper_msg_set_time(struct  timeval *time)
{
	if(time == NULL)
	{
		return -1;
	}

	struct  timeval  time_now ;
	time_now.tv_sec = 0;
	time_now.tv_usec = 0;
	gettimeofday(&time_now, 0);

	time->tv_sec = time_now.tv_sec;
	time->tv_usec = time_now.tv_usec;

	return 0;
}

int32_t upper_msg_monitor_time_out_status(struct  timeval *time, uint32_t time_out)
{
//	heart_serv_info_zte_t *temp_heart_info =(heart_serv_info_zte_t *)arg;
	struct  timeval  time_now ;
	time_now.tv_sec = 0;
	time_now.tv_usec = 0;
	struct  timeval  time_old;
	time_old.tv_sec = 0;
	time_old.tv_usec = 0 ;
	struct  timeval delta_time ;
	delta_time.tv_sec = 0;
	delta_time.tv_usec = 0;

	gettimeofday(&time_now, 0);
	time_old.tv_sec = time->tv_sec;
	time_old.tv_usec = time->tv_usec;

	if(-1 == upper_msg_tim_subtract(&delta_time, &time_old, &time_now)) {
		nslog(NS_ERROR," tim_subtract is error!\n");
		return ROOM_RETURN_FAIL;
	}

	if(delta_time.tv_sec > time_out) {

		return ROOM_RETURN_TIMEOUT;
	}

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_get_idle_index(RoomMsgEnv *pRoom)
{
	int ret_val = -1;

	int i = 0;
	for (i = 0; i < ROOM_MSG_TIME_QUE_NUM; i++) {
		if (pRoom->msgTimeQue[i].MsgCode == 0) {
			ret_val = i;
			break;
		}
	}

	if (ret_val == -1) {
		nslog(NS_INFO, "upper_msg_get_idle_index error! queue full!!!!");
	}

	return ret_val;
}

int32_t upper_msg_have_same_msgcode(RoomMsgEnv *pRoom, int32_t msgCode, int8_t *passKey, int8_t *userId, MsgHeader *head)
{
	int i 		= 0;
	int ret_val = ROOM_RETURN_FAIL;

	if (pRoom == NULL || passKey == NULL || userId == NULL) {
		nslog(NS_ERROR, " ---[param error]!");
		return -1;
	}

	for (i = 0; i < ROOM_MSG_TIME_QUE_NUM; i++) {
		if (pRoom->msgTimeQue[i].MsgCode == 0) {
			continue;
		}

		//连接教室信息，不区分平台，没回应前只设置一次
		if (msgCode == pRoom->msgTimeQue[i].MsgCode && msgCode == MSG_CONNECT_ROOM_REQ) {
			ret_val = ROOM_RETURN_SUCC;
			break;
		}

#if 1
		if(!r_strcmp(passKey, pRoom->msgTimeQue[i].PassKey) &&\
			msgCode == pRoom->msgTimeQue[i].MsgCode)
#else
		if(!r_strcmp(passKey, pRoom->msgTimeQue[i].PassKey) &&\
			msgCode == pRoom->msgTimeQue[i].MsgCode && \
			head->sData == pRoom->msgTimeQue[i].head.sData)
#endif
		{
			if (!r_strcmp(passKey, MSG_LIVENODE_PASSKEY))
			{
				if (!r_strcmp(userId, pRoom->msgTimeQue[i].UserId))
					ret_val = ROOM_RETURN_SUCC;
				else
					ret_val = ROOM_RETURN_FAIL;
			}
			else
			{
				ret_val = ROOM_RETURN_SUCC;
			}

			break;
		}
	}

	return ret_val;
}

int32_t upper_msg_time_que_add_node(RoomMsgEnv *pRoom, int32_t msgCodeValue, int8_t *passKey, int8_t *userId, MsgHeader *head, int32_t (*time_out_process_fun)(RoomMsgEnv *, int32_t , pthread_mutex_t *, MsgHeader *, uint32_t , int8_t *, int8_t *, int32_t ))
{
	int32_t ret_val = 0;

	pthread_mutex_lock(&pRoom->msgQueMutex);

	ret_val = upper_msg_have_same_msgcode(pRoom, msgCodeValue, passKey, userId, head);
	if (ROOM_RETURN_SUCC == ret_val || -1 == ret_val) {
		nslog(NS_ERROR, " --- have the same msgcode! or error");
		pthread_mutex_unlock(&pRoom->msgQueMutex);
		return ROOM_RETURN_FAIL;
	}

	int index = upper_msg_get_idle_index(pRoom);
	if (index == -1) {
		pthread_mutex_unlock(&pRoom->msgQueMutex);
		return ROOM_RETURN_FAIL;
	} else {
		r_bzero(&pRoom->msgTimeQue[index], sizeof(MsgTimeQueue));
		pRoom->msgTimeQue[index].MsgCode = msgCodeValue;
		r_strcpy(pRoom->msgTimeQue[index].PassKey, passKey);
		r_strcpy(pRoom->msgTimeQue[index].UserId, userId);
		r_memcpy(&pRoom->msgTimeQue[index].head, head, sizeof(MsgHeader));
		upper_msg_set_time(&pRoom->msgTimeQue[index].time);
		pRoom->msgTimeQue[index].time_out_process_fun = time_out_process_fun;


		if (msgCodeValue != 30019)
			nslog(NS_INFO, " MsgCode = %d, PassKey = %s", msgCodeValue, passKey);
	}

	pthread_mutex_unlock(&pRoom->msgQueMutex);
	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_time_que_del_node(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head)
{
	int i 				 = 0;
	int32_t msgCodeValue = 0;

	msgCodeValue = upper_msg_get_msg_code_value(parseXml);
	if (msgCodeValue == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, " --- get msg code value error!");
		return ROOM_RETURN_FAIL;
	}

	int8_t  passKey[ROOM_MSG_VAL_LEN] = {0};
	r_bzero(passKey, ROOM_MSG_VAL_LEN);
	if (!upper_msg_get_pass_key_value(parseXml, passKey)) {
		nslog(NS_ERROR, " --- get pass key value error!");
		return ROOM_RETURN_FAIL;
	}

	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);

	pthread_mutex_lock(&pRoom->msgQueMutex);
	for (i = 0; i < ROOM_MSG_TIME_QUE_NUM; i++) {
		if (pRoom->msgTimeQue[i].MsgCode == 0) {
			continue;
		}

#if 1
		if(!r_strcmp(passKey, pRoom->msgTimeQue[i].PassKey) &&\
			msgCodeValue == pRoom->msgTimeQue[i].MsgCode)
#else
		if(!r_strcmp(passKey, pRoom->msgTimeQue[i].PassKey) &&\
			msgCodeValue == pRoom->msgTimeQue[i].MsgCode &&\
			head->sData == pRoom->msgTimeQue[i].head.sData)
#endif
		{
			if (!r_strcmp(passKey, MSG_LIVENODE_PASSKEY))
			{
				if (!r_strcmp(user_id, pRoom->msgTimeQue[i].UserId))  {
					nslog(NS_INFO, " MsgCode = %d, PassKey = %s, user_id = %s", msgCodeValue, passKey, user_id);
					r_bzero(&pRoom->msgTimeQue[i], sizeof(MsgTimeQueue));
				}
			}
			else
			{

				if (msgCodeValue != 30019)
					nslog(NS_INFO, " MsgCode = %d, PassKey = %s", msgCodeValue, passKey);
				r_bzero(&pRoom->msgTimeQue[i], sizeof(MsgTimeQueue));
			}
			break;
		}

		//连接教室信息后，等待登录, 不比较PASSKEY
		if(msgCodeValue == MSG_ENC_LOGIN && pRoom->msgTimeQue[i].MsgCode == MSG_CONNECT_ROOM_REQ)	{
			nslog(NS_INFO, " MsgCode = %d, PassKey = %s", msgCodeValue, passKey);

			r_bzero(&pRoom->msgTimeQue[i], sizeof(MsgTimeQueue));
			break;
		}
	}

	pthread_mutex_unlock(&pRoom->msgQueMutex);
	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_monitor_time_queue(RoomMsgEnv *pRoom)
{
	int32_t i = 0;
	int32_t time_out = 0;
	int32_t ret_val  = ROOM_RETURN_FAIL;

	pthread_mutex_lock(&pRoom->msgQueMutex);
	for (i = 0; i < ROOM_MSG_TIME_QUE_NUM; i++) {
		if (pRoom->msgTimeQue[i].MsgCode == 0) {
			continue;
		}

		if (pRoom->msgTimeQue[i].MsgCode == MSG_ENC_LOGIN)
			time_out = ROOM_MSG_LOGIN_TIME_OUT_VAL;
		else if (pRoom->msgTimeQue[i].MsgCode == MSG_VOLUME_REQ)
			time_out = ROOM_MSG_VOLUME_TIME_OUT_VAL;
		else
			time_out = ROOM_MSG_TIME_OUT_VAL;

		ret_val = upper_msg_monitor_time_out_status(&pRoom->msgTimeQue[i].time, time_out);
		if (ROOM_RETURN_TIMEOUT == ret_val) {
			if (pRoom->msgTimeQue[i].time_out_process_fun)
			{
				pRoom->msgTimeQue[i].time_out_process_fun(pRoom, pRoom->upperEnv.tcp_socket,\
									&pRoom->upperEnv.tcp_socket_mutex, &pRoom->msgTimeQue[i].head,\
									pRoom->msgTimeQue[i].MsgCode, pRoom->msgTimeQue[i].PassKey, \
									pRoom->msgTimeQue[i].UserId, pRoom->RoomInfo.RoomId);
			}

			if(pRoom->msgTimeQue[i].MsgCode == MSG_CONNECT_ROOM_REQ || \
				pRoom->msgTimeQue[i].MsgCode == MSG_RECORD_CTRL) {
				int8_t *xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
				if (!xml_buf) {
					nslog(NS_ERROR, " ---[r_malloc] error!");
					r_bzero(&pRoom->msgTimeQue[i], sizeof(MsgTimeQueue));
					continue;
				}

				//更新录制信息
				if (pRoom->record_handle) {
					pRoom->RecStatusInfo.RecStatus = lower_msg_opt_type_trans(pRoom->record_handle->get_record_status(pRoom->record_handle));
					r_strcpy(pRoom->RecStatusInfo.RecName, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
				} else {
					pRoom->RecStatusInfo.RecStatus = 0;
					r_strcpy(pRoom->RecStatusInfo.RecName, pRoom->RecInfo.CourseName);
				}

				//回复状态
				r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
				lower_msg_package_req_status_info(xml_buf, &pRoom->RecStatusInfo);
				nslog(NS_ERROR,"WAHT ----4\n");
				upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, &pRoom->msgTimeQue[i].head, xml_buf);

				if (xml_buf)
					r_free(xml_buf);
			}

			r_bzero(&pRoom->msgTimeQue[i], sizeof(MsgTimeQueue));
		} else if (ROOM_RETURN_FAIL == ret_val) {
//			r_bzero(&pRoom->msgTimeQue[i], sizeof(MsgTimeQueue));
			upper_msg_set_time(&pRoom->msgTimeQue[i].time);
		}
	}

	pthread_mutex_unlock(&pRoom->msgQueMutex);
	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_check_enc_num(RoomMsgEnv *pRoom)
{
	int32_t i 	= 0;
	int32_t num = 0;

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0){
			continue;
		}

		num++;
	}

	return num;
}

enc_info *upper_msg_get_jpeg_enc_info(RoomMsgEnv *pRoom)
{
	int32_t i = 0;
	enc_info *e_info = NULL;

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0){
			continue;
		}

		if (pRoom->RoomInfo.EncInfo[i].EncodeType == EncodeType_JPEG) {
			e_info = &pRoom->RoomInfo.EncInfo[i];
			break;
		}
	}

	return e_info;
}

void upper_msg_print_cinfo(course_record_condition_t *crc)
{
	nslog(NS_INFO, "----------------------------------");

	nslog(NS_INFO, "crc->cinfo.RecDateTime = %s"		, crc->cinfo.RecDateTime);
	nslog(NS_INFO, "crc->cinfo.RecordID = %s"			, crc->cinfo.RecordID);
	nslog(NS_INFO, "crc->cinfo.MainTeacher = %s"		, crc->cinfo.MainTeacher);
	nslog(NS_INFO, "crc->cinfo.CName = %s"				, crc->cinfo.CName);
	nslog(NS_INFO, "crc->cinfo.ScmType = %s"			, crc->cinfo.ScmType);
	nslog(NS_INFO, "crc->cinfo.Notes = %s"				, crc->cinfo.Notes);
	nslog(NS_INFO, "");

	nslog(NS_INFO, "crc->dev_id = %s"						, crc->dev_id);
	nslog(NS_INFO, "crc->record_root_dir = %s"				, crc->record_root_dir);
	nslog(NS_INFO, "crc->room_ctrl_msg_fd = %d"				, crc->room_ctrl_msg_fd);
	nslog(NS_INFO, "crc->roomid = %d"						, crc->roomid);
	nslog(NS_INFO, "crc->sindex_sum = %d"					, crc->sindex_sum);
	nslog(NS_INFO, "crc->max_course_record_duration = %d"	, crc->max_course_record_duration);
	nslog(NS_INFO, "");

	int32_t i = 0;
	for (i = 0; i < sizeof(crc->rinfo)/sizeof(crc->rinfo[0]); i++) {
		nslog(NS_INFO, "crc->rinfo[%d].audio_SampleRate = %d"	, i, crc->rinfo[i].audio_SampleRate);
		nslog(NS_INFO, "crc->rinfo[%d].audio_BitRate = %d"		, i, crc->rinfo[i].audio_BitRate);
		nslog(NS_INFO, "crc->rinfo[%d].width = %d"				, i, crc->rinfo[i].width);
		nslog(NS_INFO, "crc->rinfo[%d].height = %d"				, i, crc->rinfo[i].height);
		nslog(NS_INFO, "crc->rinfo[%d].stream_type = %d"		, i, crc->rinfo[i].stream_type);
		nslog(NS_INFO, "");
	}

	nslog(NS_INFO, "----------------------------------");
}

int32_t upper_msg_set_record_condition(RoomMsgEnv *pRoom, course_record_condition_t *crc)
{
	int32_t ret_val = ROOM_RETURN_SUCC;
	localtime_t t;
	enc_info *e_info = NULL;

	get_localtime(&t);
	r_sprintf(crc->cinfo.RecDateTime, "%04d-%02d-%02d %02d:%02d:%02d",\
							t.tm_year, t.tm_mon, t.tm_mday,	t.tm_hour, t.tm_min, t.tm_sec);

	r_strncpy(crc->cinfo.RecordID, pRoom->RecInfo.RecordID, sizeof(crc->cinfo.RecordID)-1);
	r_strncpy(crc->cinfo.MainTeacher, pRoom->RecInfo.TeacherName, sizeof(crc->cinfo.MainTeacher)-1);
	r_strncpy(crc->cinfo.CName, pRoom->RecInfo.AliasName, sizeof(crc->cinfo.CName)-1);
	r_strncpy(crc->cinfo.ScmType, "-1", sizeof(crc->cinfo.ScmType)-1);
	r_strncpy(crc->cinfo.Notes, pRoom->RecInfo.Notes, sizeof(crc->cinfo.Notes)-1);

	r_strncpy(crc->record_root_dir, pRoom->RecInfo.RecPath, sizeof(crc->record_root_dir)-1);
	r_strncpy(crc->dev_id, pRoom->RecInfo.ServerSeries, sizeof(crc->dev_id)-1);
	crc->max_course_record_duration = pRoom->RoomInfo.RecordMaxTime * 3600;
	crc->room_ctrl_msg_fd = pRoom->lowerEnv.msg_id[ROOMMSGID];
	crc->roomid = pRoom->RoomInfo.RoomId;
//	crc->sindex_sum = upper_msg_get_mask_num(pRoom->RoomInfo.valid_stream_mask);

	if (upper_msg_get_jpeg_enc_info(pRoom)){
		crc->get_jpeg_buf_info_callback = get_jpeg_buf_info;
		crc->release_jpeg_buf_callback = release_jpeg_buf;
	} else {
		crc->get_jpeg_buf_info_callback = NULL;
		crc->release_jpeg_buf_callback 	= NULL;
	}

	crc->get_media_buf_info_callback = get_media_buf_info;
	crc->release_media_buf_callback = release_media_buf;

	//音频固定为第一路
	if ((pRoom->RoomInfo.room_info_mask & (1 << pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.MsgQueType)) ||
		(pRoom->RoomInfo.room_info_mask & (1 << pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.MsgQueType))) {
		crc->rinfo[0].audio_SampleRate 	= pRoom->RoomInfo.AudioInfo.SampleRate;
		crc->rinfo[0].audio_BitRate 	= pRoom->RoomInfo.AudioInfo.Bitrate;

		crc->rinfo[1].audio_SampleRate 	= pRoom->RoomInfo.AudioInfo.SampleRate;
		crc->rinfo[1].audio_BitRate 	= pRoom->RoomInfo.AudioInfo.Bitrate;
	}

	pRoom->RoomInfo.record_stream_mask = 0;

	int32_t i = 0, j = 0, jpeg = 0;
	for (i = 0, j = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			j+=2;
			continue;
		}

		if (pRoom->RoomInfo.EncInfo[i].EncodeType == EncodeType_H264) {
			if ((pRoom->RoomInfo.room_info_mask & (1 << pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType)) ||
				(pRoom->RoomInfo.room_info_mask & (1 << pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType))) {
				crc->rinfo[j].width = pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.EncWidth;
				crc->rinfo[j].height = pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.EncHeight;
				crc->rinfo[j].stream_type = RECORD_HD;
				pRoom->RoomInfo.record_stream_mask |= (1 << pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType);

				if (crc->rinfo[j].width == 0 || crc->rinfo[j].height == 0) {
					nslog(NS_ERROR, "ENC ID = %d, width = %d, height = %d", pRoom->RoomInfo.EncInfo[i].ID, crc->rinfo[j].width, crc->rinfo[j].height);
					ret_val = ROOM_RETURN_FAIL;
				}
				j++;

				crc->rinfo[j].width = pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.EncWidth;
				crc->rinfo[j].height = pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.EncHeight;
				crc->rinfo[j].stream_type = RECORD_SD;
				pRoom->RoomInfo.record_stream_mask |= (1 << pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType);

				if (crc->rinfo[j].width == 0 || crc->rinfo[j].height == 0) {
					nslog(NS_ERROR, "ENC ID = %d, width = %d, height = %d", pRoom->RoomInfo.EncInfo[i].ID, crc->rinfo[j].width, crc->rinfo[j].height);
					ret_val = ROOM_RETURN_FAIL;
				}
				j++;

			}	else {
				j+=2;
			}
		} else if (pRoom->RoomInfo.EncInfo[i].EncodeType == EncodeType_INVALID){
			j+=2;
		}

		jpeg = j;
	}

	e_info = upper_msg_get_jpeg_enc_info(pRoom);
	if (e_info) {
//		if (pRoom->RoomInfo.room_info_mask & (1 << e_info->HD_QuaInfo.MsgQueType)) {
			pRoom->RoomInfo.record_stream_mask |= (1 << e_info->HD_QuaInfo.MsgQueType);
			crc->rinfo[jpeg].stream_type = RECORD_JPEG;
//		}
	} else {
		jpeg--;
	}

	crc->sindex_sum = jpeg+1;

	return ret_val;
}

int upper_msg_get_current_samename_node_nums(xmlNodePtr curNode)
{
	int i = 0;
	xmlChar *key = NULL;
	xmlNodePtr tmp_node = curNode;

	if(NULL == tmp_node) {
		return -1;
	}

	key = (xmlChar *)tmp_node->name;

	while(NULL != tmp_node) {
		if(!xmlStrcmp(tmp_node->name, key)) {
			i ++;
		}

		tmp_node = tmp_node->next;
	}

	return i;
}


xmlNodePtr upper_msg_get_next_samename_node(xmlNodePtr curNode)
{
	xmlChar *key = NULL;
	xmlNodePtr node = NULL;

	if(NULL == curNode){
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

xmlNodePtr upper_msg_get_req_msg_code_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgHead");
	if (!tmp_node) {
		return NULL;
	}

	*pnode = get_children_node(tmp_node, BAD_CAST "MsgCode");

	return *pnode;
}

int32_t upper_msg_get_xml_msg_code_value(int8_t *xml)
{
	int32_t status = 0;
	int8_t value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr  tmp_node = NULL;
	parse_xml_t *parse_xml_user = NULL;

	if (!xml) {
		nslog(NS_ERROR, "xml = %p", xml);
		return ROOM_RETURN_FAIL;
	}

	parse_xml_user = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if (parse_xml_user == NULL) {
		status = ROOM_RETURN_FAIL;
		nslog(NS_ERROR, " --- r_malloc ERROR!:[%s]", strerror(errno));
		goto exit;
	}

	if(NULL == init_dom_tree(parse_xml_user, xml)) {
		status = ROOM_RETURN_FAIL;
		nslog(NS_ERROR, " --- init_dom_tree ERROR!");
		goto exit;
	}

	if (!parse_xml_user->proot) {
		status = ROOM_RETURN_FAIL;
		nslog(NS_ERROR, " --- parse_xml_user->proot is null!");
		goto exit;
	}

	tmp_node = upper_msg_get_req_msg_code_node(&tmp_node, parse_xml_user->proot);
	if (!tmp_node) {
		return ROOM_RETURN_FAIL;
	}

	get_current_node_value(value, ROOM_MSG_VAL_LEN, parse_xml_user->pdoc, tmp_node);

exit:

	if(parse_xml_user->pdoc != NULL) {
		release_dom_tree(parse_xml_user->pdoc);
	}

	if (parse_xml_user != NULL) {
		r_free(parse_xml_user);
		parse_xml_user = NULL;
	}

	return atoi(value);
}


int32_t upper_msg_get_msg_code_value(parse_xml_t *parseXml)
{
	int8_t value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr  tmp_node = NULL;

	tmp_node = upper_msg_get_req_msg_code_node(&tmp_node, parseXml->proot);
	if (!tmp_node) {
		return ROOM_RETURN_FAIL;
	}

	get_current_node_value(value, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	return atoi(value);
}

int8_t *upper_msg_get_pass_key_value(parse_xml_t *parseXml, int8_t *pass_key)
{
	int8_t value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr  pNode_tmp = NULL;

	if (pass_key == NULL || parseXml == NULL) {
		return NULL;
	}

	pNode_tmp = get_req_pass_key_node(&pNode_tmp, parseXml->proot);
	if (!pNode_tmp) {
		return NULL;
	}

	get_current_node_value(value, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode_tmp);

	r_strcpy(pass_key, value);

	return pass_key;
}

int8_t *upper_msg_get_user_id_value(parse_xml_t *parseXml, int8_t *user_id)
{
	int8_t value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr  pNode_tmp = NULL;

	if (user_id == NULL || parseXml == NULL) {
		return NULL;
	}

	pNode_tmp = get_children_node(parseXml->proot, MSG_USERID_KEY);
	if (!pNode_tmp) {
//		nslog(NS_INFO, " ---[no UserID]!");
		return NULL;
	}

	get_current_node_value(value, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode_tmp);

	r_strcpy(user_id, value);

	return user_id;
}


int32_t upper_msg_get_leaf_value(char *value, int32_t value_len, xmlChar *key, xmlNodePtr curNode, xmlDocPtr pdoc)
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


enc_info *upper_msg_get_enc_info_by_id(RoomMsgEnv *pRoom, uint32_t enc_id)
{
	int32_t i = 0;
	enc_info *e_info = NULL;

	if (!enc_id) {
		return NULL;
	}

	for (i = 0; i < ROOM_ENC_NUM; i++){
		if (pRoom->RoomInfo.EncInfo[i].ID == 0){
			continue;
		}

		if (pRoom->RoomInfo.EncInfo[i].ID == enc_id) {
			e_info = &pRoom->RoomInfo.EncInfo[i];
			break;
		}
	}

	return e_info;
}

int32_t upper_msg_get_mask_num(uint32_t stream_mask)
{
	int32_t 	i 		= 0;
	int32_t 	num		= 0;
	uint32_t 	mask	= 0;

	mask = stream_mask;
	for (i = 0; i < sizeof(uint32_t)*8; i++){
		if(1 == ((mask >> i)&0x1)){
			num++;
		}
	}

	return num;
}

int32_t upper_msg_get_valid_stream_num(RoomMsgEnv *pRoom)
{
	int32_t 	i 		= 0;
	int32_t 	num		= 0;

	for (i = 0; i < ROOM_ENC_NUM; i++){
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		}

		if (pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.Status == 1) {
			num++;
		}

		if (pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.Status == 1) {
			num++;
		}
	}

	nslog(NS_INFO, "============== num = %d", num);

	return num;
}


void upper_msg_send_record_msg_to_enc(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t i				= 0;
	int32_t socket 			= -1;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex 	= NULL;

	int32_t index  = lower_msg_get_platfrom_key(parseXml);
	if (index < 0) {
		nslog(NS_ERROR, " ---[lower_msg_get_platfrom_key] error!");
		return ;
	}

	pRoom->record_mask[index] = MSG_QUEUE_REC_CLR;
	pRoom->record_mask[index] |= MSG_QUEUE_REC_FLG;

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		}

		e_info = &pRoom->RoomInfo.EncInfo[i];

		//HD
		socket = e_info->HD_QuaInfo.Socket;
		mutex  = e_info->HD_QuaInfo.mutex;

		if (socket > 0) {
			pRoom->record_mask[index] |= (1 << e_info->HD_QuaInfo.MsgQueType);
			upper_msg_tcp_send(socket, mutex, head, xml);
		}
		nslog(NS_INFO, " --- HD -- EncNum = %d, i = %d, socket = %d, record_mask[%d] = %d, valid_stream_mask = %d, MsgQueType = %d", pRoom->RoomInfo.EncNum, i, socket, index, pRoom->record_mask[index], pRoom->RoomInfo.valid_stream_mask, e_info->HD_QuaInfo.MsgQueType);

		//JPEG
		if (e_info->EncodeType == EncodeType_JPEG) {
			nslog(NS_INFO, "Enc %d is JPEG, don't send record xml to SD, continue for next", e_info->ID);
			continue;
		}

		//SD
		socket = e_info->SD_QuaInfo.Socket;
		mutex  = e_info->SD_QuaInfo.mutex;

		if (socket > 0) {
			pRoom->record_mask[index] |= (1 << e_info->SD_QuaInfo.MsgQueType);
			upper_msg_tcp_send(socket, mutex, head, xml);
		}
		nslog(NS_INFO, " --- SD -- EncNum = %d, i = %d, socket = %d, record_mask[%d] = %d, valid_stream_mask = %d, MsgQueType = %d", pRoom->RoomInfo.EncNum, i, socket, index, pRoom->record_mask[index], pRoom->RoomInfo.valid_stream_mask, e_info->SD_QuaInfo.MsgQueType);
	}

}

int32_t upper_msg_package_add_xml_leaf(xmlNodePtr father_node, const xmlChar *key_name, int8_t *key_value)
{
	xmlNodePtr tmp_node	= NULL;

	tmp_node = xmlNewNode(NULL, key_name);
	xmlAddChild(father_node, tmp_node);
	xmlAddChild(tmp_node, xmlNewText((const xmlChar *)key_value));

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_package_req_server_info_xml(int32_t room_id, int8_t *out_buf, int32_t msgcode, int8_t *pass_key, int8_t *user_id)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen		= 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf	= NULL;
	xmlDocPtr	pxml		= NULL;
	xmlNodePtr	root_node	= NULL;
	xmlNodePtr	head_node	= NULL;
	xmlNodePtr	body_node	= NULL;
	xmlNodePtr	RoomInfoReq_node = NULL;

	if(NULL == out_buf){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//RequestMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msgcode);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);

	//MsgBody
	upper_msg_package_add_xml_leaf(root_node, MSG_BODY_KEY, "");

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}



int32_t upper_msg_package_reponse_xml(int8_t *out_buf, int32_t msgcode, int8_t *ret_code, int8_t *pass_key, int8_t *user_id, int32_t room_id)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen = 0;;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pxml = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;


	if(NULL == out_buf || NULL == ret_code){
		nslog(NS_ERROR, " ---- param error");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, RESP_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msgcode);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", ret_code);
	upper_msg_package_add_xml_leaf(head_node, MSG_RETURNCODE_KEY, msgcode_buf);


	xmlNodePtr body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", room_id);
	upper_msg_package_add_xml_leaf(body_node, MSG_ROOMID_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,  XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}

int32_t upper_msg_package_connc_room_req_xml(int8_t *out_buf, int32_t msgcode, int8_t *ret_code, int8_t *pass_key, int32_t opt, int8_t *user_id, int32_t room_id)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen = 0;;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pxml = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;


	if(NULL == out_buf || NULL == ret_code){
		nslog(NS_ERROR, " ---- param error");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, RESP_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msgcode);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", ret_code);
	upper_msg_package_add_xml_leaf(head_node, MSG_RETURNCODE_KEY, msgcode_buf);


	xmlNodePtr body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", room_id);
	upper_msg_package_add_xml_leaf(body_node, MSG_ROOMID_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", opt);
	upper_msg_package_add_xml_leaf(body_node, MSG_OPTTYPE_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,  XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}


int32_t upper_msg_package_stream_reponse_xml(int8_t *out_buf, int32_t msgcode, int8_t *ret_code, int8_t *pass_key, int8_t *user_id, int8_t *EncodeIndex, int32_t room_id, StrmReq *strm_req_info)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen = 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pxml = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;


	if(NULL == out_buf || NULL == ret_code){
		nslog(NS_ERROR, " ---- param error");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, RESP_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msgcode);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", ret_code);
	upper_msg_package_add_xml_leaf(head_node, MSG_RETURNCODE_KEY, msgcode_buf);


	xmlNodePtr body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", room_id);
	upper_msg_package_add_xml_leaf(body_node, MSG_ROOMID_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", EncodeIndex);
	upper_msg_package_add_xml_leaf(body_node, MSG_ENCODEINDEX_KEY, msgcode_buf);

	xmlNodePtr StrmReq_node = xmlNewNode(NULL, MSG_STREAMREQ_KEY);
	xmlAddChild(body_node, StrmReq_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", room_id);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_ROOMID_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", strm_req_info->Quality);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_QUALITY_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", strm_req_info->EncodeIndex);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_ENCODEINDEX_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	msg_ctrl_inet_ntoa(msgcode_buf, strm_req_info->Ipaddr1);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_IPADDR1_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", strm_req_info->Port1);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_PORT1_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	msg_ctrl_inet_ntoa(msgcode_buf, strm_req_info->Ipaddr2);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_IPADDR2_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", strm_req_info->Port2);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_PORT2_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	msg_ctrl_inet_ntoa(msgcode_buf, strm_req_info->Ipaddr3);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_IPADDR3_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", strm_req_info->Port3);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_PORT3_KEY, msgcode_buf);

	// add zl
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	msg_ctrl_inet_ntoa(msgcode_buf, strm_req_info->Ipaddr4);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_IPADDR4_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", strm_req_info->Port4);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_PORT4_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	msg_ctrl_inet_ntoa(msgcode_buf, strm_req_info->Ipaddr5);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_IPADDR5_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", strm_req_info->Port5);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_PORT5_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	msg_ctrl_inet_ntoa(msgcode_buf, strm_req_info->Ipaddr6);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_IPADDR6_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", strm_req_info->Port6);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_PORT6_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	msg_ctrl_inet_ntoa(msgcode_buf, strm_req_info->Ipaddr7);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_IPADDR7_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", strm_req_info->Port7);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_PORT7_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	msg_ctrl_inet_ntoa(msgcode_buf, strm_req_info->Ipaddr8);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_IPADDR8_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", strm_req_info->Port8);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_PORT8_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	msg_ctrl_inet_ntoa(msgcode_buf, strm_req_info->Ipaddr9);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_IPADDR9_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", strm_req_info->Port9);
	upper_msg_package_add_xml_leaf(StrmReq_node, MSG_PORT9_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,  XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}


int32_t upper_msg_package_record_xml(int8_t *out_buf, int32_t result, int8_t *file, int8_t *pass_key, int8_t *user_id, int8_t *rec_id, int32_t opt_type, uint32_t rec_status)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen = 0;;

	int8_t		value_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pxml = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;


	if(NULL == out_buf || NULL == file){
		nslog(NS_ERROR, " ---- param error");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	sprintf((char *)value_buf, "%d", MSG_RECORD_REQ);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, value_buf);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, MSG_RECORD_PASS);

	xmlNodePtr body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	xmlNodePtr tmp_node = xmlNewNode(NULL, MSG_RECORD_REP_KEY);
	xmlAddChild(body_node, tmp_node);

	//RecStatus
	r_bzero(value_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)value_buf, "%d", rec_status);
	upper_msg_package_add_xml_leaf(tmp_node, MSG_RECSTATUS_KEY, value_buf);

	//Result
	r_bzero(value_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)value_buf, "%d", result);
	upper_msg_package_add_xml_leaf(tmp_node, MSG_RESULT_KEY, value_buf);

	//RecordID
	r_bzero(value_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)value_buf, "%s", rec_id);
	upper_msg_package_add_xml_leaf(tmp_node, MSG_RECORDID_KEY, value_buf);

	//FileName
	r_bzero(value_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)value_buf, "%s", file);
	upper_msg_package_add_xml_leaf(tmp_node, MSG_FILENAME_KEY, value_buf);

	//OptType
	r_bzero(value_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)value_buf, "%d", opt_type);
	upper_msg_package_add_xml_leaf(tmp_node, MSG_OPTTYPE_KEY, value_buf);

	//PassKey
	r_bzero(value_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)value_buf, "%s", pass_key);//如果是预约的，则返回为AllPlatform
	upper_msg_package_add_xml_leaf(tmp_node, MSG_PASSKEY_KEY, value_buf);

	//user_id
	r_bzero(value_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)value_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, value_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,  XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}


int32_t upper_msg_package_warn_req_msg(int8_t *out_buf, int32_t msg_code, int8_t *pass_key, int8_t *user_id, WarnInfo *w_info)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen 		= 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf 	= NULL;
	xmlDocPtr	pxml 		= NULL;
	xmlNodePtr	root_node 	= NULL;
	xmlNodePtr	head_node 	= NULL;
	xmlNodePtr	body_node 	= NULL;
	xmlNodePtr	warn_node 	= NULL;

	if(NULL == out_buf){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//RequestMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	//msg code
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msg_code);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	//PassKey
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//warn
	warn_node = xmlNewNode(NULL, MSG_WARNREQ_KEY);
	xmlAddChild(body_node, warn_node);

	//id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", w_info->id);
	upper_msg_package_add_xml_leaf(warn_node, MSG_WARNID_KEY, msgcode_buf);

	//source
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", w_info->source);
	upper_msg_package_add_xml_leaf(warn_node, MSG_WARNSOURCE_KEY, msgcode_buf);

	//roomid
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", w_info->room_id);
	upper_msg_package_add_xml_leaf(warn_node, MSG_ROOMID_KEY, msgcode_buf);

	//codecid
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", w_info->codec_id);
	upper_msg_package_add_xml_leaf(warn_node, MSG_WARNCODECID_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}

int32_t upper_msg_package_log_req_msg(int8_t *out_buf, int32_t msg_code, int8_t *pass_key, int8_t *user_id, int32_t level, int8_t *content)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen 		= 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf 	= NULL;
	xmlDocPtr	pxml 		= NULL;
	xmlNodePtr	root_node 	= NULL;
	xmlNodePtr	head_node 	= NULL;
	xmlNodePtr	body_node 	= NULL;
	xmlNodePtr	log_node 	= NULL;

	if(NULL == out_buf){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//RequestMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	//msg code
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msg_code);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	//PassKey
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//log
	log_node = xmlNewNode(NULL, MSG_LOGREQ_KEY);
	xmlAddChild(body_node, log_node);

	//level
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", level);
	upper_msg_package_add_xml_leaf(log_node, MSG_LOGLEVEL_KEY, msgcode_buf);

	//content
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", content);
	upper_msg_package_add_xml_leaf(log_node, MSG_LOGCONTENT_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}

int32_t upper_msg_package_req_enc_info(int32_t room_id, int8_t *out_buf, int32_t msgcode, int8_t *pass_key, int8_t *user_id)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen 		= 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf 	= NULL;
	xmlDocPtr	pxml 		= NULL;
	xmlNodePtr	root_node 	= NULL;
	xmlNodePtr	head_node 	= NULL;
	xmlNodePtr	body_node 	= NULL;
	xmlNodePtr	RoomInfoReq_node = NULL;

	if(NULL == out_buf){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//RequestMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msgcode);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	RoomInfoReq_node = xmlNewNode(NULL, MSG_ROOMINFOREQ_KEY);
	xmlAddChild(body_node, RoomInfoReq_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", room_id);
	upper_msg_package_add_xml_leaf(RoomInfoReq_node, MSG_ROOMID_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}
int32_t upper_msg_package_rec_ctrl_resp_msg(int8_t *out_buf, int32_t msg_code, int32_t ret_code, int8_t *user_id, int32_t result)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen 		= 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf 	= NULL;
	xmlDocPtr	pxml 		= NULL;
	xmlNodePtr	root_node 	= NULL;
	xmlNodePtr	head_node 	= NULL;
	xmlNodePtr	body_node 	= NULL;
	xmlNodePtr	RecCtrlResp_node = NULL;

	if(NULL == out_buf){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//ResponseMsg
	root_node = xmlNewNode(NULL, RESP_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	//msg code
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msg_code);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	//return code
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", ret_code);
	upper_msg_package_add_xml_leaf(head_node, MSG_RETURNCODE_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//RecCtrlResp
	RecCtrlResp_node = xmlNewNode(NULL, MSG_RECCTRLRESP_KEY);
	xmlAddChild(body_node, RecCtrlResp_node);

	//Result
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", result);
	upper_msg_package_add_xml_leaf(RecCtrlResp_node, MSG_RESULT_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}

//把enc_info 打包成xml
int32_t upper_msg_package_quality_info(enc_info *q_info, int8_t *msg_xml, int32_t enc_id, int32_t room_id, int32_t msgcode, int8_t *pass_key, int8_t *user_id)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen		= 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf	= NULL;
	xmlDocPtr	pxml		= NULL;
	xmlNodePtr	root_node	= NULL;
	xmlNodePtr	head_node	= NULL;
	xmlNodePtr	body_node	= NULL;
	xmlNodePtr  QualityInfo_ptr  = NULL;
	xmlNodePtr	RoomInfoReq_node = NULL;

	if(NULL == msg_xml || NULL == q_info){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//ResponseMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msgcode);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//SetRoomInfoReq
	RoomInfoReq_node = xmlNewNode(NULL, MSG_SETROOMINFOSREQ_KEY);
	xmlAddChild(body_node, RoomInfoReq_node);

	//RoomInfo
	xmlNodePtr RoomInfo_ptr = NULL;
	RoomInfo_ptr = xmlNewNode(NULL, MSG_ROOMINFO_KEY);
	xmlAddChild(RoomInfoReq_node, RoomInfo_ptr);

	//RoomId
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", room_id);
	upper_msg_package_add_xml_leaf(RoomInfo_ptr, MSG_ROOMID_KEY, msgcode_buf);

	//EncInfo
	xmlNodePtr EncInfo_ptr = NULL;
	EncInfo_ptr = xmlNewNode(NULL, MSG_ENCINFO_KEY);
	xmlAddChild(RoomInfo_ptr, EncInfo_ptr);

	//ID
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", enc_id);
	upper_msg_package_add_xml_leaf(EncInfo_ptr, MSG_ENCINFOID_KEY, msgcode_buf);

	//HD
	if (q_info->HD_QuaInfo.RateType != -1) {
		//HD_QualityInfo
		QualityInfo_ptr = xmlNewNode(NULL, MSG_QUALITYINFO_KEY);
		xmlAddChild(EncInfo_ptr, QualityInfo_ptr);

		//RateType
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", q_info->HD_QuaInfo.RateType);
		upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_RATETYPE_KEY, msgcode_buf);

		//EncBitrate
		if (q_info->HD_QuaInfo.EncBitrate != -1) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", q_info->HD_QuaInfo.EncBitrate);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCBITRATE_KEY, msgcode_buf);
		}

		//EncWidth
		if (q_info->HD_QuaInfo.EncWidth != -1) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", q_info->HD_QuaInfo.EncWidth);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCWIDTH_KEY, msgcode_buf);
		}

		//EncHeight
		if (q_info->HD_QuaInfo.EncHeight != -1) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", q_info->HD_QuaInfo.EncHeight);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCHEIGHT_KEY, msgcode_buf);
		}

		//EncFrameRate
		if (q_info->HD_QuaInfo.EncFrameRate != -1) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", q_info->HD_QuaInfo.EncFrameRate);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCFRAMERATE_KEY, msgcode_buf);
		}
	}

	//SD
	if (q_info->SD_QuaInfo.RateType != -1) {
		//SD_QualityInfo
		QualityInfo_ptr = xmlNewNode(NULL, MSG_QUALITYINFO_KEY);
		xmlAddChild(EncInfo_ptr, QualityInfo_ptr);

		//RateType
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", q_info->SD_QuaInfo.RateType);
		upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_RATETYPE_KEY, msgcode_buf);

		//EncBitrate
		if (q_info->SD_QuaInfo.EncBitrate != -1) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", q_info->SD_QuaInfo.EncBitrate);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCBITRATE_KEY, msgcode_buf);
		}

		//EncWidth
		if (q_info->SD_QuaInfo.EncWidth != -1) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", q_info->SD_QuaInfo.EncWidth);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCWIDTH_KEY, msgcode_buf);
		}

		//EncHeight
		if (q_info->SD_QuaInfo.EncHeight != -1) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", q_info->SD_QuaInfo.EncHeight);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCHEIGHT_KEY, msgcode_buf);
		}

		//EncFrameRate
		if (q_info->SD_QuaInfo.EncFrameRate != -1) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", q_info->SD_QuaInfo.EncFrameRate);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCFRAMERATE_KEY, msgcode_buf);
		}
	}

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);


	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(msg_xml, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}

int32_t upper_msg_package_audio_info(audio_info *a_info, int8_t *msg_xml, int32_t room_id, int32_t msgcode, int8_t *pass_key, int8_t *user_id)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen		= 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf	= NULL;
	xmlDocPtr	pxml		= NULL;
	xmlNodePtr	root_node	= NULL;
	xmlNodePtr	head_node	= NULL;
	xmlNodePtr	body_node	= NULL;
	xmlNodePtr  RoomInfo_ptr= NULL;
	xmlNodePtr	SetRoomAudioReq_node = NULL;

	if(NULL == msg_xml || NULL == a_info){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//ResponseMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", msgcode);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//SetRoomAudioReq
	SetRoomAudioReq_node = xmlNewNode(NULL, MSG_SETAUDIOINFOSREQ_KEY);
	xmlAddChild(body_node, SetRoomAudioReq_node);

	//RoomInfo
	RoomInfo_ptr = xmlNewNode(NULL, MSG_ROOMINFO_KEY);
	xmlAddChild(SetRoomAudioReq_node, RoomInfo_ptr);

	//RoomId
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", room_id);
	upper_msg_package_add_xml_leaf(RoomInfo_ptr, MSG_ROOMID_KEY, msgcode_buf);

	//AudioInfo
	xmlNodePtr AudioInfo_ptr = NULL;
	AudioInfo_ptr = xmlNewNode(NULL, MSG_AUDIOINFO_KEY);
	xmlAddChild(RoomInfo_ptr, AudioInfo_ptr);

	//InputMode
	if (a_info->InputMode != -1) {
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", a_info->InputMode);
		upper_msg_package_add_xml_leaf(AudioInfo_ptr, MSG_INPUTMODE_KEY, msgcode_buf);
	}

	//SampleRate
	if (a_info->SampleRate != -1) {
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", a_info->SampleRate);
		upper_msg_package_add_xml_leaf(AudioInfo_ptr, MSG_SAMPLERATE_KEY, msgcode_buf);
	}

	//Bitrate
	if (a_info->Bitrate != -1) {
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", a_info->Bitrate);
		upper_msg_package_add_xml_leaf(AudioInfo_ptr, MSG_BITRATE_KEY, msgcode_buf);
	}

	//Lvolume
	if (a_info->Lvolume != -1) {
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", a_info->Lvolume);
		upper_msg_package_add_xml_leaf(AudioInfo_ptr, MSG_LVOLUME_KEY, msgcode_buf);
	}

	//Rvolume
	if (a_info->Rvolume != -1) {
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", a_info->Rvolume);
		upper_msg_package_add_xml_leaf(AudioInfo_ptr, MSG_RVOLUME_KEY, msgcode_buf);
	}

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);


	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(msg_xml, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}


int32_t upper_msg_package_remote_ctrl_info(RemoteCtrlInfo *remote_ctrl_info, int8_t *msg_xml, int8_t *pass_key, int8_t *user_id, int32_t room_id)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen		= 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf	= NULL;
	xmlDocPtr	pxml		= NULL;
	xmlNodePtr	root_node	= NULL;
	xmlNodePtr	head_node	= NULL;
	xmlNodePtr	body_node	= NULL;
	xmlNodePtr	RemoteCtrlReq = NULL;

	if(NULL == msg_xml || NULL == remote_ctrl_info){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//ResponseMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", MSG_REMOTE_CTRL);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pass_key);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//RemoteCtrlReq
	RemoteCtrlReq = xmlNewNode(NULL, MSG_REMOTECTRLREQ_KEY);
	xmlAddChild(body_node, RemoteCtrlReq);

	//RoomInfo
	xmlNodePtr RoomInfo_ptr = NULL;
	RoomInfo_ptr = xmlNewNode(NULL, MSG_ROOMINFO_KEY);
	xmlAddChild(RemoteCtrlReq, RoomInfo_ptr);

	//RoomId
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", room_id);
	upper_msg_package_add_xml_leaf(RoomInfo_ptr, MSG_ROOMID_KEY, msgcode_buf);

	//EncInfo
	xmlNodePtr EncInfo_ptr = NULL;
	EncInfo_ptr = xmlNewNode(NULL, MSG_ENCINFO_KEY);
	xmlAddChild(RoomInfo_ptr, EncInfo_ptr);

	//ID
	if (remote_ctrl_info->ID != -1) {
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", remote_ctrl_info->ID);
		upper_msg_package_add_xml_leaf(EncInfo_ptr, MSG_ENCINFOID_KEY, msgcode_buf);
	}

	//Type
	if (remote_ctrl_info->Type != -1) {
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", remote_ctrl_info->Type);
		upper_msg_package_add_xml_leaf(EncInfo_ptr, MSG_TYPE_KEY, msgcode_buf);
	}

	//Addr
	if (remote_ctrl_info->Addr != -1) {
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", remote_ctrl_info->Addr);
		upper_msg_package_add_xml_leaf(EncInfo_ptr, MSG_ADDR_KEY, msgcode_buf);
	}

	//Speed
	if (remote_ctrl_info->Speed != -1) {
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", remote_ctrl_info->Speed);
		upper_msg_package_add_xml_leaf(EncInfo_ptr, MSG_SPEED_KEY, msgcode_buf);
	}

	//Num
	if (remote_ctrl_info->Num != -1) {
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", remote_ctrl_info->Num);
		upper_msg_package_add_xml_leaf(EncInfo_ptr, MSG_NUM_KEY, msgcode_buf);
	}

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(msg_xml, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}

int32_t upper_msg_package_req_system_info(int8_t *out_buf, int8_t *user_id)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen		= 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf	= NULL;
	xmlDocPtr	pxml		= NULL;
	xmlNodePtr	root_node	= NULL;
	xmlNodePtr	head_node	= NULL;
	xmlNodePtr	body_node	= NULL;


	if(NULL == out_buf){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//ResponseMsg
	root_node = xmlNewNode(NULL, RESP_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", MSG_SYSTEM_REQ);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, MSG_RECORD_PASS);

	//MsgBody
	upper_msg_package_add_xml_leaf(root_node, MSG_BODY_KEY, "0");

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}

int32_t upper_msg_package_req_stream_info(RoomMsgEnv *pRoom, int8_t *out_buf, int8_t *user_id, int32_t Quality)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen		= 0;

	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf	= NULL;
	xmlDocPtr	pxml		= NULL;
	xmlNodePtr	root_node	= NULL;
	xmlNodePtr	head_node	= NULL;
	xmlNodePtr	body_node	= NULL;
	xmlNodePtr	StrmReq_ptr = NULL;

	if(NULL == out_buf){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
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
	sprintf((char *)msgcode_buf, "%d", MSG_REQUEST_CODE_STREAM);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	//PassKey
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, MSG_RECORD_PASS);

	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//StrmReq
	StrmReq_ptr= xmlNewNode(NULL, MSG_STREAMREQ_KEY);
	xmlAddChild(body_node, StrmReq_ptr);

	//RoomID
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.RoomId);
	upper_msg_package_add_xml_leaf(StrmReq_ptr, MSG_ROOMID_KEY, msgcode_buf);

	//Quality
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", Quality);
	upper_msg_package_add_xml_leaf(StrmReq_ptr, MSG_QUALITY_KEY, msgcode_buf);

	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

	return_code = ROOM_RETURN_SUCC;

EXIT:

	return return_code;;
}

int32_t upper_msg_package_set_quality_info(RoomMsgEnv *pRoom, int8_t *out_buf, int8_t *user_id, int32_t enc_id)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	int32_t 	xmllen		= 0;
	enc_info	*e_info		= NULL;
	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf	= NULL;
	xmlDocPtr	pxml		= NULL;
	xmlNodePtr	root_node	= NULL;
	xmlNodePtr	head_node	= NULL;
	xmlNodePtr	body_node	= NULL;
	xmlNodePtr	QualityInfo_ptr  = NULL;
	xmlNodePtr	RoomInfoReq_node = NULL;

	if(NULL == pRoom || NULL == out_buf || NULL == user_id){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	if (enc_id < 1 || enc_id > ROOM_ENC_NUM) {
		nslog(NS_ERROR, "--- failed, enc_id is [%d]", enc_id);
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//ResponseMsg
	root_node = xmlNewNode(NULL, REQ_ROOT_KEY);
	xmlDocSetRootElement(pxml, root_node);

	//MsgHead
	head_node = xmlNewNode(NULL, MSG_HEAD_KEY);
	xmlAddChild(root_node, head_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", MSG_SET_QUALITY_INFO);
	upper_msg_package_add_xml_leaf(head_node, MSG_CODE_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", MSG_ROOMCTRL_PASSKEY);
	upper_msg_package_add_xml_leaf(head_node, MSG_PASSKEY_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//SetRoomInfoReq
	RoomInfoReq_node = xmlNewNode(NULL, MSG_SETROOMINFOSREQ_KEY);
	xmlAddChild(body_node, RoomInfoReq_node);

	//RoomInfo
	xmlNodePtr RoomInfo_ptr = NULL;
	RoomInfo_ptr = xmlNewNode(NULL, MSG_ROOMINFO_KEY);
	xmlAddChild(RoomInfoReq_node, RoomInfo_ptr);

	//RoomId
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.RoomId);
	upper_msg_package_add_xml_leaf(RoomInfo_ptr, MSG_ROOMID_KEY, msgcode_buf);

	//EncInfo
	xmlNodePtr EncInfo_ptr = NULL;
	EncInfo_ptr = xmlNewNode(NULL, MSG_ENCINFO_KEY);
	xmlAddChild(RoomInfo_ptr, EncInfo_ptr);

	//ID
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", enc_id);
	upper_msg_package_add_xml_leaf(EncInfo_ptr, MSG_ENCINFOID_KEY, msgcode_buf);

	e_info = &pRoom->RoomInfo.EncInfo[enc_id-1];

	return_code = ROOM_RETURN_FAIL;

	//HD
	if (e_info->HD_QuaInfo.EncBitrate > 0 || e_info->HD_QuaInfo.EncFrameRate > 0) {
		//HD_QualityInfo
		QualityInfo_ptr = xmlNewNode(NULL, MSG_QUALITYINFO_KEY);
		xmlAddChild(EncInfo_ptr, QualityInfo_ptr);

		//RateType
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", e_info->HD_QuaInfo.RateType);
		upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_RATETYPE_KEY, msgcode_buf);

		//EncBitrate
		if (e_info->HD_QuaInfo.EncBitrate > 0) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", e_info->HD_QuaInfo.EncBitrate);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCBITRATE_KEY, msgcode_buf);
		}

		//EncFrameRate
		if (e_info->HD_QuaInfo.EncFrameRate > 0) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", e_info->HD_QuaInfo.EncFrameRate);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCFRAMERATE_KEY, msgcode_buf);
		}

		return_code = ROOM_RETURN_SUCC;
	}
#if 0
	//SD
	if (e_info->SD_QuaInfo.EncBitrate > 0 || e_info->SD_QuaInfo.EncFrameRate > 0) {
		//SD_QualityInfo
		QualityInfo_ptr = xmlNewNode(NULL, MSG_QUALITYINFO_KEY);
		xmlAddChild(EncInfo_ptr, QualityInfo_ptr);

		//RateType
		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", e_info->SD_QuaInfo.RateType);
		upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_RATETYPE_KEY, msgcode_buf);

		//EncBitrate
		if (e_info->SD_QuaInfo.EncBitrate > 0) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", e_info->SD_QuaInfo.EncBitrate);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCBITRATE_KEY, msgcode_buf);
		}
		//EncFrameRate
		if (e_info->SD_QuaInfo.EncFrameRate > 0) {
			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", e_info->SD_QuaInfo.EncFrameRate);
			upper_msg_package_add_xml_leaf(QualityInfo_ptr, MSG_ENCFRAMERATE_KEY, msgcode_buf);
		}

		return_code = ROOM_RETURN_SUCC;
	}
#endif
	//user_id
	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", user_id);
	upper_msg_package_add_xml_leaf(root_node, MSG_USERID_KEY, msgcode_buf);

	xmlDocDumpFormatMemoryEnc(pxml, &xml_buf, &xmllen,	XML_TEXT_CODE_TYPE, 1);
	r_memcpy(out_buf, xml_buf, xmllen);
	xmlFree(xml_buf);

	if(pxml != NULL){
		release_dom_tree(pxml);
	}

EXIT:

	return return_code;;
}


int32_t upper_msg_package_resp_room_info(RoomMsgEnv *pRoom, int8_t *out_buf, int32_t msgcode, int32_t ret_code, int8_t *pass_key, int8_t *user_id)
{
	int32_t 	xmllen = 0;
	int8_t		msgcode_buf[ROOM_MSG_VAL_LEN] = {0};

	xmlChar 	*xml_buf = NULL;
	xmlDocPtr	pxml = NULL;
	xmlNodePtr	root_node = NULL;
	xmlNodePtr	head_node = NULL;
	xmlNodePtr	body_node = NULL;


	if(NULL == out_buf){
		nslog(NS_ERROR, "---params is NULL!");
		return ROOM_RETURN_FAIL;
	}

	pxml = xmlNewDoc(XML_DOC_VERSION);

	//ResponseMsg
	root_node = xmlNewNode(NULL, RESP_ROOT_KEY);
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

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", ret_code);
	upper_msg_package_add_xml_leaf(head_node, MSG_RETURNCODE_KEY, msgcode_buf);


	//MsgBody
	body_node = xmlNewNode(NULL, MSG_BODY_KEY);
	xmlAddChild(root_node, body_node);

	//RoomInfo
	xmlNodePtr	RoomInfo_node = NULL;
	RoomInfo_node = xmlNewNode(NULL, MSG_ROOMINFO_KEY);
	xmlAddChild(body_node, RoomInfo_node);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.RoomId);
	upper_msg_package_add_xml_leaf(RoomInfo_node, MSG_ROOMID_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.ConnectStatus);
	upper_msg_package_add_xml_leaf(RoomInfo_node, MSG_CONNECTSTATUS_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.RecordMaxTime);
	upper_msg_package_add_xml_leaf(RoomInfo_node, MSG_RECMAXTIME_REP_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.RecordStatus);
	upper_msg_package_add_xml_leaf(RoomInfo_node, MSG_RECSTATUS_KEY, msgcode_buf);

	r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
	sprintf((char *)msgcode_buf, "%s", pRoom->RoomInfo.RcdName);
	upper_msg_package_add_xml_leaf(RoomInfo_node, MSG_RECNAME_KEY, msgcode_buf);

	//AudioInfo, 第一路高低都没回教室信息，则不封音频信息，以免封的是全零情况
	if (pRoom->RoomInfo.room_info_mask & (1 << pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.MsgQueType)||\
		pRoom->RoomInfo.room_info_mask & (1 << pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.MsgQueType)) {

		xmlNodePtr	AudioInfo_node = NULL;
		AudioInfo_node = xmlNewNode(NULL, MSG_AUDIOINFO_KEY);
		xmlAddChild(RoomInfo_node, AudioInfo_node);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.AudioInfo.InputMode);
		upper_msg_package_add_xml_leaf(AudioInfo_node, MSG_INPUTMODE_KEY, msgcode_buf);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.AudioInfo.SampleRate);
		upper_msg_package_add_xml_leaf(AudioInfo_node, MSG_SAMPLERATE_KEY, msgcode_buf);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.AudioInfo.Bitrate);
		upper_msg_package_add_xml_leaf(AudioInfo_node, MSG_BITRATE_KEY, msgcode_buf);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.AudioInfo.Lvolume);
		upper_msg_package_add_xml_leaf(AudioInfo_node, MSG_LVOLUME_KEY, msgcode_buf);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.AudioInfo.Rvolume);
		upper_msg_package_add_xml_leaf(AudioInfo_node, MSG_RVOLUME_KEY, msgcode_buf);
	}

	//EncInfo
	int32_t i = 0, j = 0;
	xmlNodePtr	EncInfo_node		= NULL;
	xmlNodePtr	HD_QualityInfo_node = NULL;
	xmlNodePtr	SD_QualityInfo_node = NULL;

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		}

		EncInfo_node = xmlNewNode(NULL, MSG_ENCINFO_KEY);
		xmlAddChild(RoomInfo_node, EncInfo_node);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].ID);
		upper_msg_package_add_xml_leaf(EncInfo_node, MSG_ENCINFOID_KEY, msgcode_buf);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		msg_ctrl_inet_ntoa(msgcode_buf, pRoom->RoomInfo.EncInfo[i].EncIP);
		upper_msg_package_add_xml_leaf(EncInfo_node, MSG_ENCIP_KEY, msgcode_buf);

		r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
		sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].Status);
		upper_msg_package_add_xml_leaf(EncInfo_node, MSG_STATUS_KEY, msgcode_buf);

		 //HD_QualityInfo
//		if ((pRoom->RoomInfo.room_info_mask & (1 << pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType)) || \
//			(pRoom->RoomInfo.room_info_mask & (1 << pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType))) {

			HD_QualityInfo_node = xmlNewNode(NULL, MSG_QUALITYINFO_KEY);
			xmlAddChild(EncInfo_node, HD_QualityInfo_node);

			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.RateType);
			upper_msg_package_add_xml_leaf(HD_QualityInfo_node, MSG_RATETYPE_KEY, msgcode_buf);

			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.EncBitrate);
			if (pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.EncBitrate > 0)
				upper_msg_package_add_xml_leaf(HD_QualityInfo_node, MSG_ENCBITRATE_KEY, msgcode_buf);

			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.EncWidth);
			if (pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.EncWidth > 0)
				upper_msg_package_add_xml_leaf(HD_QualityInfo_node, MSG_ENCWIDTH_KEY, msgcode_buf);

			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.EncHeight);
			if (pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.EncHeight > 0)
				upper_msg_package_add_xml_leaf(HD_QualityInfo_node, MSG_ENCHEIGHT_KEY, msgcode_buf);

			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.EncFrameRate);
			if (pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.EncFrameRate > 0)
				upper_msg_package_add_xml_leaf(HD_QualityInfo_node, MSG_ENCFRAMERATE_KEY, msgcode_buf);
//		}

		//JPEG 无低码流信息
		if (pRoom->RoomInfo.EncInfo[i].EncodeType == EncodeType_JPEG) {
			continue;
		}

		//SD_QualityInfo
//		if ((pRoom->RoomInfo.room_info_mask & (1 << pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType)) || \
//			(pRoom->RoomInfo.room_info_mask & (1 << pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType))) {

			SD_QualityInfo_node = xmlNewNode(NULL, MSG_QUALITYINFO_KEY);
			xmlAddChild(EncInfo_node, SD_QualityInfo_node);

			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.RateType);
			upper_msg_package_add_xml_leaf(SD_QualityInfo_node, MSG_RATETYPE_KEY, msgcode_buf);

			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.EncBitrate);
			if (pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.EncBitrate > 0)
				upper_msg_package_add_xml_leaf(SD_QualityInfo_node, MSG_ENCBITRATE_KEY, msgcode_buf);

			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.EncWidth);
			if (pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.EncWidth > 0)
				upper_msg_package_add_xml_leaf(SD_QualityInfo_node, MSG_ENCWIDTH_KEY, msgcode_buf);

			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.EncHeight);
			if (pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.EncHeight > 0)
				upper_msg_package_add_xml_leaf(SD_QualityInfo_node, MSG_ENCHEIGHT_KEY, msgcode_buf);

			r_bzero(msgcode_buf, ROOM_MSG_VAL_LEN);
			sprintf((char *)msgcode_buf, "%d", pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.EncFrameRate);
			if (pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.EncFrameRate > 0)
				upper_msg_package_add_xml_leaf(SD_QualityInfo_node, MSG_ENCFRAMERATE_KEY, msgcode_buf);
//		}
	}

	//user_id
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


int32_t upper_msg_check_room_info_req_id(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t 		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node 	= NULL;
	int32_t 	ret_val		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_ROOMINFOREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_ROOMID_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		get_current_node_value(tmp_buf, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);
		if (atoi(tmp_buf) == roomID) {
			ret_val = 1;
			break;
		}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

int32_t upper_msg_time_out_resp(RoomMsgEnv *pRoom, int32_t socket, pthread_mutex_t *mutex, MsgHeader *head, uint32_t msg_code, int8_t *pass_key, int8_t *user_id, int32_t room_id)
{
	int8_t  ret_val[]  = "0";
	int8_t *xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ROOM_RETURN_FAIL;
	}

	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_reponse_xml(xml_buf, msg_code, ret_val, pass_key, user_id, room_id);
	upper_msg_tcp_send(socket, mutex, head, xml_buf);

	nslog(NS_INFO, "time out!........ msg_code = %d, pass_key = %s", msg_code, pass_key);

	if (xml_buf)
		r_free(xml_buf);
}

int32_t upper_msg_connect_time_out_resp(RoomMsgEnv *pRoom, int32_t socket, pthread_mutex_t *mutex, MsgHeader *head, uint32_t msg_code, int8_t *pass_key, int8_t *user_id, int32_t room_id)
{
	int32_t valid_num	= 0;
	int8_t  ret_val[2]  = {0};
	int8_t *xml_buf 	= NULL;

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ROOM_RETURN_FAIL;
	}

	valid_num = upper_msg_get_valid_stream_num(pRoom);
	if (valid_num) {
		ret_val[0] = '1';
	} else {
		ret_val[0] = '0';
	}

	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_connc_room_req_xml(xml_buf, msg_code, ret_val, pass_key, 1, user_id, room_id);
	upper_msg_tcp_send(socket, mutex, head, xml_buf);

	nslog(NS_INFO, "time out!........ msg_code = %d, pass_key = %s, upper_msg_get_valid_stream_num = %d", msg_code, pass_key, valid_num);

	if (xml_buf)
		r_free(xml_buf);
}

int32_t upper_msg_record_time_out_resp(RoomMsgEnv *pRoom, int32_t socket, pthread_mutex_t *mutex, MsgHeader *head, uint32_t msg_code, int8_t *pass_key, int8_t *user_id, int32_t room_id)
{
	int8_t *xml_buf						= NULL;
	int32_t result 						= 0;
	int32_t ret_val						= 0;
	int8_t  rec_file[ROOM_MSG_VAL_LEN]  = {0};
	int8_t  rec_id[ROOM_MSG_VAL_LEN]  = {0};

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ROOM_RETURN_FAIL;
	}

	//启动录制
	ret_val = lower_msg_start_record(pRoom);
	if (ret_val == ROOM_RETURN_FAIL) {
		result = 0;
	} else {
		result = 1;
	}

	//启动录制不成功，则告诉编码器解锁分辨率
	if(!result) {
		nslog(NS_WARN, "--- start record error, send stop xml to enc! ret_value = %d\n", result);

		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		lower_msg_package_record_req(xml_buf, NULL, 0);
		lower_msg_send_xml_to_enc(pRoom, xml_buf);
	}

	if (pRoom->record_handle) {
		pRoom->RecStatusInfo.RecStatus = lower_msg_opt_type_trans(pRoom->record_handle->get_record_status(pRoom->record_handle));
		r_strcpy(rec_file, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
		r_strcpy(rec_id, pRoom->record_handle->get_RecordID(pRoom->record_handle));
	} else {
		pRoom->RecStatusInfo.RecStatus = 0;
		r_strcpy(rec_file, pRoom->RecInfo.CourseName);
		r_strcpy(rec_id, pRoom->RecInfo.RecordID);
	}

	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_record_xml(xml_buf, result, rec_file, pass_key, user_id, rec_id, pRoom->RecInfo.OptType, pRoom->RecStatusInfo.RecStatus);
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

	nslog(NS_INFO, "time out!........ msg_code = %d, pass_key = %s, result = %d", msg_code, pass_key, result);

	if (xml_buf)
		r_free(xml_buf);
}

xmlNodePtr upper_msg_get_room_info_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int32_t 	ret			= 0;
	int8_t 		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node 	= NULL;
	xmlNodePtr 	ret_val		= NULL;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_SETROOMINFOSREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_ROOMINFO_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

xmlNodePtr upper_msg_get_room_info_resp_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int32_t 	ret			= 0;
	int8_t 		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node 	= NULL;
	xmlNodePtr 	ret_val		= NULL;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_ROOMINFO_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}


xmlNodePtr upper_msg_get_audio_info_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int32_t 	ret			= 0;
	int8_t 		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node 	= NULL;
	xmlNodePtr 	ret_val		= NULL;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_SETAUDIOINFOSREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_ROOMINFO_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

xmlNodePtr upper_msg_get_resp_RoomInfo_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t 		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node 	= NULL;
	xmlNodePtr 	ret_val		= NULL;
	int32_t 	ret 		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_ROOMINFO_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}


xmlNodePtr upper_msg_get_rec_ctrl_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t 		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node 	= NULL;
	xmlNodePtr 	ret_val		= NULL;
	int32_t 	ret			= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_RECCTRLREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

xmlNodePtr upper_msg_get_req_code_stream_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t 	ret			= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_STREAMREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	ret_val = tmp_node;

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

xmlNodePtr upper_msg_get_remote_ctrl_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t 	ret 		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_REMOTECTRLREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_ROOMINFO_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

int32_t upper_msg_save_remote_ctrl_info_by_id(RemoteCtrlInfo *remote_ctrl_info, xmlNodePtr pNode, xmlDocPtr pdoc, int32_t enc_id)
{
	xmlNodePtr 	tmp_node 					= NULL;
	int8_t 		tmp_buf[ROOM_MSG_VAL_LEN] 	= {0};
	int32_t 	ret							= 0;

	tmp_node = upper_msg_get_enc_info_node_by_id(pNode, pdoc, enc_id);
	if (!tmp_node) {
		nslog(NS_WARN, "upper_msg_get_enc_info_node_by_id error, enc_id = %d", enc_id);
		return ROOM_RETURN_FAIL;
	}

	remote_ctrl_info->ID = enc_id;

	ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_TYPE_KEY, tmp_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		remote_ctrl_info->Type = atoi(tmp_buf);
	else
		remote_ctrl_info->Type = -1;

	ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ADDR_KEY, tmp_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		remote_ctrl_info->Addr = atoi(tmp_buf);
	else
		remote_ctrl_info->Addr = -1;

	ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_SPEED_KEY, tmp_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		remote_ctrl_info->Speed = atoi(tmp_buf);
	else
		remote_ctrl_info->Speed = -1;

	ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_NUM_KEY, tmp_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		remote_ctrl_info->Num = atoi(tmp_buf);
	else
		remote_ctrl_info->Num = -1;

	return ROOM_RETURN_SUCC;
}

xmlNodePtr upper_msg_get_iFrame_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t		ret   		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_IFRAMEREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

xmlNodePtr upper_msg_get_connect_room_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t 	ret 		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_CONNECTROOMREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

xmlNodePtr upper_msg_get_send_logo_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t 	ret 		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_UPLOADLOGREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

xmlNodePtr upper_msg_get_add_title_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t 	ret 		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_ADDTITLEREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

xmlNodePtr upper_msg_get_volume_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t 	ret 		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_GETVOLUMEREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

xmlNodePtr upper_msg_get_mute_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t 	ret 		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_MUTEREQ_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}


xmlNodePtr upper_msg_get_pic_adjust_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t 	ret 		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	int32_t i = 0;
	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

xmlNodePtr upper_msg_get_enc_info_node_by_id(xmlNodePtr encInfoNode, xmlDocPtr pdoc, uint32_t encInfoId)
{
	int8_t 		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node 	= NULL;
	xmlNodePtr 	ret_val		= NULL;
	int32_t 	ret 		= 0;

	tmp_node = get_children_node(encInfoNode, MSG_ENCINFO_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ENCINFOID_KEY, tmp_node, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == encInfoId) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}

int32_t upper_msg_init_enc_id(RoomMsgEnv *pRoom)
{
	int32_t i = 0;

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		pRoom->RoomInfo.EncInfo[i].ID = 0;
	}

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_save_set_room_info(xmlNodePtr curNode, xmlDocPtr pdoc, RoomMsgEnv *pRoom)
{
	int8_t 	value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr 	EncInfo_ptr	= NULL;
	uint32_t   	EncInfoId	= 0;
	uint32_t   	EncNum		= 0;
	int32_t 	ret			= 0;

	//录制名称
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ROOMNAME_KEY, curNode, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(pRoom->RoomInfo.RcdName, value);
	else
		r_bzero(pRoom->RoomInfo.RcdName, ROOM_MSG_VAL_LEN);

	//最大录制时间
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RECMAXTIME_REP_KEY, curNode, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		pRoom->RoomInfo.RecordMaxTime = atoi(value);
	else
		pRoom->RoomInfo.RecordMaxTime = 0;

	//取各路ENC 信息
	EncInfo_ptr = get_children_node(curNode, MSG_ENCINFO_KEY);
	for (; EncInfo_ptr != NULL; EncInfo_ptr = upper_msg_get_next_samename_node(EncInfo_ptr)) {
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCINFOID_KEY, EncInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			EncInfoId = atoi(value);
		else
			continue;

		if (EncInfoId < 1 || EncInfoId > ROOM_ENC_NUM) {
			continue;
		}

		pRoom->RoomInfo.EncInfo[EncInfoId-1].ID 		= 0;
		pRoom->RoomInfo.EncInfo[EncInfoId-1].EncodeType = EncodeType_INVALID;
		pRoom->RoomInfo.EncInfo[EncInfoId-1].EncIP 		= 0;

		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCIP_KEY, EncInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC && r_strlen(value)) {
			pRoom->RoomInfo.EncInfo[EncInfoId-1].ID 		= EncInfoId;
			pRoom->RoomInfo.EncInfo[EncInfoId-1].EncodeType = EncodeType_INVALID;
			pRoom->RoomInfo.EncInfo[EncInfoId-1].EncIP = msg_ctrl_inet_aton(value, NULL);
			EncNum++;
		}
	}

	pRoom->RoomInfo.EncNum = EncNum;

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_cmp_room_info(xmlNodePtr curNode, xmlDocPtr pdoc, RoomMsgEnv *pRoom)
{
	int8_t	value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	EncInfo_ptr = NULL;
	uint32_t	EncInfoId	= 0;
	uint32_t	EncNum		= 0;
	int32_t 	ret 		= 0;
	int32_t 	i 			= 0;

	room_info	r_info;


	//取各路ENC 信息
	EncInfo_ptr = get_children_node(curNode, MSG_ENCINFO_KEY);
	for (; EncInfo_ptr != NULL; EncInfo_ptr = upper_msg_get_next_samename_node(EncInfo_ptr)) {
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCINFOID_KEY, EncInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			EncInfoId = atoi(value);
		else
			continue;

		if (EncInfoId < 1 || EncInfoId > ROOM_ENC_NUM) {
			continue;
		}

		r_info.EncInfo[EncInfoId-1].ID = EncInfoId;
		r_info.EncInfo[EncInfoId-1].EncodeType = EncodeType_INVALID;

		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCIP_KEY, EncInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC) {
			r_info.EncInfo[EncInfoId-1].EncIP = msg_ctrl_inet_aton(value, NULL);
		} else {
			nslog(NS_ERROR, "upper_msg_get_leaf_value error");
			r_info.EncInfo[EncInfoId-1].ID = 0;
		}

		nslog(NS_INFO, "ID = %d:%d, EncIP = %d:%d", \
			pRoom->RoomInfo.EncInfo[EncInfoId-1].ID	  , r_info.EncInfo[EncInfoId-1].ID,\
			pRoom->RoomInfo.EncInfo[EncInfoId-1].EncIP, r_info.EncInfo[EncInfoId-1].EncIP);

		if (pRoom->RoomInfo.EncInfo[EncInfoId-1].ID == r_info.EncInfo[EncInfoId-1].ID && \
			pRoom->RoomInfo.EncInfo[EncInfoId-1].ID == 0) {
			continue;
		}

		if (pRoom->RoomInfo.EncInfo[EncInfoId-1].ID != r_info.EncInfo[EncInfoId-1].ID ||
			pRoom->RoomInfo.EncInfo[EncInfoId-1].EncIP != r_info.EncInfo[EncInfoId-1].EncIP) {
			EncNum = 0;
			break;
		}

		if (r_info.EncInfo[EncInfoId-1].ID)
			EncNum++;
	}

	nslog(NS_INFO, "EncNum = %d, pRoom->RoomInfo.EncNum = %d", EncNum, pRoom->RoomInfo.EncNum);

	if (EncNum != pRoom->RoomInfo.EncNum || pRoom->RoomInfo.EncNum == 0) {
		return ROOM_RETURN_FAIL;
	} else {
		return ROOM_RETURN_SUCC;
	}

}

int32_t upper_msg_save_quality_info(RoomMsgEnv *pRoom, enc_info *q_info, int32_t enc_id)
{
	int32_t 	return_code = ROOM_RETURN_SUCC;
	enc_info	*s_info		= NULL;

	if(NULL == pRoom || NULL == q_info){
		nslog(NS_ERROR, "--- failed, params is NULL!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	if (enc_id < 0 || enc_id >= ROOM_ENC_NUM) {
		nslog(NS_ERROR, "--- failed, enc_id error!");
		return_code = ROOM_RETURN_FAIL;
		goto EXIT;
	}

	s_info = &pRoom->RoomInfo.EncInfo[enc_id-1];

	//HD
	if (q_info->HD_QuaInfo.RateType != -1) {
		//RateType
		s_info->HD_QuaInfo.RateType = q_info->HD_QuaInfo.RateType;

		//EncBitrate
		if (q_info->HD_QuaInfo.EncBitrate != -1) {
			s_info->HD_QuaInfo.EncBitrate = q_info->HD_QuaInfo.EncBitrate;
		}

		//EncWidth
		if (q_info->HD_QuaInfo.EncWidth != -1) {
			s_info->HD_QuaInfo.EncWidth = q_info->HD_QuaInfo.EncWidth;
		}

		//EncHeight
		if (q_info->HD_QuaInfo.EncHeight != -1) {
			s_info->HD_QuaInfo.EncHeight = q_info->HD_QuaInfo.EncHeight;
		}

		//EncFrameRate
		if (q_info->HD_QuaInfo.EncFrameRate != -1) {
			s_info->HD_QuaInfo.EncFrameRate = q_info->HD_QuaInfo.EncFrameRate;
		}
	}

	//SD
	if (q_info->SD_QuaInfo.RateType != -1) {
		//RateType
		s_info->SD_QuaInfo.RateType = q_info->SD_QuaInfo.RateType;

		//EncBitrate
		if (q_info->SD_QuaInfo.EncBitrate != -1) {
			s_info->SD_QuaInfo.EncBitrate = q_info->SD_QuaInfo.EncBitrate;
		}

		//EncWidth
		if (q_info->SD_QuaInfo.EncWidth != -1) {
			s_info->SD_QuaInfo.EncWidth = q_info->SD_QuaInfo.EncWidth;
		}

		//EncHeight
		if (q_info->SD_QuaInfo.EncHeight != -1) {
			s_info->SD_QuaInfo.EncHeight = q_info->SD_QuaInfo.EncHeight;
		}

		//EncFrameRate
		if (q_info->SD_QuaInfo.EncFrameRate != -1) {
			s_info->SD_QuaInfo.EncFrameRate = q_info->SD_QuaInfo.EncFrameRate;
		}
	}

EXIT:

	return return_code;;
}


//解析xml 到enc_info
int32_t upper_msg_analyze_quality_info(xmlNodePtr curNode, xmlDocPtr pdoc, enc_info *e_info, int32_t enc_id)
{
	int8_t value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr EncInfo_ptr		= NULL;
	xmlNodePtr QualityInfo_ptr	= NULL;
	uint32_t   QualityInfo_id	= 0;
	quality_info   *q_info_tmp	= NULL;
	int32_t 		ret 		= 0;

	EncInfo_ptr = upper_msg_get_enc_info_node_by_id(curNode, pdoc, enc_id);
	if (EncInfo_ptr == NULL) {
		return ROOM_RETURN_FAIL;
	}

	e_info->ID = enc_id;

	e_info->SD_QuaInfo.RateType = -1;
	e_info->HD_QuaInfo.RateType = -1;

	//获取第一路流
	QualityInfo_ptr = get_children_node(EncInfo_ptr, MSG_QUALITYINFO_KEY);
	if (QualityInfo_ptr) {
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RATETYPE_KEY, QualityInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC) {
			QualityInfo_id = atoi(value);
			if (QualityInfo_id) {
				q_info_tmp = &e_info->SD_QuaInfo;
			}else {
				q_info_tmp = &e_info->HD_QuaInfo;
			}
		} else {
			nslog(NS_ERROR, "no RateType node");
			return ROOM_RETURN_FAIL;
		}

		//RateType
		q_info_tmp->RateType = QualityInfo_id;

		//EncBitrate
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCBITRATE_KEY, QualityInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			q_info_tmp->EncBitrate = atoi(value);
		else
			q_info_tmp->EncBitrate = -1;

		//EncWidth
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCWIDTH_KEY, QualityInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			q_info_tmp->EncWidth= atoi(value);
		else
			q_info_tmp->EncWidth = -1;

		//EncHeight
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCHEIGHT_KEY, QualityInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			q_info_tmp->EncHeight= atoi(value);
		else
			q_info_tmp->EncHeight = -1;

		//EncFrameRate
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCFRAMERATE_KEY, QualityInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			q_info_tmp->EncFrameRate= atoi(value);
		else
			q_info_tmp->EncFrameRate = -1;
	} else {
		return ROOM_RETURN_FAIL;
	}

	//获取第二路流
	QualityInfo_ptr = upper_msg_get_next_samename_node(QualityInfo_ptr);
	if (QualityInfo_ptr) {
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RATETYPE_KEY, QualityInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC) {
			QualityInfo_id = atoi(value);
			if (QualityInfo_id) {
				q_info_tmp = &e_info->SD_QuaInfo;
			}else {
				q_info_tmp = &e_info->HD_QuaInfo;
			}
		} else {
			nslog(NS_WARN, "second stream no RateType node");
			return ROOM_RETURN_SUCC;
		}

		//RateType
		q_info_tmp->RateType = QualityInfo_id;

		//EncBitrate
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCBITRATE_KEY, QualityInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			q_info_tmp->EncBitrate = atoi(value);
		else
			q_info_tmp->EncBitrate = -1;

		//EncWidth
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCWIDTH_KEY, QualityInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			q_info_tmp->EncWidth = atoi(value);
		else
			q_info_tmp->EncWidth = -1;

		//EncHeight
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCHEIGHT_KEY, QualityInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			q_info_tmp->EncHeight = atoi(value);
		else
			q_info_tmp->EncHeight = -1;

		//EncFrameRate
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCFRAMERATE_KEY, QualityInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			q_info_tmp->EncFrameRate = atoi(value);
		else
			q_info_tmp->EncFrameRate = -1;
	}

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_analyze_audio_info(xmlNodePtr curNode, xmlDocPtr pdoc, audio_info *a_info)
{
	int8_t 		value[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr 	AudioInfo_ptr	 		= NULL;
	int32_t 	ret 					= 0;

	if (curNode == NULL || a_info == NULL) {
		nslog(NS_ERROR, "error, curNode = %p, a_info = %p", curNode, a_info);
		return ROOM_RETURN_FAIL;
	}

	AudioInfo_ptr = get_children_node(curNode, MSG_AUDIOINFO_KEY);
	if (AudioInfo_ptr) {
		//InputMode
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_INPUTMODE_KEY, AudioInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			a_info->InputMode = atoi(value);
		else
			a_info->InputMode = -1;

		//SampleRate
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_SAMPLERATE_KEY, AudioInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			a_info->SampleRate = atoi(value);
		else
			a_info->SampleRate = -1;

		//Bitrate
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_BITRATE_KEY, AudioInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			a_info->Bitrate= atoi(value);
		else
			a_info->Bitrate = -1;

		//Lvolume
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_LVOLUME_KEY, AudioInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			a_info->Lvolume= atoi(value);
		else
			a_info->Lvolume = -1;

		//Rvolume
		r_bzero(value, ROOM_MSG_VAL_LEN);
		ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RVOLUME_KEY, AudioInfo_ptr, pdoc);
		if (ret == ROOM_RETURN_SUCC)
			a_info->Rvolume= atoi(value);
		else
			a_info->Rvolume = -1;
	}

	return ROOM_RETURN_SUCC;
}


int32_t upper_msg_analyze_system_info(RecInfo *rec_info, parse_xml_t *parseXml)
{
	xmlNodePtr 	tmp_node 				= NULL;
	int8_t		value[ROOM_MSG_VAL_LEN] = {0};
	int32_t 	ret 					= 0;

	//MsgBody
	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ROOM_RETURN_FAIL;
	}

	//PreRecInfoReq
	tmp_node = get_children_node(tmp_node, MSG_PRERECINFO_REP_KEY);
	if (!tmp_node) {
		return ROOM_RETURN_FAIL;
	}

	//RecPath
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RECPATH_REP_KEY, tmp_node, parseXml->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(rec_info->RecPath, value);
	else
		r_bzero(rec_info->RecPath, ROOM_MSG_VAL_LEN);

	//ServerSeries
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_SEVSERIES_REP_KEY, tmp_node, parseXml->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(rec_info->ServerSeries, value);
	else
		r_bzero(rec_info->ServerSeries, ROOM_MSG_VAL_LEN);

//	//RecordMaxTime
//	r_bzero(value, ROOM_MSG_VAL_LEN);
//	upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RECMAXTIME_REP_KEY, tmp_node, parseXml->pdoc);
//	rec_info->RecordMaxTime = atoi(value);

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_analyze_resp_server_info(ServerInfo *ser_info, parse_xml_t *parseXml)
{
	xmlNodePtr 	tmp_node 				= NULL;
	int8_t		value[ROOM_MSG_VAL_LEN] = {0};
	int32_t 	ret 					= 0;

	//MsgBody
	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ROOM_RETURN_FAIL;
	}

	//GetSysParam
	tmp_node = get_children_node(tmp_node, MSG_GETSYSPARAM_KEY);
	if (!tmp_node) {
		return ROOM_RETURN_FAIL;
	}

	//TimeServerAddr
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_TIMESERVERADDR_KEY, tmp_node, parseXml->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		ser_info->TimeServerAddr = msg_ctrl_inet_aton(value, NULL);

	//DiskMaxSpace
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_DISKMAXSPACE_KEY, tmp_node, parseXml->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		ser_info->DiskMaxSpace = atoi(value);

	//DiskAvailableSpace
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_DISKAVAILSPACE_KEY, tmp_node, parseXml->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		ser_info->DiskAvailableSpace = atoi(value);

	return ROOM_RETURN_SUCC;
}


int32_t upper_msg_analyze_rec_info(RecInfo *r_info, xmlNodePtr cur_node, xmlDocPtr pdoc)
{
	int8_t  value[ROOM_MSG_VAL_LEN] = {0};
	int8_t  note[ROOM_MSG_NOTE_LEN] = {0};
	int32_t ret = 0;

	//RecordID
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RECORDID_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(r_info->RecordID, value);
	else
		r_bzero(r_info->RecordID, ROOM_MSG_VAL_LEN);

	//OptType
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_OPTTYPE_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_info->OptType = atoi(value);

	//RoomName
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ROOMNAME_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(r_info->RoomName, value);
	else
		r_bzero(r_info->RoomName, ROOM_MSG_VAL_LEN);

	//AliasName
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ALIASNAME_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(r_info->AliasName, value);
	else
		r_bzero(r_info->AliasName, ROOM_MSG_VAL_LEN);

	//TeacherName
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_TEACHERNAME_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(r_info->TeacherName, value);
	else
		r_bzero(r_info->TeacherName, ROOM_MSG_VAL_LEN);

	//CourseName
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_COURSENAME_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(r_info->CourseName, value);
	else
		r_bzero(r_info->CourseName, ROOM_MSG_VAL_LEN);

	//Notes
	r_bzero(note, ROOM_MSG_NOTE_LEN);
	ret = upper_msg_get_leaf_value(note, ROOM_MSG_NOTE_LEN, MSG_NOTES_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(r_info->Notes, note);
	else
		r_bzero(r_info->Notes, ROOM_MSG_VAL_LEN);

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_analyze_stream_req_info(StrmReq *strm_req, xmlNodePtr cur_node, xmlDocPtr pdoc)
{
	int8_t  value[ROOM_MSG_VAL_LEN] = {0};
	int32_t ret = 0;

	//Quality
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_QUALITY_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Quality = atoi(value);
	else
		return ROOM_RETURN_FAIL;

	//EncodeIndex
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ENCODEINDEX_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(strm_req->EncodeIndex, value);
	else
		return ROOM_RETURN_FAIL;

	//Ipaddr1
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_IPADDR1_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Ipaddr1 = msg_ctrl_inet_aton(value, NULL);
	else
		strm_req->Ipaddr1 = 0;

	//Port1
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_PORT1_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Port1 = atoi(value);
	else
		strm_req->Port1 = 0;

	//Ipaddr2
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_IPADDR2_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Ipaddr2 = msg_ctrl_inet_aton(value, NULL);
	else
		strm_req->Ipaddr2 = 0;

	//Port2
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_PORT2_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Port2 = atoi(value);
	else
		strm_req->Port2 = 0;

	//Ipaddr3
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_IPADDR3_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Ipaddr3 = msg_ctrl_inet_aton(value, NULL);
	else
		strm_req->Ipaddr3 = 0;

	//Port3
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_PORT3_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Port3 = atoi(value);
	else
		strm_req->Port3 = 0;

	//Ipaddr4
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_IPADDR4_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Ipaddr4 = msg_ctrl_inet_aton(value, NULL);
	else
		strm_req->Ipaddr4 = 0;

	//Port4
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_PORT4_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Port4 = atoi(value);
	else
		strm_req->Port4 = 0;

	//Ipaddr5
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_IPADDR5_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Ipaddr5 = msg_ctrl_inet_aton(value, NULL);
	else
		strm_req->Ipaddr5 = 0;

	//Port5
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_PORT5_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Port5 = atoi(value);
	else
		strm_req->Port5 = 0;

	//Ipaddr6
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_IPADDR6_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Ipaddr6 = msg_ctrl_inet_aton(value, NULL);
	else
		strm_req->Ipaddr6 = 0;

	//Port6
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_PORT6_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Port6 = atoi(value);
	else
		strm_req->Port6 = 0;

	//Ipaddr7
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_IPADDR7_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Ipaddr7 = msg_ctrl_inet_aton(value, NULL);
	else
		strm_req->Ipaddr7 = 0;

	//Port7
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_PORT7_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Port7 = atoi(value);
	else
		strm_req->Port7 = 0;

	//Ipaddr8
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_IPADDR8_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Ipaddr8 = msg_ctrl_inet_aton(value, NULL);
	else
		strm_req->Ipaddr8 = 0;

	//Port8
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_PORT8_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Port8 = atoi(value);
	else
		strm_req->Port8 = 0;

	//Ipaddr9
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_IPADDR9_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Ipaddr9 = msg_ctrl_inet_aton(value, NULL);
	else
		strm_req->Ipaddr9 = 0;

	//Port9
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_PORT9_KEY, cur_node, pdoc);
	if (ret == ROOM_RETURN_SUCC)
		strm_req->Port9 = atoi(value);
	else
		strm_req->Port9 = 0;

	return ROOM_RETURN_SUCC;

}

int32_t upper_msg_req_stream(RoomMsgEnv *pRoom, int32_t Quality, int32_t p_mask_index)
{
	int32_t i				= -1;
	int32_t socket 			= -1;
	int32_t stream_id		= -1;
	int32_t encode_type		= -1;
	int32_t send_num		= 0;
	int32_t req_num			= 0;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex 	= NULL;
	int32_t 	ret_val 	= ROOM_RETURN_SUCC;
	MsgHeader 	head;
	int8_t 		*xml_buf  	= NULL;
	int32_t 	req_mask	= 0;

	r_bzero(&head, sizeof(MsgHeader));

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ROOM_RETURN_FAIL;
	}

	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_req_stream_info(pRoom, xml_buf, "0", Quality);

	req_mask = pRoom->RoomInfo.req_stream_mask;

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		}

		e_info = &pRoom->RoomInfo.EncInfo[i];
		e_info->SD_QuaInfo.RateType = MSG_RATETYPE_SD;
		e_info->HD_QuaInfo.RateType = MSG_RATETYPE_HD;

		socket		= -1;
		encode_type = e_info->EncodeType;

		//HD
		if (Quality == MSG_RATETYPE_HD && encode_type == EncodeType_H264) {
			socket		= e_info->HD_QuaInfo.Socket;
			mutex		= e_info->HD_QuaInfo.mutex;
			stream_id	= e_info->HD_QuaInfo.MsgQueType;
			e_info->HD_QuaInfo.RateType = MSG_RATETYPE_HD;
		}

		//SD
		if (Quality == MSG_RATETYPE_SD && encode_type == EncodeType_H264) {
			socket		= e_info->SD_QuaInfo.Socket;
			mutex		= e_info->SD_QuaInfo.mutex;
			stream_id	= e_info->SD_QuaInfo.MsgQueType;
			e_info->SD_QuaInfo.RateType = MSG_RATETYPE_SD;
		}

		//JPEG
		if (encode_type == EncodeType_JPEG) {
			socket		= e_info->HD_QuaInfo.Socket;
			mutex		= e_info->HD_QuaInfo.mutex;
			stream_id	= e_info->HD_QuaInfo.MsgQueType;
			e_info->HD_QuaInfo.RateType = MSG_RATETYPE_HD;
		}

		req_num++;
		pRoom->RoomInfo.req_stream_mask 	|= (1 << stream_id);

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, &head, xml_buf);
			pRoom->StrmReqMask[p_mask_index] 		= MSG_QUEUE_REC_ALL;
			send_num++;
		}

		nslog(NS_INFO, "--- req_stream_mask = %d, stream_id = %d, EncNum = %d, i = %d, socket = %d, encode_type = %d", \
			pRoom->RoomInfo.req_stream_mask, stream_id, pRoom->RoomInfo.EncNum, i, socket, encode_type);
	}

	if (xml_buf)
		r_free(xml_buf);

	nslog(NS_INFO, "req_num = %d, send_num = %d", req_num, send_num);

	if (/*req_num == send_num && */send_num != 0) {
		ret_val = ROOM_RETURN_SUCC;
	} else {
		pRoom->RoomInfo.req_stream_mask = req_mask;
		ret_val = ROOM_RETURN_FAIL;
	}

	return ret_val;
}

int32_t upper_msg_resp_stream_req(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *ret_code, int8_t *usr_id, int8_t *EncodeIndex)
{
	int8_t *xml_buf = NULL;
	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	tmp_node = NULL;

	if (!pRoom || !parseXml || !head || !usr_id || !ret_code || !EncodeIndex) {
		nslog(NS_ERROR, " ---[param] error!");
		return ROOM_RETURN_FAIL;
	}

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ROOM_RETURN_FAIL;
	}

	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_stream_reponse_xml(xml_buf, MSG_REQUEST_CODE_STREAM, ret_code, ret_pass_key, usr_id, EncodeIndex, pRoom->RoomInfo.RoomId, &pRoom->StrmReq);
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

	if (xml_buf)
		r_free(xml_buf);

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_get_room_info_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t 	i		= 0;
	int32_t 	index   = -1;
	int32_t 	ret_val = ROOM_RETURN_FAIL;
	xmlNodePtr 	pNode 	= NULL;
	int8_t 		pass_key[ROOM_MSG_VAL_LEN] = {0};

	//检测是否包含本会议室ROOM ID
	if (!upper_msg_check_room_info_req_id(parseXml, pRoom->RoomInfo.RoomId)) {
		nslog(NS_WARN, " --- room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_WARN, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	//向各 路流发送编码器请求信息
	pRoom->roomInfoMask[index] = MSG_QUEUE_REC_CLR;	//清空

	int32_t socket 			= -1;
	int32_t stream_id		= -1;
	pthread_mutex_t *mutex 	= NULL;
	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		}

		if (pRoom->RoomInfo.EncInfo[i].EncodeType == EncodeType_JPEG) {
			//JPEG
			mutex		= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.mutex;
			socket		= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.Socket;
			stream_id	= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType;
			if (socket > 0) {
				ret_val = upper_msg_tcp_send(socket, mutex, head, xml);
				if (ROOM_RETURN_SUCC == ret_val) {
					pRoom->roomInfoMask[index] |= (1 << stream_id);
				}
			}

			nslog(NS_INFO, " ---EncNum = %d, i = %d, JPEG, socket = %d, stream_id = %d, roomInfoMask = %d", pRoom->RoomInfo.EncNum, i, socket, stream_id, pRoom->roomInfoMask[index]);
		} else if (pRoom->RoomInfo.EncInfo[i].EncodeType == EncodeType_H264) {
			//H264_HD
			mutex 		= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.mutex;
			socket 		= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.Socket;
			stream_id 	= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType;
			if (socket > 0) {
				ret_val = upper_msg_tcp_send(socket, mutex, head, xml);
				if (ROOM_RETURN_SUCC == ret_val) {
					pRoom->roomInfoMask[index] |= (1 << stream_id);
				}
			}
			nslog(NS_INFO, " ---EncNum = %d, i = %d, H264_HD, socket = %d, stream_id = %d, roomInfoMask = %d", pRoom->RoomInfo.EncNum, i, socket, stream_id, pRoom->roomInfoMask[index]);

			//H264_SD
			mutex		= pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.mutex;
			socket		= pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.Socket;
			stream_id	= pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType;
			if (socket > 0) {
				ret_val = upper_msg_tcp_send(socket, mutex, head, xml);
				if (ROOM_RETURN_SUCC == ret_val) {
					pRoom->roomInfoMask[index] |= (1 << stream_id);
				}
			}
			nslog(NS_INFO, " ---EncNum = %d, i = %d, H264_SD, socket = %d, stream_id = %d, roomInfoMask = %d", pRoom->RoomInfo.EncNum, i, socket, stream_id, pRoom->roomInfoMask[index]);
		}
	}

	//--fix me...........更新录制状态和录制名称
	if (pRoom->record_handle) {
		pRoom->RoomInfo.RecordStatus = lower_msg_opt_type_trans(pRoom->record_handle->get_record_status(pRoom->record_handle));
		r_strcpy(pRoom->RoomInfo.RcdName, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
	} else {
		pRoom->RoomInfo.RecordStatus = 0;
		r_strcpy(pRoom->RoomInfo.RcdName, pRoom->RecInfo.CourseName);
	}

	//获取pass key
	get_req_pass_key_node(&pNode, parseXml->proot);
	if (!pNode) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}
	r_bzero(pass_key, ROOM_MSG_VAL_LEN);
	get_current_node_value(pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode);

	//取本地信息直接回应上报
	int8_t *xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ROOM_RETURN_FAIL;
	}

	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);

	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_resp_room_info(pRoom, xml_buf, MSG_GET_ROOM_INFO, ROOM_RETURN_SUCC, pass_key, user_id);
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

	if (xml_buf)
		r_free(xml_buf);

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_set_room_info_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	xmlNodePtr 	pNode 			= NULL;
	int32_t 	ret_val			= ROOM_RETURN_FAIL;
	int32_t 	ret 			= ROOM_RETURN_FAIL;
	int32_t		return_code_val = 0;
	int8_t 		ret_pass_key[ROOM_MSG_VAL_LEN] 		= {0};
	int8_t 		msg_return_code[ROOM_MSG_VAL_LEN] 	= {0};
	xmlNodePtr 	tmp_node 		= NULL;
	int8_t 		*xml_buf		= NULL;
	int8_t 		value[ROOM_MSG_VAL_LEN] = {0};

	if (pRoom->record_handle) {
		nslog(NS_WARN, " --- RecInfo.OptType = %d", pRoom->RecInfo.OptType);
		return_code_val = -1;
		goto exit;
	}

	pNode = upper_msg_get_room_info_node(parseXml, pRoom->RoomInfo.RoomId);
	if (NULL == pNode) {
		nslog(NS_WARN, " --- room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	//录制名称
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_ROOMNAME_KEY, pNode, parseXml->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		r_strcpy(pRoom->RoomInfo.RcdName, value);
	else
		r_bzero(pRoom->RoomInfo.RcdName, ROOM_MSG_VAL_LEN);

	//最大录制时间
	r_bzero(value, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_RECMAXTIME_REP_KEY, pNode, parseXml->pdoc);
	if (ret == ROOM_RETURN_SUCC)
		pRoom->RoomInfo.RecordMaxTime = atoi(value);
	else
		pRoom->RoomInfo.RecordMaxTime = 8;

	//设置的IP 不变，则直接回应
	ret = upper_msg_cmp_room_info(pNode, parseXml->pdoc, pRoom);
	if (ret == ROOM_RETURN_SUCC) {
		return_code_val = ROOM_RETURN_SUCC;
		goto exit;
	}

	//初始化ENC ID 为0
	upper_msg_init_enc_id(pRoom);

	ret_val = upper_msg_save_set_room_info(pNode, parseXml->pdoc, pRoom);
	if (ROOM_RETURN_FAIL == ret_val) {
		nslog(NS_ERROR, " --- room info error!");
		return_code_val = -1;
		goto exit;
	}

	//重设直播模块状态
	{
		video_sindex_info_t enc_info;

		r_bzero(&enc_info, sizeof(video_sindex_info_t));

		if (pRoom->live_handle)
			pRoom->live_handle->set_lives_enc_info(&enc_info, pRoom->live_handle);

		lower_msg_print_live_param(&enc_info);
	}

	//设置教室信息
	int32_t 	i 		= 0, j = 0, num = 0;
	int32_t		retVal 	= 0;
	recv_param rec_param[ROOM_STR_NUM];

	pRoom->loginMask = MSG_QUEUE_REC_CLR;
	pRoom->loginMask |= MSG_QUEUE_REC_FLG;

	r_bzero(rec_param, sizeof(rec_param));
	for (i = 0, j = 0, num = 0; i < ROOM_ENC_NUM; i++) {
//		printf("pRoom->RoomInfo.EncInfo[i].ID = %d\n", pRoom->RoomInfo.EncInfo[i].ID);

		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			j += 2;
			continue;      // zl question !!! 编码未配置 教育套包中不会有此现象!!!
		}

		rec_param[j].status 	= 1;
		rec_param[j].enc_id = pRoom->RoomInfo.EncInfo[i].ID	  ; // add zl
		rec_param[j].stream_id 	= j + 1;
		msg_ctrl_inet_ntoa(rec_param[j].ipaddr, pRoom->RoomInfo.EncInfo[i].EncIP);
		pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType = rec_param[j].stream_id;
		pRoom->loginMask |= (1 << rec_param[j].stream_id);
		nslog(NS_INFO, " ---HD_QuaInfo- EncNum = %d, iEnc = %d, jRec = %d, port = %d, stream_id = %d, ipaddr = %s, loginMask = %d   <status : %d>", pRoom->RoomInfo.EncNum, i, j, rec_param[j].enc_id, rec_param[j].stream_id, rec_param[j].ipaddr, pRoom->loginMask,rec_param[j].status);
		j++;
		num++;

		if(pRoom->RoomInfo.EncInfo[i].ID != 1)    // add zl
		{
			rec_param[j].status 	= 2;
		}
		else
		{
			rec_param[j].status 	= 1;
		}

		rec_param[j].enc_id = pRoom->RoomInfo.EncInfo[i].ID	   ;// add zl
		rec_param[j].stream_id 	= j + 1;
		msg_ctrl_inet_ntoa(rec_param[j].ipaddr, pRoom->RoomInfo.EncInfo[i].EncIP);
		pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType = rec_param[j].stream_id;
		pRoom->loginMask |= (1 << rec_param[j].stream_id);
		nslog(NS_INFO, " ---SD_QuaInfo- EncNum = %d, iEnc = %d, jRec = %d, port = %d, stream_id = %d, ipaddr = %s, loginMask = %d <status : %d>", pRoom->RoomInfo.EncNum, i, j, rec_param[j].enc_id, rec_param[j].stream_id, rec_param[j].ipaddr, pRoom->loginMask,rec_param[j].status);
		j++;
		num++;
	}

	nslog(NS_INFO, "i = %d, j = %d, num = %d, EncNum = %d", i, j, num, pRoom->RoomInfo.EncNum);

	retVal = pRoom->handle->set_room_info(rec_param, pRoom->handle, ROOM_STR_NUM);
	if (return_code_val == 0) {
		return_code_val = retVal;
	}

	if (retVal == 0) {
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
			pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.RateType = MSG_RATETYPE_HD;
			pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.RateType = MSG_RATETYPE_SD;
		}
	}


exit:

	//回应
	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ROOM_RETURN_FAIL;
	}

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	if (return_code_val == -1)
		return_code_val = 0;
	else
		return_code_val = 1;

	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);

	sprintf(msg_return_code, "%d", return_code_val);
	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_reponse_xml(xml_buf, MSG_SET_ROOM_INFO, msg_return_code, ret_pass_key, user_id, pRoom->RoomInfo.RoomId);
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml);

	if (xml_buf)
		r_free(xml_buf);

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_set_quality_info_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t    	room_id  	= 0;
	int32_t    	ret_val		= ROOM_RETURN_FAIL;
	int8_t 		msg_pass[ROOM_MSG_VAL_LEN]	= {0};
	int8_t 		value[ROOM_MSG_VAL_LEN]		= {0};
	int8_t 		user_id[ROOM_MSG_VAL_LEN] 	= {0};
	int8_t 		*msg_xml 	= NULL;

	xmlNodePtr 	pNode		= NULL;
	xmlNodePtr 	pNode_tmp	= NULL;
	enc_info 	e_info;

	//获取本会议室node
	pNode = upper_msg_get_room_info_node(parseXml, pRoom->RoomInfo.RoomId);
	if (!pNode) {
		nslog(NS_WARN, " --- room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	//获取pass key
	pNode_tmp = get_req_pass_key_node(&pNode_tmp, parseXml->proot);
	get_current_node_value(value, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode_tmp);
	r_strcpy(msg_pass, value);

	//user_id
	upper_msg_get_user_id_value(parseXml, user_id);

	//获取room id
	room_id = pRoom->RoomInfo.RoomId;

	//设置mask ，用于接收端线程等待所有ENC 回应后才向上回应
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_WARN, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	nslog(NS_INFO, " --- msg_code = %d, msg_pass = %s, room_id = %d, index = %d", MSG_SET_QUALITY_INFO, msg_pass, room_id, index);

//	pRoom->qualityMask[index] = MSG_QUEUE_REC_CLR;
//	pRoom->qualityMask[index] |= MSG_QUEUE_REC_FLG;	//初始化为返回成功
	pRoom->qualityMask[index] |= MSG_QUEUE_REC_ALL;

	msg_xml = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!msg_xml) {
		nslog(NS_ERROR, " --- r_malloc error!");
		return ROOM_RETURN_FAIL;
	}

	//录制中直接返回失败
	if (pRoom->record_handle) {
		nslog(NS_WARN, " --- RecInfo.OptType = %d", pRoom->RecInfo.OptType);

		goto exit;
	}

	int32_t i 				= 0;
	int32_t enc_id 			= 0;
	int32_t socket_HD 		= -1;
	int32_t socket_SD		= -1;
	int32_t socket			= -1;
	int32_t stream_id		= -1;
	pthread_mutex_t *mutex 	= NULL;
	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		}

		enc_id		= pRoom->RoomInfo.EncInfo[i].ID;

		//解析
		r_bzero(msg_xml, ROOM_MSG_MAX_LEN);
		r_bzero(&e_info, sizeof(enc_info));
		ret_val = upper_msg_analyze_quality_info(pNode, parseXml->pdoc, &e_info, enc_id);
		if (ROOM_RETURN_FAIL == ret_val) {
			nslog(NS_ERROR, " --- analyze quality xml error, EncNum = %d, enc_id = %d, stream_id = %d", pRoom->RoomInfo.EncNum, enc_id, stream_id);
			continue;
		}

		//保存
		upper_msg_save_quality_info(pRoom, &e_info, enc_id);

		socket_HD 	= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.Socket;
		socket_SD 	= pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.Socket;
		if (socket_HD > 0)	{
			socket		= socket_HD;
			mutex 		= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.mutex;
			stream_id 	= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType;
		} else if (socket_SD > 0)	{
			socket		= socket_SD;
			mutex 		= pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.mutex;
			stream_id 	= pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType;
		}  else {
			socket = -1;
			continue;
		}

		//封包
		ret_val = upper_msg_package_quality_info(&e_info, msg_xml, enc_id, room_id, MSG_SET_QUALITY_INFO, msg_pass, user_id);
		if (ROOM_RETURN_FAIL == ret_val) {
			nslog(NS_ERROR, " --- package quality xml error, EncNum = %d, enc_id = %d, stream_id = %d", pRoom->RoomInfo.EncNum, enc_id, stream_id);
			continue;
		}

		//发送
		if (socket > 0) {
			ret_val = upper_msg_tcp_send(socket, mutex, head, msg_xml);
			pRoom->qualityMask[index] |= (1 << stream_id);
		}

		nslog(NS_INFO, " --- set quality info, EncNum = %d, enc_id = %d, stream_id = %d, socket = %d, qualityMask[%d] = %d", pRoom->RoomInfo.EncNum, enc_id, stream_id, socket, index, pRoom->qualityMask[index]);
	}

exit:

#if 1
	//回应
	r_bzero(msg_xml, ROOM_MSG_MAX_LEN);
	upper_msg_package_reponse_xml(msg_xml, MSG_SET_QUALITY_INFO, "1", msg_pass, user_id, room_id);
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, msg_xml);
#else
	if (pRoom->qualityMask[index] == MSG_QUEUE_REC_FLG) {
		//发送失败，直接回应
		r_bzero(msg_xml, ROOM_MSG_MAX_LEN);
		upper_msg_package_reponse_xml(msg_xml, MSG_SET_QUALITY_INFO, "0", msg_pass, user_id, room_id);
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, msg_xml);
		nslog(NS_INFO, " --- resp faild xml, EncNum = %d, qualityMask[%d] = %d", pRoom->RoomInfo.EncNum, index, pRoom->qualityMask[index]);
	} else {
		//添加到超时队列
		upper_msg_time_que_add_node(pRoom, MSG_SET_QUALITY_INFO, msg_pass, user_id, head, upper_msg_time_out_resp);
	}
#endif

	if (msg_xml)
		r_free(msg_xml);

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_set_audio_info_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t i 				= 0;
	int32_t socket_HD		= -1;
	int32_t socket_SD		= -1;
	int32_t stream_id		= -1;
	pthread_mutex_t *mutex 	= NULL;
	int32_t    	room_id  	= 0;
	int32_t 	index		= 0;
	int32_t 	ret_val		= 0;
	int8_t 		*msg_xml 	= NULL;
	xmlNodePtr 	pNode 		= NULL;
	audio_info  a_info;
	xmlNodePtr 	pNode_tmp	= NULL;
	int8_t 		msg_pass[ROOM_MSG_VAL_LEN]	= {0};
	int8_t 		user_id[ROOM_MSG_VAL_LEN] 	= {0};

	index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_WARN, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	pNode = upper_msg_get_audio_info_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " --- room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	msg_xml = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!msg_xml) {
		nslog(NS_ERROR, " --- r_malloc error!");
		return ROOM_RETURN_FAIL;
	}

	//获取pass key
	pNode_tmp = get_req_pass_key_node(&pNode_tmp, parseXml->proot);
	get_current_node_value(msg_pass, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode_tmp);

	upper_msg_get_user_id_value(parseXml, user_id);

	//获取room id
	room_id = pRoom->RoomInfo.RoomId;

	nslog(NS_INFO, " --- pass_key = %s, room_id = %d, index = %d", msg_pass, room_id, index);

	//音频只往第一路发送
	pRoom->audioMask[index] = MSG_QUEUE_REC_CLR;
	pRoom->audioMask[index] |= MSG_QUEUE_REC_FLG;

	//录制中, 返回失败
	if (pRoom->record_handle) {
		nslog(NS_INFO, " --- RecInfo.OptType = %d", pRoom->RecInfo.OptType);
		goto exit;
	}

	//解析
	ret_val = upper_msg_analyze_audio_info(pNode, parseXml->pdoc, &a_info);
	if (ROOM_RETURN_FAIL == ret_val) {
		nslog(NS_ERROR, " --- analyze audio xml error");
		goto exit;
	}

	//封包
	r_bzero(msg_xml, ROOM_MSG_MAX_LEN);
	ret_val = upper_msg_package_audio_info(&a_info, msg_xml, room_id, MSG_SET_AUDIO_INFO, msg_pass, user_id);
	if (ROOM_RETURN_FAIL == ret_val) {
		nslog(NS_ERROR, " --- package audio xml error");
		goto exit;
	}

	// 音频只发送ENC1200 高清版	第四个编码器!!! add zl
	socket_HD	= pRoom->RoomInfo.EncInfo[3].HD_QuaInfo.Socket;
	if(socket_HD >0)
	{
		mutex		= pRoom->RoomInfo.EncInfo[3].HD_QuaInfo.mutex;
		stream_id	= pRoom->RoomInfo.EncInfo[3].HD_QuaInfo.MsgQueType;
		pRoom->audioMask[index] |= (1 << stream_id);
		upper_msg_tcp_send(socket_HD, mutex, head, msg_xml);
	}
	else
	{
		goto exit;
	}

	#if 0
	//音频相关只往第一路发
	socket_HD	= pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.Socket;
	socket_SD	= pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.Socket;
	if (socket_HD > 0) {
		mutex		= pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.mutex;
		stream_id	= pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.MsgQueType;
		pRoom->audioMask[index] |= (1 << stream_id);
		upper_msg_tcp_send(socket_HD, mutex, head, msg_xml);
	} else if (socket_SD > 0) {
		mutex		= pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.mutex;
		stream_id	= pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.MsgQueType;
		pRoom->audioMask[index] |= (1 << stream_id);
		upper_msg_tcp_send(socket_SD, mutex, head, msg_xml);
	} else {
		goto exit;
	}
	#endif

exit:

	if (pRoom->audioMask[index] == MSG_QUEUE_REC_FLG) {
		//发送失败，直接回应
		r_bzero(msg_xml, ROOM_MSG_MAX_LEN);
		upper_msg_package_reponse_xml(msg_xml, MSG_SET_AUDIO_INFO, "0", msg_pass, user_id, room_id);
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, msg_xml);
	} else {
		//添加到超时队列
		upper_msg_time_que_add_node(pRoom, MSG_SET_AUDIO_INFO, msg_pass, user_id, head, upper_msg_time_out_resp);
	}

	if (msg_xml)
		r_free(msg_xml);

	return ROOM_RETURN_SUCC;
}

// --------------------- add zl

xmlNodePtr upper_msg_get_getcamctl_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t		ret   		= 0;
	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}
	tmp_node = get_children_node(tmp_node, MSG_REMOTEGET_KEY);
	if (!tmp_node) {
		return ret_val;
	}
	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}
		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}
	return ret_val;
}

int32_t upper_msg_get_camctl_ctrl_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	xmlNodePtr	pNode	= NULL;
	int8_t 		EncodeIndex[ROOM_MSG_VAL_LEN] = {0};
	int32_t 	ret 	= 0;

	pNode = upper_msg_get_getcamctl_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}
	r_bzero(EncodeIndex, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(EncodeIndex, ROOM_MSG_VAL_LEN, MSG_ENCODEINDEX_KEY, pNode, parseXml->pdoc);
	if (ret == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, "get EncodeIndex error");
		return ROOM_RETURN_FAIL;
	}
	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}
	pRoom->setRemoteCtrlMask[index] = MSG_QUEUE_REC_CLR;
	pRoom->setRemoteCtrlMask[index] |= MSG_QUEUE_REC_FLG;	//初始化为返回成功

	int32_t i				= -1;
	int32_t socket			= -1;
	int32_t stream_id		= -1;
	int32_t encode_type 	= -1;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex	= NULL;
	int32_t enc_id[ROOM_ENC_NUM] = {0};

	if (strchr(EncodeIndex, '1')) {
		enc_id[0] = 1;
	}

	if (strchr(EncodeIndex, '2')) {
		enc_id[1] = 2;
	}

	if (strchr(EncodeIndex, '3')) {
		enc_id[2] = 3;
	}

	if (strchr(EncodeIndex, '4')) {
		enc_id[3] = 4;
	}

	if (strchr(EncodeIndex, '5')) {
		enc_id[4] = 5;
	}

	if (strchr(EncodeIndex, '6')) {
		enc_id[5] = 6;
	}
	// add zl
	if (strchr(EncodeIndex, '7')) {
		enc_id[6] = 7;
	}

	if (strchr(EncodeIndex, '8')) {
		enc_id[7] = 8;
	}

	if (strchr(EncodeIndex, '9')) {
		enc_id[8] = 9;
	}

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0 || enc_id[i] == 0) {
			continue;
		}

		e_info = upper_msg_get_enc_info_by_id(pRoom, enc_id[i]);
		if (!e_info) {
			nslog(NS_WARN, "enc:enc_id = %d not found, continue for next", enc_id[i]);
			continue;
		}

		//HD
		socket		= e_info->HD_QuaInfo.Socket;
		mutex		= e_info->HD_QuaInfo.mutex;
		stream_id	= e_info->HD_QuaInfo.MsgQueType;
		pRoom->adjustMask[index] |= (1 << stream_id);

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, head, xml);
		}

		nslog(NS_INFO, " --- adjust--- EncodeIndex = %s, i = %d, EncNum = %d, socket = %d, stream_id = %d, adjustMask[%d] = %d", EncodeIndex, i, pRoom->RoomInfo.EncNum, socket, stream_id, index, pRoom->adjustMask[index]);
	}
	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	tmp_node = NULL;

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);
	//添加到超时队列
	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);
	upper_msg_time_que_add_node(pRoom, MSGCODE_GET_CAMCTL_PROLIST, ret_pass_key, user_id, head, upper_msg_time_out_resp);
	return ROOM_RETURN_SUCC;

}

xmlNodePtr upper_msg_get_setcamctl_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t		ret   		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_REMOTESET_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}


int32_t upper_msg_set_camctl_ctrl_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{

	xmlNodePtr	pNode	= NULL;
	int8_t 		EncodeIndex[ROOM_MSG_VAL_LEN] = {0};
	int32_t 	ret 	= 0;

	pNode = upper_msg_get_setcamctl_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	r_bzero(EncodeIndex, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(EncodeIndex, ROOM_MSG_VAL_LEN, MSG_ENCODEINDEX_KEY, pNode, parseXml->pdoc);
	if (ret == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, "get EncodeIndex error");
		return ROOM_RETURN_FAIL;
	}

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	pRoom->getRemoteCtrlMask[index] = MSG_QUEUE_REC_CLR;
	pRoom->getRemoteCtrlMask[index] |= MSG_QUEUE_REC_FLG;	//初始化为返回成功

	int32_t i				= -1;
	int32_t socket			= -1;
	int32_t stream_id		= -1;
	int32_t encode_type 	= -1;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex	= NULL;
	int32_t enc_id[ROOM_ENC_NUM] = {0};

	if (strchr(EncodeIndex, '1')) {
		enc_id[0] = 1;
	}

	if (strchr(EncodeIndex, '2')) {
		enc_id[1] = 2;
	}

	if (strchr(EncodeIndex, '3')) {
		enc_id[2] = 3;
	}

	if (strchr(EncodeIndex, '4')) {
		enc_id[3] = 4;
	}

	if (strchr(EncodeIndex, '5')) {
		enc_id[4] = 5;
	}

	if (strchr(EncodeIndex, '6')) {
		enc_id[5] = 6;
	}
	// add zl
	if (strchr(EncodeIndex, '7')) {
		enc_id[6] = 7;
	}

	if (strchr(EncodeIndex, '8')) {
		enc_id[7] = 8;
	}

	if (strchr(EncodeIndex, '9')) {
		enc_id[8] = 9;
	}


	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0 || enc_id[i] == 0) {
			continue;
		}

		e_info = upper_msg_get_enc_info_by_id(pRoom, enc_id[i]);
		if (!e_info) {
			nslog(NS_WARN, "enc:enc_id = %d not found, continue for next", enc_id[i]);
			continue;
		}

		//HD
		socket		= e_info->HD_QuaInfo.Socket;
		mutex		= e_info->HD_QuaInfo.mutex;
		stream_id	= e_info->HD_QuaInfo.MsgQueType;
		pRoom->adjustMask[index] |= (1 << stream_id);

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, head, xml);
		}

		nslog(NS_INFO, " --- adjust--- EncodeIndex = %s, i = %d, EncNum = %d, socket = %d, stream_id = %d, adjustMask[%d] = %d", EncodeIndex, i, pRoom->RoomInfo.EncNum, socket, stream_id, index, pRoom->adjustMask[index]);
	}

	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	tmp_node = NULL;

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	//添加到超时队列
	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);
	upper_msg_time_que_add_node(pRoom, MSGCODE_SET_CAMCTL_PRO, ret_pass_key, user_id, head, upper_msg_time_out_resp);

	return ROOM_RETURN_SUCC;

}

xmlNodePtr upper_msg_get_image_synthesis_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t		ret   		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_PICTURESYN_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}


int32_t upper_msg_image_synthesis_req_node(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	xmlNodePtr	pNode	= NULL;
	int8_t 		EncodeIndex[ROOM_MSG_VAL_LEN] = {0};
	int32_t 	ret 	= 0;

	pNode = upper_msg_get_image_synthesis_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	r_bzero(EncodeIndex, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(EncodeIndex, ROOM_MSG_VAL_LEN, MSG_ENCODEINDEX_KEY, pNode, parseXml->pdoc);
	if (ret == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, "get EncodeIndex error");
		return ROOM_RETURN_FAIL;
	}

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	//录制中直接返回失败
	if (pRoom->record_handle) {
		nslog(NS_WARN, " --- RecInfo.OptType = %d", pRoom->RecInfo.OptType);
		return ROOM_RETURN_FAIL;
	}

	pRoom->getPictureSynMask[index] = MSG_QUEUE_REC_CLR;
	pRoom->getPictureSynMask[index] |= MSG_QUEUE_REC_FLG;	//初始化为返回成功

	int32_t i				= -1;
	int32_t socket			= -1;
	int32_t stream_id		= -1;
	int32_t encode_type 	= -1;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex	= NULL;
	int32_t enc_id[ROOM_ENC_NUM] = {0};

	if (strchr(EncodeIndex, '1')) {
		enc_id[0] = 1;
	}

	if (strchr(EncodeIndex, '2')) {
		enc_id[1] = 2;
	}

	if (strchr(EncodeIndex, '3')) {
		enc_id[2] = 3;
	}

	if (strchr(EncodeIndex, '4')) {
		enc_id[3] = 4;
	}

	if (strchr(EncodeIndex, '5')) {
		enc_id[4] = 5;
	}

	if (strchr(EncodeIndex, '6')) {
		enc_id[5] = 6;
	}
	// add zl
	if (strchr(EncodeIndex, '7')) {
		enc_id[6] = 7;
	}

	if (strchr(EncodeIndex, '8')) {
		enc_id[7] = 8;
	}

	if (strchr(EncodeIndex, '9')) {
		enc_id[8] = 9;
	}


	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0 || enc_id[i] == 0) {
			continue;
		}

		e_info = upper_msg_get_enc_info_by_id(pRoom, enc_id[i]);
		if (!e_info) {
			nslog(NS_WARN, "enc:enc_id = %d not found, continue for next", enc_id[i]);
			continue;
		}

		//HD
		socket		= e_info->HD_QuaInfo.Socket;
		mutex		= e_info->HD_QuaInfo.mutex;
		stream_id	= e_info->HD_QuaInfo.MsgQueType;
		pRoom->adjustMask[index] |= (1 << stream_id);

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, head, xml);
		}

		nslog(NS_INFO, " --- adjust--- EncodeIndex = %s, i = %d, EncNum = %d, socket = %d, stream_id = %d, adjustMask[%d] = %d", EncodeIndex, i, pRoom->RoomInfo.EncNum, socket, stream_id, index, pRoom->adjustMask[index]);
	}

	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	tmp_node = NULL;

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	//添加到超时队列
	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);
	upper_msg_time_que_add_node(pRoom, MSG_ENC_IMAGE_SYNTHESIS, ret_pass_key, user_id, head, upper_msg_time_out_resp);

	return ROOM_RETURN_SUCC;

}

xmlNodePtr upper_msg_get_director_mode_req_node(parse_xml_t *parseXml, uint32_t roomID)
{
	int8_t		tmp_buf[ROOM_MSG_VAL_LEN]	= {0};
	xmlNodePtr	tmp_node	= NULL;
	xmlNodePtr	ret_val 	= NULL;
	int32_t		ret   		= 0;

	tmp_node = get_children_node(parseXml->proot, MSG_BODY_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	tmp_node = get_children_node(tmp_node, MSG_DIRECTORMODE_KEY);
	if (!tmp_node) {
		return ret_val;
	}

	for (;tmp_node != NULL;) {
		ret = upper_msg_get_leaf_value(tmp_buf, ROOM_MSG_VAL_LEN, MSG_ROOMID_KEY, tmp_node, parseXml->pdoc);
		if (ret == ROOM_RETURN_SUCC)
			if (atoi(tmp_buf) == roomID) {
				ret_val = tmp_node;
				break;
			}

		tmp_node = upper_msg_get_next_samename_node(tmp_node);
	}

	return ret_val;
}


int32_t upper_msg_director_mode_ctrl_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{

	xmlNodePtr	pNode	= NULL;
	int8_t 		EncodeIndex[ROOM_MSG_VAL_LEN] = {0};
	int32_t 	ret 	= 0;

	pNode = upper_msg_get_director_mode_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	r_bzero(EncodeIndex, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(EncodeIndex, ROOM_MSG_VAL_LEN, MSG_ENCODEINDEX_KEY, pNode, parseXml->pdoc);
	if (ret == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, "get EncodeIndex error");
		return ROOM_RETURN_FAIL;
	}

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	pRoom->setRemoteModeMask[index] = MSG_QUEUE_REC_CLR;
	pRoom->setRemoteModeMask[index] |= MSG_QUEUE_REC_FLG;	//初始化为返回成功

	int32_t i				= -1;
	int32_t socket			= -1;
	int32_t stream_id		= -1;
	int32_t encode_type 	= -1;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex	= NULL;
	int32_t enc_id[ROOM_ENC_NUM] = {0};

	if (strchr(EncodeIndex, '1')) {
		enc_id[0] = 1;
	}

	if (strchr(EncodeIndex, '2')) {
		enc_id[1] = 2;
	}

	if (strchr(EncodeIndex, '3')) {
		enc_id[2] = 3;
	}

	if (strchr(EncodeIndex, '4')) {
		enc_id[3] = 4;
	}

	if (strchr(EncodeIndex, '5')) {
		enc_id[4] = 5;
	}

	if (strchr(EncodeIndex, '6')) {
		enc_id[5] = 6;
	}
	// add zl
	if (strchr(EncodeIndex, '7')) {
		enc_id[6] = 7;
	}

	if (strchr(EncodeIndex, '8')) {
		enc_id[7] = 8;
	}

	if (strchr(EncodeIndex, '9')) {
		enc_id[8] = 9;
	}


	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0 || enc_id[i] == 0) {
			continue;
		}

		e_info = upper_msg_get_enc_info_by_id(pRoom, enc_id[i]);
		if (!e_info) {
			nslog(NS_WARN, "enc:enc_id = %d not found, continue for next", enc_id[i]);
			continue;
		}

		//HD
		socket		= e_info->HD_QuaInfo.Socket;
		mutex		= e_info->HD_QuaInfo.mutex;
		stream_id	= e_info->HD_QuaInfo.MsgQueType;
		pRoom->adjustMask[index] |= (1 << stream_id);

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, head, xml);
		}

		nslog(NS_INFO, " --- adjust--- EncodeIndex = %s, i = %d, EncNum = %d, socket = %d, stream_id = %d, adjustMask[%d] = %d", EncodeIndex, i, pRoom->RoomInfo.EncNum, socket, stream_id, index, pRoom->adjustMask[index]);
	}

	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	tmp_node = NULL;

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	//添加到超时队列
	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);
	upper_msg_time_que_add_node(pRoom, MSG_ECN_DIRECTOR_MODE, ret_pass_key, user_id, head, upper_msg_time_out_resp);

	return ROOM_RETURN_SUCC;

}


int32_t upper_msg_record_ctrl_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t 	i 		= 0;
	int32_t 	wait_st = 0;
	int32_t 	req_num = 0;
	int32_t 	val_num	= 0;
	int32_t 	room_info_num	= 0;

	int8_t  	msg_pass[ROOM_MSG_VAL_LEN] 	= {0};
	int8_t	 	rec_file[ROOM_MSG_VAL_LEN]	= {0};
	int8_t 		user_id[ROOM_MSG_VAL_LEN]	= {0};
	int8_t 		rec_id[ROOM_MSG_VAL_LEN] 	= {0};
	int32_t 	rec_return_code = ROOM_RETURN_SUCC;
	int32_t 	ret_val 		= ROOM_RETURN_SUCC;
	xmlNodePtr 	pNode 			= NULL;
	xmlNodePtr 	pNode_tmp		= NULL;
	uint32_t 	req_mask, valid_mask, room_info_mask;

	//获取本会议室节点
	pNode = upper_msg_get_rec_ctrl_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " --- room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	upper_msg_get_user_id_value(parseXml, user_id);

	//获取pass key
	pNode_tmp = get_req_pass_key_node(&pNode_tmp, parseXml->proot);
	ret_val = get_current_node_value(msg_pass, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode_tmp);
	if (ret_val == -1) {
		nslog(NS_ERROR, " ---get pass key error!");
		return ROOM_RETURN_FAIL;
	}

	//解析录制信息并存储到pRoom->RecInfo
	upper_msg_analyze_rec_info(&pRoom->RecInfo, pNode, parseXml->pdoc);
	r_bzero(pRoom->RecInfo.userid, ROOM_MSG_VAL_LEN);
	r_strcpy(pRoom->RecInfo.userid, user_id);

	r_strcpy(rec_file, pRoom->RecInfo.CourseName);
	r_strcpy(rec_id, pRoom->RecInfo.RecordID);

	nslog(NS_INFO, " ---record-- OptType = %d, stream_num = %d, record_handle = %p", pRoom->RecInfo.OptType, pRoom->handle->stream_num, pRoom->record_handle);

	if (pRoom->RecInfo.OptType == MSG_RECORD_CTRL_START) {
		if (pRoom->record_handle) {
			pRoom->record_handle->record_resume(pRoom->record_handle);

			for (i = 0; i < pRoom->handle->stream_num; i++) {
				if (pRoom->RoomInfo.record_stream_mask & (1 << pRoom->handle->stream_hand[i].stream_id)) {
					pRoom->handle->set_rec_status(&pRoom->handle->stream_hand[i], START_REC);
				}
			}

			rec_return_code = ROOM_RETURN_SUCC;
		} else {
			//结束时间未到15 秒或磁盘空间少于5G，不启动录制
//			ret_val = upper_msg_monitor_time_out_status(&pRoom->RecInfo.RecordStopTime, RECORD_INT_TIME_VAL);
			if (/*ret_val != ROOM_RETURN_TIMEOUT ||*/\
				pRoom->server_info.DiskAvailableSpace < MSG_MIN_VALID_SPACE) {

				nslog(NS_INFO, " --- [space[5G] not enough] DiskAvailableSpace = [%dM] !", pRoom->server_info.DiskAvailableSpace);
				rec_return_code = ROOM_RETURN_FAIL;
			} else {
				if (pRoom->RoomInfo.ConnectStatus == 1)
				{
					nslog(NS_INFO, "---record-- req HD & SD stream");

					rec_return_code = ROOM_RETURN_FAIL;

					//请求高码流
					ret_val = upper_msg_req_stream(pRoom, MSG_RATETYPE_HD, RecServer);
					if (rec_return_code == ROOM_RETURN_FAIL) {
						rec_return_code = ret_val;
					}

					//请求低码流
					ret_val = upper_msg_req_stream(pRoom, MSG_RATETYPE_SD, RecServer);
					if (rec_return_code == ROOM_RETURN_FAIL) {
						rec_return_code = ret_val;
					}
				} else {
					nslog(NS_INFO, "---record-- req stream RoomInfo.ConnectStatus == 0");
					rec_return_code = ROOM_RETURN_FAIL;
				}
			}
		}
	} else if (pRoom->RecInfo.OptType == MSG_RECORD_CTRL_PAUSE) {
		if (pRoom->record_handle) {
			for (i = 0; i < pRoom->handle->stream_num; i++) {
				pRoom->handle->set_rec_status(&pRoom->handle->stream_hand[i], STOP_REC);
			}

			pRoom->record_handle->record_pause(pRoom->record_handle);

			rec_return_code = ROOM_RETURN_SUCC;
		} else {
			rec_return_code = ROOM_RETURN_FAIL;
		}
	} else if (pRoom->RecInfo.OptType == MSG_RECORD_CTRL_STOP) {
		if (pRoom->record_handle) {
			for (i = 0; i < pRoom->handle->stream_num; i++) {
				pRoom->handle->set_rec_status(&pRoom->handle->stream_hand[i], STOP_REC);
			}

			r_strcpy(rec_file, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
			r_strcpy(rec_id, pRoom->record_handle->get_RecordID(pRoom->record_handle));
			pRoom->record_handle->record_close(pRoom->record_handle);
			unregister_course_record_mode(&pRoom->record_handle);
			pRoom->record_handle = NULL;
			pRoom->RoomInfo.record_stream_mask = 0;

			//记录停止时间
//			pRoom->RecInfo.RecordStopTime.tv_sec 	= 0;
//			pRoom->RecInfo.RecordStopTime.tv_usec 	= 0;
//			gettimeofday(&pRoom->RecInfo.RecordStopTime, 0);
			rec_return_code = ROOM_RETURN_SUCC;
		} else {
			//原来未开启录制时，下发停止录制，回复2 以区分正常停止情况
			rec_return_code = 2;
		}
	} else {
		rec_return_code = ROOM_RETURN_FAIL;
	}

	nslog(NS_INFO, "OptType = %d, rec_return_code = %d, record_handle = %p", \
					pRoom->RecInfo.OptType, rec_return_code,  pRoom->record_handle);

	wait_st = -1;
	if (pRoom->RecInfo.OptType == MSG_RECORD_CTRL_START && \
		rec_return_code == ROOM_RETURN_SUCC && pRoom->record_handle == NULL) {

		wait_st = ROOM_MSG_TIME_NUM;
#if 0
		//等待码流5s, 都没准备好，就返回启动录制失败
		while (wait_st--) {
			req_num 		= upper_msg_get_mask_num(pRoom->RoomInfo.req_stream_mask);
			val_num 		= upper_msg_get_mask_num(pRoom->RoomInfo.valid_stream_mask);
			room_info_num 	= upper_msg_get_mask_num(pRoom->RoomInfo.room_info_mask);

			nslog(NS_INFO, "\n-----^^^^^---wait_st = [%d]---^^^^^---req_num = %d, val_num = %d, room_info_num = %d\n\n", wait_st, req_num, val_num, room_info_num);

			if (req_num == val_num && req_num == room_info_num) {
				break;
			}

			req_mask 		= pRoom->RoomInfo.req_stream_mask;
			valid_mask 		= pRoom->RoomInfo.valid_stream_mask;
			room_info_mask 	= pRoom->RoomInfo.room_info_mask;

			nslog(NS_INFO, "\n-^---req_mask & valid_mask = %d, req_mask & room_info_mask = %d, req_mask = %d\n\n", \
				req_mask & valid_mask, req_mask & room_info_mask, req_mask);

			if (((req_mask & valid_mask) == req_mask) && \
				((req_mask & room_info_mask) == req_mask) && \
				(req_mask != 0) ) {
				nslog(NS_INFO, " ---req_mask = %d,	valid_mask = %d, room_info_mask = %d!!!\n", req_mask, valid_mask, room_info_mask);
				break;
			}
			upper_msg_monitor_time_queue(pRoom);
			r_usleep(ROOM_MSG_RESP_TIME);
		}
#endif

		if (wait_st >= 0) {
			/*开启录制是在lower 端等待到ENC回复已锁定分辨率后才开启*/
			upper_msg_send_record_msg_to_enc(pRoom, parseXml, head, xml);
			upper_msg_time_que_add_node(pRoom, MSG_RECORD_CTRL, msg_pass, user_id, head, upper_msg_record_time_out_resp);
		} else {
			rec_return_code = ROOM_RETURN_FAIL;
		}
	}
	else if (pRoom->RecInfo.OptType == MSG_RECORD_CTRL_STOP)
	{
		//转发给ENC, 解锁分辨率
		upper_msg_send_record_msg_to_enc(pRoom, parseXml, head, xml);
	}

	if (wait_st < 0) {
		//回应
		int8_t *xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
		if (!xml_buf) {
			nslog(NS_ERROR, " ---[r_malloc] error!");
			return ROOM_RETURN_FAIL;
		}

		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		if (pRoom->record_handle) {
			r_strcpy(rec_file, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
			r_strcpy(rec_id, pRoom->record_handle->get_RecordID(pRoom->record_handle));
		}

		if (pRoom->record_handle) {
			pRoom->RecStatusInfo.RecStatus = lower_msg_opt_type_trans(pRoom->record_handle->get_record_status(pRoom->record_handle));
			r_strcpy(pRoom->RecStatusInfo.RecName, rec_file);
		} else {
			pRoom->RecStatusInfo.RecStatus = 0;
			r_strcpy(pRoom->RecStatusInfo.RecName, rec_file);
		}

		pRoom->RoomInfo.RecordStatus = pRoom->RecStatusInfo.RecStatus;
		r_strcpy(pRoom->RoomInfo.RcdName, pRoom->RecStatusInfo.RecName);

		upper_msg_package_record_xml(xml_buf, rec_return_code, rec_file, msg_pass, user_id, rec_id, pRoom->RecInfo.OptType, pRoom->RecStatusInfo.RecStatus);
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

		//打开失败，重置状态为0
		if (rec_return_code == ROOM_RETURN_FAIL && pRoom->RecInfo.OptType == MSG_RECORD_CTRL_START) {
			pRoom->RecInfo.OptType = 0;
		}

		//回复状态
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		lower_msg_package_req_status_info(xml_buf, &pRoom->RecStatusInfo);
		nslog(NS_ERROR,"WAHT ----5\n");

		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

		if (xml_buf)
			r_free(xml_buf);
	}

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_req_code_stream_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t i				= -1;
	int32_t socket 			= -1;
	int32_t stream_id		= -1;
	int32_t user_id_num		= -1;
	int32_t encode_type		= -1;
	int32_t req_flag		= 0;
	int32_t send_flag		= 0;
	int32_t ret_val			= 0;
	xmlNodePtr 	pNode 		= NULL;
	xmlNodePtr 	pNode_tmp	= NULL;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex 	= NULL;
	int32_t enc_id[ROOM_ENC_NUM] 		= {0};
	int8_t usr_id[ROOM_MSG_VAL_LEN] 	= {0};
	int8_t msg_pass[ROOM_MSG_VAL_LEN] 	= {0};
	int8_t enc_indx[ROOM_MSG_VAL_LEN] 	= {0};
	int8_t ret_code_val[2]	= {0};

	//获取pass key
	pNode_tmp = get_req_pass_key_node(&pNode_tmp, parseXml->proot);
	get_current_node_value(msg_pass, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode_tmp);

	//获取user id
	if (!upper_msg_get_user_id_value(parseXml, usr_id)) {
		nslog(NS_ERROR, " ---get user id error");
		return ROOM_RETURN_FAIL;
	} else {
		user_id_num = atoi(usr_id);
	}

	pNode = upper_msg_get_req_code_stream_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	//异常退出直播处理
	if (!r_strcmp(msg_pass, MSG_RECORD_PASS)) {

		//EncodeIndex
		r_bzero(enc_indx, ROOM_MSG_VAL_LEN);
		ret_val = upper_msg_get_leaf_value(enc_indx, ROOM_MSG_VAL_LEN, MSG_ENCODEINDEX_KEY, pNode, parseXml->pdoc);
		if (ret_val == ROOM_RETURN_FAIL) {
			nslog(NS_ERROR, " ---upper_msg_get_leaf_value error");
			return ROOM_RETURN_FAIL;
		}

		if (strchr(enc_indx, 'U')) {
			nslog(NS_WARN, " ---stop_lives_user_info_unusual");
			if (pRoom->live_handle)
			 	pRoom->live_handle->stop_lives_user_info_unusual(user_id_num,  pRoom->live_handle);

			return ROOM_RETURN_SUCC;
		} else {
			nslog(NS_WARN, " pass key = %s, EncodeIndex = %s", MSG_RECORD_PASS, enc_indx);
		}
	}

	ret_val = upper_msg_analyze_stream_req_info(&pRoom->StrmReq, pNode, parseXml->pdoc);
	if (ret_val == ROOM_RETURN_FAIL){
		nslog(NS_WARN, " ---upper_msg_analyze_stream_req_info error!");
		return ROOM_RETURN_FAIL;
	}

	nslog(NS_INFO, " ---req_code_stream analyze finish, EncodeIndex = %s", pRoom->StrmReq.EncodeIndex);

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

//	pRoom->StrmReqMask[index] = MSG_QUEUE_REC_CLR;
//	pRoom->StrmReqMask[index] |= MSG_QUEUE_REC_FLG;	//初始化为返回成功
	pRoom->StrmReqMask[index] = MSG_QUEUE_REC_ALL;

	nslog(NS_INFO, "--- index = %d, StrmReqMask[%d] = %d", index, index, pRoom->StrmReqMask[index]);

	if (strchr(pRoom->StrmReq.EncodeIndex, 'S')) {
		;//do nothing
	} else {
		if (strchr(pRoom->StrmReq.EncodeIndex, 'A') || strchr(pRoom->StrmReq.EncodeIndex, '1')) {
			enc_id[0] = 1;
			req_flag++;
		}

		if (strchr(pRoom->StrmReq.EncodeIndex, '2')) {
			enc_id[1] = 2;
			req_flag++;
		}

		if (strchr(pRoom->StrmReq.EncodeIndex, '3')) {
			enc_id[2] = 3;
			req_flag++;
		}

		if (strchr(pRoom->StrmReq.EncodeIndex, '4')) {
			enc_id[3] = 4;
			req_flag++;
		}

		if (strchr(pRoom->StrmReq.EncodeIndex, '5')) {
			enc_id[4] = 5;
			req_flag++;
		}

		if (strchr(pRoom->StrmReq.EncodeIndex, '6')) {
			enc_id[5] = 6;
			req_flag++;
		}

		if (strchr(pRoom->StrmReq.EncodeIndex, '7')) {
			enc_id[6] = 7;
			req_flag++;
		}

		if (strchr(pRoom->StrmReq.EncodeIndex, '8')) {
			enc_id[7] = 8;
			req_flag++;
		}

		if (strchr(pRoom->StrmReq.EncodeIndex, '9')) {
			enc_id[8] = 9;
			req_flag++;
		}

		for (i = 0; i < ROOM_ENC_NUM; i++) {
			if (pRoom->RoomInfo.EncInfo[i].ID == 0 || enc_id[i] == 0) {
				continue;
			}

			e_info = upper_msg_get_enc_info_by_id(pRoom, enc_id[i]);
			if (!e_info) {
				nslog(NS_INFO, "--- upper_msg_get_enc_info_by_id error");
				continue;
			}

			socket 		= -1;
			encode_type = e_info->EncodeType;
			e_info->SD_QuaInfo.RateType = MSG_RATETYPE_SD;
			e_info->HD_QuaInfo.RateType = MSG_RATETYPE_HD;

			nslog(NS_ERROR ," %d  ---- %d ---- %d\n",encode_type,pRoom->StrmReq.Quality,i);
			//HD
			if (pRoom->StrmReq.Quality == MSG_RATETYPE_HD && encode_type == EncodeType_H264) {
				socket		= e_info->HD_QuaInfo.Socket;
				mutex		= e_info->HD_QuaInfo.mutex;
				stream_id	= e_info->HD_QuaInfo.MsgQueType;
				pRoom->RoomInfo.req_stream_mask |= (1 << stream_id);
				pRoom->StrmReqMask[index] |= (1 << stream_id);
				e_info->HD_QuaInfo.RateType = MSG_RATETYPE_HD;
			}

			//SD
			if (pRoom->StrmReq.Quality == MSG_RATETYPE_SD && encode_type == EncodeType_H264) {
				socket		= e_info->SD_QuaInfo.Socket;
				mutex		= e_info->SD_QuaInfo.mutex;
				stream_id	= e_info->SD_QuaInfo.MsgQueType;
				pRoom->RoomInfo.req_stream_mask |= (1 << stream_id);
				pRoom->StrmReqMask[index] |= (1 << stream_id);
				e_info->SD_QuaInfo.RateType = MSG_RATETYPE_SD;
			}

			//JPEG
			if (encode_type == EncodeType_JPEG) {
				socket		= e_info->HD_QuaInfo.Socket;
				mutex		= e_info->HD_QuaInfo.mutex;
				stream_id	= e_info->HD_QuaInfo.MsgQueType;
				pRoom->RoomInfo.req_stream_mask |= (1 << stream_id);
				pRoom->StrmReqMask[index] |= (1 << stream_id);
				e_info->HD_QuaInfo.RateType = MSG_RATETYPE_HD;
			}

			if (socket > 0) {
				upper_msg_tcp_send(socket, mutex, head, xml);
				send_flag += 1;
			}

			nslog(NS_INFO, " ---stream---- EncNum = %d, i = %d, socket = %d, EncodeIndex = %s, Quality = %d, encode_type = %d, stream_id = %d",\
				pRoom->RoomInfo.EncNum, i, socket, pRoom->StrmReq.EncodeIndex, pRoom->StrmReq.Quality, encode_type);
		}

		nslog(NS_INFO, "send_flag = %d, req_flag = %d", send_flag, req_flag);

		//一路都没连接上，或者请求的和发送的流数不一致, 就直接回复失败
//		if (send_flag == 0/* || req_flag != send_flag*/) {
//			upper_msg_resp_stream_req(pRoom, parseXml, head, "0", usr_id, pRoom->StrmReq.EncodeIndex);
//			return ROOM_RETURN_FAIL;
//		}

	}

	nslog(NS_INFO, "--- index = %d, StrmReqMask[%d] = %d", index, index, pRoom->StrmReqMask[index]);

	int8_t  ip_addr[16] = {0};
	lives_user_info_t live_user_temp ;
	r_bzero((int8_t *)&live_user_temp, sizeof(lives_user_info_t));

	msg_ctrl_inet_ntoa(ip_addr, pRoom->StrmReq.Ipaddr1);
	r_memcpy(live_user_temp.lives_user_addr[0].m_user_ip, ip_addr, 16);
	live_user_temp.lives_user_addr[0].m_user_port = pRoom->StrmReq.Port1;

	msg_ctrl_inet_ntoa(ip_addr, pRoom->StrmReq.Ipaddr2);
	r_memcpy(live_user_temp.lives_user_addr[1].m_user_ip, ip_addr, 16);
	live_user_temp.lives_user_addr[1].m_user_port = pRoom->StrmReq.Port2;

	msg_ctrl_inet_ntoa(ip_addr, pRoom->StrmReq.Ipaddr3);
	r_memcpy(live_user_temp.lives_user_addr[2].m_user_ip, ip_addr, 16);
	live_user_temp.lives_user_addr[2].m_user_port = pRoom->StrmReq.Port3;

	// add zl
	msg_ctrl_inet_ntoa(ip_addr, pRoom->StrmReq.Ipaddr4);
	r_memcpy(live_user_temp.lives_user_addr[3].m_user_ip, ip_addr, 16);
	live_user_temp.lives_user_addr[3].m_user_port = pRoom->StrmReq.Port4;

	msg_ctrl_inet_ntoa(ip_addr, pRoom->StrmReq.Ipaddr5);
	r_memcpy(live_user_temp.lives_user_addr[4].m_user_ip, ip_addr, 16);
	live_user_temp.lives_user_addr[4].m_user_port = pRoom->StrmReq.Port5;

	msg_ctrl_inet_ntoa(ip_addr, pRoom->StrmReq.Ipaddr6);
	r_memcpy(live_user_temp.lives_user_addr[5].m_user_ip, ip_addr, 16);
	live_user_temp.lives_user_addr[5].m_user_port = pRoom->StrmReq.Port6;

	msg_ctrl_inet_ntoa(ip_addr, pRoom->StrmReq.Ipaddr7);
	r_memcpy(live_user_temp.lives_user_addr[6].m_user_ip, ip_addr, 16);
	live_user_temp.lives_user_addr[6].m_user_port = pRoom->StrmReq.Port7;

	msg_ctrl_inet_ntoa(ip_addr, pRoom->StrmReq.Ipaddr8);
	r_memcpy(live_user_temp.lives_user_addr[7].m_user_ip, ip_addr, 16);
	live_user_temp.lives_user_addr[7].m_user_port = pRoom->StrmReq.Port8;

	msg_ctrl_inet_ntoa(ip_addr, pRoom->StrmReq.Ipaddr9);
	r_memcpy(live_user_temp.lives_user_addr[8].m_user_ip, ip_addr, 16);
	live_user_temp.lives_user_addr[8].m_user_port = pRoom->StrmReq.Port9;

	live_user_temp.video_quality_type = pRoom->StrmReq.Quality;
	r_memcpy(live_user_temp.video_encode_index , pRoom->StrmReq.EncodeIndex, VIDEO_ENCODE_INDEX_LEN);
//	r_memcpy(live_user_temp.video_encode_index , pRoom->StrmReq.EncodeIndex, 4);

	live_user_temp.user_id = user_id_num;

	nslog(NS_INFO, "--- stream_num = %d, EncodeIndex = %s, pRoom->live_handle = %p, \n[Ipaddr1:%s, Port1:%d] \n[Ipaddr2:%s, Port2:%d] \n[Ipaddr3:%s, Port3:%d]", \
		pRoom->handle->stream_num, pRoom->StrmReq.EncodeIndex, pRoom->live_handle, live_user_temp.lives_user_addr[0].m_user_ip, pRoom->StrmReq.Port1, live_user_temp.lives_user_addr[1].m_user_ip, pRoom->StrmReq.Port2, live_user_temp.lives_user_addr[2].m_user_ip, pRoom->StrmReq.Port3);

	if (strchr(pRoom->StrmReq.EncodeIndex, 'S')) {
		if (pRoom->live_handle)
		 	ret_val = pRoom->live_handle->stop_lives_user_info(&live_user_temp,  pRoom->live_handle);
		else
			ret_val = -1;

		if (!ret_val)
			r_strcpy(ret_code_val, "1");
		else
			r_strcpy(ret_code_val, "0");

		upper_msg_resp_stream_req(pRoom, parseXml, head, ret_code_val, usr_id, pRoom->StrmReq.EncodeIndex);

		nslog(NS_INFO, "--- stop_lives_user_info--- ret_val = %d i = %d", ret_val, i);
	} else {
		if (pRoom->live_handle) {
			for (i = 0; i < pRoom->handle->stream_num; i++) {
				pRoom->handle->set_live_status(&pRoom->handle->stream_hand[i], START_LIVE);

				nslog(NS_INFO, "--- set_live_status--- stream_num = %d i = %d", pRoom->handle->stream_num, i);
			}

		 	ret_val = pRoom->live_handle->set_lives_user_info(&live_user_temp,  pRoom->live_handle);
			nslog(NS_INFO, "--- set_lives_user_info--- ret_val = %d i = %d", ret_val, i);

			if (!ret_val)
				r_strcpy(ret_code_val, "1");
			else
				r_strcpy(ret_code_val, "0");

			upper_msg_resp_stream_req(pRoom, parseXml, head, ret_code_val, usr_id, pRoom->StrmReq.EncodeIndex);
		} else {
			upper_msg_resp_stream_req(pRoom, parseXml, head, "0", usr_id, pRoom->StrmReq.EncodeIndex);
		}
	}

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_remote_ctrl_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t     ret_val_1				= ROOM_RETURN_FAIL;
	int32_t     ret_val_2				= ROOM_RETURN_FAIL;
	xmlNodePtr 	pNode 					= NULL;
	int8_t 		*xml_buf				= NULL;
	int8_t 		value[ROOM_MSG_VAL_LEN] = {0};
	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	RemoteCtrlInfo remote_ctrl_info;

	pNode = upper_msg_get_remote_ctrl_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ROOM_RETURN_FAIL;
	}

	pRoom->remoteCtrlMask[index] = MSG_QUEUE_REC_CLR;
	pRoom->remoteCtrlMask[index] |= MSG_QUEUE_REC_FLG;

	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);

	//passkey
	xmlNodePtr 	pass_key_Node 					= NULL;
	get_req_pass_key_node(&pass_key_Node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, pass_key_Node);

	int32_t i 				= 0;
	int32_t enc_id 			= 0;
	int32_t socket 			= -1;
	int32_t socket_HD		= -1;
	int32_t socket_SD		= -1;
	int32_t stream_id		= -1;
	pthread_mutex_t *mutex 	= NULL;
	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			continue;
		}

		socket_HD 	= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.Socket;
		socket_SD 	= pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.Socket;
		if (socket_HD > 0)	{
			socket		= socket_HD;
			enc_id 		= pRoom->RoomInfo.EncInfo[i].ID;
			mutex 		= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.mutex;
			stream_id 	= pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType;
		} else if (socket_SD > 0)	{
			socket		= socket_SD;
			enc_id 		= pRoom->RoomInfo.EncInfo[i].ID;
			mutex 		= pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.mutex;
			stream_id 	= pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType;
		} else {
			continue;
		}

		//封包
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		r_bzero(&remote_ctrl_info, sizeof(RemoteCtrlInfo));
		ret_val_1 = upper_msg_save_remote_ctrl_info_by_id(&remote_ctrl_info, pNode, parseXml->pdoc, enc_id);
		if (ROOM_RETURN_FAIL == ret_val_1) {
			nslog(NS_WARN, "enc:enc_id = %d not found, continue for next", enc_id);
			continue;
		}

		upper_msg_package_remote_ctrl_info(&remote_ctrl_info, xml_buf, ret_pass_key, user_id, pRoom->RoomInfo.RoomId);

		pRoom->remoteCtrlMask[index] |= (1 << stream_id);

		//发送
		if (socket > 0) {
			ret_val_1 = upper_msg_tcp_send(socket, mutex, head, xml_buf);
		}

		nslog(NS_INFO, " ---remote_ctrl--- EncNum = %d, i = %d, socket = %d, stream_id = %d", pRoom->RoomInfo.EncNum, i, socket, stream_id);
	}

	if (pRoom->remoteCtrlMask[index] == MSG_QUEUE_REC_FLG) {
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		upper_msg_package_reponse_xml(xml_buf, MSG_REMOTE_CTRL, "0", ret_pass_key, user_id, pRoom->RoomInfo.RoomId);
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);
	} else {
		//添加到超时队列
		upper_msg_time_que_add_node(pRoom, MSG_REMOTE_CTRL, ret_pass_key, user_id, head, upper_msg_time_out_resp);
	}

	if (xml_buf)
		r_free(xml_buf);

	return ROOM_RETURN_SUCC;
}


int32_t upper_msg_iFrame_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	xmlNodePtr	pNode	= NULL;
	int8_t 		EncodeIndex[ROOM_MSG_VAL_LEN] = {0};
	int32_t 	ret 	= 0;

	pNode = upper_msg_get_iFrame_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	r_bzero(EncodeIndex, ROOM_MSG_VAL_LEN);
	ret = upper_msg_get_leaf_value(EncodeIndex, ROOM_MSG_VAL_LEN, MSG_ENCODEINDEX_KEY, pNode, parseXml->pdoc);
	if (ret == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, "get EncodeIndex error");
		return ROOM_RETURN_FAIL;
	}

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	pRoom->iframeMask[index] = MSG_QUEUE_REC_CLR;
	pRoom->iframeMask[index] |= MSG_QUEUE_REC_FLG;	//初始化为返回成功

	int32_t i				= -1;
	int32_t socket			= -1;
	int32_t stream_id		= -1;
	int32_t encode_type 	= -1;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex	= NULL;
	int32_t enc_id[ROOM_ENC_NUM] = {0};

	if (strchr(EncodeIndex, '1')) {
		enc_id[0] = 1;
	}

	if (strchr(EncodeIndex, '2')) {
		enc_id[1] = 2;
	}

	if (strchr(EncodeIndex, '3')) {
		enc_id[2] = 3;
	}

	if (strchr(EncodeIndex, '4')) {
		enc_id[3] = 4;
	}

	if (strchr(EncodeIndex, '5')) {
		enc_id[4] = 5;
	}

	if (strchr(EncodeIndex, '6')) {
		enc_id[5] = 6;
	}
	// add zl
	if (strchr(EncodeIndex, '7')) {
		enc_id[6] = 7;
	}

	if (strchr(EncodeIndex, '8')) {
		enc_id[7] = 8;
	}

	if (strchr(EncodeIndex, '9')) {
		enc_id[8] = 9;
	}

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0 || enc_id[i] == 0) {
			continue;
		}

		e_info = upper_msg_get_enc_info_by_id(pRoom, enc_id[i]);
		if (!e_info) {
			nslog(NS_WARN, "enc:enc_id = %d not found, continue for next", enc_id[i]);
			continue;
		}

		//HD
		socket		= e_info->HD_QuaInfo.Socket;
		mutex		= e_info->HD_QuaInfo.mutex;
		stream_id	= e_info->HD_QuaInfo.MsgQueType;
		pRoom->iframeMask[index] |= (1 << stream_id);

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, head, xml);
		}
		nslog(NS_INFO, " --- HD iFrame---EncodeIndex = %s, EncNum = %d, i = %d, socket = %d, stream_id = %d", EncodeIndex, pRoom->RoomInfo.EncNum, i, socket, stream_id);

		if (e_info->EncodeType == EncodeType_JPEG) {
			continue;
		}

		//SD
		socket		= e_info->SD_QuaInfo.Socket;
		mutex		= e_info->SD_QuaInfo.mutex;
		stream_id	= e_info->SD_QuaInfo.MsgQueType;
		pRoom->iframeMask[index] |= (1 << stream_id);

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, head, xml);
		}
		nslog(NS_INFO, " --- SD iFrame---EncodeIndex = %s, EncNum = %d, i = %d, socket = %d, stream_id = %d", EncodeIndex, pRoom->RoomInfo.EncNum, i, socket, stream_id);

	}

	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	tmp_node = NULL;

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	//添加到超时队列
	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);
	upper_msg_time_que_add_node(pRoom, MSG_IFRAMEREQ_CTRL, ret_pass_key, user_id, head, upper_msg_time_out_resp);

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_connect_room_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	xmlNodePtr	pNode		= NULL;
	int32_t 	opt_type	= 0;
	int32_t 	retVal  	= 0;
	xmlNodePtr 	tmp_node 	= NULL;
	int8_t 		*xml_buf 	= 	NULL;
	int8_t		value[ROOM_MSG_VAL_LEN] 			= {0};
	int8_t 		ret_pass_key[ROOM_MSG_VAL_LEN] 		= {0};
	int8_t 		msg_return_code[ROOM_MSG_VAL_LEN] 	= {0};
	int32_t 	return_code = 0;
	int32_t 	i	= 0;
	int32_t 	num = 0;

	pNode = upper_msg_get_connect_room_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);
//	nslog(NS_WARN, " ---user_id = [%s]!\n%s", user_id, xml);

	//获取pass key
	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	r_bzero(value, ROOM_MSG_VAL_LEN);
	retVal = upper_msg_get_leaf_value(value, ROOM_MSG_VAL_LEN, MSG_OPTTYPE_KEY, pNode, parseXml->pdoc);
	if (retVal == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, "get OptType error");
		return ROOM_RETURN_FAIL;
	} else {
		opt_type = atoi(value);
	}

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ROOM_RETURN_FAIL;
	}

	if (pRoom->record_handle) {
		nslog(NS_WARN, " --- RecInfo.OptType = %d", pRoom->RecInfo.OptType);
		sprintf(msg_return_code, "%d", 0);
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		upper_msg_package_connc_room_req_xml(xml_buf, MSG_CONNECT_ROOM_REQ, msg_return_code, ret_pass_key, opt_type, user_id, pRoom->RoomInfo.RoomId);
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);
		goto exit;
	}

	if (opt_type) {
		if (pRoom->RoomInfo.ConnectStatus == 1) {
			//全部都连接上，直接回应成功
			sprintf(msg_return_code, "%d", 1);
			r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
			upper_msg_package_connc_room_req_xml(xml_buf, MSG_CONNECT_ROOM_REQ, msg_return_code, ret_pass_key, opt_type, user_id, pRoom->RoomInfo.RoomId);
			upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);
		} else {
			//部分连接上，直接回应失败
			if (upper_msg_get_valid_stream_num(pRoom)) {
				sprintf(msg_return_code, "%d", 0);
				r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
				upper_msg_package_connc_room_req_xml(xml_buf, MSG_CONNECT_ROOM_REQ, msg_return_code, ret_pass_key, opt_type, user_id, pRoom->RoomInfo.RoomId);
				upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);
			} else {
				pRoom->RoomInfo.ConnectStatus		= 0;
				pRoom->RecStatusInfo.ConnStatus 	= 0;
				pRoom->RoomInfo.req_stream_mask 	= MSG_QUEUE_REC_CLR;
				pRoom->RoomInfo.room_info_mask		= MSG_QUEUE_REC_CLR;
				pRoom->RoomInfo.valid_stream_mask	= MSG_QUEUE_REC_CLR;


				pRoom->loginMask = MSG_QUEUE_REC_CLR;
				pRoom->loginMask |= MSG_QUEUE_REC_FLG;

				for (i = 0, num = 0; i < ROOM_ENC_NUM; i++) {
					if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
						continue;
					}

					pRoom->RoomInfo.EncInfo[i].Status	= 0;
					pRoom->RoomInfo.EncInfo[i].s_Status = 0;
					pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.Status = 0;
					pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.Status = 0;

					pRoom->loginMask |= (1 << pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType);
					pRoom->loginMask |= (1 << pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType);
				}

				retVal = pRoom->handle->open_room_connect(pRoom->handle);

				//添加到超时队列,等待登录后返回
				upper_msg_time_que_add_node(pRoom, MSG_CONNECT_ROOM_REQ, ret_pass_key, user_id, head, upper_msg_connect_time_out_resp);
			}
		}
	} else {
		retVal = pRoom->handle->close_room_connect(pRoom->handle);
		if (return_code == 0) {
			return_code = retVal;
		}

		if (retVal == 0) {
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


		//回应
		if (return_code == -1)
			return_code = 0;
		else
			return_code = 1;

		sprintf(msg_return_code, "%d", return_code);
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		upper_msg_package_connc_room_req_xml(xml_buf, MSG_CONNECT_ROOM_REQ, msg_return_code, ret_pass_key, opt_type, user_id, pRoom->RoomInfo.RoomId);
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);
	}

exit:

	nslog(NS_INFO, " --- connect_room_req--- OPTTYPE = %d, retVal = %d", opt_type, retVal);

	if (xml_buf)
		r_free(xml_buf);

	return ROOM_RETURN_SUCC;
}


int32_t upper_msg_send_logo_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	xmlNodePtr 	pNode = NULL;
	int8_t      EncodeIndex[ROOM_MSG_VAL_LEN] = {0};

	pNode = upper_msg_get_send_logo_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	upper_msg_get_leaf_value(EncodeIndex, ROOM_MSG_VAL_LEN, MSG_ENCODEINDEX_KEY, pNode, parseXml->pdoc);

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	pRoom->logoMask[index] = MSG_QUEUE_REC_CLR;
	pRoom->logoMask[index] |= MSG_QUEUE_REC_FLG;	//初始化为返回成功

	int32_t i				= -1;
	int32_t socket			= -1;
	int32_t stream_id		= -1;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex	= NULL;
	int32_t enc_id[ROOM_ENC_NUM] = {0};

	if (strchr(EncodeIndex, '1')) {
		enc_id[0] = 1;
	}

	if (strchr(EncodeIndex, '2')) {
		enc_id[1] = 2;
	}

	if (strchr(EncodeIndex, '3')) {
		enc_id[2] = 3;
	}

	if (strchr(EncodeIndex, '4')) {
		enc_id[3] = 4;
	}

	if (strchr(EncodeIndex, '5')) {
		enc_id[4] = 5;
	}

	if (strchr(EncodeIndex, '6')) {
		enc_id[5] = 6;
	}
	// add zl
	if (strchr(EncodeIndex, '7')) {
		enc_id[6] = 7;
	}
	if (strchr(EncodeIndex, '8')) {
		enc_id[7] = 8;
	}
	if (strchr(EncodeIndex, '9')) {
		enc_id[8] = 9;
	}

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0 || enc_id[i] == 0) {
			continue;
		}

		e_info = upper_msg_get_enc_info_by_id(pRoom, enc_id[i]);
		if (!e_info) {
			nslog(NS_WARN, "enc:enc_id = %d not found, continue for next", enc_id[i]);
			continue;
		}

		//HD
		socket		= e_info->HD_QuaInfo.Socket;
		mutex		= e_info->HD_QuaInfo.mutex;
		stream_id	= e_info->HD_QuaInfo.MsgQueType;
		pRoom->logoMask[index] |= (1 << stream_id);

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, head, xml);
		}

		nslog(NS_INFO, " --- logo --- EncodeIndex = %s, i = %d, EncNum = %d, socket = %d, stream_id = %d, logoMask[%d] = %d", EncodeIndex, i, pRoom->RoomInfo.EncNum, socket, stream_id, index, pRoom->logoMask[index]);
	}

	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	tmp_node = NULL;

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	//添加到超时队列
	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);
	upper_msg_time_que_add_node(pRoom, MSG_SEND_LOGO_PIC, ret_pass_key, user_id, head, upper_msg_time_out_resp);

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_add_title_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	xmlNodePtr	pNode	= NULL;
	int8_t      EncodeIndex[ROOM_MSG_VAL_LEN] = {0};

	pNode = upper_msg_get_add_title_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	upper_msg_get_leaf_value(EncodeIndex, ROOM_MSG_VAL_LEN, MSG_ENCODEINDEX_KEY, pNode, parseXml->pdoc);

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	pRoom->titleMask[index] = MSG_QUEUE_REC_CLR;
	pRoom->titleMask[index] |= MSG_QUEUE_REC_FLG;	//初始化为返回成功

	int32_t i				= -1;
	int32_t socket			= -1;
	int32_t stream_id		= -1;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex	= NULL;
	int32_t enc_id[ROOM_ENC_NUM] = {0};

	if (strchr(EncodeIndex, '1')) {
		enc_id[0] = 1;
	}

	if (strchr(EncodeIndex, '2')) {
		enc_id[1] = 2;
	}

	if (strchr(EncodeIndex, '3')) {
		enc_id[2] = 3;
	}

	if (strchr(EncodeIndex, '4')) {
		enc_id[3] = 4;
	}

	if (strchr(EncodeIndex, '5')) {
		enc_id[4] = 5;
	}

	if (strchr(EncodeIndex, '6')) {
		enc_id[5] = 6;
	}

	if (strchr(EncodeIndex, '6')) {
		enc_id[5] = 6;
	}

	// add zl
	if (strchr(EncodeIndex, '7')) {
		enc_id[6] = 7;
	}

	if (strchr(EncodeIndex, '8')) {
		enc_id[7] = 8;
	}

	if (strchr(EncodeIndex, '9')) {
		enc_id[8] = 9;
	}

	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0 || enc_id[i] == 0) {
			continue;
		}

		e_info = upper_msg_get_enc_info_by_id(pRoom, enc_id[i]);
		if (!e_info) {
			nslog(NS_WARN, "enc:enc_id = %d not found, continue for next", enc_id[i]);
			continue;
		}

		//HD
		socket		= e_info->HD_QuaInfo.Socket;
		mutex		= e_info->HD_QuaInfo.mutex;
		stream_id	= e_info->HD_QuaInfo.MsgQueType;
		pRoom->titleMask[index] |= (1 << stream_id);

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, head, xml);
		}

		nslog(NS_INFO, " --- title--- EncodeIndex = %s, i = %d, EncNum = %d, socket = %d, stream_id = %d, titleMask[%d] = %d", EncodeIndex, i, pRoom->RoomInfo.EncNum, socket, stream_id, index, pRoom->titleMask[index]);
	}

	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	tmp_node = NULL;

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	//添加到超时队列
	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);
	upper_msg_time_que_add_node(pRoom, MSG_ADD_TITLE_REQ, ret_pass_key, user_id, head, upper_msg_time_out_resp);

	return ROOM_RETURN_SUCC;
}


int32_t upper_msg_volume_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	xmlNodePtr	pNode		= NULL;
	int32_t 	socket_HD	= -1;
	int32_t 	socket_SD	= -1;
	int32_t 	stream_id	= -1;
	pthread_mutex_t *mutex 	= NULL;
	int8_t 		*msg_xml	= NULL;
	xmlNodePtr	pNode_tmp	= NULL;
	xmlNodePtr	tmp_node 	= NULL;
	int8_t 		msg_pass[ROOM_MSG_VAL_LEN]	= {0};
	int8_t 		user_id[ROOM_MSG_VAL_LEN] 	= {0};

	pNode = upper_msg_get_volume_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}


	upper_msg_get_user_id_value(parseXml, user_id);

	//获取pass key
	pNode_tmp = get_req_pass_key_node(&pNode_tmp, parseXml->proot);
	get_current_node_value(msg_pass, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode_tmp);

	pRoom->volumeMask[index] = MSG_QUEUE_REC_FLG;

	// 音频只发送ENC1200 高清版	第四个编码器!!! add zl

	if (pRoom->RoomInfo.EncInfo[3].ID == 0 ||\
		pRoom->RoomInfo.EncInfo[3].Status == 0)  {
		goto exit;
	}

	socket_HD	= pRoom->RoomInfo.EncInfo[3].HD_QuaInfo.Socket;

	if(socket_HD >0)
	{
		mutex		= pRoom->RoomInfo.EncInfo[3].HD_QuaInfo.mutex;
		stream_id	= pRoom->RoomInfo.EncInfo[3].HD_QuaInfo.MsgQueType;
		pRoom->volumeMask[index] |= (1 << stream_id);
		upper_msg_tcp_send(socket_HD, mutex, head, xml);
	}
	else
	{
		goto exit;
	}

	#if 0

	if (pRoom->RoomInfo.EncInfo[0].ID == 0 ||\
		pRoom->RoomInfo.EncInfo[0].Status == 0)  {
		goto exit;
	}
	//音频相关只往第一路发
	socket_HD	= pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.Socket;
	socket_SD	= pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.Socket;
	if (socket_HD > 0) {
		mutex		= pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.mutex;
		stream_id	= pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.MsgQueType;
		pRoom->volumeMask[index] |= (1 << stream_id);
		upper_msg_tcp_send(socket_HD, mutex, head, xml);
	} else if (socket_SD > 0) {
		mutex		= pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.mutex;
		stream_id	= pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.MsgQueType;
		pRoom->volumeMask[index] |= (1 << stream_id);
		upper_msg_tcp_send(socket_SD, mutex, head, xml);
	} else {
		goto exit;
	}
	#endif
exit:

//	nslog(NS_INFO, " --- volume_req--- stream_id = %d, volumeMask[%d] = %d", stream_id, index, pRoom->volumeMask[index]);

	if (pRoom->volumeMask[index] == MSG_QUEUE_REC_FLG) {
		msg_xml = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
		if (!msg_xml) {
			nslog(NS_ERROR, " --- r_malloc error!");
			return ROOM_RETURN_FAIL;
		}

		r_bzero(msg_xml, ROOM_MSG_MAX_LEN);
		upper_msg_package_reponse_xml(msg_xml, MSG_VOLUME_REQ, "0", msg_pass, user_id, pRoom->RoomInfo.RoomId);
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, msg_xml);

		if (msg_xml)
			r_free(msg_xml);
	} else {
		//添加到超时队列
		upper_msg_time_que_add_node(pRoom, MSG_VOLUME_REQ, msg_pass, user_id, head, upper_msg_time_out_resp);
	}

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_mute_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	xmlNodePtr	pNode		= NULL;
	int32_t 	socket_HD	= -1;
	int32_t 	socket_SD	= -1;
	int32_t 	stream_id	= -1;
	pthread_mutex_t *mutex 	= NULL;
	int8_t 		*msg_xml	= NULL;
	xmlNodePtr	pNode_tmp	= NULL;
	xmlNodePtr	tmp_node 	= NULL;
	int8_t 		msg_pass[ROOM_MSG_VAL_LEN]	= {0};
	int8_t 		user_id[ROOM_MSG_VAL_LEN] 	= {0};

	pNode = upper_msg_get_mute_req_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	upper_msg_get_user_id_value(parseXml, user_id);

	//获取pass key
	pNode_tmp = get_req_pass_key_node(&pNode_tmp, parseXml->proot);
	get_current_node_value(msg_pass, ROOM_MSG_VAL_LEN, parseXml->pdoc, pNode_tmp);

	pRoom->muteMask[index] = MSG_QUEUE_REC_FLG;

	// 音频只发送ENC1200 高清版	第四个编码器!!! add zl

	if (pRoom->RoomInfo.EncInfo[3].ID == 0 ||\
		pRoom->RoomInfo.EncInfo[3].Status == 0)  {
		goto exit;
	}

	socket_HD	= pRoom->RoomInfo.EncInfo[3].HD_QuaInfo.Socket;
	if(socket_HD >0)
	{
		mutex		= pRoom->RoomInfo.EncInfo[3].HD_QuaInfo.mutex;
		stream_id	= pRoom->RoomInfo.EncInfo[3].HD_QuaInfo.MsgQueType;
		pRoom->muteMask[index] |= (1 << stream_id);
		upper_msg_tcp_send(socket_HD, mutex, head, xml);
	}
	else
	{
		goto exit;
	}

	#if 0
	if (pRoom->RoomInfo.EncInfo[0].ID == 0 ||\
		pRoom->RoomInfo.EncInfo[0].Status == 0)  {
		goto exit;
	}

	//音频相关只往第一路发
	socket_HD	= pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.Socket;
	socket_SD	= pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.Socket;
	if (socket_HD > 0) {
		mutex		= pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.mutex;
		stream_id	= pRoom->RoomInfo.EncInfo[0].HD_QuaInfo.MsgQueType;
		pRoom->muteMask[index] |= (1 << stream_id);
		upper_msg_tcp_send(socket_HD, mutex, head, xml);
	} else if (socket_SD > 0) {
		mutex		= pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.mutex;
		stream_id	= pRoom->RoomInfo.EncInfo[0].SD_QuaInfo.MsgQueType;
		pRoom->muteMask[index] |= (1 << stream_id);
		upper_msg_tcp_send(socket_SD, mutex, head, xml);
	} else {
		goto exit;
	}
	#endif
exit:

	nslog(NS_INFO, " --- _mute_req--- stream_id = %d, muteMask[%d] = %d", stream_id, index, pRoom->muteMask[index]);

	if (pRoom->muteMask[index] == MSG_QUEUE_REC_FLG) {
		msg_xml = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
		if (!msg_xml) {
			nslog(NS_ERROR, " --- r_malloc error!");
			return ROOM_RETURN_FAIL;
		}

		r_bzero(msg_xml, ROOM_MSG_MAX_LEN);
		upper_msg_package_reponse_xml(msg_xml, MSG_MUTE_REQ, "0", msg_pass, user_id, pRoom->RoomInfo.RoomId);
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, msg_xml);

		if (msg_xml)
			r_free(msg_xml);
	} else {
		//添加到超时队列
		upper_msg_time_que_add_node(pRoom, MSG_MUTE_REQ, msg_pass, user_id, head, upper_msg_time_out_resp);
	}

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_pic_adjust_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	xmlNodePtr	pNode	= NULL;
	int8_t      EncodeIndex[ROOM_MSG_VAL_LEN] = {0};

	pNode = upper_msg_get_pic_adjust_node(parseXml, pRoom->RoomInfo.RoomId);
	if (pNode == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	upper_msg_get_leaf_value(EncodeIndex, ROOM_MSG_VAL_LEN, MSG_ENCODEINDEX_KEY, pNode, parseXml->pdoc);

	int32_t index = lower_msg_get_platfrom_key(parseXml);
	if (index == -1) {
		nslog(NS_ERROR, " ---[get_req_pass_key_node] error!");
		return ROOM_RETURN_FAIL;
	}

	pRoom->adjustMask[index] = MSG_QUEUE_REC_CLR;
	pRoom->adjustMask[index] |= MSG_QUEUE_REC_FLG;	//初始化为返回成功

	int32_t i				= -1;
	int32_t socket			= -1;
	int32_t stream_id		= -1;
	enc_info *e_info		= NULL;
	pthread_mutex_t *mutex	= NULL;
	int32_t enc_id[ROOM_ENC_NUM] = {0};

	if (strchr(EncodeIndex, '1')) {
		enc_id[0] = 1;
	}

	if (strchr(EncodeIndex, '2')) {
		enc_id[1] = 2;
	}

	if (strchr(EncodeIndex, '3')) {
		enc_id[2] = 3;
	}

	if (strchr(EncodeIndex, '4')) {
		enc_id[3] = 4;
	}

	if (strchr(EncodeIndex, '5')) {
		enc_id[4] = 5;
	}

	if (strchr(EncodeIndex, '6')) {
		enc_id[5] = 6;
	}
	// add zl
	if (strchr(EncodeIndex, '7')) {
		enc_id[6] = 7;
	}

	if (strchr(EncodeIndex, '8')) {
		enc_id[7] = 8;
	}

	if (strchr(EncodeIndex, '9')) {
		enc_id[8] = 9;
	}


	for (i = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0 || enc_id[i] == 0) {
			continue;
		}

		e_info = upper_msg_get_enc_info_by_id(pRoom, enc_id[i]);
		if (!e_info) {
			nslog(NS_WARN, "enc:enc_id = %d not found, continue for next", enc_id[i]);
			continue;
		}

		//HD
		socket		= e_info->HD_QuaInfo.Socket;
		mutex		= e_info->HD_QuaInfo.mutex;
		stream_id	= e_info->HD_QuaInfo.MsgQueType;
		pRoom->adjustMask[index] |= (1 << stream_id);

		if (socket > 0) {
			upper_msg_tcp_send(socket, mutex, head, xml);
		}

		nslog(NS_INFO, " --- adjust--- EncodeIndex = %s, i = %d, EncNum = %d, socket = %d, stream_id = %d, adjustMask[%d] = %d", EncodeIndex, i, pRoom->RoomInfo.EncNum, socket, stream_id, index, pRoom->adjustMask[index]);
	}

	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	tmp_node = NULL;

	get_req_pass_key_node(&tmp_node, parseXml->proot);
	get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);

	//添加到超时队列
	int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
	upper_msg_get_user_id_value(parseXml, user_id);
	upper_msg_time_que_add_node(pRoom, MSG_PICADJUST_REQ, ret_pass_key, user_id, head, upper_msg_time_out_resp);

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_enc_login_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	return ROOM_RETURN_SUCC;
}


int32_t upper_msg_enc_restart_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_enc_update_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_system_info_req_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	upper_msg_analyze_system_info(&pRoom->RecInfo, parseXml);

	nslog(NS_INFO, " --- rec_info: MsgCode = 36002, RecPath = %s, ServerSeries = %s", pRoom->RecInfo.RecPath, pRoom->RecInfo.ServerSeries);

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_resp_server_info_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t i 		= 0;
	int8_t *xml_buf = NULL;
	int8_t  rec_file[ROOM_MSG_VAL_LEN] 	= {0};
	int8_t  rec_id[ROOM_MSG_VAL_LEN] 	= {0};

	upper_msg_analyze_resp_server_info(&pRoom->server_info, parseXml);
	nslog(NS_INFO, "DiskAvailableSpace = %d, record_handle = %p\n", pRoom->server_info.DiskAvailableSpace, pRoom->record_handle);
	if (pRoom->server_info.DiskAvailableSpace < MSG_MIN_VALID_SPACE && pRoom->record_handle) {
		for (i = 0; i < pRoom->handle->stream_num; i++) {
			pRoom->handle->set_rec_status(&pRoom->handle->stream_hand[i], STOP_REC);
		}

		r_strcpy(rec_file, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
		r_strcpy(rec_id, pRoom->record_handle->get_RecordID(pRoom->record_handle));
		pRoom->record_handle->record_close(pRoom->record_handle);
		unregister_course_record_mode(&pRoom->record_handle);
		pRoom->record_handle = NULL;
		pRoom->RoomInfo.record_stream_mask = 0;

		//记录停止时间
//		pRoom->RecInfo.RecordStopTime.tv_sec 	= 0;
//		pRoom->RecInfo.RecordStopTime.tv_usec 	= 0;
//		gettimeofday(&pRoom->RecInfo.RecordStopTime, 0);

		//回复状态
		xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
		if (!xml_buf) {
			nslog(NS_ERROR, " ---[r_malloc] error!");
			return ROOM_RETURN_FAIL;
		}

		pRoom->RecInfo.OptType			= 0;
		pRoom->RecStatusInfo.RecStatus 	= 0;
		r_strcpy(pRoom->RecStatusInfo.RecName, rec_file);

		//回复服务端
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		upper_msg_package_record_xml(xml_buf, 1, pRoom->RecStatusInfo.RecName, MSG_ALLPLATFORM_PASSKEY, pRoom->RecInfo.userid, rec_id, pRoom->RecInfo.OptType, pRoom->RecStatusInfo.RecStatus);
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		lower_msg_package_req_status_info(xml_buf, &pRoom->RecStatusInfo);
		nslog(NS_ERROR,"WAHT ----6\n");
		upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, head, xml_buf);

		//解锁分辨率
		r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
		lower_msg_package_record_req(xml_buf, NULL, 0);
		lower_msg_send_xml_to_enc(pRoom, xml_buf);

		if (xml_buf)
			r_free(xml_buf);
	}

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_req_system_info_deal(RoomMsgEnv *pRoom)
{
	int32_t ret_val = 0;

	ret_val = upper_msg_monitor_time_out_status(&pRoom->server_info.time, SERVER_INFO_TIME_OUT);
	if (ret_val == ROOM_RETURN_FAIL || ret_val == ROOM_RETURN_TIMEOUT) {
		pRoom->server_info.time.tv_sec	= 0;
		pRoom->server_info.time.tv_usec = 0;
		gettimeofday(&pRoom->server_info.time, 0);

		upper_msg_get_server_info(pRoom);
	}

	return ROOM_RETURN_SUCC;
}

int32_t upper_msg_resp_room_info_deal(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t 	i 		= 0;
	int32_t 	j 		= 0;
	int32_t     num		= 0;
	recv_param 	rec_param[ROOM_STR_NUM];
	int8_t		ret_pass_key[ROOM_MSG_VAL_LEN] = {0};
	xmlNodePtr	tmp_node = NULL;

	//本地配置过IP，则不再使用control 下发的信息
	num = upper_msg_check_enc_num(pRoom);
	if (num > 0) {
		nslog(NS_WARN, " ---upper_msg_check_enc_num, num = %d !", num);
		return ROOM_RETURN_FAIL;
	}

	tmp_node = upper_msg_get_room_info_resp_node(parseXml, pRoom->RoomInfo.RoomId);
	if (tmp_node == NULL) {
		nslog(NS_WARN, " ---room id is not cmp!");
		return ROOM_RETURN_FAIL;
	}

	//更新教室信息
	upper_msg_init_enc_id(pRoom);
	lower_msg_load_room_info(pRoom, xml);

	pRoom->loginMask = MSG_QUEUE_REC_CLR;
	pRoom->loginMask |= MSG_QUEUE_REC_FLG;

	r_bzero(rec_param, sizeof(rec_param));
	for (i = 0, j = 0, num = 0; i < ROOM_ENC_NUM; i++) {
		if (pRoom->RoomInfo.EncInfo[i].ID == 0) {
			j+=2;
			continue;
		}

		pRoom->RoomInfo.EncInfo[i].Status 	= 0;
		pRoom->RoomInfo.EncInfo[i].s_Status = 0;
		pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.Status = 0;
		pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.Status = 0;
		pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.RateType = MSG_RATETYPE_HD;
		pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.RateType = MSG_RATETYPE_SD;

		rec_param[j].status 	= 1;
		//rec_param[j].port 		= 3400;
		rec_param[j].enc_id = pRoom->RoomInfo.EncInfo[i].ID	;   // add zl
		rec_param[j].stream_id 	= j + 1;
		msg_ctrl_inet_ntoa(rec_param[j].ipaddr, pRoom->RoomInfo.EncInfo[i].EncIP);
		pRoom->RoomInfo.EncInfo[i].HD_QuaInfo.MsgQueType = rec_param[j].stream_id;
		pRoom->loginMask |= (1 << rec_param[j].stream_id);
		nslog(NS_INFO, " ---HD_QuaInfo- EncNum = %d, iEnc = %d, jRec = %d, port = %d, stream_id = %d, ipaddr = %s, loginMask = %d", pRoom->RoomInfo.EncNum, i, j, rec_param[j].enc_id, rec_param[j].stream_id, rec_param[j].ipaddr, pRoom->loginMask);
		j++;
		num++;

		rec_param[j].status 	= 1;
	//	rec_param[j].port 		= 3400;
		//rec_param[j].stream_id 	= j + 1;
		rec_param[j].enc_id = pRoom->RoomInfo.EncInfo[i].ID	 ;  // add zl
		msg_ctrl_inet_ntoa(rec_param[j].ipaddr, pRoom->RoomInfo.EncInfo[i].EncIP);
		pRoom->RoomInfo.EncInfo[i].SD_QuaInfo.MsgQueType = rec_param[j].stream_id;
		pRoom->loginMask |= (1 << rec_param[j].stream_id);
		nslog(NS_INFO, " ---SD_QuaInfo- EncNum = %d, iEnc = %d, jRec = %d, port = %d, stream_id = %d, ipaddr = %s, loginMask = %d", pRoom->RoomInfo.EncNum, i, j, rec_param[j].enc_id, rec_param[j].stream_id, rec_param[j].ipaddr, pRoom->loginMask);
		j++;
		num++;
	}

	nslog(NS_INFO, "i = %d, j = %d, num = %d, EncNum = %d", i, j, num, pRoom->RoomInfo.EncNum);

	pRoom->RoomInfo.req_stream_mask 	= MSG_QUEUE_REC_CLR;
	pRoom->RoomInfo.room_info_mask		= MSG_QUEUE_REC_CLR;
	pRoom->RoomInfo.valid_stream_mask	= MSG_QUEUE_REC_CLR;

	//设置教室信息
	pRoom->handle->set_room_info(rec_param, pRoom->handle, ROOM_STR_NUM);

	if (pRoom->RoomInfo.ConnectStatus == 1) {
		pRoom->RoomInfo.ConnectStatus 	= 0;
		pRoom->RecStatusInfo.ConnStatus = 0;

		//连接ENC
		pRoom->loginMask |= MSG_QUEUE_REC_ALL;
		pRoom->handle->open_room_connect(pRoom->handle);

//		get_req_pass_key_node(&tmp_node, parseXml->proot);
//		get_current_node_value(ret_pass_key, ROOM_MSG_VAL_LEN, parseXml->pdoc, tmp_node);
//
//		//添加到超时队列,等待登录后返回
//		int8_t user_id[ROOM_MSG_VAL_LEN] = {0};
//		upper_msg_get_user_id_value(parseXml, user_id);
//		upper_msg_time_que_add_node(pRoom, MSG_CONNECT_ROOM_REQ, ret_pass_key, user_id, head, upper_msg_connect_time_out_resp);
	} else {
		pRoom->RoomInfo.ConnectStatus 	= 0;
		pRoom->RecStatusInfo.ConnStatus = 0;
	}

	return ROOM_RETURN_SUCC;
}

void upper_msg_get_system_info(RoomMsgEnv  *pRoom)
{
	MsgHeader head;
	int8_t *xml_buf = NULL;


	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ;
	}

	r_bzero(&head, sizeof(MsgHeader));
	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_req_system_info(xml_buf, "0");
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, &head, xml_buf);

	nslog(NS_INFO, " ---[upper_msg_get_system_info] !");

	if (xml_buf)
		r_free(xml_buf);
}

void upper_msg_get_room_info(RoomMsgEnv  *pRoom)
{
	MsgHeader head;
	int8_t *xml_buf = NULL;

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ;
	}

	r_bzero(&head, sizeof(MsgHeader));
	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_req_enc_info(pRoom->RoomInfo.RoomId, xml_buf, MSG_GET_ROOM_INFO, MSG_RECORD_PASS, "0");
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, &head, xml_buf);

	nslog(NS_INFO, " ---[upper_msg_get_system_info] !");

	if (xml_buf)
		r_free(xml_buf);
}

void upper_msg_get_server_info(RoomMsgEnv  *pRoom)
{
	MsgHeader head;
	int8_t *xml_buf = NULL;

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ;
	}

	r_bzero(&head, sizeof(MsgHeader));
	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_req_server_info_xml(pRoom->RoomInfo.RoomId, xml_buf, MSG_GET_SYS_PARAM, MSG_RECORD_PASS, "0");
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, &head, xml_buf);

	if (xml_buf)
		r_free(xml_buf);

}


//deal the request xml msg
int32_t upper_msg_deal_request(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t ret_val		 = ROOM_RETURN_SUCC;
	int32_t msgCodeValue = 0;
	int8_t  passKey[ROOM_MSG_VAL_LEN] 	= {0};
	int8_t	user_id[ROOM_MSG_VAL_LEN]  	= {0};

	msgCodeValue = upper_msg_get_msg_code_value(parseXml);
	if (msgCodeValue == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, " --- get msg code value error!");
		return ROOM_RETURN_FAIL;
	}

	r_bzero(passKey, ROOM_MSG_VAL_LEN);
	if (!upper_msg_get_pass_key_value(parseXml, passKey)) {
		nslog(NS_ERROR, " --- get pass key value error!");
		return ROOM_RETURN_FAIL;
	}

	if (msgCodeValue != 30019)
		nslog(NS_INFO, " >>>>>>>>>>>>>>>>>>msgCodeValue = %d, passKey = %s >>>>>>>>>>>>>>>>", msgCodeValue, passKey);

	//判断超时队列里是否有同样的信令或队列是否已满，有则不处理,直接返回
	upper_msg_get_user_id_value(parseXml, user_id);
	pthread_mutex_lock(&pRoom->msgQueMutex);
	ret_val = upper_msg_get_idle_index(pRoom);
	if (ret_val != -1)
		ret_val = upper_msg_have_same_msgcode(pRoom, msgCodeValue, passKey, user_id, head);
	pthread_mutex_unlock(&pRoom->msgQueMutex);

	if (ret_val == ROOM_RETURN_SUCC || ret_val == -1) {
		nslog(NS_ERROR, " --- have the same msg or queue full, return");
		return ROOM_RETURN_FAIL;
	}

	switch (msgCodeValue) {
		case MSG_GET_ROOM_INFO:
			{
				ret_val = upper_msg_get_room_info_deal(pRoom, parseXml, head, xml);
			}
			break;
		case MSG_SET_ROOM_INFO:
			{
				ret_val = upper_msg_set_room_info_deal(pRoom, parseXml, head, xml);
			}
			break;
		case MSG_SET_QUALITY_INFO:
			{
				ret_val = upper_msg_set_quality_info_deal(pRoom, parseXml, head, xml);
			}
			break;
		case MSG_SET_AUDIO_INFO:
			{
				ret_val = upper_msg_set_audio_info_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_RECORD_CTRL:
			{
				ret_val = upper_msg_record_ctrl_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_REQUEST_CODE_STREAM:
			{
				ret_val = upper_msg_req_code_stream_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_REMOTE_CTRL:
			{
				ret_val = upper_msg_remote_ctrl_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_IFRAMEREQ_CTRL:
			{
				ret_val = upper_msg_iFrame_req_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_CONNECT_ROOM_REQ:
			{
				ret_val = upper_msg_connect_room_req_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_SEND_LOGO_PIC:
			{
				ret_val = upper_msg_send_logo_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_ADD_TITLE_REQ:
			{
				ret_val = upper_msg_add_title_req_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_VOLUME_REQ:
			{
				ret_val = upper_msg_volume_req_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_MUTE_REQ:
			{
				ret_val = upper_msg_mute_req_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_PICADJUST_REQ:
			{
				ret_val = upper_msg_pic_adjust_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_ENC_LOGIN:
			{
				ret_val = upper_msg_enc_login_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_ENC_RESTART:
			{
				ret_val = upper_msg_enc_restart_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_ENC_UPDATE:
			{
				ret_val = upper_msg_enc_update_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSGCODE_GET_CAMCTL_PROLIST:
			{
				ret_val = upper_msg_get_camctl_ctrl_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSGCODE_SET_CAMCTL_PRO:
			{
				ret_val = upper_msg_set_camctl_ctrl_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_ENC_IMAGE_SYNTHESIS:
			{
				ret_val = upper_msg_image_synthesis_req_node(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_ECN_DIRECTOR_MODE:
			{
				ret_val = upper_msg_director_mode_ctrl_deal(pRoom, parseXml, head, xml);
			}
			break;

		default:
			break;
	}

	if (msgCodeValue != 30019)
		nslog(NS_INFO, " <<<<<<<<<<<<<<<<<<msgCodeValue = %d, passKey = %s <<<<<<<<<<<<<<<<<<<<\n", msgCodeValue, passKey);

	return ret_val;
}

//deal the response xml msg
int32_t upper_msg_deal_response(RoomMsgEnv *pRoom, parse_xml_t *parseXml, MsgHeader *head, int8_t *xml)
{
	int32_t ret_val		 = ROOM_RETURN_SUCC;
	int32_t msgCodeValue = 0;

	msgCodeValue = upper_msg_get_msg_code_value(parseXml);
	if (msgCodeValue == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, " --- get msg code value error!");
		return ROOM_RETURN_FAIL;
	}

//	nslog(NS_INFO, "===========rec xml============\n%s\n===========rec xml============", xml);

	switch (msgCodeValue) {
		case MSG_SYSTEM_REQ:
			{
				ret_val = upper_msg_system_info_req_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_GET_ROOM_INFO:
			{
				ret_val = upper_msg_resp_room_info_deal(pRoom, parseXml, head, xml);
			}
			break;

		case MSG_GET_SYS_PARAM:
			{
				ret_val = upper_msg_resp_server_info_deal(pRoom, parseXml, head, xml);
			}
			break;

		default:
			break;
	}

	return ret_val;
}

int32_t upper_msg_analyze(RoomMsgEnv *pRoom, MsgHeader *head, int8_t *xml)
{
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
		nslog(NS_ERROR, " --- init_dom_tree ERROR!");
		goto EXIT;
	}

	if (!parse_xml_user->proot) {
		status = ROOM_RETURN_FAIL;
		nslog(NS_ERROR, " --- parse_xml_user->proot is null!");
		goto EXIT;
	}


	//request msg process
	if(is_req_msg(parse_xml_user->proot)) {
		status = upper_msg_deal_request(pRoom, parse_xml_user, head, xml);
	}

	//response msg process
	if (is_resp_msg(parse_xml_user->proot)) {
		status = upper_msg_deal_response(pRoom, parse_xml_user, head, xml);
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


int32_t upper_msg_deal(RoomMsgEnv *pRoom, MsgHeader *head, int8_t *xml)
{
	nslog(NS_WARN, "\n[%s]\n", xml);
	return upper_msg_analyze(pRoom, head, xml);
}

int32_t upper_msg_recv(RoomMsgEnv *pRoom)
{
	fd_set fdsr;
	int32_t ret = 0;
	int32_t status = ROOM_RETURN_SUCC;
	struct timeval tv;

	MsgHeader msgHead;
	int8_t  *recv_data_buf = NULL;
	UpperMsgEnv *env = &(pRoom->upperEnv);

	pRoom->server_info.time.tv_sec 	= 0;
	pRoom->server_info.time.tv_usec = 0;
	gettimeofday(&pRoom->server_info.time, 0);

	//malloc xml buff
	recv_data_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!recv_data_buf) {
		nslog(NS_ERROR, "--- r_malloc failed!----[%s]", strerror(errno));
		cleanup(ROOM_RETURN_FAIL);
	}

	while(!rCtrlGblGetQuit(&pRoom->glb)) {
		upper_msg_monitor_time_queue(pRoom);
		upper_msg_req_system_info_deal(pRoom);
		lower_msg_monitor_stream_status(pRoom);

		FD_ZERO(&fdsr);
		FD_SET(env->tcp_socket, &fdsr);

		tv.tv_sec  = 1;
		tv.tv_usec = 0;
		//nslog(NS_INFO, "env->tcp_socket:[%d]\n", env->tcp_socket);
		ret = select(env->tcp_socket + 1, &fdsr, NULL, NULL, &tv);
		if (ret < 0) {
			nslog(NS_ERROR, "--- select head error!----[%s]", strerror(errno));
			status = ROOM_RETURN_FAIL;
			break;
		} else if (ret == 0) {
			continue;
		}
		nslog(NS_ERROR,"CCXX  ------- FD : %d\n",env->tcp_socket);
		//recv head
		r_bzero((void *)&msgHead, sizeof(struct MsgHeader));
		ret = recv_long_tcp_data(env->tcp_socket, (void *)&msgHead, sizeof(struct MsgHeader));
		if (ret <= 0) {
			nslog(NS_ERROR, "--- recv_long_tcp_data head error!----[%s]", strerror(errno));
			break;
		}

		msgHead.sLen = ntohs(msgHead.sLen);
		nslog(NS_ERROR,"LEN : %d\n",msgHead.sLen);
		if (msgHead.sLen > ROOM_MSG_MAX_LEN) {
			nslog(NS_ERROR, "msgHead.sLen = %d", msgHead.sLen);
			continue;
		}

		//head checkout
		ret = upper_msg_head_checkout(&msgHead);
		if (ret == ROOM_RETURN_FAIL) {
			nslog(NS_INFO, "---  head checkout error!---");
			continue;
		}

		//recv xml
		r_bzero((void *)recv_data_buf, ROOM_MSG_MAX_LEN);
		ret = recv_long_tcp_data(env->tcp_socket, (void *)recv_data_buf, msgHead.sLen - sizeof(MsgHeader));
		if (ret <= 0) {
			nslog(NS_ERROR, "--- recv_long_tcp_data xml error!----[%s]", strerror(errno));
			break;
		}

		//deal the msg
		upper_msg_deal(pRoom, &msgHead, recv_data_buf);
	}

cleanup:

	if (env->tcp_socket > 0) {
		shutdown(env->tcp_socket, SHUT_RDWR);
		r_close(env->tcp_socket);
	}

	if (recv_data_buf) {
		r_free(recv_data_buf);
		recv_data_buf = NULL;
	}

	return status;
}

int32_t upper_msg_connect(RoomMsgEnv  *roomMsg)
{
	int32_t ret 		=  0;
	int32_t tcp_socket 	= -1;
	int32_t remote_port =  0;
	int8_t  remote_ip[IP_LEN] = {0};

	if (roomMsg == NULL)
		return ROOM_RETURN_FAIL;

	UpperMsgEnv *env = &(roomMsg->upperEnv);

	sockaddr_in_t serv_addr;

	tcp_socket = r_socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_socket < 0) {
		nslog(NS_ERROR, " socket is error!---[%s]", strerror(errno));
		return ROOM_RETURN_FAIL;
	}

	remote_port = env->tcp_port;
	r_strcpy(remote_ip, env->tcp_ip);
	set_net_addr(&serv_addr, remote_ip, remote_port);

	while (!rCtrlGblGetQuit(&roomMsg->glb)) {
		ret = r_connect(tcp_socket, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
		if (ret < 0){
			nslog(NS_ERROR, "--- connect failed!----[%s]", strerror(errno));
			r_sleep(1);
			continue;
		} else {
			break;
		}
	}

	env->tcp_socket = tcp_socket;

	return ROOM_RETURN_SUCC;
}

void upper_msg_get_init_info(RoomMsgEnv  *pRoom)
{
	upper_msg_get_room_info(pRoom);
	upper_msg_get_system_info(pRoom);
	upper_msg_get_server_info(pRoom);
}

void upper_msg_update_room_info_to_ctrl(RoomMsgEnv  *pRoom)
{
	MsgHeader 	head;
	int8_t 		*xml_buf = NULL;
	int8_t 		pass_key[ROOM_MSG_VAL_LEN]	= {0};

	//--fix me...........更新录制状态和录制名称
	if (pRoom->record_handle) {
		pRoom->RoomInfo.RecordStatus = lower_msg_opt_type_trans(pRoom->record_handle->get_record_status(pRoom->record_handle));
		r_strcpy(pRoom->RoomInfo.RcdName, pRoom->record_handle->get_course_root_dir(pRoom->record_handle));
	} else {
		pRoom->RoomInfo.RecordStatus = 0;
		r_strcpy(pRoom->RoomInfo.RcdName, pRoom->RecInfo.CourseName);
	}

	//取本地信息直接回应上报
	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, " ---[r_malloc] error!");
		return ;
	}

	r_bzero(&head, sizeof(MsgHeader));
	r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
	upper_msg_package_resp_room_info(pRoom, xml_buf, MSG_GET_ROOM_INFO, ROOM_RETURN_SUCC, MSG_RECORD_PASS, "0");
	upper_msg_tcp_send(pRoom->upperEnv.tcp_socket, &pRoom->upperEnv.tcp_socket_mutex, &head, xml_buf);

	if (xml_buf)
		r_free(xml_buf);

}

void upper_msg_stop_lives_user_all_unusual(RoomMsgEnv  *pRoom)
{
	if (pRoom->live_handle) {
		nslog(NS_INFO, "stop_lives_user_all_unusual");
		pRoom->live_handle->stop_lives_user_all_unusual(pRoom->live_handle);
	}
}

void *upper_msg_process(void *arg)
{
	int32_t ret = 0, i = 0;
	int8_t  *xml_buf = NULL;
	int32_t get_init_info_status = 1;

	MsgHeader 	head;
	RoomMsgEnv  *roomMsg = (RoomMsgEnv *)arg;
	UpperMsgEnv *upperMsgEnv = &(roomMsg->upperEnv);

//	roomMsg->server_info.DiskAvailableSpace = 102400;

	while(!rCtrlGblGetQuit(&roomMsg->glb)) {
		ret = upper_msg_connect(roomMsg);
		if (ret == ROOM_RETURN_FAIL) {
			nslog(NS_ERROR, "--- connect failed!----[%s]", strerror(errno));
			r_sleep(3);
			continue;
		}
		r_sleep(2);

		//第一次启动，从control 获取信息，重连后，则主动上报信息, 以本地为主
		if (get_init_info_status) {
			upper_msg_get_init_info(roomMsg);
			get_init_info_status = 0;
		} else {
			upper_msg_get_server_info(roomMsg);
			upper_msg_update_room_info_to_ctrl(roomMsg);
		}

		upper_msg_stop_lives_user_all_unusual(roomMsg);

		upper_msg_recv(roomMsg);

		r_sleep(1);
	}

	//停止录制
	if (roomMsg->record_handle) {
		for (i = 0; i < roomMsg->handle->stream_num; i++) {
			roomMsg->handle->set_rec_status(&roomMsg->handle->stream_hand[i], STOP_REC);
			nslog(NS_INFO, "--- set_live_status--- stream_num = %d i = %d", roomMsg->handle->stream_num, i);
		}

		r_strcpy(roomMsg->RecStatusInfo.RecName, roomMsg->record_handle->get_course_root_dir(roomMsg->record_handle));
		r_strcpy(roomMsg->RecInfo.RecordID, roomMsg->record_handle->get_RecordID(roomMsg->record_handle));

		roomMsg->record_handle->record_close(roomMsg->record_handle);
		unregister_course_record_mode(&roomMsg->record_handle);
		roomMsg->record_handle = NULL;

		roomMsg->RecStatusInfo.RecStatus = 0;
		roomMsg->RecInfo.OptType = 0;

		xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
		if (!xml_buf) {
			nslog(NS_ERROR, " ---[r_malloc] error!");
		} else {
			//回复服务端
			r_bzero(&head, sizeof(MsgHeader));
			r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
			upper_msg_package_record_xml(xml_buf, 1, roomMsg->RecStatusInfo.RecName, MSG_ALLPLATFORM_PASSKEY, roomMsg->RecInfo.userid, roomMsg->RecInfo.RecordID, roomMsg->RecInfo.OptType, roomMsg->RecStatusInfo.RecStatus);
			upper_msg_tcp_send(roomMsg->upperEnv.tcp_socket, &roomMsg->upperEnv.tcp_socket_mutex, &head, xml_buf);

			//状态上报
			r_bzero(&head, sizeof(MsgHeader));
			r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
			lower_msg_package_req_status_info(xml_buf, &roomMsg->RecStatusInfo);
			nslog(NS_ERROR,"WAHT ----7\n");
			upper_msg_tcp_send(roomMsg->upperEnv.tcp_socket, &roomMsg->upperEnv.tcp_socket_mutex, &head, xml_buf);

			//解锁分辨率
			r_bzero(xml_buf, ROOM_MSG_MAX_LEN);
			lower_msg_package_record_req(xml_buf, NULL, 0);
			lower_msg_send_xml_to_enc(roomMsg, xml_buf);

			if (xml_buf)
				r_free(xml_buf);
		}
	}

	nslog(NS_WARN, "upper_msg_process exit...");
	return NULL;
}


