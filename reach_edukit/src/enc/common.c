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


#include "common.h"

#define SET_GPIO (0x55555555)
#define GET_GPIO (0xAAAAAAAA)



typedef struct _R_GPIO_data_ {
	unsigned int gpio_num;
	unsigned int gpio_value;
} R_GPIO_data;

#if 0
int FindH264StartNAL(unsigned char *pp)
{
	/*is for 00 00 00 01 Nal header*/
	if(pp[0] != 0 || pp[1] != 0 || pp[2] != 0 || pp[3] != 1) {
		return 0;
	} else {
		return 1;
	}
}


int ParseIDRHeader(unsigned char *pdata)
{
	int len = 0x17;
	int ret = 0, I_frame_header_length = 0;

	unsigned char *find = pdata;

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

	len = (int)(find - pdata);

	if(len > 0x40) {
		len = 0x18;
	}

	return len;
}
#endif

uint32_t getostime()
{
	struct timeval tv;
	struct timezone tz;
	uint32_t ultime;

	gettimeofday(&tv , &tz);
	ultime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return (ultime);
}

/*ÉèÖÃÍøÂçIP,ÑÚÂë,Íø¹Ø*/
void SetEthConfig(unsigned int ipaddr, unsigned netmask, unsigned int gateway)
{
	int ret;
	struct in_addr addr1, addr2;
	char temp[200];

	memset(temp, 0, sizeof(temp));
	memcpy(&addr1, &ipaddr, 4);
	memcpy(&addr2, &netmask, 4);
	strcpy(temp, "ifconfig eth0 ");
	strcat(temp, inet_ntoa(addr1));
	strcat(temp, " netmask ");
	strcat(temp, inet_ntoa(addr2));
	fprintf(stdout, "command:%s\n", temp);

	ret = system(temp);

	if(ret < 0) {
		fprintf(stdout, "ifconfig %s up ERROR\n", inet_ntoa(addr1));
	}

	memset(temp, 0, sizeof(temp));
	memcpy(&addr1, &gateway, 4);
	strcpy(temp, "route add default gw ");
	strcat(temp, inet_ntoa(addr1));
	fprintf(stdout, "command:%s\n", temp);

	ret = system(temp);

	if(ret < 0) {
		fprintf(stdout, "set gateway error\n");
	}
}

int ChangeSampleIndex(int index)
{
	int ret = 44100;

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

int CheckIPNetmask(int ipaddr, int netmask, int gw)
{
	int mask, ip, gateip;
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
int open_gpio_device()
{
	return open("/dev/Rgpio", O_RDWR);
}

int set_gpio_value(int fd, unsigned int gnum, int gvalue)
{
	int ret = 0;
	R_GPIO_data gpio_st;

	if(fd < 0) {
		fprintf(stderr, "set_gpio_value failed, gpio fd error!");
		return -1;
	}

	if(gvalue != 0) {
		gvalue = 1;
	}

	gpio_st.gpio_num = gnum;
	gpio_st.gpio_value = gvalue;

	ret = ioctl(fd, SET_GPIO, &gpio_st);

	if(ret < 0) {
		fprintf(stderr, "set gpio error!\n");
		return -1;
	}

	return 0;
}


int write_yuv_420(unsigned char *py, unsigned char *pu, unsigned char *pv,
                  int width, int height, int chid)
{
	unsigned char *pY = py;
	unsigned char *pU = pu;
	unsigned char *pV = pv;

	unsigned int i;
#if 0
	pY = addr;
	pU = addr + width * height; //2088960;//width*height;
	pV = addr + width * height + 1; //2088961;//width*height+1;
#endif

	char buf[256] = {0};
	static int time0 = 0;
	static int time1 = 0;
	static int time2 = 0;

	if(chid == 0) {
		sprintf(buf, "picture_yuv420_vp0_%d.yuv", time0++);
	} else if(chid == 1) {
		sprintf(buf, "picture_yuv420_vp1_%d.yuv", time1++);
	} else {
		sprintf(buf, "picture_yuv420_mp_%d.yuv", time2++);
	}


	FILE *fp = fopen(buf, "w+");

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

int write_yuv_422(unsigned char *py, unsigned char *pu, unsigned char *pv,
                  int width, int height, int chid)
{
	unsigned char *pY = py;
	unsigned char *pU = pu;
	unsigned char *pV = pv;

	unsigned int i;
#if 0
	pY = addr;
	pU = addr + width * height; //2088960;//width*height;
	pV = addr + width * height + 1; //2088961;//width*height+1;
#endif

	char buf[256] = {0};
	static int time0 = 0;
	static int time1 = 0;
	static int time2 = 0;

	if(chid == 0) {
		sprintf(buf, "picture_yuv422_vp0_%d.yuv", time0++);
	} else if(chid == 1) {
		sprintf(buf, "picture_yuv422_vp1_%d.yuv", time1++);
	} else {
		sprintf(buf, "picture_yuv422_mp_%d.yuv", time2++);
	}


	FILE *fp = fopen(buf, "w+");

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




