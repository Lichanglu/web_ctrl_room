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
#include "webmiddle.h"
#include "reach_os.h"
#include "md5lib.h"
#include "command_resolve.h"
#include "params.h"
int32_t app_weblisten_init(server_set *pser);
extern int32_t SysRollback();
extern int32_t UpgradeApp(int8_t *filename);
extern int32_t updatekernel(void);
extern int32_t app_update_HD_card(int8_t *filename);
extern int32_t rebootsys(int32_t time);
#endif
