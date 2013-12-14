/*
 * =====================================================================================
 *
 *       Filename:  common.c
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <termios.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <linux/netlink.h>
#include <netinet/ether.h>
#include <linux/rtnetlink.h>
#include <netdb.h>
#include <linux/watchdog.h>


#include "common.h"
#include "control_log.h"
#include "reach_os.h"
#include "lcd.h"
#include "sercenterctrl.h"



#define SET_GPIO	(0x55555555)
#define GET_GPIO	(0xAAAAAAAA)

#define WATCH_DOG_DEV  ("/dev/watchdog")


#if 0
typedef struct _R_GPIO_data_{
	uint32_t gpio_num;
	uint32_t gpio_value;
}R_GPIO_data;
#endif


int32_t FindH264StartNAL(uint8_t *pp)
{
	/*is for 00 00 00 01 Nal header*/
	if(pp[0] != 0 || pp[1] != 0 || pp[2] != 0 || pp[3] != 1) {
		return 0;
	} else {
		return 1;
	}
}


int32_t ParseIDRHeader(uint8_t *pdata)
{
	int32_t len = 0x17;
	int32_t ret = 0, I_frame_header_length = 0;

	uint8_t *find = pdata;

	while(find) {
		ret = FindH264StartNAL(find);

		if(ret) {
			I_frame_header_length++ ;

			if(I_frame_header_length >= 3) {
				break;
			} else {
				find += 3;    //find next NAL header
			}
		}

		find++;
	}

	len = (int32_t)(find - pdata);

	if(len > 0x40) {
		len = 0x18;
	}

	return len;
}

uint32_t getostime()
{
	struct timeval tv;
	struct timezone tz;
	uint32_t ultime;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return (ultime);
}

/*设置网络IP,掩码,网关*/
void SetEthConfig(uint32_t ipaddr, uint32_t netmask, uint32_t gateway,int8_t *ethdev)
{
	int32_t ret;
	struct in_addr addr1, addr2;
	int8_t temp[200];

	r_memset(temp, 0, sizeof(temp));
	r_memcpy(&addr1, &ipaddr, 4);
	r_memcpy(&addr2, &netmask, 4);
	r_strcpy(temp, (int8_t*)"ifconfig ");
	r_strcat(temp, ethdev);
	r_strcat(temp, (int8_t*)" ");
	r_strcat(temp, (int8_t*)inet_ntoa(addr1));
	r_strcat(temp, (int8_t*)" netmask ");
	r_strcat(temp, (int8_t*)inet_ntoa(addr2));
	zlog_info(DBGLOG, "command:%s\n", temp);

	ret = r_system(temp);

	if(ret < 0) {
		zlog_info(DBGLOG, "ifconfig %s up ERROR\n", inet_ntoa(addr1));
	}

	r_memset(temp, 0, sizeof(temp));
	r_memcpy(&addr1, &gateway, 4);
	r_strcpy(temp, (int8_t*)"route add default gw ");
	r_strcat(temp, (int8_t*)inet_ntoa(addr1));
	r_strcat(temp, (int8_t*)" dev ");
	r_strcat(temp, ethdev);
	zlog_info(DBGLOG, "command:%s\n", temp);

	ret = r_system(temp);

	if(ret < 0) {
		zlog_error(DBGLOG, "set gateway error\n");
	}
}

