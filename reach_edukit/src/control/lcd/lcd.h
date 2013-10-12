#ifndef _LCD_H_
#define _LCD_H_

#include "stdint.h"

#if 0
typedef struct _params_table_{
	int8_t version[64];				/*版本*/
	int8_t serial_num[64];			/*序列号*/

	uint32_t ip_addr;				/*IP地址*/
	uint32_t gateway;				/*网关*/

	uint32_t net_mask;				/*外网掩码*/
	int8_t mac_addr[24];			/*MAC地址*/

	uint32_t DiskMaxSpace;			/*总空间*/
	uint32_t DiskAvailableSpace; 	/*剩余空间*/
}params_table, *pParams_table;
#endif

int32_t open_lcd_port();

int32_t start_lcd_display_task();
void stop_lcd_display_task(void);

#endif
