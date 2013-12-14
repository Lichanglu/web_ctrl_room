#ifndef _WEBTCPCOM_H
#define _WEBTCPCOM_H


int appCmdIntParse(int cmd,int invalue,int inlen,int *outvalue,int *outlen);
int appCmdStringParse(int cmd,char *invalue,int inlen,char  *outvalue,int *outlen);
int appCmdStructParse(int cmd, void *inval, int len, void *outval,int *outlen);

#endif
