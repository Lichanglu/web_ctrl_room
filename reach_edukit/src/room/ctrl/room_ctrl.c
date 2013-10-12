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
#include <netinet/in.h>

#include "room_ctrl.h"
#include "upper_msg_ctrl.h"
#include "lower_msg_ctrl.h"

#include "reach_os.h"
#include "media_msg.h"
#include "stdint.h"
#include "xml_base.h"
#include "xml_msg_management.h"

int ctrl_room_id = 0;
#define	NSLOG_ROOM_CONF			"./nslog.conf"
#define	NSLOG_ROOM_TEST_CONF	"./nslog.conf"
#define NSLOG_RULE_PATH			"/usr/local/reach/room_nslog_rule"
#define NSLOG_RULE_GLOBAL	"default format = \"\%D.\%ms \%V [\%t:\%p:\%f:\%U:\%L] \%m\%n\""
#define NSLOG_REC_RULES 	"Rec_server.INFO   \"/var/log/nodeserver/ns_rec.\%d(\%u).log\", 5MB * 2 ~ \"/var/log/nodeserver/ns_rec.\%d(\%u).#2r.log\""

rGlobalData *glb_data = NULL;

//IP 字符串转成整形
uint32_t msg_ctrl_inet_aton(int8_t *ip_buf, uint32_t *ip_addr)
{
	struct in_addr addr;
	uint32_t ip;

	r_inet_aton(ip_buf, &addr);
	r_memcpy(&ip, &addr, 4);

	if (ip_addr) {
		*ip_addr = ip;
	}

	return ip;
}

//IP 整形转成字符串
int8_t *msg_ctrl_inet_ntoa(int8_t *ip_buf, uint32_t ip_addr)
{
	struct in_addr addr;

	r_memcpy(&addr, &ip_addr, 4);
	r_strcpy(ip_buf, inet_ntoa(addr));

	return ip_buf;
}

void msg_ctrl_clear_msg_queue(int32_t msg_que_id)
{
	msgque 	msgp;
	int32_t ret = 0;

	while(1)
	{
		r_usleep(10000);
		r_bzero(&msgp, sizeof(msgp));
		ret = r_msg_recv(msg_que_id, &msgp, sizeof(msgque), 0, IPC_NOWAIT);
		if (ret == -1) {
			nslog(NS_ERROR, "--- r_msg_recv failed!----[%s]", strerror(errno));
			if (errno == EAGAIN) {
				continue;
			}

			if (errno == ENOMSG) {
				break;
			}

			break;
		} else if (ret == 0) {
			continue;
		}

		//free
		if (msgp.msgbuf) {
			nslog(NS_WARN, "msgp.msgtype = %d", msgp.msgtype);
			r_free(msgp.msgbuf);
			msgp.msgbuf = NULL;
		}

	}

}

int32_t msg_ctrl_create_threads(RoomMsgEnv *msgEnv)
{
	//创建上层消息处理线程
	if(r_pthread_create(&(msgEnv->pUpper), NULL, upper_msg_process, (void *)msgEnv)) {
		nslog(NS_ERROR, "[%s] Failed to create thread: %s", __func__, strerror(errno));
		return ROOM_RETURN_FAIL;
	}

	msgEnv->initMask |= UPPERTHREADCREATED;

	//创建下层消息处理线程
	if(r_pthread_create(&(msgEnv->pLower), NULL, lower_msg_process, (void *)msgEnv)) {
		nslog(NS_ERROR, "[%s] Failed to create thread: %s", __func__, strerror(errno));
		return ROOM_RETURN_FAIL;
	}

	msgEnv->initMask |= LOWERTHREADCREATED;
		
	return ROOM_RETURN_SUCC;
}

int32_t msg_ctrl_join_threads(RoomMsgEnv *msgEnv)
{
	void *ret;
	int32_t status = ROOM_RETURN_SUCC;
	
	if (msgEnv->initMask & UPPERTHREADCREATED) {
		if(pthread_join(msgEnv->pUpper, &ret) == 0) {
			if(ret == (void *)-1) {
				status = ROOM_RETURN_FAIL;
			}
		}
	}

	if (msgEnv->initMask & LOWERTHREADCREATED) {
		if(pthread_join(msgEnv->pLower, &ret) == 0) {
			if(ret == (void *)-1) {
				status = ROOM_RETURN_FAIL;
			}
		}
	}

	return status;
}

