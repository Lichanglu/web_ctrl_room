
#include "media_msg.h"
#include "reach_os.h"
//#include "node_cfg.h"
/*消息头打包*/
void pack_header_msg(void *data, uint8_t type, uint16_t len , uint16_t m_persist_data)
{
	msg_header_t  *p;
	p = (msg_header_t *)data;
	r_memset(p, 0x00, sizeof(msg_header_t));
	p->m_data = m_persist_data;
	p->m_len = htons(len);
	p->m_ver = MSG_VER;
	p->m_msg_type = type;
}

void pack_header_msg_xml(void *data, uint16_t len , uint16_t ver)
{
	msg_header_t  *p;
	p = (msg_header_t *)data;
	r_memset(p, 0x00, sizeof(msg_header_t));
	p->m_len = htons(len + MSG_HEAD_LEN);
	p->m_ver = htons(ver);
}

void pack_header_data(void *data, uint8_t type, uint16_t len)
{
	msg_header_t  *p;
	p = (msg_header_t *)data;
	r_memset(p, 0x00, sizeof(msg_header_t));
	p->m_len = htons(len);
	p->m_msg_type = type;
}

void pack_unite_user_info(net_user_info_t *nu)
{
	nu->uflag =  USER_FLAG_UNITE;
	//nu->uflag =  USER_FLAG_PLAYER;
	r_strcpy(nu->uname, USER_NAME);
	r_strcpy(nu->upasswd, UNITE_PASSWD);
	//r_strcpy(nu->upasswd, USER_PASSWD);
}





int32_t write_file(char *file, void *buf, int buflen)
{
	static FILE *fp = NULL;

	if(NULL == fp) {
		fp =	fopen(file, "w");

		if(NULL == fp) {
			nslog(NS_ERROR, "fopen  : %s", strerror(errno));
			return -1;
		}
	}

	nslog(NS_INFO, "[write_file] buf : [%p], buflen : [%d] , fp : [%p]\n", buf, buflen, fp);
	fwrite(buf, buflen, 1, fp);
	return 0;
}



void init_channel_infos(channel_info_t *cit)
{
	cit->m_channel = -1;
	cit->m_data = NULL;
	cit->m_data_len = 0;
	cit->m_platform_flag = -1;
	cit->m_msg_type = -1;
}
#if 0
int32_t get_specified_ip(int8_t *ip)
{
	int8_t *ifname = NULL;
	ifname = get_node_cfg_NetCard();

	if(NULL == ifname) {
		nslog(NS_INFO, "get cfg netcard failed!!!\n");
		return -1;
	}

	if(get_local_ip(ifname, ip) < 0) {
		nslog(NS_INFO, "[get_specified_ip] ----error----- [%s:%s]\n", ifname, ip);
		return -1;
	}

	return 0;
}
#endif
// ************************ BEYONSYS PLATFROM **************************************
int32_t check_net_user_beyonsys(net_user_info_t *nu)
{
	if(USER_FLAG_PLAYER == nu->uflag) {
		if(r_strcmp((const int8_t *)nu->uname, (const int8_t *)USER_NAME) == 0 &&
		   r_strcmp((const int8_t *)nu->upasswd, (const int8_t *)USER_PASSWD) == 0) {
			return USER_LOGIN_VALID;
		} else {
			return USER_LOGIN_INVALID;
		}
	}

	return USER_LOGIN_INVALID;
}
// ************************ POWERLIVE PLATFROM  ***********************************
int32_t check_net_user_powerlive(int8_t *password , int32_t password_len)
{
	if(NULL == password) {
		return USER_LOGIN_INVALID;
	}

	nslog(NS_INFO, "[%s]-_____--[%s]---[%d]\n", __func__, password, password_len);

	if(!(r_strncmp(LIVE_CONNECT_KEY_1, password, password_len)) ||
	   !(r_strncmp(LIVE_CONNECT_KEY_2, password, password_len))) {
		return USER_LOGIN_VALID;
	}

	return USER_LOGIN_INVALID;
}

int32_t check_vod_user_powerlive(int8_t *password , int32_t password_len)
{
	if(NULL == password) {
		return USER_LOGIN_INVALID;
	}

	//nslog(NS_INFO, "[%s]-_____--[%s]---[%d]\n", __func__, password, password_len);

	if(!(r_strncmp(VOD_CONNECT_KEY_1, password, password_len)) ||
	   !(r_strncmp(VOD_CONNECT_KEY_2, password, password_len))) {
		return USER_LOGIN_VALID;
	}

	return USER_LOGIN_INVALID;
}

void get_sun_path(int8_t *sun_path, int8_t *base_sun_path, int32_t index)
{
	sprintf(sun_path, "%s%d", base_sun_path, index);
}

