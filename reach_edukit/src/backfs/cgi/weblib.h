#ifndef _WEBLIB_H
#define _WEBLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "webtcpcom.h"
#include "../bkfs_middle.h"

extern int WebGetDevInfo(DevInfo *devinfo);
extern int WebGetSysDiskInfo(DiskInfo *diskinfo);
extern int WebSetUsrPassword(char *password);
extern int WebGetUsrPassword(char *password);
extern int WebSetEnChSwitch(int language);
extern int WebGetEnChSwitch(int *language);
extern int WebSetLanIpconfig(IpConfig *ipconfig);
extern int WebSetWanIpconfig(IpConfig *ipconfig);
extern int WebGetLanIpconfig(IpConfig *ipconfig);
extern int WebGetWanIpconfig(IpConfig *ipconfig);
extern int WebSysUpgrade(char *filename);
extern int WebSysRollBack(char *filename);
extern int WebRebootSys(int reboot);
extern int WebSetDevSerialnum(char *num);

extern int WebGetCtrlProto(ProtoList *outdex);
extern int WebUpgradeCtrlProto(char *outdex);
extern int WebDelCtrlProto(char *outdex);
extern int WebSetCtrlProto(char *outdex);


#endif