int32_t msg_ctrl_init_net(RoomMsgEnv *msgEnv, int32_t roomId)
{
	int32_t fd 				= 0;
	int32_t ret 			= 0;
	int32_t status			= ROOM_RETURN_SUCC;
	int8_t  fileName[RECORD_FILE_MAX_FILENAME] 	= {0};

	sprintf(fileName, "%s/room_%d.xml", FILEPREFIX, roomId);

	parse_xml_t *parse_xml_cfg = (parse_xml_t *)r_malloc(sizeof(parse_xml_t));
	if(NULL == init_file_dom_tree(parse_xml_cfg, fileName)) {
		nslog(NS_ERROR, "[%s] Failed to init_file_dom_tree", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	if(r_strcmp(parse_xml_cfg->proot->name, MSG_ROOM_CFG_KEY)) {
		nslog(NS_INFO, "[%s] --- is not RoomCfg!", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	xmlNodePtr ptr;
	int8_t value[128] = {0};

	//get ipaddr
	r_bzero(value, sizeof(value));
	ptr = get_children_node(parse_xml_cfg->proot, MSG_ROOM_IP_KEY);
	get_current_node_value(value, 128, parse_xml_cfg->pdoc, ptr);
	r_strcpy(msgEnv->upperEnv.tcp_ip, value);
	nslog(NS_INFO, "ipaddr = %s", msgEnv->upperEnv.tcp_ip);

	//get port
	r_bzero(value, sizeof(value));
	ptr = get_children_node(parse_xml_cfg->proot, MSG_ROOM_PORT_KEY);
	get_current_node_value(value, 128, parse_xml_cfg->pdoc, ptr);
	msgEnv->upperEnv.tcp_port = atoi(value);
	nslog(NS_INFO, "port = %d", msgEnv->upperEnv.tcp_port);

cleanup:

	if(parse_xml_cfg->pdoc != NULL) {
		release_dom_tree(parse_xml_cfg->pdoc);
	}

	if(parse_xml_cfg) {
		r_free(parse_xml_cfg);
	}

	return status;
}

int32_t msg_ctrl_init_env(RoomMsgEnv *msgEnv, int32_t roomId)
{
	int32_t i = 0;
	int32_t status = ROOM_RETURN_SUCC;

	//init mutex	
	pthread_mutex_init(&msgEnv->upperEnv.tcp_socket_mutex, NULL);
	pthread_mutex_init(&msgEnv->msgQueMutex, NULL);
	pthread_mutex_init(&msgEnv->glb.mutex, NULL);

	//init net env
	status = msg_ctrl_init_net(msgEnv, roomId);
	if (status == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, "[%s] msg_ctrl_init_net error", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	//create msg que
	msgEnv->lowerEnv.msg_id[ROOMMSGID] = r_msg_create_u(MSG_ROOM_CTRL_BASE_KEY+roomId);
	if (msgEnv->lowerEnv.msg_id[ROOMMSGID] == -1) {
		nslog(NS_ERROR, "[%s] msg create error", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	return status;

cleanup:

	return status;
}

void msg_ctrl_destroy_env(RoomMsgEnv *msgEnv)
{
	int32_t i = 0, ret = -1;
	int32_t status = 0;

	//remove msg
	ret = msgctl(msgEnv->lowerEnv.msg_id[ROOMMSGID], IPC_RMID, NULL);
	if (ret < 0) {
		nslog(NS_ERROR, "[%s]remove msg error!------%s", __func__, strerror(errno));
	}

	//destroy mutex
	pthread_mutex_destroy(&msgEnv->upperEnv.tcp_socket_mutex);
	pthread_mutex_destroy(&msgEnv->msgQueMutex);
	pthread_mutex_destroy(&msgEnv->glb.mutex);

	return;
}

void msg_ctrl_init_room_info(RoomMsgEnv *pRoom, int32_t roomId)
{
	int32_t ret_val					= 0;
	int8_t  *xml_buf 				= NULL;
	int8_t  file[ROOM_MSG_VAL_LEN] 	= {0};

	xml_buf = (int8_t *)r_malloc(ROOM_MSG_MAX_LEN);
	if (!xml_buf) {
		nslog(NS_ERROR, "[%s] ---[r_malloc] error!", __func__);
		return ;
	}

	sprintf(file, "%s_%d.xml", MSG_ROOM_CONF_FILE_PREFIX, pRoom->RoomInfo.RoomId);	
	ret_val = upper_msg_read_file(file, xml_buf);
	if (ret_val) {
		nslog(NS_ERROR, "[%s] ---[upper_msg_read_file] error!", __func__);

		if (xml_buf)
			r_free(xml_buf);

		return ;
	}

	lower_msg_load_room_info(pRoom, xml_buf);
	pRoom->RoomInfo.RoomId = roomId;

	if (xml_buf)
		r_free(xml_buf);
}

void msg_ctrl_exit_service(int32_t signal)
{
	if (signal == SIGUSR2) {
		if (glb_data) {
			nslog(NS_WARN, "msg_ctrl_exit_service............");
			rCtrlGblSetQuit(glb_data);
		}
	}
}

int32_t msg_ctrl_run_service(int32_t roomId)
{
	int32_t ret = 0, i = 0;
	int32_t status 			= ROOM_RETURN_SUCC;
	RoomMsgEnv *msgCtrlEnv 	= NULL;

	msgCtrlEnv = r_malloc(sizeof(RoomMsgEnv));
	if (!msgCtrlEnv) {
		nslog(NS_ERROR, "r_malloc [%s]", strerror(errno));
		return ROOM_RETURN_FAIL;
	}

	r_bzero(msgCtrlEnv, sizeof(RoomMsgEnv));

	//VERSION
	nslog(NS_INFO, "ROOM_VERSION = %s", ROOM_VERSION);	

	//初始化Media 库
	MediaSysInit();

	//初始化XML 
	xmlInitParser();

//	//初始化教室信息
//	msg_ctrl_init_room_info(&msgCtrlEnv, roomId);

	//设置教室ID
	msgCtrlEnv->RoomInfo.RoomId = roomId;
	msgCtrlEnv->RecStatusInfo.RoomID 	= msgCtrlEnv->RoomInfo.RoomId;

	/*取消PIPE坏的信号*/
	Signal(SIGPIPE, SIG_IGN);

	//初始化消息控制参数
	ret = msg_ctrl_init_env(msgCtrlEnv, roomId);
	if(ret == ROOM_RETURN_FAIL) {
		nslog(NS_ERROR, "[%s] msg_ctrl_init_env error", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}
#if 1
	//注册直播模块
	msgCtrlEnv->live_handle  = register_room_lives_module(roomId);
	if (!msgCtrlEnv->live_handle) {
		nslog(NS_ERROR, "[%s] register_room_lives_module error", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	msgCtrlEnv->lowerEnv.msg_id[LIVEMSGID] = msgCtrlEnv->live_handle->lives_mode_info.msgid;
#endif
#if 1

	//注册接收模块
	msgCtrlEnv->handle = register_room_receive_module(ROOM_STR_NUM);   // add zl
	if (!msgCtrlEnv->handle) {
		nslog(NS_ERROR, "[%s] register_room_receive_module error", __func__);
		cleanup(ROOM_RETURN_FAIL);
	} else {
		for (i = 0; i < msgCtrlEnv->handle->stream_num; i++) {
			nslog(NS_ERROR ,"FUCK -- SHIRT  -- %d\n",msgCtrlEnv->lowerEnv.msg_id[ROOMMSGID]);
			msgCtrlEnv->handle->set_recv_to_ctrl_msgid(msgCtrlEnv->lowerEnv.msg_id[ROOMMSGID], &msgCtrlEnv->handle->stream_hand[i]);
			msgCtrlEnv->handle->set_recv_to_live_msgid(msgCtrlEnv->lowerEnv.msg_id[LIVEMSGID], &msgCtrlEnv->handle->stream_hand[i]);

			msgCtrlEnv->handle->set_live_status(&msgCtrlEnv->handle->stream_hand[i], DISCONNECT);
			msgCtrlEnv->handle->set_rec_status(&msgCtrlEnv->handle->stream_hand[i], DISCONNECT);
		}
	}
#endif

	rCtrlGblSetRun(&msgCtrlEnv->glb);

	//创建消息处理线程
	ret = msg_ctrl_create_threads(msgCtrlEnv);
	if(ret == ROOM_RETURN_FAIL) {		
		nslog(NS_ERROR, "[%s] msg_ctrl_create_threads error", __func__);
		cleanup(ROOM_RETURN_FAIL);
	}

	//安装退出信号
	glb_data = &msgCtrlEnv->glb;
	Signal(SIGUSR2, msg_ctrl_exit_service);

	while(!rCtrlGblGetQuit(&msgCtrlEnv->glb)) {
		sleep(1);
	}
	
cleanup:
	
	nslog(NS_WARN, "room will exit...");

	//exit lower pthread
	msgque quit_msg;
	r_bzero(&quit_msg, sizeof(quit_msg));
	quit_msg.msgtype = 1;
	r_msg_send(msgCtrlEnv->lowerEnv.msg_id[ROOMMSGID], &quit_msg, MsgqueLen - sizeof(int64_t), 0);
	nslog(NS_WARN, "r_msg_send msg to lower msg queue...");

	msg_ctrl_join_threads(msgCtrlEnv);
	nslog(NS_WARN, "msg_ctrl_join_threads...");

	unregister_room_receive_module(msgCtrlEnv->handle);
	nslog(NS_WARN, "unregister_room_receive_module...");

	unregister_room_lives_module(msgCtrlEnv->live_handle);
	nslog(NS_WARN, "unregister_room_lives_module...");

	msg_ctrl_clear_msg_queue(msgCtrlEnv->lowerEnv.msg_id[ROOMMSGID]);
	nslog(NS_WARN, "msg_ctrl_clear_msg_queue...");

	msg_ctrl_destroy_env(msgCtrlEnv);
	nslog(NS_WARN, "msg_ctrl_destroy_env...");

	xmlCleanupParser();
	nslog(NS_WARN, "xmlCleanupParser...");

	nslog(NS_WARN, "[msg_ctrl_run_service] exit.........");

	if (msgCtrlEnv)
		r_free(msgCtrlEnv);

	return status;
}


/*
##子进程捕捉回调函数
*/
void msg_ctrl_sig_child(int32_t signo)
{
	pid_t	pid;
	int32_t 	stat;

	while((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		nslog(NS_ERROR, " child %d terminated", pid);

		pid = fork();
		if(pid < 0) {
			nslog(NS_ERROR, "[%s] fork error: %s", __func__, strerror(errno));
			exit(1);
		}

		usleep(1000000);

		if(0 == pid) {
			//child
			usleep(1000000);
			msg_ctrl_run_service(ctrl_room_id);
		}
	}

	return;
}

int32_t msg_ctrl_mk_rules_file(int8_t *rule_file, int8_t *rule_cname, int32_t room_id)
{
	int32_t ret_val 		= 0;
	int8_t 	rule_buf[1024]  = {0};
	int8_t  mkdir_cmd[128]	= {0};

	if (!rule_file || !rule_cname) {
		printf("error : rule_file = %p, rule_cname = %p, r_strlen(rule_buf) = %d\n", rule_file, rule_cname, r_strlen(rule_buf));
		return -1;
	}

	sprintf(mkdir_cmd, "mkdir -p %s", NSLOG_RULE_PATH);
	r_system(mkdir_cmd);

	sprintf(rule_buf, "[global]\n%s\n[levels]\nINFO = 40, LOG_INFO\n[rules]\n%s\n%s.DEBUG	 \"/var/log/nodeserver/ns_room_%d.%%d(%%u).log\", 5MB * 2 ~ \"/var/log/nodeserver/ns_room_%d.%%d(%%u).#2r.log\"\n",\
					   NSLOG_RULE_GLOBAL,\
					   NSLOG_REC_RULES,\
					   rule_cname,\
					   room_id, \
					   room_id);

	ret_val = upper_msg_write_file(rule_file, rule_buf);
	if (ret_val == -1) {
		printf("upper_msg_write_file error\n");
		return -1;
	}

	return 0;
}

int32_t  main(int32_t argc, uint8_t **argv)
{
	int32_t rc;
	int32_t roomId 			= 0;
	int8_t  nslog_path[128] = {0};
	int8_t  nslog_cname[32]	= {0};
		
	if (argc != 2) {
		printf("Usage: %s room_id\n", argv[0]);
		return -1;
	}

	ctrl_room_id = atoi(argv[1]);

	sprintf((char *)nslog_path, "%s/nslog_room_%d.DEBUG", NSLOG_RULE_PATH, ctrl_room_id);
	sprintf((char *)nslog_cname, "nslog_room_%d", ctrl_room_id);

	rc = dzlog_init(NSLOG_ROOM_CONF, "nslog_room");
	if (rc) {
		printf("dzlog_init failed : <%s><%s>\n", nslog_path, nslog_cname);
		msg_ctrl_mk_rules_file(nslog_path, nslog_cname, ctrl_room_id);
		rc = dzlog_init(nslog_path, nslog_cname);
		if(rc) {
			printf("dzlog_init failed !!!<%s><%s>\n", nslog_path, nslog_cname);
			return -1;
		}
	}

#if 1
	msg_ctrl_run_service(ctrl_room_id);
#else
	pid_t pid = 0;
	pid = fork();
	if(pid < 0) {
		nslog(NS_ERROR, "fork error %s", strerror(errno));
		exit(1);
	}

	if(0 == pid)  {    //child
		msg_ctrl_run_service(ctrl_room_id);
	}  else {
		Signal(SIGCHLD, msg_ctrl_sig_child);

		while(1) {
			sleep(1);
		}
	}
#endif

	zlog_fini();
	return 0;
}