int32_t ChangeSampleIndex(int32_t index)
{
	int32_t ret = 44100;

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

int32_t JoinMacAddr(int8_t *dst, uint8_t *src, int32_t num)
{
	sprintf((char *)dst, "%02x:%02x:%02x:%02x:%02x:%02x", src[0],
	        src[1], src[2], src[3], src[4], src[5]);

	return 0;
}


int32_t CheckIPNetmask(int32_t ipaddr, int32_t netmask, int32_t gw)
{
	int32_t mask, ip, gateip;
	mask = netmask;
	mask = htonl(mask);
	ip = ipaddr;
	ip = htonl(ip);
	gateip = gw;
	gateip = htonl(gateip);

	if((((ip & 0xFF000000) >> 24) > 223) || ((((ip & 0xFF000000) >> 24 == 127)))) {
		return 0;
	}

	if(((ip & 0xFF000000) >> 24) < 1) {
		return 0;
	}

	if((((gateip & 0xFF000000) >> 24) > 223) || (((gateip & 0xFF000000) >> 24 == 127))) {
		return 0;
	}

	if(((gateip & 0xFF000000) >> 24) < 1) {
		return 0;
	}

	if((ip & mask) == 0) {
		return 0;
	}

	if((ip & (~mask)) == 0) {
		return 0;
	}

	if((~(ip | mask)) == 0) {
		return 0;
	}

	while(mask != 0) {
		if(mask > 0) {
			return 0;
		}

		mask <<= 1;
	}

	return 1;
}

int32_t open_gpio_device()
{
	return open("/dev/Rgpio", O_RDWR);
}

int32_t get_gpio_value(int32_t fd, uint32_t gnum, int32_t *gvalue)
{
	int32_t ret = 0;
	R_GPIO_data gpio_st;

	if(fd < 0){
		fprintf(stderr, "get_gpio_value failed, gpio fd error!");
		return -1;
	}

	gpio_st.gpio_num = gnum;
	ret = ioctl(fd, GET_GPIO, &gpio_st);
	if(ret < 0){
		fprintf(stderr, "get gpio error!\n");
		return -1;
	}

	*gvalue = gpio_st.gpio_value;

	return 0;
}


int32_t set_gpio_value(int32_t fd, uint32_t gnum, int32_t gvalue)
{
	int32_t ret = 0;
	R_GPIO_data gpio_st;

	if(fd < 0){
		fprintf(stderr, "set_gpio_value failed, gpio fd error!");
		return -1;
	}

	if(gvalue != 0){
		gvalue = 1;
	}

	gpio_st.gpio_num = gnum;
	gpio_st.gpio_value = gvalue;

	ret = ioctl(fd, SET_GPIO, &gpio_st);
	if(ret < 0){
		fprintf(stderr, "set gpio error!\n");
		return -1;
	}

	return 0;
}

int32_t write_yuv_420(uint8_t *py, uint8_t *pu, uint8_t *pv,
                  int32_t width, int32_t height, int32_t chid)
{
	uint8_t *pY = py;
	uint8_t *pU = pu;
	uint8_t *pV = pv;

	uint8_t i;
#if 0
	pY = addr;
	pU = addr + width * height; //2088960;//width*height;
	pV = addr + width * height + 1; //2088961;//width*height+1;
#endif

	int8_t buf[256] = {0};
	static int32_t time0 = 0;
	static int32_t time1 = 0;
	static int32_t time2 = 0;

	if(chid == 0) {
		sprintf((char *)buf, "picture_yuv420_vp0_%d.yuv", time0++);
	} else if(chid == 1) {
		sprintf((char *)buf, "picture_yuv420_vp1_%d.yuv", time1++);
	} else {
		sprintf((char *)buf, "picture_yuv420_mp_%d.yuv", time2++);
	}


	FILE *fp = fopen((char *)buf, "w+");

	if(!fp) {
		return -1;
	}

	for(i = 0; i < height * width; i++) {
		fputc(*pY, fp);
		pY++;
	}

	for(i = 0; i < height * width / 4; i++) {
		fputc(*pU, fp);
		pU++;
		pU++;
	}

	for(i = 0; i < height * width / 4; i++) {
		fputc(*pV, fp);
		pV++;
		pV++;
	}

	fclose(fp);
	fprintf(stderr, "write over  height=%d   width=%d\n", height, width);

	return 1;

}

int32_t write_yuv_422(uint8_t *py, uint8_t *pu, uint8_t *pv,
                  int32_t width, int32_t height, int32_t chid)
{
	uint8_t *pY = py;
	uint8_t *pU = pu;
	uint8_t *pV = pv;

	uint32_t i;
#if 0
	pY = addr;
	pU = addr + width * height; //2088960;//width*height;
	pV = addr + width * height + 1; //2088961;//width*height+1;
#endif

	int8_t buf[256] = {0};
	static int32_t time0 = 0;
	static int32_t time1 = 0;
	static int32_t time2 = 0;

	if(chid == 0) {
		sprintf((char *)buf, "picture_yuv422_vp0_%d.yuv", time0++);
	} else if(chid == 1) {
		sprintf((char *)buf, "picture_yuv422_vp1_%d.yuv", time1++);
	} else {
		sprintf((char *)buf, "picture_yuv422_mp_%d.yuv", time2++);
	}


	FILE *fp = fopen((char *)buf, "w+");

	if(!fp) {
		return -1;
	}

	for(i = 0; i < height * width; i++) {
		fputc(*pY, fp);
		pY++;
	}

	for(i = 0; i < height * width / 2; i++) {
		fputc(*pU, fp);
		pU++;
		pU++;
	}

	for(i = 0; i < height * width / 2; i++) {
		fputc(*pV, fp);
		pV++;
		pV++;
	}

	fclose(fp);
	fprintf(stderr, "write over  height=%d	 width=%d\n", height, width);

	return 1;

}

int32_t set_send_timeout(int32_t socket, uint32_t time)
{
	struct timeval timeout;
	int32_t ret = 0;

	timeout.tv_sec = time;
	timeout.tv_usec = 0;

	ret = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO,& timeout, sizeof(timeout));
	if(ret == -1){
		zlog_error(DBGLOG, "set_send_timeout failed!\n");
	}

	return ret;
}

