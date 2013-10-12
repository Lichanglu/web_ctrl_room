#ifndef _COMMON_H_
#define _COMMON_H_

#include <osa_thr.h>

#include <assert.h>


#define 	ENCODE_TYPE					6

#define MAX_CLIENT				6
#define DSP_NUM				    4




typedef int SOCKET;
typedef int DSPFD;
typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef unsigned char  BYTE;


#if 0
typedef enum {
    DL_NONE,
    DL_ERROR,
    DL_WARNING,
    DL_FLOW,
    DL_DEBUG,
    DL_ALL,
} EDebugLevle;

#define DEBUG_LEVEL   	(DL_ALL)


#define DEBUG(x,fmt,arg...) \
	do {\
		if ((x)<DEBUG_LEVEL){\
			fprintf(stderr, fmt, ##arg);\
		}\
	}while (0)


#endif


int ParseIDRHeader(unsigned char *pdata);
		
unsigned int getostime();

int ChangeSampleIndex(int index);

void SetEthConfig(unsigned int ipaddr, unsigned netmask, unsigned int gateway);

int CheckIPNetmask(int ipaddr, int netmask, int gw);

int set_gpio_value(int fd,unsigned int gnum, int gvalue);

int write_yuv_420(unsigned char *py, unsigned char *pu, unsigned char *pv, 
								int width, int height, int chid);

int write_yuv_422(unsigned char *py, unsigned char *pu, unsigned char *pv, 
								int width, int height, int chid);

int open_gpio_device();





#endif
