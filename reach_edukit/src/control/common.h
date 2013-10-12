/*
 * =====================================================================================
 *
 *       Filename:  common.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2012年11月1日 09时12分18秒
 *       Revision:  
 *       Compiler:  gcc
 *
 *         Author:  黄海洪 
 *        Company:  深圳锐取信息技术股份有限公司
 *
 * =====================================================================================
 */


#ifndef _COMMON_H_
#define _COMMON_H_

#include "zlog.h"
#include "stdint.h"




#define BUFSIZE 8192



typedef struct package
{
	uint32_t version;
	uint32_t len;
	uint32_t reserves;
	uint8_t hash[32];
}Package;

struct route_info{
	 u_int dstAddr;
	 u_int srcAddr;
	 u_int gateWay;
	 char ifName[4];
};




int32_t ParseIDRHeader(uint8_t *pdata);
		
uint32_t getostime();

int32_t ChangeSampleIndex(int32_t index);

void SetEthConfig(uint32_t ipaddr, uint32_t netmask, uint32_t gateway,int8_t *ethdev);

int32_t JoinMacAddr(int8_t *dst, uint8_t *src, int32_t num);

int32_t CheckIPNetmask(int32_t ipaddr, int32_t netmask, int32_t gw);

int32_t write_yuv_420(uint8_t *py, uint8_t *pu, uint8_t *pv, 
								int32_t width, int32_t height, int32_t chid);

int32_t write_yuv_422(uint8_t *py, uint8_t *pu, uint8_t *pv, 
								int32_t width, int32_t height, int32_t chid);


int32_t open_gpio_device();
int32_t get_gpio_value(int32_t fd, uint32_t gnum, int32_t *gvalue);
int32_t set_gpio_value(int32_t fd, uint32_t gnum, int32_t gvalue);


int32_t set_send_timeout(int32_t socket, uint32_t time);
int32_t set_recv_timeout(int32_t socket, uint32_t time);

int32_t tcp_recv_longdata(int32_t sockfd, int8_t* buffer, const int32_t len);
int32_t tcp_send_longdata(int32_t sockfd, int8_t* buffer, const int32_t len);

int32_t set_send_timeout(int32_t socket, uint32_t time);
int32_t set_recv_timeout(int32_t socket, uint32_t time);

int32_t reboot_server();
int32_t reboot_server2();
int32_t check_recvfile(int8_t *file, int32_t fd, uint32_t filelen);

int32_t get_gateway(int8_t *gateway);
//void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo, int8_t *gateway);
int32_t ip_get_proc(int8_t *dev);
uint32_t GetIPaddr(int8_t *interface_name);
uint32_t GetNetmask(int8_t *interface_name);
uint32_t GetBroadcast(int8_t *interface_name);

int32_t set_send_buf(int32_t socket, int32_t length);
int32_t set_recv_buf(int32_t socket, int32_t length);
int32_t ntp_time_sync(int8_t *ipaddr);

int32_t init_watch_dog(int32_t second);
int32_t write_watch_dog(int32_t fd);









#endif
