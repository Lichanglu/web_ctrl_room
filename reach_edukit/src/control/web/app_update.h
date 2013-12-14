#ifndef _APP_UPDATE_H__
#define _APP_UPDATE_H__


#define SET_GPIO (0x55555555)
#define GET_GPIO (0xAAAAAAAA)


#define GPIO_4			4
#define GPIO_5			5
#define GPIO_6			6
#define GPIO_7			7
#define GPIO_8			8
#define GPIO_9			9

int updatefpga(void);
int get_is_update_fpga();
int updatekernel(void);

#endif

