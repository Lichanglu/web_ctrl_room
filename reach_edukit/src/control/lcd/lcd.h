#ifndef _LCD_H_
#define _LCD_H_

#include "stdint.h"

#if 0
typedef struct _params_table_{
	int8_t version[64];				/*�汾*/
	int8_t serial_num[64];			/*���к�*/

	uint32_t ip_addr;				/*IP��ַ*/
	uint32_t gateway;				/*����*/

	uint32_t net_mask;				/*��������*/
	int8_t mac_addr[24];			/*MAC��ַ*/

	uint32_t DiskMaxSpace;			/*�ܿռ�*/
	uint32_t DiskAvailableSpace; 	/*ʣ��ռ�*/
}params_table, *pParams_table;
#endif

int32_t open_lcd_port();

int32_t start_lcd_display_task();
void stop_lcd_display_task(void);

#endif