int32_t set_recv_timeout(int32_t socket, uint32_t time)
{
	 struct timeval timeout;
	 int32_t ret = 0;

	 timeout.tv_sec = time;
	 timeout.tv_usec = 0;

	 ret = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	 if(ret < 0){
		zlog_error(DBGLOG, "set_recv_timeout failed!\n");
	 }

	 return ret;
}

int32_t set_send_buf(int32_t socket, int32_t length)
{
	int32_t buflen = 0;
	int32_t ret = 0;

	buflen = length;

	ret = r_setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&buflen , sizeof(buflen));
	if(ret == -1){
		zlog_error(DBGLOG, "set_send_buf failed!\n");
	}

	return ret;
}

int32_t set_recv_buf(int32_t socket, int32_t length)
{
	int32_t buflen = 0;
	int32_t ret = 0;

	buflen = length;

	ret = r_setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char *)&buflen , sizeof(buflen));
	if(ret == -1){
		zlog_error(DBGLOG, "set_recv_buf failed!\n");
	}

	return ret;
}



int32_t tcp_recv_longdata(int32_t sockfd, int8_t* buffer, const int32_t len)
{
	int32_t total_recv = 0;
	int32_t nrecv_len = 0;

	while(total_recv < len){
		nrecv_len = recv(sockfd, buffer + total_recv, len - total_recv, 0);
		if(nrecv_len < 1){
			if(errno != EAGAIN)
				zlog_error(DBGLOG, "tcp_recv_longdata failed, nRecvLen = %d, errno = %d, err msg:%s\n",
									nrecv_len, errno, strerror(errno));
			if(errno == EAGAIN)
				continue;

			return nrecv_len;
		}

		total_recv += nrecv_len;
		if(total_recv == len)
			break;
	}

	return total_recv;
}


int32_t tcp_send_longdata(int32_t sockfd, int8_t* buffer, const int32_t len)
{
	int32_t total_send = 0;
	int32_t nsend_len = 0;

	while(total_send < len){
		nsend_len = send(sockfd, buffer + total_send, len - total_send, 0);
		if(nsend_len < 1){
			zlog_error(DBGLOG, "tcp_send_longdata failed, err msg:%s\n", strerror(errno));
			return -1;
		}

		total_send += nsend_len;
		if(len == total_send)
			break;
	}

	return total_send;
}

int32_t reboot_server()
{
	/* TODO: 具体重启方案? */
	r_system((const int8_t *)"sync");
//	stop_lcd_display_task();
	sleep(3);
	r_system((const int8_t *)"reboot -f");
	usleep(200000);

	return 0;
}

int32_t reboot_server2()
{
	/* TODO: 具体重启方案? */
	r_system((const int8_t *)"sync");

	printf(" <GOD ++++++++++++++++++++++++++++++ 4> MSG_SYSREBOOT\n");
	//清空前面板
//	ClearStateLed();

	printf(" <GOD ++++++++++++++++++++++++++ 5> MSG_SYSREBOOT\n");
	sleep(5);
	r_system((const int8_t *)"reboot -f");

	printf(" <GOD +++++++++++++++++++++++++ 6> MSG_SYSREBOOT\n");
	usleep(200000);

	printf(" <GOD ++++++++++++++++++++++++++ 7> MSG_SYSREBOOT\n");
	return 0;
}


