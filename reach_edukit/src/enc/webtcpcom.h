#ifndef _WEBTCPCOM_H
#define _WEBTCPCOM_H


#define ROOM_TO_CTRL_SERVER_PORT				(20010)
#define ROOM_TO_CTRL_MSG_MAX_LENGHT				(264)



int appCmdIntParse(int cmd,int invalue,int inlen,int *outvalue,int *outlen,int port);
int appCmdStringParse(int cmd,char *invalue,int inlen,char  *outvalue,int *outlen,int port);
int appCmdStructParse(int cmd, void *inval, int len, void *outval,int *outlen,int port);
int roomgetPreviewPassword(char *password);

#endif
