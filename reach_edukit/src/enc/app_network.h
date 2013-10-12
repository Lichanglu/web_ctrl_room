#ifndef _APP_NETWORK_H
#define _APP_NETWORK_H



 unsigned int GetIPaddr(char *interface_name);
 unsigned int getGateWay(char *interface_name);
 unsigned int GetNetmask(char *interface_name);
 unsigned int GetBroadcast(char *interface_name);

#endif
