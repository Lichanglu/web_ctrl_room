#ifndef HD_CRAD_H
#define HD_CARD_H

#define lprintf(X...)	 	do {														\	
		printf("[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);			\
		printf(X);											\
} while(0)


#define HD_EDUKIT_PACKET				("/usr/hd_edukit.bin")
#define HD_REBACK_PATH				("/usr/local/reach/hd_edukit.bin")

#define CTRL_UPDATE_HD_CARD_MSG	0x13
#define CTRL_GET_HD_SYSINFO_MSG		0x14
#define CTRL_GET_HD_REBOOT_MSG		0x15
#define CTRL_UPLOGO_HD_CARD_MSG	0x16
#define CTRL_SINGLE_UPDATE_HD_MSG	0x17
#define CTRL_SYNC_SYSTEM_TIME_MSG	0X18

#define HD_CARD_RETURN_FAIL			-1
#define HD_CARD_RETURN_SUCCESS		1


#define	HD_ENC_IP	"169.254.0.4"


int web_get_hd_version(char* app_version,char* fpga,char *ker,char* build_time);
int ctrl_send_msg_hd_card(int msg_code,void* data,int data_len);
void*  hd_card_pthread(void);

int app_update_HD_card(int8_t *filename);



#endif