int32_t check_recvfile(int8_t *updatefile, int32_t fd, uint32_t filelen)
{
	int8_t szData[4096] = {0};
	uint32_t ulen = 0;
	FILE *file = NULL;
	Package *pack = NULL;
	uint32_t type = 0;

	if(filelen <= sizeof(Package))
	{
		return -1;
	}

#if 1
	ulen = tcp_recv_longdata(fd, (int8_t *)szData, sizeof(Package));
	if(ulen != sizeof(Package))
	{
		return -1;
	}
	pack = (Package *)szData;
	printf("version = %u %u %u %u\n",pack->version,pack->len,pack->reserves,ulen);

	//8168升级包
	if((pack->version == 2012) || (pack->len == sizeof(Package)))
	{
		type = 0;
	}

	//6467升级包
	else if ((szData[0] == 0x7e)&&(szData[1] == 0x7e)&&(szData[2] == 0x7e) && (szData[3] == 0x7e)
							&& (szData[4] == 0x48) && (szData[5] == 0x45) && (szData[6] == 0x4e) && (szData[7] == 0x43))
	{
		type = 1;
	}
	else
	{
		return -1;
	}
#endif

	filelen = filelen - ulen;
	printf("---->len [%u]\n",filelen);
	file = fopen((const char *)updatefile, "w");
	if(NULL == file)
	{
		return -1;
	}

	//6467包
	if(type == 1)
	{
		fwrite(&szData[0],1,sizeof(Package),file);
	}

	while(filelen > 0)
	{
		if(filelen >= 4096)
		{
			ulen = 4096;
		}
		else
		{
			ulen = filelen;
		}


		ulen = recv(fd, (int8_t *)szData, ulen, 0);
		if(ulen <= 0)
		{
			fclose(file);
			unlink("/var/tmp/update.tar.gz");
			return -1;
		}
		filelen = filelen - ulen;
		fwrite(szData,1,ulen,file);

	}

	printf("---->len1111 [%u]\n",filelen);
	fclose(file);


	return type;
}

#if 0
//获取默认网关
int32_t get_gateway(int8_t *gateway)
{
	struct nlmsghdr *nlMsg;
	struct rtmsg *rtMsg;
	struct route_info *rtInfo;
	int8_t msgBuf[BUFSIZE];
	struct in_addr	addr ;
	uint32_t 	value;
	uint32_t   dwGateWay;

	int32_t sock, len, msgSeq = 0;

	/* Create Socket */
	if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) <= 0) {
		perror("Socket Creation: ");
	}

	/* Initialize the buffer */
	memset(msgBuf, 0, BUFSIZE);

	/* point the header and the msg structure pointers into the buffer */
	nlMsg = (struct nlmsghdr *)msgBuf;
	rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

	/* Fill in the nlmsg header*/
	nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
	nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .

	nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
	nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
	nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.

	/* Send the request */
	if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) <= 0) {
		printf("Write To Socket Failed...\n");
		return -1;
	}

	/* Read the response */
	if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) <= 0) {
		printf("Read From Socket Failed...\n");
		return -1;
	}

	/* Parse and print the response */
	rtInfo = (struct route_info *)malloc(1024);
	for(; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len)) {
		memset(rtInfo, 0, sizeof(struct route_info));
		parseRoutes(nlMsg, rtInfo, gateway);
	}

	inet_aton(gateway, &addr);
	memcpy(&value, &addr, 4);
	dwGateWay = value;
	free(rtInfo);

	close(sock);
	return dwGateWay;
}
#endif

#if 0
void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo, int8_t *gateway)
{
  struct rtmsg *rtMsg;
  struct rtattr *rtAttr;
  int32_t rtLen;
  int8_t *tempBuf = NULL;
 //2007-12-10
  struct in_addr dst;
  struct in_addr gate;

  tempBuf = (char *)malloc(100);
  rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);
  // If the route is not for AF_INET or does not belong to main routing table
  //then return.
  if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
  return;

  rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
  rtLen = RTM_PAYLOAD(nlHdr);
  for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen)){
   switch(rtAttr->rta_type) {
   case RTA_OIF:
    if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
    break;
   case RTA_GATEWAY:
    rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
    break;
   case RTA_PREFSRC:
    rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
    break;
   case RTA_DST:
    rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
    break;
   }
  }
  //2007-12-10
  dst.s_addr = rtInfo->dstAddr;
  if (strstr((char *)inet_ntoa(dst), "0.0.0.0"))
  {
    gate.s_addr = rtInfo->gateWay;
    sprintf(gateway, (char *)inet_ntoa(gate));
  }
  free(tempBuf);
  return;
}
#endif

