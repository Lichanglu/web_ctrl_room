#ifndef _WEBLISTEN_H
#define _WEBLISTEN_H

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

 int app_weblisten_init(void);

#endif
