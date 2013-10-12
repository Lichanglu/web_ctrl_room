#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "stdint.h"

#define FLAG_CLR			0x01
#define FLAG_RESET			0x02
#define FLAG_WAYLEFT		0x05
#define FLAG_WAYRIGHT		0x06
#define FLAG_HIDECURSOR		0x0C
#define FLAG_SHOWCURSOR		0x0F
#define FLAG_MOVELEFT		0x10
#define FLAG_MOVERIGHT		0x14
#define FLAG_SCREENLEFT		0x18
#define FLAG_SCREENRIGHT	0x1C
#define FLAG_SETCURSOR		0x80
#define FLAG_LIGHTSHOW		0x28
#define FLAG_LIGHTHIDE		0x2C

#define FLAG_UP				0xB8
#define FLAG_DOWN			0xE8
#define FLAG_ENTER			0xD8
#define FLAG_ESCAPE			0x78

#define FLAG_NULL			0x1
#define FLAG_CENTER			0x2
#define FLAG_LEFT			0x4
#define FLAG_RIGHT			0x8
#define FLAG_NEWLINE		0x10

#define ONE_LINE_POS		0x00
#define TWO_LINE_POS		0x40

#define MAX_DISPLAY_CHAR	16

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE  0
#endif


int32_t init_com_port(int32_t port_num, int32_t baudrate, int32_t databits, int32_t stopbits, int32_t parity);

int32_t send_order(int32_t fd, int32_t nFlag, int32_t nParam);

int32_t send_data(int32_t fd, int8_t *pdata, int32_t uflag);

void close_com_port(int32_t fd);


#endif

