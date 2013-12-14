#ifndef __WEBLISTEN_H__
#define __WEBLISTEN_H__

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "stdint.h"
typedef struct hdmiDisplayResolution_ {
	int	res;
	char width[8];
	char height[8];
} hdmiDisplayResolution_t;

int32_t enc_app_weblisten_init(void *arg);
hdmiDisplayResolution_t *getHdmiDisplayResolution(void);
int32_t ReadHDMICfgFile(void);

#endif	//__WEBLISTEN_H__