int32_t ip_get_proc(int8_t *dev)
{
	FILE *fp;
	char buf[512];
	uint32_t gateway = 0;
	// route
	fp = fopen("/proc/net/route", "r");
	if (fp == NULL) {
		perror("open file /proc/net/route failed");
		return -1;
	}
	while (fgets((char *)buf, sizeof(buf), fp) != NULL) {
		unsigned long dest, gate;
		char buffer[20] = {0};
		dest = gate = 0;
		sscanf(buf, "%s%lx%lx",buffer,&dest, &gate);
		if (dest == 0 && gate != 0 && (0 == strcmp(buffer,(char *)dev)))
		{
			gateway = gate;
			struct in_addr *addr = (struct in_addr *)&gateway;
			printf("    GateWay:%s\n", inet_ntoa(*addr));

			break;
		}
	}
	fclose(fp);

	return gateway;
}

uint32_t GetIPaddr(int8_t *interface_name)
{
	int32_t s;
	uint32_t ip;

	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, (char *)interface_name);

	if(ioctl(s, SIOCGIFADDR, &ifr) < 0) {
		perror("ioctl");
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_addr;

	printf("        IP:%s\n", inet_ntoa(ptr->sin_addr));

	memcpy(&ip, &ptr->sin_addr, 4);
	close(s);
	return ip;
}

uint32_t GetNetmask(int8_t *interface_name)
{
	int32_t s;
	uint32_t ip;

	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, (char *)interface_name);

	if(ioctl(s, SIOCGIFNETMASK, &ifr) < 0) {
		perror("ioctl");
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_netmask;

	printf("        Netmask:%s\n", inet_ntoa(ptr->sin_addr));

	memcpy(&ip, &ptr->sin_addr, 4);
	close(s);
	return ip;
}

uint32_t GetBroadcast(int8_t *interface_name)
{
	int32_t s;
	uint32_t ip;

	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return -1;
	}

	struct ifreq ifr;

	strcpy(ifr.ifr_name, (char *)interface_name);

	if(ioctl(s, SIOCGIFBRDADDR, &ifr) < 0) {
		perror("ioctl");
		return -1;
	}

	struct sockaddr_in *ptr;

	ptr = (struct sockaddr_in *) &ifr.ifr_ifru.ifru_broadaddr;

	printf("        Broadcast:%s\n", inet_ntoa(ptr->sin_addr));

	memcpy(&ip, &ptr->sin_addr, 4);
	close(s);
	return ip;

}

int32_t ntp_time_sync(int8_t *ipaddr)
{
	int8_t cmd[64] = {0};

	if(NULL == ipaddr){
		zlog_debug(OPELOG, "ntp_time_sync failed, params is NULL!\n");
		return -1;
	}

	sprintf((char *)cmd, "/usr/local/bin/ntpdate %s && hwclock --systohc", ipaddr);
	zlog_debug(OPELOG, "cmd: %s\n", cmd);
	r_system((const int8_t *)cmd);
	r_system("/etc/init.d/save-rtc.sh");
	return 0;
}

/*喂狗*/
int32_t write_watch_dog(int32_t fd)
{
	int32_t dummy;

	if(ioctl(fd, WDIOC_KEEPALIVE, &dummy) != 0) {
		fprintf(stderr, "write WatchDog failed strerror(errno) = %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

/*初始化看门狗*/
int32_t init_watch_dog(int32_t second)
{
	int32_t fd = -1;
	fd = open(WATCH_DOG_DEV, O_WRONLY);

	if(fd == -1) {
		fprintf(stderr, "open watch dog failed strerror(errno) = %s\n", strerror(errno));
		return -1;
	}

	struct timeval timeout = {second, 0};

	ioctl(fd, WDIOC_SETTIMEOUT, &timeout);

	fprintf(stderr, "init watch dog finished!\n");

	return fd;
}


